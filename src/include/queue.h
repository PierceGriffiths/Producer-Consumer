#ifndef QUEUE_H
#define QUEUE_H
#include <stddef.h>
struct Queue{
	long *const restrict array;
	const unsigned short capacity;
	unsigned short size, front, back;
};

struct Queue* createQueue(const unsigned short capacity);

void deleteQueue(struct Queue *restrict q);

unsigned short enqueue(struct Queue *const restrict q, const long num);

long dequeue(struct Queue *const restrict q, unsigned short *const index);

__attribute__((always_inline)) inline int isEmpty(const struct Queue *restrict q){
	return !q->size;
}

__attribute__((always_inline)) inline int isFull(const struct Queue *restrict q){
	return q->size == q->capacity;
}
#endif
