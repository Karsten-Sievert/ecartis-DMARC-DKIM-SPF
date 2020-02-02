#include <stdio.h>

#include "toolbox.h"

struct LPMAPI *LMAPI;

void toolbox_switch_context(void)
{
   LMAPI->log_printf(15, "Switching context in module Toolbox\n");
}

int toolbox_upgradelist(int prev, int cur)
{
   LMAPI->log_printf(10, "Upgrading lists in module Toolbox\n");
   return 1;
}

int toolbox_upgrade(int prev, int cur)
{
   LMAPI->log_printf(10, "Upgrading module Toolbox\n");
   return 1;
}

void toolbox_unload(void)
{
   LMAPI->log_printf(10, "Unloading module Toolbox\n");
}

void toolbox_init(void)
{
   LMAPI->log_printf(10, "Initializing module Toolbox\n");
}

void toolbox_load(struct LPMAPI* api)
{
   LMAPI = api;

   /* Log the module initialization */
   LMAPI->log_printf(10, "Loading module Toolbox\n");

   /* Register us as a modules */
   LMAPI->add_module("Toolbox", "Generic maintainance toolbox module");

   /* Register variables */
   LMAPI->register_var("newlist-qmail", "no", "Maintenance", 
     "When creating a new list, do we need to make dot-qmail aliases?",
     "newlist-qmail = no", VAR_BOOL, VAR_GLOBAL);
   LMAPI->register_var("listserver-bin-dir", NULL, "Maintenance",
     "When creating a new list, what directory do we prepend to the binary name when we make the aliases (if not set, defaults to the path the binary was run with).",
     "listserver-bin-dir = /home/list", VAR_STRING, VAR_GLOBAL);
   LMAPI->register_var("newlist-admin", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_GLOBAL|VAR_INTERNAL);

   /* Cmd line args */
   LMAPI->add_cmdarg("-checkusers", 1, NULL, cmdarg_checkusers);
   LMAPI->add_cmdarg("-newlist", 1, "<listname>", cmdarg_newlist);
   LMAPI->add_cmdarg("-qmail", 0, NULL, cmdarg_qmail);
   LMAPI->add_cmdarg("-freshen", 1, "<listname>", cmdarg_freshen);
   LMAPI->add_cmdarg("-admin",1,"<e-mail>",cmdarg_admin);
   LMAPI->add_cmdarg("-buildaliases",0,NULL,cmdarg_buildaliases);

   /* Mode of operation */
   LMAPI->add_mode("checkuser", mode_checkusers);
   LMAPI->add_mode("newlist", mode_newlist);
   LMAPI->add_mode("freshen", mode_freshen);
   LMAPI->add_mode("buildaliases", mode_buildaliases);
}
