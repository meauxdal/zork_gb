CC = lcc
CFLAGS = -Iinclude
LDFLAGS = -Wa-l -Wl-m -Wl-j -Wl-yt0x1B -Wl-yo2 -Wl-ya1

# List of all object files
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

# Default target
all: zork_gb.gb

# Link the final Game Boy ROM
zork_gb.gb: $(OBJS) data/zork_data.o
	$(CC) $(LDFLAGS) -o zork_gb.gb $(OBJS) data/zork_data.o

# Compile C files to object files
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# DATA HANDLING: Wrap the Z3 binary in an assembly file to export symbols
data/zork_data.o: data/zork1.z3
	@echo ".area _CODE_1" > data/zork_data.s
	@echo ".globl _zork_data" >> data/zork_data.s
	@echo "_zork_data:" >> data/zork_data.s
	@echo ".incbin \"data/zork1.z3\"" >> data/zork_data.s
	$(CC) -c -o data/zork_data.o data/zork_data.s

# Cleanup
clean:
	rm -f src/*.o data/*.o data/*.s *.gb *.map *.sym *.lst
