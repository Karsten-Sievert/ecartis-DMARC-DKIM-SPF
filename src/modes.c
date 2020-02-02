#include <stdlib.h>
#include <string.h>
#include "modes.h"

struct listserver_mode *modes = NULL;

/* Initialize the modes list */
void new_modes(void)
{
    modes = NULL;
}

/* Clean up the list of modes */
void nuke_modes(void)
{
    struct listserver_mode *tmp, *tmp2;

    tmp = modes;
    while(tmp) {
        tmp2 = tmp->next;
        if(tmp->modename) free(tmp->modename);
        free(tmp);
        tmp = tmp2;
    }
    modes = NULL;
}

/* Find the mode handle structure for a specific mode */
struct listserver_mode *find_mode(const char *mode)
{
    struct listserver_mode *tmp = modes;
    while(tmp) {
        if(strcmp(mode, tmp->modename) == 0)
            return tmp;
        tmp = tmp->next;
    }
    return NULL;
}

/* Add a mode handling routine */
void add_mode(const char *mode, ModeFn fn)
{
    struct listserver_mode *tmp;
    tmp = (struct listserver_mode *)malloc(sizeof(struct listserver_mode));
    tmp->modename = strdup(mode);
    tmp->modefn = fn;
 
    tmp->next = modes;
    modes = tmp; 
}

struct listserver_mode *get_modes()
{
	return modes;
}
