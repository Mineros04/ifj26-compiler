/**
* @file semantic.h
 * @authors Pavel Bobek
 * @brief Header file for the semantic analyzer for IFJ26.
 */

#pragma once

#include "symtable.h"
#include "shared/error.h"
#include "shared/error.h"
#include <string.h>

void semantic_analyzer(AstNode *program);