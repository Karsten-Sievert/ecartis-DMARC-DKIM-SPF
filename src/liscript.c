#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>
#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#endif
#include <stdlib.h>

#include "core.h"
#include "config.h"
#include "mystring.h"
#include "fileapi.h"
#include "variables.h"
#include "liscript.h"
#include "funcparse.h"
#include "userstat.h"

#define LPARSE_NORMAL   0
#define LPARSE_PREBLOCK 1
#define LPARSE_SIMPLE   2
#define LPARSE_LOGIC    3
#define LPARSE_COMMENT  4

#define LISCRIPT_MAX_RECURSE 5

int liscript_parse_file_lowlevel(FILE *infile, FILE *outfile, int level);
int parse_until_else_endif(FILE *infile, FILE *outfile, int level);
int skip_until_else_endif(FILE *infile, FILE *outfile, int level);
int parse_until_endif(FILE *infile, FILE *outfile, int level);
int skip_until_endif(FILE *infile, FILE *outfile, int level);


char *liscript_format_variable(const char *fmtstr, const char *varname)
{
   int rjust;
   int hex;
   int prec;
   int gotnum, error;
   int zeropad;
   char tempbuffer[BIG_BUF];
   char tbuf2[BIG_BUF];
   const char *inptr;
   char *outptr;
   struct var_data *var;
   const char *val;

   var = find_var_rec(varname);

   /* Sanity check */
   if (!var) return NULL;

   val = get_cur_varval(var);

   zeropad = rjust = gotnum = error = hex = prec = 0;

   inptr = fmtstr;

   while(*inptr && !error) {
      switch (*inptr) {
         case '+': 
           if (!gotnum) rjust = 1; else error = 1;
           break;
         case 'x':
           if (!gotnum) hex = 1; else error = 1;
           break;
         case '.':
           if (!gotnum) {
              zeropad = 1;
           } else error = 1;
         default:
           if (isdigit((int)*inptr)) {
              prec = (prec * 10) + (*inptr - '0');
           }
           break;
      }
      inptr++;
   }

   memset(tbuf2, 0, sizeof(tbuf2));
   memset(tempbuffer, 0, sizeof(tempbuffer));


   if (error) {
      log_printf(1,"Bad format string '%s' for var '%s'\n",
         fmtstr, var->name);
      return NULL;
   } else {

      /* Ok, yes, I know... this really SHOULD just read the
         data directly, but since it'll all be rewritten for the
         string handling rewrite and this code will go away,
         I don't care right now and will use the get_* functions
         to format out data. */
         if (var->type == VAR_BOOL) {
           int bval;

           bval = get_bool(var->name);
           if (prec) {
              if (rjust) {
                 buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%+*s", 
                   prec, bval ? "true" : "false");
              } else {
                 buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%-*s", 
                   prec, bval ? "true" : "false");
              }
           } else {
              buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%s", 
                   bval ? "true" : "false");
           }
         } else if (var->type == VAR_DURATION) {
           if (prec) {
              if (rjust) {
                 buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%+*s", 
                   prec, val ? val : "");
              } else {
                 buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%-*s", 
                   prec, val ? val : "");
              }
           } else {
              buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%s", 
                   val ? val : "");
           }
         } else if ((var->type == VAR_STRING) || (var->type == VAR_CHOICE)) {
           if(val) {
              if(var->flags & VAR_NOEXPAND)
                 buffer_printf(tbuf2, sizeof(tbuf2) - 1, "%s", val);
              else
                 liscript_parse_line(val, tbuf2, sizeof(tbuf2) - 1);
           } else {
              buffer_printf(tbuf2, sizeof(tbuf2) - 1, "(No value set)");
           }
           if (prec) {
              if (zeropad) {
                 if (rjust) {
                    buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%+.*s", prec, tbuf2);
                 } else {
                    buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%.*s", prec, tbuf2);
                 }
              } else {
                 if (rjust) {
                    buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%+*s", prec, tbuf2);
                 } else {
                    buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%*s", prec, tbuf2);
                 }
              }
           } else {
              buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%s", tbuf2);
           }
         } else if (var->type == VAR_TIME) {
            char datebuf[BIG_BUF];

            if (!val) {
              time_t now;

              now = time(NULL);
              get_date(datebuf, sizeof(datebuf),now);
            } else {
              get_date(datebuf, sizeof(datebuf),atoi(val));
            }

            if (prec) {
               if (rjust) {
                  buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%+*s",
                    prec, datebuf);
               } else {
                  buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%-*s",
                    prec, datebuf);
               }
            } else {
               buffer_printf(tempbuffer, sizeof(tempbuffer) - 1, "%s",
                   datebuf);
            }
         } else if (var->type == VAR_INT) {
           int ival;

           ival = val ? atoi(val) : 0;

           if (prec) {
              if (zeropad) {
                 buffer_printf(tempbuffer, sizeof(tempbuffer) - 1,
                   hex ? "%.*x" : "%.*d",
                   prec, ival);
              } else if (rjust) {
                 buffer_printf(tempbuffer, sizeof(tempbuffer) - 1,
                   hex ? "%*x" : "%*d", prec, ival);
              } else {
                 buffer_printf(tempbuffer, sizeof(tempbuffer) - 1,
                   hex ? "%-*x" : "%-*d", prec, ival);
              }
           } else {
              buffer_printf(tempbuffer, sizeof(tempbuffer) - 1,
                   hex ? "%x" : "%d", ival);
           }
         } 
/* I don't THINK we need this, but in case, I wrote it 
   It might be useful if we want to ensure a common formatting for
   duration strings - actually, we might want to put it into a
   function elsewhere, duration_to_string :) */
/*
         else if (var->type == VAR_DURATION) {
            char secstr[SMALL_BUF], minstr[SMALL_BUF], hourstr[SMALL_BUF], daystr[SMALL_BUF];
            int sec, min, hour, day;
            int ival;

            sec = min = hour = day = 0;

            memset(secstr, 0, sizeof(secstr));
            memset(minstr, 0, sizeof(minstr));
            memset(hourstr, 0, sizeof(hourstr));
            memset(daystr, 0, sizeof(daystr));

            ival = get_seconds(var->name);

            if (ival) {

               sec = ival;

               if (sec >= 60) {
                  sec = ival % 60;
                  min = ival / 60;
               }

               if (sec)
                 buffer_printf(secstr, sizeof(secstr) - 1, "%d s", sec);

               if (min >= 60) {
                  hour = min / 60;
                  min = min % 60;
                  if (min) 
                     buffer_printf(minstr, sizeof(minstr) - 1, "%d m", min);
               }

               if (hour >= 24) {
                  day = hour / 24;
                  hour = hour % 24;
                  if (hour)
                     buffer_printf(hourstr, sizeof(hourstr) - 1, "%d h", hour);
                  if (day)
                     buffer_printf(daystr, sizeof(daystr) - 1, "%d d", day);
               }

               buffer_printf(tempbuffer, sizeof(tempbuffer) - 1,
                 "%s%s%s%s%s%s%s", 
                 daystr, 
                 day && (hour || min || sec) ? " " : "",
                 hourstr, 
                 hour && (min || sec) ? " " : "",
                 minstr, 
                 min && sec ? " " : "",
                 secstr);
               
            } else {
               buffer_printf(tempbuffer, sizeof(tempbuffer) - 1,
                 "No duration set");
            }
         }
*/
   }
   outptr = strdup(tempbuffer);

   return outptr;
}

int liscript_parse_line(const char *inputline, char *outline, int bufferlen)
{
   char tempchar, escape, parsemode, *bufptr = NULL;
   int tagfmt;
   char buffer[BIG_BUF], fmtbuf[BIG_BUF];
   const char *inptr;
   char *outptr;
   int done;
   char bounder = '\0';

   parsemode = LPARSE_NORMAL; done = 0;
   escape = 0; tagfmt = 0;

   inptr = inputline;
   outptr = outline;

   memset(outline, 0, bufferlen);

   while(*inptr && ((outptr - outline) < bufferlen) && !done) {
      tempchar = *inptr;

      if (tempchar == '\\') escape = 1;

      switch(parsemode) {
         case LPARSE_NORMAL: /* Normal file i/o */
           {
              if (((tempchar == '<') || (tempchar == '[')) && !escape) {
                 parsemode = LPARSE_PREBLOCK;
                 bounder = tempchar;
              } else {
                 if (tempchar == '\n') {
                    if (!escape) {
                       *outptr = 0;
                       done = 1;
                    }
                 } else {
                    *outptr++ = *inptr;
                    escape = 0;
                 }
              }
           }
           break;

        case LPARSE_PREBLOCK: /* Beginning of a Liscript block? */
          {
             memset(buffer, 0, sizeof(buffer));
             bufptr = &buffer[0];
             if (tempchar == '$') {
                parsemode = LPARSE_SIMPLE;
                tagfmt = 0;
                memset(fmtbuf, 0, sizeof(fmtbuf));
             } else 
             if (tempchar == '#') parsemode = LPARSE_COMMENT; else
             if (tempchar == '?') parsemode = LPARSE_LOGIC; else 
             if ((tempchar == '<') || (tempchar == '[')) {
                *outptr++ = bounder;
                bounder = tempchar;
             } else {
                parsemode = LPARSE_NORMAL;
                outptr = outline + buffer_printf(outline, bufferlen - 1,
                  "%s%c%c", outline, bounder, tempchar);
             }
          }
          break;

        case LPARSE_SIMPLE: /* Liscript variable replacement */
          {
             if ((((tempchar == '>') && (bounder == '<')) ||
                 ((tempchar == ']') && (bounder == '[')))) {
                char *varfmt;

                varfmt = liscript_format_variable(fmtbuf,buffer);

                if (varfmt) {
                   *outptr = 0;
                   strncat(outline, varfmt, bufferlen - 1 - strlen(outline));
                   free(varfmt);
                   varfmt = NULL;
                   outptr = outline + (strlen(outline));
                } else {
                   *outptr = 0;
                   strncat(outline, "<NoValue>", bufferlen - 1 - strlen(outline));
                   outptr = outline + (strlen(outline));
                }

                parsemode = LPARSE_NORMAL;
             } else if (tempchar == '=') {
                *bufptr = 0; bufptr = &fmtbuf[0];
             } else {
                *bufptr++ = tempchar;
             }
          }
          break;

        case LPARSE_LOGIC: /* Liscript logic block */
          {
             if ((((tempchar == '>') && (bounder == '<')) ||
                 ((tempchar == ']') && (bounder == '['))) && !escape) {
                log_printf(5,"Logic block in line: %s\n", inputline);
             }
          }
          break;

        case LPARSE_COMMENT: /* Liscript comment */
          {
             if ((((tempchar == '>') && (bounder == '<')) ||
                 ((tempchar == ']') && (bounder == '['))) && !escape) {
                parsemode = LPARSE_NORMAL;
                /* We just eat comment blocks */
             }
          }
          break;

        default:
          /* We Should Never Be Here */
          break;
      }
      inptr++;
   }

   log_printf(18, "Parsed '%s' into '%s'\n", inputline, outline);
   return 1;
}

int liscript_parse_logic(FILE *outfile, int level,
       const char *command, const char *params, int ifblock, int swapped,
       int parseeat)
{

   log_printf(9,"Liscript: logic: %s - %s\n", command, params ? params : 
      "No params");

   /* include command */
   if (!parseeat && strcasecmp(command,"include") == 0) {
      if (!params) {
         write_file(outfile,"(Liscript: include requires filename)\n");
         return 0;
      } else {
         FILE *newinfile;

         if ((newinfile = open_file(params,"r")) == 0) {
            write_file(outfile,"(Liscript: file '%s' not found.)\n",
              params);
            return 0;
         }

         liscript_parse_file_lowlevel(newinfile,outfile,level+1);

         return 1;
      }
   }

   /* version command */
   else if (!parseeat && strcasecmp(command,"version") == 0) {
      write_file(outfile,"%s %s",
        SERVICE_NAME_MC, VER_PRODUCTVERSION_STR);
      return 1;
   }

   else if (!parseeat && strcasecmp(command,"hostname") == 0) {
      if (get_var("hostname"))
        write_file(outfile,"%s", get_var("hostname"));
      else {
        char hostname[BIG_BUF];

        build_hostname(hostname, sizeof(hostname) - 1);

        write_file(outfile,"%s",hostname);      
      }
      return 1;
   }

   else if (!parseeat && strcasecmp(command,"userstat") == 0) {
      char tparams[BIG_BUF];
      char *tchr;
      const char *list;
      char paramval[BIG_BUF];
      const char *username;

      list = get_var("list");

      if (!list) {
         write_file(outfile,"(Liscript:'userstat' only valid in a list context.)");
         return 0;
      }

      if (!params) {
         write_file(outfile,"(Liscript:'userstat' requires params.)");
         return 0;
      }

      stringcpy(tparams,params);
      if (!(tchr = strchr(tparams,','))) {
         write_file(outfile,"(Liscript:'userstat' requires 2 params.)");
         return 0;
      }

      *tchr++ = 0; 

      if (tparams[0] == '$') {
         username = get_var(&tparams[1]);
         if (!username)
            return 0;
      } else {
         username = &tparams[0];
      }

      if (userstat_get_stat(list,username,tchr, paramval, sizeof(paramval) - 1)) {
         write_file(outfile,"%s",paramval);
      }
      return 1;
   }

   else if (strcasecmp(command,"else") == 0) {
      if (!ifblock) {
         write_file(outfile,"(Liscript: Improper use of 'else' outside an if block)");
         return 0;
      }

      if (swapped) {
         write_file(outfile,"(Liscript: Not allowed to use more than one 'else' per if block)");
         return 0;
      }

      return 3;
   }

   else if (strcasecmp(command,"if") == 0) {
      return 2;
   }

   else if (strcasecmp(command,"endif") == 0) {
      if (!ifblock) {
         write_file(outfile,"(Liscript: endif without matching if)");
         return 0;
      }

      return 4;
   }

   /* Fallback */
   return 0;
}

int parse_until_endif(FILE *infile, FILE *outfile, int level)
{
   char tempchar, escape, parsemode, startline, *bufptr=NULL;
   int value, tagfmt;
   char buffer[BIG_BUF], fmtbuf[BIG_BUF];
   char bounder = '\0';

   parsemode = 0;
   escape = 0; tagfmt = 0;
   startline = 1;

   while((value = fgetc(infile)) != EOF) {
      tempchar = (char)value;
      if (tempchar == '\\') escape = 1;

      switch(parsemode) {
         case LPARSE_NORMAL: {
              if (((tempchar == '<') || (tempchar == '[')) && !escape) {
                 parsemode = LPARSE_PREBLOCK;
                 bounder = tempchar;
              } else {
                 fputc(tempchar,outfile);
                 escape = 0;
                 if (tempchar == '\n') startline = 1; else startline = 0;
              }
           }
           break;
        case LPARSE_PREBLOCK: {
             memset(buffer, 0, sizeof(buffer));
             bufptr = &buffer[0];
             if (tempchar == '$') {
                parsemode = LPARSE_SIMPLE;
                tagfmt = 0;
                memset(fmtbuf, 0, sizeof(fmtbuf));
             } else 
             if (tempchar == '#') parsemode = LPARSE_COMMENT; else
             if (tempchar == '?') parsemode = LPARSE_LOGIC; else 
             if ((tempchar == '<') || (tempchar == '[')) {
                fputc(bounder,outfile);
                bounder = tempchar;
             } else {
                parsemode = LPARSE_NORMAL;
                fputc(bounder,outfile);
                fputc(tempchar,outfile);
                startline = 0;
             }
          }
          break;
        case LPARSE_SIMPLE: {
             if (((tempchar == '>') && (bounder == '<')) ||
                 ((tempchar == ']') && (bounder == '['))) {
                char *varfmt;

                varfmt = liscript_format_variable(fmtbuf,buffer);

                fprintf(outfile,"%s",varfmt ? varfmt : "<NoValue>");
                if (varfmt) {
                   free(varfmt);
                   varfmt = NULL;
                }
                parsemode = LPARSE_NORMAL;

                /* Eat trailing newline if we're the only thing on the line */
                if (startline) {
                   int temp;

                   temp = fgetc(infile);
                   if (temp != EOF) {
                      tempchar = (char)temp;

                      if (tempchar != '\n') ungetc(tempchar, infile);
                   } else ungetc(EOF,infile);
                }
             } else if (tempchar == '=') {
                *bufptr = 0; bufptr = &fmtbuf[0];
             } else {
                *bufptr++ = tempchar;
             }
          }
          break;

        case LPARSE_LOGIC: {
             char *logicparam;
             int logictype;

             if ((((tempchar == '>') && (bounder == '<')) ||
                 ((tempchar == ']') && (bounder == '[')))
                  && !escape) {

                if (startline) {
                   int temp;

                   temp = fgetc(infile);
                   if (temp != EOF) {
                      tempchar = (char)temp;

                      if (tempchar != '\n') ungetc(tempchar, infile);
                   } else ungetc(EOF,infile);
                }
                parsemode = LPARSE_NORMAL;
                logicparam = strchr(buffer,' ');
                if (logicparam) {
                   *logicparam++ = 0;
                   while(isspace((int)*logicparam)) logicparam++;
                }
                logictype = liscript_parse_logic(outfile,level,buffer,logicparam, 1, 1, 0);
                switch(logictype) {
                   case 0: /* do nothing */
                   case 1: /* do nothing */
                     break;
                   case 2: /* An if */ {
                     int ifresult;
                     int res;
                     if (logicparam) {
                       char resbuf[BIG_BUF];
                       char errbuf[BIG_BUF];
                       res = parse_function(logicparam, resbuf, errbuf);
                       if(res) {
                           log_printf(15, "Function '%s' eval to '%s'\n", logicparam, resbuf);
                           ifresult = !(!(atoi(resbuf)));
                       } else {
                           log_printf(5, "Error in Liscript expression '%s'\n", logicparam);
                           log_printf(5, "Error was: %s\n", errbuf);
                           ifresult = 1;
                       }
                     } else ifresult = 1;
                     if(ifresult) {
                         res = parse_until_else_endif(infile, outfile, level);
                         if(res == 1)
                             skip_until_endif(infile, outfile, level);
                     } else {
                         res = skip_until_else_endif(infile, outfile, level);
                         if(res == 1)
                             parse_until_endif(infile, outfile, level);
                     }
                     break;
                   }
                   case 3: /* <?else> swap */
                     break;
                   case 4: /* <?endif> */
                     return 1;
                   default:
                     break;
                }
             } else {
                *bufptr++ = tempchar;
             }
          }
          break;

        case LPARSE_COMMENT: /* Liscript comment */
          {
             if ((((tempchar == '>') && (bounder == '<')) ||
                 ((tempchar == ']') && (bounder == '['))) && !escape) {
                parsemode = LPARSE_NORMAL;
                /* We just eat comment blocks */

                /* Eat trailing newline if we're the only thing on the
                   line */
                if (startline) {
                   int temp;

                   temp = fgetc(infile);
                   if (temp != EOF) {
                      tempchar = (char)temp;

                      if (tempchar != '\n') ungetc(tempchar, infile);
                   }
                }
             }
          }
          break;

        default:
          /* We Should Never Be Here */
          break;
      }
   }
   return 0;
}

int skip_until_else_endif(FILE *infile, FILE *outfile, int level)
{
   char tempchar, escape, parsemode, startline, *bufptr=NULL;
   int value, tagfmt;
   char buffer[BIG_BUF], fmtbuf[BIG_BUF];
   char bounder = '\0';

   parsemode = 0;
   escape = 0; tagfmt = 0;
   startline = 1;

   while((value = fgetc(infile)) != EOF) {
      tempchar = (char)value;
      if (tempchar == '\\') escape = 1;

      switch(parsemode) {
         case LPARSE_NORMAL: {
              if (((tempchar == '<') || (tempchar == '[')) && !escape) {
                 parsemode = LPARSE_PREBLOCK;
                 bounder = tempchar;
              } else {
                 escape = 0;
                 if (tempchar == '\n') startline = 1; else startline = 0;
              }
           }
           break;
        case LPARSE_PREBLOCK: {
             memset(buffer, 0, sizeof(buffer));
             bufptr = &buffer[0];
             if (tempchar == '$') {
                parsemode = LPARSE_SIMPLE;
                tagfmt = 0;
                memset(fmtbuf, 0, sizeof(fmtbuf));
             } else 
             if (tempchar == '#') parsemode = LPARSE_COMMENT; else
             if (tempchar == '?') parsemode = LPARSE_LOGIC; else 
             if ((tempchar == '<') || (tempchar == '[')) {
                bounder = tempchar;
             } else {
                parsemode = LPARSE_NORMAL;
                startline = 0;
             }
          }
          break;
        case LPARSE_SIMPLE: {
             if (((tempchar == '>') && (bounder == '<')) ||
                 ((tempchar == ']') && (bounder == '['))) {
                char *varfmt;

                varfmt = liscript_format_variable(fmtbuf,buffer);

                if (varfmt) {
                   free(varfmt);
                   varfmt = NULL;
                }
                parsemode = LPARSE_NORMAL;

                /* Eat trailing newline if we're the only thing on the line */
                if (startline) {
                   int temp;

                   temp = fgetc(infile);
                   if (temp != EOF) {
                      tempchar = (char)temp;

                      if (tempchar != '\n') ungetc(tempchar, infile);
                   } else ungetc(EOF,infile);
                }
             } else if (tempchar == '=') {
                *bufptr = 0; bufptr = &fmtbuf[0];
             } else {
                *bufptr++ = tempchar;
             }
          }
          break;

        case LPARSE_LOGIC: {
             char *logicparam;
             int logictype;

             if ((((tempchar == '>') && (bounder == '<')) ||
                 ((tempchar == ']') && (bounder == '[')))
                  && !escape) {

                if (startline) {
                   int temp;

                   temp = fgetc(infile);
                   if (temp != EOF) {
                      tempchar = (char)temp;

                      if (tempchar != '\n') ungetc(tempchar, infile);
                   } else ungetc(EOF,infile);
                }
                parsemode = LPARSE_NORMAL;
                logicparam = strchr(buffer,' ');
                if (logicparam) {
                   *logicparam++ = 0;
                   while(isspace((int)*logicparam)) logicparam++;
                }
                logictype = liscript_parse_logic(outfile,level,buffer,logicparam, 1, 0, 1);
                switch(logictype) {
                   case 0: /* do nothing */
                   case 1: /* do nothing */
                     break;
                   case 2: /* An if */ {
                     int res = skip_until_else_endif(infile, outfile, level);
                     if(res == 1)
                         skip_until_endif(infile, outfile, level);
                     break;
                   }
                   case 3: /* <?else> swap */
                     return 1;
                     break;
                   case 4: /* <?endif> */
                     return 2;
                   default:
                     break;
                }
             } else {
                *bufptr++ = tempchar;
             }
          }
          break;

        case LPARSE_COMMENT: /* Liscript comment */
          {
             if ((((tempchar == '>') && (bounder == '<')) ||
                 ((tempchar == ']') && (bounder == '['))) && !escape) {
                parsemode = LPARSE_NORMAL;
                /* We just eat comment blocks */

                /* Eat trailing newline if we're the only thing on the
                   line */
                if (startline) {
                   int temp;

                   temp = fgetc(infile);
                   if (temp != EOF) {
                      tempchar = (char)temp;

                      if (tempchar != '\n') ungetc(tempchar, infile);
                   }
                }
             }
          }
          break;

        default:
          /* We Should Never Be Here */
          break;
      }
   }
   return 0;
}

int skip_until_endif(FILE *infile, FILE *outfile, int level)
{
   char tempchar, escape, parsemode, startline, *bufptr=NULL;
   int value, tagfmt;
   char buffer[BIG_BUF], fmtbuf[BIG_BUF];
   char bounder = '\0';

   parsemode = 0;
   escape = 0; tagfmt = 0;
   startline = 1;

   while((value = fgetc(infile)) != EOF) {
      tempchar = (char)value;
      if (tempchar == '\\') escape = 1;

      switch(parsemode) {
         case LPARSE_NORMAL: {
              if (((tempchar == '<') || (tempchar == '[')) && !escape) {
                 parsemode = LPARSE_PREBLOCK;
                 bounder = tempchar;
              } else {
                 escape = 0;
                 if (tempchar == '\n') startline = 1; else startline = 0;
              }
           }
           break;
        case LPARSE_PREBLOCK: {
             memset(buffer, 0, sizeof(buffer));
             bufptr = &buffer[0];
             if (tempchar == '$') {
                parsemode = LPARSE_SIMPLE;
                tagfmt = 0;
                memset(fmtbuf, 0, sizeof(fmtbuf));
             } else 
             if (tempchar == '#') parsemode = LPARSE_COMMENT; else
             if (tempchar == '?') parsemode = LPARSE_LOGIC; else 
             if ((tempchar == '<') || (tempchar == '[')) {
                bounder = tempchar;
             } else {
                parsemode = LPARSE_NORMAL;
                startline = 0;
             }
          }
          break;
        case LPARSE_SIMPLE: {
             if (((tempchar == '>') && (bounder == '<')) ||
                 ((tempchar == ']') && (bounder == '['))) {
                char *varfmt;

                varfmt = liscript_format_variable(fmtbuf,buffer);

                if (varfmt) {
                   free(varfmt);
                   varfmt = NULL;
                }
                parsemode = LPARSE_NORMAL;

                /* Eat trailing newline if we're the only thing on the line */
                if (startline) {
                   int temp;

                   temp = fgetc(infile);
                   if (temp != EOF) {
                      tempchar = (char)temp;

                      if (tempchar != '\n') ungetc(tempchar, infile);
                   } else ungetc(EOF,infile);
                }
             } else if (tempchar == '=') {
                *bufptr = 0; bufptr = &fmtbuf[0];
             } else {
                *bufptr++ = tempchar;
             }
          }
          break;

        case LPARSE_LOGIC: {
             char *logicparam;
             int logictype;

             if ((((tempchar == '>') && (bounder == '<')) ||
                 ((tempchar == ']') && (bounder == '[')))
                  && !escape) {

                if (startline) {
                   int temp;

                   temp = fgetc(infile);
                   if (temp != EOF) {
                      tempchar = (char)temp;

                      if (tempchar != '\n') ungetc(tempchar, infile);
                   } else ungetc(EOF,infile);
                }
                parsemode = LPARSE_NORMAL;
                logicparam = strchr(buffer,' ');
                if (logicparam) {
                   *logicparam++ = 0;
                   while(isspace((int)*logicparam)) logicparam++;
                }
                logictype = liscript_parse_logic(outfile,level,buffer,logicparam, 1, 1, 1);
                switch(logictype) {
                   case 0: /* do nothing */
                   case 1: /* do nothing */
                     break;
                   case 2: /* An if */ {
                     int res = skip_until_else_endif(infile, outfile, level);
                     if(res == 1)
                         skip_until_endif(infile, outfile, level);
                     break;
                   }
                   case 3: /* <?else> swap */
                     break;
                   case 4: /* <?endif> */
                     return 1;
                   default:
                     break;
                }
             } else {
                *bufptr++ = tempchar;
             }
          }
          break;

        case LPARSE_COMMENT: /* Liscript comment */
          {
             if ((((tempchar == '>') && (bounder == '<')) ||
                 ((tempchar == ']') && (bounder == '['))) && !escape) {
                parsemode = LPARSE_NORMAL;
                /* We just eat comment blocks */

                /* Eat trailing newline if we're the only thing on the
                   line */
                if (startline) {
                   int temp;

                   temp = fgetc(infile);
                   if (temp != EOF) {
                      tempchar = (char)temp;

                      if (tempchar != '\n') ungetc(tempchar, infile);
                   }
                }
             }
          }
          break;

        default:
          /* We Should Never Be Here */
          break;
      }
   }
   return 0;
}

int parse_until_else_endif(FILE *infile, FILE *outfile, int level)
{
   char tempchar, escape, parsemode, startline, *bufptr=NULL;
   int value, tagfmt;
   char buffer[BIG_BUF], fmtbuf[BIG_BUF];
   char bounder = '\0';

   parsemode = 0;
   escape = 0; tagfmt = 0;
   startline = 1;

   while((value = fgetc(infile)) != EOF) {
      tempchar = (char)value;
      if (tempchar == '\\') escape = 1;

      switch(parsemode) {
         case LPARSE_NORMAL: {
              if (((tempchar == '<') || (tempchar == '[')) && !escape) {
                 parsemode = LPARSE_PREBLOCK;
                 bounder = tempchar;
              } else {
                 fputc(tempchar,outfile);
                 escape = 0;
                 if (tempchar == '\n') startline = 1; else startline = 0;
              }
           }
           break;
        case LPARSE_PREBLOCK: {
             memset(buffer, 0, sizeof(buffer));
             bufptr = &buffer[0];
             if (tempchar == '$') {
                parsemode = LPARSE_SIMPLE;
                tagfmt = 0;
                memset(fmtbuf, 0, sizeof(fmtbuf));
             } else 
             if (tempchar == '#') parsemode = LPARSE_COMMENT; else
             if (tempchar == '?') parsemode = LPARSE_LOGIC; else 
             if ((tempchar == '<') || (tempchar == '[')) {
                fputc(bounder,outfile);
                bounder = tempchar;
             } else {
                parsemode = LPARSE_NORMAL;
                fputc(bounder,outfile);
                fputc(tempchar,outfile);
                startline = 0;
             }
          }
          break;
        case LPARSE_SIMPLE: {
             if (((tempchar == '>') && (bounder == '<')) ||
                 ((tempchar == ']') && (bounder == '['))) {
                char *varfmt;

                varfmt = liscript_format_variable(fmtbuf,buffer);

                fprintf(outfile,"%s",varfmt ? varfmt : "<NoValue>");
                if (varfmt) {
                   free(varfmt);
                   varfmt = NULL;
                }
                parsemode = LPARSE_NORMAL;

                /* Eat trailing newline if we're the only thing on the line */
                if (startline) {
                   int temp;

                   temp = fgetc(infile);
                   if (temp != EOF) {
                      tempchar = (char)temp;

                      if (tempchar != '\n') ungetc(tempchar, infile);
                   } else ungetc(EOF,infile);
                }
             } else if (tempchar == '=') {
                *bufptr = 0; bufptr = &fmtbuf[0];
             } else {
                *bufptr++ = tempchar;
             }
          }
          break;

        case LPARSE_LOGIC: {
             char *logicparam;
             int logictype;

             if ((((tempchar == '>') && (bounder == '<')) ||
                 ((tempchar == ']') && (bounder == '[')))
                  && !escape) {

                if (startline) {
                   int temp;

                   temp = fgetc(infile);
                   if (temp != EOF) {
                      tempchar = (char)temp;

                      if (tempchar != '\n') ungetc(tempchar, infile);
                   } else ungetc(EOF,infile);
                }
                parsemode = LPARSE_NORMAL;
                logicparam = strchr(buffer,' ');
                if (logicparam) {
                   *logicparam++ = 0;
                   while(isspace((int)*logicparam)) logicparam++;
                }
                logictype = liscript_parse_logic(outfile,level,buffer,logicparam, 1, 0, 0);
                switch(logictype) {
                   case 0: /* do nothing */
                   case 1: /* do nothing */
                     break;
                   case 2: /* An if */ {
                     int ifresult;
                     int res;
                     if (logicparam) {
                       char resbuf[BIG_BUF];
                       char errbuf[BIG_BUF];
                       res = parse_function(logicparam, resbuf, errbuf);
                       if(res) {
                           log_printf(15, "Function '%s' eval to '%s'\n", logicparam, resbuf);
                           ifresult = !(!(atoi(resbuf)));
                       } else {
                           log_printf(5, "Error in Liscript expression '%s'\n", logicparam);
                           log_printf(5, "Error was: %s\n", errbuf);
                           ifresult = 1;
                       }
                     } else ifresult = 1;
                     if(ifresult) {
                         res = parse_until_else_endif(infile, outfile, level);
                         if(res == 1)
                             skip_until_endif(infile, outfile, level);
                     } else {
                         res = skip_until_else_endif(infile, outfile, level);
                         if(res == 1)
                             parse_until_endif(infile, outfile, level);
                     }
                     break;
                   }
                   case 3: /* <?else> swap */
                     return 1;
                     break;
                   case 4: /* <?endif> */
                     return 2;
                   default:
                     break;
                }
             } else {
                *bufptr++ = tempchar;
             }
          }
          break;

        case LPARSE_COMMENT: /* Liscript comment */
          {
             if ((((tempchar == '>') && (bounder == '<')) ||
                 ((tempchar == ']') && (bounder == '['))) && !escape) {
                parsemode = LPARSE_NORMAL;
                /* We just eat comment blocks */

                /* Eat trailing newline if we're the only thing on the
                   line */
                if (startline) {
                   int temp;

                   temp = fgetc(infile);
                   if (temp != EOF) {
                      tempchar = (char)temp;

                      if (tempchar != '\n') ungetc(tempchar, infile);
                   }
                }
             }
          }
          break;

        default:
          /* We Should Never Be Here */
          break;
      }
   }
   return 0;
}

int liscript_parse_file_lowlevel(FILE *infile, FILE *outfile, int level)
{
   char tempchar, escape, parsemode, startline, *bufptr=NULL;
   int value, tagfmt, swapped;
   char buffer[BIG_BUF], fmtbuf[BIG_BUF];
   char bounder = '\0';

   parsemode = 0;
   escape = 0; tagfmt = 0;

   swapped = 0;

   startline = 1;

   if (level > LISCRIPT_MAX_RECURSE) return 0;

   while((value = fgetc(infile)) != EOF) {

      tempchar = (char)value;

      if (tempchar == '\\') escape = 1;

      switch(parsemode) {

         case LPARSE_NORMAL: /* Normal file i/o */
           {
              if (((tempchar == '<') || (tempchar == '[')) && !escape) {
                 parsemode = LPARSE_PREBLOCK;
                 bounder = tempchar;
              } else {
                 fputc(tempchar,outfile);
                 escape = 0;
                 if (tempchar == '\n') startline = 1; else startline = 0;
              }
           }
           break;

        case LPARSE_PREBLOCK: /* Beginning of a Liscript block? */
          {
             memset(buffer, 0, sizeof(buffer));
             bufptr = &buffer[0];
             if (tempchar == '$') {
                parsemode = LPARSE_SIMPLE;
                tagfmt = 0;
                memset(fmtbuf, 0, sizeof(fmtbuf));
             } else 
             if (tempchar == '#') parsemode = LPARSE_COMMENT; else
             if (tempchar == '?') parsemode = LPARSE_LOGIC; else 
             if ((tempchar == '<') || (tempchar == '[')) {
                fputc(bounder,outfile);
                bounder = tempchar;
             } else {
                parsemode = LPARSE_NORMAL;
                fputc(bounder,outfile);
                fputc(tempchar,outfile);
                startline = 0;
             }
          }
          break;

        case LPARSE_SIMPLE: /* Liscript variable replacement */
          {
             if (((tempchar == '>') && (bounder == '<')) ||
                 ((tempchar == ']') && (bounder == '['))) {
                char *varfmt;

                varfmt = liscript_format_variable(fmtbuf,buffer);

                fprintf(outfile,"%s",varfmt ? varfmt : "<NoValue>");
                if (varfmt) {
                   free(varfmt);
                   varfmt = NULL;
                }

                parsemode = LPARSE_NORMAL;

                /* Eat trailing newline if we're the only thing on the
                   line */
                if (startline) {
                   int temp;

                   temp = fgetc(infile);
                   if (temp != EOF) {
                      tempchar = (char)temp;

                      if (tempchar != '\n') ungetc(tempchar, infile);
                   } else ungetc(EOF,infile);
                }
             } else if (tempchar == '=') {
                *bufptr = 0; bufptr = &fmtbuf[0];
             } else {
                *bufptr++ = tempchar;
             }
          }
          break;

        case LPARSE_LOGIC: /* Liscript logic block */
          {
             char *logicparam;
             int logictype;

             if ((((tempchar == '>') && (bounder == '<')) ||
                 ((tempchar == ']') && (bounder == '[')))
                  && !escape) {
                parsemode = LPARSE_NORMAL;

                if (startline) {
                   int temp;

                   temp = fgetc(infile);
                   if (temp != EOF) {
                      tempchar = (char)temp;

                      if (tempchar != '\n') ungetc(tempchar, infile);
                   } else ungetc(EOF,infile);
                }
                parsemode = LPARSE_NORMAL;
                logicparam = strchr(buffer,' ');
                if (logicparam) {
                   *logicparam++ = 0;
                   while(isspace((int)*logicparam)) logicparam++;
                }
                logictype = liscript_parse_logic(outfile,level,buffer,logicparam, 1, 0, 0);
                switch(logictype) {
                   case 0: /* do nothing */
                   case 1: /* do nothing */
                     break;
                   case 2: /* An if */ {
                     int ifresult;
                     int res;
                     if (logicparam) {
                       char resbuf[BIG_BUF];
                       char errbuf[BIG_BUF];
                       res = parse_function(logicparam, resbuf, errbuf);
                       if(res) {
                           log_printf(15, "Function '%s' eval to '%s'\n", logicparam, resbuf);
                           ifresult = !(!(atoi(resbuf)));
                       } else {
                           log_printf(5, "Error in Liscript expression '%s'\n", logicparam);
                           log_printf(5, "Error was: %s\n", errbuf);
                           ifresult = 0;
                       }
                     } else ifresult = 0;
                     if(ifresult) {
                         res = parse_until_else_endif(infile, outfile, level);
                         if(res == 1)
                             skip_until_endif(infile, outfile, level);
                     } else {
                         res = skip_until_else_endif(infile, outfile, level);
                         if(res == 1)
                             parse_until_endif(infile, outfile, level);
                     }
                     break;
                   }
                   case 3: /* <?else> swap */
                     break;
                   case 4: /* <?endif> */
                     break;
                   default:
                     break;
                }
             } else {
                *bufptr++ = tempchar;
             }
          }
          break;

        case LPARSE_COMMENT: /* Liscript comment */
          {
             if ((((tempchar == '>') && (bounder == '<')) ||
                 ((tempchar == ']') && (bounder == '['))) && !escape) {
                parsemode = LPARSE_NORMAL;
                /* We just eat comment blocks */

                /* Eat trailing newline if we're the only thing on the
                   line */
                if (startline) {
                   int temp;

                   temp = fgetc(infile);
                   if (temp != EOF) {
                      tempchar = (char)temp;

                      if (tempchar != '\n') ungetc(tempchar, infile);
                   }
                }
             }
          }
          break;

        default:
          /* We Should Never Be Here */
          break;
      }
   }

   return 1;
}

int liscript_parse_file(const char *file1, const char *file2)
{
   FILE *infile, *outfile;

   if ((infile = open_file(file1,"r")) == NULL) {
      return 0;
   }
   if ((outfile = open_file(file2,"w")) == NULL) {
      close_file(infile);
      return 0;
   }

   liscript_parse_file_lowlevel(infile,outfile,1);

   close_file(infile);
   close_file(outfile);

   return 1;
}
