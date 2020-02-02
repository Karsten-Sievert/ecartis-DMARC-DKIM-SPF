#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "compat.h"
#include "config.h"
#include "core.h"
#include "fileapi.h"
#include "variables.h"
#include "regexp.h"
#include "unmime.h"
#include "mystring.h"
#include "codes.h"
#include "rfc2047.h"
#include "moderate.h"

struct mime_handler *mime_handlers;

int mimecounter;

/* Eat allocated memory.  *Burp* */
void mime_eat_header(struct mime_header *header)
{
   struct mime_field *field;
   int loop;

   if (!header) return;

   for (loop = 0; loop < header->numfields; loop++) {
      field = header->fields[loop];

      if (field->params) free((char *)field->params);
      if (field->value) free((char *)field->value);
      if (field->name) free((char *)field->name);
      free(field);
   }
   free(header->fields);
   header->fields = NULL;
}

struct mime_field *mime_getfield(struct mime_header *header, const char
                                 *fieldname)
{
   struct mime_field *temp = NULL;
   int loop, done;
   if (header == NULL)
       return NULL;

   loop = done = 0;

   while ((loop < header->numfields) && !done) {
       temp = header->fields[loop];

       if (strcasecmp(temp->name,fieldname) == 0) done = 1;
       loop++;
   }

   if (!done) return NULL; else return temp;
}

/* YOU WILL **NOT** WANT TO FREE THIS RETURN RESULT */
const char *mime_fieldval(struct mime_header *header, const char *fieldname)
{
   struct mime_field *field;

   if (!header) return NULL;

   field = mime_getfield(header,fieldname);

   if (!field) return NULL; else return field->value;
}

/* YOU WILL NEED TO FREE THIS RETURN RESULT */
const char *mime_parameter(struct mime_header *header, const char *fieldname,
                              const char *paramname)
{
   char temp[BIG_BUF], temp2[SMALL_BUF];
   char *tptr, *tptr2;
   int done, eattrail, length; 
   struct mime_field *field;
   int escape;

   field = mime_getfield(header,fieldname);
   if (!field) return NULL;
   if (!field->params) return NULL;

   /* Get some sane temporary workspace */
   buffer_printf(temp, sizeof(temp) - 1, "%s", field->params);

   tptr = strtok(temp,";");

   done = 0;

   while (tptr && !done) {
      /* Clear our temporary buffer */
      memset(temp2, 0, sizeof(temp2));

      tptr2 = &temp2[0];

      while (*tptr && (*tptr != '=') && tptr2 < temp2 + sizeof(temp2) - 1) {
         if (!isspace((int)*tptr)) *tptr2++ = *tptr;
         tptr++;
      }

      /* Is this our parameter? */
      if (strcasecmp(temp2,paramname) == 0) {
         char *endspace = NULL ;
         done = 1; eattrail = 1;
         tptr++;

         escape = 0;
       
         /* Clear our temporary buffer */
         memset(temp2, 0, sizeof(temp2));

         tptr2 = &temp2[0];

         while (*tptr && (*tptr != ';') && tptr2 < temp2 + sizeof(temp2) - 1) {
            if ( (!escape) && isspace((int)*tptr) ) {
               if (!eattrail) {
                  /* We store the position to remove end spaces */
                  if (!endspace) endspace = tptr2 ;
                  *tptr2++ = *tptr;
               }
            } else {
               eattrail = 0;
               endspace = NULL ;

               if ( !(*tptr == '\"' || *tptr == '\\') || escape) {
                  *tptr2++ = *tptr;
                  escape = 0;
               }
               else { /* this is to avoid a '\' to escape itself */
                  if (*tptr == '\\') escape = 1;
               }
            }

            tptr++;
         }

         if (endspace) *endspace = 0 ;

      } else tptr = strtok(NULL,";");
   }
   
   if (!done) return NULL;

   length = strlen(temp2);

   tptr = (char *)malloc(length + 1);
   memset(tptr,0,length + 1);
   buffer_printf(tptr,length + 1,"%s",temp2);

   return tptr;   
}

struct mime_header *mime_newheader(struct mime_field *field)
{
   struct mime_header *header;

   if (!field) return NULL;

   header = (struct mime_header *)malloc(sizeof(struct mime_header));
   header->numfields = 1;
   header->fields = (struct mime_field **)malloc(sizeof(struct mime_field *));   
   header->fields[0] = field;

   return header;
}

struct mime_header * mime_addheader(struct mime_header *header, 
                                    struct mime_field *field)
{
   if (!header) return mime_newheader(field);
   if (!field) return header;

   header->numfields++;

   header->fields = (struct mime_field **)realloc(header->fields,
      header->numfields * sizeof(struct mime_field *));

   header->fields[header->numfields - 1] = field;

   return header;
}

struct mime_field *mime_makefield(FILE *instream, const char *line,
                                  int *readlast)
{
   char tempbuf[BIG_BUF];
   char finalbuf[HUGE_BUF];
   int done;
   char *name, *value, *params;
   char *tptr, *tptr2;
   struct mime_field *field;
   char *backptr;

   *readlast = 0;

   memset(tempbuf, 0, sizeof(tempbuf));
   memset(finalbuf, 0, sizeof(finalbuf));

   /* The use of BIG_BUF here is deliberate */
   if (line) {
      buffer_printf(finalbuf, sizeof(finalbuf) - 1, "%s", line);
      if (finalbuf[strlen(finalbuf) - 1] == '\n')
          finalbuf[strlen(finalbuf) - 1] = 0;
   }

   done = 0;

   /* Walk backwards, to catch MIME with trailing spaces */
   backptr = &(finalbuf[strlen(finalbuf) - 1]);
   while ((*backptr) && isspace((int)(*backptr))) backptr--;
   if (*backptr != ';') done = 1;

   if (!done) {
      backptr++;
      *backptr = 0;
   }

   done = 0;

   if ((finalbuf[strlen(finalbuf) - 1] == ';') || !finalbuf[0]) {
      while (!done) {
         if (!read_file(tempbuf, sizeof(tempbuf), instream)) return NULL;

         if (tempbuf[0] == '\n') { done = 1; *readlast = 1; return NULL; }

         if (tempbuf[strlen(tempbuf) - 1] == '\n')
             tempbuf[strlen(tempbuf) - 1] = 0;

         if ((tempbuf[0] != 0) && (tempbuf[0] != '\n')) {
            stringcat(finalbuf, tempbuf);

            /* Walk backwards, to catch MIME with trailing spaces */
            backptr = &(tempbuf[strlen(tempbuf) - 1]);
            while ((*backptr) && isspace((int)(*backptr))) backptr--;
            if (*backptr != ';') done = 1;

         } else {
            if (tempbuf[0] == '\n') { done = 1; *readlast = 1; }
         }
      }
   }

   if (!finalbuf[0]) return NULL;

   tptr = strchr(finalbuf,':');
   if (!tptr) return NULL;

   *tptr++ = 0;

   name = (char *)malloc(strlen(finalbuf) + 1);
   buffer_printf(name,strlen(finalbuf) + 1,"%s",finalbuf);

   /* Eat whitespace */
   while (isspace((int)*tptr)) tptr++;

   tptr2 = strchr(tptr,';');
   if (tptr2) *tptr2++ = 0;
   
   value = (char *)malloc(strlen(tptr) + 1);
   buffer_printf(value,strlen(tptr) + 1,"%s",tptr);

   params = NULL;

   if (tptr2) {
      tptr = tptr2;
      while (isspace((int)*tptr)) tptr++;

      params = (char *)malloc(strlen(tptr) + 1);
      buffer_printf(params,strlen(tptr) + 1,"%s",tptr);
   }

   field = (struct mime_field *)malloc(sizeof(struct mime_field));
   field->name = name;
   field->value = value;
   field->params = params;

   return field;
}

void new_mime_handlers()
{
   mime_handlers = NULL;
   mimecounter = 0;
}

void nuke_mime_handlers()
{
   struct mime_handler *handler, *temp;

   handler = mime_handlers;
   
   while (handler) {
      temp = handler->next;
      if(handler->mimetype) free(handler->mimetype);
      free(handler);
      handler = temp;
   }
   mime_handlers = NULL;
}

void add_mime_handler(const char *mimetype, int priority, MimeFn function)
{
   struct mime_handler *handler, **temp;

   handler = (struct mime_handler *)malloc(sizeof(struct mime_handler));
   handler->mimetype = lowerstr(mimetype);
   handler->handler = function;
   handler->priority = priority;
   handler->next = mime_handlers;

   temp = &mime_handlers;

   while(*temp && ((*temp)->priority <= priority))
     temp = &((*temp)->next);

   handler->next = *temp;
   *temp = handler;
}

struct mime_handler *get_mime_handler(const char *mimetype)
{
   struct mime_handler *handler;
   char *tempstring;

   if (!mimetype) return NULL;

   tempstring = lowerstr(mimetype);

   handler = mime_handlers;

   while (handler) {
      regexp *treg;

      treg = regcomp((char *)handler->mimetype);
      if (treg) {
         if (regexec(treg, tempstring)) {
            free(treg);  free(tempstring); return handler;
         }
         free(treg);
      }
      handler = handler->next;
   }

   free(tempstring);

   return NULL;
}

MIME_HANDLER(mimehandle_multipart_default)
{
   FILE *infile, *outfile, *outfile2;
   char templine[BIG_BUF];
   const char *bound;
   struct mime_field *field;
   int done=0, readlast, firstcont, loop2;
   int firstfile;
   char overridetype[BIG_BUF];
   int override;

   if ((infile = open_file(mimefile,"r")) == NULL) return MIME_HANDLE_FAIL;

   bound = NULL;

   loop2 = 0;

   firstfile = 1;

   bound = mime_parameter(header,"content-type","boundary");

   firstcont = 1;

   while (loop2 ? !done : read_file(templine, sizeof(templine), infile) && !done) {
      struct mime_header *subheader;
      const char *temp;

      subheader = NULL;

      readlast = 0; override = 0;

      loop2 = 1;

      if (strlen(templine) > 3 ? strncmp(&templine[2],bound,strlen(bound)) == 0 : 0)
         loop2 = 0;
         
      while(!readlast) {
         field = mime_makefield(infile,loop2 ? templine : NULL,&readlast);
         loop2 = 0;
         if (field) {
            subheader = mime_addheader(subheader,field);
         } else readlast = 1;

         /* Handle GNUs' odd MIME implementation where it's valid
          * for the first attachment to have no content-type and thus
          * count as text/plain... fooie. 
          *
          * This is pretty hackish, but, eh... */
         if (readlast && !subheader && firstfile) {

            field = (struct mime_field *)malloc(sizeof(struct mime_field));

            field->name = strdup("Content-type");
            field->value = strdup("text/plain");
            field->params = NULL;

            subheader = mime_addheader(subheader,field);
         }
      }

      loop2 = 1;

      /* This links in with PantoMIME, if installed */
      if (get_bool("unmime-forceweb") && !firstfile && !get_bool("unmime-moderate-mode")) {
          override = 1;
          buffer_printf(overridetype, sizeof(overridetype) - 1, "ecartis-internal/pantomime");
      }

      /* Ok, now, yes this IS a cheap hack, forcing ALL attachments,
         no matter the type, to be stripped. */
      if (strcmp(get_string("mode"), "bounce") &&
			  !get_bool("unmime-moderate-mode") &&
			  get_bool("rabid-mime") && !firstfile) {
          override = 1;
          buffer_printf(overridetype, sizeof(overridetype) - 1, "ecartis-internal/rabid");
      }

      /* Here's another cheap hack.  This prevents someone from having
         something that strips message/rfc822 down to just a few headers,
         and thus having it break something. */
      if (get_bool("unmime-moderate-mode") &&
           subheader ? strcasecmp(mime_fieldval(subheader,"content-type"),"message/rfc822") == 0 : 0) {
          override = 1;
          buffer_printf(overridetype, sizeof(overridetype) - 1,
            "ecartis-internal/moderate");
      }

      firstfile = 0;

      temp = mime_fieldval(subheader,"content-type");
      if (temp) {
         char subfilename[BIG_BUF];
         int coding;
         int donechunk;
         int haveread, havewritten;
         struct mime_handler *handler;

         coding = 0;

         donechunk = 0;

         havewritten = 0;

         if (!override) {
            handler = get_mime_handler(temp);
         } else {
            handler = get_mime_handler(overridetype);
         }

         /* Hey, look!  The EASY way to do multipart/alternative <grin> */
         if (strcasecmp(mime_fieldval(header,"content-type"),"multipart/alternative") == 0) {
            if (strcasecmp(mime_fieldval(subheader,"content-type"),"text/plain") != 0)
               handler = NULL;
         }

         if (!handler) {
            /* What do we do here? */

            while(read_file(templine, sizeof(templine), infile) && !donechunk) {
               if (strlen(templine) > 3 ? strncasecmp(&templine[2],
                  bound, strlen(bound)) == 0 : 0) {
                  donechunk = 1; haveread = 1; havewritten = 0;
               }
            }

            if (!donechunk) done = 1;

         } else { 
            buffer_printf(subfilename, sizeof(subfilename) - 1, "%s.%d", mimefile, mimecounter++);

            outfile = open_file(subfilename,"w");

            temp = mime_fieldval(subheader,"content-transfer-encoding");

            if (temp) {
               if (strcasecmp(temp,"base64") == 0) coding = 1; else
               if (strcasecmp(temp,"quoted-printable") == 0) coding = 2;
                 else coding = 0;
            }

            haveread = 0;         

            switch(coding) {
               case 0: 
                 while(read_file(templine, sizeof(templine), infile) && !donechunk) {
                    if (strlen(templine) > 3 ? 
                        (strncmp(&templine[2],bound,strlen(bound)) == 0)
                        : 0) {
                          donechunk = 1;
                    } else write_file(outfile,"%s",templine);
                    haveread = 1;
                 }
                 close_file(outfile);
                 handler->handler(subheader,subfilename);
                 havewritten = 1;
                 break;
               case 1: 
                 {
                    char **boundaries;
                    int boundcount;
                    char tbuf[BIG_BUF];

                    boundaries = (char **)malloc(2 * sizeof(char *));
                    buffer_printf(tbuf, sizeof(tbuf) - 1, "--%s", bound);
                    boundaries[0] = &tbuf[0];
                    boundcount = 1;

                    from64(infile,outfile,boundaries,&boundcount);
                    close_file(outfile);
                    handler->handler(subheader,subfilename);
                    loop2 = 0;
                    havewritten = 1;
                 }
                 break;
               case 2: 
                 {
                    char **boundaries;
                    int boundcount;
                    char tbuf[BIG_BUF];

                    boundaries = (char **)malloc(2 * sizeof(char *));
                    buffer_printf(tbuf, sizeof(tbuf) - 1, "--%s", bound);
                    boundaries[0] = &tbuf[0];
                    boundcount = 1;

                    fromqp(infile,outfile,boundaries,&boundcount);
                    close_file(outfile);
                    handler->handler(subheader,subfilename);
                    havewritten = 1;
                    loop2 = 0;
                 }
                 break;
               case 3: 
                 close_file(outfile);
                 break;
            }

            if (havewritten) {
               FILE *infile2;
               char tempbuffer[BIG_BUF];
               char tempfilename[BIG_BUF];

               buffer_printf(tempfilename, sizeof(tempfilename) - 1, "%s.output", mimefile);

               if ((outfile2 = open_file(tempfilename,"a")) == NULL) {
                  /* bad stuff happens here */

                  close_file(infile);
                  unlink_file(subfilename);
                  return MIME_HANDLE_FAIL;
               }

               if ((infile2 = open_file(subfilename,"r")) == NULL) {
                  write_file(outfile2,"-- Unable to access decoded file.\n");
                  close_file(outfile2);
               } else {
                  while (read_file(tempbuffer, sizeof(tempbuffer), infile2)) {
                     write_file(outfile2,"%s",tempbuffer);
                  }
                  write_file(outfile2,"\n");
                  close_file(infile2);
                  close_file(outfile2);
                  unlink_file(subfilename);
               }
            } else {
               unlink_file(subfilename);
            }
         }
      } else {
         done = 1;
      }
   }

   close_file(infile);
   if(bound) free((char *)bound);

   buffer_printf(templine, sizeof(templine) - 1, "%s.output", mimefile);
   replace_file(templine,mimefile);

   return MIME_HANDLE_OK;   
}

MIME_HANDLER(mimehandle_text)
{
   FILE *tempfile, *infile;
   char tempfilename[BIG_BUF], tempbuf[BIG_BUF];
   const char *temp;

   buffer_printf(tempfilename, sizeof(tempfilename) - 1, "%s.altertext", mimefile);
   if ((infile = open_file(mimefile,"r")) == NULL) {
      return MIME_HANDLE_FAIL;
   }
   if ((tempfile = open_file(tempfilename,"w")) == NULL) {
      close_file(infile);
      return MIME_HANDLE_FAIL;
   }

   if (!get_bool("unmime-first-level")) {
      write_file(tempfile,"-- Attached file included as plaintext by %s --\n",
                 SERVICE_NAME_MC);
    
      temp = mime_parameter(header,"content-type","name");
      if (temp) {
         write_file(tempfile,"-- File: %s\n", temp);
         free((char *)temp);
      }
      temp = mime_fieldval(header,"content-description");
      if (temp) {
         if (*temp) write_file(tempfile,"-- Desc: %s\n", temp);
      }
      write_file(tempfile,"\n");
   } else clean_var("unmime-first-level", VAR_TEMP);

   while(read_file(tempbuf, sizeof(tempbuf), infile)) {
      write_file(tempfile,"%s",tempbuf);
   }
   close_file(infile);
   close_file(tempfile);

   replace_file(tempfilename,mimefile);

   return MIME_HANDLE_OK;
}

MIME_HANDLER(mimehandle_moderate)
{
   clean_var("unmime-moderate-mode",VAR_GLOBAL);

   do_moderate(mimefile);
   unlink_file(mimefile);

   set_var("unmime-moderate-mode","yes",VAR_GLOBAL);

   return MIME_HANDLE_OK;
}


MIME_HANDLER(mimehandle_unknown)
{
   FILE *tempfile, *infile;
   char tempfilename[BIG_BUF];
   const char *temp;

   clean_var("unmime-first-level", VAR_TEMP);

   buffer_printf(tempfilename, sizeof(tempfilename) - 1, "%s.alterimg", mimefile);
   if ((infile = open_file(mimefile,"r")) == NULL) {
      return MIME_HANDLE_FAIL;
   }
   if ((tempfile = open_file(tempfilename,"w")) == NULL) {
      close_file(infile);
      return MIME_HANDLE_FAIL;
   }

   if (!get_bool("unmime-quiet")) {
      write_file(tempfile,"-- Binary/unsupported file stripped by %s --\n",
                 SERVICE_NAME_MC);
   
      temp = mime_fieldval(header,"content-type");
      if (temp) {
         write_file(tempfile,"-- Type: %s\n", temp);
      }

      temp = mime_parameter(header,"content-type","name");
      if (temp) {
         write_file(tempfile,"-- File: %s\n", temp); 
         free((char *)temp);
      }

      temp = mime_fieldval(header,"content-description");
      if (temp) {
         if (*temp) write_file(tempfile,"-- Desc: %s\n", temp);
      }

      write_file(tempfile,"\n");
   }

   close_file(infile);
   close_file(tempfile);

   replace_file(tempfilename,mimefile);

   return MIME_HANDLE_OK;
}

MIME_HANDLER(mimehandle_rabid)
{
   FILE *tempfile, *infile;
   char tempfilename[BIG_BUF];
   const char *temp;

   clean_var("unmime-first-level", VAR_TEMP);

   buffer_printf(tempfilename, sizeof(tempfilename) - 1, "%s.alterimg", mimefile);
   if ((infile = open_file(mimefile,"r")) == NULL) {
      return MIME_HANDLE_FAIL;
   }
   if ((tempfile = open_file(tempfilename,"w")) == NULL) {
      close_file(infile);
      return MIME_HANDLE_FAIL;
   }

   if (!get_bool("unmime-quiet")) {
      write_file(tempfile,"-- No attachments (even text) are allowed --\n");
   
      temp = mime_fieldval(header,"content-type");
      if (temp) {
         write_file(tempfile,"-- Type: %s\n", temp);
      }

      temp = mime_parameter(header,"content-type","name");
      if (temp) {
         write_file(tempfile,"-- File: %s\n", temp); 
         free((char *)temp);
      }

      temp = mime_fieldval(header,"content-description");
      if (temp) {
         if (*temp) write_file(tempfile,"-- Desc: %s\n", temp);
      }

      write_file(tempfile,"\n");
   }

   close_file(infile);
   close_file(tempfile);

   replace_file(tempfilename,mimefile);

   return MIME_HANDLE_OK;
}


void unmime_file(const char *file1, const char *file2)
{
   FILE *infile, *infile2, *outfile;
   char templine[BIG_BUF], tempfile[BIG_BUF];
   const char *bound, *charsetp;
   char charset[SMALL_BUF];
   struct mime_header *header;
   struct mime_field *field;
   struct mime_handler *handler;
   int done=0, readlast, firstcont;
   int inbody;
   int eatline, encoding;

   clean_var("just-unmimed", VAR_GLOBAL);
   
   if (get_bool("unmimed-file")) return;

   set_var("unmime-first-level","yes",VAR_TEMP);

   if ((infile = open_file(file1,"r")) == NULL) return;

   buffer_printf(tempfile, sizeof(tempfile) - 1, "%s.demime", file2);

   header = NULL;
   bound = NULL;
   firstcont = 1;

   inbody = 0;
   outfile = NULL;

   readlast = 0;

   while (read_file(templine, sizeof(templine), infile) && !done) {

      if ((strncasecmp("Content-type:",templine,13) == 0) && firstcont && !inbody) {
         field = mime_makefield(infile, templine,&readlast);
         if (field) header = mime_addheader(header,field);
         if (readlast) inbody = 1;

         bound = mime_parameter(header,"content-type","boundary");
         firstcont = 0;
      }

      if ((strncasecmp(templine,"Content-transfer-encoding:",26) == 0) &&
            !inbody) {
         field = mime_makefield(infile, templine,&readlast);
         if (field) header = mime_addheader(header,field);
         if (readlast) inbody = 1;
      }
      
      if ((templine[0] == '\n') && !inbody) { 

         /* If header is NULL, there was no Content-type line.  If there
          * was no Content-type line, this isn't a MIME message. */
         if (!header) {
            close_file(infile); 
            return; 
         } 

         /* For moderation - the logic behind this is that you might
          * conceivably have a text/plain handler and we do NOT want
          * it called during moderation. -- likewise on message/rfc822 */
         if (get_bool("unmime-moderate-mode")) {
            const char *content = mime_fieldval(header,"content-type");

            if (strcasecmp(content,"text/plain") == 0 ||
                strcasecmp(content,"message/rfc822") == 0) {
               /* let's clean up our header and bail */
               mime_eat_header(header);
               close_file(infile);
               return;
            }
         }

         inbody = 1; 
         firstcont = 1; 
      }
      if (inbody) {
         const char *content = mime_fieldval(header,"content-type");

         if (firstcont) {
            if ((strlen(templine) > 3 && bound) ? strncmp(&templine[2],bound,strlen(bound)) == 0 : 0) {
               firstcont = 0;
               outfile = open_file(tempfile,"w");              
            } else if (content) {
               if (!strncasecmp(content,"multipart",9) == 0) {
                  firstcont = 0;
                  outfile = open_file(tempfile,"w");              
               }
            }
         }
         if (outfile && inbody && !firstcont)
            write_file(outfile,"%s",templine);
      }
   }

   close_file(infile);
   if(outfile) close_file(outfile);

   memset(charset, 0, sizeof(charset));

   charsetp = mime_parameter(header,"content-type","charset");
   if (charsetp) {
      buffer_printf(charset, sizeof(charset) - 1, "%s", charsetp);
      free((char *)charsetp);
   }

   handler = get_mime_handler(mime_fieldval(header,"content-type"));
   if (handler) {
      handler->handler(header,tempfile);
   }   

   mime_eat_header(header);
   free(header);
   header = NULL;
   if(bound) free((char *)bound);

   if (get_bool("unmime-moderate-mode")) {
      set_var("unmimed-file", "yes", VAR_GLOBAL);
      set_var("just-unmimed", "yes", VAR_GLOBAL);
   }

   if ((infile = open_file(file1,"r")) == NULL) {
      unlink_file(tempfile);
      return;
   }

   if ((infile2 = open_file(tempfile,"r")) == NULL) {
      close_file(infile);
      unlink_file(tempfile);
      return;
   }

   if ((outfile = open_file(file2,"w")) == NULL) {
      close_file(infile);
      unlink_file(tempfile);      
      return;
   }

   done = 0; eatline = 0;
   encoding = 0;

   while(read_file(templine, sizeof(templine), infile) && !done) {
      if (templine[0] == '\n') done = 1;

      /* Eat the transfer-encoding line */
      else if (strncasecmp(templine,"Content-transfer-encoding:",26) == 0) {
         write_file(outfile,"Content-Transfer-Encoding: 8bit\n");
         encoding = 1;
      }

      /* Handle content-type line */
      else if (strncasecmp(templine,"Content-type:",13) == 0) {
         write_file(outfile,"Content-type: text/plain");

         /* Preserve character set if explicit */
         if (charset[0]) write_file(outfile, "; charset=%s", charset);

         write_file(outfile,"\n");
         eatline = 1;
      } else if (!isspace((int)templine[0]) || !eatline) {
         eatline = 0;
         write_file(outfile,"%s",templine);
      }
   }
   close_file(infile);

   if (!encoding) {
      write_file(outfile,"Content-Transfer-Encoding: 8bit\n");
   }

   write_file(outfile,"\n");

   done = 0;

   while (read_file(templine, sizeof(templine), infile2)) {
      /* well, this is an UGLY hack */
      if (templine[0] == '\n' && !done) { done++; continue; }
      write_file(outfile,"%s",templine);
   }

   close_file(infile2);
   close_file(outfile);
   unlink_file(tempfile);

   set_var("unmimed-file", "yes", VAR_GLOBAL);
   set_var("just-unmimed", "yes", VAR_GLOBAL);

   return;   
}

void unquote_string(const char *orig, char *dest, int len)
{
    if(!orig || !dest || ((unsigned int)len < strlen(orig))) {
       if(dest && orig) {
         /* we need to have something in the dest, so */
         strncpy(dest, orig, len - 1);
       } else if(dest) {
         strncpy(dest, "", len - 1);
       }
    }
    rfc2047_decode(orig, dest, len);
}

void requote_string(const char *orig, char *dest, int len)
{
   toqps(orig, dest, len);
}

void unquote_file(const char *file1, const char *file2)
{
   char tbuf[BIG_BUF];
   char tempfilename[SMALL_BUF];
   int doneheader;
   int done = 0;
   FILE *infile, *outfile;

   if (get_bool("humanize-mime")) return;

   clean_var("just-unquoted", VAR_GLOBAL);

   if(get_bool("unquoted-file")) return;

   if (!exists_file(file1)) return;

   if ((infile = open_file(file1,"r")) == NULL)
      return;

   buffer_printf(tempfilename, sizeof(tempfilename) - 1, "%s.tmpquote", file2);

   if ((outfile = open_file(file2,"w")) == NULL) {
      close_file(infile);
      return;
   }

   doneheader = 0;

   while(!done && read_file(tbuf, sizeof(tbuf), infile)) {
      if(!doneheader &&
          (strncasecmp(tbuf,"Content-Transfer-Encoding:",26) == 0)) {
         char *tptr = &tbuf[27];
         if(strncasecmp(tptr, "quoted-printable", 16) == 0) {
            write_file(outfile,"Content-Transfer-Encoding: 8bit\n");
            write_file(outfile,"X-MIME-Autoconverted: from quoted-printable to 8bit by %s\n", SERVICE_NAME_MC);
             fromqp(infile, outfile, NULL, NULL);
             done = 1;
         }
      } 
      else if (strncasecmp(tbuf,"Content-type:",13) == 0) {
         int readlast = 0;
         struct mime_field *field;         
         struct mime_header *header;

         field = mime_makefield(infile, tbuf,&readlast);
         if (field) {
            const char *encoding, *charset;

            header = NULL;

            header = mime_addheader(header,field);            
	    
	    charset = mime_parameter(header, "content-type", "charset") ;
	    /* We have a charset, let's register it... */
          if (charset) {
            if (!get_var("headers-charset")) {
              if (!get_var("headers-charset-frombody")) {
                log_printf(5, "Setting frombody charset : %s\n", charset);
                set_var("headers-charset-frombody", charset, VAR_GLOBAL);
              }
            }
          }
	    
            encoding = mime_parameter(header, "content-type", "content-encoding");
            if (encoding) {
	       /* We got an encoding, let's deal about it */
               if (strncasecmp(encoding, "quoted-printable", 16) == 0) {
		  /* What about un-quoted-printabling ? */
		  
		  /* this is a bad hack as we should try to rebuild the
		   * Content-Type field by modifying the content-encoding instead
		   * of making a new one from scratch
		   * This is mostly a quick fix and I am in lazy mode */
		  if (charset) {
                     write_file(outfile,"Content-type: %s; charset=\"%s\"\n", mime_fieldval(header, "content-type"), charset) ;
		  }
		  else {
		     write_file(outfile,"Content-type: %s\n", mime_fieldval(header, "content-type"));
		  }

                  write_file(outfile,"Content-Transfer-Encoding: 8bit\n");
                  write_file(outfile,"X-MIME-Autoconverted: from quoted-printable to 8bit by %s\n", SERVICE_NAME_MC);
                  fromqp(infile, outfile, NULL, NULL);
                  done = 1;
               } else {
		  /* I am lazy, I already have the field Content-Type */
		  if (field->params) {
		     write_file(outfile, "Content-Type: %s; %s\n", field->value, field->params) ;
		  }
		  else {
		     write_file(outfile, "Content-Type: %s\n", field->value) ;
		  }
               }
            } else {
	       /* Still lazy... :-) */
	       if (field->params) {
		  write_file(outfile, "Content-Type: %s; %s\n", field->value, field->params) ;
	       }
	       else {
		  write_file(outfile, "Content-Type: %s\n", field->value) ;
	       }
            }
         }

      }
      else {
         write_file(outfile, "%s", tbuf);
         if (tbuf[0] == '\n') {
             doneheader = 1;
         }
      }
   }
   close_file(infile);
   close_file(outfile);

   set_var("just-unquoted","yes",VAR_GLOBAL);
   set_var("unquoted-file","yes",VAR_GLOBAL);
}
