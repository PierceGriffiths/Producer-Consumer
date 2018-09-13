#ifndef QUEUE_H
#define QUEUE_H
typedef struct{
    unsigned front, back, size, capacity, *array;
}Queue;

Queue* createQueue(unsigned capacity);

void* deleteQueue(Queue *q);

inline int isEmpty(Queue *q){
    return (q->size == 0);
}

inline int isFull(Queue *q){
    return (q->size == q->capacity);
}

unsigned enqueue(Queue *q, unsigned num);

unsigned dequeue(Queue *q);
#endif
