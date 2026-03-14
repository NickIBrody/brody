#include "common.h"

static int is_digit(char c){ return c>='0'&&c<='9'; }
static int is_alpha(char c){ return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_'; }
static int is_alnum(char c){ return is_alpha(c)||is_digit(c); }

static struct { const char *w; TokenType t; } kw[] = {
    {"let",TK_LET},{"fn",TK_FN},{"return",TK_RETURN},
    {"if",TK_IF},{"elif",TK_ELIF},{"else",TK_ELSE},
    {"while",TK_WHILE},{"for",TK_FOR},{"in",TK_IN},
    {"break",TK_BREAK},{"continue",TK_CONTINUE},
    {"and",TK_AND},{"or",TK_OR},{"not",TK_NOT},
    {"true",TK_TRUE},{"false",TK_FALSE},{"nil",TK_NIL},
    {"print",TK_PRINT},{"input",TK_INPUT},
    {"shell",TK_SHELL},{"import",TK_IMPORT},
    {NULL,0}
};

void lexer_init(Lexer *l, const char *src){ l->src=src; l->pos=0; l->line=1; }

Token lexer_next(Lexer *l){
    Token t; memset(&t,0,sizeof(t)); t.line=l->line;
again:
    while(l->src[l->pos]==' '||l->src[l->pos]=='\t'||l->src[l->pos]=='\r') l->pos++;
    char c=l->src[l->pos];
    if(c==0){ t.type=TK_EOF; return t; }
    if(c=='#'){ while(l->src[l->pos]&&l->src[l->pos]!='\n') l->pos++; goto again; }
    if(c=='\n'){ l->pos++; l->line++; t.type=TK_NEWLINE; strcpy(t.str,"\\n"); return t; }

    if(c=='"'||c=='\''){
        char q=c; l->pos++; int i=0;
        while(l->src[l->pos]&&l->src[l->pos]!=q){
            char ch=l->src[l->pos++];
            if(ch=='\\'){
                char e=l->src[l->pos++];
                if(e=='n') ch='\n'; else if(e=='t') ch='\t'; else ch=e;
            }
            if(i<510) t.str[i++]=ch;
        }
        if(l->src[l->pos]==q) l->pos++;
        t.str[i]=0; t.type=TK_STRING; return t;
    }

    if(is_digit(c)||(c=='.'&&is_digit(l->src[l->pos+1]))){
        int i=0; int fl=0;
        while(is_digit(l->src[l->pos])||l->src[l->pos]=='_')
            { char ch=l->src[l->pos++]; if(ch!='_') t.str[i++]=ch; }
        if(l->src[l->pos]=='.'&&is_digit(l->src[l->pos+1])){ fl=1; t.str[i++]=l->src[l->pos++]; while(is_digit(l->src[l->pos])) t.str[i++]=l->src[l->pos++]; }
        t.str[i]=0;
        if(fl){ t.type=TK_FLOAT; t.fval=atof(t.str); }
        else { t.type=TK_INT; t.ival=atoll(t.str); }
        return t;
    }

    if(is_alpha(c)){
        int i=0;
        while(is_alnum(l->src[l->pos])) t.str[i++]=l->src[l->pos++];
        t.str[i]=0;
        for(int k=0;kw[k].w;k++) if(!strcmp(t.str,kw[k].w)){ t.type=kw[k].t; return t; }
        t.type=TK_IDENT; return t;
    }

    l->pos++;
    char n=l->src[l->pos];
    switch(c){
    case '+': if(n=='='){ l->pos++; t.type=TK_PLUSEQ;  strcpy(t.str,"+="); return t; } t.type=TK_PLUS;  strcpy(t.str,"+"); return t;
    case '-': if(n=='='){ l->pos++; t.type=TK_MINUSEQ; strcpy(t.str,"-="); return t; } t.type=TK_MINUS; strcpy(t.str,"-"); return t;
    case '*': if(n=='*'){ l->pos++; t.type=TK_STARSTAR; strcpy(t.str,"**"); return t; }
              if(n=='='){ l->pos++; t.type=TK_STAREQ;  strcpy(t.str,"*="); return t; } t.type=TK_STAR;  strcpy(t.str,"*"); return t;
    case '/': if(n=='='){ l->pos++; t.type=TK_SLASHEQ; strcpy(t.str,"/="); return t; } t.type=TK_SLASH; strcpy(t.str,"/"); return t;
    case '%': t.type=TK_PERCENT; strcpy(t.str,"%"); return t;
    case '=': if(n=='='){ l->pos++; t.type=TK_EQ;  strcpy(t.str,"=="); return t; } t.type=TK_ASSIGN; strcpy(t.str,"="); return t;
    case '!': if(n=='='){ l->pos++; t.type=TK_NEQ; strcpy(t.str,"!="); return t; } t.type=TK_NOT; strcpy(t.str,"!"); return t;
    case '<': if(n=='='){ l->pos++; t.type=TK_LE;  strcpy(t.str,"<="); return t; } t.type=TK_LT; strcpy(t.str,"<"); return t;
    case '>': if(n=='='){ l->pos++; t.type=TK_GE;  strcpy(t.str,">="); return t; } t.type=TK_GT; strcpy(t.str,">"); return t;
    case '(': t.type=TK_LPAREN;   strcpy(t.str,"("); return t;
    case ')': t.type=TK_RPAREN;   strcpy(t.str,")"); return t;
    case '[': t.type=TK_LBRACKET; strcpy(t.str,"["); return t;
    case ']': t.type=TK_RBRACKET; strcpy(t.str,"]"); return t;
    case '{': t.type=TK_LBRACE;   strcpy(t.str,"{"); return t;
    case '}': t.type=TK_RBRACE;   strcpy(t.str,"}"); return t;
    case ',': t.type=TK_COMMA;    strcpy(t.str,","); return t;
    case '.': t.type=TK_DOT;      strcpy(t.str,"."); return t;
    case ':': t.type=TK_COLON;    strcpy(t.str,":"); return t;
    default:  goto again;
    }
}
