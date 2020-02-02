#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "config.h"
#include "core.h"
#include "liscript.h"
#include "variables.h"
#include "funcparse.h"
#include "mystring.h"

struct listserver_funcdef *funcs = NULL;

void add_func(const char *name, int nargs, const char *desc, FuncFn fn)
{
    struct listserver_funcdef *temp = (struct listserver_funcdef *)malloc(sizeof (struct listserver_funcdef));
    temp->funcname = strdup(name);
    temp->desc = strdup(desc);
    temp->nargs = nargs;
    temp->fn = fn;
    temp->next = funcs;
    funcs = temp;
}

void new_funcs(void)
{
    funcs = NULL;
}

void nuke_funcs(void)
{
    struct listserver_funcdef *temp;
    while(funcs) {
        temp = funcs->next;
        free(funcs->desc);
        free(funcs->funcname);
        free(funcs);
        funcs = temp;
    }
    funcs = NULL;
}

struct listserver_funcdef *get_funcs(void)
{
    return funcs;
}

struct listserver_funcdef *find_func(const char *name)
{
    struct listserver_funcdef *temp = funcs;
    while(temp) {
        if(strcasecmp(temp->funcname, name) == 0)
            return temp;
        temp = temp->next;
    }
    return NULL;
}

int parse_function(const char *buf, char *result, char *error);

int eval_literal(const char *buf, char *result, char *error)
{
    while(*buf && isspace((int)*buf)) buf++;
    log_printf(20, "eval_litaral -- evaluating '%s'\n", buf);
    if(!*buf) {
        /* No value isn't legal */
        strncpy(result, "0", BIG_BUF - 1);
        if(error) strncpy(error, "No value present", BIG_BUF - 1);
        return 0;
    } else if(*buf == '$') {
        /* Treat it as a variable */
        char varbuf[BIG_BUF];
        int res;
        buffer_printf(varbuf, sizeof(varbuf) - 1, "[%s]", buf);
        res = liscript_parse_line(varbuf, result, BIG_BUF - 1);
        if(!res) {
            strncpy(result, "0", BIG_BUF - 1);
            if(error) buffer_printf(error, BIG_BUF - 1, "Unable to parse variable '%s'", buf);
        } else {
            log_printf(20, "eval_litaral -- var evaled to '%s'\n", result);
        }
        return res;
    } else {
        strncpy(result, buf, BIG_BUF - 1);
        if(!strcasecmp(result, "yes") || !strcasecmp(result, "true") ||
           !strcasecmp(result, "no") || !strcasecmp(result, "false")) {
            if(!strcasecmp(result, "yes") || !strcasecmp(result, "true"))
                strncpy(result, "1", BIG_BUF - 1);
            else
                strncpy(result, "0", BIG_BUF - 1);
            log_printf(20, "eval_literal -- boolean evaled to '%s'\n", result);
        } else {
            log_printf(20, "eval_literal -- literal evaled to '%s'\n", result);
        }
    }
    return 1;
}

int parse_args(const char *buf, char **argv, char *error)
{
    char curarg[BIG_BUF];
    int is_func = 0;
    int depth = 0;
    int count = 0;
    char *arg = curarg;
    char tempres[BIG_BUF];
    int res;

    /* Sanity check */
    if(!argv) {
        if(error) strncpy(error, "Argv was null to parse_args.", BIG_BUF - 1);
        return 0;
    }

    memset(curarg, 0, sizeof(curarg));
    while(*buf) {
        while(*buf && isspace((int)*buf)) buf++;
        if(*buf == ',' && depth == 0) {
            /* We should have an argument */
            if(!curarg[0]) {
                if(error) strncpy(error, "Empty argument passed.", BIG_BUF - 1);
                return 0;
            }

            /* Skip over the comma and any white space */
            buf++;
            while(*buf && isspace((int)*buf)) buf++;

            /* Evaluate the argument */
            if(is_func) {
                res = parse_function(curarg, tempres, error);
                if(!res) return 0;
            } else {
                res = eval_literal(curarg, tempres, error);
                if(!res) return 0;
            }
            /* Build the result string */
            argv[count++] = strdup(tempres);
            memset(curarg, 0, sizeof(curarg));
            is_func = 0;
            arg = curarg;
        } else if(*buf) {
            if(*buf == '(') {
                depth++;
                is_func = 1;
            }
            if(*buf == ')') depth--;
            *arg++ = *buf++;
        }
    }

    /* Copy over the last arg */
    if(!curarg[0]) {
        if(error) strncpy(error, "Empty argument passed.", BIG_BUF - 1);
        return 0;
    }
    /* Evaluate the argument */
    if(is_func) {
        res = parse_function(curarg, tempres, error);
        if(!res) return 0;
    } else {
        res = eval_literal(curarg, tempres, error);
        if(!res) return 0;
    }
    /* Build the result string */
    argv[count++] = strdup(tempres);
    return 1;
}

int parse_function(const char *buf, char *result, char *error)
{
    char *temp = NULL;
    char buf1[BIG_BUF];

    /* sanity check */
    if(!result) {
        if(error) strncpy(error, "Result buffer required.", BIG_BUF - 1);
        return 0;
    }

    /* Check for an initial buffer */
    if(!buf) {
        strncpy(result, "0", BIG_BUF - 1);
        if(error) strncpy(error, "No initial buffer", BIG_BUF - 1);
        return 0;
    }

    /* skip leading whitespace */
    while(*buf && isspace((int)*buf)) buf++;
    stringcpy(buf1, buf);
    temp = buf1;

    /* find the function name */
    while(*temp && *temp != '(') temp++;
    if(!*temp) {
        /* No function found, treat as literal */
        int res = eval_literal(buf, result, error);
        if(!res) strncpy(result, "0", BIG_BUF - 1);
        return res;
    } else {
        /* there is a function name, look it up */
        struct listserver_funcdef *funcdef;
        char *t2 = temp-1;
        *temp++ = '\0';
        while(isspace((int)*t2)) t2--;
        *++t2 = '\0';
        funcdef = find_func(buf1);
        log_printf(20, "parse_function -- Evaluating function '%s'\n", buf1);
        if(!funcdef) {
            /* No such function */
            strncpy(result, "0", BIG_BUF - 1);
            if(error) buffer_printf(error, BIG_BUF - 1,"No such function as '%s'", buf1);
            return 0;
        } else {
            /* Okay.. We have a valid function.  Let's build the arg buf */
            int count = 0;
            int copied_arg = 0;
            int depth = 1;
            char argbuf[BIG_BUF];
            char **argv = NULL;
            char *args = argbuf;
            int i;

            /* Copy all the arguments over to the buffer and count the args */
            while(*temp && depth > 0) {
                while(*temp && isspace((int)*temp)) temp++;
                if(*temp == ')') {
                    depth--;
                    if(copied_arg && (depth == 0)) count++;
                }
                if(*temp == '(') depth++;
                if((*temp == ',') && (depth == 1)) count++;
                if(depth && *temp) {
                    *args++ = *temp;
                    copied_arg = 1;
                }
                if(*temp) temp++;
            }

            /* Make sure we ended at depth 0 */
            if(depth != 0) {
                strncpy(result, "0", BIG_BUF - 1);
                if(error) strncpy(error, "Missing end parenthesis", BIG_BUF - 1);
                return 0;
            }

            /* Check the argument count */
            if(count != funcdef->nargs) {
                strncpy(result, "0", BIG_BUF - 1);
                if(error)
                    buffer_printf(error, BIG_BUF - 1, "Function '%s' has %d args, %d found",
                            funcdef->funcname, funcdef->nargs, count);
                return 0;
            }

            *args = '\0';
            /*
             * We should now be at the end of the input buffer so temp
             * should be pointing to nothing or to whitespace.
             */ 
            while(isspace((int)*temp)) temp++;
            if(*temp) {
                strncpy(result, "0", BIG_BUF - 1);
                if(error) strncpy(error, "Garbage past end of expression", BIG_BUF - 1);
                return 0;
            }
            if(funcdef->nargs) {
                argv = (char **)calloc(funcdef->nargs, sizeof(char *));
                for(i = 0; i < funcdef->nargs; i++)
                    argv[i] = NULL;
                count = parse_args(argbuf, argv, error);
                if(!count) {
                    /* Error should be filled out by parse_args */
                    strncpy(result, "0", BIG_BUF - 1);
                    /* Deallocate argv and any values it has */
                    for(i = 0; i < funcdef->nargs; i++)
                        if(argv[i]) free(argv[i]);
                    free(argv);
                    return 0;
                }
            }
            count = funcdef->fn(argv, result, error);
            /*
             * If count is 0 here we had an error in the function and
             * want to wipe result.   Error should be filled by function
             * called.
             */
            if(!count) strncpy(result, "0", BIG_BUF - 1);
            if(funcdef->nargs) {
                /* Deallocate argv and any values it has */
                for(i = 0; i < funcdef->nargs; i++)
                    if(argv[i]) free(argv[i]);
                free(argv);
            }
            return count;
        }
    }
}
