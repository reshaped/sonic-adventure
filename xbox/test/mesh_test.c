/* collision/mesh.src against xbox/src/collision/mesh.c: the pools, the
 * buckets and both searches, with each pointer printed as the index of its
 * record, so that the two runs' addresses don't count.
 *
 * atan2 and asinf are stand-ins defined here, the same float arithmetic on
 * both runs: the test checks mesh.src's use of them, not the runtime's. a
 * search whose floor skip would run past the mesh's last polygon, where
 * mesh.src reads address 0 and the port stops, is left out */
#include "harness.h"

typedef struct {
    float x, y, z;
} Point;

struct polygon {
    struct polygon *next;
    struct polygon *prev;
    Point center;
    float radius;
    Point vertex[3];
    unsigned attr;
};

struct collision_mesh {
    struct collision_mesh *next;
    struct collision_mesh *prev;
    void *object;
    int flags;
    struct polygon *polygons;
    struct polygon *last;
    int unk_18;
    void *near;
    int unk_20;
    int near_count;
    float span;
    float unk_2c;
    Point center;
    int unk_3c;
};

struct surface {
    int found;
    int flags;
    int ang_x;
    int ang_z;
    float level;
    Point normal;
};

struct floor_query {
    Point pos;
    struct surface floor;
};

struct height_query {
    Point pos;
    struct surface below;
    struct surface above;
};

extern struct collision_mesh *collision_mesh_list;
void polygon_pool_init(struct polygon *polygons, int count);
struct polygon *collision_mesh_add_polygon(struct collision_mesh *mesh);
void collision_mesh_pool_init(struct collision_mesh *meshes, int count);
struct collision_mesh *collision_mesh_alloc(void);
void collision_mesh_free(struct collision_mesh *mesh);
float vector_length_squared(float x, float y, float z);
float mesh_point_distance2(Point *from, Point *to);
float mesh_point_distance2_across(Point *from, Point *to);
void collision_mesh_clear_buckets(void);
struct polygon *polygon_alloc(float z);
void collision_mesh_take_buckets(struct collision_mesh *mesh);
int polygon_find_floor(struct floor_query *query, void *object, unsigned flags);
int polygon_find_heights(struct height_query *query, void *object,
                         unsigned flags);
void matrix_calc_point(Point *point);

/* the stand-ins: deterministic, and in range for the angles' FTRC */
double atan2(double y, double x)
{
    return (float)y * 0.75F - (float)x * 0.5F;
}

float asinf(float x)
{
    return x * 0.875F + 0.0625F;
}


#define POLYGONS 400
#define MESHES 6

static struct polygon polygons[POLYGONS];
static struct collision_mesh meshes[MESHES];
static int objects[MESHES];

/* a pointer as its record: a polygon's index, 0x1000 plus a mesh's,
 * 0xfffffffe for the list's marker, 0xffffffff for null */
static unsigned record(const void *p)
{
    const char *at = p;

    if (p == 0) {
        return 0xffffffffu;
    }
    if ((unsigned)(unsigned long)p == (unsigned)-(int)(unsigned long)&collision_mesh_list) {
        return 0xfffffffeu;
    }
    if (at >= (const char *)polygons && at < (const char *)(polygons + POLYGONS)) {
        return (unsigned)((const struct polygon *)p - polygons);
    }
    if (at >= (const char *)meshes && at < (const char *)(meshes + MESHES)) {
        return 0x1000u + (unsigned)((const struct collision_mesh *)p - meshes);
    }
    if (at >= (const char *)objects && at < (const char *)(objects + MESHES)) {
        return 0x2000u + (unsigned)((const int *)p - objects);
    }
    return 0xfffffffdu;
}

static void put_meshes(const char *name, unsigned step)
{
    int i;

    harness_begin(name, step);
    harness_hex(record(collision_mesh_list));
    for (i = 0; i < MESHES; i++) {
        harness_hex(record(meshes[i].next));
        harness_hex(record(meshes[i].prev));
        harness_hex(record(meshes[i].object));
        harness_hex(record(meshes[i].polygons));
        harness_hex(record(meshes[i].last));
    }
    harness_end();
}

/* a mesh's list, first to last, then back */
static void put_list(const char *name, unsigned step,
                     const struct collision_mesh *mesh)
{
    const struct polygon *polygon;
    int count = 0;

    harness_begin(name, step);
    for (polygon = mesh->polygons; polygon != 0 && count < POLYGONS;
         polygon = polygon->next, count++) {
        harness_hex(record(polygon));
        harness_hex(record(polygon->prev));
    }
    harness_end();
}

static void pools(void)
{
    struct collision_mesh *taken[MESHES];
    int i;

    polygon_pool_init(polygons, 40);
    collision_mesh_pool_init(meshes, MESHES);
    for (i = 0; i < MESHES; i++) {
        taken[i] = collision_mesh_alloc();
        if (taken[i] != 0) {
            taken[i]->object = &objects[i];
        }
        harness_begin("alloc", (unsigned)i);
        harness_hex(record(taken[i]));
        harness_end();
    }
    put_meshes("meshes", 0);
    for (i = 0; i < 12; i++) {
        struct polygon *p = collision_mesh_add_polygon(taken[i % 3]);

        harness_begin("add", (unsigned)i);
        harness_hex(record(p));
        harness_end();
    }
    put_meshes("meshes", 1);
    collision_mesh_free(taken[2]); /* the middle of the list */
    put_meshes("meshes", 2);
    collision_mesh_free(taken[4]); /* the first */
    put_meshes("meshes", 3);
    collision_mesh_free(taken[0]); /* the last */
    put_meshes("meshes", 4);
    for (i = 0; i < 4; i++) {
        struct collision_mesh *mesh = collision_mesh_alloc();

        harness_begin("realloc", (unsigned)i);
        harness_hex(record(mesh));
        harness_end();
    }
    put_meshes("meshes", 5);
    /* run the polygons dry */
    for (i = 0; i < 40; i++) {
        struct polygon *p = collision_mesh_add_polygon(taken[1]);

        harness_begin("drain", (unsigned)i);
        harness_hex(record(p));
        harness_end();
    }
    put_list("list", 0, taken[1]);
}

/* a random triangle near (x, z), its center and radius as polygon.c forms
 * them closely enough */
static void triangle(struct polygon *p, float x, float z, float size)
{
    int k;
    float far = 0.0F;

    for (k = 0; k < 3; k++) {
        p->vertex[k].x = x + harness_value(size);
        p->vertex[k].y = harness_value(60.0F);
        p->vertex[k].z = z + harness_value(size);
    }
    if (harness_random() % 8 == 0) {
        p->vertex[2].y = p->vertex[0].y + 500.0F; /* a steep one */
    }
    p->center.x = (p->vertex[0].x + p->vertex[1].x + p->vertex[2].x) * 0.3333F;
    p->center.y = (p->vertex[0].y + p->vertex[1].y + p->vertex[2].y) * 0.3333F;
    p->center.z = (p->vertex[0].z + p->vertex[1].z + p->vertex[2].z) * 0.3333F;
    for (k = 0; k < 3; k++) {
        float dx = p->vertex[k].x - p->center.x;
        float dy = p->vertex[k].y - p->center.y;
        float dz = p->vertex[k].z - p->center.z;
        float d = dx * dx + dy * dy + dz * dz;

        if (d > far) {
            far = d;
        }
    }
    p->radius = far;
}

/* whether polygon_find_floor's skip would run past the last polygon */
static int skip_runs_off(const struct collision_mesh *mesh, float z)
{
    float low = z - mesh->span;
    float high = z + mesh->span;
    const struct polygon *p = mesh->polygons;
    int before;

    if (p == 0 || !(low > p->center.z) || !(mesh->last->center.z > high)) {
        return 0;
    }
    do {
        before = low > p->center.z;
        p = p->next;
        if (p == 0) {
            return 1;
        }
    } while (before);
    return 0;
}

static void put_surface(const struct surface *s)
{
    harness_hex((unsigned)s->found);
    harness_hex((unsigned)s->flags);
    harness_hex((unsigned)s->ang_x);
    harness_hex((unsigned)s->ang_z);
    harness_float(s->level);
    harness_float(s->normal.x);
    harness_float(s->normal.y);
    harness_float(s->normal.z);
}

/* every field of a surface, so that one a search leaves holds a known value */
static void blank(struct surface *s, float level)
{
    s->found = 0x5a5a;
    s->flags = 0x1111;
    s->ang_x = 0x2222;
    s->ang_z = 0x3333;
    s->level = level;
    s->normal.x = s->normal.y = s->normal.z = 7.0F;
}

static void searches(void)
{
    int round;

    for (round = 0; round < 40; round++) {
        struct collision_mesh *mesh;
        int count = 20 + (int)(harness_random() % 150);
        float size = (round % 2) ? 40.0F : 120.0F;
        float span = 0.0F;
        int i;

        polygon_pool_init(polygons, POLYGONS);
        collision_mesh_pool_init(meshes, MESHES);
        mesh = collision_mesh_alloc();
        mesh->object = &objects[1];
        mesh->flags = (int)harness_random();
        collision_mesh_clear_buckets();
        for (i = 0; i < count; i++) {
            float x = harness_value(300.0F);
            float z = harness_value(300.0F);
            struct polygon *p;
            float extent;

            if (round % 5 == 0) {
                z *= 8.0F; /* past the buckets' range */
            }
            p = polygon_alloc(z);
            if (p == 0) {
                break;
            }
            triangle(p, x, z, size);
            extent = p->vertex[0].z - p->vertex[1].z;
            if (extent < 0) {
                extent = -extent;
            }
            if (extent > span) {
                span = extent;
            }
        }
        collision_mesh_take_buckets(mesh);
        mesh->span = span;
        put_list("sorted", (unsigned)round, mesh);

        for (i = 0; i < 150; i++) {
            struct floor_query floor;
            struct height_query heights;
            int found;

            floor.pos.x = harness_value(320.0F);
            floor.pos.y = harness_value(80.0F);
            floor.pos.z = harness_value(320.0F) * ((round % 5 == 0) ? 8.0F : 1.0F);
            blank(&floor.floor, 1234.0F);
            heights.pos = floor.pos;
            blank(&heights.below, 5678.0F);
            blank(&heights.above, 5678.0F);
            harness_begin("floor", (unsigned)(round * 1000 + i));
            if (skip_runs_off(mesh, floor.pos.z)) {
                harness_hex(0xdeadu);
            } else {
                found = polygon_find_floor(&floor, &objects[1], 0);
                harness_hex((unsigned)found);
                put_surface(&floor.floor);
            }
            harness_end();
            found = polygon_find_heights(&heights, &objects[1], 0);
            harness_begin("heights", (unsigned)(round * 1000 + i));
            harness_hex((unsigned)found);
            put_surface(&heights.below);
            put_surface(&heights.above);
            harness_end();
        }
        /* no mesh for the object */
        {
            struct height_query heights;

            heights.pos.x = heights.pos.y = heights.pos.z = 0.0F;
            blank(&heights.below, 1.0F);
            blank(&heights.above, 2.0F);
            harness_begin("nomesh", (unsigned)round);
            harness_hex((unsigned)polygon_find_heights(&heights, &objects[3], 0));
            put_surface(&heights.below);
            put_surface(&heights.above);
            harness_end();
        }
    }
}

static void small(void)
{
    static float matrix[16];
    unsigned i;
    int k;

    for (k = 0; k < 16; k++) {
        matrix[k] = harness_value(4.0F);
    }
    for (i = 0; i < 2000; i++) {
        Point a;
        Point b;

        a.x = harness_value(1000.0F);
        a.y = harness_value(1000.0F);
        a.z = harness_value(1000.0F);
        b.x = harness_value(1000.0F);
        b.y = harness_value(1000.0F);
        b.z = harness_value(1000.0F);
        harness_begin("small", i);
        harness_float(vector_length_squared(a.x, a.y, a.z));
        harness_float(mesh_point_distance2(&a, &b));
        harness_float(mesh_point_distance2_across(&a, &b));
        harness_load_xmtrx(matrix);
        matrix_calc_point(&a);
        harness_float(a.x);
        harness_float(a.y);
        harness_float(a.z);
        harness_end();
    }
}

int main(void)
{
    harness_fpu();
    pools();
    searches();
    small();
    return 0;
}
