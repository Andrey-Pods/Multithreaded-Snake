#include <criterion/criterion.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include "our_zlib.h"
#include "crc.h"

/* ─────────────────────────── helpers ─────────────────────────── */

static char* read_file(const char* path, size_t* len) {
	FILE* f = fopen(path, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	if (sz < 0) { fclose(f); return NULL; }
	rewind(f);
	char* buf = malloc((size_t)sz + 1);
	if (!buf) { fclose(f); return NULL; }
	if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { free(buf); fclose(f); return NULL; }
	fclose(f);
	buf[sz] = '\0';
	*len = (size_t)sz;
	return buf;
}

static char* read_gz_compressed(const char* path, size_t* comp_len, gz_header_t* hdr) {
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

/* Write buf to path; returns 0 on success. */
static int write_file(const char* path, const void* buf, size_t len) {
	FILE* f = fopen(path, "wb");
	if (!f) return -1;
	int ok = (fwrite(buf, 1, len, f) == len) ? 0 : -1;
	fclose(f);
	return ok;
}

/* ─────────────── parse_member / skip_gz_header tests ─────────── */

/*
 * Compress a known file with system gzip, then parse_member should populate
 * the header fields correctly (CM=8, full_size matches original).
 */
Test(parse_member, header_fields_from_system_gzip) {
	const char* tmp_txt = "/tmp/test_pm_input.txt";
	const char* tmp_gz  = "/tmp/test_pm_input.txt.gz";
	const char* content = "Parse member test content 1234567890\n";
	size_t content_len  = strlen(content);
	cr_assert_eq(write_file(tmp_txt, content, content_len), 0);

	char cmd[256];
	snprintf(cmd, sizeof(cmd), "gzip -k -f %s", tmp_txt);
	cr_assert_eq(system(cmd), 0, "system gzip failed");

	FILE* f = fopen(tmp_gz, "rb");
	cr_assert_not_null(f);
	gz_header_t hdr = {0};
	cr_assert_eq(parse_member(f, &hdr), 0, "parse_member returned error");
	fclose(f);

	cr_assert_eq(hdr.cm, 8, "CM should be 8 (deflate), got %d", hdr.cm);
	cr_assert_eq(hdr.full_size, (unsigned int)content_len,
		"ISIZE %u != original %zu", hdr.full_size, content_len);

	free(hdr.name);
	free(hdr.comment);
	free(hdr.extra);
	remove(tmp_txt);
	remove(tmp_gz);
}

/*
 * parse_member on a file that does NOT start with the gzip magic must return 1.
 */
Test(parse_member, rejects_non_gz_file) {
	const char* tmp = "/tmp/test_pm_bad.bin";
	const char* junk = "This is not a gzip file at all!";
	cr_assert_eq(write_file(tmp, junk, strlen(junk)), 0);
	FILE* f = fopen(tmp, "rb");
	cr_assert_not_null(f);
	gz_header_t hdr = {0};
	cr_assert_neq(parse_member(f, &hdr), 0, "parse_member should fail on non-gz data");
	fclose(f);
	remove(tmp);
}

/*
 * skip_gz_header_to_compressed_data should position the file pointer
 * at the first byte of compressed data (past the header).
 */
Test(parse_member, skip_header_positions_correctly) {
	const char* tmp_txt = "/tmp/test_skip_hdr.txt";
	const char* tmp_gz  = "/tmp/test_skip_hdr.txt.gz";
	const char* content = "Skip header test.";
	cr_assert_eq(write_file(tmp_txt, content, strlen(content)), 0);

	char cmd[256];
	snprintf(cmd, sizeof(cmd), "gzip -k -f %s", tmp_txt);
	cr_assert_eq(system(cmd), 0);

	FILE* f = fopen(tmp_gz, "rb");
	cr_assert_not_null(f);
	gz_header_t hdr = {0};
	cr_assert_eq(skip_gz_header_to_compressed_data(f, &hdr), 0);
	long pos = ftell(f);
	cr_assert(pos > 10, "file pointer %ld should be past the 10-byte fixed header", pos);
	fclose(f);

	free(hdr.name);
	free(hdr.comment);
	free(hdr.extra);
	remove(tmp_txt);
	remove(tmp_gz);
}

/* ───────────────────────── inflate tests ─────────────────────── */

/*
 * Inflate data compressed by system gzip - short ASCII string.
 */
Test(inflate, short_ascii_string) {
	const char* tmp_txt = "/tmp/test_inflate_short.txt";
	const char* tmp_gz  = "/tmp/test_inflate_short.txt.gz";
	const char* original = "Hello, gzip!";
	size_t orig_len = strlen(original);
	cr_assert_eq(write_file(tmp_txt, original, orig_len), 0);

	char cmd[256];
	snprintf(cmd, sizeof(cmd), "gzip -k -f %s", tmp_txt);
	cr_assert_eq(system(cmd), 0);

	size_t comp_len = 0;
	gz_header_t hdr = {0};
	char* comp = read_gz_compressed(tmp_gz, &comp_len, &hdr);
	cr_assert_not_null(comp);

	char* result = inflate(comp, comp_len);
	cr_assert_not_null(result, "inflate returned NULL");
	cr_assert_eq(memcmp(result, original, orig_len), 0, "inflated data does not match original");

	free(comp);
	free(result);
	free(hdr.name);
	free(hdr.comment);
	free(hdr.extra);
	remove(tmp_txt);
	remove(tmp_gz);
}

/*
 * Inflate a highly repetitive input (compresses very well).
 */
Test(inflate, highly_repetitive_input) {
	const char* tmp_txt = "/tmp/test_inflate_rep.txt";
	const char* tmp_gz  = "/tmp/test_inflate_rep.txt.gz";
	size_t orig_len = 4096;
	char* original = malloc(orig_len);
	cr_assert_not_null(original);
	memset(original, 'A', orig_len);
	cr_assert_eq(write_file(tmp_txt, original, orig_len), 0);

	char cmd[256];
	snprintf(cmd, sizeof(cmd), "gzip -k -f %s", tmp_txt);
	cr_assert_eq(system(cmd), 0);

	size_t comp_len = 0;
	gz_header_t hdr = {0};
	char* comp = read_gz_compressed(tmp_gz, &comp_len, &hdr);
	cr_assert_not_null(comp);

	char* result = inflate(comp, comp_len);
	cr_assert_not_null(result, "inflate returned NULL on repetitive input");
	cr_assert_eq(memcmp(result, original, orig_len), 0, "inflated data does not match original");

	free(original);
	free(comp);
	free(result);
	free(hdr.name);
	free(hdr.comment);
	free(hdr.extra);
	remove(tmp_txt);
	remove(tmp_gz);
}

/*
 * Inflate binary data (all byte values 0–255 cycling).
 */
Test(inflate, binary_data) {
	const char* tmp_txt = "/tmp/test_inflate_bin.bin";
	const char* tmp_gz  = "/tmp/test_inflate_bin.bin.gz";
	size_t orig_len = 1024;
	unsigned char* original = malloc(orig_len);
	cr_assert_not_null(original);
	for (size_t i = 0; i < orig_len; i++)
		original[i] = (unsigned char)(i & 0xFF);
	cr_assert_eq(write_file(tmp_txt, original, orig_len), 0);

	char cmd[256];
	snprintf(cmd, sizeof(cmd), "gzip -k -f %s", tmp_txt);
	cr_assert_eq(system(cmd), 0);

	size_t comp_len = 0;
	gz_header_t hdr = {0};
	char* comp = read_gz_compressed(tmp_gz, &comp_len, &hdr);
	cr_assert_not_null(comp);

	char* result = inflate(comp, comp_len);
	cr_assert_not_null(result, "inflate returned NULL on binary data");
	cr_assert_eq(memcmp(result, original, orig_len), 0, "inflated binary data does not match original");

	free(original);
	free(comp);
	free(result);
	free(hdr.name);
	free(hdr.comment);
	free(hdr.extra);
	remove(tmp_txt);
	remove(tmp_gz);
}

/*
 * Inflate a single-byte file.
 */
Test(inflate, single_byte) {
	const char* tmp_txt = "/tmp/test_inflate_1byte.bin";
	const char* tmp_gz  = "/tmp/test_inflate_1byte.bin.gz";
	unsigned char single = 0x42;
	cr_assert_eq(write_file(tmp_txt, &single, 1), 0);

	char cmd[256];
	snprintf(cmd, sizeof(cmd), "gzip -k -f %s", tmp_txt);
	cr_assert_eq(system(cmd), 0);

	size_t comp_len = 0;
	gz_header_t hdr = {0};
	char* comp = read_gz_compressed(tmp_gz, &comp_len, &hdr);
	cr_assert_not_null(comp);

	char* result = inflate(comp, comp_len);
	cr_assert_not_null(result, "inflate returned NULL for single byte");
	cr_assert_eq((unsigned char)result[0], single, "inflated byte does not match original");

	free(comp);
	free(result);
	free(hdr.name);
	free(hdr.comment);
	free(hdr.extra);
	remove(tmp_txt);
	remove(tmp_gz);
}

/* ───────────────────────── deflate tests ─────────────────────── */

/*
 * deflate on a short string produces non-NULL output with positive length.
 */
Test(deflate, produces_output) {
	const char* tmp = "/tmp/test_deflate_out.txt";
	const char* data = "Deflate test string!";
	cr_assert_eq(write_file(tmp, data, strlen(data)), 0);

	size_t out_len = 0;
	char* compressed = deflate((char*)tmp, (char*)data, strlen(data), &out_len);
	cr_assert_not_null(compressed, "deflate returned NULL");
	cr_assert(out_len > 0, "deflate produced 0 bytes");

	free(compressed);
	remove(tmp);
}

/*
 * Output of deflate must start with the gzip magic bytes (0x1f 0x8b).
 */
Test(deflate, output_starts_with_gzip_magic) {
	const char* tmp = "/tmp/test_deflate_magic.txt";
	const char* data = "Magic check";
	cr_assert_eq(write_file(tmp, data, strlen(data)), 0);

	size_t out_len = 0;
	char* compressed = deflate((char*)tmp, (char*)data, strlen(data), &out_len);
	cr_assert_not_null(compressed);
	cr_assert(out_len >= 2, "output too short");
	cr_assert_eq((unsigned char)compressed[0], 0x1f, "ID1 mismatch");
	cr_assert_eq((unsigned char)compressed[1], 0x8b, "ID2 mismatch");

	free(compressed);
	remove(tmp);
}

/*
 * The CM field (byte 2) in our deflate output must be 8 (deflate method).
 */
Test(deflate, cm_field_is_deflate) {
	const char* tmp = "/tmp/test_deflate_cm.txt";
	const char* data = "CM field test";
	cr_assert_eq(write_file(tmp, data, strlen(data)), 0);

	size_t out_len = 0;
	char* compressed = deflate((char*)tmp, (char*)data, strlen(data), &out_len);
	cr_assert_not_null(compressed);
	cr_assert(out_len >= 3);
	cr_assert_eq((unsigned char)compressed[2], 8, "CM should be 8");

	free(compressed);
	remove(tmp);
}

/*
 * deflate then inflate round-trip: recovered data must equal original.
 */
Test(deflate, round_trip_short_string) {
	const char* tmp = "/tmp/test_rt_short.txt";
	const char* original = "Round-trip: Hello World! 1234567890";
	size_t orig_len = strlen(original);
	cr_assert_eq(write_file(tmp, original, orig_len), 0);

	size_t out_len = 0;
	char* compressed = deflate((char*)tmp, (char*)original, orig_len, &out_len);
	cr_assert_not_null(compressed);

	/* Extract compressed payload (skip header, exclude trailer) */
	FILE* fmem = fmemopen(compressed, out_len, "rb");
	cr_assert_not_null(fmem);
	gz_header_t hdr = {0};
	cr_assert_eq(skip_gz_header_to_compressed_data(fmem, &hdr), 0);
	long comp_start = ftell(fmem);
	fclose(fmem);

	size_t comp_payload_len = out_len - 8 - (size_t)comp_start;
	char* payload = compressed + comp_start;

	char* recovered = inflate(payload, comp_payload_len);
	cr_assert_not_null(recovered, "inflate returned NULL during round-trip");
	cr_assert_eq(memcmp(recovered, original, orig_len), 0, "round-trip data mismatch");

	free(hdr.name);
	free(hdr.comment);
	free(hdr.extra);
	free(compressed);
	free(recovered);
	remove(tmp);
}

/*
 * deflate output is accepted by system gzip -t (integrity check).
 */
Test(deflate, accepted_by_system_gzip) {
	const char* tmp_input = "/tmp/test_deflate_gzip_t_input.txt";
	const char* tmp_gz    = "/tmp/test_deflate_gzip_t_output.gz";
	const char* data = "Verify with system gzip. ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789.";
	cr_assert_eq(write_file(tmp_input, data, strlen(data)), 0);

	size_t out_len = 0;
	char* compressed = deflate((char*)tmp_input, (char*)data, strlen(data), &out_len);
	cr_assert_not_null(compressed);
	cr_assert_eq(write_file(tmp_gz, compressed, out_len), 0);

	char cmd[256];
	snprintf(cmd, sizeof(cmd), "gzip -t %s", tmp_gz);
	cr_assert_eq(system(cmd), 0, "gzip -t rejected our compressed output");

	free(compressed);
	remove(tmp_input);
	remove(tmp_gz);
}

/*
 * deflate then decompress with system gzip -d; recovered data must match.
 */
Test(deflate, system_gzip_decompresses_correctly) {
	const char* tmp_input = "/tmp/test_deflate_sysdc_input.txt";
	const char* tmp_gz    = "/tmp/test_deflate_sysdc_output.gz";
	const char* tmp_dec   = "/tmp/test_deflate_sysdc_decoded.txt";
	const char* original =
		"The quick brown fox jumps over the lazy dog. "
		"Pack my box with five dozen liquor jugs. "
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789.";
	size_t orig_len = strlen(original);
	cr_assert_eq(write_file(tmp_input, original, orig_len), 0);

	size_t out_len = 0;
	char* compressed = deflate((char*)tmp_input, (char*)original, orig_len, &out_len);
	cr_assert_not_null(compressed);
	cr_assert_eq(write_file(tmp_gz, compressed, out_len), 0);

	char cmd[512];
	snprintf(cmd, sizeof(cmd), "gzip -d -c %s > %s", tmp_gz, tmp_dec);
	cr_assert_eq(system(cmd), 0, "gzip -d failed on our compressed file");

	size_t dec_len = 0;
	char* decoded = read_file(tmp_dec, &dec_len);
	cr_assert_not_null(decoded);
	cr_assert_eq(dec_len, orig_len, "decoded length %zu != original %zu", dec_len, orig_len);
	cr_assert_eq(memcmp(decoded, original, orig_len), 0, "decoded data does not match original");

	free(compressed);
	free(decoded);
	remove(tmp_input);
	remove(tmp_gz);
	remove(tmp_dec);
}

/*
 * deflate round-trip on a large (~50 KB) repeating-pattern buffer.
 */
Test(deflate, round_trip_large_repeating) {
	const char* tmp_input = "/tmp/test_deflate_large_rep.txt";
	const char* tmp_gz    = "/tmp/test_deflate_large_rep.gz";
	const char* tmp_dec   = "/tmp/test_deflate_large_rep_dec.txt";
	size_t orig_len = 50 * 1024;
	char* original = malloc(orig_len);
	cr_assert_not_null(original);
	for (size_t i = 0; i < orig_len; i++)
		original[i] = (char)('A' + (i % 26));
	cr_assert_eq(write_file(tmp_input, original, orig_len), 0);

	size_t out_len = 0;
	char* compressed = deflate((char*)tmp_input, original, orig_len, &out_len);
	cr_assert_not_null(compressed, "deflate returned NULL on large repeating input");
	cr_assert(out_len > 0);
	cr_assert_eq(write_file(tmp_gz, compressed, out_len), 0);

	char cmd[512];
	snprintf(cmd, sizeof(cmd), "gzip -d -c %s > %s", tmp_gz, tmp_dec);
	cr_assert_eq(system(cmd), 0, "gzip -d failed");

	size_t dec_len = 0;
	char* decoded = read_file(tmp_dec, &dec_len);
	cr_assert_not_null(decoded);
	cr_assert_eq(dec_len, orig_len);
	cr_assert_eq(memcmp(decoded, original, orig_len), 0, "large round-trip mismatch");

	free(original);
	free(compressed);
	free(decoded);
	remove(tmp_input);
	remove(tmp_gz);
	remove(tmp_dec);
}

/*
 * deflate with out_len = NULL must not crash.
 */
Test(deflate, null_out_len_does_not_crash) {
	const char* tmp = "/tmp/test_deflate_null_outlen.txt";
	const char* data = "No out_len pointer.";
	cr_assert_eq(write_file(tmp, data, strlen(data)), 0);

	char* compressed = deflate((char*)tmp, (char*)data, strlen(data), NULL);
	/* May or may not succeed, but must not segfault */
	free(compressed);
	remove(tmp);
}

/* ──────────────────── CRC / trailer integrity ────────────────── */

/*
 * The CRC32 in the gzip trailer produced by deflate must match
 * get_crc() computed independently over the original data.
 */
Test(deflate, trailer_crc_correct) {
	const char* tmp = "/tmp/test_deflate_crc.txt";
	const char* data = "CRC check data 12345";
	size_t data_len = strlen(data);
	cr_assert_eq(write_file(tmp, data, data_len), 0);

	size_t out_len = 0;
	char* compressed = deflate((char*)tmp, (char*)data, data_len, &out_len);
	cr_assert_not_null(compressed);
	cr_assert(out_len >= 8);

	/* Trailer is the last 8 bytes: bytes [out_len-8 .. out_len-5] = CRC32 LE */
	unsigned char* trailer = (unsigned char*)compressed + out_len - 8;
	unsigned int stored_crc =
		(unsigned int)trailer[0]        |
		((unsigned int)trailer[1] << 8) |
		((unsigned int)trailer[2] << 16)|
		((unsigned int)trailer[3] << 24);

	unsigned int expected_crc = get_crc((const unsigned char*)data, data_len);
	cr_assert_eq(stored_crc, expected_crc,
		"stored CRC 0x%08x != expected 0x%08x", stored_crc, expected_crc);

	free(compressed);
	remove(tmp);
}


  static int extract_payload(char* gz_buf, size_t gz_len,
                           char** payload, size_t* payload_len) {
	FILE* f = fmemopen(gz_buf, gz_len, "rb");
	if (!f) return -1;
	gz_header_t hdr = {0};
	if (skip_gz_header_to_compressed_data(f, &hdr) != 0) { fclose(f); return -1; }
	long start = ftell(f);
	fclose(f);
	if (start < 0 || (size_t)start + 8 > gz_len) return -1;
	*payload     = gz_buf + start;
	*payload_len = gz_len - 8 - (size_t)start;
	free(hdr.name);
	free(hdr.comment);
	free(hdr.extra);
	return 0;
}

/* ──────────────────── deflate → inflate round-trip tests ──────── */

/*
 * Round-trip: plain ASCII sentence.
 * Exercises the basic happy path end-to-end.
 */
Test(round_trip, ascii_sentence) {
	const char* tmp        = "/tmp/rt_ascii.txt";
	const char* original   = "The quick brown fox jumps over the lazy dog.";
	size_t      orig_len   = strlen(original);
	char*       compressed = NULL;
	char*       result     = NULL;

	cr_assert_eq(write_file(tmp, original, orig_len), 0);

	size_t gz_len = 0;
	compressed = deflate((char*)tmp, (char*)original, orig_len, &gz_len);
	cr_expect_not_null(compressed, "deflate returned NULL");
	cr_expect(gz_len > 0, "deflate produced 0 bytes");

	char* payload = NULL;
	size_t payload_len = 0;
	cr_expect_eq(extract_payload(compressed, gz_len, &payload, &payload_len), 0,
		"extract_payload failed");

	if (payload) {
		result = inflate(payload, payload_len);
		cr_expect_not_null(result, "inflate returned NULL");
		if (result)
			cr_expect_eq(memcmp(result, original, orig_len), 0,
				"round-trip data does not match original");
	}

	free(compressed);
	free(result);
	remove(tmp);
}

/*
 * Round-trip: all-zero buffer (maximum compression ratio).
 * Stresses the LZ77 back-reference path heavily.
 */
Test(round_trip, all_zeros) {
	const char* tmp      = "/tmp/rt_zeros.bin";
	size_t      orig_len = 8 * 1024;
	char*       original = calloc(1, orig_len);
	char*       compressed = NULL;
	char*       result     = NULL;

	cr_assert_not_null(original, "calloc failed");
	cr_assert_eq(write_file(tmp, original, orig_len), 0);

	size_t gz_len = 0;
	compressed = deflate((char*)tmp, original, orig_len, &gz_len);
	cr_expect_not_null(compressed, "deflate returned NULL on all-zeros");
	cr_expect(gz_len > 0);

	char* payload = NULL; size_t payload_len = 0;
	cr_expect_eq(extract_payload(compressed, gz_len, &payload, &payload_len), 0);

	if (payload) {
		result = inflate(payload, payload_len);
		cr_expect_not_null(result, "inflate returned NULL on all-zeros");
		if (result)
			cr_expect_eq(memcmp(result, original, orig_len), 0,
				"round-trip mismatch on all-zeros");
	}

	free(original);
	free(compressed);
	free(result);
	remove(tmp);
}

/*
 * Round-trip: all byte values 0x00–0xFF cycling (binary data).
 * Ensures no byte value is mishandled during compression or decompression.
 */
Test(round_trip, all_byte_values) {
	const char* tmp      = "/tmp/rt_allbytes.bin";
	size_t      orig_len = 1024;
	unsigned char* original  = malloc(orig_len);
	char*          compressed = NULL;
	char*          result     = NULL;

	cr_assert_not_null(original);
	for (size_t i = 0; i < orig_len; i++)
		original[i] = (unsigned char)(i & 0xFF);
	cr_assert_eq(write_file(tmp, original, orig_len), 0);

	size_t gz_len = 0;
	compressed = deflate((char*)tmp, (char*)original, orig_len, &gz_len);
	cr_expect_not_null(compressed, "deflate returned NULL on binary data");
	cr_expect(gz_len > 0);

	char* payload = NULL; size_t payload_len = 0;
	cr_expect_eq(extract_payload(compressed, gz_len, &payload, &payload_len), 0);

	if (payload) {
		result = inflate(payload, payload_len);
		cr_expect_not_null(result, "inflate returned NULL on binary data");
		if (result)
			cr_expect_eq(memcmp(result, original, orig_len), 0,
				"round-trip mismatch on binary data");
	}

	free(original);
	free(compressed);
	free(result);
	remove(tmp);
}

/*
 * Round-trip: repeating short pattern ("ABCDABCD...").
 * Exercises back-references across many identical short matches.
 */
Test(round_trip, repeating_pattern) {
	const char* tmp      = "/tmp/rt_pattern.bin";
	const char  pat[]    = "ABCDEFGH";
	size_t      pat_len  = sizeof(pat) - 1;
	size_t      orig_len = 16 * 1024;
	char*       original  = malloc(orig_len);
	char*       compressed = NULL;
	char*       result     = NULL;

	cr_assert_not_null(original);
	for (size_t i = 0; i < orig_len; i++)
		original[i] = pat[i % pat_len];
	cr_assert_eq(write_file(tmp, original, orig_len), 0);

	size_t gz_len = 0;
	compressed = deflate((char*)tmp, original, orig_len, &gz_len);
	cr_expect_not_null(compressed, "deflate returned NULL on repeating pattern");
	cr_expect(gz_len > 0);

	char* payload = NULL; size_t payload_len = 0;
	cr_expect_eq(extract_payload(compressed, gz_len, &payload, &payload_len), 0);

	if (payload) {
		result = inflate(payload, payload_len);
		cr_expect_not_null(result, "inflate returned NULL on repeating pattern");
		if (result)
			cr_expect_eq(memcmp(result, original, orig_len), 0,
				"round-trip mismatch on repeating pattern");
	}

	free(original);
	free(compressed);
	free(result);
	remove(tmp);
}

/*
 * Round-trip: pseudo-random data (~50 KB).
 * Low compressibility - exercises the stored/literal block paths.
 */
Test(round_trip, pseudo_random_50kb) {
	const char* tmp      = "/tmp/rt_random.bin";
	size_t      orig_len = 50 * 1024;
	unsigned char* original  = malloc(orig_len);
	char*          compressed = NULL;
	char*          result     = NULL;

	cr_assert_not_null(original);
	unsigned s = 0xCAFEBABEu;
	for (size_t i = 0; i < orig_len; i++) {
		s = s * 1103515245u + 12345u;
		original[i] = (unsigned char)(s >> 16);
	}
	cr_assert_eq(write_file(tmp, original, orig_len), 0);

	size_t gz_len = 0;
	compressed = deflate((char*)tmp, (char*)original, orig_len, &gz_len);
	cr_expect_not_null(compressed, "deflate returned NULL on pseudo-random data");
	cr_expect(gz_len > 0);

	char* payload = NULL; size_t payload_len = 0;
	cr_expect_eq(extract_payload(compressed, gz_len, &payload, &payload_len), 0);

	if (payload) {
		result = inflate(payload, payload_len);
		cr_expect_not_null(result, "inflate returned NULL on pseudo-random data");
		if (result)
			cr_expect_eq(memcmp(result, original, orig_len), 0,
				"round-trip mismatch on pseudo-random data");
	}

	free(original);
	free(compressed);
	free(result);
	remove(tmp);
}

/*
 * Round-trip: multi-block input larger than DEFLATE_BLOCK_SIZE.
 * Forces deflate to emit more than one DEFLATE block, verifying
 * that inflate correctly reassembles across block boundaries.
 */
Test(round_trip, multi_block) {
	const char* tmp      = "/tmp/rt_multiblock.bin";
	/* Use 3x the typical block size to guarantee multiple blocks */
	size_t      orig_len = 3 * DEFLATE_BLOCK_SIZE + 1;
	char*       original  = malloc(orig_len);
	char*       compressed = NULL;
	char*       result     = NULL;

	cr_assert_not_null(original);
	/* Mix of compressible and less-compressible data */
	for (size_t i = 0; i < orig_len; i++)
		original[i] = (char)((i % 251) ^ (i >> 8));
	cr_assert_eq(write_file(tmp, original, orig_len), 0);

	size_t gz_len = 0;
	compressed = deflate((char*)tmp, original, orig_len, &gz_len);
	cr_expect_not_null(compressed, "deflate returned NULL on multi-block input");
	cr_expect(gz_len > 0);

	char* payload = NULL; size_t payload_len = 0;
	cr_expect_eq(extract_payload(compressed, gz_len, &payload, &payload_len), 0);

	if (payload) {
		result = inflate(payload, payload_len);
		cr_expect_not_null(result, "inflate returned NULL on multi-block input");
		if (result)
			cr_expect_eq(memcmp(result, original, orig_len), 0,
				"round-trip mismatch across block boundaries");
	}

	free(original);
	free(compressed);
	free(result);
	remove(tmp);
}