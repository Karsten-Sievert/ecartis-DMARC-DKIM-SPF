#ifndef _MODERATE_H
#define _MODERATE_H

#include "cookie.h"

extern void make_moderated_post(const char *reason);
extern void do_moderate(const char *infilename);
extern COOKIE_HANDLER(expire_modpost);

#endif // _MODERATE_H
