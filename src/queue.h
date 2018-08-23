#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>
#define QUEUE_ELEM_TYPE uint16_t

typedef struct Queue
{
	size_t length;
	size_t head;
	size_t tail;
	QUEUE_ELEM_TYPE *data;
} queue_t;

int queue_init(queue_t *q, size_t length);
void queue_free(queue_t *q);
int queue_empty(queue_t *q);
int queue_full(queue_t *q);
int enqueue(queue_t *q, QUEUE_ELEM_TYPE value);
int dequeue(queue_t *q, QUEUE_ELEM_TYPE *value);

#endif /* QUEUE_H */
