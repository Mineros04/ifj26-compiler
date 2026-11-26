/**
* @file codegen.c
* @authors Pavel Bobek
* @brief Implementation of the code generator for IFJ26.
*/

#include "codegen.h"

typedef struct
{
    const char* src_name;   // Original identifier in AST
    size_t unique_id;       // Unique suffix inside current function
    int scope_depth;        // Block nesting level
} VarBinding;

typedef struct
{
    char* data;
    size_t len;
    size_t cap;
} StrBuf;

typedef struct
{
    FILE* out;
    size_t label_counter;
    size_t var_counter;
    int scope_depth;

    const char* current_func;

    StrBuf decl_buffer;
    StrBuf body_buffer;

    // Active variable bindings in the current function
    VarBinding* vars;
    size_t var_count;
    size_t var_cap;
} CodeGenContext;

static void gen_stmt(CodeGenContext* ctx, const AstNode* node);
static void gen_expr(CodeGenContext* ctx, const AstNode* node);

static void sb_init(StrBuf* sb)
{
    sb->cap = 1024;
    sb->len = 0;
    sb->data = safe_malloc(sb->cap);
    sb->data[0] = '\0';
}

static void sb_free(StrBuf* sb)
{
    free(sb->data);
    sb->data = nullptr;
    sb->len = 0;
    sb->cap = 0;
}

static void sb_append_char(StrBuf* sb, const char c)
{
    if (sb->len + 2 > sb->cap)
    {
        sb->cap = sb->cap == 0 ? 1024 : sb->cap * 2;
        sb->data = safe_realloc(sb->data, sb->cap);
    }
    sb->data[sb->len] = c;
    sb->len++;
    sb->data[sb->len] = '\0';
}

static void sb_vappendf(StrBuf* sb, const char* fmt, va_list args)
{
    va_list argsCopy;
    va_copy(argsCopy, args);
    const int needed = vsnprintf(nullptr, 0, fmt, argsCopy);
    va_end(argsCopy);

    if (needed <= 0)
        return;

    const size_t required = sb->len + (size_t) needed + 1;
    if (required > sb->cap)
    {
        while (sb->cap < required)
        {
            sb->cap = sb->cap == 0 ? 1024 : sb->cap * 2;
        }
        sb->data = safe_realloc(sb->data, sb->cap);
    }

    vsnprintf(sb->data + sb->len, sb->cap - sb->len, fmt, args);
    sb->len += (size_t) needed;
}

static void emit_decl(CodeGenContext* ctx, const char* fmt, ...)
{
    va_list args = {};
    va_start(args, fmt);
    sb_vappendf(&ctx->decl_buffer, fmt, args);
    va_end(args);
}

static void emit_body(CodeGenContext* ctx, const char* fmt, ...)
{
    va_list args = {};
    va_start(args, fmt);
    sb_vappendf(&ctx->body_buffer, fmt, args);
    va_end(args);
}

/*
static const char* convertDataTypeToString(const DataType dataType) {
    switch (dataType){
        case TYPE_UNKNOWN:
            return "UNKNOWN";
        case TYPE_INT:
            return "INT";
        case TYPE_DOUBLE:
            return "DOUBLE";
        case TYPE_STRING:
            return "STRING";
        case TYPE_UNIT:
            return "UNIT";
        case TYPE_BOOL:
            return "BOOL";
    }
    return "";
}
*/


/**
 * @brief Emits an IFJcode26 formatted string literal with the "string@" prefix.
 *
 * Escapes control characters, whitespace (ASCII 0-32), '#' (35), '\' (92),
 * and non-ASCII bytes (>= 127) using the \xyz decimal escape format.
 *
 * @param ctx Context for code generator
 * @param src Null-terminated C string to format (emits empty string if NULL).
 */
static void emit_string(CodeGenContext* ctx, const char* src)
{
    emit_body(ctx, "string@");
    if (!src)
        return;

    for (auto p = (const unsigned char *)src; *p != '\0'; p++)
    {
        const unsigned char c = *p;

        if (c <= 32 || c == 35 || c == 92 || c >= 127)
            emit_body(ctx, "\\%03u", (unsigned int) c);
        else
            sb_append_char(&ctx->body_buffer, (char) c);
    }
}

/**
 * @brief Enters a new lexical block scope.
 *
 * Increments the current scope nesting depth in the code generation context.
 *
 * @param ctx Pointer to the code generator context.
 */
static void scope_enter(CodeGenContext* ctx)
{
    ctx->scope_depth++;
}

/**
 * @brief Leaves the current lexical block scope.
 *
 * Removes all local variable bindings declared at the current nesting depth
 * and decrements the scope depth counter.
 *
 * @param ctx Pointer to the code generator context.
 */
static void scope_leave(CodeGenContext* ctx)
{
    while (ctx->var_count > 0 && ctx->vars[ctx->var_count - 1].scope_depth == ctx->scope_depth)
    {
        ctx->var_count--;
    }
    ctx->scope_depth--;
}

/**
 * @brief Registers a new local variable in the current scope and emits its DEFVAR instruction.
 *
 * Assigns a function-unique numeric suffix to prevent name collisions across
 * shadowed scopes and grows the internal variable binding array if needed.
 *
 * @param ctx Pointer to the code generator context.
 * @param name Source identifier name of the variable.
 * @return Unique numeric suffix assigned to the variable.
 */
static size_t declare_local_var(CodeGenContext* ctx, const char* name)
{
    if (ctx->var_count == ctx->var_cap)
    {
        ctx->var_cap = ctx->var_cap == 0 ? 16 : ctx->var_cap * 2;
        ctx->vars = safe_realloc(ctx->vars, ctx->var_cap * sizeof(VarBinding));
    }

    const size_t id = ++ctx->var_counter;
    ctx->vars[ctx->var_count++] = (VarBinding) {
        .src_name = name,
        .unique_id = id,
        .scope_depth = ctx->scope_depth
    };

    emit_decl(ctx, "DEFVAR LF@%s$%zu\n", name, id);
    return id;
}

/**
 * @brief Looks up the innermost visible local variable binding by its source name.
 *
 * Searches the active bindings stack from the most recent declaration outward
 * to properly resolve shadowed variables.
 *
 * @param ctx Pointer to the code generator context.
 * @param name Source identifier name to search for.
 * @return Unique numeric suffix of the matching variable, or 0 if not found.
 */
static size_t lookup_local_var(const CodeGenContext* ctx, const char* name)
{
    for (size_t i = ctx->var_count; i > 0; i--)
    {
        if (strcmp(ctx->vars[i - 1].src_name, name) == 0)
            return ctx->vars[i - 1].unique_id;
    }

    return 0;
}

/**
 * @brief Generates IFJcode26 instructions for a function definition.
 *
 * Resets function-local context state, emits the function label and local frame
 * setup, binds incoming stack parameters in reverse order, generates the body,
 * and emits frame teardown with either EXIT (for main) or RETURN.
 *
 * @param ctx Pointer to the code generator context.
 * @param astNode Pointer to an AST_FUNC_DEF node.
 */
static void gen_func_def(CodeGenContext* ctx, const AstNode* astNode)
{
    ctx->current_func = astNode->func_def.name;
    ctx->var_counter = 0;
    ctx->var_count = 0;
    ctx->scope_depth = 0;

    sb_init(&ctx->decl_buffer);
    sb_init(&ctx->body_buffer);

    fprintf(ctx->out, "\nLABEL %s\n", astNode->func_def.name);
    fprintf(ctx->out, "CREATEFRAME\n");
    fprintf(ctx->out, "PUSHFRAME\n");

    for (size_t i = astNode->func_def.params.count; i > 0; i--) {
        const char* varName = astNode->func_def.params.items[i-1]->param.name;
        const size_t id = declare_local_var(ctx, varName);
        emit_body(ctx, "POPS LF@%s$%zu\n", varName, id);
    }

    if (astNode->func_def.body)
        gen_stmt(ctx, astNode->func_def.body);

    emit_body(ctx, "LABEL $%s$end\n", ctx->current_func);

    fputs(ctx->decl_buffer.data, ctx->out);
    fputs(ctx->body_buffer.data, ctx->out);

    fprintf(ctx->out, "POPFRAME\n");

    if (astNode->func_def.is_main)
        fprintf(ctx->out, "EXIT int@0\n");
    else
        fprintf(ctx->out, "RETURN\n");

    sb_free(&ctx->decl_buffer);
    sb_free(&ctx->body_buffer);
}

/**
 * @brief Generates IFJcode26 instructions for a function call.
 *
 * Pushes call arguments onto the data stack and emits the CALL instruction
 * targeting the callee label.
 *
 * @param ctx Pointer to the code generator context.
 * @param node Pointer to an AST_FUNC_CALL node.
 */
static void callFunc(CodeGenContext* ctx, const AstNode* node)
{
    for (size_t i = 0; i < node->func_call.args.count; i++)
    {
        gen_expr(ctx, node->func_call.args.items[i]);
    }

    emit_body(ctx, "CALL %s\n", node->func_call.func_name);
}

/**
 * @brief Generates IFJcode26 instructions for a statement AST node.
 *
 * Handles blocks, variable declarations, assignments, control flow statements
 * (if, while, for), return statements, and standalone function calls or expressions.
 *
 * @param ctx Pointer to the code generator context.
 * @param node Pointer to the statement AST node (ignored if NULL).
 */
static void gen_stmt(CodeGenContext* ctx, const AstNode* node)
{
    if (!node)
        return;

    switch (node->type)
    {
        case AST_BLOCK:
        {
            scope_enter(ctx);
            for (size_t i = 0; i < node->block.statements.count; i++)
                gen_stmt(ctx, node->block.statements.items[i]);
            scope_leave(ctx);
            break;
        }

        case AST_VAR_DECL:
        {
            const size_t id = declare_local_var(ctx, node->var_decl.name);
            if (node->var_decl.init_expr)
            {
                gen_expr(ctx, node->var_decl.init_expr);
                emit_body(ctx, "POPS LF@%s$%zu\n", node->var_decl.name, id);
            }
            break;
        }

        case AST_ASSIGN:
        {
            const size_t id = lookup_local_var(ctx, node->assign.target_name);
            gen_expr(ctx, node->assign.value_expr);
            emit_body(ctx, "POPS LF@%s$%zu\n", node->assign.target_name, id);
            break;
        }

        case AST_IF:
        {
            const size_t id = ++ctx->label_counter;

            gen_expr(ctx, node->if_stmt.condition);
            emit_body(ctx, "PUSHS bool@false\n");
            emit_body(ctx, "JUMPIFEQS $if_else_%zu\n", id);

            gen_stmt(ctx, node->if_stmt.then_branch);

            emit_body(ctx, "JUMP $if_end_%zu\n", id);
            emit_body(ctx, "LABEL $if_else_%zu\n", id);

            if (node->if_stmt.else_branch != nullptr)
                gen_stmt(ctx, node->if_stmt.else_branch);

            emit_body(ctx, "LABEL $if_end_%zu\n", id);
            break;
        }

        case AST_WHILE:
        {
            const size_t id = ++ctx->label_counter;

            emit_body(ctx, "LABEL $while_start_%zu\n", id);

            gen_expr(ctx, node->while_stmt.condition);

            emit_body(ctx, "PUSHS bool@false\n");
            emit_body(ctx, "JUMPIFEQS $while_end_%zu\n", id);

            gen_stmt(ctx, node->while_stmt.body);

            emit_body(ctx, "JUMP $while_start_%zu\n", id);
            emit_body(ctx, "LABEL $while_end_%zu\n", id);

            break;
        }

        case AST_FOR:
        {
            const size_t forID = ++ctx->label_counter;

            // Calculate bounds and step
            gen_expr(ctx, node->for_stmt.from_expr);
            gen_expr(ctx, node->for_stmt.to_expr);
            if (node->for_stmt.step_expr != nullptr)
                gen_expr(ctx, node->for_stmt.step_expr);
            else
                emit_body(ctx, "PUSHS int@1\n");

            scope_enter(ctx);

            emit_decl(ctx, "DEFVAR LF@$for_step$%zu\n", forID);
            emit_body(ctx, "POPS LF@$for_step$%zu\n", forID);
            emit_body(ctx, "PUSHS LF@$for_step$%zu\n", forID);
            emit_body(ctx, "PUSHS int@0\n");
            emit_body(ctx, "JUMPIFNEQS $check_%zu\n", forID);
            emit_body(ctx, "EXIT int@40\n");
            emit_body(ctx, "LABEL $check_%zu\n", forID);

            emit_decl(ctx, "DEFVAR LF@$for_to$%zu\n", forID);
            emit_body(ctx, "POPS LF@$for_to$%zu\n", forID);

            const size_t id = declare_local_var(ctx, node->for_stmt.iter_name);
            emit_body(ctx, "POPS LF@%s$%zu\n", node->for_stmt.iter_name, id);

            emit_body(ctx, "LABEL $for_start_%zu\n", forID);

            // Check condition
            emit_body(ctx, "PUSHS LF@%s$%zu\n", node->for_stmt.iter_name, id);
            emit_body(ctx, "PUSHS LF@$for_to$%zu\n", forID);
            if (node->for_stmt.is_until)
            {
                emit_body(ctx, "LTS\n");
                emit_body(ctx, "PUSHS bool@false\n");
            }
            else
            {
                emit_body(ctx, "GTS\n");
                emit_body(ctx, "PUSHS bool@true\n");
            }
            emit_body(ctx, "JUMPIFEQS $for_end_%zu\n", forID);

            // generate guards
            for (size_t i = 0; i < node->for_stmt.guards.count; i++)
            {
                gen_expr(ctx, node->for_stmt.guards.items[i]);
                emit_body(ctx, "PUSHS bool@false\n");
                emit_body(ctx, "JUMPIFEQS $for_step_%zu\n", forID);
            }

            // generate for loop body
            gen_stmt(ctx, node->for_stmt.body);

            // process step
            emit_body(ctx, "LABEL $for_step_%zu\n", forID);
            emit_body(ctx, "PUSHS LF@%s$%zu\n", node->for_stmt.iter_name, id);
            emit_body(ctx, "PUSHS LF@$for_step$%zu\n", forID);
            emit_body(ctx, "ADDS\n");
            emit_body(ctx, "POPS LF@%s$%zu\n", node->for_stmt.iter_name, id);

            emit_body(ctx, "JUMP $for_start_%zu\n", forID);
            emit_body(ctx, "LABEL $for_end_%zu\n", forID);
            scope_leave(ctx);
            break;
        }

        case AST_RETURN:
        {
            if (node->return_stmt.expr)
                gen_expr(ctx, node->return_stmt.expr);

            emit_body(ctx, "JUMP $%s$end\n", ctx->current_func);
            break;
        }

        case AST_FUNC_CALL:
        {
            callFunc(ctx, node);

            // Discard void type
            if (node->inferred_type != TYPE_UNIT)
                emit_body(ctx, "POPS GF@$concat$1\n");
            break;
        }

        default:
        {
            gen_expr(ctx, node);
            break;
        }
    }
}

/**
 * @brief Generates IFJcode26 instructions for an expression AST node.
 *
 * Evaluates the expression such that its result is left on top of the data stack.
 * Applies implicit Int-to-Double conversion (INT2FLOATS) if flagged on the node.
 *
 * @param ctx Pointer to the code generator context.
 * @param node Pointer to the expression AST node (ignored if NULL).
 */
static void gen_expr(CodeGenContext* ctx, const AstNode* node)
{
    if (!node)
        return;

    switch (node->type)
    {
        case AST_LIT_INT:
        {
            emit_body(ctx, "PUSHS int@%ld\n", node->lit_int);
            break;
        }

        case AST_LIT_DOUBLE:
        {
            emit_body(ctx, "PUSHS float@%a\n", node->lit_double);
            break;
        }

        case AST_LIT_STRING:
        {
            emit_body(ctx, "PUSHS ");
            emit_string(ctx, node->lit_string);
            sb_append_char(&ctx->body_buffer, '\n');
            break;
        }

        case AST_LIT_BOOL:
        {
            const char* boolean = node->lit_bool ? "true" : "false";
            emit_body(ctx, "PUSHS bool@%s\n", boolean);
            break;
        }

        case AST_LIT_UNIT:
        {
            emit_body(ctx, "PUSHS nil@nil\n");
            break;
        }

        case AST_IDENT:
        {
            const size_t id = lookup_local_var(ctx, node->ident.name);
            emit_body(ctx, "PUSHS LF@%s$%zu\n", node->ident.name, id);
            break;
        }

        case AST_BIN_OP:
        {
            gen_expr(ctx, node->bin_op.left);

            if (node->bin_op.op == OP_AND)
            {
                const size_t id = ++ctx->label_counter;
                emit_body(ctx, "PUSHS bool@false\n");
                emit_body(ctx, "JUMPIFEQS $and_false_%zu\n", id);

                gen_expr(ctx, node->bin_op.right);
                emit_body(ctx, "JUMP $and_end_%zu\n", id);

                emit_body(ctx, "LABEL $and_false_%zu\n", id);
                emit_body(ctx, "PUSHS bool@false\n");
                emit_body(ctx, "LABEL $and_end_%zu\n", id);
            }
            else if (node->bin_op.op == OP_OR)
            {
                const size_t id = ++ctx->label_counter;
                emit_body(ctx, "PUSHS bool@true\n");
                emit_body(ctx, "JUMPIFEQS $or_true_%zu\n", id);

                gen_expr(ctx, node->bin_op.right);
                emit_body(ctx, "JUMP $or_end_%zu\n", id);

                emit_body(ctx, "LABEL $or_true_%zu\n", id);
                emit_body(ctx, "PUSHS bool@true\n");
                emit_body(ctx, "LABEL $or_end_%zu\n", id);
            }
            else
            {
                gen_expr(ctx, node->bin_op.right);
            }


            const char* op = "";
            switch (node->bin_op.op)
            {
                case OP_ADD:
                    if (node->inferred_type == TYPE_STRING)
                    {
                        emit_body(ctx, "POPS GF@$concat$2\n");
                        emit_body(ctx, "POPS GF@$concat$1\n");
                        emit_body(ctx, "CONCAT GF@$concat$1 GF@$concat$1 GF@$concat$2\n");
                        emit_body(ctx, "PUSHS GF@$concat$1\n");
                    }
                    else
                    {
                        op = "ADDS\n";
                    }
                    break;
                case OP_SUB:
                    op = "SUBS\n";
                    break;
                case OP_MUL:
                    op = "MULS\n";
                    break;
                case OP_DIV:
                    if (node->inferred_type == TYPE_DOUBLE)
                        op = "DIVS\n";
                    else
                        op = "IDIVS\n";
                    break;
                case OP_EQ:
                    op = "EQS\n";
                    break;
                case OP_NEQ:
                    op = "EQS\nNOTS\n";
                    break;
                case OP_LT:
                    op = "LTS\n";
                    break;
                case OP_LE:
                    op = "GTS\nNOTS\n";
                    break;
                case OP_GT:
                    op = "GTS\n";
                    break;
                case OP_GE:
                    op = "LTS\nNOTS\n";
                    break;
                default:
                    break;
            }

            emit_body(ctx, "%s", op);
            break;
        }

        case AST_UN_OP:
        {
            switch (node->un_op.op)
            {
                case OP_NEG:
                {
                    if (node->inferred_type == TYPE_DOUBLE)
                        emit_body(ctx, "PUSHS float@0x0p+0\n");
                    else
                        emit_body(ctx, "PUSHS int@0\n");
                    gen_expr(ctx, node->un_op.operand);
                    emit_body(ctx, "SUBS\n");
                    break;
                }
                case OP_POS:
                {
                    gen_expr(ctx, node->un_op.operand);
                    break;
                }
                case OP_NOT:
                {
                    gen_expr(ctx, node->un_op.operand);
                    emit_body(ctx, "NOTS\n");
                    break;
                }
                default:
                    break;
            }
            break;
        }

        case AST_FUNC_CALL:
        {
            callFunc(ctx, node);
            break;
        }

        case AST_IF:
        {
            const size_t id = ++ctx->label_counter;

            // TODO: Check if parser gives expression as then_branch and else_branch

            gen_expr(ctx, node->if_stmt.condition);
            emit_body(ctx, "PUSHS bool@false\n");
            emit_body(ctx, "JUMPIFEQS $ternary_false$%zu\n", id);
            gen_expr(ctx, node->if_stmt.then_branch);
            emit_body(ctx, "JUMP $ternary_end$%zu\n", id);
            emit_body(ctx, "LABEL $ternary_false$%zu\n", id);
            gen_expr(ctx, node->if_stmt.else_branch);
            emit_body(ctx, "LABEL $ternary_end$%zu\n", id);
            break;
        }

        default:
            break;
    }

    if (node->convert_to_double)
        emit_body(ctx, "INT2FLOATS\n");
}

void codegen_generate(const AstNode* program, FILE* out)
{
    if (program == nullptr || program->type != AST_PROGRAM || out == nullptr)
        return;

    CodeGenContext ctx = {};
    ctx.out = out;

    fprintf(ctx.out, ".IFJcode26\n");

    fprintf(ctx.out, "DEFVAR GF@$concat$1\n");
    fprintf(ctx.out, "DEFVAR GF@$concat$2\n");

    fprintf(ctx.out, "JUMP main\n");

    for (size_t i = 0; i < program->program.functions.count; i++)
    {
        const AstNode* astNode = program->program.functions.items[i];
        if (astNode && astNode->type == AST_FUNC_DEF)
            gen_func_def(&ctx, astNode);
    }

    free(ctx.vars);
}