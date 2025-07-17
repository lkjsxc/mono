# LKJAgent Makefile
# Builds the autonomous AI agent with zero external dependencies

# Compiler and flags
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -O2 -g
LDFLAGS = -Wl,-z,stack-size=1073741824

# Directories
SRCDIR = src
BUILDDIR = build
UTILSDIR = $(SRCDIR)/utils
AGENTDIR = $(SRCDIR)/agent

# Target executable
TARGET = $(BUILDDIR)/lkjagent
TEST_TARGET = $(BUILDDIR)/simple_test

# Source files organized by module
MAIN_SOURCES = $(SRCDIR)/main.c
AGENT_SOURCES = $(AGENTDIR)/core.c
UTILS_SOURCES = $(UTILSDIR)/token.c $(UTILSDIR)/file.c $(UTILSDIR)/http.c $(UTILSDIR)/json.c
CONFIG_SOURCES = $(SRCDIR)/config.c

# All source files
ALL_SOURCES = $(MAIN_SOURCES) $(AGENT_SOURCES) $(UTILS_SOURCES) $(CONFIG_SOURCES)

# Object files
OBJECTS = $(ALL_SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

# Default target
all: $(TARGET)

# Create build directory
$(BUILDDIR):
	mkdir -p $(BUILDDIR)
	mkdir -p $(BUILDDIR)/agent
	mkdir -p $(BUILDDIR)/utils

# Build main executable
$(TARGET): $(BUILDDIR) $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Built $(TARGET) successfully"

# Compile source files to object files
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Test target (build simple test)
test: $(TEST_TARGET)

$(TEST_TARGET): $(BUILDDIR) $(filter-out $(BUILDDIR)/main.o, $(OBJECTS))
	@echo "int main() { printf(\"Test build successful\\n\"); return 0; }" > $(BUILDDIR)/test_main.c
	$(CC) $(CFLAGS) -c $(BUILDDIR)/test_main.c -o $(BUILDDIR)/test_main.o
	$(CC) $(filter-out $(BUILDDIR)/main.o, $(OBJECTS)) $(BUILDDIR)/test_main.o -o $(TEST_TARGET) $(LDFLAGS)
	@echo "Built $(TEST_TARGET) successfully"

# Run the agent
run: $(TARGET)
	./$(TARGET)

# Run tests
check: $(TEST_TARGET)
	./$(TEST_TARGET)

# Clean build artifacts
clean:
	rm -rf $(BUILDDIR)
	@echo "Cleaned build directory"

# Install (copy to /usr/local/bin)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/lkjagent
	@echo "Installed lkjagent to /usr/local/bin"

# Uninstall
uninstall:
	sudo rm -f /usr/local/bin/lkjagent
	@echo "Uninstalled lkjagent"

# Show compilation database for IDE support
compiledb:
	bear -- make clean all

# Debug build
debug: CFLAGS += -DDEBUG -O0
debug: $(TARGET)

# Release build
release: CFLAGS += -DNDEBUG -O3
release: clean $(TARGET)

# Static analysis with cppcheck (if available)
static-analysis:
	@which cppcheck > /dev/null 2>&1 || (echo "cppcheck not found, skipping static analysis"; exit 0)
	cppcheck --enable=all --std=c11 --suppress=missingIncludeSystem $(SRCDIR)

# Memory check with valgrind (if available)
memcheck: $(TARGET)
	@which valgrind > /dev/null 2>&1 || (echo "valgrind not found, skipping memory check"; exit 0)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

# Show help
help:
	@echo "LKJAgent Build System"
	@echo "====================="
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build the main executable (default)"
	@echo "  test         - Build test executable"
	@echo "  run          - Build and run the agent"
	@echo "  check        - Build and run tests"
	@echo "  clean        - Remove build artifacts"
	@echo "  debug        - Build with debug symbols and no optimization"
	@echo "  release      - Build optimized release version"
	@echo "  install      - Install to /usr/local/bin"
	@echo "  uninstall    - Remove from /usr/local/bin"
	@echo "  compiledb    - Generate compile_commands.json for IDE support"
	@echo "  static-analysis - Run cppcheck static analysis"
	@echo "  memcheck     - Run valgrind memory check"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make                 # Build the agent"
	@echo "  make run             # Build and run"
	@echo "  make debug run       # Build debug version and run"
	@echo "  make clean release   # Clean build and make optimized version"

# Phony targets
.PHONY: all test run check clean install uninstall debug release static-analysis memcheck help compiledb

# Dependencies (header files)
$(OBJECTS): $(SRCDIR)/lkjagent.h
