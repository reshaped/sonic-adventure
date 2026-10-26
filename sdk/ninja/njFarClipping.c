#include <ninja.h>
#include <sg_xpt.h>

Int njFarClipping(NJS_POINT3COL *p, Int n)
{
    Int i;

    for (i = 0; i < n; i++) {
        if (p->p[i].z > _nj_far_clip_) {
            return 0;
        }
    }
    return 1;
}
