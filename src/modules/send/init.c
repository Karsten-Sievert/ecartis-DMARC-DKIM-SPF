#include <stdio.h>

#include "send-mod.h"

struct LPMAPI *LMAPI;

void send_switch_context(void)
{
    LMAPI->log_printf(15, "Switching context in module Send\n");
}

int send_upgradelist(int prev, int cur)
{
    LMAPI->log_printf(10, "Upgrade lists in module Send\n");
    return 1;
}

int send_upgrade(int prev, int cur)
{
    LMAPI->log_printf(10, "Upgrade module Send\n");
    return 1;
}

void send_init(void)
{
    LMAPI->log_printf(10, "Initializing module Send\n");
}

void send_unload(void)
{
    LMAPI->log_printf(10, "Unloading module Send\n");
}

void send_load(struct LPMAPI *api)
{
    LMAPI = api;
    LMAPI->log_printf(10, "Loading module Send\n");

    LMAPI->add_module("Send","Mail sending module, also handles moderation.");

    /*
     * -priv allows one list to be used in two modes, normal where the
     * reply-to is used as defined by the list config, and private where
     * any replies sent to the message will be sent only to the sender.
     * It is here because otherwise it would go into a seperate module
     * and we'd add the reply-to header and then remove it in the other
     * module.
     */
    LMAPI->add_cmdarg("-priv", 0, NULL, cmdarg_private_reply);

    LMAPI->add_hook("FINAL", 10, hook_final_send);

    /* We need to do some of the same processing for digest as for send */
    LMAPI->add_hook("DIGEST", 170, hook_send_stripheaders);
    LMAPI->add_hook("DIGEST", 150, hook_send_rfc2369);
    LMAPI->add_hook("DIGEST", 120, hook_send_precedence);

    LMAPI->add_hook("SEND", 200, hook_send_stripheaders);
    LMAPI->add_hook("SEND", 150, hook_send_stripmdn);
    LMAPI->add_hook("SEND", 100, hook_send_footer); 
    LMAPI->add_hook("SEND", 100, hook_send_header); 
    LMAPI->add_hook("SEND", 100, hook_send_version);
    LMAPI->add_hook("SEND", 100, hook_send_returnpath);
    LMAPI->add_hook("SEND", 100, hook_send_forcefrom);
    LMAPI->add_hook("SEND", 100, hook_send_precedence);
    LMAPI->add_hook("SEND", 100, hook_send_replyto);
    LMAPI->add_hook("SEND", 100, hook_send_rfc2369);
    LMAPI->add_hook("SEND", 100, hook_send_xlist);
    LMAPI->add_hook("SEND", 75, hook_send_strip_rfc2369);
    LMAPI->add_hook("SEND", 60, hook_send_tag);
    LMAPI->add_hook("SEND", 55, hook_send_approvedby);

    LMAPI->add_hook("PRESEND",100, hook_presend_check_subject);
    LMAPI->add_hook("PRESEND", 75, hook_presend_check_overquoting);
    LMAPI->add_hook("PRESEND", 70, hook_presend_check_size); 
    LMAPI->add_hook("PRESEND", 70, hook_presend_check_xlist);
    LMAPI->add_hook("PRESEND", 55, hook_presend_check_outside);
    LMAPI->add_hook("PRESEND", 45, hook_presend_check_moderate);
    LMAPI->add_hook("PRESEND", 40, hook_presend_check_password);
    LMAPI->add_hook("PRESEND", 35, hook_presend_check_modpost);
    LMAPI->add_hook("PRESEND", 35, hook_presend_check_nopost);
    LMAPI->add_hook("PRESEND", 30, hook_presend_blacklist);
    LMAPI->add_hook("PRESEND", 30, hook_presend_check_messageid);
    LMAPI->add_hook("PRESEND", 30, hook_presend_check_closed);
    LMAPI->add_hook("PRESEND", 11, hook_presend_unmime);
    LMAPI->add_hook("PRESEND", 10, hook_presend_unquote);
    LMAPI->add_hook("PRESEND", 8, hook_presend_check_bcc);

    LMAPI->add_flag("NOPOST", "User cannot post to list while this flag is set.  Used for disciplinary purposes.", ADMIN_UNSAFE);
    LMAPI->add_flag("PREAPPROVE", "User's posts are automatically approved when user posts to a moderated list.", ADMIN_SAFESET | ADMIN_SAFEUNSET);
    LMAPI->add_flag("MODPOST", "User is marked as requiring moderation even when list is not moderated.", ADMIN_UNSAFE);

    LMAPI->add_hook("TOLIST", 1000, hook_tolist_sort);
    LMAPI->add_hook("TOLIST", 500, hook_tolist_remove_vacationers);
    LMAPI->add_hook("TOLIST", 0, hook_tolist_build_tolist);

    /* Files */
    LMAPI->add_file("post-password-reject", "post-password-reject-file", "File to send to users who try to post to a list with a posting password and who do not include the correct password.");
    LMAPI->add_file("closed", "closed-file", "File to send to users who try to post to a closed for public posts.");
    LMAPI->add_file("footer","footer-file","Text to append to end of messages for list.");
    LMAPI->add_file("header","header-file","Text to prepend to messages for list.");
    LMAPI->add_file("nopost","nopost-file","File to send users who are set NOPOST when they try to post to the list.");
    LMAPI->add_file("outside","outside-file","File to send users who are not subscribers, but when their post IS accepted.  (E.g. when a list is not closed-post.)");
    LMAPI->add_file("overquote", "overquote-file","File to send users who fail the overquoting check on a list.");
    LMAPI->add_file("posting-acl", "posting-acl-file","List of regexps to use as an ACL for whether or not unsubscribed users can post to a closed-post list.");

    /* Variable registration */
    LMAPI->register_var("ignore-reply-to", "no", NULL, NULL, NULL, VAR_BOOL,
                        VAR_INTERNAL|VAR_GLOBAL);
    LMAPI->register_var("message-subject", "", NULL, NULL, NULL, VAR_STRING,
                        VAR_INTERNAL|VAR_GLOBAL);
    LMAPI->register_var("subject-tag", NULL, "Basic Configuration",
                        "Optional tag to be included in subject lines of posts sent to the list.",
                        "subject-tag = [MyList]", VAR_STRING, VAR_ALL);
    LMAPI->register_var("header-file", "text/header.txt",
                        "Files", "Text to prepend to list messages.",
                        "header-file = text/header.txt",
                        VAR_STRING, VAR_ALL);
    LMAPI->register_var("footer-file", "text/footer.txt",
                        "Files", "Text to append to list messages.",
                        "footer-file = text/footer.txt",
                        VAR_STRING, VAR_ALL);
    LMAPI->register_var("reply-to", NULL, "Basic Configuration",
                        "Address which will appear in the Reply-To: header.",
                        "reply-to = list@myhost.dom", VAR_STRING, VAR_ALL);
    LMAPI->register_var("reply-to-sender", "no", "Misc",
                        "Forcibly set the Reply-To: header to be the address the mail came from.",
			"reply-to-sender = false", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("force-from-address", NULL, "Misc",
                        "If specified, this will be used as the RFC 822 From: address.",
                        "force-from-address = list-admins@myhost.dom",
                        VAR_STRING, VAR_ALL);
    LMAPI->register_var("precedence", "bulk", "Misc",
                        "The precedence header which will be included in all traffic to the list.",
                        "precedence = bulk", VAR_STRING, VAR_ALL);
    LMAPI->register_var("humanize-mime", "yes", "MIME",
                        "Should the server strip out non-text MIME attachments.",
                        "humanize-mime = true", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("body-max-size", NULL, "Posting Limits",
                        "Posts with a body larger than this in bytes will be moderated.",
                        "body-max-size = 10000", VAR_INT, VAR_ALL);
    LMAPI->register_var("header-max-size", NULL, "Posting Limits",
                        "Posts with headers larger than this in bytes will be moderated.",
                        "header-max-size = 2000", VAR_INT, VAR_ALL);
    LMAPI->register_var("rfc2369-headers", "no", "RFC2369",
                        "Should RFC 2369 headers be enabled for this list.",
                        "rfc2369-headers = on", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("rfc2369-list-help", NULL, "RFC2369",
                        "URL to use in the RFC 2369 List-help: header.",
                        "rfc2369-list-help = http://www.mydom.com/list/help.html",
                        VAR_STRING, VAR_ALL);
    LMAPI->register_var("rfc2369-listname", NULL, "RFC2369",
                        "The name of the list to use in the RFC 2369 List-name: header.",
                        "rfc2369-listname = Mylist", VAR_STRING, VAR_ALL);
    LMAPI->register_var("rfc2369-minimal", "no", "RFC2369",
                        "Should only the minimal set of RFC 2369 headers be emitted.",
                        "rfc2369-minimal = true", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("rfc2369-post-address", NULL, "RFC2369",
                        "The mailto to be used in the RFC 2369 List-post: header.   The special value of 'closed' will show the list as closed in that header.",
                        "rfc2369-post-address = myname@myhost.dom", VAR_STRING,
                        VAR_ALL);
    LMAPI->register_var("rfc2369-archive-url", NULL, "RFC2369",
                        "The URL for use in the List-archive: RFC 2369 header.",
                        "rfc2369-archive-url = ftp://ftp.myhost.dom/lists",
                        VAR_STRING, VAR_ALL);
    LMAPI->register_alias("rfc2369-archive", "rfc2369-archive-url");
    LMAPI->register_alias("rfc2369-archives", "rfc2369-archive-url");
    LMAPI->register_var("use-rfc2919-listid-subdomain", "no", "RFC2369",
                        "Instead of the 'pure' RFC2369 List-ID field, use the suggested 'list-id' subdomain addition from RFC2919 (the successor to RFC2369)",
                        "use-rfc2919-listid-subdomain = true", 
                        VAR_BOOL, VAR_ALL);
    LMAPI->register_var("rfc2369-legacy-listid", "yes", "RFC2369",
                        "Add older X-List-ID headers, for backwards compatibility.", 
                        "rfc2369-legacy-listid = false", 
                        VAR_BOOL, VAR_ALL);
    LMAPI->register_var("strip-headers", NULL, "Headers",
                        "A colon seperated list of headers to remove from outgoing messages.",
                        "strip-headers = X-pmrq:X-Reciept-To", VAR_STRING,
                        VAR_ALL);
    LMAPI->register_var("sort-tolist", "yes", "ToList",
                        "Should the recipients be sorted by domain before sending.   This is memory expensive and is a bad idea if your SMTP server already does this sorting.   If you SMTP server doesn't do this, it can really improve outgoing mail performance.",
                        "sort-tolist = off", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("cc-lists", "", "List Integration",
                        "A colon seperated list of local lists which recieve copies of all posts to this list.",
                        "cc-lists = mylist1:mylist2", VAR_STRING, VAR_LIST);
    LMAPI->register_var("moderated", "no", "Moderation",
                        "Is this list moderated.", "moderated = yes",
                        VAR_BOOL, VAR_ALL);
    LMAPI->register_var("admin-approvepost", "yes", "Moderation",
                        "Are posts by an administrator to a moderated list automatically approved.",
                        "admin-approvepost = false", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("moderator-approvepost", "yes", "Moderation",
                        "Are posts by a moderator to a moderated list automatically approved.",
                        "moderator-approvepost = false", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("closed-post", "no", "Misc",
                        "Is this list closed to posting from non-members.",
                        "closed-post = true", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("closed-post-subject", NULL, "Misc",
                        "A customized subject for closed-post rejection mails.",
                        "closed-post-subject = The List Is Closed", 
                        VAR_STRING, VAR_ALL);
    LMAPI->register_var("union-lists", NULL, "List Integration",
                        "A colon seperated list of local lists whose members can post to this list even if it is closed.",
                        "union-lists = mylist1:mylist2", VAR_STRING, VAR_LIST);
    LMAPI->register_var("closed-file", "text/closed-post.txt", "Files",
                        "File sent to message author when the post is rejected because the list is closed.",
                        "closed-file = text/closed-post.txt", VAR_STRING,
                        VAR_ALL);
    LMAPI->register_var("outside-file", "text/outside.txt", "Files",
                        "File sent to message author when post is accepted to list but author is not a subscriber.",
                        "outside-file = text/outside.txt", VAR_STRING,
                        VAR_ALL);
    LMAPI->register_var("closed-post-blackhole", "no", "Misc",
                        "Do messages submitted to a closed-post list by non-members get thrown away.",
                        "closed-post-blackhole = yes", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("nopost-file", "text/nopost.txt", "Files",
                        "File sent to users flagged NOPOST if they submit a post to the list",
                        "nopost-file = text/nopost.txt", VAR_STRING, VAR_ALL);
    LMAPI->register_var("no-dupes", "yes", "Duplicate Message detection",
                        "Should we track the Message-Id headers from incoming traffic to prevent duplicate posts to the list.   Message-Id's expire after 1 day.",
                        "no-dupes = off", VAR_BOOL, VAR_ALL);
    LMAPI->register_alias("nodupes", "no-dupes");
    LMAPI->register_var("no-dupes-forever", "no", "Duplicate Message detection",
                        "Should we never expire the Message-Id's.",
                        "no-dupes-forever = yes", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("post-password", NULL, "Moderation",
                        "If specified, all incoming messages must have an X-posting-pass: header with this password in it.",
                        "post-password = NeverBeGuessed", VAR_STRING, VAR_ALL);
    LMAPI->register_var("password-implies-approved", "no", "Moderation",
                        "If true, a correct use of X-posting-pass automatically preapproves the message.  Useful if you want to have a moderated list and allow preapproved functionality through the use of the password.",
                        "password-implies-approved = yes", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("password-failure-blackhole", "yes", "Moderation",
                        "If true, a post to a password list that doesn't have the correct password will be eaten.  If false, it will be sent to the moderators.",
                        "password-failure-blackhole = yes", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("post-password-reject-file", "text/postpassword.txt",
                        "Moderation",
                        "File sent to submitters to a password protected list if they don't include the posting password header.",
                        "post-password-reject-file = text/postpassword.txt",
                        VAR_STRING, VAR_ALL);
    LMAPI->register_alias("post-password-file", "post-password-reject-file");
    LMAPI->register_var("rfc2369-subscribe",NULL,"RFC2369",
                        "If specified, this overrides the default generated RFC2369 List-subscribe value.",
                        "rfc2369-subscribe = mailto:ecartis@ecartis.org?subject=subscribe%20ecartis-support",
                        VAR_STRING, VAR_ALL);
    LMAPI->register_var("rfc2369-unsubscribe",NULL,"RFC2369",
                        "If specified, this overrides the default generated RFC2369 List-unsubscribe value.",
                        "rfc2369-unsubscribe = mailto:ecartis@ecartis.org?subject=unsubscribe%20ecartis-support",
                        VAR_STRING, VAR_ALL);
    LMAPI->register_var("subject-required","no","Moderation",
                        "If set to true, then any post sent to the list without a subject will be made moderated.",
                        "subject-required = yes", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("tag-to-front","yes","Misc",
                        "If set to true and there is a subject tag for the list, it will be removed and moved to the beginning of the line (before any 'Re:'s).  If not set, any duplicate Re:'s will still be removed, and the subject-tag will be moved after the Re:.",
                        "tag-to-front = no", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("humanize-quotedprintable", "no", "MIME",
                        "If set to true, attempt to remove any and all quoted printable characters from subject and body replacing them with their actual character.",
                        "humanize-quotedprintable = true", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("strip-mdn", "yes", "Headers",
                        "If true, strip all read-reciept (mail delivery notification) headers from mail before sending out.",
                        "strip-mdn = on", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("quoting-limits", "no", "Quoting",
                        "If true, posts to the list will be checked for overquoting.",
                        "quoting-limits = yes", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("quoting-max-percent", "0", "Quoting",
                        "If greater than 0 and quoting-limits is true, this is the maximum percent of the message allowed to be quoted from a previous post.",
                        "quoting-max-percent = 15", VAR_INT, VAR_ALL);
    LMAPI->register_var("quoting-max-lines", "10", "Quoting",
                        "If greater than 0 and quoting-limits is true, this is the maximum number of lines allowed to be quoted from a previous message.",
                        "quoting-max-lines = 10", VAR_INT, VAR_ALL);
    LMAPI->register_var("quoting-line-reset", "yes", "Quoting",
                        "If quoting-limits is on, should the count of quoted lines be reset if the user places their own text in the message? (This has the effect of making it so that quoting-max-lines is the total number of lines that can be quoted at once IN A BLOCK, instead of in the entire message.)",
                        "quoting-line-reset = yes", VAR_BOOL, VAR_ALL);
    LMAPI->register_var("quoting-tolerance-lines", "7", "Quoting",
                        "If greater than 0, this is the number of lines that must be exceeded in the total message before the quoting percentage limit will be applied.  It would be silly to have a 25% quoting limit and have a three-line message be rejected because two lines were quoted.",
                        "quoting-tolerance-lines = 7", VAR_INT, VAR_ALL);
    LMAPI->register_var("overquote-file","text/overquote.txt", "Files",
                        "Filename under the list directory of the file sent to a user who fails an overquoting check.  (See 'quoting-limit'.)  This is the file retrieved with 'getconf overquote'.",
                        "overquote-file = text/overquote.txt", VAR_STRING,
                        VAR_ALL);
    LMAPI->register_var("overquoting-reason",NULL,NULL,NULL,NULL,
                        VAR_STRING, VAR_TEMP|VAR_INTERNAL);
    LMAPI->register_var("enforced-addressing-to",NULL,"Misc",
                        "If this is set, this address must be in the To or Cc field of a post to the list.  Usually, you would set the list address here.",
                        "enforced-addressing-to = mylist@foo.bar.com", VAR_STRING,
                        VAR_ALL);
    LMAPI->register_var("enforced-address-blackhole","false","Misc",
                        "If this is true and enforced-addressing-to is enabled, posts that fail the check will be simply eaten.  Otherwise, they will be marked for moderation.",
                        "enforced-address-blackhole = true",
                        VAR_BOOL, VAR_ALL);
    LMAPI->register_var("posting-acl-file","postacl","Files",
                        "The filename within the list directory to use for the posting-acl file.",
                        "posting-acl-file = postacl",
                        VAR_STRING, VAR_ALL);
    LMAPI->register_var("posting-acl","true","Misc",
                        "Should we check the postacl file if it exists?",
                        "posting-acl = true",
                        VAR_BOOL, VAR_ALL);
}
