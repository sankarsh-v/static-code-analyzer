#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Allocates and initializes an AST node with zeroed payload */
AstNode *ast_create(NodeKind kind, int line, int column) {
    AstNode *node = calloc(1, sizeof(AstNode));
    if (!node) return NULL;
    node->kind = kind;
    node->line = line;
    node->column = column;
    node->end_line = line;
    node->next = NULL;
    return node;
}

/* Returns the string representation of an AST node kind */
const char *ast_node_kind_name(NodeKind kind) {
    switch (kind) {
        case NODE_TRANSLATION_UNIT: return "TranslationUnit";
        case NODE_FUNCTION_DEF: return "FunctionDef";
        case NODE_PARAM: return "Param";
        case NODE_VAR_DECL: return "VarDecl";
        case NODE_BLOCK: return "Block";
        case NODE_IF: return "If";
        case NODE_WHILE: return "While";
        case NODE_FOR: return "For";
        case NODE_DO_WHILE: return "DoWhile";
        case NODE_SWITCH: return "Switch";
        case NODE_CASE: return "Case";
        case NODE_DEFAULT: return "Default";
        case NODE_RETURN: return "Return";
        case NODE_BREAK: return "Break";
        case NODE_CONTINUE: return "Continue";
        case NODE_EXPR_STMT: return "ExprStmt";
        case NODE_EMPTY_STMT: return "EmptyStmt";
        case NODE_BINARY: return "Binary";
        case NODE_UNARY: return "Unary";
        case NODE_ASSIGN: return "Assign";
        case NODE_CALL: return "Call";
        case NODE_INDEX: return "Index";
        case NODE_IDENTIFIER: return "Identifier";
        case NODE_INT_LITERAL: return "IntLiteral";
        case NODE_FLOAT_LITERAL: return "FloatLiteral";
        case NODE_CHAR_LITERAL: return "CharLiteral";
        case NODE_STRING_LITERAL: return "StringLiteral";
        case NODE_TERNARY: return "Ternary";
        default: return "UnknownNode";
    }
}

/* Frees statement and control-flow payloads */
static void free_stmt_payload(AstNode *node) {
    switch (node->kind) {
        case NODE_BLOCK: ast_free(node->data.block.statements); break;
        case NODE_IF:
            ast_free(node->data.if_stmt.condition);
            ast_free(node->data.if_stmt.then_branch);
            ast_free(node->data.if_stmt.else_branch);
            break;
        case NODE_WHILE:
            ast_free(node->data.while_stmt.condition);
            ast_free(node->data.while_stmt.body);
            break;
        case NODE_FOR:
            ast_free(node->data.for_stmt.init);
            ast_free(node->data.for_stmt.condition);
            ast_free(node->data.for_stmt.step);
            ast_free(node->data.for_stmt.body);
            break;
        case NODE_DO_WHILE:
            ast_free(node->data.do_while.body);
            ast_free(node->data.do_while.condition);
            break;
        case NODE_SWITCH:
            ast_free(node->data.switch_stmt.expr);
            ast_free(node->data.switch_stmt.body);
            break;
        case NODE_CASE:
            ast_free(node->data.case_stmt.expr);
            ast_free(node->data.case_stmt.body);
            break;
        case NODE_DEFAULT: ast_free(node->data.default_stmt.body); break;
        case NODE_RETURN: ast_free(node->data.return_stmt.expr); break;
        case NODE_EXPR_STMT: ast_free(node->data.expr_stmt.expr); break;
        default: break;
    }
}

/* Frees expression node payloads */
static void free_expr_payload(AstNode *node) {
    switch (node->kind) {
        case NODE_BINARY:
            ast_free(node->data.binary.left);
            ast_free(node->data.binary.right);
            break;
        case NODE_UNARY: ast_free(node->data.unary.operand); break;
        case NODE_ASSIGN:
            ast_free(node->data.assign.left);
            ast_free(node->data.assign.right);
            break;
        case NODE_CALL:
            ast_free(node->data.call.callee);
            ast_free(node->data.call.args);
            break;
        case NODE_INDEX:
            ast_free(node->data.index.array);
            ast_free(node->data.index.index);
            break;
        case NODE_IDENTIFIER: free(node->data.identifier.name); break;
        case NODE_INT_LITERAL:
        case NODE_FLOAT_LITERAL:
        case NODE_CHAR_LITERAL:
        case NODE_STRING_LITERAL: free(node->data.literal.value); break;
        case NODE_TERNARY:
            ast_free(node->data.ternary.condition);
            ast_free(node->data.ternary.true_expr);
            ast_free(node->data.ternary.false_expr);
            break;
        default: break;
    }
}

/* Frees node payload union resources based on node kind */
static void ast_free_node_payload(AstNode *node) {
    switch (node->kind) {
        case NODE_TRANSLATION_UNIT:
            ast_free(node->data.translation_unit.declarations);
            break;
        case NODE_FUNCTION_DEF:
            free(node->data.function_def.return_type);
            free(node->data.function_def.name);
            ast_free(node->data.function_def.params);
            ast_free(node->data.function_def.body);
            break;
        case NODE_PARAM:
            free(node->data.param.type_name);
            free(node->data.param.name);
            break;
        case NODE_VAR_DECL:
            free(node->data.var_decl.type_name);
            free(node->data.var_decl.name);
            ast_free(node->data.var_decl.init);
            break;
        default:
            free_stmt_payload(node);
            free_expr_payload(node);
            break;
    }
}

/* Recursively frees an AST node and all linked siblings */
void ast_free(AstNode *node) {
    while (node) {
        AstNode *next = node->next;
        ast_free_node_payload(node);
        free(node);
        node = next;
    }
}

/* Prints leading spaces corresponding to indentation level */
static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) printf(" ");
}

/* Dumps function definition and variable declaration node headers */
static void dump_decl_node(const AstNode *node, int indent) {
    print_indent(indent);
    if (node->kind == NODE_FUNCTION_DEF) {
        printf("FunctionDef %s %s%s (line %d:%d)\n",
               node->data.function_def.return_type,
               node->data.function_def.name,
               node->data.function_def.body ? "" : " [prototype]",
               node->line, node->column);
        if (node->data.function_def.params) {
            print_indent(indent + 2); printf("Params:\n");
            ast_dump(node->data.function_def.params, indent + 4);
        }
        if (node->data.function_def.body) {
            print_indent(indent + 2); printf("Body:\n");
            ast_dump(node->data.function_def.body, indent + 4);
        }
    } else if (node->kind == NODE_VAR_DECL) {
        printf("VarDecl %s%s %s%s (line %d:%d)\n",
               node->data.var_decl.type_name,
               node->data.var_decl.pointer_depth > 0 ? "*" : "",
               node->data.var_decl.name,
               node->data.var_decl.is_array ? "[]" : "",
               node->line, node->column);
        if (node->data.var_decl.init) {
            ast_dump(node->data.var_decl.init, indent + 2);
        }
    } else if (node->kind == NODE_PARAM) {
        printf("Param %s%s %s (line %d:%d)\n",
               node->data.param.type_name,
               node->data.param.pointer_depth > 0 ? "*" : "",
               node->data.param.name ? node->data.param.name : "",
               node->line, node->column);
    }
}

/* Dumps loop statement subtrees for for, while, and do-while nodes */
static void dump_loop_stmt_node(const AstNode *node, int indent) {
    if (node->kind == NODE_WHILE) {
        print_indent(indent + 2); printf("Condition:\n");
        ast_dump(node->data.while_stmt.condition, indent + 4);
        print_indent(indent + 2); printf("Body:\n");
        ast_dump(node->data.while_stmt.body, indent + 4);
    } else if (node->kind == NODE_FOR) {
        if (node->data.for_stmt.init) {
            print_indent(indent + 2); printf("Init:\n");
            ast_dump(node->data.for_stmt.init, indent + 4);
        }
        if (node->data.for_stmt.condition) {
            print_indent(indent + 2); printf("Condition:\n");
            ast_dump(node->data.for_stmt.condition, indent + 4);
        }
        if (node->data.for_stmt.step) {
            print_indent(indent + 2); printf("Step:\n");
            ast_dump(node->data.for_stmt.step, indent + 4);
        }
        print_indent(indent + 2); printf("Body:\n");
        ast_dump(node->data.for_stmt.body, indent + 4);
    } else if (node->kind == NODE_DO_WHILE) {
        print_indent(indent + 2); printf("Body:\n");
        ast_dump(node->data.do_while.body, indent + 4);
        print_indent(indent + 2); printf("Condition:\n");
        ast_dump(node->data.do_while.condition, indent + 4);
    }
}

/* Dumps statement nodes with their sub-trees */
static void dump_stmt_node(const AstNode *node, int indent) {
    print_indent(indent);
    printf("%s (line %d:%d)\n", ast_node_kind_name(node->kind), node->line, node->column);
    switch (node->kind) {
        case NODE_BLOCK: ast_dump(node->data.block.statements, indent + 2); break;
        case NODE_IF:
            print_indent(indent + 2); printf("Condition:\n");
            ast_dump(node->data.if_stmt.condition, indent + 4);
            print_indent(indent + 2); printf("Then:\n");
            ast_dump(node->data.if_stmt.then_branch, indent + 4);
            if (node->data.if_stmt.else_branch) {
                print_indent(indent + 2); printf("Else:\n");
                ast_dump(node->data.if_stmt.else_branch, indent + 4);
            }
            break;
        case NODE_WHILE:
        case NODE_FOR:
        case NODE_DO_WHILE:
            dump_loop_stmt_node(node, indent);
            break;
        case NODE_SWITCH:
            print_indent(indent + 2); printf("Expr:\n");
            ast_dump(node->data.switch_stmt.expr, indent + 4);
            print_indent(indent + 2); printf("Body:\n");
            ast_dump(node->data.switch_stmt.body, indent + 4);
            break;
        case NODE_CASE:
            print_indent(indent + 2); printf("Expr:\n");
            ast_dump(node->data.case_stmt.expr, indent + 4);
            if (node->data.case_stmt.body) {
                print_indent(indent + 2); printf("Body:\n");
                ast_dump(node->data.case_stmt.body, indent + 4);
            }
            break;
        case NODE_DEFAULT:
            if (node->data.default_stmt.body) {
                print_indent(indent + 2); printf("Body:\n");
                ast_dump(node->data.default_stmt.body, indent + 4);
            }
            break;
        case NODE_RETURN:
            if (node->data.return_stmt.expr) ast_dump(node->data.return_stmt.expr, indent + 2);
            break;
        case NODE_EXPR_STMT:
            if (node->data.expr_stmt.expr) ast_dump(node->data.expr_stmt.expr, indent + 2);
            break;
        default: break;
    }
}

/* Dumps expression nodes including binary, unary, and literals */
static void dump_expr_node(const AstNode *node, int indent) {
    print_indent(indent);
    switch (node->kind) {
        case NODE_TERNARY:
            printf("Ternary (line %d:%d)\n", node->line, node->column);
            print_indent(indent + 2); printf("Condition:\n");
            ast_dump(node->data.ternary.condition, indent + 4);
            print_indent(indent + 2); printf("True:\n");
            ast_dump(node->data.ternary.true_expr, indent + 4);
            print_indent(indent + 2); printf("False:\n");
            ast_dump(node->data.ternary.false_expr, indent + 4);
            break;
        case NODE_BINARY:
            printf("Binary '%s' (line %d:%d)\n", node->data.binary.op, node->line, node->column);
            ast_dump(node->data.binary.left, indent + 2);
            ast_dump(node->data.binary.right, indent + 2);
            break;
        case NODE_ASSIGN:
            printf("Assign '%s' (line %d:%d)\n", node->data.assign.op, node->line, node->column);
            ast_dump(node->data.assign.left, indent + 2);
            ast_dump(node->data.assign.right, indent + 2);
            break;
        case NODE_UNARY:
            printf("Unary '%s'%s (line %d:%d)\n", node->data.unary.op,
                   node->data.unary.is_postfix ? " [postfix]" : " [prefix]",
                   node->line, node->column);
            ast_dump(node->data.unary.operand, indent + 2);
            break;
        case NODE_CALL:
            printf("Call (line %d:%d)\n", node->line, node->column);
            print_indent(indent + 2); printf("Callee:\n");
            ast_dump(node->data.call.callee, indent + 4);
            if (node->data.call.args) {
                print_indent(indent + 2); printf("Args:\n");
                ast_dump(node->data.call.args, indent + 4);
            }
            break;
        case NODE_INDEX:
            printf("Index (line %d:%d)\n", node->line, node->column);
            ast_dump(node->data.index.array, indent + 2);
            ast_dump(node->data.index.index, indent + 2);
            break;
        case NODE_IDENTIFIER:
            printf("Identifier '%s' (line %d:%d)\n", node->data.identifier.name, node->line, node->column);
            break;
        case NODE_INT_LITERAL:
        case NODE_FLOAT_LITERAL:
        case NODE_CHAR_LITERAL:
        case NODE_STRING_LITERAL:
            printf("%s '%s' (line %d:%d)\n", ast_node_kind_name(node->kind),
                   node->data.literal.value, node->line, node->column);
            break;
        default:
            printf("%s (line %d:%d)\n", ast_node_kind_name(node->kind), node->line, node->column);
            break;
    }
}

/* Recursively prints an AST node tree in human readable indented format */
void ast_dump(const AstNode *node, int indent) {
    while (node) {
        if (node->kind == NODE_TRANSLATION_UNIT) {
            print_indent(indent);
            printf("TranslationUnit (line %d:%d)\n", node->line, node->column);
            ast_dump(node->data.translation_unit.declarations, indent + 2);
        } else if (node->kind == NODE_FUNCTION_DEF || node->kind == NODE_VAR_DECL || node->kind == NODE_PARAM) {
            dump_decl_node(node, indent);
        } else if (node->kind >= NODE_BLOCK && node->kind <= NODE_EMPTY_STMT) {
            dump_stmt_node(node, indent);
        } else {
            dump_expr_node(node, indent);
        }
        node = node->next;
    }
}
