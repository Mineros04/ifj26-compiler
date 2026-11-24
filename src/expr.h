/**
 * @file expr.h
 * @authors Tomáš Komenda (xkoment00)
 * @brief Expression parser interface.
 */

#pragma once

#include "ast.h"    // AstNode
#include "parser.h" // lexer_t, token_type_t
// #include "symtable.h" // symtable_t
#include <assert.h>

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
