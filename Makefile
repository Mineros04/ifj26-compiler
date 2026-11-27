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

PACKAGE_LOGIN = xkoment00
PACKAGE_ARCHIVE = dist/$(PACKAGE_LOGIN).zip

.SILENT: all build run clean build_test test docs package
.PHONY: all build run clean build_test test memcheck docs package

all: build

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

# Submission archive (assignment section 6). Sources must not be nested deeper
# than src/, so src/shared/ is flattened and `#include "shared/x.h"` is rewritten
# to `#include "x.h"` in the staged copy only. The archive is then verified by
# unpacking it and building it from scratch with plain `make`.
package: docs
	rm -rf dist/package dist/package_check
	rm -f $(PACKAGE_ARCHIVE)
	mkdir -p dist/package/src dist/package/tests
	dups=$$(for f in $(BUILD_SOURCES) $(BUILD_HEADERS); do basename $$f; done | sort | uniq -d); \
	if [ -n "$$dups" ]; then echo "package: duplicate source file names: $$dups" >&2; exit 1; fi
	cp $(BUILD_SOURCES) $(BUILD_HEADERS) dist/package/src/
	cp $(TEST_SOURCES) $(call find_files,tests,h) dist/package/tests/
	sed -i -E 's|#include "(\.\./src/)?shared/|#include "\1|' dist/package/src/* dist/package/tests/*
	cp Makefile docs/rozdeleni docs/rozsireni docs/dokumentace.pdf dist/package/
	if grep -q "$$(printf '\r')" dist/package/rozdeleni dist/package/rozsireni; then \
		echo "package: rozdeleni and rozsireni must use LF line endings" >&2; exit 1; fi
	cd dist/package && zip -q -r -X ../$(PACKAGE_LOGIN).zip .
	mkdir -p dist/package_check
	unzip -q $(PACKAGE_ARCHIVE) -d dist/package_check
	$(MAKE) -s -C dist/package_check
	test -x dist/package_check/$(BUILD_TARGET)
	rm -rf dist/package dist/package_check
	unzip -l $(PACKAGE_ARCHIVE)
	echo "Created $(PACKAGE_ARCHIVE)"