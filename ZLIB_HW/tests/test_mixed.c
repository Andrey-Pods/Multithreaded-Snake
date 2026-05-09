#include <criterion/criterion.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include "our_zlib.h"

/* Helper: read entire file into malloc'd buffer, set *len. Returns NULL on error. */
static char* read_file(const char* path, size_t* len) {
	FILE* f = fopen(path, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	if (sz < 0) { fclose(f); return NULL; }
	rewind(f);
	char* buf = malloc((size_t)sz);
	if (!buf) { fclose(f); return NULL; }
	if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { free(buf); fclose(f); return NULL; }
	fclose(f);
	*len = (size_t)sz;
	return buf;
}

/* Helper: read compressed data from a .gz file (skip header, exclude 8-byte trailer).
   Also fills header so caller can get full_size. Returns malloc'd buffer, sets *comp_len. */
static char*  read_gz_compressed(const char* path, size_t* comp_len, gz_header_t* hdr) {
	FILE* f = fopen(path, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	long file_size = ftell(f);
	rewind(f);
	if (parse_member(f, hdr) != 0) { fclose(f); return NULL; }
	rewind(f);
	if (skip_gz_header_to_compressed_data(f, hdr) != 0) { fclose(f); return NULL; }
	long comp_start = ftell(f);
	if (comp_start < 0 || comp_start + 8 > file_size) { fclose(f); return NULL; }
	*comp_len = (size_t)(file_size - 8 - comp_start);
	char* buf = malloc(*comp_len);
	if (!buf || fread(buf, 1, *comp_len, f) != *comp_len) { free(buf); fclose(f); return NULL; }
	fclose(f);
	return buf;
}

/*
 * Test 1: gzip compresses README.md, our inflate decompresses it, compare to original.
 */
Test(mixed, gzip_decompress_readme) {
	/* Read original */
	size_t orig_len = 0;
	char* orig = read_file("README.md", &orig_len);
	cr_assert_not_null(orig, "Could not read README.md");
	cr_assert(orig_len > 0, "README.md is empty");

	/* Compress with system gzip */
	int rc = system("gzip -k -f README.md");
	cr_assert_eq(rc, 0, "gzip -k -f README.md failed");

	/* Read compressed file and inflate */
	size_t comp_len = 0;
	gz_header_t hdr = {0};
	char* comp = read_gz_compressed("README.md.gz", &comp_len, &hdr);
	cr_assert_not_null(comp, "Could not read README.md.gz");
	cr_assert(hdr.full_size == (unsigned int)orig_len,
		"ISIZE %u != original %zu", hdr.full_size, orig_len);

	char* decompressed = inflate(comp, comp_len);
	cr_assert_not_null(decompressed, "inflate returned NULL");
	cr_assert_eq(memcmp(orig, decompressed, orig_len), 0,
		"Decompressed data does not match original");

	free(orig);
	free(comp);
	free(decompressed);
	remove("README.md.gz");
}

/*
 * Test 2: our deflate compresses a buffer, gzip -d decompresses it, compare to original.
 */
Test(mixed, zlib_compress_gzip_decompress) {
	const char* test_str = "Hello, this is a test string for gzip interop testing. "
		"It needs to be long enough to exercise the compression path adequately. "
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz 0123456789 "
		"The quick brown fox jumps over the lazy dog. Pack my box with five dozen liquor jugs.";
	size_t test_len = strlen(test_str);

	/* Write original to temp file (deflate needs a filename for the header) */
	const char* tmp_input = "/tmp/test_mixed_input.txt";
	const char* tmp_gz = "/tmp/test_mixed_output.gz";
	const char* tmp_dec = "/tmp/test_mixed_decoded.txt";
	FILE* f = fopen(tmp_input, "wb");
	cr_assert_not_null(f);
	fwrite(test_str, 1, test_len, f);
	fclose(f);

	/* Compress with our deflate */
	size_t out_len = 0;
	char* compressed = deflate((char*)tmp_input, (char*)test_str, test_len, &out_len);
	cr_assert_not_null(compressed, "deflate returned NULL");
	cr_assert(out_len > 0, "deflate produced 0 bytes");

	/* Write compressed data */
	f = fopen(tmp_gz, "wb");
	cr_assert_not_null(f);
	fwrite(compressed, 1, out_len, f);
	fclose(f);

	/* Validate with gzip -t */
	char cmd[512];
	snprintf(cmd, sizeof(cmd), "gzip -t %s", tmp_gz);
	int rc = system(cmd);
	cr_assert_eq(rc, 0, "gzip -t rejected our compressed file");

	/* Decompress with gzip */
	snprintf(cmd, sizeof(cmd), "gzip -d -c %s > %s", tmp_gz, tmp_dec);
	rc = system(cmd);
	cr_assert_eq(rc, 0, "gzip -d failed on our compressed file");

	/* Compare */
	size_t dec_len = 0;
	char* decoded = read_file(tmp_dec, &dec_len);
	cr_assert_not_null(decoded);
	cr_assert_eq(dec_len, test_len, "Decoded length %zu != original %zu", dec_len, test_len);
	cr_assert_eq(memcmp(test_str, decoded, test_len), 0,
		"Decoded data does not match original");

	free(compressed);
	free(decoded);
	remove(tmp_input);
	remove(tmp_gz);
	remove(tmp_dec);
}

/*
 * Test 3: deflate README.md, then inflate the result, compare to original.
 */
Test(mixed, zlib_roundtrip_readme) {
	size_t orig_len = 0;
	char* orig = read_file("README.md", &orig_len);
	cr_assert_not_null(orig, "Could not read README.md");
	cr_assert(orig_len > 0);

	/* Compress */
	size_t comp_len = 0;
	char* compressed = deflate("README.md", orig, orig_len, &comp_len);
	cr_assert_not_null(compressed, "deflate returned NULL");
	cr_assert(comp_len > 0);

	/* Write to temp file so we can parse header with read_gz_compressed */
	const char* tmp_gz = "/tmp/test_mixed_roundtrip.gz";
	FILE* f = fopen(tmp_gz, "wb");
	cr_assert_not_null(f);
	fwrite(compressed, 1, comp_len, f);
	fclose(f);

	/* Read back and inflate */
	size_t inf_comp_len = 0;
	gz_header_t hdr = {0};
	char* comp_data = read_gz_compressed(tmp_gz, &inf_comp_len, &hdr);
	cr_assert_not_null(comp_data, "Could not read back compressed file");

	char* decompressed = inflate(comp_data, inf_comp_len);
	cr_assert_not_null(decompressed, "inflate returned NULL");
	cr_assert_eq(memcmp(orig, decompressed, orig_len), 0,
		"Roundtrip data does not match original");

	free(orig);
	free(compressed);
	free(comp_data);
	free(decompressed);
	remove(tmp_gz);
}

/*
 * Test 4: deflate + inflate roundtrip with a small inline string.
 */
Test(mixed, zlib_roundtrip_small) {
	const char* msg = "Small roundtrip test!";
	size_t msg_len = strlen(msg);

	/* Write to temp file for deflate's stat() call */
	const char* tmp_input = "/tmp/test_mixed_small.txt";
	FILE* f = fopen(tmp_input, "wb");
	cr_assert_not_null(f);
	fwrite(msg, 1, msg_len, f);
	fclose(f);

	/* Compress */
	size_t comp_len = 0;
	char* compressed = deflate((char*)tmp_input, (char*)msg, msg_len, &comp_len);
	cr_assert_not_null(compressed, "deflate returned NULL");
	cr_assert(comp_len > 0);

	/* Write compressed to temp, read back, inflate */
	const char* tmp_gz = "/tmp/test_mixed_small.gz";
	f = fopen(tmp_gz, "wb");
	cr_assert_not_null(f);
	fwrite(compressed, 1, comp_len, f);
	fclose(f);

	size_t inf_comp_len = 0;
	gz_header_t hdr = {0};
	char* comp_data = read_gz_compressed(tmp_gz, &inf_comp_len, &hdr);
	cr_assert_not_null(comp_data, "Could not read back compressed file");

	char* decompressed = inflate(comp_data, inf_comp_len);
	cr_assert_not_null(decompressed, "inflate returned NULL");
	cr_assert_eq(memcmp(msg, decompressed, msg_len), 0,
		"Roundtrip data does not match original");

	free(compressed);
	free(comp_data);
	free(decompressed);
	remove(tmp_input);
	remove(tmp_gz);
}
