#ifndef PNG_OVERLAY_H
#define PNG_OVERLAY_H

#include <stdint.h>

/* Overlay a smaller image onto a larger one starting at (x_offset, y_offset),
 * replacing the larger image's pixels wherever the smaller image lies. */
int png_overlay_paste(const char *large_path, const char *small_path,
                      const char *output_path, uint32_t x_offset, uint32_t y_offset);

#endif

