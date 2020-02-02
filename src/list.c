#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/stat.h>
#endif // WIN32

#include "list.h"
#include "config.h"
#include "smtp.h"
#include "user.h"
#include "core.h"
#include "fileapi.h"
#include "variables.h"
#include "mystring.h"
#include "lpm-mods.h"
#include "submodes.h"
#include "cookie.h"

/* Get the list directory -- must be freed */
char *list_directory(const char *listname)
{
    char buffer[BIG_BUF];
    char *tlistname;

    /* tolower the listname, for safety */
    tlistname = lowerstr(listname);

    buffer_printf(buffer, sizeof(buffer) - 1, "%s/%s",
      get_string("lists-root"), tlistname);

    log_printf(20,"list_directory: %s\n", buffer);

    /* free the lowercase listname */
    free(tlistname);

    /* Return a list directory */
    return strdup(buffer);
}

/* Shorthand, because I got tired of using list_directory everywhere */
int listdir_file(char *buffer, const char *list, const char *filename)
{
   char *listdir;
   int val;

   listdir = list_directory(list);
   val = buffer_printf(buffer, BIG_BUF - 1, "%s/%s", listdir, filename);
   free(listdir);

   return val;
}

/* Set up a specific list by reading the config files and such */
int set_context_list(const char *list)
{
    char listtemp[BIG_BUF];
    const char *curlist;
    char *tempptr;

    curlist = get_var("list");

    /* Sanity check. */
    if (strchr(list,'@') || strchr(list,'!') || strchr(list,'/'))
       return 0;

    buffer_printf(listtemp, sizeof(listtemp) - 1, "%s", list);
    tempptr = &listtemp[0];

    while(*tempptr) {
       *tempptr = tolower(*tempptr);
       tempptr++;
    }

    list = &listtemp[0];
    if(!list_valid(list))  return 0;

    /* If we're not expiring all cookies on initial
       run, we must expire on list context switch */
    if (!get_bool("expire-all-cookies") && 
         strcmp(list,get_string("list"))) {
       char cookiefile[BIG_BUF];
       listdir_file(cookiefile,list,"cookies");
       expire_cookies(cookiefile);
    }

    if (!get_bool("adminmode")) {
        const char *cur_mode = get_string("mode");
        wipe_vars(VAR_LIST|VAR_TEMP);
        if (curlist) {
            if (strcmp(list, curlist) != 0) {
                set_var("list",list, VAR_GLOBAL);
                if((strcasecmp(cur_mode, "nolist") == 0) ||
                   (strcasecmp(cur_mode, "request") == 0)) {
                    result_printf("\nList context changed to '%s' by following command.",list);
                }
            }
        } else {
            set_var("list",list, VAR_GLOBAL);
            if((strcasecmp(cur_mode, "nolist") == 0) ||
               (strcasecmp(cur_mode, "request") == 0)) {
                result_printf("\nList context changed to '%s' by following command.",list);
            }
        }
        return (list_read_conf());
    } else {
        if(strcmp(list, curlist) != 0)
            return 0;
        return 1;
    }
}

/* Parse a config line into key/value pair */
/* Check: are name, value guaranteed to be large enough?  Need better
   error handling? */
void parse_cfg_line(const char *inputline, char *name, char *value)
{
    char *delim;

    delim = strchr(inputline, '=');
    
    if (!delim) return;		/* Return on error */

    strncpy(name, inputline, (delim - inputline));
    name[delim - inputline] = 0; /* Null-terminate it */

    strncpy(value, delim + 1, BIG_BUF - 1);

    trim(name);
    trim(value);
}

/* Read a specfic config file */
int read_conf(const char *filename, int level) 
{
    FILE *listfile;
    char  fileline[HUGE_BUF];
    char  varname[BIG_BUF];
    char  varval[BIG_BUF];

    log_printf(3, "Trying to read configuration file %s\n", filename);

    if (!exists_file(filename)) return 0;

    if ((listfile = open_file(filename, "r")) == NULL) return 0;
  
    while (read_file(fileline, sizeof(fileline), listfile)) {
        if ((fileline[0] != '#') && (strchr(&fileline[0],'='))) {
            if(fileline[strlen(fileline)-1] == '\n') 
                fileline[strlen(fileline) - 1] = '\0';

            while(fileline[strlen(fileline) - 1] == '\\') {
                char tbuf[BIG_BUF];

                fileline[strlen(fileline) - 1] = '\0';
                read_file(tbuf, sizeof(tbuf), listfile);
                stringcat(fileline, tbuf);
                if (fileline[strlen(fileline) - 1] == '\n') {
                    fileline[strlen(fileline) - 1] = '\0';
                }
            }

            parse_cfg_line(&fileline[0], &varname[0], &varval[0]);
            set_var(&varname[0],&varval[0], level);
        }
    }
    log_printf(3, "Done reading configuration file.\n");

    close_file(listfile);
    return 1;
}

/*
 * Search a config file for a specific keyword
 */
int read_conf_parm(const char *list, const char *parm, int level) 
{
    FILE *listfile;
    char  fileline[HUGE_BUF];
    char  varname[BIG_BUF];
    char  varval[BIG_BUF];
    char  filename[BIG_BUF];
    int success;
    char *listdir;

    listdir = list_directory(list);

    if (!listdir) return 0;

    buffer_printf(filename, sizeof(filename) - 1, "%s/%s", listdir,
      get_string("config-file"));

    free(listdir);

    log_printf(9, "Trying to read configuration file %s for parm %s\n",
               filename, parm);

    if (!exists_file(filename)) return 0;
    if ((listfile = open_file(filename, "r")) == NULL) return 0;

    log_printf(9,"Reading file...\n");

    success = 0;  

    while (read_file(fileline, sizeof(fileline), listfile)) {
        if ((fileline[0] != '#') && (strchr(&fileline[0],'='))) {
            if(fileline[strlen(fileline)-1] == '\n') 
                fileline[strlen(fileline) - 1] = '\0';

            while(fileline[strlen(fileline) - 1] == '\\') {
                char tbuf[BIG_BUF];

                fileline[strlen(fileline) - 1] = '\0';
                read_file(tbuf, sizeof(tbuf), listfile);
                stringcat(fileline, tbuf);
                if (fileline[strlen(fileline) - 1] == '\n') {
                    fileline[strlen(fileline) - 1] = '\0';
                }
            }

            parse_cfg_line(&fileline[0], &varname[0], &varval[0]);
            if (!strcasecmp(&varname[0],parm)) {
                set_var(&varname[0],&varval[0], level);
                success = 1;
            }
        }
        if (success) break;
    }

    close_file(listfile);

    log_printf(9, "Done reading configuration file.\n");

    return success;
}

/* check whether a list should be considered valid */
int list_valid(const char *listname)
{
    char buffer[BIG_BUF];
    char *temp, *listdir;

    if(get_bool("assume-lists-valid")) return 1;

    if(!listname) return 0;

    temp = lowerstr(listname);

    listdir = list_directory(temp);
    free(temp);

    if (!listdir) {
       log_printf(9,"list_valid: unable to generate list directory.\n");
       return 0;
    }

    buffer_printf(buffer, sizeof(buffer) - 1, "%s/%s", listdir, get_string("config-file"));

    if(!exists_file(buffer)) {
        log_printf(9,"list_valid: '%s' doesn't exist\n", buffer);
        free(listdir);
        return 0;
    }
    buffer_printf(buffer, sizeof(buffer) - 1, "%s/users", listdir);
    if(!exists_file(buffer)) {
        log_printf(9,"list_valid: '%s' doesn't exist\n", buffer);
        free(listdir);
        return 0;
    }
    free(listdir);
    return 1;
}

/* Read the conf file for the current list */
int list_read_conf(void)
{
    char buf[BIG_BUF];
    int res;
    char *listdir;

    listdir = list_directory(get_string("list"));

    buffer_printf(buf, sizeof(buf) - 1, "%s/%s", listdir, get_string("config-file"));

    free(listdir);

    res = read_conf(buf, VAR_LIST);
    if(res) {
        /* Only load the subscription modes on a valid list */
        nuke_submodes();
        read_submodes(get_string("list"));
        /* only do the module context switch on a valid read */
        switch_context_all_modules();
    } 
    return res;
}

/* Directory stack structure */
struct d_ent {
    LDIR dir;
    const char *path;
    struct d_ent *next;
};

static struct d_ent *dirs = NULL;

const char *get_dirstack_path(void)
{
    if(dirs) {
        log_printf(15, "peek_dirstack_path: %s\n", dirs->path);
        return dirs->path;
    }
    log_printf(15, "peek_dirstack_path: empty\n");
    return 0;
}

LDIR peek_dirstack(void)
{
    if(dirs) {
        log_printf(15, "peek_dirstack: %s\n", dirs->path);
        return dirs->dir;
    }
    log_printf(15, "peek_dirstack: empty\n");
    return 0;
}

LDIR pop_dirstack(void)
{
    if(dirs) {
        struct d_ent *tmp = dirs->next;
        log_printf(15, "pop_dirstack: popped %s\n", dirs->path);
        free((char *)dirs->path);
        free(dirs);
        dirs = tmp;
        if(dirs)
            return dirs->dir;
    }
    log_printf(15, "pop_dirstack: empty\n");
    return 0;
}

void push_dirstack(LDIR dir, const char *path)
{
    struct d_ent *tmp = (struct d_ent *)malloc(sizeof(struct d_ent));
    tmp->next = dirs;
    tmp->dir = dir;
    tmp->path = strdup(path);
    log_printf(15, "push_dirstack: pushed %s\n", path);
    dirs = tmp;
}

int step_list(char *buffer, int status, int readfirst)
{
   LDIR tmp = peek_dirstack();
   const char *path = get_dirstack_path();
   char buf[BIG_BUF];

    if(tmp && status && readfirst)
        status = next_dir(tmp, buffer);
    while(tmp) {
        while(status) {
            if(buffer[0] != '.') {
                buffer_printf(buf, sizeof(buf) - 1, "%s/%s", path, buffer);
                if(exists_dir(buf)) {
                    buffer_printf(buf, sizeof(buf) - 1, "%s/%s/users", path, buffer);
                    if(exists_file(buf))
                        return 1;
                    else {
                        buffer_printf(buf, sizeof(buf) - 1, "%s/%s", path, buffer);
                        status = walk_dir(buf, buffer, &tmp);
                        push_dirstack(tmp, buf);
                        return step_list(buffer, status, 0);
                    }
                }
            }
            status = next_dir(tmp, buffer);
        }
        close_dir(tmp);
        tmp = pop_dirstack();
        path = get_dirstack_path();
        if(tmp) {
            status = next_dir(tmp, buffer);
        }
    }
    return 0;
}

int walk_lists(char *buffer)
{
   LDIR tmp;
   int status = walk_dir(get_string("lists-root"), buffer, &tmp);
   if(!status) return status;
   push_dirstack(tmp, get_string("lists-root"));
   return step_list(buffer, status, 0);
}

int next_lists(char *buffer)
{
    return step_list(buffer, 1, 1);
}
