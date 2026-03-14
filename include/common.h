#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

#define MAX_IDENT   256
#define MAX_VARS    512
#define MAX_PARAMS  32
#define MAX_ARGS    32

typedef enum {
    TK_EOF, TK_NEWLINE,
    TK_INT, TK_FLOAT, TK_STRING, TK_IDENT,
    TK_PLUS, TK_MINUS, TK_STAR, TK_SLASH, TK_PERCENT, TK_STARSTAR,
    TK_EQ, TK_NEQ, TK_LT, TK_GT, TK_LE, TK_GE,
    TK_ASSIGN, TK_PLUSEQ, TK_MINUSEQ, TK_STAREQ, TK_SLASHEQ,
    TK_LPAREN, TK_RPAREN, TK_LBRACKET, TK_RBRACKET, TK_LBRACE, TK_RBRACE,
    TK_COMMA, TK_DOT, TK_COLON,
    TK_LET, TK_FN, TK_RETURN,
    TK_IF, TK_ELIF, TK_ELSE,
    TK_WHILE, TK_FOR, TK_IN,
    TK_BREAK, TK_CONTINUE,
    TK_AND, TK_OR, TK_NOT,
    TK_TRUE, TK_FALSE, TK_NIL,
    TK_PRINT, TK_INPUT,
    TK_SHELL, TK_IMPORT,
} TokenType;

typedef struct {
    TokenType type;
    char      str[512];
    long long ival;
    double    fval;
    int       line;
} Token;

typedef struct Lexer {
    const char *src;
    int pos, line;
} Lexer;

void  lexer_init(Lexer *l, const char *src);
Token lexer_next(Lexer *l);

#endif
