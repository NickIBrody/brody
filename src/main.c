#include "interp.h"
#include "parser.h"

int main(int argc, char **argv){
    if(argc<2){fprintf(stderr,"Brody v1.0  usage: brody <script.br>\n");return 1;}
    FILE *f=fopen(argv[1],"r");
    if(!f){fprintf(stderr,"cannot open '%s'\n",argv[1]);return 1;}
    fseek(f,0,SEEK_END);long sz=ftell(f);rewind(f);
    char *src=malloc(sz+2);
    fread(src,1,sz,f);
    src[sz]='\n';src[sz+1]=0;fclose(f);
    Lexer lx;lexer_init(&lx,src);
    Parser pr;parser_init(&pr,&lx);
    Node *prog=parser_parse(&pr);
    Env *env=env_new(NULL);
    Interp ip;interp_init(&ip,env);
    Val *res=interp_exec(&ip,prog);
    val_unref(res);node_free(prog);env_free(env);free(src);
    return 0;
}
