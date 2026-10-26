/* the sound library's debug output */
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdarg.h>
#include <stdio.h>

static char print_buffer[1024];

/* the formatted text must fit print_buffer's 1024 bytes: vsprintf doesn't
 * check its length */
int sdDebugPrintf(const char *format, ...)
{
    va_list args;

    if (sdgPrintFunc != NULL && sdgDebugMode == true) {
        va_start(args, format);
        vsprintf(print_buffer, format, args);
        va_end(args);
        sdDebugPrint(print_buffer);
    }
    return 0;
}

int sdDebugPrint(const char *str)
{
    if (sdgPrintFunc != NULL && sdgDebugMode == true) {
        sdgPrintFunc(str);
    }
    return 0;
}

int sdDebugSetPrintFunc(void (*func)(const char *str))
{
    sdgPrintFunc = func;
    return 0;
}
