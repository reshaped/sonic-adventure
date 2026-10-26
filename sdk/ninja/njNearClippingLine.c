#include <ninja.h>
#include <sg_xpt.h>

/* clip the line between in's two points at the near plane into out. a point
 * whose z is at or above _nj_near_clip_ is kept. where the points lie on
 * opposite sides of the plane, the crossing goes in out[0] and the kept point
 * in out[1]. the crossing's color is interpolated, and under NJD_USE_TEXTURE
 * its texture coordinates too. where there is no crossing, each kept point
 * goes in out at its index in in. a kept point's texture coordinates are
 * copied wherever out->tex is not null. a point on the plane makes no
 * crossing: with the other point below the plane, out keeps what it held at
 * that other point's index. returns 0, writing nothing, when both points lie
 * below the plane, else 2 */
Int njNearClippingLine(NJS_POINT3COL *in, NJS_POINT3COL *out, Uint32 attr)
{
    Int cut = 0;
    Float t;

    if ((in->p[1].z - _nj_near_clip_) * (in->p[0].z - _nj_near_clip_) < 0.0F) {
        t = (_nj_near_clip_ - in->p[1].z) / (in->p[0].z - in->p[1].z);
        out->p[0].x = in->p[1].x + ((in->p[0].x - in->p[1].x) * t);
        out->p[0].y = in->p[1].y + ((in->p[0].y - in->p[1].y) * t);
        out->p[0].z = _nj_near_clip_;
        if (attr & NJD_USE_TEXTURE) {
            out->tex[0].tex.u =
                in->tex[1].tex.u + ((in->tex[0].tex.u - in->tex[1].tex.u) * t);
            out->tex[0].tex.v =
                in->tex[1].tex.v + ((in->tex[0].tex.v - in->tex[1].tex.v) * t);
        }
        out->col[0].argb.a =
            in->col[1].argb.a + ((in->col[0].argb.a - in->col[1].argb.a) * t);
        out->col[0].argb.r =
            in->col[1].argb.r + ((in->col[0].argb.r - in->col[1].argb.r) * t);
        out->col[0].argb.g =
            in->col[1].argb.g + ((in->col[0].argb.g - in->col[1].argb.g) * t);
        out->col[0].argb.b =
            in->col[1].argb.b + ((in->col[0].argb.b - in->col[1].argb.b) * t);
        cut = 1;
    }
    if (in->p[0].z >= _nj_near_clip_) {
        out->p[cut].x = in->p[0].x;
        out->p[cut].y = in->p[0].y;
        out->p[cut].z = in->p[0].z;
        if (out->tex != NULL) {
            out->tex[cut].tex.u = in->tex[0].tex.u;
            out->tex[cut].tex.v = in->tex[0].tex.v;
        }
        out->col[cut].color = in->col[0].color;
    }
    if (in->p[1].z >= _nj_near_clip_) {
        out->p[1].x = in->p[1].x;
        out->p[1].y = in->p[1].y;
        out->p[1].z = in->p[1].z;
        if (out->tex != NULL) {
            out->tex[1].tex.u = in->tex[1].tex.u;
            out->tex[1].tex.v = in->tex[1].tex.v;
        }
        out->col[1].color = in->col[1].color;
    }
    if (in->p[0].z < _nj_near_clip_ && in->p[1].z < _nj_near_clip_) {
        return 0;
    }
    return 2;
}
