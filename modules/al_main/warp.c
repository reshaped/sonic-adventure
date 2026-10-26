/* the gardens' warps, WARP SS, WARP EC, WARP MR and WARP EGG. a garden's
 * warp is a pad of three tiers under the emblem of the garden it leads to.
 * the egg warp is an egg, which leads out of the gardens to the Egg
 * Carrier */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_main/alife.h"
#include "al_main/card.h"
#include "al_main/models.h"
#include "al_main/warp.h"
#include "chao/chao.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "player/player.h"
#include "progress/progress.h"
#include "save/card.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

static NJS_MATERIAL warp_pad_top_materials[1] = {
#include "al_main/warp/warp_pad_top_materials.inc"
};

static Sint16 warp_pad_top_meshes0[17] = {
#include "al_main/warp/warp_pad_top_meshes0.inc"
};

static NJS_COLOR warp_pad_top_uvs0[14] = {
#include "al_main/warp/warp_pad_top_uvs0.inc"
};

static NJS_MESHSET warp_pad_top_meshsets[1] = {
#include "al_main/warp/warp_pad_top_meshsets.inc"
};

static NJS_POINT3 warp_pad_top_points[10] = {
#include "al_main/warp/warp_pad_top_points.inc"
};

static NJS_VECTOR warp_pad_top_normals[10] = {
#include "al_main/warp/warp_pad_top_normals.inc"
};

static NJS_MODEL warp_pad_top_model = {
#include "al_main/warp/warp_pad_top_model.inc"
};

static NJS_OBJECT warp_pad_top_object = {
#include "al_main/warp/warp_pad_top_object.inc"
};

static NJS_MATERIAL warp_pad_materials[9] = {
#include "al_main/warp/warp_pad_materials.inc"
};

static Sint16 warp_pad_meshes0[11] = {
#include "al_main/warp/warp_pad_meshes0.inc"
};

static Sint16 warp_pad_meshes1[45] = {
#include "al_main/warp/warp_pad_meshes1.inc"
};

static Sint16 warp_pad_meshes2[23] = {
#include "al_main/warp/warp_pad_meshes2.inc"
};

static Sint16 warp_pad_meshes3[28] = {
#include "al_main/warp/warp_pad_meshes3.inc"
};

static Sint16 warp_pad_meshes4[38] = {
#include "al_main/warp/warp_pad_meshes4.inc"
};

static Sint16 warp_pad_meshes5[7] = {
#include "al_main/warp/warp_pad_meshes5.inc"
};

static Sint16 warp_pad_meshes6[12] = {
#include "al_main/warp/warp_pad_meshes6.inc"
};

static Sint16 warp_pad_meshes7[6] = {
#include "al_main/warp/warp_pad_meshes7.inc"
};

static Sint16 warp_pad_meshes8[34] = {
#include "al_main/warp/warp_pad_meshes8.inc"
};

static NJS_COLOR warp_pad_uvs0[10] = {
#include "al_main/warp/warp_pad_uvs0.inc"
};

static NJS_COLOR warp_pad_uvs1[40] = {
#include "al_main/warp/warp_pad_uvs1.inc"
};

static NJS_COLOR warp_pad_uvs2[22] = {
#include "al_main/warp/warp_pad_uvs2.inc"
};

static NJS_COLOR warp_pad_uvs3[24] = {
#include "al_main/warp/warp_pad_uvs3.inc"
};

static NJS_COLOR warp_pad_uvs4[32] = {
#include "al_main/warp/warp_pad_uvs4.inc"
};

static NJS_COLOR warp_pad_uvs5[6] = {
#include "al_main/warp/warp_pad_uvs5.inc"
};

static NJS_COLOR warp_pad_uvs6[10] = {
#include "al_main/warp/warp_pad_uvs6.inc"
};

static NJS_COLOR warp_pad_uvs7[5] = {
#include "al_main/warp/warp_pad_uvs7.inc"
};

static NJS_COLOR warp_pad_uvs8[28] = {
#include "al_main/warp/warp_pad_uvs8.inc"
};

static NJS_MESHSET warp_pad_meshsets[9] = {
#include "al_main/warp/warp_pad_meshsets.inc"
};

static NJS_POINT3 warp_pad_points[85] = {
#include "al_main/warp/warp_pad_points.inc"
};

static NJS_VECTOR warp_pad_normals[85] = {
#include "al_main/warp/warp_pad_normals.inc"
};

static NJS_MODEL warp_pad_model = {
#include "al_main/warp/warp_pad_model.inc"
};

static NJS_OBJECT warp_pad_object = {
#include "al_main/warp/warp_pad_object.inc"
};

static NJS_MATERIAL warp_ring_materials[1] = {
#include "al_main/warp/warp_ring_materials.inc"
};

static Sint16 warp_ring_meshes0[23] = {
#include "al_main/warp/warp_ring_meshes0.inc"
};

static NJS_COLOR warp_ring_uvs0[22] = {
#include "al_main/warp/warp_ring_uvs0.inc"
};

static NJS_MESHSET warp_ring_meshsets[1] = {
#include "al_main/warp/warp_ring_meshsets.inc"
};

static NJS_POINT3 warp_ring_points[20] = {
#include "al_main/warp/warp_ring_points.inc"
};

static NJS_VECTOR warp_ring_normals[20] = {
#include "al_main/warp/warp_ring_normals.inc"
};

static NJS_MODEL warp_ring_model = {
#include "al_main/warp/warp_ring_model.inc"
};

/* nothing names this object. each ring draws warp_ring_model alone */
static NJS_OBJECT warp_ring_object = {
#include "al_main/warp/warp_ring_object.inc"
};

static NJS_MATERIAL warp_small_ring_materials[1] = {
#include "al_main/warp/warp_small_ring_materials.inc"
};

static Sint16 warp_small_ring_meshes0[23] = {
#include "al_main/warp/warp_small_ring_meshes0.inc"
};

static NJS_COLOR warp_small_ring_uvs0[22] = {
#include "al_main/warp/warp_small_ring_uvs0.inc"
};

static NJS_MESHSET warp_small_ring_meshsets[1] = {
#include "al_main/warp/warp_small_ring_meshsets.inc"
};

static NJS_POINT3 warp_small_ring_points[20] = {
#include "al_main/warp/warp_small_ring_points.inc"
};

static NJS_VECTOR warp_small_ring_normals[20] = {
#include "al_main/warp/warp_small_ring_normals.inc"
};

static NJS_MODEL warp_small_ring_model = {
#include "al_main/warp/warp_small_ring_model.inc"
};

/* nothing names this object, so nothing draws the small ring */
static NJS_OBJECT warp_small_ring_object = {
#include "al_main/warp/warp_small_ring_object.inc"
};

static NJS_MATERIAL warp_station_square_materials[1] = {
#include "al_main/warp/warp_station_square_materials.inc"
};

static Sint16 warp_station_square_meshes0[5] = {
#include "al_main/warp/warp_station_square_meshes0.inc"
};

static NJS_COLOR warp_station_square_uvs0[4] = {
#include "al_main/warp/warp_station_square_uvs0.inc"
};

static NJS_MESHSET warp_station_square_meshsets[1] = {
#include "al_main/warp/warp_station_square_meshsets.inc"
};

static NJS_POINT3 warp_station_square_points[4] = {
#include "al_main/warp/warp_station_square_points.inc"
};

static NJS_VECTOR warp_station_square_normals[4] = {
#include "al_main/warp/warp_station_square_normals.inc"
};

static NJS_MODEL warp_station_square_model = {
#include "al_main/warp/warp_station_square_model.inc"
};

static NJS_OBJECT warp_station_square_object = {
#include "al_main/warp/warp_station_square_object.inc"
};

static NJS_MATERIAL warp_station_square_glow_materials[1] = {
#include "al_main/warp/warp_station_square_glow_materials.inc"
};

static Sint16 warp_station_square_glow_meshes0[15] = {
#include "al_main/warp/warp_station_square_glow_meshes0.inc"
};

static NJS_COLOR warp_station_square_glow_uvs0[12] = {
#include "al_main/warp/warp_station_square_glow_uvs0.inc"
};

static NJS_MESHSET warp_station_square_glow_meshsets[1] = {
#include "al_main/warp/warp_station_square_glow_meshsets.inc"
};

static NJS_POINT3 warp_station_square_glow_points[6] = {
#include "al_main/warp/warp_station_square_glow_points.inc"
};

static NJS_VECTOR warp_station_square_glow_normals[6] = {
#include "al_main/warp/warp_station_square_glow_normals.inc"
};

static NJS_MODEL warp_station_square_glow_model = {
#include "al_main/warp/warp_station_square_glow_model.inc"
};

static NJS_OBJECT warp_station_square_glow_object = {
#include "al_main/warp/warp_station_square_glow_object.inc"
};

static NJS_MATERIAL warp_egg_carrier_materials[1] = {
#include "al_main/warp/warp_egg_carrier_materials.inc"
};

static Sint16 warp_egg_carrier_meshes0[5] = {
#include "al_main/warp/warp_egg_carrier_meshes0.inc"
};

static NJS_COLOR warp_egg_carrier_uvs0[4] = {
#include "al_main/warp/warp_egg_carrier_uvs0.inc"
};

static NJS_MESHSET warp_egg_carrier_meshsets[1] = {
#include "al_main/warp/warp_egg_carrier_meshsets.inc"
};

static NJS_POINT3 warp_egg_carrier_points[4] = {
#include "al_main/warp/warp_egg_carrier_points.inc"
};

static NJS_VECTOR warp_egg_carrier_normals[4] = {
#include "al_main/warp/warp_egg_carrier_normals.inc"
};

static NJS_MODEL warp_egg_carrier_model = {
#include "al_main/warp/warp_egg_carrier_model.inc"
};

static NJS_OBJECT warp_egg_carrier_object = {
#include "al_main/warp/warp_egg_carrier_object.inc"
};

static NJS_MATERIAL warp_egg_carrier_glow_materials[1] = {
#include "al_main/warp/warp_egg_carrier_glow_materials.inc"
};

static Sint16 warp_egg_carrier_glow_meshes0[15] = {
#include "al_main/warp/warp_egg_carrier_glow_meshes0.inc"
};

static NJS_COLOR warp_egg_carrier_glow_uvs0[12] = {
#include "al_main/warp/warp_egg_carrier_glow_uvs0.inc"
};

static NJS_MESHSET warp_egg_carrier_glow_meshsets[1] = {
#include "al_main/warp/warp_egg_carrier_glow_meshsets.inc"
};

static NJS_POINT3 warp_egg_carrier_glow_points[6] = {
#include "al_main/warp/warp_egg_carrier_glow_points.inc"
};

static NJS_VECTOR warp_egg_carrier_glow_normals[6] = {
#include "al_main/warp/warp_egg_carrier_glow_normals.inc"
};

static NJS_MODEL warp_egg_carrier_glow_model = {
#include "al_main/warp/warp_egg_carrier_glow_model.inc"
};

static NJS_OBJECT warp_egg_carrier_glow_object = {
#include "al_main/warp/warp_egg_carrier_glow_object.inc"
};

static NJS_MATERIAL warp_mystic_ruins_materials[1] = {
#include "al_main/warp/warp_mystic_ruins_materials.inc"
};

static Sint16 warp_mystic_ruins_meshes0[5] = {
#include "al_main/warp/warp_mystic_ruins_meshes0.inc"
};

static NJS_COLOR warp_mystic_ruins_uvs0[4] = {
#include "al_main/warp/warp_mystic_ruins_uvs0.inc"
};

static NJS_MESHSET warp_mystic_ruins_meshsets[1] = {
#include "al_main/warp/warp_mystic_ruins_meshsets.inc"
};

static NJS_POINT3 warp_mystic_ruins_points[4] = {
#include "al_main/warp/warp_mystic_ruins_points.inc"
};

static NJS_VECTOR warp_mystic_ruins_normals[4] = {
#include "al_main/warp/warp_mystic_ruins_normals.inc"
};

static NJS_MODEL warp_mystic_ruins_model = {
#include "al_main/warp/warp_mystic_ruins_model.inc"
};

static NJS_OBJECT warp_mystic_ruins_object = {
#include "al_main/warp/warp_mystic_ruins_object.inc"
};

static NJS_MATERIAL warp_mystic_ruins_glow_materials[1] = {
#include "al_main/warp/warp_mystic_ruins_glow_materials.inc"
};

static Sint16 warp_mystic_ruins_glow_meshes0[15] = {
#include "al_main/warp/warp_mystic_ruins_glow_meshes0.inc"
};

static NJS_COLOR warp_mystic_ruins_glow_uvs0[12] = {
#include "al_main/warp/warp_mystic_ruins_glow_uvs0.inc"
};

static NJS_MESHSET warp_mystic_ruins_glow_meshsets[1] = {
#include "al_main/warp/warp_mystic_ruins_glow_meshsets.inc"
};

static NJS_POINT3 warp_mystic_ruins_glow_points[6] = {
#include "al_main/warp/warp_mystic_ruins_glow_points.inc"
};

static NJS_VECTOR warp_mystic_ruins_glow_normals[6] = {
#include "al_main/warp/warp_mystic_ruins_glow_normals.inc"
};

static NJS_MODEL warp_mystic_ruins_glow_model = {
#include "al_main/warp/warp_mystic_ruins_glow_model.inc"
};

static NJS_OBJECT warp_mystic_ruins_glow_object = {
#include "al_main/warp/warp_mystic_ruins_glow_object.inc"
};

static NJS_MATERIAL warp_egg_materials[8] = {
#include "al_main/warp/warp_egg_materials.inc"
};

static Sint16 warp_egg_meshes0[48] = {
#include "al_main/warp/warp_egg_meshes0.inc"
};

static Sint16 warp_egg_meshes1[30] = {
#include "al_main/warp/warp_egg_meshes1.inc"
};

static Sint16 warp_egg_meshes2[40] = {
#include "al_main/warp/warp_egg_meshes2.inc"
};

static Sint16 warp_egg_meshes3[40] = {
#include "al_main/warp/warp_egg_meshes3.inc"
};

static Sint16 warp_egg_meshes4[32] = {
#include "al_main/warp/warp_egg_meshes4.inc"
};

static Sint16 warp_egg_meshes5[44] = {
#include "al_main/warp/warp_egg_meshes5.inc"
};

static Sint16 warp_egg_meshes6[6] = {
#include "al_main/warp/warp_egg_meshes6.inc"
};

static Sint16 warp_egg_meshes7[4] = {
#include "al_main/warp/warp_egg_meshes7.inc"
};

static Sint16 warp_egg_meshes8[9] = {
#include "al_main/warp/warp_egg_meshes8.inc"
};

static Sint16 warp_egg_meshes9[14] = {
#include "al_main/warp/warp_egg_meshes9.inc"
};

static NJS_COLOR warp_egg_uvs0[48] = {
#include "al_main/warp/warp_egg_uvs0.inc"
};

static NJS_COLOR warp_egg_uvs1[30] = {
#include "al_main/warp/warp_egg_uvs1.inc"
};

static NJS_COLOR warp_egg_uvs2[40] = {
#include "al_main/warp/warp_egg_uvs2.inc"
};

static NJS_COLOR warp_egg_uvs3[40] = {
#include "al_main/warp/warp_egg_uvs3.inc"
};

static NJS_COLOR warp_egg_uvs4[32] = {
#include "al_main/warp/warp_egg_uvs4.inc"
};

static NJS_COLOR warp_egg_uvs5[44] = {
#include "al_main/warp/warp_egg_uvs5.inc"
};

static NJS_COLOR warp_egg_uvs6[6] = {
#include "al_main/warp/warp_egg_uvs6.inc"
};

static NJS_COLOR warp_egg_uvs7[4] = {
#include "al_main/warp/warp_egg_uvs7.inc"
};

static NJS_COLOR warp_egg_uvs8[9] = {
#include "al_main/warp/warp_egg_uvs8.inc"
};

static NJS_COLOR warp_egg_uvs9[12] = {
#include "al_main/warp/warp_egg_uvs9.inc"
};

static NJS_MESHSET warp_egg_meshsets[10] = {
#include "al_main/warp/warp_egg_meshsets.inc"
};

static NJS_POINT3 warp_egg_points[85] = {
#include "al_main/warp/warp_egg_points.inc"
};

static NJS_VECTOR warp_egg_normals[85] = {
#include "al_main/warp/warp_egg_normals.inc"
};

static NJS_MODEL warp_egg_model = {
#include "al_main/warp/warp_egg_model.inc"
};

static NJS_OBJECT warp_egg_object = {
#include "al_main/warp/warp_egg_object.inc"
};

/* each character's event flags, in alife_character's order. a character's
 * flag for a garden marks the garden as one it has visited. no warp to a
 * garden opens to a character until its flag in warp_flags, below, is set */
static Sint32 visit_flags[CHAO_PLAYERS][GARDEN_COUNT] = {
    {183, 184, 185}, {238, 239, 240}, {295, 296, 297},
    {347, 348, 349}, {418, 419, 420}, {474, 475, 476},
};
static Sint32 warp_flags[CHAO_PLAYERS] = {
    FLAG_SONIC_UNK_82, FLAG_TAILS_UNK_83, FLAG_KNUCKLES_UNK_84,
    FLAG_AMY_UNK_85,   FLAG_E102_UNK_86,  FLAG_BIG_UNK_87,
};

struct warp_vars {
    Uint8 garden; /* the garden the warp leads to, or WARP_EGG */
    /* set to 60 as the rings start. each ring takes 10 from it. the fifth
     * ring takes it below 20, which ends the rings */
    Uint8 ring_countdown;
    Uint8 unk_02[2];
    NJS_OBJECT *emblem; /* the destination's emblem, null for the egg warp */
    NJS_OBJECT *glow; /* the emblem's glow, null for the egg warp */
};

#define VARS(work) ((struct warp_vars *)&(work)->vars)

/* the garden of the egg warp, which leads to no garden */
#define WARP_EGG 10

/* in a warp's work flags. WARP_OPEN marks a warp the player can take.
 * WARP_GLOW flips at each draw of an open garden warp */
#define WARP_OPEN 0x200
#define WARP_GLOW 0x100

/* a warp's modes. WARP_WAIT waits for the player to stand on the warp.
 * WARP_HOLD starts the gardens' save. WARP_RINGS sends up the rings.
 * WARP_GO sends the player on once card_saving clears */
enum warp_mode { WARP_WAIT, WARP_HOLD, WARP_RINGS, WARP_GO };

/* a garden warp's init does nothing until its timer passes START_STEPS. a
 * warp takes the player once its timer passes STAND_STEPS. the timer
 * restarts at each step the player isn't standing on the warp */
#define START_STEPS 10
#define STAND_STEPS 50

/* the act of STAGE_EGG_CARRIER_INSIDE that the egg warp leads to */
#define EGG_ACT 5

/* a ring's work. ang.y is its turn, and ang.x its progress through its
 * climb. scl.x, scl.y and scl.z are its width, height and glow. RING_BASE is
 * the y it climbs from */
#define RING_BASE(work) ((work)->vars.f[0])

/* a quarter and a half of a turn, in Angle's units */
#define QUARTER_TURN 0x4000
#define HALF_TURN 0x8000

static void warp_step(struct task *task);
static void warp_destroy(struct task *task);
static void warp_draw(struct task *task);
static void warp_ring_init(struct task *task);
static void warp_ring_step(struct task *task);
static void warp_ring_destroy(struct task *task);
static void warp_ring_draw(struct task *task);

/* the pad's three tiers, then a sphere over the top tier. warp_step takes
 * the player the sphere hits as standing on the warp */
static const struct collision_shape warp_shapes[4] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     14.7F,
     1.4F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     12.7F,
     2.8F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     8.5F,
     4.5F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 4.8F, 0.0F},
     3.7F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

Bool warp_mark_visit_check_open(int garden)
{
    int character = alife_character;

    /* @hack spelling: alife_character read again in place of character, so SHC
     * works out visit_flags' row again for the return (0x0c91a164) rather
     * than keep it on the stack; through character, 38 of 51 words differ */
    event_flag_set(visit_flags[alife_character][alife_garden]);
    if (!event_flag_check(warp_flags[character])) {
        return FALSE;
    }
    return event_flag_check(visit_flags[character][garden]);
}

void warp_station_square_init(struct task *task)
{
    struct task_work *work = task->work;

    if (TASK_TIMER(work)++ > START_STEPS) {
        if (!placement_editing(task)) {
            collision_init(task, warp_shapes, COLLISION_OBJECT, 1, 4);
        }
        chao_clear_carried();
        if (warp_mark_visit_check_open(GARDEN_STATION_SQUARE)) {
            work->flags |= WARP_OPEN;
        } else {
            work->flags &= ~WARP_OPEN;
        }
        VARS(work)->garden = GARDEN_STATION_SQUARE;
        VARS(work)->emblem = &warp_station_square_object;
        VARS(work)->glow = &warp_station_square_glow_object;
        task->step = warp_step;
        task->destructor = warp_destroy;
        task->draw = warp_draw;
        work->timer = 0;
    }
}

void warp_egg_carrier_init(struct task *task)
{
    struct task_work *work = task->work;

    if (TASK_TIMER(work)++ > START_STEPS) {
        if (!placement_editing(task)) {
            collision_init(task, warp_shapes, COLLISION_OBJECT, 1, 4);
        }
        chao_clear_carried();
        if (warp_mark_visit_check_open(GARDEN_EGG_CARRIER)) {
            work->flags |= WARP_OPEN;
        } else {
            work->flags &= ~WARP_OPEN;
        }
        VARS(work)->garden = GARDEN_EGG_CARRIER;
        VARS(work)->emblem = &warp_egg_carrier_object;
        VARS(work)->glow = &warp_egg_carrier_glow_object;
        task->step = warp_step;
        task->destructor = warp_destroy;
        task->draw = warp_draw;
        work->timer = 0;
    }
}

void warp_mystic_ruins_init(struct task *task)
{
    struct task_work *work = task->work;

    if (TASK_TIMER(work)++ > START_STEPS) {
        if (!placement_editing(task)) {
            collision_init(task, warp_shapes, COLLISION_OBJECT, 1, 4);
        }
        chao_clear_carried();
        if (warp_mark_visit_check_open(GARDEN_MYSTIC_RUINS)) {
            work->flags |= WARP_OPEN;
        } else {
            work->flags &= ~WARP_OPEN;
        }
        VARS(work)->garden = GARDEN_MYSTIC_RUINS;
        VARS(work)->emblem = &warp_mystic_ruins_object;
        VARS(work)->glow = &warp_mystic_ruins_glow_object;
        task->step = warp_step;
        task->destructor = warp_destroy;
        task->draw = warp_draw;
        work->timer = 0;
    }
}

void warp_egg_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        collision_init(task, warp_shapes, COLLISION_OBJECT, 1, 4);
    }
    chao_clear_carried();
    work->flags |= WARP_OPEN;
    VARS(work)->garden = WARP_EGG;
    VARS(work)->emblem = NULL;
    VARS(work)->glow = NULL;
    task->step = warp_step;
    task->destructor = warp_destroy;
    task->draw = warp_draw;
}

/* draw the warp unless it is out of view. ang.x and ang.z shake it, so it
 * trembles while warp_step turns them. a garden warp shows its emblem and
 * glow only while open. the draw that sets WARP_GLOW draws the glow with a
 * constant material alpha offset of -0.5, half faded. the draw that clears
 * it draws the glow whole */
static void warp_draw_at(struct task_work *work)
{
    Angle turn;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    njSetTexture(&alife_objects_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x + (njSin(work->ang.x) * 0.03F),
                work->pos.y + (njSin(work->ang.x + work->ang.z) * 0.05F),
                work->pos.z + (njSin(work->ang.z) * 0.08F));
    TURN_VIA(njRotateY, work->ang.y, turn);
    njScale(NULL, 0.7F, 0.7F, 0.7F);
    if (VARS(work)->garden == WARP_EGG) {
        draw_tree(&warp_egg_object);
    } else {
        if (work->flags & WARP_OPEN) {
            draw_tree(VARS(work)->emblem);
            if (work->flags & WARP_GLOW) {
                work->flags &= ~WARP_GLOW;
                draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
            } else {
                work->flags |= WARP_GLOW;
                draw_set_material(-0.5F, 0.0F, 0.0F, 0.0F);
            }
            draw_tree(VARS(work)->glow);
        }
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_tree(&warp_pad_object);
    }
    njPopMatrix(1);
}

static void warp_step(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *hit;
    struct task *ring;
    struct task_work *ring_work;

    if (edit_mode != 0) {
        warp_draw_at(work);
    } else {
        hit = work->collision->shapes[3].hit;
        if (!(work->flags & WARP_OPEN)) {
            hit = NULL;
        }
        switch (work->mode) {
        case WARP_WAIT:
            if (hit != NULL) {
                if (hit->kind == COLLISION_PLAYER) {
                    if (TASK_TIMER(work) > STAND_STEPS) {
                        work->timer = 0;
                        work->mode++;
                        VARS(work)->ring_countdown = 60;
                        pad_disable_player(0);
                        if (VARS(work)->garden != WARP_EGG) {
                            if (player_work[0]->flags & PLAYER_HOLDING) {
                                chao_set_carried_to(VARS(work)->garden);
                            } else {
                                chao_clear_carried();
                            }
                        }
                    }
                } else {
                    work->timer = 0;
                }
            } else {
                work->timer = 0;
            }
            break;
        case WARP_HOLD:
            if (TASK_TIMER(work) > 2) {
                alife_save();
                work->mode++;
            }
            break;
        case WARP_RINGS:
            if (TASK_TIMER(work) > 20) {
                ring = task_create(TASK_MOTION | TASK_WORK, 4, warp_ring_init);
                ring_work = ring->work;
                ring_work->pos = work->pos;
                RING_BASE(ring_work) = work->pos.y + 2.0F;
                work->timer = 0;
                VARS(work)->ring_countdown -= 10;
                if (VARS(work)->ring_countdown < 20) {
                    work->mode++;
                    work->timer = 0;
                }
            }
            work->ang.x += 0x2000;
            work->ang.z += 0x1200;
            break;
        case WARP_GO:
            if (!card_saving) {
                switch (VARS(work)->garden) {
                case GARDEN_STATION_SQUARE:
                    stage_set_entrance(1);
                    stage_fade_to(STAGE_ALIFE_SS, 0);
                    break;
                case GARDEN_EGG_CARRIER:
                    stage_set_entrance(1);
                    stage_fade_to(STAGE_ALIFE_EG, 0);
                    break;
                case GARDEN_MYSTIC_RUINS:
                    stage_set_entrance(1);
                    stage_fade_to(STAGE_ALIFE_MR, 0);
                    break;
                default:
                    stage_set_entrance(1);
                    stage_fade_to(STAGE_EGG_CARRIER_INSIDE, EGG_ACT);
                    break;
                }
            }
            break;
        }
        warp_draw_at(work);
        collision_register(work);
        work->tick++;
        work->timer++;
    }
}

static void warp_destroy(struct task *task)
{
}

static void warp_draw(struct task *task)
{
    struct task_work *work = task->work;

    warp_draw_at(work);
}

static void warp_ring_init(struct task *task)
{
    struct task_work *work = task->work;

    work->scl.x = 1.0F;
    work->scl.y = 0.0F;
    work->scl.z = 0.0F;
    work->ang.x = 0;
    work->ang.z = 0;
    task->step = warp_ring_step;
    task->destructor = warp_ring_destroy;
    task->draw = warp_ring_draw;
}

static void warp_ring_draw_at(struct task_work *work)
{
    Float width = work->scl.x * 0.7F;
    Float height = work->scl.y * 4.5F;
    Float glow = work->scl.z;
    Angle turn;

    njSetTexture(&alife_objects_texlist);
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    njPushMatrix(NULL);
    draw_set_material(glow, glow, glow, glow);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, turn);
    njScale(NULL, width, height, width);
    draw_model(&warp_ring_model);
    njPopMatrix(1);
    draw_attr_clear(0, NJD_FLAG_USE_ALPHA);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* each step a ring turns, narrows and fades, and ang.x moves it on through
 * its climb. its height is the sine of ang.x, so it grows tall and then flat
 * again over half a turn. meanwhile it climbs 18 above RING_BASE. the ring
 * goes once ang.x passes half a turn */
static void warp_ring_step(struct task *task)
{
    struct task_work *work = task->work;

    /* @hack spelling: a switch of one case, which SHC tests with CMP/EQ #0,R0
     * (0x0c91a96e); as an if, 7 of 80 words differ */
    switch (work->mode) {
    case 0:
        work->ang.y += 0x180;
        work->scl.x -= 0.002F;
        work->scl.y = njSin(work->ang.x);
        work->ang.x += 0x222;
        work->scl.z -= (1.0F / 55.0F);
        if (work->ang.x < QUARTER_TURN) {
            work->pos.y = RING_BASE(work) + (work->scl.y * 9.0F);
        } else {
            work->pos.y = RING_BASE(work) + ((2.0F - work->scl.y) * 9.0F);
        }
        warp_ring_draw_at(work);
        break;
    }
    if (work->ang.x > HALF_TURN) {
        task_kill(task);
    }
}

static void warp_ring_destroy(struct task *task)
{
}

static void warp_ring_draw(struct task *task)
{
    struct task_work *work = task->work;

    warp_ring_draw_at(work);
}
