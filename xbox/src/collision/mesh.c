/* collision meshes: their pools, the buckets that sort a new mesh's polygons
 * by z, and the searches for a mesh's surfaces at a point. make pc compiles
 * this in place of src/collision/mesh.src, whose comments give each search's
 * walk, and mesh.h each function's contract.
 *
 * the C runs mesh.src's float operations on its operands in its order. a
 * FIPR whose fourth operands mesh.src zeroes adds 0 * 0 here too, so a sum
 * of -0 comes out 0 as FIPR's does. FIPR, FSRRA and FTRV run the port's
 * models of them (xbox/include/sh4.h).
 *
 * the port differs from mesh.src in three ways, none of which a valid mesh
 * meets except the second, which the shipped game also meets:
 * - the first mesh in collision_mesh_list has for prev the list's address
 *   negated, as mesh.src writes it. mesh.src tells it from a mesh's address
 *   by its sign, since the console's RAM lies above 0x80000000. x86's lies
 *   below, so the C compares prev with that marker instead;
 * - where polygon_find_floor's skip, a shipped bug, runs past the mesh's
 *   last polygon to null, mesh.src walks on from address 0, which on the
 *   console reads the boot ROM. the C ends the search there;
 * - a search of a mesh with no polygons, which mesh.src begins by reading
 *   the centers at address 0, finds nothing */
#include <math.h>
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/mesh.h"
#include "collision/polygon.h"

/* the buckets a new mesh's polygons sort into by z, each 16 of z wide */
#define BUCKETS 128
#define BUCKET_BITS 4

struct bucket {
    struct polygon *first;
    struct polygon *last;
};

/* the first free polygon and the first free mesh */
static struct polygon *free_polygons;
static struct collision_mesh *free_meshes;
struct collision_mesh *collision_mesh_list;
/* whether the search under way has taken a surface, which it returns */
static Bool mesh_found;
static struct bucket mesh_buckets[BUCKETS];

/* a polygon is near the point when the squared distance from its center is
 * at most its radius, a square already, plus 30 squared */
#define MESH_MARGIN 900.0F
/* Ninja angles in a radian, 65536 / 2 pi cut to 10430 */
#define MESH_ANGLE_PER_RADIAN 10430.0F
/* how far below and above the point's y a height query's levels start */
#define MESH_LEVEL_START 220.0F

/* a polygon is edge-on when its normal's y, squared, is no more than
 * mesh.src's H'38D1B717, 0.0001 */
static Float mesh_edge_on_y2(void)
{
    union {
        Uint32 bits;
        Float value;
    } edge = {0x38d1b717};

    return edge.value;
}

/* the marker the first mesh in collision_mesh_list has for prev */
static struct collision_mesh *list_marker(void)
{
    return (struct collision_mesh *)(-(Sint32)(Uint32)&collision_mesh_list);
}

/* FTRC: f truncated toward 0, saturated to Sint32's range as the SH-4's
 * FTRC saturates */
static Sint32 ftrc(Float f)
{
    if (f >= 2147483648.0F || f != f) {
        return 0x7fffffff;
    }
    if (f < -2147483648.0F) {
        return (Sint32)0x80000000;
    }
    return (Sint32)f;
}

/* FIPR of (x, y, z, 0) with itself, its fourth operands zero */
static Float fipr3_self(Float x, Float y, Float z)
{
    return sh4_fipr_self(x, y, z, 0.0F);
}

/* NOLINTBEGIN(readability-identifier-length) */
void polygon_pool_init(struct polygon *polygons, int count)
{
    struct polygon *polygon = polygons;
    int left = count - 1;

    free_polygons = polygons;
    do {
        polygon->next = polygon + 1;
        polygon++;
    } while (--left != 0);
    polygon->next = NULL;
}

/* take the first free polygon, but never the free list's last. NULL when
 * that is all the list holds */
static struct polygon *take_polygon(void)
{
    struct polygon *polygon = free_polygons;

    if (polygon->next == NULL) {
        return NULL;
    }
    free_polygons = polygon->next;
    return polygon;
}

/* put polygon first in the list from *first to *last */
static void put_first(struct polygon *polygon, struct polygon **first,
                      struct polygon **last)
{
    struct polygon *was = *first;

    polygon->next = was;
    if (was != NULL) {
        was->prev = polygon;
    } else {
        *last = polygon;
    }
    *first = polygon;
    polygon->prev = NULL;
}

struct polygon *collision_mesh_add_polygon(struct collision_mesh *mesh)
{
    struct polygon *polygon = take_polygon();

    if (polygon != NULL) {
        put_first(polygon, &mesh->polygons, &mesh->last);
    }
    return polygon;
}

void collision_mesh_pool_init(struct collision_mesh *meshes, int count)
{
    struct collision_mesh *mesh = meshes;
    int left = count - 1;

    free_meshes = meshes;
    do {
        mesh->next = mesh + 1;
        mesh++;
    } while (--left != 0);
    mesh->next = NULL;
    collision_mesh_list = NULL;
}

struct collision_mesh *collision_mesh_alloc(void)
{
    struct collision_mesh *mesh = free_meshes;
    struct collision_mesh *was;

    if (mesh->next == NULL) {
        return NULL;
    }
    free_meshes = mesh->next;
    was = collision_mesh_list;
    mesh->next = was;
    if (was != NULL) {
        was->prev = mesh;
    }
    collision_mesh_list = mesh;
    mesh->prev = list_marker();
    return mesh;
}

/* take mesh out of collision_mesh_list, and put it first in the free list */
static void mesh_release(struct collision_mesh *mesh)
{
    struct collision_mesh *next = mesh->next;
    struct collision_mesh *prev = mesh->prev;

    if (next != NULL) {
        next->prev = prev;
    }
    if (prev == list_marker()) {
        collision_mesh_list = next;
    } else {
        prev->next = next;
    }
    mesh->next = free_meshes;
    free_meshes = mesh;
}

void collision_mesh_free(struct collision_mesh *mesh)
{
    struct polygon *first = mesh->polygons;
    struct polygon *last = mesh->last;

    if (first != NULL && last != NULL) {
        last->next = free_polygons;
        free_polygons = first;
    }
    mesh->object = NULL;
    mesh_release(mesh);
}

Float vector_length_squared(Float x, Float y, Float z)
{
    return fipr3_self(x, y, z);
}

Float mesh_point_distance2(NJS_POINT3 *from, NJS_POINT3 *to)
{
    return fipr3_self(from->x - to->x, from->y - to->y, from->z - to->z);
}

Float mesh_point_distance2_across(NJS_POINT3 *from, NJS_POINT3 *to)
{
    return fipr3_self(from->x - to->x, 0.0F, from->z - to->z);
}

void collision_mesh_clear_buckets(void)
{
    int i;

    for (i = 0; i < BUCKETS; i++) {
        mesh_buckets[i].first = NULL;
        mesh_buckets[i].last = NULL;
    }
}

struct polygon *polygon_alloc(Float z)
{
    Sint32 index = (ftrc(z) >> BUCKET_BITS) + BUCKETS / 2;
    struct bucket *bucket;
    struct polygon *polygon;

    if (index < 0) {
        index = 0;
    }
    if (index > BUCKETS - 1) {
        index = BUCKETS - 1;
    }
    bucket = &mesh_buckets[index];
    polygon = take_polygon();
    if (polygon != NULL) {
        put_first(polygon, &bucket->first, &bucket->last);
    }
    return polygon;
}

void collision_mesh_take_buckets(struct collision_mesh *mesh)
{
    struct polygon *tail = NULL;
    int i = 0;

    while (i < BUCKETS && mesh_buckets[i].first == NULL) {
        i++;
    }
    if (i < BUCKETS) {
        mesh->polygons = mesh_buckets[i].first;
        tail = mesh_buckets[i].last;
        for (i++; i < BUCKETS; i++) {
            struct polygon *first = mesh_buckets[i].first;

            if (first != NULL) {
                tail->next = first;
                first->prev = tail;
                tail = mesh_buckets[i].last;
            }
        }
    }
    mesh->last = tail;
}

/* (x - a.x)(b.z - a.z) - (z - a.z)(b.x - a.x): its sign says which side of
 * the line through a and b, across x and z, the point lies on */
static Float edge_side(const NJS_POINT3 *a, const NJS_POINT3 *b, Float x,
                       Float z)
{
    Float across_x = b->x - a->x;
    Float across_z = b->z - a->z;

    return (-a->x + x) * across_z - (-a->z + z) * across_x;
}

/* the polygon's surface at the point's x and z, as mesh.src's tests find
 * it: FALSE where the point lies outside or on a side, or the polygon is
 * edge-on. otherwise TRUE, with the level at x and z and the normal, not
 * yet a unit vector */
static Bool polygon_level(const struct polygon *polygon, Float x, Float z,
                          Float *level, NJS_VECTOR *normal)
{
    const NJS_POINT3 *a = &polygon->vertex[0];
    const NJS_POINT3 *b = &polygon->vertex[1];
    const NJS_POINT3 *c = &polygon->vertex[2];
    Float side = edge_side(a, b, x, z);
    Float next = edge_side(b, c, x, z);
    Float bx;
    Float by;
    Float bz;
    Float cx;
    Float cy;
    Float cz;
    Float d;

    if (!(side * next > 0.0F)) {
        return FALSE;
    }
    side = next;
    next = edge_side(c, a, x, z);
    if (!(side * next > 0.0F)) {
        return FALSE;
    }
    /* (b - a) x (c - a), as _mesh_triangle_normal forms it */
    bx = b->x - a->x;
    by = b->y - a->y;
    bz = b->z - a->z;
    cx = c->x - a->x;
    cy = c->y - a->y;
    cz = c->z - a->z;
    normal->x = by * cz - bz * cy;
    normal->y = bz * cx - bx * cz;
    normal->z = bx * cy - by * cx;
    if (!(normal->y * normal->y > mesh_edge_on_y2())) {
        return FALSE;
    }
    /* FIPR FV8,FV0 of the normal and a, the normal first, their fourth
     * operands zero */
    d = sh4_fipr(normal->x, normal->y, normal->z, 0.0F, a->x, a->y, a->z, 0.0F);
    *level = -((x * normal->x + z * normal->z) + -d) / normal->y;
    return TRUE;
}

/* take a surface: its found, flags, level and unit normal, then the angles
 * from atan2 and asinf of the normal */
static void take_surface(struct surface *surface, Sint32 flags, Float level,
                         const NJS_VECTOR *normal)
{
    Float scale = _builtin_fsrra(fipr3_self(normal->x, normal->y, normal->z));

    mesh_found = TRUE;
    surface->found = TRUE;
    surface->flags = flags;
    surface->level = level;
    surface->normal.x = normal->x * scale;
    surface->normal.y = normal->y * scale;
    surface->normal.z = normal->z * scale;
    surface->ang_z = ftrc(
        -((Float)atan2(surface->normal.x, surface->normal.y) * MESH_ANGLE_PER_RADIAN));
    surface->ang_x = ftrc(asinf(surface->normal.z) * MESH_ANGLE_PER_RADIAN);
}

/* a search's test of one polygon near the point */
typedef void (*mesh_test)(const struct polygon *polygon, void *query,
                          Sint32 flags);

static void test_floor(const struct polygon *polygon, void *query,
                       Sint32 flags)
{
    struct floor_query *floor_query = query;
    NJS_VECTOR normal;
    Float level;

    if (!polygon_level(polygon, floor_query->pos.x, floor_query->pos.z, &level,
                       &normal)) {
        return;
    }
    /* @bug logic: as mesh.src's: a later polygon replaces the floor when the
     * kept level less the normal's y is over the distance from the normal's
     * y to the new level. the kept level's difference keeps its sign */
    if (floor_query->floor.found != FALSE &&
        !(floor_query->floor.level - normal.y >
          __builtin_fabsf(level - normal.y))) {
        return;
    }
    take_surface(&floor_query->floor, flags, level, &normal);
}

static void test_heights(const struct polygon *polygon, void *query,
                         Sint32 flags)
{
    struct height_query *height_query = query;
    NJS_VECTOR normal;
    Float level;

    if (!polygon_level(polygon, height_query->pos.x, height_query->pos.z,
                       &level, &normal)) {
        return;
    }
    if (level > height_query->pos.y) {
        struct surface *above = &height_query->above;

        if (above->found != FALSE && level > above->level) {
            return;
        }
        take_surface(above, flags, level, &normal);
    } else {
        struct surface *below = &height_query->below;

        if (below->found != FALSE && !(level > below->level)) {
            return;
        }
        take_surface(below, flags, level, &normal);
    }
}

/* the first mesh in collision_mesh_list whose object is object, or null */
static struct collision_mesh *mesh_of(NJS_OBJECT *object)
{
    struct collision_mesh *mesh;

    for (mesh = collision_mesh_list; mesh != NULL; mesh = mesh->next) {
        if (mesh->object == object) {
            return mesh;
        }
    }
    return NULL;
}

/* whether the polygon's center is near pos: its squared distance at most
 * the polygon's radius, a square, plus the margin, summed as mesh.src sums
 * it, axis by axis */
static Bool polygon_near(const struct polygon *polygon, const NJS_POINT3 *pos)
{
    Float reach = polygon->radius + MESH_MARGIN;
    Float x = polygon->center.x - pos->x;
    Float y;
    Float z;

    x *= x;
    if (x > reach) {
        return FALSE;
    }
    y = polygon->center.y - pos->y;
    y = y * y + x;
    if (y > reach) {
        return FALSE;
    }
    z = polygon->center.z - pos->z;
    z = z * z + y;
    return !(z > reach);
}

/* walk mesh's window about pos as mesh.src's searches do, testing each
 * polygon near pos. buggy_skip takes polygon_find_floor's skip, which tests
 * the center of the polygon it leaves */
static void mesh_walk(struct collision_mesh *mesh, const NJS_POINT3 *pos,
                      Bool buggy_skip, mesh_test test, void *query)
{
    Float low = pos->z - mesh->span;
    Float high = pos->z + mesh->span;
    struct polygon *polygon = mesh->polygons;

    if (polygon == NULL) {
        return;
    }
    if (low > polygon->center.z) {
        struct polygon *last = mesh->last;

        if (!(last->center.z > high)) {
            /* backward from the last polygon */
            for (polygon = last; polygon != NULL; polygon = polygon->prev) {
                if (low > polygon->center.z) {
                    break;
                }
                if (polygon_near(polygon, pos)) {
                    test(polygon, query, mesh->flags);
                }
            }
            return;
        }
        if (buggy_skip) {
            /* @bug memory: as mesh.src's, the skip tests the center of the
             * polygon it has just left */
            Bool before;

            do {
                before = low > polygon->center.z;
                polygon = polygon->next;
                if (polygon == NULL) {
                    return;
                }
            } while (before);
        } else {
            do {
                polygon = polygon->next;
                if (polygon == NULL) {
                    return;
                }
            } while (low > polygon->center.z);
        }
    }
    /* forward */
    for (; polygon != NULL; polygon = polygon->next) {
        if (!(high > polygon->center.z)) {
            break;
        }
        if (polygon_near(polygon, pos)) {
            test(polygon, query, mesh->flags);
        }
    }
}

Bool polygon_find_floor(struct floor_query *query, NJS_OBJECT *object,
                        Uint32 flags)
{
    struct collision_mesh *mesh;

    (void)flags;
    query->floor.found = FALSE;
    mesh = mesh_of(object);
    if (mesh == NULL) {
        return FALSE;
    }
    mesh_found = FALSE;
    mesh_walk(mesh, &query->pos, TRUE, test_floor, query);
    return mesh_found;
}

Bool polygon_find_heights(struct height_query *query, NJS_OBJECT *object,
                          Uint32 flags)
{
    struct collision_mesh *mesh;

    (void)flags;
    query->below.found = FALSE;
    query->above.found = FALSE;
    mesh = mesh_of(object);
    if (mesh == NULL) {
        return FALSE;
    }
    mesh_found = FALSE;
    query->below.level = -MESH_LEVEL_START + query->pos.y;
    query->above.level = MESH_LEVEL_START + query->pos.y;
    mesh_walk(mesh, &query->pos, FALSE, test_heights, query);
    return mesh_found;
}

void matrix_calc_point(NJS_POINT3 *point)
{
    sh4_xmtrx_apply3(point, 1.0F, point);
}
/* NOLINTEND(readability-identifier-length) */
