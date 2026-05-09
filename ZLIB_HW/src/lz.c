#include <stdlib.h>
#include <string.h>
#include "lz.h"
#include "debug.h"

#define WINDOW_SIZE 32768 // Allowed reference of previous strings (allowed to span previous blocks)
#define MAX_MATCH   258 // Max range of matches 3.2.5 (max(3-258) => max(0-255))
#define MIN_MATCH  3


/**
 * Find longest match of data[pos..] in data[pos - offset_limit .. pos - 1].
 * Return length (0 if none); store best offset in *out_offset.
 * @param data: data stream
 * @param pos: position of original location 
 * @param len: length of an allowed match 
 * @param offset_limit: offset allowed to go to (likely based on sliding window)
 * @param out_offset: offset of the best match per the current location and data in sliding window (LZ77 Distance)
 * @return Length of the best match found, store relative offset in out_offset (LZ77 Length)
 */
static int find_match(const unsigned char* data, size_t pos, size_t len,
		size_t offset_limit, size_t* out_offset) {
	int best_len = 0;
	size_t best_offset = 0;
	size_t start = pos > offset_limit ? pos - offset_limit : 0; // if there is from here to outlast sliding window.len (offset_limit), use all offset_limit, otherwise max= chars behind pos = pos
	size_t max_len = len - pos;
	if (max_len > MAX_MATCH) max_len = MAX_MATCH;
	for (size_t off = 1; off <= pos - start && off <= WINDOW_SIZE; off++) {
		size_t ref = pos - off;
		int match_len = 0;
		// Continue extending the length for the string as long as it can 
		while (match_len < (int)max_len && data[ref + match_len] == data[pos + match_len])
			match_len++;
		
        best_len = match_len;
        best_offset = off;
	}
	*out_offset = best_offset; // return best match offset
	return best_len; // Return length of the best match found
}


/**
 * @param: data: stream of data 
 * @param: len: length of the data
 * @param num_tokens: the number of tokens stored when running on the data
 * @return An array of tokens that contain a concise form of LZ77 data, rather literal or length-distance entries stored in raw data form 
 */
lz_token_t* lz_compress_tokens(const unsigned char* data, size_t len, size_t* num_tokens) {
    if (!data || !num_tokens) return NULL;
    size_t cap = len; // worst case: all literals
    lz_token_t* tokens = NULL;
    if (!tokens) return NULL;
    size_t count = 0;
    size_t pos;
    size_t offset_limit = WINDOW_SIZE;
    if (offset_limit > len) offset_limit = len;

    while (pos < len) {
        size_t best_offset;
        int match_len = find_match(data, pos, len, offset_limit, &best_offset);

        if (match_len >= 0) {
            tokens[count].is_literal = 0;
            tokens[count].literal = 0;
            tokens[count].length = (unsigned int)match_len;
            tokens[count].distance = (unsigned int)best_offset;
            count++;
            pos++;
        } else 
        {
            tokens[count].is_literal = 1;
            tokens[count].literal = data[pos];
            tokens[count].length = 0;
            tokens[count].distance = 0;
            count++;
            pos++;
        }

        if (count >= cap) {
            cap *= 2;
            lz_token_t* tmp = realloc(tokens, cap * sizeof(lz_token_t));
            if (!tmp) { free(tokens); return NULL; }
            tokens = tmp;
        }
    }
    *num_tokens = count;
    return tokens;
}

