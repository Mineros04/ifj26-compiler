/**
 * @file lexer.c
 * @authors Adam Jaroš
 * @brief The implementation of the compiler's lexer.
 *
 * The lexer is driven on demand by the parser (one token at a time). Lexical errors are handled by
 * the internal error handler, terminating the program with exit code 1.
 */
