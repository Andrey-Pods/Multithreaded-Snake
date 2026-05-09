#include <stdlib.h>
#include <string.h>
#include "queue.h"
#include "debug.h"

#define QUEUE_CAP 512

typedef struct {
	void* node;
	unsigned int priority;
} item_t;

static item_t heap[QUEUE_CAP];
static size_t heap_size;

/**
 * Enqueues arbitrary pointer node with priority
 */
void enqueue(void* node, unsigned int priority) {
	if (heap_size > QUEUE_CAP) {
		debug("queue has reached max size, exiting...");
		return;
	}
	size_t i = heap_size++;
	// insert node at the end of the array
	heap[i].node = node;
	heap[i].priority = priority;
	while (i > 0) {
		// start at highest priority node? and recursively examine parent
		size_t parent = (i - 1) / 2;
		// node's priority must be greater than parent's priority:
		// if parent's priority is less than or equal to the node's priority, we have gone too far
		if (heap[parent].priority <= heap[i].priority) break;
		// swap parent with node to insert
		item_t tmp = heap[parent];
		heap[parent] = heap[i];
		heap[i] = tmp;
		// go up one level
		i = parent;
	}
}

/**
 * 
 */
void* dequeue() {
	if (heap_size == 0) return NULL;
	// take out root node
	void* out = heap[0].node;
	// replace root node with last (rightmost) leaf node
	// decrease node count
	heap[0] = heap[--heap_size];
	size_t i = 0;
	// while correct subtree doesn't change:
	for (;;) {
		// calculate left and right nodes
		size_t left = 2 * i + 1;
		size_t right = 2 * i + 2;
		size_t smallest = i;
		// move to left subtree if its priority is smaller
		if (left < heap_size && heap[left].priority < heap[smallest].priority)
			smallest = left;
		// move to right subtree if its priority is smaller
		if (right < heap_size && heap[right].priority < heap[smallest].priority)
			smallest = right;
		if (smallest == i) break;
		// swap parent node with child that has the least priority
		item_t tmp = heap[i];
		heap[i] = heap[smallest];
		heap[smallest] = tmp;
		i = smallest;
	}
	return out;
}

int queue_empty(void) {
	return heap_size == 0;
}

size_t queue_size(void) {
	return heap_size;
}

void queue_clear(void) {
	heap_size = 0;
}
