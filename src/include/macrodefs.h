#ifndef MACRODEFS_H
#define MACRODEFS_H

#ifdef __linux__
#define ID_FORMAT "d"
#else
#include <inttypes.h>
#define ID_FORMAT PRIuMAX
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#else
#define NANOSLEEP_TIME (const struct timespec[]){{0, 1}}
#endif

#define PRODUCER_LOG_FILENAME "producer-event.log"
#define CONSUMER_LOG_FILENAME "consumer-event.log"

#define ERR_BUFF_LEN 256

#endif//#ifndef MACRODEFS_H
