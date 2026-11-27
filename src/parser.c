/**
 * @file parser.c
 * @authors Ondřej Pippal
 * @brief Implementation of the recursive descent parser for IFJ26.
 */

#include "parser.h"
#include "shared/error.h"
#include <stdlib.h>

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
[[noreturn]] void syntax_error(const Parser *p, const char *msg) {
    error_throw_compile(ERR_SYNTAX,
                        p->current.line,
                        p->current.column,
                        "near %s: %s",
                        token_type_name(p->current.type),
                        msg);
}

const token_t *parser_current(const Parser *p) {
    return &p->current;
}

/**
 * @brief Releases the current token and fetches the next one from the lexer.
 */
void parser_advance(Parser *p) {
    token_free(&p->current);
    p->current = lexer_next(p->lex);
}

/**
 * @brief Peeks at the type of the next token without consuming the current token.
 */
token_type_t parser_peek_next(Parser *p) {
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
 * @brief Parses a data type keyword (`Int`, `Double`, `String`, `Boolean`, `Unit`).
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
    case TOK_KW_BOOL:
        parser_advance(p);
        return TYPE_BOOL;
    case TOK_KW_UNIT:
        parser_advance(p);
        return TYPE_UNIT;
    default:
        syntax_error(p, "expected a valid data type (Int, Double, String, Boolean, Unit)");
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
        } else if (!p->current.after_newline &&
                   p->current.type != TOK_DEDENT &&
                   p->current.type != TOK_EOF) {
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

    int line       = (int)p->current.line;
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
            int param_line   = (int)p->current.line;
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

    AstNode *func_node = ast_new_func_def(func_name, is_main, TYPE_UNKNOWN, nullptr, line);
    free(func_name);

    while (p->current.type == TOK_SEP_LPAREN) {
        parse_param_list(p, func_node);
    }

    if (parser_accept(p, TOK_SEP_COLON)) {
        func_node->func_def.return_type = parse_data_type(p);
    }

    parser_expect(p, TOK_OP_ASSIGN, "expected '=' before function body");
    func_node->func_def.body = parse_block_or_stmt(p);
    return func_node;
}
// Statement parsing

/**
 * @brief Parses a variable declaration (`val` or `var`).
 */
static AstNode *parse_var_decl(Parser *p) {
    int line    = (int)p->current.line;
    bool is_var = (p->current.type == TOK_KW_VAR);
    parser_advance(p);

    if (p->current.type != TOK_IDENT) {
        syntax_error(p, "expected variable identifier in declaration");
    }
    char *var_name = str_dup_cstr(&p->current.str_val);
    parser_advance(p);

    DataType var_type = TYPE_UNKNOWN;
    if (parser_accept(p, TOK_SEP_COLON)) {
        var_type = parse_data_type(p);
    }

    parser_expect(p, TOK_OP_ASSIGN, "expected '=' in variable declaration");
    AstNode *init_expr = parse_expression(p);

    AstNode *node = ast_new_var_decl(var_name, is_var, var_type, init_expr, line);
    free(var_name);
    return node;
}

/**
 * @brief Parses an `if` statement (`if cond then body [else body]`).
 */
static AstNode *parse_if_stmt(Parser *p) {
    int line = (int)p->current.line;
    parser_expect(p, TOK_KW_IF, "expected 'if'");

    AstNode *cond = parse_expression(p);
    parser_expect(p, TOK_KW_THEN, "expected 'then' after if condition");

    AstNode *then_branch = parse_block_or_stmt(p);
    AstNode *else_branch = nullptr;

    if (p->current.type == TOK_NEWLINE && parser_peek_next(p) == TOK_KW_ELSE) {
        parser_advance(p);
    }

    if (parser_accept(p, TOK_KW_ELSE)) {
        else_branch = parse_block_or_stmt(p);
    }

    return ast_new_if(cond, then_branch, else_branch, line);
}

/**
 * @brief Parses a `while` loop (`while cond do body`).
 */
static AstNode *parse_while_stmt(Parser *p) {
    int line = (int)p->current.line;
    parser_expect(p, TOK_KW_WHILE, "expected 'while'");

    AstNode *cond = parse_expression(p);
    parser_expect(p, TOK_KW_DO, "expected 'do' after while condition");

    AstNode *body = parse_block_or_stmt(p);
    return ast_new_while(cond, body, line);
}

/**
 * @brief Parses a `return` statement.
 */
static AstNode *parse_return_stmt(Parser *p) {
    int line = (int)p->current.line;
    parser_expect(p, TOK_KW_RETURN, "expected 'return'");

    AstNode *expr = nullptr;
    if (p->current.type != TOK_NEWLINE && p->current.type != TOK_DEDENT &&
        p->current.type != TOK_SEP_SEMI && p->current.type != TOK_EOF) {
        expr = parse_expression(p);
    }
    return ast_new_return(expr, line);
}

AstNode *parse_func_call(Parser *p) {
    if (p->current.type != TOK_IDENT) {
        syntax_error(p, "expected function identifier in call");
    }
    int line   = (int)p->current.line;
    char *name = str_dup_cstr(&p->current.str_val);
    parser_advance(p);

    AstNode *call_node = ast_new_func_call(name, line);
    free(name);

    do {
        parser_expect(p, TOK_SEP_LPAREN, "expected '(' after function name");

        if (p->current.type != TOK_SEP_RPAREN) {
            while (true) {
                AstNode *arg = parse_expression(p);
                ast_list_append(&call_node->func_call.args, arg);
                if (!parser_accept(p, TOK_SEP_COMMA)) {
                    break;
                }
                if (p->current.type == TOK_SEP_RPAREN && p->current.after_newline) {
                    break;
                }
            }
        }

        parser_expect(p, TOK_SEP_RPAREN, "expected ')' after function call arguments");
    } while (p->current.type == TOK_SEP_LPAREN);

    return call_node;
}

/**
 * @brief Parses an assignment `id = expr` or standalone function call `id(args...)`.
 */
static AstNode *parse_ident_led_stmt(Parser *p) {
    if (parser_peek_next(p) == TOK_SEP_LPAREN) {
        return parse_func_call(p);
    }

    int line   = (int)p->current.line;
    char *name = str_dup_cstr(&p->current.str_val);
    parser_advance(p);

    if (parser_accept(p, TOK_OP_ASSIGN)) {
        AstNode *val_expr    = parse_expression(p);
        AstNode *assign_node = ast_new_assign(name, val_expr, line);
        free(name);
        return assign_node;
    }

    free(name);
    syntax_error(p, "expected '=' for assignment or '(' for function call");
}

/**
 * @brief Dispatches statement parsing based on the current token.
 */
static AstNode *parse_statement(Parser *p) {
    switch (p->current.type) {
    case TOK_KW_VAL:
    case TOK_KW_VAR:
        return parse_var_decl(p);
    case TOK_KW_IF:
        return parse_if_stmt(p);
    case TOK_KW_WHILE:
        return parse_while_stmt(p);
    case TOK_KW_RETURN:
        return parse_return_stmt(p);
    case TOK_IDENT:
        if (parser_peek_next(p) == TOK_OP_ASSIGN || parser_peek_next(p) == TOK_SEP_LPAREN) {
            return parse_ident_led_stmt(p);
        }
        return parse_expression(p);
    default:
        return parse_expression(p);
    }
}

// Entry point

AstNode *parse_program(lexer_t *lex) {
    Parser p = {
        .lex     = lex,
        .current = lexer_next(lex),
    };

    AstNode *program = ast_new_program();
    parse_prologue(&p);
    parser_skip_newlines(&p);

    while (p.current.type != TOK_EOF) {
        if (parser_accept(&p, TOK_AT_MAIN)) {
            parser_skip_newlines(&p);
            AstNode *main_func = parse_func_def(&p, true);
            ast_list_append(&program->program.functions, main_func);
        } else if (p.current.type == TOK_KW_DEF) {
            AstNode *func = parse_func_def(&p, false);
            ast_list_append(&program->program.functions, func);
        } else {
            syntax_error(&p, "expected '@main' or 'def' function declaration at top level");
        }
        parser_skip_newlines(&p);
    }

    token_free(&p.current);
    return program;
}
