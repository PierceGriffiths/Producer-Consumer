SHELL := /bin/bash
CC=gcc
CFLAGS= -march='native' -O3 -std=gnu11 -pedantic -I$(IDIR) -Wall

SDIR=./src
IDIR=$(SDIR)/include
ODIR=$(SDIR)/obj
LIBS=-lpthread

_DEPS = queue.h producer.h consumer.h argstruct.h 
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = queue.o producer.o consumer.o producer-consumer.o 
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	@if [ ! -d "$(ODIR)" ]; then	\
	    mkdir $(ODIR);		\
	fi;				
	$(CC) $(CFLAGS) -c -o $@ $<


producer-consumer: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

debug: $(OBJ)
	$(CC) -g $(CFLAGS) -o debug-pc $^ $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
