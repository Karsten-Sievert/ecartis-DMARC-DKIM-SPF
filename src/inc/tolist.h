#ifndef _TOLIST_H
#define _TOLIST_H

#include "config.h"

struct listinfo
{
    char *listname;
    char *listflags;
    struct listinfo *next;
};

struct tolist
{
    char *address;
    struct listinfo *linfo;
    struct tolist *next;
};

struct tolist_flag
{
    char *flag;
    int add;
    int flagged;
    struct tolist_flag *next;
};

extern void new_tolist(void);
extern void nuke_tolist(void);
extern void add_from_list_all(const char *list);
extern void add_from_list_flagged(const char *list, const char *flag);
extern void add_from_list_unflagged(const char *list, const char *flag);
extern void remove_user_all(const char *address);
extern void remove_flagged_all(const char *flag);
extern void remove_flagged_all_prilist(const char *flag, const char *prilist);
extern void remove_unflagged_all(const char *flag);
extern void remove_unflagged_all_prilist(const char *flag, const char *prilist);
extern void remove_list_flagged(const char *list, const char *flag);
extern void remove_list_unflagged(const char *list, const char *flag);
extern void sort_tolist(void);
extern struct tolist *start_tolist(void);
extern struct tolist *next_tolist(void);
extern void finish_tolist(void);
extern int send_to_tolist(const char *fromaddy, const char *file,
                          int do_ackpost, int do_echopost, int fullbounce);

#endif
