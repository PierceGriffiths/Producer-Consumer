#include <stdlib.h>

typedef struct{
    unsigned front, back, size, capacity, * restrict array;
}Queue;

Queue* createQueue(unsigned const capacity){
    Queue *q = malloc(sizeof(*q));
    if(q != NULL){
	q->capacity = capacity;
	q->front = 0;
	q->back = capacity - 1;
	q->size = 0;
	q->array = calloc(capacity, sizeof(*q->array));//Only need to allocate memory on queue creation, whereas using a linked list would require allocation on every enqueue
    }
    return q;
}

void* deleteQueue(Queue * restrict q){
    free(q->array);
    q->array = NULL;
    free(q);
    return NULL;
}

unsigned enqueue(Queue * restrict q, unsigned const num){
    q->back = (q->back + 1)%q->capacity;
    q->array[q->back] = num;
    q->size++;
    return q->back;
}

unsigned dequeue(Queue *restrict q){
    unsigned data = q->array[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    return data;
}
