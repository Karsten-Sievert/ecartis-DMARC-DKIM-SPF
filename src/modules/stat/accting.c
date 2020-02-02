#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "lpm.h"
#include "stat-mod.h"

HOOK_HANDLER(hook_after_postsize)
{
   int fullsize, headersize, bodysize;
   struct stat fst;
   char inbody;
   FILE *infile;
   char buffer[BIG_BUF];
   char trafficsize[SMALL_BUF];
   unsigned long trafficval;

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

   trafficval = 0;

   if (LMAPI->userstat_get_stat(LMAPI->get_string("list"),LMAPI->get_string("fromaddress"),
            "traffic", trafficsize, sizeof(trafficsize) - 1)) {
       trafficval = atol(trafficsize);
   }

   trafficval = trafficval + ((bodysize / 1024) + 1);
   LMAPI->buffer_printf(trafficsize, sizeof(trafficsize) - 1, "%lu", trafficval);

   if(!LMAPI->userstat_set_stat(LMAPI->get_string("list"),LMAPI->get_string("fromaddress"),
            "traffic", trafficsize)) {
       LMAPI->log_printf(5,"Unable to write stat 'traffic' to userstat file.\n");
   }

   return HOOK_RESULT_OK;
}

HOOK_HANDLER(hook_after_numposts)
{
   char numposts[SMALL_BUF];
   unsigned long numpostval;

   numpostval = 0;

   if (LMAPI->userstat_get_stat(LMAPI->get_string("list"),LMAPI->get_string("fromaddress"),
            "posts", numposts, sizeof(numposts) - 1)) {
       numpostval = atol(numposts);
   }

   numpostval = numpostval + 1;
   LMAPI->buffer_printf(numposts, sizeof(numposts) - 1, "%lu", numpostval);

   if (!LMAPI->userstat_set_stat(LMAPI->get_string("list"),LMAPI->get_string("fromaddress"),
            "posts", numposts)) {
       LMAPI->log_printf(5,"Unable to write stat 'posts' to userstat file.\n");
   }

   return HOOK_RESULT_OK;
}

CMD_HANDLER(cmd_stats)
{
    struct list_user user;
    char username[BIG_BUF];
    char listname[BIG_BUF];
    char buffer[BIG_BUF];
    char *tok;
    int skipline;

    if (params->num != 0) {
        if (LMAPI->get_bool("adminmode")) {
            LMAPI->buffer_printf(username, sizeof(username) - 1, "%s", params->words[0]);
            LMAPI->buffer_printf(listname, sizeof(listname) - 1, "%s", LMAPI->get_string("list"));
        } else {
            LMAPI->buffer_printf(username, sizeof(username) - 1, "%s", LMAPI->get_string("fromaddress"));
            LMAPI->buffer_printf(listname, sizeof(listname) - 1, "%s", params->words[0]);
        }
    } else {
        LMAPI->buffer_printf(username, sizeof(username) - 1, "%s", LMAPI->get_string("fromaddress"));
        LMAPI->buffer_printf(listname, sizeof(listname) - 1, "%s", LMAPI->get_string("list"));
    }

    if (!strcmp(listname,"")) {
        LMAPI->spit_status("No list in current context.");
        return CMD_RESULT_CONTINUE;
    }

    if(!LMAPI->list_valid(listname)) {
        LMAPI->nosuch(listname);
        return CMD_RESULT_CONTINUE;
    }

    if(!LMAPI->set_context_list(listname)) {
       LMAPI->spit_status("Unable to switch context to list '%s'.", listname);
       return CMD_RESULT_END;
    }

    if (!LMAPI->user_find_list(listname,&username[0],&user)) {
        if (strcmp(LMAPI->get_string("realsender"),username)) {
            LMAPI->spit_status("%s is not a member of list '%s'",username,listname);
        } else {
            LMAPI->spit_status("You are not a member of list '%s'",listname);
        }
        return CMD_RESULT_CONTINUE;
    }

    LMAPI->spit_status("Current account flags for '%s' on '%s':\n", username, listname);

    LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s", &(user.flags[1]));

    tok = strtok(buffer,"|");

    while(tok) {
        LMAPI->result_printf("\t%s\n",tok);
        tok = strtok(NULL,"|");
    }

    LMAPI->result_printf("\n");

    skipline = 0;

    if (LMAPI->userstat_get_stat(LMAPI->get_string("list"),LMAPI->get_string("fromaddress"),
                "posts", buffer, sizeof(buffer) - 1)) {
        LMAPI->result_printf("Recorded posts by user to list: %s\n", buffer);
        skipline = 1;
    }

    if (LMAPI->userstat_get_stat(LMAPI->get_string("list"),LMAPI->get_string("fromaddress"),
                "traffic", buffer, sizeof(buffer) - 1)) {
        LMAPI->result_printf("Recorded traffic from user on list: %sk\n", buffer);
        skipline = 1;
    }

    if (skipline)
        LMAPI->result_printf("\n");

    return CMD_RESULT_CONTINUE;
}

CMD_HANDLER(cmd_review)
{
    struct list_user user;
    char filename[BIG_BUF]; /* Changed from SMALL_BUF to BIG_BUF due to listdir_file */
    FILE *listuserfile;
    char buffer[SMALL_BUF];

    if (params->num != 0) {
        if (!LMAPI->set_context_list(params->words[0])) {
            LMAPI->nosuch(params->words[0]);
            return CMD_RESULT_CONTINUE;
        }
    }  

    if (!LMAPI->get_var("list")) {
        LMAPI->spit_status("No list in current context.");
        return CMD_RESULT_CONTINUE;
    }

    if (!LMAPI->user_find_list(LMAPI->get_string("list"),LMAPI->get_string("fromaddress"),&user)) {
        LMAPI->spit_status("You aren't a member of that list.");
        return CMD_RESULT_CONTINUE;
    }

    if (!LMAPI->user_hasflag(&user,"ADMIN")) {
        LMAPI->spit_status("You aren't an admin on that list.");
        return CMD_RESULT_CONTINUE;
    }

    LMAPI->listdir_file(filename, LMAPI->get_string("list"), "users");

    if ((listuserfile = LMAPI->open_file(filename,"r")) == NULL) {
        LMAPI->filesys_error(filename);
        return CMD_RESULT_END;
    }

    LMAPI->spit_status("List review report:");

    LMAPI->result_printf("\n");

    while (LMAPI->user_read(listuserfile,&user)) {
        int gotname, gotposts, gottraffic;

        gotname = 0; gotposts = 0; gottraffic = 0;

        LMAPI->result_printf("  ");

        if (LMAPI->userstat_get_stat(LMAPI->get_string("list"),user.address,"realname", buffer, sizeof(buffer) - 1)) {
           LMAPI->result_printf("%s <", buffer);
           gotname = 1;
        }

        LMAPI->result_printf("%s", user.address);

        if (gotname) {
           LMAPI->result_printf(">");
        }

        if (LMAPI->user_hasflag(&user,"HIDDEN")) {
            LMAPI->result_printf("  (HIDDEN)");
        }

        LMAPI->result_printf("\n\t");

        if (LMAPI->userstat_get_stat(LMAPI->get_string("list"),user.address,"position", buffer, sizeof(buffer) - 1)) {
           LMAPI->result_printf("Position: %s", buffer);
           LMAPI->result_printf("\n\t");
        }

        if (LMAPI->userstat_get_stat(LMAPI->get_string("list"),user.address,"posts", buffer, sizeof(buffer) - 1)) {
            LMAPI->result_printf("Posts: %s \t", buffer);
            gotposts = 1;
        }
        if (LMAPI->userstat_get_stat(LMAPI->get_string("list"),user.address,"traffic", buffer, sizeof(buffer) - 1)) {
            LMAPI->result_printf("Traffic: %sk", buffer);
            gottraffic = 1;
        }
        if (gottraffic || gotposts) LMAPI->result_printf("\n\t");

        LMAPI->result_printf("Flags:");

        if (LMAPI->user_hasflag(&user,"ADMIN")) {
            LMAPI->result_printf("ADMIN ");
        }
        if (LMAPI->user_hasflag(&user,"DIAGNOSTIC")) {
            LMAPI->result_printf("DIAGNOSTIC ");
        }
        if (LMAPI->user_hasflag(&user,"MODERATOR")) {
            LMAPI->result_printf("MODERATOR ");
        }
        if (LMAPI->user_hasflag(&user,"VACATION")) {
            LMAPI->result_printf("VACATION ");
        }
        if (LMAPI->user_hasflag(&user,"DIGEST")) {
            LMAPI->result_printf("DIGEST ");
        }
        LMAPI->result_printf("\n\n");
    }

    LMAPI->close_file(listuserfile);
    LMAPI->result_printf("\nReport done.\n");
    
    return CMD_RESULT_CONTINUE;
}

CMD_HANDLER(cmd_setname)
{
    struct list_user user;
    char buffer[SMALL_BUF];
    int counter;

    if (!LMAPI->get_var("adminmode")) {
        LMAPI->spit_status("This command can only be used in admin mode.");
        return CMD_RESULT_CONTINUE;
    }

    if (params->num < 2) {
        LMAPI->spit_status("Needs at least two parameters.");
        return CMD_RESULT_CONTINUE;
    }  

    if (!LMAPI->get_var("list")) {
        LMAPI->spit_status("No list in current context.");
        return CMD_RESULT_CONTINUE;
    }

    if (!LMAPI->user_find_list(LMAPI->get_string("list"),params->words[0],&user))
    {
        LMAPI->spit_status("Not a member of this list.");
        return CMD_RESULT_CONTINUE;
    }

    counter = 1;
  
    LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s", params->words[1]);
  
    for(counter = 2; counter < params->num; counter++) {
       stringcat(buffer, " ");
       stringcat(buffer, params->words[counter]);
    }

    if (!LMAPI->userstat_set_stat(LMAPI->get_string("list"),params->words[0],
       "realname",buffer)) {
       LMAPI->spit_status("Error writing to userdb.");
    } else {
       LMAPI->spit_status("Real name set.");
    }

    return CMD_RESULT_CONTINUE;
}

CMD_HANDLER(cmd_setrole)
{
    struct list_user user;
    char buffer[SMALL_BUF];
    int counter;

    if (!LMAPI->get_var("adminmode")) {
        LMAPI->spit_status("This command can only be used in admin mode.");
        return CMD_RESULT_CONTINUE;
    }

    if (params->num < 2) {
        LMAPI->spit_status("Needs at least two parameters.");
        return CMD_RESULT_CONTINUE;
    }  

    if (!LMAPI->get_var("list")) {
        LMAPI->spit_status("No list in current context.");
        return CMD_RESULT_CONTINUE;
    }

    if (!LMAPI->user_find_list(LMAPI->get_string("list"),params->words[0],&user))
    {
        LMAPI->spit_status("Not a member of this list.");
        return CMD_RESULT_CONTINUE;
    }

    counter = 1;
  
    LMAPI->buffer_printf(buffer, sizeof(buffer) - 1, "%s", params->words[1]);
  
    for(counter = 2; counter < params->num; counter++) {
       stringcat(buffer, " ");
       stringcat(buffer, params->words[counter]);
    }

    if (!LMAPI->userstat_set_stat(LMAPI->get_string("list"),params->words[0],
       "position",buffer)) {
       LMAPI->spit_status("Error writing to userdb.");
    } else {
       LMAPI->spit_status("Position set.");
    }

    return CMD_RESULT_CONTINUE;
}

