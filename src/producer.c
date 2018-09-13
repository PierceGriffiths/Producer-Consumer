#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "queue.h"
#include "argstructs.h"

//Global variables declared in producer-consumer.c
extern Queue *buffer;
extern pthread_mutex_t mutex;
extern pthread_cond_t canProduce, canConsume;


void* producer(producer_args *args){
    struct timespec ts;
    const unsigned long id = pthread_self();//Thread ID
    unsigned num, i;
    printf("Producer thread %lu started.\n", id);
    srand(time(NULL) + rand());
    while(args->num_produced < *args->target){
	num = rand();//Get random number
	pthread_mutex_lock(&mutex);//Lock buffer
	while(isFull(buffer)){//If buffer is full, unlock until something is consumed
	    pthread_cond_wait(&canProduce, &mutex);
	}
	if(args->num_produced == *args->target){
	    pthread_mutex_unlock(&mutex);
	    break;
	}
	args->num_produced++;
	i = enqueue(buffer, num);//Place num at end of the buffer and get its index
	if(*args->useProducerLogPtr){
	    clock_gettime(CLOCK_REALTIME, &ts);
	    fprintf(args->producerLog, "%ld Producer %lu %u %u\n", ts.tv_nsec, id, i, num);
	}

	printf("Producer thread %lu produced %u and stored it at index %u\n",
		id, num, i);

	pthread_cond_broadcast(&canConsume);//Signal to waiting consumers
	pthread_mutex_unlock(&mutex);//Unlock buffer
    }
    printf("Producer thread %lu finished.\n", id);
    return NULL;//End of thread
}
