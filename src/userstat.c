#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include "core.h"
#include "list.h"
#include "config.h"
#include "smtp.h"
#include "user.h"
#include "flag.h"
#include "fileapi.h"
#include "variables.h"
#include "compat.h"
#include "mystring.h"

int userstat_get_stat(const char *list, const char *username, const char *key,
                      char *value, int vallen)
{
    FILE *userdbfile;
    char filename[SMALL_BUF];
    char buffer[BIG_BUF];
    int done, haveuser, gotit;
    char *listdir;

    listdir = list_directory(list);
    buffer_printf(filename, sizeof(filename) - 1, "%s/userdb", listdir);
    free(listdir);
    if ((userdbfile = open_file(filename,"r")) == NULL) {
       log_printf(1,"Unable to open '%s'...\n", filename);
       return 0;
    }

    done = 0;
    haveuser = 0;
    gotit = 0;

    while (read_file(buffer, sizeof(buffer), userdbfile) && !done) {
       if (buffer[0] == '^') {
           if (!haveuser) {
               buffer[strlen(buffer) - 1] = 0;
               if (address_match(username,&buffer[1])) {
                   haveuser = 1;
               }
           } else {
               done = 1;
               haveuser = 0;
           }
       } else if (haveuser) {
           buffer[strlen(buffer) - 1] = 0;
           if (strncasecmp(buffer,key,strlen(key)) == 0) {
              char *ptr;

              ptr = strchr(buffer,'=');
              if (ptr) {
                 ptr = ptr + 1;
                 buffer_printf(value,vallen,"%s",ptr);
                 gotit = 1;
                 done = 1;
              }
           }          
       }
    }

    close_file(userdbfile);

    return gotit;
}

int userstat_set_stat(const char *list, const char *user, const char *key, const char *val)
{
    FILE *userdbfile, *tempdbfile;

    char filename[SMALL_BUF];
    char buffer[BIG_BUF];
    int haveuser, gotit, wroteit;
    char *listdir;

    listdir = list_directory(list);
    buffer_printf(filename, sizeof(filename) - 1, "%s/userdb", listdir);
    if ((userdbfile = open_exclusive(filename,"r+")) == NULL) {
       if ((userdbfile = open_exclusive(filename,"w+")) == NULL) {
          log_printf(1,"Unable to open '%s' for write...\n", filename);
          free(listdir);
          return 0;
       } else {
          write_file(userdbfile,"^%s\n",user);
          write_file(userdbfile,"%s=%s\n", key, val);
          close_file(userdbfile);
          free(listdir);
          return 1;
       }
    }

    haveuser = 0;
    gotit = 0;
    wroteit = 0;

    buffer_printf(filename, sizeof(filename) - 1, "%s/userdb.temp", listdir);
    free(listdir);
    if ((tempdbfile = open_exclusive(filename,"w+")) == NULL) {
       log_printf(1,"Unable to open '%s' for writing...\n", filename);
       close_file(userdbfile);
       free(listdir);
       return 0;
    }

    while (read_file(buffer, sizeof(buffer), userdbfile)) {
       if (buffer[0] == '^') {
           if (!haveuser) {
               buffer[strlen(buffer) - 1] = 0;
               if (address_match(user,&buffer[1])) {
                   haveuser = 1;
               }
               write_file(tempdbfile,"%s\n", buffer);
           } else {
               haveuser = 0;
               if (!gotit) {
                   write_file(tempdbfile,"%s=%s\n",key,val);
                   wroteit = 1;
               }
               write_file(tempdbfile,"%s",buffer);
           }
       } else if (haveuser) {
           buffer[strlen(buffer) - 1] = 0;
           if (strncasecmp(buffer,key,strlen(key)) == 0) {
              write_file(tempdbfile,"%s=%s\n", key, val);
              wroteit = 1;
              gotit = 1;
           } else {
              write_file(tempdbfile,"%s\n",buffer);
           }
       } else {
           write_file(tempdbfile,"%s",buffer);
       }
    }

    if (!wroteit) {
       if (!haveuser)
          write_file(tempdbfile,"^%s\n",user);
       write_file(tempdbfile,"%s=%s\n", key, val);
    }

    flush_file(tempdbfile);

    rewind_file(tempdbfile);
    rewind_file(userdbfile);
    truncate_file(userdbfile, 0);

    while (read_file(buffer, sizeof(buffer), tempdbfile)) {
       write_file(userdbfile,"%s",buffer);
    }
    
    close_file(tempdbfile);
    close_file(userdbfile);
    unlink_file(filename);

    return 1;    
}
