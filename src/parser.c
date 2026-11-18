/**
 * @file parser.c
 * @authors Ondřej Pippal
 * @brief Implementation of the recursive descent parser for IFJ26.
 */

#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

#define ERR_SYNTAX 2

/**
 * @brief Internal state of the recursive descent parser.
 */
struct Parser {
    lexer_t *lex;
    token_t current;
};

// Parser state and token stream helpers

/**
 * @brief Reports a syntax error and terminates the compiler with exit code 2.
 */
[[noreturn]] static void syntax_error(const Parser *p, const char *msg) {
    fprintf(stderr, "Syntax error at %zu:%zu (near %s): %s\n", p->current.line, p->current.column,
            token_type_name(p->current.type), msg);
    exit(ERR_SYNTAX);
}

/**
 * @brief Releases the current token and fetches the next one from the lexer.
 */
static void parser_advance(Parser *p) {
    token_free(&p->current);
    p->current = lexer_next(p->lex);
}

/**
 * @brief Peeks at the type of the next token without consuming the current token.
 */
static token_type_t parser_peek_next(Parser *p) {
    return lexer_peek(p->lex)->type;
}

/**
 * @brief Skips any consecutive logical newline tokens.
 */
static void parser_skip_newlines(Parser *p) {
    while (p->current.type == TOK_NEWLINE) {
        parser_advance(p);
    }
}

/**
 * @brief Skips statement separators (`TOK_NEWLINE` and `TOK_SEP_SEMI`).
 */
static void parser_skip_separators(Parser *p) {
    while (p->current.type == TOK_NEWLINE || p->current.type == TOK_SEP_SEMI) {
        parser_advance(p);
    }
}

/**
 * @brief Consumes the current token if it matches `type`.
 */
static bool parser_accept(Parser *p, token_type_t type) {
    if (p->current.type == type) {
        parser_advance(p);
        return true;
    }
    return false;
}

/**
 * @brief Requires the current token to match `type`, otherwise terminates with Error 2.
 */
static void parser_expect(Parser *p, token_type_t type, const char *err_msg) {
    if (!parser_accept(p, type)) {
        syntax_error(p, err_msg);
    }
}

/**
 * @brief Parses a data type keyword (`Int`, `Double`, `String`, `Unit`).
 */
static DataType parse_data_type(Parser *p) {
    switch (p->current.type) {
        case TOK_KW_INT:
            parser_advance(p);
            return TYPE_INT;
        case TOK_KW_DOUBLE:
            parser_advance(p);
            return TYPE_DOUBLE;
        case TOK_KW_STRING:
            parser_advance(p);
            return TYPE_STRING;
        case TOK_KW_UNIT:
            parser_advance(p);
            return TYPE_UNIT;
        default:
            syntax_error(p, "expected a valid data type (Int, Double, String, Unit)");
    }
}

// Prologue, blocks, and function definitions

static AstNode *parse_statement(Parser *p);

/**
 * @brief Parses the mandatory `import ifj26.*` prologue.
 */
static void parse_prologue(Parser *p) {
    parser_skip_newlines(p);
    parser_expect(p, TOK_KW_IMPORT, "expected 'import ifj26.*' prologue at start of file");

    if (p->current.type != TOK_IDENT || !str_equals_cstr(&p->current.str_val, "ifj26")) {
        syntax_error(p, "expected 'ifj26' after 'import'");
    }
    parser_advance(p);

    parser_expect(p, TOK_SEP_DOT, "expected '.' after 'ifj26'");
    parser_expect(p, TOK_OP_STAR, "expected '*' in 'import ifj26.*'");

    if (p->current.type != TOK_NEWLINE && p->current.type != TOK_EOF) {
        syntax_error(p, "expected newline after 'import ifj26.*'");
    }
}

/**
 * @brief Parses an indented block (`TOK_INDENT ... TOK_DEDENT`).
 */
static AstNode *parse_indented_block(Parser *p) {
    int line = (int)p->current.line;
    parser_expect(p, TOK_INDENT, "expected indented block");

    AstNode *block = ast_new_block(line);
    parser_skip_separators(p);

    while (p->current.type != TOK_DEDENT && p->current.type != TOK_EOF) {
        AstNode *stmt = parse_statement(p);
        ast_list_append(&block->block.statements, stmt);

        if (p->current.type == TOK_NEWLINE || p->current.type == TOK_SEP_SEMI) {
            parser_skip_separators(p);
        } else if (p->current.type != TOK_DEDENT && p->current.type != TOK_EOF) {
            syntax_error(p, "expected newline or ';' between statements");
        }
    }

    parser_expect(p, TOK_DEDENT, "expected dedent to close block");
    return block;
}

/**
 * @brief Parses either an indented multi-line block or a single inline statement.
 */
static AstNode *parse_block_or_stmt(Parser *p) {
    if (p->current.type == TOK_NEWLINE) {
        parser_skip_newlines(p);
        return parse_indented_block(p);
    }
    if (p->current.type == TOK_INDENT) {
        return parse_indented_block(p);
    }

    int line = (int)p->current.line;
    AstNode *block = ast_new_block(line);
    ast_list_append(&block->block.statements, parse_statement(p));
    return block;
}

/**
 * @brief Parses a formal parameter list `(id: Type, ...)`.
 */
static void parse_param_list(Parser *p, AstNode *func_node) {
    parser_expect(p, TOK_SEP_LPAREN, "expected '(' after function name");

    if (p->current.type != TOK_SEP_RPAREN) {
        while (true) {
            if (p->current.type != TOK_IDENT) {
                syntax_error(p, "expected parameter identifier");
            }
            int param_line = (int)p->current.line;
            char *param_name = str_dup_cstr(&p->current.str_val);
            parser_advance(p);

            parser_expect(p, TOK_SEP_COLON, "expected ':' after parameter name");
            DataType param_type = parse_data_type(p);

            AstNode *param = ast_new_param(param_name, param_type, param_line);
            free(param_name);
            ast_list_append(&func_node->func_def.params, param);

            if (!parser_accept(p, TOK_SEP_COMMA)) {
                break;
            }
            // Allow trailing comma before ')' if separated by a newline
            if (p->current.type == TOK_SEP_RPAREN && p->current.after_newline) {
                break;
            }
        }
    }

    parser_expect(p, TOK_SEP_RPAREN, "expected ')' after parameter list");
}

/**
 * @brief Parses a top-level function definition `[@main] def name(...) [: Type] = body`.
 */
static AstNode *parse_func_def(Parser *p, bool is_main) {
    int line = (int)p->current.line;
    parser_expect(p, TOK_KW_DEF, "expected 'def' keyword");

    if (p->current.type != TOK_IDENT) {
        syntax_error(p, "expected function identifier after 'def'");
    }

    char *func_name = str_dup_cstr(&p->current.str_val);
    parser_advance(p);

    AstNode *func_node = ast_new_func_def(func_name, is_main, TYPE_UNIT, nullptr, line);
    free(func_name);

    parse_param_list(p, func_node);

    if (parser_accept(p, TOK_SEP_COLON)) {
        func_node->func_def.return_type = parse_data_type(p);
    }

    parser_expect(p, TOK_OP_ASSIGN, "expected '=' before function body");
    func_node->func_def.body = parse_block_or_stmt(p);
    return func_node;
}