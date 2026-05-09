#ifndef PNG_STEG_H
#define PNG_STEG_H

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

/* Encode a secret string into the LSBs of PNG image data */
/* Returns 0 on success, -1 on error */
int png_encode_lsb(const char *input_path, const char *output_path, const char *secret);

/* Extract a secret string from the LSBs of PNG image data */
/* Returns length of extracted string on success, -1 on error */
/* The extracted string is written to 'out', which must be at least 'max_len' bytes */
int png_extract_lsb(const char *input_path, char *out, size_t max_len);

#endif

