/* Shinobi store queue setup, as far as the sources use it */
#ifndef SG_SQ_H
#define SG_SQ_H
#include <sg_xpt.h>

void sySqInit(void);
void sySqFinish(void);
/* point the store queues at dstArray, aligned down to 32 bytes, and return the
 * store queue address that writes there. with address translation on, UTLB
 * entry 57 maps dstArray's 1 MB page, and *size gets the bytes from that
 * address to the page's end. with translation off, *size gets 0x1fffffff */
void *sySqStart(Uint32 *size, void *dstArray);

#endif
