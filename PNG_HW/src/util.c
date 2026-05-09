#include "util.h"
#include <string.h>
#include <stdlib.h>
#include <zlib.h>

/* Big-endian helpers */
uint32_t read_u32_be(const uint8_t *buf)
{
    return ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8) |
           ((uint32_t)buf[3]);
}

/* Safe read helpers */
int read_exact(FILE *fp, uint8_t *buf, size_t len)
{
    size_t total_read = 0;
    while (total_read < len) {
        size_t n = fread(buf + total_read, 1, len - total_read, fp);
        if (n == 0) {
            if (feof(fp)) {
                return -1; /* EOF before reading all bytes */
            }
            return -1; /* Error */
        }
        total_read += n;
    }
    return 0;
}

/* Decompress data using zlib inflate */
/* Returns 0 on success, -1 on error. Caller must free *out_data. */
int util_inflate_data(const uint8_t *compressed, size_t compressed_size,
                      uint8_t **out_data, size_t *out_size)
{
    if (compressed == NULL || out_data == NULL || out_size == NULL) {
        return -1;
    }

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = compressed_size;
    strm.next_in = (uint8_t *)compressed;

    if (inflateInit(&strm) != Z_OK) {
        return -1;
    }

    size_t decompressed_capacity = 8192;
    size_t decompressed_size = 0;
    uint8_t *decompressed = (uint8_t *)malloc(decompressed_capacity);
    if (decompressed == NULL) {
        inflateEnd(&strm);
        return -1;
    }

    int ret;
    do {
        strm.avail_out = decompressed_capacity - decompressed_size;
        strm.next_out = decompressed + decompressed_size;
        ret = inflate(&strm, Z_NO_FLUSH);
        decompressed_size = decompressed_capacity - strm.avail_out;

        if (ret == Z_OK && strm.avail_out == 0) {
            decompressed_capacity *= 2;
            uint8_t *new_decompressed = (uint8_t *)realloc(decompressed, decompressed_capacity);
            if (new_decompressed == NULL) {
                free(decompressed);
                inflateEnd(&strm);
                return -1;
            }
            decompressed = new_decompressed;
            strm.next_out = new_decompressed + decompressed_size;
            strm.avail_out = decompressed_capacity - decompressed_size;
        }
    } while (ret == Z_OK);

    inflateEnd(&strm);

    if (ret != Z_STREAM_END) {
        free(decompressed);
        return -1;
    }

    *out_data = decompressed;
    *out_size = decompressed_size;
    return 0;
}

/* Compress data using zlib deflate with PNG-compatible settings */
/* PNG requires windowBits = 15 (32KB window) */
/* Returns 0 on success, -1 on error. Caller must free *out_data. */
int util_deflate_data_png(const uint8_t *data, size_t data_size,
                          uint8_t **out_data, size_t *out_size)
{
    if (data == NULL || out_data == NULL || out_size == NULL) {
        return -1;
    }

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return -1;
    }

    strm.avail_in = data_size;
    strm.next_in = (uint8_t *)data;

    size_t compressed_capacity = data_size;
    uint8_t *compressed = (uint8_t *)malloc(compressed_capacity);
    if (compressed == NULL) {
        deflateEnd(&strm);
        return -1;
    }

    strm.avail_out = compressed_capacity;
    strm.next_out = compressed;

    int ret = deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        free(compressed);
        deflateEnd(&strm);
        return -1;
    }

    size_t compressed_size = compressed_capacity - strm.avail_out;
    deflateEnd(&strm);

    *out_data = compressed;
    *out_size = compressed_size;
    return 0;
}
