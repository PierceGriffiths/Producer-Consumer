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
    int max_p_log_line, max_c_log_line;
    struct timespec ts;
    int charswritten;
}pc_thread_args;

typedef struct{
    FILE * restrict producerLog;
    pthread_mutex_t * restrict mutex;
    int max_log_line;
    short ret;
}producerlog_thread_args;

typedef struct{
    FILE  * restrict consumerLog;
    pthread_mutex_t * restrict mutex;
    int max_log_line;
    short ret;
}consumerlog_thread_args;
#endif
