/* exercise the Dreamcast collector, update and pool together */
#include <kamui.h>
#include <ninja.h>
#include <shinobi.h>
#include "collision/arc.h"
#include "engine/tests/dc_yaw.h"
#include "collision/collision.h"
#include "collision/mesh.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "object/panel.h"
#include "player/e102.h"
#include "player/e102_effects.h"
#include "player/knuckles.h"
#include "player/player.h"
#include "player/sonic.h"
#include "player/tails.h"
#include "player_terrain.h"
#include "modules/stg03/ride.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

struct world_object world_objects[WORLD_OBJECTS];
Sint16 world_count;
NJS_VECTOR world_gravity;
/* The test ELF links the player's production contact classification. */
Bool player_sweep_world(struct polygon_ball *ball, NJS_OBJECT *object,
                        Uint32 flags, struct task_work *work);
int player_prepare_world(struct world_object *out, struct task_work *work,
                         struct task_motion *motion, struct player *record,
                         Bool pinball);
int knuckles_glide_collide(struct task_work *work, struct task_motion *motion,
                           struct player *record);
void knuckles_start_climb(struct task_work *work, struct task_motion *motion,
                          struct player *record);
Bool knuckles_wall_near(struct task_work *work, struct task_motion *motion,
                        struct player *record);
Uint32 player_collide_world(struct task_work *work, struct task_motion *motion,
                            struct player *record, Bool grab);
static Uint32 move_in_world(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    return player_collide_world(work, motion, record, FALSE);
}
extern struct character_constants character_constants[];
Bool tails_try_jump(struct task_work *work, struct player *record);
void e102_skate(struct task_work *work, struct task_motion *motion,
                struct player *record);
Bool e102_hover(struct task_work *work, struct task_motion *motion,
                struct player *record);
void car_carry(struct task *task);
void traffic_car_carry(struct task *task);
void egg_lift_move(struct task *task, struct task_work *work,
                   struct task_force *force);
struct task_work *camera_work;
struct pad_stick pad_sticks[8];
PDS_PERIPHERAL pad_records[8];
Uint32 pad_jump_buttons = PDD_DGT_TA;

int panel_jump_ready_or_idle(struct task_work *work)
{
    return 0;
}
void panel_jump(struct task_work *work)
{
}
int sound_play(int se, int id, int priority, int volume)
{
    return 0;
}
int sound_play_at(int se, int id, int priority, int volume, Float x, Float y,
                  Float z)
{
    return 0;
}
void sound_stop(int se)
{
}
void e102_hover_dust(struct task_work *work, Float ground)
{
}
Int _nj_matrix_flag_;
Sint8 debug_hidden = -1;
int _errno;
static NJS_OBJECT objects[384];
static NJS_MODEL model;
static NJS_MATRIX stack[8] __attribute__((aligned(8)));
NJS_MATRIX _unit_matrix_ = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
NJS_MATRIX *_nj_current_matrix_ptr_ = stack;
Sint32 _stack_size_ = 1, _max_stack_size_ = 8;
static NJS_MATRIX draw_matrix;
static int draws, wrong_matrix;
volatile int result;

Uint16 stage_get_stage_act(void)
{
    return STAGE_BEACH << 8;
}
Bool event_in_control(void)
{
    return FALSE;
}
Sint16 edit_mode;
Uint16 collision_enemy_count, collision_item_count;
int frame_redrawing;

static void install_player_terrain(NJS_OBJECT *object, Uint32 flags)
{
    collision_mesh_init();
    world_count = 1;
    world_objects[0] = (struct world_object){.object = object, .flags = flags};
    collision_mesh_update();
}

void draw_line_v(NJS_POINT3 *from, NJS_POINT3 *to)
{
    wrong_matrix = 1;
}
void njSimpleDrawObject(NJS_OBJECT *object)
{
    wrong_matrix = 1;
}

void draw_tree(NJS_OBJECT *object)
{
    wrong_matrix = 1;
}
void draw_line(Float x1, Float y1, Float z1, Float x2, Float y2, Float z2)
{
    wrong_matrix = 1;
}
void draw_set_material(Float a, Float r, Float g, Float b)
{
    wrong_matrix = 1;
}

void world_collect_near_players(void)
{
}
void world_collect_all(void)
{
}
void world_prioritize_meshes(void)
{
}
static struct land_entry rider_land[256];
static struct land_table rider_table = {.entries = rider_land};
const struct land_table *world_collision_land(void)
{
    return &rider_table;
}
static Float world_radius;
void world_list_near(Float x, Float y, Float z, Float radius)
{
    world_radius = radius;
}
void njPrintColor(Uint32 color)
{
}
void syMallocStat(Uint32 *total, Uint32 *largest)
{
    *total = *largest = 0;
}
static unsigned char contact_heap[16384] __attribute__((aligned(32)));
static Bool heap_enabled, heap_busy, heap_bad_free;
static int heap_requests, heap_frees;

void *syMalloc(Uint32 bytes)
{
    heap_requests++;
    if (!heap_enabled || heap_busy || bytes > sizeof contact_heap) {
        return NULL;
    }
    heap_busy = TRUE;
    return contact_heap;
}

void syFree(void *block)
{
    if (!heap_busy || block != contact_heap) {
        heap_bad_free = TRUE;
        return;
    }
    heap_busy = FALSE;
    heap_frees++;
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

void njDrawTriangle3D(NJS_POINT3COL *points, Int count, Uint32 flags)
{
    NJS_MATRIX current;
    njGetMatrix(&current);
    for (int i = 0; i < 16; i++) {
        if (draw_matrix[i] != current[i]) {
            wrong_matrix = 1;
        }
    }
    draws++;
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

/* The floor is narrower than half a radius. The shipped sweep steps over
 * its face even though the ball crosses it. */
static int cross_floor(void)
{
    static const struct {
        NJS_POINT3 from;
        NJS_VECTOR speed;
        Bool hit;
        Bool sampled_hit;
    } paths[] = {
        {{-0.25F, 1.25F, 0.25F}, {5, -4, 0}, TRUE, TRUE},
        {{0.375F, 1.25F, 0.25F}, {-5, -4, 0}, TRUE, TRUE},
        {{-0.25F, 0.75F, 0.25F}, {5, 0, 0}, FALSE, FALSE},
        {{0, 1, 0}, {0, 0, 0}, TRUE, TRUE},
        {{0.0625F, 1.25F, 0.25F}, {0, -2.5F, 0}, TRUE, TRUE},
        {{-0.25F, 1.5F, 0.25F}, {1, -1, 0}, FALSE, FALSE},
        {{0.25F, 0.75F, 0.25F}, {0, 0, 0}, FALSE, FALSE},
        {{-0.25F, 1.25F, 0.25F}, {5, 0, 0}, FALSE, FALSE},
        /* The sphere touches the floor at x = 1/16 between samples. */
        {{-0.1875F, 1.0625F, 0.25F}, {0.5F, -0.125F, 0}, TRUE, FALSE},
        {{-0.1875F, 1.125F, 0.25F}, {0.5F, -0.125F, 0}, FALSE, FALSE},
        {{0.0625F, 1.0625F, 0.25F}, {0, -0.125F, 0}, TRUE, TRUE},
    };
    struct collision_mesh *mesh;
    struct polygon *poly;
    struct polygon_ball ball = {.radius = 1};

    collision_mesh_init();
    if (!update(1, 0)) {
        return 1;
    }
    mesh = collision_mesh_list;
    collision_mesh_clear_buckets();
    poly = polygon_alloc(0);
    poly->vertex[0] = (NJS_POINT3){0, 0, 0};
    poly->vertex[1] = (NJS_POINT3){0, 0, 2};
    poly->vertex[2] = (NJS_POINT3){0.125F, 0, 0};
    poly->center = (NJS_POINT3){0.04F, 0, 0.67F};
    poly->radius = 2;
    collision_mesh_take_buckets(mesh);
    for (int i = 0; i < (int)(sizeof(paths) / sizeof(*paths)); i++) {
        ball.pos = paths[i].from;
        ball.speed = paths[i].speed;
        if (polygon_collect_near(&ball) != 1 ||
            polygon_sweep(&ball, mesh->object, 0) != paths[i].hit ||
            (paths[i].hit && (ball.count != 1 || ball.hit.normal.y < 0.99F)) ||
            polygon_sweep_for(&ball, mesh->object, POLYGON_SWEEP_SAMPLES) !=
                paths[i].sampled_hit ||
            _stack_size_ != 1) {
            return 2 + i;
        }
    }
    /* The player advances by hit.speed from hit.pos. A recovered floor
     * crossing must leave that destination a radius above the plane. */
    ball.pos = paths[0].from;
    ball.speed = paths[0].speed;
    if (!polygon_sweep(&ball, mesh->object, 0) ||
        njAbs(ball.hit.pos.y + ball.hit.speed.y - ball.radius) > 0.001F ||
        njAbs(ball.hit.speed.x - ball.speed.x) > 0.001F) {
        return 16;
    }
    /* Approach the back of a double-sided vertical face. */
    poly->vertex[1] = (NJS_POINT3){0, 2, 0};
    poly->attr = NJD_FLAG_DOUBLE_SIDE;
    ball.pos = (NJS_POINT3){-0.25F, 0.25F, 1.25F};
    ball.speed = (NJS_VECTOR){5, 0, -4};
    if (polygon_collect_near(&ball) != 1 ||
        !polygon_sweep(&ball, mesh->object, 0) || ball.hit.front.z > -0.99F ||
        ball.hit.normal.z < 0.99F) {
        return 10;
    }

    /* The center projects onto vertex 0 at 9/22 of the step. Rounding
     * that time before projecting puts the center outside the face. */
    poly->vertex[1] = (NJS_POINT3){-21, 0, -2};
    poly->vertex[2] = (NJS_POINT3){6, 0, 14};
    poly->center = (NJS_POINT3){-5, 0, 4};
    poly->radius = 320;
    mesh->span = 16;
    ball.radius = 2;
    ball.pos = (NJS_POINT3){-9, 11, -9};
    ball.speed = (NJS_VECTOR){22, -22, 22};
    poly->attr = 0;
    if (polygon_collect_near(&ball) != 1 ||
        !polygon_sweep(&ball, mesh->object, 0)) {
        return 11;
    }
    ball.pos.y = -11;
    ball.speed.y = 22;
    if (polygon_sweep(&ball, mesh->object, 0)) {
        return 12;
    }
    poly->attr = NJD_FLAG_DOUBLE_SIDE;
    if (!polygon_sweep(&ball, mesh->object, 0) || ball.hit.normal.y > -0.99F) {
        return 13;
    }
    /* The same boundary on an oblique face. Moving the ball a quarter
     * unit beyond the vertex must miss. Both endpoints clear the slab. */
    poly->vertex[1] = (NJS_POINT3){125, 300, -26};
    poly->vertex[2] = (NJS_POINT3){-40, -96, 0};
    poly->center = (NJS_POINT3){85.0F / 3, 68, -26.0F / 3};
    poly->radius = 70000;
    mesh->span = 26;
    poly->attr = 0;
    ball.pos = (NJS_POINT3){-152, 7, -52};
    ball.speed = (NJS_VECTOR){476, 196, 364};
    ball.radius = 91;
    if (polygon_collect_near(&ball) != 1 ||
        !polygon_sweep(&ball, mesh->object, 0)) {
        return 14;
    }
    ball.pos.z += 0.25F;
    if (polygon_collect_near(&ball) != 1 ||
        polygon_sweep(&ball, mesh->object, 0)) {
        return 15;
    }
    return 0;
}

/* A full contact list must return the caller's matrix, including when the
 * caller has already used the last Ninja stack slot. */
static int full_contacts(void)
{
    NJS_MATRIX after;
    struct polygon_ball ball = {.radius = 1, .pos = {0.25F, 0.5F, 0.25F}};
    struct collision_mesh *mesh;

    collision_mesh_init();
    if (!update(1, 0)) {
        return 1;
    }
    mesh = collision_mesh_list;
    collision_mesh_clear_buckets();
    for (int i = 0; i < POLYGON_CONTACTS; i++) {
        struct polygon *poly = polygon_alloc(0);
        poly->vertex[0] = (NJS_POINT3){0, 0, 0};
        poly->vertex[1] = (NJS_POINT3){0, 0, 2};
        poly->vertex[2] = (NJS_POINT3){2, 0, 0};
        poly->center = (NJS_POINT3){0.67F, 0, 0.67F};
        poly->radius = 3;
    }
    collision_mesh_take_buckets(mesh);
    njUnitMatrix(NULL);
    njScale(NULL, 2, 3, 4);
    njGetMatrix(&draw_matrix);
    debug_hidden = 0;
    for (int depth = 1; depth <= 8; depth += 7) {
        draws = 0;
        _stack_size_ = depth;
        _nj_current_matrix_ptr_ = &stack[depth - 1];
        if (polygon_collect_near(&ball) != POLYGON_CONTACTS ||
            !polygon_sweep(&ball, mesh->object, 0) ||
            ball.count != POLYGON_CONTACTS || _stack_size_ != depth ||
            _nj_current_matrix_ptr_ != &stack[depth - 1] || wrong_matrix ||
            draws != POLYGON_CONTACTS) {
            return 2;
        }
        njGetMatrix(&after);
        for (int i = 0; i < 16; i++) {
            if (draw_matrix[i] != after[i]) {
                return 3;
            }
        }
    }
    debug_hidden = -1;
    return 0;
}

/* A rotation with integer coefficients and uniform scale 9. Points also
 * move into stage coordinates so the world origin cannot hide an error. */
static NJS_VECTOR corner_transform(NJS_VECTOR v, int rotated, int point)
{
    if (!rotated) {
        return v;
    }
    return (NJS_VECTOR){v.x + 8 * v.y + 4 * v.z + 2048 * point,
                        8 * v.x + v.y - 4 * v.z - 1024 * point,
                        -4 * v.x + 4 * v.y - 7 * v.z + 3072 * point};
}

static int corner_near(float tolerance, NJS_VECTOR a, NJS_VECTOR b)
{
    return njAbs(a.x - b.x) <= tolerance && njAbs(a.y - b.y) <= tolerance &&
           njAbs(a.z - b.z) <= tolerance;
}

enum corner_face {
    FLOOR,
    X_WALL,
    Z_WALL,
    DIAGONAL,
    RAISED_FLOOR,
    CEILING,
    SHALLOW_WALL,
    BACK_WALL,
    LATER_WALL
};

static NJS_POINT3 corner_vertex(enum corner_face surface, NJS_POINT3 v)
{
    switch (surface) {
    case X_WALL:
        return (NJS_POINT3){0, v.z, v.x};
    case BACK_WALL:
        return (NJS_POINT3){0, v.z, -v.x};
    case LATER_WALL:
        return (NJS_POINT3){-1, v.z, v.x};
    case Z_WALL:
        return (NJS_POINT3){v.z, v.x, 0};
    case DIAGONAL:
        /* The wall's normal is (4/5, 3/5, 0). */
        return (NJS_POINT3){0.6F * v.x + 0.5F, -0.8F * v.x, v.z};
    case SHALLOW_WALL: {
        /* A unit sphere centered at (1, 1, 0.5) touches both walls.
         * Ignoring the 1:512 tilt permits a slide into the tilted wall. */
        const Float tilt = 1.0F / 512;
        Float x = 1.0F - __builtin_sqrtf(1.0F + tilt * tilt);
        return (NJS_POINT3){x - tilt * v.x, v.z, v.x + 0.5F};
    }
    case RAISED_FLOOR:
        v.y = 0.25F;
        break;
    case CEILING:
        v.x = -v.x;
        v.y = 1;
        break;
    case FLOOR:
        break;
    }
    return v;
}

static int check_player_corner(struct polygon_ball ball, NJS_OBJECT *object,
                               int rotated)
{
    /* The same two faces are both walkable on a banked support
     * frame, and form a blocking wall for an upright player. */
    struct task_work work = {0};
    Float scale = rotated ? 1.0F / 9.0F : 1.0F;
    world_gravity = corner_transform((NJS_VECTOR){0, -scale, 0}, rotated, 0);
    ball.normal = corner_transform(
        (NJS_VECTOR){0.70710677F * scale, 0.70710677F * scale, 0}, rotated, 0);
    NJS_POINT3 resolved = ball.hit.pos;
    NJS_VECTOR allowed = ball.hit.speed;
    if (!polygon_sweep(&ball, object, 0)) {
        return 13;
    }
    NJS_POINT3 before = ball.hit.pos;
    NJS_VECTOR speed = ball.hit.speed;
    if (!player_sweep_world(&ball, object, 0, &work)) {
        return 13;
    }
    if (!corner_near(0, ball.hit.pos, before) ||
        !corner_near(0, ball.hit.speed, speed)) {
        return 12;
    }
    ball.normal = corner_transform((NJS_VECTOR){0, scale, 0}, rotated, 0);
    if (!player_sweep_world(&ball, object, WORLD_NO_SLOPE, &work) ||
        !corner_near(0, ball.hit.pos, before) ||
        !corner_near(0, ball.hit.speed, speed)) {
        return 14;
    }
    for (int rider = 0; rider < 2; rider++) {
        int (*board_mode)(int) = rider ? tails_board_mode : sonic_board_mode;
        int mode = 0;
        while (mode < 128 && !board_mode(mode)) {
            mode++;
        }
        PLAYER_VARS(&work)->character =
            rider ? CHARACTER_TAILS : CHARACTER_SONIC;
        work.mode = mode;
        if (mode == 128 || !player_sweep_world(&ball, object, 0, &work) ||
            !corner_near(0, ball.hit.pos, before) ||
            !corner_near(0, ball.hit.speed, speed)) {
            return 15;
        }
    }
    work.mode = 0;
    if (!player_sweep_world(&ball, object, 0, &work) ||
        !corner_near(0, ball.hit.pos, resolved) ||
        !corner_near(0, ball.hit.speed, allowed)) {
        return 13;
    }

    /* Keep steering into the corner for eight ticks. Each query starts
     * where the last slide ended, with a fresh candidate list. */
    ball.pos = ball.hit.pos;
    ball.speed = corner_transform((NJS_VECTOR){-1, -2, 0.25F}, rotated, 0);
    for (int tick = 1; tick <= 8; tick++) {
        if (polygon_collect_near(&ball) != 2 ||
            !player_sweep_world(&ball, object, 0, &work)) {
            return 17;
        }
        ball.pos.x = ball.hit.pos.x + ball.hit.speed.x;
        ball.pos.y = ball.hit.pos.y + ball.hit.speed.y;
        ball.pos.z = ball.hit.pos.z + ball.hit.speed.z;
        NJS_POINT3 expected = {1, 1, 0.5F + tick * 0.25F};
        if (!corner_near(ball.radius * 0.001F, ball.pos,
                         corner_transform(expected, rotated, 1))) {
            return 18;
        }
    }
    return 0;
}

/* Two walls leave their intersection free to slide along. A third wall
 * can close it. Reordering or repeating a triangle describes the same room. */
static int inside_corner(int rotated)
{
    static const NJS_POINT3 face[3] = {
        {-16, 0, -16}, {-16, 0, 32}, {32, 0, -16}};
    static const struct {
        int count;
        enum corner_face face[POLYGON_CONTACTS + 1];
        NJS_VECTOR speed, allowed;
        NJS_POINT3 center;
    } cases[] = {
        {2, {FLOOR, X_WALL}, {-1, -2, 3}, {0, 0, 3}, {1, 1, 0.5F}},
        {2, {X_WALL, FLOOR}, {-1, -2, 3}, {0, 0, 3}, {1, 1, 0.5F}},
        {3, {X_WALL, FLOOR, FLOOR}, {-1, -2, 3}, {0, 0, 3}, {1, 1, 0.5F}},
        {3, {Z_WALL, FLOOR, X_WALL}, {-1, -2, -3}, {0, 0, 0}, {1, 1, 1}},
        {3, {X_WALL, Z_WALL, FLOOR}, {2, -2, 3}, {2, 0, 3}, {1, 1, 1}},
        {2, {DIAGONAL, FLOOR}, {-1, -2, 3}, {0, 0, 3}, {1, 1, 0.5F}},
        {2, {FLOOR, DIAGONAL}, {1, -1, 3}, {1, 0, 3}, {1, 1, 0.5F}},
        {2, {FLOOR, RAISED_FLOOR}, {1, -2, 3}, {1, 0, 3}, {0.5F, 1.25F, 0.5F}},
        {2, {FLOOR, CEILING}, {1, 2, 3}, {1, 0, 3}, {0.5F, 0.5F, 0.5F}},
        {3,
         {FLOOR, X_WALL, SHALLOW_WALL},
         {-3, -1, -0.004F},
         {0, 0, 0},
         {1, 1, 0.5F}},
        {2, {FLOOR, BACK_WALL}, {-1, -2, 3}, {0, 0, 3}, {1, 1, 0.5F}},
        {2, {FLOOR, LATER_WALL}, {-1, -2, 3}},
        {POLYGON_CONTACTS + 1, {X_WALL, Z_WALL, FLOOR}, {-1, -1, -1}},
    };
    for (int k = 0; k < (int)(sizeof(cases) / sizeof(*cases)); k++) {
        struct polygon_ball ball = {
            .radius = rotated ? 9 : 1,
            .pos = corner_transform((NJS_VECTOR){0.5F, 0.5F, 0.5F}, rotated, 1),
            .speed = corner_transform(cases[k].speed, rotated, 0)};
        collision_mesh_init();
        if (!update(1, 0)) {
            return 1;
        }
        struct collision_mesh *mesh = collision_mesh_list;
        collision_mesh_clear_buckets();
        for (int j = 0; j < cases[k].count; j++) {
            struct polygon *poly = polygon_alloc(0);
            poly->attr =
                cases[k].face[j] == BACK_WALL ? NJD_FLAG_DOUBLE_SIDE : 0;
            for (int i = 0; i < 3; i++) {
                NJS_POINT3 v = corner_vertex(cases[k].face[j], face[i]);
                poly->vertex[i] = corner_transform(v, rotated, 1);
            }
            poly->center = corner_transform((NJS_POINT3){0, 0, 0}, rotated, 1);
            poly->radius = 1280 * ball.radius * ball.radius;
        }
        collision_mesh_take_buckets(mesh);
        if (polygon_collect_near(&ball) != cases[k].count ||
            !polygon_sweep(&ball, mesh->object, 0)) {
            return 2;
        }
        int count = cases[k].count < POLYGON_CONTACTS ? cases[k].count
                                                      : POLYGON_CONTACTS;
        if (ball.count != count) {
            return 2;
        }
        if (count == POLYGON_CONTACTS || cases[k].face[1] == LATER_WALL) {
            NJS_POINT3 before = ball.hit.pos;
            NJS_VECTOR speed = ball.hit.speed;
            polygon_resolve_corner(&ball);
            if (!corner_near(0, ball.hit.pos, before) ||
                !corner_near(0, ball.hit.speed, speed)) {
                return 16;
            }
            continue;
        }
        polygon_resolve_corner(&ball);
        if (!corner_near(ball.radius * 0.001F, ball.hit.pos,
                         corner_transform(cases[k].center, rotated, 1)) ||
            !corner_near(ball.radius * 0.001F, ball.hit.speed,
                         corner_transform(cases[k].allowed, rotated, 0))) {
            return 3 + k;
        }
        if (k == 0) {
            int failure = check_player_corner(ball, mesh->object, rotated);
            if (failure) {
                return failure;
            }
        }
    }
    return 0;
}

/* The two player movement paths take a snapshot of the admitted objects.
 * Several entries may name the same support with different owners or flags. */
static int crowded_world(void)
{
    static struct world_object ordered[WORLD_OBJECTS];
    static struct player player;
    struct task_work work = {0}, missing = {0},
                     active = {.flags = WORLD_TESTED};
    struct task absent = {.work = &missing}, platform = {.work = &active};
    struct task_motion motion = {0};
    struct polygon_ball ball = {.radius = 1,
                                .pos = {0, 3, 0},
                                .speed = {0, -5, 0},
                                .normal = {0, 1, 0}};
    collision_mesh_init();
    if (!update(1, 0)) {
        return 1;
    }
    struct collision_mesh *mesh = collision_mesh_list;
    collision_mesh_clear_buckets();
    struct polygon *floor = polygon_alloc(0);
    floor->vertex[0] = (NJS_POINT3){-8, 0, -8};
    floor->vertex[1] = (NJS_POINT3){-8, 0, 16};
    floor->vertex[2] = (NJS_POINT3){16, 0, -8};
    floor->center = (NJS_POINT3){0, 0, 0};
    floor->radius = 320;
    floor->attr = 0;
    collision_mesh_take_buckets(mesh);
    if (polygon_collect_near(&ball) != 1) {
        return 2;
    }
    world_gravity = (NJS_VECTOR){0, -1, 0};
    player.contact = &ball;
    /* Object zero stops the fall. The two registrations of object one
     * go last, even when their owners and flags differ. */
    static const int sizes[] = {1, 16, 17, 256, WORLD_OBJECTS};
    for (int k = 0; k < (int)(sizeof(sizes) / sizeof(*sizes)); k++) {
        world_count = sizes[k];
        for (int i = 0; i < world_count; i++) {
            world_objects[i] =
                (struct world_object){.object = &objects[2], .flags = i * 4};
        }
        world_objects[0].object = mesh->object;
        if (world_count > 1) {
            world_objects[1].object = &objects[1];
            world_objects[world_count - 1].object = &objects[1];
            world_objects[world_count - 1].owner = &platform;
        }
        if (world_count > 2) {
            world_objects[2].flags |= WORLD_LIQUID;
        }
        if (world_count > 3) {
            world_objects[3].owner = &absent;
        }
        player.ground_object = &objects[1];
        for (int forward = 0; forward <= 1; forward++) {
            player.ground_task = forward ? &platform : NULL;
            for (int gamma = 0; gamma <= 1; gamma++) {
                PLAYER_VARS(&work)->character =
                    gamma ? CHARACTER_E102 : CHARACTER_SONIC;
                for (int pinball = 0; pinball <= 1; pinball++) {
                    ball.speed = (NJS_VECTOR){0, -5, 0};
                    int count = player_prepare_world(ordered, &work, &motion,
                                                     &player, pinball);
                    if (count != world_count - (!gamma && world_count > 2) -
                                     (world_count > 3) ||
                        ordered[0].object != &objects[0]) {
                        return 3;
                    }
                    int at = 1;
                    for (int support = 0; support <= 1; support++) {
                        for (int j = 0; j < world_count; j++) {
                            int i = forward ? j : world_count - 1 - j;
                            if (i == 0 || (!gamma && i == 2) || i == 3 ||
                                (world_objects[i].object ==
                                 player.ground_object) != support) {
                                continue;
                            }
                            if (at >= count ||
                                ordered[at].flags != world_objects[i].flags ||
                                ordered[at].object != world_objects[i].object ||
                                ordered[at].owner != world_objects[i].owner) {
                                return 4;
                            }
                            at++;
                        }
                    }
                    if (at != count) {
                        return 5;
                    }
                }
            }
        }
    }
    return 0;
}

static struct polygon *floor_triangle(float x, float z)
{
    struct polygon *p = polygon_alloc(z);
    p->vertex[0] = (NJS_POINT3){x - 4, 0, z - 4};
    p->vertex[1] = (NJS_POINT3){x - 4, 0, z + 8};
    p->vertex[2] = (NJS_POINT3){x + 8, 0, z - 4};
    p->center = (NJS_POINT3){x, 0, z};
    p->radius = 80;
    p->attr = 0;
    return p;
}

/* Only the final triangle under the ball is a floor. A second mesh owns
 * another floor in the same place; both must survive a crowded query. */
static int dense_meshes(Bool backward)
{
    struct polygon_ball ball = {
        .pos = {0, 3, 0}, .speed = {0, -5, 0}, .radius = 1};
    struct collision_mesh *first, *second;
    collision_mesh_init();
    if (!update(2, 0)) {
        return 1;
    }
    first = collision_mesh_list;
    second = first->next;
    first->span = second->span = 12;
    collision_mesh_clear_buckets();
    floor_triangle(0, 0);
    collision_mesh_take_buckets(second);
    if (polygon_collect_near(&ball) != 1 ||
        !polygon_sweep(&ball, second->object, 0)) {
        return 2;
    }
    collision_mesh_clear_buckets();
    if (!backward) {
        floor_triangle(0, 0);
    }
    /* The bucket prepends. Leave long runs, short gaps and gaps over 32
     * before the floor in traversal order. */
    int admitted = 1;
    for (int i = 0; i < 320; i++) {
        Bool near = i % 79 < 46 && i % 7 != 0;
        floor_triangle(near ? 8 : 64, 0);
        admitted += near;
    }
    if (backward) {
        floor_triangle(0, 0);
        floor_triangle(8, -128);
    }
    collision_mesh_take_buckets(first);
    if (polygon_collect_near(&ball) != admitted + 1 ||
        first->near_count != admitted || second->near_count != 1 ||
        !polygon_sweep(&ball, first->object, 0) || ball.count != 1 ||
        !polygon_sweep(&ball, second->object, 0) || ball.count != 1) {
        return 3;
    }
    /* The next query admits nothing from either previous selection. */
    ball.pos.x = 256;
    if (polygon_collect_near(&ball) || polygon_sweep(&ball, first->object, 0) ||
        polygon_sweep(&ball, second->object, 0)) {
        return 4;
    }
    return 0;
}

/* Force a partial mask in every mesh and use the last free polygon. */
static int full_near_pool(void)
{
    struct polygon_ball ball = {
        .pos = {0, 3, 0}, .speed = {0, -5, 0}, .radius = 1};
    collision_mesh_init();
    if (!update(192, 0)) {
        return 1;
    }
    int remaining = 8191;
    for (struct collision_mesh *mesh = collision_mesh_list; mesh;
         mesh = mesh->next) {
        int count = mesh->next ? 33 : remaining;
        remaining -= count;
        collision_mesh_clear_buckets();
        floor_triangle(0, 0);
        for (int i = 1; i < count; i++) {
            floor_triangle(8, 0);
        }
        collision_mesh_take_buckets(mesh);
    }
    if (remaining || polygon_alloc(0) || polygon_collect_near(&ball) != 8191) {
        return 2;
    }
    for (struct collision_mesh *mesh = collision_mesh_list; mesh;
         mesh = mesh->next) {
        if (mesh->near_count != (mesh->next ? 33 : 1888) ||
            !polygon_sweep(&ball, mesh->object, 0) || ball.count != 1) {
            return 3;
        }
    }
    return 0;
}

/* the ball reaches this floor near the vertex farthest from its center */
static int swept_reach(void)
{
    struct polygon_ball ball = {
        .pos = {-6, 2, 0}, .speed = {6.75F, -1.5F, 0}, .radius = 1};
    collision_mesh_init();
    if (!update(1, 0)) {
        return 1;
    }
    struct collision_mesh *mesh = collision_mesh_list;
    collision_mesh_clear_buckets();
    struct polygon *p = polygon_alloc(0);
    p->vertex[0] = (NJS_POINT3){0, 0, 0};
    p->vertex[1] = (NJS_POINT3){10, 0, 1};
    p->vertex[2] = (NJS_POINT3){10, 0, -1};
    p->center = (NJS_POINT3){20 * 0.3333F, 0, 0};
    p->radius = p->center.x * p->center.x;
    p->attr = 0;
    mesh->span = 2;
    collision_mesh_take_buckets(mesh);
    return polygon_collect_near(&ball) != 1 || world_radius < 7.9F ||
           !polygon_sweep(&ball, mesh->object, 0);
}

static void small_floor(struct collision_mesh *mesh, float x, float z,
                        Bool prepend)
{
    Float cz = (z - 0.25F + (z + 0.5F) + (z - 0.25F)) * 0.3333F;
    struct polygon *p = prepend ? collision_mesh_add_polygon(mesh)
                                : polygon_alloc(cz - mesh->center.z);
    p->vertex[0] = (NJS_POINT3){x - 0.25F, 0, z - 0.25F};
    p->vertex[1] = (NJS_POINT3){x - 0.25F, 0, z + 0.5F};
    p->vertex[2] = (NJS_POINT3){x + 0.5F, 0, z - 0.25F};
    p->center = (NJS_POINT3){x, 0, cz};
    p->radius = 0;
    for (int i = 0; i < 3; i++) {
        Float dx = p->vertex[i].x - x, dz = p->vertex[i].z - cz;
        Float radius = dx * dx + dz * dz;
        if (radius > p->radius) {
            p->radius = radius;
        }
    }
    p->attr = 0;
}

/* an out-of-range center can precede the floor inside a bucket. direct
 * insertion can also put it on the far side of a whole bucket */
static int bucket_floors(void)
{
    static const struct {
        float root, z, gap;
        Bool back, prepend;
    } cases[] = {{0, 1, 12, FALSE, FALSE},
                 {80, 93, 12, TRUE, FALSE},
                 {0, 2048, 2048, FALSE, FALSE},
                 {64, -2048, 2048, TRUE, FALSE},
                 {0, 0, 160, FALSE, TRUE}};
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        Float z = cases[i].z;
        struct polygon_ball ball = {
            .pos = {0, 3, z}, .speed = {0, -5, 0}, .radius = 1};
        collision_mesh_init();
        if (!update(1, 0)) {
            return 1;
        }
        struct collision_mesh *mesh = collision_mesh_list;
        mesh->center.z = cases[i].root;
        mesh->span = 0.75F;
        collision_mesh_clear_buckets();
        if (cases[i].back) {
            small_floor(mesh, 8, z, FALSE);
            small_floor(mesh, 0, z - cases[i].gap, FALSE);
            small_floor(mesh, 0, z, FALSE);
            small_floor(mesh, 8, z - 128, FALSE);
        } else {
            small_floor(mesh, 0, z, FALSE);
            if (cases[i].prepend) {
                collision_mesh_take_buckets(mesh);
            }
            small_floor(mesh, 0, z + cases[i].gap, cases[i].prepend);
            small_floor(mesh, 8, z, cases[i].prepend);
        }
        if (!cases[i].prepend) {
            collision_mesh_take_buckets(mesh);
        }
        if (!polygon_collect_near(&ball) ||
            !polygon_sweep(&ball, mesh->object, 0) || ball.count != 1) {
            return 2 + i;
        }
    }

    /* at stage-sized coordinates the stored centroid can lie outside even
     * the mesh span plus the ball's reach */
    collision_mesh_init();
    if (!update(1, 0)) {
        return 1;
    }
    struct collision_mesh *mesh = collision_mesh_list;
    collision_mesh_clear_buckets();
    mesh->span = 0.75F;
    mesh->center.z = 25000;
    small_floor(mesh, 0, 25000, FALSE);
    collision_mesh_take_buckets(mesh);
    struct polygon_ball ball = {
        .pos = {0, 0.2F, 25000}, .speed = {0, -0.3F, 0}, .radius = 0.1F};
    return polygon_collect_near(&ball) != 1 ||
           !polygon_sweep(&ball, mesh->object, 0);
}

/* a vertical sweep meets the square whenever its center passes within one
 * radius of the perimeter, whichever way the square is triangulated */
static void probe_floor(int a, int b, int c)
{
    static const NJS_POINT3 corner[] = {
        {0, 0, 0}, {0, 0, 4}, {4, 0, 4}, {4, 0, 0}};
    struct polygon *poly = polygon_alloc(0);
    poly->vertex[0] = corner[a];
    poly->vertex[1] = corner[b];
    poly->vertex[2] = corner[c];
    poly->center = (NJS_POINT3){2, 0, 2};
    poly->radius = 8;
    poly->attr = 0;
}

static int terrain_probes(void)
{
    struct polygon_ball ball = {
        .radius = 1, .pos = {-0.6F, 3, 1}, .speed = {0, -5, 0}};
    collision_mesh_init();
    if (!update(1, 0)) {
        return 1;
    }
    struct collision_mesh *mesh = collision_mesh_list;
    collision_mesh_clear_buckets();
    probe_floor(0, 1, 3);
    collision_mesh_take_buckets(mesh);
    struct world_object other = {.object = &objects[1]};
    polygon_contacts[0].height = 789.0F;
    ball.count = 7;
    if (!polygon_collect_near(&ball) || !polygon_ball_touches(&ball, NULL) ||
        polygon_ball_touches(&ball, &other)) {
        return 2;
    }
    if (!polygon_ball_touches(&ball, &world_objects[0]) || ball.count != 7 ||
        polygon_contacts[0].height != 789.0F) {
        return 3;
    }
    /* skim the face between samples, miss just outside the open edge,
     * then approach an isolated vertex from below */
    ball.pos = (NJS_POINT3){-2, 1, 1};
    ball.speed = (NJS_VECTOR){8, 0, 0};
    if (!polygon_collect_near(&ball) || !polygon_ball_touches(&ball, NULL)) {
        return 5;
    }
    ball.pos.y = 1.01F;
    if (polygon_collect_near(&ball) && polygon_ball_touches(&ball, NULL)) {
        return 6;
    }
    ball.pos = (NJS_POINT3){-0.6F, -3, -0.6F};
    ball.speed = (NJS_VECTOR){0, 5, 0};
    if (!polygon_collect_near(&ball) || !polygon_ball_touches(&ball, NULL)) {
        return 7;
    }
    /* only the diagonal grip probe reaches this vertex */
    struct collision_shape shape = {.center = {-4.6F, 0, -4.6F}};
    struct collision_work collision = {.shapes = &shape};
    struct task_work work = {.collision = &collision};
    static struct player player;
    player.consts.height = 1;
    if (!knuckles_wall_near(&work, NULL, &player) || world_radius < 6.65F) {
        return 4;
    }
    shape.center.z -= 1.0F;
    if (knuckles_wall_near(&work, NULL, &player)) {
        return 13;
    }
    /* a zero-area strip connector has no surface */
    mesh->polygons->vertex[1] = mesh->polygons->vertex[0];
    if (!polygon_collect_near(&ball) || polygon_ball_touches(&ball, NULL)) {
        return 8;
    }
    mesh->polygons->vertex[1] = (NJS_POINT3){2.1F, 3.3F, 1.7F};
    mesh->polygons->vertex[2] = mesh->polygons->vertex[1];
    ball.pos = mesh->polygons->vertex[1];
    ball.speed = (NJS_VECTOR){0, 0, 0};
    if (!polygon_collect_near(&ball) || polygon_ball_touches(&ball, NULL)) {
        return 14;
    }
    for (int diagonal = 0; diagonal <= 1; diagonal++) {
        collision_mesh_init();
        if (!update(1, 0)) {
            return 9;
        }
        mesh = collision_mesh_list;
        collision_mesh_clear_buckets();
        probe_floor(diagonal, diagonal + 1, diagonal + 2);
        probe_floor(diagonal, diagonal + 2, (diagonal + 3) % 4);
        collision_mesh_take_buckets(mesh);
        /* one enclosing collection serves the swept and stationary probes */
        ball.radius = 12;
        ball.pos = (NJS_POINT3){2, 0, 2};
        ball.speed = (NJS_VECTOR){0, 0, 0};
        if (polygon_collect_near(&ball) != 2) {
            return 10;
        }
        ball.radius = 1;
        for (int x = -5; x <= 21; x++) {
            for (int z = -5; z <= 21; z++) {
                int dx = x < 0 ? -x : x > 16 ? x - 16 : 0;
                int dz = z < 0 ? -z : z > 16 ? z - 16 : 0;
                Bool expected = dx * dx + dz * dz <= 16;
                ball.pos = (NJS_POINT3){x * 0.25F, 3, z * 0.25F};
                ball.speed = (NJS_VECTOR){0, -5, 0};
                if (polygon_ball_touches(&ball, NULL) != expected) {
                    return 11;
                }
                ball.pos.y = 0;
                ball.speed.y = 0;
                if (polygon_ball_touches(&ball, NULL) != expected) {
                    return 12;
                }
            }
        }
    }
    return 0;
}

/* Both walls are vertical. The oblique wall has normal (4/5, 0, -3/5). */
static void air_wall_triangle(int wall, int a, int b, int c)
{
    static const NJS_POINT3 corners[] = {
        {-64, 0, -64}, {-64, 0, 64}, {64, 0, 64}, {64, 0, -64}};
    int indices[] = {a, b, c};
    struct polygon *tri = polygon_alloc(0);
    for (int i = 0; i < 3; i++) {
        NJS_POINT3 p = corners[indices[i]];
        tri->vertex[i] = wall ? (NJS_POINT3){0.6F * p.x, p.z, 0.8F * p.x}
                              : (NJS_POINT3){0, p.z, p.x};
    }
    tri->center = (NJS_POINT3){0};
    tri->radius = 8192;
    tri->attr = 0;
}

static int air_wall_scene(int separate, int diagonal, int reverse,
                          const struct polygon *extra)
{
    collision_mesh_init();
    for (int i = 0; i < 2; i++) {
        world_objects[i] = (struct world_object){.flags = WORLD_SOLID};
    }
    if (!update(1 + separate, 0)) {
        return 0;
    }
    for (struct collision_mesh *mesh = collision_mesh_list; mesh;
         mesh = mesh->next) {
        collision_mesh_clear_buckets();
        for (int k = 0; k < 2; k++) {
            int wall = k ^ reverse;
            if (separate && mesh->object != &objects[wall]) {
                continue;
            }
            int a = diagonal;
            air_wall_triangle(wall, a, (a + 1) % 4, (a + 2) % 4);
            air_wall_triangle(wall, a, (a + 2) % 4, (a + 3) % 4);
        }
        if (extra && mesh == collision_mesh_list) {
            struct polygon *tri = polygon_alloc(0);
            for (int i = 0; i < 3; i++) {
                tri->vertex[i] = extra->vertex[i];
            }
            tri->center = extra->center;
            tri->radius = extra->radius;
            tri->attr = extra->attr;
        }
        mesh->span = 64;
        collision_mesh_take_buckets(mesh);
    }
    if (separate && reverse) {
        world_objects[0].object = &objects[1];
        world_objects[1].object = &objects[0];
    }
    return 1;
}

/* Feet and velocity are checked separately. The distance traveled before an
 * impact contributes to position but never to the velocity leaving a wall. */
struct air_move {
    NJS_POINT3 start;
    NJS_VECTOR velocity;
    NJS_POINT3 end;
    NJS_VECTOR outgoing, normal;
};

static Bool near_point(NJS_POINT3 a, NJS_POINT3 b)
{
    return njAbs(a.x - b.x) < 0.0001F && njAbs(a.y - b.y) < 0.0001F &&
           njAbs(a.z - b.z) < 0.0001F;
}

static Bool air_move(int mode, const struct air_move *move, NJS_POINT3 *end)
{
    static struct player record;
    static struct task_motion motion;
    static struct task_work work;
    static struct polygon_ball ball;
    const NJS_VECTOR actor_contact = {0, 0, 1};
    world_gravity = (NJS_VECTOR){0, -1, 0};
    player_work[0] = &work;
    player_motion[0] = &motion;
    player_records[0] = &record;
    motion = (struct task_motion){.speed = move->velocity};
    work = (struct task_work){.mode = mode, .pos = move->start};
    ball = (struct polygon_ball){0};
    record = (struct player){.contact = &ball,
                             .speed = motion.speed,
                             .consts = character_constants[CHARACTER_SONIC],
                             .ground_normal = {0, 1, 0},
                             .wall_normal = actor_contact};
    unsigned hit = move_in_world(&work, &motion, &record);
    *end = work.pos;
    /* A clear terrain sweep preserves a contact from the earlier actor pass. */
    NJS_VECTOR normal = move->normal;
    if (normal.x == 0 && normal.y == 0 && normal.z == 0) {
        normal = actor_contact;
    }
    return !hit && !(work.flags & PLAYER_ON_GROUND) && !record.ground_task &&
           !record.ground_object && near_point(work.pos, move->end) &&
           near_point(motion.speed, move->outgoing) &&
           near_point(record.wall_normal, normal);
}

static Bool defers_air_move(const struct polygon_ball *ball,
                            struct polygon_ball *collected, Float clearance)
{
    struct polygon_slide slide = {.correction = {1, 0, 0},
                                  .travel = {2, 0, 0},
                                  .velocity = {3, 0, 0},
                                  .normal = {4, 0, 0}};
    return !polygon_slide_air(ball, &slide, collected, clearance, 0.47F,
                              FALSE) &&
           slide.correction.x == 1 && slide.travel.x == 2 &&
           slide.velocity.x == 3 && slide.normal.x == 4;
}

static int airborne_walls(void)
{
    static const struct air_move moves[] = {
        /* Both walls initially overlap the sphere. */
        {{2.5F, -2.5F, 2.5F},
         {-5, 1.25F, 5},
         {6.7F, -1.25F, 0.6F},
         {0.6F, 1.25F, 0.8F},
         {0.8F, 0, -0.6F}},
        /* The oblique wall is first, at 9/35 of the tick. */
        {{7, -2.5F, -2},
         {-5, 1.25F, 5},
         {6.16F, -1.25F, -0.12F},
         {0.6F, 1.25F, 0.8F},
         {0.8F, 0, -0.6F}},
        /* Hit the X wall at 1/3, then the oblique wall at 7/12. */
        {{7, -2.5F, -4},
         {-6, 1.25F, 4},
         {5.8F, -1.25F, -0.6F},
         {1.92F, 1.25F, 2.56F},
         {0.8F, 0, -0.6F}},
        /* A hit on the last instant changes next tick's velocity. */
        {{8, -2.5F, -8},
         {-3, 1.25F, 0},
         {5, -1.25F, -8},
         {0, 1.25F, 0},
         {1, 0, 0}},
    };
    enum { MOVES = sizeof moves / sizeof *moves };
    for (int separate = 0; separate <= 1; separate++) {
        for (int diagonal = 0; diagonal <= 1; diagonal++) {
            NJS_POINT3 first[2][2][MOVES];
            for (int reverse = 0; reverse <= 1; reverse++) {
                if (!air_wall_scene(separate, diagonal, reverse, NULL)) {
                    return 1;
                }
                int modes = 0;
                for (int mode = 0; mode < 256; mode++) {
                    if (player_terrain_motion(CHARACTER_SONIC, mode) !=
                        PLAYER_TERRAIN_AIR) {
                        continue;
                    }
                    if (modes == 2) {
                        return 10;
                    }
                    for (int i = 0; i < MOVES; i++) {
                        for (int sign = -1; sign <= 1; sign += 2) {
                            struct air_move m = moves[i];
                            m.velocity.y *= sign;
                            m.outgoing.y *= sign;
                            m.end.y = m.start.y + m.velocity.y;
                            NJS_POINT3 end;
                            if (!air_move(mode, &m, &end)) {
                                return 2 + i;
                            }
                            NJS_POINT3 *previous = &first[modes][sign > 0][i];
                            if (!reverse) {
                                *previous = end;
                            } else if (end.x != previous->x ||
                                       end.y != previous->y ||
                                       end.z != previous->z) {
                                return 18;
                            }
                        }
                    }
                    modes++;
                }
                if (modes != 2) {
                    return 10;
                }
            }
        }
    }
    /* The straight sweep stays at x = 7.5. Sliding along the oblique wall
     * reaches x = 14, where a third wall stops the horizontal movement. */
    static const struct polygon stop = {
        .vertex = {{19, -64, -64}, {19, -64, 64}, {19, 64, 0}},
        .center = {19, -21, 0},
        .radius = 8192};
    static const struct air_move turn = {{7.5F, -2.5F, 0},
                                         {0, 1.25F, 20},
                                         {14, -1.25F, 31.0F / 3.0F},
                                         {0, 1.25F, 0},
                                         {-1, 0, 0}};
    for (int reverse = 0; reverse <= 1; reverse++) {
        if (!air_wall_scene(1, 0, reverse, &stop)) {
            return 19;
        }
        for (int mode = 0; mode < 256; mode++) {
            NJS_POINT3 end;
            if (player_terrain_motion(CHARACTER_SONIC, mode) ==
                    PLAYER_TERRAIN_AIR &&
                !air_move(mode, &turn, &end)) {
                return 20;
            }
        }
    }
    /* A deflector owns its response, even if it reuses a plain wall mesh. */
    if (!air_wall_scene(1, 0, 0, NULL)) {
        return 11;
    }
    world_objects[1].flags |= WORLD_DEFLECT;
    struct polygon_ball desired = {
        .pos = {2.5F, 2.5F, 2.5F}, .radius = 5, .speed = {-5, 1.25F, 5}};
    struct polygon_ball collected = desired;
    struct polygon_slide slide;
    polygon_collect_near(&collected);
    if (!defers_air_move(&desired, &collected, 6.1F)) {
        return 12;
    }
    /* This ledge is outside the body sphere but inside the step probe's
     * clearance at the end of the slide. Its own landing test must run. */
    static const struct polygon ledge = {
        .vertex = {{5, 9.3F, -2}, {5, 9.3F, 3}, {10, 9.3F, 0}},
        .center = {6.67F, 9.3F, 0.33F},
        .radius = 36};
    if (!air_wall_scene(1, 0, 0, &ledge)) {
        return 13;
    }
    polygon_collect_near(&collected);
    if (!defers_air_move(&desired, &collected, 6.1F)) {
        return 14;
    }
    if (!polygon_slide_air(&desired, &slide, &collected, 5.0F, 0.47F, FALSE)) {
        return 15;
    }
    /* With no requested motion, the push-out reaches a third triangle
     * beyond the original collection. Rejection restores that collection. */
    static const struct polygon lip = {.vertex = {{10.4F, 2.5F, -0.3F},
                                                  {10.4F, 2.4F, -0.1F},
                                                  {10.4F, 2.6F, -0.1F}},
                                       .center = {10.4F, 2.5F, -0.17F},
                                       .radius = 0.04F,
                                       .attr = NJD_FLAG_DOUBLE_SIDE};
    if (!air_wall_scene(1, 0, 0, &lip)) {
        return 16;
    }
    collected.speed = (NJS_VECTOR){0};
    desired = collected;
    if (polygon_collect_near(&collected) != 4 ||
        !defers_air_move(&desired, &collected, 5.0F) ||
        collision_mesh_list->near_count != 2 || world_radius != 5.0F) {
        return 17;
    }
    return 0;
}

/* A triangle in the positive Y,Z quadrant exposes one edge and the origin. */
static int airborne_rims(void)
{
    static const struct air_move moves[] = {
        {{8, 0, -3},
         {-10, 0, 0},
         {1.84F, 0, -5.88F},
         {-3.6F, 0, -4.8F},
         {0.8F, 0, -0.6F}},
        {{8, -6.8F, -2.4F},
         {-10, 0, 0},
         {1.84F, -8.528F, -4.704F},
         {-3.6F, -2.88F, -3.84F},
         {0.8F, -0.36F, -0.48F}},
        {{8, 0, -5}, {-10, 0, 0}, {-2, 0, -5}, {-10, 0, 0}, {0, 0, 0}},
        /* Enter the face projection before crossing the plane's radius band. */
        {{5.00004F, 0, -0.05F},
         {-0.002F, 0, 16},
         {5, 0, 15.95F},
         {0, 0, 16},
         {1, 0, 0}},
    };
    collision_mesh_init();
    world_objects[0] = (struct world_object){.flags = WORLD_SOLID};
    if (!update(1, 0)) {
        return 1;
    }
    collision_mesh_clear_buckets();
    struct polygon *tri = polygon_alloc(0);
    tri->vertex[0] = (NJS_POINT3){0, 0, 0};
    tri->vertex[1] = (NJS_POINT3){0, 64, 0};
    tri->vertex[2] = (NJS_POINT3){0, 0, 64};
    tri->center = (NJS_POINT3){0, 21, 21};
    tri->radius = 2304;
    tri->attr = 0;
    collision_mesh_list->span = 64;
    collision_mesh_take_buckets(collision_mesh_list);
    for (int mode = 0; mode < 256; mode++) {
        if (player_terrain_motion(CHARACTER_SONIC, mode) ==
            PLAYER_TERRAIN_AIR) {
            for (unsigned i = 0; i < sizeof moves / sizeof *moves; i++) {
                NJS_POINT3 end;
                if (!air_move(mode, &moves[i], &end)) {
                    return 2 + i;
                }
            }
        }
    }
    return 0;
}

/* The same strip can be triangulated in either direction or divided between
 * land objects. Only the owner of the final floor can depend on that split. */
enum strip_shape {
    STRIP_FLAT,
    STRIP_LEDGE,
    STRIP_RAMP,
    STRIP_CREST,
    STRIP_GAP,
    STRIP_STEEP,
    STRIP_SHALLOW_CREST
};

static void ground_quad(const NJS_POINT3 vertex[4], int diagonal, int reverse,
                        Uint32 attr)
{
    for (int i = 0; i < 2; i++) {
        int first = diagonal + (i ^ reverse) + 1;
        struct polygon *tri = polygon_alloc(0);
        tri->vertex[0] = vertex[diagonal];
        tri->vertex[1] = vertex[first % 4];
        tri->vertex[2] = vertex[(first + 1) % 4];
        tri->center = (NJS_POINT3){(vertex[0].x + vertex[2].x) * 0.5F,
                                   (vertex[0].y + vertex[2].y) * 0.5F,
                                   (vertex[0].z + vertex[2].z) * 0.5F};
        tri->radius = 8192;
        tri->attr = attr;
    }
}

/* Tilt the entire contact set about X. Its common tangent becomes
 * (0, 24/25, 7/25), and every face still qualifies as a side wall. */
static NJS_VECTOR fan_tilt(NJS_VECTOR v, int tilted)
{
    return tilted ? (NJS_VECTOR){v.x, (24 * v.y - 7 * v.z) / 25,
                                 (7 * v.y + 24 * v.z) / 25}
                  : v;
}

static Bool wall_fan(int count, int layout, int tilted)
{
    int split = layout & 1, reverse = layout >> 1;
    collision_mesh_init();
    for (int i = 0; i < 2; i++) {
        world_objects[i] = (struct world_object){.flags = WORLD_SOLID};
    }
    if (!update(1 + split, 0)) {
        return FALSE;
    }
    for (struct collision_mesh *mesh = collision_mesh_list; mesh;
         mesh = mesh->next) {
        collision_mesh_clear_buckets();
        for (int j = 0; j < count; j++) {
            int i = reverse ? count - 1 - j : j;
            if (split && mesh->object != objects + (i >= count / 2)) {
                continue;
            }
            /* t = tan(angle/2). Each panel is tangent to the radius-five
             * sphere, and the middle panel touches at x = 5. */
            Float t = (i - count / 2) * (1.0F / 24);
            Float c = (1 - t * t) / (1 + t * t);
            Float n = 2 * t / (1 + t * t);
            NJS_POINT3 quad[4];
            for (int k = 0; k < 4; k++) {
                Float side = k == 1 || k == 2 ? 1 : -1;
                quad[k] =
                    fan_tilt((NJS_VECTOR){5 * c - side * n, k < 2 ? -24 : 32,
                                          5 * n + side * c},
                             tilted);
                quad[k].y += 2.5F;
            }
            ground_quad(quad, reverse, reverse, 0);
        }
        mesh->span = 128;
        collision_mesh_take_buckets(mesh);
    }
    if (split && reverse) {
        world_objects[0].object = objects + 1;
        world_objects[1].object = objects;
    }
    return TRUE;
}

static int crowded_walls(void)
{
    static const int sizes[] = {15, 19, 35};
    for (int layout = 0; layout < 4; layout++) {
        for (int tilted = 0; tilted <= 1; tilted++) {
            for (int sample = 0; sample < 3; sample++) {
                int count = sizes[sample];
                if (!wall_fan(count, layout, tilted)) {
                    return 1;
                }
                for (int kind = 0; kind < 3; kind++) {
                    /* Pushing X into a symmetric fan leaves only the axial
                     * velocity. Pushing Z slides out along its last panel. */
                    NJS_VECTOR speed = {2, 1, 0};
                    NJS_VECTOR allowed = {0, 1, 0}, normal = {-1, 0, 0};
                    Float overlap = kind == 2 ? 0.25F : 0;
                    if (kind == 1) {
                        Float t = (count / 2) * (1.0F / 24);
                        Float c = (1 - t * t) / (1 + t * t),
                              n = 2 * t / (1 + t * t);
                        speed = (NJS_VECTOR){0, 1, 1};
                        allowed = (NJS_VECTOR){-n * c, 1, c * c};
                        normal = (NJS_VECTOR){-c, 0, -n};
                    }
                    speed = fan_tilt(speed, tilted);
                    allowed = fan_tilt(allowed, tilted);
                    normal = fan_tilt(normal, tilted);
                    struct polygon_ball ball = {
                        .pos = {overlap, 2.5F, 0}, .speed = speed, .radius = 5};
                    struct polygon_ball region = ball;
                    region.radius = 10;
                    if (!polygon_collect_near(&region)) {
                        return 2;
                    }
                    int requests = heap_requests, frees = heap_frees;
                    heap_enabled = TRUE;
                    struct polygon_slide slide;
                    if (!polygon_slide_air(&ball, &slide, &region, 10, 0.47F,
                                           FALSE) ||
                        !near_point(slide.correction,
                                    (NJS_VECTOR){-overlap, 0, 0}) ||
                        !near_point(slide.travel, allowed) ||
                        !near_point(slide.velocity, allowed) || slide.support) {
                        return 3 + kind;
                    }
                    struct air_move move = {
                        .start = {overlap, -2.5F, 0},
                        .velocity = speed,
                        .end = {allowed.x, allowed.y - 2.5F, allowed.z},
                        .outgoing = allowed,
                        .normal = normal};
                    NJS_POINT3 end;
                    if (!air_move(sonic_terrain_modes.jump, &move, &end)) {
                        return 6 + kind;
                    }
                    heap_enabled = FALSE;
                    if (heap_busy || heap_bad_free ||
                        heap_requests - requests != heap_frees - frees ||
                        (count > POLYGON_CONTACTS) !=
                            (heap_requests != requests)) {
                        return 9;
                    }
                }
            }
        }
    }
    /* All panels touch when the center reaches x = 0. Denying the heap
     * keeps the travel up to that impact, including a partially spent tick. */
    if (!wall_fan(35, 3, FALSE)) {
        return 1;
    }
    for (int distance = 0; distance <= 2; distance += 2) {
        struct polygon_ball ball = {.pos = {-distance, 2.5F, 0},
                                    .speed = {2 + distance, 0, 0},
                                    .radius = 5};
        struct polygon_ball region = ball;
        region.radius = 10;
        int requests = heap_requests;
        struct polygon_slide slide;
        if (!polygon_collect_near(&region) ||
            !polygon_slide_air(&ball, &slide, &region, 10, 0.47F, FALSE) ||
            !slide.stalled || !near_point(slide.velocity, (NJS_VECTOR){0}) ||
            !near_point(slide.travel, (NJS_VECTOR){distance, 0, 0}) ||
            !near_point(slide.correction, (NJS_VECTOR){0})) {
            return 10;
        }
        struct air_move move = {.start = {-distance, -2.5F, 0},
                                .velocity = ball.speed,
                                .end = {0, -2.5F, 0}};
        NJS_POINT3 end;
        if (!air_move(sonic_terrain_modes.jump, &move, &end) ||
            heap_requests == requests || heap_busy || heap_bad_free) {
            return 11;
        }
        /* A mover's wider probe can reach the same mesh before the body.
         * A failed allocation still stops the player at the physical hit. */
        struct task_work active = {.flags = WORLD_TESTED};
        struct task mover = {.work = &active};
        world_objects[world_count++] = (struct world_object){
            WORLD_SOLID | WORLD_MOVES, world_objects[0].object, &mover};
        Bool stopped =
            polygon_collect_near(&region) &&
            polygon_slide_air(&ball, &slide, &region, 10, 0.47F, TRUE) &&
            slide.stalled &&
            near_point(slide.travel, (NJS_VECTOR){distance, 0, 0}) &&
            near_point(slide.velocity, (NJS_VECTOR){0});
        world_count--;
        if (!stopped) {
            return 12;
        }
    }
    return 0;
}

static void ground_tile(Float left, Float right, Float back, Float front,
                        Float slope, int diagonal, int reverse)
{
    Float rise = (right - left) * slope;
    NJS_POINT3 vertex[] = {{left, 0, back},
                           {left, 0, front},
                           {right, rise, front},
                           {right, rise, back}};
    ground_quad(vertex, diagonal, reverse, 0);
}

static Bool ground_strip(enum strip_shape shape, int split, int diagonal,
                         int reverse, int rows)
{
    collision_mesh_init();
    for (int i = 0; i < 2; i++) {
        world_objects[i] = (struct world_object){.flags = WORLD_SOLID};
    }
    if (!update(1 + split, 0)) {
        return FALSE;
    }
    for (struct collision_mesh *mesh = collision_mesh_list; mesh;
         mesh = mesh->next) {
        collision_mesh_clear_buckets();
        for (int i = 0; i < 2; i++) {
            int tile = i ^ reverse;
            if ((split && mesh->object != &objects[tile]) ||
                (tile && shape == STRIP_LEDGE)) {
                continue;
            }
            Float left = tile ? (shape == STRIP_GAP ? 3 : 0) : -32;
            Float right = tile ? 32 : 0;
            static const Float slopes[] = {[STRIP_RAMP] = 0.25F,
                                           [STRIP_CREST] = -0.25F,
                                           [STRIP_STEEP] = 0.75F,
                                           [STRIP_SHALLOW_CREST] = -0.03F};
            Float slope = tile ? slopes[shape] : 0;
            for (int row = 0; row < rows; row++) {
                Float back = -32 + row * (64 / rows);
                ground_tile(left, right, back, back + 64 / rows, slope,
                            diagonal, reverse);
            }
        }
        mesh->span = 64;
        collision_mesh_take_buckets(mesh);
    }
    if (split && reverse) {
        world_objects[0].object = &objects[1];
        world_objects[1].object = &objects[0];
    }
    return TRUE;
}

static void begin_stride(int mode, NJS_POINT3 pos, Float speed)
{
    static struct player record;
    static struct task_motion motion;
    static struct task_work work;
    static struct polygon_ball ball;
    world_gravity = (NJS_VECTOR){0, -1, 0};
    player_work[0] = &work;
    player_motion[0] = &motion;
    player_records[0] = &record;
    work =
        (struct task_work){.mode = mode, .flags = PLAYER_ON_LAND, .pos = pos};
    motion = (struct task_motion){.speed = {speed, 0, 0}};
    ball = (struct polygon_ball){0};
    record = (struct player){.contact = &ball,
                             .speed = motion.speed,
                             .consts = character_constants[CHARACTER_SONIC],
                             .level = 1,
                             .ground_normal = {0, 1, 0},
                             .last_ground_flags = WORLD_SOLID,
                             .last_flags = PLAYER_ON_LAND,
                             .ground_object = &objects[0]};
    record.ground_flags = WORLD_SOLID;
}

/* On beach_act1_land, water below entry 80 must let Sonic stop at the lip.
 * Water above entry 62 must let Sonic leave the bank. Gamma stands on it.
 * The final face of each case belongs to the water entry. */
static int coast_water_stride(void)
{
    static struct {
        NJS_POINT3 points[9];
        Sint16 faces[5][3];
        Uint32 material[5];
        int count;
        Uint32 flags;
        NJS_POINT3 start, normal, velocity, end, outgoing;
        Angle yaw, pitch, roll;
        Bool grounded, test_gamma;
    } cases[] = {
        {.points = {{1187.07422F, 546.999939F, -970.615356F},
                    {1184.48511F, 546.999939F, -1000.09155F},
                    {1185.11804F, 544.999939F, -970.028198F},
                    {1182.4751F, 544.999939F, -1000.1178F},
                    {1096.69739F, 544.999939F, -997.994263F},
                    {1189.51038F, 576.999939F, -1000.02594F},
                    {1194.0F, 540.0F, -907.0F},
                    {1194.0F, 540.0F, -1047.0F},
                    {1054.0F, 540.0F, -907.0F}},
         .faces = {{0, 1, 2}, {2, 1, 3}, {2, 3, 4}, {0, 5, 1}, {6, 7, 8}},
         .material = {0x9428a400, 0x9428a400, 0x94282400, 0x94282400,
                      0x94302400},
         .count = 5,
         .flags = LAND_DRAWN | WORLD_LARGE | WORLD_SOLID,
         .start = {1180.94427F, 544.999939F, -992.40290F},
         .normal = {0, 1, 0},
         .velocity = {3.48657615F, 0, -0.30624624F},
         .end = {1180.94427F, 544.999939F, -992.40290F},
         .outgoing = {0},
         .yaw = 914,
         .grounded = TRUE},
        {.points = {{500.000061F, 531.923767F, -834.0F},
                    {500.000061F, 531.798096F, -789.999939F},
                    {504.07428F, 529.374939F, -836.764648F},
                    {506.223053F, 529.793762F, -787.390625F},
                    {494.0F, 540.0F, -907.0F},
                    {494.0F, 540.0F, -767.0F},
                    {634.0F, 540.0F, -907.0F}},
         .faces = {{0, 1, 2}, {1, 3, 2}, {4, 5, 6}},
         .material = {0x9428a400, 0x9428a400, 0x94302400},
         .count = 3,
         .flags = LAND_DRAWN | WORLD_MEDIUM | WORLD_SOLID,
         .start = {503.098072F, 530.507527F, -801.474468F},
         .normal = {0.31475327F, 0.94892435F, -0.02174757F},
         .velocity = {-3.30822657F, 1.08942242F, -0.34466760F},
         .end = {498.901519F, 532.521545F, -801.944396F},
         .outgoing = {-2.94904875F, 1.85164582F, -0.35287073F},
         .yaw = 31685,
         .pitch = 65309,
         .roll = 62195,
         .grounded = FALSE,
         .test_gamma = TRUE},
    };
    for (unsigned i = 0; i < sizeof cases / sizeof *cases; i++) {
        int count = cases[i].count;
        NJS_MATERIAL materials[5] = {0};
        NJS_MESHSET sets[5] = {0};
        for (int j = 0; j < count; j++) {
            materials[j].attrflags = cases[i].material[j];
            sets[j] = (NJS_MESHSET){.type_matId = NJD_MESHSET_3 | j,
                                    .nbMesh = 1,
                                    .meshes = cases[i].faces[j]};
        }
        NJS_MODEL models[] = {{.points = cases[i].points,
                               .nbPoint = 9,
                               .mats = materials,
                               .nbMat = count,
                               .meshsets = sets,
                               .nbMeshset = count - 1},
                              {.points = cases[i].points,
                               .nbPoint = 9,
                               .mats = materials,
                               .nbMat = count,
                               .meshsets = sets + count - 1,
                               .nbMeshset = 1}};
        NJS_OBJECT geometry[] = {{.model = models, .scl = {1, 1, 1}},
                                 {.model = models + 1, .scl = {1, 1, 1}}};
        static const Uint32 liquid[] = {
            0, WORLD_WATER, WORLD_WATER | WORLD_SOLID, WORLD_THICK_LIQUID,
            WORLD_THICK_LIQUID | WORLD_SOLID};
        for (int actor = 0; actor <= cases[i].test_gamma; actor++) {
            Bool gamma = actor == 1;
            int character = gamma ? CHARACTER_E102 : CHARACTER_SONIC;
            int mode =
                gamma ? e102_terrain_modes.walk : sonic_terrain_modes.walk;
            for (unsigned j = 0; j < sizeof liquid / sizeof *liquid; j++) {
                collision_mesh_init();
                world_count = liquid[j] ? 2 : 1;
                world_objects[0] = (struct world_object){
                    .object = geometry, .flags = cases[i].flags};
                world_objects[1] = (struct world_object){
                    .object = geometry + 1,
                    .flags = LAND_DRAWN | WORLD_LARGE | liquid[j]};
                collision_mesh_update();
                begin_stride(mode, cases[i].start, 3.5F);
                struct task_work *work = player_work[0];
                struct task_motion *motion = player_motion[0];
                struct player *record = player_records[0];
                PLAYER_VARS(work)->character = character;
                record->consts = character_constants[character];
                record->ground_object = geometry;
                record->ground_flags = record->last_ground_flags =
                    cases[i].flags;
                record->ground_normal = cases[i].normal;
                work->ang.x = cases[i].pitch;
                work->ang.y = cases[i].yaw;
                work->ang.z = gamma ? -3340 : cases[i].roll;
                motion->aim = work->ang;
                record->contact->normal = cases[i].normal;
                motion->speed = cases[i].velocity;
                Bool grounded = gamma || cases[i].grounded;
                NJS_OBJECT *support = gamma && liquid[j] ? geometry + 1
                                      : grounded         ? geometry
                                                         : NULL;
                NJS_POINT3 end = cases[i].end;
                if (gamma) {
                    end = liquid[j] ? (NJS_POINT3){500.615783F, 539.969665F,
                                                   -801.873168F}
                                    : (NJS_POINT3){499.200897F, 532.332397F,
                                                   -801.908203F};
                }
                if (move_in_world(work, motion, record) || work->mode != mode ||
                    !!(work->flags & PLAYER_ON_GROUND) != grounded ||
                    record->ground_object != support ||
                    !near_point(work->pos, end) ||
                    !near_point(motion->speed, gamma ? cases[i].velocity
                                                     : cases[i].outgoing) ||
                    (gamma &&
                     (record->ground_flags & (WORLD_LIQUID | WORLD_SOLID)) !=
                         (WORLD_SOLID | liquid[j]))) {
                    return 1 + 10 * i + 5 * actor + j;
                }
            }
        }
    }
    world_count = 0;
    collision_mesh_init();
    return 0;
}

static Bool stride(int mode, Float drift, NJS_POINT3 end, NJS_VECTOR speed,
                   NJS_OBJECT *owner)
{
    begin_stride(mode, (NJS_POINT3){-4, drift, -0.25F}, 12);
    struct task_work *work = player_work[0];
    struct task_motion *motion = player_motion[0];
    struct player *record = player_records[0];
    record->contact->hit.normal_speed = (NJS_VECTOR){17, 19, 23};
    Uint32 result = move_in_world(work, motion, record);
    return near_point(record->contact->hit.normal_speed, (NJS_VECTOR){0}) &&
           !result && !!(work->flags & PLAYER_ON_GROUND) == !!owner &&
           record->ground_object == owner && near_point(work->pos, end) &&
           near_point(motion->speed, speed);
}

static Bool ground_banking(void)
{
    if (!ground_strip(STRIP_STEEP, 1, 0, 1, 1)) {
        return FALSE;
    }
    begin_stride(2, (NJS_POINT3){-1.7F, 0, -0.25F}, 1);
    struct task_work *work = player_work[0];
    struct task_motion *motion = player_motion[0];
    struct player *record = player_records[0];
    for (int tick = 1; tick <= 5; tick++) {
        record->last_ground_flags = record->ground_flags;
        record->ground_flags = 0;
        if (move_in_world(work, motion, record) ||
            !(work->flags & PLAYER_ON_LAND) ||
            record->ground_object != &objects[1]) {
            return FALSE;
        }
        player_take_speed(work, motion, record);
        /* tan(atan(3/4)/2) is 1/3. The support point crosses two
         * radius/3 offsets when the center turns onto this face. */
        Float distance = -1.7F + tick + 10.0F / 3.0F;
        if (!near_point(work->pos, (NJS_POINT3){0.8F * distance,
                                                0.6F * distance, -0.25F}) ||
            !near_point(record->speed, (NJS_VECTOR){1, 0, 0})) {
            return FALSE;
        }
        player_apply_accel(work, motion, record);
    }
    return TRUE;
}

/* An eleven-degree bend repeats from level ground through an inverted
 * ceiling. The guide keeps its incoming speed as it reaches the first bend. */
static Bool run_loop(int split, Bool guided, Bool back)
{
    enum { panels = 17, turn = 0x800 };
    const Float radius = 80, speed = 12;
    collision_mesh_init();
    int count = split ? panels : 1;
    for (int i = 0; i < count; i++) {
        world_objects[i] = (struct world_object){.flags = WORLD_SOLID};
    }
    if (!update(count, 0)) {
        return FALSE;
    }
    for (struct collision_mesh *mesh = collision_mesh_list; mesh;
         mesh = mesh->next) {
        collision_mesh_clear_buckets();
        for (int i = 0; i < panels; i++) {
            if (split && mesh->object != &objects[i]) {
                continue;
            }
            Float x = i ? radius * njSin((i - 1) * turn) : -32;
            Float y = i ? radius * (1 - njCos((i - 1) * turn)) : 0;
            Float end_x = radius * njSin(i * turn);
            Float end_y = radius * (1 - njCos(i * turn));
            Float z = back ? -32 : 32;
            NJS_POINT3 v[] = {
                {x, y, -z}, {x, y, z}, {end_x, end_y, z}, {end_x, end_y, -z}};
            ground_quad(v, i & 1, split, back ? NJD_FLAG_DOUBLE_SIDE : 0);
        }
        mesh->span = 192;
        collision_mesh_take_buckets(mesh);
    }
    begin_stride(sonic_terrain_modes.walk, (NJS_POINT3){-8, 0, 1}, speed);
    struct task_work *work = player_work[0];
    struct task_motion *motion = player_motion[0];
    struct player *record = player_records[0];
    struct collision_work collision = {0};
    work->collision = &collision;
    if (guided) {
        work->flags |= PLAYER_ON_PATH;
    }
    for (int tick = 0; tick < 24; tick++) {
        if (player_collide(work, motion, record) ||
            !(work->flags & PLAYER_ON_LAND) ||
            !!(work->flags & PLAYER_ON_PATH) != !!guided) {
            return FALSE;
        }
        NJS_VECTOR up = record->ground_normal;
        if (guided) {
            if (up.y < 1) {
                /* The guide gets the next step with its approach velocity.
                 * Its path task supplies the steering in the stage. */
                return near_point(motion->speed, (NJS_VECTOR){speed, 0, 0});
            }
        } else if (njAbs(njScalor(&motion->speed) - speed) > 0.01F ||
                   njAbs(njInnerProduct(&motion->speed, &up)) > 0.005F ||
                   njAbs((work->pos.x * up.x + (work->pos.y - radius) * up.y) +
                         radius * njCos(turn / 2)) > 0.01F) {
            return FALSE;
        }
        if (up.y < -0.95F) {
            /* Gravity draws a slow runner away from the ceiling. */
            player_take_speed(work, motion, record);
            record->speed = motion->speed = (NJS_VECTOR){0};
            player_accel_ground(work, motion, record);
            player_apply_accel(work, motion, record);
            player_collide(work, motion, record);
            return !(work->flags & PLAYER_ON_GROUND);
        }
        player_take_speed(work, motion, record);
        player_apply_accel(work, motion, record);
    }
    return FALSE;
}

/* The same quarter circle can spend one movement tick crossing more
 * faces than fit in a simultaneous contact set. */
static Bool make_terrain_bend(int panels, Bool wall)
{
    const Float radius = 20;
    collision_mesh_init();
    world_objects[0] = (struct world_object){.flags = WORLD_SOLID};
    if (!update(1, 0)) {
        return FALSE;
    }
    collision_mesh_clear_buckets();
    Float x = -30, rise = 0;
    for (int i = 0; i <= panels; i++) {
        Angle angle = i * 0x4000 / panels;
        Float end_x = radius * njSin(angle);
        Float end_rise = radius * (1 - njCos(angle));
        NJS_POINT3 face[] = {{x, rise, -32},
                             {x, rise, 32},
                             {end_x, end_rise, 32},
                             {end_x, end_rise, -32}};
        if (wall) {
            for (int j = 0; j < 4; j++) {
                Float y = face[j].y;
                face[j].y = -face[j].z;
                face[j].z = y;
            }
        }
        ground_quad(face, (i + 1) & 1, 0, 0);
        x = end_x;
        rise = end_rise;
    }
    collision_mesh_list->span = 256;
    collision_mesh_take_buckets(collision_mesh_list);
    return TRUE;
}

static Bool many_face_bend(int panels, Bool wall)
{
    const Float speed = 20;
    if (!make_terrain_bend(panels, wall)) {
        return FALSE;
    }
    struct collision_work collision = {0};
    NJS_POINT3 start =
        wall ? (NJS_POINT3){-1, -2.5F, 5.001F} : (NJS_POINT3){-1, 0, 0.25F};
    begin_stride(wall ? sonic_terrain_modes.jump : sonic_terrain_modes.walk,
                 start, speed);
    struct task_work *work = player_work[0];
    struct task_motion *motion = player_motion[0];
    struct player *record = player_records[0];
    work->collision = &collision;
    if (wall) {
        work->flags = record->last_flags = 0;
        record->ground_object = NULL;
        record->ground_flags = record->last_ground_flags = 0;
        move_in_world(work, motion, record);
        rider_table.count = 1;
        rider_land[0] = (struct land_entry){
            .flags = WORLD_SOLID, .object = objects, .radius = 128};
        NJS_POINT3 center = {start.x, start.y + 5, start.z};
        NJS_VECTOR carry = {speed, 0, 0};
        Bool finished = polygon_limit_carry(&center, 5, &carry);
        rider_table.count = 0;
        if (!finished ||
            !near_point(work->pos,
                        (NJS_POINT3){start.x + carry.x, start.y + carry.y,
                                     start.z + carry.z})) {
            return FALSE;
        }
        /* The wall bends the velocity in XZ. The height stays fixed. */
        return !(work->flags & PLAYER_ON_GROUND) &&
               njAbs(work->pos.y - start.y) < 0.0001F && work->pos.x > 13.5F &&
               work->pos.x < 14.6F && work->pos.z > 14.4F &&
               work->pos.z < 15.8F && motion->speed.x > 5 &&
               motion->speed.x < 7 && motion->speed.z > 17 &&
               motion->speed.z < 19;
    }
    player_collide(work, motion, record);
    return (work->flags & PLAYER_ON_LAND) &&
           record->ground_object == &objects[0] &&
           njAbs(work->pos.z - start.z) < 0.0001F && work->pos.x > 18.5F &&
           work->pos.x < 19.5F && work->pos.y > 13.4F && work->pos.y < 14.7F &&
           njAbs(njScalor(&motion->speed) - speed) < 0.01F &&
           njAbs(njInnerProduct(&motion->speed, &record->ground_normal)) <
               0.005F;
}

/* A stride on a vertical wall preserves its speed across the lip.
 * A coplanar panel past a one-unit gap cannot keep the runner grounded. */
static Bool vertical_lip(int axis, Bool descending, Bool gap)
{
    collision_mesh_init();
    world_objects[0] = (struct world_object){.flags = WORLD_SOLID};
    if (!update(1, 0)) {
        return FALSE;
    }
    collision_mesh_clear_buckets();
    Float side = axis & 2 ? -16 : 16;
    Float x = axis & 1 ? side : 0, z = axis & 1 ? 0 : side;
    for (int i = 0; i < (gap ? 2 : 1); i++) {
        Float y = i ? (descending ? -13 : 13) : 0;
        NJS_POINT3 face[] = {
            {-x, y, -z}, {x, y, z}, {x, y + 12, z}, {-x, y + 12, -z}};
        ground_quad(face, i, 0, 0);
    }
    collision_mesh_list->span = 64;
    collision_mesh_take_buckets(collision_mesh_list);
    NJS_POINT3 feet = {x / 16, 6, z / 16};
    NJS_VECTOR up = {-z / 16, 0, x / 16};
    NJS_VECTOR velocity = {0, descending ? -4 : 4, 0};
    begin_stride(sonic_terrain_modes.walk, feet, 4);
    struct task_work *work = player_work[0];
    struct task_motion *motion = player_motion[0];
    struct player *record = player_records[0];
    struct collision_work collision = {0};
    work->collision = &collision;
    record->stride_object = record->ground_object;
    record->stride_feet = feet;
    record->stride_normal = record->ground_normal = up;
    motion->speed = velocity;
    for (int tick = 0; tick < 2; tick++) {
        feet.y += velocity.y;
        if (player_collide(work, motion, record) ||
            (work->flags & PLAYER_ON_GROUND) != (tick ? 0 : PLAYER_ON_LAND) ||
            record->ground_object != (tick ? NULL : &objects[0]) ||
            !near_point(work->pos, feet) ||
            !near_point(motion->speed, velocity)) {
            return FALSE;
        }
    }
    return TRUE;
}

/* Speed Highway's wall can be returned as ground by the side probes.
 * A runner arriving there keeps the character frame until a running
 * surface actually leads onto the wall. */
static Bool inherited_wall(void)
{
    begin_stride(sonic_terrain_modes.walk, (NJS_POINT3){0}, 2);
    struct task_work *work = player_work[0];
    struct task_motion *motion = player_motion[0];
    struct player *record = player_records[0];
    record->ground_normal = (NJS_VECTOR){-1, 0, 0};
    player_apply_accel(work, motion, record);
    if (!near_point(motion->speed, (NJS_VECTOR){2, 0, 0})) {
        return FALSE;
    }
    record->stride_object = record->ground_object;
    record->stride_normal = record->ground_normal;
    record->stride_feet = work->pos;
    player_apply_accel(work, motion, record);
    if (!near_point(motion->speed, (NJS_VECTOR){0, 2, 0})) {
        return FALSE;
    }
    work->pos.z += 1;
    player_apply_accel(work, motion, record);
    return near_point(motion->speed, (NJS_VECTOR){2, 0, 0});
}

/* The supporting material changes halfway up the ramp. Crossing the join
 * preserves the stride and selects the new face's traction and effects. */
static Bool material_stride(Uint32 from, Uint32 to, int reverse)
{
    const struct polygon_ball body = {.pos = {-4, 0, -0.25F},
                                      .radius = 5,
                                      .speed = {12, 0, 0},
                                      .normal = {0, 1, 0}};
    const NJS_POINT3 end = {8.95544F, 2.23886F, -0.25F};
    world_objects[reverse].flags = from;
    world_objects[!reverse].flags = to;
    struct ground_step step;
    if (!polygon_follow_ground(&body, objects, 2, 0.47F, 0.08F, 0, 0, &step) ||
        step.support != objects + 1 || step.flags != to ||
        !near_point(step.feet, end)) {
        return FALSE;
    }
    begin_stride(sonic_terrain_modes.walk, body.pos, body.speed.x);
    struct task_work *work = player_work[0];
    struct player *record = player_records[0];
    struct task_motion *motion = player_motion[0];
    struct collision_work collision = {0};
    work->collision = &collision;
    record->ground_flags = from;
    return !player_collide(work, motion, record) &&
           record->ground_object == objects + 1 && record->ground_flags == to &&
           motion->gravity == (to & WORLD_GRIP_HIGH ? 1.5F : 1.0F) &&
           near_point(work->pos, end);
}

static Bool material_strides(void)
{
    static const Uint32 tags[] = {
        WORLD_GRIP_HIGH, WORLD_FOOTPRINTS,
        WORLD_DIGGABLE,  WORLD_UNCLIMBABLE,
        LAND_FAR,        WORLD_GRIP_HIGH | WORLD_FOOTPRINTS};
    for (int reverse = 0; reverse < 2; reverse++) {
        if (!ground_strip(STRIP_RAMP, 1, reverse, reverse, 1)) {
            return FALSE;
        }
        for (unsigned i = 0; i < sizeof tags / sizeof *tags; i++) {
            Uint32 flags = WORLD_SOLID | tags[i];
            if (!material_stride(WORLD_SOLID, flags, reverse) ||
                !material_stride(flags, WORLD_SOLID, reverse)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

static int ground_strides(void)
{
    for (int shape = STRIP_FLAT; shape <= STRIP_GAP; shape++) {
        for (int bits = 0; bits < 16; bits++) {
            int split = bits & 1;
            if (!ground_strip(shape, split, (bits >> 1) & 1, (bits >> 2) & 1,
                              bits & 8 ? 8 : 1)) {
                return 1;
            }
            NJS_POINT3 end = {8, 0, -0.25F};
            NJS_VECTOR speed = {12, 0, 0};
            NJS_OBJECT *owner = shape == STRIP_FLAT ? &objects[split] : NULL;
            if (shape == STRIP_RAMP) {
                /* A radius-five sphere first touches the rising face at
                 * x = -5*tan(atan(1/4)/2). Carry the remaining distance
                 * along its unit tangent, then subtract the new radius. */
                end = (NJS_POINT3){8.95544F, 2.23886F, -0.25F};
                speed = (NJS_VECTOR){11.64171F, 2.91043F, 0};
                owner = &objects[split];
            }
            for (int mode = 0; mode < 256; mode++) {
                if (player_terrain_motion(CHARACTER_SONIC, mode) ==
                        PLAYER_TERRAIN_STRIDE &&
                    (!stride(mode, 0, end, speed, owner) ||
                     !stride(mode, 0.125F, end, speed, owner))) {
                    return 2 + shape;
                }
            }
        }
    }
    return 0;
}

/* A slow approach can roll around a convex edge under gravity. The same
 * approach across an open ledge has no downhill face to follow. */
static int ground_departures(void)
{
    struct polygon_ball body = {.pos = {-0.125F, 0, -0.25F},
                                .radius = 5,
                                .normal = {0, 1, 0},
                                .speed = {0.25F, 0, 8}};
    struct ground_step step = {.feet = {1, 2, 3}};
    if (!ground_strip(STRIP_CREST, 1, 0, 1, 1) ||
        polygon_follow_ground(&body, objects, 2, 0.47F, 0.08F, 0, 0, &step) ||
        !near_point(step.feet, (NJS_POINT3){1, 2, 3})) {
        return 1;
    }
    if (!ground_strip(STRIP_LEDGE, 1, 0, 1, 1) ||
        !polygon_follow_ground(&body, objects, 2, 0.47F, 0.08F, 0, 0, &step) ||
        step.support ||
        !near_point(step.feet, (NJS_POINT3){0.125F, 0, 7.75F})) {
        return 2;
    }
    body.speed.x = 2;
    if (!ground_strip(STRIP_CREST, 1, 1, 0, 1) ||
        !polygon_follow_ground(&body, objects, 2, 0.47F, 0.08F, 0, 0, &step) ||
        step.support || !near_point(step.velocity, body.speed)) {
        return 3;
    }
    body.speed = (NJS_VECTOR){2, 1, 0};
    step.feet = (NJS_POINT3){1, 2, 3};
    if (polygon_follow_ground(&body, objects, 2, 0.47F, 0.08F, 0, 0, &step) ||
        !near_point(step.feet, (NJS_POINT3){1, 2, 3})) {
        return 4;
    }
    body.speed = (NJS_VECTOR){2, 0, 0};
    if (!ground_strip(STRIP_FLAT, 1, 1, 0, 1)) {
        return 5;
    }
    world_objects[1].flags |= WORLD_DEFLECT;
    if (polygon_follow_ground(&body, objects, 2, 0.47F, 0.08F, 0, 0, &step) ||
        !near_point(step.feet, (NJS_POINT3){1, 2, 3})) {
        return 6;
    }
    /* Turn the same convex crest upside down. Gravity pulls the runner
     * away throughout the bend, even at a slow crossing speed. */
    if (!ground_strip(STRIP_CREST, 1, 0, 1, 1)) {
        return 7;
    }
    for (struct collision_mesh *mesh = collision_mesh_list; mesh;
         mesh = mesh->next) {
        for (struct polygon *tri = mesh->polygons; tri; tri = tri->next) {
            tri->center.y = -tri->center.y;
            tri->center.z = -tri->center.z;
            for (int i = 0; i < 3; i++) {
                tri->vertex[i].z = -tri->vertex[i].z;
                tri->vertex[i].y = -tri->vertex[i].y;
            }
            if (tri == mesh->last) {
                break;
            }
        }
    }
    body.normal = (NJS_VECTOR){0, -1, 0};
    body.speed = (NJS_VECTOR){0.25F, 0, 8};
    if (!polygon_follow_ground(&body, objects, 2, 0.47F, 0.08F, 0, 0, &step) ||
        step.support || !near_point(step.velocity, body.speed)) {
        return 8;
    }
    /* Four crests lie inside one tick. Each turn needs about three quarters
     * of that tick's gravity, so only the first can retain support. */
    collision_mesh_init();
    world_objects[0] = (struct world_object){.flags = WORLD_SOLID};
    if (!update(1, 0)) {
        return 9;
    }
    collision_mesh_clear_buckets();
    ground_tile(-32, 0, -8, 8, 0, 0, 0);
    Float x = 0, y = 0;
    for (int i = 1; i <= 4; i++) {
        Float width = i == 4 ? 16 : 0.5F;
        Float end_y = y - width * (0.03F * i);
        NJS_POINT3 tile[] = {{x, y, -8},
                             {x, y, 8},
                             {x + width, end_y, 8},
                             {x + width, end_y, -8}};
        ground_quad(tile, i & 1, 0, 0);
        x += width;
        y = end_y;
    }
    collision_mesh_list->span = 64;
    collision_mesh_take_buckets(collision_mesh_list);
    body = (struct polygon_ball){.pos = {-0.1F, 0, 0.75F},
                                 .speed = {2, 0, 0},
                                 .radius = 5,
                                 .normal = {0, 1, 0}};
    if (!polygon_follow_ground(&body, objects, 2, 0.47F, 0.08F, 0, 0, &step) ||
        step.support || step.velocity.y >= -0.05F || step.velocity.y < -0.08F) {
        return 10;
    }
    return 0;
}

/* Alternating strips arrive before their neighbors. A bounded interval
 * workspace must still distinguish a late bridge from a missing tile. */
static int ground_many_tiles(void)
{
    for (int gap = 0; gap < 2; gap++) {
        for (int order = 0; order < 2; order++) {
            collision_mesh_init();
            world_objects[0] = (struct world_object){.flags = WORLD_SOLID};
            if (!update(1, 0)) {
                return 1;
            }
            collision_mesh_clear_buckets();
            for (int i = 0; i < 64; i++) {
                int tile = (i % 32) * 2 + (i / 32);
                if (order) {
                    tile = 63 - tile;
                }
                if (!gap || tile != 49) {
                    ground_tile(tile, tile + 1, -4, 4, 0, i & 1, order);
                }
            }
            collision_mesh_list->span = 64;
            collision_mesh_take_buckets(collision_mesh_list);
            struct polygon_ball body = {.pos = {0.5F, 0, 0},
                                        .normal = {0, 1, 0},
                                        .radius = 5,
                                        .speed = {63, 0, 0}};
            struct ground_step step;
            if (!polygon_follow_ground(&body, objects, 2, 0.47F, 0.08F, 0, 0,
                                       &step) ||
                !!step.support == gap ||
                !near_point(step.feet, (NJS_POINT3){63.5F, 0, 0})) {
                return 2;
            }
        }
    }
    return 0;
}

/* A ramp whose edge stops short of the old floor, or meets it at a single
 * vertex, cannot transport a grounded character onto the new sheet. */
static Bool ground_requires_join(void)
{
    for (int vertex_only = 0; vertex_only <= 1; vertex_only++) {
        collision_mesh_init();
        world_objects[0] = (struct world_object){.flags = WORLD_SOLID};
        if (!update(1, 0)) {
            return FALSE;
        }
        collision_mesh_clear_buckets();
        ground_tile(-32, 0, -32, vertex_only ? 0 : 32, 0, 0, 0);
        ground_tile(vertex_only ? 0 : 0.25F, 32, vertex_only ? 0 : -32, 32,
                    0.25F, 0, 0);
        collision_mesh_list->span = 64;
        collision_mesh_take_buckets(collision_mesh_list);
        const struct polygon_ball ball = {.radius = 5,
                                          .normal = {0, 1, 0},
                                          .pos = {-4, 0, 0},
                                          .speed = {12, 0, 0}};
        struct ground_step step = {.feet = {1, 2, 3}};
        if (polygon_follow_ground(&ball, objects, 2, 0.47F, 0.08F, 0, 0,
                                  &step) ||
            !near_point(step.feet, (NJS_POINT3){1, 2, 3})) {
            return FALSE;
        }
    }
    return TRUE;
}

/* The two wide panels meet at less than one degree. Cycling a triangle's
 * first vertex used to make the same shared edge disappear. */
static Bool ground_shallow_bend(void)
{
    static const NJS_POINT3 vertices[] = {{371, 186, -682},
                                          {-781, -198, 86},
                                          {-589, -198, 374},
                                          {563, 186, -394},
                                          {-398.125F, -193.125F, 662.75F},
                                          {753.875F, 190.875F, -105.25F}};
    static const Uint8 faces[][3] = {
        {0, 1, 2}, {0, 2, 3}, {3, 2, 4}, {3, 4, 5}};
    const struct polygon_ball body = {
        .radius = 5,
        .pos = {-15.018354F, -5.933185F, -13.461832F},
        .normal = {-0.222375F, 0.963624F, 0.14825F},
        .speed = {6.237753F, -0.13955F, 10.263703F}};
    for (int first = 0; first < 3; first++) {
        collision_mesh_init();
        world_objects[0] = (struct world_object){.flags = WORLD_SOLID};
        if (!update(1, 0)) {
            return FALSE;
        }
        collision_mesh_clear_buckets();
        for (int i = 0; i < 4; i++) {
            struct polygon *tri = polygon_alloc(0);
            for (int j = 0; j < 3; j++) {
                tri->vertex[j] = vertices[faces[i][(first + j) % 3]];
            }
            tri->center = body.pos;
            tri->radius = 1000000;
            tri->attr = 0;
        }
        collision_mesh_list->span = 1200;
        collision_mesh_take_buckets(collision_mesh_list);
        struct ground_step step;
        if (!polygon_follow_ground(&body, objects, 2, 0.47F, 0.08F, 0, 0,
                                   &step) ||
            step.support != objects ||
            !near_point(step.feet,
                        (NJS_POINT3){-8.76679F, -5.95903F, -3.12055F}) ||
            !near_point(step.velocity,
                        (NJS_VECTOR){6.19804F, 0.02944F, 10.28863F})) {
            return FALSE;
        }
    }
    return TRUE;
}

/* In Red Mountain's recording Knuckles crosses a diagonal inside a sloped
 * strip, then the crest at the top. Only the second turn launches him. */
static int mountain_crest(void)
{
    static const NJS_POINT3 vertices[] = {
        {-3566.42285F, 726.965393F, -1789.65979F},
        {-3480.32251F, 726.965393F, -1921.06494F},
        {-3574.19287F, 732.118469F, -1802.03064F},
        {-3492.79004F, 732.118469F, -1928.5896F},
        {-3515.13184F, 732.157837F, -1938.28674F},
        {-3591.63159F, 732.517151F, -1821.24609F},
    };
    static const Uint8 faces[][3] = {
        {0, 1, 2}, {2, 1, 3}, {3, 4, 2}, {2, 4, 5}};
    const NJS_POINT3 start = {-3545.49658F, 730.496338F, -1838.1991F};
    const NJS_VECTOR uphill = {-1.67151F, 0.77412F, -1.44353F};
    for (int layout = 0; layout < 12; layout++) {
        int split = layout & 1, reverse = layout & 2;
        collision_mesh_init();
        for (int i = 0; i <= split; i++) {
            world_objects[i] = (struct world_object){.flags = 0xa0000001U};
        }
        if (!update(1 + split, 0)) {
            return 1;
        }
        for (struct collision_mesh *mesh = collision_mesh_list; mesh;
             mesh = mesh->next) {
            collision_mesh_clear_buckets();
            for (int i = 0; i < 4; i++) {
                int f = reverse ? 3 - i : i;
                if (split && mesh->object != &objects[f != 0]) {
                    continue;
                }
                struct polygon *tri = polygon_alloc(0);
                for (int j = 0; j < 3; j++) {
                    tri->vertex[j] = vertices[faces[f][(j + layout / 4) % 3]];
                }
                tri->center = start;
                tri->radius = 32768;
                tri->attr = f < 2 ? 0x9430a400U : 0x94202400U;
            }
            mesh->span = 192;
            collision_mesh_take_buckets(mesh);
        }
        begin_stride(knuckles_terrain_modes.walk, start, 2.34F);
        struct task_work *work = player_work[0];
        struct task_motion *motion = player_motion[0];
        struct player *record = player_records[0];
        struct collision_work collision = {0};
        work->collision = &collision;
        PLAYER_VARS(work)->character = CHARACTER_KNUCKLES;
        record->consts = character_constants[CHARACTER_KNUCKLES];
        record->ground_normal =
            (NJS_VECTOR){0.302602261F, 0.932265818F, 0.198273495F};
        motion->speed = (NJS_VECTOR){-1.6736815F, 0.770172298F, -1.44507205F};
        for (int tick = 0; tick < 3; tick++) {
            if (player_collide(work, motion, record) ||
                !!(work->flags & PLAYER_ON_LAND) != (tick < 2) ||
                record->ground_object != (tick < 2 ? &objects[split] : NULL) ||
                !corner_near(0.001F, motion->speed, uphill)) {
                return 2 + tick;
            }
            if (!tick && !corner_near(0.003F, work->pos,
                                      (NJS_POINT3){-3547.04106F, 731.21762F,
                                                   -1839.54017F})) {
                return 5;
            }
            player_take_speed(work, motion, record);
        }
    }
    return 0;
}

/* The downhill tile is shallow enough to stay on under gravity. Only
 * a shared edge lets the player cross to it without becoming airborne. */
static int shallow_crest_edges(void)
{
    for (int join = 0; join < 3; join++) {
        collision_mesh_init();
        world_objects[0] = (struct world_object){.flags = WORLD_SOLID};
        if (!update(1, 0)) {
            return 1;
        }
        collision_mesh_clear_buckets();
        ground_tile(-32, 0, -32, join == 2 ? 0 : 32, 0, 0, 0);
        ground_tile(join == 1 ? 0.02F : 0, 32, join == 2 ? 0 : -32, 32, -0.03F,
                    1, 0);
        collision_mesh_list->span = 64;
        collision_mesh_take_buckets(collision_mesh_list);
        struct polygon_ball body = {.pos = {-1, 0, 0},
                                    .speed = {2, 0, 0},
                                    .normal = {0, 1, 0},
                                    .radius = 5};
        struct ground_step step;
        if (!polygon_follow_ground(&body, objects, 2, 0.47F, 0.08F, 0, 0,
                                   &step) ||
            step.support != (join ? NULL : objects)) {
            return 2 + join;
        }
        if (join && !near_point(step.velocity, body.speed)) {
            return 5;
        }
        if (!join) {
            /* The first tick ends between the two offset face planes. */
            begin_stride(sonic_terrain_modes.walk, (NJS_POINT3){-1.98F, 0, 0},
                         2);
            struct task_work *work = player_work[0];
            struct task_motion *motion = player_motion[0];
            struct player *record = player_records[0];
            struct collision_work collision = {0};
            work->collision = &collision;
            for (int tick = 0; tick < 3; tick++) {
                Float start_x = work->pos.x;
                if (player_collide(work, motion, record) ||
                    !(work->flags & PLAYER_ON_LAND) ||
                    record->ground_object != objects ||
                    work->pos.x - start_x < 1.8F) {
                    return 6;
                }
                player_take_speed(work, motion, record);
                player_apply_accel(work, motion, record);
            }
            if (njAbs(work->pos.y + 0.03F * work->pos.x) > 0.005F ||
                njAbs(motion->speed.y + 0.03F * motion->speed.x) > 0.005F ||
                njAbs(njScalor(&motion->speed) - 2) > 0.01F) {
                return 7;
            }
        }
    }
    return 0;
}

/* The first tick ends with the sphere touching the edge between both faces.
 * Dividing the strip between objects must preserve that contact on the next
 * tick, and switching to jump must release it. */
static int partial_crest_strides(Float start_x)
{
    struct stride_sample {
        NJS_POINT3 feet;
        NJS_VECTOR speed, normal;
    } reference[4];
    struct collision_work collision = {0};
    for (int layout = 0; layout < 4; layout++) {
        int split = layout & 1, reverse = layout >> 1;
        if (!ground_strip(STRIP_SHALLOW_CREST, split, reverse, reverse, 1)) {
            return 1;
        }
        for (int jump = 0; jump < 2; jump++) {
            begin_stride(sonic_terrain_modes.walk, (NJS_POINT3){start_x, 0, 0},
                         2);
            struct task_work *work = player_work[0];
            struct task_motion *motion = player_motion[0];
            struct player *record = player_records[0];
            work->collision = &collision;
            for (int tick = 0; tick < 4; tick++) {
                if (player_collide(work, motion, record)) {
                    return 2;
                }
                if (jump && tick == 1) {
                    if ((work->flags & PLAYER_ON_LAND) || motion->speed.y < 1 ||
                        work->pos.y < 1) {
                        return 3;
                    }
                    break;
                }
                if (!(work->flags & PLAYER_ON_LAND) || !record->ground_object ||
                    njAbs(njScalor(&motion->speed) - 2) > 0.001F) {
                    return 4;
                }
                if (!layout && !jump) {
                    reference[tick] = (struct stride_sample){
                        work->pos, motion->speed, record->ground_normal};
                } else if (!near_point(work->pos, reference[tick].feet) ||
                           !near_point(motion->speed, reference[tick].speed) ||
                           !near_point(record->ground_normal,
                                       reference[tick].normal)) {
                    return 5;
                }
                if (!tick && !jump) {
                    /* Retrace the turn before the contact reaches the slope. */
                    struct polygon_ball body = {.pos = work->pos,
                                                .speed = {-motion->speed.x,
                                                          -motion->speed.y,
                                                          -motion->speed.z},
                                                .normal = record->ground_normal,
                                                .radius = 5};
                    struct ground_step step;
                    if (!polygon_follow_ground(&body, record->ground_object, 2,
                                               0.47F, 0.08F, 0, 0, &step) ||
                        step.support != objects ||
                        !near_point(step.feet, (NJS_POINT3){start_x, 0, 0}) ||
                        !near_point(step.normal, (NJS_VECTOR){0, 1, 0}) ||
                        !near_point(step.velocity, (NJS_VECTOR){-2, 0, 0})) {
                        return 8;
                    }
                }
                player_take_speed(work, motion, record);
                if (jump) {
                    work->mode = sonic_terrain_modes.jump;
                    record->speed.y = record->consts.jump_speed;
                }
                player_apply_accel(work, motion, record);
            }
        }
    }
    if (!near_point(reference[0].feet, (NJS_POINT3){0, 0, 0}) ||
        reference[0].normal.x <= 0 || reference[0].normal.x >= 0.03F ||
        njAbs(njInnerProduct(&reference[0].speed, &reference[0].normal)) >
            0.0001F) {
        return 6;
    }
    /* The feet stay on the crest as the sphere rolls around it. The center
     * advances between the flat and downhill horizontal speeds either way. */
    Float previous = start_x;
    for (int tick = 0; tick < 4; tick++) {
        Float center = reference[tick].feet.x + 5 * reference[tick].normal.x;
        Float travel = center - previous;
        if (travel < 2 / njSqrt(1 + 0.03F * 0.03F) - 0.001F ||
            travel > 2.001F ||
            njAbs(reference[tick].feet.y + 0.03F * reference[tick].feet.x) >
                0.0001F) {
            return 7;
        }
        previous = center;
    }
    /* A wall just beyond the outward edge of the sphere interrupts the
     * remaining turn before the center reaches the downslope offset plane. */
    collision_mesh_init();
    world_objects[0] = (struct world_object){.flags = WORLD_SOLID};
    if (!update(1, 0)) {
        return 9;
    }
    collision_mesh_clear_buckets();
    ground_tile(-32, 0, -32, 32, 0, 0, 0);
    ground_tile(0, 32, -32, 32, -0.03F, 0, 0);
    const NJS_POINT3 wall[] = {{5.145F, -16, -32},
                               {5.145F, -16, 32},
                               {5.145F, 32, 32},
                               {5.145F, 32, -32}};
    ground_quad(wall, 0, 0, NJD_FLAG_DOUBLE_SIDE);
    collision_mesh_list->span = 64;
    collision_mesh_take_buckets(collision_mesh_list);
    struct polygon_ball body = {.pos = reference[0].feet,
                                .speed = reference[0].speed,
                                .normal = reference[0].normal,
                                .radius = 5};
    struct ground_step step;
    if (polygon_follow_ground(&body, objects, 2, 0.47F, 0.08F, 5, 0, &step) &&
        step.feet.x + 5 * step.normal.x > wall[0].x - 5 + 0.0001F) {
        return 10;
    }
    return 0;
}

/* The floor and wall can meet on either side of an object boundary. */
static Bool landing_corner(int split, int diagonal, int reverse)
{
    collision_mesh_init();
    for (int i = 0; i < 2; i++) {
        world_objects[i] = (struct world_object){.flags = WORLD_SOLID};
    }
    if (!update(1 + split, 0)) {
        return FALSE;
    }
    for (struct collision_mesh *mesh = collision_mesh_list; mesh;
         mesh = mesh->next) {
        collision_mesh_clear_buckets();
        if (!split || mesh->object == objects) {
            ground_tile(-64, 64, -64, 64, 0, diagonal, reverse);
        }
        if (!split || mesh->object == objects + 1) {
            for (int i = 0; i < 2; i++) {
                int a = diagonal + (i ^ reverse) + 1;
                air_wall_triangle(0, diagonal, a % 4, (a + 1) % 4);
            }
        }
        mesh->span = 64;
        collision_mesh_take_buckets(mesh);
    }
    if (split && reverse) {
        world_objects[0].object = objects + 1;
        world_objects[1].object = objects;
    }
    return TRUE;
}

/* Repartitioning a level floor and wall must preserve both the tangent and
 * support. Align the player to the approach so his local stride also checks
 * the heading used by acceleration on the following tick. */
static Bool ground_wall_strides(void)
{
    for (int split = 0; split <= 1; split++) {
        for (int diagonal = 0; diagonal <= 1; diagonal++) {
            for (int reverse = 0; reverse <= 1; reverse++) {
                if (!landing_corner(split, diagonal, reverse)) {
                    return FALSE;
                }
                for (int sign = -1; sign <= 1; sign += 2) {
                    begin_stride(2, (NJS_POINT3){5.8F, 0, -8}, 4);
                    struct task_work *work = player_work[0];
                    struct task_motion *motion = player_motion[0];
                    struct player *record = player_records[0];
                    motion->speed = (NJS_VECTOR){-1, 0, sign * 4};
                    work->ang.y = motion->aim.y =
                        sign * 0x49fb; /* heading of (-1, 4) */
                    if (move_in_world(work, motion, record) ||
                        !near_point(work->pos,
                                    (NJS_POINT3){5, 0, -8 + sign * 4}) ||
                        !near_point(motion->speed,
                                    (NJS_VECTOR){0, 0, sign * 4}) ||
                        !near_point(record->wall_normal,
                                    (NJS_VECTOR){1, 0, 0}) ||
                        !(work->flags & PLAYER_ON_LAND) ||
                        record->ground_object != objects ||
                        work->ang.y != motion->aim.y ||
                        angle_distance(work->ang.y, sign * 0x4000) > 2) {
                        return FALSE;
                    }
                    player_take_speed(work, motion, record);
                    if (njAbs(record->speed.x - 4) > 0.001F ||
                        record->speed.z != 0) {
                        return FALSE;
                    }
                }
                /* The character still owns the direct, grounded stop. */
                begin_stride(2, (NJS_POINT3){6, 0, -8}, 4);
                struct task_work *work = player_work[0];
                struct task_motion *motion = player_motion[0];
                motion->speed = (NJS_VECTOR){-4, 0, 0};
                if (move_in_world(work, motion, player_records[0]) ||
                    !near_point(work->pos, (NJS_POINT3){6, 0, -8}) ||
                    !near_point(motion->speed, (NJS_VECTOR){0})) {
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}

/* The center turns onto an incline before reaching the wall at X=7.
 * Blocking X leaves the uphill velocity pointing away from the ramp. */
static int uphill_wall(void)
{
    static NJS_POINT3 points[] = {
        {-45, 0, -40},  {-45, 0, 40},    {0, 0, 40},     {0, 0, -40},
        {50, 25, 40},   {50, 25, -40},   {7, 3.5F, -40}, {7, 3.5F, 40},
        {7, 37.5F, 40}, {7, 37.5F, -40}, {-32, -10, 13}, {32, -10, 13},
        {32, 34, 13},   {-32, 34, 13}};
    static Sint16 treads[] = {0, 1, 2, 0, 2, 3, 2, 4, 5, 2, 5, 3};
    static Sint16 face[] = {0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7};
    static NJS_MATERIAL material = {.attrflags = NJD_FLAG_DOUBLE_SIDE};
    static NJS_MESHSET sets[] = {
        {.type_matId = NJD_MESHSET_3, .nbMesh = 4, .meshes = treads},
        {.type_matId = NJD_MESHSET_3, .nbMesh = 2, .meshes = face}};
    static NJS_MODEL models[] = {{.points = points,
                                  .nbPoint = 6,
                                  .meshsets = sets,
                                  .nbMeshset = 1,
                                  .r = 84},
                                 {.points = points + 6,
                                  .nbPoint = 8,
                                  .meshsets = sets + 1,
                                  .nbMeshset = 1,
                                  .mats = &material,
                                  .nbMat = 1,
                                  .r = 80}};
    static NJS_OBJECT land = {.model = models, .scl = {1, 1, 1}};
    static NJS_OBJECT wall = {.model = models + 1, .scl = {1, 1, 1}};
    static const struct {
        Float slope, start, forward, side;
        int walls, result;
    } cases[] = {{0.5F, -4, 20, 0, 0, 0},
                 {0.5F, -4, 10, 0, 1, 1},
                 {0.5F, -4, 20, 0, 1, PLAYER_CRASHED},
                 {0.5F, -2, 4.75F, 8, 1, 0},
                 {1, -4, 10, 0, 1, 1},
                 {0.5F, -2, 4.75F, 8, 2, 0}};
    for (int layout = 0; layout < 4; layout++) {
        int sign = (layout & 1) ? -1 : 1;
        land.ang[1] = wall.ang[1] = sign < 0 ? 0x8000 : 0;
        for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
            Float slope = cases[i].slope;
            points[4].y = points[5].y = 50 * slope;
            points[6].y = points[7].y = 7 * slope;
            points[8].y = points[9].y = 7 * slope + 34;
            int walls = cases[i].walls;
            Bool blocked = walls != 0;
            sets[1].nbMesh = 2 * walls;
            int first = blocked && (layout & 2);
            collision_mesh_init();
            world_count = blocked ? 2 : 1;
            world_objects[first] =
                (struct world_object){.flags = WORLD_SOLID, .object = &land};
            if (blocked) {
                world_objects[!first] = (struct world_object){
                    .flags = WORLD_SOLID, .object = &wall};
            }
            if (!collision_mesh_update()) {
                return 1;
            }
            Float vx = cases[i].forward, vz = cases[i].side;
            NJS_POINT3 start = {cases[i].start * sign, 0, 0.25F * sign};
            begin_stride(sonic_terrain_modes.walk, start,
                         njSqrt(vx * vx + vz * vz));
            struct task_work *work = player_work[0];
            struct task_motion *motion = player_motion[0];
            struct player *record = player_records[0];
            struct collision_work collision = {0};
            work->collision = &collision;
            /* The glancing runner faces (4.75,0,8) as he reaches the bank. */
            work->ang.y = motion->aim.y = land.ang[1] + (vz ? 0x2a2b : 0);
            motion->speed = (NJS_VECTOR){vx * sign, 0, vz * sign};
            record->ground_object = &land;
            int result = player_collide(work, motion, record);
            /* The sphere leaves the flat at X=-r*tan(theta/2). Its upward
             * speed stays vx*sin(theta) for the rest of the tick. */
            Float cosine = 1.0F / njSqrt(1.0F + slope * slope);
            Float sine = slope * cosine, lead = 5 * sine / (1 + cosine);
            Float remaining = vx + cases[i].start + lead;
            NJS_POINT3 center = {sign *
                                     (blocked ? 2 : -lead + remaining * cosine),
                                 5 + remaining * sine, sign * (0.25F + vz)};
            NJS_VECTOR velocity = {blocked ? 0 : sign * vx * cosine, vx * sine,
                                   sign * vz};
            if (walls == 2) {
                /* X detaches the runner before the sphere reaches Z=13-r. */
                center.z = 8 * sign;
                velocity.z = 0;
            }
            NJS_VECTOR normal = {blocked ? -sign : 0, 0, 0};
            if (result != cases[i].result ||
                (result == PLAYER_CRASHED &&
                 !corner_near(0, record->speed, (NJS_VECTOR){0})) ||
                !corner_near(0.003F, record->contact->pos, center) ||
                !corner_near(0.003F, motion->speed, velocity) ||
                !corner_near(0.003F, record->wall_normal, normal) ||
                !!(work->flags & PLAYER_ON_GROUND) != !blocked ||
                record->ground_object != (blocked ? NULL : &land) ||
                record->ground_task ||
                record->ground_flags != (blocked ? 0 : WORLD_SOLID)) {
                return 2 + i;
            }
            if (blocked) {
                NJS_POINT3 feet = {center.x, center.y - 5, center.z};
                Angle heading = land.ang[1] + (vz ? 0x4000 : 0);
                if (!corner_near(0.003F, work->pos, feet) ||
                    angle_distance(work->ang.y, heading) > 2 ||
                    angle_distance(motion->aim.y, heading) > 2) {
                    return 7;
                }
                if (vz) {
                    /* The next airborne query starts from those same feet. */
                    player_take_speed(work, motion, record);
                    work->mode = sonic_terrain_modes.fall;
                    feet.y += velocity.y;
                    feet.z += velocity.z;
                    if (player_collide(work, motion, record) ||
                        !corner_near(0.003F, work->pos, feet) ||
                        !corner_near(0.003F, motion->speed, velocity) ||
                        (work->flags & PLAYER_ON_GROUND) ||
                        record->ground_object) {
                        return 8;
                    }
                }
            }
        }
    }
    return 0;
}

enum curb_approach {
    CURB_WALK,
    CURB_DESCEND,
    CURB_GLANCE,
    CURB_STAND,
    CURB_BACK_FACE,
    CURB_OVERHANG
};

/* The lower tread ends at x=0. Its two-unit riser and upper tread share
 * an object; the ceiling can share that object or be visited first. */
static Bool build_curb(int layout, Float roof, enum curb_approach approach)
{
    const NJS_POINT3 top[] = {
        {0, 2, -32}, {0, 2, 32}, {64, 2, 32}, {64, 2, -32}};
    const NJS_POINT3 lip[] = {{0, 0, 32}, {0, 2, 32}, {0, 2, -32}, {0, 0, -32}};
    Float left = approach == CURB_OVERHANG ? -4 : -64;
    Float side = approach == CURB_BACK_FACE ? -32 : 32;
    const NJS_POINT3 ceiling[] = {{left, roof, side},
                                  {left, roof, -side},
                                  {64, roof, -side},
                                  {64, roof, side}};
    collision_mesh_init();
    int count = layout ? 3 : 1;
    for (int i = 0; i < count; i++) {
        world_objects[i] = (struct world_object){.flags = WORLD_SOLID};
    }
    if (!update(count, 0)) {
        return FALSE;
    }
    for (struct collision_mesh *mesh = collision_mesh_list; mesh;
         mesh = mesh->next) {
        collision_mesh_clear_buckets();
        if (!layout || mesh->object == objects) {
            ground_tile(-64, 0, -32, 32, 0, 0, 0);
        }
        if (!layout || mesh->object == objects + 1) {
            ground_quad(top, 1, 0, 0);
            ground_quad(lip, 0, 0, 0);
        }
        if (roof && (!layout || mesh->object == objects + 2)) {
            ground_quad(ceiling, 0, 0, 0);
        }
        mesh->span = 128;
        collision_mesh_take_buckets(mesh);
    }
    if (layout == 2) {
        world_objects[0].object = objects + 2;
        world_objects[2].object = objects;
    }
    return TRUE;
}

static int curb_clearance(int layout, Float roof, enum curb_approach approach)
{
    Bool descending = approach == CURB_DESCEND;
    Bool blocked = roof == 11 && approach != CURB_BACK_FACE;
    int tangent = approach == CURB_GLANCE;
    if (!build_curb(layout, roof, approach)) {
        return 1;
    }
    Float start = descending ? 8 : -8;
    Float vx = descending ? -2 : 2;
    int mode = approach == CURB_STAND ? sonic_terrain_modes.stand
                                      : sonic_terrain_modes.walk;
    begin_stride(mode, (NJS_POINT3){start, descending ? 2 : 0, 1}, vx);
    struct task_work *work = player_work[0];
    struct task_motion *motion = player_motion[0];
    struct player *record = player_records[0];
    struct collision_work collision = {0};
    work->collision = &collision;
    if (descending) {
        work->ang.y = motion->aim.y = 0x8000;
        record->speed.x = 2;
        record->ground_object = objects + !!layout;
    }
    for (int tick = 0; tick < 10; tick++) {
        if (blocked) {
            motion->speed = (NJS_VECTOR){2, 0, tangent};
            player_take_speed(work, motion, record);
        }
        player_collide(work, motion, record);
        if (blocked) {
            /* At y=0 the sphere touches the top edge four units away:
             * 4*4 + (5-2)*(5-2) = 5*5. */
            if (!(work->flags & PLAYER_ON_LAND) ||
                record->ground_object != objects ||
                njAbs(work->pos.y) > 0.002F || work->pos.x > -3.998F ||
                njAbs(work->pos.z - (1 + (tick + 1) * tangent)) > 0.005F) {
                return 2;
            }
        } else if (njAbs(work->pos.x - (start + (tick + 1) * vx)) > 0.005F ||
                   njAbs(work->pos.z - 1) > 0.005F) {
            return 3;
        }
        player_take_speed(work, motion, record);
        player_apply_accel(work, motion, record);
    }
    if (blocked) {
        Float x = work->pos.x;
        if (x < -6.001F) {
            return 4;
        }
        work->ang.y = motion->aim.y = 0x8000;
        motion->speed = (NJS_VECTOR){-2, 0, 0};
        player_take_speed(work, motion, record);
        player_collide(work, motion, record);
        if (!near_point(work->pos, (NJS_POINT3){x - 2, 0, 1 + 10 * tangent})) {
            return 5;
        }
    } else if (!(work->flags & PLAYER_ON_LAND) ||
               njAbs(work->pos.y - (descending ? 0 : 2)) > 0.002F ||
               record->ground_object != objects + (!descending && layout)) {
        return 6;
    }
    if (approach == CURB_BACK_FACE) {
        work->mode = sonic_terrain_modes.stand;
        motion->speed = record->speed = (NJS_VECTOR){0};
        NJS_POINT3 rest = work->pos;
        for (int tick = 0; tick < 3; tick++) {
            player_collide(work, motion, record);
            if (!near_point(work->pos, rest) ||
                record->ground_object != objects + !!layout) {
                return 7;
            }
        }
    }
    return 0;
}

/* A cap just above the sphere center is outside the body at wall impact.
 * The larger step probe can still reach it, including on a separate mesh. */
static Bool ground_wall_clearance(void)
{
    const NJS_POINT3 cap[] = {
        {-16, 5.25F, -16}, {-16, 5.25F, 16}, {0, 5.25F, 16}, {0, 5.25F, -16}};
    const NJS_POINT3 ceiling[] = {{-16, 11.95F, -16},
                                  {16, 11.95F, -16},
                                  {16, 11.95F, 16},
                                  {-16, 11.95F, 16}};
    struct polygon_ball ball = {.pos = {5.8F, 0, -8},
                                .speed = {-1, 0, 4},
                                .radius = 5,
                                .normal = {0, 1, 0}};
    const NJS_POINT3 end_wall[] = {
        {-16, -16, 0.6F}, {-16, 16, 0.6F}, {16, 16, 0.6F}, {16, -16, 0.6F}};
    enum { CAP, CEILING, LEDGE, CORNER };
    for (int obstacle = CAP; obstacle <= CORNER; obstacle++) {
        for (int reverse = 0; reverse < 2; reverse++) {
            collision_mesh_init();
            for (int i = 0; i < 3; i++) {
                world_objects[i] = (struct world_object){.flags = WORLD_SOLID};
            }
            if (!update(3, 0)) {
                return FALSE;
            }
            for (struct collision_mesh *mesh = collision_mesh_list; mesh;
                 mesh = mesh->next) {
                collision_mesh_clear_buckets();
                if (mesh->object == objects) {
                    ground_tile(-32, 32, -32, obstacle == LEDGE ? -4.5F : 32, 0,
                                0, reverse);
                } else if (mesh->object == objects + 1) {
                    Float top = obstacle == CAP ? 5.25F : 32;
                    NJS_POINT3 wall[] = {{0, -32, -16},
                                         {0, top, -16},
                                         {0, top, 16},
                                         {0, -32, 16}};
                    ground_quad(wall, 0, reverse, 0);
                } else if (obstacle != LEDGE) {
                    const NJS_POINT3 *extra = obstacle == CAP       ? cap
                                              : obstacle == CEILING ? ceiling
                                                                    : end_wall;
                    ground_quad(extra, 0, reverse, 0);
                }
                mesh->span = 64;
                collision_mesh_take_buckets(mesh);
            }
            world_objects[1].object = objects + 1 + reverse;
            world_objects[2].object = objects + 2 - reverse;
            struct ground_step step = {.feet = {1, 2, 3}};
            if (polygon_follow_ground(&ball, objects, 2, 0.47F, 0.08F, 6.9F, 0,
                                      &step) ||
                !near_point(step.feet, (NJS_POINT3){1, 2, 3})) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

static void begin_fall(int mode, NJS_POINT3 start, NJS_VECTOR speed)
{
    begin_stride(mode, start, 0);
    struct task_work *work = player_work[0];
    struct task_motion *motion = player_motion[0];
    struct player *record = player_records[0];
    work->flags = 0;
    motion->speed = record->speed = speed;
    record->ground_object = NULL;
    record->last_flags = record->last_ground_flags = record->ground_flags = 0;
    record->contact->hit.normal_speed = (NJS_VECTOR){17, 19, 23};
}

/* The three walls touch a radius-five sphere at the origin. The passage
 * runs along (2,9,1); a sideways push can add no velocity along it. */
static int tilted_passage(void)
{
    /* Each triangle spans the passage axis and (x,0,z). Round its
     * Z intercept to a grid so subtracting the vertices recovers both
     * edge directions exactly. */
    static const struct {
        int x, z;
        Float intercept;
    } wall[] = {{4, -3, 52449 / 8192.0F},
                {4, 1, 42282 / 8192.0F},
                {-41, -10, -42225 / 8192.0F}};
    static NJS_POINT3 points[9];
    static Sint16 indices[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    static NJS_MESHSET set = {
        .type_matId = NJD_MESHSET_3, .nbMesh = 3, .meshes = indices};
    static NJS_MODEL room = {.points = points,
                             .nbPoint = 9,
                             .meshsets = &set,
                             .nbMeshset = 1,
                             .r = 144};
    static NJS_OBJECT object = {.model = &room, .scl = {1, 1, 1}};
    for (int reverse = 0; reverse <= 1; reverse++) {
        for (int i = 0; i < 3; i++) {
            int face = reverse ? 2 - i : i;
            for (int j = 0; j < 3; j++) {
                int along = j == 1 ? 3 : -1;
                int across = j == 2 ? 3 : -1;
                points[3 * i + j] = (NJS_POINT3){
                    2 * along + wall[face].x * across, 9 * along,
                    wall[face].intercept + along + wall[face].z * across};
            }
        }
        collision_mesh_init();
        world_count = 1;
        world_objects[0] =
            (struct world_object){.flags = WORLD_SOLID, .object = &object};
        if (!collision_mesh_update()) {
            return 1;
        }
        for (int moving = 0; moving <= 1; moving++) {
            /* The second speed has an axial component of -27/43 times
             * (2,9,1). The first is perpendicular to the passage. */
            Float t = moving ? -27.0F / 43 : 0;
            NJS_VECTOR axial = {2 * t, 9 * t, t};
            NJS_VECTOR speed =
                moving ? (NJS_VECTOR){-4, -4, -10} : (NJS_VECTOR){-4, 0, 8};
            begin_fall(sonic_terrain_modes.fall, (NJS_POINT3){0, -5, 0}, speed);
            struct task_work *work = player_work[0];
            struct task_motion *motion = player_motion[0];
            struct player *record = player_records[0];
            struct collision_work collision = {0};
            work->collision = &collision;
            NJS_POINT3 feet = {axial.x, axial.y - 5, axial.z};
            if (player_collide(work, motion, record) ||
                (work->flags & PLAYER_ON_GROUND) || record->ground_object ||
                !corner_near(0.002F, work->pos, feet) ||
                !corner_near(0.002F, motion->speed, axial)) {
                return 2 + moving;
            }
        }
    }
    return 0;
}

static Bool land_from(int mode, NJS_POINT3 start, NJS_VECTOR speed,
                      NJS_POINT3 end, NJS_VECTOR outgoing, NJS_OBJECT *owner)
{
    begin_fall(mode, start, speed);
    struct task_work *work = player_work[0];
    struct task_motion *motion = player_motion[0];
    struct player *record = player_records[0];
    int hit = move_in_world(work, motion, record);
    NJS_VECTOR tangent = outgoing;
    if (owner) {
        tangent.y = 0;
    }
    if (hit || record->ground_task || record->ground_object != owner ||
        !!(work->flags & PLAYER_ON_LAND) != !!owner ||
        record->ground_flags != (owner ? WORLD_SOLID : 0) ||
        !near_point(work->pos, end) || !near_point(motion->speed, tangent)) {
        return FALSE;
    }
    /* The landing animation reads vertical impact speed. Movement on the
     * following tick reads local speed, which has no inward component. */
    player_take_speed(work, motion, record);
    if (!near_point(motion->speed, tangent) ||
        record->landing_speed_y != (owner ? outgoing.y : 0.0F)) {
        return FALSE;
    }
    if (owner) {
        NJS_POINT3 center = end;
        center.y += record->contact->radius;
        if (!near_point(record->contact->pos, center) ||
            !near_point(record->contact->speed, (NJS_VECTOR){0}) ||
            !near_point(record->contact->hit.normal_speed,
                        (NJS_VECTOR){0, outgoing.y, 0})) {
            return FALSE;
        }
        outgoing.y = 0;
    }
    return near_point(record->speed, outgoing);
}

/* Enter through the same collision and speed updates as each character's
 * movement task. The contact offset depends on the character's height. */
static int roster_terrain(void)
{
    static const struct player_terrain_modes *const modes[CHARACTER_COUNT] = {
        [CHARACTER_SONIC] = &sonic_terrain_modes,
        [CHARACTER_TAILS] = &tails_terrain_modes,
        [CHARACTER_KNUCKLES] = &knuckles_terrain_modes,
        [CHARACTER_AMY] = &amy_terrain_modes,
        [CHARACTER_E102] = &e102_terrain_modes,
        [CHARACTER_BIG] = &big_terrain_modes,
    };
    struct collision_work collision = {0};
    const int dry_skate[] = {E102_SKATE, E102_SKATE_STOP, E102_SKATE_JUMP};
    for (unsigned i = 0; i < sizeof dry_skate / sizeof *dry_skate; i++) {
        if (player_terrain_motion(CHARACTER_AMY, dry_skate[i]) !=
            PLAYER_TERRAIN_SPECIAL) {
            return 45;
        }
    }
    for (int id = 0; id < CHARACTER_COUNT; id++) {
        const struct player_terrain_modes *row = modes[id];
        if (player_terrain_motion(id, 0) != PLAYER_TERRAIN_SPECIAL ||
            player_terrain_motion(id, -1) != PLAYER_TERRAIN_SPECIAL ||
            player_terrain_motion(id, 256) != PLAYER_TERRAIN_SPECIAL) {
            return 46;
        }
        if (!row) {
            for (int mode = 0; mode < 256; mode++) {
                if (player_terrain_motion(id, mode) != PLAYER_TERRAIN_SPECIAL) {
                    return 1;
                }
            }
            continue;
        }
        Float radius = character_constants[id].height * 0.5F;
        for (int split = 0; split <= 1; split++) {
            if (!ground_strip(STRIP_RAMP, split, split, split, 1)) {
                return 2;
            }
            int mode = split ? row->stop : row->walk;
            begin_stride(mode, (NJS_POINT3){-4, 0, 1}, 12);
            struct task_work *work = player_work[0];
            struct task_motion *motion = player_motion[0];
            struct player *record = player_records[0];
            work->collision = &collision;
            PLAYER_VARS(work)->character = id;
            record->consts = character_constants[id];
            /* The center reaches the incline at -radius*tan(angle/2). */
            Float cosine = 4.0F / njSqrt(17.0F);
            Float sine = cosine * 0.25F;
            Float offset = radius * sine / (1.0F + cosine);
            Float x = -offset + (8.0F + offset) * cosine + radius * sine;
            if (player_terrain_motion(id, mode) != PLAYER_TERRAIN_STRIDE ||
                player_collide(work, motion, record) ||
                !(work->flags & PLAYER_ON_LAND) ||
                record->ground_object != &objects[split] ||
                !near_point(work->pos, (NJS_POINT3){x, x * 0.25F, 1}) ||
                !near_point(motion->speed,
                            (NJS_VECTOR){12 * cosine, 12 * sine, 0})) {
                return 3 + id;
            }
            player_take_speed(work, motion, record);
            if (!near_point(record->speed, (NJS_VECTOR){12, 0, 0})) {
                return 11 + id;
            }

            mode = split ? row->jump : row->fall;
            if (!landing_corner(split, split, split)) {
                return 19;
            }
            begin_fall(mode, (NJS_POINT3){radius + 6, 8, -4},
                       (NJS_VECTOR){-12, -16, 5});
            work->collision = &collision;
            PLAYER_VARS(work)->character = id;
            record->consts = character_constants[id];
            if (player_terrain_motion(id, mode) != PLAYER_TERRAIN_AIR ||
                player_collide(work, motion, record) ||
                !(work->flags & PLAYER_ON_LAND) ||
                record->ground_object != objects ||
                !near_point(work->pos, (NJS_POINT3){radius, 0, 1}) ||
                !near_point(motion->speed, (NJS_VECTOR){0, 0, 5})) {
                return 20 + id;
            }
            player_take_speed(work, motion, record);
            if (player_landing_speed(motion->speed.y, record) != -16 ||
                !near_point(record->speed, (NJS_VECTOR){0, 0, 5})) {
                return 28 + id;
            }
            player_clear_landing_speed(record);
            if (player_landing_speed(motion->speed.y, record) != 0) {
                return 36 + id;
            }
        }
    }
    return 0;
}

/* A position correction is independent of approach velocity. Run at the
 * character entry point so support and landing impulses are checked too. */
static Bool level_overlap(Bool grounded, NJS_POINT3 start, NJS_VECTOR speed,
                          NJS_POINT3 feet, NJS_VECTOR outgoing, Bool landed)
{
    struct collision_work collision = {0};
    if (grounded) {
        begin_stride(sonic_terrain_modes.walk, start, 1);
    } else {
        begin_fall(sonic_terrain_modes.fall, start, speed);
    }
    struct task_work *work = player_work[0];
    struct task_motion *motion = player_motion[0];
    struct player *record = player_records[0];
    work->collision = &collision;
    motion->speed = speed;
    if (player_collide(work, motion, record) || !near_point(work->pos, feet) ||
        !near_point(motion->speed, outgoing) ||
        !!(work->flags & PLAYER_ON_GROUND) != landed ||
        record->ground_object != (landed ? objects : NULL)) {
        return FALSE;
    }
    if (landed) {
        Float fall = grounded ? 0 : speed.y;
        if (record->landing_speed_y != fall ||
            !near_point(record->contact->hit.normal_speed,
                        (NJS_VECTOR){0, fall, 0})) {
            return FALSE;
        }
    }
    return TRUE;
}

static Bool overlap_corner(int layout, Bool oblique)
{
    int split = layout & 1, diagonal = (layout >> 1) & 1, reverse = layout >> 2;
    int count = split ? 3 : 1;
    collision_mesh_init();
    for (int i = 0; i < count; i++) {
        world_objects[i] = (struct world_object){.flags = WORLD_SOLID};
    }
    if (!update(count, 0)) {
        return FALSE;
    }
    for (struct collision_mesh *mesh = collision_mesh_list; mesh;
         mesh = mesh->next) {
        collision_mesh_clear_buckets();
        for (int i = 0; i < 3; i++) {
            int face = reverse ? 2 - i : i;
            if (split && mesh->object != objects + face) {
                continue;
            }
            if (face == 0) {
                ground_tile(-32, 32, -32, 32, 0, diagonal, reverse);
            } else if (face == 1 || oblique) {
                for (int j = 0; j < 2; j++) {
                    int a = diagonal + (j ^ reverse) + 1;
                    air_wall_triangle(face - 1, diagonal, a % 4, (a + 1) % 4);
                }
            } else {
                const NJS_POINT3 end[] = {
                    {-32, -32, 0}, {32, -32, 0}, {32, 32, 0}, {-32, 32, 0}};
                ground_quad(end, diagonal, reverse, 0);
            }
        }
        mesh->span = 64;
        collision_mesh_take_buckets(mesh);
    }
    if (split && reverse) {
        world_objects[0].object = objects + 2;
        world_objects[2].object = objects;
    }
    if (oblique) {
        /* Clearing the X wall opens a 0.6-unit gap to the other wall.
         * The remaining half unit along Z never closes that gap. */
        return level_overlap(FALSE, (NJS_POINT3){4, 0, -8.0F / 3.0F},
                             (NJS_VECTOR){-1, -3, 0.5F},
                             (NJS_POINT3){5, 0, -8.0F / 3.0F + 0.5F},
                             (NJS_VECTOR){0, 0, 0.5F}, TRUE);
    }
    return level_overlap(FALSE, (NJS_POINT3){4.75F, 0, 4.5F},
                         (NJS_VECTOR){-1, -3, -2}, (NJS_POINT3){5, 0, 5},
                         (NJS_VECTOR){0}, TRUE);
}

static int initial_overlaps(void)
{
    for (int layout = 0; layout < 8; layout++) {
        if (!overlap_corner(layout, FALSE) || !overlap_corner(layout, TRUE)) {
            return 6;
        }
        if (!landing_corner(layout & 1, (layout >> 1) & 1, layout >> 2)) {
            return 1;
        }
        for (int dx = -1; dx <= 1; dx++) {
            Float vx = dx * 2;
            Float out = dx > 0 ? vx : 0;
            for (int h = -1; h <= 1; h++) {
                if (!level_overlap(FALSE, (NJS_POINT3){4, h * 0.5F, -5},
                                   (NJS_VECTOR){vx, -2, 1},
                                   (NJS_POINT3){5 + out, 0, -4},
                                   (NJS_VECTOR){out, 0, 1}, TRUE)) {
                    return 2;
                }
            }
            if (!level_overlap(FALSE, (NJS_POINT3){4, 1, -5},
                               (NJS_VECTOR){vx, -2, 1},
                               (NJS_POINT3){5 + out, 0, -4},
                               (NJS_VECTOR){out, 0, 1}, TRUE) ||
                !level_overlap(FALSE, (NJS_POINT3){4, -0.5F, -5},
                               (NJS_VECTOR){vx, 2, 1},
                               (NJS_POINT3){5 + out, 2, -4},
                               (NJS_VECTOR){out, 2, 1}, FALSE)) {
                return 3;
            }
            if (!level_overlap(TRUE, (NJS_POINT3){4.75F, 0, -8},
                               (NJS_VECTOR){vx, 0, 4},
                               (NJS_POINT3){5 + out, 0, -4},
                               (NJS_VECTOR){out, 0, 4}, TRUE)) {
                return 4;
            }
        }
        if (!level_overlap(TRUE, (NJS_POINT3){2.5F, 0, -8}, (NJS_VECTOR){0},
                           (NJS_POINT3){5, 0, -8}, (NJS_VECTOR){0}, TRUE)) {
            return 5;
        }
    }
    return 0;
}

/* Both halves lie on y = sx*x + sz*z. Splitting them across objects or
 * changing their diagonals must preserve the landing and its outgoing speed. */
static Bool landing_bank(Float sx, Float sz, Bool wall, int layout)
{
    int split = layout & 1, reverse = (layout >> 1) & 1,
        diagonal = (layout >> 2) & 1;
    int count = 1 + split + wall;
    collision_mesh_init();
    for (int i = 0; i < count; i++) {
        world_objects[i] = (struct world_object){.flags = WORLD_SOLID};
    }
    if (!update(count, 0)) {
        return FALSE;
    }
    for (struct collision_mesh *mesh = collision_mesh_list; mesh;
         mesh = mesh->next) {
        collision_mesh_clear_buckets();
        if (wall && mesh->object == objects + count - 1) {
            for (int i = 0; i < 2; i++) {
                int j = diagonal + (i ^ reverse) + 1;
                air_wall_triangle(0, diagonal, j % 4, (j + 1) % 4);
            }
        } else {
            for (int i = 0; i < 2; i++) {
                int tile = i ^ reverse;
                if (split && mesh->object != objects + tile) {
                    continue;
                }
                Float x = tile ? 0 : -64;
                NJS_POINT3 v[] = {
                    {x, 0, -64}, {x, 0, 64}, {x + 64, 0, 64}, {x + 64, 0, -64}};
                for (int j = 0; j < 4; j++) {
                    v[j].y = sx * v[j].x + sz * v[j].z;
                }
                Uint32 attr = 0;
                if (layout & 8) {
                    NJS_POINT3 corner = v[1];
                    v[1] = v[3];
                    v[3] = corner;
                    attr = NJD_FLAG_DOUBLE_SIDE;
                }
                ground_quad(v, diagonal, reverse, attr);
            }
        }
        mesh->span = 128;
        collision_mesh_take_buckets(mesh);
    }
    if (reverse) {
        for (int i = 0; i < count; i++) {
            world_objects[i].object = objects + count - 1 - i;
        }
    }
    return TRUE;
}

/* The original wall push would enter a bank that started 0.2 units clear.
 * At x = 5 the center must rise another 0.5 to clear both planes. */
static Bool overlap_bank(int layout)
{
    struct collision_work collision = {0};
    if (!landing_bank(0.75F, 0, TRUE, layout)) {
        return FALSE;
    }
    for (struct collision_mesh *mesh = collision_mesh_list; mesh;
         mesh = mesh->next) {
        for (struct polygon *poly = mesh->polygons; poly != mesh->last->next;
             poly = poly->next) {
            if (poly->vertex[0].x || poly->vertex[1].x || poly->vertex[2].x) {
                for (int i = 0; i < 3; i++) {
                    poly->vertex[i].y -= 3.5F;
                }
                poly->center.y -= 3.5F;
            }
        }
    }
    begin_fall(sonic_terrain_modes.fall, (NJS_POINT3){4, 1, -5},
               (NJS_VECTOR){-2, -2, 1});
    struct task_work *w = player_work[0];
    struct task_motion *m = player_motion[0];
    struct player *r = player_records[0];
    w->collision = &collision;
    return !player_collide(w, m, r) && (w->flags & PLAYER_ON_LAND) &&
           near_point(w->pos, (NJS_POINT3){8, 2.5F, -4}) &&
           near_point(m->speed, (NJS_VECTOR){0, 0, 1}) &&
           r->landing_speed_y == -2;
}

/* Gamma's skate presses him into the ground. Braking and jumping use the
 * same acceleration routines as his walking stop and his ordinary jump. */
static int gamma_skates(void)
{
    struct collision_work collision = {0};
    const struct character_constants *constants =
        &character_constants[CHARACTER_E102];
    for (int braking = 0; braking <= 1; braking++) {
        if (!ground_strip(STRIP_RAMP, 1, 0, 0, 1)) {
            return 1;
        }
        begin_stride(braking ? E102_SKATE_STOP : E102_SKATE,
                     (NJS_POINT3){-2, 0, 1}, constants->sprint_speed);
        struct task_work *work = player_work[0];
        struct task_motion *motion = player_motion[0];
        struct player *record = player_records[0];
        PLAYER_VARS(work)->character = CHARACTER_E102;
        work->collision = &collision;
        record->consts = *constants;
        if (braking) {
            player_accel_brake(work, motion, record);
        } else {
            e102_skate(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        Float forward = motion->speed.x;
        Float tangent = forward * (4.0F / njSqrt(17.0F));
        if (player_collide(work, motion, record) ||
            !(work->flags & PLAYER_ON_LAND) ||
            record->ground_object != &objects[1] || work->pos.x <= 0 ||
            njAbs(work->pos.y - work->pos.x * 0.25F) > 0.001F ||
            !near_point(motion->speed,
                        (NJS_VECTOR){tangent, tangent * 0.25F, 0})) {
            return 2 + braking;
        }
        player_take_speed(work, motion, record);
        if (!near_point(record->speed, (NJS_VECTOR){forward, 0, 0})) {
            return 4 + braking;
        }
    }

    if (!landing_corner(1, 1, 1)) {
        return 6;
    }
    Float radius = constants->height * 0.5F;
    begin_fall(E102_SKATE_JUMP, (NJS_POINT3){radius + 1, 1, -3},
               (NJS_VECTOR){-2, -2, 0.8F});
    struct task_work *work = player_work[0];
    struct task_motion *motion = player_motion[0];
    struct player *record = player_records[0];
    PLAYER_VARS(work)->character = CHARACTER_E102;
    work->collision = &collision;
    record->consts = *constants;
    player_accel_air(work, motion, record);
    player_apply_accel(work, motion, record);
    NJS_VECTOR incoming = motion->speed;
    if (player_collide(work, motion, record) ||
        !(work->flags & PLAYER_ON_LAND) || record->ground_object != objects ||
        !near_point(work->pos, (NJS_POINT3){radius, 0, -3 + incoming.z}) ||
        !near_point(motion->speed, (NJS_VECTOR){0, 0, incoming.z})) {
        return 7;
    }
    player_take_speed(work, motion, record);
    if (!near_point(record->speed, (NJS_VECTOR){0, 0, incoming.z}) ||
        player_landing_speed(motion->speed.y, record) != incoming.y) {
        return 8;
    }
    return 0;
}

static int gamma_landings(void)
{
    struct collision_work collision = {0};
    struct e102 gamma = {0};
    /* A bank turns the descent uphill. The booster slows world Y while
     * Gamma retains local Y for his landing animation. */
    static const struct {
        int mode;
        Float local_y, incoming_y;
        Bool hover;
    } cases[] = {{E102_JUMP, -12, -12, FALSE},
                 {E102_FALL, -2, -2, FALSE},
                 {E102_FALL, -3, -0.4F, TRUE}};
    if (!landing_bank(0.75F, 0, FALSE, 0)) {
        return 1;
    }
    for (int i = 0; i < (int)(sizeof cases / sizeof *cases); i++) {
        begin_fall(cases[i].mode, (NJS_POINT3){-7, 2, -2},
                   (NJS_VECTOR){15, cases[i].local_y, 3});
        struct task_work *work = player_work[0];
        struct task_motion *motion = player_motion[0];
        struct player *record = player_records[0];
        PLAYER_VARS(work)->character = CHARACTER_E102;
        work->collision = &collision;
        record->consts = character_constants[CHARACTER_E102];
        record->own.e102.record[0] = &gamma;
        if (cases[i].hover) {
            record->upgrades = PLAYER_E102_BOOSTER;
            pad_records[0].on = pad_jump_buttons;
            Bool hovering = e102_hover(work, motion, record);
            pad_records[0].on = 0;
            if (!hovering) {
                return 2;
            }
        }
        Float incoming_y = motion->speed.y;
        if (record->speed.y != cases[i].local_y ||
            njAbs(incoming_y - cases[i].incoming_y) > 0.000001F ||
            player_collide(work, motion, record) ||
            record->ground_object != objects ||
            !(work->flags & PLAYER_ON_LAND)) {
            return 3 + i;
        }
        player_take_speed(work, motion, record);
        if (motion->speed.y <= 0 || record->speed.y != 0 ||
            player_landing_speed(motion->speed.y, record) != incoming_y) {
            return 6 + i;
        }
        player_clear_landing_speed(record);
        if (player_landing_speed(motion->speed.y, record) != motion->speed.y) {
            return 9;
        }
    }
    /* Gamma can stand on water. His existing water handler needs its flag. */
    if (!landing_bank(0, 0, FALSE, 0)) {
        return 10;
    }
    world_objects[0].flags = WORLD_WATER;
    begin_fall(E102_FALL, (NJS_POINT3){-5, 8, 3}, (NJS_VECTOR){0, -16, 0});
    struct task_work *work = player_work[0];
    struct task_motion *motion = player_motion[0];
    struct player *record = player_records[0];
    work->collision = &collision;
    PLAYER_VARS(work)->character = CHARACTER_E102;
    record->consts = character_constants[CHARACTER_E102];
    if (player_collide(work, motion, record) ||
        !(work->flags & PLAYER_ON_LAND) ||
        record->ground_flags != WORLD_WATER ||
        record->ground_object != objects || record->landing_speed_y != 0) {
        return 11;
    }
    return 0;
}

static int bank_landings(void)
{
    static const struct {
        Float sx, sz;
        Bool wall, supported;
        NJS_POINT3 feet;
        NJS_VECTOR speed;
        NJS_POINT3 end;
        NJS_VECTOR tangent;
    } cases[] = {
        {0.75F,
         0,
         FALSE,
         TRUE,
         {-7, 2, -2},
         {15, -12, 3},
         {2.72F, 2.04F, 1},
         {3.84F, 2.88F, 3}},
        {0.75F,
         0.25F,
         FALSE,
         TRUE,
         {-7, 2, -2},
         {15, -12, 3},
         {73.0F / 26, 50.0F / 26, -19.0F / 26},
         {51.0F / 13, 36.0F / 13, -9.0F / 13}},
        /* Both planes touch at half a tick. Only the line along Z is free. */
        {0.75F,
         0,
         TRUE,
         TRUE,
         {13, 15, -1.5F},
         {-16, -20, 3},
         {8, 6, 1.5F},
         {0, 0, 3}},
        /* The downhill face redirects Sonic upward. Removing his inward X
         * speed at the wall then separates him from the bank. */
        {-0.75F,
         0,
         TRUE,
         FALSE,
         {12, -2.75F, -1},
         {-16, -8, 4},
         {5, -1.15F, 3},
         {0, 4.8F, 4}},
        /* Moving the start by 1/32768 changes whether the wall is touching
         * at floor impact. Only the separated impacts release Sonic. */
        {-0.75F,
         0,
         TRUE,
         FALSE,
         {12, 1 - 0x1p-13F, -1},
         {-16, -8, 4},
         {5, 0.199956F, 3},
         {0, 4.8F, 4}},
        {-0.75F,
         0,
         TRUE,
         TRUE,
         {12, 1 - 3 * 0x1p-15F, -1},
         {-16, -8, 4},
         {2.000073F, -1.500055F, 3},
         {0, 0, 4}}};
    for (int i = 0; i < (int)(sizeof cases / sizeof *cases); i++) {
        for (int layout = 0; layout < 16; layout++) {
            if (!landing_bank(cases[i].sx, cases[i].sz, cases[i].wall,
                              layout)) {
                return 1;
            }
            for (int mode = 0; mode < 256; mode++) {
                if (player_terrain_motion(CHARACTER_SONIC, mode) !=
                    PLAYER_TERRAIN_AIR) {
                    continue;
                }
                begin_fall(mode, cases[i].feet, cases[i].speed);
                struct player *r = player_records[0];
                struct task_work *w = player_work[0];
                struct task_motion *m = player_motion[0];
                w->ang.y = layout & 2 ? 0x2800 : 0;
                NJS_OBJECT *owner =
                    cases[i].supported ? objects + (layout & 1) : NULL;
                if (move_in_world(w, m, r) || r->ground_object != owner ||
                    !!(w->flags & PLAYER_ON_LAND) != !!owner ||
                    !near_point(w->pos, cases[i].end) ||
                    !near_point(m->speed, cases[i].tangent)) {
                    return 2 + i;
                }
                if (owner) {
                    NJS_VECTOR up = {-cases[i].sx, 1, -cases[i].sz};
                    njUnitVector(&up);
                    NJS_VECTOR incoming = cases[i].speed;
                    Float inward = njInnerProduct(&up, &incoming);
                    NJS_VECTOR impulse = {up.x * inward, up.y * inward,
                                          up.z * inward};
                    NJS_POINT3 center = {w->pos.x + 5 * up.x,
                                         w->pos.y + 5 * up.y,
                                         w->pos.z + 5 * up.z};
                    if (!near_point(r->ground_normal, up) ||
                        !near_point(r->contact->pos, center) ||
                        !near_point(r->contact->hit.normal_speed, impulse) ||
                        r->landing_speed_y != cases[i].speed.y) {
                        return 8 + i;
                    }
                }
                player_take_speed(w, m, r);
                NJS_VECTOR back = r->speed;
                player_turn_to_world(w, &back);
                /* The support tilt is quantized to 16-bit Ninja angles. */
                if (njAbs(back.x - m->speed.x) > 0.001F ||
                    njAbs(back.y - m->speed.y) > 0.001F ||
                    njAbs(back.z - m->speed.z) > 0.001F) {
                    return 14 + i;
                }
                if (i == 0 && !w->ang.y) {
                    /* Switch from falling to walking for the next tick. */
                    w->mode = 2;
                    player_apply_accel(w, m, r);
                    r->last_flags = w->flags;
                    r->last_ground_flags = r->ground_flags;
                    r->ground_flags = 0;
                    NJS_POINT3 next = {6.56F, 4.92F, 4};
                    if (move_in_world(w, m, r) || r->ground_object != owner ||
                        njDistanceP2P(&w->pos, &next) > 0.001F) {
                        return 20;
                    }
                }
            }
        }
    }
    return 0;
}

static int airborne_landings(void)
{
    for (int split = 0; split < 2; split++) {
        for (int diagonal = 0; diagonal < 2; diagonal++) {
            for (int reverse = 0; reverse < 2; reverse++) {
                if (!landing_corner(split, diagonal, reverse)) {
                    return 1;
                }
                /* The wall is reached at half a tick. Land before it, at
                 * the same instant, after it, or exactly at tick end. */
                for (int h = 4; h <= 16; h += 4) {
                    for (int mode = 0; mode < 256; mode++) {
                        if (player_terrain_motion(CHARACTER_SONIC, mode) ==
                                PLAYER_TERRAIN_AIR &&
                            !land_from(mode, (NJS_POINT3){11, h, -4},
                                       (NJS_VECTOR){-12, -16, 5},
                                       (NJS_POINT3){5, 0, 1},
                                       (NJS_VECTOR){0, -16, 5}, objects)) {
                            return 2;
                        }
                    }
                }
            }
        }
    }
    int mode = 0;
    while (mode < 256 &&
           player_terrain_motion(CHARACTER_SONIC, mode) != PLAYER_TERRAIN_AIR) {
        mode++;
    }
    static const enum strip_shape strips[] = {STRIP_FLAT, STRIP_LEDGE,
                                              STRIP_GAP};
    for (int reverse = 0; reverse < 2; reverse++) {
        for (int i = 0; i < (int)(sizeof strips / sizeof *strips); i++) {
            enum strip_shape shape = strips[i];
            if (!ground_strip(shape, 1, reverse, reverse, 4) ||
                !land_from(mode, (NJS_POINT3){-10, 4, 1},
                           (NJS_VECTOR){16, -16, 1}, (NJS_POINT3){6, 0, 2},
                           (NJS_VECTOR){16, shape == STRIP_FLAT ? -16 : 0, 1},
                           shape == STRIP_FLAT ? objects + 1 : NULL)) {
                return 3;
            }
        }
    }
    /* Both stories are within one fall. The upper face owns the landing
     * even when the lower story is registered first. */
    if (!ground_strip(STRIP_FLAT, 1, 0, 0, 1)) {
        return 10;
    }
    for (struct collision_mesh *mesh = collision_mesh_list; mesh;
         mesh = mesh->next) {
        collision_mesh_clear_buckets();
        Float y = mesh->object == objects ? 0 : -5;
        struct polygon *tri = polygon_alloc(0);
        tri->vertex[0] = (NJS_POINT3){-32, y, -32};
        tri->vertex[1] = (NJS_POINT3){0, y, 32};
        tri->vertex[2] = (NJS_POINT3){32, y, -32};
        tri->center = (NJS_POINT3){0, y, -10.67F};
        tri->radius = 4096;
        tri->attr = 0;
        collision_mesh_take_buckets(mesh);
    }
    for (int first = 0; first < 2; first++) {
        world_objects[0].object = objects + first;
        world_objects[1].object = objects + (1 - first);
        if (!land_from(mode, (NJS_POINT3){0, 12, 1}, (NJS_VECTOR){3, -20, 2},
                       (NJS_POINT3){3, 0, 3}, (NJS_VECTOR){3, -20, 2},
                       objects)) {
            return 11;
        }
    }
    if (!ground_strip(STRIP_FLAT, 0, 0, 0, 1)) {
        return 4;
    }
    /* A short fall can finish inside the original foot probe's reach. */
    struct polygon_ball body = {
        .pos = {4, 9, 1}, .speed = {0, -8, 0}, .radius = 5};
    struct polygon_ball region = body;
    region.radius = 7;
    struct polygon_slide slide = {.fall_speed = 42};
    polygon_collect_near(&region);
    if (polygon_slide_air(&body, &slide, &region, 7, 0.47F, FALSE) ||
        slide.fall_speed != 42) {
        return 6;
    }
    body.speed.y = -3;
    if (polygon_slide_air(&body, &slide, &region, 7, 0.47F, TRUE) ||
        slide.fall_speed != 42) {
        return 5;
    }
    body.speed.y = -8;
    static const Uint32 authored[] = {
        WORLD_DEFLECT, WORLD_MOVES,    WORLD_HURTS,      WORLD_NO_SLOPE,
        WORLD_CLING,   WORLD_GRIP_LOW, WORLD_GRIP_LEAST, WORLD_SAND};
    for (int i = 0; i < (int)(sizeof authored / sizeof *authored); i++) {
        world_objects[0].flags = WORLD_SOLID | authored[i];
        if (polygon_slide_air(&body, &slide, &region, 7, 0.47F, TRUE) ||
            slide.fall_speed != 42) {
            return 7;
        }
    }
    return 0;
}

/* The floor ends at x = 32 and z = 32. A sphere can strike its edge or
 * corner while the feet remain entirely outside its footprint. */
static int open_floor_rims(void)
{
    static const struct {
        NJS_POINT3 start, end;
        NJS_VECTOR speed, outgoing;
    } cases[] = {
        /* At t = 11/24 the edge normal is (3, 4, 0) / 5. */
        {{35, 10, -8}, {41.24F, -5.68F, -2}, {0, -24, 6}, {11.52F, -8.64F, 6}},
        /* The oblique approach reaches the same edge at t = 1/2. */
        {{38, 5, -2}, {35.96F, -1.72F, 2}, {-6, -12, 4}, {1.92F, -1.44F, 4}},
        /* The corner normal at t = 1/2 is (2.4, 3, 3.2) / 5. */
        {{34.4F, 10, 35.2F},
         {37.856F, -9.68F, 39.808F},
         {0, -24, 0},
         {6.912F, -15.36F, 9.216F}},
        {{38, 10, 0}, {38, -14, 0}, {0, -24, 0}, {0, -24, 0}},
        /* Travel tangent to the edge or away from it at t = 0. */
        {{35, -1, 0}, {39, -4, 4}, {4, -3, 4}, {4, -3, 4}},
        {{35, -1, 0}, {40, -2, 0}, {5, -1, 0}, {5, -1, 0}},
        /* A one-sided floor permits passage from below. */
        {{35, -15, 0}, {35, 9, 0}, {0, 24, 0}, {0, 24, 0}},
    };
    struct collision_work collision = {0};
    for (int layout = 0; layout < 8; layout++) {
        if (!ground_strip(STRIP_FLAT, layout & 1, (layout >> 1) & 1,
                          layout >> 2, 1)) {
            return 1;
        }
        int mode =
            layout & 1 ? sonic_terrain_modes.jump : sonic_terrain_modes.fall;
        /* The join at x = 0 is an ordinary landing on either triangulation. */
        begin_fall(mode, (NJS_POINT3){0, 10, 0}, (NJS_VECTOR){0, -24, 0});
        struct task_work *work = player_work[0];
        struct task_motion *motion = player_motion[0];
        struct player *record = player_records[0];
        work->collision = &collision;
        if (player_collide(work, motion, record) ||
            !(work->flags & PLAYER_ON_LAND) || !record->ground_object ||
            !near_point(work->pos, (NJS_POINT3){0, 0, 0}) ||
            !near_point(motion->speed, (NJS_VECTOR){0})) {
            return 2;
        }
        /* A foot crossing the lip can land on the terrace one unit below
         * or step up from one unit beneath it. */
        for (int height = -1; height <= 1; height++) {
            begin_fall(mode, (NJS_POINT3){35, (Float)height, 0},
                       (NJS_VECTOR){-6, height > 0 ? -1 : 0, 2});
            work->collision = &collision;
            if (player_collide(work, motion, record) ||
                !(work->flags & PLAYER_ON_LAND) || !record->ground_object ||
                !near_point(work->pos, (NJS_POINT3){29, 0, 2}) ||
                motion->speed.x != -6 || motion->speed.z != 2) {
                return 20 + height;
            }
        }
        /* His center crosses above the terrace before his feet reach it.
         * The following tick completes the step onto its upper surface. */
        begin_fall(mode, (NJS_POINT3){35, -4, 1}, (NJS_VECTOR){-6, 2, 2});
        work->collision = &collision;
        if (player_collide(work, motion, record) ||
            (work->flags & PLAYER_ON_GROUND) ||
            !near_point(work->pos, (NJS_POINT3){29, -2, 3})) {
            return 23;
        }
        player_take_speed(work, motion, record);
        if (player_collide(work, motion, record) ||
            !(work->flags & PLAYER_ON_LAND) ||
            !near_point(work->pos, (NJS_POINT3){23, 0, 5}) ||
            !near_point(motion->speed, (NJS_VECTOR){-6, 2, 2})) {
            return 24;
        }
        for (int i = 0; i < (int)(sizeof cases / sizeof *cases); i++) {
            begin_fall(mode, cases[i].start, cases[i].speed);
            work->collision = &collision;
            NJS_POINT3 end = cases[i].end;
            for (int tick = 0; tick < 2; tick++) {
                if (player_collide(work, motion, record) ||
                    (work->flags & PLAYER_ON_GROUND) || record->ground_object ||
                    record->landing_speed_y != 0 ||
                    !near_point(work->pos, end) ||
                    !near_point(motion->speed, cases[i].outgoing)) {
                    return 3 + 2 * i + tick;
                }
                player_take_speed(work, motion, record);
                end.x += cases[i].outgoing.x;
                end.y += cases[i].outgoing.y;
                end.z += cases[i].outgoing.z;
            }
        }
    }
    /* On y = 0.75*z the lip rises along z. The incoming sphere first
     * touches (64, 0, 0) with normal (3, 3.2, -2.4) / 5. */
    for (int layout = 0; layout < 16; layout += 5) {
        if (!landing_bank(0, 0.75F, FALSE, layout)) {
            return 17;
        }
        begin_fall(sonic_terrain_modes.fall, (NJS_POINT3){67, 10.2F, -2.4F},
                   (NJS_VECTOR){0, -24, 0});
        struct task_work *work = player_work[0];
        struct task_motion *motion = player_motion[0];
        struct player *record = player_records[0];
        work->collision = &collision;
        if (player_collide(work, motion, record) ||
            (work->flags & PLAYER_ON_GROUND) || record->ground_object ||
            !near_point(work->pos, (NJS_POINT3){71.608F, -8.8848F, -6.0864F}) ||
            !near_point(motion->speed,
                        (NJS_VECTOR){9.216F, -14.1696F, -7.3728F})) {
            return 18;
        }
    }
    return 0;
}

/* A wall stops Knuckles' travel. He retains the approach speed for his grip
 * unless the exposed floor lip at x=0 also changes his movement. */
static int knuckles_floor_rim(void)
{
    static const struct {
        NJS_VECTOR speed, allowed;
        Bool wall_x, keep_approach;
    } cases[] = {
        {{0, -24, -16}, {11.52F, -8.64F, 0}, FALSE, FALSE},
        /* The +Z wall leaves travel tangent to the lip. */
        {{4, -3, -16}, {4, -3, 0}, FALSE, TRUE},
        /* The -X wall turns the same approach into the lip. */
        {{4, -3, -16}, {0, 0, -16}, TRUE, FALSE},
    };
    struct collision_work collision = {0};
    Float radius = character_constants[CHARACTER_KNUCKLES].height * 0.5F;
    for (int i = 0; i < (int)(sizeof cases / sizeof *cases); i++) {
        NJS_POINT3 wall[] = {
            {-32, -32, 0}, {32, -32, 0}, {32, 32, 0}, {-32, 32, 0}};
        if (cases[i].wall_x) {
            for (int j = 0; j < 4; j++) {
                wall[j].z = wall[j].x;
                wall[j].x = 1.6F * radius;
            }
        }
        for (int reverse = 0; reverse <= 1; reverse++) {
            if (!ground_strip(STRIP_LEDGE, 1, reverse, reverse, 1)) {
                return 1;
            }
            struct collision_mesh *mesh = collision_mesh_list;
            while (mesh->object != objects + 1) {
                mesh = mesh->next;
            }
            collision_mesh_clear_buckets();
            ground_quad(wall, reverse, reverse, 0);
            collision_mesh_take_buckets(mesh);
            /* The wall and the lip's (3,4,0)/5 normal meet at t = 1/2. */
            NJS_POINT3 hit = {0.6F * radius, -0.2F * radius, radius};
            NJS_VECTOR speed = cases[i].speed, allowed = cases[i].allowed;
            NJS_POINT3 start = {hit.x - speed.x * 0.5F, hit.y - speed.y * 0.5F,
                                hit.z - speed.z * 0.5F};
            NJS_POINT3 end = {hit.x + allowed.x * 0.5F,
                              hit.y + allowed.y * 0.5F,
                              hit.z + allowed.z * 0.5F};
            begin_fall(knuckles_terrain_modes.jump, start, speed);
            struct task_work *work = player_work[0];
            struct player *record = player_records[0];
            struct task_motion *motion = player_motion[0];
            PLAYER_VARS(work)->character = CHARACTER_KNUCKLES;
            work->collision = &collision;
            record->consts = character_constants[CHARACTER_KNUCKLES];
            if (player_collide(work, motion, record) ||
                (work->flags & PLAYER_ON_GROUND) || record->ground_object ||
                !near_point(work->pos, end) ||
                !near_point(motion->speed,
                            cases[i].keep_approach ? speed : allowed)) {
                return 2 + i;
            }
        }
    }
    return 0;
}

static Bool carried(NJS_POINT3 center, NJS_VECTOR travel, NJS_VECTOR expected)
{
    return polygon_limit_carry(&center, 5, &travel) &&
           near_point(travel, expected);
}

/* A platform carries the rider into every wall of the fan at mid-tick.
 * The remaining displacement can follow the common axis or an open edge. */
static int crowded_carry(void)
{
    const int sizes[] = {15, 17, 35};
    for (int layout = 0; layout < 4; layout++) {
        for (int tilted = 0; tilted < 2; tilted++) {
            for (int sample = 0; sample < 3; sample++) {
                int count = sizes[sample];
                if (!wall_fan(count, layout, tilted)) {
                    return 1;
                }
                rider_table.count = 1 + (layout & 1);
                for (int i = 0; i < rider_table.count; i++) {
                    int index = layout & 2 ? rider_table.count - i - 1 : i;
                    rider_land[i] =
                        (struct land_entry){.flags = WORLD_SOLID,
                                            .object = objects + index,
                                            .radius = 128};
                }
                for (int open = 0; open < 2; open++) {
                    NJS_VECTOR move = {4, 1, 0}, tangent = {0, 1, 0};
                    if (open) {
                        Float t = (count / 2) * (1.0F / 24);
                        Float c = (1 - t * t) / (1 + t * t);
                        Float n = 2 * t / (1 + t * t);
                        move = (NJS_VECTOR){0, 1, 1};
                        tangent = (NJS_VECTOR){-n * c, 1, c * c};
                    }
                    move = fan_tilt(move, tilted);
                    tangent = fan_tilt(tangent, tilted);
                    NJS_POINT3 start = {0, 2.5F, 0};
                    NJS_VECTOR expected = tangent;
                    if (!open) {
                        start =
                            (NJS_POINT3){-move.x * 0.5F, 2.5F - move.y * 0.5F,
                                         -move.z * 0.5F};
                        expected = (NJS_VECTOR){(move.x + tangent.x) * 0.5F,
                                                (move.y + tangent.y) * 0.5F,
                                                (move.z + tangent.z) * 0.5F};
                    }
                    int requests = heap_requests, frees = heap_frees;
                    heap_enabled = TRUE;
                    if (!carried(start, move, expected)) {
                        return 2 + sample * 2 + open;
                    }
                    heap_enabled = FALSE;
                    if (heap_busy || heap_bad_free ||
                        heap_requests - requests != heap_frees - frees ||
                        ((count > POLYGON_CONTACTS) !=
                         (heap_requests > requests))) {
                        return 8;
                    }
                }
            }
        }
    }
    /* With no temporary storage, only the already verified half is safe. */
    NJS_POINT3 start = {-2, 2, 0};
    NJS_VECTOR move = {4, 1, 0};
    int requests = heap_requests;
    if (!wall_fan(17, 0, 0)) {
        return 9;
    }
    rider_table.count = 1;
    rider_land[0] = (struct land_entry){
        .flags = WORLD_SOLID, .object = objects, .radius = 128};
    if (polygon_limit_carry(&start, 5, &move) ||
        !near_point(move, (NJS_VECTOR){2, 0.5F, 0}) ||
        heap_requests == requests || heap_busy || heap_bad_free) {
        return 10;
    }
    rider_table.count = 0;
    return 0;
}

static int carry_surfaces(void)
{
    for (int diagonal = 0; diagonal < 2; diagonal++) {
        for (int reverse = 0; reverse < 2; reverse++) {
            if (!air_wall_scene(1, diagonal, reverse, NULL)) {
                return 1;
            }
            rider_table.count = 1;
            rider_land[0] = (struct land_entry){
                .flags = WORLD_SOLID, .object = objects, .radius = 128};
            /* Start inside the wall's five-unit clearance. Crossing the
             * quad diagonal can slide along the sheet or leave it. */
            if (!carried((NJS_POINT3){3, 0, -9}, (NJS_VECTOR){0, 0, 18},
                         (NJS_VECTOR){0, 0, 18}) ||
                !carried((NJS_POINT3){3, 0, -9}, (NJS_VECTOR){3, 0, 18},
                         (NJS_VECTOR){3, 0, 18}) ||
                !carried((NJS_POINT3){3, 0, 73}, (NJS_VECTOR){0, 0, -16},
                         (NJS_VECTOR){5.28F, 0, -8.96F})) {
                return 2;
            }
            /* Cross the plane outside the mesh, then strike the front of
             * its rim at (3, 0, 68), halfway through the tick. */
            if (!carried((NJS_POINT3){-1, 0, 74}, (NJS_VECTOR){8, 0, -12},
                         (NJS_VECTOR){9.44F, 0, -10.08F})) {
                return 12;
            }
            /* The second wall changes the direction of the remaining slide. */
            rider_table.count = 2;
            rider_land[1] = rider_land[0];
            rider_land[reverse].object = objects + 1;
            if (!carried((NJS_POINT3){7, 2.5F, -4}, (NJS_VECTOR){-6, 1.25F, 4},
                         (NJS_VECTOR){-1.2F, 1.25F, 3.4F})) {
                return 3;
            }
        }
    }
    rider_table.count = 1;
    rider_land[0].object = objects;
    if (!carried((NJS_POINT3){-11, 0, 0}, (NJS_VECTOR){24, 0, 0},
                 (NJS_VECTOR){24, 0, 0})) {
        return 4;
    }
    struct collision_mesh *mesh = collision_mesh_list;
    while (mesh->object != objects) {
        mesh = mesh->next;
    }
    for (struct polygon *tri = mesh->polygons; tri != mesh->last->next;
         tri = tri->next) {
        tri->attr = NJD_FLAG_DOUBLE_SIDE;
    }
    if (!carried((NJS_POINT3){-11, 0, 0}, (NJS_VECTOR){24, 0, 0},
                 (NJS_VECTOR){6, 0, 0})) {
        return 5;
    }
    /* The same clearance applies overhead and lets a descending lift
     * carry the player away again. */
    collision_mesh_init();
    if (!update(1, 0)) {
        return 6;
    }
    static const NJS_POINT3 ceiling[] = {
        {-64, 20, -64}, {64, 20, -64}, {64, 20, 64}, {-64, 20, 64}};
    collision_mesh_clear_buckets();
    ground_quad(ceiling, 0, 0, 0);
    collision_mesh_take_buckets(collision_mesh_list);
    if (!carried((NJS_POINT3){0, 5, 0}, (NJS_VECTOR){6, 24, 4},
                 (NJS_VECTOR){6, 10, 4}) ||
        !carried((NJS_POINT3){6, 15, 4}, (NJS_VECTOR){2, 8, 3},
                 (NJS_VECTOR){2, 0, 3}) ||
        !carried((NJS_POINT3){8, 15, 7}, (NJS_VECTOR){0, -12, 0},
                 (NJS_VECTOR){0, -12, 0})) {
        return 7;
    }
    NJS_POINT3 center = {0};
    NJS_VECTOR delta = {0, 24, 0};
    rider_land[0].object = objects + 1;
    if (polygon_limit_carry(&center, 5, &delta) ||
        !near_point(delta, (NJS_VECTOR){0})) {
        return 8;
    }
    rider_land[0].flags |= WORLD_WATER;
    if (!carried(center, (NJS_VECTOR){0, 24, 0}, (NJS_VECTOR){0, 24, 0})) {
        return 9;
    }
    /* The first 255 entries fit the player's 400-unit collection radius.
     * Only the last entry meets the sphere carried toward the ceiling. */
    rider_table.count = 256;
    for (int i = 0; i < 255; i++) {
        rider_land[i] = (struct land_entry){
            .flags = WORLD_SOLID, .center = {120, 0, 0}, .radius = 5};
    }
    rider_land[255] = (struct land_entry){
        .flags = WORLD_SOLID, .object = objects, .radius = 128};
    if (!carried(center, (NJS_VECTOR){0, 24, 0}, (NJS_VECTOR){0, 15, 0})) {
        return 10;
    }
    for (int i = 0; i < 255; i++) {
        rider_land[i] = rider_land[255];
    }
    if (!carried(center, (NJS_VECTOR){0, 24, 0}, (NJS_VECTOR){0, 15, 0})) {
        return 11;
    }
    /* A dry wall clips a rider independently of its traction, footprint
     * or character response. Liquids and non-solid entries stay passable. */
    if (!air_wall_scene(1, 0, 0, NULL)) {
        return 12;
    }
    rider_table.count = 1;
    rider_land[0] = (struct land_entry){.object = objects, .radius = 128};
    static const Uint32 blocks[] = {
        WORLD_SOLID | WORLD_GRIP_HIGH, WORLD_SOLID | WORLD_FOOTPRINTS,
        WORLD_SOLID | WORLD_SAND, WORLD_SOLID | WORLD_CLING,
        WORLD_SOLID | WORLD_DEFLECT};
    static const Uint32 passes[] = {
        WORLD_FOOTPRINTS, WORLD_WATER, WORLD_SOLID | WORLD_WATER,
        WORLD_SOLID | WORLD_THICK_LIQUID, WORLD_SOLID | WORLD_MOVES};
    for (unsigned i = 0; i < sizeof blocks / sizeof *blocks; i++) {
        rider_land[0].flags = blocks[i];
        if (!carried((NJS_POINT3){11, 0, 0}, (NJS_VECTOR){-24, 0, 0},
                     (NJS_VECTOR){-6, 0, 0})) {
            return 13;
        }
    }
    for (unsigned i = 0; i < sizeof passes / sizeof *passes; i++) {
        rider_land[0].flags = passes[i];
        if (!carried((NJS_POINT3){11, 0, 0}, (NJS_VECTOR){-24, 0, 0},
                     (NJS_VECTOR){-24, 0, 0})) {
            return 14;
        }
    }
    rider_table.count = 0;
    return 0;
}

/* An oblique wall redirects a purely westward carry into a wall south of
 * its original swept box. Both contacts must survive a crowded first scan. */
static Bool redirected_carry(void)
{
    static const NJS_POINT3 south[4] = {
        {-64, -64, -13}, {64, -64, -13}, {64, 64, -13}, {-64, 64, -13}};
    for (int copies = 1; copies <= 20; copies += 19) {
        if (!air_wall_scene(1, 0, 0, NULL)) {
            return FALSE;
        }
        for (struct collision_mesh *mesh = collision_mesh_list; mesh;
             mesh = mesh->next) {
            collision_mesh_clear_buckets();
            if (mesh->object == objects) {
                ground_quad(south, 0, 0, 0);
            } else {
                for (int i = 0; i < copies; i++) {
                    air_wall_triangle(1, 0, 1, 2);
                    air_wall_triangle(1, 0, 2, 3);
                }
            }
            mesh->span = 64;
            collision_mesh_take_buckets(mesh);
        }
        rider_table.count = 2;
        for (int i = 0; i < 2; i++) {
            rider_land[i] = (struct land_entry){
                .flags = WORLD_SOLID, .object = objects + i, .radius = 128};
        }
        if (!carried((NJS_POINT3){20, 0, 0}, (NJS_VECTOR){-40, 0, 0},
                     (NJS_VECTOR){-19.75F, 0, -8})) {
            return FALSE;
        }
    }
    rider_table.count = 0;
    return TRUE;
}

static Bool carry_pool_exhaustion(void)
{
    static struct polygon pool[2];
    static NJS_POINT3 points[3] = {{0, -32, -32}, {0, 32, -32}, {0, 0, 32}};
    static Sint16 indices[] = {0, 1, 2, 0, 1, 2};
    static NJS_MESHSET set = {
        .type_matId = NJD_MESHSET_3, .nbMesh = 2, .meshes = indices};
    collision_mesh_init();
    polygon_pool_init(pool, 2);
    model = (NJS_MODEL){
        .points = points, .nbPoint = 3, .meshsets = &set, .nbMeshset = 1};
    world_objects[0] = (struct world_object){.flags = WORLD_SOLID};
    rider_table.count = 1;
    rider_land[0] = (struct land_entry){
        .flags = WORLD_SOLID, .object = objects, .radius = 128};
    NJS_POINT3 center = {10, 0, 0};
    NJS_VECTOR travel = {-24, 0, 0};
    if (!update(1, 0) || !collision_mesh_list->out_of_polygons ||
        (collision_mesh_list->polygons &&
         collision_mesh_list->polygons->center.z != __builtin_inff()) ||
        polygon_limit_carry(&center, 5, &travel) ||
        !near_point(travel, (NJS_VECTOR){0})) {
        return FALSE;
    }
    collision_mesh_free(collision_mesh_list);
    model.nbMeshset = 0;
    if (!update(1, 0) || collision_mesh_list->out_of_polygons ||
        !carried(center, (NJS_VECTOR){-24, 0, 0}, (NJS_VECTOR){-24, 0, 0})) {
        return FALSE;
    }
    rider_table.count = 0;
    model = (NJS_MODEL){0};
    return TRUE;
}

static int rides;
static NJS_POINT3 rider_center;

static void observe_ride(struct task *task, struct task_work *work,
                         struct task_force *force)
{
    rides++;
    rider_center = work->pos;
}

static int platform_rides(void)
{
    static struct task_work platform_work = {.flags = WORLD_TESTED};
    static struct task_force forces[2];
    static struct task platform = {.work = &platform_work, .force = forces};
    for (int slot = 0; slot < 2; slot++) {
        if (!landing_corner(1, slot, slot)) {
            return 1;
        }
        /* The player's own query contains only the platform. Its carry
         * reaches a wall outside that query. */
        world_count = 1;
        world_objects[0] =
            (struct world_object){.flags = WORLD_SOLID | WORLD_MOVES,
                                  .object = objects,
                                  .owner = &platform};
        rider_table.count = 1;
        rider_land[0] = (struct land_entry){
            .flags = WORLD_SOLID, .object = objects + 1, .radius = 128};
        forces[1 - slot] = (struct task_force){.speed = {100, 0, 0}};
        forces[slot] = (struct task_force){
            .speed = {-24, 0, 8}, .turn = {0, 64, 0}, .carry = observe_ride};
        rides = 0;
        begin_stride(2, (NJS_POINT3){11, 0, -4}, 1);
        struct task_work *w = player_work[0];
        struct task_motion *m = player_motion[0];
        struct player *r = player_records[0];
        PLAYER_VARS(w)->player = slot;
        player_work[slot] = w;
        player_motion[slot] = m;
        player_records[slot] = r;
        r->ground_task = &platform;
        r->last_ground_flags = r->ground_flags = WORLD_SOLID | WORLD_MOVES;
        if (move_in_world(w, m, r) || rides != 1 || w->ang.y != 64 ||
            r->ground_task != &platform || !(w->flags & PLAYER_ON_GROUND) ||
            !near_point(rider_center, (NJS_POINT3){12, 5, -4}) ||
            !near_point(w->pos, (NJS_POINT3){5, 0, 4}) ||
            !near_point(r->contact->pos, (NJS_POINT3){5, 5, 4}) ||
            !near_point(m->speed, (NJS_VECTOR){1, 0, 0}) ||
            !near_point(forces[slot].speed, (NJS_VECTOR){-24, 0, 8}) ||
            world_count != 1 || world_objects[0].object != objects) {
            return 2;
        }
        for (struct collision_mesh *mesh = collision_mesh_list; mesh;
             mesh = mesh->next) {
            if (mesh->object == objects + 1 && mesh->near_count) {
                return 3;
            }
        }
        /* A stationary rider stays flush with the wall, slides along it,
         * then follows the platform freely as it reverses. */
        m->speed = r->speed = (NJS_VECTOR){0};
        for (int tick = 0; tick < 2; tick++) {
            r->last_flags = w->flags;
            r->last_ground_flags = r->ground_flags;
            r->ground_flags = 0;
            if (tick) {
                forces[slot].speed = (NJS_VECTOR){3, 0, -2};
            }
            NJS_POINT3 end =
                tick ? (NJS_POINT3){8, 0, 10} : (NJS_POINT3){5, 0, 12};
            if (move_in_world(w, m, r) || rides != tick + 2 ||
                r->ground_task != &platform || !near_point(w->pos, end) ||
                !near_point(m->speed, (NJS_VECTOR){0})) {
                return 4;
            }
        }
        /* Once airborne the rider no longer consumes the platform delta. */
        begin_fall(6, (NJS_POINT3){8, 32, 10}, (NJS_VECTOR){2, 3, 0});
        if (move_in_world(w, m, r) || rides != 3 || r->ground_task ||
            !near_point(w->pos, (NJS_POINT3){10, 35, 10})) {
            return 5;
        }
    }
    player_work[1] = NULL;
    player_motion[1] = NULL;
    player_records[1] = NULL;
    rider_table.count = 0;
    return 0;
}

static int rotating_floor(void)
{
    static struct task_work deck = {.pos = {3, 0, -20}, .flags = WORLD_TESTED};
    static struct task_forces bank;
    struct task_force *forces = bank.riders;
    static struct task floor = {.work = &deck, .force = &bank};
    static struct task_work second = {.pos = {3, 0, -24},
                                      .flags = PLAYER_ON_LAND};
    static struct player other = {.ground_task = &floor};
    if (!landing_corner(1, 0, 0)) {
        return 1;
    }
    world_count = 1;
    world_objects[0] =
        (struct world_object){WORLD_SOLID | WORLD_MOVES, objects, &floor};
    rider_table.count = 1;
    rider_land[0] = (struct land_entry){
        .flags = WORLD_SOLID, .object = objects + 1, .radius = 128};
    begin_stride(2, (NJS_POINT3){11, 0, -4}, 0);
    struct task_work *work = player_work[0];
    struct task_motion *motion = player_motion[0];
    struct player *record = player_records[0];
    record->ground_task = &floor;
    player_work[1] = &second;
    player_records[1] = &other;
    /* Both riders stand on the same floor at different radii. The first
     * rider's chord crosses the wall at x = 0. */
    twinkle_turn_riders(&floor, -0x4000);
    if (!near_point(forces[0].speed, (NJS_VECTOR){-24, 0, -8}) ||
        !near_point(forces[1].speed, (NJS_VECTOR){4, 0, 4}) ||
        forces[0].turn.y != 0x4000 || forces[1].turn.y != 0x4000 ||
        !near_point(work->pos, (NJS_POINT3){11, 0, -4}) ||
        move_in_world(work, motion, record) || record->ground_task != &floor ||
        !near_point(work->pos, (NJS_POINT3){5, 0, -12}) ||
        !near_point(motion->speed, (NJS_VECTOR){0}) || work->ang.y != 0x4000) {
        return 2;
    }
    /* A grounded bystander and an airborne former rider get no carry. */
    record->ground_task = NULL;
    second.flags = 0;
    twinkle_turn_riders(&floor, 0x4000);
    if (!near_point(forces[0].speed, (NJS_VECTOR){0}) || forces[0].turn.y ||
        !near_point(forces[1].speed, (NJS_VECTOR){0}) || forces[1].turn.y) {
        return 3;
    }
    second.flags = PLAYER_ON_LAND;
    twinkle_turn_riders(&floor, 0x4000);
    if (!near_point(forces[1].speed, (NJS_VECTOR){-4, 0, 4}) ||
        forces[1].turn.y != -0x4000) {
        return 4;
    }
    twinkle_turn_riders(&floor, 0);
    player_work[1] = NULL;
    player_records[1] = NULL;
    rider_table.count = 0;
    return !near_point(forces[1].speed, (NJS_VECTOR){0}) || forces[1].turn.y
               ? 5
               : 0;
}

static Bool jumps_with(int slot, struct task_work *work, struct player *record,
                       NJS_VECTOR expected)
{
    struct task_motion motion = {.accel = {9, 8, 7}};
    struct task_motion *previous = player_motion[slot];
    player_motion[slot] = &motion;
    work->flags = PLAYER_ON_LAND;
    record->speed = (NJS_VECTOR){0};
    pad_records[slot].press = pad_jump_buttons;
    Bool jumped = tails_try_jump(work, record);
    pad_records[slot].press = 0;
    Bool inherited = near_point(motion.accel, expected);
    player_accel_air(work, &motion, record);
    player_apply_accel(work, &motion, record);
    player_motion[slot] = previous;
    expected.y += record->consts.jump_speed;
    return jumped && inherited && near_point(motion.speed, expected);
}

static int platform_jumps(void)
{
    static struct task_work deck;
    static struct task_forces forces;
    static struct task floor = {.work = &deck, .force = &forces};
    static struct task_work second;
    static struct player other;
    begin_stride(2, (NJS_POINT3){-8, 0, 0}, 0);
    struct task_work *first = player_work[0];
    struct player *one = player_records[0];
    /* Zero gravity and drag isolate the launch velocity on the first air step.
     */
    PLAYER_VARS(first)->character = CHARACTER_TAILS;
    one->consts = (struct character_constants){
        .jump_speed = 2.5F, .speed_limit = 32, .fall_limit = 32};
    one->ground_task = &floor;
    second = (struct task_work){.flags = PLAYER_ON_LAND, .pos = {8, 0, 0}};
    PLAYER_VARS(&second)->player = 1;
    PLAYER_VARS(&second)->character = CHARACTER_TAILS;
    other = *one;
    player_work[1] = &second;
    player_records[1] = &other;
    twinkle_turn_riders(&floor, 0x4000);
    if (!jumps_with(0, first, one, (NJS_VECTOR){8, 0, 8}) ||
        !jumps_with(1, &second, &other, (NJS_VECTOR){-8, 0, -8})) {
        return 1;
    }
    /* At the pivot a rider has zero linear velocity while the deck turns. */
    second.pos = deck.pos;
    twinkle_turn_riders(&floor, -0x4000);
    if (!jumps_with(1, &second, &other, (NJS_VECTOR){0})) {
        return 2;
    }
    /* A translating lift can publish one velocity for both riders. */
    static struct task_forces lift_forces = {.riders = {{.speed = {1, 2, 3}}}};
    struct task lift = {.force = &lift_forces};
    other.ground_task = &lift;
    if (!jumps_with(1, &second, &other, lift_forces.riders[0].speed)) {
        return 3;
    }
    lift.force = NULL;
    if (!jumps_with(1, &second, &other, (NJS_VECTOR){9, 8, 7})) {
        return 4;
    }
    player_work[1] = NULL;
    player_records[1] = NULL;
    return 0;
}

/* Both traffic actors keep the previous collision object in vars.p[2].
 * Their next pose is in work when they publish the ride for this tick. */
static int traffic_riders(void (*carry)(struct task *))
{
    if (!landing_corner(1, 0, 0)) {
        return 1;
    }
    struct task_work body = {
        .pos = {4, 0, 2}, .ang = {0, 0x4000, 0}, .flags = WORLD_TESTED};
    struct task_motion car_motion = {.accel = {2, 0, -1}};
    struct task_forces forces = {0};
    struct task car = {.work = &body, .motion = &car_motion, .force = &forces};
    NJS_OBJECT *solid = objects;
    body.vars.p[2] = solid;
    solid->pos[0] = 2;
    solid->pos[1] = 0;
    solid->pos[2] = -1;
    solid->ang[0] = solid->ang[1] = solid->ang[2] = 0;
    world_count = 1;
    world_objects[0] = (struct world_object){WORLD_MOVES, solid, &car};
    rider_table.count = 0;
    begin_stride(2, (NJS_POINT3){9, 0, 3}, 0);
    struct task_work *first = player_work[0];
    struct player *one = player_records[0];
    one->ground_task = &car;
    one->ground_flags = one->last_ground_flags = WORLD_MOVES;
    struct task_work second = {.pos = {-4, 0, 1}, .flags = PLAYER_ON_LAND};
    struct player two = {.ground_task = &car};
    PLAYER_VARS(&second)->player = 1;
    PLAYER_VARS(&second)->character = CHARACTER_TAILS;
    player_work[1] = &second;
    player_records[1] = &two;
    forces.riders[0].turn.y = -0x4000;
    carry(&car);
    /* A quarter turn carries (7, 0, 4) to (4, 0, -7) about the old pivot.
     * The second rider starts on the other side of the roof. */
    if (!near_point(forces.riders[0].speed, (NJS_VECTOR){-1, 0, -8}) ||
        !near_point(forces.riders[1].speed, (NJS_VECTOR){10, 0, 7}) ||
        forces.riders[1].turn.y != -0x4000 ||
        !near_point(first->pos, (NJS_POINT3){9, 0, 3}) ||
        move_in_world(first, player_motion[0], one) ||
        one->ground_task != &car || !(first->flags & PLAYER_ON_LAND) ||
        !near_point(first->pos, (NJS_POINT3){8, 0, -5}) ||
        (Uint16)first->ang.y != 0xc000) {
        return 2;
    }
    two.consts = (struct character_constants){
        .jump_speed = 2.5F, .speed_limit = 32, .fall_limit = 32};
    if (!jumps_with(1, &second, &two, (NJS_VECTOR){10, 0, 7})) {
        return 3;
    }
    /* Transform the local roof points (8, 3, 4) and (-5, 4, -6) by
     * each collision pose. All three Euler angles change. */
    solid->ang[0] = 0x800;
    solid->ang[1] = 0x2400;
    solid->ang[2] = -0x400;
    body.ang = (struct angles){0xa00, 0x2600, -0x100};
    first->pos = (NJS_POINT3){10.730889F, 1.312538F, -4.323977F};
    second.pos = (NJS_POINT3){-4.584141F, 5.766811F, -0.373113F};
    second.flags = PLAYER_ON_LAND;
    two.ground_task = &car;
    carry(&car);
    NJS_POINT3 first_end = first->pos, second_end = second.pos;
    njAddVector(&first_end, &forces.riders[0].speed);
    njAddVector(&second_end, &forces.riders[1].speed);
    if (!near_point(first_end,
                    (NJS_POINT3){12.512646F, 1.729783F, -1.680043F}) ||
        !near_point(second_end,
                    (NJS_POINT3){-2.739590F, 5.505062F, 3.127929F}) ||
        forces.riders[0].turn.y != -0x200 ||
        forces.riders[1].turn.y != -0x200) {
        return 4;
    }
    /* On Speed Highway the first moving car advances from route distance
     * 500 to 504.78. Its roof center is directly above the yaw pivot. */
    if (carry == traffic_car_carry) {
        solid->pos[0] = -118.11554F;
        solid->pos[1] = -2;
        solid->pos[2] = -1297.12621F;
        solid->ang[0] = solid->ang[2] = 0;
        solid->ang[1] = 44588;
        body.pos = (NJS_POINT3){-116.32884F, -2, -1301.67069F};
        body.ang = (struct angles){0, 44996, 0};
        first->pos = (NJS_POINT3){solid->pos[0], 21.568623F, solid->pos[2]};
        carry(&car);
        if (!near_point(forces.riders[0].speed,
                        (NJS_VECTOR){body.pos.x - solid->pos[0], 0,
                                     body.pos.z - solid->pos[2]}) ||
            forces.riders[0].turn.y != -408) {
            return 6;
        }
    }
    /* Takeoff leaves the old support pointer until the next terrain query.
     * A grounded player beside the car can belong to another support. */
    first->flags &= ~PLAYER_ON_LAND;
    two.ground_task = NULL;
    carry(&car);
    if (!near_point(forces.riders[0].speed, (NJS_VECTOR){0}) ||
        !near_point(forces.riders[1].speed, (NJS_VECTOR){0}) ||
        forces.riders[0].turn.y || forces.riders[1].turn.y) {
        return 5;
    }
    player_work[1] = NULL;
    player_records[1] = NULL;
    carry(&car);
    return 0;
}

NJS_ACTION egg_lift_action;

/* Two samples from the inside Egg Carrier lift's action, at steps 167
 * and 168 of its ascent. The fixture supplies animation poses to the actor. */
void njGetAction(NJS_ACTION *action, NJS_MOTION_DATA *data, Float frame)
{
    data->pos[0] = 0;
    data->pos[1] = frame < 50.25F ? 109.2571F : 110.5288F;
    data->pos[2] = -240;
    data->ang[0] = data->ang[2] = 0;
    data->ang[1] = frame < 50.25F ? 71102 : 72731;
}

static int inside_lift_riders(void)
{
    NJS_OBJECT solid = {0};
    struct task_work body = {0};
    struct task_forces forces = {0};
    struct task lift = {.work = &body, .force = &forces};
    body.vars.p[0] = &solid;
    body.vars.f[2] = 50.099922F;
    egg_lift_move(&lift, &body, forces.riders);
    begin_stride(2, (NJS_POINT3){1, 109.2571F, -240}, 0);
    player_records[0]->ground_task = &lift;
    struct task_work second = {.flags = PLAYER_ON_LAND, .pos = body.pos};
    struct player other = {.ground_task = &lift};
    player_work[1] = &second;
    player_records[1] = &other;
    /* Player 0 can board within one unit of the axis; a rider on the axis
     * just rises. Both must follow the lift's heading. */
    body.vars.f[2] = 50.399921F;
    egg_lift_move(&lift, &body, forces.riders);
    if (!near_point(forces.riders[0].speed,
                    (NJS_VECTOR){-0.01217108F, 1.2717F, -0.15554428F}) ||
        !near_point(forces.riders[1].speed, (NJS_VECTOR){0, 1.2717F, 0}) ||
        forces.riders[0].turn.y != -1629 || forces.riders[1].turn.y != -1629 ||
        solid.ang[1] != 72731 || solid.pos[1] != 110.5288F) {
        return 1;
    }
    egg_lift_move(&lift, &body, forces.riders);
    player_work[1] = NULL;
    player_records[1] = NULL;
    return !near_point(forces.riders[0].speed, (NJS_VECTOR){0}) ||
           !near_point(forces.riders[1].speed, (NJS_VECTOR){0}) ||
           forces.riders[0].turn.y || forces.riders[1].turn.y;
}

static void begin_climb(int slot, struct task *previous)
{
    static struct collision_work actor;
    begin_stride(2, (NJS_POINT3){0, 2, -4}, 0);
    struct task_work *work = player_work[0];
    struct player *record = player_records[0];
    PLAYER_VARS(work)->character = CHARACTER_KNUCKLES;
    PLAYER_VARS(work)->player = slot;
    work->collision = &actor;
    work->ang.z = 0xc000; /* local up points out of the wall at x = 0 */
    record->consts = character_constants[CHARACTER_KNUCKLES];
    record->ground_task = previous;
    record->contact->pos = (NJS_POINT3){1000, 2, -4};
}

static int climbing_walls(void)
{
    static struct task_work platform_work = {.flags = WORLD_TESTED};
    static struct task_force forces[2];
    static struct task platform = {.work = &platform_work};
    static const NJS_POINT3 barrier[4] = {
        {-64, -64, 12}, {-64, 64, 12}, {64, 64, 12}, {64, -64, 12}};
    if (!landing_corner(1, 0, 0)) {
        return 1;
    }
    struct collision_mesh *wall = collision_mesh_list;
    struct collision_mesh *obstacle = wall->next;
    if (obstacle->object == objects + 1) {
        wall = obstacle;
        obstacle = collision_mesh_list;
    }
    collision_mesh_clear_buckets();
    ground_quad(barrier, 0, 0, 0);
    obstacle->span = 0;
    collision_mesh_take_buckets(obstacle);
    world_count = 1;
    world_objects[0] =
        (struct world_object){.object = objects + 1,
                              .owner = &platform,
                              .flags = WORLD_SOLID | WORLD_MOVES};
    platform.force = forces;
    for (int slot = 0; slot < 2; slot++) {
        for (int attached = 0; attached < 2; attached++) {
            begin_climb(slot, attached ? &platform : NULL);
            struct task_work *w = player_work[0];
            struct task_motion *m = player_motion[0];
            struct player *r = player_records[0];
            forces[0] = (struct task_force){.speed = {0, 0, 8}};
            forces[1] = (struct task_force){.speed = {0, 0, 2}};
            m->speed = r->speed = (NJS_VECTOR){0, 1, 0};
            for (int tick = 1; tick <= 3; tick++) {
                NJS_POINT3 end = {0, 2 + tick,
                                  -4 + tick * forces[slot].speed.z};
                if (player_collide_climb(w, m, r) ||
                    !(w->flags & PLAYER_ON_LAND) ||
                    r->ground_task != &platform ||
                    r->ground_object != objects + 1 ||
                    !near_point(w->pos, end) ||
                    !near_point(r->contact->pos, end) ||
                    !near_point(m->speed, (NJS_VECTOR){0, 1, 0})) {
                    return 2;
                }
            }
        }
    }
    /* The static barrier first meets the side of Knuckles' body while the
     * wall carries him. He keeps climbing along it and can ride away. */
    begin_climb(0, &platform);
    struct task_work *w = player_work[0];
    struct task_motion *m = player_motion[0];
    struct player *r = player_records[0];
    rider_table.count = 1;
    rider_land[0] = (struct land_entry){
        .flags = WORLD_SOLID, .object = objects, .radius = 128};
    forces[0] = (struct task_force){.speed = {0, 2, 24}};
    Float stop = 12 - r->consts.height * 0.75F;
    for (int tick = 1; tick <= 3; tick++) {
        if (tick == 3) {
            forces[0].speed.z = -4;
        }
        NJS_POINT3 end = {0, 2 + 2 * tick, stop - (tick == 3 ? 4 : 0)};
        if (player_collide_climb(w, m, r) || !near_point(w->pos, end) ||
            !near_point(m->speed, (NJS_VECTOR){0}) ||
            r->ground_task != &platform) {
            return 3;
        }
    }
    /* Change supports, then publish the wall's new pose on the next tick. */
    rider_table.count = 0;
    struct task_force previous_forces[2] = {{0}, {.speed = {17, 0, 0}}};
    struct task previous = {.work = &platform_work, .force = previous_forces};
    begin_climb(1, &previous);
    forces[1] = (struct task_force){.speed = {2, 0, 0}};
    if (player_collide_climb(w, m, r) || r->ground_task != &platform ||
        !near_point(w->pos, (NJS_POINT3){2, 2, -4})) {
        return 4;
    }
    struct angles angle = w->ang;
    for (struct polygon *p = wall->polygons; p; p = p->next) {
        p->center.x += 2;
        for (int i = 0; i < 3; i++) {
            p->vertex[i].x += 2;
        }
        if (p == wall->last) {
            break;
        }
    }
    forces[1] = (struct task_force){.speed = {-1, 0, 0}, .turn = {13, 17, 19}};
    if (player_collide_climb(w, m, r) || !(w->flags & PLAYER_ON_LAND) ||
        !near_point(w->pos, (NJS_POINT3){1, 2, -4}) ||
        !near_point(r->contact->pos, w->pos) || w->ang.x != angle.x + 13 ||
        w->ang.y != angle.y + 17 || w->ang.z != angle.z + 19) {
        return 5;
    }
    platform.force = NULL;
    begin_climb(1, &platform);
    w->pos.x = 2;
    if (player_collide_climb(w, m, r) || !(w->flags & PLAYER_ON_LAND) ||
        r->ground_task != &platform ||
        !near_point(w->pos, (NJS_POINT3){2, 2, -4})) {
        return 6;
    }
    /* Only the off-center ledge probe can reach this small floor patch. */
    for (int direction = -1; direction <= 1; direction += 2) {
        begin_climb(0, NULL);
        Float x = -direction * r->consts.height * 0.25F;
        Float y = direction * 8.0F;
        NJS_POINT3 patch[4] = {
            {x - 1, y, -5}, {x - 1, y, -3}, {x + 1, y, -3}, {x + 1, y, -5}};
        collision_mesh_clear_buckets();
        ground_quad(patch, 0, 0, 0);
        collision_mesh_take_buckets(wall);
        wall->polygons->radius = wall->last->radius = 2;
        wall->span = 2;
        m->speed.y = direction * 2.0F;
        r->contact->hit.normal = (NJS_VECTOR){0};
        if (player_collide_climb(w, m, r) || (w->flags & PLAYER_ON_LAND) ||
            !near_point(r->contact->hit.normal, (NJS_VECTOR){1, 1, 0})) {
            return 7;
        }
    }
    return 0;
}

struct climb_wall {
    int axis, sign;
    Float gap; /* surface clearance from the initial body, negative inside */
    int quads;
    Uint32 flags;
};

static Bool climb_scene(const struct climb_wall *walls, int count, int split,
                        int reverse)
{
    Float radius = character_constants[CHARACTER_KNUCKLES].height * 0.75F;
    collision_mesh_init();
    int objects_count = split ? count : 1;
    for (int i = 0; i < objects_count; i++) {
        world_objects[i] = (struct world_object){.flags = WORLD_SOLID};
    }
    if (!update(objects_count, 0)) {
        return FALSE;
    }
    for (struct collision_mesh *mesh = collision_mesh_list; mesh;
         mesh = mesh->next) {
        collision_mesh_clear_buckets();
        for (int i = 0; i < count; i++) {
            int index = reverse ? count - i - 1 : i;
            if (split && mesh->object != objects + index) {
                continue;
            }
            const struct climb_wall *wall = &walls[index];
            int axis = wall->axis;
            NJS_POINT3 quad[4];
            for (int k = 0; k < 4; k++) {
                Float p[3] = {radius, 2, -4};
                p[axis] -= wall->sign * (radius + wall->gap);
                p[(axis + 1) % 3] += (k == 1 || k == 2 ? 48 : -48) * wall->sign;
                p[(axis + 2) % 3] += k >= 2 ? 48 : -48;
                quad[k] = (NJS_POINT3){p[0], p[1], p[2]};
            }
            for (int j = 0; j < wall->quads; j++) {
                ground_quad(quad, (j + reverse) % 2, reverse, 0);
            }
        }
        collision_mesh_take_buckets(mesh);
        mesh->span = 96;
    }
    for (int i = 0; i < objects_count; i++) {
        int index = reverse ? objects_count - i - 1 : i;
        world_objects[i] = (struct world_object){
            .object = objects + index,
            .flags = WORLD_SOLID | (split ? walls[index].flags : 0)};
    }
    return TRUE;
}

static Float begin_knuckles_air(int mode, NJS_POINT3 start, NJS_VECTOR travel,
                                struct collision_work *actor)
{
    begin_fall(mode, start, travel);
    struct task_work *work = player_work[0];
    struct player *record = player_records[0];
    PLAYER_VARS(work)->character = CHARACTER_KNUCKLES;
    record->consts = character_constants[CHARACTER_KNUCKLES];
    Float radius = record->consts.height * 0.5F;
    work->collision = actor;
    work->ang.y = 0x8000;
    work->flags = PLAYER_GUARD;
    actor->shapes->center = start;
    actor->shapes->center.y += radius;
    player_take_speed(work, player_motion[0], record);
    return radius;
}

/* He can reach the wall while rising or after his jump turns downward.
 * The descending grab uses the contact point on the wall as his new feet. */
static int jump_to_climb(void)
{
    enum { GLIDE_CLIMB = 2, ACTION_GRAB_WALL = 52 };
    static const struct climb_wall wall = {0, 1, 0, 1};
    struct collision_shape shape = {0};
    struct collision_work actor = {.shapes = &shape};
    if (!climb_scene(&wall, 1, 0, 0)) {
        return 1;
    }
    for (int rising_hit = 0; rising_hit <= 1; rising_hit++) {
        NJS_POINT3 start = {rising_hit ? 8 : 11, 15, -4};
        Float radius = begin_knuckles_air(KNUCKLES_JUMP, start,
                                          (NJS_VECTOR){-3, 1, 1}, &actor);
        struct task_work *work = player_work[0];
        struct task_motion *motion = player_motion[0];
        struct player *record = player_records[0];
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        NJS_POINT3 top = {rising_hit ? radius : 8, 16, -3};
        if (work->mode != KNUCKLES_JUMP || (work->flags & PLAYER_ON_GROUND) ||
            !near_point(work->pos, top)) {
            return 2;
        }
        shape.center = work->pos;
        shape.center.y += radius;
        /* Keep the horizontal speed the rising movement actually produced. */
        record->speed.y = motion->speed.y = -0.5F;
        if (knuckles_glide_collide(work, motion, record) != GLIDE_CLIMB) {
            return rising_hit ? 4 : 3;
        }
        NJS_POINT3 grip = {0, top.y + radius - record->consts.snap, top.z};
        if (!near_point(record->contact->hit.point, grip)) {
            return 5;
        }
        knuckles_start_climb(work, motion, record);
        if (work->mode != KNUCKLES_CLIMB ||
            record->anim.action != ACTION_GRAB_WALL ||
            (work->flags & (PLAYER_ON_LAND | PLAYER_GUARD)) != PLAYER_ON_LAND ||
            !near_point(work->pos, grip) || work->ang.x ||
            (Uint16)work->ang.z != 0xc000 || record->speed.x ||
            record->speed.z) {
            return 6;
        }
        /* Once the grab animation ends, each climbing tick advances from
         * that grip and keeps the same wall. */
        for (int tick = 0; tick < 2; tick++) {
            motion->speed = record->speed = (NJS_VECTOR){0, 1, 0};
            grip.y++;
            if (player_collide_climb(work, motion, record) ||
                !(work->flags & PLAYER_ON_LAND) ||
                record->ground_object != objects ||
                !near_point(work->pos, grip)) {
                return 7;
            }
        }
    }
    return 0;
}

/* The forward sphere can reach the wall a tick before the body does.
 * Trying to grab it during a jump must leave that tick for movement. */
static int jump_reaches_wall(void)
{
    enum { GLIDE_BOUNCE = 1, GLIDE_CLIMB = 2 };
    static const struct climb_wall wall = {0, 1, 0, 1};
    struct collision_shape shape = {0};
    struct collision_work actor = {.shapes = &shape};
    const NJS_POINT3 start = {8.75F, 15, -4};
    const NJS_VECTOR travel = {-3, -0.5F, 1};
    const NJS_POINT3 end = {5.75F, 14.5F, -3};
    if (!climb_scene(&wall, 1, 0, 0)) {
        return 1;
    }
    for (int glide = 0; glide <= 1; glide++) {
        int mode = glide ? KNUCKLES_GLIDE : KNUCKLES_JUMP;
        Float radius = begin_knuckles_air(mode, start, travel, &actor);
        struct task_work *work = player_work[0];
        struct task_motion *motion = player_motion[0];
        struct player *record = player_records[0];
        record->last_flags = PLAYER_ATTACKING;
        record->wall_normal = (NJS_VECTOR){0, 0, 1};
        NJS_VECTOR local = record->speed;
        if (knuckles_glide_collide(work, motion, record) != GLIDE_BOUNCE) {
            return 2;
        }
        /* The same confirmation consumes movement in glide mode, where there
         * is no second player_collide call. */
        if (!glide) {
            if (!near_point(work->pos, start) || work->flags != PLAYER_GUARD ||
                record->last_flags != PLAYER_ATTACKING ||
                !near_point(motion->speed, travel) ||
                !near_point(record->speed, local) ||
                !near_point(record->wall_normal, (NJS_VECTOR){0, 0, 1}) ||
                !near_point(record->contact->hit.normal_speed,
                            (NJS_VECTOR){17, 19, 23})) {
                return 3;
            }
            if (player_collide(work, motion, record)) {
                return 4;
            }
        }
        player_take_speed(work, motion, record);
        if (!near_point(work->pos, end) || work->mode != mode ||
            (work->flags & PLAYER_ON_GROUND) ||
            !near_point(motion->speed, travel)) {
            return 5;
        }
        shape.center = work->pos;
        shape.center.y += radius;
        if (knuckles_glide_collide(work, motion, record) != GLIDE_CLIMB) {
            return 6;
        }
        knuckles_start_climb(work, motion, record);
        NJS_POINT3 grip = {0, end.y + radius - record->consts.snap, end.z};
        if (!near_point(work->pos, grip) || work->mode != KNUCKLES_CLIMB ||
            (work->flags & (PLAYER_ON_LAND | PLAYER_GUARD)) != PLAYER_ON_LAND) {
            return 7;
        }
    }
    return 0;
}

static int climb_corners(void)
{
    const struct climb_wall corner[] = {
        {0, 1, 0, 18}, {1, -1, -1, 1}, {2, 1, -1, 1}};
    for (int split = 0; split < 2; split++) {
        for (int reverse = 0; reverse < 2; reverse++) {
            if (!climb_scene(corner, 3, split, reverse)) {
                return 1;
            }
            begin_climb(0, NULL);
            struct task_work *w = player_work[0];
            struct task_motion *m = player_motion[0];
            struct player *r = player_records[0];
            for (int tick = 1; tick <= 4; tick++) {
                Float along = tick < 3 ? tick * 0.35355339F : 1.0F;
                if (player_collide_climb(w, m, r) ||
                    !near_point(w->pos,
                                (NJS_POINT3){0, 2 - along, -4 + along}) ||
                    !near_point(m->speed, (NJS_VECTOR){0})) {
                    return 2;
                }
            }
            begin_climb(0, NULL);
            pad_sticks[0].stroke = 1;
            int result = player_collide_climb(w, m, r);
            pad_sticks[0].stroke = 0;
            if (result || (w->flags & PLAYER_ON_LAND) ||
                !near_point(w->pos, (NJS_POINT3){0, 2, -4})) {
                return 3;
            }
        }
    }

    struct climb_wall channel[] = {
        {0, 1, 0, 1}, {2, 1, -1, 1}, {2, -1, 0.125F, 1}};
    for (int test = 0; test < 3; test++) {
        if (test == 1) {
            channel[2].gap = -0.125F;
        } else if (test == 2) {
            channel[2].gap = -0.125F;
            channel[2].flags = WORLD_UNCLIMBABLE;
        }
        if (!climb_scene(channel, 3, 1, test % 2)) {
            return 4;
        }
        begin_climb(0, NULL);
        for (int tick = 0; tick < 2; tick++) {
            if (player_collide_climb(player_work[0], player_motion[0],
                                     player_records[0]) ||
                !near_point(player_work[0]->pos,
                            (NJS_POINT3){0, 2, test ? -4 : -3.875F})) {
                return 5;
            }
        }
    }
    /* A wall beyond this tick is clear. A wall within it stops the grip
     * without pushing the resting sphere. Moving out of an overlap is free. */
    struct climb_wall route[] = {{0, 1, 0, 1}, {2, -1, 3, 1}};
    for (int test = 0; test < 3; test++) {
        route[1].gap = test == 0 ? 3 : test == 1 ? 1 : -0.25F;
        if (!climb_scene(route, 2, 0, 0)) {
            return 6;
        }
        begin_climb(0, NULL);
        struct task_work *w = player_work[0];
        struct task_motion *m = player_motion[0];
        struct player *r = player_records[0];
        m->speed.z = r->speed.z = test == 2 ? -2 : 2;
        if (player_collide_climb(w, m, r) ||
            ((w->flags & PLAYER_ON_LAND) != 0) != (test != 1) ||
            !near_point(w->pos, (NJS_POINT3){0, 2,
                                             test == 0   ? -2
                                             : test == 1 ? -4
                                                         : -6})) {
            return 7;
        }
    }
    return 0;
}

/* Seventy-three distinct planes share a corner. The two outermost normals
 * have Z component 0.8, so a 0.125 overlap needs 0.15625 along Z. */
static int climb_fan(void)
{
    struct climb_wall support = {0, 1, 0, 1};
    for (int reverse = 0; reverse < 2; reverse++) {
        if (!climb_scene(&support, 1, 0, 0)) {
            return 1;
        }
        begin_climb(0, NULL);
        Float r = player_records[0]->consts.height * 0.75F;
        collision_mesh_clear_buckets();
        for (int i = -36; i <= 36; i++) {
            Float slope = (reverse ? -i : i) / 48.0F;
            Float z = _builtin_fsrra(1 + slope * slope);
            Float y = slope * z;
            NJS_POINT3 quad[4];
            for (int j = 0; j < 4; j++) {
                Float across = j >= 2 ? 32 : -32;
                quad[j] = (NJS_POINT3){r + (j == 1 || j == 2 ? 32 : -32),
                                       2 - (r - 0.125F) * y + across * z,
                                       -4 - (r - 0.125F) * z - across * y};
            }
            ground_quad(quad, reverse, reverse, 0);
        }
        struct collision_mesh *mesh = collision_mesh_list;
        collision_mesh_take_buckets(mesh);
        mesh->span = 64;
        struct task_work *w = player_work[0];
        for (int incomplete = 1; incomplete >= 0; incomplete--) {
            mesh->out_of_polygons = incomplete;
            if (player_collide_climb(w, player_motion[0], player_records[0]) ||
                !near_point(w->pos,
                            (NJS_POINT3){0, 2, incomplete ? -4 : -3.84375F})) {
                return 2;
            }
        }
    }
    return 0;
}

/* The back wall fixes Z. At each new Y, the next tilted wall has the
 * largest remaining overlap. Sixteen tilted walls need seventeen additions
 * even though only the back and final walls bind the completed projection. */
static int climb_many_constraints(void)
{
    struct {
        Float sine, overlap;
    } panel[17] = {{0}};
    Float sine = 0.85F, stride = 0.495F / sine, height = 0;
    for (int i = 1; i <= 16; i++) {
        height += stride;
        panel[i].sine = sine;
        panel[i].overlap = sine * height;
        sine /= 2.428F;
        stride *= 1.42F;
    }
    Float radius = character_constants[CHARACTER_KNUCKLES].height * 0.75F;
    const struct climb_wall wall = {0, 1, 0, 1};
    for (int layout = 0; layout < 4; layout++) {
        int reverse = layout & 1;
        for (int count = 15; count <= 16; count++) {
            if (!climb_scene(&wall, 1, 0, 0)) {
                return 1;
            }
            collision_mesh_clear_buckets();
            for (int j = 0; j <= count; j++) {
                int i = reverse ? count - j : j;
                Float y = panel[i].sine;
                Float z = i ? njSqrt(1 - y * y) : -1;
                Float distance = radius - panel[i].overlap;
                NJS_POINT3 quad[4];
                for (int k = 0; k < 4; k++) {
                    Float across = k >= 2 ? 1024 : -1024;
                    quad[k] =
                        (NJS_POINT3){radius + (k == 1 || k == 2 ? 48 : -48),
                                     2 - distance * y + across * z,
                                     -4 - distance * z - across * y};
                }
                ground_quad(quad, reverse, reverse, 0);
                if (layout >= 2) {
                    /* The rescan also works when the 64-contact cache fills. */
                    ground_quad(quad, reverse, reverse, 0);
                }
            }
            collision_mesh_take_buckets(collision_mesh_list);
            collision_mesh_list->span = 2048;
            for (int allow = 0; allow <= 1; allow++) {
                begin_climb(0, NULL);
                int requests = heap_requests, frees = heap_frees;
                heap_enabled = allow;
                struct task_work *work = player_work[0];
                Uint32 result = player_collide_climb(work, player_motion[0],
                                                     player_records[0]);
                heap_enabled = FALSE;
                Bool grows = count == 16;
                NJS_POINT3 expected = {0, grows && !allow ? 2 : 2.5F, -4};
                if (result || !near_point(work->pos, expected)) {
                    return 2;
                }
                if (heap_busy || heap_bad_free ||
                    heap_requests - requests != grows ||
                    heap_frees - frees != (grows && allow)) {
                    return 3;
                }
            }
        }
    }
    return 0;
}

/* The projection of the body's center misses the whole quad. Its side can
 * still meet the left edge, or just the lower left vertex. */
static int climb_rims(void)
{
    struct climb_wall support = {0, 1, 0, 1};
    if (!climb_scene(&support, 1, 0, 0)) {
        return 1;
    }
    struct collision_mesh *mesh = collision_mesh_list;
    NJS_VECTOR up = {1, 0, 0};
    for (int vertex = 0; vertex < 2; vertex++) {
        for (int both = 0; both < 2; both++) {
            Float low = vertex ? 3 : -20;
            NJS_POINT3 quad[] = {
                {4, low, 0}, {4, 20, 0}, {20, 20, 0}, {20, low, 0}};
            collision_mesh_clear_buckets();
            ground_quad(quad, 0, 0, both ? NJD_FLAG_DOUBLE_SIDE : 0);
            collision_mesh_take_buckets(mesh);
            mesh->span = 0;
            for (int side = -1; side <= 1; side += 2) {
                for (int distance = 1; distance <= 4; distance += 3) {
                    struct polygon_ball probe = {
                        .pos = {0, 0, 10 * side},
                        .radius = 8,
                        .speed = {0, 0, -side * distance}};
                    polygon_collect_near(&probe);
                    Bool caught = both || side < 0;
                    caught = caught && distance == 4;
                    if (polygon_block_climb(&probe, &up, NULL) != caught ||
                        (caught && !near_point(probe.hit.normal,
                                               (NJS_POINT3){0, 0, side}))) {
                        return 2;
                    }
                }
            }
        }
    }
    return 0;
}

/* Use the same broad-phase bounds for each vertex order. */
static void climb_triangle(const NJS_POINT3 v[3], int first)
{
    collision_mesh_clear_buckets();
    struct polygon *tri = polygon_alloc(0);
    for (int i = 0; i < 3; i++) {
        tri->vertex[i] = v[(first + i) % 3];
    }
    tri->attr = 0;
    tri->center = (NJS_POINT3){512, 512, 512};
    tri->radius = 1024 * 1024;
    collision_mesh_take_buckets(collision_mesh_list);
    collision_mesh_list->span = 1024;
}

static int climb_thin_faces(void)
{
    const struct climb_wall wall = {2, 1, 0, 1};
    if (!climb_scene(&wall, 1, 0, 0)) {
        return 1;
    }
    const NJS_VECTOR up = {1, 0, 0};
    const NJS_POINT3 tip[3] = {
        {0, 0, 0}, {1024, 1024, 0}, {1024 - 1.0F / 128, 1024 + 1.0F / 128, 0}};
    NJS_POINT3 sliver[3] = {{0, 0, 0},
                            {1024, 1023.875F, 1023.75F},
                            {1023.875F, 1023.75F, 1023.625F}};
    for (int first = 0; first < 3; first++) {
        climb_triangle(tip, first);
        /* Past the sharp tip the nearest point is the origin. */
        for (int moving = 0; moving <= 1; moving++) {
            for (int hit = 0; hit <= 1; hit++) {
                Float xy = hit ? -0.75F : -1.25F;
                struct polygon_ball body = {
                    .pos = {xy, xy, moving ? 3.0F : 1.5F},
                    .radius = 2,
                    .speed = {0, 0, moving ? -1.5F : 0}};
                if (polygon_collect_near(&body) != 1 ||
                    polygon_block_climb(&body, &up, NULL) != hit) {
                    return 2;
                }
            }
        }
        climb_triangle(sliver, first);
        /* The exact cross product is (-1, 2, -1) / 64. */
        struct polygon_ball probe = {.pos = {511.75F, 512.4375F, 511.625F},
                                     .radius = 1};
        const NJS_VECTOR normal = {-0.40824829F, 0.81649658F, -0.40824829F};
        if (polygon_collect_near(&probe) != 1 ||
            !polygon_block_climb(&probe, &up, NULL) ||
            !near_point(probe.hit.normal, normal)) {
            return 3;
        }
    }
    sliver[2] = (NJS_POINT3){512, 511.9375F, 511.875F};
    climb_triangle(sliver, 0);
    struct polygon_ball probe = {.pos = sliver[2], .radius = 1};
    return polygon_collect_near(&probe) != 1 ||
                   polygon_block_climb(&probe, &up, NULL)
               ? 4
               : 0;
}

/* A flat wall gives the same clearance across every internal diagonal.
 * A gap between two tiles leaves both facing edges exposed. */
static int climb_seams(void)
{
    const struct climb_wall walls[2] = {{2, 1, 0, 1}, {2, 1, 0, 1}};
    const NJS_VECTOR up = {1, 0, 0};
    const Float depths[] = {0.125F, 0.75F, 3.0F, 8.25F};
    for (int layout = 0; layout < 2; layout++) {
        if (!climb_scene(walls, 2, layout, layout)) {
            return 1;
        }
        for (struct collision_mesh *mesh = collision_mesh_list; mesh;
             mesh = mesh->next) {
            collision_mesh_clear_buckets();
            for (int j = -3; j < 3; j++) {
                for (int i = -3; i < 3; i++) {
                    if (layout && mesh->object != objects + ((i + j + 6) % 2)) {
                        continue;
                    }
                    Float x = (layout ? -i - 1 : i) * 4 + 0.25F;
                    Float y = j * 4 + 0.625F;
                    NJS_POINT3 quad[] = {{x, y, 0},
                                         {x + 4, y, 0},
                                         {x + 4, y + 4, 0},
                                         {x, y + 4, 0}};
                    ground_quad(quad, (i + j + 6 + layout) % 2, layout, 0);
                }
            }
            collision_mesh_take_buckets(mesh);
            mesh->span = 0;
        }
        for (int d = 0; d < 4; d++) {
            for (int speed = -1; speed <= 1; speed++) {
                struct polygon_ball body = {.pos = {0, 1, 9 - depths[d]},
                                            .radius = 9,
                                            .speed = {0, speed, 0}};
                struct polygon_ball region = body;
                region.radius += POLYGON_CLIMB_PUSH;
                polygon_collect_near(&region);
                NJS_POINT3 feet = {0};
                Bool blocked = polygon_block_climb(&body, &up, &feet);
                Float push = speed ? 0
                             : depths[d] < POLYGON_CLIMB_PUSH
                                 ? depths[d]
                                 : POLYGON_CLIMB_PUSH;
                if (blocked != !speed ||
                    !near_point(feet, (NJS_POINT3){0, 0, push})) {
                    return 2;
                }
            }
        }
    }
    if (!climb_scene(walls, 1, 0, 0)) {
        return 1;
    }
    collision_mesh_clear_buckets();
    for (int sign = -1; sign <= 1; sign += 2) {
        Float x = sign < 0 ? -24 : 4;
        NJS_POINT3 tile[] = {
            {x, -16, 0}, {x + 20, -16, 0}, {x + 20, 16, 0}, {x, 16, 0}};
        ground_quad(tile, 0, 0, 0);
    }
    collision_mesh_take_buckets(collision_mesh_list);
    for (int sign = -1; sign <= 1; sign += 2) {
        struct polygon_ball body = {
            .pos = {0, 0, 3}, .radius = 4, .speed = {12 * sign, 0, 0}};
        polygon_collect_near(&body);
        if (!polygon_block_climb(&body, &up, NULL)) {
            return 3;
        }
    }
    return 0;
}

/* The conveyor bar publishes a world-space box at its animated center. */
static int box_headings(void)
{
    struct collision_shape body = {.flags = SHAPE_SPHERE | SHAPE_HURTABLE,
                                   .radius = 2};
    struct collision_shape bar = {.flags = SHAPE_RECTANGLE,
                                  .levels = SHAPE_HURTS_PLAYERS | ATTACK(2),
                                  .push =
                                      PUSH_PLAYERS | PUSH_FIRM(PUSH_PLAYERS),
                                  .radius = 45,
                                  .size_y = 44.5F,
                                  .size_z = 10};
    struct task_work player = {0},
                     object = {.ang = {0, 0x4000, 0}, .pos = {40, 0, 70}};
    struct task actor = {.work = &player}, obstacle = {.work = &object};
    struct collision_work a = {.kind = COLLISION_PLAYER,
                               .count = 1,
                               .shapes = &body,
                               .owner = &actor,
                               .radius = 25};
    struct collision_work b = {.kind = COLLISION_OBJECT,
                               .count = 1,
                               .shapes = &bar,
                               .owner = &obstacle,
                               .radius = 100};
    struct task_work *previous = player_work[0];
    player.collision = &a;
    object.collision = &b;
    player_work[0] = &player;
    const struct {
        Uint32 flags;
        Angle task, shape, heading;
    } poses[] = {
        {0, 0x4000, 0, 0x4000},
        {SHAPE_UNTURNED, 0x4000, 0x4000, 0x4000},
        {SHAPE_IN_WORLD, 0x4000, 0x4000, 0x4000},
        {0, -0x2000, 0x4000, 0x2000},
        {SHAPE_IN_WORLD, -17510, -17510, -17510},
        {SHAPE_UNTURNED, -0x2000, 0, 0},
    };
    for (unsigned i = 0; i < sizeof(poses) / sizeof(poses[0]); ++i) {
        bar.flags = SHAPE_RECTANGLE | poses[i].flags;
        bar.ang.y = poses[i].shape;
        object.ang.y = poses[i].task;
        bar.center = bar.flags & SHAPE_IN_WORLD ? object.pos : (NJS_POINT3){0};
        for (int inside = 0; inside < 2; ++inside) {
            NJS_POINT3 offset =
                inside ? (NJS_POINT3){35, 0, 11} : (NJS_POINT3){11, 0, 35};
            njPushMatrix(&_unit_matrix_);
            njRotateY(NULL, poses[i].heading);
            njCalcVector(NULL, &offset, &player.pos);
            njPopMatrix(1);
            player.pos.x += object.pos.x;
            player.pos.z += object.pos.z;
            player_set_history(0, 1, &player.pos, NULL);
            collision_register(&player);
            collision_register(&object);
            collision_collide_all();
            NJS_POINT3 final = {player.pos.x - object.pos.x, 0,
                                player.pos.z - object.pos.z};
            njPushMatrix(&_unit_matrix_);
            njRotateY(NULL, -poses[i].heading);
            njCalcVector(NULL, &final, &offset);
            njPopMatrix(1);
            if ((a.hit == &b) != inside ||
                !!(player.flags & COLLISION_DAMAGED) != inside ||
                (inside &&
                 (!(a.flags & COLLISION_HIT_SIDE) || offset.z < 11.9F))) {
                player_work[0] = previous;
                return 2 * i + inside + 1;
            }
        }
    }
    player_work[0] = previous;
    return 0;
}

static Bool sweep_head(struct task *head, struct task_work *player,
                       const NJS_POINT3 *from, const NJS_POINT3 *to)
{
    return collision_arc_player(head->extra, head, player, from, to);
}

/* A stationary head gives an analytic check on sliding. At (-1.6, 1.2)
 * on the two-unit sphere, motion along x detaches after 2 * log(3) units. */
static Bool slide_over_head(void)
{
    struct collision_arc arc = {.start = {10, 0, 0}};
    const struct {
        NJS_POINT3 from, to, expected;
        Bool hit;
    } cases[] = {
        {{7, 0, 0}, {17, 0, 0}, {8, 0, 0}, TRUE},
        {{10, 0, 0}, {11, 0, 0}, {13, 0, 0}, TRUE},
        {{8.4F, 1.2F, 0}, {9.4F, 1.2F, 0}, {8.927877F, 1.688358F, 0}, TRUE},
        {{8.4F, 1.2F, 0}, {12.4F, 1.2F, 0}, {11.802775F, 2, 0}, TRUE},
        {{7, 2, 0}, {13, 2, 0}, {13, 2, 0}, FALSE},
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        NJS_VECTOR push = {0};
        Bool hit = collision_arc_sweep(&arc, &arc.start, &cases[i].from,
                                       &cases[i].to, 2, &push);
        NJS_POINT3 end = cases[i].to;
        VECTOR_ADD(&end, &push);
        if (hit != cases[i].hit || !near_point(end, cases[i].expected)) {
            return FALSE;
        }
    }
    return TRUE;
}

/* A quarter turn reaches beyond its chord. The player can cross the
 * same point before, during, or after the head passes it. */
static int swinging_head(void)
{
    struct collision_shape body = {.flags = SHAPE_HURTABLE, .radius = 0.5F};
    struct collision_shape head = {.levels = SHAPE_HURTS_PLAYERS | ATTACK(2),
                                   .radius = 0.5F};
    struct task_work player = {0}, object = {0};
    struct task actor = {.work = &player}, obstacle = {.work = &object};
    struct collision_arc arc = {.pivot = {63, -29, 135}};
    obstacle.extra = &arc;
    struct collision_work a = {.kind = COLLISION_PLAYER,
                               .count = 1,
                               .shapes = &body,
                               .owner = &actor,
                               .radius = 1};
    struct collision_work b = {.kind = COLLISION_OBJECT,
                               .count = 1,
                               .shapes = &head,
                               .owner = &obstacle,
                               .radius = 1,
                               .sweep = sweep_head};
    struct task_work *previous = player_work[0];
    enum body_change { MOVE, ANIMATE, WARP, DISABLE, PUSH, FIRM, RESIZE };
    const struct {
        NJS_POINT3 from, to;
        enum body_change change;
        Bool hit;
    } cases[] = {
        {{7.75F, 0, -7.75F}, {7.75F, 0, -7.75F}, MOVE, TRUE},
        {{7.8F, 0, -7.8F}, {7.8F, 0, -7.8F}, MOVE, FALSE},
        {{5, 0, -5}, {5, 0, -5}, MOVE, FALSE},
        {{7.07F, -4, -7.07F}, {7.07F, 4, -7.07F}, ANIMATE, TRUE},
        {{10, 0, 8}, {10, 0, -0.2F}, MOVE, FALSE},
        {{10, 0, 0}, {0, 0, -10}, MOVE, TRUE},
        {{7.75F, 0, -7.75F}, {7.75F, 0, -7.75F}, WARP, FALSE},
        {{7.75F, 0, -7.75F}, {7.75F, 0, -7.75F}, DISABLE, FALSE},
        {{7.75F, 0, -7.75F}, {7.75F, 0, -7.75F}, PUSH, TRUE},
        {{7.75F, 0, -7.75F}, {7.75F, 0, -7.75F}, FIRM, TRUE},
        {{5, 0, -5}, {5, 0, -5}, PUSH, FALSE},
        {{7.75F, 0, -7.75F}, {7.75F, 0, -7.75F}, RESIZE, FALSE},
    };
    player.collision = &a;
    object.collision = &b;
    player_work[0] = &player;
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        for (int reverse = 0; reverse < 2; ++reverse) {
            arc.start = (NJS_POINT3){reverse ? 0 : 10, 0, reverse ? -10 : 0};
            object.pos = (NJS_POINT3){reverse ? 10 : 0, 0, reverse ? 0 : -10};
            VECTOR_ADD(&arc.start, &arc.pivot);
            VECTOR_ADD(&object.pos, &arc.pivot);
            NJS_POINT3 start = reverse ? cases[i].to : cases[i].from;
            NJS_POINT3 end = reverse ? cases[i].from : cases[i].to;
            VECTOR_ADD(&start, &arc.pivot);
            VECTOR_ADD(&end, &arc.pivot);
            /* An animated body can cross between ticks while its task stands
             * still. */
            Bool solid = cases[i].change == PUSH || cases[i].change == FIRM;
            Bool animated = cases[i].change == ANIMATE || solid;
            body.push = cases[i].change == FIRM ? PUSH_FIRM(PUSH_OBJECTS) : 0;
            body.flags = SHAPE_HURTABLE | (animated ? SHAPE_IN_WORLD : 0);
            player.pos = animated ? arc.pivot : start;
            body.center = animated ? start : (NJS_POINT3){0};
            player_set_history(0, 0, &player.pos, NULL);
            body.radius = head.radius = 0.5F;
            head.flags = head.push = head.levels = 0;
            collision_register(&player);
            collision_register(&object);
            collision_collide_all();
            if (cases[i].change == WARP) {
                player.pos.y += 100;
            }
            player_set_history(0, 1, &player.pos, NULL);
            player.pos = animated ? arc.pivot : end;
            body.center = animated ? end : (NJS_POINT3){0};
            head.flags = cases[i].change == DISABLE ? SHAPE_OFF : 0;
            head.push = solid ? PUSH_PLAYERS : 0;
            body.radius = cases[i].change == RESIZE ? 0.75F : 0.5F;
            head.levels = solid ? 0 : SHAPE_HURTS_PLAYERS | ATTACK(2);
            collision_register(&player);
            collision_register(&object);
            collision_collide_all();
            Bool pushed = cases[i].change == PUSH && cases[i].hit;
            NJS_VECTOR shift;
            VECTOR_DIFF(&shift, &body.center, &end);
            NJS_POINT3 feet = arc.pivot;
            VECTOR_ADD(&feet, &shift);
            if ((a.hit == &b) != cases[i].hit ||
                !!(player.flags & COLLISION_DAMAGED) !=
                    (cases[i].hit && !solid) ||
                !!(a.flags & COLLISION_PUSHED) != pushed ||
                (solid && (!near_point(player.pos, feet) ||
                           (pushed ? !(shift.x > 0 && shift.z < 0)
                                   : VECTOR_SQUARED(&shift) != 0)))) {
                player_work[0] = previous;
                return 2 * i + reverse + 1;
            }
        }
    }
    /* The resting head overlaps the player beside a stone wall at x = 0.
     * Its full push would put the body through the wall. */
    if (!air_wall_scene(1, 0, 0, NULL)) {
        player_work[0] = previous;
        return 30;
    }
    rider_table.count = 1;
    rider_land[0] = (struct land_entry){
        .flags = WORLD_SOLID, .object = objects, .radius = 128};
    arc.pivot = (NJS_POINT3){0};
    object.pos = arc.start = (NJS_POINT3){2, 0, 0};
    body.flags = SHAPE_IN_WORLD | SHAPE_HURTABLE;
    body.push = head.push = head.levels = 0;
    body.radius = 0.5F;
    head.radius = 1.5F;
    player.pos = body.center = (NJS_POINT3){1, 0, 0};
    player_set_history(0, 0, &player.pos, NULL);
    collision_register(&player);
    collision_register(&object);
    collision_collide_all();
    Bool held = TRUE;
    NJS_POINT3 history = player.pos;
    for (int tick = 0; tick < 4; ++tick) {
        player_set_history(0, 1, &history, NULL);
        history = player.pos;
        player_set_history(0, 0, &history, NULL);
        head.push = PUSH_PLAYERS;
        head.flags = tick == 3 ? SHAPE_OFF : 0;
        collision_register(&player);
        collision_register(&object);
        collision_collide_all();
        if ((a.hit == &b) != (tick < 3) ||
            !!(a.flags & COLLISION_PUSHED) != (tick < 3) ||
            !near_point(body.center, (NJS_POINT3){0.5F, 0, 0}) ||
            !near_point(player.pos, body.center)) {
            held = FALSE;
        }
    }
    rider_table.count = 0;
    player_work[0] = previous;
    return held ? 0 : 31;
}

extern void hammer_head_follow(struct task *head);

/* Another mover already knows the player's previous pose when the hammer
 * first registers. Only its second publication spans a movement interval. */
static Bool hammer_appears(void)
{
    struct collision_shape body = {.flags = SHAPE_HURTABLE, .radius = 1};
    struct collision_shape head = {.levels = SHAPE_HURTS_PLAYERS | ATTACK(2),
                                   .radius = 1};
    struct task_work player = {0}, work = {0}, pivot = {0};
    struct task_motion arm = {0};
    struct task parent = {.work = &pivot, .motion = &arm},
                actor = {.work = &player};
    struct task mover = {.work = &work, .parent = &parent};
    struct collision_work a = {.kind = COLLISION_PLAYER,
                               .count = 1,
                               .shapes = &body,
                               .owner = &actor,
                               .radius = 1};
    struct collision_work b = {.kind = COLLISION_OBJECT,
                               .count = 1,
                               .shapes = &head,
                               .owner = &mover,
                               .radius = 1};
    struct collision_arc observer = {.start = {10, 0, 0}};
    struct task_work *previous = player_work[0];
    player_work[0] = &player;
    player.collision = &a;
    work.collision = &b;
    work.scl = work.pos = observer.start;
    player.pos.x = 4;
    player_set_history(0, 0, &player.pos, NULL);
    mover.extra = &observer;
    b.sweep = sweep_head;
    collision_register(&player);
    collision_register(&work);
    collision_collide_all();
    b.sweep = NULL;
    Bool correct = TRUE;
    for (int tick = 0; tick < 2; tick++) {
        player_set_history(0, 1, &player.pos, NULL);
        player.pos.x = tick ? 4 : 16;
        player_set_history(0, 0, &player.pos, NULL);
        collision_register(&player);
        hammer_head_follow(&mover);
        collision_collide_all();
        if ((a.hit == &b) != (tick == 1) ||
            !!(player.flags & COLLISION_DAMAGED) != (tick == 1)) {
            correct = FALSE;
        }
    }
    player_work[0] = previous;
    return correct;
}

/* A Hot Shelter gear tooth can pass between consecutive body positions.
 * Both endpoints clear this tip; its upper front corner clips the sphere. */
static int owned_tooth(void)
{
    static NJS_POINT3 points[] = {{0, 32, 0}, {-8, 16, 0}, {8, 16, 0}};
    static Sint16 strip[] = {3, 0, 1, 2};
    static NJS_MESHSET set = {
        .type_matId = NJD_MESHSET_TRIMESH, .nbMesh = 1, .meshes = strip};
    static NJS_MATERIAL material;
    static NJS_MODEL tooth = {.points = points,
                              .nbPoint = 3,
                              .meshsets = &set,
                              .nbMeshset = 1,
                              .mats = &material,
                              .nbMat = 1,
                              .r = 64};
    static struct task_work gear_work = {.flags = WORLD_TESTED};
    static struct task gear = {.work = &gear_work};
    struct collision_work collision = {0};
    NJS_OBJECT *object = &objects[0];
    *object = (NJS_OBJECT){.model = &tooth, .scl = {1, 1, 1}};
    collision_mesh_init();
    world_count = 1;
    world_objects[0] = (struct world_object){
        .owner = &gear,
        .object = object,
        .flags = WORLD_LARGE | WORLD_MOVES | WORLD_GRIP_HIGH};
    /* Turn the same tooth face up. Landing acquires its owner; a jump
     * from the platform must release both the owner and the ground flag. */
    object->ang[0] = -0x4000;
    collision_mesh_update();
    begin_fall(amy_terrain_modes.fall, (NJS_POINT3){0, 6, -24},
               (NJS_VECTOR){0, -10, 0});
    struct task_work *work = player_work[0];
    struct task_motion *motion = player_motion[0];
    struct player *record = player_records[0];
    work->collision = &collision;
    PLAYER_VARS(work)->character = CHARACTER_AMY;
    record->consts = character_constants[CHARACTER_AMY];
    player_collide(work, motion, record);
    if (record->ground_task != &gear || record->ground_object != object ||
        !(work->flags & PLAYER_ON_GROUND)) {
        return 6;
    }
    work->mode = amy_terrain_modes.jump;
    work->flags &= ~PLAYER_ON_TASK;
    motion->speed = record->speed = (NJS_VECTOR){0, 12, 0};
    NJS_POINT3 departure = work->pos;
    departure.y += 12;
    player_collide(work, motion, record);
    if (work->flags & PLAYER_ON_GROUND) {
        return 7;
    }
    if (!near_point(work->pos, departure)) {
        return 8;
    }
    if (record->ground_task) {
        return 9;
    }
    player_collide(work, motion, record);
    if (record->ground_task || record->ground_object ||
        (work->flags & PLAYER_ON_GROUND)) {
        return 9;
    }
    object->ang[0] = 0;
    for (int pose = 0; pose < 2; pose++) {
        object->ang[2] = -180 * pose;
        collision_mesh_update();
        if (!collision_mesh_list || !collision_mesh_list->polygons) {
            return 1;
        }
        Float tip_x = -32 * njSin(object->ang[2]);
        Float tip_y = 32 * njCos(object->ang[2]);
        /* The back face and a front-side near miss share the broad phase
         * with the real contact. Neither may deflect the player. */
        static const Float offsets[] = {-2.4F, 4.2F, 2.4F};
        for (int approach = 0; approach < 3; approach++) {
            NJS_POINT3 start = {tip_x + 4, tip_y - 2, offsets[approach]};
            begin_fall(amy_terrain_modes.fall, start, (NJS_VECTOR){-8, 0, 0});
            work = player_work[0];
            motion = player_motion[0];
            record = player_records[0];
            work->collision = &collision;
            PLAYER_VARS(work)->character = CHARACTER_AMY;
            record->consts = character_constants[CHARACTER_AMY];
            struct polygon_ball query = {.pos = {start.x, start.y + 5, start.z},
                                         .speed = motion->speed,
                                         .radius = 5};
            if (!polygon_collect_near(&query) ||
                !collision_mesh_list->near_count) {
                return 10;
            }
            player_collide(work, motion, record);
            if (record->ground_task || record->ground_object ||
                (work->flags & PLAYER_ON_GROUND)) {
                return 2;
            }
            if (approach != 2) {
                start.x -= 8;
                if (!near_point(work->pos, start) ||
                    !near_point(motion->speed, (NJS_VECTOR){-8, 0, 0})) {
                    return 3;
                }
            } else {
                /* At t=0.1 the radius vector is (3.2, 3, 2.4). */
                NJS_POINT3 end = {tip_x - 1.05088F, tip_y + 0.7648F, 4.61184F};
                if (!near_point(work->pos, end) ||
                    !near_point(motion->speed,
                                (NJS_VECTOR){-4.7232F, 3.072F, 2.4576F})) {
                    return 4 + pose;
                }
            }
        }
    }
    return 0;
}

/* The Red Mountain fan's world object uses its blade model. The base and
 * pole are separate entity shapes. The focused actor never draws. */
NJS_TEXLIST obj_mountain_texlist;
NJS_MODEL mountain_propeller_base_model, mountain_propeller_blades_model;
NJS_MODEL mountain_propeller_base_far_model,
    mountain_propeller_blades_far_model;
static NJS_MESHSET fan_set = {
    .type_matId = NJD_MESHSET_TRIMESH, .nbMesh = 3, .meshes = fan_strip};
static NJS_MATERIAL fan_material;
static NJS_MODEL fan_model = {.points = fan_points,
                              .nbPoint = 7,
                              .meshsets = &fan_set,
                              .nbMeshset = 1,
                              .mats = &fan_material,
                              .nbMat = 1,
                              .r = 118.53348F};
NJS_OBJECT mountain_propeller_world_object[1] = {{.model = &fan_model}};
void mountain_propeller_run(struct task *task);

Bool placement_editing(struct task *task)
{
    return FALSE;
}
Bool draw_in_view(Float x, Float y, Float z, Float radius)
{
    return FALSE;
}
void draw_model(NJS_MODEL *mesh)
{
}
Sint32 njSetTexture(NJS_TEXLIST *textures)
{
    return 0;
}

struct fan_fixture {
    struct task_work work;
    struct task_forces forces;
    struct task task;
    NJS_OBJECT solid;
};

static void fan_setup(struct fan_fixture *fan, Sint16 turn, Float amplitude,
                      Sint16 phase_step)
{
    collision_mesh_init();
    *fan = (struct fan_fixture){0};
    fan->task.work = &fan->work;
    fan->task.force = &fan->forces;
    fan->work.flags = WORLD_TESTED;
    fan->work.scl.y = amplitude;
    fan->work.scl.z = phase_step;
    fan->work.vars.p[0] = &fan->solid;
    fan->work.vars.w[3] = turn;
    fan->solid = (NJS_OBJECT){.model = &fan_model, .scl = {1, 1, 1}};
    world_count = 1;
    world_objects[0] = (struct world_object){
        .flags = WORLD_MOVES, .owner = &fan->task, .object = &fan->solid};
    rider_table.count = 0;
}

/* Players run at task level one, the fan at three. The mesh is then
 * rebuilt. The following player tick consumes the published motion. */
static Bool fan_advance(struct fan_fixture *fan)
{
    mountain_propeller_run(&fan->task);
    collision_mesh_update();
    return collision_mesh_list && collision_mesh_list->polygons;
}

static int fan_crossing(struct fan_fixture *fan, int route)
{
    static const NJS_POINT3 centers[] = {{101.847245F, 19.479752F, -9.263280F},
                                         {100.809239F, 25.347618F, -11.583552F},
                                         {99.556699F, 13.779752F, -9.054949F}};
    NJS_POINT3 from = centers[route >= 3 ? 0 : route];
    from.y -= 5.7F;
    NJS_VECTOR v = {.y = route == 1 ? -12 : 0};
    begin_fall(knuckles_terrain_modes.fall, from, v);
    struct task_work *work = player_work[0];
    struct player *record = player_records[0];
    PLAYER_VARS(work)->character = CHARACTER_KNUCKLES;
    record->consts = character_constants[CHARACTER_KNUCKLES];
    struct collision_shape primary = {.center = from};
    struct collision_work body = {.shapes = &primary};
    work->collision = &body;
    fan_setup(fan, 700, 3, 920);
    if (route == 4) {
        /* Leave a quarter unit between the body and the wall. The blade's
         * unobstructed push would take the body more than a unit through it. */
        static NJS_POINT3 face[] = {{107.8F, -64, -64},
                                    {107.8F, -64, 64},
                                    {107.8F, 64, 64},
                                    {107.8F, 64, -64}};
        static Sint16 indices[] = {3, 0, 1, 2, 3, 0, 2, 3};
        static NJS_MESHSET set = {
            .type_matId = NJD_MESHSET_TRIMESH, .nbMesh = 2, .meshes = indices};
        static NJS_MODEL wall = {.points = face,
                                 .nbPoint = 4,
                                 .meshsets = &set,
                                 .nbMeshset = 1,
                                 .mats = &fan_material,
                                 .nbMat = 1,
                                 .r = 160};
        static NJS_OBJECT obstacle = {.model = &wall, .scl = {1, 1, 1}};
        world_objects[world_count++] =
            (struct world_object){.flags = WORLD_SOLID, .object = &obstacle};
    }
    if (!fan_advance(fan)) {
        return 1;
    }
    if (route == 3) {
        /* A distant fan keeps its phase. Re-entry publishes only the
         * current tick, and cannot sweep from the last visible pose. */
        primary.center.x += 1000;
        Angle angle = fan->solid.ang[1];
        Float height = fan->solid.pos[1];
        for (int i = 0; i < 8; i++) {
            mountain_propeller_run(&fan->task);
            if ((fan->work.flags & WORLD_TESTED) || fan->forces.rotation ||
                fan->solid.ang[1] != angle || fan->solid.pos[1] != height) {
                return 2;
            }
        }
        primary.center = from;
        if (!fan_advance(fan) || !fan->forces.rotation ||
            (Uint16)fan->solid.ang[1] != (Uint16)-6300) {
            return 2;
        }
        struct polygon_yaw next;
        fan->forces.rotation->pose(&fan->task, &next);
        Float rise = 3 * (njSin(9200) - njSin(8280));
        if (next.object != &fan->solid || next.turn != -700 ||
            !near_point(next.travel, (NJS_VECTOR){0, rise, 0})) {
            return 2;
        }
    }
    struct task_motion *motion = player_motion[0];
    player_collide(work, motion, record);
    NJS_POINT3 freely = {from.x, from.y + v.y, from.z};
    if (route == 4) {
        return work->pos.x <= 107.8F - 5.7F + 0.003F &&
                       !near_point(work->pos, freely)
                   ? 0
                   : 5;
    }
    if (route >= 2) {
        return near_point(work->pos, freely) && near_point(motion->speed, v) &&
                       !record->ground_task
                   ? 0
                   : 3;
    }
    if (work->pos.x < freely.x + 0.25F || motion->speed.x < 0.25F) {
        return 4;
    }
    return 0;
}

/* Without the blade's force the standing controller can still settle the
 * feet onto its pitched surface. Compare each rider against that same
 * controller on the stationary fan. */
static int fan_rider_step(struct fan_fixture *fan, Bool turning,
                          NJS_POINT3 end[2])
{
    static const NJS_POINT3 feet[] = {{-29.897929F, 20.311162F, 53.458295F},
                                      {90.236590F, 19.390376F, -17.648992F}};
    fan_setup(fan, turning ? 440 : 0, turning ? 2 : 0, 310);
    begin_stride(sonic_terrain_modes.walk, feet[0], 0);
    struct task_work second = *player_work[0];
    struct task_motion following = *player_motion[0];
    struct player tails = *player_records[0];
    struct polygon_ball ball = {0};
    struct collision_shape primary[2] = {{.center = feet[0]},
                                         {.center = feet[1]}};
    struct collision_work bodies[2] = {{.shapes = &primary[0]},
                                       {.shapes = &primary[1]}};
    tails.contact = &ball;
    tails.consts = character_constants[CHARACTER_TAILS];
    second.pos = feet[1];
    second.mode = tails_terrain_modes.walk;
    PLAYER_VARS(&second)->character = CHARACTER_TAILS;
    PLAYER_VARS(&second)->player = 1;
    player_work[1] = &second;
    player_motion[1] = &following;
    player_records[1] = &tails;
    for (int p = 0; p < 2; p++) {
        player_work[p]->collision = &bodies[p];
        player_records[p]->ground_task = &fan->task;
        player_records[p]->ground_object = &fan->solid;
        player_records[p]->ground_flags = player_records[p]->last_ground_flags =
            WORLD_MOVES;
    }
    int failure = 0;
    if (!fan_advance(fan)) {
        failure = 1;
    } else {
        for (int p = 0; p < 2; p++) {
            struct task_force *force = &fan->forces.riders[p];
            static const NJS_VECTOR carries[] = {
                {-2.227843F, 0.059433F, -1.308413F},
                {0.664015F, 0.059433F, 3.821155F}};
            NJS_VECTOR wanted = turning ? carries[p] : (NJS_VECTOR){0};
            if (!failure && (!near_point(force->speed, wanted) ||
                             force->turn.y != (turning ? 440 : 0))) {
                failure = 2 + p;
            }
            player_collide(player_work[p], player_motion[p], player_records[p]);
            end[p] = player_work[p]->pos;
            if (!failure && (player_records[p]->ground_task != &fan->task ||
                             !(player_work[p]->flags & PLAYER_ON_LAND))) {
                failure = 4 + p;
            }
        }
    }
    player_work[1] = NULL;
    player_motion[1] = NULL;
    player_records[1] = NULL;
    return failure;
}

static Bool clear_of_fan(shz_vec3_t center)
{
    center.y -= 0.264269F;
    struct yaw_path path = {.origin = center, .turn = -700};
    struct yaw_sample end = yaw_at(&path, 1);
    for (unsigned j = 0; j < sizeof fan_strip / sizeof *fan_strip; j += 4) {
        NJS_POINT3 v[] = {fan_points[fan_strip[j + 1]],
                          fan_points[fan_strip[j + 2]],
                          fan_points[fan_strip[j + 3]]};
        struct impact_triangle face;
        if (!impact_triangle_init(&face, v)) {
            return FALSE;
        }
        shz_vec3_t q = shz_vec3_sub(end.center, zam_load_vec3(v));
        unsigned edges = impact_outside(&face, q);
        shz_vec3_t radial =
            edges ? impact_rim(&face, q, edges)
                  : shz_vec3_scale(face.normal, plane_dot(q, face.normal));
        if (plane_dot(radial, radial) < 5.698F * 5.698F) {
            return FALSE;
        }
    }
    return TRUE;
}

/* The falling body lands on blade 15/3/4, whose normal has Y = 0.98294.
 * The standing body misses the fan on a low floor. Raising that floor places
 * the same body in the path of the blade tip, at rest or while walking. */
static int fan_support(struct fan_fixture *fan, int route, Angle pitch,
                       Bool spinning)
{
    NJS_POINT3 feet = route ? (NJS_POINT3){101.847245F, 0, -9.263280F}
                            : (NJS_POINT3){-31.334723F, 23.457581F, 52.635796F};
    NJS_VECTOR up = {0, 1, 0};
    struct task_work tilt = {.ang = {.x = pitch}};
    player_turn_to_world(&tilt, &up);
    if (route) {
        /* Hold the sphere center fixed while tilting the floor under it. */
        feet.z -= 5.7F * up.z;
        feet.y = route == 1 ? 0 : 19.479752F - 5.7F * up.y;
        begin_stride(route == 3 ? knuckles_terrain_modes.walk
                                : knuckles_terrain_modes.stand,
                     feet, route == 3 ? 0.125F : 0);
    } else {
        begin_fall(knuckles_terrain_modes.fall, feet, (NJS_VECTOR){0, -8, 0});
    }
    struct task_work *work = player_work[0];
    struct player *record = player_records[0];
    work->ang.x = pitch;
    record->ground_normal = up;
    PLAYER_VARS(work)->character = CHARACTER_KNUCKLES;
    record->consts = character_constants[CHARACTER_KNUCKLES];
    struct collision_shape primary = {.center = feet};
    struct collision_work body = {.shapes = &primary};
    work->collision = &body;
    fan_setup(fan, spinning ? 700 : 0, spinning ? 3 : 0, 920);
    static NJS_POINT3 points[] = {
        {64, 0, -48}, {64, 0, 48}, {144, 0, 48}, {144, 0, -48}};
    static Sint16 indices[] = {3, 0, 1, 2, 3, 0, 2, 3};
    static NJS_MESHSET set = {
        .type_matId = NJD_MESHSET_TRIMESH, .nbMesh = 2, .meshes = indices};
    static NJS_MODEL floor = {.points = points,
                              .nbPoint = 4,
                              .meshsets = &set,
                              .nbMeshset = 1,
                              .mats = &fan_material,
                              .nbMat = 1,
                              .r = 160};
    static NJS_OBJECT support = {.model = &floor, .scl = {1, 1, 1}};
    if (route) {
        for (int i = 0; i < 4; i++) {
            points[i].y = -(points[i].z - feet.z) * up.z / up.y;
        }
        support.pos[1] = feet.y;
        world_objects[world_count++] =
            (struct world_object){.flags = WORLD_SOLID, .object = &support};
        record->ground_object = &support;
    }
    if (!fan_advance(fan)) {
        return 1;
    }
    player_collide(work, player_motion[0], record);
    if (route >= 2 && spinning) {
        NJS_VECTOR offset = {0, 5.7F, 0};
        player_turn_to_world(work, &offset);
        shz_vec3_t center =
            shz_vec3_add(zam_load_vec3(&work->pos), zam_load_vec3(&offset));
        Float above = plane_dot(shz_vec3_sub(center, zam_load_vec3(&feet)),
                                zam_load_vec3(&up));
        return work->pos.x > feet.x + 0.25F && above >= 5.698F &&
                       clear_of_fan(center)
                   ? 0
                   : 5;
    }
    if (!(work->flags & PLAYER_ON_LAND)) {
        return 2;
    }
    if (route) {
        feet.x += route == 3 ? 0.125F : 0;
        return near_point(work->pos, feet) && !record->ground_task &&
                       record->ground_object == &support
                   ? 0
                   : 3;
    }
    return record->ground_object == &fan->solid &&
                   record->ground_task == &fan->task &&
                   record->ground_normal.y > 0.95F
               ? 0
               : 4;
}

static Bool fan_heap_reset(const struct polygon_yaw_ops *ops)
{
    heap_init();
    struct task *owners = heap_calloc(2, sizeof *owners);
    struct task_forces *forces = heap_calloc(2, sizeof *forces);
    if (!owners || !forces) {
        return FALSE;
    }
    for (int i = 0; i < 2; i++) {
        owners[i].force = &forces[i];
        polygon_rotate_with(&owners[i], ops);
    }
    heap_init();
    return polygon_rotating == NULL;
}

static int fan_contacts(void)
{
    static struct fan_fixture fan;
    int failure;
    for (int route = 0; route < 5; route++) {
        failure = fan_crossing(&fan, route);
        polygon_rotate_with(&fan.task, NULL);
        if (failure) {
            return 10 + route * 5 + failure;
        }
    }
    for (int route = 0; route < 4; route++) {
        failure = fan_support(&fan, route, 0, TRUE);
        polygon_rotate_with(&fan.task, NULL);
        if (failure) {
            return 60 + 5 * route + failure;
        }
    }
    for (int i = 0; i < 8; i++) {
        failure = fan_support(&fan, 2 + (i & 1), i & 4 ? -768 : 768, !(i & 2));
        polygon_rotate_with(&fan.task, NULL);
        if (failure) {
            return 90 + 6 * i + failure;
        }
    }
    NJS_POINT3 still[2], turned[2];
    for (int turn = 0; turn < 2; turn++) {
        failure = fan_rider_step(&fan, turn, turn ? turned : still);
        if (!failure && turn) {
            struct task_forces other = {0};
            struct task task = {.force = &other};
            const struct polygon_yaw_ops *ops = fan.forces.rotation;
            polygon_rotate_with(&task, ops);
            polygon_rotate_with(&fan.task, NULL);
            if (polygon_rotating != &task || other.next_rotating) {
                return 80;
            }
            polygon_rotate_with(&task, NULL);
            if (polygon_rotating) {
                return 81;
            }
            if (!fan_heap_reset(ops)) {
                return 82;
            }
        }
        polygon_rotate_with(&fan.task, NULL);
        if (failure) {
            return 40 + turn * 6 + failure;
        }
    }
    for (int p = 0; p < 2; p++) {
        NJS_VECTOR actual = {turned[p].x - still[p].x, turned[p].y - still[p].y,
                             turned[p].z - still[p].z};
        NJS_VECTOR expected = fan.forces.riders[p].speed;
        if (njAbs(actual.x - expected.x) > 0.04F ||
            njAbs(actual.y - expected.y) > 0.04F ||
            njAbs(actual.z - expected.z) > 0.04F) {
            return 53 + p;
        }
    }
    return 0;
}

/* All corners fall in one z bucket, so following prev from last visits the
 * triangles in submission order. Each index has a distinct transformed x. */
static Bool strip_triangle(struct polygon **cursor, int a, int b, int c,
                           Uint32 attr)
{
    const int ids[3] = {a, b, c};
    struct polygon *p = *cursor;
    if (!p || p->attr != attr) {
        return FALSE;
    }
    for (int i = 0; i < 3; i++) {
        int k = ids[i];
        NJS_POINT3 v = p->vertex[i];
        if (v.x != 7 + 2 * k || v.y != -5 + 3 * (k % 3) ||
            v.z != 4 + 4 * (k % 2)) {
            return FALSE;
        }
    }
    *cursor = p->prev;
    return TRUE;
}

static int mesh_strips(void)
{
    enum { POINTS = 1025, FACES = 7 + (POINTS - 2) + 2 };
    static NJS_POINT3 points[POINTS];
    static Sint16 strip[1 + POINTS + 5];
    static Sint16 triangles[] = {1, 2, 3, 4, 3, 2}, quad[] = {2, 1, 4, 3};
    static Sint16 counted[] = {0, 1, 9, 2, 4, 8, (Sint16)(NJD_TRIMESH_END | 5),
                               0, 1, 2, 3, 4};
    static NJS_MESHSET sets[] = {
        {.type_matId = NJD_MESHSET_3, .nbMesh = 2, .meshes = triangles},
        {.type_matId = NJD_MESHSET_4 | 1, .nbMesh = 1, .meshes = quad},
        {.type_matId = NJD_MESHSET_N | 2, .nbMesh = 4, .meshes = counted},
        {.type_matId = NJD_MESHSET_TRIMESH | 3, .nbMesh = 2, .meshes = strip}};
    static NJS_MATERIAL mats[] = {{.attrflags = 17},
                                  {.attrflags = 23},
                                  {.attrflags = 11},
                                  {.attrflags = 31}};
    static NJS_MODEL shapes[] = {{.points = points,
                                  .nbPoint = POINTS,
                                  .meshsets = sets,
                                  .nbMeshset = 4,
                                  .mats = mats,
                                  .nbMat = 4},
                                 {.points = points,
                                  .nbPoint = POINTS,
                                  .meshsets = sets,
                                  .nbMeshset = 1}};
    static NJS_OBJECT objs[] = {
        {.model = &shapes[0], .pos = {7, -5, 4}, .scl = {2, 3, 4}},
        {.model = &shapes[1], .pos = {7, -5, 4}, .scl = {2, 3, 4}}};
    for (int i = 0; i < POINTS; i++) {
        points[i] = (NJS_POINT3){i, i % 3, i % 2};
        strip[1 + i] = (i + 1) % POINTS;
    }
    const Sint16 tail[] = {(Sint16)(NJD_TRIMESH_END | 4), 73, 72, 71, 70};
    __builtin_memcpy(strip + 1 + POINTS, tail, sizeof tail);
    for (int pass = 0; pass < 3; pass++) {
        Bool short_pool = pass == 2;
        strip[0] = (Sint16)(POINTS | (pass == 1 ? NJD_TRIMESH_END : 0));
        collision_mesh_init();
        /* Reuse the production pool, including its reserved final record. */
        collision_mesh_clear_buckets();
        polygon_pool_init(polygon_alloc(0), FACES + 1 - short_pool);
        world_count = 2;
        for (int i = 0; i < 2; i++) {
            world_objects[i] =
                (struct world_object){.flags = WORLD_SOLID, .object = &objs[i]};
        }
        collision_mesh_update();
        for (int i = 0; i < 2; i++) {
            struct collision_mesh *m = collision_mesh_list;
            while (m && m->object != &objs[i]) {
                m = m->next;
            }
            Bool denied = i == 0 ? short_pool : !short_pool;
            if (!m || m->out_of_polygons != denied) {
                return 1;
            }
            struct polygon *p = m->last;
            if (m->out_of_polygons) {
                if (p && p->center.z != __builtin_inff()) {
                    return 2;
                }
                continue;
            }
            if (!strip_triangle(&p, 1, 2, 3, i ? 0 : 17) ||
                !strip_triangle(&p, 4, 3, 2, i ? 0 : 17)) {
                return 3;
            }
            if (i == 0) {
                if (!strip_triangle(&p, 2, 1, 4, 23) ||
                    !strip_triangle(&p, 1, 3, 4, 23) ||
                    !strip_triangle(&p, 0, 2, 1, 11) ||
                    !strip_triangle(&p, 1, 2, 3, 11) ||
                    !strip_triangle(&p, 2, 4, 3, 11)) {
                    return 4;
                }
                for (int j = 0; j < POINTS - 2; j++) {
                    int b = j + 2, c = (j + 3) % POINTS;
                    Bool flip = (j & 1) ^ (pass == 1);
                    if (!strip_triangle(&p, j + 1, flip ? c : b, flip ? b : c,
                                        31)) {
                        return 5;
                    }
                }
                if (!strip_triangle(&p, 73, 71, 72, 31) ||
                    !strip_triangle(&p, 72, 71, 70, 31)) {
                    return 6;
                }
            }
            if (p) {
                return 7;
            }
        }
    }
    return 0;
}

static int model_heights(void)
{
    enum { upper_count = 1028, lower_count = 38 };
    static NJS_POINT3 points[] = {{0, 2, 0}, {0, 2, 4}, {4, 2, 0},
                                  {0, 5, 0}, {0, 5, 4}, {4, 5, 0}};
    /* Three strips with too few indices precede the two real strips. */
    static Sint16 strips[6 + 1 + upper_count + 1 + lower_count] = {0, 1, 0,
                                                                   2, 0, 1};
    /* The fixed triangle ties the lower strip at y=2 with opposite winding. */
    static Sint16 tie[] = {0, 1, 2};
    static NJS_MESHSET sets[] = {
        {.type_matId = NJD_MESHSET_TRIMESH, .nbMesh = 5, .meshes = strips},
        {.type_matId = NJD_MESHSET_3, .nbMesh = 1, .meshes = tie}};
    static NJS_MODEL floor = {
        .points = points, .nbPoint = 6, .meshsets = sets, .nbMeshset = 2};
    NJS_OBJECT object = {.model = &floor,
                         .pos = {12, 20, -16},
                         .ang = {0x8000, 0, 0},
                         .scl = {2, 3, 4}};
    static const struct {
        Uint32 flags;
        NJS_POINT3 pos;
        Float below, above;
    } poses[] = {{NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL,
                  {1, 3, 1},
                  2,
                  5},
                 {NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL, {13, 23, -15}, 22, 25},
                 /* The half turn about X sends local y to 20 - 3*y. */
                 {0, {14, 11, -20}, 5, 14}};
    Sint16 *upper = strips + 6;
    Sint16 *lower = upper + 1 + upper_count;
    NJS_MATRIX before, after;

    /* Only the last three indices in each strip form a triangle. Both
     * triangles have odd parity. The upper strip reverses its winding. */
    upper[0] = (Sint16)(upper_count | NJD_TRIMESH_END);
    for (int i = 1; i <= upper_count - 2; i++) {
        upper[i] = 3;
    }
    upper[upper_count - 1] = 4;
    upper[upper_count] = 5;
    lower[0] = lower_count;
    lower[lower_count - 1] = 1;
    lower[lower_count] = 2;

    njPushMatrix(&_unit_matrix_);
    njTranslate(NULL, -19, 7, 13);
    njGetMatrix(&before);
    int depth = _stack_size_;
    for (unsigned i = 0; i < sizeof poses / sizeof *poses; i++) {
        struct height_query q = {.pos = poses[i].pos};
        object.evalflags = poses[i].flags;
        if (!polygon_find_model_heights(&q, &object) || !q.below.found ||
            !q.above.found || njAbs(q.below.level - poses[i].below) > 0.0001F ||
            njAbs(q.above.level - poses[i].above) > 0.0001F ||
            !near_point(q.below.normal, (NJS_VECTOR){0, -1, 0}) ||
            !near_point(q.above.normal, (NJS_VECTOR){0, 1, 0})) {
            return 1 + i;
        }
        njGetMatrix(&after);
        if (_stack_size_ != depth ||
            _nj_current_matrix_ptr_ != &stack[depth - 1]) {
            return 4;
        }
        for (int j = 0; j < 16; j++) {
            if (before[j] != after[j]) {
                return 4;
            }
        }
    }
    njPopMatrix(1);
    return 0;
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
    if (polygon_collect_near(&ball) != 1 || !full->near) {
        return 4;
    }
    empty->near = full->near;
    empty->near_count = 7;
    if (polygon_collect_near(&ball) != 1 || empty->near || empty->near_count ||
        full->near_count != 1 || !full->near) {
        return 4;
    }
    collision_mesh_free(empty);
    collision_mesh_free(full);
    if (polygon_alloc(0) != poly) {
        return 5;
    }
    int failure = mesh_strips();
    if (failure) {
        return 1200 + failure;
    }
    failure = model_heights();
    if (failure) {
        return 1220 + failure;
    }
    failure = fan_geometry();
    if (failure) {
        return 1600 + failure;
    }
    failure = fan_contacts();
    if (failure) {
        return 800 + failure;
    }
    failure = owned_tooth();
    if (failure) {
        return 650 + failure;
    }
    failure = terrain_crest_case(install_player_terrain,
                                 &character_constants[CHARACTER_KNUCKLES]);
    if (failure) {
        return 1000 + failure;
    }
    failure = coast_water_stride();
    if (failure) {
        return 1400 + failure;
    }
    failure = terrain_tread_cases(install_player_terrain,
                                  &character_constants[CHARACTER_SONIC]);
    if (failure) {
        return 1700 + failure;
    }
    failure = terrain_curb_case(install_player_terrain,
                                &character_constants[CHARACTER_SONIC]);
    if (failure) {
        return 1100 + failure;
    }
    failure = terrain_probes();
    if (failure) {
        return 100 + failure;
    }
    failure = bucket_floors();
    if (failure) {
        return 90 + failure;
    }
    if (swept_reach()) {
        return 89;
    }
    failure = full_near_pool();
    if (failure) {
        return 80 + failure;
    }
    failure = dense_meshes(FALSE);
    if (failure) {
        return 70 + failure;
    }
    failure = dense_meshes(TRUE);
    if (failure) {
        return 75 + failure;
    }
    failure = crowded_world();
    if (failure) {
        return 60 + failure;
    }
    failure = cross_floor();
    if (failure) {
        return 5 + failure;
    }
    for (int rotated = 0; rotated <= 1; rotated++) {
        failure = inside_corner(rotated);
        if (failure) {
            return 30 + 10 * rotated + failure;
        }
    }
    failure = airborne_walls();
    if (failure) {
        return 120 + failure;
    }
    failure = crowded_walls();
    if (failure) {
        return 620 + failure;
    }
    failure = tilted_passage();
    if (failure) {
        return 730 + failure;
    }
    failure = airborne_rims();
    if (failure) {
        return 150 + failure;
    }
    if (!material_strides()) {
        return 663;
    }
    failure = ground_strides();
    if (failure) {
        return 170 + failure;
    }
    failure = terrain_dash_case(install_player_terrain,
                                &character_constants[CHARACTER_SONIC]);
    if (failure) {
        return 1740 + failure;
    }
    failure = terrain_roll_case(install_player_terrain, character_constants);
    if (failure) {
        return 1240 + failure;
    }
    failure = roster_terrain();
    if (failure) {
        return 400 + failure;
    }
    for (int layout = 0; layout < 8; layout++) {
        if (!run_loop(layout & 1, layout & 2, layout & 4)) {
            return 500 + layout;
        }
    }
    for (int layout = 0; layout < 16; layout++) {
        if (!vertical_lip(layout & 3, layout & 4, layout & 8)) {
            return 508;
        }
    }
    if (!inherited_wall()) {
        return 509;
    }
    failure = ground_departures();
    if (failure) {
        return 180 + failure;
    }
    if (!ground_requires_join()) {
        return 187;
    }
    if (!ground_shallow_bend()) {
        return 188;
    }
    failure = mountain_crest();
    if (failure) {
        return 540 + failure;
    }
    failure = shallow_crest_edges();
    if (failure) {
        return 550 + failure;
    }
    failure = partial_crest_strides(-1.86F);
    if (failure) {
        return 580 + failure;
    }
    failure = partial_crest_strides(-1.998F);
    if (failure) {
        return 600 + failure;
    }
    for (int wall = 0; wall < 2; wall++) {
        if (!many_face_bend(12, wall) || !many_face_bend(32, wall) ||
            !many_face_bend(48, wall)) {
            return 610 + wall;
        }
    }
    if (!ground_banking()) {
        return 189;
    }
    failure = initial_overlaps();
    if (failure) {
        return 470 + failure;
    }
    for (int layout = 0; layout < 8; layout++) {
        if (!overlap_bank(layout)) {
            return 479;
        }
    }
    failure = uphill_wall();
    if (failure) {
        return 740 + failure;
    }
    if (!ground_wall_strides()) {
        return 460;
    }
    if (!ground_wall_clearance()) {
        return 461;
    }
    for (int layout = 0; layout < 3; layout++) {
        static const struct {
            Float roof;
            enum curb_approach approach;
        } cases[] = {
            {0, CURB_WALK},  {12.5F, CURB_WALK},   {12.5F, CURB_DESCEND},
            {11, CURB_WALK}, {11, CURB_GLANCE},    {11, CURB_STAND},
            {12, CURB_WALK}, {11, CURB_BACK_FACE}, {11, CURB_OVERHANG}};
        for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
            failure = curb_clearance(layout, cases[i].roof, cases[i].approach);
            if (failure) {
                return 630 + 10 * i + failure;
            }
        }
    }
    failure = airborne_landings();
    if (failure) {
        return 200 + failure;
    }
    failure = open_floor_rims();
    if (failure) {
        return 480 + failure;
    }
    failure = knuckles_floor_rim();
    if (failure) {
        return 500 + failure;
    }
    failure = bank_landings();
    if (failure) {
        return 220 + failure;
    }
    failure = gamma_skates();
    if (failure) {
        return 1540 + failure;
    }
    failure = gamma_landings();
    if (failure) {
        return 450 + failure;
    }
    failure = ground_many_tiles();
    if (failure) {
        return 190 + failure;
    }
    failure = crowded_carry();
    if (failure) {
        return 900 + failure;
    }
    failure = carry_surfaces();
    if (failure) {
        return 250 + failure;
    }
    if (!redirected_carry()) {
        return 280;
    }
    if (!carry_pool_exhaustion()) {
        return 270;
    }
    failure = platform_rides();
    if (failure) {
        return 260 + failure;
    }
    failure = rotating_floor();
    if (failure) {
        return 350 + failure;
    }
    failure = platform_jumps();
    if (failure) {
        return 360 + failure;
    }
    failure = jump_reaches_wall();
    if (failure) {
        return 420 + failure;
    }
    failure = jump_to_climb();
    if (failure) {
        return 410 + failure;
    }
    failure = traffic_riders(car_carry);
    if (failure) {
        return 430 + failure;
    }
    failure = traffic_riders(traffic_car_carry);
    if (failure) {
        return 440 + failure;
    }
    failure = inside_lift_riders();
    if (failure) {
        return 450 + failure;
    }
    failure = climbing_walls();
    if (failure) {
        return 300 + failure;
    }
    failure = climb_corners();
    if (failure) {
        return 310 + failure;
    }
    failure = climb_fan();
    if (failure) {
        return 320 + failure;
    }
    failure = climb_many_constraints();
    if (failure) {
        return 675 + failure;
    }
    failure = climb_rims();
    if (failure) {
        return 325 + failure;
    }
    failure = climb_seams();
    if (failure) {
        return 330 + failure;
    }
    failure = climb_thin_faces();
    if (failure) {
        return 340 + failure;
    }
    failure = box_headings();
    if (failure) {
        return 370 + failure;
    }
    if (!hammer_appears()) {
        return 388;
    }
    if (!slide_over_head()) {
        return 389;
    }
    failure = swinging_head();
    if (failure) {
        return 390 + failure;
    }
    failure = full_contacts();
    return failure ? 20 + failure : 0;
}

__attribute__((section(".text.boot"))) void start(void)
{
    result = check() + 1;
    for (;;) {
        __asm__ volatile("nop");
    }
}
