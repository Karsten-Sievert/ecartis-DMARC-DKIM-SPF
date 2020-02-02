#ifndef _SUBMODES_H
#define _SUBMODES_H
struct submode
{
    char *modename;
    char *flaglist;
    char *description;
    struct submode *next;
};

extern struct submode *submodes;

extern void new_submodes(void);
extern void nuke_submodes(void);
extern void read_submodes(const char *list);
extern struct submode *get_submodes(void);
extern struct submode *get_submode(const char *mode);
extern const char *get_submode_flags(const char *mode);

#endif /* _SUBMODES_H */
