CC      := lcc
CFLAGS  := -Iinclude -Wa-l
LDFLAGS := -Wl-m -Wl-j -Wl-yt0x1B -Wl-yo4 -Wl-ya1

SRC     := src
DATA    := data

ENGINE_C := $(SRC)/main.c \
            $(SRC)/z_memory.c \
            $(SRC)/z_dispatcher.c \
            $(SRC)/z_variable_stack.c \
            $(SRC)/z_string_decoder.c \
            $(SRC)/z_object_engine.c \
            $(SRC)/z_vwf_render.c \
            $(SRC)/zork_font.c \
            $(SRC)/workboy.c \
            $(SRC)/z_status_bar.c

# Automatically include all bank C files
BANK_C := $(wildcard $(DATA)/zork_bank*.c)

OBJS    := $(ENGINE_C:.c=.o) $(BANK_C:.c=.o)

# Rule: Compile engine and banks
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Rule: Build the final ROM
zork_gb.gb: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

# Rule: Generate banked data from binary
$(DATA)/zork_bank1.c: $(DATA)/zork1.z3
	python3 tools/bin2banks.py $(DATA)/zork1.z3 $(DATA) $(DATA)/zork_data.h --start-bank 1

# Clean
clean:
	rm -f $(SRC)/*.o $(DATA)/*.o $(DATA)/zork_bank*.c *.gb *.map *.sym *.lst
