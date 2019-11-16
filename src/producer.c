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
#include <stdlib.h>
#include <threads.h>
#include <time.h>
#include <errno.h>
#include <string.h>

int producer(struct thread_args *const args){
#ifdef __linux__
	register const pid_t thread_id = syscall(SYS_gettid);//get thread ID 
#else
	register const uintmax_t thread_id = (uintmax_t)thrd_current();
#endif
	printf("Producer thread %"ID_FORMAT" started\n", thread_id);
	while(args->num_produced < args->target){
#ifndef _WIN32
		nanosleep(NANOSLEEP_TIME, NULL);//sleep for one nanosecond so that other producers can acquire the mutex
#endif
		mtx_lock(args->mutex);//Lock buffer
		const int num = rand();//Get random number

		while(isFull(args->buffer))//If buffer is full, unlock until something is consumed
			cnd_wait(args->canProduce, args->mutex);

		if(args->num_produced == args->target){
			printf("Producer thread %"ID_FORMAT" finished\n", thread_id);
			mtx_unlock(args->mutex);
			return 0;
		}
		const uint_fast16_t index = enqueue(args->buffer, num);//Place num at end of the buffer and get its index
		if(args->producerLog != NULL){
			struct timespec ts;
			timespec_get(&ts, TIME_UTC);
			
			errno = 0;
			const int_fast16_t charswritten = fprintf(args->producerLog, "%ld%09ld Producer %"ID_FORMAT" %"PRIuFAST16" %d\n", (long)ts.tv_sec, ts.tv_nsec, thread_id, index, num);
			const int errnoCopy = errno;
			if(charswritten < 0){
				fprintf(stderr, "Producer thread %"ID_FORMAT" failed to write to "PRODUCER_LOG_FILENAME"\n", thread_id);
				char err_buff[ERR_BUFF_LEN];
                strerror_r(errnoCopy, err_buff, ERR_BUFF_LEN);  
                fprintf(stderr, "Error string: %s\n", err_buff);

				if(errnoCopy != EOVERFLOW || errnoCopy != EAGAIN || errnoCopy != EINTR){
					fprintf(stderr, "Producer threads will no longer write to "PRODUCER_LOG_FILENAME"\n");
					fclose(args->producerLog);
					args->producerLog = NULL;
				}
			}
			else if(charswritten > args->max_p_log_line){
				args->max_p_log_line = charswritten;
			}
		}

		printf("Producer thread %"ID_FORMAT" produced %d and stored it at index %"PRIuFAST16"\n", thread_id, num, index);

		++args->num_produced;

		cnd_broadcast(args->canConsume);//Signal to waiting consumers
		mtx_unlock(args->mutex);//Unlock buffer
	}
	mtx_lock(args->mutex);
	printf("Producer thread %"ID_FORMAT" finished.\n", thread_id);
	mtx_unlock(args->mutex);
	return 0;
}
