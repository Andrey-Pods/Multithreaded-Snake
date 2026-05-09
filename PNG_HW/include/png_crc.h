#ifndef PNG_CRC_H
#define PNG_CRC_H

#include <stdint.h>
#include <stddef.h>

/* Compute CRC over chunk type + data */
uint32_t png_crc(const uint8_t *buf, size_t len);

#endif
