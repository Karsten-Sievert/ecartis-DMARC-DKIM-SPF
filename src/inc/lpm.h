#ifndef LSM_H
#define LSM_H

#include "config.h"
#include "version.h"

#ifdef DYNMOD
# include "lpm-def.h"
#else
# include "alias.h"
# include "cmdarg.h"
# include "command.h"
# include "cookie.h"
# include "core.h"
# include "file.h"
# include "fileapi.h"
# include "flag.h"
# include "forms.h"
# include "hooks.h"
# include "list.h"
# include "lcgi.h"
# include "modes.h"
# include "module.h"
# include "mystring.h"
# include "parse.h"
# include "regexp.h"
# include "smtp.h"
# include "submodes.h"
# include "tolist.h"
# include "unhtml.h"
# include "unmime.h"
# include "user.h"
# include "userstat.h"
# include "variables.h"
# include "funcparse.h"
#endif

#include "lpm-api.h"

#endif
