/**
 * @file hashtable.c
 * @authors Tomáš Komenda (xkoment00)
 * @brief Hash table implementation, according to TRP-izp specification (open addressing)
 */

#include "hashtable.h"

typedef struct slot {
    char *key; // if key is NULL -> slot is empty
    void *value;
} slot_t;

struct hashtable {
    slot_t *items;   // item array
    size_t capacity; // hash table capacity
    size_t count;    // item count
};
