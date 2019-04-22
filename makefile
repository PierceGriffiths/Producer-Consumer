CC := gcc
SDIR := ./src
IDIR := $(SDIR)/include
CFLAGS := -march='native' -std=gnu11 -pedantic -I$(IDIR) -Wall
LIBS := -lpthread
ODIR := $(SDIR)/obj
ifeq ($(MAKECMDGOALS),$(filter $(MAKECMDGOALS),release producer-consumer))
    CFLAGS := $(CFLAGS) -O3
else ifeq ($(MAKECMDGOALS),$(filter $(MAKECMDGOALS),debug producer-consumer-debug))
    CFLAGS := $(CFLAGS) -Og -g -pg
    ODIR := $(SDIR)/obj/debug
endif

DEPS := $(wildcard $(IDIR)/*.h)

_OBJ := $(patsubst %.c, %.o, $(notdir $(wildcard $(SDIR)/*.c)))
OBJ := $(patsubst %, $(ODIR)/%, $(_OBJ))

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
	rm -f $(ODIR)/*.o $(ODIR)/debug/*.o *~ core $(IDIR)/*~

cleanall: cleanobj
	rm -f producer-consumer producer-consumer-debug
