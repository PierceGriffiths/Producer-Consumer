#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <queue.h>

//Global variables declared in producer-consumer.c
extern FILE *consumerLog;
extern Queue *buffer;
extern pthread_mutex_t mutex;
extern pthread_cond_t canProduce, canConsume; 
extern unsigned num_consumed, target;

void* consumer(unsigned *arg){
    struct timespec ts;
    const unsigned long id = pthread_self();
    unsigned num, i;
    printf("Consumer thread %lu started.\n", id);
    while(num_consumed < target){
        pthread_mutex_lock(&mutex);//Lock buffer
        while(isEmpty(buffer) && num_consumed < target){
            pthread_cond_wait(&canConsume, &mutex);
	}
	if(num_consumed == target){
	    pthread_mutex_unlock(&mutex);
	    break;
	}
        i = buffer->front;//Need to get front index before it changes in next line's function call
        num_consumed++;//Increment num_consumed by 1
        num = dequeue(buffer);//Remove item at front of buffer
	clock_gettime(CLOCK_REALTIME, &ts);
	fprintf(consumerLog, "%ld Consumer %lu %u %u\n", ts.tv_nsec, id, i, num);
        printf("Consumer thread %lu consumed %u from index %u\n",
                id, num, i);
	pthread_cond_broadcast(&canProduce);//Signal to waiting producers
        pthread_mutex_unlock(&mutex);//Unlock buffer
    }
    
    printf("Consumer thread %lu finished.\n", id);
    return NULL;//End of thread
}
