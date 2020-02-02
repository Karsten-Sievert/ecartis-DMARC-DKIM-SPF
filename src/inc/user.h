#ifndef _USER_H
#define _USER_H

#include <stdio.h>
#include  "config.h"

#define USER_FULLBOUNCE		0x0001
#define USER_ECHOPOST		0x0002
#define USER_VACATION		0x0004
#define USER_ADMIN		0x0008

#define ERR_LISTOPEN		-1
#define ERR_LISTCREATE		-2
#define ERR_LISTWRITE		-3
#define ERR_LISTSWAP		-4

#define ERR_NOTADMIN		-5
#define ERR_FLAGSET		-6
#define ERR_FLAGNOTSET		-7
#define ERR_NOSUCHFLAG		-8
#define ERR_UNSETTABLE		-9

struct list_user {
   char address[BIG_BUF];
   char flags[HUGE_BUF];
   struct list_user *next;
};

extern int user_read(FILE *userfile, struct list_user *user);
extern int user_find(const char *listusers, const char *addy,
                     struct list_user *user);
extern int user_find_list(const char *list, const char *addy,
                          struct list_user *user);
extern int user_add(const char *listusers, const char *fromaddy);
extern int user_remove(const char *listusers, const char *fromaddy);
extern int user_write(const char *listusers, struct list_user *user);
extern int user_setflag(struct list_user *user, const char *flag, int admin);
extern int user_unsetflag(struct list_user *user, const char *flag, int admin);
extern int user_hasflag(struct list_user *user, const char *flag);

#endif /* _USER_H */
