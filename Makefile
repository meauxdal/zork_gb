CC = lcc

# Common flags
# -Wl-yo8: reserve 8 ROM banks (bank 0 = GBDK code, banks 1-6 = Z-data, bank 7 spare)
CFLAGS = -Iinclude \
         -Wa-l \
         -Wl-m -Wl-j \
         -Wl-yt0x1B -Wl-yo8 -Wl-ya1

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

# Bank data sources (generated from zork1.z3 by tools/bin2banks.py)
BANK_SRCS = data/zork_bank1.c \
            data/zork_bank2.c \
            data/zork_bank3.c \
            data/zork_bank4.c \
            data/zork_bank5.c \
            data/zork_bank6.c

OBJS     = $(SRCS:.c=.o)
BANK_OBJS = $(BANK_SRCS:.c=.o)

all: zork_gb.gb

zork_gb.gb: $(OBJS) $(BANK_OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(BANK_OBJS)

# Compile main sources
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Compile banked data files (each has #pragma bank N at the top)
data/%.o: data/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f src/*.o data/*.o *.gb *.map *.sym *.lst

# Regenerate bank C files from zork1.z3 (requires Python 3)
regen-banks:
	python3 tools/bin2banks.py data/zork1.z3 data/ include/zork_data.h
