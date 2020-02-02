#ifndef _PASSWD_H
#define _PASSWD_H

void set_pass(const char *user, const char *newpass);
int find_pass(const char *user);
int check_pass(const char *user, const char *pass);

#endif
