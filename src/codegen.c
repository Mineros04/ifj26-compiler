/**
- @file codegen.c
- @authors Pavel Bobek
- @brief Implementation of the code generator for IFJ26.
 */

#include "codegen.h"

typedef struct {
    const char *src_name; // Original identifier in AST
    size_t unique_id;     // Unique suffix inside current function
    int scope_depth;      // Block nesting level
} Variable;

typedef struct {
    char *data; // Data of the string
    size_t len; // Length of the string
    size_t cap; // Length of the allocated buffer
} StrBuf;

typedef struct {
    FILE *out;            // Output file
    size_t label_counter; // Number of labels
    size_t var_counter;   // Number of variables in current scope
    int scope_depth;      // Depth of the scope

    const char *current_func;          // Record of the current function
    DataType current_func_return_type; // Return type of the current function

    StrBuf decl_buffer; // Buffer of declared variables in a body
    StrBuf body_buffer; // Buffer for the body

    Variable *vars;   // Active variable bindings in the current function
    size_t var_count; // Length of variables in the vars list
    size_t var_cap;   // Length of the allocaled buffer for vars
} CodeGenContext;

static void gen_stmt(CodeGenContext *ctx, const AstNode *node);
static void gen_expr(CodeGenContext *ctx, const AstNode *node);

/**
- @brief Checks if a body only has statements
 *
- @param node Node of the statement
 */
static bool is_stmt_only(const AstNode *node) {
    if (node == nullptr)
        return true;

    switch (node->type) {
    case AST_VAR_DECL:
    case AST_ASSIGN:
    case AST_WHILE:
    case AST_FOR:
    case AST_RETURN:
        return true;
    case AST_IF:
        return node->if_stmt.else_branch == nullptr;
    default:
        return false;
    }
}

/**
- @brief Generate all builtin functions
 *
- @param out Output file
 */
static void gen_builtins(FILE *out) {
    // print(value: Any): Unit
    fputs("\nLABEL print\n"
          "CREATEFRAME\n"
          "PUSHFRAME\n"
          "DEFVAR LF@val\n"
          "DEFVAR LF@type\n"
          "POPS LF@val\n"
          "TYPE LF@type LF@val\n"
          "JUMPIFEQ $print$unit LF@type string@nil\n"
          "WRITE LF@val\n"
          "JUMP $print$end\n"
          "LABEL $print$unit\n"
          "WRITE string@()\n"
          "LABEL $print$end\n"
          "PUSHS nil@nil\n"
          "POPFRAME\n"
          "RETURN\n",
          out);

    // readLine(): String
    fputs("\nLABEL readLine\n"
          "CREATEFRAME\n"
          "PUSHFRAME\n"
          "DEFVAR LF@res\n"
          "DEFVAR LF@type\n"
          "READ LF@res string\n"
          "TYPE LF@type LF@res\n"
          "JUMPIFNEQ $readLine$ok LF@type string@nil\n"
          "MOVE LF@res string@\n"
          "LABEL $readLine$ok\n"
          "PUSHS LF@res\n"
          "POPFRAME\n"
          "RETURN\n",
          out);

    // readInt(): Int
    fputs("\nLABEL readInt\n"
          "CREATEFRAME\n"
          "PUSHFRAME\n"
          "DEFVAR LF@res\n"
          "DEFVAR LF@type\n"
          "READ LF@res int\n"
          "TYPE LF@type LF@res\n"
          "JUMPIFNEQ $readInt$ok LF@type string@nil\n"
          "MOVE LF@res int@0\n"
          "LABEL $readInt$ok\n"
          "PUSHS LF@res\n"
          "POPFRAME\n"
          "RETURN\n",
          out);

    // readDouble(): Double
    fputs("\nLABEL readDouble\n"
          "CREATEFRAME\n"
          "PUSHFRAME\n"
          "DEFVAR LF@res\n"
          "DEFVAR LF@type\n"
          "READ LF@res float\n"
          "TYPE LF@type LF@res\n"
          "JUMPIFNEQ $readDouble$ok LF@type string@nil\n"
          "MOVE LF@res float@0x0p+0\n"
          "LABEL $readDouble$ok\n"
          "PUSHS LF@res\n"
          "POPFRAME\n"
          "RETURN\n",
          out);

    // readBoolean(): Boolean
    fputs("\nLABEL readBoolean\n"
          "CREATEFRAME\n"
          "PUSHFRAME\n"
          "DEFVAR LF@res\n"
          "DEFVAR LF@type\n"
          "READ LF@res bool\n"
          "TYPE LF@type LF@res\n"
          "JUMPIFNEQ $readBoolean$ok LF@type string@nil\n"
          "MOVE LF@res bool@false\n"
          "LABEL $readBoolean$ok\n"
          "PUSHS LF@res\n"
          "POPFRAME\n"
          "RETURN\n",
          out);

    // toStr(value: Any): String
    fputs("\nLABEL toStr\n"
          "CREATEFRAME\n"
          "PUSHFRAME\n"
          "DEFVAR LF@val\n"
          "DEFVAR LF@type\n"
          "DEFVAR LF@res\n"
          "POPS LF@val\n"
          "TYPE LF@type LF@val\n"
          "JUMPIFEQ $toStr$str LF@type string@string\n"
          "JUMPIFEQ $toStr$int LF@type string@int\n"
          "JUMPIFEQ $toStr$float LF@type string@float\n"
          "JUMPIFEQ $toStr$bool LF@type string@bool\n"
          "MOVE LF@res string@()\n"
          "JUMP $toStr$end\n"
          "LABEL $toStr$str\n"
          "MOVE LF@res LF@val\n"
          "JUMP $toStr$end\n"
          "LABEL $toStr$int\n"
          "INT2STR LF@res LF@val\n"
          "JUMP $toStr$end\n"
          "LABEL $toStr$float\n"
          "FLOAT2STR LF@res LF@val\n"
          "JUMP $toStr$end\n"
          "LABEL $toStr$bool\n"
          "JUMPIFEQ $toStr$true LF@val bool@true\n"
          "MOVE LF@res string@false\n"
          "JUMP $toStr$end\n"
          "LABEL $toStr$true\n"
          "MOVE LF@res string@true\n"
          "LABEL $toStr$end\n"
          "PUSHS LF@res\n"
          "POPFRAME\n"
          "RETURN\n",
          out);

    // toInt(value: Double): Int
    fputs("\nLABEL toInt\n"
          "CREATEFRAME\n"
          "PUSHFRAME\n"
          "DEFVAR LF@val\n"
          "DEFVAR LF@res\n"
          "POPS LF@val\n"
          "FLOAT2INT LF@res LF@val\n"
          "PUSHS LF@res\n"
          "POPFRAME\n"
          "RETURN\n",
          out);

    // floor(value: Double): Double
    fputs("\nLABEL floor\n"
          "CREATEFRAME\n"
          "PUSHFRAME\n"
          "DEFVAR LF@val\n"
          "DEFVAR LF@tmp\n"
          "DEFVAR LF@res\n"
          "DEFVAR LF@cond\n"
          "POPS LF@val\n"
          "FLOAT2INT LF@tmp LF@val\n"
          "INT2FLOAT LF@res LF@tmp\n"
          "LT LF@cond LF@val LF@res\n"
          "JUMPIFEQ $floor$done LF@cond bool@false\n"
          "SUB LF@res LF@res float@0x1p+0\n"
          "LABEL $floor$done\n"
          "PUSHS LF@res\n"
          "POPFRAME\n"
          "RETURN\n",
          out);

    // strLength(value: String): Int
    fputs("\nLABEL strLength\n"
          "CREATEFRAME\n"
          "PUSHFRAME\n"
          "DEFVAR LF@val\n"
          "DEFVAR LF@res\n"
          "POPS LF@val\n"
          "STRLEN LF@res LF@val\n"
          "PUSHS LF@res\n"
          "POPFRAME\n"
          "RETURN\n",
          out);

    // subStr(value: String, start: Int, end: Int): String
    fputs("\nLABEL subStr\n"
          "CREATEFRAME\n"
          "PUSHFRAME\n"
          "DEFVAR LF@end\n"
          "DEFVAR LF@start\n"
          "DEFVAR LF@str\n"
          "DEFVAR LF@len\n"
          "DEFVAR LF@cond\n"
          "DEFVAR LF@res\n"
          "DEFVAR LF@ch\n"
          "DEFVAR LF@i\n"
          "POPS LF@end\n"
          "POPS LF@start\n"
          "POPS LF@str\n"
          "STRLEN LF@len LF@str\n"
          "LT LF@cond LF@start int@0\n"
          "JUMPIFEQ $subStr$err LF@cond bool@true\n"
          "LT LF@cond LF@end int@0\n"
          "JUMPIFEQ $subStr$err LF@cond bool@true\n"
          "GT LF@cond LF@start LF@end\n"
          "JUMPIFEQ $subStr$err LF@cond bool@true\n"
          "LT LF@cond LF@start LF@len\n"
          "JUMPIFEQ $subStr$err LF@cond bool@false\n"
          "GT LF@cond LF@end LF@len\n"
          "JUMPIFEQ $subStr$err LF@cond bool@true\n"
          "MOVE LF@res string@\n"
          "MOVE LF@i LF@start\n"
          "LABEL $subStr$loop\n"
          "LT LF@cond LF@i LF@end\n"
          "JUMPIFEQ $subStr$done LF@cond bool@false\n"
          "GETCHAR LF@ch LF@str LF@i\n"
          "CONCAT LF@res LF@res LF@ch\n"
          "ADD LF@i LF@i int@1\n"
          "JUMP $subStr$loop\n"
          "LABEL $subStr$err\n"
          "EXIT int@40\n"
          "LABEL $subStr$done\n"
          "PUSHS LF@res\n"
          "POPFRAME\n"
          "RETURN\n",
          out);

    // ord(value: String, index: Int): Int
    fputs("\nLABEL ord\n"
          "CREATEFRAME\n"
          "PUSHFRAME\n"
          "DEFVAR LF@idx\n"
          "DEFVAR LF@str\n"
          "DEFVAR LF@len\n"
          "DEFVAR LF@cond\n"
          "DEFVAR LF@res\n"
          "POPS LF@idx\n"
          "POPS LF@str\n"
          "STRLEN LF@len LF@str\n"
          "LT LF@cond LF@idx int@0\n"
          "JUMPIFEQ $ord$oob LF@cond bool@true\n"
          "LT LF@cond LF@idx LF@len\n"
          "JUMPIFEQ $ord$oob LF@cond bool@false\n"
          "STRI2INT LF@res LF@str LF@idx\n"
          "PUSHS LF@res\n"
          "POPFRAME\n"
          "RETURN\n"
          "LABEL $ord$oob\n"
          "PUSHS int@-1\n"
          "POPFRAME\n"
          "RETURN\n",
          out);

    // chr(code: Int): String
    fputs("\nLABEL chr\n"
          "CREATEFRAME\n"
          "PUSHFRAME\n"
          "DEFVAR LF@code\n"
          "DEFVAR LF@res\n"
          "POPS LF@code\n"
          "INT2CHAR LF@res LF@code\n"
          "PUSHS LF@res\n"
          "POPFRAME\n"
          "RETURN\n",
          out);
}

/**
- @brief Initializes a string buffer.
 *
- @param sb Pointer to the string buffer.
 */
static void sb_init(StrBuf *sb) {
    sb->cap     = 1024;
    sb->len     = 0;
    sb->data    = safe_malloc(sb->cap);
    sb->data[0] = '\0';
}

/**
- @brief Frees a string buffer.
 *
- @param sb Pointer to the string buffer.
 */
static void sb_free(StrBuf *sb) {
    free(sb->data);
    sb->data = nullptr;
    sb->len  = 0;
    sb->cap  = 0;
}

/**
- @brief Appends a character to a string buffer.
 *
- @param sb Pointer to the string buffer.
- @param c Character to append.
 */
static void sb_append_char(StrBuf *sb, const char c) {
    if (sb->len + 2 > sb->cap) {
        sb->cap  = sb->cap == 0 ? 1024 : sb->cap * 2;
        sb->data = safe_realloc(sb->data, sb->cap);
    }
    sb->data[sb->len] = c;
    sb->len++;
    sb->data[sb->len] = '\0';
}

/**
- @brief Appends a formatted string to the end of a string buffer. Resizes if needed.
 *
- @param sb Pointer to the string buffer.
- @param fmt String format.
- @param args Arguments for the formatted string.
 */
static void sb_vappendf(StrBuf *sb, const char *fmt, va_list args) {
    va_list argsCopy;
    va_copy(argsCopy, args);
    const int needed = vsnprintf(nullptr, 0, fmt, argsCopy);
    va_end(argsCopy);

    if (needed <= 0)
        return;

    const size_t required = sb->len + (size_t)needed + 1;
    if (required > sb->cap) {
        while (sb->cap < required) {
            sb->cap = sb->cap == 0 ? 1024 : sb->cap * 2;
        }
        sb->data = safe_realloc(sb->data, sb->cap);
    }

    vsnprintf(sb->data + sb->len, sb->cap - sb->len, fmt, args);
    sb->len += (size_t)needed;
}

/**
- @brief Emits the declaration of variables.
 *
- @param ctx Context for code generator.
- @param fmt String format.
- @param ... Arguments for formatted string.
 */
static void emit_decl(CodeGenContext *ctx, const char *fmt, ...) {
    va_list args = {};
    va_start(args, fmt);
    sb_vappendf(&ctx->decl_buffer, fmt, args);
    va_end(args);
}

/**
- @brief Emits the body.
 *
- @param ctx Context for code generator.
- @param fmt String format.
- @param ... Arguments for formatted string.
 */
static void emit_body(CodeGenContext *ctx, const char *fmt, ...) {
    va_list args = {};
    va_start(args, fmt);
    sb_vappendf(&ctx->body_buffer, fmt, args);
    va_end(args);
}

/**
- @brief Emits an IFJcode26 formatted string literal with the string@ prefix.
 *
- Escapes control characters using (ASCII 0-32), whitespace, '#' (35), '\' (92),
- and non-ASCII bytes (>= 127) using the \xyz decimal escape format.
 *
- @param ctx Context for code generator.
- @param src Null-terminated C string to format (emits empty string if NULL).
 */
static void emit_string(CodeGenContext *ctx, const char *src) {
    emit_body(ctx, "string@");
    if (!src)
        return;

    for (auto p = (const unsigned char *)src; *p != '\0'; p++) {
        const unsigned char c = *p;

        if (c <= 32 || c == 35 || c == 92 || c >= 127)
            emit_body(ctx, "\\%03u", (unsigned int)c);
        else
            sb_append_char(&ctx->body_buffer, (char)c);
    }
}

/**
- @brief Enters a new block scope. Increments the current scope in the code generation context.
 *
- @param ctx Pointer to the code generator context.
 */
static void scope_enter(CodeGenContext *ctx) {
    ctx->scope_depth++;
}

/**
- @brief Leaves the current block scope. Removes all local variables declared at the current depth
- and decrements the scope depth counter.
 *
- @param ctx Pointer to the code generator context.
 */
static void scope_leave(CodeGenContext *ctx) {
    while (ctx->var_count > 0 && ctx->vars[ctx->var_count - 1].scope_depth == ctx->scope_depth) {
        ctx->var_count--;
    }
    ctx->scope_depth--;
}

/**
- @brief Registers a new local variable in the current scope and emits its DEFVAR instruction.
 *
- @param ctx Pointer to the code generator context.
- @param name Name of the variable.
 *
- @return Unique numeric suffix assigned to the variable.
 */
static size_t declare_local_var(CodeGenContext *ctx, const char *name) {
    // Check if the buffer needs resizing
    if (ctx->var_count == ctx->var_cap) {
        ctx->var_cap = ctx->var_cap == 0 ? 16 : ctx->var_cap * 2;
        ctx->vars    = safe_realloc(ctx->vars, ctx->var_cap * sizeof(Variable));
    }

    // Create a record of the variable
    const size_t id = ++ctx->var_counter;
    ctx->vars[ctx->var_count++] =
        (Variable){.src_name = name, .unique_id = id, .scope_depth = ctx->scope_depth};

    emit_decl(ctx, "DEFVAR LF@%s$%zu\n", name, id);
    return id;
}

/**
- @brief Looks up the innermost visible local variables by its name.
 *
- @param ctx Pointer to the code generator context.
- @param name Name to search for.
 *
- @return Unique numeric suffix of the matching variable, or 0 if not found.
 */
static size_t lookup_local_var(const CodeGenContext *ctx, const char *name) {
    for (size_t i = ctx->var_count; i > 0; i--) {
        if (strcmp(ctx->vars[i - 1].src_name, name) == 0)
            return ctx->vars[i - 1].unique_id;
    }

    return 0;
}

/**
- @brief Generates IFJcode26 instructions for a function definition.
 *
- Resets function-local context state, emits the function label and local frame
- setup, binds incoming stack parameters in reverse order, generates the body,
- and emits frame teardown with either EXIT (for main) or RETURN.
 *
- @param ctx Pointer to the code generator context.
- @param astNode Pointer to a function node.
 */
static void gen_func_def(CodeGenContext *ctx, const AstNode *astNode) {
    // Reset local context
    ctx->current_func             = astNode->func_def.name;
    ctx->current_func_return_type = astNode->func_def.return_type;
    ctx->var_counter              = 0;
    ctx->var_count                = 0;
    ctx->scope_depth              = 0;

    // Initialize buffers for both body and variable declaration
    sb_init(&ctx->decl_buffer);
    sb_init(&ctx->body_buffer);

    // Print the label and frame entry
    fprintf(ctx->out, "\nLABEL %s\n", astNode->func_def.name);
    fprintf(ctx->out, "CREATEFRAME\n");
    fprintf(ctx->out, "PUSHFRAME\n");

    // Pop all arguments to variables in reverse order
    for (size_t i = astNode->func_def.params.count; i > 0; i--) {
        const char *varName = astNode->func_def.params.items[i - 1]->param.name;
        const size_t id     = declare_local_var(ctx, varName);
        emit_body(ctx, "POPS LF@%s$%zu\n", varName, id);
    }

    // Evaluate the body as an expression so it always leaves 1 return value on the stack
    if (astNode->func_def.body != nullptr)
        gen_expr(ctx, astNode->func_def.body);
    else
        emit_body(ctx, "PUSHS nil@nil\n");

    // For Unit functions, discard the body value and push Unit only for non-main functions
    if (astNode->func_def.return_type == TYPE_UNIT) {
        emit_body(ctx, "POPS GF@$concat$1\n");
        if (!astNode->func_def.is_main)
            emit_body(ctx, "PUSHS nil@nil\n");
    }

    // Emit ending label for a function
    emit_body(ctx, "LABEL $%s$end\n", ctx->current_func);

    // Print variable declaration first and then the body
    fputs(ctx->decl_buffer.data, ctx->out);
    fputs(ctx->body_buffer.data, ctx->out);

    // End the frame
    fprintf(ctx->out, "POPFRAME\n");

    // Print the final return
    fprintf(ctx->out, "RETURN\n");

    // Free both buffers
    sb_free(&ctx->decl_buffer);
    sb_free(&ctx->body_buffer);
}

/**
- @brief Generates IFJcode26 instructions for a function call. Pushes all arguments onto the stack.
 *
- @param ctx Pointer to the code generator context.
- @param node Pointer to a fuction node.
 */
static void callFunc(CodeGenContext *ctx, const AstNode *node) {
    // Pushes all parameters to the stack
    for (size_t i = 0; i < node->func_call.args.count; i++) {
        gen_expr(ctx, node->func_call.args.items[i]);
    }

    // Print call to the function
    emit_body(ctx, "CALL %s\n", node->func_call.func_name);
}

/**
- @brief Recursivly generates IFJcode26 instructions for a statement AST node.
 *
- Handles blocks, variable declarations, assignments, control flow statements
- (if, while, for), return statements, and standalone function calls or expressions.
 *
- @param ctx Pointer to the code generator context.
- @param node Pointer to the statement AST node (ignored if NULL).
 */
static void gen_stmt(CodeGenContext *ctx, const AstNode *node) {
    // Skip empty node
    if (!node)
        return;

    // Process all types of nodes in a body
    switch (node->type) {
    case AST_BLOCK: {
        // Enter new scope and process the inside block
        scope_enter(ctx);
        for (size_t i = 0; i < node->block.statements.count; i++)
            gen_stmt(ctx, node->block.statements.items[i]);
        scope_leave(ctx);
        break;
    }

    case AST_VAR_DECL: {
        // Declare the variable
        const size_t id = declare_local_var(ctx, node->var_decl.name);

        // If it has init expression process it
        if (node->var_decl.init_expr != nullptr) {
            gen_expr(ctx, node->var_decl.init_expr);
            emit_body(ctx, "POPS LF@%s$%zu\n", node->var_decl.name, id);
        }
        break;
    }

    case AST_ASSIGN: {
        // Find the variable
        const size_t id = lookup_local_var(ctx, node->assign.target_name);

        // Process the expression
        gen_expr(ctx, node->assign.value_expr);

        // Pop the result into the variable
        emit_body(ctx, "POPS LF@%s$%zu\n", node->assign.target_name, id);
        break;
    }

    case AST_IF: {
        // Get the unique ID for the statement
        const size_t id = ++ctx->label_counter;

        // Process the if expression
        gen_expr(ctx, node->if_stmt.condition);

        // Print condition skip
        emit_body(ctx, "PUSHS bool@false\n");
        emit_body(ctx, "JUMPIFEQS $if_else_%zu\n", id);

        // Generate then branch
        gen_stmt(ctx, node->if_stmt.then_branch);

        // Print skip else branch and create label for else branch
        emit_body(ctx, "JUMP $if_end_%zu\n", id);
        emit_body(ctx, "LABEL $if_else_%zu\n", id);

        // Generate else branch if it exists
        if (node->if_stmt.else_branch != nullptr)
            gen_stmt(ctx, node->if_stmt.else_branch);

        // Print end for the if statement
        emit_body(ctx, "LABEL $if_end_%zu\n", id);
        break;
    }

    case AST_WHILE: {
        // Get the unique ID for the statement
        const size_t id = ++ctx->label_counter;

        // Print the starting point of the loop
        emit_body(ctx, "LABEL $while_start_%zu\n", id);

        // Generate the condition
        gen_expr(ctx, node->while_stmt.condition);

        // Skip the if condition
        emit_body(ctx, "PUSHS bool@false\n");
        emit_body(ctx, "JUMPIFEQS $while_end_%zu\n", id);

        // Generate the body of the loop
        gen_stmt(ctx, node->while_stmt.body);

        // Print jump to start and end of loop label
        emit_body(ctx, "JUMP $while_start_%zu\n", id);
        emit_body(ctx, "LABEL $while_end_%zu\n", id);

        break;
    }

    case AST_FOR: {
        // Get the unique ID for the statement
        const size_t forID = ++ctx->label_counter;

        // Generate bounds and step of the for loop
        gen_expr(ctx, node->for_stmt.from_expr);
        gen_expr(ctx, node->for_stmt.to_expr);
        if (node->for_stmt.step_expr != nullptr)
            gen_expr(ctx, node->for_stmt.step_expr);
        else
            emit_body(ctx, "PUSHS int@1\n");

        // Enter new scope for bounds and step variables
        scope_enter(ctx);

        // Generate and pop variables for bounds and step
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
        if (node->for_stmt.is_until) {
            emit_body(ctx, "LTS\n");
            emit_body(ctx, "PUSHS bool@false\n");
        } else {
            emit_body(ctx, "GTS\n");
            emit_body(ctx, "PUSHS bool@true\n");
        }
        emit_body(ctx, "JUMPIFEQS $for_end_%zu\n", forID);

        // generate guards
        for (size_t i = 0; i < node->for_stmt.guards.count; i++) {
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

    case AST_RETURN: {
        // Generate expression if it exists, otherwise push Unit
        if (node->return_stmt.expr != nullptr) {
            gen_expr(ctx, node->return_stmt.expr);
            if (ctx->current_func_return_type == TYPE_UNIT) {
                emit_body(ctx, "POPS GF@$concat$1\n");
                emit_body(ctx, "PUSHS nil@nil\n");
            }
        } else {
            emit_body(ctx, "PUSHS nil@nil\n");
        }

        // Print the jump to the end of the function
        emit_body(ctx, "JUMP $%s$end\n", ctx->current_func);
        break;
    }

    case AST_FUNC_CALL: {
        callFunc(ctx, node);

        // Auto discard last value on stack
        emit_body(ctx, "POPS GF@$concat$1\n");

        break;
    }

    default: {
        gen_expr(ctx, node);
        emit_body(ctx, "POPS GF@$concat$1\n");
        break;
    }
    }
}

/**
- @brief Generates IFJcode26 instructions for an expression AST node.
 *
- Evaluates the expression such that its result is left on top of the data stack.
- Applies implicit Int-to-Double conversion (INT2FLOATS) if flagged on the node.
 *
- @param ctx Pointer to the code generator context.
- @param node Pointer to the expression AST node (ignored if NULL).
 */
static void gen_expr(CodeGenContext *ctx, const AstNode *node) {
    // Skip empty node
    if (!node)
        return;

    // Process all types of nodes in an expression
    switch (node->type) {
    case AST_LIT_INT: {
        emit_body(ctx, "PUSHS int@%ld\n", node->lit_int);
        break;
    }

    case AST_LIT_DOUBLE: {
        emit_body(ctx, "PUSHS float@%a\n", node->lit_double);
        break;
    }

    case AST_LIT_STRING: {
        emit_body(ctx, "PUSHS ");
        emit_string(ctx, node->lit_string);
        sb_append_char(&ctx->body_buffer, '\n');
        break;
    }

    case AST_LIT_BOOL: {
        const char *boolean = node->lit_bool ? "true" : "false";
        emit_body(ctx, "PUSHS bool@%s\n", boolean);
        break;
    }

    case AST_LIT_UNIT: {
        emit_body(ctx, "PUSHS nil@nil\n");
        break;
    }

    case AST_IDENT: {
        // Find the variable, or call parameterless function if not found locally
        const size_t id = lookup_local_var(ctx, node->ident.name);
        if (id == 0)
            emit_body(ctx, "CALL %s\n", node->ident.name);
        else
            emit_body(ctx, "PUSHS LF@%s$%zu\n", node->ident.name, id);
        break;
    }

    case AST_BIN_OP: {
        // Generate the left expression
        gen_expr(ctx, node->bin_op.left);

        // Check and operation (here because if the left is false then we can't generate the right
        // expression)
        if (node->bin_op.op == OP_AND) {
            // Get the unique ID for the statement
            const size_t id = ++ctx->label_counter;
            emit_body(ctx, "PUSHS bool@false\n");
            emit_body(ctx, "JUMPIFEQS $and_false_%zu\n", id);

            // Generate right expression
            gen_expr(ctx, node->bin_op.right);
            emit_body(ctx, "JUMP $and_end_%zu\n", id);

            // Create a false end or a right_expression end
            emit_body(ctx, "LABEL $and_false_%zu\n", id);
            emit_body(ctx, "PUSHS bool@false\n");
            emit_body(ctx, "LABEL $and_end_%zu\n", id);
        }

        // Check or operation (here because if the left is true then we can skip the right
        // expression)
        else if (node->bin_op.op == OP_OR) {
            // Get the unique ID for the statement
            const size_t id = ++ctx->label_counter;
            emit_body(ctx, "PUSHS bool@true\n");
            emit_body(ctx, "JUMPIFEQS $or_true_%zu\n", id);

            // Generate right expression
            gen_expr(ctx, node->bin_op.right);
            emit_body(ctx, "JUMP $or_end_%zu\n", id);

            // Create a true end or a right_expression end
            emit_body(ctx, "LABEL $or_true_%zu\n", id);
            emit_body(ctx, "PUSHS bool@true\n");
            emit_body(ctx, "LABEL $or_end_%zu\n", id);
        }

        // Generate the right expression normally
        else {
            gen_expr(ctx, node->bin_op.right);
        }

        // Process the type of binary operation
        const char *op = "";
        switch (node->bin_op.op) {
        case OP_ADD:
            // Check if the type result type is string then we concat instead
            if (node->inferred_type == TYPE_STRING) {
                emit_body(ctx, "POPS GF@$concat$2\n");
                emit_body(ctx, "POPS GF@$concat$1\n");
                emit_body(ctx, "CONCAT GF@$concat$1 GF@$concat$1 GF@$concat$2\n");
                emit_body(ctx, "PUSHS GF@$concat$1\n");
            } else {
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
            // Check the result inferred type for the correct type of division
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

        // Emit the operation
        emit_body(ctx, "%s", op);
        break;
    }

    case AST_UN_OP: {
        // Process the unary operation
        switch (node->un_op.op) {
        // Negation works like this: (0 - x)
        case OP_NEG: {
            // Push the correct type of 0
            if (node->inferred_type == TYPE_DOUBLE)
                emit_body(ctx, "PUSHS float@0x0p+0\n");
            else
                emit_body(ctx, "PUSHS int@0\n");

            // Generate the expression
            gen_expr(ctx, node->un_op.operand);

            // Emit subtraction from 0
            emit_body(ctx, "SUBS\n");
            break;
        }
        case OP_POS: {
            gen_expr(ctx, node->un_op.operand);
            break;
        }
        case OP_NOT: {
            gen_expr(ctx, node->un_op.operand);
            emit_body(ctx, "NOTS\n");
            break;
        }
        default:
            break;
        }
        break;
    }

    case AST_FUNC_CALL: {
        callFunc(ctx, node);
        break;
    }

    case AST_BLOCK: {
        scope_enter(ctx);

        const size_t count = node->block.statements.count;
        if (count == 0) {
            emit_body(ctx, "PUSHS nil@nil\n");
        } else {
            // Execute all statements except the last one (discarding any values)
            for (size_t i = 0; i < count - 1; i++) {
                gen_stmt(ctx, node->block.statements.items[i]);
            }

            // Last item produces the block value
            const AstNode *last = node->block.statements.items[count - 1];
            if (last->type == AST_RETURN) {
                gen_stmt(ctx, last);
            } else if (is_stmt_only(last)) {
                gen_stmt(ctx, last);
                emit_body(ctx, "PUSHS nil@nil\n");
            } else {
                gen_expr(ctx, last);
            }
        }

        scope_leave(ctx);
        break;
    }

    case AST_RETURN: {
        gen_stmt(ctx, node);
        break;
    }

    case AST_IF: {
        // Get the unique ID for the statement
        const size_t id = ++ctx->label_counter;

        // Generate the body of an if statement
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

    case AST_MEMBER_ACCESS: {
        gen_expr(ctx, node->member_access.object);

        const char *member = node->member_access.member_name;
        if (strcmp(member, "toString") == 0) {
            emit_body(ctx, "CALL toStr\n");
        } else if (strcmp(member, "length") == 0) {
            emit_body(ctx, "CALL strLength\n");
        } else if (strcmp(member, "toInt") == 0) {
            emit_body(ctx, "CALL toInt\n");
        } else if (strcmp(member, "floor") == 0) {
            emit_body(ctx, "CALL floor\n");
        } else {
            error_throw_internal("Unsupported member '%s' in codegen.", member);
        }
        break;
    }

    default:
        break;
    }

    if (node->convert_to_double)
        emit_body(ctx, "INT2FLOATS\n");
}

void codegen_generate(const AstNode *program, FILE *out) {
    // Skip empty program
    if (program == nullptr || program->type != AST_PROGRAM || out == nullptr)
        return;

    // Create a code generator context
    CodeGenContext ctx = {};
    ctx.out            = out;

    // Print the starting statement
    fprintf(ctx.out, ".IFJcode26\n");

    // Print temporary global variables for concatanation or dumping
    fprintf(ctx.out, "DEFVAR GF@$concat$1\n");
    fprintf(ctx.out, "DEFVAR GF@$concat$2\n");

    // Find the main function
    const char *main_name = nullptr;
    for (size_t i = 0; i < program->program.functions.count; i++) {
        const AstNode *fn = program->program.functions.items[i];
        if (fn != nullptr && fn->type == AST_FUNC_DEF && fn->func_def.is_main) {
            main_name = fn->func_def.name;
            break;
        }
    }
    if (main_name == nullptr)
        error_throw_internal("Missing '@main' function in codegen.");

    // Call main function
    fprintf(out, "CALL %s\n", main_name);
    fprintf(out, "EXIT int@0\n");

    // Generate all fucntions
    for (size_t i = 0; i < program->program.functions.count; i++) {
        const AstNode *astNode = program->program.functions.items[i];
        if (astNode && astNode->type == AST_FUNC_DEF)
            gen_func_def(&ctx, astNode);
    }

    // Generate all builtin functions
    gen_builtins(ctx.out);

    // Free all alocated memory
    free(ctx.vars);
}
