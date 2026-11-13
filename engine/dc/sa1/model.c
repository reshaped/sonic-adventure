/* SA1's model walk on the Dreamcast, and the kernels it runs, on SH4ZAM's FTRV
 * and FIPR. the walk's entries are inlined into live.h's sa_present_live, for
 * SA1's basic models. batch.h's kernels project and light a model's points.
 * they test them against the near clip and the screen's edges too. the loops
 * that send a textured meshset's strips where no point is near are inline
 * assembly. the draws model.h names are SA1's compatibility layer,
 * src/draw/model.c, which replaces model.src. the layer keeps Ninja's state
 * and hands each draw here as an sa_draw_live. each of its calls compiles into
 * the entry the draw's state names. the walk reads Ninja's state only through
 * the names live.h lends, during the call alone. it names no symbol of Ninja's
 * or the game's. it reads a model's bytes during the call alone, as sa_dc.h's
 * SA_BORROW_CALL says.
 * model.src's project_points projects x by _nj_screen_.dist alone, and its
 * cull_model tests x by it. its near clip takes dist times _nj_scr_aspect_x_,
 * as Ninja's own projections do. the two scales agree at the shipped aspect of
 * 1. the port's 16:9 sets the picture's aspect, dreamcast.h's
 * port_draw_aspect, to 3/4. there model.src would draw each model a third too
 * wide. its cull_model would drop the models in the sides 16:9 adds. the draws
 * here take that aspect, as sa1_live_screen takes it, and model.src keeps its
 * bytes.
 *
 * where model.src computes a float from the same inputs, the code here takes
 * its operations in their order. so the result keeps model.src's bits.
 * model.src holds no FMAC, and make dc fuses no product into a sum here. some
 * of model.src's tests branch on a false FCMP/GT, which a NaN gives. the tests
 * here spell those !(a > b), which holds for a NaN where a <= b doesn't.
 *
 * apart from the aspect, the code here departs from model.src in these ways.
 * - the draws project and count a model's points alone. model.src's
 *   project_points takes the 12 bytes past an odd count as one more point. its
 *   @bug tag says so.
 * - a meshset that sends draw_unlit_offsets sends 0 for a point whose index is
 *   DRAW_UNLIT_OFFSETS or more. model.src's strips_tex reads that point's
 *   offset color past draw_unlit_offsets, by FMOV.S @(R0,R3),FR4 at
 *   0x8c0389f0. it finds there the offset color shade_points gave the point
 *   DRAW_UNLIT_OFFSETS before it, in its lit_offsets.
 * - near_clip sends nothing for a triangle left with under three corners. only
 *   a NaN or infinite z leaves so few. model.src's near clip fills the strip
 *   out with corners an earlier clip left.
 * - model_in_view's edge tests take the sphere's 1/z by FSRRA and pad the clip
 *   by SPHERE_PAD. so a model within the pad of an edge is drawn where
 *   model.src's cull_model drops it.
 * - a land draw sends nothing for a model that model_off_screen or
 *   points_off_screen finds off the screen.
 * - send_clipped_strips sends nothing for a triangle off the screen that
 *   near_clip would cut, by triangle_off_screen's test. model.src's near clip
 *   sends what is left of it at z >= 1.
 * - set_point_alpha writes nothing where an earlier call for the model left
 *   each point the alpha it gives. it skips that test for an
 *   NJD_FLAG_IGNORE_LIGHT material, which turns the points white. model.src's
 *   writes an alpha of 128 or more again, as its @bug tag says.
 * - project_points folds the screen's scales into XMTRX. so a point's x and y
 *   on the screen may differ from model.src's, as zam_xmtrx_project_points'
 *   comment says. for finite points and matrices its 1/z keeps model.src's
 *   bits.
 * - any_past_near takes a point's z by one FIPR with XMTRX's z row.
 *   model.src's draw_model_unculled turns the point by FTRV. a FIPR may round
 *   otherwise than an FTRV. a point within a rounding of the near clip may
 *   then count otherwise. sa1_live_land may then set a model's header words
 *   where model.src leaves them, or the reverse.
 * - a land draw's points take their palette entries from a bake, as
 *   BAKE_REACH's comment says. a point may then take the entry beside the one
 *   model.src's shade_points gives it.
 * - send_meshes sends a translucent meshset that shows_opaque finds opaque in
 *   the opaque list, under an opaque header. model.src sends it in the
 *   translucent list. the PVR shows its color where it lies nearest either
 *   way. the picture can part from model.src's by the blend's error at alpha
 *   255, if the blend has one. it parts too where an opaque polygon sent after
 *   the meshset, at its depth, shows in its place. it parts too where a
 *   translucent polygon sent before it, at its depth, shows over it. the
 *   translucent list, which the TA takes after the flip and the PVR sorts,
 *   loses the meshset.
 * - sa1_live_screen takes the screen's width and height too, for
 *   model_off_screen and off_edges. it takes the draws' scratch once, as
 *   view.scratch. model.src's project_points reads _nj_vertex_buf_ at each
 *   draw.
 * - sa1_live_forms can make a nested draw take Ninja's forms, unlit and
 *   translucent, as model.h says. such a draw shades no point, as
 *   project_like_ninja says. model.src has no such draws.
 *
 * model.src's other bugs that change what a draw sends stay, each under a @bug
 * tag. the walk drops model.src's uncalled code and its reads and writes past
 * an array that change nothing sent.
 *
 * ALWAYS_INLINE makes GCC inline a function whatever its inline limits.
 * next_vector, take_point and send_textured take it, and so do send_point,
 * send_mesh_uv_strip and send_env_uv_strip. so the loops over points, polygons
 * and vertices make no call. send_strips and send_clipped_strips take it, so
 * each strips_ and clipped_ function holds a copy with a constant uv. left to
 * choose, GCC compiles one send_clipped_strips out of line, with uv an
 * argument. the entries sa1_live_model, sa1_live_nested and sa1_live_land take
 * it, as live.h declares them. so each of the layer's draws holds its entry's
 * code. points_past takes it so that each of points_off_screen's four tests
 * hands points_above its plane's words. the land kernel then keeps its lines.
 *
 * OUT_OF_LINE keeps a function out of line under its own name, which GCC would
 * otherwise inline or clone. ld.py places each function a kernel row of
 * config/1st_read/kernel.txt names by its name, so each takes OUT_OF_LINE. so
 * do bake_make and bake_admit, so that shade_from_bake alone takes the land
 * kernel's lines. so does strips_culled, whose code GCC would make larger
 * inlined into draw_meshes_like_ninja, its one caller. so does
 * set_point_alpha_unlit, so that send_meshes' two copies each reach
 * point_alpha out of line. so does clipped_untextured, which
 * strips_untextured, its one caller, would take whole into the rare kernel's
 * window */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <sh4zam/shz_mem.h>
#include <sh4zam/shz_scalar.h>
#include <sh4zam/shz_vector.h>
#include <sh4zam/shz_xmtrx.h>

#include "engine/dc/batch.h"
#include "sa1/live.h"

#define OUT_OF_LINE static __attribute__((noinline, noclone))

/* the texture word's top byte, its fourth in memory. FORMAT_BITS of it hold
 * the pixel format. FORMAT_RGB565 is their value for kamui.h's
 * KM_TEXTURE_RGB565, whose texels hold no alpha */
#define TEXTURE_TOP(word) (((const Uint8 *)&(word))[3])
#define FORMAT_BITS 0x38
#define FORMAT_RGB565 (KM_TEXTURE_RGB565 << 3)

/* the ISP word's culling mode, as njkm.c names it. set whole, as Kamui's
 * KM_CULLCW, it drops a triangle that turns clockwise on the screen. Ninja's
 * _cull drawers drop the same */
#define ISP_CULL 0x18000000
/* the color, less its alpha, model.src gives a point of an
 * NJD_FLAG_IGNORE_LIGHT material in the translucent list */
#define UNLIT_WHITE 0x00ffffff
/* a list's write pointer, split for the store queues as njkm.c splits it.
 * SQ_ADDRESS keeps its low 26 bits, its address in the store queue area at
 * SQ_BASE. SQ_AREA moves its bits 26 to 28 into QACR0's and QACR1's area
 * field */
#define SQ_ADDRESS 0x03ffffff
#define SQ_BASE 0xe0000000
#define SQ_AREA(ptr) (((ptr) >> 24) & 0x1c)
#define QACR0 (*(volatile Uint32 *)0xff000038)
#define QACR1 (*(volatile Uint32 *)0xff00003c)

/* the flags of a triangle's points a, b and c in a clipped strip. a point is
 * flagged unless 0 < 1/z <= 1, which puts it at z >= 1. a NaN 1/z flags it */
#define FLAGGED_A 0x01
#define FLAGGED_B 0x02
#define FLAGGED_C 0x04
#define FLAGGED_ALL (FLAGGED_A | FLAGGED_B | FLAGGED_C)

/* off_edges counts a corner past an edge of the screen only where it lies
 * OFF_EDGE pixels past it. it counts one only within OFF_REACH of the eye, in
 * pixels at z = 1. rounding toward zero lands near_clip's cut between two such
 * corners under 4 pixels from its exact place. OFF_EDGE is twice that, so the
 * cut lies past the screen's edge too. the error grows with the corners'
 * distance from the eye, so no margin alone bounds it */
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

/* points_off_screen drops a model whose points all lie OFF_PLANE units past
 * one edge's plane, in view space. it tries only a model whose sphere of
 * LAND_REACH times r lies within PLANE_REACH units of the eye. within that
 * reach, rounding toward zero moves near_clip's cuts by under 16 * 2^-23 times
 * the corners' reach. with the reach in pixels at z = 1, that is under half a
 * unit toward the plane. it moves a projected point by less. points_past turns
 * the plane into the model's space and dots each point there. its test rounds
 * the same products as a projection, each once more, and moves by less too. so
 * all the draw would send lies past the edge */
#define OFF_PLANE 1.0F
#define PLANE_REACH 0x1p16F

/* the walk PREFs ZAM_PREFETCH_AHEAD bytes ahead of a polygon's indices and
 * uvs, as batch.h's kernels do. batch.h asks its caller to keep DMA from
 * writing what a PREF can reach. no DMA does while the draws run. the pads'
 * buffers are read through P2, and a stream's sectors lie kilobytes past its
 * work's output buffers */

/* where a point lies on the screen, and its 1/z */
struct screen {
    Float x, y;
    Float rhw;
};

/* a point as project_points leaves it in _nj_vertex_buf_, a vertex's first
 * four words. its control word is KM_VERTEXPARAM_NORMAL. its colors, a
 * vertex's last two words, lie past the points. so the textured strips move a
 * vertex as four FMOV.D pairs */
struct point {
    Uint32 control;
    struct screen at;
};

/* the base and offset colors shade_points gives a point, a vertex's last
 * two words */
struct colors {
    NJS_COLOR base;
    Uint32 offset;
};

/* the colors of count points at points, one pair each. shade_points writes
 * them just past the last point */
static inline struct colors *point_colors(struct point *points, int count)
{
    return (struct colors *)(points + count);
}
/* take_point reaches point index at index times 16, and a pair of points
 * fills a 32-byte cache line */
_Static_assert(sizeof(struct point) == 16, "a point isn't 16 bytes");

/* the operand cache, as live.h's OC_LINE, OC_SETS and OC_SIZE give it. code
 * reads its literal pools through it. config/1st_read/kernel.txt lays out
 * the code the draws run first, then the object walks', which call the
 * draws, then the sweep's, whose pools no draw reads. the draws' scratch in
 * _nj_vertex_buf_ starts at the first line in the set of polygon_sweep's
 * first whole line, or in set OC_SETS / 2 where that set lies under it,
 * or in set OC_SETS - 2 where that set is the cache's last.
 * project_points' first pass writes that line, the points start on the
 * next, and their colors follow them, so the scratch takes the sweep's
 * sets first, and the cache's first OC_SETS / 2 only past its last */

/* claim each line wholly inside [start, end) by MOVCA.L, which reads no
 * memory. as SH4ZAM's shz_dcache_alloc_line says, a claimed line's first word
 * holds what R0 held until written. its other words are undefined until then.
 * an interrupt's loads may evict a claimed line before the code writes it. the
 * cache then writes those words to RAM. the code's stores fetch the line
 * again, so each word they write lands. so a claim holds where the code writes
 * each word of the line before anything reads it. no other data may share the
 * line */
static inline void claim_lines(void *start, void *end)
{
    Uint32 line = (Uint32)end & ~(OC_LINE - 1);

    while (line - OC_LINE >= (Uint32)start) {
        line -= OC_LINE;
        shz_dcache_alloc_line((void *)line);
    }
}

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

/* a polygon header as the tile accelerator takes it. its first four words are
 * as _nj_parameterkm_ holds them. send_meshes, which opens each polygon,
 * leaves the other four unwritten, and texture too for an untextured
 * polygon */
struct polygon_header {
    Uint32 global, isp, tsp, texture;
    Uint32 unwritten[4];
};
/* a header fills a store queue, as a vertex does */
_Static_assert(sizeof(struct polygon_header) == sizeof(struct vertex),
               "a header isn't a vertex's size");

/* a triangle's corner as near_clip cuts it: its place in view space and what
 * its strip sends. its u and v are 255 times a vertex's, and send_corner
 * scales them by INV_255 */
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

/* a meshset as its strips send it. it holds the points its polygons index and
 * the model's normals for ENV_UV. colors holds the points' lit or unlit
 * colors, as send_meshes picks them */
struct strips {
    const NJS_MESHSET *meshset;
    /* a polygon's point count, or 0 where each polygon gives its own */
    int polygon_size;
    const struct point *points;
    const struct colors *colors;
    const NJS_VECTOR *normals;
};

/* how set_point_alpha takes a material's alpha. ALPHA_CONSTANT takes
 * _nj_constant_material_'s in its place, under
 * NJD_CONTROL_3D_CONSTANT_MATERIAL. ALPHA_OFFSET adds the two, under
 * NJD_CONTROL_3D_OFFSET_MATERIAL */
enum alpha_mode { ALPHA_OWN, ALPHA_CONSTANT, ALPHA_OFFSET };

/* a polygon's point count by meshset type, in the order NJD_MESHSET_3,
 * NJD_MESHSET_4, NJD_MESHSET_N and NJD_MESHSET_TRIMESH. the last two's
 * polygons each give their own */
static const int polygon_sizes[] = {3, 4, 0, 0};

/* an edge of the screen as a plane through the eye, with (across, depth) a
 * unit normal. a point in view space lies past it where across * x + depth * z
 * < 0. the top and bottom edges take y for x */
struct edge {
    Float across, depth;
};

/* what the draws project by, as sa1_live_screen copies it. the picture's
 * aspect stands in for Ninja's x aspect */
static struct {
    /* where the draws project a model's points, as sa1_live_screen sets it. it
     * lies beside screen, which each draw that projects reads. the two share a
     * line unless view starts in a line's last word. a static of its own lands
     * where the link puts it, which can be in the points' sets */
    struct point *scratch;
    /* the screen's center, dist times the picture's aspect and aspect_y, as
     * scale_x and scale_y, and the near clip */
    zam_screen_t screen;
    Float dist;
    Float aspect_y;
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

/* the edge where across * x + depth * z is 0, scaled to a unit normal */
static struct edge screen_edge(Float across, Float depth)
{
    Float inv = 1.0F / __builtin_sqrtf((across * across) + (depth * depth));
    struct edge edge = {across * inv, depth * inv};

    return edge;
}

/* copy the screen into view, with aspect_x for Ninja's x aspect, and make its
 * edges. a point lies on the screen where 0 <= x * scale_x / z + cx <= w. y
 * does the same with scale_y, cy and h */
static void sa1_live_screen(Float aspect_x, void *scratch)
{
    view.screen.cx = sa1_live_ninja_screen.cx;
    view.screen.cy = sa1_live_ninja_screen.cy;
    view.dist = sa1_live_ninja_screen.dist;
    view.aspect_y = sa1_live_aspect_y;
    view.screen.near = sa1_live_near_clip;
    view.screen.scale_x = aspect_x * view.dist;
    view.screen.scale_y = sa1_live_aspect_y * view.dist;
    view.inv_scale_x = 1.0F / view.screen.scale_x;
    view.inv_scale_y = 1.0F / view.screen.scale_y;
    view.left = screen_edge(view.screen.scale_x, view.screen.cx);
    view.right = screen_edge(-view.screen.scale_x,
                             sa1_live_ninja_screen.w - view.screen.cx);
    view.top = screen_edge(view.screen.scale_y, view.screen.cy);
    view.bottom = screen_edge(-view.screen.scale_y,
                              sa1_live_ninja_screen.h - view.screen.cy);
    view.w = sa1_live_ninja_screen.w;
    view.h = sa1_live_ninja_screen.h;
    view.scratch = scratch;
}

/* take the constant attributes and material mode live.h lends for the draws
 * that follow */
OUT_OF_LINE void take_constant_attr(void)
{
    Uint32 flags = sa1_live_control;

    constant_attr = (flags & NJD_CONTROL_3D_CONSTANT_ATTR) != 0;
    attr_and = sa1_live_attr_and;
    attr_or = sa1_live_attr_or;
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

/* p in view space, as XMTRX turns it, its words loaded by post-increment */
static inline shz_vec3_t view_point(const NJS_POINT3 *p)
{
    shz_vec4_t v = next_vector(&p, 1.0F);

    return shz_xmtrx_transform_point3(shz_vec3_init(v.x, v.y, v.z));
}

/* what the sphere tests pad the clip by, a pixel, which FLDI1 loads. FSRRA's
 * 1/z lies within 2^-21 of the exact reciprocal, by the SH-4 manual. that and
 * the tests' rounding move a projected edge by under 2^-19 of its distance
 * from the center. they add two units in the edge's last place. the sum is
 * under a pixel for a side of the clip within 2^18 pixels of the center. so
 * the pad keeps in view each sphere the exact divide keeps */
#define SPHERE_PAD 1.0F

/* whether the sphere of radius r about center, in view space, may be in view.
 * the sphere is out when it lies wholly nearer than near or past the far clip.
 * it is out too where an edge test puts it wholly past the 2D clip, padded by
 * SPHERE_PAD. the edge tests project the sphere's sides at its far z, center.z
 * + r, which lies past near. so FSRRA of its square gives its reciprocal, as
 * SH4ZAM's shz_invf_fsrra takes it. shz_inv_sqrtf_fsrra_sh4, the SH-4 back
 * end's entry, runs FSRRA whatever its argument */
static inline Bool sphere_in_view(shz_vec3_t center, Float r, Float near,
                                  Float side, Float bottom, Float top)
{
    const NJS_POINT2 *clip = sa1_live_clip;
    Float cx = sa1_live_ninja_screen.cx;
    Float cy = sa1_live_ninja_screen.cy;
    Float far;
    Float inv;

    if (!(center.z > near - r) || center.z > sa1_live_far_clip + r) {
        return FALSE;
    }
    far = center.z + r;
    inv = shz_inv_sqrtf_fsrra_sh4(far * far);
    side *= inv;
    bottom *= inv;
    top *= inv;
    if (!((center.x + r) * side + cx > clip[0].x - SPHERE_PAD) ||
        (center.x - r) * side + cx > clip[1].x + SPHERE_PAD) {
        return FALSE;
    }
    /* @bug logic: the bottom test compares y with the clip's right edge,
     * clip[1].x, not its bottom, clip[1].y. model.src loads the bottom into
     * FR9 and compares y with FR8, the right edge, in cull_model at
     * 0x8c03821c and 0x8c038224 */
    return (center.y + r) * top + cy > clip[0].y - SPHERE_PAD &&
           !((center.y - r) * bottom + cy > clip[1].x + SPHERE_PAD);
}

OUT_OF_LINE Bool model_in_view(const NJS_MODEL *model)
{
    /* @bug logic: the bottom test scales y by dist, not scale_y, and the top
     * test by aspect_y times the screen's cx. model.src's cull_model scales
     * the bottom test's y by dist in FMUL FR6,FR11 at 0x8c038220, and
     * multiplies aspect_y by cx for the top test's scale in FMUL FR4,FR7 at
     * 0x8c03820a */
    return sphere_in_view(view_point(&model->center), model->r,
                          view.screen.near, view.screen.scale_x, view.dist,
                          view.aspect_y * sa1_live_ninja_screen.cx);
}

/* whether a sphere of radius reach lies wholly past edge. across and z are its
 * center's in view space. across is its x for a side edge, and its y for the
 * top or bottom */
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

/* PREF the line ZAM_PREFETCH_AHEAD bytes past p. the address is an integer,
 * since it may lie past p's array. SH4ZAM's SHZ_PREFETCH clobbers no memory,
 * where machine.h's prefetch clobbers it for the store queues. but it is an
 * asm volatile, which GCC's scheduler moves no instruction across. so each
 * loop PREFs at the top of a pass, before its loads */
static inline void prefetch_ahead(const void *p)
{
    SHZ_PREFETCH((Uint32)p + ZAM_PREFETCH_AHEAD);
}

/* project model's points into points, onto view.screen, by
 * zam_xmtrx_project_points. return how many lie no farther than
 * view.screen.near. shade_points gives them their colors.
 *
 * its callers pass view.scratch, one or more whole lines into _nj_vertex_buf_.
 * system_boot_init's njInit3D sets that to texture_buffer, which src/main.c
 * aligns to 32 bytes. so the points start on a line, as the kernel requires,
 * and the line before them is free. their colors follow them, over the two
 * lines where the kernel keeps XMTRX's words while it runs. shade_points and
 * the strips read XMTRX unchanged, since the kernel puts it back. an interrupt
 * may come between the kernel's FRCHGs or FSCHGs. Shinobi's syIntMasterISR
 * saves FPSCR and both banks, runs under an FPSCR of its own, and puts them
 * back */
OUT_OF_LINE int project_points(const NJS_MODEL *model, struct point *points)
{
    return zam_xmtrx_project_points(&model->points->x, model->nbPoint,
                                    &view.screen, points,
                                    KM_VERTEXPARAM_NORMAL);
}

/* whether any of model's points, turned by XMTRX, dots the plane over bound.
 * the plane is (x, y, z, 0) in view space. the transpose of XMTRX turns the
 * plane into the model's space, where each point takes one FIPR */
OUT_OF_LINE Bool points_above(const NJS_MODEL *model, Float x, Float y, Float z,
                              Float bound)
{
    shz_vec4_t plane =
        zam_xmtrx_transform_vec4_transpose(shz_vec4_init(x, y, z, 0.0F));

    return zam_vec4_dot_any_above(plane, &model->points->x, model->nbPoint,
                                  bound);
}

/* whether any of model's points lies farther than view.screen.near, by its
 * z, its dot with XMTRX's z row */
OUT_OF_LINE Bool any_past_near(const NJS_MODEL *model)
{
    return points_above(model, 0.0F, 0.0F, 1.0F, view.screen.near);
}

/* whether each of model's points, turned by XMTRX, lies OFF_PLANE or more past
 * edge's plane. the plane takes a point's y where top_or_bottom is TRUE, and
 * its x for a side. a point whose dot is NaN, which only a NaN or infinite
 * point or matrix gives, counts past */
ALWAYS_INLINE Bool points_past(const NJS_MODEL *model, struct edge edge,
                               Bool top_or_bottom)
{
    return !points_above(model, top_or_bottom ? 0.0F : edge.across,
                         top_or_bottom ? edge.across : 0.0F, edge.depth,
                         -OFF_PLANE);
}

/* whether model's points all lie OFF_PLANE past the plane of an edge its
 * center lies past. it tests only where model's sphere of LAND_REACH times r
 * lies within PLANE_REACH of the eye. the center's |x| + |y| + |z| is at least
 * its distance */
OUT_OF_LINE Bool points_off_screen(const NJS_MODEL *model)
{
    shz_vec3_t center = view_point(&model->center);

    if (!(__builtin_fabsf(center.x) + __builtin_fabsf(center.y) +
              __builtin_fabsf(center.z) + (LAND_REACH * model->r) <
          PLANE_REACH)) {
        return FALSE;
    }
    return (past_edge(view.left, center.x, center.z, 0.0F) &&
            points_past(model, view.left, FALSE)) ||
           (past_edge(view.right, center.x, center.z, 0.0F) &&
            points_past(model, view.right, FALSE)) ||
           (past_edge(view.top, center.y, center.z, 0.0F) &&
            points_past(model, view.top, TRUE)) ||
           (past_edge(view.bottom, center.y, center.z, 0.0F) &&
            points_past(model, view.bottom, TRUE));
}

/* entry, held to the palette. a normal's dot with the light picks (int)(dot *
 * 127.5 + 127.5). so -1 picks entry 0, and 1 the last */
static inline int palette_entry(int entry)
{
    if (entry < 0) {
        entry = 0;
    }
    if (entry > SA_PALETTE_ENTRIES - 1) {
        entry = SA_PALETTE_ENTRIES - 1;
    }
    return entry;
}

/* a land model's bake: its points' palette entries under one light, kept
 * across pictures. the land doesn't move, and the light in view space turns
 * with the view. so in a land model's space the light is the same from any
 * view, but for rounding. the entries index the palette, so the palette and
 * the offset colors' choice stay the draw's. the bakes and their entries take
 * 30 KB of .bss */

/* a bake serves a draw whose light lies within BAKE_REACH of its own on each
 * axis. a point's dot then moves by under 3 * BAKE_REACH times its normal's
 * length. no normal of the game's images or model banks is 1.12 or longer. so
 * the entry moves by under 0.11. a point takes the entry shade_points would
 * give it, or the one beside it. a light that close could give either. its
 * color then moves by the palette's step between the two, which may be large.
 * the Past's fire swings palette 0 toward palette 4. palette 4's base colors
 * jump from white to black past entry 63. a light that holds still differs
 * from its bake's by the view's rounding alone. that rounding moves
 * shade_points' own entries from picture to picture as much */
#define BAKE_REACH 0x1p-12F
/* the slots, and how many from a model's first slot may hold its bakes and
 * seen lights. a slot holds one light. slots empty only all at once, in
 * sa1_live_forget. so a model's slots lie before the first empty one it
 * probes. of the acts' land, mountain_land0 needs the most slots, 341 */
#define BAKE_SLOTS 512
#define BAKE_PROBES 8
/* the room for entries in bake_entries, a byte a point. a bake that doesn't
 * fit in what is left forgets them all first, as a land's load does. of the
 * acts' land, icecap_act2_land bakes the most points under one light,
 * 16,542 */
#define BAKE_BYTES 0x5000
_Static_assert(SA_UNLIT_OFFSETS <= BAKE_BYTES,
               "a model's entries may not fit in bake_entries");
/* the entries past a bake's that zam_palette_pairs reads, which bake_entries
 * holds past its BAKE_BYTES */
#define BAKE_PAST 3
/* the bit of a slot's at that marks a light seen but not baked */
#define BAKE_SEEN 0x80000000
/* a bake holds each point's entry less BAKE_MIDDLE, as a signed byte. MOV.B's
 * sign extension then gives the entry's distance from the palette's middle
 * entry */
#define BAKE_MIDDLE 128
/* a slot: its model and the light in the model's space it holds. at is where
 * the bake's entries start in bake_entries. a light seen but not baked has
 * BAKE_SEEN and its pass's low 31 bits in at */
struct bake {
    const NJS_MODEL *model;
    Float x, y, z;
    Uint32 at;
};
/* GCC emits an object's no_reorder data first, in the file's order, and its
 * other data last definition first. so these five open the layer's .bss. the
 * walk's other data, defined ahead of the layer's palettes, lie past the
 * palettes */
/* TRUE while sa1_live_land projects and shades a land model. shade_points
 * reads it, since its four arguments already take R4 to R7 */
static Bool land_draw __attribute__((no_reorder));
/* the land's passes, which sa1_live_run counts */
static Uint32 bake_pass __attribute__((no_reorder));
/* the bytes of bake_entries' room the bakes fill */
static int bake_used __attribute__((no_reorder));
static Sint8 bake_entries[BAKE_BYTES + BAKE_PAST] __attribute__((no_reorder));
static struct bake bakes[BAKE_SLOTS] __attribute__((no_reorder));

/* model's first slot, before the mask: its address over 16. an NJS_MODEL
 * takes more than 16 bytes, so no two models share one */
static inline Uint32 bake_hash(const NJS_MODEL *model)
{
    return (Uint32)model >> 4;
}

void sa1_live_forget(void)
{
    int i;

    for (i = 0; i < BAKE_SLOTS; i++) {
        bakes[i].model = NULL;
    }
    bake_used = 0;
}

/* whether bake holds a light of model's within BAKE_REACH of (x, y, z) on each
 * axis. the light may be baked or only seen. a light with an infinite or NaN
 * word lies within BAKE_REACH of none */
static inline Bool bake_near(const struct bake *bake, const NJS_MODEL *model,
                             Float x, Float y, Float z)
{
    return bake->model == model && __builtin_fabsf(bake->x - x) < BAKE_REACH &&
           __builtin_fabsf(bake->y - y) < BAKE_REACH &&
           __builtin_fabsf(bake->z - z) < BAKE_REACH;
}

/* whether bake is model's, made for a light within BAKE_REACH of (x, y, z)
 * on each axis */
static inline Bool bake_made_near(const struct bake *bake,
                                  const NJS_MODEL *model, Float x, Float y,
                                  Float z)
{
    return bake_near(bake, model, x, y, z) && bake->at < BAKE_SEEN;
}

/* make model's bake for the light (x, y, z) in its space in the slot bake.
 * return the slot. where it doesn't fit in what bake_entries has left, it
 * forgets every bake and takes model's first slot. that slot then lies before
 * the first empty one. each point takes the entry shade_points gives an odd
 * point left over */
OUT_OF_LINE const struct bake *
bake_make(struct bake *bake, const NJS_MODEL *model, Float x, Float y, Float z)
{
    shz_vec4_t light = shz_vec4_init(x, y, z, 0.0F);
    int count = model->nbPoint;
    const NJS_VECTOR *normal = model->normals;
    Sint8 *entry;
    int i;

    if (bake_used + count > BAKE_BYTES) {
        sa1_live_forget();
        bake = &bakes[bake_hash(model) & (BAKE_SLOTS - 1)];
    }
    bake->model = model;
    bake->x = x;
    bake->y = y;
    bake->z = z;
    bake->at = bake_used;
    entry = &bake_entries[bake_used];
    bake_used += count;
    for (i = 0; i < count; i++) {
        shz_vec4_t n = next_vector(&normal, 1.0F);
        int a = palette_entry((int)(zam_fipr(light, n) * HALF_255 + HALF_255));

        entry[i] = (Sint8)(a - BAKE_MIDDLE);
    }
    return bake;
}

/* model's bake for the light (x, y, z) in its space from the slots it probes,
 * or NULL. a light is baked only where it shows again in a later pass of the
 * land. so a light that turns at each step makes no bake, as a Casinopolis
 * mirror ball's does. a light within BAKE_REACH of one seen in an earlier pass
 * is baked now, in that one's slot. a light no slot holds is seen now. it
 * takes the first slot model probes that is empty or holds a light an earlier
 * pass saw. failing that, it takes model's first slot. shade_points then
 * shades without a bake. a light seen in this pass, at another draw of model,
 * waits for the next */
OUT_OF_LINE const struct bake *bake_admit(const NJS_MODEL *model, Float x,
                                          Float y, Float z)
{
    Uint32 hash = bake_hash(model);
    Uint32 now = BAKE_SEEN | bake_pass;
    struct bake *spare = NULL;
    int i;

    for (i = 0; i < BAKE_PROBES; i++) {
        struct bake *bake = &bakes[(hash + i) & (BAKE_SLOTS - 1)];

        if (bake_near(bake, model, x, y, z)) {
            if (bake->at < BAKE_SEEN) {
                return bake;
            }
            return bake->at != now ? bake_make(bake, model, x, y, z) : NULL;
        }
        if (spare == NULL && (bake->model == NULL ||
                              (bake->at >= BAKE_SEEN && bake->at != now))) {
            spare = bake;
        }
        if (bake->model == NULL) {
            break;
        }
    }
    if (spare == NULL) {
        spare = &bakes[hash & (BAKE_SLOTS - 1)];
    }
    spare->model = model;
    spare->x = x;
    spare->y = y;
    spare->z = z;
    spare->at = now;
    return NULL;
}

/* color model's points at lit from its bake for the light (x, y, z) in its
 * space. bake_admit may make the bake now. return TRUE, or FALSE where
 * bake_admit gives none. each entry's offset color must lie just past its base
 * color in palette.
 *
 * zam_palette_pairs serves the points by pairs. for an odd count, the last
 * pair's second point takes the entry one past the bake's. as a signed byte,
 * it indexes the palette as any entry does. its colors land just past lit,
 * over unlit's first pair, which make_unlit writes before any strip reads it.
 * with the two its loop reads past the last pair's, zam_palette_pairs reads up
 * to BAKE_PAST past the bake's */
OUT_OF_LINE Bool shade_from_bake(const NJS_MODEL *model, struct colors *lit,
                                 const Uint32 *palette, Float x, Float y,
                                 Float z)
{
    const struct bake *bake = &bakes[bake_hash(model) & (BAKE_SLOTS - 1)];

    if (!bake_made_near(bake, model, x, y, z)) {
        bake = bake_admit(model, x, y, z);
        if (bake == NULL) {
            return FALSE;
        }
    }
    zam_palette_pairs(&bake_entries[bake->at], (Uint32 *)lit,
                      (model->nbPoint + 1) / 2, palette + BAKE_MIDDLE * 2);
    return TRUE;
}

/* sa1_live_light in the model's space, with a w of 0, by its dots with XMTRX's
 * first three columns */
static inline shz_vec4_t turn_light(void)
{
    shz_vec3_t turned = zam_xmtrx_transform_vec3_transpose(&sa1_live_light.x);

    return shz_vec4_init(turned.x, turned.y, turned.z, 0.0F);
}

/* shade each of model's points from sa1_live_palette. the dot of its normal
 * with sa1_live_light, turned into the model's space, picks the entry. the
 * entry's base and offset colors go past the points, as point_colors places
 * them. the offset colors come from the next palette when first has a specular
 * color and first_flags lack NJD_FLAG_IGNORE_SPECULAR. each normal loads with
 * a w of 1, which the light's w of 0 cancels in the dot.
 *
 * zam_vec4_dot_palette_pairs holds a pair's entries to the palette only when
 * one lies outside it. an odd point left over holds its entry by
 * palette_entry, with no test first. a land draw whose offset colors follow
 * its base colors takes them from shade_from_bake where it can */
OUT_OF_LINE void shade_points(const NJS_MODEL *model, struct point *points,
                              const NJS_MATERIAL *first, Uint32 first_flags)
{
    const NJS_VECTOR *normal = model->normals;
    const Uint32 *palette = sa1_live_palette;
    const Uint32 *offsets = palette + 1;
    int count = model->nbPoint;
    struct colors *lit = point_colors(points, count);
    shz_vec4_t light = turn_light();

    if (!(first_flags & NJD_FLAG_IGNORE_SPECULAR) &&
        first->specular.color != 0) {
        offsets += SA_PALETTE_ENTRIES * 2;
    }
    /* the passes write every color before anything reads one. so the lines
     * wholly under them are claimed, not filled */
    claim_lines(lit, lit + count);
    if (land_draw && offsets == palette + 1 &&
        shade_from_bake(model, lit, palette, light.x, light.y, light.z)) {
        return;
    }
    if (count > 1) {
        zam_vec4_dot_palette_pairs(light, (const Float **)&normal,
                                   (Uint32 **)&lit, count / 2, palette, offsets,
                                   SA_PALETTE_ENTRIES);
    }
    if (count % 2 != 0) {
        shz_vec4_t n = next_vector(&normal, 1.0F);
        int a = palette_entry((int)(zam_fipr(light, n) * HALF_255 + HALF_255));

        lit[0].base.color = palette[a * 2];
        lit[0].offset = offsets[a * 2];
    }
}

/* set the alpha in the base colors of count points at colors to material's, as
 * alpha_mode takes it. under NJD_FLAG_IGNORE_LIGHT in flags, the points take
 * unlit's red, green and blue too. unlit's alpha byte is 0. shared is the
 * alpha byte every point holds, or -1. return the alpha byte every point then
 * holds, or -1.
 *
 * without NJD_FLAG_IGNORE_LIGHT, nothing is written where shared or the first
 * point's alpha already holds the new byte. the first point's never matches an
 * alpha of 128 or more, as the @bug below says. with it, nothing is written
 * where the first point already holds its new color, unless write_all is TRUE.
 * set_point_alpha takes unlit as UNLIT_WHITE. set_point_alpha_unlit, which
 * draw_meshes_like_ninja's send_meshes calls, takes unlit from its caller with
 * write_all TRUE. an unshaded draw's first point may hold that color by chance
 * where the others don't */
ALWAYS_INLINE int point_alpha(struct colors *colors, int count,
                              const NJS_MATERIAL *material, Uint32 flags,
                              int shared, Uint32 unlit, Bool write_all)
{
    Float alpha = (Float)material->diffuse.argb.a * INV_255;
    int value;
    int i;

    if (alpha_mode == ALPHA_CONSTANT) {
        alpha = sa1_live_material.a;
    } else if (alpha_mode == ALPHA_OFFSET) {
        alpha += sa1_live_material.a;
        if (alpha > 1.0F) {
            alpha = 1.0F;
        }
        if (!(alpha > 0.0F)) {
            alpha = 0.0F;
        }
    }
    value = (int)(alpha * 255.0F);
    if (flags & NJD_FLAG_IGNORE_LIGHT) {
        Uint32 color = (Uint32)value << 24 | unlit;

        if (!write_all && colors[0].base.color == color) {
            return shared;
        }
        for (i = 0; i < count; i++) {
            colors[i].base.color = color;
        }
        return (Uint8)value;
    }
    if ((Uint8)value == shared) {
        return shared;
    }
    /* @bug logic: the first point's alpha compares with value as a signed
     * byte, so a value of 128 or more never matches. every point then takes
     * it again. model.src's MOV.B @R1,R8 at 0x8c0388e2 sign-extends it for
     * CMP/EQ at 0x8c0388e4 */
    if ((Sint8)colors[0].base.argb.a == value) {
        return shared;
    }
    for (i = 0; i < count; i++) {
        colors[i].base.argb.a = (Uint8)value;
    }
    return (Uint8)value;
}

OUT_OF_LINE int set_point_alpha(struct colors *colors, int count,
                                const NJS_MATERIAL *material, Uint32 flags,
                                int shared)
{
    return point_alpha(colors, count, material, flags, shared, UNLIT_WHITE,
                       FALSE);
}

LINKED_LAST(set_point_alpha_unlit)
OUT_OF_LINE int set_point_alpha_unlit(struct colors *colors, int count,
                                      const NJS_MATERIAL *material,
                                      Uint32 flags, int shared, Uint32 unlit)
{
    return point_alpha(colors, count, material, flags, shared, unlit, TRUE);
}

/* set the TSP word's shading back to MODULATE_ALPHA, which set_header may
 * have cleared */
static inline void reset_shading(void)
{
    sa1_live_header.TSPPARAMBUFFER |= MODULATE_ALPHA;
}

/* a textured vertex's x, y, 1/z and colors. each lies in an FR register, as
 * model.src's strip loops hold them. a color's Float holds its word's bits,
 * and nothing here does arithmetic on it */
struct vertex_words {
    Float x, y, rhw;
    Float base, offset;
};

/* the words of point index of strips, for the clipped strips' single stores.
 * "z" puts index in R0, the only register FMOV.S @(R0,Rn) indexes by. index
 * times 8 reaches its base color, with its offset color 4 bytes on. index
 * times 16 reaches the point, whose words past its control word load by
 * post-increment */
ALWAYS_INLINE struct vertex_words take_point(const struct strips *strips,
                                             int index)
{
    struct vertex_words p;

    __asm__("shll2 %[k]\n\t"
            "add %[k],%[k]\n\t"
            "fmov.s @(%[k],%[colors]),%[base]\n\t"
            "add #4,%[k]\n\t"
            "fmov.s @(%[k],%[colors]),%[offset]\n\t"
            "add %[k],%[k]\n\t"
            "add #-4,%[k]\n\t"
            "add %[points],%[k]\n\t"
            "fmov.s @%[k]+,%[x]\n\t"
            "fmov.s @%[k]+,%[y]\n\t"
            "fmov.s @%[k]+,%[rhw]"
            : [k] "+z"(index), [x] "=f"(p.x), [y] "=f"(p.y), [rhw] "=f"(p.rhw),
              [base] "=&f"(p.base), [offset] "=&f"(p.offset)
            : [colors] "r"(strips->colors), [points] "r"(strips->points),
              "m"(*(const struct colors(*)[])strips->colors),
              "m"(*(const struct point(*)[])strips->points));
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

/* write a textured vertex at sq from p, u and v, and send it. return the
 * address past it. FMOV.S takes no displacement, so the asm writes down from
 * the vertex's end, as model.src's strips_tex does. its PREF sends the queue.
 * it names no memory: it writes only the store queue area, which no C reads */
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

/* the u and v of an ENV_UV vertex of point index, from its normal turned by
 * XMTRX */
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
        return send_untextured(sq, control, &point->at,
                               strips->colors[index].base.color);
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
 * rather than take **mesh as an "m" input. with that input, GCC copies *mesh
 * into another register around each load */
static inline int next_index(const Sint16 **mesh)
{
    int index;

    __asm__("mov.w @%[mesh]+,%[index]"
            : [mesh] "+r"(*mesh), [index] "=r"(index)
            :
            : "memory");
    return index;
}

/* send the point at *mesh of strips untextured, with control, and step *mesh
 * past it */
static inline struct vertex *send_next(struct vertex *sq, Uint32 control,
                                       const struct strips *strips,
                                       const Sint16 **mesh)
{
    return send_point(sq, control, strips, NO_UV, next_index(mesh), NULL, 0);
}

/* send the polygon of count points at *mesh as a strip of MESH_UV vertices.
 * step *mesh and *vertuv past it, and return the address past the strip. count
 * must be 2 or more. a vertex's u and v load from *vertuv's next pair as one
 * word, u in its low half. each converts by FLOAT and an FMUL by INV_255.
 *
 * under FPSCR.SZ, which the first FSCHG sets and the last clears, each FMOV
 * moves a pair of words. the first word of a pair lies in the even register.
 * DR0 holds control and x, and DR2 holds y and 1/z. DR4 holds u and v, and DR6
 * the point's base and offset colors. a pass of the loop stores vertex i as
 * four pairs, down from its end. it loads and converts vertex i + 1 into the
 * same registers, each after i's store from it. its colors load at the index
 * times 8 into colors. its point's words load as two pairs, at the index times
 * 16 into points and points + 8. each index is made two cycles before its
 * load. the SH7750 manual's figure 8.3 (e) shows a load stalling on an address
 * made the cycle before. by the manual's issue rules, a pass then issues in 14
 * cycles but for misses. v's FMUL sits in the delay slot. v's LDS still waits
 * a cycle on the EXTS.W before it. the last vertex writes
 * KM_VERTEXPARAM_ENDOFSTRIP over its control word before its PREF. the asm's
 * memory operands name what it reads: the polygon's indices and uvs and
 * strips' points and colors. it writes only the store queue area, as
 * send_textured does */
ALWAYS_INLINE struct vertex *send_mesh_uv_strip(struct vertex *sq,
                                                const struct strips *strips,
                                                int count, const Sint16 **mesh,
                                                const NJS_COLOR **vertuv)
{
    int left = count - 1;
    int k, pair, tex;
    Uint32 points8 = (Uint32)strips->points + 8;

    __asm__ volatile(
        "fschg\n\t"
        "mov.w @%[mesh]+,%[k]\n\t"
        "mov.l @%[vertuv]+,%[pair]\n\t"
        "shll2 %[k]\n\t"
        "exts.w %[pair],%[tex]\n\t"
        "shlr16 %[pair]\n\t"
        "lds %[tex],fpul\n\t"
        "add %[k],%[k]\n\t"
        "exts.w %[pair],%[pair]\n\t"
        "float fpul,fr4\n\t"
        "lds %[pair],fpul\n\t"
        "fmov @(%[k],%[colors]),dr6\n\t"
        "add %[k],%[k]\n\t"
        "float fpul,fr5\n\t"
        "fmov @(%[k],%[points]),dr0\n\t"
        "fmul %[scale],fr4\n\t"
        "fmov @(%[k],%[points8]),dr2\n\t"
        "fmul %[scale],fr5\n\t"
        "add #32,%[sq]\n\t"
        "1:\n\t"
        "mov.l @%[vertuv]+,%[pair]\n\t"
        "dt %[left]\n\t"
        "mov.w @%[mesh]+,%[k]\n\t"
        "fmov dr6,@-%[sq]\n\t"
        "exts.w %[pair],%[tex]\n\t"
        "fmov dr4,@-%[sq]\n\t"
        "shll2 %[k]\n\t"
        "lds %[tex],fpul\n\t"
        "add %[k],%[k]\n\t"
        "fmov dr2,@-%[sq]\n\t"
        "float fpul,fr4\n\t"
        "fmov @(%[k],%[colors]),dr6\n\t"
        "shlr16 %[pair]\n\t"
        "fmov dr0,@-%[sq]\n\t"
        "exts.w %[pair],%[pair]\n\t"
        "lds %[pair],fpul\n\t"
        "add %[k],%[k]\n\t"
        "pref @%[sq]\n\t"
        "float fpul,fr5\n\t"
        "fmov @(%[k],%[points]),dr0\n\t"
        "fmul %[scale],fr4\n\t"
        "fmov @(%[k],%[points8]),dr2\n\t"
        "add #64,%[sq]\n\t"
        "bf.s 1b\n\t"
        "fmul %[scale],fr5\n\t"
        "fmov dr6,@-%[sq]\n\t"
        "fmov dr4,@-%[sq]\n\t"
        "fmov dr2,@-%[sq]\n\t"
        "fmov dr0,@-%[sq]\n\t"
        "mov.l %[end],@%[sq]\n\t"
        "pref @%[sq]\n\t"
        "add #32,%[sq]\n\t"
        "fschg"
        : [sq] "+r"(sq), [mesh] "+r"(*mesh), [vertuv] "+r"(*vertuv),
          [left] "+r"(left), [k] "=&z"(k), [pair] "=&r"(pair), [tex] "=&r"(tex)
        : [colors] "r"(strips->colors), [points] "r"(strips->points),
          [points8] "r"(points8), [end] "r"(KM_VERTEXPARAM_ENDOFSTRIP),
          [scale] "f"(INV_255), "m"(*(const Sint16(*)[])(*mesh)),
          "m"(*(const NJS_COLOR(*)[])(*vertuv)),
          "m"(*(const struct colors(*)[])strips->colors),
          "m"(*(const struct point(*)[])strips->points)
        : "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "fpul", "t");
    return sq;
}

/* send the polygon of count points at *mesh as a strip of ENV_UV vertices.
 * step *mesh past it, and return the address past the strip. count must be 2
 * or more. each vertex's u and v are env_uv's, by FTRV, then FMUL and FADD by
 * 0.5.
 *
 * FPSCR.SZ is set as send_mesh_uv_strip sets it, except while a normal loads
 * by three FMOV.S. a normal is aligned only to 4 bytes. a turned normal moves
 * by one FMOV.D from DR8 to DR4, which then holds (u, v). a pass of the loop
 * scales vertex i's turned normal and turns vertex i + 1's in FV8. that FTRV
 * runs under i's stores and the next pass's start. i + 1's index loads in the
 * pass before. the last pass branches past that load, so the asm reads no
 * index past the polygon's. vertex 0 loads before the loop, and the last
 * vertex scales and stores after it. the asm turns each normal in FV8, FR8 to
 * FR11, which it clobbers. it clobbers memory too: named as a memory operand,
 * strips' normals cost strips_culled spills */
ALWAYS_INLINE struct vertex *send_env_uv_strip(struct vertex *sq,
                                               const struct strips *strips,
                                               int count, const Sint16 **mesh)
{
    int left = count - 1;
    int k;
    const NJS_VECTOR *n;
    Uint32 points8 = (Uint32)strips->points + 8;

    __asm__ volatile(
        "mov.w @%[mesh]+,%[k]\n\t"
        "shll2 %[k]\n\t"
        "mov %[k],%[n]\n\t"
        "add %[k],%[n]\n\t"
        "add %[k],%[n]\n\t"
        "add %[normals],%[n]\n\t"
        "fmov.s @%[n]+,fr8\n\t"
        "fmov.s @%[n]+,fr9\n\t"
        "fmov.s @%[n]+,fr10\n\t"
        "fldi0 fr11\n\t"
        "ftrv xmtrx,fv8\n\t"
        "add %[k],%[k]\n\t"
        "fschg\n\t"
        "fmov @(%[k],%[colors]),dr6\n\t"
        "add %[k],%[k]\n\t"
        "fmov @(%[k],%[points]),dr0\n\t"
        "fmov @(%[k],%[points8]),dr2\n\t"
        "add #32,%[sq]\n\t"
        "mov.w @%[mesh]+,%[k]\n\t"
        "1:\n\t"
        "fmov dr8,dr4\n\t"
        "shll2 %[k]\n\t"
        "fmul %[half],fr4\n\t"
        "mov %[k],%[n]\n\t"
        "fmul %[half],fr5\n\t"
        "add %[k],%[n]\n\t"
        "fmov dr6,@-%[sq]\n\t"
        "add %[k],%[n]\n\t"
        "add %[normals],%[n]\n\t"
        "fschg\n\t"
        "fmov.s @%[n]+,fr8\n\t"
        "fadd %[half],fr4\n\t"
        "fmov.s @%[n]+,fr9\n\t"
        "fadd %[half],fr5\n\t"
        "fmov.s @%[n]+,fr10\n\t"
        "add %[k],%[k]\n\t"
        "fldi0 fr11\n\t"
        "fschg\n\t"
        "ftrv xmtrx,fv8\n\t"
        "dt %[left]\n\t"
        "fmov dr4,@-%[sq]\n\t"
        "fmov dr2,@-%[sq]\n\t"
        "fmov dr0,@-%[sq]\n\t"
        "pref @%[sq]\n\t"
        "fmov @(%[k],%[colors]),dr6\n\t"
        "add %[k],%[k]\n\t"
        "fmov @(%[k],%[points]),dr0\n\t"
        "add #64,%[sq]\n\t"
        "fmov @(%[k],%[points8]),dr2\n\t"
        "bt 3f\n\t"
        "bra 1b\n\t"
        "mov.w @%[mesh]+,%[k]\n\t"
        "3:\n\t"
        "fmov dr8,dr4\n\t"
        "fmul %[half],fr4\n\t"
        "fmul %[half],fr5\n\t"
        "fadd %[half],fr4\n\t"
        "fadd %[half],fr5\n\t"
        "fmov dr6,@-%[sq]\n\t"
        "fmov dr4,@-%[sq]\n\t"
        "fmov dr2,@-%[sq]\n\t"
        "fmov dr0,@-%[sq]\n\t"
        "mov.l %[end],@%[sq]\n\t"
        "pref @%[sq]\n\t"
        "add #32,%[sq]\n\t"
        "fschg"
        : [sq] "+r"(sq), [mesh] "+r"(*mesh), [left] "+r"(left), [k] "=&z"(k),
          [n] "=&r"(n)
        : [colors] "r"(strips->colors), [points] "r"(strips->points),
          [points8] "r"(points8), [normals] "r"(strips->normals),
          [end] "r"(KM_VERTEXPARAM_ENDOFSTRIP), [half] "f"(0.5F)
        : "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "fr8", "fr9",
          "fr10", "fr11", "t", "memory");
    return sq;
}

/* the point count of the polygon at *mesh. where the polygon gives its own,
 * *mesh steps past it */
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
 * from is copied into a local, which no store to the store queues can alias. a
 * polygon must have two points or more. with fewer, its loop can run on past
 * its points, as model.src's DT loop does. every polygon of the game's models
 * has three or more.
 * where cull is TRUE, the strips go to a header that culls by ISP_CULL. the
 * hardware takes a strip's first triangle as turning as its points run. it
 * takes each after it the other way. Ninja's _cull drawers take each triangle
 * of a trimesh strip under NJD_TRIMESH_END as turning the other way. so such a
 * strip goes with its first point twice. the triangle of the two copies covers
 * nothing, and each after it turns as Ninja's test takes it. no other
 * polygon's count holds NJD_TRIMESH_END */
ALWAYS_INLINE struct vertex *send_strips_cull(struct vertex *sq,
                                              const struct strips *from,
                                              enum uv uv, Bool cull)
{
    const struct strips copy = *from;
    const struct strips *strips = &copy;
    const Sint16 *mesh = strips->meshset->meshes;
    const NJS_COLOR *vertuv = uv == MESH_UV ? strips->meshset->vertuv : NULL;
    int polygons;

    for (polygons = strips->meshset->nbMesh; polygons != 0; polygons--) {
        Bool reversed =
            cull && strips->polygon_size == 0 && (*mesh & NJD_TRIMESH_END);
        int count;

        prefetch_polygon(mesh, uv, vertuv);
        count = next_polygon_size(strips, &mesh);
        if (reversed) {
            sq = send_point(sq, KM_VERTEXPARAM_NORMAL, strips, uv, mesh[0],
                            vertuv, 0);
        }
        if (uv == MESH_UV) {
            sq = send_mesh_uv_strip(sq, strips, count, &mesh, &vertuv);
        } else if (uv == ENV_UV) {
            sq = send_env_uv_strip(sq, strips, count, &mesh);
        } else {
            int left = count - 1;

            do {
                sq = send_next(sq, KM_VERTEXPARAM_NORMAL, strips, &mesh);
            } while (--left != 0);
            sq = send_next(sq, KM_VERTEXPARAM_ENDOFSTRIP, strips, &mesh);
        }
    }
    return sq;
}

ALWAYS_INLINE struct vertex *send_strips(struct vertex *sq,
                                         const struct strips *from, enum uv uv)
{
    return send_strips_cull(sq, from, uv, FALSE);
}

/* set corner from point index of strips, with its x, y and z taken back into
 * view space. a MESH_UV corner takes vertuv's u and v. an ENV_UV corner takes
 * its point's normal, turned by XMTRX, as n * 127.5 + 127.5 */
static inline void take_corner(struct corner *corner,
                               const struct strips *strips, enum uv uv,
                               int index, const NJS_COLOR *vertuv)
{
    const struct point *point = &strips->points[index];

    corner->at = point->at;
    corner->base = strips->colors[index].base.color;
    corner->offset = strips->colors[index].offset;
    corner->z = 1.0F / point->at.rhw;
    corner->x = (point->at.x - view.screen.cx) * corner->z * view.inv_scale_x;
    corner->y = (point->at.y - view.screen.cy) * corner->z * view.inv_scale_y;
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
 * point. a channel moves by bits 16 to 31 of (b - a) * t, as a signed
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

/* set cut where the edge from a to b crosses z = 1, projected by scale_x and
 * scale_y. its colors blend a's toward b's by t in 16.16 fixed point. an
 * untextured cut takes no u or v */
static void cut_edge(struct corner *cut, const struct corner *a,
                     const struct corner *b, enum uv uv)
{
    Float t = (1.0F - a->z) / (b->z - a->z);
    Sint32 fraction = (Sint32)(t * 65536.0F);
    Float x = (b->x - a->x) * t + a->x;
    Float y = (b->y - a->y) * t + a->y;

    cut->at.x = x * view.screen.scale_x + view.screen.cx;
    cut->at.y = y * view.screen.scale_y + view.screen.cy;
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

/* clip the triangle of points index[0] to index[2] of strips at z = 1. send
 * what is left as a strip of its own. vertuv holds the points' u and v for
 * MESH_UV. a corner at z = 1 exactly isn't kept. the cut on its edge from a
 * nearer corner stands for it. nothing is sent when no corner lies past z = 1.
 * nor is anything sent when under three corners are left, which only a NaN or
 * infinite z leaves */
OUT_OF_LINE struct vertex *near_clip(struct vertex *sq,
                                     const struct strips *strips, enum uv uv,
                                     const Sint16 *index,
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
    /* each edge runs from the corner before. a cut lies on it when one end is
     * nearer than z = 1 and the other isn't */
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
    /* a strip covers a quad's corners in the order 0, 1, 3, 2 */
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

/* the edges of the screen p lies past, as OFF_ bits. take_corner rebuilds from
 * p the point at z = 1 / p->rhw. its x is (p->x - cx) * z / scale_x. (p->x +
 * OFF_EDGE) * p->rhw is exactly (scale_x * x + (cx + OFF_EDGE) * z) / z^2. its
 * sign gives the point's side of a plane through the eye, OFF_EDGE pixels past
 * the left edge. it does so at either sign of z. rounding toward zero keeps
 * that sign or gives 0, which counts past no edge. the other three edges go
 * the same way. p lies past none beyond OFF_REACH, or where a word of it is a
 * NaN or infinite. view.w in the reach test bounds |z| under OFF_REACH /
 * view.w */
static inline int off_edges(const struct screen *p)
{
    Float rhw = p->rhw;
    int edges = 0;

    if (!(__builtin_fabsf(p->x - view.screen.cx) +
              __builtin_fabsf(p->y - view.screen.cy) + view.w <
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

/* whether the triangle of points index[0] to index[2] of strips has every
 * corner past one edge, by off_edges. the whole triangle then lies past that
 * edge's plane. the part near_clip would keep, at z >= 1, projects past the
 * edge */
OUT_OF_LINE Bool triangle_off_screen(const struct strips *strips,
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

/* send strips as send_strips does, for a model with near points. a polygon of
 * n points goes as n - 2 triangles. triangle i's a, b and c are the polygon's
 * points i to i + 2. a triangle with no point flagged goes on in the strip
 * being sent. one with a point flagged ends that strip at b, and near_clip
 * clips it. it is dropped instead when all three are flagged, or when it lies
 * off the screen by triangle_off_screen.
 * where cull is TRUE, Ninja's test takes a triangle at an odd i as turned. in
 * a strip under NJD_TRIMESH_END, it takes one at an even i as turned instead.
 * a strip that starts on a turned triangle goes with its a twice. near_clip
 * takes a turned triangle's corners as b, a and c */
ALWAYS_INLINE struct vertex *send_clipped_strips_cull(struct vertex *sq,
                                                      const struct strips *from,
                                                      enum uv uv, Bool cull)
{
    const struct strips copy = *from;
    const struct strips *strips = &copy;
    const Sint16 *mesh = strips->meshset->meshes;
    const NJS_COLOR *vertuv = uv == MESH_UV ? strips->meshset->vertuv : NULL;
    int polygons;

    for (polygons = strips->meshset->nbMesh; polygons != 0; polygons--) {
        int reversed =
            cull && strips->polygon_size == 0 && (*mesh & NJD_TRIMESH_END);
        Bool in_strip = FALSE;
        int size, flags;
        int i;

        prefetch_polygon(mesh, uv, vertuv);
        size = next_polygon_size(strips, &mesh);
        flags =
            point_flag(strips, mesh[0]) | (point_flag(strips, mesh[1]) << 1);
        for (i = 0; i + 2 < size; i++) {
            Bool turned = cull && ((i ^ reversed) & 1) != 0;

            flags |= point_flag(strips, mesh[i + 2]) << 2;
            if (flags == 0) {
                if (!in_strip) {
                    if (turned) {
                        sq = send_point(sq, KM_VERTEXPARAM_NORMAL, strips, uv,
                                        mesh[i], vertuv, i);
                    }
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
                    if (turned) {
                        Sint16 bac[3] = {mesh[i + 1], mesh[i], mesh[i + 2]};
                        NJS_COLOR uvs[3];

                        if (uv == MESH_UV) {
                            uvs[0] = vertuv[i + 1];
                            uvs[1] = vertuv[i];
                            uvs[2] = vertuv[i + 2];
                        }
                        sq = near_clip(sq, strips, uv, bac,
                                       uv == MESH_UV ? uvs : NULL);
                    } else {
                        sq = near_clip(sq, strips, uv, &mesh[i],
                                       uv == MESH_UV ? &vertuv[i] : NULL);
                    }
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

ALWAYS_INLINE struct vertex *
send_clipped_strips(struct vertex *sq, const struct strips *from, enum uv uv)
{
    return send_clipped_strips_cull(sq, from, uv, FALSE);
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

static struct vertex *clipped_env(struct vertex *sq,
                                  const struct strips *strips)
{
    return send_clipped_strips(sq, strips, ENV_UV);
}

/* an untextured meshset's strips, clipped where clipped is TRUE. few meshsets
 * are untextured, so the test of clipped lies here, in kernel.txt's rare
 * kernel. there it takes none of the mesh kernel's window */
OUT_OF_LINE struct vertex *
strips_untextured(struct vertex *sq, const struct strips *strips, Bool clipped)
{
    if (clipped) {
        return clipped_untextured(sq, strips);
    }
    return send_strips(sq, strips, NO_UV);
}

/* a meshset's strips for draw_meshes_like_ninja's header that culls by
 * ISP_CULL. they are clipped where clipped is TRUE, with uv's u and v. it
 * takes uv as an argument, where the other strips functions are one for each
 * uv */
LINKED_LAST(strips_culled)
OUT_OF_LINE struct vertex *strips_culled(struct vertex *sq,
                                         const struct strips *strips,
                                         Bool clipped, enum uv uv)
{
    if (clipped) {
        return send_clipped_strips_cull(sq, strips, uv, TRUE);
    }
    return send_strips_cull(sq, strips, uv, TRUE);
}

/* the color, less its alpha, that Ninja's njDrawModel packs from the constant
 * material. it rounds each channel toward zero, as sdk/ninja/njModel.c's
 * pack_argb_body does */
static Uint32 constant_rgb(void)
{
    const NJS_ARGB *material = &sa1_live_material;
    const Float channels[3] = {material->r, material->g, material->b};
    Uint32 rgb = 0;
    int i;

    for (i = 0; i < 3; i++) {
        Float c = channels[i];

        if (0.0F > c) {
            c = 0.0F;
        } else if (c > 1.0F) {
            c = 1.0F;
        }
        rgb = rgb << 8 | (Uint32)(Sint32)(c * 255.0F);
    }
    return rgb;
}

/* give count points their unlit pairs at unlit. each takes its base color from
 * lit's pair and its offset color from the unlit offsets. the loop writes
 * every pair before anything reads one. so the lines wholly under them are
 * claimed, not filled */
OUT_OF_LINE void make_unlit(const struct colors *lit, struct colors *unlit,
                            int count)
{
    const Uint32 *offsets =
        &sa1_live_colors[SA_PALETTES * SA_PALETTE_ENTRIES * 2];
    int i;

    claim_lines(unlit, unlit + count);
    for (i = 0; i < count; i++) {
        NJS_COLOR base = lit[i].base;
        Uint32 offset = offsets[i];

        unlit[i].base = base;
        unlit[i].offset = offset;
    }
}

/* the unlit pairs of count points at unlit. make_unlit writes them from lit's
 * unless *made says unlit holds them */
ALWAYS_INLINE const struct colors *unlit_pairs(const struct colors *lit,
                                               struct colors *unlit, int count,
                                               Bool *made)
{
    if (!*made) {
        make_unlit(lit, unlit, count);
        *made = TRUE;
    }
    return unlit;
}

/* TRUE where every pixel of a translucent meshset shows alpha 1, so its blend
 * gives its own color. it holds where shared, the alpha byte every point
 * holds, is 255. drawn must also blend by source alpha over its inverse, and
 * the header's texture word name KM_TEXTURE_RGB565. an untextured meshset
 * leaves the last texture's word there, and shows alpha 1 under any. the test
 * takes drawn's blend factors from its top six bits, and the format by
 * TEXTURE_TOP. so it loads no constant into kernel.txt's mesh window. the
 * models draw under the contexts njInitContext sets up, which compare depth by
 * KM_GREATEREQUAL and write it. the game calls nothing that changes either. so
 * such a meshset shows in the opaque list where it does in the translucent */
static inline Bool shows_opaque(Uint32 drawn, int shared)
{
    return shared == 0xff &&
           drawn >> TSP_DST_SHIFT ==
               (SRC(BLEND_SRC_ALPHA) | DST(BLEND_INV_SRC_ALPHA)) >>
                   TSP_DST_SHIFT &&
           (TEXTURE_TOP(sa1_live_header.TexturePARAMBUFFER) & FORMAT_BITS) ==
               FORMAT_RGB565;
}

/* draw each of model's meshsets as a polygon of strips. the strips go through
 * the near clip where clipped is TRUE, as for a model with near points. the
 * header takes the material's flags as drawn_flags gives them, in drawn. the
 * strips choose their uv by its attrflags, in own, as model.src's do.
 * NJD_FLAG_USE_ALPHA sends a meshset to the translucent list, after
 * set_point_alpha. a meshset that shows_opaque finds opaque goes to the opaque
 * list instead, under an opaque header. a textured meshset under
 * NJD_FLAG_IGNORE_LIGHT sends the unlit offsets as its offset colors. when
 * opaque, it takes rules.h's MODE_DECAL from drawn_mode. its textured strips
 * take the unlit pairs make_unlit writes past the lit ones. so they send
 * through the same strips as a lit meshset's. the pairs are made at the first
 * such meshset, and again after set_point_alpha may change a base color.
 *
 * rules.h's set_header writes the header words. opening each polygon writes
 * and sends what njkm.c's njStart functions do. an opaque polygon's TSP word
 * takes TSP_OPAQUE in place of its blend bits. an untextured polygon leaves
 * the texture word's place in the store queue as it was. closing it writes
 * what njEndVertex does.
 *
 * where ninja is TRUE, the draws are draw_meshes_like_ninja's */
ALWAYS_INLINE void send_meshes(const NJS_MODEL *model, struct point *points,
                               Bool clipped, Uint32 unlit_rgb, Bool ninja)
{
    const NJS_MESHSET *meshset = model->meshsets;
    struct colors *lit = point_colors(points, model->nbPoint);
    struct colors *unlit = lit + model->nbPoint;
    struct strips strips = {NULL, 0, points, lit, model->normals};
    struct nj_parameterkm *header = &sa1_live_header;
    /* the alpha byte every point holds, or -1 */
    int shared = -1;
    /* TRUE while unlit holds each point's base color as lit does */
    Bool unlit_made = FALSE;
    int meshsets;

    for (meshsets = model->nbMeshset; meshsets != 0; meshsets--, meshset++) {
        Uint32 type = meshset->type_matId;
        const NJS_MATERIAL *material = &model->mats[type & MATERIAL_ID];
        Uint32 drawn = drawn_flags(material);
        Uint32 own = material->attrflags;
        Bool trans = (drawn & NJD_FLAG_USE_ALPHA) != 0;
        Bool cull = ninja && !(drawn & NJD_FLAG_DOUBLE_SIDE);
        Uint32 **list = &header->opaque_polygon;
        Uint32 mode = drawn_mode(drawn);
        Uint32 tsp;
        Uint32 at;
        struct polygon_header *sq;
        struct vertex *end;

        if (ninja && trans && (drawn & NJD_FLAG_IGNORE_LIGHT)) {
            mode &= ~COLOR_MODE_OFFSET;
        }
        set_header(material, drawn, mode);
        if (trans) {
            shared = ninja
                         ? set_point_alpha_unlit(lit, model->nbPoint, material,
                                                 drawn, shared, unlit_rgb)
                         : set_point_alpha(lit, model->nbPoint, material, drawn,
                                           shared);
            unlit_made = FALSE;
            trans = !shows_opaque(drawn, shared);
            if (trans) {
                list = &header->trans_polygon;
            }
        }
        at = (Uint32)*list;
        sq = (struct polygon_header *)((at & SQ_ADDRESS) | SQ_BASE);
        sa1_live_sq_base = at & LIST_TOP;
        QACR0 = SQ_AREA(at);
        QACR1 = SQ_AREA(at);
        if (drawn & NJD_FLAG_USE_TEXTURE) {
            sq->texture = header->TexturePARAMBUFFER;
        }
        tsp = header->TSPPARAMBUFFER;
        sq->tsp = trans ? tsp : (tsp & ~TSP_BLEND) | TSP_OPAQUE;
        sq->isp = header->ISPPARAMBUFFER | (cull ? ISP_CULL : 0);
        sq->global = trans ? header->GLOBALPARAMBUFFER
                           : header->GLOBALPARAMBUFFER & ~GLOBAL_TRANSLUCENT;
        prefetch(sq);
        sa1_live_list = list;
        end = (struct vertex *)(sq + 1);
        strips.meshset = meshset;
        strips.polygon_size =
            polygon_sizes[(type & NJD_MESHSET_MASK) >> MESHSET_TYPE_SHIFT];
        strips.colors = lit;
        if (cull) {
            enum uv uv = !(own & NJD_FLAG_USE_TEXTURE) ? NO_UV
                         : own & NJD_FLAG_USE_ENV      ? ENV_UV
                                                       : MESH_UV;

            if (uv != NO_UV && (drawn & NJD_FLAG_USE_TEXTURE) &&
                (drawn & NJD_FLAG_IGNORE_LIGHT)) {
                strips.colors =
                    unlit_pairs(lit, unlit, model->nbPoint, &unlit_made);
            }
            end = strips_culled(end, &strips, clipped, uv);
        } else if (!(own & NJD_FLAG_USE_TEXTURE)) {
            end = strips_untextured(end, &strips, clipped);
        } else {
            if ((drawn & NJD_FLAG_USE_TEXTURE) &&
                (drawn & NJD_FLAG_IGNORE_LIGHT)) {
                strips.colors =
                    unlit_pairs(lit, unlit, model->nbPoint, &unlit_made);
            }
            if (own & NJD_FLAG_USE_ENV) {
                end = clipped ? clipped_env(end, &strips)
                              : strips_env(end, &strips);
            } else {
                end = clipped ? clipped_textured(end, &strips)
                              : strips_textured(end, &strips);
            }
        }
        *list = (Uint32 *)(((Uint32)end & SQ_ADDRESS) | (at & LIST_TOP));
    }
}

/* send_meshes in the Ninja forms, whose NJD_CONTROL_3D_CONSTANT_MATERIAL
 * gives each point the constant material's color */
LINKED_LAST(draw_meshes_like_ninja)
static void draw_meshes_like_ninja(const NJS_MODEL *model, struct point *points,
                                   Bool clipped)
{
    send_meshes(model, points, clipped, constant_rgb(), TRUE);
}

OUT_OF_LINE void draw_meshes(const NJS_MODEL *model, struct point *points,
                             Bool clipped)
{
    send_meshes(model, points, clipped, UNLIT_WHITE, FALSE);
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
        Uint32 drawn = drawn_flags(material);

        set_header(material, drawn, drawn_mode(drawn));
    }
}

/* project model's points into view.scratch, shade them unless every one is
 * near, and return how many are near */
OUT_OF_LINE int project_and_shade(const NJS_MODEL *model)
{
    struct point *points = view.scratch;
    int near_points = project_points(model, points);
    const NJS_MATERIAL *first;

    if (near_points < model->nbPoint) {
        first = &model->mats[model->meshsets->type_matId & MATERIAL_ID];
        shade_points(model, points, first, drawn_flags(first));
    }
    return near_points;
}

/* project_and_shade in the Ninja forms, which shades no point. the forms'
 * constant attributes make every material unlit and translucent. so
 * set_point_alpha_unlit colors each point, and no meshset sends a point's
 * offset color. a textured one sends the unlit offsets with its offset bit
 * clear, and an untextured one none */
LINKED_LAST(project_like_ninja)
OUT_OF_LINE int project_like_ninja(const NJS_MODEL *model)
{
    return project_points(model, view.scratch);
}

/* the projection and the meshsets' draw sa1_live_nested calls, as
 * sa1_live_forms sets them. a call through either pointer costs the code in
 * kernel.txt's model window one load. a test of the forms there would push
 * that code past the window's end */
LINKED_LAST_DATA(drawer) static struct {
    int (*project)(const NJS_MODEL *model);
    void (*meshes)(const NJS_MODEL *model, struct point *points, Bool clipped);
} drawer = {project_and_shade, draw_meshes};

ALWAYS_INLINE void sa1_live_forms(Bool on)
{
    drawer.project = on ? project_like_ninja : project_and_shade;
    drawer.meshes = on ? draw_meshes_like_ninja : draw_meshes;
}

static void sa1_live_run(void)
{
    bake_pass++;
    take_constant_attr();
}

/* SA1_LIVE_MODEL: a model in view, between the walk's own opening and
 * closing of the context. it opens nothing for a model whose every point is
 * near */
ALWAYS_INLINE void sa1_live_model(const NJS_MODEL *model)
{
    int near_points;

    if (model_in_view(model)) {
        take_constant_attr();
        near_points = project_and_shade(model);
        if (near_points < model->nbPoint) {
            sa1_live_open();
            draw_meshes(model, view.scratch, near_points != 0);
            reset_shading();
            sa1_live_close();
        }
    }
}

/* SA1_LIVE_NESTED: a model in view, in a context its caller opened. it sets
 * the shading back to MODULATE_ALPHA for a model in view even when every
 * point is near */
ALWAYS_INLINE void sa1_live_nested(const NJS_MODEL *model)
{
    int near_points;

    if (model_in_view(model)) {
        take_constant_attr();
        near_points = drawer.project(model);
        if (near_points < model->nbPoint) {
            drawer.meshes(model, view.scratch, near_points != 0);
        }
        reset_shading();
    }
}

/* SA1_LIVE_LAND: a model of a run, with no model_in_view test, under the
 * constant attributes its sa1_live_run took. a model that model_off_screen or
 * points_off_screen finds off the screen is neither projected nor sent. where
 * one of its points lies past view.screen.near, its meshsets still set the
 * header words. the run then leaves them as model.src's does */
ALWAYS_INLINE void sa1_live_land(const NJS_MODEL *model)
{
    int near_points;

    if (model_off_screen(model) || points_off_screen(model)) {
        if (any_past_near(model)) {
            set_meshes_header(model);
        }
        return;
    }
    land_draw = TRUE;
    near_points = project_and_shade(model);
    land_draw = FALSE;
    if (near_points < model->nbPoint) {
        draw_meshes(model, view.scratch, near_points != 0);
    }
}
