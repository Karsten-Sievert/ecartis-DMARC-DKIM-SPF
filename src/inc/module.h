#ifndef _MODULE_H
#define _MODULE_H

struct listserver_module {
   char	*name;
   char	*desc;
   char advert;
   struct listserver_module *next;
};

extern void new_modules(void);
extern void nuke_modules(void);

extern void add_module(const char *, const char *);
extern struct listserver_module *get_modules(void);
#endif /* _MODULE_H */
