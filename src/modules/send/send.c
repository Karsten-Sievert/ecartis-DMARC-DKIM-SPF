#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#endif

#include "send-mod.h"

HOOK_HANDLER(hook_tolist_sort)
{
    if (!LMAPI->get_bool("sort-tolist")) return HOOK_RESULT_OK;

    LMAPI->sort_tolist();
    return HOOK_RESULT_OK;
}

HOOK_HANDLER(hook_tolist_build_tolist)
{
   char *extra_lists; 

   LMAPI->new_tolist();

   LMAPI->add_from_list_all(LMAPI->get_string("list"));

   if(LMAPI->get_bool("megalist"))
       return HOOK_RESULT_OK;

   /* Okay.. Heres' how we bounce to multiple lists */
   extra_lists = strdup(LMAPI->get_string("cc-lists"));
   if(extra_lists) {
       char *l = strtok(extra_lists, ":");
       while(l) {
           if(LMAPI->list_valid(l)) {
              LMAPI->add_from_list_all(l);
           }
           l = strtok(NULL,":");
       }
   }
   free(extra_lists);

   return(HOOK_RESULT_OK);
}

HOOK_HANDLER(hook_tolist_remove_vacationers)
{
   LMAPI->remove_flagged_all_prilist("VACATION",LMAPI->get_string("list"));
   LMAPI->remove_flagged_all_prilist("DIAGNOSTIC",LMAPI->get_string("list"));

   return(HOOK_RESULT_OK);
}

HOOK_HANDLER(hook_presend_check_moderate)
{
   if (LMAPI->get_bool("moderated")) {
      const char *tempchk;
      struct list_user user;

      if (LMAPI->user_find_list(LMAPI->get_string("list"),LMAPI->get_string("realsender"),
         &user)) {
         if (LMAPI->user_hasflag(&user,"ADMIN")) {
            if (LMAPI->get_bool("admin-approvepost")) {
               LMAPI->set_var("moderated-approved-by",LMAPI->get_string("realsender"),VAR_GLOBAL);
            }
         }
         if (LMAPI->user_hasflag(&user,"PREAPPROVE")) {
            LMAPI->set_var("moderated-approved-by",LMAPI->get_string("realsender"),VAR_GLOBAL);
         }
         if(LMAPI->user_hasflag(&user, "MODERATOR")) {
            if (LMAPI->get_bool("moderator-approvepost")) {
               LMAPI->set_var("moderated-approved-by",LMAPI->get_string("realsender"),VAR_GLOBAL);
            }
         }
      }

      tempchk = LMAPI->get_var("moderated-approved-by");
      if (!tempchk) {
         LMAPI->make_moderated_post("Post to moderated list.");
         return HOOK_RESULT_STOP;
      }
   }
   return (HOOK_RESULT_OK);
}

HOOK_HANDLER(hook_presend_check_modpost)
{
   /* No point in checking if the list is moderated, eh? */
   if (!LMAPI->get_bool("moderated")) {
      const char *tempchk;
      struct list_user user;

      if (LMAPI->user_find_list(LMAPI->get_string("list"),LMAPI->get_string("realsender"),
         &user)) {
         if (LMAPI->user_hasflag(&user,"MODPOST")) {
            tempchk = LMAPI->get_var("moderated-approved-by");
            if (!tempchk) {
               LMAPI->make_moderated_post("Post by a user set MODPOST.");
               return HOOK_RESULT_STOP;
            }
         }
      }

   }
   return (HOOK_RESULT_OK);
}


HOOK_HANDLER(hook_presend_check_closed)
{
   if (LMAPI->get_var("moderated-approved-by")) return HOOK_RESULT_OK;

   if (LMAPI->get_bool("closed-post")) {
      struct list_user user; 
      int founduser;
      char userfilepath[BIG_BUF];  /* Changed from SMALL_BUF to BIG_BUF due to listdir_file */

      LMAPI->listdir_file(userfilepath, LMAPI->get_string("list"), "users");

      founduser = LMAPI->user_find_list(LMAPI->get_string("list"),LMAPI->get_string("fromaddress"),&user);

      if (!founduser) {
         founduser = LMAPI->user_find_list(LMAPI->get_string("list"),
                LMAPI->get_string("fromaddress"),&user);
      }

      if (!founduser && LMAPI->get_bool("posting-acl")) {
         char aclfilename[BIG_BUF];
         FILE *aclfile;
         char aclbuffer[BIG_BUF];

         LMAPI->listdir_file(aclfilename,LMAPI->get_string("list"),
                             LMAPI->get_string("posting-acl-file"));         

         if (LMAPI->exists_file(aclfilename)) {
            
            if ((aclfile = LMAPI->open_file(aclfilename,"r")) != NULL) {

               while (LMAPI->read_file(aclbuffer, sizeof(aclbuffer), aclfile)) {

                  if (aclbuffer[strlen(aclbuffer) - 1] == '\n')
                      aclbuffer[strlen(aclbuffer) - 1] = 0;

                  if (LMAPI->match_reg(aclbuffer, 
                                       LMAPI->get_string("fromaddress")))
                     founduser = 1;

               }

               LMAPI->close_file(aclfile);

            }

         }

      }

      if (!founduser && LMAPI->get_var("union-lists")) {
         char unions[64];
         char *tptr, *tptr2;

         LMAPI->log_printf(9,"Checking union lists...\n");

         LMAPI->buffer_printf(unions, 64, "%.63s", LMAPI->get_var("union-lists"));

         tptr = &unions[0];
         tptr2 = strchr(tptr,':');
         if (tptr2) *tptr2++ = 0;

         while (!founduser && tptr) {
            LMAPI->log_printf(9,"Checking '%s'...\n", tptr);

            founduser = LMAPI->user_find_list(tptr,
               LMAPI->get_string("fromaddress"), &user);

            tptr = tptr2;

            if (tptr) {
               tptr2 = strchr(tptr,':');
               if (tptr2) *tptr2++ = 0;
            }
         }         
      }

      if (!founduser) {
            const char *closedfile;
            int usedefault;
            char inbuffer[BIG_BUF];
            const char *closedsubject;

            closedfile = LMAPI->get_var("closed-file");
            usedefault = 1;
            closedsubject = LMAPI->get_var("closed-post-subject");
            
            if (!closedsubject) {
               LMAPI->buffer_printf(inbuffer, sizeof(inbuffer) - 1, "List '%s' closed to public posts", LMAPI->get_string("list"));
            }
            else {
               LMAPI->buffer_printf(inbuffer, sizeof(inbuffer) - 1, "%s", closedsubject);
            }
            LMAPI->set_var("task-form-subject",&inbuffer[0],VAR_TEMP);

            if (closedfile) {
                LMAPI->listdir_file(inbuffer, LMAPI->get_string("list"),
                                    closedfile);

                usedefault = !LMAPI->send_textfile_expand_append(LMAPI->get_string("fromaddress"),&inbuffer[0],LMAPI->get_bool("moderate-include-queue"));
            }

            if(!LMAPI->get_bool("closed-post-blackhole")) {
                LMAPI->clean_var("moderate-quiet",VAR_TEMP);
                if (!usedefault)
                   LMAPI->set_var("moderate-quiet","yes",VAR_TEMP);
                LMAPI->make_moderated_post("Non-member submission to closed-post list.");
                LMAPI->clean_var("moderate-quiet",VAR_TEMP);
            }
            return HOOK_RESULT_STOP;
        }         
    }
    return (HOOK_RESULT_OK);
}

HOOK_HANDLER(hook_presend_check_outside)
{
   struct list_user user; 
   int founduser;
   char userfilepath[BIG_BUF]; /* Changed from SMALL_BUF to BIG_BUF due to listdir_file */

   if (LMAPI->get_var("moderated-approved-by")) return HOOK_RESULT_OK;
   if (LMAPI->get_bool("closed-post")) return HOOK_RESULT_OK;

   LMAPI->listdir_file(userfilepath, LMAPI->get_string("list"), "users");

   founduser = LMAPI->user_find_list(LMAPI->get_string("list"),LMAPI->get_string("fromaddress"),&user);

   if (!founduser && LMAPI->get_var("union-lists")) {
      char unions[64];
      char *tptr, *tptr2;

      LMAPI->log_printf(9,"Checking union lists...\n");

      LMAPI->buffer_printf(unions, 64, "%.63s", LMAPI->get_var("union-lists"));

      tptr = &unions[0];
      tptr2 = strchr(tptr,':');
      if (tptr2) *tptr2++ = 0;

      while (!founduser && tptr) {
         LMAPI->log_printf(9,"Checking '%s'...\n", tptr);

         founduser = LMAPI->user_find_list(tptr,
               LMAPI->get_string("fromaddress"), &user);

         tptr = tptr2;

         if (tptr) {
            tptr2 = strchr(unions,':');
            if (tptr2) *tptr2++ = 0;
         }
      }         
   }

   if (!founduser) {
         const char *outsidefile;
         char inbuffer[BIG_BUF];

         outsidefile = LMAPI->get_var("outside-file");

         if (outsidefile) {
            LMAPI->buffer_printf(inbuffer, sizeof(inbuffer) - 1, "Re: post to '%s'",
               LMAPI->get_string("list"));

            LMAPI->set_var("task-form-subject",&inbuffer[0],VAR_TEMP);

            LMAPI->listdir_file(inbuffer, LMAPI->get_string("list"),
                                outsidefile);

            LMAPI->send_textfile_expand(LMAPI->get_string("fromaddress"),
               &inbuffer[0]);
         }
         LMAPI->clean_var("task-form-subject",VAR_TEMP);
    }
    return (HOOK_RESULT_OK);
}


HOOK_HANDLER(hook_presend_check_nopost)
{
      struct list_user user; 
      int founduser;

      if (LMAPI->get_var("moderated-approved-by")) return HOOK_RESULT_OK;

      founduser = LMAPI->user_find_list(LMAPI->get_string("list"),LMAPI->get_string("fromaddress"),&user);

      if (founduser && LMAPI->user_hasflag(&user,"NOPOST")) {
            const char *closedfile;
            int usedefault;
            char inbuffer[BIG_BUF];

            closedfile = LMAPI->get_var("nopost-file");
            usedefault = 1;

            LMAPI->buffer_printf(inbuffer, sizeof(inbuffer) - 1, "Set NOPOST on list '%s'", LMAPI->get_string("list"));
            LMAPI->set_var("task-form-subject",&inbuffer[0],VAR_TEMP);

            if (closedfile) {
                LMAPI->listdir_file(inbuffer, LMAPI->get_string("list"), 
                                    closedfile);

                usedefault = !LMAPI->send_textfile_expand(LMAPI->get_string("fromaddress"),&inbuffer[0]);
            }

            if (usedefault) {
                if(!LMAPI->task_heading(LMAPI->get_string("fromaddress")))
                    return HOOK_RESULT_FAIL;
                LMAPI->smtp_body_text("Your post to list '");
                LMAPI->smtp_body_text(LMAPI->get_string("list"));
                LMAPI->smtp_body_line("' could not be processed.");
                LMAPI->smtp_body_line("");
                LMAPI->smtp_body_line("You are set NOPOST for this list.  Users who are set NOPOST cannot");
                LMAPI->smtp_body_line("post to the list.  This flag is usually used for disciplinary reasons.");
                LMAPI->smtp_body_line("");
                LMAPI->smtp_body_line("Your post has been eaten.  It was crunchy, and tasted good with");
                LMAPI->smtp_body_line("ketchup.");
                LMAPI->smtp_body_line("");
                LMAPI->smtp_body_line("You may wish to contact a list owner if you are unsure why you are");
                LMAPI->smtp_body_line("set NOPOST.");
                LMAPI->task_ending();
            }
            return HOOK_RESULT_STOP;
        }         

        return (HOOK_RESULT_OK);
}


HOOK_HANDLER(hook_final_send)
{
    const char *sendas;

    /* First thing, build the TO list so that we can know where we're going */
    if(LMAPI->do_hooks("TOLIST") == HOOK_RESULT_FAIL)
        return HOOK_RESULT_FAIL;

    sendas = LMAPI->get_var("send-as");
    if (!sendas) sendas = LMAPI->get_var("list-owner");
    if (!sendas) sendas = LMAPI->get_var("listserver-admin");

    if(!LMAPI->send_to_tolist(sendas, LMAPI->get_string("queuefile"), 1, 1,
                              LMAPI->get_bool("full-bounce"))) {
          return HOOK_RESULT_FAIL;
    }

    if(LMAPI->do_hooks("AFTER") == HOOK_RESULT_FAIL)
        return HOOK_RESULT_FAIL;
    return HOOK_RESULT_OK;
}

HOOK_HANDLER(hook_presend_check_messageid)
{
    char buffer[BIG_BUF], buffer2[BIG_BUF];
    FILE *infile, *messagefile;
    time_t lasttime;
    int inbody, gotit;
    int docheck;

    if (!LMAPI->get_bool("no-dupes"))
        return HOOK_RESULT_OK;

    docheck = 1;

    /* If this has been approved, we don't need to dupecheck it.  If an admin/moderator
       has a mailer loop, they shouldn't be moderating anyway. */
    if (LMAPI->get_var("moderated-approved-by"))
       return HOOK_RESULT_OK;

    infile = LMAPI->open_file(LMAPI->get_string("queuefile"), "r");
    if (infile == NULL) {
       return HOOK_RESULT_OK;
    }

    LMAPI->listdir_file(buffer, LMAPI->get_string("list"), "nodupes");

    if ((messagefile = LMAPI->open_exclusive(buffer,"a+")) == NULL) {
       LMAPI->close_file(infile);
       return HOOK_RESULT_OK;
    }
    LMAPI->rewind_file(messagefile);

    if(!LMAPI->read_file(buffer, sizeof(buffer), messagefile)) {
       time(&lasttime);
       LMAPI->write_file(messagefile,"%d\n",(int)lasttime);      
       docheck = 0;
    } else {
       time_t now;

       time(&now);
       lasttime = (time_t)atoi(buffer);

       if (!LMAPI->get_bool("no-dupes-forever")) {
          if ((now / 86400) != (lasttime / 86400)) {
             LMAPI->rewind_file(messagefile);
             LMAPI->truncate_file(messagefile, 0);
             LMAPI->write_file(messagefile,"%d\n",(int)now);
             docheck = 0;
          }
       }
    }

    inbody = 0; gotit = 0;

    while(LMAPI->read_file(buffer, sizeof(buffer), infile) && !inbody) {
       if (!strncasecmp(buffer,"Message-Id:",11)) {
          gotit = 1;
          break;    
       }
       if (buffer[0] == '\n') inbody = 1;
    }


    if (!gotit) {
       LMAPI->log_printf(1,"No message ID for message, unable to dupe-check.\n");
       LMAPI->close_file(messagefile);
       LMAPI->close_file(infile);
       return HOOK_RESULT_OK;
    }

    if (!docheck) {
       LMAPI->write_file(messagefile,"%s",buffer);
       LMAPI->close_file(messagefile);
       LMAPI->close_file(infile);
       return HOOK_RESULT_OK;
    }

    gotit = 0;

    while(LMAPI->read_file(buffer2, sizeof(buffer2), messagefile)) {
       if (!strcasecmp(buffer2,buffer))
           gotit = 1;
    }

    if (!gotit) {
      /* We should now be situated at the end of the file */
      LMAPI->write_file(messagefile,"%s",buffer);
      LMAPI->close_file(messagefile);
      LMAPI->close_file(infile);
      return HOOK_RESULT_OK;
    } else {
      LMAPI->log_printf(0,"Received duplicate message, ignoring.\n");
      LMAPI->close_file(infile);
      return HOOK_RESULT_STOP;
    }
}

HOOK_HANDLER(hook_presend_check_password)
{
    char buffer[BIG_BUF], outfilename[BIG_BUF];
    FILE *infile, *outfile;
    int gotpass;

    LMAPI->log_printf(9,"In the password check...\n");

    if (!LMAPI->get_var("post-password")) return HOOK_RESULT_OK;

    gotpass = 0;

    LMAPI->buffer_printf(outfilename, sizeof(outfilename) - 1, "%s.nukepass", LMAPI->get_string("queuefile"));

    if (!(infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r"))) {
       LMAPI->filesys_error(LMAPI->get_string("queuefile"));
       return HOOK_RESULT_FAIL;
    }

    if (!(outfile = LMAPI->open_file(outfilename,"w"))) {
       LMAPI->close_file(infile);
       LMAPI->filesys_error(outfilename);
       return HOOK_RESULT_FAIL;
    }

    while(LMAPI->read_file(buffer, sizeof(buffer), infile)) {
       if (!strncasecmp(buffer,"X-posting-pass:",15) ||
           !strncasecmp(buffer,"Password:",9)) {
          const char *pass;
          char *compare;

          pass = LMAPI->get_string("post-password");

          LMAPI->log_printf(5,"Checking posting password...\n");

          if (!strncasecmp(buffer,"X-posting-pass:",15))
             compare = &buffer[16];
          else /* 'Password:' */
             compare = &buffer[10];

          if (strncmp(compare,pass,strlen(pass))) {
             buffer[strlen(buffer) - 1] = 0;
             LMAPI->log_printf(1, "Invalid password '%s' for list '%s'\n",
                    &buffer[15], LMAPI->get_string("list"));
             LMAPI->close_file(outfile);
             LMAPI->close_file(infile);
             LMAPI->unlink_file(outfilename);
             if (LMAPI->get_bool("password-failure-blackhole")) {
                LMAPI->result_printf("Invalid password for posting to list '%s'.\n",
                    LMAPI->get_string("list"));
             } else {
                LMAPI->make_moderated_post("Incorrect password on password-restricted list.");
             }
             return HOOK_RESULT_STOP;
          } else {
             gotpass = 1;
          }
       } else {
          LMAPI->write_file(outfile,"%s",buffer);
       }
    }
    LMAPI->close_file(infile);
    LMAPI->close_file(outfile);

    if (!gotpass) {
        const char *nopassfile = LMAPI->get_var("post-password-reject-file");
        int usedefault = 1;

        if (LMAPI->get_bool("password-failure-blackhole")) {
           if (nopassfile) {
               char inbuffer[BIG_BUF];
               LMAPI->set_var("task-form-subject", "Posting rejected.", VAR_TEMP);
               LMAPI->listdir_file(inbuffer, LMAPI->get_string("list"),
                                   nopassfile);
               usedefault = !LMAPI->send_textfile_expand(LMAPI->get_string("fromaddress"),inbuffer);
               LMAPI->clean_var("task-form-subject", VAR_TEMP);
           }

           if (usedefault) {
               LMAPI->result_printf("Sorry, you did not provide an X-posting-pass ");
               LMAPI->result_printf("for list '%s'.\n", LMAPI->get_string("list"));
           }

           LMAPI->unlink_file(outfilename);
        } else {
           LMAPI->make_moderated_post("Post submitted to moderated list.");
        }
        return HOOK_RESULT_STOP;
    } else {
       LMAPI->log_printf(3,"Posting validated for list '%s'.\n",
           LMAPI->get_string("list"));
       LMAPI->replace_file(outfilename,LMAPI->get_string("queuefile"));
       if (LMAPI->get_bool("password-implies-approved"))
          LMAPI->set_var("moderated-approved-by",
            LMAPI->get_string("realsender"), VAR_GLOBAL);
       return HOOK_RESULT_OK;
    }
}

HOOK_HANDLER(hook_presend_check_overquoting)
{
    int quotepercent, quotelines;
    int totallines;
    int quotedlines;
    int realquotedlines, tolerancelines;
    int inbody, resetlines;
    char buffer[BIG_BUF];
    FILE *infile;

    LMAPI->log_printf(9,"Overquoting check...\n");

    if (!LMAPI->get_bool("quoting-limits"))
       return HOOK_RESULT_OK;
    if (LMAPI->get_var("moderated-approved-by"))
       return HOOK_RESULT_OK;

    quotepercent = LMAPI->get_number("quoting-max-percent");
    quotelines = LMAPI->get_number("quoting-max-lines");
    tolerancelines = LMAPI->get_number("quoting-tolerance-lines");

    /* We can't do anything if they didn't set limits */
    if (!quotepercent && !quotelines)
       return HOOK_RESULT_OK;

    resetlines = LMAPI->get_bool("quoting-line-reset");

    if ((infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) ==
        NULL) {
       LMAPI->filesys_error(LMAPI->get_string("queuefile"));
       return HOOK_RESULT_FAIL;        
    }

    totallines = 0; quotedlines = 0; realquotedlines = 0;

    inbody = 0;

    while(LMAPI->read_file(buffer, sizeof(buffer), infile)) {
       if (inbody) totallines++;
       if (buffer[0] == '\n') inbody = 1;
       if (buffer[0] == '>') {
          /* We don't count sendmail escaping against them */
          if (strncmp(&buffer[1],"From ",5)) {
             quotedlines++;
             realquotedlines++;
          }
       } else
       if (buffer[0] == '|') {
          quotedlines++;
          realquotedlines++;
       } else
       if (resetlines && quotelines) {
          /* If we reset after a block, AND if we haven't exceeded our
             maximum already... */
          if (quotedlines <= quotelines)
             quotedlines = 0;
       }
    }

    LMAPI->close_file(infile);

    LMAPI->log_printf(9,"Overquoting: %d %d %d (%d or %d%% max)\n",
       totallines, realquotedlines, quotedlines, quotelines,
       quotepercent);

    if (quotepercent && (tolerancelines ? totallines > tolerancelines : 1)) {
       int percent;

       percent = (realquotedlines * 100) / totallines;
       LMAPI->log_printf(9,"Overquoting: %d percent (%d%% max)\n",
         percent, quotepercent);

       if (percent > quotepercent) {
          char failure[BIG_BUF];
          char overquotefile[BIG_BUF];

          LMAPI->buffer_printf(failure, sizeof(failure) - 1,
            "%d%% of message quoted (%d%% max allowed)",
               percent, quotepercent);

          LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "Overquoting: %s",
               failure);

          LMAPI->set_var("overquoting-reason",failure,VAR_TEMP);
          LMAPI->listdir_file(overquotefile,LMAPI->get_string("list"),
             LMAPI->get_string("overquote-file"));

          /* We want the user to know their post was held because of
             overquoting. */
          LMAPI->set_var("moderate-force-notify","yes",VAR_TEMP);

          if (LMAPI->send_textfile_expand(LMAPI->get_string("realsender"),
                overquotefile))
             LMAPI->set_var("moderate-quiet","yes",VAR_TEMP);
          LMAPI->make_moderated_post(buffer);
          LMAPI->clean_var("moderate-quiet",VAR_TEMP);
          LMAPI->clean_var("overquoting-reason",VAR_TEMP);
          LMAPI->clean_var("moderate-force-notify",VAR_TEMP);
          return HOOK_RESULT_STOP;
       }
    }

    if (quotelines) {
       if (quotedlines > quotelines) {
          char failure[BIG_BUF];
          char overquotefile[BIG_BUF];

          LMAPI->buffer_printf(failure, sizeof(failure) - 1,
            "%d lines quoted%s (%d max allowed)",
               quotedlines, resetlines ? " in a block" : "", quotelines);

          LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "Overquoting: %s",
               failure);

          /* We want the user to know their post was held because of
             overquoting. */
          LMAPI->set_var("moderate-force-notify","yes",VAR_TEMP);

          LMAPI->set_var("overquoting-reason",failure,VAR_TEMP);
          LMAPI->listdir_file(overquotefile,LMAPI->get_string("list"),
             LMAPI->get_string("overquote-file"));
          if (LMAPI->send_textfile_expand(LMAPI->get_string("realsender"),
                overquotefile))
             LMAPI->set_var("moderate-quiet","yes",VAR_TEMP);
          LMAPI->make_moderated_post(buffer);
          LMAPI->clean_var("moderate-quiet",VAR_TEMP);
          LMAPI->clean_var("overquoting-reason",VAR_TEMP);
          LMAPI->clean_var("moderate-force-notify",VAR_TEMP);
          return HOOK_RESULT_STOP;
       }
    }

    return HOOK_RESULT_OK;
}

HOOK_HANDLER(hook_presend_check_bcc)
{
    FILE *queuefile;
    char inputbuf[BIG_BUF];
    int  lastvalid;
    int  gotaddress;
    const char *enforceto;

    enforceto = LMAPI->get_var("enforced-addressing-to");

    if (!enforceto)
       return HOOK_RESULT_OK;
    if (LMAPI->get_var("moderated-approved-by"))
       return HOOK_RESULT_OK;

    queuefile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r");

    if (queuefile == NULL) {
       LMAPI->filesys_error(LMAPI->get_string("queuefile"));
       return HOOK_RESULT_FAIL;
    }

    gotaddress = 0;
    lastvalid = 0;

    while(LMAPI->read_file(inputbuf, sizeof(inputbuf), queuefile) && !gotaddress) {
       if (inputbuf[0] == '\n') break;

       if (isspace((int)(inputbuf[0])) && lastvalid) {
          if (LMAPI->strcasestr(inputbuf, enforceto)) gotaddress = 1;
       } else lastvalid = 0;

       if (strncasecmp(inputbuf,"To:",3) == 0) {
          lastvalid = 1;
          if (LMAPI->strcasestr(inputbuf, enforceto)) gotaddress = 1;
       }
       else if (strncasecmp(inputbuf,"Cc:",3) == 0) {
          lastvalid = 1;
          if (LMAPI->strcasestr(inputbuf, enforceto)) gotaddress = 1;
       }
    }

    LMAPI->close_file(queuefile);

    if (gotaddress) return HOOK_RESULT_OK;

    if (LMAPI->get_var("enforced-address-blackhole"))
       return HOOK_RESULT_STOP;

    LMAPI->make_moderated_post("The list address was not in the To or Cc fields.  This list does not\naccept Bcc'd posts.");

    return HOOK_RESULT_STOP;
}

