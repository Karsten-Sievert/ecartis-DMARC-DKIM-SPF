#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "file.h"

struct list_file *listfilerec;

/* initialize the file list */
void new_files()
{
    listfilerec = NULL;
}

/* Wipe the file list */
void nuke_files()
{
    struct list_file *temp, *temp2;

    temp = listfilerec;

    while(temp) {
        temp2 = temp->next;
        if(temp->name) free(temp->name);
        if(temp->varname) free(temp->varname);
        if(temp->desc) free(temp->desc);
        free(temp);
        temp = temp2;
    }
    listfilerec = NULL;
}

void add_file(const char *filename, const char *varname, const char *desc)
{
    add_file_flagged(filename, varname, desc, 0);
}

/* Add an actual file and what it's known as and what it is */
void add_file_flagged(const char *filename, const char *varname, const
                      char *desc, int flags)
{
    struct list_file *temp; 
    temp = find_file(filename);

    if (!temp) {
        temp = (struct list_file *)malloc(sizeof(struct list_file));
        temp->name = strdup(filename);
        temp->varname = strdup(varname);
        temp->desc = strdup(desc);
        temp->flags = flags;
        temp->next = listfilerec;
        listfilerec = temp;
    }
}

/* See if a specific name is valid */
struct list_file * find_file(const char *filename)
{
    struct list_file *temp;

    temp = listfilerec;

    while(temp) {
        if (!strcmp(temp->name,filename))
            return temp;
        temp = temp->next;
    }

    return NULL;
}

struct list_file * get_files(void)
{
	return listfilerec;
}
