/**
 * === DO NOT MODIFY THIS FILE ===
 * This file is essential in setting up the coverage sanitizer.
 * Modifying this file may break an essential component of
 * the fuzzer. 
 * 
 * During grading, we will replace this file with our own.
 * You have been warned.
 * === DO NOT MODIFY THIS FILE ===
 */

#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <sanitizer/coverage_interface.h>
#include <sys/mman.h>

#define COVERAGE_MAP_FD 767
#define COVERAGE_MAP_SIZE 4096

static char *bitmap;

void __sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop) {
    if (start == stop || *start) return;

    // Check if there are too many branches for the bitmap
    // Q: Why the +1? 
    // A: Guard value 0 is reserved so we have one less bit in the coverage bitmap
    if (stop - start + 1 > COVERAGE_MAP_SIZE) {
        return;
    }

    // load bitmap if it exists
    if (fcntl(COVERAGE_MAP_FD, F_GETFD) == -1) {
        if (errno == EBADF) {
            return;
        }
    }
    bitmap = mmap(NULL, COVERAGE_MAP_SIZE / 8, PROT_READ | PROT_WRITE, MAP_SHARED, COVERAGE_MAP_FD, 0);
    if (!bitmap) {
        return;
    }

    // Assign an ID for each edge in the control flow graph
    static uint64_t N;
    for (uint32_t *x = start; x < stop; x++) {
        *x = ++N;
    }
}

void __sanitizer_cov_trace_pc_guard(uint32_t *guard) {
    if (!*guard) return; 
    uint32_t guard_index = *guard;
    // turn on the bit for the relevant guard index
    bitmap[guard_index / 8] |= 1 << (guard_index % 8);
}