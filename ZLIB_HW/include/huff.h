#ifndef HUFF_H
#define HUFF_H

#include <stddef.h>
#include "lz.h"

/* Decode Huffman-encoded buffer. Returns malloc'd buffer or NULL.
 * *out_len is set to the decoded length.
 * history/history_len: previously decompressed data for cross-block distance refs.
 * Pass NULL/0 for single-block or standalone decode. */
unsigned char* huffman_decode(const unsigned char* data, size_t enc_len, unsigned int btype_val, unsigned long*bits_read, size_t* out_len, const unsigned char* history, size_t history_len);

unsigned char* huffman_encode_tokens(const lz_token_t* tokens, size_t num_tokens, unsigned long* bits_written, size_t* out_len, unsigned char *returned_btype);

#endif
