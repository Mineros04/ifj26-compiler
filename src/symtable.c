/**
 * @file symtable.h
 * @authors Tomáš Komenda (xkoment00)
 * @brief Symbol table implementation.
 */

#include "symtable.h"
#include "shared/hashtable.h"

#define HASHTABLE_MIN_CAPACITY 2

// scope struct
typedef struct {
    hashtable_t *namespace;
    struct scope_t *parent_scope; // nullptr if global
} scope_t;

// symtable struct
// destroying a symtable frees the chain from the current scope to global scope
struct symtable {
    scope_t global_scope;  // holds functions, cannot be nullptr or popped
    scope_t current_scope; // must never be nullptr, if no local scopes are pushed -> global scope
    unsigned next_id;      // next id to assign to variable
};
