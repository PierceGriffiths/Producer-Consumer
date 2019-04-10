#ifndef QUEUE_H
#define QUEUE_H
typedef struct{
    long *restrict array;
    size_t front, back, size, capacity;
}Queue;

Queue* createQueue(const size_t capacity);

void deleteQueue(Queue * restrict q);

size_t enqueue(Queue * restrict q, const long num);

size_t dequeue(Queue *restrict q, long *const num);

__attribute__((always_inline)) inline int isEmpty(const Queue * restrict q){
    return (q->size == 0);
}

__attribute__((always_inline)) inline int isFull(const Queue * restrict q){
    return q->size == q->capacity;
}
#endif
