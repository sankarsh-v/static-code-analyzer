# Static Code Quality Analyzer — Test Report

This document records the complete test suite specification, test methodology, and regression test results for the **Static Code Quality Analyzer**.

---

## 1. Test Harness & Execution Methodology

Testing is automated through [tests/run_tests.sh](file:///Users/dhanush/cd%20project/tests/run_tests.sh) and integrated directly into the build system via `make test`:

1. **Test Ingestion**: Discovers all source files matching `tests/cases/*.c`.
2. **Execution**: Invokes `./analyzer "$case_file"` capturing standard output and standard error.
3. **Diagnostic Extraction**: Extracts normalized `RULEID:line` diagnostic indicators using POSIX `sed`:
   ```sh
   sed -n 's/.*:\([0-9]\{1,\}\):[0-9]\{1,\}:.*\[\([A-Za-z0-9_]\{1,\}\)\].*/\2:\1/p'
   ```
4. **Validation**: Uses `cmp -s` to compare the extracted diagnostic sequence against the baseline in `tests/expected/<name>.txt`.
5. **Exit Verification**: Guarantees zero regression on every run, returning exit code `0` on 100% pass and non-zero on failure.

---

## 2. Test Execution Summary

- **Total Test Cases**: 44
- **Passed**: 44
- **Failed**: 0
- **Pass Rate**: 100%
- **Execution Time**: ~0.04 seconds

---

## 3. Comprehensive Test Results Table

### Category 1: Static Quality & Safety Check Rules (W001 – W014)

| Test File | Target Rule | What It Checks | Expected Diagnostics | Status |
|:----------|:------------|:---------------|:---------------------|:------:|
| [chk_w001.c](file:///Users/dhanush/cd%20project/tests/cases/chk_w001.c) | `W001` Unused Variable | Local variable declared but never read (`WARNING`); variable assigned but never read (`INFO`). Exempts global variables and function parameters. | `W001:4`<br>`W001:5` | **PASS** |
| [chk_w002.c](file:///Users/dhanush/cd%20project/tests/cases/chk_w002.c) | `W002` Assignment in Condition | Unparenthesized assignment `=` inside `if`, `while`, and `for` conditions. Exempts equality `==`, parenthesized `((x = y))`, and comparisons. | `W002:2`<br>`W002:5`<br>`W002:8` | **PASS** |
| [chk_w003.c](file:///Users/dhanush/cd%20project/tests/cases/chk_w003.c) | `W003` Unreachable Code | Statements directly following `return`, or statements after an `if-else` where both branches terminate. Exempts reachable branches. | `W003:3`<br>`W003:12` | **PASS** |
| [chk_w004.c](file:///Users/dhanush/cd%20project/tests/cases/chk_w004.c) | `W004` Missing Return | Non-void functions missing return paths. Exempts `void` functions, `main()`, and intentional infinite loops (`while(1)` without break). | `W004:1`<br>`W004:7` | **PASS** |
| [chk_w005.c](file:///Users/dhanush/cd%20project/tests/cases/chk_w005.c) | `W005` Use Before Assignment | Local scalar variables read before any assignment statement. Exempts variables initialized at declaration. | `W005:3` | **PASS** |
| [chk_w006.c](file:///Users/dhanush/cd%20project/tests/cases/chk_w006.c) | `W006` Magic Number | Unnamed numeric literals other than `0, 1, 2, -1` in expressions and array indices. Exempts initializers and array size declarations. | `W006:10`<br>`W006:11`<br>`W006:12`<br>`W006:14`<br>`W006:17` | **PASS** |
| [chk_w007.c](file:///Users/dhanush/cd%20project/tests/cases/chk_w007.c) | `W007` Deep Nesting | Control-flow nesting depth exceeding configurable threshold (`> 3`). Reports once per function at the deepest point. | `W007:6` | **PASS** |
| [chk_w008.c](file:///Users/dhanush/cd%20project/tests/cases/chk_w008.c) | `W008` Long Function | Function body span exceeding configurable threshold (`> 40` lines) using start line and closing brace line. | `W008:1` | **PASS** |
| [chk_w009.c](file:///Users/dhanush/cd%20project/tests/cases/chk_w009.c) | `W009` Empty Body | Empty statement blocks `{}` or lone semicolons `;` in `if`, `else`, `while`, `for`. Exempts idiomatic `for(;;);`. | `W009:2`<br>`W009:3`<br>`W009:7`<br>`W009:11`<br>`W009:13`<br>`W009:14`<br>`W009:16`<br>`W009:17` | **PASS** |
| [chk_w010.c](file:///Users/dhanush/cd%20project/tests/cases/chk_w010.c) | `W010` Switch Without Default | `switch` statement lacking a `default:` branch. Clean when `default:` is present. | `W010:3` | **PASS** |
| [chk_w011.c](file:///Users/dhanush/cd%20project/tests/cases/chk_w011.c) | `W011` Unsafe Function | Insecure libc calls: `gets` (`ERROR`), `strcpy`, `strcat`, `sprintf`, and unbounded `scanf` `%s` (`WARNING`). Clean for `fgets`, `snprintf`, and `%31s`. | `W011:2`<br>`W011:3`<br>`W011:4`<br>`W011:5`<br>`W011:6` | **PASS** |
| [chk_w012.c](file:///Users/dhanush/cd%20project/tests/cases/chk_w012.c) | `W012` Variable Shadowing | Local variable or parameter declared with identical name as an enclosing scope variable. Clean when identifiers are distinct. | `W012:3`<br>`W012:6` | **PASS** |
| [chk_w013.c](file:///Users/dhanush/cd%20project/tests/cases/chk_w013.c) | `W013` Division by Zero | Division or modulo by literal `0` in binary expressions (`/`, `%`) and compound assignments (`/=`, `%=`). Clean for variable divisors and non-zero constants. | `W013:2`<br>`W013:3`<br>`W013:4`<br>`W013:5` | **PASS** |
| [chk_w014.c](file:///Users/dhanush/cd%20project/tests/cases/chk_w014.c) | `W014` Fall-Through Case | Non-last case containing statements that does not end in `break`, `return`, or `continue`. Exempts multi-case labels (`case 1: case 2:`) and the last case in a switch. | `W014:4` | **PASS** |

---

### Category 2: Robustness & Stress Cases

| Test File | Category | What It Checks | Expected Diagnostics | Status |
|:----------|:---------|:---------------|:---------------------|:------:|
| [robust_empty.c](file:///Users/dhanush/cd%20project/tests/cases/robust_empty.c) | Boundary | Zero-byte empty file handling. Scanner immediately produces `TOK_EOF`. | *None* | **PASS** |
| [robust_comments_only.c](file:///Users/dhanush/cd%20project/tests/cases/robust_comments_only.c) | Boundary | File containing only single-line and multi-line comments. Verifies comment skipper consumes file without emitting tokens. | *None* | **PASS** |
| [robust_binary_garbage.c](file:///Users/dhanush/cd%20project/tests/cases/robust_binary_garbage.c) | Error Recovery | Binary bytes, null characters, and non-ASCII control characters. Verifies `E004` scanner reporting and panic-mode syntax recovery without crashes. | `E004:1`<br>`E100:1`<br>`E100:1`<br>`E004:1`<br>`E004:1`<br>`E004:1` | **PASS** |
| [robust_5000_lines.c](file:///Users/dhanush/cd%20project/tests/cases/robust_5000_lines.c) | Stress | 5,000 generated functions. Verifies parser performance and validates that iterative `ast_free` does not exhaust stack frames. | *None* | **PASS** |
| [robust_deep_parens.c](file:///Users/dhanush/cd%20project/tests/cases/robust_deep_parens.c) | Stress | 200 nested parentheses `((((...1...))))`. Verifies recursive-descent expression parsing and child node deallocation. | *None* | **PASS** |
| [robust_unmatched_braces.c](file:///Users/dhanush/cd%20project/tests/cases/robust_unmatched_braces.c) | Error Recovery | Multiple unclosed opening braces and excess closing braces. Tests parser block synchronization and AST recovery. | `W004:1`<br>`W001:4`<br>`E100:14`<br>`E100:14` | **PASS** |

---

### Category 3: Lexer & Scanner Test Cases

| Test File | Category | What It Checks | Expected Diagnostics | Status |
|:----------|:---------|:---------------|:---------------------|:------:|
| [lex_comments.c](file:///Users/dhanush/cd%20project/tests/cases/lex_comments.c) | Lexer | Inline comments, block comments, and comments containing code-like strings. | *None* | **PASS** |
| [lex_literals.c](file:///Users/dhanush/cd%20project/tests/cases/lex_literals.c) | Lexer | Integer suffixes (`u`, `L`, `ULL`), hexadecimal (`0x`), octal (`0`), scientific float notation (`1e-4`, `2.5E+3`), character escapes, and string literals. | `W006:1`<br>`E100:1` | **PASS** |
| [lex_operators.c](file:///Users/dhanush/cd%20project/tests/cases/lex_operators.c) | Lexer | Multi-character operators: `++`, `--`, `+=`, `-=`, `<<=`, `>>=`, `==`, `!=`, `&&`, `||`, `->`. | `E100:1`<br>`E100:1`<br>`E100:1`<br>`E100:3`<br>`E100:3` | **PASS** |
| [lex_preprocessor.c](file:///Users/dhanush/cd%20project/tests/cases/lex_preprocessor.c) | Lexer | Preprocessor directives `#include`, `#define`, and multi-line macros using backslash line continuation. | `I002:6` | **PASS** |
| [lex_unknown_char.c](file:///Users/dhanush/cd%20project/tests/cases/lex_unknown_char.c) | Lexer | Stray characters like `@` or `$`. Verifies `E004: unknown character` diagnostic and scanner advancement. | `E004:2`<br>`E100:2`<br>`E004:3`<br>`E100:3` | **PASS** |
| [lex_unterminated_string.c](file:///Users/dhanush/cd%20project/tests/cases/lex_unterminated_string.c) | Lexer | Unterminated string literals spanning past newline. Verifies `E001: unterminated string literal`. | `E001:2`<br>`E100:3` | **PASS** |

---

### Category 4: Parser Syntax & Recovery Test Cases

| Test File | Category | What It Checks | Expected Diagnostics | Status |
|:----------|:---------|:---------------|:---------------------|:------:|
| [parse_control_flow.c](file:///Users/dhanush/cd%20project/tests/cases/parse_control_flow.c) | Parser | Full syntactic verification of `if`/`else`, `while`, `for`, and `do-while` control structures. | `W006:5`<br>`W006:8`<br>`W006:9`<br>`W006:13` | **PASS** |
| [parse_err_missing_brace.c](file:///Users/dhanush/cd%20project/tests/cases/parse_err_missing_brace.c) | Parser | Panic-mode recovery when a function body closing brace `}` is omitted. | `W004:1`<br>`W003:4`<br>`E100:6` | **PASS** |
| [parse_err_missing_semi.c](file:///Users/dhanush/cd%20project/tests/cases/parse_err_missing_semi.c) | Parser | Panic-mode recovery when a statement semicolon `;` is omitted. | `E100:3` | **PASS** |
| [parse_err_stray_token.c](file:///Users/dhanush/cd%20project/tests/cases/parse_err_stray_token.c) | Parser | Error emission on stray unexpected tokens inside expression statements. | `E004:3`<br>`E100:3` | **PASS** |
| [parse_multi_decl.c](file:///Users/dhanush/cd%20project/tests/cases/parse_multi_decl.c) | Parser | Comma-separated variable declarations: `int a, *b, c[10] = 5;`. | `W001:4`<br>`W001:4`<br>`W006:5` | **PASS** |
| [parse_precedence.c](file:///Users/dhanush/cd%20project/tests/cases/parse_precedence.c) | Parser | Precedence climbing with mixed arithmetic (`*` before `+`), bitwise (`&` before `|`), relational, and assignment operators. | *None* | **PASS** |
| [parse_simple.c](file:///Users/dhanush/cd%20project/tests/cases/parse_simple.c) | Parser | Valid minimal C function definitions and variable declarations. | *None* | **PASS** |
| [parse_switch_ternary.c](file:///Users/dhanush/cd%20project/tests/cases/parse_switch_ternary.c) | Parser | Switch blocks with `case`/`default` labels and nested ternary conditional expressions `? :`. | `W006:5`<br>`W006:8` | **PASS** |
| [parse_unsupported.c](file:///Users/dhanush/cd%20project/tests/cases/parse_unsupported.c) | Parser | Graceful interception of unsupported constructs: `struct`, `union`, `enum`, `typedef`, `goto`. Verifies `I001: unsupported construct skipped`. | `I001:1`<br>`I001:6`<br>`W006:9` | **PASS** |

---

### Category 5: Symbol Table & Lexical Scope Test Cases

| Test File | Category | What It Checks | Expected Diagnostics | Status |
|:----------|:---------|:---------------|:---------------------|:------:|
| [sym_globals.c](file:///Users/dhanush/cd%20project/tests/cases/sym_globals.c) | Symbol Table | Global variable registration, scope level 0 resolution, and access from within functions. | *None* | **PASS** |
| [sym_nested_scopes.c](file:///Users/dhanush/cd%20project/tests/cases/sym_nested_scopes.c) | Symbol Table | Deeply nested block scopes (`level 2`, `level 3`), variable visibility, and scope popping on block exit. | *None* | **PASS** |
| [sym_parameters.c](file:///Users/dhanush/cd%20project/tests/cases/sym_parameters.c) | Symbol Table | Parameter binding to function body scope (sharing scope level 1). | `W006:2` | **PASS** |
| [sym_redecl.c](file:///Users/dhanush/cd%20project/tests/cases/sym_redecl.c) | Symbol Table | Duplicate variable declaration in the exact same scope. Emits `E200: redeclaration of symbol`. | `E200:3` | **PASS** |
| [sym_shadowing.c](file:///Users/dhanush/cd%20project/tests/cases/sym_shadowing.c) | Symbol Table | Shadowing resolution: parameter shadowing global variable and inner local shadowing outer local. | `W012:4`<br>`W012:7` | **PASS** |

---

### Category 6: Boundary & Edge Case Test Cases

| Test File | Category | What It Checks | Expected Diagnostics | Status |
|:----------|:---------|:---------------|:---------------------|:------:|
| [edge_crlf.c](file:///Users/dhanush/cd%20project/tests/cases/edge_crlf.c) | Edge Case | Windows CRLF `\r\n` line endings. Verifies scanner line count tracking handles both `\n` and `\r\n`. | *None* | **PASS** |
| [edge_empty.c](file:///Users/dhanush/cd%20project/tests/cases/edge_empty.c) | Edge Case | Zero-length file. Verifies scanner and parser exit cleanly without memory leaks. | *None* | **PASS** |
| [edge_long_line.c](file:///Users/dhanush/cd%20project/tests/cases/edge_long_line.c) | Edge Case | Source line spanning thousands of characters. Verifies terminal snippet formatter does not overflow line buffers. | *None* | **PASS** |
| [edge_no_newline.c](file:///Users/dhanush/cd%20project/tests/cases/edge_no_newline.c) | Edge Case | File lacking a trailing newline before EOF. Verifies scanner handles EOF mid-token correctly. | *None* | **PASS** |

---

## 4. Full Pipeline Verification on `samples/demo.c`

The analyzer was executed on the full embedded telemetry demonstration program ([samples/demo.c](file:///Users/dhanush/cd%20project/samples/demo.c)), validating that all 14 built-in rules trigger accurately in a single file:

```text
1 error, 11 warnings, 3 info in samples/demo.c
Analysis completed: 0 instructions were executed.
```

- **W001** (line 18, 19): Unused variable `unused_metric` and assigned-only variable `mode`.
- **W002** (line 25): Assignment operator used as condition in `if (mode = raw_input)`.
- **W003** (line 57): Unreachable assignment `reading = 0;` after `return reading;`.
- **W004** (line 6): Control reaches end of non-void function `process_packet_header`.
- **W005** (line 21): Uninitialized variable `uninit_bias` read before assignment.
- **W006** (line 23): Magic number literal `42` outside variable initializers.
- **W007** (line 34): Nesting depth 4 in control structure exceeding threshold of 3.
- **W008** (line 17): Function `run_telemetry_pipeline` spanning 42 lines (exceeds limit 40).
- **W009** (line 29): Empty statement body in `if (reading > 0);`.
- **W010** (line 63): `switch (command)` statement missing a `default:` case label.
- **W011** (line 7): Call to unsafe library function `strcpy` without bounds checking.
- **W012** (line 17): Parameter `g_channel` shadowing global variable `g_channel`.
- **W013** (line 41): Division by literal zero `reading / 0`.
- **W014** (line 64): Case 1 with statements falling through to Case 2 without `break`.
