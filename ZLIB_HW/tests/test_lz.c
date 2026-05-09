#include <criterion/criterion.h>
#include <stdlib.h>
#include <string.h>
#include "lz.h"
#include "stdio.h"
#define LARGE_SIZE_100K  (100 * 1024)
#define LARGE_SIZE_1M    (1024 * 1024)

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

/* Repeated short pattern to trigger many LZ matches */
static unsigned char* make_repeating_pattern(size_t len, const unsigned char* pat, size_t pat_len) {
	unsigned char* data = malloc(len);
	if (!data || pat_len == 0) return data;
	for (size_t i = 0; i < len; i++)
		data[i] = pat[i % pat_len];
	return data;
}

static void compare_tokens_with_file(const char* filename, const lz_token_t* tokens, size_t num_tokens) {
    FILE* f = fopen(filename, "rb");
    cr_assert_not_null(f, "failed to open reference file %s", filename);
    size_t ref_num_tokens = 0;
    fread(&ref_num_tokens, sizeof(size_t), 1, f);
    cr_assert_eq(num_tokens, ref_num_tokens, "token count %zu != reference %zu", num_tokens, ref_num_tokens);
    lz_token_t* ref_tokens = malloc(ref_num_tokens * sizeof(lz_token_t));
    cr_assert_not_null(ref_tokens);
    fread(ref_tokens, sizeof(lz_token_t), ref_num_tokens, f);
    fclose(f);
    for (size_t i = 0; i < num_tokens; i++) {
        cr_assert_eq(tokens[i].is_literal, ref_tokens[i].is_literal, "token %zu: is_literal mismatch", i);
        cr_assert_eq(tokens[i].literal,    ref_tokens[i].literal,    "token %zu: literal mismatch", i);
        cr_assert_eq(tokens[i].length,     ref_tokens[i].length,     "token %zu: length mismatch", i);
        cr_assert_eq(tokens[i].distance,   ref_tokens[i].distance,   "token %zu: distance mismatch", i);
    }
    free(ref_tokens);
}
Test(lz, compare_token_file_repeating) {
size_t len = LARGE_SIZE_100K;
unsigned char* data = make_repeating(len, 0xAB);
cr_assert_not_null(data);
size_t num_tokens = 0;
lz_token_t* tokens = lz_compress_tokens(data, len, &num_tokens);
cr_assert_not_null(tokens, "lz_compress_tokens failed");
compare_tokens_with_file("tests/data/ref_tokens_repeating.bin", tokens, num_tokens);
free(data);
free(tokens);
}

Test(lz, compare_token_file_sequential) {
size_t len = LARGE_SIZE_100K;
unsigned char* data = make_sequential(len);
cr_assert_not_null(data);
size_t num_tokens = 0;
lz_token_t* tokens = lz_compress_tokens(data, len, &num_tokens);
cr_assert_not_null(tokens, "lz_compress_tokens failed");
compare_tokens_with_file("tests/data/ref_tokens_sequential.bin", tokens, num_tokens);
free(data);
free(tokens);
}

Test(lz, compare_token_file_pseudo_random) {
size_t len = LARGE_SIZE_100K;
unsigned char* data = make_pseudo_random(len, 42);
cr_assert_not_null(data);
size_t num_tokens = 0;
lz_token_t* tokens = lz_compress_tokens(data, len, &num_tokens);
cr_assert_not_null(tokens, "lz_compress_tokens failed");
compare_tokens_with_file("tests/data/ref_tokens_pseudo_random.bin", tokens, num_tokens);
free(data);
free(tokens);
}

Test(lz, compare_token_file_1mb_stream) {
size_t len = LARGE_SIZE_1M;
unsigned char* data = make_pseudo_random(len, 123);
cr_assert_not_null(data);
size_t num_tokens = 0;
lz_token_t* tokens = lz_compress_tokens(data, len, &num_tokens);
cr_assert_not_null(tokens, "lz_compress_tokens failed on 1MB");
compare_tokens_with_file("tests/data/ref_tokens_1mb_stream.bin", tokens, num_tokens);
free(data);
free(tokens);
}

Test(lz, compare_token_file_repeating_pattern) {
unsigned char pat[] = "ABCDEFGHIJKLMNOP";
size_t len = LARGE_SIZE_100K;
unsigned char* data = make_repeating_pattern(len, pat, sizeof(pat) - 1);
cr_assert_not_null(data);
size_t num_tokens = 0;
lz_token_t* tokens = lz_compress_tokens(data, len, &num_tokens);
cr_assert_not_null(tokens, "lz_compress_tokens failed on repeating pattern");
compare_tokens_with_file("tests/data/ref_tokens_repeating_pattern.bin", tokens, num_tokens);
free(data);
free(tokens);
}
