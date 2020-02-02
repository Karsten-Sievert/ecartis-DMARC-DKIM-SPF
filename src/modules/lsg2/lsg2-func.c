#include <stdlib.h>
#include <string.h>

#include "lsg2.h"

int flag_cmp(const void *e1, const void *e2)
{
   struct listserver_flag *f1, *f2;

   f1 = *(struct listserver_flag **)e1;
   f2 = *(struct listserver_flag **)e2;

   return (strcasecmp(f1->name,f2->name));
}

CGI_HANDLER(cgihook_modehead)
{
  printf("<form action=\"%s\" method=post>\n",
    LMAPI->get_string("lsg2-cgi-url"));
  printf("<input type=hidden name=\"lcgi-mode\" value=\"%s\">\n", param);
  if (LMAPI->get_var("lcgi-mode")) {
     printf("<input type=hidden name=\"lcgi-lastmode\" value=\"%s\">\n",
      LMAPI->get_var("lcgi-mode"));
  }
  return 1;
}

CGI_HANDLER(cgihook_modeheadex)
{
  printf("<form action=\"%s\" method=post>\n",
    LMAPI->get_string("lsg2-cgi-url"));
  printf("<input type=hidden name=\"lcgi-mode\" value=\"%s\">\n", param);

  if (LMAPI->get_var("lcgi-mode")) {
     printf("<input type=hidden name=\"lcgi-lastmode\" value=\"%s\">\n",
      LMAPI->get_var("lcgi-mode"));
  }

  if (LMAPI->get_var("lcgi-user")) {
    printf("<input name=\"lcgi-user\" type=hidden value=\"%s\">\n", LMAPI->get_var("lcgi-user"));
  } 

  if (LMAPI->get_var("lcgi-cookie")) {
    printf("<input name=\"lcgi-cookie\" type=hidden value=\"%s\">\n",
      LMAPI->get_var("lcgi-cookie"));
  }

  if (LMAPI->get_var("list")) {
    printf("<input name=\"lcgi-list\" type=hidden value=\"%s\">\n",
      LMAPI->get_string("list"));
  }

  return 1;
}

CGI_HANDLER(cgihook_modeend)
{
  printf("</form>\n");
  return 1;
}

CGI_HANDLER(cgihook_editusername)
{
  printf("<input name=\"lcgi-user\"");
  if (LMAPI->get_var("lcgi-user")) {
    printf("value=\"%s\" ", LMAPI->get_var("lcgi-user"));
  }
  if (param ? atoi(param) : 0) {
    printf("length=%d", atoi(param));
  }
  printf(">\n");
  return 1;
}

CGI_HANDLER(cgihook_username)
{
  printf("<input name=\"lcgi-user\"");
  if (LMAPI->get_var("lcgi-user")) {
    printf(" type=hidden value=\"%s\"", LMAPI->get_var("lcgi-user"));
  } 
  else if (param ? atoi(param) : 0) {
    printf(" length=%d", atoi(param));
  }
  printf(">\n");
  return 1;
}

CGI_HANDLER(cgihook_authcookie)
{
  printf("<input name=\"lcgi-cookie\"");
  if (LMAPI->get_var("lcgi-cookie")) {
    printf("type=hidden value=\"%s\" ", LMAPI->get_var("lcgi-cookie"));
  }
  else if (param ? atoi(param) : 0) {
    printf("length=%d", atoi(param));
  }
  printf(">\n");
  return 1;
}

CGI_HANDLER(cgihook_password)
{
  printf("<input type=password name=\"lcgi-pass\"");
  if (param ? atoi(param) : 0) {
    printf("length=%d", atoi(param));
  }
  printf(">\n");
  return 1;
}

CGI_HANDLER(cgihook_submit)
{
  printf("<input type=submit");
  if (param) {
    printf(" value=\"%s\"", param);
  }
  printf(">\n");
  return 1;
}

void lsg2_lists_selectbox()
{
  int status;
  char dname[BIG_BUF];
  struct list_user user;

   printf("<select name=\"lcgi-list\" size=1>\n");

   if (!(status = LMAPI->walk_lists(&dname[0]))) {
       printf("<option value=\".uhoh.\">-- Unable to access lists --\n");
   } else {
       while (status) {
          if(LMAPI->list_valid(dname)) {  
              LMAPI->clean_var("advertise", VAR_TEMP);
              LMAPI->read_conf_parm(dname,"advertise", VAR_TEMP);
              if (LMAPI->user_find_list(dname,LMAPI->get_string("lcgi-user"),&user)) {
                 printf("\t<option value=\"%s\"> %s (Subscribed)\n",
                   dname, dname);
              } else {
                 if (LMAPI->get_bool("advertise")) {
                    printf("\t<option value=\"%s\"> %s\n", dname, dname);
                 }
              }
              LMAPI->clean_var("advertise", VAR_TEMP);
          }
          status = LMAPI->next_lists(&dname[0]);
       }   
   }
   printf("</select>\n");
}

void lsg2_lists_selectboxex(int listsize)
{
  int status;
  char dname[BIG_BUF];
  struct list_user user;
  int count = listsize;

   if (count == 0) {
      status = LMAPI->walk_lists(&dname[0]);
      if (!status) {
         printf("<option value=\".uhoh.\">-- Unable to access lists --\n");
      }

      while (status) {
         if (LMAPI->list_valid(&dname[0])) count++;
         status = LMAPI->next_lists(&dname[0]);
      }
   }

   printf("<select name=\"lcgi-list\" size=%d>\n", count);

   if (!(status = LMAPI->walk_lists(&dname[0]))) {
       printf("<option value=\".uhoh.\">-- Unable to access lists --\n");
   } else {
       while (status) {
          if(LMAPI->list_valid(dname)) {  
              LMAPI->clean_var("advertise", VAR_TEMP);
              LMAPI->read_conf_parm(dname,"advertise", VAR_TEMP);
              if (LMAPI->user_find_list(dname,LMAPI->get_string("lcgi-user"),&user)) {
                 printf("\t<option value=\"%s\"> %s (Subscribed)\n",
                   dname, dname);
              } else {
                 if (LMAPI->get_bool("advertise")) {
                    printf("\t<option value=\"%s\"> %s\n", dname, dname);
                 }
              }
              LMAPI->clean_var("advertise", VAR_TEMP);
          }
          status = LMAPI->next_lists(&dname[0]);
       }   
   }
   printf("</select>\n");
}

CGI_HANDLER(cgihook_lists)
{
   lsg2_lists_selectbox();
   return 1;
}

CGI_HANDLER(cgihook_listsex)
{
   lsg2_lists_selectboxex(param ? atoi(param) : 0);
   return 1;
}

CGI_HANDLER(cgihook_display_textfile)
{
   if (LMAPI->get_var("tlcgi-textfile")) {
      lsg2_html_textfile(LMAPI->get_string("tlcgi-textfile"));
   }
   return 1;
}

CGI_HANDLER(cgihook_welcome_button)
{
   char tempfile[BIG_BUF];

   LMAPI->listdir_file(tempfile,LMAPI->get_string("lcgi-list"),
      LMAPI->get_string("welcome-file"));

   if (LMAPI->exists_file(tempfile))
      printf("<input type=submit name=\"lcgipl-show-welcome\" value=\"Intro File\">\n");
   return 1;
}

CGI_HANDLER(cgihook_faq_button)
{
   char tempfile[BIG_BUF];

   LMAPI->listdir_file(tempfile,LMAPI->get_string("lcgi-list"),
      LMAPI->get_string("faq-file"));

   if (LMAPI->exists_file(tempfile))
      printf("<input type=submit name=\"lcgipl-show-faq\" value=\"FAQ\">\n");
   return 1;
}

CGI_HANDLER(cgihook_infofile_button)
{
   char tempfile[BIG_BUF];

   LMAPI->listdir_file(tempfile,LMAPI->get_string("lcgi-list"),
      LMAPI->get_string("info-file"));

   if (LMAPI->exists_file(tempfile))
      printf("<input type=submit name=\"lcgipl-show-info\" value=\"List Info File\">\n");
   return 1;
}

CGI_HANDLER(cgihook_curlist)
{
   if (LMAPI->get_var("list")) {
     printf("<input type=hidden name=\"lcgi-list\" value=\"%s\">",
       LMAPI->get_string("list"));
   }
   return 1;
}

void display_user(const char *format, struct list_user *user)
{
   char output[BIG_BUF], output2[BIG_BUF];
   char tempbuf[BIG_BUF];
   const char *list;

   list = LMAPI->get_string("list");

   LMAPI->strreplace(output, sizeof(output) - 1,format,"%u",user->address);
   stringcpy(output2, output);
   

   if (LMAPI->userstat_get_stat(list,user->address,"realname", tempbuf, sizeof(tempbuf) - 1)) {
      LMAPI->strreplace(output, sizeof(output) - 1,output2,"%n",tempbuf);
   } else {
      LMAPI->strreplace(output, sizeof(output) - 1,output2,"%n","");
   }
   stringcpy(output2, output);

   if (LMAPI->userstat_get_stat(list,user->address,"traffic", tempbuf, sizeof(tempbuf) - 1)) {
      char temp[SMALL_BUF];

      LMAPI->buffer_printf(temp, sizeof(temp) - 1, "%sk", tempbuf);
      LMAPI->strreplace(output, sizeof(output) - 1,output2,"%t",temp);
   } else {
      LMAPI->strreplace(output, sizeof(output) - 1,output2,"%t","");
   }
   stringcpy(output2, output);   

   if (LMAPI->userstat_get_stat(list,user->address,"posts", tempbuf, sizeof(tempbuf) - 1)) {
      LMAPI->strreplace(output, sizeof(output) - 1,output2,"%P",tempbuf);
   } else {
      LMAPI->strreplace(output, sizeof(output) - 1,output2,"%P","");
   }
   stringcpy(output2, output);   

   if (LMAPI->userstat_get_stat(list,user->address,"position", tempbuf, sizeof(tempbuf) - 1)) {
     LMAPI->strreplace(output, sizeof(output) - 1,output2,"%p",tempbuf);
   } else {
     LMAPI->strreplace(output, sizeof(output) - 1,output2,"%p","");
   }
   stringcpy(output2, output);

   LMAPI->strreplace(output, sizeof(output) - 1,output2,"%d",
     LMAPI->user_hasflag(user,"DIGEST") ? "DIGEST" : "");
   stringcpy(output2, output);

   LMAPI->strreplace(output, sizeof(output) - 1,output2,"%D",
     LMAPI->user_hasflag(user,"DIGEST") ? 
     LMAPI->get_string("lsg2-img-digest") : "");
   stringcpy(output2, output);

   LMAPI->strreplace(output, sizeof(output) - 1,output2,"%a",
     LMAPI->user_hasflag(user,"ADMIN") ? "ADMIN" : "");
   stringcpy(output2, output);

   LMAPI->strreplace(output, sizeof(output) - 1,output2,"%A",
     LMAPI->user_hasflag(user,"ADMIN") ? 
     LMAPI->get_string("lsg2-img-admin") : "");
   stringcpy(output2, output);

   LMAPI->strreplace(output, sizeof(output) - 1,output2,"%h",
     LMAPI->user_hasflag(user,"HIDDEN") ? "HIDDEN" : "");
   stringcpy(output2, output);

   LMAPI->strreplace(output, sizeof(output) - 1,output2,"%H",
     LMAPI->user_hasflag(user,"HIDDEN") ? 
     LMAPI->get_string("lsg2-img-hidden") : "");
   stringcpy(output2, output);

   LMAPI->strreplace(output, sizeof(output) - 1,output2,"%m",
     LMAPI->user_hasflag(user,"MODERATOR") ? "MODERATOR" : "");
   stringcpy(output2, output);

   LMAPI->strreplace(output, sizeof(output) - 1,output2,"%M",
     LMAPI->user_hasflag(user,"MODERATOR") ? 
     LMAPI->get_string("lsg2-img-moderator") : "");
   stringcpy(output2, output);

   LMAPI->strreplace(output, sizeof(output) - 1,output2,"%v",
     LMAPI->user_hasflag(user,"VACATION") ? "VACATION" : "");
   stringcpy(output2, output);

   LMAPI->strreplace(output, sizeof(output) - 1,output2,"%V",
     LMAPI->user_hasflag(user,"VACATION") ? 
     LMAPI->get_string("lsg2-img-vacation") : "");

   printf("%s\n",output);
}

CGI_HANDLER(cgihook_userlist)
{
   char tempbuffer[BIG_BUF];
   int isadmin, allow=0;
   const char *list;
   struct list_user user;
   FILE *userfile;
   unsigned long total, admins, moderators, digest, hidden, vacation;

   list = LMAPI->get_string("lcgi-list");

// CHANGES RW
// HATES IT! MURDER! DEATH! KILL!

   if (strcmp(LMAPI->get_string("who-status"),"public") == 0)
     { allow=1; }
   if ((LMAPI->user_find_list(list,LMAPI->get_string("lcgi-user"), &user)) &&
       (strcmp(LMAPI->get_string("who-status"),"private") == 0))
     { allow=1; }
   if (LMAPI->user_hasflag(&user,"ADMIN"))
     { allow=1;
       isadmin=1;
     }
     else { isadmin=0; }

   if (allow == 0)
     { printf("<B>Permission denied.</b>");
       return 1;
     }

   LMAPI->listdir_file(tempbuffer,list,"users");

   if ((userfile = LMAPI->open_file(tempbuffer,"r")) == NULL) {
      printf("<B>permission denied.</b>");
   }

   total = admins = moderators = digest = hidden = vacation = 0;

   while(LMAPI->user_read(userfile,&user)) {
      total++;
      if (LMAPI->user_hasflag(&user,"ADMIN")) admins++;
      if (LMAPI->user_hasflag(&user,"MODERATORS")) moderators++;
      if (LMAPI->user_hasflag(&user,"DIGEST")) digest++;
      if (LMAPI->user_hasflag(&user,"HIDDEN")) hidden++;
      if (LMAPI->user_hasflag(&user,"VACATION")) vacation++;

      if (!LMAPI->user_hasflag(&user,"HIDDEN") || isadmin)
         display_user(param,&user);
   }

   LMAPI->buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%lu", total);
   LMAPI->set_var("tlcgi-users-total",tempbuffer,VAR_TEMP);

   LMAPI->buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%lu", admins);
   LMAPI->set_var("tlcgi-users-admins",tempbuffer,VAR_TEMP);
   
   LMAPI->buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%lu", moderators);
   LMAPI->set_var("tlcgi-users-moderators",tempbuffer,VAR_TEMP);
   
   LMAPI->buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%lu", hidden);
   LMAPI->set_var("tlcgi-users-hidden",tempbuffer,VAR_TEMP);
   
   LMAPI->buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%lu", digest);
   LMAPI->set_var("tlcgi-users-digest",tempbuffer,VAR_TEMP);
   
   LMAPI->buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%lu", vacation);
   LMAPI->set_var("tlcgi-users-vacation",tempbuffer,VAR_TEMP);

   return 1;   
}

CGI_HANDLER(cgihook_liscript)
{
   char tempbuffer[BIG_BUF];
   char escape[BIG_BUF];

   if (!param) return 1;

   LMAPI->strreplace(tempbuffer, sizeof(tempbuffer) - 1,param,"{","[");
   stringcpy(escape, tempbuffer);
   LMAPI->strreplace(tempbuffer, sizeof(tempbuffer) - 1,escape,"}","]");
   stringcpy(escape, tempbuffer);

   if (LMAPI->liscript_parse_line(escape, tempbuffer, sizeof(tempbuffer) - 1)) {
      printf("%s",tempbuffer);
   }

   return 1;
}

CGI_HANDLER(cgihook_subscribe)
{
  if (!LMAPI->get_var("lcgi-cookie") || !LMAPI->get_var("lcgi-user") ||
      !LMAPI->get_var("list")) return 1;

  printf("<form action=\"%s\" method=post>\n",
    LMAPI->get_string("lsg2-cgi-url"));
  printf("<input name=\"lcgi-user\" type=hidden value=\"%s\">\n", LMAPI->get_var("lcgi-user"));
  printf("<input name=\"lcgi-cookie\" type=hidden value=\"%s\">\n",
    LMAPI->get_var("lcgi-cookie"));
  printf("<input name=\"lcgi-list\" type=hidden value=\"%s\">\n",
    LMAPI->get_string("list"));
  printf("<input type=hidden name=\"lcgi-mode\" value=\"subscribe\">\n");
  if (param && *param) 
    printf("<input type=submit value=\"%s\">\n", param);
  else
    printf("<input type=submit value=\"Subscribe\">\n");
  printf("</form>\n");
  return 1;
}

CGI_HANDLER(cgihook_unsubscribe)
{
  if (!LMAPI->get_var("lcgi-cookie") || !LMAPI->get_var("lcgi-user") ||
      !LMAPI->get_var("list")) return 1;

  printf("<form action=\"%s\" method=post>\n",
    LMAPI->get_string("lsg2-cgi-url"));
  printf("<input name=\"lcgi-user\" type=hidden value=\"%s\">\n", LMAPI->get_var("lcgi-user"));
  printf("<input name=\"lcgi-cookie\" type=hidden value=\"%s\">\n",
    LMAPI->get_var("lcgi-cookie"));
  printf("<input name=\"lcgi-list\" type=hidden value=\"%s\">\n",
    LMAPI->get_string("list"));
  printf("<input type=hidden name=\"lcgi-mode\" value=\"unsubscribe\">\n");
  if (param && *param) 
    printf("<input type=submit value=\"%s\">\n", param);
  else
    printf("<input type=submit value=\"Unsubscribe\">\n");
  printf("</form>\n");
  return 1;
}

CGI_HANDLER(cgihook_flaglist)
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
        LMAPI->get_string("lcgi-user"), &user)) {
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

     if (!strcasecmp(tflag->name,"superadmin") || 
        (tflag->admin & ADMIN_UNSAFE) || 
        (tflag->admin & ADMIN_UNSETTABLE) ||
        (tflag->admin && !isadmin)) {
        tflag = tflag->next;
        continue;
     }

     printf("  <td valign=top width=%d%%>\n", 100 / cols);
     printf("   <input type=\"checkbox\" name=\"lcgipl-%s\"",
       tflag->name);
     if (LMAPI->user_hasflag(&user,tflag->name)) {
       printf(" checked=\"true\"");
     }
     printf("> %s\n", tflag->name);
     printf("   <ul>%s</ul>\n", tflag->desc);
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
        printf("  <td>&nbsp;</td>\n");
        curcol++;
     }
     printf(" </tr>\n");
  }

  free(flagarr);

  printf("</table>");
  return 1;
}

CGI_HANDLER(cgihook_setname)
{
  char fullname[256];
  const char *user, *list;

  user = LMAPI->get_var("lcgi-user");
  list = LMAPI->get_var("lcgi-list");

  if (!user || !list) return 0;

  if (LMAPI->userstat_get_stat(list,user,"realname",&fullname[0],255)) {
     printf("<input name=\"lcgi-fullname\" length=255 value=\"%s\">\n",
        fullname);
  } else {
     printf("<input name=\"lcgi-fullname\" length=255>\n");
  }

  return 1;
}

CGI_HANDLER(cgihook_showflags)
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

    stringcpy(unparse, param);

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
        LMAPI->get_string("lcgi-user"), &user)) {
     printf("<b>You are not subscribed to this list.</b>");
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
