#ifndef CHECKS_H
#define CHECKS_H

#include "ast.h"
#include "diag.h"
#include "symtab.h"
#include <stdbool.h>

/* Configurable thresholds for static analysis checks */
typedef struct {
    int max_nesting_depth;  /* Maximum statement nesting depth (default 3) */
    int max_function_lines; /* Maximum function body length in lines (default 40) */
} ChecksConfig;

/* Central configuration instance */
extern ChecksConfig g_checks_config;

/* Function pointer signature for an AST/symbol rule check */
typedef void (*CheckRuleFn)(const AstNode *root, const SymbolTable *st, DiagList *diags);

/* Definition entry in the static checks rule table */
typedef struct {
    const char *id;
    const char *name;
    DiagSeverity severity;
    const char *description;
    CheckRuleFn fn;
    bool enabled;
} Rule;

/* Runs all enabled static quality and safety checks over AST and SymbolTable */
void checks_run(const AstNode *root, const SymbolTable *st, DiagList *diags);

/* Rule W001: Local variable declared but never read, or assigned but never read */
void check_w001_unused_variables(const AstNode *root, const SymbolTable *st, DiagList *diags);

/* Rule W002: Assignment operator '=' used as condition without extra parentheses */
void check_w002_assignment_in_condition(const AstNode *root, const SymbolTable *st, DiagList *diags);

/* Rule W003: Unreachable statements following return, break, or continue in block */
void check_w003_unreachable_code(const AstNode *root, const SymbolTable *st, DiagList *diags);

/* Rule W004: Non-void function where control flow can reach end of body without return */
void check_w004_missing_return(const AstNode *root, const SymbolTable *st, DiagList *diags);

/* Rule W005: Local scalar variable read before any prior assignment (heuristic) */
void check_w005_use_before_assignment(const AstNode *root, const SymbolTable *st, DiagList *diags);

/* Rule W006: Numeric literals other than 0, 1, 2, -1 in expressions or array index */
void check_w006_magic_numbers(const AstNode *root, const SymbolTable *st, DiagList *diags);

/* Rule W007: Nesting depth of if/loop/switch statements exceeds threshold */
void check_w007_deep_nesting(const AstNode *root, const SymbolTable *st, DiagList *diags);

/* Rule W008: Function body span in lines exceeds threshold */
void check_w008_long_function(const AstNode *root, const SymbolTable *st, DiagList *diags);

/* Rule W009: Control flow statement with an empty block or lone semicolon */
void check_w009_empty_body(const AstNode *root, const SymbolTable *st, DiagList *diags);

/* Rule W010: Switch statement without a default case label */
void check_w010_switch_without_default(const AstNode *root, const SymbolTable *st, DiagList *diags);

/* Rule W011: Calls to unsafe C standard library functions */
void check_w011_unsafe_functions(const AstNode *root, const SymbolTable *st, DiagList *diags);

/* Rule W012: Local variable or parameter shadowing an enclosing scope variable */
void check_w012_variable_shadowing(const AstNode *root, const SymbolTable *st, DiagList *diags);

/* Rule W013: Division or modulo operation with integer literal 0 on right-hand side */
void check_w013_division_by_zero(const AstNode *root, const SymbolTable *st, DiagList *diags);

/* Rule W014: Switch case label with statements falling through without break/return */
void check_w014_fall_through_case(const AstNode *root, const SymbolTable *st, DiagList *diags);

#endif /* CHECKS_H */
