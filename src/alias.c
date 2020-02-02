/* Ecartis: Modular MLM
 * ---
 * File   : alias.c
 * Purpose: Create, maintain, and query a generic alias hashtable.
 *        : This is usually used to alias variables to older names,
 *        : allowing older config files to work invisibly.
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "alias.h"
#include "core.h"
#include "mystring.h"

struct alias_info *aliases = NULL;

/* Function: alias_hash
 * Purpose : Compute a value for the hash table bucket.
 * Params  : alias - the alias for which we need a hash value.
 */
int alias_hash(const char *alias)
{
    int i, len, val;
    len = strlen(alias);
    val = 0;
    for(i = 0; i < len; i++)
        val += alias[i];
    return val % ALIASHASH;
}

/* Function: nuke_aliases
 * Purpose : Destroy the aliases table, freeing all allocated memory.
 * Params  : none
 */
void nuke_aliases(void)
{
    struct alias_data *temp, *temp2;
    int i;
    for(i = 0; i < ALIASHASH; i++) {
        temp = aliases->bucket[i];
        while(temp) {
            temp2 = temp->next;
            if(temp->aliasname) free(temp->aliasname);
            if(temp->realname) free(temp->realname);
            free(temp);
            temp = temp2;
        }
        aliases->bucket[i] = NULL;
    }
}

/* Function: init_aliases
 * Purpose : Initialize the aliases hashtable, and allocate some 
 *         : memory for it.
 * Params  : none
 */
void init_aliases(void)
{
    int i;
    aliases = (struct alias_info *)malloc(sizeof(struct alias_info));
    for(i = 0; i < ALIASHASH; i++)
        aliases->bucket[i] = NULL;
}

/* Function: register_alias
 * Purpose : Actually registers an alias, adding it to the alias hash
 *         : after allocating the appropriate memory.  It double-checks
 *         : and will not allow you to set an alias twice.
 * Params  : alias - the alias being set
 *         : real  - the actual value the alias maps to
 */
int register_alias(const char *alias, const char *real)
{
    int hash = alias_hash(alias);
    int found = 0;
    struct alias_data *temp = aliases->bucket[hash];

    /* See if this alias is already registered. */
    while(temp) {
        if(strcasecmp(temp->aliasname, alias) == 0) {
            found = 1;
            break;
        }
        temp = temp->next;
    }

    /* We can't redefine existing aliases.  Error out. */
    if(found) {
        log_printf(5, "Attempting to redefine alias '%s'\n", alias);
        return 0;
    }

    /* Allocate and store. */
    temp = (struct alias_data *)malloc(sizeof(struct alias_data));
    temp->aliasname = strdup(alias);
    temp->realname = strdup(real);
    temp->next = aliases->bucket[hash];
    aliases->bucket[hash] = temp;
    return 1;
}

/* Function: lookup_alias
 * Purpose : Looks up a string in the alias hashtable, and if it is
 *         : present, returns the string it maps to (e.g. what was
 *         : provided as 'real' in the register_alias command).
 * Params  : alias - the alias to look up
 */
const char *lookup_alias(const char *alias)
{
    int hash = alias_hash(alias);
    struct alias_data *temp = aliases->bucket[hash];

    while(temp) {
        if(strcasecmp(temp->aliasname, alias) == 0)
            return temp->realname;
        temp = temp->next;
    }
    return NULL;
}
