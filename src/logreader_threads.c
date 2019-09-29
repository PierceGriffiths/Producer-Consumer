#define _GNU_SOURCE

#include "macrodefs.h"
#include "argstruct.h"
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#pragma GCC optimize ("Os")
int producer_log_reader(struct log_thread_args *const args){
	char **restrict lineBuffer = NULL;
	//Use number of characters in longest line of the log file as the size of the line buffer
	size_t lineBufferSize = args->max_log_line;
	FILE *producerLog;
	if(lineBufferSize > 0){
		lineBuffer = calloc(lineBufferSize, sizeof **lineBuffer);
		if(lineBuffer == NULL){
			//Lock mutex before printing error messages to stdout to ensure that it isn't printed while the consumer log is being written to stdout
			mtx_lock(args->mutex);
			fprintf(stderr, "Failed to allocate memory needed to read "PRODUCER_LOG_FILENAME"\n");
			mtx_unlock(args->mutex);
			return 1;
		}
	}
	else{
		mtx_lock(args->mutex);
		printf(PRODUCER_LOG_FILENAME" was not written to, so it will not be read\n");
		mtx_unlock(args->mutex);
		return 1;
	}
	producerLog = fopen(PRODUCER_LOG_FILENAME, "r");

	if(producerLog == NULL){
		free(lineBuffer);
		mtx_lock(args->mutex);
		fprintf(stderr, "Unable to read from "PRODUCER_LOG_FILENAME"\n");
		mtx_unlock(args->mutex);
		return 1;
	}
	else{
		if(getline(lineBuffer, &lineBufferSize, producerLog) == -1){
			fclose(producerLog);
			free(lineBuffer);
			mtx_lock(args->mutex);
			printf(PRODUCER_LOG_FILENAME" is empty\n");
			return 1;
		}
		mtx_lock(args->mutex);
		printf("Reading from "PRODUCER_LOG_FILENAME":\n");
		do{
			printf("%s", *lineBuffer);
		}
		while(getline(lineBuffer, &lineBufferSize, producerLog) != -1);

		fclose(producerLog);
		free(lineBuffer);
		printf("End of "PRODUCER_LOG_FILENAME"\n\n");
	}
	mtx_unlock(args->mutex);
	return 0;
}

int consumer_log_reader(struct log_thread_args *const args){
	char **restrict lineBuffer = NULL;
	//Use number of characters in longest line of the log file as the size of the line buffer
	size_t lineBufferSize = args->max_log_line;
	FILE *consumerLog;

	if(lineBufferSize > 0){
		lineBuffer = calloc(lineBufferSize, sizeof **lineBuffer);
		if(lineBuffer == NULL){
			//Lock mutex before printing error messages to stdout to ensure that it isn't printed while the producer log is being written to stdout
			mtx_lock(args->mutex);
			fprintf(stderr, "Failed to allocate memory needed to read "CONSUMER_LOG_FILENAME"\n");
			mtx_unlock(args->mutex);
			return 1;
		}
	}
	else{
		mtx_lock(args->mutex);
		printf(CONSUMER_LOG_FILENAME" was not written to, so it will not be read\n");
		mtx_unlock(args->mutex);
		return 1;
	}
	consumerLog = fopen(CONSUMER_LOG_FILENAME, "r");

	if(consumerLog == NULL){
		free(lineBuffer);
		mtx_lock(args->mutex);
		fprintf(stderr, "Unable to read from "CONSUMER_LOG_FILENAME"\n");
		mtx_unlock(args->mutex);
		return 1;
	}
	else{
		if(getline(lineBuffer, &lineBufferSize, consumerLog) == -1){
			fclose(consumerLog);
			free(lineBuffer);
			mtx_lock(args->mutex);
			printf(CONSUMER_LOG_FILENAME" is empty\n");
			mtx_unlock(args->mutex);
			return 1;
		}
		mtx_lock(args->mutex);
		printf("Reading from "CONSUMER_LOG_FILENAME":\n");
		do{
			printf("%s", *lineBuffer);
		}
		while(getline(lineBuffer, &lineBufferSize, consumerLog) != -1);

		fclose(consumerLog);
		free(lineBuffer);
		printf("End of "CONSUMER_LOG_FILENAME"\n\n");
	}
	mtx_unlock(args->mutex);
	return 0;
}
