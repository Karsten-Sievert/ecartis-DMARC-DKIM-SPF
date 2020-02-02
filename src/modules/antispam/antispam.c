#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#ifndef WIN32
#include <sys/types.h>
#include <sys/time.h>
#endif

#include "lpm.h"

struct LPMAPI *LMAPI;

/* Antispam processor */
HOOK_HANDLER(hook_presend_antispam)
{
   char buffer[BIG_BUF], regbuf[BIG_BUF], tbuf[BIG_BUF], matchedbuf[BIG_BUF];
   FILE *infile, *regfile;
   int done, matched;

   LMAPI->log_printf(15,"In hook_send_antispam\n");

   /* I don't know WHY you'd ever want to do this, but here it is... */
   if (LMAPI->get_bool("allow-spam")) {
     LMAPI->log_printf(9,"AntiSPAM: Spam allowed, not checking spamfile.\n");
     return HOOK_RESULT_OK;
   }

   /* Moderated messages are immune to spam-checks. */
   if (LMAPI->get_var("moderated-approved-by"))
     return HOOK_RESULT_OK;

   /* Sanity check */
   if (!LMAPI->get_var("spamfile")) {
     LMAPI->log_printf(9,"AntiSPAM: No spamfile set.\n");
     return HOOK_RESULT_OK;
   }

   LMAPI->listdir_file(buffer,LMAPI->get_string("list"),
      LMAPI->get_string("spamfile"));

   /* Do we HAVE a spamfile? */
   if (!LMAPI->exists_file(buffer)) {
      return HOOK_RESULT_OK;
   }

   LMAPI->log_printf(15,"Trying to open '%s'\n", buffer);
   if ((regfile = LMAPI->open_file(buffer,"r")) == NULL) {
      LMAPI->log_printf(9,"AntiSPAM: Unable to open spamfile.\n");
      return HOOK_RESULT_OK;
   }

   LMAPI->log_printf(15,"Opened '%s'\n", buffer);

   if ((infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) == NULL) {
      LMAPI->log_printf(9,"AntiSPAM: Unable to open queuefile.\n");
      LMAPI->close_file(regfile);
      return HOOK_RESULT_OK;
   }

   done = 0; matched = 0;

   /* Read message... */
   while(LMAPI->read_file(buffer, sizeof(buffer), infile) && !done) {

      /* If we don't need to check the message body, stop after the header. */
      if((buffer[0] == '\n') && !(LMAPI->get_bool("antispam-checkbody")))
	 done = 1;

      if (!done) {
         buffer[strlen(buffer) - 1] = 0;
         LMAPI->rewind_file(regfile);

         /* Read spam regex's and try to match. */
         while(LMAPI->read_file(regbuf, sizeof(regbuf), regfile) && !matched) {
            if(regbuf[0] == '#') continue;  /* skip comments */
            regbuf[strlen(regbuf) - 1] = 0;  /* strip newline */
            LMAPI->log_printf(9,"AntiSPAM: Checking %s vs %s\n", buffer,regbuf);
            if (LMAPI->match_reg(regbuf,buffer)) {
               FILE *spamlog;

               /* Log to spammers file */
               LMAPI->listdir_file(tbuf,LMAPI->get_string("list"),"spammers.log");
               if ((spamlog = LMAPI->open_file(tbuf,"a")) != NULL) {
                  char datebuf[81];
                  time_t now = time(NULL);
                  struct tm *tm_now;
                  tm_now = gmtime(&now);
                  strftime(datebuf, sizeof(datebuf) - 1, "%a, %d %b %Y %H:%M:%S", tm_now);
                  
                  LMAPI->write_file(spamlog,"%s:\n", LMAPI->get_string("realsender"));
                  LMAPI->write_file(spamlog, "\tPattern   : %s\n", regbuf);
                  LMAPI->write_file(spamlog, "\tMatched   : %s\n", buffer);
                  LMAPI->write_file(spamlog, "\tDate (GMT): %s\n\n", datebuf);
                  LMAPI->close_file(spamlog);
               }

               if (LMAPI->get_bool("antispam-blackhole")) {
                  LMAPI->log_printf(1,"AntiSPAM: Spam from %s eaten.\n",
                    LMAPI->get_string("realsender"));
                  matched = 1; done = 1;
               } else {
                  /* Log spam... */
                  LMAPI->log_printf(1, "AntiSPAM: Spam from %s sent to moderator for list %s.\n",
                      LMAPI->get_string("realsender"),LMAPI->get_string("list"));
                  matched = 1; done = 1;

                  /* Moderate post, in case. */
                  LMAPI->buffer_printf(matchedbuf, sizeof(matchedbuf) - 1, "Post failed antispam check on rule:\n%s",
                          regbuf);
                  if (LMAPI->get_bool("no-spam-return"))
                     LMAPI->set_var("moderate-quiet","yes",VAR_TEMP);
                  LMAPI->make_moderated_post(matchedbuf);
                  LMAPI->clean_var("moderate-quiet",VAR_TEMP);
               }
            }
         }         
      }
   }

   LMAPI->close_file(regfile);  LMAPI->close_file(infile);
   
   if (matched) return HOOK_RESULT_STOP; else return HOOK_RESULT_OK;
}

/* Module load call */
void antispam_load(struct LPMAPI *api)
{
   LMAPI = api;

   /* Log module init */
   LMAPI->log_printf(10, "Loading module AntiSPAM\n");

   /* Add us to the 'modules' list. */
   LMAPI->add_module("AntiSPAM","Anti Spam blocker module");

   /* File definitions */
   LMAPI->add_file("spamfile","spamfile", "File of regexps on a per-list basis for blocking spam.");

   /* Hook definitions */
   LMAPI->add_hook("PRESEND", 29, hook_presend_antispam);

   /* Variable registration */
   LMAPI->register_var("allow-spam", "no", "AntiSpam",
                       "Should we disable the antispam check for this list.",
                       "allow-spam = false", VAR_BOOL, VAR_ALL);
   LMAPI->register_var("antispam-blackhole", "no", "AntiSpam",
                       "If we receive spam, should we simply eat it?  (If 'no', then it is moderated.)",
                       "antispam-blackhole = yes", VAR_BOOL, VAR_ALL);
   LMAPI->register_var("spamfile", "spam-regexp", "AntiSpam",
                       "The file on disk which contains the regular expressions used to detect if a given sender is a spammer.",
                       "spamfile = spam-regexp", VAR_STRING, VAR_ALL);
   LMAPI->register_var("antispam-checkbody", "no", "AntiSpam",
		       "Should the antispam module also check the message body.",
		       "antispam-checkbody = no", VAR_BOOL, VAR_ALL);
}

void antispam_switch_context(void)
{
   LMAPI->log_printf(15, "Switching context in module AntiSPAM\n");
}

int antispam_upgradelist(int prev, int cur)
{
   LMAPI->log_printf(10, "Upgrading lists in module AntiSPAM\n");
   return 1;
}

int antispam_upgrade(int prev, int cur)
{
   LMAPI->log_printf(10, "Upgrading module AntiSPAM\n");
   return 1;
}

void antispam_init(void)
{
   LMAPI->log_printf(10, "Initializing module AntiSPAM\n");
}

void antispam_unload(void)
{
   LMAPI->log_printf(10, "Unloading module AntiSPAM\n");
}

