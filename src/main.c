#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#include "macrodefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include <errno.h>
#include "queue.h"
#include "threaded_functions.h"
#include "argstruct.h"

Queue * buffer;

static void checkArguments(char *argv[], pc_thread_args * restrict tArgs, size_t *numProducers, size_t *numConsumers);
static void forkAndJoin(const size_t *restrict numProducers, const size_t *restrict numConsumers, pc_thread_args *tArgs);
static void readLogFiles(register FILE *restrict producerLog, register FILE *restrict consumerLog, const int max_p_log_line, const int max_c_log_line);

int main(int argc, char *argv[]){
    FILE *producerLog, *consumerLog;
    pc_thread_args * restrict tArgs;
    size_t numProducers = 0, numConsumers = 0;
    unsigned char max_p_log_line, max_c_log_line;

    if(argc != 5){//check for correct number of arguments
	printf("Usage: %s <# producer threads> <# consumer threads> <buffer size> <# items to produce>\n", argv[0]);
	return 1;
    }

    tArgs = malloc(sizeof(*tArgs));
    if(tArgs == NULL){
	fprintf(stderr, "Failed to allocate memory for thread arguments.\n");
	return 1;
    }

    checkArguments(argv, tArgs, &numProducers, &numConsumers);

    producerLog = fopen(PRODUCER_LOG_FILENAME, "w");
    tArgs->producerLog = producerLog;
    if(producerLog == NULL)
	fprintf(stderr, "Unable to open "PRODUCER_LOG_FILENAME" for writing. Proceeding without producer event logging.\n");

    consumerLog = fopen(CONSUMER_LOG_FILENAME, "w");
    tArgs->consumerLog = consumerLog;
    if(consumerLog == NULL)
	fprintf(stderr, "Unable to open "CONSUMER_LOG_FILENAME" for writing. Proceeding without consumer event logging.\n");

    forkAndJoin(&numProducers, &numConsumers, tArgs);
    max_p_log_line = tArgs->max_p_log_line;
    max_c_log_line = tArgs->max_c_log_line;
    free(tArgs);
    tArgs = NULL;
    readLogFiles(producerLog, consumerLog, max_p_log_line, max_c_log_line);
}//main


static void checkArguments(char *argv[], pc_thread_args * restrict tArgs, size_t *numProducers, size_t *numConsumers){
#ifdef SUPPORTS_RLIM
    struct rlimit rlim;
#endif
    size_t argCheck;
    unsigned char i;

    for(i = 1; i < 5; ++i){//Ensure all provided arguments are valid
	argCheck = strtoumax(argv[i], NULL, 10);
	if(argCheck == 0 || errno == ERANGE){
	    fprintf(stderr, "argument %u (\'%s\') not valid. Please provide a positive integer no greater than %zu.\n",
		    i, argv[i], SIZE_MAX);
	    printf("Usage: %s <# producer threads> <# consumer threads> <buffer size> <# items to produce>\n", argv[0]);
	    exit(1);
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
		    exit(1);
		}
		continue;
	    case 4:
		tArgs->target = argCheck;
	}
    }
#ifdef SUPPORTS_RLIM
    prlimit(0, RLIMIT_NPROC, NULL, &rlim);//Stores the soft and hard limits for the number of threads that the invoking user is permitted to have running
    if(*numProducers + *numConsumers >= rlim.rlim_max){//If the total number of threads the user requested exceeds the hard limit, don't bother going any further
	fprintf(stderr, "The number of threads you wish to create exceeds the hard limit for the number of threads that can be created by the current user.\n");
	exit(1);
    }
    else if(*numProducers + *numConsumers > rlim.rlim_cur ){//Check if the soft limit should be increased
	rlim.rlim_cur = rlim.rlim_max;
	prlimit(0, RLIMIT_NPROC, &rlim, NULL);
    }
#endif
    tArgs->num_produced = 0;
    tArgs->num_consumed = 0;
}//checkArguments

static void forkAndJoin(const size_t *restrict numProducers, const size_t *restrict numConsumers, pc_thread_args *tArgs){
    pthread_mutex_t mutex;
    pthread_cond_t canProduce, canConsume;
    pthread_attr_t tAttrs;//thread attributes
    pthread_t *producers, *consumers;
    size_t i;


#ifdef __linux__//pthread_attr_init() always succeeds on Linux
    pthread_attr_init(&tAttrs);
    pthread_attr_setstacksize(&tAttrs, 32768);//set minimum stack size for threads to 32 KiB
#else
    if(pthread_attr_init(&tAttrs)){//pthread_attr_init() returns nozero on error
	fprintf(stderr, "Failed to initialize thread attributes, proceeding with defaults.\n");
	&tAttrs = NULL;
    }
    else{
	pthread_attr_setstacksize(&tAttrs, 32768);//set minimum stack size for threads to 32 KiB
    }
#endif

    if(pthread_mutex_init(&mutex, NULL)){//pthread_mutex_init() returns a nonzero int upon failure
	fprintf(stderr, "Failed to initialize mutex.\n");
	exit(1);
    }

    if(pthread_cond_init(&canProduce, NULL) || pthread_cond_init(&canConsume, NULL)){//pthread_cond_init() returns a nonzero int upon failure
	fprintf(stderr, "Failed to initialize condition variables.\n");
	exit(1);
    }
    tArgs->mutex = &mutex;
    tArgs->canConsume = &canConsume;
    tArgs->canProduce = &canProduce;

    producers = calloc(*numProducers, sizeof(*producers));//See producer.c for the implementation of the producer function
    if(producers == NULL){//Check whether memory was allocated
	fprintf(stderr, "Failed to allocate memory for producer threads.\n");
	exit(1);
    }

    consumers = calloc(*numConsumers, sizeof(*consumers));//See consumer.c for the implementation of the consumer function
    if(consumers == NULL){
	fprintf(stderr, "Failed to allocate memory for consumer threads.\n");
	exit(1);
    }
    
    srand(time(NULL));
    tArgs->max_p_log_line = 0;
    tArgs->max_c_log_line = 0;
    if(*numProducers >= *numConsumers){
	for(i = 0; i < *numConsumers; ++i){
	    if(pthread_create(&producers[i], &tAttrs, producer, tArgs)){//Check for a nonzero return value, which indicates an error
		fprintf(stderr, "Unable to create the requested number of producer threads.\n");
		exit(1);
	    }

	    if(pthread_create(&consumers[i], &tAttrs, consumer, tArgs)){//Check for a nonzero return value, which indicates an error
		fprintf(stderr, "Unable to create the requested number of consumer threads.\n");
		exit(1);
	    }
	}
	
	for(; i < *numProducers; ++i){
	    if(pthread_create(&producers[i], &tAttrs, producer, tArgs)){//Check for a nonzero return value, which indicates an error
		fprintf(stderr, "Unable to create the requested number of producer threads.\n");
		exit(1);
	    }
	}
	
#ifndef __linux__
	if(&tAttrs != NULL)
	    pthread_attr_destroy(&tAttrs);
#else
	pthread_attr_destroy(&tAttrs);
#endif
	
	
	for(i = 0; i < *numConsumers; ++i){
	    pthread_join(producers[i], NULL);
	    pthread_join(consumers[i], NULL);
	}
	free(consumers);
	
	if(tArgs->consumerLog != NULL)
	    fclose(tArgs->consumerLog);
	
	for(; i < *numProducers; ++i)
	    pthread_join(producers[i], NULL);
	free(producers);
	
	if(tArgs->producerLog != NULL)
	    fclose(tArgs->producerLog);
    }
    else{
	for(i = 0; i < *numProducers; ++i){
	    if(pthread_create(&producers[i], &tAttrs, producer, tArgs)){//Check for a nonzero return value, which indicates an error
		fprintf(stderr, "Unable to create the requested number of producer threads.\n");
		exit(1);
	    }

	    if(pthread_create(&consumers[i], &tAttrs, consumer, tArgs)){//Check for a nonzero return value, which indicates an error
		fprintf(stderr, "Unable to create the requested number of consumer threads.\n");
		exit(1);
	    }
	}
	
	for(; i < *numConsumers; ++i){
	    if(pthread_create(&consumers[i], &tAttrs, consumer, tArgs)){//Check for a nonzero return value, which indicates an error
		fprintf(stderr, "Unable to create the requested number of consumer threads.\n");
		exit(1);
	    }
	}
	
#ifndef __linux__
	if(&tAttrs != NULL)
	    pthread_attr_destroy(&tAttrs);
#else
	pthread_attr_destroy(&tAttrs);
#endif
	
	for(i = 0; i < *numProducers; ++i){
	    pthread_join(producers[i], NULL);
	    pthread_join(consumers[i], NULL);
	}
	free(producers);
	
	if(tArgs->producerLog != NULL)
	    fclose(tArgs->producerLog);
	
	for(; i < *numConsumers; ++i)
	    pthread_join(consumers[i], NULL);
	free(consumers);
	
	if(tArgs->consumerLog != NULL)
	    fclose(tArgs->consumerLog);
    }//end of else


    deleteQueue(buffer);//deleteQueue NULLs buffer
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&canProduce);
    pthread_cond_destroy(&canConsume);

    printf("All threads finished.\n");
    printf("Produced: %zu\nConsumed: %zu\n\n", tArgs->num_produced, tArgs->num_consumed);
}//forkAndJoin

static void readLogFiles(FILE *restrict producerLog, FILE *restrict consumerLog, const int max_p_log_line, const int max_c_log_line){
    producerlog_thread_args producerlog_args;
    consumerlog_thread_args consumerlog_args;
    pthread_t producerlog_thread, consumerlog_thread;
    pthread_mutex_t mutex;
    pthread_attr_t tAttrs;//thread attributes

#ifdef __linux__//pthread_attr_init() always succeeds on Linux
    pthread_attr_init(&tAttrs);
    pthread_attr_setstacksize(&tAttrs, 32768);//set minimum stack size for threads to 32 KiB
#else
    if(pthread_attr_init(&tAttrs)){//pthread_attr_init() returns nozero on error
	fprintf(stderr, "Failed to initialize thread attributes. Log files will be read in threads with default attributes.\n");
	&tAttrs = NULL;
    }
    else{
	pthread_attr_setstacksize(&tAttrs, 32768);//set minimum stack size for threads to 32 KiB
    }
#endif

    if(pthread_mutex_init(&mutex, NULL)){//pthread_mutex_init() returns a nonzero int upon failure
	fprintf(stderr, "Failed to initialize mutex. Log files will not be read.\n");
	exit(1);
    }
    producerlog_args.mutex = &mutex;
    consumerlog_args.mutex = &mutex;
    producerlog_args.producerLog = producerLog;
    consumerlog_args.consumerLog = consumerLog;
    producerlog_args.max_log_line = max_p_log_line;
    consumerlog_args.max_log_line = max_c_log_line;

    pthread_create(&producerlog_thread, &tAttrs, producer_log_reader, &producerlog_args);
    pthread_create(&consumerlog_thread, &tAttrs, consumer_log_reader, &consumerlog_args);
#ifndef __linux__
    if(&tAttrs != NULL)
	pthread_attr_destroy(&tAttrs);
#else
    pthread_attr_destroy(&tAttrs);
#endif

    pthread_join(producerlog_thread, NULL);
    pthread_join(consumerlog_thread, NULL);
    pthread_mutex_destroy(&mutex);

    exit(!consumerlog_args.ret && !producerlog_args.ret);//Return 0 if both rets are 0
}//readLogFiles
