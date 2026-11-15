/* njClipZ: set the near and far clip planes to z = n and z = f, each held
 * to [-65535, -1], and _nj_near_clip_ and _nj_far_clip_ to them negated.
 * make pc compiles this in place of sdk/ninja/njClipZ.c, whose body is SH-4
 * assembly. a NaN stays as it is, as the body's FCMP/GTs leave it */
#include <ninja.h>

/* hold z to [-65535, -1] */
static Float clip_plane(Float z)
{
    if (z > -1.0F) {
        return -1.0F;
    }
    if (-65535.0F > z) {
        return -65535.0F;
    }
    return z;
}

void(njClipZ)(Float n, Float f)
{
    Float nearer = clip_plane(n);
    Float farther = clip_plane(f);

    _nj_near_clip_ = -nearer;
    _nj_far_clip_ = -farther;
}
