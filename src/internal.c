#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#ifndef WIN32
#include <unistd.h>
#endif /* WIN32 */

#include "core.h"
#include "cmdarg.h"
#include "modes.h"
#include "user.h"
#include "variables.h"
#include "smtp.h"
#include "command.h"
#include "hooks.h"
#include "list.h"
#include "forms.h"
#include "parse.h"
#include "fileapi.h"
#include "compat.h"
#include "cookie.h"
#include "mystring.h"
#include "unmime.h"
#include "build.h"
#include "lpm-mods.h"
#include "internal.h"
#include "upgrade.h"

extern MODE_HANDLER(mode_approved);

/* Handle the file command line argument */
CMDARG_HANDLER(cmdarg_filefrom)
{
    if(!argv[0]) {
        internal_error("Switch -f requires a filename parameter.");
        return CMDARG_ERR;
    }

    log_printf(1,"Using '%s' instead of STDIN...\n", argv[0]);

    clean_var("listserver-infile", VAR_GLOBAL);
    set_var("listserver-infile", argv[0], VAR_GLOBAL);
    return CMDARG_OK;
}

CMDARG_HANDLER(cmdarg_cheatsheet)
{
    if (!argv[0]) {
        printf("Switch -cheatsheet requires a filename parameter.");
        return CMDARG_ERR;
    }

    write_cheatsheet(argv[0],NULL);

    return CMDARG_EXIT;
}

CMDARG_HANDLER(cmdarg_upgrade)
{
    FILE *fp = open_file(get_string("version-file"), "r+");
    int prev = 0;

    set_var("mode","upgrade",VAR_GLOBAL);

    if(fp) {
        fscanf(fp, "%d", &prev);
        if(prev != CUR_BUILD_VERSION) {
            rewind_file(fp);
            truncate_file(fp, 0);
            fprintf(fp, "%d\n", CUR_BUILD_VERSION);
        }
        fclose(fp);
    } else {
        /* Make sure the directory exists if it doesn't already */
	mkdirs(get_string("version-file"));
        /* now try to open the file */
        fp = open_file(get_string("version-file"), "w");
        if(fp) {
            fprintf(fp, "%d\n", CUR_BUILD_VERSION);
            fclose(fp);
        } else {
            log_printf(0, "Unable to create version info file.\n");
            return CMDARG_EXIT;
        }
    }
    if(!upgrade_listserver(prev, CUR_BUILD_VERSION)) {
        log_printf(0, "Error while upgrading from version %d to version %d.\n",
                   prev, CUR_BUILD_VERSION);
        return CMDARG_EXIT;
    }
    return CMDARG_EXIT;
}

/* Handle the debug command line argument */
CMDARG_HANDLER(cmdarg_debug)
{
    if(!argv[0] || atoi(argv[0]) == 0) {
        internal_error("Switch -d requires a numeric param of 1 or greater.");
        return CMDARG_ERR;
    }
    set_var("debug", argv[0], VAR_TEMP);
    lock_var("debug");
    return CMDARG_OK;
}

/* Handle the optional config file argument */
CMDARG_HANDLER(cmdarg_config)
{
    char tmp[BIG_BUF];
    if (count != 1) {
        internal_error("Alternate config file must be first parameter.");
        return CMDARG_ERR;
    }

    if(!argv[0]) {
        internal_error("Switch -c requires a file argument.");
        return CMDARG_ERR;
    }

    buffer_printf(tmp, sizeof(tmp) - 1, "%s/%s", get_string("listserver-conf"), argv[0]);
    if(!exists_file(tmp)) {
        buffer_printf(tmp, sizeof(tmp) - 1, "Unable to find config file '%s/%s'",
                get_string("listserver-conf"), argv[0]);
        internal_error(tmp);
        return CMDARG_ERR;
    }

    clean_var("site-config-file", VAR_GLOBAL);
    set_var("site-config-file", tmp, VAR_GLOBAL);
    read_conf(tmp, VAR_SITE);

    /*
     * And now we need to remunge the lists-root variable just in case.
     * Redirect lists-root to be relative to listserver-data.
     * If the lists-root is already prefixed by listserver-data (as it would
     * be if we still had the default version from core.c, then we don't
     * want to mess with it.
     * It would also be prefixed if someone was overly anal, so the check
     * is still useful.
     */
    buffer_printf(tmp, sizeof(tmp) - 1, "%s", get_string("listserver-data"));
    if(strncmp(get_string("lists-root"), tmp, strlen(tmp)) != 0) {
        buffer_printf(tmp, sizeof(tmp) - 1, "%s/%s", get_string("listserver-data"),
                get_string("lists-root"));
        set_var("lists-root", tmp, VAR_SITE);
    }

    generate_queue();

    return CMDARG_OK;
}


/* Handle the -r switch (Set up things for request mode handling) */
CMDARG_HANDLER(cmdarg_request)
{
   if(!argv[0] || !list_valid(argv[0])) {
       if(!argv[0]) {
           internal_error("Switch -r requires a list as a parameter.");
       } else {
           char buf[BIG_BUF];
           buffer_printf(buf, sizeof(buf) - 1, "'%s' is invalid list for the -r switch.", argv[0]);
           internal_error(buf);
       }
       return CMDARG_ERR;
   }
   set_var("list", argv[0], VAR_GLOBAL);
   set_var("mode", "request", VAR_GLOBAL);
   return CMDARG_OK;
}

/* Handle the -s command switch (send a message to a list) */
CMDARG_HANDLER(cmdarg_resend)
{
   if(!argv[0] || !list_valid(argv[0])) {
       if(!argv[0]) {
           internal_error("Switch -s requires a list as a parameter.");
       } else {
           char buf[BIG_BUF];
           buffer_printf(buf, sizeof(buf) - 1, "'%s' is invalid list for the -s switch.", argv[0]);
           internal_error(buf);
       }
       return CMDARG_ERR;
   }
   set_var("list", argv[0], VAR_GLOBAL);
   set_var("mode", "resend", VAR_GLOBAL);
   return CMDARG_OK;
}

/* Handle the -a switch (Approved posting mode) */
CMDARG_HANDLER(cmdarg_approved)
{
   if(!argv[0] || !list_valid(argv[0])) {
       if(!argv[0]) {
           internal_error("Switch -a requires a list as a parameter.");
       } else {
           char buf[BIG_BUF];
           buffer_printf(buf, sizeof(buf) - 1, "'%s' is invalid list for the -a switch.", argv[0]);
           internal_error(buf);
       }
       return CMDARG_ERR;
   }
   set_var("list", argv[0], VAR_GLOBAL);
   set_var("mode", "approved", VAR_GLOBAL);
   return CMDARG_OK;
}

/* Parse the -admins switch */
CMDARG_HANDLER(cmdarg_admins)
{
   if(!argv[0] || !list_valid(argv[0])) {
       if(!argv[0]) {
           internal_error("Switch -admins requires a list as a parameter.");
       } else {
           char buf[BIG_BUF];
           buffer_printf(buf, sizeof(buf) - 1, "'%s' is invalid list for the -admins switch.", argv[0]);
           internal_error(buf);
       }
       return CMDARG_ERR;
   }
   set_var("list", argv[0], VAR_GLOBAL);
   set_var("mode", "admins", VAR_GLOBAL);
   return CMDARG_OK;
}

/* Parse the -moderators switch */
CMDARG_HANDLER(cmdarg_moderators)
{
   if(!argv[0] || !list_valid(argv[0])) {
       if(!argv[0]) {
           internal_error("Switch -moderators requires a list as a parameter.");
       } else {
           char buf[BIG_BUF];
           buffer_printf(buf, sizeof(buf) - 1, "'%s' is invalid list for the -moderators switch.", argv[0]);
           internal_error(buf);
       }
       return CMDARG_ERR;
   }
   set_var("list", argv[0], VAR_GLOBAL);
   set_var("mode", "moderators", VAR_GLOBAL);
   return CMDARG_OK;
}

int anti_loop(void)
{
    FILE *queuefile;
    char buffer[BIG_BUF];
    int inbody, antiloop;

    if (!exists_file(get_string("queuefile")))
        return 0;

    if ((queuefile = open_file(get_string("queuefile"),"r")) == NULL)
        return 0;

    inbody = 0;
    antiloop = 0;

    while(read_file(buffer, sizeof(buffer), queuefile) && !inbody)  {
        if(buffer[0] == '\n')
            inbody = 1;
        else {
            if (strncasecmp(buffer, "X-ecartis-antiloop", 18) == 0) {
                log_printf(0,"Message rejected: %s", buffer);
                antiloop = 1;
            }
            
            /* Legacy support for our predecessor versions */
            else if (strncasecmp(buffer,"X-listar-antiloop", 17) == 0) {
                log_printf(0,"Message rejected: %s", buffer);
                antiloop = 1;
            } 
            else if (strncasecmp(buffer,"X-sllist-antiloop", 17) == 0) {
                log_printf(0,"Message rejected: %s", buffer);
                antiloop = 1;
            }
        }
    }

    close_file(queuefile);

    return antiloop;
}

/* Utility function to open and start parsing the queuefile */
FILE *setup_queuefile(void)
{
    const char *queue;
    FILE *queuefile;
    char buffer[BIG_BUF];
    char fromaddy[BIG_BUF], fromaddy2[BIG_BUF];
    char *fromad, *fromad2;
    char backupaddy[BIG_BUF];
    int resent_from = 0;
	int header = 0;

    memset(fromaddy, 0, sizeof(fromaddy));
    memset(fromaddy2, 0, sizeof(fromaddy2));

    queue = get_var("queuefile");
    if(!queue) {
        internal_error("Queuefile undefined.");
        return NULL;
    }
    queuefile = open_file(queue, "r");
    if(!queuefile) {
        buffer_printf(buffer, sizeof(buffer) - 1, "Unable to open queuefile '%s'.", queue);
        filesys_error(buffer);
        return NULL;
    }
      
    if(!read_file(buffer, sizeof(buffer), queuefile)) {
        buffer_printf(buffer, sizeof(buffer) - 1, "Queuefile '%s' is empty.", queue);
        filesys_error(buffer);
        close_file(queuefile);
        return NULL;
    }
    if (!sscanf(buffer, "From %s", fromaddy)) {
        if (!get_bool("deny-822-from")) {
           log_printf(2,"Mangled message, no 'from' top-line.  Assuming RFC822...\n");
        } else {
           close_file(queuefile);
           internal_error("No SMTP from, and RFC822 not allowed!");
           set_var("address-failure","yes",VAR_TEMP);
           return NULL;
        }
    } else {
        fromad = strchr(fromaddy,':');
        if (fromad) fromad++; else fromad = &fromaddy[0];

        fromad2 = &fromaddy2[0];

        while (*fromad) {
           if ((*fromad != '<') && (*fromad != '>')) {
              *fromad2++ = *fromad;
           }
           fromad++;
        }
        *fromad2 = '\0';
        /* trim off any trailing whitespace */
        fromad2--;
        while(isspace((int)(*fromad2))) { *fromad2 = '\0'; fromad2--; }
    }

    buffer_printf(backupaddy, sizeof(backupaddy) - 1, "%s", fromaddy);

    if (!get_bool("deny-822-from")) 
    {
       int done;
       char *fromad3;
       char tbuffer[BIG_BUF];

       done = 0;
	   header = 1;

       log_printf(9,"Checking for RFC822 From.\n");

       while(read_file(tbuffer, sizeof(tbuffer), queuefile) && !done && header) {
           if (strncmp(tbuffer,"From:",5) == 0) {
               memcpy(&buffer[0],&tbuffer[0],sizeof(tbuffer));
               done = 1;
           }
		   if(tbuffer[0] == '\n') header = 0;
       }

       /* Rewind and eat first line again */
       rewind_file(queuefile);
       read_file(tbuffer, sizeof(tbuffer), queuefile);

       if (done) {
          log_printf(9,"Trying RFC822 From: as opposed to SMTP from.\n");

          fromad = strchr(buffer,':');
          fromad++;
          while(isspace((int)(*fromad))) fromad++;

          fromad2 = strchr(fromad,'<');
          if (fromad2) fromad = fromad2++;

          fromad2 = &fromaddy2[0];

          while (*fromad) {
             if ((*fromad != '<') && (*fromad != '>') &&
                 (*fromad != '\n') && (*fromad != '\r')) {
                *fromad2++ = *fromad;
             }
             fromad++;
          }
          *fromad2 = '\0';

          fromad3 = strchr(fromaddy2,'(');
          if (fromad3) {
             *fromad3 = 0;
             fromad2 = fromad3;
          }

          /* trim off any trailing whitespace */
          fromad2--;
          while(isspace((int)(*fromad2))) { *fromad2 = '\0'; fromad2--; }
       }       
    }

    if (!get_bool("deny-822-bounce")) 
    {
       int done;
       char *fromad3;
       char tbuffer[BIG_BUF];

	   header = 1;
       done = 0;

       log_printf(9,"Checking for RFC822 Resent-From.\n");

       rewind_file(queuefile);

       while(read_file(tbuffer, sizeof(tbuffer), queuefile) && !done && header) {
           if (strncasecmp(tbuffer,"Resent-From:",12) == 0) {
               memcpy(&buffer[0],&tbuffer[0],sizeof(tbuffer));
               done = 1;
           }
		   if(tbuffer[0] == '\n') header = 0;
       }

       /* Rewind and eat first line again */
       rewind_file(queuefile);
       read_file(tbuffer, sizeof(tbuffer), queuefile);

       if (done) {
          log_printf(9,"Using RFC822 Resent-From: as opposed to SMTP from.\n");

          fromad = strchr(buffer,':');
          fromad++;
          while(isspace((int)(*fromad))) fromad++;

          fromad2 = strchr(fromad,'<');
          if (fromad2) fromad = fromad2++;

          fromad2 = &fromaddy2[0];

          while (*fromad) {
             if ((*fromad != '<') && (*fromad != '>') &&
                 (*fromad != '\n') && (*fromad != '\r')) {
                *fromad2++ = *fromad;
             }
             fromad++;
          }
          *fromad2 = '\0';

          fromad3 = strchr(fromaddy2,'(');
          if (fromad3) {
             *fromad3 = 0;
             fromad2 = fromad3;
          }

          /* trim off any trailing whitespace */
          fromad2--;
          while(isspace((int)(*fromad2))) { *fromad2 = '\0'; fromad2--; }
          resent_from = 1;
       }       
    }

    if (!check_address(fromaddy2) && backupaddy[0]) {
       log_printf(1,"Mangled, invalid, or nonexisted return address...\n");
       buffer_printf(fromaddy2, sizeof(fromaddy2) - 1, "%s", backupaddy);
       log_printf(1,"Reverting to '%s', SMTP trace value.\n", backupaddy); 
    }

    if (!check_address(fromaddy2)) {
       close_file(queuefile);
       internal_error("Received mail without valid return address.");
       set_var("address-failure","yes",VAR_TEMP);
       return NULL;
    }
    if(resent_from)
       set_var("resent-from", fromaddy2, VAR_GLOBAL);
    set_var("realsender", fromaddy2, VAR_GLOBAL);
    set_var("fromaddress", fromaddy2, VAR_GLOBAL);
   
    return queuefile; 
}

/* Handle resending a message out to the list */
MODE_HANDLER(mode_resend)
{
    int res;
    FILE *queuefile;

    log_printf(9, "Attempting to resend to list %s.\n", get_string("list"));

    /* We only setup the queuefile here so that the fromaddress gets set */
    queuefile = setup_queuefile();
    if(!queuefile) {
        if (get_bool("address-failure"))
           return MODE_END;
        else
           return MODE_ERR;
    }
    close_file(queuefile);

    if(anti_loop()) return MODE_OK;

    res = do_hooks("PRESEND");
    if(res == HOOK_RESULT_OK) {
        res = do_hooks("SEND");
        if(res == HOOK_RESULT_OK)
            res = do_hooks("FINAL");
    }
    if(res == HOOK_RESULT_FAIL)
        return MODE_ERR;
    return MODE_OK;
}

/* Handle a request (subscribe, unsubscribe, set, etc) */
MODE_HANDLER(mode_request)
{
    int inbody, success;
    FILE *queuefile;
    char buffer[BIG_BUF];
    int commands;

    queuefile = setup_queuefile();
    if(!queuefile) {
        if (get_bool("address-failure"))
           return MODE_END;
        else
           return MODE_ERR;
    }
    
    close_file(queuefile);

    buffer_printf(buffer, sizeof(buffer) - 1, "%s.unquote", get_string("queuefile"));
    unquote_file(get_string("queuefile"), buffer);
    if(get_bool("just-unquoted")) {
       if (replace_file(buffer,get_string("queuefile"))) {
           char tempbuf[BIG_BUF];

           buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", buffer, get_string("queuefile"));
           filesys_error(&tempbuf[0]);
           (void)unlink_file(buffer);
       }
    }

    buffer_printf(buffer, sizeof(buffer) - 1, "%s.unmime", get_string("queuefile"));
    unmime_file(get_string("queuefile"),buffer);

    if (get_bool("just-unmimed")) {
       if (replace_file(buffer,get_string("queuefile"))) {
           char tempbuf[BIG_BUF];

           buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", buffer, get_string("queuefile"));
           filesys_error(&tempbuf[0]);
           (void)unlink_file(buffer);
       }
    }

   strip_queue();
   queuefile = open_file(get_string("queuefile"),"r");

   /* If we have a resent-from, this antiloop check will break. */
   if(!get_var("resent-from")) {
      if(anti_loop()) {
         close_file(queuefile);
         return MODE_OK;
      }
   }
	
    /*
     * Two modes, nolist and request could put us in here, so we want to
     * put out a message in the case of the request mode
     */
    if(strcmp(get_string("mode"), "request") == 0) {
        result_printf("Request received for list '%s' via request address.\n",
                      get_string("list"));
    }
    commands = 0;
    inbody = 0;
    success = 0;
    while(!inbody ? (read_file(buffer, sizeof(buffer), queuefile)) : 0) {
        if(strncasecmp("subject:", buffer, 8) == 0) {
            if (!get_bool("jobeoj-wrapper") && !get_bool("ignore-subject-commands")) {
               success = parse_line(buffer+9, CMD_HEADER, queuefile, &commands);
               if(success == CMD_RESULT_END) {
                  close_file(queuefile);
                  return MODE_OK;
               }
            } else {
/*
               set_var("cur-parse-line",buffer,VAR_GLOBAL);
               spit_status("Job/eoj block in message, ignoring command in subject.");
 */
               log_printf(9,"Job/eoj block in message, ignoring any command in subject.\n");
            }
        } else if(buffer[0] == '\n') {
            inbody = 1;
        }
    }
    while((read_file(buffer, sizeof(buffer), queuefile))) {
        if(get_bool("adminspit")) {
            handle_spit_admin(buffer);
        } else 
        if (get_bool("adminspit2")) {
            handle_spit_admin2(buffer);
        } else {
            if(buffer[0] == '\n')
                continue;
            success = parse_line(buffer, CMD_BODY, queuefile, &commands);
            if(success == CMD_RESULT_END)
                break;
        }
    }

    if (get_bool("adminspit")) {
       close_file(get_adminspit());
    }

    /* I was wrong about this being dead code */
    /*
     * This is needed to end the adminspit2 stuff if the end of file
     * is reached without an adminend2 command
     */
    if (get_bool("adminspit2")) {
       FILE *spitfile;

       spitfile = get_adminspit();

       if (spitfile) {
          write_file(get_adminspit(),"adminend\nend\n");
          close_file(get_adminspit());
          buffer_printf(buffer, sizeof(buffer) - 1, "%s.adminspit2", get_string("queuefile"));
          send_textfile(get_string("realsender"),buffer);
          (void)unlink_file(buffer); 
       }
    }

    close_file(queuefile);

    if (!commands) {
       FILE *infile;

       if (get_var("no-command-file")) {
          char tempfilename[BIG_BUF];

          buffer_printf(tempfilename, sizeof(tempfilename) - 1, "%s",
            get_string("no-command-file"));
          if ((infile = open_file(tempfilename,"r")) != NULL) {

             result_printf("\nNo commands detected in message.\n\n");

             while(read_file(buffer, sizeof(buffer), infile)) {
                result_printf("%s",buffer);
             }
             close_file(infile);
          }
       }
    }

    return MODE_OK;
}


/* Send a message to people specifically tagged with a certain flag */
int flagged_send(const char *flag)
{
    FILE *queuefile, *userfile, *errfile;
    char buffer[BIG_BUF];
    int count, errors;
    struct list_user user;
    char *listdir;

    queuefile = setup_queuefile();
    if(!queuefile) {
        if (get_bool("address-failure"))
           return MODE_END;
        else
           return MODE_ERR;
    }

    buffer_printf(buffer, sizeof(buffer) - 1, "%s.serr", get_string("queuefile"));
    set_var("smtp-errors-file", buffer, VAR_GLOBAL);
    errfile = open_file(buffer, "w");
    errors = 0;

    listdir = list_directory(get_string("list"));

    if (listdir) {
       buffer_printf(buffer, sizeof(buffer) - 1, "%s/users", listdir);
       userfile = open_file(buffer, "r");
       free(listdir);
    } else userfile = NULL;

    if(!userfile) {
        if(errfile) { 
           close_file(errfile);
           buffer_printf(buffer, sizeof(buffer) - 1, "%s.serr", get_string("queuefile"));
           unlink_file(buffer);
        }
        log_printf(0, "Unable to open users file for list '%s'.",
                   get_string("list"));
        return MODE_ERR;
    }
    if(!smtp_start(1))
        return MODE_ERR;
    if(!smtp_from(get_string("fromaddress"))) {
        smtp_end();
        if(errfile) {
            write_file(errfile, "%s\n", get_string("smtp-last-error"));
            close_file(errfile);
            close_file(userfile);
            close_file(queuefile);
            bounce_message();
        }
        return MODE_ERR;
    }
    count = 0;
    while(user_read(userfile, &user)) {
        if(user_hasflag(&user, flag) && !user_hasflag(&user, "VACATION")) {

            if (strcasecmp(flag,"ADMIN") || !user_hasflag(&user,"MYOPIC")) {
               count++;
               if(!smtp_to(user.address) && errfile) {
                   errors++;
                   write_file(errfile, "%s\n", get_string("smtp-last-error"));
               }
            }
        }
    }

    /* Last-ditch attempt */
    if((count == errors) || !count) {
        const char *owner;

        owner = get_var("listserver-admin");
        if (owner ? smtp_to(owner) : 0) count++;
    }

    /* If we get here, we didn't send to anyone, so abort early */
    if((count == errors) || !count) {
        smtp_end();
        if(errfile) {
            close_file(errfile);
            if (count) 
               bounce_message();
            else {
               buffer_printf(buffer, sizeof(buffer) - 1, "%s.serr", get_string("queuefile"));
               unlink_file(buffer);
            }
        }
        close_file(userfile);
        close_file(queuefile);

        /* If there's just no one with this flag, don't defer. */
        return (count ? MODE_ERR : MODE_END);
    }

    if(!smtp_body_start())
        return MODE_ERR;
    while(read_file(buffer, sizeof(buffer), queuefile)) {
        smtp_body_text(buffer);
    }
    smtp_body_end();
    smtp_end();

    /* Now handle any bounces from local recipients */
    if(errfile) {
        close_file(errfile);
        if(errors)
            bounce_message();
        else {
            buffer_printf(buffer, sizeof(buffer) - 1, "%s.serr", get_string("queuefile"));
            unlink_file(buffer);
        }
    }
    close_file(userfile);
    close_file(queuefile);
    return MODE_OK;
}

/* What to do with a message to all admins */
MODE_HANDLER(mode_admins)
{
    return flagged_send("ADMIN");
}

/* What to do with a message to all moderators*/
MODE_HANDLER(mode_moderators)
{
    return flagged_send("MODERATOR");
}

/* Initialize the internal commands and modes */
void init_internal(void)
{
    add_cmdarg("-d", 1, "<level>", cmdarg_debug);
    add_cmdarg("-debug", 1, "<level>", cmdarg_debug);
    add_cmdarg("-f", 1, "<inputfile>", cmdarg_filefrom);
    add_cmdarg("-file", 1, "<inputfile>", cmdarg_filefrom);
    add_cmdarg("-r", 1, "<list>", cmdarg_request);
    add_cmdarg("-request", 1, "<list>", cmdarg_request);
    add_cmdarg("-s", 1, "<list>", cmdarg_resend);
    add_cmdarg("-send", 1, "<list>", cmdarg_resend);
    add_cmdarg("-a", 1, "<list>", cmdarg_approved);
    add_cmdarg("-approved", 1, "<list>", cmdarg_approved);
    add_cmdarg("-repost",1,"<list>",cmdarg_approved);
    add_cmdarg("-admins", 1, "<list>", cmdarg_admins);
    add_cmdarg("-moderators", 1, "<list>", cmdarg_moderators);
    add_cmdarg("-c", 1, "<conffile>", cmdarg_config);
    add_cmdarg("-config", 1, "<conffile>", cmdarg_config);
    add_cmdarg("-upgrade", 0, NULL, cmdarg_upgrade);
    add_cmdarg("-cheatsheet", 1, "<file>", cmdarg_cheatsheet);

    add_mode("resend", mode_resend);
    add_mode("request", mode_request);
    add_mode("nolist", mode_request);
    add_mode("approved", mode_approved);
    add_mode("admins", mode_admins);
    add_mode("moderators", mode_moderators);

    /* Default handlers */
    add_mime_handler("text/plain",100,mimehandle_text);
    add_mime_handler("application/pgp", 150, mimehandle_text);
    add_mime_handler("message/.*",150,mimehandle_text);
    add_mime_handler("multipart/.*",250,mimehandle_multipart_default);

    /* Ecartis internal MIME-types, cannot be overridden. */
    add_mime_handler("ecartis-internal/rabid",1,mimehandle_rabid);
    add_mime_handler("ecartis-internal/moderate",1,mimehandle_moderate);

    /* Old Listar internal types.  Cannot be overridden; witness 
     * priority level 1.  Included for backwards compatiblity with
     * some modules. */
    add_mime_handler("listar-internal/rabid",1,mimehandle_rabid);
    add_mime_handler("listar-internal/moderate",1,mimehandle_moderate);

    /* Generic 'eat me' fallback */
    add_mime_handler(".*/.*",1000,mimehandle_unknown);
}
