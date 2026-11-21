/* exercise the translated searches with the native pool's 32-bit links */
#include <assert.h>

#include "../native/mesh.c"

void native_asm_init(void);
uintptr_t polygon_find_floor(uintptr_t, uintptr_t, uintptr_t);
uintptr_t polygon_find_heights(uintptr_t, uintptr_t, uintptr_t);

float native_atan2(float y, float x)
{
    return atan2f(y, x);
}

/* mesh.src writes eight words per surface after the query point */
static struct {
    vector pos;
    uint32_t below[8], above[8];
} query;
static struct polygon pool[4];
static struct collision_mesh meshes[2];

static void search(struct collision_mesh *mesh, float z, int hit)
{
    query.pos = (vector){0, 1, z};
    assert(polygon_find_floor((uintptr_t)&query, (uintptr_t)mesh, 0) == hit);
    assert(query.below[0] == hit);
    assert(polygon_find_heights((uintptr_t)&query, (uintptr_t)mesh, 0) == hit);
    assert(query.below[0] == hit && query.above[0] == 0);
}

static void check_pool(int count)
{
    struct collision_mesh *mesh;
    struct polygon *a, *b;

    polygon_pool_init(pool, count);
    collision_mesh_pool_init(meshes, 2);
    for (int i = 0; i < 3; i++) {
        mesh = collision_mesh_alloc();
        mesh->object = mesh;
        mesh->span = 10;
        collision_mesh_clear_buckets();
        collision_mesh_take_buckets(mesh);
        search(mesh, 50, 0);
        if (i == 1) {
            a = collision_mesh_add_polygon(mesh);
            a->center.z = 100;
            b = collision_mesh_add_polygon(mesh);
            b->center.z = 0;
            if (count == 3) {
                assert(!collision_mesh_add_polygon(mesh));
            }
        } else {
            b = polygon_alloc(0);
            b->center.z = 0;
            a = polygon_alloc(100);
            a->center.z = 100;
            if (i == 2 && count == 3) {
                assert(!polygon_alloc(200));
            }
            collision_mesh_take_buckets(mesh);
        }
        assert(mesh->polygons == b && mesh->last == a);
        assert(a->next == &end_polygon);
        assert(a >= pool && a < pool + 2 && b >= pool && b < pool + 2 &&
               a != b);
        b->vertex[0] = (vector){-2, 0, -2};
        b->vertex[1] = (vector){0, 0, 2};
        b->vertex[2] = (vector){2, 0, -2};
        b->radius = 8;
        search(mesh, 50, 0);
        search(mesh, 0, 1);
        collision_mesh_free(mesh);
    }
    mesh = collision_mesh_alloc();
    collision_mesh_clear_buckets();
    collision_mesh_take_buckets(mesh);
    collision_mesh_free(mesh);
    assert(free_polygons >= pool && free_polygons < pool + 2);
    assert(end_polygon.next == NULL && end_polygon.prev == NULL);
}

static void mesh_capacity(void)
{
    static struct collision_mesh records[192];
    struct collision_mesh *mesh;

    collision_mesh_pool_init(records, 192);
    for (int i = 0; i < 192; i++) {
        assert(collision_mesh_alloc() == records + i);
    }
    assert(!collision_mesh_alloc());
    collision_mesh_free(records + 97);
    mesh = collision_mesh_alloc();
    assert(mesh == records + 97 && collision_mesh_list == mesh);
    assert(!collision_mesh_alloc());
}

int main(void)
{
    native_asm_init();
    mesh_capacity();
    check_pool(4);
    check_pool(3);
    return 0;
}
