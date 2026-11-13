/* SA1's easy models on the Dreamcast: the walk sa_present_live runs for a
 * live draw of SA_SA1_EASY. an easy model is a basic model lit by one light,
 * with one intensity a point. sdk/ninja/njModelE.c is SA1's layer. it keeps
 * the light and projection njSetEasyLight takes, the model clip, the tree and
 * the point count. it hands each model here with its state. a near model
 * has a point at z 0 or less. of a near model, the walk sends only what lies
 * beyond z = 1. it reads and leaves Ninja's words through the names
 * header.h and this header declare. it reads the model's bytes during the
 * call alone.
 *
 * the walk keeps njModelE.c's float operations in their order. so from the
 * same state it sends the tile accelerator the same words. it writes the
 * points through the store queues as njEasyCalcModel does. so the queues
 * hold what njEasyCalcModel leaves in them. a word a header or vertex leaves
 * unwritten sends what its queue holds. it departs from njModelE.c in these
 * ways:
 * - an opaque untextured meshset's header takes its material's alpha.
 *   njModelE.c's can take an uninitialized alpha, as its @bug tag says.
 * - a clip left with fewer than three corners, which only a NaN z leaves,
 *   sends nothing. njModelE.c's sends uninitialized corners.
 * - Ninja's counts of the polygons it takes and sends, which nothing
 *   reads, go uncounted */
#ifndef SA1_EASY_H
#define SA1_EASY_H

#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <sh4zam/shz_mem.h>
#include <sh4zam/shz_vector.h>

#include "batch.h"
#include "rules.h"
#include "sa1/header.h"
#include "sa_dc.h"
#include "sa_sa1.h"

/* the current texture's bank, which a draw reads and leaves beside
 * sa1_live_texture. sa1_live_color_mode points to the current context's
 * color mode, which a draw sets */
extern Uint32 sa1_live_texture_bank;
extern Uint32 *sa1_live_color_mode;

/* a model's texture coordinates and a material's color bytes count 256 to
 * one */
#define EASY_SCALE (1.0F / 256.0F)

/* the header's fields the walk reads beyond rules.h's. a color mode sets
 * EASY_MODE_GLOBAL's bits of the global word and EASY_MODE_TSP's of the TSP
 * word. EASY_MODE_TRANS holds the bits that make a mode translucent.
 * EASY_TEXTURE_ADDRESS is the texture word's address field, in 8-byte units.
 * EASY_TEXTURE_ADDRESS_MAX is video memory's 8 MB in those units */
#define EASY_MODE_GLOBAL (GLOBAL_TRANSLUCENT | GLOBAL_OBJECT_CONTROL)
#define EASY_MODE_TSP (TSP_ALPHA | TSP_IGNORE_ALPHA)
#define EASY_MODE_TRANS (GLOBAL_TRANSLUCENT | TSP_ALPHA)
#define EASY_GLOBAL_SHADOW 0x00000080
#define EASY_TEXTURE_ADDRESS 0x001fffff
#define EASY_TEXTURE_ADDRESS_MAX 0x00100000

/* a point as the walk projects and lights it */
struct sa1_easy_point {
    Float x, y;
    Float reciprocal_z;
    Float intensity;
};

/* a vertex as the tile accelerator takes it under an intensity header: a
 * store queue's 32 bytes. an untextured vertex leaves u and v unwritten. no
 * vertex writes the last word, the offset intensity, since the header leaves
 * the offset off */
struct sa1_easy_vertex {
    Uint32 control;
    Float x, y, reciprocal_z;
    Float u, v;
    Float intensity;
    Uint32 unwritten;
};

/* a polygon header: its global, ISP, TSP and texture words, and the color
 * each vertex's intensity scales */
struct sa1_easy_header {
    Uint32 global, isp, tsp, texture;
    NJS_ARGB color;
};

/* an sa_draw_live of SA_SA1_EASY's state. njSetEasyLight sets the light's
 * direction in view space and the screen's center and scales. scale_x is
 * the screen x per unit of x / z, and scale_y likewise for y. the screen's
 * near stays 0. njSetEasyLightIntensity sets the light's intensity, negated,
 * and its ambient. tint is the light's color, which a textured meshset's
 * header takes. a translucent one leaves its material's alpha there for
 * later draws. points is Ninja's vertex buffer, whose 32-byte lines the draw
 * writes whole */
struct sa1_live_easy {
    NJS_VECTOR light;
    Float negative_diffuse;
    Float ambient;
    zam_screen_t screen;
    NJS_ARGB *tint;
    struct sa1_easy_point *points;
};

/* project and light model's points into state->points as njEasyCalcModel
 * does, by batch.h's kernel. the kernel's light is state->light turned into
 * model space by XMTRX's transpose and scaled by the negated intensity. its
 * w is the ambient, the least intensity a point takes. this stays out of
 * line, so that no live value holds a register the kernel's asm names.
 * return how many points lie at z 0 or less */
static __attribute__((noinline)) int
easy_project(const NJS_MODEL *model, const struct sa1_live_easy *state)
{
    shz_vec3_t turned = zam_xmtrx_transform_vec3_transpose(&state->light.x);
    Float diffuse = state->negative_diffuse;

    return zam_xmtrx_project_lit_points(
        &model->points->x, &model->normals->x, model->nbPoint, &state->screen,
        shz_vec4_init(turned.x * diffuse, turned.y * diffuse,
                      turned.z * diffuse, state->ambient),
        state->points);
}

/* send a vertex from sq, writing from the intensity down to the control
 * word as njModelE.c does. PREF at the control word then sends the queue.
 * machine.h's prefetch gives the PREF, since its memory clobber keeps the
 * stores ahead of it. SH4ZAM's SHZ_PREFETCH names no memory. return the next
 * vertex's address */
ALWAYS_INLINE struct sa1_easy_vertex *
easy_send(struct sa1_easy_vertex *sq, Uint32 control, Float x, Float y,
          Float reciprocal_z, Float intensity, Bool textured, Float u, Float v)
{
    sq->intensity = intensity;
    if (textured) {
        sq->v = v;
        sq->u = u;
    }
    sq->reciprocal_z = reciprocal_z;
    sq->y = y;
    sq->x = x;
    sq->control = control;
    prefetch(sq);
    return sq + 1;
}

/* send point p from sq, with the coordinates of uv's pair i where textured.
 * an untextured meshset's vertuv may be null, and C gives a null pointer no
 * offset. so a send takes uv and an index, and forms a pair's address only
 * where textured */
ALWAYS_INLINE struct sa1_easy_vertex *
easy_send_point(struct sa1_easy_vertex *sq, Uint32 control,
                const struct sa1_easy_point *p, Bool textured,
                const NJS_COLOR *uv, int i)
{
    return easy_send(sq, control, p->x, p->y, p->reciprocal_z, p->intensity,
                     textured, textured ? uv[i].tex.u * EASY_SCALE : 0.0F,
                     textured ? uv[i].tex.v * EASY_SCALE : 0.0F);
}

/* whether p lies beyond z = 1, its 1/z between 0 and 1 */
ALWAYS_INLINE Bool easy_direct(const struct sa1_easy_point *p)
{
    return p->reciprocal_z > 0.0F && p->reciprocal_z < 1.0F;
}

/* twice triangle a, b, c's signed area on the screen. easy_polygons sends an
 * unclipped triangle or quad only where its first three points' area is
 * negative */
ALWAYS_INLINE Float easy_area(const struct sa1_easy_point *a,
                              const struct sa1_easy_point *b,
                              const struct sa1_easy_point *c)
{
    return ((b->x - a->x) * (c->y - b->y)) - ((b->y - a->y) * (c->x - b->x));
}

/* a triangle's corner in easy_clip. easy_corner recovers its place in view
 * space by inverting the projection. u and v are in units of 1/256 */
struct sa1_easy_corner {
    Float x, y, reciprocal_z;
    Float view_x, view_y, view_z;
    Float u, v;
    Float intensity;
};

/* corner from point p, with the coordinates of uv's pair i where textured.
 * inverse_x and inverse_y are the reciprocals of state->screen's scales */
ALWAYS_INLINE void easy_corner(struct sa1_easy_corner *corner,
                               const struct sa1_easy_point *p, Bool textured,
                               const NJS_COLOR *uv, int i, Float inverse_x,
                               Float inverse_y,
                               const struct sa1_live_easy *state)
{
    corner->x = p->x;
    corner->y = p->y;
    corner->reciprocal_z = p->reciprocal_z;
    corner->view_z = 1.0F / p->reciprocal_z;
    corner->view_x = (p->x - state->screen.cx) * corner->view_z * inverse_x;
    corner->view_y = (p->y - state->screen.cy) * corner->view_z * inverse_y;
    corner->intensity = p->intensity;
    corner->u = textured ? uv[i].tex.u : 0.0F;
    corner->v = textured ? uv[i].tex.v : 0.0F;
}

/* send corner from sq, its coordinates scaled where textured */
ALWAYS_INLINE struct sa1_easy_vertex *
easy_send_corner(struct sa1_easy_vertex *sq, Uint32 control,
                 const struct sa1_easy_corner *corner, Bool textured)
{
    return easy_send(sq, control, corner->x, corner->y, corner->reciprocal_z,
                     corner->intensity, textured, corner->u * EASY_SCALE,
                     corner->v * EASY_SCALE);
}

/* send the part of triangle a, b, c beyond z = 1 from sq, as a strip. the
 * strip takes the triangle's corners beyond z = 1. it takes the points where
 * the triangle's edges cross z = 1 too, whose 1/z is 1. a fourth corner goes
 * before the third, so that the strip's two triangles cover the part. uv
 * holds a's, b's and c's coordinates where textured. the crossings take
 * them in units of 1/256. the clip culls nothing, and sends nothing where
 * no corner lies beyond z = 1. return the address past the last vertex */
ALWAYS_INLINE struct sa1_easy_vertex *
easy_clip(struct sa1_easy_vertex *sq, const struct sa1_easy_point *a,
          const struct sa1_easy_point *b, const struct sa1_easy_point *c,
          Bool textured, const NJS_COLOR *uv, const struct sa1_live_easy *state)
{
    Float inverse_x = 1.0F / state->screen.scale_x;
    Float inverse_y = 1.0F / state->screen.scale_y;
    struct sa1_easy_corner in[3];
    struct sa1_easy_corner out[4];
    int count = 0;
    int last = 2;
    int i;

    easy_corner(&in[0], a, textured, uv, 0, inverse_x, inverse_y, state);
    easy_corner(&in[1], b, textured, uv, 1, inverse_x, inverse_y, state);
    easy_corner(&in[2], c, textured, uv, 2, inverse_x, inverse_y, state);
    if (!(in[0].view_z > 1.0F || in[1].view_z > 1.0F || in[2].view_z > 1.0F)) {
        return sq;
    }
    for (i = 0; i < 3; last = i, i++) {
        const struct sa1_easy_corner *previous = &in[last];
        const struct sa1_easy_corner *current = &in[i];

        if ((previous->view_z <= 1.0F) ^ (current->view_z <= 1.0F)) {
            struct sa1_easy_corner *cut = &out[count++];
            Float t = (1.0F - previous->view_z) /
                      (current->view_z - previous->view_z);
            Float view_x =
                previous->view_x + ((current->view_x - previous->view_x) * t);
            Float view_y =
                previous->view_y + ((current->view_y - previous->view_y) * t);

            cut->x = (view_x * state->screen.scale_x) + state->screen.cx;
            cut->y = (view_y * state->screen.scale_y) + state->screen.cy;
            cut->reciprocal_z = 1.0F;
            cut->u = previous->u + ((current->u - previous->u) * t);
            cut->v = previous->v + ((current->v - previous->v) * t);
            cut->intensity = previous->intensity +
                             ((current->intensity - previous->intensity) * t);
        }
        if (current->view_z > 1.0F) {
            out[count++] = *current;
        }
    }
    if (count < 3) {
        return sq;
    }
    sq = easy_send_corner(sq, KM_VERTEXPARAM_NORMAL, &out[0], textured);
    sq = easy_send_corner(sq, KM_VERTEXPARAM_NORMAL, &out[1], textured);
    if (count != 3) {
        sq = easy_send_corner(sq, KM_VERTEXPARAM_NORMAL, &out[3], textured);
    }
    return easy_send_corner(sq, KM_VERTEXPARAM_ENDOFSTRIP, &out[2], textured);
}

/* easy_clip out of line, untextured and textured, since the polygon sends
 * call it seldom */
static __attribute__((noinline)) struct sa1_easy_vertex *
easy_clip_untextured(struct sa1_easy_vertex *sq, const struct sa1_easy_point *a,
                     const struct sa1_easy_point *b,
                     const struct sa1_easy_point *c,
                     const struct sa1_live_easy *state)
{
    return easy_clip(sq, a, b, c, FALSE, NULL, state);
}

static __attribute__((noinline)) struct sa1_easy_vertex *
easy_clip_textured(struct sa1_easy_vertex *sq, const struct sa1_easy_point *a,
                   const struct sa1_easy_point *b,
                   const struct sa1_easy_point *c, const NJS_COLOR *uv,
                   const struct sa1_live_easy *state)
{
    return easy_clip(sq, a, b, c, TRUE, uv, state);
}

/* clip a, b and c by the copy of easy_clip textured picks. where textured,
 * their pairs are uv's from pair i on */
ALWAYS_INLINE struct sa1_easy_vertex *
easy_cut(struct sa1_easy_vertex *sq, const struct sa1_easy_point *a,
         const struct sa1_easy_point *b, const struct sa1_easy_point *c,
         Bool textured, const NJS_COLOR *uv, int i,
         const struct sa1_live_easy *state)
{
    return textured ? easy_clip_textured(sq, a, b, c, &uv[i], state)
                    : easy_clip_untextured(sq, a, b, c, state);
}

/* send meshset's strips from sq, with a coordinate pair for each point where
 * textured. each strip must hold three points or more. unless near, each
 * strip goes whole. where near, a strip goes a triangle at a time. each run
 * of triangles whose points all lie beyond z = 1 goes as one strip. a
 * triangle with only some points beyond goes through easy_clip, with the
 * pairs from its first point's on. return the address past the last vertex */
ALWAYS_INLINE struct sa1_easy_vertex *
easy_strips(struct sa1_easy_vertex *sq, const NJS_MESHSET *meshset,
            const struct sa1_easy_point *points, Bool textured, Bool near,
            const struct sa1_live_easy *state)
{
    const Sint16 *index = meshset->meshes;
    const NJS_COLOR *uv = meshset->vertuv;
    int strips;

    for (strips = meshset->nbMesh; strips != 0; strips--) {
        int count = *index++ & STRIP_COUNT;
        const struct sa1_easy_point *v0 = &points[index[0]];
        const struct sa1_easy_point *v1 = &points[index[1]];
        const struct sa1_easy_point *v2 = v1;
        int run = 0;
        int k;

        SHZ_PREFETCH(index + count);
        if (!near) {
            for (k = 0; k < count - 1; k++) {
                sq = easy_send_point(sq, KM_VERTEXPARAM_NORMAL,
                                     &points[index[k]], textured, uv, k);
            }
            sq = easy_send_point(sq, KM_VERTEXPARAM_ENDOFSTRIP,
                                 &points[index[k]], textured, uv, k);
        } else {
            for (k = 0; k < count - 2; k++, v0 = v1, v1 = v2) {
                Bool whole;

                v2 = &points[index[k + 2]];
                whole = easy_direct(v0) && easy_direct(v1) && easy_direct(v2);
                if (whole) {
                    if (!run) {
                        sq = easy_send_point(sq, KM_VERTEXPARAM_NORMAL, v0,
                                             textured, uv, k);
                        run = 1;
                    }
                    sq = easy_send_point(sq, KM_VERTEXPARAM_NORMAL, v1,
                                         textured, uv, k + 1);
                } else {
                    if (run) {
                        sq = easy_send_point(sq, KM_VERTEXPARAM_ENDOFSTRIP, v1,
                                             textured, uv, k + 1);
                        run = 0;
                    }
                    if (easy_direct(v0) || easy_direct(v1) || easy_direct(v2)) {
                        sq = easy_cut(sq, v0, v1, v2, textured, uv, k, state);
                    }
                }
            }
            if (run) {
                sq = easy_send_point(sq, KM_VERTEXPARAM_ENDOFSTRIP, v2,
                                     textured, uv, count - 1);
            }
        }
        index += count;
        /* a null uv takes no step: easy_send_point's rule */
        if (textured) {
            uv += count;
        }
    }
    return sq;
}

/* send meshset's triangles or quads from sq, as size is 3 or 4. each point
 * takes a coordinate pair where textured. a polygon whose first three
 * points' area is negative goes as a strip of its points in order. where
 * near, one with a point not beyond z = 1 goes through easy_clip instead,
 * unculled. a clipped quad goes as two triangles, its points 0 to 2 and 1 to
 * 3. each goes only where one of its points lies beyond z = 1. return the
 * address past the last vertex */
ALWAYS_INLINE struct sa1_easy_vertex *
easy_polygons(struct sa1_easy_vertex *sq, const NJS_MESHSET *meshset,
              const struct sa1_easy_point *points, int size, Bool textured,
              Bool near, const struct sa1_live_easy *state)
{
    const Sint16 *index = meshset->meshes;
    const NJS_COLOR *uv = meshset->vertuv;
    int left;

    /* a null uv takes no step: easy_send_point's rule */
    for (left = meshset->nbMesh; left != 0;
         left--, index += size, uv = textured ? uv + size : uv) {
        const struct sa1_easy_point *v0 = &points[index[0]];
        const struct sa1_easy_point *v1 = &points[index[1]];
        const struct sa1_easy_point *v2 = &points[index[2]];
        const struct sa1_easy_point *v3 = size == 4 ? &points[index[3]] : v2;
        int k;

        SHZ_PREFETCH(index + 2 * size);
        if (near && !(easy_direct(v0) && easy_direct(v1) && easy_direct(v2) &&
                      easy_direct(v3))) {
            if (easy_direct(v0) || easy_direct(v1) || easy_direct(v2)) {
                sq = easy_cut(sq, v0, v1, v2, textured, uv, 0, state);
            }
            if (size == 4 &&
                (easy_direct(v3) || easy_direct(v1) || easy_direct(v2))) {
                sq = easy_cut(sq, v1, v2, v3, textured, uv, 1, state);
            }
        } else if (easy_area(v0, v1, v2) < 0.0F) {
            for (k = 0; k < size; k++) {
                sq = easy_send_point(sq,
                                     k == size - 1 ? KM_VERTEXPARAM_ENDOFSTRIP
                                                   : KM_VERTEXPARAM_NORMAL,
                                     &points[index[k]], textured, uv, k);
            }
        }
    }
    return sq;
}

/* send meshset's polygons from sq, as its NJD_MESHSET_ type says: strips,
 * triangles or quads. an N-gon meshset sends nothing. return the address
 * past the last vertex */
ALWAYS_INLINE struct sa1_easy_vertex *
easy_meshset(struct sa1_easy_vertex *sq, const NJS_MESHSET *meshset,
             const struct sa1_easy_point *points, Bool textured, Bool near,
             const struct sa1_live_easy *state)
{
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_TRIMESH:
        return easy_strips(sq, meshset, points, textured, near, state);
    case NJD_MESHSET_3:
        return easy_polygons(sq, meshset, points, 3, textured, near, state);
    case NJD_MESHSET_4:
        return easy_polygons(sq, meshset, points, 4, textured, near, state);
    }
    return sq;
}

/* easy_meshset out of line, for each pair of textured and near */
static __attribute__((noinline)) struct sa1_easy_vertex *
easy_untextured(struct sa1_easy_vertex *sq, const NJS_MESHSET *meshset,
                const struct sa1_easy_point *points,
                const struct sa1_live_easy *state)
{
    return easy_meshset(sq, meshset, points, FALSE, FALSE, state);
}

static __attribute__((noinline)) struct sa1_easy_vertex *
easy_textured(struct sa1_easy_vertex *sq, const NJS_MESHSET *meshset,
              const struct sa1_easy_point *points,
              const struct sa1_live_easy *state)
{
    return easy_meshset(sq, meshset, points, TRUE, FALSE, state);
}

static __attribute__((noinline)) struct sa1_easy_vertex *
easy_untextured_near(struct sa1_easy_vertex *sq, const NJS_MESHSET *meshset,
                     const struct sa1_easy_point *points,
                     const struct sa1_live_easy *state)
{
    return easy_meshset(sq, meshset, points, FALSE, TRUE, state);
}

static __attribute__((noinline)) struct sa1_easy_vertex *
easy_textured_near(struct sa1_easy_vertex *sq, const NJS_MESHSET *meshset,
                   const struct sa1_easy_point *points,
                   const struct sa1_live_easy *state)
{
    return easy_meshset(sq, meshset, points, TRUE, TRUE, state);
}

/* set the header's words for a meshset of material drawn with flags in
 * mode. it sets them as njSetkmModelAttr, njSetkmTextureNum and
 * njSetkmColorMode do, in that order. where flags ask a texture, the
 * material's texture becomes current unless it and its bank are current
 * already. a texture word whose address passes video memory's 8 MB leaves
 * the header's as it was. the context's color mode becomes mode */
ALWAYS_INLINE void easy_set_header(const NJS_MATERIAL *material, Uint32 flags,
                                   Uint32 mode)
{
    struct nj_parameterkm *header = &sa1_live_header;
    Uint32 control = sa1_live_control;
    Uint32 global;

    header->TSPPARAMBUFFER =
        (header->TSPPARAMBUFFER & ~MATERIAL_TSP) | (flags & MATERIAL_TSP);
    if (flags & NJD_FLAG_USE_TEXTURE) {
        const NJS_TEXMEMLIST *texture =
            (const NJS_TEXMEMLIST *)sa1_live_texlist
                ->textures[material->attr_texId & MATERIAL_TEXTURE]
                .texaddr;

        if (texture->globalIndex != sa1_live_texture ||
            sa1_live_texture_bank != texture->bank) {
            sa1_live_texture = texture->globalIndex;
            sa1_live_texture_bank = texture->bank;
            header->TSPPARAMBUFFER = (header->TSPPARAMBUFFER & ~TSP_UV_SIZE) |
                                     (texture->tspparambuffer & TSP_UV_SIZE);
            if ((texture->texparambuffer & EASY_TEXTURE_ADDRESS) <=
                EASY_TEXTURE_ADDRESS_MAX) {
                header->TexturePARAMBUFFER = texture->texparambuffer;
            }
        }
    }
    global = (header->GLOBALPARAMBUFFER & ~EASY_MODE_GLOBAL) |
             (mode & EASY_MODE_GLOBAL);
    if ((control & NJD_CONTROL_3D_SHADOW) ||
        ((control & NJD_CONTROL_3D_SHADOW_OPAQUE) &&
         !(mode & EASY_MODE_TRANS))) {
        global |= EASY_GLOBAL_SHADOW;
    } else {
        global &= ~EASY_GLOBAL_SHADOW;
    }
    header->GLOBALPARAMBUFFER = global;
    header->TSPPARAMBUFFER =
        (header->TSPPARAMBUFFER & ~EASY_MODE_TSP) | (mode & EASY_MODE_TSP);
    *sa1_live_color_mode = mode;
}

/* open a polygon with color at the end of list, as the njStart functions
 * do. list is the translucent list where trans is TRUE, and the opaque one
 * otherwise. return the address of its first vertex */
ALWAYS_INLINE struct sa1_easy_vertex *
easy_open(Uint32 **list, Bool textured, Bool trans, const NJS_ARGB *color)
{
    struct nj_parameterkm *header = &sa1_live_header;
    Uint32 at = (Uint32)*list;
    struct sa1_easy_header *sq =
        (struct sa1_easy_header *)((at & ZAM_SQ_ADDRESS) | ZAM_SQ_BASE);

    sa1_live_sq_base = at & LIST_TOP;
    ZAM_QACR0 = ZAM_SQ_AREA(at);
    ZAM_QACR1 = ZAM_SQ_AREA(at);
    sq->color.b = color->b;
    sq->color.g = color->g;
    sq->color.r = color->r;
    sq->color.a = color->a;
    if (textured) {
        sq->texture = header->TexturePARAMBUFFER;
    }
    sq->tsp = trans ? header->TSPPARAMBUFFER
                    : (header->TSPPARAMBUFFER & ~TSP_BLEND) | TSP_OPAQUE;
    sq->isp = header->ISPPARAMBUFFER;
    sq->global = header->GLOBALPARAMBUFFER;
    prefetch(sq);
    sa1_live_list = list;
    return (struct sa1_easy_vertex *)(sq + 1);
}

/* draw model as its state says, consumed in the call, each meshset in a
 * polygon of its own. a model whose points all lie at z 0 or less draws
 * nothing. a model with some point there goes through the near sends */
ALWAYS_INLINE void sa1_easy_draw(const NJS_MODEL *model,
                                 const struct sa1_live_easy *state)
{
    struct nj_parameterkm *header = &sa1_live_header;
    const NJS_MESHSET *meshset = model->meshsets;
    const struct sa1_easy_point *points = state->points;
    int behind = easy_project(model, state);
    Bool near = behind != 0;
    int left;

    if (behind == model->nbPoint) {
        return;
    }
    for (left = model->nbMeshset; left != 0; left--, meshset++) {
        const NJS_MATERIAL *material =
            &model->mats[meshset->type_matId & MATERIAL_ID];
        Uint32 flags = material->attrflags;
        Uint32 **list;
        Bool textured;
        Bool trans;
        NJS_ARGB color;
        const NJS_ARGB *taken;
        struct sa1_easy_vertex *sq;

        if (sa1_live_control & NJD_CONTROL_3D_CONSTANT_ATTR) {
            flags = (flags & sa1_live_attr_and) | sa1_live_attr_or;
        }
        textured = (flags & NJD_FLAG_USE_TEXTURE) != 0;
        trans = (flags & NJD_FLAG_USE_ALPHA) != 0;
        easy_set_header(material, flags,
                        (trans ? COLOR_MODE_TRANS : COLOR_MODE_OPAQUE) |
                            (textured ? COLOR_MODE_TEXTURE : 0) |
                            COLOR_MODE_INTENSITY);
        if (textured) {
            if (trans) {
                state->tint->a = material->diffuse.argb.a * EASY_SCALE;
            }
            taken = state->tint;
        } else {
            color.a = material->diffuse.argb.a * EASY_SCALE;
            color.r = material->diffuse.argb.r * state->tint->r * EASY_SCALE;
            color.g = material->diffuse.argb.g * state->tint->g * EASY_SCALE;
            color.b = material->diffuse.argb.b * state->tint->b * EASY_SCALE;
            taken = &color;
        }
        list = trans ? &header->trans_polygon : &header->opaque_polygon;
        sq = easy_open(list, textured, trans, taken);
        SHZ_PREFETCH(meshset->meshes);
        if (textured) {
            sq = near ? easy_textured_near(sq, meshset, points, state)
                      : easy_textured(sq, meshset, points, state);
        } else {
            sq = near ? easy_untextured_near(sq, meshset, points, state)
                      : easy_untextured(sq, meshset, points, state);
        }
        *list = (Uint32 *)(((Uint32)sq & ZAM_SQ_ADDRESS) | sa1_live_sq_base);
    }
}

/* draw an easy model as its state says, consumed in the call, into the
 * lists Ninja's header names. the call compiles into the layer's draw, as
 * live.h's does. boundary.py fails an object that includes two headers
 * defining the call. the engine goes unread */
ALWAYS_INLINE sa_result sa_present_live(sa_engine *engine,
                                        const sa_draw_live *draw)
{
    (void)engine;
    if (draw->format != SA_SA1_EASY) {
        return SA_ERR_UNSUPPORTED;
    }
    sa1_easy_draw(draw->geometry, draw->state);
    return SA_OK;
}

#endif
