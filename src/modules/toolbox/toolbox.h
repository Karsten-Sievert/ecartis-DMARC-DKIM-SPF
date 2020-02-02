#ifndef _TOOLBOX_H
#define _TOOLBOX_H

#include "lpm.h"

extern struct LPMAPI *LMAPI;

extern CMDARG_HANDLER(cmdarg_checkusers);
extern MODE_HANDLER(mode_checkusers);

extern CMDARG_HANDLER(cmdarg_qmail);
extern CMDARG_HANDLER(cmdarg_newlist);
extern CMDARG_HANDLER(cmdarg_admin);
extern CMDARG_HANDLER(cmdarg_freshen);
extern CMDARG_HANDLER(cmdarg_buildaliases);
extern MODE_HANDLER(mode_newlist);
extern MODE_HANDLER(mode_freshen);
extern MODE_HANDLER(mode_buildaliases);

#endif /* _TOOLBOX_H */
