#include "png_reader.h"
#include "png_crc.h"
#include "util.h"
#include "global.h"
#include <stdlib.h>
#include <string.h>

/* Opens a PNG file and validates signature */
FILE *png_open(const char *path)
{
    return NULL;
}

/* Reads the next chunk from the file */
int png_read_chunk(FILE *fp, png_chunk_t *out)
{
    return 0;
}

/* Frees memory allocated inside png_chunk_t */
void png_free_chunk(png_chunk_t *chunk)
{
	
}

int png_extract_ihdr(FILE *fp, png_ihdr_t *out)
{
    return 0;
}

int png_extract_plte(FILE *fp, png_color_t **out_colors, size_t *out_count)
{
    return 0;
}

int png_summary(const char *filename, png_chunk_t **out_summary)
{
    return 0;
}