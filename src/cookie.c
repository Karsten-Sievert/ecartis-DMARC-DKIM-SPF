#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#ifndef WIN32
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "config.h"
#include "core.h"
#include "list.h"
#include "user.h"
#include "fileapi.h"
#include "variables.h"
#include "compat.h"
#include "cookie.h"
#include "mystring.h"

struct cookie_data *cookies;
static int gcookiecount;

/* initializes the cookie type storage */
void new_cookies()
{
    cookies = NULL;
    gcookiecount = 0;
}

/* cleans up the cookie type storage */
void nuke_cookies()
{
    struct cookie_data *temp, *temp2;
    temp = cookies;
    while(temp) {
        temp2 = temp->next;
        if(temp->expire) free(temp->expire);
        free(temp);
        temp = temp2;
    }
    cookies = NULL;
}

struct cookie_data *find_cookie_info(char type)
{
   struct cookie_data *temp = cookies;
   while(temp) {
       if(temp->type == type) return temp;
       temp = temp->next;
   }
   return NULL;
}

/* Register a new type of cookie for us to deal with */
void register_cookie(char type, const char *expirevar, CookieFn create,
                     CookieFn destroy)
{
    struct cookie_data *temp = find_cookie_info(type);
    if(temp) {
        log_printf(0, "Reregistration of cookie type '%c'\n", type);
        return;
    }
    temp = (struct cookie_data *)malloc(sizeof(struct cookie_data));
    temp->type = type;
    temp->create = create;
    temp->destroy = destroy;
    temp->expire = NULL;
    if(expirevar)
        temp->expire = strdup(expirevar);
    temp->next = cookies;
    cookies = temp;
}

/* Generates a cookie.  'encstring' is the string to encode,
 * finalstr is the buffer for the resulting cookie.  This is
 * used internally. */
void gen_cookie(const char *encstring, char *finalstr)
{
    char tbuf[SMALL_BUF];
    signed char *tptr;
    const signed char *cptr;
    time_t now;
    int temp;

    time(&now);

    tptr = (signed char *)tbuf;
    cptr = (const signed char *)encstring;

    while(*cptr) {
        if (isalpha((int)*cptr)) {
            *tptr = tolower(*cptr);

            temp = (*tptr - 96) - 13;
            if (temp <= 0)
                temp += 26;
            *tptr = (char)(temp + 96);
            tptr++;
        }
        cptr++;
    }
    *tptr = '\0';

    gcookiecount++;

#ifdef OBSDMOD
    buffer_printf(finalstr, BIG_BUF - 1, "%lX:%X.%X:%s", (long)now, (int)getpid(),
      gcookiecount, tbuf);
#else
#if DEC_UNIX || _AIX
    buffer_printf(finalstr, BIG_BUF - 1, "%X:%X.%X:%s", now, (int)getpid(), gcookiecount, tbuf);
#else
    buffer_printf(finalstr, BIG_BUF - 1, "%lX:%X.%X:%s", now, (int)getpid(), gcookiecount,
     tbuf);
#endif /* DEC_UNIX */
#endif /* OBSDMOD */
}

/* Used internally, checks an plain-text string against the encoded
 * string portion of a given cookie, and returns 1 if matched, 0 if not.
 * Used internally. */
int match_cookie(const char *cookie, const char *match)
{
    char buffer[BIG_BUF];
    const char *tcheck;
    char *tcheck2;

    log_printf(9, "Checking cookies %s and %s\n", cookie, match);

    memset(buffer, 0, sizeof(buffer));

    gen_cookie(match,buffer);

    tcheck = (char *)strrchr(cookie,':');
    tcheck2 = (char *)strrchr(buffer,':');

    if (tcheck && tcheck2) {
        tcheck++;
        tcheck2++;
        if (!strcmp(tcheck,tcheck2))
            return 1;
    }

    return 0;
}

/* Requests a cookie.  Filename is the cookiefile to place it in,
 * cookiedata is the string to tie to the cookie, and cookie is 
 * a buffer to place the cookie into. */
int request_cookie(const char *filename, char *cookie, char type,
                   const char *cookiedata)
{
    FILE *cookiefile;
    const char *cookiefor;
    struct cookie_data *temp = find_cookie_info(type);

    if(!temp) {
        log_printf(0, "Cookie requested for unknown type '%c'\n", type);
        return 0;
    }

    if ((cookiefile = open_file(filename,"a")) == NULL)
        return 0;

    cookiefor = get_var("cookie-for");
    if (!cookiefor)
        cookiefor = get_string("realsender");

    gen_cookie(cookiefor,cookie);

    write_file(cookiefile,"%s : %c%s\n", cookie, type, cookiedata);
    close_file(cookiefile);

    if(temp->create)
        return !(((temp->create)(cookie, type, cookiedata)));

    return 1;
}

/* Verifies a cookie.  Filename is the filename to check for the cookie,
 * cookie is the cookie to check for, and reason is a buffer to place
 * the reason for the cookie into (the cookiedata from above). */
int verify_cookie(const char *filename, const char *cookie, char type,
                  char *data)
{
    FILE *cookiefile;
    char cookiestring[BIG_BUF];
    int foundcookie;
    char tbuf[BIG_BUF];
    struct cookie_data *temp = find_cookie_info(type);

    if(!temp) {
        log_printf(0, "Cookie verification attempted on unknown type '%s'\n",
                   type);
        return 0;
    }

    if (!exists_file(filename)) return 0;

    if ((cookiefile = open_file(filename,"r")) == NULL)
        return 0;

    log_printf(9, "Looking for cookie : %s\n", cookie);

    foundcookie = 0;

    while (read_file(cookiestring, sizeof(cookiestring), cookiefile) && !foundcookie) {
        if (strchr(cookiestring,':')) {
            sscanf(cookiestring, "%s :", &tbuf[0]);
            log_printf(9, "Checking cookie : %s and tbuf : %s\n", cookie, tbuf);
            if (!strcmp(tbuf,cookie)) {
                char *tval;

                foundcookie = 1;
                if (cookiestring[strlen(cookiestring) - 1] == '\n')
                    cookiestring[strlen(cookiestring) - 1] = '\0';
                tval = strchr(cookiestring,':');
                *tval++ = '\0';
                tval = strstr(tval," : ");
                tval = tval + 3;
                if(temp->type != *tval) {
                    log_printf(1, "Wrong cookie type! '%c'!='%c'\n", *tval,
                               temp->type);
                    return 0;
                }
                buffer_printf(data, BIG_BUF - 1, "%s", tval+1);
            }
        }
    }

    close_file(cookiefile);

    return foundcookie;
}

int modify_cookie(const char *filename, const char *cookie, const char *data)
{
    FILE *cookiefile, *newcookiefile;
    char cookiestring[BIG_BUF];
    char tbuf[BIG_BUF];

    if (!exists_file(filename)) return 0;

    if((cookiefile = open_exclusive(filename, "r+")) == NULL)
        return 0;
    buffer_printf(tbuf, sizeof(tbuf) - 1, "%s.cookiechange", get_string("queuefile"));
    if ((newcookiefile = open_file(tbuf,"w+")) == NULL) {
        close_file(cookiefile);
        return 0;
    }

    while (read_file(cookiestring, sizeof(cookiestring), cookiefile)) {
        char *loc = strchr(cookiestring, ':');
        if (!loc)
            continue;
        sscanf(cookiestring, "%s :", &tbuf[0]);
        if (strcmp(tbuf,cookie)) {
            write_file(newcookiefile,"%s",cookiestring);
        } else {
            if(cookiestring[strlen(cookiestring)-1] == '\n')
                cookiestring[strlen(cookiestring)-1] = '\0'; /* smash newline*/
            loc = strstr(loc, " : ");
            loc = loc + 4;
            *loc = '\0';
            if(data[strlen(data)-1] != '\n')
                write_file(newcookiefile, "%s%s\n", cookiestring, data);
            else
                write_file(newcookiefile, "%s%s", cookiestring, data);
        }
    }
    rewind_file(newcookiefile);
    rewind_file(cookiefile);
    truncate_file(cookiefile, 0);
    while(read_file(tbuf, sizeof(tbuf), newcookiefile)) {
        write_file(cookiefile, "%s", tbuf);
    }
    close_file(cookiefile);
    close_file(newcookiefile);

    buffer_printf(tbuf, sizeof(tbuf) - 1, "%s.cookiechange", get_string("queuefile"));
    (void)unlink_file(tbuf);

    return 1;
}

/* matches a cookie given a type and portion of the data field */
/* You *MUST* free this string when you are done with it */
char *find_cookie(const char *filename, char type, const char *partial)
{
    FILE *cookiefile;
    char *res = NULL;
    char cookiestring[BIG_BUF];
    int done = 0;

    if (!exists_file(filename)) return 0;

    if((cookiefile = open_file(filename, "r")) == NULL)
        return NULL;

    while (!done && read_file(cookiestring, sizeof(cookiestring), cookiefile)) {
        char *loc = strstr(cookiestring, " : ");
        if(!loc)
            continue;
        loc+=3;
        if(*loc != type)
            continue;
        loc++;
        if(cookiestring[strlen(cookiestring)-1] == '\n')
            cookiestring[strlen(cookiestring)-1] = '\0'; /* smash newline*/
        if(strstr(loc, partial)) {
            res = strdup(cookiestring);
            done = 1;
        }
    }
    close_file(cookiefile);
    return res;
}


/* Deletes a cookie from a file. */
int del_cookie(const char *filename, const char *cookie)
{
    FILE *cookiefile, *newcookiefile;
    char cookiestring[BIG_BUF];
    char tbuf[BIG_BUF];
    struct cookie_data *temp;
    int res = 1;

    if (!exists_file(filename)) return 0;

    if((cookiefile = open_exclusive(filename, "r+")) == NULL)
        return 0;
    buffer_printf(tbuf, sizeof(tbuf) - 1, "%s.cookiechange", get_string("queuefile"));
    if ((newcookiefile = open_file(tbuf,"w+")) == NULL) {
        close_file(cookiefile);
        return 0;
    }

    while (read_file(cookiestring, sizeof(cookiestring), cookiefile)) {
        char *loc = strchr(cookiestring, ':');
        if (loc) {
            sscanf(cookiestring, "%s :", &tbuf[0]);
            if (strcmp(tbuf,cookie)) {
                write_file(newcookiefile,"%s",cookiestring);
            } else {
                char type;
                if(cookiestring[strlen(cookiestring)-1] == '\n')
                    cookiestring[strlen(cookiestring)-1] = '\0'; /* smash newline*/
                loc = strstr(loc, " : ");
                *loc = '\0';
                loc = loc + 3;
                type = *loc++;
                temp = find_cookie_info(type);
                if(temp && temp->destroy)
                    res = !((temp->destroy)(cookie, type, loc));
            }
        }
    }
    rewind_file(newcookiefile);
    rewind_file(cookiefile);
    truncate_file(cookiefile, 0);
    while(read_file(tbuf, sizeof(tbuf), newcookiefile)) {
        write_file(cookiefile, "%s", tbuf);
    }
    close_file(cookiefile);
    close_file(newcookiefile);

    buffer_printf(tbuf, sizeof(tbuf) - 1, "%s.cookiechange", get_string("queuefile"));
    (void)unlink_file(tbuf);

    return res;
}

/* Expires cookies in a file. */
void expire_cookies(const char *filename)
{
    char buf[BIG_BUF];
    char buf2[BIG_BUF];
    char cookie[BIG_BUF];
#ifdef OBSDMOD
    long cookiedate;
#else
    time_t cookiedate;
#endif
    int age;
    time_t old = time(NULL);
    FILE *oldfile, *newfile;

    if(!exists_file(filename)) return;

    if((oldfile = open_exclusive(filename, "r+")) == NULL)
        return;
    buffer_printf(buf, sizeof(buf) - 1, "%s.cookiechange", get_string("queuefile"));
    if ((newfile = open_exclusive(buf,"w+")) == NULL) {
        close_file(oldfile);
        return;
    }


    while(read_file(buf, sizeof(buf), oldfile)) {
#ifdef OBSDMOD
        long expiretime = 0;
#else
        time_t expiretime = 0;
#endif
        char *tmp;
        char type = ' ';
        int bogus = 1;
        struct cookie_data *temp = NULL;
        /* Now you know the deep, dark secret of why the first chunk of
         * the cookie is a hexadecimal timestamp.  Easier to expire. :) */
#ifdef OBSDMOD
        sscanf(buf, "%lX:%s", &cookiedate, &cookie[0]);
#else
#if DEC_UNIX || _AIX
        sscanf(buf, "%X:%s", &cookiedate, &cookie[0]);
#else
        sscanf(buf, "%lX:%s", &cookiedate, &cookie[0]);
#endif
#endif
        stringcpy(buf2, buf);
        if(buf2[strlen(buf2)-1] == '\n')
            buf2[strlen(buf2)-1] = '\0'; /* smash newline*/
        tmp = strstr(buf2, " : ");
        if(tmp) {
            *tmp = '\0';
            tmp = tmp + 3;
            type = *tmp++;
            temp = find_cookie_info(type);
            /* expire older cookies */
            if(temp) {
               bogus = 0;
               if(temp->expire) {
                   /*
                   * This hack is to handle cookies which carry their
                   * own timeout inside their data
                   */
                   if(strcasecmp(temp->expire, "$$COOKIE$$") == 0) {
#ifdef OBSDMOD
                       sscanf(tmp, "%lX;", &expiretime);
#else
#if DEC_UNIX || _AIX
                       sscanf(tmp, "%X;", &expiretime);
#else
                       sscanf(tmp, "%lX;", &expiretime);
#endif
#endif
                   } else if(get_var(temp->expire)) {
                       age = get_seconds(temp->expire);
                       old -= age;
                   } else {
                       age = get_seconds("cookie-expiration-time");
                       old -= age;
                   }
               } else {
                   old = 0;
               }
            }
        }
        if(bogus)
            continue;
        if((expiretime && (old > expiretime)) ||
           (!expiretime && (cookiedate < old))) {
            if(temp && temp->destroy)
                (temp->destroy)(cookie, type, tmp);
            continue;
        }
        write_file(newfile, "%s", buf);
    }
    rewind_file(newfile);
    rewind_file(oldfile);
    truncate_file(oldfile, 0);
    while(read_file(buf, sizeof(buf), newfile)) {
        write_file(oldfile, "%s", buf);
    }
    close_file(oldfile);
    close_file(newfile);

    buffer_printf(buf, sizeof(buf) - 1, "%s.cookiechange", get_string("queuefile"));
    (void)unlink_file(buf);
}

/* Expires cookies in all lists. */
void expire_all_cookies(void)
{
    char buf[BIG_BUF];
    char tbuf[BIG_BUF];
    int status;

    status = 0;

    buffer_printf(buf, sizeof(buf) - 1, "%s/SITEDATA/cookies", get_string("lists-root"));
    if(exists_file(buf)){
        wipe_vars(VAR_LIST|VAR_TEMP);
        set_var("mode", "nolist", VAR_TEMP);
        expire_cookies(buf);
    }

    if (!get_bool("expire-all-cookies")) return;

    status = walk_lists(&tbuf[0]);
    while(status) {
        if(list_valid(tbuf)) {
            listdir_file(buf, tbuf, "cookies");
            if(exists_file(buf)){
                wipe_vars(VAR_LIST|VAR_TEMP);
                set_var("mode", "nolist", VAR_TEMP);
                set_var("list", tbuf, VAR_TEMP);
                list_read_conf();
                expire_cookies(buf);
            }
        }
        status = next_lists(&tbuf[0]);
    }
    wipe_vars(VAR_LIST|VAR_TEMP);
}
