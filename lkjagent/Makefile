CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
SRCDIR = src
TESTDIR = test
BUILDDIR = build
SOURCES = $(SRCDIR)/main.c
TARGET = $(BUILDDIR)/lkjagent

.PHONY: all clean test test-json

all: $(TARGET)

$(TARGET): $(SOURCES) | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

test: $(BUILDDIR)/test_comprehensive
	$(BUILDDIR)/test_comprehensive

test-json: $(BUILDDIR)/test_json
	$(BUILDDIR)/test_json

$(BUILDDIR)/test_comprehensive: $(TESTDIR)/test_comprehensive.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $(BUILDDIR)/test_comprehensive $(TESTDIR)/test_comprehensive.c

$(BUILDDIR)/test_post: $(TESTDIR)/test_post.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $(BUILDDIR)/test_post $(TESTDIR)/test_post.c

$(BUILDDIR)/test_json: $(TESTDIR)/test_json.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $(BUILDDIR)/test_json $(TESTDIR)/test_json.c

clean:
	rm -rf $(BUILDDIR)

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
