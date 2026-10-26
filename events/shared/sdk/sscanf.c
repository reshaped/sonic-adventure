#include <stdarg.h>
#include <stdio.h>

#include "shared/sdk/scan.h"

int sscanf(const char *s, const char *format, ...)
{
    va_list args;
    int count;

    va_start(args, format);
    count = _fmtin(1, NULL, s, format, args);
    va_end(args);
    return count;
}
