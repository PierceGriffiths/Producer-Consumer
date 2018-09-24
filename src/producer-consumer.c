#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64

#if defined(__linux__)
#define IS_POSIX
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__)) //#if defined(__linux__)
#include <sys/param.h>
#ifdef BSD
#define IS_POSIX
#endif //#ifdef BSD
#endif //if defined(__linux__)

#ifndef IS_POSIX
#warning "This program is not likely to be compatible with your operating system."
#else
#include <sys/resource.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include "queue.h"
#include "producer.h"
#include "consumer.h"
#include "argstruct.h"

Queue * restrict buffer;

int main(int argc, char *argv[]){
    register unsigned i, numProducers = 0, numConsumers = 0;//register keyword tells compiler that it should try to optimize access to these variables
    unsigned long argCheck;
    char *lineBuffer = NULL;
    size_t lineBufferSize = 50;
    register FILE *producerLog, *consumerLog;//The addresses of these pointers themselves are never taken, so the use of the register keyword here is valid

    struct rlimit rlim;

    pthread_t * restrict producers, * restrict consumers;
    pthread_mutex_t mutex;
    pthread_cond_t canProduce, canConsume;
    pthread_attr_t tAttrs;//thread attributes
    thread_args * restrict tArgs = malloc(sizeof(*tArgs));

    if(tArgs == NULL){
	printf("Failed to allocate memory for thread arguments.\n");
	return -1;
    }

    if(pthread_mutex_init(&mutex, NULL)){//pthread_mutex_init() returns a nonzero int upon failure
	printf("Failed to initialize mutex.\n");
	return -1;
    }

    if(pthread_cond_init(&canProduce, NULL) || pthread_cond_init(&canConsume, NULL)){//pthread_cond_init() returns a nonzero int upon failure
	printf("Failed to initialize condition variables.\n");
	return -1;
    }

#ifdef __linux__//pthread_attr_init() always succeeds on Linux
    pthread_attr_init(&tAttrs);
#else
    if(pthread_attr_init(&tAttrs)){//pthread_attr_init() returns nozero on error
	printf("Failed to initialize thread attributes.\n");
	return -1;
    }
#endif

    pthread_attr_setstacksize(&tAttrs, 32768);//set minimum stack size for threads to 32 KiB

    tArgs->num_produced = 0;
    tArgs->num_consumed = 0;
    tArgs->mutex = &mutex;
    tArgs->canConsume = &canConsume;
    tArgs->canProduce = &canProduce;

    if(argc != 5){//check for correct number of arguments
	printf("Usage: %s <# producer threads> <#consumer threads> <buffer size> <# items to produce>\n", argv[0]);
	return -1;
    }
    for(i = 1; i < 5; ++i){//Ensure all provided arguments are valid
	argCheck = strtoul(argv[i], NULL, 10);
	if(argCheck == 0 || argCheck >= UINT_MAX){//makes sure that all arguments can safely be cast to unsigned integers
	    printf("argument %u (\'%s\') not valid. Please provide a positive integer no greater than %u.\n",
		    i, argv[i], UINT_MAX - 1);
	    printf("Usage: %s <# producer threads> <#consumer threads> <buffer size> <# items to produce>\n", argv[0]);
	    return -1;
	}
	switch(i){
	    case 1:
		numProducers = (unsigned)argCheck;
		break;
	    case 2:
		numConsumers = (unsigned)argCheck;
		break;
	    case 3:
		buffer = createQueue((unsigned)argCheck);
		if(buffer == NULL || buffer->array == NULL){
		    printf("Failed to allocate memory for buffer.\n");
		    return -1;
		}
		break;
	    case 4:
		tArgs->target = (unsigned)argCheck;
		break;
	}
    }
#ifdef IS_POSIX
    prlimit(0, RLIMIT_NPROC, NULL, &rlim);//Stores the soft and hard limits for the number of threads that the invoking user is permitted to have running
    if(numProducers + numConsumers >= rlim.rlim_max){//If the total number of threads the user requested exceeds the hard limit, don't bother going any further
	printf("The number of threads you wish to create exceeds the hard limit for the number of threads that can be created by the current user.\n");
	return -1;
    }
    else if(numProducers + numConsumers > rlim.rlim_cur ){//Check if the soft limit should be increased
	rlim.rlim_cur = rlim.rlim_max;
	prlimit(0, RLIMIT_NPROC, &rlim, NULL);
    }
#endif

    producerLog = fopen("producer-event.log", "w");
    tArgs->producerLog = producerLog;
    if(producerLog == NULL){
	printf("Unable to open producer-event.log for writing. Proceeding without producer event logging.\n");
    }

    consumerLog = fopen("consumer-event.log", "w");
    tArgs->consumerLog = consumerLog;
    if(consumerLog == NULL){
	printf("Unable to open consumer-event.log for writing. Proceeding without consumer event logging.\n");
    }

    producers = calloc(numProducers, sizeof(*producers));//See producer.c for the implementation of the producer function
    if(producers == NULL){//Check whether memory was allocated
	printf("Failed to allocate memory for producer threads.\n");
	return -1;
    }
    for(i = numProducers; i; ){
	if(pthread_create(&producers[--i], &tAttrs, producer, tArgs)){//Check for a nonzero return value, which indicates an error
	    printf("Unable to create the requested number of threads.\n");
	    return -1;
	}
    }

    consumers = calloc(numConsumers, sizeof(*consumers));//See consumer.c for the implementation of the consumer function
    if(consumers == NULL){
	printf("Failed to allocate memory for consumer threads.\n");
	return -1;
    }
    for(i = numConsumers; i; ){
	if(pthread_create(&consumers[--i], &tAttrs, consumer, tArgs)){//Check for a nonzero return value, which indicates an error
	    printf("Failed to create the requested number of threads.\n");
	    return -1;
	}
    }

    pthread_attr_destroy(&tAttrs);

    for(i = numProducers; i; ){
	pthread_join(producers[--i], NULL);
    }
    free(producers);
    if(producerLog != NULL){
	fclose(producerLog);
    }

    for(i = numConsumers; i; ){
	pthread_join(consumers[--i], NULL);
    }
    free(consumers);
    if(consumerLog != NULL){
	fclose(consumerLog);
    }
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&canProduce);
    pthread_cond_destroy(&canConsume);
    buffer = deleteQueue(buffer);//deleteQueue returns a NULL pointer


    printf("All threads finished.\n");
    printf("Produced: %u\nConsumed: %u\n\n", tArgs->num_produced, tArgs->num_consumed);
    free(tArgs);
    if(producerLog != NULL || consumerLog != NULL){
	lineBuffer = calloc(lineBufferSize, sizeof(*lineBuffer));
	if(lineBuffer == NULL){
	    printf("Failed to allocate memory needed to read log files.\n");
	    printf("Done.\n");
	    return 1;
	}
    }

    if(producerLog != NULL){
	producerLog = fopen("producer-event.log", "r");

	if(producerLog == NULL){
	    printf("Unable to read from producer-event.log.\n");
	}
	else{
	    printf("Reading from producer-event.log:\n");
	    while(getline(&lineBuffer, &lineBufferSize, producerLog) != -1){
		printf("%s", lineBuffer);
	    }

	    fclose(producerLog);
	    printf("End of producer-event.log.\n\n");
	}
    }
    else{
	printf("\nproducer-event.log was not written to, so it will not be read.\n");
    }

    if(consumerLog != NULL){
	consumerLog = fopen("consumer-event.log", "r");

	if(consumerLog == NULL){
	    printf("Unable to read from consumer-event.log.\n");
	}
	else{
	    printf("Reading from consumer-event.log:\n");
	    while(getline(&lineBuffer, &lineBufferSize, consumerLog) != -1){
		printf("%s", lineBuffer);
	    }

	    fclose(consumerLog);
	    printf("End of consumer-event.log.\n\n");
	}
    }
    else{
	printf("\nconsumer-event.log was not written to, so it will not be read.\n");
    }

    free(lineBuffer);//if lineBuffer is NULL, no action will be performed

    printf("Done.\n");
    return 0;
}
