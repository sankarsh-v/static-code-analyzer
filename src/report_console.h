#ifndef REPORT_CONSOLE_H
#define REPORT_CONSOLE_H

#include "diag.h"
#include <stdbool.h>

/* Prints compiler-grade diagnostic reports with source line context, carets, and colors */
void report_console_print(const char *filename, const char *source,
                          const DiagList *diags, bool quiet);

#endif /* REPORT_CONSOLE_H */
