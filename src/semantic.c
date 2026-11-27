/**
 * @file semantic.c
 * @authors Pavel Bobek
 * @brief Implementation of the semantic analyzer for IFJ26.
 */

#include "semantic.h"

typedef struct {
    symtable_t *funcs;      // Table for all functions
    symtable_t *locals;     // Table for all local variables
    AstNode *program;       // Main program branch node
    symbol_t *current_func; // Pointer to the record of the current function
} SemanticContext;

static void check_stmt(SemanticContext *ctx, AstNode *node);
static DataType check_expr(SemanticContext *ctx, AstNode *node);

/**
 * @brief Searches for a function in the main program branch. Throws error if no function was found.
 *
 * @param program Pointer to the program (list of functions).
 * @param name Name of the function to find.
 *
 * @return Returns the node of the named function.
 */
static AstNode *find_func_node(const AstNode *program, const char *name) {
    for (size_t i = 0; i < program->program.functions.count; i++) {
        AstNode *node = program->program.functions.items[i];
        if (node != nullptr && node->type == AST_FUNC_DEF && strcmp(node->func_def.name, name) == 0)
            return node;
    }

    error_throw_internal("Function AST node '%s' not found.", name);
}

/**
 * @brief Checks if the DataType is a number (int or double)
 *
 * @param type Datatype to check.
 *
 * @return Bool if the type was int or double.
 */
static bool is_num(const DataType type) {
    return type == TYPE_INT || type == TYPE_DOUBLE;
}

/**
 * @brief Converts int to double if the other node is double and sets convert_to_double.
 *
 * @param left Left node of the expression.
 * @param left_type Type of the left node.
 * @param right Right node of the expression.
 * @param right_type Type of the right node.
 * @param line Number of line for error code.
 *
 * @return Retuns the resulting DataType.
 */
// TODO: check if this is the correct conversion according to the assignment (it is not) :)
static DataType convert_num_to_double(AstNode *left,
                                      const DataType left_type,
                                      AstNode *right,
                                      const DataType right_type,
                                      const int line) {
    // Check if both variables are numbers
    if (!is_num(left_type) || !is_num(right_type))
        error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                            (size_t)line,
                            0,
                            "Operator requires numeric operands.");

    // Convert right int to double if left is double
    if (left_type == TYPE_DOUBLE && right_type == TYPE_INT) {
        right->convert_to_double = true;
        return TYPE_DOUBLE;
    }

    // Convert left int to double if right is double
    if (left_type == TYPE_INT && right_type == TYPE_DOUBLE) {
        left->convert_to_double = true;
        return TYPE_DOUBLE;
    }

    return left_type;
}

/**
 * @brief Pushes the scope of local variables. Throws error if Symtable runs out of memory.
 *
 * @param ctx Context of the semantic.
 */
static void push_scope(const SemanticContext *ctx) {
    if (symtable_push_scope(ctx->locals) == SYMTABLE_OUT_OF_MEM)
        error_throw_internal("Symtable ran out of memory.");
}

/**
 * @brief Pops the scope of local variables.
 *
 * @param ctx Context of the semantic.
 */
static void pop_scope(const SemanticContext *ctx) {
    symtable_pop_scope(ctx->locals);
}

/**
 * @brief Creates a record of the variable in symtable.
 *
 * @param ctx Context of the semantic.
 * @param name Name of the variable.
 * @param type DataType of the variable.
 * @param is_mutable Bool if the variable is mutable.
 * @param line Line number for error throwing.
 */
static void declare_var(const SemanticContext *ctx,
                        const char *name,
                        const DataType type,
                        const bool is_mutable,
                        const size_t line) {
    // Create a variable descriptor
    const var_desc_t desc = {type, 0, is_mutable};

    // Insert the variable descriptor into the symtable
    const symtable_status_t status =
        symtable_insert_var(ctx->locals, name, desc, (int)line, nullptr);

    // Throw a correct error
    if (status == SYMTABLE_ITEM_EXISTS)
        error_throw_compile(ERR_SEM_REDEF,
                            line,
                            0,
                            "variable '%s' is already declared in this scope.",
                            name);
    if (status == SYMTABLE_OUT_OF_MEM)
        error_throw_internal("Symbol table ran out of memory.");
}

/**
 * @brief Checks if a function has a return in all logical branches.
 *
 * @param node Get the node of a function.
 *
 * @return Boolean if a function has return keyword in all logical branches.
 */
static bool check_return(const AstNode *node) {
    // Skip empty node
    if (node == nullptr)
        return false;

    switch (node->type) {
    case AST_RETURN:
        return true;

    // Check if there is atleast 1 return in the whole block
    case AST_BLOCK: {
        for (size_t i = 0; i < node->block.statements.count; i++) {
            if (check_return(node->block.statements.items[i]))
                return true;
        }
        return false;
    }

    // Check if both if branches have return
    case AST_IF:
        return node->if_stmt.else_branch != nullptr && check_return(node->if_stmt.then_branch) &&
               check_return(node->if_stmt.else_branch);

    default:
        return false;
    }
}

/**
 * @brief Analyzes a function. Checks if a function has a return type. Checks for correct usage of
 * variables, their scopes and types.
 *
 * @param ctx Context of the semantic.
 * @param node Node of a function.
 */
static void analyze_func(SemanticContext *ctx, AstNode *node) {
    // Find the function to analyze
    symbol_t *func_symbol = symtable_find_func(ctx->funcs, node->func_def.name);
    if (func_symbol == nullptr)
        error_throw_internal("Function '%s' is missing from symbol table.", node->func_def.name);

    // Skip processed function
    func_desc_t *fn = &func_symbol->data.function;
    if (!fn->return_explicit && fn->inference_state == RETURN_FINALIZED)
        return;

    // Function does not have an explicit return type and resursivly call itself
    if (fn->inference_state == RETURN_ANALYSING)
        error_throw_compile(ERR_SEM_UNDEF,
                            (size_t)node->line,
                            0,
                            "Cannot infer return type of recursive function '%s'.",
                            node->func_def.name);

    // Set the function inference stat to RETURN_ANALYSING
    if (!fn->return_explicit)
        fn->inference_state = RETURN_ANALYSING;

    // Set the prev function and prev local variables
    symbol_t *prev_func     = ctx->current_func;
    symtable_t *prev_locals = ctx->locals;

    // Set the new function and variables into context
    ctx->current_func = func_symbol;
    ctx->locals       = symtable_create();
    if (ctx->locals == nullptr)
        error_throw_internal("Failed to allocate local symbol table.");

    // Push a new scope
    push_scope(ctx);

    // Declare all parameters
    for (size_t i = 0; i < node->func_def.params.count; i++) {
        const AstNode *param = node->func_def.params.items[i];
        declare_var(ctx, param->param.name, param->param.data_type, false, param->line);
    }

    if (node->func_def.body != nullptr) {
        // Check the body of a function
        if (node->func_def.body->type == AST_BLOCK)
            check_stmt(ctx, node->func_def.body);

        // Handle the special definition of a function where it does not have a body but an
        // expression instead
        else {
            // Get the DataType and check it
            const DataType body_type = check_expr(ctx, node->func_def.body);
            if (fn->return_explicit) {
                if (fn->return_type == TYPE_DOUBLE && body_type == TYPE_INT)
                    node->func_def.body->convert_to_double = true;
                else if (body_type != fn->return_type)
                    error_throw_compile(ERR_SEM_OTHER,
                                        (size_t)node->line,
                                        0,
                                        "Function body type does not match declared return type.");
            } else
                fn->return_type = body_type;
        }
    }

    // Set return type to UNIT if needed, finalize the inference state of the function and set the
    // node return_type
    if (!fn->return_explicit) {
        if (fn->return_type == TYPE_UNKNOWN)
            fn->return_type = TYPE_UNIT;

        fn->inference_state        = RETURN_FINALIZED;
        node->func_def.return_type = fn->return_type;
    }

    // Check if the function has a return type
    if (fn->return_type != TYPE_UNIT && node->func_def.body != nullptr &&
        node->func_def.body->type == AST_BLOCK && !check_return(node->func_def.body))
        error_throw_compile(ERR_SEM_OTHER,
                            (size_t)node->line,
                            0,
                            "Function '%s' does not have a return value.",
                            node->func_def.name);

    node->inferred_type = fn->return_type;

    // Free all allocated memory and set the prev function and locals
    symtable_destroy(ctx->locals);
    ctx->locals       = prev_locals;
    ctx->current_func = prev_func;
}

/**
 * @brief Recursivly checks a body. Checks all variables if they have correct DataType and exist in
 * their scopes.
 *
 * @param ctx Context of the semantic.
 * @param node Node of a body.
 */
static void check_stmt(SemanticContext *ctx, AstNode *node) {
    // Skip empty body
    if (node == nullptr)
        return;

    switch (node->type) {
    case AST_BLOCK: {
        // Check the whole block in a new scope
        push_scope(ctx);
        for (size_t i = 0; i < node->block.statements.count; i++) {
            check_stmt(ctx, node->block.statements.items[i]);
        }
        pop_scope(ctx);
        break;
    }

    case AST_VAR_DECL: {
        // Get the type of the declared variable
        const DataType declared_type = node->var_decl.var_type;
        DataType final_type          = declared_type;

        // Check the initialize expression type
        if (node->var_decl.init_expr != nullptr) {
            // Get the initialize expression type
            const DataType init_type = check_expr(ctx, node->var_decl.init_expr);

            // Set the final DataType and check for int to double conversion
            if (declared_type == TYPE_UNKNOWN) {
                if (init_type == TYPE_UNIT)
                    error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                                        (size_t)node->line,
                                        0,
                                        "Cannot infer variable type from Unit expression.");
                final_type = init_type;
            } else if (declared_type == TYPE_DOUBLE && init_type == TYPE_INT) {
                node->var_decl.init_expr->convert_to_double = true;
            } else if (declared_type != init_type) {
                error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                                    (size_t)node->line,
                                    0,
                                    "Incompatible initializer type for variable '%s'.",
                                    node->var_decl.name);
            }
        } else if (declared_type == TYPE_UNKNOWN) {
            error_throw_compile(
                ERR_SEM_TYPE_MISMATCH,
                (size_t)node->line,
                0,
                "Variable '%s' must have either an explicit type or an initializer.",
                node->var_decl.name);
        }

        // Declare the variable with its final type
        declare_var(ctx, node->var_decl.name, final_type, node->var_decl.is_var, node->line);
        break;
    }

    case AST_ASSIGN: {
        // Get the symbol of the variable to be assigned and check if it exists
        const symbol_t *symbol = symtable_find_var(ctx->locals, node->assign.target_name);
        if (symbol == nullptr)
            error_throw_compile(ERR_SEM_UNDEF,
                                (size_t)node->line,
                                0,
                                "Undefined variable '%s'.",
                                node->assign.target_name);

        // Check if the variable can be changed
        if (!symbol->data.variable.is_mutable)
            error_throw_compile(ERR_SEM_WRITE_IMMUT,
                                (size_t)node->line,
                                0,
                                "Cannot assign to immutable variable '%s'.",
                                node->assign.target_name);

        // Get the DataType for the variable and the value
        const DataType val_type = check_expr(ctx, node->assign.value_expr);
        const DataType var_type = symbol->data.variable.data_type;

        // Set the conversion and check if value and variable DataType matches
        if (var_type == TYPE_DOUBLE && val_type == TYPE_INT)
            node->assign.value_expr->convert_to_double = true;
        else if (var_type != val_type)
            error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                                (size_t)node->line,
                                0,
                                "Cannot assign incompatible type to variable '%s'.",
                                node->assign.target_name);

        break;
    }

    case AST_IF: {
        // Check if the if condition has resulting type of bool
        if (check_expr(ctx, node->if_stmt.condition) != TYPE_BOOL)
            error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                                (size_t)node->if_stmt.condition->line,
                                0,
                                "If condition must be of type Bool.");

        // Check the body of the then branch
        check_stmt(ctx, node->if_stmt.then_branch);

        // Check the body of the else branch if it exists
        if (node->if_stmt.else_branch != nullptr)
            check_stmt(ctx, node->if_stmt.else_branch);
        break;
    }

    case AST_WHILE: {
        // Check if the while condition has resulting type of bool
        if (check_expr(ctx, node->while_stmt.condition) != TYPE_BOOL)
            error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                                (size_t)node->while_stmt.condition->line,
                                0,
                                "While condition must be of type Bool.");

        // Check the body of the while loop
        check_stmt(ctx, node->while_stmt.body);
        break;
    }

    case AST_FOR: {
        // Check if the bounds are type of INT
        if (check_expr(ctx, node->for_stmt.from_expr) != TYPE_INT)
            error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                                (size_t)node->for_stmt.from_expr->line,
                                0,
                                "For loop start expression must be of type Int.");
        if (check_expr(ctx, node->for_stmt.to_expr) != TYPE_INT)
            error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                                (size_t)node->for_stmt.to_expr->line,
                                0,
                                "For loop end expression must be of type Int.");

        // Check the step expression
        if (node->for_stmt.step_expr != nullptr &&
            check_expr(ctx, node->for_stmt.step_expr) != TYPE_INT)
            error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                                (size_t)node->for_stmt.step_expr->line,
                                0,
                                "For loop step expression must be of type Int.");

        // Create new scope and iterator variable
        push_scope(ctx);
        declare_var(ctx, node->for_stmt.iter_name, TYPE_INT, false, (size_t)node->line);

        // Check that all guards are type of bool
        for (size_t i = 0; i < node->for_stmt.guards.count; i++) {
            AstNode *guard = node->for_stmt.guards.items[i];
            if (check_expr(ctx, guard) != TYPE_BOOL)
                error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                                    (size_t)guard->line,
                                    0,
                                    "For loop guard must be of type Bool.");
        }

        // Check the body of the for loop
        check_stmt(ctx, node->for_stmt.body);

        pop_scope(ctx);
        break;
    }

    case AST_RETURN: {
        // Get the function descriptor and type of the function
        func_desc_t *fn = &ctx->current_func->data.function;
        const DataType expr_type =
            node->return_stmt.expr != nullptr ? check_expr(ctx, node->return_stmt.expr) : TYPE_UNIT;

        // Set the conversion
        if (fn->return_explicit) {
            if (fn->return_type == TYPE_DOUBLE && expr_type == TYPE_INT)
                node->return_stmt.expr->convert_to_double = true;
            else if (expr_type != fn->return_type)
                error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                                    (size_t)node->line,
                                    0,
                                    "Return expression type does not match function return type.");
        }
        // Set the conversion and return type
        else {
            if (fn->return_type == TYPE_UNKNOWN)
                fn->return_type = expr_type;
            else if (fn->return_type == TYPE_DOUBLE && expr_type == TYPE_INT)
                node->return_stmt.expr->convert_to_double = true;
            else if (expr_type != fn->return_type)
                error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                                    (size_t)node->line,
                                    0,
                                    "Inconsistent return types in function '%s'.",
                                    ctx->current_func->identifier);
        }
        break;
    }

    case AST_FUNC_CALL:
    default: {
        check_expr(ctx, node);
        break;
    }
    }
}

/**
 * @brief Recursivly checks an expression. Checks all variables if they have correct DataType and
 * exist in their scopes.
 *
 * @param ctx Context of the semantic.
 * @param node Node of a body.
 *
 * @return Returns the final DataType of the expression.
 */
static DataType check_expr(SemanticContext *ctx, AstNode *node) {
    // Skip empty node with returning UNIT (void) type
    if (node == nullptr)
        return TYPE_UNIT;

    DataType result = TYPE_UNKNOWN;

    // Get the resulting type of the expression
    switch (node->type) {
    case AST_LIT_INT: {
        result = TYPE_INT;
        break;
    }

    case AST_LIT_DOUBLE: {
        result = TYPE_DOUBLE;
        break;
    }

    case AST_LIT_STRING: {
        result = TYPE_STRING;
        break;
    }

    case AST_LIT_BOOL: {
        result = TYPE_BOOL;
        break;
    }

    case AST_LIT_UNIT: {
        result = TYPE_UNIT;
        break;
    }

    case AST_IDENT: {
        // Find the variable and check if it exists
        const symbol_t *symbol = symtable_find_var(ctx->locals, node->ident.name);
        if (symbol == nullptr)
            error_throw_compile(ERR_SEM_UNDEF,
                                (size_t)node->line,
                                0,
                                "Undefined variable '%s'",
                                node->ident.name);

        // Set the result to the symbol variable DataType
        result = symbol->data.variable.data_type;
        break;
    }

    case AST_UN_OP: {
        // Check the inner expression and get its DataType
        const DataType operation_type = check_expr(ctx, node->un_op.operand);

        switch (node->un_op.op) {
        case OP_POS:
        case OP_NEG: {
            // Check if OP_POS and OP_NEG are used only on numbers
            if (!is_num(operation_type))
                error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                                    (size_t)node->line,
                                    0,
                                    "Unary +/- requires a numeric operand.");
            result = operation_type;
            break;
        }

        case OP_NOT: {
            // Check if not is used only on bool
            if (operation_type != TYPE_BOOL)
                error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                                    (size_t)node->line,
                                    0,
                                    "Unary '!' requires a bool operand.");
            result = TYPE_BOOL;
            break;
        }

        default:
            error_throw_internal("Invalid unary operator in AST_UN_OP.");
        }
        break;
    }

    case AST_BIN_OP: {
        // Get DataTypes for both sides of the expression
        const DataType left_type  = check_expr(ctx, node->bin_op.left);
        const DataType right_type = check_expr(ctx, node->bin_op.right);

        switch (node->bin_op.op) {
        case OP_ADD: {
            // Add can be used a concatanation
            if (left_type == TYPE_STRING && right_type == TYPE_STRING)
                result = TYPE_STRING;
            else
                result = convert_num_to_double(node->bin_op.left,
                                               left_type,
                                               node->bin_op.right,
                                               right_type,
                                               node->line);
            break;
        }

        case OP_SUB:
        case OP_MUL:
        case OP_DIV: {
            result = convert_num_to_double(node->bin_op.left,
                                           left_type,
                                           node->bin_op.right,
                                           right_type,
                                           node->line);
            break;
        }

        case OP_LT:
        case OP_LE:
        case OP_GT:
        case OP_GE: {
            if (left_type == TYPE_STRING && right_type == TYPE_STRING)
                result = TYPE_BOOL;
            else {
                convert_num_to_double(node->bin_op.left,
                                      left_type,
                                      node->bin_op.right,
                                      right_type,
                                      node->line);
                result = TYPE_BOOL;
            }
            break;
        }

        case OP_EQ:
        case OP_NEQ: {
            if (is_num(left_type) && is_num(right_type))
                convert_num_to_double(node->bin_op.left,
                                      left_type,
                                      node->bin_op.right,
                                      right_type,
                                      node->line);
            else if (left_type != right_type)
                error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                                    (size_t)node->line,
                                    0,
                                    "cannot compare incompatible types");

            result = TYPE_BOOL;
            break;
        }

        case OP_AND:
        case OP_OR: {
            if (left_type != TYPE_BOOL || right_type != TYPE_BOOL)
                error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                                    (size_t)node->line,
                                    0,
                                    "Logical operator requires Bool operands.");

            result = TYPE_BOOL;
            break;
        }

        default:
            error_throw_internal("Invalid binary operator in AST_BIN_OP.");
        }
        break;
    }

    case AST_FUNC_CALL: {
        // Get the symbol of the function and check if it exists
        const symbol_t *symbol = symtable_find_func(ctx->funcs, node->func_call.func_name);
        if (symbol == nullptr)
            error_throw_compile(ERR_SEM_UNDEF,
                                (size_t)node->line,
                                0,
                                "Undefined function '%s'.",
                                node->func_call.func_name);

        const func_desc_t *fn = &symbol->data.function;

        // Check if we have the correct number of arguments for the function call
        if (node->func_call.args.count != fn->parameter_cnt)
            error_throw_compile(ERR_SEM_INVALID_ARG,
                                (size_t)node->line,
                                0,
                                "Function '%s' expects %zu argument(s) instead got %zu.",
                                node->func_call.func_name,
                                fn->parameter_cnt,
                                node->func_call.args.count);

        // Check argument count inside each parameter list
        for (size_t i = 0; i < fn->param_list_cnt; i++)
        {
            if (node->func_call.arg_list_sizes[i] != fn->param_list_sizes[i])
                error_throw_compile(ERR_SEM_INVALID_ARG, (size_t)node->line, 0,
                                    "Parameter list %zu of '%s' expects %zu argument(s), got %zu.",
                                    i + 1,
                                    node->func_call.func_name,
                                    fn->param_list_sizes[i],
                                    node->func_call.arg_list_sizes[i]);
        }

        // Defensive total argument count check
        if (node->func_call.args.count != fn->parameter_cnt)
            error_throw_compile(ERR_SEM_INVALID_ARG, (size_t)node->line, 0,
                                "Function '%s' expects %zu argument(s), got %zu.",
                                node->func_call.func_name,
                                fn->parameter_cnt,
                                node->func_call.args.count);

        // Check all arguments and their DataTypes if they match with parameters DataTypes
        for (size_t i = 0; i < node->func_call.args.count; i++) {
            AstNode *argument           = node->func_call.args.items[i];
            const DataType arg_type     = check_expr(ctx, argument);
            const param_desc_t expected = fn->parameters[i];

            if (expected.accepts_any)
                continue;

            if (expected.data_type == TYPE_DOUBLE && arg_type == TYPE_INT)
                argument->convert_to_double = true;
            else if (expected.data_type != arg_type)
                error_throw_compile(ERR_SEM_INVALID_ARG,
                                    (size_t)argument->line,
                                    0,
                                    "Argument %zu of '%s' has incompatible type.",
                                    i + 1,
                                    node->func_call.func_name);
        }

        // Analyze function that has not yet been analyzed
        if (fn->inference_state == RETURN_UNRESOLVED) {
            AstNode *analyze_node = find_func_node(ctx->program, node->func_call.func_name);
            analyze_func(ctx, analyze_node);
        } else if (fn->inference_state == RETURN_ANALYSING) {
            error_throw_compile(ERR_SEM_OTHER,
                                (size_t)node->line,
                                0,
                                "Cannot infer return type of recursive function '%s'.",
                                node->func_call.func_name);
        }

        result = symbol->data.function.return_type;
        break;
    }

    case AST_IF: {
        // Check if the ternary expression has the type bool in its first part
        if (check_expr(ctx, node->if_stmt.condition) != TYPE_BOOL)
            error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                                (size_t)node->if_stmt.condition->line,
                                0,
                                "Ternary condition must be of type Bool.");

        // Ternary expression needs to have an else branch
        if (node->if_stmt.else_branch == nullptr)
            error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                                (size_t)node->line,
                                0,
                                "Ternary expression must have an else branch.");

        // Get both types of then and else expression
        const DataType then_type = check_expr(ctx, node->if_stmt.then_branch);
        const DataType else_type = check_expr(ctx, node->if_stmt.else_branch);

        // Check if they match and set the conversion to double
        if (then_type == else_type)
            result = then_type;
        else if (is_num(then_type) && is_num(else_type))
            result = convert_num_to_double(node->if_stmt.then_branch,
                                           then_type,
                                           node->if_stmt.else_branch,
                                           else_type,
                                           node->line);
        else
            error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                                (size_t)node->line,
                                0,
                                "Branches of ternary expression have incompatible types.");
        break;
    }

    case AST_MEMBER_ACCESS:
    {
        if (node->member_access.object == nullptr || node->member_access.member_name == nullptr)
            error_throw_internal("Invalid AST_MEMBER_ACCESS node.");

        const DataType obj_type = check_expr(ctx, node->member_access.object);
        const char *member      = node->member_access.member_name;

        if (strcmp(member, "toStr") == 0)
        {
            result = TYPE_STRING;
        }
        else if (obj_type == TYPE_STRING && strcmp(member, "length") == 0)
        {
            result = TYPE_INT;
        }
        else if (obj_type == TYPE_DOUBLE && strcmp(member, "toInt") == 0)
        {
            result = TYPE_INT;
        }
        else if (obj_type == TYPE_INT && strcmp(member, "toDouble") == 0)
        {
            result = TYPE_DOUBLE;
        }
        else if (obj_type == TYPE_INT && strcmp(member, "chr") == 0)
        {
            result = TYPE_STRING;
        }
        else
        {
            // Verify required exit code for unknown member in section 5.4
            error_throw_compile(ERR_SEM_UNDEF,
                                (size_t)node->line,
                                0,
                                "Unknown member '%s' for this expression type.",
                                member);
        }
        break;
    }

    case AST_BLOCK: {
        // Create a new scope and check all the statements in the block
        push_scope(ctx);
        const size_t count = node->block.statements.count;
        if (count == 0)
            result = TYPE_UNIT;
        else {
            for (size_t i = 0; i < count - 1; i++) {
                check_stmt(ctx, node->block.statements.items[i]);
            }
            result = check_expr(ctx, node->block.statements.items[count - 1]);
        }
        pop_scope(ctx);
        break;
    }

    default:
        error_throw_internal("Unexpected AST node type in check_expr().");
    }

    node->inferred_type = result;

    return result;
}

void semantic_analyzer(AstNode *program) {
    // Return if there is no program to analyze
    if (program == nullptr || program->type != AST_PROGRAM)
        return;

    // Create a symtable
    symtable_t *funcs = symtable_create();
    if (funcs == nullptr)
        error_throw_internal("Failed to allocate symtable.");

    // Create a semantic context
    SemanticContext ctx = {funcs, nullptr, program, nullptr};

    // Register all builtints into the symtable
    symtable_status_t symtable_status = symtable_register_builtins(ctx.funcs);
    if (symtable_status == SYMTABLE_ITEM_EXISTS)
        error_throw_internal("Failed to register builtins");
    if (symtable_status == SYMTABLE_OUT_OF_MEM)
        error_throw_internal("Symtable ran out of memory.");

    // Loop through all functions and register them
    for (size_t i = 0; i < program->program.functions.count; i++) {
        const AstNode *node = program->program.functions.items[i];
        if (node == nullptr || node->type != AST_FUNC_DEF)
            error_throw_internal("Program node is not a function node.");

        const size_t param_cnt = node->func_def.params.count;
        param_desc_t *params = param_cnt > 0 ? safe_malloc(sizeof(param_desc_t) * param_cnt) : nullptr;

        // Loop through all parameters and register them
        for (size_t j = 0; j < node->func_def.params.count; j++) {
            const param_desc_t param = {node->func_def.params.items[j]->param.data_type, false};
            params[j]                = param;
        }

        // Create a function descriptor
        const bool is_explicit = node->func_def.return_type != TYPE_UNKNOWN;
        const func_desc_t func_desc = {
            .parameter_cnt    = param_cnt,
            .parameters       = params,
            .param_list_cnt   = node->func_def.param_list_cnt,
            .param_list_sizes = node->func_def.param_list_sizes,
            .is_main          = node->func_def.is_main,
            .is_builtin       = false,
            .return_type      = node->func_def.return_type,
            .return_explicit  = is_explicit,
            .inference_state  = is_explicit ? RETURN_FINALIZED : RETURN_UNRESOLVED,
        };
        symtable_status =
            symtable_insert_func(ctx.funcs, node->func_def.name, func_desc, node->line);

        free(params);

        if (symtable_status == SYMTABLE_ITEM_EXISTS)
            error_throw_compile(ERR_SEM_REDEF,
                                node->line,
                                0,
                                "function '%s' is already defined",
                                node->func_def.name);
        if (symtable_status == SYMTABLE_OUT_OF_MEM)
            error_throw_internal("Symtable ran out of memory.");
    }

    // Check if we have a main function with the correct return type
    const symbol_t *main_symbol = symtable_find_func(ctx.funcs, "main");
    if (main_symbol == nullptr)
        error_throw_compile(ERR_SEM_UNDEF, 0, 0, "Missing 'main' function.");
    if (main_symbol->data.function.parameter_cnt != 0)
        error_throw_compile(ERR_SEM_INVALID_ARG,
                            (size_t)main_symbol->line_number,
                            0,
                            "'main' function must not have any parameters.");
    if (main_symbol->data.function.return_explicit &&
        main_symbol->data.function.return_type != TYPE_INT)
        error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                            (size_t)main_symbol->line_number,
                            0,
                            "'main' function must have return type of int.");

    // Loop through all functions and analyze them
    for (size_t i = 0; i < program->program.functions.count; i++) {
        AstNode *node = program->program.functions.items[i];
        if (node != nullptr && node->type == AST_FUNC_DEF)
            analyze_func(&ctx, node);
    }

    // Check the return type of the main function once more
    main_symbol = symtable_find_func(ctx.funcs, "main");
    if (main_symbol->data.function.return_type != TYPE_INT)
        error_throw_compile(ERR_SEM_TYPE_MISMATCH,
                            (size_t)main_symbol->line_number,
                            0,
                            "'main' function must have return type of int.");

    // Free all allocated memory
    symtable_destroy(funcs);
}
