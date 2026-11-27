/**
 * @file ast.c
 * @authors Ondřej Pippal
 * @brief Implementation of AST constructors and memory management.
 *
 * Project: IFJ26 Compiler Implementation
 */

#include "ast.h"
#include "shared/error.h"
#include "shared/str.h"
#include <stdlib.h>

#define INITIAL_LIST_CAPACITY 8

/**
 * @brief Helper function to duplicate a C string using the dynamic string module.
 */
static char *ast_copy_str(const char *src) {
    if (src == nullptr) {
        return nullptr;
    }
    string_t tmp;
    str_init_from(&tmp, src);
    return str_to_owned(&tmp);
}

/**
 * @brief Allocates and zeroes out a basic AST node.
 */
static AstNode *ast_alloc_node(AstNodeType type, int line) {
    AstNode *node           = safe_calloc(1, sizeof(AstNode));
    node->type              = type;
    node->line              = line;
    node->inferred_type     = TYPE_UNKNOWN;
    node->convert_to_double = false;
    return node;
}

// Node list operations

void ast_list_init(AstNodeList *list) {
    list->items    = nullptr;
    list->count    = 0;
    list->capacity = 0;
}

void ast_list_append(AstNodeList *list, AstNode *node) {
    if (list->count >= list->capacity) {
        size_t new_cap = (list->capacity == 0) ? INITIAL_LIST_CAPACITY : list->capacity * 2;
        list->items    = safe_realloc(list->items, new_cap * sizeof(AstNode *));
        list->capacity = new_cap;
    }
    list->items[list->count++] = node;
}
static void append_list_size(size_t **sizes, size_t *count, size_t size) {
    size_t new_cnt  = *count + 1;
    *sizes          = safe_realloc(*sizes, new_cnt * sizeof(size_t));
    (*sizes)[*count] = size;
    *count          = new_cnt;
}

void ast_func_def_add_param_list(AstNode *func_node, size_t size) {
    append_list_size(&func_node->func_def.param_list_sizes,
                     &func_node->func_def.param_list_cnt,
                     size);
}

void ast_func_call_add_arg_list(AstNode *call_node, size_t size) {
    append_list_size(&call_node->func_call.arg_list_sizes,
                     &call_node->func_call.arg_list_cnt,
                     size);
}

// AST Node Constructors

AstNode *ast_new_program(void) {
    AstNode *node = ast_alloc_node(AST_PROGRAM, 0);
    ast_list_init(&node->program.functions);
    return node;
}

AstNode *
ast_new_func_def(const char *name, bool is_main, DataType return_type, AstNode *body, int line) {
    AstNode *node          = ast_alloc_node(AST_FUNC_DEF, line);
    node->func_def.name    = ast_copy_str(name);
    node->func_def.is_main = is_main;
    ast_list_init(&node->func_def.params);
    node->func_def.return_type = return_type;
    node->func_def.body        = body;
    return node;
}

AstNode *ast_new_param(const char *name, DataType data_type, int line) {
    AstNode *node         = ast_alloc_node(AST_PARAM, line);
    node->param.name      = ast_copy_str(name);
    node->param.data_type = data_type;
    node->inferred_type   = data_type;
    return node;
}

AstNode *ast_new_block(int line) {
    AstNode *node = ast_alloc_node(AST_BLOCK, line);
    ast_list_init(&node->block.statements);
    return node;
}

AstNode *
ast_new_var_decl(const char *name, bool is_var, DataType var_type, AstNode *init_expr, int line) {
    AstNode *node            = ast_alloc_node(AST_VAR_DECL, line);
    node->var_decl.name      = ast_copy_str(name);
    node->var_decl.is_var    = is_var;
    node->var_decl.var_type  = var_type;
    node->var_decl.init_expr = init_expr;
    return node;
}

AstNode *ast_new_assign(const char *target_name, AstNode *value_expr, int line) {
    AstNode *node            = ast_alloc_node(AST_ASSIGN, line);
    node->assign.target_name = ast_copy_str(target_name);
    node->assign.value_expr  = value_expr;
    return node;
}

AstNode *ast_new_if(AstNode *condition, AstNode *then_branch, AstNode *else_branch, int line) {
    AstNode *node             = ast_alloc_node(AST_IF, line);
    node->if_stmt.condition   = condition;
    node->if_stmt.then_branch = then_branch;
    node->if_stmt.else_branch = else_branch;
    return node;
}

AstNode *ast_new_while(AstNode *condition, AstNode *body, int line) {
    AstNode *node              = ast_alloc_node(AST_WHILE, line);
    node->while_stmt.condition = condition;
    node->while_stmt.body      = body;
    return node;
}

AstNode *ast_new_for(const char *iter_name,
                     AstNode *from_expr,
                     AstNode *to_expr,
                     AstNode *step_expr,
                     bool is_until,
                     AstNode *body,
                     int line) {
    AstNode *node            = ast_alloc_node(AST_FOR, line);
    node->for_stmt.iter_name = ast_copy_str(iter_name);
    node->for_stmt.from_expr = from_expr;
    node->for_stmt.to_expr   = to_expr;
    node->for_stmt.step_expr = step_expr;
    node->for_stmt.is_until  = is_until;
    ast_list_init(&node->for_stmt.guards);
    node->for_stmt.body = body;
    return node;
}

AstNode *ast_new_return(AstNode *expr, int line) {
    AstNode *node          = ast_alloc_node(AST_RETURN, line);
    node->return_stmt.expr = expr;
    return node;
}

AstNode *ast_new_func_call(const char *func_name, int line) {
    AstNode *node             = ast_alloc_node(AST_FUNC_CALL, line);
    node->func_call.func_name = ast_copy_str(func_name);
    ast_list_init(&node->func_call.args);
    return node;
}

AstNode *ast_new_bin_op(OperatorType op, AstNode *left, AstNode *right, int line) {
    AstNode *node      = ast_alloc_node(AST_BIN_OP, line);
    node->bin_op.op    = op;
    node->bin_op.left  = left;
    node->bin_op.right = right;
    return node;
}

AstNode *ast_new_un_op(OperatorType op, AstNode *operand, int line) {
    AstNode *node       = ast_alloc_node(AST_UN_OP, line);
    node->un_op.op      = op;
    node->un_op.operand = operand;
    return node;
}

AstNode *ast_new_ident(const char *name, int line) {
    AstNode *node    = ast_alloc_node(AST_IDENT, line);
    node->ident.name = ast_copy_str(name);
    return node;
}

AstNode *ast_new_lit_int(int64_t value, int line) {
    AstNode *node       = ast_alloc_node(AST_LIT_INT, line);
    node->lit_int       = value;
    node->inferred_type = TYPE_INT;
    return node;
}

AstNode *ast_new_lit_double(double value, int line) {
    AstNode *node       = ast_alloc_node(AST_LIT_DOUBLE, line);
    node->lit_double    = value;
    node->inferred_type = TYPE_DOUBLE;
    return node;
}

AstNode *ast_new_lit_string(const char *value, int line) {
    AstNode *node       = ast_alloc_node(AST_LIT_STRING, line);
    node->lit_string    = ast_copy_str(value);
    node->inferred_type = TYPE_STRING;
    return node;
}

AstNode *ast_new_lit_bool(bool value, int line) {
    AstNode *node       = ast_alloc_node(AST_LIT_BOOL, line);
    node->lit_bool      = value;
    node->inferred_type = TYPE_BOOL;
    return node;
}

AstNode *ast_new_lit_unit(int line) {
    AstNode *node       = ast_alloc_node(AST_LIT_UNIT, line);
    node->inferred_type = TYPE_UNIT;
    return node;
}

AstNode *ast_new_member_access(AstNode *object, const char *member_name, int line) {
    AstNode *node = ast_alloc_node(AST_MEMBER_ACCESS, line);
    node->member_access.object = object;
    node->member_access.member_name = ast_copy_str(member_name);
    return node;
}

// Memory Management (Recursive cleanup)

static void ast_list_free(AstNodeList *list) {
    for (size_t i = 0; i < list->count; i++) {
        ast_free(list->items[i]);
    }
    free(list->items);
    list->items    = nullptr;
    list->count    = 0;
    list->capacity = 0;
}

void ast_free(AstNode *node) {
    if (node == nullptr) {
        return;
    }

    switch (node->type) {
    case AST_PROGRAM:
        ast_list_free(&node->program.functions);
        break;

    case AST_FUNC_DEF:
        free(node->func_def.name);
        ast_list_free(&node->func_def.params);
        free(node->func_def.param_list_sizes);
        ast_free(node->func_def.body);
        break;

    case AST_PARAM:
        free(node->param.name);
        break;

    case AST_BLOCK:
        ast_list_free(&node->block.statements);
        break;

    case AST_VAR_DECL:
        free(node->var_decl.name);
        ast_free(node->var_decl.init_expr);
        break;

    case AST_ASSIGN:
        free(node->assign.target_name);
        ast_free(node->assign.value_expr);
        break;

    case AST_IF:
        ast_free(node->if_stmt.condition);
        ast_free(node->if_stmt.then_branch);
        ast_free(node->if_stmt.else_branch);
        break;

    case AST_WHILE:
        ast_free(node->while_stmt.condition);
        ast_free(node->while_stmt.body);
        break;

    case AST_FOR:
        free(node->for_stmt.iter_name);
        ast_free(node->for_stmt.from_expr);
        ast_free(node->for_stmt.to_expr);
        ast_free(node->for_stmt.step_expr);
        ast_list_free(&node->for_stmt.guards);
        ast_free(node->for_stmt.body);
        break;

    case AST_RETURN:
        ast_free(node->return_stmt.expr);
        break;

    case AST_FUNC_CALL:
        free(node->func_call.func_name);
        ast_list_free(&node->func_call.args);
        free(node->func_call.arg_list_sizes);
        break;

    case AST_BIN_OP:
        ast_free(node->bin_op.left);
        ast_free(node->bin_op.right);
        break;

    case AST_UN_OP:
        ast_free(node->un_op.operand);
        break;

    case AST_IDENT:
        free(node->ident.name);
        break;

    case AST_LIT_STRING:
        free(node->lit_string);
        break;

    case AST_MEMBER_ACCESS:
        ast_free(node->member_access.object);
        free(node->member_access.member_name);
        break;

    case AST_LIT_INT:
    case AST_LIT_DOUBLE:
    case AST_LIT_BOOL:
    case AST_LIT_UNIT:
        break;
    }

    free(node);
}
