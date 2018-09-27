SHELL := /bin/bash
CC=gcc
CFLAGS= -march='native' -std=gnu11 -pedantic -I$(IDIR) -Wall
SDIR=./src
IDIR=$(SDIR)/include
LIBS=-lpthread
ifneq ($(MAKECMDGOALS),debug)
    CFLAGS += -O3
    ODIR=$(SDIR)/obj
else
    CFLAGS += -Og -g -pg
    ODIR=$(SDIR)/obj/debug
endif

_DEPS = queue.h threaded_functions.h argstruct.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = queue.o producer.o consumer.o main.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	@if [ ! -d "$(ODIR)" ]; then	\
	    mkdir -p $(ODIR);		\
	fi;				
	$(CC) $(CFLAGS) -c -o $@ $<


producer-consumer: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

debug: $(OBJ)
	$(CC) $(CFLAGS) -o debug-pc $^ $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $(ODIR)/debug/*.o *~ core $(INCDIR)/*~
