#ifndef THREADED_FUNCTIONS_H
#define THREADED_FUNCTIONS_H
void* producer(void *arg);

void* consumer(void *arg);

void* producer_log_reader(void *arg);

void* consumer_log_reader(void *arg);
#endif
