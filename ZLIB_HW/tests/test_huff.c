#include <criterion/criterion.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "huff.h"
#include "lz.h"

#define LARGE_SIZE_100K  (100 * 1024)
#define LARGE_SIZE_1M    (1024 * 1024)

static lz_token_t* get_tokens_from_file(const char* filename, size_t* num_tokens) {
    FILE* f = fopen(filename, "r");
    cr_assert_not_null(f, "failed to open reference file %s", filename);
    size_t ref_num_tokens = 0;
    fread(&ref_num_tokens, sizeof(size_t), 1, f);
    lz_token_t* ref_tokens = malloc(ref_num_tokens * sizeof(lz_token_t));
    cr_assert_not_null(ref_tokens);
    fread(ref_tokens, sizeof(lz_token_t), ref_num_tokens, f);
    fclose(f);
	*num_tokens = ref_num_tokens;
	return ref_tokens;
}
static void compare_bytes_with_file(const char* filename, const void* data, size_t len) {
    FILE* f = fopen(filename, "r");
    cr_assert_not_null(f, "failed to open reference file %s", filename);
    size_t ref_len = 0;
    fread(&ref_len, sizeof(size_t), 1, f);
    cr_assert_eq(len, ref_len, "byte count %zu != reference length %zu", len, ref_len);
    lz_token_t* ref_data = malloc(ref_len);
    cr_assert_not_null(ref_data);
    fread(ref_data, 1, ref_len, f);
    fclose(f);
	cr_assert_arr_eq(data, ref_data, ref_len, "data mismatch");
	free(ref_data);
}

static unsigned char* make_repeating(size_t len, unsigned char byte) {
	unsigned char* data = malloc(len);
	if (!data) return NULL;
	memset(data, byte, len);
	return data;
}

static unsigned char* make_sequential(size_t len) {
	unsigned char* data = malloc(len);
	if (!data) return NULL;
	for (size_t i = 0; i < len; i++)
		data[i] = (unsigned char)(i & 0xFF);
	return data;
}

static unsigned char* make_pseudo_random(size_t len, unsigned seed) {
	unsigned char* data = malloc(len);
	if (!data) return NULL;
	unsigned s = seed;
	for (size_t i = 0; i < len; i++) {
		s = s * 1103515245u + 12345u;
		data[i] = (unsigned char)(s >> 16);
	}
	return data;
}

Test(huff, encode_decode_large_repeating) {
	size_t len = LARGE_SIZE_100K;
	unsigned char* data = make_repeating(len, 0xAB);
	cr_assert_not_null(data);
	size_t num_tokens = 0;
	lz_token_t* tokens = get_tokens_from_file("tests/data/ref_tokens_repeating.bin", &num_tokens);

	unsigned long b = 0;
	size_t enc_len = 0;
	unsigned char btype = 0;

	unsigned char* enc = huffman_encode_tokens(tokens, num_tokens, &b, &enc_len, &btype);
	cr_assert_not_null(enc, "huffman_encode failed");
	compare_bytes_with_file("tests/data/ref_encoded_repeating.bin", enc, enc_len);

	size_t dec_len = 0;
	unsigned long bits_read = 0;
	unsigned char* dec = huffman_decode(enc, enc_len,(unsigned int) btype,&bits_read, &dec_len, NULL, 0);
	cr_assert_not_null(dec, "huffman_decode failed");
	cr_assert_eq(dec_len, len, "decoded length %zu != original %zu", dec_len, len);
	cr_assert_eq(memcmp(data, dec, len), 0, "decode does not match original");
	for (int i = 0; i < dec_len; i++) {
		if (data[i] != dec[i]) fprintf(stderr, "mismatch at %d", i);
	}

	free(tokens);
	free(data);
	free(enc);
	free(dec);
}

Test(huff, encode_decode_large_sequential) {
	size_t len = LARGE_SIZE_100K;
	unsigned char* data = make_sequential(len);
	cr_assert_not_null(data);
	size_t num_tokens = 0;
	lz_token_t* tokens = get_tokens_from_file("tests/data/ref_tokens_sequential.bin", &num_tokens);

	unsigned long b;
	size_t enc_len = 0;
	unsigned char btype = 0;
	unsigned char* enc = huffman_encode_tokens(tokens, num_tokens, &b, &enc_len, &btype);
	cr_assert_not_null(enc, "huffman_encode failed");
	compare_bytes_with_file("tests/data/ref_encoded_sequential.bin", enc, enc_len);

	size_t dec_len = 0;
	unsigned long bits_read = 0;
	unsigned char* dec = huffman_decode(enc, enc_len, (unsigned int) btype, &bits_read, &dec_len, NULL, 0);
	cr_assert_not_null(dec, "huffman_decode failed");
	cr_assert_eq(dec_len, len, "decoded length %zu != original %zu", dec_len, len);
	cr_assert_eq(memcmp(data, dec, len), 0, "decode does not match original");

	free(tokens);
	free(data);
	free(enc);
	free(dec);
}

Test(huff, encode_decode_large_pseudo_random) {
	size_t len = LARGE_SIZE_100K;
	unsigned char* data = make_pseudo_random(len, 42);
	cr_assert_not_null(data);
	size_t num_tokens = 0;
	lz_token_t* tokens = get_tokens_from_file("tests/data/ref_tokens_pseudo_random.bin", &num_tokens);

	unsigned long b;
	size_t enc_len = 0;
	unsigned char btype = 0;
	unsigned char* enc = huffman_encode_tokens(tokens, num_tokens, &b, &enc_len, &btype);
	cr_assert_not_null(enc, "huffman_encode failed");
	compare_bytes_with_file("tests/data/ref_encoded_pseudo_random.bin", enc, enc_len);

	size_t dec_len = 0;
	unsigned long bits_read = 0;
	unsigned char* dec = huffman_decode(enc, enc_len, (unsigned int) btype, &bits_read, &dec_len, NULL, 0);
	cr_assert_not_null(dec, "huffman_decode failed");
	cr_assert_eq(dec_len, len, "decoded length %zu != original %zu", dec_len, len);
	cr_assert_eq(memcmp(data, dec, len), 0, "decode does not match original");

	free(tokens);
	free(data);
	free(enc);
	free(dec);
}

Test(huff, encode_decode_1mb_stream) {
	size_t len = LARGE_SIZE_1M;
	unsigned char* data = make_pseudo_random(len, 123);
	cr_assert_not_null(data);
	size_t num_tokens = 0;
	lz_token_t* tokens = get_tokens_from_file("tests/data/ref_tokens_1mb_stream.bin", &num_tokens);

	unsigned long b;
	size_t enc_len = 0;
	unsigned char btype = 0;
	unsigned char* enc = huffman_encode_tokens(tokens, num_tokens, &b, &enc_len,&btype);
	cr_assert_not_null(enc, "huffman_encode failed on 1MB");
	compare_bytes_with_file("tests/data/ref_encoded_1mb_stream.bin", enc, enc_len);

	size_t dec_len = 0;
	unsigned long bits_read = 0;
	unsigned char* dec = huffman_decode(enc, enc_len, (unsigned int)btype, &bits_read, &dec_len, NULL, 0);
	cr_assert_not_null(dec, "huffman_decode failed on 1MB");
	cr_assert_eq(dec_len, len, "decoded length %zu != original %zu", dec_len, len);
	cr_assert_eq(memcmp(data, dec, len), 0, "decode does not match original for 1MB");

	free(tokens);
	free(data);
	free(enc);
	free(dec);
}

Test(huff, encode_decode_empty) {
	size_t enc_len = 0;
	unsigned long b;
	unsigned char btype = 0;
	unsigned char* enc = huffman_encode_tokens(NULL, 0, &b, &enc_len, &btype);
	cr_assert(enc == NULL || enc_len == 0);
	if (enc) free(enc);
}

Test(huff, encode_decode_single_byte) {
	lz_token_t one = {.literal = 0x77, .is_literal = 1};
	size_t enc_len = 0;
	unsigned long b;
	unsigned char btype = 0;
	unsigned char* enc = huffman_encode_tokens(&one, 1, &b, &enc_len, &btype);
	cr_assert_not_null(enc);

	size_t dec_len = 0;
	unsigned long bits_read = 0;
	unsigned char* dec = huffman_decode(enc, enc_len,(unsigned int)btype,&bits_read, &dec_len, NULL, 0);
	cr_assert_not_null(dec);
	cr_assert_eq(dec_len, 1u);
	cr_assert_eq(dec[0], 0x77);

	free(enc);
	free(dec);
}



