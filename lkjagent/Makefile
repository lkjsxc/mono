CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
SRCDIR = src
TESTDIR = test
BUILDDIR = build

# All source files for the library organized by functionality

# Core agent management
CORE_SOURCES = $(SRCDIR)/core/agent_core.c \
               $(SRCDIR)/core/agent_runner.c

# Configuration management
CONFIG_SOURCES = $(SRCDIR)/config.c

# State management system
STATE_SOURCES = $(SRCDIR)/state/state_manager.c \
                $(SRCDIR)/state/state_thinking.c \
                $(SRCDIR)/state/state_executing.c \
                $(SRCDIR)/state/state_evaluating.c \
                $(SRCDIR)/state/state_paging.c

# Memory management
MEMORY_SOURCES = $(SRCDIR)/memory/memory_manager.c

# Tool system
TOOL_SOURCES = $(SRCDIR)/tools/agent_tools.c

# API integration
API_SOURCES = $(SRCDIR)/api/lmstudio_api.c

# Utility functions
UTIL_SOURCES = $(SRCDIR)/utils/error.c \
               $(SRCDIR)/utils/file.c \
               $(SRCDIR)/utils/http.c \
               $(SRCDIR)/utils/json.c \
               $(SRCDIR)/utils/token.c

# Legacy agent file (to be phased out)
LEGACY_SOURCES = $(SRCDIR)/agent.c

# Combined library sources (without legacy agent.c)
LIB_SOURCES = $(CORE_SOURCES) \
              $(CONFIG_SOURCES) \
              $(STATE_SOURCES) \
              $(MEMORY_SOURCES) \
              $(TOOL_SOURCES) \
              $(API_SOURCES) \
              $(UTIL_SOURCES)

# Main application
MAIN_SOURCE = $(SRCDIR)/main.c
TARGET = $(BUILDDIR)/lkjagent

.PHONY: all clean test test-json

all: $(TARGET)

$(TARGET): $(MAIN_SOURCE) $(LIB_SOURCES) | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(MAIN_SOURCE) $(LIB_SOURCES)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

test: $(BUILDDIR)/test_comprehensive
	$(BUILDDIR)/test_comprehensive

test-json: $(BUILDDIR)/test_json
	$(BUILDDIR)/test_json

$(BUILDDIR)/test_comprehensive: $(TESTDIR)/test_comprehensive.c $(LIB_SOURCES) | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $(BUILDDIR)/test_comprehensive $(TESTDIR)/test_comprehensive.c $(LIB_SOURCES)

$(BUILDDIR)/test_post: $(TESTDIR)/test_post.c $(LIB_SOURCES) | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $(BUILDDIR)/test_post $(TESTDIR)/test_post.c $(LIB_SOURCES)

$(BUILDDIR)/test_json: $(TESTDIR)/test_json.c $(LIB_SOURCES) | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $(BUILDDIR)/test_json $(TESTDIR)/test_json.c $(LIB_SOURCES)

clean:
	rm -rf $(BUILDDIR)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/lkjagent

.PHONY: help

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/lkjagent

.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all              - Build the main lkjagent binary in build/"
	@echo "  test             - Build and run comprehensive tests"
	@echo "  test_comprehensive - Build comprehensive test binary in build/"
	@echo "  test_post        - Build POST test binary in build/"
	@echo "  clean            - Remove build directory and all binaries"
	@echo "  install          - Install lkjagent to /usr/local/bin"
	@echo "  help             - Show this help message"
