# Makefile for lkjagent
# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g
LDFLAGS =
DEBUG_CFLAGS = -Wall -Wextra -std=c11 -O0 -g3 -DDEBUG
RELEASE_CFLAGS = -Wall -Wextra -std=c11 -O2 -DNDEBUG -flto
INCLUDES = -Isrc/include
LIBS = 

# Directories
SRC_DIR = src
INCLUDE_DIR = src/include
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
	mkdir -p $(OBJ_DIR)/utils

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link the executable
$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

# Compile all source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean build files
clean:
	rm -rf $(BUILD_DIR)

# Rebuild everything
rebuild: clean all

# Install (optional)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# Uninstall (optional)
uninstall:
	rm -f /usr/local/bin/lkjagent

# Run the program
run: $(TARGET)
	./$(TARGET)

# Debug build
debug: CFLAGS = $(DEBUG_CFLAGS)
debug: $(TARGET)

# Release build
release: CFLAGS = $(RELEASE_CFLAGS)
release: $(TARGET)

# Static analysis with cppcheck
check:
	cppcheck --enable=all --std=c11 $(SRC_DIR)/

# Memory check with valgrind
memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

# Show help
help:
	@echo "Available targets:"
	@echo "  all        - Build the project (default)"
	@echo "  clean      - Remove build files"
	@echo "  rebuild    - Clean and build"
	@echo "  install    - Install to /usr/local/bin"
	@echo "  uninstall  - Remove from /usr/local/bin"
	@echo "  run        - Build and run the program"
	@echo "  debug      - Build with debug flags"
	@echo "  release    - Build with optimization flags"
	@echo "  check      - Run static analysis with cppcheck"
	@echo "  memcheck   - Run memory check with valgrind"
	@echo "  help       - Show this help message"

# Phony targets
.PHONY: all clean rebuild install uninstall run debug release check memcheck help
