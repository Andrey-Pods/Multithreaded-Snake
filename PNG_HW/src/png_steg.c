#include "png_steg.h"
#include "png_reader.h"
#include "png_chunks.h"
#include "png_crc.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

/* Encode secret string into LSBs of image data */
int png_encode_lsb(const char *input_path, const char *output_path, const char *secret)
{
    return 0;
}

/* Extract secret string from LSBs of image data */
int png_extract_lsb(const char *input_path, char *out, size_t max_len)
{
    return 0;
}

