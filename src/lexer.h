/**
 * @file lexer.h
 * @authors Adam Jaroš
 * @brief Public API for the lexer.
 *
 * The lexer is driven on-demand by the parser (one token at a time). Lexical errors are handled by
 * the internal error handler, terminating the program with exit code 1.
 */

#pragma once

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

