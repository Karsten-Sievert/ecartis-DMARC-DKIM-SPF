#ifndef _LSG2_H
#define _LSG2_H

#include "lpm.h"
extern struct LPMAPI* LMAPI;

#define LSG2_VERSION "v1.0"

extern void lsg2_internal_error(const char *);
extern int  lsg2_update_cookie(char *buffer, int length);
extern void lsg2_html_textfile(const char *);
extern int  lsg2_validate(char *buffer, int length);

extern CMDARG_HANDLER(cmdarg_lsg2);

extern MODE_HANDLER(mode_lsg2);

extern CGI_HANDLER(cgihook_modehead);
extern CGI_HANDLER(cgihook_modeheadex);
extern CGI_HANDLER(cgihook_modeend);
extern CGI_HANDLER(cgihook_username);
extern CGI_HANDLER(cgihook_editusername);
extern CGI_HANDLER(cgihook_password);
extern CGI_HANDLER(cgihook_authcookie);
extern CGI_HANDLER(cgihook_submit);
extern CGI_HANDLER(cgihook_lists);
extern CGI_HANDLER(cgihook_listsex);
extern CGI_HANDLER(cgihook_display_textfile);
extern CGI_HANDLER(cgihook_welcome_button);
extern CGI_HANDLER(cgihook_faq_button);
extern CGI_HANDLER(cgihook_infofile_button);
extern CGI_HANDLER(cgihook_curlist);
extern CGI_HANDLER(cgihook_userlist);
extern CGI_HANDLER(cgihook_liscript);
extern CGI_HANDLER(cgihook_subscribe);
extern CGI_HANDLER(cgihook_unsubscribe);
extern CGI_HANDLER(cgihook_flaglist);
extern CGI_HANDLER(cgihook_setname);
extern CGI_HANDLER(cgihook_showflags);
extern CGI_HANDLER(cgihook_configfile);
extern CGI_HANDLER(cgihook_fileedit);
extern CGI_HANDLER(cgihook_adminfilelistdrop);
extern CGI_HANDLER(cgihook_admin_flaglist);
extern CGI_HANDLER(cgihook_admin_showflags);
extern CGI_HANDLER(cgihook_admin_userlistbox);
extern CGI_HANDLER(cgihook_admin_setname);

extern CGI_MODE(cgimode_default);
extern CGI_MODE(cgimode_login);
extern CGI_MODE(cgimode_logout);
extern CGI_MODE(cgimode_listmenu);
extern CGI_MODE(cgimode_displayfile);
extern CGI_MODE(cgimode_userlist);
extern CGI_MODE(cgimode_passwd);
extern CGI_MODE(cgimode_subscribe);
extern CGI_MODE(cgimode_unsubscribe);
extern CGI_MODE(cgimode_flagedit);
extern CGI_MODE(cgimode_setflags);
extern CGI_MODE(cgimode_setname);
extern CGI_MODE(cgimode_userinfo);
extern CGI_MODE(cgimode_admin);
extern CGI_MODE(cgimode_config);
extern CGI_MODE(cgimode_setconfig);
extern CGI_MODE(cgimode_getfile);
extern CGI_MODE(cgimode_putfile);
extern CGI_MODE(cgimode_admin_subscribe);
extern CGI_MODE(cgimode_admin_unsubscribe);
extern CGI_MODE(cgimode_admin_setflags);
extern CGI_MODE(cgimode_admin_setname);
extern CGI_MODE(cgimode_admin_userinfo);
extern CGI_MODE(cgimode_admin_usersetinfo);
extern CGI_MODE(cgimode_admin_userfor_generic);


#endif
