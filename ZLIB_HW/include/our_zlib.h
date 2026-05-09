#include <stddef.h>
#include <stdio.h>

// program modes
#define M_DEFLATE	0
#define M_INFLATE	1
#define M_INFO		2

// compressed block header
#define BF_SET				1
#define BT_STATIC			1
#define BT_DYNAMIC			2
#define BT_NO_COMPRESSION	0
#define BT_MASK				3
#define DEFLATE_BLOCK_SIZE 65535

// compression levels
#define L_NO_COMPRESSION         0
#define L_BEST_SPEED             1
#define L_BEST_COMPRESSION       9
#define L_DEFAULT_COMPRESSION  (-1)

// compression strategy
#define S_FILTERED            1
#define S_HUFFMAN_ONLY        2
#define S_RLE                 3 // run-length encoding
#define S_FIXED               4
#define S_DEFAULT_STRATEGY    0

// flags
#define F_TEXT		1
#define F_HCRC		2
#define F_EXTRA		4
#define F_NAME		8
#define F_COMMENT	16

#define FH_LIT1LEN		8
#define FH_LIT1START	0b00110000	// 0
#define FH_LIT1END		0b10111111	// 143
#define FH_LIT2LEN		9
#define FH_LIT2START	0b110010000 // 144
#define FH_LIT2END		0b111111111 // 255
#define FH_EXT1LEN		7
#define FH_EXT1START	0b0000000	// 256
#define FH_EXT1END		0b0010111	// 279
#define FH_EXT2LEN		8
#define FH_EXT2START	0b11000000	// 280
#define FH_EXT2END		0b11000111	// 287

typedef struct {
	unsigned char	cm;			// compression method
	unsigned char	flags;
    unsigned int	mtime;		// modification time
    unsigned char	xflags;		// extra flags
    unsigned char	os;			// operating system
    unsigned short	extra_len;	// extra field length							(optional)
    char*			extra;		// pointer to extra field or NULL				(optional)
    char*			name;		// pointer to zero-terminated file name or NULL (optional)
    char*			comment;	// pointer to zero-terminated comment or NULL	(optional)
    unsigned short	hcrc;		// header CRC, 16 bits							(optional)
	unsigned int	crc;		// 32-bit CRC for the data
	unsigned int	full_size;	// uncompressed size
} gz_header_t;

#define ID 0x1f8b
int parse_member(FILE* file, gz_header_t* header);
int skip_gz_header_to_compressed_data(FILE* file, gz_header_t* header);
/* Inflate: decompress. bytes = compressed data, comp_len = its length. Returns malloc'd decompressed buffer, length in header->full_size or first 4 bytes of format. */
char* inflate(char* bytes, size_t comp_len);
/* Deflate: compress. bytes = input, len = input length. Returns malloc'd compressed buffer. */
char* deflate(char* filename, char* bytes, size_t len, size_t* out_len);