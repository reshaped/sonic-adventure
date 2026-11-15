/* the model renderer in C, on the SH-4's FTRV and FIPR (the SH-4's own under
 * make gcc, the port's native models under XBOX). make gcc compiles it
 * in place of model.src. model.src's project_points projects x by
 * _nj_screen_.dist alone, and its cull_model tests x by it. its near clip
 * takes dist times _nj_scr_aspect_x_, as Ninja's own projections do. the two
 * scales agree at the shipped aspect of 1.
 * the port's 16:9 sets the picture's aspect, port.h's port_draw_aspect, to
 * 3/4. there model.src would draw each model a third too wide, and
 * cull_model would drop the models in the sides 16:9 adds. the draws here
 * take that aspect, and model.src keeps its bytes. draw_in_view takes the
 * window's, port_view_aspect, where model.src takes _nj_scr_aspect_x_.
 *
 * where model.src computes a float result from the same inputs, the code
 * here takes model.src's operations in model.src's order, and so keeps its
 * bits. model.src holds no FMAC, and make gcc fuses no product into a sum in
 * gcc/. some of model.src's tests branch on a false FCMP/GT, which a NaN
 * gives. the tests here spell those !(a > b), which holds for a NaN where
 * a <= b doesn't.
 *
 * apart from the aspect, the code here departs from model.src in these:
 * - the draws project and count a model's points alone. model.src's
 *   project_points takes the 12 bytes past an odd count of points as one
 *   more, as its @bug tag says.
 * - a meshset that sends draw_unlit_offsets sends 0 for a point whose index
 *   is DRAW_UNLIT_OFFSETS or more. model.src's strips_tex reads that point's
 *   offset color past draw_unlit_offsets, by FMOV.S @(R0,R3),FR4 at
 *   0x8c0389f0. it finds there the offset color shade_points gave the point
 *   DRAW_UNLIT_OFFSETS before it, in lit_offsets.
 * - near_clip sends nothing for a triangle left with fewer than three
 *   corners, which only a NaN or infinite z leaves. model.src's near clip
 *   fills the strip out with corners an earlier clip left.
 * - draw_model_unculled sends nothing for a model off the screen, by
 *   model_off_screen's test.
 * - send_clipped_strips sends nothing for a triangle off the screen that
 *   near_clip would cut, by triangle_off_screen's test. model.src's near
 *   clip sends what is left of it at z >= 1.
 * - the draws send nothing while event_skipping is TRUE, when a skip's
 *   curtain covers the screen.
 * - draw_model and draw_model_nested draw on step 1 too where port.h's
 *   FRAME_DRAWS says, for a split frame's second picture. timer 1 stops
 *   across those draws, as port.h's present_untimed says.
 * - draw_model_init leaves GBR as it was, since nothing here reads GBR. it
 *   reads the screen's width and height too, for model_off_screen and
 *   off_edges, so a change to either takes another call.
 *
 * model.src's other bugs that change what a draw sends stay, each under
 * a @bug tag. its code that nothing calls is gone, and so are its reads
 * and writes past an array that change nothing sent.
 *
 * ALWAYS_INLINE makes GCC inline a function whatever its inline limits.
 * next_vector, view_next, take_point, send_textured and send_point take it
 * so that the loops over points and vertices make no call. send_strips and
 * send_clipped_strips take it so that each strips_ and clipped_ function
 * holds its own copy, with its uv a constant. left to choose, GCC compiles
 * one send_clipped_strips out of line, with uv an argument.
 *
 * OUT_OF_LINE keeps a function out of line under its own name, where GCC
 * would inline it or clone it under another. ld.py places each function
 * config/1st_read/kernel.txt names by its name, so each static one here
 * takes OUT_OF_LINE. so does each clipped_ function, so that the near clip
 * stays out of the kernel */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>
#ifndef XBOX
#include <sh4zam/shz_mem.h>
#include <sh4zam/shz_vector.h>
#include <sh4zam/shz_xmtrx.h>
#endif

#include "draw/model.h"
#include "sdk/ninja/njkamui.h"
#include "system/frame.h"

#define OUT_OF_LINE static __attribute__((noinline, noclone))

/* the vectors and the FTRV and FIPR the drawer runs. make gcc's are its own
 * SH-4 entries, each draw_ name one of them. the Xbox port's are its native
 * models of the SH-4's instructions and its XMTRX (xbox/include/sh4.h),
 * which ninja.h includes */
#ifdef XBOX
typedef struct {
    Float x, y;
} draw_vec2;
typedef struct {
    Float x, y, z;
} draw_vec3;
typedef struct {
    Float x, y, z, w;
} draw_vec4;

ALWAYS_INLINE draw_vec4 draw_vec4_init(Float x, Float y, Float z, Float w)
{
    draw_vec4 v = {x, y, z, w};

    return v;
}

/* FTRV XMTRX of v */
ALWAYS_INLINE draw_vec4 draw_xmtrx_vec4(draw_vec4 v)
{
    sh4_xmtrx_ftrv(&v.x, &v.x);
    return v;
}

/* FTRV XMTRX of (p, 1), its first three results */
ALWAYS_INLINE draw_vec3 draw_xmtrx_point3(const NJS_POINT3 *p)
{
    draw_vec4 v = draw_xmtrx_vec4(draw_vec4_init(p->x, p->y, p->z, 1.0F));
    draw_vec3 r = {v.x, v.y, v.z};

    return r;
}

/* FTRV XMTRX of (n, 0), its first three results */
ALWAYS_INLINE draw_vec3 draw_xmtrx_vec3(const NJS_VECTOR *n)
{
    draw_vec4 v = draw_xmtrx_vec4(draw_vec4_init(n->x, n->y, n->z, 0.0F));
    draw_vec3 r = {v.x, v.y, v.z};

    return r;
}

/* FIPR of a and b, a first */
ALWAYS_INLINE Float draw_fipr(draw_vec4 a, draw_vec4 b)
{
    return sh4_fipr(a.x, a.y, a.z, a.w, b.x, b.y, b.z, b.w);
}

/* two FIPRs of l, with r1 and with r2 */
ALWAYS_INLINE draw_vec2 draw_dot2(draw_vec4 l, draw_vec4 r1, draw_vec4 r2)
{
    draw_vec2 r = {draw_fipr(l, r1), draw_fipr(l, r2)};

    return r;
}

/* XMTRX's column j, XF4j to XF4j+3 */
ALWAYS_INLINE draw_vec4 draw_xmtrx_col(int j)
{
    return draw_vec4_init(sh4_xmtrx[4 * j], sh4_xmtrx[4 * j + 1],
                          sh4_xmtrx[4 * j + 2], sh4_xmtrx[4 * j + 3]);
}

/* (FIPRs of dir with XMTRX's first three columns, 0): a direction in the
 * model's space */
ALWAYS_INLINE draw_vec4 draw_in_model(draw_vec4 dir)
{
    return draw_vec4_init(draw_fipr(dir, draw_xmtrx_col(0)),
                          draw_fipr(dir, draw_xmtrx_col(1)),
                          draw_fipr(dir, draw_xmtrx_col(2)), 0.0F);
}

/* MOVCA.L's claim of a cache line, which x86 leaves to its cache */
#define draw_alloc_line(p) ((void)(p))
#else
typedef shz_vec2_t draw_vec2;
typedef shz_vec3_t draw_vec3;
typedef shz_vec4_t draw_vec4;
#define draw_vec4_init shz_vec4_init
#define draw_xmtrx_vec4 shz_xmtrx_transform_vec4
#define draw_xmtrx_point3(p) shz_xmtrx_transform_point3(zam_load_vec3(p))
#define draw_xmtrx_vec3(n) shz_xmtrx_transform_vec3(zam_load_vec3(n))
#define draw_fipr zam_fipr
#define draw_dot2 shz_vec4_dot2
#define draw_xmtrx_col shz_xmtrx_read_col
#define draw_in_model(dir)                                                     \
    shz_vec3_vec4(shz_vec4_dot3((dir), shz_xmtrx_read_col(0),                  \
                                shz_xmtrx_read_col(1), shz_xmtrx_read_col(2)), \
                  0.0F)
#define draw_alloc_line shz_dcache_alloc_line
#endif

/* masks, as njModel.c names them: the material index in a meshset's
 * type_matId, the texture number in a material's attr_texId, and a strip's
 * point count below NJD_TRIMESH_END */
#define MATERIAL_ID 0x3fff
#define MATERIAL_TEXTURE 0x3fffffff
#define STRIP_COUNT 0x7fff
/* the shift that takes a meshset's NJD_MESHSET_ type to its index in
 * polygon_sizes */
#define MESHSET_TYPE_SHIFT 14

/* fields of the polygon header's global and TSP words. njkm.c names each but
 * GLOBAL_VERTEX_TYPE and MODULATE_ALPHA. GLOBAL_VERTEX_TYPE is the object
 * control byte's color type, texture, offset, Gouraud and 16-bit UV bits.
 * MODULATE_ALPHA sets both bits of TSP_SHADING, the texture and shading
 * instruction, where decal clears both */
#define GLOBAL_TRANSLUCENT 0x02000000
#define GLOBAL_OBJECT_CONTROL 0x000000ff
#define GLOBAL_VERTEX_TYPE 0x0000003f
#define TSP_ALPHA 0x00100000
#define TSP_IGNORE_ALPHA 0x00080000
#define TSP_SHADING 0x000000c0
#define TSP_UV_SIZE 0x0000003f
#define MODULATE_ALPHA TSP_SHADING

/* the modes set_mode takes. a mode's GLOBAL_MODE bits go to the global word,
 * and its TSP_MODE bits to the TSP word. its TSP_SHADING bits lie where the
 * object control byte's shadow and volume bits do, which GLOBAL_MODE leaves
 * out. an untextured meshset takes ninja.h's COLOR_MODE_OPAQUE or
 * COLOR_MODE_TRANS. a textured one takes MODE_DECAL, MODE_OPAQUE_TEXTURED or
 * MODE_TRANS_TEXTURED, which add a texture and an offset color */
#define GLOBAL_MODE (GLOBAL_TRANSLUCENT | GLOBAL_VERTEX_TYPE)
#define TSP_MODE (TSP_ALPHA | TSP_IGNORE_ALPHA | TSP_SHADING)
#define TEXTURED (COLOR_MODE_TEXTURE | COLOR_MODE_OFFSET)
#define MODE_DECAL (COLOR_MODE_OPAQUE | TEXTURED)
#define MODE_OPAQUE_TEXTURED (COLOR_MODE_OPAQUE | TEXTURED | MODULATE_ALPHA)
#define MODE_TRANS_TEXTURED (COLOR_MODE_TRANS | TEXTURED | MODULATE_ALPHA)

/* the flags of a triangle's points a, b and c in a clipped strip. a point is
 * flagged unless its 1/z is over 0 and at most 1, where it lies at z = 1 or
 * past it. a NaN 1/z flags it */
#define FLAGGED_A 0x01
#define FLAGGED_B 0x02
#define FLAGGED_C 0x04
#define FLAGGED_ALL (FLAGGED_A | FLAGGED_B | FLAGGED_C)

/* off_edges counts a corner past an edge of the screen only where it lies
 * OFF_EDGE pixels past it, and only within OFF_REACH of the eye, in pixels
 * at z = 1. under the console's rounding toward zero, near_clip's cut between
 * two such corners lands under 4 pixels from where exact arithmetic puts it,
 * and OFF_EDGE is twice that, so the cut lies past the screen's edge too.
 * that error grows with the corners' distance from the eye, so no margin
 * alone bounds it */
#define OFF_EDGE 8.0F
#define OFF_REACH 0x1p21F
/* the bits of off_edges' result, one for each edge of the screen */
#define OFF_LEFT 0x1
#define OFF_RIGHT 0x2
#define OFF_TOP 0x4
#define OFF_BOTTOM 0x8

/* a land model's points can lie past its bounding sphere. over every stage's
 * land, the farthest lies 1.4143 times r from the center. model_off_screen
 * takes the sphere of LAND_REACH times r, which holds them all. a model with
 * a point past it may lose polygons on the screen */
#define LAND_REACH 1.5F

/* model.src's 1/255, H'3B808083, which lies 2 units in the last place above
 * H'3B808081, the float nearest 1/255. the draws scale u, v and alpha by it,
 * so that they keep model.src's bits */
#define INV_255 0x1.010106p-8F
#define HALF_255 127.5F

/* the drawer PREFs PREFETCH_AHEAD bytes, two lines, ahead of its reads of a
 * model's points and normals and a polygon's indices and uvs. a PREF may
 * reach that far past an array, into other RAM: it fills a line as a load
 * would and writes out no store queue. no DMA writes RAM a PREF reaches
 * while the draws run: the pads' buffers are read through P2, and a
 * stream's sectors lie kilobytes past its work's output buffers */
#define PREFETCH_AHEAD 64

/* where a point lies on the screen, and its 1/z */
struct screen {
    Float x, y;
    Float rhw;
};

/* a point as project_points leaves it in _nj_vertex_buf_, with the base
 * color shade_points gives it */
struct point {
    struct screen at;
    NJS_COLOR color;
};
/* take_point reaches point index at index times 16, and a pair of points
 * fills a 32-byte cache line */
_Static_assert(sizeof(struct point) == 16, "a point isn't 16 bytes");

/* a vertex as the tile accelerator takes it, KMVERTEX3's words. an
 * untextured one leaves u, v and offset unwritten */
struct vertex {
    Uint32 control;
    struct screen at;
    Float u, v;
    Uint32 base, offset;
};
/* a vertex fills the 32 bytes of a store queue, which one PREF sends.
 * send_textured writes it down from its end, 32 bytes past sq */
_Static_assert(sizeof(struct vertex) == 32, "a vertex isn't 32 bytes");

/* a triangle's corner as the near clip cuts it: where it lies in view space,
 * and what its strip sends. its u and v are 255 times a vertex's, and the
 * send scales them by INV_255 */
struct corner {
    Float x, y, z;
    struct screen at;
    Uint32 base, offset;
    Float u, v;
};

/* where a strip's u and v come from: nowhere, for an untextured meshset; the
 * meshset's vertuv; or, under NJD_FLAG_USE_ENV, each point's normal turned by
 * XMTRX */
enum uv { NO_UV, MESH_UV, ENV_UV };

/* a meshset as its strips send it: its polygons, the points they index, the
 * offset color each point sends, and the model's normals for ENV_UV */
struct strips {
    const NJS_MESHSET *meshset;
    /* a polygon's point count, or 0 where each polygon gives its own */
    int polygon_size;
    const struct point *points;
    const Uint32 *offsets;
    const NJS_VECTOR *normals;
};

/* how set_point_alpha takes a material's alpha: as its own,
 * _nj_constant_material_'s in its place under
 * NJD_CONTROL_3D_CONSTANT_MATERIAL, or the two added under
 * NJD_CONTROL_3D_OFFSET_MATERIAL */
enum alpha_mode { ALPHA_OWN, ALPHA_CONSTANT, ALPHA_OFFSET };

/* a polygon's point count, by meshset type: 3 for NJD_MESHSET_3, 4 for
 * NJD_MESHSET_4, and 0 for NJD_MESHSET_N and NJD_MESHSET_TRIMESH, whose
 * polygons each give their own */
static const int polygon_sizes[] = {3, 4, 0, 0};

/* an edge of the screen as a plane through the eye: a point in view space
 * lies past it where across * x + depth * z < 0, with y for x at the top and
 * bottom edges. (across, depth) is a unit normal */
struct edge {
    Float across, depth;
};

/* what the draws project by, as draw_model_init copies it: Ninja's screen,
 * its y aspect and near clip, and port_draw_aspect in place of its x
 * aspect */
static struct {
    Float cx, cy;
    Float dist;
    Float aspect_y;
    Float near;
    Float scale_x; /* dist times port_draw_aspect */
    Float scale_y; /* dist times aspect_y */
    Float inv_scale_x;
    Float inv_scale_y;
    /* _nj_screen_'s edges, as x and y project by scale_x and scale_y */
    struct edge left, right, top, bottom;
    /* _nj_screen_'s width and height */
    Float w, h;
} view;

/* Ninja's constant attributes and material mode, as take_constant_attr last
 * read them. constant_attr is TRUE under NJD_CONTROL_3D_CONSTANT_ATTR */
static Bool constant_attr;
static Uint32 attr_and;
static Uint32 attr_or;
static enum alpha_mode alpha_mode;

/* the palette the models shade from. it points into draw_colors.words, so a
 * read of the next palette after the last reads draw_unlit_offsets */
static const Uint32 *shade_palette;
/* light_dir is the direction the models shade by, in view space, and
 * light_base the one draw_set_light gave */
static NJS_VECTOR light_dir;
static NJS_VECTOR light_base;

union draw_colors draw_colors;
/* the offset colors shade_points gives a model's points, one for each */
static Uint32 lit_offsets[DRAW_MOST_POINTS];
int draw_debug_counts[DRAW_DEBUG_COUNTS];

/* the edge where across * x + depth * z is 0, with (across, depth) scaled to
 * a unit normal */
static struct edge screen_edge(Float across, Float depth)
{
    Float inv = 1.0F / __builtin_sqrtf((across * across) + (depth * depth));
    struct edge edge = {across * inv, depth * inv};

    return edge;
}

/* copy Ninja's screen, its y aspect and near clip into view, with
 * port_draw_aspect in place of its x aspect, and make the screen's edges
 * from them. a point lies on the screen where
 * 0 <= x * scale_x / z + cx <= _nj_screen_.w, and so for y with h */
void draw_model_init(void)
{
    view.cx = _nj_screen_.cx;
    view.cy = _nj_screen_.cy;
    view.dist = _nj_screen_.dist;
    view.aspect_y = _nj_scr_aspect_y_;
    view.near = _nj_near_clip_;
    view.scale_x = port_draw_aspect * view.dist;
    view.scale_y = _nj_scr_aspect_y_ * view.dist;
    view.inv_scale_x = 1.0F / view.scale_x;
    view.inv_scale_y = 1.0F / view.scale_y;
    view.left = screen_edge(view.scale_x, view.cx);
    view.right = screen_edge(-view.scale_x, _nj_screen_.w - view.cx);
    view.top = screen_edge(view.scale_y, view.cy);
    view.bottom = screen_edge(-view.scale_y, _nj_screen_.h - view.cy);
    view.w = _nj_screen_.w;
    view.h = _nj_screen_.h;
    shade_palette = draw_colors.words;
}

/* take Ninja's constant attributes and material mode for the draws that
 * follow */
OUT_OF_LINE void take_constant_attr(void)
{
    Uint32 flags = _nj_control_3d_flag_;

    constant_attr = (flags & NJD_CONTROL_3D_CONSTANT_ATTR) != 0;
    attr_and = _nj_constant_attr_and_;
    attr_or = _nj_constant_attr_or_;
    alpha_mode = ALPHA_OWN;
    if (flags & NJD_CONTROL_3D_OFFSET_MATERIAL) {
        alpha_mode = ALPHA_OFFSET;
    }
    if (flags & NJD_CONTROL_3D_CONSTANT_MATERIAL) {
        alpha_mode = ALPHA_CONSTANT;
    }
}

/* material's flags as drawn: its attrflags, through Ninja's constant
 * attributes under NJD_CONTROL_3D_CONSTANT_ATTR */
static inline Uint32 drawn_flags(const NJS_MATERIAL *material)
{
    Uint32 flags = material->attrflags;

    return constant_attr ? (flags & attr_and) | attr_or : flags;
}

static inline draw_vec3 view_point(const NJS_POINT3 *p)
{
    return draw_xmtrx_point3(p);
}

/* whether the sphere of radius r about center, in view space, may be in
 * view. the sphere is out when it lies wholly nearer than near or past
 * _nj_far_clip_. it is out too when an edge test puts it wholly beyond an
 * edge of _nj_2d_clip_. the edge tests project the sphere's sides at its far
 * z, center.z + r. the side tests scale x by side, the bottom test scales y
 * by bottom, and the top test scales y by top */
static inline Bool sphere_in_view(draw_vec3 center, Float r, Float near,
                                  Float side, Float bottom, Float top)
{
    Float inv;

    if (!(center.z > near - r) || center.z > _nj_far_clip_ + r) {
        return FALSE;
    }
    inv = 1.0F / (center.z + r);
    side *= inv;
    bottom *= inv;
    top *= inv;
    if (!((center.x + r) * side + _nj_screen_.cx > _nj_2d_clip_[0].x) ||
        (center.x - r) * side + _nj_screen_.cx > _nj_2d_clip_[1].x) {
        return FALSE;
    }
    /* @bug logic: the bottom test compares y with the clip's right edge,
     * _nj_2d_clip_[1].x, not its bottom, _nj_2d_clip_[1].y. model.src loads
     * the bottom into FR9 and compares y with FR8, the right edge: in
     * cull_model at 0x8c03821c and 0x8c038224, and in draw_in_view at
     * 0x8c039d94 and 0x8c039d9c */
    return (center.y + r) * top + _nj_screen_.cy > _nj_2d_clip_[0].y &&
           !((center.y - r) * bottom + _nj_screen_.cy > _nj_2d_clip_[1].x);
}

OUT_OF_LINE Bool model_in_view(const NJS_MODEL *model)
{
    /* @bug logic: the bottom test scales y by dist, not scale_y, and the top
     * test by aspect_y times _nj_screen_.cx. model.src's cull_model scales
     * the bottom test's y by dist in FMUL FR6,FR11 at 0x8c038220, and
     * multiplies aspect_y by cx for the top test's scale in FMUL FR4,FR7 at
     * 0x8c03820a */
    return sphere_in_view(view_point(&model->center), model->r, view.near,
                          view.scale_x, view.dist,
                          view.aspect_y * _nj_screen_.cx);
}

/* whether a sphere of radius reach lies wholly past edge. across and z are
 * its center's, in view space: its x at a side edge, or its y at the top or
 * bottom */
static inline Bool past_edge(struct edge edge, Float across, Float z,
                             Float reach)
{
    return (edge.across * across) + (edge.depth * z) < -reach;
}

/* whether model's sphere of LAND_REACH times r lies wholly past one edge of
 * the screen. past an edge, nothing the draw sends reaches a pixel. a point
 * there at z > 0 projects past the edge. a clipped triangle keeps only its
 * part at z >= 1, which lies there too */
OUT_OF_LINE Bool model_off_screen(const NJS_MODEL *model)
{
    draw_vec3 center = view_point(&model->center);
    Float reach = LAND_REACH * model->r;

    return past_edge(view.left, center.x, center.z, reach) ||
           past_edge(view.right, center.x, center.z, reach) ||
           past_edge(view.top, center.y, center.z, reach) ||
           past_edge(view.bottom, center.y, center.z, reach);
}

/* what project_points projects by: view's, in a copy no store to the points
 * can alias */
struct projection {
    Float cx, cy;
    Float scale_x, scale_y;
    Float near;
};

/* what project_begin takes from a point in view space: its z, its 1/z, and
 * its x and y times scale_x and scale_y, not yet times 1/z */
struct projecting {
    Float z;
    Float rhw;
    Float x, y;
};

static inline struct projecting project_begin(draw_vec4 v,
                                              const struct projection *by)
{
    struct projecting p = {v.z, 1.0F / v.z, v.x * by->scale_x,
                           v.y * by->scale_y};

    return p;
}

/* the vector at *from, with w, and *from stepped past it. FMOV.S takes no
 * displacement, so the asm loads the three through *from by post-increment,
 * as model.src's loops do */
ALWAYS_INLINE draw_vec4 next_vector(const NJS_VECTOR **from, Float w)
{
    Float x, y, z;

#ifdef XBOX
    x = (*from)->x;
    y = (*from)->y;
    z = (*from)->z;
    (*from)++;
#else
    __asm__("fmov.s @%[from]+,%[x]\n\t"
            "fmov.s @%[from]+,%[y]\n\t"
            "fmov.s @%[from]+,%[z]"
            : [from] "+r"(*from), [x] "=f"(x), [y] "=f"(y), [z] "=f"(z)
            : "m"(**from));
#endif
    return draw_vec4_init(x, y, z, w);
}

/* PREF the line PREFETCH_AHEAD bytes past p. the address is an integer,
 * since it may lie past p's array. SH4ZAM's SHZ_PREFETCH clobbers no memory,
 * where machine.h's prefetch clobbers it for the store queues. but it is an
 * asm volatile, which GCC's scheduler moves no instruction across, so each
 * loop PREFs at the top of a pass, before its loads */
static inline void prefetch_ahead(const void *p)
{
#ifdef XBOX
    /* a prefetch is a hint, which the port leaves out */
    (void)p;
#else
    SHZ_PREFETCH((Uint32)p + PREFETCH_AHEAD);
#endif
}

/* the point at *from in view space, with *from stepped past it */
ALWAYS_INLINE draw_vec4 view_next(const NJS_POINT3 **from)
{
    return draw_xmtrx_vec4(next_vector(from, 1.0F));
}

/* write p's x and y on the screen and its 1/z into point, and return point.
 * FMOV.S takes no displacement, so the asm writes the three down from
 * point's color by pre-decrement, as model.src's project_points does. it
 * returns the register the asm leaves at point, so that the step to the next
 * point takes no register of its own */
static inline struct point *project_end(struct point *point,
                                        struct projecting p,
                                        const struct projection *by)
{
    Float *word = (Float *)&point->color;
    Float x = p.x * p.rhw + by->cx;
    Float y = p.y * p.rhw + by->cy;

#ifdef XBOX
    point->at.x = x;
    point->at.y = y;
    point->at.rhw = p.rhw;
    word = (Float *)point;
#else
    __asm__ volatile("fmov.s %[rhw],@-%[word]\n\t"
                     "fmov.s %[y],@-%[word]\n\t"
                     "fmov.s %[x],@-%[word]"
                     : [word] "+r"(word)
                     : [rhw] "f"(p.rhw), [x] "f"(x), [y] "f"(y)
                     : "memory");
#endif
    return (struct point *)word;
}

/* project model's points into points, and return how many lie no farther
 * than view.near. a point takes x * scale_x * (1/z) + cx,
 * y * scale_y * (1/z) + cy and 1/z. shade_points gives it its color.
 *
 * points must start on 32 bytes. each pass projects a pair of points, a and
 * b, into the 32-byte cache line they fill. MOVCA.L claims that line without
 * reading memory, and the pass writes the pair's six words there.
 * shade_points then writes the two colors, and reads the line from memory
 * only where the cache has dropped it since. were points not on 32 bytes,
 * MOVCA.L would claim a line that starts before the pair. the cache would
 * then write what it held there back over the bytes before the pair.
 * project_and_shade passes _nj_vertex_buf_, which system_boot_init's
 * njInit3D sets to texture_buffer, and src/main.c defines texture_buffer on
 * 32 bytes. for an odd count, the last line's second half keeps what the
 * cache held.
 *
 * a's nearness is counted as a is begun, so that no FR register holds a's z
 * into the next pass. the count takes no branch. a pass transforms the point
 * after its pair before it stores the pair. so each point's 1/z starts before
 * the next point's FTRV, and its words are stored after that FTRV, as in
 * model.src's loop. a pass PREFs once and reads 24 bytes of points, under a
 * line, so its PREFs skip no line */
OUT_OF_LINE int project_points(const NJS_MODEL *model, struct point *points)
{
    const struct projection by = {view.cx, view.cy, view.scale_x, view.scale_y,
                                  view.near};
    const NJS_POINT3 *from = model->points;
    const NJS_POINT3 *end = from + model->nbPoint;
    int near_points = 0;
    struct projecting a;
    int a_near;

    if (from == end) {
        return 0;
    }
    a = project_begin(view_next(&from), &by);
    a_near = !(a.z > by.near);
    while (end - from > 1) {
        draw_vec4 next;
        struct projecting b;

        prefetch_ahead(from);
        next = view_next(&from);
        b = project_begin(next, &by);
        near_points += a_near + !(b.z > by.near);
        next = view_next(&from);
        /* MOVCA.L R0,@points claims the line. SH4ZAM's asm passes no value
         * in R0, so a's x takes whatever R0 holds, until project_end writes
         * it. the line's other words hold what the cache held, not the zeros
         * SH4ZAM's doc says, until project_end and shade_points write them */
        draw_alloc_line(points);
        points = project_end(points, a, &by) + 1;
        points = project_end(points, b, &by) + 1;
        a = project_begin(next, &by);
        a_near = !(a.z > by.near);
    }
    /* the last line holds a, and for an even count the last point after it */
    near_points += a_near;
    draw_alloc_line(points);
    points = project_end(points, a, &by) + 1;
    if (from != end) {
        struct projecting b = project_begin(view_next(&from), &by);

        near_points += !(b.z > by.near);
        project_end(points, b, &by);
    }
    return near_points;
}

/* whether any of model's points lies farther than view.near, by the z
 * project_points gives it */
OUT_OF_LINE Bool any_past_near(const NJS_MODEL *model)
{
    const NJS_POINT3 *from = model->points;
    const NJS_POINT3 *end = from + model->nbPoint;

    while (from != end) {
        if (view_next(&from).z > view.near) {
            return TRUE;
        }
    }
    return FALSE;
}

/* entry, held to the palette: a normal's dot with the light picks
 * (int)(dot * 127.5 + 127.5), so -1 picks entry 0, and 1 the last */
static inline int palette_entry(int entry)
{
    if (entry < 0) {
        entry = 0;
    }
    if (entry > DRAW_PALETTE_ENTRIES - 1) {
        entry = DRAW_PALETTE_ENTRIES - 1;
    }
    return entry;
}

/* shade each of model's points from shade_palette. the dot of its normal
 * with light_dir, turned into the model's space, picks the entry. the
 * entry's base color goes to the point, and its offset color to lit_offsets.
 * the offset colors come from the next palette when first has a specular
 * color, and first_flags lack NJD_FLAG_IGNORE_SPECULAR. each normal loads
 * with a w of 1, which the light's w of 0 cancels in the dot.
 *
 * a pass shades four points, and holds their entries to the palette only
 * when one lies outside it. a pair left over holds both its entries without
 * that test */
OUT_OF_LINE void shade_points(const NJS_MODEL *model, struct point *points,
                              const NJS_MATERIAL *first, Uint32 first_flags)
{
    const NJS_VECTOR *normal = model->normals;
    const Uint32 *palette = shade_palette;
    const Uint32 *offsets = palette + 1;
    Uint32 *lit = lit_offsets;
    draw_vec4 dir = draw_vec4_init(light_dir.x, light_dir.y, light_dir.z, 0.0F);
    draw_vec4 light;
    int left;

    /* light_dir in the model's space: its dot with each of XMTRX's first
     * three columns */
    light = draw_in_model(dir);
    if (!(first_flags & NJD_FLAG_IGNORE_SPECULAR) &&
        first->specular.color != 0) {
        offsets += DRAW_PALETTE_ENTRIES * 2;
    }
    for (left = model->nbPoint; left > 3; left -= 4) {
        draw_vec4 n;
        draw_vec2 ab, cd;
        int a, b, c, d;

        /* a PREF for each pair of normals, 24 bytes, under a line, so that
         * the PREFs skip no line */
        prefetch_ahead(normal);
        prefetch_ahead(normal + 2);
        n = next_vector(&normal, 1.0F);
        ab = draw_dot2(light, n, next_vector(&normal, 1.0F));
        a = (int)(ab.x * HALF_255 + HALF_255);
        b = (int)(ab.y * HALF_255 + HALF_255);
        n = next_vector(&normal, 1.0F);
        cd = draw_dot2(light, n, next_vector(&normal, 1.0F));
        c = (int)(cd.x * HALF_255 + HALF_255);
        d = (int)(cd.y * HALF_255 + HALF_255);
        if ((Uint32)(a | b | c | d) > DRAW_PALETTE_ENTRIES - 1) {
            a = palette_entry(a);
            b = palette_entry(b);
            c = palette_entry(c);
            d = palette_entry(d);
        }
        points[0].color.color = palette[a * 2];
        lit[0] = offsets[a * 2];
        points[1].color.color = palette[b * 2];
        lit[1] = offsets[b * 2];
        points[2].color.color = palette[c * 2];
        lit[2] = offsets[c * 2];
        points[3].color.color = palette[d * 2];
        lit[3] = offsets[d * 2];
        points += 4;
        lit += 4;
    }
    if (left > 1) {
        draw_vec4 n = next_vector(&normal, 1.0F);
        draw_vec2 ab = draw_dot2(light, n, next_vector(&normal, 1.0F));
        int a = palette_entry((int)(ab.x * HALF_255 + HALF_255));
        int b = palette_entry((int)(ab.y * HALF_255 + HALF_255));

        points[0].color.color = palette[a * 2];
        lit[0] = offsets[a * 2];
        points[1].color.color = palette[b * 2];
        lit[1] = offsets[b * 2];
        points += 2;
        lit += 2;
        left -= 2;
    }
    if (left > 0) {
        draw_vec4 n = next_vector(&normal, 1.0F);
        int a = palette_entry((int)(draw_fipr(light, n) * HALF_255 + HALF_255));

        points[0].color.color = palette[a * 2];
        lit[0] = offsets[a * 2];
    }
}

/* set the alpha of each of count points to material's, as alpha_mode takes
 * it. under ALPHA_OFFSET the sum is held to 0 to 1. under
 * NJD_FLAG_IGNORE_LIGHT in flags, the points turn white too. nothing is
 * written when the first point already holds what it would be given */
OUT_OF_LINE void set_point_alpha(struct point *points, int count,
                                 const NJS_MATERIAL *material, Uint32 flags)
{
    Float alpha = (Float)material->diffuse.argb.a * INV_255;
    int value;
    int i;

    if (alpha_mode == ALPHA_CONSTANT) {
        alpha = _nj_constant_material_.a;
    } else if (alpha_mode == ALPHA_OFFSET) {
        alpha += _nj_constant_material_.a;
        if (alpha > 1.0F) {
            alpha = 1.0F;
        }
        if (!(alpha > 0.0F)) {
            alpha = 0.0F;
        }
    }
    value = (int)(alpha * 255.0F);
    if (flags & NJD_FLAG_IGNORE_LIGHT) {
        Uint32 white = (Uint32)value << 24 | 0x00ffffff;

        if (points[0].color.color == white) {
            return;
        }
        for (i = 0; i < count; i++) {
            points[i].color.color = white;
        }
        return;
    }
    /* @bug logic: the first point's alpha compares with value as a signed
     * byte, so a value of 128 or more never matches. every point then takes
     * it again. model.src's MOV.B @R1,R8 at 0x8c0388e2 sign-extends it for
     * CMP/EQ at 0x8c0388e4 */
    if ((Sint8)points[0].color.argb.a == value) {
        return;
    }
    for (i = 0; i < count; i++) {
        points[i].color.argb.a = (Uint8)value;
    }
}

/* make texture n of _nj_current_texlist current, unless its globalIndex is
 * _nj_current_texture already: its TSP_UV_SIZE bits go to the TSP word, and
 * its texture word to the header's. unlike njSetkmTextureNum, it reads no
 * bank and checks no address */
OUT_OF_LINE void set_texture(Uint32 n)
{
    const NJS_TEXMEMLIST *texture =
        (const NJS_TEXMEMLIST *)_nj_current_texlist->textures[n].texaddr;

    if (texture->globalIndex == _nj_current_texture) {
        return;
    }
    _nj_current_texture = texture->globalIndex;
    _nj_parameterkm_.TSPPARAMBUFFER =
        (_nj_parameterkm_.TSPPARAMBUFFER & ~TSP_UV_SIZE) |
        (texture->tspparambuffer & TSP_UV_SIZE);
    _nj_parameterkm_.TexturePARAMBUFFER = texture->texparambuffer;
}

/* replace the global word's GLOBAL_TRANSLUCENT bit and object control byte
 * with mode's GLOBAL_MODE bits, and the TSP word's TSP_MODE bits with
 * mode's */
static inline void set_mode(Uint32 mode)
{
    _nj_parameterkm_.GLOBALPARAMBUFFER =
        (_nj_parameterkm_.GLOBALPARAMBUFFER &
         ~(GLOBAL_TRANSLUCENT | GLOBAL_OBJECT_CONTROL)) |
        (mode & GLOBAL_MODE);
    _nj_parameterkm_.TSPPARAMBUFFER =
        (_nj_parameterkm_.TSPPARAMBUFFER & ~TSP_MODE) | (mode & TSP_MODE);
}

/* set the TSP word's shading back to MODULATE_ALPHA, which set_mode may have
 * cleared */
static inline void reset_shading(void)
{
    _nj_parameterkm_.TSPPARAMBUFFER |= MODULATE_ALPHA;
}

/* x, y, 1/z and the base and offset colors of a textured vertex, each in an
 * FR register as model.src's strip loops hold them. a color's Float holds
 * its word's bits, and nothing here does arithmetic on it */
struct vertex_words {
    Float x, y, rhw;
    Float base, offset;
};

/* the words of point index of strips, loaded as model.src's strip loops load
 * them. "z" puts index in R0, the only register FMOV.S @(R0,Rn) indexes by.
 * index times 4 reaches the offset color, and index times 16 the point,
 * whose words load by post-increment. the asm reads strips->offsets[index]
 * and strips->points[index], which no operand names, so it clobbers memory */
ALWAYS_INLINE struct vertex_words take_point(const struct strips *strips,
                                             int index)
{
    struct vertex_words p;

#ifdef XBOX
    {
        const Float *words = (const Float *)&strips->points[index];

        p.x = words[0];
        p.y = words[1];
        p.rhw = words[2];
        p.base = words[3];
        p.offset = ((const Float *)strips->offsets)[index];
    }
#else
    __asm__("shll2 %[k]\n\t"
            "fmov.s @(%[k],%[offsets]),%[offset]\n\t"
            "shll2 %[k]\n\t"
            "add %[points],%[k]\n\t"
            "fmov.s @%[k]+,%[x]\n\t"
            "fmov.s @%[k]+,%[y]\n\t"
            "fmov.s @%[k]+,%[rhw]\n\t"
            "fmov.s @%[k]+,%[base]"
            : [k] "+z"(index), [x] "=f"(p.x), [y] "=f"(p.y), [rhw] "=f"(p.rhw),
              [base] "=f"(p.base), [offset] "=f"(p.offset)
            : [offsets] "r"(strips->offsets), [points] "r"(strips->points)
            : "memory");
#endif
    return p;
}

/* write an untextured vertex at sq, send it, and return the address past it.
 * it leaves u, v and offset unwritten. machine.h's prefetch sends it, and
 * its asm clobbers memory, so no store moves past its PREF. SH4ZAM's
 * SHZ_PREFETCH clobbers none */
static inline struct vertex *send_untextured(struct vertex *sq, Uint32 control,
                                             const struct screen *at,
                                             Uint32 base)
{
    sq->base = base;
    sq->at = *at;
    sq->control = control;
    prefetch(sq);
    return sq + 1;
}

/* write a textured vertex at sq from p, u and v, send it, and return the
 * address past it. FMOV.S takes no displacement, so the asm writes the words
 * down from the vertex's end, as model.src's strips_tex does, and its PREF
 * sends the queue. it names no memory: it writes only the store queue area,
 * which no C reads */
ALWAYS_INLINE struct vertex *send_textured(struct vertex *sq, Uint32 control,
                                           struct vertex_words p, Float u,
                                           Float v)
{
#ifdef XBOX
    {
        Float *words = (Float *)sq;

        sq->control = control;
        words[1] = p.x;
        words[2] = p.y;
        words[3] = p.rhw;
        words[4] = u;
        words[5] = v;
        words[6] = p.base;
        words[7] = p.offset;
    }
#else
    __asm__ volatile(
        "add #32,%[sq]\n\t"
        "fmov.s %[offset],@-%[sq]\n\t"
        "fmov.s %[base],@-%[sq]\n\t"
        "fmov.s %[v],@-%[sq]\n\t"
        "fmov.s %[u],@-%[sq]\n\t"
        "fmov.s %[rhw],@-%[sq]\n\t"
        "fmov.s %[y],@-%[sq]\n\t"
        "fmov.s %[x],@-%[sq]\n\t"
        "mov.l %[control],@-%[sq]\n\t"
        "pref @%[sq]"
        : [sq] "+r"(sq)
        : [control] "r"(control), [x] "f"(p.x), [y] "f"(p.y), [rhw] "f"(p.rhw),
          [u] "f"(u), [v] "f"(v), [base] "f"(p.base), [offset] "f"(p.offset));
#endif
    return sq + 1;
}

/* the u and v of an ENV_UV vertex of point index: its normal, turned by
 * XMTRX, as n * 0.5 + 0.5 */
static inline void env_uv(const struct strips *strips, int index, Float *u,
                          Float *v)
{
    const NJS_VECTOR *n = &strips->normals[index];
    draw_vec4 turned = draw_xmtrx_vec4(next_vector(&n, 0.0F));

    *u = turned.x * 0.5F + 0.5F;
    *v = turned.y * 0.5F + 0.5F;
}

/* send point index of strips, with control. a MESH_UV vertex takes the u
 * and v of vertuv[uv_index] over 255, and no other reads vertuv */
ALWAYS_INLINE struct vertex *send_point(struct vertex *sq, Uint32 control,
                                        const struct strips *strips, enum uv uv,
                                        int index, const NJS_COLOR *vertuv,
                                        int uv_index)
{
    const struct point *point = &strips->points[index];
    Float u, v;

    if (uv == NO_UV) {
        return send_untextured(sq, control, &point->at, point->color.color);
    }
    if (uv == MESH_UV) {
        /* u and v come in one 32-bit load, u in its low half. a 16-bit load
         * by displacement writes only R0, which take_point holds for its
         * index */
        Uint32 pair = vertuv[uv_index].color;

        u = (Float)(Sint16)pair * INV_255;
        v = (Float)(Sint16)(pair >> 16) * INV_255;
    } else {
        env_uv(strips, index, &u, &v);
    }
    return send_textured(sq, control, take_point(strips, index), u, v);
}

/* the index at *mesh, and *mesh stepped past it. the asm loads it through
 * *mesh by post-increment, as model.src's strip loops do. it clobbers memory
 * rather than take **mesh as an "m" input, with which GCC copies *mesh into
 * another register around each load */
static inline int next_index(const Sint16 **mesh)
{
    int index;

#ifdef XBOX
    index = *(*mesh)++;
#else
    __asm__("mov.w @%[mesh]+,%[index]"
            : [mesh] "+r"(*mesh), [index] "=r"(index)
            :
            : "memory");
#endif
    return index;
}

/* the u and v at *vertuv over 255, and *vertuv stepped past them. a 16-bit
 * load by displacement writes only R0, which take_point holds for its index.
 * so the asm loads the two by post-increment, as model.src's strips_tex does.
 * it clobbers memory for the reason next_index gives */
static inline void next_uv(const NJS_TEX **vertuv, Float *u, Float *v)
{
    int tex_u, tex_v;

#ifdef XBOX
    tex_u = ((const Sint16 *)*vertuv)[0];
    tex_v = ((const Sint16 *)*vertuv)[1];
    (*vertuv)++;
#else
    __asm__("mov.w @%[vertuv]+,%[u]\n\t"
            "mov.w @%[vertuv]+,%[v]"
            : [vertuv] "+r"(*vertuv), [u] "=r"(tex_u), [v] "=r"(tex_v)
            :
            : "memory");
#endif
    *u = (Float)tex_u * INV_255;
    *v = (Float)tex_v * INV_255;
}

/* send the point at *mesh of strips, with control, and step *mesh past it. a
 * MESH_UV vertex takes the u and v at *vertuv over 255, and steps *vertuv
 * past them */
static inline struct vertex *send_next(struct vertex *sq, Uint32 control,
                                       const struct strips *strips, enum uv uv,
                                       const Sint16 **mesh,
                                       const NJS_TEX **vertuv)
{
    int index = next_index(mesh);
    Float u, v;

    if (uv != MESH_UV) {
        return send_point(sq, control, strips, uv, index, NULL, 0);
    }
    next_uv(vertuv, &u, &v);
    return send_textured(sq, control, take_point(strips, index), u, v);
}

/* the point count of the polygon at *mesh, with *mesh stepped past the count
 * where the polygon gives its own */
static inline int next_polygon_size(const struct strips *strips,
                                    const Sint16 **mesh)
{
    return strips->polygon_size != 0 ? strips->polygon_size
                                     : *(*mesh)++ & STRIP_COUNT;
}

/* PREF ahead of a polygon's indices at mesh and, for MESH_UV, its uvs at
 * vertuv. a polygon that reads more than a line of either leaves a line
 * without a PREF */
static inline void prefetch_polygon(const Sint16 *mesh, enum uv uv,
                                    const void *vertuv)
{
    prefetch_ahead(mesh);
    if (uv == MESH_UV) {
        prefetch_ahead(vertuv);
    }
}

/* send each of from's polygons as a strip, and return the address past them.
 * from is copied into a local, which no store to the store queues can alias.
 * a polygon must have two points or more: with fewer, its loop runs on past
 * its points, as model.src's DT loop does. every polygon of the game's models
 * has three or more */
ALWAYS_INLINE struct vertex *send_strips(struct vertex *sq,
                                         const struct strips *from, enum uv uv)
{
    const struct strips copy = *from;
    const struct strips *strips = &copy;
    const Sint16 *mesh = strips->meshset->meshes;
    const NJS_TEX *vertuv =
        uv == MESH_UV ? &strips->meshset->vertuv->tex : NULL;
    int polygons;

    for (polygons = strips->meshset->nbMesh; polygons != 0; polygons--) {
        int left;

        prefetch_polygon(mesh, uv, vertuv);
        left = next_polygon_size(strips, &mesh) - 1;
        do {
            sq = send_next(sq, KM_VERTEXPARAM_NORMAL, strips, uv, &mesh,
                           &vertuv);
        } while (--left != 0);
        sq = send_next(sq, KM_VERTEXPARAM_ENDOFSTRIP, strips, uv, &mesh,
                       &vertuv);
    }
    return sq;
}

/* set corner from point index of strips, with its x, y and z taken back into
 * view space. a MESH_UV corner takes vertuv's u and v, and an ENV_UV corner
 * its point's normal, turned by XMTRX, as n * 127.5 + 127.5 */
static inline void take_corner(struct corner *corner,
                               const struct strips *strips, enum uv uv,
                               int index, const NJS_COLOR *vertuv)
{
    const struct point *point = &strips->points[index];

    corner->at = point->at;
    corner->base = point->color.color;
    corner->offset = strips->offsets[index];
    corner->z = 1.0F / point->at.rhw;
    corner->x = (point->at.x - view.cx) * corner->z * view.inv_scale_x;
    corner->y = (point->at.y - view.cy) * corner->z * view.inv_scale_y;
    if (uv == MESH_UV) {
        corner->u = (Float)vertuv->tex.u;
        corner->v = (Float)vertuv->tex.v;
    } else if (uv == ENV_UV) {
        draw_vec3 turned = draw_xmtrx_vec3(&strips->normals[index]);

        corner->u = turned.x * HALF_255 + HALF_255;
        corner->v = turned.y * HALF_255 + HALF_255;
    }
}

/* a with each 8-bit channel moved toward b's by t, a fraction in 16.16 fixed
 * point. a channel moves by bits 16 to 31 of (b - a) * t, read as a signed
 * halfword. model.src's DMULS.L, SWAP.W and EXTS.W read them so */
static inline Uint32 blend(Uint32 a, Uint32 b, Sint32 t)
{
    Uint32 color = 0;
    int shift;

    for (shift = 0; shift < 32; shift += 8) {
        Sint32 from = a >> shift & 0xff;
        Sint32 to = b >> shift & 0xff;
        Sint32 moved = (Sint16)((Uint32)((to - from) * t) >> 16);

        color |= (Uint32)(from + moved) << shift;
    }
    return color;
}

/* set cut to the point where the edge from a to b crosses z = 1, projected
 * by scale_x and scale_y. its colors blend a's toward b's by t in 16.16
 * fixed point, and an untextured cut takes no u or v */
static void cut_edge(struct corner *cut, const struct corner *a,
                     const struct corner *b, enum uv uv)
{
    Float t = (1.0F - a->z) / (b->z - a->z);
    Sint32 fraction = (Sint32)(t * 65536.0F);
    Float x = (b->x - a->x) * t + a->x;
    Float y = (b->y - a->y) * t + a->y;

    cut->at.x = x * view.scale_x + view.cx;
    cut->at.y = y * view.scale_y + view.cy;
    cut->at.rhw = 1.0F;
    cut->base = blend(a->base, b->base, fraction);
    cut->offset = blend(a->offset, b->offset, fraction);
    if (uv != NO_UV) {
        cut->u = (b->u - a->u) * t + a->u;
        cut->v = (b->v - a->v) * t + a->v;
    }
}

/* the bits of word in an FR register */
static inline Float word_bits(Uint32 word)
{
    Float bits;

    __builtin_memcpy(&bits, &word, sizeof bits);
    return bits;
}

/* send corner, with control, its u and v scaled by INV_255 */
static inline struct vertex *send_corner(struct vertex *sq, Uint32 control,
                                         enum uv uv,
                                         const struct corner *corner)
{
    struct vertex_words p;

    if (uv == NO_UV) {
        return send_untextured(sq, control, &corner->at, corner->base);
    }
    p.x = corner->at.x;
    p.y = corner->at.y;
    p.rhw = corner->at.rhw;
    p.base = word_bits(corner->base);
    p.offset = word_bits(corner->offset);
    return send_textured(sq, control, p, corner->u * INV_255,
                         corner->v * INV_255);
}

/* clip the triangle of points index[0] to index[2] of strips at z = 1, and
 * send what is left as a strip of its own. vertuv holds the points' u and v
 * for MESH_UV. a corner at z = 1 exactly isn't kept: the cut on its edge from
 * a corner nearer than z = 1 stands for it. nothing is sent when no corner
 * lies past z = 1. nor is anything sent when fewer than three corners are
 * left, which only a NaN or infinite z leaves */
static struct vertex *near_clip(struct vertex *sq, const struct strips *strips,
                                enum uv uv, const Sint16 *index,
                                const NJS_COLOR *vertuv)
{
    struct corner corners[3];
    struct corner polygon[4];
    const struct corner *before = &corners[2];
    int count = 0;
    int i;

    for (i = 0; i < 3; i++) {
        take_corner(&corners[i], strips, uv, index[i],
                    uv == MESH_UV ? &vertuv[i] : NULL);
    }
    if (!(corners[0].z > 1.0F) && !(corners[1].z > 1.0F) &&
        !(corners[2].z > 1.0F)) {
        return sq;
    }
    /* each edge, from the corner before: a cut lies on it when one end is
     * nearer than z = 1 and the other isn't, and the corner stays when it
     * lies past z = 1 */
    for (i = 0; i < 3; i++) {
        const struct corner *corner = &corners[i];

        if ((1.0F > before->z) != (1.0F > corner->z)) {
            cut_edge(&polygon[count++], before, corner, uv);
        }
        if (corner->z > 1.0F) {
            polygon[count++] = *corner;
        }
        before = corner;
    }
    if (count < 3) {
        return sq;
    }
    /* a quad's corners go 0, 1, 3, 2, the order in which a strip covers it */
    sq = send_corner(sq, KM_VERTEXPARAM_NORMAL, uv, &polygon[0]);
    sq = send_corner(sq, KM_VERTEXPARAM_NORMAL, uv, &polygon[1]);
    if (count == 4) {
        sq = send_corner(sq, KM_VERTEXPARAM_NORMAL, uv, &polygon[3]);
    }
    return send_corner(sq, KM_VERTEXPARAM_ENDOFSTRIP, uv, &polygon[2]);
}

/* the flag of point index of strips, as a triangle's a */
static inline int point_flag(const struct strips *strips, int index)
{
    Float rhw = strips->points[index].at.rhw;

    return rhw > 0.0F && !(rhw > 1.0F) ? 0 : FLAGGED_A;
}

/* the edges of the screen p lies past, as OFF_ bits. for the point
 * take_corner rebuilds from p, at z = 1 / p->rhw and
 * x = (p->x - cx) * z / scale_x, (p->x + OFF_EDGE) * p->rhw is exactly
 * (scale_x * x + (cx + OFF_EDGE) * z) / z^2: the point's side of a plane
 * through the eye OFF_EDGE pixels out from the left edge, at either sign of
 * z. rounding toward zero keeps its sign or gives 0, past no edge, and the
 * other edges follow. p lies past none beyond OFF_REACH, or where a word of
 * it is a NaN or infinite. view.w in the reach test bounds p's depth */
static inline int off_edges(const struct screen *p)
{
    Float rhw = p->rhw;
    int edges = 0;

    if (!(__builtin_fabsf(p->x - view.cx) + __builtin_fabsf(p->y - view.cy) +
              view.w <
          OFF_REACH * __builtin_fabsf(rhw))) {
        return 0;
    }
    if ((p->x + OFF_EDGE) * rhw < 0.0F) {
        edges |= OFF_LEFT;
    }
    if ((view.w + OFF_EDGE - p->x) * rhw < 0.0F) {
        edges |= OFF_RIGHT;
    }
    if ((p->y + OFF_EDGE) * rhw < 0.0F) {
        edges |= OFF_TOP;
    }
    if ((view.h + OFF_EDGE - p->y) * rhw < 0.0F) {
        edges |= OFF_BOTTOM;
    }
    return edges;
}

/* whether the corners of the triangle of points index[0] to index[2] of
 * strips all lie past one edge of the screen, by off_edges. the whole
 * triangle then lies past that edge's plane, and the part near_clip would
 * keep, at z >= 1, projects past the edge */
static Bool triangle_off_screen(const struct strips *strips,
                                const Sint16 *index)
{
    const struct point *points = strips->points;
    int edges = off_edges(&points[index[0]].at);

    if (edges != 0) {
        edges &= off_edges(&points[index[1]].at);
    }
    if (edges != 0) {
        edges &= off_edges(&points[index[2]].at);
    }
    return edges != 0;
}

/* send strips as send_strips does, for a model with near points. a polygon
 * of n points goes as n - 2 triangles, whose a, b and c are its points i to
 * i + 2. a triangle with no point flagged goes on in the strip being sent.
 * one with a point flagged ends that strip at b, and near_clip clips it. it
 * is dropped instead when all three are flagged, or when it lies off the
 * screen by triangle_off_screen */
ALWAYS_INLINE struct vertex *
send_clipped_strips(struct vertex *sq, const struct strips *from, enum uv uv)
{
    const struct strips copy = *from;
    const struct strips *strips = &copy;
    const Sint16 *mesh = strips->meshset->meshes;
    const NJS_COLOR *vertuv = uv == MESH_UV ? strips->meshset->vertuv : NULL;
    int polygons;

    for (polygons = strips->meshset->nbMesh; polygons != 0; polygons--) {
        Bool in_strip = FALSE;
        int size, flags;
        int i;

        prefetch_polygon(mesh, uv, vertuv);
        size = next_polygon_size(strips, &mesh);
        flags =
            point_flag(strips, mesh[0]) | (point_flag(strips, mesh[1]) << 1);
        for (i = 0; i + 2 < size; i++) {
            flags |= point_flag(strips, mesh[i + 2]) << 2;
            if (flags == 0) {
                if (!in_strip) {
                    sq = send_point(sq, KM_VERTEXPARAM_NORMAL, strips, uv,
                                    mesh[i], vertuv, i);
                    in_strip = TRUE;
                }
                sq = send_point(sq, KM_VERTEXPARAM_NORMAL, strips, uv,
                                mesh[i + 1], vertuv, i + 1);
            } else {
                if (in_strip) {
                    sq = send_point(sq, KM_VERTEXPARAM_ENDOFSTRIP, strips, uv,
                                    mesh[i + 1], vertuv, i + 1);
                    in_strip = FALSE;
                }
                if (flags != FLAGGED_ALL &&
                    !triangle_off_screen(strips, &mesh[i])) {
                    sq = near_clip(sq, strips, uv, &mesh[i],
                                   uv == MESH_UV ? &vertuv[i] : NULL);
                }
            }
            /* b and c become the next triangle's a and b */
            flags >>= 1;
        }
        if (in_strip) {
            sq = send_point(sq, KM_VERTEXPARAM_ENDOFSTRIP, strips, uv,
                            mesh[size - 1], vertuv, size - 1);
        }
        mesh += size;
        if (uv == MESH_UV) {
            vertuv += size;
        }
    }
    return sq;
}

OUT_OF_LINE struct vertex *strips_untextured(struct vertex *sq,
                                             const struct strips *strips)
{
    return send_strips(sq, strips, NO_UV);
}

OUT_OF_LINE struct vertex *strips_textured(struct vertex *sq,
                                           const struct strips *strips)
{
    return send_strips(sq, strips, MESH_UV);
}

OUT_OF_LINE struct vertex *strips_env(struct vertex *sq,
                                      const struct strips *strips)
{
    return send_strips(sq, strips, ENV_UV);
}

OUT_OF_LINE struct vertex *clipped_untextured(struct vertex *sq,
                                              const struct strips *strips)
{
    return send_clipped_strips(sq, strips, NO_UV);
}

OUT_OF_LINE struct vertex *clipped_textured(struct vertex *sq,
                                            const struct strips *strips)
{
    return send_clipped_strips(sq, strips, MESH_UV);
}

OUT_OF_LINE struct vertex *clipped_env(struct vertex *sq,
                                       const struct strips *strips)
{
    return send_clipped_strips(sq, strips, ENV_UV);
}

/* set the polygon header's words from drawn, material's flags as drawn: the
 * TSP bits the flags hold and the mode they pick. for a textured material,
 * it also makes the material's texture current through set_texture */
static inline void set_material_header(const NJS_MATERIAL *material,
                                       Uint32 drawn)
{
    njSetkmModelAttr(drawn);
    if (drawn & NJD_FLAG_USE_TEXTURE) {
        set_texture(material->attr_texId & MATERIAL_TEXTURE);
        if (drawn & NJD_FLAG_USE_ALPHA) {
            set_mode(MODE_TRANS_TEXTURED);
        } else {
            set_mode(drawn & NJD_FLAG_IGNORE_LIGHT ? MODE_DECAL
                                                   : MODE_OPAQUE_TEXTURED);
        }
    } else {
        set_mode(drawn & NJD_FLAG_USE_ALPHA ? COLOR_MODE_TRANS
                                            : COLOR_MODE_OPAQUE);
    }
}

/* draw each of model's meshsets: set the polygon header from its material,
 * open a polygon, and send the meshset's polygons as strips. the strips go
 * through the near clip where clipped is TRUE, as it is for a model with
 * near points. the header takes the material's flags as drawn, and the
 * strips take its own, as model.src's do. NJD_FLAG_USE_ALPHA sends a
 * meshset to the translucent list, after set_point_alpha. a textured
 * meshset under NJD_FLAG_IGNORE_LIGHT sends draw_unlit_offsets as its offset
 * colors, and shades as a decal when opaque */
#ifdef XBOX
/* the port's profile (xbox/port/port.h): the vertices the drawer sends, a
 * meshset's from its store queue pointer, and the models it draws, with
 * whether their points changed since they were last drawn */
struct port_counts {
    unsigned drawer_models, drawer_points, drawer_vertices;
    unsigned changed_models, changed_points;
    unsigned ninja_vertices;
    unsigned packed_loads, copied_loads;
    unsigned gpu_vertices;
    unsigned kept_vertices, built_vertices;
    unsigned long long drawer_tsc, render_tsc;
};
#define PORT_TSC_BEGIN unsigned long long port_tsc = port_profile ? __builtin_ia32_rdtsc() : 0
#define PORT_TSC_END()                                                    \
    do {                                                                  \
        if (port_profile) {                                               \
            port_counts.drawer_tsc += __builtin_ia32_rdtsc() - port_tsc; \
        }                                                                 \
    } while (0)
extern struct port_counts port_counts;
extern volatile int port_profile;
void port_count_model(const void *model, const void *points, int count);
static void *port_sq_start;

/* the port's models for the host to transform (host_api's video_vertices3).
 * a model the drawer needn't clip, one wholly past the near plane and within
 * GPU_GUARD pixels of the screen by its sphere of LAND_REACH times r, isn't
 * projected, and its opaque meshsets don't go through the lists: each is a
 * run of the renderer's (xbox/port/render.c's port_run_begin), the vertices
 * the host draws, a point of the model's where x, y and 1/z would be, with
 * the uv and the colors a strip's vertex takes, and its triangles' indices.
 * the list takes the meshset's header, as ever, and one parameter that
 * names the run, for the list's order. the run's transform is XMTRX with
 * the projection folded in. the points are shaded as ever, so the colors
 * are the palette's. a meshset under NJD_FLAG_USE_ALPHA is a run too, unless
 * port_gpu_models lacks bit 1: the translucent list is sorted by each
 * strip's nearest vertex, and so is such a run's each strip, a part of the
 * run with the 1/z of its nearest vertex, which the drawer finds by the
 * transform's third row. (sorted as one, at the model's center, Emerald
 * Coast's water, one meshset, drew out of order with what lies in it, and
 * left dark squares around Sonic and the sprites there, on a real Xbox.)
 * without the bit such a meshset is projected and goes through its list,
 * as before.
 * the sphere is the model's own for draw_model's and draw_model_nested's
 * models, which model_in_view culls by it, and LAND_REACH times it for
 * draw_model_unculled's, the land's */
extern volatile int port_gpu_models;
/* how far past the screen a model's sphere may reach: less across, where
 * the Xbox's antialiased surface doubles x */
#define GPU_GUARD_X 256.0F
#define GPU_GUARD_Y 512.0F
#define PARA_MODEL 0xc0000000u

/* the renderer's vertex, xbox/port/host.h's host_vertex */
struct run_vertex {
    Float x, y, z;
    Float u, v;
    Uint32 base, offset;
};
int port_run_begin(struct run_vertex **vertices, int *vertex_room, Uint16 **indices, int *index_room);
/* a strip of a translucent run, the renderer's run_part: its indices in the
 * run's, and the 1/z of its nearest vertex */
struct run_part {
    int first_index, index_count;
    Float rhw_near;
};
struct run_part *port_run_parts(int *room);
extern unsigned port_run_unfit;
extern unsigned port_run_unfit_vertices;
void port_run_end(int run, int vertices, int indices, int part_total, const Float transform[12], Float rhw_near,
                  Float rhw_mid, unsigned serial);
void port_run_lit(int run, const Float light[3], const Uint32 *palette, const Uint32 *offsets);

/* the model's sphere in view space, by the tests above: whether the host
 * can take the model, and the largest 1/z and the center's */
static Bool gpu_takes(const NJS_MODEL *model, Float reach_scale, Float *rhw_near, Float *rhw_mid)
{
    draw_vec3 c;
    Float reach;
    Float inv;

    if (!port_gpu_models || model->nbPoint > DRAW_MOST_POINTS) {
        return FALSE;
    }
    c = view_point(&model->center);
    reach = reach_scale * model->r;
    if (!(c.z - reach > view.near)) {
        return FALSE;
    }
    inv = 1.0F / (c.z - reach);
    if (!((c.x - reach) * view.scale_x * inv + view.cx > -GPU_GUARD_X) ||
        !((c.x + reach) * view.scale_x * inv + view.cx < view.w + GPU_GUARD_X) ||
        !((c.y - reach) * view.scale_y * inv + view.cy > -GPU_GUARD_Y) ||
        !((c.y + reach) * view.scale_y * inv + view.cy < view.h + GPU_GUARD_Y)) {
        return FALSE;
    }
    *rhw_near = inv;
    *rhw_mid = 1.0F / c.z;
    return TRUE;
}

/* the rows of XMTRX that give a point's x * scale_x + z * cx, its
 * y * scale_y + z * cy and its z in view space, which over z are where
 * project_points puts it */
static void gpu_transform(Float rows[12])
{
    /* XMTRX's columns, which are where it puts the axes and the origin */
    draw_vec4 ex = draw_xmtrx_col(0);
    draw_vec4 ey = draw_xmtrx_col(1);
    draw_vec4 ez = draw_xmtrx_col(2);
    draw_vec4 t = draw_xmtrx_col(3);

    rows[0] = ex.x * view.scale_x + ex.z * view.cx;
    rows[1] = ey.x * view.scale_x + ey.z * view.cx;
    rows[2] = ez.x * view.scale_x + ez.z * view.cx;
    rows[3] = t.x * view.scale_x + t.z * view.cx;
    rows[4] = ex.y * view.scale_y + ex.z * view.cy;
    rows[5] = ey.y * view.scale_y + ey.z * view.cy;
    rows[6] = ez.y * view.scale_y + ez.z * view.cy;
    rows[7] = t.y * view.scale_y + t.z * view.cy;
    rows[8] = ex.z;
    rows[9] = ey.z;
    rows[10] = ez.z;
    rows[11] = t.z;
}

/* a meshset's polygons into a run, each a strip, as send_strips sends them:
 * a vertex is its point, as the model has it, the uv send_point gives it,
 * the base color shade_points left in points and its offset color, 0 for an
 * untextured one. a strip's triangles all turn as its first. it returns the
 * vertices, and *index_count the indices; a polygon the room can't hold
 * ends the run */
static Uint32 keep_cmp(const NJS_VECTOR *n, Float scale);

static int gpu_send(const NJS_MODEL *model, const struct strips *strips, enum uv uv, struct run_vertex *out,
                    int vertex_room, Uint16 *indices, int index_room, int *index_count, const Float *depth_row,
                    struct run_part *run_parts, int part_room, int *part_total, const Float *lit_scale)
{
    const NJS_POINT3 *from = model->points;
    const Sint16 *mesh = strips->meshset->meshes;
    const NJS_TEX *vertuv = uv == MESH_UV ? &strips->meshset->vertuv->tex : NULL;
    int sent = 0;
    int named = 0;
    int made = 0;
    int polygons;

    for (polygons = strips->meshset->nbMesh; polygons != 0; polygons--) {
        int size = next_polygon_size(strips, &mesh);
        Float nearest = 1.0e30F;
        int k;

        if (size < 3 || sent + size > vertex_room || named + (size - 2) * 3 > index_room ||
            (depth_row != NULL && made >= part_room)) {
            break;
        }
        for (k = 0; k < size; k++) {
            int index = *mesh++;
            struct run_vertex *v = &out[sent + k];

            v->x = from[index].x;
            v->y = from[index].y;
            v->z = from[index].z;
            if (depth_row != NULL) {
                /* the vertex's depth in view space, for the nearest of
                 * them, which the translucent list sorts the run by */
                Float z = depth_row[0] * from[index].x + depth_row[1] * from[index].y +
                          depth_row[2] * from[index].z + depth_row[3];

                if (z < nearest) {
                    nearest = z;
                }
            }
            v->u = 0.0F;
            v->v = 0.0F;
            if (uv == MESH_UV) {
                next_uv(&vertuv, &v->u, &v->v);
            } else if (uv == ENV_UV) {
                env_uv(strips, index, &v->u, &v->v);
            }
            v->base = strips->points[index].color.color;
            v->offset = uv == NO_UV ? 0 : strips->offsets[index];
            if (lit_scale != NULL) {
                /* lit by the host (port_run_lit): the base color's alpha,
                 * and the point's normal, over the model's scale */
                v->base &= 0xff000000u;
                v->offset = keep_cmp(&model->normals[index], *lit_scale);
            }
            if (k >= 2) {
                indices[named++] = (Uint16)(sent + k - 2 + (k & 1));
                indices[named++] = (Uint16)(sent + k - 1 - (k & 1));
                indices[named++] = (Uint16)(sent + k);
            }
        }
        if (depth_row != NULL) {
            /* the strip is a part, sorted by its nearest vertex */
            run_parts[made].first_index = named - (size - 2) * 3;
            run_parts[made].index_count = (size - 2) * 3;
            run_parts[made].rhw_near = nearest > 0.0F ? 1.0F / nearest : 1.0e30F;
            made++;
        }
        sent += size;
    }
    *index_count = named;
    *part_total = made;
    return sent;
}

/* whether all of a meshset's polygons fit the room gpu_send has: the
 * vertices, the indices and, where part_room isn't negative, a part a
 * strip. gpu_send ends the run at a polygon the room can't hold, so a
 * meshset that doesn't fit whole would draw in part, the rest left out,
 * and whole the next frame. because that flickered where a frame filled the
 * arena, such a meshset goes through its list, whole. a polygon of fewer than
 * three points ends gpu_send's run, so it ends the count too */
static Bool gpu_fits(const struct strips *strips, int vertex_room, int index_room, int part_room, int *wanted)
{
    const Sint16 *mesh = strips->meshset->meshes;
    int vertices = 0;
    int indices = 0;
    int parts = 0;
    int polygons;

    for (polygons = strips->meshset->nbMesh; polygons != 0; polygons--) {
        int size = next_polygon_size(strips, &mesh);

        if (size < 3) {
            break;
        }
        mesh += size;
        vertices += size;
        indices += (size - 2) * 3;
        parts++;
    }
    *wanted = vertices;
    return vertices <= vertex_room && indices <= index_room && (part_room < 0 || parts <= part_room);
}

/* the kept models (xbox/port/render.c): a model that is as it was when it
 * was last drawn, under the same light and palette, keeps the vertices its
 * runs were given, in the host's store, and is neither shaded nor written
 * again: its meshsets are runs of those vertices. what a model's vertices
 * come from, and so what the drawer compares:
 * - the light in the model's space, within KEEP_LIGHT, a fifth of a palette
 *   entry's step: the land's is the same from frame to frame but for
 *   rounding, as the camera moves;
 * - a signature: the palette's colors, which of them the offsets are, the
 *   material mode, and each meshset's flags as drawn, its own, and its
 *   material's alpha;
 * - the model's data: its points and normals and each meshset's polygons
 *   and uvs. a sum over all of it is compared one draw in KEEP_FULL, and a
 *   sum over a few words of each (the first, the last, some between) at the
 *   others, which finds a model the game moves or scrolls as a whole.
 * a model under another light than its records were built by takes one
 * more record, up to KEEP_VARIANTS of them: a model placed many times,
 * turned differently, is under another light at each place. one that
 * turns leaves its record behind, unused, and goes as before: it takes
 * another only every KEEP_RECENT frames. a model whose signature or data changed within
 * KEEP_RECENT frames of its build isn't kept for a while, longer each time:
 * one that morphs, or whose uvs scroll. a model with an NJD_FLAG_USE_ENV
 * meshset, whose uvs are the view's, isn't kept, nor one of more than
 * KEEP_SETS meshsets */
struct keep_set {
    Uint16 first, count;
    Uint32 indices;
    Uint32 mesh_words; /* its polygons' words, and KEEP_SET_UVS where each vertex has a uv */
};
#define KEEP_SET_UVS 0x80000000u
#define KEEP_SETS 16
struct keep_info {
    Float light[3];
    Uint32 signature, probe, full;
    Uint32 built;
    Uint32 quiet_until;
    Uint8 strikes, sets, held, spare;
    Float box[6];
    struct keep_set set[KEEP_SETS];
};
int port_keep_next(const void *model, int after);
struct keep_info *port_keep_info(int id);
int port_keep_note(const void *model);
int port_keep_begin(const void *model, int reuse, int vertices, struct run_vertex **out, Uint16 **indices);
void port_keep_drop(int id);
int port_run_kept(int id, int set, int part_total, const Float transform[12], Float rhw_near, Float rhw_mid,
                  unsigned serial);
unsigned port_keep_frame(void);
/* HOST_VERTEX_CHECK=1 (the host sets it): each kept hit sums all of the
 * model's data, not a few words, and a record the few words passed but
 * the whole doesn't is logged (render.c) and built again */
extern volatile int port_keep_check;
/* a weld's model (src/draw/motion.c), whose points change each frame */
int port_keep_never(const void *model);
void port_keep_stale(const void *model, int id);
#define KEEP_LIGHT (1.0F / 1024.0F)
#define KEEP_FULL 16
#define KEEP_RECENT 60
#define KEEP_MOST_VERTICES 16384
/* the records a model may have at once, each under a light of its own: a
 * model placed many times, turned differently, has another light in its
 * space at each */
#define KEEP_VARIANTS 4
enum keep_state { KEEP_NONE, KEEP_HIT, KEEP_BUILD };
/* why models weren't drawn from the store, for the renderer's log: a stale
 * signature, a light beyond the model's records, a stale whole sum or
 * sampled sum; then the draws of a model that isn't kept for now, and of
 * one that can't be */
extern unsigned port_keep_why[6];
typedef Uint32 __attribute__((may_alias)) keep_word;
typedef Uint16 __attribute__((may_alias)) keep_half;

static inline Uint32 keep_mix(Uint32 h, Uint32 word)
{
    h = (h ^ word) * 0x9e3779b1u;
    return h ^ (h >> 15);
}

static Uint32 keep_words(Uint32 h, const void *from, int words)
{
    const keep_word *w = from;
    int k;

    for (k = 0; k < words; k++) {
        h = keep_mix(h, w[k]);
    }
    return h;
}

static Uint32 keep_halves(Uint32 h, const void *from, int halves)
{
    const keep_half *w = from;
    int k;

    for (k = 0; k + 1 < halves; k += 2) {
        h = keep_mix(h, (Uint32)w[k] | (Uint32)w[k + 1] << 16);
    }
    if (k < halves) {
        h = keep_mix(h, w[k]);
    }
    return h;
}

/* a sum over the colors a model shades from: the palette's 256 pairs and
 * the next's, where a specular material's offsets are. it is made once a
 * frame for a palette, and again where a few of its words have changed */
static Uint32 keep_palette_sum(const Uint32 *palette)
{
    static struct {
        const Uint32 *palette;
        Uint32 frame, sample, sum;
    } memo[8];
    static int next;
    Uint32 frame = port_keep_frame();
    Uint32 sample = palette[0] ^ palette[255] * 3 ^ palette[510] * 5 ^ palette[511] * 7 ^ palette[513] * 11 ^
                    palette[1023] * 13;
    int k;

    for (k = 0; k < 8; k++) {
        if (memo[k].palette == palette && memo[k].frame == frame && memo[k].sample == sample) {
            return memo[k].sum;
        }
    }
    k = next;
    next = (next + 1) & 7;
    memo[k].palette = palette;
    memo[k].frame = frame;
    memo[k].sample = sample;
    memo[k].sum = keep_words(0x811c9dc5u, palette, DRAW_PALETTE_ENTRIES * 4);
    return memo[k].sum;
}

/* the same for draw_unlit_offsets, an unlit textured meshset's offsets */
static Uint32 keep_unlit_sum(void)
{
    static Uint32 at_frame, sum;
    Uint32 frame = port_keep_frame();

    if (at_frame != frame) {
        at_frame = frame;
        sum = keep_words(0x811c9dc5u, draw_unlit_offsets, DRAW_UNLIT_OFFSETS);
    }
    return sum;
}

/* whether the drawer may keep model at all, and the signature of what its
 * colors come from besides the light and its data */
/* whether the host lights model's kept vertices (host.h's host_item,
 * port_gpu_models' bit 4), so that its records hold under any light. a
 * meshset's colors are then the palette's, as shade_points gives them, but
 * where keep_unlit says they don't depend on the light */
static Bool keep_lit(const NJS_MODEL *model)
{
    return (port_gpu_models & 16) && model->normals != NULL;
}

/* whether a meshset drawn under drawn has colors that don't depend on the
 * light, which a lit model's run then keeps as the drawer made them: one
 * under NJD_FLAG_IGNORE_LIGHT that is translucent, whose points
 * set_point_alpha turns white, or textured, an opaque one a decal (no base
 * color) with draw_unlit_offsets */
static Bool keep_unlit(Uint32 drawn)
{
    return (drawn & NJD_FLAG_IGNORE_LIGHT) && (drawn & (NJD_FLAG_USE_TEXTURE | NJD_FLAG_USE_ALPHA));
}

/* whether set_point_alpha turns the points white under drawn, for the
 * meshsets after it too */
static Bool keep_whitens(Uint32 drawn)
{
    return (drawn & NJD_FLAG_IGNORE_LIGHT) && (drawn & NJD_FLAG_USE_ALPHA);
}

/* the most of any component of model's normals, at least 1: the host's
 * normals (keep_cmp) hold -1 to 1, so a longer normal goes over it, and the
 * light the host takes times it, which gives the same dot */
static Float keep_normal_scale(const NJS_MODEL *model)
{
    Float most = 1.0F;
    int k;

    for (k = 0; k < model->nbPoint; k++) {
        const NJS_VECTOR *n = &model->normals[k];
        Float m = __builtin_fabsf(n->x);

        m = __builtin_fabsf(n->y) > m ? __builtin_fabsf(n->y) : m;
        m = __builtin_fabsf(n->z) > m ? __builtin_fabsf(n->z) : m;
        most = m > most ? m : most;
    }
    return most;
}

/* a component f of -1 to 1 as a signed field of the NV2A's CMP, to the
 * nearest of its steps, the largest one */
static Uint32 keep_cmp_field(Float f, int largest, Uint32 mask)
{
    Float scaled = f * (Float)largest;
    int v = (int)(scaled + (scaled < 0.0F ? -0.5F : 0.5F));

    v = v > largest ? largest : v < -largest ? -largest : v;
    return (Uint32)v & mask;
}

/* normal n over scale, packed as the NV2A's CMP: x and y in 11 bits, z in
 * 10 (host.h's host_item) */
static Uint32 keep_cmp(const NJS_VECTOR *n, Float scale)
{
    Float inv = 1.0F / scale;

    return keep_cmp_field(n->x * inv, 1023, 0x7ff) | keep_cmp_field(n->y * inv, 1023, 0x7ff) << 11 |
           keep_cmp_field(n->z * inv, 511, 0x3ff) << 22;
}

static Bool keep_signature(const NJS_MODEL *model, Uint32 *signature)
{
    const NJS_MATERIAL *first = &model->mats[model->meshsets->type_matId & MATERIAL_ID];
    const NJS_MESHSET *meshset = model->meshsets;
    Uint32 h = 0x811c9dc5u;
    Bool unlit = FALSE;
    int k;

    if (model->nbMeshset > KEEP_SETS || model->nbMeshset == 0 || model->nbPoint == 0) {
        return FALSE;
    }
    h = keep_mix(h, (Uint32)model->points);
    h = keep_mix(h, (Uint32)model->normals);
    h = keep_mix(h, (Uint32)model->nbPoint);
    h = keep_mix(h, (Uint32)model->meshsets);
    h = keep_mix(h, (Uint32)model->mats);
    h = keep_mix(h, (Uint32)model->nbMeshset);
    /* a model the host lights takes the palette as it draws, so its
     * records hold under any palette's colors */
    h = keep_mix(h, keep_lit(model) ? 0x4c495448u : keep_palette_sum(shade_palette));
    h = keep_mix(h, !(drawn_flags(first) & NJD_FLAG_IGNORE_SPECULAR) && first->specular.color != 0);
    h = keep_mix(h, (Uint32)alpha_mode);
    if (alpha_mode != ALPHA_OWN) {
        h = keep_words(h, &_nj_constant_material_.a, 1);
    }
    for (k = 0; k < model->nbMeshset; k++, meshset++) {
        const NJS_MATERIAL *material = &model->mats[meshset->type_matId & MATERIAL_ID];
        Uint32 drawn = drawn_flags(material);
        Uint32 own = material->attrflags;

        if ((own & NJD_FLAG_USE_TEXTURE) && (own & NJD_FLAG_USE_ENV)) {
            return FALSE;
        }
        if ((drawn & NJD_FLAG_USE_ALPHA) && !(port_gpu_models & 2)) {
            return FALSE;
        }
        if ((drawn & NJD_FLAG_USE_TEXTURE) && (drawn & NJD_FLAG_IGNORE_LIGHT)) {
            unlit = TRUE;
        }
        h = keep_mix(h, meshset->type_matId);
        h = keep_mix(h, meshset->nbMesh);
        h = keep_mix(h, (Uint32)meshset->meshes);
        h = keep_mix(h, (Uint32)meshset->vertuv);
        h = keep_mix(h, drawn);
        h = keep_mix(h, own);
        h = keep_mix(h, material->diffuse.argb.a);
    }
    if (unlit) {
        h = keep_mix(h, keep_unlit_sum());
    }
    *signature = h;
    return TRUE;
}

/* the sums over a kept model's data: all of it, or a few words of each
 * part */
static Uint32 keep_sum(const NJS_MODEL *model, const struct keep_info *info, Bool full)
{
    const NJS_MESHSET *meshset = model->meshsets;
    int n = model->nbPoint;
    Uint32 h = 0x811c9dc5u;
    int k;

    if (full) {
        h = keep_words(h, model->points, n * 3);
        h = keep_words(h, model->normals, n * 3);
    } else {
        const int at[4] = {0, n / 3, n * 2 / 3, n - 1};

        for (k = 0; k < 4; k++) {
            h = keep_words(h, &model->points[at[k]], 3);
            h = keep_words(h, &model->normals[at[k]], 3);
        }
    }
    for (k = 0; k < info->sets; k++, meshset++) {
        int words = (int)(info->set[k].mesh_words & ~KEEP_SET_UVS);
        int uvs = (info->set[k].mesh_words & KEEP_SET_UVS) ? info->set[k].count : 0;

        if (full) {
            h = keep_halves(h, meshset->meshes, words);
            if (uvs != 0) {
                h = keep_words(h, meshset->vertuv, uvs);
            }
        } else {
            if (words >= 2) {
                h = keep_halves(h, meshset->meshes, 2);
                h = keep_halves(h, meshset->meshes + (words / 2 & ~1), 2);
                h = keep_halves(h, meshset->meshes + words - 2, 2);
            }
            if (uvs != 0) {
                h = keep_words(h, meshset->vertuv, 1);
                h = keep_words(h, (const keep_word *)meshset->vertuv + uvs / 2, 1);
                h = keep_words(h, (const keep_word *)meshset->vertuv + uvs - 1, 1);
            }
        }
    }
    return h;
}

/* the light in the model's space, as shade_points makes it */
static void keep_light(Float light[3])
{
    draw_vec4 l = draw_in_model(draw_vec4_init(light_dir.x, light_dir.y, light_dir.z, 0.0F));

    light[0] = l.x;
    light[1] = l.y;
    light[2] = l.z;
}

/* what the drawer does with model: draws its kept vertices (*id the record
 * that holds them), builds them (*id a record of the model's without any,
 * or -1), or neither */
static enum keep_state keep_look(const NJS_MODEL *model, int *id, Uint32 *signature, Float light[3])
{
    Uint32 now = port_keep_frame();
    struct keep_info *info;
    int match = -1;
    int bare = -1; /* the model's record that holds no vertices */
    int variants = 0;
    Uint32 newest = 0; /* the frame the last of its records was built in */
    int at;
    Bool ok;
    Bool lit;

    *id = -1;
    if (!(port_gpu_models & 4)) {
        return KEEP_NONE;
    }
    if (port_keep_never(model) || !keep_signature(model, signature)) {
        port_keep_why[5]++;
        return KEEP_NONE;
    }
    keep_light(light);
    lit = keep_lit(model);
    for (at = port_keep_next(model, -1); at >= 0; at = port_keep_next(model, at)) {
        int k;

        info = port_keep_info(at);
        if (!info->held) {
            bare = at;
            continue;
        }
        variants++;
        if (info->built > newest) {
            newest = info->built;
        }
        /* a record the host lights holds under any light: its light[0]
         * is its normals' scale (keep_normal_scale) */
        for (k = 0; k < 3 && !lit; k++) {
            Float off = info->light[k] - light[k];

            if (!(off < KEEP_LIGHT && off > -KEEP_LIGHT)) {
                break;
            }
        }
        if (k == 3 || lit) {
            match = at;
        }
    }
    if (bare >= 0 && now < port_keep_info(bare)->quiet_until) {
        port_keep_why[4]++;
        return KEEP_NONE;
    }
    if (match < 0) {
        /* under a light none of its records has. one more record, where
         * the others were all built this frame, the model placed several
         * times, or none lately: a model whose light has moved on within
         * KEEP_RECENT frames of its last record is turning, and a record
         * a frame would fill the store with what no later frame draws */
        if (variants >= KEEP_VARIANTS || (variants != 0 && newest != now && now - newest < KEEP_RECENT)) {
            port_keep_why[1]++;
            return KEEP_NONE;
        }
        *id = bare;
        return KEEP_BUILD;
    }
    info = port_keep_info(match);
    ok = info->signature == *signature;
    if (!ok) {
        port_keep_why[0]++;
    } else if ((now + (Uint32)match) % KEEP_FULL == 0) {
        ok = keep_sum(model, info, TRUE) == info->full;
        if (!ok) {
            port_keep_why[2]++;
        }
    } else {
        ok = keep_sum(model, info, FALSE) == info->probe;
        if (!ok) {
            port_keep_why[3]++;
        } else if (port_keep_check && keep_sum(model, info, TRUE) != info->full) {
            /* stale, though its few words held: drawn, it would show the
             * model as it was when built */
            port_keep_stale(model, match);
            port_keep_why[2]++;
            ok = FALSE;
        }
    }
    if (ok) {
        *id = match;
        return KEEP_HIT;
    }
    {
        /* stale: its vertices go, and the model's record without any says
         * how long it isn't kept, where this one was built lately */
        Bool recent = now - info->built < KEEP_RECENT;

        port_keep_drop(match);
        if (bare < 0) {
            bare = port_keep_note(model);
        }
        if (bare < 0) {
            return KEEP_NONE;
        }
        info = port_keep_info(bare);
        if (recent) {
            if (info->strikes < 4) {
                info->strikes++;
            }
            info->quiet_until = now + ((Uint32)KEEP_RECENT << info->strikes);
            return KEEP_NONE;
        }
        info->strikes = 0;
    }
    *id = bare;
    return KEEP_BUILD;
}

/* the vertices model's runs take, as gpu_send would send them, with each
 * meshset's count, polygons' words and uvs in sets: 0 where they are too
 * many to keep */
static int keep_count(const NJS_MODEL *model, struct keep_set sets[KEEP_SETS])
{
    const NJS_MESHSET *meshset = model->meshsets;
    int total = 0;
    int k;

    for (k = 0; k < model->nbMeshset; k++, meshset++) {
        int fixed = polygon_sizes[(meshset->type_matId & NJD_MESHSET_MASK) >> MESHSET_TYPE_SHIFT];
        const NJS_MATERIAL *material = &model->mats[meshset->type_matId & MATERIAL_ID];
        const Sint16 *mesh = meshset->meshes;
        int count = 0;
        int polygons;

        for (polygons = meshset->nbMesh; polygons != 0; polygons--) {
            int size = fixed != 0 ? fixed : *mesh++ & STRIP_COUNT;

            if (size < 3) {
                break;
            }
            mesh += size;
            count += size;
        }
        if (count > 0xffff || total + count > KEEP_MOST_VERTICES) {
            return 0;
        }
        sets[k].first = (Uint16)total;
        sets[k].count = (Uint16)count;
        sets[k].indices = 0;
        sets[k].mesh_words =
            (Uint32)(mesh - meshset->meshes) | ((material->attrflags & NJD_FLAG_USE_TEXTURE) ? KEEP_SET_UVS : 0);
        total += count;
    }
    return total;
}

/* the parts of a translucent meshset's run, one for each strip, as
 * gpu_send makes them: its indices in the run's and the 1/z of its nearest
 * vertex, by depth_row, the transform's third row. it returns their count */
static int gpu_parts(const NJS_MODEL *model, const struct strips *strips, const Float *depth_row,
                     struct run_part *run_parts, int part_room)
{
    const NJS_POINT3 *from = model->points;
    const Sint16 *mesh = strips->meshset->meshes;
    int named = 0;
    int made = 0;
    int polygons;

    for (polygons = strips->meshset->nbMesh; polygons != 0; polygons--) {
        int size = next_polygon_size(strips, &mesh);
        Float nearest = 1.0e30F;
        int k;

        if (size < 3 || made >= part_room) {
            break;
        }
        for (k = 0; k < size; k++) {
            int index = *mesh++;
            Float z = depth_row[0] * from[index].x + depth_row[1] * from[index].y + depth_row[2] * from[index].z +
                      depth_row[3];

            if (z < nearest) {
                nearest = z;
            }
        }
        run_parts[made].first_index = named;
        run_parts[made].index_count = (size - 2) * 3;
        run_parts[made].rhw_near = nearest > 0.0F ? 1.0F / nearest : 1.0e30F;
        named += (size - 2) * 3;
        made++;
    }
    return made;
}

/* the box that holds model's points, as box[0..2] its least x, y and z and
 * box[3..5] its most */
static void model_box(const NJS_MODEL *model, Float box[6])
{
    const NJS_POINT3 *p = model->points;
    int k;

    box[0] = box[3] = p->x;
    box[1] = box[4] = p->y;
    box[2] = box[5] = p->z;
    for (k = 1, p++; k < model->nbPoint; k++, p++) {
        if (p->x < box[0]) {
            box[0] = p->x;
        }
        if (p->x > box[3]) {
            box[3] = p->x;
        }
        if (p->y < box[1]) {
            box[1] = p->y;
        }
        if (p->y > box[4]) {
            box[4] = p->y;
        }
        if (p->z < box[2]) {
            box[2] = p->z;
        }
        if (p->z > box[5]) {
            box[5] = p->z;
        }
    }
}

/* gpu_takes' tests by the model's box, for one whose sphere fails them:
 * every point lies among the box's corners, so where each corner is past
 * the near plane and within the guard of the screen, each point is */
static Bool box_takes(const Float box[6], Float *rhw_near, Float *rhw_mid)
{
    Float nearest = 1.0e30F;
    Float sum = 0.0F;
    int k;

    for (k = 0; k < 8; k++) {
        NJS_POINT3 corner = {box[(k & 1) ? 3 : 0], box[(k & 2) ? 4 : 1], box[(k & 4) ? 5 : 2]};
        draw_vec3 c = view_point(&corner);
        Float inv;
        Float x, y;

        if (!(c.z > view.near)) {
            return FALSE;
        }
        inv = 1.0F / c.z;
        x = c.x * view.scale_x * inv + view.cx;
        y = c.y * view.scale_y * inv + view.cy;
        if (!(x > -GPU_GUARD_X) || !(x < view.w + GPU_GUARD_X) || !(y > -GPU_GUARD_Y) ||
            !(y < view.h + GPU_GUARD_Y)) {
            return FALSE;
        }
        if (c.z < nearest) {
            nearest = c.z;
        }
        sum += c.z;
    }
    *rhw_near = 1.0F / nearest;
    *rhw_mid = 8.0F / sum;
    return TRUE;
}

/* a model the host takes: its meshsets as draw_meshes sends them, as runs.
 * a kept model's are runs of the vertices the host's store holds (state
 * KEEP_HIT), or of those built into it now (KEEP_BUILD). any other's are
 * written into the frame's arena, the opaque ones, and the translucent
 * ones under port_gpu_models' bit 1; the rest are projected, from a second
 * set of points made as the first of them comes, and so is a meshset the
 * renderer has no run for */
static struct point gpu_projected[DRAW_MOST_POINTS] __attribute__((aligned(32)));

static void gpu_draw(const NJS_MODEL *model, Float rhw_near, Float rhw_mid, enum keep_state state, int kept,
                     Uint32 signature, const Float light[3])
{
    struct point *points = (struct point *)_nj_vertex_buf_;
    const NJS_MATERIAL *first = &model->mats[model->meshsets->type_matId & MATERIAL_ID];
    const NJS_MESHSET *meshset = model->meshsets;
    struct strips strips = {NULL, 0, points, NULL, model->normals};
    static unsigned serials;
    unsigned serial = ++serials; /* this draw of a model, for its runs to share its transform */
    struct keep_info *info = NULL;
    struct keep_set sets[KEEP_SETS];
    struct run_vertex *kept_vertices = NULL;
    Uint16 *kept_indices = NULL;
    Float rows[12];
    Bool lit = FALSE;
    Float lit_scale = 1.0F;
    Float lit_light[3] = {0.0F, 0.0F, 0.0F};
    const Uint32 *lit_pairs = NULL; /* the pairs whose offset colors a lit run takes */
    Bool whitened = FALSE; /* a meshset so far turned the points white (keep_whitens) */
    Bool projected = FALSE;
    Bool have_rows = FALSE;
    int meshsets;
    int set = 0;
    int k;

    if (state == KEEP_BUILD) {
        int total = keep_count(model, sets);

        if (total != 0) {
            kept = port_keep_begin(model, kept, total, &kept_vertices, &kept_indices);
        }
        if (kept_vertices == NULL) {
            /* no room in the store, or no record: not asked again for a
             * while, and written anew like any other */
            if (kept >= 0) {
                port_keep_info(kept)->quiet_until = port_keep_frame() + KEEP_RECENT;
            }
            state = KEEP_NONE;
        }
    }
    if (state != KEEP_NONE) {
        info = port_keep_info(kept);
        /* lit by the host: a record's normals are over its scale, which its
         * light[0] keeps, and the light the host takes is times it */
        lit = keep_lit(model);
        if (lit) {
            lit_scale = state == KEEP_BUILD ? keep_normal_scale(model) : info->light[0];
            lit_light[0] = light[0] * lit_scale;
            lit_light[1] = light[1] * lit_scale;
            lit_light[2] = light[2] * lit_scale;
            lit_pairs = shade_palette + 1;
            if (!(drawn_flags(first) & NJD_FLAG_IGNORE_SPECULAR) && first->specular.color != 0) {
                lit_pairs += DRAW_PALETTE_ENTRIES * 2;
            }
        }
    }
    if (state != KEEP_HIT) {
        /* the points' colors alone: their places are the model's own */
        shade_points(model, points, first, drawn_flags(first));
    }
    if (port_profile) {
        port_count_model(model, model->points, model->nbPoint);
    }
    for (meshsets = model->nbMeshset; meshsets != 0; meshsets--, meshset++, set++) {
        const NJS_MATERIAL *material = &model->mats[meshset->type_matId & MATERIAL_ID];
        Uint32 drawn = drawn_flags(material);
        Uint32 own = material->attrflags;
        enum uv uv = !(own & NJD_FLAG_USE_TEXTURE) ? NO_UV : (own & NJD_FLAG_USE_ENV) ? ENV_UV : MESH_UV;
        struct run_vertex *run_vertices = NULL;
        Uint16 *run_indices = NULL;
        int vertex_room = 0, index_room = 0;
        int run = -1;
        struct vertex *sq;
        struct vertex *start;

        set_material_header(material, drawn);
        strips.offsets = lit_offsets;
        strips.meshset = meshset;
        strips.polygon_size = polygon_sizes[(meshset->type_matId & NJD_MESHSET_MASK) >> MESHSET_TYPE_SHIFT];
        if ((drawn & NJD_FLAG_USE_TEXTURE) && (drawn & NJD_FLAG_IGNORE_LIGHT)) {
            strips.offsets = draw_unlit_offsets;
        }
        if ((drawn & NJD_FLAG_USE_ALPHA) && state != KEEP_HIT) {
            /* the points take the alpha, and under NJD_FLAG_IGNORE_LIGHT
             * the white, which the meshsets after it read too */
            set_point_alpha(points, model->nbPoint, material, drawn);
        }
        if (keep_whitens(drawn)) {
            whitened = TRUE;
        }
        if (drawn & NJD_FLAG_USE_TEXTURE) {
            sq = drawn & NJD_FLAG_USE_ALPHA ? njStartVertexTexTrans() : njStartVertexTexOpaque();
        } else {
            sq = drawn & NJD_FLAG_USE_ALPHA ? njStartVertexNonTexTrans() : njStartVertexNonTexOpaque();
        }
        start = sq;
        if (state != KEEP_NONE) {
            /* a kept model's meshset: a run of the store's vertices */
            struct run_part *run_parts = NULL;
            int part_room = 0;
            int part_total = 0;

            if (!have_rows) {
                gpu_transform(rows);
                have_rows = TRUE;
            }
            if (state == KEEP_BUILD) {
                int index_count = 0;
                int unused = 0;
                int sent = gpu_send(model, &strips, uv, kept_vertices + sets[set].first, sets[set].count,
                                    kept_indices + sets[set].first * 3, sets[set].count * 3, &index_count, NULL, NULL,
                                    0, &unused, lit && !keep_unlit(drawn) ? &lit_scale : NULL);

                sets[set].count = (Uint16)sent;
                sets[set].indices = (Uint32)index_count;
                info->set[set] = sets[set];
                if (port_profile) {
                    port_counts.built_vertices += (unsigned)sent;
                }
            } else if (port_profile) {
                port_counts.kept_vertices += info->set[set].count;
            }
            if (drawn & NJD_FLAG_USE_ALPHA) {
                run_parts = port_run_parts(&part_room);
                part_total = gpu_parts(model, &strips, &rows[8], run_parts, part_room);
            }
            run = info->set[set].indices != 0
                      ? port_run_kept(kept, set, part_total, rows, rhw_near, rhw_mid, serial)
                      : -1;
            if (run >= 0 && lit && !keep_unlit(drawn)) {
                /* an untextured meshset's vertices take no offset color,
                 * and points an earlier meshset turned white a white base */
                port_run_lit(run, lit_light, whitened ? NULL : shade_palette, uv == NO_UV ? NULL : lit_pairs);
            }
            if (run >= 0) {
                Uint32 *words = (Uint32 *)sq;

                words[0] = PARA_MODEL;
                words[1] = (Uint32)run;
                for (k = 2; k < 8; k++) {
                    words[k] = 0;
                }
                sq++;
                if (port_profile) {
                    port_counts.drawer_vertices += info->set[set].count;
                    port_counts.gpu_vertices += info->set[set].count;
                }
            }
            njEndVertex((Uint32 *)sq);
            continue;
        }
        if (!(drawn & NJD_FLAG_USE_ALPHA) || (port_gpu_models & 2)) {
            run = port_run_begin(&run_vertices, &vertex_room, &run_indices, &index_room);
        }
        if (run >= 0) {
            int part_room = -1;
            int wanted;

            if (drawn & NJD_FLAG_USE_ALPHA) {
                port_run_parts(&part_room);
            }
            if (!gpu_fits(&strips, vertex_room, index_room, part_room, &wanted)) {
                /* begun, not ended: the run's slot stays free. its
                 * vertices count in what the frame wanted of the arena */
                run = -1;
                port_run_unfit++;
                port_run_unfit_vertices += (unsigned)wanted;
            }
        }
        if (run < 0) {
            /* projected, through its list, with the colors the points
             * hold now */
            if (!projected) {
                project_points(model, gpu_projected);
                projected = TRUE;
            }
            for (k = 0; k < model->nbPoint; k++) {
                gpu_projected[k].color = points[k].color;
            }
            strips.points = gpu_projected;
        } else {
            strips.points = points;
        }
        if (run >= 0) {
            Uint32 *words = (Uint32 *)sq;
            int index_count;
            struct run_part *run_parts = NULL;
            int part_room = 0;
            int part_total = 0;
            int sent;

            if (!have_rows) {
                gpu_transform(rows);
                have_rows = TRUE;
            }
            /* a translucent run's strips sort apart, as the list's do: each
             * by the 1/z of its nearest vertex */
            if (drawn & NJD_FLAG_USE_ALPHA) {
                run_parts = port_run_parts(&part_room);
            }
            sent = gpu_send(model, &strips, uv, run_vertices, vertex_room, run_indices, index_room, &index_count,
                            (drawn & NJD_FLAG_USE_ALPHA) ? &rows[8] : NULL, run_parts, part_room, &part_total, NULL);
            port_run_end(run, sent, index_count, part_total, rows, rhw_near, rhw_mid, serial);
            words[0] = PARA_MODEL;
            words[1] = (Uint32)run;
            for (k = 2; k < 8; k++) {
                words[k] = 0;
            }
            sq++;
            if (port_profile) {
                port_counts.drawer_vertices += (unsigned)sent;
                port_counts.gpu_vertices += (unsigned)sent;
            }
        } else {
            if (uv == NO_UV) {
                sq = strips_untextured(sq, &strips);
            } else if (uv == ENV_UV) {
                sq = strips_env(sq, &strips);
            } else {
                sq = strips_textured(sq, &strips);
            }
            if (port_profile) {
                port_counts.drawer_vertices += (unsigned)(sq - start);
            }
        }
        njEndVertex((Uint32 *)sq);
    }
    if (state == KEEP_BUILD) {
        /* built: what it was built from, for the next draw to compare */
        for (k = 0; k < 3; k++) {
            info->light[k] = light[k];
        }
        if (lit) {
            info->light[0] = lit_scale;
        }
        info->signature = signature;
        info->sets = (Uint8)model->nbMeshset;
        info->probe = keep_sum(model, info, FALSE);
        info->full = keep_sum(model, info, TRUE);
        info->built = port_keep_frame();
        model_box(model, info->box);
        info->held = 1;
    }
}

/* draw model by the host where it can take it, by its sphere of reach_scale
 * times r, or else by its box: whether it did. get_pointer has it take
 * Kamui's pointer first, as draw_model does before its meshes */
static Bool gpu_try(const NJS_MODEL *model, Float reach_scale, Bool get_pointer)
{
    Float rhw_near, rhw_mid;
    Float light[3] = {0.0F, 0.0F, 0.0F};
    Uint32 signature = 0;
    enum keep_state state;
    int kept;

    if (!port_gpu_models || model->nbPoint > DRAW_MOST_POINTS) {
        return FALSE;
    }
    if (gpu_takes(model, reach_scale, &rhw_near, &rhw_mid)) {
        state = keep_look(model, &kept, &signature, light);
    } else if (port_gpu_models & 8) {
        Float box[6];

        state = keep_look(model, &kept, &signature, light);
        if (state == KEEP_HIT) {
            if (!box_takes(port_keep_info(kept)->box, &rhw_near, &rhw_mid)) {
                return FALSE;
            }
        } else {
            if (model->nbPoint == 0) {
                return FALSE;
            }
            model_box(model, box);
            if (!box_takes(box, &rhw_near, &rhw_mid)) {
                return FALSE;
            }
        }
    } else {
        return FALSE;
    }
    if (get_pointer) {
        njGetkmPointer();
    }
    gpu_draw(model, rhw_near, rhw_mid, state, kept, signature, light);
    return TRUE;
}
#endif

OUT_OF_LINE void draw_meshes(const NJS_MODEL *model, struct point *points,
                             Bool clipped)
{
    const NJS_MESHSET *meshset = model->meshsets;
    struct strips strips = {NULL, 0, points, NULL, model->normals};
    int meshsets;

#ifdef XBOX
    if (port_profile) {
        port_count_model(model, model->points, model->nbPoint);
    }
#endif
    for (meshsets = model->nbMeshset; meshsets != 0; meshsets--, meshset++) {
        const NJS_MATERIAL *material =
            &model->mats[meshset->type_matId & MATERIAL_ID];
        Uint32 drawn = drawn_flags(material);
        Uint32 own = material->attrflags;
        void *sq;

        set_material_header(material, drawn);
        strips.offsets = lit_offsets;
        if (drawn & NJD_FLAG_USE_ALPHA) {
            set_point_alpha(points, model->nbPoint, material, drawn);
        }
        if (drawn & NJD_FLAG_USE_TEXTURE) {
            if (drawn & NJD_FLAG_IGNORE_LIGHT) {
                strips.offsets = draw_unlit_offsets;
            }
            sq = drawn & NJD_FLAG_USE_ALPHA ? njStartVertexTexTrans()
                                            : njStartVertexTexOpaque();
        } else {
            sq = drawn & NJD_FLAG_USE_ALPHA ? njStartVertexNonTexTrans()
                                            : njStartVertexNonTexOpaque();
        }
        strips.meshset = meshset;
        strips.polygon_size =
            polygon_sizes[(meshset->type_matId & NJD_MESHSET_MASK) >>
                          MESHSET_TYPE_SHIFT];
#ifdef XBOX
        port_sq_start = sq;
#endif
        if (!(own & NJD_FLAG_USE_TEXTURE)) {
            sq = clipped ? clipped_untextured(sq, &strips)
                         : strips_untextured(sq, &strips);
        } else if (own & NJD_FLAG_USE_ENV) {
            sq = clipped ? clipped_env(sq, &strips) : strips_env(sq, &strips);
        } else {
            sq = clipped ? clipped_textured(sq, &strips)
                         : strips_textured(sq, &strips);
        }
#ifdef XBOX
        if (port_profile) {
            port_counts.drawer_vertices +=
                (unsigned)((char *)sq - (char *)port_sq_start) / 32;
        }
#endif
        njEndVertex(sq);
    }
}

/* set the header words as draw_meshes leaves them after model's meshsets,
 * and send nothing */
OUT_OF_LINE void set_meshes_header(const NJS_MODEL *model)
{
    const NJS_MESHSET *meshset = model->meshsets;
    int meshsets;

    for (meshsets = model->nbMeshset; meshsets != 0; meshsets--, meshset++) {
        const NJS_MATERIAL *material =
            &model->mats[meshset->type_matId & MATERIAL_ID];

        set_material_header(material, drawn_flags(material));
    }
}

/* project model's points into _nj_vertex_buf_, shade them unless every one
 * is near, and return how many are near */
OUT_OF_LINE int project_and_shade(const NJS_MODEL *model)
{
    struct point *points = (struct point *)_nj_vertex_buf_;
    int near_points = project_points(model, points);
    const NJS_MATERIAL *first;

    if (near_points < model->nbPoint) {
        first = &model->mats[model->meshsets->type_matId & MATERIAL_ID];
        shade_points(model, points, first, drawn_flags(first));
    }
    return near_points;
}

#ifdef XBOX
static void draw_model_body(NJS_MODEL *model);
void draw_model(NJS_MODEL *model)
{
    PORT_TSC_BEGIN;

    draw_model_body(model);
    PORT_TSC_END();
}
#define draw_model draw_model_body
static
#endif
void draw_model(NJS_MODEL *model)
{
    int near_points;

    if (!FRAME_DRAWS() || event_skipping) {
        return;
    }
    present_untimed(TRUE);
    present_record(PRESENT_MODEL, model, 0, 0, 0.0F, 0);
    if (model_in_view(model)) {
        take_constant_attr();
#ifdef XBOX
        {
            if (gpu_try(model, 1.0F, TRUE)) {
                reset_shading();
                njSetkmPointer();
                present_untimed(FALSE);
                return;
            }
        }
#endif
        near_points = project_and_shade(model);
        if (near_points < model->nbPoint) {
            njGetkmPointer();
            draw_meshes(model, (struct point *)_nj_vertex_buf_,
                        near_points != 0);
            reset_shading();
            njSetkmPointer();
        }
    }
    present_untimed(FALSE);
}

/* unlike draw_model, it sets the shading back to MODULATE_ALPHA for a model
 * in view even when every point is near */
#ifdef XBOX
static void draw_model_nested_body(NJS_MODEL *model);
void draw_model_nested(NJS_MODEL *model)
{
    PORT_TSC_BEGIN;

    draw_model_nested_body(model);
    PORT_TSC_END();
}
#define draw_model_nested draw_model_nested_body
static
#endif
void draw_model_nested(NJS_MODEL *model)
{
    int near_points;

    if (!FRAME_DRAWS() || event_skipping) {
        return;
    }
    present_untimed(TRUE);
    present_record(PRESENT_MODEL, model, 0, 0, 0.0F, 0);
    if (model_in_view(model)) {
        take_constant_attr();
#ifdef XBOX
        {
            if (gpu_try(model, 1.0F, FALSE)) {
                reset_shading();
                present_untimed(FALSE);
                return;
            }
        }
#endif
        near_points = project_and_shade(model);
        if (near_points < model->nbPoint) {
            draw_meshes(model, (struct point *)_nj_vertex_buf_,
                        near_points != 0);
        }
        reset_shading();
    }
    present_untimed(FALSE);
}

/* a model off the screen is neither projected nor sent. when one of its
 * points lies past view.near, its meshsets still set the header words, so
 * that the run leaves them as model.src's does */
#ifdef XBOX
static void draw_model_unculled_body(NJS_MODEL *model);
void draw_model_unculled(NJS_MODEL *model)
{
    PORT_TSC_BEGIN;

    draw_model_unculled_body(model);
    PORT_TSC_END();
}
#define draw_model_unculled draw_model_unculled_body
static
#endif
void draw_model_unculled(NJS_MODEL *model)
{
    int near_points;

    if (event_skipping) {
        return;
    }
    if (model_off_screen(model)) {
        if (any_past_near(model)) {
            set_meshes_header(model);
        }
        return;
    }
#ifdef XBOX
    {
        if (gpu_try(model, LAND_REACH, FALSE)) {
            return;
        }
    }
#endif
    near_points = project_and_shade(model);
    if (near_points < model->nbPoint) {
        draw_meshes(model, (struct point *)_nj_vertex_buf_, near_points != 0);
    }
}

#ifdef XBOX
#undef draw_model
#undef draw_model_nested
#undef draw_model_unculled
#endif

void draw_models_begin(void)
{
    take_constant_attr();
    njGetkmPointer();
}

void draw_models_end(void)
{
    reset_shading();
    njSetkmPointer();
}

Bool draw_in_view(Float x, Float y, Float z, Float radius)
{
    NJS_POINT3 center = {x, y, z};
    Float side = port_view_aspect * _nj_screen_.dist;

    /* @bug logic: the bottom test scales y by the side tests' scale, not dist
     * times _nj_scr_aspect_y_, and the top test by _nj_scr_aspect_y_ times
     * _nj_screen_.cx. model.src's FMUL FR2,FR11 at 0x8c039d98 scales the
     * bottom test's y by the side tests' scale, and its FMUL FR7,FR3 at
     * 0x8c039d82 multiplies _nj_scr_aspect_y_ by cx for the top test's
     * scale */
    Bool in = frame_step == 0 &&
              sphere_in_view(view_point(&center), radius, _nj_near_clip_, side,
                             side, _nj_scr_aspect_y_ * _nj_screen_.cx);

    /* for the split frame's pass, which calls no callback of a task this
     * refused on step 0 */
    present_in_view(in);
    return in;
}

void draw_set_light(Float x, Float y, Float z)
{
    light_dir.x = light_base.x = x;
    light_dir.y = light_base.y = y;
    light_dir.z = light_base.z = z;
}

void draw_divide_light(Float scale)
{
    Float inv = 1.0F / scale;

    light_dir.x = light_base.x * inv;
    light_dir.y = light_base.y * inv;
    light_dir.z = light_base.z * inv;
}

void draw_reset_light(void)
{
    light_dir = light_base;
}

void draw_set_palette(int palette)
{
    shade_palette = &draw_colors.words[palette * DRAW_PALETTE_ENTRIES * 2];
}

void draw_get_shading(struct draw_shading *shading)
{
    shading->dir = light_dir;
    shading->base = light_base;
    shading->palette = shade_palette;
}

void draw_put_shading(const struct draw_shading *shading)
{
    light_dir = shading->dir;
    light_base = shading->base;
    shade_palette = shading->palette;
}

/* model.c keeps no point light queue or list, so there is nothing to take */
void draw_take_point_lights(void)
{
}
