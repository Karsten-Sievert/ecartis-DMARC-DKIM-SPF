#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "alias.h"
#include "variables.h"
#include "core.h"
#include "mystring.h"
#include "moderate.h"
#include "liscript.h"
#include "fileapi.h"
#include "trust.h"

static const char *_sortorder = NULL;
struct list_vars *listdata = NULL;
struct var_data **sortedvars = NULL;

static int count_all_vars();
static int var_cmp(const void *e1, const void *e2);

/* Hash a variable name to a hash bucket */
int var_hash(const char *varname)
{
    int i, len, val;

    len = strlen(varname);
    val = 0;
    for(i = 0; i < len; i++) {
        val += tolower(varname[i]);
    }
    return val % HASHSIZE;
}

/* get the current variable value at a specific level */
const char *get_cur_varval_level(struct var_data *var, int level)
{
    if(level == VAR_TEMP) {
        if(var->temp && (var->flags & VAR_TEMP)) return var->temp;
    } else if (level == VAR_LIST) {
        if(var->list && (var->flags & VAR_LIST)) return var->list;
    } else if (level == VAR_SITE) {
        if(var->site && (var->flags & VAR_SITE)) return var->site;
    } else if (level == VAR_GLOBAL) {
        if(var->global && (var->flags & VAR_GLOBAL)) return var->global;
    } else {
        return NULL;
    }
    return NULL;
}

/* get the current variable value at a specific level or above */
const char *get_cur_varval_level_default(struct var_data *var, int level)
{
    switch(level) {
        case VAR_TEMP:
            if(var->temp && (var->flags & VAR_TEMP)) return var->temp;
        case VAR_LIST:
            if(var->list && (var->flags & VAR_LIST)) return var->list;
        case VAR_SITE:
            if(var->site && (var->flags & VAR_SITE)) return var->site;
        case VAR_GLOBAL:
            if(var->global && (var->flags & VAR_GLOBAL)) return var->global;
        default:
            return var->defval;
    }
}

/* Get the current value of a variable from it's variable record */
const char *get_cur_varval(struct var_data *var)
{
    if(var->temp && (var->flags & VAR_TEMP)) return var->temp;
    if(var->list && (var->flags & VAR_LIST)) return var->list;
    if(var->site && (var->flags & VAR_SITE)) return var->site;
    if(var->global && (var->flags & VAR_GLOBAL)) return var->global;
    return var->defval;
}

/* Lookup the variable and return it's record if it exists, else null */
struct var_data *find_var_rec(const char *varname)
{
    const char *alias = lookup_alias(varname);
    int hash;
    struct var_data *temp;

    if(!alias)
        alias = varname;

    hash = var_hash(alias);
    temp = listdata->bucket[hash];

    while(temp) {
        if(strcasecmp(temp->name, alias) == 0)
            return temp;
        temp = temp->next;
    }
    return NULL;
}

/* clean up values on a variable record */
void clean_var(const char *varname, int flags)
{
    struct var_data *temp = find_var_rec(varname);
    const char *varval = NULL;

    if(!temp) {
        log_printf(9, "clean_var: variable '%s' doesn't exist.\n", varname);
        return;
    }
    if(temp->flags & VAR_LOCKED) {
        log_printf(1, "clean_var: Attempting to clean locked variable '%s'\n",
                   varname);
        return;
    }
    temp->flags &= ~VAR_NOEXPAND;

    if((flags & VAR_TEMP) && temp->temp) {
        log_printf(9, "clean_var: cleaning var '%s' (TEMP)\n", temp->name);
        if(temp->type != VAR_DATA)
            free(temp->temp);
        temp->temp = NULL;
    }
    if((flags & VAR_LIST) && temp->list) {
        log_printf(9, "clean_var: cleaning var '%s' (LIST)\n", temp->name);
        if(temp->type != VAR_DATA)
            free(temp->list);
        temp->list = NULL;
    }
    if((flags & VAR_SITE) && temp->site) {
        log_printf(9, "clean_var: cleaning var '%s' (SITE)\n", temp->name);
        if(temp->type != VAR_DATA)
            free(temp->site);
        temp->site = NULL;
    }
    if((flags & VAR_GLOBAL) && temp->global) {
        log_printf(9, "clean_var: cleaning var '%s' (GLOBAL)\n", temp->name);
        if(temp->type != VAR_DATA)
            free(temp->global);
        temp->global = NULL;
    }

    if(temp->expanded){
        free(temp->expanded);
        temp->expanded = NULL;
    }
    varval = get_cur_varval(temp);
    if(!varval || strstr(varval, "<$") == NULL)
        temp->flags  |= VAR_NOEXPAND;
}

/* Wipe all non-protected variables */
void wipe_vars(int flags)
{
    struct var_data *temp;
    int i;
    const char *varval = NULL;

    for(i = 0; i < HASHSIZE; i++) {
        temp = listdata->bucket[i];

        while(temp) {
            if(temp->flags & VAR_LOCKED) {
                log_printf(9, "wipe_vars: skipping locked variable '%s'\n",
                           temp->name);
                temp = temp->next;
                continue;
            }
            temp->flags &= ~VAR_NOEXPAND;

            if((flags & VAR_TEMP) && temp->temp) {
                if(temp->type != VAR_DATA)
                    free(temp->temp);
                temp->temp = NULL;
            }
            if((flags & VAR_LIST) && temp->list) {
                if(temp->type != VAR_DATA)
                    free(temp->list);
                temp->list = NULL;
            }
            if((flags & VAR_SITE) && temp->site) {
                if(temp->type != VAR_DATA)
                    free(temp->site);
                temp->site = NULL;
            }
            if((flags & VAR_GLOBAL) && temp->global) {
                if(temp->type != VAR_DATA)
                    free(temp->global);
                temp->global = NULL;
            }
           if(temp->expanded) {
               free(temp->expanded);
               temp->expanded = NULL;
           }
           varval = get_cur_varval(temp);
           if(!varval || strstr(varval, "<$") == NULL)
               temp->flags  |= VAR_NOEXPAND;
           temp = temp->next;
        }
    }
}

/* Destroy the variable hash table */
void nuke_vars(void)
{
    struct var_data *temp, *temp2;
    int i;

    for(i = 0; i < HASHSIZE; i++) {
        temp = listdata->bucket[i];

        while(temp) {
            if(temp->flags & VAR_LOCKED) {
                log_printf(9, "nuke_vars: variable '%s' was never unlocked.\n",
                           temp->name);
            }

            temp2 = temp->next;
            if(temp->name) free(temp->name);
            if(temp->description) free(temp->description);
            if(temp->section) free(temp->section);
            if(temp->example) free(temp->example);
            if(temp->defval) free(temp->defval);
            if(temp->global && temp->type != VAR_DATA) free(temp->global);
            if(temp->site && temp->type != VAR_DATA) free(temp->site);
            if(temp->list && temp->type != VAR_DATA) free(temp->list);
            if(temp->temp && temp->type != VAR_DATA) free(temp->temp);
            if(temp->expanded) free(temp->expanded);
            if(temp->choices) free(temp->choices);
            free(temp);
            temp = temp2;
        }
        listdata->bucket[i] = NULL;
    }
}

/* register a variables information */
void register_var(const char *varname, const char *defval,
                  const char *section, const char *desc, 
                  const char *example, enum var_type type, int flags)
{
    struct var_data *temp = find_var_rec(varname);
    int hash;

    if(temp) {
        log_printf(0, "Attempting to reregister variable '%s'\n", varname);
        return;
    }
    if(flags & VAR_LOCKED) {
        flags &= ~VAR_LOCKED;
    }
    if(flags & VAR_NOEXPAND) {
        flags &= ~VAR_NOEXPAND;
    }

    hash = var_hash(varname);
    temp = (struct var_data *)malloc(sizeof(struct var_data));
    temp->name = strdup(varname);
    temp->description = NULL;
    temp->section = NULL;
    temp->example = NULL;
    temp->defval= NULL;
    temp->global = NULL;
    temp->site = NULL;
    temp->list = NULL;
    temp->temp = NULL;
    temp->expanded = NULL;
    temp->choices = NULL;
    temp->type = type;
    if(desc)
        temp->description = strdup(desc);
    if(section)
        temp->section = strdup(section);
    if(example)
        temp->example = strdup(example);
    if(defval) {
        /* lets do some validation of the variables */
        switch (temp->type) {
            case VAR_DATA:
                temp->defval = (char *)defval;
                break;
            case VAR_CHOICE: {
                char *deftmp, *delim, *dv;
                deftmp = strdup(defval);
                delim = strchr(deftmp, ':');
                dv = strchr(deftmp, '|');
                if(delim == NULL || dv == NULL) {
                   log_printf(0, "Choice variable '%s' had no list of choices.\n",
                              varname);
                   if(temp->name) free(temp->name);
                   if(temp->description) free(temp->description);
                   if(temp->section) free(temp->section);
                   if(temp->example) free(temp->example);
                   free(deftmp);
                   free(temp);
                   return;
                } else {
                   if((*(delim+1) != '|') || (delim[strlen(delim)-1] != '|')) {
                       log_printf(0, "Choice variable '%s' had invalid list of choices.\n",
                                  varname);
                       if(temp->name) free(temp->name);
                       if(temp->description) free(temp->description);
                       if(temp->section) free(temp->section);
                       if(temp->example) free(temp->example);
                       free(temp);
                       free(deftmp);
                       return;
                   }
                   temp->choices = strdup(delim+1);
                   if(delim != defval) {
                       *delim = '\0';
                       temp->defval = strdup(defval);
                   }
                   free(deftmp);
                }
                break;
            }
            case VAR_BOOL:
                if(atoi(defval) || !strcasecmp(defval, "yes") || 
                   !strcasecmp(defval, "on") || !strcasecmp(defval, "y") ||
                   !strcasecmp(defval, "true")) {
                    temp->defval = strdup("1");
                } else {
                    temp->defval = strdup("0");
                }
                break;
            case VAR_TIME:
                if(atoi(defval)) {
                    temp->defval = strdup(defval);
                } else {
                    char buf[BIG_BUF];
                    time_t now = time(NULL);
                    buffer_printf(buf, sizeof(buf) - 1, "%d", (int)now);
                    temp->defval = strdup(buf);
                }
                break;
            case VAR_INT:
                if(atoi(defval)) {
                    temp->defval = strdup(defval);
                } else {
                    temp->defval = strdup("0");
                }
                break;
            case VAR_DURATION:
            case VAR_STRING:
                temp->defval = strdup(defval);
                break;
            default:
                /* We should never get here!! */
                log_printf(0, "Attempt to register var of unknown type '%s'\n",
                           varname);
                return;
        }
    }
    temp->flags = flags;
    if(!temp->defval || strstr(temp->defval, "<$") == NULL)
        temp->flags |= VAR_NOEXPAND;
    temp->next = listdata->bucket[hash];
    listdata->bucket[hash] = temp;
}

/* Set a variables value */
void set_var(const char *varname, const char *varval, int level)
{
    struct var_data *listtemp;
    char *temp = NULL;

    listtemp = find_var_rec(varname);
    if(!listtemp) {
        if(!get_bool("global-pass")) {
            log_printf(0, "set_var: Setting an unknown variable '%s'\n",
                       varname);
        }
        return;
    }
    if(listtemp->flags & VAR_LOCKED) {
        if(listtemp->type == VAR_DATA) {
            log_printf(1, "Attempt to set locked variable '%s' to '%x'\n",
                       varname, varval);
        } else {
            log_printf(1, "Attempt to set locked variable '%s' to '%s'\n",
                       varname, varval);
        }
    }
    if((listtemp->flags & VAR_RESTRICTED) && (level & VAR_LIST) &&
       !(level & VAR_RESTRICTED)) {
        const char *listname;

        listname = get_var("list");

        if (!listname)
           return;

        if (!is_trusted(listname))
           return;
    }

    if(level & VAR_LOCKED) {
        level &= ~VAR_LOCKED;
    }
    if(level & VAR_INTERNAL) {
        level &= ~VAR_INTERNAL;
    }
    if(level & VAR_NOEXPAND) {
        level &= ~VAR_NOEXPAND;
    }
    if(level & VAR_RESTRICTED) {
        level &= ~VAR_RESTRICTED;
    }

    if((listtemp->flags & level) == 0) {
        log_printf(0, "set_var: Variable '%s' illegal at level %d\n", varname,
                   level);
        return;
    }

    listtemp->flags &= ~VAR_NOEXPAND;

    /* lets do some validation of the variables */
    switch (listtemp->type) {
        case VAR_DATA:
            temp = (char *)varval;
            break;
        case VAR_CHOICE: {
            char buf[BIG_BUF];
            buffer_printf(buf, sizeof(buf) - 1, "|%s|", varval);
            if(strstr(listtemp->choices, buf) == NULL) {
                log_printf(0, "Attempt to set invalid value for choice variable '%s'\n", varname);
                return;
            }
            temp = strdup(varval);
            break;
        }
        case VAR_BOOL:
            if(varval) {
                if(atoi(varval) || !strcasecmp(varval, "yes") || 
                   !strcasecmp(varval, "on") || !strcasecmp(varval, "y") ||
                   !strcasecmp(varval, "true")) {
                    temp = strdup("1");
                } else {
                    temp = strdup("0");
                }
            }
            break;
        case VAR_TIME:
            if(varval && atoi(varval)) {
                temp = strdup(varval);
            } else {
                char buf[BIG_BUF];
                time_t now = time(NULL);
                buffer_printf(buf, sizeof(buf) - 1, "%d", (int)now);
                temp = strdup(buf);
            }
            break;
        case VAR_INT:
            if(varval && atoi(varval)) {
                temp = strdup(varval);
            } else {
                temp = strdup("0");
            }
            break;
        case VAR_DURATION:
        case VAR_STRING:
            if(varval)
                temp = strdup(varval);
            break;
        default:
            /* We should never get here!! */
            log_printf(0, "Attempt to set a variable of unknown type '%s'\n",
                       varname);
            return;
    }

    switch (level) {
        case VAR_GLOBAL:
            if(listtemp->global && listtemp->type != VAR_DATA)
                free(listtemp->global);
            listtemp->global = temp;
            break;
        case VAR_SITE:
            if(listtemp->site && listtemp->type != VAR_DATA)
                free(listtemp->site);
            listtemp->site = temp;
            break;
        case VAR_LIST:
            if(listtemp->list && listtemp->type != VAR_DATA)
                free(listtemp->list);
            listtemp->list = temp;
            break;
        case VAR_TEMP:
            if(listtemp->temp && listtemp->type != VAR_DATA)
                free(listtemp->temp);
            listtemp->temp = temp;
            break;
        default:
            log_printf(1, "Attempt to set var '%s' at unknown level %d\n",
                       varname, level);
            return;
    }
    if(listtemp->type == VAR_DATA) {
      log_printf(9, "Setvar: '%s'='%x' at level %d\n", varname, varval, level);
    } else {
      log_printf(9, "Setvar: '%s'='%s' at level %d\n", varname, varval, level);
    }
    if(listtemp->expanded) {
        free(listtemp->expanded);
        listtemp->expanded = NULL;
    }
    temp = (char *)get_cur_varval(listtemp);
    if(!temp || strstr(temp, "<$") == NULL)
        listtemp->flags |= VAR_NOEXPAND;
}

/* Return the variable as a data_type */
const void *get_data(const char *varname)
{
    struct var_data *tmp = find_var_rec(varname);

    if(!tmp) {
        log_printf(9, "get_data: Query for non-variable '%s'\n", varname);
        return NULL;
    }
    if(tmp->type != VAR_DATA) {
        log_printf(9, "get_data: Variable '%s' is not type DATA\n", varname);
        return NULL;
    }
    return (void *)get_cur_varval(tmp);
}

/* Return a copy of the variable unexpanded */

const char *get_var_unexpanded(const char *varname)
{
    struct var_data *tmp = find_var_rec(varname);
    const char *c;
    
    if(!tmp) {
        log_printf(9, "get_var: Query for non-variable '%s'\n", varname);
        return NULL;
    }
    c = get_cur_varval(tmp);
	log_printf(19, "Getting %s unexpaned as %s\n", varname, c);
	return c;
}

/* Return the variable data as a raw pointer */
const char *get_var(const char *varname)
{
    struct var_data *tmp = find_var_rec(varname);
    const char *c;
    
    if(!tmp) {
        log_printf(9, "get_var: Query for non-variable '%s'\n", varname);
        return NULL;
    }
    c = get_cur_varval(tmp);
    if(c && (tmp->type == VAR_STRING || tmp->type == VAR_CHOICE)) {
        if((tmp->flags & VAR_NOEXPAND) == 0) {
            char tbuf[BIG_BUF];
            liscript_parse_line(c, tbuf, sizeof(tbuf) - 1);
            if(tmp->expanded) free(tmp->expanded);
            log_printf(19,"Expanded '%s' -> '%s'\n", c, tbuf);
            tmp->expanded = strdup(tbuf);
            c = tmp->expanded;
        }
    }
    return c;
}

/* Convert the raw data to a boolean value and return it */
const int get_bool(const char *varname)
{
    struct var_data *tmp = find_var_rec(varname);
    const char *c;
    
    if(!tmp) {
        log_printf(9, "get_bool: Query for non-variable '%s'\n", varname);
        return 0;
    }
    if(tmp->type != VAR_BOOL) {
        log_printf(9, "get_bool: Variable '%s' is not of type BOOL\n", varname);
    }

    c = get_cur_varval(tmp);

    if(!c)
       return 0;

    return atoi(c);
}

/* Convert the raw data to an integer and return it */
const int get_number(const char *varname)
{
    struct var_data *tmp = find_var_rec(varname);
    const char *c;

    if(!tmp) {
        log_printf(9, "get_number: Query for non-variable '%s'\n", varname);
        return 0;
    }
    if(tmp->type != VAR_INT && tmp->type != VAR_TIME) {
        log_printf(9,"get_number: Variable '%s' is not available as type INT\n",varname);
        return 0;
    }
    
    c = get_cur_varval(tmp);
    if(tmp->type == VAR_TIME) {
       if(!c)
          return (int)time(NULL);
       return atoi(c);
    } else {
       if(!c)
          return 0;
       return atoi(c);
    }
}

/* Return the raw value as a string, NULL is the empty string */
const char *get_string(const char *varname)
{
    struct var_data *tmp = find_var_rec(varname);
    const char *c;
    /* used if we are asked for the string value of a null VAR_TIME var */
    static char datebuf[BIG_BUF];

    if(!tmp) {
        log_printf(9, "get_string: Query for non-variable '%s'\n", varname);
        return "";
    }
    if((tmp->type != VAR_STRING) && (tmp->type != VAR_TIME) &&
       (tmp->type != VAR_DURATION) && (tmp->type != VAR_CHOICE)) {
        log_printf(9, "get_string: Variable '%s' is not available as type STRING\n",
                   varname);
        return "";
    }
    
    c = get_cur_varval(tmp);
    if(tmp->type == VAR_TIME) {
        if(!c)  {
            time_t now = time(NULL);
            get_date(datebuf, sizeof(datebuf), now);
            return &datebuf[0];
        } else {
            get_date(datebuf, sizeof(datebuf), atoi(c));
            return &datebuf[0];
        }
    } else {
        if(!c)
            return "";
        if(tmp->flags & VAR_NOEXPAND) {
            return c;
        } else {
            char tbuf[BIG_BUF];
            liscript_parse_line(c, tbuf, sizeof(tbuf) - 1);
            if(tmp->expanded) free(tmp->expanded);
            log_printf(19,"Expanded '%s' -> '%s'\n", c, tbuf);
            tmp->expanded = strdup(tbuf);
            return tmp->expanded;
        }
    }
    return "";
}

/* Parse the string for a date format and return it as a number of seconds */
const int get_seconds(const char *varname)
{
    struct var_data *tmp = find_var_rec(varname);
    const char *c;
    int res = 0;
    int total = 0;

    if(!tmp) {
        log_printf(9, "get_seconds: Query for non-variable '%s'\n", varname);
        return 0;
    }
    if(tmp->type != VAR_DURATION) {
        log_printf(9, "get_seconds: Variable '%s' is not of type DURATION\n",
                   varname);
        return 0;
    }
    
    c = get_cur_varval(tmp);

    if(!c)
        return 0;
    while(*c) {
        while (*c && isspace((int)(*c))) c++;
        while (*c && isdigit((int)(*c))) { res = res * 10 + (*c - '0'); c++; }
        while (*c && isspace((int)(*c))) c++;
        switch(*c) {
            case 'd': total += (24 * 60 * 60) * res; c++; break;
            case 'h': total += (60 * 60) * res; c++; break;
            case 'm': total += (60) * res; c++; break;
            default : total += res; c++; break;
        }
    }
    return total;    
}

/* Initialize the hash table */
void init_vars(void)
{
    int i;
    listdata = (struct list_vars *)malloc(sizeof(struct list_vars));
    for(i = 0; i < HASHSIZE; i++)
        listdata->bucket[i] = NULL;
}

static int varwalkcount, varwalkmax;

struct var_data *start_varlist(void)
{
    int count, num, i;

    count = count_all_vars();

    sortedvars = (struct var_data **)malloc(sizeof(struct var_data *)*count);
    if(!sortedvars) {
        log_printf(0, "Unable to allocate memory for sorting.\n");
        return NULL;
    }

    num = 0;

    for (i = 0; i < HASHSIZE; i++) {
        struct var_data *tmp = listdata->bucket[i];
        while(tmp) {
            if(!(tmp->flags & VAR_INTERNAL)) {
                sortedvars[num++] = tmp;
            }
            tmp = tmp->next;
        }
    }
    _sortorder = NULL;
    qsort(sortedvars, count, sizeof(struct var_data *), var_cmp);

    varwalkcount = 0;
    varwalkmax = count;

    return(sortedvars[0]);
}

struct var_data *next_varlist ()
{
    if (!sortedvars) return NULL;

    varwalkcount++;
    if (varwalkcount >= varwalkmax)
       return NULL;

    return sortedvars[varwalkcount];
}

void finish_varlist(void)
{
    if (!sortedvars) return;
    varwalkcount = 0; varwalkmax = 0;
    free(sortedvars);
    sortedvars = NULL;
}

void lock_var(const char *varname)
{
    struct var_data *temp = find_var_rec(varname);
    if(temp)
        temp->flags |= VAR_LOCKED;
}

void restrict_var(const char *varname)
{
    struct var_data *temp = find_var_rec(varname);
    log_printf(8,"restrict_var: %s\n", varname);
    if(temp)
        temp->flags |= VAR_RESTRICTED;
}

void unlock_var(const char *varname)
{
    struct var_data *temp = find_var_rec(varname);
    if(temp)
        temp->flags &= ~VAR_LOCKED;
}

static int count_vars(int level)
{
    int i;
    int count = 0;
    int trusted;

    if (level == VAR_LIST) {
       if (get_var("list")) 
          trusted = is_trusted(get_var("list"));
       else trusted = 0;
    } else trusted = 1;

    for (i = 0; i < HASHSIZE; i++) {
        struct var_data *tmp = listdata->bucket[i];
        while(tmp) {
            if((tmp->flags & level) && !(tmp->flags & VAR_INTERNAL)
               && (!(tmp->flags & VAR_RESTRICTED) || trusted))
                count++;
            tmp = tmp->next;
        }
    }
    return count;
}

static int count_all_vars()
{
    int i;
    int count = 0;
    for (i = 0; i < HASHSIZE; i++) {
        struct var_data *tmp = listdata->bucket[i];
        while(tmp) {
            if(!(tmp->flags & VAR_INTERNAL))
                count++;
            tmp = tmp->next;
        }
    }
    return count;
}


static int var_cmp(const void *e1, const void *e2)
{
    struct var_data *v1, *v2;
    int cmpval;

    v1 = *(struct var_data **)e1;
    v2 = *(struct var_data **)e2;

    /* Sanity check! */
    if (v1->section && !v2->section) return -1;
    if (v2->section && !v1->section) return 1; 
    if (!v1->section && !v2->section) return 0;

    if(_sortorder) {
        char *s1, *s2;
        char buf1[BIG_BUF];
        char buf2[BIG_BUF];
        buffer_printf(buf1, sizeof(buf1) - 1, ":%s:", v1->section);
        buffer_printf(buf2, sizeof(buf2) - 1, ":%s:", v2->section);
        s1 = strstr(_sortorder, buf1);
        s2 = strstr(_sortorder, buf2);
        if(s1 && !s2) return -1;
        else if(s2 && !s1) return 1;
        else if(s1 && s2) {
            if(s1 < s2) return -1;
            else if(s2 < s1) return 1;
            else return 0;
        }
    }

    cmpval = strcasecmp(v1->section, v2->section);

    if (!cmpval) {
        cmpval = strcasecmp(v1->name,v2->name);
    }

    return cmpval;
}

void write_configfile(const char *filename, int level, const char *sortorder)
{
    /* Eventually, sortorder will provide a way to sort sections.  NYI */
    FILE *ofile = open_exclusive(filename, "w");
    int i, trusted;
    int count = 0, num = 0;
    struct var_data **arr = NULL;
    char *lastsection;

    if(!ofile) {
        log_printf(0, "Unable to open config file '%s' for writing.\n",
                   filename);
        return;
    }

    if (level & VAR_LIST) {
       if (get_var("list"))
           trusted = is_trusted(get_var("list"));
       else trusted = 0;
    } else trusted = 1;

    count = count_vars(level);
    if(count == 0) {
        log_printf(0, "No variables to write at level %d.\n", level);
        close_file(ofile);
        return;
    }
   
    arr = (struct var_data **)malloc(sizeof(struct var_data *)*count);
    if(!arr) {
        log_printf(0, "Unable to allocate memory for sorting.\n");
        close_file(ofile);
        return;
    }
 
    for (i = 0; i < HASHSIZE; i++) {
        struct var_data *tmp = listdata->bucket[i];
        while(tmp) {
            if((tmp->flags & level) && !(tmp->flags & VAR_INTERNAL) &&
               ((!(tmp->flags & VAR_RESTRICTED)) || trusted)) {
                arr[num++] = tmp;
            }
            tmp = tmp->next;
        }
    }
    _sortorder = sortorder;
    qsort(arr, count, sizeof(struct var_data *), var_cmp);
    _sortorder = NULL;

    lastsection = NULL;

    for(i = 0; i < count; i++) {
        char *desc = arr[i]->description;
        const char *val;
        int col = 0;

        if (lastsection) {
           if(!arr[i]->section) {
              free(lastsection);
              lastsection = NULL;
              write_file(ofile,"\n\n# Miscellaneous Settings\n\n");
           } else if(strcasecmp(arr[i]->section,lastsection) != 0) {
              unsigned int counter;

              free(lastsection);
              lastsection = upperstr(arr[i]->section);

              for (counter = 0; counter < (strlen(lastsection) + 4);
                   counter++) {
                  write_file(ofile,"#");
              }

              write_file(ofile,"\n# %s #\n", lastsection);

              for (counter = 0; counter < (strlen(lastsection) + 4);
                   counter++) {
                  write_file(ofile,"#");
              }
              write_file(ofile,"\n\n");
           }
        } else {
           if (arr[i]->section) {
              unsigned int counter;

              lastsection = upperstr(arr[i]->section);
              for (counter = 0; counter < (strlen(lastsection) + 4);
                   counter++) {
                  write_file(ofile,"#");
              }

              write_file(ofile,"\n# %s #\n", lastsection);

              for (counter = 0; counter < (strlen(lastsection) + 4);
                   counter++) {
                  write_file(ofile,"#");
              }
              write_file(ofile,"\n\n");
           }
        }

        write_file(ofile, "# %s\n", arr[i]->name);

        while(desc ? *desc : 0) {
            if(col == 0) {
                write_file(ofile, "# ");
                col = 2;
            }
            if((*desc == ' ' && col > 65)) {
                col = 0;
                write_file(ofile, "\n");
            } else {
                write_file(ofile, "%c", *desc);
                col++;
            }
            desc++;
        }
        if (arr[i]->example)
           write_file(ofile, "\n# Example: %s\n", arr[i]->example);
        write_file(ofile, "#\n");
        val = get_cur_varval_level(arr[i], level);
        if(val) {
            if(arr[i]->type == VAR_CHOICE) {
                 char *z = strchr(val, ':');
                 if(z) *z = '\0';
            }
            write_file(ofile, "%s = %s\n\n", arr[i]->name,
                 (arr[i]->type == VAR_BOOL) ?
                    (val ? (*val == '1' ? "true" : "false" ) : "false") : 
                    (val ? val : ""));
        } else {
            val = get_cur_varval_level_default(arr[i], level);            
            if(arr[i]->type == VAR_CHOICE) {
                 char *z = strchr(val, ':');
                 if(z) *z = '\0';
            }
            write_file(ofile, "# %s = %s\n\n", arr[i]->name,
                 (arr[i]->type == VAR_BOOL) ?
                    (val ? (*val == '1' ? "true" : "false" ) : "false") : 
                    (val ? val : ""));
        }        
    }
    close_file(ofile);
}

void write_configfile_section(const char *filename, int level,
                              const char *section)
{
    FILE *ofile = open_exclusive(filename, "w");
    int i;

    if(!ofile) {
        log_printf(0, "Unable to open config file '%s' for writing.\n",
                   filename);
        return;
    }

    for (i = 0; i < HASHSIZE; i++) {
        struct var_data *tmp = listdata->bucket[i];
        while(tmp) {
            if((tmp->flags & level) && !(tmp->flags & VAR_INTERNAL)) {
                if(strcasecmp(tmp->section, section) == 0) {
                    char *desc = tmp->description;
                    const char *val;
                    int col = 0;
                    write_file(ofile, "# %s\n", tmp->name);
                    while(desc ? *desc : 0) {
                        if(col == 0) {
                            write_file(ofile, "# ");
                            col = 2;
                        }
                        if((*desc == ' ' && col > 65)) {
                            col = 0;
                            write_file(ofile, "\n");
                        } else {
                            write_file(ofile, "%c", *desc);
                            col++;
                        }
                        desc++;
                    }
                    if (tmp->example)
                       write_file(ofile, "\n# Example: %s", tmp->example);
                    write_file(ofile, "#\n");
                    val = get_cur_varval_level(tmp, level);
                    if(val)
                        write_file(ofile, "%s = %s\n\n", tmp->name,
                             (tmp->type == VAR_BOOL) ? 
                             (val ? (*val == '1' ? "true" : "false" ) : "false") : 
                             (val ? val : ""));
                    else {
                        val = get_cur_varval_level_default(tmp, level);
                        if (val)
                           write_file(ofile, "# %s = %s\n\n", tmp->name,
                                (tmp->type == VAR_BOOL) ? 
                                (val ? (*val == '1' ? "true" : "false" ) : "false") : 
                                (val ? val : ""));
                        else
                           write_file(ofile, "# %s = \n\n",
                                tmp->name);
                    }
                }
            }
            tmp = tmp->next;
        }
    }
    close_file(ofile);
}


void init_regvars(void)
{
    char buf[BIG_BUF];

    /* I'm going to register cookies here too since I don't have any
       better place */
    register_cookie('M', "modpost-expiration-time", NULL, expire_modpost);
  
    /* and register some variables */ 
    register_var("path", ".", NULL, NULL, NULL, VAR_STRING,
                 VAR_GLOBAL|VAR_INTERNAL);
    register_var("queuefile", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_GLOBAL|VAR_INTERNAL);
    register_var("listserver-root", NULL, "Location",
                 "The path to the root of the Listserver installation",
                 "listserver-root = /usr/local/listserver", VAR_STRING,
                 VAR_GLOBAL);
    register_alias("listar-root", "listserver-root");
    register_alias("sllist-root", "listserver-root");
    register_alias("ecartis-root", "listserver-root");
    register_var("config-file", "config", "Basic Configuration",
                 "The name of the list-specific configuration file.",
                 "config-file = config", VAR_STRING, VAR_GLOBAL|VAR_SITE);
    register_var("listserver-modules", NULL, "Location",
                 "The path to the directory containing the LPM modules.",
                 "listserver-modules = /usr/local/lists/modules", VAR_STRING,
                 VAR_GLOBAL);
    register_alias("listar-modules", "listserver-modules");
    register_alias("sllist-modules", "listserver-modules");
    register_alias("ecartis-modules", "listserver-modules");
    register_var("listserver-conf", NULL, "Location",
                 "The path to the listserver configuration files.",
                 "listserver-conf = /usr/local/mylists/configs",
                 VAR_STRING, VAR_GLOBAL);
    register_alias("listar-conf", "listserver-conf");
    register_alias("sllist-conf", "listserver-conf");
    register_alias("ecartis-conf", "listserver-conf");
    register_var("listserver-data", NULL, "Location",
                 "The path to the listserver data root.",
                 "listserver-data = /usr/local/mylists/data",
                 VAR_STRING, VAR_GLOBAL|VAR_SITE);
    register_alias("listar-data", "listserver-data");
    register_alias("sllist-data", "listserver-data");
    register_alias("ecartis-data", "listserver-data");
    register_var("mailserver", "localhost", "SMTP",
                 "The name of the outging SMTP server to use.",
                 "mailserver = mail.host1.dom", VAR_STRING,
                 VAR_GLOBAL|VAR_SITE);
    register_var("smtp-errors-file", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_INTERNAL|VAR_GLOBAL);
    register_var("form-send-as", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_TEMP|VAR_INTERNAL);
    register_var("listserver-admin", "root@localhost", "Addresses",
                 "The email address of the human in charge of the listserver.",
                 "listserver-admin = user1@host2.dom", VAR_STRING,
                 VAR_GLOBAL|VAR_SITE);
    register_alias("listar-admin", "listserver-admin");
    register_alias("sllist-admin", "listserver-admin");
    register_alias("ecartis-admin", "listserver-admin");
    register_alias("listar-owner", "listserver-admin");
    register_alias("sllist-owner", "listserver-admin");
    register_alias("ecartis-owner", "listserver-admin");
    register_alias("listserver-owner", "listserver-admin");
    register_var("task-expires", "no", NULL, NULL, NULL, VAR_BOOL,
                 VAR_INTERNAL|VAR_TEMP);
    register_var("results-subject-override", NULL, NULL, NULL, NULL,
                 VAR_STRING, VAR_INTERNAL|VAR_TEMP);
    register_var("initial-cmd", NULL, NULL, NULL, NULL,
                 VAR_STRING, VAR_INTERNAL|VAR_GLOBAL);
    register_var("task-form-subject", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_INTERNAL|VAR_TEMP);
    register_var("global-pass", "no", NULL, NULL, NULL, VAR_BOOL,
                 VAR_INTERNAL|VAR_TEMP);
    register_var("preserve-queue", "no", "Debugging",
                 "Controls whether to remove queue file after processing.",
                 "preserve-queue = yes", VAR_BOOL, VAR_ALL);
    register_var("lists-root", "<$listserver-data>/lists", "Location",
                 "Location of the directory containing all the list info.",
                 "lists-root = lists", VAR_STRING, VAR_GLOBAL|VAR_SITE);
    register_var("mode", "nolist", NULL, NULL, NULL, VAR_STRING,
                 VAR_TEMP|VAR_GLOBAL|VAR_INTERNAL);
    register_var("send-as", NULL, "SMTP",
                 "Controls what the SMTP return path is set to.",
                 "send-as = list2-bounce@test2.dom",  VAR_STRING, VAR_ALL);
    register_var("list-owner", NULL, "Basic Configuration",
                 "Defines an email address to reach the list owner(s).",
                 "list-owner = list2-admins@hostname.dom", VAR_STRING, VAR_ALL);
    register_var("list", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_INTERNAL|VAR_GLOBAL|VAR_TEMP);
    register_var("smtp-last-error", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_INTERNAL|VAR_TEMP);
    register_var("listserver-full-name", SERVICE_NAME_MC, "Addresses",
                 "The friendly name used to identify the listserver.",
                 "listserver-full-name = List Server",
                 VAR_STRING, VAR_GLOBAL|VAR_SITE);
    register_alias("listar-full-name", "listserver-full-name");
    register_alias("sllist-full-name", "listserver-full-name");
    register_alias("ecartis-full-name", "listserver-full-name");
    register_var("listserver-address", SERVICE_ADDRESS, "Addresses",
                 "The email address for the listserver control account.",
                 "listserver-address = listserv@myhost.dom",
                 VAR_STRING, VAR_GLOBAL|VAR_SITE);
    register_alias("listar-address", "listserver-address");
    register_alias("sllist-address", "listserver-address");
    register_alias("ecartis-address", "listserver-address");
    register_var("fakequeue", NULL, NULL, NULL, NULL, VAR_BOOL,
                 VAR_INTERNAL|VAR_GLOBAL);
    register_var("listserver-infile", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_INTERNAL|VAR_GLOBAL);
    register_var("cookie-for", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_INTERNAL|VAR_TEMP);
    register_var("realsender", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_INTERNAL|VAR_GLOBAL);
    register_var("cookie-expiration-time", "1 d", "Timeouts",
                 "How long until a generated cookie expires.",
                 "cookie-expiration-time = 3 d 6 h", VAR_DURATION, VAR_ALL);
    register_var("modpost-expiration-time", NULL, "Timeouts",
                 "How long until a moderated post cookie expires.",
                 "modpost-expiration-time = 2 h", VAR_DURATION, VAR_ALL);
    register_var("reply-expires-time", "1 d", "Timeouts",
                 "How long until an automatic reply expires from the mailbox",
                 "reply-expires-time = 3 h", VAR_DURATION, VAR_NOLIST);
    register_var("form-cc-address", NULL, "SMTP",
                 "Who should be cc'd on any tasks/forms that the server sends.",
                 "form-cc-address = user2@host1.dom", VAR_STRING, VAR_ALL);
    register_var("form-reply-to", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_INTERNAL|VAR_TEMP);
    register_var("task-no-footer", "no", "Basic Configuration",
                 "Should the messages produced by the server have a footer with version information",
                 "task-no-footer = yes", VAR_BOOL, VAR_ALL);
    register_var("site-config-file", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_GLOBAL|VAR_INTERNAL);
    register_var("deny-822-from", "no", "Address Handling",
                 "Should the RFC822 From: header be trusted for sender.",
                 "deny-822-from = no", VAR_BOOL, VAR_ALL);
    register_var("deny-822-bounce", "no", "Address Handling",
                 "Should the RFC822 Resent-From: header be trusted for sender.",
                 "deny-822-bounce = yes", VAR_BOOL, VAR_ALL);
    register_var("just-unquoted", "no", NULL, NULL, NULL, VAR_BOOL,
                 VAR_INTERNAL|VAR_GLOBAL);
    register_var("just-unmimed", "no", NULL, NULL, NULL, VAR_BOOL,
                 VAR_INTERNAL|VAR_GLOBAL);
    register_var("unmime-moderate-mode", "no", NULL, NULL, NULL, VAR_BOOL,
                 VAR_ALL|VAR_INTERNAL);
    register_var("resent-from", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_INTERNAL|VAR_GLOBAL);
    register_var("jobeoj-wrapper", "no", NULL, NULL, NULL, VAR_BOOL,
                 VAR_INTERNAL|VAR_GLOBAL);
    register_var("ignore-subject-commands", "no", "Basic Configuration",
                 "Should the server ignore commands in the subject line.",
                 "ignore-subject-commands = false", VAR_BOOL,
                 VAR_GLOBAL| VAR_SITE);
    register_var("fromaddress", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_INTERNAL|VAR_GLOBAL);
    register_var("socket-timeout", "30 s", "Socket IO",
                 "How long should the server wait on reading a socket.",
                 "socket-timeout = 5 m", VAR_DURATION, VAR_GLOBAL|VAR_SITE);
    register_var("debug", "0", "Debugging", "How much logging should be done.",
                 "debug = 10", VAR_INT, VAR_ALL);
    register_var("cur-parse-line", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_INTERNAL|VAR_GLOBAL);
    register_var("approved-address", "<$list>-repost@<$hostname>", "Addresses",
                 "Address to which approved/rejected/modified moderated posts should be sent.",
                 "approved-address = mylist-repost@myhost.dom", VAR_STRING,
                 VAR_ALL);
    register_var("moderator", NULL, "Moderation",
                 "Address for the list moderator(s).",
                 "moderator = foolist-moderators@hostname.dom", VAR_STRING,
                 VAR_ALL);
    register_var("moderate-notify-nonsub", "no", "Moderation", 
                 "Should posts from non-subscribers be acked if they are moderated.",
                 "moderate-notify-nonsub = true", VAR_BOOL, VAR_ALL);
    register_var("moderate-force-notify", "no", NULL, NULL, NULL,
                 VAR_BOOL, VAR_TEMP|VAR_INTERNAL);
    register_var("moderated-approved-by", NULL, NULL, NULL, NULL,
                 VAR_STRING, VAR_GLOBAL|VAR_INTERNAL);    
    register_var("max-rcpt-tries", "5", "SMTP",
                 "How many times to attempt reading a RCPT TO: response.",
                 "max-recpt-tries = 3", VAR_INT, VAR_GLOBAL|VAR_SITE);
    register_var("sendmail-sleep", "no", "SMTP",
                 "Should we attempt to sleep a short time between message recipients.",
                 "sendmail-sleep = on", VAR_BOOL, VAR_GLOBAL|VAR_SITE);
    register_var("sendmail-sleep-length", "3 s", "SMTP",
                 "Override if you need the 'Sendmail Sleeper' option at a different duration than the 3 second default.",
                 "sendmail-sleep-length = 1 s", VAR_DURATION,
                 VAR_GLOBAL|VAR_SITE);

    register_var("per-user-queuefile", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_INTERNAL|VAR_TEMP);
    register_var("per-user-datafile", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_INTERNAL|VAR_TEMP);
    register_var("per-user-list", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_INTERNAL|VAR_TEMP);
    register_var("per-user-address", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_INTERNAL|VAR_TEMP);
    register_var("per-user-data", NULL, NULL, NULL, NULL, VAR_DATA,
                 VAR_INTERNAL|VAR_TEMP);
    register_var("megalist", "no", "ToList",
                 "Should we process this list on-disk instead of in memory? This disables the receipient list sorting and list-merging functionality of Ecartis, in order to prevent large memory footprint operations.  It is useful for lists where the receipient list is too large to effectively do memory-based operations on.",
                 "megalist = true", VAR_BOOL, VAR_ALL);
    register_var("smtp-queue-chunk", NULL, "SMTP",
                 "Maximum recipients per message submitted to the mail server.  Larger lists will be split into chunks of this size.",
                 "smtp-queue-chunk = 25", VAR_INT, VAR_ALL);
    register_var("per-user-modifications", "no", "ToList",
                 "Do we do per-user processing for list members.",
                 "per-user-modifications = false", VAR_BOOL, VAR_ALL);
    register_var("tolist-send-pause", "0", "ToList",
                 "How long (in milliseconds) do we sleep between SMTP chunks.",
                 "tolist-send-pause = 30", VAR_INT, VAR_ALL);
    register_var("unmimed-file", "no", NULL, NULL, NULL, VAR_BOOL,
                 VAR_INTERNAL|VAR_GLOBAL);
    register_var("unquoted-file", "no", NULL, NULL, NULL, VAR_BOOL,
                 VAR_INTERNAL|VAR_GLOBAL);
    register_var("unmime-first-level", "no", NULL, NULL, NULL, VAR_BOOL,
                 VAR_INTERNAL|VAR_TEMP);
    register_var("unmime-quiet", "no", "MIME",
                 "Should the listserver report when it strips MIME attachments.",
                 "unmime-quiet = no", VAR_BOOL, VAR_ALL);
    register_var("validate-users", "no", "Debugging",
                 "Perform a minimal validation of user@host.dom on all users in the list's user file and log errors.",
                 "validate-users = true", VAR_BOOL, VAR_ALL);
    register_var("no-loose-domain-match", "no", "ToList",
                 "Should the server treat users of a subdomain as users of the domain for validation purposes.",
                 "no-loose-domain-match = on", VAR_BOOL, VAR_ALL);
    register_var("default-flags", "|ECHOPOST|", "Basic Configuration",
                 "Default flags given to a user when they are subscribed.",
                 "default-flags = |NOPOST|DIGEST|", VAR_STRING, VAR_ALL);
    register_var("global-blacklist", "banned", "Files",
                 "Global file containing regular expressions for users who are not allowed to subscribe to lists hosted on this server.",
                 "global-blacklist = banned", VAR_STRING, VAR_GLOBAL|VAR_SITE);
    register_var("logfile", NULL, "Debugging",
                 "Filename where debugging log information will be stored.",
                 "logfile = ./server.log", VAR_STRING, VAR_GLOBAL|VAR_SITE);
    register_var("full-bounce", "no", "SMTP",
                 "Should bounces contain the full message or only the headers.",
                 "full-bounce = false", VAR_BOOL, VAR_ALL);
    register_var("error-include-queue", "yes", "Error Handling",
                 "Should error reports contain the queue associated with that run",
                 "error-include-queue = yes", VAR_BOOL, VAR_ALL);
    register_var("address-failure", "no", NULL, NULL, NULL,
                 VAR_BOOL, VAR_TEMP|VAR_INTERNAL);
    register_var("moderate-quiet","no",NULL,NULL,NULL,
                 VAR_BOOL, VAR_TEMP|VAR_INTERNAL);
    register_var("moderate-include-queue", "no", "Moderation", 
                 "Should moderated messages contain the full message that triggered moderation?",
                 "moderate-include-queue = yes", VAR_BOOL, VAR_ALL);
    register_var("moderate-verbose-subject", "yes", "Moderation",
                 "Should moderated messages have a more informative subject?",
                 "moderate-verbose-subject = yes", VAR_BOOL, VAR_ALL);
    buffer_printf(buf, sizeof(buf) - 1, "%s.hlp", SERVICE_NAME_LC);
    register_var("no-command-file", buf, "Files",
                 "This is a global file to send if a message to the main listserver or request address has no commands.",
                 "no-command-file = helpfile",VAR_STRING, VAR_GLOBAL|VAR_SITE);
    register_var("submodes-file", "submodes", "Files",
                 "File containing list specific customized subscription modes.",
                 "submodes-file = submodes", VAR_STRING, VAR_ALL);
    register_var("submodes-mode", NULL, NULL, NULL, NULL, VAR_STRING,
                 VAR_TEMP|VAR_INTERNAL);
    register_var("rabid-mime","no","MIME",
                 "Should ABSOLUTELY no attachments, EVEN text/plain, be allowed",
                 "rabid-mime = no",VAR_BOOL,VAR_ALL);
    register_var("smtp-socket", "25", "SMTP",
                 "Which socket should the SMTP server be contacted on.",
                 "smtp-socket = 26", VAR_INT, VAR_SITE|VAR_GLOBAL);
    register_var("cgi-template-dir", "<$listserver-data>/templates",
                 "CGI", "Directory for CGI gateway templates.",
                 "cgi-template-dir = <$listserver-data>/templates",
                 VAR_STRING, VAR_ALL);
    register_var("version-file", "<$lists-root>/SITEDATA/version", NULL, NULL,
                 NULL, VAR_STRING, VAR_GLOBAL|VAR_INTERNAL);
    register_var("verbose-moderate-fail", "yes", "Moderation", 
                 "When a moderator approves a message but it is rejected, should the message in question be included in the rejection note?",
                 "verbose-moderate-fail = yes", VAR_BOOL, VAR_GLOBAL|VAR_SITE|VAR_LIST);

    register_var("assume-lists-valid", "no", "Misc",
                 "Should we assume that all list directories are valid or should we perform checks",
                 "assume-lists-valid = yes", VAR_BOOL, VAR_GLOBAL|VAR_SITE);
    register_var("expire-all-cookies","yes","Cookies", 
                 "Should we expire cookies for all lists on initial run? Should only be set to 'no' on installations with a huge (multi-thousand) number of lists.",
                 "expire-all-cookies = yes", VAR_BOOL, VAR_GLOBAL|VAR_SITE);
    register_var("hooktype", NULL, NULL, NULL, NULL, VAR_STRING, VAR_TEMP|VAR_INTERNAL);
    register_var("cheatsheet-file", NULL, NULL, NULL, NULL, VAR_STRING, VAR_TEMP|VAR_INTERNAL);
    register_var("stocksend-extra-headers", NULL, NULL, NULL, NULL, VAR_STRING, VAR_TEMP|VAR_INTERNAL);
    register_var("form-show-listname", "no", "Misc",
                 "Should we use the list name (or RFC2369 name) instead of the listserver full name for forms on a per-list basis?  (Like admin wrappers and such.)",
                 "form-show-listname = yes", VAR_BOOL, VAR_GLOBAL|VAR_SITE|VAR_LIST);
    register_var("lock-to-user", SERVICE_NAME_LC, "Basic Configuration",
                 "What is the name of the user whose UID/GID we should lock to if we're run as root?",
                 "lock-to-user = ecartis", VAR_STRING, VAR_GLOBAL|VAR_INTERNAL);
    register_var("liscript-allow-explicit-list", "false", NULL, NULL, NULL,
                 VAR_BOOL, VAR_TEMP|VAR_INTERNAL);
    register_var("pwdfile", "<$lists-root>/SITEDATA/site-passwords",
                 "Files", "Path to the file containing sitewide passwords (used by web interface and others).",
                 "pwdfile = SITEDATA/passwd", VAR_STRING, VAR_GLOBAL|VAR_INTERNAL);
    register_var("hostname", NULL, "Basic Configuration",
                 "Hostname for URLs/addresses/headers",
                 "hostname = lists.mydomain.com", VAR_STRING,
                 VAR_GLOBAL|VAR_SITE);

    register_var("smtp-blind-blast", "false", "SMTP",
                 "Should Ecartis perform all SMTP operations without regard for result codes (e.g. trust that it was delivered)?  NOT RECOMMENDED.",
                 "smtp-blind-blast = yes", VAR_BOOL,
                 VAR_GLOBAL|VAR_SITE);
    register_var("smtp-retry-forever", "false", "SMTP",
                 "Should Ecartis continue to wait for SMTP responses for an infinite amount of time, e.g. never give up?  This can negatively impact delivery times.",
                 "smtp-retry-forever = yes", VAR_BOOL,
                 VAR_GLOBAL|VAR_SITE);
    register_var("headers-charset", NULL, "Global",
                 "Charset used in headers", "headers-charset = ISO-8859-1",
                 VAR_STRING, VAR_GLOBAL|VAR_SITE|VAR_LIST|VAR_INTERNAL);
    register_var("headers-charset-frombody", NULL, "Global",
                 "Charset used in body, possibly header", "headers-charset-frombody = ISO-8859-1",
                 VAR_STRING, VAR_GLOBAL|VAR_SITE|VAR_LIST|VAR_INTERNAL|VAR_TEMP);

    register_var("copy-requests-to", NULL, "Misc",
                 "If set, all user request results will be sent to this address.  Useful for debugging.",
                 "copy-requests-to = <$list>-admins@<$hostname>",
                 VAR_STRING, VAR_GLOBAL|VAR_SITE|VAR_LIST);
}
   
int check_duration(const char *d)
{
    if(!d)
        return 0;
    while(*d) {
        while (*d && isspace((int)(*d))) d++;
        while (*d && isdigit((int)(*d))) d++;
        while (*d && isspace((int)(*d))) d++;
        switch(*d) {
            case 'd':
            case 'h':
            case 'm':
            case 's':
                d++; break;
            default:
                if(isspace((int)*d)) break;
                else return 0;
        }
    }
    return 1;
}

void write_cheatsheet(const char *filename, const char *sortorder)
{
    /* Eventually, sortorder will provide a way to sort sections.  NYI */
    FILE *ofile = open_exclusive(filename, "w");
    int i;
    int count = 0, num = 0;
    struct var_data **arr = NULL;
    char *lastsection;

    log_printf(1, "Writing variable cheatsheet file '%s'\n", filename);

    if(!ofile) {
        log_printf(0, "Unable to open cheatsheet file '%s' for writing.\n",
                   filename);
        return;
    }

    count = count_all_vars();
   
    arr = (struct var_data **)malloc(sizeof(struct var_data *)*count);
    if(!arr) {
        log_printf(0, "Unable to allocate memory for sorting.\n");
        close_file(ofile);
        return;
    }
 
    for (i = 0; i < HASHSIZE; i++) {
        struct var_data *tmp = listdata->bucket[i];
        while(tmp) {
            if(!(tmp->flags & VAR_INTERNAL)) {
                arr[num++] = tmp;
            }
            tmp = tmp->next;
        }
    }
    _sortorder = sortorder;
    qsort(arr, count, sizeof(struct var_data *), var_cmp);
    _sortorder = NULL;

    lastsection = NULL;

    write_file(ofile,"<title>%s Variable Reference</title>\n\n",
            SERVICE_NAME_MC);

    write_file(ofile,"<P><font size=+3>%s %s Variable Reference</font></P>\n\n",
        SERVICE_NAME_MC, VER_PRODUCTVERSION_STR);

    write_file(ofile,"<P>\n<i><b>Note</b>: The 'valid' field describes the %s config files\n", SERVICE_NAME_MC);
    write_file(ofile,"where that variable is valid.  'G' means the global config file, 'V' means\n");
    write_file(ofile,"a virtual host configuration file, and 'L' means individual list files.\n</P>\n");

    write_file(ofile,"<table border=1 width=100%%>\n");

    for(i = 0; i < count; i++) {
        char *desc = arr[i]->description;

        write_file(ofile,"\t<tr>\n");

        if (lastsection) {
           if(!arr[i]->section) {
              free(lastsection);
              lastsection = NULL;
              write_file(ofile,"\t\t<td colspan=4><font size=+2>Miscellaneous Settings</font></td>\n\t</tr>\n\t<tr>\n");
              write_file(ofile,"\t<tr>\n\t<td><b>Variable</b></td><td><b>Type</b></td><td><b>Valid</b></td><td><b>Description</b></td>\n\t</tr>\n<tr>");
           } else if(strcasecmp(arr[i]->section,lastsection) != 0) {
              free(lastsection);
              lastsection = upperstr(arr[i]->section);

              write_file(ofile,"\t\t<td colspan=4><font size=+2>%s</font></td>\n\t</tr>\n\t<tr>\n", lastsection);
              write_file(ofile,"\t<tr>\n\t<td><b>Variable</b></td><td><b>Type</b></td><td><b>Valid</b></td><td><b>Description</b></td>\n\t</tr>\n<tr>");
           }
        } else {
           if (arr[i]->section) {
              lastsection = upperstr(arr[i]->section);

              write_file(ofile,"\t\t<td colspan=4><font size=+2>%s</font></td>\n\t</tr>\n", lastsection);
              write_file(ofile,"\t<tr>\n\t<td><b>Variable</b></td><td><b>Type</b></td><td><b>Valid</b></td><td><b>Description</b></td>\n\t</tr>\n<tr>");
           }
        }

        write_file(ofile,"\t\t<td valign=top><b>%s</b></td>\n", arr[i]->name);

        write_file(ofile,"\t\t<td valign=top>");
        switch (arr[i]->type) {
            case VAR_STRING:
                write_file(ofile,"string");
                break;
            case VAR_BOOL:
                write_file(ofile,"boolean");
                break;
            case VAR_INT:
                write_file(ofile,"integer");
                break;
            case VAR_DURATION:
                write_file(ofile,"duration");
                break;
            case VAR_DATA:
                write_file(ofile,"data");
                break;
            case VAR_TIME:
                write_file(ofile,"time");
                break;
            case VAR_CHOICE:
                write_file(ofile,"choice");
                break;
            default:
                write_file(ofile,"<i>unknown</i>");
        }
        write_file(ofile,"</td>\n");

        write_file(ofile,"\t\t<td valign=top>\n\t\t\t");
        
        if (arr[i]->flags & VAR_GLOBAL) {
            write_file(ofile,"G");
        }
        if (arr[i]->flags & VAR_SITE) {
            write_file(ofile,"V");
        }
        if (arr[i]->flags & VAR_LIST) {
            write_file(ofile,"L");
        }

        write_file(ofile,"\n\t\t</td>\n\t\t<td>\n\t\t\t");
        if (desc) {
            write_file(ofile,"<P>");
            while(*desc) {
                if (*desc == '<') 
                    write_file(ofile,"&lt;");
                else if (*desc == '>')
                    write_file(ofile,"&gt;");
                else
                    write_file(ofile,"%c", *desc);
            
                desc++;
            }
            write_file(ofile,"</P>\n\t\t\t");
        } else
            write_file(ofile,"<P><i>No description</i></P>\n\t\t\t");

        if (arr[i]->example)
           write_file(ofile,"Example:<BR>&nbsp;&nbsp;<code>%s</code><BR><BR>\n", 
                arr[i]->example);

        if (arr[i]->defval) {
            if (arr[i]->type == VAR_BOOL) {
                write_file(ofile,"Default value is <code>%s</code><BR>\n",
                   atoi(arr[i]->defval) ? "true" : "false");
            } else if (arr[i]->type == VAR_CHOICE) {
                char tempstr[SMALL_BUF];
                char *tempptr;

                buffer_printf(tempstr, sizeof(tempstr) - 1, "%s", arr[i]->defval);
                tempptr = strchr(tempstr,':');
                if (tempptr) *tempptr = 0;

                write_file(ofile,"Default value is <code>%s</code><BR>\n",
                   tempstr);
            } else {
                write_file(ofile,"Default value is <code>%s</code><BR>\n",
                   arr[i]->defval);
            }
        }

        write_file(ofile,"\t\t</td>\n\t</tr>\n");
    }
    write_file(ofile,"</table>\n");
    close_file(ofile);
}
