#ifndef DIAG_H
#define DIAG_H

#include <stdbool.h>
#include <stddef.h>

/* Severity level for static code diagnostics */
typedef enum {
    DIAG_INFO = 0,
    DIAG_WARNING,
    DIAG_ERROR
} DiagSeverity;

/* Single diagnostic issue record */
typedef struct {
    char rule_id[16];
    DiagSeverity severity;
    int line;
    int column;
    char *message;
    char *suggestion;
} Diagnostic;

/* Growable list of diagnostics */
typedef struct {
    Diagnostic *items;
    size_t count;
    size_t capacity;
} DiagList;

/* Initializes a diagnostic list structure */
void diag_list_init(DiagList *list);

/* Appends a new diagnostic item to the list */
void diag_add(DiagList *list, const char *rule_id, DiagSeverity severity,
              int line, int col, const char *message, const char *suggestion);

/* Backwards-compatible alias for diag_add */
void diag_list_add(DiagList *list, const char *rule_id, DiagSeverity severity,
                   int line, int column, const char *message, const char *suggestion);

/* Sorts diagnostics in-place by line then column */
void diag_list_sort(DiagList *list);

/* Counts occurrences of diagnostics matching a given severity */
size_t diag_count_severity(const DiagList *list, DiagSeverity sev);

/* Frees all dynamically allocated memory in the diagnostic list */
void diag_list_free(DiagList *list);

/* Returns true if the list contains at least one warning or error */
bool diag_has_warnings_or_errors(const DiagList *list);

/* Converts severity enum to display string */
const char *diag_severity_to_string(DiagSeverity severity);

#endif /* DIAG_H */
