#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "queue.h"
#include "argstruct.h"

//Global variables declared in producer-consumer.c
extern Queue * buffer;

void* consumer(pc_thread_args *args){
    const register unsigned long id = pthread_self();
    unsigned num, i;
    printf("Consumer thread %lu started.\n", id);
    while(args->num_consumed < args->target){
	pthread_mutex_lock(args->mutex);//Lock buffer
	while(isEmpty(buffer) && args->num_consumed < args->target){
	    pthread_cond_wait(args->canConsume, args->mutex);
	}
	if(args->num_consumed == args->target){
	    printf("Consumer thread %lu finished.\n", id);
	    pthread_mutex_unlock(args->mutex);
	    pthread_exit(NULL);
	}
	i = dequeue(buffer, &num);
	++args->num_consumed;//Increment num_consumed by 1
	if(args->consumerLog != NULL){
	    clock_gettime(CLOCK_REALTIME, &args->ts);
	    fprintf(args->consumerLog, "%ld Consumer %lu %u %u\n", args->ts.tv_nsec, id, i, num);
	}
	printf("Consumer thread %lu consumed %u from index %u\n",
		id, num, i);
	pthread_mutex_unlock(args->mutex);//Unlock buffer
	pthread_cond_broadcast(args->canProduce);//Signal to waiting producers
    }
    printf("Consumer thread %lu finished.\n", id);
    pthread_exit(NULL);//End of thread
}
