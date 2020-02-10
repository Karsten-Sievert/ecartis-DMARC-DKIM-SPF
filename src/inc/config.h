#ifndef _CONFIG_H
#define _CONFIG_H

#include "compat.h"
#include "version.h"

#define SMALL_BUF 256
#define BIG_BUF 1024
#define HUGE_BUF 2048

/* This one to help typing safe code in strings functions */
#define SAFELEN(x) (sizeof(x) - 1)
/* These two as new str... SAFE functions */
/* (in waiting for a new library of SAFE strings manipulations functions) */
/* TO USE ONLY THEN x is fully know (local or global defined char array) !!!
     not when x is a pointer received as a argument of a function... */
#define stringcat(dest,src) strncat(dest, src, sizeof(dest) - 1 - strlen(dest))
#define stringcpy(dest,src) strncpy(dest, src, sizeof(dest) - 1)

#define SERVICE_NAME_UC "ECARTIS"
#define SERVICE_NAME_MC "Ecartis"
#define SERVICE_NAME_LC "ecartis"
#define GLOBAL_CFG_FILE "ecartis.cfg"
#define SERVICE_ADDRESS "ecartis@localhost"

#endif /* _CONFIG_H */
