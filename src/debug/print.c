/* debug text: a format printed on the screen */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdarg.h>
#include <stdio.h>

#include "debug/print.h"

static char debug_print_buffer[1024];

int debug_print(Int loc, const char *format, ...)
{
    va_list args;
    int count;

    va_start(args, format);
    count = vsprintf(debug_print_buffer, format, args);
    njPrintC(loc, debug_print_buffer);
    return count;
}
