/* collision meshes on SH4ZAM. make dc compiles this in place of
 * src/collision/mesh.src. mesh.src says how a search walks its window, and
 * where its walks go wrong. each FTRV, FIPR and FSRRA comes from SH4ZAM or
 * ninja_zam.h and takes mesh.src's operands in their roles. each other step
 * is mesh.src's product, sum or test, in mesh.src's order */
#include <math.h>
#include <mathf.h>
#include <ninja.h>
#include <sh4zam/shz_mem.h>

#include "collision/mesh.h"
#include "collision/polygon.h"

/* mesh.h points each call of polygon_find_heights outside polygon.c at
 * polygon.c's indexed search. here the name is the shipped search's, which
 * the indexed one falls back to */
#undef polygon_find_heights

/* the buckets a new mesh's polygons sort into by z. each is a list's first
 * and last polygons, and takes 16 of z, 1 << BUCKET_BITS */
#define BUCKETS 128
#define BUCKET_BITS 4

/* a polygon is near the point when its center's squared distance is at most
 * its radius plus this. the radius is a square already, and this is 30
 * squared */
#define MESH_MARGIN 900.0F
/* Ninja angles in a radian, 65536 / 2 pi or 10430.38, cut to 10430 */
#define ANGLE_PER_RADIAN 10430.0F
/* a polygon is edge-on when its normal's y squared is no more than this,
 * 0.0001 or H'38D1B717. that y is twice the area of the polygon's projection
 * on x and z. the level divides by it */
#define EDGE_ON_Y2 0x1.a36e2ep-14F
/* how far below and above the point's y a height query's levels start */
#define LEVEL_START 220.0F

struct bucket {
    struct polygon *first;
    struct polygon *last;
};

/* these data link after 1ST_READ.BIN's other data. mesh.src keeps them in
 * its code. mesh_found says whether the search under way has taken a
 * surface, and the search returns it */
LINKED_LAST_DATA(free_polygons) static struct polygon *free_polygons;
LINKED_LAST_DATA(free_meshes) static struct collision_mesh *free_meshes;
LINKED_LAST_DATA(collision_mesh_list)
struct collision_mesh *collision_mesh_list;
LINKED_LAST_DATA(mesh_found) static Bool mesh_found;
LINKED_LAST_DATA(mesh_buckets)
static struct bucket mesh_buckets[BUCKETS] __attribute__((aligned(32)));

void polygon_pool_init(struct polygon *polygons, int count)
{
    Uint32 left = (Uint32)count - 1;

    free_polygons = polygons;
    do {
        polygons->next = polygons + 1;
        polygons++;
    } while (--left != 0);
    polygons->next = NULL;
}

struct polygon *collision_mesh_add_polygon(struct collision_mesh *mesh)
{
    struct polygon *polygon = free_polygons;
    struct polygon *rest = polygon->next;
    struct polygon *first;

    if (rest == NULL) {
        return NULL;
    }
    free_polygons = rest;
    first = mesh->polygons;
    polygon->next = first;
    if (first != NULL) {
        first->prev = polygon;
    } else {
        mesh->last = polygon;
    }
    mesh->polygons = polygon;
    polygon->prev = NULL;
    return polygon;
}

void collision_mesh_pool_init(struct collision_mesh *meshes, int count)
{
    Uint32 left = (Uint32)count - 1;

    free_meshes = meshes;
    do {
        meshes->next = meshes + 1;
        meshes++;
    } while (--left != 0);
    meshes->next = NULL;
    collision_mesh_list = NULL;
}

/* the first mesh's prev: collision_mesh_list's address, negated. mesh.src
 * tells it from a mesh by its sign, as a mesh's address in P1 is negative.
 * this compares it for equality, which gives the same on the SH-4 and reads
 * no address's sign */
#define LIST_MARK ((struct collision_mesh *)-(intptr_t)&collision_mesh_list)

struct collision_mesh *collision_mesh_alloc(void)
{
    struct collision_mesh *mesh = free_meshes;
    struct collision_mesh *rest = mesh->next;
    struct collision_mesh *first;

    if (rest == NULL) {
        return NULL;
    }
    free_meshes = rest;
    first = collision_mesh_list;
    mesh->next = first;
    if (first != NULL) {
        first->prev = mesh;
    }
    collision_mesh_list = mesh;
    mesh->prev = LIST_MARK;
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
    if (prev == LIST_MARK) {
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
    return zam_fipr_self(x, y, z, 0.0F);
}

Float mesh_point_distance2(NJS_POINT3 *from, NJS_POINT3 *to)
{
    return zam_squared_length(
        shz_vec3_init(from->x - to->x, from->y - to->y, from->z - to->z));
}

Float mesh_point_distance2_across(NJS_POINT3 *from, NJS_POINT3 *to)
{
    return zam_squared_length(
        shz_vec3_init(from->x - to->x, 0.0F, from->z - to->z));
}

/* one 8-byte store a bucket, by SH4ZAM, where mesh.src stores each long */
void collision_mesh_clear_buckets(void)
{
    shz_memset8(mesh_buckets, 0, sizeof(mesh_buckets));
}

struct polygon *polygon_alloc(Float z)
{
    Sint32 at = ((Sint32)z >> BUCKET_BITS) + BUCKETS / 2;
    struct bucket *bucket;
    struct polygon *polygon;
    struct polygon *rest;
    struct polygon *first;

    if (at < 0) {
        at = 0;
    }
    if (at > BUCKETS - 1) {
        at = BUCKETS - 1;
    }
    bucket = &mesh_buckets[at];
    polygon = free_polygons;
    rest = polygon->next;
    if (rest == NULL) {
        return NULL;
    }
    free_polygons = rest;
    first = bucket->first;
    polygon->next = first;
    if (first != NULL) {
        first->prev = polygon;
    } else {
        bucket->last = polygon;
    }
    bucket->first = polygon;
    polygon->prev = NULL;
    return polygon;
}

void collision_mesh_take_buckets(struct collision_mesh *mesh)
{
    struct bucket *bucket = mesh_buckets;
    struct bucket *end = mesh_buckets + BUCKETS;
    struct polygon *last = NULL;

    while (bucket < end && bucket->first == NULL) {
        bucket++;
    }
    if (bucket < end) {
        mesh->polygons = bucket->first;
        last = bucket->last;
        for (bucket++; bucket < end; bucket++) {
            if (bucket->first != NULL) {
                last->next = bucket->first;
                bucket->first->prev = last;
                last = bucket->last;
            }
        }
    }
    mesh->last = last;
}

/* the side of the line from a to b that (x, z) lies on, over x and z. the
 * sign gives the side, and 0 is on the line */
ALWAYS_INLINE Float edge_side(const NJS_POINT3 *a, const NJS_POINT3 *b, Float x,
                              Float z)
{
    Float across = b->x - a->x;
    Float along = b->z - a->z;

    return (-a->x + x) * along - (-a->z + z) * across;
}

/* TRUE where (x, z) lies inside the polygon's three sides, not on one */
ALWAYS_INLINE Bool inside(const struct polygon *polygon, Float x, Float z)
{
    const NJS_POINT3 *v = polygon->vertex;
    Float side_01 = edge_side(&v[0], &v[1], x, z);
    Float side_12 = edge_side(&v[1], &v[2], x, z);

    if (!(side_01 * side_12 > 0.0F)) {
        return FALSE;
    }
    return side_12 * edge_side(&v[2], &v[0], x, z) > 0.0F;
}

/* (b - a) x (c - a) for the polygon's points a, b and c */
ALWAYS_INLINE shz_vec3_t triangle_normal(const struct polygon *polygon)
{
    const NJS_POINT3 *v = polygon->vertex;
    Float ax = v[1].x - v[0].x;
    Float ay = v[1].y - v[0].y;
    Float az = v[1].z - v[0].z;
    Float bx = v[2].x - v[0].x;
    Float by = v[2].y - v[0].y;
    Float bz = v[2].z - v[0].z;

    return shz_vec3_init(ay * bz - az * by, az * bx - ax * bz,
                         ax * by - ay * bx);
}

/* the y of the polygon's plane at pos's x and z, where n is its normal */
ALWAYS_INLINE Float plane_y(const struct polygon *polygon, shz_vec3_t n,
                            const NJS_POINT3 *pos)
{
    Float offset = -zam_fipr(shz_vec3_vec4(n, 0.0F),
                             zam_load_vec4(&polygon->vertex[0], 0.0F));

    return -(pos->x * n.x + pos->z * n.z + offset) / n.y;
}

/* fill surface as mesh.h describes a surface the search takes */
ALWAYS_INLINE void take(struct surface *surface, Sint32 flags, Float level,
                        shz_vec3_t n)
{
    mesh_found = TRUE;
    surface->found = TRUE;
    surface->flags = flags;
    surface->level = level;
    n = shz_vec3_scale(n, _builtin_fsrra(zam_squared_length(n)));
    zam_store_vec3(&surface->normal, n);
    surface->ang_z = (Angle)(-((Float)atan2(n.x, n.y) * ANGLE_PER_RADIAN));
    surface->ang_x = (Angle)(asinf(surface->normal.z) * ANGLE_PER_RADIAN);
}

/* TRUE where the polygon is near pos, as MESH_MARGIN says */
ALWAYS_INLINE Bool polygon_near(const struct polygon *polygon,
                                const NJS_POINT3 *pos)
{
    Float reach = polygon->radius + MESH_MARGIN;
    Float x = polygon->center.x - pos->x;
    Float y;
    Float z;

    x = x * x;
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

/* test polygon against a floor query. a later level replaces a kept floor
 * by mesh.src's test, as its @bug tag says. that test reads n.y where the
 * point's y was meant, and leaves the kept level's difference signed */
static void test_floor(void *test_query, const struct polygon *polygon,
                       Sint32 flags)
{
    struct floor_query *query = test_query;
    shz_vec3_t n;
    Float level;

    if (!inside(polygon, query->pos.x, query->pos.z)) {
        return;
    }
    n = triangle_normal(polygon);
    if (!(n.y * n.y > EDGE_ON_Y2)) {
        return;
    }
    level = plane_y(polygon, n, &query->pos);
    if (query->floor.found &&
        !(query->floor.level - n.y > __builtin_fabsf(level - n.y))) {
        return;
    }
    take(&query->floor, flags, level, n);
}

/* a level over the point's y replaces above when it is no higher, or above
 * has none; one at or under replaces below when it is higher, or below has
 * none */
void mesh_test_heights(struct height_query *query,
                       const struct polygon *polygon, Sint32 flags)
{
    struct surface *surface;
    shz_vec3_t n;
    Float level;

    if (!inside(polygon, query->pos.x, query->pos.z)) {
        return;
    }
    n = triangle_normal(polygon);
    if (!(n.y * n.y > EDGE_ON_Y2)) {
        return;
    }
    level = plane_y(polygon, n, &query->pos);
    if (level > query->pos.y) {
        surface = &query->above;
        if (surface->found && level > surface->level) {
            return;
        }
    } else {
        surface = &query->below;
        if (surface->found && !(level > surface->level)) {
            return;
        }
    }
    take(surface, flags, level, n);
}

static void test_heights(void *query, const struct polygon *polygon,
                         Sint32 flags)
{
    mesh_test_heights(query, polygon, flags);
}

/* the mesh in collision_mesh_list whose object is object, or null */
ALWAYS_INLINE struct collision_mesh *mesh_of(NJS_OBJECT *object)
{
    struct collision_mesh *mesh;

    for (mesh = collision_mesh_list; mesh != NULL; mesh = mesh->next) {
        if (mesh->object == object) {
            break;
        }
    }
    return mesh;
}

/* a polygon's test against a query, for the mesh's world flags */
typedef void test_fn(void *query, const struct polygon *polygon, Sint32 flags);

/* test each polygon near pos, from polygon on, until a center at or past
 * high. with back set, walk back instead, until a center before low. pos
 * points at the search's own copy of the query's point. the copy stays in
 * registers across the tests, as mesh.src keeps it in FR8 to FR10 */
ALWAYS_INLINE void walk(struct polygon *polygon, Bool back, Float low,
                        Float high, const NJS_POINT3 *pos, test_fn *test,
                        void *query, Sint32 flags)
{
    do {
        if (back ? low > polygon->center.z : !(high > polygon->center.z)) {
            return;
        }
        if (polygon_near(polygon, pos)) {
            test(query, polygon, flags);
        }
        polygon = back ? polygon->prev : polygon->next;
    } while (polygon != NULL);
}

/* the skip to the window keeps mesh.src's, as its @bug tag says. it tests
 * the center of the polygon it has just left, not the one it moved to. so it
 * stops one past the first polygon at or past low, and that one goes
 * untested. where that one is the mesh's last, the skip and the walk read
 * through a null polygon */
Bool polygon_find_floor(struct floor_query *query, NJS_OBJECT *object,
                        Uint32 flags)
{
    struct collision_mesh *mesh;
    struct polygon *polygon;
    NJS_POINT3 at;
    Float low;
    Float high;
    Float center;
    Bool before;

    query->floor.found = FALSE;
    mesh = mesh_of(object);
    if (mesh == NULL) {
        return FALSE;
    }
    mesh_found = FALSE;
    polygon = mesh->polygons;
    at = query->pos;
    low = at.z - mesh->span;
    high = at.z + mesh->span;
    center = polygon->center.z;
    if (low > center) {
        if (!(mesh->last->center.z > high)) {
            walk(mesh->last, TRUE, low, high, &at, test_floor, query,
                 mesh->flags);
            return mesh_found;
        }
        do {
            polygon = polygon->next;
            before = low > center;
            center = polygon->center.z;
        } while (before);
    }
    walk(polygon, FALSE, low, high, &at, test_floor, query, mesh->flags);
    return mesh_found;
}

Bool polygon_find_heights(struct height_query *query, NJS_OBJECT *object,
                          Uint32 flags)
{
    struct collision_mesh *mesh = mesh_of(object);

    if (mesh == NULL) {
        query->below.found = FALSE;
        query->above.found = FALSE;
        return FALSE;
    }
    return mesh_find_heights(query, mesh);
}

Bool mesh_find_heights(struct height_query *query, struct collision_mesh *mesh)
{
    struct polygon *polygon;
    NJS_POINT3 at;
    Float low;
    Float high;

    query->below.found = FALSE;
    query->above.found = FALSE;
    mesh_found = FALSE;
    polygon = mesh->polygons;
    at = query->pos;
    low = at.z - mesh->span;
    high = at.z + mesh->span;
    query->below.level = -LEVEL_START + at.y;
    query->above.level = LEVEL_START + at.y;
    if (low > polygon->center.z) {
        if (!(mesh->last->center.z > high)) {
            walk(mesh->last, TRUE, low, high, &at, test_heights, query,
                 mesh->flags);
            return mesh_found;
        }
        do {
            polygon = polygon->next;
        } while (low > polygon->center.z);
    }
    walk(polygon, FALSE, low, high, &at, test_heights, query, mesh->flags);
    return mesh_found;
}

/* transform the point in place by XMTRX: one FTRV of (point, 1) */
void matrix_calc_point(NJS_POINT3 *point)
{
    zam_store_vec3(point,
                   shz_xmtrx_transform_vec4(zam_load_vec4(point, 1.0F)).xyz);
}
