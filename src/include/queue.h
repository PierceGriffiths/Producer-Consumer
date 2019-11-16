#ifndef QUEUE_H
#define QUEUE_H
#include <inttypes.h>
struct Queue{
	int *restrict array;
	uint_fast16_t capacity, size, front, back;
};

struct Queue* createQueue(const uint_fast16_t capacity);

uint_fast16_t enqueue(struct Queue *const q, const int num);

int dequeue(struct Queue *const restrict q, uint_fast16_t *const restrict index);

__attribute__((always_inline)) inline int isEmpty(const struct Queue *q){
	return !q->size;
}

__attribute__((always_inline)) inline int isFull(const struct Queue *q){
	return q->size == q->capacity;
}
#endif
