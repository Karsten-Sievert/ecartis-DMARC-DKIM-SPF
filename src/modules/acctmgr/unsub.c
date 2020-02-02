#include "config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "acctmgr-mod.h"

/* PREUNSUB hook to check if user is subscribed */
HOOK_HANDLER(hook_preunsub_subscribed)
{
    const char *fromaddy, *listname;
    struct list_user user;

    /* Get our data */
    fromaddy = LMAPI->get_string("subscribe-me");
    listname = LMAPI->get_string("list");

    /* Sanity check */
    if(!LMAPI->list_valid(listname)) {
        LMAPI->nosuch(listname);
        return HOOK_RESULT_FAIL;
    }

    /* Check if user is on list */
    if (!LMAPI->user_find_list(listname,fromaddy,&user)) {
        if (LMAPI->get_bool("adminmode")) {
            LMAPI->spit_status("User is not a member of that list.");
            return HOOK_RESULT_FAIL;
        } else {
            LMAPI->spit_status("'Unsubscribe' request denied.");
            LMAPI->result_printf("Your request was rejected for the following reason:\n\n");
            LMAPI->result_printf("You are not on the list '%s'.\n\n",listname);
        }
        return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;
}

/* PREUNSUB hook to check if list is closed for unsubscribe */
HOOK_HANDLER(hook_preunsub_closed)
{
    const char *unsubscribemode;
    const char *fromaddy, *listname;
    char buf[BIG_BUF];

    /* Admin mode is immune to this check */
    if (LMAPI->get_bool("adminmode"))
        return HOOK_RESULT_OK;

    unsubscribemode = LMAPI->get_var("unsubscribe-mode");

    if (!unsubscribemode)
        unsubscribemode = LMAPI->get_var("subscribe-mode");

    fromaddy = LMAPI->get_string("subscribe-me");
    listname = LMAPI->get_string("list");

    /* Check the mode */
    if (strcasecmp(unsubscribemode,"closed") == 0) {
       const char *adminaddy;
       char cookie[BIG_BUF], cookiefile[SMALL_BUF];
       char cmdbuf[BIG_BUF];
       char *cmdptr;
       char *listdir;

       adminaddy = LMAPI->get_var("administrivia-address");
       if (!adminaddy) adminaddy = LMAPI->get_string("list-owner");

       listdir = LMAPI->list_directory(LMAPI->get_string("list"));
       LMAPI->buffer_printf(cookiefile, sizeof(cookiefile) - 1, "%s/cookies",  listdir);

       LMAPI->set_var("cookie-for", LMAPI->get_string("list"), VAR_TEMP);

       /* Request our cookie */
       if (!LMAPI->request_cookie(cookiefile,&cookie[0],'U',fromaddy)) {
           LMAPI->spit_status("Unable to generate unsubscribe cookie!");
           LMAPI->filesys_error(cookiefile);
           return HOOK_RESULT_FAIL;
       }
       LMAPI->clean_var("cookie-for", VAR_TEMP);

       LMAPI->buffer_printf(buf, sizeof(buf) - 1, "Unsubscription request for '%s'", listname);
       LMAPI->set_var("task-form-subject", buf, VAR_TEMP);

       /* ...and send the ticket to the admin */
       if(!LMAPI->task_heading(adminaddy))
           return HOOK_RESULT_FAIL;
       LMAPI->smtp_body_text("# Unsubscription request received from ");
       LMAPI->smtp_body_line(LMAPI->get_string("fromaddress"));
       LMAPI->smtp_body_text("# for the address ");
       LMAPI->smtp_body_text(fromaddy);
       LMAPI->smtp_body_text(" and list ");
       LMAPI->smtp_body_line(LMAPI->get_string("list"));
       LMAPI->smtp_body_line("# ");
       LMAPI->smtp_body_text("# To approve this, reply to this to ");
       LMAPI->smtp_body_text(LMAPI->get_string("listserver-address"));
       LMAPI->smtp_body_line(":");
       LMAPI->smtp_body_line("// job");

       LMAPI->buffer_printf(cmdbuf, sizeof(cmdbuf) - 1, "appunsub %s %s %s",
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

       /* With the original request if so desired */
       if (LMAPI->get_bool("administrivia-include-requests")) {
          FILE *infile;
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

       LMAPI->spit_status("List is closed-unsubscription, request has been forwarded to list admins.");

       return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;
}

/* PREUNSUB hook to check for confirm-mode unsubscribe */
HOOK_HANDLER(hook_preunsub_confirm)
{
    const char *unsubscribemode;
    const char *fromaddy, *listname;

    if (LMAPI->get_bool("adminmode"))
        return HOOK_RESULT_OK;

    unsubscribemode = LMAPI->get_var("unsubscribe-mode");

    /* Default to subscribe-mode if not set */
    if (!unsubscribemode)
        unsubscribemode = LMAPI->get_var("subscribe-mode");

    /* Get our data */
    fromaddy = LMAPI->get_string("subscribe-me");
    listname = LMAPI->get_string("list");

    /* Check mode equal to confirm, or the addresses not matching */
    if ((strcasecmp(unsubscribemode,"confirm") == 0) ||
       (strcasecmp(fromaddy,LMAPI->get_string("fromaddress")) && 
       (!LMAPI->get_bool("adminmode")) &&
       (strcasecmp(unsubscribemode,"open-auto") != 0)))
    {
       const char *adminaddy;
       const char *sendas;
       char cookie[BIG_BUF], cookiefile[BIG_BUF]; /* Changed cookiefile from SMALL_BUF to BIG_BUF due to listdir_file */
       char cmdbuf[BIG_BUF];
       char *cmdptr;
	   int unsub_confirm_file_included = 0;

       adminaddy = LMAPI->get_var("administrivia-address");
       if (!adminaddy) adminaddy = LMAPI->get_string("list-owner");

       LMAPI->listdir_file(cookiefile, LMAPI->get_string("list"), "cookies");

       LMAPI->set_var("cookie-for", LMAPI->get_string("list"), VAR_TEMP);

       /* Request our cookie */
       if (!LMAPI->request_cookie(cookiefile,&cookie[0],'U',fromaddy)) {
           LMAPI->spit_status("Unable to generate unsubscribe cookie!");
           LMAPI->filesys_error(cookiefile);
           return HOOK_RESULT_FAIL;
       }
       LMAPI->clean_var("cookie-for", VAR_TEMP);

       /* Spit back the status */
       LMAPI->spit_status("Subscription confirmation ticket sent to user being unsubscribed.");
       sendas = LMAPI->get_var("send-as");

       /* And send the ticket */
       if (!sendas) sendas = LMAPI->get_var("list-owner");
       if (!sendas) sendas = LMAPI->get_var("listserver-address");
       LMAPI->set_var("form-send-as", sendas, VAR_TEMP);
       LMAPI->set_var("form-reply-to",
			   LMAPI->get_string("listserver-address"), VAR_TEMP);
	   LMAPI->set_var("task-form-subject",
			   LMAPI->get_string("unsubscribe-confirm-subject"), VAR_TEMP);

       if(!LMAPI->task_heading(fromaddy))
           return HOOK_RESULT_FAIL;

	   if(LMAPI->get_var("unsubscribe-confirm-file")) {
		   FILE *infile;
		   char tempfilename[BIG_BUF]; /* Changed from SMALL_BUF to BIG_BUF due to listdir_file */
		   LMAPI->listdir_file(tempfilename, LMAPI->get_string("list"),
				   LMAPI->get_string("unsubscribe-confirm-file"));

		   if((infile = LMAPI->open_file(tempfilename, "r")) != NULL) {
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

			   unsub_confirm_file_included = 1;
		   }
	   }
	   if(!unsub_confirm_file_included) {
		   LMAPI->smtp_body_text("# ");
		   LMAPI->smtp_body_text(LMAPI->get_string("fromaddress"));
		   LMAPI->smtp_body_line(" has requested that you be unsubscribed");
		   LMAPI->smtp_body_text("# from the ");
		   LMAPI->smtp_body_text(LMAPI->get_string("list"));
		   LMAPI->smtp_body_line(" mailing list.");
		   LMAPI->smtp_body_line("# To unsubscribe, reply to this message leaving the message body");
		   LMAPI->smtp_body_line("# intact, or send the following lines in e-mail to");
		   LMAPI->smtp_body_text(LMAPI->get_string("listserver-address"));
		   LMAPI->smtp_body_line(":\n");
	   }
	   LMAPI->smtp_body_line("// job");

       LMAPI->buffer_printf(cmdbuf, sizeof(cmdbuf) - 1, "appunsub %s %s %s",
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

    /* If we have prevent-second-message set, eat the PERsonal Results output for this session so far. */ 
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

/* POSTUNSUB hook to send the welcome message */
HOOK_HANDLER(hook_postunsub_goodbye)
{
    char outbuf[BIG_BUF];
    const char *fromaddy, *listname;
    char *listdir;
    const char *adminnotice;
    
    /* If ADMIN, check if message should be sent */
    if (LMAPI->get_bool("adminmode")) {
        if (LMAPI->get_bool("admin-silent-subscribe"))
            return HOOK_RESULT_OK;

        adminnotice = LMAPI->get_var("admin-unsubscribe-notice");
        if ((strcasecmp(adminnotice, "silent") == 0) ||
            (strcasecmp(adminnotice, "notify") == 0))
            return HOOK_RESULT_OK;
    }

    /* Get our data */
    fromaddy = LMAPI->get_string("subscribe-me");
    listname = LMAPI->get_string("list");
    
    /* Send the textfile.  Easy! */
    listdir = LMAPI->list_directory(listname);
    LMAPI->buffer_printf(outbuf, sizeof(outbuf) - 1, "%s/%s", listdir, LMAPI->get_string("goodbye-file"));
    free(listdir);
    LMAPI->set_var("task-form-subject",
    	LMAPI->get_string("goodbye-subject"), VAR_TEMP);
    LMAPI->send_textfile_expand(fromaddy,outbuf);
    LMAPI->clean_var("task-form-subject", VAR_TEMP);

    if (LMAPI->exists_file(outbuf) && LMAPI->get_bool("prevent-second-message")) {
       char resultfile[BIG_BUF];

       LMAPI->buffer_printf(resultfile, sizeof(resultfile) - 1, 
          "%s.perr", LMAPI->get_string("queuefile"));

       LMAPI->unlink_file(resultfile);
    }
    
    return HOOK_RESULT_OK;
}

/* POSTUNSUB hook for sending the administrivia */
HOOK_HANDLER(hook_postunsub_administrivia)
{
    const char *fromaddy, *listname;

    /* Get our data */
    fromaddy = LMAPI->get_string("subscribe-me");
    listname = LMAPI->get_string("list");

    /* Make sure we're supposed to send */
    if(!LMAPI->get_bool("no-administrivia")) {
        const char *toaddy = NULL;
        
        toaddy = LMAPI->get_var("administrivia-address");
        if (!toaddy && LMAPI->get_bool("owner-fallback")) 
           toaddy = LMAPI->get_var("list-owner");

        if(toaddy) {
            char subject[SMALL_BUF];

            LMAPI->buffer_printf(subject, sizeof(subject) - 1, "%s unsubscribed from %s",
               fromaddy, listname);

            /* Send the administrivia note */
            LMAPI->set_var("task-form-subject",subject,VAR_TEMP);
            if(!LMAPI->task_heading(toaddy))
                return HOOK_RESULT_FAIL;
            LMAPI->smtp_body_text(fromaddy);
            LMAPI->smtp_body_text(" unsubscribed from list ");
            LMAPI->smtp_body_line(listname);
            LMAPI->smtp_body_text("Command came from: ");
            LMAPI->smtp_body_line(LMAPI->get_string("realsender"));

            /* And include the request, if so desired */
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

/* 'unsubscribe' command */
CMD_HANDLER(cmd_unsubscribe)
{
    const char *listname = NULL;
    char userfilepath[BIG_BUF];
    struct list_user user;
    const char *fromaddy;
    char *listdir;
    const char *adminnotice;

    fromaddy = LMAPI->get_string("fromaddress");
    listname = LMAPI->get_var("list");

    /* Mode check */
    if(params->num == 1) {
        if(LMAPI->get_bool("adminmode")) {
            /* Make sure it's an address if we're in admin mode */
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
        LMAPI->spit_status("Does not appear to be a valid unsubscribe command.  No list in present context.  To see the lists available on this machine, send %s the command 'lists'.", SERVICE_NAME_MC);
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

    /* Set up our data */
    /* This needs to be VAR_GLOBAL to keep from being clobbered. */
    LMAPI->set_var("subscribe-me",fromaddy,VAR_GLOBAL);

    /* Run PREUNSUB hooks */
    if (LMAPI->do_hooks("PREUNSUB") == HOOK_RESULT_FAIL) {
        return CMD_RESULT_CONTINUE;
    }

    listdir = LMAPI->list_directory(LMAPI->get_string("list"));
    LMAPI->buffer_printf(userfilepath, sizeof(userfilepath) - 1, "%s/users", listdir);
    free(listdir);

    /* Make sure the address we unsubscribe is the correct one, in cases
       where loose domain matching is enabled. */
    if (!LMAPI->user_find_list(LMAPI->get_string("list"),fromaddy,&user)) {
       LMAPI->spit_status("Error unsubscribing.");
       return CMD_RESULT_CONTINUE;
    }

    LMAPI->set_var("subscribe-me",user.address,VAR_GLOBAL);

    /* Remove user */
    if (!LMAPI->user_remove(userfilepath,user.address)) {
        char outbuf[BIG_BUF];

        LMAPI->log_printf(0, "%s unsubscribed from %s\n",user.address,listname);

        /* Send result note */
        if (LMAPI->get_var("adminmode") || strcmp(fromaddy,LMAPI->get_string("fromaddress"))) {

            LMAPI->spit_status("Successfully unsubscribed.");

            adminnotice = LMAPI->get_var("admin-unsubscribe-notice");

            if (!LMAPI->get_bool("adminmode") ||
                (!LMAPI->get_bool("admin-silent-subscribe") &&
                 !(strcasecmp(adminnotice, "silent") == 0) &&
                 !(strcasecmp(adminnotice, "goodbye") == 0))) {

               LMAPI->buffer_printf(outbuf, sizeof(outbuf) - 1, "Unsubscribed from list '%s'", listname);
               LMAPI->set_var("task-form-subject", outbuf, VAR_TEMP);

               if(!LMAPI->task_heading(fromaddy))
                   return HOOK_RESULT_FAIL;

               if (!LMAPI->get_bool("adminmode") || LMAPI->get_bool("admin-actions-shown")) {
                   LMAPI->smtp_body_text(">> (Unsubscribed from list by ");
                   LMAPI->smtp_body_text(LMAPI->get_var("realsender"));
                   LMAPI->smtp_body_line(")");
               }
               LMAPI->quote_command();
               LMAPI->buffer_printf(outbuf, sizeof(outbuf) - 1, "You have been removed from list '%s'", listname);
               LMAPI->smtp_body_line(&outbuf[0]);
               LMAPI->task_ending();        
            }
        } else {
            LMAPI->spit_status("Unsubscribed.");
        }

        /* Do POSTUNSUB hooks */
        (void)LMAPI->do_hooks("POSTUNSUB");

        return CMD_RESULT_CONTINUE;
    } else {
        LMAPI->filesys_error(&userfilepath[0]);
        return CMD_RESULT_END;
    }
    return CMD_RESULT_CONTINUE;
}

/* 'appunsub' command */
CMD_HANDLER(cmd_appunsub)
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
         LMAPI->spit_status("Invalid list.");
         return CMD_RESULT_CONTINUE;
      }

      /* Check if we're admin or not */
      if (LMAPI->user_find_list(params->words[0],LMAPI->get_string("realsender"),&user)) {
         if (LMAPI->user_hasflag(&user,"ADMIN")) {
            fromaddy = params->words[1]; isadmin=1;
         } else {
            fromaddy = params->words[1];
         }
      } else fromaddy = params->words[1];

      listdir = LMAPI->list_directory(params->words[0]);
      LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s/cookies", listdir);

      /* Verify cookie exists */
      if (LMAPI->verify_cookie(filename,params->words[2],'U', &buffer[0])) {
        /* Verify cookie data matches */
        if(LMAPI->match_cookie(params->words[2],LMAPI->get_string("list"))) {
           struct list_user user;

           LMAPI->del_cookie(filename,params->words[2]);
           listdir = LMAPI->list_directory(params->words[0]);
           LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s/users", listdir);
           free(listdir);

           if (!LMAPI->user_find_list(params->words[0],
             fromaddy, &user)) {
              LMAPI->spit_status("Error unsubscribing; user is no longer on list.");
              return CMD_RESULT_CONTINUE;
           }

           /* Remove the user */
           if (!LMAPI->user_remove(filename,fromaddy)) {
               char outbuf[BIG_BUF];

               LMAPI->log_printf(0, "%s unsubscribed from %s\n",fromaddy,params->words[0]);

               /* Send note */
               if (isadmin) {
                   LMAPI->buffer_printf(outbuf, sizeof(outbuf) - 1, "Unsubscribed from list '%s'", params->words[0]);
                   LMAPI->set_var("task-form-subject", outbuf, VAR_TEMP);

                   LMAPI->spit_status("Successfully unsubscribed.");
                   if(!LMAPI->task_heading(fromaddy))
                       return HOOK_RESULT_FAIL;
                   LMAPI->smtp_body_text(">> (unsubscribed from list by ");
                   LMAPI->smtp_body_text(LMAPI->get_var("realsender"));
                   LMAPI->smtp_body_line(")");
                   LMAPI->quote_command();
                   LMAPI->buffer_printf(outbuf, sizeof(outbuf) - 1, "You have been removed from list '%s'", params->words[0]);
                   LMAPI->smtp_body_line(&outbuf[0]);
                   LMAPI->task_ending();        
               } else {
                   LMAPI->spit_status("Unsubscribed.");
               }

               /* Do POSTUNSUB hooks */

               LMAPI->set_var("subscribe-me",user.address,VAR_GLOBAL);
               (void)LMAPI->do_hooks("POSTUNSUB");
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
