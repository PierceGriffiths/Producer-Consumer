#define _GNU_SOURCE
#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#endif

#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include "queue.h"
#include "argstruct.h"
#include "macrodefs.h"

//Global variables declared in main.c
extern struct Queue * buffer;

void* consumer(struct pc_thread_args *const args){
#ifdef __linux__
    const register pid_t thread_id = syscall(SYS_gettid);//get thread ID 
#else
    const register unsigned long long = (unsigned long long)pthread_self();
#endif
    int charswritten;
    long num;
    size_t index;
    struct timespec ts;
    printf("Consumer thread %"ID_FORMAT" started.\n", thread_id);
    while(args->num_consumed < args->target){
	nanosleep(NANOSLEEP_TIME, NULL);//sleep for 1 nanosecond so that other consumers can acquire the mutex
	pthread_mutex_lock(args->mutex);//Lock buffer
	while(isEmpty(buffer) && args->num_consumed < args->target){
	    pthread_cond_wait(args->canConsume, args->mutex);
	}
	if(args->num_consumed == args->target){
	    printf("Consumer thread %"ID_FORMAT" finished.\n", thread_id);
	    pthread_mutex_unlock(args->mutex);
	    pthread_exit(NULL);
	}
	index = dequeue(buffer, &num);
	timespec_get(&ts, TIME_UTC);
	if(args->consumerLog != NULL){
	    charswritten = fprintf(args->consumerLog, "%lld%09ld Consumer %"ID_FORMAT" %zu %ld\n", (long long)ts.tv_sec, ts.tv_nsec, thread_id, index, num);
	    if(charswritten < 0){ 
		int errnoCopy = errno;
		fprintf(stderr, "Consumer thread %"ID_FORMAT" encountered an error while writing to "CONSUMER_LOG_FILENAME". Write operation failed.\n", thread_id);
		fprintf(stderr, "Error description: %s", strerror(errnoCopy));
		if(errnoCopy != EOVERFLOW || errnoCopy != EAGAIN || errnoCopy != EINTR){
		    fprintf(stderr, "Due to the nature of the error, consumer threads will no longer write to "CONSUMER_LOG_FILENAME" for the remainder of runtime.\n");
		    fclose(args->consumerLog);
		    args->consumerLog = NULL;
		}
	    }
	    else if(charswritten > args->max_c_log_line){
		args->max_c_log_line = charswritten;
	    }
	}
	printf("Consumer thread %"ID_FORMAT" consumed %ld from index %zu\n", thread_id, num, index);
	
	++args->num_consumed;//Increment num_consumed by 1
	
	pthread_mutex_unlock(args->mutex);//Unlock buffer
	pthread_cond_broadcast(args->canProduce);//Signal to waiting producers
    }
    pthread_mutex_lock(args->mutex);
    printf("Consumer thread %"ID_FORMAT" finished.\n", thread_id);
    pthread_mutex_unlock(args->mutex);
    pthread_exit(NULL);//End of thread
}
