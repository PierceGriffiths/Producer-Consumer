#define _GNU_SOURCE

#include "argstruct.h"
#include "macrodefs.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#ifdef __linux__
#include <sys/types.h>
#include <sys/syscall.h>
#endif

#include <stdio.h>
#include <time.h>
#include <threads.h>
#include <errno.h>
#include <string.h>

int consumer(struct thread_args *const args){
#ifdef __linux__
	register const pid_t thread_id = syscall(SYS_gettid);//get thread ID 
#else
	register const uintmax_t thread_id = (uintmax_t)thrd_current();
#endif
	printf("Consumer thread %"ID_FORMAT" started.\n", thread_id);
	while(args->num_consumed < args->target){
#ifndef _WIN32
		nanosleep(NANOSLEEP_TIME, NULL);//sleep for 1 nanosecond so that other consumers can acquire the mutex
#endif
		mtx_lock(args->mutex);
		while(isEmpty(args->buffer) && args->num_consumed < args->target)
			cnd_wait(args->canConsume, args->mutex);

		if(args->num_consumed == args->target){
			printf("Consumer thread %"ID_FORMAT" finished.\n", thread_id);
			mtx_unlock(args->mutex);
			return 0;
		}
		uint_fast16_t index;
		const int num = dequeue(args->buffer, &index);
		if(args->consumerLog != NULL){
			struct timespec ts;
			timespec_get(&ts, TIME_UTC);

			errno = 0;
			const int_fast16_t charswritten = fprintf(args->consumerLog, "%ld%09ld Consumer %"ID_FORMAT" %"PRIuFAST16" %d\n", (long)ts.tv_sec, ts.tv_nsec, thread_id, index, num);
			const int errnoCopy = errno;
			if(charswritten < 0){
				fprintf(stderr, "Consumer thread %"ID_FORMAT" failed to write to "CONSUMER_LOG_FILENAME"\n", thread_id);
				char err_buff[ERR_BUFF_LEN];
				strerror_r(errnoCopy, err_buff, ERR_BUFF_LEN);
				fprintf(stderr, "Error description: %s\n", err_buff);
				if(errnoCopy != EOVERFLOW || errnoCopy != EAGAIN || errnoCopy != EINTR){
					fprintf(stderr,  "Consumer threads will no longer write to "CONSUMER_LOG_FILENAME"\n");
					fclose(args->consumerLog);
					args->consumerLog = NULL;
				}
			}
			else if(charswritten > args->max_c_log_line){
				args->max_c_log_line = charswritten;
			}
		}
		printf("Consumer thread %"ID_FORMAT" consumed %d from index %"PRIuFAST16"\n", thread_id, num, index);

		++args->num_consumed;//Increment num_consumed by 1

		cnd_broadcast(args->canProduce);//Signal to waiting producers
		mtx_unlock(args->mutex);//Unlock buffer
	}
	mtx_lock(args->mutex);
	printf("Consumer thread %"ID_FORMAT" finished.\n", thread_id);
	mtx_unlock(args->mutex);
	return 0;
}
