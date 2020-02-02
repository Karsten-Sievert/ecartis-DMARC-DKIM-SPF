#include "lsg2.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

extern int flag_cmp(const void *, const void *);

int lsg2_admin_validate()
{
   char cookiebuf[BIG_BUF];
   const char *list, *fromaddy;
   struct list_user user;
   
   if (!lsg2_validate(cookiebuf, sizeof(cookiebuf) - 1)) {
      return 0;
   }

   list = LMAPI->get_var("lcgi-list");
   fromaddy = LMAPI->get_var("lcgi-user");

   if (!list || !fromaddy) {
      lsg2_internal_error("Mode initialized incorrectly.");
      return 0;
   }

   if (!LMAPI->user_find_list(list,fromaddy,&user)) {
      lsg2_internal_error("Mode initialized incorrectly.");
      return 0;
   }

   if (LMAPI->get_bool("lsg2-paranoia")) {
      lsg2_internal_error("Attempt to use admin mode when paranoia is enabled!");
      return 0;
   }
   
   if (!LMAPI->user_hasflag(&user,"ADMIN")) {
      lsg2_internal_error("A non-admin user tried to use admin mode.");
      return 0;
   }

   return 1;
}

CGI_MODE(cgimode_admin)
{
   if (!lsg2_admin_validate()) {
      return;
   }

   if (!LMAPI->cgi_unparse_template("adminmenu")) {
      lsg2_internal_error("No 'adminmenu' template.");
   }

   return;
}

CGI_MODE(cgimode_config)
{
   if (!lsg2_admin_validate()) {
      return;
   }

   if (!LMAPI->cgi_unparse_template("config")) {
      lsg2_internal_error("No 'config' template.");
   }

   return;
}

CGI_HANDLER(cgihook_configfile)
{
   const char *fromaddy, *list;
   struct list_user user;
   struct var_data *vartemp;
   char lastsect[BIG_BUF];
   int counter, trusted;

   fromaddy = LMAPI->get_var("lcgi-user");
   list = LMAPI->get_var("lcgi-list");

   if (!fromaddy || !list) return 0;

   if (!LMAPI->user_find_list(list,fromaddy,&user))
      return 0;

   if (!LMAPI->user_hasflag(&user,"ADMIN"))
      return 0;

   trusted = LMAPI->is_trusted(list);

   printf("<table border=0 width=100%%>\n");

   counter = 0;

   vartemp = LMAPI->start_varlist();

   while (vartemp) {
      const char *val;

      if (!(vartemp->flags & VAR_LIST) || (vartemp->flags & VAR_INTERNAL)) {
         vartemp = LMAPI->next_varlist();
         continue;
      }

      if ((vartemp->flags & VAR_RESTRICTED) && !trusted) {
         vartemp = LMAPI->next_varlist();
         continue;
      }

      if (counter ? (vartemp->section ? strcasecmp(lastsect,vartemp->section) : 0 ) : 1) {
         stringcpy(lastsect,vartemp->section);
         if (counter) {
            printf("<tr><td colspan=4><HR></td></tr>\n");
         }
         printf("<tr>\n");
         printf("<td colspan=4><a name=\"%s\"><font size=+1><b>%s</b></font></td>\n",
           lastsect, lastsect);
         printf("</tr>\n");
      }

      if (!vartemp->section && lastsect[0]) {
         lastsect[0] = 0;
         if (counter) {
            printf("<tr><td colspan=4><HR></td></tr>\n");
         }
         printf("<tr><td colspan=4><a name=\"misc\"><font size=+1><b>(Unclassified/Misc)</b></font></td></tr>\n");
      }

      counter++;

      printf("<tr>\n");

      val = LMAPI->get_cur_varval_level(vartemp, VAR_LIST);
      if (val) {
         printf(" <td valign=top><input name=\"lcgipl-%s-set\" type=checkbox",
          vartemp->name);
         printf(" checked=\"true\"");      
         printf("></td>\n");
      } else {
         val = LMAPI->get_cur_varval_level_default(vartemp, VAR_LIST);
         printf("<td>&nbsp;</td>\n");
      }

      LMAPI->log_printf(18,"CGI: %s = %s\n", vartemp->name, val);

      printf(" <td valign=top>%s</td>\n", vartemp->name);
      printf(" <td valign=top>");

      switch (vartemp->type) {
         case VAR_STRING:
         case VAR_INT:
         case VAR_TIME:
            printf("  <input name=\"lcgipl-%s-value\"", vartemp->name);
            if (vartemp->type == VAR_STRING) {
               printf(" size=30");
            }
            if (val) {
               printf(" value=\"%s\"", val);
            }
            printf(">\n");
            break;

         case VAR_DURATION:
            {
               int days, hours, mins, secs;
               int parse;
               const char *tchr;
               
               days = hours = mins = secs = 0;

               parse = 0;

               tchr = val;

               while (tchr ? *tchr : 0) {
                  while (*tchr && isspace((int)(*tchr))) tchr++;
                  while (*tchr && isdigit((int)(*tchr))) { parse = parse * 10 + (*tchr - '0'); tchr++; }
                  while (*tchr && isspace((int)(*tchr))) tchr++;
                  if (*tchr) {
                     switch(*tchr) {
                        case 'd': days = parse; parse = 0; tchr++; break;
                        case 'h': hours = parse; parse = 0; tchr++; break;
                        case 'm': mins = parse; parse = 0; tchr++; break;
                        default: secs = parse; parse = 0; tchr++; break;
                     }
                  }
               }

               printf("\n   <input name=\"lcgipl-%s-value-days\" value=\"%d\" size=2> d\n", vartemp->name, days);
               printf("   <input name=\"lcgipl-%s-value-hours\" value=\"%d\" size=2> h\n", vartemp->name, hours);
               printf("   <input name=\"lcgipl-%s-value-mins\" value=\"%d\" size=2> m\n", vartemp->name, mins);
               printf("   <input name=\"lcgipl-%s-value-secs\" value=\"%d\" size=2> s\n  ", vartemp->name, secs);
            }
            break;

         case VAR_BOOL:
            printf("<input type=checkbox name=\"lcgipl-%s-value\"",
               vartemp->name);
            if (val ? (*val == '1') : 0) {
               printf(" checked=true");
            }
            printf(">");
            break;

         case VAR_CHOICE:
            {
               char choices[BIG_BUF], *tmpptr, *tmpptr2;

               LMAPI->log_printf(18,"CGI: %s (%s)\n", vartemp->name, vartemp->choices);

               printf("\n   <select name=\"lcgipl-%s-value\" size=1>\n",
                  vartemp->name);
               stringcpy(choices, vartemp->choices);

               tmpptr = &choices[1];
               while(*tmpptr) {
                  tmpptr2 = strchr(tmpptr,'|');
                  *tmpptr2++ = 0;

                  if (strcasecmp(tmpptr,val) == 0) {
                    printf("    <option value=\"%s\" selected> %s\n",
                      tmpptr, tmpptr);
                  } else {
                    printf("    <option value=\"%s\"> %s\n", tmpptr, tmpptr);
                  }

                  tmpptr = tmpptr2;
               }
               printf("   </select>\n");
            }
            break;

         default:
            printf("<b>UNSETTABLE VARIABLE!  ERROR!</b>");
            break;
      }
      
      printf("</td>\n");
      if (vartemp->description) {
         printf(" <td valign=top>%s\n",
           vartemp->description);
         printf("<BR>&nbsp;</td>\n");
      } else {
         printf(" <td>&nbsp;<BR><BR>&nbsp;</td>\n");
      }
      printf("</tr>");

      vartemp = LMAPI->next_varlist();
   }
   LMAPI->finish_varlist();

   printf("</table>\n");

   return 1;
}

CGI_MODE(cgimode_setconfig)
{
   char buffer[BIG_BUF];
   const char *tempval;
   struct var_data *var;
   struct listserver_cgi_tempvar *cvar;
   int override, durtotal;

   durtotal = 0;

   if (!lsg2_admin_validate())
      return;

   var = LMAPI->start_varlist();

   while (var) {
      if (!(var->flags & VAR_LIST) || (var->flags & VAR_INTERNAL)) {
         var = LMAPI->next_varlist();
         continue;
      }

      LMAPI->log_printf(18,"CGI: Config submit: checking '%s'...\n",
        var->name);

      LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s-set",
         var->name);

      cvar = LMAPI->find_cgi_tempvar(buffer);

      tempval = LMAPI->get_cur_varval_level(var,VAR_LIST);

      override = 0;

      if (cvar) LMAPI->log_printf(18,"CGI: Config (%s set '%s')\n",
         var->name, cvar->value);

      if (cvar ? strcasecmp(cvar->value,"on") == 0 : !tempval) {
         char varbuffer[BIG_BUF];

         LMAPI->log_printf(18,"CGI: Entering the variable check loop...\n");
         
         if (cvar ? strcasecmp(cvar->value,"on") == 0 : 0) override = 1;

            switch(var->type) {
               case VAR_STRING:
               case VAR_INT:
               case VAR_TIME:
               case VAR_CHOICE:
                  LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s-value",
                     var->name);
                  cvar = LMAPI->find_cgi_tempvar(buffer);
                  if (cvar) {
                     LMAPI->buffer_printf(varbuffer, sizeof(varbuffer) - 1,
                        "%s", cvar->value);
                  } else {
                     memset(varbuffer, 0, sizeof(varbuffer));
                     LMAPI->clean_var(var->name,VAR_LIST);
                  }
                  break;

               case VAR_DURATION:
                  {
                     int days, hours, mins, secs;
                     char daystr[SMALL_BUF], hourstr[SMALL_BUF], minstr[SMALL_BUF], secstr[SMALL_BUF];

                     days = hours = mins = secs = 0;

                     LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s-value-days",
                         var->name);
                     cvar = LMAPI->find_cgi_tempvar(buffer);
                     if (cvar)
                        days = atoi(cvar->value);
                     LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s-value-hours",
                         var->name);
                     cvar = LMAPI->find_cgi_tempvar(buffer);
                     if (cvar)
                        hours = atoi(cvar->value);
                     LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s-value-mins",
                         var->name);
                     cvar = LMAPI->find_cgi_tempvar(buffer);
                     if (cvar)
                        mins = atoi(cvar->value);
                     LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s-value-secs",
                         var->name);
                     cvar = LMAPI->find_cgi_tempvar(buffer);
                     if (cvar)
                        secs = atoi(cvar->value);

                     LMAPI->buffer_printf(daystr, sizeof(daystr) - 1, "%d d ", days);
                     LMAPI->buffer_printf(hourstr, sizeof(hourstr) - 1, "%d h ", hours);
                     LMAPI->buffer_printf(minstr, sizeof(minstr) - 1, "%d m ", mins);
                     LMAPI->buffer_printf(secstr, sizeof(secstr) - 1, "%d s", secs);

                     durtotal = secs + (mins * 60) + (hours * 3600) +
                                (days * 86400);

                     LMAPI->buffer_printf(varbuffer, sizeof(varbuffer) - 1,
                       "%s%s%s%s",
                       days ? daystr : "", hours ? hourstr : "",
                       mins ? minstr : "", secs ? secstr : "");
                  }
                  break;

               case VAR_BOOL:
                  LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s-value",
                     var->name);
                  if ((cvar = LMAPI->find_cgi_tempvar(buffer))) {
                     if (strcasecmp(cvar->value,"on") == 0) {
                        LMAPI->buffer_printf(varbuffer, sizeof(varbuffer) - 1, "true");
                     } else {
                        LMAPI->buffer_printf(varbuffer, sizeof(varbuffer) - 1, "false");
                     }
                  } else {
                     LMAPI->buffer_printf(varbuffer, sizeof(varbuffer) - 1, "false");
                  }
                  break;

               default:
                  break;            
            }

            if (!override) {
               const char *val;

               val = LMAPI->get_cur_varval_level(var,VAR_LIST);

               if (!val) {
                  val = LMAPI->get_cur_varval_level_default(var,VAR_LIST);

                  LMAPI->log_printf(18, "CGI: %s = %s (now %s)\n",
                    var->name, val, varbuffer);

                  if (val) {
                     switch (var->type) {
                        case VAR_STRING:
                        case VAR_CHOICE:
                          if (strcasecmp(val,varbuffer) != 0) {
                             LMAPI->set_var(var->name,varbuffer,VAR_LIST);
                          }
                          break;

                        case VAR_INT:
                        case VAR_TIME:
                          if (LMAPI->get_number(var->name) != atoi(varbuffer)) {
                             LMAPI->set_var(var->name,varbuffer,VAR_LIST);
                          }
                          break;

                        case VAR_BOOL:
                          LMAPI->log_printf(18,"CGI: %s: %d / %d\n",
                             var->name, LMAPI->get_bool(var->name),
                             (strcasecmp("true",varbuffer) == 0));
                          if (LMAPI->get_bool(var->name) !=
                              (strcasecmp("true",varbuffer) == 0)) {
                             LMAPI->set_var(var->name,varbuffer,VAR_LIST);
                          }
                          break;

                        case VAR_DURATION:
                          if (LMAPI->get_seconds(var->name) != durtotal) {
                             LMAPI->set_var(var->name,varbuffer,VAR_LIST);
                          }
                          break;

                        default:
                          break;
                     }
                  } else {
                     if (varbuffer[0]) {
                        LMAPI->set_var(var->name,varbuffer,VAR_LIST);
                     }
                  }
               } else {
                  LMAPI->clean_var(var->name,VAR_LIST);
               }
            } else {
               LMAPI->set_var(var->name,varbuffer,VAR_LIST);
            }
      } else {
         LMAPI->clean_var(var->name,VAR_LIST);
      }

      var = LMAPI->next_varlist();
   }

   LMAPI->finish_varlist();

   LMAPI->log_printf(18,"CGI: strip-mdn = %s\n",
     LMAPI->get_bool("strip-mdn") ? "true" : "false");

   LMAPI->listdir_file(buffer,LMAPI->get_string("lcgi-list"),"config");
   LMAPI->write_configfile(buffer,VAR_LIST,NULL);

   LMAPI->log_printf(1,"CGI: %s: %s sent new config\n",
      LMAPI->get_string("lcgi-list"), LMAPI->get_string("lcgi-user"));

   if (!LMAPI->cgi_unparse_template("adminmenu")) {
      lsg2_internal_error("No 'adminmenu' template.");
   }

   return;
}

CGI_MODE(cgimode_getfile)
{
   const char *getfile;
   struct list_file *tfile;

   if (!lsg2_admin_validate()) {
      return;
   }

   getfile = LMAPI->get_var("lcgi-adminfile");
   if (!getfile) {
      lsg2_internal_error("Mode initialized wrong.");
      return;
   }

   tfile = LMAPI->find_file(getfile);
   if (!tfile) {
      lsg2_internal_error("Attempt to retrieve a nonexistant file.");
      return;
   }

   LMAPI->set_var("tlcgi-editfile", LMAPI->get_var(tfile->varname),
     VAR_TEMP);

   LMAPI->set_var("tlcgi-editfile-desc", tfile->desc, VAR_TEMP);

   if (!LMAPI->cgi_unparse_template("fileedit")) {
      lsg2_internal_error("No 'fileedit' template.");
   }

   return;
}

int file_cmp(const void *e1, const void *e2)
{
   struct list_file *f1, *f2;
   f1 = *(struct list_file **)e1;
   f2 = *(struct list_file **)e2;

   return (strcasecmp(f1->name,f2->name));
}

CGI_HANDLER(cgihook_adminfilelistdrop)
{
   struct list_file *tfile;
   struct list_file **filearr;
   int counter, i;

   tfile = LMAPI->get_files();

   counter = 0;

   while (tfile) {
      tfile = tfile->next;
      counter++;
   }


   filearr = (struct list_file **)malloc(sizeof(struct list_file *) * counter);

   tfile = LMAPI->get_files();
   for (i = 0; i < counter; i++) {
      filearr[i] = tfile;
      tfile = tfile->next;
   }

   qsort(filearr,counter,sizeof(struct list_file *),file_cmp);

   printf("<select name=\"lcgi-adminfile\" size=6>\n");

   for (i = 0; i < counter; i++) {
      if (!(filearr[i]->flags & FILE_NOWEBEDIT))
         printf("<option value=\"%s\"> %s\n", filearr[i]->name,
            filearr[i]->name);
   }
   printf("</select>\n");

   free(filearr);

   return 1;
}

CGI_HANDLER(cgihook_fileedit)
{
   const char *filepath;
   FILE *infile;
   char buffer[BIG_BUF];

   filepath = LMAPI->get_var("tlcgi-editfile");
   if (!filepath) return 0;

   LMAPI->listdir_file(buffer,LMAPI->get_string("lcgi-list"),filepath);

   infile = LMAPI->open_file(buffer,"r");

   printf("<textarea name=\"lcgipl-textbody\" rows=18 cols=76 wrap=physical>");

   if (infile) {
      while(LMAPI->read_file(buffer, sizeof(buffer), infile)) {
         printf("%s",buffer);
      }
      LMAPI->close_file(infile);
   } else {
      printf("** File was empty **\n");
   }
   printf("</textarea>");

   return 1;
}

CGI_MODE(cgimode_putfile)
{
   const char *getfile;
   struct list_file *tfile;
   FILE *outfile;
   char buffer[BIG_BUF];
   struct listserver_cgi_tempvar *cvar;

   if (!lsg2_admin_validate()) {
      return;
   }

   getfile = LMAPI->get_var("lcgi-adminfile");
   if (!getfile) {
      lsg2_internal_error("Mode initialized wrong.");
      return;
   }

   LMAPI->log_printf(18,"CGI: Made it into putfile...\n");

   tfile = LMAPI->find_file(getfile);
   if (!tfile) {
      lsg2_internal_error("Attempt to replace a nonexistant file.");
      return;
   }

   cvar = LMAPI->find_cgi_tempvar("textbody");

   if (!cvar) {
      /* The entire textbox contents were deleted... the file is gone */

      LMAPI->listdir_file(buffer,LMAPI->get_string("lcgi-list"),
            LMAPI->get_string(tfile->varname));
      LMAPI->unlink_file(buffer);
      return;
   }

   LMAPI->log_printf(18,"CGI: We have our textbody...\n");

   LMAPI->listdir_file(buffer,LMAPI->get_string("lcgi-list"),LMAPI->get_string(tfile->varname));

   LMAPI->log_printf(18,"CGI: Attempting to put file %s\n", buffer);

   if ((outfile = LMAPI->open_file(buffer,"w")) == NULL) {
      lsg2_internal_error("File access error.");
      return;
   }

   LMAPI->write_file(outfile,"%s",cvar->value);
   LMAPI->close_file(outfile);

   LMAPI->log_printf(1,"CGI: %s: %s replaced '%s' \n",
      LMAPI->get_string("lcgi-list"), LMAPI->get_string("lcgi-user"),
      tfile->name);

   if (!LMAPI->cgi_unparse_template("adminmenu")) {
      lsg2_internal_error("No 'adminmenu' template.");
   }

   return;
}

CGI_MODE(cgimode_admin_subscribe)
{
   char userfile[BIG_BUF];
   char tempbuf[BIG_BUF];
   const char *from;
   char *inputbuffer, *ptr1, *ptr2;
   struct listserver_cgi_tempvar *cvar;

   if (!lsg2_admin_validate()) {
      return;
   }

   from = LMAPI->get_var("lcgi-user");

   cvar = LMAPI->find_cgi_tempvar("subscribe-userlist");

   if (!cvar) {
      lsg2_internal_error("No users provided.");
   }

   inputbuffer = strdup(cvar->value);

   LMAPI->set_var("adminmode","true",VAR_GLOBAL);
   LMAPI->set_var("fromaddress",from,VAR_GLOBAL);
   LMAPI->set_var("realsender",from,VAR_GLOBAL);

   ptr1 = inputbuffer;


   while(ptr1 && *ptr1) {
      char *tptr;

      while(isspace((int)(*ptr1))) ptr1++;

      ptr2 = strchr(ptr1,'\n');

      tptr = NULL;

      if (ptr2) {
         *ptr2++ = 0;
         if (*ptr2 == 0x0d) *ptr2++ = 0;
      }

      tptr = strchr(ptr1,0x0d);
      if (tptr) *tptr = 0;

      if (!strlen(ptr1) || !strchr(ptr1,'@') || !strchr(ptr1,'.')) {
         ptr1 = ptr2;
         continue;
      }

      LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "subscribe %s",
        ptr1);
      LMAPI->set_var("cur-parse-line", tempbuf, VAR_GLOBAL);
      LMAPI->set_var("subscribe-me",ptr1,VAR_GLOBAL);

      if (LMAPI->do_hooks("PRESUB") == HOOK_RESULT_FAIL) {
         ptr1 = ptr2;
         continue;      
      }

      LMAPI->listdir_file(userfile,LMAPI->get_string("list"),"users");

      if (LMAPI->user_add(userfile,ptr1)) {
          LMAPI->spit_status("Filesystem error while subscribing user.");
          ptr1 = ptr2;
          continue;
      }

      LMAPI->log_printf(0, "%s subscribed to %s by %s\n",ptr1,          
         LMAPI->get_string("listname"),from);

      LMAPI->do_hooks("POSTSUB");

      ptr1 = ptr2;
   }

   LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s.perr");
   if (LMAPI->exists_file(tempbuf)) {
      LMAPI->set_var("tlcgi-textfile",tempbuf,VAR_TEMP);
      if (!LMAPI->cgi_unparse_template("textfile")) {
          lsg2_internal_error("No 'textfile' template.");
      }
      LMAPI->unlink_file(tempbuf);
      return;
   }

   if (!LMAPI->cgi_unparse_template("adminmenu")) {
      lsg2_internal_error("No 'adminmenu' template.");
   }

   return;
}

CGI_MODE(cgimode_admin_unsubscribe)
{
   char userfile[BIG_BUF];
   const char *from, *userfor;
   char tempbuf[BIG_BUF];

   if (!lsg2_admin_validate()) {
      return;
   }

   from = LMAPI->get_var("lcgi-user");
   userfor = LMAPI->get_var("lcgi-userfor");

   LMAPI->set_var("adminmode","true",VAR_GLOBAL);

   if (!userfor) {
      lsg2_internal_error("Mode initialized wrong.");
      return;
   }

   LMAPI->set_var("subscribe-me",userfor,VAR_GLOBAL);
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
      return;      
   }

   LMAPI->listdir_file(userfile,LMAPI->get_string("list"),"users");

   if (LMAPI->user_remove(userfile,userfor)) {
       lsg2_internal_error("Filesystem error while unsubscribing user.");
       LMAPI->filesys_error(userfile);
       LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s.perr",
          LMAPI->get_string("queuefile"));
       LMAPI->unlink_file(tempbuf);
       return;
   }

   LMAPI->log_printf(0, "%s unsubscribed from %s by %s\n",userfor,
      LMAPI->get_string("listname"),from);   

   LMAPI->do_hooks("POSTUNSUB");

   if (!LMAPI->cgi_unparse_template("adminmenu")) {
      lsg2_internal_error("No 'adminmenu' template.");
      return;
   }

   return;
}

CGI_MODE(cgimode_admin_setflags)
{
   struct list_user user;
   const char *list, *fromaddy;
   struct listserver_flag *tflag;
   int isadmin;
   char tbuf[BIG_BUF];

   if (!lsg2_admin_validate()) {
      return;
   }

   list = LMAPI->get_var("lcgi-list");
   fromaddy = LMAPI->get_var("lcgi-userfor");

   LMAPI->set_var("realsender",LMAPI->get_var("lcgi-user"),VAR_GLOBAL);
   LMAPI->set_var("fromaddress",LMAPI->get_var("lcgi-user"),VAR_GLOBAL);

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
          (tflag->admin & ADMIN_UNSETTABLE)) {
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
                  LMAPI->user_setflag(&user,tflag->name,1);
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
                  LMAPI->user_unsetflag(&user,tflag->name,1);
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
      if (!LMAPI->cgi_unparse_template("admin-userinfo")) {
         lsg2_internal_error("No 'admin-userinfo' template.");
      }
   }
}

CGI_MODE(cgimode_admin_setname)
{
   const char *list, *user, *name;

   if (!lsg2_admin_validate()) {
      return;
   }

   list = LMAPI->get_var("lcgi-list");
   user = LMAPI->get_var("lcgi-userfor");
   name = LMAPI->get_var("lcgi-fullname");

   if (!list || !user || !name) {
      lsg2_internal_error("Mode not initialized properly.");
      return;
   }
                                       
   LMAPI->userstat_set_stat(list,user,"realname",name);

   if (!LMAPI->cgi_unparse_template("admin-userinfo")) {
      lsg2_internal_error("No 'admin-userinfo' template.");
   }   

   return;
}

CGI_MODE(cgimode_admin_userinfo)
{
   const char *list, *user;

   if (!lsg2_admin_validate()) {
      return;
   }

   list = LMAPI->get_var("lcgi-list");
   user = LMAPI->get_var("lcgi-userfor");

   if (!list || !user) {
      lsg2_internal_error("Mode not initialized properly.");
      return;
   }
                                       
   if (!LMAPI->cgi_unparse_template("admin-userinfo")) {
      lsg2_internal_error("No 'admin-userinfo' template.");
   }   

   return;
}

CGI_MODE(cgimode_admin_usersetinfo)
{
   const char *list, *user, *target;
   struct list_user realuser, realtarget;

   if (!lsg2_admin_validate()) {
      return;
   }

   list = LMAPI->get_var("lcgi-list");
   user = LMAPI->get_var("lcgi-user");
   target = LMAPI->get_var("lcgi-userfor");

   if (!list || !user) {
      lsg2_internal_error("Mode not initialized properly.");
      return;
   }

   if (LMAPI->user_find_list(list,target,&realtarget)) {
      if (LMAPI->user_find_list(list,user,&realuser)) {

         if (LMAPI->user_hasflag(&realtarget,"ADMIN") &&
             !LMAPI->user_hasflag(&realuser,"SUPERADMIN")) {
                lsg2_internal_error("You are not a SUPERADMIN and cannot alter options on other admins.");
                return;
         }
      }
   }
                                       
   if (!LMAPI->cgi_unparse_template("admin-usersetinfo")) {
      lsg2_internal_error("No 'admin-usersetinfo' template.");
   }   

   return;
}

CGI_MODE(cgimode_admin_userfor_generic)
{
   if (!lsg2_admin_validate()) {
      return;
   }

   if (LMAPI->find_cgi_tempvar("unsubscribe")) {
      cgimode_admin_unsubscribe();
      return;
   }

   if (LMAPI->find_cgi_tempvar("userinfo")) {
      cgimode_admin_userinfo();
      return;
   }

   if (LMAPI->find_cgi_tempvar("usersetinfo")) {
      cgimode_admin_usersetinfo();
   }
}

CGI_HANDLER(cgihook_admin_flaglist)
{
  int cols, curcol;
  struct listserver_flag *tflag;
  struct listserver_flag **flagarr;
  struct list_user user;
  int isadmin, counter, i;

  cols = 2;

  if (param && *param) {
    cols = atoi(param);
    if (!cols) cols = 2;
  }

  if (cols < 1) cols = 1;

  tflag = LMAPI->get_flags();

  curcol = 1;

  if (!LMAPI->user_find_list(LMAPI->get_string("lcgi-list"),
        LMAPI->get_string("lcgi-userfor"), &user)) {
     printf("<b>You are not subscribed to this list.</b>");
     return 0;
  }

  isadmin = LMAPI->user_hasflag(&user,"ADMIN");
  
  printf("<table border=0 width=100%%>\n");
  printf(" <tr>\n");

  counter = 0;

  while(tflag) {
     counter++;
     tflag = tflag->next;
  }

  flagarr = (struct listserver_flag **)malloc(sizeof(struct listserver_flag *) * counter);

  tflag = LMAPI->get_flags();

  for (i = 0; i < counter; i++) {
     flagarr[i] = tflag;
     tflag = tflag->next;
  }

  qsort(flagarr,counter,sizeof(struct listserver_flag *),flag_cmp);

  for (i = 0; i < counter; i++) {
     if (!curcol) curcol = 1;

     tflag = flagarr[i];

     if (strcasecmp(tflag->name,"superadmin") || 
        !(tflag->admin & ADMIN_UNSETTABLE)) {

        printf("  <td valign=top width=%d%%>\n", 100 / cols);
        printf("   <input type=\"checkbox\" name=\"lcgipl-%s\"",
          tflag->name);
        if (LMAPI->user_hasflag(&user,tflag->name)) {
          printf(" checked=\"true\"");
        }
        printf("> %s\n", tflag->name);
        printf("   <ul>%s</ul>\n", tflag->desc);
        printf("  </td>");

        curcol++;

        if (curcol > cols) {
           printf(" </tr>\n<tr>\n");
           curcol = 0;
        }
     } 
  }

  if (curcol != 0) {
     while(curcol < cols) {
        printf("  <td>&nbsp;</td>\n");
        curcol++;
     }
     printf(" </tr>\n");
  }

  free(flagarr);

  printf("</table>");
  return 1;
}

CGI_HANDLER(cgihook_admin_setname)
{
  char fullname[256];
  const char *user, *list;

  user = LMAPI->get_var("lcgi-userfor");
  list = LMAPI->get_var("lcgi-list");

  if (!user || !list) return 0;

  if (LMAPI->userstat_get_stat(list,user,"realname",&fullname[0],255)) {
     printf("<input name=\"lcgi-fullname\" length=255 value=\"%s\">\n",
        fullname);
  } else {
     printf("<input name=\"lcgi-fullname\" length=255>\n");
  }
  printf("<input name=\"lcgi-userfor\" type=hidden value=\"%s\">\n",
        LMAPI->get_var("lcgi-userfor"));

  return 1;
}

CGI_HANDLER(cgihook_admin_showflags)
{
  int cols, curcol, width;
  struct listserver_flag *tflag;
  struct listserver_flag **flagarr;
  struct list_user user;
  int counter, i;

  cols = 2;
  width = 100;

  if (param && *param) {
    char *parseme;
    char unparse[BIG_BUF];

    stringcpy(unparse,param);

    if ((parseme = strchr(unparse,','))) {
       *parseme++ = 0;
       width = atoi(parseme);
    }
    cols = atoi(unparse);
    if (!cols) cols = 2;
  }

  if (cols < 1) cols = 1;
  if (width > 100) width = 100;
  if (width < 0) width = 0;

  tflag = LMAPI->get_flags();

  curcol = 1;

  if (!LMAPI->user_find_list(LMAPI->get_string("lcgi-list"),
        LMAPI->get_string("lcgi-userfor"), &user)) {
     printf("<b>%s is not subscribed to this list.</b>",
        LMAPI->get_string("lcgi-userfor"));
     return 0;
  }

  counter = 0;

  while(tflag) {
     counter++;
     tflag = tflag->next;
  }

  flagarr = (struct listserver_flag **)malloc(sizeof(struct listserver_flag *) * counter);

  tflag = LMAPI->get_flags();

  for (i = 0; i < counter; i++) {
     flagarr[i] = tflag;
     tflag = tflag->next;
  }

  qsort(flagarr,counter,sizeof(struct listserver_flag *),flag_cmp);
  
  printf("<table border=0");
  if (width) {
     printf(" width=%d%%", width);
  }
  printf(">\n");
  printf(" <tr>\n");
  
  for(i = 0; i < counter; i++) {
     if (!curcol) curcol = 1;
     tflag = flagarr[i];
     if (!LMAPI->user_hasflag(&user,tflag->name)) {
        tflag = tflag->next;
        continue;
     }

     printf("  <td valign=top>\n");
     printf("<b>%s</b>\n", tflag->name);
     printf("  </td><td valign=top>\n");
     printf("   %s\n", tflag->desc);
     printf("  </td>");

     tflag = tflag->next;
     curcol++;

     if (curcol > cols) {
        printf(" </tr>\n<tr>\n");
        curcol = 0;
     } 
  }

  if (curcol != 0) {
     while(curcol < cols) {
        printf("  <td>&nbsp;</td><td>&nbsp;</td>\n");
        curcol++;
     }
     printf(" </tr>\n");
  }

  free(flagarr);

  printf("</table>");
  return 1;
}

/*  
 * kjh: qsort compare function for e-mail addresses
 */
static int user_compare(const void *p1, const void *p2)
{ 
  /* convert params to their "real" type */
  const char *u1 = (const char *) *((const char **) p1);   
  const char *u2 = (const char *) *((const char **) p2);
  /* parse out the domain */
  const char *domain1 = strchr(u1, '@');
  const char *domain2 = strchr(u2, '@');
  int   rv = 0;
      
  /* skip past the '@' sign, otherwise make sure that we have an ptr to a
   * deferenceable string (just in case there wasn't a domain)
   */
  if (domain1 != 0)
    domain1++; 
  else
    domain1 = "";
   
  if (domain2 != 0)
    domain2++;
  else
    domain2 = "";
      
  rv = strcasecmp(domain1, domain2);  /* compare the domain */
   
//  if (rv == 0)                        /* if same domain, compare user */
    rv = strcasecmp(u1, u2);
   
  return rv;
}



CGI_HANDLER(cgihook_admin_userlistbox)
{
  FILE *infile;
  char filename[BIG_BUF];
  struct list_user user;
  int   n_users = 256;
  const char **users = 0;
  int   n = 0;

  LMAPI->listdir_file(filename,LMAPI->get_string("lcgi-list"),"users");

  if ((infile = LMAPI->open_file(filename,"r")) == NULL) 
     return 0;

  printf("<select name=\"lcgi-userfor\"");
  if (param) {
    int size;

    size = atoi(param);
    if (size < 1) size = 1;
    printf(" size=%d", size);
  } else {
    printf(" size=20");
  }
  printf(">\n");

  if (LMAPI->get_bool("lsg2-sort-userlist")) {
     users = (const char **)malloc(n_users * sizeof(char *));
      
     while(LMAPI->user_read(infile,&user)) {
       if (n == n_users) {
         n_users *= 2;    
         users = realloc((void*)users, n_users*sizeof(char *));
       }
       users[n++] = strdup(user.address);
     }
 
     /* kjh: sort users by e-mail address */
     qsort((void*)users, n, sizeof(const char *), user_compare); 
  
     {
       const char **userp;
       for (userp = users; userp < users+n; userp++) {
         printf("<option value=\"%s\"> %s\n",
                *userp, *userp);
         free((char *) *userp);
       }
     }   
     free((void*)users);
  }
  else {
     while(LMAPI->user_read(infile,&user)) {
        printf("<option value=\"%s\"> %s\n",
          user.address, user.address);
     }
     printf("</select>\n");
  }
 
  printf("</select>\n");
   

  return 1;
}

