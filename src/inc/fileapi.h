#ifndef _FILEAPI_H
#define _FILEAPI_H

#include <stdio.h>

#ifndef WIN32
# include <sys/file.h>
#endif

#ifndef WIN32
# include <sys/types.h>
# include <dirent.h>
# define LDIR DIR*
#else
# define LDIR long
#endif

#ifndef LOCK_SH
/* Operations for the `flock' call.  */
#define LOCK_SH 1    /* Shared lock.  */
#define LOCK_EX 2    /* Exclusive lock.  */
#define LOCK_UN 8    /* Unlock.  */

/* Can be OR'd in to one of the above.  */
#define LOCK_NB 4    /* Don't block when locking.  */ 
#endif

extern FILE *open_file(const char *path, const char *mode);
extern int close_file(FILE *stream);
extern FILE *open_exclusive(const char *path, const char *mode);
extern FILE *open_shared(const char *path, const char *mode);

extern char *read_file(char *buffer, int size, FILE *stream);
extern int write_file(FILE *stream, const char *format, ...);
extern int flush_file(FILE *stream);
extern int putc_file(char output, FILE *stream);
extern int ungetc_file(int putback, FILE *stream);
extern int getc_file(FILE *stream);
extern void truncate_file (FILE *, int);
extern void rewind_file(FILE *);
extern int unlink_file(const char *);

extern int append_file(const char *, const char *);

extern int exists_file(const char *path);
extern int exists_dir(const char *path);

extern int replace_file(const char *src, const char *dest);

extern int walk_dir(const char *path, char *buffer, LDIR *dir);
extern int next_dir(LDIR dir, char *buffer);
extern int close_dir(LDIR dir);

extern int mkdirs(const char *path);

extern int public_file(const char *filename);
extern int private_file(const char *filename);

#ifdef USE_HITCHING_LOCK
void nuke_lockfiles(void);
#endif

#endif /* _FILEAPI_H */
