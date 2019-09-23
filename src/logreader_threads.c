#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "macrodefs.h"
#include "argstruct.h"

void* producer_log_reader(struct producerlog_thread_args *const args){
	char **restrict lineBuffer = NULL;
	//Use number of characters in longest line of the log file as the size of the line buffer
	size_t lineBufferSize = args->max_log_line;
	FILE *producerLog;
	if(lineBufferSize > 0){
		lineBuffer = calloc(lineBufferSize, sizeof(**lineBuffer));
		if(lineBuffer == NULL){
			args->ret = 1;
			//Lock mutex before printing error messages to stdout to ensure that it isn't printed while the consumer log is being written to stdout
			pthread_mutex_lock(args->mutex);
			fprintf(stderr, "Failed to allocate memory needed to read "PRODUCER_LOG_FILENAME"\n");
			goto producer_log_exit;
		}
	}
	else{
		args->ret = 1;
		pthread_mutex_lock(args->mutex);
		printf(PRODUCER_LOG_FILENAME" was not written to, so it will not be read\n");
		goto producer_log_exit;
	}
	producerLog = fopen(PRODUCER_LOG_FILENAME, "r");

	if(producerLog == NULL){
		free(lineBuffer);
		args->ret = 1;
		pthread_mutex_lock(args->mutex);
		fprintf(stderr, "Unable to read from "PRODUCER_LOG_FILENAME"\n");
		goto producer_log_exit;
	}
	else{
		if(getline(lineBuffer, &lineBufferSize, producerLog) == -1){
			fclose(producerLog);
			free(lineBuffer);
			args->ret = 1;
			pthread_mutex_lock(args->mutex);
			printf(PRODUCER_LOG_FILENAME" is empty\n");
			goto producer_log_exit;
		}
		pthread_mutex_lock(args->mutex);
		printf("Reading from "PRODUCER_LOG_FILENAME":\n");
		do{
			printf("%s", *lineBuffer);
		}
		while(getline(lineBuffer, &lineBufferSize, producerLog) != -1);

		fclose(producerLog);
		free(lineBuffer);
		printf("End of "PRODUCER_LOG_FILENAME"\n\n");
	}
	args->ret = 0;
producer_log_exit:
	pthread_mutex_unlock(args->mutex);
	pthread_exit(NULL);
}

void* consumer_log_reader(struct consumerlog_thread_args *const args){
	char **restrict lineBuffer = NULL;
	//Use number of characters in longest line of the log file as the size of the line buffer
	size_t lineBufferSize = args->max_log_line;
	FILE *consumerLog;

	if(lineBufferSize > 0){
		lineBuffer = calloc(lineBufferSize, sizeof(**lineBuffer));
		if(lineBuffer == NULL){
			args->ret = 1;
			//Lock mutex before printing error messages to stdout to ensure that it isn't printed while the producer log is being written to stdout
			pthread_mutex_lock(args->mutex);
			fprintf(stderr, "Failed to allocate memory needed to read "CONSUMER_LOG_FILENAME"\n");
			goto consumer_log_exit;
		}
	}
	else{
		args->ret = 1;
		pthread_mutex_lock(args->mutex);
		printf(CONSUMER_LOG_FILENAME" was not written to, so it will not be read\n");
		goto consumer_log_exit;
	}
	consumerLog = fopen(CONSUMER_LOG_FILENAME, "r");

	if(consumerLog == NULL){
		free(lineBuffer);
		args->ret = 1;
		pthread_mutex_lock(args->mutex);
		fprintf(stderr, "Unable to read from "CONSUMER_LOG_FILENAME"\n");
		goto consumer_log_exit;
	}
	else{
		if(getline(lineBuffer, &lineBufferSize, consumerLog) == -1){
			fclose(consumerLog);
			free(lineBuffer);
			args->ret = 1;
			pthread_mutex_lock(args->mutex);
			printf(CONSUMER_LOG_FILENAME" is empty\n");
			goto consumer_log_exit;
		}
		pthread_mutex_lock(args->mutex);
		printf("Reading from "CONSUMER_LOG_FILENAME":\n");
		do{
			printf("%s", *lineBuffer);
		}
		while(getline(lineBuffer, &lineBufferSize, consumerLog) != -1);

		fclose(consumerLog);
		free(lineBuffer);
		printf("End of "CONSUMER_LOG_FILENAME"\n\n");
	}
	args->ret = 0;
consumer_log_exit:
	pthread_mutex_unlock(args->mutex);
	pthread_exit(NULL);
}
