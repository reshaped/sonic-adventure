/* the simple draws, njSimpleDrawModel and njSimpleDrawObject, lit by
 * _nj_light_control_'s first light. njSimpleCalcModel projects a model's points
 * through XMTRX into _nj_vertex_buf_ and, unless NJD_CONTROL_3D_NO_CLIP_CHECK,
 * flags them against the clip planes. the drawers send each meshset through the
 * store queues as its material's flags ask: lit smooth or flat, or unlit, and
 * untextured, textured or environment mapped. they cull polygons by their
 * winding, and cut them at the near plane where a point lies nearer. a strip
 * goes through the strip helpers, inline assembly that keeps its lighting terms
 * in the second float bank, XF0 to XF15 */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>

/* one vertex in a store queue: VERTEX_BYTES bytes, or VERTEX_WORDS Float
 * words. a byte count shifted right by VERTEX_SHIFT counts vertices */
#define VERTEX_BYTES 32
#define VERTEX_WORDS 8
#define VERTEX_SHIFT 5

/* a strip's point count: the low 15 bits of its first word, whose top bit is
 * NJD_TRIMESH_END */
#define STRIP_COUNT 0x7fff
/* the low 14 bits of a meshset's type_matId: its material's index */
#define MESHSET_MATERIAL 0x3fff
/* the low 30 bits of a material's attr_texId: its texture's number in the
 * current texture list */
#define MATERIAL_TEXTURE 0x3fffffff
/* the material flags that pick a meshset's drawer. NJD_FLAG_USE_ENV without
 * NJD_FLAG_USE_TEXTURE picks none */
#define DRAWER_FLAGS                                                           \
    (NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_FLAT | NJD_FLAG_USE_ENV |            \
     NJD_FLAG_USE_TEXTURE)
#define EVAL_UNIT (NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL)

/* a point's clip flags, which loops 3 and 4 set. CLIP_NEAR marks a point
 * nearer than the near plane, its view z less than _nj_near_clip_, and
 * CLIP_FAR one whose view z is greater than _nj_far_clip_. CLIP_SIDES' four
 * bits, from bit 2, test the screen position: sx less than work->clip_left, sx
 * greater than clip_right, sy less than clip_top and sy greater than
 * clip_bottom. a point with CLIP_NEAR takes no side flag */
#define CLIP_NEAR 1
#define CLIP_FAR 2
#define CLIP_SIDES 0x3c
#define CLIP_ALL 0x3f

/* a model's texture coordinates are in units of 1/256 */
#define TEX_SCALE (1.0F / 256.0F)

/* the color modes the drawers set. a _FLAT_ mode leaves out
 * COLOR_MODE_GOURAUD, so each triangle takes its color from the vertex that
 * completes it */
#define MODE_TEX_OPAQUE                                                        \
    (COLOR_MODE_OPAQUE | COLOR_MODE_TEXTURE | COLOR_MODE_INTENSITY)
#define MODE_TEX_TRANS                                                         \
    (COLOR_MODE_TRANS | COLOR_MODE_TEXTURE | COLOR_MODE_INTENSITY)
#define MODE_TEX_FLAT_OPAQUE (MODE_TEX_OPAQUE & ~COLOR_MODE_GOURAUD)
#define MODE_TEX_FLAT_TRANS (MODE_TEX_TRANS & ~COLOR_MODE_GOURAUD)
#define MODE_FLAT_OPAQUE (NJD_COLOR_MODE_NORMAL & ~COLOR_MODE_GOURAUD)
#define MODE_FLAT_TRANS (NJD_COLOR_MODE_NORMAL_TRANS & ~COLOR_MODE_GOURAUD)

/* the argument of njSimpleCalcModel_Loop1 to Loop4, whose bodies read its
 * fields by offset, in this order. only loops 3 and 4 read clip_and, clip_or
 * and the clip area */
struct calc_work {
    Sint32 count;
    Uint8 *out;
    NJS_POINT3 *points;
    NJS_VECTOR *normals;
    Uint32 clip_and;
    Uint32 clip_or;
    /* the screen x per unit of x / z, and the screen y per unit of y / z */
    Float scale_x, scale_y;
    Float clip_left, clip_right, clip_top, clip_bottom;
};

/* a point as the loops leave it in _nj_vertex_buf_, one 32-byte cache line */
struct calc_vertex {
    NJS_VECTOR normal;
    Uint32 clip;
    /* the screen position, and oow 1/|z|. a point with CLIP_NEAR holds its
     * view x, y and z in them instead */
    Float sx, sy;
    Float oow;
    /* no loop stores this word, so each record takes what its store queue
     * held. the smooth clippers, NJM_SET_POLY_STRIP and EMIT_SMOOTH_POLYGON
     * store the point's SIMPLE_INTENSITY here */
    Float intensity;
};

/* the store queue address of the next vertex the drawers write. it lives in
 * R8, where the strip helpers' bodies read and step it */
#pragma global_register(sq_cursor = R8)
Float *sq_cursor;

/* the current material's attrflags, after NJD_CONTROL_3D_CONSTANT_ATTR's
 * masks */
static Uint32 material_flags;
/* the OR of the clip flags of the model's points, which _njSimpleCalcModel
 * sets. 0, as under NJD_CONTROL_3D_NO_CLIP_CHECK, picks the drawers that don't
 * clip */
static Uint32 model_clip_or;
/* the current matrix in memory, which ENV_COLUMN_DOT reads. _njSimpleCalcModel
 * and DRAW_OBJECT_CONTENTS set it to _nj_current_matrix_ptr_, and
 * DRAW_OBJECT_CONTENTS stores XMTRX there */
NJS_MATRIX *mp;
/* the vector of _nj_light_control_'s first light, turned by the light's
 * matrix and then the view matrix. _njSimpleDrawModel and _njSimpleDrawObject
 * set it, base_light_scale and iamb */
static NJS_VECTOR base_light_direction;
/* base_light_direction in model space, as a unit vector times
 * base_light_scale. _njSimpleCalcModel sets it for each model */
NJS_VECTOR light2;
/* the light's diffuse intensity */
static Float base_light_scale;
/* the light's ambient intensity */
Float iamb;
/* the strip meshset a polygon drawer passes to its strip drawer */
NJS_MESHSET *_njm_mesh;
/* the screen x per unit of x / z, and the screen y per unit of y / z */
static Float projection_x;
static Float projection_y;
/* the part of a triangle a clipper keeps. clipped_vertices holds its corners,
 * three or four in order round it, and clipped_uv their texture coordinates,
 * in units of 1/256 */
static NJS_TEX clipped_uv[4];
static struct calc_vertex clipped_vertices[4];
/* a word of bss that no word in 1ST_READ.BIN points into. it keeps
 * clip_cull_requested and corner_uv_window at the image's addresses */
static Uint32 dat_8c8c262c;
/* whether the next clipper drops its triangle when it faces away. a clipper
 * that sends its triangle clears it */
static Sint32 clip_cull_requested;
/* a word of bss that no word in 1ST_READ.BIN points into. it keeps
 * corner_uv_window at the image's address */
static Uint32 dat_8c8c2634;
/* the texture coordinates a drawer passes to a clipper, in meshset->vertuv's
 * type: a triangle's three, a quad's four, or the three of an N-gon's current
 * triangle. for an N-gon, the drawer steps the window one corner at a time */
static NJS_COLOR corner_uv_window[4];

/* the inline bodies below, which SHC alone compiles: clang sees only these
 * declarations */
/* NOLINTBEGIN(clang-diagnostic-undefined-internal) */
static void loop1_body(struct calc_work *work, NJS_MATRIX *m);
static void loop2_body(struct calc_work *work, NJS_MATRIX *m);
static void loop3_body(struct calc_work *work, NJS_MATRIX *m);
static void loop4_body(struct calc_work *work, NJS_MATRIX *m);
static void simple_draw_noop(void);
static void save_queue_cursor(Float **slot);
static void restore_queue_cursor(Float **slot);
/* NOLINTEND(clang-diagnostic-undefined-internal) */
void SimpleCalcModelMatrix(NJS_POINT3 *out, NJS_POINT3 *in);
void SimpleCalcModelLight(NJS_VECTOR *out, NJS_VECTOR *in, Float scale);
void NJM_SET_POLY_STRIP(struct calc_vertex *vertex, Uint32 pcw);
void NJM_SET_POLY_X_STRIP(struct calc_vertex *vertex, Uint32 pcw);
void _NJM_SET_TEX_STRIP(struct calc_vertex *vertex, Uint32 pcw, NJS_TEX *uv);
void _NJM_SET_TEX_X_STRIP(struct calc_vertex *vertex, Uint32 pcw, NJS_TEX *uv);
void NJM_SET_ENV_STRIP(struct calc_vertex *vertex, Uint32 pcw);
void NJM_SET_ENV_X_STRIP(struct calc_vertex *vertex, Uint32 pcw);
void NJM_SET_POLY_STRIP_START(void);
void NJM_SET_POLY_X_STRIP_START(void);
void NJM_SET_TEX_STRIP_START(void);
void NJM_SET_TEX_X_STRIP_START(void);
void NJM_SET_ENV_STRIP_START(void);
void NJM_SET_ENV_X_STRIP_START(void);
void NJM_SET_POLY_STRIP_END(void);
void NJM_SET_POLY_X_STRIP_END(void);
void NJM_SET_TEX_STRIP_END(void);
void NJM_SET_TEX_X_STRIP_END(void);
void NJM_SET_ENV_STRIP_END(void);
void NJM_SET_ENV_X_STRIP_END(void);

/* clang-format off */
#ifndef __clang__
/* @asm opcode: cache-line, OCBI @R10 at 0x8c62bbb2, in njSimpleCalcModel_Loop1,
 * invalidates a record's cache line */
#pragma inline_asm(loop1_body)
static void loop1_body(struct calc_work *work, NJS_MATRIX *m)
{
        FMOV.S      FR15,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR12,@-R15
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R8,@-R15
        MOV         R4,R0
        MOV.L       @R0+,R8
        MOV.L       @R0+,R2
        MOV.L       @R0+,R4
        MOV.L       @R0+,R5
        ADD         #8,R0
        FMOV.S      @R0+,FR6
        FMOV.S      @R0+,FR7
        MOV.L       #__nj_screen_+12,R0
        FMOV.S      @R0+,FR14
        FMOV.S      @R0+,FR15
        MOV         R2,R10
        MOV.L       #H'1C000000,R0
        MOV.L       #H'FF000038,R1
        AND         R2,R0
        SHLR16      R0
        SHLR8       R0
        MOV.L       R0,@(0,R1)
        MOV.L       R0,@(4,R1)
        MOV.L       #H'03FFFFFF,R0
        MOV.L       #H'E0000000,R1
        AND         R0,R2
        OR          R1,R2
        MOV         R2,R3
        MOV         R10,R11
        ADD         #32,R3
        ADD         #32,R11
        TST         R8,R8
        BT          ?done
        MOV         #0,R1
        FLDI1       FR3
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR1
        FMOV.S      @R4+,FR2
        BRA         ?project
        FTRV        XMTRX,FV0
?store:
        FMOV.S      @R5+,FR10
        FMOV.S      @R5+,FR11
        FMOV.S      @R5+,FR12
        FMUL        FR2,FR1
        FMUL        FR2,FR0
        OCBI        @R10
        FADD        FR15,FR1
        FADD        FR14,FR0
        ADD         #28,R2
        FMOV.S      FR2,@-R2
        FMOV.S      FR1,@-R2
        FMOV.S      FR0,@-R2
        MOV.L       R1,@-R2
        FMOV.S      FR12,@-R2
        FMOV.S      FR11,@-R2
        FMOV.S      FR10,@-R2
        FLDI1       FR3
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR1
        FMOV.S      @R4+,FR2
        PREF        @R2
        FTRV        XMTRX,FV0
        DT          R8
        ADD         #32,R2
        BT/S        ?done
        ADD         #32,R10
        PREF        @R5
?project:
        FMUL        FR2,FR2
        FSRRA       FR2
        FMUL        FR7,FR1
        FMUL        FR6,FR0
        BRA         ?store
        PREF        @R4
?done:
        MOV.L       @R15+,R8
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        RTS
        FMOV.S      @R15+,FR15
}

/* @asm opcode: cache-line, OCBI @R10 at 0x8c62bc6c, in njSimpleCalcModel_Loop2,
 * invalidates a record's cache line */
#pragma inline_asm(loop2_body)
static void loop2_body(struct calc_work *work, NJS_MATRIX *m)
{
        FMOV.S      FR15,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR12,@-R15
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R8,@-R15
        MOV         R4,R0
        MOV.L       @R0+,R8
        MOV.L       @R0+,R2
        MOV.L       @R0+,R4
        MOV.L       @R0+,R5
        ADD         #8,R0
        FMOV.S      @R0+,FR6
        FMOV.S      @R0+,FR7
        MOV.L       #__nj_screen_+12,R0
        FMOV.S      @R0+,FR14
        FMOV.S      @R0+,FR15
        MOV         R2,R10
        MOV.L       #H'1C000000,R0
        MOV.L       #H'FF000038,R1
        AND         R2,R0
        SHLR16      R0
        SHLR8       R0
        MOV.L       R0,@(0,R1)
        MOV.L       R0,@(4,R1)
        MOV.L       #H'03FFFFFF,R0
        MOV.L       #H'E0000000,R1
        AND         R0,R2
        OR          R1,R2
        MOV         R2,R3
        MOV         R10,R11
        ADD         #32,R3
        ADD         #32,R11
        TST         R8,R8
        BT          ?done
        MOV         #0,R1
        FLDI1       FR3
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR1
        FMOV.S      @R4+,FR2
        BRA         ?project
        FTRV        XMTRX,FV0
?store:
        FMUL        FR2,FR1
        FMUL        FR2,FR0
        OCBI        @R10
        FADD        FR15,FR1
        FADD        FR14,FR0
        ADD         #28,R2
        FMOV.S      FR2,@-R2
        FMOV.S      FR1,@-R2
        FMOV.S      FR0,@-R2
        MOV.L       R1,@-R2
        FLDI1       FR3
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR1
        FMOV.S      @R4+,FR2
        PREF        @R2
        FTRV        XMTRX,FV0
        DT          R8
        ADD         #20,R2
        BT/S        ?done
        ADD         #32,R10
        PREF        @R5
?project:
        FMUL        FR2,FR2
        FSRRA       FR2
        FMUL        FR7,FR1
        FMUL        FR6,FR0
        BRA         ?store
        PREF        @R4
?done:
        MOV.L       @R15+,R8
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        RTS
        FMOV.S      @R15+,FR15
}

/* @asm opcode: cache-line, OCBI @R10 at 0x8c62bd52, in njSimpleCalcModel_Loop3,
 * invalidates a record's cache line */
#pragma inline_asm(loop3_body)
static void loop3_body(struct calc_work *work, NJS_MATRIX *m)
{
        FMOV.S      FR15,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR12,@-R15
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R8,@-R15
        MOV.L       R4,@-R15
        ADD         #-16,R15
        MOV.L       @R4+,R8
        MOV.L       @R4+,R2
        MOV.L       @R4+,R0
        MOV.L       @R4+,R5
        MOV.L       @R4+,R6
        MOV.L       @R4+,R7
        FMOV.S      @R4+,FR6
        FMOV.S      @R4+,FR7
        FMOV.S      @R4+,FR10
        FMOV.S      @R4+,FR11
        FMOV.S      @R4+,FR12
        FMOV.S      @R4+,FR13
        MOV.L       #__nj_screen_+12,R4
        FMOV.S      @R4+,FR14
        FMOV.S      @R4+,FR15
        MOV.L       #__nj_near_clip_,R4
        FMOV.S      @R4,FR8
        MOV.L       #__nj_far_clip_,R4
        FMOV.S      @R4,FR9
        MOV         R0,R4
        MOV         R2,R10
        MOV.L       #H'1C000000,R0
        MOV.L       #H'FF000038,R1
        AND         R2,R0
        SHLR16      R0
        SHLR8       R0
        MOV.L       R0,@(0,R1)
        MOV.L       R0,@(4,R1)
        MOV.L       #H'03FFFFFF,R0
        MOV.L       #H'E0000000,R1
        AND         R0,R2
        OR          R1,R2
        MOV         R2,R3
        MOV         R10,R11
        ADD         #32,R3
        ADD         #32,R11
        TST         R8,R8
        BT          ?done
        FLDI1       FR3
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR1
        FMOV.S      @R4+,FR2
        BRA         ?project
        FTRV        XMTRX,FV0
?sides:
        FMOV.S      FR4,FR3
        FMUL        FR4,FR1
        FMUL        FR4,FR0
        FADD        FR15,FR1
        FADD        FR14,FR0
        FCMP/GT     FR13,FR1
        ROTCL       R1
        FCMP/GT     FR1,FR12
        ROTCL       R1
        FCMP/GT     FR11,FR0
        ROTCL       R1
        FCMP/GT     FR0,FR10
        ROTCL       R1
?depth:
        OCBI        @R10
        FCMP/GT     FR9,FR2
        ROTCL       R1
        FCMP/GT     FR2,FR8
        ROTCL       R1
        ADD         #28,R2
        FMOV.S      FR3,@-R2
        FMOV.S      FR1,@-R2
        FMOV.S      FR0,@-R2
        FMOV.S      @R5+,FR2
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR0
        MOV.L       R1,@-R2
        FMOV.S      FR0,@-R2
        FMOV.S      FR1,@-R2
        FMOV.S      FR2,@-R2
        FLDI1       FR3
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR1
        FMOV.S      @R4+,FR2
        PREF        @R2
        FTRV        XMTRX,FV0
        AND         R1,R6
        OR          R1,R7
        DT          R8
        ADD         #32,R2
        BT/S        ?done
        ADD         #32,R10
        PREF        @R5
?project:
        MOV         #0,R1
        FMOV.S      FR2,FR4
        FMUL        FR2,FR4
        FCMP/GT     FR2,FR8
        BT/S        ?depth
        FMOV.S      FR2,FR3
        PREF        @R4
        FSRRA       FR4
        FMUL        FR7,FR1
        BRA         ?sides
        FMUL        FR6,FR0
?done:
        ADD         #16,R15
        MOV.L       @R15+,R4
        MOV.L       R6,@(16,R4)
        MOV.L       R7,@(20,R4)
        MOV.L       @R15+,R8
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        RTS
        FMOV.S      @R15+,FR15
}

/* @asm opcode: cache-line, OCBI @R10 at 0x8c62be5e, in njSimpleCalcModel_Loop4,
 * invalidates a record's cache line */
#pragma inline_asm(loop4_body)
static void loop4_body(struct calc_work *work, NJS_MATRIX *m)
{
        FMOV.S      FR15,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR12,@-R15
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R8,@-R15
        MOV.L       R4,@-R15
        ADD         #-16,R15
        MOV.L       @R4+,R8
        MOV.L       @R4+,R2
        MOV.L       @R4+,R0
        ADD         #4,R4
        MOV.L       @R4+,R6
        MOV.L       @R4+,R7
        FMOV.S      @R4+,FR6
        FMOV.S      @R4+,FR7
        FMOV.S      @R4+,FR10
        FMOV.S      @R4+,FR11
        FMOV.S      @R4+,FR12
        FMOV.S      @R4+,FR13
        MOV.L       #__nj_screen_+12,R4
        FMOV.S      @R4+,FR14
        FMOV.S      @R4+,FR15
        MOV.L       #__nj_near_clip_,R4
        FMOV.S      @R4,FR8
        MOV.L       #__nj_far_clip_,R4
        FMOV.S      @R4,FR9
        MOV         R0,R4
        MOV         R2,R10
        MOV.L       #H'1C000000,R0
        MOV.L       #H'FF000038,R1
        AND         R2,R0
        SHLR16      R0
        SHLR8       R0
        MOV.L       R0,@(0,R1)
        MOV.L       R0,@(4,R1)
        MOV.L       #H'03FFFFFF,R0
        MOV.L       #H'E0000000,R1
        AND         R0,R2
        OR          R1,R2
        MOV         R2,R3
        MOV         R10,R11
        ADD         #32,R3
        ADD         #32,R11
        TST         R8,R8
        BT          ?done
        FLDI1       FR3
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR1
        FMOV.S      @R4+,FR2
        BRA         ?project
        FTRV        XMTRX,FV0
?sides:
        FMOV.S      FR4,FR3
        FMUL        FR4,FR1
        FMUL        FR4,FR0
        FADD        FR15,FR1
        FADD        FR14,FR0
        FCMP/GT     FR13,FR1
        ROTCL       R1
        FCMP/GT     FR1,FR12
        ROTCL       R1
        FCMP/GT     FR11,FR0
        ROTCL       R1
        FCMP/GT     FR0,FR10
        ROTCL       R1
?depth:
        OCBI        @R10
        FCMP/GT     FR9,FR2
        ROTCL       R1
        FCMP/GT     FR2,FR8
        ROTCL       R1
        ADD         #28,R2
        FMOV.S      FR3,@-R2
        FMOV.S      FR1,@-R2
        FMOV.S      FR0,@-R2
        MOV.L       R1,@-R2
        FLDI1       FR3
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR1
        FMOV.S      @R4+,FR2
        PREF        @R2
        FTRV        XMTRX,FV0
        AND         R1,R6
        OR          R1,R7
        DT          R8
        ADD         #20,R2
        BT/S        ?done
        ADD         #32,R10
        PREF        @R5
?project:
        MOV         #0,R1
        FMOV.S      FR2,FR4
        FMUL        FR2,FR4
        FCMP/GT     FR2,FR8
        BT/S        ?depth
        FMOV.S      FR2,FR3
        PREF        @R4
        FSRRA       FR4
        FMUL        FR7,FR1
        BRA         ?sides
        FMUL        FR6,FR0
?done:
        ADD         #16,R15
        MOV.L       @R15+,R4
        MOV.L       R6,@(16,R4)
        MOV.L       R7,@(20,R4)
        MOV.L       @R15+,R8
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        RTS
        FMOV.S      @R15+,FR15
}
#endif
/* clang-format on */

/* the loops project the work->count points at work->points through XMTRX, not
 * m, into the calc_vertex records at work->out. a record's sx and sy are the
 * transformed point's x and y times work->scale_x and work->scale_y, times
 * 1/|z|, plus the screen's center, and its oow is 1/|z|. loops 3 and 4 also
 * set each point's clip flags, leave a point with CLIP_NEAR as its view x, y
 * and z, and AND the flags into work->clip_and and OR them into work->clip_or.
 *
 * the loops write the records through the store queues, and set QACR0 and
 * QACR1 from work->out, even for a count of 0. a queue sends all 32 bytes of a
 * record, so a word a loop doesn't store takes what the queue held: the
 * intensity in every loop, and the normal in loops 2 and 4. the store queues
 * write past the operand cache, so the loops invalidate each record's line.
 *
 * each loop loads the next point before it tests the count, so a positive
 * count reads one point past the last. FPSCR.SZ and FPSCR.PR must be 0 */

/* copy each point's normal from work->normals, and store 0 as its clip
 * flags */
void njSimpleCalcModel_Loop1(struct calc_work *work, NJS_MATRIX *m)
{
    loop1_body(work, m);
}

/* store 0 as each point's clip flags, and no normal. it still reads
 * work->normals, and prefetches through it before each point after the first */
void njSimpleCalcModel_Loop2(struct calc_work *work, NJS_MATRIX *m)
{
    loop2_body(work, m);
}

/* copy each point's normal from work->normals, and set its clip flags */
void njSimpleCalcModel_Loop3(struct calc_work *work, NJS_MATRIX *m)
{
    loop3_body(work, m);
}

/* set each point's clip flags, and store no normal. it prefetches through m
 * before each point after the first */
void njSimpleCalcModel_Loop4(struct calc_work *work, NJS_MATRIX *m)
{
    loop4_body(work, m);
}

/* clang-format off */
#ifndef __clang__
/* set out to the x, y and z of XMTRX times (in, 1). out may be in */
/* @asm open: no inspect rule reads FV0 filled by three loads and FLDI1 FR3
 * before its FTRV */
#pragma inline_asm(SimpleCalcModelMatrix)
void SimpleCalcModelMatrix(NJS_POINT3 *out, NJS_POINT3 *in)
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

/* set out to the x, y and z of XMTRX's transpose times (in, 0), scaled to
 * length scale. out may be in */
/* @asm operand: fipr-registers, FIPR FV0,FV4 at 0x8c62bee4, 0x8c62bef0 and
 * 0x8c62befc and FIPR FV8,FV0 at 0x8c62bf08 */
#pragma inline_asm(SimpleCalcModelLight)
void SimpleCalcModelLight(NJS_VECTOR *out, NJS_VECTOR *in, Float scale)
{
        FMOV.S      FR4,FR11
        FMOV.S      @R5+,FR0
        FMOV.S      @R5+,FR1
        FMOV.S      @R5+,FR2
        FLDI0       FR3
        FSCHG
        FMOV.S      FR1,FR4
        FMOV.S      FR3,FR6
        FIPR        FV0,FV4
        FSCHG
        FMOV.S      FR7,FR8
        FSCHG
        FMOV.S      FR5,FR4
        FMOV.S      FR7,FR6
        FIPR        FV0,FV4
        FSCHG
        FMOV.S      FR7,FR9
        FSCHG
        FMOV.S      FR9,FR4
        FMOV.S      FR11,FR6
        FIPR        FV0,FV4
        FSCHG
        FMOV.S      FR7,FR10
        FMOV.S      FR8,FR0
        FMOV.S      FR9,FR1
        FMOV.S      FR10,FR2
        FIPR        FV8,FV0
        ADD         #12,R4
        FSRRA       FR3
        FMUL        FR11,FR3
        FMUL        FR3,FR10
        FMUL        FR3,FR9
        FMUL        FR3,FR8
        FMOV.S      FR10,@-R4
        FMOV.S      FR9,@-R4
        FMOV.S      FR8,@-R4
}
#endif
/* clang-format on */

/* project model as njSimpleCalcModel does, inside the caller's own save and
 * restore of R8 */
Sint32 _njSimpleCalcModel(NJS_MODEL *model)
{
    struct calc_work work;
    NJS_POINT3 view_center;
    register Uint8 *output;
    register NJS_POINT3 *points;
    NJS_POINT3 *source_points;
    register NJS_VECTOR *normals;
    NJS_SCREEN *screen;
    Float radius;
    Float screen_center_x;
    register Float sphere_reciprocal;
    register Float left;
    register Float top;
    register Float right;
    register Float bottom;

    output = (Uint8 *)(struct calc_vertex *)(Uint32)_nj_vertex_buf_;
    points = model->points;
    normals = model->normals;
    model_clip_or = 0;
    mp = _nj_current_matrix_ptr_;
    screen = &_nj_screen_;
    projection_x = screen->dist * _nj_scr_aspect_x_;
    projection_y = screen->dist * _nj_scr_aspect_y_;

    if (_nj_control_3d_flag_ & NJD_CONTROL_3D_MODEL_CLIP) {
        SimpleCalcModelMatrix(&view_center, &model->center);
        radius = model->r;
        if (view_center.z + radius <= _nj_near_clip_) {
            return -1;
        }
        if (view_center.z - radius > _nj_far_clip_) {
            return -1;
        }
        sphere_reciprocal = 1.0F / (view_center.z + radius);

        /* @hack spelling: screen_center_x set inside the first sum with a
         * comma, which loads projection_x into FR9 and the view center's x into
         * FR8 (0x8c62c002, 0x8c62c006); with screen->cx read in each sum, the
         * two trade registers and 8 of 301 words differ */
        left = (projection_x * (view_center.x - radius) * sphere_reciprocal) +
               (screen_center_x = screen->cx, screen_center_x);
        top = (projection_y * (view_center.y - radius) * sphere_reciprocal) +
              screen->cy;
        right = ((view_center.x + radius) * projection_x * sphere_reciprocal) +
                screen_center_x;
        bottom = ((view_center.y + radius) * projection_y * sphere_reciprocal) +
                 screen->cy;
        if (_nj_2d_clip_[0].x > right || left > _nj_2d_clip_[1].x ||
            _nj_2d_clip_[0].y > bottom || top > _nj_2d_clip_[1].y) {
            return -1;
        }
    }
    _nj_calc_vertex_ += model->nbPoint;
    SimpleCalcModelLight(&light2, &base_light_direction, base_light_scale);
    source_points = points;
    work.count = model->nbPoint;
    work.out = output;
    work.points = source_points;
    work.scale_x = projection_x;
    work.scale_y = projection_y;

    if (_nj_control_3d_flag_ & NJD_CONTROL_3D_NO_CLIP_CHECK) {
        if (normals) {
            work.normals = normals;
            njSimpleCalcModel_Loop1(&work, mp);
        } else {
            /* @bug uninitialized: work.normals is unset on this path, and
             * njSimpleCalcModel_Loop2 reads it at 0x8c62bc14 and prefetches
             * through it at 0x8c62bc90, before each point after the first */
            njSimpleCalcModel_Loop2(&work, mp);
        }
    } else {
        work.clip_and = -1;
        work.clip_or = 0;
        work.clip_left = _nj_2d_clip_[0].x;
        work.clip_right = _nj_2d_clip_[1].x;
        work.clip_top = _nj_2d_clip_[0].y;
        work.clip_bottom = _nj_2d_clip_[1].y;
        if (normals) {
            work.normals = normals;
            njSimpleCalcModel_Loop3(&work, mp);
        } else {
            njSimpleCalcModel_Loop4(&work, mp);
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
    }
    return 0;
}

/* clang-format off */
#ifndef __clang__
/* an empty function: each strip drawer calls it first, through DRAW_NOOP */
/* @asm open: every instruction is one SHC writes for C */
#pragma inline_asm(simple_draw_noop)
static void simple_draw_noop(void)
{
}

/* the strip helpers write vertex at sq_cursor as a strip's next vertex, with
 * pcw as its PCW, then send it and step sq_cursor past it. they take their
 * terms from the second float bank. the START of a kind saves part of the bank
 * on the stack and loads the kind's terms, and its END loads the saved part
 * back. START returns with R15 lowered past what it saved, and END raises it
 * again. between the two, the caller must not address its frame through R15.
 * FPSCR.SZ and FPSCR.PR must be 0 */

/* lit: the material's diffuse color, its red, green and blue times
 * SIMPLE_INTENSITY(vertex->normal). the helper also stores that intensity in
 * vertex->intensity */
/* @asm operand: fschg-pairs, FMOV.S @R4+,FR8 at 0x8c62c210 and FMOV.S
 * @R4+,FR10 at 0x8c62c212 load DR8 and DR10 as pairs in the run FSCHG at
 * 0x8c62c202 opens */
#pragma inline_asm(NJM_SET_POLY_STRIP(size=62))
void NJM_SET_POLY_STRIP(struct calc_vertex *vertex, Uint32 pcw)
{
        ADD         #16,R8
        FSCHG
        FMOV.S      FR1,FR0
        FMOV.S      FR3,FR2
        FMOV.S      @R4+,FR4
        FMOV.S      @R4+,FR6
        FLDI0       FR3
        FIPR        FV4,FV0
        FMOV.S      @R4+,FR8
        FMOV.S      @R4+,FR10
        FMOV.S      FR9,FR0
        FMOV.S      FR5,FR4
        FSUB        FR3,FR0
        FMOV.S      FR7,FR6
        FSCHG
        FMOV.S      FR10,@-R8
        FMOV.S      FR0,@-R4
        OCBWB       @R4
        FMUL        FR0,FR7
        FMOV.S      FR9,@-R8
        FMUL        FR0,FR6
        FMOV.S      FR8,@-R8
        FMUL        FR0,FR5
        MOV.L       R5,@-R8
        ADD         #32,R8
        FMOV.S      FR7,@-R8
        FMOV.S      FR6,@-R8
        FMOV.S      FR5,@-R8
        FMOV.S      FR4,@-R8
        PREF        @R8
        ADD         #16,R8
}

/* unlit: the material's diffuse color as it is */
/* @asm open: no inspect rule reads the XD0 and XD2 its FMOV.S FR1,FR0 and
 * FR3,FR2 move after FSCHG */
#pragma inline_asm(NJM_SET_POLY_X_STRIP(size=32))
void NJM_SET_POLY_X_STRIP(struct calc_vertex *vertex, Uint32 pcw)
{
        ADD         #32,R8
        FSCHG
        ADD         #16,R4
        FMOV.S      FR1,FR0
        FMOV.S      FR3,FR2
        FMOV.S      @R4+,FR4
        FMOV.S      @R4,FR6
        FMOV.S      FR2,@-R8
        FMOV.S      FR0,@-R8
        FSCHG
        FMOV.S      FR6,@-R8
        FMOV.S      FR5,@-R8
        FMOV.S      FR4,@-R8
        MOV.L       R5,@-R8
        PREF        @R8
        ADD         #32,R8
}

/* lit and textured: u and v are uv's coordinates times 1/256, and the intensity
 * is SIMPLE_INTENSITY(vertex->normal) */
/* @asm operand: fipr-registers, FIPR FV8,FV0 at 0x8c62c274 dots light2, moved
 * from the second bank into FV8, with the normal */
#pragma inline_asm(_NJM_SET_TEX_STRIP(size=62))
void _NJM_SET_TEX_STRIP(struct calc_vertex *vertex, Uint32 pcw, NJS_TEX *uv)
{
        ADD         #16,R8
        FSCHG
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR2
        FLDI0       FR3
        FMOV.S      FR1,FR8
        FMOV.S      FR3,FR10
        FIPR        FV8,FV0
        FMOV.S      @R4+,FR4
        FMOV.S      @R4,FR6
        FMOV.S      FR5,FR8
        FSCHG
        MOV.W       @R6+,R2
        MOV.W       @R6,R3
        FMOV.S      FR6,@-R8
        LDS         R3,FPUL
        FMOV.S      FR5,@-R8
        FSUB        FR3,FR11
        FMOV.S      FR4,@-R8
        FLOAT       FPUL,FR1
        FMUL        FR8,FR1
        LDS         R2,FPUL
        MOV.L       R5,@-R8
        FLOAT       FPUL,FR2
        FMUL        FR8,FR2
        ADD         #28,R8
        FMOV.S      FR11,@-R8
        FMOV.S      FR1,@-R8
        FMOV.S      FR2,@-R8
        PREF        @R8
        ADD         #16,R8
}

/* textured: u and v as _NJM_SET_TEX_STRIP gives them, and the intensity 1 */
/* @asm open: no inspect rule reads the XD0 its FMOV.S FR1,FR0 moves after
 * FSCHG */
#pragma inline_asm(_NJM_SET_TEX_X_STRIP(size=52))
void _NJM_SET_TEX_X_STRIP(struct calc_vertex *vertex, Uint32 pcw, NJS_TEX *uv)
{
        ADD         #16,R4
        FSCHG
        ADD         #16,R8
        FMOV.S      @R4+,FR4
        FMOV.S      @R4,FR6
        FMOV.S      FR1,FR0
        FSCHG
        MOV.W       @R6+,R2
        MOV.W       @R6,R3
        FMOV.S      FR6,@-R8
        LDS         R3,FPUL
        FMOV.S      FR5,@-R8
        FLOAT       FPUL,FR1
        FMOV.S      FR4,@-R8
        FMUL        FR0,FR1
        MOV.L       R5,@-R8
        LDS         R2,FPUL
        ADD         #28,R8
        FLOAT       FPUL,FR2
        FLDI1       FR3
        FMUL        FR0,FR2
        FMOV.S      FR3,@-R8
        FMOV.S      FR1,@-R8
        FMOV.S      FR2,@-R8
        PREF        @R8
        ADD         #16,R8
}

/* lit and environment mapped: u and v are half the x and y of vertex->normal
 * turned by the current matrix, plus 1/2, and the intensity is
 * SIMPLE_INTENSITY(vertex->normal). one FTRV gives all three, through the
 * matrix NJM_SET_ENV_STRIP_START leaves in XMTRX */
/* @asm open: no inspect rule reads FV0 filled by paired loads and FLDI1 FR3
 * before its FTRV */
#pragma inline_asm(NJM_SET_ENV_STRIP(size=38))
void NJM_SET_ENV_STRIP(struct calc_vertex *vertex, Uint32 pcw)
{
        ADD         #16,R8
        FSCHG
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR2
        FLDI1       FR3
        FTRV        XMTRX,FV0
        FMOV.S      @R4+,FR4
        FMOV.S      @R4,FR6
        FSCHG
        FMOV.S      FR6,@-R8
        FMOV.S      FR5,@-R8
        FMOV.S      FR4,@-R8
        MOV.L       R5,@-R8
        ADD         #28,R8
        FMOV.S      FR2,@-R8
        FMOV.S      FR1,@-R8
        FMOV.S      FR0,@-R8
        PREF        @R8
        ADD         #16,R8
}

/* environment mapped: u and v as NJM_SET_ENV_STRIP gives them, and the
 * intensity 1 */
/* @asm open: no inspect rule reads FV0 filled by paired loads and FLDI1 FR3
 * before its FTRV */
#pragma inline_asm(NJM_SET_ENV_X_STRIP(size=40))
void NJM_SET_ENV_X_STRIP(struct calc_vertex *vertex, Uint32 pcw)
{
        ADD         #16,R8
        FSCHG
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR2
        FLDI1       FR3
        FTRV        XMTRX,FV0
        FMOV.S      @R4+,FR4
        FMOV.S      @R4,FR6
        FSCHG
        FMOV.S      FR6,@-R8
        FMOV.S      FR5,@-R8
        FMOV.S      FR4,@-R8
        MOV.L       R5,@-R8
        ADD         #28,R8
        FLDI1       FR2
        FMOV.S      FR2,@-R8
        FMOV.S      FR1,@-R8
        FMOV.S      FR0,@-R8
        PREF        @R8
        ADD         #16,R8
}

/* save XF0 to XF9 on the stack, and load light2 into XF0 to XF2, the material's
 * diffuse color into XF4 to XF7 and iamb into XF8 */
/* @asm abi: return-below-entry, the slot of its RTS, at 0x8c62c368, leaves R15
 * 40 or, with SZ 1, 80 bytes below its entry, holding what it saves for
 * NJM_SET_POLY_STRIP_END */
#pragma inline_asm(NJM_SET_POLY_STRIP_START(size=48))
void NJM_SET_POLY_STRIP_START(void)
{
        FRCHG
        FMOV.S      FR0,@-R15
        FMOV.S      FR1,@-R15
        FMOV.S      FR2,@-R15
        FMOV.S      FR3,@-R15
        FMOV.S      FR4,@-R15
        FMOV.S      FR5,@-R15
        FMOV.S      FR6,@-R15
        FMOV.S      FR7,@-R15
        FMOV.S      FR8,@-R15
        FMOV.S      FR9,@-R15
        MOV.L       #_light2,R0
        FMOV.S      @R0+,FR0
        FMOV.S      @R0+,FR1
        FMOV.S      @R0,FR2
        MOV.L       #__nj_light_material_,R0
        ADD         #16,R0
        FMOV.S      @R0+,FR4
        FMOV.S      @R0+,FR5
        FMOV.S      @R0+,FR6
        FMOV.S      @R0,FR7
        MOV.L       #_iamb,R0
        FMOV.S      @R0,FR8
        FRCHG
}

/* save XF0 to XF3 on the stack, and load the material's diffuse color in their
 * place */
/* @asm abi: return-below-entry, the slot of its RTS, at 0x8c62c392, leaves R15
 * 16 or, with SZ 1, 32 bytes below its entry, holding what it saves for
 * NJM_SET_POLY_X_STRIP_END */
#pragma inline_asm(NJM_SET_POLY_X_STRIP_START(size=24))
void NJM_SET_POLY_X_STRIP_START(void)
{
        FRCHG
        FMOV.S      FR0,@-R15
        FMOV.S      FR1,@-R15
        FMOV.S      FR2,@-R15
        FMOV.S      FR3,@-R15
        MOV.L       #__nj_light_material_,R0
        ADD         #16,R0
        FMOV.S      @R0+,FR0
        FMOV.S      @R0+,FR1
        FMOV.S      @R0+,FR2
        FMOV.S      @R0,FR3
        FRCHG
}

/* save XF0 to XF5 on the stack, and load light2 into XF0 to XF2, iamb into XF3
 * and 1/256 into XF4 */
/* @asm abi: return-below-entry, the slot of its RTS, at 0x8c62c3ba, leaves R15
 * 24 or, with SZ 1, 48 bytes below its entry, holding what it saves for
 * NJM_SET_TEX_STRIP_END */
#pragma inline_asm(NJM_SET_TEX_STRIP_START(size=32))
void NJM_SET_TEX_STRIP_START(void)
{
        FRCHG
        FMOV.S      FR0,@-R15
        FMOV.S      FR1,@-R15
        FMOV.S      FR2,@-R15
        FMOV.S      FR3,@-R15
        FMOV.S      FR4,@-R15
        FMOV.S      FR5,@-R15
        MOV.L       #_light2,R0
        FMOV.S      @R0+,FR0
        FMOV.S      @R0+,FR1
        FMOV.S      @R0,FR2
        MOV.L       #_iamb,R0
        FMOV.S      @R0,FR3
        MOVA        #H'3B800000,R0
        FMOV.S      @R0,FR4
        FRCHG
}

/* save XF0 and XF1 on the stack, and load 1/256 into XF0 */
/* @hack spelling: size=1024, past the body's 12 bytes, which makes SHC write
 * the pending literal pool, with _njm_mesh's address, behind a BRA ahead of
 * each expansion (the BRA at 0x8c637e78 in simple_draw_unlit_tex_polygons and
 * at 0x8c6385f6 in simple_draw_unlit_tex_polygons_clip). at 12, those words
 * join later pools: 28 of 524 and 40 of 890 words differ, each function is 8
 * bytes shorter, and 1030 words of simple_draw_flat_env_polygons, after them,
 * differ */
/* @asm abi: return-below-entry, the slot of its RTS, at 0x8c62c3d6, leaves R15
 * 8 or, with SZ 1, 16 bytes below its entry, holding what it saves for
 * NJM_SET_TEX_X_STRIP_END */
#pragma inline_asm(NJM_SET_TEX_X_STRIP_START(size=1024))
void NJM_SET_TEX_X_STRIP_START(void)
{
        FRCHG
        FMOV.S      FR0,@-R15
        FMOV.S      FR1,@-R15
        MOVA        #H'3B800000,R0
        FMOV.S      @R0,FR0
        FRCHG
}

/* save all of XMTRX, the current matrix, on the stack, halve its first two rows
 * and give them the translation 1/2, and load -light2 and iamb into its
 * third */
/* @asm abi: return-below-entry, the slot of its RTS, at 0x8c62c426, leaves R15
 * 64 or, with SZ 1, 128 bytes below its entry, holding what it saves for
 * NJM_SET_ENV_STRIP_END */
#pragma inline_asm(NJM_SET_ENV_STRIP_START(size=72))
void NJM_SET_ENV_STRIP_START(void)
{
        FRCHG
        FMOV.S      FR0,@-R15
        FMOV.S      FR1,@-R15
        FMOV.S      FR2,@-R15
        FMOV.S      FR3,@-R15
        FMOV.S      FR4,@-R15
        FMOV.S      FR5,@-R15
        FMOV.S      FR6,@-R15
        FMOV.S      FR7,@-R15
        FMOV.S      FR8,@-R15
        FMOV.S      FR9,@-R15
        FMOV.S      FR10,@-R15
        FMOV.S      FR11,@-R15
        FMOV.S      FR12,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR15,@-R15
        MOVA        #H'3F000000,R0
        FMOV.S      @R0,FR12
        FMOV.S      FR12,FR13
        FMUL        FR12,FR0
        FMUL        FR12,FR4
        FMUL        FR12,FR8
        FMUL        FR12,FR1
        FMUL        FR12,FR5
        FMUL        FR12,FR9
        MOV.L       #_light2,R0
        FMOV.S      @R0+,FR2
        FMOV.S      @R0+,FR6
        FMOV.S      @R0,FR10
        FNEG        FR2
        FNEG        FR6
        FNEG        FR10
        MOV.L       #_iamb,R0
        FMOV.S      @R0,FR14
        FRCHG
}

/* save XF0 to XF13 on the stack, halve the first two rows of XMTRX, the current
 * matrix, and give them the translation 1/2 */
/* @asm abi: return-below-entry, the slot of its RTS, at 0x8c62c468, leaves R15
 * 56 or, with SZ 1, 112 bytes below its entry, holding what it saves for
 * NJM_SET_ENV_X_STRIP_END */
#pragma inline_asm(NJM_SET_ENV_X_STRIP_START(size=50))
void NJM_SET_ENV_X_STRIP_START(void)
{
        FRCHG
        FMOV.S      FR0,@-R15
        FMOV.S      FR1,@-R15
        FMOV.S      FR2,@-R15
        FMOV.S      FR3,@-R15
        FMOV.S      FR4,@-R15
        FMOV.S      FR5,@-R15
        FMOV.S      FR6,@-R15
        FMOV.S      FR7,@-R15
        FMOV.S      FR8,@-R15
        FMOV.S      FR9,@-R15
        FMOV.S      FR10,@-R15
        FMOV.S      FR11,@-R15
        FMOV.S      FR12,@-R15
        FMOV.S      FR13,@-R15
        MOVA        #H'3F000000,R0
        FMOV.S      @R0,FR12
        FMOV.S      FR12,FR13
        FMUL        FR12,FR0
        FMUL        FR12,FR4
        FMUL        FR12,FR8
        FMUL        FR12,FR1
        FMUL        FR12,FR5
        FMUL        FR12,FR9
        FRCHG
}

/* load back into the second bank what NJM_SET_POLY_STRIP_START saved */
/* @asm abi: stack-above-entry, FMOV.S @R15+,FR9 at 0x8c62c472 takes R15 above
 * its entry; its RTS at 0x8c62c488 returns 40 or, with SZ 1, 80 bytes higher,
 * past what NJM_SET_POLY_STRIP_START saved */
#pragma inline_asm(NJM_SET_POLY_STRIP_END(size=24))
void NJM_SET_POLY_STRIP_END(void)
{
        FRCHG
        FMOV.S      @R15+,FR9
        FMOV.S      @R15+,FR8
        FMOV.S      @R15+,FR7
        FMOV.S      @R15+,FR6
        FMOV.S      @R15+,FR5
        FMOV.S      @R15+,FR4
        FMOV.S      @R15+,FR3
        FMOV.S      @R15+,FR2
        FMOV.S      @R15+,FR1
        FMOV.S      @R15+,FR0
        FRCHG
}

/* load back into the second bank what NJM_SET_POLY_X_STRIP_START saved */
/* @asm abi: stack-above-entry, FMOV.S @R15+,FR3 at 0x8c62c48e takes R15 above
 * its entry; its RTS at 0x8c62c498 returns 16 or, with SZ 1, 32 bytes higher,
 * past what NJM_SET_POLY_X_STRIP_START saved */
#pragma inline_asm(NJM_SET_POLY_X_STRIP_END(size=12))
void NJM_SET_POLY_X_STRIP_END(void)
{
        FRCHG
        FMOV.S      @R15+,FR3
        FMOV.S      @R15+,FR2
        FMOV.S      @R15+,FR1
        FMOV.S      @R15+,FR0
        FRCHG
}

/* load back into the second bank what NJM_SET_TEX_STRIP_START saved */
/* @asm abi: stack-above-entry, FMOV.S @R15+,FR5 at 0x8c62c49e takes R15 above
 * its entry; its RTS at 0x8c62c4ac returns 24 or, with SZ 1, 48 bytes higher,
 * past what NJM_SET_TEX_STRIP_START saved */
#pragma inline_asm(NJM_SET_TEX_STRIP_END(size=16))
void NJM_SET_TEX_STRIP_END(void)
{
        FRCHG
        FMOV.S      @R15+,FR5
        FMOV.S      @R15+,FR4
        FMOV.S      @R15+,FR3
        FMOV.S      @R15+,FR2
        FMOV.S      @R15+,FR1
        FMOV.S      @R15+,FR0
        FRCHG
}

/* load back into the second bank what NJM_SET_TEX_X_STRIP_START saved */
/* @asm abi: stack-above-entry, FMOV.S @R15+,FR1 at 0x8c62c4b2 takes R15 above
 * its entry; its RTS at 0x8c62c4b8 returns 8 or, with SZ 1, 16 bytes higher,
 * past what NJM_SET_TEX_X_STRIP_START saved */
#pragma inline_asm(NJM_SET_TEX_X_STRIP_END(size=8))
void NJM_SET_TEX_X_STRIP_END(void)
{
        FRCHG
        FMOV.S      @R15+,FR1
        FMOV.S      @R15+,FR0
        FRCHG
}

/* load back into the second bank what NJM_SET_ENV_STRIP_START saved */
/* @asm abi: stack-above-entry, FMOV.S @R15+,FR15 at 0x8c62c4be takes R15 above
 * its entry; its RTS at 0x8c62c4e0 returns 64 or, with SZ 1, 128 bytes higher,
 * past what NJM_SET_ENV_STRIP_START saved */
#pragma inline_asm(NJM_SET_ENV_STRIP_END(size=36))
void NJM_SET_ENV_STRIP_END(void)
{
        FRCHG
        FMOV.S      @R15+,FR15
        FMOV.S      @R15+,FR14
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR11
        FMOV.S      @R15+,FR10
        FMOV.S      @R15+,FR9
        FMOV.S      @R15+,FR8
        FMOV.S      @R15+,FR7
        FMOV.S      @R15+,FR6
        FMOV.S      @R15+,FR5
        FMOV.S      @R15+,FR4
        FMOV.S      @R15+,FR3
        FMOV.S      @R15+,FR2
        FMOV.S      @R15+,FR1
        FMOV.S      @R15+,FR0
        FRCHG
}

/* load back into the second bank what NJM_SET_ENV_X_STRIP_START saved */
/* @asm abi: stack-above-entry, FMOV.S @R15+,FR13 at 0x8c62c4e6 takes R15 above
 * its entry; its RTS at 0x8c62c504 returns 56 or, with SZ 1, 112 bytes higher,
 * past what NJM_SET_ENV_X_STRIP_START saved */
#pragma inline_asm(NJM_SET_ENV_X_STRIP_END(size=32))
void NJM_SET_ENV_X_STRIP_END(void)
{
        FRCHG
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR11
        FMOV.S      @R15+,FR10
        FMOV.S      @R15+,FR9
        FMOV.S      @R15+,FR8
        FMOV.S      @R15+,FR7
        FMOV.S      @R15+,FR6
        FMOV.S      @R15+,FR5
        FMOV.S      @R15+,FR4
        FMOV.S      @R15+,FR3
        FMOV.S      @R15+,FR2
        FMOV.S      @R15+,FR1
        FMOV.S      @R15+,FR0
        FRCHG
}
#endif
/* clang-format on */

/* @hack spelling: simple_draw_noop called through a pointer to it, which keeps
 * the image's JSR to the empty function at the start of each strip drawer
 * (0x8c62e214 in simple_draw_smooth_strips_cull), and keeps the function out of
 * line. called by name, SHC expands the empty body in place and writes no
 * function, and 34 of simple_draw_smooth_strips_cull's 300 words differ */
#define DRAW_NOOP() (*(void (*)(void))simple_draw_noop)()

/* the emitters write a vertex at sq_cursor, from the last word they write down
 * to command, its PCW. then they send the store queue to memory with a prefetch
 * of its address, and step sq_cursor to the next vertex. texcoord is an NJS_TEX
 * pointer, or in a _NEXT emitter an NJS_COLOR pointer, which the emitter steps
 * past the pair it reads */
#define EMIT_TEX_INTENSITY(vertex, texcoord, command)                          \
    {                                                                          \
        sq_cursor += 7;                                                        \
        *--sq_cursor = (vertex)->intensity;                                    \
        *--sq_cursor = (Float)(texcoord)->v * TEX_SCALE;                       \
        *--sq_cursor = (Float)(texcoord)->u * TEX_SCALE;                       \
        *--sq_cursor = (vertex)->oow;                                          \
        *--sq_cursor = (vertex)->sy;                                           \
        *--sq_cursor = (vertex)->sx;                                           \
        *(Uint32 *)--sq_cursor = command;                                      \
        prefetch(sq_cursor);                                                   \
        sq_cursor += VERTEX_WORDS;                                             \
    }

#define EMIT_UNLIT_TEX(vertex, texcoord, command)                              \
    {                                                                          \
        sq_cursor += 7;                                                        \
        *--sq_cursor = 1.0F;                                                   \
        *--sq_cursor = (Float)(texcoord)->v * TEX_SCALE;                       \
        *--sq_cursor = (Float)(texcoord)->u * TEX_SCALE;                       \
        *--sq_cursor = (vertex)->oow;                                          \
        *--sq_cursor = (vertex)->sy;                                           \
        *--sq_cursor = (vertex)->sx;                                           \
        *(Uint32 *)--sq_cursor = command;                                      \
        prefetch(sq_cursor);                                                   \
        sq_cursor += VERTEX_WORDS;                                             \
    }

#define EMIT_UNLIT_TEX_NEXT(vertex, texcoord, command)                         \
    {                                                                          \
        sq_cursor += 7;                                                        \
        *--sq_cursor = 1.0F;                                                   \
        *--sq_cursor = (Float)(texcoord)->tex.v * TEX_SCALE;                   \
        *--sq_cursor = (Float)(texcoord)->tex.u * TEX_SCALE;                   \
        (texcoord)++;                                                          \
        *--sq_cursor = (vertex)->oow;                                          \
        *--sq_cursor = (vertex)->sy;                                           \
        *--sq_cursor = (vertex)->sx;                                           \
        *(Uint32 *)--sq_cursor = command;                                      \
        prefetch(sq_cursor);                                                   \
        sq_cursor += VERTEX_WORDS;                                             \
    }

#define EMIT_COLOR_INTENSITY(vertex, command)                                  \
    {                                                                          \
        sq_cursor += VERTEX_WORDS;                                             \
        *--sq_cursor = _nj_light_material_.diffuse.b * (vertex)->intensity;    \
        *--sq_cursor = _nj_light_material_.diffuse.g * (vertex)->intensity;    \
        *--sq_cursor = _nj_light_material_.diffuse.r * (vertex)->intensity;    \
        *--sq_cursor = _nj_light_material_.diffuse.a;                          \
        *--sq_cursor = (vertex)->oow;                                          \
        *--sq_cursor = (vertex)->sy;                                           \
        *--sq_cursor = (vertex)->sx;                                           \
        *(Uint32 *)--sq_cursor = command;                                      \
        prefetch(sq_cursor);                                                   \
        sq_cursor += VERTEX_WORDS;                                             \
    }

#define EMIT_UNLIT_COLOR(vertex, command)                                      \
    {                                                                          \
        sq_cursor += VERTEX_WORDS;                                             \
        *--sq_cursor = _nj_light_material_.diffuse.b;                          \
        *--sq_cursor = _nj_light_material_.diffuse.g;                          \
        *--sq_cursor = _nj_light_material_.diffuse.r;                          \
        *--sq_cursor = _nj_light_material_.diffuse.a;                          \
        *--sq_cursor = (vertex)->oow;                                          \
        *--sq_cursor = (vertex)->sy;                                           \
        *--sq_cursor = (vertex)->sx;                                           \
        *(Uint32 *)--sq_cursor = command;                                      \
        prefetch(sq_cursor);                                                   \
        sq_cursor += VERTEX_WORDS;                                             \
    }

/* set point to vertex's view position. a vertex with CLIP_NEAR holds it in sx,
 * sy and oow. for any other, undo the projection, and set depth to the vertex's
 * z */
#define DECODE_VIEW_POINT(vertex, point, depth)                                \
    {                                                                          \
        if (!((vertex)->clip & CLIP_NEAR)) {                                   \
            (depth) = 1.0F / (vertex)->oow;                                    \
            (point).x =                                                        \
                ((vertex)->sx - _nj_screen_.cx) * (depth) / projection_x;      \
            (point).y =                                                        \
                ((vertex)->sy - _nj_screen_.cy) * (depth) / projection_y;      \
            (point).z = (depth);                                               \
        } else {                                                               \
            (point).x = (vertex)->sx;                                          \
            (point).y = (vertex)->sy;                                          \
            (point).z = (vertex)->oow;                                         \
        }                                                                      \
    }

/* the light's intensity at normal, a model-space normal: iamb minus the dot of
 * light2 and normal, unclamped */
#define SIMPLE_INTENSITY(normal)                                               \
    (iamb - (light2.x * (normal).x) - (light2.y * (normal).y) -                \
     (light2.z * (normal).z))

/* the x (column 0) or y (column 1) of normal turned by *mp, the current matrix
 * in memory */
#define ENV_COLUMN_DOT(normal, column)                                         \
    ((((Float *)mp)[column] * (normal).x) +                                    \
     (((Float *)mp)[(column) + 4] * (normal).y) +                              \
     (((Float *)mp)[(column) + 8] * (normal).z))

/* the clippers send the part of triangle v0, v1, v2 beyond the near plane, a
 * triangle or a quad, and add its corners to _nj_draw_polygon_. at least one
 * corner must lie beyond the plane. a clipper that finds clip_cull_requested
 * set drops a triangle that faces away, and leaves the flag set. a clipper
 * that sends its triangle clears the flag. a textured clipper's corner_uv
 * holds v0's, v1's and v2's texture coordinates. the smooth clippers store
 * each corner's SIMPLE_INTENSITY in its intensity. the environment-mapped
 * ones set each corner's u to 128 times ENV_COLUMN_DOT(normal, 0) plus 128,
 * and its v the same from column 1, each truncated to whole units of 1/256 */
static void simple_clip_smooth_tex_triangle(struct calc_vertex *v0,
                                            struct calc_vertex *v1,
                                            struct calc_vertex *v2,
                                            NJS_COLOR *corner_uv)
{
    struct calc_vertex *vertices[3];
    NJS_POINT3 view_points[3];
    Float view_x;
    Float view_y;
    Float view_z;
    Float near_reciprocal;
    Float edge_t;
    Float view_facing;
    int corner;
    int previous;
    int output_count;

    near_reciprocal = 1.0F / _nj_near_clip_;
    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    for (corner = 0; corner < 3; corner++) {
        vertices[corner]->intensity =
            SIMPLE_INTENSITY(vertices[corner]->normal);
        DECODE_VIEW_POINT(vertices[corner], view_points[corner], view_z);
    }
    view_facing = (view_points[2].x * ((view_points[0].z * view_points[1].y) -
                                       (view_points[0].y * view_points[1].z))) +
                  (view_points[2].y * ((view_points[0].x * view_points[1].z) -
                                       (view_points[0].z * view_points[1].x))) +
                  (view_points[2].z * ((view_points[1].x * view_points[0].y) -
                                       (view_points[0].x * view_points[1].y)));

    if ((view_facing < 0.0F) & clip_cull_requested) {
        return;
    }
    clip_cull_requested = 0;
    output_count = 0;
    previous = 2;
    for (corner = 0; corner < 3; corner++) {
        if (((vertices[previous]->clip & CLIP_NEAR) ^
             (vertices[corner]->clip & CLIP_NEAR)) == 1) {
            edge_t = (_nj_near_clip_ - view_points[previous].z) /
                     (view_points[corner].z - view_points[previous].z);
            view_x =
                view_points[previous].x +
                ((view_points[corner].x - view_points[previous].x) * edge_t);
            clipped_vertices[output_count].sx =
                (projection_x * view_x * near_reciprocal) + _nj_screen_.cx;
            view_y =
                view_points[previous].y +
                ((view_points[corner].y - view_points[previous].y) * edge_t);
            clipped_vertices[output_count].sy =
                (projection_y * view_y * near_reciprocal) + _nj_screen_.cy;
            clipped_vertices[output_count].oow = near_reciprocal;
            clipped_vertices[output_count].intensity =
                vertices[previous]->intensity +
                ((vertices[corner]->intensity - vertices[previous]->intensity) *
                 edge_t);
            clipped_uv[output_count].u =
                corner_uv[previous].tex.u +
                ((corner_uv[corner].tex.u - corner_uv[previous].tex.u) *
                 edge_t);
            clipped_uv[output_count].v =
                corner_uv[previous].tex.v +
                ((corner_uv[corner].tex.v - corner_uv[previous].tex.v) *
                 edge_t);
            output_count++;
        }
        if (!(vertices[corner]->clip & CLIP_NEAR)) {
            clipped_vertices[output_count].sx = vertices[corner]->sx;
            clipped_vertices[output_count].sy = vertices[corner]->sy;
            clipped_vertices[output_count].oow = vertices[corner]->oow;
            clipped_vertices[output_count].intensity =
                vertices[corner]->intensity;
            clipped_uv[output_count].u = corner_uv[corner].tex.u;
            clipped_uv[output_count].v = corner_uv[corner].tex.v;
            output_count++;
        }
        previous = corner;
    }

    if (output_count == 3) {
        EMIT_TEX_INTENSITY(&clipped_vertices[0], &clipped_uv[0],
                           KM_VERTEXPARAM_NORMAL);
        EMIT_TEX_INTENSITY(&clipped_vertices[1], &clipped_uv[1],
                           KM_VERTEXPARAM_NORMAL);
        EMIT_TEX_INTENSITY(&clipped_vertices[2], &clipped_uv[2],
                           KM_VERTEXPARAM_ENDOFSTRIP);
        _nj_draw_polygon_ += 3;
    } else {
        EMIT_TEX_INTENSITY(&clipped_vertices[0], &clipped_uv[0],
                           KM_VERTEXPARAM_NORMAL);
        EMIT_TEX_INTENSITY(&clipped_vertices[1], &clipped_uv[1],
                           KM_VERTEXPARAM_NORMAL);
        EMIT_TEX_INTENSITY(&clipped_vertices[3], &clipped_uv[3],
                           KM_VERTEXPARAM_NORMAL);
        EMIT_TEX_INTENSITY(&clipped_vertices[2], &clipped_uv[2],
                           KM_VERTEXPARAM_ENDOFSTRIP);
        _nj_draw_polygon_ += 4;
    }
}

static void simple_clip_smooth_env_triangle(struct calc_vertex *v0,
                                            struct calc_vertex *v1,
                                            struct calc_vertex *v2)
{
    struct calc_vertex *vertices[3];
    NJS_POINT3 view_points[3];
    NJS_TEX quantized_uv[3];
    struct calc_vertex *vertex;
    Float view_x;
    Float view_y;
    Float view_z;
    Float near_reciprocal;
    Float edge_t;
    Float view_facing;
    int corner;
    int previous;
    int output_count;

    near_reciprocal = 1.0F / _nj_near_clip_;
    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    for (corner = 0; corner < 3; corner++) {
        vertices[corner]->intensity =
            SIMPLE_INTENSITY(vertices[corner]->normal);

        vertex = vertices[corner];
        quantized_uv[corner].u =
            (ENV_COLUMN_DOT(vertex->normal, 0) * 128.0F) + 128.0F;
        quantized_uv[corner].v =
            (ENV_COLUMN_DOT(vertex->normal, 1) * 128.0F) + 128.0F;
        DECODE_VIEW_POINT(vertices[corner], view_points[corner], view_z);
    }
    view_facing = (view_points[2].x * ((view_points[0].z * view_points[1].y) -
                                       (view_points[0].y * view_points[1].z))) +
                  (view_points[2].y * ((view_points[0].x * view_points[1].z) -
                                       (view_points[0].z * view_points[1].x))) +
                  (view_points[2].z * ((view_points[1].x * view_points[0].y) -
                                       (view_points[0].x * view_points[1].y)));
    if ((view_facing < 0.0F) & clip_cull_requested) {
        return;
    }
    clip_cull_requested = 0;
    output_count = 0;
    previous = 2;
    for (corner = 0; corner < 3; corner++) {
        if (((vertices[previous]->clip & CLIP_NEAR) ^
             (vertices[corner]->clip & CLIP_NEAR)) == 1) {
            edge_t = (_nj_near_clip_ - view_points[previous].z) /
                     (view_points[corner].z - view_points[previous].z);
            view_x =
                view_points[previous].x +
                ((view_points[corner].x - view_points[previous].x) * edge_t);
            clipped_vertices[output_count].sx =
                (projection_x * view_x * near_reciprocal) + _nj_screen_.cx;
            view_y =
                view_points[previous].y +
                ((view_points[corner].y - view_points[previous].y) * edge_t);
            clipped_vertices[output_count].sy =
                (projection_y * view_y * near_reciprocal) + _nj_screen_.cy;
            clipped_vertices[output_count].oow = near_reciprocal;
            clipped_vertices[output_count].intensity =
                vertices[previous]->intensity +
                ((vertices[corner]->intensity - vertices[previous]->intensity) *
                 edge_t);
            clipped_uv[output_count].u =
                quantized_uv[previous].u +
                ((quantized_uv[corner].u - quantized_uv[previous].u) * edge_t);
            clipped_uv[output_count].v =
                quantized_uv[previous].v +
                ((quantized_uv[corner].v - quantized_uv[previous].v) * edge_t);
            output_count++;
        }
        if (!(vertices[corner]->clip & CLIP_NEAR)) {
            clipped_vertices[output_count].sx = vertices[corner]->sx;
            clipped_vertices[output_count].sy = vertices[corner]->sy;
            clipped_vertices[output_count].oow = vertices[corner]->oow;
            clipped_vertices[output_count].intensity =
                vertices[corner]->intensity;
            clipped_uv[output_count].u = quantized_uv[corner].u;
            clipped_uv[output_count].v = quantized_uv[corner].v;
            output_count++;
        }
        previous = corner;
    }
    if (output_count == 3) {
        EMIT_TEX_INTENSITY(&clipped_vertices[0], &clipped_uv[0],
                           KM_VERTEXPARAM_NORMAL);
        EMIT_TEX_INTENSITY(&clipped_vertices[1], &clipped_uv[1],
                           KM_VERTEXPARAM_NORMAL);
        EMIT_TEX_INTENSITY(&clipped_vertices[2], &clipped_uv[2],
                           KM_VERTEXPARAM_ENDOFSTRIP);
        _nj_draw_polygon_ += 3;
    } else {
        EMIT_TEX_INTENSITY(&clipped_vertices[0], &clipped_uv[0],
                           KM_VERTEXPARAM_NORMAL);
        EMIT_TEX_INTENSITY(&clipped_vertices[1], &clipped_uv[1],
                           KM_VERTEXPARAM_NORMAL);
        EMIT_TEX_INTENSITY(&clipped_vertices[3], &clipped_uv[3],
                           KM_VERTEXPARAM_NORMAL);
        EMIT_TEX_INTENSITY(&clipped_vertices[2], &clipped_uv[2],
                           KM_VERTEXPARAM_ENDOFSTRIP);
        _nj_draw_polygon_ += 4;
    }
}

static void simple_clip_unlit_tex_triangle(struct calc_vertex *v0,
                                           struct calc_vertex *v1,
                                           struct calc_vertex *v2,
                                           NJS_COLOR *corner_uv)
{
    struct calc_vertex *vertices[3];
    NJS_POINT3 view_points[3];
    Float view_x;
    Float view_y;
    Float view_z;
    Float near_reciprocal;
    Float edge_t;
    Float view_facing;
    int corner;
    int previous;
    int output_count;

    near_reciprocal = 1.0F / _nj_near_clip_;
    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    for (corner = 0; corner < 3; corner++) {
        DECODE_VIEW_POINT(vertices[corner], view_points[corner], view_z);
    }
    view_facing = (view_points[2].x * ((view_points[0].z * view_points[1].y) -
                                       (view_points[0].y * view_points[1].z))) +
                  (view_points[2].y * ((view_points[0].x * view_points[1].z) -
                                       (view_points[0].z * view_points[1].x))) +
                  (view_points[2].z * ((view_points[1].x * view_points[0].y) -
                                       (view_points[0].x * view_points[1].y)));
    if ((view_facing < 0.0F) & clip_cull_requested) {
        return;
    }
    clip_cull_requested = 0;
    output_count = 0;
    previous = 2;
    for (corner = 0; corner < 3; corner++) {
        if (((vertices[previous]->clip & CLIP_NEAR) ^
             (vertices[corner]->clip & CLIP_NEAR)) == 1) {
            edge_t = (_nj_near_clip_ - view_points[previous].z) /
                     (view_points[corner].z - view_points[previous].z);
            view_x =
                view_points[previous].x +
                ((view_points[corner].x - view_points[previous].x) * edge_t);
            clipped_vertices[output_count].sx =
                (projection_x * view_x * near_reciprocal) + _nj_screen_.cx;
            view_y =
                view_points[previous].y +
                ((view_points[corner].y - view_points[previous].y) * edge_t);
            clipped_vertices[output_count].sy =
                (projection_y * view_y * near_reciprocal) + _nj_screen_.cy;
            clipped_vertices[output_count].oow = near_reciprocal;
            clipped_uv[output_count].u =
                corner_uv[previous].tex.u +
                ((corner_uv[corner].tex.u - corner_uv[previous].tex.u) *
                 edge_t);
            clipped_uv[output_count].v =
                corner_uv[previous].tex.v +
                ((corner_uv[corner].tex.v - corner_uv[previous].tex.v) *
                 edge_t);
            output_count++;
        }
        if (!(vertices[corner]->clip & CLIP_NEAR)) {
            clipped_vertices[output_count].sx = vertices[corner]->sx;
            clipped_vertices[output_count].sy = vertices[corner]->sy;
            clipped_vertices[output_count].oow = vertices[corner]->oow;
            clipped_uv[output_count].u = corner_uv[corner].tex.u;
            clipped_uv[output_count].v = corner_uv[corner].tex.v;
            output_count++;
        }
        previous = corner;
    }
    if (output_count == 3) {
        EMIT_UNLIT_TEX(&clipped_vertices[0], &clipped_uv[0],
                       KM_VERTEXPARAM_NORMAL);
        EMIT_UNLIT_TEX(&clipped_vertices[1], &clipped_uv[1],
                       KM_VERTEXPARAM_NORMAL);
        EMIT_UNLIT_TEX(&clipped_vertices[2], &clipped_uv[2],
                       KM_VERTEXPARAM_ENDOFSTRIP);
        _nj_draw_polygon_ += 3;
    } else {
        EMIT_UNLIT_TEX(&clipped_vertices[0], &clipped_uv[0],
                       KM_VERTEXPARAM_NORMAL);
        EMIT_UNLIT_TEX(&clipped_vertices[1], &clipped_uv[1],
                       KM_VERTEXPARAM_NORMAL);
        EMIT_UNLIT_TEX(&clipped_vertices[3], &clipped_uv[3],
                       KM_VERTEXPARAM_NORMAL);
        EMIT_UNLIT_TEX(&clipped_vertices[2], &clipped_uv[2],
                       KM_VERTEXPARAM_ENDOFSTRIP);
        _nj_draw_polygon_ += 4;
    }
}

static void simple_clip_unlit_env_triangle(struct calc_vertex *v0,
                                           struct calc_vertex *v1,
                                           struct calc_vertex *v2)
{
    struct calc_vertex *vertices[3];
    NJS_POINT3 view_points[3];
    NJS_TEX quantized_uv[3];
    struct calc_vertex *vertex;
    Float view_x;
    Float view_y;
    Float view_z;
    Float near_reciprocal;
    Float edge_t;
    Float view_facing;
    int corner;
    int previous;
    int output_count;

    near_reciprocal = 1.0F / _nj_near_clip_;
    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    for (corner = 0; corner < 3; corner++) {
        vertex = vertices[corner];
        quantized_uv[corner].u =
            (Sint16)((ENV_COLUMN_DOT(vertex->normal, 0) * 128.0F) + 128.0F);
        quantized_uv[corner].v =
            (Sint16)((ENV_COLUMN_DOT(vertex->normal, 1) * 128.0F) + 128.0F);
        DECODE_VIEW_POINT(vertices[corner], view_points[corner], view_z);
    }
    view_facing = (view_points[2].x * ((view_points[0].z * view_points[1].y) -
                                       (view_points[0].y * view_points[1].z))) +
                  (view_points[2].y * ((view_points[0].x * view_points[1].z) -
                                       (view_points[0].z * view_points[1].x))) +
                  (view_points[2].z * ((view_points[1].x * view_points[0].y) -
                                       (view_points[0].x * view_points[1].y)));
    if ((view_facing < 0.0F) & clip_cull_requested) {
        return;
    }
    clip_cull_requested = 0;
    output_count = 0;
    previous = 2;
    for (corner = 0; corner < 3; corner++) {
        if (((vertices[previous]->clip & CLIP_NEAR) ^
             (vertices[corner]->clip & CLIP_NEAR)) == 1) {
            edge_t = (_nj_near_clip_ - view_points[previous].z) /
                     (view_points[corner].z - view_points[previous].z);
            view_x =
                view_points[previous].x +
                ((view_points[corner].x - view_points[previous].x) * edge_t);
            clipped_vertices[output_count].sx =
                (projection_x * view_x * near_reciprocal) + _nj_screen_.cx;
            view_y =
                view_points[previous].y +
                ((view_points[corner].y - view_points[previous].y) * edge_t);
            clipped_vertices[output_count].sy =
                (projection_y * view_y * near_reciprocal) + _nj_screen_.cy;
            clipped_vertices[output_count].oow = near_reciprocal;
            clipped_uv[output_count].u =
                quantized_uv[previous].u +
                ((quantized_uv[corner].u - quantized_uv[previous].u) * edge_t);
            clipped_uv[output_count].v =
                quantized_uv[previous].v +
                ((quantized_uv[corner].v - quantized_uv[previous].v) * edge_t);
            output_count++;
        }
        if (!(vertices[corner]->clip & CLIP_NEAR)) {
            clipped_vertices[output_count].sx = vertices[corner]->sx;
            clipped_vertices[output_count].sy = vertices[corner]->sy;
            clipped_vertices[output_count].oow = vertices[corner]->oow;
            clipped_uv[output_count].u = quantized_uv[corner].u;
            clipped_uv[output_count].v = quantized_uv[corner].v;
            output_count++;
        }
        previous = corner;
    }
    if (output_count == 3) {
        EMIT_UNLIT_TEX(&clipped_vertices[0], &clipped_uv[0],
                       KM_VERTEXPARAM_NORMAL);
        EMIT_UNLIT_TEX(&clipped_vertices[1], &clipped_uv[1],
                       KM_VERTEXPARAM_NORMAL);
        EMIT_UNLIT_TEX(&clipped_vertices[2], &clipped_uv[2],
                       KM_VERTEXPARAM_ENDOFSTRIP);
        _nj_draw_polygon_ += 3;
    } else {
        EMIT_UNLIT_TEX(&clipped_vertices[0], &clipped_uv[0],
                       KM_VERTEXPARAM_NORMAL);
        EMIT_UNLIT_TEX(&clipped_vertices[1], &clipped_uv[1],
                       KM_VERTEXPARAM_NORMAL);
        EMIT_UNLIT_TEX(&clipped_vertices[3], &clipped_uv[3],
                       KM_VERTEXPARAM_NORMAL);
        EMIT_UNLIT_TEX(&clipped_vertices[2], &clipped_uv[2],
                       KM_VERTEXPARAM_ENDOFSTRIP);
        _nj_draw_polygon_ += 4;
    }
}

static void simple_clip_smooth_triangle(struct calc_vertex *v0,
                                        struct calc_vertex *v1,
                                        struct calc_vertex *v2)
{
    struct calc_vertex *vertices[3];
    NJS_POINT3 view_points[3];
    Float view_x;
    Float view_y;
    Float view_z;
    Float near_reciprocal;
    Float edge_t;
    Float view_facing;
    int corner;
    int previous;
    int output_count;

    near_reciprocal = 1.0F / _nj_near_clip_;
    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    for (corner = 0; corner < 3; corner++) {
        vertices[corner]->intensity =
            SIMPLE_INTENSITY(vertices[corner]->normal);
        DECODE_VIEW_POINT(vertices[corner], view_points[corner], view_z);
    }
    view_facing = (view_points[2].x * ((view_points[0].z * view_points[1].y) -
                                       (view_points[0].y * view_points[1].z))) +
                  (view_points[2].y * ((view_points[0].x * view_points[1].z) -
                                       (view_points[0].z * view_points[1].x))) +
                  (view_points[2].z * ((view_points[1].x * view_points[0].y) -
                                       (view_points[0].x * view_points[1].y)));
    if ((view_facing < 0.0F) & clip_cull_requested) {
        return;
    }
    clip_cull_requested = 0;
    output_count = 0;
    previous = 2;
    for (corner = 0; corner < 3; corner++) {
        if (((vertices[previous]->clip & CLIP_NEAR) ^
             (vertices[corner]->clip & CLIP_NEAR)) == 1) {
            edge_t = (_nj_near_clip_ - view_points[previous].z) /
                     (view_points[corner].z - view_points[previous].z);
            view_x =
                view_points[previous].x +
                ((view_points[corner].x - view_points[previous].x) * edge_t);
            clipped_vertices[output_count].sx =
                (projection_x * view_x * near_reciprocal) + _nj_screen_.cx;
            view_y =
                view_points[previous].y +
                ((view_points[corner].y - view_points[previous].y) * edge_t);
            clipped_vertices[output_count].sy =
                (projection_y * view_y * near_reciprocal) + _nj_screen_.cy;
            clipped_vertices[output_count].oow = near_reciprocal;
            clipped_vertices[output_count].intensity =
                vertices[previous]->intensity +
                ((vertices[corner]->intensity - vertices[previous]->intensity) *
                 edge_t);
            output_count++;
        }
        if (!(vertices[corner]->clip & CLIP_NEAR)) {
            clipped_vertices[output_count].sx = vertices[corner]->sx;
            clipped_vertices[output_count].sy = vertices[corner]->sy;
            clipped_vertices[output_count].oow = vertices[corner]->oow;
            clipped_vertices[output_count].intensity =
                vertices[corner]->intensity;
            output_count++;
        }
        previous = corner;
    }
    if (output_count == 3) {
        EMIT_COLOR_INTENSITY(&clipped_vertices[0], KM_VERTEXPARAM_NORMAL);
        EMIT_COLOR_INTENSITY(&clipped_vertices[1], KM_VERTEXPARAM_NORMAL);
        EMIT_COLOR_INTENSITY(&clipped_vertices[2], KM_VERTEXPARAM_ENDOFSTRIP);
        _nj_draw_polygon_ += 3;
    } else {
        EMIT_COLOR_INTENSITY(&clipped_vertices[0], KM_VERTEXPARAM_NORMAL);
        EMIT_COLOR_INTENSITY(&clipped_vertices[1], KM_VERTEXPARAM_NORMAL);
        EMIT_COLOR_INTENSITY(&clipped_vertices[3], KM_VERTEXPARAM_NORMAL);
        EMIT_COLOR_INTENSITY(&clipped_vertices[2], KM_VERTEXPARAM_ENDOFSTRIP);
        _nj_draw_polygon_ += 4;
    }
}

static void simple_clip_unlit_triangle(struct calc_vertex *v0,
                                       struct calc_vertex *v1,
                                       struct calc_vertex *v2)
{
    struct calc_vertex *vertices[3];
    NJS_POINT3 view_points[3];
    Float view_x;
    Float view_y;
    Float view_z;
    Float near_reciprocal;
    Float edge_t;
    Float view_facing;
    int corner;
    int previous;
    int output_count;

    near_reciprocal = 1.0F / _nj_near_clip_;
    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    for (corner = 0; corner < 3; corner++) {
        DECODE_VIEW_POINT(vertices[corner], view_points[corner], view_z);
    }
    view_facing = (view_points[2].x * ((view_points[0].z * view_points[1].y) -
                                       (view_points[0].y * view_points[1].z))) +
                  (view_points[2].y * ((view_points[0].x * view_points[1].z) -
                                       (view_points[0].z * view_points[1].x))) +
                  (view_points[2].z * ((view_points[1].x * view_points[0].y) -
                                       (view_points[0].x * view_points[1].y)));
    if ((view_facing < 0.0F) & clip_cull_requested) {
        return;
    }
    clip_cull_requested = 0;
    output_count = 0;
    previous = 2;
    for (corner = 0; corner < 3; corner++) {
        if (((vertices[previous]->clip & CLIP_NEAR) ^
             (vertices[corner]->clip & CLIP_NEAR)) == 1) {
            edge_t = (_nj_near_clip_ - view_points[previous].z) /
                     (view_points[corner].z - view_points[previous].z);
            view_x =
                view_points[previous].x +
                ((view_points[corner].x - view_points[previous].x) * edge_t);
            clipped_vertices[output_count].sx =
                (projection_x * view_x * near_reciprocal) + _nj_screen_.cx;
            view_y =
                view_points[previous].y +
                ((view_points[corner].y - view_points[previous].y) * edge_t);
            clipped_vertices[output_count].sy =
                (projection_y * view_y * near_reciprocal) + _nj_screen_.cy;
            clipped_vertices[output_count].oow = near_reciprocal;
            output_count++;
        }
        if (!(vertices[corner]->clip & CLIP_NEAR)) {
            clipped_vertices[output_count].sx = vertices[corner]->sx;
            clipped_vertices[output_count].sy = vertices[corner]->sy;
            clipped_vertices[output_count].oow = vertices[corner]->oow;
            output_count++;
        }
        previous = corner;
    }
    if (output_count == 3) {
        EMIT_UNLIT_COLOR(&clipped_vertices[0], KM_VERTEXPARAM_NORMAL);
        EMIT_UNLIT_COLOR(&clipped_vertices[1], KM_VERTEXPARAM_NORMAL);
        EMIT_UNLIT_COLOR(&clipped_vertices[2], KM_VERTEXPARAM_ENDOFSTRIP);
        _nj_draw_polygon_ += 3;
    } else {
        EMIT_UNLIT_COLOR(&clipped_vertices[0], KM_VERTEXPARAM_NORMAL);
        EMIT_UNLIT_COLOR(&clipped_vertices[1], KM_VERTEXPARAM_NORMAL);
        EMIT_UNLIT_COLOR(&clipped_vertices[3], KM_VERTEXPARAM_NORMAL);
        EMIT_UNLIT_COLOR(&clipped_vertices[2], KM_VERTEXPARAM_ENDOFSTRIP);
        _nj_draw_polygon_ += 4;
    }
}

/* the strip drawers send meshset's strips through the strip helpers of their
 * kind, whose START and END the caller runs around them. a strip whose count
 * has NJD_TRIMESH_END set starts with the other winding. each drawer adds
 * every point of its strips to _nj_calc_polygon_. a textured drawer's
 * meshset->vertuv holds a pair for each point of each strip.
 *
 * a _cull drawer drops each triangle that faces away, which ends the run of
 * triangles before it. it adds each vertex it sends, and one more for each
 * run, to _nj_draw_polygon_.
 *
 * a _near drawer also drops each triangle whose corners all share a clip flag.
 * it sends a triangle that crosses the near plane through a clipper, unless
 * the triangle faces away. it adds every point of its strips to
 * _nj_draw_polygon_, whatever it sends. the clippers add the corners they send
 * as well */
static void simple_draw_smooth_strips_cull(NJS_MESHSET *meshset)
{
    register Sint16 *mesh_cursor;
    Sint32 remaining;
    register struct calc_vertex *vertices;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register Uint8 *run_start;
    Uint32 accounted_bytes;
    Uint32 input_corners;
    Sint32 strips_left;
    register Float winding;
    register Float x1;
    register Float x0;
    Float y0;
    Float y1;
    Float facing;

    DRAW_NOOP();
    mesh_cursor = meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct calc_vertex *)_nj_vertex_buf_;
    input_corners = accounted_bytes = 0;
/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c62e22e, the pool at 0x8c62e234). without it, SHC writes
 * simple_clip_unlit_triangle's literals behind that function's RTS, making it
 * 38 bytes longer, and this function's in a later pool: 285 of 300 words differ
 * and the function is 38 bytes shorter */
#pragma asm
    ;
#pragma endasm
    do {
        remaining = (Uint16)*mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }

        input_corners += remaining;
        remaining -= 2;
        v0 = &vertices[(Uint16)*mesh_cursor++];
        x0 = v0->sx;
        v1 = &vertices[(Uint16)*mesh_cursor++];
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = (Uint8 *)sq_cursor;
        do {
            v2 = &vertices[(Uint16)*mesh_cursor++];
            prefetch(&vertices[(Uint16)*mesh_cursor]);
            facing =
                (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                winding;
            if ((Uint32)run_start < (Uint32)sq_cursor) {
                if (facing < 0.0F) {
                    NJM_SET_POLY_STRIP(v1, KM_VERTEXPARAM_NORMAL);
                } else {
                    NJM_SET_POLY_STRIP(v1, KM_VERTEXPARAM_ENDOFSTRIP);

                    accounted_bytes +=
                        (Uint32)sq_cursor - (Uint32)run_start + VERTEX_BYTES;
                    run_start = (Uint8 *)sq_cursor;
                }
            } else {
                if (facing < 0.0F) {
                    NJM_SET_POLY_STRIP(v0, KM_VERTEXPARAM_NORMAL);
                    NJM_SET_POLY_STRIP(v1, KM_VERTEXPARAM_NORMAL);
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
        if ((Uint32)run_start < (Uint32)sq_cursor) {
            NJM_SET_POLY_STRIP(v2, KM_VERTEXPARAM_ENDOFSTRIP);
            accounted_bytes +=
                (Uint32)sq_cursor - (Uint32)run_start + VERTEX_BYTES;
        }
    } while (--strips_left);
    _nj_draw_polygon_ += accounted_bytes >> VERTEX_SHIFT;
    _nj_calc_polygon_ += input_corners;
}

static void simple_draw_smooth_tex_strips_cull(NJS_MESHSET *meshset)
{
    register Sint16 *mesh_cursor;
    register NJS_COLOR *corner_uv;
    Sint32 remaining;
    struct calc_vertex *vertices;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register Uint8 *run_start;
    Uint32 accounted_bytes;
    Uint32 input_corners;
    Sint32 strips_left;
    register Float winding;
    register Float x1;
    register Float x0;
    Float y0;
    Float y1;
    Float facing;

    DRAW_NOOP();
    corner_uv = meshset->vertuv;
    mesh_cursor = meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct calc_vertex *)_nj_vertex_buf_;
    input_corners = accounted_bytes = 0;
/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c62e4c2, the pool at 0x8c62e4c8). without it, SHC writes
 * simple_draw_smooth_strips_cull's literals behind that function's RTS, making
 * it 14 bytes longer, and this function's in a later pool: 292 of 315 words
 * differ and the function is 18 bytes shorter */
#pragma asm
    ;
#pragma endasm
    do {
        remaining = (Uint16)*mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_corners += remaining;
        remaining -= 2;
        v0 = &vertices[(Uint16)*mesh_cursor++];
        x0 = v0->sx;
        v1 = &vertices[(Uint16)*mesh_cursor++];
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = (Uint8 *)sq_cursor;
        do {
            v2 = &vertices[(Uint16)*mesh_cursor++];
            prefetch(&vertices[(Uint16)*mesh_cursor]);
            facing =
                (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                winding;
            if ((Uint32)run_start < (Uint32)sq_cursor) {
                if (facing < 0.0F) {
                    _NJM_SET_TEX_STRIP(v1, KM_VERTEXPARAM_NORMAL,
                                       (NJS_TEX *)(corner_uv++));
                } else {
                    _NJM_SET_TEX_STRIP(v1, KM_VERTEXPARAM_ENDOFSTRIP,
                                       (NJS_TEX *)(corner_uv++));

                    /* corner_uv points at v0's pair while no run is open, and
                     * at v1's while one is. v1, which ends this run, is v0 for
                     * the next triangle, so corner_uv steps back to its pair */
                    corner_uv--;
                    accounted_bytes +=
                        (Uint32)sq_cursor - (Uint32)run_start + VERTEX_BYTES;
                    run_start = (Uint8 *)sq_cursor;
                }
            } else {
                corner_uv++;
                if (facing < 0.0F) {
                    corner_uv--;
                    _NJM_SET_TEX_STRIP(v0, KM_VERTEXPARAM_NORMAL,
                                       (NJS_TEX *)(corner_uv++));
                    _NJM_SET_TEX_STRIP(v1, KM_VERTEXPARAM_NORMAL,
                                       (NJS_TEX *)(corner_uv++));
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
        if ((Uint32)run_start < (Uint32)sq_cursor) {
            corner_uv -= 2;
            _NJM_SET_TEX_STRIP(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                               (NJS_TEX *)(corner_uv++));
            accounted_bytes +=
                (Uint32)sq_cursor - (Uint32)run_start + VERTEX_BYTES;
        }
    } while (--strips_left);
    _nj_draw_polygon_ += accounted_bytes >> VERTEX_SHIFT;
    _nj_calc_polygon_ += input_corners;
}

static void simple_draw_smooth_env_strips_cull(NJS_MESHSET *meshset)
{
    register Sint16 *mesh_cursor;
    Sint32 remaining;
    register struct calc_vertex *vertices;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register Uint8 *run_start;
    Uint32 accounted_bytes;
    Uint32 input_corners;
    Sint32 strips_left;
    register Float winding;
    register Float x1;
    register Float x0;
    Float y0;
    Float y1;
    Float facing;

    DRAW_NOOP();
    mesh_cursor = meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct calc_vertex *)_nj_vertex_buf_;
    input_corners = accounted_bytes = 0;
/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c62e75e, the pool at 0x8c62e764). without it, SHC writes
 * simple_draw_smooth_tex_strips_cull's literals behind that function's RTS,
 * making it 14 bytes longer, and this function's in a later pool: 229 of 252
 * words differ and the function is 18 bytes shorter */
#pragma asm
    ;
#pragma endasm
    do {
        remaining = (Uint16)*mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_corners += remaining;
        remaining -= 2;
        v0 = &vertices[(Uint16)*mesh_cursor++];
        x0 = v0->sx;
        v1 = &vertices[(Uint16)*mesh_cursor++];
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = (Uint8 *)sq_cursor;
        do {
            v2 = &vertices[(Uint16)*mesh_cursor++];
            prefetch(&vertices[(Uint16)*mesh_cursor]);
            facing =
                (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                winding;
            if ((Uint32)run_start < (Uint32)sq_cursor) {
                if (facing < 0.0F) {
                    NJM_SET_ENV_STRIP(v1, KM_VERTEXPARAM_NORMAL);
                } else {
                    NJM_SET_ENV_STRIP(v1, KM_VERTEXPARAM_ENDOFSTRIP);
                    accounted_bytes +=
                        (Uint32)sq_cursor - (Uint32)run_start + VERTEX_BYTES;
                    run_start = (Uint8 *)sq_cursor;
                }
            } else {
                if (facing < 0.0F) {
                    NJM_SET_ENV_STRIP(v0, KM_VERTEXPARAM_NORMAL);
                    NJM_SET_ENV_STRIP(v1, KM_VERTEXPARAM_NORMAL);
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
        if ((Uint32)run_start < (Uint32)sq_cursor) {
            NJM_SET_ENV_STRIP(v2, KM_VERTEXPARAM_ENDOFSTRIP);
            accounted_bytes +=
                (Uint32)sq_cursor - (Uint32)run_start + VERTEX_BYTES;
        }
    } while (--strips_left);
    _nj_draw_polygon_ += accounted_bytes >> VERTEX_SHIFT;
    _nj_calc_polygon_ += input_corners;
}

/* the flat_path drawers draw the strip meshsets the flat polygon drawers pass
 * them. each _cull one runs its smooth one's code, which lights every vertex.
 * under a _FLAT_ color mode, each triangle then takes the color of the vertex
 * that completes it */
static void simple_draw_flat_path_strips_cull(NJS_MESHSET *meshset)
{
    register Sint16 *mesh_cursor;
    Sint32 remaining;
    register struct calc_vertex *vertices;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register Uint8 *run_start;
    Uint32 accounted_bytes;
    Uint32 input_corners;
    Sint32 strips_left;
    register Float winding;
    register Float x1;
    register Float x0;
    Float y0;
    Float y1;
    Float facing;

    DRAW_NOOP();
    mesh_cursor = meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct calc_vertex *)_nj_vertex_buf_;
    input_corners = accounted_bytes = 0;
/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c62e97e, the pool at 0x8c62e984). without it, SHC writes
 * simple_draw_smooth_env_strips_cull's literals behind that function's RTS,
 * making it 14 bytes longer, and this function's in a later pool: 277 of 300
 * words differ and the function is 18 bytes shorter */
#pragma asm
    ;
#pragma endasm
    do {
        remaining = (Uint16)*mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_corners += remaining;
        remaining -= 2;
        v0 = &vertices[(Uint16)*mesh_cursor++];
        x0 = v0->sx;
        v1 = &vertices[(Uint16)*mesh_cursor++];
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = (Uint8 *)sq_cursor;
        do {
            v2 = &vertices[(Uint16)*mesh_cursor++];
            prefetch(&vertices[(Uint16)*mesh_cursor]);
            facing =
                (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                winding;
            if ((Uint32)run_start < (Uint32)sq_cursor) {
                if (facing < 0.0F) {
                    NJM_SET_POLY_STRIP(v1, KM_VERTEXPARAM_NORMAL);
                } else {
                    NJM_SET_POLY_STRIP(v1, KM_VERTEXPARAM_ENDOFSTRIP);
                    accounted_bytes +=
                        (Uint32)sq_cursor - (Uint32)run_start + VERTEX_BYTES;
                    run_start = (Uint8 *)sq_cursor;
                }
            } else {
                if (facing < 0.0F) {
                    NJM_SET_POLY_STRIP(v0, KM_VERTEXPARAM_NORMAL);
                    NJM_SET_POLY_STRIP(v1, KM_VERTEXPARAM_NORMAL);
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
        if ((Uint32)run_start < (Uint32)sq_cursor) {
            NJM_SET_POLY_STRIP(v2, KM_VERTEXPARAM_ENDOFSTRIP);
            accounted_bytes +=
                (Uint32)sq_cursor - (Uint32)run_start + VERTEX_BYTES;
        }
    } while (--strips_left);
    _nj_draw_polygon_ += accounted_bytes >> VERTEX_SHIFT;
    _nj_calc_polygon_ += input_corners;
}

/* simple_draw_smooth_tex_strips_cull's code, for the flat drawers' strips */
static void simple_draw_flat_path_tex_strips_cull(NJS_MESHSET *meshset)
{
    register Sint16 *mesh_cursor;
    register NJS_COLOR *corner_uv;
    Sint32 remaining;
    struct calc_vertex *vertices;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register Uint8 *run_start;
    Uint32 accounted_bytes;
    Uint32 input_corners;
    Sint32 strips_left;
    register Float winding;
    register Float x1;
    register Float x0;
    Float y0;
    Float y1;
    Float facing;

    DRAW_NOOP();
    corner_uv = meshset->vertuv;
    mesh_cursor = meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct calc_vertex *)_nj_vertex_buf_;
    input_corners = accounted_bytes = 0;
/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c62ebfe, the pool at 0x8c62ec04). without it, SHC writes
 * simple_draw_flat_path_strips_cull's literals behind that function's RTS,
 * making it 14 bytes longer, and this function's in a later pool: 292 of 315
 * words differ and the function is 18 bytes shorter */
#pragma asm
    ;
#pragma endasm
    do {
        remaining = (Uint16)*mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_corners += remaining;
        remaining -= 2;
        v0 = &vertices[(Uint16)*mesh_cursor++];
        x0 = v0->sx;
        v1 = &vertices[(Uint16)*mesh_cursor++];
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = (Uint8 *)sq_cursor;
        do {
            v2 = &vertices[(Uint16)*mesh_cursor++];
            prefetch(&vertices[(Uint16)*mesh_cursor]);
            facing =
                (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                winding;
            if ((Uint32)run_start < (Uint32)sq_cursor) {
                if (facing < 0.0F) {
                    _NJM_SET_TEX_STRIP(v1, KM_VERTEXPARAM_NORMAL,
                                       (NJS_TEX *)(corner_uv++));
                } else {
                    _NJM_SET_TEX_STRIP(v1, KM_VERTEXPARAM_ENDOFSTRIP,
                                       (NJS_TEX *)(corner_uv++));
                    corner_uv--;
                    accounted_bytes +=
                        (Uint32)sq_cursor - (Uint32)run_start + VERTEX_BYTES;
                    run_start = (Uint8 *)sq_cursor;
                }
            } else {
                corner_uv++;
                if (facing < 0.0F) {
                    corner_uv--;
                    _NJM_SET_TEX_STRIP(v0, KM_VERTEXPARAM_NORMAL,
                                       (NJS_TEX *)(corner_uv++));
                    _NJM_SET_TEX_STRIP(v1, KM_VERTEXPARAM_NORMAL,
                                       (NJS_TEX *)(corner_uv++));
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
        if ((Uint32)run_start < (Uint32)sq_cursor) {
            corner_uv -= 2;
            _NJM_SET_TEX_STRIP(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                               (NJS_TEX *)(corner_uv++));
            accounted_bytes +=
                (Uint32)sq_cursor - (Uint32)run_start + VERTEX_BYTES;
        }
    } while (--strips_left);
    _nj_draw_polygon_ += accounted_bytes >> VERTEX_SHIFT;
    _nj_calc_polygon_ += input_corners;
}

static void simple_draw_unlit_tex_strips_cull(NJS_MESHSET *meshset)
{
    register Sint16 *mesh_cursor;
    register NJS_COLOR *corner_uv;
    Sint32 remaining;
    struct calc_vertex *vertices;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register Uint8 *run_start;
    Uint32 accounted_bytes;
    Uint32 input_corners;
    Sint32 strips_left;
    register Float winding;
    register Float x1;
    register Float x0;
    Float y0;
    Float y1;
    Float facing;

    DRAW_NOOP();
    corner_uv = meshset->vertuv;
    mesh_cursor = meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct calc_vertex *)_nj_vertex_buf_;
    input_corners = accounted_bytes = 0;
/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c62ee9a, the pool at 0x8c62eea0). without it, SHC writes
 * simple_draw_flat_path_tex_strips_cull's literals behind that function's RTS,
 * making it 14 bytes longer, and this function's in a later pool: 272 of 295
 * words differ and the function is 18 bytes shorter */
#pragma asm
    ;
#pragma endasm
    do {
        remaining = (Uint16)*mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_corners += remaining;
        remaining -= 2;
        v0 = &vertices[(Uint16)*mesh_cursor++];
        x0 = v0->sx;
        v1 = &vertices[(Uint16)*mesh_cursor++];
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = (Uint8 *)sq_cursor;
        do {
            v2 = &vertices[(Uint16)*mesh_cursor++];
            prefetch(&vertices[(Uint16)*mesh_cursor]);
            facing =
                (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                winding;
            if ((Uint32)run_start < (Uint32)sq_cursor) {
                if (facing < 0.0F) {
                    _NJM_SET_TEX_X_STRIP(v1, KM_VERTEXPARAM_NORMAL,
                                         (NJS_TEX *)(corner_uv++));
                } else {
                    _NJM_SET_TEX_X_STRIP(v1, KM_VERTEXPARAM_ENDOFSTRIP,
                                         (NJS_TEX *)(corner_uv++));
                    corner_uv--;
                    accounted_bytes +=
                        (Uint32)sq_cursor - (Uint32)run_start + VERTEX_BYTES;
                    run_start = (Uint8 *)sq_cursor;
                }
            } else {
                corner_uv++;
                if (facing < 0.0F) {
                    corner_uv--;
                    _NJM_SET_TEX_X_STRIP(v0, KM_VERTEXPARAM_NORMAL,
                                         (NJS_TEX *)(corner_uv++));
                    _NJM_SET_TEX_X_STRIP(v1, KM_VERTEXPARAM_NORMAL,
                                         (NJS_TEX *)(corner_uv++));
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
        if ((Uint32)run_start < (Uint32)sq_cursor) {
            corner_uv -= 2;
            _NJM_SET_TEX_X_STRIP(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                                 (NJS_TEX *)(corner_uv++));
            accounted_bytes +=
                (Uint32)sq_cursor - (Uint32)run_start + VERTEX_BYTES;
        }
    } while (--strips_left);
    _nj_draw_polygon_ += accounted_bytes >> VERTEX_SHIFT;
    _nj_calc_polygon_ += input_corners;
}

/* simple_draw_smooth_env_strips_cull's code, for the flat drawers' strips */
static void simple_draw_flat_path_env_strips_cull(NJS_MESHSET *meshset)
{
    register Sint16 *mesh_cursor;
    Sint32 remaining;
    register struct calc_vertex *vertices;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register Uint8 *run_start;
    Uint32 accounted_bytes;
    Uint32 input_corners;
    Sint32 strips_left;
    register Float winding;
    register Float x1;
    register Float x0;
    Float y0;
    Float y1;
    Float facing;

    DRAW_NOOP();
    mesh_cursor = meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct calc_vertex *)_nj_vertex_buf_;
    input_corners = accounted_bytes = 0;
/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c62f110, the pool at 0x8c62f114). without it, SHC writes
 * simple_draw_unlit_tex_strips_cull's literals behind that function's RTS,
 * making it 12 bytes longer, and this function's in a later pool: 229 of 252
 * words differ and the function is 16 bytes shorter */
#pragma asm
    ;
#pragma endasm
    do {
        remaining = (Uint16)*mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_corners += remaining;
        remaining -= 2;
        v0 = &vertices[(Uint16)*mesh_cursor++];
        x0 = v0->sx;
        v1 = &vertices[(Uint16)*mesh_cursor++];
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = (Uint8 *)sq_cursor;
        do {
            v2 = &vertices[(Uint16)*mesh_cursor++];
            prefetch(&vertices[(Uint16)*mesh_cursor]);
            facing =
                (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                winding;
            if ((Uint32)run_start < (Uint32)sq_cursor) {
                if (facing < 0.0F) {
                    NJM_SET_ENV_STRIP(v1, KM_VERTEXPARAM_NORMAL);
                } else {
                    NJM_SET_ENV_STRIP(v1, KM_VERTEXPARAM_ENDOFSTRIP);
                    accounted_bytes +=
                        (Uint32)sq_cursor - (Uint32)run_start + VERTEX_BYTES;
                    run_start = (Uint8 *)sq_cursor;
                }
            } else {
                if (facing < 0.0F) {
                    NJM_SET_ENV_STRIP(v0, KM_VERTEXPARAM_NORMAL);
                    NJM_SET_ENV_STRIP(v1, KM_VERTEXPARAM_NORMAL);
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
        if ((Uint32)run_start < (Uint32)sq_cursor) {
            NJM_SET_ENV_STRIP(v2, KM_VERTEXPARAM_ENDOFSTRIP);
            accounted_bytes +=
                (Uint32)sq_cursor - (Uint32)run_start + VERTEX_BYTES;
        }
    } while (--strips_left);
    _nj_draw_polygon_ += accounted_bytes >> VERTEX_SHIFT;
    _nj_calc_polygon_ += input_corners;
}

static void simple_draw_unlit_env_strips_cull(NJS_MESHSET *meshset)
{
    register Sint16 *mesh_cursor;
    Sint32 remaining;
    register struct calc_vertex *vertices;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register Uint8 *run_start;
    Uint32 accounted_bytes;
    Uint32 input_corners;
    Sint32 strips_left;
    register Float winding;
    register Float x1;
    register Float x0;
    Float y0;
    Float y1;
    Float facing;

    DRAW_NOOP();
    mesh_cursor = meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct calc_vertex *)_nj_vertex_buf_;
    input_corners = accounted_bytes = 0;
/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c62f32e, the pool at 0x8c62f334). without it, SHC writes
 * simple_draw_flat_path_env_strips_cull's literals behind that function's RTS,
 * making it 14 bytes longer, and this function's in a later pool: 233 of 256
 * words differ and the function is 18 bytes shorter */
#pragma asm
    ;
#pragma endasm
    do {
        remaining = (Uint16)*mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_corners += remaining;
        remaining -= 2;
        v0 = &vertices[(Uint16)*mesh_cursor++];
        x0 = v0->sx;
        v1 = &vertices[(Uint16)*mesh_cursor++];
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = (Uint8 *)sq_cursor;
        do {
            v2 = &vertices[(Uint16)*mesh_cursor++];
            prefetch(&vertices[(Uint16)*mesh_cursor]);
            facing =
                (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                winding;
            if ((Uint32)run_start < (Uint32)sq_cursor) {
                if (facing < 0.0F) {
                    NJM_SET_ENV_X_STRIP(v1, KM_VERTEXPARAM_NORMAL);
                } else {
                    NJM_SET_ENV_X_STRIP(v1, KM_VERTEXPARAM_ENDOFSTRIP);
                    accounted_bytes +=
                        (Uint32)sq_cursor - (Uint32)run_start + VERTEX_BYTES;
                    run_start = (Uint8 *)sq_cursor;
                }
            } else {
                if (facing < 0.0F) {
                    NJM_SET_ENV_X_STRIP(v0, KM_VERTEXPARAM_NORMAL);
                    NJM_SET_ENV_X_STRIP(v1, KM_VERTEXPARAM_NORMAL);
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
        if ((Uint32)run_start < (Uint32)sq_cursor) {
            NJM_SET_ENV_X_STRIP(v2, KM_VERTEXPARAM_ENDOFSTRIP);
            accounted_bytes +=
                (Uint32)sq_cursor - (Uint32)run_start + VERTEX_BYTES;
        }
    } while (--strips_left);
    _nj_draw_polygon_ += accounted_bytes >> VERTEX_SHIFT;
    _nj_calc_polygon_ += input_corners;
}

static void simple_draw_unlit_strips_cull(NJS_MESHSET *meshset)
{
    register Sint16 *mesh_cursor;
    Sint32 remaining;
    register struct calc_vertex *vertices;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register Uint8 *run_start;
    Uint32 accounted_bytes;
    Uint32 input_corners;
    Sint32 strips_left;
    register Float winding;
    register Float x1;
    register Float x0;
    Float y0;
    Float y1;
    Float facing;

    DRAW_NOOP();
    mesh_cursor = meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct calc_vertex *)_nj_vertex_buf_;
    input_corners = accounted_bytes = 0;
/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c62f554, the pool at 0x8c62f558). without it, SHC writes
 * simple_draw_unlit_env_strips_cull's literals behind that function's RTS,
 * making it 12 bytes longer, and this function's in a later pool: 212 of 238
 * words differ and the function is 16 bytes shorter */
#pragma asm
    ;
#pragma endasm
    do {
        remaining = (Uint16)*mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        input_corners += remaining;
        remaining -= 2;
        v0 = &vertices[(Uint16)*mesh_cursor++];
        x0 = v0->sx;
        v1 = &vertices[(Uint16)*mesh_cursor++];
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        run_start = (Uint8 *)sq_cursor;
        do {
            v2 = &vertices[(Uint16)*mesh_cursor++];
            prefetch(&vertices[(Uint16)*mesh_cursor]);
            facing =
                (((x1 - x0) * (v2->sy - y1)) - ((y1 - y0) * (v2->sx - x1))) *
                winding;
            if ((Uint32)run_start < (Uint32)sq_cursor) {
                if (facing < 0.0F) {
                    NJM_SET_POLY_X_STRIP(v1, KM_VERTEXPARAM_NORMAL);
                } else {
                    NJM_SET_POLY_X_STRIP(v1, KM_VERTEXPARAM_ENDOFSTRIP);
                    accounted_bytes +=
                        (Uint32)sq_cursor - (Uint32)run_start + VERTEX_BYTES;
                    run_start = (Uint8 *)sq_cursor;
                }
            } else {
                if (facing < 0.0F) {
                    NJM_SET_POLY_X_STRIP(v0, KM_VERTEXPARAM_NORMAL);
                    NJM_SET_POLY_X_STRIP(v1, KM_VERTEXPARAM_NORMAL);
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
        if ((Uint32)run_start < (Uint32)sq_cursor) {
            NJM_SET_POLY_X_STRIP(v2, KM_VERTEXPARAM_ENDOFSTRIP);
            accounted_bytes +=
                (Uint32)sq_cursor - (Uint32)run_start + VERTEX_BYTES;
        }
    } while (--strips_left);
    _nj_draw_polygon_ += accounted_bytes >> VERTEX_SHIFT;
    _nj_calc_polygon_ += input_corners;
}

static void simple_draw_smooth_strips_near(NJS_MESHSET *meshset)
{
    register struct calc_vertex *vertices;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register Uint16 *mesh_cursor;
    register Float *run_start;
    register Float x0;
    register Float y0;
    register Float x1;
    register Float y1;
    register Float winding;
    register Float facing;
    Uint32 outside_all;
    Uint32 crosses_near;
    Uint32 input_corners;
    Uint32 remaining;
    Sint32 strips_left;
    Sint32 needs_clip;

    DRAW_NOOP();
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct calc_vertex *)_nj_vertex_buf_;
    input_corners = 0;
/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c62f754, the pool at 0x8c62f758). without it, SHC writes
 * simple_draw_unlit_strips_cull's literals behind that function's RTS, making
 * it 12 bytes longer, and this function's in a later pool: 431 of 461 words
 * differ and the function is 16 bytes shorter */
#pragma asm
    ;
#pragma endasm
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        run_start = sq_cursor;
        input_corners += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        do {
            v2 = &vertices[*mesh_cursor++];
            prefetch(&vertices[*mesh_cursor]);
            needs_clip = 0;
            facing = 1.0F;
            outside_all = v0->clip & v1->clip & v2->clip & CLIP_ALL;

            if (outside_all) {
                goto submit_run;
            }
            crosses_near = (v0->clip | v1->clip | v2->clip) & CLIP_NEAR;
            if (crosses_near) {
                struct calc_vertex *triangle[3];
                NJS_POINT3 view_points[3];
                Float view_z;
                int corner;

                triangle[0] = v0;
                triangle[1] = v1;
                triangle[2] = v2;
                for (corner = 0; corner < 3; corner++) {
                    DECODE_VIEW_POINT(triangle[corner], view_points[corner],
                                      view_z);
                }
                facing = (view_points[2].x *
                          ((view_points[0].z * view_points[1].y) -
                           (view_points[0].y * view_points[1].z))) +
                         (view_points[2].y *
                          ((view_points[0].x * view_points[1].z) -
                           (view_points[0].z * view_points[1].x))) +
                         (view_points[2].z *
                          ((view_points[1].x * view_points[0].y) -
                           (view_points[0].x * view_points[1].y)));
                facing *= -winding;
                if (!(facing > 0.0F)) {
                    needs_clip = 1;
                    facing = 1.0F;
                }
            } else {
                facing = (((x1 - x0) * (v2->sy - y1)) -
                          ((y1 - y0) * (v2->sx - x1))) *
                         winding;
            }
        submit_run:
            if ((Uint32)run_start < (Uint32)sq_cursor) {
                if (facing < 0.0F) {
                    NJM_SET_POLY_STRIP(v1, KM_VERTEXPARAM_NORMAL);
                } else {
                    NJM_SET_POLY_STRIP(v1, KM_VERTEXPARAM_ENDOFSTRIP);
                    run_start = sq_cursor;
                }
            } else if (facing < 0.0F) {
                NJM_SET_POLY_STRIP(v0, KM_VERTEXPARAM_NORMAL);
                NJM_SET_POLY_STRIP(v1, KM_VERTEXPARAM_NORMAL);
            }
            if (needs_clip) {
                clip_cull_requested = 0;
                simple_clip_smooth_triangle(v0, v1, v2);
                run_start = sq_cursor;
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            winding = -winding;
            y1 = v2->sy;
        } while (--remaining != 0);
        if ((Uint32)run_start < (Uint32)sq_cursor) {
            NJM_SET_POLY_STRIP(v2, KM_VERTEXPARAM_ENDOFSTRIP);
        }
    } while (--strips_left != 0);

    _nj_calc_polygon_ += input_corners;
    _nj_draw_polygon_ += input_corners;
}

static void simple_draw_smooth_tex_strips_near(NJS_MESHSET *meshset)
{
    register Sint16 *mesh_cursor;
    register NJS_COLOR *corner_uv;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register Uint8 *run_start;
    struct calc_vertex *vertices;
    Sint32 remaining;
    Sint32 input_corners;
    Sint32 strips_left;
    Sint32 needs_clip;
    Uint32 outside_all;
    Uint32 crosses_near;
    register Float winding;
    register Float x0;
    register Float y0;
    register Float x1;
    register Float y1;
    register Float facing;

    DRAW_NOOP();
    corner_uv = meshset->vertuv;
    mesh_cursor = meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct calc_vertex *)_nj_vertex_buf_;
    input_corners = 0;
/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c62fb1c, the pool at 0x8c62fb20). without it, SHC writes
 * simple_draw_smooth_strips_near's literals behind that function's RTS, making
 * it 24 bytes longer, and this function's in a later pool: 496 of 525 words
 * differ and the function is 28 bytes shorter */
#pragma asm
    ;
#pragma endasm
    do {
        remaining = (Uint16)*mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        run_start = (Uint8 *)sq_cursor;
        input_corners += remaining;
        remaining -= 2;
        v0 = &vertices[(Uint16)*mesh_cursor++];
        v1 = &vertices[(Uint16)*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        do {
            v2 = &vertices[(Uint16)*mesh_cursor++];
            prefetch(&vertices[(Uint16)*mesh_cursor]);
            needs_clip = 0;
            facing = 1.0F;
            outside_all = v0->clip & v1->clip & v2->clip & CLIP_ALL;
            if (outside_all) {
                goto submit_run;
            }
            crosses_near = (v0->clip | v1->clip | v2->clip) & CLIP_NEAR;
            if (crosses_near) {
                struct calc_vertex *triangle[3];
                NJS_POINT3 view_points[3];
                Float view_z;
                int corner;

                triangle[0] = v0;
                triangle[1] = v1;
                triangle[2] = v2;
                for (corner = 0; corner < 3; corner++) {
                    DECODE_VIEW_POINT(triangle[corner], view_points[corner],
                                      view_z);
                }
                facing = (view_points[2].x *
                          ((view_points[0].z * view_points[1].y) -
                           (view_points[0].y * view_points[1].z))) +
                         (view_points[2].y *
                          ((view_points[0].x * view_points[1].z) -
                           (view_points[0].z * view_points[1].x))) +
                         (view_points[2].z *
                          ((view_points[1].x * view_points[0].y) -
                           (view_points[0].x * view_points[1].y)));
                facing *= -winding;
                if (!(facing > 0.0F)) {
                    needs_clip = 1;
                    facing = 1.0F;
                }
            } else {
                facing = (((x1 - x0) * (v2->sy - y1)) -
                          ((y1 - y0) * (v2->sx - x1))) *
                         winding;
            }
        submit_run:
            if ((Uint32)run_start < (Uint32)sq_cursor) {
                if (facing < 0.0F) {
                    _NJM_SET_TEX_STRIP(v1, KM_VERTEXPARAM_NORMAL,
                                       (NJS_TEX *)(corner_uv++));
                } else {
                    _NJM_SET_TEX_STRIP(v1, KM_VERTEXPARAM_ENDOFSTRIP,
                                       (NJS_TEX *)(corner_uv++));
                    corner_uv--;
                    run_start = (Uint8 *)sq_cursor;
                }
            } else {
                corner_uv++;
                if (facing < 0.0F) {
                    corner_uv--;
                    _NJM_SET_TEX_STRIP(v0, KM_VERTEXPARAM_NORMAL,
                                       (NJS_TEX *)(corner_uv++));
                    _NJM_SET_TEX_STRIP(v1, KM_VERTEXPARAM_NORMAL,
                                       (NJS_TEX *)(corner_uv++));
                }
            }

            if (needs_clip) {
                corner_uv--;
                clip_cull_requested = 0;
                /* @hack type: each coordinate makes a round trip through
                 * Float, which changes no value but has SHC convert it with
                 * FLOAT and FTRC (0x8c62fe2e to 0x8c62fe84). without the
                 * casts, the section runs 52 bytes short and 143 of 525 words
                 * differ */
                corner_uv_window[0].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[0].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[1].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[1].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[2].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[2].tex.v = (Sint16)(Float)corner_uv->tex.v;
                simple_clip_smooth_tex_triangle(v0, v1, v2, corner_uv_window);
                corner_uv--;
                run_start = (Uint8 *)sq_cursor;
            }
            v0 = v1;
            x0 = x1;
            v1 = v2;
            y0 = y1;
            x1 = v2->sx;
            winding = -winding;
            y1 = v2->sy;
        } while (--remaining != 0);
        corner_uv += 2;
        if ((Uint32)run_start < (Uint32)sq_cursor) {
            corner_uv -= 2;
            _NJM_SET_TEX_STRIP(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                               (NJS_TEX *)(corner_uv++));
        }
    } while (--strips_left);
    _nj_calc_polygon_ += input_corners;
    _nj_draw_polygon_ += input_corners;
}

static void simple_draw_smooth_env_strips_near(NJS_MESHSET *meshset)
{
    register struct calc_vertex *vertices;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register Uint16 *mesh_cursor;
    register Float *run_start;
    register Float x0;
    register Float y0;
    register Float x1;
    register Float y1;
    register Float winding;
    register Float facing;
    Uint32 outside_all;
    Uint32 crosses_near;
    Uint32 input_corners;
    Uint32 remaining;
    Sint32 strips_left;
    Sint32 needs_clip;

    DRAW_NOOP();
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct calc_vertex *)_nj_vertex_buf_;
    input_corners = 0;
/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c62ff70, the pool at 0x8c62ff74). without it, SHC writes
 * simple_draw_smooth_tex_strips_near's literals behind that function's RTS,
 * making it 28 bytes longer, and this function's in a later pool: 387 of 413
 * words differ and the function is 32 bytes shorter */
#pragma asm
    ;
#pragma endasm
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        run_start = sq_cursor;
        input_corners += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        do {
            v2 = &vertices[*mesh_cursor++];
            prefetch(&vertices[*mesh_cursor]);
            needs_clip = 0;
            facing = 1.0F;
            outside_all = v0->clip & v1->clip & v2->clip & CLIP_ALL;
            if (outside_all) {
                goto submit_run;
            }
            crosses_near = (v0->clip | v1->clip | v2->clip) & CLIP_NEAR;
            if (crosses_near) {
                struct calc_vertex *triangle[3];
                NJS_POINT3 view_points[3];
                Float view_z;
                int corner;

                triangle[0] = v0;
                triangle[1] = v1;
                triangle[2] = v2;
                for (corner = 0; corner < 3; corner++) {
                    DECODE_VIEW_POINT(triangle[corner], view_points[corner],
                                      view_z);
                }
                facing = (view_points[2].x *
                          ((view_points[0].z * view_points[1].y) -
                           (view_points[0].y * view_points[1].z))) +
                         (view_points[2].y *
                          ((view_points[0].x * view_points[1].z) -
                           (view_points[0].z * view_points[1].x))) +
                         (view_points[2].z *
                          ((view_points[1].x * view_points[0].y) -
                           (view_points[0].x * view_points[1].y)));
                facing *= -winding;
                if (!(facing > 0.0F)) {
                    needs_clip = 1;
                    facing = 1.0F;
                }
            } else {
                facing = (((x1 - x0) * (v2->sy - y1)) -
                          ((y1 - y0) * (v2->sx - x1))) *
                         winding;
            }
        submit_run:
            if ((Uint32)run_start < (Uint32)sq_cursor) {
                if (facing < 0.0F) {
                    NJM_SET_ENV_STRIP(v1, KM_VERTEXPARAM_NORMAL);
                } else {
                    NJM_SET_ENV_STRIP(v1, KM_VERTEXPARAM_ENDOFSTRIP);
                    run_start = sq_cursor;
                }
            } else if (facing < 0.0F) {
                NJM_SET_ENV_STRIP(v0, KM_VERTEXPARAM_NORMAL);
                NJM_SET_ENV_STRIP(v1, KM_VERTEXPARAM_NORMAL);
            }
            if (needs_clip) {
                clip_cull_requested = 0;
                simple_clip_smooth_env_triangle(v0, v1, v2);
                run_start = sq_cursor;
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            winding = -winding;
            y1 = v2->sy;
        } while (--remaining != 0);
        if ((Uint32)run_start < (Uint32)sq_cursor) {
            NJM_SET_ENV_STRIP(v2, KM_VERTEXPARAM_ENDOFSTRIP);
        }
    } while (--strips_left != 0);
    _nj_calc_polygon_ += input_corners;
    _nj_draw_polygon_ += input_corners;
}

/* the flat clippers, defined below, cut as the clippers above do. the corners
 * they send from the third on carry the intensity or the color they are
 * passed, which a flat color mode reads; the first two carry neither */
static void simple_clip_flat_tex_triangle(struct calc_vertex *v0,
                                          struct calc_vertex *v1,
                                          struct calc_vertex *v2,
                                          NJS_COLOR *corner_uv,
                                          Float intensity);
static void simple_clip_constant_color_triangle(struct calc_vertex *v0,
                                                struct calc_vertex *v1,
                                                struct calc_vertex *v2,
                                                NJS_ARGB *color);
static void simple_clip_flat_env_triangle(struct calc_vertex *v0,
                                          struct calc_vertex *v1,
                                          struct calc_vertex *v2,
                                          Float intensity);

/* the flat_path _near drawers run the smooth _near drawers' code, but send a
 * triangle they clip through a flat clipper. they light it once, at the unit
 * vector along the sum of its corners' normals */
static void simple_draw_flat_path_strips_near(NJS_MESHSET *meshset)
{
    register struct calc_vertex *vertices;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register Uint16 *mesh_cursor;
    register Float *run_start;
    register Float x0;
    register Float y0;
    register Float x1;
    register Float y1;
    register Float winding;
    register Float facing;
    Uint32 outside_all;
    Uint32 crosses_near;
    Uint32 input_corners;
    Uint32 remaining;
    Sint32 strips_left;
    Sint32 needs_clip;

    DRAW_NOOP();
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct calc_vertex *)_nj_vertex_buf_;
    input_corners = 0;
/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c6302e8, the pool at 0x8c6302ec). without it, SHC writes
 * simple_draw_smooth_env_strips_near's literals behind that function's RTS,
 * making it 24 bytes longer, and this function's in a later pool: 504 of 531
 * words differ and the function is 28 bytes shorter */
#pragma asm
    ;
#pragma endasm
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        run_start = sq_cursor;
        input_corners += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        do {
            v2 = &vertices[*mesh_cursor++];
            prefetch(&vertices[*mesh_cursor]);
            needs_clip = 0;
            facing = 1.0F;
            outside_all = v0->clip & v1->clip & v2->clip & CLIP_ALL;
            if (outside_all) {
                goto submit_run;
            }
            crosses_near = (v0->clip | v1->clip | v2->clip) & CLIP_NEAR;
            if (crosses_near) {
                struct calc_vertex *triangle[3];
                NJS_POINT3 view_points[3];
                Float view_z;
                int corner;

                triangle[0] = v0;
                triangle[1] = v1;
                triangle[2] = v2;
                for (corner = 0; corner < 3; corner++) {
                    DECODE_VIEW_POINT(triangle[corner], view_points[corner],
                                      view_z);
                }
                facing = (view_points[2].x *
                          ((view_points[0].z * view_points[1].y) -
                           (view_points[0].y * view_points[1].z))) +
                         (view_points[2].y *
                          ((view_points[0].x * view_points[1].z) -
                           (view_points[0].z * view_points[1].x))) +
                         (view_points[2].z *
                          ((view_points[1].x * view_points[0].y) -
                           (view_points[0].x * view_points[1].y)));
                facing *= -winding;
                if (!(facing > 0.0F)) {
                    needs_clip = 1;
                    facing = 1.0F;
                }
            } else {
                facing = (((x1 - x0) * (v2->sy - y1)) -
                          ((y1 - y0) * (v2->sx - x1))) *
                         winding;
            }
        submit_run:
            if ((Uint32)run_start < (Uint32)sq_cursor) {
                if (facing < 0.0F) {
                    NJM_SET_POLY_STRIP(v1, KM_VERTEXPARAM_NORMAL);
                } else {
                    NJM_SET_POLY_STRIP(v1, KM_VERTEXPARAM_ENDOFSTRIP);
                    run_start = sq_cursor;
                }
            } else if (facing < 0.0F) {
                NJM_SET_POLY_STRIP(v0, KM_VERTEXPARAM_NORMAL);
                NJM_SET_POLY_STRIP(v1, KM_VERTEXPARAM_NORMAL);
            }
            if (needs_clip) {
                NJS_VECTOR flat_normal;
                NJS_ARGB flat_color;
                Float flat_intensity;

                clip_cull_requested = 0;
                flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
                flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
                flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                flat_color.a = _nj_light_material_.diffuse.a;
                flat_color.r = _nj_light_material_.diffuse.r * flat_intensity;
                flat_color.g = _nj_light_material_.diffuse.g * flat_intensity;
                flat_color.b = _nj_light_material_.diffuse.b * flat_intensity;
                simple_clip_constant_color_triangle(v0, v1, v2, &flat_color);
                run_start = sq_cursor;
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            winding = -winding;
            y1 = v2->sy;
        } while (--remaining != 0);
        if ((Uint32)run_start < (Uint32)sq_cursor) {
            NJM_SET_POLY_STRIP(v2, KM_VERTEXPARAM_ENDOFSTRIP);
        }
    } while (--strips_left != 0);
    _nj_calc_polygon_ += input_corners;
    _nj_draw_polygon_ += input_corners;
}

static void simple_draw_flat_path_tex_strips_near(NJS_MESHSET *meshset)
{
    register Sint16 *mesh_cursor;
    register NJS_COLOR *corner_uv;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register Uint8 *run_start;
    struct calc_vertex *vertices;
    Sint32 remaining;
    Sint32 input_corners;
    Sint32 strips_left;
    Sint32 needs_clip;
    Uint32 outside_all;
    Uint32 crosses_near;
    register Float winding;
    register Float x0;
    register Float y0;
    register Float x1;
    register Float y1;
    register Float facing;

    DRAW_NOOP();
    corner_uv = meshset->vertuv;
    mesh_cursor = meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct calc_vertex *)_nj_vertex_buf_;
    input_corners = 0;
/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c630748, the pool at 0x8c63074c). without it, SHC writes
 * simple_draw_flat_path_strips_near's literals behind that function's RTS,
 * making it 40 bytes longer, and this function's in a later pool: 547 of 573
 * words differ and the function is 44 bytes shorter */
#pragma asm
    ;
#pragma endasm
    do {
        remaining = (Uint16)*mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        run_start = (Uint8 *)sq_cursor;
        input_corners += remaining;
        remaining -= 2;
        v0 = &vertices[(Uint16)*mesh_cursor++];
        v1 = &vertices[(Uint16)*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        do {
            v2 = &vertices[(Uint16)*mesh_cursor++];
            prefetch(&vertices[(Uint16)*mesh_cursor]);
            needs_clip = 0;
            facing = 1.0F;
            outside_all = v0->clip & v1->clip & v2->clip & CLIP_ALL;
            if (outside_all) {
                goto submit_run;
            }
            crosses_near = (v0->clip | v1->clip | v2->clip) & CLIP_NEAR;
            if (crosses_near) {
                struct calc_vertex *triangle[3];
                NJS_POINT3 view_points[3];
                Float view_z;
                int corner;

                triangle[0] = v0;
                triangle[1] = v1;
                triangle[2] = v2;
                for (corner = 0; corner < 3; corner++) {
                    DECODE_VIEW_POINT(triangle[corner], view_points[corner],
                                      view_z);
                }
                facing = (view_points[2].x *
                          ((view_points[0].z * view_points[1].y) -
                           (view_points[0].y * view_points[1].z))) +
                         (view_points[2].y *
                          ((view_points[0].x * view_points[1].z) -
                           (view_points[0].z * view_points[1].x))) +
                         (view_points[2].z *
                          ((view_points[1].x * view_points[0].y) -
                           (view_points[0].x * view_points[1].y)));
                facing *= -winding;
                if (!(facing > 0.0F)) {
                    needs_clip = 1;
                    facing = 1.0F;
                }
            } else {
                facing = (((x1 - x0) * (v2->sy - y1)) -
                          ((y1 - y0) * (v2->sx - x1))) *
                         winding;
            }
        submit_run:
            if ((Uint32)run_start < (Uint32)sq_cursor) {
                if (facing < 0.0F) {
                    _NJM_SET_TEX_STRIP(v1, KM_VERTEXPARAM_NORMAL,
                                       (NJS_TEX *)(corner_uv++));
                } else {
                    _NJM_SET_TEX_STRIP(v1, KM_VERTEXPARAM_ENDOFSTRIP,
                                       (NJS_TEX *)(corner_uv++));
                    corner_uv--;
                    run_start = (Uint8 *)sq_cursor;
                }
            } else {
                corner_uv++;
                if (facing < 0.0F) {
                    corner_uv--;
                    _NJM_SET_TEX_STRIP(v0, KM_VERTEXPARAM_NORMAL,
                                       (NJS_TEX *)(corner_uv++));
                    _NJM_SET_TEX_STRIP(v1, KM_VERTEXPARAM_NORMAL,
                                       (NJS_TEX *)(corner_uv++));
                }
            }
            if (needs_clip) {
                NJS_VECTOR flat_normal;
                Float flat_intensity;

                corner_uv--;
                clip_cull_requested = 0;
                flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
                flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
                flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                /* @hack type: each coordinate makes a round trip through Float,
                 * which changes no value but has SHC convert it with FLOAT and
                 * FTRC (0x8c630aca to 0x8c630b1a). without the casts, the
                 * section runs 52 bytes short and 146 of 573 words differ */
                corner_uv_window[0].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[0].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[1].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[1].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[2].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[2].tex.v = (Sint16)(Float)corner_uv->tex.v;
                simple_clip_flat_tex_triangle(v0, v1, v2, corner_uv_window,
                                              flat_intensity);
                run_start = (Uint8 *)sq_cursor;
                corner_uv--;
            }
            v0 = v1;
            x0 = x1;
            v1 = v2;
            y0 = y1;
            x1 = v2->sx;
            winding = -winding;
            y1 = v2->sy;
        } while (--remaining != 0);
        corner_uv += 2;
        if ((Uint32)run_start < (Uint32)sq_cursor) {
            corner_uv -= 2;
            _NJM_SET_TEX_STRIP(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                               (NJS_TEX *)(corner_uv++));
        }
    } while (--strips_left);
    _nj_calc_polygon_ += input_corners;
    _nj_draw_polygon_ += input_corners;
}

static void simple_draw_unlit_tex_strips_near(NJS_MESHSET *meshset)
{
    register Sint16 *mesh_cursor;
    register NJS_COLOR *corner_uv;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register Uint8 *run_start;
    struct calc_vertex *vertices;
    Sint32 remaining;
    Sint32 input_corners;
    Sint32 strips_left;
    Sint32 needs_clip;
    Uint32 outside_all;
    Uint32 crosses_near;
    register Float winding;
    register Float x0;
    register Float y0;
    register Float x1;
    register Float y1;
    register Float facing;

    DRAW_NOOP();
    corner_uv = meshset->vertuv;
    mesh_cursor = meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct calc_vertex *)_nj_vertex_buf_;
    input_corners = 0;
/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c630c34, the pool at 0x8c630c38). without it, those literals join later
 * pools: 481 of 505 words differ and the function is 4 bytes shorter */
#pragma asm
    ;
#pragma endasm
    do {
        remaining = (Uint16)*mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        run_start = (Uint8 *)sq_cursor;
        input_corners += remaining;
        remaining -= 2;
        v0 = &vertices[(Uint16)*mesh_cursor++];
        v1 = &vertices[(Uint16)*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        do {
            v2 = &vertices[(Uint16)*mesh_cursor++];
            prefetch(&vertices[(Uint16)*mesh_cursor]);
            needs_clip = 0;
            facing = 1.0F;
            outside_all = v0->clip & v1->clip & v2->clip & CLIP_ALL;
            if (outside_all) {
                goto submit_run;
            }
            crosses_near = (v0->clip | v1->clip | v2->clip) & CLIP_NEAR;
            if (crosses_near) {
                struct calc_vertex *triangle[3];
                NJS_POINT3 view_points[3];
                Float view_z;
                int corner;

                triangle[0] = v0;
                triangle[1] = v1;
                triangle[2] = v2;
                for (corner = 0; corner < 3; corner++) {
                    DECODE_VIEW_POINT(triangle[corner], view_points[corner],
                                      view_z);
                }
                facing = (view_points[2].x *
                          ((view_points[0].z * view_points[1].y) -
                           (view_points[0].y * view_points[1].z))) +
                         (view_points[2].y *
                          ((view_points[0].x * view_points[1].z) -
                           (view_points[0].z * view_points[1].x))) +
                         (view_points[2].z *
                          ((view_points[1].x * view_points[0].y) -
                           (view_points[0].x * view_points[1].y)));
                facing *= -winding;
                if (!(facing > 0.0F)) {
                    needs_clip = 1;
                    facing = 1.0F;
                }
            } else {
                facing = (((x1 - x0) * (v2->sy - y1)) -
                          ((y1 - y0) * (v2->sx - x1))) *
                         winding;
            }
        submit_run:
            if ((Uint32)run_start < (Uint32)sq_cursor) {
                if (facing < 0.0F) {
                    _NJM_SET_TEX_X_STRIP(v1, KM_VERTEXPARAM_NORMAL,
                                         (NJS_TEX *)(corner_uv++));
                } else {
                    _NJM_SET_TEX_X_STRIP(v1, KM_VERTEXPARAM_ENDOFSTRIP,
                                         (NJS_TEX *)(corner_uv++));
                    corner_uv--;
                    run_start = (Uint8 *)sq_cursor;
                }
            } else {
                corner_uv++;
                if (facing < 0.0F) {
                    corner_uv--;
                    _NJM_SET_TEX_X_STRIP(v0, KM_VERTEXPARAM_NORMAL,
                                         (NJS_TEX *)(corner_uv++));
                    _NJM_SET_TEX_X_STRIP(v1, KM_VERTEXPARAM_NORMAL,
                                         (NJS_TEX *)(corner_uv++));
                }
            }
            if (needs_clip) {
                corner_uv--;
                clip_cull_requested = 0;
                /* @hack type: each coordinate makes a round trip through Float,
                 * which changes no value but has SHC convert it with FLOAT and
                 * FTRC (0x8c630f10 to 0x8c630f66). without the casts, the
                 * section runs 52 bytes short and 138 of 505 words differ */
                corner_uv_window[0].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[0].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[1].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[1].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[2].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[2].tex.v = (Sint16)(Float)corner_uv->tex.v;
                simple_clip_unlit_tex_triangle(v0, v1, v2, corner_uv_window);
                corner_uv--;
                run_start = (Uint8 *)sq_cursor;
            }
            v0 = v1;
            x0 = x1;
            v1 = v2;
            y0 = y1;
            x1 = v2->sx;
            winding = -winding;
            y1 = v2->sy;
        } while (--remaining != 0);
        corner_uv += 2;
        if ((Uint32)run_start < (Uint32)sq_cursor) {
            corner_uv -= 2;
            _NJM_SET_TEX_X_STRIP(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                                 (NJS_TEX *)(corner_uv++));
        }
    } while (--strips_left);
    _nj_calc_polygon_ += input_corners;
    _nj_draw_polygon_ += input_corners;
}

static void simple_draw_flat_path_env_strips_near(NJS_MESHSET *meshset)
{
    register struct calc_vertex *vertices;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register Uint16 *mesh_cursor;
    register Float *run_start;
    register Float x0;
    register Float y0;
    register Float x1;
    register Float y1;
    register Float winding;
    register Float facing;
    Uint32 outside_all;
    Uint32 crosses_near;
    Uint32 input_corners;
    Uint32 remaining;
    Sint32 strips_left;
    Sint32 needs_clip;

    DRAW_NOOP();
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct calc_vertex *)_nj_vertex_buf_;
    input_corners = 0;
/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c631048, the pool at 0x8c63104c). without it, SHC writes
 * simple_draw_unlit_tex_strips_near's literals behind that function's RTS,
 * making it 28 bytes longer, and this function's in a later pool: 434 of 461
 * words differ and the function is 32 bytes shorter */
#pragma asm
    ;
#pragma endasm
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        run_start = sq_cursor;
        input_corners += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        do {
            v2 = &vertices[*mesh_cursor++];
            prefetch(&vertices[*mesh_cursor]);
            needs_clip = 0;
            facing = 1.0F;
            outside_all = v0->clip & v1->clip & v2->clip & CLIP_ALL;
            if (outside_all) {
                goto submit_run;
            }
            crosses_near = (v0->clip | v1->clip | v2->clip) & CLIP_NEAR;
            if (crosses_near) {
                struct calc_vertex *triangle[3];
                NJS_POINT3 view_points[3];
                Float view_z;
                int corner;

                triangle[0] = v0;
                triangle[1] = v1;
                triangle[2] = v2;
                for (corner = 0; corner < 3; corner++) {
                    DECODE_VIEW_POINT(triangle[corner], view_points[corner],
                                      view_z);
                }
                facing = (view_points[2].x *
                          ((view_points[0].z * view_points[1].y) -
                           (view_points[0].y * view_points[1].z))) +
                         (view_points[2].y *
                          ((view_points[0].x * view_points[1].z) -
                           (view_points[0].z * view_points[1].x))) +
                         (view_points[2].z *
                          ((view_points[1].x * view_points[0].y) -
                           (view_points[0].x * view_points[1].y)));
                facing *= -winding;
                if (!(facing > 0.0F)) {
                    needs_clip = 1;
                    facing = 1.0F;
                }
            } else {
                facing = (((x1 - x0) * (v2->sy - y1)) -
                          ((y1 - y0) * (v2->sx - x1))) *
                         winding;
            }
        submit_run:
            if ((Uint32)run_start < (Uint32)sq_cursor) {
                if (facing < 0.0F) {
                    NJM_SET_ENV_STRIP(v1, KM_VERTEXPARAM_NORMAL);
                } else {
                    NJM_SET_ENV_STRIP(v1, KM_VERTEXPARAM_ENDOFSTRIP);
                    run_start = sq_cursor;
                }
            } else if (facing < 0.0F) {
                NJM_SET_ENV_STRIP(v0, KM_VERTEXPARAM_NORMAL);
                NJM_SET_ENV_STRIP(v1, KM_VERTEXPARAM_NORMAL);
            }
            if (needs_clip) {
                NJS_VECTOR flat_normal;
                Float flat_intensity;

                clip_cull_requested = 0;
                flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
                flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
                flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                simple_clip_flat_env_triangle(v0, v1, v2, flat_intensity);
                run_start = sq_cursor;
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            winding = -winding;
            y1 = v2->sy;
        } while (--remaining != 0);
        if ((Uint32)run_start < (Uint32)sq_cursor) {
            NJM_SET_ENV_STRIP(v2, KM_VERTEXPARAM_ENDOFSTRIP);
        }
    } while (--strips_left != 0);
    _nj_calc_polygon_ += input_corners;
    _nj_draw_polygon_ += input_corners;
}

static void simple_draw_unlit_env_strips_near(NJS_MESHSET *meshset)
{
    register struct calc_vertex *vertices;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register Uint16 *mesh_cursor;
    register Float *run_start;
    register Float x0;
    register Float y0;
    register Float x1;
    register Float y1;
    register Float winding;
    register Float facing;
    Uint32 outside_all;
    Uint32 crosses_near;
    Uint32 input_corners;
    Uint32 remaining;
    Sint32 strips_left;
    Sint32 needs_clip;

    DRAW_NOOP();
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct calc_vertex *)_nj_vertex_buf_;
    input_corners = 0;
/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c631420, the pool at 0x8c631424). without it, SHC writes
 * simple_draw_flat_path_env_strips_near's literals behind that function's RTS,
 * making it 36 bytes longer, and this function's in a later pool: 392 of 417
 * words differ and the function is 40 bytes shorter */
#pragma asm
    ;
#pragma endasm
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        run_start = sq_cursor;
        input_corners += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        do {
            v2 = &vertices[*mesh_cursor++];
            prefetch(&vertices[*mesh_cursor]);
            needs_clip = 0;
            facing = 1.0F;
            outside_all = v0->clip & v1->clip & v2->clip & CLIP_ALL;
            if (outside_all) {
                goto submit_run;
            }
            crosses_near = (v0->clip | v1->clip | v2->clip) & CLIP_NEAR;
            if (crosses_near) {
                struct calc_vertex *triangle[3];
                NJS_POINT3 view_points[3];
                Float view_z;
                int corner;

                triangle[0] = v0;
                triangle[1] = v1;
                triangle[2] = v2;
                for (corner = 0; corner < 3; corner++) {
                    DECODE_VIEW_POINT(triangle[corner], view_points[corner],
                                      view_z);
                }
                facing = (view_points[2].x *
                          ((view_points[0].z * view_points[1].y) -
                           (view_points[0].y * view_points[1].z))) +
                         (view_points[2].y *
                          ((view_points[0].x * view_points[1].z) -
                           (view_points[0].z * view_points[1].x))) +
                         (view_points[2].z *
                          ((view_points[1].x * view_points[0].y) -
                           (view_points[0].x * view_points[1].y)));
                facing *= -winding;
                if (!(facing > 0.0F)) {
                    needs_clip = 1;
                    facing = 1.0F;
                }
            } else {
                facing = (((x1 - x0) * (v2->sy - y1)) -
                          ((y1 - y0) * (v2->sx - x1))) *
                         winding;
            }
        submit_run:
            if ((Uint32)run_start < (Uint32)sq_cursor) {
                if (facing < 0.0F) {
                    NJM_SET_ENV_X_STRIP(v1, KM_VERTEXPARAM_NORMAL);
                } else {
                    NJM_SET_ENV_X_STRIP(v1, KM_VERTEXPARAM_ENDOFSTRIP);
                    run_start = sq_cursor;
                }
            } else if (facing < 0.0F) {
                NJM_SET_ENV_X_STRIP(v0, KM_VERTEXPARAM_NORMAL);
                NJM_SET_ENV_X_STRIP(v1, KM_VERTEXPARAM_NORMAL);
            }
            if (needs_clip) {
                clip_cull_requested = 0;
                simple_clip_unlit_env_triangle(v0, v1, v2);
                run_start = sq_cursor;
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            winding = -winding;
            y1 = v2->sy;
        } while (--remaining != 0);
        if ((Uint32)run_start < (Uint32)sq_cursor) {
            NJM_SET_ENV_X_STRIP(v2, KM_VERTEXPARAM_ENDOFSTRIP);
        }
    } while (--strips_left != 0);
    _nj_calc_polygon_ += input_corners;
    _nj_draw_polygon_ += input_corners;
}

static void simple_draw_unlit_strips_near(NJS_MESHSET *meshset)
{
    register struct calc_vertex *vertices;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register Uint16 *mesh_cursor;
    register Float *run_start;
    register Float x0;
    register Float y0;
    register Float x1;
    register Float y1;
    register Float winding;
    register Float facing;
    Uint32 outside_all;
    Uint32 crosses_near;
    Uint32 input_corners;
    Uint32 remaining;
    Sint32 strips_left;
    Sint32 needs_clip;

    DRAW_NOOP();
    mesh_cursor = (Uint16 *)meshset->meshes;
    strips_left = meshset->nbMesh;
    vertices = (struct calc_vertex *)_nj_vertex_buf_;
    input_corners = 0;
/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c6317a8, the pool at 0x8c6317ac). without it, SHC writes
 * simple_draw_unlit_env_strips_near's literals behind that function's RTS,
 * making it 24 bytes longer, and this function's in a later pool: 377 of 401
 * words differ and the function is 28 bytes shorter */
#pragma asm
    ;
#pragma endasm
    do {
        remaining = *mesh_cursor++;
        winding = 1.0F;
        if (remaining & NJD_TRIMESH_END) {
            winding = -1.0F;
            remaining &= STRIP_COUNT;
        }
        run_start = sq_cursor;
        input_corners += remaining;
        remaining -= 2;
        v0 = &vertices[*mesh_cursor++];
        v1 = &vertices[*mesh_cursor++];
        x0 = v0->sx;
        x1 = v1->sx;
        y0 = v0->sy;
        y1 = v1->sy;
        do {
            v2 = &vertices[*mesh_cursor++];
            prefetch(&vertices[*mesh_cursor]);
            needs_clip = 0;
            facing = 1.0F;
            outside_all = v0->clip & v1->clip & v2->clip & CLIP_ALL;
            if (outside_all) {
                goto submit_run;
            }
            crosses_near = (v0->clip | v1->clip | v2->clip) & CLIP_NEAR;
            if (crosses_near) {
                struct calc_vertex *triangle[3];
                NJS_POINT3 view_points[3];
                Float view_z;
                int corner;

                triangle[0] = v0;
                triangle[1] = v1;
                triangle[2] = v2;
                for (corner = 0; corner < 3; corner++) {
                    DECODE_VIEW_POINT(triangle[corner], view_points[corner],
                                      view_z);
                }
                facing = (view_points[2].x *
                          ((view_points[0].z * view_points[1].y) -
                           (view_points[0].y * view_points[1].z))) +
                         (view_points[2].y *
                          ((view_points[0].x * view_points[1].z) -
                           (view_points[0].z * view_points[1].x))) +
                         (view_points[2].z *
                          ((view_points[1].x * view_points[0].y) -
                           (view_points[0].x * view_points[1].y)));
                facing *= -winding;
                if (!(facing > 0.0F)) {
                    needs_clip = 1;
                    facing = 1.0F;
                }
            } else {
                facing = (((x1 - x0) * (v2->sy - y1)) -
                          ((y1 - y0) * (v2->sx - x1))) *
                         winding;
            }
        submit_run:
            if ((Uint32)run_start < (Uint32)sq_cursor) {
                if (facing < 0.0F) {
                    NJM_SET_POLY_X_STRIP(v1, KM_VERTEXPARAM_NORMAL);
                } else {
                    NJM_SET_POLY_X_STRIP(v1, KM_VERTEXPARAM_ENDOFSTRIP);
                    run_start = sq_cursor;
                }
            } else if (facing < 0.0F) {
                NJM_SET_POLY_X_STRIP(v0, KM_VERTEXPARAM_NORMAL);
                NJM_SET_POLY_X_STRIP(v1, KM_VERTEXPARAM_NORMAL);
            }
            if (needs_clip) {
                clip_cull_requested = 0;
                simple_clip_unlit_triangle(v0, v1, v2);
                run_start = sq_cursor;
            }
            v0 = v1;
            v1 = v2;
            x0 = x1;
            y0 = y1;
            x1 = v2->sx;
            winding = -winding;
            y1 = v2->sy;
        } while (--remaining != 0);
        if ((Uint32)run_start < (Uint32)sq_cursor) {
            NJM_SET_POLY_X_STRIP(v2, KM_VERTEXPARAM_ENDOFSTRIP);
        }
    } while (--strips_left != 0);
    _nj_calc_polygon_ += input_corners;
    _nj_draw_polygon_ += input_corners;
}

/* the flat emitters. a _POSITION emitter leaves a vertex's intensity or color
 * words as the store queue held them: under a flat color mode a triangle takes
 * them from the vertex that completes it, which an _INTENSITY or _COLOR emitter
 * writes */
#define EMIT_FLAT_TEX_POSITION(vertex, texcoord, command)                      \
    {                                                                          \
        sq_cursor += 6;                                                        \
        *--sq_cursor = (Float)(texcoord)->v * TEX_SCALE;                       \
        *--sq_cursor = (Float)(texcoord)->u * TEX_SCALE;                       \
        *--sq_cursor = (vertex)->oow;                                          \
        *--sq_cursor = (vertex)->sy;                                           \
        *--sq_cursor = (vertex)->sx;                                           \
        *(Uint32 *)--sq_cursor = command;                                      \
        prefetch(sq_cursor);                                                   \
        sq_cursor += VERTEX_WORDS;                                             \
    }

#define EMIT_FLAT_TEX_POSITION_NEXT(vertex, texcoord, command)                 \
    {                                                                          \
        sq_cursor += 6;                                                        \
        *--sq_cursor = (Float)(texcoord)->tex.v * TEX_SCALE;                   \
        *--sq_cursor = (Float)(texcoord)->tex.u * TEX_SCALE;                   \
        (texcoord)++;                                                          \
        *--sq_cursor = (vertex)->oow;                                          \
        *--sq_cursor = (vertex)->sy;                                           \
        *--sq_cursor = (vertex)->sx;                                           \
        *(Uint32 *)--sq_cursor = command;                                      \
        prefetch(sq_cursor);                                                   \
        sq_cursor += VERTEX_WORDS;                                             \
    }

#define EMIT_FLAT_TEX_INTENSITY(vertex, texcoord, command, intensity)          \
    {                                                                          \
        sq_cursor += 7;                                                        \
        *--sq_cursor = (intensity);                                            \
        *--sq_cursor = (Float)(texcoord)->v * TEX_SCALE;                       \
        *--sq_cursor = (Float)(texcoord)->u * TEX_SCALE;                       \
        *--sq_cursor = (vertex)->oow;                                          \
        *--sq_cursor = (vertex)->sy;                                           \
        *--sq_cursor = (vertex)->sx;                                           \
        *(Uint32 *)--sq_cursor = command;                                      \
        prefetch(sq_cursor);                                                   \
        sq_cursor += VERTEX_WORDS;                                             \
    }

#define EMIT_FLAT_TEX_INTENSITY_NEXT(vertex, texcoord, command, intensity)     \
    {                                                                          \
        sq_cursor += 7;                                                        \
        *--sq_cursor = (intensity);                                            \
        *--sq_cursor = (Float)(texcoord)->tex.v * TEX_SCALE;                   \
        *--sq_cursor = (Float)(texcoord)->tex.u * TEX_SCALE;                   \
        (texcoord)++;                                                          \
        *--sq_cursor = (vertex)->oow;                                          \
        *--sq_cursor = (vertex)->sy;                                           \
        *--sq_cursor = (vertex)->sx;                                           \
        *(Uint32 *)--sq_cursor = command;                                      \
        prefetch(sq_cursor);                                                   \
        sq_cursor += VERTEX_WORDS;                                             \
    }

static void simple_clip_flat_tex_triangle(struct calc_vertex *v0,
                                          struct calc_vertex *v1,
                                          struct calc_vertex *v2,
                                          NJS_COLOR *corner_uv, Float intensity)
{
    struct calc_vertex *vertices[3];
    NJS_POINT3 view_points[3];
    Float view_x;
    Float view_y;
    Float view_z;
    Float near_reciprocal;
    Float edge_t;
    Float view_facing;
    int corner;
    int previous;
    int output_count;

    near_reciprocal = 1.0F / _nj_near_clip_;
    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    for (corner = 0; corner < 3; corner++) {
        DECODE_VIEW_POINT(vertices[corner], view_points[corner], view_z);
    }
    view_facing = (view_points[2].x * ((view_points[0].z * view_points[1].y) -
                                       (view_points[0].y * view_points[1].z))) +
                  (view_points[2].y * ((view_points[0].x * view_points[1].z) -
                                       (view_points[0].z * view_points[1].x))) +
                  (view_points[2].z * ((view_points[1].x * view_points[0].y) -
                                       (view_points[0].x * view_points[1].y)));
    if ((view_facing < 0.0F) & clip_cull_requested) {
        return;
    }
    clip_cull_requested = 0;
    output_count = 0;
    previous = 2;
    for (corner = 0; corner < 3; corner++) {
        if (((vertices[previous]->clip & CLIP_NEAR) ^
             (vertices[corner]->clip & CLIP_NEAR)) == 1) {
            edge_t = (_nj_near_clip_ - view_points[previous].z) /
                     (view_points[corner].z - view_points[previous].z);
            view_x =
                view_points[previous].x +
                ((view_points[corner].x - view_points[previous].x) * edge_t);
            clipped_vertices[output_count].sx =
                (projection_x * view_x * near_reciprocal) + _nj_screen_.cx;
            view_y =
                view_points[previous].y +
                ((view_points[corner].y - view_points[previous].y) * edge_t);
            clipped_vertices[output_count].sy =
                (projection_y * view_y * near_reciprocal) + _nj_screen_.cy;
            clipped_vertices[output_count].oow = near_reciprocal;
            clipped_uv[output_count].u =
                corner_uv[previous].tex.u +
                ((corner_uv[corner].tex.u - corner_uv[previous].tex.u) *
                 edge_t);
            clipped_uv[output_count].v =
                corner_uv[previous].tex.v +
                ((corner_uv[corner].tex.v - corner_uv[previous].tex.v) *
                 edge_t);
            output_count++;
        }
        if (!(vertices[corner]->clip & CLIP_NEAR)) {
            clipped_vertices[output_count].sx = vertices[corner]->sx;
            clipped_vertices[output_count].sy = vertices[corner]->sy;
            clipped_vertices[output_count].oow = vertices[corner]->oow;
            clipped_uv[output_count].u = corner_uv[corner].tex.u;
            clipped_uv[output_count].v = corner_uv[corner].tex.v;
            output_count++;
        }
        previous = corner;
    }
    if (output_count == 3) {
        EMIT_FLAT_TEX_POSITION(&clipped_vertices[0], &clipped_uv[0],
                               KM_VERTEXPARAM_NORMAL);
        EMIT_FLAT_TEX_POSITION(&clipped_vertices[1], &clipped_uv[1],
                               KM_VERTEXPARAM_NORMAL);
        EMIT_FLAT_TEX_INTENSITY(&clipped_vertices[2], &clipped_uv[2],
                                KM_VERTEXPARAM_ENDOFSTRIP, intensity);
        _nj_draw_polygon_ += 3;
    } else {
        EMIT_FLAT_TEX_POSITION(&clipped_vertices[0], &clipped_uv[0],
                               KM_VERTEXPARAM_NORMAL);
        EMIT_FLAT_TEX_POSITION(&clipped_vertices[1], &clipped_uv[1],
                               KM_VERTEXPARAM_NORMAL);
        EMIT_FLAT_TEX_INTENSITY(&clipped_vertices[3], &clipped_uv[3],
                                KM_VERTEXPARAM_NORMAL, intensity);
        EMIT_FLAT_TEX_INTENSITY(&clipped_vertices[2], &clipped_uv[2],
                                KM_VERTEXPARAM_ENDOFSTRIP, intensity);
        _nj_draw_polygon_ += 4;
    }
}

static void simple_clip_flat_env_triangle(struct calc_vertex *v0,
                                          struct calc_vertex *v1,
                                          struct calc_vertex *v2,
                                          Float intensity)
{
    struct calc_vertex *vertices[3];
    NJS_POINT3 view_points[3];
    NJS_TEX quantized_uv[3];
    struct calc_vertex *vertex;
    Float view_x;
    Float view_y;
    Float view_z;
    Float near_reciprocal;
    Float edge_t;
    Float view_facing;
    int corner;
    int previous;
    int output_count;

    near_reciprocal = 1.0F / _nj_near_clip_;
    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    for (corner = 0; corner < 3; corner++) {
        vertex = vertices[corner];
        quantized_uv[corner].u =
            (Sint16)((ENV_COLUMN_DOT(vertex->normal, 0) * 128.0F) + 128.0F);
        quantized_uv[corner].v =
            (Sint16)((ENV_COLUMN_DOT(vertex->normal, 1) * 128.0F) + 128.0F);
        DECODE_VIEW_POINT(vertices[corner], view_points[corner], view_z);
    }
    view_facing = (view_points[2].x * ((view_points[0].z * view_points[1].y) -
                                       (view_points[0].y * view_points[1].z))) +
                  (view_points[2].y * ((view_points[0].x * view_points[1].z) -
                                       (view_points[0].z * view_points[1].x))) +
                  (view_points[2].z * ((view_points[1].x * view_points[0].y) -
                                       (view_points[0].x * view_points[1].y)));
    if ((view_facing < 0.0F) & clip_cull_requested) {
        return;
    }
    clip_cull_requested = 0;
    output_count = 0;
    previous = 2;
    for (corner = 0; corner < 3; corner++) {
        if (((vertices[previous]->clip & CLIP_NEAR) ^
             (vertices[corner]->clip & CLIP_NEAR)) == 1) {
            edge_t = (_nj_near_clip_ - view_points[previous].z) /
                     (view_points[corner].z - view_points[previous].z);
            view_x =
                view_points[previous].x +
                ((view_points[corner].x - view_points[previous].x) * edge_t);
            clipped_vertices[output_count].sx =
                (projection_x * view_x * near_reciprocal) + _nj_screen_.cx;
            view_y =
                view_points[previous].y +
                ((view_points[corner].y - view_points[previous].y) * edge_t);
            clipped_vertices[output_count].sy =
                (projection_y * view_y * near_reciprocal) + _nj_screen_.cy;
            clipped_vertices[output_count].oow = near_reciprocal;
            clipped_uv[output_count].u =
                quantized_uv[previous].u +
                ((quantized_uv[corner].u - quantized_uv[previous].u) * edge_t);
            clipped_uv[output_count].v =
                quantized_uv[previous].v +
                ((quantized_uv[corner].v - quantized_uv[previous].v) * edge_t);
            output_count++;
        }
        if (!(vertices[corner]->clip & CLIP_NEAR)) {
            clipped_vertices[output_count].sx = vertices[corner]->sx;
            clipped_vertices[output_count].sy = vertices[corner]->sy;
            clipped_vertices[output_count].oow = vertices[corner]->oow;
            clipped_uv[output_count].u = quantized_uv[corner].u;
            clipped_uv[output_count].v = quantized_uv[corner].v;
            output_count++;
        }
        previous = corner;
    }
    if (output_count == 3) {
        EMIT_FLAT_TEX_POSITION(&clipped_vertices[0], &clipped_uv[0],
                               KM_VERTEXPARAM_NORMAL);
        EMIT_FLAT_TEX_POSITION(&clipped_vertices[1], &clipped_uv[1],
                               KM_VERTEXPARAM_NORMAL);
        EMIT_FLAT_TEX_INTENSITY(&clipped_vertices[2], &clipped_uv[2],
                                KM_VERTEXPARAM_ENDOFSTRIP, intensity);
        _nj_draw_polygon_ += 3;
    } else {
        EMIT_FLAT_TEX_POSITION(&clipped_vertices[0], &clipped_uv[0],
                               KM_VERTEXPARAM_NORMAL);
        EMIT_FLAT_TEX_POSITION(&clipped_vertices[1], &clipped_uv[1],
                               KM_VERTEXPARAM_NORMAL);
        EMIT_FLAT_TEX_INTENSITY(&clipped_vertices[3], &clipped_uv[3],
                                KM_VERTEXPARAM_NORMAL, intensity);
        EMIT_FLAT_TEX_INTENSITY(&clipped_vertices[2], &clipped_uv[2],
                                KM_VERTEXPARAM_ENDOFSTRIP, intensity);
        _nj_draw_polygon_ += 4;
    }
}

#define EMIT_FLAT_POSITION(vertex, command)                                    \
    {                                                                          \
        sq_cursor += 4;                                                        \
        *--sq_cursor = (vertex)->oow;                                          \
        *--sq_cursor = (vertex)->sy;                                           \
        *--sq_cursor = (vertex)->sx;                                           \
        *(Uint32 *)--sq_cursor = command;                                      \
        prefetch(sq_cursor);                                                   \
        sq_cursor += VERTEX_WORDS;                                             \
    }

#define EMIT_FLAT_COLOR(vertex, command, color)                                \
    {                                                                          \
        sq_cursor += VERTEX_WORDS;                                             \
        *--sq_cursor = (color).b;                                              \
        *--sq_cursor = (color).g;                                              \
        *--sq_cursor = (color).r;                                              \
        *--sq_cursor = (color).a;                                              \
        *--sq_cursor = (vertex)->oow;                                          \
        *--sq_cursor = (vertex)->sy;                                           \
        *--sq_cursor = (vertex)->sx;                                           \
        *(Uint32 *)--sq_cursor = command;                                      \
        prefetch(sq_cursor);                                                   \
        sq_cursor += VERTEX_WORDS;                                             \
    }

static void simple_clip_constant_color_triangle(struct calc_vertex *v0,
                                                struct calc_vertex *v1,
                                                struct calc_vertex *v2,
                                                NJS_ARGB *color)
{
    struct calc_vertex *vertices[3];
    NJS_POINT3 view_points[3];
    Float view_x;
    Float view_y;
    Float view_z;
    Float near_reciprocal;
    Float edge_t;
    Float view_facing;
    int corner;
    int previous;
    int output_count;

    near_reciprocal = 1.0F / _nj_near_clip_;
    vertices[0] = v0;
    vertices[1] = v1;
    vertices[2] = v2;
    for (corner = 0; corner < 3; corner++) {
        DECODE_VIEW_POINT(vertices[corner], view_points[corner], view_z);
    }
    view_facing = (view_points[2].x * ((view_points[0].z * view_points[1].y) -
                                       (view_points[0].y * view_points[1].z))) +
                  (view_points[2].y * ((view_points[0].x * view_points[1].z) -
                                       (view_points[0].z * view_points[1].x))) +
                  (view_points[2].z * ((view_points[1].x * view_points[0].y) -
                                       (view_points[0].x * view_points[1].y)));
    if ((view_facing < 0.0F) & clip_cull_requested) {
        return;
    }
    clip_cull_requested = 0;
    output_count = 0;
    previous = 2;
    for (corner = 0; corner < 3; corner++) {
        if (((vertices[previous]->clip & CLIP_NEAR) ^
             (vertices[corner]->clip & CLIP_NEAR)) == 1) {
            edge_t = (_nj_near_clip_ - view_points[previous].z) /
                     (view_points[corner].z - view_points[previous].z);
            view_x =
                view_points[previous].x +
                ((view_points[corner].x - view_points[previous].x) * edge_t);
            clipped_vertices[output_count].sx =
                (projection_x * view_x * near_reciprocal) + _nj_screen_.cx;
            view_y =
                view_points[previous].y +
                ((view_points[corner].y - view_points[previous].y) * edge_t);
            clipped_vertices[output_count].sy =
                (projection_y * view_y * near_reciprocal) + _nj_screen_.cy;
            clipped_vertices[output_count].oow = near_reciprocal;
            output_count++;
        }
        if (!(vertices[corner]->clip & CLIP_NEAR)) {
            clipped_vertices[output_count].sx = vertices[corner]->sx;
            clipped_vertices[output_count].sy = vertices[corner]->sy;
            clipped_vertices[output_count].oow = vertices[corner]->oow;
            output_count++;
        }
        previous = corner;
    }
    if (output_count == 3) {
        EMIT_FLAT_POSITION(&clipped_vertices[0], KM_VERTEXPARAM_NORMAL);
        EMIT_FLAT_POSITION(&clipped_vertices[1], KM_VERTEXPARAM_NORMAL);
        EMIT_FLAT_COLOR(&clipped_vertices[2], KM_VERTEXPARAM_ENDOFSTRIP,
                        *color);
        _nj_draw_polygon_ += 3;
    } else {
        EMIT_FLAT_POSITION(&clipped_vertices[0], KM_VERTEXPARAM_NORMAL);
        EMIT_FLAT_POSITION(&clipped_vertices[1], KM_VERTEXPARAM_NORMAL);
        EMIT_FLAT_COLOR(&clipped_vertices[3], KM_VERTEXPARAM_NORMAL, *color);
        EMIT_FLAT_COLOR(&clipped_vertices[2], KM_VERTEXPARAM_ENDOFSTRIP,
                        *color);
        _nj_draw_polygon_ += 4;
    }
}

/* set base_light_direction from _nj_light_control_'s first light,
 * base_light_scale to its diffuse intensity and iamb to its ambient one. then
 * draw model as njSimpleDrawModelSub does */
void _njSimpleDrawModel(NJS_MODEL *model)
{
    NJS_MATRIX combined;
    NJS_LIGHT *base_light;
    Float *rotation;
    Float *light_matrix;
    Float *view_matrix;

    base_light = _nj_light_control_.light[0];
    rotation = combined;
    view_matrix = _nj_view_matrix_;
    light_matrix = base_light->mtrx;
    rotation[0] = (light_matrix[0] * view_matrix[0]) +
                  (light_matrix[1] * view_matrix[4]) +
                  (light_matrix[2] * view_matrix[8]);
    rotation[1] = (light_matrix[0] * view_matrix[1]) +
                  (light_matrix[1] * view_matrix[5]) +
                  (light_matrix[2] * view_matrix[9]);
    rotation[2] = (light_matrix[0] * view_matrix[2]) +
                  (light_matrix[1] * view_matrix[6]) +
                  (light_matrix[2] * view_matrix[10]);
    rotation[4] = (light_matrix[4] * view_matrix[0]) +
                  (light_matrix[5] * view_matrix[4]) +
                  (light_matrix[6] * view_matrix[8]);
    rotation[5] = (light_matrix[4] * view_matrix[1]) +
                  (light_matrix[5] * view_matrix[5]) +
                  (light_matrix[6] * view_matrix[9]);
    rotation[6] = (light_matrix[4] * view_matrix[2]) +
                  (light_matrix[5] * view_matrix[6]) +
                  (light_matrix[6] * view_matrix[10]);
    rotation[8] = (light_matrix[8] * view_matrix[0]) +
                  (light_matrix[9] * view_matrix[4]) +
                  (light_matrix[10] * view_matrix[8]);
    rotation[9] = (light_matrix[8] * view_matrix[1]) +
                  (light_matrix[9] * view_matrix[5]) +
                  (light_matrix[10] * view_matrix[9]);
    rotation[10] = (light_matrix[8] * view_matrix[2]) +
                   (light_matrix[9] * view_matrix[6]) +
                   (light_matrix[10] * view_matrix[10]);
    base_light_direction.x = (rotation[0] * base_light->vctr.x) +
                             (rotation[4] * base_light->vctr.y) +
                             (rotation[8] * base_light->vctr.z);
    base_light_direction.y = (rotation[1] * base_light->vctr.x) +
                             (rotation[5] * base_light->vctr.y) +
                             (rotation[9] * base_light->vctr.z);
    base_light_direction.z = (rotation[2] * base_light->vctr.x) +
                             (rotation[6] * base_light->vctr.y) +
                             (rotation[10] * base_light->vctr.z);
    base_light_scale = base_light->attr.idif;
    iamb = base_light->attr.iamb;
    njSimpleDrawModelSub(model);
}

#define WORK_VERTEX(index)                                                     \
    (&((struct calc_vertex *)_nj_vertex_buf_)[(Uint16)(index)])

#define EMIT_SMOOTH_POLYGON(vertex, command)                                   \
    {                                                                          \
        (vertex)->intensity = SIMPLE_INTENSITY((vertex)->normal);              \
        EMIT_COLOR_INTENSITY(vertex, command)                                  \
    }

/* the polygon drawers send each of meshset's triangles, quads or N-gons as a
 * strip of its corners, unless its first three corners face away. a strip
 * meshset goes to their strip drawer, between their kind's START and END. they
 * add every corner to _nj_calc_polygon_, and each corner they send to
 * _nj_draw_polygon_. material goes unread.
 *
 * the _clip drawers also drop a polygon whose corners all share a clip flag. a
 * polygon with a corner nearer than the near plane goes to a clipper instead,
 * as triangles: a quad's 0, 1, 2 and 1, 2, 3, and an N-gon's in strip order,
 * but not one whose corners are all nearer. a triangle goes with
 * clip_cull_requested set, and so does a quad's first. a quad's second goes
 * with the flag set unless the clipper sent the first. an N-gon's triangles go
 * with the flag clear */
static void simple_draw_smooth_polygons(NJS_MESHSET *meshset,
                                        NJS_MATERIAL *material)
{
    register Sint16 *mesh_cursor;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register struct calc_vertex *v3;
    Sint32 polygon;
    Sint32 corner;
    Sint32 corner_count;
    register Float facing;

    mesh_cursor = meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));

            if (facing > 0.0F) {
                continue;
            }
            EMIT_SMOOTH_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
            EMIT_SMOOTH_POLYGON(v1, KM_VERTEXPARAM_NORMAL);
            EMIT_SMOOTH_POLYGON(v2, KM_VERTEXPARAM_ENDOFSTRIP);
            _nj_draw_polygon_ += 3;
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                continue;
            }
            EMIT_SMOOTH_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
            EMIT_SMOOTH_POLYGON(v1, KM_VERTEXPARAM_NORMAL);
            EMIT_SMOOTH_POLYGON(v2, KM_VERTEXPARAM_NORMAL);
            EMIT_SMOOTH_POLYGON(v3, KM_VERTEXPARAM_ENDOFSTRIP);
            _nj_draw_polygon_ += 4;
        }
        break;
    case NJD_MESHSET_N:
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            corner_count = (Uint16)*mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            v0 = WORK_VERTEX(mesh_cursor[0]);
            v1 = WORK_VERTEX(mesh_cursor[1]);
            v2 = WORK_VERTEX(mesh_cursor[2]);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                mesh_cursor += corner_count;
                continue;
            }
            _nj_draw_polygon_ += corner_count;
            for (corner = 0; corner < corner_count - 1; corner++) {
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_SMOOTH_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
            }
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_SMOOTH_POLYGON(v0, KM_VERTEXPARAM_ENDOFSTRIP);
        }
        break;
    case NJD_MESHSET_TRIMESH:
        _njm_mesh = meshset;
        NJM_SET_POLY_STRIP_START();
        simple_draw_smooth_strips_cull(_njm_mesh);
        NJM_SET_POLY_STRIP_END();
        break;
    }
}

static void simple_draw_smooth_polygons_clip(NJS_MESHSET *meshset,
                                             NJS_MATERIAL *material)
{
    register Sint16 *mesh_cursor;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register struct calc_vertex *v3;
    Sint32 polygon;
    Sint32 corner;
    Sint32 corner_count;
    Uint32 clip_and;
    Uint32 clip_or;
    register Float facing;

    mesh_cursor = meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            clip_and = v0->clip & v1->clip & v2->clip;
            if (clip_and & CLIP_ALL) {
                continue;
            }
            clip_or = v0->clip | v1->clip | v2->clip;
            if (!(clip_or & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    continue;
                }
                EMIT_SMOOTH_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_SMOOTH_POLYGON(v1, KM_VERTEXPARAM_NORMAL);
                EMIT_SMOOTH_POLYGON(v2, KM_VERTEXPARAM_ENDOFSTRIP);
                _nj_draw_polygon_ += 3;
            } else {
                clip_cull_requested = 1;
                simple_clip_smooth_triangle(v0, v1, v2);
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            clip_and = v0->clip & v1->clip & v2->clip & v3->clip;
            if (clip_and & CLIP_ALL) {
                continue;
            }
            clip_or = v0->clip | v1->clip | v2->clip | v3->clip;
            if (!(clip_or & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    continue;
                }
                EMIT_SMOOTH_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_SMOOTH_POLYGON(v1, KM_VERTEXPARAM_NORMAL);
                EMIT_SMOOTH_POLYGON(v2, KM_VERTEXPARAM_NORMAL);
                EMIT_SMOOTH_POLYGON(v3, KM_VERTEXPARAM_ENDOFSTRIP);
                _nj_draw_polygon_ += 4;
            } else {
                clip_cull_requested = 1;
                clip_and = v0->clip & v1->clip & v2->clip;
                if (!(clip_and & CLIP_NEAR)) {
                    simple_clip_smooth_triangle(v0, v1, v2);
                }
                clip_and = v1->clip & v2->clip & v3->clip;
                if (!(clip_and & CLIP_NEAR)) {
                    simple_clip_smooth_triangle(v1, v2, v3);
                }
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            corner_count = (Uint16)*mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            clip_and = ~(Uint32)0;
            clip_or = 0;
            for (corner = 0; corner < corner_count; corner++) {
                clip_and &= WORK_VERTEX(mesh_cursor[corner])->clip;
                clip_or |= WORK_VERTEX(mesh_cursor[corner])->clip;
            }
            if (clip_and & CLIP_ALL) {
                mesh_cursor += corner_count;
                continue;
            }
            if (!(clip_or & CLIP_NEAR)) {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);

                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    mesh_cursor += corner_count;
                    continue;
                }
                _nj_draw_polygon_ += corner_count;
                for (corner = 0; corner < corner_count - 1; corner++) {
                    v0 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_SMOOTH_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_SMOOTH_POLYGON(v0, KM_VERTEXPARAM_ENDOFSTRIP);
            } else {
                clip_cull_requested = 0;
                v0 = WORK_VERTEX(*mesh_cursor++);
                v1 = WORK_VERTEX(*mesh_cursor++);
                for (corner = 0; corner < corner_count - 2; corner++) {
                    v2 = WORK_VERTEX(*mesh_cursor++);
                    clip_and = v0->clip & v1->clip & v2->clip;
                    if (!(clip_and & CLIP_NEAR)) {
                        simple_clip_smooth_triangle(v0, v1, v2);
                    }
                    v0 = v1;
                    v1 = v2;
                }
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        _njm_mesh = meshset;
        NJM_SET_POLY_STRIP_START();
        simple_draw_smooth_strips_near(_njm_mesh);
        NJM_SET_POLY_STRIP_END();
        break;
    }
}

#define EMIT_SMOOTH_TEX_POLYGON(vertex, uv_cursor, command)                    \
    {                                                                          \
        sq_cursor += 7;                                                        \
        *--sq_cursor = SIMPLE_INTENSITY((vertex)->normal);                     \
        *--sq_cursor = (Float)(uv_cursor)->tex.v * TEX_SCALE;                  \
        *--sq_cursor = (Float)(uv_cursor)->tex.u * TEX_SCALE;                  \
        (uv_cursor)++;                                                         \
        *--sq_cursor = (vertex)->oow;                                          \
        *--sq_cursor = (vertex)->sy;                                           \
        *--sq_cursor = (vertex)->sx;                                           \
        *(Uint32 *)--sq_cursor = command;                                      \
        prefetch(sq_cursor);                                                   \
        sq_cursor += VERTEX_WORDS;                                             \
    }

static void simple_draw_smooth_tex_polygons(NJS_MESHSET *meshset,
                                            NJS_MATERIAL *material)
{
    register Sint16 *mesh_cursor;
    register NJS_COLOR *corner_uv;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register struct calc_vertex *v3;
    Sint32 polygon;
    Sint32 corner;
    Sint32 corner_count;
    register Float facing;

    corner_uv = meshset->vertuv;
    mesh_cursor = meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                corner_uv += 3;
                continue;
            }
            EMIT_SMOOTH_TEX_POLYGON(v0, corner_uv, KM_VERTEXPARAM_NORMAL);
            EMIT_SMOOTH_TEX_POLYGON(v1, corner_uv, KM_VERTEXPARAM_NORMAL);
            EMIT_SMOOTH_TEX_POLYGON(v2, corner_uv, KM_VERTEXPARAM_ENDOFSTRIP);
            _nj_draw_polygon_ += 3;
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                corner_uv += 4;
                continue;
            }
            EMIT_SMOOTH_TEX_POLYGON(v0, corner_uv, KM_VERTEXPARAM_NORMAL);
            EMIT_SMOOTH_TEX_POLYGON(v1, corner_uv, KM_VERTEXPARAM_NORMAL);
            EMIT_SMOOTH_TEX_POLYGON(v2, corner_uv, KM_VERTEXPARAM_NORMAL);
            EMIT_SMOOTH_TEX_POLYGON(v3, corner_uv, KM_VERTEXPARAM_ENDOFSTRIP);
            _nj_draw_polygon_ += 4;
        }
        break;
    case NJD_MESHSET_N:
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            corner_count = (Uint16)*mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
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
            _nj_draw_polygon_ += corner_count;
            for (corner = 0; corner < corner_count - 1; corner++) {
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_SMOOTH_TEX_POLYGON(v0, corner_uv, KM_VERTEXPARAM_NORMAL);
            }
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_SMOOTH_TEX_POLYGON(v0, corner_uv, KM_VERTEXPARAM_ENDOFSTRIP);
        }
        break;
    case NJD_MESHSET_TRIMESH:
        _njm_mesh = meshset;
        NJM_SET_TEX_STRIP_START();
        simple_draw_smooth_tex_strips_cull(_njm_mesh);
        NJM_SET_TEX_STRIP_END();
        break;
    }
}

static void simple_draw_smooth_tex_polygons_clip(NJS_MESHSET *meshset,
                                                 NJS_MATERIAL *material)
{
    register Sint16 *mesh_cursor;
    register NJS_COLOR *corner_uv;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register struct calc_vertex *v3;
    Sint32 polygon;
    Sint32 corner;
    Sint32 corner_count;
    Uint32 clip_and;
    Uint32 clip_or;
    register Float facing;

    corner_uv = meshset->vertuv;
    mesh_cursor = meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            clip_and = v0->clip & v1->clip & v2->clip;
            if (clip_and & CLIP_ALL) {
                corner_uv += 3;
                continue;
            }
            clip_or = v0->clip | v1->clip | v2->clip;
            if (!(clip_or & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    corner_uv += 3;
                    continue;
                }
                EMIT_SMOOTH_TEX_POLYGON(v0, corner_uv, KM_VERTEXPARAM_NORMAL);
                EMIT_SMOOTH_TEX_POLYGON(v1, corner_uv, KM_VERTEXPARAM_NORMAL);
                EMIT_SMOOTH_TEX_POLYGON(v2, corner_uv,
                                        KM_VERTEXPARAM_ENDOFSTRIP);
                _nj_draw_polygon_ += 3;
            } else {
                clip_cull_requested = 1;
                /* @hack type: each coordinate makes a round trip through Float,
                 * here and in the function's two copies below, which changes no
                 * value but has SHC convert it with FLOAT and FTRC (0x8c63409e
                 * to 0x8c6340ea). without the casts, the section runs 188 bytes
                 * short and 826 of 1065 words differ */
                corner_uv_window[0].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[0].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[1].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[1].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[2].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[2].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                simple_clip_smooth_tex_triangle(v0, v1, v2, corner_uv_window);
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            clip_and = v0->clip & v1->clip & v2->clip & v3->clip;
            if (clip_and & CLIP_ALL) {
                corner_uv += 4;
                continue;
            }
            clip_or = v0->clip | v1->clip | v2->clip | v3->clip;
            if (!(clip_or & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    corner_uv += 4;
                    continue;
                }
                EMIT_SMOOTH_TEX_POLYGON(v0, corner_uv, KM_VERTEXPARAM_NORMAL);
                EMIT_SMOOTH_TEX_POLYGON(v1, corner_uv, KM_VERTEXPARAM_NORMAL);
                EMIT_SMOOTH_TEX_POLYGON(v2, corner_uv, KM_VERTEXPARAM_NORMAL);
                EMIT_SMOOTH_TEX_POLYGON(v3, corner_uv,
                                        KM_VERTEXPARAM_ENDOFSTRIP);
                _nj_draw_polygon_ += 4;
            } else {
                clip_cull_requested = 1;
                corner_uv_window[0].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[0].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[1].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[1].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[2].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[2].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[3].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[3].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                clip_and = v0->clip & v1->clip & v2->clip;
                if (!(clip_and & CLIP_NEAR)) {
                    simple_clip_smooth_tex_triangle(v0, v1, v2,
                                                    &corner_uv_window[0]);
                }
                clip_and = v1->clip & v2->clip & v3->clip;
                if (!(clip_and & CLIP_NEAR)) {
                    simple_clip_smooth_tex_triangle(v1, v2, v3,
                                                    corner_uv_window + 1);
                }
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            corner_count = (Uint16)*mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            clip_and = ~(Uint32)0;
            clip_or = 0;
            for (corner = 0; corner < corner_count; corner++) {
                clip_and &= WORK_VERTEX(mesh_cursor[corner])->clip;
                clip_or |= WORK_VERTEX(mesh_cursor[corner])->clip;
            }
            if (clip_and & CLIP_ALL) {
                corner_uv += corner_count;
                mesh_cursor += corner_count;
                continue;
            }
            if (!(clip_or & CLIP_NEAR)) {
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
                _nj_draw_polygon_ += corner_count;
                for (corner = 0; corner < corner_count - 1; corner++) {
                    v0 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_SMOOTH_TEX_POLYGON(v0, corner_uv,
                                            KM_VERTEXPARAM_NORMAL);
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_SMOOTH_TEX_POLYGON(v0, corner_uv,
                                        KM_VERTEXPARAM_ENDOFSTRIP);
            } else {
                clip_cull_requested = 0;
                v0 = WORK_VERTEX(*mesh_cursor++);
                v1 = WORK_VERTEX(*mesh_cursor++);
                corner_uv_window[0].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[0].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[1].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[1].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                for (corner = 0; corner < corner_count - 2; corner++) {
                    v2 = WORK_VERTEX(*mesh_cursor++);
                    corner_uv_window[2].tex.u = (Sint16)(Float)corner_uv->tex.u;
                    corner_uv_window[2].tex.v = (Sint16)(Float)corner_uv->tex.v;
                    corner_uv++;
                    clip_and = v0->clip & v1->clip & v2->clip;
                    if (!(clip_and & CLIP_NEAR)) {
                        simple_clip_smooth_tex_triangle(v0, v1, v2,
                                                        corner_uv_window);
                    }

                    corner_uv_window[0] = corner_uv_window[1];
                    corner_uv_window[1] = corner_uv_window[2];
                    v0 = v1;
                    v1 = v2;
                }
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        _njm_mesh = meshset;
        NJM_SET_TEX_STRIP_START();
        simple_draw_smooth_tex_strips_near(_njm_mesh);
        NJM_SET_TEX_STRIP_END();
        break;
    }
}

#define EMIT_SMOOTH_ENV_POLYGON(vertex, command)                               \
    {                                                                          \
        sq_cursor += 7;                                                        \
        *--sq_cursor = SIMPLE_INTENSITY((vertex)->normal);                     \
        *--sq_cursor = (ENV_COLUMN_DOT((vertex)->normal, 1) * 0.5F) + 0.5F;    \
        *--sq_cursor = (ENV_COLUMN_DOT((vertex)->normal, 0) * 0.5F) + 0.5F;    \
        *--sq_cursor = (vertex)->oow;                                          \
        *--sq_cursor = (vertex)->sy;                                           \
        *--sq_cursor = (vertex)->sx;                                           \
        *(Uint32 *)--sq_cursor = command;                                      \
        prefetch(sq_cursor);                                                   \
        sq_cursor += VERTEX_WORDS;                                             \
    }

static void simple_draw_smooth_env_polygons(NJS_MESHSET *meshset,
                                            NJS_MATERIAL *material)
{
    register Sint16 *mesh_cursor;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register struct calc_vertex *v3;
    Sint32 polygon;
    Sint32 corner;
    Sint32 corner_count;
    register Float facing;

    mesh_cursor = meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                continue;
            }
            EMIT_SMOOTH_ENV_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
            EMIT_SMOOTH_ENV_POLYGON(v1, KM_VERTEXPARAM_NORMAL);
            EMIT_SMOOTH_ENV_POLYGON(v2, KM_VERTEXPARAM_ENDOFSTRIP);
            _nj_draw_polygon_ += 3;
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                continue;
            }
            EMIT_SMOOTH_ENV_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
            EMIT_SMOOTH_ENV_POLYGON(v1, KM_VERTEXPARAM_NORMAL);
            EMIT_SMOOTH_ENV_POLYGON(v2, KM_VERTEXPARAM_NORMAL);
            EMIT_SMOOTH_ENV_POLYGON(v3, KM_VERTEXPARAM_ENDOFSTRIP);
            _nj_draw_polygon_ += 4;
        }
        break;
    case NJD_MESHSET_N:
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            corner_count = (Uint16)*mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            v0 = WORK_VERTEX(mesh_cursor[0]);
            v1 = WORK_VERTEX(mesh_cursor[1]);
            v2 = WORK_VERTEX(mesh_cursor[2]);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                mesh_cursor += corner_count;
                continue;
            }
            _nj_draw_polygon_ += corner_count;
            for (corner = 0; corner < corner_count - 1; corner++) {
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_SMOOTH_ENV_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
            }
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_SMOOTH_ENV_POLYGON(v0, KM_VERTEXPARAM_ENDOFSTRIP);
        }
        break;
    case NJD_MESHSET_TRIMESH:
        _njm_mesh = meshset;
        NJM_SET_ENV_STRIP_START();
        simple_draw_smooth_env_strips_cull(_njm_mesh);
        NJM_SET_ENV_STRIP_END();
        break;
    }
}

static void simple_draw_smooth_env_polygons_clip(NJS_MESHSET *meshset,
                                                 NJS_MATERIAL *material)
{
    register Sint16 *mesh_cursor;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register struct calc_vertex *v3;
    Sint32 polygon;
    Sint32 corner;
    Sint32 corner_count;
    Uint32 clip_and;
    Uint32 clip_or;
    register Float facing;

    mesh_cursor = meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            clip_and = v0->clip & v1->clip & v2->clip;
            if (clip_and & CLIP_ALL) {
                continue;
            }
            clip_or = v0->clip | v1->clip | v2->clip;
            if (!(clip_or & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    continue;
                }
                EMIT_SMOOTH_ENV_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_SMOOTH_ENV_POLYGON(v1, KM_VERTEXPARAM_NORMAL);
                EMIT_SMOOTH_ENV_POLYGON(v2, KM_VERTEXPARAM_ENDOFSTRIP);
                _nj_draw_polygon_ += 3;
            } else {
                clip_cull_requested = 1;
                simple_clip_smooth_env_triangle(v0, v1, v2);
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            clip_and = v0->clip & v1->clip & v2->clip & v3->clip;
            if (clip_and & CLIP_ALL) {
                continue;
            }
            clip_or = v0->clip | v1->clip | v2->clip | v3->clip;
            if (!(clip_or & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    continue;
                }
                EMIT_SMOOTH_ENV_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_SMOOTH_ENV_POLYGON(v1, KM_VERTEXPARAM_NORMAL);
                EMIT_SMOOTH_ENV_POLYGON(v2, KM_VERTEXPARAM_NORMAL);
                EMIT_SMOOTH_ENV_POLYGON(v3, KM_VERTEXPARAM_ENDOFSTRIP);
                _nj_draw_polygon_ += 4;
            } else {
                clip_cull_requested = 1;
                clip_and = v0->clip & v1->clip & v2->clip;
                if (!(clip_and & CLIP_NEAR)) {
                    simple_clip_smooth_env_triangle(v0, v1, v2);
                }
                clip_and = v1->clip & v2->clip & v3->clip;
                if (!(clip_and & CLIP_NEAR)) {
                    simple_clip_smooth_env_triangle(v1, v2, v3);
                }
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            corner_count = (Uint16)*mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            clip_and = ~(Uint32)0;
            clip_or = 0;
            for (corner = 0; corner < corner_count; corner++) {
                clip_and &= WORK_VERTEX(mesh_cursor[corner])->clip;
                clip_or |= WORK_VERTEX(mesh_cursor[corner])->clip;
            }
            if (clip_and & CLIP_ALL) {
                mesh_cursor += corner_count;
                continue;
            }
            if (!(clip_or & CLIP_NEAR)) {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);

                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    mesh_cursor += corner_count;
                    continue;
                }
                _nj_draw_polygon_ += corner_count;
                for (corner = 0; corner < corner_count - 1; corner++) {
                    v0 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_SMOOTH_ENV_POLYGON(v0, KM_VERTEXPARAM_NORMAL);
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_SMOOTH_ENV_POLYGON(v0, KM_VERTEXPARAM_ENDOFSTRIP);
            } else {
                clip_cull_requested = 0;
                v0 = WORK_VERTEX(*mesh_cursor++);
                v1 = WORK_VERTEX(*mesh_cursor++);
                for (corner = 0; corner < corner_count - 2; corner++) {
                    v2 = WORK_VERTEX(*mesh_cursor++);
                    clip_and = v0->clip & v1->clip & v2->clip;
                    if (!(clip_and & CLIP_NEAR)) {
                        simple_clip_smooth_env_triangle(v0, v1, v2);
                    }
                    v0 = v1;
                    v1 = v2;
                }
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        _njm_mesh = meshset;
        NJM_SET_ENV_STRIP_START();
        simple_draw_smooth_env_strips_near(_njm_mesh);
        NJM_SET_ENV_STRIP_END();
        break;
    }
}

static void simple_draw_flat_polygons(NJS_MESHSET *meshset,
                                      NJS_MATERIAL *material)
{
    register Sint16 *mesh_cursor;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register struct calc_vertex *v3;
    Sint32 polygon;
    Sint32 corner;
    Sint32 corner_count;
    register Float facing;
    NJS_VECTOR flat_normal;
    NJS_ARGB flat_color;
    register Float flat_intensity;

    mesh_cursor = meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                continue;
            }
            flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
            flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
            flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
            njUnitVector(&flat_normal);
            flat_intensity = SIMPLE_INTENSITY(flat_normal);
            flat_color.a = _nj_light_material_.diffuse.a;
            flat_color.r = _nj_light_material_.diffuse.r * flat_intensity;
            flat_color.g = _nj_light_material_.diffuse.g * flat_intensity;
            flat_color.b = _nj_light_material_.diffuse.b * flat_intensity;
            EMIT_FLAT_POSITION(v0, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_POSITION(v1, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_COLOR(v2, KM_VERTEXPARAM_ENDOFSTRIP, flat_color);
            _nj_draw_polygon_ += 3;
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                continue;
            }
            flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
            flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
            flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
            njUnitVector(&flat_normal);
            flat_intensity = SIMPLE_INTENSITY(flat_normal);
            flat_color.a = _nj_light_material_.diffuse.a;
            flat_color.r = _nj_light_material_.diffuse.r * flat_intensity;
            flat_color.g = _nj_light_material_.diffuse.g * flat_intensity;
            flat_color.b = _nj_light_material_.diffuse.b * flat_intensity;
            EMIT_FLAT_POSITION(v0, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_POSITION(v1, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_COLOR(v2, KM_VERTEXPARAM_NORMAL, flat_color);
            EMIT_FLAT_COLOR(v3, KM_VERTEXPARAM_ENDOFSTRIP, flat_color);
            _nj_draw_polygon_ += 4;
        }
        break;
    case NJD_MESHSET_N:
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            corner_count = (Uint16)*mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            v0 = WORK_VERTEX(mesh_cursor[0]);
            v1 = WORK_VERTEX(mesh_cursor[1]);
            v2 = WORK_VERTEX(mesh_cursor[2]);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                mesh_cursor += corner_count;
                continue;
            }
            _nj_draw_polygon_ += corner_count;
            flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
            flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
            flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
            njUnitVector(&flat_normal);
            flat_intensity = SIMPLE_INTENSITY(flat_normal);
            flat_color.a = _nj_light_material_.diffuse.a;
            flat_color.r = _nj_light_material_.diffuse.r * flat_intensity;
            flat_color.g = _nj_light_material_.diffuse.g * flat_intensity;
            flat_color.b = _nj_light_material_.diffuse.b * flat_intensity;
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_POSITION(v0, KM_VERTEXPARAM_NORMAL);
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_POSITION(v0, KM_VERTEXPARAM_NORMAL);
            for (corner = 2; corner < corner_count - 1; corner++) {
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_COLOR(v0, KM_VERTEXPARAM_NORMAL, flat_color);
            }
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_COLOR(v0, KM_VERTEXPARAM_ENDOFSTRIP, flat_color);
        }
        break;
    case NJD_MESHSET_TRIMESH:
        _njm_mesh = meshset;
        NJM_SET_POLY_STRIP_START();
        simple_draw_flat_path_strips_cull(_njm_mesh);
        NJM_SET_POLY_STRIP_END();
        break;
    }
}

static void simple_draw_flat_polygons_clip(NJS_MESHSET *meshset,
                                           NJS_MATERIAL *material)
{
    register Sint16 *mesh_cursor;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register struct calc_vertex *v3;
    Sint32 polygon;
    Sint32 corner;
    Sint32 corner_count;
    Uint32 clip_and;
    Uint32 clip_or;
    Float facing;
    register Float v1_x;
    NJS_VECTOR flat_normal;
    register struct calc_vertex *v1_copy;
    struct calc_vertex *v0_copy;
    NJS_ARGB flat_color;
    register Float flat_intensity;

    mesh_cursor = meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            /* @hack spelling: v0's normal read through v0_copy, a copy of v0,
             * which keeps v0 in R14 and reads its clip flags before forming the
             * other two corners' addresses (0x8c635fd6 to 0x8c635fe8); read
             * through v0, 60 of 1115 words differ */
            v0_copy = v0;
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            clip_and = v0->clip & v1->clip & v2->clip;
            if (clip_and & CLIP_ALL) {
                continue;
            }
            clip_or = v0->clip | v1->clip | v2->clip;
            if (!(clip_or & CLIP_NEAR)) {
                v1_x = v1->sx;
                facing = ((v1_x - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1_x));
                if (facing > 0.0F) {
                    continue;
                }
                flat_normal.x = v0_copy->normal.x + v1->normal.x + v2->normal.x;
                flat_normal.y = v0_copy->normal.y + v1->normal.y + v2->normal.y;
                flat_normal.z = v0_copy->normal.z + v1->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                flat_color.a = _nj_light_material_.diffuse.a;
                flat_color.r = _nj_light_material_.diffuse.r * flat_intensity;
                flat_color.g = _nj_light_material_.diffuse.g * flat_intensity;
                flat_color.b = _nj_light_material_.diffuse.b * flat_intensity;
                EMIT_FLAT_POSITION(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_POSITION(v1, KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_COLOR(v2, KM_VERTEXPARAM_ENDOFSTRIP, flat_color);
                _nj_draw_polygon_ += 3;
            } else {
                clip_cull_requested = 1;
                flat_normal.x = v0_copy->normal.x + v1->normal.x + v2->normal.x;
                flat_normal.y = v0_copy->normal.y + v1->normal.y + v2->normal.y;
                flat_normal.z = v0_copy->normal.z + v1->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                flat_color.a = _nj_light_material_.diffuse.a;
                flat_color.r = _nj_light_material_.diffuse.r * flat_intensity;
                flat_color.g = _nj_light_material_.diffuse.g * flat_intensity;
                flat_color.b = _nj_light_material_.diffuse.b * flat_intensity;
                simple_clip_constant_color_triangle(v0, v1, v2, &flat_color);
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            /* @hack spelling: v1's normal read through v1_copy, a copy of v1,
             * which puts the N-gon loop's corner in R5 and its cursor in R4
             * (0x8c636526, 0x8c63652e); read through v1, the two trade
             * registers and 5 of 1115 words differ */
            v1_copy = v1;
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            clip_and = v0->clip & v1->clip & v2->clip & v3->clip;
            if (clip_and & CLIP_ALL) {
                continue;
            }
            clip_or = v0->clip | v1->clip | v2->clip | v3->clip;
            if (!(clip_or & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    continue;
                }
                flat_normal.x = v0->normal.x + v1_copy->normal.x + v2->normal.x;
                flat_normal.y = v0->normal.y + v1_copy->normal.y + v2->normal.y;
                flat_normal.z = v0->normal.z + v1_copy->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                flat_color.a = _nj_light_material_.diffuse.a;
                flat_color.r = _nj_light_material_.diffuse.r * flat_intensity;
                flat_color.g = _nj_light_material_.diffuse.g * flat_intensity;
                flat_color.b = _nj_light_material_.diffuse.b * flat_intensity;
                EMIT_FLAT_POSITION(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_POSITION(v1, KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_COLOR(v2, KM_VERTEXPARAM_NORMAL, flat_color);
                EMIT_FLAT_COLOR(v3, KM_VERTEXPARAM_ENDOFSTRIP, flat_color);
                _nj_draw_polygon_ += 4;
            } else {
                clip_cull_requested = 1;
                flat_normal.x = v0->normal.x + v1_copy->normal.x + v2->normal.x;
                flat_normal.y = v0->normal.y + v1_copy->normal.y + v2->normal.y;
                flat_normal.z = v0->normal.z + v1_copy->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                flat_color.a = _nj_light_material_.diffuse.a;
                flat_color.r = _nj_light_material_.diffuse.r * flat_intensity;
                flat_color.g = _nj_light_material_.diffuse.g * flat_intensity;
                flat_color.b = _nj_light_material_.diffuse.b * flat_intensity;
                clip_and = v0->clip & v1->clip & v2->clip;
                if (!(clip_and & CLIP_NEAR)) {
                    simple_clip_constant_color_triangle(v0, v1, v2,
                                                        &flat_color);
                }
                clip_and = v1->clip & v2->clip & v3->clip;
                if (!(clip_and & CLIP_NEAR)) {
                    simple_clip_constant_color_triangle(v1, v2, v3,
                                                        &flat_color);
                }
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            corner_count = (Uint16)*mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            clip_and = (Uint32)-1;
            clip_or = 0;

            /* @hack spelling: corner = corner + 1 and clip_or = clip_or |
             * WORK_VERTEX(mesh_cursor[corner])->clip spelled out, which keeps
             * clip_and in R7 and clip_or in R6 (0x8c63651a, 0x8c63651e); with
             * ++ and |=, the two trade registers, and 7 of 1115 words differ */
            for (corner = 0; corner < corner_count; corner = corner + 1) {
                clip_and &= WORK_VERTEX(mesh_cursor[corner])->clip;
                clip_or = clip_or | WORK_VERTEX(mesh_cursor[corner])->clip;
            }
            if (clip_and & CLIP_ALL) {
                mesh_cursor += corner_count;
                continue;
            }
            if (!(clip_or & CLIP_NEAR)) {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    mesh_cursor += corner_count;
                    continue;
                }
                _nj_draw_polygon_ += corner_count;
                flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
                flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
                flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                flat_color.a = _nj_light_material_.diffuse.a;
                flat_color.r = _nj_light_material_.diffuse.r * flat_intensity;
                flat_color.g = _nj_light_material_.diffuse.g * flat_intensity;
                flat_color.b = _nj_light_material_.diffuse.b * flat_intensity;
                v3 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_POSITION(v3, KM_VERTEXPARAM_NORMAL);
                v3 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_POSITION(v3, KM_VERTEXPARAM_NORMAL);
                for (corner = 2; corner < corner_count - 1; corner++) {
                    v3 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_FLAT_COLOR(v3, KM_VERTEXPARAM_NORMAL, flat_color);
                }
                v3 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_COLOR(v3, KM_VERTEXPARAM_ENDOFSTRIP, flat_color);
            } else {
                clip_cull_requested = 0;

                /* @bug uninitialized: this branch lights an N-gon that crosses
                 * the near plane from v0, v1 and v2 before it sets them,
                 * reading their normals at 0x8c63676e, 0x8c636770 and
                 * 0x8c636774. they hold corners of the meshset's last earlier
                 * N-gon whose corners share no clip flag. with none, they are
                 * R12, R14 and R13 as _njSimpleDrawModelSub left them: the
                 * meshset, _nj_light_material_'s address and the material */
                flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
                flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
                flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                flat_color.a = _nj_light_material_.diffuse.a;
                flat_color.r = _nj_light_material_.diffuse.r * flat_intensity;
                flat_color.g = _nj_light_material_.diffuse.g * flat_intensity;
                flat_color.b = _nj_light_material_.diffuse.b * flat_intensity;
                corner_count -= 2;
                v0 = WORK_VERTEX(*mesh_cursor++);
                v1 = WORK_VERTEX(*mesh_cursor++);
                for (corner = 0; corner < corner_count; corner++) {
                    v2 = WORK_VERTEX(*mesh_cursor++);
                    clip_and = v0->clip & v1->clip & v2->clip;
                    if (!(clip_and & CLIP_NEAR)) {
                        simple_clip_constant_color_triangle(v0, v1, v2,
                                                            &flat_color);
                    }
                    v0 = v1;
                    v1 = v2;
                }
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        _njm_mesh = meshset;
        NJM_SET_POLY_STRIP_START();
        simple_draw_flat_path_strips_near(_njm_mesh);
        NJM_SET_POLY_STRIP_END();
        break;
    }
}

static void simple_draw_flat_tex_polygons(NJS_MESHSET *meshset,
                                          NJS_MATERIAL *material)
{
    register Sint16 *mesh_cursor;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register struct calc_vertex *v3;
    Sint32 polygon;
    Sint32 corner;
    Sint32 corner_count;
    register Float facing;
    NJS_VECTOR flat_normal;
    register NJS_COLOR *corner_uv;
    register Float flat_intensity;

    corner_uv = meshset->vertuv;
    mesh_cursor = meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                corner_uv += 3;
                continue;
            }
            flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
            flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
            flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
            njUnitVector(&flat_normal);
            flat_intensity = SIMPLE_INTENSITY(flat_normal);
            EMIT_FLAT_TEX_POSITION_NEXT(v0, corner_uv, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_TEX_POSITION_NEXT(v1, corner_uv, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_TEX_INTENSITY_NEXT(
                v2, corner_uv, KM_VERTEXPARAM_ENDOFSTRIP, flat_intensity);
            _nj_draw_polygon_ += 3;
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                corner_uv += 4;
                continue;
            }
            flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
            flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
            flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
            njUnitVector(&flat_normal);
            flat_intensity = SIMPLE_INTENSITY(flat_normal);
            EMIT_FLAT_TEX_POSITION_NEXT(v0, corner_uv, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_TEX_POSITION_NEXT(v1, corner_uv, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_TEX_INTENSITY_NEXT(v2, corner_uv, KM_VERTEXPARAM_NORMAL,
                                         flat_intensity);
            EMIT_FLAT_TEX_INTENSITY_NEXT(
                v3, corner_uv, KM_VERTEXPARAM_ENDOFSTRIP, flat_intensity);
            _nj_draw_polygon_ += 4;
        }
        break;
    case NJD_MESHSET_N:
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            corner_count = (Uint16)*mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
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
            _nj_draw_polygon_ += corner_count;
            flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
            flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
            flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
            njUnitVector(&flat_normal);
            flat_intensity = SIMPLE_INTENSITY(flat_normal);
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_TEX_POSITION_NEXT(v0, corner_uv, KM_VERTEXPARAM_NORMAL);
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_TEX_POSITION_NEXT(v0, corner_uv, KM_VERTEXPARAM_NORMAL);
            for (corner = 2; corner < corner_count - 1; corner++) {
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_TEX_INTENSITY_NEXT(
                    v0, corner_uv, KM_VERTEXPARAM_NORMAL, flat_intensity);
            }
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_TEX_INTENSITY_NEXT(
                v0, corner_uv, KM_VERTEXPARAM_ENDOFSTRIP, flat_intensity);
        }
        break;
    case NJD_MESHSET_TRIMESH:
        _njm_mesh = meshset;
        NJM_SET_TEX_STRIP_START();
        simple_draw_flat_path_tex_strips_cull(_njm_mesh);
        NJM_SET_TEX_STRIP_END();
        break;
    }
}

static void simple_draw_flat_tex_polygons_clip(NJS_MESHSET *meshset,
                                               NJS_MATERIAL *material)
{
    register Sint16 *mesh_cursor;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register struct calc_vertex *v3;
    Sint32 polygon;
    Sint32 corner;
    Sint32 corner_count;
    Uint32 clip_and;
    Uint32 clip_or;
    Float facing;
    NJS_VECTOR flat_normal;
    register NJS_COLOR *corner_uv;
    Float flat_intensity;
    struct calc_vertex *v1_copy;

    corner_uv = meshset->vertuv;
    mesh_cursor = meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            /* @hack spelling: v1's normal read through v1_copy, a copy of v1,
             * which forms v0's address in R12 before v1's in R14 (0x8c636ffc to
             * 0x8c63700c); read through v1, 106 of 1277 words differ */
            v1_copy = v1;
            v2 = WORK_VERTEX(*mesh_cursor++);
            clip_and = v0->clip & v1->clip & v2->clip;
            if (clip_and & CLIP_ALL) {
                corner_uv += 3;
                continue;
            }
            clip_or = v0->clip | v1->clip | v2->clip;
            if (!(clip_or & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    corner_uv += 3;
                    continue;
                }
                flat_normal.x = v0->normal.x + v1_copy->normal.x + v2->normal.x;
                flat_normal.y = v0->normal.y + v1_copy->normal.y + v2->normal.y;
                flat_normal.z = v0->normal.z + v1_copy->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                EMIT_FLAT_TEX_POSITION_NEXT(v0, corner_uv,
                                            KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_TEX_POSITION_NEXT(v1, corner_uv,
                                            KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_TEX_INTENSITY_NEXT(
                    v2, corner_uv, KM_VERTEXPARAM_ENDOFSTRIP, flat_intensity);
                _nj_draw_polygon_ += 3;
            } else {
                /* @hack spelling: the triangle's three window entries written
                 * through first_uv, middle_uv and last_uv, which gives the
                 * N-gon branch's corner and window addresses their frame slots
                 * (0x8c637290 to 0x8c63729a); written as the window's members,
                 * the slots at 0 and 4 trade, so do those at 16 and 20, and 15
                 * of 1277 words differ */
                NJS_TEX *first_uv;
                NJS_TEX *last_uv;
                NJS_TEX *middle_uv;
                clip_cull_requested = 1;
                flat_normal.x = v0->normal.x + v1_copy->normal.x + v2->normal.x;
                flat_normal.y = v0->normal.y + v1_copy->normal.y + v2->normal.y;
                flat_normal.z = v0->normal.z + v1_copy->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                first_uv = (NJS_TEX *)&corner_uv_window[0];
                /* @hack type: each coordinate makes a round trip through Float,
                 * here and in the function's two copies below, which changes no
                 * value but has SHC convert it with FLOAT and FTRC (0x8c6371fe
                 * to 0x8c63724c). without the casts, the section runs 200 bytes
                 * short and 989 of 1277 words differ */
                first_uv->u = (Sint16)(Float)corner_uv->tex.u;
                first_uv->v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;

                middle_uv = (NJS_TEX *)&corner_uv_window[1];
                middle_uv->u = (Sint16)(Float)corner_uv->tex.u;
                middle_uv->v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;

                last_uv = (NJS_TEX *)&corner_uv_window[2];
                last_uv->u = (Sint16)(Float)corner_uv->tex.u;
                last_uv->v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                simple_clip_flat_tex_triangle(v0, v1, v2, corner_uv_window,
                                              flat_intensity);

                /* @hack redundant: a store through last_uv that never runs,
                 * which loads &corner_uv_window[1] and &corner_uv_window[2]
                 * from pool words of their own, into R3 and R11 (0x8c636fd0,
                 * 0x8c636fd4); without it, the one word R11 loads serves both,
                 * 1147 of 1277 words differ and the function is 4 bytes
                 * shorter */
                if (0) {
                    last_uv->u = 0;
                }
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            clip_and = v0->clip & v1->clip & v2->clip & v3->clip;
            if (clip_and & CLIP_ALL) {
                corner_uv += 4;
                continue;
            }
            clip_or = v0->clip | v1->clip | v2->clip | v3->clip;
            if (!(clip_or & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    corner_uv += 4;
                    continue;
                }
                flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
                flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
                flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                EMIT_FLAT_TEX_POSITION_NEXT(v0, corner_uv,
                                            KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_TEX_POSITION_NEXT(v1, corner_uv,
                                            KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_TEX_INTENSITY_NEXT(
                    v2, corner_uv, KM_VERTEXPARAM_NORMAL, flat_intensity);
                EMIT_FLAT_TEX_INTENSITY_NEXT(
                    v3, corner_uv, KM_VERTEXPARAM_ENDOFSTRIP, flat_intensity);
                _nj_draw_polygon_ += 4;
            } else {
                /* @hack spelling: the quad's last window entry written through
                 * last_uv, which gives &corner_uv_window[0] the frame slot at 4
                 * and &corner_uv_window[1] the one at 0 (0x8c636fe6,
                 * 0x8c636fe8); written as the window's member, the two slots
                 * trade and 21 of 1277 words differ */
                NJS_TEX *last_uv;
                clip_cull_requested = 1;
                flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
                flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
                flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                corner_uv_window[0].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[0].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[1].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[1].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[2].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[2].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                last_uv = (NJS_TEX *)&corner_uv_window[3];
                last_uv->u = (Sint16)(Float)corner_uv->tex.u;
                last_uv->v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                clip_and = v0->clip & v1->clip & v2->clip;
                if (!(clip_and & CLIP_NEAR)) {
                    simple_clip_flat_tex_triangle(
                        v0, v1, v2, &corner_uv_window[0], flat_intensity);
                }
                clip_and = v1->clip & v2->clip & v3->clip;
                if (!(clip_and & CLIP_NEAR)) {
                    simple_clip_flat_tex_triangle(
                        v1, v2, v3, corner_uv_window + 1, flat_intensity);
                }

                /* @hack redundant: a copy within the window that never runs,
                 * which gives the N-gon branch's window addresses the frame
                 * slots at 4, 0 and 8 (0x8c637294 to 0x8c63729e); without it,
                 * the three slots rotate and 11 of 1277 words differ */
                if (0) {
                    corner_uv_window[1].tex.u = corner_uv_window[0].tex.u;
                }
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            /* @hack redundant: clip_or declared again, as a register, in the
             * N-gon loop's body, which gives the window addresses the frame
             * slots at 4 and 0 (0x8c637294, 0x8c637298) and loads two normals
             * into FR5 and FR4 (0x8c637302, 0x8c63730c); declared once, the
             * slots trade and so do the registers, 431 of 1277 words differ and
             * the function is 8 bytes shorter */
            register Uint32 clip_or;
            corner_count = (Uint16)*mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            clip_and = (Uint32)-1;
            clip_or = 0;
            for (corner = 0; corner < corner_count; corner++) {
                clip_and &= WORK_VERTEX(mesh_cursor[corner])->clip;
                clip_or |= WORK_VERTEX(mesh_cursor[corner])->clip;
            }
            if (clip_and & CLIP_ALL) {
                corner_uv += corner_count;
                mesh_cursor += corner_count;
                continue;
            }
            if (!(clip_or & CLIP_NEAR)) {
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
                _nj_draw_polygon_ += corner_count;
                flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
                flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
                flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                v3 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_TEX_POSITION_NEXT(v3, corner_uv,
                                            KM_VERTEXPARAM_NORMAL);
                v3 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_TEX_POSITION_NEXT(v3, corner_uv,
                                            KM_VERTEXPARAM_NORMAL);
                for (corner = 2; corner < corner_count - 1; corner++) {
                    v3 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_FLAT_TEX_INTENSITY_NEXT(
                        v3, corner_uv, KM_VERTEXPARAM_NORMAL, flat_intensity);
                }
                v3 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_TEX_INTENSITY_NEXT(
                    v3, corner_uv, KM_VERTEXPARAM_ENDOFSTRIP, flat_intensity);
            } else {
                clip_cull_requested = 0;

                /* @bug uninitialized: this branch lights an N-gon that crosses
                 * the near plane from v0, v1 and v2 before it sets them,
                 * reading their normals at 0x8c63788e, 0x8c637890 and
                 * 0x8c637894. they hold corners of the meshset's last earlier
                 * N-gon whose corners share no clip flag. with none, they are
                 * R12, R14 and R13 as _njSimpleDrawModelSub left them: the
                 * meshset, _nj_light_material_'s address and the material */
                flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
                flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
                flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                v0 = WORK_VERTEX(*mesh_cursor++);
                v1 = WORK_VERTEX(*mesh_cursor++);
                corner_uv_window[0].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[0].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[1].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[1].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;

                corner = 0;
                corner_count -= 2;
                for (; corner < corner_count; corner++) {
                    v2 = WORK_VERTEX(*mesh_cursor++);
                    corner_uv_window[2].tex.u = (Sint16)(Float)corner_uv->tex.u;
                    corner_uv_window[2].tex.v = (Sint16)(Float)corner_uv->tex.v;
                    corner_uv++;
                    clip_and = v0->clip & v1->clip & v2->clip;
                    if (!(clip_and & CLIP_NEAR)) {
                        simple_clip_flat_tex_triangle(
                            v0, v1, v2, corner_uv_window, flat_intensity);
                    }
                    corner_uv_window[0] = corner_uv_window[1];
                    corner_uv_window[1] = corner_uv_window[2];
                    v0 = v1;
                    v1 = v2;
                }
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        _njm_mesh = meshset;
        NJM_SET_TEX_STRIP_START();
        simple_draw_flat_path_tex_strips_near(_njm_mesh);
        NJM_SET_TEX_STRIP_END();
        break;
    }

    /* @hack redundant: a read of meshset->nbMesh that never runs, which keeps
     * meshset in the frame slot at 12 and the N-gon branch's corner at 16
     * (0x8c636f96, 0x8c637290); without it, the two slots trade and 15 of 1277
     * words differ */
    if (0) {
        meshset->nbMesh;
    }
}

static void simple_draw_unlit_tex_polygons(NJS_MESHSET *meshset,
                                           NJS_MATERIAL *material)
{
    register Sint16 *mesh_cursor;
    register NJS_COLOR *corner_uv;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register struct calc_vertex *v3;
    Sint32 polygon;
    Sint32 corner;
    Sint32 corner_count;
    register Float facing;

    corner_uv = meshset->vertuv;
    mesh_cursor = meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                corner_uv += 3;
            } else {
                EMIT_UNLIT_TEX_NEXT(v0, corner_uv, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_TEX_NEXT(v1, corner_uv, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_TEX_NEXT(v2, corner_uv, KM_VERTEXPARAM_ENDOFSTRIP);
                _nj_draw_polygon_ += 3;
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                corner_uv += 4;
            } else {
                EMIT_UNLIT_TEX_NEXT(v0, corner_uv, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_TEX_NEXT(v1, corner_uv, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_TEX_NEXT(v2, corner_uv, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_TEX_NEXT(v3, corner_uv, KM_VERTEXPARAM_ENDOFSTRIP);
                _nj_draw_polygon_ += 4;
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            corner_count = (Uint16)*mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            v0 = WORK_VERTEX(mesh_cursor[0]);
            v1 = WORK_VERTEX(mesh_cursor[1]);
            v2 = WORK_VERTEX(mesh_cursor[2]);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                mesh_cursor += corner_count;
                corner_uv += corner_count;
            } else {
                _nj_draw_polygon_ += corner_count;
                for (corner = 0; corner < corner_count - 1; corner++) {
                    v0 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_UNLIT_TEX_NEXT(v0, corner_uv, KM_VERTEXPARAM_NORMAL);
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_UNLIT_TEX_NEXT(v0, corner_uv, KM_VERTEXPARAM_ENDOFSTRIP);
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        _njm_mesh = meshset;
        NJM_SET_TEX_X_STRIP_START();
        simple_draw_unlit_tex_strips_cull(_njm_mesh);
        NJM_SET_TEX_X_STRIP_END();
        break;
    }
}

static void simple_draw_unlit_tex_polygons_clip(NJS_MESHSET *meshset,
                                                NJS_MATERIAL *material)
{
    register Sint16 *mesh_cursor;
    register NJS_COLOR *corner_uv;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register struct calc_vertex *v3;
    Sint32 polygon;
    Sint32 corner;
    Sint32 corner_count;
    Uint32 clip_and;
    Uint32 clip_or;
    register Float facing;

    corner_uv = meshset->vertuv;
    mesh_cursor = meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            clip_and = v0->clip & v1->clip & v2->clip;
            if (clip_and & CLIP_ALL) {
                corner_uv += 3;
                continue;
            }
            clip_or = v0->clip | v1->clip | v2->clip;
            if (!(clip_or & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    corner_uv += 3;
                    continue;
                }
                EMIT_UNLIT_TEX_NEXT(v0, corner_uv, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_TEX_NEXT(v1, corner_uv, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_TEX_NEXT(v2, corner_uv, KM_VERTEXPARAM_ENDOFSTRIP);
                _nj_draw_polygon_ += 3;
            } else {
                clip_cull_requested = 1;
                /* @hack type: each coordinate makes a round trip through Float,
                 * here and in the function's two copies below, which changes no
                 * value but has SHC convert it with FLOAT and FTRC (0x8c638074
                 * to 0x8c6380c0). without the casts, the section runs 196 bytes
                 * short and 686 of 890 words differ */
                corner_uv_window[0].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[0].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[1].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[1].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[2].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[2].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                simple_clip_unlit_tex_triangle(v0, v1, v2, corner_uv_window);
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            clip_and = v0->clip & v1->clip & v2->clip & v3->clip;
            if (clip_and & CLIP_ALL) {
                corner_uv += 4;
                continue;
            }
            clip_or = v0->clip | v1->clip | v2->clip | v3->clip;
            if (!(clip_or & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    corner_uv += 4;
                    continue;
                }
                EMIT_UNLIT_TEX_NEXT(v0, corner_uv, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_TEX_NEXT(v1, corner_uv, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_TEX_NEXT(v2, corner_uv, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_TEX_NEXT(v3, corner_uv, KM_VERTEXPARAM_ENDOFSTRIP);
                _nj_draw_polygon_ += 4;
            } else {
                clip_cull_requested = 1;
                corner_uv_window[0].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[0].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[1].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[1].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[2].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[2].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[3].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[3].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                clip_and = v0->clip & v1->clip & v2->clip;
                if (!(clip_and & CLIP_NEAR)) {
                    simple_clip_unlit_tex_triangle(v0, v1, v2,
                                                   &corner_uv_window[0]);
                }
                clip_and = v1->clip & v2->clip & v3->clip;
                if (!(clip_and & CLIP_NEAR)) {
                    simple_clip_unlit_tex_triangle(v1, v2, v3,
                                                   corner_uv_window + 1);
                }
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            corner_count = (Uint16)*mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            clip_and = ~(Uint32)0;
            clip_or = 0;
            for (corner = 0; corner < corner_count; corner++) {
                clip_and &= WORK_VERTEX(mesh_cursor[corner])->clip;
                clip_or |= WORK_VERTEX(mesh_cursor[corner])->clip;
            }
            if (clip_and & CLIP_ALL) {
                corner_uv += corner_count;
                mesh_cursor += corner_count;
                continue;
            }
            if (!(clip_or & CLIP_NEAR)) {
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
                _nj_draw_polygon_ += corner_count;
                for (corner = 0; corner < corner_count - 1; corner++) {
                    v0 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_UNLIT_TEX_NEXT(v0, corner_uv, KM_VERTEXPARAM_NORMAL);
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_UNLIT_TEX_NEXT(v0, corner_uv, KM_VERTEXPARAM_ENDOFSTRIP);
            } else {
                clip_cull_requested = 0;
                v0 = WORK_VERTEX(*mesh_cursor++);
                v1 = WORK_VERTEX(*mesh_cursor++);
                corner_uv_window[0].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[0].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                corner_uv_window[1].tex.u = (Sint16)(Float)corner_uv->tex.u;
                corner_uv_window[1].tex.v = (Sint16)(Float)corner_uv->tex.v;
                corner_uv++;
                for (corner = 0; corner < corner_count - 2; corner++) {
                    v2 = WORK_VERTEX(*mesh_cursor++);
                    corner_uv_window[2].tex.u = (Sint16)(Float)corner_uv->tex.u;
                    corner_uv_window[2].tex.v = (Sint16)(Float)corner_uv->tex.v;
                    corner_uv++;
                    clip_and = v0->clip & v1->clip & v2->clip;
                    if (!(clip_and & CLIP_NEAR)) {
                        simple_clip_unlit_tex_triangle(v0, v1, v2,
                                                       corner_uv_window);
                    }
                    corner_uv_window[0] = corner_uv_window[1];
                    corner_uv_window[1] = corner_uv_window[2];
                    v0 = v1;
                    v1 = v2;
                }
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        _njm_mesh = meshset;
        NJM_SET_TEX_X_STRIP_START();
        simple_draw_unlit_tex_strips_near(_njm_mesh);
        NJM_SET_TEX_X_STRIP_END();
        break;
    }
}

#define EMIT_FLAT_ENV_POSITION(vertex, command)                                \
    {                                                                          \
        sq_cursor += 6;                                                        \
        *--sq_cursor = (ENV_COLUMN_DOT((vertex)->normal, 1) * 0.5F) + 0.5F;    \
        *--sq_cursor = (ENV_COLUMN_DOT((vertex)->normal, 0) * 0.5F) + 0.5F;    \
        *--sq_cursor = (vertex)->oow;                                          \
        *--sq_cursor = (vertex)->sy;                                           \
        *--sq_cursor = (vertex)->sx;                                           \
        *(Uint32 *)--sq_cursor = command;                                      \
        prefetch(sq_cursor);                                                   \
        sq_cursor += VERTEX_WORDS;                                             \
    }

#define EMIT_FLAT_ENV_INTENSITY(vertex, command, intensity)                    \
    {                                                                          \
        sq_cursor += 7;                                                        \
        *--sq_cursor = (intensity);                                            \
        *--sq_cursor = (ENV_COLUMN_DOT((vertex)->normal, 1) * 0.5F) + 0.5F;    \
        *--sq_cursor = (ENV_COLUMN_DOT((vertex)->normal, 0) * 0.5F) + 0.5F;    \
        *--sq_cursor = (vertex)->oow;                                          \
        *--sq_cursor = (vertex)->sy;                                           \
        *--sq_cursor = (vertex)->sx;                                           \
        *(Uint32 *)--sq_cursor = command;                                      \
        prefetch(sq_cursor);                                                   \
        sq_cursor += VERTEX_WORDS;                                             \
    }

static void simple_draw_flat_env_polygons(NJS_MESHSET *meshset,
                                          NJS_MATERIAL *material)
{
    register Sint16 *mesh_cursor;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register struct calc_vertex *v3;
    Sint32 polygon;
    Sint32 corner;
    Sint32 corner_count;
    register Float facing;
    NJS_VECTOR flat_normal;
    register Float flat_intensity;

    mesh_cursor = meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                continue;
            }
            flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
            flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
            flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
            njUnitVector(&flat_normal);
            flat_intensity = SIMPLE_INTENSITY(flat_normal);
            EMIT_FLAT_ENV_POSITION(v0, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_ENV_POSITION(v1, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_ENV_INTENSITY(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                                    flat_intensity);
            _nj_draw_polygon_ += 3;
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                continue;
            }
            flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
            flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
            flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
            njUnitVector(&flat_normal);
            flat_intensity = SIMPLE_INTENSITY(flat_normal);
            EMIT_FLAT_ENV_POSITION(v0, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_ENV_POSITION(v1, KM_VERTEXPARAM_NORMAL);
            EMIT_FLAT_ENV_INTENSITY(v2, KM_VERTEXPARAM_NORMAL, flat_intensity);
            EMIT_FLAT_ENV_INTENSITY(v3, KM_VERTEXPARAM_ENDOFSTRIP,
                                    flat_intensity);
            _nj_draw_polygon_ += 4;
        }
        break;
    case NJD_MESHSET_N:
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            corner_count = (Uint16)*mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            v0 = WORK_VERTEX(mesh_cursor[0]);
            v1 = WORK_VERTEX(mesh_cursor[1]);
            v2 = WORK_VERTEX(mesh_cursor[2]);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                mesh_cursor += corner_count;
                continue;
            }
            _nj_draw_polygon_ += corner_count;
            flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
            flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
            flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
            njUnitVector(&flat_normal);
            flat_intensity = SIMPLE_INTENSITY(flat_normal);
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_ENV_POSITION(v0, KM_VERTEXPARAM_NORMAL);
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_ENV_POSITION(v0, KM_VERTEXPARAM_NORMAL);
            for (corner = 2; corner < corner_count - 1; corner++) {
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_ENV_INTENSITY(v0, KM_VERTEXPARAM_NORMAL,
                                        flat_intensity);
            }
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_ENV_INTENSITY(v0, KM_VERTEXPARAM_ENDOFSTRIP,
                                    flat_intensity);
        }
        break;
    case NJD_MESHSET_TRIMESH:
        _njm_mesh = meshset;
        NJM_SET_ENV_STRIP_START();
        simple_draw_flat_path_env_strips_cull(_njm_mesh);
        NJM_SET_ENV_STRIP_END();
        break;
    }
}

static void simple_draw_flat_env_polygons_clip(NJS_MESHSET *meshset,
                                               NJS_MATERIAL *material)
{
    register Sint16 *mesh_cursor;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register struct calc_vertex *v3;
    Sint32 polygon;
    Sint32 corner;
    Sint32 corner_count;
    Uint32 clip_and;
    Uint32 clip_or;
    Float facing;
    Float v1_x;
    NJS_VECTOR flat_normal;
    struct calc_vertex *v0_copy;
    Float flat_intensity;

    mesh_cursor = meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            /* @hack spelling: v0's normal read through v0_copy, a copy of v0,
             * which keeps v0 in R14 and reads its clip flags before forming the
             * other two corners' addresses (0x8c638fd8 to 0x8c638fec); read
             * through v0, 110 of 1399 words differ */
            v0_copy = v0;
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            clip_and = v0->clip & v1->clip & v2->clip;
            if (clip_and & CLIP_ALL) {
                continue;
            }
            clip_or = v0->clip | v1->clip | v2->clip;
            if (!(clip_or & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    continue;
                }
                flat_normal.x = v0_copy->normal.x + v1->normal.x + v2->normal.x;
                flat_normal.y = v0_copy->normal.y + v1->normal.y + v2->normal.y;
                flat_normal.z = v0_copy->normal.z + v1->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                EMIT_FLAT_ENV_POSITION(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_ENV_POSITION(v1, KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_ENV_INTENSITY(v2, KM_VERTEXPARAM_ENDOFSTRIP,
                                        flat_intensity);
                _nj_draw_polygon_ += 3;
            } else {
                clip_cull_requested = 1;
                flat_normal.x = v0_copy->normal.x + v1->normal.x + v2->normal.x;
                flat_normal.y = v0_copy->normal.y + v1->normal.y + v2->normal.y;
                flat_normal.z = v0_copy->normal.z + v1->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                simple_clip_flat_env_triangle(v0, v1, v2, flat_intensity);
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            clip_and = v0->clip & v1->clip & v2->clip & v3->clip;
            if (clip_and & CLIP_ALL) {
                continue;
            }
            clip_or = v0->clip | v1->clip | v2->clip | v3->clip;
            if (!(clip_or & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    continue;
                }
                flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
                flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
                flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                EMIT_FLAT_ENV_POSITION(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_ENV_POSITION(v1, KM_VERTEXPARAM_NORMAL);
                EMIT_FLAT_ENV_INTENSITY(v2, KM_VERTEXPARAM_NORMAL,
                                        flat_intensity);
                EMIT_FLAT_ENV_INTENSITY(v3, KM_VERTEXPARAM_ENDOFSTRIP,
                                        flat_intensity);
                _nj_draw_polygon_ += 4;
            } else {
                clip_cull_requested = 1;
                flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
                flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
                flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                clip_and = v0->clip & v1->clip & v2->clip;
                if (!(clip_and & CLIP_NEAR)) {
                    simple_clip_flat_env_triangle(v0, v1, v2, flat_intensity);
                }
                clip_and = v1->clip & v2->clip & v3->clip;
                if (!(clip_and & CLIP_NEAR)) {
                    simple_clip_flat_env_triangle(v1, v2, v3, flat_intensity);
                }
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            corner_count = (Uint16)*mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            clip_and = ~(Uint32)0;
            clip_or = 0;
            for (corner = 0; corner < corner_count; corner++) {
                clip_and &= WORK_VERTEX(mesh_cursor[corner])->clip;
                clip_or |= WORK_VERTEX(mesh_cursor[corner])->clip;
            }
            if (clip_and & CLIP_ALL) {
                mesh_cursor += corner_count;
                continue;
            }
            if (!(clip_or & CLIP_NEAR)) {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);
                v1_x = v1->sx;
                facing = ((v1_x - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1_x));
                if (facing > 0.0F) {
                    mesh_cursor += corner_count;
                    continue;
                }
                _nj_draw_polygon_ += corner_count;
                flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
                flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
                flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                v3 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_ENV_POSITION(v3, KM_VERTEXPARAM_NORMAL);
                v3 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_ENV_POSITION(v3, KM_VERTEXPARAM_NORMAL);
                for (corner = 2; corner < corner_count - 1; corner++) {
                    v3 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_FLAT_ENV_INTENSITY(v3, KM_VERTEXPARAM_NORMAL,
                                            flat_intensity);
                }
                v3 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_ENV_INTENSITY(v3, KM_VERTEXPARAM_ENDOFSTRIP,
                                        flat_intensity);
            } else {
                clip_cull_requested = 0;

                /* @bug uninitialized: this branch lights an N-gon that crosses
                 * the near plane from v0, v1 and v2 before it sets them,
                 * reading their normals at 0x8c63998a, 0x8c63998c and
                 * 0x8c639990. they hold corners of the meshset's last earlier
                 * N-gon whose corners share no clip flag. with none, they are
                 * R12, R14 and R13 as _njSimpleDrawModelSub left them: the
                 * meshset, _nj_light_material_'s address and the material */
                flat_normal.x = v0->normal.x + v1->normal.x + v2->normal.x;
                flat_normal.y = v0->normal.y + v1->normal.y + v2->normal.y;
                flat_normal.z = v0->normal.z + v1->normal.z + v2->normal.z;
                njUnitVector(&flat_normal);
                flat_intensity = SIMPLE_INTENSITY(flat_normal);
                v0 = WORK_VERTEX(*mesh_cursor++);
                v1 = WORK_VERTEX(*mesh_cursor++);
                corner_count -= 2;
                for (corner = 0; corner < corner_count; corner++) {
                    v2 = WORK_VERTEX(*mesh_cursor++);
                    clip_and = v0->clip & v1->clip & v2->clip;
                    if (!(clip_and & CLIP_NEAR)) {
                        simple_clip_flat_env_triangle(v0, v1, v2,
                                                      flat_intensity);
                    }
                    v0 = v1;
                    v1 = v2;
                }
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        _njm_mesh = meshset;
        NJM_SET_ENV_STRIP_START();
        simple_draw_flat_path_env_strips_near(_njm_mesh);
        NJM_SET_ENV_STRIP_END();
        break;
    }
}

static void simple_draw_unlit_env_polygons(NJS_MESHSET *meshset,
                                           NJS_MATERIAL *material)
{
    register Sint16 *mesh_cursor;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register struct calc_vertex *v3;
    Sint32 polygon;
    Sint32 corner;
    Sint32 corner_count;
    register Float facing;

    mesh_cursor = meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                continue;
            }
            EMIT_FLAT_ENV_INTENSITY(v0, KM_VERTEXPARAM_NORMAL, 1.0F);
            EMIT_FLAT_ENV_INTENSITY(v1, KM_VERTEXPARAM_NORMAL, 1.0F);
            EMIT_FLAT_ENV_INTENSITY(v2, KM_VERTEXPARAM_ENDOFSTRIP, 1.0F);
            _nj_draw_polygon_ += 3;
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                continue;
            }
            EMIT_FLAT_ENV_INTENSITY(v0, KM_VERTEXPARAM_NORMAL, 1.0F);
            EMIT_FLAT_ENV_INTENSITY(v1, KM_VERTEXPARAM_NORMAL, 1.0F);
            EMIT_FLAT_ENV_INTENSITY(v2, KM_VERTEXPARAM_NORMAL, 1.0F);
            EMIT_FLAT_ENV_INTENSITY(v3, KM_VERTEXPARAM_ENDOFSTRIP, 1.0F);
            _nj_draw_polygon_ += 4;
        }
        break;
    case NJD_MESHSET_N:
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            corner_count = (Uint16)*mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            v0 = WORK_VERTEX(mesh_cursor[0]);
            v1 = WORK_VERTEX(mesh_cursor[1]);
            v2 = WORK_VERTEX(mesh_cursor[2]);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                mesh_cursor += corner_count;
                continue;
            }
            _nj_draw_polygon_ += corner_count;
            for (corner = 0; corner < corner_count - 1; corner++) {
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_ENV_INTENSITY(v0, KM_VERTEXPARAM_NORMAL, 1.0F);
            }
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_FLAT_ENV_INTENSITY(v0, KM_VERTEXPARAM_ENDOFSTRIP, 1.0F);
        }
        break;
    case NJD_MESHSET_TRIMESH:
        _njm_mesh = meshset;
        NJM_SET_ENV_X_STRIP_START();
        simple_draw_unlit_env_strips_cull(_njm_mesh);
        NJM_SET_ENV_X_STRIP_END();
        break;
    }
}

static void simple_draw_unlit_env_polygons_clip(NJS_MESHSET *meshset,
                                                NJS_MATERIAL *material)
{
    register Sint16 *mesh_cursor;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register struct calc_vertex *v3;
    Sint32 polygon;
    Sint32 corner;
    Sint32 corner_count;
    Uint32 clip_and;
    Uint32 clip_or;
    register Float facing;

    mesh_cursor = meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            clip_and = v0->clip & v1->clip & v2->clip;
            if (clip_and & CLIP_ALL) {
                continue;
            }
            clip_or = v0->clip | v1->clip | v2->clip;
            if (!(clip_or & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    continue;
                }
                EMIT_FLAT_ENV_INTENSITY(v0, KM_VERTEXPARAM_NORMAL, 1.0F);
                EMIT_FLAT_ENV_INTENSITY(v1, KM_VERTEXPARAM_NORMAL, 1.0F);
                EMIT_FLAT_ENV_INTENSITY(v2, KM_VERTEXPARAM_ENDOFSTRIP, 1.0F);
                _nj_draw_polygon_ += 3;
            } else {
                clip_cull_requested = 1;
                simple_clip_unlit_env_triangle(v0, v1, v2);
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            clip_and = v0->clip & v1->clip & v2->clip & v3->clip;
            if (clip_and & CLIP_ALL) {
                continue;
            }
            clip_or = v0->clip | v1->clip | v2->clip | v3->clip;
            if (!(clip_or & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    continue;
                }
                EMIT_FLAT_ENV_INTENSITY(v0, KM_VERTEXPARAM_NORMAL, 1.0F);
                EMIT_FLAT_ENV_INTENSITY(v1, KM_VERTEXPARAM_NORMAL, 1.0F);
                EMIT_FLAT_ENV_INTENSITY(v2, KM_VERTEXPARAM_NORMAL, 1.0F);
                EMIT_FLAT_ENV_INTENSITY(v3, KM_VERTEXPARAM_ENDOFSTRIP, 1.0F);
                _nj_draw_polygon_ += 4;
            } else {
                clip_cull_requested = 1;
                clip_and = v0->clip & v1->clip & v2->clip;
                if (!(clip_and & CLIP_NEAR)) {
                    simple_clip_unlit_env_triangle(v0, v1, v2);
                }
                clip_and = v1->clip & v2->clip & v3->clip;
                if (!(clip_and & CLIP_NEAR)) {
                    simple_clip_unlit_env_triangle(v1, v2, v3);
                }
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            corner_count = (Uint16)*mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            clip_and = (Uint32)-1;
            clip_or = 0;
            for (corner = 0; corner < corner_count; corner++) {
                clip_and &= WORK_VERTEX(mesh_cursor[corner])->clip;
                clip_or |= WORK_VERTEX(mesh_cursor[corner])->clip;
            }
            if (clip_and & CLIP_ALL) {
                mesh_cursor += corner_count;
                continue;
            }
            if (!(clip_or & CLIP_NEAR)) {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    mesh_cursor += corner_count;
                    continue;
                }
                _nj_draw_polygon_ += corner_count;
                for (corner = 0; corner < corner_count - 1; corner++) {
                    v0 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_FLAT_ENV_INTENSITY(v0, KM_VERTEXPARAM_NORMAL, 1.0F);
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_FLAT_ENV_INTENSITY(v0, KM_VERTEXPARAM_ENDOFSTRIP, 1.0F);
            } else {
                clip_cull_requested = 0;
                v0 = WORK_VERTEX(*mesh_cursor++);
                v1 = WORK_VERTEX(*mesh_cursor++);
                for (corner = 0; corner < corner_count - 2; corner++) {
                    v2 = WORK_VERTEX(*mesh_cursor++);
                    clip_and = v0->clip & v1->clip & v2->clip;
                    if (!(clip_and & CLIP_NEAR)) {
                        simple_clip_unlit_env_triangle(v0, v1, v2);
                    }
                    v0 = v1;
                    v1 = v2;
                }
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        _njm_mesh = meshset;
        NJM_SET_ENV_X_STRIP_START();
        simple_draw_unlit_env_strips_near(_njm_mesh);
        NJM_SET_ENV_X_STRIP_END();
        break;
    }
}

static void simple_draw_unlit_polygons(NJS_MESHSET *meshset,
                                       NJS_MATERIAL *material)
{
    register Sint16 *mesh_cursor;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register struct calc_vertex *v3;
    Sint32 polygon;
    Sint32 corner;
    Sint32 corner_count;
    register Float facing;

    mesh_cursor = meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                continue;
            }
            EMIT_UNLIT_COLOR(v0, KM_VERTEXPARAM_NORMAL);
            EMIT_UNLIT_COLOR(v1, KM_VERTEXPARAM_NORMAL);
            EMIT_UNLIT_COLOR(v2, KM_VERTEXPARAM_ENDOFSTRIP);
            _nj_draw_polygon_ += 3;
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                continue;
            }
            EMIT_UNLIT_COLOR(v0, KM_VERTEXPARAM_NORMAL);
            EMIT_UNLIT_COLOR(v1, KM_VERTEXPARAM_NORMAL);
            EMIT_UNLIT_COLOR(v2, KM_VERTEXPARAM_NORMAL);
            EMIT_UNLIT_COLOR(v3, KM_VERTEXPARAM_ENDOFSTRIP);
            _nj_draw_polygon_ += 4;
        }
        break;
    case NJD_MESHSET_N:
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            corner_count = (Uint16)*mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            v0 = WORK_VERTEX(mesh_cursor[0]);
            v1 = WORK_VERTEX(mesh_cursor[1]);
            v2 = WORK_VERTEX(mesh_cursor[2]);
            facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                     ((v1->sy - v0->sy) * (v2->sx - v1->sx));
            if (facing > 0.0F) {
                mesh_cursor += corner_count;
                continue;
            }
            _nj_draw_polygon_ += corner_count;
            for (corner = 0; corner < corner_count - 1; corner++) {
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_UNLIT_COLOR(v0, KM_VERTEXPARAM_NORMAL);
            }
            v0 = WORK_VERTEX(*mesh_cursor++);
            EMIT_UNLIT_COLOR(v0, KM_VERTEXPARAM_ENDOFSTRIP);
        }
        break;
    case NJD_MESHSET_TRIMESH:
        _njm_mesh = meshset;
        NJM_SET_POLY_X_STRIP_START();
        simple_draw_unlit_strips_cull(_njm_mesh);
        NJM_SET_POLY_X_STRIP_END();
        break;
    }
}

static void simple_draw_unlit_polygons_clip(NJS_MESHSET *meshset,
                                            NJS_MATERIAL *material)
{
    register Sint16 *mesh_cursor;
    register struct calc_vertex *v0;
    register struct calc_vertex *v1;
    register struct calc_vertex *v2;
    register struct calc_vertex *v3;
    Sint32 polygon;
    Sint32 corner;
    Sint32 corner_count;
    Uint32 clip_and;
    Uint32 clip_or;
    register Float facing;

    mesh_cursor = meshset->meshes;
    switch (meshset->type_matId & NJD_MESHSET_MASK) {
    case NJD_MESHSET_3:
        _nj_calc_polygon_ += meshset->nbMesh * 3;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            clip_and = v0->clip & v1->clip & v2->clip;
            if (clip_and & CLIP_ALL) {
                continue;
            }
            clip_or = v0->clip | v1->clip | v2->clip;
            if (!(clip_or & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    continue;
                }
                EMIT_UNLIT_COLOR(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_COLOR(v1, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_COLOR(v2, KM_VERTEXPARAM_ENDOFSTRIP);
                _nj_draw_polygon_ += 3;
            } else {
                clip_cull_requested = 1;
                simple_clip_unlit_triangle(v0, v1, v2);
            }
        }
        break;
    case NJD_MESHSET_4:
        _nj_calc_polygon_ += meshset->nbMesh * 4;
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            v0 = WORK_VERTEX(*mesh_cursor++);
            v1 = WORK_VERTEX(*mesh_cursor++);
            v2 = WORK_VERTEX(*mesh_cursor++);
            v3 = WORK_VERTEX(*mesh_cursor++);
            clip_and = v0->clip & v1->clip & v2->clip & v3->clip;
            if (clip_and & CLIP_ALL) {
                continue;
            }
            clip_or = v0->clip | v1->clip | v2->clip | v3->clip;
            if (!(clip_or & CLIP_NEAR)) {
                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    continue;
                }
                EMIT_UNLIT_COLOR(v0, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_COLOR(v1, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_COLOR(v2, KM_VERTEXPARAM_NORMAL);
                EMIT_UNLIT_COLOR(v3, KM_VERTEXPARAM_ENDOFSTRIP);
                _nj_draw_polygon_ += 4;
            } else {
                clip_cull_requested = 1;
                clip_and = v0->clip & v1->clip & v2->clip;
                if (!(clip_and & CLIP_NEAR)) {
                    simple_clip_unlit_triangle(v0, v1, v2);
                }
                clip_and = v1->clip & v2->clip & v3->clip;
                if (!(clip_and & CLIP_NEAR)) {
                    simple_clip_unlit_triangle(v1, v2, v3);
                }
            }
        }
        break;
    case NJD_MESHSET_N:
        for (polygon = 0; polygon < meshset->nbMesh; polygon++) {
            corner_count = (Uint16)*mesh_cursor++;
            _nj_calc_polygon_ += corner_count;
            clip_and = ~(Uint32)0;
            clip_or = 0;
            for (corner = 0; corner < corner_count; corner++) {
                clip_and &= WORK_VERTEX(mesh_cursor[corner])->clip;
                clip_or |= WORK_VERTEX(mesh_cursor[corner])->clip;
            }
            if (clip_and & CLIP_ALL) {
                mesh_cursor += corner_count;
                continue;
            }
            if (!(clip_or & CLIP_NEAR)) {
                v0 = WORK_VERTEX(mesh_cursor[0]);
                v1 = WORK_VERTEX(mesh_cursor[1]);
                v2 = WORK_VERTEX(mesh_cursor[2]);

                facing = ((v1->sx - v0->sx) * (v2->sy - v1->sy)) -
                         ((v1->sy - v0->sy) * (v2->sx - v1->sx));
                if (facing > 0.0F) {
                    mesh_cursor += corner_count;
                    continue;
                }
                _nj_draw_polygon_ += corner_count;
                for (corner = 0; corner < corner_count - 1; corner++) {
                    v0 = WORK_VERTEX(*mesh_cursor++);
                    EMIT_UNLIT_COLOR(v0, KM_VERTEXPARAM_NORMAL);
                }
                v0 = WORK_VERTEX(*mesh_cursor++);
                EMIT_UNLIT_COLOR(v0, KM_VERTEXPARAM_ENDOFSTRIP);
            } else {
                clip_cull_requested = 0;
                v0 = WORK_VERTEX(*mesh_cursor++);
                v1 = WORK_VERTEX(*mesh_cursor++);
                for (corner = 0; corner < corner_count - 2; corner++) {
                    v2 = WORK_VERTEX(*mesh_cursor++);
                    clip_and = v0->clip & v1->clip & v2->clip;
                    if (!(clip_and & CLIP_NEAR)) {
                        simple_clip_unlit_triangle(v0, v1, v2);
                    }
                    v0 = v1;
                    v1 = v2;
                }
            }
        }
        break;
    case NJD_MESHSET_TRIMESH:
        _njm_mesh = meshset;
        NJM_SET_POLY_X_STRIP_START();
        simple_draw_unlit_strips_near(_njm_mesh);
        NJM_SET_POLY_X_STRIP_END();
        break;
    }
}

/* the factor that takes a material's color byte, 0 to 255, to a Float from 0
 * to 1 */
#define COLOR_SCALE (1.0F / 255.0F)

#pragma inline(set_material_specular)

static void set_material_specular(NJS_MATERIAL *material)
{
    Float channel_scale = COLOR_SCALE;

    _nj_light_material_.specular.r =
        (Float)material->specular.argb.r * channel_scale;
    _nj_light_material_.specular.g =
        (Float)material->specular.argb.g * channel_scale;
    _nj_light_material_.specular.b =
        (Float)material->specular.argb.b * channel_scale;
}

/* draw model as njSimpleDrawModelSub does, inside the caller's own save and
 * restore of R8 */
void _njSimpleDrawModelSub(NJS_MODEL *model)
{
    NJS_MESHSET *meshset;
    NJS_MATERIAL *material;
    Uint16 material_id;
    Uint16 previous_material;
    Uint32 drawer_flags;
    Sint32 mesh_index;

    if (model->nbPoint > _nj_vertex_num_) {
        return;
    }
    meshset = model->meshsets;

    if (njSimpleCalcModel(model)) {
        return;
    }
    for (mesh_index = 0; mesh_index < model->nbMeshset; mesh_index++) {
        material_id = meshset->type_matId & MESHSET_MATERIAL;
        if (mesh_index == 0 || material_id != previous_material) {
            material = &model->mats[material_id];
            material_flags = material->attrflags;
            if (_nj_control_3d_flag_ & NJD_CONTROL_3D_CONSTANT_ATTR) {
                material_flags &= _nj_constant_attr_and_;
                material_flags |= _nj_constant_attr_or_;
            }
            if (material_flags & NJD_FLAG_USE_TEXTURE) {
                njSetkmTextureNum(material->attr_texId & MATERIAL_TEXTURE);

                if (_nj_control_3d_flag_ &
                    NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL) {
                    _nj_light_material_.diffuse.a =
                        (Float)material->diffuse.argb.a * COLOR_SCALE;
                    _nj_light_material_.diffuse.r = 1.0F;
                    _nj_light_material_.diffuse.g = 1.0F;
                    _nj_light_material_.diffuse.b = 1.0F;
                } else {
                    Float channel_scale = COLOR_SCALE;

                    _nj_light_material_.diffuse.r =
                        (Float)material->diffuse.argb.r * channel_scale;
                    _nj_light_material_.diffuse.g =
                        (Float)material->diffuse.argb.g * channel_scale;
                    _nj_light_material_.diffuse.b =
                        (Float)material->diffuse.argb.b * channel_scale;
                    _nj_light_material_.diffuse.a =
                        (Float)material->diffuse.argb.a * channel_scale;
                }
            } else {
                Float channel_scale = COLOR_SCALE;

                _nj_light_material_.diffuse.r =
                    (Float)material->diffuse.argb.r * channel_scale;
                _nj_light_material_.diffuse.g =
                    (Float)material->diffuse.argb.g * channel_scale;
                _nj_light_material_.diffuse.b =
                    (Float)material->diffuse.argb.b * channel_scale;
                _nj_light_material_.diffuse.a =
                    (Float)material->diffuse.argb.a * channel_scale;
            }

            if (_nj_control_3d_flag_ & NJD_CONTROL_3D_CONSTANT_MATERIAL) {
                _nj_light_material_.diffuse.a = _nj_constant_material_.a;
                _nj_light_material_.diffuse.r = _nj_constant_material_.r;
                _nj_light_material_.diffuse.g = _nj_constant_material_.g;
                _nj_light_material_.diffuse.b = _nj_constant_material_.b;
            } else if (_nj_control_3d_flag_ & NJD_CONTROL_3D_OFFSET_MATERIAL) {
                _nj_light_material_.diffuse.a += _nj_constant_material_.a;
                _nj_light_material_.diffuse.r += _nj_constant_material_.r;
                _nj_light_material_.diffuse.g += _nj_constant_material_.g;
                _nj_light_material_.diffuse.b += _nj_constant_material_.b;
            }
            set_material_specular(material);
            _nj_light_material_.exponent = material->exponent;
            drawer_flags = material_flags & DRAWER_FLAGS;
            njSetkmModelAttr(material_flags);
            if (material_flags & NJD_FLAG_USE_TEXTURE) {
                if (material_flags & NJD_FLAG_IGNORE_LIGHT) {
                    if (material_flags & NJD_FLAG_USE_ALPHA) {
                        njSetkmColorMode(MODE_TEX_FLAT_TRANS);
                        sq_cursor = (Float *)njStartVertexTexTransIntens(
                            (Uint32 *)&_nj_light_material_.diffuse);
                    } else {
                        njSetkmColorMode(MODE_TEX_FLAT_OPAQUE);
                        sq_cursor = (Float *)njStartVertexTexOpaqueIntens(
                            (Uint32 *)&_nj_light_material_.diffuse);
                    }
                } else if (material_flags & NJD_FLAG_USE_FLAT) {
                    if (material_flags & NJD_FLAG_USE_ALPHA) {
                        njSetkmColorMode(MODE_TEX_FLAT_TRANS);
                        sq_cursor = (Float *)njStartVertexTexTransIntens(
                            (Uint32 *)&_nj_light_material_.diffuse);
                    } else {
                        njSetkmColorMode(MODE_TEX_FLAT_OPAQUE);
                        sq_cursor = (Float *)njStartVertexTexOpaqueIntens(
                            (Uint32 *)&_nj_light_material_.diffuse);
                    }
                } else {
                    if (material_flags & NJD_FLAG_USE_ALPHA) {
                        njSetkmColorMode(MODE_TEX_TRANS);
                        sq_cursor = (Float *)njStartVertexTexTransIntens(
                            (Uint32 *)&_nj_light_material_.diffuse);
                    } else {
                        njSetkmColorMode(MODE_TEX_OPAQUE);
                        sq_cursor = (Float *)njStartVertexTexOpaqueIntens(
                            (Uint32 *)&_nj_light_material_.diffuse);
                    }
                }
            } else {
                if (material_flags & NJD_FLAG_IGNORE_LIGHT) {
                    if (material_flags & NJD_FLAG_USE_ALPHA) {
                        njSetkmColorMode(MODE_FLAT_TRANS);
                        sq_cursor = (Float *)njStartVertexNonTexTrans();
                    } else {
                        njSetkmColorMode(MODE_FLAT_OPAQUE);
                        sq_cursor = (Float *)njStartVertexNonTexOpaque();
                    }
                } else if (material_flags & NJD_FLAG_USE_FLAT) {
                    if (material_flags & NJD_FLAG_USE_ALPHA) {
                        njSetkmColorMode(MODE_FLAT_TRANS);
                        sq_cursor = (Float *)njStartVertexNonTexTrans();
                    } else {
                        njSetkmColorMode(MODE_FLAT_OPAQUE);
                        sq_cursor = (Float *)njStartVertexNonTexOpaque();
                    }
                } else {
                    if (material_flags & NJD_FLAG_USE_ALPHA) {
                        njSetkmColorMode(NJD_COLOR_MODE_NORMAL_TRANS);
                        sq_cursor = (Float *)njStartVertexNonTexTrans();
                    } else {
                        njSetkmColorMode(NJD_COLOR_MODE_NORMAL);
                        sq_cursor = (Float *)njStartVertexNonTexOpaque();
                    }
                }
            }
            previous_material = material_id;
            /* @hack redundant: an empty else, whose jump makes the clip test
             * load model_clip_or into R2 (MOV.L @R4,R2 at 0x8c63b6b2); without
             * it, the load takes R3 and 2 of 542 words differ */
        } else {
        }

/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c63b656, the pool at 0x8c63b65c). without it, 230 of 542 words differ and
 * the function is 28 bytes longer */
#pragma asm
        ;
#pragma endasm

        if (model_clip_or == 0) {
            switch (drawer_flags) {
            case 0:
                simple_draw_smooth_polygons(meshset, material);
                break;
            case NJD_FLAG_USE_TEXTURE:
                simple_draw_smooth_tex_polygons(meshset, material);
                break;
            case NJD_FLAG_USE_TEXTURE | NJD_FLAG_USE_ENV:
                simple_draw_smooth_env_polygons(meshset, material);
                break;
            case NJD_FLAG_USE_FLAT:
                simple_draw_flat_polygons(meshset, material);
                break;
            case NJD_FLAG_USE_FLAT | NJD_FLAG_USE_TEXTURE:
                simple_draw_flat_tex_polygons(meshset, material);
                break;
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_TEXTURE:
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_FLAT |
                NJD_FLAG_USE_TEXTURE:
                simple_draw_unlit_tex_polygons(meshset, material);
                break;
            case NJD_FLAG_USE_FLAT | NJD_FLAG_USE_TEXTURE | NJD_FLAG_USE_ENV:
                simple_draw_flat_env_polygons(meshset, material);
                break;
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_TEXTURE |
                NJD_FLAG_USE_ENV:
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_FLAT |
                NJD_FLAG_USE_TEXTURE | NJD_FLAG_USE_ENV:
                simple_draw_unlit_env_polygons(meshset, material);
                break;
            case NJD_FLAG_IGNORE_LIGHT:
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_FLAT:
                simple_draw_unlit_polygons(meshset, material);
                break;
            }
        } else {
            switch (drawer_flags) {
            case 0:
                simple_draw_smooth_polygons_clip(meshset, material);
                break;
            case NJD_FLAG_USE_TEXTURE:
                simple_draw_smooth_tex_polygons_clip(meshset, material);
                break;
            case NJD_FLAG_USE_TEXTURE | NJD_FLAG_USE_ENV:
                simple_draw_smooth_env_polygons_clip(meshset, material);
                break;
            case NJD_FLAG_USE_FLAT:
                simple_draw_flat_polygons_clip(meshset, material);
                break;
            case NJD_FLAG_USE_FLAT | NJD_FLAG_USE_TEXTURE:
                simple_draw_flat_tex_polygons_clip(meshset, material);
                break;
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_TEXTURE:
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_FLAT |
                NJD_FLAG_USE_TEXTURE:
                simple_draw_unlit_tex_polygons_clip(meshset, material);
                break;
            case NJD_FLAG_USE_FLAT | NJD_FLAG_USE_TEXTURE | NJD_FLAG_USE_ENV:
                simple_draw_flat_env_polygons_clip(meshset, material);
                break;
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_TEXTURE |
                NJD_FLAG_USE_ENV:
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_FLAT |
                NJD_FLAG_USE_TEXTURE | NJD_FLAG_USE_ENV:
                simple_draw_unlit_env_polygons_clip(meshset, material);
                break;
            case NJD_FLAG_IGNORE_LIGHT:
            case NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_FLAT:
                simple_draw_unlit_polygons_clip(meshset, material);
                break;
            /* @hack redundant: default: break, which makes the loop's test
             * reload mesh_index into R3 after the increment stores it from R2
             * (0x8c63b8a8 to 0x8c63b8b0); without it, the test takes the
             * increment's register, 23 of 542 words differ and the function is
             * 4 bytes longer */
            default:
                break;
            }
        }

/* @hack redundant: a #pragma asm block holding only an empty assembler comment,
 * at which SHC flushes the pending literal pool behind a BRA (the BRA at
 * 0x8c63b824, the pool, which holds the drawers' addresses, at 0x8c63b828).
 * without it, 139 of 542 words differ */
#pragma asm
        ;
#pragma endasm

        njEndVertex((Uint32 *)sq_cursor);
        meshset++;
    }
}

/* set base_light_direction from _nj_light_control_'s first light,
 * base_light_scale to its diffuse intensity and iamb to its ambient one. then
 * draw object as njSimpleDrawObjectSub does */
void _njSimpleDrawObject(NJS_OBJECT *object)
{
    NJS_MATRIX combined;
    NJS_LIGHT *base_light;
    Float *rotation;
    Float *light_matrix;
    Float *view_matrix;

    base_light = _nj_light_control_.light[0];
    rotation = combined;
    view_matrix = _nj_view_matrix_;
    light_matrix = base_light->mtrx;
    rotation[0] = (light_matrix[0] * view_matrix[0]) +
                  (light_matrix[1] * view_matrix[4]) +
                  (light_matrix[2] * view_matrix[8]);
    rotation[1] = (light_matrix[0] * view_matrix[1]) +
                  (light_matrix[1] * view_matrix[5]) +
                  (light_matrix[2] * view_matrix[9]);
    rotation[2] = (light_matrix[0] * view_matrix[2]) +
                  (light_matrix[1] * view_matrix[6]) +
                  (light_matrix[2] * view_matrix[10]);
    rotation[4] = (light_matrix[4] * view_matrix[0]) +
                  (light_matrix[5] * view_matrix[4]) +
                  (light_matrix[6] * view_matrix[8]);
    rotation[5] = (light_matrix[4] * view_matrix[1]) +
                  (light_matrix[5] * view_matrix[5]) +
                  (light_matrix[6] * view_matrix[9]);
    rotation[6] = (light_matrix[4] * view_matrix[2]) +
                  (light_matrix[5] * view_matrix[6]) +
                  (light_matrix[6] * view_matrix[10]);
    rotation[8] = (light_matrix[8] * view_matrix[0]) +
                  (light_matrix[9] * view_matrix[4]) +
                  (light_matrix[10] * view_matrix[8]);
    rotation[9] = (light_matrix[8] * view_matrix[1]) +
                  (light_matrix[9] * view_matrix[5]) +
                  (light_matrix[10] * view_matrix[9]);
    rotation[10] = (light_matrix[8] * view_matrix[2]) +
                   (light_matrix[9] * view_matrix[6]) +
                   (light_matrix[10] * view_matrix[10]);
    base_light_direction.x = (rotation[0] * base_light->vctr.x) +
                             (rotation[4] * base_light->vctr.y) +
                             (rotation[8] * base_light->vctr.z);
    base_light_direction.y = (rotation[1] * base_light->vctr.x) +
                             (rotation[5] * base_light->vctr.y) +
                             (rotation[9] * base_light->vctr.z);
    base_light_direction.z = (rotation[2] * base_light->vctr.x) +
                             (rotation[6] * base_light->vctr.y) +
                             (rotation[10] * base_light->vctr.z);
    base_light_scale = base_light->attr.idif;
    iamb = base_light->attr.iamb;
    njSimpleDrawObjectSub(object);
}

/* draw node's model, unless NJD_EVAL_HIDE, and then its children, unless
 * NJD_EVAL_BREAK. the transforms before it leave the current matrix in XMTRX,
 * and st_ext stores XMTRX to *mp, the copy in memory ENV_COLUMN_DOT reads */
#define DRAW_OBJECT_CONTENTS(node)                                             \
    {                                                                          \
        mp = _nj_current_matrix_ptr_;                                          \
        st_ext((Float(*)[4])mp);                                               \
        if (!((node)->evalflags & NJD_EVAL_HIDE)) {                            \
            njSimpleDrawModelSub((node)->model);                               \
        }                                                                      \
        if (!((node)->evalflags & NJD_EVAL_BREAK)) {                           \
            njSimpleDrawObjectSub((node)->child);                              \
        }                                                                      \
    }

#define APPLY_OBJECT_ROTATION(node)                                            \
    {                                                                          \
        if ((node)->evalflags & NJD_EVAL_ZXY_ANG) {                            \
            njRotateZXY(0, (node)->ang[0], (node)->ang[1], (node)->ang[2]);    \
        } else {                                                               \
            njRotateXYZ(0, (node)->ang[0], (node)->ang[1], (node)->ang[2]);    \
        }                                                                      \
    }

#define APPLY_OBJECT_TRANSLATION(node)                                         \
    njTranslate(0, (node)->pos[0], (node)->pos[1], (node)->pos[2])

#define APPLY_OBJECT_SCALE(node)                                               \
    njScale(0, (node)->scl[0], (node)->scl[1], (node)->scl[2])

/* draw object as njSimpleDrawObjectSub does, inside the caller's own save and
 * restore of R8 */
void _njSimpleDrawObjectSub(NJS_OBJECT *object)
{
    do {
        switch (object->evalflags & EVAL_UNIT) {
        case 0:
            njPushMatrix(0);
            APPLY_OBJECT_TRANSLATION(object);
            APPLY_OBJECT_ROTATION(object);
            APPLY_OBJECT_SCALE(object);
            DRAW_OBJECT_CONTENTS(object);
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_POS:
            njPushMatrix(0);
            APPLY_OBJECT_ROTATION(object);
            APPLY_OBJECT_SCALE(object);
            DRAW_OBJECT_CONTENTS(object);
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_ANG:
            njPushMatrix(0);
            APPLY_OBJECT_TRANSLATION(object);
            APPLY_OBJECT_SCALE(object);
            DRAW_OBJECT_CONTENTS(object);
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG:
            njPushMatrix(0);
            APPLY_OBJECT_SCALE(object);
            DRAW_OBJECT_CONTENTS(object);
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_SCL:
            njPushMatrix(0);
            APPLY_OBJECT_TRANSLATION(object);
            APPLY_OBJECT_ROTATION(object);
            DRAW_OBJECT_CONTENTS(object);
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_SCL:
            njPushMatrix(0);
            APPLY_OBJECT_ROTATION(object);
            DRAW_OBJECT_CONTENTS(object);
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL:
            njPushMatrix(0);
            APPLY_OBJECT_TRANSLATION(object);
            DRAW_OBJECT_CONTENTS(object);
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL:
            DRAW_OBJECT_CONTENTS(object);
            break;
        }
        object = object->sibling;
    } while (object);
}

/* clang-format off */
#ifndef __clang__
/* store R8, which holds sq_cursor, at *slot. in njSimpleDrawModel the expansion
 * is MOV.L R8,@R4 at 0x8c63bee2, ahead of the load of _njSimpleDrawModel's
 * address */
/* @asm open: no inspect rule reads R8 at its entry */
#pragma inline_asm(save_queue_cursor(size=2))
static void save_queue_cursor(Float **slot)
{
        MOV.L       R8,@R4
}

/* load R8 back from *slot. in njSimpleDrawModel the expansion is MOV.L @R4,R8
 * at 0x8c63beec, ahead of the load of njSetkmPointer's address, and the JSR's
 * delay slot at 0x8c63bef2 holds a NOP */
/* @asm open: no inspect rule reads the R8 it leaves its callers */
#pragma inline_asm(restore_queue_cursor(size=2))
static void restore_queue_cursor(Float **slot)
{
        MOV.L       @R4,R8
}
#endif
/* clang-format on */

/* R8 belongs to the caller under the calling convention, but sq_cursor takes it
 * in this file, so each entry point saves R8 around its work */
void njSimpleDrawModel(NJS_MODEL *model)
{
    Float *saved_cursor;

    njGetkmPointer();
    save_queue_cursor(&saved_cursor);
    _njSimpleDrawModel(model);
    restore_queue_cursor(&saved_cursor);
    njSetkmPointer();
}

Sint32 njSimpleCalcModel(NJS_MODEL *model)
{
    Float *saved_cursor;
    Sint32 result;

    save_queue_cursor(&saved_cursor);
    result = _njSimpleCalcModel(model);
    restore_queue_cursor(&saved_cursor);
    return result;
}

void njSimpleDrawModelSub(NJS_MODEL *model)
{
    Float *saved_cursor;

    save_queue_cursor(&saved_cursor);
    _njSimpleDrawModelSub(model);
    restore_queue_cursor(&saved_cursor);
}

void njSimpleDrawObject(NJS_OBJECT *object)
{
    Float *saved_cursor;

    njGetkmPointer();
    save_queue_cursor(&saved_cursor);
    _njSimpleDrawObject(object);
    restore_queue_cursor(&saved_cursor);
    njSetkmPointer();
}

void njSimpleDrawObjectSub(NJS_OBJECT *object)
{
    Float *saved_cursor;

    save_queue_cursor(&saved_cursor);
    _njSimpleDrawObjectSub(object);
    restore_queue_cursor(&saved_cursor);
}
