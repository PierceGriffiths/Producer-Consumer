#ifndef ARGSTRUCT_H
#define ARGSTRUCT_H
#include <stdio.h>
#include <threads.h>

struct pc_thread_args{
	FILE *restrict producerLog, *restrict consumerLog;
	mtx_t *restrict mutex;
	cnd_t *restrict canProduce, *restrict canConsume;
	size_t num_produced, num_consumed, target;
	int max_p_log_line, max_c_log_line;
};

struct log_thread_args{
	mtx_t *restrict mutex;
	int max_log_line;
};
#endif
