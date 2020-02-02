#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "hooks.h"
#include "config.h"
#include "core.h"
#include "variables.h"

struct listserver_hook *hooks = NULL;

/* Start with a NULL pointer. */
void new_hooks()
{
    hooks = NULL;
}

/* Remove all commands (free memory) */
void nuke_hooks()
{
    struct listserver_hook *temp, *temp2;

    temp = hooks;

    /* Walk the list and free all the command structures. */
    while(temp) {
        temp2 = temp->next;
        if(temp->type) free(temp->type);
        free(temp);
        temp = temp2;
    }
    hooks = NULL;
}

/* Add a new command to the tree */
void add_hook(const char *type, unsigned int priority, HookFn hook )
{
    struct listserver_hook *temp, **temp2;

    temp = (struct listserver_hook *)malloc(sizeof(struct listserver_hook));

    temp->type = strdup(type);
    temp->priority = priority;
    temp->hook = hook;

    /* Add to the list */
    temp2 = &hooks;
    while (*temp2 && (*temp2)->priority <= priority)
       temp2 = &((*temp2)->next);
    temp->next = *temp2;
    *temp2 = temp;
}

/* Handle a hook type */
int do_hooks(const char *hooktype)
{
    int count, result;
    struct listserver_hook *temp;
    char *oldhooktype;

    oldhooktype = NULL;

    if (get_var("hooktype")) {
       oldhooktype = strdup(get_var("hooktype"));
    }

    count = 0;
    result = 0;
    temp = hooks ;

    set_var("hooktype", hooktype, VAR_TEMP);
    /* Walk the list until out of commands or a command returned failure. */
    while(temp && (result == HOOK_RESULT_OK)) {

        /* If we're the right type, run us. */
        if (strcmp(temp->type, hooktype)==0) {
            log_printf(15,"Running a hook type '%s'\n", hooktype);
            result = (temp->hook)();
            count++;
        }

        temp = temp->next;
    }
    clean_var("hooktype", VAR_TEMP);
    if(oldhooktype) {
       set_var("hooktype", oldhooktype, VAR_TEMP);
       free(oldhooktype);
    }

    return result;
}

struct listserver_hook *get_hooks()
{
	return hooks;
}
