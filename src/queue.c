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

void deleteQueue(Queue *q){
    q->front = 0;
    q->back = 0;
    q->size = 0;
    q->capacity = 0;
    free(q->array);
    q->array = NULL;
}

int isEmpty(Queue *q){//returns 1 if the array is empty, and 0 otherwise
    return(q->size == 0);
}

int isFull(Queue *q){//Returns 1 if the array is full, and 0 otherwise
    return (q->size == q->capacity);
}

unsigned enqueue(Queue *q, unsigned num){
    q->back = (q->back + 1)%q->capacity;
    q->array[q->back] = num;
    q->size++;
    return q->back;
}

unsigned dequeue(Queue *q){
    unsigned data;
    data = q->array[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    return data;
}
