#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef UNCRYPTED_PASS
/* Keep glibc happy */
#define __USE_XOPEN

/* Keep SunOS happy, too... */
# ifdef SUNOS_5
# include <crypt.h>
# endif
#endif

#ifndef WIN32
#include <unistd.h>
#endif /* WIN32 */

#include "core.h"
#include "cmdarg.h"
#include "modes.h"
#include "user.h"
#include "variables.h"
#include "smtp.h"
#include "command.h"
#include "hooks.h"
#include "list.h"
#include "forms.h"
#include "parse.h"
#include "fileapi.h"
#include "compat.h"
#include "cookie.h"
#include "mystring.h"
#include "unmime.h"
#include "build.h"
#include "lpm-mods.h"
#include "internal.h"
#include "upgrade.h"

int upgrade_build7()
{
   char tbuf[BIG_BUF];
   char tbuf2[BIG_BUF];

   printf("* Upgrading %s installation to build 7\n", SERVICE_NAME_MC);
   printf(" - Moving files...");
   /* Move files to proper location */
   buffer_printf(tbuf, sizeof(tbuf) - 1, "%s/site-passwords", get_string("lists-root"));
   buffer_printf(tbuf2, sizeof(tbuf2) - 1, "%s", get_string("pwdfile"));
   if (exists_file(tbuf) && (strcmp(tbuf,tbuf2) != 0)) {
      mkdirs(tbuf2);
      replace_file(tbuf,tbuf2);
   }

   buffer_printf(tbuf, sizeof(tbuf) - 1, "%s/cookies", get_string("lists-root"));
   buffer_printf(tbuf2, sizeof(tbuf2) - 1, "%s/SITEDATA/cookies", get_string("lists-root"));
   if (exists_file(tbuf)) {
      mkdirs(tbuf2);
      replace_file(tbuf,tbuf2);
   }

   buffer_printf(tbuf, sizeof(tbuf) - 1, "%s/restrictvars", get_string("lists-root"));
   buffer_printf(tbuf2, sizeof(tbuf2) - 1, "%s/SITEDATA/restrictvars", get_string("lists-root"));
   if (exists_file(tbuf)) {
      mkdirs(tbuf2);
      replace_file(tbuf,tbuf2);
   }

   buffer_printf(tbuf, sizeof(tbuf) - 1, "%s/trusted", get_string("lists-root"));
   buffer_printf(tbuf2, sizeof(tbuf2) - 1, "%s/SITEDATA/trusted", get_string("lists-root"));
   if (exists_file(tbuf)) {
      mkdirs(tbuf2);
      replace_file(tbuf,tbuf2);
   }
   printf("done!\n");

#ifndef UNCRYPTED_PASS
   /* Convert us to a crypted password file */
   buffer_printf(tbuf, sizeof(tbuf) - 1, "%s", get_string("pwdfile"));
   buffer_printf(tbuf2, sizeof(tbuf2) - 1, "%s.pwdchange", get_string("pwdfile"));
   if (exists_file(tbuf)) {
      FILE *infile, *outfile;
      char buffer[BIG_BUF];

      printf(" - Converting to crypted passwords...");

      if ((infile = open_file(tbuf,"r")) == NULL) {
         return 1;
      }
      if ((outfile = open_file(tbuf2,"w")) == NULL) {
         close_file(infile);
         return 1;
      }

      while(read_file(buffer, sizeof(buffer), infile)) {
         char *pt;

         pt = strchr(buffer,':');
         if (!pt) continue;

         /* Eat newline */
         if (buffer[strlen(buffer) - 1] == '\n')
            buffer[strlen(buffer) - 1] = 0;

         *pt = 0;  pt += 2;

         /* We're already encrypted */
         if (*pt == '^') {
            write_file(outfile,"%s: %s\n", buffer, pt);
         } else {
            write_file(outfile, "%s: ^ %s\n", buffer, crypt(pt,"SP"));
         }
      }
      close_file(infile);
      close_file(outfile);

      replace_file(tbuf2,tbuf);
      printf("done!\n");
   }
#endif

printf("* Build 7 upgrade complete.\n\n");

return 1;

}

int upgrade_listserver(int prev, int cur)
{
    int status;
    char tbuf[BIG_BUF];

    if(cur == prev)  return 1;

    /* Perform all upgrade calls here */

    /* Build 7 upgrade; moves files to SITEDATA subdirectory, and
       encrypts passwords */
    if ((cur > 6) && (prev < 6))
       upgrade_build7();

    /* Allow the modules to perform all upgrade tasks */
    upgrade_all_modules(prev, cur);

    /* Now handle per list upgrade of all lists and all modules per list */
    status = walk_lists(&tbuf[0]);
    while(status) {
        if(list_valid(tbuf)) {
           /* switch context to the new list */
           wipe_vars(VAR_LIST|VAR_TEMP);
           set_context_list(tbuf);
           upgrade_list(prev, cur);
           listupgrade_all_modules(prev, cur);
        }
        status = next_lists(&tbuf[0]);
    }
    return 1;
}

int upgrade_list(int prev, int cur)
{
    /* Perform any list specific upgrade tasks here */
    return 1;
}

