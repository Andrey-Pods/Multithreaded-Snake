#include "input_queue.h"

INPUT_QUEUE input_queue_init() {
    return 0;
}

void input_queue_fini(INPUT_QUEUE queue) {
    (void) queue;
}

void enqueue_high_prio_input(INPUT_QUEUE queue, INPUT input) {
    (void) queue;
    (void) input;
}

void enqueue_low_prio_input(INPUT_QUEUE queue, INPUT input) {
    (void) queue;
    (void) input;
}

INPUT dequeue_input(INPUT_QUEUE queue) {
    (void) queue;
    return 0;
}
