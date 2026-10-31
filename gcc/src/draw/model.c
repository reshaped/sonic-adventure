/* the model renderer in C, on SH4ZAM's FTRV and FIPR. make gcc compiles it
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
#include <sh4zam/shz_mem.h>
#include <sh4zam/shz_vector.h>
#include <sh4zam/shz_xmtrx.h>

#include "draw/model.h"
#include "sdk/ninja/njkamui.h"
#include "system/frame.h"

#define OUT_OF_LINE static __attribute__((noinline, noclone))

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

static inline shz_vec3_t view_point(const NJS_POINT3 *p)
{
    return shz_xmtrx_transform_point3(zam_load_vec3(p));
}

/* whether the sphere of radius r about center, in view space, may be in
 * view. the sphere is out when it lies wholly nearer than near or past
 * _nj_far_clip_. it is out too when an edge test puts it wholly beyond an
 * edge of _nj_2d_clip_. the edge tests project the sphere's sides at its far
 * z, center.z + r. the side tests scale x by side, the bottom test scales y
 * by bottom, and the top test scales y by top */
static inline Bool sphere_in_view(shz_vec3_t center, Float r, Float near,
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
    shz_vec3_t center = view_point(&model->center);
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

static inline struct projecting project_begin(shz_vec4_t v,
                                              const struct projection *by)
{
    struct projecting p = {v.z, 1.0F / v.z, v.x * by->scale_x,
                           v.y * by->scale_y};

    return p;
}

/* the vector at *from, with w, and *from stepped past it. FMOV.S takes no
 * displacement, so the asm loads the three through *from by post-increment,
 * as model.src's loops do */
ALWAYS_INLINE shz_vec4_t next_vector(const NJS_VECTOR **from, Float w)
{
    Float x, y, z;

    __asm__("fmov.s @%[from]+,%[x]\n\t"
            "fmov.s @%[from]+,%[y]\n\t"
            "fmov.s @%[from]+,%[z]"
            : [from] "+r"(*from), [x] "=f"(x), [y] "=f"(y), [z] "=f"(z)
            : "m"(**from));
    return shz_vec4_init(x, y, z, w);
}

/* PREF the line PREFETCH_AHEAD bytes past p. the address is an integer,
 * since it may lie past p's array. SH4ZAM's SHZ_PREFETCH clobbers no memory,
 * where machine.h's prefetch clobbers it for the store queues. but it is an
 * asm volatile, which GCC's scheduler moves no instruction across, so each
 * loop PREFs at the top of a pass, before its loads */
static inline void prefetch_ahead(const void *p)
{
    SHZ_PREFETCH((Uint32)p + PREFETCH_AHEAD);
}

/* the point at *from in view space, with *from stepped past it */
ALWAYS_INLINE shz_vec4_t view_next(const NJS_POINT3 **from)
{
    return shz_xmtrx_transform_vec4(next_vector(from, 1.0F));
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

    __asm__ volatile("fmov.s %[rhw],@-%[word]\n\t"
                     "fmov.s %[y],@-%[word]\n\t"
                     "fmov.s %[x],@-%[word]"
                     : [word] "+r"(word)
                     : [rhw] "f"(p.rhw), [x] "f"(x), [y] "f"(y)
                     : "memory");
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
        shz_vec4_t next;
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
        shz_dcache_alloc_line(points);
        points = project_end(points, a, &by) + 1;
        points = project_end(points, b, &by) + 1;
        a = project_begin(next, &by);
        a_near = !(a.z > by.near);
    }
    /* the last line holds a, and for an even count the last point after it */
    near_points += a_near;
    shz_dcache_alloc_line(points);
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
    shz_vec4_t dir = shz_vec4_init(light_dir.x, light_dir.y, light_dir.z, 0.0F);
    shz_vec4_t light;
    int left;

    /* light_dir in the model's space: its dot with each of XMTRX's first
     * three columns */
    light = shz_vec3_vec4(shz_vec4_dot3(dir, shz_xmtrx_read_col(0),
                                        shz_xmtrx_read_col(1),
                                        shz_xmtrx_read_col(2)),
                          0.0F);
    if (!(first_flags & NJD_FLAG_IGNORE_SPECULAR) &&
        first->specular.color != 0) {
        offsets += DRAW_PALETTE_ENTRIES * 2;
    }
    for (left = model->nbPoint; left > 3; left -= 4) {
        shz_vec4_t n;
        shz_vec2_t ab, cd;
        int a, b, c, d;

        /* a PREF for each pair of normals, 24 bytes, under a line, so that
         * the PREFs skip no line */
        prefetch_ahead(normal);
        prefetch_ahead(normal + 2);
        n = next_vector(&normal, 1.0F);
        ab = shz_vec4_dot2(light, n, next_vector(&normal, 1.0F));
        a = (int)(ab.x * HALF_255 + HALF_255);
        b = (int)(ab.y * HALF_255 + HALF_255);
        n = next_vector(&normal, 1.0F);
        cd = shz_vec4_dot2(light, n, next_vector(&normal, 1.0F));
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
        shz_vec4_t n = next_vector(&normal, 1.0F);
        shz_vec2_t ab = shz_vec4_dot2(light, n, next_vector(&normal, 1.0F));
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
        shz_vec4_t n = next_vector(&normal, 1.0F);
        int a = palette_entry((int)(zam_fipr(light, n) * HALF_255 + HALF_255));

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
    return sq + 1;
}

/* the u and v of an ENV_UV vertex of point index: its normal, turned by
 * XMTRX, as n * 0.5 + 0.5 */
static inline void env_uv(const struct strips *strips, int index, Float *u,
                          Float *v)
{
    const NJS_VECTOR *n = &strips->normals[index];
    shz_vec4_t turned = shz_xmtrx_transform_vec4(next_vector(&n, 0.0F));

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

    __asm__("mov.w @%[mesh]+,%[index]"
            : [mesh] "+r"(*mesh), [index] "=r"(index)
            :
            : "memory");
    return index;
}

/* the u and v at *vertuv over 255, and *vertuv stepped past them. a 16-bit
 * load by displacement writes only R0, which take_point holds for its index.
 * so the asm loads the two by post-increment, as model.src's strips_tex does.
 * it clobbers memory for the reason next_index gives */
static inline void next_uv(const NJS_TEX **vertuv, Float *u, Float *v)
{
    int tex_u, tex_v;

    __asm__("mov.w @%[vertuv]+,%[u]\n\t"
            "mov.w @%[vertuv]+,%[v]"
            : [vertuv] "+r"(*vertuv), [u] "=r"(tex_u), [v] "=r"(tex_v)
            :
            : "memory");
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
        shz_vec3_t turned =
            shz_xmtrx_transform_vec3(zam_load_vec3(&strips->normals[index]));

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
OUT_OF_LINE void draw_meshes(const NJS_MODEL *model, struct point *points,
                             Bool clipped)
{
    const NJS_MESHSET *meshset = model->meshsets;
    struct strips strips = {NULL, 0, points, NULL, model->normals};
    int meshsets;

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
        if (!(own & NJD_FLAG_USE_TEXTURE)) {
            sq = clipped ? clipped_untextured(sq, &strips)
                         : strips_untextured(sq, &strips);
        } else if (own & NJD_FLAG_USE_ENV) {
            sq = clipped ? clipped_env(sq, &strips) : strips_env(sq, &strips);
        } else {
            sq = clipped ? clipped_textured(sq, &strips)
                         : strips_textured(sq, &strips);
        }
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
    near_points = project_and_shade(model);
    if (near_points < model->nbPoint) {
        draw_meshes(model, (struct point *)_nj_vertex_buf_, near_points != 0);
    }
}

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
    return frame_step == 0 &&
           sphere_in_view(view_point(&center), radius, _nj_near_clip_, side,
                          side, _nj_scr_aspect_y_ * _nj_screen_.cx);
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
