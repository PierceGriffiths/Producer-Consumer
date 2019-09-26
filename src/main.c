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

#ifdef SUPPORTS_RLIM
#include <sys/resource.h>
#endif

struct Queue * buffer;

struct pc_thread_args* checkArguments(const char *restrict argv[], size_t *numProducers, size_t *numConsumers);
unsigned char forkAndJoin(const size_t numProducers, const size_t numConsumers, struct pc_thread_args *const tArgs);
unsigned char readLogFiles(const int max_p_log_line, const int max_c_log_line);

int main(int argc, const char *argv[]){
	size_t numProducers = 0, numConsumers = 0;

	if(argc != 5){//check for correct number of arguments
		printf("Usage: %s <# producer threads> <# consumer threads> <buffer capacity> <# items to produce>\n", argv[0]);
		return 1;
	}

	struct pc_thread_args *tArgs = checkArguments(argv, &numProducers, &numConsumers);
	if(!tArgs)
		return 1;

	tArgs->producerLog = fopen(PRODUCER_LOG_FILENAME, "w");
	if(!tArgs->producerLog)
		fprintf(stderr, "Unable to open "PRODUCER_LOG_FILENAME" for writing. Proceeding without producer event logging.\n");

	tArgs->consumerLog = fopen(CONSUMER_LOG_FILENAME, "w");
	if(!tArgs->consumerLog)
		fprintf(stderr, "Unable to open "CONSUMER_LOG_FILENAME" for writing. Proceeding without consumer event logging.\n");

	if(forkAndJoin(numProducers, numConsumers, tArgs))
		return 1;
	int max_p_log_line = tArgs->max_p_log_line;
	int max_c_log_line = tArgs->max_c_log_line;
	free(tArgs);
	return readLogFiles(max_p_log_line, max_c_log_line);
}//main


struct pc_thread_args* checkArguments(const char *restrict argv[], size_t *numProducers, size_t *numConsumers){
	struct pc_thread_args *tArgs = NULL;
	size_t argCheck;
	unsigned char i;

	for(i = 1; i < 5; ++i){//Ensure all provided arguments are valid
		if(argv[i][0] == '-')
			goto invalidarg;
		errno = 0;
		argCheck = strtoumax(argv[i], NULL, 10);
		if(errno == ERANGE || argCheck == 0){
invalidarg:
			fprintf(stderr, "Argument %u (\'%s\') not valid. Please provide a positive integer no greater than %zu.\n",
					i, argv[i], SIZE_MAX);
			printf("Usage: %s <# producer threads> <# consumer threads> <buffer capacity> <# items to produce>\n", argv[0]);
			return NULL;
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
					return NULL;
				}
				continue;
			case 4:
				tArgs = malloc(sizeof *tArgs);
				if(tArgs){
					tArgs->target = argCheck;
				}
				else{
					fprintf(stderr, "Failed to allocate memory for thread arguments.\n");
					return NULL;
				}
		}
	}
#ifdef SUPPORTS_RLIM
	struct rlimit rlim;
	prlimit(0, RLIMIT_NPROC, NULL, &rlim);//Stores the soft and hard limits for the number of threads that the invoking user is permitted to have running
	if(*numProducers + *numConsumers >= rlim.rlim_cur){
		fprintf(stderr, "The number of threads you wish to create exceeds the hard limit for the number of threads that can be created by the current user.\n");
		deleteQueue(buffer);
		free(tArgs);
		return NULL;
	}
#endif
	tArgs->num_produced = 0;
	tArgs->num_consumed = 0;
	return tArgs;
}//checkArguments

unsigned char forkAndJoin(const size_t numProducers, const size_t numConsumers, struct pc_thread_args *const tArgs){

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

	thrd_t *const producers = calloc(numProducers, sizeof *producers);//See producer.c for the implementation of the producer function
	if(!producers){
		free(mutex);
		free(canProduce);
		free(canConsume);
		fprintf(stderr, "Failed to allocate memory for producer threads.\n");
		return 1;
	}

	thrd_t *const consumers = calloc(numConsumers, sizeof *consumers);//See consumer.c for the implementation of the consumer function
	if(!consumers){
		free(mutex);
		free(canProduce);
		free(canConsume);
		free(producers);
		fprintf(stderr, "Failed to allocate memory for consumer threads.\n");
		return 1;
	}

	srand48((long)time(NULL));
	tArgs->max_p_log_line = 0;
	tArgs->max_c_log_line = 0;
	if(numProducers >= numConsumers){
		size_t i;
		for(i = 0; i < numConsumers; ++i){
			if(thrd_create(&producers[i], producer, tArgs) != thrd_success){
				mtx_lock(mutex);
				fprintf(stderr, "Unable to create the requested number of producer threads.\n");
				return 1;
			}

			if(thrd_create(&consumers[i], consumer, tArgs) != thrd_success){
				mtx_lock(mutex);
				fprintf(stderr, "Unable to create the requested number of consumer threads.\n");
				return 1;
			}
		}

		for(; i < numProducers; ++i){
			if(thrd_create(&producers[i], producer, tArgs) != thrd_success){
				mtx_lock(mutex);
				fprintf(stderr, "Unable to create the requested number of producer threads.\n");
				return 1;
			}
		}

		for(i = 0; i < numConsumers; ++i){
			thrd_join(producers[i], NULL);
			thrd_join(consumers[i], NULL);
		}

		for(; i < numProducers; ++i)
			thrd_join(producers[i], NULL);

	}
	else{
		size_t i;
		for(i = 0; i < numProducers; ++i){
			if(thrd_create(&producers[i], producer, tArgs) != thrd_success){
				mtx_lock(mutex);
				fprintf(stderr, "Unable to create the requested number of producer threads.\n");
				return 1;
			}

			if(thrd_create(&consumers[i], consumer, tArgs) != thrd_success){
				mtx_lock(mutex);
				fprintf(stderr, "Unable to create the requested number of consumer threads.\n");
				return 1;
			}
		}

		for(; i < numConsumers; ++i){
			if(thrd_create(&consumers[i], consumer, tArgs) != thrd_success){
				mtx_lock(mutex);
				fprintf(stderr, "Unable to create the requested number of consumer threads.\n");
				return 1;
			}
		}

		for(i = 0; i < numProducers; ++i){
			thrd_join(producers[i], NULL);
			thrd_join(consumers[i], NULL);
		}

		for(; i < numConsumers; ++i)
			thrd_join(consumers[i], NULL);
	}//end of else

	if(tArgs->producerLog != NULL)
		fclose(tArgs->producerLog);
	if(tArgs->consumerLog != NULL)
		fclose(tArgs->consumerLog);
	mtx_destroy(mutex);
	cnd_destroy(canProduce);
	cnd_destroy(canConsume);
	
	deleteQueue(buffer);
	free(mutex);
	free(canProduce);
	free(canConsume);
	free(producers);
	free(consumers);

	tArgs->mutex = NULL;
	tArgs->canConsume = NULL;
	tArgs->canProduce = NULL;

	printf("All threads finished.\n");
	printf("Produced: %zu\nConsumed: %zu\n\n", tArgs->num_produced, tArgs->num_consumed);
	return 0;
}//forkAndJoin

unsigned char readLogFiles(const int max_p_log_line, const int max_c_log_line){
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
