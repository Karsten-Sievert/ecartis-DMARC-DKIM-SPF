#include <stdio.h>

#include "acctmgr-mod.h"

struct LPMAPI *LMAPI;

void acctmgr_switch_context(void)
{
   LMAPI->log_printf(15, "Switching context in module AcctMgr\n");
}

int acctmgr_upgrade(int prev, int cur)
{
   LMAPI->log_printf(10, "Upgrading module AcctMgr\n");
   return 1;
}

int acctmgr_upgradelist(int prev, int cur)
{
   LMAPI->log_printf(10, "Upgrading lists in module AcctMgr\n");
   return 1;
}
void acctmgr_unload(void)
{
   LMAPI->log_printf(10, "Unloading module AcctMgr\n");
}

void acctmgr_init(void)
{
   LMAPI->log_printf(10, "Initializing module AcctMgr\n");
}

void acctmgr_load(struct LPMAPI *api)
{
   LMAPI = api;

   LMAPI->log_printf(10, "Loading module AcctMgr\n");

   /* Add module definition */
   LMAPI->add_module("AcctMgr",
                     "Manages user account settings and subscriptions.");

   /* Add module commands */
   LMAPI->add_command("unset","Unsets a user variable.",
                      "unset [<list>] <flag>", NULL, NULL,
                      CMD_HEADER|CMD_BODY, cmd_unset);

   LMAPI->add_command("set","Sets a user variable.",
                      "set [<list>] <flag>", NULL, NULL,
                      CMD_HEADER|CMD_BODY, cmd_set);

   LMAPI->add_command("appsub","Confirm subscription to list.",
                      "appsub <list> <address> <cookie>", NULL, NULL,
                      CMD_BODY|CMD_HEADER, cmd_appsub);

   LMAPI->add_command("appunsub","Confirm unsubscription from list",
                      "appunsub <list> <address> <cookie>", NULL, NULL,
                      CMD_BODY|CMD_HEADER, cmd_appunsub);

   LMAPI->add_command("unsubscribe","Unsubscribe from a list",
                      "unsubscribe [<list>] [<user>]", NULL, "unsubscribe <user>",
                      CMD_BODY|CMD_HEADER, cmd_unsubscribe);

   LMAPI->add_command("subscribe","Subscribe to a list",
                      "subscribe [<list>] [<user>]", NULL, "subscribe <user>",
                      CMD_BODY|CMD_HEADER, cmd_subscribe);

   LMAPI->add_command("subscription","Alias for subscribe",
                      "subscription [<list>] [<user>]", NULL, "subscription <user>",
                      CMD_BODY|CMD_HEADER, cmd_subscribe);

   LMAPI->add_command("submode","Subscribe to a list in an admin specified mode",
                      "submode <mode> [<list>] [<user>]", NULL, "submode <mode> <user>",
                      CMD_BODY|CMD_HEADER, cmd_submode);
   LMAPI->add_command("submodes", "Show the admin specified subscription modes.",
                      "submodes [<list>]", NULL, NULL, CMD_BODY|CMD_HEADER,
                      cmd_submodes);

   LMAPI->add_command("del","Alias for unsubscribe.",
                      "del [<list>] [<user>]", NULL, "del <user>",
                      CMD_BODY|CMD_HEADER, cmd_unsubscribe);

   LMAPI->add_command("signoff", "Alias for unsubscribe.",
                      "signoff [<list>] [<user>]", NULL, "signoff <user>",
                      CMD_BODY|CMD_HEADER, cmd_unsubscribe);

   LMAPI->add_command("remove","Alias for unsubscribe.",
                      "remove [<list>] [<user>]", NULL, "remove <user>",
                      CMD_BODY|CMD_HEADER, cmd_unsubscribe);

   LMAPI->add_command("add","Alias for subscribe",
                      "add [<list>] [<user>]", NULL, "add <user>",
                       CMD_BODY|CMD_HEADER, cmd_subscribe);

   LMAPI->add_command("join","Alias for subscribe",
                      "join [<list>] [<user>]", NULL, "join <user>",
                      CMD_BODY|CMD_HEADER, cmd_subscribe);

   LMAPI->add_command("leave","Alias for unsubscribe.",
                      "leave [<list>] [<user>]", NULL, "leave <user>",
                      CMD_BODY|CMD_HEADER, cmd_unsubscribe);

   LMAPI->add_command("setaddy","Reset address to same user on a sub-domain",
                      "setaddy <address>", NULL, NULL, CMD_BODY|CMD_HEADER,
                      cmd_setaddy);

   LMAPI->add_command("vacation","Set user on vacation for specified duration",
                      "vacation [<list>] [<duration>]", NULL,
                      "vacation <user> [<duration>]", CMD_BODY|CMD_HEADER,
                      cmd_vacation);

   LMAPI->add_command("tempban","Temporarily ban a user for a specified duration",
                      NULL, NULL, "tempban <user> [<duration>]",
                      CMD_BODY|CMD_ADMIN, cmd_tempban);

   /* Add module hooks */
   LMAPI->add_hook("PRESUB", 50, hook_presub_blacklist);
   LMAPI->add_hook("PRESUB", 55, hook_presub_acl);
   LMAPI->add_hook("PRESUB", 60, hook_presub_subscribed);
   LMAPI->add_hook("PRESUB", 70, hook_presub_closed);
   LMAPI->add_hook("PRESUB", 80, hook_presub_confirm);

   LMAPI->add_hook("POSTSUB", 50, hook_postsub_administrivia);
   LMAPI->add_hook("POSTSUB", 50, hook_postsub_welcome);

   LMAPI->add_hook("PREUNSUB", 50, hook_preunsub_subscribed);
   LMAPI->add_hook("PREUNSUB", 51, hook_preunsub_closed);
   LMAPI->add_hook("PREUNSUB", 52, hook_preunsub_confirm);

   LMAPI->add_hook("POSTUNSUB", 50, hook_postunsub_goodbye);
   LMAPI->add_hook("POSTUNSUB", 50, hook_postunsub_administrivia);

   LMAPI->add_hook("SETFLAG", 50, hook_setflag_vacation);
   LMAPI->add_hook("UNSETFLAG", 50, hook_setflag_vacation);

   LMAPI->add_hook("SETFLAG", 50, hook_setflag_nopost);
   LMAPI->add_hook("UNSETFLAG", 50, hook_setflag_nopost);
   

   /* Add module flags */
   LMAPI->add_flag("hidden",
                   "User won't show up in membership listing of list unless viewed by an admin.",0);

   LMAPI->add_flag("echopost",
             "User receives copies of their own posts.",0);
   LMAPI->add_flag("ackpost",
             "User receives small note when a message is posted, or approved by a moderator.",0);
   LMAPI->add_flag("vacation",
             "User is on vacation; don't send mail until flag unset.",0);

   /* files */
   LMAPI->add_file("blacklist-reject","blacklist-reject-file","Customized 'blacklist' bounce message textfile.");
   LMAPI->add_file("blacklist","blacklist-mask","List of addresses who are banned from this list.");
   LMAPI->add_file("welcome","welcome-file","Customized 'welcome' textfile.");
   LMAPI->add_file("goodbye","goodbye-file","Customized 'goodbye' textfile.");
   LMAPI->add_file("closed-subscribe","closed-subscribe-file","Custom response message for when a user tries to subscribe to a list with subscribe-mode = closed");
	LMAPI->add_file("subscribe-confirm", "subscribe-confirm-file",
		"Customized subscription message for subscribe-mode = confirm");
	LMAPI->add_file("unsubscribe-confirm", "unsubscribe-confirm-file",
		"Customized unsubscription message for unsubscribe-mode = confirm");
   LMAPI->add_file("submodes", "submodes-file", "File which contains customized subscription modes for a list.");
   LMAPI->add_file("tempban", "tempban-file", "File to be sent to user when an admin issues a tempban command on them.");
   LMAPI->add_file("tempban-end", "tempban-end-file", "File to be sent to a user who was tempbanned when they are no longer tempbanned.");
   LMAPI->add_file("acl","subscribe-acl-file", "File of regular expressions to check against when someone subscribes.  A match is REQUIRED.");
   LMAPI->add_file("acl-text", "subscribe-acl-text-file", "Textfile to send to user if their subscription has been denied because they didn't match an ACL pattern.");

   /* Add the cookie registrations */
   LMAPI->register_cookie('S', "subscription-expiration-time", NULL, NULL);
   LMAPI->register_cookie('U', "unsubscription-expiration-time", NULL, NULL);
   LMAPI->register_cookie('V', "$$COOKIE$$", NULL, destroy_vacation_cookie);
   LMAPI->register_cookie('T', "$$COOKIE$$", NULL, destroy_tempban_cookie);

   /* register variables */
   LMAPI->register_var("allow-setaddy", "yes", "Account Management",
                       "Allow the use of the setaddy command to replace the subscribed address.",
                       "allow-setaddy=no", VAR_BOOL, VAR_ALL);
   LMAPI->register_var("setflag-user", NULL, NULL, NULL, NULL, VAR_STRING,
                       VAR_TEMP|VAR_INTERNAL);
   LMAPI->register_var("setflag-flag", NULL, NULL, NULL, NULL, VAR_STRING,
                       VAR_TEMP|VAR_INTERNAL);
   LMAPI->register_var("subscribe-me", NULL, NULL, NULL, NULL, VAR_STRING,
                       VAR_GLOBAL|VAR_INTERNAL);
   LMAPI->register_var("blacklist-reject-file", "text/blacklist.txt", "Files",
                       "File sent to a user when their subscription or post is rejected because they are blacklisted.",
                       "blacklist-reject-file = text/blacklist.txt",
                       VAR_STRING, VAR_ALL);
   LMAPI->register_alias("blacklist-file", "blacklist-reject-file");
   LMAPI->register_var("blacklist-mask", "blacklist", "Files",
                 "Per-list file containing regular expressions for users who are not allowed to subscribe to the list.",
                 "blacklist-mask = blacklist", VAR_STRING, VAR_ALL);
   LMAPI->register_var("subscription-expiration-time", NULL, "Timeouts",
                       "How long until subscription verification cookies expire.",
                       "subscription-expiration-time = 5 d", VAR_DURATION,
                       VAR_ALL);
   LMAPI->register_var("unsubscription-expiration-time", NULL, "Timeouts",
                       "How long until unsubscription verification cookies expire.",
                       "unsubscription-expiration-time = 5 d", VAR_DURATION,
                       VAR_ALL);
   LMAPI->register_var("subscribe-mode", "closed:|closed|confirm|open|open-auto|", "Subscribe Options",
                       "Subscription mode for the list. Setting this to \"confirm\" will be \"double opt-in\"", 
                       "subscribe-mode = open", VAR_CHOICE, VAR_ALL);
   LMAPI->register_var("silent-resubscribe", "no", "Subscribe Options",
                       "Silence warnings when an already-subscribed user re-subscribes.",
                       "silent-resubscribe = on", VAR_BOOL, VAR_ALL);
   LMAPI->register_var("unsubscribe-mode", "open:|closed|confirm|open|open-auto|", "Subscribe Options",
                       "Unsubscription mode for the list.",
                       "unsubscribe-mode = closed", VAR_CHOICE, VAR_ALL);
   LMAPI->register_var("administrivia-address", NULL, "Addresses",
                       "Address to which subscription/unsubscription attempt notifications should be sent.",
                       "administrivia-address = mylist-admins@host.dom",
                       VAR_STRING, VAR_ALL);
   LMAPI->register_var("welcome-subject",
		   "Welcome to list '<$list>'", "Subscribe Options",
		   "Subject to be used in the mail sent to new subscribers of a list.",
		   "welcome-subject = Welcome to list '<$list>'",
		   VAR_STRING, VAR_ALL);
   LMAPI->register_var("welcome-file", "text/intro.txt", "Files",
                       "File sent to new subscribers of a list.",
                       "welcome-file = text/intro.txt", VAR_STRING, VAR_ALL);
   LMAPI->register_var("goodbye-subject",
		   "You have signed off '<$list>'", "Subscribe Options",
		   "Subject to be used in the mail sent if someone unsubscribes off a list.",
		   "goodbye-subject = You have signed off '<$list>'",
		   VAR_STRING, VAR_ALL);
   LMAPI->register_var("goodbye-file", "text/goodbye.txt", "Files",
                       "File sent to someone unsubscribing from a list.",
                       "goodbye-file = text/goodbye.txt", VAR_STRING, VAR_ALL);
   LMAPI->register_var("subscribe-confirm-subject",
		   "Subscription confirmation for '<$list>'", "Subscribe Options",
		   "Subject to be used in the mail sent to someone subscribing to a list, if subscribe-mode = confirm.",
		   "subscribe-confirm-subject = Subscription confirmation for '<$list>'",
		   VAR_STRING, VAR_ALL);
   LMAPI->register_var("subscribe-confirm-file",
		   "text/subscribe-confirm.txt", "Subscribe Options",
		   "File sent to someone subscribing to a list, if subscribe-mode = confirm.",
		   "subscribe-confirm-file = text/subscribe-confirm.txt",
		   VAR_STRING, VAR_ALL);
   LMAPI->register_var("unsubscribe-confirm-subject",
		   "Unsubscription confirmation for '<$list>'",
		   "Subscribe Options",
		   "Subject to be used in the mail sent to someone unsubscribing from a list, if unsubscribe-mode = confirm.",
		   "unsubscribe-confirm-subject = Unsubscription confirmation for '<$list>'",
		   VAR_STRING, VAR_ALL);
   LMAPI->register_var("unsubscribe-confirm-file",
		   "text/unsubscribe-confirm.txt", "Subscribe Options",
		   "File sent to someone unsubscribing from a list, if unsubscribe-mode = confirm.",
		   "unsubscribe-confirm-file = text/unsubscribe-confirm.txt",
		   VAR_STRING, VAR_ALL);

   LMAPI->register_var("no-administrivia", "no", "Subscribe Options",
                       "Should the administrivia address be notified when a user subscribes or unsubscribes.",
                       "no-administrivia = on", VAR_BOOL, VAR_ALL);
   LMAPI->register_var("owner-fallback", "no", "Subscribe Options",
                       "Should the list-owner be used if administrivia-address is not defined when notifying of (un)subscribes.",
                       "owner-fallback = true", VAR_BOOL, VAR_ALL);
   LMAPI->register_var("administrivia-include-requests", "no", "Subscribe Options",
                       "Should the mail which caused the (un)subscription action be included in the message to the administrivia address.",
                       "administrivia-include-requests = on", VAR_BOOL,
                       VAR_ALL);
   LMAPI->register_alias("include-requests", "administrivia-include-requests");
   LMAPI->register_var("closed-subscribe-file","text/closed-subscribe.txt",
                       "Files",
                       "Filename of file to be sent if a user tries to subscribe to a closed subscription list.",
                       "closed-subscribe-file = text/closed-subscribe.txt",
                       VAR_STRING, VAR_ALL);
   LMAPI->register_var("admin-subscribe-notice", "all:|silent|welcome|notify|all|",
                       "Subscribe Options",
                       "Notifications sent when an administrator (ADMIN) adds a subscriber.  The value of 'all' will send both the notification and welcome text (if configured) to the subscriber.  The 'silent' option will send neither.",
                       "admin-subscribe-notice = welcome", VAR_CHOICE, VAR_ALL);
   LMAPI->register_var("admin-unsubscribe-notice", "all:|silent|goodbye|notify|all|",
                       "Subscribe Options",
                       "Notifications sent when an administrator (ADMIN) removes a subscriber.  The value of 'all' will send both the notification and goodbye text (if configured) to the subscriber.  The 'silent' option will send neither.",
                       "admin-unsubscribe-notice = goodbye", VAR_CHOICE, VAR_ALL);
   LMAPI->register_var("admin-silent-subscribe", "no", "Subscribe Options",
                       "If set true, when an admin subscribes a user to a list they will receive no subscription notification AND no welcome message.",
                       "admin-silent-subscribe = no", VAR_BOOL, VAR_ALL);
   LMAPI->register_var("vacation-default-duration", "14 d", "Vacation",
                       "If a person sends the vacation command without a duration, how long they will be set vacation.",
                       "vacation-default-duration", VAR_DURATION, VAR_ALL);
   LMAPI->register_var("tempban-default-duration", "7 d", "Tempban",
                       "If an administrator issues the tempban command without a duration, this default will be used.",
                       "tempban-default-duration = 7 d", VAR_DURATION,
                       VAR_ALL);
   LMAPI->register_var("tempban-file", "text/tempban.txt", "Files",
                       "Filename of file to be sent to a user when an admin issues the tempban command on them.",
                       "tempban-file = text/tempban.txt", VAR_STRING,
                       VAR_ALL);
   LMAPI->register_var("tempban-end-file", "text/tempban-end.txt", "Files",
                       "Filename of file to be sent to a user who was tempbanned when the tempban expires.",
                       "tempban-end-file = text/tempban-end.txt",
                       VAR_STRING, VAR_ALL);
   LMAPI->register_var("tempban-end-date", NULL, NULL,
                       "Internal variable, for use in Liscript templates for tempban-file and tempban-end-file, which contains the ending time for the temporary ban.",
                       NULL, VAR_STRING, VAR_TEMP|VAR_INTERNAL);
   LMAPI->register_var("subscribe-acl-file", "subscribe-acl", "Files",
                       "File containing regular expressions against which a user's address will be matched when they try to subscribe to a list.  If an address does not match at least one, subscription is denied.  Can be gotten with 'getconf acl'.",
                       "subscribe-acl-file = subscribe-acl", VAR_STRING, VAR_ALL);
   LMAPI->register_var("subscribe-acl-text-file", "text/subscribe-acl-deny.txt",
                       "Files", "Textfile to be sent to a user who fails the ACL subscription check.  Can be gotten with 'getconf acl-text'.",
                       "subscribe-acl-text-file = text/subscribe-acl-deny.txt",
                       VAR_STRING, VAR_ALL);
   LMAPI->register_var("subscription-acl", "true", "Subscribe Options",
                       "If 'true' and the file given in 'subscribe-acl-file' exists, a subscription access list check will be performed when users attempt to subscribe to the list.",
                       "subscription-acl = true", VAR_BOOL, VAR_ALL);

   LMAPI->register_var("prevent-second-message", "false", "Subscribe Options",
                       "If set 'true', then when someone tries to subscribe or unsubscribe to an Ecartis list and no other commands are given, the 'Ecartis command results' message will not be sent.",
                       "prevent-second-message = true", VAR_BOOL, VAR_ALL);

   /* Set up some functions */
   LMAPI->add_func("subscribed", 1,
                   "Check if given address is subscribed to current list.",
                   func_subscribed);
   LMAPI->add_func("subscribed_list", 2,
                   "Check if given address is subscribed to given list.",
                   func_subscribed_list);
   LMAPI->add_func("hasflag", 2,
                   "Check if given address has given flag on current list.",
                   func_hasflag);
   LMAPI->add_func("hasflag_list", 3,
                   "Check if given address has given flag on given list.",
                   func_hasflag_list);
}
