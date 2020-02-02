#include "acctmgr-mod.h"
#include <string.h>
#include <stdlib.h>

CMD_HANDLER(cmd_vacation)
{
    const char *listname = LMAPI->get_var("list");
    const char *fromaddy = LMAPI->get_string("fromaddress");
    char *oldcookie = NULL;
    char cookiefile[BIG_BUF];
    char userfile[BIG_BUF];
    char cookie[BIG_BUF];
    char databuf[BIG_BUF];
    char datebuf[80];
    time_t now;
    int seconds;
    int i;
    char durationbuf[BIG_BUF];
    int curparm = 0;
    struct list_user u;

    LMAPI->buffer_printf(durationbuf, sizeof(durationbuf) - 1, "%s", LMAPI->get_string("vacation-default-duration"));

    if(params->num > 0) {
        if(LMAPI->get_bool("adminmode")) {
            if(strchr(params->words[0],'@')) {
               fromaddy = params->words[0];
               curparm = 1;
            } else {
               LMAPI->spit_status("Must provide a user in this context.");
               return CMD_RESULT_CONTINUE;
            }
        } else {
            if(LMAPI->list_valid(params->words[0])) {
                listname = params->words[0];
                curparm = 1;
            }
        }
        if(params->num > curparm) {
            LMAPI->buffer_printf(durationbuf, sizeof(durationbuf) - 1, "%s ", params->words[curparm++]);
            for(i = curparm; i < params->num; i++) {
                stringcat(durationbuf, params->words[i]);
                if(i < (params->num - 1)) stringcat(durationbuf, " ");
            }
        }
    }

    if(!listname) {
        LMAPI->spit_status("No list in present context.  To see the lists available on this machine, send %s the command 'lists'.", SERVICE_NAME_MC);
        return CMD_RESULT_CONTINUE;
    }

    if(!LMAPI->set_context_list(listname)) {
       LMAPI->nosuch(listname);
       /* Return CMD_RESULT_END cause otherwise the list context
        * might be incorrect for further changes
        */
        return CMD_RESULT_END;
    }

    if(!LMAPI->check_duration(durationbuf)) {
        LMAPI->spit_status("'%s' is not a valid duration.", durationbuf);
        LMAPI->result_printf("Durations are: [<num> d] [<num> h] [<num> m] [<num> s]\n");
        return CMD_RESULT_CONTINUE;
    }

    LMAPI->listdir_file(userfile,LMAPI->get_string("list"),"users");
    if(!LMAPI->user_find(userfile, fromaddy, &u)) {
        LMAPI->spit_status("User '%s' not subscribed to the list %s",
                           fromaddy, listname);
        return CMD_RESULT_CONTINUE;
    }

    LMAPI->listdir_file(cookiefile,LMAPI->get_string("list"),
       "cookies");
    oldcookie = LMAPI->find_cookie(cookiefile, 'V', fromaddy);
    LMAPI->set_var("vacation-default-duration", durationbuf, VAR_TEMP);
    seconds = LMAPI->get_seconds("vacation-default-duration");
    time(&now);
    now+=seconds;
#ifdef OBSDMOD
    LMAPI->buffer_printf(databuf, sizeof(databuf) - 1, "%lX;%s", (long)now, fromaddy);
#else
#if DEC_UNIX || _AIX
    LMAPI->buffer_printf(databuf, sizeof(databuf) - 1, "%X;%s", now, fromaddy);
#else
    LMAPI->buffer_printf(databuf, sizeof(databuf) - 1, "%lX;%s", now, fromaddy);
#endif /* DEC_UNIX */
#endif /* OBSDMOD */
    LMAPI->get_date(datebuf, sizeof(datebuf), now);

    if(oldcookie) {
        char *tmp = strstr(oldcookie, " : ");
        *tmp = '\0';
        if(!LMAPI->modify_cookie(cookiefile, oldcookie, databuf)) {
            LMAPI->spit_status("Unable to update timed vacation.");
            LMAPI->filesys_error(cookiefile);
            free(oldcookie);
            return CMD_RESULT_CONTINUE;
        }
        free(oldcookie);
        LMAPI->buffer_printf(databuf, sizeof(databuf) - 1, "Vacation updated to end on %s", datebuf);
    } else {
        LMAPI->set_var("cookie-for", fromaddy, VAR_TEMP);
        if(!LMAPI->request_cookie(cookiefile, &cookie[0], 'V', databuf)) {
            LMAPI->spit_status("Unable to set timed vacation.");
            LMAPI->filesys_error(cookiefile);
            return CMD_RESULT_CONTINUE;
        }
        LMAPI->clean_var("cookie-for", VAR_TEMP);
        LMAPI->buffer_printf(databuf, sizeof(databuf) - 1, "Vacation will end on %s", datebuf);
        LMAPI->user_setflag(&u, "VACATION", 0);
        LMAPI->user_write(userfile, &u);
    }
    LMAPI->spit_status(databuf);
    LMAPI->clean_var("vacation-default-duration", VAR_TEMP);
    return CMD_RESULT_CONTINUE;
}

HOOK_HANDLER(hook_setflag_vacation)
{
    char cookiefile[BIG_BUF];

    LMAPI->listdir_file(cookiefile,LMAPI->get_string("list"),"cookies");

    if(strcasecmp(LMAPI->get_string("setflag-flag"), "VACATION") == 0) {
        const char *fromaddy = LMAPI->get_string("setflag-user");
        char *cookie = LMAPI->find_cookie(cookiefile, 'V', fromaddy);
        char *tmp;
        if(cookie) {
            LMAPI->result_printf("The following removed the timed vacation for %s.", fromaddy);
            tmp = strstr(cookie, " : ");
            *tmp = '\0';
            LMAPI->del_cookie(cookiefile, cookie);
            if(strcasecmp(LMAPI->get_string("hooktype"), "SETFLAG") == 0) {
                char *user;
                struct list_user u;
                char userfile[BIG_BUF];

                user = strstr(tmp+3, ";");
                if(user) {
                    user++;
                    LMAPI->listdir_file(userfile,
                       LMAPI->get_string("list"),"users");
                    if(LMAPI->user_find(userfile, user, &u)) {
                        LMAPI->user_unsetflag(&u, "VACATION", 0);
                        LMAPI->user_write(userfile, &u);
                    }
                }
            }
            free(cookie);
        }
    }
    return HOOK_RESULT_OK;
}

COOKIE_HANDLER(destroy_vacation_cookie)
{
    char *user;
    struct list_user u;
    char userfile[BIG_BUF];
    time_t expire, now;

    time(&now);

    if(cookietype != 'V') return COOKIE_HANDLE_FAIL;

    user = strstr(cookiedata, ";");
    if(!user) return COOKIE_HANDLE_FAIL;
    *user++ = '\0';

#ifdef OBSDMOD
    sscanf(cookiedata, "%iX", &expire);
#else
#if DEC_UNIX || _AIX
    sscanf(cookiedata, "%X", &expire);
#else
    sscanf(cookiedata, "%lX", &expire);
#endif
#endif

    LMAPI->listdir_file(userfile,LMAPI->get_string("list"),"users");
    if(LMAPI->user_find(userfile, user, &u)) {
        if(expire < now) {
            LMAPI->user_unsetflag(&u, "VACATION", 0);
            LMAPI->user_write(userfile, &u);
        }
    }

    return COOKIE_HANDLE_OK;
}
