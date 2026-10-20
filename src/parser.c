#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declarations for mutually recursive parsing routines */
static AstNode *parse_expr(Parser *p);
static AstNode *parse_assignment(Parser *p);
static AstNode *parse_statement(Parser *p);
static AstNode *parse_block(Parser *p);
static AstNode *parse_decl_or_func(Parser *p);

/* Checks if current token text matches a string */
static bool tok_matches(Token tok, const char *str) {
    size_t len = strlen(str);
    return tok.length == len && memcmp(tok.start, str, len) == 0;
}

/* Copies token text into a newly heap allocated null-terminated string */
static char *tok_to_str(Token tok) {
    char *s = malloc(tok.length + 1);
    if (!s) return NULL;
    memcpy(s, tok.start, tok.length);
    s[tok.length] = '\0';
    return s;
}

/* Consumes current token and advances parser to the next token */
static void advance(Parser *p) {
    if (p->current.type != TOK_EOF) {
        p->current = lexer_next(p->lexer);
    }
}

/* Returns true if current token matches the given punctuation or operator symbol */
static bool check_punct(Parser *p, const char *punct) {
    return (p->current.type == TOK_PUNCTUATOR || p->current.type == TOK_OPERATOR) &&
           tok_matches(p->current, punct);
}

/* Returns true if current token is operator matching the given text */
static bool check_op(Parser *p, const char *op) {
    return p->current.type == TOK_OPERATOR && tok_matches(p->current, op);
}

/* Returns true if current token is keyword matching the given text */
static bool check_kw(Parser *p, const char *kw) {
    return p->current.type == TOK_KEYWORD && tok_matches(p->current, kw);
}

/* Matches and consumes a punctuator token */
static bool match_punct(Parser *p, const char *punct) {
    if (check_punct(p, punct)) {
        advance(p);
        return true;
    }
    return false;
}

/* Matches and consumes an operator token */
static bool match_op(Parser *p, const char *op) {
    if (check_op(p, op)) {
        advance(p);
        return true;
    }
    return false;
}

/* Matches and consumes a keyword token */
static bool match_kw(Parser *p, const char *kw) {
    if (check_kw(p, kw)) {
        advance(p);
        return true;
    }
    return false;
}

/* Recovers parser state by advancing to the next statement boundary */
static void synchronize(Parser *p) {
    p->panic_mode = false;
    while (p->current.type != TOK_EOF) {
        if (check_punct(p, ";")) {
            advance(p);
            return;
        }
        if (check_punct(p, "}")) {
            advance(p);
            return;
        }
        if (check_kw(p, "if") || check_kw(p, "while") || check_kw(p, "for") ||
            check_kw(p, "return") || check_kw(p, "int") || check_kw(p, "char") ||
            check_kw(p, "void") || check_kw(p, "switch") || check_kw(p, "case")) {
            return;
        }
        advance(p);
    }
}

/* Reports a syntax error diagnostic with expected vs found description */
static void emit_expected_error(Parser *p, const char *expected) {
    char msg[128];
    char found_buf[32] = {0};
    if (p->current.type == TOK_EOF) {
        strcpy(found_buf, "EOF");
    } else {
        size_t cplen = p->current.length < 31 ? p->current.length : 31;
        memcpy(found_buf, p->current.start, cplen);
    }
    snprintf(msg, sizeof(msg), "expected '%s', found '%s'", expected, found_buf);
    if (p->diags) {
        diag_list_add(p->diags, "E100", DIAG_ERROR, p->current.line, p->current.column,
                      msg, "check grammar syntax");
    }
}

/* Verifies and consumes an expected punctuator token or recovers */
static bool expect_punct(Parser *p, const char *punct, const char *desc) {
    if (check_punct(p, punct)) {
        advance(p);
        return true;
    }
    emit_expected_error(p, desc ? desc : punct);
    synchronize(p);
    return false;
}

/* Verifies and consumes an expected keyword token or recovers */
static bool expect_kw(Parser *p, const char *kw, const char *desc) {
    if (check_kw(p, kw)) {
        advance(p);
        return true;
    }
    emit_expected_error(p, desc ? desc : kw);
    synchronize(p);
    return false;
}

/* Checks if current token begins a valid C type specification */
static bool is_type_token(Token tok) {
    if (tok.type != TOK_KEYWORD) return false;
    return tok_matches(tok, "int") || tok_matches(tok, "char") ||
           tok_matches(tok, "float") || tok_matches(tok, "double") ||
           tok_matches(tok, "long") || tok_matches(tok, "short") ||
           tok_matches(tok, "unsigned") || tok_matches(tok, "signed") ||
           tok_matches(tok, "void");
}

/* Parses base type name including qualifiers into an allocated string */
static char *parse_type_specifier(Parser *p) {
    char buf[64] = {0};
    while (is_type_token(p->current)) {
        if (buf[0] != '\0') strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
        size_t rem = sizeof(buf) - strlen(buf) - 1;
        size_t cplen = p->current.length < rem ? p->current.length : rem;
        strncat(buf, p->current.start, cplen);
        advance(p);
    }
    return strdup(buf[0] ? buf : "int");
}

/* Parses sequence of pointer asterisks and returns pointer indirection depth */
static int parse_pointer_depth(Parser *p) {
    int depth = 0;
    while (check_op(p, "*")) {
        depth++;
        advance(p);
    }
    return depth;
}

/* Checks if an operator token is a valid compound or simple assignment operator */
static bool is_assignment_op(Token tok) {
    if (tok.type != TOK_OPERATOR) return false;
    return tok_matches(tok, "=") || tok_matches(tok, "+=") || tok_matches(tok, "-=") ||
           tok_matches(tok, "*=") || tok_matches(tok, "/=") || tok_matches(tok, "%=") ||
           tok_matches(tok, "&=") || tok_matches(tok, "|=") || tok_matches(tok, "^=") ||
           tok_matches(tok, "<<=") || tok_matches(tok, ">>=");
}

/* Returns operator precedence for binary expression climbing */
static int get_binary_prec(Token tok) {
    if (tok.type != TOK_OPERATOR) return -1;
    if (tok_matches(tok, "||")) return 1;
    if (tok_matches(tok, "&&")) return 2;
    if (tok_matches(tok, "|")) return 3;
    if (tok_matches(tok, "^")) return 4;
    if (tok_matches(tok, "&")) return 5;
    if (tok_matches(tok, "==") || tok_matches(tok, "!=")) return 6;
    if (tok_matches(tok, "<") || tok_matches(tok, ">") ||
        tok_matches(tok, "<=") || tok_matches(tok, ">=")) return 7;
    if (tok_matches(tok, "<<") || tok_matches(tok, ">>")) return 8;
    if (tok_matches(tok, "+") || tok_matches(tok, "-")) return 9;
    if (tok_matches(tok, "*") || tok_matches(tok, "/") || tok_matches(tok, "%")) return 10;
    return -1;
}

/* Parses primary expressions: identifiers, literals, and parenthesized expressions */
static AstNode *parse_primary(Parser *p) {
    Token tok = p->current;
    if (tok.type == TOK_IDENTIFIER) {
        advance(p);
        AstNode *node = ast_create(NODE_IDENTIFIER, tok.line, tok.column);
        node->data.identifier.name = tok_to_str(tok);
        return node;
    }
    if (tok.type == TOK_INT_LITERAL || tok.type == TOK_FLOAT_LITERAL ||
        tok.type == TOK_CHAR_LITERAL || tok.type == TOK_STRING_LITERAL) {
        advance(p);
        NodeKind k = (tok.type == TOK_INT_LITERAL) ? NODE_INT_LITERAL :
                     (tok.type == TOK_FLOAT_LITERAL) ? NODE_FLOAT_LITERAL :
                     (tok.type == TOK_CHAR_LITERAL) ? NODE_CHAR_LITERAL : NODE_STRING_LITERAL;
        AstNode *node = ast_create(k, tok.line, tok.column);
        node->data.literal.value = tok_to_str(tok);
        return node;
    }
    if (match_punct(p, "(")) {
        AstNode *expr = parse_expr(p);
        if (expr) expr->is_parenthesized = true;
        expect_punct(p, ")", ")");
        return expr;
    }
    emit_expected_error(p, "expression");
    return NULL;
}

/* Parses function call argument lists */
static AstNode *parse_call_args(Parser *p) {
    AstNode *args = NULL;
    AstNode **tail = &args;
    if (!check_punct(p, ")")) {
        while (true) {
            AstNode *arg = parse_assignment(p);
            if (arg) {
                *tail = arg;
                tail = &arg->next;
            }
            if (!match_punct(p, ",")) break;
        }
    }
    expect_punct(p, ")", ")");
    return args;
}

/* Parses postfix expressions: function calls, array indexing, and ++/-- */
static AstNode *parse_postfix(Parser *p) {
    AstNode *expr = parse_primary(p);
    if (!expr) return NULL;

    while (true) {
        if (check_op(p, "++") || check_op(p, "--")) {
            Token op_tok = p->current;
            advance(p);
            AstNode *node = ast_create(NODE_UNARY, op_tok.line, op_tok.column);
            memcpy(node->data.unary.op, op_tok.start, op_tok.length < 7 ? op_tok.length : 7);
            node->data.unary.operand = expr;
            node->data.unary.is_postfix = true;
            expr = node;
        } else if (match_punct(p, "(")) {
            AstNode *call = ast_create(NODE_CALL, expr->line, expr->column);
            call->data.call.callee = expr;
            call->data.call.args = parse_call_args(p);
            expr = call;
        } else if (match_punct(p, "[")) {
            AstNode *idx_node = ast_create(NODE_INDEX, expr->line, expr->column);
            idx_node->data.index.array = expr;
            idx_node->data.index.index = parse_expr(p);
            expect_punct(p, "]", "]");
            expr = idx_node;
        } else {
            break;
        }
    }
    return expr;
}

/* Parses prefix unary expressions including address-of, dereference, and sizeof */
static AstNode *parse_unary(Parser *p) {
    if (check_op(p, "++") || check_op(p, "--") || check_op(p, "+") || check_op(p, "-") ||
        check_op(p, "!") || check_op(p, "~") || check_op(p, "*") || check_op(p, "&") ||
        check_kw(p, "sizeof")) {
        Token op_tok = p->current;
        advance(p);
        AstNode *operand = parse_unary(p);
        AstNode *node = ast_create(NODE_UNARY, op_tok.line, op_tok.column);
        size_t cplen = op_tok.length < 7 ? op_tok.length : 7;
        memcpy(node->data.unary.op, op_tok.start, cplen);
        node->data.unary.operand = operand;
        node->data.unary.is_postfix = false;
        return node;
    }
    return parse_postfix(p);
}

/* Parses binary expressions with left-associative precedence climbing */
static AstNode *parse_binary(Parser *p, int min_prec) {
    AstNode *left = parse_unary(p);
    if (!left) return NULL;

    while (true) {
        int prec = get_binary_prec(p->current);
        if (prec < min_prec) break;

        Token op_tok = p->current;
        advance(p);
        AstNode *right = parse_binary(p, prec + 1);
        if (!right) {
            ast_free(left);
            return NULL;
        }
        AstNode *node = ast_create(NODE_BINARY, op_tok.line, op_tok.column);
        size_t cplen = op_tok.length < 7 ? op_tok.length : 7;
        memcpy(node->data.binary.op, op_tok.start, cplen);
        node->data.binary.left = left;
        node->data.binary.right = right;
        left = node;
    }
    return left;
}

/* Parses ternary conditional expression (?:) with right associativity */
static AstNode *parse_ternary(Parser *p) {
    AstNode *cond = parse_binary(p, 1);
    if (!cond) return NULL;
    if (match_op(p, "?")) {
        AstNode *true_expr = parse_expr(p);
        expect_punct(p, ":", ":");
        AstNode *false_expr = parse_ternary(p);
        AstNode *node = ast_create(NODE_TERNARY, cond->line, cond->column);
        node->data.ternary.condition = cond;
        node->data.ternary.true_expr = true_expr;
        node->data.ternary.false_expr = false_expr;
        return node;
    }
    return cond;
}

/* Parses assignment expressions with right-to-left associativity */
static AstNode *parse_assignment(Parser *p) {
    AstNode *left = parse_ternary(p);
    if (!left) return NULL;
    if (is_assignment_op(p->current)) {
        Token op_tok = p->current;
        advance(p);
        AstNode *right = parse_assignment(p);
        AstNode *node = ast_create(NODE_ASSIGN, op_tok.line, op_tok.column);
        size_t cplen = op_tok.length < 7 ? op_tok.length : 7;
        memcpy(node->data.assign.op, op_tok.start, cplen);
        node->data.assign.left = left;
        node->data.assign.right = right;
        return node;
    }
    return left;
}

/* Parses an expression, delegating to assignment precedence */
static AstNode *parse_expr(Parser *p) {
    return parse_assignment(p);
}

/* Parses an if-else statement resolving dangling else to the nearest if */
static AstNode *parse_if(Parser *p) {
    int line = p->current.line;
    int col = p->current.column;
    advance(p); /* Consume 'if' */
    expect_punct(p, "(", "(");
    AstNode *cond = parse_expr(p);
    expect_punct(p, ")", ")");
    AstNode *then_b = parse_statement(p);
    AstNode *else_b = NULL;
    if (match_kw(p, "else")) {
        else_b = parse_statement(p);
    }
    AstNode *node = ast_create(NODE_IF, line, col);
    node->data.if_stmt.condition = cond;
    node->data.if_stmt.then_branch = then_b;
    node->data.if_stmt.else_branch = else_b;
    return node;
}

/* Parses a while loop statement */
static AstNode *parse_while(Parser *p) {
    int line = p->current.line;
    int col = p->current.column;
    advance(p); /* Consume 'while' */
    expect_punct(p, "(", "(");
    AstNode *cond = parse_expr(p);
    expect_punct(p, ")", ")");
    AstNode *body = parse_statement(p);
    AstNode *node = ast_create(NODE_WHILE, line, col);
    node->data.while_stmt.condition = cond;
    node->data.while_stmt.body = body;
    return node;
}

/* Parses a for loop statement supporting declaration in init clause */
static AstNode *parse_for(Parser *p) {
    int line = p->current.line;
    int col = p->current.column;
    advance(p); /* Consume 'for' */
    expect_punct(p, "(", "(");
    AstNode *init = NULL;
    if (is_type_token(p->current)) {
        init = parse_decl_or_func(p);
    } else if (!match_punct(p, ";")) {
        AstNode *e = parse_expr(p);
        expect_punct(p, ";", ";");
        init = ast_create(NODE_EXPR_STMT, e ? e->line : line, e ? e->column : col);
        init->data.expr_stmt.expr = e;
    }
    AstNode *cond = NULL;
    if (!check_punct(p, ";")) cond = parse_expr(p);
    expect_punct(p, ";", ";");
    AstNode *step = NULL;
    if (!check_punct(p, ")")) step = parse_expr(p);
    expect_punct(p, ")", ")");
    AstNode *body = parse_statement(p);
    AstNode *node = ast_create(NODE_FOR, line, col);
    node->data.for_stmt.init = init;
    node->data.for_stmt.condition = cond;
    node->data.for_stmt.step = step;
    node->data.for_stmt.body = body;
    return node;
}

/* Parses a do-while loop statement */
static AstNode *parse_do_while(Parser *p) {
    int line = p->current.line;
    int col = p->current.column;
    advance(p); /* Consume 'do' */
    AstNode *body = parse_statement(p);
    expect_kw(p, "while", "while");
    expect_punct(p, "(", "(");
    AstNode *cond = parse_expr(p);
    expect_punct(p, ")", ")");
    expect_punct(p, ";", ";");
    AstNode *node = ast_create(NODE_DO_WHILE, line, col);
    node->data.do_while.body = body;
    node->data.do_while.condition = cond;
    return node;
}

/* Parses a switch statement block */
static AstNode *parse_switch(Parser *p) {
    int line = p->current.line;
    int col = p->current.column;
    advance(p); /* Consume 'switch' */
    expect_punct(p, "(", "(");
    AstNode *expr = parse_expr(p);
    expect_punct(p, ")", ")");
    AstNode *body = parse_statement(p);
    AstNode *node = ast_create(NODE_SWITCH, line, col);
    node->data.switch_stmt.expr = expr;
    node->data.switch_stmt.body = body;
    return node;
}

/* Parses a return statement with optional return value */
static AstNode *parse_return(Parser *p) {
    int line = p->current.line;
    int col = p->current.column;
    advance(p); /* Consume 'return' */
    AstNode *expr = NULL;
    if (!check_punct(p, ";")) expr = parse_expr(p);
    expect_punct(p, ";", ";");
    AstNode *node = ast_create(NODE_RETURN, line, col);
    node->data.return_stmt.expr = expr;
    return node;
}

/* Emits diagnostic and skips unsupported language constructs */
static void skip_unsupported(Parser *p) {
    if (p->diags) {
        diag_list_add(p->diags, "I001", DIAG_INFO, p->current.line, p->current.column,
                      "unsupported construct skipped", "feature is out of project scope");
    }
    int brace_depth = 0;
    while (p->current.type != TOK_EOF) {
        if (check_punct(p, "{")) brace_depth++;
        else if (check_punct(p, "}")) {
            if (brace_depth > 0) brace_depth--;
            if (brace_depth == 0) { advance(p); if (check_punct(p, ";")) advance(p); return; }
        } else if (check_punct(p, ";") && brace_depth == 0) {
            advance(p);
            return;
        }
        advance(p);
    }
}

/* Parses a compound statement block enclosed in curly braces */
static AstNode *parse_block(Parser *p) {
    int line = p->current.line;
    int col = p->current.column;
    if (!expect_punct(p, "{", "{")) return NULL;

    AstNode *stmts = NULL;
    AstNode **tail = &stmts;
    while (!check_punct(p, "}") && p->current.type != TOK_EOF) {
        AstNode *stmt = NULL;
        if (is_type_token(p->current)) {
            stmt = parse_decl_or_func(p);
        } else {
            stmt = parse_statement(p);
        }
        if (stmt) {
            *tail = stmt;
            while (*tail && (*tail)->next) tail = &((*tail)->next);
            tail = &((*tail)->next);
        }
    }
    int close_line = p->current.line;
    expect_punct(p, "}", "}");
    AstNode *block = ast_create(NODE_BLOCK, line, col);
    block->end_line = close_line;
    block->data.block.statements = stmts;
    return block;
}

/* Parses a single statement matching standard C statement grammar */
static AstNode *parse_statement(Parser *p) {
    if (check_punct(p, "{")) return parse_block(p);
    if (check_kw(p, "if")) return parse_if(p);
    if (check_kw(p, "while")) return parse_while(p);
    if (check_kw(p, "for")) return parse_for(p);
    if (check_kw(p, "do")) return parse_do_while(p);
    if (check_kw(p, "switch")) return parse_switch(p);
    if (check_kw(p, "return")) return parse_return(p);
    if (check_kw(p, "break")) {
        AstNode *n = ast_create(NODE_BREAK, p->current.line, p->current.column);
        advance(p); expect_punct(p, ";", ";"); return n;
    }
    if (check_kw(p, "continue")) {
        AstNode *n = ast_create(NODE_CONTINUE, p->current.line, p->current.column);
        advance(p); expect_punct(p, ";", ";"); return n;
    }
    if (check_kw(p, "case")) {
        int line = p->current.line, col = p->current.column;
        advance(p); AstNode *expr = parse_expr(p); expect_punct(p, ":", ":");
        AstNode *n = ast_create(NODE_CASE, line, col); n->data.case_stmt.expr = expr; return n;
    }
    if (check_kw(p, "default")) {
        int line = p->current.line, col = p->current.column;
        advance(p); expect_punct(p, ":", ":");
        return ast_create(NODE_DEFAULT, line, col);
    }
    if (check_punct(p, ";")) {
        int l = p->current.line, c = p->current.column;
        advance(p);
        return ast_create(NODE_EMPTY_STMT, l, c);
    }
    if (check_kw(p, "struct") || check_kw(p, "union") || check_kw(p, "enum") ||
        check_kw(p, "typedef") || check_kw(p, "goto")) {
        skip_unsupported(p); return ast_create(NODE_EMPTY_STMT, p->current.line, p->current.column);
    }
    AstNode *e = parse_expr(p);
    expect_punct(p, ";", ";");
    AstNode *stmt = ast_create(NODE_EXPR_STMT, e ? e->line : p->current.line, e ? e->column : p->current.column);
    stmt->data.expr_stmt.expr = e;
    return stmt;
}

/* Parses formal parameter list for function definition or prototype */
static AstNode *parse_parameter_list(Parser *p) {
    if (match_punct(p, ")")) return NULL;
    if (check_kw(p, "void") && p->lexer->source[p->lexer->cursor] == ')') {
        advance(p); expect_punct(p, ")", ")"); return NULL;
    }
    AstNode *params = NULL;
    AstNode **tail = &params;
    while (true) {
        if (!is_type_token(p->current)) break;
        char *tname = parse_type_specifier(p);
        int pdepth = parse_pointer_depth(p);
        char *pname = NULL;
        int pline = p->current.line, pcol = p->current.column;
        if (p->current.type == TOK_IDENTIFIER) {
            pname = tok_to_str(p->current);
            advance(p);
        }
        bool is_arr = false;
        if (match_punct(p, "[")) { is_arr = true; match_punct(p, "]"); }
        AstNode *param = ast_create(NODE_PARAM, pline, pcol);
        param->data.param.type_name = tname;
        param->data.param.pointer_depth = pdepth;
        param->data.param.name = pname;
        param->data.param.is_array = is_arr;
        *tail = param;
        tail = &param->next;
        if (!match_punct(p, ",")) break;
    }
    expect_punct(p, ")", ")");
    return params;
}

/* Parses variable declarators following the first identifier in a declaration */
static AstNode *parse_var_decl_list(Parser *p, const char *base_type, int start_line, int start_col,
                                    char *first_name, int first_pdepth) {
    AstNode *decls = NULL;
    AstNode **tail = &decls;
    char *name = first_name;
    int pdepth = first_pdepth;

    while (true) {
        bool is_arr = false; int arr_sz = 0;
        if (match_punct(p, "[")) {
            is_arr = true;
            if (p->current.type == TOK_INT_LITERAL) {
                arr_sz = atoi(p->current.start); advance(p);
            }
            expect_punct(p, "]", "]");
        }
        AstNode *init = NULL;
        if (match_op(p, "=")) init = parse_assignment(p);

        AstNode *decl = ast_create(NODE_VAR_DECL, start_line, start_col);
        decl->data.var_decl.type_name = strdup(base_type);
        decl->data.var_decl.pointer_depth = pdepth;
        decl->data.var_decl.name = name;
        decl->data.var_decl.is_array = is_arr;
        decl->data.var_decl.array_size = arr_sz;
        decl->data.var_decl.init = init;
        *tail = decl; tail = &decl->next;

        if (!match_punct(p, ",")) break;
        pdepth = parse_pointer_depth(p);
        if (p->current.type != TOK_IDENTIFIER) { emit_expected_error(p, "identifier"); break; }
        name = tok_to_str(p->current); advance(p);
    }
    expect_punct(p, ";", ";");
    return decls;
}

/* Parses function definitions, prototypes, or comma-separated variable declarations */
static AstNode *parse_decl_or_func(Parser *p) {
    int start_line = p->current.line, start_col = p->current.column;
    char *base_type = parse_type_specifier(p);
    int pdepth = parse_pointer_depth(p);

    if (p->current.type != TOK_IDENTIFIER) {
        emit_expected_error(p, "identifier");
        free(base_type); synchronize(p); return NULL;
    }
    char *name = tok_to_str(p->current);
    advance(p);

    if (match_punct(p, "(")) {
        AstNode *params = parse_parameter_list(p);
        AstNode *body = NULL;
        if (check_punct(p, "{")) {
            body = parse_block(p);
        } else {
            expect_punct(p, ";", ";");
        }
        AstNode *func = ast_create(NODE_FUNCTION_DEF, start_line, start_col);
        func->end_line = body ? body->end_line : start_line;
        func->data.function_def.return_type = base_type;
        func->data.function_def.pointer_depth = pdepth;
        func->data.function_def.name = name;
        func->data.function_def.params = params;
        func->data.function_def.body = body;
        return func;
    }

    AstNode *decls = parse_var_decl_list(p, base_type, start_line, start_col, name, pdepth);
    free(base_type);
    return decls;
}

/* Initializes a parser state with an active lexer and diagnostics list */
void parser_init(Parser *parser, Lexer *lexer, DiagList *diags) {
    parser->lexer = lexer;
    parser->diags = diags;
    parser->panic_mode = false;
    parser->current = lexer_next(lexer);
}

/* Releases parser allocated state */
void parser_free(Parser *parser) {
    (void)parser;
}

/* Parses the entire token stream into a root translation unit AST node */
AstNode *parser_parse(Parser *parser) {
    AstNode *root = ast_create(NODE_TRANSLATION_UNIT, 1, 1);
    AstNode *decls = NULL;
    AstNode **tail = &decls;

    while (parser->current.type != TOK_EOF) {
        if (check_kw(parser, "struct") || check_kw(parser, "union") ||
            check_kw(parser, "enum") || check_kw(parser, "typedef")) {
            skip_unsupported(parser);
            continue;
        }
        if (is_type_token(parser->current)) {
            AstNode *node = parse_decl_or_func(parser);
            if (node) {
                *tail = node;
                while (*tail && (*tail)->next) tail = &((*tail)->next);
                tail = &((*tail)->next);
            }
        } else {
            AstNode *stmt = parse_statement(parser);
            if (stmt) {
                *tail = stmt;
                while (*tail && (*tail)->next) tail = &((*tail)->next);
                tail = &((*tail)->next);
            }
        }
    }
    root->data.translation_unit.declarations = decls;
    return root;
}
