#include <criterion/criterion.h>
#include <criterion/internal/assert.h>

#include "input_queue.h"

TestSuite(input_queue_tests, .timeout = 1);

Test(input_queue_tests, input_queue_enqueues_0) {
    INPUT_QUEUE queue = input_queue_init();
    cr_assert_not_null(queue, "Expected queue to be non-null");

    INPUT hi_prio_inputs[] = {
        make_input("a"),
        make_input("b"),
        make_input("c"),
        make_input("d"),
        make_input("e"),
        make_input("f"),
        make_input("g"),
        make_input("h"),
        make_input("i"),
        make_input("j"),
        make_input("k"),
    };
    size_t hi_prio_inputs_sz = sizeof(hi_prio_inputs) / sizeof(hi_prio_inputs[0]);
    for (size_t i = 0; i < hi_prio_inputs_sz; ++i)
        enqueue_high_prio_input(queue, hi_prio_inputs[i]);

    INPUT lo_prio_inputs[] = {
        make_input("l"),
        make_input("m"),
        make_input("o")
    };
    size_t lo_prio_inputs_sz = sizeof(lo_prio_inputs) / sizeof(lo_prio_inputs[0]);
    for (size_t i = 0; i < lo_prio_inputs_sz; ++i)
        enqueue_low_prio_input(queue, lo_prio_inputs[i]);

    size_t inputs_sz = hi_prio_inputs_sz + lo_prio_inputs_sz;

    size_t hi_prio_count = 0;
    size_t lo_prio_count = 0;
    for (size_t i = 0; i < inputs_sz; ++i) {
        INPUT expected_input_0 = hi_prio_inputs[hi_prio_count % hi_prio_inputs_sz];
        INPUT expected_input_1 = lo_prio_inputs[lo_prio_count % lo_prio_inputs_sz];
        INPUT actual_input = dequeue_input(queue);

        cr_assert(expected_input_0 == actual_input || expected_input_1 == actual_input,
                "Expected input = %p or %p, Actual input = %p",
                expected_input_0, expected_input_1, actual_input);
        if (actual_input == expected_input_0) ++hi_prio_count;
        if (actual_input == expected_input_1) ++lo_prio_count;
    }
}