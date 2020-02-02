#ifndef _FILES_H
#define _FILES_H

#include "config.h"

#define FILE_NAME_LEN 64
#define FILE_VAR_LEN 32

#define FILE_NOWEBEDIT		0x01
#define FILE_NOMAILEDIT		0x02

struct list_file {
   char *name;
   char *varname;
   char *desc;
   int  flags;
   struct list_file *next;
};

extern void add_file(const char *filename, const char *varname, const char *desc);
extern void add_file_flagged(const char *filename, const char *varname, const char *desc, int flags);
extern void new_files(void);
extern void nuke_files(void);
extern struct list_file *find_file(const char *filename);
extern struct list_file *get_files(void);

#endif /* _FILES_H */
