# System Architecture & Design Specification

This document details the architectural design, formal grammar, compiler phase mappings, and internal algorithms of the **Static Code Quality Analyzer**.

---

## 1. System Architecture & Pipeline Diagram

The analyzer operates as a classic single-pass compiler front-end with decoupled checking passes. Source code is ingested from disk, tokenized into lexical tokens, parsed into an Abstract Syntax Tree (AST), resolved against a hierarchical symbol table, and evaluated through an extensible table of static rule checkers.

```
+-----------------------------------------------------------------------------+
|                                Source File                                  |
|                                (e.g. foo.c)                                 |
+-----------------------------------------------------------------------------+
                                       |
                                       v
                     +-----------------------------------+
                     |      Lexical Scanner (lexer.c)    |
                     |  - Tokenizes stream               |
                     |  - Strips whitespace & comments   |
                     |  - Tracks 1-based line/col        |
                     +-----------------------------------+
                                       |
                                       | Token Stream
                                       v
                     +-----------------------------------+
                     |     Syntax Parser (parser.c)      |
                     |  - Recursive descent statements   |
                     |  - Precedence climbing exprs      |
                     |  - Panic-mode recovery            |
                     +-----------------------------------+
                                       |
                                       | Abstract Syntax Tree
                                       v
                     +-----------------------------------+
                     |     Scope Resolver (symtab.c)     |
                     |  - Multi-level scope stack        |
                     |  - Binds identifiers to symbols   |
                     |  - Tracks used/assigned flags     |
                     +-----------------------------------+
                                       |
                                       | AST + Symbol Table
                                       v
                     +-----------------------------------+
                     |     Quality Checker (checks.c)    |
                     |  - Table of independent rules     |
                     |  - AST walkers (W001 - W014)      |
                     |  - Configurable thresholds        |
                     +-----------------------------------+
                                       |
                                       | Diagnostics List
                                       v
                     +-----------------------------------+
                     |   Console Reporter (report_*.c)   |
                     |  - Source gutter & caret snippet  |
                     |  - ANSI severity color-coding     |
                     |  - Summary & remediation advice   |
                     +-----------------------------------+
```

---

## 2. Compiler Phases Mapping

In classical compiler design (e.g., Aho, Lam, Sethi, Ullman — the "Dragon Book"), a compiler is structured into distinct phases:

| Classical Compiler Phase | Corresponding Analyzer Module | Implementation Responsibility |
|:-------------------------|:------------------------------|:------------------------------|
| **1. Lexical Analysis** | [src/lexer.c](file:///Users/dhanush/cd%20project/src/lexer.c), [src/lexer.h](file:///Users/dhanush/cd%20project/src/lexer.h) | Converts raw source byte buffer into typed tokens (`Token`), discards whitespace, removes line and block comments, skips preprocessor directives, and records line/column coordinates. |
| **2. Syntax Analysis** | [src/parser.c](file:///Users/dhanush/cd%20project/src/parser.c), [src/parser.h](file:///Users/dhanush/cd%20project/src/parser.h), [src/ast.c](file:///Users/dhanush/cd%20project/src/ast.c), [src/ast.h](file:///Users/dhanush/cd%20project/src/ast.h) | Validates grammatical correctness against the C subset grammar using recursive-descent parsing for statements and precedence climbing for binary/unary expressions. Builds the Abstract Syntax Tree. |
| **3. Semantic Analysis** | [src/symtab.c](file:///Users/dhanush/cd%20project/src/symtab.c), [src/symtab.h](file:///Users/dhanush/cd%20project/src/symtab.h), [src/checks.c](file:///Users/dhanush/cd%20project/src/checks.c), [src/checks.h](file:///Users/dhanush/cd%20project/src/checks.h) | Constructs lexical scope hierarchy, declares symbols, detects redeclaration conflicts, binds identifier AST nodes to symbol table entries, tracks read/write usage flags, and executes static rule passes. |
| **4. Intermediate Code Generation** | *Omitted* | **Not Applicable.** |
| **5. Code Optimization** | *Omitted* | **Not Applicable.** |
| **6. Target Code Generation** | *Omitted* | **Not Applicable.** |

### Why There Is No Code Generation Phase

Traditional compilers (such as GCC, Clang, or MSVC) exist to **transform** source code into an executable machine artifact (LLVM IR, assembly, or binary machine code). 

In contrast, this project is a **Static Code Quality Analyzer**:
1. **Analysis, Not Execution**: The explicit lab requirement is that the tool **must NEVER execute the analyzed code**.
2. **Safety & Security Goal**: Its mission is to audit, inspect, and certify source code quality, detect safety violations (buffer overflow potentials, division by zero), and flag architectural smells (deep nesting, oversized functions).
3. **AST as the Final Representation**: In static analysis, the decorated AST and Symbol Table represent the ultimate data structures required to emit diagnostics. Lowering to intermediate bytecode or machine code would lose critical source-level abstractions (variable names, scope boundaries, token positions, and syntactic constructs).

---

## 3. Lexical Analyzer (Scanner)

The scanner (`src/lexer.c`) operates directly over an in-memory buffer without dynamic memory allocation per token:
- **Token Structure**:
  ```c
  typedef struct {
      TokenKind type;
      const char *start;
      size_t length;
      int line;
      int column;
  } Token;
  ```
- **Zero-Copy Lexing**: `tok.start` points directly into the source string buffer, and `tok.length` specifies byte span.
- **Escape Sequence Processing**: String and character literals handle common escape codes (`\n`, `\t`, `\\`, `\'`, `\"`, `\0`).
- **Preprocessor Line Handling**: Lines beginning with `#` are skipped from the `#` to the newline, with support for backslash (`\`) line continuation.
- **Unknown Token Recovery**: Non-matching characters trigger diagnostic `E004: unknown character` and advance by one character to prevent infinite scanner loops.

---

## 4. Formal Grammar (EBNF)

The supported language subset is defined by the following formal Extended Backus-Naur Form (EBNF) grammar:

```ebnf
TranslationUnit   ::= { ExternalDecl } ;

ExternalDecl      ::= FunctionDef | VarDeclList | UnsupportedDecl ;

UnsupportedDecl   ::= ( "struct" | "union" | "enum" | "typedef" ) ... ;

TypeSpecifier     ::= [ "unsigned" | "signed" | "short" | "long" ]
                      ( "int" | "char" | "float" | "double" | "void" ) ;

Pointer           ::= { "*" } ;

FunctionDef       ::= TypeSpecifier Pointer IDENTIFIER "(" [ ParameterList ] ")" ( CompoundStmt | ";" ) ;

ParameterList     ::= Parameter { "," Parameter } ;
Parameter         ::= TypeSpecifier Pointer IDENTIFIER [ "[" [ INT_LITERAL ] "]" ]
                    | "void" ;

VarDeclList       ::= TypeSpecifier Declarator { "," Declarator } ";" ;
Declarator        ::= Pointer IDENTIFIER [ "[" [ INT_LITERAL ] "]" ] [ "=" AssignmentExpr ] ;

Statement         ::= CompoundStmt
                    | IfStmt
                    | WhileStmt
                    | ForStmt
                    | DoWhileStmt
                    | SwitchStmt
                    | CaseStmt
                    | DefaultStmt
                    | ReturnStmt
                    | BreakStmt
                    | ContinueStmt
                    | ExprStmt
                    | EmptyStmt ;

CompoundStmt      ::= "{" { VarDeclList | Statement } "}" ;

IfStmt            ::= "if" "(" Expression ")" Statement [ "else" Statement ] ;
WhileStmt         ::= "while" "(" Expression ")" Statement ;
ForStmt           ::= "for" "(" [ VarDeclList | ExprStmt | ";" ] [ Expression ] ";" [ Expression ] ")" Statement ;
DoWhileStmt       ::= "do" Statement "while" "(" Expression ")" ";" ;
SwitchStmt        ::= "switch" "(" Expression ")" Statement ;
CaseStmt          ::= "case" Expression ":" ;
DefaultStmt       ::= "default" ":" ;
ReturnStmt        ::= "return" [ Expression ] ";" ;
BreakStmt         ::= "break" ";" ;
ContinueStmt      ::= "continue" ";" ;
ExprStmt          ::= Expression ";" ;
EmptyStmt         ::= ";" ;

Expression        ::= AssignmentExpr ;
AssignmentExpr    ::= LogicalOrExpr [ ( "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^=" | "<<=" | ">>=" ) AssignmentExpr ] ;
LogicalOrExpr     ::= LogicalAndExpr { "||" LogicalAndExpr } ;
LogicalAndExpr    ::= BitOrExpr { "&&" BitOrExpr } ;
BitOrExpr         ::= BitXorExpr { "|" BitXorExpr } ;
BitXorExpr        ::= BitAndExpr { "^" BitAndExpr } ;
BitAndExpr        ::= EqualityExpr { "&" EqualityExpr } ;
EqualityExpr      ::= RelationalExpr { ( "==" | "!=" ) RelationalExpr } ;
RelationalExpr    ::= ShiftExpr { ( "<" | "<=" | ">" | ">=" ) ShiftExpr } ;
ShiftExpr         ::= AdditiveExpr { ( "<<" | ">>" ) AdditiveExpr } ;
AdditiveExpr      ::= MultiplicativeExpr { ( "+" | "-" ) MultiplicativeExpr } ;
MultiplicativeExpr::= UnaryExpr { ( "*" | "/" | "%" ) UnaryExpr } ;

UnaryExpr         ::= ( "++" | "--" | "+" | "-" | "!" | "~" | "*" | "&" ) UnaryExpr
                    | PostfixExpr ;

PostfixExpr       ::= PrimaryExpr { "[" Expression "]"
                                  | "(" [ ArgumentList ] ")"
                                  | "++"
                                  | "--" } ;

PrimaryExpr       ::= IDENTIFIER
                    | INT_LITERAL
                    | FLOAT_LITERAL
                    | CHAR_LITERAL
                    | STRING_LITERAL
                    | "(" Expression ")" ;

ArgumentList      ::= AssignmentExpr { "," AssignmentExpr } ;
```

---

## 5. Abstract Syntax Tree (AST) Design

The AST is composed of uniform, heap-allocated `AstNode` structs defined in [src/ast.h](file:///Users/dhanush/cd%20project/src/ast.h):

```c
struct AstNode {
    NodeKind kind;
    int line;
    int column;
    int end_line;
    bool is_parenthesized;
    AstNode *next;
    union {
        /* Kind-specific payloads: translation_unit, function_def, block, if_stmt, ... */
    } data;
};
```

### Node Kind Inventory

| Enum Kind | Description | Payload Data Fields |
|:----------|:------------|:-------------------|
| `NODE_TRANSLATION_UNIT` | Root program node | `declarations` (linked sibling chain) |
| `NODE_FUNCTION_DEF` | Function definition/prototype | `return_type`, `pointer_depth`, `name`, `params`, `body` |
| `NODE_PARAM` | Function parameter | `type_name`, `pointer_depth`, `name`, `is_array`, `array_size` |
| `NODE_VAR_DECL` | Variable declaration | `type_name`, `pointer_depth`, `name`, `is_array`, `array_size`, `init` |
| `NODE_BLOCK` | Compound `{ ... }` block | `statements` (linked sibling chain) |
| `NODE_IF` | If-Else statement | `condition`, `then_branch`, `else_branch` |
| `NODE_WHILE` | While loop | `condition`, `body` |
| `NODE_FOR` | For loop | `init`, `condition`, `step`, `body` |
| `NODE_DO_WHILE` | Do-While loop | `body`, `condition` |
| `NODE_SWITCH` | Switch statement | `expr`, `body` |
| `NODE_CASE` | Switch case label | `expr`, `body` |
| `NODE_DEFAULT` | Switch default label | `body` |
| `NODE_RETURN` | Return statement | `expr` |
| `NODE_BREAK` | Break statement | None |
| `NODE_CONTINUE` | Continue statement | None |
| `NODE_EXPR_STMT` | Expression statement | `expr` |
| `NODE_EMPTY_STMT` | Lone semicolon `;` | None |
| `NODE_BINARY` | Binary infix operator | `op`, `left`, `right` |
| `NODE_UNARY` | Unary prefix/postfix operator | `op`, `operand`, `is_postfix` |
| `NODE_ASSIGN` | Assignment operator | `op`, `left`, `right` |
| `NODE_CALL` | Function call | `callee`, `args` (linked sibling chain) |
| `NODE_INDEX` | Array indexing `a[i]` | `array`, `index` |
| `NODE_IDENTIFIER` | Named variable/symbol reference | `name`, `symbol` (resolved pointer) |
| `NODE_INT_LITERAL` | Integer literal token | `value` string |
| `NODE_FLOAT_LITERAL`| Float literal token | `value` string |
| `NODE_CHAR_LITERAL` | Character literal token | `value` string |
| `NODE_STRING_LITERAL`| String literal token | `value` string |
| `NODE_TERNARY` | Conditional expression `? :` | `condition`, `true_expr`, `false_expr` |

---

## 6. Symbol Table & Lexical Scopes

Symbol management is implemented in [src/symtab.c](file:///Users/dhanush/cd%20project/src/symtab.c) and [src/symtab.h](file:///Users/dhanush/cd%20project/src/symtab.h):

- **Scope Stack Structure**:
  ```c
  struct Scope {
      int id;
      int level;              /* 0 = global, 1 = function, 2+ = nested blocks */
      char description[64];
      Symbol *symbols;        /* Singly-linked list of symbols declared in scope */
      Scope *parent;          /* Enclosing parent scope pointer */
  };
  ```
- **Symbol Records**:
  ```c
  struct Symbol {
      char *name;
      SymbolKind kind;        /* SYM_VARIABLE, SYM_PARAMETER, SYM_FUNCTION */
      char *type_str;
      AstNode *decl_node;
      int line;
      int column;
      bool used;              /* Marked true when read */
      bool assigned;          /* Marked true when written */
      bool initialized_at_declaration;
      bool is_global;
      int scope_level;
      Symbol *next;
  };
  ```
- **Resolution Pass (`resolver_resolve`)**:
  - Traverses the AST once prior to rule execution.
  - Pushes a new scope on entering functions, compound blocks, `for` loop headers, and `switch` statements.
  - Binds each `NODE_IDENTIFIER` directly to its resolved `Symbol *`.
  - Sets `assigned = true` for write targets (left side of `=` and compound assignments, operands of `++`/`--`).
  - Sets `used = true` for reads (expressions, condition tests, function arguments, right side of assignments).
  - Handles undeclared identifiers (e.g., standard library functions `printf`, `strcpy`) by recording `I002` notes without crashing.

---

## 7. Static Rule Checkers Architecture

All quality, complexity, and safety checks are registered in a master dispatch table in [src/checks.c](file:///Users/dhanush/cd%20project/src/checks.c):

```c
typedef void (*CheckRuleFn)(const AstNode *root, const SymbolTable *st, DiagList *diags);

typedef struct {
    const char *id;
    const char *name;
    DiagSeverity severity;
    const char *description;
    CheckRuleFn fn;
    bool enabled;
} Rule;
```

### High-Level Rule Mechanics

1. **W001 (Unused Variable)**: Iterates over all scopes in `st->all_scopes`. For every local variable (`level > 0`), if `!used && !assigned`, emits `WARNING [W001]`; if `!used && assigned`, emits `INFO [W001]`.
2. **W002 (Assignment in Condition)**: Inspects condition trees in `if`, `while`, `for`, `do-while`, and ternary expressions. Flags any top-level unparenthesized `NODE_ASSIGN` with operator `"="'`.
3. **W003 (Unreachable Code)**: Walks statements within compound blocks. When an unconditional terminating statement (`return`, `break`, `continue`, or an `if-else` where both branches terminate) is encountered, subsequent sibling statements before the next `case`/`default` label are flagged.
4. **W004 (Missing Return)**: Checks non-void function definitions. If `stmt_always_terminates(body)` evaluates to false, reports missing return path. Accurately treats infinite loops (`while(1)` or `for(;;)` without `break`) as terminating control paths.
5. **W005 (Use Before Assignment)**: Flow-insensitive scanner tracking local variable states. Flags reads of uninitialized local scalar variables occurring prior to any assignment statement.
6. **W006 (Magic Numbers)**: Traverses expression trees and flags numeric literals other than `0, 1, 2, -1`. Skips variable declaration initializers, const initializers, and array size specifiers.
7. **W007 (Deep Nesting)**: Computes statement nesting depth across `if`, `while`, `for`, `do-while`, and `switch`. Tracks maximum depth and emits a single warning per function at the deepest point exceeding `g_checks_config.max_nesting_depth`.
8. **W008 (Long Function)**: Evaluates `func->end_line - func->line + 1`. Flags functions spanning more lines than `g_checks_config.max_function_lines`.
9. **W009 (Empty Body)**: Detects empty blocks `{}` or lone semicolons `;` as bodies of `if`, `else`, `while`, and `for`. Exempts idiomatic `for(;;);`.
10. **W010 (Switch Without Default)**: Inspects statements inside switch blocks; flags any `switch` lacking a `NODE_DEFAULT` label.
11. **W011 (Unsafe Function)**: Inspects `NODE_CALL` callee names. Flags `gets` (`ERROR`), `strcpy`, `strcat`, `sprintf` (`WARNING`), and checks `scanf` format string arguments for unbounded `%s`.
12. **W012 (Variable Shadowing)**: For every local variable or parameter in each scope, walks parent scopes `sc->parent`. If an outer variable with an identical identifier name exists, flags shadowing.
13. **W013 (Division by Zero)**: Traverses binary operations (`/`, `%`) and compound assignments (`/=`, `%=`). Flags operations where the right-hand operand is integer literal `0`.
14. **W014 (Fall-Through Case)**: Inspects switch case statements. If a case label has statements that do not conclude with `break`, `return`, or `continue`, and is followed by another case or default label, flags implicit fall-through.
