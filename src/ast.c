#include "ast.h"
#include <stdlib.h>
#include <string.h>

Node *node_new(NodeType type, int line){
    Node *n = calloc(1, sizeof(Node));
    n->type = type; n->line = line; return n;
}

NodeList nodelist_new(void){
    NodeList l; l.items=NULL; l.count=0; l.cap=0; return l;
}

void nodelist_push(NodeList *l, Node *n){
    if(l->count>=l->cap){ l->cap=l->cap?l->cap*2:8; l->items=realloc(l->items,l->cap*sizeof(Node*)); }
    l->items[l->count++]=n;
}

static void nodelist_free(NodeList *l){
    for(int i=0;i<l->count;i++) node_free(l->items[i]);
    free(l->items); l->items=NULL; l->count=l->cap=0;
}

void node_free(Node *n){
    if(!n) return;
    switch(n->type){
    case N_STRING: free(n->sval); break;
    case N_IDENT:  free(n->sval); break;
    case N_LIST:   nodelist_free(&n->list_items); break;
    case N_BINOP:  node_free(n->binop.left); node_free(n->binop.right); break;
    case N_UNOP:   node_free(n->unop.operand); break;
    case N_ASSIGN: free(n->assign.name); node_free(n->assign.value); break;
    case N_OPASSIGN: free(n->opassign.name); node_free(n->opassign.value); break;
    case N_INDEX:  node_free(n->index.obj); node_free(n->index.index); break;
    case N_INDEX_ASSIGN: node_free(n->index_assign.obj); node_free(n->index_assign.index); node_free(n->index_assign.value); break;
    case N_CALL:   node_free(n->call.callee); nodelist_free(&n->call.args); break;
    case N_METHOD: node_free(n->method.obj); nodelist_free(&n->method.args); break;
    case N_BLOCK:  nodelist_free(&n->block.stmts); break;
    case N_IF:
        node_free(n->ifstmt.cond); node_free(n->ifstmt.then);
        for(int i=0;i<n->ifstmt.elif_count;i++){ node_free(n->ifstmt.elif_conds[i]); node_free(n->ifstmt.elif_thens[i]); }
        free(n->ifstmt.elif_conds); free(n->ifstmt.elif_thens); node_free(n->ifstmt.els); break;
    case N_WHILE: node_free(n->whilestmt.cond); node_free(n->whilestmt.body); break;
    case N_FOR:   node_free(n->forstmt.iter); node_free(n->forstmt.body); break;
    case N_FN:
        for(int i=0;i<n->fn.param_count;i++) free(n->fn.params[i]);
        free(n->fn.params); node_free(n->fn.body); break;
    case N_RETURN: node_free(n->ret.value); break;
    case N_PRINT:
        for(int i=0;i<n->print.count;i++) node_free(n->print.exprs[i]);
        free(n->print.exprs); break;
    case N_INPUT:  node_free(n->input_prompt.expr); break;
    case N_SHELL:  node_free(n->shell.cmd); break;
    case N_EXPR_STMT: node_free(n->expr_stmt.expr); break;
    default: break;
    }
    free(n);
}
