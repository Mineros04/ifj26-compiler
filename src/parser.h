/**
 * @file parser.h
 * @authors Ondřej Pippal
 * @brief Recursive descent parser interface for the IFJ26 compiler.
 */

#pragma once

#include "ast.h"

// Forward declaration of internal parser state
typedef struct Parser Parser;

/**
 * @brief Parses the entire source program from standard input and builds the AST.
 *
 * Terminates with exit code 2 on syntax error, or 99 on internal allocation error.
 *
 * @returns Pointer to the root AST_PROGRAM node (never nullptr).
 */
[[nodiscard]] AstNode *parse_program(void);

/**
 * @brief Parses an expression using precedence analysis (implemented by expression module).
 *
 * @param p Pointer to the active parser state.
 * @returns Pointer to the root AST node of the parsed expression.
 */
[[nodiscard]] AstNode *parse_expression(Parser *p);
