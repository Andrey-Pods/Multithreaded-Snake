# CSE 320 HW2 - Snake Server Makefile
# DO NOT MODIFY THIS FILE

CC       = gcc
CFLAGS   = -Wall -Wextra -Werror -Wno-unused-parameter -std=gnu11 -I include
LDFLAGS  = -lpthread

# Source files
SRC_DIR  = src
SRC      = $(SRC_DIR)/main.c \
           $(SRC_DIR)/game_board.c \
           $(SRC_DIR)/snake.c \
           $(SRC_DIR)/protocol.c \
           $(SRC_DIR)/server.c

# Object files
OBJ_DIR  = build
OBJ      = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Test source files (Criterion) — exclude main.c
TEST_DIR = tests
TEST_SRC = $(TEST_DIR)/test_snake.c \
           $(TEST_DIR)/test_board.c \
           $(TEST_DIR)/test_protocol.c
LIB_SRC  = $(SRC_DIR)/game_board.c \
           $(SRC_DIR)/snake.c \
           $(SRC_DIR)/protocol.c \
           $(SRC_DIR)/server.c
LIB_OBJ  = $(LIB_SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TEST_OBJ = $(TEST_SRC:$(TEST_DIR)/%.c=$(OBJ_DIR)/%.o)

# Executables
BIN_DIR  = bin
TARGET   = $(BIN_DIR)/snake_server
TEST_BIN = $(BIN_DIR)/snake_test

# ---- Targets ---------------------------------------------------------------

.PHONY: all debug test clean

all: CFLAGS += -O2
all: $(TARGET)

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

test: CFLAGS += -g -DDEBUG
test: $(TEST_BIN)

# ---- Linking ----------------------------------------------------------------

$(TARGET): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_BIN): $(LIB_OBJ) $(TEST_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lcriterion

# ---- Compilation ------------------------------------------------------------

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(TEST_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# ---- Directories ------------------------------------------------------------

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# ---- Clean ------------------------------------------------------------------

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
