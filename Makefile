# ===========================
# Makefile for zork_gb
# ===========================

CC     = lcc
# -Iinclude: pick up zork_data.h and generated headers
# -Wa-l: suppress assembly listing
CFLAGS  = -Iinclude -Wa-l
# -Wl-m: map file  -Wl-j: join identical sections
# -Wl-yt0x1B: MBC5 (0x1B)  -Wl-yo4: 4 ROM banks min
# -Wl-ya4: 4 SRAM banks (dynamic memory + save state)
LDFLAGS = -Wl-m -Wl-j -Wl-yt0x1B -Wl-yo16 -Wl-ya4

SRCDIR  = src
DATADIR = data
TOOLDIR = tools

DATA_BIN = $(DATADIR)/zork1.z3
DATA_OUT = $(DATADIR)/zork_data
DATA_HDR = include/zork_data.h
DATA_BANK_START = 2

SRCFILES = \
    $(SRCDIR)/main.c           \
    $(SRCDIR)/z_memory.c       \
    $(SRCDIR)/z_dispatcher.c   \
    $(SRCDIR)/z_variable_stack.c \
    $(SRCDIR)/z_string_decoder.c \
    $(SRCDIR)/z_object_engine.c  \
    $(SRCDIR)/z_render.c         \
    $(SRCDIR)/zork_font.c        \
    $(SRCDIR)/workboy.c          \
    $(SRCDIR)/z_status_bar.c

# Bank object files — count must match ZORK_DATA_NUM_BANKS in zork_data.h
BANK_OBJS = \
    $(DATA_OUT)/zork_bank2.o  \
    $(DATA_OUT)/zork_bank3.o  \
    $(DATA_OUT)/zork_bank4.o  \
    $(DATA_OUT)/zork_bank5.o  \
    $(DATA_OUT)/zork_bank6.o  \
    $(DATA_OUT)/zork_bank7.o

OBJFILES = $(patsubst %.c,%.o,$(SRCFILES)) $(BANK_OBJS)

# -----------------------------------------------------------------------
all: zork_gb.gb

# Generate bank C files + header from z3 binary
$(DATA_HDR): $(DATA_BIN)
	@mkdir -p $(DATA_OUT)
	python3 $(TOOLDIR)/bin2banks.py $(DATA_BIN) $(DATA_OUT) $(DATA_HDR) --start-bank $(DATA_BANK_START)

# Compile source files
$(SRCDIR)/%.o: $(SRCDIR)/%.c $(DATA_HDR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Compile generated bank files
$(DATA_OUT)/%.o: $(DATA_OUT)/%.c $(DATA_HDR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Link
zork_gb.gb: $(OBJFILES)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(SRCDIR)/*.o \
	      $(DATA_OUT)/*.o \
	      $(DATA_OUT)/zork_bank*.c \
	      $(DATA_HDR) \
	      zork_gb.gb \
	      zork_gb.map

.PHONY: all clean
