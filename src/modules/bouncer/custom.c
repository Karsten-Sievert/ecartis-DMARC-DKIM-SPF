#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "bouncer.h"

void parse_qmail_bounce(FILE *infile, FILE *outfile, const char *outfilename, 
                        int *errors)
{
   char buf[BIG_BUF];
   char *mptr;
   int done;

   done = 0;

   while(LMAPI->read_file(buf, sizeof(buf), infile) && !done) {

      if (buf[strlen(buf) - 1] == '\n')
          buf[strlen(buf) - 1] = 0;

      mptr = &(buf[0]);

      if ((mptr[0] == '<') && (mptr[strlen(mptr) - 1] == ':') &&
          (mptr[strlen(mptr) - 2] == '>'))
      {
             char useraddr[SMALL_BUF], *tptr, *tptr2;
             char errorstr[SMALL_BUF], valstr[4];
             int status;
         
             memset(useraddr, 0, sizeof(useraddr));
             tptr = &useraddr[0]; tptr2 = mptr + 1;

             while(*tptr2 != '>') *tptr++ = *tptr2++;

             LMAPI->read_file(errorstr, sizeof(errorstr), infile);
             tptr = strrchr(errorstr,'('); 

             if (tptr) {
                *(tptr - 1) = 0;
                tptr++;

                memset(valstr, 0, sizeof(valstr));
                tptr2 = &valstr[0];
                while(*tptr && *tptr != ')') {
					if (isdigit((int)(*tptr))) *tptr2++ = *tptr;
					tptr++;
                }         
                status = atoi(valstr);
             } else {
                status = 550;
                LMAPI->buffer_printf(errorstr, sizeof(errorstr) - 1, "[unable to parse error]");
             }
             (*errors)++;
             handle_error(status,useraddr,errorstr,outfilename);
             LMAPI->write_file(outfile, "User: %s\n      (%d) %s\n\n",
                           useraddr, status, errorstr);
      } 
      else if (strncmp(mptr,"--- Below",9) == 0) done = 1;
   }
}

void parse_postfix_bounce(FILE *infile, FILE *outfile, const char *outfilename, 
                          int *errors)
{
   char buf[BIG_BUF];
   char *mptr;
   int maindone;

   maindone = 0;

   while (LMAPI->read_file(buf, sizeof(buf), infile) && !maindone) {
      if (buf[strlen(buf) - 1] == '\n')
          buf[strlen(buf) - 1] = 0;

      mptr = &(buf[0]);

      if ((mptr[0] == '<') && (strstr(mptr,">:"))) {
		  char useraddr[SMALL_BUF], *tptr, *tptr2, *eptr;
		  char errorstr[SMALL_BUF];
		  int done;

		  memset(useraddr, 0, sizeof(useraddr));
		  tptr = &useraddr[0]; tptr2 = mptr + 1; 

		  eptr = strchr(mptr, ':');
		  while (eptr && (*(eptr - 1) != '>')) eptr = strchr(eptr+1,':');

		  if (eptr) {
			  eptr++;

			  while(*eptr ? isspace((int)*eptr) : 0) eptr++;
			  LMAPI->buffer_printf(errorstr, sizeof(errorstr) - 1, "[No numeric] %s",
					  *eptr ? eptr : "[No error]");
		  } else {
			  LMAPI->buffer_printf(errorstr, sizeof(errorstr) - 1, "[No numeric] [No error]");
		  }

		  done = 0;

		  while (!done) {
			  while(*tptr2 ? *tptr2 != '>' : 0) *tptr++ = *tptr2++;
			  (*errors)++;
			  handle_error(550,useraddr,errorstr,outfilename);
			  LMAPI->write_file(outfile, "User: %s:\n      (%d) %s\n\n",
					  useraddr, 550, errorstr);
			  memset(useraddr, 0, sizeof(useraddr));
			  tptr = &useraddr[0];
			  tptr2++;

			  if (*tptr2 == ':') done = 1; 
			  if (!done) {
				  while(*tptr2 && *tptr2 != '<') tptr2++;
				  if (!*tptr2)
					  done = 1;
				  else
					  tptr2++;
			  }
		  }
      } 
      else if (strncmp(mptr,"Received:",9) == 0) maindone = 1;
   }
}

void parse_exim_bounce(FILE *infile, FILE *outfile, const char *outfilename,
                       int *errors)
{
   int maindone;
   char useraddy[SMALL_BUF], errorstr[SMALL_BUF], buf[BIG_BUF];

   maindone = 0;

   while (LMAPI->read_file(buf, sizeof(buf), infile) && !maindone) {
      if (buf[strlen(buf) - 1] == '\n')
          buf[strlen(buf) - 1] = 0;
 
      if ((strncmp(buf,"  ",2) == 0) && strchr(buf,'@') && strchr(buf,':')) {
         char *mptr;

         mptr = &(buf[0]);
         while (*mptr ? isspace((int)(*mptr)) : 0) mptr++;

         LMAPI->buffer_printf(useraddy, sizeof(useraddy) - 1, "%s", mptr);
         mptr = strrchr(useraddy,':');
         if (*mptr) *mptr = 0;

         LMAPI->read_file(buf, sizeof(buf), infile);

         if (buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = 0;

         mptr = &(buf[0]);
         while (*mptr ? isspace((int)(*mptr)) : 0) mptr++;

         LMAPI->buffer_printf(errorstr, sizeof(errorstr) - 1, "[No numeric] %s", mptr);

         handle_error(550,useraddy,errorstr,outfilename);
         LMAPI->write_file(outfile, "User: %s:\n      (%d) %s\n\n",
                           useraddy, 550, errorstr);

         (*errors)++;
      }
      else if (strncmp(buf,"-----",5) == 0) maindone = 1;
   }   
}

void parse_msn_bounce(FILE *infile, FILE *outfile,
			      const char *outfilename, int *errors)
{
   int done = 0;
   char useraddy[SMALL_BUF], errorstr[SMALL_BUF], buf[BIG_BUF];

   while (!done && LMAPI->read_file(buf, sizeof(buf), infile)) {
      if (buf[strlen(buf) - 1] == '\n')
	 buf[strlen(buf) - 1] = 0;
      if (buf[0] == 0) continue;
      if (strchr(buf, '@') == NULL || strchr(buf, ':') != NULL ||
	  buf[0] == '-')
	 break; /* does not look like proper email address */
      LMAPI->buffer_printf(useraddy, sizeof(useraddy) - 1, "%s", buf);

      if (!LMAPI->read_file(buf, sizeof(buf), infile)) break;
      if (buf[strlen(buf) - 1] == '\n')
	 buf[strlen(buf) - 1] = 0;
      if (strlen(buf) == 0)
	 break; /* no errorstr? */
      LMAPI->buffer_printf(errorstr, sizeof(errorstr) - 1, "[No numeric] %s", buf);

      handle_error(550,useraddy,errorstr,outfilename);
      LMAPI->write_file(outfile, "User: %s:\n      (%d) %s\n\n",
			useraddy, 550, errorstr);

      (*errors)++;

      /* skip possible additional errorstr lines */
      while (LMAPI->read_file(buf, sizeof(buf), infile)) {
	 if (buf[0] == 0 || buf[0] == 13 || buf[0] == 10)
	    break;
	 if (buf[0] == '-') {
	    done = 1;
	    break;
	 }
      }
   }
}

/* sendmail with something like this (process transcript part):

   ----- The following addresses had permanent fatal errors -----
<email1@example.com>
<email2@example.com>

   ----- Transcript of session follows -----
procmail: Unknown user "email1"
550 <email1@example.com>... User unknown
procmail: Unknown user "email2"
550 <email2@example.com>... User unknown

   ----- Message header follows -----

 */
void parse_sendmail_bounce(FILE *infile, FILE *outfile,
			   const char *outfilename, int *errors)
{
   int error;
   char useraddy[SMALL_BUF], errorstr[SMALL_BUF], buf[BIG_BUF], *start, *end;

   while (LMAPI->read_file(buf, sizeof(buf), infile)) {
      if (buf[strlen(buf) - 1] == '\n')
	 buf[strlen(buf) - 1] = 0;
      if (buf[0] == 0 || strstr(buf, "---") != NULL) break;
      error = atoi(buf);
      if (error < 1) continue;

      start = strchr(buf, '<');
      end = strstr(buf, ">... ");
      if (start == NULL || end == NULL || end < start) continue;

      start++;
      *end = 0;
      end += 5;

      LMAPI->buffer_printf(useraddy, sizeof(useraddy) - 1, "%s", start);
      LMAPI->buffer_printf(errorstr, sizeof(errorstr) - 1, "%s", end);

      handle_error(error,useraddy,errorstr,outfilename);
      LMAPI->write_file(outfile, "User: %s:\n      (%d) %s\n\n",
			useraddy, error, errorstr);

      (*errors)++;
   }
}

/* Lotus with something like this:

------- Failure Reasons  --------

User  not listed in public Name & Address Book
email@example.com


------- Returned Message --------

 */
void parse_lotus_bounce(FILE *infile, FILE *outfile,
			const char *outfilename, int *errors)
{
   char useraddy[SMALL_BUF], errorstr[SMALL_BUF], buf[BIG_BUF];

   while (LMAPI->read_file(buf, sizeof(buf), infile)) {
      if (buf[strlen(buf) - 1] == '\n')
	 buf[strlen(buf) - 1] = 0;
      if (buf[0] == 0) continue;
      if (strstr(buf, "---") != NULL) break;
      LMAPI->buffer_printf(errorstr, sizeof(errorstr) - 1, "[No numeric] %s", buf);

      if (!LMAPI->read_file(buf, sizeof(buf), infile)) break;
      if (buf[strlen(buf) - 1] == '\n')
	 buf[strlen(buf) - 1] = 0;
      if (strchr(buf, '@') == NULL || strchr(buf, ':') != NULL ||
	  strstr(buf, "---") != NULL) break;
      LMAPI->buffer_printf(useraddy, sizeof(useraddy) - 1, "%s", buf);

      handle_error(550,useraddy,errorstr,outfilename);
      LMAPI->write_file(outfile, "User: %s:\n      (%d) %s\n\n",
			useraddy, 550, errorstr);

      (*errors)++;
   }
}

/* yahoo.com with something like this:

Message from  yahoo.com.
Unable to deliver message to the following address(es).

<email@yahoo.com>:
here in samColo
Sorry, your message to email@yahoo.com cannot be delivered.  This account is overquota.
Sorry, your message to email@yahoo.com cannot be delivered.  This account is overquota.

--- Original message follows.

 */
void parse_yahoo_bounce(FILE *infile, FILE *outfile,
			const char *outfilename, int *errors)
{
   char useraddy[SMALL_BUF], errorstr[SMALL_BUF], buf[BIG_BUF], *start, *end;

   if (!LMAPI->read_file(buf, sizeof(buf), infile) ||
       strncmp(buf, "Unable to deliver message", 25) != 0)
      return;

   while (LMAPI->read_file(buf, sizeof(buf), infile)) {
      if (buf[strlen(buf) - 1] == '\n')
	 buf[strlen(buf) - 1] = 0;

      if (strstr(buf, "---") != NULL) break;
      if (buf[0] == 0) continue;

      start = strchr(buf, '<');
      end = strstr(buf, ">:");
      if (start == NULL || end == NULL || end < start) continue;
      start++;
      *end = 0;
      LMAPI->buffer_printf(useraddy, sizeof(useraddy) - 1, "%s", start);

      /* get the last text line as the errorstr */
      errorstr[0] = 0;
      for (;;) {
	 if (!LMAPI->read_file(buf, sizeof(buf), infile))
	    break;
	 if (buf[strlen(buf) - 1] == '\n')
	    buf[strlen(buf) - 1] = 0;
	 if (buf[0] == 0 || strncmp(buf, "---", 3) == 0) break;
	 LMAPI->buffer_printf(errorstr, sizeof(errorstr) - 1, "[No numeric] %s", buf);
      }

      handle_error(550,useraddy,errorstr,outfilename);
      LMAPI->write_file(outfile, "User: %s:\n      (%d) %s\n\n",
			useraddy, 550, errorstr);

      (*errors)++;
   }
}

/* MMS SMTP Relay with something like this:

The MMS SMTP Relay is returning your message because:

Unable to deliver to receipient on remote mail host:
<foo@bar.com> - 550 invalid recipient

*/
void parse_mms_relay_bounce(FILE *infile, FILE *outfile,
			const char *outfilename, int *errors)
{
   char useraddy[SMALL_BUF], errorstr[SMALL_BUF], buf[BIG_BUF], *start, *end;

   if (!LMAPI->read_file(buf, sizeof(buf), infile))
      return;

   if (!LMAPI->read_file(buf, sizeof(buf), infile) ||
       (strncmp(buf,"Unable to deliver to receipient on remote mail host:", 52) != 0)) {
      return;
   }

   if (!LMAPI->read_file(buf, sizeof(buf), infile))
      return;

   start = &buf[0];

   if (*start++ != '<')
      return;

   end = strchr(start,'>');

   if (end == NULL)
      return;

   *end++ = 0;

   LMAPI->buffer_printf(useraddy, sizeof(useraddy) - 1, "%s", start);

   start = end + 2;

   LMAPI->buffer_printf(errorstr, sizeof(errorstr) - 1, "%s", start);

   handle_error(550,useraddy,errorstr,outfilename);
   LMAPI->write_file(outfile, "User: %s:\n      (%d) %s\n\n",
		useraddy, 550, errorstr);

   (*errors)++;

}

int parse_xmail_bounce(FILE *infile, FILE *outfile, const char *outfilename, char *bstr, int *errors)
{
   int ssize, errcode;
   char *pbase, *pend, *errstr;
   char const *prcpt = "Rcpt=[", *perr = "Error=[";
   char useraddy[SMALL_BUF], errorstr[SMALL_BUF];

   if (!(pbase = strstr(bstr, prcpt)))
   return 0;
   pbase += strlen(prcpt);
   if (!(pend = strchr(pbase, ']')))
   return 0;
   ssize = (int) (pend - pbase) < sizeof(useraddy) ? (int) (pend - pbase): sizeof(useraddy) - 1;
   strncpy(useraddy, pbase, ssize);
   useraddy[ssize] = '\0';

   if (!(pbase = strstr(bstr, perr)))
   return 0;
   pbase += strlen(perr);
   if (!(pend = strrchr(pbase, ']')))
   return 0;
   ssize = (int) (pend - pbase) < sizeof(errorstr) ? (int) (pend - pbase): sizeof(errorstr) - 1;
   strncpy(errorstr, pbase, ssize);
   errorstr[ssize] = '\0';

   if (!isdigit(errorstr[0])) {
	handle_error(550, useraddy, errorstr, outfilename);
	LMAPI->write_file(outfile, "User: %s:\n      (%d) %s\n\n", useraddy, 550, errorstr);

   } else {
	errcode = atoi(errorstr);
	for (errstr = errorstr; *errstr && isdigit(*errstr); errstr++);
	if (*errstr) ++errstr;
	handle_error(errcode, useraddy, errstr, outfilename);
	LMAPI->write_file(outfile, "User: %s:\n      (%d) %s\n\n", useraddy, errcode, errstr);
   }

   (*errors)++;
   return 1;

}
