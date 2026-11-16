/**
 * @file hashtable.c
 * @authors Tomáš Komenda (xkoment00)
 * @brief Hash table implementation, according to TRP-izp specification (open addressing)
 */

#include "hashtable.h"
#include <stdlib.h> // malloc, calloc

typedef struct slot {
    char *key; // if key is NULL -> slot is empty
    void *value;
} slot_t;

struct hashtable {
    slot_t *items;   // item array
    size_t capacity; // hash table capacity
    size_t count;    // item count
};

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
