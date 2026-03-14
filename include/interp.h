#ifndef INTERP_H
#define INTERP_H

#include "ast.h"

typedef enum { VT_INT, VT_FLOAT, VT_STRING, VT_BOOL, VT_NIL, VT_LIST, VT_FN } VType;

typedef struct Val Val;
typedef struct VList { Val **items; int len, cap; } VList;
typedef struct { char **params; int np; Node *body; struct Env *closure; } VFn;

struct Val {
    VType type;
    int   refs;
    union {
        long long ival;
        double    fval;
        char     *sval;
        int       bval;
        VList    *list;
        VFn      *fn;
    };
};

typedef struct EnvVar { char name[MAX_IDENT]; Val *val; } EnvVar;
typedef struct Env {
    EnvVar  *vars;
    int      count, cap;
    struct Env *parent;
} Env;

Val *val_int(long long v);
Val *val_float(double v);
Val *val_str(const char *s);
Val *val_bool(int b);
Val *val_nil(void);
Val *val_list(void);
Val *val_ref(Val *v);
void val_unref(Val *v);
Val *val_copy(Val *v);
char *val_tostr(Val *v);
int  val_truthy(Val *v);
void val_print(Val *v);

Env *env_new(Env *parent);
void env_free(Env *e);
Val *env_get(Env *e, const char *name);
void env_set(Env *e, const char *name, Val *v);
void env_set_local(Env *e, const char *name, Val *v);

typedef struct {
    Env  *env;
    int   do_return, do_break, do_continue;
    Val  *retval;
} Interp;

void  interp_init(Interp *i, Env *env);
Val  *interp_exec(Interp *i, Node *n);

#endif
