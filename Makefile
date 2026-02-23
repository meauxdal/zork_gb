# ===========================
# MVP Makefile for zork_gb
# ===========================

# Compiler
CC = lcc
CFLAGS = -Iinclude -Wa-l
LDFLAGS = -Wl-m -Wl-j -Wl-yt0x1B -Wl-yo4 -Wl-ya1

# Sources
SRCDIR = src
DATADIR = data
DATA_BIN = $(DATADIR)/zork1.z3
DATA_OUT = $(DATADIR)/zork_data
DATA_HDR = include/zork_data.h

SRCFILES = $(SRCDIR)/main.c \
           $(SRCDIR)/z_memory.c \
           $(SRCDIR)/z_dispatcher.c \
           $(SRCDIR)/z_variable_stack.c \
           $(SRCDIR)/z_string_decoder.c \
           $(SRCDIR)/z_object_engine.c \
           $(SRCDIR)/z_vwf_render.c \
           $(SRCDIR)/zork_font.c \
           $(SRCDIR)/workboy.c \
           $(SRCDIR)/z_status_bar.c

OBJFILES = $(patsubst %.c,%.o,$(SRCFILES)) $(DATA_OUT)/zork_bank1.o \
           $(DATA_OUT)/zork_bank2.o $(DATA_OUT)/zork_bank3.o \
           $(DATA_OUT)/zork_bank4.o $(DATA_OUT)/zork_bank5.o \
           $(DATA_OUT)/zork_bank6.o $(DATA_OUT)/zork_bank7.o \
           $(DATA_OUT)/zork_bank8.o $(DATA_OUT)/zork_bank9.o \
           $(DATA_OUT)/zork_bank10.o

# Default target
all: zork_gb.gb

# Generate bank files from binary
$(DATA_HDR): $(DATA_BIN)
	@mkdir -p $(DATA_OUT)
	python3 tools/bin2banks.py $(DATA_BIN) $(DATA_OUT) $(DATA_HDR)

# Compile source files
$(SRCDIR)/%.o: $(SRCDIR)/%.c $(DATA_HDR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Compile bank .c files
$(DATA_OUT)/%.o: $(DATA_OUT)/%.c $(DATA_HDR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Link final ROM
zork_gb.gb: $(OBJFILES)
	$(CC) $(LDFLAGS) -o $@ $^

# Clean up build artifacts
clean:
	rm -f $(SRCDIR)/*.o $(DATA_OUT)/*.o $(DATA_OUT)/zork_bank*.c $(DATA_HDR) zork_gb.gb

.PHONY: all clean
