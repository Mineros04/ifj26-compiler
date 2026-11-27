/**
 * @file codegen.h
 * @authors Pavel Bobek
 * @brief Header file for the code generator for IFJ26.
 */

#pragma once

#include "ast.h"
#include "shared/error.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Entry point of the IFJ26 code generator.
 *
 * Emits the .IFJcode26 header, global temporary variables, initial jump to main,
 * and generates target code for all function definitions in the program AST.
 *
 * @param program Pointer to the root AST_PROGRAM node.
 * @param out Output file stream (typically stdout).
 */
void codegen_generate(const AstNode *program, FILE *out);
