#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include <queue.h>
#include <producer.h>
#include <consumer.h>

//Global variables belonging to this source file
FILE *producerLog, *consumerLog;
Queue *buffer;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t canProduce = PTHREAD_COND_INITIALIZER, canConsume = PTHREAD_COND_INITIALIZER;
unsigned num_consumed = 0, num_produced = 0, target;
short useProducerLog = 1, useConsumerLog = 1;

int main(int argc, char *argv[]){
    unsigned i, numProducers, numConsumers;
    unsigned long argCheck;
    pthread_t *producers, *consumers;
    char *lineBuffer = NULL;
    size_t lineBufferSize = 50;
    
    if(argc != 5){//check for correct number of arguments
	printf("Usage: %s <# producer threads> #consumer threads> <buffer size> <# items to produce>\n", argv[0]);
	exit(1);
    }
    for(i = 1; i < 5; i++){//Ensure all provided arguments are valid
	argCheck = strtoul(argv[i], NULL, 10);
	if(argCheck < 1 || argCheck >= UINT_MAX){
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
		target = (unsigned)argCheck;
		break;
	}
    }
    
    
    producerLog = fopen("producer-event.log", "w+");
    if(producerLog == NULL){
	fprintf(stderr, "Unable to open producer-event.log for writing. Proceeding without producer event logging.\n");
	fclose(producerLog);
	useProducerLog = 0;
    }
    
    consumerLog = fopen("consumer-event.log", "w+");
    if(consumerLog == NULL){
	fprintf(stderr, "Unable to open consumer-event.log for writing. Proceeding without consumer event logging.\n");
	fclose(consumerLog);
	useConsumerLog = 0;
    }
    
    //Using calloc here wastes CPU cycles because the memory is zeroed before the pointer is returned
    producers = malloc(numProducers * sizeof(*producers));//See producer.c for the implementation of the producer function
    if(producers == NULL){//Check whether memory was allocated
	fprintf(stderr, "Failed to allocate memory for producer threads.\n");
	exit(1);
    }
    else{
	for(i = 0; i < numProducers; i++){
	    pthread_create(&producers[i], NULL, producer, NULL);
	}
    }
    
    consumers = malloc(numConsumers * sizeof(*consumers));//See consumer.c for the implementation of the consumer function
    if(consumers == NULL){
	fprintf(stderr, "Failed to allocate memory for consumer threads.\n");
	exit(1);
    }
    else{
	for(i = 0; i < numConsumers; i++){
	    pthread_create(&consumers[i], NULL, consumer, NULL);
	}
    }
    
    for(i = 0; i < numProducers; i++){
	pthread_join(producers[i], NULL);
    }
    free(producers);
    fclose(producerLog);
    for(i = 0; i < numConsumers; i++){
	pthread_join(consumers[i], NULL);
    }
    fclose(consumerLog);
    free(consumers);
    deleteQueue(buffer);
    printf("All threads finished.\n");
    printf("Produced: %u\nConsumed: %u\n\n", num_produced, num_consumed);
    
    if(useProducerLog || useConsumerLog){
	lineBuffer = calloc(lineBufferSize, sizeof(*lineBuffer));//Use calloc so that memory for lineBuffer is zero initialized
    }
    
    if(useProducerLog){
	producerLog = fopen("producer-event.log", "r");
	if(producerLog == NULL || lineBuffer == NULL){
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
	if(consumerLog == NULL || lineBuffer == NULL){
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
