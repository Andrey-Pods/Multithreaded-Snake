/*
 * DO NOT MODIFY THIS FILE
 * Debug utilities for CSE 320
 */
#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#ifdef DEBUG
#define debug(fmt, ...) \
    fprintf(stderr, "[DEBUG] %s:%d:%s(): " fmt "\n", \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define debug(fmt, ...) ((void)0)
#endif

#endif /* DEBUG_H */
