/* Ecartis: Modular MLM
 * ---
 * File   : cmdarg.c
 * Purpose: Create, query, and maintain a list of dynamically-defined
 *        : command-line arguments for the program executable.  This
 *        : allows LPM modules to add new command-line parameters.
 * ---
 */

#include <stdlib.h>
#include <string.h>
#include "cmdarg.h"

struct listserver_cmdarg *args = NULL;

/* Function: new_cmdargs
 * Purpose : Initialize the command-line argument structure.  Only
 *         : ever called by the core binary itself, on startup.
 * Params  : none
 */
void new_cmdargs(void)
{
    args = NULL;
}

/* Function: nuke_cmdargs
 * Purpose : Clear out the command-line argument structure, freeing up
 *         : any allocated memory.
 * Params  : none
 */
void nuke_cmdargs(void)
{
    struct listserver_cmdarg *tmp, *tmp2;

    tmp = args;
    while(tmp) {
        tmp2 = tmp->next;
        if(tmp->arg) free(tmp->arg);
        if(tmp->parmdesc) free(tmp->parmdesc);
        free(tmp);
        tmp = tmp2;
    }
    args = NULL;
}

/* Function: find_cmdarg
 * Purpose : Look up a command-line argument and return it if present,
 *         : NULL otherwise.
 * Params  : arg - the string argument to look up (-c, -help, etc.)
 */
struct listserver_cmdarg *find_cmdarg(const char *arg)
{
    struct listserver_cmdarg *tmp = args;
    while(tmp) {
        if(strcmp(arg, tmp->arg) == 0)
            return tmp;
        tmp = tmp->next;
    }
    return NULL;
}

/* Function: add_cmdarg
 * Purpose : Adds a new command-line argument to the dynamic parser
 *         : table.
 * Params  : arg - command line argument being registered
 *         : params - number of parameters it takes
 *         : pdesc - a description (used for output of -help)
 *         : fn - a CmdArgFn which is called when this parameter is
 *         :      invoked.
 */
void add_cmdarg(const char *arg, int params, char *pdesc, CmdArgFn fn)
{
    struct listserver_cmdarg *tmp;
    tmp = (struct listserver_cmdarg *)malloc(sizeof(struct listserver_cmdarg));
    tmp->parmdesc = NULL;
    tmp->arg = strdup(arg);
    if(pdesc)
       tmp->parmdesc = strdup(pdesc);
    tmp->params = params;
    tmp->fn = fn;
 
    tmp->next = args;
    args = tmp; 
}

/* Function: get_cmdargs
 * Purpose : Returns the command-line argument list pointer; used for
 *         : walking the list by things like -help which need to display
 *         : all of them.
 * Params  : none
 */
struct listserver_cmdarg *get_cmdargs(void)
{
	return args;
}
