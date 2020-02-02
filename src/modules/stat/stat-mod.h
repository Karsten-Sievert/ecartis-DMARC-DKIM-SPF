#ifndef _STAT_MOD_H
#define _STAT_MOD_H

#include "lpm.h"
extern struct LPMAPI* LMAPI;

/* Commands */
extern CMD_HANDLER(cmd_help);
extern CMD_HANDLER(cmd_list_commands);
extern CMD_HANDLER(cmd_list_modules);
extern CMD_HANDLER(cmd_list_lists);
extern CMD_HANDLER(cmd_list_flags);
extern CMD_HANDLER(cmd_list_users);
extern CMD_HANDLER(cmd_list_files);
extern CMD_HANDLER(cmd_stats);
extern CMD_HANDLER(cmd_review);
extern CMD_HANDLER(cmd_setname);
extern CMD_HANDLER(cmd_setrole);
extern CMD_HANDLER(cmd_which_lists);
extern CMD_HANDLER(cmd_faq);
extern CMD_HANDLER(cmd_info);

extern HOOK_HANDLER(hook_after_postsize);
extern HOOK_HANDLER(hook_after_numposts);

extern FUNC_HANDLER(func_list_exists);
extern FUNC_HANDLER(func_hasstat);

#endif /* _STAT_MOD_H */
