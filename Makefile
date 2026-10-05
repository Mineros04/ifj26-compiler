CC = clang
CFLAGS = -std=c17 -Wall -Wextra -Wpedantic
SOURCES = $(wildcard src/*.c)

.SILENT: all build run clean
.PHONY: all run clean

all: build run

build: $(SOURCES) $(wildcard src/*.h)
	mkdir -p dist
	$(CC) $(CFLAGS) $(SOURCES) -o dist/compiler

run:
	./dist/compiler

clean:
	rm -rf dist
