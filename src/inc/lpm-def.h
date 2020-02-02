#ifndef LPM_DEF_H
#define LPM_DEF_H

/* From alias.h */

#define ALIASHASH 64

struct alias_data {
    char *aliasname;
    char *realname;
    struct alias_data *next;
};

struct alias_info {
    struct alias_data *bucket[ALIASHASH];
};

/* From CmdArg.h */

#define MAX_CMDARG_LEN 64

typedef int (*CmdArgFn)(char **argv, int count);

struct listserver_cmdarg {
   char *arg;
   CmdArgFn fn;
   int params;
   char *parmdesc;
   struct listserver_cmdarg *next;
};

#define CMDARG_HANDLER(a) int a(char **argv, int count)
#define CMDARG_ERR 0
#define CMDARG_OK 1
#define CMDARG_EXIT 2

/* From Command.h */

#define MAX_CMD_LEN	64
#define MAX_DESC_LEN	BIG_BUF

/* Command flags */
#define CMD_BODY	0x00000001
#define CMD_HEADER	0x00000002
#define CMD_ADMIN	0x00000004

/* Result codes */
#define CMD_RESULT_CONTINUE	0
#define CMD_RESULT_END	-1

#define MAX_PARAMS 10
struct cmd_params
{
   int num;
   char *words[MAX_PARAMS];
};

/* Parameter format */
#define CMD_PARAMS	struct cmd_params *params

typedef int (*CmdFn)(CMD_PARAMS);

struct listserver_cmd
{
   char *name;
   char *desc;
   char *syntax;
   char *altsyntax;
   char *adminsyntax;
   int flags;
   CmdFn cmd;
   struct listserver_cmd *next;
};

#define CMD_HANDLER(a) int a(CMD_PARAMS)

/* From cookie.h */
#define COOKIE_PARAMS const char *cookie, char cookietype, const char *cookiedata

typedef int (*CookieFn)(COOKIE_PARAMS);

#define COOKIE_HANDLE_OK 0
#define COOKIE_HANDLE_FAIL 1

#define COOKIE_HANDLER(x) int x(COOKIE_PARAMS)


/* From File.h */

#define FILE_NAME_LEN 64
#define FILE_VAR_LEN 32

#define FILE_NOWEBEDIT		0x01
#define FILE_NOMAILEDIT		0x02

struct list_file {
   char *name;
   char *varname;
   char *desc;
   int  flags;
   struct list_file *next;
};

/* From Fileapi.h */

#ifndef WIN32
# include <sys/types.h>
# include <dirent.h>
# define LDIR DIR*
#else
# define LDIR long
#endif

/* From Flag.h */

#define MAX_FLAG_NAME 32

struct listserver_flag {
   char *name;
   char *desc;
   int  admin;
   struct listserver_flag *next;
};

#define ADMIN_UNSAFE		0x01
#define ADMIN_SAFESET		0x02
#define ADMIN_SAFEUNSET		0x04
#define ADMIN_UNSETTABLE	0x08

/* From Hooks.h */

#define MAX_HOOK_LEN		64

/* Result codes */
#define HOOK_RESULT_OK		0
#define HOOK_RESULT_FAIL	-1
#define HOOK_RESULT_STOP	-2

typedef int (*HookFn)(void);

struct listserver_hook {
   char *type;
   unsigned int priority;
   HookFn hook;
   struct listserver_hook *next;
};

#define HOOK_HANDLER(a) int a(void)

/* From Modes.h */

#define MAX_MODE_LEN 64

typedef int (*ModeFn)(void);

struct listserver_mode {
   char *modename;
   ModeFn modefn;
   struct listserver_mode *next;
};

#define MODE_HANDLER(a) int a(void)

#define MODE_ERR 0
#define MODE_OK  1
#define MODE_END 2

/* From Module.h */

struct listserver_module {
   char *name;
   char *desc;
   char advert;
   struct listserver_module *next;
};

/* From Regexp.h */

#define NSUBEXP  10
typedef struct regexp {
   char *startp[NSUBEXP];
   char *endp[NSUBEXP];
   char regstart;   /* Internal use only. */
   char reganch;    /* Internal use only. */
   char *regmust;   /* Internal use only. */
   int regmlen;     /* Internal use only. */
   char program[1]; /* Unwarranted chumminess with compiler. */
} regexp;

/* From submodes.h */

struct submode
{
    char *modename;
    char *flaglist;
    char *description;
    struct submode *next;
};

/* From Tolist.h */

struct listinfo {
   char *listname;
   char *listflags;
   struct listinfo *next;
};

struct tolist {
   char *address;
   struct listinfo *linfo;
   struct tolist *next;
};

/* From User.h */

#define USER_FULLBOUNCE		0x0001
#define USER_ECHOPOST		0x0002
#define USER_VACATION		0x0004
#define USER_ADMIN		0x0008

#define ERR_LISTOPEN		-1
#define ERR_LISTCREATE		-2
#define ERR_LISTWRITE		-3
#define ERR_LISTSWAP		-4

#define ERR_NOTADMIN		-5
#define ERR_FLAGSET		-6
#define ERR_FLAGNOTSET		-7
#define ERR_NOSUCHFLAG		-8
#define ERR_UNSETTABLE		-9

struct list_user {
   char address[BIG_BUF];
   char flags[HUGE_BUF];
   struct list_user *next;
};

/* From Variables.h */

#define HASHSIZE 64

#define VAR_GLOBAL     0x0001
#define VAR_SITE       0x0002
#define VAR_LIST       0x0004
#define VAR_TEMP       0x0008
#define VAR_INTERNAL   0x0010
#define VAR_LOCKED     0x0020
#define VAR_NOEXPAND   0x0040
#define VAR_RESTRICTED 0x0080

#define VAR_ALL VAR_GLOBAL|VAR_SITE|VAR_LIST|VAR_TEMP
#define VAR_ANY VAR_GLOBAL|VAR_SITE|VAR_LIST|VAR_TEMP

enum var_type {
    VAR_STRING, VAR_BOOL, VAR_INT, VAR_DURATION, VAR_DATA, VAR_TIME,
    VAR_CHOICE
};

struct var_data {
   char *name;
   char *description;
   char *section;
   char *example;
   char *defval;
   char *global;
   char *site;
   char *list;
   char *temp;
   char *expanded;
   char *choices;
   int flags;
   enum var_type type;
   struct var_data *next;
};

struct list_vars {
   struct var_data *bucket[HASHSIZE];
};

/* from Unmime.h */
struct mime_field {
   char *name;
   char *value;
   char *params;
};

struct mime_header {
   int numfields;
   struct mime_field **fields;
};

#define MIME_PARAMS struct mime_header *header, const char *mimefile
typedef int (*MimeFn)(MIME_PARAMS);

#define MIME_HANDLE_OK		0
#define MIME_HANDLE_FAIL	1

#define MIME_HANDLER(x)  int x(MIME_PARAMS)

struct mime_handler {
   char *mimetype;
   MimeFn handler;
   int priority;
   struct mime_handler *next;
};

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

/* From Funcparse.h */
typedef int(*FuncFn)(char **argv, char *result, char *error);

struct listserver_funcdef
{   
    char *funcname;
    char *desc;
    int nargs;
    FuncFn fn;
    struct listserver_funcdef *next;
};

#define FUNC_HANDLER(a) int a(char **argv, char *result, char *error)

#endif
