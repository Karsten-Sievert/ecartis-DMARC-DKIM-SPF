#ifndef _HOOKS_H
#define _HOOKS_H

#include "config.h"

#define MAX_HOOK_LEN		64

/* Result codes */
#define HOOK_RESULT_OK		0
#define HOOK_RESULT_FAIL	-1
#define HOOK_RESULT_STOP	-2

typedef int (*HookFn)(void);

struct listserver_hook
{
   char *type;
   unsigned int priority;
   HookFn hook;
   struct listserver_hook *next;
};

#define HOOK_HANDLER(a) int a(void)

extern void new_hooks(void);
extern void nuke_hooks(void);
extern void add_hook(const char *type, unsigned int priority, HookFn cmd);
extern int do_hooks(const char *hooktype);
extern struct listserver_hook *get_hooks(void);

#endif /* _HOOKS_H */
