#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "lpm.h"
#include "stat-mod.h"

CMD_HANDLER(cmd_faq)
{
   struct list_user user;
   const char *list;
   char namebuf[BIG_BUF];

   if (!LMAPI->get_var("faq-file")) {
      LMAPI->spit_status("No FAQ file configured.");
      return CMD_RESULT_CONTINUE;
   }

   list = NULL;

   if (params->num == 0) {
      if ((list = LMAPI->get_var("list")) == NULL) {
         LMAPI->spit_status("No list in current context.");
         return CMD_RESULT_CONTINUE;
      }
   } else if (LMAPI->get_var("adminmode")) {
      LMAPI->spit_status("'faq' cannot take parameters in admin mode.");
      return CMD_RESULT_CONTINUE;
   } else if (params->num > 1) {
      LMAPI->spit_status("'faq' wants at most one parameter.");
      return CMD_RESULT_CONTINUE;
   } else {
      list = params->words[0];
   }   

   if (!LMAPI->set_context_list(list)) {
      LMAPI->nosuch(list);
      return CMD_RESULT_CONTINUE;
   }
  
   if (!LMAPI->user_find_list(list,LMAPI->get_string("fromaddress"),&user)
      && !LMAPI->get_var("adminmode")) {
      LMAPI->spit_status("Not a member of that list.");
      return CMD_RESULT_CONTINUE;
   }

   LMAPI->buffer_printf(namebuf, sizeof(namebuf) - 1, "FAQ file for list '%s'", list);
   LMAPI->set_var("task-form-subject", namebuf, VAR_TEMP); 

   LMAPI->listdir_file(namebuf, list, LMAPI->get_string("faq-file"));

   if (!LMAPI->send_textfile_expand(LMAPI->get_string("fromaddress"),
        namebuf)) {
      LMAPI->spit_status("No FAQ file for list.");
   } else {
      LMAPI->spit_status("FAQ file sent.");
   }
   LMAPI->clean_var("task-form-subject", VAR_TEMP);
   return CMD_RESULT_CONTINUE;
}


CMD_HANDLER(cmd_info)
{
   const char *list;
   char namebuf[BIG_BUF];

   if (!LMAPI->get_var("info-file")) {
      LMAPI->spit_status("No info file configured.");
      return CMD_RESULT_CONTINUE;
   }

   list = NULL;

   if (params->num == 0) {
      if ((list = LMAPI->get_var("list")) == NULL) {
         LMAPI->spit_status("No list in current context.");
         return CMD_RESULT_CONTINUE;
      }
   } else if (LMAPI->get_var("adminmode")) {
      LMAPI->spit_status("'info cannot take parameters in admin mode.");
      return CMD_RESULT_CONTINUE;
   } else if (params->num > 1) {
      LMAPI->spit_status("'info' wants at most one parameter.");
      return CMD_RESULT_CONTINUE;
   } else {
      list = params->words[0];
   }   

   if (!LMAPI->set_context_list(list)) {
      LMAPI->nosuch(list);
      return CMD_RESULT_CONTINUE;
   }
  
   LMAPI->buffer_printf(namebuf, sizeof(namebuf) - 1, "Info file for list '%s'", list);
   LMAPI->set_var("task-form-subject", namebuf, VAR_TEMP); 

   LMAPI->listdir_file(namebuf, list, LMAPI->get_string("info-file"));

   if (!LMAPI->send_textfile_expand(LMAPI->get_string("fromaddress"),
        namebuf)) {
      LMAPI->spit_status("No info file for list.");
   } else {
      LMAPI->spit_status("Info file sent.");
   }
   LMAPI->clean_var("task-form-subject", VAR_TEMP);
   return CMD_RESULT_CONTINUE;
}

