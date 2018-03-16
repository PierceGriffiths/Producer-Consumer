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

int main(int argc, char *argv[]){
    unsigned i, numProducers, numConsumers;
    unsigned long argCheck;
    pthread_t *producers, *consumers;
    char *lineBuffer;
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
    }
    numProducers = (unsigned)strtoul(argv[1], NULL, 10);
    numConsumers = (unsigned)strtoul(argv[2], NULL, 10);
    target = (unsigned)strtoul(argv[4], NULL, 10);
    producerLog = fopen("producer-event.log", "w+");
    consumerLog = fopen("consumer-event.log", "w+");
    
    buffer = createQueue(strtoul(argv[3], NULL, 10));//see queue.c for the queue implementation
    if(buffer == NULL){
	fprintf(stderr, "Failed to allocate memory for buffer.\n");
	exit(1);
    }
    
    producers = calloc(numProducers, sizeof(*producers));//See producer.c for the implementation of the producer function
    if(producers == NULL){//Check whether memory was allocated
	fprintf(stderr, "Failed to allocate memory for producer threads.\n");
	exit(1);
    }
    else{
	for(i = 0; i < numProducers; i++){
	    pthread_create(&producers[i], NULL, producer, NULL);
	}
    }
    
    consumers = calloc(numConsumers, sizeof(*consumers));//See consumer.c for the implementation of the consumer function
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
    printf("Produced: %u\nConsumed: %u\n", num_produced, num_consumed);
    
    lineBuffer = calloc(lineBufferSize, sizeof(*lineBuffer));
    printf("Reading from producer-event.log:\n");
    producerLog = fopen("producer-event.log", "r");
    while(getline(&lineBuffer, &lineBufferSize, producerLog) != -1){
	printf("%s", lineBuffer);
    }
    fclose(producerLog);
    
    printf("\n\nEnd of producer-event.log. Reading from consumer-event.log:\n");
    consumerLog = fopen("consumer-event.log", "r");
    while(getline(&lineBuffer, &lineBufferSize, consumerLog) != -1){
	printf("%s", lineBuffer);
    }
    
    free(lineBuffer);
    fclose(consumerLog);
    
    printf("\n\nDone.\n");
    return 0;
}
