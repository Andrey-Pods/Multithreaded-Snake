/*
 * === DO NOT MODIFY THIS FILE ===
 * During testing, we will replace this file with our
 * own. You can create your own header files if necessary.
 * You have been warned. 
 * === DO NOT MODIFY THIS FILE ===
 */

#ifndef INPUT_H
#define INPUT_H

#include <stdlib.h>
#include <stdint.h>

// The type representing the mutator state
typedef uint64_t MUTATOR_STATE;

/*
 * The type representing an input to be used in the fuzzer.
 * It holds the string for the input and the mutator state
 * which will be manipulator by the mutation engine.
 *
 * See the assignment document for more details!
 */
typedef struct input * INPUT;

/*
 * Creates an input object containing the input string.
 * The mutation state is initialized to zero.
 *
 * @param input_str The input string
 * @returns the created input object
 */
INPUT make_input(const char *input_str);

/*
 * Releases and free any resource or memory of an input.
 *
 * @param input     The input object to destroy
 */
void free_input(INPUT input);

/*
 * Retrieves the length of the input
 *
 * @param input     The input object
 * @returns the length of the input string
 */
size_t input_len(INPUT input);

/*
 * Retrieves the reference to the input string
 *
 * @param input     The input object
 * @returns a reference to the input string 
 */
const char *input_str(INPUT input);

/*
 * Retrieves the mutator state of the input object
 *
 * @param input     The input object
 * @returns the current mutation state of the input object
 */
MUTATOR_STATE input_mutator_state(INPUT input);

/*
 * Sets the mutator state of the input object
 *
 * @param input     The input object
 * @param state     The new mutator state
 * @returns the mutation state before setting to the new mutator
 *          state
 */
MUTATOR_STATE input_set_state(INPUT input, MUTATOR_STATE state);

/*
 * Increments the mutator state of the input object by one.
 *
 * @param input     The input object
 * @returns the mutation state before the increment
 */
MUTATOR_STATE input_state_step(INPUT input);

#endif