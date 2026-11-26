/**
 * @file expr.h
 * @authors Tomáš Komenda (xkoment00)
 * @brief Expression parser interface.
 */

#pragma once

#include "ast.h"    // AstNode
#include "parser.h" // lexer_t, token_type_t
#include "shared/error.h"
#include <assert.h>

#define EXPR_INIT_STACK_CAP 16

// terminal types
typedef enum {
    EXPR_TERMINAL_PRIORITY_1,
    EXPR_TERMINAL_PRIORITY_2,
    EXPR_TERMINAL_PRIORITY_3,
    EXPR_TERMINAL_PRIORITY_4,
    EXPR_TERMINAL_PRIORITY_5,
    EXPR_TERMINAL_OPERAND,
    EXPR_TERMINAL_END_EXPRESSION,
    EXPR_TERMINAL_OPEN_PAREN,
    EXPR_TERMINAL_CLOSE_PAREN,
    EXPR_TERMINAL_COUNT // total count of terminals
} expr_terminal_t;

// terminal relations
typedef enum {
    EXPR_RELATION_ERROR,
    EXPR_RELATION_ACCEPT,
    EXPR_RELATION_SHIFT,
    EXPR_RELATION_REDUCE,
    EXPR_RELATION_EQUAL
} expr_relation_t;

// stack entry type
typedef enum {
    EXPR_ENTRY_TERMINAL,
    EXPR_ENTRY_EXPRESSION
} expr_entry_type_t;

// stack entry struct
typedef struct {
    expr_entry_type_t entry_type;
    union {
        struct {
            token_type_t token_type;
            expr_terminal_t terminal_type;
            int line_number;
            AstNode *leaf_node; // operand carries its leaf node here (nullptr for others)
        } shifted_terminal;

        struct {
            AstNode *node;
        } expression;
    };
} expr_stack_entry_t;

// stack
typedef struct {
    expr_stack_entry_t *entries;
    size_t count;
    size_t capacity;
} expr_stack_t;

// precedence table for terminals & their relations
// row: terminal on stack; column: incoming terminal
static const expr_relation_t RELATION_TABLE[EXPR_TERMINAL_COUNT][EXPR_TERMINAL_COUNT] = {
    [EXPR_TERMINAL_PRIORITY_1]     = {[EXPR_TERMINAL_PRIORITY_1]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_2]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_PRIORITY_3]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_PRIORITY_4]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_PRIORITY_5]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_OPERAND]        = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_OPEN_PAREN]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_CLOSE_PAREN]    = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_END_EXPRESSION] = EXPR_RELATION_REDUCE},
    [EXPR_TERMINAL_PRIORITY_2]     = {[EXPR_TERMINAL_PRIORITY_1]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_2]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_PRIORITY_3]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_PRIORITY_4]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_PRIORITY_5]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_OPERAND]        = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_OPEN_PAREN]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_CLOSE_PAREN]    = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_END_EXPRESSION] = EXPR_RELATION_REDUCE},
    [EXPR_TERMINAL_PRIORITY_3]     = {[EXPR_TERMINAL_PRIORITY_1]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_2]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_3]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_PRIORITY_4]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_PRIORITY_5]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_OPERAND]        = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_OPEN_PAREN]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_CLOSE_PAREN]    = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_END_EXPRESSION] = EXPR_RELATION_REDUCE},
    [EXPR_TERMINAL_PRIORITY_4]     = {[EXPR_TERMINAL_PRIORITY_1]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_2]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_3]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_4]     = EXPR_RELATION_ERROR,
                                      [EXPR_TERMINAL_PRIORITY_5]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_OPERAND]        = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_OPEN_PAREN]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_CLOSE_PAREN]    = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_END_EXPRESSION] = EXPR_RELATION_REDUCE},
    [EXPR_TERMINAL_PRIORITY_5]     = {[EXPR_TERMINAL_PRIORITY_1]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_2]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_3]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_4]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_5]     = EXPR_RELATION_ERROR,
                                      [EXPR_TERMINAL_OPERAND]        = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_OPEN_PAREN]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_CLOSE_PAREN]    = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_END_EXPRESSION] = EXPR_RELATION_REDUCE},
    [EXPR_TERMINAL_OPERAND]        = {[EXPR_TERMINAL_PRIORITY_1]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_PRIORITY_2]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_PRIORITY_3]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_PRIORITY_4]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_PRIORITY_5]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_OPERAND]        = EXPR_RELATION_ERROR,
                                      [EXPR_TERMINAL_OPEN_PAREN]     = EXPR_RELATION_ERROR,
                                      [EXPR_TERMINAL_CLOSE_PAREN]    = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_END_EXPRESSION] = EXPR_RELATION_REDUCE},
    [EXPR_TERMINAL_OPEN_PAREN]     = {[EXPR_TERMINAL_PRIORITY_1]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_2]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_3]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_4]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_5]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_OPERAND]        = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_OPEN_PAREN]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_CLOSE_PAREN]    = EXPR_RELATION_EQUAL,
                                      [EXPR_TERMINAL_END_EXPRESSION] = EXPR_RELATION_ERROR},
    [EXPR_TERMINAL_CLOSE_PAREN]    = {[EXPR_TERMINAL_PRIORITY_1]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_PRIORITY_2]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_PRIORITY_3]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_PRIORITY_4]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_PRIORITY_5]     = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_OPERAND]        = EXPR_RELATION_ERROR,
                                      [EXPR_TERMINAL_OPEN_PAREN]     = EXPR_RELATION_ERROR,
                                      [EXPR_TERMINAL_CLOSE_PAREN]    = EXPR_RELATION_REDUCE,
                                      [EXPR_TERMINAL_END_EXPRESSION] = EXPR_RELATION_REDUCE},
    [EXPR_TERMINAL_END_EXPRESSION] = {[EXPR_TERMINAL_PRIORITY_1]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_2]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_3]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_4]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_PRIORITY_5]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_OPERAND]        = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_OPEN_PAREN]     = EXPR_RELATION_SHIFT,
                                      [EXPR_TERMINAL_CLOSE_PAREN]    = EXPR_RELATION_ERROR,
                                      [EXPR_TERMINAL_END_EXPRESSION] = EXPR_RELATION_ACCEPT}};

/**
 * @brief Classifes token by behavior & priority.
 * @param token Token to classify.
 * @returns Terminal type.
 */
static expr_terminal_t expr_classify_token(token_type_t token) {
    switch (token) {
    case TOK_INDENT:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_DEDENT:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_NEWLINE:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_EOF:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_IDENT:
        return EXPR_TERMINAL_OPERAND;
    case TOK_LIT_INT:
        return EXPR_TERMINAL_OPERAND;
    case TOK_LIT_DOUBLE:
        return EXPR_TERMINAL_OPERAND;
    case TOK_LIT_STRING:
        return EXPR_TERMINAL_OPERAND;
    case TOK_KW_IMPORT:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_KW_DEF:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_KW_IF:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_KW_THEN:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_KW_ELSE:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_KW_WHILE:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_KW_DO:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_KW_RETURN:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_KW_VAL:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_KW_VAR:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_KW_INT:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_KW_DOUBLE:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_KW_STRING:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_KW_UNIT:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_OP_PLUS:
        return EXPR_TERMINAL_PRIORITY_3;
    case TOK_OP_MINUS:
        return EXPR_TERMINAL_PRIORITY_3;
    case TOK_OP_STAR:
        return EXPR_TERMINAL_PRIORITY_2;
    case TOK_OP_SLASH:
        return EXPR_TERMINAL_PRIORITY_2;
    case TOK_OP_EQ:
        return EXPR_TERMINAL_PRIORITY_5;
    case TOK_OP_NEQ:
        return EXPR_TERMINAL_PRIORITY_5;
    case TOK_OP_LT:
        return EXPR_TERMINAL_PRIORITY_4;
    case TOK_OP_LE:
        return EXPR_TERMINAL_PRIORITY_4;
    case TOK_OP_GT:
        return EXPR_TERMINAL_PRIORITY_4;
    case TOK_OP_GE:
        return EXPR_TERMINAL_PRIORITY_4;
    case TOK_OP_ASSIGN:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_SEP_LPAREN:
        return EXPR_TERMINAL_OPEN_PAREN;
    case TOK_SEP_RPAREN:
        return EXPR_TERMINAL_CLOSE_PAREN;
    case TOK_SEP_COMMA:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_SEP_COLON:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_SEP_SEMI:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_SEP_DOT:
        return EXPR_TERMINAL_END_EXPRESSION;
    case TOK_AT_MAIN:
        return EXPR_TERMINAL_END_EXPRESSION;
    }

    assert(!"expr_classify_token: unreachable statement reached");
    unreachable();
}

/**
 * @brief Resolves the terminal (useful for unary/binary operations).
 * @param terminal Terminal to resolve.
 * @param prev_terminal Previous terminal.
 * @returns Resolved terminal.
 */
static expr_terminal_t expr_resolve_terminal(expr_terminal_t terminal,
                                             expr_terminal_t prev_terminal) {
    if (terminal == EXPR_TERMINAL_PRIORITY_3 &&
        (prev_terminal != EXPR_TERMINAL_OPERAND && prev_terminal != EXPR_TERMINAL_CLOSE_PAREN)) {
        return EXPR_TERMINAL_PRIORITY_1;
    }

    return terminal;
}

/**
 * @brief Looks up the relation between two terminals (stack top & incoming terminal)
 * @param terminal Terminal on stack top (the row in the table).
 * @param next_terminal Incoming terminal (the column in the table).
 * @returns Relation for the two terminals.
 */
static expr_relation_t expr_lookup_relation(expr_terminal_t terminal,
                                            expr_terminal_t next_terminal) {
    assert(terminal < EXPR_TERMINAL_COUNT && next_terminal < EXPR_TERMINAL_COUNT &&
           "expr_lookup_relation: terminal is out of range");
    return RELATION_TABLE[terminal][next_terminal];
}

/**
 * @brief Creates a new terminal stack entry.
 * @param token_type Token type.
 * @param terminal Terminal type.
 * @param line_number Line number.
 * @returns New stack entry.
 */
[[nodiscard]] static expr_stack_entry_t
expr_create_terminal(token_type_t token_type, expr_terminal_t terminal, int line_number) {
    expr_stack_entry_t new_entry = {.entry_type       = EXPR_ENTRY_TERMINAL,
                                    .shifted_terminal = {.line_number   = line_number,
                                                         .terminal_type = terminal,
                                                         .token_type    = token_type}};
    return new_entry;
}

/**
 * @brief Creates a new expression stack entry.
 * @param node AST node of expression.
 * @returns New stack entry.
 */
[[nodiscard]] static expr_stack_entry_t expr_create_expression(AstNode *node) {
    expr_stack_entry_t new_entry = {.entry_type = EXPR_ENTRY_EXPRESSION,
                                    .expression = {.node = node}};
    return new_entry;
}

/**
 * @brief Creates a new operand stack entry.
 * @param token_type Type of token.
 * @param leaf_node Leaf node of operand (non-NULL).
 * @param line_number Line number.
 * @returns New stack entry.
 */
[[nodiscard]] static expr_stack_entry_t
expr_create_operand(token_type_t token_type, AstNode *leaf_node, int line_number) {
    assert(leaf_node && "expr_create_operand: leaf node cannot be nullptr");
    expr_stack_entry_t new_entry = {.entry_type                     = EXPR_ENTRY_TERMINAL,
                                    .shifted_terminal.terminal_type = EXPR_TERMINAL_OPERAND,
                                    .shifted_terminal.leaf_node     = leaf_node,
                                    .shifted_terminal.line_number   = line_number,
                                    .shifted_terminal.token_type    = token_type};
    return new_entry;
}

/**
 * @brief Initializes a stack.
 * @param stack Stack to initialize (owned by the caller).
 * @note Must be called before other functions, allocates nothing.
 */
static void expr_stack_init(expr_stack_t *stack) {
    stack->entries  = nullptr;
    stack->count    = 0;
    stack->capacity = 0;
}

/**
 * @brief Frees a stack.
 * @param stack Stack to free.
 * @note Frees the stack array, but not the `AstNode`s - on a successful parse, there are none
 * left.
 */
static void expr_stack_free(expr_stack_t *stack) {
    free(stack->entries);
    stack->entries  = nullptr;
    stack->count    = 0;
    stack->capacity = 0;
}

/**
 * @brief Pushes an entry onto the stack.
 * @param stack Stack to push to.
 * @param entry Entry to push.
 * @note May grow the stack - could cause an allocation failure.
 *
 * Pointers from `expr_stack_peek` are invalidated after a push.
 */
static void expr_stack_push(expr_stack_t *stack, expr_stack_entry_t entry) {
    if (stack->count == stack->capacity) {
        if (stack->capacity == 0) {
            stack->capacity = EXPR_INIT_STACK_CAP;
        } else {
            stack->capacity *= 2;
        }

        expr_stack_entry_t *temp =
            realloc(stack->entries, stack->capacity * sizeof(expr_stack_entry_t));
        if (!temp) {
            error_throw_internal("Compiler internal error: memory allocation failed.\n");
        } else {
            stack->entries = temp;
        }
    }

    stack->entries[stack->count] = entry;
    stack->count++;
}

/**
 * @brief Pops an entry from the stack.
 * @param stack Stack to pop from (non-NULL).
 * @returns The popped stack entry (belongs to the caller afterwards).
 * @note The stack's count must be higher than 0.
 */
static expr_stack_entry_t expr_stack_pop(expr_stack_t *stack) {
    assert(stack && stack->count > 0 && "expr_stack_pop: invalid stack setup");

    stack->count--;
    return stack->entries[stack->count];
}

/**
 * @brief Shows the entry at `depth` in `stack`.
 * @param stack Stack to get entry from.
 * @param depth Depth of entry.
 * @returns Pointer to entry.
 * @note `depth` must be lower than `count`, pointer is valid until next pop/push.
 */
static expr_stack_entry_t *expr_stack_peek(const expr_stack_t *stack, size_t depth) {
    assert(depth < stack->count && "expr_stack_peek: condition not met");

    return &stack->entries[stack->count - 1 - depth];
}

/**
 * @brief Get the terminal at the top of the stack.
 * @param stack Stack to get terminal from.
 * @returns Terminal at the stack top.
 * @note Requires the bottom marker, no two finished expressions sit next to each other.
 */
[[nodiscard]] static expr_terminal_t expr_stack_top_terminal(const expr_stack_t *stack) {
    const expr_stack_entry_t *top = expr_stack_peek(stack, 0);

    if (top->entry_type == EXPR_ENTRY_TERMINAL) {
        return top->shifted_terminal.terminal_type;
    }

    const expr_stack_entry_t *below = expr_stack_peek(stack, 1);
    assert(below->entry_type == EXPR_ENTRY_TERMINAL);
    return below->shifted_terminal.terminal_type;
}

/**
 * @brief Creates a new leaf node.
 * @param token Literal token (allowed are identifiers, int/double/string literals).
 * @returns Pointer to new leaf node (cannot be nullptr).
 * @note Passing other token types is a bug.
 */
[[nodiscard]] static AstNode *expr_create_leaf(const token_t *token) {
    switch (token->type) {
    case TOK_IDENT:
        return ast_new_ident(str_cstr(&token->str_val), (int)token->line);
    case TOK_LIT_INT:
        return ast_new_lit_int(token->int_val, (int)token->line);
    case TOK_LIT_DOUBLE:
        return ast_new_lit_double(token->double_val, (int)token->line);
    case TOK_LIT_STRING:
        return ast_new_lit_string(str_cstr(&token->str_val), (int)token->line);
    default:
        assert(!"expr_create_leaf: other token type passed");
        unreachable();
    }
}
