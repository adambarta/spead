CFLAGS += -DDEBUG
CFLAGS += -DDATA
CFLAGS += -Wall
CFLAGS += -ggdb
CFLAGS += -O3
CFLAGS += -DIKATCP
CC = gcc

LIB= -lspead
LDIR= -L../src
IDIR= -I../src


HDR = mcs.h
UTILS = mcs.c packet.c
EXEst = mcs-static
EXE = mcs



###############
#   VERSION   #
GITVER = $(shell git describe --tags 2> /dev/null || echo unknown)
CFLAGS += -DVERSION=\"$(GITVER)\"



OBJ = $(patsubst %.c,%.o,$(UTILS))

all: $(EXE) $(EXEst)

%.o: %.c $(HDR)
	$(CC) $(CFLAGS) $(IDIR) -c $< -o $@

$(EXE): $(OBJ)
	$(CC) $^ -o $@ $(LDIR) $(LIB)

$(EXEst): $(OBJ)
	$(CC) $^ -o $@ $(LDIR) $(LIB) -s -static -ldl

clean:
	$(RM) $(OBJ) $(EXE) core vgcore*

install: all
	install $(EXE) /usr/local/bin
