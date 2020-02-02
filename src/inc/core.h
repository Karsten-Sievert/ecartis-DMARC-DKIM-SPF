#ifndef _CORE_H
#define _CORE_H

#include <stdarg.h>
#include <time.h>
#ifndef WIN32
#include <sys/time.h>
#endif

#include "config.h"

extern int send_textfile(const char *address, const char *textfile);
extern int send_textfile_expand(const char *address, const char *textfile);
extern int send_textfile_expand_append(const char *address, 
                     const char *textfile, int includequeue);
extern int flagged_send_textfile(const char *fromaddy, const char *list,
                     const char *flag, const char *file, const char *subject);
extern int blacklisted(const char *address);
extern int match_reg(const char *pattern, const char *match);
extern void log_printf(int level, char *format, ...);
extern void debug_printf(char *format, ...);
extern void result_printf(char *format, ...);
extern void result_append(const char *filename);
extern void quote_command();
extern void filesys_error(const char *filename);
extern void internal_error(const char *message);
extern void spit_status(const char *statustext, ...);
extern void bounce_message(void);
extern const char *resolve_error(int error);
extern void nosuch(const char *);
extern void get_date(char *buffer, int len, time_t now);
extern void do_sleep(int millis);
extern int expand_append(const char *mainfile, const char *liscript);
extern void build_hostname(char *buffer, int len);
extern int generate_queue();

#endif /* _CORE_H */
