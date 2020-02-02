#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef WIN32
#include <unistd.h>
#else
#include <winsock2.h>
#endif

#include "toolbox.h"

void qmail_dot_file(const char *list, const char *suffix, const char *flags,
                    const char *siteconfig)
{
   char tempbuf[BIG_BUF];
   char buf[BIG_BUF];
   FILE *outfile;

   LMAPI->buffer_printf(buf, sizeof(buf) - 1, "qmail-aliases/.qmail-%s%s",
                        list, suffix);
   LMAPI->listdir_file(tempbuf, list, buf);

   if ((outfile = LMAPI->open_file(tempbuf,"w")) == NULL) {
      fprintf(stderr,"ERROR creating %s\n",tempbuf);
      return;
   }

   LMAPI->write_file(outfile,"|%s/%s %s%s %s\n",
     LMAPI->get_string("listserver-bin-dir"), SERVICE_NAME_LC,
     siteconfig, flags, list);

   LMAPI->close_file(outfile);
}

void make_list_aliases(const char *listname, const char *siteconfig)
{
   char tempbuf[BIG_BUF];

   if (!LMAPI->get_bool("newlist-qmail")) {
     printf("\n# Aliases for '%s' mailing list.\n", listname);

     printf("%s: \"|%s/%s %s-s %s\"\n",
       listname, LMAPI->get_string("listserver-bin-dir"),
       SERVICE_NAME_LC, siteconfig, listname);

     printf("%s-request: \"|%s/%s %s-r %s\"\n",
       listname, LMAPI->get_string("listserver-bin-dir"),
       SERVICE_NAME_LC, siteconfig, listname);

     printf("%s-repost: \"|%s/%s %s-a %s\"\n",
       listname, LMAPI->get_string("listserver-bin-dir"),
       SERVICE_NAME_LC, siteconfig, listname);

     printf("%s-admins: \"|%s/%s %s-admins %s\"\n",
       listname, LMAPI->get_string("listserver-bin-dir"),
       SERVICE_NAME_LC, siteconfig, listname);

     printf("%s-moderators: \"|%s/%s %s-moderators %s\"\n",
       listname, LMAPI->get_string("listserver-bin-dir"),
       SERVICE_NAME_LC, siteconfig, listname);

     printf("%s-bounce: \"|%s/%s %s-bounce %s\"\n",
       listname, LMAPI->get_string("listserver-bin-dir"),
       SERVICE_NAME_LC, siteconfig, listname);

   } else {
     fprintf(stderr,"Creating dot-qmail aliases...");

     LMAPI->listdir_file(tempbuf, listname, "qmail-aliases/dummyfile");
     LMAPI->mkdirs(tempbuf);

     qmail_dot_file(listname,"","-s",siteconfig);
     qmail_dot_file(listname,"-request","-r",siteconfig);
     qmail_dot_file(listname,"-repost","-a",siteconfig);
     qmail_dot_file(listname,"-admins","-admins",siteconfig);
     qmail_dot_file(listname,"-bounce","-bounce",siteconfig);
     qmail_dot_file(listname,"-moderators","-moderators",siteconfig);

     fprintf(stderr,"done.\n");
     fprintf(stderr,"qmail dot-aliases are in the 'qmail-aliases' directory under the list.\nCopy them to your global aliases dir.\n");
   }
}

void newlist(const char *listname)
{
   char tempbuf[BIG_BUF];
   char hostname[BIG_BUF];
   char admin[BIG_BUF];
   char *siteconfig;
   struct list_user u;
   FILE *dummyfile;

   if (LMAPI->get_var("hostname")) {
      LMAPI->buffer_printf(hostname, sizeof(hostname) - 1, "%s",
        LMAPI->get_string("hostname"));
   } else {
      memset(hostname, 0, sizeof(hostname));
      LMAPI->build_hostname(&hostname[0],sizeof(hostname));
   }

   siteconfig = NULL;

   if (!LMAPI->get_var("listserver-bin-dir")) {
      LMAPI->set_var("listserver-bin-dir",LMAPI->get_string("listserver-root"),
                     VAR_GLOBAL);
   }

   if (LMAPI->get_var("site-config-file")) {
      char sitebuf[BIG_BUF];
      char *ptr;

      stringcpy(tempbuf,LMAPI->get_string("site-config-file"));
      ptr = &tempbuf[0] + strlen(LMAPI->get_string("listserver-conf")) + 1;

      LMAPI->buffer_printf(sitebuf, sizeof(sitebuf) - 1, "-c %s ", ptr);
      siteconfig = strdup(sitebuf);
   } else {
      siteconfig = strdup("");
   }

   if (!LMAPI->get_var("newlist-admin")) {
      fprintf(stderr,"List admin e-mail: ");
      LMAPI->read_file(admin, sizeof(admin), stdin);
   } else {
      LMAPI->buffer_printf(admin, sizeof(admin) - 1, "%s", LMAPI->get_string("newlist-admin"));
   }

   if (admin[strlen(admin) - 1] == '\n') 
      admin[strlen(admin) - 1] = 0;

   if (!strchr(admin,'@')) {
      if ((strlen(admin) + strlen(hostname) + 1) < (sizeof(admin) - 1)) {
        stringcat(admin, "@");
        stringcat(admin, hostname);
      } else {
        fprintf(stderr,"Invalid administrator address!\n");
        return;
      }
   }

   LMAPI->set_var("list-owner", admin, VAR_LIST);

   LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s-bounce@%s",
     listname, hostname);
   LMAPI->set_var("send-as", tempbuf, VAR_LIST);

   LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s@%s",
     listname, hostname);
   LMAPI->set_var("reply-to", tempbuf, VAR_LIST);

   LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s-admins@%s",
     listname, hostname);
   LMAPI->set_var("administrivia-address", tempbuf, VAR_LIST);

   LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s-moderators@%s",
     listname, hostname);
   LMAPI->set_var("moderator", tempbuf, VAR_LIST);

   LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "[%s]",
     listname);
   LMAPI->set_var("subject-tag", tempbuf, VAR_LIST);

   LMAPI->listdir_file(tempbuf, listname, "dummyfile");
   LMAPI->mkdirs(tempbuf);
   LMAPI->listdir_file(tempbuf, listname, "text/dummyfile");
   LMAPI->mkdirs(tempbuf);

   fprintf(stderr," Writing config file...");

   LMAPI->listdir_file(tempbuf, listname, "config");
   LMAPI->write_configfile(tempbuf,VAR_LIST,
     ":Basic Configuration:CGI:Debugging:Digest:Bouncer:");

   fprintf(stderr,"done.\n");

   fprintf(stderr," Creating default user file...");

   /* Dummy up a user file - eventually we'll want a 'user_file_create'
      function that takes a list, and then it will be able to work with
      things other than a textfile for users, like a SQL database. */  
   LMAPI->listdir_file(tempbuf, listname, "users");
   dummyfile = LMAPI->open_file(tempbuf,"w");
   LMAPI->close_file(dummyfile);

   strncpy(u.address,admin,BIG_BUF - 1);
   LMAPI->buffer_printf(u.flags,HUGE_BUF - 1,
     "|ADMIN|SUPERADMIN|MODERATOR|CCERRORS|REPORTS|ECHOPOST|");
   LMAPI->user_write(tempbuf,&u);

   fprintf(stderr,"done.\n");

   fprintf(stderr,"Sending aliases for sendmail/Exim/Postfix/Zmailer to stdout.\n");
   make_list_aliases(listname, siteconfig);

   free(siteconfig);

   LMAPI->do_hooks("NEWLIST");
}

CMDARG_HANDLER(cmdarg_qmail)
{
   LMAPI->set_var("newlist-qmail","yes",VAR_GLOBAL);
   return CMDARG_OK;
}

MODE_HANDLER(mode_newlist)
{
   fprintf(stderr,"Creating new list '%s'...\n", LMAPI->get_string("list"));
   newlist(LMAPI->get_string("list"));
   return MODE_OK;
}

MODE_HANDLER(mode_freshen)
{
   char tempbuf[BIG_BUF];

   fprintf(stderr,"Freshening list '%s'...", LMAPI->get_string("list"));
   LMAPI->listdir_file(tempbuf, LMAPI->get_string("list"), "config");
   LMAPI->write_configfile(tempbuf,VAR_LIST,
                           ":Basic Configuration:CGI:Debugging:Digest:Bouncer:");
   fprintf(stderr,"done.\n");

   return MODE_OK;
}

MODE_HANDLER(mode_buildaliases)
{
   char tempbuf[BIG_BUF];
   int status;
   char *siteconfig;

   if (LMAPI->get_var("site-config-file")) {
      char sitebuf[BIG_BUF];
      char *ptr;

      stringcpy(tempbuf,LMAPI->get_string("site-config-file"));
      ptr = &tempbuf[0] + strlen(LMAPI->get_string("listserver-conf")) + 1;

      LMAPI->buffer_printf(sitebuf, sizeof(sitebuf) - 1, "-c %s ", ptr);
      siteconfig = strdup(sitebuf);
   } else {
      siteconfig = strdup("");
   }

   fprintf(stderr,"Rebuilding aliases for all lists: ");

   status = LMAPI->walk_lists(&tempbuf[0]);
   while (status) {
      if (LMAPI->list_valid(tempbuf)) {
         fprintf(stderr,"%s ",tempbuf);
         make_list_aliases(tempbuf,siteconfig);
      }
      printf("\n");
      status = LMAPI->next_lists(&tempbuf[0]);
   }  

   fprintf(stderr,"\n");

   free(siteconfig);

   return MODE_OK;
}

CMDARG_HANDLER(cmdarg_buildaliases)
{
   LMAPI->set_var("mode","buildaliases",VAR_GLOBAL);
   LMAPI->set_var("fakequeue","yes",VAR_GLOBAL);

   return CMDARG_OK;
}

CMDARG_HANDLER(cmdarg_newlist)
{
   if(!argv[0]) {
       fprintf(stderr,"Switch -newlist requires a list as a parameter.\n");
       return CMDARG_ERR;
   } else {
       char *listname;
       char *tmp;

       listname = LMAPI->lowerstr(argv[0]);

       tmp = LMAPI->list_directory(listname);
       if (LMAPI->exists_file(tmp)) {
           fprintf(stderr,"List '%s' cannot be created because of a filename conflict.\n", argv[0]);
           free(tmp);
           return CMDARG_ERR;
       }
       LMAPI->set_var("list", listname, VAR_GLOBAL);
       LMAPI->set_var("mode", "newlist", VAR_GLOBAL);
       LMAPI->set_var("fakequeue", "yes", VAR_GLOBAL);
       free(listname);
       free(tmp);
       return CMDARG_OK;
   }
}

CMDARG_HANDLER(cmdarg_admin)
{
   if(!argv[0]) {
       fprintf(stderr,"Switch -admin requires an address as a parameter.\n");
   }
   else if (!strchr(argv[0],'@')) {
       fprintf(stderr,"Switch -admin requires a valid address as a parameter.\n");
       return CMDARG_ERR;
   }

   LMAPI->set_var("newlist-admin", argv[0], VAR_GLOBAL);

   return CMDARG_OK;
}

CMDARG_HANDLER(cmdarg_freshen)
{
   if(!argv[0]) {
       fprintf(stderr,"Switch -freshen requires a list as a parameter.\n");
       return CMDARG_ERR;
   } else {
       char *listname;

       listname = LMAPI->lowerstr(argv[0]);

       if (!LMAPI->list_valid(listname)) {
           fprintf(stderr,"List '%s' not found.\n", argv[0]);
           free(listname);
           return CMDARG_ERR;
       }
       LMAPI->set_var("list", listname, VAR_GLOBAL);
       LMAPI->set_var("mode", "freshen", VAR_GLOBAL);
       LMAPI->set_var("fakequeue", "yes", VAR_GLOBAL);
       free(listname);
       return CMDARG_OK;
   }
}

