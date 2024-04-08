CC = gcc
CFLAGS = -Wall -Werror -g -o0
TARGET = test.out
SRC_DIR = .
BUILD_DIR = build


SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean

all: $(BUILD_DIR) $(TARGET)

full: clean all test

full_mem: clean all mem_check_test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

test:
	./$(TARGET)

mem_check_test:
	valgrind --leak-check=yes --error-exitcode=1 --quiet ./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
