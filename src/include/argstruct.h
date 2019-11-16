#ifndef ARGSTRUCT_H
#define ARGSTRUCT_H
#include "queue.h"
#include <stdio.h>
#include <stdint.h>
#include <threads.h>

struct thread_args{
	struct Queue *restrict buffer;
	FILE *restrict producerLog, *restrict consumerLog;
	mtx_t *restrict mutex;
	cnd_t *restrict canProduce, *restrict canConsume;
	int_fast16_t num_produced, num_consumed, target;
	int_fast16_t max_p_log_line, max_c_log_line;
};
#endif
