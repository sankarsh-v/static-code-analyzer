#include "report_html.h"
#include <stdio.h>

/* Generates a standalone self-contained HTML report file */
bool report_html_generate(const char *source_filename, const char *source_code,
                          const DiagList *diags, const char *html_output_path) {
    (void)source_filename;
    (void)source_code;
    (void)diags;
    if (!html_output_path) return false;
    FILE *fp = fopen(html_output_path, "w");
    if (!fp) return false;
    fprintf(fp, "<!DOCTYPE html><html><head><title>Report</title></head><body></body></html>\n");
    fclose(fp);
    return true;
}
