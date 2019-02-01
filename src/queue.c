#include <stdlib.h>
#include "queue.h"

Queue* createQueue(const size_t capacity){
    Queue *q = malloc(sizeof(*q));
    if(q != NULL){
	q->array = calloc(capacity, sizeof(*q->array));//Only need to allocate memory on queue creation, whereas using a linked list would require allocation on every enqueue
	if(q->array == NULL){
	    free(q);
	    return NULL;
	}
	q->front = 0;
	q->back = capacity - 1;
	q->size = 0;
	q->capacity = capacity;
    }
    return q;
}

void deleteQueue(Queue * restrict q){
    free(q->array);
    q->array = NULL;
    free(q);
    q = NULL;
}

size_t enqueue(Queue * restrict q, unsigned const num){
    q->back = (q->back + 1)%q->capacity;
    q->array[q->back] = num;
    ++q->size;
    return q->back;
}

size_t dequeue(Queue *restrict q, unsigned *restrict num){
    const register size_t index = q->front;
    *num = q->array[q->front];
    q->front = (q->front + 1) % q->capacity;
    --q->size;
    return index;
}
