#ifndef _ADMIN_MOD_H
#define _ADMIN_MOD_H

#include "lpm.h"

extern struct LPMAPI *LMAPI;

/* Admin commands */
extern CMD_HANDLER(cmd_admin);
extern CMD_HANDLER(cmd_admin2);
extern CMD_HANDLER(cmd_admin_verify);
extern CMD_HANDLER(cmd_adminend);
extern CMD_HANDLER(cmd_adminbecome);
extern CMD_HANDLER(cmd_adminset);
extern CMD_HANDLER(cmd_adminunset);
extern CMD_HANDLER(cmd_adminfilereq);
extern CMD_HANDLER(cmd_adminfileput);

extern HOOK_HANDLER(hook_setflag_moderator);

#endif /* _ADMIN_MOD_H */
