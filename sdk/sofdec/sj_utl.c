/* stream joint chunk splitting */
#include <sg_xpt.h>
#include <sj.h>

/* ck1 or ck2 may be ck itself, but ck1 and ck2 must differ */
void SJ_SplitChunk(SJCK *ck, Sint32 nbyte, SJCK *ck1, SJCK *ck2)
{
    *ck1 = *ck;
    ck2->len = ck1->len;
    if (ck1->len > nbyte) {
        ck1->len = nbyte;
    }
    ck2->data = ck1->data + ck1->len;
    ck2->len -= ck1->len;
}
