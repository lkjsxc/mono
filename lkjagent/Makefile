# LKJAgent Makefile

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Werror -pedantic -O2
DEBUG_CFLAGS = -std=c99 -Wall -Wextra -Werror -pedantic -g -DDEBUG
INCLUDES = -Isrc
TARGET = lkjagent
BUILD_DIR = build
SRC_DIR = src

# Source files
SOURCES = $(shell find $(SRC_DIR) -name "*.c")
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Default target
all: $(BUILD_DIR)/$(TARGET)

# Debug build
debug: CFLAGS = $(DEBUG_CFLAGS)
debug: $(BUILD_DIR)/$(TARGET)

# Create build directory structure
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/agent
	@mkdir -p $(BUILD_DIR)/config
	@mkdir -p $(BUILD_DIR)/memory
	@mkdir -p $(BUILD_DIR)/state
	@mkdir -p $(BUILD_DIR)/llm
	@mkdir -p $(BUILD_DIR)/utils
	@mkdir -p $(BUILD_DIR)/persistence

# Build target
$(BUILD_DIR)/$(TARGET): $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(OBJECTS) -o $@

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean build files
clean:
	rm -rf $(BUILD_DIR)

# Create data directory and default config
setup: | $(BUILD_DIR)
	@mkdir -p data
	@if [ ! -f data/config.json ]; then \
		echo '{"agent":{"max_iterations":-1},"llm":{"endpoint":"http://localhost:1234/v1/chat/completions"}}' > data/config.json; \
	fi
	@if [ ! -f data/memory.json ]; then \
		echo '{"working_memory":[],"disk_memory":[]}' > data/memory.json; \
	fi
	@if [ ! -f data/context_keys.json ]; then \
		echo '{"keys":[]}' > data/context_keys.json; \
	fi

# Install (copy to /usr/local/bin)
install: $(BUILD_DIR)/$(TARGET)
	sudo cp $(BUILD_DIR)/$(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

# Format code
format:
	find $(SRC_DIR) -name "*.c" -o -name "*.h" | xargs clang-format -i

# Static analysis (if available)
check:
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --std=c99 $(SRC_DIR); \
	else \
		echo "cppcheck not available, skipping static analysis"; \
	fi

# Help
help:
	@echo "Available targets:"
	@echo "  all      - Build the project (default)"
	@echo "  debug    - Build with debug symbols"
	@echo "  clean    - Remove build files"
	@echo "  setup    - Create data directory and default files"
	@echo "  install  - Install to /usr/local/bin"
	@echo "  format   - Format source code"
	@echo "  check    - Run static analysis"
	@echo "  help     - Show this help"

.PHONY: all debug clean setup install uninstall format check help
