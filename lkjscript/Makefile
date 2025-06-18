CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -g -O2
LDFLAGS = 
SRCDIR = .
TARGET = lkjscript
MAIN_SOURCE = lkjscript.c

# Source files (lkjscript.c includes all others)
SOURCES = $(MAIN_SOURCE) lkjscript.h tokenize.c token.c readsrc.c parse.c genbin.c exec.c

.PHONY: all clean install test

all: $(TARGET)

$(TARGET): $(MAIN_SOURCE) lkjscript.h
	$(CC) $(CFLAGS) -o $@ $(MAIN_SOURCE) $(LDFLAGS)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

test: $(TARGET)
	@echo "Testing lkjscript..."
	@mkdir -p script
	@echo 'fn main() -> i64 (' > script/main.lkjscript
	@echo '  var result : i64' >> script/main.lkjscript
	@echo '  &result = 42' >> script/main.lkjscript
	@echo '  return result' >> script/main.lkjscript
	@echo ')' >> script/main.lkjscript
	./$(TARGET)

debug: $(MAIN_SOURCE) lkjscript.h
	$(CC) $(CFLAGS) -DDEBUG -g -O0 -o $(TARGET) $(MAIN_SOURCE) $(LDFLAGS)

static: $(MAIN_SOURCE) lkjscript.h
	$(CC) $(CFLAGS) -static -o $(TARGET) $(MAIN_SOURCE) $(LDFLAGS)

help:
	@echo "Available targets:"
	@echo "  all     - Build lkjscript (default)"
	@echo "  clean   - Remove built files"
	@echo "  install - Install to /usr/local/bin"
	@echo "  test    - Build and run a simple test"
	@echo "  debug   - Build with debug symbols and no optimization"
	@echo "  static  - Build statically linked binary"
	@echo "  help    - Show this help message"
