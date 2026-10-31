/* the easy draws, njEasyDrawModel and njEasyDrawObject, light each vertex
 * with one intensity, from one light. njEasyCalcModel projects and lights a
 * model's points. the drawers then send each meshset through the store queues:
 * strips whole, and triangles and quads culled by their winding on the screen.
 * when some point has z 0 or less, the near drawers send in their place, and
 * keep only the part of each polygon beyond z = 1 */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>

/* a strip's point count: the low 15 bits of its first word, whose top bit is
 * NJD_TRIMESH_END */
#define STRIP_COUNT 0x7fff
/* the low 14 bits of a meshset's type_matId: its material's index */
#define MESHSET_MATERIAL 0x3fff
/* the low 30 bits of a material's attr_texId: its texture's number in the
 * current texture list */
#define MATERIAL_TEXTURE 0x3fffffff
#define EVAL_UNIT (NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL)

#define MODE_TEX_OPAQUE                                                        \
    (COLOR_MODE_OPAQUE | COLOR_MODE_TEXTURE | COLOR_MODE_INTENSITY)
#define MODE_TEX_TRANS                                                         \
    (COLOR_MODE_TRANS | COLOR_MODE_TEXTURE | COLOR_MODE_INTENSITY)
#define MODE_OPAQUE (COLOR_MODE_OPAQUE | COLOR_MODE_INTENSITY)
#define MODE_TRANS (COLOR_MODE_TRANS | COLOR_MODE_INTENSITY)

/* the store queues. once QACR0 and QACR1 hold SQ_AREA(address), a queue
 * filled at SQ_BASE | (address & SQ_ADDRESS) is sent to address. SQ_AREA moves
 * address's bits 26 to 28 to bits 2 to 4, where the registers take them */
#define QACR0 (*(Uint32 *)0xff000038)
#define QACR1 (*(Uint32 *)0xff00003c)
#define SQ_AREA(address) (((address) >> 24) & 0x1c)
#define SQ_ADDRESS 0x03ffffff
#define SQ_BASE 0xe0000000

/* a model's texture coordinates are in units of 1/256 */
#define TEX_SCALE (1.0F / 256.0F)
/* the factor that takes a material's color byte, 0 to 255, to a Float from 0
 * to 255/256 */
#define COLOR_SCALE (1.0F / 256.0F)

/* a point as njEasyCalcModel projects and lights it */
struct projected_point {
    Float x, y; /* screen coordinates */
    Float reciprocal_z;
    Float intensity;
};

/* a vertex as the drawers write it: one store queue's 32 bytes. no drawer
 * writes unused, and PUT_SOLID leaves u and v as the queue held them */
struct vertex_packet {
    Uint32 pcw;
    Float x, y, reciprocal_z;
    Float u, v;
    Float intensity;
    Uint32 unused;
};

/* njEasyCalcModel's argument, whose fields its body reads in this order.
 * njSetEasyLight and njSetEasyLightIntensity set the light and the
 * projection, and _njEasyDrawModel the rest for each model */
struct calc_work {
    NJS_VECTOR light_direction;
    Float negative_diffuse;
    Sint32 point_count;
    /* where the points go, as a store queue address */
    struct projected_point *queue_points;
    NJS_POINT3 *points;
    NJS_VECTOR *normals;
    Float ambient;
    /* the screen x per unit of x / z, and the screen y per unit of y / z */
    Float projection_x, projection_y;
    Float center_x, center_y;
    /* queue_points' memory, as a cached address */
    struct projected_point *cache_points;
};

/* 20 bytes of bss that no word in 1ST_READ.BIN points into. they keep
 * packet_cursor and calc, the rest of the object's bss, at the image's
 * addresses */
static Uint8 dat_8c8c2514[20];
/* the store queue address of the next vertex the drawers write */
static struct vertex_packet *packet_cursor;
static struct calc_work calc;
/* the light's color, which njSetEasyLightColor sets, and in a, the alpha the
 * last translucent textured meshset took from its material. each textured
 * meshset's header takes all four floats, a included */
static NJS_ARGB light_tint = {1.0F, 1.0F, 1.0F, 1.0F};

/* SHC alone compiles the inline body below; clang sees only this declaration */
/* NOLINTNEXTLINE(clang-diagnostic-undefined-internal) */
static Sint32 easy_calc_body(struct calc_work *work);

/* clang-format off */
#ifndef __clang__
/* @asm opcode: cache-line, OCBI @R7 at 0x8c629992 and 0x8c6299a2 invalidates
 * the line of each two points written */
#pragma inline_asm(easy_calc_body)
static Sint32 easy_calc_body(struct calc_work *work)
{
        FMOV.S      FR15,@-R15
        FMOV.S      FR14,@-R15
        FMOV.S      FR13,@-R15
        FMOV.S      FR12,@-R15
        FMOV.S      @R4+,FR12
        FMOV.S      @R4+,FR13
        FMOV.S      @R4+,FR14
        FLDI0       FR15
        FSCHG
        FMOV.S      FR1,FR0
        FMOV.S      FR3,FR2
        FIPR        FV12,FV0
        FMOV.S      FR5,FR4
        FMOV.S      FR7,FR6
        FIPR        FV12,FV4
        FMOV.S      FR9,FR8
        FMOV.S      FR11,FR10
        FIPR        FV12,FV8
        FSCHG
        FMOV.S      FR3,FR12
        FMOV.S      FR7,FR13
        FMOV.S      FR11,FR14
        FMOV.S      @R4+,FR0
        MOV.L       @R4+,R1
        FMUL        FR0,FR12
        MOV.L       @R4+,R2
        FMUL        FR0,FR13
        MOV.L       @R4+,R3
        MOV.L       @R4+,R5
        PREF        @R3
        FMUL        FR0,FR14
        FMOV.S      @R4+,FR15
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR1
        FMOV.S      @R4+,FR2
        FMOV.S      @R4+,FR3
        MOV.L       @R4+,R7
        MOV         #0,R0
        MOV         #0,R6
        PREF        @R5
?point:
        FMOV.S      @R3+,FR4
        FMOV.S      @R3+,FR5
        FMOV.S      @R3+,FR6
        FLDI1       FR7
        FTRV        XMTRX,FV4
        ADD         #12,R3
        FMOV.S      @R5+,FR8
        FMOV.S      @R5+,FR9
        FMOV.S      @R5+,FR10
        FLDI1       FR11
        FIPR        FV12,FV8
        FLDI1       FR7
        FDIV        FR6,FR7
        PREF        @R3
        FCMP/GT     FR15,FR11
        ADD         #16,R2
        BT          ?lit
        FMOV.S      FR15,FR11
?lit:
        FMOV.S      FR11,@-R2
        ADD         #-12,R3
        FLDI0       FR11
        FCMP/GT     FR11,FR6
        BT          ?ahead
        ADD         #1,R6
?ahead:
        FMUL        FR1,FR5
        ADD         #12,R5
        FMUL        FR0,FR4
        PREF        @R5
        FMUL        FR7,FR5
        FMOV.S      FR7,@-R2
        FMUL        FR7,FR4
        ADD         #-12,R5
        FADD        FR3,FR5
        ADD         #1,R0
        FMOV.S      FR5,@-R2
        TST         #1,R0
        FADD        FR2,FR4
        BF/S        ?next
        FMOV.S      FR4,@-R2
        OCBI        @R7
        ADD         #32,R7
        PREF        @R2
?next:
        DT          R1
        BF/S        ?point
        ADD         #16,R2
        TST         #1,R0
        BT          ?done
        OCBI        @R7
        PREF        @R2
?done:
        FMOV.S      @R15+,FR12
        FMOV.S      @R15+,FR13
        FMOV.S      @R15+,FR14
        FMOV.S      @R15+,FR15
        RTS
        MOV         R6,R0
}
#endif
/* clang-format on */

/* project the work->point_count points at work->points through XMTRX into
 * work->queue_points: x and y on the screen, 1/z, and an intensity. the
 * intensity is work->ambient, plus work->negative_diffuse times the dot of the
 * point's normal in work->normals with work->light_direction, where that
 * product is positive. XMTRX's transpose takes the direction into model space.
 * the store queues write past the operand cache, so it invalidates the points'
 * lines at work->cache_points. it returns the number of points whose z is 0 or
 * less.
 *
 * work->point_count must be at least 1, and QACR0 and QACR1 must hold
 * SQ_AREA(work->cache_points). FPSCR.SZ and FPSCR.PR must be 0 */
Sint32 njEasyCalcModel(struct calc_work *work)
{
    return easy_calc_body(work);
}

/* PUT_SOLID and PUT_TEX write a vertex at queue, a Float pointer into a store
 * queue, from its intensity down to its PCW. each then sends the queue to
 * memory, and steps queue to the next vertex */
#define PUT_SOLID(queue, pcw, x, y, reciprocal_z, intensity)                   \
    {                                                                          \
        (queue) += 7;                                                          \
        *--(queue) = (intensity);                                              \
        (queue) -= 2;                                                          \
        *--(queue) = (reciprocal_z);                                           \
        *--(queue) = (y);                                                      \
        *--(queue) = (x);                                                      \
        --(queue);                                                             \
        *(Uint32 *)(queue) = (pcw);                                            \
        prefetch(queue);                                                       \
        (queue) += 8;                                                          \
    }
#define PUT_TEX(queue, pcw, x, y, reciprocal_z, u, v, intensity)               \
    {                                                                          \
        (queue) += 7;                                                          \
        *--(queue) = (intensity);                                              \
        *--(queue) = (v);                                                      \
        *--(queue) = (u);                                                      \
        *--(queue) = (reciprocal_z);                                           \
        *--(queue) = (y);                                                      \
        *--(queue) = (x);                                                      \
        --(queue);                                                             \
        *(Uint32 *)(queue) = (pcw);                                            \
        prefetch(queue);                                                       \
        (queue) += 8;                                                          \
    }

/* PUT_TEX with address, an integer, in place of queue */
#define PUT_TEX_AT(address, pcw, x, y, reciprocal_z, u, v, intensity)          \
    {                                                                          \
        (address) += sizeof(struct vertex_packet) - sizeof(Float);             \
        *(Float *)((address) -= sizeof(Float)) = (intensity);                  \
        *(Float *)((address) -= sizeof(Float)) = (v);                          \
        *(Float *)((address) -= sizeof(Float)) = (u);                          \
        *(Float *)((address) -= sizeof(Float)) = (reciprocal_z);               \
        *(Float *)((address) -= sizeof(Float)) = (y);                          \
        *(Float *)((address) -= sizeof(Float)) = (x);                          \
        *(Uint32 *)((address) -= sizeof(Uint32)) = (pcw);                      \
        prefetch((void *)(address));                                           \
        (address) += sizeof(struct vertex_packet);                             \
    }

/* whether p lies beyond z = 1: its 1/z is between 0 and 1. a polygon whose
 * points all do needs no clipping. DIRECT3 and DIRECT4 test every point */
#define DIRECT(p) ((p)->reciprocal_z > 0.0F && (p)->reciprocal_z < 1.0F)
#define DIRECT3(a, b, c)                                                       \
    ((a)->reciprocal_z > 0.0F && (b)->reciprocal_z > 0.0F &&                   \
     (c)->reciprocal_z > 0.0F && (a)->reciprocal_z < 1.0F &&                   \
     (b)->reciprocal_z < 1.0F && (c)->reciprocal_z < 1.0F)
#define DIRECT4(a, b, c, d)                                                    \
    ((a)->reciprocal_z > 0.0F && (b)->reciprocal_z > 0.0F &&                   \
     (c)->reciprocal_z > 0.0F && (d)->reciprocal_z > 0.0F &&                   \
     (a)->reciprocal_z < 1.0F && (b)->reciprocal_z < 1.0F &&                   \
     (c)->reciprocal_z < 1.0F && (d)->reciprocal_z < 1.0F)
/* twice the signed area of triangle a, b, c on the screen. the drawers that
 * cull send a triangle whose AREA is negative, and drop the rest */
#define AREA(a, b, c)                                                          \
    ((((b)->x - (a)->x) * ((c)->y - (b)->y)) -                                 \
     (((b)->y - (a)->y) * ((c)->x - (b)->x)))

/* the direct drawers, for a model with no point at z 0 or less. each sends
 * meshset's polygons from packet_cursor, adds the points meshset gives to
 * _nj_calc_polygon_ and the vertices it sends to _nj_draw_polygon_, and
 * returns the store queue address past the last vertex. meshset must hold at
 * least one polygon, and each strip at least three points */

/* each strip whole and unculled. the winding NJD_TRIMESH_END reverses goes
 * unused: the bit only comes off the count */
static struct vertex_packet *draw_solid_strips(NJS_MESHSET *meshset)
{
    register Float *queue;
    register int sent;
    register int given;
    Sint16 *index = meshset->meshes;
    struct projected_point *points;
    register struct projected_point *point;
    /* @hack spelling: each projected_point's floats read through a Float cursor
     * stepped across them, which SHC turns into post-increment loads (the
     * first, FMOV.S @R4+, at 0x8c6299f2); read by member, 75 of 86 words differ
     * and the function is 16 bytes longer */
    register Float *field;
    int left;
    int strips;
    Float x;
    Float y;
    Float reciprocal_z;
    Float intensity;

    prefetch(index);
    points = (struct projected_point *)_nj_vertex_buf_;
    sent = 0;
    given = 0;
    queue = (Float *)packet_cursor;
    strips = meshset->nbMesh;
    do {
        left = *index++;
        point = &points[*index++];
        prefetch(point);
        field = (Float *)point;
        if (left & NJD_TRIMESH_END) {
            left &= STRIP_COUNT;
        }
        sent += left;
        given += left;
        x = *field++;
        y = *field++;
        left--;
        reciprocal_z = *field++;
        intensity = *field;
        do {
            point = &points[*index++];
            prefetch(point);
            field = (Float *)point;
            PUT_SOLID(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z,
                      intensity);
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            prefetch(index);
            intensity = *field;
        } while (--left);
        PUT_SOLID(queue, KM_VERTEXPARAM_ENDOFSTRIP, x, y, reciprocal_z,
                  intensity);
    } while (--strips);
    _nj_draw_polygon_ += sent;
    _nj_calc_polygon_ += given;
    return (struct vertex_packet *)queue;
}

/* each triangle whose AREA is negative, as a strip of three */
static struct vertex_packet *draw_solid_triangles(NJS_MESHSET *meshset)
{
    register Sint16 *index = meshset->meshes;
    register struct projected_point *points;
    register Float *queue;
    /* @hack spelling: each projected_point's floats read through a Float cursor
     * stepped across them, which SHC turns into post-increment loads (the
     * first, FMOV.S @R4+, at 0x8c629ade); read by member, 111 of 115 words
     * differ and the function is 8 bytes longer */
    register Float *field;
    register struct projected_point *v0;
    register struct projected_point *v1;
    register struct projected_point *v2;
    register int left;
    register int sent;
    register Float x;
    register Float y;
    register Float reciprocal_z;
    register Float intensity;

    points = (struct projected_point *)_nj_vertex_buf_;
    prefetch(index + 3);
    queue = (Float *)packet_cursor;
    left = meshset->nbMesh;
    _nj_calc_polygon_ += left * 3;
    sent = 0;
    do {
        v0 = &points[*index++];
        v1 = &points[*index++];
        v2 = &points[*index++];
        prefetch(index + 3);
        if (AREA(v0, v1, v2) < 0.0F) {
            field = (Float *)v0;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            PUT_SOLID(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z,
                      intensity);
            field = (Float *)v1;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            PUT_SOLID(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z,
                      intensity);
            field = (Float *)v2;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            PUT_SOLID(queue, KM_VERTEXPARAM_ENDOFSTRIP, x, y, reciprocal_z,
                      intensity);
            sent += 3;
        }
    } while (--left);
    _nj_draw_polygon_ += sent;
    return (struct vertex_packet *)queue;
}

/* each quad whose first three points' AREA is negative, as a strip of its
 * four points in order */
static struct vertex_packet *draw_solid_quads(NJS_MESHSET *meshset)
{
    register Sint16 *index = meshset->meshes;
    register struct projected_point *points;
    register Float *queue;
    /* @hack spelling: each projected_point's floats read through a Float cursor
     * stepped across them, which SHC turns into post-increment loads (the
     * first, FMOV.S @R4+, at 0x8c629bc6); read by member, 125 of 132 words
     * differ and the function is 12 bytes longer */
    register Float *field;
    register struct projected_point *v0;
    register struct projected_point *v1;
    register struct projected_point *v2;
    register struct projected_point *v3;
    register int left;
    register int sent;
    register Float x;
    register Float y;
    register Float reciprocal_z;
    register Float intensity;

    points = (struct projected_point *)_nj_vertex_buf_;
    prefetch(index + 4);
    queue = (Float *)packet_cursor;
    left = meshset->nbMesh;
    _nj_calc_polygon_ += left * 4;
    sent = 0;
    do {
        v0 = &points[*index++];
        v1 = &points[*index++];
        v2 = &points[*index++];
        v3 = &points[*index++];
        prefetch(index + 4);
        if (AREA(v0, v1, v2) < 0.0F) {
            field = (Float *)v0;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            PUT_SOLID(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z,
                      intensity);
            field = (Float *)v1;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            PUT_SOLID(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z,
                      intensity);
            field = (Float *)v2;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            prefetch(v3);
            PUT_SOLID(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z,
                      intensity);
            field = (Float *)v3;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            PUT_SOLID(queue, KM_VERTEXPARAM_ENDOFSTRIP, x, y, reciprocal_z,
                      intensity);
            sent += 4;
        }
    } while (--left);
    _nj_draw_polygon_ += sent;
    return (struct vertex_packet *)queue;
}

/* draw_solid_strips with texture coordinates: meshset->vertuv holds a pair
 * for each point of each strip */
static struct vertex_packet *draw_tex_strips(NJS_MESHSET *meshset)
{
    Sint16 *index = meshset->meshes;
    struct projected_point *points;
    register struct projected_point *point;
    register Float *queue;
    /* @hack spelling: each projected_point's floats read through a Float cursor
     * stepped across them, which SHC turns into post-increment loads (the
     * first, FMOV.S @R4+, at 0x8c629cb0); read by member, 89 of 117 words
     * differ and the function is 12 bytes longer */
    register Float *field;
    register NJS_COLOR *uv;
    int left;
    int strips;
    register int sent;
    register int given;
    Uint32 count_mask;
    Uint32 pcw;
    Float tex_scale;
    Float y;
    register Float x;
    register Float reciprocal_z;
    register Float intensity;
    register Float u;
    register Float v;

    prefetch(index);
    points = (struct projected_point *)_nj_vertex_buf_;
    uv = meshset->vertuv;
    sent = 0;
    given = 0;
    queue = (Float *)packet_cursor;
    strips = meshset->nbMesh;
    count_mask = STRIP_COUNT;
    pcw = KM_VERTEXPARAM_NORMAL;
    tex_scale = TEX_SCALE;
    do {
        left = *index++;
        point = &points[*index++];
        prefetch(point);
        field = (Float *)point;
        if (left & NJD_TRIMESH_END) {
            left &= count_mask;
        }
        /* @hack redundant: a store to count_mask and pcw that never runs, which
         * keeps count_mask in R8 and pcw in R9 (0x8c629c8c, 0x8c629c90);
         * without it, count_mask goes to the frame, 109 of 117 words differ and
         * the function is 12 bytes longer */
        if (0) {
            count_mask = pcw = left;
        }
        given += left;
        x = *field++;
        sent += left;
        y = *field++;
        left--;
        reciprocal_z = *field++;
        intensity = *field;
        prefetch(uv);
        do {
            field = (Float *)&points[*index++];
            prefetch(index);
            u = uv->tex.u * tex_scale;
            v = uv->tex.v * tex_scale;
            prefetch(field);
            PUT_TEX(queue, pcw, x, y, reciprocal_z, u, v, intensity);
            uv++;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            prefetch(uv);
            intensity = *field;
        } while (--left);
        u = uv->tex.u * tex_scale;
        v = uv->tex.v * tex_scale;
        PUT_TEX(queue, KM_VERTEXPARAM_ENDOFSTRIP, x, y, reciprocal_z, u, v,
                intensity);
        uv++;
    } while (--strips);
    _nj_draw_polygon_ += sent;
    _nj_calc_polygon_ += given;
    /* @hack redundant: a store to queue that never runs, which keeps strips
     * in R11 (EXTU.W at 0x8c629c8e); without it, strips goes to the frame,
     * 107 of 117 words differ and the function is 12 bytes longer */
    if (0) {
        queue = (Float *)strips;
    }
    return (struct vertex_packet *)queue;
}

/* draw_solid_triangles with texture coordinates: meshset->vertuv holds three
 * pairs for each triangle, culled or not */
static struct vertex_packet *draw_tex_triangles(NJS_MESHSET *meshset)
{
    register Sint16 *index = meshset->meshes;
    register struct projected_point *points;
    register Float *queue;
    /* @hack spelling: each projected_point's floats read through a Float cursor
     * stepped across them, which SHC turns into post-increment loads (the
     * first, FMOV.S @R4+, at 0x8c629e08); read by member, 114 of 164 words
     * differ and the function is 12 bytes longer */
    register Float *field;
    register struct projected_point *v0;
    register struct projected_point *v1;
    register struct projected_point *v2;
    register NJS_COLOR *uv;
    register int left;
    register int sent;
    register Float x;
    register Float y;
    register Float reciprocal_z;
    register Float intensity;
    register Float u;
    register Float v;

    prefetch(index + 3);
    uv = meshset->vertuv;
    points = (struct projected_point *)_nj_vertex_buf_;
    queue = (Float *)packet_cursor;
    left = meshset->nbMesh;
    _nj_calc_polygon_ += left * 3;
    sent = 0;
    do {
        v0 = &points[*index++];
        v1 = &points[*index++];
        v2 = &points[*index++];
        prefetch(index + 3);
        if (AREA(v0, v1, v2) < 0.0F) {
            prefetch(uv + 3);
            field = (Float *)v0;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            u = uv->tex.u * TEX_SCALE;
            /* @hack spelling: uv stepped inside the read of v, here for v0 and
             * below for v1, which loads v2's x into FR8 and its 1/z into FR9
             * (0x8c629e70, 0x8c629e76); stepped after it, x and 1/z trade and 4
             * of 164 words differ */
            v = (uv++)->tex.v * TEX_SCALE;
            PUT_TEX(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z, u, v,
                    intensity);
            field = (Float *)v1;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            u = uv->tex.u * TEX_SCALE;
            v = (uv++)->tex.v * TEX_SCALE;
            PUT_TEX(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z, u, v,
                    intensity);
            field = (Float *)v2;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            u = uv->tex.u * TEX_SCALE;
            v = uv->tex.v * TEX_SCALE;
            uv++;
            PUT_TEX(queue, KM_VERTEXPARAM_ENDOFSTRIP, x, y, reciprocal_z, u, v,
                    intensity);
            sent += 3;
        } else {
            uv += 3;
        }
    } while (--left);
    _nj_draw_polygon_ += sent;
    return (struct vertex_packet *)queue;
}

/* draw_solid_quads with texture coordinates: meshset->vertuv holds four pairs
 * for each quad, culled or not */
static struct vertex_packet *draw_tex_quads(NJS_MESHSET *meshset)
{
    register Sint16 *index = meshset->meshes;
    register struct projected_point *points;
    /* @hack type: the queue's address in an integer, which loads v2's
     * intensity, u and v into FR5, FR4 and FR6 (0x8c629fc0 to 0x8c629fd6)
     * and v3's intensity and v into FR6 and FR5 (0x8c629ffc, 0x8c62a008); as
     * a Float pointer with PUT_TEX, they take others and 19 of 194 words
     * differ */
    register Uint32 address;
    /* @hack spelling: each projected_point's floats read through a Float cursor
     * stepped across them, which SHC turns into post-increment loads (the
     * first, FMOV.S @R4+, at 0x8c629f50); read by member, 158 of 194 words
     * differ and the function is 16 bytes longer */
    register Float *field;
    register struct projected_point *v0;
    register struct projected_point *v1;
    register struct projected_point *v2;
    register struct projected_point *v3;
    register NJS_COLOR *uv;
    register int left;
    register int sent;
    register Float x;
    register Float y;
    register Float reciprocal_z;
    register Float intensity;
    register Float u;
    register Float v;

    prefetch(index + 4);
    uv = meshset->vertuv;
    points = (struct projected_point *)_nj_vertex_buf_;
    address = (Uint32)packet_cursor;
    left = meshset->nbMesh;
    _nj_calc_polygon_ += left * 4;
    sent = 0;
    do {
        v0 = &points[*index++];
        v1 = &points[*index++];
        v2 = &points[*index++];
        v3 = &points[*index++];
        prefetch(index + 4);
        if (AREA(v0, v1, v2) < 0.0F) {
            prefetch(uv + 4);
            field = (Float *)v0;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            u = uv->tex.u * TEX_SCALE;
            v = uv->tex.v * TEX_SCALE;
            uv++;
            PUT_TEX_AT(address, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z, u, v,
                       intensity);
            field = (Float *)v1;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            u = uv->tex.u * TEX_SCALE;
            v = uv->tex.v * TEX_SCALE;
            uv++;
            PUT_TEX_AT(address, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z, u, v,
                       intensity);
            field = (Float *)v2;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            u = uv->tex.u * TEX_SCALE;
            v = uv->tex.v * TEX_SCALE;
            uv++;
            prefetch(v3);
            PUT_TEX_AT(address, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z, u, v,
                       intensity);
            field = (Float *)v3;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            u = uv->tex.u * TEX_SCALE;
            v = uv->tex.v * TEX_SCALE;
            uv++;
            PUT_TEX_AT(address, KM_VERTEXPARAM_ENDOFSTRIP, x, y, reciprocal_z,
                       u, v, intensity);
            sent += 4;
        } else {
            uv += 4;
        }
    } while (--left);
    _nj_draw_polygon_ += sent;
    return (struct vertex_packet *)address;
}

/* a triangle's corner in a clipper: its projected point, and its position in
 * view space, which SET_CORNER recovers from it */
struct clip_corner {
    Float x, y, reciprocal_z;
    Float view_x, view_y, view_z;
    Float u, v; /* in units of 1/256 */
    Float intensity;
};

/* set corner from point, a projected_point, undoing calc's projection for its
 * view position. inverse_x and inverse_y must be the reciprocals of
 * calc.projection_x and calc.projection_y. corner's u and v are left as they
 * were */
#define SET_CORNER(corner, point, inverse_x, inverse_y)                        \
    {                                                                          \
        (corner).x = (point)->x;                                               \
        (corner).y = (point)->y;                                               \
        (corner).reciprocal_z = (point)->reciprocal_z;                         \
        (corner).view_z = 1.0F / (point)->reciprocal_z;                        \
        (corner).view_x =                                                      \
            ((point)->x - calc.center_x) * (corner).view_z * (inverse_x);      \
        (corner).view_y =                                                      \
            ((point)->y - calc.center_y) * (corner).view_z * (inverse_y);      \
        (corner).intensity = (point)->intensity;                               \
    }

/* send the part of triangle v0, v1, v2 that lies beyond z = 1, from
 * packet_cursor, as a strip of three or four vertices: its corners beyond
 * z = 1, and the points where its edges cross z = 1, whose 1/z is 1. it culls
 * nothing, sends nothing when no corner lies beyond z = 1, and adds the
 * vertices it sends to _nj_draw_polygon_. it returns the store queue address
 * past the last vertex */
static struct vertex_packet *clip_solid_triangle(struct projected_point *v0,
                                                 struct projected_point *v1,
                                                 struct projected_point *v2)
{
    struct clip_corner input[3];
    struct clip_corner output[4];
    struct clip_corner *previous;
    struct clip_corner *current;
    register struct clip_corner *out;
    struct clip_corner *corner;
    register Float inverse_x;
    register Float inverse_y;
    register Float *queue;
    register Float t;
    register Float view_x;
    register Float view_y;
    register int i;
    register unsigned int count;
    register int last;

    inverse_x = 1.0F / calc.projection_x;
    inverse_y = 1.0F / calc.projection_y;
    queue = (Float *)packet_cursor;
    SET_CORNER(input[0], v0, inverse_x, inverse_y);
    SET_CORNER(input[1], v1, inverse_x, inverse_y);
    SET_CORNER(input[2], v2, inverse_x, inverse_y);
    if (input[0].view_z > 1.0F || input[1].view_z > 1.0F ||
        input[2].view_z > 1.0F) {
        count = 0;
        last = 2;
        /* @hack spelling: i set ahead of the loop, not in its header, which
         * puts output's address in R9 and previous in R10 (MOV R15,R9 at
         * 0x8c62a1a0, MOV R4,R10 at 0x8c62a1b8); in the header, i's move takes
         * the BRA's delay slot, output's address trades registers with
         * previous, and 24 of 371 words differ */
        i = count;
        for (; i < 3; last = i, i++) {
            previous = &input[last];
            current = &input[i];
            /* @hack spelling: the last corner's view_z read by its index, which
             * keeps inverse_x in FR5 and inverse_y in FR4 (FDIV at 0x8c62a08a,
             * 0x8c62a094) and last in R4 (0x8c62a19c); through previous, each
             * pair trades and 65 of 371 words differ */
            if ((input[last].view_z <= 1.0F) ^ (current->view_z <= 1.0F)) {
                t = (1.0F - previous->view_z) /
                    (current->view_z - previous->view_z);
                view_x = previous->view_x +
                         ((current->view_x - previous->view_x) * t);
                view_y = previous->view_y +
                         ((current->view_y - previous->view_y) * t);
                out = &output[count];
                out->x = (view_x * calc.projection_x) + calc.center_x;
                out->y = (view_y * calc.projection_y) + calc.center_y;
                out->reciprocal_z = 1.0F;
                out->intensity =
                    previous->intensity +
                    ((current->intensity - previous->intensity) * t);
                count++;
            }
            /* @hack redundant: current set to &input[i] again, which forms its
             * address once more, in R14, ahead of the view_z test (0x8c62a264
             * to 0x8c62a272); without it, SHC forms it in R2 inside the copy,
             * and 22 of 371 words differ */
            current = &input[i];
            if (current->view_z > 1.0F) {
                corner = &output[count];
                /* @hack spelling: x read by the corner's index, which forms
                 * corner's address after the view_z test (0x8c62a280 to
                 * 0x8c62a28e); through current, before it, and 23 of 371
                 * words differ */
                corner->x = input[i].x;
                corner->y = current->y;
                corner->reciprocal_z = current->reciprocal_z;
                corner->intensity = current->intensity;
                count++;
            } else {
                /* @hack redundant: a continue at the end of the loop's body,
                 * which makes SHC enter the loop by a BRA to its test and write
                 * the pending literal pool right behind that BRA (BRA at
                 * 0x8c62a1a2, the pool at 0x8c62a1a8); without it, the pool
                 * moves into the loop, 203 of 371 words differ and the function
                 * is 8 bytes longer */
                continue;
            }
        }
        {
            register Float x;
            Float y;
            register Float reciprocal_z;
            Float intensity;

            corner = output;
            x = corner->x;
            y = corner->y;
            reciprocal_z = corner->reciprocal_z;
            intensity = corner->intensity;
            /* @hack redundant: a store to y that never runs, which loads x
             * into FR5 and 1/z into FR4, here and for output[3] (0x8c62a2b0,
             * 0x8c62a2ba, 0x8c62a2fc, 0x8c62a306); without it, each pair
             * trades and 8 of 371 words differ */
            if (0) {
                y = reciprocal_z;
            }
            PUT_SOLID(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z,
                      intensity);
        }
        {
            register Float x;
            register Float y;
            register Float reciprocal_z;
            register Float intensity;

            corner++;
            x = corner->x;
            y = corner->y;
            reciprocal_z = corner->reciprocal_z;
            intensity = corner->intensity;
            /* @hack redundant: a store to x that never runs, which loads y
             * into FR4 and 1/z into FR6 (0x8c62a2d4, 0x8c62a2da); without it,
             * the two trade and 4 of 371 words differ */
            if (0) {
                x = y;
            }
            PUT_SOLID(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z,
                      intensity);
        }
        /* a fourth corner goes before output[2]: the strip runs 0, 1, 3, 2 */
        if (count != 3) {
            register Float x;
            Float y;
            register Float reciprocal_z;
            Float intensity;

            corner = &output[3];
            x = corner->x;
            y = corner->y;
            reciprocal_z = corner->reciprocal_z;
            intensity = corner->intensity;
            PUT_SOLID(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z,
                      intensity);
        }
        {
            register Float x;
            Float y;
            Float reciprocal_z;
            Float intensity;

            corner = &output[2];
            x = corner->x;
            y = corner->y;
            reciprocal_z = corner->reciprocal_z;
            intensity = corner->intensity;
            PUT_SOLID(queue, KM_VERTEXPARAM_ENDOFSTRIP, x, y, reciprocal_z,
                      intensity);
        }
        _nj_draw_polygon_ += count;
    }
    return (struct vertex_packet *)queue;
}

/* clip_solid_triangle with texture coordinates: uv holds v0's, v1's and v2's
 * pairs, which it interpolates in units of 1/256 and scales by TEX_SCALE as it
 * sends them */
struct vertex_packet *njDrawNearClipTexture(struct projected_point *v0,
                                            struct projected_point *v1,
                                            struct projected_point *v2,
                                            NJS_COLOR *uv)
{
    struct clip_corner input[3];
    struct clip_corner output[4];
    struct clip_corner *previous;
    struct clip_corner *current;
    register struct clip_corner *out;
    struct clip_corner *corner;
    register Float inverse_x;
    register Float inverse_y;
    register Float *queue;
    register Float t;
    register Float view_x;
    register Float view_y;
    register int i;
    register unsigned int count;
    int last;

    inverse_x = 1.0F / calc.projection_x;
    inverse_y = 1.0F / calc.projection_y;
    queue = (Float *)packet_cursor;
    SET_CORNER(input[0], v0, inverse_x, inverse_y);
    input[0].u = uv->tex.u;
    input[0].v = uv->tex.v;
    uv++;
    SET_CORNER(input[1], v1, inverse_x, inverse_y);
    input[1].u = uv->tex.u;
    input[1].v = uv->tex.v;
    uv++;
    SET_CORNER(input[2], v2, inverse_x, inverse_y);
    input[2].u = uv->tex.u;
    input[2].v = uv->tex.v;
    uv++;
    if (input[0].view_z > 1.0F || input[1].view_z > 1.0F ||
        input[2].view_z > 1.0F) {
        count = 0;
        last = 2;
        /* @hack spelling: i set ahead of the loop, not in its header, which
         * keeps the loop's bound, 3, in R7 and previous in R13 (MOV #3,R7 at
         * 0x8c62a4f4, MOV R5,R13 at 0x8c62a508); in the header, i's move takes
         * the BRA's delay slot, the bound trades registers with previous, and
         * 32 of 461 words differ */
        i = count;
        for (; i < 3; last = i, i++) {
            previous = &input[last];
            current = &input[i];
            /* @hack redundant: a store to previous that never runs, which
             * forms previous's address in R13, then current's in R10
             * (0x8c62a508 to 0x8c62a526); without it, the two trade and 33 of
             * 461 words differ */
            if (0) {
                previous = 0;
            }
            if ((previous->view_z <= 1.0F) ^ (current->view_z <= 1.0F)) {
                t = (1.0F - previous->view_z) /
                    (current->view_z - previous->view_z);
                view_x = previous->view_x +
                         ((current->view_x - previous->view_x) * t);
                view_y = previous->view_y +
                         ((current->view_y - previous->view_y) * t);
                out = &output[count];
                out->x = (view_x * calc.projection_x) + calc.center_x;
                out->y = (view_y * calc.projection_y) + calc.center_y;
                out->reciprocal_z = 1.0F;
                out->u = previous->u + ((current->u - previous->u) * t);
                out->v = previous->v + ((current->v - previous->v) * t);
                out->intensity =
                    previous->intensity +
                    ((current->intensity - previous->intensity) * t);
                count++;
            }
            /* @hack redundant: current set to &input[i] again, which forms its
             * address once more, in R13, ahead of the view_z test (0x8c62a5d4
             * to 0x8c62a5e2); without it, SHC keeps the one from the loop's
             * head, in R10, 181 of 461 words differ and the function is 16
             * bytes shorter */
            current = &input[i];
            if (current->view_z > 1.0F) {
                out = &output[count];
                out->x = current->x;
                out->y = current->y;
                out->reciprocal_z = current->reciprocal_z;
                out->u = current->u;
                out->v = current->v;
                out->intensity = current->intensity;
                count++;
            } else {
                /* @hack redundant: a continue at the end of the loop's body,
                 * which makes SHC enter the loop by a BRA to its test and write
                 * the pending literal pool right behind that BRA (BRA at
                 * 0x8c62a4f2, the pool at 0x8c62a4f8); without it, the pool
                 * moves into the loop, 238 of 461 words differ and the function
                 * is 4 bytes longer */
                continue;
            }
        }
        {
            register Float x;
            register Float y;
            register Float reciprocal_z;
            register Float intensity;
            register Float u;
            Float v;

            corner = output;
            x = corner->x;
            y = corner->y;
            reciprocal_z = corner->reciprocal_z;
            intensity = corner->intensity;
            u = corner->u * TEX_SCALE;
            v = corner->v * TEX_SCALE;
            /* @hack redundant: stores to x, intensity and u that never run,
             * which load x, y and u into FR4, FR8 and FR6 (0x8c62a630,
             * 0x8c62a634, 0x8c62a646); without them, these registers and the
             * last two vertices' change, and 17 of 461 words differ */
            if (0) {
                /* NOLINTNEXTLINE(clang-diagnostic-self-assign) */
                x = x;
                intensity = x;
                u = intensity;
            }
            PUT_TEX(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z, u, v,
                    intensity);
        }
        {
            register Float x;
            register Float y;
            register Float reciprocal_z;
            register Float intensity;
            register Float u;
            register Float v;

            corner++;
            x = corner->x;
            y = corner->y;
            reciprocal_z = corner->reciprocal_z;
            intensity = corner->intensity;
            u = corner->u * TEX_SCALE;
            v = corner->v * TEX_SCALE;
            PUT_TEX(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z, u, v,
                    intensity);
        }
        /* a fourth corner goes before output[2]: the strip runs 0, 1, 3, 2 */
        if (count != 3) {
            register Float x;
            Float y;
            Float reciprocal_z;
            register Float intensity;
            Float u;
            Float v;

            corner = &output[3];
            x = corner->x;
            y = corner->y;
            reciprocal_z = corner->reciprocal_z;
            intensity = corner->intensity;
            /* @hack spelling: u scaled in a statement of its own, which loads
             * y, 1/z and u into FR8, FR7 and FR6 (0x8c62a6a0 to 0x8c62a6ae);
             * scaled as it is read, the three rotate and 7 of 461 words
             * differ */
            u = corner->u;
            u *= TEX_SCALE;
            v = corner->v * TEX_SCALE;
            PUT_TEX(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z, u, v,
                    intensity);
        }
        {
            register Float x;
            register Float y;
            Float reciprocal_z;
            register Float intensity;
            Float u;
            Float v;

            corner = &output[2];
            x = corner->x;
            y = corner->y;
            reciprocal_z = corner->reciprocal_z;
            intensity = corner->intensity;
            u = corner->u * TEX_SCALE;
            v = corner->v * TEX_SCALE;
            PUT_TEX(queue, KM_VERTEXPARAM_ENDOFSTRIP, x, y, reciprocal_z, u, v,
                    intensity);
        }
        _nj_draw_polygon_ += count;
    }
    return (struct vertex_packet *)queue;
}

/* the near drawers, for a model with a point at z 0 or less, keep the direct
 * drawers' contract. each sends a polygon whose points are all DIRECT as its
 * direct drawer does, gives one with some DIRECT point to a clipper, and
 * drops the rest. in a strip, each run of triangles whose points are all
 * DIRECT goes as one strip */

static struct vertex_packet *draw_solid_strips_near(NJS_MESHSET *meshset)
{
    register Sint16 *index = meshset->meshes;
    register struct projected_point *points;
    register Float *queue;
    register struct projected_point *v0;
    struct projected_point *v1;
    struct projected_point *v2;
    register int strips;
    register int sent;
    int left;
    int run;
    register int given;
    register int clip;
    register Float intensity;

    prefetch(index + 3);
    points = (struct projected_point *)_nj_vertex_buf_;
    sent = 0;
    given = 0;
    queue = (Float *)packet_cursor;
    strips = meshset->nbMesh;
    do {
        left = *index++;
        if (left & NJD_TRIMESH_END) {
            left &= STRIP_COUNT;
        }
        given += left;
        left -= 2;
        run = 0;
        v0 = &points[*index++];
        v1 = &points[*index++];
        do {
            v2 = &points[*index++];
            clip = 0;
            if (!DIRECT3(v0, v1, v2)) {
                clip = 1;
            }
            if (!clip) {
                if (!run) {
                    intensity = v0->intensity;
                    PUT_SOLID(queue, KM_VERTEXPARAM_NORMAL, v0->x, v0->y,
                              v0->reciprocal_z, intensity);
                    run++;
                }
                intensity = v1->intensity;
                PUT_SOLID(queue, KM_VERTEXPARAM_NORMAL, v1->x, v1->y,
                          v1->reciprocal_z, intensity);
                run++;
            } else if (run) {
                intensity = v1->intensity;
                PUT_SOLID(queue, KM_VERTEXPARAM_ENDOFSTRIP, v1->x, v1->y,
                          v1->reciprocal_z, intensity);
                sent += run + 1;
                run = 0;
            }
            if (clip) {
                if (DIRECT(v0) || DIRECT(v1) || DIRECT(v2)) {
                    packet_cursor = (struct vertex_packet *)queue;
                    queue = (Float *)clip_solid_triangle(v0, v1, v2);
                }
            }
            prefetch(index);
            v0 = v1;
            v1 = v2;
        } while (--left);
        prefetch(index + 3);
        if (run) {
            intensity = v2->intensity;
            PUT_SOLID(queue, KM_VERTEXPARAM_ENDOFSTRIP, v2->x, v2->y,
                      v2->reciprocal_z, intensity);
            sent += run + 1;
        }
    } while (--strips);
    _nj_draw_polygon_ += sent;
    _nj_calc_polygon_ += given;
    return (struct vertex_packet *)queue;
}

static struct vertex_packet *draw_solid_triangles_near(NJS_MESHSET *meshset)
{
    register Sint16 *index = meshset->meshes;
    register struct projected_point *points;
    register Float *queue;
    /* @hack spelling: each projected_point's floats read through a Float cursor
     * stepped across them, which SHC turns into post-increment loads (the
     * first, FMOV.S @R4+, at 0x8c62aa18); read by member, 70 of 173 words
     * differ and the function is 12 bytes longer */
    register Float *field;
    register struct projected_point *v0;
    register struct projected_point *v1;
    register struct projected_point *v2;
    register int left;
    register int sent;
    register Float x;
    register Float y;
    register Float reciprocal_z;
    register Float intensity;

    points = (struct projected_point *)_nj_vertex_buf_;
    prefetch(index + 3);
    queue = (Float *)packet_cursor;
    left = meshset->nbMesh;
    _nj_calc_polygon_ += left * 3;
    sent = 0;
    do {
        v0 = &points[*index++];
        v1 = &points[*index++];
        v2 = &points[*index++];
        prefetch(index + 3);
        if (!DIRECT3(v0, v1, v2)) {
            if (DIRECT(v0) || DIRECT(v1) || DIRECT(v2)) {
                packet_cursor = (struct vertex_packet *)queue;
                queue = (Float *)clip_solid_triangle(v0, v1, v2);
            }
        } else if (AREA(v0, v1, v2) < 0.0F) {
            field = (Float *)v0;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            PUT_SOLID(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z,
                      intensity);
            field = (Float *)v1;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            PUT_SOLID(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z,
                      intensity);
            field = (Float *)v2;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            PUT_SOLID(queue, KM_VERTEXPARAM_ENDOFSTRIP, x, y, reciprocal_z,
                      intensity);
            sent += 3;
        }
    } while (--left);
    _nj_draw_polygon_ += sent;
    return (struct vertex_packet *)queue;
}

/* a quad with a point not DIRECT goes to the clipper as triangles 0, 1, 2
 * and 1, 2, 3 */
static struct vertex_packet *draw_solid_quads_near(NJS_MESHSET *meshset)
{
    register Sint16 *index = meshset->meshes;
    register struct projected_point *points;
    register Float *queue;
    /* @hack spelling: each projected_point's floats read through a Float cursor
     * stepped across them, which SHC turns into post-increment loads (the
     * first, FMOV.S @R4+, at 0x8c62abe8); read by member, 201 of 236 words
     * differ and the function is 32 bytes longer */
    register Float *field;
    register struct projected_point *v0;
    register struct projected_point *v1;
    register struct projected_point *v2;
    register struct projected_point *v3;
    register int left;
    register int sent;
    register Float x;
    register Float y;
    register Float reciprocal_z;
    register Float intensity;

    points = (struct projected_point *)_nj_vertex_buf_;
    prefetch(index + 4);
    queue = (Float *)packet_cursor;
    left = meshset->nbMesh;
    _nj_calc_polygon_ += left * 4;
    sent = 0;
    do {
        v0 = &points[*index++];
        v1 = &points[*index++];
        v2 = &points[*index++];
        v3 = &points[*index++];
        prefetch(index + 4);
        if (!DIRECT4(v0, v1, v2, v3)) {
            if (DIRECT(v0) || DIRECT(v1) || DIRECT(v2)) {
                packet_cursor = (struct vertex_packet *)queue;
                queue = (Float *)clip_solid_triangle(v0, v1, v2);
            }
            if (DIRECT(v3) || DIRECT(v1) || DIRECT(v2)) {
                packet_cursor = (struct vertex_packet *)queue;
                queue = (Float *)clip_solid_triangle(v1, v2, v3);
            }
        } else if (AREA(v0, v1, v2) < 0.0F) {
            field = (Float *)v0;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            PUT_SOLID(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z,
                      intensity);
            field = (Float *)v1;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            PUT_SOLID(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z,
                      intensity);
            field = (Float *)v2;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            PUT_SOLID(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z,
                      intensity);
            field = (Float *)v3;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            PUT_SOLID(queue, KM_VERTEXPARAM_ENDOFSTRIP, x, y, reciprocal_z,
                      intensity);
            sent += 4;
        }
    } while (--left);
    _nj_draw_polygon_ += sent;
    return (struct vertex_packet *)queue;
}

/* meshset->vertuv holds a texture coordinate pair for each point of each
 * strip. a clipped triangle's three pairs start at its first point's */
static struct vertex_packet *draw_tex_strips_near(NJS_MESHSET *meshset)
{
    Sint16 *index = meshset->meshes;
    struct projected_point *points;
    register Float *queue;
    /* @hack spelling: each projected_point's floats read through a Float cursor
     * stepped across them, which SHC turns into post-increment loads (the
     * first, FMOV.S @R5+, at 0x8c62ad70); read by member, 238 of 312 words
     * differ and the function is 16 bytes longer */
    register Float *field;
    register struct projected_point *v0;
    register struct projected_point *v1;
    register struct projected_point *v2;
    register NJS_COLOR *uv;
    register int strips;
    int run;
    register int left;
    register int sent;
    register int given;
    register int clip;
    register Float tex_scale;

    prefetch(index + 3);
    uv = meshset->vertuv;
    points = (struct projected_point *)_nj_vertex_buf_;
    sent = 0;
    given = 0;
    queue = (Float *)packet_cursor;
    strips = meshset->nbMesh;
    tex_scale = TEX_SCALE;
    do {
        left = *index++;
        if (left & NJD_TRIMESH_END) {
            left &= STRIP_COUNT;
        }
        /* @hack spelling: run's 0 added to left inside v0's index, a comma
         * expression that also adds left to given and takes 2 from left. it
         * loads the intensity of an open run's last vertex into FR5 and its u
         * into FR6 (0x8c62aec0, 0x8c62aec2); with the locals given, left and
         * run set in statements of their own, the intensity and u trade
         * registers, and 5 of 312 words differ */
        v0 = &points[(given += left, left -= 2, left += (run = 0), *index++)];
        v1 = &points[*index++];
        do {
            v2 = &points[*index++];
            clip = 0;
            if (!DIRECT3(v0, v1, v2)) {
                clip = 1;
            }
            prefetch(uv + 1);
            if (!clip) {
                if (!run) {
                    {
                        register Float x;
                        register Float y;
                        Float reciprocal_z;
                        register Float intensity;
                        register Float u;
                        register Float v;

                        field = (Float *)v0;
                        x = *field++;
                        y = *field++;
                        reciprocal_z = *field++;
                        intensity = *field;
                        u = uv->tex.u * tex_scale;
                        v = uv->tex.v * tex_scale;
                        PUT_TEX(queue, KM_VERTEXPARAM_NORMAL, x, y,
                                reciprocal_z, u, v, intensity);
                    }
                    uv++;
                    run++;
                }
                {
                    Float x;
                    Float y;
                    register Float reciprocal_z;
                    register Float intensity;
                    register Float u;
                    register Float v;

                    field = (Float *)v1;
                    x = *field++;
                    y = *field++;
                    reciprocal_z = *field++;
                    intensity = *field;
                    u = uv->tex.u * tex_scale;
                    v = uv->tex.v * tex_scale;
                    PUT_TEX(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z, u,
                            v, intensity);
                }
                uv++;
                run++;
            } else if (run) {
                {
                    register Float x;
                    Float y;
                    register Float reciprocal_z;
                    register Float intensity;
                    register Float u;
                    register Float v;

                    field = (Float *)v1;
                    x = *field++;
                    y = *field++;
                    reciprocal_z = *field++;
                    intensity = *field;
                    u = uv->tex.u * tex_scale;
                    v = uv->tex.v * tex_scale;
                    PUT_TEX(queue, KM_VERTEXPARAM_ENDOFSTRIP, x, y,
                            reciprocal_z, u, v, intensity);
                }
                sent += run + 1;
                run = 0;
            } else {
                uv++;
            }
            if (clip) {
                if (DIRECT(v0) || DIRECT(v1) || DIRECT(v2)) {
                    uv--;
                    packet_cursor = (struct vertex_packet *)queue;
                    queue = (Float *)njDrawNearClipTexture(v0, v1, v2, uv);
                    uv++;
                }
            }
            prefetch(index);
            v0 = v1;
            v1 = v2;
        } while (--left);
        prefetch(index + 3);
        if (run) {
            {
                register Float x;
                register Float y;
                Float reciprocal_z;
                register Float intensity;
                register Float u;
                register Float v;

                field = (Float *)v2;
                x = *field++;
                y = *field++;
                reciprocal_z = *field++;
                intensity = *field;
                u = uv->tex.u * tex_scale;
                v = uv->tex.v * tex_scale;
                PUT_TEX(queue, KM_VERTEXPARAM_ENDOFSTRIP, x, y, reciprocal_z, u,
                        v, intensity);
            }
            uv++;
            sent += run + 1;
        } else {
            uv += 2;
        }
    } while (--strips);
    _nj_draw_polygon_ += sent;
    _nj_calc_polygon_ += given;
    return (struct vertex_packet *)queue;
}

static struct vertex_packet *draw_tex_triangles_near(NJS_MESHSET *meshset)
{
    register Sint16 *index = meshset->meshes;
    register struct projected_point *points;
    register Float *queue;
    /* @hack spelling: each projected_point's floats read through a Float cursor
     * stepped across them, which SHC turns into post-increment loads (the
     * first, FMOV.S @R4+, at 0x8c62b028); read by member, 140 of 223 words
     * differ and the function is 16 bytes longer */
    register Float *field;
    register struct projected_point *v0;
    register struct projected_point *v1;
    register struct projected_point *v2;
    register NJS_COLOR *uv;
    register int left;
    register int sent;
    register Float x;
    register Float y;
    register Float reciprocal_z;
    register Float intensity;
    register Float u;
    register Float v;

    prefetch(index + 3);
    uv = meshset->vertuv;
    points = (struct projected_point *)_nj_vertex_buf_;
    queue = (Float *)packet_cursor;
    left = meshset->nbMesh;
    _nj_calc_polygon_ += left * 3;
    sent = 0;
    do {
        v0 = &points[*index++];
        v1 = &points[*index++];
        v2 = &points[*index++];
        prefetch(index + 3);
        if (!DIRECT3(v0, v1, v2)) {
            if (DIRECT(v0) || DIRECT(v1) || DIRECT(v2)) {
                packet_cursor = (struct vertex_packet *)queue;
                queue = (Float *)njDrawNearClipTexture(v0, v1, v2, uv);
            }
            uv += 3;
        } else if (AREA(v0, v1, v2) < 0.0F) {
            prefetch(uv + 3);
            field = (Float *)v0;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            u = uv->tex.u * TEX_SCALE;
            v = uv->tex.v * TEX_SCALE;
            uv++;
            PUT_TEX(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z, u, v,
                    intensity);
            field = (Float *)v1;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            u = uv->tex.u * TEX_SCALE;
            v = uv->tex.v * TEX_SCALE;
            uv++;
            PUT_TEX(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z, u, v,
                    intensity);
            field = (Float *)v2;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            u = uv->tex.u * TEX_SCALE;
            v = uv->tex.v * TEX_SCALE;
            uv++;
            PUT_TEX(queue, KM_VERTEXPARAM_ENDOFSTRIP, x, y, reciprocal_z, u, v,
                    intensity);
            sent += 3;
        } else {
            uv += 3;
        }
    } while (--left);
    _nj_draw_polygon_ += sent;
    return (struct vertex_packet *)queue;
}

/* a quad with a point not DIRECT goes to the clipper as triangles 0, 1, 2
 * and 1, 2, 3, with their points' texture coordinates */
static struct vertex_packet *draw_tex_quads_near(NJS_MESHSET *meshset)
{
    register Sint16 *index = meshset->meshes;
    register struct projected_point *points;
    register Float *queue;
    /* @hack spelling: each projected_point's floats read through a Float cursor
     * stepped across them, which SHC turns into post-increment loads (the
     * first, FMOV.S @R4+, at 0x8c62b274); read by member, 274 of 312 words
     * differ and the function is 24 bytes longer */
    register Float *field;
    register struct projected_point *v0;
    register struct projected_point *v1;
    register struct projected_point *v2;
    register struct projected_point *v3;
    register NJS_COLOR *uv;
    register int left;
    register int sent;
    register Float x;
    register Float y;
    register Float reciprocal_z;
    register Float intensity;
    register Float u;
    register Float v;

    prefetch(index + 4);
    uv = meshset->vertuv;
    points = (struct projected_point *)_nj_vertex_buf_;
    queue = (Float *)packet_cursor;
    left = meshset->nbMesh;
    _nj_calc_polygon_ += left * 4;
    sent = 0;
    do {
        v0 = &points[*index++];
        v1 = &points[*index++];
        v2 = &points[*index++];
        v3 = &points[*index++];
        prefetch(index + 4);
        if (!DIRECT4(v0, v1, v2, v3)) {
            if (DIRECT(v0) || DIRECT(v1) || DIRECT(v2)) {
                packet_cursor = (struct vertex_packet *)queue;
                queue = (Float *)njDrawNearClipTexture(v0, v1, v2, uv);
            }
            if (DIRECT(v3) || DIRECT(v1) || DIRECT(v2)) {
                packet_cursor = (struct vertex_packet *)queue;
                queue = (Float *)njDrawNearClipTexture(v1, v2, v3, uv + 1);
            }
            uv += 4;
        } else if (AREA(v0, v1, v2) < 0.0F) {
            prefetch(uv + 4);
            field = (Float *)v0;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            u = uv->tex.u * TEX_SCALE;
            v = uv->tex.v * TEX_SCALE;
            uv++;
            PUT_TEX(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z, u, v,
                    intensity);
            field = (Float *)v1;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            u = uv->tex.u * TEX_SCALE;
            v = uv->tex.v * TEX_SCALE;
            uv++;
            PUT_TEX(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z, u, v,
                    intensity);
            field = (Float *)v2;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            u = uv->tex.u * TEX_SCALE;
            v = uv->tex.v * TEX_SCALE;
            uv++;
            prefetch(v3);
            PUT_TEX(queue, KM_VERTEXPARAM_NORMAL, x, y, reciprocal_z, u, v,
                    intensity);
            field = (Float *)v3;
            x = *field++;
            y = *field++;
            reciprocal_z = *field++;
            intensity = *field;
            u = uv->tex.u * TEX_SCALE;
            v = uv->tex.v * TEX_SCALE;
            uv++;
            PUT_TEX(queue, KM_VERTEXPARAM_ENDOFSTRIP, x, y, reciprocal_z, u, v,
                    intensity);
            sent += 4;
        } else {
            uv += 4;
        }
    } while (--left);
    _nj_draw_polygon_ += sent;
    return (struct vertex_packet *)queue;
}

/* draw model as njEasyDrawModel does, inside the caller's own njGetkmPointer
 * and njSetkmPointer */
void _njEasyDrawModel(NJS_MODEL *model)
{
    NJS_ARGB color;
    NJS_POINT3 center;
    /* @hack spelling: color's channels stored through their addresses, which
     * the frame holds and each store loads (0x8c62b5c4, 0x8c62b5da,
     * 0x8c62b5ee, 0x8c62b604); stored as color's members, 306 of 437 words
     * differ and the function is 24 bytes shorter */
    Float *red;
    Float *green;
    Float *blue;
    Float *alpha;
    /* @hack spelling: the stores through red and alpha made through copies of
     * them, red_out and alpha_out, which give alpha the frame slot at 8 and
     * green and blue those at 12 and 16 (0x8c62b4e0, 0x8c62b4e8, 0x8c62b540);
     * without them, 8 of 437 words differ */
    Float *alpha_out;
    Float *red_out;
    register Float r;
    Float projection_x;
    Float projection_y;
    register Float inverse_z;
    register NJS_MESHSET *meshset;
    register NJS_MATERIAL *material;
    register Uint32 flags;
    Uint32 texture;
    Uint32 translucent;
    register int behind;
    register int left;
    register Float color_scale;
    register Uint16 id;
    Float bound;
    register Uint32 area;
    register struct projected_point *points;

    if (model->nbPoint > _nj_vertex_num_) {
        return;
    }
    if (_nj_control_3d_flag_ & NJD_CONTROL_3D_MODEL_CLIP) {
        njCalcPoint(0, &model->center, &center);
        r = model->r;
        if (!(center.z + r > _nj_near_clip_)) {
            return;
        }
        if (center.z - r > _nj_far_clip_) {
            return;
        }
#ifdef GCC
        /* the picture's aspect, which 16:9 sets in place of Ninja's (port.h) */
        projection_x = _nj_screen_.dist * port_draw_aspect;
#else
        projection_x = _nj_screen_.dist * _nj_scr_aspect_x_;
#endif
        projection_y = _nj_screen_.dist * _nj_scr_aspect_y_;
        inverse_z = 1.0F / (center.z + r);
        bound = (projection_x * (center.x - r) * inverse_z) + _nj_screen_.cx;
        /* @hack redundant: a store to projection_x that never runs, which
         * loads center.x into FR10 and the screen's cx into FR9, and takes
         * this bound in FR8 (0x8c62b410, 0x8c62b420, 0x8c62b42c); without
         * it, the three rotate and 9 of 437 words differ */
        if (0) {
            projection_x = bound;
        }
        if (bound > _nj_2d_clip_[1].x) {
            return;
        }
        bound = (projection_x * (center.x + r) * inverse_z) + _nj_screen_.cx;
        if (bound < _nj_2d_clip_[0].x) {
            return;
        }
        bound = (projection_y * (center.y - r) * inverse_z) + _nj_screen_.cy;
        if (bound > _nj_2d_clip_[1].y) {
            return;
        }
        bound = (projection_y * (center.y + r) * inverse_z) + _nj_screen_.cy;
        if (bound < _nj_2d_clip_[0].y) {
            return;
        }
    }
    meshset = model->meshsets;
    points = (struct projected_point *)_nj_vertex_buf_;
    calc.cache_points = points;
    calc.queue_points =
        (struct projected_point *)(((Uint32)points & SQ_ADDRESS) | SQ_BASE);
    area = SQ_AREA((Uint32)points);
    QACR0 = area;
    QACR1 = area;
    calc.points = model->points;
    calc.normals = model->normals;
    calc.point_count = model->nbPoint;
    _nj_calc_vertex_ += model->nbPoint;
    behind = njEasyCalcModel(&calc);
    if (behind == model->nbPoint) {
        return;
    }
    left = model->nbMeshset;
    red = &color.r;
    red_out = red;
    green = &color.g;
    blue = &color.b;
    /* @hack redundant: a label nothing jumps to, which has SHC form alpha's
     * address in R3, the register that held blue's (MOV R15,R3 at 0x8c62b53a);
     * without it, alpha's address takes R1 and 3 of 437 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-label) */
alpha_setup:
    color_scale = COLOR_SCALE;
    alpha = &color.a;
    alpha_out = alpha;
    do {
        id = meshset->type_matId & MESHSET_MATERIAL;
        material = &model->mats[id];
        flags = material->attrflags;
        if (_nj_control_3d_flag_ & NJD_CONTROL_3D_CONSTANT_ATTR) {
            flags &= _nj_constant_attr_and_;
            flags |= _nj_constant_attr_or_;
        }
        njSetkmModelAttr(flags);
        texture = NJD_FLAG_USE_TEXTURE;
        texture &= flags;
        translucent = NJD_FLAG_USE_ALPHA;
        translucent &= flags;
        if (texture) {
            njSetkmTextureNum(material->attr_texId & MATERIAL_TEXTURE);
            if (translucent) {
                light_tint.a = material->diffuse.argb.a * color_scale;
                njSetkmColorMode(MODE_TEX_TRANS);
                packet_cursor =
                    njStartVertexTexTransIntens((Uint32 *)&light_tint);
            } else {
                njSetkmColorMode(MODE_TEX_OPAQUE);
                packet_cursor =
                    njStartVertexTexOpaqueIntens((Uint32 *)&light_tint);
            }
        } else {
            *red_out = material->diffuse.argb.r * light_tint.r * color_scale;
            *green = material->diffuse.argb.g * light_tint.g * color_scale;
            *blue = material->diffuse.argb.b * light_tint.b * color_scale;
            if (translucent) {
                *alpha_out = material->diffuse.argb.a * color_scale;
                njSetkmColorMode(MODE_TRANS);
                packet_cursor =
                    njStartVertexNonTexTransIntens((Uint32 *)&color);
            } else {
                njSetkmColorMode(MODE_OPAQUE);
                /* @bug uninitialized: an opaque untextured meshset ahead of the
                 * model's first translucent untextured one passes color with
                 * its alpha unset: njStartVertexNonTexOpaqueIntens, called with
                 * &color at 0x8c62b688, reads color.a into the header at
                 * 0x8c658c40 */
                packet_cursor =
                    njStartVertexNonTexOpaqueIntens((Uint32 *)&color);
            }
        }
        prefetch(meshset->meshes);
        if (!behind) {
            if (texture) {
                switch (meshset->type_matId & NJD_MESHSET_MASK) {
                case NJD_MESHSET_TRIMESH:
                    packet_cursor = draw_tex_strips(meshset);
                    break;
                case NJD_MESHSET_3:
                    packet_cursor = draw_tex_triangles(meshset);
                    break;
                case NJD_MESHSET_4:
                    packet_cursor = draw_tex_quads(meshset);
                    break;
                }
            } else {
                switch (meshset->type_matId & NJD_MESHSET_MASK) {
                case NJD_MESHSET_TRIMESH:
                    packet_cursor = draw_solid_strips(meshset);
                    break;
                case NJD_MESHSET_3:
                    packet_cursor = draw_solid_triangles(meshset);
                    break;
                case NJD_MESHSET_4:
                    packet_cursor = draw_solid_quads(meshset);
                    break;
                }
            }
        } else {
            if (texture) {
                switch (meshset->type_matId & NJD_MESHSET_MASK) {
                case NJD_MESHSET_TRIMESH:
                    packet_cursor = draw_tex_strips_near(meshset);
                    break;
                case NJD_MESHSET_3:
                    packet_cursor = draw_tex_triangles_near(meshset);
                    break;
                case NJD_MESHSET_4:
                    packet_cursor = draw_tex_quads_near(meshset);
                    break;
                }
            } else {
                switch (meshset->type_matId & NJD_MESHSET_MASK) {
                case NJD_MESHSET_TRIMESH:
                    packet_cursor = draw_solid_strips_near(meshset);
                    break;
                case NJD_MESHSET_3:
                    packet_cursor = draw_solid_triangles_near(meshset);
                    break;
                case NJD_MESHSET_4:
                    packet_cursor = draw_solid_quads_near(meshset);
                    break;
                }
            }
        }
        njEndVertex((Uint32 *)packet_cursor);
        meshset++;
    } while (--left);
}

void njSetEasyLight(Float x, Float y, Float z)
{
    calc.light_direction.x = x;
    calc.light_direction.y = y;
    calc.light_direction.z = z;
#ifdef GCC
    /* the picture's aspect, which 16:9 sets in place of Ninja's (port.h) */
    calc.projection_x = _nj_screen_.dist * port_draw_aspect;
#else
    calc.projection_x = _nj_screen_.dist * _nj_scr_aspect_x_;
#endif
    calc.projection_y = _nj_screen_.dist * _nj_scr_aspect_y_;
    calc.center_x = _nj_screen_.cx;
    calc.center_y = _nj_screen_.cy;
}

void njSetEasyLightIntensity(Float inten, Float ambient)
{
    calc.negative_diffuse = -inten;
    calc.ambient = ambient;
}

void njSetEasyLightColor(Float r, Float g, Float b)
{
    light_tint.r = r;
    light_tint.g = g;
    light_tint.b = b;
}

#ifdef GCC
NJS_VECTOR *port_easy_light(void)
{
    return &calc.light_direction;
}
#endif

/* draw obj as njEasyDrawObject does, inside the caller's own njGetkmPointer
 * and njSetkmPointer */
void _njEasyDrawObject(NJS_OBJECT *obj)
{
    do {
        switch (obj->evalflags & EVAL_UNIT) {
        case NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL:
            if (!(obj->evalflags & NJD_EVAL_HIDE)) {
                _njEasyDrawModel(obj->model);
            }
            if (!(obj->evalflags & NJD_EVAL_BREAK)) {
                _njEasyDrawObject(obj->child);
            }
            break;
        case 0:
            njPushMatrix(0);
            njTranslate(0, obj->pos[0], obj->pos[1], obj->pos[2]);
            if (obj->evalflags & NJD_EVAL_ZXY_ANG) {
                njRotateZXY(0, obj->ang[0], obj->ang[1], obj->ang[2]);
            } else {
                njRotateXYZ(0, obj->ang[0], obj->ang[1], obj->ang[2]);
            }
            njScale(0, obj->scl[0], obj->scl[1], obj->scl[2]);
            if (!(obj->evalflags & NJD_EVAL_HIDE)) {
                _njEasyDrawModel(obj->model);
            }
            if (!(obj->evalflags & NJD_EVAL_BREAK)) {
                _njEasyDrawObject(obj->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_POS:
            njPushMatrix(0);
            if (obj->evalflags & NJD_EVAL_ZXY_ANG) {
                njRotateZXY(0, obj->ang[0], obj->ang[1], obj->ang[2]);
            } else {
                njRotateXYZ(0, obj->ang[0], obj->ang[1], obj->ang[2]);
            }
            njScale(0, obj->scl[0], obj->scl[1], obj->scl[2]);
            if (!(obj->evalflags & NJD_EVAL_HIDE)) {
                _njEasyDrawModel(obj->model);
            }
            if (!(obj->evalflags & NJD_EVAL_BREAK)) {
                _njEasyDrawObject(obj->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_ANG:
            njPushMatrix(0);
            njTranslate(0, obj->pos[0], obj->pos[1], obj->pos[2]);
            njScale(0, obj->scl[0], obj->scl[1], obj->scl[2]);
            if (!(obj->evalflags & NJD_EVAL_HIDE)) {
                _njEasyDrawModel(obj->model);
            }
            if (!(obj->evalflags & NJD_EVAL_BREAK)) {
                _njEasyDrawObject(obj->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG:
            njPushMatrix(0);
            njScale(0, obj->scl[0], obj->scl[1], obj->scl[2]);
            if (!(obj->evalflags & NJD_EVAL_HIDE)) {
                _njEasyDrawModel(obj->model);
            }
            if (!(obj->evalflags & NJD_EVAL_BREAK)) {
                _njEasyDrawObject(obj->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_SCL:
            njPushMatrix(0);
            njTranslate(0, obj->pos[0], obj->pos[1], obj->pos[2]);
            if (obj->evalflags & NJD_EVAL_ZXY_ANG) {
                njRotateZXY(0, obj->ang[0], obj->ang[1], obj->ang[2]);
            } else {
                njRotateXYZ(0, obj->ang[0], obj->ang[1], obj->ang[2]);
            }
            if (!(obj->evalflags & NJD_EVAL_HIDE)) {
                _njEasyDrawModel(obj->model);
            }
            if (!(obj->evalflags & NJD_EVAL_BREAK)) {
                _njEasyDrawObject(obj->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_SCL:
            njPushMatrix(0);
            if (obj->evalflags & NJD_EVAL_ZXY_ANG) {
                njRotateZXY(0, obj->ang[0], obj->ang[1], obj->ang[2]);
            } else {
                njRotateXYZ(0, obj->ang[0], obj->ang[1], obj->ang[2]);
            }
            if (!(obj->evalflags & NJD_EVAL_HIDE)) {
                _njEasyDrawModel(obj->model);
            }
            if (!(obj->evalflags & NJD_EVAL_BREAK)) {
                _njEasyDrawObject(obj->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL:
            njPushMatrix(0);
            njTranslate(0, obj->pos[0], obj->pos[1], obj->pos[2]);
            if (!(obj->evalflags & NJD_EVAL_HIDE)) {
                _njEasyDrawModel(obj->model);
            }
            if (!(obj->evalflags & NJD_EVAL_BREAK)) {
                _njEasyDrawObject(obj->child);
            }
            njPopMatrix(1);
            break;
        }
        obj = obj->sibling;
    } while (obj);
}

void njEasyDrawModel(NJS_MODEL *model)
{
    njGetkmPointer();
    _njEasyDrawModel(model);
    njSetkmPointer();
}

void njEasyDrawObject(NJS_OBJECT *obj)
{
    njGetkmPointer();
    _njEasyDrawObject(obj);
    njSetkmPointer();
}
