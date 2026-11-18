/**
 * @file parser.c
 * @authors Ondřej Pippal
 * @brief Implementation of the recursive descent parser for IFJ26.
 */

#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

#define ERR_SYNTAX 2

/**
 * @brief Internal state of the recursive descent parser.
 */
struct Parser {
    lexer_t *lex;
    token_t current;
};

// ============================================================================
// Parser state and token stream helpers
// ============================================================================

/**
 * @brief Reports a syntax error and terminates the compiler with exit code 2.
 */
[[noreturn]] static void syntax_error(const Parser *p, const char *msg) {
    fprintf(stderr, "Syntax error at %zu:%zu (near %s): %s\n", p->current.line, p->current.column,
            token_type_name(p->current.type), msg);
    exit(ERR_SYNTAX);
}

/**
 * @brief Releases the current token and fetches the next one from the lexer.
 */
static void parser_advance(Parser *p) {
    token_free(&p->current);
    p->current = lexer_next(p->lex);
}

/**
 * @brief Peeks at the type of the next token without consuming the current token.
 */
static token_type_t parser_peek_next(Parser *p) {
    return lexer_peek(p->lex)->type;
}

/**
 * @brief Skips any consecutive logical newline tokens.
 */
static void parser_skip_newlines(Parser *p) {
    while (p->current.type == TOK_NEWLINE) {
        parser_advance(p);
    }
}

/**
 * @brief Skips statement separators (`TOK_NEWLINE` and `TOK_SEP_SEMI`).
 */
static void parser_skip_separators(Parser *p) {
    while (p->current.type == TOK_NEWLINE || p->current.type == TOK_SEP_SEMI) {
        parser_advance(p);
    }
}

/**
 * @brief Consumes the current token if it matches `type`.
 */
static bool parser_accept(Parser *p, token_type_t type) {
    if (p->current.type == type) {
        parser_advance(p);
        return true;
    }
    return false;
}

/**
 * @brief Requires the current token to match `type`, otherwise terminates with Error 2.
 */
static void parser_expect(Parser *p, token_type_t type, const char *err_msg) {
    if (!parser_accept(p, type)) {
        syntax_error(p, err_msg);
    }
}

/**
 * @brief Parses a data type keyword (`Int`, `Double`, `String`, `Unit`).
 */
static DataType parse_data_type(Parser *p) {
    switch (p->current.type) {
        case TOK_KW_INT:
            parser_advance(p);
            return TYPE_INT;
        case TOK_KW_DOUBLE:
            parser_advance(p);
            return TYPE_DOUBLE;
        case TOK_KW_STRING:
            parser_advance(p);
            return TYPE_STRING;
        case TOK_KW_UNIT:
            parser_advance(p);
            return TYPE_UNIT;
        default:
            syntax_error(p, "expected a valid data type (Int, Double, String, Unit)");
    }
}