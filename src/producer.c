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
#include <stdlib.h>
#include <threads.h>
#include <time.h>
#include <errno.h>
#include <string.h>

//Global variables declared in main.c
extern struct Queue * buffer;

int producer(struct pc_thread_args *const args){
	int charswritten;
#ifdef __linux__
	register const pid_t thread_id = syscall(SYS_gettid);//get thread ID 
#else
	register const uintmax_t thread_id = (uintmax_t)thrd_current();
#endif
	printf("Producer thread %"ID_FORMAT" started.\n", thread_id);
	while(args->num_produced < args->target){
		const long num = lrand48();//Get random number
		nanosleep(NANOSLEEP_TIME, NULL);//sleep for one nanosecond so that other producers can acquire the mutex
		mtx_lock(args->mutex);//Lock buffer
		
		while(isFull(buffer)){//If buffer is full, unlock until something is consumed
			cnd_wait(args->canProduce, args->mutex);
		}
		if(args->num_produced == args->target){
			printf("Producer thread %"ID_FORMAT" finished.\n", thread_id);
			mtx_unlock(args->mutex);
			return 0;
		}
		if(args->producerLog != NULL){
			const unsigned short index = enqueue(buffer, num);//Place num at end of the buffer and get its index
			
			struct timespec ts;
			timespec_get(&ts, TIME_UTC);
			
			errno = 0;
			charswritten = fprintf(args->producerLog, "%lld%09ld Producer %"ID_FORMAT" %hu %ld\n", (long long)ts.tv_sec, ts.tv_nsec, thread_id, index, num);
			const int errnoCopy = errno;
			if(charswritten < 0){
				fprintf(stderr, "Producer thread %"ID_FORMAT" encountered an error while writing to "PRODUCER_LOG_FILENAME". Write operation failed.\n", thread_id);
				char err_buff[ERR_BUFF_LEN];
                strerror_r(errnoCopy, err_buff, ERR_BUFF_LEN);  
                fprintf(stderr, "Error description: %s\n", err_buff);

				if(errnoCopy != EOVERFLOW || errnoCopy != EAGAIN || errnoCopy != EINTR){
					fprintf(stderr, "Due to the nature of the error, producer threads will no longer write to "PRODUCER_LOG_FILENAME" for the remainder of runtime.\n");
					fclose(args->producerLog);
					args->producerLog = NULL;
				}
			}
			else if(charswritten > args->max_p_log_line){
				args->max_p_log_line = charswritten;
			}
			printf("Producer thread %"ID_FORMAT" produced %ld and stored it at index %hu\n", thread_id, num, index);
		}
		else{
			printf("Producer thread %"ID_FORMAT" produced %ld and stored it at index %hu\n", thread_id, num, enqueue(buffer, num));
		}

		++args->num_produced;

		mtx_unlock(args->mutex);//Unlock buffer
		cnd_broadcast(args->canConsume);//Signal to waiting consumers
	}
	mtx_lock(args->mutex);
	printf("Producer thread %"ID_FORMAT" finished.\n", thread_id);
	mtx_unlock(args->mutex);
	return 0;
}
