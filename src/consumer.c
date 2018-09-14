#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "queue.h"
#include "argstruct.h"

//Global variables declared in producer-consumer.c
extern Queue * restrict buffer;

void* consumer(thread_args *args){
    const register unsigned long id = pthread_self();
    register unsigned num, i;
    printf("Consumer thread %lu started.\n", id);
    while(args->num_consumed < args->target){
	pthread_mutex_lock(args->mutex);//Lock buffer
	while(isEmpty(buffer) && args->num_consumed < args->target){
	    pthread_cond_wait(args->canConsume, args->mutex);
	}
	if(args->num_consumed == args->target){
	    pthread_mutex_unlock(args->mutex);
	    break;
	}
	i = buffer->front;//Need to get front index before it changes in next line's function call
	num = dequeue(buffer);//Remove item at front of buffer
	args->num_consumed++;//Increment num_consumed by 1
	if(args->consumerLog != NULL){
	    fprintf(args->consumerLog, "%ld Consumer %lu %u %u\n", (long)time(NULL), id, i, num);
	}
	printf("Consumer thread %lu consumed %u from index %u\n",
		id, num, i);
	pthread_cond_broadcast(args->canProduce);//Signal to waiting producers
	pthread_mutex_unlock(args->mutex);//Unlock buffer
    }
    printf("Consumer thread %lu finished.\n", id);
    return NULL;//End of thread
}
