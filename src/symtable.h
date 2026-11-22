/**
 * @file symtable.h
 * @authors Tomáš Komenda (xkoment00)
 * @brief Symbol table implementation.
 */

#pragma once

#include "ast.h"
#include <stddef.h>

// symbol table struct
typedef struct symtable symtable_t;

// symbol table states
typedef enum {
    SYMTABLE_SUCCESS,
    SYMTABLE_ITEM_EXISTS,
    SYMTABLE_OUT_OF_MEM
} symtable_status_t;

// symbol types - variables/functions
typedef enum {
    SYMBOL_VARIABLE, // symbol is a variable
    SYMBOL_FUNCTION  // symbol is a function
} symbol_type_t;

// return type inference state, moved by the semantic pass
typedef enum {
    RETURN_UNRESOLVED,
    RETURN_ANALYSING, // used in recursion detection during inference
    RETURN_FINALIZED
} return_type_infer_t;

// variable description struct
// variables always use their pushed local scope
typedef struct {
    DataType data_type; // variable data type
    unsigned id; // variable id, uniquely assigned per declaration by the table, within the table
    bool is_mutable; // is variable mutable?
} var_desc_t;

// parameter description struct
typedef struct {
    DataType data_type; // parameter data type
    bool accepts_any;   // accepts any? (data_type is ignored if true)
} param_desc_t;

// function description struct
// only the return_type and inference_state are changeable for a symbol
// functions always use the global scope
typedef struct {
    size_t parameter_cnt;                // parameter count
    const param_desc_t *parameters;      // parameter array
    bool is_main;                        // is a main function?
    bool is_builtin;                     // is builtin?
    DataType return_type;                // data type of returned value
    bool return_explicit;                // is the return explicit?
    return_type_infer_t inference_state; // return type inference state
} func_desc_t;

// symbol definition struct
// returned symbols stay valid until their scope is popped or the table is destroyed
// callers must not free symbols or their identifiers
typedef struct {
    symbol_type_t sym_type; // symbol type (var/func)
    char *identifier;       // name of symbol
    int line_number;        // line number, used for error messages
    union {
        var_desc_t variable;  // variable description
        func_desc_t function; // function description
    } data;
} symbol_t;

/**
 * @brief Creates a new empty symbol table.
 * @returns Pointer to new symbol table, or nullptr on allocation failure.
 * @note Creates a global scope.
 */
[[nodiscard]] symtable_t *symtable_create(void);

/**
 * @brief Destroys an existing symbol table.
 * @param table Pointer to existing symbol table.
 * @note Passing a nullptr does nothing.
 */
void symtable_destroy(symtable_t *table);

/**
 * @brief Populates the global scope of the symbol table with builtin standard functions.
 * @param table Pointer to symbol table (non-NULL).
 * @returns SYMTABLE_SUCCESS on successful population, SYMTABLE_ITEM_EXISTS if a duplicate item is
 * added (shouldn't happen), SYMTABLE_OUT_OF_MEM if there is an allocation failure during
 * population.
 */
[[nodiscard]] symtable_status_t symtable_register_builtins(symtable_t *table);

/**
 * @brief Pushes a new scope into symbol table.
 * @param table Pointer to symbol table (non-NULL).
 * @returns SYMTABLE_SUCCESS on successful push, SYMTABLE_OUT_OF_MEM on allocation failure.
 */
[[nodiscard]] symtable_status_t symtable_push_scope(symtable_t *table);

/**
 * @brief Pops a scope from the symbol table.
 * @param table Pointer to symbol table (non-NULL).
 * @note Popping from an empty scope stack does nothing - the global scope is never removed.
 */
void symtable_pop_scope(symtable_t *table);

/**
 * @brief Inserts a new variable into the symbol table - uses pushed local scope.
 * @param table Pointer to symbol table (non-NULL).
 * @param name Variable identifier.
 * @param description Variable description. (id is set by the symbol table on
 * insert, whatever the caller passes.)
 * @param line_num Line number.
 * @param out_symbol Optional symbol output. (can be NULL)
 * @returns SYMTABLE_SUCCESS on successful insertion, SYMTABLE_ITEM_EXISTS if variable is
 * redeclared, SYMTABLE_OUT_OF_MEM on allocation failure.
 * @note If SYMTABLE_ITEM_EXISTS is returned, the variable is already declared - error 4.
 * @attention Checks only the current scope (allows for outer scope shadowing), requires at least
 * one pushed local scope.
 */
[[nodiscard]] symtable_status_t symtable_insert_var(symtable_t *table,
                                                    const char *name,
                                                    var_desc_t description,
                                                    int line_num,
                                                    symbol_t **out_symbol);

/**
 * @brief Inserts a new function into the symbol table - uses global scope.
 * @param table Pointer to symbol table (non-NULL).
 * @param name Function name.
 * @param description Function description. (inference_state is set by the symbol table on
 * insert, whatever the caller passes, a nullptr parameter array is accepted.)
 * @param line_num Line number.
 * @returns SYMTABLE_SUCCESS on successful insertion, SYMTABLE_ITEM_EXISTS if function is
 * redeclared, SYMTABLE_OUT_OF_MEM on allocation failure.
 * @note If SYMTABLE_ITEM_EXISTS is returned, the function is already declared - error 4. The
 * parameter array is copied - the caller retains ownership.
 *
 * If the return is explicit - the inference state is RETURN_FINALIZED.
 *
 * If not, the inference state is RETURN_UNRESOLVED and return type is TYPE_UNKNOWN, whatever the
 * caller passes.
 */
[[nodiscard]] symtable_status_t
symtable_insert_func(symtable_t *table, const char *name, func_desc_t description, int line_num);

/**
 * @brief Finds a variable in the symbol table - uses pushed local scope.
 * @param table Pointer to symbol table (non-NULL).
 * @param name Variable identifier.
 * @returns nullptr if undeclared or if there is no pushed local scope, pointer to symbol otherwise.
 * @note A nullptr means the variable is not defined - error 3.
 * @attention Checks scopes from innermost to outermost.
 */
[[nodiscard]] symbol_t *symtable_find_var(const symtable_t *table, const char *name);

/**
 * @brief Finds a function in the symbol table - uses global scope.
 * @param table Pointer to symbol table (non-NULL).
 * @param name Function identifier.
 * @returns nullptr if undeclared, pointer to symbol otherwise.
 * @note A nullptr means the function is not defined - error 3.
 */
[[nodiscard]] symbol_t *symtable_find_func(const symtable_t *table, const char *name);
