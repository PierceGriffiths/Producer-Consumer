#include <stdlib.h>
#include "queue.h"

struct Queue* createQueue(const size_t capacity){
    struct Queue *q = malloc(sizeof(*q));
    if(q != NULL){
	q->array = calloc(capacity, sizeof(*q->array));//Only need to allocate memory on queue creation, whereas using a linked list would require allocation on every enqueue
	if(q->array == NULL){
	    free(q);
	    return NULL;
	}
	q->capacity = capacity;
	q->back = capacity - 1;
	q->front = 0;
	q->size = 0;
    }
    return q;
}

void deleteQueue(struct Queue * restrict q){
    free(q->array);
    q->array = NULL;
    free(q);
    q = NULL;
}

size_t enqueue(struct Queue * restrict q, const long num){
    q->back = (q->back + 1)%q->capacity;
    q->array[q->back] = num;
    ++q->size;
    return q->back;
}

size_t dequeue(struct Queue *restrict q, long *const num){
    const size_t index = q->front;
    *num = q->array[q->front];
    q->front = (q->front + 1) % q->capacity;
    --q->size;
    return index;
}
