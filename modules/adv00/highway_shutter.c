/* O HWBSHUTTER shutter across the way to the highway, which rolls up for Sonic
 * and breaks open for Tails */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/highway_shutter.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT highway_shutter_object;
extern NJS_OBJECT highway_shutter_broken_object;

#define SHUTTER_TOP 50.0F /* the height at which the rolling shutter stops */
#define SHUTTER_SPEED 0.2F /* the rolling shutter's rise a step */
#define SE_SHUTTER_ROLL 0x217
#define SE_SHUTTER_STOP 0x216

#define WALL_WORLD_FLAGS                                                       \
    (WORLD_SMALL | WORLD_MOVES | WORLD_UNCLIMBABLE | WORLD_SOLID)

/* modes of Sonic's shutter as it rolls up */
enum highway_shutter_mode { SHUTTER_SHUT, SHUTTER_ROLLING, SHUTTER_OPEN };

struct highway_shutter_vars {
    NJS_OBJECT *wall; /* the wall's object in the world */
};

#define VARS(work) ((struct highway_shutter_vars *)&(work)->vars)

/* collision shapes that no code reads */
/* clang-format off */
static struct collision_shape highway_shutter_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 28.0F, -0.5F}, 1.0F, 22.0F, 55.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 30.0F, 52.5F}, 8.0F, 30.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 30.0F, -53.5F}, 8.0F, 30.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 28.0F, -31.0F}, 1.0F, 22.0F, 17.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 28.0F, 31.0F}, 3.0F, 22.0F, 17.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 40.0F, 0.0F}, 3.0F, 10.0F, 14.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 30.0F, 51.5F}, 8.0F, 30.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 30.0F, -52.5F}, 8.0F, 30.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 28.0F, -0.5F}, 1.0F, 22.0F, 55.0F, {0}, 0, NULL},
};
/* clang-format on */

/* the wall models for the world, whole and broken open */
static NJS_MATERIAL highway_shutter_wall_materials[1] = {
#include "adv00/highway_shutter/highway_shutter_wall_materials.inc"
};

static Sint16 highway_shutter_wall_meshes[7] = {
#include "adv00/highway_shutter/highway_shutter_wall_meshes.inc"
};

static NJS_MESHSET highway_shutter_wall_meshsets[1] = {
#include "adv00/highway_shutter/highway_shutter_wall_meshsets.inc"
};

static NJS_POINT3 highway_shutter_wall_points[6] = {
#include "adv00/highway_shutter/highway_shutter_wall_points.inc"
};

static NJS_VECTOR highway_shutter_wall_normals[6] = {
#include "adv00/highway_shutter/highway_shutter_wall_normals.inc"
};

static NJS_MODEL highway_shutter_wall_model = {
#include "adv00/highway_shutter/highway_shutter_wall_model.inc"
};

static NJS_OBJECT highway_shutter_wall[1] = {
#include "adv00/highway_shutter/highway_shutter_wall.inc"
};

static NJS_MATERIAL highway_shutter_broken_wall_materials[1] = {
#include "adv00/highway_shutter/highway_shutter_broken_wall_materials.inc"
};

static Sint16 highway_shutter_broken_wall_meshes[28] = {
#include "adv00/highway_shutter/highway_shutter_broken_wall_meshes.inc"
};

static NJS_MESHSET highway_shutter_broken_wall_meshsets[1] = {
#include "adv00/highway_shutter/highway_shutter_broken_wall_meshsets.inc"
};

static NJS_POINT3 highway_shutter_broken_wall_points[16] = {
#include "adv00/highway_shutter/highway_shutter_broken_wall_points.inc"
};

static NJS_VECTOR highway_shutter_broken_wall_normals[16] = {
#include "adv00/highway_shutter/highway_shutter_broken_wall_normals.inc"
};

static NJS_MODEL highway_shutter_broken_wall_model = {
#include "adv00/highway_shutter/highway_shutter_broken_wall_model.inc"
};

static NJS_OBJECT highway_shutter_broken_wall[1] = {
#include "adv00/highway_shutter/highway_shutter_broken_wall.inc"
};

/* TRUE for Sonic or Tails once his story opens the way to the highway */
static Bool highway_shutter_open(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_SS_ENTRANCE_HIGHWAY];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_SS_ENTRANCE_HIGHWAY];
    case CHARACTER_KNUCKLES:
    case CHARACTER_AMY:
    case CHARACTER_E102:
    case CHARACTER_BIG:
    default:
        return FALSE;
    }
}

static void highway_shutter_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&highway_shutter_object);
    njPopMatrix(1);
}

static void highway_shutter_broken_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&highway_shutter_broken_object);
    njPopMatrix(1);
}

static void highway_shutter_broken_step(struct task *task)
{
    if (placement_editing(task) || !placement_try_leave(task)) {
        highway_shutter_broken_draw(task);
    }
}

/* once Tails' way is open, switch his shutter and its wall to the broken
 * models */
static void highway_shutter_tails_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_editing(task)) {
        highway_shutter_broken_draw(task);
    } else if (!placement_try_leave(task)) {
        if (highway_shutter_open()) {
            task->step = highway_shutter_broken_step;
            task->draw = highway_shutter_broken_draw;
            VARS(work)->wall->model = highway_shutter_broken_wall[0].model;
            highway_shutter_broken_draw(task);
        } else {
            highway_shutter_draw(task);
        }
    }
}

/* roll Sonic's shutter up once his way is open. at SHUTTER_TOP, move its wall
 * up to it and have the task destroy itself on its next step. the task's end
 * takes the wall out of the world */
static void highway_shutter_sonic_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        if (placement_try_leave(task)) {
            return;
        }
        switch (work->mode) {
        case SHUTTER_SHUT:
            if (event_flags[FLAG_SONIC_SS_ENTRANCE_HIGHWAY]) {
                work->mode = SHUTTER_ROLLING;
            }
            break;
        case SHUTTER_ROLLING:
            work->pos.y += SHUTTER_SPEED;
            if (work->pos.y >= SHUTTER_TOP) {
                VARS(work)->wall->pos[1] = work->pos.y;
                work->mode = SHUTTER_OPEN;
                placement_kill(task);
                sound_play(SE_SHUTTER_STOP, 0, 0, 0);
            } else {
                sound_play_for(SE_SHUTTER_ROLL, (int)task, 1, 0, 2);
            }
            break;
        case SHUTTER_OPEN:
            break;
        }
    }
    highway_shutter_draw(task);
}

static void highway_shutter_step(struct task *task)
{
    if (placement_editing(task) || !placement_try_leave(task)) {
        highway_shutter_draw(task);
    }
}

static void highway_shutter_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->wall) {
        world_remove(task, VARS(work)->wall);
        world_object_free(VARS(work)->wall);
    }
}

/* in the set editor, only draw the shutter. otherwise start it for player 0's
 * character: when the way is already open, Sonic's is gone and Tails' broken.
 * every shutter left adds its wall to the world */
void highway_shutter_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_MODEL *model;
    NJS_OBJECT *wall;

    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        task->step = highway_shutter_step;
        task->draw = highway_shutter_draw;
        return;
    }
    model = highway_shutter_wall[0].model;
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        if (highway_shutter_open()) {
            placement_kill(task);
            return;
        } else {
            work->mode = SHUTTER_SHUT;
            task->step = highway_shutter_sonic_step;
            task->draw = highway_shutter_draw;
        }
        break;
    case CHARACTER_TAILS:
        if (highway_shutter_open()) {
            task->step = highway_shutter_broken_step;
            task->draw = highway_shutter_broken_draw;
            model = highway_shutter_broken_wall[0].model;
        } else {
            task->step = highway_shutter_tails_step;
            task->draw = highway_shutter_draw;
        }
        break;
    default:
        task->step = highway_shutter_step;
        task->draw = highway_shutter_draw;
    }
    VARS(work)->wall = wall = world_object_alloc();
    wall->evalflags = NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK | NJD_EVAL_ZXY_ANG;
    wall->scl[0] = 1.0F;
    wall->scl[1] = 1.0F;
    wall->scl[2] = 1.0F;
    wall->ang[0] = 0;
    wall->ang[1] = work->ang.y;
    wall->ang[2] = 0;
    wall->pos[0] = work->pos.x;
    wall->pos[1] = work->pos.y;
    wall->pos[2] = work->pos.z;
    wall->model = model;
    wall->child = NULL;
    wall->sibling = NULL;
    world_add(WALL_WORLD_FLAGS, task, wall);
    work->flags |= WORLD_TESTED;
    task->destructor = highway_shutter_destroy;
}
