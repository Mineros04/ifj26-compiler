/**
 * @file semantic.h
 * @authors Pavel Bobek
 * @brief Header file for the semantic analyzer for IFJ26.
 */

#pragma once

#include "shared/error.h"
#include "symtable.h"
#include <string.h>

/**
 * @brief Main entry point for semantic analyzer.
 *
 * Checks all used functions if they exist.
 * All have return keyword in all logical branches and have a correct usage of variables in all
 * scopes and their DataTypes.
 *
 * @param program The main branch of the program with list of all functions.
 */
void semantic_analyzer(AstNode *program);
