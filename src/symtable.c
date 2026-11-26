/**
 * @file symtable.c
 * @authors Tomáš Komenda (xkoment00)
 * @brief Symbol table implementation.
 */

#include "symtable.h"
#include "shared/hashtable.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SYMTABLE_GLOBAL_CAP 32 // higher due to builtin functions
#define SYMTABLE_LOCAL_CAP 8
#define BUILTIN_MAX_PARAMS 3

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

// builtin function description struct
typedef struct {
    const char *name;
    size_t parameter_cnt;
    param_desc_t parameters[BUILTIN_MAX_PARAMS];
    DataType return_type;
} builtin_desc_t;

// builtin functions
static const builtin_desc_t BUILTINS[] = {
    {.name          = "print",
     .parameter_cnt = 1,
     .parameters    = {{.accepts_any = true}},
     .return_type   = TYPE_UNIT},
    {.name = "readLine", .return_type = TYPE_STRING},
    {.name = "readInt", .return_type = TYPE_INT},
    {.name = "readDouble", .return_type = TYPE_DOUBLE},
    {.name          = "toStr",
     .parameter_cnt = 1,
     .parameters    = {{.accepts_any = true}},
     .return_type   = TYPE_STRING},
    {.name          = "toInt",
     .parameter_cnt = 1,
     .parameters    = {{.data_type = TYPE_DOUBLE}},
     .return_type   = TYPE_INT},
    {.name          = "strLength",
     .parameter_cnt = 1,
     .parameters    = {{.data_type = TYPE_STRING}},
     .return_type   = TYPE_INT},
    {.name          = "subStr",
     .parameter_cnt = 3,
     .parameters  = {{.data_type = TYPE_STRING}, {.data_type = TYPE_INT}, {.data_type = TYPE_INT}},
     .return_type = TYPE_STRING},
    {.name          = "ord",
     .parameter_cnt = 2,
     .parameters    = {{.data_type = TYPE_STRING}, {.data_type = TYPE_INT}},
     .return_type   = TYPE_INT},
    {.name          = "chr",
     .parameter_cnt = 1,
     .parameters    = {{.data_type = TYPE_INT}},
     .return_type   = TYPE_STRING},
    {.name = "readBoolean", .parameter_cnt = 0, .return_type = TYPE_BOOL}};

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
        free((void *)to_free->data.function.param_list_sizes);
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
 * @param scope Scope to destroy (caller decides which scope).
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
    }
    table->current_scope = new_scope;
    return SYMTABLE_SUCCESS;
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
            *out_symbol = new_var;
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

symbol_t *symtable_find_var(const symtable_t *table, const char *name) {
    assert(table && name && name[0] != '\0' &&
           "symtable_find_var: one or more conditions are not met");
    scope_t *cursor = table->current_scope;

    while (cursor->parent_scope) {
        symbol_t *lookup = hashtable_find(cursor->namespace, name);
        if (lookup) {
            return lookup;
        }

        cursor = cursor->parent_scope;
    }

    return nullptr;
}

symbol_t *symtable_find_func(const symtable_t *table, const char *name) {
    assert(table && name && name[0] != '\0' &&
           "symtable_find_func: one or more conditions are not met");
    return hashtable_find(table->global_scope->namespace, name);
}

symtable_status_t
symtable_insert_func(symtable_t *table, const char *name, func_desc_t description, int line_num) {
    assert(table && name && name[0] != '\0' &&
           "symtable_insert_func: one or more conditions are not met");
    assert((!description.parameter_cnt || description.parameters) &&
           "symtable_insert_func: parameter count is non-zero and parameter array is nullptr");

    assert(description.param_list_cnt || !description.parameter_cnt);
    assert(!description.param_list_cnt || description.param_list_sizes);

    symbol_t *new_func = calloc(1, sizeof(symbol_t));
    if (!new_func) {
        return SYMTABLE_OUT_OF_MEM;
    }

    new_func->identifier = strdup(name);
    if (!new_func->identifier) {
        free_symbol(new_func);
        return SYMTABLE_OUT_OF_MEM;
    }

    new_func->data.function                  = description;
    new_func->data.function.parameters       = nullptr; // copy this, assign nullptr for now
    new_func->data.function.param_list_sizes = nullptr;
    new_func->line_number                    = line_num;
    new_func->sym_type                       = SYMBOL_FUNCTION;

    if (new_func->data.function.parameter_cnt != 0) {
        param_desc_t *temp = calloc(new_func->data.function.parameter_cnt, sizeof(param_desc_t));
        if (!temp) {
            free_symbol(new_func);
            return SYMTABLE_OUT_OF_MEM;
        }

        memcpy(temp,
               description.parameters,
               new_func->data.function.parameter_cnt * sizeof(param_desc_t));
        new_func->data.function.parameters = temp;
    }

    if (new_func->data.function.param_list_cnt != 0) {
        size_t *temp = calloc(new_func->data.function.param_list_cnt, sizeof(size_t));
        if (!temp) {
            free_symbol(new_func);
            return SYMTABLE_OUT_OF_MEM;
        }

        memcpy(temp,
               description.param_list_sizes,
               new_func->data.function.param_list_cnt * sizeof(size_t));
        new_func->data.function.param_list_sizes = temp;
    }

    if (new_func->data.function.return_explicit) {
        new_func->data.function.inference_state = RETURN_FINALIZED;
    } else {
        new_func->data.function.inference_state = RETURN_UNRESOLVED;
        new_func->data.function.return_type     = TYPE_UNKNOWN;
    }

    hashtable_status_t in_status = hashtable_insert(table->global_scope->namespace, name, new_func);
    switch (in_status) {
    case HASHTABLE_SUCCESS:
        return SYMTABLE_SUCCESS;

    case HASHTABLE_ITEM_EXISTS:
        free_symbol(new_func);
        return SYMTABLE_ITEM_EXISTS;

    case HASHTABLE_OUT_OF_MEM:
        free_symbol(new_func);
        return SYMTABLE_OUT_OF_MEM;
    default:
        assert(!"symtable_insert_func: unreachable switch statement reached");
        return SYMTABLE_OUT_OF_MEM;
    }
}

symtable_status_t symtable_register_builtins(symtable_t *table) {
    assert(table);

    for (size_t i = 0; i < (sizeof(BUILTINS) / sizeof(BUILTINS[0])); i++) {
        func_desc_t function_desc = {.is_builtin       = true,
                                     .is_main          = false,
                                     .parameter_cnt    = BUILTINS[i].parameter_cnt,
                                     .parameters       = BUILTINS[i].parameters,
                                     .return_explicit  = true,
                                     .return_type      = BUILTINS[i].return_type,
                                     .param_list_cnt   = 1,
                                     .param_list_sizes = &BUILTINS[i].parameter_cnt};

        symtable_status_t in_status =
            symtable_insert_func(table, BUILTINS[i].name, function_desc, 0);
        if (in_status != SYMTABLE_SUCCESS) {
            return in_status;
        }
    }
    return SYMTABLE_SUCCESS;
}
