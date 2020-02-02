#include <string.h>
#include <stdlib.h>

#include "core.h"
#include "config.h"
#include "fileapi.h"
#include "lcgi.h"
#include "liscript.h"
#include "mystring.h"
#include "variables.h"

struct listserver_cgi_hook *cgi_hooks;
struct listserver_cgi_mode *cgi_modes;
struct listserver_cgi_tempvar *cgi_tempvars;

void new_cgi_hooks()
{
   cgi_hooks = NULL;
}

void nuke_cgi_hooks()
{
  struct listserver_cgi_hook *temp;

  temp = cgi_hooks;

  while(cgi_hooks) {
    temp = cgi_hooks->next;
    free(cgi_hooks->name);
    free(cgi_hooks);
    cgi_hooks = temp;
  }
}

void add_cgi_hook(const char *name, CgiHook function)
{
  struct listserver_cgi_hook *temphook;

  temphook = (struct listserver_cgi_hook *)malloc(sizeof(struct
             listserver_cgi_hook));

  temphook->name = strdup(name);
  temphook->hook = function;

  temphook->next = cgi_hooks;
  cgi_hooks = temphook;
}

struct listserver_cgi_hook *find_cgi_hook(const char *name)
{
  struct listserver_cgi_hook *temp;

  temp = cgi_hooks;

  while(temp) {
    if (strcasecmp(temp->name,name) == 0) break;
    temp = temp->next;
  }

  return temp;
}

struct listserver_cgi_hook *get_cgi_hooks()
{
  return cgi_hooks;
}

int cgi_unparse_template(const char *name)
{
  char outfilename[BIG_BUF],infilename[BIG_BUF];
  int inval;
  int cgi_unparse_mode;
  char inchar;
  FILE *infile;
  char varbuffer[BIG_BUF];
  char parmbuffer[BIG_BUF];
  int varbuflen, parmbuflen;

  buffer_printf(infilename, sizeof(infilename) - 1, "%s/%s.lsc", get_string("cgi-template-dir"),
    name);

  buffer_printf(outfilename, sizeof(outfilename) - 1, "%s.cgi-unparse", get_string("queuefile"));

  log_printf(9,"LCGI unparse: %s\n", infilename);

  if (!liscript_parse_file(infilename,outfilename)) {
    log_printf(2,"Unable to parse liscript %s into %s\n",
      infilename, outfilename);
    return 0;
  }

  if ((infile = open_file(outfilename,"r")) == NULL) {
    log_printf(2,"Unable to read %s for LCGI parse!\n", outfilename);
    unlink_file(outfilename);
    return 0;
  }

  cgi_unparse_mode = 0;
  memset(varbuffer, 0, sizeof(varbuffer));
  varbuflen = 0;
  memset(parmbuffer, 0, sizeof(parmbuffer));
  parmbuflen = 0;

  while((inval = getc_file(infile)) != EOF) {
    inchar = (char)inval;

    switch(cgi_unparse_mode) {
       case CGI_UNPARSE_NORMAL:
         if (inchar == '[') cgi_unparse_mode = CGI_UNPARSE_FIRSTHASH;
            else putc_file(inchar,stdout);
         break;

       case CGI_UNPARSE_FIRSTHASH:
         if (inchar == '@') cgi_unparse_mode = CGI_UNPARSE_GETVAR;
            else {
                cgi_unparse_mode = CGI_UNPARSE_NORMAL;
                printf("[%c",inchar);
            }
         break;

       case CGI_UNPARSE_GETVAR:
         if (inchar == ']') { 
               struct listserver_cgi_hook *tempcgihook;
      
               tempcgihook = find_cgi_hook(varbuffer);
               if (tempcgihook) {
                  (*tempcgihook->hook)(parmbuffer);
               } else {
                  printf("<!-- Unknown CGI hook: %s -->\n", 
                    varbuffer);
               }
         
               cgi_unparse_mode = CGI_UNPARSE_NORMAL;
               memset(varbuffer, 0, sizeof(varbuffer));
               varbuflen = 0;
               memset(parmbuffer, 0, sizeof(parmbuffer));
               parmbuflen = 0; 
            }
            else if (inchar == ':') {
               cgi_unparse_mode = CGI_UNPARSE_GETPARM;
            }
            else if (varbuflen < sizeof(varbuffer)) {
               varbuffer[varbuflen++] = inchar;
            }
         break;

       case CGI_UNPARSE_GETPARM:
         if (inchar == ']') { 
               struct listserver_cgi_hook *tempcgihook;
      
               tempcgihook = find_cgi_hook(varbuffer);
               if (tempcgihook) {
                  (*tempcgihook->hook)(parmbuffer);
               } else {
                  printf("<!-- Unknown CGI hook: %s -->\n", 
                    varbuffer);
               }
         
               cgi_unparse_mode = CGI_UNPARSE_NORMAL;
               memset(varbuffer, 0, sizeof(varbuffer));
               varbuflen = 0;
               memset(parmbuffer, 0, sizeof(parmbuffer));
               parmbuflen = 0; 
            }
            else if (parmbuflen < sizeof(parmbuffer)) {
               parmbuffer[parmbuflen++] = inchar;
            }
         break;
    }

  }

  close_file(infile);
  unlink_file(outfilename);

  return 1;
}

void new_cgi_modes()
{
  cgi_modes = NULL;
}

void nuke_cgi_modes()
{
  struct listserver_cgi_mode *temp;

  temp = cgi_modes;

  while(cgi_modes) {
    temp = cgi_modes->next;
    free(cgi_modes->name);
    free(cgi_modes);
    cgi_modes = temp;
  }
}

void add_cgi_mode(const char *name, CgiMode function)
{
  struct listserver_cgi_mode *temphook;

  temphook = (struct listserver_cgi_mode *)malloc(sizeof(struct
             listserver_cgi_mode));

  temphook->name = strdup(name);
  temphook->mode = function;

  temphook->next = cgi_modes;
  cgi_modes = temphook;
}

struct listserver_cgi_mode *find_cgi_mode(const char *name)
{
  struct listserver_cgi_mode *temp;

  temp = cgi_modes;

  while(temp) {
    if (strcasecmp(temp->name,name) == 0) break;
    temp = temp->next;
  }

  return temp;
}


void new_cgi_tempvars()
{
  cgi_tempvars = NULL;
}

void nuke_cgi_tempvars()
{
  struct listserver_cgi_tempvar *temp;

  temp = cgi_tempvars;

  while(cgi_tempvars) {
    temp = cgi_tempvars->next;
    free(cgi_tempvars->name);
    free(cgi_tempvars->value);
    free(cgi_tempvars);
    cgi_tempvars = temp;
  }
}

void add_cgi_tempvar(const char *name, const char *val)
{
  struct listserver_cgi_tempvar *tempvar;

  tempvar = (struct listserver_cgi_tempvar *)malloc(sizeof(struct
             listserver_cgi_tempvar));

  tempvar->name = strdup(name);
  tempvar->value = strdup(val);

  tempvar->next = cgi_tempvars;
  cgi_tempvars = tempvar;
}

struct listserver_cgi_tempvar *get_cgi_tempvars()
{
  return cgi_tempvars;
}

struct listserver_cgi_tempvar *find_cgi_tempvar(const char *name)
{
  struct listserver_cgi_tempvar *temp;

  temp = cgi_tempvars;

  while(temp) {
    log_printf(18,"CGI: tempvar search: comparing %s to %s...\n",
       name, temp->name);
    if (strcasecmp(temp->name,name) == 0) break;
    temp = temp->next;
  }

  return temp;
}
