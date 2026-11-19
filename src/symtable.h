/**
 * @file symtable.h
 * @authors Tomáš Komenda (xkoment00)
 * @brief Symbol table implementation.
 */

#pragma once

#include "ast.h"

// symbol table struct
typedef struct symtable symtable_t;

// symbol table states
typedef enum {
    SYMTABLE_SUCCESS,
    SYMTABLE_ITEM_EXISTS,
    SYMTABLE_OUT_OF_MEMORY
} symtable_status_t;

// symbol types - variables/functions
typedef enum {
    SYM_VARIABLE,
    SYM_FUNCTION
} symbol_type_t;

// return type inference state
typedef enum {
    SYM_UNRESOLVED,
    SYM_ANALYSING,
    SYM_FINALIZED
} return_type_infer_t;

// parameter description struct
typedef struct {
    DataType data_type;
    bool accepts_any;
} param_desc_t;

typedef struct {
    symbol_type_t type; // symbol type (var/func)
    char *identifier;
    int line_num; // line number (might not be needed)
    union {
        struct {
            DataType type;
            unsigned id;
            bool is_mutable;
        } variable;
        struct {
            size_t parameter_cnt;
            param_desc_t *parameters;
            bool is_main;
            bool is_builtin;
            DataType return_type;
            bool return_explicit;
            return_type_infer_t inference_state;
        } function;
    } data;
} symbol_t;
