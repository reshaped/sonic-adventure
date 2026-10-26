#include <ninja.h>
#include <sg_xpt.h>

/* clip the polygon of n points in at the near plane into out. a point whose z
 * is at or above _nj_near_clip_ is kept. each point is taken in order with the
 * edge that ends at it, from the point before it, or for the first point from
 * the last. where that edge crosses the plane, its crossing goes into out
 * ahead of the point, with its color interpolated. texture coordinates,
 * interpolated at a crossing and copied for a kept point, are written only
 * under NJD_USE_TEXTURE. returns the number of points written to out */
Int njNearClipping(NJS_POINT3COL *in, Int n, NJS_POINT3COL *out, Uint32 attr)
{
    Int i;
    Int last;
    Int count;
    Float t;

    count = 0;
    last = n - 1;
    for (i = 0; i < n; i++) {
        if ((in->p[last].z - _nj_near_clip_) * (in->p[i].z - _nj_near_clip_) <
            0.0F) {
            t = (_nj_near_clip_ - in->p[last].z) / (in->p[i].z - in->p[last].z);
            out->p[count].x =
                in->p[last].x + ((in->p[i].x - in->p[last].x) * t);
            out->p[count].y =
                in->p[last].y + ((in->p[i].y - in->p[last].y) * t);
            out->p[count].z = _nj_near_clip_;
            if (attr & NJD_USE_TEXTURE) {
                out->tex[count].tex.u =
                    in->tex[last].tex.u +
                    ((in->tex[i].tex.u - in->tex[last].tex.u) * t);
                out->tex[count].tex.v =
                    in->tex[last].tex.v +
                    ((in->tex[i].tex.v - in->tex[last].tex.v) * t);
            }
            out->col[count].argb.a =
                in->col[last].argb.a +
                ((in->col[i].argb.a - in->col[last].argb.a) * t);
            out->col[count].argb.r =
                in->col[last].argb.r +
                ((in->col[i].argb.r - in->col[last].argb.r) * t);
            out->col[count].argb.g =
                in->col[last].argb.g +
                ((in->col[i].argb.g - in->col[last].argb.g) * t);
            out->col[count].argb.b =
                in->col[last].argb.b +
                ((in->col[i].argb.b - in->col[last].argb.b) * t);
            count++;
        }
        if (in->p[i].z >= _nj_near_clip_) {
            out->p[count].x = in->p[i].x;
            out->p[count].y = in->p[i].y;
            out->p[count].z = in->p[i].z;
            if (attr & NJD_USE_TEXTURE) {
                out->tex[count].tex.u = in->tex[i].tex.u;
                out->tex[count].tex.v = in->tex[i].tex.v;
            }
            out->col[count].color = in->col[i].color;
            count++;
        }
        last = i;
    }
    return count;
}
