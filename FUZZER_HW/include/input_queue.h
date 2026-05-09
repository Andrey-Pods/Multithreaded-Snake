/*
 * === DO NOT MODIFY THIS FILE ===
 * During testing, we will replace this file with our
 * own. You can create your own header files if necessary.
 * You have been warned. 
 * === DO NOT MODIFY THIS FILE ===
 */

#ifndef INPUT_QUEUE_H
#define INPUT_QUEUE_H

#include "input.h"

/*
 * The input queue is made up of two queues. One queue stores
 * high priority inputs and the second stores low priority
 * inputs
 */
typedef struct input_queue * INPUT_QUEUE;

/*
 * Initializes the input queue. This should acquire/allocate any
 * resource or memory necessary to create the input queue.
 *
 * @returns the handler to the newly initialized input queue.
 */
INPUT_QUEUE input_queue_init();

/*
 * Finalizes the input queue. This should release/deallocate all
 * resources and memory that the input queue is currently holding.
 *
 * @param queue     The input queue to finalize and free
 */
void input_queue_fini(INPUT_QUEUE queue);

/*
 * Enqueues a input as a high priority input into the input queue.
 *
 * @param queue     The input queue to insert the input into
 * @param input     The high priority input to insert
 */
void enqueue_high_prio_input(INPUT_QUEUE queue, INPUT input);

/*
 * Enqueues a input as a low priority input into the input queue.
 *
 * @param queue     The input queue to insert the input into
 * @param input     The low priority input to insert
 */
void enqueue_low_prio_input(INPUT_QUEUE queue, INPUT input);

/*
 * Dequeues an input from the input queue following a preset
 * pattern. When the input is dequeued, it is re-enqueued
 * at the end of the queue where it was dequeued. The reference 
 * to the dequeued input is returned to the user. 
 *
 * See the assignment document for more details!
 *
 * @param queue     The input queue to dequeue an input from
 */
INPUT dequeue_input(INPUT_QUEUE queue);

#endif