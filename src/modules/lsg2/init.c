#include <stdio.h>

#include "lsg2.h"

struct LPMAPI *LMAPI;

void lsg2_switch_context(void)
{
   LMAPI->log_printf(15, "Switching context in module LSG/2\n");
}

int lsg2_upgradelist(int prev, int cur)
{
   LMAPI->log_printf(10, "Upgrading lists in module LSG/2\n");
   return 1;
}

int lsg2_upgrade(int prev, int cur)
{
   LMAPI->log_printf(10, "Upgrading module LSG/2\n");
   return 1;
}

void lsg2_unload(void)
{
   LMAPI->log_printf(10, "Unloading module LSG/2\n");
}

void lsg2_init(void)
{
   LMAPI->log_printf(10, "Initializing module LSG/2\n");
}

void lsg2_load(struct LPMAPI* api)
{
   LMAPI = api;

   /* Log the module initialization */
   LMAPI->log_printf(10, "Loading module LSG/2\n");

   /* Register us as a modules */
   LMAPI->add_module("LSG/2", "Listargate/2 WWW Interface");

   /* Cmd line args */
   LMAPI->add_cmdarg("-lsg2", 0, NULL, cmdarg_lsg2);

   /* Mode of operation */
   LMAPI->add_mode("lsg2", mode_lsg2);

   /* Cookie type stuff */
   LMAPI->register_cookie('W', "$$COOKIE$$", NULL, NULL);

   /* CGI Modes */
   LMAPI->add_cgi_mode("default", cgimode_default);
   LMAPI->add_cgi_mode("login", cgimode_login);
   LMAPI->add_cgi_mode("logout", cgimode_logout);
   LMAPI->add_cgi_mode("listmenu", cgimode_listmenu);
   LMAPI->add_cgi_mode("displayfile", cgimode_displayfile);
   LMAPI->add_cgi_mode("userlist", cgimode_userlist);
   LMAPI->add_cgi_mode("passwd", cgimode_passwd);
   LMAPI->add_cgi_mode("subscribe", cgimode_subscribe);
   LMAPI->add_cgi_mode("unsubscribe", cgimode_unsubscribe);
   LMAPI->add_cgi_mode("flagedit", cgimode_flagedit);
   LMAPI->add_cgi_mode("setflags", cgimode_setflags);
   LMAPI->add_cgi_mode("setname", cgimode_setname);
   LMAPI->add_cgi_mode("userinfo", cgimode_userinfo);
   LMAPI->add_cgi_mode("admin", cgimode_admin);
   LMAPI->add_cgi_mode("setconfig", cgimode_setconfig);
   LMAPI->add_cgi_mode("config", cgimode_config);
   LMAPI->add_cgi_mode("getfile", cgimode_getfile);
   LMAPI->add_cgi_mode("putfile", cgimode_putfile);
   LMAPI->add_cgi_mode("admin_subscribe", cgimode_admin_subscribe);
   LMAPI->add_cgi_mode("admin_unsubscribe", cgimode_admin_unsubscribe);
   LMAPI->add_cgi_mode("admin_setflags", cgimode_admin_setflags);
   LMAPI->add_cgi_mode("admin_setname", cgimode_admin_setname);
   LMAPI->add_cgi_mode("admin_userinfo", cgimode_admin_userinfo);
   LMAPI->add_cgi_mode("admin_usersetinfo", cgimode_admin_usersetinfo);
   LMAPI->add_cgi_mode("admin_userfor_generic", cgimode_admin_userfor_generic);

   /* CGI Hooks */
   LMAPI->add_cgi_hook("modehead", cgihook_modehead);
   LMAPI->add_cgi_hook("modeheadex", cgihook_modeheadex);
   LMAPI->add_cgi_hook("modeend", cgihook_modeend);
   LMAPI->add_cgi_hook("username", cgihook_username);
   LMAPI->add_cgi_hook("editusername", cgihook_editusername);
   LMAPI->add_cgi_hook("authcookie", cgihook_authcookie);
   LMAPI->add_cgi_hook("password", cgihook_password);
   LMAPI->add_cgi_hook("submit", cgihook_submit);
   LMAPI->add_cgi_hook("lists", cgihook_lists);
   LMAPI->add_cgi_hook("listsex", cgihook_listsex);
   LMAPI->add_cgi_hook("displayfile", cgihook_display_textfile);
   LMAPI->add_cgi_hook("welcome-button", cgihook_welcome_button);
   LMAPI->add_cgi_hook("faq-button", cgihook_faq_button);
   LMAPI->add_cgi_hook("info-button", cgihook_infofile_button);
   LMAPI->add_cgi_hook("curlist", cgihook_curlist);
   LMAPI->add_cgi_hook("userlist", cgihook_userlist);
   LMAPI->add_cgi_hook("liscript", cgihook_liscript);
   LMAPI->add_cgi_hook("subscribe", cgihook_subscribe);
   LMAPI->add_cgi_hook("unsubscribe", cgihook_unsubscribe);
   LMAPI->add_cgi_hook("flaglist", cgihook_flaglist);
   LMAPI->add_cgi_hook("setname", cgihook_setname);
   LMAPI->add_cgi_hook("showflags", cgihook_showflags);
   LMAPI->add_cgi_hook("configfile", cgihook_configfile);   
   LMAPI->add_cgi_hook("fileedit", cgihook_fileedit);
   LMAPI->add_cgi_hook("adminfilelist", cgihook_adminfilelistdrop);
   LMAPI->add_cgi_hook("adminshowflags", cgihook_admin_showflags);
   LMAPI->add_cgi_hook("adminflaglist", cgihook_admin_flaglist);
   LMAPI->add_cgi_hook("adminuserlistbox", cgihook_admin_userlistbox);
   LMAPI->add_cgi_hook("adminsetname", cgihook_admin_setname);
   
   /* Variables */
   LMAPI->register_var("lsg2-cookie-duration", "15 m", "CGI",
     "The length of time that cookies for the LSG/2 web form should last.",
     "lsg2-cookie-duration = 30 m", VAR_DURATION, VAR_GLOBAL|VAR_SITE);
   LMAPI->register_var("lsg2-cgi-url", NULL, "CGI",
     "URL on the associated web server pointing to the LSG/2 cgi wrapper.",
     "lsg2-cgi-url = http://my.dom/cgi-bin/lsg2.cgi", VAR_STRING,
     VAR_GLOBAL|VAR_SITE);
   LMAPI->register_var("lsg2-iis-support", "no", "CGI", 
     "Does LSG/2 need to run in Microsoft IIS-compatible mode?  (Likely breaks other webservers.)",
     "lg2-iis-support = yes", VAR_BOOL, VAR_GLOBAL);
   LMAPI->register_var("lsg2-paranoia", "no", "CGI",
     "Is LSG/2 paranoid, e.g. does it deny all remote administration.",
     "lsg2-paranoia = no", VAR_BOOL, VAR_GLOBAL|VAR_SITE);
   LMAPI->register_var("lsg2-sort-userlist", "yes", "CGI",
     "Should LSG/2 sort the userlist in admin mode.  This can be expensive for large lists.",
     "lsg2-sort-userlist = true", VAR_BOOL, VAR_ALL);

   /* Internal variables */
   LMAPI->register_var("lcgi-error", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_ALL|VAR_INTERNAL);
   LMAPI->register_var("lcgi-remote-host", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_ALL|VAR_INTERNAL);
   LMAPI->register_var("lcgi-server-soft", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_ALL|VAR_INTERNAL);
   LMAPI->register_var("lcgi-mode", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_ALL|VAR_INTERNAL);
   LMAPI->register_var("lcgi-lastmode", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_ALL|VAR_INTERNAL);
   LMAPI->register_var("lcgi-user", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_ALL|VAR_INTERNAL);
   LMAPI->register_var("lcgi-userfor", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_ALL|VAR_INTERNAL);
   LMAPI->register_var("lcgi-list", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_ALL|VAR_INTERNAL);
   LMAPI->register_var("lcgi-pass", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_ALL|VAR_INTERNAL);
   LMAPI->register_var("lcgi-pass-confirm", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_ALL|VAR_INTERNAL);
   LMAPI->register_var("lcgi-cookie", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_ALL|VAR_INTERNAL);
   LMAPI->register_var("lcgi-fullname", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_ALL|VAR_INTERNAL);
   LMAPI->register_var("lcgi-adminfile", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_ALL|VAR_INTERNAL);

   LMAPI->register_var("lsg2-img-admin", "<b>A</b>", NULL, NULL, NULL,
     VAR_STRING, VAR_ALL|VAR_INTERNAL);
   LMAPI->register_var("lsg2-img-moderators", "<b>M</b>", NULL, NULL, NULL,
     VAR_STRING, VAR_ALL|VAR_INTERNAL);
   LMAPI->register_var("lsg2-img-hidden", "<b>H</b>", NULL, NULL, NULL,
     VAR_STRING, VAR_ALL|VAR_INTERNAL);
   LMAPI->register_var("lsg2-img-digest", "<b>D</b>", NULL, NULL, NULL,
     VAR_STRING, VAR_ALL|VAR_INTERNAL);
   LMAPI->register_var("lsg2-img-vacation", "<b>V</b>", NULL, NULL, NULL,
     VAR_STRING, VAR_ALL|VAR_INTERNAL);

   LMAPI->register_var("tlcgi-textfile", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_TEMP|VAR_INTERNAL);
   LMAPI->register_var("tlcgi-pathinfo", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_GLOBAL|VAR_INTERNAL);
   LMAPI->register_var("tlcgi-generic-text", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_TEMP|VAR_INTERNAL);
   LMAPI->register_var("tlcgi-editfile", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_TEMP|VAR_INTERNAL);
   LMAPI->register_var("tlcgi-editfile-desc", NULL, NULL, NULL, NULL,
     VAR_STRING, VAR_TEMP|VAR_INTERNAL);

   LMAPI->register_var("tlcgi-users-total", NULL, NULL, NULL, NULL,
     VAR_INT, VAR_TEMP|VAR_INTERNAL);
   LMAPI->register_var("tlcgi-users-admins", NULL, NULL, NULL, NULL,
     VAR_INT, VAR_TEMP|VAR_INTERNAL);
   LMAPI->register_var("tlcgi-users-moderators", NULL, NULL, NULL, NULL,
     VAR_INT, VAR_TEMP|VAR_INTERNAL);
   LMAPI->register_var("tlcgi-users-digest", NULL, NULL, NULL, NULL,
     VAR_INT, VAR_TEMP|VAR_INTERNAL);
   LMAPI->register_var("tlcgi-users-hidden", NULL, NULL, NULL, NULL,
     VAR_INT, VAR_TEMP|VAR_INTERNAL);
   LMAPI->register_var("tlcgi-users-vacation", NULL, NULL, NULL, NULL,
     VAR_INT, VAR_TEMP|VAR_INTERNAL);
}
