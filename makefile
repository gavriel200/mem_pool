CC = gcc
CFLAGS = -Wall -Wextra -Werror -DDEBUG -g -I./src

SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build

SRC_FILES = $(SRC_DIR)/mem_pool.c
TEST_FILES = $(TEST_DIR)/test_mem_pool.c

TEST_EXEC = $(BUILD_DIR)/test_mem_pool

all: run_test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TEST_EXEC): $(SRC_FILES) $(TEST_FILES) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRC_FILES) $(TEST_FILES) -o $(TEST_EXEC)

run_test: $(TEST_EXEC)
	./$(TEST_EXEC)

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean all
