#ifndef MACRODEFS_H
#define MACRODEFS_H
#define PRODUCER_LOG_FILENAME "producer-event.log"
#define CONSUMER_LOG_FILENAME "consumer-event.log"

#if defined(__linux__)
#define IS_POSIX
#define SUPPORTS_RLIM
#elif defined(__unix__)
#define IS_POSIX
#include <sys/param.h>
#ifdef BSD
#define SUPPORTS_RLIM
#endif //#ifdef BSD
#elif defined(__APPLE__) && defined(__MACH__)
#define IS_POSIX
#include <TargetConditionals.h>
#if TARGET_OS_MAC == 1
#define SUPPORTS_RLIM
#endif //if TARGET_OS_MAC == 1
#endif //if defined(__linux__)
 
#ifndef IS_POSIX
#warning "This program is not likely to be compatible with your operating system."
#elif defined(SUPPORTS_RLIM)
#include <sys/resource.h>
#endif

#endif
