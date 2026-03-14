#ifndef PARSER_H
#define PARSER_H

#include "common.h"
#include "ast.h"

typedef struct {
    Lexer *lexer;
    Token  cur;
    Token  peek;
} Parser;

void  parser_init(Parser *p, Lexer *l);
Node *parser_parse(Parser *p);

#endif
