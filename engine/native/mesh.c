/* mesh.src's pools and buckets in C, as mesh.h gives them. mesh.src's
 * searches run as asm2c.py translates them, so a polygon and a mesh keep the
 * SH-4's layout here. each pointer is 32 bits, as marks.py makes polygon.h's.
 *
 * the first mesh's prev holds collision_mesh_list's address, negated.
 * mesh.src tells that mark from a mesh by its sign, since a P1 address is
 * negative. a host's address can take either sign, so collision_mesh_free
 * compares prev with LIST_MARK itself */
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "native.h"

/* P32 is marks.py's mark for a 32-bit pointer. mingw's _mingw.h defines
 * __ptr32 as nothing, hiding clang's keyword. a host whose pointers hold 4
 * bytes marks none */
#undef __ptr32
#if UINTPTR_MAX == UINT32_MAX
#define P32
#else
#define P32 __ptr32 __uptr
#endif

typedef struct {
    float x, y, z;
} vector;

struct polygon {
    struct polygon *P32 next, *P32 prev;
    vector center;
    float radius;
    vector vertex[3];
    uint32_t attr;
};

/* polygon.h's struct collision_mesh, field for field, so link_pool steps
 * the game's array of them by its size */
struct collision_mesh {
    struct collision_mesh *P32 next, *P32 prev;
    void *P32 object;
    int32_t flags;
    struct polygon *P32 polygons, *P32 last;
    int32_t unk_18;
    struct polygon **P32 near;
    int32_t unk_20;
    int near_count;
    float span, unk_2c;
    vector center;
    int32_t unk_3c;
};

#define BUCKETS 128
#define BUCKET_BITS 4

struct collision_mesh *collision_mesh_list;
static struct polygon *free_polygons;
static struct collision_mesh *free_meshes;
static struct {
    struct polygon *P32 first, *P32 last;
} buckets[BUCKETS];

/* the land near two players far apart can ask more polygons than the pool
 * lends: Sonic's and Tails' in Lost World and Final Egg, and the racers' in
 * Tails' Ice Cap. collision_mesh_update then leaves one mesh short of its
 * model and each mesh after it empty. the searches read an empty mesh's
 * first polygon through null, and polygon_find_floor's skip can step past a
 * short mesh's last. the console reads the bottom of its memory there. a
 * host's read of null ends the game. so a mesh that leaves the pool none to
 * lend ends at end_polygon, and an empty one is end_polygon alone. its
 * center lies past every window's end, so a walk by centers stops at it.
 * its corners are one point, so a walk of the whole list takes no surface
 * from it. a mesh the pool served whole ends in null as before */
static struct polygon end_polygon = {.center = {INFINITY, INFINITY, INFINITY}};

#define LIST_MARK                                                              \
    ((struct collision_mesh * P32)(uint32_t)(-(intptr_t)&collision_mesh_list))

/* count records lie size bytes apart from first. each one's first word
 * links to the next */
static void *link_pool(void *first, int count, size_t size)
{
    char *record = first;

    for (; count > 1; count--, record += size) {
        *(uint32_t *)record = (uint32_t)(uintptr_t)(record + size);
    }
    *(uint32_t *)record = 0;
    return first;
}

void polygon_pool_init(struct polygon *polygons, int count)
{
    free_polygons = link_pool(polygons, count, sizeof *polygons);
}

void collision_mesh_pool_init(struct collision_mesh *meshes, int count)
{
    free_meshes = link_pool(meshes, count, sizeof *meshes);
    collision_mesh_list = NULL;
}

struct collision_mesh *collision_mesh_alloc(void)
{
    struct collision_mesh *mesh = free_meshes;

    if (!mesh->next) {
        return NULL;
    }
    free_meshes = mesh->next;
    mesh->next = collision_mesh_list;
    if (mesh->next) {
        mesh->next->prev = mesh;
    }
    collision_mesh_list = mesh;
    mesh->prev = LIST_MARK;
    return mesh;
}

void collision_mesh_free(struct collision_mesh *mesh)
{
    /* an empty mesh's last is end_polygon, which is no pool's */
    if (mesh->polygons && mesh->last && mesh->last != &end_polygon) {
        mesh->last->next = free_polygons;
        free_polygons = mesh->polygons;
    }
    mesh->object = NULL;
    if (mesh->next) {
        mesh->next->prev = mesh->prev;
    }
    if (mesh->prev == LIST_MARK) {
        collision_mesh_list = mesh->next;
    } else {
        mesh->prev->next = mesh->next;
    }
    mesh->next = free_meshes;
    free_meshes = mesh;
}

/* take a free polygon and put it first in the list from *first to *last */
static struct polygon *take_polygon(struct polygon * P32 * first,
                                    struct polygon * P32 * last)
{
    struct polygon *polygon = free_polygons;

    if (!polygon->next) {
        return NULL;
    }
    free_polygons = polygon->next;
    polygon->next = *first;
    if (*first) {
        (*first)->prev = polygon;
    } else {
        *last = polygon;
    }
    *first = polygon;
    polygon->prev = NULL;
    return polygon;
}

struct polygon *collision_mesh_add_polygon(struct collision_mesh *mesh)
{
    return take_polygon(&mesh->polygons, &mesh->last);
}

void collision_mesh_clear_buckets(void)
{
    for (int i = 0; i < BUCKETS; i++) {
        buckets[i].first = buckets[i].last = NULL;
    }
}

/* FTRC cuts z toward zero, and an arithmetic shift floors it to a bucket */
struct polygon *polygon_alloc(float z)
{
    int32_t bucket = ((int32_t)z >> BUCKET_BITS) + BUCKETS / 2;

    bucket = bucket < 0 ? 0 : bucket > BUCKETS - 1 ? BUCKETS - 1 : bucket;
    return take_polygon(&buckets[bucket].first, &buckets[bucket].last);
}

void collision_mesh_take_buckets(struct collision_mesh *mesh)
{
    struct polygon *last = NULL;
    int i = 0;

    while (i < BUCKETS && !buckets[i].first) {
        i++;
    }
    if (i < BUCKETS) {
        mesh->polygons = buckets[i].first;
        last = buckets[i].last;
        for (i++; i < BUCKETS; i++) {
            if (buckets[i].first) {
                last->next = buckets[i].first;
                buckets[i].first->prev = last;
                last = buckets[i].last;
            }
        }
    }
    mesh->last = last;
    if (!free_polygons->next) {
        if (last) {
            last->next = &end_polygon;
        } else {
            mesh->polygons = mesh->last = &end_polygon;
        }
    }
}
