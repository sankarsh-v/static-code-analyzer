#include "checks.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global configuration thresholds for quality and complexity checks */
ChecksConfig g_checks_config = {
    .max_nesting_depth = 3,
    .max_function_lines = 40,
};

/* Rule W001: Detects local variables declared but never read, or assigned but never read */
void check_w001_unused_variables(const AstNode *root, const SymbolTable *st, DiagList *diags) {
    (void)root;
    for (size_t i = 0; i < st->all_scopes_count; i++) {
        Scope *sc = st->all_scopes[i];
        if (sc->level == 0) continue;
        for (Symbol *s = sc->symbols; s != NULL; s = s->next) {
            if (s->kind != SYM_VARIABLE || s->is_global) continue;
            if (!s->used && !s->assigned) {
                char msg[128];
                snprintf(msg, sizeof(msg), "unused variable '%s'", s->name);
                diag_add(diags, "W001", DIAG_WARNING, s->line, s->column, msg, "remove or use this variable");
            } else if (!s->used && s->assigned) {
                char msg[128];
                snprintf(msg, sizeof(msg), "variable '%s' is assigned but never read", s->name);
                diag_add(diags, "W001", DIAG_INFO, s->line, s->column, msg, "consider removing if unnecessary");
            }
        }
    }
}

/* Validates that a condition expression is not an unparenthesized '=' assignment */
static void inspect_condition(const AstNode *cond, DiagList *diags) {
    if (!cond) return;
    if (cond->kind == NODE_ASSIGN && strcmp(cond->data.assign.op, "=") == 0 && !cond->is_parenthesized) {
        diag_add(diags, "W002", DIAG_WARNING, cond->line, cond->column,
                 "assignment in condition", "did you mean '=='?");
    }
}

/* Recursively walks AST nodes to find condition expressions for W002 */
static void walk_conditions(const AstNode *node, DiagList *diags) {
    if (!node) return;
    if (node->kind == NODE_IF) {
        inspect_condition(node->data.if_stmt.condition, diags);
        walk_conditions(node->data.if_stmt.then_branch, diags);
        walk_conditions(node->data.if_stmt.else_branch, diags);
    } else if (node->kind == NODE_WHILE) {
        inspect_condition(node->data.while_stmt.condition, diags);
        walk_conditions(node->data.while_stmt.body, diags);
    } else if (node->kind == NODE_DO_WHILE) {
        inspect_condition(node->data.do_while.condition, diags);
        walk_conditions(node->data.do_while.body, diags);
    } else if (node->kind == NODE_FOR) {
        inspect_condition(node->data.for_stmt.condition, diags);
        walk_conditions(node->data.for_stmt.init, diags);
        walk_conditions(node->data.for_stmt.body, diags);
    } else if (node->kind == NODE_TERNARY) {
        inspect_condition(node->data.ternary.condition, diags);
        walk_conditions(node->data.ternary.true_expr, diags);
        walk_conditions(node->data.ternary.false_expr, diags);
    } else if (node->kind == NODE_BLOCK) {
        for (const AstNode *s = node->data.block.statements; s != NULL; s = s->next) walk_conditions(s, diags);
    } else if (node->kind == NODE_FUNCTION_DEF) {
        walk_conditions(node->data.function_def.body, diags);
    } else if (node->kind == NODE_TRANSLATION_UNIT) {
        for (const AstNode *d = node->data.translation_unit.declarations; d != NULL; d = d->next) walk_conditions(d, diags);
    }
}

/* Rule W002: Checks if condition expression is an unparenthesized '=' assignment */
void check_w002_assignment_in_condition(const AstNode *root, const SymbolTable *st, DiagList *diags) {
    (void)st;
    walk_conditions(root, diags);
}

/* Checks recursively if a statement or block contains a break statement */
static bool contains_break(const AstNode *node) {
    if (!node) return false;
    if (node->kind == NODE_BREAK) return true;
    if (node->kind == NODE_BLOCK) {
        for (const AstNode *s = node->data.block.statements; s != NULL; s = s->next) {
            if (contains_break(s)) return true;
        }
    } else if (node->kind == NODE_IF) {
        return contains_break(node->data.if_stmt.then_branch) ||
               contains_break(node->data.if_stmt.else_branch);
    }
    return false;
}

/* Returns true if a statement unconditionally terminates block execution */
static bool stmt_always_terminates(const AstNode *stmt) {
    if (!stmt) return false;
    if (stmt->kind == NODE_RETURN || stmt->kind == NODE_BREAK || stmt->kind == NODE_CONTINUE) return true;
    if (stmt->kind == NODE_IF) {
        if (stmt->data.if_stmt.then_branch && stmt->data.if_stmt.else_branch) {
            return stmt_always_terminates(stmt->data.if_stmt.then_branch) &&
                   stmt_always_terminates(stmt->data.if_stmt.else_branch);
        }
        return false;
    }
    if (stmt->kind == NODE_BLOCK) {
        for (const AstNode *s = stmt->data.block.statements; s != NULL; s = s->next) {
            if (stmt_always_terminates(s)) return true;
        }
        return false;
    }
    if (stmt->kind == NODE_WHILE) {
        const AstNode *cond = stmt->data.while_stmt.condition;
        if (cond && cond->kind == NODE_INT_LITERAL && strtol(cond->data.literal.value, NULL, 0) != 0) {
            return !contains_break(stmt->data.while_stmt.body);
        }
        return false;
    }
    if (stmt->kind == NODE_FOR) {
        if (!stmt->data.for_stmt.condition) return !contains_break(stmt->data.for_stmt.body);
        return false;
    }
    return false;
}

/* Scans block statements for unreachable code following a terminating statement */
static void scan_block_reachability(const AstNode *block, DiagList *diags) {
    bool terminated = false;
    bool reported = false;
    for (const AstNode *s = block->data.block.statements; s != NULL; s = s->next) {
        if (s->kind == NODE_CASE || s->kind == NODE_DEFAULT) {
            terminated = false;
            reported = false;
            continue;
        }
        if (terminated && !reported) {
            diag_add(diags, "W003", DIAG_WARNING, s->line, s->column,
                     "unreachable code", "remove unreachable statements");
            reported = true;
        }
        if (stmt_always_terminates(s)) terminated = true;
    }
}

/* Recursively walks AST to check statement reachability inside each block */
static void walk_reachability(const AstNode *node, DiagList *diags) {
    if (!node) return;
    if (node->kind == NODE_BLOCK) {
        scan_block_reachability(node, diags);
        for (const AstNode *s = node->data.block.statements; s != NULL; s = s->next) walk_reachability(s, diags);
    } else if (node->kind == NODE_IF) {
        walk_reachability(node->data.if_stmt.then_branch, diags);
        walk_reachability(node->data.if_stmt.else_branch, diags);
    } else if (node->kind == NODE_WHILE) {
        walk_reachability(node->data.while_stmt.body, diags);
    } else if (node->kind == NODE_FOR) {
        walk_reachability(node->data.for_stmt.body, diags);
    } else if (node->kind == NODE_DO_WHILE) {
        walk_reachability(node->data.do_while.body, diags);
    } else if (node->kind == NODE_SWITCH) {
        walk_reachability(node->data.switch_stmt.body, diags);
    } else if (node->kind == NODE_FUNCTION_DEF) {
        walk_reachability(node->data.function_def.body, diags);
    } else if (node->kind == NODE_TRANSLATION_UNIT) {
        for (const AstNode *d = node->data.translation_unit.declarations; d != NULL; d = d->next) walk_reachability(d, diags);
    }
}

/* Rule W003: Detects unreachable statements following return, break, or continue */
void check_w003_unreachable_code(const AstNode *root, const SymbolTable *st, DiagList *diags) {
    (void)st;
    walk_reachability(root, diags);
}

/* Rule W004: Detects non-void functions that can finish without returning a value */
void check_w004_missing_return(const AstNode *root, const SymbolTable *st, DiagList *diags) {
    (void)st;
    if (!root || root->kind != NODE_TRANSLATION_UNIT) return;
    for (const AstNode *d = root->data.translation_unit.declarations; d != NULL; d = d->next) {
        if (d->kind != NODE_FUNCTION_DEF || !d->data.function_def.body) continue;
        const char *ret_type = d->data.function_def.return_type;
        if (ret_type && strcmp(ret_type, "void") == 0 && d->data.function_def.pointer_depth == 0) continue;
        if (strcmp(d->data.function_def.name, "main") == 0) continue;
        if (!stmt_always_terminates(d->data.function_def.body)) {
            char msg[128];
            snprintf(msg, sizeof(msg), "control reaches end of non-void function '%s'", d->data.function_def.name);
            diag_add(diags, "W004", DIAG_WARNING, d->line, d->column, msg, "add a return statement along all code paths");
        }
    }
}

/* Tracking record for variable assignment status in heuristic checks */
typedef struct {
    char name[64];
    bool assigned;
} VarStateEntry;

/* Collection of variable assignment states for local scope checking */
typedef struct {
    VarStateEntry entries[128];
    size_t count;
} VarState;

/* Records that a named variable has received an assignment */
static void var_mark_assigned(VarState *vs, const char *name) {
    if (!name) return;
    for (size_t i = 0; i < vs->count; i++) {
        if (strcmp(vs->entries[i].name, name) == 0) {
            vs->entries[i].assigned = true;
            return;
        }
    }
    if (vs->count < 128) {
        strncpy(vs->entries[vs->count].name, name, sizeof(vs->entries[0].name) - 1);
        vs->entries[vs->count].assigned = true;
        vs->count++;
    }
}

/* Checks if a variable has been assigned prior to current read */
static bool var_is_assigned(const VarState *vs, const char *name) {
    if (!name) return true;
    for (size_t i = 0; i < vs->count; i++) {
        if (strcmp(vs->entries[i].name, name) == 0) return vs->entries[i].assigned;
    }
    return false;
}

/* Scans expression tree for variable reads prior to any assignment */
static void scan_expr_use_before_assign(const AstNode *node, VarState *vs, DiagList *diags) {
    if (!node) return;
    if (node->kind == NODE_IDENTIFIER) {
        Symbol *s = node->data.identifier.symbol;
        if (s && s->kind == SYM_VARIABLE && !s->is_global && s->scope_level > 0 && !s->initialized_at_declaration) {
            if (!var_is_assigned(vs, s->name)) {
                char msg[128];
                snprintf(msg, sizeof(msg), "variable '%s' may be used uninitialized", s->name);
                diag_add(diags, "W005", DIAG_WARNING, node->line, node->column, msg, "initialize variable before using it");
            }
        }
    } else if (node->kind == NODE_ASSIGN) {
        scan_expr_use_before_assign(node->data.assign.right, vs, diags);
        if (node->data.assign.left->kind == NODE_IDENTIFIER) {
            var_mark_assigned(vs, node->data.assign.left->data.identifier.name);
        } else {
            scan_expr_use_before_assign(node->data.assign.left, vs, diags);
        }
    } else if (node->kind == NODE_BINARY) {
        scan_expr_use_before_assign(node->data.binary.left, vs, diags);
        scan_expr_use_before_assign(node->data.binary.right, vs, diags);
    } else if (node->kind == NODE_UNARY) {
        scan_expr_use_before_assign(node->data.unary.operand, vs, diags);
    } else if (node->kind == NODE_CALL) {
        for (const AstNode *a = node->data.call.args; a != NULL; a = a->next) scan_expr_use_before_assign(a, vs, diags);
    } else if (node->kind == NODE_INDEX) {
        scan_expr_use_before_assign(node->data.index.array, vs, diags);
        scan_expr_use_before_assign(node->data.index.index, vs, diags);
    } else if (node->kind == NODE_TERNARY) {
        scan_expr_use_before_assign(node->data.ternary.condition, vs, diags);
        scan_expr_use_before_assign(node->data.ternary.true_expr, vs, diags);
        scan_expr_use_before_assign(node->data.ternary.false_expr, vs, diags);
    }
}

/* Scans statements for flow-insensitive use-before-assignment heuristic */
static void scan_stmt_use_before_assign(const AstNode *node, VarState *vs, DiagList *diags) {
    if (!node) return;
    if (node->kind == NODE_VAR_DECL) {
        if (node->data.var_decl.init) scan_expr_use_before_assign(node->data.var_decl.init, vs, diags);
        if (node->data.var_decl.init) var_mark_assigned(vs, node->data.var_decl.name);
    } else if (node->kind == NODE_EXPR_STMT) {
        scan_expr_use_before_assign(node->data.expr_stmt.expr, vs, diags);
    } else if (node->kind == NODE_RETURN) {
        scan_expr_use_before_assign(node->data.return_stmt.expr, vs, diags);
    } else if (node->kind == NODE_IF) {
        scan_expr_use_before_assign(node->data.if_stmt.condition, vs, diags);
        scan_stmt_use_before_assign(node->data.if_stmt.then_branch, vs, diags);
        scan_stmt_use_before_assign(node->data.if_stmt.else_branch, vs, diags);
    } else if (node->kind == NODE_WHILE) {
        scan_expr_use_before_assign(node->data.while_stmt.condition, vs, diags);
        scan_stmt_use_before_assign(node->data.while_stmt.body, vs, diags);
    } else if (node->kind == NODE_FOR) {
        scan_stmt_use_before_assign(node->data.for_stmt.init, vs, diags);
        scan_expr_use_before_assign(node->data.for_stmt.condition, vs, diags);
        scan_expr_use_before_assign(node->data.for_stmt.step, vs, diags);
        scan_stmt_use_before_assign(node->data.for_stmt.body, vs, diags);
    } else if (node->kind == NODE_BLOCK) {
        for (const AstNode *s = node->data.block.statements; s != NULL; s = s->next) scan_stmt_use_before_assign(s, vs, diags);
    }
}

/* Rule W005: Flags local scalar variables read before assignment (flow-insensitive heuristic) */
void check_w005_use_before_assignment(const AstNode *root, const SymbolTable *st, DiagList *diags) {
    (void)st;
    if (!root || root->kind != NODE_TRANSLATION_UNIT) return;
    for (const AstNode *d = root->data.translation_unit.declarations; d != NULL; d = d->next) {
        if (d->kind == NODE_FUNCTION_DEF && d->data.function_def.body) {
            VarState vs;
            memset(&vs, 0, sizeof(VarState));
            scan_stmt_use_before_assign(d->data.function_def.body, &vs, diags);
        }
    }
}

/* Checks if an integer literal text represents allowed numeric constants 0, 1, or 2 */
static bool is_allowed_int_lit(const char *str) {
    if (!str) return false;
    char *end = NULL;
    long val = strtol(str, &end, 0);
    return (end && *end == '\0' && (val == 0 || val == 1 || val == 2));
}

/* Checks if a float literal text represents allowed numeric constants 0.0, 1.0, or 2.0 */
static bool is_allowed_float_lit(const char *str) {
    if (!str) return false;
    char *end = NULL;
    double val = strtod(str, &end);
    if (!end) return false;
    if (*end == 'f' || *end == 'F') end++;
    return (*end == '\0' && (val == 0.0 || val == 1.0 || val == 2.0));
}

/* Checks unary minus operand for allowed -1 or disallowed negative magic numbers */
static bool inspect_unary_minus_magic(const AstNode *node, DiagList *diags) {
    if (node->kind != NODE_UNARY || strcmp(node->data.unary.op, "-") != 0) return false;
    const AstNode *operand = node->data.unary.operand;
    if (operand && operand->kind == NODE_INT_LITERAL) {
        long val = strtol(operand->data.literal.value, NULL, 0);
        if (val == 1) return true;
        char msg[64];
        snprintf(msg, sizeof(msg), "magic number '-%s'", operand->data.literal.value);
        diag_add(diags, "W006", DIAG_INFO, node->line, node->column, msg,
                 "define a named constant or enum instead of using magic numbers");
        return true;
    }
    if (operand && operand->kind == NODE_FLOAT_LITERAL) {
        double val = strtod(operand->data.literal.value, NULL);
        if (val == 1.0) return true;
        char msg[64];
        snprintf(msg, sizeof(msg), "magic number '-%s'", operand->data.literal.value);
        diag_add(diags, "W006", DIAG_INFO, node->line, node->column, msg,
                 "define a named constant or enum instead of using magic numbers");
        return true;
    }
    return false;
}

/* Traverses expression nodes to detect magic numbers outside exempt initializers */
static void scan_expr_magic_numbers(const AstNode *node, DiagList *diags) {
    if (!node) return;
    if (node->kind == NODE_UNARY && strcmp(node->data.unary.op, "-") == 0) {
        if (inspect_unary_minus_magic(node, diags)) return;
        scan_expr_magic_numbers(node->data.unary.operand, diags);
        return;
    }
    if (node->kind == NODE_INT_LITERAL) {
        if (!is_allowed_int_lit(node->data.literal.value)) {
            char msg[64];
            snprintf(msg, sizeof(msg), "magic number '%s'", node->data.literal.value);
            diag_add(diags, "W006", DIAG_INFO, node->line, node->column, msg,
                     "define a named constant or enum instead of using magic numbers");
        }
        return;
    }
    if (node->kind == NODE_FLOAT_LITERAL) {
        if (!is_allowed_float_lit(node->data.literal.value)) {
            char msg[64];
            snprintf(msg, sizeof(msg), "magic number '%s'", node->data.literal.value);
            diag_add(diags, "W006", DIAG_INFO, node->line, node->column, msg,
                     "define a named constant or enum instead of using magic numbers");
        }
        return;
    }
    if (node->kind == NODE_BINARY) {
        scan_expr_magic_numbers(node->data.binary.left, diags);
        scan_expr_magic_numbers(node->data.binary.right, diags);
    } else if (node->kind == NODE_UNARY) {
        scan_expr_magic_numbers(node->data.unary.operand, diags);
    } else if (node->kind == NODE_ASSIGN) {
        scan_expr_magic_numbers(node->data.assign.left, diags);
        scan_expr_magic_numbers(node->data.assign.right, diags);
    } else if (node->kind == NODE_CALL) {
        scan_expr_magic_numbers(node->data.call.callee, diags);
        for (const AstNode *a = node->data.call.args; a; a = a->next) scan_expr_magic_numbers(a, diags);
    } else if (node->kind == NODE_INDEX) {
        scan_expr_magic_numbers(node->data.index.array, diags);
        scan_expr_magic_numbers(node->data.index.index, diags);
    } else if (node->kind == NODE_TERNARY) {
        scan_expr_magic_numbers(node->data.ternary.condition, diags);
        scan_expr_magic_numbers(node->data.ternary.true_expr, diags);
        scan_expr_magic_numbers(node->data.ternary.false_expr, diags);
    }
}

/* Walks statement AST nodes checking expressions while exempting variable initializers */
static void walk_magic_numbers(const AstNode *node, DiagList *diags) {
    if (!node) return;
    if (node->kind == NODE_VAR_DECL) {
        /* Variable declaration initializers and array sizes are explicitly exempt */
        return;
    }
    if (node->kind == NODE_EXPR_STMT) {
        scan_expr_magic_numbers(node->data.expr_stmt.expr, diags);
    } else if (node->kind == NODE_RETURN) {
        scan_expr_magic_numbers(node->data.return_stmt.expr, diags);
    } else if (node->kind == NODE_IF) {
        scan_expr_magic_numbers(node->data.if_stmt.condition, diags);
        walk_magic_numbers(node->data.if_stmt.then_branch, diags);
        walk_magic_numbers(node->data.if_stmt.else_branch, diags);
    } else if (node->kind == NODE_WHILE) {
        scan_expr_magic_numbers(node->data.while_stmt.condition, diags);
        walk_magic_numbers(node->data.while_stmt.body, diags);
    } else if (node->kind == NODE_DO_WHILE) {
        scan_expr_magic_numbers(node->data.do_while.condition, diags);
        walk_magic_numbers(node->data.do_while.body, diags);
    } else if (node->kind == NODE_FOR) {
        if (node->data.for_stmt.init && node->data.for_stmt.init->kind != NODE_VAR_DECL) {
            scan_expr_magic_numbers(node->data.for_stmt.init, diags);
        }
        scan_expr_magic_numbers(node->data.for_stmt.condition, diags);
        scan_expr_magic_numbers(node->data.for_stmt.step, diags);
        walk_magic_numbers(node->data.for_stmt.body, diags);
    } else if (node->kind == NODE_SWITCH) {
        scan_expr_magic_numbers(node->data.switch_stmt.expr, diags);
        walk_magic_numbers(node->data.switch_stmt.body, diags);
    } else if (node->kind == NODE_CASE) {
        scan_expr_magic_numbers(node->data.case_stmt.expr, diags);
    } else if (node->kind == NODE_BLOCK) {
        for (const AstNode *s = node->data.block.statements; s; s = s->next) walk_magic_numbers(s, diags);
    } else if (node->kind == NODE_FUNCTION_DEF) {
        walk_magic_numbers(node->data.function_def.body, diags);
    } else if (node->kind == NODE_TRANSLATION_UNIT) {
        for (const AstNode *d = node->data.translation_unit.declarations; d; d = d->next) walk_magic_numbers(d, diags);
    }
}

/* Rule W006: Flags numeric literals other than 0, 1, 2, -1 appearing outside initializers */
void check_w006_magic_numbers(const AstNode *root, const SymbolTable *st, DiagList *diags) {
    (void)st;
    walk_magic_numbers(root, diags);
}

/* Tracking state for identifying maximum nesting depth in a function */
typedef struct {
    int max_depth;
    int deepest_line;
    int deepest_col;
} NestingTracker;

/* Recursively computes statement nesting depth and records deepest point */
static void walk_nesting(const AstNode *node, int current_depth, NestingTracker *tracker) {
    if (!node) return;
    bool is_nesting = (node->kind == NODE_IF || node->kind == NODE_WHILE ||
                       node->kind == NODE_FOR || node->kind == NODE_DO_WHILE ||
                       node->kind == NODE_SWITCH);
    int next_depth = current_depth + (is_nesting ? 1 : 0);
    if (is_nesting && next_depth > tracker->max_depth) {
        tracker->max_depth = next_depth;
        tracker->deepest_line = node->line;
        tracker->deepest_col = node->column;
    }
    if (node->kind == NODE_IF) {
        walk_nesting(node->data.if_stmt.then_branch, next_depth, tracker);
        walk_nesting(node->data.if_stmt.else_branch, next_depth, tracker);
    } else if (node->kind == NODE_WHILE) {
        walk_nesting(node->data.while_stmt.body, next_depth, tracker);
    } else if (node->kind == NODE_FOR) {
        walk_nesting(node->data.for_stmt.body, next_depth, tracker);
    } else if (node->kind == NODE_DO_WHILE) {
        walk_nesting(node->data.do_while.body, next_depth, tracker);
    } else if (node->kind == NODE_SWITCH) {
        walk_nesting(node->data.switch_stmt.body, next_depth, tracker);
    } else if (node->kind == NODE_BLOCK) {
        for (const AstNode *s = node->data.block.statements; s; s = s->next) walk_nesting(s, next_depth, tracker);
    }
}

/* Rule W007: Flags deep control-flow nesting exceeding threshold once per function */
void check_w007_deep_nesting(const AstNode *root, const SymbolTable *st, DiagList *diags) {
    (void)st;
    if (!root || root->kind != NODE_TRANSLATION_UNIT) return;
    for (const AstNode *d = root->data.translation_unit.declarations; d; d = d->next) {
        if (d->kind == NODE_FUNCTION_DEF && d->data.function_def.body) {
            NestingTracker tracker = { 0, 0, 0 };
            walk_nesting(d->data.function_def.body, 0, &tracker);
            if (tracker.max_depth > g_checks_config.max_nesting_depth) {
                char msg[128];
                snprintf(msg, sizeof(msg), "nesting depth of %d exceeds threshold of %d",
                         tracker.max_depth, g_checks_config.max_nesting_depth);
                diag_add(diags, "W007", DIAG_WARNING, tracker.deepest_line, tracker.deepest_col,
                         msg, "refactor nested logic into helper functions");
            }
        }
    }
}

/* Rule W008: Flags function definitions whose body length exceeds line threshold */
void check_w008_long_function(const AstNode *root, const SymbolTable *st, DiagList *diags) {
    (void)st;
    if (!root || root->kind != NODE_TRANSLATION_UNIT) return;
    for (const AstNode *d = root->data.translation_unit.declarations; d; d = d->next) {
        if (d->kind == NODE_FUNCTION_DEF && d->data.function_def.body) {
            int span = d->end_line - d->line + 1;
            if (span > g_checks_config.max_function_lines) {
                char msg[128];
                snprintf(msg, sizeof(msg), "function '%s' is %d lines long (exceeds limit of %d)",
                         d->data.function_def.name, span, g_checks_config.max_function_lines);
                diag_add(diags, "W008", DIAG_WARNING, d->line, d->column, msg,
                         "break large function into smaller modular functions");
            }
        }
    }
}

/* Returns true if an AST statement node represents an empty block or empty statement */
static bool is_empty_body(const AstNode *node) {
    if (!node) return true;
    if (node->kind == NODE_EMPTY_STMT) return true;
    if (node->kind == NODE_BLOCK && node->data.block.statements == NULL) return true;
    return false;
}

/* Traverses AST checking for control structures with empty blocks or lone semicolons */
static void walk_empty_bodies(const AstNode *node, DiagList *diags) {
    if (!node) return;
    if (node->kind == NODE_IF) {
        if (is_empty_body(node->data.if_stmt.then_branch)) {
            diag_add(diags, "W009", DIAG_WARNING, node->line, node->column,
                     "empty body in 'if' statement", "remove empty statement or add body logic");
        }
        if (node->data.if_stmt.else_branch) {
            if (node->data.if_stmt.else_branch->kind != NODE_IF && is_empty_body(node->data.if_stmt.else_branch)) {
                diag_add(diags, "W009", DIAG_WARNING, node->data.if_stmt.else_branch->line,
                         node->data.if_stmt.else_branch->column,
                         "empty body in 'else' branch", "remove empty branch or add branch logic");
            }
        }
        walk_empty_bodies(node->data.if_stmt.then_branch, diags);
        walk_empty_bodies(node->data.if_stmt.else_branch, diags);
    } else if (node->kind == NODE_WHILE) {
        if (is_empty_body(node->data.while_stmt.body)) {
            diag_add(diags, "W009", DIAG_WARNING, node->line, node->column,
                     "empty body in 'while' loop", "remove empty loop or implement loop body");
        }
        walk_empty_bodies(node->data.while_stmt.body, diags);
    } else if (node->kind == NODE_FOR) {
        bool is_forever = (node->data.for_stmt.init == NULL &&
                           node->data.for_stmt.condition == NULL &&
                           node->data.for_stmt.step == NULL);
        if (!is_forever && is_empty_body(node->data.for_stmt.body)) {
            diag_add(diags, "W009", DIAG_WARNING, node->line, node->column,
                     "empty body in 'for' loop", "remove empty loop or implement loop body");
        }
        walk_empty_bodies(node->data.for_stmt.body, diags);
    } else if (node->kind == NODE_BLOCK) {
        for (const AstNode *s = node->data.block.statements; s; s = s->next) walk_empty_bodies(s, diags);
    } else if (node->kind == NODE_FUNCTION_DEF) {
        walk_empty_bodies(node->data.function_def.body, diags);
    } else if (node->kind == NODE_TRANSLATION_UNIT) {
        for (const AstNode *d = node->data.translation_unit.declarations; d; d = d->next) walk_empty_bodies(d, diags);
    }
}

/* Rule W009: Detects control flow statements with an empty block or lone semicolon */
void check_w009_empty_body(const AstNode *root, const SymbolTable *st, DiagList *diags) {
    (void)st;
    walk_empty_bodies(root, diags);
}

/* Traverses AST to find switch statements missing a default label */
static void walk_switch_default(const AstNode *node, DiagList *diags) {
    if (!node) return;
    if (node->kind == NODE_SWITCH) {
        bool has_default = false;
        const AstNode *body = node->data.switch_stmt.body;
        if (body && body->kind == NODE_BLOCK) {
            for (const AstNode *s = body->data.block.statements; s; s = s->next) {
                if (s->kind == NODE_DEFAULT) {
                    has_default = true;
                    break;
                }
            }
        }
        if (!has_default) {
            diag_add(diags, "W010", DIAG_WARNING, node->line, node->column,
                     "switch statement missing 'default' label",
                     "add a 'default:' case to handle unanticipated values");
        }
        walk_switch_default(node->data.switch_stmt.body, diags);
    } else if (node->kind == NODE_BLOCK) {
        for (const AstNode *s = node->data.block.statements; s; s = s->next) walk_switch_default(s, diags);
    } else if (node->kind == NODE_IF) {
        walk_switch_default(node->data.if_stmt.then_branch, diags);
        walk_switch_default(node->data.if_stmt.else_branch, diags);
    } else if (node->kind == NODE_WHILE) {
        walk_switch_default(node->data.while_stmt.body, diags);
    } else if (node->kind == NODE_FOR) {
        walk_switch_default(node->data.for_stmt.body, diags);
    } else if (node->kind == NODE_FUNCTION_DEF) {
        walk_switch_default(node->data.function_def.body, diags);
    } else if (node->kind == NODE_TRANSLATION_UNIT) {
        for (const AstNode *d = node->data.translation_unit.declarations; d; d = d->next) walk_switch_default(d, diags);
    }
}

/* Rule W010: Detects switch statements lacking a default case label */
void check_w010_switch_without_default(const AstNode *root, const SymbolTable *st, DiagList *diags) {
    (void)st;
    walk_switch_default(root, diags);
}

/* Returns true if a scanf format string contains '%s' without an explicit width */
static bool scanf_has_unbounded_s(const char *fmt) {
    if (!fmt) return false;
    for (const char *p = fmt; *p; p++) {
        if (*p == '%') {
            p++;
            if (*p == '%') continue;
            bool has_width = false;
            while (*p >= '0' && *p <= '9') {
                has_width = true;
                p++;
            }
            if (*p == 's' && !has_width) return true;
            if (!*p) break;
        }
    }
    return false;
}

/* Validates function call safety and suggests modern bounded alternatives */
static void inspect_unsafe_call(const AstNode *call, DiagList *diags) {
    const AstNode *callee = call->data.call.callee;
    if (!callee || callee->kind != NODE_IDENTIFIER) return;
    const char *fname = callee->data.identifier.name;
    if (!fname) return;

    if (strcmp(fname, "gets") == 0) {
        diag_add(diags, "W011", DIAG_ERROR, call->line, call->column,
                 "call to inherently unsafe function 'gets'",
                 "use 'fgets' with buffer size limit instead");
    } else if (strcmp(fname, "strcpy") == 0) {
        diag_add(diags, "W011", DIAG_WARNING, call->line, call->column,
                 "call to unsafe function 'strcpy' without bounds checking",
                 "use 'strncpy' or safer bounded alternative");
    } else if (strcmp(fname, "strcat") == 0) {
        diag_add(diags, "W011", DIAG_WARNING, call->line, call->column,
                 "call to unsafe function 'strcat' without bounds checking",
                 "use 'strncat' with buffer limit");
    } else if (strcmp(fname, "sprintf") == 0) {
        diag_add(diags, "W011", DIAG_WARNING, call->line, call->column,
                 "call to unsafe function 'sprintf' without buffer limit",
                 "use 'snprintf' with buffer limit");
    } else if (strcmp(fname, "scanf") == 0) {
        const AstNode *first_arg = call->data.call.args;
        if (first_arg && first_arg->kind == NODE_STRING_LITERAL) {
            if (scanf_has_unbounded_s(first_arg->data.literal.value)) {
                diag_add(diags, "W011", DIAG_WARNING, call->line, call->column,
                         "call to 'scanf' with '%s' format without width limit",
                         "specify maximum width like '%31s' or use 'fgets'");
            }
        }
    }
}

/* Recursively walks expression nodes looking for unsafe function calls */
static void walk_expr_unsafe_calls(const AstNode *node, DiagList *diags) {
    if (!node) return;
    if (node->kind == NODE_CALL) {
        inspect_unsafe_call(node, diags);
        for (const AstNode *a = node->data.call.args; a; a = a->next) walk_expr_unsafe_calls(a, diags);
    } else if (node->kind == NODE_BINARY) {
        walk_expr_unsafe_calls(node->data.binary.left, diags);
        walk_expr_unsafe_calls(node->data.binary.right, diags);
    } else if (node->kind == NODE_UNARY) {
        walk_expr_unsafe_calls(node->data.unary.operand, diags);
    } else if (node->kind == NODE_ASSIGN) {
        walk_expr_unsafe_calls(node->data.assign.left, diags);
        walk_expr_unsafe_calls(node->data.assign.right, diags);
    } else if (node->kind == NODE_INDEX) {
        walk_expr_unsafe_calls(node->data.index.array, diags);
        walk_expr_unsafe_calls(node->data.index.index, diags);
    } else if (node->kind == NODE_TERNARY) {
        walk_expr_unsafe_calls(node->data.ternary.condition, diags);
        walk_expr_unsafe_calls(node->data.ternary.true_expr, diags);
        walk_expr_unsafe_calls(node->data.ternary.false_expr, diags);
    }
}

/* Traverses statement AST nodes inspecting expressions for unsafe function calls */
static void walk_stmt_unsafe_calls(const AstNode *node, DiagList *diags) {
    if (!node) return;
    if (node->kind == NODE_VAR_DECL) {
        if (node->data.var_decl.init) walk_expr_unsafe_calls(node->data.var_decl.init, diags);
    } else if (node->kind == NODE_EXPR_STMT) {
        walk_expr_unsafe_calls(node->data.expr_stmt.expr, diags);
    } else if (node->kind == NODE_RETURN) {
        walk_expr_unsafe_calls(node->data.return_stmt.expr, diags);
    } else if (node->kind == NODE_IF) {
        walk_expr_unsafe_calls(node->data.if_stmt.condition, diags);
        walk_stmt_unsafe_calls(node->data.if_stmt.then_branch, diags);
        walk_stmt_unsafe_calls(node->data.if_stmt.else_branch, diags);
    } else if (node->kind == NODE_WHILE) {
        walk_expr_unsafe_calls(node->data.while_stmt.condition, diags);
        walk_stmt_unsafe_calls(node->data.while_stmt.body, diags);
    } else if (node->kind == NODE_DO_WHILE) {
        walk_expr_unsafe_calls(node->data.do_while.condition, diags);
        walk_stmt_unsafe_calls(node->data.do_while.body, diags);
    } else if (node->kind == NODE_FOR) {
        walk_stmt_unsafe_calls(node->data.for_stmt.init, diags);
        walk_expr_unsafe_calls(node->data.for_stmt.condition, diags);
        walk_expr_unsafe_calls(node->data.for_stmt.step, diags);
        walk_stmt_unsafe_calls(node->data.for_stmt.body, diags);
    } else if (node->kind == NODE_SWITCH) {
        walk_expr_unsafe_calls(node->data.switch_stmt.expr, diags);
        walk_stmt_unsafe_calls(node->data.switch_stmt.body, diags);
    } else if (node->kind == NODE_BLOCK) {
        for (const AstNode *s = node->data.block.statements; s; s = s->next) walk_stmt_unsafe_calls(s, diags);
    } else if (node->kind == NODE_FUNCTION_DEF) {
        walk_stmt_unsafe_calls(node->data.function_def.body, diags);
    } else if (node->kind == NODE_TRANSLATION_UNIT) {
        for (const AstNode *d = node->data.translation_unit.declarations; d; d = d->next) walk_stmt_unsafe_calls(d, diags);
    }
}

/* Rule W011: Flags calls to unsafe functions like gets, strcpy, strcat, sprintf, and unbounded scanf */
void check_w011_unsafe_functions(const AstNode *root, const SymbolTable *st, DiagList *diags) {
    (void)st;
    walk_stmt_unsafe_calls(root, diags);
}

/* Rule W012: Detects local variables or parameters shadowing variables in outer scopes */
void check_w012_variable_shadowing(const AstNode *root, const SymbolTable *st, DiagList *diags) {
    (void)root;
    if (!st) return;
    for (size_t i = 0; i < st->all_scopes_count; i++) {
        Scope *sc = st->all_scopes[i];
        if (sc->level == 0) continue;
        for (Symbol *s = sc->symbols; s != NULL; s = s->next) {
            if (s->kind != SYM_VARIABLE && s->kind != SYM_PARAMETER) continue;
            for (Scope *p = sc->parent; p != NULL; p = p->parent) {
                bool found = false;
                for (Symbol *sym = p->symbols; sym != NULL; sym = sym->next) {
                    if ((sym->kind == SYM_VARIABLE || sym->kind == SYM_PARAMETER) &&
                        strcmp(sym->name, s->name) == 0) {
                        char msg[128];
                        snprintf(msg, sizeof(msg),
                                 "declaration of '%s' shadows variable in enclosing scope (line %d)",
                                 s->name, sym->line);
                        diag_add(diags, "W012", DIAG_WARNING, s->line, s->column, msg,
                                 "rename variable to avoid masking outer scope variable");
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
        }
    }
}

/* Inspects binary expressions and assignments for division or modulo by literal zero */
static void inspect_div_zero_expr(const AstNode *node, DiagList *diags) {
    if (!node) return;
    if (node->kind == NODE_BINARY) {
        const char *op = node->data.binary.op;
        if (strcmp(op, "/") == 0 || strcmp(op, "%") == 0) {
            const AstNode *r = node->data.binary.right;
            if (r && r->kind == NODE_INT_LITERAL) {
                long val = strtol(r->data.literal.value, NULL, 0);
                if (val == 0) {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "division by zero: right operand of '%s' is literal 0", op);
                    diag_add(diags, "W013", DIAG_ERROR, node->line, node->column, msg,
                             "ensure denominator is non-zero before dividing");
                }
            }
        }
        inspect_div_zero_expr(node->data.binary.left, diags);
        inspect_div_zero_expr(node->data.binary.right, diags);
    } else if (node->kind == NODE_ASSIGN) {
        const char *op = node->data.assign.op;
        if (strcmp(op, "/=") == 0 || strcmp(op, "%=") == 0) {
            const AstNode *r = node->data.assign.right;
            if (r && r->kind == NODE_INT_LITERAL) {
                long val = strtol(r->data.literal.value, NULL, 0);
                if (val == 0) {
                    char msg[64];
                    snprintf(msg, sizeof(msg), "division by zero: right operand of '%s' is literal 0", op);
                    diag_add(diags, "W013", DIAG_ERROR, node->line, node->column, msg,
                             "ensure denominator is non-zero before dividing");
                }
            }
        }
        inspect_div_zero_expr(node->data.assign.left, diags);
        inspect_div_zero_expr(node->data.assign.right, diags);
    } else if (node->kind == NODE_UNARY) {
        inspect_div_zero_expr(node->data.unary.operand, diags);
    } else if (node->kind == NODE_CALL) {
        for (const AstNode *a = node->data.call.args; a; a = a->next) inspect_div_zero_expr(a, diags);
    } else if (node->kind == NODE_INDEX) {
        inspect_div_zero_expr(node->data.index.array, diags);
        inspect_div_zero_expr(node->data.index.index, diags);
    } else if (node->kind == NODE_TERNARY) {
        inspect_div_zero_expr(node->data.ternary.condition, diags);
        inspect_div_zero_expr(node->data.ternary.true_expr, diags);
        inspect_div_zero_expr(node->data.ternary.false_expr, diags);
    }
}

/* Recursively walks statements inspecting all expressions for division by zero */
static void walk_stmt_div_zero(const AstNode *node, DiagList *diags) {
    if (!node) return;
    if (node->kind == NODE_VAR_DECL) {
        if (node->data.var_decl.init) inspect_div_zero_expr(node->data.var_decl.init, diags);
    } else if (node->kind == NODE_EXPR_STMT) {
        inspect_div_zero_expr(node->data.expr_stmt.expr, diags);
    } else if (node->kind == NODE_RETURN) {
        inspect_div_zero_expr(node->data.return_stmt.expr, diags);
    } else if (node->kind == NODE_IF) {
        inspect_div_zero_expr(node->data.if_stmt.condition, diags);
        walk_stmt_div_zero(node->data.if_stmt.then_branch, diags);
        walk_stmt_div_zero(node->data.if_stmt.else_branch, diags);
    } else if (node->kind == NODE_WHILE) {
        inspect_div_zero_expr(node->data.while_stmt.condition, diags);
        walk_stmt_div_zero(node->data.while_stmt.body, diags);
    } else if (node->kind == NODE_DO_WHILE) {
        inspect_div_zero_expr(node->data.do_while.condition, diags);
        walk_stmt_div_zero(node->data.do_while.body, diags);
    } else if (node->kind == NODE_FOR) {
        walk_stmt_div_zero(node->data.for_stmt.init, diags);
        inspect_div_zero_expr(node->data.for_stmt.condition, diags);
        inspect_div_zero_expr(node->data.for_stmt.step, diags);
        walk_stmt_div_zero(node->data.for_stmt.body, diags);
    } else if (node->kind == NODE_SWITCH) {
        inspect_div_zero_expr(node->data.switch_stmt.expr, diags);
        walk_stmt_div_zero(node->data.switch_stmt.body, diags);
    } else if (node->kind == NODE_BLOCK) {
        for (const AstNode *s = node->data.block.statements; s; s = s->next) walk_stmt_div_zero(s, diags);
    } else if (node->kind == NODE_FUNCTION_DEF) {
        walk_stmt_div_zero(node->data.function_def.body, diags);
    } else if (node->kind == NODE_TRANSLATION_UNIT) {
        for (const AstNode *d = node->data.translation_unit.declarations; d; d = d->next) walk_stmt_div_zero(d, diags);
    }
}

/* Rule W013: Flags division or modulo operations where the right side is literal 0 */
void check_w013_division_by_zero(const AstNode *root, const SymbolTable *st, DiagList *diags) {
    (void)st;
    walk_stmt_div_zero(root, diags);
}

/* Returns true if a statement terminates flow via break, return, or continue */
static bool case_ends_with_terminator(const AstNode *last_stmt) {
    if (!last_stmt) return false;
    if (last_stmt->kind == NODE_BREAK || last_stmt->kind == NODE_RETURN ||
        last_stmt->kind == NODE_CONTINUE) return true;
    return stmt_always_terminates(last_stmt);
}

/* Checks a switch block for cases containing statements that fall through without break */
static void check_switch_fall_through(const AstNode *sw, DiagList *diags) {
    const AstNode *body = sw->data.switch_stmt.body;
    if (!body || body->kind != NODE_BLOCK) return;

    const AstNode *s = body->data.block.statements;
    while (s) {
        if (s->kind == NODE_CASE) {
            const AstNode *case_node = s;
            const AstNode *curr = s->next;
            const AstNode *last_stmt = NULL;
            int stmt_count = 0;

            while (curr && curr->kind != NODE_CASE && curr->kind != NODE_DEFAULT) {
                stmt_count++;
                last_stmt = curr;
                curr = curr->next;
            }

            if (curr != NULL && stmt_count > 0) {
                if (!case_ends_with_terminator(last_stmt)) {
                    diag_add(diags, "W014", DIAG_INFO, case_node->line, case_node->column,
                             "case label falls through to next case without break",
                             "add 'break;' or comment fall-through if intentional");
                }
            }
            s = curr;
        } else {
            s = s->next;
        }
    }
}

/* Traverses AST to locate all switch statements for case fall-through analysis */
static void walk_fall_through(const AstNode *node, DiagList *diags) {
    if (!node) return;
    if (node->kind == NODE_SWITCH) {
        check_switch_fall_through(node, diags);
        walk_fall_through(node->data.switch_stmt.body, diags);
    } else if (node->kind == NODE_BLOCK) {
        for (const AstNode *s = node->data.block.statements; s; s = s->next) walk_fall_through(s, diags);
    } else if (node->kind == NODE_IF) {
        walk_fall_through(node->data.if_stmt.then_branch, diags);
        walk_fall_through(node->data.if_stmt.else_branch, diags);
    } else if (node->kind == NODE_WHILE) {
        walk_fall_through(node->data.while_stmt.body, diags);
    } else if (node->kind == NODE_FOR) {
        walk_fall_through(node->data.for_stmt.body, diags);
    } else if (node->kind == NODE_FUNCTION_DEF) {
        walk_fall_through(node->data.function_def.body, diags);
    } else if (node->kind == NODE_TRANSLATION_UNIT) {
        for (const AstNode *d = node->data.translation_unit.declarations; d; d = d->next) walk_fall_through(d, diags);
    }
}

/* Rule W014: Flags case labels having statements that fall through to the next case without break */
void check_w014_fall_through_case(const AstNode *root, const SymbolTable *st, DiagList *diags) {
    (void)st;
    walk_fall_through(root, diags);
}

/* Master rule registry table */
static const Rule RULES_TABLE[] = {
    { "W001", "unused-variable", DIAG_WARNING, "Unused local variable", check_w001_unused_variables, true },
    { "W002", "assignment-in-condition", DIAG_WARNING, "Assignment in condition", check_w002_assignment_in_condition, true },
    { "W003", "unreachable-code", DIAG_WARNING, "Unreachable statement in block", check_w003_unreachable_code, true },
    { "W004", "missing-return", DIAG_WARNING, "Missing return in non-void function", check_w004_missing_return, true },
    { "W005", "use-before-assignment", DIAG_WARNING, "Variable used before assignment", check_w005_use_before_assignment, true },
    { "W006", "magic-number", DIAG_INFO, "Numeric literal other than 0, 1, 2, -1 in expression", check_w006_magic_numbers, true },
    { "W007", "deep-nesting", DIAG_WARNING, "Statement nesting depth exceeds threshold", check_w007_deep_nesting, true },
    { "W008", "long-function", DIAG_WARNING, "Function body length exceeds line threshold", check_w008_long_function, true },
    { "W009", "empty-body", DIAG_WARNING, "Empty statement or block as body", check_w009_empty_body, true },
    { "W010", "switch-without-default", DIAG_WARNING, "Switch statement missing default label", check_w010_switch_without_default, true },
    { "W011", "unsafe-function", DIAG_WARNING, "Call to unsafe standard library function", check_w011_unsafe_functions, true },
    { "W012", "variable-shadowing", DIAG_WARNING, "Local variable shadows enclosing declaration", check_w012_variable_shadowing, true },
    { "W013", "division-by-zero", DIAG_ERROR, "Division or modulo by literal zero", check_w013_division_by_zero, true },
    { "W014", "fall-through-case", DIAG_INFO, "Case label falls through without break", check_w014_fall_through_case, true }
};

/* Executes all enabled static analysis checks over the AST and symbol table */
void checks_run(const AstNode *root, const SymbolTable *st, DiagList *diags) {
    size_t num_rules = sizeof(RULES_TABLE) / sizeof(RULES_TABLE[0]);
    for (size_t i = 0; i < num_rules; i++) {
        if (RULES_TABLE[i].enabled && RULES_TABLE[i].fn) {
            RULES_TABLE[i].fn(root, st, diags);
        }
    }
}
