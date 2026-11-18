/**
 * @file ast.h
 * @authors Ondřej Pippal
 * @brief Abstract Syntax Tree (AST) node definitions and interfaces.
 *
 * Project: IFJ26 Compiler Implementation
 */

#ifndef AST_H
#define AST_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// Forward declaration of the main AST node
typedef struct AstNode AstNode;

/**
 * @brief Data types supported by the IFJ26 language.
 */
typedef enum {
    TYPE_UNKNOWN = 0, // Type not specified (to be inferred during semantic analysis)
    TYPE_INT,         // Int (64-bit in target code, 32-bit semantics)
    TYPE_DOUBLE,      // Double (IEEE 754)
    TYPE_STRING,      // String
    TYPE_UNIT,        // Unit ()
    TYPE_BOOL         // Boolean (for relational expressions and BOOLTHEN extension)
} DataType;

/**
 * @brief Operators for expression nodes.
 */
typedef enum {
    OP_ADD,    // + (addition or concatenation)
    OP_SUB,    // - (subtraction)
    OP_MUL,    // *
    OP_DIV,    // / (integer and floating-point division)
    OP_EQ,     // ==
    OP_NEQ,    // !=
    OP_LT,     // <
    OP_LE,     // <=
    OP_GT,     // >
    OP_GE,     // >=
    OP_AND,    // && (BOOLTHEN)
    OP_OR,     // || (BOOLTHEN)
    OP_NEG,    // unary -
    OP_POS,    // unary +
    OP_NOT     // unary ! (BOOLTHEN)
} OperatorType;

/**
 * @brief AST node types.
 */
typedef enum {
    AST_PROGRAM,      // Root of the program (list of functions)
    AST_FUNC_DEF,     // Function definition (def id(...) = ...)
    AST_PARAM,        // Formal function parameter (id: Type)
    AST_BLOCK,        // Block of statements (creates a new scope)

    // Statements
    AST_VAR_DECL,     // Variable declaration (val / var)
    AST_ASSIGN,       // Assignment (id = expr)
    AST_IF,           // If-then-else statement (or expression in CONTROL)
    AST_WHILE,        // While loop
    AST_FOR,          // For loop (CONTROL extension)
    AST_RETURN,       // Return statement

    // Expressions & Calls
    AST_FUNC_CALL,    // Function call id(args...)
    AST_BIN_OP,       // Binary operation (a + b, x < y, ...)
    AST_UN_OP,        // Unary operation (-x, !b)
    AST_IDENT,        // Variable identifier in an expression
    AST_LIT_INT,      // Integer literal
    AST_LIT_DOUBLE,   // Double literal
    AST_LIT_STRING,   // String literal
    AST_LIT_BOOL,     // Boolean literal (true / false)
    AST_LIT_UNIT      // Unit literal ()
} AstNodeType;

/**
 * @brief Dynamic array of AST node pointers (for lists of statements, parameters, arguments).
 */
typedef struct {
    AstNode **items;
    size_t count;
    size_t capacity;
} AstNodeList;

/**
 * @brief Main AST node structure.
 */
struct AstNode {
    AstNodeType type;       // Type of the node
    int line;               // Source code line number (for error reporting)
    DataType inferred_type; // Type inferred during semantic analysis
    bool convert_to_double; // Flag for code generation: implicit conversion of Int to Double

    union {
        // AST_PROGRAM
        struct {
            AstNodeList functions;
        } program;

        // AST_FUNC_DEF
        struct {
            char *name;
            bool is_main;
            AstNodeList params;
            DataType return_type;
            AstNode *body;
        } func_def;

        // AST_PARAM
        struct {
            char *name;
            DataType data_type;
        } param;

        // AST_BLOCK
        struct {
            AstNodeList statements;
        } block;

        // AST_VAR_DECL (val / var)
        struct {
            char *name;
            bool is_var;
            DataType var_type;
            AstNode *init_expr;
        } var_decl;

        // AST_ASSIGN (id = ...)
        struct {
            char *target_name;
            AstNode *value_expr;
        } assign;

        // AST_IF
        struct {
            AstNode *condition;
            AstNode *then_branch;
            AstNode *else_branch;
        } if_stmt;

        // AST_WHILE
        struct {
            AstNode *condition;
            AstNode *body;
        } while_stmt;

        // AST_FOR
        struct {
            char *iter_name;
            AstNode *from_expr;
            AstNode *to_expr;
            AstNode *step_expr;
            bool is_until;
            AstNodeList guards;
            AstNode *body;
        } for_stmt;

        // AST_RETURN
        struct {
            AstNode *expr;
        } return_stmt;

        // AST_FUNC_CALL
        struct {
            char *func_name;
            AstNodeList args;
        } func_call;

        // AST_BIN_OP
        struct {
            OperatorType op;
            AstNode *left;
            AstNode *right;
        } bin_op;

        // AST_UN_OP
        struct {
            OperatorType op;
            AstNode *operand;
        } un_op;

        // AST_IDENT
        struct {
            char *name;
        } ident;

        // Literals
        int64_t lit_int;
        double lit_double;
        char *lit_string;
        bool lit_bool;
    };
};

// Node list operations

void ast_list_init(AstNodeList *list);
void ast_list_append(AstNodeList *list, AstNode *node);

// AST Node Constructors

AstNode *ast_new_program(void);
AstNode *ast_new_func_def(const char *name, bool is_main, DataType return_type, AstNode *body, int line);
AstNode *ast_new_param(const char *name, DataType data_type, int line);
AstNode *ast_new_block(int line);
AstNode *ast_new_var_decl(const char *name, bool is_var, DataType var_type, AstNode *init_expr, int line);
AstNode *ast_new_assign(const char *target_name, AstNode *value_expr, int line);
AstNode *ast_new_if(AstNode *condition, AstNode *then_branch, AstNode *else_branch, int line);
AstNode *ast_new_while(AstNode *condition, AstNode *body, int line);
AstNode *ast_new_for(const char *iter_name, AstNode *from_expr, AstNode *to_expr, AstNode *step_expr, bool is_until, AstNode *body, int line);
AstNode *ast_new_return(AstNode *expr, int line);
AstNode *ast_new_func_call(const char *func_name, int line);
AstNode *ast_new_bin_op(OperatorType op, AstNode *left, AstNode *right, int line);
AstNode *ast_new_un_op(OperatorType op, AstNode *operand, int line);
AstNode *ast_new_ident(const char *name, int line);
AstNode *ast_new_lit_int(int64_t value, int line);
AstNode *ast_new_lit_double(double value, int line);
AstNode *ast_new_lit_string(const char *value, int line);
AstNode *ast_new_lit_bool(bool value, int line);
AstNode *ast_new_lit_unit(int line);

// Memory Management

void ast_free(AstNode *node);

#endif // AST_H
