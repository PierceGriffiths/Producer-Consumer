#ifndef ARGSTRUCTS_H
#define ARGSTRUCTS_H
#include <stdio.h>//FILE is defined in stdio.h
//the "target"s in both of these structures will, in the instances of these structs which exist at runtime, both point to the same location in memory
typedef struct{
    unsigned num_produced, *useProducerLogPtr, *target;
    FILE * producerLog;
}producer_args;

typedef struct{
    unsigned num_consumed, *useConsumerLogPtr, *target;
    FILE * consumerLog;
}consumer_args;
#endif
