/*
 * === DO NOT MODIFY THIS FILE ===
 * During testing, we will replace this file with our
 * own. You can create your own header files if necessary.
 * You have been warned. 
 * === DO NOT MODIFY THIS FILE ===
 */

#ifndef MUTATOR_H
#define MUTATOR_H

#include "input.h"

/*
 * Mutates the given input to create a new modified input.
 * The specific algorithm for the mutation is specified in the
 * assignment document. Refer to that as well as `Mutation.md`.
 * 
 * @param input The input to mutate
 * @returns The newly created mutated input
 */
INPUT mutate(INPUT input);

#endif