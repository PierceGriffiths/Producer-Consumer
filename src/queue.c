#include "queue.h"
#include <stdlib.h>

struct Queue* createQueue(const uint_fast16_t capacity){
	struct Queue *q = malloc(sizeof *q);
	if(!q)
		return NULL;

	q->array = malloc(capacity * sizeof *q->array);
	if(!q->array){
		free(q);
		return NULL;
	}
	*q = (struct Queue){q->array, capacity, 0, 0, capacity - 1};
	return q;
}

uint_fast16_t enqueue(struct Queue *const restrict q, const int num){
	q->back = (q->back + 1) % q->capacity;
	q->array[q->back] = num;
	++q->size;
	return q->back;
}

int dequeue(struct Queue *const restrict q, uint_fast16_t *const restrict index){
	*index = q->front;
	q->front = (q->front + 1) % q->capacity;
	--q->size;
	return q->array[*index];
}
