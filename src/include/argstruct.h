#ifndef ARGSTRUCT_H
#define ARGSTRUCT_H
#define PRODUCER_LOG_FILENAME "producer-event.log"
#define CONSUMER_LOG_FILENAME "consumer-event.log"
#include <stdio.h>//FILE is defined in stdio.h
#include <pthread.h>

typedef struct{
    FILE * restrict producerLog, * restrict consumerLog;
    pthread_mutex_t * restrict mutex;
    pthread_cond_t * restrict canProduce, * restrict canConsume;
    size_t num_produced, num_consumed, target;
    struct timespec ts;
}pc_thread_args;

typedef struct{
    pthread_mutex_t * restrict mutex;
    FILE * restrict producerLog;
    short ret;
}producerlog_thread_args;

typedef struct{
    FILE  * restrict consumerLog;
    pthread_mutex_t * restrict mutex;
    short ret;
}consumerlog_thread_args;
#endif
