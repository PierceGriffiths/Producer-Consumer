Producer-Consumer
======

## Summary:
This program creates "w" producer threads, "x" consumer threads, a shared buffer with a capacity of "y" items, and will produce exactly "z" items, where w, x, y, and z are unsigned integers whose values are given as command line arguments. The items produced are pseudorandom unsigned integers. A single mutex is used to restrict access to the buffer to exactly one thread at a time. The buffer takes the form of a queue, which ensures that items are consumed in the same order in which they were produced. Each time an item is produced or consumed, the event is shown on stdout as well as logged to the appropriate log file.

### How to compile:
While in the root directory for this project (it contains this readme), enter `make` into the shell, and the makefile will take care of the rest


### How to run:
`./producer-consumer <# producer threads> <# consumer threads> <buffer size> <# items to be produced>`

### Understanding the log files (consumer-event.log and producer-event.log):
`<Timestamp (in nanoseconds)> <Thread type ("Producer" or "Consumer"> <Thread ID> <Buffer entry index> <Item>`

### Notes on the directories:
This directory (./) serves as the root directory for the project, and contains the README, the makefile, the executable version of the project (once compiled), and the logfiles


src/ contains the (.c) source files for this project


src/include/ contains header files


src/obj/ contains object (.o) files created at compile time


To compile from scratch, use `make clean` followed by `make`
