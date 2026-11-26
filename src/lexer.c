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

typedef struct {
    const char *name;
    token_type_t tok_val;
} kw_keyval;

static const kw_keyval KW_TABLE[] = {
    {"def", TOK_KW_DEF},
    {"do", TOK_KW_DO},
    {"Double", TOK_KW_DOUBLE},
    {"else", TOK_KW_ELSE},
    {"if", TOK_KW_IF},
    {"import", TOK_KW_IMPORT},
    {"Int", TOK_KW_INT},
    {"return", TOK_KW_RETURN},
    {"String", TOK_KW_STRING},
    {"then", TOK_KW_THEN},
    {"Unit", TOK_KW_UNIT},
    {"val", TOK_KW_VAL},
    {"var", TOK_KW_VAR},
    {"while", TOK_KW_WHILE},
    {"Boolean", TOK_KW_BOOL},
};

static constexpr size_t KW_TABLE_LEN = sizeof(KW_TABLE) / sizeof(KW_TABLE[0]);

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

static void scan_annotation(lexer_t *lex, token_t *tok) {
    int c;

    if (advance(lex) != 'm' || advance(lex) != 'a' || advance(lex) != 'i' || advance(lex) != 'n' ||
        (c = peek(lex)) == '_' || isalnum(c)) {
        error_throw_compile(ERR_LEXICAL, tok->line, tok->column, "invalid annotation");
    }

    tok->type = TOK_AT_MAIN;
}

// Reads the remaining ciphers. Assumes that the last read character was a cipher. Underscores are
// discarded, ciphers appended to `buf`.
static void scan_digit_tail(lexer_t *lex, const token_t *tok, string_t *buf) {
    while (true) {
        if (isdigit(peek(lex))) {
            str_push(buf, (char)advance(lex));
        } else if (peek(lex) == '_') {
            while (peek(lex) == '_') {
                advance(lex);
            }
            if (!isdigit(peek(lex))) {
                error_throw_compile(ERR_LEXICAL,
                                    tok->line,
                                    tok->column,
                                    "'_' must be between two digits");
            }
        } else {
            return;
        }
    }
}

static void scan_number(lexer_t *lex, token_t *tok, int c) {
    string_t buf   = STR_INIT;
    bool is_double = false;

    if (c == '.') {
        // DOT state: a cipher most follow.
        is_double = true;

        str_push(&buf, '.');
        str_push(&buf, (char)advance(lex));

        scan_digit_tail(lex, tok, &buf);
    } else {
        // INT / INT_ZERO state.
        str_push(&buf, (char)c);
        scan_digit_tail(lex, tok, &buf);

        if (str_cstr(&buf)[0] == '0' && str_len(&buf) > 1) {
            error_throw_compile(ERR_LEXICAL,
                                tok->line,
                                tok->column,
                                "non-zero literal must not start with '0'");
        }

        // VALMEM lookahead.
        if (peek(lex) == '.' && isdigit(peek_at(lex, 1))) {
            is_double = true;

            // '.'
            str_push(&buf, (char)advance(lex));
            // First digit.
            str_push(&buf, (char)advance(lex));

            scan_digit_tail(lex, tok, &buf);
        }
    }

    // Exponent.
    if (peek(lex) == 'e' || peek(lex) == 'E') {
        is_double = true;
        str_push(&buf, (char)advance(lex));

        if (peek(lex) == '+' || peek(lex) == '-') {
            str_push(&buf, (char)advance(lex));
        }

        if (!isdigit(peek(lex))) {
            error_throw_compile(ERR_LEXICAL,
                                tok->line,
                                tok->column,
                                "exponent requires at least one digit");
        }

        str_push(&buf, (char)advance(lex));
        scan_digit_tail(lex, tok, &buf);
    }

    // Suffix (not appended to buf).
    if (peek(lex) == 'd' || peek(lex) == 'D') {
        advance(lex);
        is_double = true;
    }

    // The number must not be followed by a character, a cipher, nor an underscore.
    int next = peek(lex);
    if (isalnum(next) || next == '_') {
        error_throw_compile(ERR_LEXICAL, tok->line, tok->column, "invalid number literal");
    }

    // Parse the number, since buf is valid and doesn't contain underscore suffix.
    char *end = nullptr;

    if (is_double) {
        tok->type       = TOK_LIT_DOUBLE;
        tok->double_val = strtod(str_cstr(&buf), &end);
    } else {
        tok->type    = TOK_LIT_INT;
        tok->int_val = strtoll(str_cstr(&buf), &end, 10);
    }

    if (*end != '\0' || errno == ERANGE) {
        error_throw_internal("failed to convert number literal '%s'", str_cstr(&buf));
    }

    str_free(&buf);
}

static void scan_ident(lexer_t *lex, token_t *tok) {
    size_t start = lex->src_pos - 1;

    while (isalnum(peek(lex)) || peek(lex) == '_') {
        advance(lex);
    }

    const char *begin = str_cstr(lex->src) + start;
    size_t len        = lex->src_pos - start;

    // Keywords.
    for (size_t i = 0; i < KW_TABLE_LEN; i++) {
        if (strlen(KW_TABLE[i].name) == len && strncmp(KW_TABLE[i].name, begin, len) == 0) {
            tok->type = KW_TABLE[i].tok_val;
            return;
        }
    }

    // Boolean literals.
    if (len == 4 && strncmp(begin, "true", 4) == 0) {
        tok->type     = TOK_LIT_BOOL;
        tok->bool_val = true;
        return;
    }
    if (len == 5 && strncmp(begin, "false", 5) == 0) {
        tok->type     = TOK_LIT_BOOL;
        tok->bool_val = false;
        return;
    }

    // Identifier.
    tok->type = TOK_IDENT;
    str_init_from_n(&tok->str_val, begin, len);
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
