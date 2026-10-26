#include <stdarg.h>
#include <stdio.h>

#include "runtime/print.h"

int vsprintf(char *s, const char *format, va_list args)
{
    return _fmtout(PRINT_TO_STRING, NULL, s, format, args);
}
