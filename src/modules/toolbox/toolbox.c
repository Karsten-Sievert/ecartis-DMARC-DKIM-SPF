#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "toolbox.h"

CMDARG_HANDLER(cmdarg_checkusers)
{
   if(!argv[0] || !LMAPI->list_valid(argv[0])) {
       if(!argv[0]) {
           LMAPI->internal_error("Switch -checkusers requires a list as a parameter.");
       } else {
           char buf[BIG_BUF];
           LMAPI->buffer_printf(buf, sizeof(buf) - 1, "'%s' is invalid list for the -rcheckusers switch.", argv[0]);
           LMAPI->internal_error(buf);
       }
       return CMDARG_ERR;
   }
   LMAPI->set_var("list", argv[0], VAR_GLOBAL);
   LMAPI->set_var("mode", "checkuser", VAR_GLOBAL);
   LMAPI->set_var("fakequeue", "yes", VAR_GLOBAL);
   return CMDARG_OK;
}

MODE_HANDLER(mode_checkusers)
{
    char buf[BIG_BUF];
    struct list_user user;
    FILE *fp, *newfile;


    LMAPI->listdir_file(buf, LMAPI->get_string("list"), "users");
    fp = LMAPI->open_file(buf, "r");
    if(!fp)
        return MODE_ERR;
    LMAPI->listdir_file(buf, LMAPI->get_string("list"), "users.new");
    newfile = LMAPI->open_file(buf, "w");
    if(!newfile) {
        LMAPI->close_file(fp);
        return MODE_ERR;
    }

    while(LMAPI->user_read(fp, &user)) {
        if(LMAPI->check_address(user.address))
            LMAPI->write_file(newfile, "%s : %s\n", user.address, user.flags);
    }

    LMAPI->close_file(fp);
    LMAPI->close_file(newfile);
    return MODE_OK;
}
