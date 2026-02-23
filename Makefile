#### FILE: Makefile
CC = lcc -Wa-l -Wl-m -Wl-j -Iinclude

LDFLAGS = -Wl-yt0x1B -Wl-yo8 -Wl-ya0

# Path correction for new folder structure
SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o) zork_data.o

all: zork_gb.gb

zork_gb.gb: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

zork_data.o: data/zork1.z3
	$(CC) -Wl-W -Wl-bo1 -c -o zork_data.o data/zork1.z3

clean:
	rm -f src/*.o *.o *.gb *.map *.sym *.lst
	