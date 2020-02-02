#ifndef _SEND_MOD_H
#define _SEND_MOD_H

#include "lpm.h"

extern struct LPMAPI *LMAPI;

/* Hooks */
extern HOOK_HANDLER(hook_presend_blacklist);
extern HOOK_HANDLER(hook_presend_check_outside);
extern HOOK_HANDLER(hook_presend_check_closed);
extern HOOK_HANDLER(hook_presend_check_nopost);
extern HOOK_HANDLER(hook_presend_check_messageid);
extern HOOK_HANDLER(hook_presend_check_overquoting);
extern HOOK_HANDLER(hook_presend_check_size);
extern HOOK_HANDLER(hook_presend_check_xlist);
extern HOOK_HANDLER(hook_presend_check_password);
extern HOOK_HANDLER(hook_presend_check_moderate);
extern HOOK_HANDLER(hook_presend_check_modpost);
extern HOOK_HANDLER(hook_presend_check_subject);
extern HOOK_HANDLER(hook_presend_check_bcc);
extern HOOK_HANDLER(hook_presend_unmime);
extern HOOK_HANDLER(hook_presend_unquote);

extern HOOK_HANDLER(hook_send_tag);
extern HOOK_HANDLER(hook_send_version);
extern HOOK_HANDLER(hook_send_returnpath);
extern HOOK_HANDLER(hook_send_precedence);
extern HOOK_HANDLER(hook_send_forcefrom);
extern HOOK_HANDLER(hook_send_replyto);
extern HOOK_HANDLER(hook_send_approvedby);
extern HOOK_HANDLER(hook_send_rfc2369);
extern HOOK_HANDLER(hook_send_xlist);
extern HOOK_HANDLER(hook_send_footer);
extern HOOK_HANDLER(hook_send_header);
extern HOOK_HANDLER(hook_send_stripheaders);
extern HOOK_HANDLER(hook_send_stripmdn);
extern HOOK_HANDLER(hook_send_strip_rfc2369);

extern HOOK_HANDLER(hook_final_send);

extern HOOK_HANDLER(hook_tolist_build_tolist);
extern HOOK_HANDLER(hook_tolist_remove_vacationers);
extern HOOK_HANDLER(hook_tolist_sort);

extern CMDARG_HANDLER(cmdarg_private_reply);

#endif /* _SEND_MOD_H */
