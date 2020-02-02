#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "fileapi.h"
#include "variables.h"
#include "list.h"
#include "user.h"
#include "unmime.h"
#include "core.h"
#include "cookie.h"
#include "forms.h"
#include "hooks.h"
#include "modes.h"
#include "smtp.h"
#include "mystring.h"
#include "moderate.h"

extern FILE *setup_queuefile(void);
extern int anti_loop(void);

int appcounter;

COOKIE_HANDLER(expire_modpost)
{
    if(cookietype != 'M') {
        log_printf(0, "expire_modpost: Called with wrong cookie type '%c'!",
                   cookietype);
        return COOKIE_HANDLE_FAIL;
    }

    if(exists_file(cookiedata)) {
        unlink_file(cookiedata);
    }
    return COOKIE_HANDLE_OK;
}


void make_moderated_post(const char *reason)
{
    const char *moderator;
    const char *queuefile;
    char toutbuf[BIG_BUF], readbuf[BIG_BUF], tfilebuf[SMALL_BUF], tfilebuf2[SMALL_BUF];
    FILE *newfile, *oldfile;
    const char *tptr;
    int founduser;
    char cookie[BIG_BUF], cookiefile[SMALL_BUF];
    struct list_user user;
    char *listdir;

    founduser = user_find_list(get_string("list"),get_string("realsender"),&user);

    queuefile = get_string("queuefile");

    tptr = strrchr(queuefile,'/');

#ifdef WIN32
    if (!tptr) {
       tptr = strrchr(queuefile,'\\');
    }
#endif

    listdir = list_directory(get_string("list"));

    if (!tptr) tptr = queuefile; else tptr++;

    buffer_printf(tfilebuf, sizeof(tfilebuf) - 1, "%s/modposts/%s", listdir, tptr);

    mkdirs(tfilebuf);

    if ((newfile = open_file(tfilebuf,"w")) == NULL) {
        filesys_error(&tfilebuf[0]);
        free(listdir);
        return;
    }

    if ((oldfile = open_file(queuefile,"r")) == NULL) {
        filesys_error(queuefile);
        close_file(newfile);
        unlink_file(tfilebuf);
        free(listdir);
        return;
    }

    while(read_file(readbuf, sizeof(readbuf), oldfile)) {
        write_file(newfile,"%s",readbuf);
    }

    close_file(newfile);
    rewind_file(oldfile);

    buffer_printf(tfilebuf2, sizeof(tfilebuf2) - 1, "%s.modpost", queuefile);

    if ((newfile = open_file(tfilebuf2,"w")) == NULL) {
        filesys_error(&tfilebuf2[0]);
        close_file(newfile);
        unlink_file(tfilebuf);
        free(listdir);
        return;
    }

    buffer_printf(cookiefile, sizeof(cookiefile) - 1, "%s/cookies", listdir);
    set_var("cookie-for",get_string("list"),VAR_TEMP);

    free(listdir);

    if (!request_cookie(cookiefile,&cookie[0], 'M', tfilebuf)) {
       filesys_error(cookiefile);
       close_file(newfile);
       unlink_file(tfilebuf);
       return;
    }
    clean_var("cookie-for", VAR_TEMP);

    write_file(newfile,"This message was received for a list you are a moderator on, and\n");
    write_file(newfile,"was marked for moderation due to the following reason:\n");
    write_file(newfile,"%s\n", reason);
    write_file(newfile,"\nTo approve this message and have it go out on the list, forward this to\n");
    write_file(newfile,"%s\n\n",get_string("approved-address"));
    write_file(newfile,"If you wish to decline the post, change the 'apppost' below to 'delpost'.\n");
    write_file(newfile,"If you wish to edit the post, change it to 'modpost' and edit the message\n");
    write_file(newfile,"as needed - not all mail programs will work with modpost.\n\n", get_string("approved-address"));

    write_file(newfile,"DO NOT DELETE THE FOLLOWING LINE.  %s needs it.\n",
               SERVICE_NAME_MC);
    write_file(newfile,"// apppost %s\n\n", cookie);

    while(read_file(readbuf, sizeof(readbuf), oldfile)) {
        write_file(newfile,"%s",readbuf);
    }

    write_file(newfile, "// eompost %s\n", cookie);
    close_file(newfile);
    close_file(oldfile);

    if (replace_file(&tfilebuf2[0],queuefile)) {
        buffer_printf(readbuf, sizeof(readbuf) - 1, "%s -> %s", tfilebuf2, queuefile);
        filesys_error(&readbuf[0]);
        return;
    }

    set_var("form-reply-to",get_string("fromaddress"),VAR_TEMP);
    moderator = get_var("moderator");
    if (!moderator) moderator = get_var("list-owner");

    if (get_bool("moderate-verbose-subject")) {
       buffer_printf(toutbuf, sizeof(toutbuf) - 1, "%s: %s post needs approval",
           get_var("list"), get_var("realsender"));
    }
    else {
       buffer_printf(toutbuf, sizeof(toutbuf) - 1, "Message submitted to '%s'", get_string("list"));
    }
    set_var("task-form-subject",&toutbuf[0],VAR_TEMP);
    set_var("task-no-footer","yes",VAR_TEMP);
    send_textfile(moderator,queuefile);
    clean_var("task-no-footer", VAR_TEMP);

    if (get_bool("moderate-quiet")) return;

    if (founduser ? 
        (user_hasflag(&user,"ACKPOST") || get_bool("moderate-force-notify")) : 
        get_bool("moderate-notify-nonsub")) {
       const char *subj;
       set_var("results-subject-override","Post sent to moderator.",VAR_TEMP);
      
       subj = get_var("message-subject");
       if (subj && *subj) { 
           buffer_printf(toutbuf, sizeof(toutbuf) - 1, "Post to list %s\nSubject: %s", get_string("list"),
              get_string("message-subject"));
       } else {
           buffer_printf(toutbuf, sizeof(toutbuf) - 1, "Post to list %s", get_string("list"));
       }
       set_var("cur-parse-line",&toutbuf[0],VAR_GLOBAL);
       spit_status("Post submitted to moderator for reason: %s", reason);

       if (get_bool("moderate-include-queue")) {
          result_printf("\n\n--- Message which triggered moderation ----\n\n");
       
          result_append(tfilebuf);
       }
    }
}

/* How to handle sending a pre-approved message to a list */
MODE_HANDLER(mode_approved)
{
    FILE *queuefile;
    char buffer[BIG_BUF];
    const char *from;
    struct list_user user;
    int permission = 0;

    /* Generic 'mode' queuefile setup for input. */
    queuefile = setup_queuefile();

    /* Something went wrong. */
    if(!queuefile) {
        /* Do we stop or requeue? */
        if (get_bool("address-failure")) 
           return MODE_END;
        else
           return MODE_ERR;
    }

    /* Fairly generic anti-loop checking. */
    if(!get_var("resent-from")) {
       if(anti_loop()) {
           close_file(queuefile);
           return MODE_OK;
       }
    }

    /* We're done playing with the queuefile. */
    close_file(queuefile);

    /* First, we do a really low-cost check.  list-owner and moderator,
     * in cases where they aren't a ecartis address, are always allowed 
     * to post. */
    from = get_string("fromaddress");
    if((strcmp(from, get_string("moderator")) == 0) ||
       (strcmp(from, get_string("list-owner")) == 0))
        permission = 1;

    /* That check probably didn't pass on any install more recent
     * than Listar 0.98 in origin, so we'll check the user's flags now. */
    if(!permission) {
        if(user_find_list(get_string("list"), from, &user)) {
            permission = user_hasflag(&user, "ADMIN")  ||
                         user_hasflag(&user, "MODERATOR");

        }
    }

    /* If permission was still denied, then we end here. */
    if(!permission) {
        set_var("task-form-subject", "Moderation attempt denied.", VAR_TEMP);
        if(!task_heading(from))
            return MODE_ERR;
        smtp_body_text("You are not a valid moderator for list '");
        smtp_body_text(get_string("list"));
        smtp_body_line("'.");
        task_ending();
        log_printf(0, "User %s tried to post to approval address for %s\n",
                   from, get_string("list"));
        return MODE_OK;
    }

    /* Reset our counter to 0. */
    appcounter = 0;

    /* Unmime the file; this allows messages to be forwarded as MIME
     * digests and moderate multiple messages at once. */
    buffer_printf(buffer, sizeof(buffer) - 1, "%s.unmime", get_string("queuefile"));
    set_var("unmime-moderate-mode","yes",VAR_GLOBAL);
    unmime_file(get_string("queuefile"),buffer);
    clean_var("unmime-moderate-mode",VAR_GLOBAL);

    /* In unmime-moderate-mode, we will ONLY be unmimed if we're
     * a multipart message.  If we aren't unmimed, we handle it as
     * a single message. */
    if (!get_bool("just-unmimed")) {

       /* Turn us back into normal text. */
       buffer_printf(buffer, sizeof(buffer) - 1, "%s.unquote", get_string("queuefile"));
       unquote_file(get_string("queuefile"),buffer);

       if (get_bool("just-unquoted")) {
          if (replace_file(buffer,get_string("queuefile"))) {
              char tempbuf[BIG_BUF];

              buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", buffer,
                 get_string("queuefile"));
              filesys_error(&tempbuf[0]);
              (void)unlink_file(buffer);
          }
       }

       /* ...and do the moderation. */
       do_moderate(get_string("queuefile"));
    } 
    else {
       /* We were unmimed. */
       
       buffer_printf(buffer, sizeof(buffer) - 1, "%s.unmime", get_string("queuefile"));
       unlink_file(buffer);
    }

    return MODE_OK;
}

void do_moderate(const char *infilename)
{
    FILE *queuefile, *outfile;
    char buffer[BIG_BUF];
    char readbuf[BIG_BUF];
    const char *from;
    char fromaddy[BIG_BUF];
    int intoreal, inbody;
    int gotcookie;
    int done;
    int res;
    char *cookie, *appptr;
    char corecookie[SMALL_BUF];
    char cookiedata[BIG_BUF]; /* changed from 256 (SMALL_BUF) to BIG_BUF due to verify_cookie) */
    char oldqueuefile[BIG_BUF];

    gotcookie = 0;
    done = 0;

    from = get_string("fromaddress");

    if ((queuefile = open_file(infilename,"r+")) == NULL) {
       return;
    }

    appcounter++;

    buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d", get_string("queuefile"), appcounter);
    if((outfile = open_file(buffer, "w")) == NULL) {
        close_file(queuefile);
        set_var("cur-parse-line", "Post of message to moderated list.", VAR_GLOBAL);
        filesys_error(buffer);
        spit_status("Filesystem error encountered.");
        return;
    }
    intoreal = 0;
    inbody = 0;

    set_var("moderated-approved-by", from, VAR_GLOBAL);
    while(read_file(readbuf, sizeof(readbuf), queuefile) && !done) {
        if((strncasecmp("// eompost", readbuf, 10) == 0) && intoreal) {
           char *tempcookie;
           tempcookie = &readbuf[11];
           tempcookie[strlen(tempcookie) - 1] = 0;
           if (strcmp(tempcookie,corecookie) == 0) {
              log_printf(9,"Found eompost, ending moderated post\n");
              done = 1;
           }           
        }
        if(intoreal && !done) {
            char *tbufp = readbuf;
            /*
             * This hack should no longer be needed, but I'm leaving it,
             * just in case.
             */
            if(!inbody && !strncasecmp("=46rom:", readbuf, 6)) {
                readbuf[2] = 'F';
                tbufp = &readbuf[2];
            }
            write_file(outfile, "%s", tbufp);
        }
        if(readbuf[0] == '\n') inbody = 1;
        if((strncasecmp("// modpost", readbuf, 10) == 0)
          && (inbody && !intoreal)) {
           cookie = &readbuf[11];
           readbuf[strlen(readbuf) - 1] = 0;
           log_printf(9,"Checking moderator cookie: %s\n", cookie);
           if (match_cookie(cookie,get_string("list"))) {
              char cookiefile[SMALL_BUF];
              char *listdir;

              listdir = list_directory(get_string("list"));

              buffer_printf(cookiefile, sizeof(cookiefile) - 1, "%s/cookies", listdir);

              free(listdir);

              if (verify_cookie(cookiefile, cookie, 'M', &cookiedata[0])) {
                 gotcookie = 1;
                 buffer_printf(corecookie, sizeof(corecookie) - 1, "%s", cookie);
                 del_cookie(cookiefile,cookie);
              }
           }
        } else
        if((appptr = strstr(readbuf, "// apppost"))
          && (inbody && !intoreal)) {
           int pos;

           pos = appptr - &readbuf[0];

           cookie = &readbuf[pos + 11];
           readbuf[strlen(readbuf) - 1] = 0;
           log_printf(9,"Checking moderator cookie: %s\n", cookie);
           if (match_cookie(cookie,get_string("list"))) {
              char cookiefile[SMALL_BUF];
              char *listdir;

              listdir = list_directory(get_string("list"));

              buffer_printf(cookiefile, sizeof(cookiefile) - 1, "%s/cookies", listdir);

              free(listdir);

              if (verify_cookie(cookiefile, cookie, 'M', &cookiedata[0])) {
                 FILE *infile2;
                 char subbuffer[BIG_BUF];

                 gotcookie = 1;

                 if ((infile2 = open_file(&cookiedata[0],"r")) == NULL) {
                    set_var("task-form-subject", "No stored message.", VAR_TEMP);
                    close_file(queuefile);
                    close_file(outfile);
                    buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d", get_string("queuefile"), appcounter);
                    unlink_file(buffer);
                    if(!task_heading(from)) {
                      buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d",
                         get_string("queuefile"),appcounter);
                       unlink_file(buffer);

                       return;
                    }
                    smtp_body_text("Could not find the file associated with the post you approved on ");
                    smtp_body_text(get_string("list"));
                    smtp_body_line(".");
                    task_ending();
                    log_printf(0, "User %s posted to %s approval address; no stored file found.\n",
                           from, get_string("list"));

                    buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d",
                      get_string("queuefile"),appcounter);
                    unlink_file(buffer);

                    return;
                 }

                 while(read_file(subbuffer, sizeof(subbuffer), infile2)) {
                    write_file(outfile,"%s",subbuffer);
                 }

                 close_file(infile2);
                 del_cookie(cookiefile,cookie);
                 log_printf(9,"Moderated post approved.\n");
                 done = 1;
              } else {
                 rewind_file(queuefile);
                 if(!task_heading(from)) {
                    buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d",
                      get_string("queuefile"),appcounter);
                    unlink_file(buffer);

                    return;
                 }
                 smtp_body_text("Cannot find a cookie for post on ");
                 smtp_body_text(get_string("list"));
                 smtp_body_line(".");
                 smtp_body_line("");
                 smtp_body_line("It may have been already approved, or it may have expired.");
                 if (get_bool("verbose-moderate-fail")) {
                    smtp_body_line("");
                    smtp_body_line("--- Message in question ---");
                    rewind_file(queuefile);
                    while(read_file(buffer, sizeof(buffer), queuefile)) {
                       smtp_body_text(buffer);
                    }
                    smtp_body_line("");
                 }
                 close_file(queuefile);
                 task_ending();
                 buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d",
                     get_string("queuefile"),appcounter);
                 unlink_file(buffer);
                 return;              
              }
           } else {
              if(!task_heading(from)) {
                buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d",
                      get_string("queuefile"),appcounter);
                unlink_file(buffer);
                return;
              }
              smtp_body_text("Cookie does not match for ");
              smtp_body_text(get_string("list"));
              smtp_body_line(".");

              if (get_bool("verbose-moderate-fail")) {
                 smtp_body_line("");
                 smtp_body_line("--- Message in question ---");
                 rewind_file(queuefile);
                 while(read_file(buffer, sizeof(buffer), queuefile)) {
                    smtp_body_text(buffer);
                 }
                 smtp_body_line("");
              } 
              task_ending();
              buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d",
                 get_string("queuefile"),appcounter);
              unlink_file(buffer);
              return;              
           }
        } else
        if((strncasecmp("// delpost", readbuf, 10) == 0)
          && (inbody && !intoreal)) {
           cookie = &readbuf[11];
           readbuf[strlen(readbuf) - 1] = 0;
           log_printf(9,"Checking moderator cookie: %s\n", cookie);
           if (match_cookie(cookie,get_string("list"))) {
              char cookiefile[SMALL_BUF];
              char *listdir;

              listdir = list_directory(get_string("list"));

              buffer_printf(cookiefile, sizeof(cookiefile) - 1, "%s/cookies", listdir);

              free(listdir);

              if (verify_cookie(cookiefile,cookie, 'M', &cookiedata[0])) {
                 gotcookie = 1;

                 if (!exists_file(&cookiedata[0])) {
                    set_var("task-form-subject", "No stored message.", VAR_TEMP);
                    close_file(queuefile);
                    close_file(outfile);
                    buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d", get_string("queuefile"), appcounter);
                    unlink_file(buffer);
                    if(!task_heading(from)) {
                       buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d",
                          get_string("queuefile"),appcounter);
                       unlink_file(buffer);
                       return;
                    }
                    smtp_body_text("Could not find the file associated with the post you declined for ");
                    smtp_body_text(get_string("list"));
                    smtp_body_line(".");
                    task_ending();
                    log_printf(0, "User %s posted to %s approval address; no stored file found.\n",
                           from, get_string("list"));
                    buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d",
                      get_string("queuefile"),appcounter);
                    unlink_file(buffer);
                    return;
                 }

                 buffer_printf(buffer, BIG_BUF - 1, "Moderated post to '%s' declined.", get_string("list"));
                 log_printf(9, "%s\n", buffer);
                 set_var("results-subject-override", buffer, VAR_TEMP);
                 result_printf(">> %s\n", readbuf);
                 result_printf("%s\n", buffer);
                 del_cookie(cookiefile, cookie);
                 done = 1;

                 smtp_body_line("");
                 smtp_body_line("--- Message in question ---");
                 rewind_file(queuefile);
                 while(read_file(buffer, sizeof(buffer), queuefile)) {
                    smtp_body_text(buffer);
                 }
                 smtp_body_line("");

                 close_file(queuefile);
                 close_file(outfile);
                 buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d", get_string("queuefile"), appcounter);
                 unlink_file(buffer);
                 return;
              } else {
                 if(!task_heading(from)) {
                    buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d",
                      get_string("queuefile"),appcounter);
                    unlink_file(buffer);
                    return;
                 }
                 smtp_body_text("No cookie found for post to list ");
                 smtp_body_text(get_string("list"));
                 smtp_body_line(".");
                 smtp_body_line("");
                 smtp_body_line("Has this post already been approved or declined?");
                 if (get_bool("verbose-moderate-fail")) {
                    smtp_body_line("");
                    smtp_body_line("--- Message in question ---");
                    rewind_file(queuefile);
                    while(read_file(buffer, sizeof(buffer), queuefile)) {
                       smtp_body_text(buffer);
                    }
                    smtp_body_line("");
                 }
                 close_file(queuefile); 
                 task_ending();
                 buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d",
                    get_string("queuefile"),appcounter);
                 unlink_file(buffer);
                 return;
              }
           } else {
              if(!task_heading(from)) {
                 buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d",
                      get_string("queuefile"),appcounter);
                 unlink_file(buffer);
                 return;
              }
              smtp_body_text("Cookie does not match for list ");
              smtp_body_text(get_string("list"));
              smtp_body_line(".");
              if (get_bool("verbose-moderate-fail")) {
                 smtp_body_line("");
                 smtp_body_line("--- Message in question ---");
                 rewind_file(queuefile);
                 while(read_file(buffer, sizeof(buffer), queuefile)) {
                    smtp_body_text(buffer);
                 }
                 smtp_body_line("");
              } 
              close_file(queuefile);
              task_ending();
              buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d",
                   get_string("queuefile"),appcounter);
              unlink_file(buffer);
              return;              
           }
        } else
        if(((strncasecmp(">From ", readbuf, 6) == 0) ||
            (strncasecmp("> From ", readbuf, 7) == 0) ||
            (strncasecmp(" From ", readbuf, 6) == 0) ||
            /* This hack should be unneeded... but... better safe */
            (strncasecmp("=46rom ", readbuf,7) == 0) ||
            (strncasecmp("From ", readbuf, 5) == 0)) &&
            (inbody && !intoreal)) {
            if (gotcookie) {
               char *frombuf;
               frombuf = &readbuf[0];
               /* This hack should be unneeded... but... better safe */
               /* Hack for if we have stupid Forte doing quoted printable */
               if(readbuf[0] == '=') {
                   readbuf[2] = 'F';
                   frombuf= &readbuf[2];
               }
               if(readbuf[0] == '>' || readbuf[0] == ' ') {
                   if(readbuf[1] == 'F') frombuf = &readbuf[1];
                   if(readbuf[2] == 'F') frombuf = &readbuf[2];
               }
               sscanf(frombuf, "From %s", fromaddy);
               set_var("fromaddress", fromaddy, VAR_GLOBAL);
               write_file(outfile,"%s",frombuf);
               intoreal = 1;
               inbody = 0;
            } else {
               set_var("task-form-subject", "No moderator cookie.", VAR_TEMP);
               if(!task_heading(from)) {
                   buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d", get_string("queuefile"), appcounter);
                   unlink_file(buffer);
                   return;
               }
               smtp_body_text("Could not find a valid cookie for this post for list ");
               smtp_body_text(get_string("list"));
               smtp_body_line(".");
               smtp_body_line("");
               smtp_body_line("Another moderator may have approved this already.");
               log_printf(0, "User %s posted to %s approval address; no valid cookie found.\n",
                   from, get_string("list"));
               if (get_bool("verbose-moderate-fail")) {
                  smtp_body_line("");
                  smtp_body_line("--- Message in question ---");
                  rewind_file(queuefile);
                  while(read_file(buffer, sizeof(buffer), queuefile)) {
                     smtp_body_text(buffer);
                  }
                  smtp_body_line("");
               }
               task_ending();
               close_file(queuefile);
               close_file(outfile);
               buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d", get_string("queuefile"), appcounter);
               unlink_file(buffer);
               return;
            }
        }
    }
    close_file(outfile);
    close_file(queuefile);

    if (!gotcookie) {
        make_moderated_post("Message sent to approval address with no cookie");
        buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d", get_string("queuefile"), appcounter);
        unlink_file(buffer);
        return;
    }

    buffer_printf(oldqueuefile, sizeof(oldqueuefile) - 1, "%s", get_string("queuefile"));
    buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d", oldqueuefile, appcounter);

    /* We reset our queuefile temporarily. */
    set_var("queuefile",buffer,VAR_GLOBAL);

    /* ...along with the sender address... */
    /*
    if ((queuefile = open_file(buffer,"r")) != NULL) {
       read_file(buffer, sizeof(buffer), queuefile);
       if (sscanf(readbuf, "From %s", fromaddy))
          set_var("fromaddress", fromaddy, VAR_GLOBAL);
       close_file(queuefile);
    }
    */
    queuefile = setup_queuefile();
    close_file(queuefile);

    res = do_hooks("SEND");
    if(res == HOOK_RESULT_OK) {
        res = do_hooks("FINAL");
    }

    /* Remove our file! */
    /* buffer_printf(buffer, sizeof(buffer) - 1, "%s.appout.%d", get_string("queuefile"), appcounter);*/
    unlink_file(buffer);

    /* ...and set the queuefile back. */
    set_var("queuefile",oldqueuefile,VAR_GLOBAL);
    queuefile = setup_queuefile();
    close_file(queuefile);
}
