CC=gcc
CFLAGS= -march='native' -std=gnu11 -pedantic -I$(IDIR) -Wall
SDIR=./src
IDIR=$(SDIR)/include
LIBS=-lpthread
ODIR=$(SDIR)/obj
ifeq ($(MAKECMDGOALS),$(filter $(MAKECMDGOALS),release producer-consumer))
    CFLAGS += -O3
else ifeq ($(MAKECMDGOALS),$(filter $(MAKECMDGOALS),debug producer-consumer-debug))
    CFLAGS += -Og -g -pg
    ODIR=$(SDIR)/obj/debug
endif

_DEPS = macrodefs.h queue.h threaded_functions.h argstruct.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = queue.o producer.o consumer.o logreader_threads.o main.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	@if [ ! -d "$(ODIR)" ]; then	\
	    mkdir -p $(ODIR);		\
	fi;				
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: release debug cleanobj cleanall

release: producer-consumer

debug: producer-consumer-debug

producer-consumer: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

producer-consumer-debug: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

cleanobj:
	rm -f $(ODIR)/*.o $(ODIR)/debug/*.o *~ core $(INCDIR)/*~

cleanall: cleanobj
	rm -f producer-consumer producer-consumer-debug
