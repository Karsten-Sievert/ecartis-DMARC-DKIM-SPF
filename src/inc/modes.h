#ifndef _MODES_H
#define _MODES_H

#define MAX_MODE_LEN 64

typedef int (*ModeFn)(void);

struct listserver_mode {
    char *modename;
    ModeFn modefn;
    struct listserver_mode *next;
};


#define MODE_HANDLER(a) int a(void)

#define MODE_ERR 0
#define MODE_OK  1
#define MODE_END 2

extern void add_mode(const char *mode, ModeFn fn);
extern void new_modes(void);
extern void nuke_modes(void);
extern struct listserver_mode *find_mode(const char *mode);
extern struct listserver_mode *get_modes(void);

extern struct listserver_mode *modes;
#endif /* _MODES_H */
