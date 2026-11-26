/**
 * @file lexer.h
 * @authors Adam Jaroš
 * @brief Public API for the lexer.
 *
 * The lexer is driven on demand by the parser (one token at a time). Lexical errors are handled by
 * the internal error handler, terminating the program with exit code 1.
 */

#pragma once

#include <stdint.h>

#include "shared/str.h"

/**
 * @brief The kind of a token.
 */
typedef enum {
    // ===== Source code layout =====

    /// An increase of the indentation level by one (start of a block).
    TOK_INDENT,
    /// A decrease of the indentation level by one (end of a block).
    TOK_DEDENT,
    /// The end of a logical line. Not emitted inside parentheses, on empty lines or on lines
    /// containing only whitespace and comments.
    TOK_NEWLINE,
    /// The end of input.
    TOK_EOF,

    // ===== Identifiers and literals =====

    /// An identifier. Its name is stored in `str_val`.
    TOK_IDENT,
    /// An integer literal. Its value is stored in `int_val`. A negative integer (e.g. `-1`) is
    /// tokenized as `TOK_OP_MINUS` followed by `TOK_LIT_INT`.
    TOK_LIT_INT,
    /// A floating-point literal. Its value is stored in `double_val`.
    TOK_LIT_DOUBLE,
    /// A string literal, either single-line or multiline (`"""`). Its value is stored in `str_val`,
    /// with all escape sequences already evaluated.
    TOK_LIT_STRING,

    // ===== Keywords =====

    /// The `import` keyword.
    TOK_KW_IMPORT,
    /// The `def` function declaration keyword.
    TOK_KW_DEF,
    /// The `if` statement keyword.
    TOK_KW_IF,
    /// The `then` conditional keyword.
    TOK_KW_THEN,
    /// The `else` statement keyword.
    TOK_KW_ELSE,
    /// The `while` loop keyword.
    TOK_KW_WHILE,
    /// The `do` loop keyword.
    TOK_KW_DO,
    /// The `return` keyword.
    TOK_KW_RETURN,
    /// The `val` immutable variable keyword.
    TOK_KW_VAL,
    /// The `var` mutable variable keyword.
    TOK_KW_VAR,
    /// The `Int` type keyword.
    TOK_KW_INT,
    /// The `Double` type keyword.
    TOK_KW_DOUBLE,
    /// The `String` type keyword.
    TOK_KW_STRING,
    /// The `Unit` type keyword.
    TOK_KW_UNIT,
    // To Be Decided (extensions): TOK_KW_BOOLEAN, TOK_KW_CASE, ...

    // ===== Operators and separators =====

    /// The `+` operator (addition, concatenation or unary plus).
    TOK_OP_PLUS,
    /// The `-` operator (subtraction or unary minus).
    TOK_OP_MINUS,
    /// The `*` operator (multiplication, or the wildcard in the `import ifj26.*` prologue).
    TOK_OP_STAR,
    /// The `/` operator.
    TOK_OP_SLASH,
    /// The `==` operator.
    TOK_OP_EQ,
    /// The `!=` operator.
    TOK_OP_NEQ,
    /// The `<` operator.
    TOK_OP_LT,
    /// The `<=` operator.
    TOK_OP_LE,
    /// The `>` operator.
    TOK_OP_GT,
    /// The `>=` operator.
    TOK_OP_GE,
    /// The `=` sign. Expresses both assignment and definition.
    TOK_OP_ASSIGN,
    /// The `(` separator.
    TOK_SEP_LPAREN,
    /// The `)` separator.
    TOK_SEP_RPAREN,
    /// The `,` separator.
    TOK_SEP_COMMA,
    /// The `:` separator.
    TOK_SEP_COLON,
    /// The `;` separator.
    TOK_SEP_SEMI,
    /// The `.` separator (only in the `import ifj26.*` prologue in the base language).
    TOK_SEP_DOT,
    /// The `@main` annotation.
    TOK_AT_MAIN,
    // To Be Decided (extensions): TOK_UNDERSCORE, TOK_AND, TOK_OR, TOK_NOT...
} token_type_t;

/**
 * @brief A single token produced by the lexer.
 *
 * @warning Tokens of type `TOK_IDENT` and `TOK_LIT_STRING` own their `str_val` and must be released
 * with `token_free` (or have the string's ownership taken over, e.g. via `str_to_owned`).
 */
typedef struct {
    /// The kind of the token, which also determines the active member of the union.
    token_type_t type;
    /// The line (counted from 1) on which the token starts.
    size_t line;
    /// The column (counted from 1) at which the token starts.
    size_t column;
    /// Whether a physical line break precedes the token (used e.g. for trailing commas, as
    /// `TOK_NEWLINE` is not emitted inside parentheses).
    bool after_newline;

    union {
        /// The value of a `TOK_LIT_INT` token.
        int64_t int_val;
        /// The value of a `TOK_LIT_DOUBLE` token.
        double double_val;
        /// The name of a `TOK_IDENT` token or the value of a `TOK_LIT_STRING` token.
        string_t str_val;
    };
} token_t;

/**
 * @brief The state of the lexer.
 * @warning All fields are internal and must not be accessed outside of the lexer implementation.
 */
typedef struct {
    /// The source code that is being tokenized (borrowed, must outlive the lexer).
    const string_t *src;
    /// The current position in `src`.
    size_t src_pos;
    /// The current line (counted from 1).
    size_t src_line;
    /// The current column (counted from 1).
    size_t src_column;

    /// The stack of indentation levels of the currently open blocks.
    size_t *indent_stack;
    /// The number of levels on the indentation stack.
    size_t indent_len;
    /// The capacity of the indentation stack.
    size_t indent_cap;
    /// The number of `TOK_DEDENT` tokens yet to be emitted.
    size_t pending_dedents;

    /// The current nesting depth of parentheses.
    size_t parens_depth;
    /// Whether the lexer is at the start of a logical line (indentation is to be measured).
    bool at_line_start;

    /// The token read ahead by `lexer_peek`.
    token_t lookahead;
    /// Whether `lookahead` holds a valid token.
    bool has_lookahead;
} lexer_t;

/**
 * @brief Initializes the lexer to tokenize the given source code. Terminates the program on
 * allocation failure.
 * @param lex The pointer to the lexer to be initialized.
 * @param src The source code to be tokenized. It is borrowed and must outlive the lexer.
 *
 * @throws Error 99 - Allocation failed
 */
void lexer_init(lexer_t *lex, const string_t *src);

/**
 * @brief Releases all resources contained in the lexer (including a pending lookahead token).
 * @param lex The pointer to the lexer to be freed.
 */
void lexer_free(lexer_t *lex);

/**
 * @brief Reads the next token from the source code and processes it. Terminates the program on a
 * lexical error or an allocation failure.
 * @param lex The pointer to the lexer to read from.
 * @returns The next token. Its ownership is transferred to the caller. Once `TOK_EOF` has been
 * returned, every further call returns `TOK_EOF` again.
 *
 * @throws Error 1 - Lexical error
 * @throws Error 99 - Allocation failed
 */
[[nodiscard]] token_t lexer_next(lexer_t *lex);

/**
 * @brief Returns the next token without processing it. Terminates the program on a lexical error or
 * an allocation failure.
 * @param lex The pointer to the lexer to read from.
 * @returns The pointer to the next token (never `nullptr`). It is owned by the lexer and stays
 * valid only until the next call of `lexer_next` or `lexer_free`.
 *
 * @throws Error 1 - Lexical error
 * @throws Error 99 - Allocation failed
 */
const token_t *lexer_peek(lexer_t *lex);

/**
 * @brief Releases the resources owned by the token; works with any token type.
 * @param tok The pointer to the token to be freed.
 */
void token_free(token_t *tok);

/**
 * @brief Returns a human-readable name of the token type, meant for error messages (e.g.
 * `"identifier"` or `"'<='"`).
 * @param type The token type to have its name returned.
 * @returns A statically allocated C string containing the token type name (never `nullptr`).
 */
const char *token_type_name(token_type_t type);
