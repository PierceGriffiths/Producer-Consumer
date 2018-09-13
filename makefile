SHELL := /bin/bash
CC=gcc
CFLAGS= -march='native' -O3 -I$(IDIR) -Wall

SDIR=./src
IDIR=$(SDIR)/include
ODIR=$(SDIR)/obj
LIBS=-lpthread

_DEPS = queue.h producer.h consumer.h argstructs.h 
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

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
