#include "utility.h"
// * keep in mind bits are not always packed in MSB to LSB order
// DEFLATE (RFC 1951) bit packing:
//   - Bits within bytes are packed LSB first (bit 0 of byte = first bit in stream)
//   - Data elements (extra bits, BTYPE, lengths) are stored LSB-first in the value
//   - Huffman codes are stored MSB-first (most significant bit of code is first in stream)
/**
 * @param data: Positioned at data[0] = byte containing the first bit to read.
 *              Callers pass data + (*bits_pointer / 8).
 * @param bit_count_to_read: Number of bits to read
 * @param bits_pointer: Total bit index into the stream (updated after reading)
 * @param lsbRet: Output value assembled from the bits read
 * @param ismsbFirst: If true, first bit read becomes MSB of result (Huffman codes).
 *                    If false, first bit read becomes LSB of result (data elements).
 * @return Number of byte boundaries crossed during the read
 */
unsigned int bit_reader(const unsigned char* data, unsigned int bit_count_to_read, unsigned long * bits_pointer, unsigned int *lsbRet, bool ismsbFirst)
{
	unsigned int result = 0;
	unsigned int start_bit = *bits_pointer % 8;
	unsigned long start_byte_abs = *bits_pointer / 8;

	for (unsigned int i = 0; i < bit_count_to_read; i++)
	{
		unsigned int cur_bit = start_bit + i;
		unsigned int byte_idx = cur_bit / 8;
		unsigned int bit_offset = cur_bit % 8;

		// Extract bit from stream (DEFLATE packs LSB first within each byte)
		unsigned int bit = (data[byte_idx] >> bit_offset) & 1;

		if (ismsbFirst)
		{
			// MSB first (Huffman codes): first bit read = most significant
			result |= bit << (bit_count_to_read - 1 - i);
		}
		else
		{
			// LSB first (data elements): first bit read = least significant
			result |= bit << i;
		}
	}

	*lsbRet = result;
	*bits_pointer += bit_count_to_read;

	return (unsigned int)(*bits_pointer / 8 - start_byte_abs);
}

/**
 * Write bits into output buffer.
 * @param value: The value to write
 * @param num_bits: Number of bits to write from value
 * @param bit_position: Total bit index into the output stream (updated after writing)
 * @param out: Output buffer (must be zero-initialized / calloc'd)
 * @param ismsbFirst: If true, MSB of value is written first (Huffman codes).
 *                    If false, LSB of value is written first (data elements).
 */
void bit_writer(unsigned int value, unsigned int num_bits, unsigned long* bit_position, unsigned char *out, bool ismsbFirst)
{
	for (unsigned int i = 0; i < num_bits; i++)
	{
		unsigned int byte_idx = *bit_position / 8;
		unsigned int bit_offset = *bit_position % 8;

		unsigned int bit;
		if (ismsbFirst)
		{
			// MSB first (Huffman codes): write most significant bit first
			bit = (value >> (num_bits - 1 - i)) & 1;
		}
		else
		{
			// LSB first (data elements): write least significant bit first
			bit = (value >> i) & 1;
		}

		if (bit) {
			out[byte_idx] |= (1 << bit_offset);
		}
		(*bit_position)++;
	}
}

// Left shift a byte array by `shift` bits
void shift_left(char *data, size_t len, int shift) 
{
    int byte_shift = shift / 8;
    int bit_shift  = shift % 8;

    for (size_t i = 0; i < len; i++) 
	{
        size_t src = i + byte_shift;
        if (src < len) 
		{
            data[i] = data[src] << bit_shift;
            if (bit_shift > 0 && src + 1 < len)
                data[i] |= data[src + 1] >> (8 - bit_shift);
        } else 
		{
            data[i] = 0;
        }
    }
}