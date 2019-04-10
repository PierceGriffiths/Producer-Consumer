#ifndef THREADED_FUNCTIONS_H
#define THREADED_FUNCTIONS_H
void* producer(void *const arg);

void* consumer(void *const arg);

void* producer_log_reader(void *const arg);

void* consumer_log_reader(void *const arg);
#endif
