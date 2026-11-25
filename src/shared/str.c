/**
 * @file str.c
 * @authors Adam Jaroš
 * @brief A dynamic string with auto-resizing capability. Backwards-compatible with standard C
 * char[].
 *
 * If a function was to fail an allocation, it will call the internal error handler itself, as
 * allocation errors are unrecoverable anyway.
 */

#include "str.h"

#include "error.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ===== Internal =====

static inline size_t min_size(size_t a, size_t b) {
    return a < b ? a : b;
}

static inline size_t max_size(size_t a, size_t b) {
    return a > b ? a : b;
}

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

// ================================================================================================
// ===== Capacity =================================================================================
// ================================================================================================

void str_reserve(string_t *s, size_t min_cap) {
    if (min_cap < s->cap) {
        return;
    }

    size_t new_cap = max_size(min_cap + 1, max_size(s->cap * 2, STR_INITIAL_CAP));

    if (s->cap == 0) {
        s->data    = safe_malloc(new_cap);
        s->data[0] = '\0';
    } else {
        s->data = safe_realloc(s->data, new_cap);
    }
    s->cap = new_cap;
}

void str_clear(string_t *s) {
    if (s->cap > 0) {
        s->data[0] = '\0';
    }

    s->len = 0;
}

void str_truncate(string_t *s, size_t new_len) {
    if (new_len >= s->len) {
        return;
    }

    s->data[new_len] = '\0';
    s->len           = new_len;
}

// ================================================================================================
// ===== Appending ================================================================================
// ================================================================================================

void str_push(string_t *s, char c) {
    str_reserve(s, s->len + 1);

    s->data[s->len++] = c;
    s->data[s->len]   = '\0';
}

char str_pop(string_t *s) {
    if (s->len == 0) {
        return '\0';
    }
    char c          = s->data[--s->len];
    s->data[s->len] = '\0';

    return c;
}

void str_append(string_t *s, const char *src) {
    str_append_n(s, src, strlen(src));
}

void str_append_n(string_t *s, const char *src, size_t n) {
    if (n == 0) {
        return;
    }

    // If src points inside of s->data, we remember only the offset from beginning of the buffer, so
    // that if str_reserve moves the buffer, the pointer dereference doesn't result in
    // use-after-free.
    bool self_ref = s->cap > 0 && src >= s->data && src < s->data + s->cap;
    size_t offset = self_ref ? (size_t)(src - s->data) : 0;

    str_reserve(s, s->len + n);

    if (self_ref) {
        src = s->data + offset;
    }

    memcpy(s->data + s->len, src, n);

    s->len += n;
    s->data[s->len] = '\0';
}

void str_append_str(string_t *s, const string_t *src) {
    str_append_n(s, src->data, src->len);
}

void str_appendf(string_t *s, const char *fmt, ...) {
    va_list args;
    va_start(args);

    str_vappendf(s, fmt, args);

    va_end(args);
}

void str_vappendf(string_t *s, const char *fmt, va_list args) {
    // Source: https://linux.die.net/man/3/vsnprintf

    // va_list can be iterated only once, hence we have to copy it and call vsnprintf with zero
    // length to retrieve the size of the later-evaluated format string.
    va_list vargs;
    va_copy(vargs, args);

    int n = vsnprintf(nullptr, 0, fmt, vargs);

    va_end(vargs);

    if (n < 0) {
        error_throw_internal("str_vappendf: formatting failed");
    }

    str_reserve(s, s->len + (size_t)n);
    vsnprintf(s->data + s->len, s->cap - s->len, fmt, args);
    s->len += (size_t)n;
}

// ================================================================================================
// ===== Public string access & comparison ========================================================
// ================================================================================================

const char *str_cstr(const string_t *s) {
    return s->data;
}

size_t str_len(const string_t *s) {
    return s->len;
}

bool str_is_empty(const string_t *s) {
    return s->len == 0;
}

char str_last(const string_t *s) {
    return s->len == 0 ? '\0' : s->data[s->len - 1];
}

int str_cmp(const string_t *a, const string_t *b) {
    size_t min_len = min_size(a->len, b->len);

    int diff = memcmp(a->data, b->data, min_len);
    if (diff != 0) {
        return diff;
    }

    // a is a proper prefix of b.
    if (a->len < b->len) {
        return -1;
    }

    // b is a proper prefix of a.
    if (a->len > b->len) {
        return 1;
    }

    return 0;
}

bool str_equals(const string_t *a, const string_t *b) {
    // We can avoid check for lower buffer length, since the AND operator short-circuits.
    return a->len == b->len && memcmp(a->data, b->data, a->len) == 0;
}

bool str_equals_cstr(const string_t *s, const char *cstr) {
    // Same short circuiting as above.
    return s->len == strlen(cstr) && memcmp(s->data, cstr, s->len) == 0;
}
