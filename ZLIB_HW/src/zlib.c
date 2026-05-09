#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "debug.h"
#include "our_zlib.h"
#include "huff.h"
#include "lz.h"
#include "utility.h"
#include "crc.h"

/**
 * Checks ID header of gzip member
 * File pointer should point to the start of a member
 * Gzip magic is ID1=0x1f, ID2=0x8b (RFC 1952)
 */
int check_id(FILE* file) {
	unsigned char id1 = 0, id2 = 0;
	if (fread(&id1, 1, 1, file) != 1) return 0;
	if (fread(&id2, 1, 1, file) != 1) return 0;
	return id1 == 0x1f && id2 == 0x8b;
}

/**
 * Dynamically allocates memory for a (null-terminated) string in a file
 * File pointer should point to the beginning of the string
 */
void get_string(FILE* file, char** dest) {
	char next_char = -1;
	int str_len = 0;
	// while previous byte is not null:
	while (next_char != 0) {
		// read byte
		if (fread(&next_char, sizeof(char), 1, file) != 1) return;
		// increment necessary length
		str_len++;
	}
	// dest will now point to a space for the string in memory
	*dest = (char*)malloc((size_t)str_len);
	// handle malloc error
	if (!*dest) return;
	// return to start of the string in the file
	if (fseek(file, -1 * (long)str_len, SEEK_CUR) != 0) return;
	// read string into allocated space
	fread(*dest, sizeof(char), str_len, file);
}

/**
 * Skips a null-terminated string in the file (e.g. FNAME or FCOMMENT).
 * File pointer ends just past the null byte.
 */
static void skip_string(FILE* file) {
	int c;
	while ((c = fgetc(file)) != EOF && c != 0)
		;
}

/**
 * Skips the gzip member header and leaves the file positioned at the first
 * byte of compressed data. Does not read the trailer.
 * Returns 0 on success, 1 on error.
 */
int skip_gz_header_to_compressed_data(FILE* file, gz_header_t* header) {
	if (!check_id(file)) return 1;
	fread(&header->cm,    sizeof(char), 1, file);
	fread(&header->flags, sizeof(char), 1, file);
	fread(&header->mtime, sizeof(unsigned int), 1, file);
	fread(&header->xflags, sizeof(char), 1, file);
	fread(&header->os,    sizeof(char), 1, file);

	if ((header->flags & F_EXTRA) != 0) {
		fread(&header->extra_len, sizeof(unsigned short), 1, file);
		if (header->extra_len > 0)
			fseek(file, (long)header->extra_len, SEEK_CUR);
	}
	if ((header->flags & F_NAME) != 0)
		skip_string(file);
	if ((header->flags & F_COMMENT) != 0)
		skip_string(file);
	if ((header->flags & F_HCRC) != 0)
		fread(&header->hcrc, sizeof(unsigned short), 1, file);
	return 0;
}

/**
 * Fills a struct with the metadata of a member.
 * File pointer should point to the start of the member.
 * File pointer ends up at the end of the member.
 * Presence of struct fields has to be used to
 * rewind to the start of the compressed data.
 */
int parse_member(FILE* file, gz_header_t* header) {
	if (!check_id(file)) return 1;
	fread(&header->cm,    sizeof(char), 1, file);
	fread(&header->flags, sizeof(char), 1, file);
	fread(&header->mtime, sizeof(unsigned int), 1, file);
	fread(&header->xflags, sizeof(char), 1, file);
	fread(&header->os,    sizeof(char), 1, file);

	if ((header->flags & F_EXTRA) != 0) {
		fread(&header->extra_len, sizeof(unsigned short), 1, file);
		if (header->extra_len > 0) {
			header->extra = malloc(header->extra_len);
			if (header->extra)
				fread(header->extra, sizeof(char), header->extra_len, file);
		}
	}
	if ((header->flags & F_NAME) != 0) {
		get_string(file, &header->name);
	}
	if ((header->flags & F_COMMENT) != 0) {
		get_string(file, &header->comment);
	}
	if ((header->flags & F_HCRC) != 0) {
		fread(&header->hcrc, sizeof(unsigned short), 1, file);
	}


	if (fseek(file, 0, SEEK_END) != 0) {
		debug("could not skip to end of file");
		return 1;
	}
	fread(&header->crc,       sizeof(unsigned int), 1, file);
	fread(&header->full_size, sizeof(unsigned int), 1, file);
	return 0;
}

/**
 * @param bytes The start of the compressed bytes
 * @param comp_len	The length of compressed data
 * @param dec_len	The expected length of decompressed data (NEVER USED)
 */
char* inflate(char* bytes, size_t comp_len) {
	// disregards dictionary
	unsigned int *bit_header = NULL;
	unsigned long bit_pointer;
	unsigned char* out_block;
	unsigned char* out_member = NULL;
	size_t len_out_member = 0;
	size_t out_len = 0;

	// It's possible to output a comp_len such that it doesn't go through the while loop, or pass data that doesn't get 
	// passed the if statement, initialized to cover cases
	
	for (;;)
	{
		bit_reader((unsigned char *)bytes + (bit_pointer / 8),3,&bit_pointer,bit_header,0);
		// Check btypes and bfinal
		unsigned int btype = (*bit_header >> 1) & BT_MASK;

		// Use macros for the btypes
		if(btype == BT_DYNAMIC || btype == BT_STATIC)
		{
			// huffman_decode handles full DEFLATE (literals + length-distance)
			// Pass previously decompressed data as history for cross-block distance refs
			out_block = huffman_decode((const unsigned char*)bytes, comp_len, btype, &bit_pointer, &out_len, out_member, len_out_member);
			if (!out_block) return (char *)out_member;
		}
		else if(btype == BT_NO_COMPRESSION)
		{
			// Proceed with no compression
			unsigned long byte_ix = (bit_pointer + 7) / 8; // Start at the next byte if this one is started
			long len = (unsigned char)bytes[byte_ix] | ((unsigned char)bytes[byte_ix+1] << 8);
			// Should confirm inverse len
			byte_ix +=2;
			long inverse_len = (unsigned char)bytes[byte_ix] | ((unsigned char)bytes[byte_ix+1] << 8);
			byte_ix +=2;
			bit_pointer = byte_ix *8; // Reassign
			if((len & 0xffff) == (~inverse_len & 0xffff))
			{
				out_block = malloc(len);
				memcpy(out_block,bytes+byte_ix,len);
				out_len = len;
			}
			else
			{
				return (char *)out_member; // return all that has been gathered so far
			}
		}
		else
		{
			// Error (BTYPE = binary 11)
			return (char *)out_member;
		}

		unsigned char *temporary = realloc(out_member,len_out_member + out_len);
		if (temporary == NULL)
		{
			return (char *)out_member;
		}
		out_member = temporary;
		memcpy(out_member+len_out_member,out_block,out_len);
		free(out_block);
		len_out_member += out_len;
		// Check if BFINAL was set and stop reading this member's data if so
		if(*bit_header & 0x01)
		{
			break;
		}
	}
	return (char *)out_member; // Should do logic on reaching end of block
	
}

// allocates maximum space for the member and sets the header
char* set_member_header(char* fname, int* fsize) {
	struct stat stat_buff;
	stat(fname, &stat_buff);
	size_t max_size = 10 + strlen(fname) + 5 + stat_buff.st_size + 8;
	char* buffer = calloc(1, max_size);
	if (buffer == NULL) {
		debug("malloc error");
		return NULL;
	}
	buffer[0] = ID >> 8;								// ID1
	buffer[1] = ID & 0xff;								// ID2
	buffer[2] = 8;										// CM = deflate (RFC 1952)
	buffer[3] = F_NAME;									// FLG
	buffer[4] = stat_buff.st_mtim.tv_sec		& 0xff;
	buffer[5] = stat_buff.st_mtim.tv_sec >> 8	& 0xff;
	buffer[6] = stat_buff.st_mtim.tv_sec >> 16	& 0xff;
	buffer[7] = stat_buff.st_mtim.tv_sec >> 24;			// MTIME
	buffer[8] = 0;										// XFL
	buffer[9] = 0;										// OS
	strcpy(buffer + 10, fname);							// FNAME
	*fsize = stat_buff.st_size;							// get file size
	return buffer + 10 + strlen(fname) + 1;
}

/**
 * Runs deflate algorithm:
 * LZ77 + Huffman-encode the input as DEFLATE blocks (max DEFLATE_BLOCK_SIZE
 * uncompressed bytes each), wrap in a single gzip member.
 * @param filename: Name of the file to be stored
 * @param bytes: Stream of data to be encoded
 * @param len: Length of (parameter) bytes
 * @param out_len: The length of data after decompression
 */
char* deflate(char* filename, char* bytes, size_t len, size_t* out_len) {
	if (out_len) *out_len = 0;

	int fsize = 0;
	int header_len = 10 + strlen(filename);
	char* out_member = set_member_header(filename, &fsize);
	if (!out_member) return NULL;
	char* real_start = out_member - header_len - 1;

	// Allocate generous output buffer for compressed blocks
	size_t gzip_hdr_len = (size_t)(out_member - real_start);
	size_t alloc = gzip_hdr_len + len + 1024;
	real_start = realloc(real_start, alloc);
	if (!real_start) return NULL;
	out_member = real_start + gzip_hdr_len;
	memset(out_member, 0, alloc - gzip_hdr_len);

	unsigned long total_bit_pos = 0; // bit position within out_member

	size_t offset = 0;
	while (offset < len) {
		size_t chunk = len - offset;
		if (chunk > DEFLATE_BLOCK_SIZE) chunk = DEFLATE_BLOCK_SIZE;
		int is_last = (offset + chunk >= len);

		// LZ77 compress this chunk
		size_t num_tokens = 0;
		lz_token_t* tokens = lz_compress_tokens((const unsigned char*)bytes + offset, chunk, &num_tokens);

		// Huffman encode the tokens
		unsigned long bits_written = 0;
		size_t huff_len = 0;
		unsigned char btype = 0;
		unsigned char* huff_buf = huffman_encode_tokens(tokens, num_tokens, &bits_written, &huff_len, &btype);
		free(tokens);
		if (!huff_buf) { free(real_start); return NULL; }

		// Ensure output buffer is large enough
		size_t needed = gzip_hdr_len + (total_bit_pos + 3 + bits_written + 7) / 8 + 8;
		if (needed > alloc) {
			alloc = needed * 2;
			real_start = realloc(real_start, alloc);
			if (!real_start) { free(huff_buf); return NULL; }
			out_member = real_start + gzip_hdr_len;
			// Zero only the new portion
			size_t cur_bytes = (total_bit_pos + 7) / 8;
			memset(out_member + cur_bytes, 0, alloc - gzip_hdr_len - cur_bytes);
		}

		// Write 3-bit block header: BFINAL (1 bit) + BTYPE (2 bits), LSB-first
		unsigned int header_val = (is_last ? 0 : 1) | ((unsigned int)btype << 1);
		bit_writer(header_val, 3, &total_bit_pos, (unsigned char*)out_member, false);

		// Merge Huffman-encoded data at current bit position
		unsigned long bit_offset = total_bit_pos / 8;
		unsigned long byte_pos = total_bit_pos % 8;
		for (size_t i = 0; i < huff_len; i++) {
			out_member[byte_pos + i]     |= (unsigned char)(huff_buf[i] << bit_offset);
			if (bit_offset > 0)
				out_member[byte_pos + i + 1] |= (unsigned char)(huff_buf[i] >> (8 - bit_offset));
		}
		total_bit_pos += bits_written;

		free(huff_buf);
		offset += chunk;
	}

	// Byte-align after all blocks
	size_t compressed_bytes = (total_bit_pos + 7) / 8;

	// Gzip trailer: CRC32 + ISIZE (over ALL original data)
	unsigned int checksum = get_crc((const unsigned char*)bytes, len);
	out_member[compressed_bytes]     = checksum        & 0xff;
	out_member[compressed_bytes + 1] = (checksum >> 8)  & 0xff;
	out_member[compressed_bytes + 2] = (checksum >> 16) & 0xff;
	out_member[compressed_bytes + 3] = checksum >> 24;
	out_member[compressed_bytes + 4] = fsize        & 0xff;
	out_member[compressed_bytes + 5] = (fsize >> 8)  & 0xff;
	out_member[compressed_bytes + 6] = (fsize >> 16) & 0xff;
	out_member[compressed_bytes + 7] = fsize >> 24;

	if (out_len) *out_len = gzip_hdr_len + compressed_bytes + 8;
	return real_start;
}