#ifndef QUEUE_H
#define QUEUE_H
typedef struct{
    unsigned front, back, size, capacity, * restrict array;
}Queue;

Queue* createQueue(unsigned const capacity);

void* deleteQueue(Queue * restrict q);

unsigned enqueue(Queue * restrict q, unsigned const num);

unsigned dequeue(Queue *restrict q, unsigned *restrict num);

__attribute__((always_inline)) inline int isEmpty(const Queue * restrict q){
    return (q->size == 0);
}

__attribute__((always_inline)) inline int isFull(const Queue * restrict q){
    return q->size == q->capacity;
}
#endif
