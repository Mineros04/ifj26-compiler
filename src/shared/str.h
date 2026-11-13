/**
 * @file str.h
 * @authors Adam Jaroš
 * @brief A dynamic string with auto-resizing capability. Backwards-compatible with standard C
 * char[].
 *
 * If a function was to fail an allocation, it will call the internal error handler itself, as
 * allocation errors are unrecoverable anyway.
 */

#pragma once

#include <stdarg.h>
#include <stddef.h>

/// Capacity of the first heap buffer allocated for a string created with STR_INIT.
#define STR_INITIAL_CAP 16

/**
 * @brief Dynamic, heap-allocated, null-terminated string.
 */
typedef struct {
    char *data; /// Null-terminated character buffer (owned only when cap > 0, never written to
                /// otherwise).
    size_t len; /// Length without the null terminator.
    size_t cap; /// Number of allocated bytes, including space for the null terminator.
} string_t;

/**
 * @brief Initializer for an empty string. No allocations are performed, cannot fail. Cannot be used
 * for static initializations.
 *
 * Meant for declarations with automatic storage and resets:
 * ```c
 * string_t s = STR_INIT;
 * s = STR_INIT;
 * ```
 */
#define STR_INIT ((string_t){.data = (char *)"", .len = 0, .cap = 0})

// ================================================================================================
// ===== Lifetime =================================================================================
// ================================================================================================

/**
 * @brief Initializes an empty string (same as `*s = STR_INIT`). Allocates nothing and
 * cannot fail.
 * @param s The pointer to which an empty string will be initialized.
 */
static inline void str_init(string_t *s) {
    *s = STR_INIT;
}

/**
 * @brief Initializes an empty string with at least `cap` characters of capacity (not counting the
 * null terminator). Terminates the program on allocation failure.
 * @param s The pointer to which an empty string will be initialized.
 * @param cap The minimum capacity of characters.
 *
 * @throws Error 99 - Allocation failed
 */
void str_init_cap(string_t *s, size_t cap);

/**
 * @brief Initializes the string as a copy of a C string. Terminates the program on allocation
 * failure.
 * @param s The pointer to which a string will be initialized.
 * @param src The C string which will be copied to the `s` owned string.
 *
 * @throws Error 99 - Allocation failed
 */
void str_init_from(string_t *s, const char *src);

/**
 * @brief Initializes the string as a copy of exactly the first `n` bytes of `src`. Terminates the
 * program on allocation failure.
 * @param s The pointer to which a string will be initialized.
 * @param src The buffer to copy from. It does not need to be null-terminated, but must have at
 * least `n` readable bytes.
 * @param n The number of bytes which will be copied over to `s`.
 *
 * @throws Error 99 - Allocation failed
 */
void str_init_from_n(string_t *s, const char *src, size_t n);

/**
 * @brief Releases the buffer and resets the string to the STR_INIT state, so it can be reused right
 * away. It is safe to call repeatedly.
 * @param s The pointer to an instance of `string_t` which will be freed.
 */
void str_free(string_t *s);

/**
 * @brief Detaches the internal buffer and transfers its ownership to the caller (who must free()
 * it). The string is reset to the STR_INIT state. Terminates the program on allocation failure.
 * @param s The pointer to string which will have its ownership transferred.
 *
 * Meant for handing the result over to structures storing plain `char *` without an extra copy. The
 * result is always free-able (if the string has a capacity of 0, an internal buffer will be
 * allocated).
 *
 * @returns The null-terminated heap buffer (never `nullptr`).
 *
 * @throws Error 99 - Allocation failed
 */
[[nodiscard]] char *str_to_owned(string_t *s);

/**
 * @brief Returns a newly malloc'd copy of the contents (caller frees). Terminates the program on
 * allocation failure.
 * @param s The pointer to a string which will be copied over into a C string.
 * @returns The copy of `s` (never `nullptr`).
 *
 * @throws Error 99 - Allocation failed
 */
[[nodiscard]] char *str_dup_cstr(const string_t *s);

// ================================================================================================
// ===== Capacity =================================================================================
// ================================================================================================

/**
 * @brief Ensures `s` can hold at least `min_cap` characters in total (not counting the null
 * terminator) without further reallocation. Does nothing if the capacity is already sufficient.
 * Terminates the program on allocation failure.
 * @param s The string which will have its capacity expanded, if needed.
 * @param min_cap The minimum total number of characters that `s` must be able to hold.
 *
 * @throws Error 99 - Allocation failed
 */
void str_reserve(string_t *s, size_t min_cap);

/**
 * @brief Sets length to 0 (keeps the allocated buffer for reuse).
 * @param s The pointer to string which will have its length reset.
 */
void str_clear(string_t *s);

/**
 * @brief Shortens the string to `new_len` characters. This function has no effect if `new_len` >=
 * current length.
 * @param s The pointer to string which will get truncated.
 * @param new_len The maximum amount of prefix characters that `s` retains after this operation is
 * executed.
 */
void str_truncate(string_t *s, size_t new_len);

// ================================================================================================
// ===== Appending ================================================================================
// ================================================================================================

/**
 * @brief Appends a single character. Terminates the program on allocation failure.
 * @param s The pointer to a string which will have the character appended.
 * @param c The character to be appended.
 *
 * @throws Error 99 - Allocation failed
 */
void str_push(string_t *s, char c);

/**
 * @brief Removes and returns the last character.
 * @param s The pointer to a string which will have its last character removed.
 * @returns The removed character, or '\0' if the string is empty.
 */
char str_pop(string_t *s);

/**
 * @brief Appends a C string. Terminates the program on allocation failure.
 * @param s The pointer to a string which will end up being extended.
 * @param src The C string which will be appended to `s`.
 *
 * @throws Error 99 - Allocation failed
 */
void str_append(string_t *s, const char *src);

/**
 * @brief Appends exactly the first `n` bytes of `src`. Terminates the program on allocation
 * failure.
 * @param s The pointer to a string which will end up being extended.
 * @param src The buffer to append from. It does not need to be null-terminated, but must have at
 * least `n` readable bytes.
 * @param n The number of bytes to be copied from `src` to `s`.
 *
 * @throws Error 99 - Allocation failed
 */
void str_append_n(string_t *s, const char *src, size_t n);

/**
 * @brief Appends the contents of another dynamic string. Terminates the program on allocation
 * failure.
 * @param s The pointer to a string which will end up being extended.
 * @param src The string which will be appended to `s`.
 *
 * @throws Error 99 - Allocation failed
 */
void str_append_str(string_t *s, const string_t *src);

/**
 * @brief Appends printf-style formatted text. Terminates the program on allocation failure.
 * @param s The pointer to a string which will be expanded.
 * @param fmt The format string.
 * @param ... The values to be formatted according to `fmt`.
 *
 * @throws Error 99 - Allocation failed
 */
[[gnu::format(printf, 2, 3)]]
void str_appendf(string_t *s, const char *fmt, ...);

/**
 * @brief va_list-compatible variant of `str_appendf`. Terminates the program on allocation
 * failure.
 * @param s The pointer to a string which will be expanded.
 * @param fmt The format string.
 * @param args The variadic arguments instance for formatting purposes.
 *
 * @throws Error 99 - Allocation failed
 */
[[gnu::format(printf, 2, 0)]]
void str_vappendf(string_t *s, const char *fmt, va_list args);

// ================================================================================================
// ===== Public string access & comparison ========================================================
// ================================================================================================

/**
 * @brief Returns the contents as a read-only C string (never `nullptr`).
 * @param s The string to be converted.
 * @returns The C string representation of `s`.
 *
 * This call is equivalent to `s->data`.
 */
const char *str_cstr(const string_t *s);

/**
 * @brief Returns the length (without the null terminator).
 * @param s The string to have its length read.
 * @returns The length of the string.
 *
 * This call is equivalent to `s->len`.
 */
size_t str_len(const string_t *s);

/**
 * @brief Returns true if the string has zero length.
 * @param s The string to check for emptiness.
 * @returns `true` if `s` is empty, `false` otherwise.
 *
 * This call is equivalent to `s->len == 0`.
 */
bool str_is_empty(const string_t *s);

/**
 * @brief Returns the last character, or '\0' if the string is empty.
 * @param s The string to read.
 * @returns The last character of `s`, or '\0' if `s` is empty.
 */
char str_last(const string_t *s);

/**
 * @brief Lexicographic comparison, same semantics as strcmp().
 * @param a The first string in the comparison.
 * @param b The string to lexicographically compare against.
 * @returns An integer whose sign signals the ordering (only the sign is meaningful):
 *          negative -> `a` orders before `b`: at the first differing position `a` has the
 *                      smaller byte (compared as `unsigned char`), or `a` is a proper prefix
 *                      of `b`.
 *          zero     -> The strings have identical contents.
 *          positive -> `a` orders after `b`: at the first differing position `a` has the
 *                      larger byte, or `b` is a proper prefix of `a`.
 */
int str_cmp(const string_t *a, const string_t *b);

/**
 * @brief Returns true if both strings have identical contents.
 * @param a The first string in the comparison.
 * @param b The string to compare against.
 * @returns `true` if the contents of both strings are equal, `false` otherwise.
 */
bool str_equals(const string_t *a, const string_t *b);

/**
 * @brief Returns true if the string's contents equal the C string `cstr`.
 * @param s The dynamic string to compare.
 * @param cstr The C string to compare against.
 * @returns `true` if the contents of `s` equal `cstr`, `false` otherwise.
 */
bool str_equals_cstr(const string_t *s, const char *cstr);
