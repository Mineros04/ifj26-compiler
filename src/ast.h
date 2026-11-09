/**
 * @file ast.h
 * @brief Definice uzlů Abstraktního syntaktického stromu (AST) a jeho rozhraní.
 *
 * Projekt: Implementace překladače imperativního jazyka IFJ26
 * Autori: tbd
 */

#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Forward deklarace hlavního uzlu
typedef struct AstNode AstNode;

/**
 * @brief Datové typy jazyka IFJ26.
 */
typedef enum {
    TYPE_UNKNOWN = 0, // Typ nebyl uveden (bude odvozen sémantickou analýzou)
    TYPE_INT,         // Int (64-bit v cílovém kódu, 32-bit sémantika)
    TYPE_DOUBLE,      // Double (IEEE 754)
    TYPE_STRING,      // String
    TYPE_UNIT,        // Unit ()
    TYPE_BOOL         // Boolean (pro relační výrazy a rozšíření BOOLTHEN)
} DataType;

/**
 * @brief Operátory pro uzly výrazů (pro precedenční analýzu - Role C).
 */
typedef enum {
    OP_ADD, // + (sčítání nebo konkatenace)
    OP_SUB, // - (odčítání)
    OP_MUL, // *
    OP_DIV, // / (celočíselné i desetinné)
    OP_EQ,  // ==
    OP_NEQ, // !=
    OP_LT,  // <
    OP_LE,  // <=
    OP_GT,  // >
    OP_GE,  // >=
    OP_AND, // && (BOOLTHEN)
    OP_OR,  // || (BOOLTHEN)
    OP_NEG, // unární -
    OP_POS, // unární +
    OP_NOT  // unární ! (BOOLTHEN)
} OperatorType;

/**
 * @brief Druhy uzlů v AST.
 */
typedef enum {
    AST_PROGRAM,  // Kořen celého programu (seznam funkcí)
    AST_FUNC_DEF, // Definice funkce (def id(...) = ...)
    AST_PARAM,    // Formální parametr funkce (id: Typ)
    AST_BLOCK,    // Blok příkazů (tvoří nový scope)

    // Příkazy (Statements)
    AST_VAR_DECL, // Definice proměnné (val / var)
    AST_ASSIGN,   // Přiřazení (id = ...)
    AST_IF,       // Podmínka if-then-else (může být i výraz v CONTROL)
    AST_WHILE,    // Cyklus while
    AST_FOR,      // Cyklus for (rozšíření CONTROL)
    AST_RETURN,   // Návrat z funkce (return)

    // Výrazy a volání (Expressions & Calls)
    AST_FUNC_CALL,  // Volání funkce id(args...)
    AST_BIN_OP,     // Binární operátor (a + b, x < y, ...)
    AST_UN_OP,      // Unární operátor (-x, !b)
    AST_IDENT,      // Identifikátor proměnné ve výrazu
    AST_LIT_INT,    // Celočíselný literál
    AST_LIT_DOUBLE, // Desetinný literál
    AST_LIT_STRING, // Řetězcový literál
    AST_LIT_BOOL,   // Booleovský literál (true / false)
    AST_LIT_UNIT    // Literál () typu Unit
} AstNodeType;

/**
 * @brief Dynamické pole ukazatelů na uzly AST (pro seznamy příkazů, parametrů, argumentů).
 */
typedef struct {
    AstNode **items;
    size_t count;
    size_t capacity;
} AstNodeList;

/**
 * @brief Hlavní struktura uzlu AST.
 */
struct AstNode {
    AstNodeType type;       // Druh uzlu
    int line;               // Řádek ve zdrojovém kódu (pro chybová hlášení)
    DataType inferred_type; // Typ vypočtený sémantickou analýzou (Role C a D)
    bool convert_to_double; // Příznak pro codegen: uzel Int se má implicitně převést na Double

    union {
        // AST_PROGRAM
        struct {
            AstNodeList functions; // Seznam AST_FUNC_DEF
        } program;

        // AST_FUNC_DEF
        struct {
            char *name;           // Jméno funkce (alokovaný C řetězec)
            bool is_main;         // Má anotaci @main?
            AstNodeList params;   // Seznam AST_PARAM
            DataType return_type; // Explicitní návratový typ (nebo TYPE_UNKNOWN)
            AstNode *body;        // Tělo funkce (vždy AST_BLOCK)
        } func_def;

        // AST_PARAM
        struct {
            char *name;         // Jméno parametru
            DataType data_type; // Typ parametru
        } param;

        // AST_BLOCK
        struct {
            AstNodeList statements; // Seznam příkazů v bloku
        } block;

        // AST_VAR_DECL (val / var)
        struct {
            char *name;         // Jméno proměnné
            bool is_var;        // true = var (modifikovatelná), false = val (nemodifikovatelná)
            DataType var_type;  // Explicitní typ (nebo TYPE_UNKNOWN)
            AstNode *init_expr; // Inicializační výraz nebo volání funkce
        } var_decl;

        // AST_ASSIGN (id = ...)
        struct {
            char *target_name;   // Jméno cílové proměnné
            AstNode *value_expr; // Přiřazovaná hodnota (výraz nebo volání)
        } assign;

        // AST_IF (příkaz i výraz v CONTROL)
        struct {
            AstNode *condition;   // Podmínka (výraz)
            AstNode *then_branch; // Větev then (AST_BLOCK)
            AstNode *else_branch; // Větev else (AST_BLOCK, nebo NULL při BOOLTHEN bez else)
        } if_stmt;

        // AST_WHILE
        struct {
            AstNode *condition; // Podmínka (výraz)
            AstNode *body;      // Tělo cyklu (AST_BLOCK)
        } while_stmt;

        // AST_FOR (rozšíření CONTROL)
        struct {
            char *iter_name;    // Jméno řídicí proměnné
            AstNode *from_expr; // Počáteční hodnota rozsahu
            AstNode *to_expr;   // Koncová hodnota rozsahu
            AstNode *step_expr; // Krok (by ...), může být NULL
            bool is_until;      // true = until (exkluzivní), false = to (inkluzivní)
            AstNodeList guards; // Seznam podmínek stráží (výrazy za if)
            AstNode *body;      // Tělo cyklu (AST_BLOCK)
        } for_stmt;

        // AST_RETURN
        struct {
            AstNode *expr; // Návratová hodnota (nebo NULL pro samotné 'return')
        } return_stmt;

        // AST_FUNC_CALL
        struct {
            char *func_name;  // Jméno volané funkce
            AstNodeList args; // Seznam argumentů (výrazy / termy)
        } func_call;

        // AST_BIN_OP
        struct {
            OperatorType op; // Druh binárního operátoru
            AstNode *left;   // Levý operand
            AstNode *right;  // Pravý operand
        } bin_op;

        // AST_UN_OP
        struct {
            OperatorType op;  // Druh unárního operátoru
            AstNode *operand; // Operand
        } un_op;

        // AST_IDENT
        struct {
            char *name; // Jméno proměnné
        } ident;

        // Literály
        int64_t lit_int;   // AST_LIT_INT
        double lit_double; // AST_LIT_DOUBLE
        char *lit_string;  // AST_LIT_STRING (alokovaný C řetězec)
        bool lit_bool;     // AST_LIT_BOOL
    };
};

// Funkce pro práci se seznamy uzlů (AstNodeList)

void ast_list_init(AstNodeList *list);
void ast_list_append(AstNodeList *list, AstNode *node);

// Konstruktory uzlů AST

AstNode *ast_new_program(void);
AstNode *
ast_new_func_def(const char *name, bool is_main, DataType return_type, AstNode *body, int line);
AstNode *ast_new_param(const char *name, DataType data_type, int line);
AstNode *ast_new_block(int line);

AstNode *
ast_new_var_decl(const char *name, bool is_var, DataType var_type, AstNode *init_expr, int line);
AstNode *ast_new_assign(const char *target_name, AstNode *value_expr, int line);
AstNode *ast_new_if(AstNode *condition, AstNode *then_branch, AstNode *else_branch, int line);
AstNode *ast_new_while(AstNode *condition, AstNode *body, int line);
AstNode *ast_new_for(const char *iter_name,
                     AstNode *from_expr,
                     AstNode *to_expr,
                     AstNode *step_expr,
                     bool is_until,
                     AstNode *body,
                     int line);
AstNode *ast_new_return(AstNode *expr, int line);

AstNode *ast_new_func_call(const char *func_name, int line);
AstNode *ast_new_bin_op(OperatorType op, AstNode *left, AstNode *right, int line);
AstNode *ast_new_un_op(OperatorType op, AstNode *operand, int line);
AstNode *ast_new_ident(const char *name, int line);
AstNode *ast_new_lit_int(int64_t value, int line);
AstNode *ast_new_lit_double(double value, int line);
AstNode *ast_new_lit_string(const char *value, int line);
AstNode *ast_new_lit_bool(bool value, int line);
AstNode *ast_new_lit_unit(int line);

// Správa paměti

void ast_free(AstNode *node);

#endif // AST_H
