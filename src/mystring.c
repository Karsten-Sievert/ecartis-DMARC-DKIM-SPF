#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "mystring.h"
#include "snprintf.h"
#include "core.h"

int buffer_printf(char *buffer, int length, char *format, ...)
{
    va_list vargs;
    int fulllen;

    va_start(vargs, format);
#ifdef WIN32
    fulllen = _vsnprintf(buffer, length, format, vargs);
#else
#ifdef NEED_SNPRINTF
    fulllen = my_vsnprintf(buffer, length, format, vargs);
#else
    fulllen = vsnprintf(buffer, length, format, vargs);
#endif
#endif
    va_end(vargs);
    return fulllen; 
}

/* This creates a NEW string, so it needs to be freed */
char *lowerstr(const char *str)
{
    char *ret = strdup(str);
    char *s = ret;
    while(*s) {
        *s = tolower(*s);
        s++;
    }
    return ret;
}

/* This creates a NEW string, so it needs to be freed */
char *upperstr(const char *str)
{
    char *ret = strdup(str);
    char *s = ret;
    while(*s) {
        *s = toupper(*s);
        s++;
    }
    return ret;
}

char *strcasestr(const char *haystack, const char *needle)
{
    char *h1 = lowerstr(haystack);
    char *n1 = lowerstr(needle);
    char *s1 = strstr(h1, n1);
    char *res = NULL;

    if(s1) {
       int a = s1-h1;
       res = (char *)(haystack+a);
    }
    if(h1) free(h1);
    if(n1) free(n1);
    return res;
}

void strreplace(char *newstr, int len, const char *base, const char *orig, 
                const char *replace) 
{
#ifndef WIN32
    char xbuf[len];
    char buf[len];
#else
    char *xbuf, *buf;
#endif
    int i = 0, j = 0;
    int k;

#ifdef WIN32
    xbuf = (char *)malloc(len);
    buf = (char *)malloc(len);
#endif

    k = strlen(replace);

#ifndef WIN32
    bcopy(base, xbuf, strlen(base) + 1);
#else
    memcpy(xbuf, base, strlen(base) + 1);
#endif

    buf[0] = '\0';

    while (xbuf[i]) {
       if (!strncmp(xbuf + i, orig, strlen(orig))) {
          if ((j + k + 1) < len) {
              strncat(buf, replace, len - 1 - strlen(buf));
              i += strlen(orig);
              j += k;
          }
       } else {
          if ((j + 1) < len) {
              buf[j++] = xbuf[i++];
              buf[j] = '\0';
          }
       }
    }
    buffer_printf(newstr, len, "%s", buf);

#ifdef WIN32
    if(xbuf) free(xbuf);
    if(buf) free(buf);
#endif
}

void strcasereplace(char *newstr, int len, const char *base, const char *orig, 
                const char *replace) 
{
#ifndef WIN32
    char xbuf[len];
    char buf[len];
#else
    char *xbuf, *buf;
#endif
    int i = 0, j = 0;
    int k;

#ifdef WIN32
    xbuf = (char *)malloc(len);
    buf = (char *)malloc(len);
#endif

    k = strlen(replace);

#ifndef WIN32
    bcopy(base, xbuf, strlen(base) + 1);
#else
    memcpy(xbuf, base, strlen(base) + 1);
#endif

    buf[0] = '\0';

    while (xbuf[i]) {
       if (!strncasecmp(xbuf + i, orig, strlen(orig))) {
          if ((j + k + 1) < len) {
              strncat(buf, replace, len - 1 - strlen(buf));
              i += strlen(orig);
              j += k;
          }
       } else {
          if ((j + 1) < len) {
              buf[j++] = xbuf[i++];
              buf[j] = '\0';
          }
       }
    }
    buffer_printf(newstr, len, "%s", buf);

#ifdef WIN32
    free(xbuf);
    free(buf);
#endif
}

int address_match(const char *addy1, const char *addy2)
{
    char domain1[SMALL_BUF], domain2[SMALL_BUF];
    char user1[SMALL_BUF], user2[SMALL_BUF];
    char buf[BIG_BUF], mbuf1[BIG_BUF], mbuf2[BIG_BUF];
    char *tptr1, *tptr2, *tptr3;

    buffer_printf(buf, sizeof(buf) - 1, "%s", addy1);

    tptr1 = strchr(buf, '@');
    if (!tptr1) {
        return (strcasecmp(addy1,addy2) == 0);
    };

    *tptr1++ = 0;

    buffer_printf(user1, sizeof(user1) - 1, "%s", buf);

    /* Handle dotted non-local hosts correctly. */
    tptr2 = strchr(tptr1,'.');
    if (tptr2) {
        tptr2++;

        while ((tptr3 = strchr(tptr2,'.'))) {
           tptr1 = tptr2;
           tptr2 = tptr3 + 1;
        }                  
    }

    buffer_printf(domain1, sizeof(domain1) - 1, "%s", tptr1);

    buffer_printf(buf, sizeof(buf) - 1, "%s", addy2);

    tptr1 = strchr(buf, '@');
    if (!tptr1) return 0;

    *tptr1++ = 0;

    buffer_printf(user2, sizeof(user2) - 1, "%s", buf);

    /* Handle dotted non-local hosts correctly. */
    tptr2 = strchr(tptr1,'.');
    if (tptr2) {
        tptr2++;

        while ((tptr3 = strchr(tptr2,'.'))) {
           tptr1 = tptr2;
           tptr2 = tptr3 + 1;
        }                  
    }

    buffer_printf(domain2, sizeof(domain2) - 1, "%s", tptr1);

    buffer_printf(mbuf1, sizeof(mbuf1) - 1, "%s@%s", user1, domain1);
    buffer_printf(mbuf2, sizeof(mbuf2) - 1, "%s@%s", user2, domain2);

    return (strcasecmp(mbuf1,mbuf2) == 0);
}

int check_address(const char *addy)
{
    char validuser[BIG_BUF];
    /* Jump through hoops to build the regexp */
    stringcpy(validuser, "^[^@( ){}<>[]+@[^@(){}<> .[]+\\.[^@( ){}<>[]+");

    return match_reg(validuser, addy);
}

/* Strips off leading and trailing whitespace, if any */

void trim(char *instr) 
{
    char *tmpstr;
    unsigned int counter;
    
    /* Find first non-whitespace in the input string. */

    for (counter = 0; counter < strlen(instr); counter++) {
	if (!isspace((int)instr[counter])) break;
    }
    
    if (counter) {		/* If found leading whitespace... */
	/* Copy the part after initial spaces to tmpstr. */
	tmpstr = strdup(instr + counter);
	
	/* Now copy this back to instr, overwriting what was there. */
	strncpy(instr, tmpstr, BIG_BUF - 1);

	free(tmpstr);		/* And free the memory used. */
    }
    
    /* Now find any trailing whitespaces. */

    while (instr[0] && isspace((int)instr[strlen(instr) - 1]))
	instr[strlen(instr) - 1] = 0;
}
