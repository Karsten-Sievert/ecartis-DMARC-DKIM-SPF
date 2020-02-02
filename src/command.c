#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "command.h"
#include "config.h"
#include "core.h"

struct listserver_cmd *commands = NULL;

/* Start with a NULL pointer. */
void new_commands()
{
    commands = NULL;
}

/* Remove all commands (free memory) */
void nuke_commands()
{
    struct listserver_cmd *temp, *temp2;

    temp = commands;

    /* Walk the list and free all the command structures. */
    while(temp) {
        temp2 = temp->next;
        if(temp->name) free(temp->name);
        if(temp->desc) free(temp->desc);
        if(temp->syntax) free(temp->syntax);
        if(temp->altsyntax) free(temp->altsyntax);
        if(temp->adminsyntax) free(temp->adminsyntax);
        free(temp);
        temp = temp2;
    }
    commands = NULL;
}

/* Add a new command to the tree */
void add_command(char *name, char *desc, char *syntax, char *altsyntax,
                 char *adminsyntax, int flags, CmdFn cmd)
{
    struct listserver_cmd *temp;

    temp = (struct listserver_cmd *)malloc(sizeof(struct listserver_cmd));

    temp->name = strdup(name);
    temp->desc = strdup(desc);
    temp->syntax = NULL;
    temp->altsyntax = NULL;
    temp->adminsyntax = NULL;
    if(syntax) {
        temp->syntax = strdup(syntax);
    }
    if(altsyntax) {
        temp->altsyntax = strdup(altsyntax);
    }
    if(adminsyntax) {
        temp->adminsyntax = strdup(adminsyntax);
    }
    temp->flags = flags;
    temp->cmd = cmd;

    /* Add to the list */
    temp->next = commands;
    commands = temp;
}

/* Locate a command in the list, and return a pointer to it.
 * THIS IS NOT A 'SAFE' POINTER.  Don't alter the data unless you
 * want to alter the list itself.  Do -NOT- free it. */
struct listserver_cmd *find_command(char *name, int flags)
{
    struct listserver_cmd *temp;

    temp = commands;

    /* Walk the list... */
    while(temp) {
        /* And check for the name and type we requested. */
        if((!strcasecmp(temp->name,name)) && ((temp->flags & flags) == flags))
            return temp;
        temp = temp->next;
    }

    /* If not found, return NULL. */
    return NULL;
}

struct listserver_cmd *get_commands(void)
{
	return commands;
}
