#include "ast.h"
#include "checks.h"
#include "diag.h"
#include "lexer.h"
#include "parser.h"
#include "report_console.h"
#include "symtab.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Displays command line usage and argument options */
static void print_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s <file.c> [--html out.html] [--quiet] [--dump-tokens] [--dump-ast] [--dump-symbols]\n", prog_name);
}

/* Reads entire file content into an allocated null-terminated buffer */
static char *read_file_to_buffer(const char *path, size_t *out_size) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "Error: cannot open file '%s'\n", path);
        return NULL;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        fprintf(stderr, "Error: failed to seek in file '%s'\n", path);
        return NULL;
    }
    long size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        fprintf(stderr, "Error: failed to determine size of file '%s'\n", path);
        return NULL;
    }
    rewind(fp);

    char *buf = calloc(1, (size_t)size + 4);
    if (!buf) {
        fclose(fp);
        fprintf(stderr, "Error: out of memory reading file '%s'\n", path);
        return NULL;
    }
    size_t read_bytes = fread(buf, 1, (size_t)size, fp);
    buf[read_bytes] = '\0';
    fclose(fp);
    if (out_size) *out_size = read_bytes;
    return buf;
}

/* Parses command-line arguments, options, and diagnostic flags */
static bool parse_cli_args(int argc, char **argv, const char **filename,
                           const char **html_path, bool *quiet,
                           bool *dump_tokens, bool *dump_ast, bool *dump_symbols) {
    *filename = NULL;
    *html_path = NULL;
    *quiet = false;
    *dump_tokens = false;
    *dump_ast = false;
    *dump_symbols = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--quiet") == 0) {
            *quiet = true;
        } else if (strcmp(argv[i], "--dump-tokens") == 0) {
            *dump_tokens = true;
        } else if (strcmp(argv[i], "--dump-ast") == 0) {
            *dump_ast = true;
        } else if (strcmp(argv[i], "--dump-symbols") == 0) {
            *dump_symbols = true;
        } else if (strcmp(argv[i], "--html") == 0) {
            if (i + 1 >= argc) return false;
            *html_path = argv[++i];
        } else if (argv[i][0] == '-') {
            return false;
        } else {
            if (*filename != NULL) return false;
            *filename = argv[i];
        }
    }
    return *filename != NULL;
}

/* Dumps all scanned tokens from source buffer to stdout */
static void dump_tokens(const char *source, DiagList *diags) {
    Lexer lexer;
    lexer_init(&lexer, source);
    lexer_set_diags(&lexer, diags);

    while (true) {
        Token tok = lexer_next(&lexer);
        if (tok.type == TOK_EOF) break;
        printf("%d:%d %s %.*s\n", tok.line, tok.column,
               token_type_name(tok.type), (int)tok.length, tok.start);
    }
}

/* Executes the full compiler pipeline: lex, parse, resolve, and run rule checks */
static void run_pipeline(const char *source, DiagList *diags,
                         bool do_dump_ast, bool do_dump_symbols) {
    Lexer lexer;
    lexer_init(&lexer, source);
    lexer_set_diags(&lexer, diags);

    Parser parser;
    parser_init(&parser, &lexer, diags);

    AstNode *root = parser_parse(&parser);
    if (do_dump_ast && root) {
        ast_dump(root, 0);
    }

    if (root) {
        SymbolTable symtab;
        symtab_init(&symtab);
        resolver_resolve(root, &symtab, diags);
        if (do_dump_symbols) {
            symtab_dump(&symtab);
        }
        checks_run(root, &symtab, diags);
        symtab_free(&symtab);
    }

    ast_free(root);
    parser_free(&parser);
}

/* Main entrypoint: drives argument parsing, execution pipeline, and reporters */
int main(int argc, char **argv) {
    const char *filename = NULL;
    const char *html_path = NULL;
    bool quiet = false;
    bool do_dump_tokens = false;
    bool do_dump_ast = false;
    bool do_dump_symbols = false;

    if (!parse_cli_args(argc, argv, &filename, &html_path, &quiet,
                        &do_dump_tokens, &do_dump_ast, &do_dump_symbols)) {
        print_usage(argv[0]);
        return 2;
    }

    size_t file_size = 0;
    char *source = read_file_to_buffer(filename, &file_size);
    if (!source) return 2;

    DiagList diags;
    diag_list_init(&diags);

    if (do_dump_tokens) {
        dump_tokens(source, &diags);
    } else {
        run_pipeline(source, &diags, do_dump_ast, do_dump_symbols);
    }

    diag_list_sort(&diags);
    report_console_print(filename, source, &diags, quiet);
    int exit_code = diag_has_warnings_or_errors(&diags) ? 1 : 0;

    diag_list_free(&diags);
    free(source);
    return exit_code;
}
