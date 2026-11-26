/**
 * @file lexer.c
 * @authors Adam Jaroš
 * @brief The implementation of the compiler's lexer.
 *
 * The lexer is driven on demand by the parser (one token at a time). Lexical errors are handled by
 * the internal error handler, terminating the program with exit code 1.
 */

#include "lexer.h"

void lexer_init(lexer_t *lex, const string_t *src) {
}

void lexer_free(lexer_t *lex) {
}

token_t lexer_next(lexer_t *lex) {
    return (token_t){};
}

const token_t *lexer_peek(lexer_t *lex) {
    return nullptr;
}

void token_free(token_t *tok) {
}

const char *token_type_name(token_type_t type) {
    return nullptr;
}
