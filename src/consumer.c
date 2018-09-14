#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "queue.h"
#include "argstructs.h"

//Global variables declared in producer-consumer.c
extern Queue * restrict buffer;
extern pthread_mutex_t mutex;
extern pthread_cond_t canProduce, canConsume; 

void* consumer(consumer_args *args){
    const unsigned long id = pthread_self();
    unsigned num, i;
    printf("Consumer thread %lu started.\n", id);
    while(args->num_consumed < *args->target){
	pthread_mutex_lock(&mutex);//Lock buffer
	while(isEmpty(buffer) && args->num_consumed < *args->target){
	    pthread_cond_wait(&canConsume, &mutex);
	}
	if(args->num_consumed == *args->target){
	    pthread_mutex_unlock(&mutex);
	    break;
	}
	i = buffer->front;//Need to get front index before it changes in next line's function call
	num = dequeue(buffer);//Remove item at front of buffer
	args->num_consumed++;//Increment num_consumed by 1
	if(*args->useConsumerLogPtr){
	    fprintf(args->consumerLog, "%ld Consumer %lu %u %u\n", (long)time(NULL), id, i, num);
	}
	printf("Consumer thread %lu consumed %u from index %u\n",
		id, num, i);
	pthread_cond_broadcast(&canProduce);//Signal to waiting producers
	pthread_mutex_unlock(&mutex);//Unlock buffer
    }
    printf("Consumer thread %lu finished.\n", id);
    return NULL;//End of thread
}
