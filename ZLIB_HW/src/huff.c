#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "huff.h"
#include "lz.h"
#include "queue.h"
#include "utility.h"
#include "debug.h"
#include "our_zlib.h"

#define NUM_SYMS 256
#define NUM_SYMS_AND_LENGTHS 288
#define NUM_DISTANCES 30
#define MAX_CODE_LEN 15 // 3.2.7
#define DECODE_TABLE_BITS 15
#define NUM_CODE_LENGTH_CODES 19

/* Length code table (RFC 1951 3.2.5) - shared by encode + decode */
static const struct {
	unsigned int base;
	unsigned int extra;
} len_table[29] = {
    {3,0},	{4,0},	{5,0},	{6,0},	{7,0},	{8,0},	{9,0},	{10,0},
    {11,1},	{13,1},	{15,1},	{17,1},	{19,2},	{23,2},	{27,2},	{31,2},
    {35,3},	{43,3},	{51,3},	{59,3},	{67,4},	{83,4},	{99,4},	{115,4},
    {131,5},{163,5},{195,5},{227,5},{258,0}
};
/* Distance code table (RFC 1951 3.2.5) - shared by encode + decode */
static const struct {
	unsigned int base;
	unsigned int extra;
} dist_table[30] = {
    {1,0},{2,0},{3,0},{4,0},{5,1},{7,1},{9,2},{13,2},
    {17,3},{25,3},{33,4},{49,4},{65,5},{97,5},{129,6},{193,6},
    {257,7},{385,7},{513,8},{769,8},{1025,9},{1537,9},{2049,10},{3073,10},
    {4097,11},{6145,11},{8193,12},{12289,12},{16385,13},{24577,13}
};

typedef struct huff_node {
	struct huff_node* left;
	struct huff_node* right;
	int symbol; /* -1 for internal */
	unsigned long freq;
} huff_node_t;

/**
 * Recursive function to fill codes and lens arrays with the encoding
 * information of every leaf node in the Huffman tree rooted at node.
 * Note that it is static and therefore an internal function to huff.c.
 * 
 * @param node The root of the Huffman tree
 * @param code The code to assign to the node
 * @param len The code length for the level
 * @param codes The array of codes to fill
 * @param lens The array of code lengths to fill
 */
static void build_codes(const huff_node_t* node, unsigned long code, int len, unsigned long* codes, unsigned char* lens) {
	if (node->symbol >= 0) {
		codes[node->symbol] = code;
		lens[node->symbol] = (unsigned char)len;
		return;
	}

	if (node->left)	build_codes(node->left, code << 1, len + 1, codes, lens);
	if (node->right) build_codes(node->right, (code << 1) | 1, len + 1, codes, lens);
}

/**
 * Recursive function to free the dynamically allocated
 * memory for Huffman nodes in the tree rooted at n.
 * Note that it is static and therefore an internal function to huff.c.
 * 
 * @param n The root of the Huffman tree, it and all of its descendants must have been dynamically allocated
 */
static void free_tree(huff_node_t* n) {
	if (!n) return;
	if (n->symbol < 0) {
		free_tree(n->left);
		free_tree(n->right);
	}
	free(n);
}

/**
 * Fills codes with canonical Huffman codes based on the code lengths in lens.
 * Per RFC 1951 section 3.2.2.
 * 
 * @param lens The array of code lengths to fill
 * @param codes The array of codes to fill
 * @param range The number of symbols
 */
static void canonical_codes(unsigned char* lens, unsigned long* codes, unsigned int range) {
	int count[MAX_CODE_LEN + 1] = {0};
	int first[MAX_CODE_LEN + 1];

	for (unsigned int i = 0; i < range; i++)
		if (lens[i] > 0 && lens[i] <= MAX_CODE_LEN) count[lens[i]]++;

	first[0] = 0;
	for (int i = 1; i <= MAX_CODE_LEN; i++)
		first[i] = (first[i - 1] + count[i - 1]) << 1;

	int next[MAX_CODE_LEN + 1];
	memcpy(next, first, sizeof(next));

	for (unsigned int i = 0; i < range; i++) {
		int l = lens[i];
		if (l > 0) codes[i] = next[l]++;
	}
}

/** 
 * Map a raw length (3-258) to its DEFLATE code index (0-28) and extra bits value 
 *
 * @param length The length code from LZ-compressed data
 * @param extra_val Will contain the number added to the base length value upon a successful call
 * @return Index of the length in the table, or 0 if it isn't in the table
*/
static int length_to_code(unsigned int length, unsigned int *extra_val) {
    for (int i = 28; i >= 0; i--) {
        if (length >= len_table[i].base) {
            *extra_val = length - len_table[i].base;
            return i; // code = 257 + i
        }
    }
    *extra_val = 0;
    return 0;
}

/** 
 * Map a raw distance (1-32768) to its DEFLATE code index (0-29) and extra bits value
 * @param distance The distance code directly from LZ-compressed data
 * @param extra_val Will be set to the amount added to the base distance value upon a successful call
 * @return Index of the distance code in the table or 0 if it doesn't exist
*/
static int distance_to_code(unsigned int distance, unsigned int *extra_val) {
    for (int i = 29; i >= 0; i--) {
        if (distance >= dist_table[i].base) {
            *extra_val = distance - dist_table[i].base;
            return i;
        }
    }
    *extra_val = 0;
    return 0;
}

/**
 * Build Huffman tree from frequencies using the existing queue (min-heap).
 * 
 * @param frequencies An array such that `frequencies[i]` is the number of times that byte value `i` appears in the data
 * @param num_symbols The size of frequencies, codes, and lens
 * @param codes `codes[i]` is be fileld with bitstrings that represent the encoding of `i` if byte value `i` appeared in the data by calling `build_codes`
 * @param lens `lens[i]` is to be filled with the number of bits in the encoding of `i` if byte value `i` appeared in the data by calling `build_codes`
 * @return The root of the constructed Huffman tree or `NULL` on error
*/
static huff_node_t* build_huffman_tree_from_freq(unsigned int *frequencies, int num_symbols, unsigned long *codes, unsigned char *lens) {
    if (!frequencies || !codes || !lens || num_symbols <= 0) return NULL;

    memset(codes, 0, num_symbols * sizeof(unsigned long));
    memset(lens, 0, num_symbols * sizeof(unsigned char));

    queue_clear();

    int count = 0;
    for (int i = 0; i < num_symbols; i++)
        if (frequencies[i] > 0) count++;

    if (count == 0) return NULL;

    if (count == 1) {
        // Single symbol: create tree with depth 1
        // Need a dummy second node so build_codes works
        huff_node_t *root = malloc(sizeof(huff_node_t));
        root->symbol = -1;
        root->freq = 0;

        huff_node_t *leaf = malloc(sizeof(huff_node_t));
        leaf->left = NULL;
        leaf->right = NULL;
        for (int i = 0; i < num_symbols; i++) {
            if (frequencies[i] > 0) {
                leaf->symbol = i;
                leaf->freq = frequencies[i];
                break;
            }
        }

        // Dummy node on right so build_codes doesn't crash
        huff_node_t *dummy = malloc(sizeof(huff_node_t));
        dummy->symbol = (leaf->symbol == 0) ? 1 : 0; // different symbol
        dummy->freq = 0;
        dummy->left = NULL;
        dummy->right = NULL;

        root->left = leaf;
        root->right = dummy;

        build_codes(root, 0, 0, codes, lens);
        // Only keep the real symbol's code
        lens[dummy->symbol] = 0;
        codes[dummy->symbol] = 0;
        return root;
    }

    for (int i = 0; i < num_symbols; i++) {
        if (frequencies[i] > 0) {
            huff_node_t *node = malloc(sizeof(huff_node_t));
            node->symbol = i;
            node->freq = frequencies[i];
            node->left = NULL;
            node->right = NULL;
            enqueue(node, frequencies[i]);
        }
    }

    while (queue_size() > 1) {
        huff_node_t *left = (huff_node_t*) dequeue();
        huff_node_t *right = (huff_node_t*) dequeue();

        huff_node_t *internal = malloc(sizeof(huff_node_t));
        internal->symbol = -1;
        internal->freq = left->freq + right->freq;
        internal->left = left;
        internal->right = right;

        enqueue(internal, internal->freq);
    }

    huff_node_t *root = (huff_node_t*)dequeue();
    build_codes(root, 0, 0, codes, lens);

    return root;
}

/* ================================================================
 * Huffman decoder helper: decode one symbol from canonical codes.
 *
 * Given code lengths and canonical codes for an alphabet, we build
 * a compact decoder that reads bits from the stream (MSB-first for
 * Huffman codes) and returns the decoded symbol.
 * ================================================================ */

typedef struct {
	int first_code[MAX_CODE_LEN + 1];   // First canonical code at each length
	int count[MAX_CODE_LEN + 1];        // Number of codes at each length
	int symbols[NUM_SYMS_AND_LENGTHS];  // Symbols sorted by (length, value)
	int sym_offset[MAX_CODE_LEN + 1];   // Offset into symbols[] for each length
} huff_decoder_t;

/**
 * Build a decoder from code lengths.
 * 
 * @param dec A Huffman decoder struct to store Huffman coding information for this block
 * @param lens An array storing encoding lengths: lens[i] = code length for symbol i (0 means symbol not present)
 * @param num_symbols The length of lens
*/
static void build_decoder(huff_decoder_t* dec, unsigned char* lens, int num_symbols) {
	memset(dec, 0, sizeof(*dec));

	for (int i = 0; i < num_symbols; i++)
		if (lens[i] > 0 && lens[i] <= MAX_CODE_LEN) dec->count[lens[i]]++;

	dec->first_code[0] = 0;
	for (int i = 1; i <= MAX_CODE_LEN; i++)
		dec->first_code[i] = (dec->first_code[i - 1] + dec->count[i - 1]) << 1;

	int offset = 0;
	for (int i = 1; i <= MAX_CODE_LEN; i++) {
		dec->sym_offset[i] = offset;
		offset += dec->count[i];
	}

	int next_offset[MAX_CODE_LEN + 1];
	memcpy(next_offset, dec->sym_offset, sizeof(next_offset));
	for (int i = 0; i < num_symbols; i++) {
		if (lens[i] > 0 && lens[i] <= MAX_CODE_LEN)
			dec->symbols[next_offset[lens[i]]++] = i;
	}
}

/**
 * Decode one Huffman symbol from the bitstream.
 * Reads bits one at a time (from LSB-packed bytes), accumulates MSB-first.
 * 
 * @param dec The Huffman decoder struct for this block
 * @param data The Huffman-encoded data
 * @param bits_read The total number of bits read in the block
 * @return The decoded symbol, or -1 on error.
 */
static int decode_symbol(huff_decoder_t* dec, const unsigned char* data, unsigned long* bits_read) {
	unsigned int code = 0;
	for (int len = 1; len <= MAX_CODE_LEN; len++) {
		unsigned int bit = 0;
		bit_reader(data + (*bits_read / 8), 1, bits_read, &bit, false);
		code = (code << 1) | bit;

		int first = dec->first_code[len];
		if (dec->count[len] > 0 && (int)code >= first && (int)code < first + dec->count[len])
			return dec->symbols[dec->sym_offset[len] + ((int)code - first)];
	}
	return -1;
}

/** ================================================================
 *  ENCODE: Fixed Huffman with LZ77 tokens (BTYPE=1)
 *  ================================================================ 
 * @param tokens: An array of tokens that contain a concise form of LZ77 data, rather literal or length-distance entries stored in raw data form 
 * @param num_tokens: number of entries in tokens
 * @param bits_written The total number of bits written to the output
 * @param out_len The size of the encoded data
 * @return Dynamically allocated array of Huffman-encoded data
 */
static unsigned char* encode_fixed_huffman_tokens(const lz_token_t* tokens, size_t num_tokens, unsigned long* bits_written, size_t* out_len) {
    size_t alloc = num_tokens * 4 + 16;
    unsigned char* out = calloc(alloc, 1);
    if (!out || !out_len) return NULL;
    *bits_written = 0;

    // Fixed Huffman code tables per RFC 1951 3.2.6
    struct { unsigned short code; unsigned char len; } fixed_codes[288];
    for (int i = 0; i <= 143; i++)   { fixed_codes[i].code = 0x30 + i;           fixed_codes[i].len = 8; }
    for (int i = 144; i <= 255; i++) { fixed_codes[i].code = 0x190 + (i - 144);  fixed_codes[i].len = 9; }
    for (int i = 256; i <= 279; i++) { fixed_codes[i].code = (i - 256);          fixed_codes[i].len = 7; }
    for (int i = 280; i <= 287; i++) { fixed_codes[i].code = 0xC0 + (i - 280);   fixed_codes[i].len = 8; }

    for (size_t i = 0; i < num_tokens; i++) {
        size_t byte_pos = *bits_written / 8;
        if (byte_pos + 8 >= alloc) {
            alloc *= 2;
            unsigned char* tmp = realloc(out, alloc);
            if (!tmp) { free(out); return NULL; }
            memset(tmp + byte_pos, 0, alloc - byte_pos);
            out = tmp;
        }

        if (tokens[i].is_literal) {
            unsigned int sym = tokens[i].literal;
            bit_writer(fixed_codes[sym].code, fixed_codes[sym].len, bits_written, out, true);
        } else {
            // Length
            unsigned int extra_val;
            int len_idx = length_to_code(tokens[i].length, &extra_val);
            unsigned int sym = 257 + len_idx;
            bit_writer(fixed_codes[sym].code, fixed_codes[sym].len, bits_written, out, true);
            if (len_table[len_idx].extra > 0)
                bit_writer(extra_val, len_table[len_idx].extra, bits_written, out, false);

            // Distance (fixed: 5-bit codes, MSB-first)
            unsigned int dist_extra;
            int dist_idx = distance_to_code(tokens[i].distance, &dist_extra);
            bit_writer((unsigned int)dist_idx, 5, bits_written, out, true);
            if (dist_table[dist_idx].extra > 0)
                bit_writer(dist_extra, dist_table[dist_idx].extra, bits_written, out, false);
        }
    }

    // Write end-of-block (symbol 256)
    bit_writer(fixed_codes[256].code, fixed_codes[256].len, bits_written, out, true);

    *out_len = (*bits_written + 7) / 8;
    return out;
}

/** ================================================================
 *  ENCODE: Dynamic Huffman with LZ77 tokens (BTYPE=2)
 *  ================================================================ 
 * @param tokens An array of tokens that contain a concise form of LZ77 data, rather literal or length-distance entries stored in raw data form 
 * @param num_tokens The number of entries inside tokens
 * @param bits_written The total number of bits written to the output
 * @param out_len The size of the encoded data
 * @return Dynamically allocated space containing some block headers (starting with `HLIT`) and encoded data, or `NULL` on error 
 */
static unsigned char* encode_dynamic_huffman_tokens(const lz_token_t* tokens, size_t num_tokens, unsigned long* bits_written, size_t* out_len) {
    if (!tokens || !out_len) return NULL;

    size_t alloc = num_tokens * 4 + 1024;
    unsigned char* out = calloc(alloc, 1);
    if (!out) return NULL;
    *bits_written = 0;

    // === PART 1: COUNT FREQUENCIES from tokens ===
    unsigned int lit_freq[NUM_SYMS_AND_LENGTHS] = {0};
    unsigned int d_freq[NUM_DISTANCES] = {0};

    for (size_t i = 0; i < num_tokens; i++) {
        if (tokens[i].is_literal) {
            lit_freq[tokens[i].literal]++;
        } else {
            unsigned int extra_val;
            int len_idx = length_to_code(tokens[i].length, &extra_val);
            lit_freq[257 + len_idx]++;
            unsigned int dist_extra;
            int dist_idx = distance_to_code(tokens[i].distance, &dist_extra);
            d_freq[dist_idx]++;
        }
    }
    lit_freq[256] = 1; // end-of-block

    // Ensure at least 1 distance code
    int has_dist = 0;
    for (int i = 0; i < NUM_DISTANCES; i++) {
        if (d_freq[i] > 0) has_dist = 1;
    }
    if (!has_dist) d_freq[0] = 1;

    // === PART 2: BUILD HUFFMAN CODES ===
    unsigned long lit_codes[NUM_SYMS_AND_LENGTHS] = {0};
    unsigned char lit_lens[NUM_SYMS_AND_LENGTHS] = {0};

    huff_node_t *lit_root = build_huffman_tree_from_freq(lit_freq, NUM_SYMS_AND_LENGTHS, lit_codes, lit_lens);
    if (!lit_root) { free(out); return NULL; }

    int valid = 1;
    for (int i = 0; i < NUM_SYMS_AND_LENGTHS; i++) {
        if (lit_lens[i] > MAX_CODE_LEN) valid = 0;
    }
    if (!valid) { free_tree(lit_root); free(out); return NULL; }
    canonical_codes(lit_lens, lit_codes, NUM_SYMS_AND_LENGTHS);

    unsigned char dist_lens[NUM_DISTANCES] = {0};
    unsigned long dist_codes[NUM_DISTANCES] = {0};
    huff_node_t *dist_root = build_huffman_tree_from_freq(d_freq, NUM_DISTANCES, dist_codes, dist_lens);
    if (!dist_root) {
        // Fall back: minimal distance tree
        dist_lens[0] = 1;
    } else {
        for (int i = 0; i < NUM_DISTANCES; i++) {
            if (dist_lens[i] > MAX_CODE_LEN) { free_tree(dist_root); free_tree(lit_root); free(out); return NULL; }
        }
    }
    canonical_codes(dist_lens, dist_codes, NUM_DISTANCES);

    // Find HLIT
    int max_lit_sym = 256;
    for (int j = 257; j < NUM_SYMS_AND_LENGTHS; j++) {
        if (lit_lens[j] > 0) { max_lit_sym = j; break; }
    }
    unsigned int hlit = max_lit_sym - 256;

    // Find HDIST
    int max_dist_sym = 0;
    for (int j = NUM_DISTANCES - 1; j >= 0; j--) {
        if (dist_lens[j] > 0) { max_dist_sym = j; break; }
    }
    unsigned int hdist = max_dist_sym; // HDIST = (# of distance codes) - 1

    // === PART 3: ENCODE CODE LENGTHS ===
    int num_lit = max_lit_sym + 1;
    int num_dist = hdist + 1;
    int total_cl = num_lit + num_dist;
    unsigned char combined_lens[NUM_SYMS_AND_LENGTHS + NUM_DISTANCES];
    for (int i = 0; i < num_lit; i++) combined_lens[i] = lit_lens[i];
    for (int i = 0; i < num_dist; i++) combined_lens[num_lit + i] = dist_lens[i];

    unsigned int cl_freq[NUM_CODE_LENGTH_CODES] = {0};
    for (int i = 0; i < total_cl; i++) {
        cl_freq[combined_lens[i]]++;
    }

    unsigned long cl_codes[NUM_CODE_LENGTH_CODES] = {0};
    unsigned char cl_lens[NUM_CODE_LENGTH_CODES] = {0};
    huff_node_t *cl_root = build_huffman_tree_from_freq(cl_freq, NUM_CODE_LENGTH_CODES, cl_codes, cl_lens);
    if (!cl_root) { free_tree(lit_root); if (dist_root) free_tree(dist_root); free(out); return NULL; }

    for (int i = 0; i < NUM_CODE_LENGTH_CODES; i++) {
        if (cl_lens[i] > 7) { free_tree(cl_root); free_tree(lit_root); if (dist_root) free_tree(dist_root); free(out); return NULL; }
    }

    canonical_codes(cl_lens, cl_codes, NUM_CODE_LENGTH_CODES);

    static const unsigned char cl_order[NUM_CODE_LENGTH_CODES] = {
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
    };
    int last_cl_idx = 3;
    for (int j = 18; j >= 3; j--) {
        if (cl_lens[cl_order[j]] != 0) { last_cl_idx = j; break; }
    }
    int hclen = last_cl_idx - 3;

    // === PART 4: WRITE HEADER ===
    bit_writer(hlit,  5, bits_written, out, false);
    bit_writer(hdist, 5, bits_written, out, false);
    bit_writer(hclen, 4, bits_written, out, false);

    for (int j = 0; j < hclen + 4; j++) {
        bit_writer(cl_lens[cl_order[j]], 3, bits_written, out, false);
    }

    for (int j = 0; j < total_cl; j++) {
        unsigned char len = combined_lens[j];
        bit_writer(cl_codes[len], cl_lens[len], bits_written, out, true);
    }

    // === PART 5: WRITE ENCODED DATA ===
    for (size_t i = 0; i < num_tokens; i++) {
        // Grow buffer if needed
        size_t byte_pos = *bits_written / 8;
        if (byte_pos + 8 >= alloc) {
            alloc *= 2;
            unsigned char* tmp = realloc(out, alloc);
            if (!tmp) { free(out); free_tree(lit_root); if (dist_root) free_tree(dist_root); free_tree(cl_root); return NULL; }
            memset(tmp + byte_pos, 0, alloc - byte_pos);
            out = tmp;
        }

        if (tokens[i].is_literal) {
            unsigned int sym = tokens[i].literal;
            bit_writer(lit_codes[sym], lit_lens[sym], bits_written, out, true);
        } else {
            unsigned int extra_val;
            int len_idx = length_to_code(tokens[i].length, &extra_val);
            unsigned int sym = 257 + len_idx;
            bit_writer(lit_codes[sym], lit_lens[sym], bits_written, out, true);
            if (len_table[len_idx].extra > 0)
                bit_writer(extra_val, len_table[len_idx].extra, bits_written, out, false);

            unsigned int dist_extra;
            int dist_idx = distance_to_code(tokens[i].distance, &dist_extra);
            bit_writer(dist_codes[dist_idx], dist_lens[dist_idx], bits_written, out, true);
            if (dist_table[dist_idx].extra > 0)
                bit_writer(dist_extra, dist_table[dist_idx].extra, bits_written, out, false);
        }
    }

    // Write end-of-block (symbol 256)
    bit_writer(lit_codes[256], lit_lens[256], bits_written, out, true);

    // Cleanup
    free_tree(lit_root);
    if (dist_root) free_tree(dist_root);
    free_tree(cl_root);

    *out_len = (*bits_written + 7) / 8;
    return out;
}

/** ================================================================
 * HUFFMAN ENCODE TOKENS: Top-level encoder for LZ77 token arrays
 * @param tokens An array of LZ objects (either a literal value or length-distance pair)
 * @param num_tokens Length of `tokens`
 * @param bits_written Total number of bits written
 * @param out_len To be set to the length of the returned encoded data
 * @param returned_btype To be set to the most efficient Huffman BTYPE for the data (01 - static or 10 - dynamic)
 * @return Dynamically allocated array of compressed block data
 */
unsigned char* huffman_encode_tokens(const lz_token_t* tokens, size_t num_tokens, unsigned long* bits_written, size_t* out_len, unsigned char *returned_btype) {
    if (!tokens || num_tokens == 0) {
        if (out_len) *out_len = 0;
        return NULL;
    }

    unsigned long bw_fixed = 0, bw_dynamic = 0;
    size_t size_fixed = 0, size_dynamic = 0;

    unsigned char *fixed_out = encode_fixed_huffman_tokens(tokens, num_tokens, &bw_fixed, &size_fixed);
    unsigned char *dynamic_out = encode_dynamic_huffman_tokens(tokens, num_tokens, &bw_dynamic, &size_dynamic);

    if (dynamic_out && (!fixed_out || size_dynamic <= size_fixed)) {
        free(fixed_out);
        *out_len = size_dynamic;
        *bits_written = bw_dynamic;
        *returned_btype = BT_DYNAMIC;
        return dynamic_out;
    } else {
        free(dynamic_out);
        *out_len = size_fixed;
        *bits_written = bw_fixed;
        *returned_btype = BT_STATIC;
        return fixed_out;
    }
}

/* ================================================================
 * HUFFMAN DECODE
 *
 * Decodes a Huffman-encoded bitstream back to raw bytes.
 * btype_val tells us how the stream is structured:
 *   BT_STATIC (1): fixed Huffman codes, data starts at bit 0
 *   BT_DYNAMIC (2): dynamic header + data
 *
 * Returns malloc'd buffer of decoded bytes, sets *out_len.
 * ================================================================ */
/**
 * @param data: Huffman data to be processed (possibly LZ77 as well)
 * @param enc_len: Length of encoded data
 * @param btype_val: The type of huffman encoding used to encode the data
 * @param returned_btype: The encoding of smaller size out of static and dynamic
 * @param history: Previously uncompressed data
 * @param history_len: Length of history
 * @return Decompressed data
 */
unsigned char* huffman_decode(const unsigned char* data, size_t enc_len, unsigned int btype_val, unsigned long *bits_read, size_t* out_len, const unsigned char* history, size_t history_len) {
	if (!data || enc_len == 0 || !out_len) return NULL;

	huff_decoder_t lit_dec;
	huff_decoder_t dist_dec;
	unsigned char lit_lens[NUM_SYMS_AND_LENGTHS] = {0};
	unsigned long lit_codes[NUM_SYMS_AND_LENGTHS] = {0};
	unsigned char d_lens[NUM_DISTANCES] = {0};
	unsigned long d_codes[NUM_DISTANCES] = {0};

	if (btype_val == BT_STATIC)
	{
		// Build fixed Huffman code lengths per RFC 1951 3.2.6
		for (int i = 0; i <= 143; i++)   lit_lens[i] = 8;
		for (int i = 144; i <= 255; i++) lit_lens[i] = 9;
		lit_lens[256] = 7; // EOB
		for (int i = 257; i <= 279; i++) lit_lens[i] = 7;
		for (int i = 280; i <= 287; i++) lit_lens[i] = 8;

		canonical_codes(lit_lens, lit_codes, NUM_SYMS_AND_LENGTHS);
		build_decoder(&lit_dec, lit_lens, NUM_SYMS_AND_LENGTHS);

		// Fixed distance codes: all 5-bit codes (0-29)
		for (int i = 0; i < NUM_DISTANCES; i++) d_lens[i] = 5;
		canonical_codes(d_lens, d_codes, NUM_DISTANCES);
		build_decoder(&dist_dec, d_lens, NUM_DISTANCES);
	}
	else if (btype_val == BT_DYNAMIC)
	{
		// Read dynamic Huffman header
		unsigned int hlit_val = 0, hdist_val = 0, hclen_val = 0;
		bit_reader(data + (*bits_read / 8), 5, bits_read, &hlit_val, false);
		bit_reader(data + (*bits_read / 8), 5, bits_read, &hdist_val, false);
		bit_reader(data + (*bits_read / 8), 4, bits_read, &hclen_val, false);

		int num_lit = hlit_val + 257;
		int num_dist = hdist_val + 1;
		int num_cl = hclen_val + 4;

		// Read code length code lengths (3 bits each, LSB first)
		static const unsigned char cl_order[NUM_CODE_LENGTH_CODES] = {
			16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
		};
		unsigned char cl_lens_arr[NUM_CODE_LENGTH_CODES] = {0};
		unsigned long cl_codes_arr[NUM_CODE_LENGTH_CODES] = {0};

		for (int i = 0; i < num_cl; i++) {
			unsigned int val = 0;
			bit_reader(data + (*bits_read / 8), 3, bits_read, &val, false);
			cl_lens_arr[cl_order[i]] = (unsigned char)val;
		}

		canonical_codes(cl_lens_arr, cl_codes_arr, NUM_CODE_LENGTH_CODES);

		// Build code length decoder
		huff_decoder_t cl_dec;
		build_decoder(&cl_dec, cl_lens_arr, NUM_CODE_LENGTH_CODES);

		// Decode lit/len + distance code lengths
		int total_codes = num_lit + num_dist;
		unsigned char all_lens[NUM_SYMS_AND_LENGTHS + NUM_DISTANCES] = {0};
		int idx = 0;

		while (idx < total_codes) {
			int sym = decode_symbol(&cl_dec, data, bits_read);
			if (sym < 0) {
				debug("huffman: decode: failed to decode code length symbol at idx %d", idx);
				return NULL;
			}

			if (sym <= 15) {
				all_lens[idx++] = (unsigned char)sym;
			}
			else if (sym == 16) {
				unsigned int extra = 0;
				bit_reader(data + (*bits_read / 8), 2, bits_read, &extra, false);
				int repeat = extra + 3;
				unsigned char prev = (idx > 0) ? all_lens[idx - 1] : 0;
				for (int r = 0; r < repeat && idx < total_codes; r++)
					all_lens[idx++] = prev;
			}
			else if (sym == 17) {
				unsigned int extra = 0;
				bit_reader(data + (*bits_read / 8), 3, bits_read, &extra, false);
				int repeat = extra + 3;
				for (int r = 0; r < repeat && idx < total_codes; r++)
					all_lens[idx++] = 0;
			}
			else if (sym == 18) {
				unsigned int extra = 0;
				bit_reader(data + (*bits_read / 8), 7, bits_read, &extra, false);
				int repeat = extra + 11;
				for (int r = 0; r < repeat && idx < total_codes; r++)
					all_lens[idx++] = 0;
			}
		}

		// Split into lit/len and distance code lengths
		memcpy(lit_lens, all_lens, num_lit);
		canonical_codes(lit_lens, lit_codes, NUM_SYMS_AND_LENGTHS);
		build_decoder(&lit_dec, lit_lens, NUM_SYMS_AND_LENGTHS);

		// Build distance decoder
		for (int i = 0; i < num_dist && i < NUM_DISTANCES; i++)
			d_lens[i] = all_lens[num_lit + i];
		canonical_codes(d_lens, d_codes, NUM_DISTANCES);
		build_decoder(&dist_dec, d_lens, NUM_DISTANCES);
	}
	else
	{
		debug("huffman: decode: unsupported btype %u", btype_val);
		return NULL;
	}

	// Decode data symbols - full DEFLATE: literals, lengths+distances, EOB
	// Prepend history so distance references across blocks work naturally.
	size_t cap = history_len + enc_len * 16;
	if (cap < 4096) cap = 4096;
	unsigned char* out = (unsigned char*)malloc(cap);
	if (!out) return NULL;
	size_t out_ix = history_len;
	if (history && history_len > 0)
		memcpy(out, history, history_len);

	for (;;) {
		int sym = decode_symbol(&lit_dec, data, bits_read);
		if (sym < 0) {
			debug("huffman: decode: failed to decode symbol at out_ix %zu", out_ix);
			free(out);
			return NULL;
		}

		if (sym == 256) {
			break; // End of block
		}
		else if (sym < 256) {
			// Literal byte
			if (out_ix >= cap) {
				cap *= 2;
				unsigned char* tmp = realloc(out, cap);
				if (!tmp) { free(out); return NULL; }
				out = tmp;
			}
			out[out_ix++] = (unsigned char)sym;
		}
		else if (sym <= 285) {
			int len_idx = sym - 257;
			unsigned int length = len_table[len_idx].base;
			if (len_table[len_idx].extra > 0) {
				unsigned int extra_val = 0;
				bit_reader(data + (*bits_read / 8), len_table[len_idx].extra, bits_read, &extra_val, false);
				*bits_read = *bits_read + 1;
				length += extra_val;
			}

			int dist_sym = decode_symbol(&dist_dec, data, bits_read);
			if (dist_sym < 0 || dist_sym >= 30) {
				debug("huffman: decode: invalid distance code %d", dist_sym);
				free(out);
				return NULL;
			}
			unsigned int distance = dist_table[dist_sym].base;
			if (dist_table[dist_sym].extra > 0) {
				unsigned int extra_val = 0;
				bit_reader(data + (*bits_read / 8), dist_table[dist_sym].extra, bits_read, &extra_val, false);
				*bits_read = *bits_read + 1;
				distance += extra_val;
			}

			while (out_ix + length > cap) {
				cap *= 2;
				unsigned char* tmp = realloc(out, cap);
				if (!tmp) { free(out); return NULL; }
				out = tmp;
			}
			for (unsigned int i = 0; i < length; i++)
				out[out_ix + i] = out[out_ix - distance + i];
			out_ix += length;
		}
		else {
			debug("huffman: decode: invalid symbol %d", sym);
			free(out);
			return NULL;
		}
	}

	// Strip the history prefix - return only the newly decoded data
	size_t new_len = out_ix - history_len;
	*out_len = new_len;
	if (new_len == 0) {
		free(out);
		return malloc(1); // return valid pointer for zero-length result
	}
	unsigned char* result = (unsigned char*)malloc(new_len);
	if (!result) { free(out); return NULL; }
	memcpy(result, out + history_len, new_len);
	free(out);
	return result;
}
