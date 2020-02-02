#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lpm.h"

struct LPMAPI *LMAPI;

HOOK_HANDLER(hook_peruser_to)
{
   FILE *infile;
   FILE *outfile;
   const char *destaddy;
   const char *infilename;
   char buffer[BIG_BUF];
   char outfilename[BIG_BUF];
   int inbody;

   if (!LMAPI->get_bool("per-user-rewrite-to"))
      return HOOK_RESULT_OK;

   destaddy = LMAPI->get_string("per-user-address");
   infilename = LMAPI->get_string("per-user-queuefile");
   LMAPI->buffer_printf(outfilename, sizeof(outfilename) - 1, "%s.tochange",
        infilename);

   if ((outfile = LMAPI->open_file(outfilename, "w")) == NULL) {
      LMAPI->filesys_error(outfilename);
      return HOOK_RESULT_FAIL;
   }

   if ((infile = LMAPI->open_file(infilename, "r")) == NULL) {
      LMAPI->close_file(outfile);
      LMAPI->unlink_file(outfilename);
      LMAPI->filesys_error(infilename);
      return HOOK_RESULT_FAIL;
   }

   inbody = 0;

   while(LMAPI->read_file(buffer, sizeof(buffer), infile)) {
      if (!inbody) {
         if (strncasecmp(buffer, "To: ", 4) == 0) {
            LMAPI->write_file(outfile, "To: %s\n", destaddy);            
         } 
         else {
            LMAPI->write_file(outfile, "%s", buffer);
         }
      }
      else {
         if (buffer[0] == '\n')
            inbody = 1;

         LMAPI->write_file(outfile, "%s", buffer);
      }
   }

   LMAPI->close_file(infile);
   LMAPI->close_file(outfile);

   LMAPI->replace_file(outfilename, infilename);

   return HOOK_RESULT_OK;
}

HOOK_HANDLER(hook_peruser_footers)
{
   char filename[BIG_BUF];

   LMAPI->listdir_file(filename,
            LMAPI->get_string("list"),
            LMAPI->get_string("per-user-footer-file"));

   if (!LMAPI->exists_file(filename))
      return HOOK_RESULT_OK;

   LMAPI->expand_append(LMAPI->get_string("per-user-queuefile"),
      filename);

   return HOOK_RESULT_OK;   
}

/* Module load */
void peruser_load(struct LPMAPI *api)
{
    LMAPI = api;

    /* Log module startup */
    LMAPI->log_printf(10, "Loading module PerUser\n");

    /* Hook definitions */
    LMAPI->add_hook("PER-USER", 10, hook_peruser_to);
    LMAPI->add_hook("PER-USER", 20, hook_peruser_footers);

    LMAPI->register_var("per-user-footer-file", "text/peruser-footer.txt",
       "Files", "Filename of the per-user footer file.", 
       "per-user-footer-file = per-user/footer.txt", VAR_STRING,
       VAR_ALL);
    LMAPI->register_var("per-user-rewrite-to", "false", "Per-User",
       "Should the To: field be rewritten for each user on the list?",
       "per-user-rewrite-to = true", VAR_BOOL, VAR_ALL);

    LMAPI->add_file("per-user-footer", "per-user-footer-file", 
       "The footer expanded and added separately for each user in per-user send mode.");
}

void peruser_switch_context(void)
{
    LMAPI->log_printf(15, "Switching context in module PerUser\n");
}

int peruser_upgradelist(int prev, int cur)
{
    LMAPI->log_printf(10, "Upgrading lists in module PerUser\n");
    return 1;
}

int peruser_upgrade(int prev, int cur)
{
    LMAPI->log_printf(10, "Upgrading module PerUser\n");
    return 1;
}

void peruser_init(void)
{
    LMAPI->log_printf(10, "Initializing module PerUser\n");
}

void peruser_unload(void)
{
    LMAPI->log_printf(10, "Unloading module PerUser\n");
}

