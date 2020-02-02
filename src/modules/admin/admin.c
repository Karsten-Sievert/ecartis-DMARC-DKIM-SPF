#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "admin-mod.h"

/* Old-style 'admin <list>' command, generates empty wrapper */
CMD_HANDLER(cmd_admin)
{
    struct list_user user;
    char tbuf[BIG_BUF];
    const char *listname;
    char cookie[BIG_BUF];
    char buffer[BIG_BUF];

    /* What's the list? */
    if(params->num == 1)
        listname = params->words[0];
    else
        listname = LMAPI->get_var("list");

    /* Context magic! */
    if (!listname) {
        LMAPI->spit_status("No list in current context.");
        return CMD_RESULT_CONTINUE;
    } else {
        /* Are we valid? */
        if(!LMAPI->list_valid(listname)) {
            LMAPI->nosuch(listname);
            return CMD_RESULT_CONTINUE;
        }

        if(!LMAPI->set_context_list(listname)) {
            LMAPI->spit_status("Unable to switch context to list '%s'.", listname);
            return CMD_RESULT_CONTINUE;
        }
    }

    /* Are we even ON the list? */
    if (!LMAPI->user_find_list(listname,LMAPI->get_string("realsender"),&user)) {
        LMAPI->spit_status("You aren't a member of that list.");
        return CMD_RESULT_CONTINUE;
    }

    /* Are we an admin? */
    if (!LMAPI->user_hasflag(&user,"ADMIN")) {
        LMAPI->spit_status("You are not an administrator for that list.");
        return CMD_RESULT_CONTINUE;
    }

    /* Bake a cookie. */
    LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s!%s", LMAPI->get_string("realsender"),
            LMAPI->get_string("list"));

    LMAPI->listdir_file(tbuf,LMAPI->get_string("list"),"cookies");

    if (!LMAPI->request_cookie(tbuf,&cookie[0], 'A', buffer)) {
        LMAPI->spit_status("Unable to obtain cookie.");
        return CMD_RESULT_CONTINUE;
    }

    /* Log the request */
    LMAPI->log_printf(0,"%s requested authentication for admin mode for list %s\n",
               LMAPI->get_string("realsender"),LMAPI->get_string("list"));

    /* Generate empty wrapper as separate message */
    LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s admin wrapper", SERVICE_NAME_MC);
    LMAPI->set_var("task-form-subject", buffer, VAR_TEMP);
    if(!LMAPI->task_heading(LMAPI->get_string("realsender")))
        return CMD_RESULT_END;
    LMAPI->smtp_body_line("# Below these comment lines, you will find a wrapper for admin");
    LMAPI->smtp_body_text("# commands.  Forward this back to ");
    LMAPI->smtp_body_line(LMAPI->get_string("listserver-address"));
    LMAPI->smtp_body_line("#");
    LMAPI->smtp_body_line("# Put the admin commands you want to use between the adminvfy and");
    LMAPI->smtp_body_line("# adminend commands.");
    LMAPI->smtp_body_line(" ");
    LMAPI->smtp_body_line("// job");
    LMAPI->smtp_body_text("adminvfy ");
    LMAPI->smtp_body_text(LMAPI->get_string("list"));
    LMAPI->smtp_body_text(" ");
    LMAPI->smtp_body_line(cookie);
    LMAPI->smtp_body_line("");
    LMAPI->smtp_body_line("adminend");
    LMAPI->smtp_body_line("// eoj");
    LMAPI->task_ending();

    /* And stick the status in the results message */ 
    LMAPI->spit_status("A cookie-signed wrapper for list '%s' admin mode has been sent to you.", LMAPI->get_string("list"));
    return CMD_RESULT_CONTINUE;
}

/* New-style 'admin2 <list>', fills wrapper with everything between this
   and 'adminend2' */
CMD_HANDLER(cmd_admin2)
{
    struct list_user user;
    char tbuf[BIG_BUF];
    const char *listname;
    FILE *spitfile;
    char cookie[BIG_BUF];
    char buffer[BIG_BUF];

    /* What's the list? */
    if(params->num == 1)
        listname = params->words[0];
    else
        listname = LMAPI->get_var("list");

    /* Context magic */
    if (!listname) {
        LMAPI->spit_status("No list in current context.");
        return CMD_RESULT_CONTINUE;
    } else {
        /* Are we valid? */
        if(!LMAPI->list_valid(listname)) {
            LMAPI->nosuch(listname);
            return CMD_RESULT_CONTINUE;
        }

        /* Set the list context */
        if(!LMAPI->set_context_list(listname)) {
            LMAPI->spit_status("Unable to switch context to list '%s'.", listname);
            return CMD_RESULT_CONTINUE;
        }
    }

    /* Are we on the list? */
    if (!LMAPI->user_find_list(listname,LMAPI->get_string("realsender"),&user)) {
        LMAPI->spit_status("You aren't a member of that list.");
        return CMD_RESULT_CONTINUE;
    }

    /* Are we an admin? */
    if (!LMAPI->user_hasflag(&user,"ADMIN")) {
        LMAPI->spit_status("You are not an administrator for that list.  CANCELING PARSE.");
        return CMD_RESULT_END;
    }

    /* Bake a cookie */
    LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s!%s", LMAPI->get_string("realsender"),
            LMAPI->get_string("list"));

    LMAPI->listdir_file(tbuf,LMAPI->get_string("list"),"cookies");

    if (!LMAPI->request_cookie(tbuf,&cookie[0], 'A', buffer)) {
        LMAPI->spit_status("Unable to obtain cookie.");
        return CMD_RESULT_CONTINUE;
    }

    /* Log the request */
    LMAPI->log_printf(0, "%s requested authentication for admin mode for list %s\n",
               LMAPI->get_string("realsender"),LMAPI->get_string("list"));

    /* Open our temporary buffer */
    LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s.adminspit2", LMAPI->get_string("queuefile"));
    if (!LMAPI->open_adminspit(buffer)) {
       LMAPI->filesys_error(buffer);
       return CMD_RESULT_END;
    }

    /* Setup adminspit2 state - we're 'spitting' text into a file. */
    LMAPI->clean_var("adminspit", VAR_GLOBAL);
    LMAPI->set_var("adminspit2","yes",VAR_GLOBAL);

    spitfile = LMAPI->get_adminspit();

    /* Put the header */
    LMAPI->write_file(spitfile,"# Below these comment lines, you will find a wrapper for admin\n");
    LMAPI->write_file(spitfile,"# commands.  Forward this back to %s\n", LMAPI->get_string("listserver-address"));
    LMAPI->write_file(spitfile,"#\n");
    LMAPI->write_file(spitfile,"# All the commands you specified in the initial request are\n");
    LMAPI->write_file(spitfile,"# already filled out.  You can add more if you choose.\n");
    LMAPI->write_file(spitfile," \n// job\n");
    LMAPI->write_file(spitfile,"adminvfy %s %s\n", LMAPI->get_string("list"), cookie);
 
    /* Spit result into results message */
    LMAPI->spit_status("Beginning admin2 mode, recording commands for wrapper.");
    return CMD_RESULT_CONTINUE;
}

/* adminvfy, actual wrapper begin */
CMD_HANDLER(cmd_admin_verify)
{
    const char *listptr;
    char listname[BIG_BUF];
    char cookie[BIG_BUF];
    char cookiedata[BIG_BUF];
    char tbuf[BIG_BUF];

    memset(listname, 0, sizeof(listname));
    memset(cookie, 0, sizeof(cookie));   

    /* Parameter parsing */
    if(params->num == 2) {
        listptr = params->words[0];
        stringcpy(cookie, params->words[1]);
    } else if(params->num == 1) {
        listptr = LMAPI->get_var("list");
        stringcpy(cookie, params->words[0]);
    } else {
        LMAPI->spit_status("Command only accepts 1 or 2 parameters.");
        return CMD_RESULT_END;
    }

    /* We have to have a list.  Is it valid? */
    if(!LMAPI->list_valid(listptr)) {
        LMAPI->nosuch(listptr);
        return CMD_RESULT_CONTINUE;
    }

    /* Set the context */
    if (!LMAPI->set_context_list(listptr)) {
        LMAPI->spit_status("Unable to switch context to list '%s'.", listptr);
        return CMD_RESULT_END;
    }
    stringcpy(listname, listptr);

    LMAPI->listdir_file(tbuf,listname,"cookies");

    /* Do we have a cookie? */
    if (!LMAPI->verify_cookie(tbuf,cookie,'A', &cookiedata[0])) {
        LMAPI->spit_status("Unable to find cookie or cookie is of wrong type.");
        return CMD_RESULT_END;
    }

    /* Is it our cookie? */
    if (!LMAPI->match_cookie(cookie,LMAPI->get_string("realsender"))) {
        LMAPI->spit_status("Cookie cannot be used from this address.");
        return CMD_RESULT_END;
    }

    /* ...eat the cookie! *munchmunch* */
    LMAPI->del_cookie(tbuf,cookie);

    /* Set adminmode state */
    LMAPI->set_var("adminmode","yes", VAR_GLOBAL);
    LMAPI->spit_status("Successful transition to admin mode.  Cookie cannot be reused.");
    LMAPI->log_printf(0, "%s transitioned to admin mode for list '%s'\n",LMAPI->get_string("realsender"),listname);
    return CMD_RESULT_CONTINUE;
}

/* 'adminend' ends admin command parsing */
CMD_HANDLER(cmd_adminend)
{
    /* Are we IN admin mode? */
    if (!LMAPI->get_bool("adminmode")) {
        LMAPI->spit_status("Not in admin mode, nothing to cancel.");
        return CMD_RESULT_CONTINUE;
    }

    /* We aren't anymore. */
    LMAPI->clean_var("adminmode", VAR_GLOBAL);

    /* Log end of admin mode, restore old state */
    LMAPI->log_printf(0, "%s cancelled admin mode.\n",LMAPI->get_string("realsender"));
    LMAPI->spit_status("Admin command mode ended.  Normal commands still accepted.");
    LMAPI->set_var("fromaddress",LMAPI->get_string("realsender"), VAR_GLOBAL);
    return CMD_RESULT_CONTINUE;
}

/* Admin 'become' command, assume a subscriber identity */
CMD_HANDLER(cmd_adminbecome)
{
    struct list_user user;

    /* Sanity check - are we IN admin mode? */
    if (!LMAPI->get_bool("adminmode")) {
        LMAPI->spit_status("Not in admin mode, can't use BECOME.");
        return CMD_RESULT_END;
    }

    /* Parameter check */
    if (!params->words[0]) {
        if (strcmp(LMAPI->get_string("fromaddress"),LMAPI->get_string("realsender"))) {
            LMAPI->spit_status("Returning to normal identity");
            LMAPI->log_printf(0, "%s resumed own identity.\n",LMAPI->get_string("realsender"));
            LMAPI->set_var("fromaddress",LMAPI->get_string("realsender"), VAR_GLOBAL);
        } else {
            LMAPI->spit_status("No identity assumed, no need to cancel.");
        }
        return CMD_RESULT_CONTINUE;
    } else {
        if(!strcasecmp(params->words[0],LMAPI->get_string("realsender"))) {
            LMAPI->spit_status("Returning to normal identity");
            LMAPI->log_printf(0, "%s resumed own identity.\n",LMAPI->get_string("realsender"));
            LMAPI->set_var("fromaddress",LMAPI->get_string("realsender"), VAR_GLOBAL);
            return CMD_RESULT_CONTINUE;
        } else
            if(!strchr(params->words[0],'.') || !strchr(params->words[0],'@')) {
                LMAPI->spit_status("That doesn't look like a valid address to me.  Command set terminated.");
                return CMD_RESULT_END;
        }

        /* Permission check.  Admins cannot impersonate other admins.
           SUPERADMIN users can setfor/unsetfor/unsubscribe other admins, though. */
        if (LMAPI->user_find_list(LMAPI->get_string("list"),params->words[0],&user)) {
            if (LMAPI->user_hasflag(&user,"ADMIN")) {
                LMAPI->spit_status("Cannot 'become' a fellow admin.  Command set terminated.");
                LMAPI->log_printf(0, "%s attempted to become %s (admin), rejected.\n",
                           LMAPI->get_string("realsender"),params->words[0]);
                return CMD_RESULT_END;
            }
        }

        /* Log the identity change. */
        LMAPI->log_printf(0, "%s assumed identity of %s\n",LMAPI->get_string("realsender"),
                   params->words[0]);
        LMAPI->set_var("fromaddress",params->words[0], VAR_GLOBAL);
        LMAPI->spit_status("Identity assumed.  Commands will be executed as alternative user.");
        return CMD_RESULT_CONTINUE;
    }
}

/* 'setfor' command */
CMD_HANDLER(cmd_adminset)
{
    char foraddy[BIG_BUF];
    char flagname[64];
    char tbuf[BIG_BUF];
    const char *listptr;
    char *tptr;
    int result;
    struct list_user user;

    listptr = LMAPI->get_var("list");

    /* Sanity check */
    if (!LMAPI->get_bool("adminmode")) {
        LMAPI->spit_status("Not in administrator mode.");
        return CMD_RESULT_CONTINUE;
    }

    /* Context check - should NEVER fail here, but... 
       theoretically, a third-party module MIGHT clobber
       the list context while in admin mode, so best to always check. */
    if (!listptr) {
        LMAPI->spit_status("No list in current context - admin commands cannot explicitly give lists.");
        return CMD_RESULT_CONTINUE;
    }

    memset(foraddy, 0, sizeof(foraddy));
    memset(flagname, 0, sizeof(flagname));

    /* Parameter check */
    if(params->num != 2) {
        LMAPI->spit_status("Command requires two parameters.");
        return CMD_RESULT_CONTINUE;
    }

    stringcpy(foraddy, params->words[0]);
    strncpy(flagname, params->words[1], 64);

    /* This probably should go ABOVE the parameter check, but... eh. */
    if(!LMAPI->list_valid(listptr)) {
        LMAPI->nosuch(listptr);
        return CMD_RESULT_CONTINUE;
    }

    LMAPI->listdir_file(tbuf,listptr,"users");

    /* Make sure the user is on the list. */
    if (!LMAPI->user_find(&tbuf[0],&foraddy[0],&user)) {
        LMAPI->spit_status("That user is not a member of that list.");
        return CMD_RESULT_CONTINUE;
    }

    /* Make sure we are allowed to set the flag on them. */
    if (LMAPI->user_hasflag(&user,"ADMIN") && strcmp(user.address,LMAPI->get_string("realsender"))) {
        if (LMAPI->user_find(&tbuf[0],LMAPI->get_string("realsender"),&user)) {
           if (LMAPI->user_hasflag(&user,"SUPERADMIN")) {
              /* User is a superadmin, can change other admins */
              LMAPI->result_printf("Superadmin: changing flag on admin.\n");

              /* Get back our old user */
              LMAPI->user_find(&tbuf[0],&foraddy[0],&user);
           } else {
              LMAPI->spit_status("Cannot change flags on fellow admins.");
              return CMD_RESULT_CONTINUE;
           }
        } else {
           LMAPI->spit_status("Cannot change flags on fellow admins.");
           return CMD_RESULT_CONTINUE;
        }
    }

    tptr = &flagname[0];
    while(*tptr) {
       *tptr = toupper(*tptr);
       tptr++;
    }

    /* Set up the parameters for 'SETFLAG' hooks. */
    LMAPI->set_var("setflag-user",user.address,VAR_TEMP);
    LMAPI->set_var("setflag-flag",flagname,VAR_TEMP);

    /* Call the 'SETFLAG' hooks. */
    if (LMAPI->do_hooks("SETFLAG") == HOOK_RESULT_FAIL) {
        return CMD_RESULT_CONTINUE;
    }

    /* Set the flag */
    if ((result = LMAPI->user_setflag(&user,&flagname[0],1))) {
        switch(result) {
            case ERR_NOSUCHFLAG:
                LMAPI->spit_status("Unrecognized flag.");
                break;
            case ERR_NOTADMIN:
                LMAPI->spit_status("You don't have permissions to set that flag.");
                break;
            case ERR_UNSETTABLE:
                LMAPI->spit_status("Flag is unsettable, even by admin.");
                break;
            case ERR_FLAGSET:
                LMAPI->spit_status("Flag is already set.");
                break;
            case ERR_FLAGNOTSET:
                LMAPI->spit_status("Flag wasn't set to begin with.");
                break;
            default:
                LMAPI->spit_status("Unknown operational error.  This shouldn't happen.");
                break;
        }
        return CMD_RESULT_CONTINUE;
    } else {
        /* Write the user */
        if (LMAPI->user_write(&tbuf[0],&user)) {
            LMAPI->filesys_error(&tbuf[0]);
            return CMD_RESULT_CONTINUE;
        }
        LMAPI->spit_status("Flag successfully set.");
        LMAPI->log_printf(0, "%s set flag %s on %s for list %s\n",
                   LMAPI->get_string("fromaddress"),flagname,foraddy,listptr);
        return CMD_RESULT_CONTINUE;
    }
}

/* SETFLAG hook for moderators */
HOOK_HANDLER(hook_setflag_moderator)
{
    /* Are we setting for a moderator? */
    if (strcasecmp(LMAPI->get_string("setflag-flag"),"MODERATOR") == 0) {
        LMAPI->log_printf(9,"Setting MODERATOR flag, checking for welcome.\n");
        if (LMAPI->get_bool("adminmode")) {
            /* Do we have a moderator welcome file? */
            if (LMAPI->get_var("moderator-welcome-file")) {
                char filename[BIG_BUF];

                LMAPI->listdir_file(filename,LMAPI->get_string("list"),
                     LMAPI->get_string("moderator-welcome-file"));

                /* If it exists, send it. */
                if (LMAPI->exists_file(filename)) {
                    LMAPI->log_printf(9,"Sending MODERATOR welcome.\n");
                    LMAPI->set_var("task-form-subject", "Moderator Introduction", VAR_TEMP);
                    LMAPI->send_textfile(LMAPI->get_string("setflag-user"), filename);
                    LMAPI->clean_var("task-form-subject", VAR_TEMP);
                }
            }
        }
    }

    return HOOK_RESULT_OK;
}

/* 'unsetfor' command */
CMD_HANDLER(cmd_adminunset)
{
    char foraddy[BIG_BUF];
    char flagname[64];
    char tbuf[BIG_BUF];
    const char *listptr;
    char *tptr;
    int result;
    struct list_user user;

    listptr = LMAPI->get_var("list");

    /* Sanity check */
    if (!LMAPI->get_bool("adminmode")) {
        LMAPI->spit_status("Not in administrator mode.");
        return CMD_RESULT_CONTINUE;
    }

    /* We should never NOT have a list context, but just in case...
       another module COULD clobber it. */
    if (!listptr) {
        LMAPI->spit_status("No list in current context - admin commands cannot explicitly give lists.");
        return CMD_RESULT_CONTINUE;
    }

    memset(foraddy, 0, sizeof(foraddy));
    memset(flagname, 0, sizeof(flagname));

    /* Parameter check */
    if(params->num != 2) {
        LMAPI->spit_status("Command requires two parameters.");
        return CMD_RESULT_CONTINUE;
    }

    stringcpy(foraddy, params->words[0]);
    strncpy(flagname, params->words[1], 64);

    /* List validity check */
    if(!LMAPI->list_valid(listptr)) {
        LMAPI->nosuch(listptr);
        return CMD_RESULT_CONTINUE;
    }

    LMAPI->listdir_file(tbuf,listptr,"users");

    /* Are they on the list? */
    if (!LMAPI->user_find(&tbuf[0],&foraddy[0],&user)) {
        LMAPI->spit_status("That user is not a member of that list.");
        return CMD_RESULT_CONTINUE;
    }

    /* Make sure we can set the flag on them. */
    if (LMAPI->user_hasflag(&user,"ADMIN") && strcmp(user.address,LMAPI->get_string("realsender"))) {
        if (LMAPI->user_find(&tbuf[0],LMAPI->get_string("realsender"),&user)) {
           if (LMAPI->user_hasflag(&user,"SUPERADMIN")) {
              /* User is a superadmin, can change other admins */
              LMAPI->result_printf("Superadmin: changing flag on admin.\n");

              /* Get back our old user */
              LMAPI->user_find(&tbuf[0],&foraddy[0],&user);
           } else {
              LMAPI->spit_status("Cannot change flags on fellow admins.");
              return CMD_RESULT_CONTINUE;
           }
        } else {
           LMAPI->spit_status("Cannot change flags on fellow admins.");
           return CMD_RESULT_CONTINUE;
        }
    }

    tptr = &flagname[0];
    while(*tptr) {
        *tptr = toupper(*tptr);
        tptr++;
    }

    /* Setup data for 'UNSETFLAG' hooks. */
    LMAPI->set_var("setflag-user",user.address,VAR_TEMP);
    LMAPI->set_var("setflag-flag",flagname,VAR_TEMP);

    /* Call UNSETFLAG hooks */
    if (LMAPI->do_hooks("UNSETFLAG") == HOOK_RESULT_FAIL) {
        return CMD_RESULT_CONTINUE;
    }

    /* Unset the flag */
    if ((result = LMAPI->user_unsetflag(&user,&flagname[0],1))) {
        switch(result) {
            case ERR_NOSUCHFLAG:
                LMAPI->spit_status("Unrecognized flag.");
                break;
            case ERR_NOTADMIN:
                LMAPI->spit_status("You don't have permissions to set that flag.");
                break;
            case ERR_UNSETTABLE:
                LMAPI->spit_status("Flag is unsettable, even by admin.");
                break;
            case ERR_FLAGSET:
                LMAPI->spit_status("Flag is already set.");
                break;
            case ERR_FLAGNOTSET:
                LMAPI->spit_status("Flag wasn't set to begin with.");
                break;
            default:
                LMAPI->spit_status("Unknown operational error.  This shouldn't happen.");
                break;
        }
        return CMD_RESULT_CONTINUE;
    } else {
        /* Write the user */
        if (LMAPI->user_write(&tbuf[0],&user)) {
            LMAPI->filesys_error(&tbuf[0]);
            return CMD_RESULT_CONTINUE;
        }
        LMAPI->spit_status("Flag successfully unset.");
        LMAPI->log_printf(0, "%s unset flag %s on %s for list %s\n",
                   LMAPI->get_string("fromaddress"),flagname,foraddy,listptr);
        return CMD_RESULT_CONTINUE;
    }
}

/* 'getconf' - retrieves a config file */
CMD_HANDLER(cmd_adminfilereq)
{
    struct list_file *tfile;
    const char *filename;
    char buffer[BIG_BUF];
    char tbuf[BIG_BUF];
    char cookie[BIG_BUF];
    int newfile;
    const char *listptr;
    FILE *infile = NULL;

    /* Security/sanity check */
    if (LMAPI->get_bool("paranoia")) {
        LMAPI->spit_status("'paranoia' mode is on.  No remote file administration is allowed.");
        return CMD_RESULT_CONTINUE;
    }

    newfile = 0;

    /* Sanity check */
    if (!LMAPI->get_bool("adminmode")) {
        LMAPI->spit_status("Not in admin mode.");
        return CMD_RESULT_CONTINUE;
    }

    listptr = LMAPI->get_var("list");

    /* Sanity check */
    if (!listptr) {
        LMAPI->spit_status("No list in current context.");
        return CMD_RESULT_CONTINUE;
    }

    /* Sanity check */
    if (params->num != 1) {
        LMAPI->spit_status("This command requires exactly one parameter.");
        return CMD_RESULT_CONTINUE;
    }

    tfile = LMAPI->find_file(params->words[0]);

    /* Sanity check */
    if (!tfile) {
        LMAPI->spit_status("That doesn't appear to be a valid admin file.");
        return CMD_RESULT_CONTINUE;
    }

    filename = LMAPI->get_var(tfile->varname);

    /* Sanity check */
    if (!filename) {
        LMAPI->spit_status("Internal configuration error, could not complete request.");
        return CMD_RESULT_CONTINUE;
    }

    /* Get a handle to the file */

    LMAPI->listdir_file(buffer,LMAPI->get_string("list"),
      filename);

    if (!LMAPI->exists_file(buffer)) newfile = 1;
    if (!newfile) {
       if ((infile = LMAPI->open_file(buffer,"r")) == NULL) {
           newfile = 1;
       }
    }

    /* Bake a cookie */
    LMAPI->listdir_file(tbuf,listptr,"cookies");
    if (!LMAPI->request_cookie(tbuf,&cookie[0],'F', params->words[0])) {
        LMAPI->spit_status("Unable to obtain cookie.");
        LMAPI->close_file(infile);
        return CMD_RESULT_CONTINUE;
    }

    LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "Config file '%s' for list '%s'", params->words[0], listptr);
    LMAPI->set_var("task-form-subject", buffer, VAR_TEMP);

    /* Generate the file wrapper in a separate message */
    if(!LMAPI->task_heading(LMAPI->get_string("realsender")))
        return CMD_RESULT_END;
    LMAPI->smtp_body_text("# This is the '");
    LMAPI->smtp_body_text(params->words[0]);
    LMAPI->smtp_body_line("' file you requested.");
    LMAPI->smtp_body_line("# ");
    LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "# This file can be edited and sent back to %s.",
            SERVICE_NAME_MC);
    LMAPI->smtp_body_line(buffer);
    LMAPI->smtp_body_line("# ");
    LMAPI->smtp_body_text("# Simply send this back to ");
    LMAPI->smtp_body_line(LMAPI->get_string("listserver-address"));
    LMAPI->smtp_body_line("# with the changes you want.  It must be sent");
    LMAPI->smtp_body_line("# from this address, however.");
    if (newfile) {
        LMAPI->smtp_body_line("# ");
        LMAPI->smtp_body_line("# This file did not already exist, so you will need to put what you want");
        LMAPI->smtp_body_line("# it to contain between the 'putconf' and 'ENDFILE' lines.");
    }
    LMAPI->smtp_body_line("");
    LMAPI->smtp_body_line("// job");
    LMAPI->smtp_body_text("putconf ");
    LMAPI->smtp_body_text(listptr);
    LMAPI->smtp_body_text(" ");
    LMAPI->smtp_body_line(cookie);

    /* If this file already existed, put the existing body in */
    if (!newfile) {
        while(LMAPI->read_file(buffer, sizeof(buffer), infile)) {
            LMAPI->smtp_body_text(buffer);
        }
        LMAPI->close_file(infile);
    }

    LMAPI->smtp_body_line("ENDFILE");
    LMAPI->smtp_body_line("// eoj");
    LMAPI->task_ending();

    /* Log the request */
    LMAPI->listdir_file(buffer,LMAPI->get_string("list"),filename);
    LMAPI->log_printf(0, "%s requested a copy of file %s\n", LMAPI->get_string("realsender"),
               buffer);

    LMAPI->spit_status("File sent w/validation cookie as separate letter.");
    return CMD_RESULT_CONTINUE;
}

/* 'putconf' - command from the pregenerated getconf result wrapper */
CMD_HANDLER(cmd_adminfileput)
{
    struct list_file *tfile;
    const char *listptr;
    char tbuf[BIG_BUF], tbuf2[BIG_BUF];
    char *mytemp;
    char cookiedata[BIG_BUF];

    /*
     * We return END on failure in here so we don't try to parse the file
     * as valid commands!
     */

    /* Sanity check */
    if (LMAPI->get_bool("paranoia")) {
        LMAPI->spit_status("'paranoia' mode is on.  No remote file administration is allowed.");
        return CMD_RESULT_END;
    }

    /* Sanity check */
    if(params->num != 2) {
        LMAPI->spit_status("This command requires two parameters.");
        return CMD_RESULT_END;
    }

    stringcpy(tbuf2, params->words[0]);
    mytemp = params->words[1];

    /* Sanity check */
    if(!LMAPI->list_valid(tbuf2)) {
        LMAPI->nosuch(tbuf2);
        return CMD_RESULT_END;
    }

    /* Set list context */
    if (!LMAPI->set_context_list(&tbuf2[0])) {
        LMAPI->spit_status("Unable to switch context to list '%s'.", tbuf2);
        return CMD_RESULT_END;
    }

    listptr = LMAPI->get_var("list");

    /* Sanity check.  This should NEVER fail. */
    if (!listptr) {
        LMAPI->spit_status("No list in current context.");
        return CMD_RESULT_END;
    }

    /* Find the cookie. */
    LMAPI->listdir_file(tbuf,listptr,"cookies");
    if (!LMAPI->verify_cookie(tbuf,mytemp,'F',&cookiedata[0])) {
        LMAPI->spit_status("Unable to find cookie or cookie is of wrong type.");
        return CMD_RESULT_END;
    }

    /* Is it ours? */
    if (!LMAPI->match_cookie(mytemp,LMAPI->get_string("realsender"))) {
        LMAPI->spit_status("Cookie cannot be used from this address.");
        return CMD_RESULT_END;
    }

    if (!(tfile = LMAPI->find_file(&cookiedata[0]))) {
        LMAPI->spit_status("Cookie is for a nonexistant config file.");
        return CMD_RESULT_END;
    }

    LMAPI->buffer_printf(tbuf2, sizeof(tbuf2) - 1, "Replaced '%s' file", cookiedata);
    LMAPI->set_var("results-subject-override",tbuf2,VAR_TEMP);

    LMAPI->listdir_file(tbuf2,LMAPI->get_string("list"),
      LMAPI->get_string(tfile->varname));

    /* Open a spitfile */
    if (!LMAPI->open_adminspit(tbuf2)) {
        LMAPI->filesys_error(tbuf2);
        return CMD_RESULT_END;
    }

    /* Eat the cookie.  This happens AFTER the spitfile for a
       reason - if the disk is full or something and it fails, 
       it will be able to retry the message on a resend. */
    LMAPI->del_cookie(tbuf,mytemp);

    /* Notify the user the cookie was eaten */
    LMAPI->spit_status("Cookie accepted, file will be replaced.  Cookie is not reusable.");

    LMAPI->log_printf(0, "%s replaced configuration file %s\n",
               LMAPI->get_string("realsender"),&cookiedata[0]);

    return CMD_RESULT_CONTINUE;
}
