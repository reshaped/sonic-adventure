/* ADX error reporting, through the error function a caller sets */
#include <sofdec.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* the last error's message, which the next one replaces. vsprintf doesn't
 * bound it, so a message must fit 256 bytes with its NUL */
char adxerr_msg[256];
void (*adxerr_func)(void *obj, char *msg) = NULL;
void *adxerr_obj = NULL;

void ADXERR_Init(void)
{
    memset(adxerr_msg, 0, sizeof(adxerr_msg));
    adxerr_func = NULL;
    adxerr_obj = NULL;
}

void ADXERR_Finish(void)
{
    memset(adxerr_msg, 0, sizeof(adxerr_msg));
    adxerr_func = NULL;
    adxerr_obj = NULL;
}

void ADXERR_EntryErrFunc(void (*func)(void *obj, char *msg), void *obj)
{
    adxerr_func = func;
    adxerr_obj = obj;
}

void ADXERR_CallErrFunc(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vsprintf(adxerr_msg, fmt, args);
    va_end(args);
    if (adxerr_func != NULL) {
        adxerr_func(adxerr_obj, adxerr_msg);
    }
}
