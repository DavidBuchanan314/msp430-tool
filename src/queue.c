#include <stddef.h>
#include <stdlib.h>

#include "queue.h"

int queue_init(queue_t *q, size_t length)
{
	q->length = length;
	q->head = q->tail = 0;
	q->data = calloc(length, sizeof(QUEUE_ELEM_TYPE));
	return q->data != NULL;
}

void queue_free(queue_t *q)
{
	free(q->data);
}

int queue_empty(queue_t *q)
{
	return q->head == q->tail;
}

int queue_full(queue_t *q)
{
	return (q->head + 1) % q->length == q->tail;
}

int enqueue(queue_t *q, QUEUE_ELEM_TYPE value)
{
	if (queue_full(q)) return 0;
	q->data[q->head] = value;
	q->head = (q->head + 1) % q->length;
	return 1;
}

int dequeue(queue_t *q, QUEUE_ELEM_TYPE *value)
{
	if (queue_empty(q)) return 0;
	*value = q->data[q->tail];
	q->tail = (q->tail + 1) % q->length;
	return 1;
}
