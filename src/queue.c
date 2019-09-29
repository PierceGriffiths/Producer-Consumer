#include "queue.h"
#include <stdlib.h>
#include <string.h>

struct Queue* createQueue(const unsigned short capacity){
	const struct Queue q_template = {calloc(capacity, sizeof *q_template.array), capacity, 0, 0, capacity - 1};
	struct Queue *q = malloc(sizeof *q);
	if(!q || !q_template.array){
		free(q);
		free(q_template.array);
		return NULL;
	}
	else{
		memcpy(q, &q_template, sizeof *q);
		return q;
	}
}

void deleteQueue(struct Queue *restrict q){
	free(q->array);
	free(q);
}

unsigned short enqueue(struct Queue *const restrict q, const long num){
	q->back = (q->back + 1) % q->capacity;
	q->array[q->back] = num;
	++q->size;
	return q->back;
}

long dequeue(struct Queue *const restrict q, unsigned short *const index){
	*index = q->front;
	const long num = q->array[q->front];
	q->front = (q->front + 1) % q->capacity;
	--q->size;
	return num;
}
