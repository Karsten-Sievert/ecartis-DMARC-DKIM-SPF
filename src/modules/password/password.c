#include "password.h"
#include <string.h>

CMD_HANDLER(cmd_setpassword)
{
    const char *user, *password;
    int auth = 0;

    if(!LMAPI->get_bool("allow-site-passwords")) {
        LMAPI->spit_status("Sitewide passwords are not allowed at this site.");
        return CMD_RESULT_CONTINUE;
    }

    if(params->num == 0) {
        LMAPI->spit_status("You must provide a new password.");
        return CMD_RESULT_CONTINUE;
    } else {
        password = params->words[0];
        user = LMAPI->get_string("fromaddress");
    }
    if(LMAPI->find_pass(user) == 1) {
        if(LMAPI->get_bool("authenticated")) {
            auth = 1;
            LMAPI->set_pass(user, password);
            LMAPI->spit_status("Password successfully set.");
        } 
    }
    if(!auth) {
        char cookie[BIG_BUF];
        char buf[BIG_BUF];

        LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%s/cookies",  LMAPI->get_string("lists-root"));

        LMAPI->set_var("cookie-for", user, VAR_TEMP);
        if(!LMAPI->request_cookie(buf, &cookie[0], 'P', password)) {
            LMAPI->spit_status("Unable to generate auth password cookie.");
            LMAPI->filesys_error(buf);
            return CMD_RESULT_CONTINUE;
        }
        LMAPI->clean_var("cookie-for", VAR_TEMP);
        LMAPI->buffer_printf(buf, sizeof(buf) - 1, "Password change request for '%s'", user);
        LMAPI->set_var("task-form-subject", buf, VAR_TEMP);
        if(!LMAPI->task_heading(user)) {
            LMAPI->spit_status("Unable to send confirmation ticket.");
            return CMD_RESULT_CONTINUE;
        }
        LMAPI->smtp_body_line("# Password change request recieved.");
        LMAPI->smtp_body_line("#");
        LMAPI->smtp_body_line("# To acctually accomplish the change, reply to this message.");
        LMAPI->smtp_body_line("#");
        LMAPI->smtp_body_line("// job");
        LMAPI->buffer_printf(buf, sizeof(buf) - 1, "authpwd %s", cookie);
        LMAPI->smtp_body_line(buf);
        LMAPI->smtp_body_line("// eoj");
        LMAPI->task_ending();
        LMAPI->spit_status("Password confirmation ticket sent.");
    }
    return CMD_RESULT_CONTINUE;
}

CMD_HANDLER(cmd_authpwd)
{
    char cdat[BIG_BUF];
    char buf[BIG_BUF];

    if(!LMAPI->get_bool("allow-site-passwords")) {
        LMAPI->spit_status("Sitewide passwords are not allowed at this site.");
        return CMD_RESULT_CONTINUE;
    }

    if(params->num != 1) {
        LMAPI->spit_status("Invalid number of parameters for authpwd.");
        return CMD_RESULT_CONTINUE;
    }
    LMAPI->buffer_printf(buf, sizeof(buf) - 1, "%s/cookies", LMAPI->get_string("lists-root"));
    if(LMAPI->verify_cookie(buf, params->words[0], 'P', &cdat[0])) {
        if(LMAPI->match_cookie(params->words[0],
                               LMAPI->get_string("fromaddress"))) {
            LMAPI->del_cookie(buf, params->words[0]);
            LMAPI->set_pass(LMAPI->get_string("fromaddress"), cdat);
            LMAPI->spit_status("Password successfully set.");
        } else {
            LMAPI->spit_status("Cookie cannot be used from this address.");
        }
    } else {
        LMAPI->spit_status("No such cookie or else cookie is of wrong type.");
    } 
 
    return CMD_RESULT_CONTINUE;
}

CMD_HANDLER(cmd_password)
{
    const char *pass, *user;

    if(!LMAPI->get_bool("allow-site-passwords")) {
        LMAPI->spit_status("Sitewide passwords are not allowed at this site.");
        return CMD_RESULT_CONTINUE;
    }

    if(params->num == 0) {
        LMAPI->spit_status("You must provide a password.");
        return CMD_RESULT_CONTINUE;
    }
    pass = params->words[0];
    user = LMAPI->get_string("fromaddress");
    if(LMAPI->check_pass(user, pass)) {
        LMAPI->spit_status("Password accepted.   Authenticated as '%s'.", user);
        LMAPI->set_var("authenticated", "yes", VAR_GLOBAL);
    } else {
        LMAPI->spit_status("Password rejected.   Not authenticated.");
    }
    return CMD_RESULT_CONTINUE;
}
