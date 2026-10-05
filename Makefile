CC = clang
CFLAGS = -std=c17 -Wall -Wextra -Wpedantic

BUILD_SOURCES = $(wildcard src/*.c)
BUILD_TARGET = dist/compiler

TEST_SOURCES = $(wildcard tests/*.c)
TEST_MODULES = $(filter-out src/main.c,$(BUILD_SOURCES))
TEST_HEADERS = $(wildcard tests/*.h src/*.h)
TEST_TARGET = dist/test_runner

.SILENT: all build run clean build_test test
.PHONY: all run clean build_test test

all: build run

build: $(BUILD_SOURCES) $(wildcard src/*.h)
	mkdir -p dist
	$(CC) $(CFLAGS) $(BUILD_SOURCES) -o $(BUILD_TARGET)

run:
	./$(BUILD_TARGET)

clean:
	rm -rf dist

build_test: $(TEST_SOURCES) $(TEST_MODULES) $(TEST_HEADERS) Makefile
	mkdir -p dist
	$(CC) $(CFLAGS) -g -Isrc -Itests $(TEST_SOURCES) $(TEST_MODULES) -o $(TEST_TARGET)

test: build_test
	./$(TEST_TARGET) $(TEST_ARGS)
