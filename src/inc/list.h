#ifndef _LIST_H
#define _LIST_H

extern int set_context_list(const char *list);

extern int read_conf(const char *filename, int level);
extern int read_conf_parm(const char *filename, const char *parm, int level);
extern int list_valid(const char *listname);
extern int list_read_conf(void);

extern char *list_directory(const char *listname);
extern int listdir_file(char *buffer, const char *list, const char *filename);

extern int walk_lists(char *buf);
extern int next_lists(char *buf);

#endif /* _LIST_H */
