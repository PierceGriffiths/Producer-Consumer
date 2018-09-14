#ifndef QUEUE_H
#define QUEUE_H
typedef struct{
    unsigned front, back, size, capacity, * restrict array;
}Queue;

Queue* createQueue(unsigned const capacity);

void* deleteQueue(Queue *q);

inline int isEmpty(const Queue *q){
    return (q->size == 0);
}

inline int isFull(const Queue *q){
    return (q->size == q->capacity);
}

unsigned enqueue(Queue *q, unsigned const num);

unsigned dequeue(Queue *q);
#endif
