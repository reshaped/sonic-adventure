/* play math by the host's own FPU, for a host too slow for fast.c's
 * integers: the PSP, whose processor takes some hundred instructions for a
 * sum of four products made as the SH-4 makes it, where the game makes
 * thousands of them a picture. FMAC, FIPR, FTRV and FSRRA are the FPU's own
 * here, each in its operation's order, under the mode its caller holds, as
 * the game's adds and multiplies are on every host. so their bits are that
 * FPU's: not math.c's, and not another host's. the rest are fast.c's, which
 * this file holds under fast_ names for the four */
#include "play_math.h"

#define SA_NUMERIC_FPU
#include "fast.c"
#undef sa_fmac
#undef sa_fipr
#undef sa_ftrv
#undef sa_fsrra

float sa_fmac(float f0, float fm, float fn)
{
    return f0 * fm + fn;
}

float sa_fipr(const float a[4], const float b[4])
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}

void sa_ftrv(const float m[16], const float v[4], float out[4])
{
    float sums[4];
    int i;

    for (i = 0; i < 4; i++) {
        sums[i] =
            m[i] * v[0] + m[i + 4] * v[1] + m[i + 8] * v[2] + m[i + 12] * v[3];
    }
    memcpy(out, sums, sizeof sums);
}

float sa_fsrra(float a)
{
    return 1.0F / sqrtf(a);
}
