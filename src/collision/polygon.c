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
#ifdef DREAMCAST
#ifdef NONMATCHING
#include <kamui.h>
#include <sh4zam/shz_mem.h>
#include <shinobi.h>
#endif
#endif

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
#ifdef NONMATCHING
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
#ifdef DREAMCAST
#ifdef NONMATCHING
/* sweep_turn's work for a triangle, kept for a later sweep of it. TURN_SETS
 * sets hold TURN_WAYS ways each, and a triangle's place in polygon_pool names
 * its set. a way matches a triangle by the EDGE_WORDS words of its tri[1] and
 * tri[2]. player_collide_world sweeps the list twice, and the second sweep
 * finds the first's work. the next step's sweeps find a triangle that stays
 * near, wherever polygon_collect_near lists it */
#define TURN_WAYS 2
#define TURN_SETS (NEAR_POLYGONS / TURN_WAYS)
#define EDGE_WORDS ((int)(2 * sizeof(NJS_POINT3) / sizeof(Uint32)))
_Static_assert(TURN_WAYS == 2,
               "sweep_turn overruns a set or leaves ways unfilled");
struct sweep_turn {
    Uint32 edges[EDGE_WORDS];
    /* an empty place's zero words would match a triangle with no edges */
    Bool kept;
    NJS_VECTOR normal;
    Angle ang_x;
    Angle ang_y;
};
static struct sweep_turn sweep_turns[TURN_SETS * TURN_WAYS];
/* collision_mesh_update's finds, by a table rather than a walk of one list
 * for each entry of the other. each update starts a new era, mesh_finds_era,
 * and a slot of an earlier era is empty. a slot of this era holds a listed
 * mesh's object, and its first entry in world_objects or null. MESH_FINDS, a
 * power of two over MESHES, keeps a probe short */
#define MESH_FINDS 256
_Static_assert(MESH_FINDS > MESHES, "a probe of a full mesh_finds never ends");
struct mesh_find {
    NJS_OBJECT *object;
    struct world_object *first;
    Uint32 era;
};
static struct mesh_find mesh_finds[MESH_FINDS];
static Uint32 mesh_finds_era;

/* object's slot in mesh_finds: its own, or the empty one it would take. the
 * probe starts at the object's address over its size. so the objects of one
 * array take neighboring slots */
static struct mesh_find *mesh_find(NJS_OBJECT *object)
{
    struct mesh_find *find;
    Uint32 i;

    for (i = (Uint32)object / sizeof(NJS_OBJECT);; i++) {
        find = &mesh_finds[i % MESH_FINDS];
        if (find->era != mesh_finds_era || find->object == object) {
            return find;
        }
    }
}

/* a new era of mesh_finds: each listed mesh's object, with its first entry
 * in world_objects */
static void mesh_finds_fill(void)
{
    struct collision_mesh *mesh;
    struct world_object *entry;
    struct mesh_find *find;
    int i;

    if (++mesh_finds_era == 0) {
        for (i = 0; i < MESH_FINDS; i++) {
            mesh_finds[i].era = 0;
        }
        mesh_finds_era = 1;
    }
    for (mesh = collision_mesh_list; mesh != NULL; mesh = mesh->next) {
        find = mesh_find(mesh->object);
        find->object = mesh->object;
        find->first = NULL;
        find->era = mesh_finds_era;
    }
    for (i = world_count, entry = world_objects; i > 0; i--, entry++) {
        find = mesh_find(entry->object);
        if (find->era == mesh_finds_era && find->first == NULL) {
            find->first = entry;
        }
    }
}

/* mesh_add_model's loads of a triangle's corners and stores of its polygon.
 * the stores write down from the record's end to center, so its words must
 * lie without a gap */
_Static_assert(__builtin_offsetof(struct polygon, radius) ==
                       __builtin_offsetof(struct polygon, center) + 12 &&
                   __builtin_offsetof(struct polygon, vertex) ==
                       __builtin_offsetof(struct polygon, radius) + 4 &&
                   __builtin_offsetof(struct polygon, attr) ==
                       __builtin_offsetof(struct polygon, vertex) + 36 &&
                   sizeof(struct polygon) ==
                       __builtin_offsetof(struct polygon, attr) + 4,
               "a polygon's words from center to attr have a gap");

/* a triangle's corners at from, into a, b and c. FMOV.S takes no
 * displacement, and GCC forms no FMOV.S @Rm+ here. so the asm loads the nine
 * words by post-increment */
ALWAYS_INLINE void mesh_load_corners(const NJS_POINT3 *from, NJS_POINT3 *a,
                                     NJS_POINT3 *b, NJS_POINT3 *c)
{
    __asm__("fmov.s @%[from]+,%[ax]\n\t"
            "fmov.s @%[from]+,%[ay]\n\t"
            "fmov.s @%[from]+,%[az]\n\t"
            "fmov.s @%[from]+,%[bx]\n\t"
            "fmov.s @%[from]+,%[by]\n\t"
            "fmov.s @%[from]+,%[bz]\n\t"
            "fmov.s @%[from]+,%[cx]\n\t"
            "fmov.s @%[from]+,%[cy]\n\t"
            "fmov.s @%[from]+,%[cz]"
            : [from] "+r"(from), [ax] "=f"(a->x), [ay] "=f"(a->y),
              [az] "=f"(a->z), [bx] "=f"(b->x), [by] "=f"(b->y),
              [bz] "=f"(b->z), [cx] "=f"(c->x), [cy] "=f"(c->y), [cz] "=f"(c->z)
            : "m"(*(const NJS_POINT3(*)[3])from));
}

/* write polygon's words from attr down to vertex[0], with a, b and c as its
 * corners. returns vertex[0]'s address. FMOV.S takes no displacement, and GCC
 * forms no FMOV.S Rm,@-Rn here. so the asm stores the words by pre-decrement
 * from the record's end */
ALWAYS_INLINE Float *mesh_store_corners(struct polygon *polygon, Uint32 attr,
                                        NJS_POINT3 a, NJS_POINT3 b,
                                        NJS_POINT3 c)
{
    Float *word = (Float *)(polygon + 1);

    __asm__("mov.l %[attr],@-%[word]\n\t"
            "fmov.s %[cz],@-%[word]\n\t"
            "fmov.s %[cy],@-%[word]\n\t"
            "fmov.s %[cx],@-%[word]\n\t"
            "fmov.s %[bz],@-%[word]\n\t"
            "fmov.s %[by],@-%[word]\n\t"
            "fmov.s %[bx],@-%[word]\n\t"
            "fmov.s %[az],@-%[word]\n\t"
            "fmov.s %[ay],@-%[word]\n\t"
            "fmov.s %[ax],@-%[word]"
            : [word] "+r"(word), "=m"(*(Uint32(*)[10])polygon->vertex)
            : [attr] "r"(attr), [ax] "f"(a.x), [ay] "f"(a.y), [az] "f"(a.z),
              [bx] "f"(b.x), [by] "f"(b.y), [bz] "f"(b.z), [cx] "f"(c.x),
              [cy] "f"(c.y), [cz] "f"(c.z));
    return word;
}

/* write radius and center's z, y and x down from word, vertex[0]'s
 * address */
ALWAYS_INLINE void mesh_store_center(Float *word, Float radius, Float x,
                                     Float y, Float z)
{
    __asm__("fmov.s %[radius],@-%[word]\n\t"
            "fmov.s %[z],@-%[word]\n\t"
            "fmov.s %[y],@-%[word]\n\t"
            "fmov.s %[x],@-%[word]"
            : [word] "+r"(word), "=m"(*(Float(*)[4])(word - 4))
            : [radius] "f"(radius), [x] "f"(x), [y] "f"(y), [z] "f"(z));
}

/* polygon.h and mesh.h point the other sources' calls of polygon_find_sides
 * and polygon_find_heights at the indexed searches. here the names call the
 * shipped searches, which the indexed ones call where the boxes can't serve */
#undef polygon_find_sides
#undef polygon_find_heights
/* what collision_mesh_init, collision_mesh_link_pools and
 * collision_mesh_update call in place of the shipped functions. each runs its
 * shipped function and the index's own work. they stay out of line, so each
 * call keeps the shipped call's size */
void collision_mesh_pool_init_indexed(struct collision_mesh *meshes, int count);
void collision_mesh_take_buckets_indexed(struct collision_mesh *mesh);
void collision_mesh_free_indexed(struct collision_mesh *mesh);
#endif
#endif
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
#ifdef NONMATCHING
/* repairs the shift the tag names: this side reads size_reach2 at the size's
 * own entry */
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
#ifdef NONMATCHING
            /* repairs the writes the tag in this loop names. this side's loop
             * tests the count, as the strips' and meshsets' loops do */
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
#ifdef NONMATCHING
    /* the meshes freed here keep their objects. so a slot of the index's
     * table of meshes found could hand a search a freed mesh. the indexed
     * init empties that table, as at collision_mesh_init */
    collision_mesh_pool_init_indexed(mesh_pool, MESHES);
#else
    collision_mesh_pool_init(mesh_pool, MESHES);
#endif
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
#ifdef NONMATCHING
    int triangles;
    Uint32 attr;
    NJS_POINT3 a;
    NJS_POINT3 b;
    NJS_POINT3 c;
    Float *word;
#endif

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
#ifdef NONMATCHING
            /* this loop makes each value by the shipped loop's operations, on
             * the same operands in the same order. GCC would spill that
             * loop's corners to the frame around its four calls. so here
             * only the center's z is made before polygon_alloc, and the
             * corners load after it. zam_fipr_self runs
             * vector_length_squared's FIPR FV4,FV4 inline, on the same
             * lanes. the corners store to the record before the FIPRs, so
             * none stays live past them. attr is read once a strip, as
             * nothing the loop runs writes a material */
            triangles = strip.count - 2;
            if (triangles > 0) {
                mats = object->model->mats;
                attr = mats != NULL
                           ? mats[meshset->type_matId & ~NJD_MESHSET_MASK]
                                 .attrflags
                           : 0;
            }
            for (i = 0, corner = strip.points; i < triangles; i++, corner++) {
                third = 0.3333F;
                cz = (corner[0].z + corner[1].z + corner[2].z) * third;
                if ((polygon = polygon_alloc(cz - mesh_center_z)) == NULL) {
                    return FALSE;
                }
                mesh_load_corners(corner, &a, &b, &c);
                cx = (a.x + b.x + c.x) * third;
                cy = (a.y + b.y + c.y) * third;
                if (!((reversed + i) & 1)) {
                    word = mesh_store_corners(polygon, attr, a, b, c);
                } else {
                    word = mesh_store_corners(polygon, attr, a, c, b);
                }
                span = njAbs(b.z - a.z);
                span1 = njAbs(c.z - b.z);
                span2 = njAbs(a.z - c.z);
                if (span1 > span) {
                    span = span1;
                }
                if (span2 > span) {
                    span = span2;
                }
                if (span > mesh_span) {
                    mesh_span = span;
                }
                radius = zam_fipr_self(a.x - cx, a.y - cy, a.z - cz, 0.0F);
                radius1 = zam_fipr_self(b.x - cx, b.y - cy, b.z - cz, 0.0F);
                if (radius1 > radius) {
                    radius = radius1;
                }
                radius1 = zam_fipr_self(c.x - cx, c.y - cy, c.z - cz, 0.0F);
                if (radius1 > radius) {
                    radius = radius1;
                }
                mesh_store_center(word, radius, cx, cy, cz);
            }
#else
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
#endif
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

#ifdef NONMATCHING
/* past the end of polygon_collect_near's list */
#define NEAR_END (near_polygons + NEAR_POLYGONS)

/* polygon's center and radius, the words from center.x on. FMOV.S takes no
 * displacement, and GCC forms no FMOV.S @Rm+ here. so the asm loads the four
 * words by post-increment */
ALWAYS_INLINE void near_load(const struct polygon *polygon, Float *x, Float *y,
                             Float *z, Float *radius)
{
    const Float *word = &polygon->center.x;

    __asm__("fmov.s @%[word]+,%[x]\n\t"
            "fmov.s @%[word]+,%[y]\n\t"
            "fmov.s @%[word]+,%[z]\n\t"
            "fmov.s @%[word]+,%[radius]"
            : [word] "+r"(word), [x] "=f"(*x), [y] "=f"(*y), [z] "=f"(*z),
              [radius] "=f"(*radius)
            : "m"(*(const Float(*)[4])word));
}

/* list at near each polygon of a mesh's window whose center lies near the
 * point. the walk runs forward from polygon to the first center not under
 * high. with back set, it runs back to the first center not over low. a
 * polygon is near where its center's squared distance isn't over
 * (reach2 + radius) * NEAR_SCALE. the distance takes the shipped FMUL and two
 * FMACs. returns past the last polygon listed, which is NEAR_END where the
 * list fills. a polygon's first cache line holds next, prev, center and
 * radius. so the walk prefetches the next one's as it tests this one */
ALWAYS_INLINE struct polygon **near_walk(struct polygon *polygon, Bool back,
                                         Float low, Float high, Float x,
                                         Float y, Float z, Float reach2,
                                         struct polygon **near)
{
    struct polygon *next;
    Float center_x;
    Float center_y;
    Float center_z;
    Float radius;
    Float dx;
    Float dy;
    Float dz;

    for (; polygon != NULL; polygon = next) {
        near_load(polygon, &center_x, &center_y, &center_z, &radius);
        if (back ? !(center_z > low) : !(high > center_z)) {
            break;
        }
        next = back ? polygon->prev : polygon->next;
        SHZ_PREFETCH(next);
        dx = center_x - x;
        dy = center_y - y;
        dz = center_z - z;
        if (__builtin_fmaf(dz, dz, __builtin_fmaf(dx, dx, dy * dy)) >
            (reach2 + radius) * NEAR_SCALE) {
            continue;
        }
        *near++ = polygon;
        if (near == NEAR_END) {
            break;
        }
    }
    return near;
}

/* the shipped list, made from the same words by the same operations. the
 * window's high end is SHC's sum, span plus reach, then z. the shipped C
 * would have GCC add z and span first. the skip to the window steps on while
 * a center isn't over low, as SHC's FCMP/GT tests it. so it steps past a
 * NaN, where the shipped C's <= stops. the ball's point stays in registers.
 * the list's count, and each mesh's place and count in it, are stored after
 * the walk. no code reads them sooner */
int polygon_collect_near(struct polygon_ball *ball)
{
    struct polygon **near = near_polygons;
    struct polygon **first;
    struct collision_mesh *mesh;
    struct polygon *polygon;
    struct world_object *object;
    int i;
    Float x = ball->pos.x;
    Float y = ball->pos.y;
    Float z = ball->pos.z;
    Float reach = njScalor(&ball->speed) + ball->radius;
    Float reach2 = reach * reach;
    Float low;
    Float high;

    world_list_near(x, y, z, ball->radius);
    for (mesh = collision_mesh_list; mesh != NULL; mesh = mesh->next) {
        mesh->near = NULL;
        mesh->near_count = 0;
        for (i = world_count, object = world_objects; i > 0; i--, object++) {
            if (object->object == mesh->object) {
                break;
            }
        }
        if (i == 0) {
            continue;
        }
        polygon = mesh->polygons;
        low = z - mesh->span - reach;
        high = mesh->span + reach + z;
        if (low > polygon->center.z && mesh->last->center.z > high) {
            while (!(polygon->center.z > low)) {
                if ((polygon = polygon->next) == NULL) {
                    break;
                }
            }
        }
        first = near;
        if (polygon != NULL && polygon->center.z > low) {
            near = near_walk(polygon, FALSE, low, high, x, y, z, reach2, near);
        } else {
            near =
                near_walk(mesh->last, TRUE, low, high, x, y, z, reach2, near);
        }
        if (near != first) {
            mesh->near = first;
            mesh->near_count = near - first;
        }
        if (near == NEAR_END) {
            if (debug_hidden >= 0) {
                njPrintC(NJM_LOCATION(2, 21), "WARNING: NEAR-poly is FULL");
            }
            break;
        }
    }
    near_polygon_count = near - near_polygons;
    return near_polygon_count;
}
#else
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
#endif

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
#ifdef NONMATCHING
    collision_mesh_pool_init_indexed(mesh_pool, MESHES);
#else
    collision_mesh_pool_init(mesh_pool, MESHES);
#endif
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
#ifdef DREAMCAST
#ifdef NONMATCHING
    mesh_finds_fill();
#endif
#endif
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
#ifdef NONMATCHING
                found = mesh_find(entry->object)->era == mesh_finds_era;
#else
                found = FALSE;
                for (mesh = collision_mesh_list; mesh != NULL;
                     mesh = mesh->next) {
                    if (mesh->object == entry->object) {
                        found = TRUE;
                        break;
                    }
                }
#endif
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
#ifdef NONMATCHING
            entry = mesh_find(mesh->object)->first;
            if (entry != NULL) {
                found = !((entry->flags & WORLD_MOVES) ||
                          mesh_stale_object == entry->object);
            }
#else
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
#endif
            next = mesh->next;
            if (!found) {
#ifdef NONMATCHING
                collision_mesh_free_indexed(mesh);
#else
                collision_mesh_free(mesh);
#endif
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
#ifdef NONMATCHING
            collision_mesh_take_buckets_indexed(mesh);
#else
            collision_mesh_take_buckets(mesh);
#endif
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

#ifdef NONMATCHING
/* the index of the meshes that don't move. polygon_find_sides walks a whole
 * mesh, at a cache miss or two a polygon. it looks along z, x and y from each
 * polygon whose first corner is within REACH. polygon.h sends the other
 * sources' calls to polygon_find_sides_indexed instead. a mesh's boxes hold
 * each polygon's bounds and the looks that may take a surface from it. the
 * indexed search runs the shipped looks in the mesh's order. it reads only the
 * polygons where a look may take a surface at the point. so it returns what
 * the shipped search does.
 *
 * the boxes' words come from the heap. the heap holds Kamui's vertex buffer
 * and the land's file, and nothing else takes from it. the update's first mesh
 * after a read of land takes the heap's largest free block, up to ARENA_BYTES.
 * the boxes give it back ahead of the next read. so the land's file gets the
 * words it would without them, unless other code frees and takes meanwhile. a
 * movie does: it gives Kamui's vertex buffer back and takes it again. after a
 * death or a restart, collision_mesh_init frees the meshes and their boxes.
 * the words stay held. where the act has land, the land's task then calls
 * world_load_act. that gives the words back, though the file stays loaded.
 *
 * a search wants the boxes of a mesh that doesn't move. it takes the shipped
 * walk until they are whole. they are made only while Kamui waits on the video
 * chip, as kmFlipFrameBuffer does for the flip's vsync. Kamui runs its wait
 * callback, index_idle, over and over meanwhile. so no step pays for the
 * making, and a frame that leaves its steps no wait makes nothing. a wait ends
 * late by one call: one polygon's work, or a move of the kept boxes.
 *
 * a search reads a mesh's groups first, and a group's boxes only where it may
 * give a look. a group is a box over GROUP_BOXES boxes in the mesh's order.
 * its bounds hold theirs, and its looks are any of theirs. so a look a box
 * gives at a point, its group gives too. a mesh lists its polygons by their
 * centers' z, so a group spans little of it.
 *
 * a search finds its mesh as the shipped walk of collision_mesh_list does. it
 * looks first in a table of the meshes found by object. mesh_finds can't
 * serve, as it holds only the update's own finds. the update makes a mesh only
 * for an object without one. it frees a moving object's mesh before it makes
 * it again. a freed mesh's object is null. so a slot whose mesh still holds
 * its object names the walk's find. collision_mesh_take_buckets_indexed covers
 * the update's one exception.
 *
 * a look along z tests the point against the polygon's three edges over x and
 * y. then it tests the normal's z. it breaks where two edge values, rounded,
 * have a product under 0. it breaks too where the normal's z squared, rounded,
 * is under 0.01 squared. the exact edge values sum to s, twice the polygon's
 * area over x and y. take w as the most either axis spans, and at least 1.
 * take a point a unit or more past the corners on x or y. take the edge values
 * in the sign of s. one there lies under -|s| / 2w, and another over |s| / 2.
 * the barycentric coordinates show it: one is under -1 / 2w, and the others
 * sum to over 1. an operation's result lies within ROUNDING of its exact
 * value, relative to it. an edge value then lies within e of its exact value.
 * e is about 4 ROUNDING times the sum of its two products' sizes. the spans
 * and REACH bound those. where |s| / 2w is over e plus FLUSH_FLOOR, both
 * values keep their signs. their product then stays a normal float under 0, so
 * the look breaks. box_make's area equals the normal's component the look
 * tests, to the bit. the two work the same differences of corners, with only
 * signs turned. so where its square is under 0.01 squared, the normal's test
 * breaks the look at any point. the other looks are alike, with their axes
 * turned. so a look runs past its polygon's bounds on its two axes only where
 * the area lets it. the walk skips a polygon where none of its looks may run
 * at the point. it skips one too where the point lies more than REACH plus a
 * unit past its group's bounds */

/* the fewest polygons a mesh takes boxes for */
#define BOXES_FEWEST 16
/* the boxes a group holds, and the last group may hold fewer */
#define GROUP_BOXES 8
/* the most of the heap the boxes take, and the least they take past the
 * arena's head */
#define ARENA_BYTES 32768
#define ARENA_BYTES_FEWEST 4096
/* the slots of the table of meshes found */
#define MESHES_SEEN 64
/* mesh_boxes.mesh where its mesh is gone */
#define BOXES_FREE 0xff
/* what polygon_find_heights adds to a polygon's squared radius for its
 * center's tests, mesh.src's _mesh_margin */
#define HEIGHTS_MARGIN 900.0F
/* the most a corner of a mesh with boxes reaches, so its whole units fit a
 * Sint16. a point a search takes boxes for lies within it too */
#define BOX_FAR 32000.0F
/* the most an operation of the console's FPU parts from its exact value,
 * relative to it. the FPU rounds toward zero */
#define ROUNDING 0x1p-23F
/* two edge values past it have a product over 1e-37. that is a normal
 * float, which DN doesn't flush to 0 */
#define FLUSH_FLOOR 1e-18F

/* how far a mesh's boxes have come */
enum boxes_state {
    /* none will be made: the mesh moves or predates the arena, or no mesh
     * holds the place */
    BOXES_OFF,
    BOXES_UNWANTED, /* no search has wanted them */
    BOXES_WANTED, /* a search has, and the making hasn't reached them */
    BOXES_MAKING,
    BOXES_MADE, /* whole */
    /* none until the mesh is made again. it has too few polygons or a corner
     * past BOX_FAR, or too few words lie free */
    BOXES_REFUSED
};

/* the looks polygon_find_sides takes */
#define LOOK_Z 1 /* along z, over x and y */
#define LOOK_X 2 /* along x, over y and z */
#define LOOK_Y 4 /* along y, over z and x */

/* a polygon's box: its bounds and looks */
struct polygon_box {
    /* its corners' least x, y and z, each cut toward 0, less two units. each
     * lies a unit or more under its least */
    Sint16 low[3];
    /* from low to their most, cut toward 0, and two units past. low plus
     * span lies a unit or more over the most */
    Uint16 span[3];
    Uint16 polygon; /* its place in polygon_pool */
    /* of looks, the ones that may take a surface at a point past the
     * bounds */
    Uint8 unbounded;
    Uint8 looks; /* the looks that may take a surface at some point */
};

/* a mesh's boxes: this head, then a struct polygon_box for each polygon and
 * one for each group. a group's box leaves its polygon unread. a struct
 * group_centers for each group follows, to a multiple of a box. the head
 * takes a box's 16 bytes. the arena's words start a multiple of 16 bytes
 * into syMalloc's block, which starts on 32. so no box crosses a 32-byte
 * cache line */
struct mesh_boxes {
    Uint16 words; /* of the head, the boxes and the groups' centers */
    Uint8 mesh; /* its mesh's place in mesh_pool, or BOXES_FREE */
    Uint16 count; /* of boxes */
    Uint8 pad[10]; /* to a box's size */
};

/* the whole units that hold a group's centers' z: from low up to, not
 * including, high */
struct group_centers {
    Sint16 low;
    Sint16 high;
};

/* a mesh a search found for its object, or the update made for it */
struct mesh_seen {
    NJS_OBJECT *object; /* or null */
    struct collision_mesh *mesh;
};

/* the boxes under way, made a polygon at a time: the count, then the
 * boxes. their words are the last taken, and only a making takes words. so
 * they don't move until the making ends */
struct boxes_making {
    int place; /* the mesh's, or -1 with none BOXES_MAKING */
    struct mesh_boxes *boxes; /* null while it counts */
    struct polygon *next; /* the next polygon */
    int count; /* of the polygons so far */
    int at; /* the next box */
};

/* the heap's words for the boxes */
struct box_arena {
    struct mesh_boxes *boxes[MESHES]; /* each BOXES_MADE mesh's */
    Uint8 states[MESHES]; /* each mesh's enum boxes_state */
    struct mesh_seen seen[MESHES_SEEN];
    struct boxes_making making;
    int wanted; /* the meshes BOXES_WANTED */
    int used; /* the words the boxes take, from the start */
    int room; /* the words for boxes */
    Uint32 words[];
};
_Static_assert(sizeof(struct polygon_box) == 16 &&
                   sizeof(struct group_centers) == 4 &&
                   sizeof(struct mesh_boxes) == sizeof(struct polygon_box) &&
                   __builtin_offsetof(struct box_arena, words) % 16 == 0,
               "a box crosses a cache line");
_Static_assert(MESHES <= BOXES_FREE && MESH_POLYGONS < 0x10000 &&
                   ARENA_BYTES / 4 <= 0xffff,
               "a place or a count outgrows its field");
/* the arena, null until the update's first mesh takes it after a read of
 * land. it is ARENA_REFUSED where the heap had too few words. .bss's common
 * block puts it in the room ALIGN(32) leaves before B32, so nothing past it
 * moves. ld.py doesn't check that room */
#define ARENA_REFUSED ((struct box_arena *)1)
struct box_arena *collision_box_arena __attribute__((common));

/* TRUE where value lies within BOX_FAR of 0, and FALSE for a NaN */
ALWAYS_INLINE Bool box_inside(Float value)
{
    return value > -BOX_FAR && value < BOX_FAR;
}

/* value cut down to a whole unit */
ALWAYS_INLINE int unit_floor(Float value)
{
    int under = (int)value;

    if ((Float)under > value) {
        under--;
    }
    return under;
}

/* the arena, where the heap has lent its words, or null */
ALWAYS_INLINE struct box_arena *arena_held(void)
{
    return collision_box_arena != ARENA_REFUSED ? collision_box_arena : NULL;
}

/* words for boxes, past the last kept, or null. where too few lie past
 * them, the kept boxes move down over the freed first. a request over the
 * whole room takes null at once, as no move could make it fit */
LINKED_LAST(boxes_take)
static struct mesh_boxes *boxes_take(struct box_arena *arena, int words)
{
    struct mesh_boxes *boxes;
    Uint32 *from;
    Uint32 *to;
    int size;
    int i;

    if (words > arena->room) {
        return NULL;
    }
    if (arena->used + words > arena->room) {
        to = arena->words;
        for (from = arena->words; from < arena->words + arena->used;
             from += size) {
            size = ((struct mesh_boxes *)from)->words;
            if (((struct mesh_boxes *)from)->mesh == BOXES_FREE) {
                continue;
            }
            if (to != from) {
                for (i = 0; i < size; i++) {
                    to[i] = from[i];
                }
                boxes = (struct mesh_boxes *)to;
                arena->boxes[boxes->mesh] = boxes;
            }
            to += size;
        }
        arena->used = to - arena->words;
        if (arena->used + words > arena->room) {
            return NULL;
        }
    }
    boxes = (struct mesh_boxes *)&arena->words[arena->used];
    arena->used += words;
    return boxes;
}

/* set box's bits for the look, where the polygon spans span_u and span_v
 * on the look's axes. area is twice its area over them, worked as
 * triangle_normal works the look's component of the normal. the look runs
 * only within reach of the first corner. the look's bit goes in looks where
 * it may take a surface at some point. it goes in unbounded too where it may
 * at a point past the bounds */
ALWAYS_INLINE void box_look(struct polygon_box *box, int look, Float span_u,
                            Float span_v, Float area, Float reach)
{
    Float area_error;
    Float edge_error;
    Float width;

    if (area * area < 0.01F * 0.01F) {
        return;
    }
    box->looks |= look;
    /* the most area and an edge value part from their exact values. area sums
     * two products of span_u and span_v at most. an edge value's point lies
     * within reach plus a span of each corner, which bounds its two products.
     * 8 and 4 roundings of those sizes bound the two to first order. 9 and 5
     * hold over the higher orders and the spans' own rounding. the test is the
     * index's, with s as area less area_error and e as edge_error */
    area_error = 9.0F * ROUNDING * span_u * span_v;
    edge_error = 5.0F * ROUNDING *
                 (span_u * (reach + span_v) + span_v * (reach + span_u));
    width = span_u > span_v ? span_u : span_v;
    if (width < 1.0F) {
        width = 1.0F;
    }
    if (njAbs(area) - area_error < 2.0F * width * (edge_error + FLUSH_FLOOR)) {
        box->unbounded |= look;
    }
}

/* make box from polygon: its bounds, and the looks it may run in and past
 * them. returns FALSE where a corner isn't within BOX_FAR of 0 */
ALWAYS_INLINE Bool box_make(struct polygon_box *box, struct polygon *polygon)
{
    const Float *corner = (const Float *)polygon->vertex;
    NJS_POINT3 *a = &polygon->vertex[0];
    NJS_POINT3 *b = &polygon->vertex[1];
    NJS_POINT3 *c = &polygon->vertex[2];
    Float span[3];
    Float low;
    Float high;
    Float reach = REACH + 1.0F;
    Float heights_reach;
    int k;

    for (k = 0; k < 9; k++) {
        if (!box_inside(corner[k])) {
            return FALSE;
        }
    }
    /* a corner under the least so far can't be over the most */
    for (k = 0; k < 3; k++) {
        low = high = corner[k];
        if (corner[k + 3] < low) {
            low = corner[k + 3];
        } else if (corner[k + 3] > high) {
            high = corner[k + 3];
        }
        if (corner[k + 6] < low) {
            low = corner[k + 6];
        } else if (corner[k + 6] > high) {
            high = corner[k + 6];
        }
        box->low[k] = (int)low - 2;
        box->span[k] = (int)high + 2 - box->low[k];
        span[k] = high - low;
    }
    box->polygon = polygon - polygon_pool;
    box->unbounded = box->looks = 0;
    box_look(box, LOOK_Z, span[0], span[1],
             ((b->x - a->x) * (c->y - a->y)) - ((b->y - a->y) * (c->x - a->x)),
             reach);
    box_look(box, LOOK_X, span[1], span[2],
             ((b->y - a->y) * (c->z - a->z)) - ((b->z - a->z) * (c->y - a->y)),
             reach);
    /* polygon_find_heights runs this look too, with no limit of REACH. it
     * reads a polygon where the point's squared distance from its center is
     * at most radius plus HEIGHTS_MARGIN. radius is the corners' greatest
     * squared distance from the center. so the point lies within twice that
     * sum's root of each corner. a unit covers the rounding */
    heights_reach = 2.0F * njSqrt(polygon->radius + HEIGHTS_MARGIN) + 1.0F;
    box_look(box, LOOK_Y, span[2], span[0],
             ((b->z - a->z) * (c->x - a->x)) - ((b->x - a->x) * (c->z - a->z)),
             heights_reach > reach ? heights_reach : reach);
    return TRUE;
}

/* end the making under way with the mesh's boxes in state. their words go
 * back unless state is BOXES_MADE */
ALWAYS_INLINE void boxes_end(struct box_arena *arena, int state)
{
    struct boxes_making *making = &arena->making;

    if (state == BOXES_MADE) {
        arena->boxes[making->place] = making->boxes;
    } else if (making->boxes != NULL) {
        arena->used = (Uint32 *)making->boxes - arena->words;
    }
    arena->states[making->place] = state;
    making->place = -1;
}

/* the groups of count boxes */
ALWAYS_INLINE int groups_of(int count)
{
    return (count + GROUP_BOXES - 1) / GROUP_BOXES;
}

/* a box's words, which a mesh's head and each of its groups take too */
#define BOX_WORDS ((int)(sizeof(struct polygon_box) / sizeof(Uint32)))

/* the words of a mesh's head, count boxes and their groups, at BOX_WORDS
 * each. the groups' centers take 1 each, to a multiple of BOX_WORDS */
ALWAYS_INLINE int boxes_words(int count)
{
    int groups = groups_of(count);

    return BOX_WORDS + BOX_WORDS * (count + groups) +
           (groups + BOX_WORDS - 1) / BOX_WORDS * BOX_WORDS;
}

/* widen group's bounds and looks over box's, and centers over a center at
 * center_z. where first is TRUE, group starts as box and centers as
 * center_z's unit */
ALWAYS_INLINE void group_grow(struct polygon_box *group,
                              struct group_centers *centers,
                              const struct polygon_box *box, Float center_z,
                              Bool first)
{
    int z = unit_floor(center_z);
    int high;
    int k;

    if (first) {
        *group = *box;
        centers->low = z;
        centers->high = z + 1;
        return;
    }
    for (k = 0; k < 3; k++) {
        high = group->low[k] + group->span[k];
        if (high < box->low[k] + box->span[k]) {
            high = box->low[k] + box->span[k];
        }
        if (group->low[k] > box->low[k]) {
            group->low[k] = box->low[k];
        }
        group->span[k] = high - group->low[k];
    }
    group->unbounded |= box->unbounded;
    group->looks |= box->looks;
    if (centers->low > z) {
        centers->low = z;
    }
    if (centers->high <= z) {
        centers->high = z + 1;
    }
}

/* take the making under way one step. a step counts a polygon, takes the
 * words after the count, or makes a box. it refuses the boxes as
 * BOXES_REFUSED says */
LINKED_LAST(boxes_make) static void boxes_make(struct box_arena *arena)
{
    struct boxes_making *making = &arena->making;
    struct mesh_boxes *boxes = making->boxes;
    struct polygon *polygon = making->next;
    struct polygon_box *first;
    struct polygon_box *groups;
    int words;

    if (boxes == NULL) {
        if (polygon != NULL) {
            making->count++;
            making->next = polygon->next;
            return;
        }
        words = boxes_words(making->count);
        if (making->count < BOXES_FEWEST ||
            (boxes = boxes_take(arena, words)) == NULL) {
            boxes_end(arena, BOXES_REFUSED);
            return;
        }
        boxes->words = words;
        boxes->mesh = making->place;
        boxes->count = making->count;
        making->boxes = boxes;
        making->next = mesh_pool[making->place].polygons;
        making->at = 0;
        return;
    }
    first = (struct polygon_box *)(boxes + 1);
    /* each change to a mesh's polygons comes with the boxes_forget or
     * arena_clear that ends its making. a step runs only in a wait of Kamui's,
     * and none falls between the two. so the list holds what the count took.
     * a list of another length is refused, here and at the making's end */
    if (polygon == NULL || making->at == boxes->count ||
        !box_make(first + making->at, polygon)) {
        boxes_end(arena, BOXES_REFUSED);
        return;
    }
    groups = first + boxes->count;
    group_grow(groups + making->at / GROUP_BOXES,
               (struct group_centers *)(groups + groups_of(boxes->count)) +
                   making->at / GROUP_BOXES,
               first + making->at, polygon->center.z,
               making->at % GROUP_BOXES == 0);
    making->next = polygon->next;
    making->at++;
    if (making->next == NULL) {
        boxes_end(arena,
                  making->at == boxes->count ? BOXES_MADE : BOXES_REFUSED);
    }
}

/* Kamui's wait callback while the arena is held. it takes one step of the
 * making under way. with none, it starts one for the first mesh in mesh_pool
 * that is BOXES_WANTED. wanted counts those, so where it is over 0 the scan
 * finds one */
LINKED_LAST(index_idle) static void index_idle(void *argument)
{
    struct box_arena *arena = arena_held();
    struct boxes_making *making;
    int place;

    if (arena == NULL) {
        return;
    }
    making = &arena->making;
    if (making->place < 0) {
        if (arena->wanted == 0) {
            return;
        }
        place = 0;
        while (arena->states[place] != BOXES_WANTED) {
            place++;
        }
        arena->states[place] = BOXES_MAKING;
        arena->wanted--;
        making->place = place;
        making->boxes = NULL;
        making->next = mesh_pool[place].polygons;
        making->count = 0;
    }
    boxes_make(arena);
}

/* forget every mesh's boxes, keeping the words */
LINKED_LAST(arena_clear) static void arena_clear(struct box_arena *arena)
{
    int i;

    for (i = 0; i < MESHES; i++) {
        arena->states[i] = BOXES_OFF;
    }
    for (i = 0; i < MESHES_SEEN; i++) {
        arena->seen[i].object = NULL;
    }
    arena->making.place = -1;
    arena->wanted = 0;
    arena->used = 0;
}

/* the arena, or null where the heap has too few words. the first call after
 * a read of land takes the words, and sets index_idle as Kamui's wait
 * callback. the movie player, the only other code that sets that callback,
 * clears it as it finishes. the next read of land gives the arena back, so
 * the next call here sets index_idle again */
LINKED_LAST(arena_take) static struct box_arena *arena_take(void)
{
    struct box_arena *arena = collision_box_arena;
    Uint32 free_bytes;
    Uint32 bytes;

    if (arena != NULL) {
        return arena_held();
    }
    syMallocStat(&free_bytes, &bytes);
    if (bytes > ARENA_BYTES) {
        bytes = ARENA_BYTES;
    }
    if (bytes < sizeof(struct box_arena) + ARENA_BYTES_FEWEST ||
        (arena = syMalloc(bytes)) == NULL) {
        collision_box_arena = ARENA_REFUSED;
        return NULL;
    }
    arena_clear(arena);
    arena->room = (int)(bytes - sizeof(struct box_arena)) / 4;
    collision_box_arena = arena;
    kmSetWaitVsyncCallback(index_idle, NULL);
    return arena;
}

LINKED_LAST(collision_index_release) void collision_index_release(void)
{
    if (arena_held() != NULL) {
        syFree(collision_box_arena);
    }
    collision_box_arena = NULL;
}

/* forget the boxes of the mesh at place, as it is made again or freed. a
 * making under way gives its words back */
LINKED_LAST(boxes_forget)
static void boxes_forget(struct box_arena *arena, int place)
{
    switch (arena->states[place]) {
    case BOXES_WANTED:
        arena->wanted--;
        break;
    case BOXES_MAKING:
        boxes_end(arena, BOXES_OFF);
        break;
    case BOXES_MADE:
        arena->boxes[place]->mesh = BOXES_FREE;
        break;
    default:
        break;
    }
    arena->states[place] = BOXES_OFF;
}

/* the slot of the table of meshes found for object */
ALWAYS_INLINE struct mesh_seen *mesh_seen_slot(struct box_arena *arena,
                                               NJS_OBJECT *object)
{
    return &arena->seen[((Uint32)object / sizeof(NJS_OBJECT)) % MESHES_SEEN];
}

/* free the meshes as collision_mesh_pool_init does, and forget every
 * mesh's boxes with them */
LINKED_LAST(collision_mesh_pool_init_indexed)
__attribute__((noinline, noclone)) void
collision_mesh_pool_init_indexed(struct collision_mesh *meshes, int count)
{
    struct box_arena *arena = arena_held();

    collision_mesh_pool_init(meshes, count);
    if (arena != NULL) {
        arena_clear(arena);
    }
}

/* take the buckets as the update makes a mesh. the mesh's place forgets the
 * boxes of a mesh made there before. collision_mesh_alloc puts each mesh first
 * in collision_mesh_list, so a walk finds it for its object. the mesh takes
 * its object's slot in arena->seen, where the slot is empty or holds that
 * object. where MESHES objects want meshes, the update frees nothing. a slot
 * then still names no older mesh of its object. a mesh that doesn't move waits
 * for a search to want its boxes. a null mesh takes the shipped call alone */
LINKED_LAST(collision_mesh_take_buckets_indexed)
__attribute__((noinline, noclone)) void
collision_mesh_take_buckets_indexed(struct collision_mesh *mesh)
{
    struct box_arena *arena;
    struct mesh_seen *seen;
    int place;

    collision_mesh_take_buckets(mesh);
    if (mesh == NULL || (arena = arena_take()) == NULL) {
        return;
    }
    place = mesh - mesh_pool;
    boxes_forget(arena, place);
    seen = mesh_seen_slot(arena, mesh->object);
    if (seen->object == NULL || seen->object == mesh->object) {
        seen->object = mesh->object;
        seen->mesh = mesh;
    }
    arena->states[place] =
        mesh->flags & WORLD_MOVES ? BOXES_OFF : BOXES_UNWANTED;
}

LINKED_LAST(collision_mesh_free_indexed)
__attribute__((noinline, noclone)) void
collision_mesh_free_indexed(struct collision_mesh *mesh)
{
    struct box_arena *arena = arena_held();

    if (arena != NULL) {
        boxes_forget(arena, mesh - mesh_pool);
    }
    collision_mesh_free(mesh);
}

/* the looks that may run at a point within a polygon's bounds on both of
 * their axes. in's bits 0, 1 and 2 are set where x, y and z lie within the
 * bounds. in & (in >> 1) holds x and y in LOOK_Z's bit, and y and z in
 * LOOK_X's */
ALWAYS_INLINE int box_looks_within(int in)
{
    return ((in & (in >> 1)) & (LOOK_Z | LOOK_X)) |
           ((in & (in >> 2) & 1) ? LOOK_Y : 0);
}

/* a walk of a mesh's boxes */
struct box_walk {
    struct polygon_box *at; /* the next box to read */
    struct polygon_box *stop; /* past the last to read of at's group */
    struct polygon_box *next; /* the next group's first */
    struct polygon_box *end; /* past the last, where the groups start */
    struct polygon_box *group; /* the next group */
    int x; /* the point, cut down to whole units */
    int y;
    int z;
};

/* start a walk of the mesh's boxes at pos. a mesh that is BOXES_UNWANTED
 * turns BOXES_WANTED. returns FALSE where its boxes aren't whole, or pos
 * isn't within BOX_FAR of 0 */
LINKED_LAST(box_walk_start)
static Bool box_walk_start(struct box_walk *walk, struct box_arena *arena,
                           struct collision_mesh *mesh, NJS_POINT3 *pos)
{
    Uint8 *state = &arena->states[mesh - mesh_pool];
    struct mesh_boxes *boxes;

    if (*state == BOXES_UNWANTED) {
        *state = BOXES_WANTED;
        arena->wanted++;
    }
    if (*state != BOXES_MADE || !box_inside(pos->x) || !box_inside(pos->y) ||
        !box_inside(pos->z)) {
        return FALSE;
    }
    boxes = arena->boxes[mesh - mesh_pool];
    walk->at = walk->stop = walk->next = (struct polygon_box *)(boxes + 1);
    walk->end = walk->group = walk->next + boxes->count;
    walk->x = unit_floor(pos->x);
    walk->y = unit_floor(pos->y);
    walk->z = unit_floor(pos->z);
    return TRUE;
}

/* the looks of box, or of a group, that may take a surface at the walk's
 * point */
ALWAYS_INLINE int box_looks(const struct box_walk *walk,
                            const struct polygon_box *box)
{
    int in = ((Uint32)(walk->x - box->low[0]) <= box->span[0]) |
             (((Uint32)(walk->y - box->low[1]) <= box->span[1]) << 1) |
             (((Uint32)(walk->z - box->low[2]) <= box->span[2]) << 2);

    return (box_looks_within(in) | box->unbounded) & box->looks;
}

/* TRUE where the walk's point lies more than REACH plus a unit past group's
 * bounds on an axis. the point then lies past REACH of each of its polygons'
 * first corners. a box takes no such test, as the search tests each
 * polygon's reach itself */
ALWAYS_INLINE Bool group_past(const struct box_walk *walk,
                              const struct polygon_box *group)
{
    const Uint32 reach = (Uint32)REACH + 1;

    return (Uint32)(walk->x - group->low[0]) + reach >
               group->span[0] + 2 * reach ||
           (Uint32)(walk->y - group->low[1]) + reach >
               group->span[1] + 2 * reach ||
           (Uint32)(walk->z - group->low[2]) + reach >
               group->span[2] + 2 * reach;
}

/* the next polygon whose looks may take a surface, with those looks, or
 * null */
ALWAYS_INLINE struct polygon *box_walk_next(struct box_walk *walk, int *looks)
{
    struct polygon_box *box;

    for (;;) {
        while (walk->at == walk->stop) {
            if (walk->next == walk->end) {
                return NULL;
            }
            walk->at = walk->next;
            walk->next = walk->end - walk->at > GROUP_BOXES
                             ? walk->at + GROUP_BOXES
                             : walk->end;
            walk->stop =
                !group_past(walk, walk->group) && box_looks(walk, walk->group)
                    ? walk->next
                    : walk->at;
            walk->group++;
        }
        box = walk->at++;
        if ((*looks = box_looks(walk, box)) != 0) {
            return &polygon_pool[box->polygon];
        }
    }
}

/* a look of polygon_find_sides from the triangle whose corners start at p0. it
 * looks along axis w, over axes u and v. it works each value as the shipped
 * look does, in its order, from the same words. minus names the side under the
 * point along w, and minus + 1 the side over it */
LINKED_LAST(index_look)
static __attribute__((noinline)) void index_look(struct sides_query *query,
                                                 const Float *p0, int u, int v,
                                                 int w, int minus,
                                                 Sint32 mesh_flags, Bool *any)
{
    const Float *pos = (const Float *)&query->pos;
    const Float *p1 = p0 + 3;
    const Float *p2 = p0 + 6;
    struct surface *side;
    NJS_VECTOR normal;
    Float e1;
    Float e2;
    Float e3;
    Float plane;
    Float level;
    const Float *n = (const Float *)&normal;

    e1 = ((p1[u] - p0[u]) * (pos[v] - p0[v])) -
         ((p1[v] - p0[v]) * (pos[u] - p0[u]));
    e2 = ((p2[u] - p1[u]) * (pos[v] - p1[v])) -
         ((p2[v] - p1[v]) * (pos[u] - p1[u]));
    e3 = ((p0[u] - p2[u]) * (pos[v] - p2[v])) -
         ((p0[v] - p2[v]) * (pos[u] - p2[u]));
    if (e1 * e2 < 0.0F || e2 * e3 < 0.0F || e1 * e3 < 0.0F) {
        return;
    }
    triangle_normal((const char *)p0, (const char *)p1, (const char *)p2,
                    &normal);
    plane = -(normal.x * p0[0]) - (normal.y * p0[1]) - (normal.z * p0[2]);
    if (n[w] * n[w] < 0.01F * 0.01F) {
        return;
    }
    level = (-(n[u] * pos[u]) - (n[v] * pos[v]) - plane) / n[w];
    side = NULL;
    if (level < pos[w]) {
        if (!query->sides[minus].found || level > query->sides[minus].level) {
            side = &query->sides[minus];
        }
    } else {
        if (!query->sides[minus + 1].found ||
            level < query->sides[minus + 1].level) {
            side = &query->sides[minus + 1];
        }
    }
    if (side == NULL) {
        return;
    }
    UNIT_NORMAL(&normal);
    *any = TRUE;
    side->found = TRUE;
    side->flags = mesh_flags;
    side->level = level;
    side->normal = normal;
    side->ang_z = -(Angle)(atan2f(normal.x, normal.y) * ANGLE_PER_RADIAN);
    side->ang_x = (Angle)(asinf(normal.z) * ANGLE_PER_RADIAN);
}

/* the mesh the shipped walk of collision_mesh_list finds for object. where
 * object's slot holds a mesh that still holds object, that mesh is the find.
 * else the walk runs, and the slot keeps its find */
ALWAYS_INLINE struct collision_mesh *index_mesh(struct box_arena *arena,
                                                NJS_OBJECT *object)
{
    struct mesh_seen *seen = mesh_seen_slot(arena, object);
    struct collision_mesh *mesh;

    if (object != NULL && seen->object == object &&
        seen->mesh->object == object) {
        return seen->mesh;
    }
    for (mesh = collision_mesh_list; mesh != NULL; mesh = mesh->next) {
        if (mesh->object == object) {
            break;
        }
    }
    if (mesh != NULL && object != NULL) {
        seen->object = object;
        seen->mesh = mesh;
    }
    return mesh;
}

/* the search polygon.h gives the other sources' calls of
 * polygon_find_sides. it runs the shipped looks of only the polygons whose
 * boxes give one at the point. where it can't, it hands the query to the
 * shipped search */
LINKED_LAST(polygon_find_sides_indexed)
Bool polygon_find_sides_indexed(struct sides_query *query, NJS_OBJECT *object,
                                Uint32 flags)
{
    struct box_arena *arena = arena_held();
    struct box_walk walk;
    struct collision_mesh *mesh;
    struct polygon *polygon;
    struct surface *side;
    const Float *p0;
    Sint32 mesh_flags;
    Bool any;
    int looks;
    int i;

    if (arena == NULL) {
        return polygon_find_sides(query, object, flags);
    }
    mesh = index_mesh(arena, object);
    if (mesh == NULL || !box_walk_start(&walk, arena, mesh, &query->pos)) {
        return polygon_find_sides(query, object, flags);
    }
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
    mesh_flags = mesh->flags;
    while ((polygon = box_walk_next(&walk, &looks)) != NULL) {
        if (((polygon->vertex[0].x - query->pos.x) *
             (polygon->vertex[0].x - query->pos.x)) +
                ((polygon->vertex[0].y - query->pos.y) *
                 (polygon->vertex[0].y - query->pos.y)) +
                ((polygon->vertex[0].z - query->pos.z) *
                 (polygon->vertex[0].z - query->pos.z)) >
            REACH * REACH) {
            continue;
        }
        p0 = (const Float *)&polygon->vertex[0];
        if (looks & LOOK_Z) {
            index_look(query, p0, 0, 1, 2, SIDE_MINUS_Z, mesh_flags, &any);
        }
        if (looks & LOOK_X) {
            index_look(query, p0, 1, 2, 0, SIDE_MINUS_X, mesh_flags, &any);
        }
        if (looks & LOOK_Y) {
            index_look(query, p0, 2, 0, 1, SIDE_BELOW, mesh_flags, &any);
        }
    }
    return any;
}

/* what polygon_find_heights does at each polygon of its walk. its squares and
 * sums are separate FMULs and FADDs, as in mesh.src */
LINKED_LAST(heights_visit)
static __attribute__((noinline)) void heights_visit(struct height_query *query,
                                                    struct polygon *polygon,
                                                    Sint32 mesh_flags)
{
    Float reach = polygon->radius + HEIGHTS_MARGIN;
    Float x = polygon->center.x - query->pos.x;
    Float y;
    Float z;

    x = x * x;
    if (x > reach) {
        return;
    }
    y = polygon->center.y - query->pos.y;
    y = y * y;
    y = y + x;
    if (y > reach) {
        return;
    }
    z = polygon->center.z - query->pos.z;
    z = z * z;
    z = z + y;
    if (z > reach) {
        return;
    }
    mesh_test_heights(query, polygon, mesh_flags);
}

/* TRUE where box, or a group, may give polygon_find_heights a surface at the
 * walk's point. that search's test of a polygon works the same edges and
 * normal as the look along y. it wants the edges' products over 0 and the
 * normal's y squared over _mesh_edge_on_y2. both are stricter than the look's
 * tests. so a polygon whose box gives no look along y at the point gives no
 * height. box_make gives that look the reach of the center's tests, past which
 * they give none */
ALWAYS_INLINE Bool box_heights(const struct box_walk *walk,
                               const struct polygon_box *box)
{
    Uint32 x = (Uint32)(walk->x - box->low[0]);
    Uint32 z = (Uint32)(walk->z - box->low[2]);

    return (box->looks & LOOK_Y) && ((box->unbounded & LOOK_Y) ||
                                     (x <= box->span[0] && z <= box->span[2]));
}

/* the search mesh.h gives the other sources' calls of polygon_find_heights.
 * it takes the shipped walk of the window in its order, to the polygon where
 * it stops. the window holds the polygons whose centers lie within the
 * mesh's span of the point on z. a polygon whose box gives no height goes
 * unread where its group's centers show none stops the walk. an object with
 * no mesh goes to the shipped search. a mesh without whole boxes goes to
 * mesh_find_heights, that search's walk of one mesh */
LINKED_LAST(polygon_find_heights_indexed)
Bool polygon_find_heights_indexed(struct height_query *query,
                                  NJS_OBJECT *object, Uint32 flags)
{
    struct box_arena *arena = arena_held();
    struct box_walk walk;
    struct collision_mesh *mesh;
    struct polygon *polygon;
    struct polygon_box *boxes;
    struct group_centers *centers;
    Sint32 mesh_flags;
    Float low;
    Float high;
    Bool inside;
    int count;
    int stop;
    int at;
    int g;

    if (arena == NULL) {
        return polygon_find_heights(query, object, flags);
    }
    mesh = index_mesh(arena, object);
    if (mesh == NULL) {
        return polygon_find_heights(query, object, flags);
    }
    if (!box_walk_start(&walk, arena, mesh, &query->pos)) {
        return mesh_find_heights(query, mesh);
    }
    query->below.found = FALSE;
    query->above.found = FALSE;
    query->below.level = query->pos.y - REACH;
    query->above.level = query->pos.y + REACH;
    low = query->pos.z - mesh->span;
    high = query->pos.z + mesh->span;
    mesh_flags = mesh->flags;
    boxes = walk.next;
    count = walk.end - boxes;
    centers = (struct group_centers *)(walk.end + groups_of(count));
    at = 0;
    if (low > mesh->polygons->center.z) {
        if (!(mesh->last->center.z > high)) {
            for (at = count - 1; at >= 0;) {
                g = at / GROUP_BOXES;
                stop = g * GROUP_BOXES;
                /* TRUE where none of group g's centers lies under low, so
                 * none of its polygons stops the walk */
                inside = (Float)centers[g].low >= low;
                if (inside && !box_heights(&walk, walk.end + g)) {
                    at = stop - 1;
                    continue;
                }
                for (; at >= stop; at--) {
                    if (inside && !box_heights(&walk, &boxes[at])) {
                        continue;
                    }
                    polygon = &polygon_pool[boxes[at].polygon];
                    if (low > polygon->center.z) {
                        goto done;
                    }
                    heights_visit(query, polygon, mesh_flags);
                }
            }
            goto done;
        }
        while (low > polygon_pool[boxes[at].polygon].center.z) {
            at++;
        }
    }
    while (at < count) {
        g = at / GROUP_BOXES;
        stop = (g + 1) * GROUP_BOXES;
        if (stop > count) {
            stop = count;
        }
        /* TRUE where none of group g's centers reaches high */
        inside = (Float)centers[g].high <= high;
        if (inside && !box_heights(&walk, walk.end + g)) {
            at = stop;
            continue;
        }
        for (; at < stop; at++) {
            if (inside && !box_heights(&walk, &boxes[at])) {
                continue;
            }
            polygon = &polygon_pool[boxes[at].polygon];
            if (!(high > polygon->center.z)) {
                goto done;
            }
            heights_visit(query, polygon, mesh_flags);
        }
    }
done:
    return query->below.found || query->above.found;
}

/* polygon_sweep's turn, as the shipped code works it. *normal takes the
 * unit normal of the triangle tri[0] to tri[2], where tri[0] is 0. XMTRX
 * turns by the normal's angles from acosf and atan2f, as njRotateX and
 * njRotateY turn it. the work depends on only the EDGE_WORDS words of tri[1]
 * and tri[2]. so a way of set whose words match them as integers holds what
 * the work would give. a turn worked takes set's first way, and the turn there
 * moves to the second. acosf and atan2f set errno only where their result
 * isn't finite, and such a turn isn't kept. the shipped code turns about x
 * before it calls atan2f, which reads nothing a turn writes. the normal's
 * length is tested by its square. FIPR's sum of squares is never a denormal,
 * as the FPU flushes one to 0. FSQRT gives 0 only of 0, so the square is 0
 * where the length is */
ALWAYS_INLINE void sweep_turn(const NJS_POINT3 *tri, struct sweep_turn *set,
                              NJS_VECTOR *normal)
{
    const Uint32 *edges = (const Uint32 *)&tri[1];
    struct sweep_turn *turn;
    Float x_turn;
    Float y_turn;
    Angle ang_x;
    Angle ang_y;
    int i;

    for (turn = set; turn < set + TURN_WAYS; turn++) {
        for (i = 0; i < EDGE_WORDS && turn->edges[i] == edges[i]; i++) {
        }
        if (turn->kept && i == EDGE_WORDS) {
            *normal = turn->normal;
            ang_x = turn->ang_x;
            ang_y = turn->ang_y;
            goto kept;
        }
    }
    triangle_normal((const char *)&tri[0], (const char *)&tri[1],
                    (const char *)&tri[2], normal);
    if (njScalor2(normal) == 0.0F) {
        normal->x = 0.0F;
        normal->y = 1.0F;
        normal->z = 0.0F;
    } else {
        njUnitVector(normal);
    }
    x_turn = acosf(normal->y);
    y_turn = atan2f(normal->x, normal->z);
    ang_x = -(Angle)(x_turn * ANGLE_PER_RADIAN);
    ang_y = -(Angle)(y_turn * ANGLE_PER_RADIAN);
    /* both finite: a NaN fails each test. __builtin_isfinite would call
     * libgcc for the NaN */
    if (__builtin_fabsf(x_turn) < __builtin_inff() &&
        __builtin_fabsf(y_turn) < __builtin_inff()) {
        set[1] = set[0];
        for (i = 0; i < EDGE_WORDS; i++) {
            set[0].edges[i] = edges[i];
        }
        set[0].kept = TRUE;
        set[0].normal = *normal;
        set[0].ang_x = ang_x;
        set[0].ang_y = ang_y;
    }
kept:
    if (ang_x) {
        zam_turn(ZAM_X, ang_x);
    }
    if (ang_y) {
        zam_turn(ZAM_Y, ang_y);
    }
}

/* polygon_sweep's touch of polygon, as the shipped code makes it. the contact
 * comes from the ball's pos and speed in the turned space, the normal and the
 * radius. it goes in touch_polygons with a copy of polygon. returns TRUE where
 * that fills touch_polygons. the sweep runs it seldom, so it stays out of line
 * and links last */
LINKED_LAST(sweep_touch)
static __attribute__((noinline)) Bool sweep_touch(const struct polygon *polygon,
                                                  const NJS_POINT3 *pos,
                                                  const NJS_VECTOR *speed,
                                                  const NJS_VECTOR *normal,
                                                  Float radius,
                                                  Uint32 two_sided)
{
    struct touch_polygon *touch = &touch_polygons[touch_polygon_count];
    struct polygon_contact contact;
    NJS_VECTOR part;
    NJS_POINT3 at;

    if (debug_hidden >= 0) {
        touch_points[0] = polygon->vertex[0];
        touch_points[1] = polygon->vertex[1];
        touch_points[2] = polygon->vertex[2];
        touch_points[0].x += normal->x;
        touch_points[1].x += normal->x;
        touch_points[2].x += normal->x;
        touch_points[0].y += normal->y;
        touch_points[1].y += normal->y;
        touch_points[2].y += normal->y;
        touch_points[0].z += normal->z;
        touch_points[1].z += normal->z;
        touch_points[2].z += normal->z;
        njGetMatrix(&touch_matrix);
        njPopMatrix(1);
        njDrawTriangle3D(&touch_triangle, 3, NJD_FILL);
        njPushMatrix(&touch_matrix);
    }
    contact.height = pos->y;
    contact.height_after = pos->y + speed->y;
    contact.hit.front = *normal;
    if (two_sided && !(pos->y >= 0.0F)) {
        contact.hit.normal.x = -normal->x;
        contact.hit.normal.y = -normal->y;
        contact.hit.normal.z = -normal->z;
    } else {
        contact.hit.normal = *normal;
    }
    contact.hit.ang_z =
        -(Angle)(atan2f(contact.hit.normal.x, contact.hit.normal.y) *
                 ANGLE_PER_RADIAN);
    contact.hit.ang_x = (Angle)(asinf(contact.hit.normal.z) * ANGLE_PER_RADIAN);
    njInvertMatrix(NULL);
    part.x = pos->x;
    part.y = 0.0F;
    part.z = pos->z;
    njCalcPoint(NULL, &part, &at);
    contact.hit.point.x = at.x + polygon->vertex[0].x;
    contact.hit.point.y = at.y + polygon->vertex[0].y;
    contact.hit.point.z = at.z + polygon->vertex[0].z;
    part.y = radius;
    njCalcPoint(NULL, &part, &at);
    contact.hit.pos.x = at.x + polygon->vertex[0].x;
    contact.hit.pos.y = at.y + polygon->vertex[0].y;
    contact.hit.pos.z = at.z + polygon->vertex[0].z;
    part.x = speed->x;
    part.y = 0.0F;
    part.z = speed->z;
    njCalcPoint(NULL, &part, &contact.hit.speed);
    part.x = 0.0F;
    part.y = speed->y;
    part.z = 0.0F;
    njCalcPoint(NULL, &part, &contact.hit.normal_speed);
    touch->polygon = *polygon;
    touch->contact = contact;
    if (++touch_polygon_count == POLYGON_CONTACTS) {
        if (debug_hidden >= 0) {
            njPrintC(NJM_LOCATION(2, 22), "WARNING: TOUCH-poly buf is FULL");
        }
        return TRUE;
    }
    return FALSE;
}

/* the shipped sweep, made from the same words by the same operations. the
 * ball's point, speed and radius stay in registers, as nothing the sweep
 * writes is the ball. the shipped code pushes the unit matrix for each
 * triangle and pops it after. where both work, a pop and the next push leave
 * the stack as it was. XMTRX then holds the unit matrix. so once a push works
 * on a stack that held one, later triangles load the unit matrix instead. the
 * last triangle pops. a touch that fills touch_polygons leaves its triangle's
 * matrix pushed, as the shipped break does. the stride's length is tested by
 * its square, as the turn's normal's is */
Bool polygon_sweep(struct polygon_ball *ball, NJS_OBJECT *object, Uint32 flags)
{
    struct box_arena *arena;
    struct collision_mesh *mesh;
    struct polygon **near;
    struct polygon *polygon;
    struct touch_polygon *touched;
    struct polygon_contact *listed;
    struct polygon_contact *hit_contact;
    Bool held;
    Bool hit;
    Bool more;
    Uint32 two_sided;
    int count;
    int nearest;
    int i;
    Float reach;
    Float length;
    Float spacing;
    Float travelled;
    Float radius;
    Float height;
    Float scale;
    Float e1;
    Float e2;
    Float e3;
    Float x0;
    Float y0;
    Float z0;
    NJS_POINT3 from;
    NJS_VECTOR moved;
    NJS_POINT3 tri[3];
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    NJS_VECTOR normal;
    NJS_VECTOR stride;
    NJS_POINT3 at;

    reach = ball->radius;
    if (reach <= 0.0F) {
        return FALSE;
    }
    touch_polygon_count = 0;
    length = njScalor(&ball->speed);
    spacing = reach * 0.5F;
    arena = arena_held();
    if (arena != NULL) {
        mesh = index_mesh(arena, object);
    } else {
        for (mesh = collision_mesh_list; mesh != NULL; mesh = mesh->next) {
            if (mesh->object == object) {
                break;
            }
        }
    }
    if (mesh == NULL) {
        return FALSE;
    }
    from = ball->pos;
    moved = ball->speed;
    held = FALSE;
    tri[0].x = tri[0].y = tri[0].z = 0.0F;
    for (count = mesh->near_count, near = mesh->near; count > 0;
         count--, near++) {
        polygon = *near;
        x0 = polygon->vertex[0].x;
        y0 = polygon->vertex[0].y;
        z0 = polygon->vertex[0].z;
        tri[1].x = polygon->vertex[1].x - x0;
        tri[1].y = polygon->vertex[1].y - y0;
        tri[1].z = polygon->vertex[1].z - z0;
        tri[2].x = polygon->vertex[2].x - x0;
        tri[2].y = polygon->vertex[2].y - y0;
        tri[2].z = polygon->vertex[2].z - z0;
        pos.x = from.x - x0;
        pos.y = from.y - y0;
        pos.z = from.z - z0;
        speed = moved;
        if (held) {
            shz_xmtrx_load_4x4((const shz_mat4x4_t *)&_unit_matrix_);
        } else {
            held = njPushMatrix(&_unit_matrix_) && _stack_size_ > 1;
        }
        sweep_turn(tri,
                   &sweep_turns[(Uint32)(polygon - polygon_pool) % TURN_SETS *
                                TURN_WAYS],
                   &normal);
        njCalcPoint(NULL, &tri[1], &tri[1]);
        njCalcPoint(NULL, &tri[2], &tri[2]);
        njCalcPoint(NULL, &pos, &pos);
        njCalcPoint(NULL, &speed, &speed);
        radius = reach;
        if ((two_sided = polygon->attr & NJD_FLAG_DOUBLE_SIDE) != 0) {
            if (pos.y < 0.0F) {
                radius *= -1.0F;
            }
        }
        hit = FALSE;
        stride = speed;
        if (njScalor2(&stride) == 0.0F) {
            stride.x = stride.y = stride.z = 0.0F;
        } else {
            njUnitVector(&stride);
        }
        stride.x *= spacing;
        stride.y *= spacing;
        stride.z *= spacing;
        travelled = 0.0F;
        at = pos;
        for (;;) {
            more = TRUE;
            if (travelled >= length) {
                more = FALSE;
                at.x = pos.x + speed.x;
                at.y = pos.y + speed.y;
                at.z = pos.z + speed.z;
            }
            if (njAbs(at.y) <= reach) {
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
        }
        if (hit &&
            sweep_touch(polygon, &pos, &speed, &normal, radius, two_sided)) {
            /* this side keeps the play of the shipped sweep's bug tag: the
             * break leaves this triangle's matrix pushed */
            break;
        }
        if (!held || count == 1) {
            njPopMatrix(1);
        }
    }
    count = touch_polygon_count;
    if (count < 1) {
        return FALSE;
    }
    nearest = 0;
    height = touch_polygons[0].contact.height_after;
    for (i = 1, touched = &touch_polygons[1]; i < count; i++, touched++) {
        if (height > touched->contact.height_after) {
            height = touched->contact.height_after;
            nearest = i;
        }
    }
    for (i = count, touched = touch_polygons, listed = polygon_contacts; i > 0;
         i--, touched++, listed++) {
        *listed = touched->contact;
    }
    ball->count = count;
    ball->hit_index = nearest;
    hit_contact = &touch_polygons[nearest].contact;
    ball->hit.ang_x = hit_contact->hit.ang_x;
    ball->hit.ang_z = hit_contact->hit.ang_z;
    ball->hit.front = hit_contact->hit.front;
    ball->hit.normal = hit_contact->hit.normal;
    ball->hit.speed = hit_contact->hit.speed;
    ball->hit.normal_speed = hit_contact->hit.normal_speed;
    if (count == 1) {
        ball->hit.point = hit_contact->hit.point;
        ball->hit.pos = hit_contact->hit.pos;
    } else {
        touched = touch_polygons;
        ball->hit.point = touched->contact.hit.point;
        ball->hit.pos = touched->contact.hit.pos;
        touched++;
        for (i = count - 1; i > 0; i--) {
            ball->hit.point.x += touched->contact.hit.point.x;
            ball->hit.point.y += touched->contact.hit.point.y;
            ball->hit.point.z += touched->contact.hit.point.z;
            ball->hit.pos.x += touched->contact.hit.pos.x;
            ball->hit.pos.y += touched->contact.hit.pos.y;
            ball->hit.pos.z += touched->contact.hit.pos.z;
            touched++;
        }
        scale = 1.0F / (Float)count;
        ball->hit.point.x *= scale;
        ball->hit.point.y *= scale;
        ball->hit.point.z *= scale;
        ball->hit.pos.x *= scale;
        ball->hit.pos.y *= scale;
        ball->hit.pos.z *= scale;
    }
    return TRUE;
}
#else

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
#endif

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
#ifdef NONMATCHING
                /* repairs the reads the tag names: this side measures to the
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
#ifdef NONMATCHING
                /* repairs the reads the tag names: this side measures to the
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
#ifdef NONMATCHING
                /* repairs the reads the tag names: this side measures to the
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
#ifdef NONMATCHING
    /* keeps the image's result, no floor, except where the shipped reach test
     * passes. that test measures to stack words, and SIZE_REACH2's shift
     * leaves the sizes smaller than WORLD_HUGE no reach. this side finds no
     * floor anywhere */
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
#ifdef NONMATCHING
                /* repairs the reads the tag names: this side measures to the
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
