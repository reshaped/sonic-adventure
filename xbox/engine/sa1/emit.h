/* the Xbox port's runs of points already on the screen: what the walk
 * (xbox/engine/sa1/model.c) projects and clips itself, and SA1's screen
 * polygons, 2D polygons and lines (sa1/strip.h's sa1_send_vertex), go to the
 * renderer (xbox/port/render.c) as runs, the host's own vertices, rather than
 * as the tile accelerator's vertices in Kamui's lists, which the renderer
 * would read back. the list keeps each draw's header and the run's mark,
 * PARA_MODEL, at the draw's place in its order.
 *
 * emit_begin opens a run where port_gpu_models' bit allows and the
 * renderer's tables have room. each vertex then goes to it, emit_vertex, and
 * each strip's end makes its triangles. emit_end closes it and writes the
 * mark. a run that overflows isn't ended: its slot stays free, and the
 * caller sends the draw into its list as the Dreamcast does. one run is
 * open at a time: its state, port_emit, is the port layer's
 * (xbox/port/emit.c) */
#ifndef XBOX_SA1_EMIT_H
#define XBOX_SA1_EMIT_H

#include <ninja.h>
#include <sg_xpt.h>

/* the renderer's vertex, xbox/port/host.h's host_vertex */
struct run_vertex {
    Float x, y, z;
    Float u, v;
    Uint32 base, offset;
};
/* a strip of a translucent run, the renderer's run_part: its indices in the
 * run's, and the 1/z of its nearest vertex */
struct run_part {
    int first_index, index_count;
    Float rhw_near;
};
int port_run_begin(struct run_vertex **vertices, int *vertex_room, Uint16 **indices, int *index_room);
struct run_part *port_run_parts(int *room);
void port_run_end(int run, int vertices, int indices, int part_total, const Float transform[12], Float rhw_near,
                  Float rhw_mid, unsigned serial);
/* a translucent run's vertices' depths, for the renderer's sort by triangle,
 * which reads them in cached memory, not the run's vertices, which lie in
 * the host's write-combined memory that the CPU reads uncached: where they
 * go and their room, then the run's, once port_run_end or port_run_kept
 * made it (xbox/port/render.c) */
Float *port_run_depths(int *room);
void port_run_depths_take(int run, int vertices);
extern volatile int port_gpu_models;
/* port_gpu_models' bits: the walk's projected meshsets, and SA1's screen
 * polygons, 2D polygons and lines */
#define EMIT_MESHSETS 32
#define EMIT_SCREEN 64
/* the list's mark of a run, in a vertex's place */
#define PARA_MODEL 0xc0000000u
#define VERTEX_END_OF_STRIP 0x10000000u
/* a run of points on the screen: each vertex holds x * z, y * z and z for
 * the point's x, y and z = 1 / rhw, so that the host's x and y over z and 1
 * over z, by these identity rows, are the point's own (host.h's host_item).
 * the runs share the rows, which their one serial says */
extern const Float port_screen_rows[12];
#define SCREEN_SERIAL 0xffffffffu

/* the open strip's vertices that are numbers, as emit_vertex wrote them, by
 * their place modulo EMIT_GOOD: bit 0 x and y, bit 1 z. emit_strip_end reads
 * these, not the run's vertices, which lie in write-combined memory: read
 * back, each load went to the memory uncached. a strip longer than EMIT_GOOD
 * reads its vertices */
#define EMIT_GOOD 1024
#define EMIT_GOOD_XY 1u
#define EMIT_GOOD_Z 2u

struct port_emit {
    Bool on;
    Bool overflow;
    Bool sorted;
    int run;
    struct run_vertex *out;
    int room, count;
    Uint16 *indices;
    int index_room, index_count;
    struct run_part *parts;
    int part_room, part_count;
    int strip_first;
    Float strip_near;
    Float rhw_near, rhw_sum;
    /* a sorted run's vertices' depths (port_run_depths), or null: past
     * their room the run has none */
    Float *depths;
    int depth_room;
    Uint8 good[EMIT_GOOD];
    /* a sprite's colors, from its header's intensity colors (emit_face):
     * the tile accelerator's sprite vertices hold an intensity of 1, and the
     * header the colors they take */
    Bool face_valid;
    Uint32 face, face_offset;
    /* an easy model's meshset's color, A R G B, which each vertex's
     * intensity scales (emit_color) */
    Float color[4];
};
extern struct port_emit port_emit;

/* the strip from strip_first on, as triangles in the list's order: the odd
 * ones swapped (render.c's flush_strip), each corner a number. a sorted
 * run's strips sort apart, a part each by its nearest 1/z */
static inline void emit_strip_end(void)
{
    struct port_emit *e = &port_emit;
    int from = e->index_count;
    Bool noted = e->count - e->strip_first <= EMIT_GOOD;
    int k;

    for (k = e->strip_first + 2; k < e->count; k++) {
        int a = k - 2 + (k & 1);
        int b = k - 1 - (k & 1);

        if (e->index_count + 3 > e->index_room) {
            e->overflow = TRUE;
            return;
        }
        /* a corner of no number draws as a wedge across the screen on the
         * NV2A, where the PowerVR shows nothing */
        if (noted) {
            if (!(e->good[a % EMIT_GOOD] & EMIT_GOOD_XY) || !(e->good[b % EMIT_GOOD] & EMIT_GOOD_XY) ||
                (e->good[k % EMIT_GOOD] & (EMIT_GOOD_XY | EMIT_GOOD_Z)) != (EMIT_GOOD_XY | EMIT_GOOD_Z)) {
                continue;
            }
        } else if (!__builtin_isfinite(e->out[a].x) || !__builtin_isfinite(e->out[a].y) ||
                   !__builtin_isfinite(e->out[b].x) || !__builtin_isfinite(e->out[b].y) ||
                   !__builtin_isfinite(e->out[k].x) || !__builtin_isfinite(e->out[k].y) ||
                   !__builtin_isfinite(e->out[k].z)) {
            continue;
        }
        e->indices[e->index_count++] = (Uint16)a;
        e->indices[e->index_count++] = (Uint16)b;
        e->indices[e->index_count++] = (Uint16)k;
    }
    if (e->sorted && e->index_count > from) {
        if (e->part_count == e->part_room) {
            e->overflow = TRUE;
            return;
        }
        e->parts[e->part_count].first_index = from;
        e->parts[e->part_count].index_count = e->index_count - from;
        e->parts[e->part_count].rhw_near = e->strip_near;
        e->part_count++;
    }
    e->strip_first = e->count;
    e->strip_near = 0.0F;
}

/* a vertex at x, y on the screen, of 1/z rhw, with control's end of strip */
static inline void emit_vertex(Float x, Float y, Float rhw, Float u, Float v, Uint32 base, Uint32 offset,
                               Uint32 control)
{
    struct port_emit *e = &port_emit;
    struct run_vertex *out;
    Float z, xz, yz;

    if (e->overflow) {
        return;
    }
    if (e->count == e->room) {
        e->overflow = TRUE;
        return;
    }
    z = 1.0F / rhw;
    xz = x * z;
    yz = y * z;
    /* noted as written: the run's memory is read back uncached */
    e->good[e->count % EMIT_GOOD] = (Uint8)((__builtin_isfinite(xz) && __builtin_isfinite(yz) ? EMIT_GOOD_XY : 0) |
                                            (__builtin_isfinite(z) ? EMIT_GOOD_Z : 0));
    if (e->depths != NULL) {
        /* the screen rows' third row takes z as it is */
        if (e->count < e->depth_room) {
            e->depths[e->count] = z;
        } else {
            e->depths = NULL;
        }
    }
    out = &e->out[e->count++];
    out->x = xz;
    out->y = yz;
    out->z = z;
    out->u = u;
    out->v = v;
    out->base = base;
    out->offset = offset;
    e->rhw_sum += rhw;
    if (rhw > e->rhw_near) {
        e->rhw_near = rhw;
    }
    if (rhw > e->strip_near) {
        e->strip_near = rhw;
    }
    if (control & VERTEX_END_OF_STRIP) {
        emit_strip_end();
    }
}

/* open a run, its strips sorted apart where sorted, as port_gpu_models' bit
 * allows: FALSE where it doesn't, or the renderer's tables are full */
static inline Bool emit_begin(Bool sorted, int bit)
{
    struct port_emit *e = &port_emit;
    int run;

    if (e->on || !(port_gpu_models & bit) || (sorted && !(port_gpu_models & 2))) {
        return FALSE;
    }
    run = port_run_begin(&e->out, &e->room, &e->indices, &e->index_room);
    if (run < 0) {
        return FALSE;
    }
    e->part_room = 0;
    e->parts = sorted ? port_run_parts(&e->part_room) : NULL;
    if (sorted && e->part_room == 0) {
        return FALSE;
    }
    e->depth_room = 0;
    e->depths = sorted ? port_run_depths(&e->depth_room) : NULL;
    e->run = run;
    e->sorted = sorted;
    e->on = TRUE;
    e->overflow = FALSE;
    e->count = e->index_count = e->part_count = 0;
    e->strip_first = 0;
    e->strip_near = e->rhw_near = e->rhw_sum = 0.0F;
    return TRUE;
}

/* emit_begin, where the run has room for vertices, indices and parts at
 * least: for a draw that can't be sent again where its run overflows, as
 * Ninja's print, whose characters are sent once */
static inline Bool emit_begin_room(Bool sorted, int bit, int vertices, int indices, int parts)
{
    struct port_emit *e = &port_emit;

    if (!emit_begin(sorted, bit)) {
        return FALSE;
    }
    if (e->room < vertices || e->index_room < indices || (sorted && e->part_room < parts)) {
        e->on = FALSE;
        return FALSE;
    }
    return TRUE;
}

/* an ARGB word of four colors of 0 to 1, as the renderer makes one
 * (render.c's float_color) */
static inline Uint32 emit_argb(const Uint32 *words)
{
    Uint32 out = 0;
    int k;

    for (k = 0; k < 4; k++) {
        Float f;
        int i;

        __builtin_memcpy(&f, &words[k], sizeof f);
        i = (int)(f * 255.0F + 0.5F);
        i = i < 0 ? 0 : i > 255 ? 255 : i;
        out = (out << 8) | (Uint32)i;
    }
    return out;
}

/* take the colors of the intensity header the draw at start follows in its
 * list: an 8-word header's face color at its words 4 to 7, or a 16-word
 * one's at 8 to 11 and its offset color at 12 to 15. a draw that follows a
 * run's mark keeps the colors of the header the mark's run followed. FALSE
 * where start follows no such header, and the draw goes to the list */
#define PCW_TYPE_POLYGON 4u
#define PCW_INTENSITY 2u
#define PCW_HAS_OFFSET 0x04u
static inline Bool emit_face(const Uint32 *start)
{
    struct port_emit *e = &port_emit;
    const Uint32 *h = NULL;

    if (start[-8] == PARA_MODEL) {
        return e->face_valid;
    }
    if ((start[-8] >> 29) == PCW_TYPE_POLYGON && !(start[-8] & PCW_HAS_OFFSET)) {
        h = start - 8;
    } else if ((start[-16] >> 29) == PCW_TYPE_POLYGON && (start[-16] & PCW_HAS_OFFSET)) {
        h = start - 16;
    }
    e->face_valid = h != NULL && ((h[0] >> 4) & 3) == PCW_INTENSITY;
    if (!e->face_valid) {
        return FALSE;
    }
    if (h[0] & PCW_HAS_OFFSET) {
        e->face = emit_argb(h + 8);
        e->face_offset = emit_argb(h + 12);
    } else {
        e->face = emit_argb(h + 4);
        e->face_offset = 0;
    }
    return TRUE;
}

/* the color an intensity header gives, which each vertex's intensity then
 * scales, as the renderer reads such a vertex (render.c's read_vertex):
 * alpha as it is, and red, green and blue times the intensity */
static inline void emit_color(const NJS_ARGB *color)
{
    port_emit.color[0] = color->a;
    port_emit.color[1] = color->r;
    port_emit.color[2] = color->g;
    port_emit.color[3] = color->b;
}

static inline Uint32 emit_intensity(Float i)
{
    Float scaled[4] = {port_emit.color[0], port_emit.color[1] * i, port_emit.color[2] * i,
                       port_emit.color[3] * i};
    Uint32 words[4];

    __builtin_memcpy(words, scaled, sizeof words);
    return emit_argb(words);
}

/* close the run, its mark at at in the list. returns where the list goes
 * on: past the mark, at at where nothing was drawn, or null where the run
 * overflowed and the draw goes to the list after all */
static inline Uint32 *emit_end(Uint32 *at)
{
    struct port_emit *e = &port_emit;
    int k;

    e->on = FALSE;
    if (e->strip_first < e->count) {
        emit_strip_end();
    }
    if (e->overflow) {
        return NULL;
    }
    if (e->index_count == 0) {
        return at;
    }
    port_run_end(e->run, e->count, e->index_count, e->part_count, port_screen_rows, e->rhw_near,
                 e->rhw_sum / (Float)e->count, SCREEN_SERIAL);
    if (e->depths != NULL) {
        port_run_depths_take(e->run, e->count);
    }
    at[0] = PARA_MODEL;
    at[1] = (Uint32)e->run;
    for (k = 2; k < 8; k++) {
        at[k] = 0;
    }
    return at + 8;
}

#endif
