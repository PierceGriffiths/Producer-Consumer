#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <queue.h>

//Global variables declared in producer-consumer.c
extern FILE *producerLog;
extern Queue *buffer;
extern pthread_mutex_t mutex;
extern pthread_cond_t canProduce, canConsume;
extern unsigned num_produced, target;
extern short useProducerLog;

void* producer(void *arg){
    struct timespec ts;
    const unsigned long id = pthread_self();//Thread ID
    unsigned num, i;
    printf("Producer thread %lu started.\n", id);
    srand(time(NULL) + rand());
    while(num_produced < target){
	num = rand();//Get random number
	pthread_mutex_lock(&mutex);//Lock buffer
	while(isFull(buffer)){//If buffer is full, unlock until something is consumed
	    pthread_cond_wait(&canProduce, &mutex);
	}
	if(num_produced == target){
	    pthread_mutex_unlock(&mutex);
	    break;
	}
	num_produced++;
	i = enqueue(buffer, num);//Place num at end of the buffer and get its index
	if(useProducerLog){
	    clock_gettime(CLOCK_REALTIME, &ts);
	    fprintf(producerLog, "%ld Producer %lu %u %u\n", ts.tv_nsec, id, i, num);
	}
	printf("Producer thread %lu produced %u and stored it at index %u\n",
		id, num, i);
	
	pthread_cond_broadcast(&canConsume);//Signal to waiting consumers
	pthread_mutex_unlock(&mutex);//Unlock buffer
    }
    
    printf("Producer thread %lu finished.\n", id);
    return NULL;//End of thread
}
