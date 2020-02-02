#ifndef _PASSWORD_H
#define _PASSWORD_H

#include "lpm.h"

extern struct LPMAPI *LMAPI;

extern CMD_HANDLER(cmd_password);
extern CMD_HANDLER(cmd_setpassword);
extern CMD_HANDLER(cmd_authpwd);

#endif /* _PASSWORD_H */
