#include "config.h"
#include "module.h"

#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <unistd.h>
#endif // WIN32

struct listserver_module *modules;

void new_modules()
{
    modules = NULL;
}

void nuke_modules()
{
    struct listserver_module *temp, *temp2;

    temp = modules;

    while (temp) {
        temp2 = temp->next;
        if(temp->name) free(temp->name);
        if(temp->desc) free(temp->desc);
        free(temp);
        temp = temp2;
    }
    modules = NULL;
}


void add_module(const char *name, const char *desc)
{
    struct listserver_module *tempmod;
 
    tempmod = (struct listserver_module *)malloc(sizeof(struct listserver_module));
    tempmod->name = strdup(name);
    tempmod->desc = strdup(desc);
    tempmod->next = modules;
    modules = tempmod;
}

struct listserver_module *get_modules()
{
	return modules;
}
