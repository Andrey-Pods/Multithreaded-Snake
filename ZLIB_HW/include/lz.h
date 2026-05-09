#ifndef LZ_H
#define LZ_H

#include <stddef.h>
#define LITLENGTH_BITS 9
#define DISTANCE_BITS 5

typedef struct {
    int is_literal;
    unsigned char literal;
    unsigned int length;
    unsigned int distance;
} lz_token_t;

lz_token_t* lz_compress_tokens(const unsigned char* data, size_t len, size_t* num_tokens);

unsigned char * lz_to_length_distance_codes(unsigned char *data, size_t len, size_t *out_len);

#endif
