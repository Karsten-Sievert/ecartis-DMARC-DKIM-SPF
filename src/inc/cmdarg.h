#ifndef _CMDARG_H
#define _CMDARG_H

#define MAX_CMDARG_LEN 64

typedef int (*CmdArgFn)(char **argv, int count);

struct listserver_cmdarg {
    char *arg;
    CmdArgFn fn;
    int params;
    char *parmdesc;
    struct listserver_cmdarg *next;
};

#define CMDARG_HANDLER(a) int a(char **argv, int count)
#define CMDARG_ERR 0
#define CMDARG_OK 1
#define CMDARG_EXIT 2

extern void add_cmdarg(const char *arg, int params, char *pdesc, CmdArgFn fn);
extern void new_cmdargs(void);
extern void nuke_cmdargs(void);
extern struct listserver_cmdarg *find_cmdarg(const char *arg);
extern struct listserver_cmdarg *get_cmdargs(void);

extern struct listserver_cmdarg *args;
#endif /* _CMDARG_H */
