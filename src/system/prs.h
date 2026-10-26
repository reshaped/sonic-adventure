/* decoding PRS, the LZ77 that packs the disc's files */
#ifndef SYSTEM_PRS_H
#define SYSTEM_PRS_H
#include <sg_xpt.h>

/* decode the PRS stream at from into to, and return the bytes it decodes.
 * it checks nothing: the stream must end in its end marker, no copy may
 * reach back past to, and to must hold the output */
Sint32 prs_decompress(void *from, void *to);

#endif
