#include "lexer.h"
#include <ctype.h>
#include <string.h>

/* Supported keywords in the language subset and recovery targets */
static const char *const KEYWORDS[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if",
    "int", "long", "register", "return", "short", "signed", "sizeof",
    "static", "struct", "switch", "typedef", "union", "unsigned", "void",
    "volatile", "while", NULL
};

/* Initializes a lexer instance with the source code buffer */
void lexer_init(Lexer *lexer, const char *src) {
    lexer->source = src ? src : "";
    lexer->cursor = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->diags = NULL;
    lexer->has_peek = false;
    memset(&lexer->peek_token, 0, sizeof(Token));
}

/* Sets the diagnostics list destination for lexer errors */
void lexer_set_diags(Lexer *lexer, DiagList *diags) {
    lexer->diags = diags;
}

/* Converts a token type enum to a readable string */
const char *token_type_name(TokenKind type) {
    switch (type) {
        case TOK_EOF: return "EOF";
        case TOK_IDENTIFIER: return "IDENTIFIER";
        case TOK_KEYWORD: return "KEYWORD";
        case TOK_INT_LITERAL: return "INT_LITERAL";
        case TOK_FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TOK_CHAR_LITERAL: return "CHAR_LITERAL";
        case TOK_STRING_LITERAL: return "STRING_LITERAL";
        case TOK_OPERATOR: return "OPERATOR";
        case TOK_PUNCTUATOR: return "PUNCTUATOR";
        case TOK_UNKNOWN: default: return "UNKNOWN";
    }
}

/* Returns the current character at cursor without consuming */
static char peek_char(const Lexer *l) {
    return l->source[l->cursor];
}

/* Returns a character at a specified offset ahead of the cursor */
static char peek_char_offset(const Lexer *l, size_t offset) {
    return l->source[l->cursor + offset];
}

/* Consumes the current character and updates line and column tracking */
static char advance_char(Lexer *l) {
    char c = l->source[l->cursor];
    if (c == '\0') return '\0';
    l->cursor++;
    if (c == '\n') {
        l->line++;
        l->column = 1;
    } else {
        l->column++;
    }
    return c;
}

/* Skips a single-line comment to the end of the line */
static void skip_single_line_comment(Lexer *l) {
    while (peek_char(l) != '\0' && peek_char(l) != '\n') {
        advance_char(l);
    }
}

/* Skips a multi-line block comment, reporting unterminated comments */
static void skip_block_comment(Lexer *l) {
    int start_line = l->line;
    int start_col = l->column - 2;
    while (peek_char(l) != '\0') {
        if (peek_char(l) == '*' && peek_char_offset(l, 1) == '/') {
            advance_char(l);
            advance_char(l);
            return;
        }
        advance_char(l);
    }
    if (l->diags) {
        diag_list_add(l->diags, "E003", DIAG_ERROR, start_line, start_col,
                      "unterminated block comment", "add '*/' to close comment");
    }
}

/* Skips a preprocessor line starting with '#' through newline, with continuation */
static void skip_preprocessor_line(Lexer *l) {
    while (peek_char(l) != '\0') {
        if (peek_char(l) == '\\') {
            if (peek_char_offset(l, 1) == '\n') {
                advance_char(l);
                advance_char(l);
                continue;
            } else if (peek_char_offset(l, 1) == '\r' && peek_char_offset(l, 2) == '\n') {
                advance_char(l);
                advance_char(l);
                advance_char(l);
                continue;
            }
        }
        if (peek_char(l) == '\n') {
            advance_char(l);
            break;
        }
        advance_char(l);
    }
}

/* Skips whitespace, comments, and preprocessor lines */
static void skip_whitespace_and_comments(Lexer *l) {
    bool at_line_start = (l->column == 1);
    while (peek_char(l) != '\0') {
        char c = peek_char(l);
        if (c == ' ' || c == '\t' || c == '\r') {
            advance_char(l);
        } else if (c == '\n') {
            advance_char(l);
            at_line_start = true;
        } else if (c == '/' && peek_char_offset(l, 1) == '/') {
            advance_char(l);
            advance_char(l);
            skip_single_line_comment(l);
        } else if (c == '/' && peek_char_offset(l, 1) == '*') {
            advance_char(l);
            advance_char(l);
            skip_block_comment(l);
            at_line_start = false;
        } else if (c == '#' && at_line_start) {
            skip_preprocessor_line(l);
            at_line_start = true;
        } else {
            break;
        }
    }
}

/* Checks if a given identifier text matches a C keyword */
static bool is_keyword(const char *start, size_t len) {
    for (int i = 0; KEYWORDS[i] != NULL; i++) {
        size_t kw_len = strlen(KEYWORDS[i]);
        if (kw_len == len && memcmp(start, KEYWORDS[i], len) == 0) {
            return true;
        }
    }
    return false;
}

/* Scans an identifier or keyword token */
static Token scan_identifier_or_keyword(Lexer *l) {
    Token tok;
    tok.line = l->line;
    tok.column = l->column;
    tok.start = &l->source[l->cursor];
    while (isalnum((unsigned char)peek_char(l)) || peek_char(l) == '_') {
        advance_char(l);
    }
    tok.length = (size_t)(&l->source[l->cursor] - tok.start);
    tok.type = is_keyword(tok.start, tok.length) ? TOK_KEYWORD : TOK_IDENTIFIER;
    return tok;
}

/* Scans integer and floating-point numeric literals */
static Token scan_number_literal(Lexer *l) {
    Token tok;
    tok.line = l->line;
    tok.column = l->column;
    tok.start = &l->source[l->cursor];
    bool is_float = false;

    if (peek_char(l) == '0' && (peek_char_offset(l, 1) == 'x' || peek_char_offset(l, 1) == 'X')) {
        advance_char(l);
        advance_char(l);
        while (isxdigit((unsigned char)peek_char(l))) advance_char(l);
    } else {
        while (isdigit((unsigned char)peek_char(l))) advance_char(l);
        if (peek_char(l) == '.' && isdigit((unsigned char)peek_char_offset(l, 1))) {
            is_float = true;
            advance_char(l);
            while (isdigit((unsigned char)peek_char(l))) advance_char(l);
        }
        if (peek_char(l) == 'e' || peek_char(l) == 'E') {
            is_float = true;
            advance_char(l);
            if (peek_char(l) == '+' || peek_char(l) == '-') advance_char(l);
            while (isdigit((unsigned char)peek_char(l))) advance_char(l);
        }
    }
    while (isalpha((unsigned char)peek_char(l))) advance_char(l);
    tok.length = (size_t)(&l->source[l->cursor] - tok.start);
    tok.type = is_float ? TOK_FLOAT_LITERAL : TOK_INT_LITERAL;
    return tok;
}

/* Scans a string literal, handling escape sequences and unterminated errors */
static Token scan_string_literal(Lexer *l) {
    Token tok;
    tok.line = l->line;
    tok.column = l->column;
    tok.start = &l->source[l->cursor];
    tok.type = TOK_STRING_LITERAL;
    advance_char(l); /* Consume opening quote */

    while (peek_char(l) != '\0' && peek_char(l) != '"' && peek_char(l) != '\n') {
        if (peek_char(l) == '\\') {
            advance_char(l);
            if (peek_char(l) != '\0') advance_char(l);
        } else {
            advance_char(l);
        }
    }
    if (peek_char(l) == '"') {
        advance_char(l);
    } else if (l->diags) {
        diag_list_add(l->diags, "E001", DIAG_ERROR, tok.line, tok.column,
                      "unterminated string literal", "add closing quote '\"'");
    }
    tok.length = (size_t)(&l->source[l->cursor] - tok.start);
    return tok;
}

/* Scans a character literal, handling escape sequences and unterminated errors */
static Token scan_char_literal(Lexer *l) {
    Token tok;
    tok.line = l->line;
    tok.column = l->column;
    tok.start = &l->source[l->cursor];
    tok.type = TOK_CHAR_LITERAL;
    advance_char(l); /* Consume opening single quote */

    while (peek_char(l) != '\0' && peek_char(l) != '\'' && peek_char(l) != '\n') {
        if (peek_char(l) == '\\') {
            advance_char(l);
            if (peek_char(l) != '\0') advance_char(l);
        } else {
            advance_char(l);
        }
    }
    if (peek_char(l) == '\'') {
        advance_char(l);
    } else if (l->diags) {
        diag_list_add(l->diags, "E002", DIAG_ERROR, tok.line, tok.column,
                      "unterminated character literal", "add closing quote '\''");
    }
    tok.length = (size_t)(&l->source[l->cursor] - tok.start);
    return tok;
}

/* Checks and consumes a 3-character operator */
static bool match_3char_op(Lexer *l, Token *tok) {
    char c1 = peek_char(l);
    char c2 = peek_char_offset(l, 1);
    char c3 = peek_char_offset(l, 2);
    if ((c1 == '<' && c2 == '<' && c3 == '=') ||
        (c1 == '>' && c2 == '>' && c3 == '=')) {
        tok->start = &l->source[l->cursor];
        advance_char(l); advance_char(l); advance_char(l);
        tok->length = 3;
        tok->type = TOK_OPERATOR;
        return true;
    }
    return false;
}

/* Checks and consumes a 2-character operator */
static bool match_2char_op(Lexer *l, Token *tok) {
    char c1 = peek_char(l);
    char c2 = peek_char_offset(l, 1);
    static const char *const OPS2[] = {
        "++", "--", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
        "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "->", NULL
    };
    for (int i = 0; OPS2[i] != NULL; i++) {
        if (c1 == OPS2[i][0] && c2 == OPS2[i][1]) {
            tok->start = &l->source[l->cursor];
            advance_char(l); advance_char(l);
            tok->length = 2;
            tok->type = TOK_OPERATOR;
            return true;
        }
    }
    return false;
}

/* Scans operators and punctuator tokens */
static Token scan_operator_or_punctuator(Lexer *l) {
    Token tok;
    tok.line = l->line;
    tok.column = l->column;

    if (match_3char_op(l, &tok)) return tok;
    if (match_2char_op(l, &tok)) return tok;

    tok.start = &l->source[l->cursor];
    char c = advance_char(l);
    tok.length = 1;

    if (strchr("(){}[];,", c)) {
        tok.type = TOK_PUNCTUATOR;
    } else if (strchr("+-*/%&|^~!<>=?:.", c)) {
        tok.type = TOK_OPERATOR;
    } else {
        tok.type = TOK_UNKNOWN;
        if (l->diags) {
            diag_list_add(l->diags, "E004", DIAG_ERROR, tok.line, tok.column,
                          "unknown character", "remove character");
        }
    }
    return tok;
}

/* Scans the next token from the source stream */
static Token lexer_scan_token(Lexer *l) {
    skip_whitespace_and_comments(l);

    if (peek_char(l) == '\0') {
        Token tok;
        tok.type = TOK_EOF;
        tok.line = l->line;
        tok.column = l->column;
        tok.start = &l->source[l->cursor];
        tok.length = 0;
        return tok;
    }

    char c = peek_char(l);
    if (isalpha((unsigned char)c) || c == '_') {
        return scan_identifier_or_keyword(l);
    }
    if (isdigit((unsigned char)c) || (c == '.' && isdigit((unsigned char)peek_char_offset(l, 1)))) {
        return scan_number_literal(l);
    }
    if (c == '"') {
        return scan_string_literal(l);
    }
    if (c == '\'') {
        return scan_char_literal(l);
    }
    return scan_operator_or_punctuator(l);
}

/* Peeks at the next token without advancing the stream cursor */
Token lexer_peek(Lexer *lexer) {
    if (!lexer->has_peek) {
        lexer->peek_token = lexer_scan_token(lexer);
        lexer->has_peek = true;
    }
    return lexer->peek_token;
}

/* Consumes and returns the next token from the input stream */
Token lexer_next(Lexer *lexer) {
    if (lexer->has_peek) {
        lexer->has_peek = false;
        return lexer->peek_token;
    }
    return lexer_scan_token(lexer);
}
