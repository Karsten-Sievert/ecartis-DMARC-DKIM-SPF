#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>

#include "lpm.h"

#include "filearchive-mod.h"

struct LPMAPI *LMAPI;

#ifdef WIN32
#define S_IFDIR _S_IFDIR
#endif

void filearchive_switch_context(void)
{
    LMAPI->log_printf(15, "Switching context in module FileArchive\n");
}

int filearchive_upgradelist(int prev, int cur)
{
    LMAPI->log_printf(10, "Upgrading lists in module FileArchive\n");
    return 1;
}

int filearchive_upgrade(int prev, int cur)
{
    LMAPI->log_printf(10, "Upgrading module FileArchive\n");
    return 1;
}

void filearchive_init(void)
{
    LMAPI->log_printf(10, "Initializing module FileArchive\n");
}

void filearchive_unload(void)
{
    LMAPI->log_printf(10, "Unloading module FileArchive\n");
}

void filearchive_load(struct LPMAPI *api)
{
    char tbuf[BIG_BUF];

    LMAPI = api;
    LMAPI->log_printf(10, "Loading module FileArchive\n");

    LMAPI->buffer_printf(tbuf, sizeof(tbuf) - 1, "File archive module (version %s)", FILEARCHIVE_VERSION);
    LMAPI->add_module("filearchive", tbuf);

    /* Set up the commands */
    LMAPI->add_command("index", "List available files.",
                       "index [<list>]", NULL, NULL, CMD_HEADER|CMD_BODY,
                       cmd_file_index);
    LMAPI->add_command("get", "Retrieve a specific file.",
                       "get [<list>] <file>", NULL, NULL, CMD_HEADER|CMD_BODY,
                       cmd_get_file);

    /* register the variables */
    LMAPI->register_var("file-archive-status", "public:|private|public|admin|",
			"FileArchive",
                        "Who is allowed to retrieve the file archives.",
                        "file-archive-status = admin", VAR_CHOICE, VAR_ALL);
    LMAPI->register_var("file-archive-dir", NULL, "FileArchive",
                        "Where are the archives for the list located.",
                        "file-archive-dir = files", VAR_STRING, VAR_ALL);
}

void print_dir_list(char *base_path, char *path, int first)
{
    struct stat fst;
    char buf[BIG_BUF], dname[BIG_BUF];
    char buf1[BIG_BUF];
    int status;
    LDIR mydir;

    LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%s/%s", base_path, path);
    status = LMAPI->walk_dir(buf, &dname[0], &mydir);
    if(!status) {
        if(first) {
            LMAPI->spit_status("List '%s' has no file archive available for retrieval.",
                    LMAPI->get_var("list"));
        } else {
            LMAPI->spit_status("Error processing directory '%s'.", path);
        }
        return;
    }

    if(first) {
        LMAPI->spit_status("");
        LMAPI->result_printf("%10s  %10s  %s\n", "SIZE", "DATE", "FILENAME");
        LMAPI->result_printf("%10s  %10s  %s\n", "----------", "----------",
                             "------------------------------------");
    }

    while(status) {
        /* We will skip any .files.  There shouldn't be any. */
        if(dname[0] != '.') {
            LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%s/%s", path, dname);
            LMAPI->buffer_printf(buf1, sizeof(buf1) - 1, "%s/%s", base_path, buf);
            stat(buf1, &fst);
            if(fst.st_mode & S_IFDIR) {
                print_dir_list(base_path, buf, 0);
            } else {
                char tbuf[11];
                strftime(tbuf, 11, "%m/%d/%Y", localtime(&fst.st_mtime));
                LMAPI->result_printf("%10ld  %10s  %s\n", (long)fst.st_size,
                                     tbuf, buf+1);
            }
        }
        status = LMAPI->next_dir(mydir,&dname[0]);
    }
    LMAPI->close_dir(mydir);
    return; 
}

CMD_HANDLER(cmd_file_index)
{
    const char *listptr;
    const char *access;
    struct list_user user;
    char buf[BIG_BUF];

    /* Okay.. Param could be either be empty or 'list' */
    /*
     * If we are in NOLIST mode, the first parameter can be the list name.
     * It can also be blank. If we are in list-request mode, then there 
     * should be no parameters
     */
 
    if((strcmp("nolist", LMAPI->get_string("mode")) == 0) &&
       params->num == 1) {
        listptr = params->words[0];
    } else {
        listptr = LMAPI->get_var("list");
    }

    if(!listptr) {
        LMAPI->spit_status("No list in current context.");
        return CMD_RESULT_CONTINUE;
    }

    if(!LMAPI->list_valid(listptr)) {
        LMAPI->nosuch(listptr);
        return CMD_RESULT_CONTINUE;
    }

    if(!LMAPI->set_context_list(listptr)) {
        LMAPI->spit_status("Unable to switch context to list '%s'.", listptr);
        return CMD_RESULT_CONTINUE;
    }

    /*
     * See if they have permission to read the directory
     */
    access = LMAPI->get_string("file-archive-status");
    if(!strcasecmp(access, "private")) {
        if(!LMAPI->user_find_list(LMAPI->get_string("list"), LMAPI->get_string("realsender"), &user)) {
            LMAPI->spit_status("File archive contents are only viewable by list members.");
            return CMD_RESULT_CONTINUE;
        }
    } else if(!strcasecmp(access,"admin")) {
        int failure = 1;
        if(LMAPI->user_find_list(LMAPI->get_string("list"), LMAPI->get_string("realsender"), &user)) {
            if(LMAPI->user_hasflag(&user, "ADMIN")) {
                failure = 0;
            }
        }
        if(failure) {
             LMAPI->spit_status("File archive contents are only viewable by list admins.");
             return CMD_RESULT_CONTINUE;
        }
    }

    /*
     * Okay, we've set up the list and read the variables in, so now
     * we need to see if they have a file directory
     */
    if(!LMAPI->get_var("file-archive-dir")) {
        LMAPI->spit_status("List '%s' has no file archive available for retrieval.",
                listptr);
        return CMD_RESULT_CONTINUE;
    }


    LMAPI->listdir_file(buf, listptr, LMAPI->get_string("file-archive-dir"));
    print_dir_list(buf, "", 1);

    return CMD_RESULT_CONTINUE;
}

CMD_HANDLER(cmd_get_file)
{
    char buf[BIG_BUF];
    char buf2[BIG_BUF];
    const char *access;
    struct list_user user;
    const char *filename, *listptr;

    /* Okay.. Param could be either be empty or 'list' */
    /*
     * If we are in NOLIST mode, the first parameter can be the list name.
     * It can also be blank. If we are in list-request mode, then there 
     * should be one parameter
     */
    if(params->num == 2) {
        if(strcmp("nolist", LMAPI->get_string("mode")) == 0) {
            listptr = params->words[0];
        } else {
            LMAPI->spit_status("You may not specify a list in this context.");
            return CMD_RESULT_CONTINUE;
        }
        filename = params->words[1];
    } else if(params->num == 1) {
        listptr = LMAPI->get_var("list");
        filename = params->words[0];
    } else {
        LMAPI->spit_status("Which file would you like to get? Use 'index' for a list of available files.");
       return CMD_RESULT_CONTINUE;
    }

    if(!listptr) {
        LMAPI->spit_status("No list in current context.");
        return CMD_RESULT_CONTINUE;
    }

    if(!LMAPI->list_valid(listptr)) {
        LMAPI->nosuch(listptr);
        return CMD_RESULT_CONTINUE;
    }

    if(!LMAPI->set_context_list(listptr)) {
        LMAPI->spit_status("Unable to switch context to list '%s'.", listptr);
        return CMD_RESULT_CONTINUE;
    }

    /*
     * See if they have permission to read the directory
     */
    access = LMAPI->get_string("file-archive-status");
    if(!strcasecmp(access, "private")) {
        if(!LMAPI->user_find_list(LMAPI->get_string("list"), LMAPI->get_string("realsender"), &user)) {
            LMAPI->spit_status("File archive contents are only retrievable by list members.");
            return CMD_RESULT_CONTINUE;
        }
    } else if(!strcasecmp(access,"admin")) {
        int failure = 1;
        if(LMAPI->user_find_list(LMAPI->get_string("list"), LMAPI->get_string("realsender"), &user)) {
            if(LMAPI->user_hasflag(&user, "ADMIN")) {
                failure = 0;
            }
        }
        if(failure) {
             LMAPI->spit_status("File archive contents are only retrievable by list admins.");
             return CMD_RESULT_CONTINUE;
        }
    }

    /*
     * Okay, we've set up the list and read the variables in, so now
     * we need to see if they have a file directory
     */
    if(!LMAPI->get_var("file-archive-dir")) {
        LMAPI->spit_status("List '%s' has no file archive available for retrieval.",
                listptr);
        return CMD_RESULT_CONTINUE;
    }

    /*
     * now for the security checks on the filename itself.  We want to
     * abort if we find '/../', '/./' or a leading '/' in the path as those
     * could allow access to files outside of the file archive directory
     */
    if(filename[0] == '/') {
        LMAPI->spit_status("Leading slash on a get file request is illegal.");
        return CMD_RESULT_CONTINUE;
    }
    if(strstr(filename, "/../") || strstr(filename, "/./")) {
        LMAPI->spit_status("The directories '.' and '..' are illegal in a get file request.");
        return CMD_RESULT_CONTINUE;
    }

    LMAPI->spit_status("File '%s' sent in separate message.", filename);
    LMAPI->buffer_printf(buf, sizeof(buf) - 1, "File Request '%s'", filename);
    LMAPI->set_var("task-form-subject", buf, VAR_TEMP);
    LMAPI->set_var("task-no-footer", "yes", VAR_TEMP);
    LMAPI->buffer_printf(buf2, sizeof(buf2) - 1, "%s/%s", LMAPI->get_string("file-archive-dir"), filename);
    LMAPI->listdir_file(buf, listptr, buf2);
    LMAPI->send_textfile(LMAPI->get_string("realsender"), buf);
    LMAPI->clean_var("task-form-subject", VAR_TEMP);
    LMAPI->clean_var("task-no-footer", VAR_TEMP);
    return CMD_RESULT_CONTINUE;
}
