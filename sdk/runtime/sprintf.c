#include <stdarg.h>
#include <stdio.h>

#include "runtime/print.h"

int sprintf(char *s, const char *format, ...)
{
    va_list args;
    int length;

    va_start(args, format);
    length = _fmtout(PRINT_TO_STRING, NULL, s, format, args);
    va_end(args);
    return length;
}
