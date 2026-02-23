# Makefile - MVP build for Zork GB

CC = lcc
CFLAGS = -Iinclude -Wa-l
LDFLAGS = -Wl-m -Wl-j -Wl-yt0x1B -Wl-yo4 -Wl-ya1

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

ZORK_BIN = data/zork1.z3
ZORK_OUT = data/zork_data

# Default target
all: zork_gb.gb

# Link final ROM
zork_gb.gb: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

# Compile each source file explicitly
src/main.o: src/main.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/z_memory.o: src/z_memory.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/z_dispatcher.o: src/z_dispatcher.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/z_variable_stack.o: src/z_variable_stack.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/z_string_decoder.o: src/z_string_decoder.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/z_object_engine.o: src/z_object_engine.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/z_vwf_render.o: src/z_vwf_render.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/zork_font.o: src/zork_font.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/workboy.o: src/workboy.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/z_status_bar.o: src/z_status_bar.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Build Zork banked data
data/zork_data.o: $(ZORK_BIN)
	python3 tools/bin2banks.py $(ZORK_BIN) $(ZORK_OUT) data/zork_data.h
	$(CC) $(CFLAGS) -c -o $@ $(ZORK_OUT)/zork_bank*.c

# Clean build artifacts
clean:
	rm -f src/*.o data/*.o $(ZORK_OUT)/zork_bank*.c zork_gb.gb

.PHONY: all clean
