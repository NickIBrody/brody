#include "parser.h"
#include <string.h>
#include <stdlib.h>

static Token adv(Parser *p){ Token t=p->cur; p->cur=p->peek; p->peek=lexer_next(p->lexer); return t; }
static int   chk(Parser *p, TokenType t){ return p->cur.type==t; }
static Token exp_tok(Parser *p, TokenType t){
    if(!chk(p,t)){ fprintf(stderr,"line %d: expected %d got %d ('%s')\n",p->cur.line,t,p->cur.type,p->cur.str); exit(1); }
    return adv(p);
}
static void skip_nl(Parser *p){ while(chk(p,TK_NEWLINE)) adv(p); }

static Node *parse_expr(Parser *p);
static Node *parse_stmt(Parser *p);

static Node *parse_block(Parser *p){
    Node *block=node_new(N_BLOCK,p->cur.line);
    block->block.stmts=nodelist_new();
    int use_brace=chk(p,TK_LBRACE);
    if(use_brace){
        adv(p); skip_nl(p);
        while(!chk(p,TK_RBRACE)&&!chk(p,TK_EOF)){
            skip_nl(p); if(chk(p,TK_RBRACE)) break;
            nodelist_push(&block->block.stmts,parse_stmt(p));
        }
        exp_tok(p,TK_RBRACE);
    } else {
        if(chk(p,TK_COLON)) adv(p);
        skip_nl(p);
        nodelist_push(&block->block.stmts,parse_stmt(p));
    }
    return block;
}

static Node *parse_primary(Parser *p){
    Token t=p->cur;
    if(chk(p,TK_INT)){ adv(p); Node*n=node_new(N_INT,t.line); n->ival=t.ival; return n; }
    if(chk(p,TK_FLOAT)){ adv(p); Node*n=node_new(N_FLOAT,t.line); n->fval=t.fval; return n; }
    if(chk(p,TK_STRING)){ adv(p); Node*n=node_new(N_STRING,t.line); n->sval=strdup(t.str); return n; }
    if(chk(p,TK_TRUE)){ adv(p); Node*n=node_new(N_BOOL,t.line); n->bval=1; return n; }
    if(chk(p,TK_FALSE)){ adv(p); Node*n=node_new(N_BOOL,t.line); n->bval=0; return n; }
    if(chk(p,TK_NIL)){ adv(p); return node_new(N_NIL,t.line); }
    if(chk(p,TK_LBRACKET)){
        adv(p); skip_nl(p);
        Node *n=node_new(N_LIST,t.line); n->list_items=nodelist_new();
        while(!chk(p,TK_RBRACKET)&&!chk(p,TK_EOF)){
            skip_nl(p); if(chk(p,TK_RBRACKET)) break;
            nodelist_push(&n->list_items,parse_expr(p));
            if(chk(p,TK_COMMA)) adv(p);
        }
        exp_tok(p,TK_RBRACKET); return n;
    }
    if(chk(p,TK_LPAREN)){ adv(p); Node*n=parse_expr(p); exp_tok(p,TK_RPAREN); return n; }
    if(chk(p,TK_NOT)){ adv(p); Node*n=node_new(N_UNOP,t.line); n->unop.op=TK_NOT; n->unop.operand=parse_primary(p); return n; }
    if(chk(p,TK_MINUS)){ adv(p); Node*n=node_new(N_UNOP,t.line); n->unop.op=TK_MINUS; n->unop.operand=parse_primary(p); return n; }
    if(chk(p,TK_INPUT)){
        adv(p); Node*n=node_new(N_INPUT,t.line); n->input_prompt.expr=NULL;
        if(chk(p,TK_LPAREN)){ adv(p); if(!chk(p,TK_RPAREN)) n->input_prompt.expr=parse_expr(p); exp_tok(p,TK_RPAREN); }
        return n;
    }
    if(chk(p,TK_IDENT)){ adv(p); Node*n=node_new(N_IDENT,t.line); n->sval=strdup(t.str); return n; }
    fprintf(stderr,"line %d: unexpected token '%s' (%d)\n",t.line,t.str,t.type); exit(1);
}

static Node *parse_postfix(Parser *p){
    Node *n=parse_primary(p);
    while(1){
        if(chk(p,TK_LBRACKET)){
            int line=p->cur.line; adv(p);
            Node *idx=parse_expr(p); exp_tok(p,TK_RBRACKET);
            Node *e=node_new(N_INDEX,line); e->index.obj=n; e->index.index=idx; n=e;
        } else if(chk(p,TK_DOT)){
            int line=p->cur.line; adv(p);
            char method[MAX_IDENT]; strncpy(method,p->cur.str,MAX_IDENT-1); exp_tok(p,TK_IDENT);
            exp_tok(p,TK_LPAREN);
            Node *e=node_new(N_METHOD,line); e->method.obj=n;
            strncpy(e->method.method,method,MAX_IDENT-1); e->method.args=nodelist_new();
            while(!chk(p,TK_RPAREN)&&!chk(p,TK_EOF)){ nodelist_push(&e->method.args,parse_expr(p)); if(chk(p,TK_COMMA)) adv(p); }
            exp_tok(p,TK_RPAREN); n=e;
        } else if(chk(p,TK_LPAREN)&&n->type==N_IDENT){
            int line=p->cur.line; adv(p);
            Node *e=node_new(N_CALL,line); strncpy(e->call.name,n->sval,MAX_IDENT-1);
            e->call.callee=n; n=NULL; e->call.args=nodelist_new();
            while(!chk(p,TK_RPAREN)&&!chk(p,TK_EOF)){ nodelist_push(&e->call.args,parse_expr(p)); if(chk(p,TK_COMMA)) adv(p); }
            exp_tok(p,TK_RPAREN); n=e;
        } else break;
    }
    return n;
}

static Node *parse_pow(Parser *p){
    Node *n=parse_postfix(p);
    if(chk(p,TK_STARSTAR)){
        int line=p->cur.line; adv(p);
        Node *e=node_new(N_BINOP,line); e->binop.op=TK_STARSTAR; e->binop.left=n; e->binop.right=parse_pow(p); return e;
    }
    return n;
}
static Node *parse_mul(Parser *p){
    Node *n=parse_pow(p);
    while(chk(p,TK_STAR)||chk(p,TK_SLASH)||chk(p,TK_PERCENT)){
        int line=p->cur.line; TokenType op=p->cur.type; adv(p);
        Node *e=node_new(N_BINOP,line); e->binop.op=op; e->binop.left=n; e->binop.right=parse_pow(p); n=e;
    }
    return n;
}
static Node *parse_add(Parser *p){
    Node *n=parse_mul(p);
    while(chk(p,TK_PLUS)||chk(p,TK_MINUS)){
        int line=p->cur.line; TokenType op=p->cur.type; adv(p);
        Node *e=node_new(N_BINOP,line); e->binop.op=op; e->binop.left=n; e->binop.right=parse_mul(p); n=e;
    }
    return n;
}
static Node *parse_cmp(Parser *p){
    Node *n=parse_add(p);
    while(chk(p,TK_LT)||chk(p,TK_GT)||chk(p,TK_LE)||chk(p,TK_GE)||chk(p,TK_EQ)||chk(p,TK_NEQ)){
        int line=p->cur.line; TokenType op=p->cur.type; adv(p);
        Node *e=node_new(N_BINOP,line); e->binop.op=op; e->binop.left=n; e->binop.right=parse_add(p); n=e;
    }
    return n;
}
static Node *parse_logic(Parser *p){
    Node *n=parse_cmp(p);
    while(chk(p,TK_AND)||chk(p,TK_OR)){
        int line=p->cur.line; TokenType op=p->cur.type; adv(p);
        Node *e=node_new(N_BINOP,line); e->binop.op=op; e->binop.left=n; e->binop.right=parse_cmp(p); n=e;
    }
    return n;
}
static Node *parse_expr(Parser *p){ return parse_logic(p); }

static Node *parse_stmt(Parser *p){
    skip_nl(p);
    int line=p->cur.line;

    if(chk(p,TK_LET)||chk(p,TK_IDENT)){
        int has_let=chk(p,TK_LET); if(has_let) adv(p);
        if(chk(p,TK_IDENT)){
            char name[MAX_IDENT]; strncpy(name,p->cur.str,MAX_IDENT-1); adv(p);
            if(chk(p,TK_LBRACKET)){
                adv(p); Node *idx=parse_expr(p); exp_tok(p,TK_RBRACKET); exp_tok(p,TK_ASSIGN);
                Node *val=parse_expr(p);
                Node *obj=node_new(N_IDENT,line); obj->sval=strdup(name);
                Node *n=node_new(N_INDEX_ASSIGN,line); n->index_assign.obj=obj; n->index_assign.index=idx; n->index_assign.value=val;
                skip_nl(p); return n;
            }
            if(chk(p,TK_ASSIGN)){
                adv(p); Node *val=parse_expr(p);
                Node *n=node_new(N_ASSIGN,line); n->assign.name=strdup(name); n->assign.value=val;
                skip_nl(p); return n;
            }
            if(chk(p,TK_PLUSEQ)||chk(p,TK_MINUSEQ)||chk(p,TK_STAREQ)||chk(p,TK_SLASHEQ)){
                TokenType op=p->cur.type; adv(p); Node *val=parse_expr(p);
                Node *n=node_new(N_OPASSIGN,line); n->opassign.name=strdup(name); n->opassign.op=op; n->opassign.value=val;
                skip_nl(p); return n;
            }
            if(!has_let){
                Node *id=node_new(N_IDENT,line); id->sval=strdup(name); Node *n=NULL;
                if(chk(p,TK_LPAREN)){
                    adv(p); n=node_new(N_CALL,line); strncpy(n->call.name,name,MAX_IDENT-1);
                    n->call.callee=id; id=NULL; n->call.args=nodelist_new();
                    while(!chk(p,TK_RPAREN)&&!chk(p,TK_EOF)){ nodelist_push(&n->call.args,parse_expr(p)); if(chk(p,TK_COMMA)) adv(p); }
                    exp_tok(p,TK_RPAREN);
                    while(chk(p,TK_DOT)||chk(p,TK_LBRACKET)){
                        if(chk(p,TK_LBRACKET)){
                            int ln=p->cur.line; adv(p); Node *idx=parse_expr(p); exp_tok(p,TK_RBRACKET);
                            Node *e=node_new(N_INDEX,ln); e->index.obj=n; e->index.index=idx; n=e;
                        } else {
                            int ln=p->cur.line; adv(p); char meth[MAX_IDENT]; strncpy(meth,p->cur.str,MAX_IDENT-1); exp_tok(p,TK_IDENT);
                            exp_tok(p,TK_LPAREN); Node *e=node_new(N_METHOD,ln); e->method.obj=n;
                            strncpy(e->method.method,meth,MAX_IDENT-1); e->method.args=nodelist_new();
                            while(!chk(p,TK_RPAREN)&&!chk(p,TK_EOF)){ nodelist_push(&e->method.args,parse_expr(p)); if(chk(p,TK_COMMA)) adv(p); }
                            exp_tok(p,TK_RPAREN); n=e;
                        }
                    }
                } else { n=node_new(N_EXPR_STMT,line); n->expr_stmt.expr=id; id=NULL; }
                node_free(id);
                Node *wrap=node_new(N_EXPR_STMT,line); wrap->expr_stmt.expr=n;
                skip_nl(p); return wrap;
            }
            fprintf(stderr,"line %d: expected = after let %s\n",line,name); exit(1);
        }
    }

    if(chk(p,TK_PRINT)){
        adv(p); Node *n=node_new(N_PRINT,line); n->print.count=0;
        int cap=4; n->print.exprs=malloc(cap*sizeof(Node*));
        if(chk(p,TK_LPAREN)){
            adv(p);
            while(!chk(p,TK_RPAREN)&&!chk(p,TK_EOF)){
                if(n->print.count>=cap){ cap*=2; n->print.exprs=realloc(n->print.exprs,cap*sizeof(Node*)); }
                n->print.exprs[n->print.count++]=parse_expr(p); if(chk(p,TK_COMMA)) adv(p);
            }
            exp_tok(p,TK_RPAREN);
        } else {
            if(n->print.count>=cap){ cap*=2; n->print.exprs=realloc(n->print.exprs,cap*sizeof(Node*)); }
            n->print.exprs[n->print.count++]=parse_expr(p);
        }
        skip_nl(p); return n;
    }

    if(chk(p,TK_IF)){
        adv(p); Node *n=node_new(N_IF,line); n->ifstmt.cond=parse_expr(p);
        if(chk(p,TK_COLON)) adv(p); skip_nl(p); n->ifstmt.then=parse_block(p);
        n->ifstmt.elif_conds=NULL; n->ifstmt.elif_thens=NULL; n->ifstmt.elif_count=0; n->ifstmt.els=NULL;
        skip_nl(p);
        while(chk(p,TK_ELIF)){
            adv(p); n->ifstmt.elif_count++;
            n->ifstmt.elif_conds=realloc(n->ifstmt.elif_conds,n->ifstmt.elif_count*sizeof(Node*));
            n->ifstmt.elif_thens=realloc(n->ifstmt.elif_thens,n->ifstmt.elif_count*sizeof(Node*));
            int i=n->ifstmt.elif_count-1; n->ifstmt.elif_conds[i]=parse_expr(p);
            if(chk(p,TK_COLON)) adv(p); skip_nl(p); n->ifstmt.elif_thens[i]=parse_block(p); skip_nl(p);
        }
        if(chk(p,TK_ELSE)){ adv(p); if(chk(p,TK_COLON)) adv(p); skip_nl(p); n->ifstmt.els=parse_block(p); }
        skip_nl(p); return n;
    }

    if(chk(p,TK_WHILE)){
        adv(p); Node *n=node_new(N_WHILE,line); n->whilestmt.cond=parse_expr(p);
        if(chk(p,TK_COLON)) adv(p); skip_nl(p); n->whilestmt.body=parse_block(p); skip_nl(p); return n;
    }

    if(chk(p,TK_FOR)){
        adv(p); Node *n=node_new(N_FOR,line); strncpy(n->forstmt.var,p->cur.str,MAX_IDENT-1);
        exp_tok(p,TK_IDENT); exp_tok(p,TK_IN); n->forstmt.iter=parse_expr(p);
        if(chk(p,TK_COLON)) adv(p); skip_nl(p); n->forstmt.body=parse_block(p); skip_nl(p); return n;
    }

    if(chk(p,TK_FN)){
        adv(p); Node *n=node_new(N_FN,line); strncpy(n->fn.name,p->cur.str,MAX_IDENT-1); exp_tok(p,TK_IDENT);
        exp_tok(p,TK_LPAREN); n->fn.params=NULL; n->fn.param_count=0;
        int pcap=4; n->fn.params=malloc(pcap*sizeof(char*));
        while(!chk(p,TK_RPAREN)&&!chk(p,TK_EOF)){
            if(n->fn.param_count>=pcap){ pcap*=2; n->fn.params=realloc(n->fn.params,pcap*sizeof(char*)); }
            n->fn.params[n->fn.param_count++]=strdup(p->cur.str); exp_tok(p,TK_IDENT); if(chk(p,TK_COMMA)) adv(p);
        }
        exp_tok(p,TK_RPAREN); if(chk(p,TK_COLON)) adv(p); skip_nl(p); n->fn.body=parse_block(p); skip_nl(p); return n;
    }

    if(chk(p,TK_RETURN)){
        adv(p); Node *n=node_new(N_RETURN,line);
        if(!chk(p,TK_NEWLINE)&&!chk(p,TK_EOF)&&!chk(p,TK_RBRACE)) n->ret.value=parse_expr(p);
        else n->ret.value=NULL;
        skip_nl(p); return n;
    }

    if(chk(p,TK_BREAK)){    adv(p); skip_nl(p); return node_new(N_BREAK,line); }
    if(chk(p,TK_CONTINUE)){ adv(p); skip_nl(p); return node_new(N_CONTINUE,line); }

    if(chk(p,TK_SHELL)){
        adv(p); Node *n=node_new(N_SHELL,line); n->shell.cmd=parse_expr(p); skip_nl(p); return n;
    }

    if(chk(p,TK_IMPORT)){
        adv(p); Node *n=node_new(N_IMPORT,line);
        if(chk(p,TK_STRING)||chk(p,TK_IDENT)){ strncpy(n->import.path,p->cur.str,511); adv(p); }
        skip_nl(p); return n;
    }

    Node *expr=parse_expr(p); Node *n=node_new(N_EXPR_STMT,line); n->expr_stmt.expr=expr; skip_nl(p); return n;
}

void parser_init(Parser *p, Lexer *l){ p->lexer=l; p->cur=lexer_next(l); p->peek=lexer_next(l); }

Node *parser_parse(Parser *p){
    Node *prog=node_new(N_BLOCK,1); prog->block.stmts=nodelist_new();
    skip_nl(p);
    while(!chk(p,TK_EOF)){ nodelist_push(&prog->block.stmts,parse_stmt(p)); skip_nl(p); }
    return prog;
}
