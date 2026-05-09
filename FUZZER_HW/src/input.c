#include "input.h"

INPUT make_input(const char *input_str) {
    (void) input_str;
    return 0;
}

void free_input(INPUT input) {
    (void) input;
}

size_t input_len(INPUT input) {
    (void) input;
    return 0;
}

const char *input_str(INPUT input) {
    (void) input;
    return 0;
}

MUTATOR_STATE input_mutator_state(INPUT input) {
    (void) input;
    return 0;
}

MUTATOR_STATE input_set_state(INPUT input, MUTATOR_STATE state) {
    (void) input;
    (void) state;
    return 0;
}

MUTATOR_STATE input_state_step(INPUT input) {
    (void) input;
    return 0;
}
