#ifndef _LPM_MODS_H
#define _LPM_MODS_H

#ifdef DYNMOD
struct listserver_modref {
#ifndef WIN32
   void *	handle;
#else
   HMODULE	handle;
#endif /* WIN32 */
   char *name;
   struct listserver_modref *next;
};

extern void init_modrefs();
extern void nuke_modrefs();
#endif /* DYNMOD */

extern int load_all_modules(void);
extern int init_all_modules(void);
extern int unload_all_modules(void);
extern int switch_context_all_modules(void);
extern int upgrade_all_modules(int prev, int cur);
extern int listupgrade_all_modules(int prev, int cur);

#endif /* _LPM_MODS_H */
