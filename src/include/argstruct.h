#ifndef ARGSTRUCT_H
#define ARGSTRUCT_H
#define PRODUCER_LOG_FILENAME "producer-event.log"
#define CONSUMER_LOG_FILENAME "consumer-event.log"
#include <stdio.h>//FILE is defined in stdio.h
#include <pthread.h>

typedef struct{
    unsigned num_produced, num_consumed, target;
    FILE *producerLog, *consumerLog;
    pthread_mutex_t * restrict mutex;
    pthread_cond_t * restrict canProduce, * restrict canConsume;
    struct timespec ts;
}pc_thread_args;

typedef struct{
    int ret;
    FILE *producerLog;
    pthread_mutex_t *mutex;
}producerlog_thread_args;

typedef struct{
    int ret;
    FILE *consumerLog;
    pthread_mutex_t *mutex;
}consumerlog_thread_args;

#endif
