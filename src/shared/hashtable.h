/**
 * @file hashtable.h
 * @authors Tomáš Komenda (xkoment00)
 * @brief Hash table implementation, according to TRP-izp specification (open addressing)
 */

#pragma once

#include <stddef.h> // size_t

// Attention: An empty string key is not a valid key.

typedef struct hashtable Hashtable;

// item insertion states
typedef enum {
    HASHTABLE_SUCCESS,     // item insertion successful
    HASHTABLE_ITEM_EXISTS, // item already exists
    HASHTABLE_OUT_OF_MEM   // ran out of memory during insertion
} HashtableStatus;

// generic callback for hashtable destruction, receives a stored value
typedef void (*HashtableCallback)(void *value);

/**
 * @brief Creates a new empty hashtable.
 * @param init_capacity Initial number of slots in hashtable.
 * @returns Pointer to new hashtable or NULL on allocation failure.
 * @note If init_capacity is 0 or 1, the function returns NULL. The
 * accepted minimum is init_capacity of 2. The table is freed by the
 * caller with hashtable_destroy.
 */
Hashtable *hashtable_create(size_t init_capacity);

/**
 * @brief Inserts a new item into the hashtable.
 * @param table Hashtable to add item into.
 * @param key Item key (non-NULL, key is copied by the table).
 * @param value Item value (non-NULL).
 * @returns On success, returns HASHTABLE_SUCCESS.
 * If the item already exists, returns HASHTABLE_ITEM_EXISTS - existing item is untouched.
 * On memory allocation failure, returns HASHTABLE_OUT_OF_MEM - table is unchanged.
 * @attention Insertion may result in an out-of-memory error.
 * @note Unless the return is HASHTABLE_SUCCESS, the caller
 * still owns the value, and must free it.
 */
HashtableStatus hashtable_insert(Hashtable *table, const char *key, void *value);

/**
 * @brief Finds an item in the hashtable.
 * @param table Hashtable to find the item in (non-NULL).
 * @param key Key to find the item by (non-NULL).
 * @returns The stored value or NULL if no item with this key exists.
 * @note The returned pointer is the stored value, which stays
 * valid after table growth. There is no modification of the
 * table in this function.
 */
void *hashtable_find(const Hashtable *table, const char *key);

/**
 * @brief Frees the hashtable and all owned keys, callback frees the values.
 * @param table Hashtable to destroy, pointer is invalidated afterwards.
 * @param callback Function called on every stored value, NULL leaves values untouched.
 * @note The callback function is called once per value.
 * Nothing happens if table is NULL. Frees all key copies,
 * slots & hashtable struct. No values are freed unless a callback
 * is given. The callback must not access the hashtable itself. The
 * order of callback calls is unspecified.
 */
void hashtable_destroy(Hashtable *table, HashtableCallback callback);

/**
 * @brief Gets item count in hashtable.
 * @param table Hashtable to get count from.
 * @returns Item count of hashtable, or 0 on NULL table.
 * @note Counted are stored items (keys).
 */
size_t hashtable_count(const Hashtable *table);
