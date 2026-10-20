#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include <stddef.h>

/* All AST Node kinds for the C subset */
typedef enum {
    NODE_TRANSLATION_UNIT = 0,
    NODE_FUNCTION_DEF,
    NODE_PARAM,
    NODE_VAR_DECL,
    NODE_BLOCK,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_DO_WHILE,
    NODE_SWITCH,
    NODE_CASE,
    NODE_DEFAULT,
    NODE_RETURN,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_EXPR_STMT,
    NODE_EMPTY_STMT,
    NODE_BINARY,
    NODE_UNARY,
    NODE_ASSIGN,
    NODE_CALL,
    NODE_INDEX,
    NODE_IDENTIFIER,
    NODE_INT_LITERAL,
    NODE_FLOAT_LITERAL,
    NODE_CHAR_LITERAL,
    NODE_STRING_LITERAL,
    NODE_TERNARY
} NodeKind;

typedef struct AstNode AstNode;
typedef struct AstNode Node;
struct Symbol;

/* Tagged union representing a syntax tree node */
struct AstNode {
    NodeKind kind;
    int line;
    int column;
    int end_line;
    bool is_parenthesized;
    AstNode *next;

    union {
        struct {
            AstNode *declarations;
        } translation_unit;

        struct {
            char *return_type;
            int pointer_depth;
            char *name;
            AstNode *params;
            AstNode *body;
        } function_def;

        struct {
            char *type_name;
            int pointer_depth;
            char *name;
            bool is_array;
            int array_size;
        } param;

        struct {
            char *type_name;
            int pointer_depth;
            char *name;
            bool is_array;
            int array_size;
            AstNode *init;
        } var_decl;

        struct {
            AstNode *statements;
        } block;

        struct {
            AstNode *condition;
            AstNode *then_branch;
            AstNode *else_branch;
        } if_stmt;

        struct {
            AstNode *condition;
            AstNode *body;
        } while_stmt;

        struct {
            AstNode *init;
            AstNode *condition;
            AstNode *step;
            AstNode *body;
        } for_stmt;

        struct {
            AstNode *body;
            AstNode *condition;
        } do_while;

        struct {
            AstNode *expr;
            AstNode *body;
        } switch_stmt;

        struct {
            AstNode *expr;
            AstNode *body;
        } case_stmt;

        struct {
            AstNode *body;
        } default_stmt;

        struct {
            AstNode *expr;
        } return_stmt;

        struct {
            AstNode *expr;
        } expr_stmt;

        struct {
            char op[8];
            AstNode *left;
            AstNode *right;
        } binary;

        struct {
            char op[8];
            AstNode *operand;
            bool is_postfix;
        } unary;

        struct {
            char op[8];
            AstNode *left;
            AstNode *right;
        } assign;

        struct {
            AstNode *callee;
            AstNode *args;
        } call;

        struct {
            AstNode *array;
            AstNode *index;
        } index;

        struct {
            char *name;
            struct Symbol *symbol;
        } identifier;

        struct {
            char *value;
        } literal;

        struct {
            AstNode *condition;
            AstNode *true_expr;
            AstNode *false_expr;
        } ternary;
    } data;
};

/* Creates an allocated AST node with given kind and source location */
AstNode *ast_create(NodeKind kind, int line, int column);

/* Recursively frees an AST node and all attached child nodes */
void ast_free(AstNode *node);

/* Prints the AST hierarchy as readable indented text */
void ast_dump(const AstNode *node, int indent);

/* Returns a human readable name for an AST node kind */
const char *ast_node_kind_name(NodeKind kind);

#endif /* AST_H */
