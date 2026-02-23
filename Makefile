#### FILE: Makefile
CC = lcc

# Compilation and Linking Flags
# -Iinclude: Look for headers in the include directory
# -Wl-yt0x1B: MBC5 + RAM + BATTERY
# -Wl-yo2: 2 ROM Banks
# -Wl-ya1: 1 RAM Bank (8KB)
CFLAGS = -Iinclude
LCCFLAGS = -Wa-l -Wl-m -Wl-j $(CFLAGS) -Wl-yt0x1B -Wl-yo2 -Wl-ya1

SRCS = src/main.c \
       src/memory_core.c \
       src/z_dispatcher.c \
       src/z_variable_stack.c \
       src/z_string_decoder.c \
       src/z_object_engine.c \
       src/vwf_render.c \
       src/zork_font.c

OBJS = $(SRCS:.c=.o) data/zork_data.o

all: zork_gb.gb

# Rule for the final ROM
zork_gb.gb: $(OBJS)
	$(CC) $(LCCFLAGS) -o $@ $(OBJS)

# Explicit rule for .c to .o conversion to ensure CFLAGS are included
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Pack the raw .z3 file into ROM Bank 1
data/zork_data.o: data/zork1.z3
	$(CC) -Wl-bo1 -c -o $@ data/zork1.z3

clean:
	rm -f src/*.o data/*.o *.gb *.map *.sym *.lst
	