CC = gcc
CFLAGS = -Werror -Wall -Wextra -std=c11 -O2 -march=native -g
LDFLAGS = -static
INCLUDES = -Isrc/
LIBS = 

SRC_DIR = src
INCLUDE_DIR = src/
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

SRCS = $(shell find $(SRC_DIR) -name "*.c")

OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

TARGET = $(BUILD_DIR)/lkjscript

HEADERS = $(wildcard $(INCLUDE_DIR)/*.h)

all: $(TARGET)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: all
