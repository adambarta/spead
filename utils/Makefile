CFLAGS += -DDEBUG
CFLAGS += -DIKATCP
CFLAGS += -Wall
CFLAGS += -ggdb
CFLAGS += -O3

LIB= -lspead -lm
LDIR= -L../src
IDIR= -I../src


UTILS = udp_io.c send.c raw_spead.c pipespead.c murmur.c proximity_search.c haversine.c coopers_multiple_dwelling_problem.c cloneeg.c 
EXE = $(patsubst %.c,%.elf,$(UTILS))
EXE2 = $(patsubst %.c,%,$(UTILS))

OBJ = $(patsubst %.c,%.o,$(UTILS))

all: $(EXE)

%.o: %.c
	$(CC) $(CFLAGS) $(IDIR) -c $^ -o $@

%.elf: %.o 
	$(CC) $< -o $@ $(LDIR) $(LIB)
	mv $@ $(patsubst %.elf,%,$@)  

clean:
	$(RM) $(OBJ) $(EXE) $(EXE2) core vgcore*

install: all
	install $(EXE) /usr/local/bin
