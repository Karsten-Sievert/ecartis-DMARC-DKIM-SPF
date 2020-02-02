#include "config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "acctmgr-mod.h"

FUNC_HANDLER(func_subscribed)
{
    char *fromaddy = argv[0];
    struct list_user luser;

    strncpy(result, "0", BIG_BUF - 1);
    if(LMAPI->user_find_list(LMAPI->get_string("list"), fromaddy, &luser)) {
        strncpy(result, "1", BIG_BUF - 1);
    }
    return 1;
}

FUNC_HANDLER(func_subscribed_list)
{
    char *fromaddy = argv[0];
    char *list = argv[1];
    struct list_user luser;

    strncpy(result, "0", BIG_BUF - 1);
    if(LMAPI->get_bool("liscript-allow-explicit-list")) {
        if(LMAPI->user_find_list(list, fromaddy, &luser)) {
            strncpy(result, "1", BIG_BUF - 1);
        }
    }
    return 1;
}


/* PRESUB hook to check if user is blacklisted */
HOOK_HANDLER(hook_presub_blacklist)
{
    const char *fromaddy, *listname;
    char userfilepath[BIG_BUF];
    const char *blacklistfile;
    int usedefault;

    /* Get our working data */
    fromaddy = LMAPI->get_string("subscribe-me");
    listname = LMAPI->get_string("list");

    /* Are we blacklisted?  (API functions are your friend!) */
    if (LMAPI->blacklisted(fromaddy)) {
        /* If we're in admin mode, return a failure note... */
        if (LMAPI->get_bool("adminmode")) {
            LMAPI->spit_status("User fails blacklist check.");
            return HOOK_RESULT_FAIL;
        } else {
            char *listdir;

            listdir = LMAPI->list_directory(listname);
            /* ...otherwise log it... */
            LMAPI->log_printf(0, "Blacklisted address %s attempted to subscribe to %s\n",
                       fromaddy, listname);
            blacklistfile = LMAPI->get_var("blacklist-reject-file");

            /* ...send either the default or the actual blacklist text... */
            LMAPI->buffer_printf(userfilepath, sizeof(userfilepath) - 1, "%s/%s", listdir, blacklistfile);
            free(listdir);
            LMAPI->set_var("task-form-subject","You have been blacklisted.",VAR_TEMP);
            usedefault = !LMAPI->send_textfile_expand(fromaddy,userfilepath);

            if (usedefault) {
                LMAPI->spit_status("Request denied.");
                LMAPI->result_printf("The address you are trying to subscribe from has been banned from\n");
                LMAPI->result_printf("this mailing list.  Your request has been rejected.\n");
            } else {
                LMAPI->spit_status("Request denied: user is blacklisted.");
            }

            /* ...and reject. */
            return HOOK_RESULT_FAIL;
        }
        return HOOK_RESULT_FAIL;
    }
    return HOOK_RESULT_OK;
}

/* PRESUB hook to check if user is matches an ACL and so can subscribe */
HOOK_HANDLER(hook_presub_acl)
{
    const char *fromaddy;
    FILE *aclfile;
    char filename[BIG_BUF];
    char buffer[BIG_BUF];
    int allowed;

    if (!LMAPI->get_bool("subscription-acl")) return HOOK_RESULT_OK;
    /* Admins can override the ACL, so if we are in admin mode, bail */
    if (LMAPI->get_bool("adminmode")) return HOOK_RESULT_OK;

    fromaddy = LMAPI->get_string("subscribe-me");
    LMAPI->listdir_file(filename,LMAPI->get_string("list"),
                        LMAPI->get_string("subscribe-acl-file"));
    
    if (!LMAPI->exists_file(filename)) return HOOK_RESULT_OK;
    
    if ((aclfile = LMAPI->open_file(filename,"r")) == NULL) {
       LMAPI->filesys_error(filename);
       return HOOK_RESULT_OK;
    }

    allowed = 0;

    while(LMAPI->read_file(buffer, sizeof(buffer), aclfile) && !allowed) {
       if (buffer[strlen(buffer) - 1] == '\n')
          buffer[strlen(buffer) - 1] = 0;

       LMAPI->log_printf(9,"Comparing '%s' against '%s'...\n",
          fromaddy, buffer);

       if (LMAPI->match_reg(buffer,fromaddy)) allowed = 1;
    }

    LMAPI->close_file(aclfile);

    if (!allowed) {
       LMAPI->set_var("task-form-subject","Subscription authorization failed.", VAR_TEMP);
       LMAPI->listdir_file(filename,LMAPI->get_string("list"),
              LMAPI->get_string("subscribe-acl-text-file"));
       if (LMAPI->exists_file(filename)) {
          LMAPI->send_textfile_expand(fromaddy,filename);
       }
       LMAPI->clean_var("task-form-subject",VAR_TEMP);
       LMAPI->spit_status("Authorization failed; your address is not in the allowed list.");
       return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;
}

/* PRESUB hook to check if user is already subscribed */
HOOK_HANDLER(hook_presub_subscribed)
{
    const char *fromaddy, *listname;
    struct list_user user;

    /* Get our PRESUB data */
    fromaddy = LMAPI->get_string("subscribe-me");
    listname = LMAPI->get_string("list");

    /* Sanity check! */
    if(!LMAPI->list_valid(listname)) {
        LMAPI->nosuch(listname);
        return HOOK_RESULT_FAIL;
    }

    /* Are we already subscribed? */
    if (LMAPI->user_find_list(listname,fromaddy,&user)) {
        if (LMAPI->get_bool("silent-resubscribe")) {
            /* no warning if silent-resubscribe is set */
            LMAPI->log_printf(9, "warning silenced by silent-resubscribe\n");
        } else
        if (LMAPI->get_bool("adminmode")) {
        /* If we're in admin mode, report that fact... */
            LMAPI->spit_status("User already belongs to that list.");
            return HOOK_RESULT_FAIL;
        } else {
            /* ...otherwise send a note to the user. */
            LMAPI->spit_status("'Subscribe' request denied.");
            LMAPI->result_printf("Your request was rejected for the following reason:\n\n");
            LMAPI->result_printf("You are already on the list '%s'.\n",listname);
            if (LMAPI->user_hasflag(&user,"VACATION")) {
                char userfilepath[BIG_BUF];
                char *listdir;

                listdir = LMAPI->list_directory(listname);
                LMAPI->buffer_printf(userfilepath, sizeof(userfilepath) - 1, "%s/users", listdir);
                free(listdir);
                LMAPI->result_printf("\nYour account was set 'VACATION' (e.g. do not receive mail\n");
                LMAPI->result_printf("and has been reset to normal.\n");
                LMAPI->user_unsetflag(&user,"VACATION",1);
                LMAPI->user_write(&userfilepath[0],&user);
            }
        }
        return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;
}

/* PRESUB hook to check if the list is closed */
HOOK_HANDLER(hook_presub_closed)
{
    const char *subscribemode;
    const char *fromaddy, *listname;
    char buf[BIG_BUF];

    /* If we're in admin mode, we don't check */
    if (LMAPI->get_bool("adminmode"))
        return HOOK_RESULT_OK;

    /* Get our data */
    subscribemode = LMAPI->get_var("subscribe-mode");
    fromaddy = LMAPI->get_string("subscribe-me");
    listname = LMAPI->get_string("list");

    /* Check if we're closed */
    if (strcasecmp(subscribemode,"closed") == 0) {
       const char *adminaddy;
       char cookie[BIG_BUF], cookiefile[SMALL_BUF];
       FILE *infile;
       char tempfilename[SMALL_BUF];
       char addybuf[BIG_BUF];
       char cmdbuf[BIG_BUF];
       char *cmdptr;
       char *listdir;

       adminaddy = LMAPI->get_var("administrivia-address");
       if (!adminaddy) adminaddy = LMAPI->get_string("list-owner");

       listdir = LMAPI->list_directory(LMAPI->get_string("list"));
       LMAPI->buffer_printf(cookiefile, sizeof(cookiefile) - 1, "%s/cookies", listdir);
       free(listdir);

       LMAPI->set_var("cookie-for", LMAPI->get_string("list"), VAR_TEMP);

       if(LMAPI->get_var("submodes-mode")) {
           LMAPI->buffer_printf(addybuf, sizeof(addybuf) - 1, "@%s@%s", 
                                LMAPI->get_string("submodes-mode"), fromaddy);
       } else {
           LMAPI->buffer_printf(addybuf, sizeof(addybuf) - 1, "%s", fromaddy);
       }

       /* Generate cookie for appsub */
       if (!LMAPI->request_cookie(cookiefile,&cookie[0], 'S', addybuf)) {
           LMAPI->spit_status("Unable to generate subscription cookie!");
           LMAPI->filesys_error(cookiefile);
           return HOOK_RESULT_FAIL;
       }
       LMAPI->clean_var("cookie-for", VAR_TEMP);

       /* Send cookie to administrator */
       
       LMAPI->buffer_printf(buf, sizeof(buf) - 1, "Subscription request for '%s'", listname);
       LMAPI->set_var("task-form-subject", buf, VAR_TEMP);

       if(!LMAPI->task_heading(adminaddy))
           return HOOK_RESULT_FAIL;
       LMAPI->smtp_body_text("# Subscription request received from ");
       LMAPI->smtp_body_line(LMAPI->get_string("fromaddress"));
       LMAPI->smtp_body_text("# for the address ");
       LMAPI->smtp_body_text(fromaddy);
       LMAPI->smtp_body_text(" and list ");
       LMAPI->smtp_body_line(LMAPI->get_string("list"));
       if (LMAPI->get_var("submodes-mode")) { 
           LMAPI->smtp_body_line("# ");
           LMAPI->smtp_body_text("# Subscriber is in '");
           LMAPI->smtp_body_text(LMAPI->get_string("submodes-mode"));
           LMAPI->smtp_body_line("' mode.");
       }
       LMAPI->smtp_body_line("# ");
       LMAPI->smtp_body_text("# To approve this, reply to this to ");
       LMAPI->smtp_body_text(LMAPI->get_string("listserver-address"));
       LMAPI->smtp_body_line(":");
       LMAPI->smtp_body_line("// job");

       LMAPI->buffer_printf(cmdbuf, sizeof(cmdbuf) - 1, "appsub %s %s %s",
         LMAPI->get_string("list"), fromaddy, cookie);

       cmdptr = NULL;

       if (strlen(cmdbuf) > 60) {
          cmdptr = strrchr(cmdbuf,' ');
          *cmdptr++ = 0;
       }

       LMAPI->smtp_body_text(cmdbuf);
       if (cmdptr) {
          LMAPI->smtp_body_line(" \\");
          LMAPI->smtp_body_text(cmdptr);
       }
       LMAPI->smtp_body_line("");
       LMAPI->smtp_body_line("// eoj");

       /* ...and include the request, in case */
       if (LMAPI->get_bool("administrivia-include-requests")) {
          char buffer[BIG_BUF];

          if ((infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) == NULL) {
             LMAPI->log_printf(1,"Acctmgr unable to open queuefile to attach.\n");
          } else {
             LMAPI->smtp_body_line("\n-- Original Message --");

             while(LMAPI->read_file(buffer, sizeof(buffer), infile)) {
                LMAPI->smtp_body_text(buffer);
             }
             LMAPI->close_file(infile);
          }
       }

       LMAPI->task_ending();

       /* Spit a result code back */
       LMAPI->spit_status("List is closed-subscription, request has been forwarded to list admins.");

       listdir = LMAPI->list_directory(LMAPI->get_string("list"));

       LMAPI->buffer_printf(tempfilename, sizeof(tempfilename) - 1, "%s/%s", listdir,
         LMAPI->get_string("closed-subscribe-file"));
       free(listdir);

       if ((infile = LMAPI->open_file(tempfilename,"r")) != NULL) {
          char inputbuffer[BIG_BUF];

          LMAPI->result_printf("---\n");

          while(LMAPI->read_file(inputbuffer, sizeof(inputbuffer), infile)) {
             LMAPI->result_printf("%s",inputbuffer);
          }
          LMAPI->close_file(infile);

          LMAPI->result_printf("---\n");
       }

       return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;
}

/* PRESUB hook to check if list is subscribe-mode confirm */
HOOK_HANDLER(hook_presub_confirm)
{
    const char *subscribemode;
    const char *fromaddy, *listname;

    /* We ignore this if it's an admin-mode subscription force */
    if (LMAPI->get_bool("adminmode"))
        return HOOK_RESULT_OK;

    /* Get our data */
    subscribemode = LMAPI->get_var("subscribe-mode");
    fromaddy = LMAPI->get_string("subscribe-me");
    listname = LMAPI->get_string("list");

    /* Check if the confirm mode is set, OR if the address 
     * doesn't match where the request came from. */
    if ((strcasecmp(subscribemode,"confirm") == 0) ||
       (strcasecmp(fromaddy,LMAPI->get_string("fromaddress")) && 
       (!LMAPI->get_bool("adminmode") &&
		(strcasecmp("open-auto",subscribemode) != 0)))) {
       const char *adminaddy;
       const char *sendas;
       char cookie[BIG_BUF], cookiefile[BIG_BUF];  /* Changed cookiefile from SMALL_BUF to BIG_BUF due to listdir_file */
       char addybuf[BIG_BUF];
       char cmdbuf[BIG_BUF];
       char *cmdptr;
       int subscribe_confirm_file_included = 0;

       adminaddy = LMAPI->get_var("administrivia-address");
       if (!adminaddy) adminaddy = LMAPI->get_string("list-owner");

       LMAPI->listdir_file(cookiefile, LMAPI->get_string("list"), "cookies");

       LMAPI->set_var("cookie-for", LMAPI->get_string("list"), VAR_TEMP);

       if(LMAPI->get_var("submodes-mode")) {
           LMAPI->buffer_printf(addybuf, sizeof(addybuf) - 1, "@%s@%s", 
                                LMAPI->get_string("submodes-mode"), fromaddy);
       } else {
           LMAPI->buffer_printf(addybuf, sizeof(addybuf) - 1, "%s", fromaddy);
       }

       /* Request a cookie for appsub */
       if (!LMAPI->request_cookie(cookiefile,&cookie[0], 'S', addybuf)) {
           LMAPI->spit_status("Unable to generate subscription cookie!");
           LMAPI->filesys_error(cookiefile);
           return HOOK_RESULT_FAIL;
       }
       LMAPI->clean_var("cookie-for", VAR_TEMP);

       /* Add a confirmation note and send the ticket */
       LMAPI->spit_status("Subscription confirmation ticket sent to user being subscribed.");
       sendas = LMAPI->get_var("send-as");
       if (!sendas) sendas = LMAPI->get_var("list-owner");
       if (!sendas) sendas = LMAPI->get_var("listserver-address");

       LMAPI->set_var("form-send-as", sendas, VAR_TEMP);
       LMAPI->set_var("form-reply-to",
			   LMAPI->get_string("listserver-address"), VAR_TEMP);
       LMAPI->set_var("task-form-subject",
			   LMAPI->get_string("subscribe-confirm-subject"), VAR_TEMP);

       if(!LMAPI->task_heading(fromaddy))
           return HOOK_RESULT_FAIL;

       if (LMAPI->get_var("submodes-mode")) {
           struct submode *subdata;
		   
		   subdata = LMAPI->get_submode(LMAPI->get_string("submodes-mode"));
           LMAPI->smtp_body_line("# ");
           LMAPI->smtp_body_text("# This subscription is requested in '");
           LMAPI->smtp_body_text(subdata->modename);
           LMAPI->smtp_body_line("' mode.");
           LMAPI->smtp_body_line("# This mode is described as: ");
           LMAPI->smtp_body_text("# ");
           LMAPI->smtp_body_line(subdata->description);
       }

		/* add file from subscribe-confirm-file */
		if (LMAPI->get_var("subscribe-confirm-file")) {
       		FILE *infile;
       		char tempfilename[BIG_BUF]; /* Changed from SMALL_BUF to BIG_BUF due to listdir_file */

       		LMAPI->listdir_file(tempfilename, LMAPI->get_string("list"),
					LMAPI->get_string("subscribe-confirm-file"));

       		if ((infile = LMAPI->open_file(tempfilename,"r")) != NULL) {
          		char inputbuffer[BIG_BUF];
          		char linebuffer[BIG_BUF];

				LMAPI->smtp_body_line("# ");
          		while(LMAPI->read_file(inputbuffer, sizeof(inputbuffer), infile)) {
             		LMAPI->buffer_printf(linebuffer, sizeof(linebuffer) - 1, "# %s",
							inputbuffer);
					LMAPI->smtp_body_text(linebuffer);
          		}
				LMAPI->smtp_body_line("# ");
          		LMAPI->close_file(infile);

                subscribe_confirm_file_included = 1;
       		}
		} /* end if subscribe-confirm-file */

		if( ! subscribe_confirm_file_included )
		{
			/*
			 * no subscribe-confirm-file has been included in the mail,
			 * let's print some defaults
			 */

	       LMAPI->smtp_body_text("# ");
	       LMAPI->smtp_body_text(LMAPI->get_string("fromaddress"));
	       LMAPI->smtp_body_line(" has requested that you be subscribed");
	       LMAPI->smtp_body_text(LMAPI->get_string("list"));
	       LMAPI->smtp_body_line(" mailing list.  ");
	       LMAPI->smtp_body_line("# ");
	       LMAPI->smtp_body_line("# To subscribe, reply to this message, leaving the message body");
	       LMAPI->smtp_body_line("# intact, or send the following lines in e-mail to");
	       LMAPI->smtp_body_text("# ");
	       LMAPI->smtp_body_text(LMAPI->get_string("listserver-address"));
	       LMAPI->smtp_body_line(":\n");
		}
       LMAPI->smtp_body_line("// job");

       LMAPI->buffer_printf(cmdbuf, sizeof(cmdbuf) - 1, "appsub %s %s %s",
         LMAPI->get_string("list"), fromaddy, cookie);
          
       cmdptr = NULL;
        
       if (strlen(cmdbuf) > 60) {
          cmdptr = strrchr(cmdbuf,' ');
          *cmdptr++ = 0;
       }
          
       LMAPI->smtp_body_text(cmdbuf);
       if (cmdptr) {
          LMAPI->smtp_body_line(" \\");
          LMAPI->smtp_body_text(cmdptr);
       }
       LMAPI->smtp_body_line("");

       LMAPI->smtp_body_line("// eoj");
       LMAPI->task_ending();
       LMAPI->clean_var("form-send-as", VAR_TEMP);
       LMAPI->clean_var("form-reply-to", VAR_TEMP);
       LMAPI->clean_var("task-form-subject", VAR_TEMP);

       /* set, eat the PERsonal Results output for this session so far. */
       if (LMAPI->get_bool("prevent-second-message")) {
         char resultfile[BIG_BUF];

         LMAPI->buffer_printf(resultfile, sizeof(resultfile) - 1,
           "%s.perr", LMAPI->get_string("queuefile"));

         LMAPI->unlink_file(resultfile);
    }
         return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;
}

/* POSTSUB hook to send the welcome message */
HOOK_HANDLER(hook_postsub_welcome)
{
    char outbuf[BIG_BUF];
    const char *fromaddy, *listname;
    char *listdir;
    const char *adminnotice;

    /* If ADMIN, check to see if the message should be sent */
    if (LMAPI->get_bool("adminmode")) {
        if (LMAPI->get_bool("admin-silent-subscribe"))
            return HOOK_RESULT_OK;

        adminnotice = LMAPI->get_var("admin-subscribe-notice");
        if ((strcasecmp(adminnotice, "silent") == 0) ||
            (strcasecmp(adminnotice, "notify") == 0))
            return HOOK_RESULT_OK;
    }

    /* Get our data */
    fromaddy = LMAPI->get_string("subscribe-me");
    listname = LMAPI->get_string("list");

    /* Send the textfile.  Easy! */
    listdir = LMAPI->list_directory(listname);
    LMAPI->buffer_printf(outbuf, sizeof(outbuf) - 1, "%s/%s", listdir, LMAPI->get_string("welcome-file"));
    free(listdir);
    LMAPI->set_var("task-form-subject",
    	LMAPI->get_string("welcome-subject"), VAR_TEMP);
    LMAPI->send_textfile_expand(fromaddy,outbuf);
    LMAPI->clean_var("task-form-subject", VAR_TEMP);

    /* If the welcome message existed and we have prevent-second-message
     * set, eat the PERsonal Results output for this session so far. */
    if (LMAPI->exists_file(outbuf) && LMAPI->get_bool("prevent-second-message")) {
       char resultfile[BIG_BUF];

       LMAPI->buffer_printf(resultfile, sizeof(resultfile) - 1,
            "%s.perr", LMAPI->get_string("queuefile"));

       LMAPI->unlink_file(resultfile);
    }

    return HOOK_RESULT_OK;
}

/* POSTSUB hook to generate administrivia */
HOOK_HANDLER(hook_postsub_administrivia)
{
    const char *fromaddy, *listname;

    /* Get our data */
    fromaddy = LMAPI->get_string("subscribe-me");
    listname = LMAPI->get_string("list");

    /* Make sure we should send this */
    if(!LMAPI->get_bool("no-administrivia")) {
        const char *toaddy = NULL;
        
        toaddy = LMAPI->get_var("administrivia-address");
        if (!toaddy && LMAPI->get_bool("owner-fallback")) 
           toaddy = LMAPI->get_var("list-owner");

        /* Make sure we have an address to send to */
        if(toaddy) {
            char subject[SMALL_BUF];

            LMAPI->buffer_printf(subject, sizeof(subject) - 1, "%s subscribed to %s",
               fromaddy, listname);

            /* Build and send the report */
            LMAPI->set_var("task-form-subject",subject,VAR_TEMP);
            if(!LMAPI->task_heading(toaddy))
                return HOOK_RESULT_FAIL;
            LMAPI->smtp_body_text(fromaddy);
            LMAPI->smtp_body_text(" subscribed to list ");
            LMAPI->smtp_body_line(listname);
            if (LMAPI->get_var("submodes-mode")) {
                LMAPI->smtp_body_text("Subscription is in '");
                LMAPI->smtp_body_text(LMAPI->get_string("submodes-mode"));
                LMAPI->smtp_body_line("' mode.");
            }
            LMAPI->smtp_body_text("Command came from: ");
            LMAPI->smtp_body_line(LMAPI->get_string("realsender"));
            if (LMAPI->get_bool("administrivia-include-requests")) {
               FILE *infile;

               if ((infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) != NULL) {
                  char inbuffer[BIG_BUF];

                  LMAPI->smtp_body_line("\n-- Original Request --");
                  while (LMAPI->read_file(inbuffer, sizeof(inbuffer), infile)) {
                     LMAPI->smtp_body_text(inbuffer);
                  }
                  LMAPI->close_file(infile);
               }
            }
            LMAPI->task_ending();
        }
    }

    return HOOK_RESULT_OK;
}

/* Subscribe command */
CMD_HANDLER(cmd_subscribe)
{
    const char *listname = NULL;
    char userfilepath[BIG_BUF];
    const char *fromaddy;
    char *listdir;
    const char *adminnotice;

    /* Get our starting data */
    fromaddy = LMAPI->get_string("fromaddress");
    listname = LMAPI->get_var("list");

    /* Mode check */
    if(params->num == 1) {
        /* Make sure it's an address if we're in admin mode */
        if(LMAPI->get_bool("adminmode")) {
            if(strchr(params->words[0],'@')) {
               fromaddy = params->words[0];
            } else {
               LMAPI->spit_status("Cannot provide a list in this context.");
               return CMD_RESULT_CONTINUE;
            }
        } else {
            if(strchr(params->words[0],'@')) {
              fromaddy = params->words[0];
            } else {
              listname = params->words[0];
            }
        }
    } else if (params->num == 2) {
        if (LMAPI->get_bool("adminmode")) {
           LMAPI->spit_status("Cannot provide a list in this context.");
           return CMD_RESULT_CONTINUE;
        }
        listname = params->words[0];
        fromaddy = params->words[1];
    }

    /* Sanity check */
    if(listname == NULL) {
        LMAPI->spit_status("Does not appear to be a valid subscribe command.  No list in present context.  To see the lists available on this machine, send %s the command 'lists'.", SERVICE_NAME_MC);
        return CMD_RESULT_CONTINUE;
    }

    if(!LMAPI->set_context_list(listname)) {
       LMAPI->nosuch(listname);
       /* Return CMD_RESULT_END cause otherwise the list context
        * might be incorrect for further changes
        */
        return CMD_RESULT_END;
    }

    /* Sanity check */
    if(!strchr(fromaddy,'@') || !strchr(fromaddy,'@')) {
       LMAPI->spit_status("That doesn't appear to be a valid e-mail address.");
       return CMD_RESULT_CONTINUE;
    }

    /* Set up our data for PRESUB hooks */
    /* It must be VAR_GLOBAL to prevent the set_context_list
       calls that might get done from wiping it. */
    LMAPI->set_var("subscribe-me",fromaddy,VAR_GLOBAL);

    /* Call PRESUB hooks */
    if (LMAPI->do_hooks("PRESUB") == HOOK_RESULT_FAIL) {
        /* If any reject, cancel and continue on */
        return CMD_RESULT_CONTINUE;
    }

    listdir = LMAPI->list_directory(LMAPI->get_string("list"));
    LMAPI->buffer_printf(userfilepath, sizeof(userfilepath) - 1, "%s/users", listdir);
    free(listdir);

    /* Add user... */
    if (!LMAPI->user_add(userfilepath,fromaddy)) {
        char outbuf[BIG_BUF];

        /* Log */
        LMAPI->log_printf(0, "%s subscribed to %s\n",fromaddy,listname);

        /* Send notification */
        if (LMAPI->get_var("adminmode") || strcmp(fromaddy,LMAPI->get_string("fromaddress"))) {
            LMAPI->spit_status("Successfully subscribed.");
            adminnotice = LMAPI->get_var("admin-subscribe-notice");

            if (!LMAPI->get_bool("adminmode") ||
                (!LMAPI->get_bool("admin-silent-subscribe") &&
                 !(strcasecmp(adminnotice, "silent") == 0) &&
                 !(strcasecmp(adminnotice, "welcome") == 0))) {
               LMAPI->buffer_printf(outbuf, sizeof(outbuf) - 1, "Subscribed to list '%s'", listname);
               LMAPI->set_var("task-form-subject", outbuf, VAR_TEMP);
               if(!LMAPI->task_heading(fromaddy))
                   return HOOK_RESULT_FAIL;

               if (!LMAPI->get_bool("adminmode") || LMAPI->get_bool("admin-actions-shown")) {
                   LMAPI->smtp_body_text(">> (Subscribed to list by ");
                   LMAPI->smtp_body_text(LMAPI->get_var("realsender"));
                   LMAPI->smtp_body_line(")");
               }

               LMAPI->quote_command();
               LMAPI->buffer_printf(outbuf, sizeof(outbuf) - 1, "You have been added to list '%s'", listname);
               LMAPI->smtp_body_line(&outbuf[0]);
               LMAPI->task_ending();        
           }
        } else {
            LMAPI->spit_status("Subscribed.");
        }

        /* Do POSTSUB hooks */
        (void)LMAPI->do_hooks("POSTSUB");

        return CMD_RESULT_CONTINUE;
    } else {
        LMAPI->filesys_error(&userfilepath[0]);
        return CMD_RESULT_END;
    }
    return CMD_RESULT_CONTINUE;
}

/* submodes command */
CMD_HANDLER(cmd_submodes)
{
    const char *listname = LMAPI->get_var("list");
    struct submode *data;
    char *tmp;
    int col;

    if(params->num) {
        listname = params->words[0];
        if(!LMAPI->set_context_list(listname)) {
            LMAPI->nosuch(listname);
            /* Return CMD_RESULT_END cause otherwise the list context
             * might be incorrect for further changes
             */
             return CMD_RESULT_END;
        }
    }
    LMAPI->spit_status("Retrieving subscription mode list");
    LMAPI->result_printf("\nThe following subscription modes are defined for the list %s.\n", listname);
    LMAPI->result_printf("LIST SUBSCRIPTION MODES\n---\n");

    data = LMAPI->get_submodes();
    if(!data) {
        LMAPI->result_printf("No modes defined.\n\n");
        return CMD_RESULT_CONTINUE;
    }

    while(data) {
        LMAPI->result_printf("%s\n", data->modename);
        tmp = &(data->description[0]);
        col = 0;
        while(*tmp) {
            if (col == 0) {
                LMAPI->result_printf("          ");
                col = 10;
            }
            if ((*tmp == ' ') && col > 65) {
                col = 0;
                LMAPI->result_printf("\n");
                tmp++;
            } else {
                LMAPI->result_printf("%c", *tmp);
                tmp++; col++;
            }
        }
        LMAPI->result_printf("\n\n");
        data = data->next;
    }
    return CMD_RESULT_CONTINUE;
}

/* Submode command */
CMD_HANDLER(cmd_submode)
{
    const char *listname = NULL;
    char userfilepath[BIG_BUF];
    const char *fromaddy;
    const char *mode = NULL;
    struct submode *data;
    char *listdir;
    const char *adminnotice;

    /* Get our starting data */
    fromaddy = LMAPI->get_string("fromaddress");
    listname = LMAPI->get_var("list");


    /* Mode check */
    if(params->num == 0) {
        LMAPI->spit_status("You must provide a subscription mode.");
        return CMD_RESULT_CONTINUE;
    } else if(params->num == 1) {
        /* one param is the mode */
        mode = params->words[0];
    } else if(params->num == 2) {
        /* two params are mode and list or user for non-admins */
        /* or mode and user for admins */
        mode = params->words[0];
        if(strchr(params->words[1], '@')) {
            fromaddy = params->words[1];
        } else {
            if(LMAPI->get_bool("adminmode")) {
               LMAPI->spit_status("Cannot provide a list in this context.");
               return CMD_RESULT_CONTINUE;
            } else {
                listname = LMAPI->get_var("list");
            }
        }
    } else if(params->num == 3) {
        /* three params are mode, list and user (not legal for adminmode) */
        if(LMAPI->get_bool("adminmode")) {
           LMAPI->spit_status("Cannot provide a list in this context.");
           return CMD_RESULT_CONTINUE;
        }
        mode = params->words[0];
        listname = params->words[1];
        fromaddy = params->words[2];
    }

    /* Sanity check */
    if(listname == NULL) {
        LMAPI->spit_status("Does not appear to be a valid subscribe command.  No list in present context.  To see the lists available on this machine, send %s the command 'lists'.", SERVICE_NAME_MC);
        return CMD_RESULT_CONTINUE;
    }

    if(!LMAPI->set_context_list(listname)) {
       LMAPI->nosuch(listname);
       /* Return CMD_RESULT_END cause otherwise the list context
        * might be incorrect for further changes
        */
       return CMD_RESULT_END;
    }

    /* Sanity check */
    if(!strchr(fromaddy,'@') || !strchr(fromaddy,'@')) {
       LMAPI->spit_status("That doesn't appear to be a valid e-mail address.");
       return CMD_RESULT_CONTINUE;
    }

    /* Make sure it's a legal mode */
    data = LMAPI->get_submode(mode);
    if(!data) {
        LMAPI->spit_status("%s does not appear to be a valid subscription mode.", mode);
        return CMD_RESULT_CONTINUE;
    }

    /* Set up our data for PRESUB hooks */
    /* It must be VAR_GLOBAL to prevent the set_context_list
       calls that might get done from wiping it. */
    LMAPI->set_var("subscribe-me",fromaddy,VAR_GLOBAL);
    LMAPI->set_var("submodes-mode",mode,VAR_TEMP);
    LMAPI->lock_var("submodes-mode");

    /* Call PRESUB hooks */
    if (LMAPI->do_hooks("PRESUB") == HOOK_RESULT_FAIL) {
        /* If any reject, cancel and continue on */
        return CMD_RESULT_CONTINUE;
    }

    listdir = LMAPI->list_directory(LMAPI->get_string("list"));
    LMAPI->buffer_printf(userfilepath, sizeof(userfilepath) - 1, "%s/users", listdir);
    free(listdir);

    /* Add user... */
    if (!LMAPI->user_add(userfilepath,fromaddy)) {
        char outbuf[BIG_BUF];

        /* Log */
        LMAPI->log_printf(0, "%s subscribed to %s\n",fromaddy,listname);

        /* Send notification */
        if (LMAPI->get_var("adminmode") || strcmp(fromaddy,LMAPI->get_string("fromaddress"))) {
            LMAPI->spit_status("Successfully subscribed.");
            adminnotice = LMAPI->get_var("admin-subscribe-notice");

	    if(!LMAPI->get_bool("adminmode") ||
               (!LMAPI->get_bool("admin-silent-subscribe") &&
                !(strcasecmp(adminnotice, "silent") == 0) &&
                !(strcasecmp(adminnotice, "welcome") == 0))) {

                LMAPI->buffer_printf(outbuf, sizeof(outbuf) - 1, "Subscribed to list '%s'", listname);
                LMAPI->set_var("task-form-subject", outbuf, VAR_TEMP);
                if(!LMAPI->task_heading(fromaddy))
                    return HOOK_RESULT_FAIL;

                if (!LMAPI->get_bool("adminmode") || LMAPI->get_bool("admin-actions-shown")) {
                    LMAPI->smtp_body_text(">> (Subscribed to list by ");
                    LMAPI->smtp_body_text(LMAPI->get_var("realsender"));
                    LMAPI->smtp_body_line(")");
                }

                LMAPI->quote_command();
                LMAPI->buffer_printf(outbuf, sizeof(outbuf) - 1, "You have been added to list '%s'", listname);
                LMAPI->smtp_body_line(&outbuf[0]);
                LMAPI->task_ending();
            }
        } else {
            LMAPI->spit_status("Subscribed.");
        }

        /* Do POSTSUB hooks */
        (void)LMAPI->do_hooks("POSTSUB");
        LMAPI->unlock_var("submodes-mode");

        return CMD_RESULT_CONTINUE;
    } else {
        LMAPI->unlock_var("submodes-mode");
        LMAPI->filesys_error(&userfilepath[0]);
        return CMD_RESULT_END;
    }
    LMAPI->unlock_var("submodes-mode");
    return CMD_RESULT_CONTINUE;
}

/* 'appsub' command */
CMD_HANDLER(cmd_appsub)
{
   /* Sanity check */
   if (params->num != 3) {
      LMAPI->spit_status("Invalid number of parameters.");
      return CMD_RESULT_CONTINUE;
   } else {
      struct list_user user;
      const char *fromaddy;
      char filename[SMALL_BUF], buffer[BIG_BUF]; /* buffer changed from 256(SMALL_BUF) to BIG_BUF due to verify_cookie */
      int isadmin;
      char *listdir;

      isadmin = 0;

      /* Set list context */     
      if (!LMAPI->set_context_list(params->words[0])) {
         LMAPI->nosuch(params->words[0]);
         return CMD_RESULT_CONTINUE;
      }

      /* check who's doing this */ 
      if (LMAPI->user_find_list(params->words[0],LMAPI->get_string("realsender"),&user)) {
         if (LMAPI->user_hasflag(&user,"ADMIN")) {
            fromaddy = params->words[1]; isadmin=1;
         } else {
            fromaddy = params->words[1];
         }
      } else fromaddy = params->words[1];

      listdir = LMAPI->list_directory(params->words[0]);
      LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s/cookies", listdir);
      free(listdir);

      /* Verify the cookie exists */
      if (LMAPI->verify_cookie(filename,params->words[2],'S', &buffer[0])) {
        /* Match the cookie data */
        if(LMAPI->match_cookie(params->words[2],LMAPI->get_string("list"))) {
           char *tptr;
           char *submode;

           LMAPI->del_cookie(filename,params->words[2]);
           listdir = LMAPI->list_directory(params->words[0]);
           LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s/users", listdir);
           free(listdir);

           submode = NULL;

           tptr = &buffer[0];
           if (*tptr == '@') {
              tptr++;
              submode = tptr;
              while(*tptr != '@') tptr++;
              *tptr++ = '\0';
           }
           if(submode) {
              LMAPI->set_var("submodes-mode", submode, VAR_TEMP);
              LMAPI->lock_var("submodes-mode");
           }

           /* Add user */              
           if (!LMAPI->user_add(filename,tptr)) {
              char outbuf[BIG_BUF];

              LMAPI->log_printf(0, "%s subscribed to %s\n",fromaddy,params->words[0]);

              if (isadmin) {
                 LMAPI->buffer_printf(outbuf, sizeof(outbuf) - 1, "Subscribed to list '%s'", params->words[0]);
                 LMAPI->set_var("task-form-subject", outbuf, VAR_TEMP);
                 LMAPI->spit_status("Successfully subscribed.");
                 if(!LMAPI->task_heading(fromaddy))
                    return HOOK_RESULT_FAIL;
                 LMAPI->smtp_body_text(">> (Subscribed to list by ");
                 LMAPI->smtp_body_text(LMAPI->get_var("realsender"));
                 LMAPI->smtp_body_line(")");
                 LMAPI->quote_command();
                 LMAPI->buffer_printf(outbuf, sizeof(outbuf) - 1, "You have been added to list '%s'", params->words[0]);
                 LMAPI->smtp_body_line(&outbuf[0]);
                 LMAPI->task_ending();        
              } else {
                 LMAPI->spit_status("Subscribed.");
              }
              LMAPI->set_var("subscribe-me",fromaddy,VAR_GLOBAL);

              /* Do POSTSUB hooks */
              (void)LMAPI->do_hooks("POSTSUB");
              if(submode) {
                  LMAPI->unlock_var("submodes-mode");
              }
              return CMD_RESULT_CONTINUE;
           } else {
              LMAPI->filesys_error(&filename[0]);
              return CMD_RESULT_END;
           }              
        } else {
           LMAPI->spit_status("Cookie cannot be used from this address.");
           return CMD_RESULT_CONTINUE;
        }
      } else {
        LMAPI->spit_status("No such cookie or else cookie is of wrong type.");
        return CMD_RESULT_CONTINUE;
      }       
   }
   return CMD_RESULT_CONTINUE;
}
