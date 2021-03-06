Producer-Consumer
======

## Summary
This program creates "w" producer threads, "x" consumer threads, a shared buffer with a capacity of "y" items, and will produce exactly "z" items, where w, x, y, and z are unsigned integers whose values are given as command line arguments. The items produced are pseudorandom nonnegative long integers over the interval [0, 2^31]. A single mutex is used to restrict access to the buffer to exactly one thread at a time. The buffer takes the form of a queue, which ensures that items are consumed in the same order in which they were produced. Each time an item is produced or consumed, the event is shown on stdout as well as logged to the appropriate log file.

A C++ version, [Producer-Consumer++](https://github.com/PierceGriffiths/Producer-Consumer-Plus-Plus), also exists.

### Understanding the log files (consumer-event.log and producer-event.log)
`<Nanoseconds since the epoch> <Thread type ("Producer" or "Consumer")> <Thread ID> <Buffer entry index> <Item>`
To save the log files under a different name and/or in a different directory, modify the definitions of `PRODUCER_LOG_FILENAME` and `CONSUMER_LOG_FILENAME` in `src/include/macrodefs.h`
