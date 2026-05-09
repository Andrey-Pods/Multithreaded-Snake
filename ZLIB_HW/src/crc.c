#include "crc.h"

/* PNG uses CRC-32 with polynomial 0xEDB88320 */
static unsigned int crc_table[256];
static int crc_table_computed = 0;

static void make_crc_table(void)
{
    unsigned int c;
    int n, k;
    unsigned int poly = 0xEDB88320UL;

    for (n = 0; n < 256; n++) {
        c = (unsigned int)n;
        for (k = 0; k < 8; k++) {
            if (c & 1) {
                c = poly ^ (c >> 1);
            } else {
                c = c >> 1;
            }
        }
        crc_table[n] = c;
    }
    crc_table_computed = 1;
}

unsigned int get_crc(const unsigned char *buf, size_t len)
{
    unsigned int c = 0xFFFFFFFFUL;
    size_t n;

    if (!crc_table_computed) {
        make_crc_table();
    }

    for (n = 0; n < len; n++) {
        c = crc_table[(c ^ buf[n]) & 0xFF] ^ (c >> 8);
    }

    return c ^ 0xFFFFFFFFUL;
}

