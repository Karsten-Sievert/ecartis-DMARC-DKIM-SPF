#include "acctmgr-mod.h"
#include <string.h>
#include <stdlib.h>

CMD_HANDLER(cmd_tempban)
{
    const char *listname = LMAPI->get_var("list");
    const char *fromaddy = LMAPI->get_var("fromaddress");
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
    char *listdir;

    LMAPI->buffer_printf(durationbuf, sizeof(durationbuf) - 1, "%s", LMAPI->get_string("tempban-default-duration"));

    if(params->num > 0) {
        if(LMAPI->get_bool("adminmode")) {
            if(strchr(params->words[0],'@')) {
               fromaddy = params->words[0];
               curparm = 1;
            } else {
               LMAPI->spit_status("Must provide a user!");
               return CMD_RESULT_CONTINUE;
            }
        } else {
            LMAPI->spit_status("The tempban command only works in admin mode.");
            return CMD_RESULT_CONTINUE;
        }
        if(params->num > curparm) {
            LMAPI->buffer_printf(durationbuf, sizeof(durationbuf) - 1, "%s ", params->words[curparm++]);
            for(i = curparm; i < params->num; i++) {
                stringcat(durationbuf, params->words[i]);
                if(i < (params->num - 1)) stringcat(durationbuf, " ");
            }
        }
    }

    if(!LMAPI->check_duration(durationbuf)) {
        LMAPI->spit_status("'%s' is not a valid duration.", durationbuf);
        LMAPI->result_printf("Durations are: [<num> d] [<num> h] [<num> m] [<num> s]\n");
        return CMD_RESULT_CONTINUE;
    }

    listdir = LMAPI->list_directory(LMAPI->get_string("list"));
    LMAPI->buffer_printf(userfile, sizeof(userfile) - 1, "%s/users", listdir);
    if(!LMAPI->user_find(userfile, fromaddy, &u)) {
        LMAPI->spit_status("User '%s' not subscribed to the list %s",
                           fromaddy, listname);
        free(listdir);
        return CMD_RESULT_CONTINUE;
    }

    LMAPI->buffer_printf(cookiefile, sizeof(cookiefile) - 1, "%s/cookies", listdir);
    free(listdir);
    oldcookie = LMAPI->find_cookie(cookiefile, 'T', fromaddy);
    LMAPI->set_var("tempban-default-duration", durationbuf, VAR_TEMP);
    seconds = LMAPI->get_seconds("tempban-default-duration");
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
            LMAPI->spit_status("Unable to update tempban.");
            LMAPI->filesys_error(cookiefile);
            free(oldcookie);
            return CMD_RESULT_CONTINUE;
        }
        free(oldcookie);
        LMAPI->buffer_printf(databuf, sizeof(databuf) - 1, "Tempban updated to end on %s", datebuf);
        LMAPI->spit_status(databuf);

        LMAPI->set_var("tempban-end-date",datebuf,VAR_TEMP);

        LMAPI->buffer_printf(databuf, sizeof(databuf) - 1,
            "%s: Temporary ban updated", listname);

        LMAPI->set_var("task-form-subject",databuf,VAR_TEMP);

           LMAPI->task_heading(fromaddy);
           LMAPI->smtp_body_text("The temporary ban set on you for the list ");
           LMAPI->smtp_body_line(LMAPI->get_string("list"));
           LMAPI->smtp_body_line("has been updated.  Your temporary ban will now end");
           LMAPI->smtp_body_text("at ");
           LMAPI->smtp_body_line(datebuf);
           LMAPI->task_ending();

        LMAPI->clean_var("task-form-subject", VAR_TEMP);
        LMAPI->clean_var("tempban-end-date", VAR_TEMP);

    } else {
        LMAPI->set_var("cookie-for", fromaddy, VAR_TEMP);
        if(!LMAPI->request_cookie(cookiefile, &cookie[0], 'T', databuf)) {
            LMAPI->spit_status("Unable to set temporary ban.");
            LMAPI->filesys_error(cookiefile);
            return CMD_RESULT_CONTINUE;
        }
        LMAPI->clean_var("cookie-for", VAR_TEMP);
        LMAPI->user_setflag(&u, "NOPOST", 1);
        LMAPI->user_write(userfile, &u);
        LMAPI->buffer_printf(databuf, sizeof(databuf) - 1, "Banning will end on %s", datebuf);
        LMAPI->spit_status(databuf);

        LMAPI->set_var("tempban-end-date",datebuf,VAR_TEMP);

        LMAPI->buffer_printf(databuf, sizeof(databuf) - 1,
            "%s: Temporarily banned", listname);

        LMAPI->set_var("task-form-subject",databuf,VAR_TEMP);

        listdir = LMAPI->list_directory(listname);
        LMAPI->buffer_printf(databuf, sizeof(databuf) - 1, "%s/%s",
            listdir,LMAPI->get_string("tempban-file"));
        free(listdir);

        if (!LMAPI->send_textfile_expand(fromaddy,databuf)) {
           LMAPI->task_heading(fromaddy);
           LMAPI->smtp_body_text("You have been temporarily banned from posting to the list ");
           LMAPI->smtp_body_line(listname);
           LMAPI->smtp_body_line("");
           LMAPI->smtp_body_text("The ban will expire at ");
           LMAPI->smtp_body_line(datebuf);
           LMAPI->task_ending();
        }

        LMAPI->clean_var("task-form-subject", VAR_TEMP);
        LMAPI->clean_var("tempban-end-date", VAR_TEMP);
    }
    LMAPI->clean_var("tempban-default-duration", VAR_TEMP);
    return CMD_RESULT_CONTINUE;
}

HOOK_HANDLER(hook_setflag_nopost)
{
    char cookiefile[BIG_BUF];
    char *listdir;

    listdir = LMAPI->list_directory(LMAPI->get_string("list"));
    LMAPI->buffer_printf(cookiefile, sizeof(cookiefile) - 1, "%s/cookies", listdir);

    if ( (LMAPI->get_bool("adminmode"))
      && (strcasecmp(LMAPI->get_string("setflag-flag"),"NOPOST") == 0)) {
        const char *fromaddy = LMAPI->get_string("setflag-user");
        char *cookie = LMAPI->find_cookie(cookiefile, 'T', fromaddy);
        char *tmp;
        if(cookie) {
            LMAPI->result_printf("The following removed the temporary ban for %s.\n", fromaddy);
            tmp = strstr(cookie, " : ");
            *tmp = '\0';
            LMAPI->del_cookie(cookiefile, cookie);
            if(strcasecmp(LMAPI->get_string("hooktype"), "UNSETFLAG") == 0) {
                char *user;
                struct list_user u;
                char userfile[BIG_BUF];
                char tempbuffer[BIG_BUF];

                user = strstr(tmp+3, ";");
                if(user) {
                    user++;
                    LMAPI->buffer_printf(userfile, sizeof(userfile) - 1, "%s/users", listdir);
                    if(LMAPI->user_find(userfile, user, &u)) {
                        LMAPI->user_unsetflag(&u, "NOPOST", 1);
                        LMAPI->user_write(userfile, &u);
                    }
                    LMAPI->buffer_printf(tempbuffer, sizeof(tempbuffer) - 1,"%s: Temporary ban removed", LMAPI->get_string("list"));
                    LMAPI->set_var("task-form-subject",tempbuffer,VAR_TEMP);

                    LMAPI->buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%s/%s",
                        listdir,
                        LMAPI->get_string("tempban-end-file"));

                    if (!LMAPI->send_textfile_expand(u.address,tempbuffer)) {
                        LMAPI->task_heading(u.address);
                        LMAPI->smtp_body_line("The temporary ban that was set on you has been removed.");
                        LMAPI->smtp_body_line("");
                        LMAPI->smtp_body_line("You can now post to the list again.");
                        LMAPI->task_ending();
                    }
                    LMAPI->clean_var("task-form-subject", VAR_TEMP);
                }
            } 
            else {
                char tempbuffer[BIG_BUF];

                LMAPI->buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%s: Temporary ban changed to permanent", LMAPI->get_string("list"));
                LMAPI->set_var("task-form-subject",tempbuffer,VAR_TEMP);

                LMAPI->task_heading(LMAPI->get_string("setflag-user"));
                LMAPI->smtp_body_line("The temporary ban that was set on you has been turned into");
                LMAPI->smtp_body_line("a non-temporary ban.  Contact an  administrator for more information.");
                LMAPI->task_ending();

                LMAPI->clean_var("task-form-subject", VAR_TEMP);             
            }
            free(cookie);
        }
    }
    free(listdir);
    return HOOK_RESULT_OK;
}

COOKIE_HANDLER(destroy_tempban_cookie)
{
    char *user;
    struct list_user u;
    char userfile[BIG_BUF];
    time_t expire, now;
    char *listdir;

    time(&now);

    if(cookietype != 'T') return COOKIE_HANDLE_FAIL;

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

    listdir = LMAPI->list_directory(LMAPI->get_string("list"));
    LMAPI->buffer_printf(userfile, sizeof(userfile) - 1, "%s/users", listdir);

    if(LMAPI->user_find(userfile, user, &u)) {
        if(expire < now) {
            char tempbuffer[BIG_BUF];

            LMAPI->user_unsetflag(&u, "NOPOST", 1);
            LMAPI->user_write(userfile, &u);

            LMAPI->buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%s: Temporary ban expired", LMAPI->get_string("list"));
            LMAPI->set_var("task-form-subject",tempbuffer,VAR_TEMP);

            LMAPI->buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%s/%s",
               listdir, LMAPI->get_string("tempban-end-file"));

            if (!LMAPI->send_textfile_expand(u.address,tempbuffer)) {
               LMAPI->task_heading(u.address);
               LMAPI->smtp_body_line("The temporary ban that was set on you has expired.");
               LMAPI->smtp_body_line("");
               LMAPI->smtp_body_line("You can now post to the list again.");
               LMAPI->task_ending();
            }

            LMAPI->clean_var("task-form-subject", VAR_TEMP);            
        }
    }
    free(listdir);

    return COOKIE_HANDLE_OK;
}
