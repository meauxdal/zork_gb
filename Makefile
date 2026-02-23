#### FILE: Makefile
CC = lcc
# MBC5 + RAM + BATTERY (0x1B)
# 2 ROM Banks, 1 RAM Bank
LCCFLAGS = -Wa-l -Wl-m -Wl-j -Iinclude -Wl-yt0x1B -Wl-yo2 -Wl-ya1

SRCS = src/main.c src/memory_core.c src/z_dispatcher.c \
       src/z_variable_stack.c src/z_string_decoder.c \
       src/z_object_engine.c src/vwf_render.c

OBJS = $(SRCS:.c=.o) data/zork_data.o

all: zork_gb.gb

zork_gb.gb: $(OBJS)
	$(CC) $(LCCFLAGS) -o $@ $(OBJS)

# This rule packs the raw .z3 file into a Game Boy object file in ROM Bank 1
data/zork_data.o: data/zork1.z3
	$(CC) -Wl-bo1 -c -o $@ data/zork1.z3

clean:
	rm -f src/*.o data/*.o *.gb *.map *.sym *.lst
