/* the C library's string.h, read ahead of the adapter's sources under
 * another name for its strupr: ninja.h declares Ninja's own, which returns
 * nothing, and a source that reads both would hold two */
#ifndef PSP_LIBC_H
#define PSP_LIBC_H

#define strupr libc_strupr
#include <string.h>
#undef strupr

#endif
