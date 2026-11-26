/**
 * @file lexer.c
 * @authors Adam Jaroš
 * @brief The implementation of the compiler's lexer.
 *
 * The lexer is driven on demand by the parser (one token at a time). Lexical errors are handled by
 * the internal error handler, terminating the program with exit code 1.
 */

#include "lexer.h"

#include "shared/error.h"
#include "shared/str.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

// The Microsoft C/C++ extension for VS Code likes to complain about C23 features. So, we shut it.
#ifdef __INTELLISENSE__
#define constexpr const
#endif

// ===== Internal =====

#define EOF (-1)

static int peek_at(const lexer_t *lex, size_t offset) {
    size_t peek_idx = lex->src_pos + offset;
    return peek_idx < str_len(lex->src) ? (unsigned char)str_cstr(lex->src)[peek_idx] : EOF;
}

static inline int peek(const lexer_t *lex) {
    return peek_at(lex, 0);
}

static int advance(lexer_t *lex) {
    int c = peek(lex);

    if (c == EOF) {
        return EOF;
    }

    lex->src_pos++;

    if (c == '\n') {
        lex->src_line++;
        lex->src_column = 1;
    } else {
        lex->src_column++;
    }

    return c;
}

static bool match_next(lexer_t *lex, char expect) {
    if (peek(lex) != expect) {
        return false;
    }

    advance(lex);

    return true;
}

// Skip a (possibly nested) multi-line comment. Returns true if it contained a newline.
static bool skip_ml_comment(lexer_t *lex, size_t line, size_t col) {
    size_t depth     = 1;
    bool has_newline = false;

    while (depth > 0) {
        int c = advance(lex);
        if (c == EOF) {
            error_throw_compile(ERR_LEXICAL, line, col, "unterminated comment");
        } else if (c == '\n') {
            has_newline = true;
        } else if (c == '/' && match_next(lex, '*')) {
            depth++;
        } else if (c == '*' && match_next(lex, '/')) {
            depth--;
        }
    }

    return has_newline;
}

// Skip a single-line comment, leaving the terminating '\n' in the input.
static void skip_sl_comment(lexer_t *lex) {
    while (peek(lex) != '\n' && peek(lex) != EOF) {
        advance(lex);
    }
}

// TODO: refactor (two functions seem ok, but at least check).
static bool skip_one_insignificant(lexer_t *lex, bool *expect_eol) {
    int c = peek(lex);

    if (c == ' ' || c == '\t' || c == '\r') {
        advance(lex);
        return true;
    }

    if (c != '/') {
        return false;
    }

    int next = peek_at(lex, 1);
    if (next == '/') {
        advance(lex);
        advance(lex);
        skip_sl_comment(lex);
        return true;
    }
    if (next == '*') {
        size_t line = lex->src_line, col = lex->src_column;
        advance(lex);
        advance(lex);
        if (skip_ml_comment(lex, line, col)) {
            *expect_eol = true;
        }
        return true;
    }

    return false;
}

static void skip_insignificant(lexer_t *lex) {
    bool expect_eol = false; // a multiline block comment was skipped

    while (skip_one_insignificant(lex, &expect_eol)) {
    }

    if (expect_eol && peek(lex) != '\n' && peek(lex) != EOF) {
        error_throw_compile(ERR_LEXICAL,
                            lex->src_line,
                            lex->src_column,
                            "unexpected token after multiline comment");
    }
}

static void scan_string(lexer_t *lex, token_t *tok) {
    // Initial '"' has been processed.
    string_t res = STR_INIT;
    int c;

    if (peek(lex) == '"' && peek_at(lex, 1) == '"') {
        // Multi-line string handling.

        advance(lex);
        advance(lex);

        while (!(peek(lex) == '"' && peek_at(lex, 1) == '"' && peek_at(lex, 2) == '"')) {
            if (peek(lex) == EOF) {
                error_throw_compile(ERR_LEXICAL,
                                    lex->src_line,
                                    lex->src_column,
                                    "unterminated multiline string");
            }
            str_push(&res, advance(lex));
        }

        advance(lex);
        advance(lex);
        advance(lex);
    } else {
        // Single-line string handling.

        while ((c = advance(lex)) != '"') {
            if (c == EOF || c == '\n') {
                error_throw_compile(ERR_LEXICAL,
                                    lex->src_line,
                                    lex->src_column,
                                    "unterminated string literal");
            }
            if (c == '\\') {
                switch (c = advance(lex)) {
                case '\\':
                    c = '\\';
                    break;
                case '"':
                    c = '"';
                    break;
                case 'n':
                    c = '\n';
                    break;
                case 'r':
                    c = '\r';
                    break;
                case 't':
                    c = '\t';
                    break;
                default:
                    error_throw_compile(ERR_LEXICAL,
                                        lex->src_line,
                                        lex->src_column,
                                        "unexpected escape sequence: '\\%c'",
                                        c);
                }
            }

            str_push(&res, c);
        }
    }

    tok->type    = TOK_LIT_STRING;
    tok->str_val = res;
}

static token_t scan_token(lexer_t *lex) {
    skip_insignificant(lex);

    token_t tok = {
        .line   = lex->src_line,
        .column = lex->src_column,
    };
    int c = advance(lex);

    switch (c) {
    case EOF:
        tok.type = TOK_EOF;
        break;
    case '\n':
        tok.type = TOK_NEWLINE;
        break;
    case '+':
        tok.type = TOK_OP_PLUS;
        break;
    case '-':
        tok.type = TOK_OP_MINUS;
        break;
    case '*':
        tok.type = TOK_OP_STAR;
        break;
    case '/':
        // Comments are already filtered out at this point.
        tok.type = TOK_OP_SLASH;
        break;
    case '(':
        tok.type = TOK_SEP_LPAREN;
        break;
    case ')':
        tok.type = TOK_SEP_RPAREN;
        break;
    case ',':
        tok.type = TOK_SEP_COMMA;
        break;
    case ':':
        tok.type = TOK_SEP_COLON;
        break;
    case ';':
        tok.type = TOK_SEP_SEMI;
        break;
    case '=':
        tok.type = match_next(lex, '=') ? TOK_OP_EQ : TOK_OP_ASSIGN;
        break;
    case '!':
        tok.type = match_next(lex, '=') ? TOK_OP_NEQ : TOK_OP_NOT;
        break;
    case '<':
        tok.type = match_next(lex, '=') ? TOK_OP_LE : TOK_OP_LT;
        break;
    case '>':
        tok.type = match_next(lex, '=') ? TOK_OP_GE : TOK_OP_GT;
        break;
    case '&':
        if (!match_next(lex, '&')) {
            error_throw_compile(ERR_LEXICAL, tok.line, tok.column, "unexpected '&'");
        }
        tok.type = TOK_OP_AND;
        break;
    case '"':
        scan_string(lex, &tok);
        break;
    case '@':
        scan_annotation(lex, &tok);
        break;
    case '.':
        if (isdigit(peek(lex))) {
            scan_number(lex, &tok, c);
        } else {
            tok.type = TOK_SEP_DOT;
        }
        break;
    default:
        if (isdigit(c)) {
            scan_number(lex, &tok, c);
        } else if (isalpha(c) || c == '_') {
            scan_ident(lex, &tok);
        } else {
            error_throw_compile(ERR_LEXICAL, tok.line, tok.column, "unexpected character '%c'", c);
        }
    }

    return tok;
}

// ===== Lexer =====

void lexer_init(lexer_t *lex, const string_t *src) {
memset(lex, 0, sizeof(lexer_t));
    lex->src = src;

    lex->src_line   = 1;
    lex->src_column = 1;

    // TODO: alloc indent stack with base level 0.

    lex->at_line_start = true;
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
