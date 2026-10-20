#include "diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Initializes a diagnostic list with zero items */
void diag_list_init(DiagList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

/* Appends a new diagnostic item to the growable list */
void diag_add(DiagList *list, const char *rule_id, DiagSeverity severity,
              int line, int col, const char *message, const char *suggestion) {
    if (list->count >= list->capacity) {
        size_t new_cap = list->capacity == 0 ? 8 : list->capacity * 2;
        Diagnostic *new_items = realloc(list->items, new_cap * sizeof(Diagnostic));
        if (!new_items) return;
        list->items = new_items;
        list->capacity = new_cap;
    }

    Diagnostic *d = &list->items[list->count++];
    strncpy(d->rule_id, rule_id ? rule_id : "W000", sizeof(d->rule_id) - 1);
    d->rule_id[sizeof(d->rule_id) - 1] = '\0';
    d->severity = severity;
    d->line = line;
    d->column = col;

    if (message) {
        size_t len = strlen(message) + 1;
        d->message = malloc(len);
        if (d->message) memcpy(d->message, message, len);
    } else {
        d->message = NULL;
    }

    if (suggestion) {
        size_t len = strlen(suggestion) + 1;
        d->suggestion = malloc(len);
        if (d->suggestion) memcpy(d->suggestion, suggestion, len);
    } else {
        d->suggestion = NULL;
    }
}

/* Backwards compatible wrapper forwarding to diag_add */
void diag_list_add(DiagList *list, const char *rule_id, DiagSeverity severity,
                   int line, int column, const char *message, const char *suggestion) {
    diag_add(list, rule_id, severity, line, column, message, suggestion);
}

/* Compares two diagnostics by line number then column number */
static int diag_compare(const void *a, const void *b) {
    const Diagnostic *da = (const Diagnostic *)a;
    const Diagnostic *db = (const Diagnostic *)b;
    if (da->line != db->line) return da->line - db->line;
    return da->column - db->column;
}

/* Sorts diagnostics list in-place by line then column */
void diag_list_sort(DiagList *list) {
    if (!list || list->count <= 1) return;
    qsort(list->items, list->count, sizeof(Diagnostic), diag_compare);
}

/* Counts the number of diagnostics matching a specific severity level */
size_t diag_count_severity(const DiagList *list, DiagSeverity sev) {
    if (!list) return 0;
    size_t count = 0;
    for (size_t i = 0; i < list->count; i++) {
        if (list->items[i].severity == sev) count++;
    }
    return count;
}

/* Frees all dynamically allocated memory in the diagnostic list */
void diag_list_free(DiagList *list) {
    if (!list) return;
    for (size_t i = 0; i < list->count; i++) {
        free(list->items[i].message);
        free(list->items[i].suggestion);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

/* Returns true if the list contains at least one warning or error */
bool diag_has_warnings_or_errors(const DiagList *list) {
    if (!list) return false;
    for (size_t i = 0; i < list->count; i++) {
        if (list->items[i].severity == DIAG_WARNING || list->items[i].severity == DIAG_ERROR) {
            return true;
        }
    }
    return false;
}

/* Converts severity enum to display string */
const char *diag_severity_to_string(DiagSeverity severity) {
    switch (severity) {
        case DIAG_INFO: return "INFO";
        case DIAG_WARNING: return "WARNING";
        case DIAG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}
