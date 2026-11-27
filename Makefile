CC = clang
CFLAGS = -std=c23 -Wall -Wextra -Wpedantic

# Recursive file lookup in a specified directory with an extension.
find_files = $(shell find $(1) -type f -name '*.$(2)' 2>/dev/null)

BUILD_SOURCES = $(call find_files,src,c)
BUILD_HEADERS = $(call find_files,src,h)
BUILD_TARGET = dist/compiler

TEST_SOURCES = $(call find_files,tests,c)
TEST_MODULES = $(filter-out src/main.c,$(BUILD_SOURCES))
TEST_HEADERS = $(call find_files,tests,h) $(BUILD_HEADERS)
TEST_TARGET = dist/test_runner

.SILENT: all build run clean build_test test docs
.PHONY: all build run clean build_test test memcheck docs

all: build run

build: $(BUILD_SOURCES) $(BUILD_HEADERS) Makefile
	mkdir -p dist
	$(CC) $(CFLAGS) -Isrc $(BUILD_SOURCES) -o $(BUILD_TARGET)

run:
	./$(BUILD_TARGET)

clean:
	rm -rf dist

build_test: $(TEST_SOURCES) $(TEST_MODULES) $(TEST_HEADERS) Makefile
	mkdir -p dist
	$(CC) $(CFLAGS) -g -Isrc -Itests $(TEST_SOURCES) $(TEST_MODULES) -o $(TEST_TARGET)

test: build_test
	./$(TEST_TARGET) $(TEST_ARGS)

memcheck:
	valgrind --leak-check=full $(BUILD_TARGET)

docs:
	dot -Tpng docs/lexer-fsm.dot -o docs/figures/lexer-fsm.png
	dot -Tpng docs/architecture.dot -o docs/figures/architecture.png
	typst compile docs/dokumentace.typ