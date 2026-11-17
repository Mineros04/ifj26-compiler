/**
 * @file lexer.h
 * @authors Adam Jaroš
 * @brief Public API for the lexer.
 *
 * The lexer is driven on-demand by the parser (one token at a time). Lexical errors are handled by
 * the internal error handler, terminating the program with exit code 1.
 */

#pragma once
