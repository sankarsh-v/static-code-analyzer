#include "symtab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Initializes symbol table state and establishes the global scope */
void symtab_init(SymbolTable *st) {
    st->current_scope = NULL;
    st->current_level = 0;
    st->all_scopes = NULL;
    st->all_scopes_count = 0;
    st->all_scopes_capacity = 0;
    st->on_exit_scope = NULL;
    st->hook_userdata = NULL;
    symtab_push_scope(st, "Global");
}

/* Appends an allocated scope to the master tracking array */
static void track_scope(SymbolTable *st, Scope *sc) {
    if (st->all_scopes_count >= st->all_scopes_capacity) {
        size_t new_cap = st->all_scopes_capacity == 0 ? 16 : st->all_scopes_capacity * 2;
        Scope **new_arr = realloc(st->all_scopes, new_cap * sizeof(Scope *));
        if (!new_arr) return;
        st->all_scopes = new_arr;
        st->all_scopes_capacity = new_cap;
    }
    st->all_scopes[st->all_scopes_count++] = sc;
}

/* Pushes a new nested scope onto the active stack */
void symtab_push_scope(SymbolTable *st, const char *desc) {
    Scope *sc = calloc(1, sizeof(Scope));
    if (!sc) return;
    sc->id = (int)st->all_scopes_count;
    sc->level = st->current_level;
    strncpy(sc->description, desc ? desc : "Scope", sizeof(sc->description) - 1);
    sc->symbols = NULL;
    sc->parent = st->current_scope;
    st->current_scope = sc;
    st->current_level++;
    track_scope(st, sc);
}

/* Pops the current scope from the active stack after invoking exit hook */
void symtab_pop_scope(SymbolTable *st) {
    if (!st->current_scope) return;
    if (st->on_exit_scope) {
        st->on_exit_scope(st->current_scope, st->hook_userdata);
    }
    st->current_scope = st->current_scope->parent;
    st->current_level--;
}

/* Looks up an identifier strictly within the current active scope */
Symbol *symtab_lookup_in_current_scope(const SymbolTable *st, const char *name) {
    if (!st->current_scope || !name) return NULL;
    for (Symbol *s = st->current_scope->symbols; s != NULL; s = s->next) {
        if (strcmp(s->name, name) == 0) return s;
    }
    return NULL;
}

/* Looks up an identifier from innermost scope outward to global scope */
Symbol *symtab_lookup(const SymbolTable *st, const char *name) {
    if (!name) return NULL;
    for (Scope *sc = st->current_scope; sc != NULL; sc = sc->parent) {
        for (Symbol *s = sc->symbols; s != NULL; s = s->next) {
            if (strcmp(s->name, name) == 0) return s;
        }
    }
    return NULL;
}

/* Declares a symbol in current scope, returning existing symbol if duplicate */
Symbol *symtab_declare(SymbolTable *st, const char *name, SymbolKind kind,
                       const char *type_str, AstNode *decl_node,
                       int line, int column, bool is_init) {
    if (!st->current_scope || !name) return NULL;
    Symbol *existing = symtab_lookup_in_current_scope(st, name);
    if (existing) return existing;

    Symbol *sym = calloc(1, sizeof(Symbol));
    if (!sym) return NULL;
    sym->name = strdup(name);
    sym->kind = kind;
    sym->type_str = strdup(type_str ? type_str : "");
    sym->decl_node = decl_node;
    sym->line = line;
    sym->column = column;
    sym->used = false;
    sym->assigned = is_init;
    sym->initialized_at_declaration = is_init;
    sym->is_global = (st->current_scope->level == 0);
    sym->scope_level = st->current_scope->level;

    /* Append to scope symbol list to preserve declaration order */
    if (!st->current_scope->symbols) {
        st->current_scope->symbols = sym;
    } else {
        Symbol *tail = st->current_scope->symbols;
        while (tail->next) tail = tail->next;
        tail->next = sym;
    }
    return NULL;
}

/* Sets a scope exit callback hook */
void symtab_set_exit_hook(SymbolTable *st, ScopeExitHook hook, void *userdata) {
    st->on_exit_scope = hook;
    st->hook_userdata = userdata;
}

/* Converts symbol kind enum to printable string */
static const char *sym_kind_str(SymbolKind kind) {
    switch (kind) {
        case SYM_VARIABLE: return "variable";
        case SYM_PARAMETER: return "parameter";
        case SYM_FUNCTION: return "function";
        default: return "symbol";
    }
}

/* Dumps all scopes and their symbols with flags to stdout */
void symtab_dump(const SymbolTable *st) {
    for (size_t i = 0; i < st->all_scopes_count; i++) {
        Scope *sc = st->all_scopes[i];
        printf("Scope %d [level %d, %s]:\n", sc->id, sc->level, sc->description);
        if (!sc->symbols) {
            printf("  (empty)\n");
            continue;
        }
        for (Symbol *s = sc->symbols; s != NULL; s = s->next) {
            printf("  %s %s [%s%s%s%s%s] (line %d:%d)\n",
                   s->type_str && s->type_str[0] ? s->type_str : "void",
                   s->name,
                   sym_kind_str(s->kind),
                   s->is_global ? ", global" : ", local",
                   s->initialized_at_declaration ? ", initialized" : "",
                   s->used ? ", used" : "",
                   s->assigned ? ", assigned" : "",
                   s->line, s->column);
        }
    }
}

/* Deallocates all scopes and symbols tracked by the symbol table */
void symtab_free(SymbolTable *st) {
    for (size_t i = 0; i < st->all_scopes_count; i++) {
        Scope *sc = st->all_scopes[i];
        Symbol *s = sc->symbols;
        while (s) {
            Symbol *next = s->next;
            free(s->name);
            free(s->type_str);
            free(s);
            s = next;
        }
        free(sc);
    }
    free(st->all_scopes);
    st->all_scopes = NULL;
    st->all_scopes_count = 0;
    st->all_scopes_capacity = 0;
    st->current_scope = NULL;
}

/* Forward declaration for statement resolver */
static void resolve_stmt(AstNode *node, SymbolTable *st, DiagList *diags);

/* Resolves identifier uses and sets read/write flags in expressions */
static void resolve_expr(AstNode *node, SymbolTable *st, DiagList *diags,
                         bool is_write, bool is_read_write) {
    if (!node) return;
    if (node->kind == NODE_IDENTIFIER) {
        Symbol *sym = symtab_lookup(st, node->data.identifier.name);
        node->data.identifier.symbol = sym;
        if (sym) {
            if (is_write) sym->assigned = true;
            if (!is_write || is_read_write) sym->used = true;
        } else if (diags) {
            char msg[128];
            snprintf(msg, sizeof(msg), "undeclared variable '%s'", node->data.identifier.name);
            diag_list_add(diags, "I002", DIAG_INFO, node->line, node->column,
                          msg, "declare variable before use");
        }
        return;
    }
    if (node->kind == NODE_ASSIGN) {
        bool compound = (strcmp(node->data.assign.op, "=") != 0);
        resolve_expr(node->data.assign.left, st, diags, true, compound);
        resolve_expr(node->data.assign.right, st, diags, false, false);
    } else if (node->kind == NODE_UNARY) {
        bool incdec = (strcmp(node->data.unary.op, "++") == 0 || strcmp(node->data.unary.op, "--") == 0);
        resolve_expr(node->data.unary.operand, st, diags, incdec, incdec);
    } else if (node->kind == NODE_CALL) {
        if (node->data.call.callee && node->data.call.callee->kind == NODE_IDENTIFIER) {
            Symbol *sym = symtab_lookup(st, node->data.call.callee->data.identifier.name);
            if (sym) sym->used = true;
            node->data.call.callee->data.identifier.symbol = sym;
        } else {
            resolve_expr(node->data.call.callee, st, diags, false, false);
        }
        for (AstNode *arg = node->data.call.args; arg != NULL; arg = arg->next) {
            resolve_expr(arg, st, diags, false, false);
        }
    } else if (node->kind == NODE_BINARY) {
        resolve_expr(node->data.binary.left, st, diags, false, false);
        resolve_expr(node->data.binary.right, st, diags, false, false);
    } else if (node->kind == NODE_INDEX) {
        resolve_expr(node->data.index.array, st, diags, false, false);
        resolve_expr(node->data.index.index, st, diags, false, false);
    } else if (node->kind == NODE_TERNARY) {
        resolve_expr(node->data.ternary.condition, st, diags, false, false);
        resolve_expr(node->data.ternary.true_expr, st, diags, false, false);
        resolve_expr(node->data.ternary.false_expr, st, diags, false, false);
    }
}

/* Resolves a single variable declaration node */
static void resolve_var_decl(AstNode *node, SymbolTable *st, DiagList *diags) {
    if (!node || node->kind != NODE_VAR_DECL) return;
    if (node->data.var_decl.init) {
        resolve_expr(node->data.var_decl.init, st, diags, false, false);
    }
    bool has_init = (node->data.var_decl.init != NULL);
    Symbol *prev = symtab_declare(st, node->data.var_decl.name, SYM_VARIABLE,
                                  node->data.var_decl.type_name, node,
                                  node->line, node->column, has_init);
    if (prev && diags) {
        char msg[128];
        snprintf(msg, sizeof(msg), "redeclaration of symbol '%s' in same scope", node->data.var_decl.name);
        diag_list_add(diags, "E200", DIAG_ERROR, node->line, node->column, msg, "rename or remove duplicate variable");
    }
}

/* Resolves statement sub-trees and manages statement-level scopes */
static void resolve_stmt(AstNode *node, SymbolTable *st, DiagList *diags) {
    if (!node) return;
    if (node->kind == NODE_VAR_DECL) {
        resolve_var_decl(node, st, diags);
    } else if (node->kind == NODE_BLOCK) {
        symtab_push_scope(st, "Block");
        for (AstNode *s = node->data.block.statements; s != NULL; s = s->next) resolve_stmt(s, st, diags);
        symtab_pop_scope(st);
    } else if (node->kind == NODE_FOR) {
        symtab_push_scope(st, "ForLoop");
        if (node->data.for_stmt.init) resolve_stmt(node->data.for_stmt.init, st, diags);
        if (node->data.for_stmt.condition) resolve_expr(node->data.for_stmt.condition, st, diags, false, false);
        if (node->data.for_stmt.step) resolve_expr(node->data.for_stmt.step, st, diags, false, false);
        if (node->data.for_stmt.body) resolve_stmt(node->data.for_stmt.body, st, diags);
        symtab_pop_scope(st);
    } else if (node->kind == NODE_SWITCH) {
        resolve_expr(node->data.switch_stmt.expr, st, diags, false, false);
        symtab_push_scope(st, "Switch");
        if (node->data.switch_stmt.body) resolve_stmt(node->data.switch_stmt.body, st, diags);
        symtab_pop_scope(st);
    } else if (node->kind == NODE_IF) {
        resolve_expr(node->data.if_stmt.condition, st, diags, false, false);
        resolve_stmt(node->data.if_stmt.then_branch, st, diags);
        if (node->data.if_stmt.else_branch) resolve_stmt(node->data.if_stmt.else_branch, st, diags);
    } else if (node->kind == NODE_WHILE) {
        resolve_expr(node->data.while_stmt.condition, st, diags, false, false);
        resolve_stmt(node->data.while_stmt.body, st, diags);
    } else if (node->kind == NODE_DO_WHILE) {
        resolve_stmt(node->data.do_while.body, st, diags);
        resolve_expr(node->data.do_while.condition, st, diags, false, false);
    } else if (node->kind == NODE_RETURN) {
        if (node->data.return_stmt.expr) resolve_expr(node->data.return_stmt.expr, st, diags, false, false);
    } else if (node->kind == NODE_EXPR_STMT) {
        if (node->data.expr_stmt.expr) resolve_expr(node->data.expr_stmt.expr, st, diags, false, false);
    }
}

/* Resolves function definitions and parameter bindings */
static void resolve_function(AstNode *node, SymbolTable *st, DiagList *diags) {
    symtab_declare(st, node->data.function_def.name, SYM_FUNCTION,
                   node->data.function_def.return_type, node,
                   node->line, node->column, false);

    if (!node->data.function_def.body) return;

    char desc[64];
    snprintf(desc, sizeof(desc), "Function '%s'", node->data.function_def.name);
    symtab_push_scope(st, desc);

    for (AstNode *p = node->data.function_def.params; p != NULL; p = p->next) {
        if (!p->data.param.name) continue;
        Symbol *prev = symtab_declare(st, p->data.param.name, SYM_PARAMETER,
                                      p->data.param.type_name, p,
                                      p->line, p->column, true);
        if (prev && diags) {
            char msg[128];
            snprintf(msg, sizeof(msg), "duplicate parameter name '%s'", p->data.param.name);
            diag_list_add(diags, "E200", DIAG_ERROR, p->line, p->column, msg, "rename parameter");
        }
    }

    AstNode *body = node->data.function_def.body;
    if (body->kind == NODE_BLOCK) {
        for (AstNode *s = body->data.block.statements; s != NULL; s = s->next) {
            resolve_stmt(s, st, diags);
        }
    } else {
        resolve_stmt(body, st, diags);
    }
    symtab_pop_scope(st);
}

/* Traverses the AST root, establishing scopes and resolving identifier bindings */
void resolver_resolve(AstNode *root, SymbolTable *st, DiagList *diags) {
    if (!root || root->kind != NODE_TRANSLATION_UNIT) return;
    for (AstNode *d = root->data.translation_unit.declarations; d != NULL; d = d->next) {
        if (d->kind == NODE_FUNCTION_DEF) {
            resolve_function(d, st, diags);
        } else if (d->kind == NODE_VAR_DECL) {
            resolve_var_decl(d, st, diags);
        } else {
            resolve_stmt(d, st, diags);
        }
    }
}
