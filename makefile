SHELL := /bin/bash
CC=gcc
CFLAGS= -I$(IDIR) -Wall

SDIR=./src
IDIR=$(SDIR)/include
ODIR=$(SDIR)/obj
LIBS=-lpthread

_DEPS = queue.h producer.h consumer.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = queue.o producer.o consumer.o producer-consumer.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	@if [ ! -d "$(ODIR)" ]; then	\
	    mkdir $(ODIR);		\
	fi;				
	$(CC) -c -o $@ $< $(CFLAGS)


producer-consumer: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
