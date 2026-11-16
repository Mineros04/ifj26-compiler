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

/**
 * @brief Standard DJB hashing function.
 * @param string String to generate hash key for.
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
