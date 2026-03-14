#include "interp.h"
#include "parser.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

Val *val_int(long long v){ Val *x=calloc(1,sizeof(Val)); x->type=VT_INT; x->ival=v; x->refs=1; return x; }
Val *val_float(double v){  Val *x=calloc(1,sizeof(Val)); x->type=VT_FLOAT; x->fval=v; x->refs=1; return x; }
Val *val_str(const char *s){ Val *x=calloc(1,sizeof(Val)); x->type=VT_STRING; x->sval=strdup(s?s:""); x->refs=1; return x; }
Val *val_bool(int b){ Val *x=calloc(1,sizeof(Val)); x->type=VT_BOOL; x->bval=!!b; x->refs=1; return x; }
Val *val_nil(void){ Val *x=calloc(1,sizeof(Val)); x->type=VT_NIL; x->refs=1; return x; }
Val *val_list(void){ Val *x=calloc(1,sizeof(Val)); x->type=VT_LIST; x->list=calloc(1,sizeof(VList)); x->list->cap=4; x->list->items=malloc(4*sizeof(Val*)); x->refs=1; return x; }
Val *val_ref(Val *v){ if(v) v->refs++; return v; }

void val_unref(Val *v){
    if(!v) return; if(--v->refs>0) return;
    if(v->type==VT_STRING) free(v->sval);
    else if(v->type==VT_LIST){ for(int i=0;i<v->list->len;i++) val_unref(v->list->items[i]); free(v->list->items); free(v->list); }
    else if(v->type==VT_FN){ if(v->fn){ for(int i=0;i<v->fn->np;i++) free(v->fn->params[i]); free(v->fn->params); free(v->fn); } }
    free(v);
}

Val *val_copy(Val *v){
    if(!v) return val_nil();
    switch(v->type){
    case VT_INT: return val_int(v->ival); case VT_FLOAT: return val_float(v->fval);
    case VT_STRING: return val_str(v->sval); case VT_BOOL: return val_bool(v->bval); case VT_NIL: return val_nil();
    case VT_LIST:{ Val *l=val_list(); for(int i=0;i<v->list->len;i++){ if(l->list->len>=l->list->cap){l->list->cap*=2;l->list->items=realloc(l->list->items,l->list->cap*sizeof(Val*));} l->list->items[l->list->len++]=val_copy(v->list->items[i]); } return l; }
    default: return val_nil();
    }
}

static void vlist_push(VList *l, Val *v){ if(l->len>=l->cap){l->cap=l->cap?l->cap*2:4;l->items=realloc(l->items,l->cap*sizeof(Val*));} l->items[l->len++]=v; }

char *val_tostr(Val *v){
    char buf[64]; if(!v) return strdup("nil");
    switch(v->type){
    case VT_INT: snprintf(buf,sizeof(buf),"%lld",v->ival); return strdup(buf);
    case VT_FLOAT: snprintf(buf,sizeof(buf),"%g",v->fval); return strdup(buf);
    case VT_STRING: return strdup(v->sval);
    case VT_BOOL: return strdup(v->bval?"true":"false");
    case VT_NIL: return strdup("nil");
    case VT_FN: return strdup("<fn>");
    case VT_LIST:{
        char *r=strdup("["); int first=1;
        for(int i=0;i<v->list->len;i++){
            if(!first){r=realloc(r,strlen(r)+3);strcat(r,", ");} first=0;
            char *s=val_tostr(v->list->items[i]); int is_str=(v->list->items[i]->type==VT_STRING);
            size_t nl=strlen(r)+strlen(s)+4; r=realloc(r,nl);
            if(is_str){strcat(r,"\"");strcat(r,s);strcat(r,"\"");}else strcat(r,s); free(s);
        }
        r=realloc(r,strlen(r)+2); strcat(r,"]"); return r;
    }
    default: return strdup("?");
    }
}

int val_truthy(Val *v){
    if(!v) return 0;
    switch(v->type){
    case VT_INT: return v->ival!=0; case VT_FLOAT: return v->fval!=0.0;
    case VT_STRING: return strlen(v->sval)>0; case VT_BOOL: return v->bval;
    case VT_NIL: return 0; case VT_LIST: return v->list->len>0; default: return 0;
    }
}

void val_print(Val *v){ char *s=val_tostr(v); printf("%s",s); free(s); }

Env *env_new(Env *parent){ Env *e=calloc(1,sizeof(Env)); e->parent=parent; e->cap=8; e->vars=malloc(8*sizeof(EnvVar)); return e; }
void env_free(Env *e){ for(int i=0;i<e->count;i++) val_unref(e->vars[i].val); free(e->vars); free(e); }

Val *env_get(Env *e, const char *name){
    for(Env *cur=e;cur;cur=cur->parent)
        for(int i=cur->count-1;i>=0;i--)
            if(!strcmp(cur->vars[i].name,name)) return cur->vars[i].val;
    return NULL;
}
void env_set(Env *e, const char *name, Val *v){
    for(Env *cur=e;cur;cur=cur->parent)
        for(int i=cur->count-1;i>=0;i--)
            if(!strcmp(cur->vars[i].name,name)){val_unref(cur->vars[i].val);cur->vars[i].val=val_ref(v);return;}
    env_set_local(e,name,v);
}
void env_set_local(Env *e, const char *name, Val *v){
    for(int i=e->count-1;i>=0;i--)
        if(!strcmp(e->vars[i].name,name)){val_unref(e->vars[i].val);e->vars[i].val=val_ref(v);return;}
    if(e->count>=e->cap){e->cap*=2;e->vars=realloc(e->vars,e->cap*sizeof(EnvVar));}
    strncpy(e->vars[e->count].name,name,MAX_IDENT-1); e->vars[e->count].val=val_ref(v); e->count++;
}

void interp_init(Interp *ip, Env *env){ ip->env=env; ip->do_return=ip->do_break=ip->do_continue=0; ip->retval=NULL; }

static Val *exec(Interp *ip, Node *n);

static Val *call_builtin(Interp *ip, const char *name, Val **args, int argc){
    if(!strcmp(name,"len")&&argc==1){ if(args[0]->type==VT_STRING) return val_int(strlen(args[0]->sval)); if(args[0]->type==VT_LIST) return val_int(args[0]->list->len); return val_int(0); }
    if(!strcmp(name,"type")&&argc==1){ const char *t[]={"int","float","string","bool","nil","list","fn"}; return val_str(t[args[0]->type]); }
    if(!strcmp(name,"int")&&argc==1){ if(args[0]->type==VT_INT) return val_copy(args[0]); if(args[0]->type==VT_FLOAT) return val_int((long long)args[0]->fval); if(args[0]->type==VT_STRING) return val_int(atoll(args[0]->sval)); if(args[0]->type==VT_BOOL) return val_int(args[0]->bval); return val_int(0); }
    if(!strcmp(name,"float")&&argc==1){ if(args[0]->type==VT_FLOAT) return val_copy(args[0]); if(args[0]->type==VT_INT) return val_float((double)args[0]->ival); if(args[0]->type==VT_STRING) return val_float(atof(args[0]->sval)); return val_float(0.0); }
    if(!strcmp(name,"str")&&argc==1){ char *s=val_tostr(args[0]); Val *r=val_str(s); free(s); return r; }
    if(!strcmp(name,"sqrt")&&argc==1){ double n=(args[0]->type==VT_INT)?(double)args[0]->ival:args[0]->fval; return val_float(sqrt(n)); }
    if(!strcmp(name,"abs")&&argc==1){ if(args[0]->type==VT_INT) return val_int(llabs(args[0]->ival)); if(args[0]->type==VT_FLOAT) return val_float(fabs(args[0]->fval)); return val_copy(args[0]); }
    if(!strcmp(name,"range")){
        long long s=0,e=0,step=1;
        if(argc==1) e=args[0]->ival; else if(argc>=2){s=args[0]->ival;e=args[1]->ival;} if(argc==3) step=args[2]->ival;
        Val *l=val_list(); for(long long i=s;step>0?i<e:i>e;i+=step) vlist_push(l->list,val_int(i)); return l;
    }
    if(!strcmp(name,"push")&&argc==2&&args[0]->type==VT_LIST){ vlist_push(args[0]->list,val_copy(args[1])); return val_nil(); }
    if(!strcmp(name,"pop")&&argc==1&&args[0]->type==VT_LIST){ if(!args[0]->list->len) return val_nil(); return args[0]->list->items[--args[0]->list->len]; }
    if(!strcmp(name,"join")&&argc==2&&args[0]->type==VT_LIST){
        char *sep=args[1]->type==VT_STRING?args[1]->sval:""; char *r=strdup("");
        for(int i=0;i<args[0]->list->len;i++){ char *s=val_tostr(args[0]->list->items[i]); r=realloc(r,strlen(r)+strlen(s)+strlen(sep)+2); strcat(r,s); if(i<args[0]->list->len-1) strcat(r,sep); free(s); }
        Val *v=val_str(r); free(r); return v;
    }
    if(!strcmp(name,"split")&&argc==2&&args[0]->type==VT_STRING){
        char *sep=args[1]->type==VT_STRING?args[1]->sval:" "; Val *l=val_list(); char *copy=strdup(args[0]->sval);
        char *tok=strtok(copy,sep); while(tok){vlist_push(l->list,val_str(tok));tok=strtok(NULL,sep);} free(copy); return l;
    }
    if(!strcmp(name,"upper")&&argc==1&&args[0]->type==VT_STRING){ char *s=strdup(args[0]->sval); for(int i=0;s[i];i++) if(s[i]>='a'&&s[i]<='z') s[i]-=32; Val *v=val_str(s); free(s); return v; }
    if(!strcmp(name,"lower")&&argc==1&&args[0]->type==VT_STRING){ char *s=strdup(args[0]->sval); for(int i=0;s[i];i++) if(s[i]>='A'&&s[i]<='Z') s[i]+=32; Val *v=val_str(s); free(s); return v; }
    if(!strcmp(name,"trim")&&argc==1&&args[0]->type==VT_STRING){
        char *s=strdup(args[0]->sval); int st=0,en=strlen(s)-1;
        while(s[st]==' '||s[st]=='\t'||s[st]=='\n') st++; while(en>st&&(s[en]==' '||s[en]=='\t'||s[en]=='\n')) en--;
        s[en+1]=0; Val *v=val_str(s+st); free(s); return v;
    }
    if(!strcmp(name,"contains")&&argc==2&&args[0]->type==VT_STRING&&args[1]->type==VT_STRING) return val_bool(strstr(args[0]->sval,args[1]->sval)!=NULL);
    if(!strcmp(name,"replace")&&argc==3&&args[0]->type==VT_STRING){
        char *hay=args[0]->sval,*needle=args[1]->sval,*rep=args[2]->sval;
        size_t nlen=strlen(needle),rlen=strlen(rep); char *r=strdup("");
        while(*hay){ char *f=strstr(hay,needle); if(!f){r=realloc(r,strlen(r)+strlen(hay)+1);strcat(r,hay);break;}
            size_t blen=f-hay; r=realloc(r,strlen(r)+blen+rlen+1); strncat(r,hay,blen); strcat(r,rep); hay=f+nlen; }
        Val *v=val_str(r); free(r); return v;
    }
    if(!strcmp(name,"exit")){ int code=(argc>0&&args[0]->type==VT_INT)?(int)args[0]->ival:0; exit(code); }
    fprintf(stderr,"undefined function '%s'\n",name); exit(1);
    (void)ip;
}

static Val *exec_block(Interp *ip, Node *block){
    Val *last=val_nil();
    for(int i=0;i<block->block.stmts.count;i++){
        val_unref(last); last=exec(ip,block->block.stmts.items[i]);
        if(ip->do_return||ip->do_break||ip->do_continue) break;
    }
    return last;
}

static Val *exec(Interp *ip, Node *n){
    if(!n) return val_nil();
    switch(n->type){
    case N_INT: return val_int(n->ival);
    case N_FLOAT: return val_float(n->fval);
    case N_STRING: return val_str(n->sval);
    case N_BOOL: return val_bool(n->bval);
    case N_NIL: return val_nil();
    case N_IDENT:{ Val *v=env_get(ip->env,n->sval); if(!v){fprintf(stderr,"line %d: undefined '%s'\n",n->line,n->sval);exit(1);} return val_copy(v); }
    case N_LIST:{ Val *l=val_list(); for(int i=0;i<n->list_items.count;i++) vlist_push(l->list,exec(ip,n->list_items.items[i])); return l; }
    case N_UNOP:{ Val *v=exec(ip,n->unop.operand); if(n->unop.op==TK_MINUS){if(v->type==VT_INT){v->ival=-v->ival;return v;}if(v->type==VT_FLOAT){v->fval=-v->fval;return v;}} if(n->unop.op==TK_NOT){int t=!val_truthy(v);val_unref(v);return val_bool(t);} return v; }
    case N_BINOP:{
        TokenType op=n->binop.op;
        if(op==TK_AND){Val *l=exec(ip,n->binop.left);if(!val_truthy(l))return l;val_unref(l);return exec(ip,n->binop.right);}
        if(op==TK_OR){Val *l=exec(ip,n->binop.left);if(val_truthy(l))return l;val_unref(l);return exec(ip,n->binop.right);}
        Val *l=exec(ip,n->binop.left); Val *r=exec(ip,n->binop.right); Val *res=NULL;
        int lf=(l->type==VT_FLOAT),rf=(r->type==VT_FLOAT);
        double ld=lf?l->fval:(double)l->ival, rd=rf?r->fval:(double)r->ival;
        switch(op){
        case TK_PLUS: if(l->type==VT_STRING||r->type==VT_STRING){char *ls=val_tostr(l),*rs=val_tostr(r);char *s=malloc(strlen(ls)+strlen(rs)+1);strcpy(s,ls);strcat(s,rs);res=val_str(s);free(ls);free(rs);free(s);}else if(lf||rf)res=val_float(ld+rd);else res=val_int(l->ival+r->ival);break;
        case TK_MINUS: res=(lf||rf)?val_float(ld-rd):val_int(l->ival-r->ival);break;
        case TK_STAR: if(l->type==VT_STRING&&r->type==VT_INT){int n2=(int)r->ival;size_t len=strlen(l->sval);char *s=malloc(len*n2+1);s[0]=0;for(int i=0;i<n2;i++)strcat(s,l->sval);res=val_str(s);free(s);}else res=(lf||rf)?val_float(ld*rd):val_int(l->ival*r->ival);break;
        case TK_SLASH: if(rd==0){fprintf(stderr,"division by zero\n");exit(1);} if(!lf&&!rf&&l->ival%r->ival==0)res=val_int(l->ival/r->ival);else res=val_float(ld/rd);break;
        case TK_PERCENT: if(r->ival==0){fprintf(stderr,"mod by zero\n");exit(1);}res=val_int(l->ival%r->ival);break;
        case TK_STARSTAR: res=val_float(pow(ld,rd));break;
        case TK_EQ: res=(l->type==VT_STRING&&r->type==VT_STRING)?val_bool(!strcmp(l->sval,r->sval)):val_bool(ld==rd);break;
        case TK_NEQ: res=(l->type==VT_STRING&&r->type==VT_STRING)?val_bool(strcmp(l->sval,r->sval)!=0):val_bool(ld!=rd);break;
        case TK_LT: res=val_bool(l->type==VT_STRING?strcmp(l->sval,r->sval)<0:ld<rd);break;
        case TK_GT: res=val_bool(l->type==VT_STRING?strcmp(l->sval,r->sval)>0:ld>rd);break;
        case TK_LE: res=val_bool(l->type==VT_STRING?strcmp(l->sval,r->sval)<=0:ld<=rd);break;
        case TK_GE: res=val_bool(l->type==VT_STRING?strcmp(l->sval,r->sval)>=0:ld>=rd);break;
        default: res=val_nil();break;
        }
        val_unref(l);val_unref(r);return res;
    }
    case N_ASSIGN:{ Val *v=exec(ip,n->assign.value); env_set(ip->env,n->assign.name,v); Val *r=val_copy(v); val_unref(v); return r; }
    case N_OPASSIGN:{
        Val *old=env_get(ip->env,n->opassign.name); Val *rhs=exec(ip,n->opassign.value); Val *res=NULL;
        if(!old){fprintf(stderr,"line %d: undefined '%s'\n",n->line,n->opassign.name);exit(1);}
        int lf=(old->type==VT_FLOAT),rf=(rhs->type==VT_FLOAT);
        double ld=lf?old->fval:(double)old->ival, rd=rf?rhs->fval:(double)rhs->ival;
        switch(n->opassign.op){
        case TK_PLUSEQ: if(old->type==VT_STRING){char *rs=val_tostr(rhs);char *s=malloc(strlen(old->sval)+strlen(rs)+1);strcpy(s,old->sval);strcat(s,rs);res=val_str(s);free(rs);free(s);}else res=(lf||rf)?val_float(ld+rd):val_int(old->ival+rhs->ival);break;
        case TK_MINUSEQ: res=(lf||rf)?val_float(ld-rd):val_int(old->ival-rhs->ival);break;
        case TK_STAREQ: res=(lf||rf)?val_float(ld*rd):val_int(old->ival*rhs->ival);break;
        case TK_SLASHEQ: if(rd==0){fprintf(stderr,"division by zero\n");exit(1);}res=val_float(ld/rd);break;
        default: res=val_nil();break;
        }
        env_set(ip->env,n->opassign.name,res); val_unref(rhs); Val *ret=val_copy(res); val_unref(res); return ret;
    }
    case N_INDEX:{
        Val *obj=exec(ip,n->index.obj); Val *idx=exec(ip,n->index.index); Val *res=NULL;
        if(obj->type==VT_LIST){long long i=idx->type==VT_INT?idx->ival:(long long)idx->fval;if(i<0)i+=obj->list->len;if(i>=0&&i<obj->list->len)res=val_copy(obj->list->items[i]);else{fprintf(stderr,"index %lld out of range\n",i);exit(1);}}
        else if(obj->type==VT_STRING){long long i=idx->type==VT_INT?idx->ival:(long long)idx->fval;if(i<0)i+=strlen(obj->sval);char buf[2]={0};if(i>=0&&i<(long long)strlen(obj->sval))buf[0]=obj->sval[i];res=val_str(buf);}
        else res=val_nil();
        val_unref(obj);val_unref(idx);return res;
    }
    case N_INDEX_ASSIGN:{
        Val *obj=env_get(ip->env,n->index_assign.obj->sval);
        if(!obj||obj->type!=VT_LIST){fprintf(stderr,"not a list\n");exit(1);}
        Val *idx=exec(ip,n->index_assign.index); Val *val=exec(ip,n->index_assign.value);
        long long i=idx->type==VT_INT?idx->ival:(long long)idx->fval; if(i<0)i+=obj->list->len;
        if(i>=0&&i<obj->list->len){val_unref(obj->list->items[i]);obj->list->items[i]=val_ref(val);}
        val_unref(idx); Val *r=val_copy(val); val_unref(val); return r;
    }
    case N_CALL:{
        Val *args[MAX_ARGS]; int argc=0;
        for(int i=0;i<n->call.args.count&&i<MAX_ARGS;i++) args[argc++]=exec(ip,n->call.args.items[i]);
        Val *fn_val=env_get(ip->env,n->call.name); Val *res=NULL;
        if(fn_val&&fn_val->type==VT_FN){
            Env *fn_env=env_new(fn_val->fn->closure);
            for(int i=0;i<fn_val->fn->np&&i<argc;i++) env_set_local(fn_env,fn_val->fn->params[i],args[i]);
            Env *saved=ip->env; ip->env=fn_env;
            int sr=ip->do_return; ip->do_return=0; Val *sv=ip->retval; ip->retval=NULL;
            res=exec_block(ip,fn_val->fn->body);
            if(ip->do_return){val_unref(res);res=ip->retval;ip->retval=NULL;}
            ip->do_return=sr; if(sv) ip->retval=sv;
            ip->env=saved; env_free(fn_env);
        } else res=call_builtin(ip,n->call.name,args,argc);
        for(int i=0;i<argc;i++) val_unref(args[i]);
        return res?res:val_nil();
    }
    case N_METHOD:{
        Val *obj=exec(ip,n->method.obj); Val *args[MAX_ARGS+1]; int argc=0; args[argc++]=obj;
        for(int i=0;i<n->method.args.count&&i<MAX_ARGS;i++) args[argc++]=exec(ip,n->method.args.items[i]);
        Val *res=call_builtin(ip,n->method.method,args,argc);
        for(int i=0;i<argc;i++) val_unref(args[i]); return res?res:val_nil();
    }
    case N_BLOCK: return exec_block(ip,n);
    case N_IF:{
        Val *cond=exec(ip,n->ifstmt.cond); int t=val_truthy(cond); val_unref(cond);
        if(t) return exec_block(ip,n->ifstmt.then);
        for(int i=0;i<n->ifstmt.elif_count;i++){Val *ec=exec(ip,n->ifstmt.elif_conds[i]);int et=val_truthy(ec);val_unref(ec);if(et)return exec_block(ip,n->ifstmt.elif_thens[i]);}
        if(n->ifstmt.els) return exec_block(ip,n->ifstmt.els); return val_nil();
    }
    case N_WHILE:{
        Val *res=val_nil();
        while(1){
            Val *cond=exec(ip,n->whilestmt.cond); int go=val_truthy(cond); val_unref(cond);
            if(!go) break;
            val_unref(res); res=exec_block(ip,n->whilestmt.body);
            if(ip->do_return) break;
            if(ip->do_break){ip->do_break=0;break;}
            if(ip->do_continue){ip->do_continue=0;continue;}
        }
        return res;
    }
    case N_FOR:{
        Val *iter=exec(ip,n->forstmt.iter); Val *res=val_nil();
        int len=0; if(iter->type==VT_LIST)len=iter->list->len; else if(iter->type==VT_STRING)len=strlen(iter->sval);
        for(int i=0;i<len;i++){
            Val *item=NULL;
            if(iter->type==VT_LIST)item=val_copy(iter->list->items[i]);
            else{char buf[2]={iter->sval[i],0};item=val_str(buf);}
            Env *loop_env=env_new(ip->env); env_set_local(loop_env,n->forstmt.var,item); val_unref(item);
            Env *saved=ip->env; ip->env=loop_env;
            val_unref(res); res=exec_block(ip,n->forstmt.body);
            ip->env=saved; env_free(loop_env);
            if(ip->do_return) break;
            if(ip->do_break){ip->do_break=0;break;}
            if(ip->do_continue){ip->do_continue=0;continue;}
        }
        val_unref(iter); return res;
    }
    case N_FN:{
        Val *fn=calloc(1,sizeof(Val)); fn->type=VT_FN; fn->refs=1; fn->fn=calloc(1,sizeof(VFn));
        fn->fn->np=n->fn.param_count; fn->fn->params=malloc(n->fn.param_count*sizeof(char*));
        for(int i=0;i<n->fn.param_count;i++) fn->fn->params[i]=strdup(n->fn.params[i]);
        fn->fn->body=n->fn.body; fn->fn->closure=ip->env;
        env_set(ip->env,n->fn.name,fn); val_unref(fn); return val_nil();
    }
    case N_RETURN:{ Val *v=n->ret.value?exec(ip,n->ret.value):val_nil(); ip->do_return=1; ip->retval=v; return val_copy(v); }
    case N_BREAK: ip->do_break=1; return val_nil();
    case N_CONTINUE: ip->do_continue=1; return val_nil();
    case N_PRINT:{
        for(int i=0;i<n->print.count;i++){Val *v=exec(ip,n->print.exprs[i]);if(i)printf(" ");val_print(v);val_unref(v);}
        printf("\n"); return val_nil();
    }
    case N_INPUT:{
        if(n->input_prompt.expr){Val *p=exec(ip,n->input_prompt.expr);char *s=val_tostr(p);printf("%s",s);fflush(stdout);free(s);val_unref(p);}
        char buf[4096]; if(!fgets(buf,sizeof(buf),stdin)) buf[0]=0;
        int l=strlen(buf); if(l>0&&buf[l-1]=='\n') buf[l-1]=0; return val_str(buf);
    }
    case N_SHELL:{ Val *cmd=exec(ip,n->shell.cmd); char *s=val_tostr(cmd); int r=system(s); free(s); val_unref(cmd); return val_int(r); }
    case N_IMPORT:{
        char path[520]; if(strstr(n->import.path,".br"))strncpy(path,n->import.path,519);else snprintf(path,sizeof(path),"%s.br",n->import.path);
        FILE *f=fopen(path,"r"); if(!f){fprintf(stderr,"cannot import '%s'\n",path);return val_nil();}
        fseek(f,0,SEEK_END);long sz=ftell(f);rewind(f);
        char *src=malloc(sz+2);fread(src,1,sz,f);src[sz]='\n';src[sz+1]=0;fclose(f);
        Lexer lx;lexer_init(&lx,src); Parser pr;parser_init(&pr,&lx); Node *prog=parser_parse(&pr);
        Interp sub;interp_init(&sub,ip->env); Val *r=exec_block(&sub,prog);
        val_unref(r);node_free(prog);free(src);return val_nil();
    }
    case N_EXPR_STMT: return exec(ip,n->expr_stmt.expr);
    default: fprintf(stderr,"unknown node %d\n",n->type); exit(1);
    }
}

Val *interp_exec(Interp *ip, Node *n){ return exec(ip,n); }
