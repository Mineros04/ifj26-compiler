/**
 * @file hashtable.c
 * @authors Tomáš Komenda (xkoment00)
 * @brief Hash table implementation, according to TRP-izp specification (open addressing)
 */

#include "hashtable.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h> // malloc, calloc
#include <string.h>

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
 * @brief Standard DJB hashing function.
 * @param str String to generate hash key for.
 * @returns Unreduced hash for string.
 * @note Sources: IAL presentation "Vyhledávací tabulky III",
 * slide 13, D. J. Bernstein.
 */
static unsigned long djb_hash(const char *str) {
    // convert to unsigned string
    const unsigned char *string = (unsigned char *)str;

    // generate hash
    unsigned long hash = 5381;
    unsigned c;
    while ((c = *string++) != 0) {
        hash = (hash * 33) + c;
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
    size_t idx = djb_hash(key) % capacity;

    for (size_t i = 0; i < capacity; i++) {
        if (!item_array[idx].key || strcmp(key, item_array[idx].key) == 0) {
            return &item_array[idx];
        }
        idx = (idx + 1) % capacity; // advance
    }

    assert(!"hashtable: table has no empty slot in item_probe function");
    return NULL;
}

Hashtable *hashtable_create(size_t init_capacity) {
    if (init_capacity < 2) {
        // capacity less than 2, not acceptable for hashtable
        return NULL;
    }

    Hashtable *new_table = malloc(sizeof(Hashtable)); // allocate new hashtable
    if (!new_table) {
        // table allocation failure
        return NULL;
    }

    new_table->items =
        calloc(sizeof(slot_t), init_capacity); // allocate new zeroed slot array (NULL = empty)
    if (!new_table->items) {
        // slot allocation failure, free created table
        free(new_table);
        return NULL;
    }

    new_table->capacity = init_capacity; // set initial capacity
    new_table->count = 0;                // set item count

    return new_table;
}

void *hashtable_find(const Hashtable *table, const char *key) {
    if (!table || !key) {
        // check preconditions
        return NULL;
    }

    // probe for slot
    slot_t *slot = item_probe(table->items, table->capacity, key);

    if (!slot || !slot->key) {
        // precondition fail, key not found
        return NULL;
    } else {
        // return value
        return slot->value;
    }
}

size_t hashtable_count(const Hashtable *table) {
    return table ? table->count : 0;
}
