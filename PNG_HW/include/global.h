#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdio.h>
#include <stdlib.h>

/* Usage/Help Messages */
#define PRINT_USAGE(prog_name) do { \
    fprintf(stdout, "Usage: %s -f png_file [options]\n", prog_name); \
    fprintf(stdout, "Options:\n"); \
    fprintf(stdout, "  -f png_file           Input PNG file (required)\n"); \
    fprintf(stdout, "  -h                    Print this help message\n"); \
    fprintf(stdout, "  -s                    Print chunk summary\n"); \
    fprintf(stdout, "  -p                    Print palette summary\n"); \
    fprintf(stdout, "  -i                    Print IHDR fields\n"); \
    fprintf(stdout, "  -e message -o out_file    Encode message and write to output file\n"); \
    fprintf(stdout, "  -d                    Decode and print hidden message\n"); \
    fprintf(stdout, "  -m file2 -o out_file [-w width] [-g height]  Overlay file2 (smaller) over input and write to output\n"); \
} while(0)

/* Error Messages */
#define PRINT_ERROR_OPEN_FILE(filename) fprintf(stderr, "Error: Failed to open file %s\n", filename)
#define PRINT_ERROR_READ_CHUNKS() fprintf(stderr, "Error: Failed to read chunks\n")
#define PRINT_ERROR_PLTE_NOT_FOUND() fprintf(stderr, "Error: No PLTE chunk found or failed to parse\n")
#define PRINT_ERROR_READ_IHDR() fprintf(stderr, "Error: Failed to read IHDR chunk\n")
#define PRINT_ERROR_PARSE_IHDR() fprintf(stderr, "Error: Failed to parse IHDR data\n")
#define PRINT_ERROR_ENCODE_REQUIRES() fprintf(stderr, "Error: -e requires <message> -o <output_file>\n")
#define PRINT_ERROR_ENCODE_FAILED() fprintf(stderr, "Error: Failed to encode message\n")
#define PRINT_ERROR_EXTRACT_FAILED() fprintf(stderr, "Error: Failed to extract message\n")
#define PRINT_ERROR_OVERLAY_REQUIRES() fprintf(stderr, "Error: -m requires <file2> -o <output_file>\n")
#define PRINT_ERROR_WIDTH_REQUIRES() fprintf(stderr, "Error: -w requires a width value\n")
#define PRINT_ERROR_HEIGHT_REQUIRES() fprintf(stderr, "Error: -g requires a height value\n")
#define PRINT_ERROR_OVERLAY_FAILED() fprintf(stderr, "Error: Failed to overlay images\n")
#define PRINT_ERROR_UNKNOWN_OPTION(option) fprintf(stderr, "Error: Unknown option %s\n", option)
#define PRINT_ERROR_MISSING_F_FLAG() fprintf(stderr, "Error: -f flag with filename is required\n")
#define PRINT_ERROR_F_REQUIRES_FILENAME() fprintf(stderr, "Error: -f requires a filename\n")

/* Chunk Summary Messages */
#define PRINT_CHUNK_SUMMARY_HEADER(filename) printf("Chunk Summary for %s:\n", filename)
#define PRINT_CHUNK_INFO(i, chunk) printf("  Chunk %d: Type=%s, Length=%u, CRC=%s\n", i, (chunk).type, (chunk).length, (chunk).crc ? "valid" : "invalid")

/* Palette Messages */
#define PRINT_PALETTE_HEADER(filename) printf("Palette Summary for %s:\n", filename)
#define PRINT_PALETTE_COUNT(count) printf("  Number of colors: %zu\n", count)
#define PRINT_PALETTE_COLOR(i, r, g, b) printf("  Color %zu: RGB(%u, %u, %u)\n", i, r, g, b)

/* IHDR Messages */
#define PRINT_IHDR(filename, ihdr) do { \
    printf("IHDR Fields for %s:\n", filename); \
    printf("  Width: %u\n", (ihdr).width); \
    printf("  Height: %u\n", (ihdr).height); \
    printf("  Bit Depth: %u\n", (ihdr).bit_depth); \
    printf("  Color Type: %u", (ihdr).color_type); \
    switch ((ihdr).color_type) { \
        case 0: printf(" (Grayscale)\n"); break; \
        case 2: printf(" (RGB)\n"); break; \
        case 3: printf(" (Palette)\n"); break; \
        case 4: printf(" (Grayscale with Alpha)\n"); break; \
        case 6: printf(" (RGB with Alpha)\n"); break; \
        default: printf("\n"); break; \
    } \
    printf("  Compression: %u\n", (ihdr).compression); \
    printf("  Filter: %u\n", (ihdr).filter); \
    printf("  Interlace: %u", (ihdr).interlace); \
    if ((ihdr).interlace == 0) { \
        printf(" (None)\n"); \
    } else if ((ihdr).interlace == 1) { \
        printf(" (Adam7)\n"); \
    } else { \
        printf("\n"); \
    } \
} while(0)

/* Success Messages */
#define PRINT_ENCODE_SUCCESS(output_file) printf("Message encoded successfully to %s\n", output_file)
#define PRINT_HIDDEN_MESSAGE(buffer) printf("Hidden message: %s\n", buffer)
#define PRINT_OVERLAY_SUCCESS(output_file) printf("Overlay completed successfully to %s\n", output_file)

#endif /* GLOBAL_H */
