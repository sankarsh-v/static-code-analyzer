# Compiler Design Lab — Viva Questions & Answers

This document provides 25 targeted viva and oral examination questions with direct, concise, and technically accurate answers based specifically on the implementation of this **Static Code Quality Analyzer**.

---

### Q1: What is the fundamental difference between this tool and a traditional C compiler like GCC or Clang?
**Answer:** A traditional C compiler (GCC/Clang) translates source code into machine code or assembly through frontend, intermediate optimization, and backend code-generation phases, intended for execution. This analyzer is an **analysis-only compiler front-end**: it tokenizes, parses into an AST, and resolves symbols solely to audit, inspect, and detect bugs, smells, and vulnerabilities. It **never executes** the code and contains **no code generation phase**.

---

### Q2: Why is there no code generation phase in this project?
**Answer:** Code generation lowers high-level constructs into low-level instructions (assembly/bytecode). In static code analysis, lowering discards essential source-level context—variable names, scope boundaries, line/column numbers, and syntactic structure. Because the tool's goal is diagnostic reporting and code quality evaluation rather than program execution, the decorated AST and Symbol Table represent the final, optimal data representations.

---

### Q3: How does this tool differ from industrial static analyzers like SonarQube or ESLint?
**Answer:** Tools like SonarQube or ESLint typically run over external JavaScript/Java ASTs or compiler plugin hooks with multi-pass data-flow frameworks and complex rule plugins. This project is a completely self-contained, dependency-free C11 compiler front-end built from scratch: it includes its own custom hand-written lexer, recursive-descent parser, scope stack symbol table, and AST traversal engine with zero external libraries.

---

### Q4: Why choose recursive descent for the parser rather than an LR / LALR parser generator like Yacc or Bison?
**Answer:** 
1. **No External Dependencies**: Fulfills the project constraint of pure C11 standard library code.
2. **Superior Error Diagnostics & Recovery**: Recursive descent enables precise panic-mode recovery (`synchronize`) and readable, contextual syntax error messages with expected vs. found token details.
3. **Natural Mapping to Grammar**: Each non-terminal in the EBNF grammar maps directly to a clean C function (`parse_if`, `parse_while`, `parse_block`), making the codebase easy to explain and debug in an academic viva.

---

### Q5: How does the expression parser implement precedence climbing, and what problem does it solve?
**Answer:** Pure recursive descent for expressions requires one function per precedence level (15+ functions in C), which creates deep call stacks and overhead. **Precedence climbing** (`parse_expr_precedence` in [src/parser.c](file:///Users/dhanush/cd%20project/src/parser.c)) uses a single loop with a numeric precedence table: it consumes an operator only if its binding power is greater than or equal to the current minimum precedence, climbing upwards for higher-precedence operators and correctly handling associativity.

---

### Q6: How does the parser resolve the classic "dangling else" ambiguity?
**Answer:** In [src/parser.c](file:///Users/dhanush/cd%20project/src/parser.c#L368-L372), `parse_if` parses the `then` statement and immediately checks `match_kw(p, "else")`. By greedily binding the `else` token to the innermost actively parsing `if` statement, it implements the standard C grammar rule that an `else` always attaches to the closest preceding `if`.

---

### Q7: How does the lexical scanner achieve zero-copy tokenization?
**Answer:** The `Token` struct stores `const char *start` and `size_t length` pointing directly into the contiguous source buffer loaded in memory. The scanner never allocates separate heap strings for identifiers or operators during tokenization; heap allocation (`strdup`) only occurs when nodes are permanently committed to the AST.

---

### Q8: How does the lexer handle preprocessor directives like `#include` and `#define`?
**Answer:** In `skip_whitespace_and_comments` in [src/lexer.c](file:///Users/dhanush/cd%20project/src/lexer.c), when `#` is encountered at the start of a logical line, the scanner advances through all characters until the terminating newline `\n`. It explicitly checks for backslash line continuations (`\` followed by `\n` or `\r\n`), advancing across continued lines so macros spanning multiple lines are skipped cleanly without emitting stray tokens.

---

### Q9: How does the symbol table represent and manage nested lexical scopes?
**Answer:** [src/symtab.h](file:///Users/dhanush/cd%20project/src/symtab.h) uses a tree of `Scope` records linked via `parent` pointers. Global scope is level 0, function body is level 1, and each nested block `{ ... }`, `for` loop, or `switch` creates a child scope with `level = parent->level + 1`. Entering a block calls `symtab_push_scope`; exiting calls `symtab_pop_scope`. Looking up an identifier searches from `current_scope` outward through `parent` pointers to the global scope.

---

### Q10: What is the purpose of the resolver pass (`resolver_resolve`), and why does it run before check rules?
**Answer:** The resolver pass ([src/symtab.c](file:///Users/dhanush/cd%20project/src/symtab.c#L265)) performs semantic analysis in a single pass over the AST. It declares symbols in their proper lexical scope, detects duplicate declarations in the same scope (`E200`), binds every `NODE_IDENTIFIER` directly to its `Symbol *` pointer, and marks `used` and `assigned` flags. This decouples semantic binding from check rules, so rules like `W001` or `W012` do not need to repeat scope lookups.

---

### Q11: How does the analyzer detect variable shadowing (Rule W012)?
**Answer:** In `check_w012_variable_shadowing` in [src/checks.c](file:///Users/dhanush/cd%20project/src/checks.c), the checker iterates through all recorded local scopes (`level > 0`). For each declared `SYM_VARIABLE` or `SYM_PARAMETER`, it traverses ancestor scopes (`sc->parent`). If an ancestor scope declares an identical identifier name, it emits a warning pinpointing both the shadowing declaration line and the outer declaration line.

---

### Q12: How does the analyzer differentiate between an unused variable and an assigned-but-never-read variable (Rule W001)?
**Answer:** The resolver tracks two distinct boolean flags on each `Symbol`:
- `used`: Set when the variable is read (e.g., in expressions, conditions, function call arguments, or right-hand side of assignments).
- `assigned`: Set when the variable is written to (left-hand side of `=`, compound assignments, `++`/`--`).
If `!used && !assigned`, it emits `WARNING [W001]: unused variable`. If `!used && assigned`, it emits `INFO [W001]: variable assigned but never read`.

---

### Q13: How does Rule W003 detect unreachable code after return, break, or continue?
**Answer:** In `scan_block_reachability` in [src/checks.c](file:///Users/dhanush/cd%20project/src/checks.c), the checker inspects statements in each compound block sequentially with a `terminated` flag. When a statement unconditionally terminates flow (`NODE_RETURN`, `NODE_BREAK`, `NODE_CONTINUE`, or an `if-else` where both branches terminate), `terminated` becomes true. Any subsequent sibling statement triggers `WARNING [W003]`. When a `case` or `default` label is reached in a switch block, `terminated` resets to false because labels are reachable branch targets.

---

### Q14: How does Rule W004 distinguish functions missing a return from intentional infinite loops like `while(1)`?
**Answer:** Helper `stmt_always_terminates` checks if a statement guarantees execution will not reach the end of the function body. For `while (1)` (condition is integer literal non-zero) or `for (;;)` (condition is NULL), it recursively inspects the loop body with `contains_break`. If the loop body contains no `break` statements, control can never exit the loop, so control never reaches the closing brace—correctly exempting server loops from missing return warnings.

---

### Q15: How does Rule W002 avoid false positives when assignment in a condition is intentional?
**Answer:** During parsing, `parse_primary` marks parenthesized expressions with `is_parenthesized = true`. Rule W002 inspects condition expressions: if the top-level node is `NODE_ASSIGN` with operator `"="` and `!is_parenthesized`, it flags `WARNING [W002]`. When written with double parentheses `if ((x = read()))` or compared `if ((c = getchar()) != EOF)`, `is_parenthesized` is true or the top-level node is `NODE_BINARY`, avoiding false positives.

---

### Q16: How does Rule W006 detect magic numbers while exempting variable declaration initializers and array sizes?
**Answer:** When walking the AST in `walk_magic_numbers`, `NODE_VAR_DECL` statements are intercepted and their `init` expressions are deliberately not traversed. Array sizes are stored as integer fields on declarations rather than expression nodes. When expressions inside statements or conditions are evaluated, literals are checked: `0`, `1`, `2`, and unary minus on `1` (`-1`) are allowed; all other numeric literals trigger `INFO [W006]`.

---

### Q17: How does Rule W009 detect empty bodies while exempting `for(;;);`?
**Answer:** In `walk_empty_bodies`, `is_empty_body` checks if a branch is `NODE_EMPTY_STMT` (lone semicolon) or `NODE_BLOCK` with `statements == NULL`. For `for` loops, it checks if `init == NULL && condition == NULL && step == NULL`. If all three are null, it recognizes the idiom `for(;;);` as an intentional infinite loop and exempts it.

---

### Q18: How does Rule W011 detect unsafe `scanf` calls without fixed buffer limits?
**Answer:** When `inspect_unsafe_call` detects a call to `scanf`, it inspects the first argument (`args`). If it is a `NODE_STRING_LITERAL`, helper `scanf_has_unbounded_s` iterates through the format string. If a `%s` directive appears without any preceding width digits (e.g., `%s` instead of `%31s`), it triggers `WARNING [W011]`. Format strings containing `%d` or `%31s` do not trigger the warning.

---

### Q19: How does Rule W013 detect division by zero across both simple division and compound assignments?
**Answer:** In `inspect_div_zero_expr`, the checker inspects both `NODE_BINARY` (operators `"/"` and `"%""`) and `NODE_ASSIGN` (operators `"/="` and `"%="`). If the right-hand operand is `NODE_INT_LITERAL` and evaluates via `strtol` to `0`, it emits `ERROR [W013]`.

---

### Q20: How does Rule W014 distinguish multi-case matching (`case 1: case 2:`) from unintended fall-through?
**Answer:** `check_switch_fall_through` scans statements between case labels in a switch block. If `case 1:` is immediately followed by `case 2:` with zero intermediate statements (`stmt_count == 0`), it recognizes an intentional multi-case label and skips it. Only if a case label has one or more statements (`stmt_count > 0`), is followed by another case/default label, and does not end with `break`, `return`, or `continue`, does it report `INFO [W014]`.

---

### Q21: What error recovery strategy does the parser use when encountering syntax errors?
**Answer:** The parser uses **panic-mode error recovery** via `synchronize(p)`. When an expected token is missing, `emit_expected_error` emits diagnostic `E100` and enters panic mode. `synchronize` discards tokens until it finds a statement boundary—either a semicolon `;`, a closing brace `}`, or a synchronizing keyword (`if`, `while`, `for`, `return`, `int`, etc.). This prevents cascading false errors on subsequent lines.

---

### Q22: How does `ast_free` avoid stack overflow when deallocating huge programs like 5,000-line files?
**Answer:** In `ast_free` ([src/ast.c](file:///Users/dhanush/cd%20project/src/ast.c#L154-L161)), sibling chains (nodes connected via `node->next`) are freed using an iterative `while (node)` loop rather than tail recursion. Subtrees (children payloads) are freed via recursive helper calls. This ensures that a 5,000-statement sibling chain consumes $O(1)$ stack frames during deallocation.

---

### Q23: Why can static analysis mathematically never find every possible runtime bug?
**Answer:** By **Rice's Theorem** and Turing's **Halting Problem**, any non-trivial semantic property of a Turing-complete program (such as whether an arbitrary division by zero will execute or whether a program terminates) is undecidable at compile time. Static analyzers must therefore make trade-offs: sound over-approximation (potential false positives) or heuristic under-approximation (potential false negatives).

---

### Q24: What are the main sources of false positives or limitations in this analyzer?
**Answer:**
1. **Flow-Insensitive Initialization (W005)**: The checker does not build a full control-flow graph (CFG). If a variable is conditionally initialized in an `if` branch and read later, W005 may emit a false positive warning.
2. **Preprocessor Evaluation**: Preprocessor `#define` constants and `#include` files are stripped by the scanner; macros are not expanded into the AST.
3. **Unsupported C Constructs**: `struct`, `union`, `enum`, `typedef`, and explicit type casts are skipped as out of lab scope.

---

### Q25: What is the asymptotic time and space complexity of the analysis pipeline?
**Answer:**
- **Time Complexity**: **$O(N + S \cdot D)$**, where $N$ is source length in bytes, $S$ is the number of declared symbols, and $D$ is scope depth. Scanning is $O(N)$, recursive descent parsing is $O(N)$ with no backtracking, resolver is $O(N)$, and check passes are linear walks over the AST. Shadowing checks take $O(S \cdot D)$ where $D \le 10$ in typical programs. Total runtime is effectively linear, processing 5,000 lines in $< 0.05$ seconds.
- **Space Complexity**: **$O(N)$**, proportional to the size of the source buffer, AST node count, and symbol table records, all of which are reclaimed upon completion.
