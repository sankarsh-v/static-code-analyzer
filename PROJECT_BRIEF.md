# PROJECT_BRIEF.md — Static Code Quality Analyzer

## Project Overview
A university Compiler Design Lab project in C: a **Static Code Quality Analyzer**. It reads a C source file, tokenizes it, parses it into an Abstract Syntax Tree (AST), executes rule checks over the AST, and reports issues to the console and to a self-contained HTML file. **It must NEVER execute the analysed code.**

---

## 1. Language & Toolchain
- **Implementation Language:** C11 standard.
- **Compiler Flags:** `gcc -std=c11 -Wall -Wextra -Wpedantic`.
- **Dependencies:** Standard library only (`<stdio.h>`, `<stdlib.h>`, `<string.h>`, `<stdbool.h>`, `<ctype.h>`, etc.). No external dependencies.
- **Portability:** Builds with a standard Makefile on Linux, macOS, and Windows (MinGW).

---

## 2. Analysed Language Subset
- **Types:** `int`, `char`, `float`, `double`, `long`, `short`, `unsigned`, `void`, pointers (`*`), and one-dimensional arrays (`[]`).
- **Statements:**
  - Variable declarations
  - Expression statements
  - `if` / `else`
  - `while`
  - `for`
  - `do` ... `while`
  - `switch` / `case` / `default`
  - `return`
  - `break`
  - `continue`
  - Compound statement blocks (`{ ... }`)
- **Expressions:**
  - Common C operators (arithmetic, relational, logical, bitwise)
  - Function calls
  - Array indexing
  - Unary and binary operators
  - Assignment and compound assignments (`+=`, `-=`, etc.)
  - Ternary conditional (`? :`)
  - Casts may be skipped.
- **Preprocessor:** Lines starting with `#` are skipped by the lexer.
- **Structs / Unions / Typedefs:** Out of scope. The parser must not crash on them; it must report a clear `'unsupported construct'` diagnostic and recover gracefully.

---

## 3. Architecture & Components
- **Lexer:** Hand-written lexical analyzer (scanner).
- **Parser:** Hand-written recursive-descent parser with precedence climbing for expressions.
- **AST:** Tagged-union node struct representing program syntax.
- **Symbol Table:** Scope-aware symbol table with a scope stack for variable/function tracking.
- **Check Passes:** Rule checkers that walk the AST to detect smells, complexity, and safety issues.
- **Diagnostics:** Diagnostic list tracking detected issues with severity, line/col, rule ID, message, and suggestion.
- **Reporters:**
  - Console reporter (`report_console.c/.h`)
  - Standalone HTML reporter (`report_html.c/.h`)

---

## 4. File Structure
```
├── Makefile
├── PROJECT_BRIEF.md
├── src/
│   ├── main.c
│   ├── lexer.h / lexer.c
│   ├── ast.h / ast.c
│   ├── parser.h / parser.c
│   ├── symtab.h / symtab.c
│   ├── checks.h / checks.c
│   ├── diag.h / diag.c
│   ├── report_console.h / report_console.c
│   └── report_html.h / report_html.c
├── tests/
└── samples/
```

---

## 5. Diagnostic Structure
Each diagnostic issue contains:
- `rule_id`: String code (e.g., `"W001"`, `"E001"`, `"S001"`)
- `severity`: `DIAG_ERROR`, `DIAG_WARNING`, `DIAG_INFO`
- `line`: 1-based source line number
- `column`: 1-based source column number
- `message`: Clear explanation of the detected defect
- `suggestion`: Remediation advice or suggested fix

---

## 6. Code Quality Rules for This Project
- **Memory Management:** Every allocated node/buffer must be freed (`ast_free` for AST). Zero leaks.
- **State Management:** No global mutable state except where explicitly noted.
- **Documentation:** Every function must have a concise one-line comment.
- **Function Length:** No function may exceed 60 lines.
- **Sanitizers:** Code must compile cleanly with zero warnings under `-Wall -Wextra -Wpedantic` and run clean under `-fsanitize=address,undefined` or `valgrind`.

---

## 7. CLI Specification
- **Command Syntax:**
  ```bash
  analyzer <file.c> [--html report.html] [--quiet]
  ```
- **Exit Codes:**
  - `0`: Analysis completed successfully with no `WARNING` or `ERROR` diagnostics.
  - `1`: Analysis completed, but one or more `WARNING` or `ERROR` diagnostics were found.
  - `2`: Command line usage error, file access failure, or invalid argument.
