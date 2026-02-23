CC = lcc

# Common flags
CFLAGS = -Iinclude \
         -Wa-l \
         -Wl-m -Wl-j \
         -Wl-yt0x1B -Wl-yo2 -Wl-ya1

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
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Compile C sources
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Convert Z-machine file into banked object (ROM bank 1)
data/zork_data.o: data/zork1.z3
	$(CC) -c -Wf-bo1 -o $@ $<

clean:
	rm -f src/*.o data/*.o *.gb *.map *.sym *.lst
	