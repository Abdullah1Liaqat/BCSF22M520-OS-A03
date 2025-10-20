# ==============================
# Makefile for BCSF22M520-OS-A03
# ==============================

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin

# Compiler and flags
CC = gcc
CFLAGS = -I$(INC_DIR) -Wall -Wextra
LDFLAGS = -lreadline          # ✅ link GNU Readline library

# Executable name
TARGET = $(BIN_DIR)/myshell

# Source and object files
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

# ==============================
# Default build target
# ==============================
all: dirs $(TARGET)

# Link all object files into executable
$(TARGET): $(OBJ)
	@echo "Linking..."
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)
	@echo "Build successful! Run ./$(TARGET)"

# Compile each source file into object file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Create directories if they don't exist
dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Clean up compiled files
clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Cleaned up!"
