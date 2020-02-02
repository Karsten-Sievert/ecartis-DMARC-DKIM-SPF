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
#include "list.h"
#include "lcgi.h"
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

#ifdef LISTARCHIVE
extern void listarchive_load(struct LPMAPI *api);
extern void listarchive_init(void);
extern void listarchive_unload(void);
extern void listarchive_upgrade(int prev, int cur);
extern void listarchive_upgradelist(int prev, int cur);
extern void listarchive_switch_context(void);
#endif
#ifdef FILEARCHIVE
extern void filearchive_load(struct LPMAPI *api);
extern void filearchive_init(void);
extern void filearchive_unload(void);
extern void filearchive_upgrade(int prev, int cur);
extern void filearchive_upgradelist(int prev, int cur);
extern void filearchive_switch_context(void);
#endif
#ifdef BOUNCER
extern void bouncer_load(struct LPMAPI *api);
extern void bouncer_init(void);
extern void bouncer_unload(void);
extern void bouncer_upgrade(int prev, int cur);
extern void bouncer_upgradelist(int prev, int cur);
extern void bouncer_switch_context(void);
#endif
#ifdef ACCTMGR
extern void acctmgr_load(struct LPMAPI *api);
extern void acctmgr_init(void);
extern void acctmgr_unload(void);
extern void acctmgr_upgrade(int prev, int cur);
extern void acctmgr_upgradelist(int prev, int cur);
extern void acctmgr_switch_context(void);
#endif
#ifdef ADMIN
extern void admin_load(struct LPMAPI *api);
extern void admin_init(void);
extern void admin_unload(void);
extern void admin_upgrade(int prev, int cur);
extern void admin_upgradelist(int prev, int cur);
extern void admin_switch_context(void);
#endif
#ifdef STAT
extern void stat_load(struct LPMAPI *api);
extern void stat_init(void);
extern void stat_unload(void);
extern void stat_upgrade(int prev, int cur);
extern void stat_upgradelist(int prev, int cur);
extern void stat_switch_context(void);
#endif
#ifdef SEND
extern void send_load(struct LPMAPI *api);
extern void send_init(void);
extern void send_unload(void);
extern void send_upgrade(int prev, int cur);
extern void send_upgradelist(int prev, int cur);
extern void send_switch_context(void);
#endif
#ifdef DIGEST
extern void digest_load(struct LPMAPI *api);
extern void digest_init(void);
extern void digest_unload(void);
extern void digest_upgrade(int prev, int cur);
extern void digest_upgradelist(int prev, int cur);
extern void digest_switch_context(void);
#endif
#ifdef BASE
extern void base_load(struct LPMAPI *api);
extern void base_init(void);
extern void base_unload(void);
extern void base_upgrade(int prev, int cur);
extern void base_upgradelist(int prev, int cur);
extern void base_switch_context(void);
#endif
#ifdef ANTISPAM
extern void antispam_load(struct LPMAPI *api);
extern void antispam_init(void);
extern void antispam_unload(void);
extern void antispam_upgrade(int prev, int cur);
extern void antispam_upgradelist(int prev, int cur);
extern void antispam_switch_context(void);
#endif
#ifdef ADMINISTRIVIA
extern void administrivia_load(struct LPMAPI *api);
extern void administrivia_init(void);
extern void administrivia_unload(void);
extern void administrivia_upgrade(int prev, int cur);
extern void administrivia_upgradelist(int prev, int cur);
extern void administrivia_switch_context(void);
#endif
#ifdef TOOLBOX
extern void toolbox_load(struct LPMAPI *api);
extern void toolbox_init(void);
extern void toolbox_unload(void);
extern void toolbox_upgrade(int prev, int cur);
extern void toolbox_upgradelist(int prev, int cur);
extern void toolbox_switch_context(void);
#endif
#ifdef PANTOMIME
extern void pantomime_load(struct LPMAPI *api);
extern void pantomime_init(void);
extern void pantomime_unload(void);
extern void pantomime_upgrade(int prev, int cur);
extern void pantomime_upgradelist(int prev, int cur);
extern void pantomime_switch_context(void);
#endif
#ifdef LISTARGATE
extern void lsg2_load(struct LPMAPI *api);
extern void lsg2_init(void);
extern void lsg2_unload(void);
extern void lsg2_upgrade(int prev, int cur);
extern void lsg2_upgradelist(int prev, int cur);
extern void lsg2_switch_context(void);
#endif
#ifdef PASSWORD
extern void password_load(struct LPMAPI *api);
extern void password_init(void);
extern void password_unload(void);
extern void password_upgrade(int prev, int cur);
extern void password_upgradelist(int prev, int cur);
extern void password_switch_context(void);
#endif
#ifdef PERUSER
extern void peruser_load(struct LPMAPI *api);
extern void peruser_init(void);
extern void peruser_unload(void);
extern void peruser_upgrade(int prev, int cur);
extern void peruser_upgradelist(int prev, int cur);
extern void peruser_switch_context(void);
#endif

int load_all_modules(void)
{
#ifdef LISTARCHIVE
    listarchive_load(&API);
#endif
#ifdef FILEARCHIVE
    filearchive_load(&API);
#endif
#ifdef BOUNCER
    bouncer_load(&API);
#endif
#ifdef ACCTMGR
    acctmgr_load(&API);
#endif
#ifdef ADMIN
    admin_load(&API);
#endif
#ifdef STAT
    stat_load(&API);
#endif
#ifdef SEND
    send_load(&API);
#endif
#ifdef DIGEST
    digest_load(&API);
#endif
#ifdef BASE
    base_load(&API);
#endif
#ifdef ANTISPAM
    antispam_load(&API);
#endif
#ifdef ADMINISTRIVIA
    administrivia_load(&API);
#endif 
#ifdef TOOLBOX
    toolbox_load(&API);
#endif
#ifdef PANTOMIME
   pantomime_load(&API);
#endif
#ifdef LISTARGATE
   lsg2_load(&API);
#endif
#ifdef PASSWORD
   password_load(&API);
#endif
#ifdef PERUSER
   peruser_load(&API);
#endif

   /* Add any extra modules you want linked in here */
   return 1;
}

int init_all_modules(void)
{
#ifdef LISTARCHIVE
    listarchive_init();
#endif
#ifdef FILEARCHIVE
    filearchive_init();
#endif
#ifdef BOUNCER
    bouncer_init();
#endif
#ifdef ACCTMGR
    acctmgr_init();
#endif
#ifdef ADMIN
    admin_init();
#endif
#ifdef STAT
    stat_init();
#endif
#ifdef SEND
    send_init();
#endif
#ifdef DIGEST
    digest_init();
#endif
#ifdef BASE
    base_init();
#endif
#ifdef ANTISPAM
    antispam_init();
#endif
#ifdef ADMINISTRIVIA
    administrivia_init();
#endif 
#ifdef TOOLBOX
    toolbox_init();
#endif
#ifdef PANTOMIME
   pantomime_init();
#endif
#ifdef LISTARGATE
   lsg2_init();
#endif
#ifdef PASSWORD
   password_init();
#endif
#ifdef PERUSER
   peruser_init();
#endif

   /* Add any extra modules you want linked in here */
   return 1;
}

int unload_all_modules(void)
{
#ifdef LISTARCHIVE
    listarchive_unload();
#endif
#ifdef FILEARCHIVE
    filearchive_unload();
#endif
#ifdef BOUNCER
    bouncer_unload();
#endif
#ifdef ACCTMGR
    acctmgr_unload();
#endif
#ifdef ADMIN
    admin_unload();
#endif
#ifdef STAT
    stat_unload();
#endif
#ifdef SEND
    send_unload();
#endif
#ifdef DIGEST
    digest_unload();
#endif
#ifdef BASE
    base_unload();
#endif
#ifdef ANTISPAM
    antispam_unload();
#endif
#ifdef ADMINISTRIVIA
    administrivia_unload();
#endif 
#ifdef TOOLBOX
    toolbox_unload();
#endif
#ifdef PANTOMIME
   pantomime_unload();
#endif
#ifdef LISTARGATE
   lsg2_unload();
#endif
#ifdef PASSWORD
   password_unload();
#endif
#ifdef PERUSER
   peruser_unload();
#endif

   /* Add any extra modules you want linked in here */
   return 1;
}

int switch_context_all_modules(void)
{
#ifdef LISTARCHIVE
    listarchive_switch_context();
#endif
#ifdef FILEARCHIVE
    filearchive_switch_context();
#endif
#ifdef BOUNCER
    bouncer_switch_context();
#endif
#ifdef ACCTMGR
    acctmgr_switch_context();
#endif
#ifdef ADMIN
    admin_switch_context();
#endif
#ifdef STAT
    stat_switch_context();
#endif
#ifdef SEND
    send_switch_context();
#endif
#ifdef DIGEST
    digest_switch_context();
#endif
#ifdef BASE
    base_switch_context();
#endif
#ifdef ANTISPAM
    antispam_switch_context();
#endif
#ifdef ADMINISTRIVIA
    administrivia_switch_context();
#endif 
#ifdef TOOLBOX
    toolbox_switch_context();
#endif
#ifdef PANTOMIME
   pantomime_switch_context();
#endif
#ifdef LISTARGATE
   lsg2_switch_context();
#endif
#ifdef PASSWORD
   password_switch_context();
#endif
#ifdef PERUSER
   peruser_switch_context();
#endif

   /* Add any extra modules you want linked in here */
   return 1;
}

int upgrade_all_modules(int prev, int cur)
{
#ifdef LISTARCHIVE
    listarchive_upgrade(prev, cur);
#endif
#ifdef FILEARCHIVE
    filearchive_upgrade(prev, cur);
#endif
#ifdef BOUNCER
    bouncer_upgrade(prev, cur);
#endif
#ifdef ACCTMGR
    acctmgr_upgrade(prev, cur);
#endif
#ifdef ADMIN
    admin_upgrade(prev, cur);
#endif
#ifdef STAT
    stat_upgrade(prev, cur);
#endif
#ifdef SEND
    send_upgrade(prev, cur);
#endif
#ifdef DIGEST
    digest_upgrade(prev, cur);
#endif
#ifdef BASE
    base_upgrade(prev, cur);
#endif
#ifdef ANTISPAM
    antispam_upgrade(prev, cur);
#endif
#ifdef ADMINISTRIVIA
    administrivia_upgrade(prev, cur);
#endif 
#ifdef TOOLBOX
    toolbox_upgrade(prev, cur);
#endif
#ifdef PANTOMIME
   pantomime_upgrade(prev, cur);
#endif
#ifdef LISTARGATE
   lsg2_upgrade(prev, cur);
#endif
#ifdef PASSWORD
   password_upgrade(prev, cur);
#endif
#ifdef PERUSER
   peruser_upgrade(prev, cur);
#endif

   /* Add any extra modules you want linked in here */
   return 1;
}

int listupgrade_all_modules(int prev, int cur)
{
#ifdef LISTARCHIVE
    listarchive_upgradelist(prev, cur);
#endif
#ifdef FILEARCHIVE
    filearchive_upgradelist(prev, cur);
#endif
#ifdef BOUNCER
    bouncer_upgradelist(prev, cur);
#endif
#ifdef ACCTMGR
    acctmgr_upgradelist(prev, cur);
#endif
#ifdef ADMIN
    admin_upgradelist(prev, cur);
#endif
#ifdef STAT
    stat_upgradelist(prev, cur);
#endif
#ifdef SEND
    send_upgradelist(prev, cur);
#endif
#ifdef DIGEST
    digest_upgradelist(prev, cur);
#endif
#ifdef BASE
    base_upgradelist(prev, cur);
#endif
#ifdef ANTISPAM
    antispam_upgradelist(prev, cur);
#endif
#ifdef ADMINISTRIVIA
    administrivia_upgradelist(prev, cur);
#endif 
#ifdef TOOLBOX
    toolbox_upgradelist(prev, cur);
#endif
#ifdef PANTOMIME
   pantomime_upgradelist(prev, cur);
#endif
#ifdef LISTARGATE
   lsg2_upgradelist(prev, cur);
#endif
#ifdef PASSWORD
   password_upgradelist(prev, cur);
#endif
#ifdef PERUSER
   peruser_upgradelist(prev, cur);
#endif

   /* Add any extra modules you want linked in here */
   return 1;
}
