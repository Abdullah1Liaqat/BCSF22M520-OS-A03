# -------------------------------
# Makefile for myshell project
# Author: Abdullah Rao (BCSF22M520)
# -------------------------------

# Compiler
CC = gcc

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin

# Executable name
TARGET = $(BIN_DIR)/myshell

# Source and object files
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

# Compilation flags
CFLAGS = -I$(INC_DIR) -Wall -Wextra

# Default target
all: dirs $(TARGET)

# Link all object files into executable
$(TARGET): $(OBJ)
	@echo "Linking..."
	$(CC) $(OBJ) -o $(TARGET)
	@echo "Build successful! Run ./$(TARGET)"

# Compile each source file into object file
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Create directories if not exist
dirs:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Clean compiled files
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Cleaned up!"

# Run the shell directly
run: all
	./$(TARGET)
