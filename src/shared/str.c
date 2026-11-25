/**
 * @file str.h
 * @authors Adam Jaroš
 * @brief A dynamic string with auto-resizing capability. Backwards-compatible with standard C
 * char[].
 *
 * If a function was to fail an allocation, it will call the internal error handler itself, as
 * allocation errors are unrecoverable anyway.
 */

#include "str.h"

#include "error.h"
#include <stdlib.h>
#include <string.h>

// ================================================================================================
// ===== Lifetime =================================================================================
// ================================================================================================

void str_init_cap(string_t *s, size_t cap) {
    char *data = safe_malloc(cap + 1);
    data[0]    = '\0';
    *s         = (string_t){
        .data = data,
        .len  = 0,
        .cap  = cap + 1,
    };
}

void str_init_from(string_t *s, const char *src) {
    str_init_from_n(s, src, strlen(src));
}

void str_init_from_n(string_t *s, const char *src, size_t n) {
    char *data = safe_malloc(n + 1);

    memcpy(data, src, n);
    data[n] = '\0';

    *s = (string_t){
        .data = data,
        .len  = n,
        .cap  = n + 1,
    };
}

void str_free(string_t *s) {
    if (s->cap > 0) {
        free(s->data);
    }
    *s = STR_INIT;
}

char *str_to_owned(string_t *s) {
    if (s->cap == 0) {
        *s = STR_INIT;

        // Garbage memory to satisfy the "always free-able" contract.
        return safe_calloc(1, sizeof(char));
    }

    char *data = s->data;
    *s         = STR_INIT;

    return data;
}

char *str_dup_cstr(const string_t *s) {
    char *data = safe_malloc(s->len + 1);
    memcpy(data, s->data, s->len + 1);

    return data;
}

