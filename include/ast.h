#ifndef AST_H
#define AST_H

#include "common.h"

typedef enum {
    N_INT, N_FLOAT, N_STRING, N_BOOL, N_NIL,
    N_IDENT, N_LIST,
    N_BINOP, N_UNOP,
    N_ASSIGN, N_OPASSIGN,
    N_INDEX, N_INDEX_ASSIGN,
    N_CALL, N_METHOD,
    N_BLOCK,
    N_IF,
    N_WHILE,
    N_FOR,
    N_FN,
    N_RETURN, N_BREAK, N_CONTINUE,
    N_PRINT,
    N_INPUT,
    N_SHELL,
    N_IMPORT,
    N_EXPR_STMT,
} NodeType;

typedef struct Node Node;

typedef struct {
    Node  **items;
    int     count, cap;
} NodeList;

struct Node {
    NodeType type;
    int      line;

    union {
        long long ival;
        double    fval;
        char     *sval;
        int       bval;

        struct { TokenType op; Node *left; Node *right; } binop;
        struct { TokenType op; Node *operand; }           unop;
        struct { char *name; Node *value; }               assign;
        struct { char *name; TokenType op; Node *value; } opassign;
        struct { Node *obj; Node *index; }                index;
        struct { Node *obj; Node *index; Node *value; }   index_assign;
        struct { Node *callee; NodeList args; char name[MAX_IDENT]; } call;
        struct { Node *obj; char method[MAX_IDENT]; NodeList args; } method;

        struct {
            NodeList stmts;
        } block;

        struct {
            Node    *cond;
            Node    *then;
            Node   **elif_conds;
            Node   **elif_thens;
            int      elif_count;
            Node    *els;
        } ifstmt;

        struct {
            Node *cond;
            Node *body;
        } whilestmt;

        struct {
            char  var[MAX_IDENT];
            Node *iter;
            Node *body;
        } forstmt;

        struct {
            char      name[MAX_IDENT];
            char    **params;
            int       param_count;
            Node     *body;
        } fn;

        struct { Node *value; } ret;

        struct {
            Node  **exprs;
            int     count;
        } print;

        struct { Node *expr; } input_prompt;
        struct { Node *cmd;  } shell;
        struct { char path[512]; } import;
        struct { Node *expr; } expr_stmt;

        NodeList list_items;
    };
};

Node    *node_new(NodeType type, int line);
NodeList nodelist_new(void);
void     nodelist_push(NodeList *l, Node *n);
void     node_free(Node *n);

#endif
