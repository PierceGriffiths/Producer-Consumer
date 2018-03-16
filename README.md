Producer-Consumer
======

### How to compile:
While in the root directory for this project (it contains this readme), enter `make` into the shell, and the makefile will take care of the rest


### How to run:
`./producer-consumer <# producer threads> <# consumer threads> <buffer size> <# items to be produced>`

### Notes on the directories:
This directory (./) serves as the root directory for the project, and contains the README, the makefile, the executable version of the project (once compiled), and the logfiles


src/ contains the (.c) source files for this project


src/include/ contains header files


src/obj/ contains object (.o) files created at compile time


To compile from scratch, use `make clean` followed by `make`
