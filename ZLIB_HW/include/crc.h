#ifndef PNG_CRC_H
#define PNG_CRC_H

#include <stdint.h>
#include <stddef.h>

/* Compute CRC over chunk type + data */
unsigned int get_crc(const unsigned char *buf, size_t len);

#endif
