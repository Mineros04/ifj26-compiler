/**
 * @file expr.c
 * @authors Tomáš Komenda (xkoment00)
 * @brief Expression parser implementation.
 */

#include "expr.h"
#include "parser.h"

// static void reduce_expression(Parser *parser, expr_stack_t *stack) {
// }

AstNode *parse_expression(Parser *p) {
    // allocate and initialize stack
    expr_stack_t stack;
    expr_stack_init(&stack);
    expr_stack_push(&stack, expr_create_terminal(TOK_EOF, EXPR_TERMINAL_END_EXPRESSION, 0));

    // setup parenthesis counter & terminal
    expr_terminal_t prev_terminal = EXPR_TERMINAL_END_EXPRESSION;
    int open_paren_cntr           = 0;

    while (true) {
        expr_terminal_t current_terminal = expr_classify_token(parser_current(p)->type);
        if (current_terminal == EXPR_TERMINAL_CLOSE_PAREN && !open_paren_cntr) {
            current_terminal = EXPR_TERMINAL_END_EXPRESSION;
        }
        current_terminal = expr_resolve_terminal(current_terminal, prev_terminal);

        expr_relation_t current_relation =
            expr_lookup_relation(expr_stack_top_terminal(&stack), current_terminal);

        switch (current_relation) {
        case EXPR_RELATION_ACCEPT:
            if (stack.count == 2 &&
                expr_stack_peek(&stack, 0)->entry_type == EXPR_ENTRY_EXPRESSION) {
                AstNode *to_return = expr_stack_pop(&stack).expression.node;
                expr_stack_free(&stack);
                return to_return;
            } else {
                syntax_error(p, "expected an expression");
            }
        case EXPR_RELATION_EQUAL:
            expr_stack_push(&stack,
                            expr_create_terminal(parser_current(p)->type,
                                                 current_terminal,
                                                 (int)parser_current(p)->line));
            parser_advance(p);
            open_paren_cntr--;
            prev_terminal = current_terminal;
            break;
        case EXPR_RELATION_ERROR:
            syntax_error(p, "unexpected token in expression");
        case EXPR_RELATION_REDUCE:
            /* code */
            break;
        case EXPR_RELATION_SHIFT: {
            const token_t *current_token = parser_current(p);
            expr_stack_entry_t new_entry;
            bool consumed_call = false;

            if (current_terminal == EXPR_TERMINAL_OPERAND) {
                AstNode *node = nullptr;
                if (current_token->type == TOK_IDENT && parser_peek_next(p) == TOK_SEP_LPAREN) {
                    node          = parse_func_call(p);
                    consumed_call = true;
                } else {
                    node = expr_create_leaf(current_token);
                }
                new_entry =
                    expr_create_operand(current_token->type, node, (int)current_token->line);
            } else {
                new_entry = expr_create_terminal(current_token->type,
                                                 current_terminal,
                                                 current_token->line);
            }

            if (current_terminal == EXPR_TERMINAL_OPEN_PAREN) {
                open_paren_cntr++;
            }

            expr_stack_push(&stack, new_entry);
            if (!consumed_call) {
                parser_advance(p);
            }
            prev_terminal = current_terminal;
            break;
        }
        }
    }
}
