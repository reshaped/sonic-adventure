/* the vertex sends of SA1's screen draws, which reach the tile accelerator
 * through the store queues. Ninja's screen polygons, 2D polygons, 2D and 3D
 * lines and print each take a format of sa_present_live's. a header of its
 * own defines the call for each. an object includes one of them at most, as
 * engine/tests/boundary.py checks. the draw's layer is the Ninja entry that
 * calls sa_present_live. it opens the strip through Ninja, which writes the
 * strip's header with its texture and list. the layer then sets sa1_live_sq
 * to the strip's first vertex. the call leaves it past the last vertex it
 * sent */
#ifndef SA1_STRIP_H
#define SA1_STRIP_H

#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>

/* NJS_COLOR's tex.u and tex.v count 256 to the texture's side */
#define SA1_UV_SCALE (1.0F / 256.0F)
/* half a 2D line's width in pixels, as Ninja's own lines take it */
#define SA1_HALF_WIDTH 0.525F

/* the store queue address of the strip's next vertex. being static, it can
 * stay in a register through the layer's draw and the inlined call */
static Uint32 *sa1_live_sq __attribute__((unused));

/* send a vertex from sq on, and return the next vertex's address. a vertex
 * takes 8 words, and its color is word 6. the stores run from the color down
 * to the control word, as Ninja's do. PREF at the control word then sends the
 * queue. machine.h's prefetch keeps the stores ahead of it. SH4ZAM's
 * SHZ_PREFETCH names no memory, so GCC could move them past it. without a
 * texture, words 4 and 5 keep what the queue held: the untextured vertex
 * reserves them */
ALWAYS_INLINE Uint32 *sa1_send_vertex(Uint32 *sq, Uint32 control, Float x,
                                      Float y, Float z, Int textured, Float u,
                                      Float v, Uint32 color)
{
    sq += 7;
    *--sq = color;
    if (textured) {
        *(Float *)--sq = v;
        *(Float *)--sq = u;
    } else {
        sq -= 2;
    }
    *(Float *)--sq = z;
    *(Float *)--sq = y;
    *(Float *)--sq = x;
    *--sq = control;
    prefetch(sq);
    return sq + 8;
}

/* an end of a 2D line, or a polygon's corner. x and y are in pixels */
struct sa1_end {
    Float x, y, u, v;
    Uint32 color;
};

/* point i of p as an end, its u and v 0 where untextured */
ALWAYS_INLINE void sa1_end_at(struct sa1_end *end, const NJS_POINT2COL *p,
                              Int i, Int textured)
{
    end->x = p->p[i].x;
    end->y = p->p[i].y;
    end->u = textured ? p->tex[i].tex.u * SA1_UV_SCALE : 0.0F;
    end->v = textured ? p->tex[i].tex.v * SA1_UV_SCALE : 0.0F;
    end->color = p->col[i].color;
}

/* send the line from a to b at z as a strip of four vertices from sq on. the
 * vertices lie SA1_HALF_WIDTH to either side of the line. return the address
 * past the last, or sq where a and b meet */
ALWAYS_INLINE Uint32 *sa1_send_edge(Uint32 *sq, const struct sa1_end *a,
                                    const struct sa1_end *b, Float z,
                                    Int textured)
{
    Float dx = b->x - a->x;
    Float dy = b->y - a->y;
    Float length2 = (dx * dx) + (dy * dy);
    Float inv_length;
    Float side_x;
    Float side_y;

    /* ends that meet on the screen give no side to widen to: 1/length is
     * infinite there, and the vertices would take NaNs. send none */
    if (!(length2 > 0.0F)) {
        return sq;
    }
    inv_length = 1.0F / _builtin_sqrtf(length2);
    side_x = dy * inv_length * SA1_HALF_WIDTH;
    side_y = -(dx * inv_length * SA1_HALF_WIDTH);
    sq = sa1_send_vertex(sq, KM_VERTEXPARAM_NORMAL, a->x + side_x,
                         a->y + side_y, z, textured, a->u, a->v, a->color);
    sq = sa1_send_vertex(sq, KM_VERTEXPARAM_NORMAL, a->x - side_x,
                         a->y - side_y, z, textured, a->u, a->v, a->color);
    sq = sa1_send_vertex(sq, KM_VERTEXPARAM_NORMAL, b->x + side_x,
                         b->y + side_y, z, textured, b->u, b->v, b->color);
    return sa1_send_vertex(sq, KM_VERTEXPARAM_ENDOFSTRIP, b->x - side_x,
                           b->y - side_y, z, textured, b->u, b->v, b->color);
}

/* the 1/z a 2D draw at priority pri takes */
ALWAYS_INLINE Float sa1_rhw_2d(Float pri)
{
    return pri < -1.0F ? -1.0F / pri : 1.0F;
}

#endif
