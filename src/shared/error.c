/**
 * @file error.c
 * @authors Adam Jaroš
 * @brief Centralized error handling for the compiler, covering both compilation errors and
 * internal errors. Also includes safe memory allocation wrappers to simplify error handling inside
 * compiler logic.
 */

#include "error.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// ===== Internal =====

/**
 * @brief Prints a formatted error message to `stderr` and terminates the program.
 * @param err The error to throw / print. Also used as the program's exit code.
 * @param line The line (counted from 1) at which the error occurred, or 0 if unknown.
 * @param col The column (counted from 1) at which the error occurred, or 0 if unknown. Ignored
 * when `line` is 0.
 * @param fmt The format string of the error message.
 * @param args Arguments for the format string, already initialized by `va_start`.
 *
 * The message has the form `<type>[: <name>][ (line L[, col C])]: <message>`, where the error name
 * is omitted for `ERR_COMPILER_INTERNAL` and the position is omitted when unknown.
 *
 * @note Serves as the shared implementation of `error_throw_compile` and `error_throw_internal`.
 * Must not call anything that may throw an error itself, to prevent infinite recursion.
 */
[[noreturn, gnu::format(printf, 4, 0)]]
static void throw_va(error_code_t err, size_t line, size_t col, const char *fmt, va_list args) {
    const char *err_type = err == ERR_COMPILER_INTERNAL ? "Internal Error" : "Compilation Error";

    fputs(err_type, stderr);
    if (err != ERR_COMPILER_INTERNAL) {
        fprintf(stderr, ": %s", error_getname(err));
    }

    // Either both line and col are passed, or only line, or nothing at all.
    if (line != 0) {
        fprintf(stderr, " (line %zu", line);
        if (col != 0) {
            fprintf(stderr, ", col %zu", col);
        }
        fputc(')', stderr);
    }

    fputs(": ", stderr);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);

    exit(err);
}

// ===== Error Handling =====

void error_throw_compile(error_code_t err, size_t line, size_t col, const char *fmt, ...) {
    ASSERT_COND(err != ERR_SUCCESS);

    va_list args;
    va_start(args);

    throw_va(err, line, col, fmt, args);

    // No need to call va_end, as any code after the error function call is unreachable.
}

void error_throw_internal(const char *fmt, ...) {
    va_list args;
    va_start(args);

    throw_va(ERR_COMPILER_INTERNAL, 0, 0, fmt, args);

    // No need to call va_end, as any code after the error function call is unreachable.
}

const char *error_getname(error_code_t err) {
    switch (err) {
    case ERR_SUCCESS:
        return "Success";
    case ERR_LEXICAL:
        return "Lexical error";
    case ERR_SYNTAX:
        return "Syntax error";
    case ERR_SEM_UNDEF:
        return "Undefined symbol";
    case ERR_SEM_REDEF:
        return "Illegal redefinition";
    case ERR_SEM_INVALID_ARG:
        return "Invalid argument(s)";
    case ERR_SEM_TYPE_MISMATCH:
        return "Incompatible type(s)";
    case ERR_SEM_WRITE_IMMUT:
        return "Immutable write";
    case ERR_SEM_OTHER:
        return "Semantic error";
    case ERR_COMPILER_INTERNAL:
        return "Internal error";
    }

    // No default on purpose, so that compiler catches missing cases.
    UNREACHABLE();
}

