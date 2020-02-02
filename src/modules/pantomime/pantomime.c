#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "pantomime.h"

MIME_HANDLER(mimehandle_html)
{
   FILE *outfile;
   char filename[BIG_BUF];

   if (strcasecmp(LMAPI->mime_fieldval(header,"Content-type"),
       "text/html") != 0) {
      /* We should NOT have been here */
      return MIME_HANDLE_FAIL;      
   }

   if (!LMAPI->get_bool("humanize-html")) {
      if (!LMAPI->get_bool("unmime-first-level")) {
         LMAPI->unlink_file(mimefile);
         if (!LMAPI->get_bool("unmime-quiet")) {
            if ((outfile = LMAPI->open_file(mimefile,"w")) != NULL) {
               LMAPI->write_file(outfile,"-- HTML file stripped by %s --\n\n",
                 SERVICE_NAME_MC);
               LMAPI->close_file(outfile);
            }
         }
         return MIME_HANDLE_OK;
      }
   }

   LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.dehtml", mimefile);
   if (!LMAPI->unhtml_file(mimefile,filename)) {
      LMAPI->unlink_file(filename);
      if (!LMAPI->get_bool("unmime-quiet")) {
         if ((outfile = LMAPI->open_file(mimefile,"w")) != NULL) {
            LMAPI->write_file(outfile,"-- Unable to decode HTML file!! --\n\n");
            LMAPI->close_file(outfile);
         } else LMAPI->unlink_file(mimefile);
      } else LMAPI->unlink_file(mimefile);

      return MIME_HANDLE_FAIL;
   }

   if ((outfile = LMAPI->open_file(mimefile,"w")) != NULL) {
      FILE *infile;
      char buffer[BIG_BUF];

      if ((infile = LMAPI->open_file(filename,"r")) == NULL) {
         LMAPI->close_file(outfile);
         LMAPI->unlink_file(mimefile);
         return MIME_HANDLE_FAIL;
      }

      if (!LMAPI->get_bool("unmime-quiet") && !LMAPI->get_bool("unmime-first-level")) {
         const char *temp;

         LMAPI->write_file(outfile,"-- HTML Attachment decoded to text by %s --\n", 
           SERVICE_NAME_MC);

         temp = LMAPI->mime_parameter(header,"content-type","name");
         if (temp) {
            LMAPI->write_file(outfile,"-- File: %s\n", temp);
            free((char *)temp);
         }
         temp = LMAPI->mime_fieldval(header,"content-description");
         if (temp) {
            if (*temp)
              LMAPI->write_file(outfile,"-- Desc: %s\n", temp);
         }

         LMAPI->write_file(outfile,"\n");
      }
      while(LMAPI->read_file(buffer, sizeof(buffer), infile)) {
         LMAPI->write_file(outfile,"%s",buffer);
      }
      LMAPI->close_file(outfile);
      LMAPI->close_file(infile);
      LMAPI->unlink_file(filename);

      return MIME_HANDLE_OK;
   } else {
      LMAPI->unlink_file(mimefile);
      return MIME_HANDLE_FAIL;
   }

   return MIME_HANDLE_OK;
}

MIME_HANDLER(mimehandle_pantomime_binary)
{
   FILE *tempfile;
   const char *temp, *filename;
   int old_umask;

   LMAPI->clean_var("unmime-first-level", VAR_TEMP);

   filename = LMAPI->mime_parameter(header,"content-type","name");

   if (!filename)
     filename = LMAPI->mime_parameter(header,"content-disposition","filename");

   if (strcasecmp(LMAPI->get_string("mode"),"bounce") == 0) {
       if ((tempfile = LMAPI->open_file(mimefile,"w")) == NULL) {
          return MIME_HANDLE_OK;
       }
       LMAPI->write_file(tempfile,"-- Binary attachments are not handled in bounces --\n");
       LMAPI->close_file(tempfile);
       if (filename) free((char *)filename);
       return MIME_HANDLE_OK;
   }

   old_umask = 022;

#ifndef WIN32
   old_umask = umask(022);
#endif

   if (!LMAPI->get_var("pantomime-dir") || !LMAPI->get_var("pantomime-url") 
        || !filename) {
       if ((tempfile = LMAPI->open_file(mimefile,"w")) == NULL) {
          if(filename) free((char *)filename);
          return MIME_HANDLE_FAIL;
       }

       if (!LMAPI->get_bool("unmime-quiet")) {
          LMAPI->write_file(tempfile,"-- Binary/unsupported file stripped by %s --\n",
                     SERVICE_NAME_MC);

          if (!filename)
             LMAPI->write_file(tempfile,"-- Err : No filename to use for decode, file stripped.\n");
   
          temp = LMAPI->mime_fieldval(header,"content-type");
          if (temp) {
             LMAPI->write_file(tempfile,"-- Type: %s\n", temp);
          }

          if (filename) {
             LMAPI->write_file(tempfile,"-- File: %s\n", filename); 
          }

          temp = LMAPI->mime_fieldval(header,"content-description");
          if (temp) {
             if (*temp) LMAPI->write_file(tempfile,"-- Desc: %s\n", temp);
          }
          LMAPI->write_file(tempfile,"\n");
       }

       LMAPI->close_file(tempfile);
   } else {
       char tempfilename[HUGE_BUF];       
       char movetofile[HUGE_BUF];
       char *tptr;
       int wroteit, count;
       struct stat fst;

       wroteit = 1;

       LMAPI->buffer_printf(tempfilename, sizeof(tempfilename) - 1, "%s", filename);

       tptr = strrchr(tempfilename,'/');
       if (!tptr) tptr = strrchr(tempfilename,'\\');

       if (tptr) tptr++; else tptr = &tempfilename[0];

       LMAPI->buffer_printf(movetofile, sizeof(movetofile) - 1, "%s/%s", LMAPI->get_string("pantomime-dir"),
         tptr);

       count = 2;

       while(LMAPI->exists_file(movetofile) && (count < 99)) {
          LMAPI->buffer_printf(movetofile, sizeof(movetofile) - 1, "%s/%.2d-%s", 
            LMAPI->get_string("pantomime-dir"),count,tptr);
          count++;
       }

       if (LMAPI->exists_file(movetofile)) {
          LMAPI->log_printf(5,"Pantomime: Reverting to queuefile-based name.\n");
          LMAPI->buffer_printf(tempfilename, sizeof(tempfilename) - 1, "%s-%s",
             LMAPI->get_string("queuefile"),filename);

          tptr = strrchr(tempfilename,'/');
          if (!tptr) tptr = strrchr(tempfilename,'\\');

          if (tptr) tptr++; else tptr = &tempfilename[0];

          LMAPI->buffer_printf(movetofile, sizeof(movetofile) - 1, "%s/%s",
            LMAPI->get_string("pantomime-dir"), tptr);
       } else {
          tptr = strrchr(movetofile,'/');
          if (!tptr) tptr = strrchr(movetofile,'\\');
          if (tptr) tptr++; else tptr = &movetofile[0];
       }

       LMAPI->log_printf(9,"PantoMIME: %s -> %s\n", mimefile, movetofile);

       LMAPI->mkdirs(movetofile);

       if (LMAPI->replace_file(mimefile,movetofile)) {
          LMAPI->log_printf(9,"PantoMIME: Unable to move file!\n");
          wroteit = 0;
       }

       LMAPI->public_file(movetofile);

       if ((tempfile = LMAPI->open_file(mimefile,"w")) != NULL) {
           LMAPI->write_file(tempfile, "-- Attached file removed by %s and put at URL below --\n", SERVICE_NAME_MC);

           temp = LMAPI->mime_fieldval(header,"content-type");
           if (temp) {
              LMAPI->write_file(tempfile,"-- Type: %s\n", temp);
           }

           temp = LMAPI->mime_fieldval(header,"content-description");
           if (temp) {
               if (*temp) LMAPI->write_file(tempfile,"-- Desc: %s\n", temp);
           }
           if (wroteit) {
               char finalurl[SMALL_BUF];
               char tempurl[SMALL_BUF/2];

               if (stat(movetofile,&fst) == 0) {
                  int filesize;

                  filesize = fst.st_size;

                  if (filesize > 1024) {
                     int ksize;
                     ksize = filesize / 1024;
                     LMAPI->write_file(tempfile,"-- Size: %dk (%d bytes)\n",
                       ksize, filesize);
                  } else {
                     LMAPI->write_file(tempfile,"-- Size: %d bytes\n",
                       filesize);
                  }
               }
               LMAPI->buffer_printf(tempurl, sizeof(tempurl)/2 - 1, "%s/%s",
                  LMAPI->get_string("pantomime-url"),tptr);
               LMAPI->strreplace(finalurl, sizeof(finalurl) - 1,tempurl," ","%20");
               LMAPI->write_file(tempfile,"-- URL : %s\n\n", finalurl);
           } else {
               LMAPI->write_file(tempfile,"-- URL : Unable to write to web directory!\n\n");
           }
           LMAPI->close_file(tempfile);
       }
   }

   if(filename) free((char *)filename);

#ifndef WIN32
   umask(old_umask);
#endif

   return MIME_HANDLE_OK;
}
