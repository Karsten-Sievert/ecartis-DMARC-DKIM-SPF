#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "acctmgr-mod.h"

FUNC_HANDLER(func_hasflag)
{
    char *fromaddy = argv[0];
    char *flag = argv[1];
    struct list_user luser;

    strncpy(result, "0", BIG_BUF - 1);
    if(LMAPI->user_find_list(LMAPI->get_string("list"), fromaddy, &luser)) {
        if(LMAPI->user_hasflag(&luser, flag)) {
            strncpy(result, "1", BIG_BUF - 1);
        }
    }
    return 1;
}

FUNC_HANDLER(func_hasflag_list)
{
    char *fromaddy = argv[0];
    char *flag = argv[1];
    char *list = argv[2];
    struct list_user luser;

    strncpy(result, "0", BIG_BUF - 1);
    if(LMAPI->get_bool("liscript-allow-explicit-list")) {
        if(LMAPI->user_find_list(list, fromaddy, &luser)) {
            if(LMAPI->user_hasflag(&luser, flag)) {
                strncpy(result, "1", BIG_BUF - 1);
            }
        }
    }
    return 1;
}

/* 'setaddy' will map the current user address to a valid subset.
 * For example, user@ix21.ix.netcom.com could do a setaddy to
 * change to user@ix.netcom.com.  Useful for a few busted mailers.
 *
 * This command is somewhat obsolete due to the introduction of the
 * loose-address matching, and should probably either be removed or
 * updated to use the address_match function. */
CMD_HANDLER(cmd_setaddy)
{
    char user1[SMALL_BUF], user2[SMALL_BUF];
    char domain1[SMALL_BUF], domain2[SMALL_BUF];
    char buf1[BIG_BUF], buf2[BIG_BUF];
    char *temp;

    /* Do we allow setaddy? */
    if (!LMAPI->get_bool("allow-setaddy")) {
       LMAPI->spit_status("setaddy is disabled on this box.");
       return CMD_RESULT_CONTINUE;
    }

    /* Sanity check! */
    if (params->num != 1) {
       LMAPI->spit_status("You must provide one (and only one) param to setaddy.");
       return CMD_RESULT_CONTINUE;
    }

    /* Get our working buffer... */
    LMAPI->buffer_printf(buf1, sizeof(buf1) - 1, "%s", LMAPI->get_string("fromaddress"));

    temp = strchr(buf1,'@');

    /* Sanity check! */
    if (!temp) {
       LMAPI->spit_status("Parse error.");
       return CMD_RESULT_CONTINUE;
    }

    *temp = 0;
    LMAPI->buffer_printf(user1, sizeof(user1) - 1, "%s", buf1);
    LMAPI->buffer_printf(domain1, sizeof(domain1) - 1, "%s", temp + 1);

    /* Get our other working buffer */
    LMAPI->buffer_printf(buf2, sizeof(buf2) - 1, "%s", params->words[0]);
    temp = strchr(buf2,'@');

    /* Sanity check again! */
    if (!temp) {
       LMAPI->spit_status("Parse error - not a valid address.");
       return CMD_RESULT_CONTINUE;
    }

    *temp = 0;
    LMAPI->buffer_printf(user2, sizeof(user2) - 1, "%s", buf2);
    LMAPI->buffer_printf(domain2, sizeof(domain2) - 1, "%s", temp + 1);    

    temp = &domain1[0];

    while(*temp) {
       *temp = tolower(*temp);
       temp++;
    }

    temp = &domain2[0];

    while(*temp) {
       *temp = tolower(*temp);
       temp++;
    }

    /* Same user? */
    if(strcasecmp(user1,user2)) {
       LMAPI->spit_status("Username doesn't match.  Will not setaddy.");
       return CMD_RESULT_CONTINUE;
    }

    /* Safety check. <grin>  Hey, anyone who subscribes to a list as 'root'
     * had damn well be able to fix their mailer so they don't have to do
     * something like this. */
    if(!strcasecmp(user2,"root")) {
       LMAPI->spit_status("Sorry, we don't play with root addresses.");
       return CMD_RESULT_CONTINUE;
    }

    /* Domain check...THIS REALLY SHOULD BE BETTER DONE. */
    if(!strstr(domain1,domain2)) {
       LMAPI->spit_status("Domain is not a valid substring.");
       return CMD_RESULT_CONTINUE;
    }

    temp = strchr(domain2,'.');

    /* One more sanity check. */
    if (!temp) {
       LMAPI->spit_status("Domain must contain at least one '.' character.");
       return CMD_RESULT_CONTINUE;
    }

    /* Finish up */
    LMAPI->set_var("fromaddress",params->words[0], VAR_GLOBAL);
    LMAPI->spit_status("Address reset.");
    return CMD_RESULT_CONTINUE;
}

/* 'set' - sets a flag on an account */
CMD_HANDLER(cmd_set)
{
    char listname[BIG_BUF];
    char flagname[64];
    char tbuf[BIG_BUF];
    const char *listptr;
    char *tptr;
    int result;
    struct list_user user;
    char *listdir;

    memset(listname, 0, sizeof(listname));
    memset(flagname, 0, sizeof(flagname));

    /* Sanity check */
    if(params->num > 2) {
        LMAPI->spit_status("Too many parameters to command.  2 expected.");
        return CMD_RESULT_CONTINUE;
    }

    /* Sort out our parameters */
    if(params->num == 2) {
        /* Sanity check */
        if(LMAPI->get_bool("adminmode")) {
            LMAPI->spit_status("Cannot provide a list parameter in this context.");
            return CMD_RESULT_CONTINUE;
        }
        listptr = params->words[0];
        strncpy(flagname, params->words[1], 64);
    } else {
        listptr = LMAPI->get_var("list");
        strncpy(flagname, params->words[0], 64);
    }

    /* Sanity check */
    if (!listptr) {
        LMAPI->spit_status("No list in current context, can't perform command.");
        return CMD_RESULT_CONTINUE;
    }

    /* Sanity check */
    if(!LMAPI->list_valid(listptr)) {
        LMAPI->nosuch(listptr);
        return CMD_RESULT_CONTINUE;
    }

    /* Set list context */
    if(!LMAPI->set_context_list(listptr)) {
       LMAPI->spit_status("Unable to switch context to list '%s'.", listptr);
       return CMD_RESULT_END;
    }

    listdir = LMAPI->list_directory(listptr);
    LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "%s/users", listdir);
    free(listdir);

    /* Only subscribed users can set flags on their subscriptions. */
    if (!LMAPI->user_find(&tbuf[0],LMAPI->get_string("fromaddress"),&user)) {
        LMAPI->spit_status("You aren't a member of that list.");
        return CMD_RESULT_CONTINUE;
    }

    tptr = &flagname[0];
    while(*tptr) {
        *tptr = toupper(*tptr);
        tptr++;
    }

    /* Set up our SETFLAG hook variables */
    LMAPI->set_var("setflag-user",user.address,VAR_TEMP);
    LMAPI->set_var("setflag-flag",flagname,VAR_TEMP);

    /* Call SETFLAG hooks.  These allow things like sending a note to a moderator
     * when they're set MODERATOR, or disallowing DIGEST on lists with no-digest = yes */
    if (LMAPI->do_hooks("SETFLAG") == HOOK_RESULT_FAIL) {
        /* Rejected?  Move along.  Up to the hook to display a result message. */
        return CMD_RESULT_CONTINUE;
    }

    /*
     *  Reread the user.  It *is* possible that they could have been modified
     * by the proceeding operation
     */
    LMAPI->user_find(&tbuf[0],LMAPI->get_string("fromaddress"),&user);

    /* Set flag */
    if ((result = LMAPI->user_setflag(&user,&flagname[0],0))) {
        /* Did we puke?  Return a result */
        switch(result) {
            case ERR_NOSUCHFLAG:
                LMAPI->spit_status("Unrecognized flag.");
                break;
            case ERR_NOTADMIN:
                LMAPI->spit_status("You don't have permissions to set that flag.");
                break;
            case ERR_UNSETTABLE:
                LMAPI->spit_status("Unsettable flag.");
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
        /* Rewrite the user file */
        if (LMAPI->user_write(&tbuf[0],&user)) {
            LMAPI->filesys_error(&tbuf[0]);
            return CMD_RESULT_CONTINUE;
        }
        /* Status and log */
        LMAPI->spit_status("Flag successfully set.");
        LMAPI->log_printf(1, "%s set flag %s for list %s\n",
                   LMAPI->get_string("fromaddress"),flagname,listptr);
        return CMD_RESULT_CONTINUE;
    }
}

/* 'unset' - opposite of 'set', unsets a flag on a list subscription */
CMD_HANDLER(cmd_unset)
{
    char listname[BIG_BUF];
    char flagname[64];
    char tbuf[BIG_BUF];
    const char *listptr;
    char *tptr;
    int result;
    struct list_user user;
    char *listdir;

    memset(listname, 0, sizeof(listname));
    memset(flagname, 0, sizeof(flagname));

    /* Sanity check */
    if(params->num > 2) {
        LMAPI->spit_status("Too many parameters to command.  2 expected.");
        return CMD_RESULT_CONTINUE;
    }

    /* Sort out our parameters */
    if(params->num == 2) {
        /* Sanity check */
        if(LMAPI->get_bool("adminmode")) {
            LMAPI->spit_status("Cannot provide a list parameter in this context.");
            return CMD_RESULT_CONTINUE;
        }
        listptr = params->words[0];
        strncpy(flagname, params->words[1], 64);
    } else {
        listptr = LMAPI->get_var("list");
        strncpy(flagname, params->words[0], 64);
    }

    /* Another sanity check */
    if (!listptr) {
        LMAPI->spit_status("No list in current context, can't perform command.");
        return CMD_RESULT_CONTINUE;
    }

    /* Another sanity check */
    if(!LMAPI->list_valid(listptr)) {
        LMAPI->nosuch(listptr);
        return CMD_RESULT_CONTINUE;
    }

    /* Set list context */
    if(!LMAPI->set_context_list(listptr)) {
       LMAPI->spit_status("Unable to switch context to list '%s'.", listptr);
       return CMD_RESULT_END;
    }

    listdir = LMAPI->list_directory(listptr);
    LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "%s/users", listdir);
    free(listdir);

    /* Make sure the user is subscribed to the list */
    if (!LMAPI->user_find(&tbuf[0],LMAPI->get_string("fromaddress"),&user)) {
        LMAPI->spit_status("You aren't a member of that list.");
        return CMD_RESULT_CONTINUE;
    }

    tptr = &flagname[0];
    while(*tptr) {
        *tptr = toupper(*tptr);
        tptr++;
    }

    /* Set up UNSETFLAG hook data */
    LMAPI->set_var("setflag-user",user.address,VAR_TEMP);
    LMAPI->set_var("setflag-flag",flagname,VAR_TEMP);

    /* Call UNSETFLAG hooks to allow a chance to reject or call other data */
    if (LMAPI->do_hooks("UNSETFLAG") == HOOK_RESULT_FAIL) {
        return CMD_RESULT_CONTINUE;
    }

    /*
     *  Reread the user.  It *is* possible that they could have been modified
     * by the proceeding operation
     */
    LMAPI->user_find(&tbuf[0],LMAPI->get_string("fromaddress"),&user);

    /* Unset the flag */
    if ((result = LMAPI->user_unsetflag(&user,&flagname[0],0))) {
        /* Did we puke on that?  Return a result */
        switch(result) {
            case ERR_NOSUCHFLAG:
                LMAPI->spit_status("Unrecognized flag.");
                break;
            case ERR_NOTADMIN:
                LMAPI->spit_status("You don't have permissions to unset that flag.");
                break;
            case ERR_UNSETTABLE:
                LMAPI->spit_status("Unsettable flag.");
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
        /* Rewrite the user file */
        if (LMAPI->user_write(&tbuf[0],&user)) {
            LMAPI->filesys_error(&tbuf[0]);
            return CMD_RESULT_CONTINUE;
        }
        /* Finish up */
        LMAPI->spit_status("Flag successfully unset.");
        LMAPI->log_printf(0, "%s unset flag %s for list %s\n",
                   LMAPI->get_string("fromaddress"),flagname,listptr);
        return CMD_RESULT_CONTINUE;
    }
}
