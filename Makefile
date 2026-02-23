# Target ROM Name
TARGET = zork_gb.gb

# Toolchain
CC = lcc

# Compiler and Linker Flags
# -Wl-yo4: Sets ROM size to 4 banks (64KB) to accommodate Zork data
# -Wl-yt0x1B: MBC5+RAM+BATTERY
# -Wl-ya1: 1 RAM Bank
CFLAGS = -Iinclude
LDFLAGS = -Wa-l -Wl-m -Wl-j -Wl-yt0x1B -Wl-yo4 -Wl-ya1

# Object Files
OBJS = src/main.o \
       src/z_memory.o \
       src/z_dispatcher.o \
       src/z_variable_stack.o \
       src/z_string_decoder.o \
       src/z_object_engine.o \
       src/z_vwf_render.o \
       src/zork_font.o \
       src/workboy.o \
       src/z_status_bar.o

# Default target for GitHub Actions
all: $(TARGET)

# Link the final ROM
# We explicitly include data/zork_data.o in the link stage
$(TARGET): $(OBJS) data/zork_data.o
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS) data/zork_data.o

# Compile C Source Files
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Generate the Assembly Bridge for the .z3 story file
# This creates the _zork_data symbol and places it in Bank 2
data/zork_data.o: data/zork1.z3
	@echo ".area _CODE_2" > data/zork_data.s
	@echo ".globl _zork_data" >> data/zork_data.s
	@echo "_zork_data:" >> data/zork_data.s
	@echo ".incbin \"data/zork1.z3\"" >> data/zork_data.s
	$(CC) -c -o data/zork_data.o data/zork_data.s

# Cleanup rule
clean:
	rm -f src/*.o data/*.o data/*.s *.gb *.map *.sym *.lst
