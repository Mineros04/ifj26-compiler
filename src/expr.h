/**
 * @file expr.h
 * @authors Tomáš Komenda (xkoment00)
 * @brief Expression parser interface.
 */

#pragma once

#include "ast.h"    // AstNode
#include "parser.h" // lexer_t, token_type_t

/**
 * @brief Parses an expression using precedence analysis (implemented by expression module).
 *
 * @param p Pointer to the active parser state.
 * @returns Pointer to the root AST node of the parsed expression.
 *
 * @throws Error 1 - Lexical error
 * @throws Error 2 - Syntax error
 * @throws Error 99 - Allocation failed
 *
 * Copied from "parser.h"
 */
[[nodiscard]] AstNode *parse_expression(Parser *p);
