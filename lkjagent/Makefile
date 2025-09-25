# **ULTIMATE MAKEFILE FOR REVOLUTIONARY LKJAGENT**
# Compiler and flags optimized for maximum performance
CC = gcc
CFLAGS = -Werror -Wall -Wextra -std=c11 -O3 -march=native -g -DULTIMATE_MODE
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
TARGET = $(BUILD_DIR)/lkjagent

# Header dependencies
HEADERS = $(wildcard $(INCLUDE_DIR)/*.h)

# Default target
all: $(TARGET)

# Create directories if they don't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
	mkdir -p $(OBJ_DIR)/lkjlib
	mkdir -p $(OBJ_DIR)/action

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link the ultimate executable
$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

# Compile all source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Phony targets
.PHONY: all

# Ultimate system targets
ultimate-build: $(TARGET)
	@echo "Ultimate LKJAgent system built successfully!"
	
ultimate-clean:
	rm -rf $(BUILD_DIR)
	@echo "Ultimate build cleaned!"

.PHONY: ultimate-build ultimate-clean
