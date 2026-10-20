#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "diag.h"
#include "lexer.h"

/* Recursive descent parser state with error recovery */
typedef struct {
    Lexer *lexer;
    Token current;
    DiagList *diags;
    bool panic_mode;
} Parser;

/* Initializes a parser with a lexer and diagnostic list */
void parser_init(Parser *parser, Lexer *lexer, DiagList *diags);

/* Parses the input token stream into an AST translation unit */
AstNode *parser_parse(Parser *parser);

/* Releases resources held by the parser instance */
void parser_free(Parser *parser);

#endif /* PARSER_H */
