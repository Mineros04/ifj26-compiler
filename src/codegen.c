/**
 * @file codegen.c
 * @authors Pavel Bobek
 * @brief Implementation of the code generator for IFJ26.
 */

#include "codegen.h"

#include <stdlib.h>
#include <string.h>

typedef struct
{
    char* src_name;         // Original identifier in AST
    long unique_id;         // Unique suffix inside current function
    int scope_depth;        // Block nesting level
} VarBinding;

typedef struct
{
    FILE* out;
    long label_counter;
    long var_counter;
    int scope_depth;

    char* current_func;
    bool in_main;

    // Active variable bindings in the current function
    VarBinding* vars;
    long var_count;
    long var_cap;
} CodeGenContext;

void gen_stmt(CodeGenContext* ctx, AstNode* node);
void gen_expr(CodeGenContext* ctx, AstNode* node);

char* convertString(char* c)
{
    // TODO: convert string to escaped string
    return c;
}

void callFunc(CodeGenContext* ctx, AstNode* node)
{
    for (int i = 0; i < node->func_call.args.count; i++)
    {
        AstNode* param = node->func_call.args.items[i];
        if (param->type != AST_PARAM)
            continue;

        // TODO: Push params to the stack
    }

    fprintf(ctx->out, "CALL %s\n", node->func_call.func_name);
}

void scope_enter(CodeGenContext* ctx)
{
    ctx->scope_depth++;
}

void scope_leave(CodeGenContext* ctx)
{
    while (ctx->var_count > 0 && ctx->vars[ctx->var_count - 1].scope_depth == ctx->scope_depth)
    {
        ctx->var_count--;
    }
    ctx->scope_depth--;
}

long declare_local_var(CodeGenContext* ctx, char* name)
{
    if (ctx->var_count == ctx->var_cap)
    {
        ctx->var_cap = ctx->var_cap == 0 ? 16 : ctx->var_cap * 2;
        ctx->vars = realloc(ctx->vars, ctx->var_cap * sizeof(VarBinding));
    }

    long id = ++ctx->var_counter;
    ctx->vars[ctx->var_count++] = (VarBinding) {
        .src_name = name,
        .unique_id = id,
        .scope_depth = ctx->scope_depth
    };

    fprintf(ctx->out, "DEFVAR LF@%s$%ld\n", name, id);
    return id;
}

long lookup_local_var(CodeGenContext* ctx, char* name)
{
    for (long i = ctx->var_count; i > 0; i--)
    {
        if (strcmp(ctx->vars[i - 1].src_name, name) == 0)
            return ctx->vars[i - 1].unique_id;
    }

    return 0;
}

void gen_func_def(CodeGenContext* ctx, AstNode* astNode)
{
    ctx->current_func = astNode->func_def.name;
    ctx->in_main = astNode->func_def.is_main;
    ctx->var_counter = 0;
    ctx->var_count = 0;
    ctx->scope_depth = 0;

    fprintf(ctx->out, "\nLABEL %s\n", astNode->func_def.name);
    fprintf(ctx->out, "CREATEFRAME\n");
    fprintf(ctx->out, "PUSHFRAME\n");

    // TODO: pop parameters

    if (astNode->func_def.body)
        gen_stmt(ctx, astNode->func_def.body);

    fprintf(ctx->out, "POPFRAME\n");

    if (astNode->func_def.is_main)
        fprintf(ctx->out, "EXIT int@0\n");
    else
        fprintf(ctx->out, "RETURN\n");
}

void gen_stmt(CodeGenContext* ctx, AstNode* node)
{
    if (!node)
        return;

    switch (node->type)
    {
        case AST_BLOCK:
        {
            scope_enter(ctx);
            for (long i = 0; i < node->block.statements.count; i++)
                gen_stmt(ctx, node->block.statements.items[i]);
            scope_leave(ctx);
            break;
        }

        case AST_VAR_DECL:
        {
            long id = declare_local_var(ctx, node->var_decl.name);
            if (node->var_decl.init_expr)
            {
                gen_expr(ctx, node->var_decl.init_expr);
                fprintf(ctx->out, "POPS LF@%s$%ld\n", node->var_decl.name, id);
            }
            break;
        }

        case AST_ASSIGN:
        {
            long id = lookup_local_var(ctx, node->assign.target_name);
            gen_expr(ctx, node->assign.value_expr);
            fprintf(ctx->out, "POPS LF@%s$%ld\n", node->assign.target_name, id);
            break;
        }

        case AST_IF:
        {
            long id = ++ctx->label_counter;

            gen_expr(ctx, node->if_stmt.condition);
            fprintf(ctx->out, "PUSHS bool@false\n");
            fprintf(ctx->out, "JUMPIFEQS $if_else_%ld\n", id);

            gen_stmt(ctx, node->if_stmt.then_branch);

            fprintf(ctx->out, "JUMP $if_end_%ld\n", id);
            fprintf(ctx->out, "LABEL $if_else_%ld\n", id);

            if (node->if_stmt.else_branch != NULL)
                gen_stmt(ctx, node->if_stmt.else_branch);

            fprintf(ctx->out, "LABEL $if_end_%ld\n", id);
            break;
        }

        case AST_WHILE:
        {
            long id = ++ctx->label_counter;

            fprintf(ctx->out, "LABEL $while_start_%ld\n", id);

            gen_expr(ctx, node->while_stmt.condition);

            fprintf(ctx->out, "PUSHS bool@false\n");
            fprintf(ctx->out, "JUMPIFEQS $while_end_%ld\n", id);

            gen_stmt(ctx, node->while_stmt.body);

            fprintf(ctx->out, "JUMP $while_start_%ld\n", id);
            fprintf(ctx->out, "LABEL $while_end_%ld\n", id);

            break;
        }

        case AST_FOR:
        {
            long forID = ++ctx->label_counter;

            // Calculate bounds and step
            gen_expr(ctx, node->for_stmt.from_expr);
            gen_expr(ctx, node->for_stmt.to_expr);
            if (node->for_stmt.step_expr != NULL)
                gen_expr(ctx, node->for_stmt.step_expr);
            else
                fprintf(ctx->out, "PUSHS int@1\n");

            scope_enter(ctx);

            fprintf(ctx->out, "DEFVAR LF@for_step$%ld\n", forID);
            fprintf(ctx->out, "POPS LF@for_step$%ld\n", forID);

            // TODO: add zero step check to exit with error code 40;

            fprintf(ctx->out, "DEFVAR LF@for_to$%ld\n", forID);
            fprintf(ctx->out, "POPS LF@for_to$%ld\n", forID);

            long id = declare_local_var(ctx, node->for_stmt.iter_name);
            fprintf(ctx->out, "POPS LF@%s$%ld\n", node->for_stmt.iter_name, id);

            fprintf(ctx->out, "LABEL $for_start_%ld\n", forID);

            // Check condition
            fprintf(ctx->out, "PUSHS LF@%s$%ld\n", node->for_stmt.iter_name, id);
            fprintf(ctx->out, "PUSHS LF@for_to$%ld\n", forID);
            if (node->for_stmt.is_until)
            {
                fprintf(ctx->out, "LTS\n");
                fprintf(ctx->out, "PUSHS bool@false\n");
            }
            else
            {
                fprintf(ctx->out, "GTS\n");
                fprintf(ctx->out, "PUSHS bool@true\n");
            }
            fprintf(ctx->out, "JUMPIFEQS $for_end_%ld\n", forID);

            // generate guards
            for (int i = 0; i < node->for_stmt.guards.count; i++)
            {
                gen_expr(ctx, node->for_stmt.guards.items[i]);
                fprintf(ctx->out, "PUSHS bool@false\n");
                fprintf(ctx->out, "JUMPIFEQS $for_step_%ld\n", forID);
            }

            // generate for loop body
            gen_stmt(ctx, node->for_stmt.body);

            // process step
            fprintf(ctx->out, "LABEL $for_step_%ld\n", forID);
            fprintf(ctx->out, "PUSHS LF@%s$%ld\n", node->for_stmt.iter_name, id);
            fprintf(ctx->out, "PUSHS LF@for_step$%ld\n", forID);
            fprintf(ctx->out, "ADDS\n");
            fprintf(ctx->out, "POPS LF@%s$%ld\n", node->for_stmt.iter_name, id);

            fprintf(ctx->out, "JUMP $for_start_%ld\n", forID);
            fprintf(ctx->out, "LABEL $for_end_%ld\n", forID);
            scope_leave(ctx);
            break;
        }

        case AST_RETURN:
        {
            if (node->return_stmt.expr)
                gen_expr(ctx, node->return_stmt.expr);

            fprintf(ctx->out, "RETURN\n");
            break;
        }

        case AST_FUNC_CALL:
        {
            callFunc(ctx, node);
            break;
        }

        default:
        {
            gen_expr(ctx, node);
            break;
        }
    }
}

void gen_expr(CodeGenContext* ctx, AstNode* node)
{
    if (!node)
        return;

    switch (node->type)
    {
        case AST_LIT_INT:
        {
            fprintf(ctx->out, "PUSHS int@%lld\n", node->lit_int);
            break;
        }

        case AST_LIT_DOUBLE:
        {
            fprintf(ctx->out, "PUSHS float@%a\n", node->lit_double);
            break;
        }

        case AST_LIT_STRING:
        {
            fprintf(ctx->out, "PUSHS string@%s\n", convertString(node->lit_string));
            break;
        }

        case AST_LIT_BOOL:
        {
            char* boolean = node->lit_bool ? "true" : "false";
            fprintf(ctx->out, "PUSHS bool@%s\n", boolean);
            break;
        }

        case AST_LIT_UNIT:
        {
            fprintf(ctx->out, "PUSHS nil@nil\n");
            break;
        }

        case AST_IDENT:
        {
            long id = lookup_local_var(ctx, node->ident.name);
            fprintf(ctx->out, "PUSHS LF@%s$%ld\n", node->ident.name, id);
            break;
        }

        case AST_BIN_OP:
        {
            gen_expr(ctx, node->bin_op.left);

            if (node->bin_op.op == OP_AND)
            {
                long id = ++ctx->label_counter;
                fprintf(ctx->out, "PUSHS bool@false\n");
                fprintf(ctx->out, "JUMPIFEQS $and_false_%ld\n", id);

                gen_expr(ctx, node->bin_op.right);
                fprintf(ctx->out, "JUMP $and_end_%ld\n", id);

                fprintf(ctx->out, "LABEL $and_false_%ld\n", id);
                fprintf(ctx->out, "PUSHS bool@false\n");
                fprintf(ctx->out, "LABEL $and_end_%ld\n", id);
            }
            else if (node->bin_op.op == OP_OR)
            {
                long id = ++ctx->label_counter;
                fprintf(ctx->out, "PUSHS bool@true\n");
                fprintf(ctx->out, "JUMPIFEQS $or_true_%ld\n", id);

                gen_expr(ctx, node->bin_op.right);
                fprintf(ctx->out, "JUMP $or_end_%ld\n", id);

                fprintf(ctx->out, "LABEL $or_true_%ld\n", id);
                fprintf(ctx->out, "PUSHS bool@true\n");
                fprintf(ctx->out, "LABEL $or_end_%ld\n", id);
            }
            else
            {
                gen_expr(ctx, node->bin_op.right);
            }


            char* op = "";
            switch (node->bin_op.op)
            {
                case OP_ADD:
                    if (node->inferred_type == TYPE_STRING)
                    {
                        fprintf(ctx->out, "POPS GF@$temp$2\n");
                        fprintf(ctx->out, "POPS GF@$temp$1\n");
                        fprintf(ctx->out, "CONCAT GF@$temp$1 GF@$temp$1 GF@$temp$2\n");
                        fprintf(ctx->out, "PUSHS GF@$temp$1\n");
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
                    // TODO: check if this is correct
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

            fprintf(ctx->out, "%s", op);
            break;
        }

        case AST_UN_OP:
        {
            switch (node->un_op.op)
            {
                case OP_NEG:
                {
                    // TODO: check if this is correct
                    if (node->inferred_type == TYPE_DOUBLE)
                        fprintf(ctx->out, "PUSHS float@0x0p+0\n");
                    else
                        fprintf(ctx->out, "PUSHS int@0\n");
                    gen_expr(ctx, node->un_op.operand);
                    fprintf(ctx->out, "SUBS\n");
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
                    fprintf(ctx->out, "NOTS\n");
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
            // TODO: ternární operace :)
            break;
        }

        default:
            break;
    }

    if (node->convert_to_double)
        fprintf(ctx->out, "INT2FLOATS\n");
}

void codegen_generate(AstNode* program, FILE* out)
{
    if (!program || program->type != AST_PROGRAM || !out)
        return;

    CodeGenContext ctx = {};
    ctx.out = out;

    fprintf(out, ".IFJcode26\n");

    fprintf(ctx.out, "DEFVAR GF@temp$1\n");
    fprintf(ctx.out, "DEFVAR GF@temp$2\n");

    fprintf(out, "JUMP main\n");

    for (long i = 0; i < program->program.functions.count; i++)
    {
        AstNode* astNode = program->program.functions.items[i];
        if (astNode && astNode->type == AST_FUNC_DEF)
            gen_func_def(&ctx, astNode);
    }

    free(ctx.vars);
}