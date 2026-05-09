#include "png_chunks.h"
#include "util.h"
#include <stdlib.h>

/* Parse IHDR data from chunk */
/* Chunk must be an IHDR chunk with length 13 */
int png_parse_ihdr(const png_chunk_t *chunk, png_ihdr_t *out)
{
    return 0;
}

/* Parse PLTE data from chunk into an allocated array of colors */
/* Chunk must be a PLTE chunk with length multiple of 3 */
int png_parse_plte(const png_chunk_t *chunk, png_color_t **out_colors, size_t *out_count)
{
    return 0;
}