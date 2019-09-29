#define _GNU_SOURCE

#include "queue.h"
#include "argstruct.h"
#include "macrodefs.h"

#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#endif

#include <stdio.h>
#include <time.h>
#include <threads.h>
#include <errno.h>
#include <string.h>

//Global variables declared in main.c
extern struct Queue *buffer;

int consumer(struct pc_thread_args *const args){
#ifdef __linux__
	register const pid_t thread_id = syscall(SYS_gettid);//get thread ID 
#else
	register const uintmax_t thread_id = (uintmax_t)pthread_self();
#endif
	int charswritten;
	printf("Consumer thread %"ID_FORMAT" started.\n", thread_id);
	while(args->num_consumed < args->target){
		nanosleep(NANOSLEEP_TIME, NULL);//sleep for 1 nanosecond so that other consumers can acquire the mutex
		mtx_lock(args->mutex);
		while(isEmpty(buffer) && args->num_consumed < args->target){
			cnd_wait(args->canConsume, args->mutex);
		}
		if(args->num_consumed == args->target){
			printf("Consumer thread %"ID_FORMAT" finished.\n", thread_id);
			mtx_unlock(args->mutex);
			return 0;
		}
		unsigned short index;
		long num = dequeue(buffer, &index);
		if(args->consumerLog != NULL){
			struct timespec ts;
			timespec_get(&ts, TIME_UTC);
			charswritten = fprintf(args->consumerLog, "%lld%09ld Consumer %"ID_FORMAT" %hu %ld\n", (long long)ts.tv_sec, ts.tv_nsec, thread_id, index, num);
			if(charswritten < 0){
				int errnoCopy = errno;
				fprintf(stderr, "Consumer thread %"ID_FORMAT" encountered an error while writing to "CONSUMER_LOG_FILENAME". Write operation failed.\n", thread_id);
				char err_buff[ERR_BUFF_LEN];
				strerror_r(errnoCopy, err_buff, ERR_BUFF_LEN);
				fprintf(stderr, "Error description: %s\n", err_buff);
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
		printf("Consumer thread %"ID_FORMAT" consumed %ld from index %hu\n", thread_id, num, index);

		++args->num_consumed;//Increment num_consumed by 1

		mtx_unlock(args->mutex);//Unlock buffer
		cnd_broadcast(args->canProduce);//Signal to waiting producers
	}
	mtx_lock(args->mutex);
	printf("Consumer thread %"ID_FORMAT" finished.\n", thread_id);
	mtx_unlock(args->mutex);
	return 0;
}
