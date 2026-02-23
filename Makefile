# Directory for generated banked C files
BANK_DIR := data
BANK_HDR := $(BANK_DIR)/zork_data.h

# Generate bank C files from the Z3 binary
$(BANK_DIR)/zork_bank%.c $(BANK_HDR): data/zork1.z3
	python3 tools/bin2banks.py data/zork1.z3 $(BANK_DIR) $(BANK_HDR) --start-bank 7

# Grab all bank .c files automatically
BANK_SRCS := $(wildcard $(BANK_DIR)/zork_bank*.c)
BANK_OBJS := $(patsubst $(BANK_DIR)/%.c,src/%.o,$(BANK_SRCS))

# Compile each bank into src/*.o
src/%.o: $(BANK_DIR)/%.c
	lcc -Iinclude -c -o $@ $<

# Add bank objects to your main OBJS list
OBJS := src/main.o src/z_memory.o src/z_dispatcher.o src/z_variable_stack.o \
        src/z_string_decoder.o src/z_object_engine.o src/z_vwf_render.o \
        src/zork_font.o src/workboy.o src/z_status_bar.o $(BANK_OBJS)

# Final link
zork_gb.gb: $(OBJS)
	lcc -Wa-l -Wl-m -Wl-j -Wl-yt0x1B -Wl-yo4 -Wl-ya1 -o $@ $(OBJS)
