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
#include "mystring.h"
#include "compat.h"
#include "submodes.h"

int inbody;
int request_type;

int user_read(FILE *userfile, struct list_user *user)
{
    char tempbuf[HUGE_BUF];
    char address[BIG_BUF];
    char flags[HUGE_BUF];
    int assigned;
    int validate = get_bool("validate-users");

    while(read_file(tempbuf, sizeof(tempbuf), userfile)) {
       if(tempbuf[strlen(tempbuf)-1] == '\n')
           tempbuf[strlen(tempbuf)-1]='\0'; /* get rid of \n */
       assigned = sscanf(&tempbuf[0], "%s : %s", &address[0], &flags[0]);
       if(assigned != 2) {
           log_printf(0, "Invalid userfile line '%s'\n", tempbuf);
           continue;
       }
       if(validate) {
           if(check_address(address) == 0) {
               log_printf(0, "BOGUS email address '%s'\n", address);
               /* Only log the line, don't through it away */
               /* continue; */
           }
       }
       strncpy(user->address,address, BIG_BUF - 1);
       strncpy(user->flags,flags, HUGE_BUF - 1);
       return 1;
    }
    return 0;
}

int user_find_list(const char *listname, const char *addy,
                   struct list_user *user)
{
    char buffer[BIG_BUF];
    char *listdir;
    listdir = list_directory(listname);
    buffer_printf(buffer, sizeof(buffer) - 1, "%s/users", listdir);
    free(listdir);
    return(user_find(&buffer[0],addy,user));
}

int user_find(const char *listusers, const char *addy, struct list_user *user)
{
    FILE *userfile;
    int found;
    int loose = get_bool("no-loose-domain-match");

    if (!exists_file(listusers)) return 0;

    if((userfile = open_file(listusers,"r")) == NULL) return 0;
 
    found = 0;
    while(!found) {
        if(!user_read(userfile,user)) break;
        if(loose) {
            if (!strcasecmp(user->address,addy))
                found = 1;
        } else {
            if(address_match(addy, user->address))
                found = 1;
        }
    }

    close_file(userfile);
    return found;
}

int user_add(const char *listusers, const char *fromaddy)
{
    struct list_user user;
    const char *userflags = NULL;

    strncpy(&(user.address[0]),fromaddy, BIG_BUF - 1);
    if(get_var("submodes-mode"))
        userflags = get_submode_flags(get_string("submodes-mode"));
    if(!userflags)
        userflags = get_submode_flags("default");
    if(!userflags)
        userflags = get_var("default-flags");
    buffer_printf(user.flags, HUGE_BUF - 1, "%s", userflags);
    return user_write(listusers, &user);
}


int user_remove(const char *listusers, const char *fromaddy)
{
    FILE *myfile;

    if (!exists_file(listusers)) return ERR_LISTOPEN;

    if ((myfile = open_exclusive(listusers, "r+")) == NULL) {
        return ERR_LISTOPEN;
    } else {
        char buf[BIG_BUF];
        FILE *newfile;
        char temp[BIG_BUF];

        buffer_printf(buf, sizeof(buf) - 1, "%s.new", listusers);
        if ((newfile = open_exclusive(buf, "w+")) == NULL) {
            close_file(myfile);
            return ERR_LISTCREATE;
        } else {
            while(read_file(buf, sizeof(buf), myfile)) {
                sscanf(buf, "%s", &temp[0]);
                if (strcasecmp(temp,fromaddy)) {
                    if (!write_file(newfile,"%s", buf)) {
                        close_file(newfile);
                        close_file(myfile);
                        return ERR_LISTWRITE;
                    }
                }
            }
            rewind_file(newfile);
            rewind_file(myfile);
            truncate_file(myfile,0);
            while(read_file(buf, sizeof(buf), newfile)) {
               write_file(myfile,"%s", buf);
            }
            close_file(myfile);
            close_file(newfile);
            buffer_printf(buf, sizeof(buf) - 1, "%s.new", listusers);
            (void)unlink_file(buf);
            return 0;
        }
    }
}

int user_write(const char *listusers, struct list_user *user)
{
    FILE *myfile;
    char buf[BIG_BUF];
    FILE *newfile;
    char temp[BIG_BUF];
    int didwrite;

    if ((myfile = open_exclusive(listusers, "r+")) == NULL)
        return ERR_LISTOPEN;

    didwrite = 0;

    buffer_printf(buf, sizeof(buf) - 1, "%s.new", listusers);
    if ((newfile = open_file(buf, "w+")) == NULL) {
       close_file(myfile);
       return ERR_LISTCREATE;
    }

    while(read_file(buf, sizeof(buf), myfile)) {
        sscanf(buf, "%s", &temp[0]);
        if (strcasecmp(&temp[0],user->address)) {
            write_file(newfile,"%s", buf);
        } else {
            didwrite = 1;
            write_file(newfile,"%s : %s\n", user->address, user->flags);
        }
    }
    if (!didwrite) {
        write_file(newfile, "%s : %s\n", user->address, user->flags);
    }
    rewind_file(newfile);
    rewind_file(myfile);
    truncate_file(myfile, 0);
    while(read_file(buf, sizeof(buf), newfile)) {
       write_file(myfile, "%s", buf);
    }
    close_file(myfile);
    close_file(newfile);
    buffer_printf(buf, sizeof(buf) - 1, "%s.new", listusers);
    unlink_file(buf);
    return 0;
}

int user_hasflag(struct list_user *user, const char *flag)
{
    char tbuf[64];

    buffer_printf(tbuf, 64, "|%s|", flag);

    return ((strstr(user->flags,tbuf)) != NULL);
}

int user_setflag(struct list_user *user, const char *flag, int admin)
{
    char tbuf[HUGE_BUF];
    struct listserver_flag *flagrec;

    flagrec = get_flag(flag);

    if (!flagrec) return ERR_NOSUCHFLAG;

    if (flagrec->admin) {
        if ((!admin) && 
           !((flagrec->admin & ADMIN_SAFESET) && user_hasflag(user,"ADMIN")))
           return ERR_NOTADMIN;

        if (flagrec->admin & ADMIN_UNSETTABLE)
           return ERR_UNSETTABLE;
    }

    if (user_hasflag(user,flag)) return ERR_FLAGSET;

    if (user->flags[strlen(user->flags) - 1] != '|') {
        buffer_printf(tbuf, sizeof(tbuf) - 1, "%s|%s|", user->flags, flag);
    } else {
        buffer_printf(tbuf, sizeof(tbuf) - 1, "%s%s|", user->flags, flag);
    }

    buffer_printf(user->flags, HUGE_BUF - 1, "%s",tbuf);

    return 0;   
}

int user_unsetflag(struct list_user *user, const char *flag, int admin)
{
    struct listserver_flag *flagrec;
    char tbuf[HUGE_BUF];
    char fbuf[64];
    char *fptr;

    flagrec = get_flag(flag);

    if (!flagrec) return ERR_NOSUCHFLAG;

    if (flagrec->admin) {
        if ((!admin) &&
           !((flagrec->admin & ADMIN_SAFEUNSET) && user_hasflag(user,"ADMIN")))
           return ERR_NOTADMIN;
    }

    if (!user_hasflag(user,flag)) return ERR_FLAGNOTSET;
    buffer_printf(fbuf, 64, "%s|",flag);

    buffer_printf(tbuf, sizeof(tbuf) - 1, "%s", user->flags);

    fptr = strstr(tbuf,fbuf);
    *fptr++ = '\0';
    while(*fptr && (*fptr != '|')) {
        fptr++;
    }
    buffer_printf(user->flags, HUGE_BUF - 1, "%s%s",tbuf,++fptr);

    return 0;
}
