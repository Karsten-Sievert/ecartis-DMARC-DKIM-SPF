#ifndef _COMMAND_H
#define _COMMAND_H

#include "config.h"

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

extern void new_commands(void);
extern void nuke_commands(void);
extern void add_command(char *name, char *desc, char *syntax, char *altsyntax,
                        char *adminsyntax, int flags, CmdFn cmd);
extern struct listserver_cmd *find_command(char *name, int flags);
extern struct listserver_cmd *get_commands(void);

#endif /* _COMMAND_H */
