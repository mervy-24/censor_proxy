
CC	:= gcc
CFLAGS	:= -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude -g

DEPFLAGS := -MMD -MP

SRC_DIR		:= src
BUILD_DIR	:= build
BIN_DIR		:= bin

PROXY_OBJS	:= $(BUILD_DIR)/proxy.o $(BUILD_DIR)/censor.o

.PHONY: all test clean

all: $(BIN_DIR)/proxy

$(BIN_DIR)/proxy: $(PROXY_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Compile:
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BIN_DIR) $(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

-include $(wildcard $(BUILD_DIR)/*.d)

