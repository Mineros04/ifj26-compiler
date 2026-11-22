/**
 * @file symtable.h
 * @authors Tomáš Komenda (xkoment00)
 * @brief Symbol table implementation.
 */

#include "symtable.h"
#include "shared/hashtable.h"
#include <assert.h>
#include <string.h>

#define SYMTABLE_GLOBAL_CAP 32 // higher due to builtin functions
#define SYMTABLE_LOCAL_CAP 8

// assert correct sizes for hashtable size minimum (2)
static_assert(SYMTABLE_GLOBAL_CAP >= 2, "hashtable_create needs a capacity of at least 2");
static_assert(SYMTABLE_LOCAL_CAP >= 2, "hashtable_create needs a capacity of at least 2");

// scope struct
typedef struct scope {
    hashtable_t *namespace;
    struct scope *parent_scope; // nullptr if global
} scope_t;

// symtable struct
// destroying a symtable frees the chain from the current scope to global scope
struct symtable {
    scope_t *global_scope;  // holds functions, cannot be nullptr or popped
    scope_t *current_scope; // must never be nullptr, if no local scopes are pushed -> global scope
    unsigned next_id;       // next id to assign to variable
};

/**
 * @brief Frees a symbol.
 * @param symbol Symbol to free.
 * @note Freeing a function with no parameters is a no-op, since free(nullptr) does nothing.
 */
static void free_symbol(void *symbol) {
    if (!symbol) {
        return;
    }
    symbol_t *to_free = symbol;

    if (to_free->sym_type == SYMBOL_FUNCTION) {
        // legitimate cast here, const protects the callers, array was created by own malloc
        free((void *)to_free->data.function.parameters);
    }

    free(to_free->identifier);
    free(to_free);
}

/**
 * @brief Creates a new scope.
 * @param parent_scope Parent scope of the scope to be created.
 * @returns New scope object or nullptr on allocation failure.
 * @note Passing nullptr as parent scope creates a global scope.
 */
static scope_t *create_scope(scope_t *parent_scope) {
    scope_t *new_scope = malloc(sizeof(scope_t));
    if (!new_scope) {
        return nullptr;
    }

    if (!parent_scope) {
        new_scope->namespace = hashtable_create(SYMTABLE_GLOBAL_CAP);
    } else {
        new_scope->namespace = hashtable_create(SYMTABLE_LOCAL_CAP);
    }

    if (!new_scope->namespace) {
        free(new_scope);
        return nullptr;
    }
    new_scope->parent_scope = parent_scope;

    return new_scope;
}

/**
 * @brief Destroys an existing scope.
 * @param scope Scope to destroy (must not be global scope, else nothing happens).
 */
static void destroy_scope(scope_t *scope) {
    hashtable_destroy(scope->namespace, free_symbol);
    free(scope);
}

symtable_t *symtable_create(void) {
    symtable_t *new_table = malloc(sizeof(symtable_t));
    if (!new_table) {
        return nullptr;
    }

    new_table->global_scope = create_scope(nullptr);
    if (!new_table->global_scope) {
        free(new_table);
        return nullptr;
    }

    new_table->current_scope = new_table->global_scope;
    new_table->next_id       = 0;

    return new_table;
}

void symtable_destroy(symtable_t *table) {
    if (!table) {
        return;
    }

    while (table->current_scope->parent_scope) {
        scope_t *parent = table->current_scope->parent_scope;
        destroy_scope(table->current_scope);

        table->current_scope = parent;
    }
    destroy_scope(table->global_scope);
    free(table);
}

symtable_status_t symtable_push_scope(symtable_t *table) {
    assert(table && "symtable_push_scope: table is nullptr");
    scope_t *new_scope = create_scope(table->current_scope);
    if (!new_scope) {
        return SYMTABLE_OUT_OF_MEM;
    } else {
        table->current_scope = new_scope;
        return SYMTABLE_SUCCESS;
    }
}

void symtable_pop_scope(symtable_t *table) {
    assert(table && "symtable_pop_scope: table is nullptr");
    if (!table->current_scope->parent_scope) {
        return;
    }

    scope_t *parent = table->current_scope->parent_scope;
    destroy_scope(table->current_scope);
    table->current_scope = parent;
}

symtable_status_t symtable_insert_var(symtable_t *table,
                                      const char *name,
                                      var_desc_t description,
                                      int line_num,
                                      symbol_t **out_symbol) {
    assert(table && name && name[0] != '\0' && table->current_scope->parent_scope &&
           "symtable_insert_var: one or more conditions are not met");
    symbol_t *new_var = calloc(1, sizeof(symbol_t));
    if (!new_var) {
        return SYMTABLE_OUT_OF_MEM;
    }

    new_var->identifier = strdup(name);
    if (!new_var->identifier) {
        free_symbol(new_var);
        return SYMTABLE_OUT_OF_MEM;
    }

    new_var->data.variable    = description;
    new_var->data.variable.id = table->next_id;
    new_var->line_number      = line_num;
    new_var->sym_type         = SYMBOL_VARIABLE;

    hashtable_status_t in_status = hashtable_insert(table->current_scope->namespace, name, new_var);
    switch (in_status) {
    case HASHTABLE_SUCCESS:
        table->next_id++;
        if (out_symbol) {
            out_symbol = &new_var;
        }
        return SYMTABLE_SUCCESS;

    case HASHTABLE_ITEM_EXISTS:
        free_symbol(new_var);
        return SYMTABLE_ITEM_EXISTS;

    case HASHTABLE_OUT_OF_MEM:
        free_symbol(new_var);
        return SYMTABLE_OUT_OF_MEM;
    default:
        assert(!"symtable_insert_var: unreachable switch statement reached");
        return SYMTABLE_OUT_OF_MEM;
    }
}
