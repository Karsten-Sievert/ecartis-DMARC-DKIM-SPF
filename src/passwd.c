#include "config.h"
#include "fileapi.h"
#include "passwd.h"
#include "variables.h"
#include "mystring.h"
#include "internal.h"
#include "core.h"
#include <string.h>

#ifndef UNCRYPTED_PASS
/* Buuuuusted glibc behavior.  Why can't I just include crypt without
   getting linker errors anymore under glibc 2.1?? */
#define __USE_XOPEN
#include <unistd.h>

# ifdef SUNOS_5
# include <crypt.h>
# endif
#endif

void set_pass(const char *user, const char *newpass)
{
    FILE *pwdfile, *newfile;
    char buf[BIG_BUF];
    char passbuf[BIG_BUF];

#ifndef UNCRYPTED_PASS
    buffer_printf(passbuf, sizeof(passbuf) - 1, "^ %s", crypt(newpass, "SP"));
#else
    buffer_printf(passbuf, sizeof(passbuf) - 1, "%s", newpass);
#endif

    log_printf(9,"Trying to write to password file '%s'\n",
       get_string("pwdfile"));

    pwdfile = open_file(get_string("pwdfile"), "r+");
    if(!pwdfile) {
        pwdfile = open_file(get_string("pwdfile"), "a");
        if(pwdfile) {
            write_file(pwdfile, "%s : %s\n", user, passbuf);
        } else {
            filesys_error(get_string("pwdfile"));
        }
    } else {
        buffer_printf(buf, sizeof(buf) - 1, "%s.pwd", get_string("queuefile"));
        newfile = open_file(buf, "w+");
        if(!newfile) {
            close_file(pwdfile);
            filesys_error(buf);
        } else {
            int gotme = 0;

            while(read_file(buf, sizeof(buf), pwdfile)) {
                char *u1, *p1;
                if(buf[0] == '#') {
                    write_file(newfile, "%s", buf);
                } else if((p1 = strstr(buf, " : ")) != NULL) {
                    u1 = buf;
                    *p1 = '\0';
                    p1+=3;
                    if(strcasecmp(u1, user) == 0) {
                        write_file(newfile, "%s : %s\n", u1, passbuf);
                        gotme = 1;
                    } else {
                        write_file(newfile, "%s : %s", u1, p1);
                    }
                }
            }
            if (!gotme) {
                write_file(newfile,"%s : %s\n", user, passbuf);
            }
            rewind_file(newfile);
            rewind_file(pwdfile);
            truncate_file(pwdfile, 0);
            while(read_file(buf, sizeof(buf), newfile)) {
                write_file(pwdfile, "%s", buf);
            }
            close_file(newfile);
            close_file(pwdfile);
            buffer_printf(buf, sizeof(buf) - 1, "%s.pwd", get_string("queuefile"));
            unlink_file(buf);
        }
    }
    return;
}

int find_pass(const char *user)
{
    FILE *pwdfile;
    char buf[BIG_BUF];

    pwdfile = open_file(get_string("pwdfile"), "r");
    if(pwdfile) {
        while(read_file(buf, sizeof(buf), pwdfile)) {
            char *p;
            if(buf[0] == '#') {
                continue;
            } else if((p = strstr(buf, " : ")) != NULL) {
                *p = '\0';
                if(strcasecmp(user, buf) == 0) {
                    close_file(pwdfile);
                    return 1;
                }
            }
        }
        close_file(pwdfile);
    }
    return 0;
}

int check_pass(const char *user, const char *pass)
{
    FILE *pwdfile;
    char buf[BIG_BUF];

    pwdfile = open_file(get_string("pwdfile"), "r");
    if(pwdfile) {
        while(read_file(buf, sizeof(buf), pwdfile)) {
            char *p;
            if(buf[0] == '#') {
                continue;
            } else if((p = strstr(buf, " : ")) != NULL) {
                *p = '\0';
                if(strcasecmp(user, buf) == 0) {
                    close_file(pwdfile);
                    p += 3;
                    if(p[strlen(p)-1] == '\n')
                        p[strlen(p)-1] = '\0';  /* trounce the newline */

#ifndef UNCRYPTED_PASS
                    /* Check if we are a crypted password */
                    if(strncmp(p,"^ ",2) == 0) {
                       char *pt, pbuffer[BIG_BUF];

                       pt = p + 2;
                       buffer_printf(pbuffer, sizeof(pbuffer) - 1, "%s",
                         crypt(pass,"SP"));

                       if(strcmp(pt,pbuffer) == 0) {
                           return 1;
                       }
                    } else 
#endif
                    {
                       if(strcmp(pass, p) == 0) {
                           return 1;
                       }
                    }
                    return 0;
                }
            }
        }
        close_file(pwdfile);
    }
    return 0;
}

