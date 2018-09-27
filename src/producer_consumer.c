#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#if defined(__linux__)
#define IS_POSIX
#define SUPPORTS_RLIM
#elif defined(__unix__)
#define IS_POSIX
#include <sys/param.h>
#ifdef BSD
#define SUPPORTS_RLIM
#endif //#ifdef BSD
#elif defined(__APPLE__) && defined(__MACH__)
#define IS_POSIX
#include <TargetConditionals.h>
#if TARGET_OS_MAC == 1
#define SUPPORTS_RLIM
#endif //if TARGET_OS_MAC == 1
#endif //if defined(__linux__)

#ifndef IS_POSIX
#warning "This program is not likely to be compatible with your operating system."
#elif defined(SUPPORTS_RLIM)
#include <sys/resource.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include "queue.h"
#include "threaded_functions.h"
#include "argstruct.h"

Queue * restrict buffer;

static void checkArguments(char *argv[], pc_thread_args * restrict tArgs, unsigned *numProducers, unsigned *numConsumers);
static void forkAndJoin(const unsigned *numProducers, const unsigned *numConsumers, pc_thread_args *tArgs);
static void readLogFiles(register FILE *producerLog, register FILE *consumerLog);

int main(int argc, char *argv[]){
    unsigned numProducers = 0, numConsumers = 0;
    register FILE *producerLog, *consumerLog;//The addresses of these pointers themselves are never taken, so the use of the register keyword here is valid
    pc_thread_args * restrict tArgs;

    if(argc != 5){//check for correct number of arguments
	printf("Usage: %s <# producer threads> <#consumer threads> <buffer size> <# items to produce>\n", argv[0]);
	return -1;
    }

    tArgs = malloc(sizeof(*tArgs));
    if(tArgs == NULL){
	printf("Failed to allocate memory for thread arguments.\n");
	return -1;
    }

    checkArguments(argv, tArgs, &numProducers, &numConsumers);

    producerLog = fopen(PRODUCER_LOG_FILENAME, "w");
    tArgs->producerLog = producerLog;
    if(producerLog == NULL)
	printf("Unable to open %s for writing. Proceeding without producer event logging.\n", PRODUCER_LOG_FILENAME);

    consumerLog = fopen(CONSUMER_LOG_FILENAME, "w");
    tArgs->consumerLog = consumerLog;
    if(consumerLog == NULL)
	printf("Unable to open %s for writing. Proceeding without consumer event logging.\n", CONSUMER_LOG_FILENAME);

    forkAndJoin(&numProducers, &numConsumers, tArgs);
    readLogFiles(producerLog, consumerLog);
}//main

static void checkArguments(char *argv[], pc_thread_args * restrict tArgs, unsigned *numProducers, unsigned *numConsumers){
    unsigned long argCheck;
    int i;
#ifdef SUPPORTS_RLIM
    struct rlimit rlim;
#endif


    for(i = 1; i < 5; ++i){//Ensure all provided arguments are valid
	argCheck = strtoul(argv[i], NULL, 10);
	if(argCheck == 0 || argCheck >= UINT_MAX){//makes sure that all arguments can safely be cast to unsigned integers
	    printf("argument %u (\'%s\') not valid. Please provide a positive integer no greater than %u.\n",
		    i, argv[i], UINT_MAX - 1);
	    printf("Usage: %s <# producer threads> <#consumer threads> <buffer size> <# items to produce>\n", argv[0]);
	    exit(-1);
	}
	switch(i){
	    case 1:
		*numProducers = (unsigned)argCheck;
		continue;
	    case 2:
		*numConsumers = (unsigned)argCheck;
		continue;
	    case 3:
		buffer = createQueue((unsigned)argCheck);
		if(buffer == NULL){
		    printf("Failed to allocate memory for buffer.\n");
		    exit(-1);
		}
		continue;
	    case 4:
		tArgs->target = (unsigned)argCheck;
	}
    }
#ifdef SUPPORTS_RLIM
    prlimit(0, RLIMIT_NPROC, NULL, &rlim);//Stores the soft and hard limits for the number of threads that the invoking user is permitted to have running
    if(*numProducers + *numConsumers >= rlim.rlim_max){//If the total number of threads the user requested exceeds the hard limit, don't bother going any further
	printf("The number of threads you wish to create exceeds the hard limit for the number of threads that can be created by the current user.\n");
	exit(-1);
    }
    else if(*numProducers + *numConsumers > rlim.rlim_cur ){//Check if the soft limit should be increased
	rlim.rlim_cur = rlim.rlim_max;
	prlimit(0, RLIMIT_NPROC, &rlim, NULL);
    }
#endif
    tArgs->num_produced = 0;
    tArgs->num_consumed = 0;
}//checkArguments

static void forkAndJoin(const unsigned *numProducers, const unsigned *numConsumers, pc_thread_args *tArgs){
    unsigned i;
    pthread_mutex_t mutex;
    pthread_cond_t canProduce, canConsume;
    pthread_t *producers, *consumers;
    pthread_attr_t tAttrs;//thread attributes


#ifdef __linux__//pthread_attr_init() always succeeds on Linux
    pthread_attr_init(&tAttrs);
#else
    if(pthread_attr_init(&tAttrs)){//pthread_attr_init() returns nozero on error
	printf("Failed to initialize thread attributes.\n");
	exit(-1);
    }
#endif

    if(pthread_mutex_init(&mutex, NULL)){//pthread_mutex_init() returns a nonzero int upon failure
	printf("Failed to initialize mutex.\n");
	exit(-1);
    }

    if(pthread_cond_init(&canProduce, NULL) || pthread_cond_init(&canConsume, NULL)){//pthread_cond_init() returns a nonzero int upon failure
	printf("Failed to initialize condition variables.\n");
	exit(-1);
    }
    tArgs->mutex = &mutex;
    tArgs->canConsume = &canConsume;
    tArgs->canProduce = &canProduce;
    pthread_attr_setstacksize(&tAttrs, 32768);//set minimum stack size for threads to 32 KiB

    producers = calloc(*numProducers, sizeof(*producers));//See producer.c for the implementation of the producer function
    if(producers == NULL){//Check whether memory was allocated
	printf("Failed to allocate memory for producer threads.\n");
	exit(-1);
    }

    consumers = calloc(*numConsumers, sizeof(*consumers));//See consumer.c for the implementation of the consumer function
    if(consumers == NULL){
	printf("Failed to allocate memory for consumer threads.\n");
	exit(-1);
    }

    for(i = *numProducers; i; ){
	if(pthread_create(&producers[--i], &tAttrs, producer, tArgs)){//Check for a nonzero return value, which indicates an error
	    printf("Unable to create the requested number of threads.\n");
	    exit(1);
	}
    }

    for(i = *numConsumers; i; ){
	if(pthread_create(&consumers[--i], &tAttrs, consumer, tArgs)){//Check for a nonzero return value, which indicates an error
	    printf("Failed to create the requested number of threads.\n");
	    exit(-1);
	}
    }
    pthread_attr_destroy(&tAttrs);

    for(i = *numProducers; i; )
	pthread_join(producers[--i], NULL);
    free(producers);

    if(tArgs->producerLog != NULL)
	fclose(tArgs->producerLog);

    for(i = *numConsumers; i; )
	pthread_join(consumers[--i], NULL);
    free(consumers);

    if(tArgs->consumerLog != NULL)
	fclose(tArgs->consumerLog);

    buffer = deleteQueue(buffer);//deleteQueue returns a NULL pointer
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&canProduce);
    pthread_cond_destroy(&canConsume);

    printf("All threads finished.\n");
    printf("Produced: %u\nConsumed: %u\n\n", tArgs->num_produced, tArgs->num_consumed);
    free(tArgs);
    tArgs = NULL;
}//forkAndJoin

static void readLogFiles(register FILE *producerLog, register FILE *consumerLog){
    producerlog_thread_args producerlog_args;
    consumerlog_thread_args consumerlog_args;
    pthread_t producerlog_thread, consumerlog_thread;
    pthread_mutex_t mutex;
    pthread_attr_t tAttrs;//thread attributes

#ifdef __linux__//pthread_attr_init() always succeeds on Linux
    pthread_attr_init(&tAttrs);
#else
    if(pthread_attr_init(&tAttrs)){//pthread_attr_init() returns nozero on error
	printf("Failed to initialize thread attributes. Log files will not be read.\n");
	 exit(-1);
    }
#endif

    if(pthread_mutex_init(&mutex, NULL)){//pthread_mutex_init() returns a nonzero int upon failure
	printf("Failed to initialize mutex. Log files will not be read.\n");
	exit(-1);
    }
    producerlog_args.mutex = &mutex;
    consumerlog_args.mutex = &mutex;
    producerlog_args.producerLog = producerLog;
    consumerlog_args.consumerLog = consumerLog;

    pthread_attr_setstacksize(&tAttrs, 32768);//set minimum stack size for threads to 32 KiB

    pthread_create(&producerlog_thread, &tAttrs, producer_log_reader, &producerlog_args);
    pthread_create(&consumerlog_thread, &tAttrs, consumer_log_reader, &consumerlog_args);
    pthread_attr_destroy(&tAttrs);

    pthread_join(producerlog_thread, NULL);
    pthread_join(consumerlog_thread, NULL);
    pthread_mutex_destroy(&mutex);

    if(!consumerlog_args.ret && !producerlog_args.ret)
	exit(0);//Return 0 if both rets are 0
    else if(consumerlog_args.ret == producerlog_args.ret)
	exit(producerlog_args.ret);//return either ret if they're equal
    else if(!producerlog_args.ret)
	exit(consumerlog_args.ret);//return the nonzero consumerlog_args.ret if producerlog_args.ret is 0
    else if(!consumerlog_args.ret)
	exit(producerlog_args.ret);//return the nonzero producerlog_args.ret if consumerlog_args.ret is 0
    else
	exit(-1);//One ret is -1 and the other is 1, so return -1
}//readLogFiles
