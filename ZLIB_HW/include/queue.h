#include <stddef.h>

void enqueue(void* node, unsigned int priority);
void* dequeue(void);
int queue_empty(void);
size_t queue_size(void);
void queue_clear(void);