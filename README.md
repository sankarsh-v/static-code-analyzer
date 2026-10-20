# Static Code Quality Analyzer

A standalone, non-executing static code quality and security analyzer for C written in C11. Built from scratch for a university Compiler Design Lab project, the analyzer implements a full compiler front-end pipeline—lexical scanner, recursive-descent parser with precedence climbing, AST generator, and lexical scope resolver—to detect bugs, code smells, complexity hotspots, and security vulnerabilities without ever running the analyzed code.

---

## Features

- **Zero External Dependencies**: Built strictly using the C11 standard library (`<stdio.h>`, `<stdlib.h>`, `<string.h>`, `<stdbool.h>`, `<ctype.h>`).
- **Complete Hand-Written Pipeline**:
  - Hand-crafted scanner tracking 1-based line and column positions.
  - Recursive-descent parser with Pratt-style precedence climbing for binary/unary expressions.
  - Typed Abstract Syntax Tree (AST) with automated memory reclamation (`ast_free`).
  - Lexical scope stack with identifier resolution, usage flags (`used`, `assigned`), and shadowing detection.
- **14 Built-In Quality & Safety Rules**: Detects undefined behavior, division by zero, dangerous libc calls (`gets`, `strcpy`, unbounded `scanf`), variable shadowing, unreachable code, deep nesting, magic numbers, and missing returns.
- **Compiler-Grade Diagnostic Output**: GCC/Clang-style terminal diagnostic reports featuring source gutters, caret indicators, severity color-coding, and actionable remediation suggestions.
- **Cross-Platform Portability**: Builds cleanly on macOS, Linux, and Windows (MinGW / MSYS2 / WSL).
- **Rock-Solid Robustness**: Tested against binary garbage, 5,000-line files, 200-level nested parentheses, unmatched braces, and non-newline terminations with zero leaks or crashes.

---

## Build Instructions

### Prerequisites
- C11 compliant compiler (`gcc` or `clang`)
- GNU `make`

### Linux & macOS
```bash
# Clone or navigate to the project directory
cd "cd project"

# Clean build
make clean && make

# Build with AddressSanitizer and UndefinedBehaviorSanitizer
make asan
```

### Windows (MinGW / MSYS2 / Git Bash)
From MSYS2 or Git Bash:
```bash
make clean && make
```
Or directly from Windows Command Prompt with MinGW:
```cmd
mingw32-make clean
mingw32-make
```

---

## Usage & CLI Options

```bash
analyzer <file.c> [--html out.html] [--quiet] [--dump-tokens] [--dump-ast] [--dump-symbols]
```

### Command Line Flags

| Option | Description |
|:-------|:------------|
| `<file.c>` | Path to the C source file to analyze. |
| `--quiet` | Suppresses individual issue snippets and prints only the summary. |
| `--dump-tokens` | Emits the scanned token stream with line:col coordinates and stops. |
| `--dump-ast` | Prints the indented syntax tree hierarchy to stdout. |
| `--dump-symbols` | Emits all recorded scopes, symbols, types, and usage flags. |
| `--html <file>` | Placeholder for self-contained HTML report export. |

### Exit Codes
- `0`: Analysis completed cleanly (no `WARNING` or `ERROR` diagnostics).
- `1`: Analysis completed, but one or more `WARNING` or `ERROR` diagnostics were reported.
- `2`: Command-line usage error, unrecognized flag, or file access failure.

---

## Console Output Examples (Screenshots-as-Text)

Running the analyzer on `samples/demo.c`:

```text
$ ./analyzer samples/demo.c
samples/demo.c:6:1: warning [W004]: control reaches end of non-void function 'process_packet_header'
    6 | int process_packet_header(char *dest, char *src, int flag) {
      | ^
      = suggestion: add a return statement along all code paths
samples/demo.c:7:5: warning [W011]: call to unsafe function 'strcpy' without bounds checking
    7 |     strcpy(dest, src);
      |     ^
      = suggestion: use 'strncpy' or safer bounded alternative
samples/demo.c:17:1: warning [W008]: function 'run_telemetry_pipeline' is 42 lines long (exceeds limit of 40)
   17 | int run_telemetry_pipeline(int g_channel, int raw_input) {
      | ^
      = suggestion: break large function into smaller modular functions
samples/demo.c:17:32: warning [W012]: declaration of 'g_channel' shadows variable in enclosing scope (line 3)
   17 | int run_telemetry_pipeline(int g_channel, int raw_input) {
      |                                ^
      = suggestion: rename variable to avoid masking outer scope variable
samples/demo.c:18:5: warning [W001]: unused variable 'unused_metric'
   18 |     int unused_metric;
      |     ^
      = suggestion: remove or use this variable
samples/demo.c:19:5: info [W001]: variable 'mode' is assigned but never read
   19 |     int mode = 0;
      |     ^
      = suggestion: consider removing if unnecessary
samples/demo.c:21:19: warning [W005]: variable 'uninit_bias' may be used uninitialized
   21 |     int reading = uninit_bias + 1;
      |                   ^
      = suggestion: initialize variable before using it
samples/demo.c:23:19: info [W006]: magic number '42'
   23 |     magic_scale = 42;
      |                   ^
      = suggestion: define a named constant or enum instead of using magic numbers
samples/demo.c:25:14: warning [W002]: assignment in condition
   25 |     if (mode = raw_input) {
      |              ^
      = suggestion: did you mean '=='?
samples/demo.c:29:5: warning [W009]: empty body in 'if' statement
   29 |     if (reading > 0);
      |     ^
      = suggestion: remove empty statement or add body logic
samples/demo.c:34:17: warning [W007]: nesting depth of 4 exceeds threshold of 3
   34 |                 if (reading > 1) {
      |                 ^
      = suggestion: refactor nested logic into helper functions
samples/demo.c:41:33: error [W013]: division by zero: right operand of '/' is literal 0
   41 |     int div_zero_test = reading / 0;
      |                                 ^
      = suggestion: ensure denominator is non-zero before dividing
samples/demo.c:57:13: warning [W003]: unreachable code
   57 |     reading = 0;
      |             ^
      = suggestion: remove unreachable statements
samples/demo.c:63:5: warning [W010]: switch statement missing 'default' label
   63 |     switch (command) {
      |     ^
      = suggestion: add a 'default:' case to handle unanticipated values
samples/demo.c:64:9: info [W014]: case label falls through to next case without break
   64 |         case 1:
      |         ^
      = suggestion: add 'break;' or comment fall-through if intentional

1 error, 11 warnings, 3 info in samples/demo.c
Analysis completed: 0 instructions were executed.
```

---

## Rule Reference

| ID | Name | Severity | Description & Trigger Example |
|:---|:-----|:--------:|:------------------------------|
| **W001** | `unused-variable` | `WARNING` / `INFO` | Local variable declared but never read (`WARNING`), or assigned but never read (`INFO`).<br>```c int a; int b = 1; return 0; ``` |
| **W002** | `assignment-in-condition` | `WARNING` | Accidental assignment `=` in condition instead of equality check `==`.<br>```c if (x = 5) { ... } ``` |
| **W003** | `unreachable-code` | `WARNING` | Code statements that can never execute following `return`, `break`, or `continue`.<br>```c return x; x++; ``` |
| **W004** | `missing-return` | `WARNING` | Non-void function execution path reaches closing brace without a return statement.<br>```c int f(int x) { if (x > 0) return 1; } ``` |
| **W005** | `use-before-assignment` | `WARNING` | Local variable read before any prior assignment.<br>```c int x; int y = x + 1; ``` |
| **W006** | `magic-number` | `INFO` | Unnamed numeric literal (other than `0, 1, 2, -1`) in expression, condition, or index.<br>```c x = timeout * 60; ``` |
| **W007** | `deep-nesting` | `WARNING` | Statement nesting depth of control flow exceeding threshold (default > 3).<br>```c if (a) { while (b) { for (;;) { if (c) { ... } } } } ``` |
| **W008** | `long-function` | `WARNING` | Function body length exceeding line threshold (default > 40 lines).<br>```c int huge_function(void) { /* > 40 lines */ } ``` |
| **W009** | `empty-body` | `WARNING` | Control statement with an empty block `{}` or lone semicolon `;` (exempts `for(;;);`).<br>```c if (flag); while (wait) {} ``` |
| **W010** | `switch-without-default` | `WARNING` | `switch` statement missing a `default:` label.<br>```c switch(x) { case 1: break; } ``` |
| **W011** | `unsafe-function` | `ERROR` / `WARNING` | Insecure library call: `gets` (`ERROR`), or `strcpy`, `strcat`, `sprintf`, unbounded `scanf("%s")` (`WARNING`).<br>```c gets(buf); strcpy(d, s); scanf("%s", str); ``` |
| **W012** | `variable-shadowing` | `WARNING` | Local variable or parameter declared with identical name as an enclosing scope variable.<br>```c int x = 1; void f(void) { int x = 2; } ``` |
| **W013** | `division-by-zero` | `ERROR` | Division or modulo operation where denominator is integer literal `0`.<br>```c y = x / 0; z %= 0; ``` |
| **W014** | `fall-through-case` | `INFO` | Switch case with statements that falls through into subsequent case without `break`, `return`, or `continue`.<br>```c case 1: val = 10; case 2: ... ``` |

---

## Project Structure

```
├── Makefile                 # Portable C11 build rules and test integration
├── PROJECT_BRIEF.md         # Lab requirements, language subset, and design decisions
├── README.md                # Project documentation, build guide, and rule reference
├── docs/
│   ├── DESIGN.md            # Architecture, pipeline diagram, EBNF grammar, and internals
│   ├── VIVA_QA.md           # 25 viva questions and model answers
│   └── TEST_REPORT.md       # Comprehensive 44-test validation table
├── src/
│   ├── main.c               # Entrypoint, argument parsing, pipeline runner
│   ├── lexer.h / lexer.c    # Tokenizer, keywords, number/string lexing
│   ├── ast.h / ast.c        # Tagged union AST definitions and memory management
│   ├── parser.h / parser.c  # Recursive descent and precedence climbing parser
│   ├── symtab.h / symtab.c  # Lexical scope stack and symbol resolution pass
│   ├── checks.h / checks.c  # Static rule checkers (W001-W014) and config struct
│   ├── diag.h / diag.c      # Diagnostic list collection, sorting, and severity counts
│   ├── report_console.h/.c  # ANSI color and source gutter terminal renderer
│   └── report_html.h/.c     # HTML export interface placeholder
├── samples/
│   └── demo.c               # Embedded telemetry pipeline triggering all 14 rules
└── tests/
    ├── run_tests.sh         # POSIX test suite driver
    ├── run_tests.bat        # Windows batch test runner
    ├── cases/               # 44 test cases (rules W001-W014, edge, lex, parse, sym, robust)
    └── expected/            # Exact expected diagnostic outputs (*.txt and *.expected)
```

---

## How to Add a New Rule in 5 Steps

Adding a new quality or safety check takes 5 clean steps following the rule table pattern:

1. **Declare the Check Function in `src/checks.h`**:
   ```c
   /* Rule W015: Detects bitwise operation with boolean literals */
   void check_w015_bitwise_boolean(const AstNode *root, const SymbolTable *st, DiagList *diags);
   ```
2. **Implement the Check in `src/checks.c`**:
   Write the detector function with a one-line comment, keeping it $\le 60$ lines:
   ```c
   /* Rule W015: Flags bitwise operators applied to boolean conditions */
   void check_w015_bitwise_boolean(const AstNode *root, const SymbolTable *st, DiagList *diags) {
       (void)st;
       /* Recursively inspect AST nodes and emit diagnostics: */
       /* diag_add(diags, "W015", DIAG_WARNING, line, col, "bitwise op on boolean", "use '&&' or '||'"); */
   }
   ```
3. **Register in `RULES_TABLE` in `src/checks.c`**:
   ```c
   { "W015", "bitwise-boolean", DIAG_WARNING, "Bitwise operator on boolean", check_w015_bitwise_boolean, true },
   ```
4. **Create Test Input in `tests/cases/chk_w015.c`**:
   Provide positive cases that trigger `W015` and negative cases that should not.
5. **Add Expected Output in `tests/expected/chk_w015.txt`**:
   Add the expected `W015:line` pairs and run `make test` to verify.

---

## Running the Tests

Execute the complete 44-test regression suite:
```bash
make test
```
Or run the POSIX test runner directly:
```bash
sh tests/run_tests.sh
```

---

## Known Limitations

1. **Unsupported C Constructs**: `struct`, `union`, `enum`, `typedef`, and `goto` are out of scope for this lab compiler front-end. The parser intercepts them and emits informative diagnostic `I001: unsupported construct skipped` while recovering cleanly.
2. **Explicit Type Casts**: Explicit casts like `(int)x` are not part of the supported primary grammar and are skipped.
3. **Flow-Insensitive Heuristic for W005**: Rule W005 uses a flow-insensitive single-pass AST walk. If a local variable is conditionally initialized inside one branch of an `if` block, later reads may still trigger a potential uninitialized use warning.
4. **Preprocessor Evaluation**: Preprocessor macros (`#define`) and file inclusions (`#include`) are stripped by the scanner; macro expansion is not performed.

---

## Future Work

- **Data-Flow Analysis & SSA Form**: Inter-procedural data-flow graphs, reaching definitions, and Static Single Assignment (SSA) to eliminate false positives in uninitialized variable detection.
- **Full C99/C11 Type System**: Struct layout calculation, typedef resolution, and complete type checking for pointer and array arithmetic.
- **Machine-Readable JSON Output**: `--json` CLI export for integration into CI/CD pipelines (GitHub Actions, GitLab CI).
- **Interactive HTML Report**: Standalone interactive report dashboard with filterable rule categories and source file navigation.
- **Extended Language Front-Ends**: Modular scanner/parser architecture enabling front-ends for other languages such as Go, Python, or Rust subsets.
