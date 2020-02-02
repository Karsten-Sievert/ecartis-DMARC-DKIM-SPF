#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#endif

#include "bouncer.h"
#include "results.h"

struct LPMAPI *LMAPI;

int mimehandled;
FILE *outputfile;

void update_watches(char type, const char *user, const char *result,
                    const char *watchfile)
{
   FILE *infile;
   FILE *outfile;
   char filename[SMALL_BUF];
   char buf[BIG_BUF], tuser[BIG_BUF];
   int  tcount, fcount, found, timestamp, firsterror;

   found = 0;
   tcount = 0;
   fcount = 0;

   LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.new", watchfile);

   if ((infile = LMAPI->open_exclusive(watchfile,"r+")) == NULL) {
      /* it doesn't already exist, so we need to create it */
      /* however, we can just create it.. no need to create a temp and copy */
      if ((outfile = LMAPI->open_file(watchfile,"w")) == NULL)
         return;

      LMAPI->write_file(outfile,"%d\n",(int)time(NULL));

      switch(type) {
         case RESULT_TRANSIENT: tcount++; break;
         case RESULT_FAILURE: fcount++; break;
      }
      
      LMAPI->write_file(outfile,"^ %s : %d : %d : %d : %d : &%s\n", user,
                        tcount, fcount, (int)time(NULL), (int)time(NULL),
                        result);
      LMAPI->close_file(outfile);
      return;
   }

   if ((outfile = LMAPI->open_file(filename,"w+")) == NULL) {
      LMAPI->close_file(infile);
      return;
   }

   /* Preserve first line */
   LMAPI->read_file(buf, sizeof(buf), infile);
   LMAPI->write_file(outfile,"%s",buf);

   while(LMAPI->read_file(buf, sizeof(buf), infile)) {

      if (buf[0] == '^') {
         sscanf(buf,"^ %s : %d : %d : %d : %d", 
           &tuser[0], &tcount, &fcount, &timestamp, &firsterror);
      } else {
         sscanf(buf,"%s : %d : %d : %d", &tuser[0], &tcount, &fcount,
                &timestamp);
         firsterror = (int)time(NULL);
      }

      if (strcasecmp(tuser,user) == 0) {
         found = 1;
     
         switch(type) {
            case RESULT_TRANSIENT: tcount++; break;
            case RESULT_FAILURE: fcount++; break;
         }

         LMAPI->write_file(outfile,"^ %s : %d : %d : %d : %d : &%s\n", user,
                           tcount, fcount, (int)time(NULL), firsterror,
                           result);
      } else
         LMAPI->write_file(outfile, "%s", buf);
   }

   if (!found) {
      tcount = 0; fcount = 0;

      switch(type) {
         case RESULT_TRANSIENT: tcount++; break;
         case RESULT_FAILURE: fcount++; break;
      }

      firsterror = (int)time(NULL);      

      LMAPI->write_file(outfile,"^ %s : %d : %d : %d : %d : &%s\n", user,
                        tcount, fcount, firsterror, firsterror, result);
   }
   LMAPI->rewind_file(outfile);
   LMAPI->rewind_file(infile);
   LMAPI->truncate_file(infile, 0);
   while(LMAPI->read_file(buf, sizeof(buf), outfile)) {
       LMAPI->write_file(infile, "%s", buf);
   }
   LMAPI->close_file(infile);
   LMAPI->close_file(outfile);
   LMAPI->unlink_file(filename);
   return;
}  

void process_watches(const char *listname, int automated)
{
   FILE *infile;
   FILE *outfile, *logfile;
   char filename[SMALL_BUF], logfilename[SMALL_BUF], watchfile[BIG_BUF];  /* Changed watchfile from SMALL_BUF to BIG_BUF due to listdir_file */
   char buf[BIG_BUF], tuser[SMALL_BUF], lasterr[SMALL_BUF];
   int  tcount, fcount, timestamp, ucount;
   time_t temptime; 
   struct tm *lttm;
   int  lasttime, firsttime;
   int  maxfatal, maxtrans, timeoutdays, neverunsub;
   int nevervacation;

   tcount = 0;
   fcount = 0;
   ucount = 0;

   maxfatal = LMAPI->get_number("bounce-max-fatal");
   maxtrans = LMAPI->get_number("bounce-max-transient");

   neverunsub = LMAPI->get_bool("bounce-never-unsub");
   nevervacation = LMAPI->get_bool("bounce-never-vacation");

   timeoutdays = LMAPI->get_number("bounce-timeout-days");

   LMAPI->listdir_file(watchfile,listname,"watches");

   LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.newproc", LMAPI->get_string("queuefile"));
   LMAPI->buffer_printf(logfilename, sizeof(logfilename) - 1, "%s.bouncelog", LMAPI->get_string("queuefile"));

   if ((infile = LMAPI->open_exclusive(watchfile,"r+")) == NULL) return;
   if ((outfile = LMAPI->open_exclusive(filename,"w+")) == NULL) {
      LMAPI->close_file(infile);
      return;
   }

   LMAPI->read_file(buf, sizeof(buf), infile);

   if (automated) {
      buf[strlen(buf) - 1] = 0;
      lasttime = atoi(buf);
      temptime = time(NULL);

      if (((int)temptime - lasttime) < 86400) {
         LMAPI->close_file(infile);
         LMAPI->close_file(outfile);
         LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.newproc", LMAPI->get_string("queuefile"));
         LMAPI->unlink_file(filename);
         LMAPI->log_printf(5,"Tried to process bounces within 24 hours of last process.\n");
         return;
      }
   } else {
      LMAPI->log_printf(5,"Forcing bounce processing.\n");
   }

   LMAPI->write_file(outfile,"%d\n",(int)time(NULL));

   logfile = LMAPI->open_exclusive(logfilename,"w");

   if (logfile) {
      LMAPI->write_file(logfile,"These users have returned errors and are being watched for\n");
      LMAPI->write_file(logfile,"the list %s.\n\n", listname);

      LMAPI->write_file(logfile,"%-10s %-5s %-5s %-20s %s\n", "Action",
                        "Trans","Fatal","Error span", "User");

      LMAPI->write_file(logfile,"----------------------------------------------------------------------\n");
   }

   while(LMAPI->read_file(buf, sizeof(buf), infile)) {
      if(buf[0] && (buf[0] != '\n')) {
         char action[SMALL_BUF], timebuf[SMALL_BUF], userfile[SMALL_BUF], timestr[SMALL_BUF];  /* Changed userfile from SMALL_BUF to BIG_BUF due to listdir_file */
         int handled, goterror;
         struct list_user userstruct;

         LMAPI->listdir_file(userfile,listname,"users");

         ucount++;
         handled = 0; goterror = 0;
         LMAPI->buffer_printf(action, sizeof(action) - 1, "None");

         /* Check for our new-format bounce data */
         if (buf[0] == '^') {
            char *tchk;

            sscanf(buf,"^ %s : %d : %d : %d : %d", &tuser[0], &tcount,
                   &fcount, &timestamp, &firsttime);
            tchk = strchr(buf,'&');
            if (tchk) {
               LMAPI->buffer_printf(lasterr, sizeof(lasterr) - 1, "%s", tchk + 1); goterror = 1;
            }
         } else {
            sscanf(buf,"%s : %d : %d : %d", &tuser[0], &tcount, &fcount,
                   &timestamp);
            firsttime = timestamp;
         }

         temptime = (time_t)firsttime;
         lttm = localtime(&temptime);
         strftime(&timestr[0], sizeof(timestr) - 1,"%b %d - ",lttm);

         temptime = (time_t)timestamp;
         lttm = localtime(&temptime);
         strftime(&timebuf[0], sizeof(timebuf) - 1,"%b %d",lttm);

         stringcat(timestr, timebuf);

         if(LMAPI->user_find_list(listname,tuser,&userstruct)) {
            if(LMAPI->user_hasflag(&userstruct,"ADMIN")) {
               handled = 1;
               LMAPI->buffer_printf(action, sizeof(action) - 1, "Admin");
            } else if (LMAPI->user_hasflag(&userstruct,"PROTECTED")) {
               handled = 1;
               LMAPI->buffer_printf(action, sizeof(action) - 1, "Protect");
            } else if((time(NULL) - timestamp) > (86400 * timeoutdays)) {
               handled = 1;
               LMAPI->buffer_printf(action, sizeof(action) - 1, "Expired");
            } else if(tcount >= maxtrans) {
               handled = 1;
               if(!nevervacation) {
                  if (!LMAPI->get_bool("bounce-always-unsub")) {
                     char vacationfile[BIG_BUF], tempbuf[SMALL_BUF];

                     LMAPI->listdir_file(vacationfile,listname,
                        LMAPI->get_string("bouncer-vacation-file"));
                     LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "Auto-set VACATION on %s", listname);
                     LMAPI->set_var("task-form-subject",tempbuf,VAR_TEMP);
                     if(LMAPI->get_var("send-as")) {
		         LMAPI->set_var("form-send-as",
                                        LMAPI->get_string("send-as"), VAR_TEMP);
                     }
                     LMAPI->send_textfile_expand(userstruct.address,vacationfile);
                     LMAPI->clean_var("task-form-subject",VAR_TEMP);
                     LMAPI->clean_var("form-send-as",VAR_TEMP);

                     LMAPI->buffer_printf(action, sizeof(action) - 1, "Vacation");
                     LMAPI->user_setflag(&userstruct,"VACATION",1);
                     LMAPI->user_write(userfile,&userstruct);
                  } else {
                     char unsubfile[BIG_BUF], tempbuf[SMALL_BUF];

                     LMAPI->listdir_file(unsubfile,listname,
                        LMAPI->get_string("bouncer-unsub-file"));
                     LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "Auto-removed from %s", listname);
                     LMAPI->set_var("task-form-subject",tempbuf,VAR_TEMP);
                     if(LMAPI->get_var("send-as")) {
		         LMAPI->set_var("form-send-as",
                                        LMAPI->get_string("send-as"), VAR_TEMP);
                     }
                     LMAPI->send_textfile_expand(userstruct.address,unsubfile);
                     LMAPI->clean_var("task-form-subject",VAR_TEMP);
                     LMAPI->clean_var("form-send-as",VAR_TEMP);

                     LMAPI->buffer_printf(action, sizeof(action) - 1, "Unsub");
                     LMAPI->user_remove(userfile,tuser);
                  }
               } else {
                  LMAPI->buffer_printf(action, sizeof(action) - 1, "Manual");
               }
               /* Attempt to notify user? */
            } else if(fcount >= maxfatal) {
               handled = 1; 
               if (neverunsub && !LMAPI->get_bool("bounce-always-unsub")) {
                  if(!nevervacation) {
                     char vacationfile[BIG_BUF], tempbuf[SMALL_BUF];

                     LMAPI->listdir_file(vacationfile,listname,
                        LMAPI->get_string("bouncer-vacation-file"));
                     LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "Auto-set VACATION on %s", listname);
                     LMAPI->set_var("task-form-subject",tempbuf,VAR_TEMP);
                     if(LMAPI->get_var("send-as")) {
		         LMAPI->set_var("form-send-as",
                                        LMAPI->get_string("send-as"), VAR_TEMP);
                     }
                     LMAPI->send_textfile_expand(userstruct.address,vacationfile);
                     LMAPI->clean_var("task-form-subject",VAR_TEMP);
                     LMAPI->clean_var("form-send-as",VAR_TEMP);

                     LMAPI->buffer_printf(action, sizeof(action) - 1, "Vacation");
                     LMAPI->user_setflag(&userstruct,"VACATION",1);
                     LMAPI->user_write(userfile,&userstruct);
                  } else {
                     LMAPI->buffer_printf(action, sizeof(action) - 1, "Manual");
                  }
               } else {
                  char unsubfile[BIG_BUF], tempbuf[SMALL_BUF];

                  LMAPI->listdir_file(unsubfile,listname,
                     LMAPI->get_string("bouncer-unsub-file"));
                  LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "Auto-removed from %s", listname);
                  LMAPI->set_var("task-form-subject",tempbuf,VAR_TEMP);
                  if(LMAPI->get_var("send-as")) {
		     LMAPI->set_var("form-send-as",
                                     LMAPI->get_string("send-as"), VAR_TEMP);
                  }
                  LMAPI->send_textfile_expand(userstruct.address,unsubfile);
                  LMAPI->clean_var("task-form-subject",VAR_TEMP);
                  LMAPI->clean_var("form-send-as",VAR_TEMP);

                  LMAPI->buffer_printf(action, sizeof(action) - 1, "Unsub");
                  LMAPI->user_remove(userfile,tuser);
               }
            }
         } else {
            LMAPI->buffer_printf(action, sizeof(action) - 1, "NotSub"); handled = 1;
         }

         if (!handled) {
            LMAPI->write_file(outfile,"%s",buf);
         }
        
         if (logfile) {
            LMAPI->write_file(logfile,"%-10s %-5d %-5d %-20s %s\n", action,
                              tcount, fcount, timestr, tuser);
            if (goterror) {
              LMAPI->write_file(logfile, "%-10s %s", "", lasterr);
            }
         }
      }
   }

   LMAPI->rewind_file(outfile);
   LMAPI->rewind_file(infile);
   LMAPI->truncate_file(infile, 0);
   while(LMAPI->read_file(buf, sizeof(buf), outfile)) {
      LMAPI->write_file(infile, "%s", buf);
   }
   LMAPI->close_file(infile);
   LMAPI->close_file(outfile);
   LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.newproc", LMAPI->get_string("queuefile"));
   (void)LMAPI->unlink_file(filename);

   if (logfile) { 
      LMAPI->write_file(logfile,"----------------------------------------------------------------------\n\n");
      LMAPI->write_file(logfile,"Key to actions:\n");
      LMAPI->write_file(logfile,"\tNone          - No action taken, user left in watchfile.\n");
      LMAPI->write_file(logfile,"\tVacation      - User's VACATION flag was set.\n");
      LMAPI->write_file(logfile,"\t                User reached %d transient errors.\n", maxtrans);
      if (!neverunsub && nevervacation) {
         LMAPI->write_file(logfile,"\t                NOTE: This list is configured so users who would\n");
         LMAPI->write_file(logfile,"\t                be set VACATION will be Unsub'd instead.\n");
      }
      LMAPI->write_file(logfile,"\tUnsub         - User was unsubscribed.\n");
      LMAPI->write_file(logfile,"\t                User reached %d fatal errors.\n", maxfatal);
      if (neverunsub && !nevervacation) {
         LMAPI->write_file(logfile,"\t                NOTE: This list is configured so users who would\n");
         LMAPI->write_file(logfile,"\t                be Unsub'd will be set VACATION instead.\n");
      }
      LMAPI->write_file(logfile,"\tExpired       - User has not had a bounce for %d days,\n", timeoutdays);
      LMAPI->write_file(logfile,"\t                watch has been expired.\n");
      LMAPI->write_file(logfile,"\tAdmin         - User is administrator, no action taken.\n");
      LMAPI->write_file(logfile,"\tProtect       - User is flagged PROTECTED, no action taken.\n");
      LMAPI->write_file(logfile,"\tManual        - There was no default action taken, handle manually.\n");
      if (neverunsub && nevervacation) {
         LMAPI->write_file(logfile,"\t                NOTE: This list is configured so users will always be\n");
         LMAPI->write_file(logfile,"\t                be marked for Manual handling.\n");
      }
      LMAPI->write_file(logfile,"\tNotSub        - Bounce message received for user not subscribed\n"); 
      LMAPI->write_file(logfile,"\t                to this list.  This is caused when someone is\n");
      LMAPI->write_file(logfile,"\t                forwarding to another address and their mailserver\n");
      LMAPI->write_file(logfile,"\t                reports the wrong one, or if someone has been\n");
      LMAPI->write_file(logfile,"\t                unsubscribed from the list but not expired from\n");
      LMAPI->write_file(logfile,"\t                the watches yet.\n");

      LMAPI->close_file(logfile);
      if (ucount) {
         const char *fromaddy;
         char subjbuffer[BIG_BUF];

         /* Sending the message as send-as creates a nice tight mail loop
          * when one of the admins gets a bounce :).
          * --
          * Not anymore, JT, but we'll let it slide. ;)
          */
         /*
         fromaddy = LMAPI->get_var("send-as");
         if (!fromaddy)
            fromaddy = LMAPI->get_string("list-owner");
         */
         fromaddy = LMAPI->get_string("listserver-admin");

         LMAPI->buffer_printf(subjbuffer, sizeof(subjbuffer) - 1,
            "Report for '%s' on error watches", listname);

         LMAPI->flagged_send_textfile(fromaddy,listname,"REPORTS",logfilename,
                subjbuffer);
      }
      (void)LMAPI->unlink_file(logfilename);
   }
}

void handle_error(int status, const char *user, const char *result,
                  const char *watchfile)
{
   char tuser[SMALL_BUF];
   const char *tptr;
   char *tptr2;

   if ((status / 100) == RESULT_SUCCESS) return;

   LMAPI->log_printf(1,"Bounce: %s: <%s> (%s) processed.\n",
       LMAPI->get_string("list"),user,result);

   tptr = user;
   tptr2 = &tuser[0];

   if (*tptr == '<') tptr++;

   while(*tptr && (*tptr != '>')) {
     *tptr2++ = *tptr++;
   }
   *tptr2 = 0;

   if ((status / 100) == RESULT_TRANSIENT) {
      update_watches(RESULT_TRANSIENT,tuser,result,watchfile);
   } else
   if ((status / 100) == RESULT_FAILURE) {
      update_watches(RESULT_FAILURE,tuser,result,watchfile);
   }
}

MIME_HANDLER(mimehandle_bounce)
{
   char buf[BIG_BUF];
   FILE *infile;
   char user[SMALL_BUF], error[BIG_BUF];
   int status, gotuser, gotorig;
   char *tptr;
   char watchfile[BIG_BUF];

   /* Keep -Wall -Werror happy */
   status = 0;

   if ((infile = LMAPI->open_file(mimefile,"r")) == NULL) {
      return MIME_HANDLE_OK;
   }

   LMAPI->listdir_file(watchfile,LMAPI->get_string("list"),"watches");

   gotuser = 0; gotorig = 0;
   memset(user, 0, sizeof(user));
   memset(error, 0, sizeof(error));

   while(LMAPI->read_file(buf, sizeof(buf), infile)) {

      /* Eat newline */
      if (buf[strlen(buf) - 1] == '\n')
          buf[strlen(buf) - 1] = 0;

      /* We found a recipient */
      if (!gotorig && (strncasecmp(buf,"Final-Recipient:",16) == 0)) {

         /* Skip past the address part, probably RFC822 */
         tptr = strchr(buf,';');
         if (tptr) {
			tptr++;
			if(*tptr == ' ' || *tptr == '\t') tptr++;
            LMAPI->buffer_printf(user, sizeof(user) - 1, "%s", tptr);
            gotuser = 1; status = 550;
         }
      }
      else if (strncasecmp(buf,"Original-Recipient:",19) == 0) {
         /* We prefer Original-Recipient if both are present */

         /* Skip past the address part, probably RFC822 */
         tptr = strchr(buf,';');
         if (tptr) {
			tptr++;
			if(*tptr == ' ' || *tptr == '\t') tptr++;
            LMAPI->buffer_printf(user, sizeof(user) - 1, "%s", tptr);
            gotuser = 1; status = 550; gotorig = 1;
         }
      }
      else if (strncasecmp(buf,"Status:",7) == 0) {
         char temp[5];
		 char *start, *end;

         LMAPI->buffer_printf(temp, sizeof(temp) - 1, "%c%c%c", buf[8], buf[10], buf[12]);
         status = atoi(temp);

		 /* In some bounces there is no "Diagnostic-Code:" and the reason is
		  * specified in "Status:", e.g.:
		  * Status: 5.0.0 (unable to deliver this message after 18 hours)
		  */
		 start = strchr(buf, '(');
		 end = strchr(buf, ')');
		 if (error[0] == 0 && start && end && end > start) {
			 start++;
			 *end = 0;
			 LMAPI->buffer_printf(error, sizeof(error) - 1, "%s", start);
		 }
	  }
      else if (strncasecmp(buf,"Diagnostic-Code:", 16) == 0) {

         /* Uuuuuuugly logic */
         tptr = strstr(buf,"...");
         if (tptr) 
            tptr += 4;
         else {
            tptr = strchr(buf,';');
            if (tptr) 
               tptr += 2; 
            else {
               tptr = &(buf[17]);
            }
         }

         LMAPI->buffer_printf(error, sizeof(error) - 1, "%s", tptr);
      }
      else if (error[0] == 0 && strncasecmp(buf,"Action:", 7) == 0) {
		  /* In some bounces there is no "Diagnostic-Code:" and the reason is
		   * specified in "Action:", e.g.:
		   * Action: failed (Bad destination mailbox address)
		   */
		  char *start, *end;
		  start = strchr(buf, '(');
		  end = strchr(buf, ')');
		  if (start && end && end > start) {
			  start++;
			  *end = 0;
			  LMAPI->buffer_printf(error, sizeof(error) - 1, "%s", start);
		  }
      }
      else if (buf[0] == '\0') {

         if (gotuser) {

            LMAPI->write_file(outputfile,"User: %s\n      (%d) %s\n\n",
              user, status, error);
            handle_error(status, user, error, watchfile);

            mimehandled++;

            gotuser = 0; gotorig = 0;
            memset(user, 0, sizeof(user));
            memset(error, 0, sizeof(error));
            status = 0;
         }
      }
   }

   LMAPI->close_file(infile);

   return MIME_HANDLE_OK;   
}

int parse_bounce(const char *list)
{
   FILE *infile, *outfile;
   int parsedheader;
   char buf[BIG_BUF], buf2[BIG_BUF];
   char from[BIG_BUF];
   char outfilename[BIG_BUF], mimefilename[SMALL_BUF];  /* Changed outfilename from SMALL_BUF to BIG_BUF due to listdir_file */
   const char *fromaddy;
   int done;
   int errors = 0;

   parsedheader = 0;

   mimehandled = 0;

   LMAPI->buffer_printf(mimefilename, sizeof(mimefilename) - 1, "%s.demime", 
      LMAPI->get_string("queuefile"));

   LMAPI->add_mime_handler("Message/DELIVERY-STATUS", 1,
      mimehandle_bounce);

   LMAPI->buffer_printf(outfilename, sizeof(outfilename) - 1, "%s.errlog", LMAPI->get_string("queuefile"));
   if ((outfile = LMAPI->open_file(outfilename,"w+")) == NULL) {
	   return 0;
   }

   outputfile = outfile;

   if ((infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) == NULL) {
      LMAPI->close_file(outfile);
      LMAPI->unlink_file(outfilename);
      return 0;
   }

   LMAPI->listdir_file(outfilename,list,"watches");

   if (!LMAPI->read_file(buf, sizeof(buf), infile)) {
      LMAPI->close_file(infile);
      LMAPI->close_file(outfile);
      LMAPI->buffer_printf(outfilename, sizeof(outfilename) - 1, "%s.errlog", LMAPI->get_string("queuefile"));
      LMAPI->unlink_file(outfilename);

      /* Do not endlessly requeue empty messages. */
      return 0;
   }
   if (sscanf(buf,"From %s",&from[0]) <= 0) {
      LMAPI->close_file(infile);
      LMAPI->close_file(outfile);
      LMAPI->buffer_printf(outfilename, sizeof(outfilename) - 1, "%s.errlog", LMAPI->get_string("queuefile"));
      LMAPI->unlink_file(outfilename);

      /* Do not endlessly requeue empty messages. */
      return 0;
   }

   LMAPI->write_file(outfile,"The following error message was delivered to the bounce address\n");
   LMAPI->write_file(outfile,"for the list '%s' from '%s'.\n",list,from);
   LMAPI->write_file(outfile,"\nYou are receiving this because you have the CCERRORS flag set.\n\n");

   while(parsedheader ? 0 : LMAPI->read_file(buf, sizeof(buf), infile)) {

      if (strncmp(buf,"X-Ecartis-Bounce",16) == 0) {
         LMAPI->log_printf(0,"CCERROR generated bounce!  Ignoring.\n");
         LMAPI->close_file(outfile);
         LMAPI->buffer_printf(outfilename, sizeof(outfilename) - 1, "%s.errlog", LMAPI->get_string("queuefile"));
         LMAPI->unlink_file(outfilename);
         LMAPI->close_file(infile);
         return 0;
      } 

      /* Legacy support */
      else if (strncmp(buf,"X-Listar-Bounce",15) == 0) {
         LMAPI->log_printf(0,"CCERROR generated bounce!  Ignoring.\n");
         LMAPI->close_file(outfile);
         LMAPI->buffer_printf(outfilename, sizeof(outfilename) - 1, "%s.errlog", LMAPI->get_string("queuefile"));
         LMAPI->unlink_file(outfilename);
         LMAPI->close_file(infile);
         return 0;
      } 
      else if (strncmp(buf,"X-SLList-Bounce",15) == 0) {
         LMAPI->log_printf(0,"CCERROR generated bounce!  Ignoring.\n");
         LMAPI->close_file(outfile);
         LMAPI->buffer_printf(outfilename, sizeof(outfilename) - 1, "%s.errlog", LMAPI->get_string("queuefile"));
         LMAPI->unlink_file(outfilename);
         LMAPI->close_file(infile);
         return 0;
      }
      if (buf[0] == '\n') {
         parsedheader = 1;
      }
   }

   LMAPI->close_file(infile);

   mimehandled = 0;
   outputfile = outfile;

   /* Catch anything using message/delivery-status */
   LMAPI->unmime_file(LMAPI->get_string("queuefile"), mimefilename);
   LMAPI->unlink_file(mimefilename);

   if ((infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) == NULL) {
      LMAPI->close_file(outfile);
      return 0;
   }

   done = 0;
   while (!done) {
      if (!LMAPI->read_file(buf, sizeof(buf), infile)) done = 1;
      if (buf[0] == '\n') done = 1;
   }

   if (mimehandled) errors = mimehandled;

   /* Time to get into the replacement for the Spaghetti Code Of Death! */
   if (!errors) {
	   done = 0;
	   while(LMAPI->read_file(buf, sizeof(buf), infile) && !done) {
		   if (strncasecmp(buf,"This is the Postfix",19) == 0 ||
				   strncmp(buf,"This is the WebTV_Postfix",25) == 0) {
			   /* We're Postfix */
			   parse_postfix_bounce(infile,outfile,outfilename,&errors);
			   done = 1;
		   }
		   else if (strncasecmp(buf, "Hi. This is the qmail-send",26) == 0 ||
				   strncmp(buf, "Hi. This is the NetZero mail server.",36) == 0) {
			   /* We're qmail */
			   parse_qmail_bounce(infile,outfile,outfilename,&errors);
			   done = 1;
		   }
		   else if (strncasecmp(buf, "This message was created automatically", 38) == 0) {
			   /* We're Exim */
			   parse_exim_bounce(infile,outfile,outfilename,&errors);
			   done = 1;
		   }
		   else if (strncmp(buf, "------Transcript of session follows -------", 43) == 0) {
			   /* We're msn.com or something Microsoftish (?) */
			   parse_msn_bounce(infile,outfile,outfilename,&errors);
			   done = 1;
		   }
		   else if (strncmp(buf, "   ----- Transcript of session follows -----", 44) == 0) {
			   /* We're sendmail */
			   parse_sendmail_bounce(infile,outfile,outfilename,&errors);
			   done = 1;
		   }
		   else if (strncmp(buf, "[<00>] XMail bounce:", 20) == 0) {
			   /* We're XMail */
			   parse_xmail_bounce(infile,outfile,outfilename,buf,&errors);
			   done = 1;
		   }
		   else if (strncmp(buf, "------- Failure Reasons  --------", 33) == 0) {
			   /* We're Lotus */
			   parse_lotus_bounce(infile,outfile,outfilename,&errors);
			   done = 1;
		   }
		   else if (strncmp(buf, "Message from yahoo.com.", 24) == 0) {
			   /* We're yahoo.com */
			   parse_yahoo_bounce(infile,outfile,outfilename,&errors);
			   done = 1;
		   }
                   else if (strncmp(buf, "The MMS SMTP Relay is returning your message because:", 53) == 0) {
                           /* We're an MMS SMTP Relay */
                           parse_mms_relay_bounce(infile,outfile,outfilename,&errors);
                   }

                   /* Special or non-multiline cases */
                   else if (strncmp(buf, "User mailbox exceeds allowed size:", 34) == 0) {
                           /* We're an SMTPv Bounce */
                           char username[SMALL_BUF];

                           LMAPI->buffer_printf(username, sizeof(username) - 1, "%s", &buf[36]);
                           handle_error(550, username, "Mailbox full", outfilename);
                           LMAPI->write_file(outfile, "User: %s\n      (%d) %s\n", username, 550, "Mailbox full");
                           errors++;
                   }                      
                   else if (strncmp(buf, "(originally addressed to", 24) == 0) {
                           /* I have no idea what generates these bounces, 
                            * but they exist */

                           char username[SMALL_BUF], errorstr[SMALL_BUF];
                           char *workptr;
                           char newbuf[BIG_BUF];

                           workptr = strchr(buf, ')');

                           if (workptr != NULL) {
                              *workptr = 0;
                              LMAPI->buffer_printf(username, sizeof(username) - 1, "%s", &buf[26]);

                              LMAPI->read_file(newbuf, sizeof(newbuf), infile);
                              LMAPI->read_file(newbuf, sizeof(newbuf), infile);
                              LMAPI->read_file(errorstr, sizeof(errorstr), infile);

                              handle_error(550, username, errorstr, outfilename);
                              LMAPI->write_file(outfile, "User: %s\n      (550) %s\n", username, errorstr);

                              errors++;
                           }
                           
                   }


		   /* More parse types go here later */
	   }
   }

   if (!errors) {
      LMAPI->write_file(outfile,"%s was unable to find a recognizable bounce\n", SERVICE_NAME_MC);
      LMAPI->write_file(outfile,"in this message.  You may wish to review it manually.\n\n");
   }

   LMAPI->write_file(outfile,"--------------- Error message follows ----------------------\n");
   LMAPI->rewind_file(infile);

   /* Eat our 'From' header again. */
   LMAPI->read_file(buf, sizeof(buf), infile);

   while(LMAPI->read_file(buf, sizeof(buf), infile)) {
      LMAPI->write_file(outfile,"%s",buf);
   }

   LMAPI->write_file(outfile, "--------------- Error message done -------------------------\n");
   LMAPI->close_file(outfile);
   LMAPI->close_file(infile);

   LMAPI->buffer_printf(outfilename, sizeof(outfilename) - 1, "%s.errlog", LMAPI->get_string("queuefile"));
   LMAPI->replace_file(outfilename,LMAPI->get_string("queuefile"));

   fromaddy = LMAPI->get_var("list-owner");
   if (!fromaddy) fromaddy = LMAPI->get_string("listserver-admin");

   LMAPI->buffer_printf(buf, sizeof(buf) - 1, "Bounced message for '%s'", list);

   LMAPI->buffer_printf(buf2, sizeof(buf2) - 1, "X-%s-Bounce: %s for %s",
     SERVICE_NAME_MC, from, list);
   LMAPI->set_var("stocksend-extra-headers",buf2,VAR_TEMP);
   LMAPI->flagged_send_textfile(fromaddy,list,"CCERRORS",
                                LMAPI->get_string("queuefile"), buf);
   LMAPI->clean_var("stocksend-extra-headers",VAR_TEMP);

   return 1;
}

HOOK_HANDLER(hook_local_bounce_detect)
{
    char addr[BIG_BUF];
    char outfilename[BIG_BUF];
    int i;

    if(!LMAPI->get_var("list"))
        return HOOK_RESULT_OK;

    i = atoi(LMAPI->get_string("bounce-error"));
    LMAPI->buffer_printf(addr, sizeof(addr) - 1, "%s", LMAPI->get_string("bounce-address"));

    LMAPI->listdir_file(outfilename,LMAPI->get_string("list"),"watches");

    handle_error(i, addr, "local bounce detected", outfilename);
    return HOOK_RESULT_OK;
}

MODE_HANDLER(mode_bounce)
{
   const char *list = LMAPI->get_var("list");
   if(!list) return MODE_ERR;

   if (parse_bounce(list))
       process_watches(list,1);
   return MODE_OK;
}

CMDARG_HANDLER(cmdarg_bounce)
{
   if (!argv[0] || !LMAPI->list_valid(argv[0])) {
      if (!argv[0]) {
         LMAPI->internal_error("-bounce requires a list as a parameter");
      } else {
         char buf[BIG_BUF];
         LMAPI->buffer_printf(buf, sizeof(buf) - 1, "-bounce requires a valid list as a parameter; %s is not valid.", argv[0]);
         LMAPI->internal_error(&buf[0]);
      }
      return CMDARG_ERR;
   } else {
      LMAPI->set_var("list",argv[0], VAR_GLOBAL);
      LMAPI->set_var("mode","bounce", VAR_GLOBAL);
      return CMDARG_OK;
   }
}

MODE_HANDLER(mode_procbounce)
{
   char buf[BIG_BUF], dname[BIG_BUF];
   int status;

   if(!(status = LMAPI->walk_lists(&dname[0])))
      return MODE_ERR;

   LMAPI->log_printf(5,"Processing error watches for all lists...\n"); 

   while(status) {
      if(dname[0] == '.') {
         status = LMAPI->next_lists(&dname[0]);
         continue;
      } else {
         if(LMAPI->list_valid(dname)) {
            LMAPI->listdir_file(buf, dname, "watches");
            if(LMAPI->exists_file(buf)) {
               LMAPI->wipe_vars(VAR_LIST|VAR_TEMP);
               LMAPI->set_var("list", dname, VAR_GLOBAL);
               LMAPI->list_read_conf();
               process_watches(dname, LMAPI->get_bool("force-procbounce") ? 0 : 1);
            }
         }
      }
      status = LMAPI->next_lists(&dname[0]);
   }

   return MODE_OK;
}

CMDARG_HANDLER(cmdarg_procbounce)
{
   LMAPI->set_var("fakequeue","yes", VAR_GLOBAL);
   LMAPI->set_var("mode","procbounce", VAR_GLOBAL);
   return CMDARG_OK;
}

CMDARG_HANDLER(cmdarg_forcebounce)
{
   LMAPI->set_var("fakequeue","yes", VAR_GLOBAL);
   LMAPI->set_var("mode","procbounce", VAR_GLOBAL);
   LMAPI->set_var("force-procbounce","yes",VAR_GLOBAL);
   return CMDARG_OK;
}

void bouncer_switch_context(void)
{
   LMAPI->log_printf(15, "Switching context in module Bouncer\n");
}

void bouncer_init(void)
{
   LMAPI->log_printf(10, "Initializing module Bouncer\n");
}

int bouncer_upgradelist(int prev, int cur)
{
   LMAPI->log_printf(10, "Upgrading lists in module Bouncer\n");
   return 1;
}

int bouncer_upgrade(int prev, int cur)
{
   LMAPI->log_printf(10, "Upgrading module Bouncer\n");
   return 1;
}

void bouncer_unload(void)
{
   LMAPI->log_printf(10, "Unloading module Bouncer\n");
}

void bouncer_load(struct LPMAPI *api)
{
   LMAPI = api;
   LMAPI->log_printf(10, "Loading module Bouncer\n");

   LMAPI->add_module("Bouncer","Handles 'bounced' messages automatically.");

   LMAPI->add_hook("LOCAL-BOUNCE", 100, hook_local_bounce_detect);

   LMAPI->add_cmdarg("-bounce",1, "<list>", cmdarg_bounce);
   LMAPI->add_mode("bounce",mode_bounce);

   LMAPI->add_cmdarg("-procbounce",0, NULL, cmdarg_procbounce);
   LMAPI->add_cmdarg("-forcebounce",0, NULL, cmdarg_forcebounce);
   LMAPI->add_mode("procbounce",mode_procbounce);

   LMAPI->add_flag("REPORTS","User wishes to have reports sent to them.  Only has meaning for admins.",ADMIN_SAFESET | ADMIN_SAFEUNSET);
   LMAPI->add_flag("CCERRORS", "User wishes to have bounces cc'd to them.  Only has meaning for admins.",ADMIN_SAFESET | ADMIN_SAFEUNSET);
   LMAPI->add_flag("PROTECTED", "User will never be unsubscribed by bouncer.",ADMIN_SAFESET | ADMIN_SAFEUNSET);

   LMAPI->add_file("bouncer-vacation", "bouncer-vacation-file", "File sent to the user when they are automatically set VACATION by Bouncer.");
   LMAPI->add_file("bouncer-unsub", "bouncer-unsub-file", "File sent to the user when they are automatically unsubscribed from a list by Bouncer.");

   /* Variable registration */
   LMAPI->register_var("bounce-error", NULL, NULL, NULL, NULL, VAR_STRING,
                       VAR_INTERNAL|VAR_TEMP);
   LMAPI->register_var("bounce-address", NULL, NULL, NULL, NULL, VAR_STRING,
                       VAR_INTERNAL|VAR_TEMP);
   LMAPI->register_var("force-procbounce", "no", NULL, NULL, NULL, VAR_BOOL,
                       VAR_INTERNAL|VAR_GLOBAL);
   LMAPI->register_var("bounce-max-fatal", "10", "Bounce Handling",
                       "Maximum number of fatal bounces before action is taken.",
                       "bounce-max-fatal = 10", VAR_INT, VAR_ALL);
   LMAPI->register_var("bounce-max-transient", "30", "Bounce Handling",
                       "Maximum number of transient bounces before action is taken.",
                       "bounce-max-transient = 100", VAR_INT, VAR_ALL);
   LMAPI->register_var("bounce-never-unsub", "no", "Bounce Handling",
                       "Should the user be unsubscribed when more than max fatal bounces have occured, or just set vacation.",
                       "bounce-never-unsub = off", VAR_BOOL, VAR_ALL);
   LMAPI->register_var("bounce-always-unsub", "no", "Bounce Handling",
                       "Should the user be unsubscribed when more than max transient bounces have occured.",
                       "bounce-always-unsub = false", VAR_BOOL, VAR_ALL);
   LMAPI->register_alias("always-unsub", "bounce-always-unsub");
   LMAPI->register_var("bounce-never-vacation", "no", "Bounce Handling",
                       "Should the user ever be set vacation for exceeding the maximum number of bounces.",
                       "bounce-never-vacation = yes", VAR_BOOL, VAR_ALL);
   LMAPI->register_var("bounce-timeout-days", "7", "Bounce Handling",
                       "Length of time (in days) during which the maximum number of bounces must not be exceeded.",
                       "bounce-timeout-days = 7", VAR_INT, VAR_ALL);
   LMAPI->register_var("bouncer-vacation-file", "text/bounce-vacation.txt",
                       "Bounce Handling", "File under the list directory to send to a user when they are automatically set vacation by the bouncer.",
                       "bouncer-vacation-file = text/bounce-vacation.txt",
                       VAR_STRING, VAR_ALL);
   LMAPI->register_var("bouncer-unsub-file", "text/bounce-unsub.txt",
                       "Bounce Handling", "File under the list directory to send to a user when they are automatically unsubscribed by the bouncer.",
                       "bouncer-unsub-file = text/bounce-unsub.txt",
                       VAR_STRING, VAR_ALL);
}
