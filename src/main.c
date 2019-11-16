#define _GNU_SOURCE

#include "macrodefs.h"
#include "queue.h"
#include "threaded_functions.h"
#include "argstruct.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <threads.h>
#include <errno.h>

static int checkArguments(struct thread_args *const tArgs, const char *restrict argv[], uint_fast16_t *const numProducers, uint_fast16_t *const numConsumers);
static int forkAndJoin(const uint_fast16_t numProducers, const uint_fast16_t numConsumers, struct thread_args *const tArgs);
static int readLogFiles(struct thread_args *const tArgs);

int main(const int argc, const char *argv[]){
	if(argc != 5){//check for correct number of arguments
		fprintf(stderr, "Usage: %s <# producer threads> <# consumer threads> <buffer capacity> <# items to produce>\n", argv[0]);
		return 1;
	}
	struct thread_args tArgs = {0};

	uint_fast16_t numProducers = 0, numConsumers = 0;
	if(checkArguments(&tArgs, argv, &numProducers, &numConsumers))
		return 1;

	tArgs.producerLog = fopen(PRODUCER_LOG_FILENAME, "w");
	if(!tArgs.producerLog)
		fprintf(stderr, "Unable to open "PRODUCER_LOG_FILENAME" for writing\n");

	tArgs.consumerLog = fopen(CONSUMER_LOG_FILENAME, "w");
	if(!tArgs.consumerLog)
		fprintf(stderr, "Unable to open "CONSUMER_LOG_FILENAME" for writing\n");

	if(forkAndJoin(numProducers, numConsumers, &tArgs))
		return 1;
	return readLogFiles(&tArgs);
}//main

static int checkArguments(struct thread_args *const tArgs, const char *restrict argv[], uint_fast16_t *const numProducers, uint_fast16_t *const numConsumers){
	for(unsigned char i = 1; i < 5; ++i){//Ensure all provided arguments are valid
		uintmax_t argCheck;
		errno = 0;
		if(argv[i][0] == '-' || !(argCheck  = strtoumax(argv[i], NULL, 10)) || errno == ERANGE || argCheck > UINT_FAST16_MAX){
			fprintf(stderr, "Argument %d (\'%s\') invalid. A positive integer no greater than %"PRIuFAST16" is required\n",
					i, argv[i], UINT_FAST16_MAX);
			fprintf(stderr, "Usage: %s <# producer threads> <# consumer threads> <buffer capacity> <# items to produce>\n", argv[0]);
			return 1;
		}
		switch(i){
			case 1:
				*numProducers = argCheck;
				break;
			case 2:
				*numConsumers = argCheck;
				break;
			case 3:
				tArgs->buffer = createQueue(argCheck);
				if(!tArgs->buffer){
					fprintf(stderr, "Failed to allocate memory for buffer\n");
					return 1;
				}
				break;
			case 4:
				tArgs->target = argCheck;
		}
	}
	return 0;
}//checkArguments

static int forkAndJoin(const uint_fast16_t numProducers, const uint_fast16_t numConsumers, struct thread_args *const tArgs){
	tArgs->mutex = malloc(sizeof *tArgs->mutex);
	if(!tArgs->mutex || mtx_init(tArgs->mutex, mtx_plain) != thrd_success){
		fprintf(stderr, "Failed to initialize mutex\n");
		return 1;
	}

	tArgs->canProduce = malloc(sizeof *tArgs->canProduce);
	tArgs->canConsume = malloc(sizeof *tArgs->canConsume);
	if(!tArgs->canProduce || !tArgs->canConsume || cnd_init(tArgs->canProduce) != thrd_success || cnd_init(tArgs->canConsume) != thrd_success){
		fprintf(stderr, "Failed to initialize condition variables\n");
		return 1;
	}

	thrd_t *const threads = calloc(numProducers + numConsumers, sizeof *threads);
	if(!threads){
		fprintf(stderr, "Failed to allocate memory for thread IDs\n");
		return 1;
	}


	size_t i;
	for(i = 0; i < numConsumers; ++i){
		if(thrd_create(&threads[i], consumer, tArgs) != thrd_success){
			mtx_lock(tArgs->mutex);
			fprintf(stderr, "Failed to create the requested number of consumer threads\n");
			return 1;
		}
	}

	srand(time(NULL));
	for(size_t i = numConsumers; i < numProducers + numConsumers; ++i){
		if(thrd_create(&threads[i], producer, tArgs) != thrd_success){
			mtx_lock(tArgs->mutex);
			fprintf(stderr, "Failed to create the requested number of producer threads\n");
			return 1;
		}
	}

	for(i = 0; i < numProducers + numConsumers; ++i)
		thrd_join(threads[i], NULL);

	free(threads);
	free(tArgs->buffer->array);
	free(tArgs->buffer);
	cnd_destroy(tArgs->canProduce);
	cnd_destroy(tArgs->canConsume);
	
	free(tArgs->canProduce);
	free(tArgs->canConsume);

	if(tArgs->producerLog != NULL)
		fclose(tArgs->producerLog);
	if(tArgs->consumerLog != NULL)
		fclose(tArgs->consumerLog);

	printf("All threads finished\nProduced: %"PRIuFAST16"\nConsumed: %"PRIuFAST16"\n\n", tArgs->num_produced, tArgs->num_consumed);
	return 0;
}//forkAndJoin

static int readLogFiles(struct thread_args *const tArgs){
	thrd_t producerlog_thread, consumerlog_thread;
	int pro_log_ret, con_log_ret;

	thrd_create(&producerlog_thread, producer_log_reader, tArgs);
	thrd_create(&consumerlog_thread, consumer_log_reader, tArgs);


	thrd_join(producerlog_thread, &pro_log_ret);
	thrd_join(consumerlog_thread, &con_log_ret);
	mtx_destroy(tArgs->mutex);
	free(tArgs->mutex);

	return !pro_log_ret && !con_log_ret;
}//readLogFiles
