#ifndef PNG_CHUNKS_H
#define PNG_CHUNKS_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t  bit_depth;
    uint8_t  color_type;
    uint8_t  compression;
    uint8_t  filter;
    uint8_t  interlace;
} png_ihdr_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} png_color_t;

typedef struct {
    uint32_t length;
    char     type[5];   /* Null-terminated */
    uint8_t *data;      /* length bytes */
    uint32_t crc;
} png_chunk_t;

/* Parse IHDR data from chunk */
/* Chunk must be an IHDR chunk with length 13 */
int png_parse_ihdr(const png_chunk_t *chunk, png_ihdr_t *out);

/* Parse PLTE data from chunk into an allocated array of colors */
/* Chunk must be a PLTE chunk with length multiple of 3 */
/* Caller owns *out_colors and must free it with free(). */
int png_parse_plte(const png_chunk_t *chunk, png_color_t **out_colors, size_t *out_count);

#endif
