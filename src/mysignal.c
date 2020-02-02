#include "config.h"
#include "mysignal.h"

#include <stdlib.h>

#include <signal.h>
#ifndef WIN32
#include <sysexits.h>
#endif

void signal_handler(int sig)
{
    exit(EX_TEMPFAIL);
}

void init_signals(void)
{
#ifdef SIGILL
  signal(SIGILL, signal_handler);
#endif

#ifdef SIGABRT
  signal(SIGABRT, signal_handler);
#endif

#ifdef SIGSEGV
  signal(SIGSEGV, signal_handler);
#endif

#ifdef SIGTERM
  signal(SIGTERM, signal_handler);
#endif

#ifdef SIGBUF
  signal(SIGBUS, signal_handler);
#endif

#ifdef SIGSTKFLT
  signal(SIGSTKFLT, signal_handler);
#endif
}


