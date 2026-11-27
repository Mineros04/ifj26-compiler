/**
 * @file test_ast.c
 * @authors Ondřej Pippal
 * @brief Unit tests of the AST module (constructors, dynamic lists, memory ownership).
 *
 * AI USE DISCLOSURE (assignment, section 4):
 * The test code in this file was generated with assistance of an AI tool and reviewed by the
 * author.
 *   Tool:   Gemini (Google)
 *   Prompt: "tak si udelam ty unit testy pro ast"
 *   Included files: "ast.h", "ast.c", "utest.h", "test_symtable.c"
 *   Tested situations (from ast.h / ast.c):
 *     - ast_free(nullptr) is a safe no-op
 *     - literal constructors set values, line numbers, inferred_type, and convert_to_double
 *     - string arguments are deep-copied (caller can overwrite or free its buffer)
 *     - nullptr string arguments are handled safely
 *     - unary and binary operator nodes store operands and operator kind
 *     - AstNodeList initializes empty and grows dynamically past INITIAL_LIST_CAPACITY
 *     - statement nodes (var_decl, assign, return with/without expr)
 *     - control-flow nodes (if with/without else, while, for with guards and optional step)
 *     - function definitions, parameters, and function calls
 *     - full nested AST_PROGRAM tree frees cleanly without leaks under ASan/Valgrind
 *
 * Build (standalone, without waiting for lexer/parser modules):
 *   clang -std=c23 -g -Wall -Wextra -Wpedantic -fsanitize=address,undefined \
 *         -Isrc -Itests tests/test_main.c tests/test_ast.c \
 *         src/ast.c src/shared/str.c src/shared/error.c -o /tmp/test_ast
 */

#include "ast.h"
#include "utest.h"

#include <string.h>

// memory safety & literals

UTEST(ast, free_null_does_nothing) {
    ast_free(nullptr);
    AstNode *n = ast_new_lit_unit(1);
    ASSERT_TRUE(n != nullptr);
    ast_free(n);
}

UTEST(ast, literals_set_values_and_inferred_types) {
    AstNode *n_int    = ast_new_lit_int(42, 10);
    AstNode *n_double = ast_new_lit_double(3.14, 11);
    AstNode *n_str    = ast_new_lit_string("hello", 12);
    AstNode *n_true   = ast_new_lit_bool(true, 13);
    AstNode *n_false  = ast_new_lit_bool(false, 14);
    AstNode *n_unit   = ast_new_lit_unit(15);

    ASSERT_EQ((int)n_int->type, (int)AST_LIT_INT);
    ASSERT_EQ(n_int->line, 10);
    ASSERT_EQ(n_int->lit_int, (int64_t)42);
    ASSERT_EQ((int)n_int->inferred_type, (int)TYPE_INT);
    ASSERT_FALSE(n_int->convert_to_double);

    ASSERT_EQ((int)n_double->type, (int)AST_LIT_DOUBLE);
    ASSERT_EQ(n_double->line, 11);
    ASSERT_NEAR(n_double->lit_double, 3.14, 1e-9);
    ASSERT_EQ((int)n_double->inferred_type, (int)TYPE_DOUBLE);
    ASSERT_FALSE(n_double->convert_to_double);

    ASSERT_EQ((int)n_str->type, (int)AST_LIT_STRING);
    ASSERT_EQ(n_str->line, 12);
    ASSERT_STREQ(n_str->lit_string, "hello");
    ASSERT_EQ((int)n_str->inferred_type, (int)TYPE_STRING);

    ASSERT_EQ((int)n_true->type, (int)AST_LIT_BOOL);
    ASSERT_TRUE(n_true->lit_bool);
    ASSERT_EQ((int)n_true->inferred_type, (int)TYPE_BOOL);

    ASSERT_EQ((int)n_false->type, (int)AST_LIT_BOOL);
    ASSERT_FALSE(n_false->lit_bool);
    ASSERT_EQ((int)n_false->inferred_type, (int)TYPE_BOOL);

    ASSERT_EQ((int)n_unit->type, (int)AST_LIT_UNIT);
    ASSERT_EQ(n_unit->line, 15);
    ASSERT_EQ((int)n_unit->inferred_type, (int)TYPE_UNIT);

    ast_free(n_int);
    ast_free(n_double);
    ast_free(n_str);
    ast_free(n_true);
    ast_free(n_false);
    ast_free(n_unit);
}

UTEST(ast, strings_are_deep_copied) {
    char buf[16] = "original";

    AstNode *ident = ast_new_ident(buf, 1);
    AstNode *lit   = ast_new_lit_string(buf, 2);
    AstNode *param = ast_new_param(buf, TYPE_INT, 3);
    AstNode *decl  = ast_new_var_decl(buf, true, TYPE_INT, nullptr, 4);
    AstNode *asgn  = ast_new_assign(buf, nullptr, 5);
    AstNode *call  = ast_new_func_call(buf, 6);
    AstNode *func  = ast_new_func_def(buf, false, TYPE_UNKNOWN, nullptr, 7);

    // Overwrite caller buffer to verify AST owns independent copies
    memset(buf, 'X', sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    ASSERT_STREQ(ident->ident.name, "original");
    ASSERT_STREQ(lit->lit_string, "original");
    ASSERT_STREQ(param->param.name, "original");
    ASSERT_STREQ(decl->var_decl.name, "original");
    ASSERT_STREQ(asgn->assign.target_name, "original");
    ASSERT_STREQ(call->func_call.func_name, "original");
    ASSERT_STREQ(func->func_def.name, "original");

    ast_free(ident);
    ast_free(lit);
    ast_free(param);
    ast_free(decl);
    ast_free(asgn);
    ast_free(call);
    ast_free(func);
}

UTEST(ast, null_strings_are_handled_safely) {
    AstNode *ident = ast_new_ident(nullptr, 1);
    AstNode *lit   = ast_new_lit_string(nullptr, 2);

    ASSERT_TRUE(ident->ident.name == nullptr);
    ASSERT_TRUE(lit->lit_string == nullptr);

    ast_free(ident);
    ast_free(lit);
}
// expressions & dynamic lists

UTEST(ast, unary_and_binary_operations) {
    AstNode *lhs  = ast_new_lit_int(10, 5);
    AstNode *rhs  = ast_new_un_op(OP_NEG, ast_new_lit_int(3, 5), 5);
    AstNode *expr = ast_new_bin_op(OP_ADD, lhs, rhs, 5);

    ASSERT_EQ((int)expr->type, (int)AST_BIN_OP);
    ASSERT_EQ((int)expr->bin_op.op, (int)OP_ADD);
    ASSERT_EQ((int)expr->inferred_type, (int)TYPE_UNKNOWN);
    ASSERT_TRUE(expr->bin_op.left == lhs);
    ASSERT_TRUE(expr->bin_op.right == rhs);

    ASSERT_EQ((int)rhs->type, (int)AST_UN_OP);
    ASSERT_EQ((int)rhs->un_op.op, (int)OP_NEG);
    ASSERT_EQ(rhs->un_op.operand->lit_int, (int64_t)3);

    ast_free(expr);
}

UTEST(ast, list_grows_dynamically_beyond_initial_capacity) {
    enum {
        ITEM_COUNT = 100
    };
    AstNode *block = ast_new_block(1);
    ASSERT_EQ(block->block.statements.count, (size_t)0);
    ASSERT_EQ(block->block.statements.capacity, (size_t)0);
    ASSERT_TRUE(block->block.statements.items == nullptr);

    for (int i = 0; i < ITEM_COUNT; i++) {
        ast_list_append(&block->block.statements, ast_new_lit_int(i, i + 1));
    }

    ASSERT_EQ(block->block.statements.count, (size_t)ITEM_COUNT);
    ASSERT_GE(block->block.statements.capacity, (size_t)ITEM_COUNT);

    for (int i = 0; i < ITEM_COUNT; i++) {
        AstNode *item = block->block.statements.items[i];
        ASSERT_EQ((int)item->type, (int)AST_LIT_INT);
        ASSERT_EQ(item->lit_int, (int64_t)i);
        ASSERT_EQ(item->line, i + 1);
    }

    ast_free(block);
}

// statements & control flow

UTEST(ast, var_decl_assign_and_return) {
    AstNode *val_decl = ast_new_var_decl("x", false, TYPE_UNKNOWN, ast_new_lit_int(1, 2), 2);
    AstNode *var_decl = ast_new_var_decl("y", true, TYPE_DOUBLE, ast_new_lit_double(2.5, 3), 3);
    AstNode *assign   = ast_new_assign("y", ast_new_lit_double(3.5, 4), 4);
    AstNode *ret_val  = ast_new_return(ast_new_ident("y", 5), 5);
    AstNode *ret_void = ast_new_return(nullptr, 6);

    ASSERT_FALSE(val_decl->var_decl.is_var);
    ASSERT_EQ((int)val_decl->var_decl.var_type, (int)TYPE_UNKNOWN);
    ASSERT_EQ(val_decl->var_decl.init_expr->lit_int, (int64_t)1);

    ASSERT_TRUE(var_decl->var_decl.is_var);
    ASSERT_EQ((int)var_decl->var_decl.var_type, (int)TYPE_DOUBLE);

    ASSERT_STREQ(assign->assign.target_name, "y");
    ASSERT_NEAR(assign->assign.value_expr->lit_double, 3.5, 1e-9);

    ASSERT_TRUE(ret_val->return_stmt.expr != nullptr);
    ASSERT_TRUE(ret_void->return_stmt.expr == nullptr);

    ast_free(val_decl);
    ast_free(var_decl);
    ast_free(assign);
    ast_free(ret_val);
    ast_free(ret_void);
}

UTEST(ast, control_flow_if_while_and_for) {
    AstNode *if_no_else = ast_new_if(ast_new_lit_bool(true, 10), ast_new_block(10), nullptr, 10);
    ASSERT_TRUE(if_no_else->if_stmt.else_branch == nullptr);

    AstNode *if_else =
        ast_new_if(ast_new_lit_bool(false, 11), ast_new_block(11), ast_new_block(12), 11);
    ASSERT_TRUE(if_else->if_stmt.else_branch != nullptr);

    AstNode *loop = ast_new_while(ast_new_lit_bool(true, 20), ast_new_block(21), 20);
    ASSERT_EQ((int)loop->type, (int)AST_WHILE);
    ASSERT_EQ((int)loop->while_stmt.body->type, (int)AST_BLOCK);

    AstNode *for_loop = ast_new_for("i",
                                    ast_new_lit_int(0, 30),
                                    ast_new_lit_int(10, 30),
                                    ast_new_lit_int(2, 30),
                                    true,
                                    ast_new_block(31),
                                    30);
    ast_list_append(&for_loop->for_stmt.guards, ast_new_lit_bool(true, 30));

    ASSERT_STREQ(for_loop->for_stmt.iter_name, "i");
    ASSERT_TRUE(for_loop->for_stmt.is_until);
    ASSERT_EQ(for_loop->for_stmt.guards.count, (size_t)1);

    ast_free(if_no_else);
    ast_free(if_else);
    ast_free(loop);
    ast_free(for_loop);
}

// functions & full program tree

UTEST(ast, full_program_builds_and_frees_cleanly) {
    AstNode *prog = ast_new_program();
    ASSERT_EQ((int)prog->type, (int)AST_PROGRAM);
    ASSERT_EQ(prog->line, 0);

    // Helper function: def add(a: Int, b: Int) = a + b
    AstNode *add_body = ast_new_block(4);
    AstNode *sum = ast_new_bin_op(OP_ADD, ast_new_ident("a", 4), ast_new_ident("b", 4), 4);
    ast_list_append(&add_body->block.statements, ast_new_return(sum, 4));

    AstNode *add_fn = ast_new_func_def("add", false, TYPE_UNKNOWN, add_body, 3);
    ast_list_append(&add_fn->func_def.params, ast_new_param("a", TYPE_INT, 3));
    ast_list_append(&add_fn->func_def.params, ast_new_param("b", TYPE_INT, 3));
    ast_list_append(&prog->program.functions, add_fn);

    // Entry function: @main def main(): Unit = print(add(1, 2))
    AstNode *call_add = ast_new_func_call("add", 7);
    ast_list_append(&call_add->func_call.args, ast_new_lit_int(1, 7));
    ast_list_append(&call_add->func_call.args, ast_new_lit_int(2, 7));

    AstNode *call_print = ast_new_func_call("print", 7);
    ast_list_append(&call_print->func_call.args, call_add);

    AstNode *main_body = ast_new_block(7);
    ast_list_append(&main_body->block.statements, call_print);

    AstNode *main_fn = ast_new_func_def("main", true, TYPE_UNIT, main_body, 6);
    ast_list_append(&prog->program.functions, main_fn);

    ASSERT_EQ(prog->program.functions.count, (size_t)2);
    ASSERT_EQ(add_fn->func_def.params.count, (size_t)2);
    ASSERT_EQ((int)add_fn->func_def.params.items[0]->inferred_type, (int)TYPE_INT);
    ASSERT_TRUE(main_fn->func_def.is_main);
    ASSERT_EQ((int)main_fn->func_def.return_type, (int)TYPE_UNIT);
    ASSERT_EQ(call_add->func_call.args.count, (size_t)2);

    // Single root free must release every node and string without leaks
    ast_free(prog);
}
