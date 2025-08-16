# Makefile for lkjlib
# Compiler and flags
CC = gcc
# Enhanced type checking and argument validation flags
CFLAGS = -Werror -Wall -Wextra -std=c11 -O2 -march=native -g
LDFLAGS = -static
INCLUDES = -Isrc/
LIBS = 

# Directories
SRC_DIR = src
INCLUDE_DIR = src/
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

# Source files - find all .c files in src/ recursively
SRCS = $(shell find $(SRC_DIR) -name "*.c")

# Object files
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Target executable
TARGET = $(BUILD_DIR)/lkjlib

# Header dependencies
HEADERS = $(wildcard $(INCLUDE_DIR)/*.h)

# Default and only target
all: $(TARGET)

# Create directories if they don't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link the executable
$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

# Compile all source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Phony targets
.PHONY: all
