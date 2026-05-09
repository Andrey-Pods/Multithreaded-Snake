
#define BITS_PER_BYTE 8
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
unsigned int bit_reader(const unsigned char* data, unsigned int bit_count_to_read,unsigned long * bits_pointer, unsigned int *littleEndianRet, bool isBigEndian);
void bit_writer(unsigned int value, unsigned int num_bits, unsigned long* bit_position, unsigned char *out, bool ismsbFirst);
void shift_left(char *data, size_t len, int shift);