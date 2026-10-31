/* njDrawModel, njDrawObject and njCalcModel. a draw calculates a model's
 * points into the vertex workspace, _nj_vertex_buf_, then lights, clips and
 * sends each meshset through the store queue */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>
#ifdef GCC
#include <sh4zam/shz_mem.h>
#include <sh4zam/shz_vector.h>
#endif

/* a work vertex's clip bits. CLIP_NEAR and CLIP_FAR mark z below _nj_near_clip_
 * or above _nj_far_clip_. the side bits mark a screen position outside the 2D
 * clip rectangle. a point below the near plane has no side bits */
#define CLIP_NEAR 1
#define CLIP_FAR 2
#define CLIP_LEFT 4
#define CLIP_RIGHT 8
#define CLIP_ABOVE 0x10
#define CLIP_BELOW 0x20
#define CLIP_SIDES (CLIP_LEFT | CLIP_RIGHT | CLIP_ABOVE | CLIP_BELOW)
#define CLIP_ALL (CLIP_NEAR | CLIP_FAR | CLIP_SIDES)
/* the material index, in a meshset's type_matId below its NJD_MESHSET_ type */
#define MATERIAL_ID 0x3fff
/* a strip's point count, in its first word below NJD_TRIMESH_END */
#define STRIP_COUNT 0x7fff
/* the texture number, in the low 30 bits of a material's attr_texId */
#define MATERIAL_TEXTURE 0x3fffffff
/* a key, which names the lighting a work vertex's colors hold, is a material
 * index ored with a light side: 0 for the front face, or BACK_FACE for the
 * back, which njCalculateLightsRGB takes as reverse */
#define BACK_FACE 0x8000
/* the store queue sends 32-byte packets. a byte count shifted right by
 * PACKET_SHIFT counts them */
#define PACKET_BYTES 32
#define PACKET_SHIFT 5

#define FLAT(mode) ((mode) & ~COLOR_MODE_GOURAUD)

/* a packed ARGB color in a work vertex's payload. component never holds a
 * float: PACKED_COLOR takes its address to reach packed */
union color_word {
    Float component;
    Uint32 packed;
};
/* a work vertex, an entry of the vertex workspace, 64 bytes. its first 16 bytes
 * and its payload make its packet. control, the packet's first word, holds the
 * clip bits until a draw writes the command over them. payload holds texture
 * coordinates and packed colors, or one floating color.
 *
 * lit_key holds the key a lit drawer last lit the vertex under. the point loops
 * write it as 0xffffffff, which no key takes. a flat or unlit drawer stores
 * colors in payload and leaves lit_key as it is */
struct model_vertex {
    union {
        Uint32 cmd;
        Uint32 clip;
    } control;
    Float sx, sy;
    Float oow; /* 1/z */
    NJS_POINT3 point; /* in view space */
    Uint32 reserve;
    union {
        struct {
            Float uv[2];
            union color_word colors[2]; /* packed diffuse, then specular */
        } tex;
        NJS_ARGB col;
    } payload;
    NJS_VECTOR vector; /* the normal, in view space */
    Uint32 lit_key;
};

/* the point loops' arguments, which their bodies read by offset. loops 6 to
 * A also read the 2D clip rectangle, and fold each vertex's clip bits into
 * clip_and with & and into clip_or with | */
struct calc_work {
    Sint32 count;
    struct model_vertex *vertex;
    NJS_POINT3 *points;
    NJS_VECTOR *normals;
    Float scale_x, scale_y;
    Uint32 clip_and, clip_or;
    Float left, right, top, bottom;
};

struct vertex_light {
    NJS_ARGB diffuse;
    NJS_ARGB specular;
};
/* a textured corner a clipper makes: its packet, with packed base and offset
 * colors, then the lit colors EMIT_CLIPPED packs into them */
struct clip_vertex {
    Uint32 cmd;
    Float sx, sy;
    Float oow;
    Float u, v;
    Uint32 color, offset;
    struct vertex_light lit;
};
/* an untextured corner a clipper makes: its packet, with a floating color */
struct color_vertex {
    Uint32 cmd;
    Float sx, sy;
    Float oow;
    NJS_ARGB col;
};

/* the clippers' corners, vtx2 textured and vtx4 untextured. _njm_col holds
 * the lit colors of a triangle's three corners and _njm_col2 those of one
 * vertex, each diffuse then specular. _njm_col2 shares _njm_col[0]'s storage */
extern struct clip_vertex vtx2[4];
extern struct color_vertex vtx4[4];
extern NJS_ARGB _njm_col[3][2];
extern NJS_ARGB _njm_col2[2];

/* call _njCalcModel and return its result, with the caller's R8 saved before
 * and restored after. model->nbPoint must not exceed _nj_vertex_num_ */
Sint32 njCalcModel(NJS_MODEL *model);
/* call _njDrawObjectSub, with the caller's R8 saved before and restored after.
 * the caller sets up the lights, as _njDrawObject does */
/* @hack type: volatile on the parameter, which keeps it in the frame
 * across the save of R8, MOV.L R4,@(4,R15) at 0x8c6537b2 in njDrawObjectSub;
 * without it, 12 of its 13 words differ and it runs 4 bytes short, and 10
 * differ in 5 functions beside it */
void njDrawObjectSub(NJS_OBJECT *volatile object);

/* the current material's attrflags, as _njDrawModelSub adjusts them */
static Uint32 material_flags;
/* model->center in view space, which _njCalcModel sets for _njDrawModelSub's
 * njGetLightDistance */
static NJS_POINT3 view_center;
/* the points' clip bits ored together, or 0 under
 * NJD_CONTROL_3D_NO_CLIP_CHECK */
static Uint32 model_clip_or;
/* 1 / _nj_near_clip_, the 1/z a clipper gives a corner on the near plane */
static Float near_reciprocal;
/* the projection's scales, _nj_screen_.dist times _nj_scr_aspect_x_ and _y_ */
static Float projection_x, projection_y;
/* their reciprocals, which nothing reads. _njCalcModel sets all three only when
 * it returns 0 without NJD_CONTROL_3D_NO_CLIP_CHECK and some point has
 * CLIP_NEAR */
static Float projection_x_reciprocal, projection_y_reciprocal;

/* the axes _njDrawModelSub draws under NJD_CONTROL_3D_DISP_AXIS: lines from
 * the first point, the origin, to each of the other three */
static NJS_POINT3 axis_points[4] = {
#include "ninja/njModel/axis_points.inc"
};
static NJS_COLOR axis_colors[4] = {
#include "ninja/njModel/axis_colors.inc"
};
static NJS_POINT3COL axis_lines = {
#include "ninja/njModel/axis_lines.inc"
};

/* the store queue address the next packet goes to. every function of this
 * object keeps it in R8, so the public entries save the caller's R8 and
 * restore it after */
#ifdef GCC
/* GCC's spelling of the #pragma global_register below. as under SHC, no
 * function of this object saves R8 in its prologue */
register Uint32 *queue_cursor __asm__("r8");
#else
#pragma global_register(queue_cursor = R8)
Uint32 *queue_cursor;
#endif

/* clang never sees the inline bodies below, so it finds these functions
 * declared and not defined */
/* NOLINTBEGIN(clang-diagnostic-undefined-internal) */
static void submit_work_vertex(const struct model_vertex *vertex);
static void submit_packet(void *block);
static void restore_queue_cursor(Uint32 *volatile *slot);
static void loop1_body(struct calc_work *work);
static void loop2_body(struct calc_work *work);
static void loop3_body(struct calc_work *work);
static void loop4_body(struct calc_work *work);
static void loop5_body(struct calc_work *work);
static void loop6_body(struct calc_work *work);
static void loop7_body(struct calc_work *work);
static void loop8_body(struct calc_work *work);
static void loop9_body(struct calc_work *work);
static void loopa_body(struct calc_work *work);
static void model_draw_noop(void);
static void writeback_line(void *address);
static void prefetch_vertex_head(struct model_vertex *vertex);
static void prefetch_vertex_tail(void);
static Float view_point_normal_dot(NJS_POINT3 *point, NJS_VECTOR *normal);
static void store_environment_uv(Float *uv, NJS_VECTOR *normal, Float half);
static void store_vertex_color(struct model_vertex *vertex, NJS_ARGB *color);
static Float view_triangle_facing(NJS_POINT3 *b, NJS_POINT3 *a, NJS_POINT3 *c);
static void copy_color_writeback(NJS_ARGB *dst, NJS_ARGB *src);
static Uint32 pack_argb_body(Float a, Float r, Float g, Float b);
static void unpack_argb_body(Uint32 color, NJS_ARGB *dst);
/* NOLINTEND(clang-diagnostic-undefined-internal) */
void CalcModelMatrix(NJS_POINT3 *dst, const NJS_POINT3 *src);

/* send vertex's packet, its first 16 bytes and its payload, at queue_cursor,
 * and advance queue_cursor 32 bytes. FPSCR.SZ must be 0: the body's FSCHGs set
 * it for the moves, which then move eight bytes each, and clear it after */
#ifdef GCC
/* an asm runs the shipped body's pair moves and both its FSCHGs, so none of
 * GCC's own moves runs while SZ is 1. the pair moves need vertex on an 8-byte
 * boundary. a work vertex lies on a 32-byte one, as the point loops' store
 * queue writes need. the m inputs keep GCC's stores to the vertex ahead of
 * the asm. the asm writes only the store queue area, which no C reads, so it
 * declares no memory output. SH4ZAM's spelling is two shz_memcpy8 calls and
 * a PREF behind a memory clobber. in a function of its own, that spelling
 * runs 38 instructions where the asm runs 17. it also makes njModel.o's text
 * about 8 KB larger */
ALWAYS_INLINE void submit_work_vertex(const struct model_vertex *vertex)
{
    const struct model_vertex *head = vertex;
    const void *payload = &vertex->payload;
    Uint32 *packet = queue_cursor;

    __asm__ volatile(
        "add #32,%[packet]\n\t"
        "fschg\n\t"
        "fmov @%[head]+,dr0\n\t"
        "fmov @%[head]+,dr2\n\t"
        "fmov @%[payload]+,dr4\n\t"
        "fmov @%[payload]+,dr6\n\t"
        "fmov dr6,@-%[packet]\n\t"
        "fmov dr4,@-%[packet]\n\t"
        "fmov dr2,@-%[packet]\n\t"
        "fmov dr0,@-%[packet]\n\t"
        "fschg\n\t"
        "pref @%[packet]"
        : [head] "+r"(head), [payload] "+r"(payload), [packet] "+r"(packet)
        : "m"(*(const Uint8(*)[PACKET_BYTES / 2]) vertex), "m"(vertex->payload)
        : "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7");
    queue_cursor += PACKET_BYTES / sizeof *queue_cursor;
}
#else
/* clang-format off */
#ifndef __clang__
/* @hack spelling: size=64, against the body's 28 bytes, SHC's estimate of each
 * expansion, by which it places the pools of the functions around it; with
 * size=28, 22845 of 33373 words differ in 47 functions */
/* @asm open: no inspect rule reads the pairs its FMOV.S loads and stores move
 * after FSCHG */
#pragma inline_asm(submit_work_vertex(size=64))
static void submit_work_vertex(const struct model_vertex *vertex)
{
        ADD         #32,R8
        FSCHG
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR2
        ADD         #16,R4
        FMOV.S      @R4+,FR4
        FMOV.S      @R4+,FR6
        FMOV.S      FR6,@-R8
        FMOV.S      FR4,@-R8
        FMOV.S      FR2,@-R8
        FMOV.S      FR0,@-R8
        FSCHG
        PREF        @R8
        ADD         #32,R8
}
#endif
/* clang-format on */
#endif

/* send the 32 bytes at block as one packet at queue_cursor, and advance
 * queue_cursor 32 bytes. FPSCR.SZ must be 0, as for submit_work_vertex */
#ifdef GCC
/* the asm is shz_sq_memcpy32_1's, with its two FSCHGs moved inside, as in
 * submit_work_vertex. SH4ZAM runs each FSCHG in an asm of its own, and a move
 * GCC puts between them would move a pair. the pair moves need block on an
 * 8-byte boundary. each block is an element of vtx2 or vtx4, on a 32-byte one
 * in njModelGlobal.src's BSG32. as in submit_work_vertex, the m input keeps
 * GCC's stores to block ahead of the asm, which declares no memory output */
ALWAYS_INLINE void submit_packet(void *block)
{
    const void *source = block;
    Uint32 *packet = queue_cursor;

    __asm__ volatile("fschg\n\t"
                     "fmov @%[source]+,dr0\n\t"
                     "fmov @%[source]+,dr2\n\t"
                     "fmov @%[source]+,dr4\n\t"
                     "fmov @%[source]+,dr6\n\t"
                     "add #32,%[packet]\n\t"
                     "fmov dr6,@-%[packet]\n\t"
                     "fmov dr4,@-%[packet]\n\t"
                     "fmov dr2,@-%[packet]\n\t"
                     "fmov dr0,@-%[packet]\n\t"
                     "pref @%[packet]\n\t"
                     "fschg"
                     : [source] "+r"(source), [packet] "+r"(packet)
                     : "m"(*(const Uint8(*)[PACKET_BYTES])block)
                     : "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7");
    queue_cursor += PACKET_BYTES / sizeof *queue_cursor;
}
#else
/* clang-format off */
#ifndef __clang__
/* @hack spelling: size=64, against the body's 26 bytes, SHC's estimate of each
 * expansion, by which it places the pools of the functions around it; with
 * size=26, 3166 of 4556 words differ in 11 functions, and unsized, align-pool
 * refuses SHC's text */
/* @asm open: no inspect rule reads the pairs its FMOV.S loads and stores move
 * after FSCHG */
#pragma inline_asm(submit_packet(size=64))
static void submit_packet(void *block)
{
        ADD         #32,R8
        FSCHG
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR2
        FMOV.S      @R4+,FR4
        FMOV.S      @R4+,FR6
        FMOV.S      FR6,@-R8
        FMOV.S      FR4,@-R8
        FMOV.S      FR2,@-R8
        FMOV.S      FR0,@-R8
        FSCHG
        PREF        @R8
        ADD         #32,R8
}
#endif
/* clang-format on */
#endif

/* store queue_cursor, R8, in *slot */
#pragma inline(save_queue_cursor)
static void save_queue_cursor(Uint32 *volatile *slot)
{
    /* @hack type: the store to a volatile slot, which keeps it, MOV.L
     * R8,@R4 at 0x8c653734 in njDrawModel, ahead of the next call's pool load;
     * without it, 12 of 49 words differ in njDrawModel, njCalcModel and
     * njDrawModelSub */
    *slot = queue_cursor;
}

/* clang-format off */
#ifndef __clang__
/* load queue_cursor, R8, from *slot */
/* assembly keeps the load, MOV.L @R4,R8 at 0x8c65373e in njDrawModel, ahead of
 * the pool load for the call after it */
/* @asm open: no inspect rule reads the R8 it leaves its callers */
#pragma inline_asm(restore_queue_cursor(size=2))
static void restore_queue_cursor(Uint32 *volatile *slot)
{
        MOV.L       @R4,R8
}
#endif
/* clang-format on */

/* the point loops write work->count work vertices from work->vertex on, one
 * for each point at work->points, with its normal at work->normals, which
 * loops 5 and A don't read:
 *
 *   loop   point                            normal
 *   1, 6   _nj_vertex_offset_ along normal  transformed and made unit length
 *   2, 7   _nj_vertex_offset_ along normal  transformed
 *   3, 8   as given                         transformed and made unit length
 *   4, 9   as given                         transformed
 *   5, A   as given                         none
 *
 * a point goes through XMTRX into view space, and its screen position is
 * x * work->scale_x / z + _nj_screen_.cx, and y alike. a normal goes through
 * XMTRX with w 0. each vertex gets its clip bits, 0 from loops 1 to 5, its
 * screen position, 1/z, view point and lit_key, and from the loops that read
 * normals, its view normal. the loops write the workspace through the store
 * queues, which they point at it by QACR0 and QACR1, and invalidate each
 * vertex's cache lines */
/* clang-format off */
#ifndef __clang__
/* @asm opcode: cache-line, OCBI @R5 at 0x8c63c00a and 0x8c63c044, in
 * njCalcModel_Loop1, invalidates a vertex's two lines */
#pragma inline_asm(loop1_body)
static void loop1_body(struct calc_work *work)
{
        .IMPORT     __nj_vertex_offset_
        FMOV.S      FR15,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR12,@-R15
        MOV.L       R14,@-R15
        MOV.L       R13,@-R15
        MOV.L       R12,@-R15
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       #__nj_screen_+12,R12
        MOV.L       #__nj_screen_+16,R13
        MOV.L       #__nj_vertex_offset_,R14
        FMOV.S      @R12,FR2
        FMOV.S      @R13,FR3
        FMOV.S      @R14,FR4
        FMOV.S      FR2,@-R15
        MOV         R15,R12
        FMOV.S      FR3,@-R15
        MOV         R15,R13
        FMOV.S      FR4,@-R15
        MOV         R15,R14
        MOV.L       #H'FFFFFFFF,R6
        MOV         R4,R1
        MOV.L       @R1+,R7
        MOV.L       @R1+,R5
        MOV.L       @R1+,R3
        MOV.L       @R1+,R2
        FMOV.S      @R1+,FR8
        FMOV.S      @R1+,FR9
        MOV         R5,R4
        MOV.L       #H'1C000000,R0
        MOV.L       #H'FF000038,R1
        AND         R5,R0
        SHLR16      R0
        SHLR8       R0
        MOV.L       R0,@(0,R1)
        MOV.L       R0,@(4,R1)
        MOV.L       #H'03FFFFFF,R0
        MOV.L       #H'E0000000,R1
        AND         R0,R4
        OR          R1,R4
        MOV         #0,R1
        TST         R7,R7
        BT/S        ?done
        ADD         #32,R5
?vertex:
        FMOV.S      @R2+,FR4
        FLDI0       FR7
        FMOV.S      @R2+,FR5
        FMOV.S      FR4,FR0
        FMOV.S      @R2+,FR6
        FMOV.S      FR5,FR1
        FMOV.S      FR6,FR2
        FTRV        XMTRX,FV4
        FMOV.S      @R14,FR3
        ADD         #32,R2
        PREF        @R2
        ADD         #-32,R2
        ADD         #64,R4
        FLDI0       FR7
        FIPR        FV4,FV4
        FSRRA       FR7
        OCBI        @R5
        FMUL        FR3,FR0
        FMUL        FR3,FR1
        FMUL        FR3,FR2
        FLDI1       FR3
        FMUL        FR7,FR6
        FMUL        FR7,FR5
        FMUL        FR7,FR4
        MOV.L       R6,@-R4
        FMOV.S      FR6,@-R4
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        FMOV.S      @R3+,FR4
        FMOV.S      @R3+,FR5
        FMOV.S      @R3+,FR6
        PREF        @R4
        FADD        FR4,FR0
        FADD        FR5,FR1
        FADD        FR6,FR2
        FTRV        XMTRX,FV0
        FLDI1       FR3
        ADD         #-32,R5
        FDIV        FR2,FR3
        FMOV.S      @R12,FR6
        FMOV.S      @R13,FR7
        ADD         #-20,R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        OCBI        @R5
        FMUL        FR9,FR1
        FMUL        FR8,FR0
        ADD         #96,R5
        ADD         #32,R3
        PREF        @R3
        ADD         #-32,R3
        FMUL        FR3,FR1
        FMUL        FR3,FR0
        FADD        FR7,FR1
        FADD        FR6,FR0
        FMOV.S      FR3,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        MOV.L       R1,@-R4
        PREF        @R4
        DT          R7
        BF/S        ?vertex
        ADD         #64,R4
?done:
        ADD         #12,R15
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
        MOV.L       @R15+,R12
        MOV.L       @R15+,R13
        MOV.L       @R15+,R14
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        RTS
        FMOV.S      @R15+,FR15
}
#endif
/* clang-format on */

void njCalcModel_Loop1(struct calc_work *work)
{
    loop1_body(work);
}

/* clang-format off */
#ifndef __clang__
/* @asm opcode: cache-line, OCBI @R5 at 0x8c63c120 and 0x8c63c154, in
 * njCalcModel_Loop2, invalidates a vertex's two lines */
#pragma inline_asm(loop2_body)
static void loop2_body(struct calc_work *work)
{
        FMOV.S      FR15,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR12,@-R15
        MOV.L       R14,@-R15
        MOV.L       R13,@-R15
        MOV.L       R12,@-R15
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       #__nj_screen_+12,R12
        MOV.L       #__nj_screen_+16,R13
        MOV.L       #__nj_vertex_offset_,R14
        FMOV.S      @R12,FR2
        FMOV.S      @R13,FR3
        FMOV.S      @R14,FR4
        FMOV.S      FR2,@-R15
        MOV         R15,R12
        FMOV.S      FR3,@-R15
        MOV         R15,R13
        FMOV.S      FR4,@-R15
        MOV         R15,R14
        MOV.L       #H'FFFFFFFF,R6
        MOV         R4,R1
        MOV.L       @R1+,R7
        MOV.L       @R1+,R5
        MOV.L       @R1+,R3
        MOV.L       @R1+,R2
        FMOV.S      @R1+,FR8
        FMOV.S      @R1+,FR9
        MOV         R5,R4
        MOV.L       #H'1C000000,R0
        MOV.L       #H'FF000038,R1
        AND         R5,R0
        SHLR16      R0
        SHLR8       R0
        MOV.L       R0,@(0,R1)
        MOV.L       R0,@(4,R1)
        MOV.L       #H'03FFFFFF,R0
        MOV.L       #H'E0000000,R1
        AND         R0,R4
        OR          R1,R4
        MOV         #0,R1
        TST         R7,R7
        BT/S        ?done
        ADD         #32,R5
?vertex:
        FMOV.S      @R2+,FR4
        FLDI0       FR7
        FMOV.S      @R2+,FR5
        FMOV.S      FR4,FR0
        FMOV.S      @R2+,FR6
        FMOV.S      FR5,FR1
        FMOV.S      FR6,FR2
        FTRV        XMTRX,FV4
        FMOV.S      @R14,FR3
        ADD         #32,R2
        PREF        @R2
        ADD         #-32,R2
        ADD         #64,R4
        OCBI        @R5
        FMUL        FR3,FR0
        FMUL        FR3,FR1
        FMUL        FR3,FR2
        FLDI1       FR3
        MOV.L       R6,@-R4
        FMOV.S      FR6,@-R4
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        FMOV.S      @R3+,FR4
        FMOV.S      @R3+,FR5
        FADD        FR4,FR0
        FMOV.S      @R3+,FR6
        PREF        @R4
        FADD        FR5,FR1
        FADD        FR6,FR2
        FTRV        XMTRX,FV0
        FLDI1       FR3
        ADD         #-32,R5
        FDIV        FR2,FR3
        FMOV.S      @R12,FR6
        FMOV.S      @R13,FR7
        ADD         #-20,R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        OCBI        @R5
        FMUL        FR9,FR1
        FMUL        FR8,FR0
        ADD         #96,R5
        ADD         #32,R3
        PREF        @R3
        ADD         #-32,R3
        FMUL        FR3,FR1
        FMUL        FR3,FR0
        FADD        FR7,FR1
        FADD        FR6,FR0
        FMOV.S      FR3,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        MOV.L       R1,@-R4
        PREF        @R4
        DT          R7
        BF/S        ?vertex
        ADD         #64,R4
?done:
        ADD         #12,R15
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
        MOV.L       @R15+,R12
        MOV.L       @R15+,R13
        MOV.L       @R15+,R14
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        RTS
        FMOV.S      @R15+,FR15
}
#endif
/* clang-format on */

void njCalcModel_Loop2(struct calc_work *work)
{
    loop2_body(work);
}

/* clang-format off */
#ifndef __clang__
/* @asm opcode: cache-line, OCBI @R5 at 0x8c63c222 and 0x8c63c256, in
 * njCalcModel_Loop3, invalidates a vertex's two lines */
#pragma inline_asm(loop3_body)
static void loop3_body(struct calc_work *work)
{
        FMOV.S      FR15,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR12,@-R15
        MOV.L       R14,@-R15
        MOV.L       R13,@-R15
        MOV.L       R12,@-R15
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       #__nj_screen_+12,R12
        MOV.L       #__nj_screen_+16,R13
        FMOV.S      @R12,FR2
        FMOV.S      @R13,FR3
        FMOV.S      FR2,@-R15
        MOV         R15,R12
        FMOV.S      FR3,@-R15
        MOV         R15,R13
        MOV.L       #H'FFFFFFFF,R6
        MOV         #0,R1
        MOV         R4,R1
        MOV.L       @R1+,R7
        MOV.L       @R1+,R5
        MOV.L       @R1+,R3
        MOV.L       @R1+,R2
        FMOV.S      @R1+,FR8
        FMOV.S      @R1+,FR9
        MOV         R5,R4
        MOV.L       #H'1C000000,R0
        MOV.L       #H'FF000038,R1
        AND         R5,R0
        SHLR16      R0
        SHLR8       R0
        MOV.L       R0,@(0,R1)
        MOV.L       R0,@(4,R1)
        MOV.L       #H'03FFFFFF,R0
        MOV.L       #H'E0000000,R1
        AND         R0,R4
        OR          R1,R4
        MOV         #0,R1
        ADD         #32,R5
        TST         R7,R7
        BT          ?done
?vertex:
        FMOV.S      @R2+,FR4
        FMOV.S      @R2+,FR5
        FMOV.S      @R2+,FR6
        FLDI0       FR7
        FTRV        XMTRX,FV4
        FMOV.S      @R3+,FR0
        FMOV.S      @R3+,FR1
        FMOV.S      @R3+,FR2
        FLDI1       FR3
        OCBI        @R5
        FLDI0       FR7
        FIPR        FV4,FV4
        ADD         #64,R4
        FSRRA       FR7
        ADD         #32,R2
        PREF        @R2
        ADD         #-32,R2
        MOV.L       R6,@-R4
        FMUL        FR7,FR6
        FMUL        FR7,FR5
        FMUL        FR7,FR4
        FTRV        XMTRX,FV0
        FMOV.S      FR6,@-R4
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        PREF        @R4
        FLDI1       FR3
        FDIV        FR2,FR3
        ADD         #-32,R5
        FMOV.S      @R12,FR6
        FMOV.S      @R13,FR7
        ADD         #-20,R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        OCBI        @R5
        FMUL        FR9,FR1
        FMUL        FR8,FR0
        ADD         #96,R5
        ADD         #32,R3
        PREF        @R3
        ADD         #-32,R3
        FMUL        FR3,FR1
        FMUL        FR3,FR0
        FADD        FR7,FR1
        FADD        FR6,FR0
        FMOV.S      FR3,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        MOV.L       R1,@-R4
        PREF        @R4
        DT          R7
        BF/S        ?vertex
        ADD         #64,R4
?done:
        ADD         #8,R15
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
        MOV.L       @R15+,R12
        MOV.L       @R15+,R13
        MOV.L       @R15+,R14
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        RTS
        FMOV.S      @R15+,FR15
}
#endif
/* clang-format on */

void njCalcModel_Loop3(struct calc_work *work)
{
    loop3_body(work);
}

/* clang-format off */
#ifndef __clang__
/* @asm opcode: cache-line, OCBI @R5 at 0x8c63c320 and 0x8c63c348, in
 * njCalcModel_Loop4, invalidates a vertex's two lines */
#pragma inline_asm(loop4_body)
static void loop4_body(struct calc_work *work)
{
        FMOV.S      FR15,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR12,@-R15
        MOV.L       R14,@-R15
        MOV.L       R13,@-R15
        MOV.L       R12,@-R15
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       #__nj_screen_+12,R12
        MOV.L       #__nj_screen_+16,R13
        FMOV.S      @R12,FR2
        FMOV.S      @R13,FR3
        FMOV.S      FR2,@-R15
        MOV         R15,R12
        FMOV.S      FR3,@-R15
        MOV         R15,R13
        MOV.L       #H'FFFFFFFF,R6
        MOV         R4,R1
        MOV.L       @R1+,R7
        MOV.L       @R1+,R5
        MOV.L       @R1+,R3
        MOV.L       @R1+,R2
        FMOV.S      @R1+,FR8
        FMOV.S      @R1+,FR9
        MOV         R5,R4
        MOV.L       #H'1C000000,R0
        MOV.L       #H'FF000038,R1
        AND         R5,R0
        SHLR16      R0
        SHLR8       R0
        MOV.L       R0,@(0,R1)
        MOV.L       R0,@(4,R1)
        MOV.L       #H'03FFFFFF,R0
        MOV.L       #H'E0000000,R1
        AND         R0,R4
        OR          R1,R4
        MOV         #0,R1
        TST         R7,R7
        BT/S        ?done
        ADD         #32,R5
?vertex:
        FMOV.S      @R2+,FR4
        FMOV.S      @R2+,FR5
        FMOV.S      @R2+,FR6
        FLDI0       FR7
        FTRV        XMTRX,FV4
        ADD         #32,R2
        PREF        @R2
        ADD         #-32,R2
        ADD         #64,R4
        OCBI        @R5
        FLDI1       FR3
        FMOV.S      @R3+,FR0
        FMOV.S      @R3+,FR1
        FMOV.S      @R3+,FR2
        MOV.L       R6,@-R4
        FMOV.S      FR6,@-R4
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        PREF        @R4
        FTRV        XMTRX,FV0
        FLDI1       FR3
        ADD         #-32,R5
        FDIV        FR2,FR3
        FMOV.S      @R12,FR6
        FMOV.S      @R13,FR7
        ADD         #-20,R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        OCBI        @R5
        FMUL        FR9,FR1
        FMUL        FR8,FR0
        ADD         #96,R5
        ADD         #32,R3
        PREF        @R3
        ADD         #-32,R3
        FMUL        FR3,FR1
        FMUL        FR3,FR0
        FADD        FR7,FR1
        FADD        FR6,FR0
        FMOV.S      FR3,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        MOV.L       R1,@-R4
        PREF        @R4
        DT          R7
        BF/S        ?vertex
        ADD         #64,R4
?done:
        ADD         #8,R15
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
        MOV.L       @R15+,R12
        MOV.L       @R15+,R13
        MOV.L       @R15+,R14
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        RTS
        FMOV.S      @R15+,FR15
}
#endif
/* clang-format on */

void njCalcModel_Loop4(struct calc_work *work)
{
    loop4_body(work);
}

/* clang-format off */
#ifndef __clang__
/* @asm opcode: cache-line, OCBI @R5 at 0x8c63c3f8 and 0x8c63c416, in
 * njCalcModel_Loop5, invalidates a vertex's two lines */
#pragma inline_asm(loop5_body)
static void loop5_body(struct calc_work *work)
{
        FMOV.S      FR15,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR12,@-R15
        MOV.L       R14,@-R15
        MOV.L       R13,@-R15
        MOV.L       R12,@-R15
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       #__nj_screen_+12,R12
        MOV.L       #__nj_screen_+16,R13
        FMOV.S      @R12,FR6
        FMOV.S      @R13,FR7
        MOV.L       #H'FFFFFFFF,R6
        MOV         R4,R1
        MOV.L       @R1+,R7
        MOV.L       @R1+,R5
        MOV.L       @R1+,R3
        ADD         #4,R1
        FMOV.S      @R1+,FR8
        FMOV.S      @R1+,FR9
        MOV         R5,R4
        MOV.L       #H'1C000000,R0
        MOV.L       #H'FF000038,R1
        AND         R5,R0
        SHLR16      R0
        SHLR8       R0
        MOV.L       R0,@(0,R1)
        MOV.L       R0,@(4,R1)
        MOV.L       #H'03FFFFFF,R0
        MOV.L       #H'E0000000,R1
        AND         R0,R4
        OR          R1,R4
        MOV         #0,R1
        TST         R7,R7
        BT/S        ?done
        ADD         #32,R5
?vertex:
        ADD         #64,R4
        OCBI        @R5
        FLDI1       FR3
        MOV.L       R6,@-R4
        FMOV.S      @R3+,FR0
        FMOV.S      @R3+,FR1
        FMOV.S      @R3+,FR2
        PREF        @R4
        FTRV        XMTRX,FV0
        FLDI1       FR3
        ADD         #-32,R5
        FDIV        FR2,FR3
        ADD         #-32,R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        OCBI        @R5
        FMUL        FR9,FR1
        FMUL        FR8,FR0
        ADD         #96,R5
        ADD         #32,R3
        PREF        @R3
        ADD         #-32,R3
        FMUL        FR3,FR1
        FMUL        FR3,FR0
        FADD        FR7,FR1
        FADD        FR6,FR0
        FMOV.S      FR3,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        MOV.L       R1,@-R4
        PREF        @R4
        DT          R7
        BF/S        ?vertex
        ADD         #64,R4
?done:
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
        MOV.L       @R15+,R12
        MOV.L       @R15+,R13
        MOV.L       @R15+,R14
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        RTS
        FMOV.S      @R15+,FR15
}
#endif
/* clang-format on */

void njCalcModel_Loop5(struct calc_work *work)
{
    loop5_body(work);
}

/* clang-format off */
#ifndef __clang__
/* @asm opcode: cache-line, OCBI @R5 at 0x8c63c50e and 0x8c63c548, in
 * njCalcModel_Loop6, invalidates a vertex's two lines */
#pragma inline_asm(loop6_body)
static void loop6_body(struct calc_work *work)
{
        FMOV.S      FR15,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR12,@-R15
        MOV.L       R14,@-R15
        MOV.L       R13,@-R15
        MOV.L       R12,@-R15
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       R4,@-R15
        MOV.L       #__nj_far_clip_,R10
        MOV.L       #__nj_near_clip_,R11
        MOV.L       #__nj_screen_+12,R12
        MOV.L       #__nj_screen_+16,R13
        MOV.L       #__nj_vertex_offset_,R14
        FMOV.S      @R10,FR10
        FMOV.S      @R11,FR11
        FMOV.S      @R12,FR2
        FMOV.S      @R13,FR3
        FMOV.S      @R14,FR4
        FMOV.S      FR10,@-R15
        MOV         R15,R10
        FMOV.S      FR11,@-R15
        MOV         R15,R11
        FMOV.S      FR2,@-R15
        MOV         R15,R12
        FMOV.S      FR3,@-R15
        MOV         R15,R13
        FMOV.S      FR4,@-R15
        MOV         R15,R14
        MOV.L       #H'FFFFFFFF,R6
        MOV         R4,R1
        MOV.L       @R1+,R7
        MOV.L       @R1+,R5
        MOV.L       @R1+,R3
        MOV.L       @R1+,R2
        FMOV.S      @R1+,FR8
        FMOV.S      @R1+,FR9
        MOV.L       @R1+,R8
        MOV.L       @R1+,R9
        FMOV.S      @R1+,FR15
        FMOV.S      @R1+,FR14
        FMOV.S      @R1+,FR13
        FMOV.S      @R1+,FR12
        MOV         R5,R4
        MOV.L       #H'1C000000,R0
        MOV.L       #H'FF000038,R1
        AND         R5,R0
        SHLR16      R0
        SHLR8       R0
        MOV.L       R0,@(0,R1)
        MOV.L       R0,@(4,R1)
        MOV.L       #H'03FFFFFF,R0
        MOV.L       #H'E0000000,R1
        AND         R0,R4
        OR          R1,R4
        TST         R7,R7
        BT/S        ?done
        ADD         #32,R5
?vertex:
        FMOV.S      @R2+,FR4
        FLDI0       FR7
        FMOV.S      @R2+,FR5
        FMOV.S      FR4,FR0
        FMOV.S      @R2+,FR6
        FMOV.S      FR5,FR1
        FMOV.S      FR6,FR2
        FTRV        XMTRX,FV4
        FMOV.S      @R14,FR3
        ADD         #32,R2
        PREF        @R2
        ADD         #-32,R2
        ADD         #64,R4
        FLDI0       FR7
        FIPR        FV4,FV4
        FSRRA       FR7
        OCBI        @R5
        FMUL        FR3,FR0
        FMUL        FR3,FR1
        FMUL        FR3,FR2
        FLDI1       FR3
        FMUL        FR7,FR6
        FMUL        FR7,FR5
        FMUL        FR7,FR4
        MOV.L       R6,@-R4
        FMOV.S      FR6,@-R4
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        FMOV.S      @R3+,FR4
        FMOV.S      @R3+,FR5
        FMOV.S      @R3+,FR6
        PREF        @R4
        FADD        FR4,FR0
        FADD        FR5,FR1
        FADD        FR6,FR2
        FTRV        XMTRX,FV0
        FLDI1       FR3
        ADD         #-32,R5
        FDIV        FR2,FR3
        FMOV.S      @R12,FR6
        FMOV.S      @R13,FR7
        ADD         #-20,R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        OCBI        @R5
        FMUL        FR9,FR1
        FMUL        FR8,FR0
        ADD         #96,R5
        MOV         #0,R1
        ADD         #32,R3
        PREF        @R3
        ADD         #-32,R3
        FMUL        FR3,FR1
        FMUL        FR3,FR0
        FCMP/GT     FR2,FR11
        FADD        FR7,FR1
        BT/S        ?depth
        FADD        FR6,FR0
        FCMP/GT     FR12,FR1
        ROTCL       R1
        FCMP/GT     FR1,FR13
        ROTCL       R1
        FCMP/GT     FR14,FR0
        ROTCL       R1
        FCMP/GT     FR0,FR15
        ROTCL       R1
?depth:
        FCMP/GT     FR10,FR2
        FMOV.S      FR3,@-R4
        ROTCL       R1
        FCMP/GT     FR2,FR11
        FMOV.S      FR1,@-R4
        ROTCL       R1
        FMOV.S      FR0,@-R4
        AND         R1,R8
        MOV.L       R1,@-R4
        OR          R1,R9
        PREF        @R4
        DT          R7
        BF/S        ?vertex
        ADD         #64,R4
?done:
        ADD         #20,R15
        MOV.L       @R15+,R4
        MOV.L       R8,@(24,R4)
        MOV.L       R9,@(28,R4)
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
        MOV.L       @R15+,R12
        MOV.L       @R15+,R13
        MOV.L       @R15+,R14
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        RTS
        FMOV.S      @R15+,FR15
}
#endif
/* clang-format on */

void njCalcModel_Loop6(struct calc_work *work)
{
    loop6_body(work);
}

/* clang-format off */
#ifndef __clang__
/* @asm opcode: cache-line, OCBI @R5 at 0x8c63c670 and 0x8c63c6a4, in
 * njCalcModel_Loop7, invalidates a vertex's two lines */
#pragma inline_asm(loop7_body)
static void loop7_body(struct calc_work *work)
{
        FMOV.S      FR15,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR12,@-R15
        MOV.L       R14,@-R15
        MOV.L       R13,@-R15
        MOV.L       R12,@-R15
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       R4,@-R15
        MOV.L       #__nj_far_clip_,R10
        MOV.L       #__nj_near_clip_,R11
        MOV.L       #__nj_screen_+12,R12
        MOV.L       #__nj_screen_+16,R13
        MOV.L       #__nj_vertex_offset_,R14
        FMOV.S      @R10,FR10
        FMOV.S      @R11,FR11
        FMOV.S      @R12,FR2
        FMOV.S      @R13,FR3
        FMOV.S      @R14,FR4
        FMOV.S      FR10,@-R15
        MOV         R15,R10
        FMOV.S      FR11,@-R15
        MOV         R15,R11
        FMOV.S      FR2,@-R15
        MOV         R15,R12
        FMOV.S      FR3,@-R15
        MOV         R15,R13
        FMOV.S      FR4,@-R15
        MOV         R15,R14
        MOV.L       #H'FFFFFFFF,R6
        MOV         R4,R1
        MOV.L       @R1+,R7
        MOV.L       @R1+,R5
        MOV.L       @R1+,R3
        MOV.L       @R1+,R2
        FMOV.S      @R1+,FR8
        FMOV.S      @R1+,FR9
        MOV.L       @R1+,R8
        MOV.L       @R1+,R9
        FMOV.S      @R1+,FR15
        FMOV.S      @R1+,FR14
        FMOV.S      @R1+,FR13
        FMOV.S      @R1+,FR12
        MOV         R5,R4
        MOV.L       #H'1C000000,R0
        MOV.L       #H'FF000038,R1
        AND         R5,R0
        SHLR16      R0
        SHLR8       R0
        MOV.L       R0,@(0,R1)
        MOV.L       R0,@(4,R1)
        MOV.L       #H'03FFFFFF,R0
        MOV.L       #H'E0000000,R1
        AND         R0,R4
        OR          R1,R4
        TST         R7,R7
        BT/S        ?done
        ADD         #32,R5
?vertex:
        FMOV.S      @R2+,FR4
        FLDI0       FR7
        FMOV.S      @R2+,FR5
        FMOV.S      FR4,FR0
        FMOV.S      @R2+,FR6
        FMOV.S      FR5,FR1
        FMOV.S      FR6,FR2
        FTRV        XMTRX,FV4
        FMOV.S      @R14,FR3
        ADD         #32,R2
        PREF        @R2
        ADD         #-32,R2
        ADD         #64,R4
        OCBI        @R5
        FMUL        FR3,FR0
        FMUL        FR3,FR1
        FMUL        FR3,FR2
        FLDI1       FR3
        MOV.L       R6,@-R4
        FMOV.S      FR6,@-R4
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        FMOV.S      @R3+,FR4
        FMOV.S      @R3+,FR5
        FADD        FR4,FR0
        FMOV.S      @R3+,FR6
        PREF        @R4
        FADD        FR5,FR1
        FADD        FR6,FR2
        FTRV        XMTRX,FV0
        FLDI1       FR3
        ADD         #-32,R5
        FDIV        FR2,FR3
        FMOV.S      @R12,FR6
        FMOV.S      @R13,FR7
        ADD         #-20,R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        OCBI        @R5
        FMUL        FR9,FR1
        FMUL        FR8,FR0
        ADD         #96,R5
        MOV         #0,R1
        ADD         #32,R3
        PREF        @R3
        ADD         #-32,R3
        FMUL        FR3,FR1
        FMUL        FR3,FR0
        FCMP/GT     FR2,FR11
        FADD        FR7,FR1
        BT/S        ?depth
        FADD        FR6,FR0
        FCMP/GT     FR12,FR1
        ROTCL       R1
        FCMP/GT     FR1,FR13
        ROTCL       R1
        FCMP/GT     FR14,FR0
        ROTCL       R1
        FCMP/GT     FR0,FR15
        ROTCL       R1
?depth:
        FCMP/GT     FR10,FR2
        FMOV.S      FR3,@-R4
        ROTCL       R1
        FCMP/GT     FR2,FR11
        FMOV.S      FR1,@-R4
        ROTCL       R1
        FMOV.S      FR0,@-R4
        AND         R1,R8
        MOV.L       R1,@-R4
        OR          R1,R9
        PREF        @R4
        DT          R7
        BF/S        ?vertex
        ADD         #64,R4
?done:
        ADD         #20,R15
        MOV.L       @R15+,R4
        MOV.L       R8,@(24,R4)
        MOV.L       R9,@(28,R4)
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
        MOV.L       @R15+,R12
        MOV.L       @R15+,R13
        MOV.L       @R15+,R14
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        RTS
        FMOV.S      @R15+,FR15
}
#endif
/* clang-format on */

void njCalcModel_Loop7(struct calc_work *work)
{
    loop7_body(work);
}

/* clang-format off */
#ifndef __clang__
/* @asm opcode: cache-line, OCBI @R5 at 0x8c63c7bc and 0x8c63c7f0, in
 * njCalcModel_Loop8, invalidates a vertex's two lines */
#pragma inline_asm(loop8_body)
static void loop8_body(struct calc_work *work)
{
        FMOV.S      FR15,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR12,@-R15
        MOV.L       R14,@-R15
        MOV.L       R13,@-R15
        MOV.L       R12,@-R15
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       R4,@-R15
        MOV.L       #__nj_far_clip_,R10
        MOV.L       #__nj_near_clip_,R11
        MOV.L       #__nj_screen_+12,R12
        MOV.L       #__nj_screen_+16,R13
        FMOV.S      @R10,FR10
        FMOV.S      @R11,FR11
        FMOV.S      @R12,FR2
        FMOV.S      @R13,FR3
        FMOV.S      FR10,@-R15
        MOV         R15,R10
        FMOV.S      FR11,@-R15
        MOV         R15,R11
        FMOV.S      FR2,@-R15
        MOV         R15,R12
        FMOV.S      FR3,@-R15
        MOV         R15,R13
        MOV.L       #H'FFFFFFFF,R6
        MOV         R4,R1
        MOV.L       @R1+,R7
        MOV.L       @R1+,R5
        MOV.L       @R1+,R3
        MOV.L       @R1+,R2
        FMOV.S      @R1+,FR8
        FMOV.S      @R1+,FR9
        MOV.L       @R1+,R8
        MOV.L       @R1+,R9
        FMOV.S      @R1+,FR15
        FMOV.S      @R1+,FR14
        FMOV.S      @R1+,FR13
        FMOV.S      @R1+,FR12
        MOV         R5,R4
        MOV.L       #H'1C000000,R0
        MOV.L       #H'FF000038,R1
        AND         R5,R0
        SHLR16      R0
        SHLR8       R0
        MOV.L       R0,@(0,R1)
        MOV.L       R0,@(4,R1)
        MOV.L       #H'03FFFFFF,R0
        MOV.L       #H'E0000000,R1
        AND         R0,R4
        OR          R1,R4
        ADD         #32,R5
        TST         R7,R7
        BT          ?done
?vertex:
        FMOV.S      @R2+,FR4
        FMOV.S      @R2+,FR5
        FMOV.S      @R2+,FR6
        FLDI0       FR7
        FTRV        XMTRX,FV4
        FMOV.S      @R3+,FR0
        FMOV.S      @R3+,FR1
        FMOV.S      @R3+,FR2
        FLDI1       FR3
        OCBI        @R5
        FLDI0       FR7
        FIPR        FV4,FV4
        ADD         #64,R4
        FSRRA       FR7
        ADD         #32,R2
        PREF        @R2
        ADD         #-32,R2
        MOV.L       R6,@-R4
        FMUL        FR7,FR6
        FMUL        FR7,FR5
        FMUL        FR7,FR4
        FTRV        XMTRX,FV0
        FMOV.S      FR6,@-R4
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        PREF        @R4
        FLDI1       FR3
        FDIV        FR2,FR3
        ADD         #-32,R5
        FMOV.S      @R12,FR6
        FMOV.S      @R13,FR7
        ADD         #-20,R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        OCBI        @R5
        FMUL        FR9,FR1
        FMUL        FR8,FR0
        ADD         #96,R5
        MOV         #0,R1
        ADD         #32,R3
        PREF        @R3
        ADD         #-32,R3
        FMUL        FR3,FR1
        FMUL        FR3,FR0
        FCMP/GT     FR2,FR11
        FADD        FR7,FR1
        BT/S        ?depth
        FADD        FR6,FR0
        FCMP/GT     FR12,FR1
        ROTCL       R1
        FCMP/GT     FR1,FR13
        ROTCL       R1
        FCMP/GT     FR14,FR0
        ROTCL       R1
        FCMP/GT     FR0,FR15
        ROTCL       R1
?depth:
        FCMP/GT     FR10,FR2
        FMOV.S      FR3,@-R4
        ROTCL       R1
        FCMP/GT     FR2,FR11
        FMOV.S      FR1,@-R4
        ROTCL       R1
        FMOV.S      FR0,@-R4
        AND         R1,R8
        MOV.L       R1,@-R4
        OR          R1,R9
        PREF        @R4
        DT          R7
        BF/S        ?vertex
        ADD         #64,R4
?done:
        ADD         #16,R15
        MOV.L       @R15+,R4
        MOV.L       R8,@(24,R4)
        MOV.L       R9,@(28,R4)
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
        MOV.L       @R15+,R12
        MOV.L       @R15+,R13
        MOV.L       @R15+,R14
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        RTS
        FMOV.S      @R15+,FR15
}
#endif
/* clang-format on */

void njCalcModel_Loop8(struct calc_work *work)
{
    loop8_body(work);
}

/* clang-format off */
#ifndef __clang__
/* @asm opcode: cache-line, OCBI @R5 at 0x8c63c904 and 0x8c63c92c, in
 * njCalcModel_Loop9, invalidates a vertex's two lines */
#pragma inline_asm(loop9_body)
static void loop9_body(struct calc_work *work)
{
        FMOV.S      FR15,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR12,@-R15
        MOV.L       R14,@-R15
        MOV.L       R13,@-R15
        MOV.L       R12,@-R15
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       R4,@-R15
        MOV.L       #__nj_far_clip_,R10
        MOV.L       #__nj_near_clip_,R11
        MOV.L       #__nj_screen_+12,R12
        MOV.L       #__nj_screen_+16,R13
        FMOV.S      @R10,FR10
        FMOV.S      @R11,FR11
        FMOV.S      @R12,FR2
        FMOV.S      @R13,FR3
        FMOV.S      FR10,@-R15
        MOV         R15,R10
        FMOV.S      FR11,@-R15
        MOV         R15,R11
        FMOV.S      FR2,@-R15
        MOV         R15,R12
        FMOV.S      FR3,@-R15
        MOV         R15,R13
        MOV.L       #H'FFFFFFFF,R6
        MOV         R4,R1
        MOV.L       @R1+,R7
        MOV.L       @R1+,R5
        MOV.L       @R1+,R3
        MOV.L       @R1+,R2
        FMOV.S      @R1+,FR8
        FMOV.S      @R1+,FR9
        MOV.L       @R1+,R8
        MOV.L       @R1+,R9
        FMOV.S      @R1+,FR15
        FMOV.S      @R1+,FR14
        FMOV.S      @R1+,FR13
        FMOV.S      @R1+,FR12
        MOV         R5,R4
        MOV.L       #H'1C000000,R0
        MOV.L       #H'FF000038,R1
        AND         R5,R0
        SHLR16      R0
        SHLR8       R0
        MOV.L       R0,@(0,R1)
        MOV.L       R0,@(4,R1)
        MOV.L       #H'03FFFFFF,R0
        MOV.L       #H'E0000000,R1
        AND         R0,R4
        OR          R1,R4
        TST         R7,R7
        BT/S        ?done
        ADD         #32,R5
?vertex:
        FMOV.S      @R2+,FR4
        FMOV.S      @R2+,FR5
        FMOV.S      @R2+,FR6
        FLDI0       FR7
        FTRV        XMTRX,FV4
        ADD         #32,R2
        PREF        @R2
        ADD         #-32,R2
        ADD         #64,R4
        OCBI        @R5
        FLDI1       FR3
        MOV.L       R6,@-R4
        FMOV.S      FR6,@-R4
        FMOV.S      FR5,@-R4
        FMOV.S      FR4,@-R4
        FMOV.S      @R3+,FR0
        FMOV.S      @R3+,FR1
        FMOV.S      @R3+,FR2
        PREF        @R4
        FTRV        XMTRX,FV0
        FLDI1       FR3
        ADD         #-32,R5
        FDIV        FR2,FR3
        FMOV.S      @R12,FR6
        FMOV.S      @R13,FR7
        ADD         #-20,R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        OCBI        @R5
        FMUL        FR9,FR1
        FMUL        FR8,FR0
        ADD         #96,R5
        MOV         #0,R1
        ADD         #32,R3
        PREF        @R3
        ADD         #-32,R3
        FMUL        FR3,FR1
        FMUL        FR3,FR0
        FCMP/GT     FR2,FR11
        FADD        FR7,FR1
        BT/S        ?depth
        FADD        FR6,FR0
        FCMP/GT     FR12,FR1
        ROTCL       R1
        FCMP/GT     FR1,FR13
        ROTCL       R1
        FCMP/GT     FR14,FR0
        ROTCL       R1
        FCMP/GT     FR0,FR15
        ROTCL       R1
?depth:
        FCMP/GT     FR10,FR2
        FMOV.S      FR3,@-R4
        ROTCL       R1
        FCMP/GT     FR2,FR11
        FMOV.S      FR1,@-R4
        ROTCL       R1
        FMOV.S      FR0,@-R4
        AND         R1,R8
        MOV.L       R1,@-R4
        OR          R1,R9
        PREF        @R4
        DT          R7
        BF/S        ?vertex
        ADD         #64,R4
?done:
        ADD         #16,R15
        MOV.L       @R15+,R4
        MOV.L       R8,@(24,R4)
        MOV.L       R9,@(28,R4)
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
        MOV.L       @R15+,R12
        MOV.L       @R15+,R13
        MOV.L       @R15+,R14
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        RTS
        FMOV.S      @R15+,FR15
}
#endif
/* clang-format on */

void njCalcModel_Loop9(struct calc_work *work)
{
    loop9_body(work);
}

/* clang-format off */
#ifndef __clang__
/* @asm opcode: cache-line, OCBI @R5 at 0x8c63ca28 and 0x8c63ca46, in
 * njCalcModel_LoopA, invalidates a vertex's two lines */
#pragma inline_asm(loopa_body)
static void loopa_body(struct calc_work *work)
{
        FMOV.S      FR15,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR12,@-R15
        MOV.L       R14,@-R15
        MOV.L       R13,@-R15
        MOV.L       R12,@-R15
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       R4,@-R15
        MOV.L       #__nj_far_clip_,R10
        MOV.L       #__nj_near_clip_,R11
        MOV.L       #__nj_screen_+12,R12
        MOV.L       #__nj_screen_+16,R13
        FMOV.S      @R10,FR10
        FMOV.S      @R11,FR11
        FMOV.S      FR10,@-R15
        MOV         R15,R10
        FMOV.S      FR11,@-R15
        MOV         R15,R11
        MOV.L       #H'FFFFFFFF,R6
        FMOV.S      @R12,FR6
        FMOV.S      @R13,FR7
        MOV         R4,R1
        MOV.L       @R1+,R7
        MOV.L       @R1+,R5
        MOV.L       @R1+,R3
        ADD         #4,R1
        FMOV.S      @R1+,FR8
        FMOV.S      @R1+,FR9
        MOV.L       @R1+,R8
        MOV.L       @R1+,R9
        FMOV.S      @R1+,FR15
        FMOV.S      @R1+,FR14
        FMOV.S      @R1+,FR13
        FMOV.S      @R1+,FR12
        MOV         R5,R4
        MOV.L       #H'1C000000,R0
        MOV.L       #H'FF000038,R1
        AND         R5,R0
        SHLR16      R0
        SHLR8       R0
        MOV.L       R0,@(0,R1)
        MOV.L       R0,@(4,R1)
        MOV.L       #H'03FFFFFF,R0
        MOV.L       #H'E0000000,R1
        AND         R0,R4
        OR          R1,R4
        TST         R7,R7
        BT/S        ?done
        ADD         #32,R5
?vertex:
        ADD         #64,R4
        OCBI        @R5
        FLDI1       FR3
        MOV.L       R6,@-R4
        FMOV.S      @R3+,FR0
        FMOV.S      @R3+,FR1
        FMOV.S      @R3+,FR2
        PREF        @R4
        FTRV        XMTRX,FV0
        FLDI1       FR3
        ADD         #-32,R5
        FDIV        FR2,FR3
        ADD         #-32,R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
        OCBI        @R5
        FMUL        FR9,FR1
        FMUL        FR8,FR0
        ADD         #96,R5
        MOV         #0,R1
        ADD         #32,R3
        PREF        @R3
        ADD         #-32,R3
        FMUL        FR3,FR1
        FMUL        FR3,FR0
        FCMP/GT     FR2,FR11
        FADD        FR7,FR1
        BT/S        ?depth
        FADD        FR6,FR0
        FCMP/GT     FR12,FR1
        ROTCL       R1
        FCMP/GT     FR1,FR13
        ROTCL       R1
        FCMP/GT     FR14,FR0
        ROTCL       R1
        FCMP/GT     FR0,FR15
        ROTCL       R1
?depth:
        FCMP/GT     FR10,FR2
        FMOV.S      FR3,@-R4
        ROTCL       R1
        FCMP/GT     FR2,FR11
        FMOV.S      FR1,@-R4
        ROTCL       R1
        FMOV.S      FR0,@-R4
        AND         R1,R8
        MOV.L       R1,@-R4
        OR          R1,R9
        PREF        @R4
        DT          R7
        BF/S        ?vertex
        ADD         #64,R4
?done:
        ADD         #8,R15
        MOV.L       @R15+,R4
        MOV.L       R8,@(24,R4)
        MOV.L       R9,@(28,R4)
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
        MOV.L       @R15+,R12
        MOV.L       @R15+,R13
        MOV.L       @R15+,R14
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        RTS
        FMOV.S      @R15+,FR15
}
#endif
/* clang-format on */

void njCalcModel_LoopA(struct calc_work *work)
{
    loopa_body(work);
}

/* clang-format off */
#ifndef __clang__
/* transform the point src by XMTRX into dst. src is read whole before dst is
 * written, so the two may be one */
/* @asm open: no inspect rule reads FV0 filled by three loads and FLDI1 FR3
 * before its FTRV */
#pragma inline_asm(CalcModelMatrix)
void CalcModelMatrix(NJS_POINT3 *dst, const NJS_POINT3 *src)
{
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR2
        FLDI1       FR3
        FTRV        XMTRX,FV0
        ADD         #12,R4
        FMOV.S      FR2,@-R4
        FMOV.S      FR1,@-R4
        FMOV.S      FR0,@-R4
}
#endif
/* clang-format on */

/* calculate model's points into the vertex workspace and return 0, or return -1
 * to reject the model. whether model has normals, NJD_CONTROL_3D_VERTEX_OFFSET
 * and _nj_matrix_flag_ choose the point loop: one of loops 1 to 5 under
 * NJD_CONTROL_3D_NO_CLIP_CHECK, and else one of loops 6 to A, which set clip
 * bits. _njCalcModel sets the projection's scales and view_center before any
 * test. it adds the points to _nj_calc_vertex_ before it writes them.
 *
 * under NJD_CONTROL_3D_MODEL_CLIP, it returns -1 before writing a vertex in two
 * cases. one is when the bounding sphere's z range, view_center.z less and plus
 * model->r, lies wholly at or below _nj_near_clip_ or wholly above
 * _nj_far_clip_. the other is when the square that view_center's x and y less
 * and plus model->r bound, projected at depth view_center.z plus model->r, lies
 * outside the 2D clip rectangle.
 *
 * under NJD_CONTROL_3D_DEPTH_QUEUE, it sets _nj_depth_enable_ to 1 when
 * view_center.z plus model->r passes _nj_depth_near_. without
 * NJD_CONTROL_3D_NO_CLIP_CHECK, it returns -1 after writing the vertices when
 * they all share a clip bit, and else sets model_clip_or to their clip bits
 * ored together */
Sint32 _njCalcModel(NJS_MODEL *model)
{
    Uint32 clip_all = 0xffffffff;
    Uint32 clip_any = 0;

    struct model_vertex *vertices = (struct model_vertex *)_nj_vertex_buf_;
    NJS_POINT3 *points = model->points;
    NJS_VECTOR *normals = model->normals;
    struct calc_work work;
    Float sphere_reciprocal;
    Float left;
    Float top;
    Float right;
    Float bottom;

    model_clip_or = 0;
#ifdef GCC
    /* the picture's aspect, which 16:9 sets in place of Ninja's (port.h) */
    projection_x = _nj_screen_.dist * port_draw_aspect;
#else
    projection_x = _nj_screen_.dist * _nj_scr_aspect_x_;
#endif
    projection_y = _nj_screen_.dist * _nj_scr_aspect_y_;
    CalcModelMatrix(&view_center, &model->center);
    if (_nj_control_3d_flag_ & NJD_CONTROL_3D_MODEL_CLIP) {
        Float radius = model->r;
        if (!(view_center.z + radius > _nj_near_clip_)) {
            return -1;
        }
        if (view_center.z - radius > _nj_far_clip_) {
            return -1;
        }
        sphere_reciprocal = 1.0F / (view_center.z + radius);
        left = (projection_x * (view_center.x - radius) * sphere_reciprocal) +
               _nj_screen_.cx;
        top = (projection_y * (view_center.y - radius) * sphere_reciprocal) +
              _nj_screen_.cy;
        right = (projection_x * (view_center.x + radius) * sphere_reciprocal) +
                _nj_screen_.cx;
        bottom = (projection_y * (view_center.y + radius) * sphere_reciprocal) +
                 _nj_screen_.cy;
        if (_nj_2d_clip_[0].x > right || left > _nj_2d_clip_[1].x ||
            bottom < _nj_2d_clip_[0].y || top > _nj_2d_clip_[1].y) {
            return -1;
        }
    }

    if (_nj_control_3d_flag_ & NJD_CONTROL_3D_DEPTH_QUEUE) {
        Float radius = model->r;
        if (view_center.z + radius > _nj_depth_near_) {
            _nj_depth_enable_ = 1;
        }
    }
    _nj_calc_vertex_ += model->nbPoint;
    work.count = model->nbPoint;
    work.vertex = vertices;
    work.points = points;
    work.scale_x = projection_x;
    work.scale_y = projection_y;
    if (_nj_control_3d_flag_ & NJD_CONTROL_3D_NO_CLIP_CHECK) {
        if (normals) {
            work.normals = normals;
            if (_nj_control_3d_flag_ & NJD_CONTROL_3D_VERTEX_OFFSET) {
                if (_nj_matrix_flag_) {
                    njCalcModel_Loop1(&work);
                } else {
                    njCalcModel_Loop2(&work);
                }
            } else {
                if (_nj_matrix_flag_) {
                    njCalcModel_Loop3(&work);
                } else {
                    njCalcModel_Loop4(&work);
                }
            }
        } else {
            njCalcModel_Loop5(&work);
        }
    } else {
        work.clip_and = clip_all;
        work.clip_or = clip_any;
        work.left = _nj_2d_clip_[0].x;
        work.right = _nj_2d_clip_[1].x;
        work.top = _nj_2d_clip_[0].y;
        work.bottom = _nj_2d_clip_[1].y;
        if (normals) {
            work.normals = normals;
            if (_nj_control_3d_flag_ & NJD_CONTROL_3D_VERTEX_OFFSET) {
                if (_nj_matrix_flag_) {
                    njCalcModel_Loop6(&work);
                } else {
                    njCalcModel_Loop7(&work);
                }
            } else {
                if (_nj_matrix_flag_) {
                    njCalcModel_Loop8(&work);
                } else {
                    njCalcModel_Loop9(&work);
                }
            }
        } else {
            njCalcModel_LoopA(&work);
        }

        if (work.clip_and & CLIP_SIDES) {
            return -1;
        }
        if (work.clip_and & CLIP_FAR) {
            return -1;
        }
        if (work.clip_and & CLIP_NEAR) {
            return -1;
        }
        model_clip_or = work.clip_or;
        if (work.clip_or & CLIP_NEAR) {
            near_reciprocal = 1.0F / _nj_near_clip_;
            projection_x_reciprocal = 1.0F / projection_x;
            projection_y_reciprocal = 1.0F / projection_y;
        }
    }
    return 0;
}

/* an empty function, which each strip drawer calls first through DRAW_NOOP */
#ifdef GCC
/* GCC inlines each call */
static inline void model_draw_noop(void)
{
}
#else
/* clang-format off */
#ifndef __clang__
/* @asm open: every instruction is one SHC writes for C */
#pragma inline_asm(model_draw_noop)
static void model_draw_noop(void)
{
}
#endif
/* clang-format on */
#endif

/* @hack spelling: model_draw_noop called through its address, which gives the
 * JSR at each strip drawer's entry, as at 0x8c63e54c in
 * model_draw_smooth_strips_all: called directly, SHC expands its empty body and
 * writes no call, and 14559 of 17334 words differ in 39 functions */
#define DRAW_NOOP() (*(void (*)(void))model_draw_noop)()

/* prefetch the work vertex that the index at mesh_cursor names */
/* @bug memory: with each triangle's third corner the walk
 * prefetches the vertex the next index names, and after a meshset's last
 * strip that index lies past the end of meshes: MOV.W @R14,R3 at 0x8c63e7e8
 * in model_draw_smooth_strips_cull reads it for the PREF at 0x8c63e7fa */
#ifdef GCC
/* repairs the read the tag names: GCC prefetches nothing, so no walk reads
 * past meshes for it. the prefetch only hints the cache */
#define PREFETCH_NEXT(vertices, mesh_cursor) ((void)0)
#else
#define PREFETCH_NEXT(vertices, mesh_cursor)                                   \
    prefetch((void *)((Uint32)(vertices) +                                     \
                      ((Uint32)(mesh_cursor)[0] * sizeof *(vertices))))
#endif

#define WORK_VERTEX(index) (&((struct model_vertex *)_nj_vertex_buf_)[index])

/* pack a, r, g and b into an ARGB word, each clamped to 0 to 1, times 255 and
 * truncated */
static Uint32 model_pack_argb(Float a, Float r, Float g, Float b);
#define PACK_ARGB(color)                                                       \
    model_pack_argb((color).a, (color).r, (color).g, (color).b)
/* unpack color into dst, each byte times 0x3b808080, the float just under
 * 1 / 255 */
static void model_unpack_argb(Uint32 color, NJS_ARGB *dst);

/* vertex's packed diffuse, channel 0, or specular, channel 1 */
/* @hack type: the packed word reached through its float member's address cast
 * to the union, which has SHC form the word's address ahead of the call that
 * packs its color and keep it on the stack, as at 0x8c640798 to 0x8c64079e in
 * model_draw_unlit_tex_strips_all; as colors[channel].packed, 24066 of 29151
 * words differ in 39 functions */
#define PACKED_COLOR(vertex, channel)                                          \
    (((union color_word *)&(vertex)->payload.tex.colors[channel].component)    \
         ->packed)

#define EMIT_CLIPPED(clipped_vertex, packet_command)                           \
    {                                                                          \
        (clipped_vertex)->cmd = (packet_command);                              \
        (clipped_vertex)->color = PACK_ARGB((clipped_vertex)->lit.diffuse);    \
        (clipped_vertex)->offset = PACK_ARGB((clipped_vertex)->lit.specular);  \
        submit_packet(clipped_vertex);                                         \
    }

/* the clippers draw the part of triangle v0, v1, v2 above the near plane, and
 * one corner at least must lie above it. each edge that crosses the plane adds
 * a corner where it crosses: its view x and y, texture coordinates and colors
 * interpolated along the edge, projected at the plane with 1/z near_reciprocal.
 * the corners above the plane stay. three corners go out as a triangle, and
 * four as a strip in the order 0, 1, 3, 2; each counts in _nj_draw_polygon_.
 *
 * a lit clipper's last parameter, light_key, is the key to light the corners
 * under, and its BACK_FACE bit lights them from behind. the clipper lights each
 * corner whose lit_key isn't light_key, and stores the corner's colors in its
 * payload with light_key as its lit_key. it reads the colors of any other
 * corner back from its payload. an environment clipper also sets each corner's
 * texture coordinates to its normal's x and y, each times 0.5 plus 0.5, before
 * it clips */

/* lit and textured, the colors kept packed */
static void model_clip_smooth_tex_triangle(struct model_vertex *v0,
                                           struct model_vertex *v1,
                                           struct model_vertex *v2,
                                           Uint16 light_key)
{
    struct model_vertex *vertices[3];
    Sint32 corner;
    Sint32 previous;
    Sint32 output_count;
    Float edge_t;
    Float view_x;
    Float view_y;

    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    for (corner = 0; corner < 3; corner++) {
        if (vertices[corner]->lit_key != light_key) {
            njCalculateLightsRGB(_njm_col[corner], &vertices[corner]->point,
                                 &vertices[corner]->vector,
                                 light_key & BACK_FACE);
            PACKED_COLOR(vertices[corner], 0) = PACK_ARGB(_njm_col[corner][0]);
            PACKED_COLOR(vertices[corner], 1) = PACK_ARGB(_njm_col[corner][1]);
            vertices[corner]->lit_key = light_key;
        } else {
            model_unpack_argb(PACKED_COLOR(vertices[corner], 0),
                              &_njm_col[corner][0]);
            model_unpack_argb(PACKED_COLOR(vertices[corner], 1),
                              &_njm_col[corner][1]);
        }
    }
    output_count = 0;
    previous = 2;
    for (corner = 0; corner < 3; corner++) {
        if (((vertices[previous]->control.clip & CLIP_NEAR) ^
             (vertices[corner]->control.clip & CLIP_NEAR)) == 1) {
            edge_t = (_nj_near_clip_ - vertices[previous]->point.z) /
                     (vertices[corner]->point.z - vertices[previous]->point.z);
            view_x =
                vertices[previous]->point.x +
                ((vertices[corner]->point.x - vertices[previous]->point.x) *
                 edge_t);
            view_y =
                vertices[previous]->point.y +
                ((vertices[corner]->point.y - vertices[previous]->point.y) *
                 edge_t);
            vtx2[output_count].sx =
                (near_reciprocal * projection_x * view_x) + _nj_screen_.cx;
            vtx2[output_count].sy =
                (near_reciprocal * projection_y * view_y) + _nj_screen_.cy;
            vtx2[output_count].oow = near_reciprocal;
            vtx2[output_count].u = vertices[previous]->payload.tex.uv[0] +
                                   ((vertices[corner]->payload.tex.uv[0] -
                                     vertices[previous]->payload.tex.uv[0]) *
                                    edge_t);
            vtx2[output_count].v = vertices[previous]->payload.tex.uv[1] +
                                   ((vertices[corner]->payload.tex.uv[1] -
                                     vertices[previous]->payload.tex.uv[1]) *
                                    edge_t);
            vtx2[output_count].lit.diffuse.a =
                _njm_col[previous][0].a +
                ((_njm_col[corner][0].a - _njm_col[previous][0].a) * edge_t);
            vtx2[output_count].lit.diffuse.r =
                _njm_col[previous][0].r +
                ((_njm_col[corner][0].r - _njm_col[previous][0].r) * edge_t);
            vtx2[output_count].lit.diffuse.g =
                _njm_col[previous][0].g +
                ((_njm_col[corner][0].g - _njm_col[previous][0].g) * edge_t);
            vtx2[output_count].lit.diffuse.b =
                _njm_col[previous][0].b +
                ((_njm_col[corner][0].b - _njm_col[previous][0].b) * edge_t);
            vtx2[output_count].lit.specular.a =
                _njm_col[previous][1].a +
                ((_njm_col[corner][1].a - _njm_col[previous][1].a) * edge_t);
            vtx2[output_count].lit.specular.r =
                _njm_col[previous][1].r +
                ((_njm_col[corner][1].r - _njm_col[previous][1].r) * edge_t);
            vtx2[output_count].lit.specular.g =
                _njm_col[previous][1].g +
                ((_njm_col[corner][1].g - _njm_col[previous][1].g) * edge_t);
            vtx2[output_count].lit.specular.b =
                _njm_col[previous][1].b +
                ((_njm_col[corner][1].b - _njm_col[previous][1].b) * edge_t);
            output_count++;
        }
        if (!(vertices[corner]->control.clip & CLIP_NEAR)) {
            vtx2[output_count].sx = vertices[corner]->sx;
            vtx2[output_count].sy = vertices[corner]->sy;
            vtx2[output_count].oow = vertices[corner]->oow;
            vtx2[output_count].u = vertices[corner]->payload.tex.uv[0];
            vtx2[output_count].v = vertices[corner]->payload.tex.uv[1];
            vtx2[output_count].lit.diffuse.a = _njm_col[corner][0].a;
            vtx2[output_count].lit.diffuse.r = _njm_col[corner][0].r;
            vtx2[output_count].lit.diffuse.g = _njm_col[corner][0].g;
            vtx2[output_count].lit.diffuse.b = _njm_col[corner][0].b;
            vtx2[output_count].lit.specular.a = _njm_col[corner][1].a;
            vtx2[output_count].lit.specular.r = _njm_col[corner][1].r;
            vtx2[output_count].lit.specular.g = _njm_col[corner][1].g;
            vtx2[output_count].lit.specular.b = _njm_col[corner][1].b;
            output_count++;
        }
        previous = corner;
    }
    if (output_count == 3) {
        EMIT_CLIPPED(&vtx2[0], KM_VERTEXPARAM_NORMAL);
        EMIT_CLIPPED(&vtx2[1], KM_VERTEXPARAM_NORMAL);
        EMIT_CLIPPED(&vtx2[2], KM_VERTEXPARAM_ENDOFSTRIP);
        _nj_draw_polygon_ += 3;
    } else {
        EMIT_CLIPPED(&vtx2[0], KM_VERTEXPARAM_NORMAL);
        EMIT_CLIPPED(&vtx2[1], KM_VERTEXPARAM_NORMAL);
        EMIT_CLIPPED(&vtx2[3], KM_VERTEXPARAM_NORMAL);
        EMIT_CLIPPED(&vtx2[2], KM_VERTEXPARAM_ENDOFSTRIP);
        _nj_draw_polygon_ += 4;
    }
}

/* lit, with environment coordinates, the colors kept packed */
static void model_clip_smooth_env_triangle(struct model_vertex *v0,
                                           struct model_vertex *v1,
                                           struct model_vertex *v2,
                                           Uint16 light_key)
{
    struct model_vertex *vertices[3];
    Sint32 corner;
    Sint32 previous;
    Sint32 output_count;
    Float edge_t;
    Float view_x;
    Float view_y;

    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    for (corner = 0; corner < 3; corner++) {
        if (vertices[corner]->lit_key != light_key) {
            njCalculateLightsRGB(_njm_col[corner], &vertices[corner]->point,
                                 &vertices[corner]->vector,
                                 light_key & BACK_FACE);
            PACKED_COLOR(vertices[corner], 0) = PACK_ARGB(_njm_col[corner][0]);
            PACKED_COLOR(vertices[corner], 1) = PACK_ARGB(_njm_col[corner][1]);
            vertices[corner]->lit_key = light_key;
        } else {
            model_unpack_argb(PACKED_COLOR(vertices[corner], 0),
                              &_njm_col[corner][0]);
            model_unpack_argb(PACKED_COLOR(vertices[corner], 1),
                              &_njm_col[corner][1]);
        }
        vertices[corner]->payload.tex.uv[0] =
            (vertices[corner]->vector.x * 0.5F) + 0.5F;
        vertices[corner]->payload.tex.uv[1] =
            (vertices[corner]->vector.y * 0.5F) + 0.5F;
    }
    output_count = 0;
    previous = 2;
    for (corner = 0; corner < 3; corner++) {
        if (((vertices[previous]->control.clip & CLIP_NEAR) ^
             (vertices[corner]->control.clip & CLIP_NEAR)) == 1) {
            edge_t = (_nj_near_clip_ - vertices[previous]->point.z) /
                     (vertices[corner]->point.z - vertices[previous]->point.z);
            view_x =
                vertices[previous]->point.x +
                ((vertices[corner]->point.x - vertices[previous]->point.x) *
                 edge_t);
            view_y =
                vertices[previous]->point.y +
                ((vertices[corner]->point.y - vertices[previous]->point.y) *
                 edge_t);
            vtx2[output_count].sx =
                (near_reciprocal * projection_x * view_x) + _nj_screen_.cx;
            vtx2[output_count].sy =
                (near_reciprocal * projection_y * view_y) + _nj_screen_.cy;
            vtx2[output_count].oow = near_reciprocal;
            vtx2[output_count].u = vertices[previous]->payload.tex.uv[0] +
                                   ((vertices[corner]->payload.tex.uv[0] -
                                     vertices[previous]->payload.tex.uv[0]) *
                                    edge_t);
            vtx2[output_count].v = vertices[previous]->payload.tex.uv[1] +
                                   ((vertices[corner]->payload.tex.uv[1] -
                                     vertices[previous]->payload.tex.uv[1]) *
                                    edge_t);
            vtx2[output_count].lit.diffuse.a =
                _njm_col[previous][0].a +
                ((_njm_col[corner][0].a - _njm_col[previous][0].a) * edge_t);
            vtx2[output_count].lit.diffuse.r =
                _njm_col[previous][0].r +
                ((_njm_col[corner][0].r - _njm_col[previous][0].r) * edge_t);
            vtx2[output_count].lit.diffuse.g =
                _njm_col[previous][0].g +
                ((_njm_col[corner][0].g - _njm_col[previous][0].g) * edge_t);
            vtx2[output_count].lit.diffuse.b =
                _njm_col[previous][0].b +
                ((_njm_col[corner][0].b - _njm_col[previous][0].b) * edge_t);
            vtx2[output_count].lit.specular.a =
                _njm_col[previous][1].a +
                ((_njm_col[corner][1].a - _njm_col[previous][1].a) * edge_t);
            vtx2[output_count].lit.specular.r =
                _njm_col[previous][1].r +
                ((_njm_col[corner][1].r - _njm_col[previous][1].r) * edge_t);
            vtx2[output_count].lit.specular.g =
                _njm_col[previous][1].g +
                ((_njm_col[corner][1].g - _njm_col[previous][1].g) * edge_t);
            vtx2[output_count].lit.specular.b =
                _njm_col[previous][1].b +
                ((_njm_col[corner][1].b - _njm_col[previous][1].b) * edge_t);
            output_count++;
        }
        if (!(vertices[corner]->control.clip & CLIP_NEAR)) {
            vtx2[output_count].sx = vertices[corner]->sx;
            vtx2[output_count].sy = vertices[corner]->sy;
            vtx2[output_count].oow = vertices[corner]->oow;
            vtx2[output_count].u = vertices[corner]->payload.tex.uv[0];
            vtx2[output_count].v = vertices[corner]->payload.tex.uv[1];
            vtx2[output_count].lit.diffuse.a = _njm_col[corner][0].a;
            vtx2[output_count].lit.diffuse.r = _njm_col[corner][0].r;
            vtx2[output_count].lit.diffuse.g = _njm_col[corner][0].g;
            vtx2[output_count].lit.diffuse.b = _njm_col[corner][0].b;
            vtx2[output_count].lit.specular.a = _njm_col[corner][1].a;
            vtx2[output_count].lit.specular.r = _njm_col[corner][1].r;
            vtx2[output_count].lit.specular.g = _njm_col[corner][1].g;
            vtx2[output_count].lit.specular.b = _njm_col[corner][1].b;
            output_count++;
        }
        previous = corner;
    }
    if (output_count == 3) {
        EMIT_CLIPPED(&vtx2[0], KM_VERTEXPARAM_NORMAL);
        EMIT_CLIPPED(&vtx2[1], KM_VERTEXPARAM_NORMAL);
        EMIT_CLIPPED(&vtx2[2], KM_VERTEXPARAM_ENDOFSTRIP);
        _nj_draw_polygon_ += 3;
    } else {
        EMIT_CLIPPED(&vtx2[0], KM_VERTEXPARAM_NORMAL);
        EMIT_CLIPPED(&vtx2[1], KM_VERTEXPARAM_NORMAL);
        EMIT_CLIPPED(&vtx2[3], KM_VERTEXPARAM_NORMAL);
        EMIT_CLIPPED(&vtx2[2], KM_VERTEXPARAM_ENDOFSTRIP);
        _nj_draw_polygon_ += 4;
    }
}

/* textured, every corner in the material's diffuse color, packed */
static void model_clip_unlit_tex_triangle(struct model_vertex *v0,
                                          struct model_vertex *v1,
                                          struct model_vertex *v2)
{
    struct model_vertex *vertices[3];
    Uint32 diffuse_argb;
    Sint32 corner;
    Sint32 previous;
    Sint32 output_count;

    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    diffuse_argb = PACK_ARGB(_nj_light_material_.diffuse);
    output_count = 0;
    previous = 2;
    for (corner = 0; corner < 3; corner++) {
        if (((vertices[previous]->control.clip & CLIP_NEAR) ^
             (vertices[corner]->control.clip & CLIP_NEAR)) == 1) {
            Float edge_t =
                (_nj_near_clip_ - vertices[previous]->point.z) /
                (vertices[corner]->point.z - vertices[previous]->point.z);
            Float view_x =
                vertices[previous]->point.x +
                ((vertices[corner]->point.x - vertices[previous]->point.x) *
                 edge_t);
            Float view_y =
                vertices[previous]->point.y +
                ((vertices[corner]->point.y - vertices[previous]->point.y) *
                 edge_t);
            vtx2[output_count].sx =
                (near_reciprocal * projection_x * view_x) + _nj_screen_.cx;
            vtx2[output_count].sy =
                (near_reciprocal * projection_y * view_y) + _nj_screen_.cy;
            vtx2[output_count].oow = near_reciprocal;
            vtx2[output_count].u = vertices[previous]->payload.tex.uv[0] +
                                   ((vertices[corner]->payload.tex.uv[0] -
                                     vertices[previous]->payload.tex.uv[0]) *
                                    edge_t);
            vtx2[output_count].v = vertices[previous]->payload.tex.uv[1] +
                                   ((vertices[corner]->payload.tex.uv[1] -
                                     vertices[previous]->payload.tex.uv[1]) *
                                    edge_t);
            vtx2[output_count].color = diffuse_argb;
            output_count++;
        }
        if (!(vertices[corner]->control.clip & CLIP_NEAR)) {
            vtx2[output_count].sx = vertices[corner]->sx;
            vtx2[output_count].sy = vertices[corner]->sy;
            vtx2[output_count].oow = vertices[corner]->oow;
            vtx2[output_count].u = vertices[corner]->payload.tex.uv[0];
            vtx2[output_count].v = vertices[corner]->payload.tex.uv[1];
            vtx2[output_count].color = diffuse_argb;
            output_count++;
        }
        previous = corner;
    }
    if (output_count == 3) {
        vtx2[0].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[0]);
        vtx2[1].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[1]);
        vtx2[2].cmd = KM_VERTEXPARAM_ENDOFSTRIP;
        submit_packet(&vtx2[2]);
        _nj_draw_polygon_ += 3;
    } else {
        vtx2[0].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[0]);
        vtx2[1].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[1]);
        vtx2[3].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[3]);
        vtx2[2].cmd = KM_VERTEXPARAM_ENDOFSTRIP;
        submit_packet(&vtx2[2]);
        _nj_draw_polygon_ += 4;
    }
}

/* with environment coordinates, every corner in the material's diffuse
 * color, packed */
static void model_clip_unlit_env_triangle(struct model_vertex *v0,
                                          struct model_vertex *v1,
                                          struct model_vertex *v2)
{
    struct model_vertex *vertices[3];
    Uint32 diffuse_argb;
    Float edge_t;
    Float view_x;
    Float view_y;
    Sint32 corner;
    Sint32 previous;
    Sint32 corner_index;
    Sint32 output_count;

    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    diffuse_argb = PACK_ARGB(_nj_light_material_.diffuse);
    for (corner_index = 0; corner_index < 3; corner_index++) {
        vertices[corner_index]->payload.tex.uv[0] =
            (vertices[corner_index]->vector.x * 0.5F) + 0.5F;
        vertices[corner_index]->payload.tex.uv[1] =
            (vertices[corner_index]->vector.y * 0.5F) + 0.5F;
    }
    output_count = 0;
    previous = 2;
    for (corner = 0; corner < 3; corner++) {
        if (((vertices[previous]->control.clip & CLIP_NEAR) ^
             (vertices[corner]->control.clip & CLIP_NEAR)) == 1) {
            edge_t = (_nj_near_clip_ - vertices[previous]->point.z) /
                     (vertices[corner]->point.z - vertices[previous]->point.z);
            view_x =
                vertices[previous]->point.x +
                ((vertices[corner]->point.x - vertices[previous]->point.x) *
                 edge_t);
            view_y =
                vertices[previous]->point.y +
                ((vertices[corner]->point.y - vertices[previous]->point.y) *
                 edge_t);
            vtx2[output_count].sx =
                (near_reciprocal * projection_x * view_x) + _nj_screen_.cx;
            vtx2[output_count].sy =
                (near_reciprocal * projection_y * view_y) + _nj_screen_.cy;
            vtx2[output_count].oow = near_reciprocal;
            vtx2[output_count].u = vertices[previous]->payload.tex.uv[0] +
                                   ((vertices[corner]->payload.tex.uv[0] -
                                     vertices[previous]->payload.tex.uv[0]) *
                                    edge_t);
            vtx2[output_count].v = vertices[previous]->payload.tex.uv[1] +
                                   ((vertices[corner]->payload.tex.uv[1] -
                                     vertices[previous]->payload.tex.uv[1]) *
                                    edge_t);
            vtx2[output_count].color = diffuse_argb;
            output_count++;
        }
        if (!(vertices[corner]->control.clip & CLIP_NEAR)) {
            vtx2[output_count].sx = vertices[corner]->sx;
            vtx2[output_count].sy = vertices[corner]->sy;
            vtx2[output_count].oow = vertices[corner]->oow;
            vtx2[output_count].u = vertices[corner]->payload.tex.uv[0];
            vtx2[output_count].v = vertices[corner]->payload.tex.uv[1];
            vtx2[output_count].color = diffuse_argb;
            output_count++;
        }
        previous = corner;
    }
    if (output_count == 3) {
        vtx2[0].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[0]);
        vtx2[1].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[1]);
        vtx2[2].cmd = KM_VERTEXPARAM_ENDOFSTRIP;
        submit_packet(&vtx2[2]);
        _nj_draw_polygon_ += 3;
    } else {
        vtx2[0].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[0]);
        vtx2[1].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[1]);
        vtx2[3].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[3]);
        vtx2[2].cmd = KM_VERTEXPARAM_ENDOFSTRIP;
        submit_packet(&vtx2[2]);
        _nj_draw_polygon_ += 4;
    }
}

/* lit and untextured. a corner's color is the diffuse alpha and the diffuse
 * plus specular red, green and blue, kept as floats */
static void model_clip_smooth_triangle(struct model_vertex *v0,
                                       struct model_vertex *v1,
                                       struct model_vertex *v2,
                                       Uint16 light_key)
{
    struct model_vertex *vertices[3];
    Float edge_t;
    Float view_x;
    Float view_y;
    Sint32 corner;
    Sint32 previous;
    Sint32 output_count;

    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    for (corner = 0; corner < 3; corner++) {
        if (vertices[corner]->lit_key != light_key) {
            njCalculateLightsRGB(_njm_col[corner], &vertices[corner]->point,
                                 &vertices[corner]->vector,
                                 light_key & BACK_FACE);
            vertices[corner]->lit_key = light_key;
            vertices[corner]->payload.col.a = _njm_col[corner][0].a;
            vertices[corner]->payload.col.r = _njm_col[corner][0].r +=
                _njm_col[corner][1].r;
            vertices[corner]->payload.col.g = _njm_col[corner][0].g +=
                _njm_col[corner][1].g;
            vertices[corner]->payload.col.b = _njm_col[corner][0].b +=
                _njm_col[corner][1].b;
        } else {
            _njm_col[corner][0].a = vertices[corner]->payload.col.a;
            _njm_col[corner][0].r = vertices[corner]->payload.col.r;
            _njm_col[corner][0].g = vertices[corner]->payload.col.g;
            _njm_col[corner][0].b = vertices[corner]->payload.col.b;
        }
    }
    output_count = 0;
    previous = 2;
    for (corner = 0; corner < 3; corner++) {
        if (((vertices[previous]->control.clip & CLIP_NEAR) ^
             (vertices[corner]->control.clip & CLIP_NEAR)) == 1) {
            edge_t = (_nj_near_clip_ - vertices[previous]->point.z) /
                     (vertices[corner]->point.z - vertices[previous]->point.z);
            view_x =
                vertices[previous]->point.x +
                ((vertices[corner]->point.x - vertices[previous]->point.x) *
                 edge_t);
            view_y =
                vertices[previous]->point.y +
                ((vertices[corner]->point.y - vertices[previous]->point.y) *
                 edge_t);
            vtx4[output_count].sx =
                (near_reciprocal * projection_x * view_x) + _nj_screen_.cx;
            vtx4[output_count].sy =
                (near_reciprocal * projection_y * view_y) + _nj_screen_.cy;
            vtx4[output_count].oow = near_reciprocal;
            vtx4[output_count].col.a =
                _njm_col[previous][0].a +
                ((_njm_col[corner][0].a - _njm_col[previous][0].a) * edge_t);
            vtx4[output_count].col.r =
                _njm_col[previous][0].r +
                ((_njm_col[corner][0].r - _njm_col[previous][0].r) * edge_t);
            vtx4[output_count].col.g =
                _njm_col[previous][0].g +
                ((_njm_col[corner][0].g - _njm_col[previous][0].g) * edge_t);
            vtx4[output_count].col.b =
                _njm_col[previous][0].b +
                ((_njm_col[corner][0].b - _njm_col[previous][0].b) * edge_t);
            output_count++;
        }
        if (!(vertices[corner]->control.clip & CLIP_NEAR)) {
            vtx4[output_count].sx = vertices[corner]->sx;
            vtx4[output_count].sy = vertices[corner]->sy;
            vtx4[output_count].oow = vertices[corner]->oow;
            vtx4[output_count].col.a = _njm_col[corner][0].a;
            vtx4[output_count].col.r = _njm_col[corner][0].r;
            vtx4[output_count].col.g = _njm_col[corner][0].g;
            vtx4[output_count].col.b = _njm_col[corner][0].b;
            output_count++;
        }
        previous = corner;
    }
    if (output_count == 3) {
        vtx4[0].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx4[0]);
        vtx4[1].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx4[1]);
        vtx4[2].cmd = KM_VERTEXPARAM_ENDOFSTRIP;
        submit_packet(&vtx4[2]);
        _nj_draw_polygon_ += 3;
    } else {
        vtx4[0].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx4[0]);
        vtx4[1].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx4[1]);
        vtx4[3].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx4[3]);
        vtx4[2].cmd = KM_VERTEXPARAM_ENDOFSTRIP;
        submit_packet(&vtx4[2]);
        _nj_draw_polygon_ += 4;
    }
}

/* untextured, every corner in the material's diffuse color */
static void model_clip_unlit_triangle(struct model_vertex *v0,
                                      struct model_vertex *v1,
                                      struct model_vertex *v2)
{
    struct model_vertex *vertices[3];
    Float edge_t;
    Float view_x;
    Float view_y;
    Sint32 corner;
    Sint32 previous;
    Sint32 output_count;

    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    output_count = 0;
    previous = 2;
    for (corner = 0; corner < 3; corner++) {
        if (((vertices[previous]->control.clip & CLIP_NEAR) ^
             (vertices[corner]->control.clip & CLIP_NEAR)) == 1) {
            edge_t = (_nj_near_clip_ - vertices[previous]->point.z) /
                     (vertices[corner]->point.z - vertices[previous]->point.z);
            view_x =
                vertices[previous]->point.x +
                ((vertices[corner]->point.x - vertices[previous]->point.x) *
                 edge_t);
            view_y =
                vertices[previous]->point.y +
                ((vertices[corner]->point.y - vertices[previous]->point.y) *
                 edge_t);
            vtx4[output_count].sx =
                (near_reciprocal * projection_x * view_x) + _nj_screen_.cx;
            vtx4[output_count].sy =
                (near_reciprocal * projection_y * view_y) + _nj_screen_.cy;
            vtx4[output_count].oow = near_reciprocal;
            vtx4[output_count].col.a = _nj_light_material_.diffuse.a;
            vtx4[output_count].col.r = _nj_light_material_.diffuse.r;
            vtx4[output_count].col.g = _nj_light_material_.diffuse.g;
            vtx4[output_count].col.b = _nj_light_material_.diffuse.b;
            output_count++;
        }
        if (!(vertices[corner]->control.clip & CLIP_NEAR)) {
            vtx4[output_count].sx = vertices[corner]->sx;
            vtx4[output_count].sy = vertices[corner]->sy;
            vtx4[output_count].oow = vertices[corner]->oow;
            vtx4[output_count].col.a = _nj_light_material_.diffuse.a;
            vtx4[output_count].col.r = _nj_light_material_.diffuse.r;
            vtx4[output_count].col.g = _nj_light_material_.diffuse.g;
            vtx4[output_count].col.b = _nj_light_material_.diffuse.b;
            output_count++;
        }
        previous = corner;
    }
    if (output_count == 3) {
        vtx4[0].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx4[0]);
        vtx4[1].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx4[1]);
        vtx4[2].cmd = KM_VERTEXPARAM_ENDOFSTRIP;
        submit_packet(&vtx4[2]);
        _nj_draw_polygon_ += 3;
    } else {
        vtx4[0].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx4[0]);
        vtx4[1].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx4[1]);
        vtx4[3].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx4[3]);
        vtx4[2].cmd = KM_VERTEXPARAM_ENDOFSTRIP;
        submit_packet(&vtx4[2]);
        _nj_draw_polygon_ += 4;
    }
}

/* write back the operand cache line holding address */
#ifdef GCC
/* SH4ZAM has no call for OCBWB. the memory clobber keeps each earlier store
 * ahead of the asm, so the line it writes back holds them */
ALWAYS_INLINE void writeback_line(void *address)
{
    __asm__ volatile("ocbwb @%0" : : "r"(address) : "memory");
}
#else
/* clang-format off */
#ifndef __clang__
/* @asm open: its OCBWB lies only in the C functions it expands into */
#pragma inline_asm(writeback_line(size=2))
static void writeback_line(void *address)
{
        OCBWB       @R4
}
#endif
/* clang-format on */
#endif

#ifdef GCC
/* the shipped head leaves the address of vertex's second cache line, where
 * its payload starts, in FPUL for the tail. GCC's code takes FPUL for its own
 * conversions between integers and floats, so here the head prefetches both
 * lines, and the tail nothing */
ALWAYS_INLINE void prefetch_vertex_head(struct model_vertex *vertex)
{
    SHZ_PREFETCH(vertex);
    SHZ_PREFETCH(&vertex->payload);
}

ALWAYS_INLINE void prefetch_vertex_tail(void)
{
}
#else
/* clang-format off */
#ifndef __clang__
/* prefetch vertex's first cache line, and leave the address of its second in
 * FPUL for prefetch_vertex_tail */
/* LDS R4,FPUL at 0x8c63e5a8, in model_draw_smooth_strips_all, keeps the address
 * in FPUL for prefetch_vertex_tail */
/* @asm open: no inspect rule reads the FPUL it leaves prefetch_vertex_tail */
#pragma inline_asm(prefetch_vertex_head(size=6))
static void prefetch_vertex_head(struct model_vertex *vertex)
{
        PREF        @R4
        ADD         #32,R4
        LDS         R4,FPUL
}
#endif
/* clang-format on */

/* clang-format off */
#ifndef __clang__
/* prefetch the cache line at the address in FPUL */
/* STS FPUL,R0 at 0x8c63e5c6, in model_draw_smooth_strips_all, takes the address
 * prefetch_vertex_head left in FPUL */
/* @asm open: no inspect rule reads FPUL at its entry */
#pragma inline_asm(prefetch_vertex_tail(size=4))
static void prefetch_vertex_tail(void)
{
        STS         FPUL,R0
        PREF        @R0
}
#endif
/* clang-format on */
#endif

#ifdef GCC
/* the dot product of point and normal. FIPR takes the normal, then point, as
 * the shipped FIPR FV4,FV0 does. GCC gives both a fourth lane of 0, so the
 * fourth term is 0. the shipped body's is NaN for an infinity or a NaN in
 * FR7 */
ALWAYS_INLINE Float view_point_normal_dot(NJS_POINT3 *point, NJS_VECTOR *normal)
{
    return zam_fipr(shz_vec4_init(normal->x, normal->y, normal->z, 0.0F),
                    shz_vec4_init(point->x, point->y, point->z, 0.0F));
}
#else
/* clang-format off */
#ifndef __clang__
/* the dot product of point and normal. FIPR's fourth term is 0 times
 * whatever FR7 holds */
/* @hack spelling: size=32, against the body's 18 bytes, SHC's estimate of each
 * expansion, by which it places the pools of the functions around it; with
 * size=18, 2087 of 3362 words differ in 9 functions */
/* @asm open: no inspect rule reads FV0 and FV4 filled by three loads each
 * before its FIPR */
#pragma inline_asm(view_point_normal_dot(size=32))
static Float view_point_normal_dot(NJS_POINT3 *point, NJS_VECTOR *normal)
{
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR1
        FMOV.S      @R4+,FR2
        FMOV.S      @R5+,FR4
        FMOV.S      @R5+,FR5
        FMOV.S      @R5+,FR6
        FLDI0       FR3
        FIPR        FV4,FV0
        FMOV.S      FR3,FR0
}
#endif
/* clang-format on */
#endif

/* set uv[0] to normal->x * half + half and uv[1] to normal->y * half + half,
 * then write back uv[1]'s cache line */
#ifdef GCC
ALWAYS_INLINE void store_environment_uv(Float *uv, NJS_VECTOR *normal,
                                        Float half)
{
    uv[0] = (normal->x * half) + half;
    uv[1] = (normal->y * half) + half;
    writeback_line(&uv[1]);
}
#else
/* clang-format off */
#ifndef __clang__
/* @asm open: its OCBWB lies only in the C functions it expands into */
#pragma inline_asm(store_environment_uv(size=20))
static void store_environment_uv(Float *uv, NJS_VECTOR *normal, Float half)
{
        FMOV.S      @R5+,FR3
        FMUL        FR4,FR3
        FADD        FR4,FR3
        FMOV.S      FR3,@R4
        FMOV.S      @R5,FR2
        ADD         #4,R4
        FMUL        FR4,FR2
        FADD        FR4,FR2
        FMOV.S      FR2,@R4
        OCBWB       @R4
}
#endif
/* clang-format on */
#endif

/* copy *color into vertex's floating color, then write back the cache line of
 * its last channel */
#ifdef GCC
ALWAYS_INLINE void store_vertex_color(struct model_vertex *vertex,
                                      NJS_ARGB *color)
{
    vertex->payload.col = *color;
    writeback_line(&vertex->payload.col.b);
}
#else
/* clang-format off */
#ifndef __clang__
/* @asm open: its OCBWB lies only in the C functions it expands into */
#pragma inline_asm(store_vertex_color(size=26))
static void store_vertex_color(struct model_vertex *vertex, NJS_ARGB *color)
{
        FMOV.S      @R5+,FR3
        ADD         #32,R4
        FMOV.S      FR3,@R4
        ADD         #4,R4
        FMOV.S      @R5+,FR3
        FMOV.S      FR3,@R4
        FMOV.S      @R5+,FR3
        ADD         #4,R4
        FMOV.S      FR3,@R4
        ADD         #4,R4
        FMOV.S      @R5,FR2
        FMOV.S      FR2,@R4
        OCBWB       @R4
}
#endif
/* clang-format on */
#endif

#ifdef GCC
/* the triple product c . (a x b), whose sign gives the winding of triangle
 * a, b, c seen from the eye. FIPR takes c, then a x b, as the shipped FIPR
 * FV4,FV0 does. each holds its y, z and x in its first three lanes, where the
 * shipped loads and FSUBs leave them. GCC gives both a fourth lane of 0, as in
 * view_point_normal_dot */
ALWAYS_INLINE Float view_triangle_facing(NJS_POINT3 *b, NJS_POINT3 *a,
                                         NJS_POINT3 *c)
{
    shz_vec3_t cross = shz_vec3_cross(shz_vec3_deref(a), shz_vec3_deref(b));

    return zam_fipr(shz_vec4_init(c->y, c->z, c->x, 0.0F),
                    shz_vec4_init(cross.y, cross.z, cross.x, 0.0F));
}
#else
/* clang-format off */
#ifndef __clang__
/* the triple product c . (a x b), whose sign gives the winding of triangle
 * a, b, c seen from the eye. FIPR's fourth term is 0 times whatever FR7
 * holds */
/* @hack spelling: size=96, against the body's 50 bytes, SHC's estimate of each
 * expansion, by which it places the pools of the functions around it; with
 * size=50, 10 of the 645 words of model_draw_smooth_tex_strips_near and
 * model_draw_flat_path_tex_strips_near differ in each */
/* @asm open: no inspect rule reads FV0 filled by differences in registers
 * before its FIPR */
#pragma inline_asm(view_triangle_facing(size=96))
static Float view_triangle_facing(NJS_POINT3 *b, NJS_POINT3 *a, NJS_POINT3 *c)
{
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR1
        FMOV.S      @R4+,FR2
        FMOV.S      @R5+,FR4
        FMOV.S      @R5+,FR5
        FMOV.S      @R5+,FR6
        FMOV.S      FR0,FR8
        FMOV.S      FR1,FR9
        FMOV.S      FR2,FR10
        FMUL        FR5,FR2
        FMUL        FR6,FR0
        FMUL        FR4,FR1
        FMUL        FR6,FR9
        FMUL        FR4,FR10
        FMUL        FR5,FR8
        FSUB        FR9,FR2
        FSUB        FR10,FR0
        FSUB        FR8,FR1
        FMOV.S      @R6+,FR6
        FMOV.S      @R6+,FR4
        FMOV.S      @R6+,FR5
        FLDI0       FR3
        FIPR        FV4,FV0
        FMOV.S      FR3,FR0
}
#endif
/* clang-format on */
#endif

/* the strip drawers draw a trimesh meshset from the work vertices. a strip is
 * its point count, with NJD_TRIMESH_END set where its winding is reversed, then
 * the points' indices. a strip needs two points at least in the _all drawers
 * and three in the others. every drawer calls model_draw_noop first, and adds
 * the points to _nj_calc_polygon_.
 *
 * the _all drawers send every point, lit from behind where its normal points
 * away from the eye, and add the points to _nj_draw_polygon_. the _cull drawers
 * send each run of triangles whose screen winding faces the eye as one strip,
 * and add each run's packets, and one more, to _nj_draw_polygon_. the _checked
 * drawers send runs as the _cull drawers do, but skip a triangle whose corners
 * share a clip bit, and send a two-sided material's back faces lit from behind.
 * the _near drawers do as the _checked drawers do, but judge a triangle with a
 * corner below the near plane by its winding in view space, and send it alone,
 * through a clipper. the _checked and _near drawers add the points to
 * _nj_draw_polygon_.
 *
 * a smooth or flat_path drawer lights each vertex. an unlit drawer gives each
 * vertex the material's diffuse color. the flat_path drawers draw a flat
 * material's trimeshes. a tex or env drawer sends packed colors beside texture
 * coordinates: a base color, and from a lit drawer an offset color too. a tex
 * drawer takes the coordinates from the meshset's vertuv, each Sint16 over 256.
 * an env drawer makes environment coordinates, the normal's x and y, each times
 * 0.5 plus 0.5. any other drawer sends one floating color.
 *
 * an EMIT_ macro sends a vertex with the command packet_command. the
 * EMIT_SMOOTH_, EMIT_CACHED_ and EMIT_CHECKED_ macros light it first, unless
 * its lit_key already holds their key. the macros evaluate their arguments more
 * than once, and some use scratch locals of the caller's, which their comments
 * name */

/* float views of a color, of a diffuse and specular pair and of a normal, for
 * the EMIT_ macros' pointer walks */
union argb_floats {
    NJS_ARGB argb;
    Float f[4];
};

union light_floats {
    NJS_ARGB argb[2];
    Float f[8];
};

union vector_floats {
    NJS_VECTOR vector;
    Float f[3];
};

/* send vertex lit from light_side, in a floating color: the diffuse alpha,
 * and the diffuse plus specular red, green and blue. scratch: vertex_command,
 * light_key */
/* @hack spelling: the key built in two statements, light_side and then
 * material_id, which takes light_side's register first, MOV R9,R4 and OR
 * R11,R4 at 0x8c63e5de in model_draw_smooth_strips_all; as one expression, 4
 * of its 238 words differ */
/* @hack spelling: Float pointers walk the light pair from the diffuse's a and
 * the specular's r through light_floats, and the vertex color's a, r, g and b
 * through argb_floats, which gives the FMOV.S @Rn+ loads and the ADD #4 store
 * steps of the summed color (0x8c63e600 to 0x8c63e622 in
 * model_draw_smooth_strips_all); read and stored by member, 220 of 238 words
 * differ in model_draw_smooth_strips_all and the section is 48 bytes longer */
#define EMIT_SMOOTH_SIDED_SPLIT(vertex, packet_command, material_id,           \
                                light_side)                                    \
    {                                                                          \
        vertex_command = (packet_command);                                     \
        (vertex)->control.cmd = vertex_command;                                \
        writeback_line(vertex);                                                \
        light_key = (light_side);                                              \
        light_key |= (material_id);                                            \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(_njm_col2, &(vertex)->point,                  \
                                 &(vertex)->vector, light_side);               \
            {                                                                  \
                Float *diffuse;                                                \
                Float *specular;                                               \
                Float *color_out;                                              \
                diffuse = ((union light_floats *)_njm_col2)->f;                \
                color_out = ((union argb_floats *)&(vertex)->payload.col)->f;  \
                specular = diffuse + 5;                                        \
                *color_out++ = *diffuse++;                                     \
                *color_out++ = *diffuse++ + *specular++;                       \
                *color_out++ = *diffuse++ + *specular++;                       \
                *color_out = *diffuse + *specular;                             \
                writeback_line(color_out);                                     \
            }                                                                  \
        }                                                                      \
        submit_work_vertex(vertex);                                            \
    }

/* the same, as the polygon drawers send a corner. scratch: light_key */
/* @hack spelling: Float pointers walk _njm_col2[0] from a and _njm_col2[1] from
 * r, and the vertex color's a, r, g and b through argb_floats, which gives the
 * FMOV.S @Rn+ loads and the ADD #4 store steps of the summed color (0x8c64777e
 * to 0x8c6477a0 in model_draw_smooth_polygons); read and stored by member, 2017
 * of 2138 words differ in model_draw_smooth_polygons and
 * model_draw_smooth_polygons_clip and the section is 224 bytes longer */
#define EMIT_SMOOTH_POLYGON(vertex, packet_command, material_id, light_side)   \
    {                                                                          \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        light_key = (material_id) | (light_side);                              \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(_njm_col2, &(vertex)->point,                  \
                                 &(vertex)->vector, light_side);               \
            {                                                                  \
                Float *diffuse;                                                \
                Float *specular;                                               \
                Float *color_out;                                              \
                color_out = ((union argb_floats *)&(vertex)->payload.col)->f;  \
                diffuse = &_njm_col2[0].a;                                     \
                specular = &_njm_col2[1].r;                                    \
                *color_out++ = *diffuse++;                                     \
                *color_out++ = *diffuse++ + *specular++;                       \
                *color_out++ = *diffuse++ + *specular++;                       \
                *color_out = *diffuse + *specular;                             \
                writeback_line(color_out);                                     \
            }                                                                  \
        }                                                                      \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_smooth_strips_all(NJS_MESHSET *meshset,
                                         Uint32 material_index)
{
    register struct model_vertex *vertex;
    register Uint16 *mesh_cursor;
    register struct model_vertex *vertices;
    register Uint32 material_id;
    register Uint32 remaining;
    register Uint32 light_side;
    Uint32 last_light_side;
    Uint32 input_vertices;
    Uint32 strips_left;
    Uint32 light_key;
    Uint32 vertex_command;
    Uint32 mask;
    Float dot;

    DRAW_NOOP();
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    material_id = material_index;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c63e566 over its literal pool to the strip
     * loop at 0x8c63e57c; without it, 225 of 238 words differ, the function
     * runs 12 bytes short, and 398 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
next_strip:
    mask = STRIP_COUNT;
    remaining = *mesh_cursor++ & mask;
    input_vertices += remaining;
    remaining--;
    do {
        vertex = &vertices[*mesh_cursor++];
        prefetch_vertex_head(&vertices[*mesh_cursor]);
        dot = view_point_normal_dot(&vertex->point, &vertex->vector);
        prefetch_vertex_tail();
        light_side = BACK_FACE;
        if (dot <= 0.0F) {
            light_side = 0;
        }
        EMIT_SMOOTH_SIDED_SPLIT(vertex, KM_VERTEXPARAM_NORMAL, material_id,
                                light_side);
    } while (--remaining != 0);
    vertex = &vertices[*mesh_cursor++];
    dot = view_point_normal_dot(&vertex->point, &vertex->vector);
    last_light_side = BACK_FACE;
    if (dot <= 0.0F) {
        last_light_side = 0;
    }
    EMIT_SMOOTH_SIDED_SPLIT(vertex, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                            last_light_side);
    if (--strips_left != 0) {
        goto next_strip;
    }
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

/* send vertex in a floating color as EMIT_SMOOTH_SIDED_SPLIT does, but lit from
 * the front into light, unless its lit_key is already key */
/* @hack spelling: Float pointers walk the light pair from the diffuse's a and
 * the specular's r through light_floats, and the vertex color's a, r, g and b
 * through argb_floats, which gives the FMOV.S @Rn+ loads and the ADD #4 store
 * steps of the summed color (0x8c63e85c to 0x8c63e87c in
 * model_draw_smooth_strips_cull); read and stored by member, 333 of 451 words
 * differ in model_draw_smooth_strips_cull and the section is 40 bytes longer */
#define EMIT_SMOOTH_CULL(vertex, packet_command, light, key)                   \
    {                                                                          \
        Uint32 light_key;                                                      \
        Float *diffuse;                                                        \
        Float *specular;                                                       \
        Float *color_out;                                                      \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        light_key = key;                                                       \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(light, &(vertex)->point, &(vertex)->vector,   \
                                 0);                                           \
            diffuse = ((union light_floats *)(light))->f;                      \
            specular = diffuse + 5;                                            \
            color_out = ((union argb_floats *)&(vertex)->payload.col)->f;      \
            *color_out++ = *diffuse++;                                         \
            *color_out++ = *diffuse++ + *specular++;                           \
            *color_out++ = *diffuse++ + *specular++;                           \
            *color_out = *diffuse + *specular;                                 \
            writeback_line(color_out);                                         \
        }                                                                      \
        submit_work_vertex(vertex);                                            \
    }

/* EMIT_SMOOTH_CULL, taking vertex's color address before light's */
/* @hack spelling: Float pointers walk the light pair from the diffuse's a and
 * the specular's r through light_floats, and the vertex color's a, r, g and b
 * through argb_floats, which gives the FMOV.S @Rn+ loads and the ADD #4 store
 * steps of the summed color (0x8c63fcf4 to 0x8c63fd18 in
 * model_draw_flat_path_strips_cull); read and stored by member, 331 of 451
 * words differ in model_draw_flat_path_strips_cull and the section is 32 bytes
 * longer */
#define EMIT_SMOOTH_CULL_DST_FIRST(vertex, packet_command, light, key)         \
    {                                                                          \
        Uint32 light_key;                                                      \
        Float *diffuse;                                                        \
        Float *specular;                                                       \
        Float *color_out;                                                      \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        light_key = key;                                                       \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(light, &(vertex)->point, &(vertex)->vector,   \
                                 0);                                           \
            color_out = ((union argb_floats *)&(vertex)->payload.col)->f;      \
            diffuse = ((union light_floats *)(light))->f;                      \
            specular = diffuse + 5;                                            \
            *color_out++ = *diffuse++;                                         \
            *color_out++ = *diffuse++ + *specular++;                           \
            *color_out++ = *diffuse++ + *specular++;                           \
            *color_out = *diffuse + *specular;                                 \
            writeback_line(color_out);                                         \
        }                                                                      \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_smooth_strips_cull(NJS_MESHSET *meshset,
                                          Uint32 material_id)
{
    register Uint16 *mesh_cursor;
    register struct model_vertex *vertices;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    register struct model_vertex *v1;
    register struct model_vertex *v0;
    register Float winding;
    register Float y1;
    Float x0;
    Float y0;
    Float x1;
    Float facing;
    Uint32 light_side = 0;
    Uint32 remaining;
    Uint32 strips_left;
    Uint32 input_vertices;
    Uint32 accounted_bytes;

    DRAW_NOOP();
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = accounted_bytes = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c63e774 over its literal pool to the strip
     * loop at 0x8c63e78c; without it, 433 of 451 words differ, the function
     * runs 16 bytes short, and 2 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        y0 = v0->sy;
        x1 = v1->sx;
        y1 = v1->sy;
        run_start = queue_cursor;
    next_triangle:
        v2 = &vertices[*mesh_cursor++];
        PREFETCH_NEXT(vertices, mesh_cursor);
        facing = (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                 winding;
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            if (facing < 0.0F) {
                EMIT_SMOOTH_CULL(v1, KM_VERTEXPARAM_NORMAL, _njm_col2,
                                 material_id | light_side);
            } else {
                EMIT_SMOOTH_CULL(v1, KM_VERTEXPARAM_ENDOFSTRIP, _njm_col2,
                                 material_id | light_side);
                accounted_bytes +=
                    (Uint32)queue_cursor - (Uint32)run_start + PACKET_BYTES;
                run_start = queue_cursor;
            }
        } else if (facing < 0.0F) {
            EMIT_SMOOTH_CULL(v0, KM_VERTEXPARAM_NORMAL, _njm_col2,
                             material_id | light_side);
            EMIT_SMOOTH_CULL(v1, KM_VERTEXPARAM_NORMAL, _njm_col2,
                             material_id | light_side);
        }
        v0 = v1;
        v1 = v2;
        x0 = x1;
        y0 = y1;
        x1 = v2->sx;
        winding = -winding;
        y1 = v2->sy;
        if (--remaining != 0) {
            goto next_triangle;
        }
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            EMIT_SMOOTH_CULL(v2, KM_VERTEXPARAM_ENDOFSTRIP, _njm_col2,
                             material_id | light_side);
            accounted_bytes +=
                (Uint32)queue_cursor - (Uint32)run_start + PACKET_BYTES;
        }
    } while (--strips_left != 0);
    _nj_draw_polygon_ += accounted_bytes >> PACKET_SHIFT;
    _nj_calc_polygon_ += input_vertices;
}

/* send vertex lit from light_side in packed colors, with corner_uv's texture
 * coordinates, and advance corner_uv */
/* @hack spelling: a Sint16 pointer walks the corner's tex.u into tex.v, which
 * gives the MOV.W @Rn+ load of u (0x8c63ec00 in
 * model_draw_smooth_tex_strips_all); read by member through the corner, 342 of
 * 584 words differ in model_draw_smooth_tex_strips_all and
 * model_draw_flat_path_tex_strips_all and the section is 16 bytes longer */
#define EMIT_SMOOTH_TEX(vertex, packet_command, material_id, light_side,       \
                        corner_uv)                                             \
    {                                                                          \
        Uint32 vertex_command;                                                 \
        Uint32 light_key;                                                      \
        register NJS_ARGB *light_color;                                        \
        Float *uv_out;                                                         \
        Sint16 *uv_in;                                                         \
        Float uv_scale;                                                        \
        vertex_command = (packet_command);                                     \
        (vertex)->control.cmd = vertex_command;                                \
        writeback_line(vertex);                                                \
        light_key = (material_id) | (light_side);                              \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(_njm_col2, &(vertex)->point,                  \
                                 &(vertex)->vector, light_side);               \
            PACKED_COLOR(vertex, 0) =                                          \
                (light_color = _njm_col2, PACK_ARGB(*light_color));            \
            PACKED_COLOR(vertex, 1) =                                          \
                (++light_color, PACK_ARGB(*light_color));                      \
        }                                                                      \
        uv_out = (vertex)->payload.tex.uv;                                     \
        uv_in = &((corner_uv)++)->tex.u;                                       \
        uv_scale = 1.0F / 256.0F;                                              \
        *uv_out++ = *uv_in++ * uv_scale;                                       \
        *uv_out = *uv_in * uv_scale;                                           \
        writeback_line(&(vertex)->payload);                                    \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_smooth_tex_strips_all(NJS_MESHSET *meshset,
                                             Uint32 material_id)
{
    Uint32 input_vertices;
    register Uint32 remaining;
    register struct model_vertex *vertex;
    register struct model_vertex *vertices;
    register NJS_COLOR *corner_uv;
    register Uint16 *mesh_cursor;
    Uint32 strips_left;
    Uint32 mask;
    Uint32 light_side;
    Float dot;

    DRAW_NOOP();
    corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;

    /* @hack redundant: a never-run increment of input_vertices, which gives it
     * the frame's first word, MOV.L R2,@R15 at 0x8c63eb18; without it, it takes
     * @(4,R15), and 8 of 292 words differ */
    if (0) {
        input_vertices++;
    }
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c63eb1a over its literal pool to the strip
     * loop at 0x8c63eb40; without it, 271 of 292 words differ, the function
     * runs 28 bytes short, and 577 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
next_strip:
    mask = STRIP_COUNT;
    remaining = *mesh_cursor++ & mask;
    input_vertices += remaining;
    remaining--;
    do {
        vertex = &vertices[*mesh_cursor++];
        prefetch_vertex_head(&vertices[*mesh_cursor]);
        dot = view_point_normal_dot(&vertex->point, &vertex->vector);
        prefetch_vertex_tail();
        light_side = BACK_FACE;
        if (dot <= 0.0F) {
            light_side = 0;
        }
        EMIT_SMOOTH_TEX(vertex, KM_VERTEXPARAM_NORMAL, material_id, light_side,
                        corner_uv);
    } while (--remaining);
    vertex = &vertices[*mesh_cursor++];
    dot = view_point_normal_dot(&vertex->point, &vertex->vector);
    light_side = BACK_FACE;
    if (dot <= 0.0F) {
        light_side = 0;
    }
    EMIT_SMOOTH_TEX(vertex, KM_VERTEXPARAM_ENDOFSTRIP, material_id, light_side,
                    corner_uv);
    if (--strips_left != 0) {
        goto next_strip;
    }
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

/* send vertex in packed colors, lit with njCalculateLightsRGB's reverse set to
 * reverse but under the key material_id | light_side, and with corner_uv's
 * texture coordinates, advancing corner_uv. it writes back the cache line at
 * payload_address. scratch: light_key */
/* @hack spelling: a Sint16 pointer walks the corner's tex.u into tex.v, which
 * gives the MOV.W @Rn+ load of u; read by member through the corner, 862 of
 * 1180 words differ in model_draw_smooth_tex_strips_cull and
 * model_draw_flat_path_tex_strips_cull and the section is 24 bytes longer */
#define EMIT_SMOOTH_TEX_CULL(vertex, packet_command, payload_address,          \
                             material_id, light_side, reverse, corner_uv)      \
    {                                                                          \
        Float *uv_out;                                                         \
        Sint16 *uv_in;                                                         \
        Float uv_scale;                                                        \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        light_key = (material_id) | (light_side);                              \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(_njm_col2, &(vertex)->point,                  \
                                 &(vertex)->vector, reverse);                  \
            PACKED_COLOR(vertex, 0) = PACK_ARGB(_njm_col2[0]);                 \
            PACKED_COLOR(vertex, 1) = PACK_ARGB(_njm_col2[1]);                 \
        }                                                                      \
        uv_out = (vertex)->payload.tex.uv;                                     \
        uv_in = &((corner_uv)++)->tex.u;                                       \
        uv_scale = 1.0F / 256.0F;                                              \
        *uv_out++ = *uv_in++ * uv_scale;                                       \
        *uv_out = *uv_in * uv_scale;                                           \
        writeback_line(payload_address);                                       \
        submit_work_vertex(vertex);                                            \
    }

/* EMIT_SMOOTH_TEX_CULL for a run's first corner. scratch: light_key,
 * specular, specular_copy */
/* @hack spelling: specular's blue read through a second pointer, which gives
 * the float loads at 0x8c63ee18 and 0x8c63ee28, in
 * model_draw_smooth_tex_strips_cull, FR4 and FR5; as PACK_ARGB(_njm_col2[1]),
 * they trade, and 6 of the 590 words of that function and of
 * model_draw_flat_path_tex_strips_cull differ in each */
/* @hack spelling: a Sint16 pointer walks the corner's tex.u into tex.v, which
 * gives the MOV.W @Rn+ load of u; read by member through the corner, 512 of
 * 1180 words differ in model_draw_smooth_tex_strips_cull and
 * model_draw_flat_path_tex_strips_cull and the section is 4 bytes longer */
#define EMIT_SMOOTH_TEX_CULL_OPEN(vertex, packet_command, payload_address,     \
                                  material_id, light_side, reverse, corner_uv) \
    {                                                                          \
        Float *uv_out;                                                         \
        Sint16 *uv_in;                                                         \
        Float uv_scale;                                                        \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        light_key = (material_id) | (light_side);                              \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(_njm_col2, &(vertex)->point,                  \
                                 &(vertex)->vector, reverse);                  \
            PACKED_COLOR(vertex, 0) = PACK_ARGB(_njm_col2[0]);                 \
            PACKED_COLOR(vertex, 1) =                                          \
                (specular_copy = (specular = &_njm_col2[1]),                   \
                 model_pack_argb(specular->a, specular->r, specular->g,        \
                                 specular_copy->b));                           \
        }                                                                      \
        uv_out = (vertex)->payload.tex.uv;                                     \
        uv_in = &((corner_uv)++)->tex.u;                                       \
        uv_scale = 1.0F / 256.0F;                                              \
        *uv_out++ = *uv_in++ * uv_scale;                                       \
        *uv_out = *uv_in * uv_scale;                                           \
        writeback_line(payload_address);                                       \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_smooth_tex_strips_cull(NJS_MESHSET *meshset,
                                              Uint32 material_id)
{
    Uint32 light_side = 0;
    register NJS_COLOR *corner_uv;
    register Uint16 *mesh_cursor;
    register struct model_vertex *v0;
    register struct model_vertex *v1;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    struct model_vertex *vertices;
    Uint32 strips_left;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 accounted_bytes;
    register Float winding;
    register Float x0;
    register Float y0;
    Float x1;
    Float y1;
    Float facing;
    Uint32 light_key;
    NJS_ARGB *specular;
    NJS_ARGB *specular_copy;
    Float *v1_payload;
    Float one;
    Uint32 mask;

    DRAW_NOOP();
    corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = accounted_bytes = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c63edac over its literal pool to the strip
     * loop at 0x8c63edd8; without it, 570 of 590 words differ, the function
     * runs 36 bytes short, and 9 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        one = 1.0F;
        mask = NJD_TRIMESH_END;
        winding = one;
        if (remaining & mask) {
            one = -1.0F;
            mask = STRIP_COUNT;
            winding = one;
            remaining &= mask;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        v1 = &vertices[*mesh_cursor++];
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
    next_triangle:
        v2 = &vertices[*mesh_cursor++];
        PREFETCH_NEXT(vertices, mesh_cursor);
        facing = (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                 winding;
        v1_payload = &v1->payload.tex.uv[0];
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            if (facing < 0.0F) {
                EMIT_SMOOTH_TEX_CULL(v1, KM_VERTEXPARAM_NORMAL, v1_payload,
                                     material_id, light_side, 0, corner_uv);
            } else {
                EMIT_SMOOTH_TEX_CULL(v1, KM_VERTEXPARAM_ENDOFSTRIP, v1_payload,
                                     material_id, light_side, 0, corner_uv);
                corner_uv--;
                accounted_bytes +=
                    (Uint32)queue_cursor - (Uint32)run_start + PACKET_BYTES;
                run_start = queue_cursor;
            }
        } else {
            corner_uv++;
            if (facing < 0.0F) {
                corner_uv--;
                EMIT_SMOOTH_TEX_CULL_OPEN(v0, KM_VERTEXPARAM_NORMAL,
                                          &v0->payload, material_id, light_side,
                                          0, corner_uv);
                EMIT_SMOOTH_TEX_CULL(v1, KM_VERTEXPARAM_NORMAL, v1_payload,
                                     material_id, light_side, 0, corner_uv);
            }
        }
        v0 = v1;
        v1 = v2;
        x0 = x1;
        y0 = y1;
        x1 = v2->sx;
        y1 = v2->sy;
        winding = -winding;
        if (--remaining != 0) {
            goto next_triangle;
        }
        corner_uv += 2;
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            corner_uv -= 2;
            EMIT_SMOOTH_TEX_CULL(v2, KM_VERTEXPARAM_ENDOFSTRIP, &v2->payload,
                                 material_id, light_side, 0, corner_uv);
            accounted_bytes +=
                (Uint32)queue_cursor - (Uint32)run_start + PACKET_BYTES;
        }
    } while (--strips_left);
    _nj_draw_polygon_ += accounted_bytes >> PACKET_SHIFT;
    _nj_calc_polygon_ += input_vertices;
}

/* EMIT_SMOOTH_SIDED_SPLIT with the key in one expression */
/* @hack spelling: Float pointers walk the light pair from the diffuse's a and
 * the specular's r through light_floats, and the vertex color's a, r, g and b
 * through argb_floats, which gives the FMOV.S @Rn+ loads and the ADD #4 store
 * steps of the summed color (0x8c63fa9c to 0x8c63fabe in
 * model_draw_flat_path_strips_all); read and stored by member, 212 of 230 words
 * differ in model_draw_flat_path_strips_all and the section is 48 bytes
 * longer */
#define EMIT_SMOOTH_SIDED(vertex, packet_command, material_id, light_side)     \
    {                                                                          \
        vertex_command = (packet_command);                                     \
        (vertex)->control.cmd = vertex_command;                                \
        writeback_line(vertex);                                                \
        light_key = (material_id) | (light_side);                              \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(_njm_col2, &(vertex)->point,                  \
                                 &(vertex)->vector, light_side);               \
            {                                                                  \
                Float *diffuse;                                                \
                Float *specular;                                               \
                Float *color_out;                                              \
                diffuse = ((union light_floats *)_njm_col2)->f;                \
                color_out = ((union argb_floats *)&(vertex)->payload.col)->f;  \
                specular = diffuse + 5;                                        \
                *color_out++ = *diffuse++;                                     \
                *color_out++ = *diffuse++ + *specular++;                       \
                *color_out++ = *diffuse++ + *specular++;                       \
                *color_out = *diffuse + *specular;                             \
                writeback_line(color_out);                                     \
            }                                                                  \
        }                                                                      \
        submit_work_vertex(vertex);                                            \
    }

/* send vertex lit from light_side in packed colors, with environment
 * coordinates. scratch: vertex_command, light_key, light_color, half,
 * vertex_uv */
/* @hack spelling: the key built in two statements, as in
 * EMIT_SMOOTH_SIDED_SPLIT, MOV R9,R4 and OR R11,R4 at 0x8c63f332 in
 * model_draw_smooth_env_strips_all; as one expression, 4 of its 286 words
 * differ */
#define EMIT_SMOOTH_ENV_SIDED_SPLIT(vertex, packet_command, material_id,       \
                                    light_side)                                \
    {                                                                          \
        vertex_command = (packet_command);                                     \
        (vertex)->control.cmd = vertex_command;                                \
        writeback_line(vertex);                                                \
        light_key = light_side;                                                \
        light_key |= (material_id);                                            \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(_njm_col2, &(vertex)->point,                  \
                                 &(vertex)->vector, light_side);               \
            PACKED_COLOR(vertex, 0) =                                          \
                (light_color = _njm_col2, PACK_ARGB(*light_color));            \
            PACKED_COLOR(vertex, 1) =                                          \
                (++light_color, PACK_ARGB(*light_color));                      \
        }                                                                      \
        half = 0.5F;                                                           \
        vertex_uv = &(vertex)->payload.tex.uv[0];                              \
        store_environment_uv(vertex_uv, &(vertex)->vector, half);              \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_smooth_env_strips_all(NJS_MESHSET *meshset,
                                             Uint32 material_index)
{
    register struct model_vertex *vertex;
    register Uint16 *mesh_cursor;
    register struct model_vertex *vertices;
    register Uint32 material_id;
    register Uint32 remaining;
    register Uint32 light_side;
    NJS_ARGB *light_color;
    register Uint32 last_light_side;
    Uint32 strips_left;
    Uint32 input_vertices;
    Uint32 light_key;
    Uint32 vertex_command;
    Uint32 mask;
    Float half;
    Float dot;
    Float *vertex_uv;

    DRAW_NOOP();
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    material_id = material_index;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c63f2a0 over its literal pool to the strip
     * loop at 0x8c63f2d0; without it, 264 of 286 words differ, the function
     * runs 44 bytes short, and 561 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
next_strip:
    mask = STRIP_COUNT;
    remaining = *mesh_cursor++ & mask;
    input_vertices += remaining;
    remaining--;
    do {
        vertex = &vertices[*mesh_cursor++];
        prefetch_vertex_head(&vertices[*mesh_cursor]);
        dot = view_point_normal_dot(&vertex->point, &vertex->vector);
        prefetch_vertex_tail();
        light_side = BACK_FACE;
        if (dot <= 0.0F) {
            light_side = 0;
        }
        EMIT_SMOOTH_ENV_SIDED_SPLIT(vertex, KM_VERTEXPARAM_NORMAL, material_id,
                                    light_side);
    } while (--remaining != 0);
    vertex = &vertices[*mesh_cursor++];
    dot = view_point_normal_dot(&vertex->point, &vertex->vector);
    last_light_side = BACK_FACE;
    if (dot <= 0.0F) {
        last_light_side = 0;
    }
    EMIT_SMOOTH_ENV_SIDED_SPLIT(vertex, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                                last_light_side);
    if (--strips_left != 0) {
        goto next_strip;
    }
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

/* send vertex in packed colors, lit from the front unless its lit_key is
 * already key, and with environment coordinates. scratch: light_key */
/* @hack spelling: a Float pointer walks the vertex normal's x into y through
 * vector_floats, which gives the FMOV.S @Rn+ load of x (0x8c63f662 in
 * model_draw_smooth_env_strips_cull); read by member, 93 of 1135 words differ
 * in model_draw_smooth_env_strips_cull and
 * model_draw_flat_path_env_strips_cull */
#define EMIT_SMOOTH_ENV_CULL(vertex, packet_command, key)                      \
    {                                                                          \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        light_key = (key);                                                     \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(_njm_col2, &(vertex)->point,                  \
                                 &(vertex)->vector, 0);                        \
            PACKED_COLOR(vertex, 0) = PACK_ARGB(_njm_col2[0]);                 \
            PACKED_COLOR(vertex, 1) = PACK_ARGB(_njm_col2[1]);                 \
        }                                                                      \
        {                                                                      \
            Float *uv_out;                                                     \
            Float *normal_xy;                                                  \
            Float half;                                                        \
            uv_out = (vertex)->payload.tex.uv;                                 \
            normal_xy = ((union vector_floats *)&(vertex)->vector)->f;         \
            half = 0.5F;                                                       \
            *uv_out++ = (*normal_xy++ * half) + half;                          \
            *uv_out = (*normal_xy * half) + half;                              \
            writeback_line(uv_out);                                            \
        }                                                                      \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_smooth_env_strips_cull(NJS_MESHSET *meshset,
                                              Uint32 material_id)
{
    Uint32 light_side;
    register Uint16 *mesh_cursor;
    Uint32 strips_left;
    register struct model_vertex *vertices;
    register struct model_vertex *v0;
    register struct model_vertex *v1;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 accounted_bytes;
    register Float winding;
    register Float x0;
    Float y0;
    Float x1;
    register Float y1;
    Float facing;
    Uint32 light_key;
    Float one;
    Uint32 mask;

    light_side = 0;
    DRAW_NOOP();
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = accounted_bytes = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c63f532 over its literal pool to the strip
     * loop at 0x8c63f55c; without it, 539 of 566 words differ, the function
     * runs 34 bytes short, and 8 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        one = 1.0F;
        mask = NJD_TRIMESH_END;
        winding = one;
        if (remaining & mask) {
            one = -1.0F;
            mask = STRIP_COUNT;
            winding = one;
            remaining &= mask;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
    triangle:
        v2 = &vertices[*mesh_cursor++];
        PREFETCH_NEXT(vertices, mesh_cursor);
        facing = (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                 winding;
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            if (facing < 0.0F) {
                EMIT_SMOOTH_ENV_CULL(v1, KM_VERTEXPARAM_NORMAL,
                                     material_id | light_side);
            } else {
                EMIT_SMOOTH_ENV_CULL(v1, KM_VERTEXPARAM_ENDOFSTRIP,
                                     material_id | light_side);
                accounted_bytes +=
                    (Uint32)queue_cursor - (Uint32)run_start + PACKET_BYTES;
                run_start = queue_cursor;
            }
        } else if (facing < 0.0F) {
            EMIT_SMOOTH_ENV_CULL(v0, KM_VERTEXPARAM_NORMAL,
                                 material_id | light_side);
            EMIT_SMOOTH_ENV_CULL(v1, KM_VERTEXPARAM_NORMAL,
                                 material_id | light_side);
        }
        v0 = v1;
        v1 = v2;
        x0 = x1;
        y0 = y1;
        x1 = v2->sx;
        winding = -winding;
        y1 = v2->sy;
        if (--remaining != 0) {
            goto triangle;
        }
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            EMIT_SMOOTH_ENV_CULL(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                                 material_id | light_side);
            accounted_bytes +=
                (Uint32)queue_cursor - (Uint32)run_start + PACKET_BYTES;
        }
    } while (--strips_left != 0);
    _nj_draw_polygon_ += accounted_bytes >> PACKET_SHIFT;
    _nj_calc_polygon_ += input_vertices;
}

static void model_draw_flat_path_strips_all(NJS_MESHSET *meshset,
                                            Uint32 material_index)
{
    NJS_MESHSET *source_meshset = meshset;
    Uint32 material_id = material_index;
    register struct model_vertex *vertex;
    register Uint32 remaining;
    register Uint16 *mesh_cursor;
    register struct model_vertex *vertices;
    register Uint32 light_side;
    Uint32 input_vertices;
    Uint32 strips_left;
    Uint32 light_key;
    Uint32 vertex_command;
    Uint32 mask;
    register Float dot;

    DRAW_NOOP();
    mesh_cursor = (Uint16 *)source_meshset->meshes;
    strips_left = source_meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c63f9ea over its literal pool to the strip
     * loop at 0x8c63fa1c; without it, 210 of 230 words differ, the function
     * runs 42 bytes short, and 444 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        mask = STRIP_COUNT;
        remaining = *mesh_cursor++ & mask;
        input_vertices += remaining;
        remaining--;
        do {
            vertex = &vertices[*mesh_cursor++];
            prefetch_vertex_head(&vertices[*mesh_cursor]);
            dot = view_point_normal_dot(&vertex->point, &vertex->vector);
            prefetch_vertex_tail();
            light_side = BACK_FACE;
            if (dot <= 0.0F) {
                light_side = 0;
            }
            EMIT_SMOOTH_SIDED(vertex, KM_VERTEXPARAM_NORMAL, material_id,
                              light_side);
        } while (--remaining != 0);
        vertex = &vertices[*mesh_cursor++];
        dot = view_point_normal_dot(&vertex->point, &vertex->vector);
        light_side = BACK_FACE;
        if (dot <= 0.0F) {
            light_side = 0;
        }
        EMIT_SMOOTH_SIDED(vertex, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                          light_side);
    } while (--strips_left != 0);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

static void model_draw_flat_path_strips_cull(NJS_MESHSET *meshset,
                                             register Uint32 material_index)
{
    NJS_MESHSET *source_meshset = meshset;
    NJS_ARGB *light;
    register Uint16 *mesh_cursor;
    struct model_vertex *vertices;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    register struct model_vertex *v1;
    register struct model_vertex *v0;

    register Float winding;
    register Float y1;
    Float x0;
    register Float y0;
    Float x1;
    Float facing;
    Uint32 remaining;

    Uint32 material_id;
    Uint32 light_side;

    struct {
        Uint32 drawn;
        Uint32 total;
        Uint32 strips;
    } totals;

    material_id = material_index;
    light = _njm_col2;

    light_side = 0;
    DRAW_NOOP();
    mesh_cursor = (Uint16 *)source_meshset->meshes;
    totals.strips = source_meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    totals.total = totals.drawn = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c63fc0c over its literal pool to the strip
     * loop at 0x8c63fc28; without it, 435 of 451 words differ, the function
     * runs 16 bytes short, and 2 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        totals.total += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
    next_triangle:
        v2 = &vertices[*mesh_cursor++];
        PREFETCH_NEXT(vertices, mesh_cursor);
        facing = (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                 winding;
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            if (facing < 0.0F) {
                EMIT_SMOOTH_CULL_DST_FIRST(v1, KM_VERTEXPARAM_NORMAL, light,
                                           material_id | light_side);
            } else {
                EMIT_SMOOTH_CULL_DST_FIRST(v1, KM_VERTEXPARAM_ENDOFSTRIP, light,
                                           material_id | light_side);
                totals.drawn +=
                    (Uint32)queue_cursor - (Uint32)run_start + PACKET_BYTES;
                run_start = queue_cursor;
            }
        } else if (facing < 0.0F) {
            EMIT_SMOOTH_CULL_DST_FIRST(v0, KM_VERTEXPARAM_NORMAL, light,
                                       material_id | light_side);
            EMIT_SMOOTH_CULL_DST_FIRST(v1, KM_VERTEXPARAM_NORMAL, light,
                                       material_id | light_side);
        }
        v0 = v1;
        v1 = v2;
        x0 = x1;
        y0 = y1;
        x1 = v2->sx;
        winding = -winding;
        y1 = v2->sy;
        if (--remaining != 0) {
            goto next_triangle;
        }
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            EMIT_SMOOTH_CULL_DST_FIRST(v2, KM_VERTEXPARAM_ENDOFSTRIP, light,
                                       material_id | light_side);
            totals.drawn +=
                (Uint32)queue_cursor - (Uint32)run_start + PACKET_BYTES;
        }
    } while (--totals.strips != 0);
    _nj_draw_polygon_ += totals.drawn >> PACKET_SHIFT;
    _nj_calc_polygon_ += totals.total;
}

static void model_draw_flat_path_tex_strips_all(NJS_MESHSET *meshset,
                                                Uint32 material_id)
{
    Uint32 input_vertices;
    register Uint32 remaining;
    register struct model_vertex *vertex;
    register struct model_vertex *vertices;
    register NJS_COLOR *corner_uv;
    register Uint16 *mesh_cursor;
    Uint32 strips_left;
    Uint32 mask;
    Uint32 light_side;
    Float dot;

    DRAW_NOOP();
    corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;

    /* @hack redundant: a never-run increment of input_vertices, which gives it
     * the frame's first word, MOV.L R2,@R15 at 0x8c63ffb0; without it, it takes
     * @(4,R15), and 8 of 292 words differ */
    if (0) {
        input_vertices++;
    }
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c63ffb2 over its literal pool to the strip
     * loop at 0x8c63ffd4; without it, 271 of 292 words differ, the function
     * runs 24 bytes short, and 569 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
next_strip:
    mask = STRIP_COUNT;
    remaining = *mesh_cursor++ & mask;
    input_vertices += remaining;
    remaining--;
    do {
        vertex = &vertices[*mesh_cursor++];
        prefetch_vertex_head(&vertices[*mesh_cursor]);
        dot = view_point_normal_dot(&vertex->point, &vertex->vector);
        prefetch_vertex_tail();
        light_side = BACK_FACE;
        if (dot <= 0.0F) {
            light_side = 0;
        }
        EMIT_SMOOTH_TEX(vertex, KM_VERTEXPARAM_NORMAL, material_id, light_side,
                        corner_uv);
    } while (--remaining);
    vertex = &vertices[*mesh_cursor++];
    dot = view_point_normal_dot(&vertex->point, &vertex->vector);
    light_side = BACK_FACE;
    if (dot <= 0.0F) {
        light_side = 0;
    }
    EMIT_SMOOTH_TEX(vertex, KM_VERTEXPARAM_ENDOFSTRIP, material_id, light_side,
                    corner_uv);
    if (--strips_left != 0) {
        goto next_strip;
    }
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

static void model_draw_flat_path_tex_strips_cull(NJS_MESHSET *meshset,
                                                 Uint32 material_id)
{
    Uint32 light_side = 0;
    register NJS_COLOR *corner_uv;
    register Uint16 *mesh_cursor;
    register struct model_vertex *v0;
    register struct model_vertex *v1;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    struct model_vertex *vertices;
    Uint32 strips_left;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 accounted_bytes;
    register Float winding;
    register Float x0;
    register Float y0;
    Float x1;
    Float y1;
    Float facing;
    Uint32 light_key;
    NJS_ARGB *specular;
    NJS_ARGB *specular_copy;
    Float *v1_payload;
    Float one;
    Uint32 mask;

    DRAW_NOOP();
    corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = accounted_bytes = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c640240 over its literal pool to the strip
     * loop at 0x8c64026c; without it, 570 of 590 words differ, the function
     * runs 36 bytes short, and 9 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        one = 1.0F;
        mask = NJD_TRIMESH_END;
        winding = one;
        if (remaining & mask) {
            one = -1.0F;
            mask = STRIP_COUNT;
            winding = one;
            remaining &= mask;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        v1 = &vertices[*mesh_cursor++];
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
    next_triangle:
        v2 = &vertices[*mesh_cursor++];
        PREFETCH_NEXT(vertices, mesh_cursor);
        facing = (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                 winding;
        v1_payload = &v1->payload.tex.uv[0];
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            if (facing < 0.0F) {
                EMIT_SMOOTH_TEX_CULL(v1, KM_VERTEXPARAM_NORMAL, v1_payload,
                                     material_id, light_side, 0, corner_uv);
            } else {
                EMIT_SMOOTH_TEX_CULL(v1, KM_VERTEXPARAM_ENDOFSTRIP, v1_payload,
                                     material_id, light_side, 0, corner_uv);
                corner_uv--;
                accounted_bytes +=
                    (Uint32)queue_cursor - (Uint32)run_start + PACKET_BYTES;
                run_start = queue_cursor;
            }
        } else {
            corner_uv++;
            if (facing < 0.0F) {
                corner_uv--;
                EMIT_SMOOTH_TEX_CULL_OPEN(v0, KM_VERTEXPARAM_NORMAL,
                                          &v0->payload, material_id, light_side,
                                          0, corner_uv);
                EMIT_SMOOTH_TEX_CULL(v1, KM_VERTEXPARAM_NORMAL, v1_payload,
                                     material_id, light_side, 0, corner_uv);
            }
        }
        v0 = v1;
        v1 = v2;
        x0 = x1;
        y0 = y1;
        x1 = v2->sx;
        y1 = v2->sy;
        winding = -winding;
        if (--remaining != 0) {
            goto next_triangle;
        }
        corner_uv += 2;
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            corner_uv -= 2;
            EMIT_SMOOTH_TEX_CULL(v2, KM_VERTEXPARAM_ENDOFSTRIP, &v2->payload,
                                 material_id, light_side, 0, corner_uv);
            accounted_bytes +=
                (Uint32)queue_cursor - (Uint32)run_start + PACKET_BYTES;
        }
    } while (--strips_left);
    _nj_draw_polygon_ += accounted_bytes >> PACKET_SHIFT;
    _nj_calc_polygon_ += input_vertices;
}

/* send vertex in the material's diffuse color, packed, with corner_uv's
 * texture coordinates, read and stored through pointers of the storage
 * classes uv_in_class and uv_out_class, and advance corner_uv */
/* @hack spelling: a Sint16 pointer walks the corner's tex.u into tex.v, which
 * gives the MOV.W @Rn+ load of u (0x8c6407be in
 * model_draw_unlit_tex_strips_all); read by member through the corner, 115 of
 * 184 words differ in model_draw_unlit_tex_strips_all and the section is 4
 * bytes longer */
#define EMIT_UNLIT_TEX(vertex, packet_command, corner_uv, uv_in_class,         \
                       uv_out_class)                                           \
    {                                                                          \
        Uint32 vertex_command;                                                 \
        uv_out_class Float *uv_out;                                            \
        uv_in_class Sint16 *uv_in;                                             \
        Float uv_scale;                                                        \
        vertex_command = (packet_command);                                     \
        (vertex)->control.cmd = vertex_command;                                \
        writeback_line(vertex);                                                \
        PACKED_COLOR(vertex, 0) = PACK_ARGB(_nj_light_material_.diffuse);      \
        uv_out = (vertex)->payload.tex.uv;                                     \
        uv_in = &((corner_uv)++)->tex.u;                                       \
        uv_scale = 1.0F / 256.0F;                                              \
        *uv_out++ = *uv_in++ * uv_scale;                                       \
        *uv_out = *uv_in * uv_scale;                                           \
        writeback_line(&(vertex)->payload);                                    \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_unlit_tex_strips_all(NJS_MESHSET *meshset,
                                            Uint32 material_id)
{
    Uint32 input_vertices;
    register Uint32 remaining;
    register struct model_vertex *vertex;
    register struct model_vertex *vertices;
    register NJS_COLOR *corner_uv;
    register Uint16 *mesh_cursor;
    Uint32 strips_left;
    Uint32 mask;

    DRAW_NOOP();
    input_vertices = 0;
    corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c64072a over its literal pool to the strip
     * loop at 0x8c640760; without it, 166 of 184 words differ, the function
     * runs 50 bytes short, and 429 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    for (;;) {
        mask = STRIP_COUNT;
        remaining = *mesh_cursor++ & mask;
        input_vertices += remaining;
        remaining--;
        do {
            vertex = &vertices[*mesh_cursor++];
            prefetch_vertex_head(&vertices[*mesh_cursor]);
            prefetch_vertex_tail();
            EMIT_UNLIT_TEX(vertex, KM_VERTEXPARAM_NORMAL, corner_uv, auto,
                           auto);
        } while (--remaining);
        vertex = &vertices[*mesh_cursor++];
        EMIT_UNLIT_TEX(vertex, KM_VERTEXPARAM_ENDOFSTRIP, corner_uv, register,
                       auto);
        if (--strips_left == 0) {
            break;
        }
    }
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

/* EMIT_UNLIT_TEX without the local vertex_command, writing back the cache line
 * at payload_address */
/* @hack spelling: a Sint16 pointer walks the corner's tex.u into tex.v, which
 * gives the MOV.W @Rn+ load of u (0x8c6409e6 in
 * model_draw_unlit_tex_strips_cull); read by member through the corner, 528 of
 * 872 words differ in model_draw_unlit_tex_strips_cull and
 * model_draw_unlit_tex_strips_checked and the section is 16 bytes longer */
#define EMIT_UNLIT_TEX_CULL(vertex, packet_command, payload_address,           \
                            corner_uv, uv_in_class, uv_out_class)              \
    {                                                                          \
        uv_out_class Float *uv_out;                                            \
        uv_in_class Sint16 *uv_in;                                             \
        Float uv_scale;                                                        \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        PACKED_COLOR(vertex, 0) = PACK_ARGB(_nj_light_material_.diffuse);      \
        uv_out = (vertex)->payload.tex.uv;                                     \
        uv_in = &((corner_uv)++)->tex.u;                                       \
        uv_scale = 1.0F / 256.0F;                                              \
        *uv_out++ = *uv_in++ * uv_scale;                                       \
        *uv_out = *uv_in * uv_scale;                                           \
        writeback_line(payload_address);                                       \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_unlit_tex_strips_cull(NJS_MESHSET *meshset,
                                             Uint32 material_id)
{
    register Uint16 *mesh_cursor;
    register NJS_COLOR *corner_uv;
    Uint32 strips_left;
    struct model_vertex *vertices;
    register struct model_vertex *v0;
    register struct model_vertex *v1;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 accounted_bytes;
    register Float winding;
    register Float y1;
    register Float x1;
    Float x0;
    Float y0;
    Float facing;
    void *v1_payload;

    DRAW_NOOP();
    corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = accounted_bytes = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c640902 over its literal pool to the strip
     * loop at 0x8c640914; without it, 396 of 430 words differ, the function
     * runs 12 bytes short */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;

        /* @hack redundant: a never-run store to x0, which gives x0 FR4 and y0
         * FR5, loaded at 0x8c640954 and 0x8c640964; without it, they trade, and
         * 6 of 430 words differ */
        if (0) {
            x0 = 0.0F;
        }
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
        do {
            v2 = &vertices[*mesh_cursor++];
            PREFETCH_NEXT(vertices, mesh_cursor);
            facing =
                (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                winding;
            v1_payload = &v1->payload;
            if ((Uint32)run_start < (Uint32)queue_cursor) {
                if (facing < 0.0F) {
                    EMIT_UNLIT_TEX_CULL(v1, KM_VERTEXPARAM_NORMAL, v1_payload,
                                        corner_uv, auto, auto);
                } else {
                    EMIT_UNLIT_TEX_CULL(v1, KM_VERTEXPARAM_ENDOFSTRIP,
                                        v1_payload, corner_uv, auto, auto);
                    corner_uv--;
                    accounted_bytes +=
                        (Uint32)queue_cursor - (Uint32)run_start + PACKET_BYTES;
                    run_start = queue_cursor;
                }
            } else {
                corner_uv++;
                if (facing < 0.0F) {
                    corner_uv--;
                    EMIT_UNLIT_TEX_CULL(v0, KM_VERTEXPARAM_NORMAL, &v0->payload,
                                        corner_uv, register, auto);
                    EMIT_UNLIT_TEX_CULL(v1, KM_VERTEXPARAM_NORMAL, v1_payload,
                                        corner_uv, auto, auto);
                }
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            y1 = v2->sy;
            winding = -winding;
        } while (--remaining);
        corner_uv += 2;
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            corner_uv -= 2;
            EMIT_UNLIT_TEX_CULL(v2, KM_VERTEXPARAM_ENDOFSTRIP, &v2->payload,
                                corner_uv, auto, auto);
            accounted_bytes +=
                (Uint32)queue_cursor - (Uint32)run_start + PACKET_BYTES;
        }
    } while (--strips_left);
    _nj_draw_polygon_ += accounted_bytes >> PACKET_SHIFT;
    _nj_calc_polygon_ += input_vertices;
}

/* EMIT_SMOOTH_ENV_SIDED_SPLIT with the key in one expression, and the step to
 * the specular color written light_color + 1 */
/* @hack spelling: the step to the specular color written light_color + 1,
 * which puts remaining, the strip's point count, in R12 and the vertex table
 * in R11 (0x8c640cba, 0x8c640c86) in model_draw_flat_path_env_strips_all; as
 * ++light_color, they trade, and 24 of its 278 words differ */
#define EMIT_SMOOTH_ENV_SIDED(vertex, packet_command, material_id, light_side) \
    {                                                                          \
        vertex_command = (packet_command);                                     \
        (vertex)->control.cmd = vertex_command;                                \
        writeback_line(vertex);                                                \
        light_key = (material_id) | (light_side);                              \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(_njm_col2, &(vertex)->point,                  \
                                 &(vertex)->vector, light_side);               \
            PACKED_COLOR(vertex, 0) =                                          \
                (light_color = _njm_col2, PACK_ARGB(*light_color));            \
            PACKED_COLOR(vertex, 1) =                                          \
                (light_color = light_color + 1, PACK_ARGB(*light_color));      \
        }                                                                      \
        half = 0.5F;                                                           \
        vertex_uv = &(vertex)->payload.tex.uv[0];                              \
        store_environment_uv(vertex_uv, &(vertex)->vector, half);              \
        submit_work_vertex(vertex);                                            \
    }

/* send vertex in the material's diffuse color, packed, with environment
 * coordinates. scratch: vertex_command, uv_out, normal_xy, half */
/* @hack spelling: a Float pointer walks the vertex normal's x into y through
 * vector_floats, which gives the FMOV.S @Rn+ load of x (0x8c641444 in
 * model_draw_unlit_env_strips_all); read by member, 20 of 169 words differ in
 * model_draw_unlit_env_strips_all */
#define EMIT_UNLIT_ENV(vertex, packet_command)                                 \
    {                                                                          \
        vertex_command = (packet_command);                                     \
        (vertex)->control.cmd = vertex_command;                                \
        writeback_line(vertex);                                                \
        PACKED_COLOR(vertex, 0) = PACK_ARGB(_nj_light_material_.diffuse);      \
        uv_out = (vertex)->payload.tex.uv;                                     \
        normal_xy = ((union vector_floats *)&(vertex)->vector)->f;             \
        half = 0.5F;                                                           \
        *uv_out++ = (*normal_xy++ * half) + half;                              \
        *uv_out = (*normal_xy * half) + half;                                  \
        writeback_line(uv_out);                                                \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_flat_path_env_strips_all(NJS_MESHSET *meshset,
                                                Uint32 material_index)
{
    NJS_MESHSET *source_meshset = meshset;
    Uint32 material_id = material_index;
    register Uint16 *mesh_cursor;
    Uint32 strips_left;
    register struct model_vertex *vertices;
    register struct model_vertex *vertex;
    register Uint32 remaining;
    Uint32 input_vertices;
    Uint32 mask;
    Uint32 light_key;
    Uint32 vertex_command;
    NJS_ARGB *light_color;
    Float half;
    Float dot;
    Float *vertex_uv;

    DRAW_NOOP();
    mesh_cursor = (Uint16 *)source_meshset->meshes;
    strips_left = source_meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c640c88 over its literal pool to the strip
     * loop at 0x8c640cb8; without it, 260 of 278 words differ, the function
     * runs 42 bytes short, and 567 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        mask = STRIP_COUNT;
        remaining = *mesh_cursor++ & mask;
        input_vertices += remaining;
        remaining--;
        do {
            vertex = &vertices[*mesh_cursor++];
            prefetch_vertex_head(&vertices[*mesh_cursor]);
            dot = view_point_normal_dot(&vertex->point, &vertex->vector);
            prefetch_vertex_tail();
            {
                register Uint32 light_side = BACK_FACE;
                if (dot <= 0.0F) {
                    light_side = 0;
                }
                EMIT_SMOOTH_ENV_SIDED(vertex, KM_VERTEXPARAM_NORMAL,
                                      material_id, light_side);
            }
        } while (--remaining != 0);
        vertex = &vertices[*mesh_cursor++];
        dot = view_point_normal_dot(&vertex->point, &vertex->vector);
        {
            register Uint32 light_side = BACK_FACE;
            if (dot <= 0.0F) {
                light_side = 0;
            }
            EMIT_SMOOTH_ENV_SIDED(vertex, KM_VERTEXPARAM_ENDOFSTRIP,
                                  material_id, light_side);
        }
    } while (--strips_left != 0);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

static void model_draw_flat_path_env_strips_cull(NJS_MESHSET *meshset,
                                                 Uint32 material_index)
{
    NJS_MESHSET *source_meshset = meshset;
    Uint32 material_id;
    Uint32 light_side;
    register Uint16 *mesh_cursor;
    Uint32 strips_left;
    struct model_vertex *vertices;
    register struct model_vertex *v0;
    register struct model_vertex *v1;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 accounted_bytes;
    register Float winding;
    Float x0;
    register Float y0;
    Float x1;
    Float y1;
    Float facing;
    Uint32 light_key;
    Float one;
    Uint32 mask;

    material_id = material_index;
    light_side = 0;
    DRAW_NOOP();
    mesh_cursor = (Uint16 *)source_meshset->meshes;
    strips_left = source_meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = accounted_bytes = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c640f10 over its literal pool to the strip
     * loop at 0x8c640f38; without it, 541 of 569 words differ, the function
     * runs 28 bytes short, and 8 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
next_strip:
    remaining = *mesh_cursor++;
    one = 1.0F;
    mask = NJD_TRIMESH_END;
    winding = one;
    if (remaining & mask) {
        one = -1.0F;
        mask = STRIP_COUNT;
        winding = one;
        remaining &= mask;
    }
    input_vertices += remaining;
    remaining -= 2;
    v0 = &vertices[*mesh_cursor++];
    v1 = &vertices[*mesh_cursor++];
    x0 = v0->sx;
    x1 = v1->sx;
    y0 = v0->sy;
    y1 = v1->sy;
    run_start = queue_cursor;
next_triangle:
    v2 = &vertices[*mesh_cursor++];
    PREFETCH_NEXT(vertices, mesh_cursor);
    facing =
        (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) * winding;
    if ((Uint32)run_start < (Uint32)queue_cursor) {
        if (facing < 0.0F) {
            EMIT_SMOOTH_ENV_CULL(v1, KM_VERTEXPARAM_NORMAL,
                                 material_id | light_side);
        } else {
            EMIT_SMOOTH_ENV_CULL(v1, KM_VERTEXPARAM_ENDOFSTRIP,
                                 material_id | light_side);
            accounted_bytes +=
                (Uint32)queue_cursor - (Uint32)run_start + PACKET_BYTES;
            run_start = queue_cursor;
        }
    } else if (facing < 0.0F) {
        EMIT_SMOOTH_ENV_CULL(v0, KM_VERTEXPARAM_NORMAL,
                             material_id | light_side);
        EMIT_SMOOTH_ENV_CULL(v1, KM_VERTEXPARAM_NORMAL,
                             material_id | light_side);
    }
    v0 = v1;
    v1 = v2;
    x0 = x1;
    y0 = y1;
    x1 = v2->sx;
    winding = -winding;
    y1 = v2->sy;
    if (--remaining != 0) {
        goto next_triangle;
    }
    if ((Uint32)run_start < (Uint32)queue_cursor) {
        EMIT_SMOOTH_ENV_CULL(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                             material_id | light_side);
        accounted_bytes +=
            (Uint32)queue_cursor - (Uint32)run_start + PACKET_BYTES;
    }
    if (--strips_left != 0) {
        goto next_strip;
    }
    _nj_draw_polygon_ += accounted_bytes >> PACKET_SHIFT;
    _nj_calc_polygon_ += input_vertices;
}

static void model_draw_unlit_env_strips_all(NJS_MESHSET *meshset,
                                            Uint32 material_id)
{
    NJS_MESHSET *source_meshset = meshset;
    register Uint16 *mesh_cursor;
    Uint32 strips_left;
    register struct model_vertex *vertices;
    register struct model_vertex *vertex;
    register Uint32 remaining;
    Uint32 input_vertices;
    Uint32 mask;
    Uint32 vertex_command;
    Float *uv_out;
    Float *normal_xy;
    Float half;

    DRAW_NOOP();
    mesh_cursor = (Uint16 *)source_meshset->meshes;
    input_vertices = 0;
    strips_left = source_meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c6413bc over its literal pool to the strip
     * loop at 0x8c6413e4; without it, 155 of 169 words differ, the function
     * runs 32 bytes short, and 400 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        mask = STRIP_COUNT;
        remaining = *mesh_cursor++ & mask;
        input_vertices += remaining;
        remaining--;
    next_vertex:
        vertex = &vertices[*mesh_cursor++];
        prefetch_vertex_head(&vertices[*mesh_cursor]);
        prefetch_vertex_tail();
        EMIT_UNLIT_ENV(vertex, KM_VERTEXPARAM_NORMAL);
        if (--remaining != 0) {
            goto next_vertex;
        }
        vertex = &vertices[*mesh_cursor++];
        EMIT_UNLIT_ENV(vertex, KM_VERTEXPARAM_ENDOFSTRIP);
    } while (--strips_left != 0);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

/* EMIT_UNLIT_ENV with locals of its own and without vertex_command, writing
 * back vertex's first cache line through a copy of its address */
/* @hack spelling: a Float pointer walks the vertex normal's x into y through
 * vector_floats, which gives the FMOV.S @Rn+ load of x (0x8c641652 in
 * model_draw_unlit_env_strips_cull); read by member, 100 of 835 words differ in
 * model_draw_unlit_env_strips_cull and model_draw_unlit_env_strips_checked */
#define EMIT_UNLIT_ENV_CULL(vertex, packet_command)                            \
    {                                                                          \
        struct model_vertex *writeback_vertex;                                 \
        Float *uv_out;                                                         \
        Float *normal_xy;                                                      \
        Float half;                                                            \
        writeback_vertex = (vertex);                                           \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(writeback_vertex);                                      \
        PACKED_COLOR(vertex, 0) = PACK_ARGB(_nj_light_material_.diffuse);      \
        uv_out = (vertex)->payload.tex.uv;                                     \
        normal_xy = ((union vector_floats *)&(vertex)->vector)->f;             \
        half = 0.5F;                                                           \
        *uv_out++ = (*normal_xy++ * half) + half;                              \
        *uv_out = (*normal_xy * half) + half;                                  \
        writeback_line(uv_out);                                                \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_unlit_env_strips_cull(NJS_MESHSET *meshset,
                                             Uint32 material_id)
{
    NJS_MESHSET *source_meshset = meshset;
    register Uint16 *mesh_cursor;
    Uint32 strips_left;
    struct model_vertex *vertices;
    register struct model_vertex *v0;
    register struct model_vertex *v1;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 accounted_bytes;
    register Float winding;
    register Float x0;
    register Float y0;
    Float x1;
    Float y1;
    Float facing;

    DRAW_NOOP();
    mesh_cursor = (Uint16 *)source_meshset->meshes;
    strips_left = source_meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = accounted_bytes = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c64156e over its literal pool to the strip
     * loop at 0x8c641584; without it, 389 of 411 words differ, the function
     * runs 12 bytes short */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    for (;;) {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
        for (;;) {
            v2 = &vertices[*mesh_cursor++];
            PREFETCH_NEXT(vertices, mesh_cursor);
            facing =
                (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                winding;
            if ((Uint32)run_start < (Uint32)queue_cursor) {
                if (facing < 0.0F) {
                    EMIT_UNLIT_ENV_CULL(v1, KM_VERTEXPARAM_NORMAL);
                } else {
                    EMIT_UNLIT_ENV_CULL(v1, KM_VERTEXPARAM_ENDOFSTRIP);
                    accounted_bytes +=
                        (Uint32)queue_cursor - (Uint32)run_start + PACKET_BYTES;
                    run_start = queue_cursor;
                }
            } else if (facing < 0.0F) {
                EMIT_UNLIT_ENV_CULL(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_ENV_CULL(v1, KM_VERTEXPARAM_NORMAL);
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            y1 = v2->sy;
            winding = -winding;
            if (--remaining == 0) {
                break;
            }
        }
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            EMIT_UNLIT_ENV_CULL(v2, KM_VERTEXPARAM_ENDOFSTRIP);
            accounted_bytes +=
                (Uint32)queue_cursor - (Uint32)run_start + PACKET_BYTES;
        }
        if (--strips_left == 0) {
            break;
        }
    }
    _nj_draw_polygon_ += accounted_bytes >> PACKET_SHIFT;
    _nj_calc_polygon_ += input_vertices;
}

/* send vertex in the material's diffuse color, as floats */
/* @hack type: the diffuse color's address spelled as &_nj_light_material_ plus
 * the size of ambient in bytes, cast to NJS_ARGB *, then as the address of that
 * color's a; as &_nj_light_material_.diffuse, 118 of
 * model_draw_unlit_strips_all's 141 words differ and it runs 12 bytes long, and
 * 9 of model_draw_unlit_strips_cull's 325 differ */
#define EMIT_UNLIT(vertex, packet_command)                                     \
    {                                                                          \
        Uint32 vertex_command;                                                 \
        NJS_ARGB *diffuse_color;                                               \
        vertex_command = (packet_command);                                     \
        (vertex)->control.cmd = vertex_command;                                \
        writeback_line(vertex);                                                \
        diffuse_color =                                                        \
            (NJS_ARGB *)&((NJS_ARGB *)((char *)&_nj_light_material_ +          \
                                       sizeof _nj_light_material_.ambient))    \
                ->a;                                                           \
        store_vertex_color(vertex, diffuse_color);                             \
        submit_work_vertex(vertex);                                            \
    }

/* EMIT_UNLIT, storing packet_command without the local vertex_command */
/* @hack type: the same address form, which gives the float loads at
 * 0x8c641a7e and 0x8c641a84, in model_draw_unlit_strips_cull, FR13 and FR14;
 * as &_nj_light_material_.diffuse, they trade, and 31 of its 325 words
 * differ */
#define EMIT_UNLIT_DIRECT_COMMAND(vertex, packet_command)                      \
    {                                                                          \
        NJS_ARGB *diffuse_color;                                               \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        diffuse_color =                                                        \
            (NJS_ARGB *)&((NJS_ARGB *)((char *)&_nj_light_material_ +          \
                                       sizeof _nj_light_material_.ambient))    \
                ->a;                                                           \
        store_vertex_color(vertex, diffuse_color);                             \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_unlit_strips_all(NJS_MESHSET *meshset,
                                        Uint32 material_id)
{
    Uint32 input_vertices;
    Uint16 *mesh_cursor;
    Uint32 strips_left;
    struct model_vertex *vertices;
    struct model_vertex *vertex;
    Uint32 remaining;
    Uint32 mask;

    DRAW_NOOP();
    input_vertices = 0;
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c6418c4 over its literal pool to the strip
     * loop at 0x8c6418ec; without it, 124 of 141 words differ, the function
     * runs 6 bytes short, and 317 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        mask = STRIP_COUNT;
        remaining = *mesh_cursor++ & mask;
        input_vertices += remaining;
        remaining--;
        do {
            vertex = &vertices[*mesh_cursor++];
            prefetch_vertex_head(&vertices[*mesh_cursor]);
            prefetch_vertex_tail();
            EMIT_UNLIT(vertex, KM_VERTEXPARAM_NORMAL);
        } while (--remaining != 0);
        vertex = &vertices[*mesh_cursor++];
        EMIT_UNLIT(vertex, KM_VERTEXPARAM_ENDOFSTRIP);
    } while (--strips_left != 0);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

static void model_draw_unlit_strips_cull(NJS_MESHSET *meshset,
                                         Uint32 material_id)
{
    register Uint16 *mesh_cursor;
    Uint32 strips_left;
    register struct model_vertex *vertices;
    register struct model_vertex *v0;
    register struct model_vertex *v1;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 accounted_bytes;
    register Float winding;
    Float x0;
    Float y0;
    Float x1;
    Float y1;
    register Float facing;
    Uint32 mask;

    DRAW_NOOP();
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = accounted_bytes = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c641a0e over its literal pool to the strip
     * loop at 0x8c641a34; without it, 307 of 325 words differ, the function
     * runs 32 bytes short, and 7 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            mask = STRIP_COUNT;
            remaining &= mask;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
        do {
            v2 = &vertices[*mesh_cursor++];
            PREFETCH_NEXT(vertices, mesh_cursor);
            facing =
                (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                winding;
            if ((Uint32)run_start < (Uint32)queue_cursor) {
                if (facing < 0.0F) {
                    EMIT_UNLIT_DIRECT_COMMAND(v1, KM_VERTEXPARAM_NORMAL);
                } else {
                    EMIT_UNLIT_DIRECT_COMMAND(v1, KM_VERTEXPARAM_ENDOFSTRIP);
                    accounted_bytes +=
                        (Uint32)queue_cursor - (Uint32)run_start + PACKET_BYTES;
                    run_start = queue_cursor;
                }
            } else if (facing < 0.0F) {
                EMIT_UNLIT_DIRECT_COMMAND(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_DIRECT_COMMAND(v1, KM_VERTEXPARAM_NORMAL);
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            y1 = v2->sy;
            winding = -winding;
        } while (--remaining != 0);
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            EMIT_UNLIT_DIRECT_COMMAND(v2, KM_VERTEXPARAM_ENDOFSTRIP);
            accounted_bytes +=
                (Uint32)queue_cursor - (Uint32)run_start + PACKET_BYTES;
        }
    } while (--strips_left != 0);
    _nj_draw_polygon_ += accounted_bytes >> PACKET_SHIFT;
    _nj_calc_polygon_ += input_vertices;
}

/* set vertex's command and light it as EMIT_SMOOTH_POLYGON does, but leave
 * the send to the caller */
/* @hack spelling: Float pointers walk the light pair from the diffuse's a and
 * the specular's r through light_floats, and the vertex color's a, r, g and b
 * through argb_floats, which gives the FMOV.S @Rn+ loads and the ADD #4 store
 * steps of the summed color (0x8c641e98 to 0x8c641eb8 in
 * model_draw_smooth_strips_near); read and stored by member, 955 of 1001 words
 * differ in model_draw_smooth_strips_near and model_draw_smooth_strips_checked
 * and the section is 200 bytes longer */
#define PREPARE_SMOOTH(vertex, packet_command, material_id, light_side)        \
    {                                                                          \
        Uint32 light_key;                                                      \
        Float *diffuse;                                                        \
        Float *specular;                                                       \
        Float *color_out;                                                      \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        light_key = (material_id) | (light_side);                              \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(_njm_col2, &(vertex)->point,                  \
                                 &(vertex)->vector, light_side);               \
            diffuse = ((union light_floats *)_njm_col2)->f;                    \
            specular = diffuse + 5;                                            \
            color_out = ((union argb_floats *)&(vertex)->payload.col)->f;      \
            *color_out++ = *diffuse++;                                         \
            *color_out++ = *diffuse++ + *specular++;                           \
            *color_out++ = *diffuse++ + *specular++;                           \
            *color_out = *diffuse + *specular;                                 \
            writeback_line(color_out);                                         \
        }                                                                      \
    }

static void model_draw_smooth_strips_near(NJS_MESHSET *meshset,
                                          Uint32 material_id)
{
    register Uint16 *mesh_cursor;
    register struct model_vertex *vertices;
    register struct model_vertex *v2;
    register struct model_vertex *v1;
    register struct model_vertex *v0;
    register Uint32 *run_start;
    Sint32 strips_left;
    Sint32 remaining;
    Sint32 input_vertices;
    Uint32 back_side;
    Uint32 light_side;
    Uint32 needs_near_clip;
    Uint32 clip_all;
    Uint32 clip_near;
    register Float winding;
    register Float x0;
    register Float y0;
    register Float x1;
    register Float y1;
    register Float facing;

    DRAW_NOOP();
    back_side = (material_flags & NJD_FLAG_DOUBLE_SIDE) ? BACK_FACE : 0;
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c641cec over its literal pool to the strip
     * loop at 0x8c641d1c; without it, 517 of 537 words differ, the function
     * runs 4 bytes short, and 8 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
        do {
            v2 = &vertices[*mesh_cursor++];
            PREFETCH_NEXT(vertices, mesh_cursor);
            light_side = needs_near_clip = 0;
            facing = 1.0F;
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip &
                       CLIP_ALL;
            if (clip_all == 0) {
                clip_near =
                    (v0->control.clip | v1->control.clip | v2->control.clip) &
                    CLIP_NEAR;
                if (clip_near) {
                    facing = ((v2->point.x * ((v1->point.z * v0->point.y) -
                                              (v1->point.y * v0->point.z))) +
                              (v2->point.y * ((v1->point.x * v0->point.z) -
                                              (v1->point.z * v0->point.x))) +
                              (v2->point.z * ((v0->point.x * v1->point.y) -
                                              (v1->point.x * v0->point.y)))) *
                             winding;
                    if (facing > 0.0F) {
                        if (back_side) {
                            needs_near_clip = light_side = back_side;
                        }
                    } else {
                        needs_near_clip = 1;
                        facing = 1.0F;
                    }
                } else {
                    facing = (((x1 - x0) * (v2->sy - y1)) -
                              ((y1 - y0) * (v2->sx - x1))) *
                             winding;
                    if (facing > 0.0F) {
                        if (back_side) {
                            light_side = back_side;
                            facing = -1.0F;
                        }
                    }
                }
            }
            if ((Uint32)run_start < (Uint32)queue_cursor) {
                if (facing < 0.0F) {
                    PREPARE_SMOOTH(v1, KM_VERTEXPARAM_NORMAL, material_id,
                                   light_side);
                    submit_work_vertex(v1);
                } else {
                    PREPARE_SMOOTH(v1, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                                   light_side);
                    submit_work_vertex(v1);
                    run_start = queue_cursor;
                }
            } else {
                if (facing < 0.0F) {
                    PREPARE_SMOOTH(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                   light_side);
                    submit_work_vertex(v0);
                    PREPARE_SMOOTH(v1, KM_VERTEXPARAM_NORMAL, material_id,
                                   light_side);
                    submit_work_vertex(v1);
                }
            }
            if (needs_near_clip) {
                model_clip_smooth_triangle(v0, v1, v2,
                                           material_id | light_side);
                run_start = queue_cursor;
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            winding = -winding;
            y1 = v2->sy;
        } while (--remaining);
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            PREPARE_SMOOTH(v2, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                           light_side);
            submit_work_vertex(v2);
        }
    } while (--strips_left);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

/* PREPARE_SMOOTH, then send vertex */
#define EMIT_SMOOTH_CHECKED(vertex, packet_command, material_id, light_side)   \
    {                                                                          \
        PREPARE_SMOOTH(vertex, packet_command, material_id, light_side)        \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_smooth_strips_checked(NJS_MESHSET *meshset,
                                             Uint32 material_index)
{
    register Uint16 *mesh_cursor;
    register struct model_vertex *vertices;
    register struct model_vertex *v0;
    register struct model_vertex *v1;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    register Float winding;
    register Float x1;
    register Float y1;
    Float x0;
    Float y0;
    register Float facing;
    Uint32 material_id = material_index;
    Uint32 strips_left;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 back_side;
    Uint32 light_side;
    Uint32 clip_all;

    DRAW_NOOP();
    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
        back_side = BACK_FACE;
    } else {
        back_side = 0;
    }
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c64216e over its literal pool to the strip
     * loop at 0x8c6421a8; without it, 443 of 464 words differ, the function
     * runs 4 bytes short, and 15 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;

        /* @hack redundant: a never-run copy of x1 into x0, which gives x1 FR14
         * and y1 FR13, loaded at 0x8c6421ee and 0x8c6421f4; without it, they
         * trade, as x0's FR5 and y0's FR6 do, and 14 of 464 words differ */
        if (0) {
            x0 = x1;
        }
        run_start = queue_cursor;
        do {
            v2 = &vertices[*mesh_cursor++];
            PREFETCH_NEXT(vertices, mesh_cursor);
            light_side = 0;
            facing = 1.0F;
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip &
                       CLIP_ALL;
            if (!clip_all) {
                facing = (((x1 - x0) * (v2->sy - y1)) -
                          ((y1 - y0) * (v2->sx - x1))) *
                         winding;
                if (facing > 0.0F) {
                    if (back_side) {
                        light_side = back_side;
                        facing = -1.0F;
                    }
                }
            }
            if ((Uint32)run_start < (Uint32)queue_cursor) {
                if (facing < 0.0F) {
                    EMIT_SMOOTH_CHECKED(v1, KM_VERTEXPARAM_NORMAL, material_id,
                                        light_side);
                } else {
                    EMIT_SMOOTH_CHECKED(v1, KM_VERTEXPARAM_ENDOFSTRIP,
                                        material_id, light_side);
                    run_start = queue_cursor;
                }
            } else if (facing < 0.0F) {
                EMIT_SMOOTH_CHECKED(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                    light_side);
                EMIT_SMOOTH_CHECKED(v1, KM_VERTEXPARAM_NORMAL, material_id,
                                    light_side);
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            winding = -winding;
            y1 = v2->sy;
        } while (--remaining != 0);
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            EMIT_SMOOTH_CHECKED(v2, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                                light_side);
        }
    } while (--strips_left != 0);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

/* store corner_uv's texture coordinates in vertex, each Sint16 over 256, and
 * advance corner_uv */
#define STORE_CORNER_UV(vertex, corner_uv)                                     \
    {                                                                          \
        STORE_CORNER_UV_AS(vertex, corner_uv, auto, auto);                     \
    }

/* the same, through pointers of the storage classes uv_in_class and
 * uv_out_class */
/* @hack spelling: a Sint16 pointer walks the corner's tex.u into tex.v, which
 * gives the MOV.W @Rn+ load of u (0x8c642608 in
 * model_draw_smooth_tex_strips_near); read by member through the corner, 7641
 * of 9870 words differ in the ten drawers that store corner coordinates and the
 * section is 180 bytes longer */
#define STORE_CORNER_UV_AS(vertex, corner_uv, uv_in_class, uv_out_class)       \
    {                                                                          \
        uv_in_class Sint16 *uv_in;                                             \
        uv_out_class Float *uv_out;                                            \
        Float uv_scale;                                                        \
        uv_scale = 1.0F / 256.0F;                                              \
        uv_out = (vertex)->payload.tex.uv;                                     \
        uv_in = &((corner_uv)++)->tex.u;                                       \
        *uv_out++ = *uv_in++ * uv_scale;                                       \
        *uv_out = *uv_in * uv_scale;                                           \
    }

/* send vertex lit from light_side in packed colors, beside texture coordinates
 * already stored. relighting it writes back the cache line at
 * payload_address */
#define EMIT_CACHED_TEX(vertex, packet_command, payload_address, material_id,  \
                        light_side)                                            \
    {                                                                          \
        Uint32 light_key;                                                      \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        light_key = (material_id) | (light_side);                              \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(_njm_col2, &(vertex)->point,                  \
                                 &(vertex)->vector, light_side);               \
            PACKED_COLOR(vertex, 0) = PACK_ARGB(_njm_col2[0]);                 \
            PACKED_COLOR(vertex, 1) = PACK_ARGB(_njm_col2[1]);                 \
            writeback_line(payload_address);                                   \
        }                                                                      \
        submit_work_vertex(vertex);                                            \
    }

/* EMIT_CACHED_TEX for a corner that doesn't end its run */
/* @hack spelling: specular's blue read through a second pointer, which gives
 * the float loads at 0x8c6425cc and 0x8c6425e0, in
 * model_draw_smooth_tex_strips_near, FR4 and FR5; as PACK_ARGB(_njm_col2[1]),
 * they trade, and 6 of the 645 words of that function and of
 * model_draw_flat_path_tex_strips_near differ in each */
#define EMIT_CACHED_TEX_CONTINUE(vertex, packet_command, payload_address,      \
                                 material_id, light_side)                      \
    {                                                                          \
        NJS_ARGB *specular;                                                    \
        NJS_ARGB *specular_copy;                                               \
        Uint32 light_key;                                                      \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        light_key = (material_id) | (light_side);                              \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(_njm_col2, &(vertex)->point,                  \
                                 &(vertex)->vector, light_side);               \
            PACKED_COLOR(vertex, 0) = PACK_ARGB(_njm_col2[0]);                 \
            PACKED_COLOR(vertex, 1) =                                          \
                (specular_copy = (specular = &_njm_col2[1]),                   \
                 model_pack_argb(specular->a, specular->r, specular->g,        \
                                 specular_copy->b));                           \
            writeback_line(payload_address);                                   \
        }                                                                      \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_smooth_tex_strips_near(NJS_MESHSET *meshset,
                                              Uint32 material_id)
{
    register Uint16 *mesh_cursor;
    register NJS_COLOR *corner_uv;
    register struct model_vertex *v1;
    register struct model_vertex *v0;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    struct model_vertex *vertices;
    Uint32 strips_left;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 back_side;
    Uint32 light_side;
    Uint32 needs_near_clip;
    Uint32 clip_all;
    Uint32 clip_near;
    Float *v1_payload;
    register Float winding;
    register Float y1;
    register Float x1;
    register Float x0;
    register Float y0;
    register Float facing;

    DRAW_NOOP();
    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
        back_side = BACK_FACE;
    } else {
        back_side = 0;
    }
    corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c64255c over its literal pool to the strip
     * loop at 0x8c642590; without it, 348 of 645 words differ, and 18 differ
     * beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        STORE_CORNER_UV(v0, corner_uv);
        STORE_CORNER_UV(v1, corner_uv);
        run_start = queue_cursor;
    next_triangle:
        v2 = &vertices[*mesh_cursor++];
        PREFETCH_NEXT(vertices, mesh_cursor);
        light_side = needs_near_clip = 0;
        facing = 1.0F;
        clip_all =
            v0->control.clip & v1->control.clip & v2->control.clip & CLIP_ALL;
        STORE_CORNER_UV(v2, corner_uv);
        if (clip_all == 0) {
            clip_near =
                (v0->control.clip | v1->control.clip | v2->control.clip) &
                CLIP_NEAR;
            if (clip_near) {
                facing =
                    view_triangle_facing(&v1->point, &v0->point, &v2->point) *
                    winding;
                if (facing > 0.0F) {
                    if (back_side) {
                        needs_near_clip = light_side = back_side;
                    }
                } else {
                    needs_near_clip = 1;
                    facing = 1.0F;
                }
            } else {
                facing = (((x1 - x0) * (v2->sy - y1)) -
                          ((y1 - y0) * (v2->sx - x1))) *
                         winding;
                if (facing > 0.0F) {
                    if (back_side) {
                        light_side = back_side;
                        facing = -1.0F;
                    }
                }
            }
        }
        v1_payload = &v1->payload.tex.uv[0];
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            if (facing < 0.0F) {
                EMIT_CACHED_TEX_CONTINUE(v1, KM_VERTEXPARAM_NORMAL, v1_payload,
                                         material_id, light_side);
            } else {
                EMIT_CACHED_TEX(v1, KM_VERTEXPARAM_ENDOFSTRIP, v1_payload,
                                material_id, light_side);
                run_start = queue_cursor;
            }
        } else if (facing < 0.0F) {
            EMIT_CACHED_TEX_CONTINUE(v0, KM_VERTEXPARAM_NORMAL,
                                     &v0->payload.tex.uv[0], material_id,
                                     light_side);
            EMIT_CACHED_TEX_CONTINUE(v1, KM_VERTEXPARAM_NORMAL, v1_payload,
                                     material_id, light_side);
        }
        if (needs_near_clip) {
            model_clip_smooth_tex_triangle(v0, v1, v2,
                                           material_id | light_side);
            run_start = queue_cursor;
        }
        v0 = v1;
        v1 = v2;
        x0 = x1;
        y0 = y1;
        x1 = v2->sx;
        y1 = v2->sy;
        winding = -winding;

        /* @hack redundant: a never-run store to y1, which gives x1 FR13 and y1
         * FR14, loaded at 0x8c6425dc and 0x8c6425e2; without it, they trade, as
         * R4 and R5 do, and 22 of 645 words differ */
        if (0) {
            y1 = 0.0F;
        }
        if (--remaining) {
            goto next_triangle;
        }
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            EMIT_CACHED_TEX(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                            &v2->payload.tex.uv[0], material_id, light_side);
        }
    } while (--strips_left);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

/* send vertex lit from light_side in packed colors, with corner_uv's texture
 * coordinates as STORE_CORNER_UV_AS stores them, and write back the cache line
 * at payload_address */
#define EMIT_CHECKED_TEX(vertex, packet_command, payload_address, material_id, \
                         light_side, corner_uv, uv_in_class, uv_out_class)     \
    {                                                                          \
        Uint32 light_key;                                                      \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        light_key = (material_id) | (light_side);                              \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(_njm_col2, &(vertex)->point,                  \
                                 &(vertex)->vector, light_side);               \
            PACKED_COLOR(vertex, 0) = PACK_ARGB(_njm_col2[0]);                 \
            PACKED_COLOR(vertex, 1) = PACK_ARGB(_njm_col2[1]);                 \
        }                                                                      \
        STORE_CORNER_UV_AS(vertex, corner_uv, uv_in_class, uv_out_class);      \
        writeback_line(payload_address);                                       \
        submit_work_vertex(vertex);                                            \
    }

/* EMIT_CHECKED_TEX for a run's first corner */
/* @hack spelling: specular's blue read through a second pointer, which gives
 * the float loads at 0x8c642b4c and 0x8c642b52, in
 * model_draw_smooth_tex_strips_checked, FR13 and FR14; as
 * PACK_ARGB(_njm_col2[1]), they trade, and 10 of the 605 words of that
 * function and of model_draw_flat_path_tex_strips_checked differ in each */
#define EMIT_CHECKED_TEX_OPEN(vertex, packet_command, payload_address,         \
                              material_id, light_side, corner_uv, uv_in_class, \
                              uv_out_class)                                    \
    {                                                                          \
        NJS_ARGB *specular;                                                    \
        NJS_ARGB *specular_copy;                                               \
        Uint32 light_key;                                                      \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        light_key = (material_id) | (light_side);                              \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(_njm_col2, &(vertex)->point,                  \
                                 &(vertex)->vector, light_side);               \
            PACKED_COLOR(vertex, 0) = PACK_ARGB(_njm_col2[0]);                 \
            PACKED_COLOR(vertex, 1) =                                          \
                (specular_copy = (specular = &_njm_col2[1]),                   \
                 model_pack_argb(specular->a, specular->r, specular->g,        \
                                 specular_copy->b));                           \
        }                                                                      \
        STORE_CORNER_UV_AS(vertex, corner_uv, uv_in_class, uv_out_class);      \
        writeback_line(payload_address);                                       \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_smooth_tex_strips_checked(NJS_MESHSET *meshset,
                                                 Uint32 material_id)
{
    register Uint16 *mesh_cursor;
    register NJS_COLOR *corner_uv;
    register struct model_vertex *v1;
    register struct model_vertex *v0;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    struct model_vertex *vertices;
    Uint32 strips_left;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 back_side;
    Uint32 light_side;
    Uint32 clip_all;
    Float *v1_payload;
    register Float winding;
    register Float y1;
    register Float x1;
    Float x0;
    Float y0;
    register Float facing;

    DRAW_NOOP();
    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
        back_side = BACK_FACE;
    } else {
        back_side = 0;
    }
    corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c642ac4 over its literal pool to the strip
     * loop at 0x8c642b04; without it, 585 of 605 words differ, the function
     * runs 4 bytes short, and 23 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
    next_triangle:
        v2 = &vertices[*mesh_cursor++];
        PREFETCH_NEXT(vertices, mesh_cursor);
        light_side = 0;
        facing = 1.0F;
        clip_all =
            v0->control.clip & v1->control.clip & v2->control.clip & CLIP_ALL;
        if (clip_all == 0) {
            facing =
                (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                winding;
            if (facing > 0.0F) {
                if (back_side) {
                    light_side = back_side;
                    facing = -1.0F;
                }
            }
        }
        v1_payload = &v1->payload.tex.uv[0];
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            if (facing < 0.0F) {
                EMIT_CHECKED_TEX(v1, KM_VERTEXPARAM_NORMAL, v1_payload,
                                 material_id, light_side, corner_uv, register,
                                 auto);
            } else {
                EMIT_CHECKED_TEX(v1, KM_VERTEXPARAM_ENDOFSTRIP, v1_payload,
                                 material_id, light_side, corner_uv, auto,
                                 register);
                corner_uv--;
                run_start = queue_cursor;
            }
        } else {
            corner_uv++;
            if (facing < 0.0F) {
                corner_uv--;
                EMIT_CHECKED_TEX_OPEN(v0, KM_VERTEXPARAM_NORMAL,
                                      &v0->payload.tex.uv[0], material_id,
                                      light_side, corner_uv, register, auto);
                EMIT_CHECKED_TEX(v1, KM_VERTEXPARAM_NORMAL, v1_payload,
                                 material_id, light_side, corner_uv, auto,
                                 register);
            }
        }
        v0 = v1;
        v1 = v2;
        x0 = x1;
        y0 = y1;
        x1 = v2->sx;
        y1 = v2->sy;
        winding = -winding;
        if (--remaining) {
            goto next_triangle;
        }
        corner_uv += 2;
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            corner_uv -= 2;
            EMIT_CHECKED_TEX(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                             &v2->payload.tex.uv[0], material_id, light_side,
                             corner_uv, auto, auto);
        }
    } while (--strips_left);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

/* send vertex lit from light_side in packed colors, with environment
 * coordinates */
/* @hack spelling: a Float pointer walks the vertex normal's x into y through
 * vector_floats, which gives the FMOV.S @Rn+ load of x (0x8c6431ec in
 * model_draw_smooth_env_strips_near); read by member, 2465 of 3894 words differ
 * in model_draw_smooth_env_strips_near, model_draw_flat_path_env_strips_near,
 * model_draw_smooth_env_polygons and model_draw_smooth_env_polygons_clip and
 * the section is 28 bytes longer */
#define EMIT_SMOOTH_ENV(vertex, packet_command, material_id, light_side)       \
    {                                                                          \
        Uint32 light_key;                                                      \
        Float *uv_out;                                                         \
        Float *normal_xy;                                                      \
        Float half;                                                            \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        light_key = (material_id) | (light_side);                              \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(_njm_col2, &(vertex)->point,                  \
                                 &(vertex)->vector, light_side);               \
            PACKED_COLOR(vertex, 0) = PACK_ARGB(_njm_col2[0]);                 \
            PACKED_COLOR(vertex, 1) = PACK_ARGB(_njm_col2[1]);                 \
        }                                                                      \
        half = 0.5F;                                                           \
        uv_out = (vertex)->payload.tex.uv;                                     \
        normal_xy = ((union vector_floats *)&(vertex)->vector)->f;             \
        *uv_out++ = (*normal_xy++ * half) + half;                              \
        *uv_out = (*normal_xy * half) + half;                                  \
        writeback_line(uv_out);                                                \
        submit_work_vertex(vertex);                                            \
    }

/* EMIT_SMOOTH_ENV for a run's first corner, setting uv_out and normal_xy
 * before half */
/* @hack spelling: a Float pointer walks the vertex normal's x into y through
 * vector_floats, which gives the FMOV.S @Rn+ load of x; read by member, 402 of
 * 1166 words differ in model_draw_smooth_env_strips_checked and
 * model_draw_flat_path_env_strips_checked */
#define EMIT_SMOOTH_ENV_OPEN(vertex, packet_command, material_id, light_side)  \
    {                                                                          \
        Uint32 light_key;                                                      \
        Float *normal_xy;                                                      \
        Float *uv_out;                                                         \
        Float half;                                                            \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        light_key = (material_id) | (light_side);                              \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(_njm_col2, &(vertex)->point,                  \
                                 &(vertex)->vector, light_side);               \
            PACKED_COLOR(vertex, 0) = PACK_ARGB(_njm_col2[0]);                 \
            PACKED_COLOR(vertex, 1) = PACK_ARGB(_njm_col2[1]);                 \
        }                                                                      \
        uv_out = (vertex)->payload.tex.uv;                                     \
        normal_xy = ((union vector_floats *)&(vertex)->vector)->f;             \
        half = 0.5F;                                                           \
        *uv_out++ = (*normal_xy++ * half) + half;                              \
        *uv_out = (*normal_xy * half) + half;                                  \
        writeback_line(uv_out);                                                \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_smooth_env_strips_near(NJS_MESHSET *meshset,
                                              Uint32 material_id)
{
    register Uint16 *mesh_cursor;
    register struct model_vertex *v2;
    register struct model_vertex *v1;
    register struct model_vertex *v0;
    register Uint32 *run_start;
    struct model_vertex *vertices;
    /* @hack redundant: unused_corner_uv, set to meshset->vertuv and never
     * read, which as meshset's third use gives it R13, MOV R4,R13 at
     * 0x8c642fae; without it, 646 of 662 words differ, the function runs 4
     * bytes long, and 24 of model_draw_smooth_tex_strips_checked's 605
     * differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    NJS_COLOR *unused_corner_uv;
    Uint32 strips_left;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 back_side;
    Uint32 light_side;
    Uint32 needs_near_clip;
    Uint32 clip_all;
    Uint32 clip_near;
    register Float winding;
    register Float x1;
    register Float y1;
    register Float x0;
    register Float y0;
    register Float facing;

    DRAW_NOOP();
    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
        back_side = BACK_FACE;
    } else {
        back_side = 0;
    }

    unused_corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c642ff2 over its literal pool to the strip
     * loop at 0x8c643034; without it, 643 of 662 words differ, the function
     * runs 8 bytes short, and 24 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
    next_triangle:
        v2 = &vertices[*mesh_cursor++];
        PREFETCH_NEXT(vertices, mesh_cursor);
        facing = 1.0F;
        light_side = needs_near_clip = 0;
        clip_all =
            v0->control.clip & v1->control.clip & v2->control.clip & CLIP_ALL;
        if (clip_all == 0) {
            clip_near =
                (v0->control.clip | v1->control.clip | v2->control.clip) &
                CLIP_NEAR;
            if (clip_near) {
                facing =
                    view_triangle_facing(&v1->point, &v0->point, &v2->point) *
                    winding;
                if (facing > 0.0F) {
                    if (back_side) {
                        needs_near_clip = light_side = back_side;
                    }
                } else {
                    needs_near_clip = 1;
                    facing = 1.0F;
                }
            } else {
                facing = (((x1 - x0) * (v2->sy - y1)) -
                          ((y1 - y0) * (v2->sx - x1))) *
                         winding;
                if (facing > 0.0F) {
                    if (back_side) {
                        light_side = back_side;
                        facing = -1.0F;
                    }
                }
            }
        }
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            if (facing < 0.0F) {
                EMIT_SMOOTH_ENV(v1, KM_VERTEXPARAM_NORMAL, material_id,
                                light_side);
            } else {
                EMIT_SMOOTH_ENV(v1, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                                light_side);
                run_start = queue_cursor;
            }
        } else if (facing < 0.0F) {
            EMIT_SMOOTH_ENV(v0, KM_VERTEXPARAM_NORMAL, material_id, light_side);
            EMIT_SMOOTH_ENV(v1, KM_VERTEXPARAM_NORMAL, material_id, light_side);
        }
        if (needs_near_clip) {
            model_clip_smooth_env_triangle(v0, v1, v2,
                                           material_id | light_side);
            run_start = queue_cursor;
        }
        v0 = v1;
        v1 = v2;
        x0 = x1;
        y0 = y1;
        x1 = v2->sx;
        y1 = v2->sy;
        winding = -winding;
        if (--remaining) {
            goto next_triangle;
        }
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            EMIT_SMOOTH_ENV(v2, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                            light_side);
        }
    } while (--strips_left);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

/* EMIT_SMOOTH_ENV, setting uv_out and normal_xy before half, and packing the
 * colors through light_color */
/* @hack spelling: a Float pointer walks the vertex normal's x into y through
 * vector_floats, which gives the FMOV.S @Rn+ load of x; read by member, 432 of
 * 1166 words differ in model_draw_smooth_env_strips_checked and
 * model_draw_flat_path_env_strips_checked */
#define EMIT_SMOOTH_ENV_COLOR_WALK(vertex, packet_command, material_id,        \
                                   light_side)                                 \
    {                                                                          \
        NJS_ARGB *light_color;                                                 \
        Uint32 light_key;                                                      \
        Float *normal_xy;                                                      \
        Float *uv_out;                                                         \
        Float half;                                                            \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        light_key = (material_id) | (light_side);                              \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(_njm_col2, &(vertex)->point,                  \
                                 &(vertex)->vector, light_side);               \
            PACKED_COLOR(vertex, 0) =                                          \
                (light_color = _njm_col2, PACK_ARGB(*light_color));            \
            PACKED_COLOR(vertex, 1) =                                          \
                (++light_color, PACK_ARGB(*light_color));                      \
        }                                                                      \
        uv_out = (vertex)->payload.tex.uv;                                     \
        normal_xy = ((union vector_floats *)&(vertex)->vector)->f;             \
        half = 0.5F;                                                           \
        *uv_out++ = (*normal_xy++ * half) + half;                              \
        *uv_out = (*normal_xy * half) + half;                                  \
        writeback_line(uv_out);                                                \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_smooth_env_strips_checked(NJS_MESHSET *meshset,
                                                 Uint32 material_id)
{
    register Uint16 *mesh_cursor;
    register struct model_vertex *v2;
    register struct model_vertex *v1;
    register struct model_vertex *v0;
    register Uint32 *run_start;
    struct model_vertex *vertices;
    Uint32 strips_left;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 back_side;
    Uint32 light_side;
    Uint32 clip_all;
    /* @hack redundant: unused_mesh_type, set to meshset->type_matId and never
     * read, which keeps meshset in R13, MOV R4,R13 at 0x8c64353c; without it,
     * 563 of 583 words differ, and the function runs 4 bytes long */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Uint32 unused_mesh_type;
    register Float winding;
    register Float y1;
    register Float x1;
    register Float facing;
    Float y0;
    Float x0;

    DRAW_NOOP();

    unused_mesh_type = meshset->type_matId;
    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
        back_side = BACK_FACE;
    } else {
        back_side = 0;
    }
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c6435be over its literal pool to the strip
     * loop at 0x8c6435cc; without it, 546 of 583 words differ, the function
     * runs 12 bytes short */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
    next_triangle:
        v2 = &vertices[*mesh_cursor++];
        PREFETCH_NEXT(vertices, mesh_cursor);
        facing = 1.0F;
        light_side = 0;
        clip_all =
            v0->control.clip & v1->control.clip & v2->control.clip & CLIP_ALL;
        if (clip_all == 0) {
            facing =
                (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                winding;
            if (facing > 0.0F) {
                if (back_side) {
                    light_side = back_side;
                    facing = -1.0F;
                }
            }
        }
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            if (facing < 0.0F) {
                EMIT_SMOOTH_ENV_COLOR_WALK(v1, KM_VERTEXPARAM_NORMAL,
                                           material_id, light_side);
            } else {
                EMIT_SMOOTH_ENV_COLOR_WALK(v1, KM_VERTEXPARAM_ENDOFSTRIP,
                                           material_id, light_side);
                run_start = queue_cursor;
            }
        } else if (facing < 0.0F) {
            EMIT_SMOOTH_ENV_OPEN(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                 light_side);
            EMIT_SMOOTH_ENV_COLOR_WALK(v1, KM_VERTEXPARAM_NORMAL, material_id,
                                       light_side);
        }
        v0 = v1;
        v1 = v2;
        x0 = x1;
        y0 = y1;
        x1 = v2->sx;
        y1 = v2->sy;
        winding = -winding;
        if (--remaining) {
            goto next_triangle;
        }
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            EMIT_SMOOTH_ENV_COLOR_WALK(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                                       material_id, light_side);
        }
    } while (--strips_left);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

/* PREPARE_SMOOTH, taking vertex's color address before _njm_col2's */
/* @hack spelling: Float pointers walk the light pair from the diffuse's a and
 * the specular's r through light_floats, and the vertex color's a, r, g and b
 * through argb_floats, which gives the FMOV.S @Rn+ loads and the ADD #4 store
 * steps of the summed color (0x8c643c2c to 0x8c643c50 in
 * model_draw_flat_path_strips_near); read and stored by member, 949 of 1000
 * words differ in model_draw_flat_path_strips_near and
 * model_draw_flat_path_strips_checked and the section is 196 bytes longer */
#define PREPARE_SMOOTH_DST_FIRST(vertex, packet_command, material_id,          \
                                 light_side)                                   \
    {                                                                          \
        Uint32 light_key;                                                      \
        Float *diffuse;                                                        \
        Float *specular;                                                       \
        Float *color_out;                                                      \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        light_key = (material_id) | (light_side);                              \
        if ((vertex)->lit_key != light_key) {                                  \
            (vertex)->lit_key = light_key;                                     \
            njCalculateLightsRGB(_njm_col2, &(vertex)->point,                  \
                                 &(vertex)->vector, light_side);               \
            color_out = ((union argb_floats *)&(vertex)->payload.col)->f;      \
            diffuse = ((union light_floats *)_njm_col2)->f;                    \
            specular = diffuse + 5;                                            \
            *color_out++ = *diffuse++;                                         \
            *color_out++ = *diffuse++ + *specular++;                           \
            *color_out++ = *diffuse++ + *specular++;                           \
            *color_out = *diffuse + *specular;                                 \
            writeback_line(color_out);                                         \
        }                                                                      \
    }

static void model_draw_flat_path_strips_near(NJS_MESHSET *meshset,
                                             Uint32 material_index)
{
    NJS_MESHSET *source_meshset = meshset;
    Uint32 material_id = material_index;
    register Uint16 *mesh_cursor;
    struct model_vertex *vertices;
    register struct model_vertex *v2;
    register struct model_vertex *v1;
    register struct model_vertex *v0;
    register Uint32 *run_start;
    Sint32 strips_left;
    Sint32 remaining;
    Sint32 input_vertices;
    Uint32 back_side;
    Uint32 light_side;
    Uint32 needs_near_clip;
    Uint32 clip_all;
    Uint32 clip_near;
    register Float winding;
    register Float x0;
    register Float y0;
    register Float x1;
    register Float y1;
    register Float facing;

    DRAW_NOOP();
    back_side = (material_flags & NJD_FLAG_DOUBLE_SIDE) ? BACK_FACE : 0;
    mesh_cursor = (Uint16 *)source_meshset->meshes;
    strips_left = source_meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c643a7e over its literal pool to the strip
     * loop at 0x8c643abc; without it, 505 of 534 words differ, the function
     * runs 8 bytes short, and 17 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
        do {
            v2 = &vertices[*mesh_cursor++];
            PREFETCH_NEXT(vertices, mesh_cursor);
            light_side = needs_near_clip = 0;
            facing = 1.0F;
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip &
                       CLIP_ALL;
            if (clip_all == 0) {
                clip_near =
                    (v0->control.clip | v1->control.clip | v2->control.clip) &
                    CLIP_NEAR;
                if (clip_near) {
                    facing = view_triangle_facing(&v1->point, &v0->point,
                                                  &v2->point) *
                             winding;
                    if (facing > 0.0F) {
                        if (back_side) {
                            needs_near_clip = light_side = back_side;
                        }
                    } else {
                        needs_near_clip = 1;
                        facing = 1.0F;
                    }
                } else {
                    facing = (((x1 - x0) * (v2->sy - y1)) -
                              ((y1 - y0) * (v2->sx - x1))) *
                             winding;
                    if (facing > 0.0F) {
                        if (back_side) {
                            light_side = back_side;
                            facing = -1.0F;
                        }
                    }
                }
            }
            if ((Uint32)run_start < (Uint32)queue_cursor) {
                if (facing < 0.0F) {
                    PREPARE_SMOOTH_DST_FIRST(v1, KM_VERTEXPARAM_NORMAL,
                                             material_id, light_side);
                    submit_work_vertex(v1);
                } else {
                    PREPARE_SMOOTH_DST_FIRST(v1, KM_VERTEXPARAM_ENDOFSTRIP,
                                             material_id, light_side);
                    submit_work_vertex(v1);
                    run_start = queue_cursor;
                }
            } else {
                if (facing < 0.0F) {
                    PREPARE_SMOOTH_DST_FIRST(v0, KM_VERTEXPARAM_NORMAL,
                                             material_id, light_side);
                    submit_work_vertex(v0);
                    PREPARE_SMOOTH_DST_FIRST(v1, KM_VERTEXPARAM_NORMAL,
                                             material_id, light_side);
                    submit_work_vertex(v1);
                }
            }
            if (needs_near_clip) {
                model_clip_smooth_triangle(v0, v1, v2,
                                           material_id | light_side);
                run_start = queue_cursor;
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            winding = -winding;
            y1 = v2->sy;
        } while (--remaining);
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            PREPARE_SMOOTH_DST_FIRST(v2, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                                     light_side);
            submit_work_vertex(v2);
        }
    } while (--strips_left);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

/* PREPARE_SMOOTH_DST_FIRST, then send vertex */
#define EMIT_SMOOTH_CHECKED_DST_FIRST(vertex, packet_command, material_id,     \
                                      light_side)                              \
    {                                                                          \
        PREPARE_SMOOTH_DST_FIRST(vertex, packet_command, material_id,          \
                                 light_side)                                   \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_flat_path_strips_checked(NJS_MESHSET *meshset,
                                                Uint32 material_index)
{
    register Uint16 *mesh_cursor;
    struct model_vertex *vertices;
    register struct model_vertex *v0;
    register struct model_vertex *v1;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    register Float winding;
    register Float x1;
    register Float y1;
    Float x0;
    register Float y0;
    register Float facing;
    NJS_MESHSET *source_meshset = meshset;
    Uint32 material_id = material_index;
    Uint32 strips_left;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 back_side;
    Uint32 light_side;
    register Uint32 clip_all;

    DRAW_NOOP();
    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
        back_side = BACK_FACE;
    } else {
        back_side = 0;
    }
    mesh_cursor = (Uint16 *)source_meshset->meshes;
    strips_left = source_meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c643f02 over its literal pool to the strip
     * loop at 0x8c643f3c; without it, 438 of 466 words differ, the function
     * runs 8 bytes short, and 9 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
        do {
            v2 = &vertices[*mesh_cursor++];
            PREFETCH_NEXT(vertices, mesh_cursor);
            light_side = 0;
            facing = 1.0F;
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip &
                       CLIP_ALL;
            if (!clip_all) {
                facing = (((x1 - x0) * (v2->sy - y1)) -
                          ((y1 - y0) * (v2->sx - x1))) *
                         winding;
                if (facing > 0.0F) {
                    if (back_side) {
                        light_side = back_side;
                        facing = -1.0F;
                    }
                }
            }
            if ((Uint32)run_start < (Uint32)queue_cursor) {
                if (facing < 0.0F) {
                    EMIT_SMOOTH_CHECKED_DST_FIRST(v1, KM_VERTEXPARAM_NORMAL,
                                                  material_id, light_side);
                } else {
                    EMIT_SMOOTH_CHECKED_DST_FIRST(v1, KM_VERTEXPARAM_ENDOFSTRIP,
                                                  material_id, light_side);
                    run_start = queue_cursor;
                }
            } else if (facing < 0.0F) {
                EMIT_SMOOTH_CHECKED_DST_FIRST(v0, KM_VERTEXPARAM_NORMAL,
                                              material_id, light_side);
                EMIT_SMOOTH_CHECKED_DST_FIRST(v1, KM_VERTEXPARAM_NORMAL,
                                              material_id, light_side);
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            winding = -winding;
            y1 = v2->sy;
        } while (--remaining != 0);
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            EMIT_SMOOTH_CHECKED_DST_FIRST(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                                          material_id, light_side);
        }
    } while (--strips_left != 0);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

static void model_draw_flat_path_tex_strips_near(NJS_MESHSET *meshset,
                                                 Uint32 material_id)
{
    register Uint16 *mesh_cursor;
    register NJS_COLOR *corner_uv;
    register struct model_vertex *v1;
    register struct model_vertex *v0;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    struct model_vertex *vertices;
    Uint32 strips_left;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 back_side;
    Uint32 light_side;
    Uint32 needs_near_clip;
    Uint32 clip_all;
    Uint32 clip_near;
    Float *v1_payload;
    register Float winding;
    register Float y1;
    register Float x1;
    register Float x0;
    register Float y0;
    register Float facing;

    DRAW_NOOP();
    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
        back_side = BACK_FACE;
    } else {
        back_side = 0;
    }
    corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c6442f0 over its literal pool to the strip
     * loop at 0x8c644320; without it, 348 of 645 words differ, and 10 differ
     * beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        STORE_CORNER_UV(v0, corner_uv);
        STORE_CORNER_UV(v1, corner_uv);
        run_start = queue_cursor;
    next_triangle:
        v2 = &vertices[*mesh_cursor++];
        PREFETCH_NEXT(vertices, mesh_cursor);
        light_side = needs_near_clip = 0;
        facing = 1.0F;
        clip_all =
            v0->control.clip & v1->control.clip & v2->control.clip & CLIP_ALL;
        STORE_CORNER_UV(v2, corner_uv);
        if (clip_all == 0) {
            clip_near =
                (v0->control.clip | v1->control.clip | v2->control.clip) &
                CLIP_NEAR;
            if (clip_near) {
                facing =
                    view_triangle_facing(&v1->point, &v0->point, &v2->point) *
                    winding;
                if (facing > 0.0F) {
                    if (back_side) {
                        needs_near_clip = light_side = back_side;
                    }
                } else {
                    needs_near_clip = 1;
                    facing = 1.0F;
                }
            } else {
                facing = (((x1 - x0) * (v2->sy - y1)) -
                          ((y1 - y0) * (v2->sx - x1))) *
                         winding;
                if (facing > 0.0F) {
                    if (back_side) {
                        light_side = back_side;
                        facing = -1.0F;
                    }
                }
            }
        }
        v1_payload = &v1->payload.tex.uv[0];
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            if (facing < 0.0F) {
                EMIT_CACHED_TEX_CONTINUE(v1, KM_VERTEXPARAM_NORMAL, v1_payload,
                                         material_id, light_side);
            } else {
                EMIT_CACHED_TEX(v1, KM_VERTEXPARAM_ENDOFSTRIP, v1_payload,
                                material_id, light_side);
                run_start = queue_cursor;
            }
        } else if (facing < 0.0F) {
            EMIT_CACHED_TEX_CONTINUE(v0, KM_VERTEXPARAM_NORMAL,
                                     &v0->payload.tex.uv[0], material_id,
                                     light_side);
            EMIT_CACHED_TEX_CONTINUE(v1, KM_VERTEXPARAM_NORMAL, v1_payload,
                                     material_id, light_side);
        }
        if (needs_near_clip) {
            model_clip_smooth_tex_triangle(v0, v1, v2,
                                           material_id | light_side);
            run_start = queue_cursor;
        }
        v0 = v1;
        v1 = v2;
        x0 = x1;
        y0 = y1;
        x1 = v2->sx;
        y1 = v2->sy;
        winding = -winding;

        /* @hack redundant: a never-run store to y1, which gives x1 FR13 and y1
         * FR14, loaded at 0x8c64436c and 0x8c644372; without it, they trade, as
         * R4 and R5 do, and 22 of 645 words differ */
        if (0) {
            y1 = 0.0F;
        }
        if (--remaining) {
            goto next_triangle;
        }
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            EMIT_CACHED_TEX(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                            &v2->payload.tex.uv[0], material_id, light_side);
        }
    } while (--strips_left);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

static void model_draw_flat_path_tex_strips_checked(NJS_MESHSET *meshset,
                                                    Uint32 material_id)
{
    register Uint16 *mesh_cursor;
    register NJS_COLOR *corner_uv;
    register struct model_vertex *v1;
    register struct model_vertex *v0;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    struct model_vertex *vertices;
    Uint32 strips_left;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 back_side;
    Uint32 light_side;
    Uint32 clip_all;
    Float *v1_payload;
    register Float winding;
    register Float y1;
    register Float x1;
    Float x0;
    Float y0;
    register Float facing;

    DRAW_NOOP();
    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
        back_side = BACK_FACE;
    } else {
        back_side = 0;
    }
    corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c644854 over its literal pool to the strip
     * loop at 0x8c644894; without it, 585 of 605 words differ, the function
     * runs 4 bytes short, and 23 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
    next_triangle:
        v2 = &vertices[*mesh_cursor++];
        PREFETCH_NEXT(vertices, mesh_cursor);
        light_side = 0;
        facing = 1.0F;
        clip_all =
            v0->control.clip & v1->control.clip & v2->control.clip & CLIP_ALL;
        if (clip_all == 0) {
            facing =
                (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                winding;
            if (facing > 0.0F) {
                if (back_side) {
                    light_side = back_side;
                    facing = -1.0F;
                }
            }
        }
        v1_payload = &v1->payload.tex.uv[0];
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            if (facing < 0.0F) {
                EMIT_CHECKED_TEX(v1, KM_VERTEXPARAM_NORMAL, v1_payload,
                                 material_id, light_side, corner_uv, register,
                                 auto);
            } else {
                EMIT_CHECKED_TEX(v1, KM_VERTEXPARAM_ENDOFSTRIP, v1_payload,
                                 material_id, light_side, corner_uv, auto,
                                 register);
                corner_uv--;
                run_start = queue_cursor;
            }
        } else {
            corner_uv++;
            if (facing < 0.0F) {
                corner_uv--;
                EMIT_CHECKED_TEX_OPEN(v0, KM_VERTEXPARAM_NORMAL,
                                      &v0->payload.tex.uv[0], material_id,
                                      light_side, corner_uv, register, auto);
                EMIT_CHECKED_TEX(v1, KM_VERTEXPARAM_NORMAL, v1_payload,
                                 material_id, light_side, corner_uv, auto,
                                 register);
            }
        }
        v0 = v1;
        v1 = v2;
        x0 = x1;
        y0 = y1;
        x1 = v2->sx;
        y1 = v2->sy;
        winding = -winding;
        if (--remaining) {
            goto next_triangle;
        }
        corner_uv += 2;
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            corner_uv -= 2;
            EMIT_CHECKED_TEX(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                             &v2->payload.tex.uv[0], material_id, light_side,
                             corner_uv, auto, auto);
        }
    } while (--strips_left);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

/* copy *src into *dst, then write back the cache line of dst's last
 * channel */
#ifdef GCC
ALWAYS_INLINE void copy_color_writeback(NJS_ARGB *dst, NJS_ARGB *src)
{
    *dst = *src;
    writeback_line(&dst->b);
}
#else
/* clang-format off */
#ifndef __clang__
/* @asm open: its OCBWB lies only in the C functions it expands into */
#pragma inline_asm(copy_color_writeback(size=26))
static void copy_color_writeback(NJS_ARGB *dst, NJS_ARGB *src)
{
        FMOV.S      @R5+,FR3
        FMOV.S      FR3,@R4
        FMOV.S      @R5+,FR3
        ADD         #4,R4
        FMOV.S      FR3,@R4
        ADD         #4,R4
        FMOV.S      @R5+,FR3
        FMOV.S      FR3,@R4
        FMOV.S      @R5,FR2
        ADD         #4,R4
        FMOV.S      FR2,@R4
        OCBWB       @R4
}
#endif
/* clang-format on */
#endif

/* send vertex in the material's diffuse color, as floats, writing back its
 * first cache line through a copy of its address */
/* @hack redundant: the send under if (1), with an empty else, which gives each
 * command's pool load R2, as at 0x8c646be8 in model_draw_unlit_strips_checked;
 * as a block, R3, and 8 of its 345 words differ */
#define EMIT_UNLIT_COPY_ALIASED(vertex, packet_command)                        \
    if (1) {                                                                   \
        struct model_vertex *writeback_vertex;                                 \
        writeback_vertex = (vertex);                                           \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(writeback_vertex);                                      \
        {                                                                      \
            NJS_ARGB *color_out;                                               \
            color_out = &(vertex)->payload.col;                                \
            copy_color_writeback(color_out, &_nj_light_material_.diffuse);     \
        }                                                                      \
        submit_work_vertex(vertex);                                            \
    } else {                                                                   \
    }

/* EMIT_UNLIT_COPY_ALIASED, writing back through vertex itself */
/* @hack redundant: the same if (1) and empty else, R2 at 0x8c6468f4 in
 * model_draw_unlit_strips_near; as a block, R3, and 8 of its 407 words
 * differ */
#define EMIT_UNLIT_COPY(vertex, packet_command)                                \
    if (1) {                                                                   \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        {                                                                      \
            NJS_ARGB *color_out;                                               \
            color_out = &(vertex)->payload.col;                                \
            copy_color_writeback(color_out, &_nj_light_material_.diffuse);     \
        }                                                                      \
        submit_work_vertex(vertex);                                            \
    } else {                                                                   \
    }

/* send vertex in the material's diffuse color, packed, beside texture
 * coordinates already stored, and write back the cache line at payload_address.
 * scratch: writeback_vertex */
#define EMIT_UNLIT_TEX_STORED(vertex, packet_command, payload_address)         \
    {                                                                          \
        writeback_vertex = (vertex);                                           \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(writeback_vertex);                                      \
        PACKED_COLOR(vertex, 0) = PACK_ARGB(_nj_light_material_.diffuse);      \
        writeback_line(payload_address);                                       \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_unlit_tex_strips_near(NJS_MESHSET *meshset,
                                             Uint32 material_id)
{
    register Uint16 *mesh_cursor;
    register NJS_COLOR *corner_uv;
    register struct model_vertex *v1;
    register struct model_vertex *v0;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    struct model_vertex *vertices;
    Uint32 strips_left;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 back_side;
    Uint32 needs_near_clip;
    Uint32 clip_all;
    Uint32 clip_near;
    Float *v1_payload;
    struct model_vertex *writeback_vertex;
    register Float winding;
    register Float x1;
    register Float y1;
    register Float x0;
    register Float y0;
    register Float facing;

    DRAW_NOOP();
    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
        back_side = BACK_FACE;
    } else {
        back_side = 0;
    }
    corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c644d80 over its literal pool to the strip
     * loop at 0x8c644dbc; without it, 423 of 443 words differ, the function
     * runs 4 bytes short, and 24 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        STORE_CORNER_UV_AS(v0, corner_uv, register, auto);
        STORE_CORNER_UV_AS(v1, corner_uv, auto, register);
        run_start = queue_cursor;
        do {
            v2 = &vertices[*mesh_cursor++];
            PREFETCH_NEXT(vertices, mesh_cursor);
            needs_near_clip = 0;
            facing = 1.0F;
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip &
                       CLIP_ALL;
            STORE_CORNER_UV_AS(v2, corner_uv, register, auto);
            if (clip_all == 0) {
                clip_near =
                    (v0->control.clip | v1->control.clip | v2->control.clip) &
                    CLIP_NEAR;
                if (clip_near) {
                    facing = view_triangle_facing(&v1->point, &v0->point,
                                                  &v2->point) *
                             winding;
                    if (facing > 0.0F) {
                        needs_near_clip = back_side;
                    } else {
                        needs_near_clip = 1;
                        facing = 1.0F;
                    }
                } else {
                    facing = (((x1 - x0) * (v2->sy - y1)) -
                              ((y1 - y0) * (v2->sx - x1))) *
                             winding;
                    if (facing > 0.0F && back_side) {
                        facing = -1.0F;
                    }
                }
            }
            v1_payload = &v1->payload.tex.uv[0];
            if ((Uint32)run_start < (Uint32)queue_cursor) {
                if (facing < 0.0F) {
                    EMIT_UNLIT_TEX_STORED(v1, KM_VERTEXPARAM_NORMAL,
                                          v1_payload);
                } else {
                    EMIT_UNLIT_TEX_STORED(v1, KM_VERTEXPARAM_ENDOFSTRIP,
                                          v1_payload);
                    run_start = queue_cursor;
                }
            } else if (facing < 0.0F) {
                EMIT_UNLIT_TEX_STORED(v0, KM_VERTEXPARAM_NORMAL,
                                      &v0->payload.tex.uv[0]);
                EMIT_UNLIT_TEX_STORED(v1, KM_VERTEXPARAM_NORMAL, v1_payload);
            }
            if (needs_near_clip) {
                model_clip_unlit_tex_triangle(v0, v1, v2);
                run_start = queue_cursor;
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            y1 = v2->sy;
            winding = -winding;
        } while (--remaining != 0);
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            EMIT_UNLIT_TEX_STORED(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                                  &v2->payload.tex.uv[0]);
        }
    } while (--strips_left != 0);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

static void model_draw_unlit_tex_strips_checked(NJS_MESHSET *meshset,
                                                Uint32 material_id)
{
    register Uint16 *mesh_cursor;
    register NJS_COLOR *corner_uv;
    Uint32 strips_left;
    struct model_vertex *vertices;
    register struct model_vertex *v0;
    register struct model_vertex *v1;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 back_side;
    register Float winding;
    register Float x1;
    register Float y1;
    Float x0;
    Float y0;
    register Float facing;
    Uint32 clip_all;
    void *v1_payload;

    DRAW_NOOP();
    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
        back_side = BACK_FACE;
    } else {
        back_side = 0;
    }
    corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c645140 over its literal pool to the strip
     * loop at 0x8c645184; without it, 424 of 442 words differ, the function
     * runs 4 bytes short, and 24 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;

        /* @hack redundant: a never-run store to x0, which gives x0 FR5 and y0
         * FR6, loaded at 0x8c6451c0 and 0x8c6451ce; without it, they trade, and
         * 6 of 442 words differ */
        if (0) {
            x0 = 0.0F;
        }
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
        do {
            v2 = &vertices[*mesh_cursor++];
            PREFETCH_NEXT(vertices, mesh_cursor);
            facing = 1.0F;
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip &
                       CLIP_ALL;
            if (!clip_all) {
                facing = (((x1 - x0) * (v2->sy - y1)) -
                          ((y1 - y0) * (v2->sx - x1))) *
                         winding;
                if (facing > 0.0F) {
                    if (back_side) {
                        facing = -1.0F;
                    }
                }
            }
            v1_payload = &v1->payload;
            if ((Uint32)run_start < (Uint32)queue_cursor) {
                if (facing < 0.0F) {
                    EMIT_UNLIT_TEX_CULL(v1, KM_VERTEXPARAM_NORMAL, v1_payload,
                                        corner_uv, register, auto);
                } else {
                    EMIT_UNLIT_TEX_CULL(v1, KM_VERTEXPARAM_ENDOFSTRIP,
                                        v1_payload, corner_uv, register, auto);
                    corner_uv--;
                    run_start = queue_cursor;
                }
            } else {
                corner_uv++;
                if (facing < 0.0F) {
                    corner_uv--;
                    EMIT_UNLIT_TEX_CULL(v0, KM_VERTEXPARAM_NORMAL, &v0->payload,
                                        corner_uv, register, auto);
                    EMIT_UNLIT_TEX_CULL(v1, KM_VERTEXPARAM_NORMAL, v1_payload,
                                        corner_uv, register, auto);
                }
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            y1 = v2->sy;
            winding = -winding;
        } while (--remaining);
        corner_uv += 2;
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            corner_uv -= 2;
            EMIT_UNLIT_TEX_CULL(v2, KM_VERTEXPARAM_ENDOFSTRIP, &v2->payload,
                                corner_uv, auto, register);
        }
    } while (--strips_left);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

static void model_draw_flat_path_env_strips_near(NJS_MESHSET *meshset,
                                                 Uint32 material_id)
{
    register Uint16 *mesh_cursor;
    register struct model_vertex *v2;
    register struct model_vertex *v1;
    register struct model_vertex *v0;
    register Uint32 *run_start;
    struct model_vertex *vertices;
    /* @hack redundant: unused_corner_uv, set to meshset->vertuv and never
     * read, which as meshset's third use gives it R13, MOV R4,R13 at
     * 0x8c6454d2; without it, 646 of 662 words differ, the function runs 4
     * bytes long, and 26 of model_draw_unlit_tex_strips_checked's 442 differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    NJS_COLOR *unused_corner_uv;
    Uint32 strips_left;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 back_side;
    Uint32 light_side;
    Uint32 needs_near_clip;
    Uint32 clip_all;
    Uint32 clip_near;
    register Float winding;
    register Float x1;
    register Float y1;
    register Float x0;
    register Float y0;
    register Float facing;

    DRAW_NOOP();
    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
        back_side = BACK_FACE;
    } else {
        back_side = 0;
    }

    unused_corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c645516 over its literal pool to the strip
     * loop at 0x8c64555c; without it, 643 of 662 words differ, the function
     * runs 8 bytes short, and 26 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
    next_triangle:
        v2 = &vertices[*mesh_cursor++];
        PREFETCH_NEXT(vertices, mesh_cursor);
        facing = 1.0F;
        light_side = needs_near_clip = 0;
        clip_all =
            v0->control.clip & v1->control.clip & v2->control.clip & CLIP_ALL;
        if (clip_all == 0) {
            clip_near =
                (v0->control.clip | v1->control.clip | v2->control.clip) &
                CLIP_NEAR;
            if (clip_near) {
                facing =
                    view_triangle_facing(&v1->point, &v0->point, &v2->point) *
                    winding;
                if (facing > 0.0F) {
                    if (back_side) {
                        needs_near_clip = light_side = back_side;
                    }
                } else {
                    needs_near_clip = 1;
                    facing = 1.0F;
                }
            } else {
                facing = (((x1 - x0) * (v2->sy - y1)) -
                          ((y1 - y0) * (v2->sx - x1))) *
                         winding;
                if (facing > 0.0F) {
                    if (back_side) {
                        light_side = back_side;
                        facing = -1.0F;
                    }
                }
            }
        }
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            if (facing < 0.0F) {
                EMIT_SMOOTH_ENV(v1, KM_VERTEXPARAM_NORMAL, material_id,
                                light_side);
            } else {
                EMIT_SMOOTH_ENV(v1, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                                light_side);
                run_start = queue_cursor;
            }
        } else if (facing < 0.0F) {
            EMIT_SMOOTH_ENV(v0, KM_VERTEXPARAM_NORMAL, material_id, light_side);
            EMIT_SMOOTH_ENV(v1, KM_VERTEXPARAM_NORMAL, material_id, light_side);
        }
        if (needs_near_clip) {
            model_clip_smooth_env_triangle(v0, v1, v2,
                                           material_id | light_side);
            run_start = queue_cursor;
        }
        v0 = v1;
        v1 = v2;
        x0 = x1;
        y0 = y1;
        x1 = v2->sx;
        y1 = v2->sy;
        winding = -winding;
        if (--remaining) {
            goto next_triangle;
        }
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            EMIT_SMOOTH_ENV(v2, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                            light_side);
        }
    } while (--strips_left);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

static void model_draw_flat_path_env_strips_checked(NJS_MESHSET *meshset,
                                                    Uint32 material_id)
{
    register Uint16 *mesh_cursor;
    register struct model_vertex *v2;
    register struct model_vertex *v1;
    register struct model_vertex *v0;
    register Uint32 *run_start;
    struct model_vertex *vertices;
    Uint32 strips_left;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 back_side;
    Uint32 light_side;
    Uint32 clip_all;
    /* @hack redundant: unused_mesh_type, set to meshset->type_matId and never
     * read, which keeps meshset in R13, MOV R4,R13 at 0x8c645a64; without it,
     * 563 of 583 words differ, and the function runs 4 bytes long */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Uint32 unused_mesh_type;
    register Float winding;
    register Float y1;
    register Float x1;
    register Float facing;
    Float y0;
    Float x0;

    DRAW_NOOP();

    unused_mesh_type = meshset->type_matId;
    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
        back_side = BACK_FACE;
    } else {
        back_side = 0;
    }
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c645ae6 over its literal pool to the strip
     * loop at 0x8c645af4; without it, 546 of 583 words differ, the function
     * runs 12 bytes short */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
    next_triangle:
        v2 = &vertices[*mesh_cursor++];
        PREFETCH_NEXT(vertices, mesh_cursor);
        facing = 1.0F;
        light_side = 0;
        clip_all =
            v0->control.clip & v1->control.clip & v2->control.clip & CLIP_ALL;
        if (clip_all == 0) {
            facing =
                (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                winding;
            if (facing > 0.0F) {
                if (back_side) {
                    light_side = back_side;
                    facing = -1.0F;
                }
            }
        }
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            if (facing < 0.0F) {
                EMIT_SMOOTH_ENV_COLOR_WALK(v1, KM_VERTEXPARAM_NORMAL,
                                           material_id, light_side);
            } else {
                EMIT_SMOOTH_ENV_COLOR_WALK(v1, KM_VERTEXPARAM_ENDOFSTRIP,
                                           material_id, light_side);
                run_start = queue_cursor;
            }
        } else if (facing < 0.0F) {
            EMIT_SMOOTH_ENV_OPEN(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                 light_side);
            EMIT_SMOOTH_ENV_COLOR_WALK(v1, KM_VERTEXPARAM_NORMAL, material_id,
                                       light_side);
        }
        v0 = v1;
        v1 = v2;
        x0 = x1;
        y0 = y1;
        x1 = v2->sx;
        y1 = v2->sy;
        winding = -winding;
        if (--remaining) {
            goto next_triangle;
        }
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            EMIT_SMOOTH_ENV_COLOR_WALK(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                                       material_id, light_side);
        }
    } while (--strips_left);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

/* EMIT_UNLIT_ENV with locals of its own, storing packet_command without
 * vertex_command */
/* @hack spelling: a Float pointer walks the vertex normal's x into y through
 * vector_floats, which gives the FMOV.S @Rn+ load of x (0x8c64613e in
 * model_draw_unlit_env_strips_near); read by member, 50 of 484 words differ in
 * model_draw_unlit_env_strips_near */
#define EMIT_UNLIT_ENV_NEAR(vertex, packet_command)                            \
    {                                                                          \
        Float *uv_out;                                                         \
        Float *normal_xy;                                                      \
        Float half;                                                            \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        PACKED_COLOR(vertex, 0) = PACK_ARGB(_nj_light_material_.diffuse);      \
        uv_out = (vertex)->payload.tex.uv;                                     \
        normal_xy = ((union vector_floats *)&(vertex)->vector)->f;             \
        half = 0.5F;                                                           \
        *uv_out++ = (*normal_xy++ * half) + half;                              \
        *uv_out = (*normal_xy * half) + half;                                  \
        writeback_line(uv_out);                                                \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_unlit_env_strips_near(NJS_MESHSET *meshset,
                                             Uint32 material_id)
{
    NJS_MESHSET *source_meshset = meshset;
    register Uint16 *mesh_cursor;
    register struct model_vertex *v0;
    register struct model_vertex *v1;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    struct model_vertex *vertices;
    Uint32 strips_left;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 back_side;
    Uint32 needs_near_clip;
    Uint32 clip_all;
    Uint32 clip_near;
    register Float winding;
    register Float x0;
    register Float y0;
    register Float x1;
    register Float y1;
    register Float facing;
    DRAW_NOOP();
    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
        back_side = BACK_FACE;
    } else {
        back_side = 0;
    }
    mesh_cursor = (Uint16 *)source_meshset->meshes;
    strips_left = source_meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c645fa4 over its literal pool to the strip
     * loop at 0x8c645fdc; without it, 120 of 484 words differ, and 17 differ
     * beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
        do {
            v2 = &vertices[*mesh_cursor++];
            PREFETCH_NEXT(vertices, mesh_cursor);
            needs_near_clip = 0;
            facing = 1.0F;
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip &
                       CLIP_ALL;
            if (clip_all == 0) {
                clip_near =
                    (v0->control.clip | v1->control.clip | v2->control.clip) &
                    CLIP_NEAR;
                if (clip_near) {
                    facing = view_triangle_facing(&v1->point, &v0->point,
                                                  &v2->point) *
                             winding;
                    if (facing > 0.0F) {
                        needs_near_clip = back_side;
                    } else {
                        needs_near_clip = 1;
                        facing = 1.0F;
                    }
                } else {
                    facing = (((x1 - x0) * (v2->sy - y1)) -
                              ((y1 - y0) * (v2->sx - x1))) *
                             winding;
                    if (facing > 0.0F) {
                        if (back_side) {
                            facing = -1.0F;
                        }
                    }
                }
            }
            if ((Uint32)run_start < (Uint32)queue_cursor) {
                if (facing < 0.0F) {
                    EMIT_UNLIT_ENV_NEAR(v1, KM_VERTEXPARAM_NORMAL);
                } else {
                    EMIT_UNLIT_ENV_NEAR(v1, KM_VERTEXPARAM_ENDOFSTRIP);
                    run_start = queue_cursor;
                }
            } else if (facing < 0.0F) {
                EMIT_UNLIT_ENV_NEAR(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_ENV_NEAR(v1, KM_VERTEXPARAM_NORMAL);
            }
            if (needs_near_clip) {
                model_clip_unlit_env_triangle(v0, v1, v2);
                run_start = queue_cursor;
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            y1 = v2->sy;
            winding = -winding;
        } while (--remaining);
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            EMIT_UNLIT_ENV_NEAR(v2, KM_VERTEXPARAM_ENDOFSTRIP);
        }
    } while (--strips_left);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

static void model_draw_unlit_env_strips_checked(NJS_MESHSET *meshset,
                                                Uint32 material_id)
{
    NJS_MESHSET *source_meshset = meshset;
    register Uint16 *mesh_cursor;
    Uint32 strips_left;
    struct model_vertex *vertices;
    register struct model_vertex *v0;
    register struct model_vertex *v1;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 back_side;
    Uint32 clip_all;
    register Float winding;
    Float x0;
    Float y0;
    register Float x1;
    register Float y1;
    register Float facing;

    DRAW_NOOP();
    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
        back_side = BACK_FACE;
    } else {
        back_side = 0;
    }
    mesh_cursor = (Uint16 *)source_meshset->meshes;
    strips_left = source_meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c6463c4 over its literal pool to the strip
     * loop at 0x8c6463fc; without it, 133 of 424 words differ, and 12 differ
     * beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    for (;;) {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
        for (;;) {
            v2 = &vertices[*mesh_cursor++];
            PREFETCH_NEXT(vertices, mesh_cursor);
            facing = 1.0F;

            /* @hack redundant: a never-run copy of x1 into facing, which gives
             * x1 FR14 and y1 FR13, loaded at 0x8c646444 and 0x8c64644a; without
             * it, they trade, and 10 of 424 words differ */
            if (0) {
                facing = x1;
            }

            /* @hack spelling: the mask built in two statements, CLIP_ALL and
             * then the corners' bits, which keeps 1.0F in FR12 from FLDI1 at
             * 0x8c646392; as one expression, SHC loads it from a pool, 158 of
             * 424 words differ, and 12 of model_draw_unlit_env_strips_near's
             * 484 */
            clip_all = CLIP_ALL;
            clip_all &= v0->control.clip & v1->control.clip & v2->control.clip;
            if (clip_all == 0) {
                facing = (((x1 - x0) * (v2->sy - y1)) -
                          ((y1 - y0) * (v2->sx - x1))) *
                         winding;
                if (facing > 0.0F) {
                    if (back_side) {
                        facing = -1.0F;
                    }
                }
            }
            if ((Uint32)run_start < (Uint32)queue_cursor) {
                if (facing < 0.0F) {
                    EMIT_UNLIT_ENV_CULL(v1, KM_VERTEXPARAM_NORMAL);
                } else {
                    EMIT_UNLIT_ENV_CULL(v1, KM_VERTEXPARAM_ENDOFSTRIP);
                    run_start = queue_cursor;
                }
            } else if (facing < 0.0F) {
                EMIT_UNLIT_ENV_CULL(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_ENV_CULL(v1, KM_VERTEXPARAM_NORMAL);
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            y1 = v2->sy;
            winding = -winding;
            if (--remaining == 0) {
                break;
            }
        }
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            EMIT_UNLIT_ENV_CULL(v2, KM_VERTEXPARAM_ENDOFSTRIP);
        }
        if (--strips_left == 0) {
            break;
        }
    }
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

static void model_draw_unlit_strips_near(NJS_MESHSET *meshset,
                                         Uint32 material_id)
{
    NJS_MESHSET *source_meshset = meshset;
    register Uint16 *mesh_cursor;
    register struct model_vertex *v0;
    register struct model_vertex *v1;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    struct model_vertex *vertices;
    Uint32 strips_left;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 back_side;
    Uint32 needs_near_clip;
    Uint32 clip_all;
    Uint32 clip_near;
    register Float winding;
    register Float x0;
    register Float y0;
    register Float x1;
    register Float y1;
    register Float facing;
    DRAW_NOOP();
    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
        back_side = BACK_FACE;
    } else {
        back_side = 0;
    }
    mesh_cursor = (Uint16 *)source_meshset->meshes;
    strips_left = source_meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c646762 over its literal pool to the strip
     * loop at 0x8c646798; without it, 376 of 407 words differ, the function
     * runs 4 bytes short, and 14 differ beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
        do {
            v2 = &vertices[*mesh_cursor++];
            PREFETCH_NEXT(vertices, mesh_cursor);
            needs_near_clip = 0;
            facing = 1.0F;
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip &
                       CLIP_ALL;
            if (clip_all == 0) {
                clip_near =
                    (v0->control.clip | v1->control.clip | v2->control.clip) &
                    CLIP_NEAR;
                if (clip_near) {
                    facing = view_triangle_facing(&v1->point, &v0->point,
                                                  &v2->point) *
                             winding;
                    if (facing > 0.0F) {
                        needs_near_clip = back_side;
                    } else {
                        needs_near_clip = 1;
                        facing = 1.0F;
                    }
                } else {
                    facing = (((x1 - x0) * (v2->sy - y1)) -
                              ((y1 - y0) * (v2->sx - x1))) *
                             winding;
                    if (facing > 0.0F) {
                        if (back_side) {
                            facing = -1.0F;
                        }
                    }
                }
            }
            if ((Uint32)run_start < (Uint32)queue_cursor) {
                if (facing < 0.0F) {
                    EMIT_UNLIT_COPY(v1, KM_VERTEXPARAM_NORMAL);
                } else {
                    EMIT_UNLIT_COPY(v1, KM_VERTEXPARAM_ENDOFSTRIP);
                    run_start = queue_cursor;
                }
            } else if (facing < 0.0F) {
                EMIT_UNLIT_COPY(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_COPY(v1, KM_VERTEXPARAM_NORMAL);
            } else {
                /* @hack redundant: an empty else, which gives the clip test's
                 * load R3, MOV.L @(16,R15),R3 at 0x8c6469d0; without it, R2,
                 * and 2 of 407 words differ */
            }
            if (needs_near_clip) {
                model_clip_unlit_triangle(v0, v1, v2);
                run_start = queue_cursor;
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            y1 = v2->sy;
            winding = -winding;
        } while (--remaining);
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            EMIT_UNLIT_COPY(v2, KM_VERTEXPARAM_ENDOFSTRIP);
        }
    } while (--strips_left);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

static void model_draw_unlit_strips_checked(NJS_MESHSET *meshset,
                                            Uint32 material_id)
{
    NJS_MESHSET *source_meshset = meshset;
    register Uint16 *mesh_cursor;
    Uint32 strips_left;
    struct model_vertex *vertices;
    register struct model_vertex *v0;
    register struct model_vertex *v1;
    register struct model_vertex *v2;
    register Uint32 *run_start;
    Uint32 remaining;
    Uint32 input_vertices;
    Uint32 back_side;
    Uint32 clip_all;
    register Float winding;
    Float x0;
    Float y0;
    register Float x1;
    register Float y1;
    register Float facing;
    DRAW_NOOP();
    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
        back_side = BACK_FACE;
    } else {
        back_side = 0;
    }
    mesh_cursor = (Uint16 *)source_meshset->meshes;
    strips_left = source_meshset->nbMesh;
    vertices = (struct model_vertex *)_nj_vertex_buf_;
    input_vertices = 0;
    /* @hack redundant: a #pragma asm block of one assembler comment, ahead of
     * which SHC writes the BRA at 0x8c646ad0 over its literal pool to the strip
     * loop at 0x8c646b04; without it, 307 of 345 words differ, and 9 differ
     * beside it */
    /* clang-format off */
#ifndef __clang__
#pragma asm
; strip loop;
#pragma endasm
#endif
    /* clang-format on */
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_vertices += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = queue_cursor;
        do {
            v2 = &vertices[*mesh_cursor++];
            PREFETCH_NEXT(vertices, mesh_cursor);
            facing = 1.0F;

            /* @hack spelling: the mask built in two statements, CLIP_ALL and
             * then the corners' bits, which keeps 1.0F in FR12 from FLDI1 at
             * 0x8c646a9e; as one expression, SHC loads it from a pool, 174 of
             * 345 words differ, and 9 of model_draw_unlit_strips_near's 407 */
            clip_all = CLIP_ALL;
            clip_all &= v0->control.clip & v1->control.clip & v2->control.clip;
            if (clip_all == 0) {
                facing = (((x1 - x0) * (v2->sy - y1)) -
                          ((y1 - y0) * (v2->sx - x1))) *
                         winding;
                if (facing > 0.0F) {
                    if (back_side) {
                        facing = -1.0F;
                    }
                }
            }
            if ((Uint32)run_start < (Uint32)queue_cursor) {
                if (facing < 0.0F) {
                    EMIT_UNLIT_COPY_ALIASED(v1, KM_VERTEXPARAM_NORMAL);
                } else {
                    EMIT_UNLIT_COPY_ALIASED(v1, KM_VERTEXPARAM_ENDOFSTRIP);
                    run_start = queue_cursor;
                }
            } else if (facing < 0.0F) {
                EMIT_UNLIT_COPY_ALIASED(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_COPY_ALIASED(v1, KM_VERTEXPARAM_NORMAL);
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            y1 = v2->sy;
            winding = -winding;
        } while (--remaining != 0);
        if ((Uint32)run_start < (Uint32)queue_cursor) {
            EMIT_UNLIT_COPY_ALIASED(v2, KM_VERTEXPARAM_ENDOFSTRIP);
        }
    } while (--strips_left != 0);
    _nj_calc_polygon_ += input_vertices;
    _nj_draw_polygon_ += input_vertices;
}

/* clang-format off */
#ifndef __clang__
/* @asm open: no inspect rule reads its MOVA of H'437F0000 */
#pragma inline_asm(pack_argb_body)
static Uint32 pack_argb_body(Float a, Float r, Float g, Float b)
{
        MOVA        #H'437F0000,R0
        FLDI0       FR0
        FLDI1       FR1
        FMOV.S      @R0,FR2
        FCMP/GT     FR4,FR0
        BF          ?a_low
        BRA         ?a_done
        FLDI0       FR4
?a_low:
        FCMP/GT     FR1,FR4
        BF          ?a_done
        FLDI1       FR4
?a_done:
        FCMP/GT     FR5,FR0
        BF          ?r_low
        BRA         ?r_done
        FLDI0       FR5
?r_low:
        FCMP/GT     FR1,FR5
        BF          ?r_done
        FLDI1       FR5
?r_done:
        FCMP/GT     FR6,FR0
        BF          ?g_low
        BRA         ?g_done
        FLDI0       FR6
?g_low:
        FCMP/GT     FR1,FR6
        BF          ?g_done
        FLDI1       FR6
?g_done:
        FCMP/GT     FR7,FR0
        BF          ?b_low
        BRA         ?b_done
        FLDI0       FR7
?b_low:
        FCMP/GT     FR1,FR7
        BF          ?b_done
        FLDI1       FR7
?b_done:
        FMUL        FR2,FR4
        FMUL        FR2,FR5
        FMUL        FR2,FR6
        FMUL        FR2,FR7
        FTRC        FR4,FPUL
        STS         FPUL,R0
        FTRC        FR5,FPUL
        STS         FPUL,R1
        FTRC        FR6,FPUL
        STS         FPUL,R2
        FTRC        FR7,FPUL
        STS         FPUL,R3
        SHLL16      R0
        SHLL16      R1
        SHLL8       R0
        SHLL8       R2
        OR          R1,R0
        OR          R3,R2
        OR          R2,R0
        RTS
        NOP
}
#endif
/* clang-format on */

static Uint32 model_pack_argb(Float a, Float r, Float g, Float b)
{
    return pack_argb_body(a, r, g, b);
}

/* clang-format off */
#ifndef __clang__
/* @asm open: no inspect rule reads its MOVA of H'3B808080 */
#pragma inline_asm(unpack_argb_body)
static void unpack_argb_body(Uint32 color, NJS_ARGB *dst)
{
        MOVA        #H'3B808080,R0
        FMOV.S      @R0,FR4
        MOV         R4,R0
        EXTU.B      R0,R0
        SHLR8       R4
        MOV         R4,R1
        EXTU.B      R1,R1
        SHLR8       R4
        MOV         R4,R2
        EXTU.B      R2,R2
        SHLR8       R4
        MOV         R4,R3
        EXTU.B      R3,R3
        LDS         R0,FPUL
        FLOAT       FPUL,FR0
        LDS         R1,FPUL
        FLOAT       FPUL,FR1
        LDS         R2,FPUL
        FLOAT       FPUL,FR2
        LDS         R3,FPUL
        FLOAT       FPUL,FR3
        FMUL        FR4,FR0
        FMUL        FR4,FR1
        FMUL        FR4,FR2
        FMUL        FR4,FR3
        ADD         #16,R5
        FMOV.S      FR0,@-R5
        FMOV.S      FR1,@-R5
        FMOV.S      FR2,@-R5
        FMOV.S      FR3,@-R5
        RTS
        NOP
}
#endif
/* clang-format on */

static void model_unpack_argb(Uint32 color, NJS_ARGB *dst)
{
    unpack_argb_body(color, dst);
}

/* untextured, every corner in constant_color, for the flat drawers */
static void model_clip_constant_color_triangle(struct model_vertex *v0,
                                               struct model_vertex *v1,
                                               struct model_vertex *v2,
                                               NJS_ARGB *constant_color)
{
    struct model_vertex *vertices[3];
    Float edge_t;
    Float view_x;
    Float view_y;
    Sint32 corner;
    Sint32 previous;
    Sint32 output_count;

    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    output_count = 0;
    previous = 2;
    for (corner = 0; corner < 3; corner++) {
        if (((vertices[previous]->control.clip & CLIP_NEAR) ^
             (vertices[corner]->control.clip & CLIP_NEAR)) == 1) {
            edge_t = (_nj_near_clip_ - vertices[previous]->point.z) /
                     (vertices[corner]->point.z - vertices[previous]->point.z);
            view_x =
                vertices[previous]->point.x +
                ((vertices[corner]->point.x - vertices[previous]->point.x) *
                 edge_t);
            view_y =
                vertices[previous]->point.y +
                ((vertices[corner]->point.y - vertices[previous]->point.y) *
                 edge_t);
            vtx4[output_count].sx =
                (near_reciprocal * projection_x * view_x) + _nj_screen_.cx;
            vtx4[output_count].sy =
                (near_reciprocal * projection_y * view_y) + _nj_screen_.cy;
            vtx4[output_count].oow = near_reciprocal;
            vtx4[output_count].col.a = constant_color->a;
            vtx4[output_count].col.r = constant_color->r;
            vtx4[output_count].col.g = constant_color->g;
            vtx4[output_count].col.b = constant_color->b;
            output_count++;
        }
        if (!(vertices[corner]->control.clip & CLIP_NEAR)) {
            vtx4[output_count].sx = vertices[corner]->sx;
            vtx4[output_count].sy = vertices[corner]->sy;
            vtx4[output_count].oow = vertices[corner]->oow;
            vtx4[output_count].col.a = constant_color->a;
            vtx4[output_count].col.r = constant_color->r;
            vtx4[output_count].col.g = constant_color->g;
            vtx4[output_count].col.b = constant_color->b;
            output_count++;
        }
        previous = corner;
    }
    if (output_count == 3) {
        vtx4[0].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx4[0]);
        vtx4[1].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx4[1]);
        vtx4[2].cmd = KM_VERTEXPARAM_ENDOFSTRIP;
        submit_packet(&vtx4[2]);
        _nj_draw_polygon_ += 3;
    } else {
        vtx4[0].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx4[0]);
        vtx4[1].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx4[1]);
        vtx4[3].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx4[3]);
        vtx4[2].cmd = KM_VERTEXPARAM_ENDOFSTRIP;
        submit_packet(&vtx4[2]);
        _nj_draw_polygon_ += 4;
    }
}

/* textured, every corner in the packed base and offset colors at
 * packed_colors, for the flat drawers */
static void model_clip_constant_tex_triangle(struct model_vertex *v0,
                                             struct model_vertex *v1,
                                             struct model_vertex *v2,
                                             const Uint32 *packed_colors)
{
    struct model_vertex *vertices[3];
    Float edge_t;
    Float view_x;
    Float view_y;
    Sint32 corner;
    Sint32 previous;
    Sint32 output_count;

    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    output_count = 0;
    previous = 2;
    for (corner = 0; corner < 3; corner++) {
        if (((vertices[previous]->control.clip & CLIP_NEAR) ^
             (vertices[corner]->control.clip & CLIP_NEAR)) == 1) {
            edge_t = (_nj_near_clip_ - vertices[previous]->point.z) /
                     (vertices[corner]->point.z - vertices[previous]->point.z);
            view_x =
                vertices[previous]->point.x +
                ((vertices[corner]->point.x - vertices[previous]->point.x) *
                 edge_t);
            view_y =
                vertices[previous]->point.y +
                ((vertices[corner]->point.y - vertices[previous]->point.y) *
                 edge_t);
            vtx2[output_count].sx =
                (near_reciprocal * projection_x * view_x) + _nj_screen_.cx;
            vtx2[output_count].sy =
                (near_reciprocal * projection_y * view_y) + _nj_screen_.cy;
            vtx2[output_count].oow = near_reciprocal;
            vtx2[output_count].u = vertices[previous]->payload.tex.uv[0] +
                                   ((vertices[corner]->payload.tex.uv[0] -
                                     vertices[previous]->payload.tex.uv[0]) *
                                    edge_t);
            vtx2[output_count].v = vertices[previous]->payload.tex.uv[1] +
                                   ((vertices[corner]->payload.tex.uv[1] -
                                     vertices[previous]->payload.tex.uv[1]) *
                                    edge_t);
            vtx2[output_count].color = packed_colors[0];
            vtx2[output_count].offset = packed_colors[1];
            output_count++;
        }
        if (!(vertices[corner]->control.clip & CLIP_NEAR)) {
            vtx2[output_count].sx = vertices[corner]->sx;
            vtx2[output_count].sy = vertices[corner]->sy;
            vtx2[output_count].oow = vertices[corner]->oow;
            vtx2[output_count].u = vertices[corner]->payload.tex.uv[0];
            vtx2[output_count].v = vertices[corner]->payload.tex.uv[1];
            vtx2[output_count].color = packed_colors[0];
            vtx2[output_count].offset = packed_colors[1];
            output_count++;
        }
        previous = corner;
    }
    if (output_count == 3) {
        vtx2[0].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[0]);
        vtx2[1].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[1]);
        vtx2[2].cmd = KM_VERTEXPARAM_ENDOFSTRIP;
        submit_packet(&vtx2[2]);
        _nj_draw_polygon_ += 3;
    } else {
        vtx2[0].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[0]);
        vtx2[1].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[1]);
        vtx2[3].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[3]);
        vtx2[2].cmd = KM_VERTEXPARAM_ENDOFSTRIP;
        submit_packet(&vtx2[2]);
        _nj_draw_polygon_ += 4;
    }
}

/* with environment coordinates, every corner in the packed colors at
 * packed_colors, for the flat drawers */
static void model_clip_constant_env_triangle(struct model_vertex *v0,
                                             struct model_vertex *v1,
                                             struct model_vertex *v2,
                                             const Uint32 *packed_colors)
{
    struct model_vertex *vertices[3];
    Float edge_t;
    Float view_x;
    Float view_y;
    Sint32 corner;
    Sint32 previous;
    Sint32 corner_index;
    Sint32 output_count;

    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    for (corner_index = 0; corner_index < 3; corner_index++) {
        vertices[corner_index]->payload.tex.uv[0] =
            (vertices[corner_index]->vector.x * 0.5F) + 0.5F;
        vertices[corner_index]->payload.tex.uv[1] =
            (vertices[corner_index]->vector.y * 0.5F) + 0.5F;
    }
    output_count = 0;
    previous = 2;
    for (corner = 0; corner < 3; corner++) {
        if (((vertices[previous]->control.clip & CLIP_NEAR) ^
             (vertices[corner]->control.clip & CLIP_NEAR)) == 1) {
            edge_t = (_nj_near_clip_ - vertices[previous]->point.z) /
                     (vertices[corner]->point.z - vertices[previous]->point.z);
            view_x =
                vertices[previous]->point.x +
                ((vertices[corner]->point.x - vertices[previous]->point.x) *
                 edge_t);
            view_y =
                vertices[previous]->point.y +
                ((vertices[corner]->point.y - vertices[previous]->point.y) *
                 edge_t);
            vtx2[output_count].sx =
                (near_reciprocal * projection_x * view_x) + _nj_screen_.cx;
            vtx2[output_count].sy =
                (near_reciprocal * projection_y * view_y) + _nj_screen_.cy;
            vtx2[output_count].oow = near_reciprocal;
            vtx2[output_count].u = vertices[previous]->payload.tex.uv[0] +
                                   ((vertices[corner]->payload.tex.uv[0] -
                                     vertices[previous]->payload.tex.uv[0]) *
                                    edge_t);
            vtx2[output_count].v = vertices[previous]->payload.tex.uv[1] +
                                   ((vertices[corner]->payload.tex.uv[1] -
                                     vertices[previous]->payload.tex.uv[1]) *
                                    edge_t);
            vtx2[output_count].color = packed_colors[0];
            vtx2[output_count].offset = packed_colors[1];
            output_count++;
        }
        if (!(vertices[corner]->control.clip & CLIP_NEAR)) {
            vtx2[output_count].sx = vertices[corner]->sx;
            vtx2[output_count].sy = vertices[corner]->sy;
            vtx2[output_count].oow = vertices[corner]->oow;
            vtx2[output_count].u = vertices[corner]->payload.tex.uv[0];
            vtx2[output_count].v = vertices[corner]->payload.tex.uv[1];
            vtx2[output_count].color = packed_colors[0];
            vtx2[output_count].offset = packed_colors[1];
            output_count++;
        }
        previous = corner;
    }
    if (output_count == 3) {
        vtx2[0].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[0]);
        vtx2[1].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[1]);
        vtx2[2].cmd = KM_VERTEXPARAM_ENDOFSTRIP;
        submit_packet(&vtx2[2]);
        _nj_draw_polygon_ += 3;
    } else {
        vtx2[0].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[0]);
        vtx2[1].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[1]);
        vtx2[3].cmd = KM_VERTEXPARAM_NORMAL;
        submit_packet(&vtx2[3]);
        vtx2[2].cmd = KM_VERTEXPARAM_ENDOFSTRIP;
        submit_packet(&vtx2[2]);
        _nj_draw_polygon_ += 4;
    }
}

/* set up the lights that are on with njSetQueue, njSetLightMaterials and
 * njCalcLightMatrices, draw model through njDrawModelSub, then empty the light
 * queue */
void _njDrawModel(NJS_MODEL *model)
{
    njSetQueue();
    njSetLightMaterials();
    njCalcLightMatrices();
    njDrawModelSub(model);
    njResetQueue();
}

/* the polygon drawers draw a meshset from the work vertices by its NJD_MESHSET_
 * type. a triangle or a quad is three or four indices, and a polygon is a
 * count, then its indices. each goes out as one strip, unless the screen
 * winding of its first three corners faces away from the eye. then it is
 * skipped, or for a two-sided material sent lit from behind. a drawer passes a
 * trimesh to a strip drawer: the _all one for a two-sided material, and else
 * the _cull one. each drawer adds the corners to _nj_calc_polygon_, and those
 * it sends to _nj_draw_polygon_.
 *
 * _njDrawModelSub calls a _clip drawer when some point has clip bits. a _clip
 * drawer skips a polygon whose corners share a clip bit. it judges a polygon
 * with a corner below the near plane by its winding in view space, and sends it
 * through a clipper: a quad as triangles 0, 1, 2 and 1, 2, 3, and a polygon as
 * each three consecutive corners. it clips only those triangles that have a
 * corner above the plane. a _clip drawer passes a trimesh to the _near strip
 * drawer when some point lies below the near plane, and else to the _checked
 * one. each corner has its own texture coordinates, which a skipped polygon
 * steps past.
 *
 * the smooth drawers light each corner as the strip drawers do. the flat
 * drawers light a polygon once, by its plane's unit normal. the unlit drawers
 * take the material's diffuse color */
static void model_draw_smooth_polygons(NJS_MESHSET *meshset,
                                       NJS_MATERIAL *material)
{
    Uint16 *mesh_cursor;
    struct model_vertex *v0;
    struct model_vertex *v1;
    struct model_vertex *v2;
    struct model_vertex *v3;
    Uint16 material_id;
    Uint32 light_side;
    Uint32 light_key;
    Sint32 polygon_index;
    Sint32 corner;
    Sint32 corner_count;
    Float facing;
    void (*strip_drawer)(NJS_MESHSET *, Uint32);

    material_id = meshset->type_matId & MATERIAL_ID;
    mesh_cursor = (Uint16 *)meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            light_side = 0;
            if (facing > 0.0F) {
                if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                    light_side = BACK_FACE;
                } else {
                    continue;
                }
            }
            EMIT_SMOOTH_POLYGON(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                light_side);
            EMIT_SMOOTH_POLYGON(v1, KM_VERTEXPARAM_NORMAL, material_id,
                                light_side);
            EMIT_SMOOTH_POLYGON(v2, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                                light_side);
            _nj_draw_polygon_ += 3;
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            light_side = 0;
            if (facing > 0.0F) {
                if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                    light_side = BACK_FACE;
                } else {
                    continue;
                }
            }
            EMIT_SMOOTH_POLYGON(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                light_side);
            EMIT_SMOOTH_POLYGON(v1, KM_VERTEXPARAM_NORMAL, material_id,
                                light_side);
            EMIT_SMOOTH_POLYGON(v2, KM_VERTEXPARAM_NORMAL, material_id,
                                light_side);
            EMIT_SMOOTH_POLYGON(v3, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                                light_side);
            _nj_draw_polygon_ += 4;
        }
        break;
    case NJD_MESHSET_N:
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            corner_count = *mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            v0 = WORK_VERTEX(mesh_cursor[0]);
            v1 = WORK_VERTEX(mesh_cursor[1]);
            v2 = WORK_VERTEX(mesh_cursor[2]);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            light_side = 0;
            if (facing > 0.0F) {
                if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                    light_side = BACK_FACE;
                } else {
                    mesh_cursor += corner_count;
                    continue;
                }
            }
            _nj_draw_polygon_ += corner_count;
            for (corner = 0; corner < corner_count - 1; corner++) {
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_SMOOTH_POLYGON(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                    light_side);
            }
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_SMOOTH_POLYGON(v0, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                                light_side);
        }
        break;
    case NJD_MESHSET_TRIMESH:
        strip_drawer = (material_flags & NJD_FLAG_DOUBLE_SIDE)
                           ? model_draw_smooth_strips_all
                           : model_draw_smooth_strips_cull;
        strip_drawer(meshset, material_id);
        break;
    }
}

static void model_draw_smooth_polygons_clip(NJS_MESHSET *meshset,
                                            NJS_MATERIAL *material)
{
    Uint16 *mesh_cursor;
    struct model_vertex *v0;
    struct model_vertex *v1;
    struct model_vertex *v2;
    struct model_vertex *v3;
    Uint16 material_id;
    Uint32 light_side;
    Uint32 light_key;
    Uint32 clip_all;
    Uint32 clip_any;
    Sint32 polygon_index;
    Sint32 corner;
    Sint32 corner_count;
    Float facing;
    void (*strip_drawer)(NJS_MESHSET *, Uint32);

    material_id = meshset->type_matId & MATERIAL_ID;
    mesh_cursor = (Uint16 *)meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip;
            if (clip_all & CLIP_ALL) {
                continue;
            }
            clip_any = v0->control.clip | v1->control.clip | v2->control.clip;
            if (!(clip_any & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        continue;
                    }
                }
                EMIT_SMOOTH_POLYGON(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                    light_side);
                EMIT_SMOOTH_POLYGON(v1, KM_VERTEXPARAM_NORMAL, material_id,
                                    light_side);
                EMIT_SMOOTH_POLYGON(v2, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                                    light_side);
                _nj_draw_polygon_ += 3;
            } else {
                facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                         (v1->point.y * v0->point.z))) +
                         (v2->point.y * ((v1->point.x * v0->point.z) -
                                         (v1->point.z * v0->point.x))) +
                         (v2->point.z * ((v0->point.x * v1->point.y) -
                                         (v1->point.x * v0->point.y)));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        continue;
                    }
                }
                model_clip_smooth_triangle(v0, v1, v2,
                                           material_id | light_side);
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip &
                       v3->control.clip;
            if (clip_all & CLIP_ALL) {
                continue;
            }
            clip_any = v0->control.clip | v1->control.clip | v2->control.clip |
                       v3->control.clip;
            if (!(clip_any & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        continue;
                    }
                }
                EMIT_SMOOTH_POLYGON(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                    light_side);
                EMIT_SMOOTH_POLYGON(v1, KM_VERTEXPARAM_NORMAL, material_id,
                                    light_side);
                EMIT_SMOOTH_POLYGON(v2, KM_VERTEXPARAM_NORMAL, material_id,
                                    light_side);
                EMIT_SMOOTH_POLYGON(v3, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                                    light_side);
                _nj_draw_polygon_ += 4;
            } else {
                facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                         (v1->point.y * v0->point.z))) +
                         (v2->point.y * ((v1->point.x * v0->point.z) -
                                         (v1->point.z * v0->point.x))) +
                         (v2->point.z * ((v0->point.x * v1->point.y) -
                                         (v1->point.x * v0->point.y)));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        continue;
                    }
                }
                clip_all =
                    v0->control.clip & v1->control.clip & v2->control.clip;
                if (!(clip_all & CLIP_NEAR)) {
                    model_clip_smooth_triangle(v0, v1, v2,
                                               material_id | light_side);
                }
                clip_all =
                    v1->control.clip & v2->control.clip & v3->control.clip;
                if (!(clip_all & CLIP_NEAR)) {
                    model_clip_smooth_triangle(v1, v2, v3,
                                               material_id | light_side);
                }
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            corner_count = *mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            clip_all = ~(Uint32)0;
            clip_any = 0;
            for (corner = 0; corner < corner_count; corner++) {
                clip_all &= WORK_VERTEX(mesh_cursor[corner])->control.clip;
                clip_any |= WORK_VERTEX(mesh_cursor[corner])->control.clip;
            }
            if (clip_all & CLIP_ALL) {
                mesh_cursor += corner_count;
                continue;
            }
            if (!(clip_any & CLIP_NEAR)) {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        mesh_cursor += corner_count;
                        continue;
                    }
                }
                _nj_draw_polygon_ += corner_count;
                for (corner = 0; corner < corner_count - 1; corner++) {
                    v0 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_SMOOTH_POLYGON(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                        light_side);
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_SMOOTH_POLYGON(v0, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                                    light_side);
            } else {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);
                facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                         (v1->point.y * v0->point.z))) +
                         (v2->point.y * ((v1->point.x * v0->point.z) -
                                         (v1->point.z * v0->point.x))) +
                         (v2->point.z * ((v0->point.x * v1->point.y) -
                                         (v1->point.x * v0->point.y)));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        mesh_cursor += corner_count;
                        continue;
                    }
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                v1 = WORK_VERTEX(*mesh_cursor++);
                for (corner = 0; corner < corner_count - 2; corner++) {
                    v2 = WORK_VERTEX(*mesh_cursor++);
                    clip_all =
                        v0->control.clip & v1->control.clip & v2->control.clip;
                    if (!(clip_all & CLIP_NEAR)) {
                        model_clip_smooth_triangle(v0, v1, v2,
                                                   material_id | light_side);
                    }
                    v0 = v1;
                    v1 = v2;
                }
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        strip_drawer = (model_clip_or & CLIP_NEAR)
                           ? model_draw_smooth_strips_near
                           : model_draw_smooth_strips_checked;
        strip_drawer(meshset, material_id);
        break;
    }
}

/* EMIT_CHECKED_TEX for a polygon's corner, writing back the cache line of
 * vertex's own payload */
#define EMIT_SMOOTH_TEX_POLYGON(vertex, packet_command, material_id,           \
                                light_side, corner_uv)                         \
    {                                                                          \
        EMIT_CHECKED_TEX(vertex, packet_command, &(vertex)->payload,           \
                         material_id, light_side, corner_uv, auto, auto);      \
    }

static void model_draw_smooth_tex_polygons(NJS_MESHSET *meshset,
                                           NJS_MATERIAL *material)
{
    Uint16 *mesh_cursor;
    struct model_vertex *v0;
    struct model_vertex *v1;
    struct model_vertex *v2;
    struct model_vertex *v3;
    NJS_COLOR *corner_uv;
    Uint16 material_id;
    Uint32 light_side;
    Sint32 polygon_index;
    Sint32 corner;
    Sint32 corner_count;
    Float facing;
    void (*strip_drawer)(NJS_MESHSET *, Uint32);

    material_id = meshset->type_matId & MATERIAL_ID;
    corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            light_side = 0;
            if (facing > 0.0F) {
                if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                    light_side = BACK_FACE;
                } else {
                    corner_uv += 3;
                    continue;
                }
            }
            EMIT_SMOOTH_TEX_POLYGON(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                    light_side, corner_uv);
            EMIT_SMOOTH_TEX_POLYGON(v1, KM_VERTEXPARAM_NORMAL, material_id,
                                    light_side, corner_uv);
            EMIT_SMOOTH_TEX_POLYGON(v2, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                                    light_side, corner_uv);
            _nj_draw_polygon_ += 3;
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            light_side = 0;
            if (facing > 0.0F) {
                if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                    light_side = BACK_FACE;
                } else {
                    corner_uv += 4;
                    continue;
                }
            }
            EMIT_SMOOTH_TEX_POLYGON(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                    light_side, corner_uv);
            EMIT_SMOOTH_TEX_POLYGON(v1, KM_VERTEXPARAM_NORMAL, material_id,
                                    light_side, corner_uv);
            EMIT_SMOOTH_TEX_POLYGON(v2, KM_VERTEXPARAM_NORMAL, material_id,
                                    light_side, corner_uv);
            EMIT_SMOOTH_TEX_POLYGON(v3, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                                    light_side, corner_uv);
            _nj_draw_polygon_ += 4;
        }
        break;
    case NJD_MESHSET_N:
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            corner_count = *mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            v0 = WORK_VERTEX(mesh_cursor[0]);
            v1 = WORK_VERTEX(mesh_cursor[1]);
            v2 = WORK_VERTEX(mesh_cursor[2]);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            light_side = 0;
            if (facing > 0.0F) {
                if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                    light_side = BACK_FACE;
                } else {
                    mesh_cursor += corner_count;
                    corner_uv += corner_count;
                    continue;
                }
            }
            _nj_draw_polygon_ += corner_count;
            for (corner = 0; corner < corner_count - 1; corner++) {
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_SMOOTH_TEX_POLYGON(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                        light_side, corner_uv);
            }
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_SMOOTH_TEX_POLYGON(v0, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                                    light_side, corner_uv);
        }
        break;
    case NJD_MESHSET_TRIMESH:
        strip_drawer = (material_flags & NJD_FLAG_DOUBLE_SIDE)
                           ? model_draw_smooth_tex_strips_all
                           : model_draw_smooth_tex_strips_cull;
        strip_drawer(meshset, material_id);
        break;
    }
}

static void model_draw_smooth_tex_polygons_clip(NJS_MESHSET *meshset,
                                                NJS_MATERIAL *material)
{
    Uint16 *mesh_cursor;
    struct model_vertex *v0;
    struct model_vertex *v1;
    struct model_vertex *v2;
    struct model_vertex *v3;
    NJS_COLOR *corner_uv;
    Uint16 material_id;
    Uint32 light_side;
    Uint32 clip_all;
    Uint32 clip_any;
    Sint32 polygon_index;
    Sint32 corner;
    Sint32 corner_count;
    Float facing;
    void (*strip_drawer)(NJS_MESHSET *, Uint32);

    material_id = meshset->type_matId & MATERIAL_ID;
    corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip;
            if (clip_all & CLIP_ALL) {
                corner_uv += 3;
                continue;
            }
            clip_any = v0->control.clip | v1->control.clip | v2->control.clip;
            if (!(clip_any & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        corner_uv += 3;
                        continue;
                    }
                }
                EMIT_SMOOTH_TEX_POLYGON(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                        light_side, corner_uv);
                EMIT_SMOOTH_TEX_POLYGON(v1, KM_VERTEXPARAM_NORMAL, material_id,
                                        light_side, corner_uv);
                EMIT_SMOOTH_TEX_POLYGON(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                                        material_id, light_side, corner_uv);
                _nj_draw_polygon_ += 3;
            } else {
                facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                         (v1->point.y * v0->point.z))) +
                         (v2->point.y * ((v1->point.x * v0->point.z) -
                                         (v1->point.z * v0->point.x))) +
                         (v2->point.z * ((v0->point.x * v1->point.y) -
                                         (v1->point.x * v0->point.y)));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        corner_uv += 3;
                        continue;
                    }
                }
                STORE_CORNER_UV(v0, corner_uv);
                STORE_CORNER_UV(v1, corner_uv);
                STORE_CORNER_UV(v2, corner_uv);
                model_clip_smooth_tex_triangle(v0, v1, v2,
                                               material_id | light_side);
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip &
                       v3->control.clip;
            if (clip_all & CLIP_ALL) {
                corner_uv += 4;
                continue;
            }
            clip_any = v0->control.clip | v1->control.clip | v2->control.clip |
                       v3->control.clip;
            if (!(clip_any & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        corner_uv += 4;
                        continue;
                    }
                }
                EMIT_SMOOTH_TEX_POLYGON(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                        light_side, corner_uv);
                EMIT_SMOOTH_TEX_POLYGON(v1, KM_VERTEXPARAM_NORMAL, material_id,
                                        light_side, corner_uv);
                EMIT_SMOOTH_TEX_POLYGON(v2, KM_VERTEXPARAM_NORMAL, material_id,
                                        light_side, corner_uv);
                EMIT_SMOOTH_TEX_POLYGON(v3, KM_VERTEXPARAM_ENDOFSTRIP,
                                        material_id, light_side, corner_uv);
                _nj_draw_polygon_ += 4;
            } else {
                facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                         (v1->point.y * v0->point.z))) +
                         (v2->point.y * ((v1->point.x * v0->point.z) -
                                         (v1->point.z * v0->point.x))) +
                         (v2->point.z * ((v0->point.x * v1->point.y) -
                                         (v1->point.x * v0->point.y)));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        corner_uv += 4;
                        continue;
                    }
                }
                STORE_CORNER_UV(v0, corner_uv);
                STORE_CORNER_UV(v1, corner_uv);
                STORE_CORNER_UV(v2, corner_uv);
                STORE_CORNER_UV(v3, corner_uv);
                clip_all =
                    v0->control.clip & v1->control.clip & v2->control.clip;
                if (!(clip_all & CLIP_NEAR)) {
                    model_clip_smooth_tex_triangle(v0, v1, v2,
                                                   material_id | light_side);
                }
                clip_all =
                    v1->control.clip & v2->control.clip & v3->control.clip;
                if (!(clip_all & CLIP_NEAR)) {
                    model_clip_smooth_tex_triangle(v1, v2, v3,
                                                   material_id | light_side);
                }
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            corner_count = *mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            clip_all = ~(Uint32)0;
            clip_any = 0;
            for (corner = 0; corner < corner_count; corner++) {
                clip_all &= WORK_VERTEX(mesh_cursor[corner])->control.clip;
                clip_any |= WORK_VERTEX(mesh_cursor[corner])->control.clip;
            }
            if (clip_all & CLIP_ALL) {
                corner_uv += corner_count;

                mesh_cursor += corner_count;
                continue;
            }
            if (!(clip_any & CLIP_NEAR)) {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        mesh_cursor += corner_count;
                        corner_uv += corner_count;
                        continue;
                    }
                }
                _nj_draw_polygon_ += corner_count;
                for (corner = 0; corner < corner_count - 1; corner++) {
                    v0 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_SMOOTH_TEX_POLYGON(v0, KM_VERTEXPARAM_NORMAL,
                                            material_id, light_side, corner_uv);
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_SMOOTH_TEX_POLYGON(v0, KM_VERTEXPARAM_ENDOFSTRIP,
                                        material_id, light_side, corner_uv);
            } else {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);
                facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                         (v1->point.y * v0->point.z))) +
                         (v2->point.y * ((v1->point.x * v0->point.z) -
                                         (v1->point.z * v0->point.x))) +
                         (v2->point.z * ((v0->point.x * v1->point.y) -
                                         (v1->point.x * v0->point.y)));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        mesh_cursor += corner_count;
                        corner_uv += corner_count;
                        continue;
                    }
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                v1 = WORK_VERTEX(*mesh_cursor++);
                STORE_CORNER_UV(v0, corner_uv);
                STORE_CORNER_UV(v1, corner_uv);
                for (corner = 0; corner < corner_count - 2; corner++) {
                    v2 = WORK_VERTEX(*mesh_cursor++);
                    STORE_CORNER_UV(v2, corner_uv);
                    clip_all =
                        v0->control.clip & v1->control.clip & v2->control.clip;
                    if (!(clip_all & CLIP_NEAR)) {
                        model_clip_smooth_tex_triangle(
                            v0, v1, v2, material_id | light_side);
                    }
                    v0 = v1;
                    v1 = v2;
                }
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        strip_drawer = (model_clip_or & CLIP_NEAR)
                           ? model_draw_smooth_tex_strips_near
                           : model_draw_smooth_tex_strips_checked;
        strip_drawer(meshset, material_id);
        break;
    }
}

static void model_draw_smooth_env_polygons(NJS_MESHSET *meshset,
                                           NJS_MATERIAL *material)
{
    Uint16 *mesh_cursor;
    struct model_vertex *v0;
    struct model_vertex *v1;
    struct model_vertex *v2;
    struct model_vertex *v3;
    Uint16 material_id;
    Uint16 mesh_type;
    Uint32 light_side;
    Sint32 polygon_index;
    Sint32 corner;
    Sint32 corner_count;
    Float facing;
    void (*strip_drawer)(NJS_MESHSET *, Uint32);

    mesh_type = meshset->type_matId;
    material_id = (Uint16)(mesh_type & MATERIAL_ID);
    mesh_cursor = (Uint16 *)meshset->meshes;
    switch (mesh_type & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            light_side = 0;
            if (facing > 0.0F) {
                if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                    light_side = BACK_FACE;
                } else {
                    continue;
                }
            }
            EMIT_SMOOTH_ENV(v0, KM_VERTEXPARAM_NORMAL, material_id, light_side);
            EMIT_SMOOTH_ENV(v1, KM_VERTEXPARAM_NORMAL, material_id, light_side);
            EMIT_SMOOTH_ENV(v2, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                            light_side);
            _nj_draw_polygon_ += 3;
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            light_side = 0;
            if (facing > 0.0F) {
                if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                    light_side = BACK_FACE;
                } else {
                    continue;
                }
            }
            EMIT_SMOOTH_ENV(v0, KM_VERTEXPARAM_NORMAL, material_id, light_side);
            EMIT_SMOOTH_ENV(v1, KM_VERTEXPARAM_NORMAL, material_id, light_side);
            EMIT_SMOOTH_ENV(v2, KM_VERTEXPARAM_NORMAL, material_id, light_side);
            EMIT_SMOOTH_ENV(v3, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                            light_side);
            _nj_draw_polygon_ += 4;
        }
        break;
    case NJD_MESHSET_N:
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            corner_count = *mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            v0 = WORK_VERTEX(mesh_cursor[0]);
            v1 = WORK_VERTEX(mesh_cursor[1]);
            v2 = WORK_VERTEX(mesh_cursor[2]);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            light_side = 0;
            if (facing > 0.0F) {
                if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                    light_side = BACK_FACE;
                } else {
                    mesh_cursor += corner_count;
                    continue;
                }
            }
            _nj_draw_polygon_ += corner_count;
            for (corner = 0; corner < corner_count - 1; corner++) {
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_SMOOTH_ENV(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                light_side);
            }
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_SMOOTH_ENV(v0, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                            light_side);
        }
        break;
    case NJD_MESHSET_TRIMESH:
        strip_drawer = (material_flags & NJD_FLAG_DOUBLE_SIDE)
                           ? model_draw_smooth_env_strips_all
                           : model_draw_smooth_env_strips_cull;
        strip_drawer(meshset, material_id);
        break;
    }
}

static void model_draw_smooth_env_polygons_clip(NJS_MESHSET *meshset,
                                                NJS_MATERIAL *material)
{
    Uint16 *mesh_cursor;
    struct model_vertex *v0;
    struct model_vertex *v1;
    struct model_vertex *v2;
    struct model_vertex *v3;
    Uint16 material_id;
    Uint32 light_side;
    Uint32 clip_all;
    Uint32 clip_any;
    Sint32 polygon_index;
    Sint32 corner;
    Sint32 corner_count;
    Float facing;
    void (*strip_drawer)(NJS_MESHSET *, Uint32);

    material_id = meshset->type_matId & MATERIAL_ID;
    mesh_cursor = (Uint16 *)meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip;
            if (clip_all & CLIP_ALL) {
                continue;
            }
            clip_any = v0->control.clip | v1->control.clip | v2->control.clip;
            if (!(clip_any & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        continue;
                    }
                }
                EMIT_SMOOTH_ENV(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                light_side);
                EMIT_SMOOTH_ENV(v1, KM_VERTEXPARAM_NORMAL, material_id,
                                light_side);
                EMIT_SMOOTH_ENV(v2, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                                light_side);
                _nj_draw_polygon_ += 3;
            } else {
                facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                         (v1->point.y * v0->point.z))) +
                         (v2->point.y * ((v1->point.x * v0->point.z) -
                                         (v1->point.z * v0->point.x))) +
                         (v2->point.z * ((v0->point.x * v1->point.y) -
                                         (v1->point.x * v0->point.y)));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        continue;
                    }
                }
                model_clip_smooth_env_triangle(v0, v1, v2,
                                               material_id | light_side);
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip &
                       v3->control.clip;
            if (clip_all & CLIP_ALL) {
                continue;
            }
            clip_any = v0->control.clip | v1->control.clip | v2->control.clip |
                       v3->control.clip;
            if (!(clip_any & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        continue;
                    }
                }
                EMIT_SMOOTH_ENV(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                light_side);
                EMIT_SMOOTH_ENV(v1, KM_VERTEXPARAM_NORMAL, material_id,
                                light_side);
                EMIT_SMOOTH_ENV(v2, KM_VERTEXPARAM_NORMAL, material_id,
                                light_side);
                EMIT_SMOOTH_ENV(v3, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                                light_side);
                _nj_draw_polygon_ += 4;
            } else {
                facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                         (v1->point.y * v0->point.z))) +
                         (v2->point.y * ((v1->point.x * v0->point.z) -
                                         (v1->point.z * v0->point.x))) +
                         (v2->point.z * ((v0->point.x * v1->point.y) -
                                         (v1->point.x * v0->point.y)));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        continue;
                    }
                }
                clip_all =
                    v0->control.clip & v1->control.clip & v2->control.clip;
                if (!(clip_all & CLIP_NEAR)) {
                    model_clip_smooth_env_triangle(v0, v1, v2,
                                                   material_id | light_side);
                }
                clip_all =
                    v1->control.clip & v2->control.clip & v3->control.clip;
                if (!(clip_all & CLIP_NEAR)) {
                    model_clip_smooth_env_triangle(v1, v2, v3,
                                                   material_id | light_side);
                }
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            corner_count = *mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            clip_all = ~(Uint32)0;
            clip_any = 0;
            for (corner = 0; corner < corner_count; corner++) {
                clip_all &= WORK_VERTEX(mesh_cursor[corner])->control.clip;
                clip_any |= WORK_VERTEX(mesh_cursor[corner])->control.clip;
            }
            if (clip_all & CLIP_ALL) {
                mesh_cursor += corner_count;
                continue;
            }
            if (!(clip_any & CLIP_NEAR)) {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        mesh_cursor += corner_count;
                        continue;
                    }
                }
                _nj_draw_polygon_ += corner_count;
                for (corner = 0; corner < corner_count - 1; corner++) {
                    v0 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_SMOOTH_ENV(v0, KM_VERTEXPARAM_NORMAL, material_id,
                                    light_side);
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_SMOOTH_ENV(v0, KM_VERTEXPARAM_ENDOFSTRIP, material_id,
                                light_side);
            } else {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);
                facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                         (v1->point.y * v0->point.z))) +
                         (v2->point.y * ((v1->point.x * v0->point.z) -
                                         (v1->point.z * v0->point.x))) +
                         (v2->point.z * ((v0->point.x * v1->point.y) -
                                         (v1->point.x * v0->point.y)));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        mesh_cursor += corner_count;
                        continue;
                    }
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                v1 = WORK_VERTEX(*mesh_cursor++);
                for (corner = 0; corner < corner_count - 2; corner++) {
                    v2 = WORK_VERTEX(*mesh_cursor++);
                    clip_all =
                        v0->control.clip & v1->control.clip & v2->control.clip;
                    if (!(clip_all & CLIP_NEAR)) {
                        model_clip_smooth_env_triangle(
                            v0, v1, v2, material_id | light_side);
                    }
                    v0 = v1;
                    v1 = v2;
                }
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        strip_drawer = (model_clip_or & CLIP_NEAR)
                           ? model_draw_smooth_env_strips_near
                           : model_draw_smooth_env_strips_checked;
        strip_drawer(meshset, material_id);
        break;
    }
}

/* set face_normal to the unit normal of the plane through the view points of
 * p0, p1 and p2 */
#define COMPUTE_FLAT_NORMAL(p0, p1, p2, face_normal)                           \
    {                                                                          \
        njGetPlaneNormal2(&(p0)->point, &(p1)->point, &(p2)->point,            \
                          &(face_normal));                                     \
        njUnitVector(&(face_normal));                                          \
    }

/* light light_point with face_normal from light_side into _njm_col2, and
 * pack both colors into flat_colors */
#define LIGHT_FLAT_POLYGON(light_point, face_normal, light_side, flat_colors)  \
    {                                                                          \
        njCalculateLightsRGB(_njm_col2, &(light_point), &(face_normal),        \
                             light_side);                                      \
        (flat_colors)[0] = PACK_ARGB(_njm_col2[0]);                            \
        (flat_colors)[1] = PACK_ARGB(_njm_col2[1]);                            \
    }

/* add the specular red, green and blue in _njm_col2 into the diffuse */
#define SUM_FLAT_LIGHT()                                                       \
    {                                                                          \
        _njm_col2[0].r = _njm_col2[0].r + _njm_col2[1].r;                      \
        _njm_col2[0].g = _njm_col2[0].g + _njm_col2[1].g;                      \
        _njm_col2[0].b = _njm_col2[0].b + _njm_col2[1].b;                      \
    }

/* send vertex in _njm_col2's diffuse color, as floats */
#define EMIT_FLAT_COLOR(vertex, packet_command)                                \
    {                                                                          \
        (vertex)->payload.col.a = _njm_col2[0].a;                              \
        (vertex)->payload.col.r = _njm_col2[0].r;                              \
        (vertex)->payload.col.g = _njm_col2[0].g;                              \
        (vertex)->payload.col.b = _njm_col2[0].b;                              \
        (vertex)->control.cmd = (packet_command);                              \
        submit_work_vertex(vertex);                                            \
    }

/* send vertex with corner_uv's texture coordinates, leaving its colors as
 * they are, and advance corner_uv. scratch: uv_scale */
#define EMIT_FLAT_TEX(vertex, packet_command, corner_uv)                       \
    {                                                                          \
        (vertex)->control.cmd = (packet_command);                              \
        uv_scale = 1.0F / 256.0F;                                              \
        (vertex)->payload.tex.uv[0] = (corner_uv)->tex.u * uv_scale;           \
        (vertex)->payload.tex.uv[1] = (corner_uv)->tex.v * uv_scale;           \
        (corner_uv)++;                                                         \
        submit_work_vertex(vertex);                                            \
    }

/* EMIT_FLAT_TEX, in the packed colors at flat_colors. scratch: uv_scale */
#define EMIT_FLAT_TEX_COLOR(vertex, packet_command, flat_colors, corner_uv)    \
    {                                                                          \
        (vertex)->control.cmd = (packet_command);                              \
        PACKED_COLOR(vertex, 0) = (flat_colors)[0];                            \
        PACKED_COLOR(vertex, 1) = (flat_colors)[1];                            \
        uv_scale = 1.0F / 256.0F;                                              \
        (vertex)->payload.tex.uv[0] = (corner_uv)->tex.u * uv_scale;           \
        (vertex)->payload.tex.uv[1] = (corner_uv)->tex.v * uv_scale;           \
        (corner_uv)++;                                                         \
        submit_work_vertex(vertex);                                            \
    }

/* light a triangle at its corners' mean, their sum times 0.33333F, a quad at
 * the midpoint of corners 1 and 2, and a polygon at the midpoint of corners
 * count / 2 - 1 and count / 2. every corner takes the diffuse alpha and the
 * diffuse plus specular red, green and blue */
static void model_draw_flat_polygons(NJS_MESHSET *meshset,
                                     NJS_MATERIAL *material)
{
    Uint16 *mesh_cursor;
    struct model_vertex *v0;
    struct model_vertex *v1;
    struct model_vertex *v2;
    struct model_vertex *v3;
    Uint16 material_id;
    Uint32 light_side;
    Sint32 polygon_index;
    Sint32 corner;
    Sint32 corner_count;
    Float facing;
    NJS_POINT3 light_point;
    NJS_VECTOR face_normal;
    /* LIGHT_FLAT_POLYGON packs both colors here, as at 0x8c64b720 and
     * 0x8c64b73e, but this drawer sends _njm_col2's floats and never reads
     * them */
    Uint32 flat_colors[2];
    void (*strip_drawer)(NJS_MESHSET *, Uint32);

    mesh_cursor = (Uint16 *)meshset->meshes;
    material_id = meshset->type_matId & MATERIAL_ID;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            light_side = 0;
            if (facing > 0.0F) {
                if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                    light_side = BACK_FACE;
                } else {
                    continue;
                }
            }
            COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
            light_point.x =
                (v0->point.x + v1->point.x + v2->point.x) * 0.33333F;
            light_point.y =
                (v0->point.y + v1->point.y + v2->point.y) * 0.33333F;
            light_point.z =
                (v0->point.z + v1->point.z + v2->point.z) * 0.33333F;
            LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                               flat_colors);
            SUM_FLAT_LIGHT();
            EMIT_FLAT_COLOR(v0, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_COLOR(v1, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_COLOR(v2, KM_VERTEXPARAM_ENDOFSTRIP);
            _nj_draw_polygon_ += 3;
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            light_side = 0;
            if (facing > 0.0F) {
                if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                    light_side = BACK_FACE;
                } else {
                    continue;
                }
            }
            COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
            light_point.x = (v1->point.x + v2->point.x) * 0.5F;
            light_point.y = (v1->point.y + v2->point.y) * 0.5F;
            light_point.z = (v1->point.z + v2->point.z) * 0.5F;
            LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                               flat_colors);
            SUM_FLAT_LIGHT();
            EMIT_FLAT_COLOR(v0, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_COLOR(v1, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_COLOR(v2, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_COLOR(v3, KM_VERTEXPARAM_ENDOFSTRIP);
            _nj_draw_polygon_ += 4;
        }
        break;
    case NJD_MESHSET_N:
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            corner_count = *mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            v0 = WORK_VERTEX(mesh_cursor[0]);
            v1 = WORK_VERTEX(mesh_cursor[1]);
            v2 = WORK_VERTEX(mesh_cursor[2]);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            light_side = 0;
            if (facing > 0.0F) {
                if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                    light_side = BACK_FACE;
                } else {
                    mesh_cursor += corner_count;
                    continue;
                }
            }
            _nj_draw_polygon_ += corner_count;
            COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
            v3 = WORK_VERTEX(mesh_cursor[(corner_count / 2) - 1]);
            light_point.x = v3->point.x;
            light_point.y = v3->point.y;
            light_point.z = v3->point.z;
            v3 = WORK_VERTEX(mesh_cursor[corner_count / 2]);
            light_point.x += v3->point.x;
            light_point.y += v3->point.y;
            light_point.z += v3->point.z;
            light_point.x *= 0.5F;
            light_point.y *= 0.5F;
            light_point.z *= 0.5F;
            LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                               flat_colors);
            SUM_FLAT_LIGHT();
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_COLOR(v0, KM_VERTEXPARAM_NORMAL);
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_COLOR(v0, KM_VERTEXPARAM_NORMAL);
            for (corner = 2; corner < corner_count - 1; corner++) {
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_COLOR(v0, KM_VERTEXPARAM_NORMAL);
            }
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_COLOR(v0, KM_VERTEXPARAM_ENDOFSTRIP);
        }
        break;
    case NJD_MESHSET_TRIMESH:
        strip_drawer = (material_flags & NJD_FLAG_DOUBLE_SIDE)
                           ? model_draw_flat_path_strips_all
                           : model_draw_flat_path_strips_cull;
        strip_drawer(meshset, material_id);
        break;
    }
}

static void model_draw_flat_polygons_clip(NJS_MESHSET *meshset,
                                          NJS_MATERIAL *material)
{
    Uint16 *mesh_cursor;
    struct model_vertex *v0;
    struct model_vertex *v1;
    struct model_vertex *v2;
    struct model_vertex *v3;
    Uint16 material_id;
    Uint32 light_side;
    Uint32 clip_all;
    Uint32 clip_any;
    Sint32 polygon_index;
    Sint32 corner;
    Sint32 corner_count;
    Float facing;
    NJS_POINT3 light_point;
    NJS_VECTOR face_normal;
    /* LIGHT_FLAT_POLYGON packs both colors here, as at 0x8c64c064 and
     * 0x8c64c082, but this drawer sends _njm_col2's floats and never reads
     * them */
    Uint32 flat_colors[2];
    void (*strip_drawer)(NJS_MESHSET *, Uint32);

    mesh_cursor = (Uint16 *)meshset->meshes;
    material_id = meshset->type_matId & MATERIAL_ID;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip;
            if (clip_all & CLIP_ALL) {
                continue;
            }
            clip_any = v0->control.clip | v1->control.clip | v2->control.clip;
            if (!(clip_any & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        continue;
                    }
                }
                COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
                light_point.x =
                    (v0->point.x + v1->point.x + v2->point.x) * 0.33333F;
                light_point.y =
                    (v0->point.y + v1->point.y + v2->point.y) * 0.33333F;
                light_point.z =
                    (v0->point.z + v1->point.z + v2->point.z) * 0.33333F;
                LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                                   flat_colors);
                SUM_FLAT_LIGHT();
                EMIT_FLAT_COLOR(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_COLOR(v1, KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_COLOR(v2, KM_VERTEXPARAM_ENDOFSTRIP);
                _nj_draw_polygon_ += 3;
            } else {
                facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                         (v1->point.y * v0->point.z))) +
                         (v2->point.y * ((v1->point.x * v0->point.z) -
                                         (v1->point.z * v0->point.x))) +
                         (v2->point.z * ((v0->point.x * v1->point.y) -
                                         (v1->point.x * v0->point.y)));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        continue;
                    }
                }
                COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
                light_point.x =
                    (v0->point.x + v1->point.x + v2->point.x) * 0.33333F;
                light_point.y =
                    (v0->point.y + v1->point.y + v2->point.y) * 0.33333F;
                light_point.z =
                    (v0->point.z + v1->point.z + v2->point.z) * 0.33333F;
                LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                                   flat_colors);
                SUM_FLAT_LIGHT();
                model_clip_constant_color_triangle(v0, v1, v2, &_njm_col2[0]);
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip &
                       v3->control.clip;
            if (clip_all & CLIP_ALL) {
                continue;
            }
            clip_any = v0->control.clip | v1->control.clip | v2->control.clip |
                       v3->control.clip;
            if (!(clip_any & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        continue;
                    }
                }
                COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
                light_point.x = (v1->point.x + v2->point.x) * 0.5F;
                light_point.y = (v1->point.y + v2->point.y) * 0.5F;
                light_point.z = (v1->point.z + v2->point.z) * 0.5F;
                LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                                   flat_colors);
                SUM_FLAT_LIGHT();
                EMIT_FLAT_COLOR(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_COLOR(v1, KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_COLOR(v2, KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_COLOR(v3, KM_VERTEXPARAM_ENDOFSTRIP);
                _nj_draw_polygon_ += 4;
            } else {
                facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                         (v1->point.y * v0->point.z))) +
                         (v2->point.y * ((v1->point.x * v0->point.z) -
                                         (v1->point.z * v0->point.x))) +
                         (v2->point.z * ((v0->point.x * v1->point.y) -
                                         (v1->point.x * v0->point.y)));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        continue;
                    }
                }
                COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
                light_point.x = (v1->point.x + v2->point.x) * 0.5F;
                light_point.y = (v1->point.y + v2->point.y) * 0.5F;
                light_point.z = (v1->point.z + v2->point.z) * 0.5F;
                LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                                   flat_colors);
                SUM_FLAT_LIGHT();
                clip_all =
                    v0->control.clip & v1->control.clip & v2->control.clip;
                if (!(clip_all & CLIP_NEAR)) {
                    model_clip_constant_color_triangle(v0, v1, v2,
                                                       &_njm_col2[0]);
                }
                clip_all =
                    v1->control.clip & v2->control.clip & v3->control.clip;
                if (!(clip_all & CLIP_NEAR)) {
                    model_clip_constant_color_triangle(v1, v2, v3,
                                                       &_njm_col2[0]);
                }
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            corner_count = *mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            clip_all = ~(Uint32)0;
            clip_any = 0;
            for (corner = 0; corner < corner_count; corner++) {
                clip_all &= WORK_VERTEX(mesh_cursor[corner])->control.clip;
                clip_any |= WORK_VERTEX(mesh_cursor[corner])->control.clip;
            }
            if (clip_all & CLIP_ALL) {
                mesh_cursor += corner_count;
                continue;
            }
            if (!(clip_any & CLIP_NEAR)) {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        mesh_cursor += corner_count;
                        continue;
                    }
                }
                _nj_draw_polygon_ += corner_count;
                COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
                v3 = WORK_VERTEX(mesh_cursor[(corner_count / 2) - 1]);
                light_point.x = v3->point.x;
                light_point.y = v3->point.y;
                light_point.z = v3->point.z;
                v3 = WORK_VERTEX(mesh_cursor[corner_count / 2]);
                light_point.x += v3->point.x;
                light_point.y += v3->point.y;
                light_point.z += v3->point.z;
                light_point.x *= 0.5F;
                light_point.y *= 0.5F;
                light_point.z *= 0.5F;
                LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                                   flat_colors);
                SUM_FLAT_LIGHT();
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_COLOR(v0, KM_VERTEXPARAM_NORMAL);
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_COLOR(v0, KM_VERTEXPARAM_NORMAL);
                for (corner = 2; corner < corner_count - 1; corner++) {
                    v0 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_FLAT_COLOR(v0, KM_VERTEXPARAM_NORMAL);
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_COLOR(v0, KM_VERTEXPARAM_ENDOFSTRIP);
            } else {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);
                facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                         (v1->point.y * v0->point.z))) +
                         (v2->point.y * ((v1->point.x * v0->point.z) -
                                         (v1->point.z * v0->point.x))) +
                         (v2->point.z * ((v0->point.x * v1->point.y) -
                                         (v1->point.x * v0->point.y)));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        mesh_cursor += corner_count;
                        continue;
                    }
                }
                COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
                v3 = WORK_VERTEX(mesh_cursor[(corner_count / 2) - 1]);
                light_point.x = v3->point.x;
                light_point.y = v3->point.y;
                light_point.z = v3->point.z;
                v3 = WORK_VERTEX(mesh_cursor[corner_count / 2]);
                light_point.x += v3->point.x;
                light_point.y += v3->point.y;
                light_point.z += v3->point.z;
                light_point.x *= 0.5F;
                light_point.y *= 0.5F;
                light_point.z *= 0.5F;
                LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                                   flat_colors);
                SUM_FLAT_LIGHT();
                v0 = WORK_VERTEX(*mesh_cursor++);
                v1 = WORK_VERTEX(*mesh_cursor++);
                for (corner = 0; corner < corner_count - 2; corner++) {
                    v2 = WORK_VERTEX(*mesh_cursor++);
                    clip_all =
                        v0->control.clip & v1->control.clip & v2->control.clip;
                    if (!(clip_all & CLIP_NEAR)) {
                        model_clip_constant_color_triangle(v0, v1, v2,
                                                           &_njm_col2[0]);
                    }
                    v0 = v1;
                    v1 = v2;
                }
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        strip_drawer = (model_clip_or & CLIP_NEAR)
                           ? model_draw_flat_path_strips_near
                           : model_draw_flat_path_strips_checked;
        strip_drawer(meshset, material_id);
        break;
    }
}

/* light each polygon at the point and by the normal that
 * model_draw_flat_polygons uses, and send the packed diffuse and specular as
 * its base and offset colors. without Gouraud shading a triangle takes the
 * color of the corner that completes it, so only the corners from the third on
 * carry the colors */
static void model_draw_flat_tex_polygons(NJS_MESHSET *meshset,
                                         NJS_MATERIAL *material)
{
    Uint16 *mesh_cursor;
    NJS_COLOR *corner_uv;
    struct model_vertex *v0;
    struct model_vertex *v1;
    struct model_vertex *v2;
    struct model_vertex *v3;
    Uint16 material_id;
    Uint32 light_side;
    Sint32 polygon_index;
    Sint32 corner;
    Sint32 corner_count;
    Float facing;
    Float uv_scale;
    NJS_POINT3 light_point;
    NJS_VECTOR face_normal;
    Uint32 flat_colors[2];
    void (*strip_drawer)(NJS_MESHSET *, Uint32);

    material_id = meshset->type_matId & MATERIAL_ID;
    corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            light_side = 0;
            if (facing > 0.0F) {
                if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                    light_side = BACK_FACE;
                } else {
                    corner_uv += 3;
                    continue;
                }
            }
            COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
            light_point.x =
                (v0->point.x + v1->point.x + v2->point.x) * 0.33333F;
            light_point.y =
                (v0->point.y + v1->point.y + v2->point.y) * 0.33333F;
            light_point.z =
                (v0->point.z + v1->point.z + v2->point.z) * 0.33333F;
            LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                               flat_colors);
            EMIT_FLAT_TEX(v0, KM_VERTEXPARAM_NORMAL, corner_uv);
            EMIT_FLAT_TEX(v1, KM_VERTEXPARAM_NORMAL, corner_uv);
            EMIT_FLAT_TEX_COLOR(v2, KM_VERTEXPARAM_ENDOFSTRIP, flat_colors,
                                corner_uv);
            _nj_draw_polygon_ += 3;
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            light_side = 0;
            if (facing > 0.0F) {
                if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                    light_side = BACK_FACE;
                } else {
                    corner_uv += 4;
                    continue;
                }
            }
            COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
            light_point.x = (v1->point.x + v2->point.x) * 0.5F;
            light_point.y = (v1->point.y + v2->point.y) * 0.5F;
            light_point.z = (v1->point.z + v2->point.z) * 0.5F;
            LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                               flat_colors);
            EMIT_FLAT_TEX(v0, KM_VERTEXPARAM_NORMAL, corner_uv);
            EMIT_FLAT_TEX(v1, KM_VERTEXPARAM_NORMAL, corner_uv);
            EMIT_FLAT_TEX_COLOR(v2, KM_VERTEXPARAM_NORMAL, flat_colors,
                                corner_uv);
            EMIT_FLAT_TEX_COLOR(v3, KM_VERTEXPARAM_ENDOFSTRIP, flat_colors,
                                corner_uv);
            _nj_draw_polygon_ += 4;
        }
        break;
    case NJD_MESHSET_N:
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            corner_count = *mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            v0 = WORK_VERTEX(mesh_cursor[0]);
            v1 = WORK_VERTEX(mesh_cursor[1]);
            v2 = WORK_VERTEX(mesh_cursor[2]);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            light_side = 0;
            if (facing > 0.0F) {
                if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                    light_side = BACK_FACE;
                } else {
                    mesh_cursor += corner_count;
                    corner_uv += corner_count;
                    continue;
                }
            }
            _nj_draw_polygon_ += corner_count;
            COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
            v3 = WORK_VERTEX(mesh_cursor[(corner_count / 2) - 1]);
            light_point.x = v3->point.x;
            light_point.y = v3->point.y;
            light_point.z = v3->point.z;
            v3 = WORK_VERTEX(mesh_cursor[corner_count / 2]);
            light_point.x += v3->point.x;
            light_point.y += v3->point.y;
            light_point.z += v3->point.z;
            light_point.x *= 0.5F;
            light_point.y *= 0.5F;
            light_point.z *= 0.5F;
            LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                               flat_colors);
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_TEX(v0, KM_VERTEXPARAM_NORMAL, corner_uv);
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_TEX(v0, KM_VERTEXPARAM_NORMAL, corner_uv);
            for (corner = 2; corner < corner_count - 1; corner++) {
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_TEX_COLOR(v0, KM_VERTEXPARAM_NORMAL, flat_colors,
                                    corner_uv);
            }
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_TEX_COLOR(v0, KM_VERTEXPARAM_ENDOFSTRIP, flat_colors,
                                corner_uv);
        }
        break;
    case NJD_MESHSET_TRIMESH:
        strip_drawer = (material_flags & NJD_FLAG_DOUBLE_SIDE)
                           ? model_draw_flat_path_tex_strips_all
                           : model_draw_flat_path_tex_strips_cull;
        strip_drawer(meshset, material_id);
        break;
    }
}

static void model_draw_flat_tex_polygons_clip(NJS_MESHSET *meshset,
                                              NJS_MATERIAL *material)
{
    Uint16 *mesh_cursor;
    NJS_COLOR *corner_uv;
    struct model_vertex *v0;
    struct model_vertex *v1;
    struct model_vertex *v2;
    struct model_vertex *v3;
    Uint16 material_id;
    Uint32 light_side;
    Uint32 clip_all;
    Uint32 clip_any;
    Sint32 polygon_index;
    Sint32 corner;
    Sint32 corner_count;
    Float facing;
    Float uv_scale;
    NJS_POINT3 light_point;
    NJS_VECTOR face_normal;
    Uint32 flat_colors[2];
    void (*strip_drawer)(NJS_MESHSET *, Uint32);

    material_id = meshset->type_matId & MATERIAL_ID;
    corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip;
            if (clip_all & CLIP_ALL) {
                corner_uv += 3;
                continue;
            }
            clip_any = v0->control.clip | v1->control.clip | v2->control.clip;
            if (!(clip_any & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        corner_uv += 3;
                        continue;
                    }
                }
                COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
                light_point.x =
                    (v0->point.x + v1->point.x + v2->point.x) * 0.33333F;
                light_point.y =
                    (v0->point.y + v1->point.y + v2->point.y) * 0.33333F;
                light_point.z =
                    (v0->point.z + v1->point.z + v2->point.z) * 0.33333F;
                LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                                   flat_colors);
                EMIT_FLAT_TEX(v0, KM_VERTEXPARAM_NORMAL, corner_uv);
                EMIT_FLAT_TEX(v1, KM_VERTEXPARAM_NORMAL, corner_uv);
                EMIT_FLAT_TEX_COLOR(v2, KM_VERTEXPARAM_ENDOFSTRIP, flat_colors,
                                    corner_uv);
                _nj_draw_polygon_ += 3;
            } else {
                facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                         (v1->point.y * v0->point.z))) +
                         (v2->point.y * ((v1->point.x * v0->point.z) -
                                         (v1->point.z * v0->point.x))) +
                         (v2->point.z * ((v0->point.x * v1->point.y) -
                                         (v1->point.x * v0->point.y)));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        corner_uv += 3;
                        continue;
                    }
                }
                COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
                light_point.x =
                    (v0->point.x + v1->point.x + v2->point.x) * 0.33333F;
                light_point.y =
                    (v0->point.y + v1->point.y + v2->point.y) * 0.33333F;
                light_point.z =
                    (v0->point.z + v1->point.z + v2->point.z) * 0.33333F;
                LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                                   flat_colors);
                STORE_CORNER_UV(v0, corner_uv);
                STORE_CORNER_UV(v1, corner_uv);
                STORE_CORNER_UV(v2, corner_uv);
                model_clip_constant_tex_triangle(v0, v1, v2, flat_colors);
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip &
                       v3->control.clip;
            if (clip_all & CLIP_ALL) {
                corner_uv += 4;
                continue;
            }
            clip_any = v0->control.clip | v1->control.clip | v2->control.clip |
                       v3->control.clip;
            if (!(clip_any & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        corner_uv += 4;
                        continue;
                    }
                }
                COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
                light_point.x = (v1->point.x + v2->point.x) * 0.5F;
                light_point.y = (v1->point.y + v2->point.y) * 0.5F;
                light_point.z = (v1->point.z + v2->point.z) * 0.5F;
                LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                                   flat_colors);
                EMIT_FLAT_TEX(v0, KM_VERTEXPARAM_NORMAL, corner_uv);
                EMIT_FLAT_TEX(v1, KM_VERTEXPARAM_NORMAL, corner_uv);
                EMIT_FLAT_TEX_COLOR(v2, KM_VERTEXPARAM_NORMAL, flat_colors,
                                    corner_uv);
                EMIT_FLAT_TEX_COLOR(v3, KM_VERTEXPARAM_ENDOFSTRIP, flat_colors,
                                    corner_uv);
                _nj_draw_polygon_ += 4;
            } else {
                facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                         (v1->point.y * v0->point.z))) +
                         (v2->point.y * ((v1->point.x * v0->point.z) -
                                         (v1->point.z * v0->point.x))) +
                         (v2->point.z * ((v0->point.x * v1->point.y) -
                                         (v1->point.x * v0->point.y)));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        corner_uv += 4;
                        continue;
                    }
                }
                COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
                light_point.x = (v1->point.x + v2->point.x) * 0.5F;
                light_point.y = (v1->point.y + v2->point.y) * 0.5F;
                light_point.z = (v1->point.z + v2->point.z) * 0.5F;
                LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                                   flat_colors);
                STORE_CORNER_UV(v0, corner_uv);
                STORE_CORNER_UV(v1, corner_uv);
                STORE_CORNER_UV(v2, corner_uv);
                STORE_CORNER_UV(v3, corner_uv);
                clip_all =
                    v0->control.clip & v1->control.clip & v2->control.clip;
                if (!(clip_all & CLIP_NEAR)) {
                    model_clip_constant_tex_triangle(v0, v1, v2, flat_colors);
                }
                clip_all =
                    v1->control.clip & v2->control.clip & v3->control.clip;
                if (!(clip_all & CLIP_NEAR)) {
                    model_clip_constant_tex_triangle(v1, v2, v3, flat_colors);
                }
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            corner_count = *mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            clip_all = ~(Uint32)0;
            clip_any = 0;
            for (corner = 0; corner < corner_count; corner++) {
                clip_all &= WORK_VERTEX(mesh_cursor[corner])->control.clip;
                clip_any |= WORK_VERTEX(mesh_cursor[corner])->control.clip;
            }
            if (clip_all & CLIP_ALL) {
                corner_uv += corner_count;
                mesh_cursor += corner_count;
                continue;
            }
            if (!(clip_any & CLIP_NEAR)) {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        mesh_cursor += corner_count;
                        corner_uv += corner_count;
                        continue;
                    }
                }
                _nj_draw_polygon_ += corner_count;
                COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
                v3 = WORK_VERTEX(mesh_cursor[(corner_count / 2) - 1]);
                light_point.x = v3->point.x;
                light_point.y = v3->point.y;
                light_point.z = v3->point.z;
                v3 = WORK_VERTEX(mesh_cursor[corner_count / 2]);
                light_point.x += v3->point.x;
                light_point.y += v3->point.y;
                light_point.z += v3->point.z;
                light_point.x *= 0.5F;
                light_point.y *= 0.5F;
                light_point.z *= 0.5F;
                LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                                   flat_colors);
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_TEX(v0, KM_VERTEXPARAM_NORMAL, corner_uv);
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_TEX(v0, KM_VERTEXPARAM_NORMAL, corner_uv);
                for (corner = 2; corner < corner_count - 1; corner++) {
                    v0 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_FLAT_TEX_COLOR(v0, KM_VERTEXPARAM_NORMAL, flat_colors,
                                        corner_uv);
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_TEX_COLOR(v0, KM_VERTEXPARAM_ENDOFSTRIP, flat_colors,
                                    corner_uv);
            } else {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);
                facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                         (v1->point.y * v0->point.z))) +
                         (v2->point.y * ((v1->point.x * v0->point.z) -
                                         (v1->point.z * v0->point.x))) +
                         (v2->point.z * ((v0->point.x * v1->point.y) -
                                         (v1->point.x * v0->point.y)));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        mesh_cursor += corner_count;
                        corner_uv += corner_count;
                        continue;
                    }
                }
                COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
                v3 = WORK_VERTEX(mesh_cursor[(corner_count / 2) - 1]);
                light_point.x = v3->point.x;
                light_point.y = v3->point.y;
                light_point.z = v3->point.z;
                v3 = WORK_VERTEX(mesh_cursor[corner_count / 2]);
                light_point.x += v3->point.x;
                light_point.y += v3->point.y;
                light_point.z += v3->point.z;
                light_point.x *= 0.5F;
                light_point.y *= 0.5F;
                light_point.z *= 0.5F;
                LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                                   flat_colors);
                v0 = WORK_VERTEX(*mesh_cursor++);
                v1 = WORK_VERTEX(*mesh_cursor++);
                STORE_CORNER_UV(v0, corner_uv);
                STORE_CORNER_UV(v1, corner_uv);
                for (corner = 0; corner < corner_count - 2; corner++) {
                    v2 = WORK_VERTEX(*mesh_cursor++);
                    STORE_CORNER_UV(v2, corner_uv);
                    clip_all =
                        v0->control.clip & v1->control.clip & v2->control.clip;
                    if (!(clip_all & CLIP_NEAR)) {
                        model_clip_constant_tex_triangle(v0, v1, v2,
                                                         flat_colors);
                    }
                    v0 = v1;
                    v1 = v2;
                }
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        strip_drawer = (model_clip_or & CLIP_NEAR)
                           ? model_draw_flat_path_tex_strips_near
                           : model_draw_flat_path_tex_strips_checked;
        strip_drawer(meshset, material_id);
        break;
    }
}

/* send vertex in the material's diffuse color, packed, with corner_uv's texture
 * coordinates, and advance corner_uv */
#define EMIT_UNLIT_TEX_POLYGON(vertex, packet_command, corner_uv)              \
    {                                                                          \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        PACKED_COLOR(vertex, 0) = PACK_ARGB(_nj_light_material_.diffuse);      \
        STORE_CORNER_UV(vertex, corner_uv);                                    \
        writeback_line(&(vertex)->payload);                                    \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_unlit_tex_polygons(NJS_MESHSET *meshset,
                                          NJS_MATERIAL *material)
{
    Uint16 *mesh_cursor;
    NJS_COLOR *corner_uv;
    struct model_vertex *v0;
    struct model_vertex *v1;
    struct model_vertex *v2;
    struct model_vertex *v3;
    Uint16 material_id;
    Sint32 polygon_index;
    Sint32 corner;
    Sint32 corner_count;
    Float facing;
    void (*strip_drawer)(NJS_MESHSET *, Uint32);

    material_id = meshset->type_matId & MATERIAL_ID;
    corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    corner_uv += 3;
                    continue;
                }
            }
            EMIT_UNLIT_TEX_POLYGON(v0, KM_VERTEXPARAM_NORMAL, corner_uv);
            EMIT_UNLIT_TEX_POLYGON(v1, KM_VERTEXPARAM_NORMAL, corner_uv);
            EMIT_UNLIT_TEX_POLYGON(v2, KM_VERTEXPARAM_ENDOFSTRIP, corner_uv);
            _nj_draw_polygon_ += 3;
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    corner_uv += 4;
                    continue;
                }
            }
            EMIT_UNLIT_TEX_POLYGON(v0, KM_VERTEXPARAM_NORMAL, corner_uv);
            EMIT_UNLIT_TEX_POLYGON(v1, KM_VERTEXPARAM_NORMAL, corner_uv);
            EMIT_UNLIT_TEX_POLYGON(v2, KM_VERTEXPARAM_NORMAL, corner_uv);
            EMIT_UNLIT_TEX_POLYGON(v3, KM_VERTEXPARAM_ENDOFSTRIP, corner_uv);
            _nj_draw_polygon_ += 4;
        }
        break;
    case NJD_MESHSET_N:
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            corner_count = *mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    mesh_cursor += corner_count;
                    corner_uv += corner_count;
                    continue;
                }
            }
            _nj_draw_polygon_ += corner_count;
            for (corner = 0; corner < corner_count - 1; corner++) {
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_UNLIT_TEX_POLYGON(v0, KM_VERTEXPARAM_NORMAL, corner_uv);
            }
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_UNLIT_TEX_POLYGON(v0, KM_VERTEXPARAM_ENDOFSTRIP, corner_uv);
        }
        break;
    case NJD_MESHSET_TRIMESH:
        strip_drawer = (material_flags & NJD_FLAG_DOUBLE_SIDE)
                           ? model_draw_unlit_tex_strips_all
                           : model_draw_unlit_tex_strips_cull;
        strip_drawer(meshset, material_id);
        break;
    }
}

static void model_draw_unlit_tex_polygons_clip(NJS_MESHSET *meshset,
                                               NJS_MATERIAL *material)
{
    Uint16 *mesh_cursor;
    NJS_COLOR *corner_uv;
    struct model_vertex *v0;
    struct model_vertex *v1;
    struct model_vertex *v2;
    struct model_vertex *v3;
    Uint16 material_id;
    Uint32 clip_all;
    Uint32 clip_any;
    Sint32 polygon_index;
    Sint32 corner;
    Sint32 corner_count;
    Float facing;
    void (*strip_drawer)(NJS_MESHSET *, Uint32);

    material_id = meshset->type_matId & MATERIAL_ID;
    corner_uv = meshset->vertuv;
    mesh_cursor = (Uint16 *)meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip;
            if (clip_all & CLIP_ALL) {
                corner_uv += 3;
                continue;
            }
            clip_any = v0->control.clip | v1->control.clip | v2->control.clip;
            if (!(clip_any & CLIP_NEAR)) {
                if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                    facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                             ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                    if (facing > 0.0F) {
                        corner_uv += 3;
                        continue;
                    }
                }
                EMIT_UNLIT_TEX_POLYGON(v0, KM_VERTEXPARAM_NORMAL, corner_uv);
                EMIT_UNLIT_TEX_POLYGON(v1, KM_VERTEXPARAM_NORMAL, corner_uv);
                EMIT_UNLIT_TEX_POLYGON(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                                       corner_uv);
                _nj_draw_polygon_ += 3;
            } else {
                if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                    facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                             (v1->point.y * v0->point.z))) +
                             (v2->point.y * ((v1->point.x * v0->point.z) -
                                             (v1->point.z * v0->point.x))) +
                             (v2->point.z * ((v0->point.x * v1->point.y) -
                                             (v1->point.x * v0->point.y)));
                    if (facing > 0.0F) {
                        corner_uv += 3;
                        continue;
                    }
                }
                STORE_CORNER_UV(v0, corner_uv);
                STORE_CORNER_UV(v1, corner_uv);
                STORE_CORNER_UV(v2, corner_uv);
                model_clip_unlit_tex_triangle(v0, v1, v2);
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip &
                       v3->control.clip;
            if (clip_all & CLIP_ALL) {
                corner_uv += 4;
                continue;
            }
            clip_any = v0->control.clip | v1->control.clip | v2->control.clip |
                       v3->control.clip;
            if (!(clip_any & CLIP_NEAR)) {
                if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                    facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                             ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                    if (facing > 0.0F) {
                        corner_uv += 4;
                        continue;
                    }
                }
                EMIT_UNLIT_TEX_POLYGON(v0, KM_VERTEXPARAM_NORMAL, corner_uv);
                EMIT_UNLIT_TEX_POLYGON(v1, KM_VERTEXPARAM_NORMAL, corner_uv);
                EMIT_UNLIT_TEX_POLYGON(v2, KM_VERTEXPARAM_NORMAL, corner_uv);
                EMIT_UNLIT_TEX_POLYGON(v3, KM_VERTEXPARAM_ENDOFSTRIP,
                                       corner_uv);
                _nj_draw_polygon_ += 4;
            } else {
                if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                    facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                             (v1->point.y * v0->point.z))) +
                             (v2->point.y * ((v1->point.x * v0->point.z) -
                                             (v1->point.z * v0->point.x))) +
                             (v2->point.z * ((v0->point.x * v1->point.y) -
                                             (v1->point.x * v0->point.y)));
                    if (facing > 0.0F) {
                        corner_uv += 4;
                        continue;
                    }
                }
                STORE_CORNER_UV(v0, corner_uv);
                STORE_CORNER_UV(v1, corner_uv);
                STORE_CORNER_UV(v2, corner_uv);
                STORE_CORNER_UV(v3, corner_uv);
                clip_all =
                    v0->control.clip & v1->control.clip & v2->control.clip;
                if (!(clip_all & CLIP_NEAR)) {
                    model_clip_unlit_tex_triangle(v0, v1, v2);
                }
                clip_all =
                    v1->control.clip & v2->control.clip & v3->control.clip;
                if (!(clip_all & CLIP_NEAR)) {
                    model_clip_unlit_tex_triangle(v1, v2, v3);
                }
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            corner_count = *mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            clip_all = ~(Uint32)0;
            clip_any = 0;
            for (corner = 0; corner < corner_count; corner++) {
                clip_all &= WORK_VERTEX(mesh_cursor[corner])->control.clip;
                clip_any |= WORK_VERTEX(mesh_cursor[corner])->control.clip;
            }
            if (clip_all & CLIP_ALL) {
                corner_uv += corner_count;
                mesh_cursor += corner_count;
                continue;
            }
            if (!(clip_any & CLIP_NEAR)) {
                if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                    v0 = WORK_VERTEX(mesh_cursor[0]);
                    v1 = WORK_VERTEX(mesh_cursor[1]);
                    v2 = WORK_VERTEX(mesh_cursor[2]);
                    facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                             ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                    if (facing > 0.0F) {
                        mesh_cursor += corner_count;
                        corner_uv += corner_count;
                        continue;
                    }
                }
                _nj_draw_polygon_ += corner_count;
                for (corner = 0; corner < corner_count - 1; corner++) {
                    v0 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_UNLIT_TEX_POLYGON(v0, KM_VERTEXPARAM_NORMAL,
                                           corner_uv);
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_UNLIT_TEX_POLYGON(v0, KM_VERTEXPARAM_ENDOFSTRIP,
                                       corner_uv);
            } else {
                if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                    v0 = WORK_VERTEX(mesh_cursor[0]);
                    v1 = WORK_VERTEX(mesh_cursor[1]);
                    v2 = WORK_VERTEX(mesh_cursor[2]);
                    facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                             (v1->point.y * v0->point.z))) +
                             (v2->point.y * ((v1->point.x * v0->point.z) -
                                             (v1->point.z * v0->point.x))) +
                             (v2->point.z * ((v0->point.x * v1->point.y) -
                                             (v1->point.x * v0->point.y)));
                    if (facing > 0.0F) {
                        mesh_cursor += corner_count;
                        corner_uv += corner_count;
                        continue;
                    }
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                v1 = WORK_VERTEX(*mesh_cursor++);
                STORE_CORNER_UV(v0, corner_uv);
                STORE_CORNER_UV(v1, corner_uv);
                for (corner = 0; corner < corner_count - 2; corner++) {
                    v2 = WORK_VERTEX(*mesh_cursor++);
                    STORE_CORNER_UV(v2, corner_uv);
                    clip_all =
                        v0->control.clip & v1->control.clip & v2->control.clip;
                    if (!(clip_all & CLIP_NEAR)) {
                        model_clip_unlit_tex_triangle(v0, v1, v2);
                    }
                    v0 = v1;
                    v1 = v2;
                }
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        strip_drawer = (model_clip_or & CLIP_NEAR)
                           ? model_draw_unlit_tex_strips_near
                           : model_draw_unlit_tex_strips_checked;
        strip_drawer(meshset, material_id);
        break;
    }
}

/* send vertex with environment coordinates, leaving its colors as they
 * are */
#define EMIT_FLAT_ENV(vertex, packet_command)                                  \
    {                                                                          \
        (vertex)->control.cmd = (packet_command);                              \
        (vertex)->payload.tex.uv[0] = ((vertex)->vector.x * 0.5F) + 0.5F;      \
        (vertex)->payload.tex.uv[1] = ((vertex)->vector.y * 0.5F) + 0.5F;      \
        submit_work_vertex(vertex);                                            \
    }

/* EMIT_FLAT_ENV, in the packed colors at flat_colors */
#define EMIT_FLAT_ENV_COLOR(vertex, packet_command, flat_colors)               \
    {                                                                          \
        (vertex)->control.cmd = (packet_command);                              \
        PACKED_COLOR(vertex, 0) = (flat_colors)[0];                            \
        PACKED_COLOR(vertex, 1) = (flat_colors)[1];                            \
        (vertex)->payload.tex.uv[0] = ((vertex)->vector.x * 0.5F) + 0.5F;      \
        (vertex)->payload.tex.uv[1] = ((vertex)->vector.y * 0.5F) + 0.5F;      \
        submit_work_vertex(vertex);                                            \
    }

/* light each polygon at the point and by the normal that
 * model_draw_flat_polygons uses, and send the packed diffuse and specular as
 * its base and offset colors. without Gouraud shading a triangle takes the
 * color of the corner that completes it, so only the corners from the third on
 * carry the colors */
static void model_draw_flat_env_polygons(NJS_MESHSET *meshset,
                                         NJS_MATERIAL *material)
{
    Uint16 *mesh_cursor;
    struct model_vertex *v0;
    struct model_vertex *v1;
    struct model_vertex *v2;
    struct model_vertex *v3;
    Uint16 material_id;
    Uint32 light_side;
    Sint32 polygon_index;
    Sint32 corner;
    Sint32 corner_count;
    Float facing;
    NJS_POINT3 light_point;
    NJS_VECTOR face_normal;
    Uint32 flat_colors[2];
    void (*strip_drawer)(NJS_MESHSET *, Uint32);

    mesh_cursor = (Uint16 *)meshset->meshes;
    material_id = meshset->type_matId & MATERIAL_ID;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            light_side = 0;
            if (facing > 0.0F) {
                if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                    light_side = BACK_FACE;
                } else {
                    continue;
                }
            }
            COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
            light_point.x =
                (v0->point.x + v1->point.x + v2->point.x) * 0.33333F;
            light_point.y =
                (v0->point.y + v1->point.y + v2->point.y) * 0.33333F;
            light_point.z =
                (v0->point.z + v1->point.z + v2->point.z) * 0.33333F;
            LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                               flat_colors);
            EMIT_FLAT_ENV(v0, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_ENV(v1, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_ENV_COLOR(v2, KM_VERTEXPARAM_ENDOFSTRIP, flat_colors);
            _nj_draw_polygon_ += 3;
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            light_side = 0;
            if (facing > 0.0F) {
                if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                    light_side = BACK_FACE;
                } else {
                    continue;
                }
            }
            COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
            light_point.x = (v1->point.x + v2->point.x) * 0.5F;
            light_point.y = (v1->point.y + v2->point.y) * 0.5F;
            light_point.z = (v1->point.z + v2->point.z) * 0.5F;
            LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                               flat_colors);
            EMIT_FLAT_ENV(v0, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_ENV(v1, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_ENV_COLOR(v2, KM_VERTEXPARAM_NORMAL, flat_colors);
            EMIT_FLAT_ENV_COLOR(v3, KM_VERTEXPARAM_ENDOFSTRIP, flat_colors);
            _nj_draw_polygon_ += 4;
        }
        break;
    case NJD_MESHSET_N:
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            corner_count = *mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            v0 = WORK_VERTEX(mesh_cursor[0]);
            v1 = WORK_VERTEX(mesh_cursor[1]);
            v2 = WORK_VERTEX(mesh_cursor[2]);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            light_side = 0;
            if (facing > 0.0F) {
                if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                    light_side = BACK_FACE;
                } else {
                    mesh_cursor += corner_count;
                    continue;
                }
            }
            _nj_draw_polygon_ += corner_count;
            COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
            v3 = WORK_VERTEX(mesh_cursor[(corner_count / 2) - 1]);
            light_point.x = v3->point.x;
            light_point.y = v3->point.y;
            light_point.z = v3->point.z;
            v3 = WORK_VERTEX(mesh_cursor[corner_count / 2]);
            light_point.x += v3->point.x;
            light_point.y += v3->point.y;
            light_point.z += v3->point.z;
            light_point.x *= 0.5F;
            light_point.y *= 0.5F;
            light_point.z *= 0.5F;
            LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                               flat_colors);
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_ENV(v0, KM_VERTEXPARAM_NORMAL);
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_ENV(v0, KM_VERTEXPARAM_NORMAL);
            for (corner = 2; corner < corner_count - 1; corner++) {
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_ENV_COLOR(v0, KM_VERTEXPARAM_NORMAL, flat_colors);
            }
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_ENV_COLOR(v0, KM_VERTEXPARAM_ENDOFSTRIP, flat_colors);
        }
        break;
    case NJD_MESHSET_TRIMESH:
        strip_drawer = (material_flags & NJD_FLAG_DOUBLE_SIDE)
                           ? model_draw_flat_path_env_strips_all
                           : model_draw_flat_path_env_strips_cull;
        strip_drawer(meshset, material_id);
        break;
    }
}

static void model_draw_flat_env_polygons_clip(NJS_MESHSET *meshset,
                                              NJS_MATERIAL *material)
{
    Uint16 *mesh_cursor;
    struct model_vertex *v0;
    struct model_vertex *v1;
    struct model_vertex *v2;
    struct model_vertex *v3;
    Uint16 material_id;
    Uint32 light_side;
    Uint32 clip_all;
    Uint32 clip_any;
    Sint32 polygon_index;
    Sint32 corner;
    Sint32 corner_count;
    Float facing;
    NJS_POINT3 light_point;
    NJS_VECTOR face_normal;
    Uint32 flat_colors[2];
    void (*strip_drawer)(NJS_MESHSET *, Uint32);

    mesh_cursor = (Uint16 *)meshset->meshes;
    material_id = meshset->type_matId & MATERIAL_ID;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip;
            if (clip_all & CLIP_ALL) {
                continue;
            }
            clip_any = v0->control.clip | v1->control.clip | v2->control.clip;
            if (!(clip_any & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        continue;
                    }
                }
                COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
                light_point.x =
                    (v0->point.x + v1->point.x + v2->point.x) * 0.33333F;
                light_point.y =
                    (v0->point.y + v1->point.y + v2->point.y) * 0.33333F;
                light_point.z =
                    (v0->point.z + v1->point.z + v2->point.z) * 0.33333F;
                LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                                   flat_colors);
                EMIT_FLAT_ENV(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_ENV(v1, KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_ENV_COLOR(v2, KM_VERTEXPARAM_ENDOFSTRIP, flat_colors);
                _nj_draw_polygon_ += 3;
            } else {
                facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                         (v1->point.y * v0->point.z))) +
                         (v2->point.y * ((v1->point.x * v0->point.z) -
                                         (v1->point.z * v0->point.x))) +
                         (v2->point.z * ((v0->point.x * v1->point.y) -
                                         (v1->point.x * v0->point.y)));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        continue;
                    }
                }
                COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
                light_point.x =
                    (v0->point.x + v1->point.x + v2->point.x) * 0.33333F;
                light_point.y =
                    (v0->point.y + v1->point.y + v2->point.y) * 0.33333F;
                light_point.z =
                    (v0->point.z + v1->point.z + v2->point.z) * 0.33333F;
                LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                                   flat_colors);
                model_clip_constant_env_triangle(v0, v1, v2, flat_colors);
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip &
                       v3->control.clip;
            if (clip_all & CLIP_ALL) {
                continue;
            }
            clip_any = v0->control.clip | v1->control.clip | v2->control.clip |
                       v3->control.clip;
            if (!(clip_any & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        continue;
                    }
                }
                COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
                light_point.x = (v1->point.x + v2->point.x) * 0.5F;
                light_point.y = (v1->point.y + v2->point.y) * 0.5F;
                light_point.z = (v1->point.z + v2->point.z) * 0.5F;
                LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                                   flat_colors);
                EMIT_FLAT_ENV(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_ENV(v1, KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_ENV_COLOR(v2, KM_VERTEXPARAM_NORMAL, flat_colors);
                EMIT_FLAT_ENV_COLOR(v3, KM_VERTEXPARAM_ENDOFSTRIP, flat_colors);
                _nj_draw_polygon_ += 4;
            } else {
                facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                         (v1->point.y * v0->point.z))) +
                         (v2->point.y * ((v1->point.x * v0->point.z) -
                                         (v1->point.z * v0->point.x))) +
                         (v2->point.z * ((v0->point.x * v1->point.y) -
                                         (v1->point.x * v0->point.y)));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        continue;
                    }
                }
                COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
                light_point.x = (v1->point.x + v2->point.x) * 0.5F;
                light_point.y = (v1->point.y + v2->point.y) * 0.5F;
                light_point.z = (v1->point.z + v2->point.z) * 0.5F;
                LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                                   flat_colors);
                clip_all =
                    v0->control.clip & v1->control.clip & v2->control.clip;
                if (!(clip_all & CLIP_NEAR)) {
                    model_clip_constant_env_triangle(v0, v1, v2, flat_colors);
                }
                clip_all =
                    v1->control.clip & v2->control.clip & v3->control.clip;
                if (!(clip_all & CLIP_NEAR)) {
                    model_clip_constant_env_triangle(v1, v2, v3, flat_colors);
                }
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            corner_count = *mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            clip_all = ~(Uint32)0;
            clip_any = 0;
            for (corner = 0; corner < corner_count; corner++) {
                clip_all &= WORK_VERTEX(mesh_cursor[corner])->control.clip;
                clip_any |= WORK_VERTEX(mesh_cursor[corner])->control.clip;
            }
            if (clip_all & CLIP_ALL) {
                mesh_cursor += corner_count;
                continue;
            }
            if (!(clip_any & CLIP_NEAR)) {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        mesh_cursor += corner_count;
                        continue;
                    }
                }
                _nj_draw_polygon_ += corner_count;
                COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
                v3 = WORK_VERTEX(mesh_cursor[(corner_count / 2) - 1]);
                light_point.x = v3->point.x;
                light_point.y = v3->point.y;
                light_point.z = v3->point.z;
                v3 = WORK_VERTEX(mesh_cursor[corner_count / 2]);
                light_point.x += v3->point.x;
                light_point.y += v3->point.y;
                light_point.z += v3->point.z;
                light_point.x *= 0.5F;
                light_point.y *= 0.5F;
                light_point.z *= 0.5F;
                LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                                   flat_colors);
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_ENV(v0, KM_VERTEXPARAM_NORMAL);
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_ENV(v0, KM_VERTEXPARAM_NORMAL);
                for (corner = 2; corner < corner_count - 1; corner++) {
                    v0 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_FLAT_ENV_COLOR(v0, KM_VERTEXPARAM_NORMAL, flat_colors);
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_ENV_COLOR(v0, KM_VERTEXPARAM_ENDOFSTRIP, flat_colors);
            } else {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);
                facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                         (v1->point.y * v0->point.z))) +
                         (v2->point.y * ((v1->point.x * v0->point.z) -
                                         (v1->point.z * v0->point.x))) +
                         (v2->point.z * ((v0->point.x * v1->point.y) -
                                         (v1->point.x * v0->point.y)));
                light_side = 0;
                if (facing > 0.0F) {
                    if (material_flags & NJD_FLAG_DOUBLE_SIDE) {
                        light_side = BACK_FACE;
                    } else {
                        mesh_cursor += corner_count;
                        continue;
                    }
                }
                COMPUTE_FLAT_NORMAL(v0, v1, v2, face_normal);
                v3 = WORK_VERTEX(mesh_cursor[(corner_count / 2) - 1]);
                light_point.x = v3->point.x;
                light_point.y = v3->point.y;
                light_point.z = v3->point.z;
                v3 = WORK_VERTEX(mesh_cursor[corner_count / 2]);
                light_point.x += v3->point.x;
                light_point.y += v3->point.y;
                light_point.z += v3->point.z;
                light_point.x *= 0.5F;
                light_point.y *= 0.5F;
                light_point.z *= 0.5F;
                LIGHT_FLAT_POLYGON(light_point, face_normal, light_side,
                                   flat_colors);
                v0 = WORK_VERTEX(*mesh_cursor++);
                v1 = WORK_VERTEX(*mesh_cursor++);
                for (corner = 0; corner < corner_count - 2; corner++) {
                    v2 = WORK_VERTEX(*mesh_cursor++);
                    clip_all =
                        v0->control.clip & v1->control.clip & v2->control.clip;
                    if (!(clip_all & CLIP_NEAR)) {
                        model_clip_constant_env_triangle(v0, v1, v2,
                                                         flat_colors);
                    }
                    v0 = v1;
                    v1 = v2;
                }
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        strip_drawer = (model_clip_or & CLIP_NEAR)
                           ? model_draw_flat_path_env_strips_near
                           : model_draw_flat_path_env_strips_checked;
        strip_drawer(meshset, material_id);
        break;
    }
}

/* send vertex in the material's diffuse color, packed, with environment
 * coordinates */
/* @hack spelling: a Float pointer walks the vertex normal's x into y through
 * vector_floats, which gives the FMOV.S @Rn+ load of x (0x8c651162 in
 * model_draw_unlit_env_polygons); read by member, 1791 of 1926 words differ in
 * model_draw_unlit_env_polygons and model_draw_unlit_env_polygons_clip and the
 * section is 24 bytes shorter */
#define EMIT_UNLIT_ENV_POLYGON(vertex, packet_command)                         \
    {                                                                          \
        Float *uv_out;                                                         \
        Float *normal_xy;                                                      \
        Float half;                                                            \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        PACKED_COLOR(vertex, 0) = PACK_ARGB(_nj_light_material_.diffuse);      \
        half = 0.5F;                                                           \
        uv_out = (vertex)->payload.tex.uv;                                     \
        normal_xy = ((union vector_floats *)&(vertex)->vector)->f;             \
        *uv_out++ = (*normal_xy++ * half) + half;                              \
        *uv_out = (*normal_xy * half) + half;                                  \
        writeback_line(uv_out);                                                \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_unlit_env_polygons(NJS_MESHSET *meshset,
                                          NJS_MATERIAL *material)
{
    Uint16 *mesh_cursor;
    struct model_vertex *v0;
    struct model_vertex *v1;
    struct model_vertex *v2;
    struct model_vertex *v3;
    Uint16 material_id;
    Sint32 polygon_index;
    Sint32 corner;
    Sint32 corner_count;
    Float facing;
    void (*strip_drawer)(NJS_MESHSET *, Uint32);

    material_id = meshset->type_matId & MATERIAL_ID;
    mesh_cursor = (Uint16 *)meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    continue;
                }
            }
            EMIT_UNLIT_ENV_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
            EMIT_UNLIT_ENV_POLYGON(v1, KM_VERTEXPARAM_NORMAL);
            EMIT_UNLIT_ENV_POLYGON(v2, KM_VERTEXPARAM_ENDOFSTRIP);
            _nj_draw_polygon_ += 3;
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    continue;
                }
            }
            EMIT_UNLIT_ENV_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
            EMIT_UNLIT_ENV_POLYGON(v1, KM_VERTEXPARAM_NORMAL);
            EMIT_UNLIT_ENV_POLYGON(v2, KM_VERTEXPARAM_NORMAL);
            EMIT_UNLIT_ENV_POLYGON(v3, KM_VERTEXPARAM_ENDOFSTRIP);
            _nj_draw_polygon_ += 4;
        }
        break;
    case NJD_MESHSET_N:
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            corner_count = *mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    mesh_cursor += corner_count;
                    continue;
                }
            }
            _nj_draw_polygon_ += corner_count;
            for (corner = 0; corner < corner_count - 1; corner++) {
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_UNLIT_ENV_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
            }
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_UNLIT_ENV_POLYGON(v0, KM_VERTEXPARAM_ENDOFSTRIP);
        }
        break;
    case NJD_MESHSET_TRIMESH:
        strip_drawer = (material_flags & NJD_FLAG_DOUBLE_SIDE)
                           ? model_draw_unlit_env_strips_all
                           : model_draw_unlit_env_strips_cull;
        strip_drawer(meshset, material_id);
        break;
    }
}

static void model_draw_unlit_env_polygons_clip(NJS_MESHSET *meshset,
                                               NJS_MATERIAL *material)
{
    Uint16 *mesh_cursor;
    struct model_vertex *v0;
    struct model_vertex *v1;
    struct model_vertex *v2;
    struct model_vertex *v3;
    Uint16 material_id;
    Uint32 clip_all;
    Uint32 clip_any;
    Sint32 polygon_index;
    Sint32 corner;
    Sint32 corner_count;
    Float facing;
    void (*strip_drawer)(NJS_MESHSET *, Uint32);

    material_id = meshset->type_matId & MATERIAL_ID;
    mesh_cursor = (Uint16 *)meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip;
            if (clip_all & CLIP_ALL) {
                continue;
            }
            clip_any = v0->control.clip | v1->control.clip | v2->control.clip;
            if (!(clip_any & CLIP_NEAR)) {
                if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                    facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                             ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                    if (facing > 0.0F) {
                        continue;
                    }
                }
                EMIT_UNLIT_ENV_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_ENV_POLYGON(v1, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_ENV_POLYGON(v2, KM_VERTEXPARAM_ENDOFSTRIP);
                _nj_draw_polygon_ += 3;
            } else {
                if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                    facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                             (v1->point.y * v0->point.z))) +
                             (v2->point.y * ((v1->point.x * v0->point.z) -
                                             (v1->point.z * v0->point.x))) +
                             (v2->point.z * ((v0->point.x * v1->point.y) -
                                             (v1->point.x * v0->point.y)));
                    if (facing > 0.0F) {
                        continue;
                    }
                }
                model_clip_unlit_env_triangle(v0, v1, v2);
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip &
                       v3->control.clip;
            if (clip_all & CLIP_ALL) {
                continue;
            }
            clip_any = v0->control.clip | v1->control.clip | v2->control.clip |
                       v3->control.clip;
            if (!(clip_any & CLIP_NEAR)) {
                if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                    facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                             ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                    if (facing > 0.0F) {
                        continue;
                    }
                }
                EMIT_UNLIT_ENV_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_ENV_POLYGON(v1, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_ENV_POLYGON(v2, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_ENV_POLYGON(v3, KM_VERTEXPARAM_ENDOFSTRIP);
                _nj_draw_polygon_ += 4;
            } else {
                if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                    facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                             (v1->point.y * v0->point.z))) +
                             (v2->point.y * ((v1->point.x * v0->point.z) -
                                             (v1->point.z * v0->point.x))) +
                             (v2->point.z * ((v0->point.x * v1->point.y) -
                                             (v1->point.x * v0->point.y)));
                    if (facing > 0.0F) {
                        continue;
                    }
                }
                clip_all =
                    v0->control.clip & v1->control.clip & v2->control.clip;
                if (!(clip_all & CLIP_NEAR)) {
                    model_clip_unlit_env_triangle(v0, v1, v2);
                }
                clip_all =
                    v1->control.clip & v2->control.clip & v3->control.clip;
                if (!(clip_all & CLIP_NEAR)) {
                    model_clip_unlit_env_triangle(v1, v2, v3);
                }
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            corner_count = *mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            clip_all = ~(Uint32)0;
            clip_any = 0;
            for (corner = 0; corner < corner_count; corner++) {
                clip_all &= WORK_VERTEX(mesh_cursor[corner])->control.clip;
                clip_any |= WORK_VERTEX(mesh_cursor[corner])->control.clip;
            }
            if (clip_all & CLIP_ALL) {
                mesh_cursor += corner_count;
                continue;
            }
            if (!(clip_any & CLIP_NEAR)) {
                if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                    v0 = WORK_VERTEX(mesh_cursor[0]);
                    v1 = WORK_VERTEX(mesh_cursor[1]);
                    v2 = WORK_VERTEX(mesh_cursor[2]);
                    facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                             ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                    if (facing > 0.0F) {
                        mesh_cursor += corner_count;
                        continue;
                    }
                }
                _nj_draw_polygon_ += corner_count;
                for (corner = 0; corner < corner_count - 1; corner++) {
                    v0 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_UNLIT_ENV_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_UNLIT_ENV_POLYGON(v0, KM_VERTEXPARAM_ENDOFSTRIP);
            } else {
                if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                    v0 = WORK_VERTEX(mesh_cursor[0]);
                    v1 = WORK_VERTEX(mesh_cursor[1]);
                    v2 = WORK_VERTEX(mesh_cursor[2]);
                    facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                             (v1->point.y * v0->point.z))) +
                             (v2->point.y * ((v1->point.x * v0->point.z) -
                                             (v1->point.z * v0->point.x))) +
                             (v2->point.z * ((v0->point.x * v1->point.y) -
                                             (v1->point.x * v0->point.y)));
                    if (facing > 0.0F) {
                        mesh_cursor += corner_count;
                        continue;
                    }
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                v1 = WORK_VERTEX(*mesh_cursor++);
                for (corner = 0; corner < corner_count - 2; corner++) {
                    v2 = WORK_VERTEX(*mesh_cursor++);
                    clip_all =
                        v0->control.clip & v1->control.clip & v2->control.clip;
                    if (!(clip_all & CLIP_NEAR)) {
                        model_clip_unlit_env_triangle(v0, v1, v2);
                    }
                    v0 = v1;
                    v1 = v2;
                }
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        strip_drawer = (model_clip_or & CLIP_NEAR)
                           ? model_draw_unlit_env_strips_near
                           : model_draw_unlit_env_strips_checked;
        strip_drawer(meshset, material_id);
        break;
    }
}

/* send vertex in the material's diffuse color, as floats */
/* @hack spelling: Float pointers walk the material's diffuse and the vertex
 * color's a, r, g and b through argb_floats, which gives the FMOV.S @Rn+ loads
 * and the ADD #4 store steps of the copy (0x8c652158 to 0x8c65216c in
 * model_draw_unlit_polygons); copied by member, 1582 of 1652 words differ in
 * model_draw_unlit_polygons and model_draw_unlit_polygons_clip and the section
 * is 112 bytes longer */
#define EMIT_UNLIT_POLYGON(vertex, packet_command)                             \
    {                                                                          \
        Float *color_out;                                                      \
        Float *color_in;                                                       \
        (vertex)->control.cmd = (packet_command);                              \
        writeback_line(vertex);                                                \
        color_out = ((union argb_floats *)&(vertex)->payload.col)->f;          \
        color_in = ((union argb_floats *)&_nj_light_material_.diffuse)->f;     \
        *color_out++ = *color_in++;                                            \
        *color_out++ = *color_in++;                                            \
        *color_out++ = *color_in++;                                            \
        *color_out = *color_in;                                                \
        writeback_line(color_out);                                             \
        submit_work_vertex(vertex);                                            \
    }

static void model_draw_unlit_polygons(NJS_MESHSET *meshset,
                                      NJS_MATERIAL *material)
{
    Uint16 *mesh_cursor;
    struct model_vertex *v0;
    struct model_vertex *v1;
    struct model_vertex *v2;
    struct model_vertex *v3;
    Uint16 material_id;
    Sint32 polygon_index;
    Sint32 corner;
    Sint32 corner_count;
    Float facing;
    void (*strip_drawer)(NJS_MESHSET *, Uint32);

    material_id = meshset->type_matId & MATERIAL_ID;
    mesh_cursor = (Uint16 *)meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    continue;
                }
            }
            EMIT_UNLIT_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
            EMIT_UNLIT_POLYGON(v1, KM_VERTEXPARAM_NORMAL);
            EMIT_UNLIT_POLYGON(v2, KM_VERTEXPARAM_ENDOFSTRIP);
            _nj_draw_polygon_ += 3;
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    continue;
                }
            }
            EMIT_UNLIT_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
            EMIT_UNLIT_POLYGON(v1, KM_VERTEXPARAM_NORMAL);
            EMIT_UNLIT_POLYGON(v2, KM_VERTEXPARAM_NORMAL);
            EMIT_UNLIT_POLYGON(v3, KM_VERTEXPARAM_ENDOFSTRIP);
            _nj_draw_polygon_ += 4;
        }
        break;
    case NJD_MESHSET_N:
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            corner_count = *mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    mesh_cursor += corner_count;
                    continue;
                }
            }
            _nj_draw_polygon_ += corner_count;
            for (corner = 0; corner < corner_count - 1; corner++) {
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_UNLIT_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
            }
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_UNLIT_POLYGON(v0, KM_VERTEXPARAM_ENDOFSTRIP);
        }
        break;
    case NJD_MESHSET_TRIMESH:
        strip_drawer = (material_flags & NJD_FLAG_DOUBLE_SIDE)
                           ? model_draw_unlit_strips_all
                           : model_draw_unlit_strips_cull;
        strip_drawer(meshset, material_id);
        break;
    }
}

static void model_draw_unlit_polygons_clip(NJS_MESHSET *meshset,
                                           NJS_MATERIAL *material)
{
    Uint16 *mesh_cursor;
    struct model_vertex *v0;
    struct model_vertex *v1;
    struct model_vertex *v2;
    struct model_vertex *v3;
    Uint16 material_id;
    Uint32 clip_all;
    Uint32 clip_any;
    Sint32 polygon_index;
    Sint32 corner;
    Sint32 corner_count;
    Float facing;
    void (*strip_drawer)(NJS_MESHSET *, Uint32);

    material_id = meshset->type_matId & MATERIAL_ID;
    mesh_cursor = (Uint16 *)meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip;
            if (clip_all & CLIP_ALL) {
                continue;
            }
            clip_any = v0->control.clip | v1->control.clip | v2->control.clip;
            if (!(clip_any & CLIP_NEAR)) {
                if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                    facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                             ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                    if (facing > 0.0F) {
                        continue;
                    }
                }
                EMIT_UNLIT_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_POLYGON(v1, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_POLYGON(v2, KM_VERTEXPARAM_ENDOFSTRIP);
                _nj_draw_polygon_ += 3;
            } else {
                if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                    facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                             (v1->point.y * v0->point.z))) +
                             (v2->point.y * ((v1->point.x * v0->point.z) -
                                             (v1->point.z * v0->point.x))) +
                             (v2->point.z * ((v0->point.x * v1->point.y) -
                                             (v1->point.x * v0->point.y)));
                    if (facing > 0.0F) {
                        continue;
                    }
                }
                model_clip_unlit_triangle(v0, v1, v2);
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            clip_all = v0->control.clip & v1->control.clip & v2->control.clip &
                       v3->control.clip;
            if (clip_all & CLIP_ALL) {
                continue;
            }
            clip_any = v0->control.clip | v1->control.clip | v2->control.clip |
                       v3->control.clip;
            if (!(clip_any & CLIP_NEAR)) {
                if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                    facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                             ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                    if (facing > 0.0F) {
                        continue;
                    }
                }
                EMIT_UNLIT_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_POLYGON(v1, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_POLYGON(v2, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_POLYGON(v3, KM_VERTEXPARAM_ENDOFSTRIP);
                _nj_draw_polygon_ += 4;
            } else {
                if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                    facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                             (v1->point.y * v0->point.z))) +
                             (v2->point.y * ((v1->point.x * v0->point.z) -
                                             (v1->point.z * v0->point.x))) +
                             (v2->point.z * ((v0->point.x * v1->point.y) -
                                             (v1->point.x * v0->point.y)));
                    if (facing > 0.0F) {
                        continue;
                    }
                }
                clip_all =
                    v0->control.clip & v1->control.clip & v2->control.clip;
                if (!(clip_all & CLIP_NEAR)) {
                    model_clip_unlit_triangle(v0, v1, v2);
                }
                clip_all =
                    v1->control.clip & v2->control.clip & v3->control.clip;
                if (!(clip_all & CLIP_NEAR)) {
                    model_clip_unlit_triangle(v1, v2, v3);
                }
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon_index = 0; polygon_index < meshset->nbMesh;
             polygon_index++) {
            corner_count = *mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            clip_all = ~(Uint32)0;
            clip_any = 0;
            for (corner = 0; corner < corner_count; corner++) {
                clip_all &= WORK_VERTEX(mesh_cursor[corner])->control.clip;
                clip_any |= WORK_VERTEX(mesh_cursor[corner])->control.clip;
            }
            if (clip_all & CLIP_ALL) {
                mesh_cursor += corner_count;
                continue;
            }
            if (!(clip_any & CLIP_NEAR)) {
                if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                    v0 = WORK_VERTEX(mesh_cursor[0]);
                    v1 = WORK_VERTEX(mesh_cursor[1]);
                    v2 = WORK_VERTEX(mesh_cursor[2]);
                    facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                             ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                    if (facing > 0.0F) {
                        mesh_cursor += corner_count;
                        continue;
                    }
                }
                _nj_draw_polygon_ += corner_count;
                for (corner = 0; corner < corner_count - 1; corner++) {
                    v0 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_UNLIT_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_UNLIT_POLYGON(v0, KM_VERTEXPARAM_ENDOFSTRIP);
            } else {
                if (!(material_flags & NJD_FLAG_DOUBLE_SIDE)) {
                    v0 = WORK_VERTEX(mesh_cursor[0]);
                    v1 = WORK_VERTEX(mesh_cursor[1]);
                    v2 = WORK_VERTEX(mesh_cursor[2]);
                    facing = (v2->point.x * ((v1->point.z * v0->point.y) -
                                             (v1->point.y * v0->point.z))) +
                             (v2->point.y * ((v1->point.x * v0->point.z) -
                                             (v1->point.z * v0->point.x))) +
                             (v2->point.z * ((v0->point.x * v1->point.y) -
                                             (v1->point.x * v0->point.y)));
                    if (facing > 0.0F) {
                        mesh_cursor += corner_count;
                        continue;
                    }
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                v1 = WORK_VERTEX(*mesh_cursor++);
                for (corner = 0; corner < corner_count - 2; corner++) {
                    v2 = WORK_VERTEX(*mesh_cursor++);
                    clip_all =
                        v0->control.clip & v1->control.clip & v2->control.clip;
                    if (!(clip_all & CLIP_NEAR)) {
                        model_clip_unlit_triangle(v0, v1, v2);
                    }
                    v0 = v1;
                    v1 = v2;
                }
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        strip_drawer = (model_clip_or & CLIP_NEAR)
                           ? model_draw_unlit_strips_near
                           : model_draw_unlit_strips_checked;
        strip_drawer(meshset, material_id);
        break;
    }
}

/* draw model with the current lights and matrix. nothing draws when model has
 * more points than _nj_vertex_num_. under NJD_CONTROL_3D_DISP_AXIS the axes
 * draw next. _njDrawModelSub then clears _nj_depth_enable_ and calls
 * njCalcModel, which may set it, and draws nothing more when njCalcModel
 * returns -1.
 *
 * each meshset draws through the drawer that material_flags choose, or that
 * drawer's _clip twin when model_clip_or isn't 0. no drawer takes
 * NJD_FLAG_USE_ENV without NJD_FLAG_USE_TEXTURE: a meshset whose material_flags
 * hold that draws nothing. every meshset ends with njEndVertex.
 *
 * the first meshset sets up its material, and so does each whose material index
 * differs from the one before. the set-up takes material_flags from the
 * material's attrflags, and adds NJD_FLAG_USE_ALPHA while _nj_depth_enable_ is
 * set. under NJD_CONTROL_3D_CONSTANT_ATTR it then ands _nj_constant_attr_and_
 * and ors _nj_constant_attr_or_ into them, which can clear NJD_FLAG_USE_ALPHA
 * again. it sets the texture under NJD_FLAG_USE_TEXTURE. it sets
 * _nj_light_material_ from the material, as NJD_CONTROL_3D_CONSTANT_MATERIAL,
 * _OFFSET_MATERIAL and _CONSTANT_TEXTURE_MATERIAL adjust it. it starts a vertex
 * list in the color mode that material_flags give */
void _njDrawModelSub(NJS_MODEL *model)
{
    NJS_MESHSET *meshset;
    NJS_MATERIAL *material;
    Sint32 meshset_index;
    Uint16 material_id;
    Uint16 previous_material_id;
    Uint32 shader_flags;

    if (model->nbPoint > _nj_vertex_num_) {
        return;
    }
    meshset = model->meshsets;
    if (_nj_control_3d_flag_ & NJD_CONTROL_3D_DISP_AXIS) {
        njDrawLine3D(&axis_lines, 3, NJD_DRAW_FAN);
    }
    _nj_depth_enable_ = 0;
    if (njCalcModel(model)) {
        return;
    }
    njGetLightDistance(&view_center);
#ifdef GCC
    /* GCC can't see that these are set wherever they are read: the loop's
     * first pass, meshset_index 0, sets each before any read */
    material = NULL;
    previous_material_id = 0;
    shader_flags = 0;
#endif
    for (meshset_index = 0; meshset_index < model->nbMeshset; meshset_index++) {
        material_id = meshset->type_matId & MATERIAL_ID;
        if (meshset_index == 0 || material_id != previous_material_id) {
            material = &model->mats[material_id];
            material_flags = material->attrflags;
            if (_nj_depth_enable_) {
                material_flags |= NJD_FLAG_USE_ALPHA;
            }
            if (_nj_control_3d_flag_ & NJD_CONTROL_3D_CONSTANT_ATTR) {
                material_flags &= _nj_constant_attr_and_;
                material_flags |= _nj_constant_attr_or_;
            }
            if (material_flags & NJD_FLAG_USE_TEXTURE) {
                njSetkmTextureNum(material->attr_texId & MATERIAL_TEXTURE);
                if (_nj_control_3d_flag_ &
                    NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL) {
                    Float byte_scale = 1.0F / 255.0F;

                    _nj_light_material_.ambient.a =
                        _nj_light_material_.diffuse.a =
                            material->diffuse.argb.a * byte_scale;
                    _nj_light_material_.ambient.r =
                        _nj_light_material_.diffuse.r = 1.0F;
                    _nj_light_material_.ambient.g =
                        _nj_light_material_.diffuse.g = 1.0F;
                    _nj_light_material_.ambient.b =
                        _nj_light_material_.diffuse.b = 1.0F;
                } else {
                    Float byte_scale = 1.0F / 255.0F;

                    _nj_light_material_.ambient.r =
                        _nj_light_material_.diffuse.r =
                            material->diffuse.argb.r * byte_scale;
                    _nj_light_material_.ambient.g =
                        _nj_light_material_.diffuse.g =
                            material->diffuse.argb.g * byte_scale;
                    _nj_light_material_.ambient.b =
                        _nj_light_material_.diffuse.b =
                            material->diffuse.argb.b * byte_scale;
                    _nj_light_material_.ambient.a =
                        _nj_light_material_.diffuse.a =
                            material->diffuse.argb.a * byte_scale;
                }
            } else {
                Float byte_scale = 1.0F / 255.0F;

                _nj_light_material_.ambient.r = _nj_light_material_.diffuse.r =
                    material->diffuse.argb.r * byte_scale;
                _nj_light_material_.ambient.g = _nj_light_material_.diffuse.g =
                    material->diffuse.argb.g * byte_scale;
                _nj_light_material_.ambient.b = _nj_light_material_.diffuse.b =
                    material->diffuse.argb.b * byte_scale;
                _nj_light_material_.ambient.a = _nj_light_material_.diffuse.a =
                    material->diffuse.argb.a * byte_scale;
            }

            if (_nj_control_3d_flag_ & NJD_CONTROL_3D_CONSTANT_MATERIAL) {
                _nj_light_material_.ambient.a = _nj_light_material_.diffuse.a =
                    _nj_constant_material_.a;
                _nj_light_material_.ambient.r = _nj_light_material_.diffuse.r =
                    _nj_constant_material_.r;
                _nj_light_material_.ambient.g = _nj_light_material_.diffuse.g =
                    _nj_constant_material_.g;
                _nj_light_material_.ambient.b = _nj_light_material_.diffuse.b =
                    _nj_constant_material_.b;
            } else if (_nj_control_3d_flag_ & NJD_CONTROL_3D_OFFSET_MATERIAL) {
                _nj_light_material_.diffuse.a += _nj_constant_material_.a;
                _nj_light_material_.diffuse.r += _nj_constant_material_.r;
                _nj_light_material_.diffuse.g += _nj_constant_material_.g;
                _nj_light_material_.diffuse.b += _nj_constant_material_.b;
                _nj_light_material_.ambient.a = _nj_light_material_.diffuse.a;
                _nj_light_material_.ambient.r = _nj_light_material_.diffuse.r;
                _nj_light_material_.ambient.g = _nj_light_material_.diffuse.g;
                _nj_light_material_.ambient.b = _nj_light_material_.diffuse.b;
            }

            {
                Float byte_scale = 1.0F / 255.0F;

                _nj_light_material_.specular.r =
                    material->specular.argb.r * byte_scale;
                _nj_light_material_.specular.g =
                    material->specular.argb.g * byte_scale;
                _nj_light_material_.specular.b =
                    material->specular.argb.b * byte_scale;
            }
            _nj_light_material_.exponent = material->exponent;
            njCalcLightMaterials();
            shader_flags =
                material_flags & (NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_FLAT |
                                  NJD_FLAG_USE_ENV | NJD_FLAG_USE_TEXTURE);
            njSetkmModelAttr(material_flags);

            if (material_flags & NJD_FLAG_USE_TEXTURE) {
                if (material_flags & NJD_FLAG_IGNORE_LIGHT) {
                    if (material_flags & NJD_FLAG_USE_ALPHA) {
                        njSetkmColorMode(
                            FLAT(COLOR_MODE_TRANS | COLOR_MODE_TEXTURE));
                        queue_cursor = njStartVertexTexTrans();
                    } else {
                        njSetkmColorMode(
                            FLAT(COLOR_MODE_OPAQUE | COLOR_MODE_TEXTURE));
                        queue_cursor = njStartVertexTexOpaque();
                    }
                } else if (material_flags & NJD_FLAG_USE_FLAT) {
                    if (material_flags & NJD_FLAG_USE_ALPHA) {
                        if (material_flags & NJD_FLAG_IGNORE_SPECULAR) {
                            njSetkmColorMode(
                                FLAT(COLOR_MODE_TRANS | COLOR_MODE_TEXTURE));
                        } else {
                            njSetkmColorMode(
                                FLAT(COLOR_MODE_TRANS | COLOR_MODE_TEXTURE) |
                                COLOR_MODE_OFFSET);
                        }
                        queue_cursor = njStartVertexTexTrans();
                    } else {
                        if (material_flags & NJD_FLAG_IGNORE_SPECULAR) {
                            njSetkmColorMode(
                                FLAT(COLOR_MODE_OPAQUE | COLOR_MODE_TEXTURE));
                        } else {
                            njSetkmColorMode(
                                FLAT(COLOR_MODE_OPAQUE | COLOR_MODE_TEXTURE) |
                                COLOR_MODE_OFFSET);
                        }
                        queue_cursor = njStartVertexTexOpaque();
                    }
                } else {
                    if (material_flags & NJD_FLAG_USE_ALPHA) {
                        if (material_flags & NJD_FLAG_IGNORE_SPECULAR) {
                            njSetkmColorMode(COLOR_MODE_TRANS |
                                             COLOR_MODE_TEXTURE);
                        } else {
                            njSetkmColorMode(COLOR_MODE_TRANS |
                                             COLOR_MODE_TEXTURE |
                                             COLOR_MODE_OFFSET);
                        }
                        queue_cursor = njStartVertexTexTrans();
                    } else {
                        if (material_flags & NJD_FLAG_IGNORE_SPECULAR) {
                            njSetkmColorMode(COLOR_MODE_OPAQUE |
                                             COLOR_MODE_TEXTURE);
                        } else {
                            njSetkmColorMode(COLOR_MODE_OPAQUE |
                                             COLOR_MODE_TEXTURE |
                                             COLOR_MODE_OFFSET);
                        }
                        queue_cursor = njStartVertexTexOpaque();
                    }
                }
            } else {
                if (material_flags & NJD_FLAG_IGNORE_LIGHT) {
                    if (material_flags & NJD_FLAG_USE_ALPHA) {
                        njSetkmColorMode(FLAT(NJD_COLOR_MODE_NORMAL_TRANS));
                        queue_cursor = njStartVertexNonTexTrans();
                    } else {
                        njSetkmColorMode(FLAT(NJD_COLOR_MODE_NORMAL));
                        queue_cursor = njStartVertexNonTexOpaque();
                    }
                } else if (material_flags & NJD_FLAG_USE_FLAT) {
                    if (material_flags & NJD_FLAG_USE_ALPHA) {
                        /* @hack redundant: both arms of this test and of the
                         * three below set one color mode, and the machine code
                         * keeps each test: TST R5,R4 at 0x8c653172, 0x8c65317e,
                         * 0x8c65318e and 0x8c65319a. as one call for each,
                         * njModel.c's code runs 28 bytes short and 314 of 650
                         * words differ */
                        if (material_flags & NJD_FLAG_IGNORE_SPECULAR) {
                            njSetkmColorMode(FLAT(NJD_COLOR_MODE_NORMAL_TRANS));
                        } else {
                            njSetkmColorMode(FLAT(NJD_COLOR_MODE_NORMAL_TRANS));
                        }
                        queue_cursor = njStartVertexNonTexTrans();
                    } else {
                        if (material_flags & NJD_FLAG_IGNORE_SPECULAR) {
                            njSetkmColorMode(FLAT(NJD_COLOR_MODE_NORMAL));
                        } else {
                            njSetkmColorMode(FLAT(NJD_COLOR_MODE_NORMAL));
                        }
                        queue_cursor = njStartVertexNonTexOpaque();
                    }
                } else {
                    if (material_flags & NJD_FLAG_USE_ALPHA) {
                        if (material_flags & NJD_FLAG_IGNORE_SPECULAR) {
                            njSetkmColorMode(NJD_COLOR_MODE_NORMAL_TRANS);
                        } else {
                            njSetkmColorMode(NJD_COLOR_MODE_NORMAL_TRANS);
                        }
                        queue_cursor = njStartVertexNonTexTrans();
                    } else {
                        if (material_flags & NJD_FLAG_IGNORE_SPECULAR) {
                            njSetkmColorMode(NJD_COLOR_MODE_NORMAL);
                        } else {
                            njSetkmColorMode(NJD_COLOR_MODE_NORMAL);
                        }
                        queue_cursor = njStartVertexNonTexOpaque();
                    }
                }
            }
            previous_material_id = material_id;
        } else {
            /* @hack redundant: an empty else, which gives the clip test's load
             * R2, MOV.L @R4,R2 at 0x8c653222; without it, R3, and 2 of 650
             * words differ */
        }

        /* @hack redundant: a #pragma asm block of one assembler comment, ahead
         * of which SHC writes the BRA at 0x8c6531ae over its literal pool to
         * the clip test at 0x8c653220; without it, 212 of 650 words differ, the
         * function runs 6 bytes short, and 123 differ beside it */
        /* clang-format off */
#ifndef __clang__
#pragma asm
; meshset dispatch;
#pragma endasm
#endif
        /* clang-format on */
        if (model_clip_or == 0) {
            switch (shader_flags) {
            case 0:
                model_draw_smooth_polygons(meshset, material);
                break;
            case NJD_FLAG_USE_TEXTURE:
                model_draw_smooth_tex_polygons(meshset, material);
                break;
            case NJD_FLAG_USE_TEXTURE | NJD_FLAG_USE_ENV:
                model_draw_smooth_env_polygons(meshset, material);
                break;
            case NJD_FLAG_USE_FLAT:
                model_draw_flat_polygons(meshset, material);
                break;
            case NJD_FLAG_USE_FLAT | NJD_FLAG_USE_TEXTURE:
                model_draw_flat_tex_polygons(meshset, material);
                break;
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_TEXTURE:
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_FLAT |
                NJD_FLAG_USE_TEXTURE:
                model_draw_unlit_tex_polygons(meshset, material);
                break;
            case NJD_FLAG_USE_FLAT | NJD_FLAG_USE_TEXTURE | NJD_FLAG_USE_ENV:
                model_draw_flat_env_polygons(meshset, material);
                break;
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_TEXTURE |
                NJD_FLAG_USE_ENV:
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_FLAT |
                NJD_FLAG_USE_TEXTURE | NJD_FLAG_USE_ENV:
                model_draw_unlit_env_polygons(meshset, material);
                break;
            case NJD_FLAG_IGNORE_LIGHT:
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_FLAT:
                model_draw_unlit_polygons(meshset, material);
                break;
            }
        } else {
            switch (shader_flags) {
            case 0:
                model_draw_smooth_polygons_clip(meshset, material);
                break;
            case NJD_FLAG_USE_TEXTURE:
                model_draw_smooth_tex_polygons_clip(meshset, material);
                break;
            case NJD_FLAG_USE_TEXTURE | NJD_FLAG_USE_ENV:
                model_draw_smooth_env_polygons_clip(meshset, material);
                break;
            case NJD_FLAG_USE_FLAT:
                model_draw_flat_polygons_clip(meshset, material);
                break;
            case NJD_FLAG_USE_FLAT | NJD_FLAG_USE_TEXTURE:
                model_draw_flat_tex_polygons_clip(meshset, material);
                break;
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_TEXTURE:
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_FLAT |
                NJD_FLAG_USE_TEXTURE:
                model_draw_unlit_tex_polygons_clip(meshset, material);
                break;
            case NJD_FLAG_USE_FLAT | NJD_FLAG_USE_TEXTURE | NJD_FLAG_USE_ENV:
                model_draw_flat_env_polygons_clip(meshset, material);
                break;
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_TEXTURE |
                NJD_FLAG_USE_ENV:
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_FLAT |
                NJD_FLAG_USE_TEXTURE | NJD_FLAG_USE_ENV:
                model_draw_unlit_env_polygons_clip(meshset, material);
                break;
            case NJD_FLAG_IGNORE_LIGHT:
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_FLAT:
                model_draw_unlit_polygons_clip(meshset, material);
                break;
            default:
                /* @hack redundant: a default: break; closing the switch, which
                 * leaves meshset_index's 0 in R2, in the delay slot at
                 * 0x8c652e46 of the JMP to the loop test, to be stored there
                 * with its increments; without it, 598 of 650 words differ,
                 * the function runs 8 bytes long, and 1 of
                 * model_draw_unlit_polygons_clip's 980 differs */
                break;
            }
        }
        /* @hack redundant: a #pragma asm block of one assembler comment, ahead
         * of which SHC writes the BRA at 0x8c653394 over its literal pool to
         * the call of njEndVertex at 0x8c653410; without it, 140 of 650 words
         * differ, the function runs 4 bytes short, and 124 differ beside it */
        /* clang-format off */
#ifndef __clang__
#pragma asm
; meshset dispatch;
#pragma endasm
#endif
        /* clang-format on */

        njEndVertex(queue_cursor);
        meshset++;
    }
}

/* draw object's tree through njDrawObjectSub, in the light setup _njDrawModel
 * makes */
void _njDrawObject(NJS_OBJECT *object)
{
    njSetQueue();
    njSetLightMaterials();
    njCalcLightMatrices();
    njDrawObjectSub(object);
    njResetQueue();
}

/* draw object, its siblings, and the children of each unless NJD_EVAL_BREAK.
 * a node draws its model, unless NJD_EVAL_HIDE, and its children in the
 * current matrix moved by its position, its angles and its scale, but for
 * those its NJD_EVAL_UNIT_ bits leave out. the angles turn in ZXY order under
 * NJD_EVAL_ZXY_ANG, else in XYZ. the matrix is pushed for a node and popped
 * after, unless the node leaves out all three. object must not be null, nor a
 * node's model without NJD_EVAL_HIDE, nor its child without NJD_EVAL_BREAK */
void _njDrawObjectSub(NJS_OBJECT *object)
{
    do {
        switch (object->evalflags &
                (NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL)) {
        case 0:
            njPushMatrix(NULL);
            njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
            if (object->evalflags & NJD_EVAL_ZXY_ANG) {
                njRotateZXY(NULL, object->ang[0], object->ang[1],
                            object->ang[2]);
            } else {
                njRotateXYZ(NULL, object->ang[0], object->ang[1],
                            object->ang[2]);
            }
            njScale(NULL, object->scl[0], object->scl[1], object->scl[2]);
            if (!(object->evalflags & NJD_EVAL_HIDE)) {
                njDrawModelSub(object->model);
            }
            if (!(object->evalflags & NJD_EVAL_BREAK)) {
                njDrawObjectSub(object->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_POS:
            njPushMatrix(NULL);
            if (object->evalflags & NJD_EVAL_ZXY_ANG) {
                njRotateZXY(NULL, object->ang[0], object->ang[1],
                            object->ang[2]);
            } else {
                njRotateXYZ(NULL, object->ang[0], object->ang[1],
                            object->ang[2]);
            }
            njScale(NULL, object->scl[0], object->scl[1], object->scl[2]);
            if (!(object->evalflags & NJD_EVAL_HIDE)) {
                njDrawModelSub(object->model);
            }
            if (!(object->evalflags & NJD_EVAL_BREAK)) {
                njDrawObjectSub(object->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_ANG:
            njPushMatrix(NULL);
            njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
            njScale(NULL, object->scl[0], object->scl[1], object->scl[2]);
            if (!(object->evalflags & NJD_EVAL_HIDE)) {
                njDrawModelSub(object->model);
            }
            if (!(object->evalflags & NJD_EVAL_BREAK)) {
                njDrawObjectSub(object->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG:
            njPushMatrix(NULL);
            njScale(NULL, object->scl[0], object->scl[1], object->scl[2]);
            if (!(object->evalflags & NJD_EVAL_HIDE)) {
                njDrawModelSub(object->model);
            }
            if (!(object->evalflags & NJD_EVAL_BREAK)) {
                njDrawObjectSub(object->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_SCL:
            njPushMatrix(NULL);
            njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
            if (object->evalflags & NJD_EVAL_ZXY_ANG) {
                njRotateZXY(NULL, object->ang[0], object->ang[1],
                            object->ang[2]);
            } else {
                njRotateXYZ(NULL, object->ang[0], object->ang[1],
                            object->ang[2]);
            }
            if (!(object->evalflags & NJD_EVAL_HIDE)) {
                njDrawModelSub(object->model);
            }
            if (!(object->evalflags & NJD_EVAL_BREAK)) {
                njDrawObjectSub(object->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_SCL:
            njPushMatrix(NULL);
            if (object->evalflags & NJD_EVAL_ZXY_ANG) {
                njRotateZXY(NULL, object->ang[0], object->ang[1],
                            object->ang[2]);
            } else {
                njRotateXYZ(NULL, object->ang[0], object->ang[1],
                            object->ang[2]);
            }
            if (!(object->evalflags & NJD_EVAL_HIDE)) {
                njDrawModelSub(object->model);
            }
            if (!(object->evalflags & NJD_EVAL_BREAK)) {
                njDrawObjectSub(object->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL:
            njPushMatrix(NULL);
            njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
            if (!(object->evalflags & NJD_EVAL_HIDE)) {
                njDrawModelSub(object->model);
            }
            if (!(object->evalflags & NJD_EVAL_BREAK)) {
                njDrawObjectSub(object->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL:
            if (!(object->evalflags & NJD_EVAL_HIDE)) {
                njDrawModelSub(object->model);
            }
            if (!(object->evalflags & NJD_EVAL_BREAK)) {
                njDrawObjectSub(object->child);
            }
            break;
        }
        object = object->sibling;
    } while (object);
}

/* the public entries save the caller's R8 and restore it after. njDrawModel
 * and njDrawObject also draw between njGetkmPointer and njSetkmPointer, which
 * save and restore the vertex context's pointers */
void njDrawModel(NJS_MODEL *model)
{
    Uint32 *saved_cursor;

    njGetkmPointer();
    save_queue_cursor(&saved_cursor);
    _njDrawModel(model);
    restore_queue_cursor(&saved_cursor);
    njSetkmPointer();
}

Sint32 njCalcModel(NJS_MODEL *model)
{
    Uint32 *saved_cursor;

    /* @hack type: the argument and the result in one volatile union, one
     * stack slot kept across the save, MOV.L R4,@(4,R15) at 0x8c653752 and
     * MOV.L R0,@(4,R15) at 0x8c653760; without volatile, 16 of 16 words
     * differ, the function runs 6 bytes long, and 28 differ in 6 functions
     * beside it; as a plain result local, 15 of 16 differ, it runs 4 bytes
     * short, and 25 differ in 5 functions beside it */
    volatile union {
        NJS_MODEL *model;
        Sint32 result;
    } call;

    call.model = model;
    save_queue_cursor(&saved_cursor);
    call.result = _njCalcModel(call.model);
    restore_queue_cursor(&saved_cursor);
    return call.result;
}

void njDrawModelSub(NJS_MODEL *volatile model)
{
    Uint32 *saved_cursor;

    save_queue_cursor(&saved_cursor);
    _njDrawModelSub(model);
    restore_queue_cursor(&saved_cursor);
}

void njDrawObject(NJS_OBJECT *object)
{
    Uint32 *saved_cursor;

    njGetkmPointer();
    save_queue_cursor(&saved_cursor);
    _njDrawObject(object);
    restore_queue_cursor(&saved_cursor);
    njSetkmPointer();
}

void njDrawObjectSub(NJS_OBJECT *volatile object)
{
    Uint32 *saved_cursor;

    save_queue_cursor(&saved_cursor);
    _njDrawObjectSub(object);
    restore_queue_cursor(&saved_cursor);
}
