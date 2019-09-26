#ifndef THREADED_FUNCTIONS_H
#define THREADED_FUNCTIONS_H
int producer(void *const arg);

int consumer(void *const arg);

int producer_log_reader(void *const arg);

int consumer_log_reader(void *const arg);
#endif
