/* Very preliminary archiving */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#endif

#include "lpm.h"

struct LPMAPI *LMAPI;

int update_mbox_archive()
{
    int closedpost;
    int moderated;
    int result;

    result=0;

    LMAPI->log_printf(9, "Checking if archive should be updated...\n");

    moderated = LMAPI->get_bool("moderated");

    if (moderated) {
        LMAPI->log_printf(9, "Performing moderator check.\n");
        if (LMAPI->get_var("moderated-approved-by")) {
            LMAPI->log_printf(9, "Passed moderator check.\n");
            result = 1; 
        } else {
            LMAPI->log_printf(9, "Failed moderator check.\n");
        } 
    } else {
        LMAPI->log_printf(9, "Performing closed list check.\n");
        closedpost = LMAPI->get_bool("closed-post");

        if (!closedpost) {
            LMAPI->log_printf(9, "Open posting, archive ok.\n");
            result = 1;
        } else {
            struct list_user tuser;

	    LMAPI->log_printf(9, "Performing closed list user check.\n");

            if (LMAPI->user_find_list(LMAPI->get_string("list"),
                                      LMAPI->get_string("fromaddress"),
                                      &tuser)) {
                LMAPI->log_printf(9, "Passed closed list user check.\n");
                result = 1; 
            } else { 
                LMAPI->log_printf(9, "Failed closed list user check.\n");
            }
	    if (!result){
        	LMAPI->log_printf(9, "Performing closed list moderator check.\n");
  
		if (LMAPI->get_var("moderated-approved-by")) {
                  LMAPI->log_printf(9, "Passed closed list moderator check.\n");
                  result = 1;
		} else { 
	           LMAPI->log_printf(9, "Failed closed list moderator check.\n"); 
		}
	    }

	    if (!result) {
	       LMAPI->log_printf(9, "Performing posting-acl check.\n");
		 
	       if (LMAPI->get_bool("posting-acl")) {
                 char aclfilename[BIG_BUF];
                 FILE *aclfile;
                 char aclbuffer[BIG_BUF];

                 LMAPI->listdir_file(aclfilename,LMAPI->get_string("list"),
                             LMAPI->get_string("posting-acl-file"));         

                 if (LMAPI->exists_file(aclfilename)) {
            
                   if ((aclfile = LMAPI->open_file(aclfilename,"r")) != NULL) {

                      while (LMAPI->read_file(aclbuffer, sizeof(aclbuffer), aclfile)) {

                         if (aclbuffer[strlen(aclbuffer) - 1] == '\n')
                                       aclbuffer[strlen(aclbuffer) - 1] = 0;

                         if (LMAPI->match_reg(aclbuffer, 
                                       LMAPI->get_string("fromaddress"))) {
			 LMAPI->log_printf(9, "Passed posting-acl check.\n");    
                         result = 1;
			 break;
			 }
		      }
                      LMAPI->close_file(aclfile);
		   }
		 }
	       } 
	       if(!result) LMAPI->log_printf(9, "Failed posting-acl check.\n"); 
	    }

	    if (!result) {
		LMAPI->log_printf(9, "Performing union-lists check.\n");    
		if (LMAPI->get_var("union-lists")) {
                  char unions[64];
                  char *tptr, *tptr2;

                  LMAPI->buffer_printf(unions, 64, "%.63s", LMAPI->get_var("union-lists"));

                  tptr = &unions[0];
                  tptr2 = strchr(tptr,':');
                  if (tptr2) *tptr2++ = 0;

                  while (tptr) {
                        if (LMAPI->user_find_list(tptr,
                                  LMAPI->get_string("fromaddress"), &tuser)) {
	                   LMAPI->log_printf(9, "Passed union-lists check (found in %s).\n",tptr);
                           result = 1;
                           break;
			} 

                        tptr = tptr2;

                        if (tptr) {
                            tptr2 = strchr(tptr,':');
                            if (tptr2) *tptr2++ = 0;
			}
		  }
              
		} 
	    if(!result) LMAPI->log_printf(9, "Failed union-lists check.\n");
	    }
            if(!result) LMAPI->log_printf(9, "Failed closed list check.\n");
       }
   }

   LMAPI->log_printf(9, "Update status: %s.\n", result ? "Yes" : "No");
   return result;
}

HOOK_HANDLER(hook_presend_listarchive)
{
    FILE *outfile;
    FILE *infile;
    char filename[BIG_BUF];
    char buf[BIG_BUF];
    int donefile, arcnum;

    if ((!LMAPI->get_var("mbox-archive-path")) &&
        (!LMAPI->get_var("archive-path")))
       return HOOK_RESULT_OK;

    LMAPI->log_printf(9,"In ListArchive presend...\n");

    if (!update_mbox_archive()) return HOOK_RESULT_OK;

    if (LMAPI->get_var("archive-num")) return HOOK_RESULT_OK;

    LMAPI->listdir_file(filename, LMAPI->get_string("list"), "archive");

    arcnum = 1;

    if ((infile = LMAPI->open_file(filename,"r")) != NULL) {
        LMAPI->read_file(buf, sizeof(buf), infile);
        arcnum = atoi(buf) + 1;
        LMAPI->close_file(infile);
    }

    if ((outfile = LMAPI->open_file(filename,"w")) != NULL) {
        LMAPI->write_file(outfile, "%d\n", arcnum);
        LMAPI->close_file(outfile);
        LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%d", arcnum);
        LMAPI->set_var("archive-num",buf, VAR_TEMP);
    } else
        return HOOK_RESULT_OK;

    donefile = 0;

    if (!(infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r"))) {
        LMAPI->filesys_error(LMAPI->get_string("queuefile"));
        return HOOK_RESULT_FAIL;
    }

    LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.arch", LMAPI->get_string("queuefile"));
    if ((outfile = LMAPI->open_file(filename,"w")) == NULL) {
        LMAPI->close_file(infile);
        LMAPI->filesys_error(&filename[0]);
        return HOOK_RESULT_FAIL;
    }

    donefile = 0;

    while(LMAPI->read_file(buf, sizeof(buf), infile)) {
        if ((buf[0] == '\n') && !donefile) {
            donefile = 1;
            LMAPI->write_file(outfile,"X-archive-position: %d\n", arcnum);
        }
        LMAPI->write_file(outfile,"%s",buf);
    }

    LMAPI->close_file(infile);
    LMAPI->close_file(outfile);
   
    if (LMAPI->replace_file(filename,LMAPI->get_string("queuefile"))) {
        char tempbuf[BIG_BUF];

        LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", filename, LMAPI->get_string("queuefile"));
        LMAPI->filesys_error(&tempbuf[0]);
        LMAPI->unlink_file(filename);
        return HOOK_RESULT_FAIL;
    }

    return HOOK_RESULT_OK;   
}


HOOK_HANDLER(hook_postsend_listarchive)
{
    char filename[BIG_BUF];
    char buf[BIG_BUF];
    char tbuf[12];
    const char *queuefile;
    FILE *outfile, *infile;
    struct tm *tm_now;
    time_t now;
    int old_umask;

    LMAPI->log_printf(9,"In ListArchive postsend...\n");

    if (!update_mbox_archive()) return HOOK_RESULT_OK;

    if ((!LMAPI->get_var("mbox-archive-path")) &&
        (!LMAPI->get_var("archive-path")))
        return HOOK_RESULT_OK;
    if (!LMAPI->get_var("archive-num"))
        return HOOK_RESULT_OK;

    now = time(NULL);
    tm_now = localtime(&now);

    old_umask = 022;

#ifndef WIN32
    if (LMAPI->get_bool("archive-world-readable"))
       old_umask = umask(022);
#endif

    strftime(&tbuf[0], 12,"%Y-%m",tm_now);

    queuefile = LMAPI->get_string("queuefile");

    if (LMAPI->get_bool("humanize-mime")) {
       LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s.unmime", queuefile);

       LMAPI->unmime_file(queuefile,filename);

       if (LMAPI->get_bool("just-unmimed"))
          if (LMAPI->replace_file(filename,queuefile)) {
              char tempbuf[BIG_BUF];

              LMAPI->buffer_printf(tempbuf, sizeof(tempbuf) - 1, "%s -> %s", filename, queuefile);
              LMAPI->filesys_error(&tempbuf[0]);
              LMAPI->unlink_file(filename);
#ifndef WIN32
              umask(old_umask);
#endif
              return HOOK_RESULT_FAIL;
          }
    }

    if (LMAPI->get_var("mbox-archive-path")) {

       LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s/%s/%s.%s", LMAPI->get_string("listserver-data"),
               LMAPI->get_string("mbox-archive-path"),
               LMAPI->get_string("list"), tbuf);
       LMAPI->mkdirs(filename);
  
       if ((outfile = LMAPI->open_file(filename,"a")) == NULL) 
          goto MH_Archive;
       if ((infile = LMAPI->open_file(queuefile,"r")) == NULL) {
          LMAPI->close_file(outfile);
          goto MH_Archive;
       }

       while(LMAPI->read_file(buf, sizeof(buf), infile)) {
          LMAPI->write_file(outfile,"%s",buf);
       }

       /* For safety's sake.  mbox doesn't REQUIRE newlines between
          messages, but apparently some tools break if they aren't
          there.  The ones that don't require them won't break if they
          ARE there, so... */
       LMAPI->write_file(outfile,"\n");

       LMAPI->close_file(outfile);
       LMAPI->close_file(infile);
    }

MH_Archive:

    if (LMAPI->get_var("archive-path")) {
       LMAPI->buffer_printf(filename, sizeof(filename) - 1, "%s/%s/%s/%d", LMAPI->get_string("listserver-data"), 
               LMAPI->get_string("archive-path"), tbuf,
               LMAPI->get_number("archive-num"));
       LMAPI->mkdirs(filename);
  
       if ((outfile = LMAPI->open_file(filename,"r")) != NULL) {
          LMAPI->close_file(outfile);
          LMAPI->log_printf(0, "Attempt to archive failed: %s exists\n",
                            filename);
#ifndef WIN32
          umask(old_umask);
#endif
          return HOOK_RESULT_OK;
       }

       if ((outfile = LMAPI->open_file(filename,"w")) == NULL) {
          LMAPI->log_printf(0, "Access error attempting to write to MH style archive.\n");
#ifndef WIN32
          umask(old_umask);
#endif
          return HOOK_RESULT_OK;
       }
       if ((infile = LMAPI->open_file(LMAPI->get_string("queuefile"),"r")) == NULL) {
          LMAPI->close_file(outfile);
          LMAPI->log_printf(0, "Could not read queuefile for output to MH archive.\n");
#ifndef WIN32
          umask(old_umask);
#endif
          return HOOK_RESULT_OK;
       } 

       while(LMAPI->read_file(buf, sizeof(buf), infile)) {
          LMAPI->write_file(outfile,"%s",buf);
       }

       LMAPI->close_file(outfile);
       LMAPI->close_file(infile);
    }

#ifndef WIN32
    umask(old_umask);
#endif
    return HOOK_RESULT_OK;   
}

void listarchive_switch_context(void)
{
    LMAPI->log_printf(15, "Switching context in module ListArchive\n");
}

int listarchive_upgradelist(int prev, int cur)
{
    LMAPI->log_printf(10, "Upgrading lists in module ListArchive\n");
    return 1;
}

int listarchive_upgrade(int prev, int cur)
{
    LMAPI->log_printf(10, "Upgrading module ListArchive\n");
    return 1;
}

void listarchive_init(void)
{
    LMAPI->log_printf(10, "Initializing module ListArchive\n");
}

void listarchive_unload(void)
{
    LMAPI->log_printf(10, "Unloading module ListArchive\n");
}

void listarchive_load(struct LPMAPI *api)
{
    LMAPI = api;

    LMAPI->log_printf(10, "Loading module ListArchive\n");

    LMAPI->add_hook("SEND", 1, hook_presend_listarchive);
    LMAPI->add_hook("AFTER", 1, hook_postsend_listarchive);

    LMAPI->add_module("ListArchive", "General-purpose archive module, replaces archive_mh and archive_mbox.  Allows both archive types.");

    /* Register variable */
    LMAPI->register_var("archive-num", NULL, NULL, NULL, NULL, VAR_INT,
                        VAR_INTERNAL|VAR_TEMP);
    LMAPI->register_var("mbox-archive-path", NULL, "ListArchive",
                        "Path to where MBox format archives are stored.",
                        "mbox-archive-path = archives/mylist/mbox",
                        VAR_STRING, VAR_ALL);
    LMAPI->register_var("mh-archive-path", NULL, "ListArchive",
                        "Path to where MH format archives are stored.",
                        "mh-archive-path = archives/mylist/mh",
                        VAR_STRING, VAR_ALL);
    LMAPI->register_alias("archive-path", "mh-archive-path");
    LMAPI->register_var("archive-world-readable", "yes", "ListArchive",
                        "Should we make all archive files world-readable?",
                        "archive-world-readable = yes",
                        VAR_BOOL, VAR_ALL);
}
