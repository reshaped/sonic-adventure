/* C runtime string formatting and scanning, as far as the sources use it */
#ifndef STDIO_H
#define STDIO_H
#include <stdarg.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* SHC's stream, through the last field the runtime reads */
typedef struct _iobuf {
    unsigned char *_bufptr;
    long _bufcnt;
    unsigned char *_bufbase;
    long _buflen;
    char _ioflag1;
} FILE;

/* _ioflag1 bits */
#define _IOREAD 1 /* a read-only stream, or an update stream reading */
#define _IOWRITE 2 /* a write-only stream, or an update stream writing */
#define _IORW 4 /* open for update */
#define _IOERR 64 /* an I/O error */

/* these write s through _fmtout. runtime/print.h says where it departs from
 * ISO C */
int sprintf(char *s, const char *format, ...);
int vsprintf(char *s, const char *format, va_list args);
/* sscanf links only into the events. it reads s through _fmtin, and
 * shared/sdk/scan.h says where that departs from ISO C */
int sscanf(const char *s, const char *format, ...);

#endif
