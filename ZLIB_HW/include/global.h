#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdio.h>
#include <stdlib.h>

/* Usage/Help Messages */
#define PRINT_USAGE(prog_name) do { \
    fprintf(stdout, "Usage: %s -i gz_file [options]\n", prog_name); \
    fprintf(stdout, "Options:\n"); \
    fprintf(stdout, "  -i gz_file            Input GZ file (required)\n"); \
    fprintf(stdout, "  -h                    Print this help message\n"); \
    fprintf(stdout, "  -m                    Print member summary\n"); \
    fprintf(stdout, "  -c                    Compress the file\n"); \
    fprintf(stdout, "  -d                    Decompress the file\n"); \
    fprintf(stdout, "  -o out_file           Output file for -c or -d (required for compress/decompress)\n"); \
} while(0)

/* Error Messages */
#define PRINT_ERROR_BAD_HEADER() fprintf(stderr, "Error: Missing .gz file ID\n")
#define PRINT_ERROR_OPEN_FILE(filename) fprintf(stderr, "Error: Failed to open file %s\n", filename)
#define PRINT_ERROR_MISSING_I_FLAG() fprintf(stderr, "Error: -i with input file is required\n")
#define PRINT_ERROR_REQUIRE_ONE_OF_MCD() fprintf(stderr, "Error: exactly one of -m, -c, or -d is required\n")
#define PRINT_ERROR_MISSING_O_FLAG() fprintf(stderr, "Error: -o with output file is required for -c and -d\n")

/* Member Summary (gzip) */
#define PRINT_MEMBER_SUMMARY_HEADER(filename) fprintf(stdout, "Member Summary for %s:\n", (filename))
#define PRINT_MEMBER_LINE(member_label, cm, mtime, os, extra, comment, size, crc_valid) do { \
    fprintf(stdout, "  Member %s: Compression Method: %u, Last Modified: %u, OS: %u, Extra: %u, ", (member_label), (unsigned)(cm), (unsigned)(mtime), (unsigned)(os), (unsigned)(extra)); \
    if ((comment) && (comment)[0]) fprintf(stdout, "Comment: %s, ", (comment)); \
    fprintf(stdout, "Size: %u, CRC: %s\n", (unsigned)(size), (crc_valid) ? "valid" : "invalid"); \
} while(0)


#endif /* GLOBAL_H */
