/**
 * @file hashtable.c
 * @authors Tomáš Komenda (xkoment00)
 * @brief Hash table implementation, according to TRP-izp specification (open addressing)
 */

#include "hashtable.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h> // malloc, calloc, free
#include <string.h> // strcmp, strdup

#define HASHTABLE_MIN_CAPACITY 2
#define HASHTABLE_EXPANSION_FACTOR 2

// the load limit must stay below 1, we use 0.7 here
#define HASHTABLE_MAX_LOAD_NUM 7
#define HASHTABLE_MAX_LOAD_DEN 10

typedef struct slot {
    char *key; // if key is NULL -> slot is empty
    void *value;
} slot_t;

struct hashtable {
    slot_t *items;   // item array
    size_t capacity; // hash table capacity
    size_t count;    // item count
};

/**
 * @brief Fowler–Noll–Vo (FNV-1a, 64-bit) hash function.
 * @param str String to generate hash key for.
 * @returns Unreduced hash for string.
 * @note Sources: Wikipedia; Glenn Fowler, Landon Curt Noll & Kiem-Phong Vo
 * https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function#FNV-1a_hash
 * (accessed 17.11.2026)
 */
static uint64_t fnv_hash(const char *str) {
    // convert to unsigned string
    const unsigned char *string = (const unsigned char *)str;

    // generate hash, 64bit values are used
    uint64_t hash = 14695981039346656037ULL;
    unsigned c;
    while ((c = *string++) != 0) {
        hash = (hash ^ c) * 1099511628211ULL;
    }

    return hash;
}

/**
 * @brief Finds the slot or the first empty slot in probe sequence.
 * @param item_array Array of slots.
 * @param capacity Array capacity.
 * @param key Key to look up (non-NULL, not empty).
 * @returns Slot holding the key, or first empty slot in probe sequence.
 * @attention Array must contain at least one empty slot & capacity must be
 * at least 1 (modulo division by zero).
 */
static slot_t *item_probe(slot_t *item_array, size_t capacity, const char *key) {
    // reduce hash for key, get start index of array
    size_t idx = fnv_hash(key) % capacity;

    for (size_t i = 0; i < capacity; i++) {
        if (!item_array[idx].key || strcmp(key, item_array[idx].key) == 0) {
            return &item_array[idx];
        }
        idx = (idx + 1) % capacity; // advance
    }

    assert(!"hashtable: table has no empty slot in item_probe function");
    return nullptr;
}

/**
 * @brief Helper function for table expansion.
 * @param table Hashtable to expand.
 * @returns true if expansion was successful, false otherwise
 * @note Expansion is done by doubling the capacity. Contents are left
 * unchanged on failure. Any slot pointer obtained before the call
 * is invalidated.
 */
static bool table_expand(Hashtable *table) {
    // size_t overflow protection, should never fail (memory would run out before this fails)
    if (table->capacity >= (SIZE_MAX / HASHTABLE_EXPANSION_FACTOR)) {
        return false;
    }

    size_t new_capacity = table->capacity * HASHTABLE_EXPANSION_FACTOR;

    // allocate new slot array
    slot_t *new_array = calloc(new_capacity, sizeof(slot_t));
    if (!new_array) {
        return false;
    }

    for (size_t i = 0; i < table->capacity; i++) {
        if (table->items[i].key) {
            slot_t *dest = item_probe(new_array, new_capacity, table->items[i].key);
            assert(dest);
            assert(dest->key == nullptr);
            *dest = table->items[i];
        }
    }
    free(table->items);

    table->items = new_array;
    table->capacity = new_capacity;
    return true;
}

Hashtable *hashtable_create(size_t init_capacity) {
    if (init_capacity < HASHTABLE_MIN_CAPACITY) {
        // capacity less than minimal capacity, not acceptable for hashtable
        return nullptr;
    }

    Hashtable *new_table = malloc(sizeof(Hashtable)); // allocate new hashtable
    if (!new_table) {
        // table allocation failure
        return nullptr;
    }

    new_table->items =
        calloc(init_capacity, sizeof(slot_t)); // allocate new zeroed slot array (NULL = empty)
    if (!new_table->items) {
        // slot allocation failure, free created table
        free(new_table);
        return nullptr;
    }

    new_table->capacity = init_capacity; // set initial capacity
    new_table->count = 0;                // set item count

    return new_table;
}

HashtableStatus hashtable_insert(Hashtable *table, const char *key, void *value) {
    assert(table && key && key[0] && value);

    // look up key, in case item exists
    slot_t *dest = item_probe(table->items, table->capacity, key);
    assert(dest);
    if (dest->key) {
        return HASHTABLE_ITEM_EXISTS;
    }

    // check expansion requirement
    if ((table->count + 1) * HASHTABLE_MAX_LOAD_DEN > table->capacity * HASHTABLE_MAX_LOAD_NUM) {
        if (!table_expand(table)) {
            return HASHTABLE_OUT_OF_MEM;
        }

        // recheck item in expanded array
        dest = item_probe(table->items, table->capacity, key);
        assert(dest);
        assert(!dest->key);
    }

    dest->key = strdup(key);
    if (!dest->key) {
        // strdup has malloc operation
        return HASHTABLE_OUT_OF_MEM;
    }

    dest->value = value;

    table->count++;
    return HASHTABLE_SUCCESS;
}

void *hashtable_find(const Hashtable *table, const char *key) {
    if (!table || !key) {
        // check preconditions
        return nullptr;
    }

    // probe for slot
    slot_t *slot = item_probe(table->items, table->capacity, key);

    if (!slot || !slot->key) {
        // precondition fail, key not found
        return nullptr;
    }
    return slot->value;
}

size_t hashtable_count(const Hashtable *table) {
    return table ? table->count : 0;
}
