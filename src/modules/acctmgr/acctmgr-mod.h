#ifndef _ACCTMGR_MOD_H
#define _ACCTMGR_MOD_H
#include "lpm.h"
/* Commands */
extern CMD_HANDLER(cmd_subscribe);
extern CMD_HANDLER(cmd_submode);
extern CMD_HANDLER(cmd_submodes);
extern CMD_HANDLER(cmd_unsubscribe);
extern CMD_HANDLER(cmd_appsub);
extern CMD_HANDLER(cmd_appunsub);
extern CMD_HANDLER(cmd_setaddy);
extern CMD_HANDLER(cmd_set);
extern CMD_HANDLER(cmd_unset);
extern CMD_HANDLER(cmd_vacation);
extern CMD_HANDLER(cmd_tempban);

extern HOOK_HANDLER(hook_presub_closed);
extern HOOK_HANDLER(hook_presub_confirm);
extern HOOK_HANDLER(hook_presub_acl);
extern HOOK_HANDLER(hook_presub_blacklist);
extern HOOK_HANDLER(hook_presub_subscribed);
extern HOOK_HANDLER(hook_postsub_administrivia);
extern HOOK_HANDLER(hook_postsub_welcome);

extern HOOK_HANDLER(hook_preunsub_closed);
extern HOOK_HANDLER(hook_preunsub_confirm);
extern HOOK_HANDLER(hook_preunsub_subscribed);
extern HOOK_HANDLER(hook_postunsub_administrivia);
extern HOOK_HANDLER(hook_postunsub_goodbye);

extern HOOK_HANDLER(hook_setflag_vacation);
extern HOOK_HANDLER(hook_setflag_nopost);

extern COOKIE_HANDLER(destroy_vacation_cookie);
extern COOKIE_HANDLER(destroy_tempban_cookie);

extern FUNC_HANDLER(func_subscribed);
extern FUNC_HANDLER(func_hasflag);
extern FUNC_HANDLER(func_subscribed_list);
extern FUNC_HANDLER(func_hasflag_list);

extern struct LPMAPI *LMAPI;

#endif /* _ACCTMGR_MOD_H */
