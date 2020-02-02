#include <stdio.h>
#include "core.h"

void regerror(char *s)
{
    log_printf(1, "regexp: %s\n", s);
}
