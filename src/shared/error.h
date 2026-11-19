/**
 * @file error.h
 * @authors Adam Jaroš
 * @brief Centralized error handling for the compiler, covering both compilation errors and
 * internal errors. Also includes safe memory allocation wrappers to simplify error handling inside
 * compiler logic.
 */

#pragma once

#include <stddef.h>

/**
 * @brief Throws an internal compiler error if `cond` evaluates to false.
 * @param cond The condition expected to be true.
 *
 * The error message contains the source file, line and the stringified condition.
 */
#define ASSERT_COND(cond)                                                                          \
    do {                                                                                           \
        if (!(cond))                                                                               \
            error_throw_internal("%s:%d: assertion '%s' failed", __FILE__, __LINE__, #cond);       \
    } while (0)

/**
 * @brief Marks a code path that must never be reached. Throws an internal compiler error if it is.
 *
 */
#define UNREACHABLE() error_throw_internal("%s:%d: unreachable", __FILE__, __LINE__)

/**
 * @brief All possible exit states that the compiler may produce.
 *
 * The enumerated value of each state is used as the compiler's exit code. Every state except for
 * `ERR_SUCCESS` represents an error that terminates the program when thrown.
 */
typedef enum {
    /// @brief A non-error, indicating successful compilation.
    ERR_SUCCESS = 0,
    /// @brief A lexical error, produced during the lexical analysis phase. This may indicate
    /// invalid structure of a lexeme, a comment or indentation.
    ERR_LEXICAL = 1,
    /// @brief A syntax error, produced during the syntax analysis phase. This may indicate
    /// incorrect syntax, a missing prologue, an invalid language construct, or an indentation
    /// structure violation.
    ERR_SYNTAX = 2,
    /// @brief A semantic error, signifying an undefined function, variable, or type, or a
    /// missing function with the `@main` annotation.
    ERR_SEM_UNDEF = 3,
    /// @brief A semantic error, signifying forbidden redefinition of a function, variable, or type.
    ERR_SEM_REDEF = 4,
    /// @brief A semantic error, signifying an invalid argument count or argument types in a
    /// function call, or forbidden parameters declared in the function with the `@main` annotation.
    ERR_SEM_INVALID_ARG = 5,
    /// @brief A semantic error, signifying type incompatibility or invalid type inference in an
    /// expression, assignment, initialization, return value, or control structure.
    ERR_SEM_TYPE_MISMATCH = 6,
    /// @brief A semantic error, signifying an attempt to write to an immutable variable or a formal
    /// function parameter.
    ERR_SEM_WRITE_IMMUT = 7,
    /// @brief A semantic error, signifying any other error not covered by errors 3-7. This may
    /// indicate a forbidden return statement, a non-exhaustive match statement, or a zero-step
    /// range.
    ERR_SEM_OTHER = 10,
    /// @brief An internal compiler error not influenced by the compiled source code, typically an
    /// allocation failure.
    ERR_COMPILER_INTERNAL = 99
} error_code_t;

/**
 * @brief Throws a compilation error and immediately terminates the program.
 * @param err The compilation error to throw. Must not be `ERR_SUCCESS`. Also
 * used as the program's exit code.
 * @param line The line (counted from 1) at which an error occurred. When set to 0, the line is
 * considered to be unknown.
 * @param col The column (counted from 1) at which the error occurred. When set to 0, the column is
 * considered to be unknown.
 * @param fmt The format string.
 * @param ... Arguments for the format string.
 *
 * @throws Error specified by argument `err`. The formatted message is printed to `stderr`.
 */
[[noreturn, gnu::format(printf, 4, 5)]]
void error_throw_compile(error_code_t err, size_t line, size_t col, const char *fmt, ...);

/**
 * @brief Throws an internal compiler error and immediately terminates the program.
 * @param fmt The format string.
 * @param ... Arguments for the format string.
 *
 * @note The behavior is equivalent to the following call:
 * ```c
 * error_throw_compile(ERR_COMPILER_INTERNAL, 0, 0, fmt, ...);
 * ```
 *
 * @throws Error 99 - `evaluated fmt`
 */
[[noreturn, gnu::format(printf, 1, 2)]]
void error_throw_internal(const char *fmt, ...);

/**
 * @brief Returns the human-readable name of an error.
 * @param err The error to translate.
 *
 * @returns A statically allocated C string with the name of `err`. The string must not be freed or
 * modified.
 *
 * @throws `UNREACHABLE()` if `err` is not a valid `error_code_t` value
 */
[[nodiscard]] const char *error_getname(error_code_t err);

// =================================================================================================
// ===== Safe Memory Allocation ====================================================================
// =================================================================================================

/**
 * @brief Allocates a contiguous, uninitialized block of `size` bytes on the heap.
 * @param size The number of bytes to allocate. May be 0.
 * @note If `size` is 0, it is set to 1 by the function. The returned block is then considered
 * empty; it may be passed to `free` or `safe_realloc`, but its contents must not be accessed.
 *
 * @returns A pointer to the allocated memory, guaranteed to be non-`nullptr`. Must be released with
 * `free`.
 *
 * @throws Error 99 - Allocation failed.
 */
[[nodiscard]] void *safe_malloc(size_t size);

/**
 * @brief Allocates a contiguous, zero-initialized block on the heap, large enough to hold `count`
 * elements of `size` bytes each.
 * @param count The number of elements to allocate. May be 0.
 * @param size The size of each element in bytes. May be 0.
 * @note If either `count` or `size` is 0, both are set to 1 by the function. The returned block is
 * then considered empty; it may be passed to `free` or `safe_realloc`, but its contents must not be
 * accessed.
 *
 * @returns A pointer to the allocated memory, guaranteed to be non-`nullptr`. Must be released with
 * `free`.
 *
 * @throws Error 99 - Allocation failed, including when `count * size` overflows `size_t`.
 */
[[nodiscard]] void *safe_calloc(size_t count, size_t size);

/**
 * @brief Resizes a previously allocated memory block, potentially moving it.
 * @param ptr The memory block to resize. Must be either `nullptr`, or a pointer previously returned
 * by `safe_malloc`, `safe_calloc` or `safe_realloc` that has not been freed yet. If `nullptr`, the
 * call is equivalent to `safe_malloc(size)`.
 * @param size The new size of the block in bytes. May be 0.
 * @note If `size` is 0, it is set to 1 by the function, thus, a zero-size request never acts as
 * `free(ptr)`. The returned block is then considered empty; it may be passed to `free` or
 * `safe_realloc`, but its contents must not be accessed.
 *
 * @returns A pointer to the resized block, guaranteed to be non-`nullptr`. The contents of the
 * original block are preserved up to the smaller of the old and new sizes. Any newly added bytes
 * are uninitialized. Must be released with `free`.
 *
 * @warning After the call, `ptr` must no longer be used (not even compared with the returned
 * pointer). Use the returned pointer instead.
 *
 * @throws Error 99 - Allocation failed.
 */
[[nodiscard]] void *safe_realloc(void *ptr, size_t size);
