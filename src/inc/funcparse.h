#ifndef __FUNCPARSE_H
#define __FUNCPARSE_H
typedef int(*FuncFn)(char **argv, char *result, char *error);

struct listserver_funcdef
{   
    char *funcname;
    char *desc;
    int nargs;
    FuncFn fn;
    struct listserver_funcdef *next;
};

#define FUNC_HANDLER(a) int a(char **argv, char *result, char *error)

extern void add_func(const char *name, int nargs, const char *desc, FuncFn fn);
extern void new_funcs(void);
extern void nuke_funcs(void);
extern struct listserver_funcdef *find_func(const char *name);
extern struct listserver_funcdef *get_funcs(void);
extern int parse_function(const char *buffer, char *result, char *error);

#endif
