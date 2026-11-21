/* exercise the Dreamcast collector, update and pool together */
#include <kamui.h>
#include <ninja.h>
#include <shinobi.h>
#include "collision/mesh.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "stage/stage.h"

struct world_object world_objects[WORLD_OBJECTS];
Sint16 world_count;
Sint8 debug_hidden = -1;
static NJS_OBJECT objects[384];
static NJS_MODEL model;
static NJS_MATRIX stack[8] __attribute__((aligned(8)));
NJS_MATRIX _unit_matrix_ = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
NJS_MATRIX *_nj_current_matrix_ptr_ = stack;
Sint32 _stack_size_ = 1, _max_stack_size_ = 8;
volatile int result;

void world_collect_near_players(void)
{
}
void world_collect_all(void)
{
}
void world_list_near(Float x, Float y, Float z, Float radius)
{
}
void njPrintColor(Uint32 color)
{
}
void syMallocStat(Uint32 *total, Uint32 *largest)
{
    *total = *largest = 0;
}
void *syMalloc(Uint32 bytes)
{
    return NULL;
}
KMSTATUS kmSetWaitVsyncCallback(PKMCALLBACKFUNC cb, void *argument)
{
    return KMSTATUS_SUCCESS;
}
void njPrintC(Int at, const char *s)
{
}
void njPrintD(Int at, Int value, Int digits)
{
}

static int update(int count, int first)
{
    struct collision_mesh *mesh;
    int live = 0;
    world_count = count;
    for (int i = 0; i < count; i++) {
        NJS_OBJECT *object = &objects[first + i];
        object->model = &model;
        object->scl[0] = object->scl[1] = object->scl[2] = 1;
        world_objects[i].object = object;
    }
    if (first == 192) {
        for (int i = 0; i < 192; i++) {
            world_objects[world_count++].object = &objects[i];
        }
    }
    collision_mesh_update();
    for (mesh = collision_mesh_list; mesh && live < 193; mesh = mesh->next) {
        if (mesh->object < objects + first ||
            mesh->object >= objects + first + count) {
            return 0;
        }
        live++;
    }
    return _stack_size_ == 1 && _nj_current_matrix_ptr_ == stack &&
           live == count;
}

static int check(void)
{
    static struct polygon polys[2];
    struct polygon_ball ball = {.radius = 2};
    struct collision_mesh *empty, *full;
    struct polygon *poly;
    collision_mesh_init();
    for (int count = 190; count <= 192; count++) {
        if (!update(count, 0)) {
            return 1;
        }
    }
    if (collision_mesh_alloc() || !update(192, 192) || !update(2, 0)) {
        return 2;
    }
    empty = collision_mesh_list;
    full = empty->next;
    polygon_pool_init(polys, 2);
    collision_mesh_clear_buckets();
    poly = polygon_alloc(0);
    if (!poly || polygon_alloc(0)) {
        return 3;
    }
    collision_mesh_take_buckets(full);
    collision_mesh_clear_buckets();
    collision_mesh_take_buckets(empty);
    empty->near = &poly;
    empty->near_count = 7;
    if (polygon_collect_near(&ball) != 1 || empty->near || empty->near_count ||
        full->near_count != 1 || *full->near != poly) {
        return 4;
    }
    collision_mesh_free(empty);
    collision_mesh_free(full);
    return polygon_alloc(0) != poly ? 5 : 0;
}

__attribute__((section(".text.boot"))) void start(void)
{
    result = check() + 1;
    for (;;) {
        __asm__ volatile("nop");
    }
}
