#ifndef _FLAG_H
#define _FLAG_H

#define MAX_FLAG_NAME 32

struct listserver_flag {
   char *name;
   char *desc;
   int  admin;
   struct listserver_flag *next;
};

#define ADMIN_UNSAFE		0x01
#define ADMIN_SAFESET		0x02
#define ADMIN_SAFEUNSET		0x04
#define ADMIN_UNSETTABLE	0x08

extern void new_flags(void);
extern void nuke_flags(void);
extern void add_flag(const char *name, const char *desc, int admin);
extern struct listserver_flag *get_flag(const char *name);
extern struct listserver_flag *get_flags(void);

#endif /* _FLAG_H */
