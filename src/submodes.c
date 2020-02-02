#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "flag.h"
#include "core.h"
#include "variables.h"
#include "fileapi.h"
#include "submodes.h"
#include "list.h"
#include "mystring.h"

struct submode *submodes;

void new_submodes(void)
{
    submodes = NULL;
}

void nuke_submodes(void)
{
   struct submode *temp, *temp2;
   temp = submodes;
   while(temp) {
      temp2 = temp->next;
      if(temp->modename) free (temp->modename);
      if(temp->flaglist) free (temp->flaglist);
      if(temp->description) free (temp->description);
      temp = temp2;
   }
   submodes = NULL;
}

struct submode *get_submodes(void)
{
    return submodes;
}

void read_submodes(const char *list)
{
    char buf[BIG_BUF];
    char *modename, *modeflags, *desc, *flag, *flagend;
    struct listserver_flag *fdata;
    struct submode *newmode;
    int valid = 1;
    FILE *fp;
    char *listdir;

    listdir = list_directory(get_string("list"));

    buffer_printf(buf, sizeof(buf) - 1, "%s/%s", listdir,
                             get_string("submodes-file"));

    free(listdir);

    fp = open_file(buf, "r");
    if(!fp) return;

    while(read_file(buf, sizeof(buf), fp)) {
        if(buf[0] == '#') continue; /* skip comments */
        if(buf[strlen(buf)-1] == '\n')
            buf[strlen(buf)-1] = '\0'; /* smash newline */
        modename = &buf[0];
        modeflags = strstr(buf, " : ");
        if(!modeflags) {
            log_printf(2, "Invalid subscription mode '%s'\n", modename);
            continue;
        }
        *modeflags = '\0';
        modeflags += 3;
        desc = strstr(modeflags, " : ");
        if(desc) {
           *desc = '\0';
           desc += 3;
        }
        /* okay, we have everything, let's validate the flags */
        if((*modeflags != '|') || (modeflags[strlen(modeflags)-1] != '|')) {
           log_printf(2, "Invalid subscription mode '%s'\n", modename);
           continue;
        }
        flag = modeflags+1;
        flagend = strchr(flag, '|');
        valid = 1;
        while(flagend) {
            *flagend = '\0';
            fdata = get_flag(flag);
            if(!fdata) {
                valid = 0;
                log_printf(2, "Flag '%s' in subscription mode '%s' is not valid.\n", flag, modename);
            }
            *flagend = '|';
            flag = flagend+1;
            flagend = strchr(flag, '|');
        }
        if(!valid) continue;
        newmode = (struct submode *)malloc(sizeof(struct submode));
        newmode->modename = strdup(modename);
        newmode->flaglist = strdup(modeflags);
        newmode->description = desc ? strdup(desc) : NULL;
        newmode->next = submodes;
        submodes = newmode;
    }
}

struct submode *get_submode(const char *mode)
{
   struct submode *temp = submodes;
   while(temp) {
       if(strcasecmp(temp->modename, mode) == 0) {
           return temp;
       }
       temp = temp->next;
   }
   return NULL;
}

const char *get_submode_flags(const char *mode)
{
   struct submode *temp = submodes;
   while(temp) {
       if(strcasecmp(temp->modename, mode) == 0) {
           return temp->flaglist;
       }
       temp = temp->next;
   }
   return NULL;
}
