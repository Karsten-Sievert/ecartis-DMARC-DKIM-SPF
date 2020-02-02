#ifndef _ALIAS_H
#define _ALIAS_H

#define ALIASHASH 64

struct alias_data {
    char *aliasname;
    char *realname;
    struct alias_data *next;
};

struct alias_info {
    struct alias_data *bucket[ALIASHASH];
};

extern struct alias_info *aliases;

extern void init_aliases(void);
extern void nuke_aliases(void);
extern int register_alias(const char *alias, const char *real);
extern const char *lookup_alias(const char *alias);

#endif /* _ALIAS_H */
