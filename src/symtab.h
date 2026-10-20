#ifndef SYMTAB_H
#define SYMTAB_H

#include "ast.h"
#include "diag.h"
#include <stdbool.h>
#include <stddef.h>

/* Categories of symbols stored in symbol table */
typedef enum {
    SYM_VARIABLE = 0,
    SYM_PARAMETER,
    SYM_FUNCTION
} SymbolKind;

typedef struct Symbol Symbol;

/* Symbol record tracking name, type, source location, and usage flags */
struct Symbol {
    char *name;
    SymbolKind kind;
    char *type_str;
    AstNode *decl_node;
    int line;
    int column;
    bool used;
    bool assigned;
    bool initialized_at_declaration;
    bool is_global;
    int scope_level;
    Symbol *next;
};

typedef struct Scope Scope;

/* Lexical scope containing symbols and pointer to parent enclosing scope */
struct Scope {
    int id;
    int level;
    char description[64];
    Symbol *symbols;
    Scope *parent;
};

typedef void (*ScopeExitHook)(Scope *scope, void *userdata);

/* Symbol table maintaining scope stack and history */
typedef struct {
    Scope *current_scope;
    int current_level;
    Scope **all_scopes;
    size_t all_scopes_count;
    size_t all_scopes_capacity;
    ScopeExitHook on_exit_scope;
    void *hook_userdata;
} SymbolTable;

/* Initializes a symbol table and enters the global scope */
void symtab_init(SymbolTable *st);

/* Pushes a new nested scope onto the stack with a description */
void symtab_push_scope(SymbolTable *st, const char *desc);

/* Pops the current scope from the stack after invoking the exit hook */
void symtab_pop_scope(SymbolTable *st);

/* Declares a symbol in the current scope, returning any prior symbol in the same scope */
Symbol *symtab_declare(SymbolTable *st, const char *name, SymbolKind kind,
                       const char *type_str, AstNode *decl_node,
                       int line, int column, bool is_init);

/* Looks up a symbol from the innermost scope outwards */
Symbol *symtab_lookup(const SymbolTable *st, const char *name);

/* Looks up a symbol only in the current active scope */
Symbol *symtab_lookup_in_current_scope(const SymbolTable *st, const char *name);

/* Registers a callback hook invoked before any scope is popped */
void symtab_set_exit_hook(SymbolTable *st, ScopeExitHook hook, void *userdata);

/* Prints all recorded scopes and their symbols with flags */
void symtab_dump(const SymbolTable *st);

/* Releases all scopes and symbols allocated by the symbol table */
void symtab_free(SymbolTable *st);

/* Traverses the AST once, binding identifiers to symbols and setting usage flags */
void resolver_resolve(AstNode *root, SymbolTable *st, DiagList *diags);

#endif /* SYMTAB_H */
