#ifndef ARGSTRUCT_H
#define ARGSTRUCT_H
#include <stdio.h>//FILE is defined in stdio.h
#include <pthread.h>
typedef struct{
    unsigned num_produced, num_consumed, target;
    FILE * restrict producerLog, * restrict consumerLog;
    pthread_mutex_t * restrict mutex;
    pthread_cond_t * restrict canProduce, * restrict canConsume;
    struct timespec ts;
}thread_args;
#endif
