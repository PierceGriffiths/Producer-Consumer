#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "queue.h"
#include "argstruct.h"

//Global variables declared in producer-consumer.c
extern Queue * restrict buffer;

void* producer(thread_args *args){
    const register unsigned long id = pthread_self();//Thread ID
    unsigned register num, i;
    printf("Producer thread %lu started.\n", id);
    srand(time(NULL) + rand());
    while(args->num_produced < args->target){
	num = rand();//Get random number
	pthread_mutex_lock(args->mutex);//Lock buffer
	while(isFull(buffer)){//If buffer is full, unlock until something is consumed
	    pthread_cond_wait(args->canProduce, args->mutex);
	}
	if(args->num_produced == args->target){
	    pthread_mutex_unlock(args->mutex);
	    break;
	}
	args->num_produced++;
	i = enqueue(buffer, num);//Place num at end of the buffer and get its index
	if(args->producerLog != NULL){
	    clock_gettime(CLOCK_REALTIME, &args->ts);
	    fprintf(args->producerLog, "%ld Producer %lu %u %u\n", args->ts.tv_nsec, id, i, num);
	}

	printf("Producer thread %lu produced %u and stored it at index %u\n",
		id, num, i);

	pthread_cond_broadcast(args->canConsume);//Signal to waiting consumers
	pthread_mutex_unlock(args->mutex);//Unlock buffer
    }
    printf("Producer thread %lu finished.\n", id);
    return NULL;//End of thread
}
