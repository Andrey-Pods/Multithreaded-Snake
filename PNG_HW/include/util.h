#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

/* Big-endian helpers */
uint32_t read_u32_be(const uint8_t *buf);

/* Safe read helpers */
int read_exact(FILE *fp, uint8_t *buf, size_t len);

/* Zlib compression/decompression helpers */
/* Decompress data using zlib inflate */
/* Returns 0 on success, -1 on error. Caller must free *out_data. */
int util_inflate_data(const uint8_t *compressed, size_t compressed_size,
                      uint8_t **out_data, size_t *out_size);

/* Compress data using zlib deflate with default settings */
/* Returns 0 on success, -1 on error. Caller must free *out_data. */
int util_deflate_data(const uint8_t *data, size_t data_size,
                      uint8_t **out_data, size_t *out_size);

/* Compress data using zlib deflate with PNG-compatible settings */
/* PNG requires windowBits = 15 (32KB window) */
/* Returns 0 on success, -1 on error. Caller must free *out_data. */
int util_deflate_data_png(const uint8_t *data, size_t data_size,
                          uint8_t **out_data, size_t *out_size);

#endif
