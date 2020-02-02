#include <stdio.h>
#include "pantomime.h"

struct LPMAPI *LMAPI;

void pantomime_switch_context(void)
{
   LMAPI->log_printf(15, "Switching context in module PantoMIME\n");
}

int pantomime_upgrade(int prev, int cur)
{
   LMAPI->log_printf(10, "Upgrading module PantoMIME\n");
   return 1;
}

int pantomime_upgradelist(int prev, int cur)
{
   LMAPI->log_printf(10, "Upgrading lists in module PantoMIME\n");
   return 1;
}

void pantomime_init(void)
{
   LMAPI->log_printf(10, "Initializing module PantoMIME\n");
}

void pantomime_unload(void)
{
   LMAPI->log_printf(10, "Unloading module PantoMIME\n");
}

void pantomime_load(struct LPMAPI* api)
{
   LMAPI = api;

   /* Log the module initialization */
   LMAPI->log_printf(10, "Loading module PantoMIME\n");

   /* Register us as a module */
   LMAPI->add_module("PantoMIME","Collection of MIME attachment handlers");

   LMAPI->register_var("humanize-html","yes","MIME",
     "Should HTML attachments be converted to plaintext",
     "humanize-html = no", VAR_BOOL, VAR_ALL);

   LMAPI->register_var("pantomime-dir",NULL,"MIME",
     "Directory on disk to store binary files placed on the web via PantoMIME.",
     "pantomime-dir = /var/www/ecartis/html/pantomime", VAR_STRING, VAR_ALL);

   LMAPI->register_var("pantomime-url",NULL,"MIME",
     "URL corresponding to pantomime-dir",
     "pantomime-url = http://www.ecartis.org/pantomime", VAR_STRING, VAR_ALL);

   LMAPI->register_var("unmime-forceweb","no","MIME",
     "Should all attachments (even text/plain) be forced to the web (pantomime-dir and pantomime-url must be set or all will be eaten)",
     "unmime-forceweb = yes", VAR_BOOL, VAR_ALL);

   LMAPI->add_mime_handler("text/html", 100, mimehandle_html);
   LMAPI->add_mime_handler("image/.*", 100, mimehandle_pantomime_binary);

   LMAPI->add_mime_handler("ecartis-internal/pantomime",1,mimehandle_pantomime_binary);
  
   /* Legacy support */
   LMAPI->add_mime_handler("listar-internal/pantomime",1,mimehandle_pantomime_binary);
}
