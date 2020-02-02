/* Ok.  This is REALLY the digest, rewritten to conform to what everyone
   has asked for.  Please, stop making me rewrite the module! :)  

   This time, we even have RFC1153 compliancy!
    --sparks
 */


#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#else
#include <winsock2.h>
#endif

#include "lpm.h"

struct LPMAPI *LMAPI;

void digest_format_name(char *newbuf, int len, const char *format,
     int volume, int issue, const char *listname)
{
   char buf[SMALL_BUF];
   char *tbuf;
   time_t now;
   struct tm *tm_now;

   tbuf = (char *)malloc(len);

   time(&now);
   tm_now = localtime(&now);

   LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%d", volume);
   LMAPI->strreplace(newbuf,len,format,"%v", buf);
   strncpy(tbuf, newbuf, len - 1);

   LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%.02d", volume);
   LMAPI->strreplace(newbuf,len,tbuf,"%V", buf);
   strncpy(tbuf, newbuf, len - 1);

   LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%d", issue);
   LMAPI->strreplace(newbuf,len,tbuf,"%i", buf);
   strncpy(tbuf, newbuf, len - 1);

   LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%.03d", issue);
   LMAPI->strreplace(newbuf,len,tbuf,"%n", buf);
   strncpy(tbuf, newbuf, len - 1);

   strftime(buf, sizeof(buf) - 2,"%b",tm_now);
   LMAPI->strreplace(newbuf,len,tbuf,"%M", buf);
   strncpy(tbuf, newbuf, len - 1);

   strftime(buf, sizeof(buf) - 1,"%a",tm_now);
   LMAPI->strreplace(newbuf,len,tbuf,"%W", buf);
   strncpy(tbuf, newbuf, len - 1);

   LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%.05d", issue);
   LMAPI->strreplace(newbuf,len,tbuf,"%I", buf);
   strncpy(tbuf, newbuf, len - 1);

   LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%s", listname);
   LMAPI->strreplace(newbuf,len,tbuf,"%l", buf);
   strncpy(tbuf, newbuf, len - 1);

   LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%.02d", tm_now->tm_mon + 1);
   LMAPI->strreplace(newbuf,len,tbuf,"%m", buf);
   strncpy(tbuf, newbuf, len - 1);

   LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%.02d", tm_now->tm_mday);
   LMAPI->strreplace(newbuf,len,tbuf,"%d", buf);
   strncpy(tbuf, newbuf, len - 1);

   LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%.02d", (tm_now->tm_year % 100));
   LMAPI->strreplace(newbuf,len,tbuf,"%y", buf);
   strncpy(tbuf, newbuf, len - 1);

   LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%.04d", tm_now->tm_year + 1900);
   LMAPI->strreplace(newbuf,len,tbuf,"%Y", buf);
   strncpy(tbuf, newbuf, len - 1);

   free(tbuf);
}

int digest_get_issue(const char *listname)
{
   FILE *digestdata;
   char digestfilename[BIG_BUF]; /* Changed from SMALL_BUF to BIG_BUF due to listdir_file */
   char buffer[BIG_BUF];
   int issue;

   LMAPI->listdir_file(digestfilename,listname,"digestinfo3");

   if ((digestdata = LMAPI->open_file(digestfilename,"r")) == NULL) {
      return 1;
   }

   LMAPI->read_file(buffer, sizeof(buffer), digestdata);

   issue = atoi(buffer);

   LMAPI->close_file(digestdata);

   return issue;
}

int digest_get_volume(const char *listname)
{
   FILE *digestdata;
   char digestfilename[BIG_BUF]; /* Changed from SMALL_BUF to BIG_BUF due to listdir_file */
   char buffer[BIG_BUF];
   int volume;

   LMAPI->listdir_file(digestfilename,listname,"digestinfo3");

   if ((digestdata = LMAPI->open_file(digestfilename,"r")) == NULL) {
      return 1;
   }

   LMAPI->read_file(buffer, sizeof(buffer), digestdata);
   LMAPI->read_file(buffer, sizeof(buffer), digestdata);

   volume = atoi(buffer);

   LMAPI->close_file(digestdata);

   return volume;
}

time_t digest_get_lasttime(const char *listname)
{
   FILE *digestdata;
   char digestfilename[BIG_BUF]; /* Changed from SMALL_BUF to BIG_BUF due to listdir_file */
   char buffer[BIG_BUF];
   time_t lasttime;

   LMAPI->listdir_file(digestfilename,listname,"digestinfo3");

   if ((digestdata = LMAPI->open_file(digestfilename,"r")) == NULL) {
      time(&lasttime);

      return lasttime;
   }

   LMAPI->read_file(buffer, sizeof(buffer), digestdata);
   LMAPI->read_file(buffer, sizeof(buffer), digestdata);
   LMAPI->read_file(buffer, sizeof(buffer), digestdata);
   if (LMAPI->read_file(buffer, sizeof(buffer), digestdata)) {
      lasttime = atoi(buffer);
   } else {
      time(&lasttime);
   }

   LMAPI->close_file(digestdata);

   return lasttime;
}

int digest_get_indexnum(const char *listname)
{
   FILE *digestdata;
   char digestfilename[BIG_BUF]; /* Changed from SMALL_BUF to BIG_BUF due to listdir_file */
   char buffer[BIG_BUF];
   int indexnum;

   indexnum = 0;

   LMAPI->listdir_file(digestfilename,listname,"digestinfo3");

   if ((digestdata = LMAPI->open_file(digestfilename,"r")) == NULL) {
      return 1;
   }

   LMAPI->read_file(buffer, sizeof(buffer), digestdata);
   LMAPI->read_file(buffer, sizeof(buffer), digestdata);
   LMAPI->read_file(buffer, sizeof(buffer), digestdata);
   if (!LMAPI->read_file(buffer, sizeof(buffer), digestdata)) {
      indexnum = 1;
   } else {
      if (!LMAPI->read_file(buffer, sizeof(buffer), digestdata)) {
         indexnum = 1;
      } else {
         indexnum = atoi(buffer);
      }
   }

   LMAPI->close_file(digestdata);

   return indexnum;
}

void digest_increment_index(const char *listname)
{
   int volume, issue, indexnum, year;
   time_t lasttime, now;
   struct tm *tm_now;
   char buffer[BIG_BUF], digestfilename[BIG_BUF];
   FILE *digestdata;

   LMAPI->listdir_file(digestfilename,listname,"digestinfo3");

   now = time(NULL);
   tm_now = localtime(&now);
   

   if ((digestdata = LMAPI->open_file(digestfilename,"r")) == NULL) {
      volume = 1;
      issue = 1;
      year = tm_now->tm_year + 1900;
      lasttime = now;
      indexnum = 1;
   } else {
      LMAPI->read_file(buffer, sizeof(buffer), digestdata);
      issue = atoi(buffer);

      LMAPI->read_file(buffer, sizeof(buffer), digestdata);
      volume = atoi(buffer);

      LMAPI->read_file(buffer, sizeof(buffer), digestdata);
      year = atoi(buffer);

      if (LMAPI->read_file(buffer, sizeof(buffer), digestdata)) {
         lasttime = (time_t)atoi(buffer);
         if (!LMAPI->read_file(buffer, sizeof(buffer), digestdata)) {
           indexnum = 1;
         } else {
           indexnum = atoi(buffer);
         }
      } else {
	  indexnum = 1;
	  lasttime = now;
      }

      LMAPI->close_file(digestdata);
   }

   if ((digestdata = LMAPI->open_file(digestfilename,"w")) == NULL) {
      return;
   }

   LMAPI->write_file(digestdata,"%d\n%d\n%d\n%d\n%d\n",
      issue, volume, year, (int)lasttime, indexnum + 1);
   LMAPI->close_file(digestdata);
}

void digest_increment_number(const char *listname)
{
   FILE *digestdata;
   char digestfilename[BIG_BUF]; /* Changed from SMALL_BUF to BIG_BUF due to listdir_file */
   char buffer[BIG_BUF];
   int volume, issue;
   int year;
   time_t now;
   struct tm *tm_now;

   time(&now);
   tm_now = localtime(&now);

   LMAPI->listdir_file(digestfilename,listname,"digestinfo3");

   if ((digestdata = LMAPI->open_file(digestfilename,"r")) == NULL) {
      volume = 1;
      issue = 1;
      year = tm_now->tm_year + 1900;
   } else {
      LMAPI->read_file(buffer, sizeof(buffer), digestdata);
      issue = atoi(buffer);

      LMAPI->read_file(buffer, sizeof(buffer), digestdata);
      volume = atoi(buffer);

      LMAPI->read_file(buffer, sizeof(buffer), digestdata);
      year = atoi(buffer);

      LMAPI->close_file(digestdata);
   }

   if ((digestdata = LMAPI->open_file(digestfilename,"w")) == NULL) {
      return;
   }

   issue++;

   if (year != (tm_now->tm_year + 1900)) {
      volume++;  issue = 1;
      year = tm_now->tm_year + 1900;
   }

   LMAPI->write_file(digestdata,"%d\n%d\n%d\n%d\n1\n",
      issue, volume, year, (int)now);
   LMAPI->close_file(digestdata);   
}

CMD_HANDLER(cmd_predigest)
{
   FILE *preamble, *digestfile;
   FILE *workfile;
   int issue;
   struct list_user user;
   char buf[BIG_BUF], tbuf[SMALL_BUF];

   if (!params->num && !LMAPI->get_var("list")) {
      LMAPI->spit_status("No list in current context.");
      return CMD_RESULT_CONTINUE;
   }

   if (params->num > 0) {
      if (!LMAPI->set_context_list(params->words[0])) {
         LMAPI->nosuch(params->words[0]);
         return CMD_RESULT_CONTINUE;
      }
   }

   if (LMAPI->get_bool("no-digest")) {
      LMAPI->spit_status("This list has digest disabled.");
      return CMD_RESULT_CONTINUE;
   }

   if (!LMAPI->user_find_list(LMAPI->get_string("list"),LMAPI->get_string("fromaddress"),
       &user)) {
      LMAPI->spit_status("You aren't a member of that list.");
      return CMD_RESULT_CONTINUE;
   }

   LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%s.predigest", LMAPI->get_string("queuefile"));
   
   if ((workfile = LMAPI->open_file(buf,"w")) == NULL) {
      LMAPI->spit_status("Local filesystem error, unable to complete request.");
      return CMD_RESULT_CONTINUE;
   }

   issue = digest_get_issue(LMAPI->get_string("list"));


   LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "digest/preamble.%d.work", issue);
   LMAPI->listdir_file(buf,LMAPI->get_string("list"),tbuf);
   if ((preamble = LMAPI->open_file(buf,"r")) == NULL) {
      LMAPI->close_file(workfile);
      LMAPI->spit_status("No posts yet for current digest issue.");
      LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%s.predigest", LMAPI->get_string("queuefile"));
      LMAPI->unlink_file(buf);
      return CMD_RESULT_CONTINUE;
   }

   LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "digest/digest.%d.work", issue);
   LMAPI->listdir_file(buf,LMAPI->get_string("list"),tbuf);
   if ((digestfile = LMAPI->open_file(buf,"r")) == NULL) {
      LMAPI->close_file(workfile);
      LMAPI->close_file(preamble);
      LMAPI->spit_status("No posts yet for current digest issue.");
      LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%s.predigest", LMAPI->get_string("queuefile"));
      LMAPI->unlink_file(buf);
      return CMD_RESULT_CONTINUE;
   }

   LMAPI->write_file(workfile,"Temporary Digest, sent by user request.\n---\n");

   while (LMAPI->read_file(buf, sizeof(buf), preamble)) {
      LMAPI->write_file(workfile,"%s",buf);
   }
   LMAPI->close_file(preamble);

   LMAPI->write_file(workfile, "\n----------------------------------------------------------------------\n\n");

   while (LMAPI->read_file(buf, sizeof(buf), digestfile)) {
      LMAPI->write_file(workfile,"%s",buf);
   }
   LMAPI->close_file(digestfile);
   LMAPI->close_file(workfile);

   LMAPI->set_var("form-subject","Interim Digest",VAR_TEMP);
   if (LMAPI->get_var("send-as")) {
      LMAPI->set_var("form-send-as", LMAPI->get_string("send-as"), VAR_TEMP);
   } else {
      LMAPI->set_var("form-send-as", LMAPI->get_string("list-owner"), VAR_TEMP);
   }
   LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%s.predigest", LMAPI->get_string("queuefile"));
   LMAPI->send_textfile(LMAPI->get_string("fromaddress"),buf);
   LMAPI->unlink_file(buf);

   LMAPI->spit_status("Interim digest sent.");

   return CMD_RESULT_CONTINUE;
}

HOOK_HANDLER(hook_presend_digest_fork)
{
   int volume, issue;
   time_t now;
   struct tm *tm_now;
   char buffer[BIG_BUF], tbuf[SMALL_BUF], subjectline[SMALL_BUF];
   FILE *preamble, *digestbody, *messagefile;
   FILE *workfile;
#ifndef WIN32
   pid_t pid;
#else
   int pid;
#endif
   int newpreamble, inbody, altertoc;
   int forked, gotfrom, gotsubj, indexnum;

   forked = 0; altertoc = 0; gotfrom = 0; gotsubj = 0;

   LMAPI->log_printf(15,"In hook_presend_digest_fork\n");

   if (LMAPI->get_bool("no-digest")) return HOOK_RESULT_OK;

   if (LMAPI->get_bool("digest-altertoc")) altertoc = 1;

   if (LMAPI->get_bool("moderated")) {
      if (!LMAPI->get_var("moderated-approved-by")) return HOOK_RESULT_OK;
   }

   LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s.digestinput", LMAPI->get_string("queuefile"));
   if ((workfile = LMAPI->open_file(buffer,"w")) == NULL)
      return HOOK_RESULT_OK;

   if ((messagefile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) == NULL) {
     LMAPI->close_file(workfile);
     return HOOK_RESULT_OK;
   }

   while(LMAPI->read_file(buffer, sizeof(buffer), messagefile)) {
     LMAPI->write_file(workfile,"%s",buffer);
   }
   LMAPI->close_file(workfile);
   LMAPI->close_file(messagefile);

   LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s.digestinput", LMAPI->get_string("queuefile"));

   if ((messagefile = LMAPI->open_file(buffer,"r")) == NULL) {
     LMAPI->unlink_file(buffer);
     return HOOK_RESULT_OK;
   } else LMAPI->close_file(messagefile);

   /* We fork and use the spare queuefile.  This prevents delivery
      from being blocked while the digest is being updated, AND
      keeps the digest in sync.  Without this, open_file() could
      cause delivery times to lag to unacceptable levels. 
   
      As this does weird/bad things under Win32, the Windows version of
      Ecartis does not fork, it forces 'digest-no-fork' behavior.
   */

#ifndef WIN32
   if (!LMAPI->get_bool("digest-no-fork")) {
      pid = fork();
   } else pid = -1;
#else
	pid = -1;
#endif

   if (pid > 0)
     return HOOK_RESULT_OK;

   if (pid == 0) {
     forked = 1;
     LMAPI->log_printf(9,"In digest subprocess (forked)...\n");
   }

   if (pid < 0) {
     LMAPI->log_printf(9,"Not forking digest process.  We'll run in current process.\n");
   }

   if (!LMAPI->get_bool("unmimed-file") && !LMAPI->get_bool("digest-no-unmime")) {
     char buf1[SMALL_BUF], buf2[SMALL_BUF];

     LMAPI->buffer_printf(buf1, sizeof(buf1) - 1, "%s.digestinput", LMAPI->get_string("queuefile"));
     LMAPI->buffer_printf(buf2, sizeof(buf2) - 1, "%s.digestunmime", LMAPI->get_string("queuefile"));

     LMAPI->unmime_file(buf1, buf2);

     if (LMAPI->get_bool("just-unmimed")) {
        LMAPI->unlink_file(buf1);  /* Unneccesary? --JT */
                                   /* Was required for Windows.  --Rach */
        LMAPI->replace_file(buf2,buf1);
     }
   }

   LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s.digestinput", LMAPI->get_string("queuefile"));

   if ((messagefile = LMAPI->open_file(buffer,"r")) == NULL) {
     LMAPI->unlink_file(buffer);
     exit(0);
   };

   LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s.digestinput", LMAPI->get_string("queuefile"));

   newpreamble = 0;
   inbody = 0;

   time(&now);
   tm_now = localtime(&now);

   issue = digest_get_issue(LMAPI->get_string("list"));
   volume = digest_get_volume(LMAPI->get_string("list"));

   LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "digest/preamble.%d.work", issue);
   LMAPI->listdir_file(buffer,LMAPI->get_string("list"),tbuf);
   LMAPI->mkdirs(buffer);

   if ((preamble = LMAPI->open_file(buffer,"r")) == NULL) {
      newpreamble = 1;
      if ((preamble = LMAPI->open_file(buffer, "w")) == NULL) {
         LMAPI->close_file(messagefile);
         LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s.digestinput", LMAPI->get_string("queuefile"));
         LMAPI->unlink_file(buffer);
         if (forked) exit(0); else return HOOK_RESULT_OK;
      }
   } else {
      LMAPI->close_file(preamble);
      if ((preamble = LMAPI->open_file(buffer,"a")) == NULL) {
         LMAPI->close_file(messagefile);
         LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s.digestinput", LMAPI->get_string("queuefile"));
         LMAPI->unlink_file(buffer);
         if (forked) exit(0); else return HOOK_RESULT_OK;
      }
   }

   LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "digest/digest.%d.work", issue);
   LMAPI->listdir_file(buffer,LMAPI->get_string("list"),tbuf);

   if ((digestbody = LMAPI->open_exclusive(buffer,"a")) == NULL) {
      LMAPI->close_file(preamble);
      LMAPI->close_file(messagefile);
      LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s.digestinput", LMAPI->get_string("queuefile"));
      LMAPI->unlink_file(buffer);
      if (forked) exit(0); else return HOOK_RESULT_OK;
   }

   if (newpreamble) {
      LMAPI->write_file(preamble,"%s Digest\t", LMAPI->get_string("list"));
      if(LMAPI->get_bool("digest-alter-datestamp")) {
         strftime(buffer, sizeof(buffer) - 1,"%A, %B %d %Y",tm_now);
      } else {
         strftime(buffer, sizeof(buffer) - 1,"%a, %d %b %Y",tm_now);
      }
      LMAPI->write_file(preamble,"%s\tVolume: %.02d  Issue: %.03d\n\n", buffer,
        volume, issue);
      if (!LMAPI->get_bool("digest-no-toc"))
         LMAPI->write_file(preamble,"In This Issue:\n");
   }

   indexnum = digest_get_indexnum(LMAPI->get_string("list"));

   if (!LMAPI->get_bool("digest-no-toc") && altertoc) {
      LMAPI->write_file(preamble,"\t#%d:", indexnum);
      LMAPI->write_file(digestbody,"Msg: #%d in digest\n", indexnum); 
   }

   digest_increment_index(LMAPI->get_string("list"));

   while(LMAPI->read_file(buffer, sizeof(buffer), messagefile)) {
      if (!inbody) {
         if (!strncasecmp(buffer,"From:",5)) {
            LMAPI->write_file(digestbody, "%s", buffer);
            if (!LMAPI->get_bool("digest-no-toc") && altertoc) {
               LMAPI->write_file(preamble,"\t%s", buffer);
            }
            gotfrom = 1;
         }
         else if (!strncasecmp(buffer,"Date:",5)) {
            LMAPI->write_file(digestbody, "%s", buffer);
         }
         else if (!strncasecmp(buffer,"Subject:",8)) {
            char tempbuf[SMALL_BUF], tempbuf2[BIG_BUF];
            char *tptr;
            const char *tag;

            tptr = &buffer[8];
            while (*tptr && isspace((int)(*tptr))) tptr++;

            buffer[strlen(buffer) - 1] = 0;

            if(LMAPI->get_bool("humanize-quotedprintable")) {
		LMAPI->unquote_string(tptr, tempbuf2, sizeof(tempbuf2) - 1);
                LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%.70s", tempbuf2);
            } else {
                LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%.70s", tptr);
            }

            tag = LMAPI->get_var("subject-tag");

            if (tag && LMAPI->get_bool("digest-strip-tags")) {
               tptr = LMAPI->strcasestr(tempbuf,tag);

               if (tptr) {
                  *tptr = 0;
                  tptr += strlen(tag) + 1;
                  LMAPI->buffer_printf(tempbuf2, sizeof(tempbuf2) - 1, "%s%s", tempbuf, tptr);
                  LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s", tempbuf2);
               }
            }

            LMAPI->write_file(digestbody, "Subject: %.70s\n", tempbuf);

            LMAPI->buffer_printf(subjectline, sizeof(subjectline) - 1, "%.60s", tempbuf);

            if (!LMAPI->get_bool("digest-no-toc")) {
               if (!altertoc) 
                     LMAPI->write_file(preamble, "\t\t%.60s\n", tempbuf);
            }
            gotsubj = 1;
         }
         else if (!strncasecmp(buffer,"Keywords:",9)) {
            LMAPI->write_file(digestbody, "%.70s", buffer);
         }
         else if (buffer[0] == '\n') {
            LMAPI->write_file(digestbody, "\n");
            inbody = 1;
         }
      } else {
         LMAPI->write_file(digestbody,"%s",buffer);
      }
   }

   if (!LMAPI->get_bool("digest-no-toc") && altertoc) {
      if (!gotfrom) {
         LMAPI->write_file(preamble,"\tFrom: %s\n",
            LMAPI->get_string("fromaddress"));
         if (gotsubj) {
            LMAPI->write_file(preamble,"\tSubject: %s\n", subjectline);
         } else {
            LMAPI->write_file(preamble,"\tSubject: No subject\n");
         }
      } else {
         if (gotsubj) {
                  LMAPI->write_file(preamble,"\t\tSubject: %s\n", subjectline);
        } else { 
                  LMAPI->write_file(preamble,"\t\tSubject: No subject\n");
        }
      }
   }

   LMAPI->write_file(digestbody,"\n------------------------------\n\n");

   LMAPI->close_file(messagefile);
   LMAPI->close_file(preamble);
   LMAPI->close_file(digestbody);

   LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s.digestinput", LMAPI->get_string("queuefile"));
   LMAPI->unlink_file(buffer);

   if (forked) exit(0); else return HOOK_RESULT_OK;
}

void digest_send(const char *listname)
{
   FILE *infile, *outfile;
   char buffer[BIG_BUF], outfilename[BIG_BUF];
   char tbuf[SMALL_BUF];
   time_t now;
   char datestr[80];
   int volume, issue;
   unsigned int loop;
   const char *sendas;


   time(&now);

   issue = digest_get_issue(listname);
   volume = digest_get_volume(listname);

   LMAPI->buffer_printf(outfilename, sizeof(outfilename) - 1, "%s.digestout", LMAPI->get_string("queuefile"));
   if ((outfile = LMAPI->open_file(outfilename,"w")) == NULL) {
      return;
   }

   LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "digest/preamble.%d.work", issue);
   LMAPI->listdir_file(buffer,listname,tbuf);

   LMAPI->mkdirs(buffer);
   if ((infile = LMAPI->open_file(buffer,"r")) == NULL) {
      LMAPI->close_file(outfile);
      LMAPI->unlink_file(outfilename);
      return;
   }

   digest_increment_number(listname);

   while(LMAPI->read_file(buffer, sizeof(buffer), infile)) {
      LMAPI->write_file(outfile,"%s",buffer);
   }

   LMAPI->close_file(infile);

   LMAPI->listdir_file(buffer,listname,LMAPI->get_string("digest-administrivia-file"));

   if (LMAPI->exists_file(buffer)) {
      char tbuf2[BIG_BUF];

      LMAPI->buffer_printf(tbuf2, sizeof(tbuf2) - 1, "%s.administrivia-expand",
        LMAPI->get_string("queuefile"));
      LMAPI->liscript_parse_file(buffer,tbuf2);

      if ((infile = LMAPI->open_file(tbuf2,"r")) != NULL) {   
         LMAPI->write_file(outfile,"\nAdministrivia:\n\n");

         while(LMAPI->read_file(buffer, sizeof(buffer), infile)) {
            LMAPI->write_file(outfile,"%s",buffer);
         }

         LMAPI->close_file(infile);
      }
      LMAPI->unlink_file(tbuf2);

      if (LMAPI->get_bool("digest-transient-administrivia")) {
         LMAPI->listdir_file(buffer,listname,LMAPI->get_string("digest-administrivia-file"));
         LMAPI->unlink_file(buffer);
      }
   }

   LMAPI->write_file(outfile, "\n----------------------------------------------------------------------\n\n");

   LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "digest/digest.%d.work", issue);
   LMAPI->listdir_file(buffer,listname,tbuf);

   if ((infile = LMAPI->open_file(buffer,"r")) == NULL) {
      LMAPI->close_file(outfile);
      (void)LMAPI->unlink_file(outfilename);
      return;
   }   

   while(LMAPI->read_file(buffer, sizeof(buffer), infile)) {
      LMAPI->write_file(outfile,"%s",buffer);
   }

   LMAPI->close_file(infile);

   LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "End of %s Digest V%d #%d", listname,
           volume, issue);
   LMAPI->write_file(outfile,"%s\n", buffer);

   for (loop = 0; loop < strlen(buffer); loop++) {
      LMAPI->putc_file('*',outfile);
   }
   LMAPI->putc_file('\n', outfile);
   LMAPI->close_file(outfile); 

   LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "digest/digest.%d.work", issue);
   LMAPI->listdir_file(buffer,listname,tbuf);
   (void)LMAPI->unlink_file(buffer);

   LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "digest/preamble.%d.work", issue);
   LMAPI->listdir_file(buffer,listname,tbuf);
   (void)LMAPI->unlink_file(buffer);

   if ((infile = LMAPI->open_file(outfilename,"r")) == NULL) {
      return;
   }

   LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s.digestsend", LMAPI->get_string("queuefile"));
   if ((outfile = LMAPI->open_file(buffer,"w+")) == NULL) {
      LMAPI->close_file(infile);
      return;
   }

   LMAPI->get_date(datestr, sizeof(datestr), now);

   LMAPI->write_file(outfile,"Received: with %s (v%s; list %s); %s\n",
           SERVICE_NAME_UC, VER_PRODUCTVERSION_STR, listname, datestr);

   LMAPI->write_file(outfile,"Date: %s\n",datestr);

   if (LMAPI->get_var("digest-from")) {
      LMAPI->write_file(outfile,"From: %s\n", LMAPI->get_string("digest-from"));
   } else {
      const char *fromname = LMAPI->get_var("listserver-full-name");
      LMAPI->write_file(outfile,"From: %s <%s>\n", fromname,
                        LMAPI->get_string("listserver-address"));
   }


   if (LMAPI->get_var("digest-to")) {
      LMAPI->write_file(outfile,"To: %s digest users <%s>\n",
        listname, LMAPI->get_string("digest-to"));
   } else {
      LMAPI->write_file(outfile,"To: %s digest users <%s>\n",
         listname, LMAPI->get_string("listserver-address"));
   }
   if (LMAPI->get_var("reply-to")) {
      LMAPI->write_file(outfile,"Reply-to: %s\n", LMAPI->get_string("reply-to"));
   }

   LMAPI->write_file(outfile,"Subject: %s Digest V%d #%d\n\n",
     listname, volume, issue);

   while(LMAPI->read_file(buffer, sizeof(buffer), infile)) {
      LMAPI->write_file(outfile,"%s",buffer);
   }

   LMAPI->close_file(infile);
   LMAPI->close_file(outfile);

   LMAPI->nuke_tolist();
   LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s.digestsend", LMAPI->get_string("queuefile"));
   if(LMAPI->do_hooks("DIGEST") == HOOK_RESULT_FAIL) {
       LMAPI->unlink_file(buffer);
       return;
   }
   if(LMAPI->do_hooks("TOLIST") == HOOK_RESULT_FAIL) {
       LMAPI->unlink_file(buffer);
       return;
   }

   sendas = LMAPI->get_var("send-as");
   if (!sendas) sendas = LMAPI->get_var("list-owner");
   if (!sendas) sendas = LMAPI->get_var("listserver-admin");

   if(!LMAPI->send_to_tolist(sendas, buffer, 0, 0, LMAPI->get_bool("full-bounce"))) {
      (void)LMAPI->unlink_file(buffer);
   }

   (void) LMAPI->unlink_file(outfilename);

   if (LMAPI->get_bool("digest-transient")) {
      (void) LMAPI->unlink_file(buffer);
   } else {
      const char *digestname;
      char digestfilename[BIG_BUF];

      LMAPI->buffer_printf(digestfilename, sizeof(digestfilename) - 1, "%s", buffer);

      digestname =  LMAPI->get_var("digest-name");

      if (!digestname) {
         LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "digest/V%.2d/%.5d", volume, issue);
         LMAPI->listdir_file(buffer,listname,tbuf);
         LMAPI->mkdirs(buffer);
      } else {
         digest_format_name(buffer, sizeof(buffer) - 1,digestname,volume,issue,
           listname);
         LMAPI->mkdirs(buffer);
      }

       LMAPI->replace_file(digestfilename,buffer);
   }
}

HOOK_HANDLER(hook_tolist_digest)
{
   const char *modename;

   if (LMAPI->get_bool("no-digest")) return HOOK_RESULT_OK;

   modename = LMAPI->get_string("mode");

   if ((strcmp(modename,"resend") == 0) || (strcmp(modename,"approved")) == 0) {
      LMAPI->remove_flagged_all("DIGEST");
   } else if (strcmp(modename,"digest") == 0) {
      LMAPI->remove_unflagged_all("DIGEST");
      LMAPI->add_from_list_flagged(LMAPI->get_string("list"),"DIGEST2");
   }

   return HOOK_RESULT_OK;
}

HOOK_HANDLER(hook_setflag_digest)
{
   if ((strcasecmp(LMAPI->get_string("setflag-flag"),"DIGEST") == 0) ||
       (strcasecmp(LMAPI->get_string("setflag-flag"),"DIGEST2") == 0)) {
       if (LMAPI->get_bool("no-digest")) {
          LMAPI->spit_status("Digest is disabled for this list, flag is useless.");
          return HOOK_RESULT_FAIL;
       }
   }

   return HOOK_RESULT_OK;
}

HOOK_HANDLER(hook_after_digest_updatecheck)
{
   int maxsize, issue, maxtime;
   time_t lasttime, now;
   struct stat fst;
   char digestfilename[BIG_BUF], tbuf[SMALL_BUF]; /* Changed disgestfilename from SMALL_BUF to BIG_BUF due to listdir_file */

   if (LMAPI->get_bool("no-digest")) return HOOK_RESULT_OK;

   maxsize = LMAPI->get_number("digest-max-size");

   issue = digest_get_issue(LMAPI->get_string("list"));

   LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "digest/digest.%d.work", issue);
   LMAPI->listdir_file(digestfilename,LMAPI->get_string("list"),tbuf);

   if(maxsize > 0) {
       if(stat(digestfilename, &fst) == 0) {
           if(fst.st_size >= maxsize) {
               LMAPI->set_var("mode", "digest", VAR_TEMP);
               digest_send(LMAPI->get_string("list"));
               LMAPI->clean_var("mode", VAR_TEMP);
               return HOOK_RESULT_OK;
           }
       }
   }   

   maxtime = LMAPI->get_seconds("digest-max-time");

   if (maxtime > 0) {
      lasttime = digest_get_lasttime(LMAPI->get_string("list"));
      time(&now);

      if (now >= (lasttime + maxtime)) {
         LMAPI->set_var("mode", "digest", VAR_TEMP);
         digest_send(LMAPI->get_string("list")); 
         LMAPI->clean_var("mode", VAR_TEMP);
         return HOOK_RESULT_OK;         
      }
   }

   return HOOK_RESULT_OK;
}

HOOK_HANDLER(hook_digest_header)
{
    FILE *infile, *qfile, *outfile;
    char tbuf[BIG_BUF], tbuf2[BIG_BUF], tbuf3[BIG_BUF];

    if (!LMAPI->get_var("digest-header-file")) return HOOK_RESULT_OK;

    LMAPI->listdir_file(tbuf,LMAPI->get_string("list"),
      LMAPI->get_string("digest-header-file"));

    LMAPI->buffer_printf(tbuf3, sizeof(tbuf3) - 1, "%s.digestheader-expand",
      LMAPI->get_string("queuefile"));

    LMAPI->liscript_parse_file(tbuf,tbuf3);

    if ((infile = LMAPI->open_file(tbuf3,"r")) == NULL) {
        return HOOK_RESULT_OK;
    }

    LMAPI->buffer_printf(tbuf2, sizeof(tbuf2) - 1, "%s.digestsend", LMAPI->get_string("queuefile"));

    if ((qfile = LMAPI->open_file(tbuf2,"r")) == NULL) {
        LMAPI->close_file(infile);
        return HOOK_RESULT_OK;
    }

    LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "%s.header", LMAPI->get_string("queuefile"));   

    if ((outfile = LMAPI->open_file(tbuf,"a")) == NULL) {
        LMAPI->close_file(infile);
        LMAPI->close_file(qfile);
        LMAPI->unlink_file(tbuf3);
        return HOOK_RESULT_OK;
    }

    while(LMAPI->read_file(tbuf, sizeof(tbuf), qfile)) {
       LMAPI->write_file(outfile,"%s",tbuf);
       if (tbuf[0] == '\n') break;
    }

    while(LMAPI->read_file(tbuf, sizeof(tbuf), infile)) {
        LMAPI->write_file(outfile,"%s",tbuf);
    }

    LMAPI->write_file(outfile,"------------------------------------\n");

    while(LMAPI->read_file(tbuf, sizeof(tbuf), qfile)) {
       LMAPI->write_file(outfile,"%s",tbuf);
    }

    LMAPI->close_file(infile);
    LMAPI->close_file(outfile);
    LMAPI->close_file(qfile);
    LMAPI->unlink_file(tbuf3);

    LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "%s.header", LMAPI->get_string("queuefile"));
    (void)LMAPI->unlink_file(tbuf2); /* Unnecessary? -- JT */
    LMAPI->replace_file(tbuf,tbuf2);

    return HOOK_RESULT_OK;
}


HOOK_HANDLER(hook_digest_footer)
{
    FILE *infile, *outfile;
    char tbuf[BIG_BUF], tbuf2[BIG_BUF];

    if (!LMAPI->get_var("digest-footer-file")) return HOOK_RESULT_OK;

    LMAPI->listdir_file(tbuf,LMAPI->get_string("list"),
       LMAPI->get_string("digest-footer-file"));

    LMAPI->buffer_printf(tbuf2, sizeof(tbuf2) - 1, "%s.digestfooter-expand",
       LMAPI->get_string("queuefile"));   

    LMAPI->liscript_parse_file(tbuf,tbuf2);

    LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "%s.digestsend",
       LMAPI->get_string("queuefile"));   

    if ((infile = LMAPI->open_file(tbuf2,"r")) == NULL) return HOOK_RESULT_OK;

    if ((outfile = LMAPI->open_file(tbuf,"a")) == NULL) {
        LMAPI->close_file(infile);
        LMAPI->unlink_file(tbuf2);
        return HOOK_RESULT_OK;
    }

    while(LMAPI->read_file(tbuf, sizeof(tbuf), infile)) {
        LMAPI->write_file(outfile,"%s",tbuf);
    }

    LMAPI->close_file(infile);
    LMAPI->close_file(outfile);
    LMAPI->unlink_file(tbuf2);

    return HOOK_RESULT_OK;
}

MODE_HANDLER(mode_digest_send)
{
    char buf[BIG_BUF], dname[BIG_BUF];
    char tbuf[SMALL_BUF];
    int status;

    if(!(status = LMAPI->walk_lists(&dname[0])))
        return MODE_ERR;

    LMAPI->log_printf(5,"Processing digests for all lists...\n");

    while(status) {
        if(dname[0] != '.') {
            struct stat fst;          
            int issue;

            if(!LMAPI->set_context_list(dname)) {
               status = LMAPI->next_lists(&dname[0]);
               continue;
            }
            if (LMAPI->get_bool("no-digest")) {
                status = LMAPI->next_lists(&dname[0]);
                continue;
            }

            issue = digest_get_issue(dname);

            LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "digest/digest.%d.work", issue);

            LMAPI->listdir_file(buf,dname,tbuf);
            if(!stat(buf, &fst)) {
               int maxsize, maxtime, sendmode, dosend;
               const char *modevar;

               sendmode = 0;  dosend = 0;

               modevar = LMAPI->get_var("digest-send-mode");

               if (!strcasecmp(modevar, "size")) {
                   sendmode = 1;
               } else if (!strcasecmp(modevar,"time")) {
                   sendmode = 2;
               } else if (!strcasecmp(modevar,"procdigest")) {
                   sendmode = 0;
               }

               maxsize = LMAPI->get_number("digest-max-size");
               maxtime = LMAPI->get_seconds("digest-max-time");

               if ((sendmode == 1) && !maxsize) sendmode = 0;
               if ((sendmode == 2) && !maxtime) sendmode = 0;

               switch (sendmode) {
                   case 0: {
                       dosend = 1;
                       break;
                   }

                   case 1: {
                       /* Don't send nightly digest unless larger than maxsize */
                       if (fst.st_size >= maxsize)
                           dosend = 1;
                       break;                  
                   }

                   case 2: {
                       time_t now, lasttime;

                       lasttime = digest_get_lasttime(dname);
                       time(&now);

                       if (now >= (lasttime + maxtime))
                           dosend = 1;

                       break;
                   }
               }
               if (dosend) digest_send(dname);
            }
        }
        status = LMAPI->next_lists(&dname[0]);
    }
   return MODE_OK;
}

CMDARG_HANDLER(cmdarg_procdigest)
{
   LMAPI->set_var("fakequeue","yes",VAR_GLOBAL);
   LMAPI->set_var("mode","digest", VAR_GLOBAL);
   return CMDARG_OK;
}

void digest_switch_context(void)
{
   LMAPI->log_printf(15, "Switching context in module Digest\n");
}

int digest_upgradelist(int prev, int cur)
{
   LMAPI->log_printf(10, "Upgrading lists in module Digest\n");
   return 1;
}

int digest_upgrade(int prev, int cur)
{
   LMAPI->log_printf(10, "Upgrading module Digest\n");
   return 1;
}

void digest_init(void)
{
   LMAPI->log_printf(10, "Initializing module Digest\n");
}

void digest_unload(void)
{
   LMAPI->log_printf(10, "Unloading module Digest\n");
}

void digest_load(struct LPMAPI *api)
{
   LMAPI = api;
   LMAPI->log_printf(10, "Loading module Digest\n");

   LMAPI->add_module("Digest", "Handles digest volumes and issues for lists in an RFC1153-compliant manner.  Version 3.1");

   LMAPI->add_hook("DIGEST", 100, hook_digest_footer);
   LMAPI->add_hook("DIGEST", 100, hook_digest_header);
   LMAPI->add_hook("SEND", 80, hook_presend_digest_fork);
   LMAPI->add_hook("TOLIST", 50, hook_tolist_digest);
   LMAPI->add_hook("AFTER", 1000, hook_after_digest_updatecheck);

   LMAPI->add_hook("SETFLAG", 50, hook_setflag_digest);
   LMAPI->add_hook("UNSETFLAG", 50, hook_setflag_digest);

   LMAPI->add_flag("DIGEST2", "User wants to receive digested version of list AND normal posts.  This flag should be set INSTEAD of DIGEST, not in addition to.",0);
   LMAPI->add_flag("DIGEST", "User wants to receive digested version of list.", 0);

   LMAPI->add_mode("digest", mode_digest_send);

   LMAPI->add_file("digest-header", "digest-header-file",
      "File to prepend to digest when sending it out.");
   LMAPI->add_file("digest-footer", "digest-footer-file",
      "File to append to digest when sending it out.");

   LMAPI->add_file("digest-administrivia", "digest-administrivia-file",
      "File to put in RFC1153 'Administrivia' section of digest.");

   LMAPI->add_cmdarg("-procdigest", 0, NULL, cmdarg_procdigest);

   LMAPI->add_command("predigest", "Retrieves the current digest issue in whatever state it is currently in.",
                      "predigest [<list>]", NULL, NULL,
                      CMD_BODY | CMD_HEADER, cmd_predigest); 

   /* Register variable */
   LMAPI->register_var("no-digest", "no", "Digest",
                       "Should digesting be disabled for this list.",
                       "no-digest = yes", VAR_BOOL, VAR_ALL);
   LMAPI->register_var("digest-altertoc", "no", "Digest",
                       "Should this list use an alternate form for the digest Table of Contents.",
                       "digest-altertoc = false", VAR_BOOL, VAR_ALL);
#ifndef WIN32
   LMAPI->register_var("digest-no-fork", "no", "Digest",
                       "Should digesting be done by forking a seperate process.",
                       "digest-no-fork = true", VAR_BOOL, VAR_ALL);
#endif
   LMAPI->register_var("digest-no-unmime", "no", "Digest",
                       "Should posts in the digest not be unmimed.",
                       "digest-no-unmime = off", VAR_BOOL, VAR_ALL);
   LMAPI->register_var("digest-alter-datestamp", "no", "Digest",
                       "Should digests use a different datestamp format.",
                       "digest-alter-datestamp = on", VAR_BOOL, VAR_ALL);
   LMAPI->register_var("digest-no-toc", "no", "Digest",
                       "Should digests exclude the Table of Contents entirely.",
                       "digest-no-toc = TRUE", VAR_BOOL, VAR_ALL);
   LMAPI->register_var("digest-strip-tags", "no", "Digest",
                       "Should subject lines of the messages in the digest have the list subject-tag stripped.",
                       "digest-strip-tags = on", VAR_BOOL, VAR_ALL);
   LMAPI->register_var("digest-administrivia-file", "digest/administrivia",
                       "Digest",
                       "File on disk used to store digest administrative information.",
                       "digest-administrivia-file = digest/administrivia",
                       VAR_STRING, VAR_ALL);
   LMAPI->register_var("digest-transient-administrivia", "no", "Digest",
                       "Should the digest administrivia file be removed after the digest is next sent.",
                       "digest-transient-administrivia = true", VAR_BOOL,
                       VAR_ALL);
   LMAPI->register_var("digest-from", NULL, "Digest",
                       "Email address used as the From: header when the digest is distributed.",
                       "digest-from = listname@host.dom", VAR_STRING, VAR_ALL);
   LMAPI->register_var("digest-to", NULL, "Digest",
                       "Email addres used as the To: header when the digest is distributed.",
                       "digest-to = listname@host.dom", VAR_STRING, VAR_ALL);
   LMAPI->register_var("digest-transient", "yes", "Digest",
                       "Are digests removed completely after they are sent.",
                       "digest-transient = off", VAR_BOOL, VAR_ALL);
   LMAPI->register_var("digest-name", NULL, "Digest",
                       "If digests are kept, what do we use as the name template for the stored copy of the digest.",
                       "digest-name = digests/%l/V%V.I%i", VAR_STRING, VAR_ALL);
   LMAPI->register_var("digest-max-size", "0", "Digest",
                       "Maximum size a digest can reach before being automatically sent.",
                       "digest-max-size = 40000", VAR_INT, VAR_ALL);
   LMAPI->register_alias("digest-file-size", "digest-max-size");
   LMAPI->register_var("digest-max-time", "0s", "Digest",
                       "Maximum age of a digest before it is sent automatically.",
                       "digest-max-time = 24h", VAR_DURATION, VAR_ALL);
   LMAPI->register_var("digest-header-file", "text/digest-header.txt", "Digest",
                       "Filename for a header file automatically included with every digest",
                       "digest-header-file = text/digest-header.txt",
                       VAR_STRING, VAR_ALL);
   LMAPI->register_var("digest-footer-file", "text/digest-footer.txt", "Digest",
                       "Filename for a footer file automatically included with every digest",
                       "digest-footer-file = text/digest-footer.txt",
                       VAR_STRING, VAR_ALL);
   LMAPI->register_var("digest-send-mode", "procdigest:|procdigest|time|size|",  "Digest",
                       "Mode used when sending digests daily via a timed job (usually around midnight of the host machine's time).  'procdigest' means that when that happens, the digest will be sent regardless of your time and size settings (which are still honored for normal posts).  Time and size are self-explanatory; time means that it will only send if there's been more than digest-max-time elapsed, while size will only send if digest-max-size has been exceeded.  digest-max-size and digest-max-time DO still apply when individual posts come across the list, even if procdigest is set; having digest-max-size set to 50000 and this variable to procdigest would mean that the digest would be sent when it exceeded 50k, or during the midnight automated run (perhaps the day's digest only reached 20k; it would still be sent).",
                       "digest-send-mode = time", VAR_CHOICE, VAR_ALL);
}
