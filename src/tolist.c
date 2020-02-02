#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tolist.h"
#include "fileapi.h"
#include "variables.h"
#include "user.h"
#include "core.h"
#include "smtp.h"
#include "forms.h"
#include "hooks.h"
#include "list.h"
#include "mystring.h"

#ifndef WIN32
#include <unistd.h>
#endif

/* These are only used in 'normal' (non megalist) mode */
static int elements;
struct tolist *sendtolist;

/* These are used in 'megalist' mode */
struct tolist_flag *tolist_flags;
char *thelist;
FILE *listfile;

/* These are used in lots of places */
struct tolist *current;

void new_tolist(void)
{
    sendtolist = NULL;
    elements = 0;
    tolist_flags = NULL;
    thelist = NULL;
}

void nuke_tolist(void)
{
    struct tolist *tmp;
    struct listinfo *l1, *l2;
    struct tolist_flag *t2;
    while(sendtolist) {
        tmp = sendtolist->next;
        l1 = sendtolist->linfo;
        while(l1) {
            l2 = l1->next;
            if(l1->listname) free(l1->listname);
            if(l1->listflags) free(l1->listflags);
            free(l1);
            l1 = l2;
        }
        if(sendtolist->address) free(sendtolist->address);
        free(sendtolist);
        sendtolist = tmp;
    }
    sendtolist = NULL;
    elements = 0;
    while(tolist_flags) {
        t2 = tolist_flags->next;
        if(tolist_flags->flag) free(tolist_flags->flag);
        free(tolist_flags);
        tolist_flags = t2;
    }
    tolist_flags = NULL;
    if(thelist) free(thelist);
    thelist = NULL;
}

static struct tolist *find_user(const char *address)
{
    struct tolist *cur = sendtolist;
    while(cur) {
        if(strcasecmp(cur->address, address) == 0)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

void add_from_list_all(const char *list)
{
    FILE *fp;
    char buffer[BIG_BUF];
    struct tolist *user;
    struct listinfo *l;
    struct list_user luser;
    char *listdir;

    if(get_bool("megalist")) {
        thelist = strdup(list); 
        return;
    }

    listdir = list_directory(list);

    buffer_printf(buffer, sizeof(buffer) - 1, "%s/users", listdir);

    free(listdir);

    fp = open_file(buffer, "r");
    if(!fp)
        return;

    while(user_read(fp, &luser)) {
        l = (struct listinfo *)malloc(sizeof(struct listinfo));
        l->listname = strdup(list);
        l->listflags = strdup(luser.flags);
        user = find_user(luser.address);
        if(user) {
           l->next = user->linfo;
        } else {
            user = (struct tolist *)malloc(sizeof(struct tolist));
            user->next = sendtolist;
            user->address = strdup(luser.address);
            sendtolist = user;
            elements++;
            l->next = NULL;
        }
        user->linfo = l;
    }

    close_file(fp);
}

void add_from_list_flagged(const char *list, const char *flag)
{
    FILE *fp;
    char buffer[BIG_BUF];
    char tbuf[SMALL_BUF];
    struct tolist *user;
    struct listinfo *l;
    struct list_user luser;
    char *listdir;

    if(get_bool("megalist")) {
        struct tolist_flag *tmp;
        if(thelist && strcmp(thelist, list) != 0)
            return;
        if(!thelist)
            thelist = strdup(list);
        tmp  = (struct tolist_flag *)malloc(sizeof(struct tolist_flag));
        tmp->flag = strdup(flag);
        tmp->add = 1;
        tmp->flagged = 1;
        tmp->next = tolist_flags;
        tolist_flags = tmp;
        return;
    }

    buffer_printf(tbuf, sizeof(tbuf) - 1, "|%s|", flag);

    listdir = list_directory(list);

    buffer_printf(buffer, sizeof(buffer) - 1, "%s/users", listdir);

    free(listdir);

    fp = open_file(buffer, "r");
    if(!fp)
        return;

    while(user_read(fp, &luser)) {
        if(strstr(luser.flags, tbuf) == NULL)
            continue;
        l = (struct listinfo *)malloc(sizeof(struct listinfo));
        l->listname = strdup(list);
        l->listflags = strdup(luser.flags);
        user = find_user(luser.address);
        if(user) {
           l->next = user->linfo;
        } else {
            user = (struct tolist *)malloc(sizeof(struct tolist));
            user->next = sendtolist;
            user->address = strdup(luser.address);
            sendtolist = user;
            elements++;
            l->next = NULL;
        }
        user->linfo = l;
    }

    close_file(fp);
}

void add_from_list_unflagged(const char *list, const char *flag)
{
    FILE *fp;
    char buffer[BIG_BUF];
    char tbuf[SMALL_BUF];
    struct tolist *user;
    struct listinfo *l;
    struct list_user luser;
    char *listdir;

    if(get_bool("megalist")) {
        struct tolist_flag *tmp;
        if(thelist && strcmp(thelist, list) != 0)
            return;
        if(!thelist)
            thelist = strdup(list);
        tmp  = (struct tolist_flag *)malloc(sizeof(struct tolist_flag));
        tmp->flag = strdup(flag);
        tmp->add = 1;
        tmp->flagged = 0;
        tmp->next = tolist_flags;
        tolist_flags = tmp;
        return;
    }

    buffer_printf(tbuf, sizeof(tbuf) - 1, "|%s|", flag);

    listdir = list_directory(list);

    buffer_printf(buffer, sizeof(buffer) - 1, "%s/users", listdir);

    free(listdir);

    fp = open_file(buffer, "r");
    if(!fp)
        return;

    while(user_read(fp, &luser)) {
        if(strstr(luser.flags, tbuf) != NULL)
            continue;
        l = (struct listinfo *)malloc(sizeof(struct listinfo));
        l->listname = strdup(list);
        l->listflags = strdup(luser.flags);
        user = find_user(luser.address);
        if(user) {
           l->next = user->linfo;
        } else {
            user = (struct tolist *)malloc(sizeof(struct tolist));
            user->next = sendtolist;
            user->address = strdup(luser.address);
            sendtolist = user;
            elements++;
            l->next = NULL;
        }
        user->linfo = l;
    }

    close_file(fp);
}

void remove_user_all(const char *address)
{
    struct tolist *prev = NULL;
    struct tolist *cur = sendtolist;
    struct listinfo *l1, *l2;

    while(cur) {
        if(strcasecmp(cur->address, address) == 0)
            break;
        prev = cur;
        cur = cur->next;
    }

    if(!cur)
        return;

    if(prev) {
        prev->next = cur->next;
    } else {
        sendtolist = cur->next;
    }
    elements--;

    l1 = cur->linfo;
    while(l1) {
        l2 = l1->next;
        if(l1->listname) free(l1->listname);
        if(l1->listflags) free(l1->listflags);
        free(l1);
        l1 = l2;
    }
    if(cur->address) free(cur->address);
    free(cur);
}


static int user_is_flagged(struct tolist *user, const char *flag)
{
    char tbuf[SMALL_BUF];
    struct listinfo *l = user->linfo;

    buffer_printf(tbuf, sizeof(tbuf) - 1, "|%s|", flag);
    while(l) {
         if(strstr(l->listflags, tbuf) != NULL)
             return 1;
         l = l->next;
    }
    return 0;
}

static int user_is_flagged_prilist(struct tolist *user, const char *flag, 
    const char *list)
{
    char tbuf[SMALL_BUF];
    struct listinfo *l = user->linfo;
    int foundit = 0;

    buffer_printf(tbuf, sizeof(tbuf) - 1, "|%s|", flag);
    while(l) {
         if(strcasecmp(l->listname, list) == 0) {
            if(strstr(l->listflags, tbuf) != NULL)
                return 1;
            else
                return 0;
         } else {
            if(strstr(l->listflags, tbuf) != NULL)
                foundit = 1;
         }
         l = l->next;
    }
    return foundit;
}

void remove_flagged_all_prilist(const char *flag, const char *prilist)
{
    struct tolist *cur, *prev, *next;
    struct listinfo *l1, *l2;

    if(get_bool("megalist")) {
        struct tolist_flag *tmp;
        if(thelist && strcmp(thelist, prilist) != 0)
            return;
        if(!thelist)
            thelist = strdup(prilist);
        tmp  = (struct tolist_flag *)malloc(sizeof(struct tolist_flag));
        tmp->flag = strdup(flag);
        tmp->add = 0;
        tmp->flagged = 1;
        tmp->next = tolist_flags;
        tolist_flags = tmp;
        return;
    }

    cur = sendtolist;
    prev = NULL;
    while(cur) {
        next = cur->next;

        if(user_is_flagged_prilist(cur, flag, prilist)) {
            elements--;
            if(prev) {
               prev->next = next;
            } else {
               sendtolist = next;
            }
            l1 = cur->linfo;
            while(l1) {
                l2 = l1->next;
                if(l1->listname) free(l1->listname);
                if(l1->listflags) free(l1->listflags);
                free(l1);
                l1 = l2;
            }
            if(cur->address) free(cur->address);
            free(cur);
        } else {
            prev = cur;
        }
        cur = next;
    }
}

void remove_flagged_all(const char *flag)
{
    struct tolist *cur, *prev, *next;
    struct listinfo *l1, *l2;

    if(get_bool("megalist")) {
        struct tolist_flag *tmp;
        tmp  = (struct tolist_flag *)malloc(sizeof(struct tolist_flag));
        tmp->flag = strdup(flag);
        tmp->add = 0;
        tmp->flagged = 1;
        tmp->next = tolist_flags;
        tolist_flags = tmp;
        return;
    }

    cur = sendtolist;
    prev = NULL;
    while(cur) {
        next = cur->next;

        if(user_is_flagged(cur, flag)) {
            elements--;
            if(prev) {
               prev->next = next;
            } else {
               sendtolist = next;
            }
            l1 = cur->linfo;
            while(l1) {
                l2 = l1->next;
                if(l1->listname) free(l1->listname);
                if(l1->listflags) free(l1->listflags);
                free(l1);
                l1 = l2;
            }
            if(cur->address) free(cur->address);
            free(cur);
        } else {
            prev = cur;
        }
        cur = next;
    }
}

void remove_unflagged_all(const char *flag)
{
    struct tolist *cur, *prev, *next;
    struct listinfo *l1, *l2;

    if(get_bool("megalist")) {
        struct tolist_flag *tmp;
        tmp  = (struct tolist_flag *)malloc(sizeof(struct tolist_flag));
        tmp->flag = strdup(flag);
        tmp->add = 0;
        tmp->flagged = 0;
        tmp->next = tolist_flags;
        tolist_flags = tmp;
        return;
    }

    cur = sendtolist;
    prev = NULL;
    while(cur) {
        next = cur->next;

        if(!user_is_flagged(cur, flag)) {
            elements--;
            if(prev) {
               prev->next = next;
            } else {
               sendtolist = next;
            }
            l1 = cur->linfo;
            while(l1) {
                l2 = l1->next;
                if(l1->listname) free(l1->listname);
                if(l1->listflags) free(l1->listflags);
                free(l1);
                l1 = l2;
            }
            if(cur->address) free(cur->address);
            free(cur);
        } else {
            prev = cur;
        }
        cur = next;
    }
}

void remove_unflagged_all_prilist(const char *flag, const char *prilist)
{
    struct tolist *cur, *prev, *next;
    struct listinfo *l1, *l2;

    if(get_bool("megalist")) {
        struct tolist_flag *tmp;
        if(thelist && strcmp(thelist, prilist) != 0)
            return;
        if(!thelist)
            thelist = strdup(prilist);
        tmp  = (struct tolist_flag *)malloc(sizeof(struct tolist_flag));
        tmp->flag = strdup(flag);
        tmp->add = 0;
        tmp->flagged = 0;
        tmp->next = tolist_flags;
        tolist_flags = tmp;
        return;
    }

    cur = sendtolist;
    prev = NULL;
    while(cur) {
        next = cur->next;

        if(!user_is_flagged_prilist(cur, flag, prilist)) {
            elements--;
            if(prev) {
               prev->next = next;
            } else {
               sendtolist = next;
            }
            l1 = cur->linfo;
            while(l1) {
                l2 = l1->next;
                if(l1->listname) free(l1->listname);
                if(l1->listflags) free(l1->listflags);
                free(l1);
                l1 = l2;
            }
            if(cur->address) free(cur->address);
            free(cur);
        } else {
            prev = cur;
        }
        cur = next;
    }
}

void remove_list_flagged(const char *list, const char *flag)
{
    struct tolist *cur, *prev, *next;
    struct listinfo *curl, *prevl, *nextl;
    char tbuf[SMALL_BUF];
    buffer_printf(tbuf, sizeof(tbuf) - 1, "|%s|", flag);

    if(get_bool("megalist")) {
        struct tolist_flag *tmp;
        if(thelist && strcmp(thelist, list) != 0)
            return;
        if(!thelist)
            thelist = strdup(list);
        tmp  = (struct tolist_flag *)malloc(sizeof(struct tolist_flag));
        tmp->flag = strdup(flag);
        tmp->add = 0;
        tmp->flagged = 1;
        tmp->next = tolist_flags;
        tolist_flags = tmp;
        return;
    }

    cur = sendtolist;
    prev = NULL;
    while(cur) {
        next = cur->next;

        /* Remove the list from the user if the list doesn't have the flag */
        curl = cur->linfo;
        prevl = NULL;
        while(curl) {
            nextl = curl->next;
            if((strcasecmp(curl->listname, list) == 0) &&
               (strstr(curl->listflags, tbuf) != NULL)) {
               if(prevl) {
                   prevl->next = curl->next;
               } else {
                   cur->linfo = curl->next;
               }
               if(curl->listname) free(curl->listname);
               if(curl->listflags) free(curl->listflags);
               free(curl);
            } else {
               prevl = curl;
            }
            curl = nextl;
        }

        /* If the user is on no more lists, remove him */
        if(cur->linfo == NULL) {
            elements--;
            if(prev) {
               prev->next = next;
            } else {
               sendtolist = next;
            }
            if(cur->address) free(cur->address);
            free(cur);
        } else {
            prev = cur;
        }
        cur = next;
    }
}

void remove_list_unflagged(const char *list, const char *flag)
{
    struct tolist *cur, *prev, *next;
    struct listinfo *curl, *prevl, *nextl;
    char tbuf[SMALL_BUF];
    buffer_printf(tbuf, sizeof(tbuf) - 1, "|%s|", flag);

    if(get_bool("megalist")) {
        struct tolist_flag *tmp;
        if(thelist && strcmp(thelist, list) != 0)
            return;
        if(!thelist)
            thelist = strdup(list);
        tmp  = (struct tolist_flag *)malloc(sizeof(struct tolist_flag));
        tmp->flag = strdup(flag);
        tmp->add = 0;
        tmp->flagged = 0;
        tmp->next = tolist_flags;
        tolist_flags = tmp;
        return;
    }

    cur = sendtolist;
    prev = NULL;
    while(cur) {
        next = cur->next;

        /* Remove the list from the user if the list doesn't have the flag */
        curl = cur->linfo;
        prevl = NULL;
        while(curl) {
            nextl = curl->next;
            if((strcasecmp(curl->listname, list) == 0) &&
               (strstr(curl->listflags, tbuf) == NULL)) {
               if(prevl) {
                   prevl->next = curl->next;
               } else {
                   cur->linfo = curl->next;
               }
               if(curl->listname) free(curl->listname);
               if(curl->listflags) free(curl->listflags);
               free(curl);
            } else {
               prevl = curl;
            }
            curl = nextl;
        }

        /* If the user is on no more lists, remove him */
        if(cur->linfo == NULL) {
            elements--;
            if(prev) {
               prev->next = next;
            } else {
               sendtolist = next;
            }
            if(cur->address) free(cur->address);
            free(cur);
        } else {
            prev = cur;
        }
        cur = next;
    }
}

static int tolist_cmp(const void *e1, const void *e2)
{
    struct tolist *t1, *t2;
    int admin1, admin2;
    int result, done;
    char *tptr1, *tptr2;
    char *cptr1, *cptr2;
    char *cbuf1, *cbuf2;
    char *tcptr1, *tcptr2;
    int comparedat;
    int countback;

    t1 = *(struct tolist **)e1;
    t2 = *(struct tolist **)e2;

    admin1 = user_is_flagged(t1, "ADMIN");
    admin2 = user_is_flagged(t2, "ADMIN");

    if (admin1 != admin2) {
       if (admin1) return -1;
       if (admin2) return 1;
    }

    cbuf1 = strdup(t1->address);
    cbuf2 = strdup(t2->address);

    cptr1 = strrchr(cbuf1,'.');
    cptr2 = strrchr(cbuf2,'.');

    tptr1 = strchr(cbuf1,'@');
    tptr2 = strchr(cbuf2,'@');

    /* If either address has no dots, we'll just do a raw compare. */
    if (!cptr1 || !cptr2) {
       return strcasecmp(t1->address,t2->address);
    }

    /* Quick hack for local addresses with no hostname */
    /* We technically don't allow these, but better to handle them
       than to choke and die */
    if (!tptr1 && !tptr2) 
    {
       free(cbuf1);
       free(cbuf2);
  
       return strcasecmp(t1->address,t2->address);
    }
    if (!tptr1)
    {
       free(cbuf1);
       free(cbuf2);
  
       return -1;
    }
    if (!tptr2)
    {
       free(cbuf1);
       free(cbuf2);
  
       return 1;
    }

    if (!strcasecmp(tptr1,tptr2)) {
       /* Same host, just sort on user */
       free(cbuf1);
       free(cbuf2);
       return (strcasecmp(t1->address,t2->address));
    }

    done = 0; comparedat = 0;
    countback = 1;
    result = 0;

    cptr1 = cbuf1 + strlen(cbuf1);
    cptr2 = cbuf2 + strlen(cbuf2);

    while(done ? 0 : !strcasecmp(cptr1,cptr2)) {
       char tchar1, tchar2;

       tcptr1 = cptr1; tcptr2 = cptr2;

       tchar1 = *cptr1;
       tchar2 = *cptr2;

       *cptr1 = 0;       
       *cptr2 = 0;

       cptr1 = strrchr(cbuf1,'.');
       cptr2 = strrchr(cbuf2,'.');
/*
       *tcptr1 = tchar1;
       *tcptr2 = tchar2;
 */
       if (!cptr1 || !cptr2)
       {
          if (!comparedat)
          {
             comparedat = 1;

             if (!cptr1)
                cptr1 = tptr1 + 1;
             else
                cptr1++;

             if (!cptr2)
                cptr2 = tptr2 + 1;
             else
                cptr2++;
          } 
          else
          {
             /* Same host, different users */
             if (!cptr1 && !cptr2) {
                done = 1;
                cptr1 = cbuf1;
                cptr2 = cbuf2;
             } else 
             {
                if (!cptr1)
                  result = -1;

                if (!cptr2)
                  result = 1; 

                done = 1;
             }
          }
       }
    }

    if (!result)
       result = strcasecmp(cptr1, cptr2);

    free(cbuf1);
    free(cbuf2);

    return result;
}   

void sort_tolist(void)
{
    struct tolist *tmp;
    struct tolist **arr;
    int i;

    /*
     * there are a couple of cases where it's not helpful to do
     * tolist sorting.
     * In megalist mode, all the users are being read off the disk, so there
     * is no tolist to sort.
     * In per-user-modifications mode or smtp-queue-chunk = 1 then
     * we are going to be sending each message to the SMTP server in a
     * single envelope anyway, so the order of the tolist is irrelevant
     */
    if(get_bool("megalist") || get_bool("per-user-modifications") ||
       (get_number("smtp-queue-chunk") == 1))
        return;

    if (!elements) return;

    arr = (struct tolist **)malloc(sizeof(struct tolist *)*elements);
    if(!arr) return;

    tmp = sendtolist;
    i = 0;
    while(tmp) {
        arr[i++] = tmp;
        tmp = tmp->next;
    }

    qsort(arr, elements, sizeof(struct tolist *), tolist_cmp);

    sendtolist = arr[0];
    tmp = sendtolist;
    for(i = 1; i < elements; i++) {
        tmp->next = arr[i];
        tmp = arr[i];
    }
    tmp->next = NULL;

    free(arr);
}

static void rev_tolist_flags(struct tolist_flag * list)
{
    struct tolist_flag *ptr = list->next;
    if(ptr) {
        rev_tolist_flags(ptr);
        ptr->next = list;
    } else {
        tolist_flags = list;
    }
    list->next = NULL;
}

int tolist_user_matches_flags(struct list_user *user)
{
    struct tolist_flag *tmp = tolist_flags;
    int result = 1;
    int has;

    log_printf(15, "entering TOLIST_USER_MATCHES_FLAG, (%s -> %s)\n", user->address, user->flags);
    while(tmp) {
        log_printf(15, "Checking %s (add: %d) (has: %d)\n", tmp->flag, tmp->add, tmp->flagged);
        has = user_hasflag(user, tmp->flag);
        if((tmp->flagged && has) || (!tmp->flagged && !has)) {
            if(tmp->add && !result)
                 result = 1;
            else if (!tmp->add && result)
                 result = 0;
        }
        tmp = tmp->next;
    }
    log_printf(15, "exiting TOLIST_USER_MATCHES_FLAG (%d)\n", result);
    return result;
}

struct tolist *start_tolist()
{
    log_printf(15, "entering START_TOLIST\n");
    if(get_bool("megalist")) {
        char buf[BIG_BUF];
        struct list_user touser;
        char *listdir;

        log_printf(15, "START_TOLIST (megalist)\n");
        if(tolist_flags)
            rev_tolist_flags(tolist_flags);

        listdir = list_directory(thelist);

        buffer_printf(buf, sizeof(buf) - 1, "%s/users", listdir); 

        free(listdir);

        listfile = open_file(buf, "r");
        if(listfile == NULL) {
            return NULL;
        }
        log_printf(15, "START_TOLIST (megalist) Opened %s\n", thelist);
        while(user_read(listfile, &touser)) {
            if(tolist_user_matches_flags(&touser)) {
                current = (struct tolist *)malloc(sizeof(struct tolist));
                current->linfo = (struct listinfo *)malloc(sizeof(struct listinfo));
                current->next = NULL;
                current->linfo->next = NULL;
                current->address = strdup(touser.address);
                current->linfo->listname = strdup(thelist);
                current->linfo->listflags = strdup(touser.flags);
                log_printf(15, "START_TOLIST (megalist) returning %s\n", current->address);
                return current;
            }
        }
        log_printf(15, "START_TOLIST (megalist) done\n");
        current = NULL;
    } else {
        current = sendtolist;
        if(current) {
            log_printf(15, "START_TOLIST (not megalist), returning %s\n", current->address);
        } else {
            log_printf(15, "START_TOLIST (not megalist), tolist is empty\n");
        }
    }
    return current;
}

struct tolist *next_tolist()
{
    log_printf(15, "entering NEXT_TOLIST\n");
    if(get_bool("megalist")) {
        struct list_user touser;

        log_printf(15, "NEXT_TOLIST (megalist)\n");
        if(listfile == NULL)
            return NULL;

        while(user_read(listfile, &touser)) {
            if(tolist_user_matches_flags(&touser)) {
                current->next = NULL;
                current->linfo->next = NULL;
                if(current->address) free(current->address);
                if(current->linfo->listflags) free(current->linfo->listflags);
                current->address = strdup(touser.address);
                current->linfo->listflags = strdup(touser.flags);
                log_printf(15, "NEXT_TOLIST (megalist) returning %s\n", current->address);
                return current;
            }
        }
        log_printf(15, "NEXT_TOLIST (megalist) done\n");
        current = NULL;
    } else {
        current = current->next;
        if(current)
            log_printf(15, "NEXT_TOLIST (not megalist), returning %s\n", current->address);
        else 
            log_printf(15, "NEXT_TOLIST (not megalist) done\n");
    }
    return current;
}

void finish_tolist()
{
    struct listinfo *l1, *l2;
    log_printf(15, "entering FINISH_TOLIST\n");
    if(get_bool("megalist")) {
        if(current) {
            l1 = current->linfo;
            while(l1) {
                l2 = l1->next;
                if(l1->listname) free(l1->listname);
                if(l1->listflags) free(l1->listflags);
                free(l1);
                l1 = l2;
            }
            if(current->address) free(current->address);
            free(current);
        }
        if(listfile)
            close_file(listfile);
    }
    current = NULL;
}

int send_to_tolist(const char *fromaddy, const char *file,
                   int do_ackpost, int do_echopost, int fullbounce)
{
    int chunksize = get_number("smtp-queue-chunk");
    int count;
    struct tolist *ttolist, *otolist = NULL;
    int errors = 0;
    int total_errors = 0;
    int sentusers = 0;
    int total_sent = 0;
    int val = 0;
    FILE *errfile;
    FILE *infile = NULL;
    char buffer[BIG_BUF];
    char userfilepath[BIG_BUF];
    struct list_user user;
    int founduser=0;
    int peruser = get_bool("per-user-modifications");
    const char *list = get_var("list");

    if(chunksize == 0) count = -1;
    else count = chunksize;

    if(peruser && chunksize != 1) {
        log_printf(1, "Queue chunk != 1, disabling per user queue modificaton.\n");
        peruser = 0;
    }

    if(!exists_file(file))
        return 0;

    if(peruser) {
        FILE *copyfile = NULL;
 
        /* Set up our queuefile and variables */
        buffer_printf(buffer, sizeof(buffer) - 1 , "%s.user", get_string("queuefile"));
        set_var("per-user-queuefile", buffer, VAR_TEMP);
        set_var("per-user-datafile", file, VAR_TEMP);

        if ((infile = open_file(file, "r")) == NULL) {
           filesys_error(file);
           return 0;
        }

        if ((copyfile = open_file(buffer, "w")) == NULL) {
           close_file(infile);
           filesys_error(buffer);
           return 0;
        }

        while(read_file(buffer, sizeof(buffer), infile)) {
           write_file(copyfile, "%s", buffer);
        }

        close_file(infile);
        close_file(copyfile);

        infile = NULL;
    }

    if((do_ackpost || do_echopost) && list) {
        char *listdir;

        listdir = list_directory(list);
        buffer_printf(userfilepath, sizeof(userfilepath) - 1, "%s/users", listdir);
        free(listdir);
        founduser = user_find_list(list, get_string("fromaddress"), &user);
    }

    if(!founduser) {
        strncpy(user.address, "",  BIG_BUF - 1);
        strncpy(user.flags, "", HUGE_BUF - 1);
    }

    if(do_ackpost && list) {
        if(founduser && user_hasflag(&user, "ACKPOST")) {
            char tbuf[SMALL_BUF];
            buffer_printf(tbuf, sizeof(tbuf) - 1, "Post received for list '%s'", list);
            set_var("task-form-subject",&tbuf[0],VAR_TEMP);
            if(task_heading(user.address)) {
                if (get_var("message-subject")) {
                    smtp_body_line("Your post entitled:");
                    smtp_body_text("    ");
                    smtp_body_line(get_string("message-subject"));
                } else {
                    smtp_body_text("Your post ");
                }
                smtp_body_text("was received for list '");
                smtp_body_text(get_string("list"));
                smtp_body_line("'.");
                task_ending();
            }
        }
    }

    ttolist = start_tolist();
    buffer_printf(buffer, sizeof(buffer) - 1, "%s.serr", get_string("queuefile"));
    set_var("smtp-errors-file", buffer, VAR_GLOBAL);
    errfile = open_file(buffer, "w");

    if(!peruser) {
        infile = open_file(file, "r");
        if(!infile)
            return 0;
    }

    while(ttolist) {

        sentusers = 0;
        errors = 0;
        if(!smtp_start(fullbounce))
            return 0;

        if(!smtp_from(fromaddy))
            return 0;
        while(ttolist && ((count == -1) || (count > 0))) {
            if(!do_echopost ||
                (user_hasflag(&user, "ECHOPOST") ||
                 strcasecmp(user.address, ttolist->address))) {
                if(!smtp_to(ttolist->address)) {
                    if(errfile)
                        write_file(errfile, "%s\n",
                                   get_string("smtp-last-error"));
                    errors++;
                    total_errors++;
                }
                sentusers++;
                total_sent++;
                if(count != -1) count--;
            }
            otolist = ttolist;
            ttolist = next_tolist();
        }

        if(peruser) {
            int res, done;
            char *peruserlist = NULL;
            struct listinfo *li = NULL;

            if(otolist != NULL) {
                set_var("per-user-data", (char *)otolist, VAR_TEMP);
                set_var("per-user-address", otolist->address, VAR_TEMP);
                li = otolist->linfo;
            }

            done = 0;

            while(li && !done) {
               peruserlist = li->listname;
               if (list && (strcmp(peruserlist,list) == 0))
                  done = 1;
               li = li->next;
            }

            set_var("per-user-list", peruserlist, VAR_TEMP);     

            res = do_hooks("PER-USER");
            if(res == HOOK_RESULT_FAIL) {
                unlink_file(buffer);
                return 0;
            } else if(res == HOOK_RESULT_STOP) {
                ttolist = ttolist->next;
                continue;
            } 
            infile = open_file(get_string("per-user-queuefile"), "r");
            if(!infile)
                return 0;
        }

        if(sentusers && (sentusers != errors)) {
           int line = 0;
           if(!smtp_body_start())
               return 0;
           while(read_file(buffer, sizeof(buffer), infile)) {
               if(line != 0 || strncmp(buffer, "From ", 5) != 0) 
                   smtp_body_text(buffer);
               line++;
           }
           smtp_body_end();
        }
        smtp_end();

        if(count != -1) count = chunksize;
        if(peruser) {
            FILE *copyfile;

            close_file(infile);
            unlink_file(get_var("per-user-queuefile"));

            if ((infile = open_file(file, "r")) == NULL) {
               filesys_error(file);
               return 0;
            }

            if ((copyfile = open_file(get_var("per-user-queuefile"), "w")) == NULL) {
               close_file(infile);
               filesys_error(get_var("per-user-queuefile"));
               return 0;
            }

            while(read_file(buffer, sizeof(buffer), infile)) {
               write_file(copyfile, "%s", buffer);
            }

            close_file(infile);
            close_file(copyfile);

            infile = NULL;

        } else {
            rewind_file(infile);
        }

        /* Now pause a bit if we are requested to */
        val = get_number("tolist-send-pause");
        if(ttolist && val) {
           do_sleep(val);
        }
    }

    finish_tolist();

    clean_var("per-user-queuefile", VAR_TEMP);
    clean_var("per-user-datafile", VAR_TEMP);
    clean_var("per-user-data", VAR_TEMP);

    close_file(infile);

    if(total_errors && total_sent && errfile) {
        close_file(errfile);
        bounce_message();
    } else {
        if(errfile)
            close_file(errfile);
    }
    buffer_printf(buffer, sizeof(buffer) - 1, "%s.serr", get_string("queuefile"));
    unlink_file(buffer);

    if(peruser) {
        buffer_printf(buffer, sizeof(buffer) - 1, "%s.user", get_string("queuefile"));
        unlink_file(buffer);
    }

    if(total_errors && total_errors == total_sent)
        return 0;
    return 1;
}
 
