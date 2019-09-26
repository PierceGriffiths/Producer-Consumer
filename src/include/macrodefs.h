#ifndef MACRODEFS_H
#define MACRODEFS_H

#if defined(__linux__)
#define SUPPORTS_RLIM
#define ID_FORMAT "d"
#elif defined(__unix__)
#include <sys/param.h>
#ifdef BSD
#define SUPPORTS_RLIM
#endif //#ifdef BSD
#elif defined(__APPLE__) && defined(__MACH__)
#include <TargetConditionals.h>
#if TARGET_OS_MAC == 1
#define SUPPORTS_RLIM
#endif //if TARGET_OS_MAC == 1
#endif //if defined(__linux__)

#ifndef __linux__
#include <inttypes.h>
#define ID_FORMAT PRIuMAX
#endif//#ifndef __linux__

#define PRODUCER_LOG_FILENAME "producer-event.log"
#define CONSUMER_LOG_FILENAME "consumer-event.log"

#define NANOSLEEP_TIME (const struct timespec[]){{0, 1}}

#endif//#ifndef MACRODEFS_H
