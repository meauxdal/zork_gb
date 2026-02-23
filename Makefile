# Makefile - MVP Green Build for Zork GB

# Compiler and flags
CC = lcc
CFLAGS = -Iinclude -Wa-l
LDFLAGS = -Wl-m -Wl-j -Wl-yt0x1B -Wl-yo4 -Wl-ya1

# Source directories
SRC_DIR = src
DATA_DIR = data/zork_data

# Collect source files
SRC = $(wildcard $(SRC_DIR)/*.c)
DATA_SRC = $(wildcard $(DATA_DIR)/*.c)

# Object files
OBJ = $(SRC:.c=.o) $(DATA_SRC:.c=.o)

# Target
TARGET = zork_gb.gb

# Default rule
all: $(TARGET)

# Compile source files
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Compile data bank files
$(DATA_DIR)/%.o: $(DATA_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Link everything
$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

# Clean
clean:
	rm -f $(SRC_DIR)/*.o $(DATA_DIR)/*.o $(TARGET)

.PHONY: all clean
