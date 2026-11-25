/**
 * @file parser.h
 * @authors Ondřej Pippal
 * @brief Recursive descent parser interface for the IFJ26 compiler.
 */

#pragma once

#include "ast.h"
#include "lexer.h"

// Forward declaration of internal parser state
typedef struct Parser Parser;

/**
 * @brief Parses the entire source program using the given lexer and builds the AST.
 *
 * @param lex Pointer to an initialized lexer instance.
 * @returns Pointer to the root AST_PROGRAM node (never `nullptr`).
 *
 * @throws Error 1 - Lexical error
 * @throws Error 2 - Syntax error
 * @throws Error 99 - Allocation failed
 */
[[nodiscard]] AstNode *parse_program(lexer_t *lex);

/**
 * @brief Parses an expression using precedence analysis (implemented by expression module).
 *
 * @param p Pointer to the active parser state.
 * @returns Pointer to the root AST node of the parsed expression.
 *
 * @throws Error 1 - Lexical error
 * @throws Error 2 - Syntax error
 * @throws Error 99 - Allocation failed
 */
[[nodiscard]] AstNode *parse_expression(Parser *p);

/**
 * @brief Returns a read-only pointer to the current token held by the parser.
 *
 * @param p Pointer to the active parser state.
 * @returns Pointer to the current token (valid until the next `parser_advance` call).
 */
[[nodiscard]] const token_t *parser_current(const Parser *p);

/**
 * @brief Releases the current token and fetches the next one from the lexer.
 *
 * @param p Pointer to the active parser state.
 *
 * @throws Error 1 - Lexical error
 * @throws Error 99 - Allocation failed
 */
void parser_advance(Parser *p);

/**
 * @brief Peeks at the type of the next token without consuming the current token.
 *
 * @param p Pointer to the active parser state.
 * @returns The `token_type_t` of the next token.
 *
 * @throws Error 1 - Lexical error
 * @throws Error 99 - Allocation failed
 */
[[nodiscard]] token_type_t parser_peek_next(Parser *p);

/**
 * @brief Reports a syntax error at the current token and terminates the compiler with exit code 2.
 *
 * @param p Pointer to the active parser state.
 * @param msg Error description message.
 *
 * @throws Error 2 - Syntax error
 */
[[noreturn]] void syntax_error(const Parser *p, const char *msg);
