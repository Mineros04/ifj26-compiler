/**
 * @file expr.c
 * @authors Tomáš Komenda (xkoment00)
 * @brief Expression parser implementation.
 */

#include "expr.h"
#include "parser.h"

/**
 * @brief Maps a token to an operator.
 * @param token_type Type of token.
 * @param is_unary Is the operation unary?
 * @returns Type of operator.
 */
static OperatorType map_operator(token_type_t token_type, bool is_unary) {
    switch (token_type) {
    case TOK_OP_PLUS:
        return is_unary ? OP_POS : OP_ADD;
    case TOK_OP_MINUS:
        return is_unary ? OP_NEG : OP_SUB;
    case TOK_OP_STAR:
        return OP_MUL;
    case TOK_OP_SLASH:
        return OP_DIV;
    case TOK_OP_EQ:
        return OP_EQ;
    case TOK_OP_NEQ:
        return OP_NEQ;
    case TOK_OP_LT:
        return OP_LT;
    case TOK_OP_LE:
        return OP_LE;
    case TOK_OP_GT:
        return OP_GT;
    case TOK_OP_GE:
        return OP_GE;
    default:
        assert(!"map_operator: unexpected operator token");
        unreachable();
    }
}

/**
 * @brief Reduces an expression.
 * @param parser Active parser.
 * @param stack Stack to get expression from.
 */
static void reduce_expression(Parser *parser, expr_stack_t *stack) {
    expr_stack_entry_t *top_entry = expr_stack_peek(stack, 0);

    // lone operand
    if (top_entry->entry_type == EXPR_ENTRY_TERMINAL &&
        top_entry->shifted_terminal.terminal_type == EXPR_TERMINAL_OPERAND) {
        expr_stack_entry_t new_entry = expr_stack_pop(stack);

        expr_stack_push(stack, expr_create_expression(new_entry.shifted_terminal.leaf_node));
        return;
    }

    // unary operation: expression, operand
    if (top_entry->entry_type == EXPR_ENTRY_EXPRESSION && stack->count >= 2) {
        const expr_stack_entry_t *below = expr_stack_peek(stack, 1);
        if (below->entry_type == EXPR_ENTRY_TERMINAL &&
            below->shifted_terminal.terminal_type == EXPR_TERMINAL_PRIORITY_1) {

            expr_stack_entry_t left_operand = expr_stack_pop(stack);
            expr_stack_entry_t operator     = expr_stack_pop(stack);
            OperatorType op_type = map_operator(operator.shifted_terminal.token_type, true);

            AstNode *unary_node = ast_new_un_op(op_type,
                                                left_operand.expression.node,
                                                operator.shifted_terminal.line_number);
            expr_stack_push(stack, expr_create_expression(unary_node));
            return;
        }
    }

    // binary operation: operand, expression, operand
    if (top_entry->entry_type == EXPR_ENTRY_EXPRESSION && stack->count >= 3) {
        const expr_stack_entry_t *operator_entry     = expr_stack_peek(stack, 1);
        const expr_stack_entry_t *left_operand_entry = expr_stack_peek(stack, 2);

        if (operator_entry->entry_type == EXPR_ENTRY_TERMINAL &&
            operator_entry->shifted_terminal.terminal_type >= EXPR_TERMINAL_PRIORITY_2 &&
            operator_entry->shifted_terminal.terminal_type <= EXPR_TERMINAL_PRIORITY_5 &&
            left_operand_entry->entry_type == EXPR_ENTRY_EXPRESSION) {
            expr_stack_entry_t right_operand = expr_stack_pop(stack);
            expr_stack_entry_t operator      = expr_stack_pop(stack);
            expr_stack_entry_t left_operand  = expr_stack_pop(stack);
            OperatorType op_type = map_operator(operator.shifted_terminal.token_type, false);

            AstNode *binary_node = ast_new_bin_op(op_type,
                                                  left_operand.expression.node,
                                                  right_operand.expression.node,
                                                  operator.shifted_terminal.line_number);
            expr_stack_push(stack, expr_create_expression(binary_node));
            return;
        }
    }

    // parenthesis: '(', expression, ')'
    if (top_entry->entry_type == EXPR_ENTRY_TERMINAL &&
        top_entry->shifted_terminal.terminal_type == EXPR_TERMINAL_CLOSE_PAREN &&
        stack->count >= 3) {
        const expr_stack_entry_t *inner_expr_entry = expr_stack_peek(stack, 1);
        const expr_stack_entry_t *open_paren_entry = expr_stack_peek(stack, 2);

        if (inner_expr_entry->entry_type == EXPR_ENTRY_EXPRESSION &&
            open_paren_entry->entry_type == EXPR_ENTRY_TERMINAL &&
            open_paren_entry->shifted_terminal.terminal_type == EXPR_TERMINAL_OPEN_PAREN) {
            expr_stack_pop(stack); // ')'
            expr_stack_entry_t expr_entry = expr_stack_pop(stack);
            expr_stack_pop(stack); // '('

            expr_stack_push(stack, expr_entry);
            return;
        }
    }

    // unit literal
    if (top_entry->entry_type == EXPR_ENTRY_TERMINAL &&
        top_entry->shifted_terminal.terminal_type == EXPR_TERMINAL_CLOSE_PAREN &&
        stack->count >= 2) {
        const expr_stack_entry_t *open_paren_entry = expr_stack_peek(stack, 1);

        if (open_paren_entry->entry_type == EXPR_ENTRY_TERMINAL &&
            open_paren_entry->shifted_terminal.terminal_type == EXPR_TERMINAL_OPEN_PAREN) {
            expr_stack_pop(stack);                                 // ')'
            expr_stack_entry_t open_paren = expr_stack_pop(stack); // '('

            AstNode *unit_node = ast_new_lit_unit(open_paren.shifted_terminal.line_number);
            expr_stack_push(stack, expr_create_expression(unit_node));
            return;
        }
    }

    syntax_error(parser, "incomplete or invalid expression");
}

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
            reduce_expression(p, &stack);
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
                                                 (int)current_token->line);
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
