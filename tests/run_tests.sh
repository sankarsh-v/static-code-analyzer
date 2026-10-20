#!/bin/sh
# tests/run_tests.sh - POSIX test runner for Static Code Quality Analyzer
# Runs every test in tests/cases/*.c, extracts RULEID:line pairs,
# and verifies against tests/expected/<name>.txt.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ANALYZER="${ROOT_DIR}/analyzer"

if [ -f "${ANALYZER}.exe" ]; then
    ANALYZER="${ANALYZER}.exe"
fi

if [ ! -x "$ANALYZER" ]; then
    echo "Error: analyzer executable not found or not executable at '$ANALYZER'" >&2
    echo "Please build the project first using 'make'." >&2
    exit 2
fi

TMP_ACTUAL="$(mktemp 2>/dev/null || echo "/tmp/analyzer_test_act_$$")"
trap 'rm -f "$TMP_ACTUAL"' EXIT INT TERM

PASSED=0
FAILED=0
TOTAL=0

echo "Running static analyzer test suite..."
echo "============================================================"

for case_file in "$ROOT_DIR"/tests/cases/*.c; do
    [ -e "$case_file" ] || continue
    TOTAL=$((TOTAL + 1))
    case_name="$(basename "$case_file" .c)"
    expected_file="$ROOT_DIR/tests/expected/${case_name}.txt"

    if [ ! -f "$expected_file" ]; then
        printf "  [FAIL] %-32s (missing %s)\n" "$case_name" "tests/expected/${case_name}.txt"
        FAILED=$((FAILED + 1))
        continue
    fi

    # Execute analyzer and extract RULEID:line pairs
    set +e
    "$ANALYZER" "$case_file" 2>&1 | sed -n 's/.*:\([0-9]\{1,\}\):[0-9]\{1,\}:.*\[\([A-Za-z0-9_]\{1,\}\)\].*/\2:\1/p' > "$TMP_ACTUAL"
    ANALYZER_EXIT=$?
    set -e

    if cmp -s "$TMP_ACTUAL" "$expected_file"; then
        printf "  [PASS] %-32s\n" "$case_name"
        PASSED=$((PASSED + 1))
    else
        printf "  [FAIL] %-32s\n" "$case_name"
        echo "    --- Expected (${case_name}.txt) ---"
        sed 's/^/      /' "$expected_file"
        echo "    --- Actual output ---"
        sed 's/^/      /' "$TMP_ACTUAL"
        FAILED=$((FAILED + 1))
    fi
done

echo "============================================================"
printf "Results: %d/%d tests passed" "$PASSED" "$TOTAL"
if [ "$FAILED" -gt 0 ]; then
    printf " (%d FAILED)\n" "$FAILED"
    exit 1
else
    printf " (ALL PASSED)\n"
    exit 0
fi
