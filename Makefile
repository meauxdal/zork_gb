CC = lcc

# Include directory mapping
CFLAGS = -Iinclude
# Linker flags for MBC1 + RAM + Battery
LCCFLAGS = $(CFLAGS) -Wa-l -Wl-m -Wl-j -Wl-yt0x1B -Wl-yo2 -Wl-ya1

SRCS = src/main.c \
       src/z_memory.c \
       src/z_dispatcher.c \
       src/z_variable_stack.c \
       src/z_string_decoder.c \
       src/z_object_engine.c \
       src/z_vwf_render.c \
       src/zork_font.c \
       src/workboy.c \
       src/z_status_bar.c

OBJS = $(SRCS:.c=.o) data/zork_data.o

all: zork_gb.gb

zork_gb.gb: $(OBJS)
	$(CC) $(LCCFLAGS) -o $@ $(OBJS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

data/zork_data.o: data/zork1.z3
	$(CC) -Wl-bo1 -c -o $@ data/zork1.z3

clean:
	rm -f src/*.o data/*.o *.gb *.map *.sym *.lst
