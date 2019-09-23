#define _GNU_SOURCE

#include "macrodefs.h"
#include "queue.h"
#include "threaded_functions.h"
#include "argstruct.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include <errno.h>

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
	if(tArgs == NULL)
		return 1;

	tArgs->producerLog = fopen(PRODUCER_LOG_FILENAME, "w");
	if(tArgs->producerLog == NULL)
		fprintf(stderr, "Unable to open "PRODUCER_LOG_FILENAME" for writing. Proceeding without producer event logging.\n");

	tArgs->consumerLog = fopen(CONSUMER_LOG_FILENAME, "w");
	if(tArgs->consumerLog == NULL)
		fprintf(stderr, "Unable to open "CONSUMER_LOG_FILENAME" for writing. Proceeding without consumer event logging.\n");

	if(forkAndJoin(numProducers, numConsumers, tArgs))
		return EXIT_FAILURE;
	int max_p_log_line = tArgs->max_p_log_line;
	int max_c_log_line = tArgs->max_c_log_line;
	free(tArgs);
	return readLogFiles(max_p_log_line, max_c_log_line);
}//main


struct pc_thread_args* checkArguments(const char *restrict argv[], size_t *numProducers, size_t *numConsumers){
#ifdef SUPPORTS_RLIM
	struct rlimit rlim;
#endif
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
				if(buffer == NULL){
					fprintf(stderr, "Failed to allocate memory for buffer.\n");
					return NULL;
				}
				continue;
			case 4:
				tArgs = malloc(sizeof *tArgs);
				if(tArgs != NULL){
					tArgs->target = argCheck;
				}
				else{
					fprintf(stderr, "Failed to allocate memory for thread arguments.\n");
					return NULL;
				}
		}
	}
#ifdef SUPPORTS_RLIM
	prlimit(0, RLIMIT_NPROC, NULL, &rlim);//Stores the soft and hard limits for the number of threads that the invoking user is permitted to have running
	if(*numProducers + *numConsumers >= rlim.rlim_max){//If the total number of threads the user requested exceeds the hard limit, don't bother going any further
		fprintf(stderr, "The number of threads you wish to create exceeds the hard limit for the number of threads that can be created by the current user.\n");
		return NULL;
	}
	else if(*numProducers + *numConsumers > rlim.rlim_cur){//Check if the soft limit should be increased
		rlim.rlim_cur = rlim.rlim_max;
		prlimit(0, RLIMIT_NPROC, &rlim, NULL);
	}
#endif
	tArgs->num_produced = 0;
	tArgs->num_consumed = 0;
	return tArgs;
}//checkArguments

unsigned char forkAndJoin(const size_t numProducers, const size_t numConsumers, struct pc_thread_args *const tArgs){
	pthread_attr_t *tAttrs = malloc(sizeof *tAttrs);

	if(!tAttrs || pthread_attr_init(tAttrs))//pthread_attr_init() returns nozero on error
		fprintf(stderr, "Failed to initialize thread attributes, proceeding with defaults.\n");
	else
		pthread_attr_setstacksize(tAttrs, 16384);//set minimum stack size for threads to 16 KiB

	pthread_mutex_t *mutex = malloc(sizeof *mutex);
	if(!mutex || pthread_mutex_init(mutex, NULL)){
		free(mutex);
		fprintf(stderr, "Failed to initialize mutex.\n");
		return 1;
	}

	pthread_cond_t *canProduce = malloc(sizeof *canProduce);
	pthread_cond_t *canConsume = malloc(sizeof *canConsume);
	if(!canProduce || !canConsume || pthread_cond_init(canProduce, NULL) || pthread_cond_init(canConsume, NULL)){
		free(mutex);
		free(canProduce);
		free(canConsume);
		fprintf(stderr, "Failed to initialize condition variables.\n");
		return 1;
	}
	tArgs->mutex = mutex;
	tArgs->canConsume = canConsume;
	tArgs->canProduce = canProduce;

	pthread_t *const producers = calloc(numProducers, sizeof *producers);//See producer.c for the implementation of the producer function
	if(producers == NULL){//Check whether memory was allocated
		fprintf(stderr, "Failed to allocate memory for producer threads.\n");
		return 1;
	}

	pthread_t *const consumers = calloc(numConsumers, sizeof *consumers);//See consumer.c for the implementation of the consumer function
	if(consumers == NULL){
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
			if(pthread_create(&producers[i], tAttrs, producer, tArgs)){//Check for a nonzero return value, which indicates an error
				pthread_mutex_lock(mutex);
				fprintf(stderr, "Unable to create the requested number of producer threads.\n");
				return 1;
			}

			if(pthread_create(&consumers[i], tAttrs, consumer, tArgs)){//Check for a nonzero return value, which indicates an error
				pthread_mutex_lock(mutex);
				fprintf(stderr, "Unable to create the requested number of consumer threads.\n");
				return 1;
			}
		}

		for(; i < numProducers; ++i){
			if(pthread_create(&producers[i], tAttrs, producer, tArgs)){//Check for a nonzero return value, which indicates an error
				pthread_mutex_lock(mutex);
				fprintf(stderr, "Unable to create the requested number of producer threads.\n");
				return 1;
			}
		}

		if(tAttrs != NULL)
			pthread_attr_destroy(tAttrs);
		free(tAttrs);

		for(i = 0; i < numConsumers; ++i){
			pthread_join(producers[i], NULL);
			pthread_join(consumers[i], NULL);
		}

		for(; i < numProducers; ++i)
			pthread_join(producers[i], NULL);

	}
	else{
		size_t i;
		for(i = 0; i < numProducers; ++i){
			if(pthread_create(&producers[i], tAttrs, producer, tArgs)){//Check for a nonzero return value, which indicates an error
				pthread_mutex_lock(mutex);
				fprintf(stderr, "Unable to create the requested number of producer threads.\n");
				return 1;
			}

			if(pthread_create(&consumers[i], tAttrs, consumer, tArgs)){//Check for a nonzero return value, which indicates an error
				pthread_mutex_lock(mutex);
				fprintf(stderr, "Unable to create the requested number of consumer threads.\n");
				return 1;
			}
		}

		for(; i < numConsumers; ++i){
			if(pthread_create(&consumers[i], tAttrs, consumer, tArgs)){//Check for a nonzero return value, which indicates an error
				pthread_mutex_lock(mutex);
				fprintf(stderr, "Unable to create the requested number of consumer threads.\n");
				return 1;
			}
		}

		if(tAttrs != NULL)
			pthread_attr_destroy(tAttrs);
		free(tAttrs);

		for(i = 0; i < numProducers; ++i){
			pthread_join(producers[i], NULL);
			pthread_join(consumers[i], NULL);
		}

		for(; i < numConsumers; ++i)
			pthread_join(consumers[i], NULL);
	}//end of else

	if(tArgs->producerLog != NULL)
		fclose(tArgs->producerLog);
	if(tArgs->consumerLog != NULL)
		fclose(tArgs->consumerLog);
	pthread_mutex_destroy(mutex);
	pthread_cond_destroy(canProduce);
	pthread_cond_destroy(canConsume);
	
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
	struct log_thread_args producerlog_args, consumerlog_args;
	pthread_t producerlog_thread, consumerlog_thread;
	pthread_mutex_t *mutex = malloc(sizeof *mutex);
	pthread_attr_t *tAttrs = malloc(sizeof *tAttrs);

	if(!tAttrs || pthread_attr_init(tAttrs)){//pthread_attr_init() returns nozero on error
		free(tAttrs);
		tAttrs = NULL;
		fprintf(stderr, "Failed to initialize thread attributes. Log files will be read in threads with default attributes.\n");
	}
	else{
		pthread_attr_setstacksize(tAttrs, 16384);//set minimum stack size for threads to 16 KiB
	}

	if(!mutex || pthread_mutex_init(mutex, NULL)){//pthread_mutex_init() returns a nonzero int upon failure
		free(mutex);
		free(tAttrs);
		fprintf(stderr, "Failed to initialize mutex. Log files will not be read.\n");
		return 1;
	}
	producerlog_args.mutex = mutex;
	consumerlog_args.mutex = mutex;
	producerlog_args.max_log_line = max_p_log_line;
	consumerlog_args.max_log_line = max_c_log_line;

	pthread_create(&producerlog_thread, tAttrs, producer_log_reader, &producerlog_args);
	pthread_create(&consumerlog_thread, tAttrs, consumer_log_reader, &consumerlog_args);

	if(tAttrs)
		pthread_attr_destroy(tAttrs);
	free(tAttrs);

	pthread_join(producerlog_thread, NULL);
	pthread_join(consumerlog_thread, NULL);
	pthread_mutex_destroy(mutex);
	free(mutex);

	return !consumerlog_args.ret && !producerlog_args.ret;//Return 0 if both rets are 0
}//readLogFiles
