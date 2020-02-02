#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include "config.h"
#include "user.h"
#include "list.h"
#include "command.h"
#include "parse.h"
#include "forms.h"
#include "smtp.h"
#include "core.h"
#include "fileapi.h"
#include "variables.h"
#include "modes.h"
#include "mystring.h"

FILE *adminspitfile;

FILE *get_adminspit()
{
    return adminspitfile;
}

int open_adminspit(const char *filename)
{
    if (adminspitfile) close_file(adminspitfile);

    if ((adminspitfile = open_file(filename,"w")) == NULL) return 0;

    set_var("adminspit", "true", VAR_GLOBAL);

    return 1;
}

void strip_queue()
{
   FILE *infile, *outfile;
   char buffer[BIG_BUF], filenamebuf[BIG_BUF];
   int gottask, donetask, inbody, skiplen = 0;

   gottask = 0; donetask = 0; inbody = 0;

   if (!exists_file(get_string("queuefile"))) return;

   if ((infile = open_file(get_string("queuefile"),"r")) == NULL)
      return;

   buffer_printf(filenamebuf, sizeof(filenamebuf) - 1, "%s.striptask", get_string("queuefile"));

   if ((outfile = open_file(filenamebuf,"w")) == NULL) {
      close_file(infile);
      return;
   }

   while (read_file(buffer, sizeof(buffer), infile)) {

     if (inbody) {
        if (!strncasecmp(buffer,"// job",6) ||
            !strncasecmp(buffer,"//job",5) ||
            !strncasecmp(buffer+1,"// job",6) ||
            !strncasecmp(buffer+1,"//job",5) ||
            !strncasecmp(buffer+2,"// job",6) ||
            !strncasecmp(buffer+2,"//job",5)) {
           char *skip = strchr(buffer, '/');
           skiplen = skip-buffer;
           gottask = 1;
           donetask = 0;
           read_file(buffer, sizeof(buffer), infile);
           set_var("jobeoj-wrapper","yes",VAR_GLOBAL);
        }
        if(!strncasecmp(buffer+skiplen, "// eoj", 6) ||
           !strncasecmp(buffer+skiplen, "//eoj", 5)) {
           donetask = 1;
        }
     }

     if ((gottask && !donetask) || !inbody) {
        write_file(outfile,"%s",buffer+skiplen);
     }

     if (buffer[skiplen] == '\n') inbody = 1;
   }

   close_file(infile);
   close_file(outfile);

   if (gottask) {
      replace_file(filenamebuf,get_string("queuefile"));
   }

   (void)unlink_file(filenamebuf);   
}

int handle_spit_admin(const char *line)
{
    if (!adminspitfile) return 0;

    if (!strncmp(line,"ENDFILE",7)) {
       close_file(adminspitfile);
       adminspitfile = NULL;
       clean_var("adminspit", VAR_GLOBAL);
       set_var("cur-parse-line","ENDFILE",VAR_GLOBAL);
       spit_status("File accepted.");
    } else {
       write_file(adminspitfile,"%s",line);
    }
    return 1;
}

int handle_spit_admin2(const char *line)
{
    if (!adminspitfile) return 0;

    if (!strncasecmp(line,"adminend2",9) || !strncasecmp(line, "end", 3)) {
       char tbuf[SMALL_BUF];
       char buf[SMALL_BUF];

       write_file(adminspitfile,"adminend\n// eoj\n");
       close_file(adminspitfile);
       adminspitfile = NULL;
       clean_var("adminspit2", VAR_GLOBAL);
       set_var("cur-parse-line",line,VAR_GLOBAL);
       spit_status("Admin request queued, you will receive a wrapper.");

       buffer_printf(tbuf, sizeof(tbuf) - 1, "%s.adminspit2", get_string("queuefile"));
       buffer_printf(buf, sizeof(buf) - 1, "%s admin wrapper", SERVICE_NAME_MC);
       set_var("task-form-subject", buf, VAR_TEMP);
       send_textfile(get_string("realsender"),tbuf);
       (void)unlink_file(tbuf);
    } else {
       write_file(adminspitfile,"%s",line);
    }
    return 1;
}

int parse_line(const char *input_line_base, int type, FILE *queuefile, int *counter)
{
    char *ptr;
    char *ptr2, *outparse;
    char *name, *c;
    struct listserver_cmd *runcmd;
    int lastspace;
    int blocknext;
    int mysize;
    int i, j;
    char input_line[HUGE_BUF];
    struct cmd_params params;

    buffer_printf(input_line, sizeof(input_line) - 1, "%s", input_line_base);

    /* Convert reply-format to normal-format */
    if (input_line[0] == '>') input_line[0] = ' ';

    if (input_line[strlen(input_line) - 1] == '\n') {
        input_line[strlen(input_line) - 1] = '\0';
    }

    if (input_line[0] == '#')
       return CMD_RESULT_CONTINUE;

    if ((input_line[0] == '/') && (input_line[1] == '/'))
       return CMD_RESULT_CONTINUE;

    while(input_line[strlen(input_line) - 1] == '\\') {
        char tempbuf[BIG_BUF];

        input_line[strlen(input_line) - 1] = ' ';
       
        if (read_file(tempbuf, sizeof(tempbuf), queuefile)) {
           stringcat(input_line, tempbuf);
           if (input_line[strlen(input_line) - 1] == '\n') {
               input_line[strlen(input_line) - 1] = '\0';
           }
        }
    }

    mysize = strlen(input_line) + 3;
    outparse = (char *)malloc(mysize);
    memset(outparse,0,mysize);
    ptr2 = outparse;
    ptr = input_line;
    lastspace = 1;
    blocknext = 0;

    while(*ptr && ((*ptr != '\n') || blocknext)) {
        if (isspace((int)(*ptr))) {
            if (!lastspace) {
                lastspace = 1;
                *ptr2++ = ' ';
            }
            ptr++;
        } else {
           lastspace = 0;
           blocknext = 0;
           if (*ptr == '\\') {
               blocknext = 1;
               ptr++;
               *ptr2++ = ' ';
           } else if (*ptr == '\n') {
                ptr++;
           } else
               *ptr2++ = *ptr++;
        }
    }

    *ptr2 = '\0';

    if (!*outparse) return CMD_RESULT_CONTINUE;

    set_var("cur-parse-line",outparse, VAR_GLOBAL);

    name = strtok(outparse, " \t");
    i = 0;
    while((c = strtok(NULL, " \t"))) {
        params.words[i] = c;
        i++;
        if(i == MAX_PARAMS)
            break;
    }
    params.num = i;

    log_printf(9, "cmd: %s\n", name);
    for(j = 0; j < i; j++)
        log_printf(9, "param %d: %s\n", j, params.words[j]);

    runcmd = find_command(name,type);

    if (runcmd) {
        int result;

        (*counter)++;

        if(!get_var("initial-cmd"))
           set_var("initial-cmd", get_string("cur-parse-line"), VAR_GLOBAL);
        result = (runcmd->cmd)(&params);
        free(outparse);

        if (type == CMD_HEADER) {
		/* Check for prevent-second-message variable */
		if (!get_bool("prevent-second-message")) {
		result_printf("\nValid command was found in subject field, body won't be checked for further commands.\n");
		}
           result = CMD_RESULT_END;
        }

        return result;
    } else {
        if (type != CMD_HEADER)
           spit_status("Unknown command.");
        free(outparse);
        return CMD_RESULT_CONTINUE;
    }
}

int parse_message(void)
{
    struct listserver_mode *mode;
    char buffer[BIG_BUF];
    int res;

    clean_var("adminspit", VAR_GLOBAL);
    adminspitfile = NULL;

    mode = find_mode(get_string("mode"));
    if(!mode) {
        buffer_printf(buffer, sizeof(buffer) - 1, "Unknown mode '%s'", get_string("mode"));
        internal_error(buffer);
        return PARSE_END;
    }

    res = mode->modefn();

    if (res == MODE_ERR)
        return PARSE_ERR;

    if (res == MODE_END)
        return PARSE_END;

    return PARSE_OK;
}
