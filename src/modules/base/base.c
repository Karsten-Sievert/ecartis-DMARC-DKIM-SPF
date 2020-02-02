#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lpm.h"

struct LPMAPI *LMAPI;

FUNC_HANDLER(func_varset)
{
    sprintf(result, "%d", !(!(LMAPI->get_var(argv[0]))));
    return 1;
}

FUNC_HANDLER(func_sub)
{
    int b = atoi(argv[0]) - atoi(argv[1]);
    sprintf(result, "%d", b);
    return 1;
}

FUNC_HANDLER(func_add)
{
    int b = atoi(argv[0]) + atoi(argv[1]);
    sprintf(result, "%d", b);
    return 1;
}

FUNC_HANDLER(func_and)
{
    int b = (atoi(argv[0]) && atoi(argv[1]));
    sprintf(result, "%d", b);
    return 1;
}

FUNC_HANDLER(func_or)
{
    int b = (atoi(argv[0]) || atoi(argv[1]));
    sprintf(result, "%d", b);
    return 1;
}

FUNC_HANDLER(func_not)
{
    int b = (!atoi(argv[0]));
    sprintf(result, "%d", b);
    return 1;
}

FUNC_HANDLER(func_gte)
{
    int alldigit1 = 0, alldigit2 = 0;

    if(isdigit((int)argv[0][0]) || isdigit((int)argv[1][0])) {
        char *t = argv[0];
        while(*t && isdigit((int)*t)) t++;
        if(!*t) alldigit1 = 1;
        t = argv[1];
        while(*t && isdigit((int)*t)) t++;
        if(!*t) alldigit2 = 1;
    }

    if(alldigit1 && alldigit2) {
       int a = atoi(argv[0]);
       int b = atoi(argv[1]);
       strncpy(result, ((a >= b) ? "1" : "0"), BIG_BUF - 1);
    } else
       strncpy(result, ((strcasecmp(argv[0], argv[1]) >= 0) ? "1" : "0"), BIG_BUF - 1);
    return 1;
}

FUNC_HANDLER(func_lte)
{
    int alldigit1 = 0, alldigit2 = 0;

    if(isdigit((int)argv[0][0]) || isdigit((int)argv[1][0])) {
        char *t = argv[0];
        while(*t && isdigit((int)*t)) t++;
        if(!*t) alldigit1 = 1;
        t = argv[1];
        while(*t && isdigit((int)*t)) t++;
        if(!*t) alldigit2 = 1;
    }

    if(alldigit1 && alldigit2) {
       int a = atoi(argv[0]);
       int b = atoi(argv[1]);
       strncpy(result, ((a <= b) ? "1" : "0"), BIG_BUF - 1);
    } else
       strncpy(result, ((strcasecmp(argv[0], argv[1]) <= 0) ? "1" : "0"), BIG_BUF - 1);
    return 1;
}

FUNC_HANDLER(func_gt)
{
    int alldigit1 = 0, alldigit2 = 0;

    if(isdigit((int)argv[0][0]) || isdigit((int)argv[1][0])) {
        char *t = argv[0];
        while(*t && isdigit((int)*t)) t++;
        if(!*t) alldigit1 = 1;
        t = argv[1];
        while(*t && isdigit((int)*t)) t++;
        if(!*t) alldigit2 = 1;
    }

    if(alldigit1 && alldigit2) {
       int a = atoi(argv[0]);
       int b = atoi(argv[1]);
       strncpy(result, ((a > b) ? "1" : "0"), BIG_BUF - 1);
    } else
       strncpy(result, ((strcasecmp(argv[0], argv[1]) > 0) ? "1" : "0"), BIG_BUF - 1);
    return 1;
}

FUNC_HANDLER(func_lt)
{
    int alldigit1 = 0, alldigit2 = 0;

    if(isdigit((int)argv[0][0]) || isdigit((int)argv[1][0])) {
        char *t = argv[0];
        while(*t && isdigit((int)*t)) t++;
        if(!*t) alldigit1 = 1;
        t = argv[1];
        while(*t && isdigit((int)*t)) t++;
        if(!*t) alldigit2 = 1;
    }

    if(alldigit1 && alldigit2) {
       int a = atoi(argv[0]);
       int b = atoi(argv[1]);
       strncpy(result, ((a < b) ? "1" : "0"), BIG_BUF - 1);
    } else
       strncpy(result, ((strcasecmp(argv[0], argv[1]) < 0) ? "1" : "0"), BIG_BUF - 1);
    return 1;
}

FUNC_HANDLER(func_eql)
{
    int alldigit1 = 0, alldigit2 = 0;

    if(isdigit((int)argv[0][0]) || isdigit((int)argv[1][0])) {
        char *t = argv[0];
        while(*t && isdigit((int)*t)) t++;
        if(!*t) alldigit1 = 1;
        t = argv[1];
        while(*t && isdigit((int)*t)) t++;
        if(!*t) alldigit2 = 1;
    }

    if(alldigit1 && alldigit2) {
       int a = atoi(argv[0]);
       int b = atoi(argv[1]);
       strncpy(result, ((a == b) ? "1" : "0"), BIG_BUF - 1);
    } else
       strncpy(result, ((strcasecmp(argv[0], argv[1]) == 0) ? "1" : "0"), BIG_BUF - 1);
    return 1;
}

FUNC_HANDLER(func_eqladdr) {
    strncpy(result, (LMAPI->address_match(argv[0], argv[1]) ? "1" : "0"), BIG_BUF - 1);

    return 1;
}

/* 'help' commandarg */
CMDARG_HANDLER(cmdarg_help)
{
     struct listserver_cmdarg *temp  = LMAPI->get_cmdargs();
     fprintf(stderr, "Usage: %s [option ...]\n", SERVICE_NAME_LC);
     fprintf(stderr, "\twhere options are:\n");
     while(temp) {
         fprintf(stderr, "\t%s %s\n", temp->arg,
                 (temp->parmdesc?temp->parmdesc:""));
         temp = temp->next;
     }
     return CMDARG_EXIT;
}

/* 'end' command */
CMD_HANDLER(cmd_end)
{
    /* Are we in admin mode? */
    if (LMAPI->get_bool("adminmode")) {
        LMAPI->log_printf(0, "Commands for %s concluded, admin mode terminated.\n",
                   LMAPI->get_string("realsender"));
    }
    
    LMAPI->spit_status("Command set concluded.  No further commands will be processed.");
    return CMD_RESULT_END;
}

/* 'Signature' divider detected. */
CMD_HANDLER(cmd_signature)
{
    /* Make sure we really ARE a signature marker. */
    if (params->num != 0) return CMD_RESULT_CONTINUE;

    if (LMAPI->get_bool("adminmode")) {
        LMAPI->log_printf(0, "Commands for %s concluded, admin mode terminated.\n",
                   LMAPI->get_string("realsender"));
    }
    /* Check for prevent-second-message variable */
    if (!LMAPI->get_bool("prevent-second-message")) {
	LMAPI->spit_status("Found signature marker, ending command mode.");
    }
    return CMD_RESULT_END;
}

/* Module load */
void base_load(struct LPMAPI *api)
{
    LMAPI = api;

    /* Log module startup */
    LMAPI->log_printf(10, "Loading module Base\n");

    /* Command definitions */
    LMAPI->add_command("end", "Ends processing of list server commands",
                       "end", NULL, NULL, CMD_HEADER|CMD_BODY, cmd_end);

    LMAPI->add_command("--", "RFC-defined message/signature divider, ends processing of commands.",
                       "-- ", NULL, NULL, CMD_HEADER|CMD_BODY, cmd_signature);

    /* Cmdarg definitions */
    LMAPI->add_cmdarg("-help", 0, NULL, cmdarg_help);

    LMAPI->add_func("eql", 2, "Is first argument equal to second", func_eql);
    LMAPI->add_func("gt", 2, "Is first argument greater than second", func_gt);
    LMAPI->add_func("lt", 2, "Is first argument less than second", func_lt);
    LMAPI->add_func("lte", 2,
                    "Is first argument less than or equal to the second",
                    func_lte);
    LMAPI->add_func("gte", 2,
                    "Is first argument greater than or equal to the second",
                    func_gte);
    LMAPI->add_func("and", 2, "Are both arguments true (1)", func_and);
    LMAPI->add_func("or", 2, "Is either argument true (1)", func_or);
    LMAPI->add_func("not", 1, "Logical negation of argument", func_not);
    LMAPI->add_func("add", 2, "Adds two arguments together", func_add);
    LMAPI->add_func("sub", 2, "Subtracts second argument from first", func_sub);
    LMAPI->add_func("eqladdr", 2, "Do the two addresses match loosely",
                    func_eqladdr);
    LMAPI->add_func("varset", 1, "Is the variable set (e.g. non-null).",
                    func_varset);
}

void base_switch_context(void)
{
    LMAPI->log_printf(15, "Switching context in module Base\n");
}

int base_upgradelist(int prev, int cur)
{
    LMAPI->log_printf(10, "Upgrading lists in module Base\n");
    return 1;
}

int base_upgrade(int prev, int cur)
{
    LMAPI->log_printf(10, "Upgrading module Base\n");
    return 1;
}

void base_init(void)
{
    LMAPI->log_printf(10, "Initializing module Base\n");
}

void base_unload(void)
{
    LMAPI->log_printf(10, "Unloading module Base\n");
}

