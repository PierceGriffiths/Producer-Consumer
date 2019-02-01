#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "queue.h"
#include "argstruct.h"

//Global variables declared in producer-consumer.c
extern Queue * buffer;

void* producer(pc_thread_args *args){
    const register unsigned long id = pthread_self();//Thread ID
    unsigned register num;
    size_t i;
    printf("Producer thread %lu started.\n", id);
    srand(time(NULL) + rand());
    while(args->num_produced < args->target){
	num = rand();//Get random number
	pthread_mutex_lock(args->mutex);//Lock buffer
	while(isFull(buffer)){//If buffer is full, unlock until something is consumed
	    pthread_cond_wait(args->canProduce, args->mutex);
	}
	if(args->num_produced == args->target){
	    printf("Producer thread %lu finished.\n", id);
	    pthread_mutex_unlock(args->mutex);
	    pthread_exit(NULL);//Eliminates branch instruction at assembly level
	}
	if(args->producerLog != NULL){
	    i = enqueue(buffer, num);//Place num at end of the buffer and get its index
	    clock_gettime(CLOCK_REALTIME, &args->ts);
	    fprintf(args->producerLog, "%ld Producer %lu %zu %u\n", args->ts.tv_nsec, id, i, num);
	    printf("Producer thread %lu produced %u and stored it at index %zu\n",
		id, num, i);
	}
	else{//Don't bother writing the index to i when producer-event.log isn't being written to
	    printf("Producer thread %lu produced %u and stored it at index %zu\n", id, num, enqueue(buffer, num));
	}

	++args->num_produced;

	pthread_mutex_unlock(args->mutex);//Unlock buffer
	pthread_cond_broadcast(args->canConsume);//Signal to waiting consumers
    }
    printf("Producer thread %lu finished.\n", id);
    pthread_exit(NULL);//End of thread
}
