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

typedef enum {
    TERMINAL_PRIORITY_1,
    TERMINAL_PRIORITY_2,
    TERMINAL_PRIORITY_3,
    TERMINAL_PRIORITY_4,
    TERMINAL_PRIORITY_5,
    TERMINAL_OPERAND,
    TERMINAL_END_EXPRESSION,
    TERMINAL_OPEN_PAREN,
    TERMINAL_CLOSE_PAREN,
    TERMINAL_COUNT // total count of terminals
} expr_terminal_t;

/**
 * @brief Classifes token by behavior & priority.
 * @param token Token to classify.
 * @returns Terminal type.
 */
static expr_terminal_t expr_classify_token(token_type_t token) {
    switch (token) {
    case TOK_INDENT:
        return TERMINAL_END_EXPRESSION;
    case TOK_DEDENT:
        return TERMINAL_END_EXPRESSION;
    case TOK_NEWLINE:
        return TERMINAL_END_EXPRESSION;
    case TOK_EOF:
        return TERMINAL_END_EXPRESSION;
    case TOK_IDENT:
        return TERMINAL_OPERAND;
    case TOK_LIT_INT:
        return TERMINAL_OPERAND;
    case TOK_LIT_DOUBLE:
        return TERMINAL_OPERAND;
    case TOK_LIT_STRING:
        return TERMINAL_OPERAND;
    case TOK_KW_IMPORT:
        return TERMINAL_END_EXPRESSION;
    case TOK_KW_DEF:
        return TERMINAL_END_EXPRESSION;
    case TOK_KW_IF:
        return TERMINAL_END_EXPRESSION;
    case TOK_KW_THEN:
        return TERMINAL_END_EXPRESSION;
    case TOK_KW_ELSE:
        return TERMINAL_END_EXPRESSION;
    case TOK_KW_WHILE:
        return TERMINAL_END_EXPRESSION;
    case TOK_KW_DO:
        return TERMINAL_END_EXPRESSION;
    case TOK_KW_RETURN:
        return TERMINAL_END_EXPRESSION;
    case TOK_KW_VAL:
        return TERMINAL_END_EXPRESSION;
    case TOK_KW_VAR:
        return TERMINAL_END_EXPRESSION;
    case TOK_KW_INT:
        return TERMINAL_END_EXPRESSION;
    case TOK_KW_DOUBLE:
        return TERMINAL_END_EXPRESSION;
    case TOK_KW_STRING:
        return TERMINAL_END_EXPRESSION;
    case TOK_KW_UNIT:
        return TERMINAL_END_EXPRESSION;
    case TOK_OP_PLUS:
        return TERMINAL_PRIORITY_3;
    case TOK_OP_MINUS:
        return TERMINAL_PRIORITY_3;
    case TOK_OP_STAR:
        return TERMINAL_PRIORITY_2;
    case TOK_OP_SLASH:
        return TERMINAL_PRIORITY_2;
    case TOK_OP_EQ:
        return TERMINAL_PRIORITY_5;
    case TOK_OP_NEQ:
        return TERMINAL_PRIORITY_5;
    case TOK_OP_LT:
        return TERMINAL_PRIORITY_4;
    case TOK_OP_LE:
        return TERMINAL_PRIORITY_4;
    case TOK_OP_GT:
        return TERMINAL_PRIORITY_4;
    case TOK_OP_GE:
        return TERMINAL_PRIORITY_4;
    case TOK_OP_ASSIGN:
        return TERMINAL_END_EXPRESSION;
    case TOK_SEP_LPAREN:
        return TERMINAL_OPEN_PAREN;
    case TOK_SEP_RPAREN:
        return TERMINAL_CLOSE_PAREN;
    case TOK_SEP_COMMA:
        return TERMINAL_END_EXPRESSION;
    case TOK_SEP_COLON:
        return TERMINAL_END_EXPRESSION;
    case TOK_SEP_SEMI:
        return TERMINAL_END_EXPRESSION;
    case TOK_SEP_DOT:
        return TERMINAL_END_EXPRESSION;
    case TOK_AT_MAIN:
        return TERMINAL_END_EXPRESSION;
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
    if (terminal == TERMINAL_PRIORITY_3 &&
        (prev_terminal != TERMINAL_OPERAND && prev_terminal != TERMINAL_CLOSE_PAREN)) {
        return TERMINAL_PRIORITY_1;
    }

    return terminal;
}
