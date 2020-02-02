#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lpm.h"

struct LPMAPI *LMAPI;

void create_default_regexps(void)
{
   FILE *exps;
   char buffer[BIG_BUF];

   LMAPI->listdir_file(buffer,LMAPI->get_string("list"),
      LMAPI->get_string("administrivia-regexp-file"));

   /* Open the regexp file.  If it doesn't exist, try to create it*/
   exps = LMAPI->open_file(buffer, "w");

   if(exps == NULL) return;

   LMAPI->write_file(exps, "# Stock commands (and close variants) at start of line \n");
   LMAPI->write_file(exps, "^subscribe .*$\n");
   LMAPI->write_file(exps, "^unsubscribe .*$\n");
   LMAPI->write_file(exps, "^unsub .*$\n");
   LMAPI->write_file(exps, "^un-sub .*$\n");
   LMAPI->write_file(exps, "^subscribe$\n");
   LMAPI->write_file(exps, "^unsubscribe$\n");
   LMAPI->write_file(exps, "^unsub$\n");
   LMAPI->write_file(exps, "^un-sub$\n");
   LMAPI->write_file(exps, "# Some common ones in the middle of text\n");
   LMAPI->write_file(exps, "^.* subscribe .*$\n");
   LMAPI->write_file(exps, "^.* unsubscribe .*$\n");
   LMAPI->write_file(exps, "^.* unsub .*$\n");
   LMAPI->write_file(exps, "^.* un-sub .*$\n");
   LMAPI->write_file(exps, "^.* take me off .*$\n");
   LMAPI->write_file(exps, "^.* get me off .*$\n");
   LMAPI->close_file(exps);
}

/* Simple reusable check */
int administrivia_check_line(FILE *f, const char *line)
{
   char buffer[BIG_BUF];

   while(LMAPI->read_file(buffer, sizeof(buffer), f)) {
       if(buffer[0] == '#') continue; /* skip comments */
       if(buffer[strlen(buffer)-1] == '\n')
           buffer[strlen(buffer)-1] = '\0';  /* strip the newline */
       if(LMAPI->match_reg(buffer, line)) {
           LMAPI->set_var("administrivia-match-pat", buffer, VAR_TEMP);
           return 1;
       }
   }
   return 0;
}

/* Administrivia check */
HOOK_HANDLER(hook_presend_check_administrivia)
{
   char buffer[BIG_BUF];
   FILE *infile;
   FILE *exps;
   struct list_user user;
   int inbody, numlines, count = 0;

   numlines = 0; inbody = 0;

   /* Allow a variable to turn off administrivia checking */
   if (!LMAPI->get_bool("administrivia-check")) return HOOK_RESULT_OK;

   /* Moderated messages are immune to administrivia check */
   if (LMAPI->get_var("moderated-approved-by")) return HOOK_RESULT_OK;

   LMAPI->log_printf(9,"Entering administrivia check.\n");

   LMAPI->listdir_file(buffer,LMAPI->get_string("list"),
           LMAPI->get_string("administrivia-regexp-file"));

   /* Open the regexp file.  If it doesn't exist, try to create it*/
   exps = LMAPI->open_file(buffer, "r");

   if(exps == NULL) {
      create_default_regexps();
      /* If we still cannot open it, bail */
      exps = LMAPI->open_file(buffer, "r");
      if(exps == NULL)
         return HOOK_RESULT_OK;
   }

   /* Is this a subscriber? */
   if (LMAPI->user_find_list(LMAPI->get_string("list"),
       LMAPI->get_string("fromaddress"), &user)) {

      /* Admins are immune to administrivia check */

      if (LMAPI->user_hasflag(&user,"ADMIN")) {
         LMAPI->log_printf(9,"User is admin, ignoring...\n");
         LMAPI->close_file(exps);
         return HOOK_RESULT_OK;
      }
   }

   /* Just in case.  This should never happen, but... */
   if ((infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) == NULL) {
      LMAPI->log_printf(5,"Administrivia unable to open queuefile.\n");

      /* Fail and notify mailserver to requeue */
      LMAPI->close_file(exps);
      return HOOK_RESULT_FAIL;
   }

   LMAPI->rewind_file(exps);
   /* Scan the file... */
   while(!inbody && LMAPI->read_file(buffer, sizeof(buffer), infile)) {
      if (buffer[0] == '\n') inbody = 1;
      if(buffer[strlen(buffer)-1] == '\n')
          buffer[strlen(buffer)-1] = '\0';  /* strip the newline */
      if (strncmp(buffer,"Subject:",8) == 0) {
         char *tempbuf;

         /* Convert buffer to lowercase... */
         tempbuf = LMAPI->lowerstr(&buffer[9]);
         LMAPI->clean_var("administrivia-match-pat", VAR_TEMP);

         /* ...and check for administrivia */
         if (administrivia_check_line(exps, tempbuf)) {
            char inputbuf[BIG_BUF];

            free(tempbuf);
            LMAPI->close_file(exps);
            LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "Failed administrivia check on pattern '%s'",
                    LMAPI->get_string("administrivia-match-pat"));
            LMAPI->set_var("moderate-include-queue","no",VAR_TEMP);
            LMAPI->make_moderated_post(buffer);
            LMAPI->clean_var("moderate-include-queue",VAR_TEMP);
            LMAPI->set_var("results-subject-override","Post rejected by administrivia.",VAR_TEMP);
            LMAPI->result_printf("Your post appears to contain a valid %s command as the\n", SERVICE_NAME_MC);
            LMAPI->result_printf("subject, or one of the first few lines. The post has\n");
            LMAPI->result_printf("been forwarded to the list administrator in case it\n");
            LMAPI->result_printf("is a real post.  If you were trying to issue a command\n");
            LMAPI->result_printf("send it to %s instead.\n", LMAPI->get_string("listserver-address"));
            LMAPI->result_printf("\nMatched pattern: %s\n",
              LMAPI->get_string("administrivia-match-pat"));
            LMAPI->result_printf("\n-- Original message --\n");
            LMAPI->rewind_file(infile);
            while(LMAPI->read_file(inputbuf, sizeof(inputbuf), infile)) {
               LMAPI->result_printf("%s",inputbuf);
            }
            LMAPI->close_file(infile);
            return HOOK_RESULT_STOP;
         }
         free(tempbuf);
      }
   }

   count = LMAPI->get_number("administrivia-body-lines");

   /* Read the first few lines */
   while(count && LMAPI->read_file(buffer, sizeof(buffer), infile)) {
      if(count > 0) count--;
      LMAPI->clean_var("administrivia-match-pat", VAR_TEMP);
      LMAPI->rewind_file(exps);

      if(buffer[strlen(buffer)-1] == '\n')
          buffer[strlen(buffer)-1] = '\0';  /* strip the newline */

      /* Check for administrivia */
      if (administrivia_check_line(exps, buffer)) {
         char inputbuf[BIG_BUF];

         LMAPI->close_file(exps);
         LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "Failed administrivia check on pattern '%s'",
                 LMAPI->get_string("administrivia-match-pat"));
         LMAPI->set_var("moderate-include-queue","no",VAR_TEMP);
         LMAPI->make_moderated_post(buffer);
         LMAPI->clean_var("moderate-include-queue",VAR_TEMP);
         LMAPI->result_printf("Your post appears to contain a valid %s command as the\n", SERVICE_NAME_MC);
         LMAPI->result_printf("subject, or one of the first few lines. The post has\n");
         LMAPI->result_printf("been forwarded to the list administrator in case it\n");
         LMAPI->result_printf("is a real post.  If you were trying to issue a command\n");
         LMAPI->result_printf("send it to %s instead.\n", LMAPI->get_string("listserver-address"));
         LMAPI->result_printf("\nMatched pattern: %s\n",
           LMAPI->get_string("administrivia-match-pat"));
         LMAPI->result_printf("\n-- Original message --\n");
         LMAPI->rewind_file(infile);
         while(LMAPI->read_file(inputbuf, sizeof(inputbuf), infile)) {
            LMAPI->result_printf("%s",inputbuf);
         }
         LMAPI->close_file(infile);
         return HOOK_RESULT_STOP;
      }
   }

   LMAPI->close_file(infile);
   LMAPI->close_file(exps);
   return HOOK_RESULT_OK;
}

void administrivia_switch_context(void)
{
   LMAPI->log_printf(15, "Switching context in module Administrivia.\n");
}

int administrivia_upgradelist(int prev, int cur)
{
   LMAPI->log_printf(10, "Upgrading lists in module Administrivia.\n");
   return 1;
}

int administrivia_upgrade(int prev, int cur)
{
   LMAPI->log_printf(10, "Upgrading module Administrivia.\n");
   return 1;
}

void administrivia_unload(void)
{
   LMAPI->log_printf(10, "Unloading module Administrivia.\n");
}

void administrivia_init(void)
{
   LMAPI->log_printf(10, "Initializing module Administrivia.\n");
}


/* Load the module */
void administrivia_load(struct LPMAPI *API)
{
   LMAPI = API;

   LMAPI->log_printf(10, "Loading module Administrivia.\n");

   /* Add us to the module list */
   LMAPI->add_module("Administrivia","Basic administrivia module.");

   /* Hook definitions */
   LMAPI->add_hook("PRESEND",55,hook_presend_check_administrivia);

   /* Add a file for administrivia check regexps */
   LMAPI->add_file("administrivia-regexp", "administrivia-regexp-file",
                   "File containing regexps used to detect if a user is sending list commands to the list instead of the request address, and bounce those messages to the moderator for handling.");

   /* Register variable */
   LMAPI->register_var("administrivia-match-pat", NULL, NULL, NULL, NULL,
                       VAR_STRING, VAR_TEMP|VAR_INTERNAL);
   LMAPI->register_var("administrivia-regexp-file", "admin-regexp",
                       "Administrivia",
                       "File containing regexps used to detect if a user is sending list commands to the list instead of the request address, and bounce those messages to the moderator for handling.",
                       "administrivia-regexp-file = admin-regexp",
                       VAR_STRING, VAR_ALL);
   LMAPI->register_var("administrivia-body-lines", "6", "Administrivia",
                       "How many lines of the body to check for commands.",
                       "administrivia-body-lines = -1", VAR_INT, VAR_ALL);
   LMAPI->register_var("administrivia-check", "true", "Administrivia",
                       "Should the administrivia check be enabled.",
                       "administrivia-check = false", VAR_BOOL, VAR_ALL);
}
