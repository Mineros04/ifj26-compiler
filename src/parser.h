/**
 * @file parser.h
 * @brief Rozhraní syntaktického analyzátoru (rekurzivní sestup).
 *
 * Projekt: Implementace překladače imperativního jazyka IFJ26
 * Autori: tbd
 */

#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

/**
 * @brief Spustí syntaktickou analýzu vstupního programu ze standardního vstupu.
 *
 * Funkce čte tokeny z lexikálního analyzátoru, ověřuje gramatickou správnost
 * programu metodou rekurzivního sestupu a staví Abstraktní syntaktický strom (AST).
 * Při syntaktické chybě ukončí program s návratovým kódem 2.
 *
 * @return Ukazatel na kořenový uzel AST (AST_PROGRAM).
 */
AstNode *parse_program(void);

#endif // PARSER_H
