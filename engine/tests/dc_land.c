/* Exercise static land admission through the Dreamcast mesh and height query.
 */
#include <kamui.h>
#include <ninja.h>
#include <shinobi.h>
#include "collision/ground.h"
#include "collision/mesh.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "player/e102_effects.h"
#include "player/player.h"
#include "system/task.h"

/* The test ELF exposes the active table, which the game's land task installs.
 */
extern struct land_table *world_land;

static struct task_work leader, follower;
struct task_work *camera_work = &leader;
struct task_work *player_work[8] = {&leader, &follower};
static NJS_MATRIX stack[8] __attribute__((aligned(8)));
NJS_MATRIX _unit_matrix_ = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
NJS_MATRIX *_nj_current_matrix_ptr_ = stack;
Sint32 _stack_size_ = 1, _max_stack_size_ = 8;
Int _nj_matrix_flag_;
Sint8 debug_hidden = -1;
int _errno;
volatile int result;

static unsigned char heap[16384] __attribute__((aligned(32)));
static Bool heap_enabled = TRUE, heap_busy;
static int heap_requests, heap_frees;
#ifdef DREAMCAST
static unsigned char boxes[32768] __attribute__((aligned(32)));
static Bool index_enabled;
static PKMCALLBACKFUNC index_step;
#endif

void *syMalloc(Uint32 size)
{
#ifdef DREAMCAST
    if (index_enabled && size == sizeof boxes) {
        return boxes;
    }
#endif
    heap_requests++;
    if (!heap_enabled || heap_busy || size > sizeof heap) {
        return NULL;
    }
    heap_busy = TRUE;
    return heap;
}

void syFree(void *ptr)
{
    if (heap_busy && ptr == heap) {
        heap_busy = FALSE;
        heap_frees++;
    }
}

void syMallocStat(Uint32 *total, Uint32 *largest)
{
    *total = *largest = 0;
#ifdef DREAMCAST
    if (index_enabled) {
        *total = *largest = sizeof boxes;
    }
#endif
}

void njPrintC(Int at, const char *text)
{
}

void njPrintD(Int at, Int value, Int digits)
{
}

void njPrintColor(Uint32 color)
{
}

KMSTATUS kmSetWaitVsyncCallback(PKMCALLBACKFUNC callback, void *argument)
{
#ifdef DREAMCAST
    index_step = callback;
#endif
    return KMSTATUS_SUCCESS;
}

static NJS_POINT3 points[] = {{-6, 0, -5}, {0, 0, 6}, {6, 0, -5}};
static Sint16 indices[] = {0, 1, 2};
static NJS_MESHSET meshset = {
    .type_matId = NJD_MESHSET_3, .nbMesh = 1, .meshes = indices};
static NJS_MODEL model = {.points = points,
                          .nbPoint = 3,
                          .meshsets = &meshset,
                          .nbMeshset = 1,
                          .r = 8};
static NJS_OBJECT objects[WORLD_OBJECTS];
static struct land_entry entries[WORLD_OBJECTS];
static struct land_table land = {.entries = entries};

static void place(int i, NJS_POINT3 center)
{
    objects[i] = (NJS_OBJECT){.model = &model,
                              .pos = {center.x, center.y, center.z},
                              .scl = {1, 1, 1}};
    entries[i] = (struct land_entry){.center = center,
                                     .radius = 8,
                                     .object = objects + i,
                                     .flags = WORLD_SOLID};
}

/* Both the world snapshot and the polygon mesh must contain this floor. */
static Bool floor_below(const NJS_POINT3 *pos, int index, Float level)
{
    struct height_query query = {.pos = *pos};
    world_list_near(pos->x, pos->y, pos->z, 5.0F);
    return world_count == 1 && world_objects[0].object == &objects[index] &&
           polygon_find_heights(&query, world_objects[0].object,
                                world_objects[0].flags) &&
           query.below.found && query.below.level == level;
}

/* The first 128 spheres surround the leader. Only the last floor is in
 * reach of the follower. Sky Deck can arrange its two players this way. */
static int two_anchors(void)
{
    leader.pos = (NJS_POINT3){0, 1, 0};
    follower.pos = (NJS_POINT3){550, 25, 0};
    world_land = &land;
    world_enable_land();
    world_set_near_radius(400.0F);
    land.count = 129;
    place(0, (NJS_POINT3){0, 0, 0});
    for (int i = 1; i < land.count - 1; i++) {
        place(i, (NJS_POINT3){200.0F + i, 0, 0});
    }
    place(128, (NJS_POINT3){550, 24, 0});

    collision_mesh_init();
    player_work[1] = NULL;
    collision_mesh_update();
    if (world_count != 128 || !floor_below(&leader.pos, 0, 0)) {
        return 1;
    }
    player_work[1] = &follower;
    collision_mesh_update();
    if (!floor_below(&leader.pos, 0, 0) ||
        !floor_below(&follower.pos, 128, 24)) {
        return 2;
    }
    return 0;
}

/* Called by the land task when its active act is released. */
extern void world_clear_land_cache(void);

static int pressure(void)
{
    const int last = 204;
    int requests = heap_requests, frees = heap_frees;
    land.count = last + 1;
    for (int i = 1; i < last; i++) {
        place(i, (NJS_POINT3){200.0F + (i % 100), 0, 0});
    }
    place(last, (NJS_POINT3){550, 24, 0});
    collision_mesh_update();
    if (!floor_below(&leader.pos, 0, 0) ||
        !floor_below(&follower.pos, last, 24) ||
        heap_requests != requests + 1 || !heap_busy) {
        return 3;
    }
    /* The mesh allocator takes the local floors first. Ordinary queries
     * still see the full snapshot in the land table's original order. */
    world_list_all();
    if (world_count != land.count) {
        return 4;
    }
    for (int i = 0; i < world_count; i++) {
        if (world_objects[i].object != &objects[i]) {
            return 5;
        }
    }
    entries[last].flags = 0;
    entries[last].object = &objects[0];
    entries[last].center.x = -900;
    entries[last].radius = 0;
    if (!floor_below(&follower.pos, last, 24)) {
        return 6;
    }
    world_collect_near_players();
    world_list_near(follower.pos.x, follower.pos.y, follower.pos.z, 5);
    if (world_count != 0 || heap_requests != requests + 1) {
        return 7;
    }
    world_clear_land_cache();
    world_list_all();
    if (world_count != 0 || heap_busy || heap_frees != frees + 1) {
        return 8;
    }
    /* Allocation failure must still leave both nearby players on a complete
     * floor when distant objects exceed the native mesh pool. */
    place(last, (NJS_POINT3){550, 24, 0});
    heap_enabled = FALSE;
    collision_mesh_update();
    if (world_count >= land.count || !floor_below(&leader.pos, 0, 0) ||
        !floor_below(&follower.pos, last, 24) || heap_busy) {
        return 9;
    }
    world_list_all();
    for (int i = 1; i < world_count; i++) {
        if (world_objects[i].object <= world_objects[i - 1].object) {
            return 10;
        }
    }
    return 0;
}

/* Repeated land entries share one mesh and one polygon budget. */
static struct collision_mesh *planned_mesh(int index)
{
    struct collision_mesh *found = NULL;
    for (struct collision_mesh *mesh = collision_mesh_list; mesh;
         mesh = mesh->next) {
        if (mesh->object == &objects[index]) {
            if (found) {
                return NULL;
            }
            found = mesh;
        }
    }
    return found;
}

/* Native lists end at a shared infinite-center polygon. Dreamcast uses NULL. */
static Bool planner_end(const struct polygon *face)
{
    return !face || (!face->next && face->center.z == __builtin_inff());
}

/* A deferred mesh owns no faces. */
static Bool planned_faces(int index, int count)
{
    struct collision_mesh *mesh = planned_mesh(index);
    if (!mesh) {
        return FALSE;
    }
    if (count == 0) {
        return mesh->out_of_polygons && mesh->polygons == mesh->last &&
               planner_end(mesh->polygons);
    }
    if (mesh->out_of_polygons) {
        return FALSE;
    }
    struct polygon *face = mesh->polygons;
    while (face && --count) {
        face = face->next;
    }
    return !planner_end(face) && face == mesh->last && planner_end(face->next);
}

static int planner_test(Sint32 motion)
{
    static struct polygon pool[5];
    static Sint16 repeated[] = {0, 1, 2, 0, 1, 2, 0, 1, 2};
    static NJS_MESHSET filler_set = {
        .type_matId = NJD_MESHSET_3, .nbMesh = 3, .meshes = repeated};
    static NJS_MODEL filler;
    static NJS_POINT3 levels[] = {{-6, 0, -5}, {0, 0, 6}, {6, 0, -5},
                                  {-6, 6, -5}, {0, 6, 6}, {6, 6, -5}};
    static Sint16 level_ids[] = {0, 1, 2, 3, 4, 5};
    static NJS_MESHSET level_set = {
        .type_matId = NJD_MESHSET_3, .nbMesh = 2, .meshes = level_ids};
    static NJS_MODEL steps = {.points = levels,
                              .nbPoint = 6,
                              .meshsets = &level_set,
                              .nbMeshset = 1,
                              .r = 12};
    static struct task_work task_work;
    static struct task mover;

    mover.work = &task_work;
    filler = model;
    filler.meshsets = &filler_set;
    land.count = 3;
    place(0, (NJS_POINT3){300, 0, 0});
    objects[0].model = &filler;
    place(1, (NJS_POINT3){0, 0, 0});
    place(2, (NJS_POINT3){550, 24, 0});
    world_land = &land;
    world_enable_land();
    world_set_near_radius(400.0F);
    player_work[0] = &leader;
    player_work[1] = &follower;
    leader.pos = (NJS_POINT3){0, 1, 0};
    follower.pos = (NJS_POINT3){550, 25, 0};
    collision_mesh_init();
    polygon_pool_init(pool, 5);

    /* Four usable slots cannot hold the first model and both players' floors.
     */
    collision_mesh_update();
    if (!planned_faces(0, 0) || !planned_faces(1, 1) || !planned_faces(2, 1) ||
        !floor_below(&leader.pos, 1, 0) || !floor_below(&follower.pos, 2, 24)) {
        return 20;
    }
    /* Rebuilding this untextured face resets its material attribute to zero,
     * even if the allocator hands it the same address again. */
    struct polygon *floor = planned_mesh(1)->polygons;
    floor->attr = 0x123456;
    collision_mesh_update();
    if (!planned_faces(0, 0) || !planned_faces(1, 1) ||
        planned_mesh(1)->polygons != floor || floor->attr != 0x123456) {
        return 21;
    }

    entries[1].flags |= WORLD_GRIP_HIGH;
    collision_mesh_update();
    if (!planned_faces(1, 1) || planned_mesh(1)->flags != entries[1].flags) {
        return 27;
    }

    /* Drop the companion floor and reference the leader's floor twice. The
     * same four slots now fit both complete models. */
    land.count = 4;
    entries[2].flags = 0;
    entries[3] = entries[1];
    collision_mesh_update();
    NJS_POINT3 far = {300, 1, 0};
    if (planned_mesh(2) || !planned_faces(1, 1) || !planned_faces(0, 3) ||
        !floor_below(&far, 0, 0)) {
        return 22;
    }
    world_list_near(0, 1, 0, 5);
    if (world_count != 2 || world_objects[0].object != &objects[1] ||
        world_objects[1].object != &objects[1]) {
        return 23;
    }

    land.count = 3;
    entries[2].flags = WORLD_SOLID;
    place(3, (NJS_POINT3){32, 6, 0});
    world_add(WORLD_SOLID | motion, &mover, &objects[3]);
    collision_mesh_update();
    NJS_POINT3 before = {32, 7, 0};
    int result = 0;
    if (!planned_faces(0, 0) || !planned_faces(3, 1) ||
        !floor_below(&before, 3, 6)) {
        result = 24;
    } else {
        /* One moving root keeps its address while gaining a second floor.
         * All four local polygons fit and the distant mesh stays deferred. */
        objects[3].model = &steps;
        objects[3].pos[0] = -32;
        objects[3].pos[1] = 9;
        collision_mesh_update();
        NJS_POINT3 after = {-32, 16, 0};
        if (!planned_faces(0, 0) || !planned_faces(1, 1) ||
            !planned_faces(2, 1) || !planned_faces(3, 2) ||
            !floor_below(&after, 3, 15)) {
            result = 25;
        }
        world_list_near(before.x, before.y, before.z, 5);
        if (!result && world_count != 0) {
            result = 26;
        }
    }
    world_remove(&mover, &objects[3]);
    collision_mesh_init();
    return result;
}

/* Speed Highway act 3 land entry 35, relative to (-1540, 100, -860).
 * The patrol spotlight can fall between its road and overhead faces. */
static int nearest_floors(void)
{
    static NJS_POINT3 pts[] = {{9.999878F, 33, 0}, {0, 33, 10}, {-20, 33, 10},
                               {0, 0, 10},         {10, 0, 0},  {-30, 0, 0},
                               {-8, 0, -38},       {0, 0, -20}, {8, 0, -38},
                               {-8, 0, 26},        {0, 0, 44},  {8, 0, 26}};
    static Sint16 stacked[] = {3, 4, 5, 0, 1, 2, 3, 4, 5};
    static Sint16 strip[] = {6, 7, 8, 3, 4, 5, 9, 10, 11};
    static NJS_MESHSET set = {.type_matId = NJD_MESHSET_3, .nbMesh = 2};
    static NJS_MODEL floors = {.points = pts,
                               .nbPoint = 12,
                               .meshsets = &set,
                               .nbMeshset = 1,
                               .r = 50};
    static NJS_MODEL upper;
    static NJS_MESHSET upper_set;
    static struct task_force force;
    static struct task_work work;
    static struct task mover = {.work = &work, .force = &force};
    const Float x = 3.34F, z = 3.57F;
    struct angles slope;
    leader.pos = (NJS_POINT3){x, 11.99F, z};
    land.count = 1;
    place(0, (NJS_POINT3){0, 0, 0});
    entries[0].radius = floors.r;
    objects[0].model = &floors;
    for (int order = 0; order < 2; order++) {
        set.meshes = stacked + 3 * order;
        collision_mesh_init();
        collision_mesh_update();
        if (ground_nearest(x, 11.99F, z, &slope) != 0 || slope.z != 0 ||
            ground_nearest_land(x, 11.99F, z, &slope) != 0 || slope.z != 0 ||
            ground_nearest(x, 16.5F, z, &slope) != 0 ||
            ground_nearest_land(x, 25, z, &slope) != 33) {
            return 30;
        }
    }

    /* Put the same floor and ceiling in different land entries. */
    upper_set = set;
    upper_set.meshes = stacked + 3;
    upper_set.nbMesh = set.nbMesh = 1;
    set.meshes = stacked;
    upper = floors;
    upper.meshsets = &upper_set;
    land.count = 2;
    place(1, (NJS_POINT3){0, 0, 0});
    entries[1].radius = upper.r;
    objects[1].model = &upper;
    collision_mesh_init();
    collision_mesh_update();
    if (ground_nearest(x, 11.99F, z, &slope) != 0 ||
        ground_nearest_land(x, 11.99F, z, &slope) != 0 ||
        ground_nearest(x, 16.5F, z, &slope) != 0) {
        return 31;
    }

    /* Only the first patch inside the Z window covers the query. */
    land.count = 1;
    set.meshes = strip;
    set.nbMesh = 3;
    collision_mesh_init();
    collision_mesh_update();
    if (ground_nearest(x, 11.99F, z, &slope) != 0 ||
        ground_nearest_land(x, 11.99F, z, &slope) != 0) {
        return 32;
    }

    /* The carrier shifts its levels to 20 and 53. The static level at 22
     * must then win for both height and material. */
    set.meshes = stacked;
    set.nbMesh = 2;
    land.count = 0;
    force.speed.y = 20;
    world_add(WORLD_WATER | WORLD_MOVES, &mover, &objects[0]);
    collision_mesh_init();
    collision_mesh_update();
    leader.pos.y = 25;
    Float carried = ground_nearest(x, 25, z, &slope);
    Float dry = ground_nearest_land(x, 25, z, &slope);
    Uint32 water = e102_floor_flags(&leader);
    int failure = 0;
    if (carried != 20 || dry != -GROUND_FAR || slope.x || slope.z ||
        !(water & WORLD_WATER)) {
        failure = 33;
    }

    land.count = 1;
    place(1, (NJS_POINT3){x, 22, z});
    entries[0] = entries[1];
    collision_mesh_update();
    if (njAbs(ground_nearest(x, 25, z, &slope) - 22.0F) > 0.0001F ||
        njAbs(ground_nearest_land(x, 25, z, &slope) - 22.0F) > 0.0001F ||
        e102_floor_flags(&leader) != WORLD_SOLID) {
        failure = 34;
    }
    force.speed.y = -20;
    if (ground_nearest(x, 0, z, &slope) != 13) {
        failure = 35;
    }
    mover.force = NULL;
    leader.pos.y = 33;
    world_list_near(x, 33, z, 5);
    if (!(e102_floor_flags(&leader) & WORLD_WATER)) {
        failure = 36;
    }
    world_remove(&mover, &objects[0]);
    return failure;
}

/* Centroids 2 and 12 share a 16-unit bucket. The outlying polygon
 * can precede the only floor in either direction of the height search. */
static int bucket_heights(void)
{
    enum { FACES = 18 };
    static NJS_POINT3 vertices[3 * FACES];
    static Sint16 triangles[3 * FACES];
    static NJS_MESHSET set = {
        .type_matId = NJD_MESHSET_3, .nbMesh = FACES, .meshes = triangles};
    static NJS_MODEL terrain = {.points = vertices,
                                .nbPoint = 3 * FACES,
                                .meshsets = &set,
                                .nbMeshset = 1,
                                .r = 80};
    land.count = 1;
    world_land = &land;
    world_enable_land();
    world_set_near_radius(400);
    player_work[1] = NULL;
#ifdef DREAMCAST
    collision_index_release();
    index_enabled = TRUE;
#endif
    for (int test = 0; test < 4; test++) {
        Bool back = test & 2;
        Float at = back ? 12 : 2;
        for (int i = 0; i < FACES; i++) {
            int face = (test & 1) ? FACES - 1 - i : i;
            Float x = face == 0 ? 0 : 4 + 3 * face;
            Float z = face == 0 ? at : back ? 2 : 12;
            if (back && face == FACES - 1) {
                z = -32;
            }
            vertices[3 * i] = (NJS_POINT3){x - 1, 4, z - 1};
            vertices[3 * i + 1] = (NJS_POINT3){x, 4, z + 2};
            vertices[3 * i + 2] = (NJS_POINT3){x + 1, 4, z - 1};
            for (int j = 0; j < 3; j++) {
                triangles[3 * i + j] = 3 * i + j;
            }
        }
        leader.pos = (NJS_POINT3){0, 5, at};
        place(0, (NJS_POINT3){0, 0, 0});
        objects[0].model = &terrain;
        entries[0].radius = terrain.r;
        world_clear_land_cache();
        collision_mesh_init();
        collision_mesh_update();
        struct angles slope;
        if (!planned_faces(0, FACES) || ground_below(0, 5, at, &slope) != 4 ||
            slope.x || slope.z) {
            return 40 + test;
        }
#ifdef DREAMCAST
        if (!index_step) {
            return 44;
        }
        for (int i = 0; i < 3 * FACES; i++) {
            index_step(NULL);
        }
        if (ground_below(0, 5, at, &slope) != 4) {
            return 45 + test;
        }
#endif
        /* Adding a polygon directly can place even a later bucket first. */
        struct polygon *extra = collision_mesh_add_polygon(planned_mesh(0));
        if (!extra) {
            return 49;
        }
        extra->center = (NJS_POINT3){16, 4, 64};
        extra->radius = 4;
        extra->vertex[0] = (NJS_POINT3){15, 4, 63};
        extra->vertex[1] = (NJS_POINT3){16, 4, 66};
        extra->vertex[2] = (NJS_POINT3){17, 4, 63};
        if (ground_below(0, 5, at, &slope) != 4 ||
            ground_below(0, 5, at + 2, &slope) != -GROUND_FAR) {
            return 50 + test;
        }
    }
#ifdef DREAMCAST
    collision_index_release();
    index_enabled = FALSE;
#endif
    return 0;
}

__attribute__((section(".text.boot"))) void start(void)
{
    njUnitMatrix(NULL);
    int check = two_anchors();
    if (!check) {
        check = pressure();
    }
    if (!check) {
        check = planner_test(WORLD_MOVES);
    }
    if (!check) {
        check = planner_test(0);
    }
    if (!check) {
        check = nearest_floors();
    }
    if (!check) {
        check = bucket_heights();
    }
    result = check + 1;
    for (;;) {
        __asm__ volatile("nop");
    }
}
