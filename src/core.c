#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef IRIX_IS_CRAP
#include <poll.h>
#endif

#ifndef WIN32
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <pwd.h>

/* Fix for brain-dead IRIX system headers */
#undef EX_OK

#include <sysexits.h>
#endif

#include "alias.h"
#include "list.h"
#include "config.h"
#include "command.h"
#include "parse.h"
#include "init.h"
#include "forms.h"
#include "flag.h"
#include "file.h"
#include "regexp.h"
#include "smtp.h"
#include "lpm-mods.h"
#include "fileapi.h"
#include "variables.h"
#include "core.h"
#include "cookie.h"
#include "cmdarg.h"
#include "internal.h"
#include "modes.h"
#include "mysignal.h"
#include "hooks.h"
#include "user.h"
#include "tolist.h"
#include "compat.h"
#include "unmime.h"
#include "liscript.h"
#include "mystring.h"
#include "submodes.h"
#include "lcgi.h"
#include "funcparse.h"
#include "trust.h"

#ifdef WIN32
extern void sock_init();
#endif

#ifdef _AIX
extern int sys_nerr;
extern char *sys_errlist[];
#endif

#ifdef DEC_UNIX_USLEEP
/* We need to give a function definition for usleep, or else
 * DEC's compiler barfs.  This is the old (pre-ANSI) version */
extern void usleep __((unsigned int));
#endif /* DEC_UNIX_USLEEP */

int messagecnt;

extern void build_lpm_api();

char pathname[BIG_BUF];
const char def_err[] = "Could not resolve error.";

void build_hostname(char *buffer, int len)
{
  char *hostnameholder;
  struct hostent *he;
  int i;

  hostnameholder = (char *)malloc(len);
  if (!hostnameholder) {
    buffer_printf(buffer, len - 1, "unknown-hostname");
	return;
  }

  if (gethostname(hostnameholder, len) == -1) {
    /* gethostname() failed, just put something there. */
    buffer_printf(buffer, len - 1, "unknown-hostname");
	free(hostnameholder);
    return;
  }

  buffer_printf(buffer, len - 1, "%s", hostnameholder);
  free(hostnameholder);

  if (strchr(hostnameholder, 'n')) {
    /* Looks FQDN already; just return. */
    return;
  }
  he = gethostbyname(hostnameholder);
  if (!he) {
    /* Couldn't look it up; do a best guess. */
    return;
  }

  buffer_printf(buffer, len - 1, "%s", he->h_name);
  if (! strchr(buffer, '.')) {
    for (i = 0; he->h_aliases[i]; i++) {
      if (strchr(he->h_aliases[i], '.')) {
        buffer_printf(buffer, len - 1, "%s", he->h_aliases[i]);
        return;
      }
    }
  }
}

/* Sends a textfile (filename) to address in addy. */
int send_textfile(const char *addy, const char *filename)
{
    FILE *myfile;
    char inbuf[BIG_BUF];

    if (!exists_file(filename)) {
        log_printf(3, "File '%s' doesn't exist\n", filename);
        return 0;
    }

    if ((myfile = open_file(filename,"r")) == NULL) {
        log_printf(3, "Unable to open file %s\n", filename);
        return 0;
    }

    if(!task_heading(addy))
        return 0;

    if (strcmp(get_string("realsender"),addy) && get_bool("adminmode") &&
        get_bool("admin-actions-shown")) {
        smtp_body_text(">> File sent due to actions of administrator ");
        smtp_body_line(get_string("realsender"));
        smtp_body_line("");
    }

    while(read_file(inbuf, sizeof(inbuf), myfile)) {
        smtp_body_text(inbuf);
    }

    close_file(myfile);

    task_ending();
    return 1;
}

/* Sends a textfile (filename) to address in addy. */
/* Performs variable expansion */
int send_textfile_expand(const char *addy, const char *filename)
{
    FILE *myfile;
    char inbuf[BIG_BUF];
    char outfilename[BIG_BUF];

    if (!exists_file(filename)) {
        log_printf(3, "File '%s' doesn't exist\n", filename);
        return 0;
    }

    buffer_printf(outfilename, sizeof(outfilename) - 1, "%s.expand", get_string("queuefile"));

    if (!liscript_parse_file(filename,outfilename)) {
        log_printf(3, "Unable to Liscript expand file %s\n", filename);
        return 0;
    }

    if ((myfile = open_file(outfilename,"r")) == NULL) {
        log_printf(3, "Unable to open decoded Liscript file %s\n",
          outfilename);
        unlink_file(outfilename);
        return 0;
    }

    if(!task_heading(addy))
        return 0;

    if (strcmp(get_string("realsender"),addy) && get_bool("adminmode") &&
        get_bool("admin-actions-shown")) {
        smtp_body_text(">> File sent due to actions of administrator ");
        smtp_body_line(get_string("realsender"));
        smtp_body_line("");
    }

    while(read_file(inbuf, sizeof(inbuf), myfile)) {
        smtp_body_text(inbuf);
    }

    close_file(myfile);
    unlink_file(outfilename);

    task_ending();
    return 1;
}

/* Appends the Liscript file */
int expand_append(const char *mainfile, const char *liscript)
{
    char outfilename[BIG_BUF];

    if (!exists_file(mainfile)) {
        log_printf(3, "File '%s' doesn't exist\n", mainfile);
        return 0;
    }

    buffer_printf(outfilename, sizeof(outfilename) - 1, "%s.expand", get_string("queuefile"));

    log_printf(9,"Expanding %s to %s\n", liscript, outfilename);

    if (!liscript_parse_file(liscript,outfilename)) {
        log_printf(3, "Unable to Liscript expand file %s\n", liscript);
        return 0;
    }

    if (!exists_file(outfilename)) {
       log_printf(3, "Unable to read expanded Liscript file.");
       return 0;
    }

    append_file(mainfile,outfilename);
    unlink_file(outfilename);

    return 1;
}

/* Sends a textfile (filename) to address in addy.
   Performs variable expansion.  Appends the queuefile if the
   third variable is true. */
int send_textfile_expand_append(const char *addy, const char *filename,
    int includequeue)
{
    FILE *myfile;
    char inbuf[BIG_BUF];
    char outfilename[BIG_BUF];

    if (!exists_file(filename)) {
        log_printf(3, "File '%s' doesn't exist\n", filename);
        return 0;
    }

    buffer_printf(outfilename, sizeof(outfilename) - 1, "%s.expand", get_string("queuefile"));

    if (!liscript_parse_file(filename,outfilename)) {
        log_printf(3, "Unable to Liscript expand file %s\n", filename);
        return 0;
    }


    if ((myfile = open_file(outfilename,"r")) == NULL) {
        log_printf(3, "Unable to open decoded Liscript file %s\n",
          outfilename);
        unlink_file(outfilename);
        return 0;
    }

    if(!task_heading(addy))
        return 0;

    if (strcmp(get_string("realsender"),addy) && get_bool("adminmode") &&
        get_bool("admin-actions-shown")) {
        smtp_body_text(">> File sent due to actions of administrator ");
        smtp_body_line(get_string("realsender"));
        smtp_body_line("");
    }

    while(read_file(inbuf, sizeof(inbuf), myfile)) {
        smtp_body_text(inbuf);
    }

    close_file(myfile);
    unlink_file(outfilename);

    if (includequeue) {
       if ((myfile = open_file(get_string("queuefile"),"r")) != NULL) {
          smtp_body_line("");
          smtp_body_line("--- Original mail message ---");
          while(read_file(inbuf, sizeof(inbuf), myfile)) {
             smtp_body_text(inbuf);
          }
          close_file(myfile);
       }
    }

    task_ending();
    return 1;
}


/* Inserts a textfile into the results list */
/* Performs variable expansion */
int insert_textfile_expand(const char *filename)
{
    FILE *myfile;
    char inbuf[BIG_BUF];
    char outfilename[BIG_BUF];

    if (!exists_file(filename)) {
        log_printf(3, "File '%s' doesn't exist\n", filename);
        return 0;
    }

    buffer_printf(outfilename, sizeof(outfilename) - 1, "%s.expand", get_string("queuefile"));

    if (!liscript_parse_file(filename,outfilename)) {
        log_printf(3, "Unable to Liscript expand file %s\n", filename);
        return 0;
    }

    if ((myfile = open_file(outfilename,"r")) == NULL) {
        log_printf(3, "Unable to open decoded Liscript file %s\n",
          outfilename);
        unlink_file(outfilename);
        return 0;
    }

    while(read_file(inbuf, sizeof(inbuf), myfile)) {
        result_printf("%s",inbuf);
    }

    close_file(myfile);
    unlink_file(outfilename);

    return 1;
}

/* Spits to the result file.  Used by commands for output in user
 * jobs.  Handles nice word-wrapping, too. */
void spit_status(const char *statustext, ...)
{
    va_list vargs;
    char buf[BIG_BUF];
    int col = 0;
    char *tmp;
    FILE *perrfile;

    buffer_printf(buf, sizeof(buf) - 1, "%s.perr", get_string("queuefile"));
    if ((perrfile = open_file(buf,"a")) != NULL) {
        buffer_printf(buf, sizeof(buf) - 1, "%s", get_string("cur-parse-line"));
        tmp = buf;
        col = 0;
        while(*tmp) {
            if (col == 0) {
                write_file(perrfile, "\n>> ");
            }
            if (((*tmp == ' ') && (col > 65)) || (*tmp == '\n')) {
                col = 0;
                tmp++;
            } else {
                write_file(perrfile, "%c", *tmp);
                tmp++; col++;
            }
        }
        write_file(perrfile, "\n");
        va_start(vargs, statustext);
        vsprintf(buf, statustext, vargs);
        va_end(vargs);
        tmp = buf;
        col = 0;
        while(*tmp) {
            if (((*tmp == ' ') && (col > 65)) || (*tmp == '\n')) {
                col = 1;
                write_file(perrfile,"\n");
                tmp++;
            } else {
                write_file(perrfile, "%c", *tmp);
                tmp++; col++;
            }
        }
        write_file(perrfile, "\n");
        close_file(perrfile);
    }
}

/* Quotes last command, used by status/error outputs. */
void quote_command()
{
    char outbuf[BIG_BUF];

    buffer_printf(outbuf, sizeof(outbuf) - 1, ">> %s", get_string("cur-parse-line"));
    smtp_body_line(outbuf);
}

/* Handles a filesystem error on filename.  Call IMMEDIATELY after
 * a filesys error to make sure errno is correct. */
void filesys_error(const char *filename)
{
    char outbuf[BIG_BUF];
    int lasterr;

    lasterr = errno;

    spit_status("Unable to process request due to filesystem error.");
    if(!error_heading())
        return;
    buffer_printf(outbuf, sizeof(outbuf) - 1, "    List: %s", get_string("list"));
    smtp_body_line(outbuf);
    buffer_printf(outbuf, sizeof(outbuf) - 1, "    User: %s", get_string("fromaddress"));
    smtp_body_line(outbuf);
    buffer_printf(outbuf, sizeof(outbuf) - 1, "  Action: %s", get_string("cur-parse-line"));
    smtp_body_line(outbuf);
    buffer_printf(outbuf, sizeof(outbuf) - 1, "    File: %s", filename);
    smtp_body_line(outbuf);
    buffer_printf(outbuf, sizeof(outbuf) - 1, "   Error: %s", resolve_error(lasterr));
    smtp_body_line(outbuf);
    if (get_bool("error-include-queue")) {
       FILE *infile;
       char filebuf[BIG_BUF];

       smtp_body_line("-- queuefile in error --");
       if ((infile = open_file(get_string("queuefile"),"r")) != NULL) {
         while (read_file(filebuf, sizeof(filebuf), infile)) {
            smtp_body_text(filebuf);
         }
         close_file(infile);
       } else {
         smtp_body_line("<< NO QUEUEFILE! >>");
       }
    }
    error_ending();
}

/* Handles an internal error. */
void internal_error(const char *message)
{
    char outbuf[BIG_BUF];

    spit_status("Unable to process request due to internal error.");
    if(!error_heading())
        return;
    buffer_printf(outbuf, sizeof(outbuf) - 1, "    User: %s", get_string("fromaddress"));
    smtp_body_line(outbuf);
    buffer_printf(outbuf, sizeof(outbuf) - 1, "   Error: %s", message);
    smtp_body_line(outbuf);
    if (get_bool("error-include-queue")) {
       FILE *infile;
       char filebuf[BIG_BUF];

       smtp_body_line("-- queuefile in error --");
       if ((infile = open_file(get_string("queuefile"),"r")) != NULL) {
         while (read_file(filebuf, sizeof(filebuf), infile)) {
            smtp_body_text(filebuf);
         }
         close_file(infile);
       } else {
         smtp_body_line("<< NO QUEUEFILE! >>");
       }
    }
    error_ending();
}

/* Handles regex matching.  Used by blacklisting and other functions. */
int match_reg(const char *pattern, const char *match)
{
    int result;
    regexp *treg;

    result = 0;

    if (!*pattern)
        return 0;

    treg = regcomp((char *)pattern);
    if (treg) {
        result = regexec(treg, (char *)match);
        free(treg);
    }

    return result;
}

/* Checks if an address is blacklisted either in current list
 * (if there is one), or globally. */
int blacklisted(const char *address)
{
    FILE *blacklist;
    char filebuf[BIG_BUF];

    /* This is where the checks against global and local banlists go */

    if (get_var("global-blacklist")) {
       buffer_printf(filebuf, sizeof(filebuf) - 1, "%s/%s", get_string("listserver-conf"),
            get_string("global-blacklist"));

       if ((blacklist = open_file(filebuf,"r")) != NULL) {
           while(read_file(filebuf, sizeof(filebuf), blacklist)) {
               if(filebuf[strlen(filebuf) - 1] == '\n')
                   filebuf[strlen(filebuf) - 1] = '\0';
               if (match_reg(filebuf,address)) {
                   close_file(blacklist);
                   return 1;
               }
           }
           close_file(blacklist);
       }
    }

    if (get_var("list") && get_var("blacklist-mask")) {
        listdir_file(filebuf, get_string("list"), get_string("blacklist-mask"));
        if ((blacklist = open_file(filebuf,"r")) != NULL) {
            while(read_file(filebuf, sizeof(filebuf), blacklist)) {
                if(filebuf[strlen(filebuf) - 1] == '\n')
                    filebuf[strlen(filebuf) - 1] = '\0';
                if(match_reg(filebuf,address)) {
                    close_file(blacklist);
                    return 1;
                }
            }
            close_file(blacklist);
        }
    }
    return 0;
}

/* One of the most oft-called functions.  Handles output to the log.
 * the 'level' is the debug level that must be met before the message
 * will be printed. */
void log_printf(int level, char *format, ...)
{
    static int inlogfunc = 0;  /* Prevents infinite loops due to reentry */
    va_list vargs;
    char mybuf[HUGE_BUF];
    FILE *logfile = NULL;
    const char *logfilename;
    
    if (inlogfunc) {
#ifdef DEBUG
        fprintf(stderr, "Warning: attempted to re-enter log_printf\n");
#endif
        return;
    }
    inlogfunc = 1;

    /* Sanity check! */
    logfilename = get_var("logfile");
    if (!logfilename) {
    	inlogfunc = 0;
    	return;
    }

    /* Are we an absolute path? */
    if (*logfilename == '/') 
        buffer_printf(mybuf, sizeof(mybuf) - 1, "%s", get_string("logfile"));
    else {
        const char *listdatadir;

        /* Sanity check! */
        listdatadir = get_var("listserver-data");
        if (!listdatadir) {
            inlogfunc = 0;
            return;
        }

        buffer_printf(mybuf, sizeof(mybuf) - 1, "%s/%s", listdatadir, logfilename);
    }

    if(level > get_number("debug")) {
        inlogfunc = 0;
        return;
    }

    logfile = open_file(mybuf, "a");

    if (logfile) {
        time_t now;
        struct tm *tm_now;

        time(&now);

        tm_now = localtime(&now);

        strftime(mybuf, sizeof(mybuf) - 1,"[%m/%d/%Y-%H:%M:%S] ",tm_now);

        write_file(logfile, "%s", mybuf);

#ifndef WIN32
        write_file(logfile, "[%d] ", (int)getpid());
#endif

        va_start(vargs, format);
        vsprintf(mybuf, format, vargs);
        write_file(logfile, "%s", mybuf);
#ifdef DEBUG
        fprintf(stderr, "%s", mybuf);
#endif
        va_end(vargs);
        close_file(logfile);
    }
    inlogfunc = 0;
}

void debug_printf(char *format, ...)
{
#ifdef DEBUGCONSOLE
    va_list vargs;
    char mybuf[BIG_BUF];
    time_t now;
    struct tm *tm_now;

    time(&now);

    tm_now = localtime(&now);

    strftime(mybuf, sizeof(mybuf) - 1,"[%m/%d/%Y-%H:%M:%S] ",tm_now);

    fprintf(stderr, mybuf);

    va_start(vargs, format);
    vsprintf(mybuf, format, vargs);
    fprintf(stderr,"%s", mybuf);
    va_end(vargs);

    fflush(stderr);
#endif
}

void result_append(const char *filename)
{
   char mybuf[BIG_BUF];

   buffer_printf(mybuf, sizeof(mybuf) - 1, "%s.perr", get_string("queuefile"));
   append_file(mybuf,filename); 
}

/* Handles printf out to the results file for a user job.
 * Used internally by spit_status, and in a few other places. */
void result_printf(char *format, ...)
{
    va_list vargs;
    char mybuf[BIG_BUF];
    FILE *perrfile;

    buffer_printf(mybuf, sizeof(mybuf) - 1, "%s.perr", get_string("queuefile"));
    if ((perrfile = open_file(mybuf,"a")) != NULL) {
        va_start(vargs, format);
        vsprintf(mybuf, format, vargs);
        write_file(perrfile, "%s", mybuf);
        va_end(vargs);
        close_file(perrfile);
    }
}

/* Generates the queue identifier for this session.  Guaranteed
 * unique, as it's based off timestamp and pid. */
int generate_queue()
{
    time_t now;
    char tbuf[SMALL_BUF];

    time(&now);
#ifdef OBSDMOD
    buffer_printf(tbuf, sizeof(tbuf) - 1, "%s/queue/%lX%d", get_string("listserver-data"), (long)now, (int)getpid());
#else
# if DEC_UNIX || _AIX
    buffer_printf(tbuf, sizeof(tbuf) - 1, "%s/queue/%X%d", get_string("listserver-data"), now, (int)getpid());
# else /* ! DEC_UNIX */
    buffer_printf(tbuf, sizeof(tbuf) - 1, "%s/queue/%lX%d", get_string("listserver-data"), now, (int)getpid());
# endif
#endif

    set_var("queuefile", &tbuf[0], VAR_GLOBAL);
    return 1;
}

/* Generates the queue identifier for this session.  Guaranteed
 * unique, as it's based off timestamp and pid. */
int init_queuefile()
{
    FILE *tqueuefile;
    int temp;

    if (!get_bool("fakequeue")) {
       if ((tqueuefile = open_file(get_string("queuefile"),"w")) == NULL)
          return 0;

       if (!get_var("listserver-infile")) {
          while((temp = fgetc(stdin)) != EOF) {
              putc_file((char)temp, tqueuefile);
          }
       } else {
          FILE *infile;

          if (!exists_file(get_string("listserver-infile"))) {
              log_printf(1,"Unable to open input file '%s'\n", get_string("listserver-infile"));
              close_file(tqueuefile);
              return 0;
          }

          if ((infile = open_file(get_string("listserver-infile"),"r")) == NULL) {
              log_printf(1,"Unable to open input file '%s'\n", get_string("listserver-infile"));
              close_file(tqueuefile);
              return 0;
          }

          while((temp = fgetc(infile)) != EOF) {
              putc_file((char)temp, tqueuefile);
          }
          close_file(infile);
       }

       close_file(tqueuefile);
    }

    return 1;
}

/* Initializes listserver. */
void init_listserver()
{
    time_t now;

    time(&now);

    messagecnt = 1;

    chdir(pathname);

    init_aliases();
    init_vars();
    new_cookies();
    init_regvars();
    set_var("path", pathname, VAR_GLOBAL);
    set_var("listserver-root", pathname, VAR_GLOBAL);
    set_var("global-pass", "yes", VAR_TEMP);
    read_conf(GLOBAL_CFG_FILE, VAR_GLOBAL);
    clean_var("global-pass", VAR_TEMP);
    if(!get_var("listserver-modules")) {
        char tmp[BIG_BUF];
        buffer_printf(tmp, sizeof(tmp) - 1, "%s/modules", get_string("listserver-root"));
        set_var("listserver-modules", tmp, VAR_GLOBAL);
    }
    if(!get_var("listserver-conf")) {
        set_var("listserver-conf", get_string("listserver-root"), VAR_GLOBAL);
    }
    if(!get_var("listserver-data")) {
        set_var("listserver-data", get_string("listserver-root"), VAR_GLOBAL);
    }

#ifndef WIN32
    /* Check to make sure we're running as something OTHER than root.
       It is bad to be root. */
    if (!getuid()) {
       uid_t euid;
       gid_t egid;

       log_printf(1,"%s is running as root, attempting to demote.\n", SERVICE_NAME_MC);

       euid = geteuid(); egid = getegid();
       if (euid) {
          log_printf(1,"Effective UID is other than super-user, all we can demote to is that.\n");
          if (setuid(euid) < 0) {
             log_printf(0, "%s was unable to demote itself from super-user!\n",
                SERVICE_NAME_MC);
          }
          setgid(egid);
       } else {
          struct passwd *pwd;

          pwd = NULL;
          setpwent();       

          pwd = getpwent();

          while (pwd ? !strcasecmp(pwd->pw_name,get_string("lock-to-user")) :
              0)
             pwd = getpwent();

          if (pwd) {
             log_printf(1,"Manually demoting to user '%s'\n",
                get_string("lock-to-user"));
             if (setreuid(pwd->pw_uid, pwd->pw_uid) < 0) {
                log_printf(0,"Unable to demote to user from superuser!\n");
             }
             setregid(pwd->pw_gid, pwd->pw_gid);
          } else {
             log_printf(0,"%s is running as root, and cannot demote itself!\n", SERVICE_NAME_MC);
             log_printf(0,"This is a VERY BAD situation.  Please correct it.\n");
             log_printf(0,"Make sure there is a '%s' user who owns the %s installation.\n", 
                get_string("lock-to-user"), SERVICE_NAME_MC);
          }

          endpwent();
       }
    }
#endif

    log_printf(7, "** INIT ** %s v%s started: %s", SERVICE_NAME_MC,
               VER_PRODUCTVERSION_STR, ctime(&now));
    log_printf(7, "Path is: %s\n", pathname);

    log_printf(7, "Mailserver is: %s\n", get_string("mailserver"));

#ifdef WIN32
    sock_init();
#endif

}

/* Handles finishing listserver. */
void finish_listserver()
{
    time_t now;
    const char *queue;
    const char *errfile;
    char buffer[BIG_BUF], buffer2[BIG_BUF];

    queue = get_string("queuefile");
    errfile = get_string("smtp-errors-file");

    set_var("form-send-as",get_string("listserver-admin"),VAR_TEMP);
    buffer_printf(buffer, sizeof(buffer) - 1, "%s.perr", queue);
    if(exists_file(buffer)) {
        set_var("task-expires","yes",VAR_TEMP);
        if(!get_var("results-subject-override")) {
           if(get_var("initial-cmd")) {
               buffer_printf(buffer2, sizeof(buffer2) - 1, "%s command results: %s", SERVICE_NAME_MC,
                 get_string("initial-cmd"));
           } else {
               buffer_printf(buffer2, sizeof(buffer2) - 1, "%s command results: No commands found", 
                       SERVICE_NAME_MC);
           }
        } else {
           buffer_printf(buffer2, sizeof(buffer2) - 1, "%s: %s", SERVICE_NAME_MC,
             get_string("results-subject-override"));
        }
        set_var("task-form-subject", buffer2, VAR_TEMP);
        send_textfile(get_string("realsender"), buffer);
        clean_var("task-form-subject", VAR_TEMP);

        if(get_var("copy-requests-to")) {
           buffer_printf(buffer2, sizeof(buffer2) - 1, "%s: %s results",
                SERVICE_NAME_MC, get_string("realsender"));
           set_var("task-form-subject", buffer2, VAR_TEMP);
           send_textfile(get_string("copy-requests-to"), buffer);
           clean_var("task-form-subject", VAR_TEMP);
        }

        (void)unlink_file(buffer);
    }

    set_var("form-send-as",get_string("listserver-admin"),VAR_TEMP);
    buffer_printf(buffer, sizeof(buffer) - 1, "%s.errs", queue);
    if(exists_file(buffer)) {
        buffer_printf(buffer2, sizeof(buffer2) - 1, "%s error report.", SERVICE_NAME_MC);
        set_var("task-form-subject", buffer2,VAR_TEMP);
        send_textfile(get_string("listserver-admin"), buffer);
        clean_var("task-form-subject", VAR_TEMP);
        (void)unlink_file(buffer);
    }

    if (!get_bool("preserve-queue")) (void)unlink_file(queue);
    if(errfile)
       (void)unlink_file(errfile);

    time(&now);
    log_printf(9, "%s terminated on %s", SERVICE_NAME_MC, ctime(&now));

    log_printf(9,"Unloading modules...\n");
    unload_all_modules();
    nuke_funcs();
    nuke_cgi_tempvars();
    nuke_cgi_hooks();
    nuke_cgi_modes();
    nuke_submodes();
    nuke_tolist();
    nuke_modes();
    nuke_cmdargs();
    nuke_commands();
    nuke_hooks();
    nuke_flags();
    nuke_files();
    nuke_mime_handlers();
    nuke_cookies();
#ifdef DYNMOD
    nuke_modrefs();
#endif
#ifdef USE_HITCHING_LOCK
    nuke_lockfiles();
#endif
    nuke_vars();
    nuke_aliases();
}

int main (int argc, char** argv)
{
    char *temp;
    int errors = 0;
    int exitearly = 0;
    int count = 0;
    char buf[BIG_BUF];

    buffer_printf(pathname, sizeof(pathname) - 1, "%s", argv[0]);
    temp = strrchr(pathname, '/');

#ifdef WIN32
    if(!temp)
         temp = strrchr(pathname, '\\');
#endif

    if (temp)
        *(temp) = '\0';
    else
        buffer_printf(pathname, sizeof(pathname) - 1, ".");

    argv++;

    init_signals();
    init_listserver();

    new_flags();
    new_commands();
    new_hooks();
    new_files();
    new_cmdargs();
    new_modes();
    new_tolist();
    new_mime_handlers();
    new_submodes();
    new_cgi_hooks();
    new_cgi_modes();
    new_cgi_tempvars();
    new_funcs();

    init_internal();
    build_lpm_api();
#ifdef DYNMOD
    log_printf(9,"Preparing to load dynamic modules...\n");
    init_modrefs();
#endif
    log_printf(9,"Loading modules...\n");
    load_all_modules();
    /*
     * Reload the global config file to pick up any variables defined by
     * the modules
     */
    read_conf(GLOBAL_CFG_FILE, VAR_GLOBAL);
    log_printf(9,"Initializing modules...\n");
    init_all_modules();

    if(!get_var("lists-root")) {
        buffer_printf(buf, sizeof(buf) - 1, "%s/lists", get_string("listserver-data"));
        set_var("lists-root", buf, VAR_GLOBAL);
    } else {
		const char *listsroot = get_var_unexpanded("lists-root");

        /* redirect lists-root to be relative to listserver-data */
        buffer_printf(buf, sizeof(buf) - 1, "%s/%s", get_string("listserver-data"), listsroot);
        set_var("lists-root", buf, VAR_GLOBAL);
    }

    init_restricted_vars();

    generate_queue();


    while(*argv) {
        struct listserver_cmdarg *tmp = find_cmdarg(argv[0]);
        if(!tmp) {
            buffer_printf(buf, sizeof(buf) - 1, "Unrecognized command line argument '%s'.", argv[0]);
            internal_error(buf);
            errors = 1;
            exitearly = 1;
            break;
        } else {
            int res = tmp->fn(++argv, ++count);
            if(res == CMDARG_ERR) {
                exitearly = 1;
                errors = 1;
                break;
            } else if(res == CMDARG_EXIT) {
                exitearly = 1;
                break;
            }
            argv += tmp->params;
        }
    }

    if(!exitearly) {
        /*
         * this has to go here so that cookies expire before they can be used
         * if they are stale.   It has to come after the argument parsing so
         * that virtual host files are picked up correctly.
         */
        log_printf(8, "Expiring old cookies.\n");
        expire_all_cookies();
        log_printf(8, "Done expiring old cookies.\n");
        wipe_vars(VAR_LIST|VAR_TEMP);

        /*
         * Now, we need to initialize the list config file if we have a current
         * list
         */
        if(get_var("list")) list_read_conf();

        if (init_queuefile()) {
           if(parse_message() == PARSE_ERR)
               errors = 1;
        }
    }

    finish_listserver();

    if(!errors)
        return 0;
    else
        return EX_TEMPFAIL;
}

/* Bounce a message */
void bounce_message(void)
{
    char buf[BIG_BUF];
    char buffer[BIG_BUF];
    const char *sender;
    time_t now;
    FILE *errfile;
   

    if(!get_var("smtp-errors-file"))
        return;
    /*
     * The error file better have been closed before we get into here other
     * wise we have a locking conflict.
     */
    errfile = open_file(get_string("smtp-errors-file"), "r");
    if(!errfile)
        return;

    buffer_printf(buf, sizeof(buf) - 1, "MAILER-DAEMON@%s", get_string("mailserver"));
    if (smtp_start(0)) {
       if(!smtp_from(buf)) {
           return;
       }

       sender = get_var("send-as");
       if(!sender) {
           sender = get_var("list-owner");
           if(!sender) {
               sender = get_var("listserver-admin");
               if(!sender) {
                   /* we're screwed.. abort. */
                   close_file(errfile);
                   return;
               }
           }
       }

       if(!smtp_to(sender))
           return;
       if(!smtp_body_start())
           return;

       now = time(NULL);
       buffer_printf(buffer, sizeof(buffer) - 1, "Date: %s", ctime(&now));
       smtp_body_text(buffer);
       buffer_printf(buffer, sizeof(buffer) - 1, "From: %s", buf);
       smtp_body_line(buffer);
       buffer_printf(buffer, sizeof(buffer) - 1, "To: %s", sender);
       smtp_body_line(buffer);
       smtp_body_line("Subject: Errors while delivering message");
       buffer_printf(buffer, sizeof(buffer) - 1, "X-%s-Bounce: %s",
          SERVICE_NAME_MC, get_string("listserver-owner"));
       smtp_body_line(buffer);

       smtp_body_line("");
       while(read_file(buffer, sizeof(buffer), errfile)) {
          smtp_body_text(buffer);
       } 

       smtp_body_end();
       smtp_end();
    }
    close_file(errfile);
    unlink_file(get_string("smtp-errors-file"));
}

int flagged_send_textfile(const char *fromaddy, const char *list, 
                          const char *flag, const char *filename,
                          const char *subject)
{
    FILE *queuefile, *userfile, *errfile;
    char buffer[BIG_BUF];
    char hostname[SMALL_BUF];
    char datebuffer[80];
    char datestr[80];
    int count, errors;
    time_t now;
    struct tm *tm_now;
    struct list_user user;
    const char *fromname;
    char *listdir;

    log_printf(15,"Entering flagged_send_textfile...\n");

    if (!exists_file(filename)) return 0;

    if ((queuefile = open_file(filename,"r")) == NULL)
        return 0;

    buffer_printf(buffer, sizeof(buffer) - 1, "%s.serr", get_string("queuefile"));
    set_var("smtp-errors-file", buffer, VAR_GLOBAL);
    errfile = open_file(buffer, "w");
    errors = 0;

    listdir = list_directory(list);

    if (listdir) {
        buffer_printf(buffer, sizeof(buffer) - 1, "%s/users", listdir);
        userfile = open_file(buffer, "r");
        free(listdir);
    } else userfile = NULL;

    if(!userfile) {
        if(errfile) close_file(errfile);
        log_printf(0, "Unable to open users file for list '%s'.", list);
        return 0;
    }
    if(!smtp_start(1))
        return 0;
    if(!smtp_from(fromaddy)) {
        smtp_end();
        if(errfile) {
            write_file(errfile, "%s\n", get_string("smtp-last-error"));
            close_file(errfile);
            close_file(userfile);
            close_file(queuefile);
            bounce_message();
        }
        return 0;
    }
    count = 0;
    while(user_read(userfile, &user)) {
        if(user_hasflag(&user, flag) && 
                (strcmp(flag,"VACATION") ? !user_hasflag(&user, "VACATION") : 1)) {
            count++;
            if(!smtp_to(user.address) && errfile) {
                errors++;
                write_file(errfile, "%s\n", get_string("smtp-last-error"));
            }
        }
    }
    /* If we get here, we didn't send to anyone, so abort early */
    if((count == errors) || !count){
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
        return 0;
    }

    time(&now);
    get_date(datestr, sizeof(datestr), now);
    buffer_printf(datebuffer, sizeof(datebuffer) - 1, "Date: %s", datestr);

    if(!smtp_body_start())
        return 0;
    if(get_var("hostname")) {
        buffer_printf(hostname, sizeof(hostname) - 1, "%s", get_string("hostname"));
    } else {
        memset(hostname, 0, sizeof(hostname));
        build_hostname(hostname, sizeof(hostname));
    }
    buffer_printf(buffer, sizeof(buffer) - 1, "Received: with %s (v%s); %s", SERVICE_NAME_MC,
            VER_PRODUCTVERSION_STR, datestr);
    smtp_body_line(buffer);
    smtp_body_line(datebuffer);
    fromname = get_var("listserver-full-name");
    buffer_printf(buffer, sizeof(buffer) - 1, "From: %s <%s>", fromname, get_string("listserver-address"));
    smtp_body_line(buffer);
    buffer_printf(buffer, sizeof(buffer) - 1, "To: Members flagged %s of list %s <%s>", flag, list, 
            get_string("listserver-address"));
    smtp_body_line(buffer);
    buffer_printf(buffer, sizeof(buffer) - 1, "Subject: %s", subject);
    smtp_body_line(buffer);
    tm_now = localtime(&now);
    buffer_printf(buffer, sizeof(buffer) - 1, "%s-%s", SERVICE_NAME_LC, "%m%d%Y%H%M%S");
    strftime(datebuffer, sizeof(datebuffer) - 1, buffer, tm_now);
    buffer_printf(buffer, sizeof(buffer) - 1, "Message-ID: <%s.%d.%d@%s>", datebuffer, (int)getpid(),
            messagecnt++, hostname);
    smtp_body_line(buffer);
    buffer_printf(buffer, sizeof(buffer) - 1, "X-%s-Version: %s v%s", SERVICE_NAME_MC, SERVICE_NAME_MC,
            VER_PRODUCTVERSION_STR);
    smtp_body_line(buffer);
    if (get_var("stocksend-extra-headers")) {
        smtp_body_line(get_string("stocksend-extra-headers"));
    }
    smtp_body_line(""); 

    while(read_file(buffer, sizeof(buffer), queuefile)) {
        smtp_body_text(buffer);
    }

    if (!get_bool("task-no-footer")) {
        smtp_body_line("");
        smtp_body_line("---");
        buffer_printf(buffer, sizeof(buffer) - 1, "%s v%s - job execution complete.", SERVICE_NAME_MC,
                VER_PRODUCTVERSION_STR);
        smtp_body_line(buffer);
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
    return 1;
}

const char *resolve_error(int error)
{
   const char *tmp;

   if (error > sys_nerr) tmp = &def_err[0]; else tmp = sys_errlist[error];

   return tmp;
}

void nosuch(const char *listname)
{
    spit_status("No such list '%s'", listname);

    if (!get_bool("adminmode")) {
        result_printf("\nFor information on what mailing lists are available on this\n");
        result_printf("site, send e-mail to %s with 'lists' in the subject\n",get_string("listserver-address"));
        result_printf("or body.\n");
    }
}

void get_date(char *buffer, int len, time_t now)
{
#if defined(WIN32) || !defined(GNU_STRFTIME)
   char tstr[80];
# ifdef WIN32
   static set_tz = 0;
   static TIME_ZONE_INFORMATION tzInfo;
   static long tzType;
   static long bias;
# endif
# ifdef NO_TM_GMTOFF
   static int set_tz = 0;
   static long bias;
   struct tm tmptime = *gmtime(&now);
# endif
#endif
   struct tm *tm_now = localtime(&now);
 
#ifndef WIN32
# ifdef GNU_STRFTIME
   strftime(buffer, len - 1,"%a, %d %b %Y %H:%M:%S %z (%Z)",tm_now);
# else
#  ifdef NO_TM_GMTOFF
   /*
    * This isn't the best answer, and I will need access to a solaris
    * box to get a better one
    */
   strftime(buffer, len - 1,"%a, %d %b %Y %H:%M:%S",tm_now);
   if(!set_tz) {
       set_tz = 1;
       bias = (tm_now->tm_hour - tmptime.tm_hour) * 60 +
              tm_now->tm_min - tmptime.tm_min;

       /* assume that offset isn't more than a day ... */
       if (tm_now->tm_year < tmptime.tm_year)
           bias -= 24 * 60;
       else if (tm_now->tm_year > tmptime.tm_year)
           bias += 24 * 60;
       else if (tm_now->tm_yday < tmptime.tm_yday)
           bias -= 24 * 60;
       else if (tm_now->tm_yday > tmptime.tm_yday)
           bias += 24 * 60;
   }
#   ifdef HAVE_TZNAME
#    ifdef MY_PRINTF_IS_BRAINDEAD
   buffer_printf(tstr, sizeof(tstr) - 1, " %+.03d%.02d (%s)", (int)(bias/60), (int)(bias%60),
           tzname[(tm_now->tm_isdst ? 1 : 0)]);
#    else
   buffer_printf(tstr, sizeof(tstr) - 1, " %+.02d%.02d (%s)", (int)(bias/60), (int)(bias%60),
           tzname[(tm_now->tm_isdst ? 1 : 0)]);
#    endif
#   else
#    ifdef HAVE_TIMEZONE
#     ifdef MY_PRINTF_IS_BRAINDEAD
   buffer_printf(tstr, sizeof(tstr) - 1, " %+.03d%.02d (%s)", (int)(bias/60), (int)(bias%60),
           timezone(bias, tm_now->tm_isdst));
#     else
   buffer_printf(tstr, sizeof(tstr) - 1, " %+.02d%.02d (%s)", (int)(bias/60), (int)(bias%60),
           timezone(bias, tm_now->tm_isdst));
#     endif
#    else
   buffer_printf(tstr, sizeof(tstr) - 1, " %+.02d%.02d", (int)(bias/60), (int)(bias%60));
#    endif /* HAVE_TIMEZONE */
#   endif /* HAVE_TZNAME */
   strncat(buffer, tstr, len - 1 - strlen(buffer));
#  else
   strftime(buffer,len - 1,"%a, %d %b %Y %H:%M:%S",tm_now);
#   ifdef MY_PRINTF_IS_BRAINDEAD
   buffer_printf(tstr, sizeof(tstr) - 1, " %+.03d%.02d (%s)", (int)((tm_now->tm_gmtoff)/3600),
           (int)(((tm_now->tm_gmtoff)/60)%60), tm_now->tm_zone );
#   else
   buffer_printf(tstr, sizeof(tstr) - 1, " %+.02d%.02d (%s)", (int)((tm_now->tm_gmtoff)/3600),
           (int)(((tm_now->tm_gmtoff)/60)%60), tm_now->tm_zone );
#   endif
   strncat(buffer, tstr, len - 1 - strlen(buffer));
#  endif
# endif
#else
   if(!set_tz) {
      _tzset();
      tzType = GetTimeZoneInformation( &tzInfo );
      bias = tzInfo.Bias;
      switch (tzType) {
         case TIME_ZONE_ID_STANDARD:
            bias += tzInfo.StandardBias;
            break;
         case TIME_ZONE_ID_DAYLIGHT:
            bias += tzInfo.DaylightBias;
            break;
      }
      bias *= -1;
      set_tz = 1;
   }
   strftime(buffer,len - 1,"%a, %d %b %Y %H:%M:%S", tm_now);

   /*
    * Windows refuses to return the 3 character time zone, so we will
    * just not use it.
    */
   buffer_printf(tstr, sizeof(tstr) - 1, " %+.02d%.02d", (bias/60), (bias%60));
   strncat(buffer, tstr, len - 1 - strlen(buffer));
#endif
}

void do_sleep(int millis) {
#ifndef WIN32
# ifdef NEED_USLEEP
    struct timeval tv;
    tv.tv_usec = millis * 1000;
    select(0, NULL, NULL, NULL, &tv);
# else
    usleep(millis*1000);
# endif
#else
    Sleep(millis);
#endif
}

