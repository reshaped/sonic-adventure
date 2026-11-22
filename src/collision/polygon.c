/* the collision meshes of the world's objects, and tests of points, balls,
 * rays and segments against those objects */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#ifdef NONMATCHING
#include <shinobi.h>
#include "collision/capsule.h"
#include "collision/support.h"
#endif
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
#endif
#endif

/* how far a test looks for triangles: around a point in a mesh or the
 * biggest objects' models, and past a ray's end */
#define REACH 220.0F

/* the longest ray polygon_cast_ray follows */
#define RAY_LENGTH 400.0F

/* the most points a strip holds: strip_next cuts a longer one short */
#define STRIP_POINTS 1024
#define MESHES COLLISION_MESH_RECORDS /* the records in mesh_pool */
#ifdef NONMATCHING
/* only entries the update admits may keep a mesh from its previous step */
#define MESH_ENTRIES (world_count < MESHES ? world_count : MESHES)
#else
#define MESH_ENTRIES world_count
#endif
#define MESH_POLYGONS 8192 /* the most polygons in all the meshes */
#define NEAR_POLYGONS 96 /* the original near list and DC turn cache size */

/* the original collector's multiplier for the squared reach */
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
#ifdef NONMATCHING
/* A full polygon pool switches mesh admission to whole models. */
Bool collision_mesh_pressure;
static int mesh_plan_capacity;
struct mesh_budget {
    /* Slots hold one-based indices in world_objects. Zero is empty. */
    Uint8 first[256];
    Uint8 state[256];
    Uint16 demand[256];
};
#endif
/* the triangles polygon_collect_near listed, mesh by mesh */
static int near_polygon_count;
#ifdef NONMATCHING
/* Each group starts at a hit. Its mask covers the next 32 mesh-list nodes
 * including that hit. Each mesh can leave only its last group short. */
struct near_group {
    struct polygon *first;
    Uint32 mask;
};
/* Keep the table's size out of the mesh and polygon pools' layout. */
struct near_group collision_near_groups[MESH_POLYGONS / 32 + MESHES]
    __attribute__((common));
#else
static struct polygon *near_polygons[NEAR_POLYGONS];
#endif
#ifdef NONMATCHING
#ifdef DREAMCAST
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
    for (i = MESH_ENTRIES, entry = world_objects; i > 0; i--, entry++) {
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

/* The indexed side search falls back to the original entry point. */
#undef polygon_find_sides
/* what collision_mesh_init, collision_mesh_link_pools and
 * collision_mesh_update call in place of the shipped functions. each runs its
 * shipped function and the index's own work. they stay out of line, so each
 * call keeps the shipped call's size */
void collision_mesh_pool_init_indexed(struct collision_mesh *meshes, int count);
void collision_mesh_take_buckets_indexed(struct collision_mesh *mesh);
void collision_mesh_free_indexed(struct collision_mesh *mesh);
#else
/* The console can index the same mesh list the native port walks. */
struct box_arena;
static inline struct box_arena *arena_held(void)
{
    return NULL;
}
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
#ifdef NONMATCHING
    collision_mesh_pressure = FALSE;
#endif
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

#ifdef NONMATCHING
enum { MESH_BATCH_TRIANGLES = 64 };

struct mesh_strip {
    int kind, count, left, strips;
    Bool reversed;
    const Sint16 *indices;
    const NJS_MODEL *model;
    NJS_POINT3 points[MESH_BATCH_TRIANGLES + 2];
};

/* Keep the preceding edge and its winding when a strip crosses a batch. */
ALWAYS_INLINE Bool mesh_strip_batch(struct mesh_strip *strip,
                                    const NJS_OBJECT *unturned)
{
    int kept = 0;
    if (strip->left) {
        strip->reversed ^= (strip->count - 2) & 1;
        strip->points[0] = strip->points[strip->count - 2];
        strip->points[1] = strip->points[strip->count - 1];
        kept = 2;
    } else {
        do {
            if (!strip->strips) {
                return FALSE;
            }
            strip->strips--;
            strip->reversed = FALSE;
            if (strip->kind == NJD_MESHSET_3 || strip->kind == NJD_MESHSET_4) {
                strip->left = strip->kind == NJD_MESHSET_3 ? 3 : 4;
            } else {
                Uint16 header = *strip->indices++;
                strip->left = header & STRIP_COUNT;
                strip->reversed = (header & NJD_TRIMESH_END) != 0;
            }
            if (strip->left < 3) {
                strip->indices += strip->left;
            }
        } while (strip->left < 3);
    }
    int count = MESH_BATCH_TRIANGLES + 2 - kept;
    if (count > strip->left) {
        count = strip->left;
    }
    strip->left -= count;
    strip->count = kept + count;
    const Sint16 *indices = strip->indices;
    NJS_POINT3 *points = strip->model->points;
    NJS_POINT3 *out = strip->points + kept;
    if (!unturned) {
        for (int i = 0; i < count; i++) {
            njCalcPoint(NULL, &points[*indices++], out++);
        }
    } else if (unturned->evalflags & NJD_EVAL_UNIT_POS) {
        for (int i = 0; i < count; i++) {
            *out++ = points[*indices++];
        }
    } else {
        Float x = unturned->pos[0], y = unturned->pos[1], z = unturned->pos[2];
        for (int i = 0; i < count; i++) {
            const NJS_POINT3 *point = &points[*indices++];
            out->x = point->x + x;
            out->y = point->y + y;
            out->z = point->z + z;
            out++;
        }
    }
    strip->indices = indices;
    return TRUE;
}
#else
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
#endif

/* put the triangles of object's own model, through the current matrix, in
 * the buckets of the mesh being made. returns FALSE when the polygons run
 * out, and TRUE otherwise. flags and mesh go unread */
static Bool mesh_add_model(NJS_OBJECT *object, Sint32 flags,
                           struct collision_mesh *mesh)
{
#ifdef NONMATCHING
    struct mesh_strip strip;
#else
    struct strip strip;
#endif
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
#ifdef NONMATCHING
        strip.left = 0;
        strip.strips = strips;
        while (mesh_strip_batch(&strip, NULL)) {
            reversed = strip.reversed;
#else
        for (; strips > 0; strips--) {
            reversed = mesh_strip_next(&strip);
#endif
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
#ifdef NONMATCHING
        mesh->out_of_polygons = TRUE;
        collision_mesh_pressure = TRUE;
#endif
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
#include "collision/bounds.h"

LINKED_LAST(sweep_sphere_bounds)
static __attribute__((noinline)) void
sweep_sphere_bounds(struct sweep_bounds *box, NJS_POINT3 pos, Float radius)
{
    sweep_extents(box, pos, (NJS_VECTOR){0}, radius);
}

LINKED_LAST(sweep_bounds)
static __attribute__((noinline)) void
sweep_bounds(struct sweep_bounds *box, const struct polygon_ball *ball)
{
    sweep_extents(box, ball->pos, ball->speed, ball->radius);
}

struct near_cursor {
    const struct near_group *group;
    struct polygon *polygon;
    Uint32 mask;
    Bool back;
};

ALWAYS_INLINE struct polygon *near_next(struct near_cursor *cursor)
{
    if (cursor->mask == 0) {
        cursor->polygon = cursor->group->first;
        cursor->mask = cursor->group++->mask;
    } else {
        do {
            cursor->polygon =
                cursor->back ? cursor->polygon->prev : cursor->polygon->next;
            cursor->mask >>= 1;
        } while (!(cursor->mask & 1));
    }
    cursor->mask &= ~1U;
    return cursor->polygon;
}

LINKED_LAST(polygon_ball_touches)
Bool polygon_ball_touches(const struct polygon_ball *ball,
                          const struct world_object *entry)
{
    if (!(ball->radius > 0.0F)) {
        return FALSE;
    }
    struct sweep_bounds box;
    sweep_bounds(&box, ball);
    Float radius2 = ball->radius * ball->radius;
    Float speed2 = capsule_dot(ball->speed, ball->speed);
    for (struct collision_mesh *mesh = collision_mesh_list; mesh;
         mesh = mesh->next) {
        if (!mesh->near_count || (entry && mesh->object != entry->object)) {
            continue;
        }
        struct near_cursor cursor = {.group = mesh->near,
                                     .back = mesh->near_back};
        for (int i = 0; i < mesh->near_count; i++) {
            struct polygon *triangle = near_next(&cursor);
            if (!sweep_box_misses(&box, triangle->vertex) &&
                capsule_triangle(triangle->vertex, ball->pos, ball->speed,
                                 speed2, radius2)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

#ifdef DREAMCAST
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

#else
ALWAYS_INLINE void near_load(const struct polygon *polygon, Float *x, Float *y,
                             Float *z, Float *radius)
{
    *x = polygon->center.x;
    *y = polygon->center.y;
    *z = polygon->center.z;
    *radius = polygon->radius;
}
#endif

/* Extra squared distance admitted by the shipped point query. */
#define HEIGHTS_MARGIN 900.0F

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
    MESH_TEST_HEIGHTS(query, polygon, mesh_flags);
}

/* centers within a bucket can be in any order. polygon_alloc truncates
 * before the shift. the end buckets also take every center beyond them */
ALWAYS_INLINE Float near_bucket_end(const struct collision_mesh *mesh,
                                    Float bound, Bool back)
{
    const int width = 1 << MESH_BUCKET_BITS;
    const int first = -MESH_BUCKETS / 2, last = MESH_BUCKETS / 2 - 1;
    Float z = bound - mesh->center.z;
    int bucket = !(z > first * width) ? first
                 : z >= last * width  ? last
                                      : (Sint32)z >> MESH_BUCKET_BITS;
    if (bucket == (back ? first : last)) {
        return back ? -__builtin_inff() : __builtin_inff();
    }
    Float edge = mesh->center.z + (back ? bucket : bucket + 1) * width;
    /* truncation brings a negative fraction toward the next whole unit */
    Float pad = 1.0F + __builtin_fabsf(mesh->center.z) * 0x1p-21F;
    return back ? edge - pad : edge + pad;
}

ALWAYS_INLINE struct near_group *near_walk(struct collision_mesh *mesh,
                                           struct polygon *polygon, Bool back,
                                           Float start, Float limit, Float x,
                                           Float y, Float z, Float reach2,
                                           struct near_group *out)
{
    Uint32 bit = 0;
    struct near_group *first = out;
    int count = 0;
    struct polygon *next;
    struct polygon *end = back ? mesh->polygons->prev : mesh->last->next;
    while (polygon != end &&
           (back ? polygon->center.z > start : polygon->center.z < start)) {
        polygon = back ? polygon->prev : polygon->next;
    }
    for (; polygon != end; polygon = next, bit <<= 1) {
        Float cx, cy, cz, radius;
        near_load(polygon, &cx, &cy, &cz, &radius);
        if (back ? cz < limit : cz > limit) {
            break;
        }
        next = back ? polygon->prev : polygon->next;
        Float dx = cx - x;
        Float dy = cy - y;
        Float dz = cz - z;
#ifdef DREAMCAST
        SHZ_PREFETCH(next);
        Float distance =
            __builtin_fmaf(dz, dz, __builtin_fmaf(dx, dx, dy * dy));
#else
        Float distance = (dx * dx) + (dy * dy) + (dz * dz);
#endif
        /* (a + b) squared is at most twice the sum of the squares. the
         * margin covers rounding the two squared radii */
        if (distance > (reach2 + radius) * (2.0F + 0x1p-12F)) {
            continue;
        }
        if (bit == 0) {
            out->first = polygon;
            out->mask = 1;
            out++;
            bit = 1;
        } else {
            out[-1].mask |= bit;
        }
        count++;
    }
    mesh->near = count ? first : NULL;
    mesh->near_count = count;
    mesh->near_back = back;
    return out;
}

int polygon_collect_near(struct polygon_ball *ball)
{
    struct near_group *out = collision_near_groups;
    int total = 0;
    struct collision_mesh *mesh;
    struct polygon *polygon;
    struct world_object *object;
    int i;
    Float x = ball->pos.x;
    Float y = ball->pos.y;
    Float z = ball->pos.z;
    Float reach = njScalor(&ball->speed) + ball->radius;
    Float reach2 = reach * reach;

    world_list_near(x, y, z, reach);
    for (mesh = collision_mesh_list; mesh != NULL; mesh = mesh->next) {
        mesh->near = NULL;
        mesh->near_count = 0;
        if (mesh->polygons == NULL) {
            continue;
        }
        for (i = world_count, object = world_objects; i > 0; i--, object++) {
            if (object->object == mesh->object) {
                break;
            }
        }
        if (i == 0) {
            continue;
        }
        polygon = mesh->polygons;
        Bool back = FALSE;
        Float start = -__builtin_inff(), limit = __builtin_inff();
        if (mesh->bucketed) {
            Float low = z - mesh->span - reach;
            Float high = mesh->span + reach + z;
            back = !(polygon->center.z > low ||
                     (low > polygon->center.z && mesh->last->center.z > high));
            /* the stored centroid uses 0.3333 times the vertex sum. its
             * drift grows with the absolute world coordinate */
            Float pad =
                (__builtin_fabsf(z) + mesh->span + reach + 1.0F) * 0x1p-12F;
            start = back ? high + pad : low - pad;
            limit = near_bucket_end(mesh, back ? low - pad : high + pad, back);
        }
        if (back) {
            polygon = mesh->last;
        }
        out =
            near_walk(mesh, polygon, back, start, limit, x, y, z, reach2, out);
        total += mesh->near_count;
    }
    near_polygon_count = total;
    return total;
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
#ifdef NONMATCHING
    collision_mesh_pressure = FALSE;
#endif
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

#ifdef NONMATCHING
/* Count the polygons mesh_add_tree would allocate, without transforming
 * vertices. A model that cannot fit need not be counted to the end. */
LINKED_LAST(mesh_fits)
static __attribute__((noinline)) Bool mesh_fits(const NJS_OBJECT *object,
                                                int *space)
{
    for (const NJS_OBJECT *child = object->child; child;
         child = child->sibling) {
        if (!mesh_fits(child, space)) {
            return FALSE;
        }
    }
    const NJS_MODEL *model = object->model;
    for (int i = 0; i < model->nbMeshset; i++) {
        const NJS_MESHSET *set = &model->meshsets[i];
        int type = set->type_matId & NJD_MESHSET_MASK;
        if (type == NJD_MESHSET_3 || type == NJD_MESHSET_4) {
            *space -= set->nbMesh * (type == NJD_MESHSET_3 ? 1 : 2);
        } else {
            const Sint16 *indices = set->meshes;
            for (int j = 0; j < set->nbMesh; j++) {
                int points = *indices & STRIP_COUNT;
                if (points > 2) {
                    *space -= points - 2;
                    if (*space < 0) {
                        return FALSE;
                    }
                }
                indices += points + 1;
            }
        }
        if (*space < 0) {
            return FALSE;
        }
    }
    return TRUE;
}

enum { MESH_DENIED = 1, MESH_VISITED = 2, MESH_COUNTED = 4 };
_Static_assert(MESHES < 256, "a pressure slot must fit a one-based mesh index");

LINKED_LAST(mesh_budget_slot)
static __attribute__((noinline)) unsigned
mesh_budget_slot(const struct mesh_budget *budget, const NJS_OBJECT *object)
{
    unsigned key = (uintptr_t)object / sizeof *object;
    for (;;) {
        unsigned slot = key++ % 256;
        int first = budget->first[slot];
        if (!first || world_objects[first - 1].object == object) {
            return slot;
        }
    }
}

/* A deferred object keeps a record without claiming any polygon slots. */
LINKED_LAST(mesh_add_planned)
static __attribute__((noinline)) void
mesh_add_planned(const struct mesh_budget *budget, struct collision_mesh *mesh)
{
    unsigned slot = mesh_budget_slot(budget, mesh->object);
    int first = budget->first[slot];
    if (!world_objects[first - 1].owner && !(mesh->flags & WORLD_MOVES)) {
        mesh->plan.rank = first;
        mesh->plan.demand = budget->demand[slot];
    }
    if (budget->state[slot] & MESH_DENIED) {
        mesh->out_of_polygons = TRUE;
    } else {
        mesh_add_tree(mesh->object, mesh->flags, mesh);
    }
}

/* Both pool readouts share the same columns and warning color. */
LINKED_LAST(mesh_print_capacity)
static __attribute__((noinline)) void
mesh_print_capacity(int row, const char *label, int percent, int value)
{
    njPrintC(NJM_LOCATION(10, row), label);
    if (percent >= USED_WARNING) {
        njPrintColor(COLOR_WARNING);
    }
    njPrintD(NJM_LOCATION(15, row), percent, 3);
    njPrintC(NJM_LOCATION(18, row), "%");
    if (percent >= USED_WARNING) {
        njPrintColor(COLOR_TEXT);
    }
    njPrintD(NJM_LOCATION(20, row), value, 5);
}

LINKED_LAST(mesh_print_pools)
static __attribute__((noinline)) void mesh_print_pools(int free, int total,
                                                       int most)
{
    mesh_print_capacity(26, "obj:", 100 - free * 100 / MESHES, MESHES - free);
    if (free == 0 && debug_hidden >= 0) {
        njPrintC(NJM_LOCATION(2, 19), "WARNING: OBJECT buf is FULL");
    }
    njPrintC(NJM_LOCATION(27, 26), "polMAX:");
    njPrintD(NJM_LOCATION(35, 26), most, 4);
    mesh_print_capacity(27, "r-p:", total * 100 / MESH_POLYGONS, total);
    if (free == 0 && debug_hidden >= 0) {
        njPrintC(NJM_LOCATION(2, 20), "WARNING: RANGE-poly buf is FULL");
    }
}

static Bool mesh_plan_pressure(struct mesh_budget *budget, Bool first);
#endif

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

#ifdef NONMATCHING
    /* The normal update needs no plan. Reserve its fixed workspace only
     * after the pool has actually filled. */
    struct mesh_budget *budget = NULL;
#endif
    if (!mesh_collect_all) {
        world_collect_near_players();
    } else {
        world_collect_all();
        mesh_collect_all = FALSE;
    }
#ifdef NONMATCHING
    if (collision_mesh_pressure) {
        budget = __builtin_alloca(sizeof *budget);
        if (!mesh_plan_pressure(budget, FALSE)) {
            return TRUE;
        }
    }
update_meshes:
#endif
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
#ifdef NONMATCHING
            if (budget &&
                budget->first[mesh_budget_slot(budget, entry->object)] !=
                    entry - world_objects + 1) {
                continue;
            }
#endif
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
#ifdef NONMATCHING
        /* even a full set can replace meshes from the previous step */
#else
        if (wanted == MESHES) {
            break;
        }
#endif
        for (mesh = collision_mesh_list; mesh != NULL; mesh = next) {
            found = FALSE;
#ifdef NONMATCHING
            entry = mesh_find(mesh->object)->first;
            if (entry != NULL) {
                found = !((entry->flags & WORLD_MOVES) ||
                          mesh_stale_object == entry->object);
            }
#else
            for (i = MESH_ENTRIES, entry = world_objects; i > 0; i--, entry++) {
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
#ifdef NONMATCHING
            if (mesh == NULL) {
                break;
            }
#endif
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
#ifdef NONMATCHING
            mesh->polygons = NULL;
#else
            mesh->polygons = NULL;
            mesh->unk_18 = 0;
#endif
            mesh->near = NULL;
            mesh->near_count = 0;
#ifdef NONMATCHING
            mesh->plan.rank = 0;
            if (budget) {
                mesh_add_planned(budget, mesh);
            } else {
                mesh_add_tree(*object, *flag, mesh);
            }
            object++;
#else
            mesh_add_tree(*object++, *flag, mesh);
#endif
#ifdef NONMATCHING
            collision_mesh_take_buckets_indexed(mesh);
#else
            collision_mesh_take_buckets(mesh);
#endif
#ifdef NONMATCHING
            mesh->span = mesh_span;
#else
            mesh->span = mesh_span;
            mesh->unk_2c = dat_8c74872c;
#endif
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
#ifdef NONMATCHING
        mesh_print_pools(free, total, most);
#else
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
#endif
    }
#ifdef NONMATCHING
    if (!budget && collision_mesh_pressure) {
        budget = __builtin_alloca(sizeof *budget);
        mesh_plan_pressure(budget, TRUE);
        goto update_meshes;
    }
#endif
    return TRUE;
}

#ifdef NONMATCHING
/* A deferred object keeps its mesh record so queries can distinguish pool
 * pressure from an unknown object. It owns no polygon slots. */
LINKED_LAST(mesh_plan_pressure)
static __attribute__((noinline)) Bool
mesh_plan_pressure(struct mesh_budget *budget, Bool first)
{
    int available = collision_polygon_capacity;
    int i;
    struct collision_mesh *mesh, *next;
    world_prioritize_meshes();
    /* Stable land keeps its transformed polygons. A moving or task-owned
     * object can change pose or topology without changing its address. */
    if (!first && mesh_plan_capacity == available && debug_hidden < 0 &&
        !mesh_stale_object) {
        int seen = 0;
        struct collision_mesh *mesh = collision_mesh_list;
        for (; mesh; mesh = mesh->next) {
            int rank = mesh->plan.rank - 1;
            if ((unsigned)rank >= (unsigned)MESH_ENTRIES) {
                break;
            }
            struct world_object *entry = &world_objects[rank];
            if (!mesh->object || entry->object != mesh->object ||
                entry->owner || entry->flags != mesh->flags ||
                (entry->flags & WORLD_MOVES)) {
                break;
            }
            seen++;
        }
        if (!mesh && seen == MESH_ENTRIES) {
            return FALSE;
        }
    }
    /* Charge each tree once, even when several entries reference it. */
    __builtin_memset(budget, 0, sizeof *budget);
    for (i = 0; i < MESH_ENTRIES; i++) {
        unsigned slot = mesh_budget_slot(budget, world_objects[i].object);
        if (!budget->first[slot]) {
            budget->first[slot] = i + 1;
        }
    }
    for (mesh = collision_mesh_list; mesh; mesh = mesh->next) {
        unsigned slot = mesh_budget_slot(budget, mesh->object);
        int rank = budget->first[slot] - 1;
        if (rank >= 0 && mesh_plan_capacity == available && mesh->plan.rank &&
            !world_objects[rank].owner &&
            !(world_objects[rank].flags & WORLD_MOVES) &&
            mesh->object != mesh_stale_object) {
            budget->demand[slot] = mesh->plan.demand;
            budget->state[slot] = MESH_COUNTED;
        }
    }
    collision_mesh_pressure = FALSE;
    for (i = 0; i < MESH_ENTRIES; i++) {
        NJS_OBJECT *object = world_objects[i].object;
        unsigned slot = mesh_budget_slot(budget, object);
        if (budget->first[slot] != i + 1) {
            continue;
        }
        int demand = budget->demand[slot];
        if (!(budget->state[slot] & MESH_COUNTED) ||
            collision_polygon_capacity >= 65535) {
            int left = collision_polygon_capacity;
            demand = mesh_fits(object, &left)
                         ? collision_polygon_capacity - left
                         : collision_polygon_capacity + 1;
            budget->demand[slot] = demand < 65535 ? demand : 65535;
        }
        if (demand > available) {
            budget->state[slot] |= MESH_DENIED;
            collision_mesh_pressure = TRUE;
        } else {
            available -= demand;
        }
    }
    for (mesh = collision_mesh_list; mesh; mesh = next) {
        next = mesh->next;
        unsigned slot = mesh_budget_slot(budget, mesh->object);
        Bool denied = (budget->state[slot] & MESH_DENIED) != 0;
        int rank = budget->first[slot] - 1;
        if (rank < 0 || (budget->state[slot] & MESH_VISITED) ||
            world_objects[rank].flags != mesh->flags ||
            (world_objects[rank].owner && !denied) ||
            (first && mesh->out_of_polygons) ||
            denied != mesh->out_of_polygons) {
#ifdef DREAMCAST
            collision_mesh_free_indexed(mesh);
#else
            collision_mesh_free(mesh);
#endif
        } else {
            mesh->plan.rank =
                world_objects[rank].owner || (mesh->flags & WORLD_MOVES)
                    ? 0
                    : budget->first[slot];
            mesh->plan.demand = budget->demand[slot];
        }
        if (budget->first[slot]) {
            budget->state[slot] |= MESH_VISITED;
        }
    }
    mesh_plan_capacity = collision_polygon_capacity;
    return TRUE;
}
#endif

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
/* Indexed alongside polygon_contacts for the duration of this sweep. */
static const struct polygon *
    sweep_faces[POLYGON_CONTACTS] LINKED_LAST_DATA(sweep_faces);

LINKED_LAST(sweep_draw)
static void sweep_draw(const struct polygon *polygon, shz_vec3_t normal)
{
    for (int i = 0; i < 3; i++) {
        zam_store_vec3(
            &touch_points[i],
            shz_vec3_add(zam_load_vec3(polygon->vertex + i), normal));
    }
    njDrawTriangle3D(&touch_triangle, 3, NJD_FILL);
}

#endif

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
 * object. a mesh that doesn't move waits for a search to want its boxes.
 * a null mesh takes the shipped call alone */
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

/* Group bounds cover every member even when their centers share a bucket
 * in arbitrary order. Once the group lies beyond the last possible bucket,
 * every following group in the direction of travel does too. */
LINKED_LAST(indexed_heights)
static __attribute__((noinline)) void
indexed_heights(struct height_query *query, struct box_walk *walk,
                struct collision_mesh *mesh, Bool back, Float start, Float stop)
{
    struct polygon_box *boxes = walk->next;
    int count = walk->end - boxes;
    int groups = groups_of(count);
    const struct group_centers *centers =
        (const struct group_centers *)(walk->end + groups);
    int step = back ? -1 : 1;
    int end = back ? -1 : groups;
    for (int g = back ? groups - 1 : 0; g != end; g += step) {
        if (back ? centers[g].high < stop : centers[g].low > stop) {
            break;
        }
        if ((back ? centers[g].low > start : centers[g].high < start) ||
            !box_heights(walk, walk->end + g)) {
            continue;
        }
        int first = GROUP_BOXES * g;
        int past = first + GROUP_BOXES;
        if (past > count) {
            past = count;
        }
        int end_face = back ? first - 1 : past;
        for (int i = back ? past - 1 : first; i != end_face; i += step) {
            if (box_heights(walk, &boxes[i])) {
                heights_visit(query, &polygon_pool[boxes[i].polygon],
                              mesh->flags);
            }
        }
    }
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
    Float normal_squared;
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
    normal_squared = njScalor2(normal);
    if (normal_squared == 0.0F) {
        normal->x = 0.0F;
        normal->y = 1.0F;
        normal->z = 0.0F;
    } else {
        zam_store_vec3(normal, shz_vec3_scale(zam_load_vec3(normal),
                                              _builtin_fsrra(normal_squared)));
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

/* Record this face in world space. The debug draw uses the caller's matrix.
 * Returning TRUE stops the sweep at the last output slot. */
LINKED_LAST(sweep_touch)
static __attribute__((noinline)) Bool
sweep_touch(const struct polygon *polygon, const NJS_POINT3 *pos,
            const NJS_VECTOR *speed, const NJS_VECTOR *normal, Float radius,
            Uint32 two_sided, const shz_mat4x4_t *caller)
{
    struct polygon_contact *contact = &polygon_contacts[touch_polygon_count];
    NJS_VECTOR part;
    NJS_POINT3 at;

    if (debug_hidden >= 0) {
        njGetMatrix(&touch_matrix);
        shz_xmtrx_load_4x4(caller);
        sweep_draw(polygon, zam_load_vec3(normal));
        shz_xmtrx_load_4x4((const shz_mat4x4_t *)&touch_matrix);
    }
    sweep_faces[touch_polygon_count] = polygon;
    contact->height = pos->y;
    contact->height_after = pos->y + speed->y;
    contact->hit.front = *normal;
    if (two_sided && !(pos->y >= 0.0F)) {
        contact->hit.normal.x = -normal->x;
        contact->hit.normal.y = -normal->y;
        contact->hit.normal.z = -normal->z;
    } else {
        contact->hit.normal = *normal;
    }
    contact->hit.ang_z =
        -(Angle)(atan2f(contact->hit.normal.x, contact->hit.normal.y) *
                 ANGLE_PER_RADIAN);
    contact->hit.ang_x =
        (Angle)(asinf(contact->hit.normal.z) * ANGLE_PER_RADIAN);
    njInvertMatrix(NULL);
    part.x = pos->x;
    part.y = 0.0F;
    part.z = pos->z;
    njCalcPoint(NULL, &part, &at);
    contact->hit.point.x = at.x + polygon->vertex[0].x;
    contact->hit.point.y = at.y + polygon->vertex[0].y;
    contact->hit.point.z = at.z + polygon->vertex[0].z;
    part.y = radius;
    njCalcPoint(NULL, &part, &at);
    contact->hit.pos.x = at.x + polygon->vertex[0].x;
    contact->hit.pos.y = at.y + polygon->vertex[0].y;
    contact->hit.pos.z = at.z + polygon->vertex[0].z;
    part.x = speed->x;
    part.y = 0.0F;
    part.z = speed->z;
    njCalcPoint(NULL, &part, &contact->hit.speed);
    part.x = 0.0F;
    part.y = speed->y;
    part.z = 0.0F;
    njCalcPoint(NULL, &part, &contact->hit.normal_speed);
    if (++touch_polygon_count == POLYGON_CONTACTS) {
        if (debug_hidden >= 0) {
            njPrintC(NJM_LOCATION(2, 22), "WARNING: TOUCH-poly buf is FULL");
        }
        return TRUE;
    }
    return FALSE;
}

/* Existing overlaps and slides keep the game's half-radius samples. The
 * whole segment misses the sampled slab if its ends lie beyond one side. */
ALWAYS_INLINE Bool sweep_samples(const NJS_POINT3 tri[3], const NJS_POINT3 *pos,
                                 const NJS_VECTOR *speed, Float reach,
                                 Float length)
{
    Float end_y = pos->y + speed->y;
    NJS_POINT3 at;
    Float e1, e2, e3;
    Float spacing = reach * 0.5F;
    Float travelled = 0.0F;
    Float squared;
    NJS_VECTOR stride;

    if ((pos->y > reach && end_y > reach) ||
        (pos->y < -reach && end_y < -reach)) {
        return FALSE;
    }
    stride = *speed;
    squared = njScalor2(&stride);
    if (squared == 0.0F) {
        stride.x = stride.y = stride.z = 0.0F;
    } else {
        zam_store_vec3(&stride, shz_vec3_scale(zam_load_vec3(speed),
                                               _builtin_fsrra(squared)));
    }
    stride.x *= spacing;
    stride.y *= spacing;
    stride.z *= spacing;
    at = *pos;
    for (;;) {
        Bool last = travelled >= length;
        if (last) {
            at.x = pos->x + speed->x;
            at.y = end_y;
            at.z = pos->z + speed->z;
        }
        if (njAbs(at.y) <= reach) {
            e1 = EDGE_SIDE(&tri[0], &tri[1], &at, z, x);
            e2 = EDGE_SIDE(&tri[1], &tri[2], &at, z, x);
            e3 = EDGE_SIDE(&tri[2], &tri[0], &at, z, x);
            if (e1 * e2 >= 0.0F && e2 * e3 >= 0.0F && e1 * e3 >= 0.0F) {
                return TRUE;
            }
        }
        if (last) {
            return FALSE;
        }
        at.x += stride.x;
        at.y += stride.y;
        at.z += stride.z;
        travelled += spacing;
    }
}

#endif

#ifdef NONMATCHING
/* Test first contact with the actual face. The quantized rotation used for
 * the game's response can put a shared vertex on the wrong side of an edge.
 * Measure the sweep relative to the triangle's first vertex. */
LINKED_LAST(sweep_crosses_face)
static __attribute__((noinline, optimize("fp-contract=off"))) Bool
sweep_crosses_face(const struct polygon *polygon,
                   const struct polygon_ball *ball,
                   enum polygon_sweep_mode mode)
{
    struct impact_triangle face;
    if (!impact_triangle_init(&face, polygon->vertex)) {
        return FALSE;
    }
    shz_vec3_t pos =
        shz_vec3_sub(zam_load_vec3(&ball->pos), zam_load_vec3(polygon->vertex));
    shz_vec3_t speed = zam_load_vec3(&ball->speed);
    Float height = plane_dot(face.normal, pos),
          delta = plane_dot(face.normal, speed);
    if (height < 0.0F && (polygon->attr & NJD_FLAG_DOUBLE_SIDE)) {
        height = -height;
        delta = -delta;
    }
    Float clearance = height - ball->radius;
    if (!(clearance > 0.0F && -delta >= clearance) ||
        (mode == POLYGON_SWEEP_SAMPLES && height + delta >= -ball->radius)) {
        return FALSE;
    }
    Float time = clearance / -delta;
    return impact_inside(&face, shz_vec3_add(pos, shz_vec3_scale(speed, time)),
                         ball->radius * 0x1p-16F);
}

/* Repeated triangles on one plane have a single response. Their stored
 * height is signed along the winding, even for two-sided geometry. */
ALWAYS_INLINE Bool sweep_same_plane(int count)
{
    const struct polygon_contact *first = &polygon_contacts[0];
    for (int i = 1; i < count; i++) {
        const struct polygon_contact *other = &polygon_contacts[i];
        if (other->hit.normal.x != first->hit.normal.x ||
            other->hit.normal.y != first->hit.normal.y ||
            other->hit.normal.z != first->hit.normal.z ||
            other->hit.front.x != first->hit.front.x ||
            other->hit.front.y != first->hit.front.y ||
            other->hit.front.z != first->hit.front.z ||
            other->height != first->height) {
            return FALSE;
        }
    }
    return TRUE;
}

/* Only a sphere whose center projects onto the finite face can start a
 * step constrained by that plane. Edge and vertex contacts have other normals.
 */
LINKED_LAST(sweep_start_plane)
static __attribute__((noinline, optimize("fp-contract=off"))) Bool
sweep_start_plane(const struct polygon *poly, shz_vec3_t pos, Float radius,
                  Float tolerance, struct contact_plane *plane)
{
    struct impact_triangle face;
    if (!impact_triangle_init(&face, poly->vertex)) {
        return FALSE;
    }
    shz_vec3_t at = shz_vec3_sub(pos, zam_load_vec3(poly->vertex));
    shz_vec3_t n = face.normal;
    Float height = plane_dot(n, at);
    if (height < 0.0F && (poly->attr & NJD_FLAG_DOUBLE_SIDE)) {
        n = shz_vec3_scale(n, -1.0F);
        height = -height;
    }
    if (!(height >= 0.0F && height <= radius + tolerance) ||
        !impact_inside(&face, at, tolerance)) {
        return FALSE;
    }
    *plane = (struct contact_plane){n, radius - height};
    return TRUE;
}

static inline Bool normal_before(shz_vec3_t a, shz_vec3_t b)
{
    return a.x < b.x ||
           (a.x == b.x && (a.y < b.y || (a.y == b.y && a.z < b.z)));
}

LINKED_LAST(sweep_add_plane)
static __attribute__((noinline)) Bool
sweep_add_plane(struct contact_plane *planes, int *count, int capacity,
                struct contact_plane plane)
{
    /* Put identical sets through the floating-point solve in the same order. */
    shz_vec3_t n = plane.normal;
    int i = 0;
    for (; i < *count; i++) {
        shz_vec3_t old = planes[i].normal;
        if (n.x == old.x && n.y == old.y && n.z == old.z) {
            if (planes[i].offset < plane.offset) {
                planes[i].offset = plane.offset;
            }
            return TRUE;
        }
        if (normal_before(n, old)) {
            break;
        }
    }
    if (*count == capacity) {
        return FALSE;
    }
    for (int j = *count; j > i; j--) {
        planes[j] = planes[j - 1];
    }
    planes[i] = plane;
    (*count)++;
    return TRUE;
}

LINKED_LAST(sweep_clip_planes)
static __attribute__((noinline, optimize("fp-contract=off"))) Bool
sweep_clip_planes(const struct contact_plane *planes, int used,
                  shz_vec3_t speed, Float radius, Float tolerance,
                  shz_vec3_t *correction, shz_vec3_t *allowed)
{
    /* A crush or a narrow wedge can have no clearance within a diameter.
     * Hold the center there and allow motion that opens or maintains every
     * contact, so an open end of the passage remains an escape route. */
    Bool cleared =
        planes_project(planes, used, (shz_vec3_t){0}, tolerance, correction) &&
        plane_dot(*correction, *correction) <= 4.0F * radius * radius;
    if (!cleared) {
        *correction = (shz_vec3_t){0};
    }
    /* Apply push-out once. Speed can use any clearance that push-out
     * opened at another face, and can always carry the ball away. */
    struct contact_plane limits[POLYGON_CONTACTS];
    for (int i = 0; i < used; i++) {
        Float gap = planes[i].offset - plane_dot(planes[i].normal, *correction);
        limits[i] = (struct contact_plane){planes[i].normal,
                                           cleared && gap < 0.0F ? gap : 0.0F};
    }
    if (!planes_project(limits, used, speed, tolerance, allowed)) {
        /* Every speed constraint admits zero at the resolved center. */
        *allowed = (shz_vec3_t){0};
        return FALSE;
    }
    return cleared;
}

/* A sweep also lists triangles encountered later in the step. Only faces
 * the starting sphere actually overlaps can constrain its initial position. */
LINKED_LAST(polygon_resolve_corner)
__attribute__((optimize("fp-contract=off"))) void
polygon_resolve_corner(struct polygon_ball *ball)
{
    struct contact_plane planes[POLYGON_CONTACTS];
    shz_vec3_t pos = zam_load_vec3(&ball->pos);
    shz_vec3_t correction, allowed;
    Float tolerance = ball->radius * (1.0F / 65536.0F);
    int used = 0;
    int count = ball->count;
    /* Filling the output stops the sweep before every face is known. */
    if (count < 2 || count == POLYGON_CONTACTS || sweep_same_plane(count)) {
        return;
    }
    int i;
    for (i = 0; i < count; i++) {
        const struct polygon *poly = sweep_faces[i];
        struct contact_plane plane;
        if (!sweep_start_plane(poly, pos, ball->radius, tolerance, &plane)) {
            return;
        }
        sweep_add_plane(planes, &used, POLYGON_CONTACTS, plane);
    }
    shz_vec3_t start = shz_vec3_sub(zam_load_vec3(&ball->hit.pos), pos);
    shz_vec3_t end = shz_vec3_add(start, zam_load_vec3(&ball->hit.speed));
    i = 0;
    while (i < used &&
           plane_dot(planes[i].normal, start) >= planes[i].offset - tolerance &&
           plane_dot(planes[i].normal, end) >= planes[i].offset - tolerance) {
        i++;
    }
    if (i == used) {
        return;
    }
    sweep_clip_planes(planes, used, zam_load_vec3(&ball->speed), ball->radius,
                      tolerance, &correction, &allowed);
    zam_store_vec3(&ball->hit.pos, shz_vec3_add(pos, correction));
    zam_store_vec3(&ball->hit.speed, allowed);
    ball->hit.point = polygon_contacts[ball->hit_index].hit.point;
}

ALWAYS_INLINE struct collision_mesh *sweep_mesh(struct box_arena *arena,
                                                NJS_OBJECT *object)
{
#ifdef DREAMCAST
    if (arena) {
        return index_mesh(arena, object);
    }
#endif
    for (struct collision_mesh *mesh = collision_mesh_list; mesh;
         mesh = mesh->next) {
        if (mesh->object == object) {
            return mesh;
        }
    }
    return NULL;
}

/* The sweep and ground loops keep their copy inline. */
LINKED_LAST(terrain_mesh)
static __attribute__((noinline)) struct collision_mesh *
terrain_mesh(struct box_arena *arena, NJS_OBJECT *object)
{
    return sweep_mesh(arena, object);
}

/* Walk the linked faces until the last bucket that can contain the point.
 * A list built by direct insertion has no bucket order and no finite stop. */
LINKED_LAST(list_heights)
static __attribute__((noinline)) void list_heights(struct height_query *query,
                                                   struct collision_mesh *mesh,
                                                   Bool back, Float start,
                                                   Float stop)
{
    struct polygon *polygon = back ? mesh->last : mesh->polygons;
    const struct polygon *end = back ? mesh->polygons->prev : mesh->last->next;
    while (polygon != end &&
           (back ? polygon->center.z > start : polygon->center.z < start)) {
        polygon = back ? polygon->prev : polygon->next;
    }
    for (; polygon != end; polygon = back ? polygon->prev : polygon->next) {
        Float center = polygon->center.z;
        if (back ? center < stop : center > stop) {
            break;
        }
        heights_visit(query, polygon, mesh->flags);
    }
}

LINKED_LAST(polygon_find_heights_indexed)
Bool polygon_find_heights_indexed(struct height_query *query,
                                  NJS_OBJECT *object, Uint32 flags)
{
    struct box_arena *arena = arena_held();
    struct collision_mesh *mesh = sweep_mesh(arena, object);
    query->below.found = FALSE;
    query->above.found = FALSE;
    if (!mesh) {
        return FALSE;
    }
    query->below.level = query->pos.y - REACH;
    query->above.level = query->pos.y + REACH;
    if (!mesh->polygons) {
        return FALSE;
    }
    if (mesh->polygons == mesh->last) {
        heights_visit(query, mesh->polygons, mesh->flags);
        return query->below.found || query->above.found;
    }
    /* Keep the original visit direction for ties. Within a bucket, any
     * center can precede any other. Only a later bucket can end the walk. */
    Float z = query->pos.z;
    Float low = z - mesh->span;
    Float high = z + mesh->span;
    Bool back =
        low > mesh->polygons->center.z && !(mesh->last->center.z > high);
    /* mesh_add_tree uses 0.3333 for one third when it stores a centroid. */
    Float pad = (__builtin_fabsf(z) + mesh->span + 1.0F) * 0x1p-12F;
    low -= pad;
    high += pad;
    Float stop = back ? -__builtin_inff() : __builtin_inff();
    if (mesh->bucketed) {
        stop = near_bucket_end(mesh, back ? low : high, back);
    }
#ifdef DREAMCAST
    struct box_walk walk;
    if (arena && mesh->bucketed &&
        box_walk_start(&walk, arena, mesh, &query->pos)) {
        indexed_heights(query, &walk, mesh, back, back ? high : low, stop);
        return query->below.found || query->above.found;
    }
#endif
    list_heights(query, mesh, back, back ? high : low, stop);
    return query->below.found || query->above.found;
}

#include "collision/yaw.h"

struct task *polygon_rotating;
LINKED_LAST(polygon_rotate_with)
void polygon_rotate_with(struct task *owner,
                         const struct polygon_yaw_ops *rotation)
{
    struct task_forces *forces = (struct task_forces *)owner->force;
    if (!forces || forces->rotation == rotation) {
        return;
    }
    if (!forces->rotation) {
        forces->next_rotating = polygon_rotating;
        polygon_rotating = owner;
    } else if (!rotation) {
        struct task_forces *previous = NULL;
        for (struct task *task = polygon_rotating; task != owner;
             task = previous->next_rotating) {
            previous = (struct task_forces *)task->force;
        }
        if (previous) {
            previous->next_rotating = forces->next_rotating;
        } else {
            polygon_rotating = forces->next_rotating;
        }
        forces->next_rotating = NULL;
    }
    forces->rotation = rotation;
}

struct rotating_hit {
    struct yaw_contact contact;
    struct polygon_yaw motion;
    const struct polygon *tri;
    struct task *owner;
    Uint32 flags;
    Float support_clearance;
};

/* An object's root sphere encloses its mesh when it has no child or sibling.
 * Compound meshes take the per-triangle bound below. */
static Bool yaw_misses(const struct polygon_yaw *pose, shz_vec3_t center,
                       shz_vec3_t move, Float radius)
{
    const NJS_OBJECT *object = pose->object;
    if (!object->model || object->child || object->sibling) {
        return FALSE;
    }
    Float scale = njAbs(object->scl[0]);
    for (int i = 1; i < 3; i++) {
        if (njAbs(object->scl[i]) > scale) {
            scale = njAbs(object->scl[i]);
        }
    }
    const NJS_POINT3 *c = &object->model->center;
    Float reach = radius + scale * (object->model->r + njAbs(c->x) +
                                    njAbs(c->y) + njAbs(c->z));
    Float length = plane_dot(move, move);
    Float t = length > 0 ? -plane_dot(center, move) / length : 0;
    if (t > 0) {
        center = shz_vec3_add(center, shz_vec3_scale(move, t < 1 ? t : 1));
    }
    return plane_dot(center, center) > reach * reach * (1.0F + 0x1p-12F);
}

LINKED_LAST(rotating_first)
static enum yaw_result rotating_first(shz_vec3_t center, shz_vec3_t speed,
                                      Float radius, Float start, Float end,
                                      struct rotating_hit *hit)
{
    struct yaw_hits best = {.contact.time = end,
                            .unresolved = __builtin_inff(),
                            .time_tolerance = __builtin_inff()};
    struct box_arena *arena = arena_held();
    for (struct task *owner = polygon_rotating; owner;) {
        struct task_forces *forces = (struct task_forces *)owner->force;
        struct polygon_yaw pose;
        forces->rotation->pose(owner, &pose);
        struct task *current = owner;
        owner = forces->next_rotating;
        if (!(current->work->flags & WORLD_TESTED) || !pose.object ||
            !(pose.turn || pose.travel.x || pose.travel.y || pose.travel.z)) {
            continue;
        }
        shz_vec3_t pivot = zam_load_vec3((const NJS_POINT3 *)pose.object->pos);
        shz_vec3_t travel = zam_load_vec3(&pose.travel);
        shz_vec3_t local = shz_vec3_sub(shz_vec3_sub(center, pivot),
                                        shz_vec3_scale(travel, start));
        shz_vec3_t relative = shz_vec3_sub(speed, travel);
        if (yaw_misses(&pose, local, shz_vec3_scale(relative, end - start),
                       radius)) {
            continue;
        }
        const struct collision_mesh *mesh = terrain_mesh(arena, pose.object);
        if (!mesh || mesh->out_of_polygons) {
            hit->contact.safe = hit->contact.time = start;
            return YAW_UNCERTAIN;
        }
        if (!mesh->polygons) {
            continue;
        }
        NJS_VECTOR delta;
        zam_store_vec3(&delta, relative);
        struct yaw_path path = {
            .origin = shz_vec3_sub(local, shz_vec3_scale(relative, start)),
            .relative = relative,
            .travel = travel,
            .turn = (Float)pose.turn,
            .radius = radius,
            .tolerance = impact_tolerance(radius, delta)};
        if (forces->rotation->cast(mesh, pivot, &path, start, end, &best)) {
            *hit = (struct rotating_hit){best.contact, pose, best.face, current,
                                         mesh->flags};
        }
    }
    if (best.unresolved <= best.contact.time) {
        if (best.face &&
            best.contact.time - best.unresolved <= best.time_tolerance) {
            if (best.unresolved < hit->contact.safe) {
                hit->contact.safe = best.unresolved;
            }
            return YAW_TOUCH;
        }
        hit->contact.safe = hit->contact.time = best.unresolved;
        return YAW_UNCERTAIN;
    }
    return best.face ? YAW_TOUCH : YAW_CLEAR;
}

/* World entries retain their own flags even when they share a mesh. */
struct terrain_cursor {
    struct box_arena *arena;
    struct near_cursor near;
    int next, left;
    Uint32 liquid_mask;
    Bool ordinary;
};

LINKED_LAST(terrain_next_triangle)
static const struct polygon *
terrain_next_triangle(struct terrain_cursor *cursor)
{
    while (!cursor->left) {
        if (cursor->next == world_count) {
            return NULL;
        }
        const struct world_object *entry = &world_objects[cursor->next++];
        if (entry->flags & cursor->liquid_mask) {
            continue;
        }
        struct collision_mesh *mesh = sweep_mesh(cursor->arena, entry->object);
        if (mesh && mesh->near_count) {
            cursor->near = (struct near_cursor){.group = mesh->near,
                                                .back = mesh->near_back};
            cursor->left = mesh->near_count;
            cursor->ordinary = world_plain_land(entry);
        }
    }
    cursor->left--;
    return near_next(&cursor->near);
}

/* Every triangle on a supporting plane shares its clearance, including a
 * neighbor across a triangulation seam. Compare relative to the start. */
static inline Bool wall_on_plane(const struct impact_triangle *face,
                                 shz_vec3_t from,
                                 const struct contact_plane *planes, int used,
                                 Float radius, Float tolerance)
{
    for (int i = 0; i < used; i++) {
        Float height = radius - planes[i].offset;
        int j = 0;
        for (; j < 3; j++) {
            shz_vec3_t v = shz_vec3_sub(from, face->v[j]);
            if (!(njAbs(plane_dot(planes[i].normal, v) - height) <=
                  tolerance)) {
                break;
            }
        }
        if (j == 3) {
            return TRUE;
        }
    }
    return FALSE;
}

/* Unit normals closer than this can represent the same sheet. The vertices
 * must independently pass its signed-distance test. */
#define GROUND_PARALLEL 0.99999F

static inline shz_vec3_t ground_face_normal(const struct polygon *tri,
                                            const struct impact_triangle *face)
{
    if ((tri->attr & NJD_FLAG_DOUBLE_SIDE) && face->normal.y < 0.0F) {
        return shz_vec3_scale(face->normal, -1.0F);
    }
    return face->normal;
}

struct landing {
    const struct polygon *face;
    const struct world_object *source, *owner;
    shz_vec3_t up;
};

struct stride_contact {
    shz_vec3_t normal, tangent;
    Float inward, speed2;
};

/* All movement within a tick is measured from the original sphere center. */
struct wall_motion {
    shz_vec3_t origin, whole, at, velocity, push, normal;
    Float radius, clearance, floor_y, tolerance, remaining;
    struct landing land;
    Float fall_speed, inward;
    Bool can_land, stride, initial, hit_unclimbable_rim, stalled;
    Bool owner_probe_hit, hit_moving_rim;
    Uint32 liquid_mask;
    struct rotating_hit rotation;
    const struct polygon *(*next)(struct terrain_cursor *);
    struct stride_contact *stride_contact;
};

enum wall_failure { WALL_UNRESOLVED, WALL_FULL, WALL_DEFER };

struct wall_planes {
    struct contact_plane *plane;
    int count, capacity;
    enum wall_failure failure;
};

static inline Bool wall_add_plane(struct wall_planes *planes,
                                  struct contact_plane contact)
{
    if (sweep_add_plane(planes->plane, &planes->count, planes->capacity,
                        contact)) {
        return TRUE;
    }
    planes->failure = WALL_FULL;
    return FALSE;
}

static inline struct wall_planes
wall_copy_planes(const struct wall_planes *planes, struct contact_plane *saved)
{
    for (int i = 0; i < planes->count; i++) {
        saved[i] = planes->plane[i];
    }
    return (struct wall_planes){saved, planes->count, planes->capacity};
}

static inline shz_vec3_t wall_relative(const struct wall_motion *motion,
                                       const struct polygon *tri)
{
    return shz_vec3_add(
        shz_vec3_sub(motion->origin, zam_load_vec3(tri->vertex)), motion->at);
}

LINKED_LAST(sweep_relative_bounds)
static __attribute__((noinline)) void
sweep_relative_bounds(struct sweep_bounds *box, const shz_vec3_t *origin,
                      shz_vec3_t at, shz_vec3_t move, Float radius)
{
    NJS_POINT3 start;
    NJS_VECTOR speed;
    zam_store_vec3(&start, shz_vec3_add(*origin, at));
    zam_store_vec3(&speed, move);
    sweep_extents(box, start, speed, radius);
}

static inline void wall_bounds(struct sweep_bounds *box,
                               const struct wall_motion *motion,
                               shz_vec3_t move, Float radius)
{
    sweep_relative_bounds(box, &motion->origin, motion->at, move, radius);
}

static inline Bool wall_blocks(const struct terrain_cursor *cursor,
                               const struct polygon *tri,
                               const struct impact_triangle *face,
                               shz_vec3_t from,
                               const struct wall_motion *motion)
{
    return cursor->ordinary && njAbs(face->normal.y) < motion->floor_y &&
           (!motion->stride || !face->normal.y) &&
           ((tri->attr & NJD_FLAG_DOUBLE_SIDE) ||
            plane_dot(face->normal, from) >= -motion->tolerance);
}

/* A float with magnitude at least 2^23 already has no fractional bits. */
static inline Float whole_coordinate(Float x)
{
    return njAbs(x) < 0x1p23F ? (Float)(int)x : x;
}

/* A face can span hundreds of units while two impacts are microunits
 * apart. Keep the sub-unit motion out of the large vertex subtraction. */
static inline Float sweep_height(shz_vec3_t whole, const struct polygon *tri,
                                 const struct impact_triangle *face,
                                 shz_vec3_t from, shz_vec3_t fine)
{
    shz_vec3_t n = face->normal;
    if ((!n.x && (!n.y || !n.z)) || (!n.y && !n.z)) {
        return plane_dot(n, from);
    }
    shz_vec3_t coarse = shz_vec3_sub(whole, zam_load_vec3(tri->vertex));
    return (plane_dot(coarse, face->cross) + plane_dot(fine, face->cross)) /
           face->cross_length;
}

/* Land from above on ordinary terrain. Coplanar neighbors still describe
 * the same sheet when a wall interrupts the remaining tangent motion. */
static inline Bool air_floor(const struct wall_motion *motion,
                             const struct terrain_cursor *cursor, shz_vec3_t up,
                             Float height)
{
    if (!motion->can_land || !cursor->ordinary || up.y < motion->floor_y ||
        height < 0.0F) {
        return FALSE;
    }
    return height >= motion->radius - motion->tolerance &&
           (motion->velocity.y < 0.0F || motion->land.face || motion->push.x ||
            motion->push.y || motion->push.z);
}

/* Outside the footprint, the top side of a walkable sheet can block the
 * sphere at its exposed edge without supplying ground under the player. */
static inline Bool air_floor_rim(const struct wall_motion *motion,
                                 const struct terrain_cursor *cursor,
                                 shz_vec3_t up, Float height)
{
    return !motion->stride && motion->can_land && cursor->ordinary &&
           up.y >= motion->floor_y && height >= -motion->tolerance;
}

/* The step probe can reach a finite floor from its upper side. The
 * projected path can also cross the footprint after falling below the lip. */
LINKED_LAST(air_foot_can_step)
static Bool air_foot_can_step(const struct impact_triangle *face,
                              shz_vec3_t from, shz_vec3_t move, shz_vec3_t up,
                              Float height, const struct wall_motion *motion)
{
    Float lo, hi;
    if (!ground_footprint(face, from, move, motion->tolerance, &lo, &hi)) {
        return FALSE;
    }
    Float delta = plane_dot(up, move);
    Float enter = height + delta * lo;
    Float leave = height + delta * hi;
    if (enter < -motion->tolerance && leave < -motion->tolerance) {
        /* Already embedded in the lip, the foot can still step onto it. */
        struct contact_plane contact;
        Float signed_height = up.y == face->normal.y ? height : -height;
        return height >= -motion->tolerance &&
               impact_contact_height(face, from, signed_height, motion->radius,
                                     motion->tolerance,
                                     &contact) == IMPACT_RIM &&
               contact.offset > motion->tolerance;
    }
    return enter <= motion->clearance || leave <= motion->clearance;
}

static Bool terrain_same_plane(const struct polygon *anchor, shz_vec3_t up,
                               const struct polygon *tri, Float tolerance)
{
    if (tri == anchor) {
        return TRUE;
    }
    shz_vec3_t origin = zam_load_vec3(anchor->vertex);
    for (int i = 0; i < 3; i++) {
        shz_vec3_t relative =
            shz_vec3_sub(zam_load_vec3(tri->vertex + i), origin);
        if (njAbs(plane_dot(up, relative)) > tolerance) {
            return FALSE;
        }
    }
    return TRUE;
}

static inline Bool landing_same_plane(const struct landing *land,
                                      const struct polygon *tri, shz_vec3_t up,
                                      Float tolerance)
{
    return plane_dot(up, land->up) >= GROUND_PARALLEL &&
           terrain_same_plane(land->face, land->up, tri, tolerance);
}

enum wall_event { WALL_CLEAR, WALL_HIT, WALL_POLICY };

/* Owned terrain keeps its landing and ride response. Its exposed edges
 * can block an airborne body before either the feet or a face meets it. */
static inline Bool owned_rims(const struct terrain_cursor *cursor,
                              const struct wall_motion *motion)
{
    const struct world_object *entry = &world_objects[cursor->next - 1];
    Uint32 allowed = WORLD_SOLID | WORLD_MOVES | WORLD_SIZE | WORLD_TURNED |
                     WORLD_GRIP_HIGH | WORLD_GRIP_LOW | WORLD_GRIP_LEAST;
    return !cursor->ordinary && !motion->stride && motion->can_land &&
           entry->owner && (entry->flags & WORLD_MOVES) &&
           !(entry->flags & ~allowed) &&
           (entry->owner->work->flags & WORLD_TESTED);
}

static Bool rotating_front(const struct world_object *entry);

/* Skip the whole published mesh when its owner supplies a moving cast.
 * The ordinary walker stays unchanged when the moving list is empty. */
static const struct polygon *fixed_triangle(struct terrain_cursor *cursor)
{
    if (cursor->left) {
        return terrain_next_triangle(cursor);
    }
    const struct polygon *tri;
    while ((tri = terrain_next_triangle(cursor))) {
        if (!rotating_front(&world_objects[cursor->next - 1])) {
            return tri;
        }
        cursor->left = 0;
    }
    return NULL;
}

/* The foot probe can reach a landing before the body hits a wall. Compare
 * both times on this segment; a slide starts a new search across the world. */
LINKED_LAST(wall_first_impact)
static __attribute__((noinline)) enum wall_event
wall_first_impact(struct wall_motion *motion, shz_vec3_t move, Float *fraction)
{
    struct sweep_bounds box;
    wall_bounds(&box, motion, move, motion->clearance);
    Float first = 1.0F;
    Float owner_probe = 2.0F;
    Bool snap = FALSE;
    enum wall_event event = WALL_CLEAR;
    shz_vec3_t fine =
        shz_vec3_add(shz_vec3_sub(motion->origin, motion->whole), motion->at);
    struct terrain_cursor cursor = {.arena = arena_held(),
                                    .liquid_mask = motion->liquid_mask};
    const struct polygon *tri;
    while ((tri = motion->next(&cursor)) != NULL) {
        struct impact_triangle face;
        if (sweep_box_misses(&box, tri->vertex) ||
            !impact_triangle_init(&face, tri->vertex)) {
            continue;
        }
        shz_vec3_t up = ground_face_normal(tri, &face);
        /* Tangent travel cannot hit the supporting plane again. The finite
         * extent of its faces still determines where support ends. */
        if (motion->land.owner && cursor.ordinary &&
            landing_same_plane(&motion->land, tri, up, motion->tolerance) &&
            plane_dot(up, move) >= -motion->tolerance) {
            continue;
        }
        shz_vec3_t from = wall_relative(motion, tri);
        Bool wall = wall_blocks(&cursor, tri, &face, from, motion);
        Float signed_height =
            sweep_height(motion->whole, tri, &face, from, fine);
        Float above = up.y == face.normal.y ? signed_height : -signed_height;
        /* A moving solid can lift the body off the floor. While its distance
         * from that floor increases, the wider foot probe cannot claim it. */
        if (motion->rotation.owner && cursor.ordinary &&
            up.y >= motion->floor_y &&
            above >= motion->radius - motion->tolerance &&
            plane_dot(up, move) >= 0) {
            continue;
        }
        Bool floor =
            !wall && !motion->stride && air_floor(motion, &cursor, up, above);
        Bool owned = owned_rims(&cursor, motion);
        if (owned) {
            /* With no physical rim hit, preserve the body's original
             * response wherever its larger probe would have reached. */
            Float time;
            if (!motion->owner_probe_hit && !motion->hit_moving_rim &&
                impact_time_height(&face, from, move, signed_height,
                                   motion->clearance, motion->tolerance, FALSE,
                                   first, &time) &&
                time < owner_probe) {
                owner_probe = time;
            }
            Bool front = (tri->attr & NJD_FLAG_DOUBLE_SIDE) ||
                         signed_height >= -motion->tolerance;
            if (!front) {
                continue;
            }
            /* Descending onto an object's footprint can attach a rider,
             * even when the sphere reaches a tooth along the way. */
            if (motion->velocity.y < 0.0F && up.y >= motion->floor_y &&
                above >= -motion->tolerance &&
                air_foot_can_step(&face, from, move, up, above, motion)) {
                first = 0.0F;
                event = WALL_POLICY;
                break;
            }
        }
        Bool blocking =
            owned || wall || floor ||
            (air_floor_rim(motion, &cursor, up, above) &&
             !air_foot_can_step(&face, from, move, up, above, motion));
        /* A physical hit takes precedence. When the feet finish just above
         * a floor, the character's short landing probe still owns the snap. */
        if (floor && motion->velocity.y < 0 && !motion->land.face && !snap &&
            event == WALL_CLEAR) {
            shz_vec3_t end = shz_vec3_add(from, move);
            Float height = above + plane_dot(up, move);
            snap = height > motion->radius + motion->tolerance &&
                   height <= motion->clearance &&
                   impact_inside(&face, end, motion->tolerance);
        }
        Float radius = blocking ? motion->radius : motion->clearance;
        Float time;
        if (impact_time_height(&face, from, move, signed_height, radius,
                               motion->tolerance, blocking, first, &time) &&
            (time < first || event == WALL_CLEAR || !blocking)) {
            event = blocking ? WALL_HIT : WALL_POLICY;
            first = time;
            if (!blocking && time == 0.0F) {
                break;
            }
        }
    }
    motion->owner_probe_hit |= owner_probe <= first;
    *fraction = first;
    return event == WALL_CLEAR && snap ? WALL_POLICY : event;
}

static Bool ground_prefers(const struct world_object *previous,
                           const struct world_object *best,
                           const struct world_object *candidate)
{
    if (!best) {
        return TRUE;
    }
    if (previous && (candidate->object == previous->object) !=
                        (best->object == previous->object)) {
        return candidate->object == previous->object;
    }
    if (candidate->object != best->object) {
        return (uintptr_t)candidate->object < (uintptr_t)best->object;
    }
    return (Uint32)candidate->flags < (Uint32)best->flags;
}

struct air_contacts {
    struct landing floor;
    Bool has_wall_rim, has_unclimbable_rim, rim_blocks, moving_rim;
    Float floor_offset;
};

/* Gather faces at the earliest hit before testing their rims. Floor and
 * moving rims constrain the velocity left after sliding along walls. */
LINKED_LAST(wall_gather)
static __attribute__((noinline)) Bool
wall_gather(const struct wall_motion *motion, const struct wall_planes *faces,
            struct wall_planes *out, struct air_contacts *hit,
            const shz_vec3_t *wall_velocity)
{
    struct sweep_bounds box;
    wall_bounds(&box, motion, (shz_vec3_t){0},
                motion->radius + motion->tolerance);
    shz_vec3_t fine =
        shz_vec3_add(shz_vec3_sub(motion->origin, motion->whole), motion->at);
    struct terrain_cursor cursor = {.arena = arena_held(),
                                    .liquid_mask = motion->liquid_mask};
    const struct polygon *tri;
    while ((tri = motion->next(&cursor)) != NULL) {
        struct impact_triangle face;
        if (sweep_box_misses(&box, tri->vertex) ||
            !impact_triangle_init(&face, tri->vertex)) {
            continue;
        }
        shz_vec3_t from = wall_relative(motion, tri);
        struct contact_plane contact;
        Float signed_height =
            sweep_height(motion->whole, tri, &face, from, fine);
        enum impact_feature feature =
            impact_contact_height(&face, from, signed_height, motion->radius,
                                  motion->tolerance, &contact);
        if (feature == IMPACT_NONE || (faces && feature != IMPACT_RIM)) {
            continue;
        }
        shz_vec3_t up = ground_face_normal(tri, &face);
        Float height = up.y == face.normal.y ? signed_height : -signed_height;
        if (feature == IMPACT_FACE &&
            (air_floor(motion, &cursor, up, height) ||
             (motion->initial && motion->can_land && cursor.ordinary &&
              up.y >= motion->floor_y && height >= 0.0F))) {
            if (!motion->initial &&
                njAbs(height - motion->radius) > motion->tolerance) {
                return FALSE;
            }
            const struct world_object *entry = &world_objects[cursor.next - 1];
            struct landing *land = &hit->floor;
            if (land->face &&
                !landing_same_plane(land, tri, up, motion->tolerance)) {
                return FALSE;
            }
            if (ground_prefers(NULL, land->source, entry)) {
                if (land == &hit->floor) {
                    hit->floor_offset = motion->radius - height;
                }
                *land = (struct landing){tri, entry, entry, up};
            }
            continue;
        }
        Bool owned = owned_rims(&cursor, motion);
        if (owned && !(tri->attr & NJD_FLAG_DOUBLE_SIDE) &&
            signed_height < -motion->tolerance) {
            continue;
        }
        if (owned && feature == IMPACT_FACE) {
            return FALSE;
        }
        Bool unclimbable_rim =
            feature == IMPACT_RIM &&
            (owned || air_floor_rim(motion, &cursor, up, height) ||
             (motion->stride && air_floor(motion, &cursor, up, height)));
        if (!unclimbable_rim &&
            !wall_blocks(&cursor, tri, &face, from, motion)) {
            return FALSE;
        }
        if (feature == IMPACT_RIM && !faces) {
            if (unclimbable_rim) {
                hit->has_unclimbable_rim = TRUE;
            } else {
                hit->has_wall_rim = TRUE;
            }
            continue;
        }
        if (faces) {
            if (unclimbable_rim != (wall_velocity != NULL)) {
                continue;
            }
            if (unclimbable_rim) {
                if (hit->floor.face && landing_same_plane(&hit->floor, tri, up,
                                                          motion->tolerance)) {
                    continue;
                }
                shz_vec3_t remaining =
                    shz_vec3_scale(motion->velocity, motion->remaining);
                if (!owned && air_foot_can_step(&face, from, remaining, up,
                                                height, motion)) {
                    return FALSE;
                }
                if (contact.offset > motion->tolerance ||
                    plane_dot(contact.normal, *wall_velocity) <
                        -motion->tolerance) {
                    hit->rim_blocks = TRUE;
                    hit->moving_rim |= owned;
                }
            } else if (wall_on_plane(&face, from, faces->plane, faces->count,
                                     motion->radius, motion->tolerance)) {
                continue;
            }
        }
        if ((motion->stride && feature == IMPACT_RIM) ||
            (!unclimbable_rim && njAbs(contact.normal.y) >= motion->floor_y) ||
            !wall_add_plane(out, contact)) {
            return FALSE;
        }
    }
    return TRUE;
}

/* A supported sphere can already touch the foot probe's larger envelope.
 * Check ordinary faces at the physical radius. A face met along the push
 * bounds the new position even when it was clear of the initial sphere. */
enum push_result { PUSH_CLEAR, PUSH_REFIT, PUSH_DELEGATE };

LINKED_LAST(wall_push_clear)
static __attribute__((noinline)) enum push_result
wall_push_clear(const struct wall_motion *motion, shz_vec3_t push,
                struct wall_planes *contacts)
{
    struct sweep_bounds box;
    wall_bounds(&box, motion, push, motion->clearance);
    struct terrain_cursor cursor = {.arena = arena_held(),
                                    .liquid_mask = motion->liquid_mask};
    enum push_result result = PUSH_CLEAR;
    shz_vec3_t fine =
        shz_vec3_add(shz_vec3_sub(motion->origin, motion->whole), motion->at);
    const struct polygon *tri;
    while ((tri = motion->next(&cursor)) != NULL) {
        if (sweep_box_misses(&box, tri->vertex)) {
            continue;
        }
        struct impact_triangle face;
        if (!impact_triangle_init(&face, tri->vertex)) {
            continue;
        }
        shz_vec3_t from = wall_relative(motion, tri);
        shz_vec3_t up = ground_face_normal(tri, &face);
        Float height = sweep_height(motion->whole, tri, &face, from, fine);
        Float above = up.y == face.normal.y ? height : -height;
        Bool ordinary =
            wall_blocks(&cursor, tri, &face, from, motion) ||
            (cursor.ordinary && motion->can_land && up.y >= motion->floor_y &&
             above >= -motion->tolerance);
        Float time;
        if (!ordinary) {
            if (impact_time_height(&face, from, push, height, motion->clearance,
                                   motion->tolerance, FALSE, 1.0F, &time)) {
                return PUSH_DELEGATE;
            }
            continue;
        }
        if (wall_on_plane(&face, from, contacts->plane, contacts->count,
                          motion->radius, motion->tolerance)) {
            continue;
        }
        if (!impact_time_height(&face, from, push, height, motion->radius,
                                motion->tolerance, TRUE, 1.0F, &time)) {
            continue;
        }
        struct contact_plane contact;
        shz_vec3_t travel = shz_vec3_scale(push, time);
        enum impact_feature feature =
            impact_contact_height(&face, shz_vec3_add(from, travel),
                                  height + plane_dot(face.normal, travel),
                                  motion->radius, motion->tolerance, &contact);
        if (feature == IMPACT_NONE) {
            return PUSH_DELEGATE;
        }
        contact.offset += plane_dot(contact.normal, travel);
        if (plane_dot(contact.normal, push) >=
            contact.offset - motion->tolerance) {
            continue;
        }
        if (feature != IMPACT_FACE || !wall_add_plane(contacts, contact)) {
            return PUSH_DELEGATE;
        }
        result = PUSH_REFIT;
    }
    return result;
}

static void landing_advance(struct wall_motion *motion, shz_vec3_t travel);

/* Refit the minimum translation if it would enter another face. No velocity
 * or support impulse comes from this displacement. The next sweep gathers
 * the contacts that actually remain at the corrected position. */
LINKED_LAST(wall_recover)
static __attribute__((noinline)) Bool wall_recover(struct wall_motion *motion,
                                                   struct wall_planes *contacts)
{
    /* Either side of a collected triangle can add a face during refitting. */
    int refits = 2 * near_polygon_count;
    for (int i = 0; i <= refits; i++) {
        shz_vec3_t push;
        if (!planes_project(contacts->plane, contacts->count, (shz_vec3_t){0},
                            motion->tolerance, &push) ||
            plane_dot(push, push) > 4.0F * motion->radius * motion->radius) {
            return FALSE;
        }
        enum push_result result = wall_push_clear(motion, push, contacts);
        if (result == PUSH_DELEGATE) {
            return FALSE;
        }
        if (result == PUSH_CLEAR) {
            if (motion->land.owner) {
                landing_advance(motion, push);
                if (!motion->land.owner) {
                    return FALSE;
                }
            }
            motion->at = shz_vec3_add(motion->at, push);
            motion->push = push;
            return TRUE;
        }
    }
    return FALSE;
}

/* A wall can remove part of the landing velocity too. Project the incoming
 * velocity once against both constraints, then check whether the ball
 * actually stays against the supporting face. */
LINKED_LAST(landing_clip)
static __attribute__((noinline, optimize("fp-contract=off"))) Bool
landing_clip(struct wall_motion *motion, struct wall_planes *walls,
             const struct landing *floor)
{
    shz_vec3_t wanted = motion->velocity;
    Float inward = plane_dot(floor->up, wanted);
    Bool touching = motion->land.owner != NULL;
    if (touching && !landing_same_plane(&motion->land, floor->face, floor->up,
                                        motion->tolerance)) {
        return FALSE;
    }
    Bool level = !floor->up.x && !floor->up.z;
    shz_vec3_t normal = motion->normal;
    Float strongest = -motion->tolerance;
    for (int i = 0; i < walls->count; i++) {
        struct contact_plane *wall = &walls->plane[i];
        if (wall->offset > motion->tolerance ||
            (motion->stride && (normal.x || normal.z) &&
             plane_dot(normal, wall->normal) < GROUND_PARALLEL)) {
            return FALSE;
        }
        if (wall->normal.y) {
            level = FALSE;
        }
        wall->offset = 0.0F;
        Float toward = plane_dot(wall->normal, wanted);
        if (toward < strongest) {
            normal = wall->normal;
            strongest = toward;
        }
    }
    shz_vec3_t allowed = wanted;
    if (level) {
        if (allowed.y < 0.0F) {
            allowed.y = 0.0F;
        }
    } else if (walls->count) {
        if (!wall_add_plane(walls, (struct contact_plane){floor->up, 0})) {
            return FALSE;
        }
    } else if (inward < 0.0F) {
        allowed = shz_vec3_sub(
            wanted, shz_vec3_scale(floor->up,
                                   inward / plane_dot(floor->up, floor->up)));
    }
    if (walls->count && !planes_project(walls->plane, walls->count, allowed,
                                        motion->tolerance, &allowed)) {
        return FALSE;
    }
    if (touching && motion->stride_contact &&
        -strongest > motion->stride_contact->inward &&
        strongest < -motion->tolerance) {
        *motion->stride_contact = (struct stride_contact){
            normal, allowed, -strongest, plane_dot(wanted, wanted)};
    }
    if (plane_dot(floor->up, allowed) > motion->tolerance) {
        motion->land.owner = NULL;
    } else if (!touching && wanted.y < 0.0F && inward < -motion->tolerance) {
        motion->land = *floor;
        motion->fall_speed = wanted.y;
        motion->inward = inward;
    }
    motion->normal = normal;
    motion->velocity = allowed;
    return TRUE;
}

LINKED_LAST(wall_clip_contacts)
static __attribute__((noinline, optimize("fp-contract=off"))) Bool
wall_clip_contacts(struct wall_motion *motion, Bool require_overlap,
                   struct wall_planes *contacts, struct contact_plane *saved)
{
    struct air_contacts hit = {0};
    if (!wall_gather(motion, NULL, contacts, &hit, NULL)) {
        goto gather_failed;
    }
    if (hit.has_wall_rim) {
        struct wall_planes faces = wall_copy_planes(contacts, saved);
        if (!wall_gather(motion, &faces, contacts, &hit, NULL)) {
            goto gather_failed;
        }
    }
    if (hit.has_unclimbable_rim) {
        struct wall_planes walls = wall_copy_planes(contacts, saved);
        shz_vec3_t velocity = motion->velocity;
        if (walls.count && !hit.floor.face) {
            /* Knuckles keeps his approach to a wall unless an unclimbable
             * rim also blocks his slide. */
            for (int i = 0; i < walls.count; i++) {
                walls.plane[i].offset = 0.0F;
            }
            if (!planes_project(walls.plane, walls.count, velocity,
                                motion->tolerance, &velocity)) {
                return FALSE;
            }
        }
        if (!wall_gather(motion, &walls, contacts, &hit, &velocity)) {
            goto gather_failed;
        }
    }
    Bool side_overlap = FALSE;
    int i;
    for (i = 0; i < contacts->count; i++) {
        side_overlap |= contacts->plane[i].offset > motion->tolerance;
    }
    if (side_overlap || hit.floor_offset > motion->tolerance) {
        /* A rising foot can enter a terrace from below. Its step response
         * still owns that move unless the body also overlaps an obstacle. */
        if (require_overlap && !side_overlap && motion->velocity.y >= 0.0F) {
            return FALSE;
        }
        if (!motion->initial ||
            (hit.floor.face &&
             !wall_add_plane(contacts, (struct contact_plane){
                                           hit.floor.up, hit.floor_offset}))) {
            return FALSE;
        }
        return wall_recover(motion, contacts);
    }
    if (require_overlap) {
        return FALSE;
    }
    if (hit.floor.face) {
        if (!landing_clip(motion, contacts, &hit.floor)) {
            return FALSE;
        }
        motion->hit_unclimbable_rim |= hit.rim_blocks;
        motion->hit_moving_rim |= hit.moving_rim;
        return TRUE;
    }
    if (!contacts->count || motion->land.owner) {
        return FALSE;
    }
    Float inward = __builtin_inff();
    shz_vec3_t normal = motion->normal;
    for (i = 0; i < contacts->count; i++) {
        struct contact_plane *plane = &contacts->plane[i];
        Float dot = plane_dot(plane->normal, motion->velocity);
        if (dot < inward) {
            normal = plane->normal;
            inward = dot;
        }
        plane->offset = 0.0F;
    }
    if (!planes_project(contacts->plane, contacts->count, motion->velocity,
                        motion->tolerance, &motion->velocity)) {
        return FALSE;
    }
    motion->normal = normal;
    motion->hit_unclimbable_rim |= hit.rim_blocks;
    motion->hit_moving_rim |= hit.moving_rim;
    return TRUE;

gather_failed:
    if (contacts->failure != WALL_FULL) {
        contacts->failure = WALL_DEFER;
    }
    return FALSE;
}

/* A bend can cross many separate faces within one tick. Only repeated
 * passes at the same time indicate a stalled sweep. The polygon count also
 * bounds the work when a path repeatedly revisits the same part of a mesh. */
#define TERRAIN_PROGRESS_INTERVAL 16

static inline Bool terrain_advances(int pass, Float remaining, Float *last)
{
    if (!pass || pass % TERRAIN_PROGRESS_INTERVAL) {
        return TRUE;
    }
    if (!(remaining < *last)) {
        return FALSE;
    }
    *last = remaining;
    return TRUE;
}

static inline void wall_stop(struct wall_motion *motion)
{
    motion->remaining = 0.0F;
    motion->stalled = TRUE;
    motion->velocity = (shz_vec3_t){0};
}

/* Most contacts fit on the stack. Only retry a full manifold when a new
 * boundary cannot fit. The floor and both sides of a triangle can also
 * bound an overlap correction after its initial contact. */
LINKED_LAST(wall_clip_impact)
static __attribute__((noinline)) Bool
wall_clip_impact(struct wall_motion *motion, Bool require_overlap)
{
    struct contact_plane local[2 * POLYGON_CONTACTS];
    struct wall_planes planes = {local, 0, POLYGON_CONTACTS};
    if (wall_clip_contacts(motion, require_overlap, &planes,
                           local + POLYGON_CONTACTS)) {
        return TRUE;
    }
    if (planes.failure != WALL_FULL) {
        return FALSE;
    }
    int limit = 3 * near_polygon_count + 1;
    int capacity = POLYGON_CONTACTS;
    while (capacity < limit) {
        capacity *= 2;
        if (capacity > limit) {
            capacity = limit;
        }
        struct contact_plane *space = syMalloc(2 * capacity * sizeof *space);
        if (!space) {
            break;
        }
        planes = (struct wall_planes){space, 0, capacity};
        Bool solved = wall_clip_contacts(motion, require_overlap, &planes,
                                         space + capacity);
        syFree(space);
        if (solved) {
            return TRUE;
        }
        if (planes.failure == WALL_DEFER) {
            return FALSE;
        }
        if (planes.failure != WALL_FULL) {
            break;
        }
    }
    /* A truncated manifold cannot certify more movement. The sweep has
     * already reached the first impact; retain that pose for this tick. */
    wall_stop(motion);
    return TRUE;
}

/* A touched triangle lies behind the radial contact plane. Over the
 * remaining time bound how far each of its vertices can cross that plane.
 * Translation is exact. Yaw's normal acceleration is -omega² dot(arm,n). */
LINKED_LAST(rotating_clip)
static Bool rotating_clip(struct wall_motion *motion,
                          const struct rotating_hit *hit)
{
    Float left = motion->remaining;
    shz_vec3_t n = hit->contact.normal;
    shz_vec3_t pivot =
        zam_load_vec3((const NJS_POINT3 *)hit->motion.object->pos);
    shz_vec3_t travel = zam_load_vec3(&hit->motion.travel);
    shz_vec3_t center = shz_vec3_sub(
        shz_vec3_add(shz_vec3_sub(motion->origin, pivot), motion->at),
        shz_vec3_scale(travel, hit->contact.time));
    struct yaw_path path = {.turn = hit->motion.turn};
    struct yaw_sample pose = yaw_at(&path, hit->contact.time);
    Float omega = path.turn / ANGLE_PER_RADIAN;
    Float normal_xz = yaw_length(n);
    Float required = plane_dot(n, hit->contact.velocity);
    for (int i = 0; left > 0 && i < 3; i++) {
        shz_vec3_t arm =
            yaw_turn(shz_vec3_sub(zam_load_vec3(hit->tri->vertex + i), pivot),
                     pose.sine, pose.cosine);
        Float clearance =
            plane_dot(n, shz_vec3_sub(center, arm)) - motion->radius;
        Float projected = n.x * arm.x + n.z * arm.z;
        Float max_accel =
            omega * omega *
            (njAbs(omega) * left * yaw_length(arm) * normal_xz - projected);
        Float speed =
            plane_dot(n, travel) + omega * (n.x * arm.z - n.z * arm.x);
        Float bound = speed - clearance / left +
                      (max_accel > 0 ? 0.5F * max_accel * left : 0.0F);
        if (bound > required) {
            required = bound;
        }
    }
    struct contact_plane planes[POLYGON_CONTACTS];
    int count = 1;
    planes[0] = (struct contact_plane){n, required + motion->tolerance};
    struct terrain_cursor cursor = {.arena = arena_held()};
    struct sweep_bounds box;
    wall_bounds(&box, motion, (shz_vec3_t){0},
                motion->radius + motion->tolerance);
    const struct polygon *tri;
    while ((tri = terrain_next_triangle(&cursor))) {
        struct impact_triangle face;
        if (!cursor.ordinary || sweep_box_misses(&box, tri->vertex) ||
            !impact_triangle_init(&face, tri->vertex)) {
            continue;
        }
        shz_vec3_t from = wall_relative(motion, tri);
        Float height = plane_dot(face.normal, from);
        if (height < -motion->tolerance &&
            !(tri->attr & NJD_FLAG_DOUBLE_SIDE)) {
            continue;
        }
        struct contact_plane contact;
        if (impact_contact_height(&face, from, height, motion->radius,
                                  motion->tolerance, &contact) == IMPACT_NONE) {
            continue;
        }
        contact.offset = 0;
        if (!sweep_add_plane(planes, &count, POLYGON_CONTACTS, contact)) {
            return FALSE;
        }
    }
    shz_vec3_t incoming = shz_vec3_sub(motion->velocity, hit->contact.velocity);
    if (!planes_project(planes, count, motion->velocity, motion->tolerance,
                        &motion->velocity)) {
        return FALSE;
    }
    motion->rotation = *hit;
    if (hit->contact.feature == IMPACT_FACE && n.y >= motion->floor_y &&
        incoming.y <= 0) {
        motion->inward = plane_dot(incoming, n);
        motion->fall_speed = incoming.y;
        shz_vec3_t relative = shz_vec3_sub(motion->velocity, travel);
        Float acceleration =
            njAbs(omega) * (yaw_length(center) + left * yaw_length(relative)) *
                njAbs(omega) +
            2 * njAbs(omega) * yaw_length(relative);
        Float separating =
            plane_dot(n, shz_vec3_sub(motion->velocity, hit->contact.velocity));
        motion->rotation.support_clearance =
            2 * motion->tolerance +
            left * (separating + 0.5F * left * acceleration);
    }
    if (motion->land.owner &&
        plane_dot(motion->velocity, motion->land.up) > motion->tolerance) {
        motion->land = (struct landing){0};
        motion->stride = FALSE;
    }
    motion->normal = n;
    motion->hit_unclimbable_rim = motion->hit_moving_rim = TRUE;
    return TRUE;
}

/* The response plane encloses all three moving corners. It can leave a
 * small gap above the face. Test that final approach against the other
 * faces in their end poses before putting the feet back on the blade. */
static Bool rotating_snap_clear(const struct wall_motion *motion,
                                const struct polygon_yaw *pose,
                                shz_vec3_t center, shz_vec3_t shift)
{
    shz_vec3_t pivot = zam_load_vec3((const NJS_POINT3 *)pose->object->pos);
    struct yaw_path path = {.turn = pose->turn,
                            .origin =
                                shz_vec3_sub(shz_vec3_sub(center, pivot),
                                             zam_load_vec3(&pose->travel))};
    if (yaw_misses(pose, path.origin, shift,
                   motion->radius + motion->tolerance)) {
        return TRUE;
    }
    const struct collision_mesh *mesh =
        terrain_mesh(arena_held(), pose->object);
    if (!mesh || mesh->out_of_polygons) {
        return FALSE;
    }
    if (!mesh->polygons) {
        return TRUE;
    }
    struct yaw_sample end = yaw_at(&path, 1);
    shz_vec3_t move = yaw_turn(shift, -end.sine, end.cosine);
    struct sweep_bounds box;
    NJS_VECTOR delta;
    zam_store_vec3(&delta, move);
    sweep_relative_bounds(&box, &pivot, end.center, move,
                          motion->radius + motion->tolerance);
    Float limit = 1.0F - motion->tolerance / njScalor(&delta);
    for (const struct polygon *tri = mesh->polygons; tri != mesh->last->next;
         tri = tri->next) {
        if (tri == motion->rotation.tri ||
            sweep_box_misses(&box, tri->vertex)) {
            continue;
        }
        struct impact_triangle face;
        if (!impact_triangle_init(&face, tri->vertex)) {
            continue;
        }
        shz_vec3_t from = yaw_from(
            &path, &end, shz_vec3_sub(zam_load_vec3(tri->vertex), pivot));
        Float height = plane_dot(face.normal, from);
        if (!(tri->attr & NJD_FLAG_DOUBLE_SIDE) &&
            height < -motion->tolerance) {
            continue;
        }
        Float time;
        if (impact_time_height(&face, from, move, height, motion->radius,
                               motion->tolerance, TRUE, limit, &time)) {
            return FALSE;
        }
    }
    return TRUE;
}

/* A rim can deflect the body. Only a walkable face under its final
 * footprint can become the next tick's carrier. */
static Bool rotating_support(struct wall_motion *motion, shz_vec3_t *up)
{
    const struct rotating_hit *hit = &motion->rotation;
    if (!hit->owner || !hit->support_clearance || motion->stalled ||
        !motion->can_land || hit->contact.feature != IMPACT_FACE ||
        hit->contact.normal.y < motion->floor_y) {
        return FALSE;
    }
    shz_vec3_t pivot =
        zam_load_vec3((const NJS_POINT3 *)hit->motion.object->pos);
    shz_vec3_t center =
        shz_vec3_add(shz_vec3_sub(motion->origin, pivot), motion->at);
    struct yaw_path path = {
        .turn = hit->motion.turn,
        .origin = shz_vec3_sub(center, zam_load_vec3(&hit->motion.travel))};
    struct yaw_sample pose = yaw_at(&path, 1);
    struct impact_triangle face;
    if (!impact_triangle_init(&face, hit->tri->vertex)) {
        return FALSE;
    }
    shz_vec3_t from = yaw_from(
        &path, &pose, shz_vec3_sub(zam_load_vec3(hit->tri->vertex), pivot));
    shz_vec3_t normal = ground_face_normal(hit->tri, &face);
    Float gap = plane_dot(normal, from) - motion->radius;
    if (gap < -2 * motion->tolerance || gap > hit->support_clearance ||
        gap > motion->radius || impact_outside(&face, from)) {
        return FALSE;
    }
    normal = yaw_turn(normal, pose.sine, pose.cosine);
    if (gap > 2 * motion->tolerance) {
        shz_vec3_t shift = shz_vec3_scale(normal, -gap);
        shz_vec3_t world = shz_vec3_add(motion->origin, motion->at);
        for (int i = 0; i < world_count; i++) {
            const struct world_object *entry = &world_objects[i];
            if (rotating_front(entry) ||
                (entry->owner && !(entry->owner->work->flags & WORLD_TESTED))) {
                continue;
            }
            struct polygon_yaw fixed = {.object = entry->object};
            if (!rotating_snap_clear(motion, &fixed, world, shift)) {
                return FALSE;
            }
        }
        for (struct task *owner = polygon_rotating; owner;) {
            struct task_forces *forces = (struct task_forces *)owner->force;
            struct polygon_yaw moving;
            forces->rotation->pose(owner, &moving);
            if ((owner->work->flags & WORLD_TESTED) && moving.object &&
                !rotating_snap_clear(motion, &moving, world, shift)) {
                return FALSE;
            }
            owner = forces->next_rotating;
        }
        motion->at = shz_vec3_add(motion->at, shift);
    }
    *up = normal;
    return TRUE;
}

static Bool rotating_front(const struct world_object *entry)
{
    if (!entry->owner) {
        return FALSE;
    }
    for (struct task *task = polygon_rotating; task;) {
        struct task_forces *forces = (struct task_forces *)task->force;
        if (entry->owner == task) {
            struct polygon_yaw pose;
            forces->rotation->pose(task, &pose);
            return pose.object == entry->object &&
                   (pose.turn || pose.travel.x || pose.travel.y ||
                    pose.travel.z);
        }
        task = forces->next_rotating;
    }
    return FALSE;
}

enum wall_result { WALL_SOLVED, WALL_DELEGATE, WALL_WIDEN };

LINKED_LAST(wall_travel)
static __attribute__((noinline, optimize("fp-contract=off"))) enum wall_result
wall_travel(struct wall_motion *motion, struct polygon_ball *collected)
{
    motion->next = polygon_rotating ? fixed_triangle : terrain_next_triangle;
    Float reach =
        njScalor(&collected->speed) + (collected->radius - motion->clearance);
    shz_vec3_t origin =
        shz_vec3_sub(motion->origin, zam_load_vec3(&collected->pos));
    if (reach < 0.0F || plane_dot(origin, origin) > reach * reach) {
        return WALL_WIDEN;
    }
    Float last = motion->remaining;
    const int limit = TERRAIN_PROGRESS_INTERVAL + 2 * near_polygon_count;
    for (int hit = 0;
         hit < limit && terrain_advances(hit, motion->remaining, &last);
         hit++) {
        Float time;
        shz_vec3_t move = shz_vec3_scale(motion->velocity, motion->remaining);
        motion->initial = hit == 0;
        enum wall_event event = wall_first_impact(motion, move, &time);
        struct rotating_hit rotor;
        enum yaw_result rotating = YAW_CLEAR;
        if (polygon_rotating && motion->remaining > 0) {
            Float start = 1.0F - motion->remaining;
            rotating = rotating_first(shz_vec3_add(motion->origin, motion->at),
                                      motion->velocity, motion->radius, start,
                                      start + time * motion->remaining, &rotor);
            if (rotating != YAW_CLEAR) {
                time = (rotor.contact.time - start) / motion->remaining;
                if (time > 1) {
                    time = 1;
                }
                if (time < 0) {
                    time = 0;
                }
            }
        }
        motion->initial &= time == 0.0F;
        shz_vec3_t travel = shz_vec3_scale(move, time);
        shz_vec3_t next = shz_vec3_add(motion->at, travel);
        shz_vec3_t extent = shz_vec3_add(origin, next);
        if (plane_dot(extent, extent) > reach * reach) {
            return WALL_WIDEN;
        }
        if (motion->land.owner) {
            landing_advance(motion, travel);
            if (motion->stride && !motion->land.owner) {
                return WALL_DELEGATE;
            }
        }
        motion->at = next;
        if (rotating != YAW_CLEAR) {
            motion->remaining *= 1.0F - time;
            if (rotating == YAW_UNCERTAIN || !rotating_clip(motion, &rotor)) {
                motion->hit_moving_rim = TRUE;
                wall_stop(motion);
                return WALL_SOLVED;
            }
            if (motion->remaining == 0) {
                return WALL_SOLVED;
            }
            continue;
        }
        if (event == WALL_POLICY && !motion->initial) {
            return WALL_DELEGATE;
        }
        if (event == WALL_CLEAR) {
            return WALL_SOLVED;
        }
        motion->remaining *= 1.0F - time;
        /* At the original pose a sole foot-probe hit may coincide with an
         * actual body overlap. Only a successful positional repair may
         * restart the query from that pose. */
        shz_vec3_t at_contact = motion->at;
        if (!wall_clip_impact(motion, event == WALL_POLICY)) {
            return WALL_DELEGATE;
        }
        extent = shz_vec3_add(origin, motion->at);
        if (plane_dot(extent, extent) > reach * reach) {
            motion->push = shz_vec3_sub(motion->push,
                                        shz_vec3_sub(motion->at, at_contact));
            motion->at = at_contact;
            return WALL_WIDEN;
        }
        if (motion->remaining == 0.0F) {
            return WALL_SOLVED;
        }
    }
    wall_stop(motion);
    return WALL_SOLVED;
}

LINKED_LAST(polygon_slide_air)
Bool polygon_slide_air(const struct polygon_ball *ball,
                       struct polygon_slide *slide,
                       struct polygon_ball *collected, Float clearance,
                       Float floor_y, Bool can_land)
{
    if (!(ball->radius > 0.0F && clearance >= ball->radius) ||
        world_count == WORLD_OBJECTS) {
        return FALSE;
    }
    struct wall_motion motion = {
        .origin = zam_load_vec3(&ball->pos),
        .velocity = zam_load_vec3(&ball->speed),
        .remaining = 1.0F,
        .can_land = can_land,
        .whole = {.x = whole_coordinate(ball->pos.x),
                  .y = whole_coordinate(ball->pos.y),
                  .z = whole_coordinate(ball->pos.z)},
        .radius = ball->radius,
        .clearance = clearance,
        .floor_y = floor_y,
        .tolerance = impact_tolerance(ball->radius, ball->speed)};
    enum wall_result result = wall_travel(&motion, collected);
    Bool widened = result == WALL_WIDEN;
    if (widened) {
        struct polygon_ball region = {.pos = ball->pos,
                                      .speed = ball->speed,
                                      .radius = 2.0F * ball->radius +
                                                clearance + motion.tolerance};
        polygon_collect_near(&region);
        motion.at = motion.push = motion.normal = (shz_vec3_t){0};
        motion.land = (struct landing){0};
        motion.fall_speed = 0;
        motion.hit_unclimbable_rim = motion.stalled = FALSE;
        motion.owner_probe_hit = motion.hit_moving_rim = FALSE;
        motion.rotation = (struct rotating_hit){0};
        motion.velocity = zam_load_vec3(&ball->speed);
        motion.remaining = 1.0F;
        result = world_count < WORLD_OBJECTS ? wall_travel(&motion, &region)
                                             : WALL_DELEGATE;
    }
    if (result == WALL_WIDEN && motion.rotation.owner) {
        wall_stop(&motion);
        result = WALL_SOLVED;
    }
    if (!motion.stalled && motion.owner_probe_hit && !motion.hit_moving_rim) {
        result = WALL_DELEGATE;
    }
    if (result == WALL_SOLVED) {
        slide->owner = NULL;
        slide->support = motion.land.owner ? motion.land.owner->object : NULL;
        slide->flags = motion.land.owner ? motion.land.owner->flags : 0;
        slide->fall_speed = motion.fall_speed;
        slide->hit_unclimbable_rim = motion.hit_unclimbable_rim;
        slide->stalled = motion.stalled;
        if (motion.land.owner) {
            zam_store_vec3(&slide->up, motion.land.up);
            zam_store_vec3(&slide->normal_speed,
                           shz_vec3_scale(motion.land.up, motion.inward));
        }
        shz_vec3_t up;
        if (rotating_support(&motion, &up)) {
            slide->owner = motion.rotation.owner;
            slide->support = motion.rotation.motion.object;
            slide->flags = motion.rotation.flags;
            zam_store_vec3(&slide->up, up);
            zam_store_vec3(&slide->normal_speed,
                           shz_vec3_scale(up, motion.inward));
        }
        zam_store_vec3(&slide->correction, motion.push);
        zam_store_vec3(&slide->travel, shz_vec3_sub(motion.at, motion.push));
        zam_store_vec3(&slide->velocity, motion.velocity);
        zam_store_vec3(&slide->normal, motion.normal);
    }
    if (widened) {
        polygon_collect_near(collected);
    }
    return result == WALL_SOLVED;
}

/* A private scan leaves both the world's current object list and the
 * player's collected triangles available to the rest of the movement tick. */
struct carry_span {
    const struct polygon *first, *end;
};

/* Most slides stay inside the first swept box. Remember its triangles so
 * a contact or crease does not walk the entire mesh again. */
#define CARRY_CACHED 32
struct carry_cache {
    struct sweep_bounds box;
    const struct polygon *triangles[CARRY_CACHED + 1];
};

struct carry_path {
    const struct carry_cache *cache;
    const struct carry_span *spans;
    shz_vec3_t origin, whole, at;
    Float radius, epsilon;
};

struct carry_scan {
    const struct polygon *const *cached;
    const struct carry_span *span;
    const struct polygon *next, *end;
    struct sweep_bounds box;
    shz_vec3_t at, fine;
};

LINKED_LAST(carry_begin)
static __attribute__((noinline)) void carry_begin(struct carry_scan *scan,
                                                  const struct carry_path *path,
                                                  shz_vec3_t at,
                                                  shz_vec3_t move)
{
    sweep_relative_bounds(&scan->box, &path->origin, at, move,
                          path->radius + path->epsilon);
    scan->at = at;
    scan->fine = shz_vec3_add(shz_vec3_sub(path->origin, path->whole), at);
    const struct carry_cache *cache = path->cache;
    scan->cached = NULL;
    if (cache && scan->box.low.x >= cache->box.low.x &&
        scan->box.low.y >= cache->box.low.y &&
        scan->box.low.z >= cache->box.low.z &&
        scan->box.high.x <= cache->box.high.x &&
        scan->box.high.y <= cache->box.high.y &&
        scan->box.high.z <= cache->box.high.z) {
        scan->cached = cache->triangles;
    }
    scan->span = path->spans;
    scan->next = scan->end = NULL;
}

LINKED_LAST(carry_next)
static const struct polygon *carry_next(struct carry_scan *scan)
{
    if (scan->cached) {
        const struct polygon *tri;
        while ((tri = *scan->cached++)) {
            if (!sweep_box_misses(&scan->box, tri->vertex)) {
                return tri;
            }
        }
        return NULL;
    }
    for (;;) {
        while (scan->next == scan->end) {
            if (!scan->span->first) {
                return NULL;
            }
            scan->next = scan->span->first;
            scan->end = scan->span++->end;
        }
        const struct polygon *tri = scan->next;
        scan->next = tri->next;
        if (!sweep_box_misses(&scan->box, tri->vertex)) {
            return tri;
        }
    }
}

struct carry_face {
    struct impact_triangle face;
    shz_vec3_t from;
    Float height;
};

LINKED_LAST(carry_face)
static __attribute__((noinline)) Bool carry_face(const struct carry_path *path,
                                                 const struct carry_scan *scan,
                                                 const struct polygon *tri,
                                                 struct carry_face *f)
{
    if (!impact_triangle_init(&f->face, tri->vertex)) {
        return FALSE;
    }
    f->from = shz_vec3_add(
        shz_vec3_sub(path->origin, zam_load_vec3(tri->vertex)), scan->at);
    f->height = sweep_height(path->whole, tri, &f->face, f->from, scan->fine);
    return TRUE;
}

static inline Bool carry_front(const struct polygon *tri, Float height,
                               Float epsilon)
{
    return (tri->attr & NJD_FLAG_DOUBLE_SIDE) || height >= -epsilon;
}

/* A sphere already overlapping a sheet can reach the next triangle's edge
 * while moving along the sheet. Only an exposed rim blocks that motion. */
LINKED_LAST(carry_covered)
static __attribute__((noinline)) Bool
carry_covered(const struct carry_path *path, const struct polygon *rim,
              shz_vec3_t normal, shz_vec3_t at)
{
    struct carry_scan scan;
    carry_begin(&scan, path, at, (shz_vec3_t){0});
    const struct polygon *tri;
    while ((tri = carry_next(&scan))) {
        struct carry_face f;
        if (tri != rim && terrain_same_plane(rim, normal, tri, path->epsilon) &&
            carry_face(path, &scan, tri, &f) &&
            carry_front(tri, f.height, path->epsilon) &&
            njAbs(plane_dot(f.face.normal, normal)) >= GROUND_PARALLEL &&
            impact_inside(&f.face, f.from, path->epsilon)) {
            return TRUE;
        }
    }
    return FALSE;
}

LINKED_LAST(carry_first)
static __attribute__((noinline, optimize("fp-contract=off"))) Bool
carry_first(struct carry_path *path, shz_vec3_t move, Float *time,
            struct carry_cache *cache)
{
    struct carry_scan scan;
    carry_begin(&scan, path, path->at, move);
    const struct polygon *tri;
    Bool hit = FALSE;
    int used = 0;
    while ((tri = carry_next(&scan))) {
        if (cache) {
            if (used == CARRY_CACHED) {
                cache = NULL;
            } else {
                cache->triangles[used++] = tri;
            }
        }
        struct carry_face f;
        Float t;
        if (!carry_face(path, &scan, tri, &f) ||
            !impact_time_height(&f.face, f.from, move, f.height, path->radius,
                                path->epsilon, TRUE, *time, &t)) {
            continue;
        }
        shz_vec3_t step = shz_vec3_scale(move, t);
        Float height = f.height + plane_dot(f.face.normal, step);
        if (!carry_front(tri, height, path->epsilon)) {
            continue;
        }
        if (njAbs(height) < path->radius - path->epsilon) {
            struct contact_plane plane;
            enum impact_feature feature = impact_contact_height(
                &f.face, shz_vec3_add(f.from, step), height, path->radius,
                path->epsilon, &plane);
            if (feature && ((t == 0.0F &&
                             plane_dot(plane.normal, move) >= -path->epsilon) ||
                            (feature == IMPACT_RIM &&
                             carry_covered(path, tri, f.face.normal,
                                           shz_vec3_add(path->at, step))))) {
                continue;
            }
        }
        *time = t;
        hit = TRUE;
    }
    if (cache) {
        cache->box = scan.box;
        cache->triangles[used] = NULL;
        path->cache = cache;
    }
    return hit;
}

/* If the stack fills, each remaining triangle can contribute at most one
 * more constraint. Count the unvisited candidates before reserving space. */
LINKED_LAST(carry_planes)
static __attribute__((noinline)) int carry_planes(const struct carry_path *path,
                                                  struct wall_planes *planes)
{
    struct carry_scan scan;
    carry_begin(&scan, path, path->at, (shz_vec3_t){0});
    const struct polygon *tri;
    while ((tri = carry_next(&scan))) {
        struct carry_face f;
        if (!carry_face(path, &scan, tri, &f) ||
            !carry_front(tri, f.height, path->epsilon)) {
            continue;
        }
        struct contact_plane plane;
        enum impact_feature feature = impact_contact_height(
            &f.face, f.from, f.height, path->radius, path->epsilon, &plane);
        if (feature == IMPACT_NONE ||
            (feature == IMPACT_RIM &&
             carry_covered(path, tri, f.face.normal, path->at))) {
            continue;
        }
        plane.offset = 0.0F;
        if (!wall_add_plane(planes, plane)) {
            int required = planes->count + 1;
            while (carry_next(&scan)) {
                required++;
            }
            return required;
        }
    }
    return planes->count;
}

LINKED_LAST(carry_clip)
static __attribute__((noinline)) Bool carry_clip(const struct carry_path *path,
                                                 shz_vec3_t *velocity)
{
    struct contact_plane local[POLYGON_CONTACTS];
    struct wall_planes planes = {local, 0, POLYGON_CONTACTS};
    int required = carry_planes(path, &planes);
    if (required <= planes.capacity) {
        return required && planes_project(local, required, *velocity,
                                          path->epsilon, velocity);
    }
    struct contact_plane *space = syMalloc(required * sizeof *space);
    if (!space) {
        return FALSE;
    }
    planes = (struct wall_planes){space, 0, required};
    int count = carry_planes(path, &planes);
    Bool solved =
        count && count <= required &&
        planes_project(space, count, *velocity, path->epsilon, velocity);
    syFree(space);
    return solved;
}

LINKED_LAST(carry_slide)
static __attribute__((noinline, optimize("fp-contract=off"))) Bool
carry_slide(struct carry_path *path, shz_vec3_t velocity)
{
    struct carry_cache cache;
    Float remaining = 1.0F;
    Float last = remaining;
    for (int i = 0; i < MESH_POLYGONS && terrain_advances(i, remaining, &last);
         i++) {
        Float time = 1.0F;
        shz_vec3_t move = shz_vec3_scale(velocity, remaining);
        Bool hit = carry_first(path, move, &time, i ? NULL : &cache);
        path->at = shz_vec3_add(path->at, shz_vec3_scale(move, time));
        if (!hit || time == 1.0F) {
            return TRUE;
        }
        if (!carry_clip(path, &velocity)) {
            return FALSE;
        }
        if (!velocity.x && !velocity.y && !velocity.z) {
            return TRUE;
        }
        remaining *= 1.0F - time;
    }
    return FALSE;
}

/* Projection against stationary surfaces cannot increase the length of
 * the requested carry. Its sphere of reach therefore contains every slide. */
LINKED_LAST(polygon_limit_carry)
Bool polygon_limit_carry(const NJS_POINT3 *center, Float radius,
                         NJS_VECTOR *travel)
{
    const struct land_table *land = world_collision_land();
    if (!(radius > 0.0F)) {
        *travel = (NJS_VECTOR){0};
        return FALSE;
    }
    if (!land || land->count <= 0) {
        return TRUE;
    }
    Float epsilon = impact_tolerance(radius, *travel);
    Float reach = (njScalor(travel) + radius + epsilon) * (1.0F + 0x1p-20F);
    shz_vec3_t start = zam_load_vec3(center);
    const int capacity = land->count < MESHES ? land->count : MESHES;
    struct carry_span spans[capacity + 1];
    int used = 0;
    struct box_arena *arena = arena_held();
    for (int i = 0; i < land->count; i++) {
        const struct land_entry *entry = &land->entries[i];
        /* A wall still blocks a rider when its material changes the
         * character's own response. Liquid and moving entries keep theirs. */
        if ((entry->flags & (WORLD_SOLID | WORLD_LIQUID | WORLD_MOVES)) !=
            WORLD_SOLID) {
            continue;
        }
        shz_vec3_t d = shz_vec3_sub(start, zam_load_vec3(&entry->center));
        Float bound = reach + entry->radius;
        if (plane_dot(d, d) > bound * bound * (1.0F + 0x1p-10F)) {
            continue;
        }
        const struct collision_mesh *mesh = terrain_mesh(arena, entry->object);
        if (!mesh || mesh->out_of_polygons) {
            *travel = (NJS_VECTOR){0};
            return FALSE;
        }
        if (!mesh->polygons) {
            continue;
        }
        const struct polygon *first = mesh->polygons, *end = mesh->last->next;
        if (mesh->bucketed) {
            Float pad =
                (njAbs(center->z) + mesh->span + reach + 1.0F) * 0x1p-12F;
            Float low = center->z - mesh->span - reach - pad;
            Float high = near_bucket_end(
                mesh, center->z + mesh->span + reach + pad, FALSE);
            while (first != end && first->center.z < low) {
                first = first->next;
            }
            const struct polygon *stop = first;
            while (stop != end && stop->center.z <= high) {
                stop = stop->next;
            }
            end = stop;
        }
        if (first == end) {
            continue;
        }
        /* Several land entries can name the same object. Each distinct
         * nonempty span belongs to one of the pool's MESHES records. */
        int j = 0;
        while (j < used && spans[j].first != first) {
            j++;
        }
        if (j == used) {
            spans[used++] = (struct carry_span){first, end};
        }
    }
    if (used) {
        spans[used] = (struct carry_span){0};
        struct carry_path path = {.spans = spans,
                                  .origin = start,
                                  .whole = {.x = whole_coordinate(start.x),
                                            .y = whole_coordinate(start.y),
                                            .z = whole_coordinate(start.z)},
                                  .radius = radius,
                                  .epsilon = epsilon};
        Bool verified = carry_slide(&path, zam_load_vec3(travel));
        zam_store_vec3(travel, path.at);
        return verified;
    }
    return TRUE;
}

/* Floors and the wall under Knuckles' hands allow the climb to continue.
 * An unclimbable object stops the probe without nudging his feet. */
enum climb_surface { CLIMB_CLEAR, CLIMB_BLOCK, CLIMB_STOP };

static inline enum climb_surface climb_surface(Uint32 flags, shz_vec3_t normal,
                                               shz_vec3_t up)
{
    if (flags & (WORLD_UNCLIMBABLE | WORLD_DEFLECT)) {
        return CLIMB_STOP;
    }
    if (normal.y < -0.86F ||
        (normal.y < 0.86F && plane_dot(up, normal) <= 0.259F)) {
        return CLIMB_BLOCK;
    }
    return CLIMB_CLEAR;
}

struct climb_hit {
    enum climb_surface kind;
    Float time, distance;
    shz_vec3_t normal;
};

static inline Bool climb_before(struct climb_hit a, struct climb_hit b)
{
    if (a.kind != b.kind) {
        return a.kind > b.kind;
    }
    if (a.time != b.time) {
        return a.time < b.time;
    }
    if (a.distance != b.distance) {
        return a.distance < b.distance;
    }
    return normal_before(a.normal, b.normal);
}

/* A rim inside a flat sheet cannot block a climb. A neighbor covers the
 * rim if it contains the closest point and extends toward the sphere's
 * projection. The segment to that vertex then starts strictly closer. */
LINKED_LAST(climb_internal_rim)
static __attribute__((noinline, optimize("fp-contract=off"))) Bool
climb_internal_rim(const struct polygon *tri, const struct world_object *entry,
                   shz_vec3_t normal, shz_vec3_t from, shz_vec3_t radial,
                   Float tol)
{
    shz_vec3_t toward =
        shz_vec3_sub(radial, shz_vec3_scale(normal, plane_dot(normal, radial)));
    Float length = njAbs(toward.x) + njAbs(toward.y) + njAbs(toward.z);
    if (length <= tol) {
        return FALSE;
    }
    shz_vec3_t rim = shz_vec3_sub(from, radial);
    shz_vec3_t origin = zam_load_vec3(tri->vertex);
    NJS_POINT3 point;
    zam_store_vec3(&point, shz_vec3_add(origin, rim));
    struct sweep_bounds box;
    sweep_sphere_bounds(&box, point, tol);
    struct terrain_cursor cursor = {.arena = arena_held()};
    const struct polygon *other;
    while ((other = terrain_next_triangle(&cursor))) {
        if (other == tri || sweep_box_misses(&box, other->vertex)) {
            continue;
        }
        const struct world_object *owner = &world_objects[cursor.next - 1];
        if (entry != owner && (entry->owner || owner->owner ||
                               ((entry->flags | owner->flags) & WORLD_MOVES) ||
                               ((entry->flags ^ owner->flags) &
                                (WORLD_UNCLIMBABLE | WORLD_DEFLECT)))) {
            continue;
        }
        struct impact_triangle next;
        if (((tri->attr ^ other->attr) & NJD_FLAG_DOUBLE_SIDE) ||
            !terrain_same_plane(tri, normal, other, tol) ||
            !impact_triangle_init(&next, other->vertex)) {
            continue;
        }
        Float facing = plane_dot(normal, next.normal);
        if ((tri->attr & NJD_FLAG_DOUBLE_SIDE ? njAbs(facing) : facing) <
            GROUND_PARALLEL) {
            continue;
        }
        shz_vec3_t q = shz_vec3_add(
            shz_vec3_sub(origin, zam_load_vec3(other->vertex)), rim);
        if (!impact_inside(&next, q, tol)) {
            continue;
        }
        for (int i = 0; i < 3; i++) {
            shz_vec3_t direction = shz_vec3_sub(next.v[i], q);
            Float error = tol * (length + njAbs(direction.x) +
                                 njAbs(direction.y) + njAbs(direction.z));
            if (plane_dot(toward, direction) > error) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

struct climb_space {
    struct sweep_bounds box;
    shz_vec3_t center, up;
    Float reach, tolerance;
};

struct climb_contact {
    struct contact_plane plane;
    const struct polygon *rim;
    const struct world_object *entry;
    Float distance;
};

/* The plane at a triangle's nearest point separates the entire triangle.
 * Its offset is the clearance needed by an overlapping face or the gap an
 * outside sphere may spend before touching it. */
LINKED_LAST(climb_plane)
static __attribute__((noinline, optimize("fp-contract=off"))) Bool
climb_plane(const struct climb_space *space, const struct polygon *tri,
            const struct world_object *entry, struct climb_contact *contact)
{
    struct contact_plane *plane = &contact->plane;
    struct impact_triangle face;
    if (sweep_box_misses(&space->box, tri->vertex) ||
        !impact_triangle_init(&face, tri->vertex)) {
        return FALSE;
    }
    shz_vec3_t from = shz_vec3_sub(space->center, zam_load_vec3(tri->vertex));
    Float height = plane_dot(face.normal, from);
    enum impact_feature feature = impact_contact_height(
        &face, from, height, space->reach, space->tolerance, plane);
    if (!feature) {
        return FALSE;
    }
    contact->rim = feature == IMPACT_RIM ? tri : NULL;
    contact->entry = entry;
    contact->distance = space->reach - plane->offset;
    plane->offset -= POLYGON_CLIMB_PUSH;
    if (plane->offset > 0.0F) {
        shz_vec3_t normal =
            height < 0.0F ? shz_vec3_scale(face.normal, -1.0F) : face.normal;
        if ((height < 0.0F && !(tri->attr & NJD_FLAG_DOUBLE_SIDE)) ||
            climb_surface(entry->flags, normal, space->up) != CLIMB_BLOCK) {
            plane->offset = 0.0F;
        } else if (plane->offset > POLYGON_CLIMB_PUSH) {
            plane->offset = POLYGON_CLIMB_PUSH;
        }
    }
    return TRUE;
}

/* Only a rim that would change the correction needs an adjacency check. */
static Bool climb_exposed(struct climb_contact *contact,
                          const struct climb_space *space)
{
    if (!contact->rim) {
        return TRUE;
    }
    const struct polygon *tri = contact->rim;
    struct impact_triangle face;
    impact_triangle_init(&face, tri->vertex);
    shz_vec3_t from = shz_vec3_sub(space->center, zam_load_vec3(tri->vertex));
    if (climb_internal_rim(
            tri, contact->entry, face.normal, from,
            shz_vec3_scale(contact->plane.normal, contact->distance),
            space->tolerance)) {
        return FALSE;
    }
    contact->rim = NULL;
    return TRUE;
}

/* Add the most violated touching face and solve again. A correction is
 * accepted only after every collected triangle has checked it. */
LINKED_LAST(climb_push)
static __attribute__((noinline, optimize("fp-contract=off"))) void
climb_push(const struct polygon_ball *ball, shz_vec3_t up, NJS_POINT3 *feet)
{
    if (world_count == WORLD_OBJECTS) {
        return;
    }
    struct box_arena *arena = arena_held();
    for (int i = 0; i < world_count; i++) {
        struct collision_mesh *mesh =
            terrain_mesh(arena, world_objects[i].object);
        if (!mesh || mesh->out_of_polygons) {
            return;
        }
    }
    struct climb_space space = {.center = zam_load_vec3(&ball->pos),
                                .up = up,
                                .reach = ball->radius + POLYGON_CLIMB_PUSH,
                                .tolerance = ball->radius * 0x1p-16F};
    sweep_sphere_bounds(&space.box, ball->pos, space.reach);
    /* A full cache switches later passes back to the mesh. */
    struct climb_contact nearby[64];
    struct contact_plane local[POLYGON_CONTACTS], *active = local;
    int capacity = POLYGON_CONTACTS;
    int count = 0, saved = 0;
    Bool complete = TRUE;
    shz_vec3_t move = {0};
    for (;;) {
        struct terrain_cursor cursor = {.arena = arena};
        struct contact_plane worst = {0};
        Float deficit = space.tolerance, fraction = 1.0F;
        int i = 0;
        for (;;) {
            struct climb_contact temp, *contact;
            if (count && complete) {
                if (i == saved) {
                    break;
                }
                contact = &nearby[i++];
                if (!contact->entry) {
                    continue;
                }
            } else {
                const struct polygon *tri = terrain_next_triangle(&cursor);
                if (!tri) {
                    break;
                }
                contact = &temp;
                if (!count && complete &&
                    saved < (int)(sizeof(nearby) / sizeof(*nearby))) {
                    contact = &nearby[saved];
                }
                if (!climb_plane(&space, tri, &world_objects[cursor.next - 1],
                                 contact)) {
                    continue;
                }
                if (!count) {
                    if (contact == &temp) {
                        complete = FALSE;
                    } else {
                        saved++;
                    }
                }
            }
            struct contact_plane plane = contact->plane;
            Float along = plane_dot(plane.normal, move);
            if (plane.offset < 0.0F) {
                if (along < plane.offset) {
                    Float limit = plane.offset / along;
                    if (limit < fraction && climb_exposed(contact, &space)) {
                        fraction = limit;
                    }
                }
                continue;
            }
            Float gap = plane.offset - along;
            if (gap > deficit || (gap == deficit && gap > space.tolerance &&
                                  normal_before(plane.normal, worst.normal))) {
                if (climb_exposed(contact, &space)) {
                    deficit = gap;
                    worst = plane;
                } else {
                    contact->entry = NULL;
                }
            }
        }
        if (deficit == space.tolerance) {
            Float squared = plane_dot(move, move);
            if (squared * fraction * fraction >
                POLYGON_CLIMB_PUSH * POLYGON_CLIMB_PUSH) {
                fraction = POLYGON_CLIMB_PUSH / njSqrt(squared);
            }
            zam_store_vec3(feet, shz_vec3_add(zam_load_vec3(feet),
                                              shz_vec3_scale(move, fraction)));
            break;
        }
        int previous = count;
        if (!sweep_add_plane(active, &count, capacity, worst)) {
            /* Each triangle supplies at most one normal at a fixed center. */
            int limit = complete ? saved : near_polygon_count;
            if (active != local || limit <= count) {
                break;
            }
            struct contact_plane *more = syMalloc(limit * sizeof *more);
            if (!more) {
                break;
            }
            for (int j = 0; j < count; j++) {
                more[j] = local[j];
            }
            active = more;
            capacity = limit;
            continue;
        }
        if (count == previous || !planes_project(active, count, (shz_vec3_t){0},
                                                 space.tolerance, &move)) {
            break;
        }
    }
    if (active != local) {
        syFree(active);
    }
}

LINKED_LAST(polygon_block_climb)
__attribute__((optimize("fp-contract=off"))) Bool polygon_block_climb(
    struct polygon_ball *ball, const NJS_VECTOR *wall_up, NJS_POINT3 *idle_feet)
{
    shz_vec3_t up = zam_load_vec3(wall_up);
    shz_vec3_t start = zam_load_vec3(&ball->pos);
    shz_vec3_t move = zam_load_vec3(&ball->speed);
    Float tolerance = impact_tolerance(ball->radius, ball->speed);
    struct sweep_bounds box;
    sweep_bounds(&box, ball);
    struct climb_hit best = {0};
    Float tangent = (njAbs(move.x) + njAbs(move.y) + njAbs(move.z)) * 0x1p-18F;
    Bool moving = move.x || move.y || move.z;
    Bool penetrates = FALSE;
    struct terrain_cursor cursor = {.arena = arena_held()};
    const struct polygon *tri;
    while ((tri = terrain_next_triangle(&cursor))) {
        struct impact_triangle face;
        if (sweep_box_misses(&box, tri->vertex) ||
            !impact_triangle_init(&face, tri->vertex)) {
            continue;
        }
        Uint32 flags = world_objects[cursor.next - 1].flags;
        shz_vec3_t from = shz_vec3_sub(start, zam_load_vec3(tri->vertex));
        Float height = plane_dot(face.normal, from);
        Bool both = tri->attr & NJD_FLAG_DOUBLE_SIDE;
        shz_vec3_t normal = height < 0.0F && both
                                ? shz_vec3_scale(face.normal, -1.0F)
                                : face.normal;
        enum climb_surface kind = climb_surface(flags, normal, up);
        Float delta = plane_dot(face.normal, move);
        if (kind == CLIMB_CLEAR &&
            !(both && (height < 0.0F) != (height + delta < 0.0F))) {
            continue;
        }
        if (!both && height < 0.0F && height + delta < 0.0F) {
            continue;
        }
        /* At equal priority only a hit at the start can tie a zero-time
         * hit. An idle correction still needs evidence of penetration. */
        struct climb_hit first = {kind, 0.0F, njAbs(height), normal};
        if (debug_hidden < 0 && kind != CLIMB_CLEAR &&
            (!idle_feet || penetrates || best.kind == CLIMB_STOP) &&
            !climb_before(first, best)) {
            continue;
        }
        Float time;
        struct contact_plane plane;
        enum impact_feature feature =
            impact_cast(&face, from, move, height, ball->radius, tolerance,
                        FALSE, 1.0F, &time, &plane);
        if (!feature) {
            continue;
        }
        Float at = height + time * delta;
        if (!both && at < 0.0F) {
            continue;
        }
        if (both && (height < 0.0F) != (at < 0.0F)) {
            normal = shz_vec3_scale(normal, -1.0F);
            kind = climb_surface(flags, normal, up);
        }
        if (kind == CLIMB_CLEAR) {
            continue;
        }
        if (kind == CLIMB_BLOCK &&
            (time == 0.0F || (height < 0.0F ? delta : -delta) <= tangent)) {
            shz_vec3_t pos = from;
            if (feature == IMPACT_LATER) {
                pos = shz_vec3_add(from, shz_vec3_scale(move, time));
                feature = impact_contact_height(&face, pos, at, ball->radius,
                                                tolerance, &plane);
            }
            /* A straight move already separating from a convex triangle
             * can only increase its distance. */
            if (time == 0.0F &&
                (!feature || (moving ? plane_dot(plane.normal, move) >= -tangent
                                     : plane.offset <= tolerance))) {
                continue;
            }
            if (moving && feature == IMPACT_RIM &&
                climb_internal_rim(
                    tri, &world_objects[cursor.next - 1], face.normal, pos,
                    shz_vec3_scale(plane.normal, ball->radius - plane.offset),
                    tolerance)) {
                continue;
            }
            if (time == 0.0F && plane.offset > tolerance) {
                penetrates = TRUE;
            }
        }
        if (debug_hidden >= 0) {
            NJS_MATRIX caller;
            njGetMatrix(&caller);
            sweep_draw(tri, face.normal);
            njSetMatrix(NULL, &caller);
        }
        struct climb_hit hit = {kind, time, njAbs(at), normal};
        if (climb_before(hit, best)) {
            best = hit;
        }
    }
    if (best.kind != CLIMB_CLEAR) {
        zam_store_vec3(&ball->hit.normal, best.normal);
    }
    if (best.kind == CLIMB_BLOCK && idle_feet && penetrates) {
        climb_push(ball, up, idle_feet);
    }
    return best.kind != CLIMB_CLEAR;
}

/* Work in the starting foot's frame. Repeatedly adding a short slide to a
 * large world coordinate would change the answer when a face is split. */
struct ground_path {
    shz_vec3_t origin, center, up, speed;
    const struct polygon *plane;
    const struct world_object *owner;
    Float radius, snap, turn_cosine, epsilon;
    Uint32 liquid_mask;
};

/* A two-sided sheet keeps the face the runner was already standing on,
 * including where a loop turns it past vertical. */
static inline shz_vec3_t support_normal(const struct polygon *tri,
                                        const struct impact_triangle *face,
                                        shz_vec3_t up)
{
    if ((tri->attr & NJD_FLAG_DOUBLE_SIDE) && plane_dot(face->normal, up) < 0) {
        return shz_vec3_scale(face->normal, -1.0F);
    }
    return face->normal;
}

static inline void ground_bounds(struct sweep_bounds *box,
                                 const struct ground_path *p, shz_vec3_t at,
                                 shz_vec3_t move, Float radius)
{
    sweep_relative_bounds(box, &p->origin, at, move, radius);
}

static inline shz_vec3_t ground_from(const struct ground_path *path,
                                     const struct polygon *tri, shz_vec3_t at)
{
    return shz_vec3_add(shz_vec3_sub(path->origin, zam_load_vec3(tri->vertex)),
                        at);
}

/* The previous owner and normal identify the sheet under the starting foot.
 * Stacked faces on one object can have the same normal at different heights. */
LINKED_LAST(ground_anchor)
static __attribute__((noinline)) Bool ground_anchor(struct ground_path *p,
                                                    NJS_OBJECT *previous)
{
    struct collision_mesh *mesh = sweep_mesh(arena_held(), previous);
    if (!mesh || !mesh->near_count) {
        return FALSE;
    }
    struct near_cursor cursor = {.group = mesh->near, .back = mesh->near_back};
    struct sweep_bounds box;
    ground_bounds(&box, p, (shz_vec3_t){0}, (shz_vec3_t){0},
                  p->snap + p->epsilon);
    shz_vec3_t up = p->up;
    Float nearest = __builtin_inff();
    Bool ambiguous = FALSE;
    for (int i = 0; i < mesh->near_count; i++) {
        const struct polygon *tri = near_next(&cursor);
        struct impact_triangle face;
        if (sweep_box_misses(&box, tri->vertex) ||
            !impact_triangle_init(&face, tri->vertex)) {
            continue;
        }
        shz_vec3_t n = support_normal(tri, &face, p->up);
        if (plane_dot(n, up) < 0.9999F) {
            continue;
        }
        shz_vec3_t from = ground_from(p, tri, (shz_vec3_t){0});
        Float height = plane_dot(n, from);
        if (njAbs(height) > p->snap + p->epsilon ||
            !impact_inside(&face, from, p->epsilon)) {
            continue;
        }
        Float distance = njAbs(height);
        if (distance < nearest) {
            ambiguous = FALSE;
        } else if (distance == nearest) {
            if (plane_dot(p->up, n) < GROUND_PARALLEL ||
                !terrain_same_plane(p->plane, p->up, tri, p->epsilon)) {
                ambiguous = TRUE;
            }
            if ((uintptr_t)tri >= (uintptr_t)p->plane) {
                continue;
            }
        } else {
            continue;
        }
        nearest = distance;
        p->plane = tri;
        p->up = n;
        p->center = shz_vec3_scale(n, p->radius - height);
    }
    return p->plane && !ambiguous;
}

/* A sphere moving on the support's offset plane can graze an edge of a
 * triangle below it. That edge cannot block the tangent motion. */
static Bool ground_below_plane(const struct ground_path *p,
                               const struct polygon *tri, shz_vec3_t contact)
{
    for (int i = 0; i < 3; i++) {
        shz_vec3_t from = shz_vec3_sub(
            shz_vec3_sub(zam_load_vec3(tri->vertex + i), p->origin), contact);
        if (plane_dot(p->up, from) > p->epsilon) {
            return FALSE;
        }
    }
    return TRUE;
}

/* The snap that finds a starting sheet must also leave the body a clear
 * route to that sheet. An overhang or a moving solid can occupy this space. */
LINKED_LAST(ground_snap_clear)
static __attribute__((noinline)) Bool ground_snap_clear(
    const struct ground_path *p, shz_vec3_t original, shz_vec3_t contact)
{
    shz_vec3_t correction = shz_vec3_sub(p->center, original);
    if (plane_dot(correction, correction) <= p->epsilon * p->epsilon) {
        return TRUE;
    }
    struct sweep_bounds box;
    ground_bounds(&box, p, original, correction, p->radius + p->epsilon);
    struct terrain_cursor cursor = {.arena = arena_held(),
                                    .liquid_mask = p->liquid_mask};
    const struct polygon *tri;
    while ((tri = terrain_next_triangle(&cursor))) {
        if (sweep_box_misses(&box, tri->vertex)) {
            continue;
        }
        struct impact_triangle face;
        if (!impact_triangle_init(&face, tri->vertex)) {
            continue;
        }
        Float facing = plane_dot(p->up, face.normal);
        if (p->plane && cursor.ordinary &&
            (facing > GROUND_PARALLEL || ((tri->attr & NJD_FLAG_DOUBLE_SIDE) &&
                                          facing < -GROUND_PARALLEL)) &&
            terrain_same_plane(p->plane, p->up, tri, p->epsilon)) {
            continue;
        }
        Float t;
        if (impact_time(&face, ground_from(p, tri, original), correction,
                        p->radius, p->epsilon, FALSE, 1.0F, &t)) {
            if (cursor.ordinary && ground_below_plane(p, tri, contact) &&
                plane_dot(p->up, shz_vec3_sub(original, contact)) >=
                    p->radius - p->epsilon &&
                plane_dot(p->up, shz_vec3_sub(p->center, contact)) >=
                    p->radius - p->epsilon) {
                continue;
            }
            return FALSE;
        }
    }
    return TRUE;
}

struct ground_event {
    const struct world_object *owner;
    const struct polygon *tri;
    shz_vec3_t up;
    Float time;
    Bool blocked;
};

/* A long strip can arrive as more disjoint spans than fit in the workspace.
 * Scan its remaining suffix only if a discarded interval can extend the
 * connected prefix. Each pass reaches farther or establishes the first gap. */
LINKED_LAST(ground_extend)
static Float ground_extend(const struct ground_path *p, shz_vec3_t foot,
                           shz_vec3_t move, Float from,
                           const struct world_object **end_owner)
{
    struct sweep_bounds box;
    ground_bounds(&box, p, foot, move, p->epsilon);
    for (;;) {
        struct ground_spans spans;
        spans.count = 0;
        spans.omitted = 1.0F;
        struct terrain_cursor cursor = {.arena = arena_held()};
        const struct polygon *tri;
        while ((tri = terrain_next_triangle(&cursor))) {
            struct impact_triangle face;
            if (!cursor.ordinary || sweep_box_misses(&box, tri->vertex) ||
                !terrain_same_plane(p->plane, p->up, tri, p->epsilon) ||
                !impact_triangle_init(&face, tri->vertex) ||
                plane_dot(support_normal(tri, &face, p->up), p->up) <
                    GROUND_PARALLEL) {
                continue;
            }
            Float lo, hi;
            if (ground_interval(&face, ground_from(p, tri, foot), move,
                                p->epsilon, &lo, &hi) &&
                hi > from) {
                ground_add_span(&spans, lo < from ? from : lo, hi);
                const struct world_object *entry =
                    &world_objects[cursor.next - 1];
                if (end_owner && hi == 1 &&
                    ground_prefers(p->owner, *end_owner, entry)) {
                    *end_owner = entry;
                }
            }
        }
        Float end = ground_prefix(&spans, from);
        if (end == from || end >= 1.0F || end < spans.omitted) {
            return end;
        }
        from = end;
    }
}

/* A single-sided face only blocks from its front. When testing a raised
 * stance, skip the tread and the material beneath its supporting plane. */
LINKED_LAST(step_clear_fraction)
static __attribute__((cold)) Float
step_clear_fraction(const struct ground_path *p, shz_vec3_t at,
                    shz_vec3_t travel, const shz_vec3_t *tread, Bool *underside)
{
    struct sweep_bounds box;
    ground_bounds(&box, p, at, travel, p->radius + p->epsilon);
    Float first = 1.0F;
    struct terrain_cursor cursor = {.arena = arena_held(),
                                    .liquid_mask = p->liquid_mask};
    const struct polygon *tri;
    while ((tri = terrain_next_triangle(&cursor))) {
        struct impact_triangle face;
        if (sweep_box_misses(&box, tri->vertex) ||
            (tread && ground_below_plane(p, tri, *tread)) ||
            !impact_triangle_init(&face, tri->vertex)) {
            continue;
        }
        shz_vec3_t from = ground_from(p, tri, at);
        Float height = plane_dot(face.normal, from);
        Float time;
        if (height < -p->epsilon && !(tri->attr & NJD_FLAG_DOUBLE_SIDE)) {
            if (underside && face.normal.y > 0 &&
                impact_time_height(&face, from, travel, height, p->radius,
                                   p->epsilon, FALSE, 1, &time)) {
                *underside = TRUE;
            }
            continue;
        }
        if (impact_time_height(&face, from, travel, height, p->radius,
                               p->epsilon, TRUE, first, &time)) {
            first = time;
        }
    }
    return first;
}

/* If reaching a raised tread would press the body into an overhang,
 * its exposed rim blocks the approach on the lower floor. */
LINKED_LAST(polygon_step_clearance)
__attribute__((cold)) Bool polygon_step_clearance(
    const struct polygon_ball *body, NJS_OBJECT *previous, Float turn_cosine,
    Uint32 liquid_mask, struct ground_step *step)
{
    if (body->normal.x || body->normal.y != 1 || body->normal.z ||
        body->speed.y) {
        return FALSE;
    }
    struct ground_path p = {.origin = zam_load_vec3(&body->pos),
                            .center = {.y = body->radius},
                            .speed = zam_load_vec3(&body->speed),
                            .up = {.y = 1},
                            .liquid_mask = liquid_mask,
                            .radius = body->radius,
                            .epsilon =
                                impact_tolerance(body->radius, body->speed)};
    struct polygon_ball region = *body;
    region.pos.y += p.radius;
    /* A rim can lie one radius ahead of first contact and one radius up. */
    region.radius =
        (p.speed.x || p.speed.z ? 3 * p.radius : p.radius) + p.epsilon;
    if (!polygon_collect_near(&region) || world_count == WORLD_OBJECTS) {
        return FALSE;
    }
    struct box_arena *arena = arena_held();
    for (int i = 0; i < world_count; i++) {
        if (world_objects[i].flags & liquid_mask) {
            continue;
        }
        const struct collision_mesh *mesh =
            terrain_mesh(arena, world_objects[i].object);
        if (!mesh || mesh->out_of_polygons ||
            (mesh->near_count && !world_plain_land(&world_objects[i]))) {
            return FALSE;
        }
        if (world_objects[i].object == previous) {
            p.owner = &world_objects[i];
        }
    }
    if (!p.owner || !ground_anchor(&p, previous)) {
        return FALSE;
    }
    struct sweep_bounds box;
    ground_bounds(&box, &p, p.center, p.speed, p.radius + p.epsilon);
    struct terrain_cursor cursor = {.arena = arena, .liquid_mask = liquid_mask};
    const struct polygon *tri;
    Float first = 1.0F;
    shz_vec3_t wall = {0};
    const struct world_object *support = NULL;
    Float rise = 0.0F;
    Bool below = FALSE;
    while ((tri = terrain_next_triangle(&cursor))) {
        struct impact_triangle face;
        if (sweep_box_misses(&box, tri->vertex) ||
            !impact_triangle_init(&face, tri->vertex)) {
            continue;
        }
        shz_vec3_t from = ground_from(&p, tri, p.center);
        Float height = plane_dot(face.normal, from);
        shz_vec3_t up = height < 0 && (tri->attr & NJD_FLAG_DOUBLE_SIDE)
                            ? shz_vec3_scale(face.normal, -1)
                            : face.normal;
        if (up.y >= turn_cosine && height < -p.epsilon &&
            !(tri->attr & NJD_FLAG_DOUBLE_SIDE)) {
            Float time;
            if (impact_time_height(&face, from, p.speed, height, p.radius,
                                   p.epsilon, FALSE, 1, &time)) {
                below = TRUE;
            }
            continue;
        }
        if (up.y < turn_cosine || plane_dot(up, from) < 0 ||
            ground_below_plane(&p, tri, (shz_vec3_t){0})) {
            continue;
        }
        Float time;
        if (!impact_time_height(&face, from, p.speed, height, p.radius,
                                p.epsilon, TRUE, first, &time)) {
            continue;
        }
        shz_vec3_t approach = shz_vec3_scale(p.speed, time);
        struct contact_plane rim;
        if (impact_contact(&face, shz_vec3_add(from, approach), p.radius,
                           p.epsilon, &rim) != IMPACT_RIM ||
            plane_dot(rim.normal, p.speed) >= -p.epsilon) {
            continue;
        }
        shz_vec3_t at = shz_vec3_add(p.center, approach);
        shz_vec3_t edge =
            shz_vec3_sub(at, shz_vec3_scale(rim.normal, p.radius - rim.offset));
        if (edge.y <= p.epsilon || edge.y > p.radius) {
            continue;
        }
        shz_vec3_t stance = shz_vec3_add(edge, shz_vec3_scale(up, p.radius));
        /* When the requested foot reaches this tread, clear the whole
         * stride above it. A short approach still tests the lip itself. */
        shz_vec3_t end = shz_vec3_add(p.center, p.speed);
        end.y += (p.radius - plane_dot(up, shz_vec3_sub(end, edge))) / up.y;
        Bool enters =
            impact_inside(&face, ground_from(&p, tri, end), p.epsilon);
        if (enters) {
            stance = end;
        }
        shz_vec3_t raised = at;
        raised.y = stance.y;
        struct ground_path tread = p;
        tread.up = up;
        Bool underside = FALSE;
        if (step_clear_fraction(&tread, at, shz_vec3_sub(raised, at), &edge,
                                &underside) == 1.0F &&
            step_clear_fraction(&tread, raised, shz_vec3_sub(stance, raised),
                                &edge, &underside) == 1.0F) {
            const struct world_object *owner = &world_objects[cursor.next - 1];
            if (underside && enters && up.y == 1.0F &&
                (!time || step_clear_fraction(&p, p.center, approach, NULL,
                                              NULL) == 1.0F) &&
                (edge.y > rise ||
                 (edge.y == rise && ground_prefers(p.owner, support, owner)))) {
                support = owner;
                rise = edge.y;
            }
            continue;
        }
        first = time;
        wall = rim.normal;
        wall.y = 0;
    }
    if (!(wall.x || wall.z)) {
        /* From underneath, an overlapping one-sided sheet cannot supply
         * ground. Keep the reachable tread while the body passes below it. */
        if (!support &&
            (!below ||
             step_clear_fraction(&p, p.center, p.speed, NULL, NULL) < 1.0F ||
             ground_extend(&p, (shz_vec3_t){0}, p.speed, 0, &support) < 1.0F)) {
            return FALSE;
        }
        *step = (struct ground_step){.velocity = body->speed,
                                     .normal = body->normal,
                                     .support = support->object,
                                     .flags = support->flags};
        zam_store_vec3(&step->feet, shz_vec3_add(p.origin, p.speed));
        step->feet.y += rise;
        return TRUE;
    }
    wall = shz_vec3_scale(wall, 1.0F / njSqrt(plane_dot(wall, wall)));
    shz_vec3_t advance = shz_vec3_scale(p.speed, first);
    Float entry = step_clear_fraction(&p, p.center, advance, NULL, NULL);
    advance = shz_vec3_scale(advance, entry);
    shz_vec3_t at = shz_vec3_add(p.center, advance);
    shz_vec3_t slide =
        shz_vec3_sub(p.speed, shz_vec3_scale(wall, plane_dot(p.speed, wall)));
    shz_vec3_t rest = shz_vec3_scale(slide, 1.0F - first);
    support = NULL;
    if (ground_extend(&p, (shz_vec3_t){0}, advance, 0, &support) < 1.0F) {
        return FALSE;
    }
    if (entry < 1.0F) {
        slide = rest = (shz_vec3_t){0};
    } else if (rest.x || rest.z) {
        Float clear = step_clear_fraction(&p, at, rest, NULL, NULL);
        if (clear < 1.0F) {
            rest = shz_vec3_scale(rest, clear);
            slide = (shz_vec3_t){0};
        }
        if (ground_extend(&p, advance, rest, 0, &support) < 1.0F) {
            slide = rest = (shz_vec3_t){0};
        }
    }
    zam_store_vec3(&step->feet,
                   shz_vec3_add(p.origin, shz_vec3_add(advance, rest)));
    zam_store_vec3(&step->velocity, slide);
    zam_store_vec3(&step->wall, wall);
    step->normal = body->normal;
    step->moving_contact = FALSE;
    step->owner = NULL;
    step->support = support->object;
    step->flags = support->flags;
    step->stalled = FALSE;
    step->turned_wall = FALSE;
    return TRUE;
}

/* The center can slide beyond the finite floor it landed on this tick.
 * Follow the union of its coplanar faces until the first uncovered point. */
LINKED_LAST(landing_advance)
static __attribute__((noinline)) void
landing_advance(struct wall_motion *motion, shz_vec3_t travel)
{
    struct landing *land = &motion->land;
    struct impact_triangle face;
    impact_triangle_init(&face, land->face->vertex);
    shz_vec3_t from = wall_relative(motion, land->face);
    if (land->source == land->owner &&
        impact_inside(&face, from, motion->tolerance) &&
        impact_inside(&face, shz_vec3_add(from, travel), motion->tolerance)) {
        return;
    }
    struct ground_path p = {.origin = motion->origin,
                            .up = land->up,
                            .plane = land->face,
                            .owner = land->owner,
                            .epsilon = motion->tolerance};
    shz_vec3_t foot =
        shz_vec3_sub(motion->at, shz_vec3_scale(land->up, motion->radius));
    const struct world_object *end = NULL;
    land->owner = ground_extend(&p, foot, travel, 0, &end) == 1.0F ? end : NULL;
}

/* Ordinary floors can supply a shared edge. Everything else retains its
 * own collision response, including the game's larger step probe. */
LINKED_LAST(ground_segment)
static __attribute__((noinline)) Bool ground_segment(
    const struct ground_path *p, shz_vec3_t move, struct ground_event *event,
    struct ground_spans *spans, const struct world_object **end_owner)
{
    struct sweep_bounds box;
    Float clearance = p->radius + p->snap + p->epsilon;
    ground_bounds(&box, p, p->center, move, clearance);
    struct terrain_cursor cursor = {.arena = arena_held(),
                                    .liquid_mask = p->liquid_mask};
    const struct polygon *tri;
    while ((tri = terrain_next_triangle(&cursor))) {
        struct impact_triangle face;
        if (sweep_box_misses(&box, tri->vertex) ||
            !impact_triangle_init(&face, tri->vertex)) {
            continue;
        }
        const struct world_object *owner = &world_objects[cursor.next - 1];
        shz_vec3_t from = ground_from(p, tri, p->center);
        shz_vec3_t n = support_normal(tri, &face, p->up);
        /* The previous sheet sets the frame for a continuation. A sharp
         * corner blocks, and ground_join verifies each turn's shared edge. */
        Float facing = plane_dot(n, p->up);
        Bool walkable = cursor.ordinary && facing >= p->turn_cosine;
        if (walkable && facing > GROUND_PARALLEL &&
            terrain_same_plane(p->plane, p->up, tri, p->epsilon)) {
            shz_vec3_t foot =
                shz_vec3_sub(from, shz_vec3_scale(p->up, p->radius));
            Float lo, hi;
            if (ground_interval(&face, foot, move, p->epsilon, &lo, &hi)) {
                ground_add_span(spans, lo, hi);
                if (hi == 1.0F && ground_prefers(p->owner, *end_owner, owner)) {
                    *end_owner = owner;
                }
            }
            continue;
        }
        Float time;
        if (!impact_time(&face, from, move, walkable ? p->radius : clearance,
                         p->epsilon, walkable, event->time, &time)) {
            continue;
        }
        if (walkable &&
            ground_below_plane(
                p, tri,
                shz_vec3_sub(zam_load_vec3(p->plane->vertex), p->origin))) {
            continue;
        }
        struct contact_plane contact;
        Bool join = walkable &&
                    impact_contact(
                        &face, shz_vec3_add(from, shz_vec3_scale(move, time)),
                        p->radius, p->epsilon, &contact) == IMPACT_FACE &&
                    contact.offset <= p->epsilon &&
                    plane_dot(contact.normal, n) > GROUND_PARALLEL;
        if (time == event->time && event->tri) {
            if (event->blocked ||
                (join && plane_dot(event->up, n) < GROUND_PARALLEL)) {
                event->blocked = TRUE;
                continue;
            }
            if (join && (uintptr_t)event->tri < (uintptr_t)tri) {
                continue;
            }
        }
        event->time = time;
        event->tri = tri;
        event->owner = owner;
        event->up = n;
        event->blocked = !join;
        if (!join && time == 0.0F) {
            return FALSE;
        }
    }
    return TRUE;
}

/* Two nearly parallel planes determine a crease poorly in float. Project
 * the estimated crossing onto a finite edge from the mesh. The caller then
 * checks that both sheets actually reach the same edge. */
static Bool ground_locate_edge(const struct ground_path *p,
                               const struct polygon *tri,
                               const struct ground_event *hit,
                               Float uncertainty, shz_vec3_t *crease,
                               struct ground_edge *line)
{
    if (!terrain_same_plane(p->plane, p->up, tri, p->epsilon)) {
        return FALSE;
    }
    shz_vec3_t opposite = zam_load_vec3(hit->tri->vertex);
    shz_vec3_t a[3];
    Float height[3];
    int i;
    for (i = 0; i < 3; i++) {
        a[i] = zam_load_vec3(tri->vertex + i);
        height[i] = plane_dot(hit->up, shz_vec3_sub(a[i], opposite));
    }
    for (i = 0; i < 3; i++) {
        int next = (i + 1) % 3;
        if (njAbs(height[i]) > p->epsilon || njAbs(height[next]) > p->epsilon ||
            height[(i + 2) % 3] <= p->epsilon) {
            continue;
        }
        shz_vec3_t edge = shz_vec3_sub(a[next], a[i]);
        shz_vec3_t from = shz_vec3_add(shz_vec3_sub(p->origin, a[i]), *crease);
        Float length2 = plane_dot(edge, edge);
        if (!(length2 > 0.0F)) {
            continue;
        }
        Float fraction = capsule_fraction(plane_dot(from, edge), length2);
        shz_vec3_t offset = shz_vec3_sub(shz_vec3_scale(edge, fraction), from);
        if (plane_dot(offset, offset) <= uncertainty * uncertainty) {
            *crease = shz_vec3_add(*crease, offset);
            *line = (struct ground_edge){a[i], edge, length2};
            return TRUE;
        }
    }
    return FALSE;
}

/* At an inside bend the center touches both offset planes. Their common
 * edge passes through q. Require overlapping finite edge lengths there on
 * both sheets; coincident infinite planes or a shared vertex do not suffice. */
LINKED_LAST(ground_join)
static __attribute__((noinline)) Bool
ground_join(const struct ground_path *p, const struct ground_event *hit)
{
    shz_vec3_t axis = plane_cross(p->up, hit->up);
    Float scale = plane_dot(axis, axis);
    if (!(scale > 0.0F)) {
        return FALSE;
    }
    shz_vec3_t q = shz_vec3_sub(
        p->center,
        shz_vec3_scale(shz_vec3_add(p->up, hit->up),
                       p->radius / (1.0F + plane_dot(p->up, hit->up))));
    struct sweep_bounds box;
    ground_bounds(&box, p, q, (shz_vec3_t){0}, p->radius + p->epsilon);
    struct ground_edge line;
    Float uncertainty = p->epsilon * (2.0F + 1.0F / njSqrt(scale));
    if (!ground_locate_edge(p, p->plane, hit, uncertainty, &q, &line)) {
        struct terrain_cursor search = {.arena = arena_held()};
        const struct polygon *tri;
        while ((tri = terrain_next_triangle(&search))) {
            if (search.ordinary && !sweep_box_misses(&box, tri->vertex) &&
                ground_locate_edge(p, tri, hit, uncertainty, &q, &line)) {
                break;
            }
        }
        if (!tri) {
            return FALSE;
        }
    }
    axis = shz_vec3_scale(line.direction, 1.0F / njSqrt(line.length2));
    int edges[2] = {0};
    struct ground_spans coverage[2];
    for (int i = 0; i < 2; i++) {
        coverage[i].count = 0;
        coverage[i].omitted = 1.0F;
    }

    struct terrain_cursor cursor = {.arena = arena_held()};
    const struct polygon *tri;
    while ((tri = terrain_next_triangle(&cursor))) {
        struct impact_triangle face;
        if (!cursor.ordinary || sweep_box_misses(&box, tri->vertex) ||
            !impact_triangle_init(&face, tri->vertex)) {
            continue;
        }
        shz_vec3_t from = ground_from(p, tri, q);
        shz_vec3_t v[3];
        for (int i = 0; i < 3; i++) {
            v[i] = shz_vec3_sub(face.v[i], from);
        }
        for (int side = 0; side < 2; side++) {
            shz_vec3_t n = side ? hit->up : p->up,
                       other = side ? p->up : hit->up;
            if (njAbs(plane_dot(n, v[0])) > p->epsilon ||
                njAbs(plane_dot(n, v[1])) > p->epsilon ||
                njAbs(plane_dot(n, v[2])) > p->epsilon) {
                continue;
            }
            Float facing = plane_dot(n, face.normal);
            if (facing < GROUND_PARALLEL &&
                !((tri->attr & NJD_FLAG_DOUBLE_SIDE) &&
                  facing < -GROUND_PARALLEL)) {
                continue;
            }
            /* Both contact points must reach this crease on their sheets.
             * A strip ending before the crease leaves a real gap. */
            shz_vec3_t foot =
                shz_vec3_sub(p->center, shz_vec3_scale(n, p->radius));
            Float lo, hi;
            if (ground_interval(&face, from, shz_vec3_sub(foot, q), p->epsilon,
                                &lo, &hi)) {
                ground_add_span(&coverage[side], lo, hi);
            }
            edges[side] |= ground_edge_sides(&line, axis, tri->vertex, v, n,
                                             other, p->epsilon);
        }
        if ((edges[0] & edges[1]) &&
            ground_prefix(&coverage[0], 0.0F) == 1.0F &&
            ground_prefix(&coverage[1], 0.0F) == 1.0F) {
            return TRUE;
        }
    }
    if (edges[0] & edges[1]) {
        for (int side = 0; side < 2; side++) {
            Float end = ground_prefix(&coverage[side], 0.0F);
            if (end == 1.0F) {
                continue;
            }
            if (end < coverage[side].omitted) {
                return FALSE;
            }
            struct ground_path sheet = *p;
            if (side) {
                sheet.plane = hit->tri;
                sheet.up = hit->up;
            }
            shz_vec3_t foot =
                shz_vec3_sub(p->center, shz_vec3_scale(sheet.up, p->radius));
            if (ground_extend(&sheet, q, shz_vec3_sub(foot, q), end, NULL) <
                1.0F) {
                return FALSE;
            }
        }
        return TRUE;
    }
    return FALSE;
}

/* The contact runs along the crease while the center turns. Cover that
 * finite length on both sides, including a join split into shorter edges.
 * Stationary contact still requires a shared length beside the crossing. */
LINKED_LAST(ground_shares_crest)
static __attribute__((noinline)) Bool ground_shares_crest(
    const struct ground_path *p, shz_vec3_t source_up,
    const struct ground_edge *line, shz_vec3_t q, shz_vec3_t axis, Float drift,
    const struct polygon *next, shz_vec3_t n)
{
    Float margin = 4.0F * p->epsilon;
    if (drift < 0.0F) {
        drift = -drift;
        axis = shz_vec3_scale(axis, -1.0F);
    }
    struct sweep_bounds box;
    ground_bounds(&box, p, q, shz_vec3_scale(axis, drift), margin);
    Float reach[2] = {0, 0};
    unsigned sides[2] = {0, 0};
    for (;;) {
        Float previous[2] = {reach[0], reach[1]};
        struct terrain_cursor cursor = {.arena = arena_held()};
        const struct polygon *tri;
        while ((tri = terrain_next_triangle(&cursor))) {
            struct impact_triangle face;
            if (!cursor.ordinary || sweep_box_misses(&box, tri->vertex) ||
                !impact_triangle_init(&face, tri->vertex)) {
                continue;
            }
            int side;
            shz_vec3_t normal = ground_face_normal(tri, &face), other;
            if (plane_dot(normal, source_up) >= GROUND_PARALLEL &&
                terrain_same_plane(p->plane, source_up, tri, p->epsilon)) {
                side = 0;
                other = n;
            } else if (plane_dot(normal, n) >= GROUND_PARALLEL &&
                       terrain_same_plane(next, n, tri, p->epsilon)) {
                side = 1;
                other = source_up;
            } else {
                continue;
            }
            shz_vec3_t from = ground_from(p, tri, q);
            for (int i = 0; i < 3; i++) {
                int j = (i + 1) % 3;
                if (!ground_on_edge_line(line, tri->vertex + i, margin) ||
                    !ground_on_edge_line(line, tri->vertex + j, margin) ||
                    plane_dot(other, shz_vec3_sub(face.v[(i + 2) % 3], from)) >=
                        -margin) {
                    continue;
                }
                Float a = plane_dot(axis, shz_vec3_sub(face.v[i], from));
                Float b = plane_dot(axis, shz_vec3_sub(face.v[j], from));
                if (a > b) {
                    Float swap = a;
                    a = b;
                    b = swap;
                }
                if (a <= margin && b >= -margin) {
                    sides[side] |= (a < -margin) | ((b > margin) << 1);
                }
                if (a <= reach[side] + margin && b > reach[side]) {
                    reach[side] = b;
                }
            }
        }
        if (reach[0] + margin >= drift && reach[1] + margin >= drift) {
            return (sides[0] & sides[1]) != 0;
        }
        if (previous[0] == reach[0] && previous[1] == reach[1]) {
            return FALSE;
        }
    }
}

/* A continuous support frame turns its tangent without losing speed. */
static inline shz_vec3_t ground_turn_speed(const struct ground_path *p,
                                           shz_vec3_t next)
{
    Float turn = plane_dot(p->speed, next) / (1.0F + plane_dot(p->up, next));
    return shz_vec3_sub(p->speed,
                        shz_vec3_scale(shz_vec3_add(p->up, next), turn));
}

LINKED_LAST(ground_same_vertex)
static __attribute__((noinline)) Bool ground_same_vertex(const NJS_POINT3 *a,
                                                         const NJS_POINT3 *b)
{
    return a->x == b->x && a->y == b->y && a->z == b->z;
}

/* Find the source triangle at the lip, which can lie past the starting
 * triangle. The two sheets must occupy opposite sides of the same edge. */
LINKED_LAST(ground_welded_edge)
static __attribute__((noinline, cold)) Bool
ground_welded_edge(const struct ground_path *p, const struct polygon *next,
                   shz_vec3_t lip, const struct sweep_bounds *box)
{
    for (int i = 0; i < 3; i++) {
        const NJS_POINT3 *a = next->vertex + i;
        const NJS_POINT3 *b = next->vertex + (i + 1) % 3;
        shz_vec3_t start = zam_load_vec3(a);
        shz_vec3_t edge = shz_vec3_sub(zam_load_vec3(b), start);
        shz_vec3_t from = shz_vec3_add(shz_vec3_sub(p->origin, start), lip);
        Float t =
            capsule_fraction(plane_dot(from, edge), plane_dot(edge, edge));
        shz_vec3_t gap = shz_vec3_sub(from, shz_vec3_scale(edge, t));
        if (plane_dot(gap, gap) > 16.0F * p->epsilon * p->epsilon) {
            continue;
        }
        shz_vec3_t across = plane_cross(p->up, edge);
        shz_vec3_t tip =
            shz_vec3_sub(zam_load_vec3(next->vertex + (i + 2) % 3), start);
        Float side = plane_dot(across, tip);
        if (!(side * plane_dot(across, p->speed) > 0.0F)) {
            continue;
        }
        struct terrain_cursor cursor = {.arena = arena_held()};
        const struct polygon *tri;
        while ((tri = terrain_next_triangle(&cursor))) {
            if (!cursor.ordinary || tri == next ||
                sweep_box_misses(box, tri->vertex)) {
                continue;
            }
            int ai = -1, bi = -1;
            for (int j = 0; j < 3; j++) {
                if (ground_same_vertex(a, tri->vertex + j)) {
                    ai = j;
                } else if (ground_same_vertex(b, tri->vertex + j)) {
                    bi = j;
                }
            }
            if (ai < 0 || bi < 0 ||
                !terrain_same_plane(p->plane, p->up, tri, p->epsilon)) {
                continue;
            }
            shz_vec3_t other =
                shz_vec3_sub(zam_load_vec3(tri->vertex + 3 - ai - bi), start);
            /* A source edge in vertex order gives its signed area along up. */
            Float facing = plane_dot(across, other);
            if (side * facing < 0.0F &&
                ((tri->attr & NJD_FLAG_DOUBLE_SIDE) ||
                 ((ai + 1) % 3 == bi ? facing > 0.0F : facing < 0.0F))) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

enum crest_step { CREST_RELEASE, CREST_SNAP, CREST_FOLLOW };

/* One tick of gravity can turn a running tangent onto a gently departing
 * sheet. A sharper crest needs gravity to hold the body on the rounded edge
 * itself. The original ground snap handles those slow turns. */
LINKED_LAST(ground_crest)
static __attribute__((noinline, cold)) enum crest_step
ground_crest(struct ground_path *path, shz_vec3_t source_up, shz_vec3_t lip,
             Float *gravity, Float *left, const struct world_object *previous)
{
    Float margin = 4.0F * path->epsilon;
    struct sweep_bounds box;
    ground_bounds(&box, path, lip, (shz_vec3_t){0}, margin);
    struct ground_event hit = {0};
    shz_vec3_t foot = {0}, contact = {0};
    struct terrain_cursor cursor = {.arena = arena_held()};
    const struct polygon *tri;
    while ((tri = terrain_next_triangle(&cursor))) {
        if (!cursor.ordinary || sweep_box_misses(&box, tri->vertex)) {
            continue;
        }
        struct impact_triangle face;
        if (!impact_triangle_init(&face, tri->vertex)) {
            continue;
        }
        shz_vec3_t n = support_normal(tri, &face, path->up);
        Float cosine = plane_dot(path->up, n);
        Float outward = plane_dot(path->speed, n);
        shz_vec3_t from = ground_from(path, tri, lip);
        if (njAbs(plane_dot(n, from)) > margin ||
            !impact_inside(&face, from, margin)) {
            continue;
        }
        shz_vec3_t difference = shz_vec3_sub(path->up, n);
        /* A welded fold smaller than the body's contact tolerance can use
         * the shared foot directly. Keep its outgoing tangent and support. */
        if (!previous &&
            !terrain_same_plane(path->plane, path->up, tri, path->epsilon) &&
            plane_dot(difference, difference) * path->radius * path->radius <=
                path->epsilon * path->epsilon &&
            (outward <= 0.0F || (n.y > 0.0F && outward <= *gravity * n.y)) &&
            ground_welded_edge(path, tri, lip, &box)) {
            if (hit.tri &&
                !terrain_same_plane(hit.tri, hit.up, tri, path->epsilon)) {
                return CREST_SNAP;
            }
            const struct world_object *owner = &world_objects[cursor.next - 1];
            if (!hit.tri || ground_prefers(path->owner, hit.owner, owner)) {
                hit = (struct ground_event){owner, tri, n, 0, FALSE};
                contact = lip;
                foot = shz_vec3_sub(lip, shz_vec3_scale(n, plane_dot(n, from)));
            }
            continue;
        }
        if (path->up.y <= 0.0F || n.y <= 0.0F || cosine < path->turn_cosine ||
            outward <= 0.0F) {
            continue;
        }
        Float half =
            (0.5F * path->radius * plane_dot(difference, difference)) / outward;
        Bool gentle = outward <= *gravity * n.y;
        if (!gentle && outward * outward > *gravity * path->up.y *
                                               path->radius *
                                               (1.0F - cosine * cosine)) {
            continue;
        }
        for (int i = 0; i < 3; i++) {
            int j = (i + 1) % 3;
            shz_vec3_t a = shz_vec3_sub(face.v[i], from);
            shz_vec3_t b = shz_vec3_sub(face.v[j], from);
            shz_vec3_t c = shz_vec3_sub(face.v[(i + 2) % 3], from);
            if (njAbs(plane_dot(path->up, a)) > margin ||
                njAbs(plane_dot(path->up, b)) > margin ||
                plane_dot(path->up, c) >= -margin) {
                continue;
            }
            struct ground_edge edge = {.start = zam_load_vec3(tri->vertex + i),
                                       .direction =
                                           shz_vec3_sub(face.v[j], face.v[i])};
            edge.length2 = plane_dot(edge.direction, edge.direction);
            if (!(edge.length2 > 0)) {
                continue;
            }
            Float length = njSqrt(edge.length2);
            shz_vec3_t axis = shz_vec3_scale(edge.direction, 1.0F / length);
            Float along = -plane_dot(axis, a);
            shz_vec3_t offset = shz_vec3_add(a, shz_vec3_scale(axis, along));
            if (plane_dot(offset, offset) > margin * margin) {
                continue;
            }
            shz_vec3_t q = shz_vec3_add(lip, offset);
            Float span = 2.0F * half;
            Float drift =
                plane_dot(axis, path->speed) * (*left < span ? *left : span);
            if (!ground_shares_crest(path, source_up, &edge, q, axis, drift,
                                     tri, n)) {
                continue;
            }
            if (!gentle || span > 1.0F) {
                return CREST_SNAP;
            }
            if (hit.tri &&
                (plane_dot(n, hit.up) < GROUND_PARALLEL ||
                 !terrain_same_plane(hit.tri, hit.up, tri, margin))) {
                return CREST_SNAP;
            }
            const struct world_object *owner = &world_objects[cursor.next - 1];
            if (hit.tri && !ground_prefers(path->owner, hit.owner, owner)) {
                continue;
            }
            hit = (struct ground_event){owner, tri, n, span, FALSE};
            foot = shz_vec3_add(q, shz_vec3_scale(axis, drift));
            contact = q;
        }
    }
    if (!hit.tri) {
        return CREST_RELEASE;
    }
    if (previous && path->owner != previous && hit.owner != previous) {
        return CREST_SNAP;
    }
    Float duration = hit.time;
    shz_vec3_t up = hit.up;
    if (duration > *left) {
        /* tan(angle/2) is proportional to the time spent on the two
         * tangents. The new radial normal also locates the stance next tick. */
        Float u = *left / duration;
        Float cosine = plane_dot(path->up, up);
        shz_vec3_t delta = shz_vec3_sub(up, path->up);
        Float k = 0.5F * plane_dot(delta, delta) / (1.0F + cosine);
        Float a = 1.0F / (1.0F + u * u * k);
        shz_vec3_t tangent = shz_vec3_sub(up, shz_vec3_scale(path->up, cosine));
        up = shz_vec3_add(shz_vec3_scale(path->up, (1.0F - u * u * k) * a),
                          shz_vec3_scale(tangent, u * (1.0F + k) * a));
        duration = *left;
    }
    shz_vec3_t entry =
        shz_vec3_add(contact, shz_vec3_scale(path->up, path->radius));
    shz_vec3_t midway =
        shz_vec3_add(entry, shz_vec3_scale(path->speed, duration * 0.5F));
    /* The two tangents stay outside the rounded edge. Each has its own
     * supporting half-space even when the tick ends between both faces. */
    path->center = midway;
    path->plane = NULL;
    if (previous && !ground_snap_clear(path, entry, contact)) {
        return CREST_SNAP;
    }
    Float outward = plane_dot(path->speed, up);
    if (outward > 0.0F && up.y > 0.0F) {
        *gravity -= outward / up.y;
    }
    path->speed = ground_turn_speed(path, up);
    path->center = shz_vec3_add(foot, shz_vec3_scale(up, path->radius));
    path->up = up;
    if (!ground_snap_clear(path, midway, foot)) {
        return CREST_SNAP;
    }
    if (duration == hit.time) {
        path->plane = hit.tri;
        path->owner = hit.owner;
    }
    *left -= duration;
    return CREST_FOLLOW;
}

/* A rounded stance keeps its foot on the physical crease. Reconstruct the
 * incident sheet from that foot, its radial normal and the tangent motion.
 * ground_crest then proves both finite edges again before taking the turn. */
LINKED_LAST(ground_resume_crest)
static __attribute__((noinline)) Bool ground_resume_crest(struct ground_path *p,
                                                          Float *gravity,
                                                          Float *remaining)
{
    const struct world_object *previous = p->owner;
    struct sweep_bounds box;
    Float margin = 4.0F * p->epsilon;
    ground_bounds(&box, p, (shz_vec3_t){0}, (shz_vec3_t){0}, margin);
    struct terrain_cursor cursor = {.arena = arena_held()};
    const struct polygon *tri;
    p->plane = NULL;
    shz_vec3_t source = {0};
    while ((tri = terrain_next_triangle(&cursor))) {
        struct impact_triangle face;
        if (!cursor.ordinary || sweep_box_misses(&box, tri->vertex) ||
            !impact_triangle_init(&face, tri->vertex)) {
            continue;
        }
        shz_vec3_t n = support_normal(tri, &face, p->up);
        shz_vec3_t from = ground_from(p, tri, (shz_vec3_t){0});
        if (n.y <= 0.0F || p->up.y <= 0.0F || plane_dot(n, p->speed) >= 0.0F ||
            plane_dot(n, p->up) < p->turn_cosine ||
            njAbs(plane_dot(n, from)) > margin ||
            !impact_inside(&face, from, margin)) {
            continue;
        }
        for (int i = 0; i < 3; i++) {
            int j = (i + 1) % 3;
            shz_vec3_t a = shz_vec3_sub(face.v[i], from);
            shz_vec3_t b = shz_vec3_sub(face.v[j], from);
            if (njAbs(plane_dot(p->up, a)) > margin ||
                njAbs(plane_dot(p->up, b)) > margin ||
                plane_dot(p->up, shz_vec3_sub(face.v[(i + 2) % 3], from)) >=
                    -margin) {
                continue;
            }
            if (p->plane &&
                (plane_dot(source, n) < GROUND_PARALLEL ||
                 !terrain_same_plane(p->plane, source, tri, margin))) {
                return FALSE;
            }
            const struct world_object *owner = &world_objects[cursor.next - 1];
            if (!p->plane || ground_prefers(previous, p->owner, owner)) {
                p->plane = tri;
                p->owner = owner;
                source = n;
            }
            break;
        }
    }
    if (!p->plane) {
        return FALSE;
    }
    return ground_crest(p, source, (shz_vec3_t){0}, gravity, remaining,
                        previous) == CREST_FOLLOW;
}

/* Resolve the rest of a stride from the supported center already reached.
 * On an unturned floor the foot probe can still defer to stepping. */
LINKED_LAST(ground_wall_sweep)
static __attribute__((noinline)) enum wall_result
ground_wall_sweep(const struct ground_path *p, struct polygon_ball *region,
                  NJS_OBJECT *previous, Float clearance, Float remaining,
                  Bool turned, struct ground_step *step)
{
    struct box_arena *arena = arena_held();
    const struct world_object *owner = NULL;
    for (int i = 0; i < world_count; i++) {
        if (world_objects[i].flags & p->liquid_mask) {
            continue;
        }
        const struct collision_mesh *mesh =
            sweep_mesh(arena, world_objects[i].object);
        if (!mesh || mesh->out_of_polygons) {
            return WALL_DELEGATE;
        }
        if (world_objects[i].object == previous) {
            owner = &world_objects[i];
        }
    }
    if (!owner || world_count == WORLD_OBJECTS) {
        return WALL_DELEGATE;
    }
    shz_vec3_t origin = zam_load_vec3(&region->pos);
    struct stride_contact contact = {0};
    struct wall_motion motion = {
        .origin = origin,
        .whole = {whole_coordinate(origin.x), whole_coordinate(origin.y),
                  whole_coordinate(origin.z)},
        .at = shz_vec3_add(shz_vec3_sub(p->origin, origin), p->center),
        .velocity = p->speed,
        .radius = p->radius,
        .clearance = clearance,
        .floor_y = p->turn_cosine,
        .tolerance = p->epsilon,
        .remaining = remaining,
        .land = {p->plane, owner, owner, p->up},
        .can_land = TRUE,
        .stride = !turned,
        .liquid_mask = p->liquid_mask,
        .stride_contact = turned ? &contact : NULL};
    enum wall_result result = wall_travel(&motion, region);
    if (result == WALL_WIDEN && motion.rotation.owner) {
        wall_stop(&motion);
        result = WALL_SOLVED;
    }
    if (result != WALL_SOLVED) {
        return result;
    }
    if (!turned &&
        !(motion.stalled || motion.rotation.owner || motion.normal.x ||
          motion.normal.z || motion.push.x || motion.push.z)) {
        return WALL_DELEGATE;
    }
    shz_vec3_t up = motion.land.owner ? motion.land.up : p->up;
    step->moving_contact = motion.hit_moving_rim;
    step->owner = NULL;
    step->support = motion.land.owner ? motion.land.owner->object : NULL;
    step->flags = motion.land.owner ? motion.land.owner->flags : 0;
    if (rotating_support(&motion, &up)) {
        step->support = motion.rotation.motion.object;
        step->flags = motion.rotation.flags;
        step->owner = motion.rotation.owner;
    }
    shz_vec3_t feet = shz_vec3_sub(shz_vec3_add(origin, motion.at),
                                   shz_vec3_scale(up, p->radius));
    zam_store_vec3(&step->feet, feet);
    zam_store_vec3(&step->velocity, motion.velocity);
    zam_store_vec3(&step->wall, contact.inward > 0 ? contact.normal
                                : motion.stalled   ? (shz_vec3_t){0}
                                                   : motion.normal);
    zam_store_vec3(&step->normal, up);
    step->stalled = motion.stalled;
    step->turned_wall = turned;
    step->wall_impact = contact.inward;
    step->entry_speed2 = contact.speed2;
    zam_store_vec3(&step->wall_tangent, contact.tangent);
    return WALL_SOLVED;
}

LINKED_LAST(ground_wall_step)
static __attribute__((noinline)) Bool
ground_wall_step(const struct ground_path *p, struct polygon_ball *region,
                 NJS_OBJECT *previous, Float clearance, Float remaining,
                 Bool turned, struct ground_step *step)
{
    if (turned) {
        clearance = p->radius;
    } else {
        if (p->up.x || p->up.y <= 0 || p->up.z || clearance < p->radius) {
            return FALSE;
        }
        clearance = (clearance > p->radius + p->snap ? clearance
                                                     : p->radius + p->snap) +
                    p->epsilon;
    }
    enum wall_result result = ground_wall_sweep(p, region, previous, clearance,
                                                remaining, turned, step);
    if (turned && result == WALL_WIDEN) {
        struct polygon_ball wider = *region;
        wider.radius += 2.0F * p->radius;
        polygon_collect_near(&wider);
        result = ground_wall_sweep(p, &wider, previous, clearance, remaining,
                                   TRUE, step);
        polygon_collect_near(region);
    }
    return result == WALL_SOLVED;
}

LINKED_LAST(polygon_follow_ground)
Bool polygon_follow_ground(const struct polygon_ball *body,
                           NJS_OBJECT *previous, Float snap, Float turn_cosine,
                           Float gravity, Float clearance, Uint32 liquid_mask,
                           struct ground_step *step)
{
    struct ground_path path = {.origin = zam_load_vec3(&body->pos),
                               .up = zam_load_vec3(&body->normal),
                               .speed = zam_load_vec3(&body->speed),
                               .radius = body->radius,
                               .snap = snap,
                               .liquid_mask = liquid_mask,
                               .turn_cosine = turn_cosine,
                               .epsilon =
                                   impact_tolerance(body->radius, body->speed)};
    /* The controller rebuilds its tangent through two truncated Ninja angles.
     * Each axis can leave up to one angle unit of outward velocity. */
    Float tangent_error =
        path.epsilon +
        (2.0F / ANGLE_PER_RADIAN) *
            (njAbs(path.speed.x) + njAbs(path.speed.y) + njAbs(path.speed.z));
    if (!(body->radius > 0.0F && turn_cosine > 0.0F && turn_cosine <= 1.0F &&
          snap >= 0.0F && gravity >= 0.0F &&
          njAbs(plane_dot(path.up, path.up) - 1.0F) <= 0.0002F) ||
        plane_dot(path.up, path.speed) > tangent_error) {
        return FALSE;
    }
    /* Snapping can move the center by snap plus the change in its radius
     * offset. The anchor accepts unit normals with dot product >= 0.9999. */
    shz_vec3_t original = shz_vec3_scale(path.up, path.radius);
    struct polygon_ball region;
    zam_store_vec3(&region.pos, shz_vec3_add(path.origin, original));
    region.speed = body->speed;
    region.radius = 1.025F * body->radius + 2.0F * snap + path.epsilon;
    if (!polygon_collect_near(&region) || world_count == WORLD_OBJECTS) {
        return FALSE;
    }
    for (int i = 0; i < world_count; i++) {
        if (world_objects[i].object == previous &&
            !(world_objects[i].flags & liquid_mask)) {
            if (!world_plain_land(&world_objects[i])) {
                return FALSE;
            }
            path.owner = &world_objects[i];
        }
    }
    if (!path.owner) {
        return FALSE;
    }
    Float remaining = 1.0F, last = remaining;
    const int limit = TERRAIN_PROGRESS_INTERVAL + 2 * near_polygon_count;
    Bool stalled = FALSE;
    const struct world_object *support;
    shz_vec3_t initial_up = path.up;
    Bool has_face = ground_anchor(&path, previous);
    shz_vec3_t difference =
        shz_vec3_scale(shz_vec3_sub(path.up, initial_up), path.radius);
    if (!has_face ||
        plane_dot(difference, difference) > path.epsilon * path.epsilon) {
        struct ground_path rim = path;
        rim.up = initial_up;
        Float budget = gravity;
        Float inward = plane_dot(rim.up, rim.speed);
        rim.speed = shz_vec3_sub(rim.speed, shz_vec3_scale(rim.up, inward));
        if (ground_resume_crest(&rim, &budget, &remaining)) {
            path = rim;
            gravity = budget;
            if (remaining == 0.0F) {
                support = path.owner;
                goto finish;
            }
            goto walk;
        }
        remaining = 1.0F;
    }
    if (!has_face ||
        !ground_snap_clear(
            &path, original,
            shz_vec3_sub(zam_load_vec3(path.plane->vertex), path.origin))) {
        return FALSE;
    }
    Float inward = plane_dot(path.speed, path.up);
    if (inward > tangent_error) {
        return FALSE;
    }
    path.speed = shz_vec3_sub(path.speed, shz_vec3_scale(path.up, inward));
    if (polygon_rotating) {
        struct rotating_hit hit;
        enum yaw_result result =
            rotating_first(shz_vec3_add(path.origin, path.center), path.speed,
                           path.radius, 0, 1, &hit);
        if (result != YAW_CLEAR) {
            if (result == YAW_TOUCH) {
                /* The original collector covers self-motion. The blade can
                 * carry the rest of the stride outside that neighborhood. */
                shz_vec3_t v = hit.contact.velocity;
                region.radius += 2.0F * njSqrt(plane_dot(v, v)) + path.radius;
                polygon_collect_near(&region);
            }
            /* A banked runner meets the blade with the body sphere even
             * before crossing another terrain face in this stride. */
            Bool banked = path.up.x || path.up.z || path.up.y <= 0.0F;
            return ground_wall_step(&path, &region, previous, clearance, 1.0F,
                                    banked, step);
        }
    }
walk:
    for (int turn = 0; turn < limit && terrain_advances(turn, remaining, &last);
         turn++) {
        shz_vec3_t move = shz_vec3_scale(path.speed, remaining);
        struct ground_event hit = {.time = 1.0F};
        struct ground_spans spans;
        spans.count = 0;
        spans.omitted = 1.0F;
        const struct world_object *end_owner = NULL;
        Bool turned = turn || remaining < 1.0F;
        if (!ground_segment(&path, move, &hit, &spans, &end_owner) ||
            hit.blocked) {
            if (!hit.tri || !world_plain_land(hit.owner) ||
                (turned ? njAbs(hit.up.y) >= path.turn_cosine
                        : hit.up.y != 0.0F)) {
                return FALSE;
            }
            goto wall;
        }
        if (!spans.count || spans.span[0].lo > 0.0F) {
            return FALSE;
        }
        Float support_end = spans.span[0].hi;
        shz_vec3_t foot =
            shz_vec3_sub(path.center, shz_vec3_scale(path.up, path.radius));
        if (support_end < hit.time && support_end >= spans.omitted) {
            support_end = ground_extend(&path, foot, move, support_end, NULL);
        }
        if (!hit.tri && support_end < 1.0F) {
            shz_vec3_t lip =
                shz_vec3_add(foot, shz_vec3_scale(move, support_end));
            Float left = remaining * (1.0F - support_end);
            enum crest_step crest =
                ground_crest(&path, path.up, lip, &gravity, &left, NULL);
            if (crest == CREST_SNAP) {
                return FALSE;
            }
            if (crest == CREST_FOLLOW) {
                remaining = left;
                if (remaining == 0.0F) {
                    support = path.owner;
                    goto finish;
                }
                continue;
            }
        }
        shz_vec3_t start = path.center;
        path.center = shz_vec3_add(start, shz_vec3_scale(move, hit.time));
        if (hit.tri) {
            if (support_end < hit.time || !ground_join(&path, &hit)) {
                if (!turned || njAbs(hit.up.y) >= path.turn_cosine ||
                    !world_plain_land(hit.owner)) {
                    return FALSE;
                }
                /* A wall can meet the turning angle without sharing an edge. */
                path.center = start;
                goto wall;
            }
            /* Rotate the cross-edge tangent into the new plane. The shared
             * edge component and the length of the velocity are preserved. */
            path.speed = ground_turn_speed(&path, hit.up);
            path.up = hit.up;
            path.plane = hit.tri;
            path.owner = hit.owner;
            remaining *= 1.0F - hit.time;
            continue;
        }
        support = support_end == 1.0F ? end_owner : NULL;
        goto finish;

    wall:
        if (ground_wall_step(&path, &region, path.owner->object, clearance,
                             remaining, turned, step)) {
            return TRUE;
        }
        if (!turned) {
            return FALSE;
        }
        break;
    }
    /* The last accepted face still supports the feet when a guard runs out. */
    support = path.owner;
    path.speed = (shz_vec3_t){0};
    stalled = TRUE;

finish:
    step->stalled = stalled;
    zam_store_vec3(
        &step->feet,
        shz_vec3_add(
            path.origin,
            shz_vec3_sub(path.center, shz_vec3_scale(path.up, path.radius))));
    zam_store_vec3(&step->velocity, path.speed);
    zam_store_vec3(&step->normal, path.up);
    step->wall = (NJS_VECTOR){0};
    step->moving_contact = FALSE;
    step->owner = NULL;
    step->turned_wall = FALSE;
    step->support = support ? support->object : NULL;
    step->flags = support ? support->flags : 0;
    return TRUE;
}

#endif

#ifdef NONMATCHING
#ifdef DREAMCAST
#define POLYGON_SH4_SWEEP
#endif
#endif

#ifdef POLYGON_SH4_SWEEP
/* The caller seeds the sum with the first contact. */
LINKED_LAST(sweep_average)
static __attribute__((noinline)) void sweep_average(struct polygon_hit *out,
                                                    int count, Float weight)
{
    for (int i = 1; i < count; i++) {
        const struct polygon_hit *hit = &polygon_contacts[i].hit;
        out->point.x += hit->point.x;
        out->point.y += hit->point.y;
        out->point.z += hit->point.z;
        out->pos.x += hit->pos.x;
        out->pos.y += hit->pos.y;
        out->pos.z += hit->pos.z;
    }
    out->point.x *= weight;
    out->point.y *= weight;
    out->point.z *= weight;
    out->pos.x *= weight;
    out->pos.y *= weight;
    out->pos.z *= weight;
}

/* The sweep kernel starts at this loop. It turns XMTRX into each triangle's
 * frame and restores the caller even when the contact list fills. */
__attribute__((section(".text.polygon_sweep"))) Bool polygon_sweep_for(
    struct polygon_ball *ball, NJS_OBJECT *object, enum polygon_sweep_mode mode)
{
    struct box_arena *arena;
    struct collision_mesh *mesh;
    struct near_cursor cursor;
    struct sweep_bounds box;
    struct polygon *polygon;
    shz_mat4x4_t caller;
    Bool hit;
    Uint32 two_sided;
    int count;
    int nearest;
    int i;
    Float reach;
    Float length;
    Bool test_crossing;
    Float radius;
    Float height;
    Float x0;
    Float y0;
    Float z0;
    NJS_POINT3 from;
    NJS_VECTOR moved;
    NJS_POINT3 tri[3];
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    NJS_VECTOR normal;

    reach = ball->radius;
    if (reach <= 0.0F) {
        return FALSE;
    }
    touch_polygon_count = 0;
    arena = arena_held();
    mesh = sweep_mesh(arena, object);
    if (mesh == NULL || mesh->near_count == 0) {
        return FALSE;
    }
    length = njScalor(&ball->speed);
    test_crossing =
        length > (mode == POLYGON_SWEEP_SAMPLES ? 2.0F * ball->radius : 0.0F);
    from = ball->pos;
    moved = ball->speed;
    shz_xmtrx_store_4x4(&caller);
    tri[0].x = tri[0].y = tri[0].z = 0.0F;
    sweep_bounds(&box, ball);
    cursor = (struct near_cursor){.group = mesh->near, .back = mesh->near_back};
    for (count = mesh->near_count; count > 0; count--) {
        polygon = near_next(&cursor);
        if (sweep_box_misses(&box, polygon->vertex)) {
            continue;
        }
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
        shz_xmtrx_load_4x4((const shz_mat4x4_t *)&_unit_matrix_);
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
        hit = sweep_samples(tri, &pos, &speed, reach, length);
        if (!hit && test_crossing) {
            hit = sweep_crosses_face(polygon, ball, mode);
        }
        if (hit && sweep_touch(polygon, &pos, &speed, &normal, radius,
                               two_sided, &caller)) {
            break;
        }
    }
    shz_xmtrx_load_4x4(&caller);
    count = touch_polygon_count;
    if (count < 1) {
        return FALSE;
    }
    nearest = 0;
    height = polygon_contacts[0].height_after;
    for (i = 1; i < count; i++) {
        if (height > polygon_contacts[i].height_after) {
            height = polygon_contacts[i].height_after;
            nearest = i;
        }
    }
    ball->count = count;
    ball->hit_index = nearest;
    ball->hit = polygon_contacts[nearest].hit;
    if (count > 1) {
        Float weight = 1.0F / (Float)count;
        ball->hit.point = polygon_contacts[0].hit.point;
        ball->hit.pos = polygon_contacts[0].hit.pos;
        sweep_average(&ball->hit, count, weight);
    }
    return TRUE;
}
#else

/* polygon_sweep's WEIGH uses give &tri[1] and &tri[2] R9 and R8 (0x8c02a5dc
 * to 0x8c02a5e4), and x0 and y0 FR4 and FR5 (0x8c02a630, 0x8c02a636).
 * without them all, each pair trades and 69 of 1121 words differ. without
 * any one of them alone, 11 to 197 words differ */
#ifdef NONMATCHING
Bool polygon_sweep_for(struct polygon_ball *ball, NJS_OBJECT *object,
                       enum polygon_sweep_mode mode)
#else
Bool polygon_sweep(struct polygon_ball *ball, NJS_OBJECT *object, Uint32 flags)
#endif
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
#ifdef NONMATCHING
    Bool test_crossing;
#endif
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
#ifdef NONMATCHING
    test_crossing =
        length > (mode == POLYGON_SWEEP_SAMPLES ? 2.0F * ball->radius : 0.0F);
#endif
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
#ifdef NONMATCHING
    if (mesh->near_count == 0) {
        return FALSE;
    }
    NJS_MATRIX caller;
    njGetMatrix(&caller);
    struct sweep_bounds box;
    sweep_bounds(&box, ball);
    struct near_cursor cursor = {.group = mesh->near, .back = mesh->near_back};
    struct polygon *candidate;
    near = &candidate;
    for (count = mesh->near_count; count > 0; count--) {
        candidate = near_next(&cursor);
        if (sweep_box_misses(&box, candidate->vertex)) {
            continue;
        }
#else
    for (count = mesh->near_count, near = mesh->near; count > 0;
         count--, near++) {
#endif
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
#ifdef NONMATCHING
        njSetMatrix(NULL, &_unit_matrix_);
#else
        njPushMatrix(&_unit_matrix_);
#endif
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
#ifdef NONMATCHING
        if (!hit && test_crossing) {
            hit = sweep_crosses_face(candidate, ball, mode);
        }
#endif
        if (hit) {
#ifdef NONMATCHING
            sweep_faces[touch_polygon_count] = candidate;
#endif
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
#ifdef NONMATCHING
                njSetMatrix(NULL, &caller);
#else
                njPopMatrix(1);
#endif
                njDrawTriangle3D(&touch_triangle, 3, NJD_FILL);
#ifdef NONMATCHING
                njSetMatrix(NULL, &touch_matrix);
#else
                njPushMatrix(&touch_matrix);
#endif
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
#ifdef NONMATCHING
    }
    njSetMatrix(NULL, &caller);
#else
        njPopMatrix(1);
        WEIGH(&njPrintC);
        WEIGH(&atan2f);
    }
#endif
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

#ifdef NONMATCHING
LINKED_LAST(polygon_sweep)
Bool polygon_sweep(struct polygon_ball *ball, NJS_OBJECT *object, Uint32 flags)
{
    return polygon_sweep_for(ball, object, POLYGON_SWEEP_MOTION);
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

#ifdef NONMATCHING
/* Height queries also visit models already in world space. */
static __attribute__((noinline)) Bool
height_strip_batch(struct mesh_strip *strip, const NJS_OBJECT *object)
{
    const NJS_OBJECT *unturned =
        (object->evalflags & EVAL_UNTURNED) == EVAL_UNTURNED ? object : NULL;
    return mesh_strip_batch(strip, unturned);
}
#endif

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
#ifdef NONMATCHING
    struct mesh_strip strip;
#else
    struct strip strip;
#endif
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
#ifdef NONMATCHING
        strip.left = 0;
        strip.strips = strips;
        while (height_strip_batch(&strip, object)) {
            reversed = strip.reversed;
#else
        for (; strips != 0; strips--) {
            if ((object->evalflags & EVAL_UNIT) == EVAL_UNIT) {
                reversed = strip_next_plain(&strip);
            } else if ((object->evalflags & EVAL_UNTURNED) == EVAL_UNTURNED) {
                reversed = strip_next_moved(&strip, object);
            } else {
                reversed = strip_next(&strip);
            }
#endif
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
