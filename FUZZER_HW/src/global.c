/*
 * === DO NOT MODIFY THIS FILE ===
 * During testing, we will replace this file with our
 * own. You can create your own header files if necessary.
 * You have been warned. 
 * === DO NOT MODIFY THIS FILE ===
 */

#include "global.h"

char *cmd = NULL;
char **args = NULL;
size_t program_argc = 0;
int timeout = 0;

uint64_t hash(uint64_t n) {
    n = (n ^ (n >> 30)) * 0xbf58476d1ce4e5b9ull;
    n = (n ^ (n >> 27)) * 0x94d049bb133111ebull;
    n = n ^ (n >> 31);
    return n;
}