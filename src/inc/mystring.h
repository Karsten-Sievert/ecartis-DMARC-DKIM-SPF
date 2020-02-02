#ifndef _MYSTRING_H
#define _MYSTRING_H

#include <stdarg.h>

extern int buffer_printf(char *buffer, int length, char *format, ...);
extern char *lowerstr(const char *);
extern char *upperstr(const char *);
extern char *strcasestr(const char *, const char *);
extern void strreplace(char *, int, const char *, const char *, const char *);
extern void strcasereplace(char *, int, const char *, const char *, const char *);
extern int address_match(const char *, const char *);
extern int check_address(const char *);
extern void trim(char *instr);
#endif
