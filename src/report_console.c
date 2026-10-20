#include "report_console.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <io.h>
#define is_terminal(f) _isatty(_fileno(f))
#else
#include <unistd.h>
#define is_terminal(f) isatty(fileno(f))
#endif

/* Checks if ANSI color codes should be enabled for stdout */
static bool should_use_color(void) {
    if (getenv("NO_COLOR") != NULL) return false;
    const char *term = getenv("TERM");
    if (term && strcmp(term, "dumb") == 0) return false;
    return is_terminal(stdout);
}

/* Returns the ANSI color sequence string corresponding to diagnostic severity */
static const char *severity_color(DiagSeverity sev, bool use_color) {
    if (!use_color) return "";
    switch (sev) {
        case DIAG_ERROR: return "\033[1;31m";
        case DIAG_WARNING: return "\033[1;33m";
        case DIAG_INFO: return "\033[1;36m";
        default: return "\033[1m";
    }
}

/* Returns the lowercase severity name string */
static const char *severity_name(DiagSeverity sev) {
    switch (sev) {
        case DIAG_ERROR: return "error";
        case DIAG_WARNING: return "warning";
        case DIAG_INFO: return "info";
        default: return "note";
    }
}

/* Finds the start pointer and byte length of a 1-based source line */
static const char *get_source_line(const char *source, int line_num, size_t *out_len) {
    if (!source || line_num <= 0) return NULL;
    const char *curr = source;
    int curr_line = 1;
    while (*curr && curr_line < line_num) {
        if (*curr == '\n') curr_line++;
        curr++;
    }
    if (!*curr && curr_line < line_num) return NULL;

    const char *line_start = curr;
    while (*curr && *curr != '\n' && *curr != '\r') curr++;
    *out_len = (size_t)(curr - line_start);
    return line_start;
}

/* Prints the source line text and caret indicator pointing at column */
static void print_source_snippet(const char *source, int line, int column,
                                 bool use_color, const char *color_code) {
    size_t line_len = 0;
    const char *line_text = get_source_line(source, line, &line_len);
    if (!line_text) return;

    const char *gutter_color = use_color ? "\033[34m" : "";
    const char *reset = use_color ? "\033[0m" : "";

    printf("%s%5d | %s%.*s\n", gutter_color, line, reset, (int)line_len, line_text);
    printf("%s      | %s", gutter_color, reset);

    int max_col = column > 1 ? column - 1 : 0;
    for (int i = 0; i < max_col && i < (int)line_len; i++) {
        if (line_text[i] == '\t') putchar('\t');
        else putchar(' ');
    }
    printf("%s^%s\n", color_code, reset);
}

/* Prints a single diagnostic message with source line, caret, and suggestion */
static void print_diagnostic(const char *filename, const char *source,
                             const Diagnostic *d, bool use_color) {
    const char *color_code = severity_color(d->severity, use_color);
    const char *bold = use_color ? "\033[1m" : "";
    const char *reset = use_color ? "\033[0m" : "";
    const char *sev_str = severity_name(d->severity);

    printf("%s%s:%d:%d: %s%s [%s]%s: %s%s\n",
           bold, filename, d->line, d->column,
           color_code, sev_str, d->rule_id, reset,
           bold, d->message ? d->message : "");

    if (source && d->line > 0 && d->column > 0) {
        print_source_snippet(source, d->line, d->column, use_color, color_code);
    }

    if (d->suggestion && d->suggestion[0]) {
        const char *info_color = use_color ? "\033[36m" : "";
        printf("      %s= suggestion:%s %s\n", info_color, reset, d->suggestion);
    }
}

/* Prints full compiler-style diagnostics report and execution summary */
void report_console_print(const char *filename, const char *source,
                          const DiagList *diags, bool quiet) {
    if (quiet || !diags) return;
    bool use_color = should_use_color();

    for (size_t i = 0; i < diags->count; i++) {
        print_diagnostic(filename, source, &diags->items[i], use_color);
    }

    size_t errors = diag_count_severity(diags, DIAG_ERROR);
    size_t warnings = diag_count_severity(diags, DIAG_WARNING);
    size_t info = diag_count_severity(diags, DIAG_INFO);

    printf("\n%zu error%s, %zu warning%s, %zu info in %s\n",
           errors, errors == 1 ? "" : "s",
           warnings, warnings == 1 ? "" : "s",
           info, filename);
    printf("Analysis completed: 0 instructions were executed.\n");
}
