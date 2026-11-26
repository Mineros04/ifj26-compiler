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
 * @brief Parses a function call `id(arg1, arg2, ...)`.
 *
 * Expects `p->current` to be `TOK_IDENT` (the function name) followed by `TOK_SEP_LPAREN`.
 * Consumes the entire call including the closing `')'`, leaving `p->current` at the next token.
 *
 * @param p Pointer to the active parser state.
 * @returns Pointer to the created AST_FUNC_CALL node.
 *
 * @throws Error 1 - Lexical error
 * @throws Error 2 - Syntax error
 * @throws Error 99 - Allocation failed
 */
[[nodiscard]] AstNode *parse_func_call(Parser *p);

