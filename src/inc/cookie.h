#ifndef _COOKIE_H
#define _COOKIE_H

#define COOKIE_PARAMS const char *cookie, char cookietype, const char *cookiedata

typedef int (*CookieFn)(COOKIE_PARAMS);

#define COOKIE_HANDLE_OK 0
#define COOKIE_HANDLE_FAIL 1

#define COOKIE_HANDLER(x) int x(COOKIE_PARAMS)

/* API calls */
extern void register_cookie(char type, const char *expirevar, CookieFn create,
                            CookieFn destroy);
extern int match_cookie(const char *cookie, const char *match);
extern int request_cookie(const char *filename, char *cookie,
                          char cookietype, const char *cookiedata);
extern int verify_cookie(const char *filename, const char *cookie,
                         char cookietype, char *cookiedata);
extern int del_cookie(const char *filename, const char *cookie);
extern void expire_cookies(const char *filename);
extern void expire_all_cookies(void);
extern char *find_cookie(const char *filename, char type,
                               const char *partial);
extern int modify_cookie(const char *filename, const char *cookie,
                         const char *newdata);

/* Non exported bits */
struct cookie_data {
    char type;
    CookieFn create;
    CookieFn destroy;
    char *expire;
    struct cookie_data *next;
};

extern struct cookie_data *cookies;

extern void new_cookies(void);
extern void nuke_cookies(void);

/* utility function */
struct cookie_data *find_cookie_info(char type);


#endif /* _COOKIE_H */
