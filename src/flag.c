#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "config.h"
#include "flag.h"

struct listserver_flag *flagdata;

/* Initialize the flag data */
void new_flags()
{
    flagdata = NULL;
}

/* Wipe the flag list */
void nuke_flags()
{
    struct listserver_flag *temp, *temp2;

    temp = flagdata;

    while(temp) {
        temp2 = temp->next;
        if(temp->name) free(temp->name);
        if(temp->desc) free(temp->desc);
        free(temp);
        temp = temp2;
    }
    flagdata = NULL;
}

/* Add a flag as long as it doesn't already exist */
void add_flag(const char *name, const char *desc, int admin)
{
    struct listserver_flag *tempflag;
 
    tempflag = get_flag(name);

    if (!tempflag) {
        char *tptr;

        tempflag = (struct listserver_flag *)malloc(sizeof(struct listserver_flag));
        tempflag->name = strdup(name);
        tempflag->desc = strdup(desc);
        tptr = tempflag->name;

        while(*tptr) {
            *tptr = toupper(*tptr);
            tptr++;
        }

        tempflag->admin = admin;
        tempflag->next = flagdata;
        flagdata = tempflag;
    }
}

/* Get the data about a specific flag */
struct listserver_flag *get_flag(const char *name)
{
    struct listserver_flag *tempflag;

    tempflag = flagdata;

    while(tempflag) {
        if (!strcasecmp(tempflag->name,name))
            return tempflag;
        tempflag = tempflag->next;
    }

    return NULL;
}

struct listserver_flag *get_flags(void)
{
	return flagdata;
}
