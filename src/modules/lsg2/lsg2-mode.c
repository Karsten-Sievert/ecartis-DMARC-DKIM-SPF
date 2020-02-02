#include "lsg2.h"
#include <string.h>

CGI_MODE(cgimode_default)
{
   if (!LMAPI->cgi_unparse_template("frontpage")) {
      lsg2_internal_error("No Default Template");
   }
}

CGI_MODE(cgimode_login)
{
   char cookiebuf[BIG_BUF];
   const char *fromaddy, *pass;

   fromaddy = LMAPI->get_var("lcgi-user");
   pass = LMAPI->get_var("lcgi-pass");

   if (!fromaddy) {
      lsg2_internal_error("No username provided!");
      return;
   }

   if(lsg2_validate(cookiebuf, sizeof(cookiebuf) - 1)) {
       if (LMAPI->get_var("lcgi-list")) {
          LMAPI->set_var("lcgi-mode","listmenu",VAR_GLOBAL);
          cgimode_listmenu();
          return;
       }
       if (!LMAPI->cgi_unparse_template("mainmenu")) {
          lsg2_internal_error("No main menu Template");
          return;
       }
   }

}

CGI_MODE(cgimode_displayfile)
{
   const char *fromaddy, *pass, *list;
   char cookiebuf[BIG_BUF];

   fromaddy = LMAPI->get_var("lcgi-user");
   pass = LMAPI->get_var("lcgi-pass");
   list = LMAPI->get_var("list");

   if (!list) {
      lsg2_internal_error("No list context!");
   }

   if (!fromaddy) {
      lsg2_internal_error("No username provided!");
      return;
   }

   if(LMAPI->find_cgi_tempvar("show-welcome")) {
      char tempbuf[BIG_BUF];

      LMAPI->listdir_file(tempbuf,list,LMAPI->get_string("welcome-file"));
      LMAPI->set_var("tlcgi-textfile",tempbuf,VAR_TEMP);
   } else
   if(LMAPI->find_cgi_tempvar("show-faq")) {
      char tempbuf[BIG_BUF];

      LMAPI->listdir_file(tempbuf,list,LMAPI->get_string("faq-file"));
      LMAPI->set_var("tlcgi-textfile",tempbuf,VAR_TEMP);
   } else
   if(LMAPI->find_cgi_tempvar("show-info")) {
      char tempbuf[BIG_BUF];

      LMAPI->listdir_file(tempbuf,list,LMAPI->get_string("info-file"));
      LMAPI->set_var("tlcgi-textfile",tempbuf,VAR_TEMP);
   } else {
      lsg2_internal_error("No textfile to display.");
   }

   if(lsg2_validate(cookiebuf, sizeof(cookiebuf) - 1)) {
       if (!LMAPI->cgi_unparse_template("textfile")) {
          lsg2_internal_error("No Textfile display template");
          return;
       }
   }   
}

CGI_MODE(cgimode_listmenu)
{
   char cookiebuf[BIG_BUF];
   const char *fromaddy, *pass, *list;

   fromaddy = LMAPI->get_var("lcgi-user");
   pass = LMAPI->get_var("lcgi-pass");
   list = LMAPI->get_var("list");

   if (!list) {
      lsg2_internal_error("No list context!");
   }

   if (!fromaddy) {
      lsg2_internal_error("No username provided!");
      return;
   }

   if(lsg2_validate(cookiebuf, sizeof(cookiebuf) - 1)) {
       if (!LMAPI->cgi_unparse_template("listmenu")) {
          lsg2_internal_error("No list menu Template");
          return;
       }
   }
}

CGI_MODE(cgimode_userlist)
{
   char cookiebuf[BIG_BUF];
   const char *fromaddy, *list;

   fromaddy = LMAPI->get_var("lcgi-user");
   list = LMAPI->get_var("list");

   if (!list) {
      lsg2_internal_error("No list context!");
   }

   if (!fromaddy) {
      lsg2_internal_error("No username provided!");
      return;
   }

   if(lsg2_validate(cookiebuf, sizeof(cookiebuf) - 1)) {
       const char *list;
       struct list_user user;
       int allow=0;
    
       list = LMAPI->get_string("list");

// CHANGES RW
// I HATE IT! KILL! MURDER! 

       if (strcmp(LMAPI->get_string("who-status"),"public") == 0)
         { allow=1; }
       else
         { if ((LMAPI->user_find_list(list,LMAPI->get_string("lcgi-user"),&user)) && (strcmp(LMAPI->get_string("who-status"),"private") == 0))
              { allow=1; }
           else
             { if (LMAPI->user_hasflag(&user,"ADMIN"))
                 { allow=1; }
             }
         }

       if (allow == 0) {
          if (!LMAPI->cgi_unparse_template("userlist-deny"))
             lsg2_internal_error("No userlist-deny template.");
          return;
       }
       if (!LMAPI->cgi_unparse_template("userlist")) {
          lsg2_internal_error("No userlist Template");
          return;
       }
   }
}

CGI_MODE(cgimode_passwd)
{
   char cookiebuf[BIG_BUF], buf[BIG_BUF];
   const char *fromaddy, *password, *passwd2;

   fromaddy = LMAPI->get_string("lcgi-user");
   password = LMAPI->get_string("lcgi-pass");
   passwd2 = LMAPI->get_string("lcgi-pass-confirm");

   if(!lsg2_validate(cookiebuf, sizeof(cookiebuf) - 1)) {
      return;
   }

   if (!password) {
      lsg2_internal_error("You must provide a password.");
      return;
   }

   if (passwd2) {
      if (strcmp(passwd2,password) != 0) {
         lsg2_internal_error("The passwords do not match.");
         return;
      }
   }

   LMAPI->set_pass(fromaddy,password);

   LMAPI->buffer_printf(buf, sizeof(buf) - 1, "Password changed.", fromaddy);
   LMAPI->set_var("tlcgi-generic-text", buf, VAR_TEMP);
   if (!LMAPI->cgi_unparse_template("generic")) {
      lsg2_internal_error("No generic template.");
   }
   return;
}

CGI_MODE(cgimode_subscribe)
{
   char cookiebuf[BIG_BUF];
   char userfile[BIG_BUF];
   char tempbuf[BIG_BUF];
   const char *from;

   if (!lsg2_validate(cookiebuf, sizeof(cookiebuf) - 1)) {
      return;
   }

   from = LMAPI->get_var("lcgi-user");

   LMAPI->set_var("subscribe-me",from,VAR_GLOBAL);
   LMAPI->set_var("fromaddress",from,VAR_GLOBAL);
   LMAPI->set_var("realsender",from,VAR_GLOBAL);

   if (LMAPI->do_hooks("PRESUB") == HOOK_RESULT_FAIL) {
      char tempbuf[BIG_BUF];

      LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s.perr",
         LMAPI->get_string("queuefile"));
      LMAPI->set_var("tlcgi-textfile",tempbuf,VAR_TEMP);
      if (!LMAPI->cgi_unparse_template("textfile")) {
         lsg2_internal_error("No textfile template.");
      }
      LMAPI->unlink_file(tempbuf);
      return;      
   }

   LMAPI->listdir_file(userfile,LMAPI->get_string("list"),"users");

   if (LMAPI->user_add(userfile,from)) {
       lsg2_internal_error("Filesystem error while subscribing user.");
       LMAPI->filesys_error(userfile);
       LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s.perr",
          LMAPI->get_string("queuefile"));
       LMAPI->unlink_file(tempbuf);
       return;
   }
   LMAPI->log_printf(0, "%s subscribed to %s\n",from,
       LMAPI->get_string("listname"));

   LMAPI->do_hooks("POSTSUB");

   if (!LMAPI->cgi_unparse_template("subscribe")) {
      lsg2_internal_error("No 'subscribe' template.");
      return;
   }

   return;
}

CGI_MODE(cgimode_unsubscribe)
{
   char cookiebuf[BIG_BUF];
   char userfile[BIG_BUF];
   const char *from;
   char tempbuf[BIG_BUF];

   if (!lsg2_validate(cookiebuf, sizeof(cookiebuf) - 1)) {
      return;
   }

   from = LMAPI->get_var("lcgi-user");

   LMAPI->set_var("subscribe-me",from,VAR_GLOBAL);
   LMAPI->set_var("fromaddress",from,VAR_GLOBAL);
   LMAPI->set_var("realsender",from,VAR_GLOBAL);

   if (LMAPI->do_hooks("PREUNSUB") == HOOK_RESULT_FAIL) {
      char tempbuf[BIG_BUF];

      LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s.perr",
         LMAPI->get_string("queuefile"));
      LMAPI->set_var("tlcgi-textfile",tempbuf,VAR_TEMP);
      if (!LMAPI->cgi_unparse_template("textfile")) {
         lsg2_internal_error("No textfile template.");
      }
      LMAPI->unlink_file(tempbuf);
      return;      
   }

   LMAPI->listdir_file(userfile,LMAPI->get_string("list"),"users");

   if (LMAPI->user_remove(userfile,from)) {
       lsg2_internal_error("Filesystem error while unsubscribing user.");
       LMAPI->filesys_error(userfile);
       LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s.perr",
          LMAPI->get_string("queuefile"));
       LMAPI->unlink_file(tempbuf);
       return;
   }

   LMAPI->log_printf(0, "%s unsubscribed from %s\n",from,          
       LMAPI->get_string("listname"));

   LMAPI->do_hooks("POSTUNSUB");

   if (!LMAPI->cgi_unparse_template("unsubscribe")) {
      lsg2_internal_error("No 'unsubscribe' template.");
      return;
   }

   return;
}


CGI_MODE(cgimode_logout)
{
   char cookiefile[BIG_BUF];
   char cookiebuf[BIG_BUF];

   if(!lsg2_validate(cookiebuf, sizeof(cookiebuf) - 1)) {
      return;
   }

   LMAPI->buffer_printf(cookiefile, sizeof(cookiefile) - 1, "%s/SITEDATA/cookies",
      LMAPI->get_string("lists-root"));
   LMAPI->del_cookie(cookiefile,cookiebuf);

   LMAPI->clean_var("lcgi-cookie", VAR_ALL);
   LMAPI->clean_var("lcgi-user", VAR_ALL);
   LMAPI->clean_var("lcgi-list", VAR_ALL);
   LMAPI->clean_var("list", VAR_ALL);

   LMAPI->wipe_vars(VAR_LIST);

   if (!LMAPI->cgi_unparse_template("frontpage")) {
      lsg2_internal_error("No frontpage template.");
   }
   return;
}

CGI_MODE(cgimode_flagedit)
{
   char cookiebuf[BIG_BUF];
   struct list_user user;
   char buf[BIG_BUF];

   if (!lsg2_validate(cookiebuf, sizeof(cookiebuf) - 1)) {
      return;
   }

   if (!LMAPI->get_var("lcgi-list")) {
      lsg2_internal_error("At flag screen without list context.");
      return;
   }

   if (!LMAPI->get_var("lcgi-user")) {
      lsg2_internal_error("At flag screen without user.");
   }

   if (!LMAPI->user_find_list(LMAPI->get_string("lcgi-list"),
                              LMAPI->get_string("lcgi-user"), &user)) {
      LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%s is not a member of %s",
        LMAPI->get_string("lcgi-user"), LMAPI->get_string("lcgi-list"));
      LMAPI->set_var("tlcgi-generic-text", buf, VAR_TEMP);
      if (!LMAPI->cgi_unparse_template("generic")) {
        lsg2_internal_error("No generic template.");
      }
      return;
   }

   if (!LMAPI->cgi_unparse_template("flagedit")) {
      lsg2_internal_error("No flagedit template.");
   }
   return;
}

CGI_MODE(cgimode_setflags)
{
   char cookiebuf[BIG_BUF];
   struct list_user user;
   const char *list, *fromaddy;
   struct listserver_flag *tflag;
   char tbuf[BIG_BUF];
   int isadmin;

   if (!lsg2_validate(cookiebuf, sizeof(cookiebuf) - 1)) {
      return;
   }

   list = LMAPI->get_var("lcgi-list");
   fromaddy = LMAPI->get_var("lcgi-user");

   if (!list || !fromaddy) {
      lsg2_internal_error("Invalid state for flag setting.");
   }

   if (!LMAPI->user_find_list(list, fromaddy, &user)) {
      char errbuf[BIG_BUF];

      LMAPI->buffer_printf(errbuf, sizeof(errbuf) - 1, "%s is not a member of %s",
        fromaddy, list);

      lsg2_internal_error(errbuf);
   }

   isadmin = LMAPI->user_hasflag(&user,"ADMIN");

   tflag = LMAPI->get_flags();
   while (tflag) {
      if (!strcasecmp(tflag->name,"superadmin") || 
          (tflag->admin && !isadmin) || 
          ((tflag->admin & ADMIN_UNSAFE) || 
           (tflag->admin & ADMIN_UNSETTABLE))) {
         tflag = tflag->next;
         continue;
      }

      if (LMAPI->find_cgi_tempvar(tflag->name)) {
         LMAPI->log_printf(18,"CGI: %s is selected.\n", tflag->name);
         if (!LMAPI->user_hasflag(&user,tflag->name)) {
            LMAPI->set_var("setflag-user",user.address,VAR_TEMP);
            LMAPI->set_var("setflag-flag",tflag->name,VAR_TEMP);
            if (LMAPI->do_hooks("SETFLAG") != HOOK_RESULT_FAIL) {
               /* Re-read the user in case they were modified by the
                * hooks. */
               LMAPI->user_find_list(list,fromaddy,&user);
               if (!LMAPI->user_hasflag(&user,tflag->name)) {
                  char userlistfile[BIG_BUF];

                  LMAPI->listdir_file(userlistfile, list, "users");
                  LMAPI->user_setflag(&user,tflag->name,0);
                  LMAPI->user_write(userlistfile, &user);
               }
            }
         }
      } else {
         LMAPI->log_printf(18,"CGI: %s is unselected.\n", tflag->name);
         if (LMAPI->user_hasflag(&user,tflag->name)) {
            LMAPI->set_var("setflag-user",user.address,VAR_TEMP);
            LMAPI->set_var("setflag-flag",tflag->name,VAR_TEMP);
            if (LMAPI->do_hooks("UNSETFLAG") != HOOK_RESULT_FAIL) {
               /* Re-read the user in case they were modified by the
                * hooks. */
               LMAPI->user_find_list(list,fromaddy,&user);
               if (LMAPI->user_hasflag(&user,tflag->name)) {
                  char userlistfile[BIG_BUF];

                  LMAPI->listdir_file(userlistfile, list, "users");
                  LMAPI->user_unsetflag(&user,tflag->name,0);
                  LMAPI->user_write(userlistfile, &user);
               }
            }
         }
      }
      tflag = tflag->next;
   }

   LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "%s.perr",
     LMAPI->get_string("queuefile"));

   if (LMAPI->exists_file(tbuf)) {
      LMAPI->set_var("tlcgi-textfile",tbuf,VAR_TEMP);
      if (!LMAPI->cgi_unparse_template("textfile")) {
         lsg2_internal_error("No 'textfile' template.");
      }
      LMAPI->unlink_file(tbuf);
   } else {
      if (!LMAPI->cgi_unparse_template("userinfo")) {
         lsg2_internal_error("No 'userinfo' template.");
      }
   }
}

CGI_MODE(cgimode_setname)
{
   char cookiebuf[BIG_BUF];
   const char *list, *user, *name;

   if (!lsg2_validate(cookiebuf, sizeof(cookiebuf) - 1)) {
      return;
   }

   list = LMAPI->get_var("lcgi-list");
   user = LMAPI->get_var("lcgi-user");
   name = LMAPI->get_var("lcgi-fullname");

   if (!list || !user || !name) {
      lsg2_internal_error("Mode not initialized properly.");
      return;
   }
                                       
   LMAPI->userstat_set_stat(list,user,"realname",name);

   if (!LMAPI->cgi_unparse_template("userinfo")) {
      lsg2_internal_error("No 'userinfo' template.");
   }   

   return;
}

CGI_MODE(cgimode_userinfo)
{
   char cookiebuf[BIG_BUF];
   const char *list, *user;

   if (!lsg2_validate(cookiebuf, sizeof(cookiebuf) - 1)) {
      return;
   }

   list = LMAPI->get_var("lcgi-list");
   user = LMAPI->get_var("lcgi-user");

   if (!list || !user) {
      lsg2_internal_error("Mode not initialized properly.");
      return;
   }
                                       
   if (!LMAPI->cgi_unparse_template("userinfo")) {
      lsg2_internal_error("No 'userinfo' template.");
   }   

   return;
}

