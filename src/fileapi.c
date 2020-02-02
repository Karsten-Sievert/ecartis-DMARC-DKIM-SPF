#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

#ifndef WIN32
#include <sys/file.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#else
#include <io.h>
#include <share.h>
#endif /* WIN32 */

#include "core.h"
#include "compat.h"
#include "fileapi.h"
#include "mystring.h"
#include "variables.h"

#ifndef MAX_FNAME
#define MAX_FNAME 1024
#endif

#ifdef NEED_STRERROR
#define strerror(a) sys_errlist[(a)]
#endif

#ifdef USE_HITCHING_LOCK

/* This should probably be more global, but for now, this is okay */
#ifdef NO_MEMMOVE
#define memmove(dst, src, len)	bcopy(src, dst, len)
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	256
#endif

#define MAXDESCPERFILE	8	/* How many times one file can be open */
#define MAXOPENFILES	32	/* How many files can be open */

#define LOCKTRIES	155	/* How many times to try */
#define LOCKSLEEP	2	/* How much to sleep each time */
#define OLDLOCK		300	/* Lockfiles this old are removed */

struct lockfiles_t {
    int descs;
    FILE *fp[MAXDESCPERFILE];
    char *lockfn;
};

struct lockfiles_t lockfiles[MAXOPENFILES];
int openfiles = 0;

int dot_lock(const char *path)
{
    static char	hitchfile[MAX_FNAME];
    static char	lockfile[MAX_FNAME];
    static char	hostname[MAXHOSTNAMELEN] = "";
    struct stat	sb;
    int		hitchfd, res, i;
    time_t	now;

    buffer_printf(lockfile, sizeof(lockfile) - 1, "%s.lock", path);
    /* Try to find the file among open files. */
    for (i = 0; i < openfiles; i++)
		if (strcmp(lockfiles[i].lockfn, lockfile) == 0)
			return i;
    /* Not found, create the lock. */
	if (openfiles >= MAXOPENFILES)
		return -1;			/* Probably should panic */
    /*
     * Ok, guys and girls, the hostname must be stored in a global
     * variable.  At least, the hostname is looked up once here.
     */
    if (!*hostname)
		if (gethostname(hostname, MAXHOSTNAMELEN) == -1)
			return -1;			/* Oops... */
    buffer_printf(hitchfile, sizeof(hitchfile) - 1, "%s.%s.%08x.%08x", lockfile, hostname,
	(unsigned int)time(NULL), (unsigned int)getpid());

    for (i = 0; i < LOCKTRIES; sleep(LOCKSLEEP), i++) {
		/*
		 * lstat(2) the lockfile and see if it's older than OLDLOCK.
		 * If it is, remove it.
		 */
		now = time(NULL);
		if (lstat(lockfile, &sb) == 0 && difftime(now, sb.st_mtime) > OLDLOCK)
			unlink(lockfile);
		/* Create the hitching post file.  */
		hitchfd = open(hitchfile, O_WRONLY | O_CREAT | O_EXCL, 0644);
		if (hitchfd == -1) {
			/* XXX: may check for EACCESS */
			return -1;
		}
		/*
		 * Now, link the lockfile to the hitching post file.  If the
		 * link(2) system call fails, then fstat(2) the hitching post
		 * file descriptor.  If number of links is 2, it means that
		 * the call did succeed, but the ACK hasn't got back from the
		 * NFS server (probably because of a crash).
		 */
		if ((res = link(hitchfile, lockfile)) == -1)
			fstat(hitchfd, &sb);
		close(hitchfd);
		unlink(hitchfile);
		if (res != -1 || sb.st_nlink == 2) {
			/* success */
			if ((lockfiles[openfiles].lockfn = strdup(lockfile)) == NULL) {
				unlink(lockfile);
				return -1;
			}
			return -2;
		}
	}
    return -1;
}

FILE * dot_fopen(const char *path, const char *mode)
{
    int		i;
    FILE	*fp;

    i = dot_lock(path);
    if (i == -1) {
		log_printf(10, "Error creating lockfile (%s)\n", strerror(errno));
		return NULL;
    }
    if ((fp = fopen(path, mode)) == NULL) {
		if (i == -2) {
			unlink(lockfiles[openfiles].lockfn);
			free(lockfiles[openfiles].lockfn);
			lockfiles[openfiles].lockfn = NULL;
		}
		return NULL;
	}
	if (i == -2) {
		lockfiles[openfiles].descs = 1;
		lockfiles[openfiles++].fp[0] = fp;
	} else {
		if (lockfiles[i].descs >= MAXDESCPERFILE)
			return NULL;			/* Probably should panic */
		lockfiles[i].fp[lockfiles[i].descs++] = fp;
	}
    return fp;
}

int dot_unlock(FILE *stream)
{
    int	i, j, res;

    for (i = 0; i < openfiles; i++)
	for (j = 0; j < lockfiles[i].descs; j++)
	    if (lockfiles[i].fp[j] == stream) {
		/*
		 * Shrink the FILE pointers array.
		 */
		if (--lockfiles[i].descs > j)
		    memmove(&lockfiles[i].fp[j], &lockfiles[i].fp[j + 1],
			sizeof(lockfiles[0].fp[0]) * (lockfiles[i].descs - j));
		/*
		 * If there file is open more than once, we are finished.
		 */
		if (lockfiles[i].descs)
		    return 0;
		/*
		 * Unlink the lockfile and shrink the lockfile array.
		 */
		res = unlink(lockfiles[i].lockfn);
		free(lockfiles[i].lockfn);
                lockfiles[i].lockfn = NULL;
		if (--openfiles > i)
		    memmove(&lockfiles[i], &lockfiles[i + 1],
			sizeof(lockfiles[0]) * (openfiles - i));
		if (res == -1 && errno != ENOENT)
		    return -1;
		return 0;
	    }
    return -1;
}

/* Wipe the lock files */
void nuke_lockfiles()
{
    int i;

    for (i = 0; i < openfiles; i++) {
        unlink(lockfiles[i].lockfn);
        free(lockfiles[i].lockfn);
        lockfiles[i].lockfn = NULL;
    }
    openfiles = 0;
}
#endif /* USE_HITCHING_LOCK */

#ifndef WIN32
int file_lock (int fd, int operation)
{
#ifndef NEED_FLOCK
  while (flock(fd, operation) == -1) {
    log_printf(10, "Error gaining %s flock lock: (%s)\n", 
	       (operation & LOCK_NB) ? "non-blocking" : "blocking",
	       strerror(errno));
    if (operation & LOCK_NB) return -1;
  }
  return 0;
#else
  struct flock lbuf;

  switch (operation & ~LOCK_NB)
    {
    case LOCK_SH:
      lbuf.l_type = F_RDLCK;
      break;
    case LOCK_EX:
      lbuf.l_type = F_WRLCK;
      break;
    case LOCK_UN:
      lbuf.l_type = F_UNLCK;
      break;
    default:
      errno = EINVAL;
      return -1;
    }

  lbuf.l_whence = SEEK_SET;
  lbuf.l_start = lbuf.l_len = 0L; /* Lock the whole file.  */

  while (fcntl(fd, (operation & LOCK_NB) ? F_SETLK : F_SETLKW, &lbuf) == -1) {
    log_printf(10, "Error gaining %s fcntl lock: (%s)\n", 
	       (operation & LOCK_NB) ? "non-blocking" : "blocking",
	       strerror(errno));
    if (operation & LOCK_NB) return -1;
  }
  return 0;
#endif /* NEED_FLOCK */
}
#endif /* WIN32 */

/* Ready a file for open, lock it according to the open mode */
FILE *open_file(const char *path, const char *mode)
{
#ifndef WIN32
#ifdef USE_HITCHING_LOCK
    FILE *fp = dot_fopen(path, mode);
#else
    FILE *fp = fopen(path, mode);
    if (fp) {
        int ltype = LOCK_EX;
        if(mode[0] == 'r' && !strchr(mode, '+'))
            ltype = LOCK_SH;

	file_lock(fileno(fp), ltype);
    }
#endif /* USE_HITCHING_LOCK */
    return fp;
#else
    FILE *fp;

    if ((mode[0] == 'r') &&  (!exists_file(path))) return NULL;

    fp = fopen(path, mode);

    debug_printf("open_file: %s (%d)\n", path, fp ? _fileno(fp) : -1);

    return (fp);
#endif /* WIN32 */
}

/* Force a file to be opened exclusive, regardless of mode */
FILE *open_exclusive(const char *path, const char *mode)
{
#ifndef WIN32
#ifdef USE_HITCHING_LOCK
    FILE *fp = dot_fopen(path, mode);
#else
    FILE *fp = fopen(path, mode);
    if(fp) {
        file_lock(fileno(fp), LOCK_EX);
    }
#endif /* USE_HITCHING_LOCK */
    return fp;
#else
    FILE *fp;

    if ((mode[0] == 'r') &&  (!exists_file(path))) return NULL;

    fp = fopen(path, mode);

    debug_printf("open_file: %s (%d)\n", path, _fileno(fp));

    return (fp);
#endif /* WIN32 */
}

/* Force a file to be opened shared, regardless of mode */
FILE *open_shared(const char *path, const char *mode)
{
#ifndef WIN32
#ifdef USE_HITCHING_LOCK
    FILE *fp = dot_fopen(path, mode);
#else
    FILE *fp = fopen(path, mode);
    if(fp) {
        file_lock(fileno(fp), LOCK_SH);
    }
#endif /* USE_HITCHING_LOCK */
    return fp;
#else
    FILE *fp;

    if ((mode[0] == 'r') &&  (!exists_file(path))) return NULL;

    fp = fopen(path, mode);

    debug_printf("open_file: %s (%d)\n", path, _fileno(fp));

    return (fp);
#endif /* WIN32 */
}

/* Close and unlock a file */
int close_file(FILE *stream)
{
    if (!stream) {
       log_printf(2,"Error: tried to close a null stream.\n");
       return 0;
    }

#ifdef WIN32
    debug_printf("close_file: %d\n", _fileno(stream));
#endif

    fflush(stream);
    if (fclose(stream) == -1)
	return -1;
#ifdef USE_HITCHING_LOCK
    return dot_unlock(stream);
#else
    return 0;
#endif /* USE_HITCHING_LOCK */
}

int flush_file(FILE *stream)
{
    if (!stream) {
       log_printf(2,"Error: tried to flush a null stream.\n");
       return 0;
    }

    return fflush(stream);
}

char *read_file(char *buf, int size, FILE *stream)
{
    if (!stream) {
        log_printf(2,"Error: tried to read from a null stream.\n");
       	return NULL;
    }

    return (fgets(buf,size,stream));
}

int getc_file(FILE *stream)
{
    return (fgetc(stream));
}

int putc_file(char output, FILE *stream)
{
    return fputc(output,stream);
}

int ungetc_file(int putback, FILE *stream)
{
    return ungetc(putback,stream);
}

int write_file(FILE *stream, const char *format, ...)
{
    va_list vargs;
    int result;

    if(!stream) {
        log_printf(2,"Error: tried to write to a null stream.\n");
        return -1;
    }

    va_start(vargs, format);
    result = vfprintf(stream, format, vargs);
    va_end(vargs);

    return result;
}

int exists_file(const char *filename)
{
    struct stat st;

    if(stat(filename, &st) == 0)
        return 1;
	
    return 0;
}

int exists_dir(const char *filename)
{
    struct stat st;

    if(stat(filename, &st) != 0)
        return 0;
	
#ifndef WIN32
    if(!S_ISDIR(st.st_mode))
        return 0;
#else
    if (!(st.st_mode & _S_IFDIR))
        return 0;
#endif /* WIN32 */

    return 1;
}

int walk_dir(const char *path, char *buffer, LDIR *dir)
{
#ifndef WIN32
    struct dirent *dp;
    DIR *dw_df;

    /* Sanity! */
    *dir = NULL;

    log_printf(15,"DIRWALK: walk_dir(%s)\n", path);

    if((dw_df = opendir(path)) == NULL)
        return 0;

    while ((dp = readdir(dw_df)) && (dp ? dp->d_name[0] == '.' : 1));

    if (dp) {
        log_printf(15,"DIRWALK: found (%s)\n", dp->d_name);
        buffer_printf(buffer, BIG_BUF - 1, "%s", dp->d_name);
        *dir = dw_df;
        return 1;
    } else {
        log_printf(15,"DIRWALK: end of dir\n");
        /* Let the user close the directory if it opened successfully, to avoid problems. */
        return 0;
    }
#else
    char tbuf[SMALL_BUF];
    struct _finddata_t finddat;
    long dw_handle;

    log_printf(15,"DIRWALK: walk_dir(%s)\n", path);

    buffer_printf(tbuf, sizeof(tbuf) - 1, "%s/*.*", path);

    dw_handle = _findfirst(tbuf,&finddat);
    *dir = dw_handle;

    if (dw_handle == -1) {
        log_printf(15,"DIRWALK: end of dir\n");
        dw_handle = 0;
        return 0;
    }

    buffer_printf(buffer, BIG_BUF - 1, "%s", finddat.name);
    log_printf(15,"DIRWALK: found (%s)\n", finddat.name);

    return 1;
#endif /* WIN32 */
}

int next_dir(LDIR dir, char *buffer)
{
#ifndef WIN32
    struct dirent *dp;

    log_printf(15,"DIRWALK: next_dir\n");

    if (!dir) return 0;

    while ((dp = readdir(dir)) && (dp ? dp->d_name[0] == '.' : 1));

    if (dp) {
        log_printf(15,"DIRWALK: found (%s)\n", dp->d_name);
        buffer_printf(buffer, BIG_BUF - 1, "%s", dp->d_name);
        return 1;
    } else {
        log_printf(15,"DIRWALK: end of dir\n");
        return 0;
    }		
#else
    struct _finddata_t finddat;
    int status;

    log_printf(15,"DIRWALK: next_dir\n");

    if (!dir) return(0);

    if ((status = _findnext(dir,&finddat)) != 0) {
        log_printf(15,"DIRWALK: end of dir\n");
        return 0;
    }

    while ((finddat.name[0] == '.') && !status) {
        status = _findnext(dir,&finddat);
    }

    buffer_printf(buffer, BIG_BUF - 1, "%s", finddat.name);
    log_printf(15,"DIRWALK: found (%s)\n", finddat.name);

    return 1;
#endif /* WIN32 */
}

int close_dir(LDIR dir)
{
#ifndef WIN32
    if (dir)
        closedir(dir);
#else
    log_printf(15,"DIRWALK: close_dir\n");
    if (dir)
        _findclose(dir);
#endif /* WIN32 */
    return 1;
}

/* Make all directories along a given path if they don't exist */
int mkdirs(const char *path)
{
    char buf[MAX_FNAME];
    char cur[MAX_FNAME];
    char *c;

    /*
     * Okay, if the path doesn't end with a '/', assume the last element
     * is a filename, not a directory.
     */
    memset(buf, 0, sizeof(buf));
    memset(cur, 0, sizeof(cur));
    stringcpy(buf, path);

    c = strrchr(buf, '/');
#ifdef WIN32
    if(!c) c = strrchr(buf, '\\');
#endif
    if(c)
      *c = '\0';

    /* correct for braindamage of strtok */
    if(*path == '/') stringcpy(cur, "/");
    
    c = strtok(buf, "/\\");
    while(c) {
        if (cur[0]) stringcat(cur, "/");
        stringcat(cur, c);
        if(exists_file(cur)) {
            /* Something with that name exists */
            if(!exists_dir(cur)) {
                /* It exists.. bail */
                return 0;
            } else {
                /* It exists and is a dir, continue */
                c = strtok(NULL, "/\\");
            }
        } else {
            /* Dir doesn't exist, so create it */
#ifndef WIN32
            mkdir(cur, 0775);
#else
            mkdir(cur);
#endif /* WIN32 */
            c = strtok(NULL, "/\\");
        }
    }
    return 1;
}

int replace_file(const char *src, const char *dest)
{
    int bad;
#ifdef WIN32
    log_printf(15,"Fileapi: unlinking %s...\n", dest);
    (void)unlink(dest);
#endif /* WIN32 */
    bad = rename(src,dest);
    if (bad && (errno == EXDEV)) {
       FILE *outfile, *infile;
       int input;

       /* Files are on different physical filesystems, cannot use rename */
       if ((infile = open_file(src,"r")) == NULL) {
          return bad;
       }
       if ((outfile = open_file(dest,"w")) == NULL) {
          close_file(infile);
          return bad;
       }
       
       while((input = fgetc(infile)) != EOF) {
         fputc((char)input,outfile);
       }
       close_file(outfile);
       close_file(infile);
       (void)unlink(src);
       return 0;
    }

    return bad;
}

void truncate_file(FILE *fp, int size)
{
    ftruncate(fileno(fp), size);
}

void rewind_file(FILE *fp)
{
    rewind(fp);
}

int unlink_file(const char *name)
{
    return unlink(name);
}

int append_file(const char *dest, const char *source)
{
   int inchar;
   FILE *infile, *outfile;

   if ((infile = open_file(source,"r")) == NULL) {
      log_printf(9,"Unable to open file %s for read\n", dest);
      return 0;
   }

   if ((outfile = open_file(dest,"a")) == NULL) {
      log_printf(9,"Unable to open file %s for append\n", dest);
      close_file(infile);
      return 0;
   }

   while((inchar = getc_file(infile)) != EOF) {
       putc_file((char)inchar,outfile);
   }

   close_file(infile);
   close_file(outfile);

   return 1;
}

int public_file(const char *filename)
{
#ifndef WIN32
   return chmod(filename, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
#else
   return 0;
#endif
}

int private_file(const char *filename)
{
#ifndef WIN32
   return chmod(filename, S_IRUSR|S_IWUSR);
#else
   return 0;
#endif
}

