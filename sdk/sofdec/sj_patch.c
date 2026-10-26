/* middleware wrapper for SJ_SplitChunk */
#include <sg_xpt.h>
#include <sj.h>

void mwSjSplitChunk(SJCK *ck, Sint32 nbyte, SJCK *ck1, SJCK *ck2)
{
    SJ_SplitChunk(ck, nbyte, ck1, ck2);
}
