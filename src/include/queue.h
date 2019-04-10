#ifndef QUEUE_H
#define QUEUE_H
struct Queue{
    long *restrict array;
    size_t front, back, size, capacity;
};

struct Queue* createQueue(const size_t capacity);

void deleteQueue(struct Queue * restrict q);

size_t enqueue(struct Queue * restrict q, const long num);

size_t dequeue(struct Queue *restrict q, long *const num);

__attribute__((always_inline)) inline int isEmpty(const struct Queue * restrict q){
    return (q->size == 0);
}

__attribute__((always_inline)) inline int isFull(const struct Queue * restrict q){
    return q->size == q->capacity;
}
#endif
