#ifndef ARGSTRUCT_H
#define ARGSTRUCT_H
#include <stdio.h>//FILE is defined in stdio.h
#include <pthread.h>

typedef struct{
    FILE * restrict producerLog, * restrict consumerLog;
    pthread_mutex_t * restrict mutex;
    pthread_cond_t * restrict canProduce, * restrict canConsume;
    size_t num_produced, num_consumed, target;
    int max_p_log_line, max_c_log_line;
}pc_thread_args;

typedef struct{
    pthread_mutex_t * restrict mutex;
    int max_log_line;
    unsigned char ret;
}producerlog_thread_args;

typedef struct{
    pthread_mutex_t * restrict mutex;
    int max_log_line;
    unsigned char ret;
}consumerlog_thread_args;
#endif
