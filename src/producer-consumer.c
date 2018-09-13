#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include "queue.h"
#include "producer.h"
#include "consumer.h"
#include "argstructs.h"


//Global variables belonging to this source file
Queue *buffer;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t canProduce = PTHREAD_COND_INITIALIZER, canConsume = PTHREAD_COND_INITIALIZER;

int main(int argc, char *argv[]){
    unsigned i, numProducers = 0, numConsumers = 0, useProducerLog = 1, useConsumerLog = 1;
    unsigned long argCheck;
    pthread_t *producers, *consumers;
    char *lineBuffer = NULL;
    size_t lineBufferSize = 50;
    FILE *producerLog, *consumerLog;
    
    producer_args *pArgs = malloc(sizeof(*pArgs));
    consumer_args *cArgs = malloc(sizeof(*cArgs));
    if(pArgs == NULL || cArgs == NULL){
	fprintf(stderr, "Failed to allocate memory for thread arguments.\n");
	exit(1);
    }
    pArgs->num_produced = 0;
    pArgs->useProducerLogPtr = &useProducerLog;//useProducerLog is a local variable because main() needs to check its value several times after the producer threads finish
    cArgs->num_consumed = 0;
    cArgs->useConsumerLogPtr = &useConsumerLog;//useConsumerLog is a local variable because main() needs to check its value several times after the consumer threads finish
    
    if(argc != 5){//check for correct number of arguments
	printf("Usage: %s <# producer threads> #consumer threads> <buffer size> <# items to produce>\n", argv[0]);
	exit(1);
    }
    for(i = 1; i < 5; i++){//Ensure all provided arguments are valid
	argCheck = strtoul(argv[i], NULL, 10);
	if(argCheck < 1 || argCheck >= UINT_MAX){//makes sure that all arguments can safely be cast to unsigned integers
	    printf("argument %u (\'%s\') not valid. Please provide a positive integer no greater than %u.\n",
		    i, argv[i], UINT_MAX - 1);
	    printf("Usage: %s <# producer threads> #consumer threads> <buffer size> <# items to produce>\n", argv[0]);
	    exit(1);
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
		if(buffer == NULL){
		    fprintf(stderr, "Failed to allocate memory for buffer.\n");
		    exit(1);
		}
		break;
	    case 4:
		//the loop has reached its final iteration, so the value of argCheck will not change for the remainder of the program
		pArgs->target = (unsigned*)&argCheck;
		cArgs->target = (unsigned*)&argCheck;
		break;
	}
    }
    
    
    producerLog = fopen("producer-event.log", "w+");
    if(producerLog == NULL){
	fprintf(stderr, "Unable to open producer-event.log for writing. Proceeding without producer event logging.\n");
	fclose(producerLog);
	useProducerLog = 0;
    }
    else{
	pArgs->producerLog = producerLog;
    }
    
    consumerLog = fopen("consumer-event.log", "w+");
    if(consumerLog == NULL){
	fprintf(stderr, "Unable to open consumer-event.log for writing. Proceeding without consumer event logging.\n");
	fclose(consumerLog);
	useConsumerLog = 0;
    }
    else{
	cArgs->consumerLog = consumerLog;
    }
    
    producers = calloc(numProducers, sizeof(*producers));//See producer.c for the implementation of the producer function
    if(producers == NULL){//Check whether memory was allocated
	fprintf(stderr, "Failed to allocate memory for producer threads.\n");
	exit(1);
    }
    for(i = 0; i < numProducers; i++){
	pthread_create(&producers[i], NULL, producer, pArgs);
    }
    
    consumers = calloc(numConsumers, sizeof(*consumers));//See consumer.c for the implementation of the consumer function
    if(consumers == NULL){
	fprintf(stderr, "Failed to allocate memory for consumer threads.\n");
	exit(1);
    }
    for(i = 0; i < numConsumers; i++){
	pthread_create(&consumers[i], NULL, consumer, cArgs);
    }
    
    for(i = 0; i < numProducers; i++){
	pthread_join(producers[i], NULL);
    }
    free(producers);
    if(useProducerLog){
	fclose(producerLog);
    }
    
    for(i = 0; i < numConsumers; i++){
	pthread_join(consumers[i], NULL);
    }
    free(consumers);
    if(useConsumerLog){
	fclose(consumerLog);
    }
    
    buffer = deleteQueue(buffer);//deleteQueue returns a NULL pointer
    printf("All threads finished.\n");
    printf("Produced: %u\nConsumed: %u\n\n", pArgs->num_produced, cArgs->num_consumed);
    free(pArgs);
    free(cArgs);
    if(useProducerLog || useConsumerLog){
	lineBuffer = calloc(lineBufferSize, sizeof(*lineBuffer));
	if(lineBuffer == NULL){
	    fprintf(stderr, "Failed to allocate memory needed to read log files.\n");
	    printf("Done.\n");
	    return 1;
	}
    }
    
    if(useProducerLog){
	producerLog = fopen("producer-event.log", "r");
	if(producerLog == NULL){
	    fprintf(stderr, "Unable to read from producer-event.log.\n");
	}
	else{
	    printf("Reading from producer-event.log:\n");
	    while(getline(&lineBuffer, &lineBufferSize, producerLog) != -1){
		printf("%s", lineBuffer);
	    }
	    printf("End of producer-event.log.\n\n");
	}
	fclose(producerLog);
    }
    else{
	fprintf(stderr, "\nproducer-event.log was not written to, so it will not be read.\n");
    }

    if(useConsumerLog){
	consumerLog = fopen("consumer-event.log", "r");
	if(consumerLog == NULL){
	    fprintf(stderr, "Unable to read from consumer-event.log.\n");
	}
	else{
	    printf("Reading from consumer-event.log:\n");
	    while(getline(&lineBuffer, &lineBufferSize, consumerLog) != -1){
		printf("%s", lineBuffer);
	    }
	    printf("End of consumer-event.log.\n\n");
	}
	fclose(consumerLog);
    }
    else{
	fprintf(stderr, "\nconsumer-event.log was not written to, so it will not be read.\n");
    }
    
    if(lineBuffer != NULL){
	free(lineBuffer);
    }
    
    printf("Done.\n");
    return 0;
}
