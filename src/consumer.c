#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "queue.h"
#include "argstruct.h"

//Global variables declared in producer-consumer.c
extern Queue * buffer;

void* consumer(pc_thread_args *args){
    const register pthread_t id = pthread_self();
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
	    //producer and consumer threads will only read from or write to charswritten during their critical section, 
            //so there is no need for each thread to have its own copy of charswritten stored in memory
	    args->charswritten = fprintf(args->consumerLog, "%ld Consumer %lu %u %u\n", args->ts.tv_nsec, id, i, num);
	    if(args->charswritten > args->max_c_log_line)
		args->max_c_log_line = args->charswritten;
	}
	printf("Consumer thread %lu consumed %u from index %u\n",
		id, num, i);
	pthread_mutex_unlock(args->mutex);//Unlock buffer
	pthread_cond_broadcast(args->canProduce);//Signal to waiting producers
    }
    printf("Consumer thread %lu finished.\n", id);
    pthread_exit(NULL);//End of thread
}
