#ifndef _LCGI_H
#define _LCGI_H

#define CGI_HOOK_PARAMS const char *param
#define CGI_MODE_PARAMS void

typedef int (*CgiHook)(CGI_HOOK_PARAMS);
typedef void (*CgiMode)(CGI_MODE_PARAMS);

#define CGI_HANDLER(x) int x(CGI_HOOK_PARAMS)
#define CGI_MODE(x) void x(CGI_MODE_PARAMS)

struct listserver_cgi_hook {
   char *name;
   CgiHook hook;
   struct listserver_cgi_hook *next;   
};

struct listserver_cgi_mode {
   char *name;
   CgiMode mode;
   struct listserver_cgi_mode *next;
};

struct listserver_cgi_tempvar {
   char *name;
   char *value;
   struct listserver_cgi_tempvar *next;
};

#define CGI_UNPARSE_NORMAL	0
#define CGI_UNPARSE_FIRSTHASH	1
#define CGI_UNPARSE_GETVAR	2
#define CGI_UNPARSE_GETPARM     3
#define CGI_UNPARSE_EATHASH	4

extern void new_cgi_hooks(void);
extern void nuke_cgi_hooks(void);
extern void add_cgi_hook(const char *name, CgiHook function);
extern struct listserver_cgi_hook *get_cgi_hooks(void);
extern struct listserver_cgi_hook *find_cgi_hook(const char *name);

extern void new_cgi_modes(void);
extern void nuke_cgi_modes(void);
extern void add_cgi_mode(const char *name, CgiMode function);
extern struct listserver_cgi_mode *find_cgi_mode(const char *name);

extern void new_cgi_tempvars(void);
extern void nuke_cgi_tempvars(void);
extern void add_cgi_tempvar(const char *name, const char *value);
extern struct listserver_cgi_tempvar* get_cgi_tempvars(void);
extern struct listserver_cgi_tempvar* find_cgi_tempvar(const char *name);

extern int cgi_unparse_template(const char *name);


#endif
