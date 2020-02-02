#include <stdio.h>

#include "config.h"
#include "lpm-mods.h"

#include "cmdarg.h"
#include "command.h"
#include "cookie.h"
#include "core.h"
#include "file.h"
#include "fileapi.h"
#include "flag.h"
#include "forms.h"
#include "hooks.h"
#include "list.h"
#include "lcgi.h"
#include "liscript.h"
#include "modes.h"
#include "module.h"
#include "mystring.h"
#include "parse.h"
#include "regexp.h"
#include "smtp.h"
#include "submodes.h"
#include "tolist.h"
#include "unmime.h"
#include "upgrade.h"
#include "user.h"
#include "variables.h"
#include "funcparse.h"

#include "lpm-api.h"

#define LOAD_FUNCNAME_PATTERN "%.200s_load"
#define INIT_FUNCNAME_PATTERN "%.200s_init"
#define UNLOAD_FUNCNAME_PATTERN "%.200s_unload"
#define UPGRADE_FUNCNAME_PATTERN "%.200s_upgrade"
#define LISTUPGRADE_FUNCNAME_PATTERN "%.200s_upgradelist"
#define SWITCH_FUNCNAME_PATTERN "%.200s_switch_context"

struct listserver_modref *mod_handles;

extern struct LPMAPI API;

/* Initialize module references */
void init_modrefs()
{
    mod_handles = NULL;
}

/* Add a module reference */
void add_modref(HMODULE handle, char *modname)
{
    struct listserver_modref *tempref;

    tempref = (struct listserver_modref *)malloc(sizeof(struct listserver_modref));
    tempref->handle = handle;
    tempref->name = strdup(modname);
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
        FreeLibrary(tempref->handle);
        if(tempref->name) free(tempref->name);
        free(tempref);
        tempref = tempref2;
    }
    mod_handles = NULL;
}

typedef void (__cdecl *LOADFUNC)(struct LPMAPI *api);
typedef void (__cdecl *INITFUNC)(void);
typedef void (__cdecl *UNLOADFUNC)(void);
typedef int  (__cdecl *UPGRADEFUNC)(int prev, int cur);
typedef int  (__cdecl *LISTUPGRADEFUNC)(int prev, int cur);
typedef void (__cdecl *SWITCHFUNC)(void);

/* Load a given module */
int load_a_module(char *pathname, char *modname)
{
    char funcbuf[_MAX_PATH];
    char *funcname;
    HMODULE dlHandle;
    LOADFUNC loadFunc;

    if(exists_file(pathname)) {
        /* We have a module, so, try to load it */

        dlHandle = LoadLibrary(pathname);
        if(dlHandle == NULL) {
            char tbuf[BIG_BUF];

            log_printf(1,"Dynmod: Error loading %s\n", pathname);
            buffer_printf(tbuf, sizeof(tbuf) - 1, "Error loading module '%s'", modname);
            filesys_error(tbuf);
            return 0;
        }
        buffer_printf(funcbuf, sizeof(funcbuf) - 1, LOAD_FUNCNAME_PATTERN, modname);
        funcname = lowerstr(funcbuf);
        loadFunc = (LOADFUNC) GetProcAddress(dlHandle, funcname);
        if(loadFunc) {
            (*loadFunc)(&API);
            add_modref(dlHandle, modname);
            log_printf(5,"Dynmod: '%s' loaded/initialized.\n", modname);
            log_printf(15,"Dynmod: Exiting load_a_module\n");
            free(funcname);
            return 1;
        } else {
            log_printf(1,"Dynmod: '%s' was not a valid LPM.\n", modname);
            FreeLibrary(dlHandle);
            free(funcname);
            return 0;
        }
    } else return 0;
}

/* initialize all loaded modules */
int init_all_modules()
{
    struct listserver_modref *tempref;
    char funcbuf[_MAX_PATH];
    INITFUNC initFunc;

    tempref = mod_handles;
    while(tempref) {
        buffer_printf(funcbuf, sizeof(funcbuf) - 1, INIT_FUNCNAME_PATTERN, tempref->name);
        initFunc = (INITFUNC) GetProcAddress(tempref->handle, funcbuf);
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
    char funcbuf[_MAX_PATH];
    UNLOADFUNC unloadFunc;

    tempref = mod_handles;
    while(tempref) {
        buffer_printf(funcbuf, sizeof(funcbuf) - 1, UNLOAD_FUNCNAME_PATTERN, tempref->name);
        unloadFunc = (UNLOADFUNC) GetProcAddress(tempref->handle, funcbuf);
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
    char funcbuf[_MAX_PATH];
    UPGRADEFUNC upgradeFunc;

    tempref = mod_handles;
    while(tempref) {
        buffer_printf(funcbuf, sizeof(funcbuf) - 1, UPGRADE_FUNCNAME_PATTERN, tempref->name);
        upgradeFunc = (UPGRADEFUNC) GetProcAddress(tempref->handle, funcbuf);
        if(upgradeFunc)
            (*upgradeFunc)(prev, cur);
        tempref = tempref->next;
    }
    return 1;
}

/* upgrade all loaded modules */
int listupgrade_all_modules(int prev, int cur)
{
    struct listserver_modref *tempref;
    char funcbuf[_MAX_PATH];
    LISTUPGRADEFUNC upgradeFunc;

    tempref = mod_handles;
    while(tempref) {
        buffer_printf(funcbuf, sizeof(funcbuf) - 1, LISTUPGRADE_FUNCNAME_PATTERN, tempref->name);
        upgradeFunc = (LISTUPGRADEFUNC) GetProcAddress(tempref->handle, funcbuf);
        if(upgradeFunc)
            (*upgradeFunc)(prev, cur);
        tempref = tempref->next;
    }
    return 1;
}


/* switch the context for all loaded modules */
int switch_context_all_modules()
{
    struct listserver_modref *tempref;
    char funcbuf[_MAX_PATH];
    SWITCHFUNC switchFunc;

    tempref = mod_handles;
    while(tempref) {
        buffer_printf(funcbuf, sizeof(funcbuf) - 1, SWITCH_FUNCNAME_PATTERN, tempref->name);
        switchFunc = (SWITCHFUNC) GetProcAddress(tempref->handle, funcbuf);
        if(switchFunc)
            (*switchFunc)();
        tempref = tempref->next;
    }
    return 1;
}

/* Load ALL modules */
int load_all_modules()
{
    char modname[_MAX_PATH];
    char pathname[_MAX_PATH];
    char *c;
    int status;
    LDIR mydir;

    status = walk_dir(get_string("listserver-modules"), &modname[0], &mydir);

    while (status) {
        if (strcasestr(modname,".lpm")) {
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
        log_printf(15,"Dynmod: Looping, status '%d' (%s)\n", status, modname);
    }

    close_dir(mydir);

    return 1;
}
