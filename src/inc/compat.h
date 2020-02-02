#ifndef _COMPAT_H
#define _COMPAT_H

# ifdef sun   /* We need to define externs for sys_errlist manually */
extern char *sys_errlist[];
extern int sys_nerr;

#  ifndef SUNOS_5 /* Well, then we must be SunOS 4 */
#   define RTLD_LISTSERVER (1)
#  endif

# endif

#ifdef NEED_STRRCHR
#define strrchr(x, y) rindex((x), (y))
#endif

#ifdef NEED_STRCHR
#define strchr(x, y) index((x), (y))
#endif

# ifdef WIN32
#  define strcasecmp(x,y)	_stricmp(x,y)
#  define strncasecmp(x,y,z)	_strnicmp(x,y,z)
#  define chdir(x)		_chdir(x)
#  define ftruncate(x,y)        _chsize(x,y)
#  define getpid		GetCurrentProcessId
#  include <windows.h>
#  include <direct.h>
#  include <io.h>
#  define EX_TEMPFAIL 75
#  define UNCRYPTED_PASS
# endif

#ifdef IRIX_IS_CRAP
#include <strings.h> /* required for rindex() */
#include <bstring.h> /* required for bcopy() */
#endif /* IRIX_IS_CRAP */

#if defined(SUNOS_5) || defined(_AIX)
# include <strings.h> /* required for some string functions */
#endif

#endif /* _COMPAT_H */
