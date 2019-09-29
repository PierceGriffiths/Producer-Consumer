#define _GNU_SOURCE

#include "macrodefs.h"
#include "queue.h"
#include "threaded_functions.h"
#include "argstruct.h"
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <inttypes.h>
#include <errno.h>

struct Queue *buffer;

int checkArguments(struct pc_thread_args *const tArgs, const char *restrict argv[], size_t *const numProducers, size_t *const numConsumers);
int forkAndJoin(const size_t numProducers, const size_t numConsumers, struct pc_thread_args *const tArgs);
int readLogFiles(const int max_p_log_line, const int max_c_log_line);

int main(int argc, const char *argv[]){
	if(argc != 5){//check for correct number of arguments
		printf("Usage: %s <# producer threads> <# consumer threads> <buffer capacity> <# items to produce>\n", argv[0]);
		return 1;
	}
	struct pc_thread_args *tArgs = malloc(sizeof *tArgs);
	if(!tArgs){
		fprintf(stderr, "Failed to allocate memory for thread arguments.\n");
		return 1;
	}
	size_t numProducers, numConsumers;
	if(checkArguments(tArgs, argv, &numProducers, &numConsumers))
		return 1;

	tArgs->producerLog = fopen(PRODUCER_LOG_FILENAME, "w");
	if(!tArgs->producerLog)
		fprintf(stderr, "Unable to open "PRODUCER_LOG_FILENAME" for writing. Proceeding without producer event logging.\n");

	tArgs->consumerLog = fopen(CONSUMER_LOG_FILENAME, "w");
	if(!tArgs->consumerLog)
		fprintf(stderr, "Unable to open "CONSUMER_LOG_FILENAME" for writing. Proceeding without consumer event logging.\n");

	if(forkAndJoin(numProducers, numConsumers, tArgs))
		return 1;
	deleteQueue(buffer);
	int max_p_log_line = tArgs->max_p_log_line;
	int max_c_log_line = tArgs->max_c_log_line;
	return readLogFiles(max_p_log_line, max_c_log_line);
}//main

#pragma GCC optimize ("Os")
int checkArguments(struct pc_thread_args *const tArgs, const char *restrict argv[], size_t *const numProducers, size_t *const numConsumers){
	for(int i = 1; i < 5; ++i){//Ensure all provided arguments are valid
		uintmax_t argCheck;
		errno = 0;
		if(argv[i][0] == '-' || !(argCheck  = strtoumax(argv[i], NULL, 10)) || argCheck > SIZE_MAX || errno == ERANGE){
			fprintf(stderr, "Argument %d (\'%s\') not valid. Please provide a positive integer no greater than %zu.\n",
					i, argv[i], SIZE_MAX);
			printf("Usage: %s <# producer threads> <# consumer threads> <buffer capacity> <# items to produce>\n", argv[0]);
			return 1;
		}
		switch(i){
			case 1:
				*numProducers = argCheck;
				continue;
			case 2:
				*numConsumers = argCheck;
				continue;
			case 3:
				buffer = createQueue(argCheck);
				if(!buffer){
					fprintf(stderr, "Failed to allocate memory for buffer.\n");
					return 1;
				}
				continue;
			case 4:
				tArgs->target = argCheck;
		}
	}
	return 0;
}//checkArguments

int forkAndJoin(const size_t numProducers, const size_t numConsumers, struct pc_thread_args *const tArgs){
	mtx_t *mutex = malloc(sizeof *mutex);
	if(!mutex || mtx_init(mutex, mtx_plain) != thrd_success){
		free(mutex);
		fprintf(stderr, "Failed to initialize mutex.\n");
		return 1;
	}

	cnd_t *canProduce = malloc(sizeof *canProduce);
	cnd_t *canConsume = malloc(sizeof *canConsume);
	if(!canProduce || !canConsume || cnd_init(canProduce) != thrd_success || cnd_init(canConsume) != thrd_success){
		free(mutex);
		free(canProduce);
		free(canConsume);
		fprintf(stderr, "Failed to initialize condition variables.\n");
		return 1;
	}
	tArgs->mutex = mutex;
	tArgs->canConsume = canConsume;
	tArgs->canProduce = canProduce;

	thrd_t *const threads = calloc(numProducers + numConsumers, sizeof *threads);
	if(!threads){
		free(mutex);
		free(canProduce);
		free(canConsume);
		fprintf(stderr, "Failed to allocate memory for threads.\n");
		return 1;
	}


	srand48((long)time(NULL));
	size_t i;
	for(i = 0; i < numConsumers; ++i){
		if(thrd_create(&threads[i], consumer, tArgs) != thrd_success){
			mtx_lock(mutex);
			fprintf(stderr, "Unable to create the requested number of consumer threads.\n");
			return 1;
		}
	}

	for(; i < numProducers + numConsumers; ++i){
		if(thrd_create(&threads[i], producer, tArgs) != thrd_success){
			mtx_lock(mutex);
			fprintf(stderr, "Unable to create the requested number of producer threads.\n");
			return 1;
		}
	}

	for(i = 0; i < numProducers + numConsumers; ++i)
		thrd_join(threads[i], NULL);

	free(threads);

	if(tArgs->producerLog != NULL)
		fclose(tArgs->producerLog);
	if(tArgs->consumerLog != NULL)
		fclose(tArgs->consumerLog);
	mtx_destroy(mutex);
	cnd_destroy(canProduce);
	cnd_destroy(canConsume);
	
	free(mutex);
	free(canProduce);
	free(canConsume);

	tArgs->mutex = NULL;
	tArgs->canConsume = NULL;
	tArgs->canProduce = NULL;

	printf("All threads finished.\n");
	printf("Produced: %zu\nConsumed: %zu\n\n", tArgs->num_produced, tArgs->num_consumed);
	return 0;
}//forkAndJoin

int readLogFiles(const int max_p_log_line, const int max_c_log_line){
	struct log_thread_args *producerlog_args, *consumerlog_args;
	thrd_t producerlog_thread, consumerlog_thread;
	int pro_log_ret, con_log_ret;

	producerlog_args = malloc(sizeof *producerlog_args);
	consumerlog_args = malloc(sizeof *consumerlog_args);
	if(!producerlog_args || !consumerlog_args){
		free(producerlog_args);
		free(consumerlog_args);
		fprintf(stderr, "Failed to initialize argument structs for log reader threads. Logs files will not be read.\n");
		return 1;
	}
	mtx_t *mutex = malloc(sizeof *mutex);

	if(!mutex || mtx_init(mutex, mtx_plain) != thrd_success){
		free(producerlog_args);
		free(consumerlog_args);
		free(mutex);
		fprintf(stderr, "Failed to initialize mutex. Log files will not be read.\n");
		return 1;
	}
	producerlog_args->mutex = mutex;
	consumerlog_args->mutex = mutex;
	producerlog_args->max_log_line = max_p_log_line;
	consumerlog_args->max_log_line = max_c_log_line;

	thrd_create(&producerlog_thread, producer_log_reader, producerlog_args);
	thrd_create(&consumerlog_thread, consumer_log_reader, consumerlog_args);


	thrd_join(producerlog_thread, &pro_log_ret);
	thrd_join(consumerlog_thread, &con_log_ret);
	free(producerlog_args);
	free(consumerlog_args);
	mtx_destroy(mutex);
	free(mutex);

	return !pro_log_ret && !con_log_ret;
}//readLogFiles
