#ifndef REPORT_HTML_H
#define REPORT_HTML_H

#include "diag.h"
#include <stdbool.h>

/* Generates a standalone self-contained HTML report file */
bool report_html_generate(const char *source_filename, const char *source_code,
                          const DiagList *diags, const char *html_output_path);

#endif /* REPORT_HTML_H */
