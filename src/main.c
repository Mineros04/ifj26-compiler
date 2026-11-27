/**
 * @file main.c
 * @authors Ondřej Pippal
 * @brief Entry point of the IFJ26 compiler.
 */

#include "ast.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include "shared/error.h"
#include "shared/str.h"
#include <stdio.h>

#define STDIN_CHUNK_SIZE 4096

/**
 * @brief Reads the entire standard input into a dynamic string buffer.
 */
static void read_stdin(string_t *out) {
    char buf[STDIN_CHUNK_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buf, 1, sizeof(buf), stdin)) > 0) {
        str_append_n(out, buf, bytes_read);
    }
    if (ferror(stdin)) {
        error_throw_internal("failed to read source code from stdin");
    }
}

int main(void) {
    string_t src = STR_INIT;
    read_stdin(&src);

    lexer_t lex = {0};
    lexer_init(&lex, &src);

    AstNode *program = parse_program(&lex);
    semantic_analyzer(program);
    codegen_generate(program, stdout);

    ast_free(program);
    lexer_free(&lex);
    str_free(&src);
    return 0;
}
