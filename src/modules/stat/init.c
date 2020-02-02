#include <stdio.h>

#include "stat-mod.h"

struct LPMAPI *LMAPI;

void stat_switch_context(void)
{
    LMAPI->log_printf(15, "Switching context in module Stat\n");
}

int stat_upgradelist(int prev, int cur)
{
    LMAPI->log_printf(10, "Upgrading lists in module Stat\n");
    return 1;
}

int stat_upgrade(int prev, int cur)
{
    LMAPI->log_printf(10, "Upgrading module Stat\n");
    return 1;
}

void stat_init(void)
{
    LMAPI->log_printf(10, "Initializing module Stat\n");
}

void stat_unload(void)
{
    LMAPI->log_printf(10, "Unloading module Stats\n");
}

void stat_load(struct LPMAPI *api)
{
    LMAPI = api;
    LMAPI->log_printf(10, "Loading module Stats\n");

    LMAPI->add_module("Stats","Provides various statistics and accounting, as well as traffic tracking.");

    LMAPI->add_hook("AFTER",50,hook_after_postsize);
    LMAPI->add_hook("AFTER",50,hook_after_numposts);

    LMAPI->add_command("stats",
                       "Displays current account configuration and statistics.",
                       "stats [<list>]", NULL, "stats [<address>]",
                       CMD_HEADER|CMD_BODY, cmd_stats);

    LMAPI->add_command("review","Displays current list statistics.",
                       "review [<list>]", NULL, NULL,
                        CMD_HEADER|CMD_BODY, cmd_review);

    LMAPI->add_command("modules","Send list of installed plugins.  Plugins are not required to list themselves in this registry, so hidden plugins may be present.",
                       "modules", NULL, NULL, CMD_BODY|CMD_HEADER,
                       cmd_list_modules);

    LMAPI->add_command("files", "Send list of installed list config files.   Modules are not required to register a file in order to use it, so hidden config files might be present.",
                       "files", NULL, NULL,  CMD_BODY|CMD_HEADER,
                       cmd_list_files);

    LMAPI->add_command("flags","Send list of available flags, providing a 'cheat sheet' for users.  This takes into account any local plugins.",
                       "flags", NULL, NULL, CMD_HEADER|CMD_BODY,
                       cmd_list_flags);

    LMAPI->add_command("commands","Send list of available commands, providing a 'cheat sheet' for users.  This takes into account any local plugins.",
                       "commands", NULL, NULL, CMD_HEADER|CMD_BODY,
                       cmd_list_commands);

    LMAPI->add_command("lists",
                       "Send list of mailing lists available on this site.",
                       "lists", NULL, NULL, CMD_HEADER|CMD_BODY,
                       cmd_list_lists);

    LMAPI->add_command("help","Send user the helpfile",
                       "help", NULL, NULL, CMD_BODY|CMD_HEADER, cmd_help);

    LMAPI->add_command("who","Display membership for a list",
                       "who [<list>]", NULL, NULL, CMD_HEADER|CMD_BODY,
                       cmd_list_users);

    LMAPI->add_command("which", "Display lists to which you are subscribed",
                       "which", NULL, NULL, CMD_HEADER|CMD_BODY,
                       cmd_which_lists);

    LMAPI->add_command("setname", "Sets the name for a user for user tracking in 'review'",
                       NULL, NULL, "setname <address> <name>",
                       CMD_BODY|CMD_ADMIN, cmd_setname);

    LMAPI->add_command("setrole", "Sets the position of a user for user tracking in 'review'",
                       NULL, NULL, "setrole <address> <position>",
                       CMD_BODY|CMD_ADMIN, cmd_setrole);

    LMAPI->add_command("faq","Retrieves the FAQ for a list.  Members only.",
                       "faq [<list>]", NULL, "faq",
                       CMD_BODY|CMD_HEADER, cmd_faq);

    LMAPI->add_command("info","Retrieves the info for a list.",
                       "info [<list>]", NULL, "info",
                       CMD_BODY|CMD_HEADER, cmd_info);

    LMAPI->add_file("faq","faq-file","FAQ for a given list.  Retrieved with the faq command.");
    LMAPI->add_file("info","info-file","Info to a given list.  Retrieved with the info command.");

    /* Register Variables */
    LMAPI->register_var("faq-file", "text/faq.txt", "Files",
                        "File on disk containing the list's FAQ file.",
                        "faq-file = text/faq.txt", VAR_STRING, VAR_ALL);
    LMAPI->register_var("info-file", "text/info.txt", "Files",
                        "File on disk containing the list's info file.",
                        "info-file = text/info.txt", VAR_STRING, VAR_ALL);
    LMAPI->register_var("advertise", "yes", "Basic Configuration",
                        "Does this list show up as being available.",
                        "advertise = false", VAR_BOOL, VAR_ANY);
    LMAPI->register_var("description", NULL, "Basic Configuration",
                        "Description of the list.",
                        "description = This is my special list",
                        VAR_STRING, VAR_ALL);
    LMAPI->register_var("who-status", "private:|admin|private|public|", "Misc",
                        "Who is allowed to view the list membership.",
                        "who-status = admin", VAR_CHOICE, VAR_ALL);

    /* Liscript expression functions */
    LMAPI->add_func("list_exists", 1,
                    "Check if list exists (obeys advertise setting)",
                    func_list_exists);
    LMAPI->add_func("has_stat", 2,
                    "Check if user has the given statistic.",
                    func_hasstat);
}
