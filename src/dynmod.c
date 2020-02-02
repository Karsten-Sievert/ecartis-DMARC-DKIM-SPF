#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

/* All Our Happy Includes! */
#include "cmdarg.h"
#include "command.h"
#include "cookie.h"
#include "core.h"
#include "file.h"
#include "fileapi.h"
#include "flag.h"
#include "forms.h"
#include "hooks.h"
#include "lcgi.h"
#include "list.h"
#include "modes.h"
#include "module.h"
#include "mystring.h"
#include "parse.h"
#include "regexp.h"
#include "smtp.h"
#include "tolist.h"
#include "unmime.h"
#include "user.h"
#include "variables.h"
#include "funcparse.h"

/* And the API structure */
#include "lpm-api.h"

#include "lpm-mods.h"

extern struct LPMAPI API;

#if defined(__linux__)
#define HAS_DYNAMIC_LINK
#endif

#if defined(BSDMOD)
#define HAS_DYNAMIC_LINK
#endif

#if defined (BSDIMOD)
#define HAS_DYNAMIC_LINK
#endif


#ifndef HAS_DYNAMIC_LINK
#error Your OS has not been ported to Dynamic link yet.  Undefine DYNMOD.
#endif

#ifndef MAX_FNAME
#define MAX_FNAME 1024
#endif

#ifdef HAS_DYNAMIC_LINK
#include <dlfcn.h>
#ifndef _DL_FUNCPTR_DEFINED
typedef void (*dl_funcptr)();
#endif
#ifdef OBSDMOD
#define LOAD_FUNCNAME_PATTERN "_%.200s_load"
#define INIT_FUNCNAME_PATTERN "_%.200s_init"
#define UNLOAD_FUNCNAME_PATTERN "_%.200s_unload"
#define UPGRADE_FUNCNAME_PATTERN "_%.200s_upgrade"
#define LISTUPGRADE_FUNCNAME_PATTERN "_%.200s_upgradelist"
#define SWITCH_FUNCNAME_PATTERN "_%.200s_switch_context"
#else
#define LOAD_FUNCNAME_PATTERN "%.200s_load"
#define INIT_FUNCNAME_PATTERN "%.200s_init"
#define UNLOAD_FUNCNAME_PATTERN "%.200s_unload"
#define UPGRADE_FUNCNAME_PATTERN "%.200s_upgrade"
#define LISTUPGRADE_FUNCNAME_PATTERN "%.200s_upgradelist"
#define SWITCH_FUNCNAME_PATTERN "%.200s_switch_context"
#endif
#endif /* HAS_DYNAMIC_LINK */

#ifdef OBSDMOD
# define RTLD_LISTSERVER 0
#endif
#ifndef RTLD_LISTSERVER
# ifdef RTLD_GLOBAL
#  ifdef RTLD_NOW
#   define RTLD_LISTSERVER (RTLD_GLOBAL|RTLD_NOW)
#  else
#   define RTLD_LISTSERVER (RTLD_GLOBAL)
#  endif
# else
#  ifdef RTLD_NOW
#   define RTLD_LISTSERVER (RTLD_NOW)
#  else
#   error Could not find appropriate definition for RTLD_LISTSERVER.
#  endif
# endif
#endif

struct listserver_modref *mod_handles;

/* Initialize module references */
void init_modrefs()
{
    mod_handles = NULL;
}

/* Add a module reference */
void add_modref(void *handle, char *name)
{
    struct listserver_modref *tempref;

    tempref = (struct listserver_modref *)malloc(sizeof(struct listserver_modref));
    tempref->handle = handle;
    tempref->name = strdup(name);
    tempref->next = mod_handles;
    mod_handles = tempref;
}

/* Close all modules and nuke their modrefs */
void nuke_modrefs()
{
    struct listserver_modref *tempref, *tempref2;

    tempref = mod_handles;
    while(tempref) {
        tempref2 = tempref->next;
        dlclose(tempref->handle);
        if(tempref->name) free(tempref->name);
        free(tempref);
        tempref = tempref2;
    }
    mod_handles = NULL;
}

/* Load a given module */
int load_a_module(char *pathname, char *modname)
{
    struct stat s;
    char funcname[MAX_FNAME];
    void *dlHandle;
    dl_funcptr loadFunc;

    stat(pathname, &s);
    if(S_ISREG(s.st_mode) && ((s.st_mode & S_IRUSR) != 0)) {
        /* We have a module, so, try to load it */

        dlHandle = dlopen(pathname, RTLD_LISTSERVER);
        if(dlHandle == NULL) {
            filesys_error(dlerror());
            return -1;
        }
        buffer_printf(funcname, sizeof(funcname) - 1, LOAD_FUNCNAME_PATTERN, modname);
        loadFunc = dlsym(dlHandle, funcname);
        if(loadFunc) {
            (*loadFunc)(&API);
            add_modref(dlHandle, modname);
        } else {
            dlclose(dlHandle);
        }
    }
    return 1;
}

/* initialize all loaded modules */
int init_all_modules()
{
    struct listserver_modref *tempref;
    char funcname[MAX_FNAME];
    dl_funcptr initFunc;

    tempref = mod_handles;
    while(tempref) {
        buffer_printf(funcname, sizeof(funcname) - 1, INIT_FUNCNAME_PATTERN, tempref->name);
        initFunc = dlsym(tempref->handle, funcname);
        if(initFunc)
            (*initFunc)();
        tempref = tempref->next;
    }
    return 1;
}

/* unload all loaded modules */
int unload_all_modules()
{
    struct listserver_modref *tempref;
    char funcname[MAX_FNAME];
    dl_funcptr unloadFunc;

    tempref = mod_handles;
    while(tempref) {
        buffer_printf(funcname, sizeof(funcname) - 1, UNLOAD_FUNCNAME_PATTERN, tempref->name);
        unloadFunc = dlsym(tempref->handle, funcname);
        if(unloadFunc)
            (*unloadFunc)();
        tempref = tempref->next;
    }
    return 1;
}

/* upgrade all loaded modules */
int upgrade_all_modules(int prev, int cur)
{
    struct listserver_modref *tempref;
    char funcname[MAX_FNAME];
    dl_funcptr upgradeFunc;

    tempref = mod_handles;
    while(tempref) {
        buffer_printf(funcname, sizeof(funcname) - 1, UPGRADE_FUNCNAME_PATTERN, tempref->name);
        upgradeFunc = dlsym(tempref->handle, funcname);
        if(upgradeFunc)
            (*upgradeFunc)(prev, cur);
        tempref = tempref->next;
    }
    return 1;
}

/* upgrade list data for all loaded modules */
int listupgrade_all_modules(int prev, int cur)
{
    struct listserver_modref *tempref;
    char funcname[MAX_FNAME];
    dl_funcptr upgradeFunc;

    tempref = mod_handles;
    while(tempref) {
        buffer_printf(funcname, sizeof(funcname) - 1, LISTUPGRADE_FUNCNAME_PATTERN, tempref->name);
        upgradeFunc = dlsym(tempref->handle, funcname);
        if(upgradeFunc)
            (*upgradeFunc)(prev, cur);
        tempref = tempref->next;
    }
    return 1;
}

/* switch context for all loaded modules */
int switch_context_all_modules()
{
    struct listserver_modref *tempref;
    char funcname[MAX_FNAME];
    dl_funcptr switchFunc;

    tempref = mod_handles;
    while(tempref) {
        buffer_printf(funcname, sizeof(funcname) - 1, SWITCH_FUNCNAME_PATTERN, tempref->name);
        switchFunc = dlsym(tempref->handle, funcname);
        if(switchFunc)
            (*switchFunc)();
        tempref = tempref->next;
    }
    return 1;
}

/* Load ALL modules */
int load_all_modules()
{
    char modname[MAX_FNAME];
    char pathname[MAX_FNAME];
    char *c;
    int status;
    LDIR mydir;

    status = walk_dir(get_string("listserver-modules"), &modname[0], &mydir);

    while(status) {
        if(strcasestr(modname, ".lpm")) {
            c = strrchr(modname, '.');
            if(c) {
                buffer_printf(pathname, sizeof(pathname) - 1, "%s/%s", get_string("listserver-modules"), modname);
                *c = '\0';
                log_printf(9,"Trying to load '%s' (%s)...\n", pathname, modname);
                load_a_module(pathname, modname);
                log_printf(15,"Dynmod: Finished loading module '%s'\n", modname);
            }
        }
        status = next_dir(mydir, &modname[0]);
        log_printf(15, "Dynmod: Looping, status '%d' (%s)\n", status, modname);
    }
    return 1;
}
