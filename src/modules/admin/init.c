#include <stdio.h>

#include "admin-mod.h"

struct LPMAPI *LMAPI;

void admin_switch_context(void)
{
    LMAPI->log_printf(15, "Switching context in module Admin\n");
}

int admin_upgrade(int prev, int cur)
{
    LMAPI->log_printf(10, "Upgrading module Admin\n");
    return 1;
}

int admin_upgradelist(int prev, int cur)
{
    LMAPI->log_printf(10, "Upgrading lists in module Admin\n");
    return 1;
}

void admin_init(void)
{
    LMAPI->log_printf(10, "Initializing module Admin\n");
}

void admin_unload(void)
{
    LMAPI->log_printf(10, "Unloading module Admin\n");
}

void admin_load(struct LPMAPI *api)
{
    LMAPI = api;

    /* Module init log */
    LMAPI->log_printf(10, "Loading module Admin\n");

    /* Add us to the 'modules' list. */
    LMAPI->add_module("Admin","Admin and list management module.");

    /* Command definitions */
    LMAPI->add_command("adminend","Marks end of admin mode.",
                       NULL, NULL, "adminend", CMD_BODY|CMD_ADMIN,
                       cmd_adminend);

    LMAPI->add_command("putconf","Replaces an admin configuration file.",
                       NULL, NULL, "putconf <list> <cookie>",
                       CMD_BODY|CMD_ADMIN, cmd_adminfileput);

    LMAPI->add_command("getconf","Requests an admin configuration file.",
                       NULL, NULL, "getconf <file>", CMD_BODY|CMD_ADMIN,
                       cmd_adminfilereq);

    LMAPI->add_command("unsetfor","Changes settings for another user.", 
                       NULL, NULL, "unsetfor <address> <flag>",
                       CMD_BODY|CMD_ADMIN, cmd_adminunset);

    LMAPI->add_command("setfor","Changes settings for another user.", 
                       NULL, NULL, "setfor <address> <flag>",
                       CMD_BODY|CMD_ADMIN, cmd_adminset);

    LMAPI->add_command("become","Becomes another user, or cancels a become.", 
                       NULL, NULL, "become [<address>]",
                       CMD_BODY|CMD_ADMIN, cmd_adminbecome);

    LMAPI->add_command("adminvfy","Begin administrator mode.", 
                       NULL, NULL, "adminvfy <list> <cookie>",
                       CMD_BODY|CMD_ADMIN, cmd_admin_verify);

    LMAPI->add_command("admin","Request administrator mode.", 
                       NULL, NULL, "admin <list>", CMD_BODY|CMD_HEADER|CMD_ADMIN,
                       cmd_admin);

    LMAPI->add_command("admin2",
                       "Request administrator mode and fill out wrapper.", 
                       NULL, NULL, "admin2 <list>", CMD_BODY|CMD_ADMIN,
                       cmd_admin2);

    /* Hook definitions */
    LMAPI->add_hook("SETFLAG", 50, hook_setflag_moderator);

    /* Flag definitions */
    LMAPI->add_flag("admin","User is an administrator.",1);
    LMAPI->add_flag("superadmin","User is a super-administrator.",ADMIN_UNSETTABLE);
    LMAPI->add_flag("myopic","Administrative user does not receive admin postings.", ADMIN_SAFESET | ADMIN_SAFEUNSET);
    LMAPI->add_flag("moderator","User is a moderator.",1);
    LMAPI->add_flag("diagnostic","User is for diagnostics only, don't receive list traffic.", ADMIN_SAFESET | ADMIN_SAFEUNSET);

    /* File definitions */
    LMAPI->add_file_flagged("config","config-file",
                            "List configuration file.", FILE_NOWEBEDIT);
    LMAPI->add_file("moderator-welcome", "moderator-welcome-file", "File to send to a user when they are set to moderator.");

    /* Cookie type registrations */
    LMAPI->register_cookie('A', "adminreq-expiration-time", NULL, NULL);
    LMAPI->register_cookie('F', "filereq-expiration-time", NULL, NULL);

    /* Variable registrations */
    LMAPI->register_var("admin-actions-shown", "yes", "Administration",
                        "Are administrator actions shown to list subscribers (i.e. subscribe/unsubscribe show the administrator email address if actions are shown)",
                        "admin-actions-shown = no", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("adminreq-expiration-time", NULL, "Timeouts",
                        "How long until administrative request cookies expire.",
                        "adminreq-expiration-time = 3 h", VAR_DURATION,
                        VAR_ALL);
    LMAPI->register_var("filereq-expiration-time", NULL, "Timeouts",
                        "How long until config file request cookies expire.",
                        "filereq-expiration-time = 3 h", VAR_DURATION,
                        VAR_ALL);
    LMAPI->register_var("adminmode", NULL, NULL, NULL, NULL, VAR_BOOL,
                        VAR_INTERNAL|VAR_GLOBAL);
    LMAPI->register_var("adminspit", NULL, NULL, NULL, NULL, VAR_BOOL,
                        VAR_INTERNAL|VAR_GLOBAL);
    LMAPI->register_var("adminspit2", NULL, NULL, NULL, NULL, VAR_BOOL,
                        VAR_INTERNAL|VAR_GLOBAL);
    LMAPI->register_var("moderator-welcome-file", "text/moderator.txt",
                        "Files",
                        "File sent to a new moderator when they set MODERATOR.",
                        "moderator-welcome-file = text/moderator.txt",
                        VAR_STRING, VAR_ALL);
    LMAPI->register_var("paranoia", "no", "Administration",
                        "Are the various list config files allowed to be edited remotely for this list.",
                        "paranoia = yes", VAR_BOOL, VAR_ALL);
}
