#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#ifndef WIN32
# include <unistd.h>
#endif

#include "stat-mod.h"

FUNC_HANDLER(func_hasstat)
{
    char statval[BIG_BUF];
    const char *user = argv[0];
    const char *stat = argv[1];
    const char *list = LMAPI->get_var("list");

    if (!list) strncpy(result,"0", BIG_BUF - 1);

    if (LMAPI->userstat_get_stat(list,user,stat,statval, sizeof(statval) - 1)) {
       strncpy(result,"1", BIG_BUF - 1);
    } else {
       strncpy(result,"0", BIG_BUF - 1);
    }
    return 1;
}

FUNC_HANDLER(func_list_exists)
{
    char *list = argv[0];

    strncpy(result, "0", BIG_BUF - 1);
    if(LMAPI->list_valid(list)) {
        LMAPI->clean_var("advertise", VAR_TEMP|VAR_LIST);
        LMAPI->read_conf_parm(list,"advertise", VAR_TEMP);
        if(LMAPI->get_bool("advertise")) {
            strncpy(result, "1", BIG_BUF - 1);
        }
        LMAPI->clean_var("advertise", VAR_TEMP|VAR_LIST);
        if(LMAPI->get_var("list")) {
            LMAPI->read_conf_parm(LMAPI->get_string("list"), "advertise",
                                  VAR_LIST);
        }
    }
    return 1;
}

CMD_HANDLER(cmd_list_files)
{
    struct list_file *temp;
    char *tmp;
    char col;

    LMAPI->spit_status("Retrieving file list");

    temp = LMAPI->get_files();
    LMAPI->result_printf("\nThe following files are defined as config files for local lists.\n");
    LMAPI->result_printf("These files are in no particular order.  A module is not required to \n");
    LMAPI->result_printf("register it's use of any specific file with this list.\n\n");
    LMAPI->result_printf("LIST CONFIGURATION FILES\n---\n");

    while(temp) {
        LMAPI->result_printf("%s\n", temp->name);
        tmp = &(temp->desc[0]);
        col = 0;
        while(*tmp) {
            if (col == 0) {
                LMAPI->result_printf("          ");
                col = 10;
            }
            if ((*tmp == ' ') && col > 65) {
                col = 0;
                LMAPI->result_printf("\n");
                tmp++;
            } else {
                LMAPI->result_printf("%c", *tmp);
                tmp++; col++;
            }
        }
        LMAPI->result_printf("\n\n");
        temp = temp->next;
    }

    return CMD_RESULT_CONTINUE;
}


CMD_HANDLER(cmd_list_modules)
{
    struct listserver_module *temp;
    char *tmp; char col;

    LMAPI->spit_status("Retrieving module list");

    temp = LMAPI->get_modules();

    LMAPI->result_printf("\nThis is a quick reference of the %s modules active locally.\n", SERVICE_NAME_MC);
    LMAPI->result_printf("These are in no particular order, and are generated from the local\n");
    LMAPI->result_printf("module list.  Modules are not required to list themselves.\n\n%s MODULES\n---\n", SERVICE_NAME_UC);

    while(temp) {
        LMAPI->result_printf("%s\n", temp->name);
        tmp = &(temp->desc[0]);
        col = 0;
        while(*tmp) {
            if (col == 0) {
                 LMAPI->result_printf("          ");
                 col = 10;
            }
            if ((*tmp == ' ') && col > 65) {
                 col = 0;
                 LMAPI->result_printf("\n");
                 tmp++;
            } else {
                 LMAPI->result_printf("%c", *tmp);
                 tmp++; col++;
            }
        }
        LMAPI->result_printf("\n\n");
        temp = temp->next;
    }

    return CMD_RESULT_CONTINUE;
}

CMD_HANDLER(cmd_list_commands)
{
    struct listserver_cmd *temp;
    char *tmp; char col;

    LMAPI->spit_status("Retrieving command list");

    temp = LMAPI->get_commands();

    LMAPI->result_printf("\nThis is a quick reference of the %s commands active locally.\n", SERVICE_NAME_MC);
    LMAPI->result_printf("These are in no particular order, and are generated from the local\n");
    LMAPI->result_printf("parser command table.\n\n%s COMMANDS\n---\n", SERVICE_NAME_UC);

    while(temp) {
        int has_syntax = 0;
        LMAPI->result_printf("%s", temp->name);
        if ((temp->flags & CMD_ADMIN) == CMD_ADMIN) {
            LMAPI->result_printf(" (ADMIN)");
        }
        LMAPI->result_printf("\n");
        if(temp->syntax) {
            has_syntax = 1;
            LMAPI->result_printf("          Syntax: %s\n", temp->syntax);
            if(temp->altsyntax) {
                LMAPI->result_printf("              or: %s\n", temp->altsyntax);
            }
        }
        if(temp->adminsyntax) {
            if(!has_syntax) {
                LMAPI->result_printf("          Syntax: %s (admin only)\n",
                                     temp->adminsyntax);
            } else {
                LMAPI->result_printf("              or: %s (admin only)\n",
                                     temp->adminsyntax);
            }
        }
        tmp = &(temp->desc[0]);
        col = 0;
        while(*tmp) {
            if (col == 0) {
                LMAPI->result_printf("          ");
                col = 10;
            }
            if ((*tmp == ' ') && col > 65) {
                col = 0;
                LMAPI->result_printf("\n");
                tmp++;
            } else {
                LMAPI->result_printf("%c", *tmp);
                tmp++; col++;
            }
        }
        LMAPI->result_printf("\n\n");
        temp = temp->next;
    }

    return CMD_RESULT_CONTINUE;
}

CMD_HANDLER(cmd_list_flags)
{
    struct listserver_flag *temp;
    char *tmp; char col;

    LMAPI->spit_status("Retrieving flag list");

    temp = LMAPI->get_flags();

    LMAPI->result_printf("\nThis is a quick reference of the %s flags active locally.\n", SERVICE_NAME_MC);
    LMAPI->result_printf("These are in no particular order, and are generated from the\n");
    LMAPI->result_printf("local flag table.\n\n");
    LMAPI->result_printf("%s USER FLAGS\n---\n", SERVICE_NAME_UC);

    while(temp) {
        LMAPI->result_printf("%s", temp->name);
        if (temp->admin) {
            LMAPI->result_printf(" (only settable by an ADMIN)");
        }
        LMAPI->result_printf("\n");
        tmp = &(temp->desc[0]);
        col = 0;
        while(*tmp) {
            if (col == 0) {
                LMAPI->result_printf("          ");
                col = 10;
            }
            if ((*tmp == ' ') && col > 65) {
                col = 0;
                LMAPI->result_printf("\n");
                tmp++;
            } else {
                LMAPI->result_printf("%c", *tmp);
                tmp++; col++;
            }
        }
        LMAPI->result_printf("\n\n");
        temp = temp->next;
    }

    return CMD_RESULT_CONTINUE;
}

CMD_HANDLER(cmd_list_lists)
{
    char dname[BIG_BUF];
    int status, count;

    LMAPI->log_printf(9,"Trying to read lists dir '%s'...\n",
                      LMAPI->get_string("lists-root"));

    if (!(status = LMAPI->walk_lists(&dname[0]))) {
        LMAPI->buffer_printf(dname, sizeof(dname) - 1, "%s/ (opening directory)", LMAPI->get_string("lists-root"));
        LMAPI->filesys_error(dname);
        LMAPI->result_printf("\nA probable cause is an empty lists directory.\n");
        return CMD_RESULT_CONTINUE;
    }

    LMAPI->spit_status("%s lists available on this machine:\n", SERVICE_NAME_MC);

    count = 0;

    while (status) {
        LMAPI->clean_var("advertise", VAR_TEMP|VAR_LIST);
        LMAPI->read_conf_parm(dname,"advertise", VAR_TEMP);
        LMAPI->clean_var("description", VAR_TEMP|VAR_LIST);
        LMAPI->read_conf_parm(dname,"description", VAR_TEMP);
        if (LMAPI->get_bool("advertise")) {
            count++;
            LMAPI->result_printf("%s\n",dname);

            if (LMAPI->get_var("description")) {
                char temp[BIG_BUF];
                int col;
                char *tmp;

                LMAPI->buffer_printf(temp, sizeof(temp) - 1, "%s", LMAPI->get_string("description"));

                tmp = &temp[0];
                col = 0;
                while(*tmp) {
                    if (col == 0) {
                        LMAPI->result_printf("          ");
                        col = 10;
                    }
                    if ((*tmp == ' ') && col > 58) {
                        col = 0;
                        LMAPI->result_printf("\n");
                        tmp++;
                    } else {
                        LMAPI->result_printf("%c", *tmp);
                        tmp++; col++;
                    }
                }
                LMAPI->result_printf("\n\n");
            } else {
                LMAPI->result_printf("          No description.\n\n");
            }
        }
        LMAPI->clean_var("advertise", VAR_TEMP|VAR_LIST);
        LMAPI->clean_var("description", VAR_TEMP|VAR_LIST);
        status = LMAPI->next_lists(&dname[0]);
    }
    if(LMAPI->get_var("list")) {
        LMAPI->read_conf_parm(LMAPI->get_string("list"), "advertise",
                              VAR_LIST);
        LMAPI->read_conf_parm(LMAPI->get_string("list"), "description",
                              VAR_LIST);
    }
    if (!count) LMAPI->result_printf("\tNo publically accessible lists on this machine.\n");

    return CMD_RESULT_CONTINUE;
}

CMD_HANDLER(cmd_list_users)
{
    const char *whostat;
    struct list_user user;
    char buffer[BIG_BUF];
    FILE *userfile;
    int isadmin;
    char *cclist;

    if(params->num) {
        if (!LMAPI->set_context_list(params->words[0])) {
            LMAPI->nosuch(params->words[0]);
            return CMD_RESULT_CONTINUE;
        }
    } else {
        if (!LMAPI->get_var("list")) {
            LMAPI->spit_status("No list in current context.");
            return CMD_RESULT_CONTINUE;
        }
    }

    whostat = LMAPI->get_var("who-status");
  
    if (whostat) {
        if (!strcasecmp(whostat,"private")) {
            if (!LMAPI->user_find_list(LMAPI->get_string("list"),LMAPI->get_string("realsender"),&user)) {
                LMAPI->spit_status("List membership is only viewable by list members.");
                return CMD_RESULT_CONTINUE;
            }
        } else if (!strcasecmp(whostat,"admin")) {
            int failure;

            failure = 1;

            if (LMAPI->user_find_list(LMAPI->get_string("list"),LMAPI->get_string("realsender"),&user)) {
                if (LMAPI->user_hasflag(&user,"ADMIN")) {
                    failure = 0;
                }
            }

            if (failure) {
                LMAPI->spit_status("List membership is only viewable by list admins.");
                return CMD_RESULT_CONTINUE;
            }
        }
    }

    LMAPI->spit_status("Membership of list '%s':",LMAPI->get_string("list"));

    isadmin = 0;

    if (LMAPI->user_find_list(LMAPI->get_string("list"),LMAPI->get_string("realsender"),&user)) {
        if (LMAPI->user_hasflag(&user,"ADMIN")) {
            isadmin = 1;
        }
    }


    LMAPI->listdir_file(buffer, LMAPI->get_string("list"), "users");
    if ((userfile = LMAPI->open_file(&buffer[0], "r")) == NULL) {
        LMAPI->filesys_error(&buffer[0]);
        return CMD_RESULT_CONTINUE;
    }

    while(LMAPI->user_read(userfile,&user)) {
        if(!LMAPI->user_hasflag(&user,"HIDDEN") || isadmin) {
            LMAPI->result_printf("\t%s",user.address);
            if (LMAPI->user_hasflag(&user,"ADMIN")) {
                LMAPI->result_printf(" (ADMIN)");
            }
            if (LMAPI->user_hasflag(&user,"HIDDEN")) {
                LMAPI->result_printf(" (HIDDEN)");
            }
            if (LMAPI->user_hasflag(&user,"VACATION") && isadmin) {
                LMAPI->result_printf(" (VACATION)");
            }
            LMAPI->result_printf("\n");
        }
    }
    LMAPI->close_file(userfile);

    /* Okay, handle cc-lists as well */
    if(LMAPI->get_var("cc-lists")) {
        int do_header = 1;
        char *l;
        cclist = strdup(LMAPI->get_string("cc-lists"));
        l = strtok(cclist, ":");
        while(l) {
            if(LMAPI->list_valid(l)) {
                int advert = 0;
                LMAPI->clean_var("advertise", VAR_TEMP|VAR_LIST);
                LMAPI->read_conf_parm(l, "advertise", VAR_TEMP);
                advert = LMAPI->get_bool("advertise");
                if(advert || isadmin) {
                    if(do_header) {
                        LMAPI->result_printf("CC-lists for list '%s':\n", 
                                             LMAPI->get_string("list"));
                    }
                    LMAPI->result_printf("\t%s", l);
                    if(isadmin && !advert) {
                        LMAPI->result_printf(" (UNADVERTISED)");
                    }
                    LMAPI->result_printf("\n");
                }
            }
            l = strtok(NULL, ":");
        }
        LMAPI->clean_var("advertise", VAR_TEMP|VAR_LIST);
        if(LMAPI->get_var("list")) {
            LMAPI->read_conf_parm(LMAPI->get_string("list"), "advertise",
                                  VAR_LIST);
        }
        free(cclist);
    }
    
    return CMD_RESULT_CONTINUE;
}

CMD_HANDLER(cmd_help)
{
    char buf[BIG_BUF];
    LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%s help file", SERVICE_NAME_MC);
    LMAPI->set_var("task-form-subject", buf, VAR_TEMP);
    LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%s/%s.hlp", LMAPI->get_string("listserver-data"), SERVICE_NAME_LC); 
    if (!LMAPI->send_textfile_expand(LMAPI->get_string("fromaddress"), buf)) {
        LMAPI->filesys_error(buf);
    } else {
        LMAPI->spit_status("File sent.");
    }
    LMAPI->clean_var("task-form-subject", VAR_TEMP);
    return CMD_RESULT_CONTINUE;
}

CMD_HANDLER(cmd_which_lists)
{
    char dname[BIG_BUF];
    int status;
    int count = 0;
    struct list_user user;

    if(!(status = LMAPI->walk_lists(&dname[0]))) {
        LMAPI->spit_status("Unable to open lists directory."); 
        return CMD_RESULT_CONTINUE;
    }
    LMAPI->spit_status("Retrieving list subscriptions.");
    LMAPI->result_printf("\n%s is subscribed to the following lists:\n",
                         LMAPI->get_string("fromaddress"));
    while(status) {
        if(!LMAPI->list_valid(dname)) {
            status = LMAPI->next_lists(dname);
            continue;
        }
        if(LMAPI->user_find_list(dname, LMAPI->get_string("fromaddress"), &user)) {
            count++;
            if(!strcasecmp(LMAPI->get_string("fromaddress"), user.address)) {
                LMAPI->result_printf("      %s\n", dname);
            } else {
                LMAPI->result_printf("      %s (subscribed as %s)\n", dname,
                                     user.address);
            }
        }
        status = LMAPI->next_lists(dname);
    }
    if(count == 0) {
        LMAPI->result_printf("        No subscribed lists.\n");
    }
    return CMD_RESULT_CONTINUE;
}

