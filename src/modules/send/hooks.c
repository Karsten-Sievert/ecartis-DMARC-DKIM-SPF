#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include "compat.h"

#ifndef WIN32
#include <unistd.h>
#endif

#include "send-mod.h"

int strip_reply(const char *orig, char *dest, int length)
{
    char *tptr=NULL, *tptr2, temp;
    char tempbuf[BIG_BUF];
    int done, changed;

    LMAPI->buffer_printf(dest, length - 1, "%s", orig);       

    LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s", orig);
    tptr2 = &tempbuf[0];

    done = 0; changed = 0;

    while(isspace((int)*tptr2)) tptr2++;

    tptr = tptr2;

    while(!done && *tptr2) {
       temp = tolower(*tptr2);

       if (strncasecmp(tptr2,"re:",3) == 0) {
          tptr2 += 3;
          changed = 1;
       } else if (isspace((int)*tptr2)) {
          tptr2++;
       } else {
          done = 1;
       }
       tptr = tptr2;
    }

    while(tptr && *tptr && (isspace((int)(*tptr)))) tptr++;

    if (*tptr) 
       LMAPI->buffer_printf(dest,length - 1, "%s", tptr);
    else
       LMAPI->buffer_printf(dest,length - 1, "(No subject)");

    return changed;
}

CMDARG_HANDLER(cmdarg_private_reply)
{
   LMAPI->set_var("ignore-reply-to","yes",VAR_GLOBAL);
   return CMDARG_OK;
}

HOOK_HANDLER(hook_send_tag)
{
    FILE *outfile;
    FILE *infile;
    char filename[BIG_BUF];
    char buf[BIG_BUF];
    const char *subjecttag;
    int donefile, gotsubj;

    subjecttag = LMAPI->get_var("subject-tag");
    LMAPI->clean_var("message-subject", VAR_GLOBAL);

    donefile = 0; gotsubj = 0;

    if ((infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) == NULL) {
        LMAPI->filesys_error(LMAPI->get_string("queuefile"));
        return HOOK_RESULT_FAIL;
    }

    LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.tmp", LMAPI->get_string("queuefile"));
    if ((outfile = LMAPI->open_file(filename,"w")) == NULL) {
        LMAPI->close_file(infile);
        LMAPI->filesys_error(&filename[0]);
        return HOOK_RESULT_FAIL;
    }

    LMAPI->log_printf(15,"About to read file...(%d)\n", sizeof(buf));

    while(LMAPI->read_file(buf, sizeof(buf), infile)) {
		LMAPI->log_printf(15,"megadebug: %s", buf);
        if (!strncasecmp("subject:",&buf[0],8) && !donefile) {
            char subject[BIG_BUF],tempsub[BIG_BUF];

          /* We read the following lines in case of a subject string
           * longer than one line...
           */
          stringcpy(tempsub, &buf[9]);
          while (!donefile && LMAPI->read_file(buf, sizeof(buf), infile))
          {
              if ((buf[0] == '\t') || (buf[0] == ' '))
                    stringcat(tempsub, buf) ;
              else
                  donefile = 1 ;
          }
            if(tempsub[0] != '\0') {
                if(LMAPI->get_bool("humanize-quotedprintable")) {
                    LMAPI->unquote_string(tempsub, subject, sizeof(subject) - 1);
                    if (!strchr(subject,'\n')) stringcat(subject, "\n");
                } else {
                    LMAPI->buffer_printf(subject, sizeof(subject) - 1, "%s", tempsub);
                }
            } else {
                LMAPI->buffer_printf(subject, sizeof(subject) - 1, "%s", "(no subject)\n");
            }
            LMAPI->set_var("message-subject",subject,VAR_GLOBAL);
            gotsubj = 1;
            if ( subjecttag && subjecttag[0] ) {
                 char newbuf[BIG_BUF], newbuf2[BIG_BUF] ;
                 char tbuf[SMALL_BUF];

                 LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "%s ", subjecttag);

                 LMAPI->strcasereplace(newbuf, sizeof(newbuf) - 1, subject, &tbuf[0], "");
                 if (!strip_reply(newbuf, newbuf2, sizeof(newbuf2) - 1)) {
                  LMAPI->buffer_printf(subject, sizeof(subject) - 1, "%s %s", subjecttag, newbuf);  
                 } else {
                    if (LMAPI->get_bool("tag-to-front")) {
                     LMAPI->buffer_printf(subject, sizeof(subject) - 1, "%s Re: %s", subjecttag, newbuf2);
                    } else {
                     LMAPI->buffer_printf(subject, sizeof(subject) - 1, "Re: %s %s", subjecttag, newbuf2);
                    }
                 }
            }
          LMAPI->requote_string( subject, tempsub, sizeof(tempsub) - 1) ;
          LMAPI->write_file( outfile, "Subject: %s", tempsub );

            donefile = 1;
        } else {
            if ((buf[0] == '\n') && !donefile) {
                donefile = 1;
                if (!gotsubj) {
                   if (subjecttag && subjecttag[0]) {
                    char tmp[BIG_BUF] ;
                    LMAPI->requote_string( subjecttag, tmp, sizeof(tmp) - 1);
                      LMAPI->buffer_printf(buf, sizeof(buf) - 1, "Subject: %s (no subject)\n\n", tmp);
                   } else {
                      LMAPI->buffer_printf(buf, sizeof(buf) - 1, "Subject: (no subject)\n\n");
                   }
                }
            }
        }
        LMAPI->write_file(outfile,"%s",buf);
    }

    LMAPI->close_file(outfile);
    LMAPI->close_file(infile);

    if (LMAPI->replace_file(filename,LMAPI->get_string("queuefile"))) {
        char tempbuf[BIG_BUF];

        LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", filename, LMAPI->get_string("queuefile"));
        LMAPI->filesys_error(&tempbuf[0]);
        (void)LMAPI->unlink_file(filename);
        return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;   
}


HOOK_HANDLER(hook_send_header)
{
    FILE *infile, *qfile, *outfile;
    char tbuf[BIG_BUF], tbuf2[BIG_BUF];

    if (!LMAPI->get_var("header-file")) return HOOK_RESULT_OK;

    LMAPI->listdir_file(tbuf, LMAPI->get_string("list"),
                        LMAPI->get_string("header-file"));

    LMAPI->buffer_printf(tbuf2, sizeof(tbuf2) - 1, "%s.expand",
        LMAPI->get_string("queuefile"));

    LMAPI->liscript_parse_file(tbuf,tbuf2);

    if ((infile = LMAPI->open_file(tbuf2,"r")) == NULL) return HOOK_RESULT_OK;

    if ((qfile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) == NULL) {
        LMAPI->close_file(infile);
        return HOOK_RESULT_OK;
    }

    LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "%s.header", LMAPI->get_string("queuefile"));   

    if ((outfile = LMAPI->open_file(tbuf,"a")) == NULL) {
        LMAPI->close_file(infile);
        LMAPI->close_file(qfile);
        return HOOK_RESULT_OK;
    }

    while(LMAPI->read_file(tbuf, sizeof(tbuf), qfile)) {
       LMAPI->write_file(outfile,"%s",tbuf);
       if (tbuf[0] == '\n') break;
    }

    while(LMAPI->read_file(tbuf, sizeof(tbuf), infile)) {
        LMAPI->write_file(outfile,"%s",tbuf);
    }

    while(LMAPI->read_file(tbuf, sizeof(tbuf), qfile)) {
       LMAPI->write_file(outfile,"%s",tbuf);
    }

    LMAPI->close_file(infile);
    LMAPI->close_file(outfile);
    LMAPI->close_file(qfile);

    LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "%s.header", LMAPI->get_string("queuefile"));
    (void)LMAPI->unlink_file(LMAPI->get_string("queuefile"));
    LMAPI->replace_file(tbuf,LMAPI->get_string("queuefile"));
    LMAPI->unlink_file(tbuf2);

    return HOOK_RESULT_OK;
}

HOOK_HANDLER(hook_send_footer)
{
    char tempfilename[BIG_BUF]; /* Changed from SMALL_BUF to BIG_BUF due to listdir_file */

    if (!LMAPI->get_var("footer-file")) return HOOK_RESULT_OK;

    LMAPI->listdir_file(tempfilename,LMAPI->get_string("list"),
        LMAPI->get_string("footer-file"));

    LMAPI->log_printf(9,"Expanding footer: %s\n", tempfilename);

    LMAPI->expand_append(LMAPI->get_string("queuefile"),
                         tempfilename);

    return HOOK_RESULT_OK;
}

HOOK_HANDLER(hook_send_replyto)
{
    FILE *outfile;
    FILE *infile;
    char filename[BIG_BUF];
    char buf[BIG_BUF];
    int donefile;

    if (!LMAPI->get_var("reply-to") &&
        !LMAPI->get_bool("reply-to-sender")) return HOOK_RESULT_OK;

    if(LMAPI->get_bool("ignore-reply-to")) return HOOK_RESULT_OK;

    donefile = 0;

    if ((infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) == NULL) {
        LMAPI->filesys_error(LMAPI->get_string("queuefile"));
        return HOOK_RESULT_FAIL;
    }

    LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.tmp", LMAPI->get_string("queuefile"));
    if ((outfile = LMAPI->open_file(filename,"w")) == NULL) {
        LMAPI->close_file(infile);
        LMAPI->filesys_error(&filename[0]);
        return HOOK_RESULT_FAIL;
    }

    while(LMAPI->read_file(buf, sizeof(buf), infile)) {
        if ((buf[0] == '\n') && !donefile) {
            donefile = 1;
            if (!LMAPI->get_bool("reply-to-sender")) {
               LMAPI->write_file(outfile,"Reply-to: %s\n",
                  LMAPI->get_string("reply-to"));
            } else {
               LMAPI->write_file(outfile,"Reply-to: %s\n",
                  LMAPI->get_string("fromaddress"));
            }
        }
        if ((strncasecmp("reply-to:",&buf[0],9) && !donefile) || (donefile)) {
            LMAPI->write_file(outfile,"%s",buf);
        }
    }

    LMAPI->close_file(outfile);
    LMAPI->close_file(infile);

    if (LMAPI->replace_file(filename,LMAPI->get_string("queuefile"))) {
        char tempbuf[BIG_BUF];

        LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", filename, LMAPI->get_string("queuefile"));
        LMAPI->filesys_error(&tempbuf[0]);
        (void)LMAPI->unlink_file(filename);
        return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;   
}

HOOK_HANDLER(hook_send_approvedby)
{
    FILE *outfile;
    FILE *infile;
    char filename[BIG_BUF];
    char buf[BIG_BUF];
    int donefile;

    if (!LMAPI->get_var("moderated-approved-by"))
       return HOOK_RESULT_OK;

    donefile = 0;

    if ((infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) == NULL) {
        LMAPI->filesys_error(LMAPI->get_string("queuefile"));
        return HOOK_RESULT_FAIL;
    }

    LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.tmp", LMAPI->get_string("queuefile"));
    if ((outfile = LMAPI->open_file(filename,"w")) == NULL) {
        LMAPI->close_file(infile);
        LMAPI->filesys_error(&filename[0]);
        return HOOK_RESULT_FAIL;
    }

    while(LMAPI->read_file(buf, sizeof(buf), infile)) {
        if ((buf[0] == '\n') && !donefile) {
            donefile = 1;
               LMAPI->write_file(outfile,"X-Approved-By: %s\n\n",
                 LMAPI->get_string("moderated-approved-by"));
        } else {
               LMAPI->write_file(outfile,"%s",buf);
        }
    }

    LMAPI->close_file(outfile);
    LMAPI->close_file(infile);

    LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.tmp", LMAPI->get_string("queuefile"));
    if (LMAPI->replace_file(filename,LMAPI->get_string("queuefile"))) {
        char tempbuf[BIG_BUF];

        LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", filename, LMAPI->get_string("queuefile"));
        LMAPI->filesys_error(&tempbuf[0]);
        (void)LMAPI->unlink_file(filename);
        return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;   
}


HOOK_HANDLER(hook_presend_check_subject)
{
    FILE *infile;

    if (LMAPI->get_var("moderated-approved-by")) return HOOK_RESULT_OK;

    if (!LMAPI->get_bool("subject-required")) return HOOK_RESULT_OK;

    if ((infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) != NULL) {
       int inbody, gotsubj;
       char buffer[BIG_BUF];

       inbody = gotsubj = 0;

       while(LMAPI->read_file(buffer, sizeof(buffer), infile) && !inbody && !gotsubj) {
          if (strncasecmp(buffer,"subject: ",9) == 0) {
             char *tptr;

             tptr = &buffer[9];

             while(*tptr && (*tptr != '\n')) {
                if (!isspace((int)(*tptr))) gotsubj = 1;
                tptr++;
             }
          } else if(buffer[0] == '\n') {
             inbody = 1;
          }
       }
       LMAPI->close_file(infile);

       if (!gotsubj) {
          LMAPI->log_printf(1,"Message with no subject received on '%s'.\n",
            LMAPI->get_string("list"));
          LMAPI->make_moderated_post("This list requires that you provide a subject for any messages posted to it.");
          return HOOK_RESULT_STOP;
       }

       return HOOK_RESULT_OK;

    } else {
       LMAPI->filesys_error(LMAPI->get_string("queuefile"));
       return HOOK_RESULT_STOP;
    }  
}

HOOK_HANDLER(hook_presend_blacklist)
{
    if (LMAPI->get_var("moderated-approved-by")) return HOOK_RESULT_OK;

    if (LMAPI->blacklisted(LMAPI->get_string("realsender"))) {
        const char *blacklistfile;
        int usedefault;

        usedefault = 1;

        LMAPI->set_var("task-form-subject","You have been blacklisted.",VAR_TEMP);

        blacklistfile = LMAPI->get_var("blacklist-reject-file");
        if (blacklistfile) {
            FILE *checkme;
            char buffer[BIG_BUF];

            LMAPI->listdir_file(buffer, LMAPI->get_string("list"),
                                blacklistfile);
            if ((checkme = LMAPI->open_file(buffer,"r")) != NULL) {
                usedefault = 0;
                LMAPI->close_file(checkme);
                LMAPI->send_textfile_expand(LMAPI->get_string("realsender"),&buffer[0]);
            }
        }

        if (usedefault) {
            if(!LMAPI->task_heading(LMAPI->get_string("realsender")))
                return HOOK_RESULT_FAIL;
            LMAPI->smtp_body_line("The address you are trying to post from has been banned from this");
            LMAPI->smtp_body_line("mailing list.");
            LMAPI->task_ending();
        }

        LMAPI->log_printf(0, "Blacklisted user %s attempted to post to %s\n",
                   LMAPI->get_string("realsender"), LMAPI->get_string("list"));

        return HOOK_RESULT_STOP;
    }

    return HOOK_RESULT_OK;
}

HOOK_HANDLER(hook_send_version)
{
    FILE *outfile;
    FILE *infile;
    char filename[BIG_BUF];
    char buf[BIG_BUF];
    char datestr[80];
    time_t now;
    int donefile;

    donefile = 0;

    if ((infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) == NULL) {
        LMAPI->filesys_error(LMAPI->get_string("queuefile"));
        return HOOK_RESULT_FAIL;
    }

    LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.tag", LMAPI->get_string("queuefile"));
    if ((outfile = LMAPI->open_file(filename,"w")) == NULL) {
        LMAPI->close_file(infile);
        LMAPI->filesys_error(&filename[0]);
        return HOOK_RESULT_FAIL;
    }

    LMAPI->read_file(buf, sizeof(buf), infile);
    LMAPI->write_file(outfile,"%s",buf);

    now = time(NULL);
    LMAPI->get_date(datestr, sizeof(datestr), now);

    LMAPI->write_file(outfile,"Received: with %s (v%s; list %s); %s\n",
                      SERVICE_NAME_UC, VER_PRODUCTVERSION_STR,
                      LMAPI->get_string("list"), datestr);

    while(LMAPI->read_file(buf, sizeof(buf), infile)) {
        if ((buf[0] == '\n') && !donefile) {
            donefile = 1;
            LMAPI->write_file(outfile,"X-%s-version: %s v%s\n",
                              SERVICE_NAME_LC, SERVICE_NAME_MC,
                              VER_PRODUCTVERSION_STR);
        }
        LMAPI->write_file(outfile,"%s",buf);
    }

    LMAPI->close_file(outfile);
    LMAPI->close_file(infile);

    if (LMAPI->replace_file(filename,LMAPI->get_string("queuefile"))) {
        char tempbuf[BIG_BUF];

        LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", filename, LMAPI->get_string("queuefile"));
        LMAPI->filesys_error(&tempbuf[0]);
        (void)LMAPI->unlink_file(filename);
        return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;   
}

HOOK_HANDLER(hook_send_returnpath)
{
    FILE *outfile;
    FILE *infile;
    char filename[BIG_BUF];
    char buf[BIG_BUF];
    int donefile;

    donefile = 0;

    if ((infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) == NULL) {
        LMAPI->filesys_error(LMAPI->get_string("queuefile"));
        return HOOK_RESULT_FAIL;
    }

    LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.tag", LMAPI->get_string("queuefile"));
    if ((outfile = LMAPI->open_file(filename,"w")) == NULL) {
        LMAPI->close_file(infile);
        LMAPI->filesys_error(&filename[0]);
        return HOOK_RESULT_FAIL;
    }

    LMAPI->read_file(buf, sizeof(buf), infile);
    LMAPI->write_file(outfile,"%s",buf);

    while(LMAPI->read_file(buf, sizeof(buf), infile)) {
        if ((buf[0] == '\n') && !donefile) {
            donefile = 1;
            if (LMAPI->get_var("send-as")) {
                LMAPI->write_file(outfile,"Sender: %s\n", LMAPI->get_string("send-as"));
                LMAPI->write_file(outfile,"Errors-to: %s\n", LMAPI->get_string("send-as"));
            } else {
                LMAPI->write_file(outfile,"Sender: %s\n", LMAPI->get_string("list-owner"));
                LMAPI->write_file(outfile,"Errors-to: %s\n", LMAPI->get_string("list-owner"));
            }
            if (!LMAPI->get_var("force-from-address")) {
               if (LMAPI->get_var("fromaddress"))
                  LMAPI->write_file(outfile,"X-original-sender: %s\n", LMAPI->get_string("fromaddress"));
            }
        }
        if (donefile || (strncasecmp(buf,"Sender:",7) &&
					strncasecmp(buf,"Errors-to:",10)))
           LMAPI->write_file(outfile,"%s",buf);
    }

    LMAPI->close_file(outfile);
    LMAPI->close_file(infile);

    if (LMAPI->replace_file(filename,LMAPI->get_string("queuefile"))) {
        char tempbuf[BIG_BUF];

        LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", filename, LMAPI->get_string("queuefile"));
        LMAPI->filesys_error(&tempbuf[0]);
        (void)LMAPI->unlink_file(filename);
        return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;   
}

HOOK_HANDLER(hook_send_forcefrom)
{
    FILE *outfile;
    FILE *infile;
    char filename[BIG_BUF];
    char buf[BIG_BUF];
    int donefile;

    donefile = 0;

    if (!LMAPI->get_var("force-from-address")) return HOOK_RESULT_OK;

    if ((infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) == NULL) {
        LMAPI->filesys_error(LMAPI->get_string("queuefile"));
        return HOOK_RESULT_FAIL;
    }

    LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.from", LMAPI->get_string("queuefile"));
    if ((outfile = LMAPI->open_file(filename,"w")) == NULL) {
        LMAPI->close_file(infile);
        LMAPI->filesys_error(&filename[0]);
        return HOOK_RESULT_FAIL;
    }

    LMAPI->read_file(buf, sizeof(buf), infile);
    LMAPI->write_file(outfile,"%s",buf);

    while(LMAPI->read_file(buf, sizeof(buf), infile)) {
        if (buf[0] == '\n') donefile = 1;
        if (!strncmp(buf,"From:",5) && !donefile) {
           LMAPI->write_file(outfile, "From: %s\n",
               LMAPI->get_string("force-from-address"));
        } else LMAPI->write_file(outfile,"%s",buf);
    }

    LMAPI->close_file(outfile);
    LMAPI->close_file(infile);

    if (LMAPI->replace_file(filename,LMAPI->get_string("queuefile"))) {
        char tempbuf[BIG_BUF];

        LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", filename, LMAPI->get_string("queuefile"));
        LMAPI->filesys_error(&tempbuf[0]);
        (void)LMAPI->unlink_file(filename);
        return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;   
}

HOOK_HANDLER(hook_send_precedence)
{
    FILE *outfile;
    FILE *infile;
    char filename[BIG_BUF];
    char queuename[BIG_BUF];
    char buf[BIG_BUF];
    int donefile;

    donefile = 0;

    if(strcasecmp(LMAPI->get_string("mode"), "DIGEST") == 0) {
       LMAPI->buffer_printf(queuename, sizeof(queuename) - 1, "%s.digestsend", LMAPI->get_string("queuefile"));
    } else {
       LMAPI->buffer_printf(queuename, sizeof(queuename) - 1, "%s", LMAPI->get_string("queuefile"));
    }

    if ((infile = LMAPI->open_file(queuename,"r")) == NULL) {
        LMAPI->filesys_error(queuename);
        return HOOK_RESULT_FAIL;
    }

    LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.tag", queuename);
    if ((outfile = LMAPI->open_file(filename,"w")) == NULL) {
        LMAPI->close_file(infile);
        LMAPI->filesys_error(&filename[0]);
        return HOOK_RESULT_FAIL;
    }

    LMAPI->read_file(buf, sizeof(buf), infile);
    LMAPI->write_file(outfile,"%s",buf);

    while(LMAPI->read_file(buf, sizeof(buf), infile)) {
        if ((buf[0] == '\n') && !donefile) {
            donefile = 1;
            LMAPI->write_file(outfile,"Precedence: %s\n", LMAPI->get_string("precedence"));
        }
        LMAPI->write_file(outfile,"%s",buf);
    }

    LMAPI->close_file(outfile);
    LMAPI->close_file(infile);

    if (LMAPI->replace_file(filename,queuename)) {
        char tempbuf[BIG_BUF];

        LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", filename, queuename);
        LMAPI->filesys_error(&tempbuf[0]);
        (void)LMAPI->unlink_file(filename);
        return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;   
}

HOOK_HANDLER(hook_send_xlist)
{
    FILE *outfile;
    FILE *infile;
    char filename[BIG_BUF];
    char buf[BIG_BUF];
    int donefile;

    donefile = 0;

    if ((infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) == NULL) {
        LMAPI->filesys_error(LMAPI->get_string("queuefile"));
        return HOOK_RESULT_FAIL;
    }

    LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.xlist", LMAPI->get_string("queuefile"));
    if ((outfile = LMAPI->open_file(filename,"w")) == NULL) {
        LMAPI->close_file(infile);
        LMAPI->filesys_error(&filename[0]);
        return HOOK_RESULT_FAIL;
    }

    LMAPI->read_file(buf, sizeof(buf), infile);
    LMAPI->write_file(outfile,"%s",buf);

    while(LMAPI->read_file(buf, sizeof(buf), infile)) {
        if ((buf[0] == '\n') && !donefile) {
            donefile = 1;
            LMAPI->write_file(outfile,"X-list: %s\n", LMAPI->get_string("list"));
        }
        LMAPI->write_file(outfile,"%s",buf);
    }

    LMAPI->close_file(outfile);
    LMAPI->close_file(infile);

    if (LMAPI->replace_file(filename,LMAPI->get_string("queuefile"))) {
        char tempbuf[BIG_BUF];

        LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", filename, LMAPI->get_string("queuefile"));
        LMAPI->filesys_error(&tempbuf[0]);
        (void)LMAPI->unlink_file(filename);
        return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;   
}

HOOK_HANDLER(hook_presend_unmime)
{
    char filename[SMALL_BUF];
    const char *queuefile;

    if (LMAPI->get_var("moderated-approved-by")) return HOOK_RESULT_OK;

    if (!LMAPI->get_bool("humanize-mime")) return HOOK_RESULT_OK;

    LMAPI->log_printf(9,"Humanizing mime...\n");

    queuefile = LMAPI->get_string("queuefile");

    LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.unmime", queuefile);

    LMAPI->unmime_file(queuefile,filename);

    if (LMAPI->get_bool("just-unmimed")) {
       if (LMAPI->replace_file(filename,queuefile)) {
           char tempbuf[BIG_BUF];

           LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", filename, queuefile);
           LMAPI->filesys_error(&tempbuf[0]);
           (void)LMAPI->unlink_file(filename);
           return HOOK_RESULT_FAIL;
       }
    } else {
       LMAPI->unlink_file(filename);
    }

    return HOOK_RESULT_OK;
}

HOOK_HANDLER(hook_presend_unquote)
{
    char filename[SMALL_BUF];
    const char *queuefile;

    if (LMAPI->get_var("moderated-approved-by")) return HOOK_RESULT_OK;

    if (!LMAPI->get_bool("humanize-quotedprintable")) return HOOK_RESULT_OK;

    LMAPI->log_printf(9,"Humanizing quoted printable...\n");

    queuefile = LMAPI->get_string("queuefile");

    LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.unquote", queuefile);

    LMAPI->unquote_file(queuefile,filename);

    if (LMAPI->get_bool("just-unquoted")) {
       if (LMAPI->replace_file(filename,queuefile)) {
           char tempbuf[BIG_BUF];

           LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", filename, queuefile);
           LMAPI->filesys_error(&tempbuf[0]);
           (void)LMAPI->unlink_file(filename);
           return HOOK_RESULT_FAIL;
       }
    } else {
        LMAPI->unlink_file(filename);
    }
    return HOOK_RESULT_OK;
}

HOOK_HANDLER(hook_presend_check_xlist)
{
   int found, done;
   char buffer[BIG_BUF];
   FILE *infile;

   found = done = 0;

   if ((infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) == NULL)
      return HOOK_RESULT_OK;

   while (!done && LMAPI->read_file(buffer, sizeof(buffer), infile)) {
      if (buffer[0] == '\n') done = 1;
      if (strncasecmp(&buffer[0],"X-list:",7) == 0) {
         if (strcasecmp(&buffer[7],LMAPI->get_string("list")) == 0) {
            found = 1;
         }
      }
   }

   LMAPI->close_file(infile);

   if (found) {
      LMAPI->log_printf(0,"Possible looped mail detected and eaten.\n");
      return HOOK_RESULT_STOP;
   }

   return HOOK_RESULT_OK;      
}

HOOK_HANDLER(hook_presend_check_size)
{
   int fullsize, headersize, bodysize;
   struct stat fst;
   char inbody;
   FILE *infile;
   char buffer[BIG_BUF];

   /* Moderated messages are immune to check. */
   if (LMAPI->get_var("moderated-approved-by")) return HOOK_RESULT_OK;

   if (!LMAPI->get_var("body-max-size") && !LMAPI->get_var("header-max-size"))
      return HOOK_RESULT_OK;

   if (stat(LMAPI->get_string("queuefile"),&fst) == 0) {
      fullsize = fst.st_size;
   } else return HOOK_RESULT_OK; /* Though we should never be here */

   if ((infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) == NULL)
      return HOOK_RESULT_OK; /* Though we shouldn't be here, either */

   inbody = 0; headersize = 0;

   while(LMAPI->read_file(buffer, sizeof(buffer), infile) && !inbody) {
      if (buffer[0] == '\n') inbody = 1;
      if (!inbody) {
         headersize += strlen(buffer);
      }
   }

   LMAPI->close_file(infile);

   bodysize = fullsize - headersize;

   LMAPI->log_printf(5,"Checking sizes: %d (header) %d (body)\n",
         headersize, bodysize);

   if (LMAPI->get_var("header-max-size")) {
      if (headersize > LMAPI->get_number("header-max-size")) {
         LMAPI->make_moderated_post("Message exceeded allowed header size.");
         return HOOK_RESULT_STOP;
      }
   }

   if (LMAPI->get_var("body-max-size")) {
      if (bodysize > LMAPI->get_number("body-max-size")) {
         LMAPI->make_moderated_post("Message exceeded allowed body size.");
         return HOOK_RESULT_STOP;
      }
   }

   return HOOK_RESULT_OK;
}

HOOK_HANDLER(hook_send_rfc2369)
{
    FILE *outfile;
    FILE *infile;
    char filename[BIG_BUF];
    char queuename[BIG_BUF];
    char buf[BIG_BUF];
    int donefile;
    char hostname[BIG_BUF];
    char listid_hostname[BIG_BUF];

    if (!LMAPI->get_bool("rfc2369-headers")) return HOOK_RESULT_OK;

    if(strcasecmp(LMAPI->get_string("mode"), "DIGEST") == 0) {
        LMAPI->buffer_printf(queuename, sizeof(queuename) - 1, "%s.digestsend", LMAPI->get_string("queuefile"));
    } else {
        LMAPI->buffer_printf(queuename, sizeof(queuename) - 1, "%s", LMAPI->get_string("queuefile"));
    }

    donefile = 0;

    if ((infile = LMAPI->open_file(queuename,"r")) == NULL) {
        LMAPI->filesys_error(queuename);
        return HOOK_RESULT_FAIL;
    }

    LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.tmp", LMAPI->get_string("queuefile"));
    if ((outfile = LMAPI->open_file(filename,"w")) == NULL) {
        LMAPI->close_file(infile);
        LMAPI->filesys_error(&filename[0]);
        return HOOK_RESULT_FAIL;
    }

    if (LMAPI->get_var("hostname")) {
       LMAPI->buffer_printf(hostname, sizeof(hostname) - 1, "%s",
          LMAPI->get_string("hostname"));
    } else {
       LMAPI->build_hostname(&hostname[0],sizeof(hostname));
    }

    if (LMAPI->get_bool("use-rfc2919-listid-subdomain")) {
       LMAPI->buffer_printf(listid_hostname, sizeof(listid_hostname) - 1, "list-id.%s", hostname);
    }
    else {
       LMAPI->buffer_printf(listid_hostname, sizeof(listid_hostname) - 1, "%s", hostname);
    }

    while(LMAPI->read_file(buf, sizeof(buf), infile)) {
        if ((buf[0] == '\n') && !donefile) {
            donefile = 1;
	    if (LMAPI->get_var("rfc2369-list-help")) {
	        LMAPI->write_file(outfile,"List-help: <%s>\n", LMAPI->get_string("rfc2369-list-help"));
   	    } else {
                LMAPI->write_file(outfile,"List-help: <mailto:%s?Subject=help>\n", LMAPI->get_string("listserver-address"));
	    }
            if (!LMAPI->get_var("rfc2369-unsubscribe")) {
               LMAPI->write_file(outfile,"List-unsubscribe: <mailto:%s-request@%s?Subject=unsubscribe>\n",
                   LMAPI->get_string("list"), hostname);
            } else {
               LMAPI->write_file(outfile,"List-unsubscribe: <%s>\n",
                   LMAPI->get_string("rfc2369-unsubscribe"));
            }
            LMAPI->write_file(outfile,"List-software: %s version %s\n",
                              SERVICE_NAME_MC, VER_PRODUCTVERSION_STR);

            if (LMAPI->get_var("rfc2369-listname")) {
                LMAPI->write_file(outfile,"List-Id: %s <%s.%s>\n",
                  LMAPI->get_string("rfc2369-listname"),
                  LMAPI->get_string("list"),
                  listid_hostname);
                if (LMAPI->get_bool("rfc2369-legacy-listid")) {
                  LMAPI->write_file(outfile,"X-List-ID: %s <%s.%s>\n",
                     LMAPI->get_string("rfc2369-listname"),
                     LMAPI->get_string("list"),
                     hostname);
                }
            } else {
                LMAPI->write_file(outfile,"List-Id: <%s.%s>\n",
                  LMAPI->get_string("list"),
                  listid_hostname);
                if (LMAPI->get_bool("rfc2369-legacy-listid")) {
                  LMAPI->write_file(outfile,"X-List-ID: <%s.%s>\n",
                    LMAPI->get_string("list"),
                    hostname);
                }
            }

            if (!LMAPI->get_bool("rfc2369-minimal")) {
                if (!LMAPI->get_var("rfc2369-subscribe")) {
                  LMAPI->write_file(outfile,"List-subscribe: <mailto:%s-request@%s?Subject=subscribe>\n",
                   LMAPI->get_string("list"), hostname);
                } else {
                  LMAPI->write_file(outfile,"List-subscribe: <%s>\n",
                      LMAPI->get_string("rfc2369-subscribe"));
                }
                LMAPI->write_file(outfile,"List-owner: <mailto:%s>\n", LMAPI->get_string("list-owner"));
                if (LMAPI->get_var("rfc2369-post-address")) {
                    const char *postaddy;

                    postaddy = LMAPI->get_string("rfc2369-post-address");

                    if (strcasecmp(postaddy,"closed") != 0) {
                        LMAPI->write_file(outfile,"List-post: <mailto:%s>\n", LMAPI->get_string("rfc2369-post-address"));
                    } else {
                        LMAPI->write_file(outfile,"List-post: NO\n");
                    }
                } else {
                    LMAPI->write_file(outfile,"List-post: <mailto:%s@%s>\n",
                        LMAPI->get_string("list"), hostname);
                }
                if (LMAPI->get_var("rfc2369-archive-url")) {
                    /* quick hack to expand %l for list name */
                    char newbuf[BIG_BUF];
                    LMAPI->strreplace(newbuf, sizeof(newbuf) - 1,
                                      LMAPI->get_string("rfc2369-archive-url"),
                                      "%l", LMAPI->get_string("list"));
                    LMAPI->write_file(outfile,"List-archive: <%s>\n", newbuf);
                }
            }
            LMAPI->write_file(outfile, "%s", buf);
        } else {
            LMAPI->write_file(outfile, "%s", buf);
        }
    }

    LMAPI->close_file(outfile);
    LMAPI->close_file(infile);

    if (LMAPI->replace_file(filename,queuename)) {
        char tempbuf[BIG_BUF];

        LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", filename, queuename);
        LMAPI->filesys_error(&tempbuf[0]);
        (void)LMAPI->unlink_file(filename);
        return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;
}

HOOK_HANDLER(hook_send_stripheaders)
{
    FILE *outfile;
    FILE *infile;
    char filename[BIG_BUF];
    char queuename[BIG_BUF];
    char buf[BIG_BUF];
    int donefile, eatline;

    donefile = 0;

    eatline = 0;

    if (LMAPI->get_var("strip-headers") == NULL)
        return HOOK_RESULT_OK;

    if(strcasecmp(LMAPI->get_string("mode"), "DIGEST") == 0) {
       LMAPI->buffer_printf(queuename, sizeof(queuename) - 1, "%s.digestsend", LMAPI->get_string("queuefile"));
    } else {
       LMAPI->buffer_printf(queuename, sizeof(queuename) - 1, "%s", LMAPI->get_string("queuefile"));
    }

    if ((infile = LMAPI->open_file(queuename,"r")) == NULL) {
        LMAPI->filesys_error(queuename);
        return HOOK_RESULT_FAIL;
    }

    LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.strip", queuename);
    if ((outfile = LMAPI->open_file(filename,"w")) == NULL) {
        LMAPI->close_file(infile);
        LMAPI->filesys_error(&filename[0]);
        return HOOK_RESULT_FAIL;
    }

    LMAPI->read_file(buf, sizeof(buf), infile);
    LMAPI->write_file(outfile,"%s",buf);

    while(LMAPI->read_file(buf, sizeof(buf), infile)) {
       if (!donefile) {
          if (buf[0] != '\n') {
             char tempbuf[BIG_BUF], *tempptr, *tempptr2;
             int okheader;

             okheader = 1;

             if (isspace((int)buf[0]) && eatline) okheader = 0;

             if (okheader) {
                int done;
                char temp2[128];

                LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s", LMAPI->get_string("strip-headers"));

                tempptr = &tempbuf[0];
                tempptr2 = strchr(tempbuf,':');

                if (tempptr2) *tempptr2 = 0;     

                done = 0;

                while (!done && okheader) {
                  LMAPI->buffer_printf(temp2, sizeof(temp2) - 1, "%s:", tempptr);

                  if (!strncasecmp(buf,temp2,strlen(temp2)))
                     okheader = 0;

                  if (tempptr2) {
                    tempptr = tempptr2 + 1;
                    tempptr2 = strchr(tempptr,':');
                    if (tempptr2) *tempptr2 = 0;
                  } else done = 1;
                }
             }

             if (okheader) {
                LMAPI->write_file(outfile,"%s",buf);
                eatline = 0;
             } else {
                LMAPI->log_printf(5,"Stripped header line: %s",buf);
                eatline = 1;
             }             
          } else {
             donefile = 1;
             LMAPI->write_file(outfile,"\n");
          }
       } else {
          LMAPI->write_file(outfile,"%s",buf);
       }
    }

    LMAPI->close_file(outfile);
    LMAPI->close_file(infile);

    if (LMAPI->replace_file(filename,queuename)) {
        char tempbuf[BIG_BUF];

        LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", filename, queuename);
        LMAPI->filesys_error(&tempbuf[0]);
        (void)LMAPI->unlink_file(filename);
        return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;   
}

HOOK_HANDLER(hook_send_stripmdn)
{
    FILE *outfile;
    FILE *infile;
    char filename[BIG_BUF];
    const char *queuename;
    char buf[BIG_BUF];
    int donefile, eatline;

    donefile = 0;

    eatline = 0;

    if (!LMAPI->get_bool("strip-mdn"))
        return HOOK_RESULT_OK;

    queuename = LMAPI->get_string("queuefile");

    if ((infile = LMAPI->open_file(queuename,"r")) == NULL) {
        LMAPI->filesys_error(queuename);
        return HOOK_RESULT_FAIL;
    }

    LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.stripmdn", queuename);
    if ((outfile = LMAPI->open_file(filename,"w")) == NULL) {
        LMAPI->close_file(infile);
        LMAPI->filesys_error(&filename[0]);
        return HOOK_RESULT_FAIL;
    }

    while(LMAPI->read_file(buf, sizeof(buf), infile)) {
       if (!donefile) {
          if (buf[0] != '\n') {
             int okheader = 1;
             if (isspace((int)buf[0]) && eatline)
                 okheader = 0;
             if (okheader) {
                 if(strncasecmp(buf, "Disposition-Notification-To:", 28) == 0)
                     okheader = 0;
                 else if(strncasecmp(buf, "X-Confirm-Reading-To:", 21) == 0)
                     okheader = 0;
                 else if(strncasecmp(buf, "X-pmrqc:", 8) == 0)
                     okheader = 0;
                 else if(strncasecmp(buf, "Return-Receipt-To:", 18) == 0)
                     okheader = 0;
                 else if(strncasecmp(buf, "Disposition-Notification-Options:", 33) == 0)
                     okheader = 0;
              }

             if (okheader) {
                LMAPI->write_file(outfile,"%s",buf);
                eatline = 0;
             } else {
                LMAPI->log_printf(5,"Stripped MDN header line: %s",buf);
                eatline = 1;
             }             
          } else {
             donefile = 1;
             LMAPI->write_file(outfile,"\n");
          }
       } else {
          LMAPI->write_file(outfile,"%s",buf);
       }
    }

    LMAPI->close_file(outfile);
    LMAPI->close_file(infile);

    if (LMAPI->replace_file(filename,queuename)) {
        char tempbuf[BIG_BUF];

        LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", filename, queuename);
        LMAPI->filesys_error(&tempbuf[0]);
        (void)LMAPI->unlink_file(filename);
        return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;   
}

HOOK_HANDLER(hook_send_strip_rfc2369)
{
    FILE *outfile;
    FILE *infile;
    char filename[BIG_BUF];
    const char *queuename;
    char buf[BIG_BUF];
    int donefile, eatline;

    donefile = 0;

    eatline = 0;

    queuename = LMAPI->get_string("queuefile");

    if ((infile = LMAPI->open_file(queuename,"r")) == NULL) {
        LMAPI->filesys_error(queuename);
        return HOOK_RESULT_FAIL;
    }

    LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.strip2369", queuename);
    if ((outfile = LMAPI->open_file(filename,"w")) == NULL) {
        LMAPI->close_file(infile);
        LMAPI->filesys_error(&filename[0]);
        return HOOK_RESULT_FAIL;
    }

    while(LMAPI->read_file(buf, sizeof(buf), infile)) {
       if (!donefile) {
          if (buf[0] != '\n') {
             int okheader = 1;
             if (isspace((int)buf[0]) && eatline)
                 okheader = 0;
             if (okheader) {
                 if(strncasecmp(buf, "List-",5) == 0)
                     okheader = 0;
                 else if(strncasecmp(buf, "X-List-", 7) == 0)
                     okheader = 0;
              }

             if (okheader) {
                LMAPI->write_file(outfile,"%s",buf);
                eatline = 0;
             } else {
                LMAPI->log_printf(5,"Stripped RFC2369 header line: %s",buf);
                eatline = 1;
             }             
          } else {
             donefile = 1;
             LMAPI->write_file(outfile,"\n");
          }
       } else {
          LMAPI->write_file(outfile,"%s",buf);
       }
    }

    LMAPI->close_file(outfile);
    LMAPI->close_file(infile);

    if (LMAPI->replace_file(filename,queuename)) {
        char tempbuf[BIG_BUF];

        LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", filename, queuename);
        LMAPI->filesys_error(&tempbuf[0]);
        (void)LMAPI->unlink_file(filename);
        return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;   
}

