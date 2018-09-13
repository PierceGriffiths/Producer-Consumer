#include <stdlib.h>

typedef struct{
    unsigned front, back, size, capacity, *array;
}Queue;

Queue* createQueue(unsigned capacity){
    Queue *q = malloc(sizeof(*q));
    q->capacity = capacity;
    q->front = 0;
    q->back = capacity - 1;
    q->size = 0;
    q->array = calloc(capacity, sizeof(*q->array));//Only need to allocate memory on queue creation, whereas using a linked list would require allocation on every enqueue
    return q;
}

void* deleteQueue(Queue *q){
    free(q->array);
    q->array = NULL;
    return NULL;
}

unsigned enqueue(Queue *q, unsigned num){
    q->back = (q->back + 1)%q->capacity;
    q->array[q->back] = num;
    q->size++;
    return q->back;
}

unsigned dequeue(Queue *q){
    unsigned data = q->array[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    return data;
}
