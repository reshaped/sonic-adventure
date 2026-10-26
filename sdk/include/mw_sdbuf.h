/* eight sound buffer handles, each with the number of a stream ring buffer
 * bank, which a sound port (mw_snd.h) takes one of for each channel */
#ifndef MW_SDBUF_H
#define MW_SDBUF_H
#include <sg_xpt.h>

/* free all eight handles, numbered 15 down to 8 in the order mwSndBufCreate
 * takes them. a handle still in use is freed with the rest */
void mwSndBufInit(void);
/* reserve one handle, or return NULL when all eight are occupied */
void *mwSndBufCreate(void);
/* free buf, a handle mwSndBufCreate returned */
void mwSndBufDestroy(void *buf);
/* buf's number, from 8 to 15 */
Sint32 mwSndBufGetNo(void *buf);
#endif
