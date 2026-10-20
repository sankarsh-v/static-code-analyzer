#ifndef LEXER_H
#define LEXER_H

#include "diag.h"
#include <stdbool.h>
#include <stddef.h>

/* Supported token types in the C subset */
typedef enum {
    TOK_EOF = 0,
    TOK_IDENTIFIER,
    TOK_KEYWORD,
    TOK_INT_LITERAL,
    TOK_FLOAT_LITERAL,
    TOK_CHAR_LITERAL,
    TOK_STRING_LITERAL,
    TOK_OPERATOR,
    TOK_PUNCTUATOR,
    TOK_UNKNOWN
} TokenKind;

/* Token representation with zero-allocation lexeme slice */
typedef struct {
    TokenKind type;
    const char *start;
    size_t length;
    int line;
    int column;
} Token;

/* Lexer scanner state */
typedef struct {
    const char *source;
    size_t cursor;
    int line;
    int column;
    DiagList *diags;
    bool has_peek;
    Token peek_token;
} Lexer;

/* Initializes a lexer instance with the source code buffer */
void lexer_init(Lexer *lexer, const char *src);

/* Sets the diagnostics list destination for lexer errors */
void lexer_set_diags(Lexer *lexer, DiagList *diags);

/* Consumes and returns the next token from the input stream */
Token lexer_next(Lexer *lexer);

/* Peeks at the next token without advancing the stream cursor */
Token lexer_peek(Lexer *lexer);

/* Converts a token type enum to a readable string */
const char *token_type_name(TokenKind type);

#endif /* LEXER_H */
