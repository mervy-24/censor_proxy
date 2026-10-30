
CC	:= gcc
CFLAGS	:= -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude -g

DEPFLAGS := -MMD -MP

SRC_DIR		:= src
BUILD_DIR	:= build
BIN_DIR		:= bin

PROXY_OBJS	:= $(BUILD_DIR)/proxy.o $(BUILD_DIR)/censor.o
CLIENT_OBJS	:= $(BUILD_DIR)/client.o
TEST_OBJS	:= $(BUILD_DIR)/test_censor.o $(BUILD_DIR)/censor.o

.PHONY: all test clean

all: $(BIN_DIR)/proxy $(BIN_DIR)/client

$(BIN_DIR)/proxy: $(PROXY_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_DIR)/client: $(CLIENT_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_DIR)/test_censor: $(TEST_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

test: $(BIN_DIR)/test_censor
	./$(BIN_DIR)/test_censor

# Compile:
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: tests/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BIN_DIR) $(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

-include $(wildcard $(BUILD_DIR)/*.d)

