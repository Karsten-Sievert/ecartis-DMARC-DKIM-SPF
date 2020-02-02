#include <stdio.h>

#include "password.h"

struct LPMAPI *LMAPI;

void password_switch_context(void)
{
   LMAPI->log_printf(15, "Switching context in module Password\n");
}

void password_unload(void)
{
   LMAPI->log_printf(10, "Unloading module Password\n");
}

int password_upgradelist(int prev, int cur)
{
   LMAPI->log_printf(10, "Upgrading lists in module Password\n");
   return 1;
}

int password_upgrade(int prev, int cur)
{
   LMAPI->log_printf(10, "Upgrading module Password\n");
   return 1;
}

void password_init(void)
{
   LMAPI->log_printf(10, "Initializing module Password\n");
}

void password_load(struct LPMAPI* api)
{
   LMAPI = api;

   /* Log the module initialization */
   LMAPI->log_printf(10, "Loading module Password\n");

   /* Register us as a modules */
   LMAPI->add_module("Password", "Password authentication module");

   /* Register a variable so that we know we're authenticated */
   LMAPI->register_var("authenticated", "no", NULL, NULL, NULL, VAR_BOOL,
                       VAR_GLOBAL|VAR_INTERNAL);
   LMAPI->register_var("allow-site-passwords", "no", "Password",
                       "Are sitewide passwords allowed.",
                       "allow=site-passwords = false", VAR_BOOL,
                       VAR_GLOBAL|VAR_SITE);
   LMAPI->register_var("password-expiration-time", "1 h", "Password",
                       "How quickly to auth-password cookies expire.",
                       "password-expiration-time = 2 d", VAR_DURATION,
                       VAR_GLOBAL|VAR_SITE);

   /* Register two command handlers */
   LMAPI->add_command("setpassword",
                      "Allows a user to set or change their password.", 
                      "setpassword <newpassword>",
                      NULL, NULL,
                      CMD_HEADER|CMD_BODY, cmd_setpassword);
   LMAPI->add_command("password",
                      "Authenticates a user with the given password.",
                      "password <password>", NULL, NULL,
                      CMD_HEADER|CMD_BODY, cmd_password);
   LMAPI->add_command("authpwd",
                      "Authenticate a password change",
                      "authpwd <cookie>", NULL, NULL, CMD_BODY, cmd_authpwd);

   /* Register a password change cookie */
   LMAPI->register_cookie('P', "password-expiration-time", NULL, NULL);
}
