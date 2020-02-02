/* Sparks' Simple Little Hack of an HTML -> Text Convertor, v0.5 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include "fileapi.h"
#include "config.h"
#include "compat.h"
#include "mystring.h"

/* Our parse modes. */
#define HTMLPARSE_NORMAL	0
#define HTMLPARSE_TAG		1
#define HTMLPARSE_EATTAG	2
#define HTMLPARSE_EXTEND        3

/* Ok, I really don't need a struct for this, but I might want more
 * data later. */
struct html_link {
  char *url;
};

/* Our list of links */
struct html_link **links = NULL;
int numlinks;

/* Forward define for html_printf */
int html_printf(char *linebuffer, const char *format, ...);

/* Add a link to the list, we only do up to 99 links. */
int add_link(char *url)
{
  struct html_link *newlink;

  if (numlinks == 99) return numlinks;

  newlink = (struct html_link *)malloc(sizeof(struct html_link));

  if (!links) {
    numlinks = 0;
    links = (struct html_link **)malloc(sizeof(struct html_link *) * 99);
  }

  numlinks++;
  newlink->url = strdup(url);
  links[numlinks - 1] = newlink;  

  return numlinks;
}

/* Clean up after ourselves. */
void nuke_links()
{
  struct html_link *templink = NULL;
  int loop;

  if (templink) return;

  for (loop = 0; loop < numlinks; loop++) {
    templink = links[loop];
    if(templink->url) free(templink->url);
    free(templink);
  }

  if(links) free(links);
  links = NULL;
}

/* Horizontal rule <HR> */
void horiz_rule(FILE *outfile, int indent)
{
  int loop;

  for (loop = 0; loop < (76 - (indent * 2)); loop++) fputc('-',outfile);
  fputc('\n',outfile);
}

/* Handle newlines */
void newline(FILE *outfile, char *linebuf, int indent, int linemode)
{
  int loop;

  switch(linemode) {
    case 1: indent++; break;
    default: /* Nothing */ break;
  }

  fprintf(outfile,"%s\n", linebuf);

 /* FC : BIG BUG !!! BIG BUG !!!
          changed memset(linebuf,0,1023) to memset(linebuf,0,80) !!!
		  as newline called (only at this time) by unhtml_file with a linebuf of 80 chars MAX !! */
  memset(linebuf, 0, 80);

  for (loop = 0; loop < indent; loop++)
    html_printf(linebuf,"  ");

}

/* Print into our line buffer */
int html_printf(char *linebuffer, const char *format, ...)
{
  va_list vargs;
  char tempbuf[1024];

  va_start(vargs,format);
  vsprintf(tempbuf,format,vargs);
  va_end(vargs);
  
  /* FC : BIG BUG !!! BIG BUG !!! changed 1024 to 79 !!!
     as html_printf called by unhtml_file and newline with a linebuffer of 80 chars MAX !! */
  strncat(linebuffer, tempbuf, 79 - strlen(linebuffer));  
  return strlen(linebuffer);
}

/* The guts of the code */
/* I really should comment this */
int unhtml_file(const char *file1, const char *file2)
{
  char buffer[1024], linebuffer[80], tempchar, *tptr, *tagptr = NULL;
  int parsemode, input, linechars, lastspace, tagmode = 0;
  int indent, linemode, badurl;
  FILE *infile, *outfile;

  if ((infile = open_file(file1,"r")) == NULL) return 0;
  if ((outfile = open_file(file2,"w")) == NULL) {
     close_file(infile);
     return 0;
  }

  links = NULL;
  numlinks = 0;

  indent = 0;

  memset(linebuffer, 0, sizeof(linebuffer));
  memset(buffer, 0, sizeof(buffer));

  linechars = 0;
  lastspace = 0;
  linemode = 0;
  badurl = 0;

  tptr = &linebuffer[0];

  parsemode = HTMLPARSE_NORMAL;
  input = 0;

  while((input = fgetc(infile)) != EOF) {
     tempchar = (char)input;

     if (tempchar == 13) tempchar = ' ';

     switch(parsemode) {

        case HTMLPARSE_NORMAL:
        case HTMLPARSE_EATTAG:
          { 
             /* Wordwrap */
             if (linechars > 76) {
                char tempbuf[1024];
                *tptr = 0;
                
                tptr = strrchr(linebuffer,' ');
                if (!tptr) tptr = strrchr(linebuffer,'-');
                if (!tptr) tptr = &tempbuf[76];

                buffer_printf(tempbuf,1023,"%s",
                  (*tptr == ' ') ? tptr + 1 : tptr);
                *tptr = 0;

                newline(outfile,&linebuffer[0],indent,linemode);
                buffer_printf(linebuffer,79,"%s",tempbuf);
                tptr = &linebuffer[strlen(linebuffer)];
                linechars = strlen(linebuffer);
                lastspace = 1;
             }
             if (tempchar == '&') {
                memset(buffer, 0, sizeof(buffer));
                tagptr = &buffer[0];
                parsemode = HTMLPARSE_EXTEND;
             } else if (tempchar == '<') {
                tagmode = 1;
                memset(buffer, 0, sizeof(buffer));
                tagptr = &buffer[0];
                parsemode = HTMLPARSE_TAG;
             } else if (parsemode != HTMLPARSE_EATTAG) {

                /* Handle whitespace */
                if (tempchar == '\n') tempchar = ' ';
                if (tempchar == '\t') tempchar = ' ';

                if ((tempchar != ' ') || !lastspace) {
                   *tptr++ = tempchar;
                   linechars++;
                   lastspace = (tempchar == ' ');
                }

             }
          }
          break;

        case HTMLPARSE_TAG:
          {
             if (tempchar == '\n') tempchar = ' ';
             if (tempchar == '\t') tempchar = ' ';
             if (tempchar != '>') {
               if (tagmode) {
                  *tagptr++ = tempchar;
                  if (tagptr >= (&buffer[1022])) tagmode = 0;
               }
             } else {
               if (tagmode) {
                 char *tagptr2;

                 tagptr2 = strchr(buffer,' ');
                 if (tagptr2) *tagptr2 = 0;

                 if (strcasecmp(buffer,"p") == 0) {
                   /* Paragraph */
                   newline(outfile,&linebuffer[0],indent,linemode);
                   newline(outfile,&linebuffer[0],indent,linemode);
                   tptr = &linebuffer[strlen(linebuffer)];
                   linechars = strlen(linebuffer); 
                   lastspace = 1;
                 } else if (strcasecmp(buffer,"/title") == 0) {
                   /* Paragraph */
                   newline(outfile,&linebuffer[0],indent,linemode);
                   newline(outfile,&linebuffer[0],indent,linemode);
                   tptr = &linebuffer[strlen(linebuffer)];
                   linechars = strlen(linebuffer); 
                   lastspace = 1;
                 } else if (strcasecmp(buffer,"br") == 0) {
                   newline(outfile,&linebuffer[0],indent,linemode);
                   tptr = &linebuffer[strlen(linebuffer)];
                   linechars = strlen(linebuffer); 
                   lastspace = 1;
                 } else if (strcasecmp(buffer,"li") == 0) {
                   newline(outfile,&linebuffer[0],indent,linemode);
                   linechars = html_printf(&linebuffer[0],"* ");
                   tptr = &linebuffer[strlen(linebuffer)];
                   if (indent) linemode = 1;
                 } else if (strcasecmp(buffer,"/li") == 0) {
                   linemode = 0;
                 } else if (strcasecmp(buffer,"ul") == 0) {
                   indent++;
                 } else if (strcasecmp(buffer,"/ul") == 0) {
                   indent--; if (indent < 0) indent = 0;
                   linemode = 0;
                   newline(outfile,&linebuffer[0],indent,linemode);
                   tptr = &linebuffer[strlen(linebuffer)];
                   linechars = strlen(linebuffer); 
                   lastspace = 1;
                 } else if (strcasecmp(buffer,"img") == 0) {
                   linechars = html_printf(&linebuffer[0],"[IMG]");
                 } else if (strcasecmp(buffer,"a") == 0) {
                   char *linkptr, *linkptr2;

                   linkptr = strcasestr(&buffer[2],"href=");
                   if (linkptr) {
                     linkptr2 = NULL;
                     linkptr += 5;
                     if (*linkptr == '\"') {
                       linkptr++;
                       linkptr2 = strchr(linkptr,'\"');
                     }

                     if (!linkptr2) linkptr2 = strchr(linkptr,' ');
                     if (linkptr2) *linkptr2 = 0;

                     add_link(linkptr);
                   } else {
                     badurl = 1;
                   }
                 } else if (strcasecmp(buffer,"/a") == 0) {
                   if (!badurl) {
                      linechars = html_printf(&linebuffer[0], "[%d]",
                       numlinks);
                      tptr = &linebuffer[strlen(linebuffer)];
                   }
                   badurl = 0;
                 } else if (strcasecmp(buffer,"hr") == 0) {
                   newline(outfile,&linebuffer[0],indent,linemode);
                   horiz_rule(outfile,indent);
                   tptr = &linebuffer[strlen(linebuffer)];
                 }

                 parsemode = HTMLPARSE_NORMAL;
               }
             }
          }
          break;

        case HTMLPARSE_EXTEND:
          {
             if (tempchar == '\n') tempchar = ' ';
             if (isspace((int)tempchar)) tempchar = ' ';
             if (tempchar == ' ') {
                linechars = html_printf(&linebuffer[0],"&%s ",buffer);
                tptr = &linebuffer[strlen(linebuffer)];
                parsemode = HTMLPARSE_NORMAL;
             } else if (tempchar == ';') {
                char newchar;

                newchar = 0;

                if (strcasecmp(buffer,"quot") == 0) newchar = '\"'; else
                if (strcasecmp(buffer,"amp") == 0) newchar = '&'; else
                if (strcasecmp(buffer,"lt") == 0) newchar = '<'; else
                if (strcasecmp(buffer,"gt") == 0) newchar = '>'; else
                if (strcasecmp(buffer,"nbsp") == 0) { 
                  newchar = ' '; lastspace = 0;
                } else if (strcasecmp(buffer,"reg") == 0) {
                  newchar = ')'; 
                  linechars = html_printf(&linebuffer[0],"(R");
                  tptr = &linebuffer[strlen(linebuffer)];
                } else if (strcasecmp(buffer,"copy") == 0) {
                  newchar = ')'; 
                  linechars = html_printf(&linebuffer[0],"(C");
                  tptr = &linebuffer[strlen(linebuffer)];
                } else if (strcasecmp(buffer,"tm") == 0) {
                  newchar = ']'; 
                  linechars = html_printf(&linebuffer[0],"[TM");
                  tptr = &linebuffer[strlen(linebuffer)];
                }

                if (newchar) { 
                  *tptr++ = newchar; 
                  linechars++;
                } else {
                  linechars = html_printf(&linebuffer[0],"&%s;", buffer);
                  tptr = &linebuffer[strlen(linebuffer)];
                }
                parsemode = HTMLPARSE_NORMAL;
             } else {
               if (tagptr < buffer + sizeof(buffer) - 1)
                 *tagptr++ = tempchar;
             }
          }
          break;
     }
  }

  newline(outfile,&linebuffer[0],indent,linemode);

  if (links) {
     int loop;

     fprintf(outfile,"\n--- Links ---\n");

     for (loop = 0; loop < numlinks; loop++) {
        fprintf(outfile," %3d %s\n", loop + 1, links[loop]->url);
     }
  }

  close_file(infile);
  close_file(outfile);

  nuke_links();

  return 1;
}
