/* debug text: a format printed on the screen */
#ifndef DEBUG_PRINT_H
#define DEBUG_PRINT_H
#include <sg_xpt.h>

/* print the text format gives at loc, as njPrintC does, and return its
 * length. the text is formatted unchecked into one 1024-byte buffer, so it
 * must fit in 1023 bytes. every call shares that buffer, so a call must not
 * interrupt another */
int debug_print(Int loc, const char *format, ...);

#endif
