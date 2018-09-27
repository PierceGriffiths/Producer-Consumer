#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "argstruct.h"

void* producer_log_reader(producerlog_thread_args *args){
    char *lineBuffer = NULL;
    size_t lineBufferSize = 50;

    if(args->producerLog != NULL){
	lineBuffer = calloc(lineBufferSize, sizeof(*lineBuffer));
	if(lineBuffer == NULL){
	    args->ret = -1;
	    //Lock mutex before printing error messages to stdout to ensure that it isn't printed while the consumer log is being written to stdout
	    pthread_mutex_lock(args->mutex);
	    printf("Failed to allocate memory needed to read %s.\n", PRODUCER_LOG_FILENAME);
	    goto producer_log_exit;
	}
    }
    else{
	args->ret = 1;
	pthread_mutex_lock(args->mutex);
	printf("%s was not written to, so it will not be read\n", PRODUCER_LOG_FILENAME);
	goto producer_log_exit;
    }
    args->producerLog = fopen(PRODUCER_LOG_FILENAME, "r");

    if(args->producerLog == NULL){
	args->ret = -1;
	pthread_mutex_lock(args->mutex);
	printf("Unable to read from %s.\n", PRODUCER_LOG_FILENAME);
	goto producer_log_exit;
    }
    else{
	if(getline(&lineBuffer, &lineBufferSize, args->producerLog) == -1){
	    fclose(args->producerLog);
	    args->ret = 1;
	    pthread_mutex_lock(args->mutex);
	    printf("%s is empty.\n", PRODUCER_LOG_FILENAME);
	    goto producer_log_exit;
	}
	pthread_mutex_lock(args->mutex);
	printf("Reading from %s:\n", PRODUCER_LOG_FILENAME);
	do{
	    printf("%s", lineBuffer);
	}
	while(getline(&lineBuffer, &lineBufferSize, args->producerLog) != -1);

	fclose(args->producerLog);
	printf("End of %s.\n\n", PRODUCER_LOG_FILENAME);
    }
    args->ret = 0;
producer_log_exit:
    pthread_mutex_unlock(args->mutex);
    pthread_exit(NULL);
}

void* consumer_log_reader(consumerlog_thread_args *args){
    char *lineBuffer = NULL;
    size_t lineBufferSize = 50;

    if(args->consumerLog != NULL){
	lineBuffer = calloc(lineBufferSize, sizeof(*lineBuffer));
	if(lineBuffer == NULL){
	    args->ret = -1;
	    //Lock mutex before printing error messages to stdout to ensure that it isn't printed while the producer log is being written to stdout
	    pthread_mutex_lock(args->mutex);
	    printf("Failed to allocate memory needed to read %s.\n", CONSUMER_LOG_FILENAME);
	    goto consumer_log_exit;
	}
    }
    else{
	args->ret = 1;
	pthread_mutex_lock(args->mutex);
	printf("%s was not written to, so it will not be read\n", CONSUMER_LOG_FILENAME);
	goto consumer_log_exit;
    }
    args->consumerLog = fopen(CONSUMER_LOG_FILENAME, "r");

    if(args->consumerLog == NULL){
	args->ret = -1;
	pthread_mutex_lock(args->mutex);
	printf("Unable to read from %s.\n", CONSUMER_LOG_FILENAME);
	goto consumer_log_exit;
    }
    else{
	if(getline(&lineBuffer, &lineBufferSize, args->consumerLog) == -1){
	    fclose(args->consumerLog);
	    args->ret = 1;
	    pthread_mutex_lock(args->mutex);
	    printf("%s is empty.\n", CONSUMER_LOG_FILENAME);
	    goto consumer_log_exit;
	}
	pthread_mutex_lock(args->mutex);
	printf("Reading from %s:\n", CONSUMER_LOG_FILENAME);
	do{
	    printf("%s", lineBuffer);
	}
	while(getline(&lineBuffer, &lineBufferSize, args->consumerLog) != -1);

	fclose(args->consumerLog);
	printf("End of %s.\n\n", CONSUMER_LOG_FILENAME);
    }
    args->ret = 0;
consumer_log_exit:
    pthread_mutex_unlock(args->mutex);
    pthread_exit(NULL);
}
