/*
 * === DO NOT MODIFY THIS FILE ===
 * During testing, we will replace this file with our
 * own. You can create your own header files if necessary.
 * You have been warned. 
 * === DO NOT MODIFY THIS FILE ===
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdio.h>
#include <stdint.h>

#define DEFAULT_RUNNER_COUNT 4
#define DEFAULT_INPUT_TOTAL 32
#define DEFAULT_TIMEOUT_SEC 5

#define COVERAGE_MAP_FD 767
#define COVERAGE_MAP_SIZE 4096

#define PROGRAM_ARGUMENT_PLACEHOLDER "@@"

/* Usage/Help Messages */
#define PRINT_USAGE(file, prog_name) do { \
    fprintf(file, "Usage: %s [options] PROGRAM ARGS...\n", prog_name); \
    fprintf(file, "Options:\n"); \
    fprintf(file, "  -h                    Print this help message\n"); \
    fprintf(file, "  -j jobs               Number of jobs used for fuzzing. Default is %d\n", DEFAULT_RUNNER_COUNT); \
    fprintf(file, "  -n inputs             Total number of inputs to attempt. Default is %d\n", DEFAULT_INPUT_TOTAL); \
    fprintf(file, "  -s seed_file          Input file containing newline-separated initial inputs to the fuzzer. REQUIRED\n"); \
    fprintf(file, "  -t time_limit         Set the time limit for the target program in seconds. Default is %d\n", DEFAULT_TIMEOUT_SEC); \
} while(0)

/*
 * Potentially useful global variables
 */
extern char *cmd;
extern char **args;
extern size_t program_argc;
extern int timeout;

/*
 * The hash function which you will primarily be using throughout this
 * program
 */
uint64_t hash(uint64_t n);
#define HASH(n) hash((n))

#endif