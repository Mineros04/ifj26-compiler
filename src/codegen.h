#ifndef IFJ26_COMPILER_CODEGEN_H
#define IFJ26_COMPILER_CODEGEN_H

#pragma once

#include "ast.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

void codegen_generate(AstNode *program, FILE *out);

#endif //IFJ26_COMPILER_CODEGEN_H