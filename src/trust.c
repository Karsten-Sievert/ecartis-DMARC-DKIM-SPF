#include <string.h>

#include "fileapi.h"
#include "variables.h"
#include "config.h"
#include "core.h"
#include "mystring.h"

int is_trusted(const char *list)
{
   FILE *infile;
   char filename[BIG_BUF], buffer[BIG_BUF];
   int found;

   buffer_printf(filename, sizeof(filename) - 1, "%s/SITEDATA/trusted", get_string("lists-root"));

   found = 0;

   if ((infile = open_file(filename,"r")) == NULL) {
      return 0;
   }

   while (read_file(buffer, sizeof(buffer), infile) && !found) {
      if (buffer[strlen(buffer) - 1] == '\n')
         buffer[strlen(buffer) - 1] = 0;

      if (strcasecmp(buffer,list) == 0) {
         found = 1;
      }
   }
   close_file(infile);

   return found;
}

void init_restricted_vars()
{
   FILE *infile;
   char filename[BIG_BUF], buffer[BIG_BUF];
   buffer_printf(filename, sizeof(filename) - 1, "%s/SITEDATA/restrictvars", get_string("lists-root"));

   if ((infile = open_file(filename,"r")) == NULL) {
      return;
   }

   while (read_file(buffer, sizeof(buffer), infile)) {
      if (buffer[strlen(buffer) - 1] == '\n')
         buffer[strlen(buffer) - 1] = 0;

      restrict_var(buffer);
   }
   close_file(infile);
}
