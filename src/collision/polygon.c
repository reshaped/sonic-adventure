/* the collision meshes of the world's objects, and tests of points, balls,
 * rays and segments against those objects */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/mesh.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "object/helpers.h"
#include "stage/stage.h"
#include "system/util.h"
#include "system/vector.h"

/* how far a test looks for triangles: around a point in a mesh or the
 * biggest objects' models, and past a ray's end */
#define REACH 220.0F

/* the longest ray polygon_cast_ray follows */
#define RAY_LENGTH 400.0F

/* the most points a strip holds: strip_next cuts a longer one short */
#define STRIP_POINTS 1024
#define MESHES 192 /* the most objects with meshes at once */
#define MESH_POLYGONS 8192 /* the most polygons in all the meshes */
#define NEAR_POLYGONS 96 /* the most polygon_collect_near lists */

/* the factor polygon_collect_near widens a triangle's squared reach by */
#define NEAR_SCALE 1.3F

/* polygon_cast_segment tries the world's objects within this of the
 * segment's start */
#define SEGMENT_REACH 5.0F

/* how far from pos polygon_find_near_levels starts *above, over pos, and
 * *below, under it. a level with no surface ends as far on the other side */
#define FAR_LEVEL 100000.0F

/* a strip's first index holds its count in these bits, and NJD_TRIMESH_END
 * when its first triangle winds the other way */
#define STRIP_COUNT 0x7fff

/* the debug display prints a pool's use in COLOR_WARNING from this percent
 * on, and goes back to COLOR_TEXT after */
#define USED_WARNING 95
#define COLOR_WARNING 0xffff8080
#define COLOR_TEXT 0xffffffff

/* the evalflags of a node that leaves its matrix as it is, and of one that
 * only moves */
#define EVAL_UNIT (NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL)
#define EVAL_UNTURNED (NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL)

#define DOT(a, b) (((a)->x * (b)->x) + ((a)->y * (b)->y) + ((a)->z * (b)->z))

/* the side of the line from a to b the point lies on, seen in the plane of
 * axes u and v. the sign gives the side, and 0 is on the line. a point
 * inside a triangle gives its three edges the same sign */
#define EDGE_SIDE(a, b, point, u, v)                                           \
    ((((b)->u - (a)->u) * ((point)->v - (a)->v)) -                             \
     (((b)->v - (a)->v) * ((point)->u - (a)->u)))

/* the normal made a unit vector, or straight up when it has no length */
#define UNIT_NORMAL(normal)                                                    \
    if (njScalor(normal) == 0.0F) {                                            \
        (normal)->x = 0.0F;                                                    \
        (normal)->y = 1.0F;                                                    \
        (normal)->z = 0.0F;                                                    \
    } else {                                                                   \
        njUnitVector(normal);                                                  \
    }

/* the vector made a unit vector, or left with no length */
#define UNIT_VECTOR(vector)                                                    \
    if (njScalor(vector) == 0.0F) {                                            \
        (vector)->x = (vector)->y = (vector)->z = 0.0F;                        \
    } else {                                                                   \
        njUnitVector(vector);                                                  \
    }

/* util.h's TURN, a turn by ang unless it's 0, as an if statement, which
 * polygon_sweep and polygon_cast_ray turn with */
/* @hack redundant: an empty else, which settles R2 and R3 in the code after the
 * turns. polygon_sweep loads *near into R3 and NJD_FLAG_DOUBLE_SIDE into R2
 * (0x8c02a7ac), and polygon_cast_ray copies at through R3 and R2
 * (0x8c02b49e); without it, the pairs trade and, summed over the two, 30 of
 * 1869 words differ */
#define TURN_IF(rotate, ang)                                                   \
    if (ang) {                                                                 \
        (rotate)(NULL, (ang));                                                 \
    } else {                                                                   \
    }

#define WEIGH(value)                                                           \
    /* @hack redundant: a test of value that never runs. SHC writes no code    \
     * for it, but counts the use when it gives out registers. the comment at  \
     * each use, or at its function, says what the use settles. without it,    \
     * summed over the five functions that use it, 859 of 4638 words differ */ \
    if (0) {                                                                   \
        if (value) {                                                           \
        }                                                                      \
    }

/* a cursor through a model's meshsets: the points of the polygon or strip it
 * has reached, in the space its strip_next gives them */
struct strip {
    int kind; /* the meshset's type, NJD_MESHSET_3 to NJD_MESHSET_TRIMESH */
    int count; /* of points */
    /* the next polygon's indices in the meshset, or a strip's count ahead of
     * them */
    Sint16 *indices;
    NJS_MODEL *model;
    NJS_POINT3 points[STRIP_POINTS];
};

/* a ball's sweep over the models of an object's tree */
struct sweep {
    Bool hit;
    int count; /* of the contacts it has found */
    int tested; /* the triangles that passed the reach test */
    Float reach; /* SIZE_REACH2 of the object's flags, which goes unread */
    Float length; /* of the ball's speed */
    Float spacing; /* how far the ball goes between tests: half its radius */
    struct polygon_contact *contacts;
};

/* a triangle polygon_sweep touched: a copy of it, and the contact */
struct touch_polygon {
    Sint32 unk_00;
    struct polygon polygon;
    struct polygon_contact contact;
};

struct polygon_contact polygon_contacts[POLYGON_CONTACTS];
/* where polygon_pool_seek_free looks from, and its index in polygon_pool.
 * only polygon_pool_seek_free writes them, and polygon_cursor starts
 * null */
static struct polygon *polygon_cursor;
static int polygon_cursor_index;
/* 4 bytes of unknown type, which no code names */
static Uint32 dat_8c6c4bb0;
#ifdef GCC
/* both on 32 bytes, so that each 64-byte record fills two cache lines. a
 * walk of a mesh reads each polygon's next, center and radius, its first 24
 * bytes, which then lie in one line. in 1ST_READ.BIN both pools start 20
 * bytes into a line, where a record spans three lines and those 24 bytes
 * two */
static struct collision_mesh mesh_pool[MESHES] __attribute__((aligned(32)));
static struct polygon polygon_pool[MESH_POLYGONS] __attribute__((aligned(32)));
#else
static struct collision_mesh mesh_pool[MESHES];
static struct polygon polygon_pool[MESH_POLYGONS];
#endif
/* the triangles polygon_collect_near listed, mesh by mesh */
static int near_polygon_count;
static struct polygon *near_polygons[NEAR_POLYGONS];
/* the triangles polygon_sweep has touched */
static int touch_polygon_count;
static struct touch_polygon touch_polygons[POLYGON_CONTACTS];
/* an object whose mesh the next update makes again. it is only ever set to
 * null */
static NJS_OBJECT *mesh_stale_object;
/* the corners of the triangle polygon_sweep last touched, moved a unit
 * along its normal, which touch_triangle draws */
static NJS_POINT3 touch_points[3];
/* what collision_mesh_update knows of the mesh it is making */
static Float mesh_center_z; /* the z its buckets sort centers from */
static Float mesh_span; /* the longest any polygon spans along z so far */
static Float dat_8c74872c; /* set to 0.0F, and copied to the mesh's unk_2c */
/* polygon_sweep's matrix, kept while it draws a touched triangle */
static NJS_MATRIX touch_matrix;

/* the touched triangle on the debug display: touch_points, with its corners
 * red, green and blue */
static NJS_COLOR touch_colors[3] = {
#include "collision/polygon/touch_colors.inc"
};
static NJS_POINT3COL touch_triangle = {
#include "collision/polygon/touch_triangle.inc"
};
/* set by collision_mesh_collect_all, and cleared by the update that then
 * takes every object of the world */
static Bool mesh_collect_all = FALSE;

/* how far a test of a model reaches from its point for an object's
 * triangles, squared, by the object's size from WORLD_HUGE to WORLD_SMALL */
static const Float size_reach2[4] = {
    REACH * REACH,
    202.0F * 202.0F,
    101.0F * 101.0F,
    30.3F * 30.3F,
};

/* size_reach2's entry for the size in an object's world flags */
/* @bug memory: the size bits shifted down 13 places, not 29, so an
 * object smaller than WORLD_HUGE reads entry 0x10000, 0x20000 or 0x30000,
 * 0xfffc or more entries past the table's end, where the image holds
 * -1958.1, 0.0 and the denormal 0x002e0018. the shift is the SHLD at
 * 0x8c028dde, 0x8c02c6e6 and 0x8c02cb9a, and the reads the FMOV.S at
 * 0x8c028de4, 0x8c02c6ec and 0x8c02cba2 */
#ifdef GCC
/* repairs the shift the tag names: GCC reads size_reach2 at the size's own
 * entry */
#define SIZE_REACH2(flags) size_reach2[((flags) & WORLD_SIZE) >> 29]
#else
#define SIZE_REACH2(flags) size_reach2[((flags) & WORLD_SIZE) >> 13]
#endif

static NJS_OBJECT *segment_cross_mesh(struct segment_query *query,
                                      NJS_OBJECT *object, Sint32 flags);
static Bool find_model_heights_near(struct height_query *query,
                                    NJS_OBJECT *object, Uint32 flags);

/* as polygon_normal does */
/* @hack type: the points as byte cursors, stepped a Float at a time, for
 * the image's @Rn+ loads; as NJS_POINT3 pointers read through byte cursors
 * of their own, 10 of 33 words differ */
static void triangle_normal(const char *v0, const char *v1, const char *v2,
                            NJS_VECTOR *normal)
{
    Float ux;
    Float uy;
    Float uz;
    Float vx;
    Float vy;
    Float vz;

    vx = *(const Float *)v0;
    v0 += sizeof(Float);
    ux = *(const Float *)v1 - vx;
    v1 += sizeof(Float);
    uy = *(const Float *)v1 - (vy = *(const Float *)v0);
    v1 += sizeof(Float);
    v0 += sizeof(Float);
    vz = *(const Float *)v0;
    uz = *(const Float *)v1;
    vx -= *(const Float *)v2;
    v2 += sizeof(Float);
    vy -= *(const Float *)v2;
    v2 += sizeof(Float);
    uz -= vz;
    vz -= *(const Float *)v2;
    normal->x = (uz * vy) - (uy * vz);
    normal->y = (ux * vz) - (uz * vx);
    normal->z = (uy * vx) - (ux * vy);
}

/* move the cursor on to the meshset's next polygon or strip, and put its
 * points, through the current matrix, in strip->points. returns TRUE when a
 * strip's first triangle winds the other way, and FALSE otherwise */
static Bool strip_next(struct strip *strip)
{
    NJS_POINT3 *points;
    NJS_POINT3 *out;
    Sint16 *indices;
    int i;
    Bool reversed = FALSE;

    switch (strip->kind) {
    case NJD_MESHSET_3:
    case NJD_MESHSET_4:
        indices = strip->indices;
        points = strip->model->points;
        out = strip->points;
        for (i = strip->count; i > 0; i--) {
            njCalcPoint(NULL, &points[*indices], out);
            indices++;
            out++;
        }
        strip->indices = indices;
        break;
    case NJD_MESHSET_N:
    case NJD_MESHSET_TRIMESH:
        strip->count = *strip->indices & STRIP_COUNT;
        if (*strip->indices & NJD_TRIMESH_END) {
            reversed = TRUE;
        }
        if (strip->count > STRIP_POINTS) {
            strip->count = STRIP_POINTS;
            if (debug_hidden >= 0) {
                njPrintC(NJM_LOCATION(2, 18), "WARNING: STRIP buf is FULL");
            }
        }
        strip->indices++;
        points = strip->model->points;
        out = strip->points;
        indices = strip->indices;
        for (i = strip->count; i > 0; i--) {
            njCalcPoint(NULL, &points[*indices], out);
            indices++;
            out++;
        }
        strip->indices = indices;
        break;
    }
    return reversed;
}

/* strip_next, with the points as the model has them */
static Bool strip_next_plain(struct strip *strip)
{
    NJS_POINT3 *points;
    NJS_POINT3 *out;
    Sint16 *indices;
    int i;
    Bool reversed = FALSE;

    switch (strip->kind) {
    case NJD_MESHSET_3:
    case NJD_MESHSET_4:
        indices = strip->indices;
        points = strip->model->points;
        out = strip->points;
        for (i = strip->count; i > 0; i--) {
            *out = points[*indices];
            indices++;
            out++;
        }
        strip->indices = indices;
        break;
    case NJD_MESHSET_N:
    case NJD_MESHSET_TRIMESH:
        strip->count = *strip->indices & STRIP_COUNT;
        if (*strip->indices & NJD_TRIMESH_END) {
            reversed = TRUE;
        }
        if (strip->count > STRIP_POINTS) {
            strip->count = STRIP_POINTS;
            if (debug_hidden >= 0) {
                njPrintC(NJM_LOCATION(2, 18), "WARNING: STRIP buf is FULL");
            }
        }
        strip->indices++;
        points = strip->model->points;
        out = strip->points;
        indices = strip->indices;
        for (i = strip->count; i > 0; i--) {
            *out = points[*indices];
            indices++;
            out++;
        }
        strip->indices = indices;
        break;
    }
    return reversed;
}

/* strip_next, with the points moved by the object's position */
static Bool strip_next_moved(struct strip *strip, NJS_OBJECT *object)
{
    NJS_POINT3 *points;
    NJS_POINT3 *out;
    Float *point;
    Float x;
    Float y;
    Float z;
    Sint16 *indices;
    int i;
    Bool reversed = FALSE;

    switch (strip->kind) {
    case NJD_MESHSET_3:
    case NJD_MESHSET_4:
        x = object->pos[0];
        y = object->pos[1];
        z = object->pos[2];
        indices = strip->indices;
        points = strip->model->points;
        out = strip->points;
        for (i = strip->count; i > 0; i--) {
            point = &points[*indices].x;
            out->x = *point++ + x;
            out->y = *point++ + y;
            out->z = *point + z;
            indices++;
            out++;
        }
        strip->indices = indices;
        break;
    case NJD_MESHSET_N:
    case NJD_MESHSET_TRIMESH:
        strip->count = *strip->indices & STRIP_COUNT;
        if (*strip->indices & NJD_TRIMESH_END) {
            reversed = TRUE;
        }
        if (strip->count > STRIP_POINTS) {
            strip->count = STRIP_POINTS;
            if (debug_hidden >= 0) {
                njPrintC(NJM_LOCATION(2, 18), "WARNING: STRIP buf is FULL");
            }
        }
        strip->indices++;
        y = object->pos[1];
        z = object->pos[2];
        x = object->pos[0];
        points = strip->model->points;
        indices = strip->indices;
        out = strip->points;
        for (i = strip->count; i > 0; i--) {
            point = &points[*indices].x;
            out->x = *point++ + x;
            out->y = *point++ + y;
            out->z = *point + z;
            indices++;
            out++;
        }
        strip->indices = indices;
        break;
    }
    return reversed;
}

/* sweep the ball over the triangles of object's own model through the
 * current matrix. each triangle it touches joins the sweep's contacts. it
 * starts no meshset or strip once the sweep holds POLYGON_CONTACTS, but
 * goes on to the end of the strip it is in */
static void sweep_model(struct polygon_ball *ball, NJS_OBJECT *object,
                        struct sweep *sweep)
{
    struct strip strip;
    NJS_POINT3 tri[3];
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    NJS_VECTOR foot;
    NJS_VECTOR normal;
    NJS_POINT3 out;
    NJS_VECTOR part;
    NJS_VECTOR stride;
    NJS_POINT3 at;
    NJS_MESHSET *meshset;
    NJS_POINT3 *corner;
    struct polygon_contact *contact;
    int meshsets;
    int strips;
    int i;
    Bool reversed;
    Bool hit;
    Bool more;
    Uint32 two_sided;
    Angle ang_x;
    Angle ang_y;
    Float *v0;
    Float *v1;
    Float *v2;
    char *edge1;
    char *edge2;
    Float dx;
    Float dy;
    Float dz;
    Float reach;
    Float edge;
    Float x;
    Float y;
    Float z;
    Float x0;
    Float y0;
    Float z0;
    Float x1;
    Float y1;
    Float z1;
    Float radius;
    Float travelled;
    Float e1;
    Float e2;
    Float e3;

    strip.model = object->model;
    meshsets = strip.model->nbMeshset;
    for (meshset = strip.model->meshsets;
         meshsets > 0 && sweep->count < POLYGON_CONTACTS;
         meshsets--, meshset++) {
        switch (strip.kind = meshset->type_matId & NJD_MESHSET_MASK) {
        case NJD_MESHSET_3:
            strip.count = 3;
            break;
        case NJD_MESHSET_4:
            strip.count = 4;
            break;
        case NJD_MESHSET_N:
        case NJD_MESHSET_TRIMESH:
            break;
        default:
            continue;
        }
        strips = meshset->nbMesh;
        strip.indices = meshset->meshes;
        for (; strips != 0 && sweep->count < POLYGON_CONTACTS; strips--) {
            reversed = strip_next(&strip);
            tri[0].x = tri[0].y = tri[0].z = 0.0F;
#ifdef GCC
            /* repairs the writes the tag in this loop names: GCC's loop tests
             * the count, as the strips' and meshsets' loops do */
            for (i = 0, corner = strip.points;
                 i < strip.count - 2 && sweep->count < POLYGON_CONTACTS;
                 i++, corner++) {
#else
            for (i = 0, corner = strip.points; i < strip.count - 2;
                 i++, corner++) {
#endif
                /* @hack spelling: Float cursors stepped across the corners'
                 * members from &corner->x, + 3 and + 6, which SHC forms again
                 * in each test and arm; as corner[0] to corner[2]'s members,
                 * 504 of 1052 words differ */
                v0 = &corner->x;
                v1 = &corner->x + 3;
                v2 = &corner->x + 6;
                x1 = *v1++;
                y1 = *v1++;
                z1 = *v1;
                dx = x1 - *v0++;
                dy = y1 - *v0++;
                dz = z1 - *v0++;
                reach = (dx * dx) + (dy * dy) + (dz * dz);
                dx = *v2++ - x1;
                dy = *v2++ - y1;
                dz = *v2++ - z1;
                if ((edge = (dx * dx) + (dy * dy) + (dz * dz)) > reach) {
                    reach = edge;
                }
                v0 -= 3;
                v2 -= 3;
                dx = *v0++ - *v2++;
                dy = *v0++ - *v2++;
                dz = *v0 - *v2;
                if ((edge = (dx * dx) + (dy * dy) + (dz * dz)) > reach) {
                    reach = edge;
                }
                reach += (ball->speed.x * ball->speed.x) +
                         (ball->speed.y * ball->speed.y) +
                         (ball->speed.z * ball->speed.z) +
                         (ball->radius * ball->radius);
                /* @hack spelling: a Float cursor stepped across the members of
                 * the ball's pos; as ball->pos.x, .y and .z, 313 of 1052 words
                 * differ */
                v0 = &ball->pos.x;
                x = *v0++;
                y = *v0++;
                z = *v0;
                /* @hack spelling: Float cursors stepped across the corners'
                 * members again, in the reach tests and both arms below; as
                 * corner[0] to corner[2]'s members, the function runs 32 bytes
                 * long and 950 of 1052 words differ */
                v0 = &corner->x;
                dx = x - *v0++;
                dy = y - *v0++;
                dz = z - *v0;
                if ((dx * dx) + (dy * dy) + (dz * dz) > reach) {
                    continue;
                }
                v0 = &corner->x + 3;
                dx = x - *v0++;
                dy = y - *v0++;
                dz = z - *v0;
                if ((dx * dx) + (dy * dy) + (dz * dz) > reach) {
                    continue;
                }
                v0 = &corner->x + 6;
                dx = x - *v0++;
                dy = y - *v0++;
                dz = z - *v0;
                if ((dx * dx) + (dy * dy) + (dz * dz) > reach) {
                    continue;
                }
                sweep->tested++;
                /* @hack spelling: edge1 and edge2, byte cursors over tri[1] and
                 * tri[2], which SHC reloads for each store; through the
                 * members, the function runs 100 bytes short and 1022 of
                 * 1052 words differ */
                if (!((i + reversed) & 1)) {
                    edge1 = (char *)&tri[1];
                    edge2 = (char *)&tri[2];
                    v1 = &corner->x + 3;
                    v0 = &corner->x;
                    x0 = *v0++;
                    y0 = *v0++;
                    z0 = *v0;
                    *(Float *)edge1 = *v1++ - x0;
                    *(Float *)(edge1 + sizeof(Float)) = *v1++ - y0;
                    *(Float *)(edge1 + (2 * sizeof(Float))) = *v1 - z0;
                    v2 = &corner->x + 6;
                    *(Float *)edge2 = *v2++ - x0;
                    *(Float *)(edge2 + sizeof(Float)) = *v2++ - y0;
                    *(Float *)(edge2 + (2 * sizeof(Float))) = *v2 - z0;
                } else {
                    edge1 = (char *)&tri[1];
                    edge2 = (char *)&tri[2];
                    v2 = &corner->x + 6;
                    v0 = &corner->x;
                    x0 = *v0++;
                    y0 = *v0++;
                    z0 = *v0;
                    *(Float *)edge1 = *v2++ - x0;
                    *(Float *)(edge1 + sizeof(Float)) = *v2++ - y0;
                    *(Float *)(edge1 + (2 * sizeof(Float))) = *v2 - z0;
                    v1 = &corner->x + 3;
                    *(Float *)edge2 = *v1++ - x0;
                    *(Float *)(edge2 + sizeof(Float)) = *v1++ - y0;
                    *(Float *)(edge2 + (2 * sizeof(Float))) = *v1 - z0;
                }
                if ((tri[0].x == tri[1].x && tri[0].y == tri[1].y &&
                     tri[0].z == tri[1].z) ||
                    (tri[0].x == tri[2].x && tri[0].y == tri[2].y &&
                     tri[0].z == tri[2].z) ||
                    (tri[1].x == tri[2].x && tri[1].y == tri[2].y &&
                     tri[1].z == tri[2].z)) {
                    continue;
                }
                pos.x = ball->pos.x - corner->x;
                pos.y = ball->pos.y - corner->y;
                pos.z = ball->pos.z - corner->z;
                speed.x = ball->speed.x;
                speed.y = ball->speed.y;
                speed.z = ball->speed.z;
                /* @hack type: each point's x converted back to its
                 * NJS_POINT3, which SHC addresses apart from tri's base; as
                 * (const char *)&tri[0] and so on, the function runs 4 bytes
                 * long and 519 of 1052 words differ */
                triangle_normal((const char *)(NJS_POINT3 *)&tri[0].x,
                                (const char *)(NJS_POINT3 *)&tri[1].x,
                                (const char *)(NJS_POINT3 *)&tri[2].x, &normal);
                UNIT_NORMAL(&normal);
                ang_x = (Angle)(acosf(normal.y) * ANGLE_PER_RADIAN);
                ang_y = (Angle)(atan2f(normal.x, normal.z) * ANGLE_PER_RADIAN);
                njPushMatrix(&_unit_matrix_);
                TURN(njRotateX, -ang_x);
                TURN(njRotateY, -ang_y);
                njCalcPoint(NULL, &tri[1], &out);
                tri[1] = out;
                njCalcPoint(NULL, &tri[2], &out);
                tri[2] = out;
                njCalcPoint(NULL, &pos, &out);
                pos = out;
                njCalcPoint(NULL, &speed, &out);
                speed = out;
                two_sided =
                    strip.model->mats[meshset->type_matId & ~NJD_MESHSET_MASK]
                        .attrflags &
                    NJD_FLAG_DOUBLE_SIDE;
                radius = ball->radius;
                if (two_sided) {
                    if (pos.y < 0.0F) {
                        radius *= -1.0F;
                    }
                } else {
                    /* @hack redundant: an empty else, which has the copy of
                     * speed into stride take speed's address in R2 and stride's
                     * in R3 (0x8c028688, 0x8c02868e); without it, the two trade
                     * and 11 of 1052 words differ */
                }
                stride = speed;
                hit = FALSE;
                UNIT_VECTOR(&stride);
                travelled = 0.0F;
                stride.x *= sweep->spacing;
                stride.y *= sweep->spacing;
                stride.z *= sweep->spacing;
                at = pos;
                for (;;) {
                    more = TRUE;
                    if (travelled >= sweep->length) {
                        /* @hack spelling: Float cursors stepped across the
                         * members of pos and speed; as pos.x, speed.x and so
                         * on, 314 of 1052 words differ */
                        v0 = &pos.x;
                        v1 = &speed.x;
                        at.x = *v0++ + *v1++;
                        at.y = *v0++ + *v1++;
                        at.z = *v0 + *v1;
                        more = FALSE;
                    }
                    if (njAbs(at.y) <= ball->radius) {
                        e1 = EDGE_SIDE(&tri[0], &tri[1], &at, z, x);
                        e2 = EDGE_SIDE(&tri[1], &tri[2], &at, z, x);
                        e3 = EDGE_SIDE(&tri[2], &tri[0], &at, z, x);
                        if (e1 * e2 >= 0.0F && e2 * e3 >= 0.0F &&
                            e1 * e3 >= 0.0F) {
                            hit = TRUE;
                            break;
                        }
                    }
                    if (!more) {
                        break;
                    }
                    /* WEIGH(more) here puts &tri[1] at R15+8 and R15+12,
                     * and &tri[2] at R15+4 and R15+16 (0x8c0281d4 to
                     * 0x8c0281da). it gives the third reach test's dx and dy
                     * FR8 and FR6 (0x8c0283de). it gives the first arm's v0
                     * and v1 R5 and R4 (0x8c028410). without it, 191 of 1052
                     * words differ */
                    WEIGH(more);
                    at.x += stride.x;
                    at.y += stride.y;
                    at.z += stride.z;
                    travelled += sweep->spacing;
                }
                if (!hit) {
                    njPopMatrix(1);
                    continue;
                }
                sweep->hit = TRUE;
                /* @bug memory: the triangles' loop doesn't test the
                 * count, so a strip that brings the contacts past
                 * POLYGON_CONTACTS writes the rest past polygon_contacts'
                 * end. the strips' and meshsets' loops test it, at
                 * 0x8c028b06 and 0x8c028b24. the triangles' loop, CMP/GE
                 * R3,R2 at 0x8c028af2, doesn't. the store indexes the
                 * contacts by the count (MUL.L R3,R2 at 0x8c0288b0) */
                contact = &sweep->contacts[sweep->count];
                contact->height = pos.y;
                contact->height_after = pos.y + speed.y;
                contact->hit.front = normal;
                if (two_sided) {
                    if (pos.y >= 0.0F) {
                        contact->hit.normal = normal;
                    } else {
                        contact->hit.normal.x = -normal.x;
                        contact->hit.normal.y = -normal.y;
                        contact->hit.normal.z = -normal.z;
                    }
                } else {
                    contact->hit.normal = normal;
                }
                contact->hit.ang_z = -(Angle)(atan2f(contact->hit.normal.x,
                                                     contact->hit.normal.y) *
                                              ANGLE_PER_RADIAN);
                contact->hit.ang_x =
                    (Angle)(asinf(contact->hit.normal.z) * ANGLE_PER_RADIAN);
                njInvertMatrix(NULL);
                foot.x = pos.x;
                foot.y = 0.0F;
                foot.z = pos.z;
                njCalcPoint(NULL, &foot, &out);
                contact->hit.point.x = out.x + corner->x;
                contact->hit.point.y = out.y + corner->y;
                contact->hit.point.z = out.z + corner->z;
                foot.y = radius;
                njCalcPoint(NULL, &foot, &out);
                contact->hit.pos.x = out.x + corner->x;
                contact->hit.pos.y = out.y + corner->y;
                contact->hit.pos.z = out.z + corner->z;
                part = speed;
                /* @hack redundant: the negation of part.y, which the store of
                 * 0.0F below overwrites (FMOV.S FR4,@(R0,R15) at 0x8c028a30
                 * and 0x8c028a6c, then FMOV.S FR15,@(R0,R15) at 0x8c028a7e,
                 * each R0 76); without it, the function runs 48 bytes short
                 * and 331 of 1052 words differ */
                if (two_sided) {
                    if (pos.y * speed.y < 0.0F) {
                        part.y = -speed.y;
                    }
                } else if (radius * speed.y < 0.0F) {
                    part.y = -speed.y;
                }
                part.x = speed.x;
                part.y = 0.0F;
                part.z = speed.z;
                njCalcPoint(NULL, &part, &out);
                contact->hit.speed = out;
                part.x = 0.0F;
                part.y = speed.y;
                part.z = 0.0F;
                njCalcPoint(NULL, &part, &out);
                contact->hit.normal_speed = out;
                njPopMatrix(1);
                sweep->count++;
            }
        }
    }
    /* WEIGH(strip.kind == NJD_MESHSET_TRIMESH) here puts ang_y at R15+32 and
     * meshsets at R15+36 (0x8c0285ac, 0x8c0281bc). without it, the two trade
     * and 6 of 1052 words differ */
    WEIGH(strip.kind == NJD_MESHSET_TRIMESH);
}

/* give the ball the hit of the sweep's nearest contact, and its index, with
 * point and pos averaged over all the contacts */
static void sweep_take_nearest(struct polygon_ball *ball, struct sweep *sweep)
{
    struct polygon_ball sum;
    struct polygon_contact *contact;
    int i;
    int nearest = 0;
    Float height;
    Float scale;

    /* the first is measured by its height before the move, and the rest
     * by theirs after */
    height = njAbs(sweep->contacts[0].height);
    for (i = 1; i < sweep->count; i++) {
        if (height > njAbs(sweep->contacts[i].height_after)) {
            nearest = i;
            height = njAbs(sweep->contacts[i].height_after);
        }
    }
    ball->hit_index = nearest;
    contact = &sweep->contacts[nearest];
    ball->hit.ang_x = contact->hit.ang_x;
    ball->hit.ang_z = contact->hit.ang_z;
    ball->hit.front = contact->hit.front;
    ball->hit.normal = contact->hit.normal;
    ball->hit.speed = contact->hit.speed;
    ball->hit.normal_speed = contact->hit.normal_speed;
    sum.hit.point.x = sum.hit.point.y = sum.hit.point.z = 0.0F;
    sum.hit.pos.x = sum.hit.pos.y = sum.hit.pos.z = 0.0F;
    scale = 1.0F / sweep->count;
    for (i = sweep->count, contact = sweep->contacts; i > 0; i--) {
        sum.hit.point.x += scale * contact->hit.point.x;
        sum.hit.point.y += scale * contact->hit.point.y;
        sum.hit.point.z += scale * contact->hit.point.z;
        sum.hit.pos.x += scale * contact->hit.pos.x;
        sum.hit.pos.y += scale * contact->hit.pos.y;
        sum.hit.pos.z += scale * contact->hit.pos.z;
        contact++;
    }
    ball->hit.point = sum.hit.point;
    ball->hit.pos = sum.hit.pos;
}

/* sweep the ball over the trees of object's children, then over its own
 * model, through its transform, and return its sibling */
static NJS_OBJECT *sweep_tree(struct polygon_ball *ball, NJS_OBJECT *object,
                              struct sweep *sweep)
{
    NJS_OBJECT *child;

    if ((object->evalflags & EVAL_UNIT) ^ EVAL_UNIT) {
        njPushMatrix(NULL);
        if (!(object->evalflags & NJD_EVAL_UNIT_POS)) {
            njTranslateV(NULL, (NJS_VECTOR *)object->pos);
        }
        if (!(object->evalflags & NJD_EVAL_UNIT_ANG)) {
            TURN(njRotateZ, object->ang[2]);
            TURN(njRotateY, object->ang[1]);
            TURN(njRotateX, object->ang[0]);
        }
        if (!(object->evalflags & NJD_EVAL_UNIT_SCL)) {
            njScaleV(NULL, (NJS_VECTOR *)object->scl);
        }
    }
    child = object->child;
    while (child != NULL) {
        child = sweep_tree(ball, child, sweep);
    }
    sweep_model(ball, object, sweep);
    if ((object->evalflags & EVAL_UNIT) ^ EVAL_UNIT) {
        njPopMatrix(1);
    }
    return object->sibling;
}

Bool polygon_sweep_model(struct polygon_ball *ball, NJS_OBJECT *object,
                         Uint32 flags)
{
    struct sweep sweep;
    NJS_OBJECT *child;

    sweep.hit = FALSE;
    sweep.count = 0;
    sweep.tested = 0;
    sweep.contacts = polygon_contacts;
    sweep.reach = SIZE_REACH2(flags);
    sweep.length = njScalor(&ball->speed);
    sweep.spacing = ball->radius * 0.5F;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    if (!(object->evalflags & NJD_EVAL_UNIT_POS)) {
        njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    }
    if (!(object->evalflags & NJD_EVAL_UNIT_ANG)) {
        TURN(njRotateZ, object->ang[2]);
        TURN(njRotateY, object->ang[1]);
        TURN(njRotateX, object->ang[0]);
    }
    if (!(object->evalflags & NJD_EVAL_UNIT_SCL)) {
        njScaleV(NULL, (NJS_VECTOR *)object->scl);
    }
    child = object->child;
    while (child != NULL) {
        child = sweep_tree(ball, child, &sweep);
    }
    sweep_model(ball, object, &sweep);
    if (sweep.count > 0) {
        sweep_take_nearest(ball, &sweep);
        ball->count = sweep.count;
    }
    njPopMatrix(1);
    return sweep.hit;
}

void polygon_find_near_levels(NJS_POINT3 *pos, Float radius, Float *above,
                              Angle *above_x, Angle *above_z, Float *below,
                              Angle *below_x, Angle *below_z)
{
    struct line_query query;
    struct world_object *objects;
    Bool found_below;
    Bool found_above;
    int i;
    Float level;

    *above = pos->y + FAR_LEVEL;
    *below = pos->y - FAR_LEVEL;
    found_above = FALSE;
    found_below = FALSE;
    objects = world_objects;
    world_list_near(pos->x, pos->y, pos->z, radius);
    query.pos.x = pos->x;
    query.pos.y = pos->y;
    query.pos.z = pos->z;
    for (i = 0; i < world_count; i++) {
        /* @bug uninitialized: polygon_find_nearest_height adds query's
         * radius and speed to its reach, and nothing here sets them (FMOV.S
         * at 0x8c02c372, 0x8c02c374, 0x8c02c378 and 0x8c02c384) */
        if (polygon_find_nearest_height(&query, objects[i].object,
                                        objects[i].flags) == TRUE) {
            level = query.level;
            if (level > pos->y) {
                if (*above > level) {
                    *above = level;
                    if (above_x != NULL) {
                        *above_x = query.ang_x;
                    }
                    if (above_z != NULL) {
                        *above_z = query.ang_z;
                    }
                    found_above = TRUE;
                }
            } else if (level > *below) {
                *below = level;
                if (below_x != NULL) {
                    *below_x = query.ang_x;
                }
                if (below_z != NULL) {
                    *below_z = query.ang_z;
                }
                found_below = TRUE;
            }
        }
    }
    if (!found_above) {
        *above = pos->y - FAR_LEVEL;
    }
    if (!found_below) {
        *below = pos->y + FAR_LEVEL;
    }
}

void collision_mesh_link_pools(void)
{
    polygon_pool_init(polygon_pool, MESH_POLYGONS);
    collision_mesh_pool_init(mesh_pool, MESHES);
}

void polygon_pool_seek_free(void)
{
    /* @bug memory: nothing points polygon_cursor into polygon_pool, so the
     * first call would read radius through null (FMOV.S @(R0,R1),FR2 at
     * 0x8c0290b4). nothing in the images calls polygon_pool_seek_free or
     * holds its address */
    while (polygon_cursor_index < MESH_POLYGONS &&
           polygon_cursor->radius != 0.0F) {
        polygon_cursor_index++;
        polygon_cursor++;
    }
    if (polygon_cursor_index >= MESH_POLYGONS) {
        polygon_cursor = NULL;
    }
}

/* as strip_next does, for a mesh being made */
static Bool mesh_strip_next(struct strip *strip)
{
    NJS_POINT3 *point;
    int i;
    Bool reversed = FALSE;

    switch (strip->kind) {
    case NJD_MESHSET_3:
    case NJD_MESHSET_4:
        for (i = strip->count, point = strip->points; i > 0; i--, point++) {
            njCalcPoint(NULL, &strip->model->points[*strip->indices], point);
            strip->indices++;
        }
        break;
    case NJD_MESHSET_N:
    case NJD_MESHSET_TRIMESH:
        strip->count = *strip->indices & STRIP_COUNT;
        if (*strip->indices & NJD_TRIMESH_END) {
            reversed = TRUE;
        }
        if (strip->count > STRIP_POINTS) {
            strip->count = STRIP_POINTS;
            if (debug_hidden >= 0) {
                njPrintC(NJM_LOCATION(2, 18), "WARNING: STRIP buf is FULL");
            }
        }
        strip->indices++;
        for (i = strip->count, point = strip->points; i > 0; i--, point++) {
            njCalcPoint(NULL, &strip->model->points[*strip->indices], point);
            strip->indices++;
        }
        break;
    }
    return reversed;
}

/* put the triangles of object's own model, through the current matrix, in
 * the buckets of the mesh being made. returns FALSE when the polygons run
 * out, and TRUE otherwise. flags and mesh go unread */
static Bool mesh_add_model(NJS_OBJECT *object, Sint32 flags,
                           struct collision_mesh *mesh)
{
    struct strip strip;
    NJS_MESHSET *meshset;
    NJS_MATERIAL *mats;
    struct polygon *polygon;
    NJS_POINT3 *corner;
    int meshsets;
    int strips;
    Bool reversed;
    int i;
    Float x0;
    Float y0;
    Float z0;
    Float x1;
    Float y1;
    Float z1;
    Float x2;
    Float y2;
    Float z2;
    Float cx;
    Float cy;
    Float cz;
    Float span;
    Float span1;
    Float span2;
    Float radius;
    Float radius1;
    Float third;

    strip.model = object->model;
    meshsets = strip.model->nbMeshset;
    for (meshset = strip.model->meshsets; meshsets > 0; meshsets--, meshset++) {
        switch (strip.kind = meshset->type_matId & NJD_MESHSET_MASK) {
        case NJD_MESHSET_3:
            strip.count = 3;
            break;
        case NJD_MESHSET_4:
            strip.count = 4;
            break;
        case NJD_MESHSET_N:
        case NJD_MESHSET_TRIMESH:
            break;
        default:
            continue;
        }
        strips = meshset->nbMesh;
        strip.indices = meshset->meshes;
        for (; strips > 0; strips--) {
            reversed = mesh_strip_next(&strip);
            for (i = 0, corner = strip.points; i < strip.count - 2;
                 i++, corner++) {
                x0 = corner[0].x;
                x1 = corner[1].x;
                x2 = corner[2].x;
                y0 = corner[0].y;
                y1 = corner[1].y;
                y2 = corner[2].y;
                z0 = corner[0].z;
                z1 = corner[1].z;
                z2 = corner[2].z;
                third = 0.3333F;
                cx = (x0 + x1 + x2) * third;
                cy = (y0 + y1 + y2) * third;
                cz = (z0 + z1 + z2) * third;
                if ((polygon = polygon_alloc(cz - mesh_center_z)) != NULL) {
                    if (!((reversed + i) & 1)) {
                        polygon->vertex[0].x = x0;
                        polygon->vertex[0].y = y0;
                        polygon->vertex[0].z = z0;
                        polygon->vertex[1].x = x1;
                        polygon->vertex[1].y = y1;
                        polygon->vertex[1].z = z1;
                        polygon->vertex[2].x = x2;
                        polygon->vertex[2].y = y2;
                        polygon->vertex[2].z = z2;
                    } else {
                        polygon->vertex[0].x = x0;
                        polygon->vertex[0].y = y0;
                        polygon->vertex[0].z = z0;
                        polygon->vertex[1].x = x2;
                        polygon->vertex[1].y = y2;
                        polygon->vertex[1].z = z2;
                        polygon->vertex[2].x = x1;
                        polygon->vertex[2].y = y1;
                        polygon->vertex[2].z = z1;
                    }
                    span = njAbs(z1 - z0);
                    span1 = njAbs(z2 - z1);
                    span2 = njAbs(z0 - z2);
                    if (span1 > span) {
                        span = span1;
                    }
                    if (span2 > span) {
                        span = span2;
                    }
                    if (span > mesh_span) {
                        mesh_span = span;
                    }
                    x0 -= cx;
                    y0 -= cy;
                    z0 -= cz;
                    x1 -= cx;
                    y1 -= cy;
                    z1 -= cz;
                    x2 -= cx;
                    y2 -= cy;
                    z2 -= cz;
                    polygon->center.x = cx;
                    polygon->center.y = cy;
                    polygon->center.z = cz;
                    radius = vector_length_squared(x0, y0, z0);
                    radius1 = vector_length_squared(x1, y1, z1);
                    if (radius1 > radius) {
                        radius = radius1;
                    }
                    radius1 = vector_length_squared(x2, y2, z2);
                    if (radius1 > radius) {
                        radius = radius1;
                    }
                    polygon->radius = radius;
                    mats = object->model->mats;
                    if (mats != NULL) {
                        polygon->attr =
                            mats[meshset->type_matId & ~NJD_MESHSET_MASK]
                                .attrflags;
                    } else {
                        polygon->attr = 0;
                    }
                } else {
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}

/* add the triangles of the models of object's tree, children first,
 * through its transform. returns its sibling, or null when the polygons run
 * out on its own model */
static NJS_OBJECT *mesh_add_tree(NJS_OBJECT *object, Sint32 flags,
                                 struct collision_mesh *mesh)
{
    NJS_OBJECT *child;

    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    TURN(njRotateZ, object->ang[2]);
    TURN(njRotateY, object->ang[1]);
    TURN(njRotateX, object->ang[0]);
    njScaleV(NULL, (NJS_VECTOR *)object->scl);
    child = object->child;
    while (child != NULL) {
        child = mesh_add_tree(child, flags, mesh);
    }
    if (!mesh_add_model(object, flags, mesh)) {
        njPopMatrix(1);
        return NULL;
    }
    njPopMatrix(1);
    return object->sibling;
}

Float polygon_distance2(Float *from, Float *to)
{
    Float x;
    Float y;
    Float z;
    Float to_x;
    Float to_y;
    Float to_z;

    x = *from++;
    to_x = *to++;
    y = *from++;
    to_y = *to++;
    x = x - to_x;
    z = *from;
    to_z = *to;
    y = y - to_y;
    z = z - to_z;
    return (x * x) + (y * y) + (z * z);
}

int polygon_collect_near(struct polygon_ball *ball)
{
    struct polygon **near;
    struct collision_mesh *mesh;
    struct polygon *polygon;
    struct polygon *last;
    Bool found;
    struct world_object *object;
    int i;
    Float reach;
    Float reach2;
    Float low;
    Float high;
    Float z;
    Float dx;
    Float dy;
    Float dz;
    Float distance2;
    Float limit;

    near_polygon_count = 0;
    near = near_polygons;
    reach = njScalor(&ball->speed) + ball->radius;
    reach2 = reach * reach;
    world_list_near(ball->pos.x, ball->pos.y, ball->pos.z, ball->radius);
    for (mesh = collision_mesh_list; mesh != NULL; mesh = mesh->next) {
        mesh->near = NULL;
        mesh->near_count = 0;
        found = FALSE;
        for (i = world_count, object = world_objects; i > 0; i--, object++) {
            if (object->object == mesh->object) {
                found = TRUE;
                break;
            }
        }
        if (!found) {
            continue;
        }
        polygon = mesh->polygons;
        last = mesh->last;
        z = ball->pos.z;
        low = z - mesh->span - reach;
        high = z + mesh->span + reach;
        if (polygon->center.z < low && last->center.z > high) {
            while (polygon->center.z <= low) {
                if ((polygon = polygon->next) == NULL) {
                    break;
                }
            }
        }
        if (polygon != NULL && polygon->center.z > low) {
            for (; polygon != NULL && polygon->center.z < high;
                 polygon = polygon->next) {
                limit = (reach2 + polygon->radius) * NEAR_SCALE;
                dx = polygon->center.x;
                dy = polygon->center.y;
                dz = polygon->center.z;
                dx -= ball->pos.x;
                dy -= ball->pos.y;
                dz -= ball->pos.z;
                distance2 = (dx * dx) + (dy * dy) + (dz * dz);
                if (distance2 > limit) {
                    continue;
                }
                if (!mesh->near) {
                    mesh->near = near;
                }
                mesh->near_count++;
                *near = polygon;
                near++;
                if (++near_polygon_count == NEAR_POLYGONS) {
                    if (debug_hidden >= 0) {
                        njPrintC(NJM_LOCATION(2, 21),
                                 "WARNING: NEAR-poly is FULL");
                    }
                    goto end;
                }
            }
        } else {
            for (polygon = last; polygon->center.z > low && polygon != NULL;
                 polygon = polygon->prev) {
                limit = (reach2 + polygon->radius) * NEAR_SCALE;
                dx = polygon->center.x;
                dy = polygon->center.y;
                dz = polygon->center.z;
                dx -= ball->pos.x;
                dy -= ball->pos.y;
                dz -= ball->pos.z;
                distance2 = (dx * dx) + (dy * dy) + (dz * dz);
                if (distance2 > limit) {
                    continue;
                }
                if (!mesh->near) {
                    mesh->near = near;
                }
                mesh->near_count++;
                *near = polygon;
                near++;
                if (++near_polygon_count == NEAR_POLYGONS) {
                    if (debug_hidden >= 0) {
                        njPrintC(NJM_LOCATION(2, 21),
                                 "WARNING: NEAR-poly is FULL");
                    }
                    goto end;
                }
            }
        }
    }
end:
    return near_polygon_count;
}

Bool collision_mesh_init(void)
{
    struct collision_mesh *mesh;
    struct polygon *polygon;
    int i;

    near_polygon_count = 0;
    touch_polygon_count = 0;
    collision_mesh_list = NULL;
    mesh_stale_object = NULL;
    for (i = MESHES, mesh = mesh_pool; i > 0; i--, mesh++) {
        mesh->object = NULL;
    }
    for (i = MESH_POLYGONS, polygon = polygon_pool; i > 0; i--, polygon++) {
        polygon->radius = 0.0F;
    }
    polygon_pool_init(polygon_pool, MESH_POLYGONS);
    collision_mesh_pool_init(mesh_pool, MESHES);
    return TRUE;
}

void collision_mesh_collect_all(void)
{
    mesh_collect_all = TRUE;
}

Bool collision_mesh_update(void)
{
    NJS_OBJECT *objects[MESHES];
    Sint32 flags[MESHES];
    NJS_POINT3 center;
    NJS_OBJECT **object;
    Sint32 *flag;
    struct world_object *entry;
    struct collision_mesh *mesh;
    struct collision_mesh *next;
    struct polygon *polygon;
    NJS_OBJECT *root;
    NJS_MATRIX *matrix;
    int count;
    Bool found;
    int wanted;
    int i;
    int free;
    int most;
    int total;
    int used;

    if (!mesh_collect_all) {
        world_collect_near_players();
    } else {
        world_collect_all();
        mesh_collect_all = FALSE;
    }
    /* @hack spelling: the block as a loop of one pass, which a break leaves
     * once the list is full. SHC loads 1, MESHES and WORLD_MOVES ahead of it
     * into R11, R12 and R10 (0x8c02996c, 0x8c029976, 0x8c02997a); as an if
     * around the loop that frees meshes, the function runs 2 bytes short and
     * 260 of 440 words differ */
    do {
        wanted = 0;
        object = objects;
        flag = flags;
        if ((count = world_count) > MESHES) {
            count = MESHES;
        }
        for (entry = world_objects; count > 0; count--, entry++) {
            if (entry->flags & WORLD_MOVES) {
                *object = entry->object;
                object++;
                wanted++;
                *flag = entry->flags;
                flag++;
            } else if (mesh_stale_object == entry->object) {
                wanted++;
                mesh_stale_object = NULL;
                *object = entry->object;
                object++;
                *flag = entry->flags;
                flag++;
            } else {
                found = FALSE;
                for (mesh = collision_mesh_list; mesh != NULL;
                     mesh = mesh->next) {
                    if (mesh->object == entry->object) {
                        found = TRUE;
                        break;
                    }
                }
                if (found) {
                    continue;
                }
                wanted++;
                *object++ = entry->object;
                *flag = entry->flags;
                flag++;
                if (wanted == MESHES) {
                    break;
                }
            }
        }
        if (wanted == MESHES) {
            break;
        }
        for (mesh = collision_mesh_list; mesh != NULL; mesh = next) {
            found = FALSE;
            for (i = world_count, entry = world_objects; i > 0; i--, entry++) {
                if (mesh->object == entry->object) {
                    if ((entry->flags & WORLD_MOVES) ||
                        mesh_stale_object == entry->object) {
                        found = FALSE;
                    } else {
                        found = TRUE;
                    }
                    break;
                }
            }
            next = mesh->next;
            if (!found) {
                collision_mesh_free(mesh);
            }
        }
    } while (0);
    njPushMatrix(&_unit_matrix_);
    flag = flags;
    object = objects;
    /* @hack spelling: the loop as a do inside an if, which tests i, read back
     * from R15+4, before it loads mesh_span's address into R8 and 0.0F into
     * FR15 (CMP/PL at 0x8c029ab0, then 0x8c029ab6 and 0x8c029ab8); as a for
     * loop, 11 of 440 words differ */
    i = wanted;
    /* @hack spelling: the unit matrix's address in matrix, which keeps it at
     * R15+8 for each pass's njPushMatrix (0x8c029aac, 0x8c029aca); as
     * &_unit_matrix_ at the call, 16 of 440 words differ */
    matrix = &_unit_matrix_;
    if (i > 0) {
        do {
            /* @bug memory: the stores below write through null once the pool
             * is out of meshes (FMOV.S FR3,@(R0,R13) at 0x8c029b4e on). the
             * pool lends 191, since collision_mesh_alloc returns null rather
             * than take the free list's last record. the counts above stop at
             * MESHES, 192, one more */
            mesh = collision_mesh_alloc();
            collision_mesh_clear_buckets();
            njPushMatrix(matrix);
            root = *object;
            njTranslateV(NULL, (NJS_VECTOR *)root->pos);
            TURN(njRotateZ, root->ang[2]);
            TURN(njRotateY, root->ang[1]);
            TURN(njRotateX, root->ang[0]);
            njScaleV(NULL, (NJS_VECTOR *)root->scl);
            center.x = root->model->center.x;
            center.y = root->model->center.y;
            center.z = root->model->center.z;
            njCalcPoint(NULL, &center, &center);
            mesh_center_z = center.z;
            mesh->center.x = center.x;
            mesh->center.y = center.y;
            mesh->center.z = center.z;
            njPopMatrix(1);
            dat_8c74872c = mesh_span = 0.0F;
            mesh->object = *object;
            mesh->flags = *flag;
            mesh->polygons = NULL;
            mesh->unk_18 = 0;
            mesh->near = NULL;
            mesh->near_count = 0;
            mesh_add_tree(*object++, *flag, mesh);
            collision_mesh_take_buckets(mesh);
            mesh->span = mesh_span;
            mesh->unk_2c = dat_8c74872c;
            flag++;
        } while (--i > 0);
    }
    njPopMatrix(1);
    if (debug_hidden >= 0) {
        njPrintC(NJM_LOCATION(2, 27), "colbuf:");
        most = 0;
        total = 0;
        for (i = MESHES, mesh = mesh_pool, free = 0; i > 0; i--, mesh++) {
            if (mesh->object == NULL) {
                free++;
            } else {
                count = 0;
                for (polygon = mesh->polygons; polygon != NULL;
                     polygon = polygon->next) {
                    count++;
                }
                if (most < count) {
                    most = count;
                }
                total += count;
            }
        }
        used = 100 - (free * 100 / MESHES);
        njPrintC(NJM_LOCATION(10, 26), "obj:");
        if (used >= USED_WARNING) {
            njPrintColor(COLOR_WARNING);
            njPrintD(NJM_LOCATION(15, 26), used, 3);
            njPrintC(NJM_LOCATION(18, 26), "%");
            njPrintColor(COLOR_TEXT);
        } else {
            njPrintD(NJM_LOCATION(15, 26), used, 3);
            njPrintC(NJM_LOCATION(18, 26), "%");
        }
        njPrintD(NJM_LOCATION(20, 26), MESHES - free, 5);
        if (free == 0 && debug_hidden >= 0) {
            njPrintC(NJM_LOCATION(2, 19), "WARNING: OBJECT buf is FULL");
        }
        njPrintC(NJM_LOCATION(27, 26), "polMAX:");
        njPrintD(NJM_LOCATION(35, 26), most, 4);
        used = total * 100 / MESH_POLYGONS;
        njPrintC(NJM_LOCATION(10, 27), "r-p:");
        if (used >= USED_WARNING) {
            njPrintColor(COLOR_WARNING);
            njPrintD(NJM_LOCATION(15, 27), used, 3);
            njPrintC(NJM_LOCATION(18, 27), "%");
            njPrintColor(COLOR_TEXT);
        } else {
            njPrintD(NJM_LOCATION(15, 27), used, 3);
            njPrintC(NJM_LOCATION(18, 27), "%");
        }
        njPrintD(NJM_LOCATION(20, 27), total, 5);
        /* @bug logic: the polygons' warning tests free, the free meshes,
         * as the meshes' warning does (TST R11,R11 at 0x8c029d8e). it shows
         * when no mesh is free, not when the polygons run out */
        if (free == 0 && debug_hidden >= 0) {
            njPrintC(NJM_LOCATION(2, 20), "WARNING: RANGE-poly buf is FULL");
        }
    }
    return TRUE;
}

Bool polygon_find_sides(struct sides_query *query, NJS_OBJECT *object,
                        Uint32 flags)
{
    NJS_POINT3 *p0;
    NJS_POINT3 *p1;
    NJS_POINT3 *p2;
    struct surface *side;
    struct collision_mesh *mesh;
    struct polygon *polygon;
    Float e1;
    Float e2;
    Float e3;
    Float plane;
    Float x;
    Float y;
    Float z;
    Sint32 mesh_flags;
    Bool found;
    Bool any;
    int i;

    any = FALSE;
    for (i = SIDES, side = query->sides; i > 0; i--, side++) {
        side->found = FALSE;
    }
    query->sides[SIDE_MINUS_X].level = query->pos.x + REACH;
    query->sides[SIDE_PLUS_X].level = query->pos.x - REACH;
    query->sides[SIDE_BELOW].level = query->pos.y + REACH;
    query->sides[SIDE_ABOVE].level = query->pos.y - REACH;
    query->sides[SIDE_MINUS_Z].level = query->pos.z + REACH;
    query->sides[SIDE_PLUS_Z].level = query->pos.z - REACH;
    found = FALSE;
    for (mesh = collision_mesh_list; mesh != NULL; mesh = mesh->next) {
        if (mesh->object == object) {
            found = TRUE;
            break;
        }
    }
    if (!found) {
        return FALSE;
    }
    /* WEIGH(mesh) here gives the test along z p0->x in FR8, p0->y in FR7
     * and p2->y in FR6 (0x8c029f44 to 0x8c029f76). without it, 105 of 773
     * words differ */
    WEIGH(mesh);
    polygon = mesh->polygons;
    mesh_flags = mesh->flags;
    for (; polygon != NULL; polygon = polygon->next) {
        if (((polygon->vertex[0].x - query->pos.x) *
             (polygon->vertex[0].x - query->pos.x)) +
                ((polygon->vertex[0].y - query->pos.y) *
                 (polygon->vertex[0].y - query->pos.y)) +
                ((polygon->vertex[0].z - query->pos.z) *
                 (polygon->vertex[0].z - query->pos.z)) >
            REACH * REACH) {
            continue;
        }
        p0 = &polygon->vertex[0];
        p1 = &polygon->vertex[1];
        p2 = &polygon->vertex[2];

        /* the three blocks below look along z, x and y. WEIGH(&njPrintC) in
         * each keeps 0.0F in FR15 and 1.0F in FR13 (0x8c029ed0, 0x8c029ec0).
         * the three also put the reach test's differences in x, y and z in
         * FR6, FR5 and FR4 (0x8c029f0a to 0x8c029f16). without them, 547 of
         * 773 words differ */
        do {
            NJS_VECTOR normal;

            WEIGH(&njPrintC);
            e1 = EDGE_SIDE(p0, p1, &query->pos, x, y);
            e2 = EDGE_SIDE(p1, p2, &query->pos, x, y);
            e3 = EDGE_SIDE(p2, p0, &query->pos, x, y);
            if (e1 * e2 < 0.0F || e2 * e3 < 0.0F || e1 * e3 < 0.0F) {
                break;
            }
            /* WEIGH(any) here has the test along z keep query->pos.y at
             * R15+28 (0x8c029f48). without it, p0->x takes that frame word
             * instead, and 194 of 773 words differ */
            WEIGH(any);
            triangle_normal((const char *)p0, (const char *)p1,
                            (const char *)p2, &normal);
            plane =
                -(normal.x * p0->x) - (normal.y * p0->y) - (normal.z * p0->z);
            if (normal.z * normal.z < 0.01F * 0.01F) {
                break;
            }
            z = (-(normal.x * query->pos.x) - (normal.y * query->pos.y) -
                 plane) /
                normal.z;
            side = NULL;
            if (z < query->pos.z) {
                if (!query->sides[SIDE_MINUS_Z].found ||
                    z > query->sides[SIDE_MINUS_Z].level) {
                    side = &query->sides[SIDE_MINUS_Z];
                }
            } else {
                if (!query->sides[SIDE_PLUS_Z].found ||
                    z < query->sides[SIDE_PLUS_Z].level) {
                    side = &query->sides[SIDE_PLUS_Z];
                }
            }
            if (side == NULL) {
                break;
            }
            UNIT_NORMAL(&normal);
            any = TRUE;
            side->found = TRUE;
            side->flags = mesh_flags;
            side->level = z;
            side->normal = normal;
            side->ang_z =
                -(Angle)(atan2f(normal.x, normal.y) * ANGLE_PER_RADIAN);
            side->ang_x = (Angle)(asinf(normal.z) * ANGLE_PER_RADIAN);
        } while (0);

        do {
            NJS_VECTOR normal;

            WEIGH(&njPrintC);
            e1 = EDGE_SIDE(p0, p1, &query->pos, y, z);
            e2 = EDGE_SIDE(p1, p2, &query->pos, y, z);
            e3 = EDGE_SIDE(p2, p0, &query->pos, y, z);
            if (e1 * e2 < 0.0F || e2 * e3 < 0.0F || e1 * e3 < 0.0F) {
                break;
            }
            triangle_normal((const char *)p0, (const char *)p1,
                            (const char *)p2, &normal);
            plane =
                -(normal.x * p0->x) - (normal.y * p0->y) - (normal.z * p0->z);
            if (normal.x * normal.x < 0.01F * 0.01F) {
                break;
            }
            x = (-(normal.y * query->pos.y) - (normal.z * query->pos.z) -
                 plane) /
                normal.x;
            side = NULL;
            if (x < query->pos.x) {
                if (!query->sides[SIDE_MINUS_X].found ||
                    x > query->sides[SIDE_MINUS_X].level) {
                    side = &query->sides[SIDE_MINUS_X];
                }
            } else {
                if (!query->sides[SIDE_PLUS_X].found ||
                    x < query->sides[SIDE_PLUS_X].level) {
                    side = &query->sides[SIDE_PLUS_X];
                }
            }
            if (side == NULL) {
                break;
            }
            UNIT_NORMAL(&normal);
            any = TRUE;
            side->found = TRUE;
            side->flags = mesh_flags;
            side->level = x;
            side->normal = normal;
            side->ang_z =
                -(Angle)(atan2f(normal.x, normal.y) * ANGLE_PER_RADIAN);
            side->ang_x = (Angle)(asinf(normal.z) * ANGLE_PER_RADIAN);
        } while (0);

        do {
            NJS_VECTOR normal;

            WEIGH(&njPrintC);
            e1 = EDGE_SIDE(p0, p1, &query->pos, z, x);
            e2 = EDGE_SIDE(p1, p2, &query->pos, z, x);
            e3 = EDGE_SIDE(p2, p0, &query->pos, z, x);
            if (e1 * e2 < 0.0F || e2 * e3 < 0.0F || e1 * e3 < 0.0F) {
                break;
            }
            triangle_normal((const char *)p0, (const char *)p1,
                            (const char *)p2, &normal);
            plane =
                -(normal.x * p0->x) - (normal.y * p0->y) - (normal.z * p0->z);
            if (normal.y * normal.y < 0.01F * 0.01F) {
                break;
            }
            y = (-(normal.z * query->pos.z) - (normal.x * query->pos.x) -
                 plane) /
                normal.y;
            side = NULL;
            if (y < query->pos.y) {
                if (!query->sides[SIDE_BELOW].found ||
                    y > query->sides[SIDE_BELOW].level) {
                    side = &query->sides[SIDE_BELOW];
                }
            } else {
                if (!query->sides[SIDE_ABOVE].found ||
                    y < query->sides[SIDE_ABOVE].level) {
                    side = &query->sides[SIDE_ABOVE];
                }
            }
            if (side == NULL) {
                break;
            }
            UNIT_NORMAL(&normal);
            any = TRUE;
            side->found = TRUE;
            side->flags = mesh_flags;
            side->level = y;
            side->normal = normal;
            side->ang_z =
                -(Angle)(atan2f(normal.x, normal.y) * ANGLE_PER_RADIAN);
            side->ang_x = (Angle)(asinf(normal.z) * ANGLE_PER_RADIAN);
        } while (0);
    }
    return any;
}

/* polygon_sweep's WEIGH uses give &tri[1] and &tri[2] R9 and R8 (0x8c02a5dc
 * to 0x8c02a5e4), and x0 and y0 FR4 and FR5 (0x8c02a630, 0x8c02a636).
 * without them all, each pair trades and 69 of 1121 words differ. without
 * any one of them alone, 11 to 197 words differ */
Bool polygon_sweep(struct polygon_ball *ball, NJS_OBJECT *object, Uint32 flags)
{
    struct collision_mesh *mesh;
    struct polygon **near;
    struct touch_polygon *touch;
    struct polygon_contact *hit_contact;
    struct touch_polygon *touched;
    struct polygon_contact *listed;
    Bool found;
    int count;
    Bool hit;
    Bool more;
    Uint32 two_sided;
    int nearest;
    int i;
    Float length;
    Float spacing;
    Float travelled;
    Float radius;
    Float height;
    Float scale;
    Float e1;
    Float e2;
    Float e3;
    NJS_POINT3 tri[3];
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    NJS_VECTOR normal;
    NJS_VECTOR stride;
    NJS_POINT3 at;
    struct polygon_contact contact;
    NJS_VECTOR part;

    if (ball->radius <= 0.0F) {
        return FALSE;
    }
    touch_polygon_count = 0;
    length = njScalor(&ball->speed);
    spacing = ball->radius * 0.5F;
    touch = touch_polygons;
    found = FALSE;
    for (mesh = collision_mesh_list; mesh != NULL; mesh = mesh->next) {
        if (mesh->object == object) {
            found = TRUE;
            break;
        }
        WEIGH(mesh);
    }
    WEIGH(found);
    if (!found) {
        return FALSE;
    }
    for (count = mesh->near_count, near = mesh->near; count > 0;
         count--, near++) {
        Float *v0;
        Float *v1;
        Float *v2;
        Float *edge;
        Float x0;
        Float y0;
        Float z0;
        Float x;
        Float y;
        Float z;
        NJS_POINT3 *vertex;

        tri[0].x = tri[0].y = tri[0].z = 0.0F;
        vertex = &(*near)->vertex[0];
        /* @hack spelling: Float cursors stepped across the three vertices'
         * members; as vertex[0] to vertex[2]'s members, with v0 kept for its
         * WEIGH, 142 of 1121 words differ */
        v0 = &vertex->x;
        v1 = &vertex[1].x;
        v2 = &vertex[2].x;
        WEIGH(v0);
        x0 = *v0++;
        x = *v1++ - x0;
        y0 = *v0++;
        y = *v1++ - y0;
        z0 = *v0;
        z = *v1 - z0;
        /* @hack spelling: tri[1] written through a Float cursor stepped down
         * from past its z; through its members, 109 of 1121 words differ */
        edge = &tri[1].x + 3;
        *--edge = z;
        *--edge = y;
        *--edge = x;
        x = *v2++ - x0;
        y = *v2++;
        z = *v2 - z0;
        y -= y0;
        /* @hack spelling: tri[2] written the same way; through its members, 109
         * of 1121 words differ */
        edge = &tri[2].x + 3;
        *--edge = z;
        WEIGH(x);
        *--edge = y;
        *--edge = x;
        WEIGH(x0);
        pos.x = ball->pos.x - x0;
        pos.y = ball->pos.y - y0;
        pos.z = ball->pos.z - z0;
        speed = ball->speed;
        njPushMatrix(&_unit_matrix_);
        /* @hack type: each point's x converted back to its NJS_POINT3, as in
         * sweep_model; as (const char *)&tri[0] and so on, 227 of 1121 words
         * differ */
        triangle_normal((const char *)(NJS_POINT3 *)&tri[0].x,
                        (const char *)(NJS_POINT3 *)&tri[1].x,
                        (const char *)(NJS_POINT3 *)&tri[2].x, &normal);
        /* @hack redundant: the unit normal in a loop of one pass, which has
         * SHC load ANGLE_PER_RADIAN into FR14, njCalcPoint's address into R11
         * and 0.0F into FR15 only once the loop over near has passed its
         * first test (0x8c02a60a to 0x8c02a614); without it, 40 of 1121
         * words differ */
        do {
            UNIT_NORMAL(&normal);
        } while (0);
        TURN_IF(njRotateX, -(Angle)(acosf(normal.y) * ANGLE_PER_RADIAN));
        TURN_IF(njRotateY,
                -(Angle)(atan2f(normal.x, normal.z) * ANGLE_PER_RADIAN));
        njCalcPoint(NULL, &tri[1], &tri[1]);
        njCalcPoint(NULL, &tri[2], &tri[2]);
        njCalcPoint(NULL, &pos, &pos);
        njCalcPoint(NULL, &speed, &speed);
        radius = ball->radius;
        if ((two_sided = (*near)->attr & NJD_FLAG_DOUBLE_SIDE) != 0) {
            if (pos.y < 0.0F) {
                radius *= -1.0F;
            }
        } else {
            /* @hack redundant: an empty else, which has the copy of speed into
             * stride take speed's address in R3 and its words in R2
             * (0x8c02a7d2, 0x8c02a7d6); without it, the two trade and 12 of
             * 1121 words differ */
        }
        hit = FALSE;
        stride = speed;
        UNIT_VECTOR(&stride);
        stride.x *= spacing;
        stride.y *= spacing;
        stride.z *= spacing;
        travelled = 0.0F;
        at = pos;
        for (;;) {
            more = TRUE;
            if (travelled >= length) {
                more = FALSE;
                /* @hack redundant: a copy of pos into at, which the sums
                 * below overwrite (0x8c02a8b0 to 0x8c02a8bc); without it, the
                 * function runs 20 bytes short and 769 of 1121 words
                 * differ */
                at = pos;
                at.x = pos.x + speed.x;
                at.y = pos.y + speed.y;
                at.z = pos.z + speed.z;
            }
            if (njAbs(at.y) <= ball->radius) {
                e1 = EDGE_SIDE(&tri[0], &tri[1], &at, z, x);
                e2 = EDGE_SIDE(&tri[1], &tri[2], &at, z, x);
                e3 = EDGE_SIDE(&tri[2], &tri[0], &at, z, x);
                if (e1 * e2 >= 0.0F && e2 * e3 >= 0.0F && e1 * e3 >= 0.0F) {
                    hit = TRUE;
                    break;
                }
            }
            if (!more) {
                break;
            }
            at.x += stride.x;
            at.y += stride.y;
            at.z += stride.z;
            travelled += spacing;
            WEIGH(spacing);
            WEIGH(spacing);
        }
        if (hit) {
            if (debug_hidden >= 0) {
                touch_points[0] = (*near)->vertex[0];
                touch_points[1] = (*near)->vertex[1];
                touch_points[2] = (*near)->vertex[2];
                touch_points[0].x += normal.x;
                touch_points[1].x += normal.x;
                touch_points[2].x += normal.x;
                touch_points[0].y += normal.y;
                touch_points[1].y += normal.y;
                touch_points[2].y += normal.y;
                touch_points[0].z += normal.z;
                touch_points[1].z += normal.z;
                touch_points[2].z += normal.z;
                njGetMatrix(&touch_matrix);
                njPopMatrix(1);
                njDrawTriangle3D(&touch_triangle, 3, NJD_FILL);
                njPushMatrix(&touch_matrix);
            }
            contact.height = pos.y;
            contact.height_after = pos.y + speed.y;
            contact.hit.front = normal;
            if (two_sided) {
                if (pos.y >= 0.0F) {
                    contact.hit.normal = normal;
                } else {
                    contact.hit.normal.x = -normal.x;
                    contact.hit.normal.y = -normal.y;
                    contact.hit.normal.z = -normal.z;
                }
            } else {
                contact.hit.normal = normal;
            }
            contact.hit.ang_z =
                -(Angle)(atan2f(contact.hit.normal.x, contact.hit.normal.y) *
                         ANGLE_PER_RADIAN);
            contact.hit.ang_x =
                (Angle)(asinf(contact.hit.normal.z) * ANGLE_PER_RADIAN);
            njInvertMatrix(NULL);
            stride.x = pos.x;
            stride.y = 0.0F;
            stride.z = pos.z;
            njCalcPoint(NULL, &stride, &at);
            contact.hit.point.x = at.x + (*near)->vertex[0].x;
            contact.hit.point.y = at.y + (*near)->vertex[0].y;
            contact.hit.point.z = at.z + (*near)->vertex[0].z;
            stride.y = radius;
            njCalcPoint(NULL, &stride, &at);
            contact.hit.pos.x = at.x + (*near)->vertex[0].x;
            contact.hit.pos.y = at.y + (*near)->vertex[0].y;
            contact.hit.pos.z = at.z + (*near)->vertex[0].z;
            part = speed;
            /* @hack redundant: the negation of part.y, which the store of
             * 0.0F below overwrites (FMOV.S FR4,@(R0,R15) at 0x8c02ac9c and
             * 0x8c02acec, then FMOV.S FR15,@(R0,R15) at 0x8c02ad00, each R0
             * 52); without it, the function runs 26 bytes short and 636 of
             * 1121 words differ */
            if (two_sided) {
                if (pos.y * speed.y < 0.0F) {
                    part.y = -speed.y;
                }
            } else if (radius * speed.y < 0.0F) {
                part.y = -speed.y;
            }
            part.x = speed.x;
            part.y = 0.0F;
            part.z = speed.z;
            njCalcPoint(NULL, &part, &at);
            contact.hit.speed = at;
            part.x = 0.0F;
            part.y = speed.y;
            part.z = 0.0F;
            njCalcPoint(NULL, &part, &at);
            contact.hit.normal_speed = at;
            touch->polygon = **near;
            touch->contact = contact;
            touch++;
            if (++touch_polygon_count == POLYGON_CONTACTS) {
                if (debug_hidden >= 0) {
                    njPrintC(NJM_LOCATION(2, 22),
                             "WARNING: TOUCH-poly buf is FULL");
                }
                /* @bug logic: the break leaves this triangle's matrix pushed.
                 * at the 16th touch (CMP/EQ #16,R0 at 0x8c02adb2), BF at
                 * 0x8c02adbe and BRA at 0x8c02adca leave the loop past
                 * njPopMatrix's JSR at 0x8c02ae22 */
                break;
            }
        }
        njPopMatrix(1);
        WEIGH(&njPrintC);
        WEIGH(&atan2f);
    }
    count = touch_polygon_count;
    if (count < 1) {
        return FALSE;
    }
    WEIGH(count);
    nearest = 0;
    height = touch_polygons[0].contact.height_after;
    for (i = 1, touched = &touch_polygons[1]; i < count; i++, touched++) {
        if (height > touched->contact.height_after) {
            height = touched->contact.height_after;
            nearest = i;
        }
    }
    WEIGH(i);
    for (i = count, touched = touch_polygons, listed = polygon_contacts; i > 0;
         i--, touched++, listed++) {
        *listed = touched->contact;
    }
    ball->count = touch_polygon_count;
    ball->hit_index = nearest;
    hit_contact = &touch_polygons[nearest].contact;
    ball->hit.ang_x = hit_contact->hit.ang_x;
    ball->hit.ang_z = hit_contact->hit.ang_z;
    ball->hit.front = hit_contact->hit.front;
    ball->hit.normal = hit_contact->hit.normal;
    ball->hit.speed = hit_contact->hit.speed;
    ball->hit.normal_speed = hit_contact->hit.normal_speed;
    if (touch_polygon_count == 1) {
        ball->hit.point = hit_contact->hit.point;
        ball->hit.pos = hit_contact->hit.pos;
    } else {
        touched = touch_polygons;
        ball->hit.point = touched->contact.hit.point;
        ball->hit.pos = touched->contact.hit.pos;
        touched++;
        for (i = touch_polygon_count - 1; i > 0; i--) {
            ball->hit.point.x += touched->contact.hit.point.x;
            ball->hit.point.y += touched->contact.hit.point.y;
            ball->hit.point.z += touched->contact.hit.point.z;
            ball->hit.pos.x += touched->contact.hit.pos.x;
            ball->hit.pos.y += touched->contact.hit.pos.y;
            ball->hit.pos.z += touched->contact.hit.pos.z;
            touched++;
        }
        scale = 1.0F / (Float)touch_polygon_count;
        ball->hit.point.x *= scale;
        ball->hit.point.y *= scale;
        ball->hit.point.z *= scale;
        ball->hit.pos.x *= scale;
        ball->hit.pos.y *= scale;
        ball->hit.pos.z *= scale;
    }
    return TRUE;
}

Bool polygon_cast_ray(struct ray_query *query)
{
    int count;
    Bool met;
    Float distance;
    struct world_object *object;
    struct collision_mesh *mesh;
    struct polygon *polygon;
    NJS_POINT3 *v0;
    NJS_POINT3 *v1;
    NJS_POINT3 *v2;
    Bool found;
    Float reach2;
    Float gap;
    Float e1;
    Float e2;
    Float e3;

    met = FALSE;
    query->surface.found = FALSE;
    if (query->length < 0.01F || query->length > RAY_LENGTH) {
        query->length = RAY_LENGTH;
    }
    reach2 = query->length + REACH;
    reach2 *= reach2;
    world_list_near(query->origin.x, query->origin.y, query->origin.z,
                    query->length);
    for (count = world_count, object = world_objects; count > 0;
         count--, object++) {
        found = FALSE;
        for (mesh = collision_mesh_list; mesh != NULL; mesh = mesh->next) {
            if (mesh->object == object->object) {
                found = TRUE;
                break;
            }
        }
        if (!found) {
            continue;
        }
        for (polygon = mesh->polygons; polygon != NULL;
             polygon = polygon->next) {
            v0 = &polygon->vertex[0];
            v1 = &v0[1];
            v2 = &v0[2];
            /* @hack spelling: to0 to to2 in a bare scope, and tri and the
             * rest in the next, which lets to0, to1 and to2 take the frame
             * slots of tri[2], tri[1] and tri[0] at R15+104, R15+92 and
             * R15+80 (0x8c02b1b6, 0x8c02b1de, 0x8c02b206); declared with the
             * function's other locals, the function runs 8 bytes long and
             * 496 of 748 words differ */
            {
                NJS_VECTOR to0;
                NJS_VECTOR to1;
                NJS_VECTOR to2;

                to0.x = v0->x - query->origin.x;
                to0.y = v0->y - query->origin.y;
                to0.z = v0->z - query->origin.z;
                to1.x = v1->x - query->origin.x;
                to1.y = v1->y - query->origin.y;
                to1.z = v1->z - query->origin.z;
                to2.x = v2->x - query->origin.x;
                to2.y = v2->y - query->origin.y;
                to2.z = v2->z - query->origin.z;
                if (DOT(&to0, &query->dir) < 0.0F &&
                    DOT(&to1, &query->dir) < 0.0F &&
                    DOT(&to2, &query->dir) < 0.0F) {
                    continue;
                }
            }
            {
                NJS_POINT3 tri[3];
                NJS_POINT3 *edge;
                NJS_POINT3 origin;
                NJS_VECTOR dir;
                NJS_VECTOR normal;
                NJS_POINT3 at;
                NJS_POINT3 out;

                /* @hack spelling: tri[1] written through edge, a second pointer
                 * to it, which gives &tri[1] R12 and &tri[2] R11 (0x8c02b10a
                 * to 0x8c02b116); through tri[1] itself, the two trade and 36
                 * of 748 words differ */
                edge = &tri[1];
                tri[0].x = tri[0].y = tri[0].z = 0.0F;
                *edge = *v1;
                tri[2] = *v2;
                edge->x -= v0->x;
                tri[2].x -= v0->x;
                edge->y -= v0->y;
                tri[2].y -= v0->y;
                edge->z -= v0->z;
                tri[2].z -= v0->z;
                origin.x = query->origin.x - v0->x;
                origin.y = query->origin.y - v0->y;
                origin.z = query->origin.z - v0->z;
                dir = query->dir;
                triangle_normal((const char *)&tri[0], (const char *)edge,
                                (const char *)&tri[2], &normal);
                if (!(polygon->attr & NJD_FLAG_DOUBLE_SIDE)) {
                    if (DOT(&normal, &dir) > -0.01F) {
                        continue;
                    }
                }
                UNIT_NORMAL(&normal);
                gap = (normal.x * origin.x) + (normal.y * origin.y) +
                      (normal.z * origin.z);
                gap *= gap;
                if (gap > reach2) {
                    continue;
                }
                njPushMatrix(&_unit_matrix_);
                TURN_IF(njRotateX,
                        -(Angle)(acosf(normal.y) * ANGLE_PER_RADIAN));
                TURN_IF(njRotateY, -(Angle)(atan2f(normal.x, normal.z) *
                                            ANGLE_PER_RADIAN));
                njCalcPoint(NULL, &tri[1], &at);
                /* @hack spelling: the block as a loop of one pass, which its
                 * breaks leave. SHC then keeps &tri[0], &tri[1] and &tri[2]
                 * in R9, R12 and R11 from the function's start (0x8c02b106 to
                 * 0x8c02b116); with each break a goto to the njPopMatrix, the
                 * function runs 22 bytes short and 703 of 748 words differ */
                /* @hack spelling: the loop begun at the copy into tri[1], after
                 * the njCalcPoint that makes it, which has the tests of to0
                 * to to2 take query->dir's y into FR4 and its z into FR6
                 * (0x8c02b22c, 0x8c02b240); begun at the njCalcPoint, the two
                 * trade, the function runs 2 bytes long and 196 of 748 words
                 * differ */
                do {
                    tri[1] = at;
                    njCalcPoint(NULL, &tri[2], &at);
                    tri[2] = at;
                    njCalcPoint(NULL, &origin, &at);
                    origin = at;
                    njCalcPoint(NULL, &dir, &at);
                    dir = at;
                    if (origin.y >= 0.0F) {
                        if (dir.y > -0.01F) {
                            break;
                        }
                    } else if (dir.y < 0.01F) {
                        break;
                    }
                    at.x = -(dir.x * origin.y / dir.y) + origin.x;
                    at.z = -(dir.z * origin.y / dir.y) + origin.z;
                    at.y = 0.0F;
                    e1 = EDGE_SIDE(&tri[0], &tri[1], &at, z, x);
                    e2 = EDGE_SIDE(&tri[1], &tri[2], &at, z, x);
                    e3 = EDGE_SIDE(&tri[2], &tri[0], &at, z, x);
                    if (e1 * e2 < 0.0F || e2 * e3 < 0.0F || e1 * e3 < 0.0F) {
                        break;
                    }
                    if ((distance = njDistanceP2P(&origin, &at)) >
                        query->length) {
                        break;
                    }
                    njInvertMatrix(NULL);
                    njCalcPoint(NULL, &at, &out);
                    at.x = out.x + v0->x;
                    at.y = out.y + v0->y;
                    at.z = out.z + v0->z;
                    if (!met || query->distance > distance) {
                        met = TRUE;
                        query->distance = distance;
                        query->hit = at;
                        query->surface.found = TRUE;
                        query->surface.flags = object->flags;
                        query->surface.ang_x =
                            (Angle)(asinf(normal.z) * ANGLE_PER_RADIAN);
                        query->surface.ang_z =
                            -(Angle)(atan2f(normal.x, normal.y) *
                                     ANGLE_PER_RADIAN);
                        query->surface.normal = normal;
                        reach2 = query->length + REACH;
                        reach2 *= reach2;
                    }
                } while (0);
                njPopMatrix(1);
            }
        }
    }
    /* WEIGH(polygon) and WEIGH(asinf(-0.01F)) here keep -0.01F in FR12 from
     * the function's start (0x8c02b11a). they keep its frame at 116 bytes
     * (ADD #-116,R15 at 0x8c02b0b8). without both, the frame takes 112
     * bytes, the function runs 6 bytes short and 305 of 748 words differ */
    WEIGH(polygon);
    WEIGH(asinf(-0.01F));
    return met;
}

Bool polygon_find_near_heights(struct height_query *query, Float radius)
{
    struct height_query farthest;
    struct world_object *object;
    Bool any;
    int i;

    any = FALSE;
    farthest.below.found = FALSE;
    farthest.above.found = FALSE;
    world_list_near(query->pos.x, query->pos.y, query->pos.z, radius);
    for (i = world_count, object = world_objects; i > 0; i--, object++) {
        if (find_model_heights_near(query, object->object, object->flags) ==
            TRUE) {
            if (query->below.found == TRUE) {
                if (!farthest.below.found) {
                    any = TRUE;
                    farthest.below.found = TRUE;
                    farthest.below.level = query->below.level;
                } else if (farthest.below.level > query->below.level) {
                    farthest.below.level = query->below.level;
                }
            }
            if (query->above.found == TRUE) {
                if (!farthest.above.found) {
                    any = TRUE;
                    farthest.above.found = TRUE;
                    farthest.above.level = query->above.level;
                } else if (farthest.above.level < query->above.level) {
                    farthest.above.level = query->above.level;
                }
            }
        }
    }
    /* @bug uninitialized: nothing writes farthest's flags, angles and
     * normal, nor a level whose found stays FALSE, and the query takes them
     * (MOV.L @(16,R14),R2 at 0x8c02b848 on, and MOV.L @(4,R3),R0 at
     * 0x8c02b868 on) */
    query->below = farthest.below;
    query->above = farthest.above;
    return any;
}

/* @hack type: the points as byte cursors, as in triangle_normal; as
 * NJS_POINT3 pointers read through byte cursors of their own, 10 of 33
 * words differ */
void polygon_normal(const char *v0, const char *v1, const char *v2,
                    NJS_VECTOR *normal)
{
    Float ux;
    Float uy;
    Float uz;
    Float vx;
    Float vy;
    Float vz;

    vx = *(const Float *)v0;
    v0 += sizeof(Float);
    ux = *(const Float *)v1 - vx;
    v1 += sizeof(Float);
    uy = *(const Float *)v1 - (vy = *(const Float *)v0);
    v1 += sizeof(Float);
    v0 += sizeof(Float);
    vz = *(const Float *)v0;
    uz = *(const Float *)v1;
    vx -= *(const Float *)v2;
    v2 += sizeof(Float);
    vy -= *(const Float *)v2;
    v2 += sizeof(Float);
    uz -= vz;
    vz -= *(const Float *)v2;
    normal->x = (uz * vy) - (uy * vz);
    normal->y = (ux * vz) - (uz * vx);
    normal->z = (uy * vx) - (ux * vy);
}

Bool collision_segment_nearest(NJS_POINT3 *start, NJS_POINT3 *end,
                               NJS_POINT3 *point, NJS_POINT3 *nearest,
                               Float *distance2)
{
    NJS_VECTOR along;
    NJS_VECTOR past_start;
    NJS_VECTOR short_of_end;
    NJS_VECTOR away;
    Float part;

    along.x = end->x - start->x;
    along.y = end->y - start->y;
    along.z = end->z - start->z;
    past_start.x = point->x - start->x;
    past_start.y = point->y - start->y;
    past_start.z = point->z - start->z;
    short_of_end.x = end->x - point->x;
    short_of_end.y = end->y - point->y;
    short_of_end.z = end->z - point->z;
    if ((along.x * past_start.x) + (along.y * past_start.y) +
                (along.z * past_start.z) <
            0.0F ||
        (along.x * short_of_end.x) + (along.y * short_of_end.y) +
                (along.z * short_of_end.z) <
            0.0F) {
        return FALSE;
    }
    if (along.x * along.x < 0.01F * 0.01F &&
        along.y * along.y < 0.01F * 0.01F &&
        along.z * along.z < 0.01F * 0.01F) {
        *distance2 = (past_start.x * past_start.x) +
                     (past_start.y * past_start.y) +
                     (past_start.z * past_start.z);
        return TRUE;
    }
    part =
        ((along.x * (point->x - start->x)) + (along.y * (point->y - start->y)) +
         (along.z * (point->z - start->z))) /
        ((along.x * along.x) + (along.y * along.y) + (along.z * along.z));
    nearest->x = start->x + (part * along.x);
    nearest->y = start->y + (part * along.y);
    nearest->z = start->z + (part * along.z);
    away.x = nearest->x - point->x;
    away.y = nearest->y - point->y;
    away.z = nearest->z - point->z;
    *distance2 = (away.x * away.x) + (away.y * away.y) + (away.z * away.z);
    return TRUE;
}

Float polygon_point_distance2_across(NJS_POINT3 *from, NJS_POINT3 *to)
{
    Float dx = to->x - from->x;
    Float dz = to->z - from->z;

    return (dx * dx) + (dz * dz);
}

Float polygon_point_distance2(NJS_POINT3 *from, NJS_POINT3 *to)
{
    Float dx = to->x - from->x;
    Float dy = to->y - from->y;
    Float dz = to->z - from->z;

    return (dx * dx) + (dy * dy) + (dz * dz);
}

Float collision_segment_distance(NJS_POINT3 *point, NJS_POINT3 *start,
                                 NJS_POINT3 *end, NJS_POINT3 *nearest)
{
    NJS_LINE line;
    Float x;
    Float y;
    Float z;

    if (vector_dot_from(start, end, point) < 0.0F) {
        if (nearest != NULL) {
            *nearest = *start;
        }
        return njDistanceP2P(point, start);
    }
    if (vector_dot_from(end, start, point) < 0.0F) {
        if (nearest != NULL) {
            *nearest = *end;
        }
        return njDistanceP2P(point, end);
    }
    x = start->x;
    y = start->y;
    z = start->z;
    line.px = x;
    line.py = y;
    line.pz = z;
    line.vx = end->x - x;
    line.vy = end->y - y;
    line.vz = end->z - z;
    return njDistanceP2L(point, &line, nearest);
}

NJS_OBJECT *polygon_cast_segment(struct segment_query *query, NJS_OBJECT *first)
{
    NJS_VECTOR away;
    struct world_object *objects;
    NJS_POINT3 *pos;
    NJS_VECTOR *dir;
    NJS_OBJECT *skip;
    NJS_OBJECT *object;
    int i;
    Float reach;
    Float scale;
    Float distance;

    pos = &query->pos;
    dir = &query->dir;
    if ((skip = first) != NULL) {
        /* @hack spelling: first reused to hold each hit, here and in the loop
         * below, which keeps it in the frame at @R15 (MOV.L R5,@R15 at
         * 0x8c02bbe0). the image reads it back from there for the null test
         * above and for this call's argument (MOV.L @R15,R2 at 0x8c02bbe6,
         * MOV.L @R15,R5 at 0x8c02bbee); with a local of its own for the
         * hits, the function runs 6 bytes short and 142 of 155 words differ */
        if ((first = segment_cross_mesh(query, first, query->flags)) != NULL) {
            return first;
        }
    }
    world_list_near(pos->x, pos->y, pos->z, SEGMENT_REACH);
    if (world_count == 0) {
        return NULL;
    }
    objects = world_objects;
    for (i = 0; i < world_count; i++) {
        if ((object = objects[i].object) == NULL) {
            continue;
        }
        if (object == skip) {
            continue;
        }
        if (objects[i].owner != NULL) {
            reach = njSqrt((dir->x * dir->x) + (dir->z * dir->z));
            scale = object->scl[0];
            if (scale < object->scl[1]) {
                scale = object->scl[1];
            }
            if (scale < object->scl[2]) {
                scale = object->scl[2];
            }
            reach += object->model->r * scale;
            away.x = object->pos[0] - pos->x;
            away.z = object->pos[2] - pos->z;
            distance = njSqrt((away.x * away.x) + (away.z * away.z));
            if (distance > reach + 1.0F) {
                continue;
            }
        }
        if ((first = segment_cross_mesh(query, object, objects[i].flags)) !=
            NULL) {
            return first;
        }
    }
    return NULL;
}

/* object when the query's segment crosses its mesh from the front, and
 * null otherwise. the query takes the nearest crossing, with flags as its
 * world flags */
static NJS_OBJECT *segment_cross_mesh(struct segment_query *query,
                                      NJS_OBJECT *object, Sint32 flags)
{
    NJS_VECTOR *dir;
    struct collision_mesh *mesh;
    struct polygon *polygon;
    NJS_OBJECT *crossed;
    NJS_POINT3 hit;
    NJS_VECTOR temp;
    NJS_VECTOR normal;
    NJS_VECTOR first;
    NJS_VECTOR second;
    NJS_VECTOR third;
    NJS_POINT3 *pos;
    NJS_POINT3 *v0;
    NJS_POINT3 *v1;
    NJS_POINT3 *v2;
    Float part;
    Float height;

    pos = &query->pos;
    dir = &query->dir;
    crossed = NULL;
    for (mesh = collision_mesh_list; mesh != NULL; mesh = mesh->next) {
        if (mesh->object == object) {
            for (polygon = mesh->polygons; polygon != NULL;
                 polygon = polygon->next) {
                v0 = &polygon->vertex[0];
                v1 = &polygon->vertex[1];
                v2 = &polygon->vertex[2];
                first.x = v1->x - v0->x;
                first.y = v1->y - v0->y;
                first.z = v1->z - v0->z;
                second.x = v2->x - v0->x;
                second.y = v2->y - v0->y;
                second.z = v2->z - v0->z;
                normal.x = (first.y * second.z) - (first.z * second.y);
                normal.y = (first.z * second.x) - (first.x * second.z);
                normal.z = (first.x * second.y) - (first.y * second.x);
                if ((part = njInnerProduct(dir, &normal)) >= -0.000001F) {
                    continue;
                }
                temp.x = pos->x - v0->x;
                temp.y = pos->y - v0->y;
                temp.z = pos->z - v0->z;
                if ((height = njInnerProduct(&temp, &normal)) <= 0.000001F) {
                    continue;
                }
                part = -(height / part);
                if (part > 1.0F || part < 0.0F) {
                    continue;
                }
                hit.x = pos->x + (part * dir->x);
                hit.y = pos->y + (part * dir->y);
                hit.z = pos->z + (part * dir->z);
                first.x = v0->x - hit.x;
                first.y = v0->y - hit.y;
                first.z = v0->z - hit.z;
                second.x = v1->x - hit.x;
                second.y = v1->y - hit.y;
                second.z = v1->z - hit.z;
                vector_cross(&first, &second, &temp);
                if (njInnerProduct(&temp, &normal) <= 0.000001F) {
                    continue;
                }
                third.x = v2->x - hit.x;
                third.y = v2->y - hit.y;
                third.z = v2->z - hit.z;
                vector_cross(&second, &third, &temp);
                if (njInnerProduct(&temp, &normal) <= 0.000001F) {
                    continue;
                }
                vector_cross(&third, &first, &temp);
                if (njInnerProduct(&temp, &normal) <= 0.000001F) {
                    continue;
                }
                if (crossed == NULL || query->distance > part) {
                    crossed = object;
                    query->hit.x = hit.x;
                    query->hit.y = hit.y;
                    query->hit.z = hit.z;
                    query->normal.x = normal.x;
                    query->normal.y = normal.y;
                    query->normal.z = normal.z;
                    vector_aim_y(&normal, &query->ang_x, &query->ang_z);
                    query->flags = flags;
                    query->distance = part;
                }
            }
        }
    }
    return crossed;
}

Bool polygon_find_nearest_height(struct line_query *query, NJS_OBJECT *object,
                                 Uint32 flags)
{
    NJS_POINT3 *v0;
    NJS_POINT3 *v1;
    NJS_POINT3 *v2;
    NJS_POINT3 *p0;
    NJS_POINT3 *p1;
    NJS_POINT3 *p2;
    NJS_MODEL *source;
    NJS_MODEL model;
    struct strip strip;
    NJS_VECTOR normal;
    NJS_VECTOR edge;
    const Float *pos;
    const Float *slot;
    Float reach;
    Float length2;
    Float px;
    Float py;
    Float pz;
    Float dx;
    Float dy;
    Float dz;
    Float e1;
    Float e2;
    Float e3;
    Float plane;
    Float y;
    Bool found;
    int strips;
    int i;
    Bool reversed;

    found = FALSE;
    source = object->model;
    model = *source;
    strip.model = &model;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    if (!(object->evalflags & NJD_EVAL_UNIT_POS)) {
        njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    }
    if (!(object->evalflags & NJD_EVAL_UNIT_ANG)) {
        TURN(njRotateZ, object->ang[2]);
        TURN(njRotateY, object->ang[1]);
        TURN(njRotateX, object->ang[0]);
    }
    if (!(object->evalflags & NJD_EVAL_UNIT_SCL)) {
        njScaleV(NULL, (NJS_VECTOR *)object->scl);
    }

    while (model.nbMeshset != 0) {
        switch (strip.kind = model.meshsets->type_matId & NJD_MESHSET_MASK) {
        case NJD_MESHSET_3:
            strip.count = 3;
            break;
        case NJD_MESHSET_4:
            strip.count = 4;
            break;
        case NJD_MESHSET_N:
        case NJD_MESHSET_TRIMESH:
            break;
        default:
            return FALSE;
        }
        strips = model.meshsets->nbMesh;
        strip.indices = model.meshsets->meshes;
        while (strips != 0) {
            if ((object->evalflags & EVAL_UNIT) == EVAL_UNIT) {
                reversed = strip_next_plain(&strip);
            } else if ((object->evalflags & EVAL_UNTURNED) == EVAL_UNTURNED) {
                reversed = strip_next_moved(&strip, object);
            } else {
                reversed = strip_next(&strip);
            }
            for (i = 0, v0 = &strip.points[0], v1 = &strip.points[1],
                v2 = &strip.points[2];
                 i < strip.count - 2; i++, v0 = v1, v1 = v2, v2++) {
                edge.x = v1->x - v0->x;
                edge.y = v1->y - v0->y;
                edge.z = v1->z - v0->z;
                reach = VECTOR_SQUARED(&edge);
                edge.x = v2->x - v1->x;
                edge.y = v2->y - v1->y;
                edge.z = v2->z - v1->z;
                if ((length2 = VECTOR_SQUARED(&edge)) > reach) {
                    reach = length2;
                }
                edge.x = v0->x - v2->x;
                edge.y = v0->y - v2->y;
                edge.z = v0->z - v2->z;
                if ((length2 = VECTOR_SQUARED(&edge)) > reach) {
                    reach = length2;
                }
                reach += VECTOR_SQUARED(&query->speed) + SQUARE(query->radius);
                /* @hack spelling: a Float cursor stepped across the members of
                 * the query's pos; as query->pos.x, .y and .z, the function
                 * runs 32 bytes long and 318 of 673 words differ */
                pos = &query->pos.x;
                px = *pos++;
                py = *pos++;
                pz = *pos;
                /* @bug memory: the first reach test reads three Floats
                 * of the stack, from v0's own slot on, not the corner v0
                 * points at (0x8c02c3a0, 0x8c02c3a6, 0x8c02c3ac) */
#ifdef GCC
                /* repairs the reads the tag names: GCC measures to the
                 * corner v0 points at */
                dx = px - v0->x;
                dy = py - v0->y;
                dz = pz - v0->z;
#else
                slot = (Float *)&v0;
                dx = px - *slot++;
                dy = py - *slot++;
                dz = pz - *slot;
#endif
                if ((dx * dx) + (dy * dy) + (dz * dz) > reach) {
                    continue;
                }
                /* @bug memory: the second reads them from v1's own
                 * slot on (0x8c02c3c8, 0x8c02c3ce, 0x8c02c3d4) */
#ifdef GCC
                /* repairs the reads the tag names: GCC measures to the
                 * corner v1 points at */
                dx = px - v1->x;
                dy = py - v1->y;
                dz = pz - v1->z;
#else
                slot = (Float *)&v1;
                dx = px - *slot++;
                dy = py - *slot++;
                dz = pz - *slot;
#endif
                if ((dx * dx) + (dy * dy) + (dz * dz) > reach) {
                    continue;
                }
                /* @bug memory: the third reads them from v2's own
                 * slot on (0x8c02c3f0, 0x8c02c3f4, 0x8c02c3f8) */
#ifdef GCC
                /* repairs the reads the tag names: GCC measures to the
                 * corner v2 points at */
                dx = px - v2->x;
                dy = py - v2->y;
                dz = pz - v2->z;
#else
                slot = (Float *)&v2;
                dx = px - *slot++;
                dy = py - *slot++;
                dz = pz - *slot;
#endif
                if ((dx * dx) + (dy * dy) + (dz * dz) > reach) {
                    continue;
                }
                if (!((i + reversed) & 1)) {
                    p0 = v0;
                    p1 = v1;
                    p2 = v2;
                } else {
                    p0 = v0;
                    p1 = v2;
                    p2 = v1;
                }
                e1 = EDGE_SIDE(p0, p1, &query->pos, z, x);
                e2 = EDGE_SIDE(p1, p2, &query->pos, z, x);
                e3 = EDGE_SIDE(p2, p0, &query->pos, z, x);
                if (e1 * e2 < 0.0F || e2 * e3 < 0.0F || e1 * e3 < 0.0F) {
                    continue;
                }
                triangle_normal((const char *)p0, (const char *)p1,
                                (const char *)p2, &normal);
                plane = -(normal.x * p0->x) - (normal.y * p0->y) -
                        (normal.z * p0->z);
                if (normal.y != 0.0F) {
                    y = (-(normal.x * query->pos.x) -
                         (normal.z * query->pos.z) - plane) /
                        normal.y;
                    UNIT_NORMAL(&normal);
                    /* @hack redundant: found tested again, against TRUE,
                     * which it holds once it isn't FALSE (TST R10,R10 at
                     * 0x8c02c588, then CMP/EQ #1,R0 at 0x8c02c58e); without
                     * it, the function runs 6 bytes short and 185 of 673
                     * words differ */
                    if (!found || (found == TRUE &&
                                   njAbs(y - query->pos.y) <
                                       njAbs(query->level - query->pos.y))) {
                        found = TRUE;
                        query->level = y;
                        query->normal = normal;
                        query->ang_z = -(Angle)(atan2f(normal.x, normal.y) *
                                                ANGLE_PER_RADIAN);
                        query->ang_x =
                            (Angle)(asinf(normal.z) * ANGLE_PER_RADIAN);
                    }
                }
            }
            strips--;
        }
        model.meshsets++;
        model.nbMeshset--;
    }
    njPopMatrix(1);
    return found;
}

/* as polygon_find_model_heights, for the triangles that pass a reach test
 * of SIZE_REACH2(flags). a shipped bug has the test measure to stack words,
 * not to a corner */
static Bool find_model_heights_near(struct height_query *query,
                                    NJS_OBJECT *object, Uint32 flags)
{
#ifdef GCC
    /* keeps the image's result, no floor, everywhere except within REACH
     * of the world origin over a WORLD_HUGE object. there the image's reach
     * test passes: it reads the pointers v0, v2 and v1 as Floats near 0, and
     * SIZE_REACH2's shift leaves the other sizes no reach. GCC finds no floor
     * anywhere */
    query->below.found = FALSE;
    query->above.found = FALSE;
    return FALSE;
#else
    NJS_POINT3 *v0;
    NJS_POINT3 *v1;
    NJS_POINT3 *v2;
    NJS_POINT3 *points;
    NJS_POINT3 *p0;
    NJS_POINT3 *p1;
    NJS_POINT3 *p2;
    struct surface *side;
    NJS_MODEL *source;
    NJS_MODEL model;
    struct strip strip;
    NJS_VECTOR normal;
    const Float *pos;
    const Float *slot;
    Float reach2;
    Float dx;
    Float dy;
    Float dz;
    Float e1;
    Float e2;
    Float e3;
    Float plane;
    Float y;
    Bool any;
    int strips;
    int i;
    Bool reversed;

    any = FALSE;
    query->below.found = FALSE;
    query->above.found = FALSE;
    source = object->model;
    model = *source;
    strip.model = &model;
    reach2 = SIZE_REACH2(flags);
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    if (!(object->evalflags & NJD_EVAL_UNIT_POS)) {
        njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    }
    if (!(object->evalflags & NJD_EVAL_UNIT_ANG)) {
        TURN(njRotateZ, object->ang[2]);
        TURN(njRotateY, object->ang[1]);
        TURN(njRotateX, object->ang[0]);
    }
    if (!(object->evalflags & NJD_EVAL_UNIT_SCL)) {
        njScaleV(NULL, (NJS_VECTOR *)object->scl);
    }

    while (model.nbMeshset != 0) {
        switch (strip.kind = model.meshsets->type_matId & NJD_MESHSET_MASK) {
        case NJD_MESHSET_3:
            strip.count = 3;
            break;
        case NJD_MESHSET_4:
            strip.count = 4;
            break;
        case NJD_MESHSET_N:
        case NJD_MESHSET_TRIMESH:
            break;
        default:
            return FALSE;
        }
        strips = model.meshsets->nbMesh;
        strip.indices = model.meshsets->meshes;
        while (strips != 0) {
            if ((object->evalflags & EVAL_UNIT) == EVAL_UNIT) {
                reversed = strip_next_plain(&strip);
            } else if ((object->evalflags & EVAL_UNTURNED) == EVAL_UNTURNED) {
                reversed = strip_next_moved(&strip, object);
            } else {
                reversed = strip_next(&strip);
            }
            for (i = 0, v0 = &strip.points[0], points = strip.points,
                v1 = &points[1], v2 = &points[2];
                 i < strip.count - 2; i++, v0 = v1, v1 = v2, v2++) {
                /* @hack spelling: a Float cursor stepped across the members of
                 * the query's pos; as query->pos.x, .y and .z, 210 of 479 words
                 * differ */
                pos = &query->pos.x;
                /* @bug memory: the reach test reads three Floats of
                 * the stack, from v0's own slot on, not the corner v0 points
                 * at (0x8c02c88c, 0x8c02c892, 0x8c02c898) */
                slot = (Float *)&v0;
                dx = *pos++ - *slot++;
                dy = *pos++ - *slot;
                slot++;
                dz = *pos - *slot;
                if ((dx * dx) + (dy * dy) + (dz * dz) > reach2) {
                    continue;
                }
                if (!((i + reversed) & 1)) {
                    p0 = v0;
                    p1 = v1;
                    p2 = v2;
                } else {
                    p0 = v0;
                    p1 = v2;
                    p2 = v1;
                }
                do {
                    e1 = EDGE_SIDE(p0, p1, &query->pos, z, x);
                    e2 = EDGE_SIDE(p1, p2, &query->pos, z, x);
                    e3 = EDGE_SIDE(p2, p0, &query->pos, z, x);
                    if (e1 * e2 < 0.0F || e2 * e3 < 0.0F || e1 * e3 < 0.0F) {
                        break;
                    }
                    triangle_normal((const char *)p0, (const char *)p1,
                                    (const char *)p2, &normal);
                    plane = -(normal.x * p0->x) - (normal.y * p0->y) -
                            (normal.z * p0->z);
                    if (normal.y * normal.y >= 0.01F * 0.01F) {
                        y = (-(normal.z * query->pos.z) -
                             (normal.x * query->pos.x) - plane) /
                            normal.y;
                        side = NULL;
                        if (query->pos.y > y) {
                            if (!query->below.found || y > query->below.level) {
                                side = &query->below;
                            }
                        } else {
                            if (!query->above.found || query->above.level > y) {
                                side = &query->above;
                            }
                        }
                        if (side == NULL) {
                            break;
                        }
                        UNIT_NORMAL(&normal);
                        any = TRUE;
                        side->found = TRUE;
                        side->level = y;
                        side->normal = normal;
                        side->ang_z = -(Angle)(atan2f(normal.x, normal.y) *
                                               ANGLE_PER_RADIAN);
                        side->ang_x =
                            (Angle)(asinf(normal.z) * ANGLE_PER_RADIAN);
                    }
                } while (0);
            }
            strips--;
        }
        model.meshsets++;
        model.nbMeshset--;
    }
    njPopMatrix(1);
    return any;
#endif
}

Bool polygon_find_model_sides(struct sides_query *query, NJS_OBJECT *object,
                              Uint32 flags)
{
    NJS_POINT3 *v0;
    NJS_POINT3 *v1;
    NJS_POINT3 *v2;
    NJS_POINT3 *p0;
    NJS_POINT3 *p1;
    NJS_POINT3 *p2;
    struct surface *side;
    NJS_MODEL *source;
    NJS_MODEL model;
    struct strip strip;
    const Float *pos;
    const Float *slot;
    Float reach2;
    Float dx;
    Float dy;
    Float dz;
    Float e1;
    Float e2;
    Float e3;
    Float plane;
    Float x;
    Float y;
    Float z;
    Bool any;
    int strips;
    int i;
    Bool reversed;
    int sides;

    any = FALSE;
    for (sides = SIDES, side = query->sides; sides > 0; sides--, side++) {
        side->found = FALSE;
    }
    source = object->model;
    model = *source;
    strip.model = &model;
    reach2 = SIZE_REACH2(flags);
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    if (!(object->evalflags & NJD_EVAL_UNIT_POS)) {
        njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    }
    if (!(object->evalflags & NJD_EVAL_UNIT_ANG)) {
        TURN(njRotateZ, object->ang[2]);
        TURN(njRotateY, object->ang[1]);
        TURN(njRotateX, object->ang[0]);
    }
    if (!(object->evalflags & NJD_EVAL_UNIT_SCL)) {
        njScaleV(NULL, (NJS_VECTOR *)object->scl);
    }

    while (model.nbMeshset != 0) {
        switch (strip.kind = model.meshsets->type_matId & NJD_MESHSET_MASK) {
        case NJD_MESHSET_3:
            strip.count = 3;
            break;
        case NJD_MESHSET_4:
            strip.count = 4;
            break;
        case NJD_MESHSET_N:
        case NJD_MESHSET_TRIMESH:
            break;
        default:
            return FALSE;
        }
        strips = model.meshsets->nbMesh;
        strip.indices = model.meshsets->meshes;
        while (strips != 0) {
            if ((object->evalflags & EVAL_UNIT) == EVAL_UNIT) {
                reversed = strip_next_plain(&strip);
            } else if ((object->evalflags & EVAL_UNTURNED) == EVAL_UNTURNED) {
                reversed = strip_next_moved(&strip, object);
            } else {
                reversed = strip_next(&strip);
            }
            for (i = 0, v0 = &strip.points[0], v1 = &strip.points[1],
                v2 = &strip.points[2];
                 i < strip.count - 2; i++, v0 = v1, v1 = v2, v2++) {
                /* @hack spelling: a Float cursor stepped across the members of
                 * the query's pos; as query->pos.x, .y and .z, 505 of 944 words
                 * differ */
                pos = &query->pos.x;
                /* @bug memory: the reach test reads three Floats of
                 * the stack, from v0's own slot on, not the corner v0 points
                 * at (0x8c02cd88, 0x8c02cd90, 0x8c02cd96) */
#ifdef GCC
                /* repairs the reads the tag names: GCC measures to the
                 * corner v0 points at */
                dx = *pos++ - v0->x;
                dy = *pos++ - v0->y;
                dz = *pos - v0->z;
#else
                slot = (Float *)&v0;
                dx = *pos++ - *slot;
                slot++;
                dy = *pos++ - *slot;
                slot++;
                dz = *pos - *slot;
#endif
                if (reach2 < (dx * dx) + (dy * dy) + (dz * dz)) {
                    continue;
                }
                if (!((i + reversed) & 1)) {
                    p0 = v0;
                    p1 = v1;
                    p2 = v2;
                } else {
                    p0 = v0;
                    p1 = v2;
                    p2 = v1;
                }
                /* the three blocks below look along z, x and y */
                do {
                    NJS_VECTOR normal;

                    e1 = EDGE_SIDE(p0, p1, &query->pos, x, y);
                    e2 = EDGE_SIDE(p1, p2, &query->pos, x, y);
                    e3 = EDGE_SIDE(p2, p0, &query->pos, x, y);
                    /* WEIGH(p0) here and WEIGH(any) at the end keep
                     * query->pos.y at R15+40 in the test along z
                     * (0x8c02cdd2). without WEIGH(p0), p2->y takes that word
                     * instead, and 136 of 944 words differ */
                    WEIGH(p0);
                    if (e1 * e2 < 0.0F || e2 * e3 < 0.0F || e1 * e3 < 0.0F) {
                        break;
                    }
                    triangle_normal((const char *)p0, (const char *)p1,
                                    (const char *)p2, &normal);
                    plane = -(normal.x * p0->x) - (normal.y * p0->y) -
                            (normal.z * p0->z);
                    if (normal.z * normal.z < 0.01F * 0.01F) {
                        break;
                    }
                    z = (-(normal.x * query->pos.x) -
                         (normal.y * query->pos.y) - plane) /
                        normal.z;
                    side = NULL;
                    if (z < query->pos.z) {
                        if (!query->sides[SIDE_MINUS_Z].found ||
                            z > query->sides[SIDE_MINUS_Z].level) {
                            side = &query->sides[SIDE_MINUS_Z];
                        }
                    } else {
                        if (!query->sides[SIDE_PLUS_Z].found ||
                            z < query->sides[SIDE_PLUS_Z].level) {
                            side = &query->sides[SIDE_PLUS_Z];
                        }
                    }
                    if (side == NULL) {
                        break;
                    }
                    UNIT_NORMAL(&normal);
                    any = TRUE;
                    side->found = TRUE;
                    side->level = z;
                    side->normal = normal;
                    side->ang_z =
                        -(Angle)(atan2f(normal.x, normal.y) * ANGLE_PER_RADIAN);
                    side->ang_x = (Angle)(asinf(normal.z) * ANGLE_PER_RADIAN);
                } while (0);

                do {
                    NJS_VECTOR normal;

                    e1 = EDGE_SIDE(p0, p1, &query->pos, y, z);
                    e2 = EDGE_SIDE(p1, p2, &query->pos, y, z);
                    e3 = EDGE_SIDE(p2, p0, &query->pos, y, z);
                    if (e1 * e2 < 0.0F || e2 * e3 < 0.0F || e1 * e3 < 0.0F) {
                        break;
                    }
                    triangle_normal((const char *)p0, (const char *)p1,
                                    (const char *)p2, &normal);
                    plane = -(normal.x * p0->x) - (normal.y * p0->y) -
                            (normal.z * p0->z);
                    if (normal.x * normal.x < 0.01F * 0.01F) {
                        break;
                    }
                    x = (-(normal.y * query->pos.y) -
                         (normal.z * query->pos.z) - plane) /
                        normal.x;
                    side = NULL;
                    if (x < query->pos.x) {
                        if (!query->sides[SIDE_MINUS_X].found ||
                            x > query->sides[SIDE_MINUS_X].level) {
                            side = &query->sides[SIDE_MINUS_X];
                        }
                    } else {
                        if (!query->sides[SIDE_PLUS_X].found ||
                            x < query->sides[SIDE_PLUS_X].level) {
                            side = &query->sides[SIDE_PLUS_X];
                        }
                    }
                    if (side == NULL) {
                        break;
                    }
                    UNIT_NORMAL(&normal);
                    any = TRUE;
                    side->found = TRUE;
                    side->level = x;
                    side->normal = normal;
                    side->ang_z =
                        -(Angle)(atan2f(normal.x, normal.y) * ANGLE_PER_RADIAN);
                    side->ang_x = (Angle)(asinf(normal.z) * ANGLE_PER_RADIAN);
                } while (0);

                do {
                    NJS_VECTOR normal;

                    e1 = EDGE_SIDE(p0, p1, &query->pos, z, x);
                    e2 = EDGE_SIDE(p1, p2, &query->pos, z, x);
                    e3 = EDGE_SIDE(p2, p0, &query->pos, z, x);
                    if (e1 * e2 < 0.0F || e2 * e3 < 0.0F || e1 * e3 < 0.0F) {
                        break;
                    }
                    triangle_normal((const char *)p0, (const char *)p1,
                                    (const char *)p2, &normal);
                    plane = -(normal.x * p0->x) - (normal.y * p0->y) -
                            (normal.z * p0->z);
                    if (normal.y * normal.y < 0.01F * 0.01F) {
                        break;
                    }
                    /* @hack spelling: y and side set in one statement, which
                     * keeps y at R15+0 (FMOV.S FR2,@R15 at 0x8c02d0ba), and x
                     * at R15+4 in the test along x (0x8c02ceac); as two
                     * statements, y goes to R15+4 and x to R15+8, and 355 of
                     * 944 words differ */
                    y = (-(normal.z * query->pos.z) -
                         (normal.x * query->pos.x) - plane) /
                        normal.y,
                    side = NULL;
                    if (y < query->pos.y) {
                        if (!query->sides[SIDE_BELOW].found ||
                            y > query->sides[SIDE_BELOW].level) {
                            side = &query->sides[SIDE_BELOW];
                        }
                    } else {
                        if (!query->sides[SIDE_ABOVE].found ||
                            y < query->sides[SIDE_ABOVE].level) {
                            side = &query->sides[SIDE_ABOVE];
                        }
                    }
                    if (side == NULL) {
                        break;
                    }
                    UNIT_NORMAL(&normal);
                    any = TRUE;
                    side->found = TRUE;
                    side->level = y;
                    side->normal = normal;
                    side->ang_z =
                        -(Angle)(atan2f(normal.x, normal.y) * ANGLE_PER_RADIAN);
                    side->ang_x = (Angle)(asinf(normal.z) * ANGLE_PER_RADIAN);
                } while (0);
            }
            strips--;
        }
        model.meshsets++;
        model.nbMeshset--;
    }
    njPopMatrix(1);
    /* WEIGH(any) here, with WEIGH(p0) in the test along z, keeps
     * query->pos.y at R15+40 there (0x8c02cdd2). without WEIGH(any), p1->x
     * takes that word instead, and 48 of 944 words differ */
    WEIGH(any);
    return any;
}

Bool polygon_find_model_heights(struct height_query *query, NJS_OBJECT *object)
{
    NJS_POINT3 *v0;
    NJS_POINT3 *v1;
    NJS_POINT3 *v2;
    NJS_POINT3 *p0;
    NJS_POINT3 *p1;
    NJS_POINT3 *p2;
    struct surface *side;
    NJS_MODEL *source;
    NJS_MODEL model;
    struct strip strip;
    NJS_VECTOR normal;
    Float e1;
    Float e2;
    Float e3;
    Float plane;
    Float y;
    Bool any;
    int strips;
    int i;
    Bool reversed;

    any = FALSE;
    query->below.found = FALSE;
    query->above.found = FALSE;
    source = object->model;
    model = *source;
    strip.model = &model;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    if (!(object->evalflags & NJD_EVAL_UNIT_POS)) {
        njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    }
    if (!(object->evalflags & NJD_EVAL_UNIT_ANG)) {
        TURN(njRotateZ, object->ang[2]);
        TURN(njRotateY, object->ang[1]);
        TURN(njRotateX, object->ang[0]);
    }
    if (!(object->evalflags & NJD_EVAL_UNIT_SCL)) {
        njScaleV(NULL, (NJS_VECTOR *)object->scl);
    }

    while (model.nbMeshset != 0) {
        switch (strip.kind = model.meshsets->type_matId & NJD_MESHSET_MASK) {
        case NJD_MESHSET_3:
            strip.count = 3;
            break;
        case NJD_MESHSET_4:
            strip.count = 4;
            break;
        case NJD_MESHSET_N:
        case NJD_MESHSET_TRIMESH:
            break;
        default:
            return FALSE;
        }
        strips = model.meshsets->nbMesh;
        strip.indices = model.meshsets->meshes;
        while (strips != 0) {
            if ((object->evalflags & EVAL_UNIT) == EVAL_UNIT) {
                reversed = strip_next_plain(&strip);
            } else if ((object->evalflags & EVAL_UNTURNED) == EVAL_UNTURNED) {
                reversed = strip_next_moved(&strip, object);
            } else {
                reversed = strip_next(&strip);
            }
            for (i = 0, v0 = &strip.points[0], v1 = &strip.points[1],
                v2 = &strip.points[2];
                 i < strip.count - 2; i++, v0 = v1, v1 = v2, v2++) {
                if (!((i + reversed) & 1)) {
                    p0 = v0;
                    p1 = v1;
                    p2 = v2;
                } else {
                    p0 = v0;
                    p1 = v2;
                    p2 = v1;
                }
                do {
                    e1 = EDGE_SIDE(p0, p1, &query->pos, z, x);
                    e2 = EDGE_SIDE(p1, p2, &query->pos, z, x);
                    e3 = EDGE_SIDE(p2, p0, &query->pos, z, x);
                    if (e1 * e2 < 0.0F || e2 * e3 < 0.0F || e1 * e3 < 0.0F) {
                        break;
                    }
                    triangle_normal((const char *)p0, (const char *)p1,
                                    (const char *)p2, &normal);
                    plane = -(normal.x * p0->x) - (normal.y * p0->y) -
                            (normal.z * p0->z);
                    if (normal.y * normal.y >= 0.01F * 0.01F) {
                        y = (-(normal.z * query->pos.z) -
                             (normal.x * query->pos.x) - plane) /
                            normal.y;
                        side = NULL;
                        if (query->pos.y > y) {
                            if (!query->below.found || y > query->below.level) {
                                side = &query->below;
                            }
                        } else {
                            if (!query->above.found || query->above.level > y) {
                                side = &query->above;
                            }
                        }
                        if (side == NULL) {
                            break;
                        }
                        UNIT_NORMAL(&normal);
                        any = TRUE;
                        side->found = TRUE;
                        side->level = y;
                        side->normal = normal;
                        side->ang_z = -(Angle)(atan2f(normal.x, normal.y) *
                                               ANGLE_PER_RADIAN);
                        side->ang_x =
                            (Angle)(asinf(normal.z) * ANGLE_PER_RADIAN);
                    }
                } while (0);
            }
            strips--;
        }
        model.meshsets++;
        model.nbMeshset--;
    }
    njPopMatrix(1);
    return any;
}

void collision_mesh_forget(NJS_OBJECT *object)
{
    struct collision_mesh *mesh;

    for (mesh = collision_mesh_list; mesh != NULL; mesh = mesh->next) {
        if (mesh->object == object) {
            mesh->object = NULL;
            break;
        }
    }
}
