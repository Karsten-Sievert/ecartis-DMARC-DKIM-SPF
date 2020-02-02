#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "lsg2.h"

const char hextable[] = "0123456789ABCDEF";

int decode_pathinfo(const char *pathinfo)
{
   char *ptr;
   char *buffer;

   buffer = strdup(pathinfo);

   ptr = buffer;

   ptr = strtok(ptr,"/");

   while (ptr ? *ptr : 0) {
      char *ptr2;
      char *ptr2lower;

      ptr2 = strchr(ptr,'=');
      if (ptr2) {
         *ptr2++ = 0;

         ptr2lower = LMAPI->lowerstr(ptr2);

         LMAPI->log_printf(18,"Parsing pathinfo: %s (%s)\n", ptr, ptr2);
         
         /* Take our pathinfo pieces */
         switch (tolower(*ptr)) {
            case 'l':
              LMAPI->set_var("lcgi-list",ptr2lower,VAR_GLOBAL);
              break;
            case 'u':
              LMAPI->set_var("lcgi-user",ptr2lower,VAR_GLOBAL);
              break;
            case 'c':
              LMAPI->set_var("lcgi-cookie",ptr2,VAR_GLOBAL);
              break;
            case 'm':
              LMAPI->set_var("lcgi-mode",ptr2,VAR_GLOBAL);
              break;
            case 'p':
              LMAPI->set_var("lcgi-pass",ptr2,VAR_GLOBAL);
              break;
         }

         free(ptr2lower);

         *(ptr2 - 1) = '=';

      }
      ptr = strtok(NULL,"/");
   }
   free(buffer);
   return 1;
}

/* Decode a key=value pair */
int decode_param(const char *param)
{
   char *buffer;
   char *paramname, *endvalue, *ptr, *ptr2;
   int returnval;

   /* Get our working buffer... */
   buffer = (char *)strdup(param);

   /* ...and split it into the key and encoded value */
   ptr = (char *)strchr(buffer,'=');
   *ptr = 0;
   paramname = (char *)strdup(buffer);

   /* Set up the workspace for the value decoding */   
   endvalue = (char *)malloc(strlen(param));
   memset(endvalue,0,strlen(param));

   ptr2 = endvalue;

   ptr++;

   /* Walk the encoded value, changing from CGI-encoded to
      normal text. */
   while (*ptr) {
      if (*ptr == '+') {
        *ptr2 = ' ';
      } else if (*ptr == '%') {
        char temp;
        const char *temp2;
        int value;

        /* Strange, ugly, scary hack to decode hexadecimal,
           because sscanf was doing odd things. */
        temp = toupper(*(ptr + 1));
        temp2 = strchr(hextable, temp);
        value = (temp2 - &hextable[0]) * 16;
        temp = toupper(*(ptr + 2));
        temp2 = strchr(hextable, temp);
        value += temp2 - &hextable[0];

        ptr += 2;
        *ptr2 = value;
      } else *ptr2 = *ptr;

      ptr2++;
      ptr++;
   }

   *ptr2 = 0;

   if (strlen(endvalue)) {
      char *endvaluelower;

      /* We can only set variables beginning in lcgi- this way,
         for security reasons.

         HOWEVER, lcgipl- is also allowed and will be stored in a
         temporary key/value structure, for the purposes of things
         like setting flags on yourself and doing the list
         configuration screen. */

      endvaluelower = LMAPI->lowerstr(endvalue);

      if (strncasecmp(paramname,"lcgi-",5) == 0) {
        if ((strcasecmp(paramname,"lcgi-user") == 0) ||
            (strcasecmp(paramname,"lcgi-list") == 0)) {
           LMAPI->set_var(paramname,endvaluelower,VAR_GLOBAL);
        } else {
           LMAPI->set_var(paramname,endvalue,VAR_GLOBAL);
        }

        returnval = 1;
      } else {
        if (strncasecmp(paramname,"lcgipl-",7) == 0) {
           LMAPI->add_cgi_tempvar((paramname+7),endvalue);
        }
        returnval = 0;
      }

     free(endvaluelower);
   } else returnval = 0;

   /* It's always good to clean up after ourselves. */
   free(paramname);
   free(endvalue);
   free(buffer);

   return(returnval);
}

void lsg2_internal_error_fallback(const char *errortext)
{
   printf("<body bgcolor=#ffffff text=#000000>\n");
   printf("<title>LSG/2 Internal Error: %s</title>\n", errortext);
   printf("<font size=+3>LSG/2 Error: %s</font>\n", errortext);
   printf("<HR>\n");
   printf("An internal and unrecoverable error has occured.\n");
   printf("Please contact <a href=\"mailto:%s\">%s</a> and report\n",
     LMAPI->get_string("listserver-admin"), 
     LMAPI->get_string("listserver-admin"));
   printf("how you got this error.");
   printf("<HR>\n");
}

void lsg2_internal_error(const char *errortext)
{
   LMAPI->set_var("lcgi-error", errortext, VAR_TEMP);
   if (!LMAPI->cgi_unparse_template("error"))
     lsg2_internal_error_fallback(errortext);
}

MODE_HANDLER(mode_lsg2)
{
   int len;
   char *inbuffer, *ptr;
   struct listserver_cgi_mode *curmode;
   char *tbuf;

   /* Generic header */

   if (LMAPI->get_bool("lsg2-iis-support"))
      printf("HTTP/1.1 200 OK\n");

   printf("Content-type: text/html\n");
   printf("LSG2-Version: %s\n", LSG2_VERSION);
   printf("\n\n");

   /* Let's do a sanity check.. */
   if (!LMAPI->get_var("lsg2-cgi-url") ||
       !LMAPI->get_var("cgi-template-dir")) {
     lsg2_internal_error("Incorrectly Configured");
     return MODE_END;
   }

   /* Get the content-length variable... */
   ptr = getenv("CONTENT_LENGTH");

   /* ...making sure it's valid... */
   if (ptr ? atoi(ptr) : 0) {
      /* ...and read in the POST data. */
      len = atoi(ptr);

      inbuffer = (char *)malloc(len + 2);
      memset(inbuffer,0,len + 2);
      fgets(inbuffer,len + 1,stdin);

      /* Walk the set of keys */
      ptr = (char *)strtok(inbuffer,"&");

      while(ptr ? *ptr : 0) {
         decode_param(ptr);      
         ptr = (char *)strtok(NULL,"&");
      }

      /* ...and clean up after ourselves. */
      free(inbuffer);
   }

   /* Do we have a PATH_INFO variable? */
   ptr = getenv("PATH_INFO");
   if (ptr) {
      LMAPI->set_var("tlcgi-pathinfo",ptr,VAR_GLOBAL);
      decode_pathinfo(ptr);
   }

   /* Now, any variables in the POST'd form that began with
      'lcgi-' and which are valid Listar variables have been
      set, so we can simply continue along.  */

   if (LMAPI->get_var("lcgi-mode")) {
       curmode = LMAPI->find_cgi_mode(LMAPI->get_string("lcgi-mode"));
       tbuf = strdup(LMAPI->get_string("lcgi-mode"));       
   } else {
     if (LMAPI->get_var("lcgi-user") && (LMAPI->get_var("lcgi-pass") ||
           LMAPI->get_var("lcgi-cookie"))) {
        if (LMAPI->get_var("lcgi-list")) {
           curmode = LMAPI->find_cgi_mode("listmenu");
           tbuf = strdup("listmenu");
        } else {
           curmode = LMAPI->find_cgi_mode("login");
           tbuf = strdup("login");
        }
     } else {
        curmode = LMAPI->find_cgi_mode("default");
        tbuf = strdup("default");
     }
   }

   if (LMAPI->get_var("lcgi-list")) {
     LMAPI->set_context_list(LMAPI->get_string("lcgi-list"));
   }

   LMAPI->set_var("lcgi-mode",tbuf,VAR_GLOBAL);

   LMAPI->set_var("lcgi-remote-host",getenv("REMOTE_ADDR"),VAR_GLOBAL);
   LMAPI->set_var("lcgi-server-soft",getenv("SERVER_SOFTWARE"),VAR_GLOBAL);

   if (!curmode) {
     lsg2_internal_error("Invalid Mode");
     printf("<!-- Mode was '%s' -->\n", tbuf);
     free(tbuf);
     return MODE_END;
   } else {
     free(tbuf);
     (curmode->mode)();
     return MODE_END;
   }
}

CMDARG_HANDLER(cmdarg_lsg2)
{
   LMAPI->set_var("mode","lsg2",VAR_GLOBAL);
   LMAPI->set_var("fakequeue","yes",VAR_GLOBAL);

   return CMDARG_OK;
}

int lsg2_update_cookie(char *buffer, int length)
{
   const char *oldcookie;
   const char *curcookie;
   char cookiefile[BIG_BUF];
   char cookie[BIG_BUF];
   char databuf[BIG_BUF];
   time_t now;
   int seconds;
   const char *fromaddy = LMAPI->get_string("lcgi-user");
   const char *remote = LMAPI->get_string("lcgi-remote-host");

   if (!LMAPI->get_var("lcgi-user")) {
      lsg2_internal_error("Invalid form data, no user.");
      return 0;
   }

   LMAPI->buffer_printf(cookiefile, sizeof(cookiefile) - 1, "%s/SITEDATA/cookies",
                        LMAPI->get_string("lists-root"));

   time(&now);
   seconds = LMAPI->get_seconds("lsg2-cookie-duration");
   now += seconds;

#ifdef OBSDMOD
    LMAPI->buffer_printf(databuf, sizeof(databuf) - 1, "%lX;%s;%s", (long)now, fromaddy, remote);
#else
#ifdef DEC_UNIX
    LMAPI->buffer_printf(databuf, sizeof(databuf) - 1, "%X;%s;%s", now, fromaddy, remote);
#else
    LMAPI->buffer_printf(databuf, sizeof(databuf) - 1, "%lX;%s;%s", (long)now, fromaddy, remote);
#endif /* DEC_UNIX */
#endif /* OBSDMOD */

   oldcookie = LMAPI->find_cookie(cookiefile,'W', 
                     LMAPI->get_string("lcgi-user"));
   curcookie = LMAPI->get_var("lcgi-cookie");

   if ((!oldcookie && !curcookie && (strcasecmp(LMAPI->get_var("lcgi-mode"),"passwd") == 0)) ||
       (curcookie && !LMAPI->match_cookie(curcookie,LMAPI->get_string("lcgi-user")))) {
      char forgeWarning[BIG_BUF];

      lsg2_internal_error("Cookie may be forged?");
      
      LMAPI->set_var("task-form-subject","Possible attempted forgery!",
                  VAR_TEMP);
      LMAPI->task_heading(LMAPI->get_var("list-owner"));
      LMAPI->buffer_printf(forgeWarning,BIG_BUF - 1,
         "Attempted forgery from %s detected!",
         LMAPI->get_var("lcgi-remote-host"));
      LMAPI->smtp_body_line(forgeWarning);
      LMAPI->smtp_body_line("");
      LMAPI->smtp_body_line("A user on the host listed above may have tried to forge credentials to LSG/2 as");
      LMAPI->smtp_body_line(LMAPI->get_var("lcgi-user"));
      LMAPI->task_ending();
      return 0;
   }

   if (curcookie || oldcookie) { 

      if (oldcookie) {
         char *tmp;
         char *cookieuser;
         char *cookiehost;

         tmp = strstr(oldcookie," : ");
         *tmp = '\0';
         tmp += 3;

         if (curcookie ? strcmp(oldcookie,curcookie) != 0 : 0) {
            lsg2_internal_error("Authorization failed!");
            return 0;
         }

         cookieuser = strchr(tmp,';');
         cookieuser++;
         cookiehost = strchr(cookieuser,';');
         *cookiehost++ = 0;

         /* This should NEVER, EVER happen.  Ever.  But just to be
          * paranoid... */
         if (strcasecmp(cookieuser,fromaddy) != 0) {
            lsg2_internal_error("Internal authorization routine fault.");
            return 0;
         }

         /* Our hosts don't match, delete old cookie and generate a 
          * new one, then send it to the user. */
         if (strcasecmp(cookiehost,remote) != 0) {
             LMAPI->del_cookie(cookiefile,oldcookie);
             LMAPI->set_var("cookie-for", fromaddy, VAR_TEMP);
             if(!LMAPI->request_cookie(cookiefile, &cookie[0], 'W', databuf)) {
                lsg2_internal_error("Unable to generate authorization code.");   
                LMAPI->filesys_error(cookiefile);
                return 0;
             }
             LMAPI->buffer_printf(buffer,length - 1,"%s",cookie);
             LMAPI->log_printf(2,"CGI: %s logged in (%s)\n",
                LMAPI->get_var("lcgi-user"),
                LMAPI->get_var("lcgi-remote-host"));
             if (!LMAPI->get_var("lcgi-pass")) {
                LMAPI->set_var("task-form-subject","Web Authorization Code",
                  VAR_TEMP);
                LMAPI->task_heading(fromaddy);
                LMAPI->smtp_body_line("Someone, perhaps you, has attempted to log in using your");
                LMAPI->smtp_body_line("e-mail address at the mailing list interface at:");
                LMAPI->smtp_body_line(LMAPI->get_string("lsg2-cgi-url"));
                LMAPI->smtp_body_line("");
                LMAPI->smtp_body_line("In order to continue, please paste the following line into");
                LMAPI->smtp_body_line("the web form.  (This code will expire if not used.)");
                LMAPI->smtp_body_line("");
                LMAPI->smtp_body_line(cookie);
                LMAPI->smtp_body_line("");
                LMAPI->smtp_body_line("Alternatively, you can go to:");
                if (!LMAPI->get_var("lcgi-list")) 
                  LMAPI->buffer_printf(databuf, sizeof(databuf) - 1,
                    "%s/u=%s/m=login/c=%s", LMAPI->get_string("lsg2-cgi-url"),
                    LMAPI->get_string("lcgi-user"), cookie);
                else
                  LMAPI->buffer_printf(databuf, sizeof(databuf) - 1,
                    "%s/u=%s/m=listmenu/l=%s/c=%s",
                    LMAPI->get_string("lsg2-cgi-url"), 
                    LMAPI->get_string("lcgi-user"), LMAPI->get_string("list"),
                    cookie);
                LMAPI->smtp_body_line(databuf);
                LMAPI->task_ending();
                LMAPI->clean_var("cookie-for", VAR_TEMP);
                return 2;
             } else {
                LMAPI->set_var("lcgi-cookie",cookie,VAR_GLOBAL);
                return 1;
             }
         }

         /* Are we logging in again from the same machine?  If so,
          * we just re-mail the cookie. */
         if (!curcookie && oldcookie) {
           LMAPI->log_printf(2,"CGI: %s logged in (%s)\n",
             LMAPI->get_var("lcgi-user"),LMAPI->get_var("lcgi-remote-host"));
           if (!LMAPI->get_var("lcgi-pass")) {
              LMAPI->set_var("task-form-subject","Web Authorization Code (Re-mail)",
                 VAR_TEMP);
              LMAPI->task_heading(fromaddy);
              LMAPI->smtp_body_line("Someone, perhaps you, has attempted to log in using your");
              LMAPI->smtp_body_line("e-mail address at the mailing list interface at:");
              LMAPI->smtp_body_line(LMAPI->get_string("lsg2-cgi-url"));
              LMAPI->smtp_body_line("");
              LMAPI->smtp_body_line("As you already had an authorization code that had not yet");
              LMAPI->smtp_body_line("expired, it is being re-mailed to you.");
              LMAPI->smtp_body_line("");
              LMAPI->smtp_body_line("In order to continue, please paste the following line into");
              LMAPI->smtp_body_line("the web form.  (This code will expire if not used.)");
              LMAPI->smtp_body_line("");
              LMAPI->smtp_body_line(oldcookie);
              LMAPI->smtp_body_line("");
              LMAPI->smtp_body_line("Alternatively, you can go to:");
              if (!LMAPI->get_var("lcgi-list")) 
                  LMAPI->buffer_printf(databuf, sizeof(databuf) - 1,
                    "%s/u=%s/m=login/c=%s", LMAPI->get_string("lsg2-cgi-url"),
                    LMAPI->get_string("lcgi-user"), oldcookie);
              else
                  LMAPI->buffer_printf(databuf, sizeof(databuf) - 1,
                    "%s/u=%s/m=listmenu/l=%s/c=%s",
                    LMAPI->get_string("lsg2-cgi-url"), 
                    LMAPI->get_string("lcgi-user"), LMAPI->get_string("lcgi-list"),
                    oldcookie);
              LMAPI->smtp_body_line(databuf);
              LMAPI->task_ending();
              LMAPI->clean_var("cookie-for", VAR_TEMP);
              free((char *)oldcookie);
              return 2;
            } else {
              LMAPI->set_var("lcgi-cookie",oldcookie,VAR_GLOBAL);
              return 1;
            }
         }

         if (!LMAPI->modify_cookie(cookiefile, oldcookie, databuf)) {
             lsg2_internal_error("Unable to renew authorization.");
             LMAPI->filesys_error(cookiefile);
             free((char *)oldcookie);
             return 0;
         }
         LMAPI->buffer_printf(buffer,length - 1,"%s",oldcookie);
         free((char *)oldcookie);
         return 1;
      } else {
         lsg2_internal_error("Authorization failed: cookie expired?");
         return 0;
      }
   } else {
        LMAPI->set_var("cookie-for", fromaddy, VAR_TEMP);
        if(!LMAPI->request_cookie(cookiefile, &cookie[0], 'W', databuf)) {
            lsg2_internal_error("Unable to generate authorization code.");   
            LMAPI->filesys_error(cookiefile);
            return 0;
        }
        LMAPI->buffer_printf(buffer,length - 1,"%s",cookie);
        LMAPI->log_printf(2,"CGI: %s logged in (%s)\n",
          LMAPI->get_var("lcgi-user"),LMAPI->get_var("lcgi-remote-host"));
        if (!LMAPI->get_var("lcgi-pass")) {
           LMAPI->set_var("task-form-subject","Web Authorization Code",
             VAR_TEMP);
           LMAPI->task_heading(fromaddy);
           LMAPI->smtp_body_line("Someone, perhaps you, has attempted to log in using your");
           LMAPI->smtp_body_line("e-mail address at the mailing list interface at:");
           LMAPI->smtp_body_line(LMAPI->get_string("lsg2-cgi-url"));
           LMAPI->smtp_body_line("");
           LMAPI->smtp_body_line("In order to continue, please paste the following line into");
           LMAPI->smtp_body_line("the web form.  (This code will expire if not used.)");
           LMAPI->smtp_body_line("");
           LMAPI->smtp_body_line(cookie);
           LMAPI->smtp_body_line("");
           LMAPI->smtp_body_line("Alternatively, you can go to:");
           if (!LMAPI->get_var("lcgi-list")) 
                  LMAPI->buffer_printf(databuf, sizeof(databuf) - 1,
                    "%s/u=%s/m=login/c=%s", LMAPI->get_string("lsg2-cgi-url"),
                    LMAPI->get_string("lcgi-user"), cookie);
           else
                  LMAPI->buffer_printf(databuf, sizeof(databuf) - 1,
                    "%s/u=%s/m=listmenu/l=%s/c=%s",
                    LMAPI->get_string("lsg2-cgi-url"), 
                    LMAPI->get_string("lcgi-user"), LMAPI->get_string("lcgi-list"),
                    cookie);
           LMAPI->smtp_body_line(databuf);
           LMAPI->task_ending();
           LMAPI->clean_var("cookie-for", VAR_TEMP);
           return 2;
        } else {
           LMAPI->set_var("lcgi-cookie",cookie,VAR_GLOBAL);
           return 1;
        }
   }
}

void lsg2_html_textfile(const char *filename)
{
   char buffer[BIG_BUF];
   FILE *infile;
   int inchar;

   LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s.unparsetext", LMAPI->get_string("queuefile"));

   if (!LMAPI->liscript_parse_file(filename,buffer)) {
      printf("<P><font size=+2><b>Unable to display textfile!</b></font></P>\n");
      printf("<!-- Error on file: %s or %s -->\n", filename, buffer);
      LMAPI->unlink_file(buffer);
      return;
   }

   if ((infile = LMAPI->open_file(buffer,"r")) == NULL) {
      printf("<P><font size=+2><b>Unable to display textfile!</b></font></P>\n");
      printf("<!-- Error on file: %s -->\n", buffer);
      LMAPI->unlink_file(buffer);
      return;
   }

   printf("<pre>");
   while((inchar = fgetc(infile)) != EOF) {
      if ((char)inchar == '<') printf("&lt;");
      else if ((char)inchar == '>') printf("&gt;");
      else printf("%c", (char)inchar);
   }
   printf("</pre>");

   LMAPI->close_file(infile);
   LMAPI->unlink_file(buffer);
}

int lsg2_validate(char *buffer, int length)
{
   const char *fromaddy;
   const char *pass;

   fromaddy = LMAPI->get_var("lcgi-user");
   pass = LMAPI->get_var("lcgi-pass");

   if (!fromaddy) {
      lsg2_internal_error("No username provided!");
      return -1;
   }
   if (!pass || (strcasecmp(LMAPI->get_string("lcgi-mode"),"passwd") == 0)) {

      if (strcasecmp(LMAPI->get_string("lcgi-mode"),"passwd") == 0) {
          if (!LMAPI->get_var("lcgi-cookie")) {
             lsg2_internal_error("Your template did not provide a cookie with a password change.  Please have your admin update their templates.");
             return 0;
          }
      }

      switch(lsg2_update_cookie(buffer, length)) {
        case 1:
          return 1;
        case 2: 
          if (!LMAPI->cgi_unparse_template("logincookie")) {
             lsg2_internal_error("No login cookie template.");
          }
          return 0;
          break;
        default:
          return 0;
      }
   } else {
      if (LMAPI->find_pass(fromaddy)) {
         if (LMAPI->check_pass(fromaddy, pass)) {
            switch(lsg2_update_cookie(buffer, length)) {
              case 1: 
                return 1;
              case 2: 
                if (LMAPI->cgi_unparse_template("logincookie")) {
                   lsg2_internal_error("No login cookie template.");
                }
                return 0;
                break;
              default:
                return 0;
            }
         } else {
            lsg2_internal_error("Incorrect password.");
            return 0;
         }
      } else {
         lsg2_internal_error("No password set for user.  Provide blank password and check e-mail for validation.");
         return 0;
      }
   }
}
