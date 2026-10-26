/* O ROTYU car parked over the way into the sewers, which Big lifts and carries
 * aside */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/parked_car.h"
#include "adv00/texlists.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "player/target.h"
#include "progress/progress.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT parked_car_object;
extern NJS_OBJECT parked_car_solid_object;

/* the car model's material for its lights, and the texture of sscar_texlist
 * the draw gives it */
#define CAR_LIGHTS 2
#define CAR_LIGHTS_TEXTURE 6

/* the world x and z Big runs to once he lifts the car */
#define CAR_AWAY_X 367.0F
#define CAR_AWAY_Z 962.0F
/* the height at which the lowered car comes to rest */
#define CAR_DOWN (-2.5F)

/* the solid copy lies SOLID_SHIFT along world x from the car's position,
 * whichever way the car faces */
#define SOLID_SHIFT 13.0F

enum parked_car_mode { CAR_PARKED, CAR_LIFTED, CAR_SET_DOWN };

struct parked_car_vars {
    /* the car's solid copy in the world, which players stand on, or NULL
     * once Big lifts the car */
    NJS_OBJECT *solid;
};

#define VARS(work) ((struct parked_car_vars *)&(work)->vars)

static void parked_car_step(struct task *task);
static void parked_car_draw(struct task *task);
static void parked_car_destroy(struct task *task);

/* the cylinder by which the car is lifted */
/* clang-format off */
static struct collision_shape parked_car_shape = {
    SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL),
    {0.0F, 6.0F, 0.0F}, 20.0F, 6.0F, 0.0F, {0}, 0, NULL,
};
/* clang-format on */

/* TRUE only for Sonic, once FLAG_SONIC_SS_ENTRANCE_SEWER is set */
static Bool parked_car_gone(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_SS_ENTRANCE_SEWER];
    case CHARACTER_TAILS:
    case CHARACTER_KNUCKLES:
    case CHARACTER_AMY:
    case CHARACTER_E102:
    case CHARACTER_BIG:
        return FALSE;
    }
    return FALSE;
}

void parked_car_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct task_force *force = task->force;
    NJS_OBJECT *solid;

    if (!placement_editing(task) && parked_car_gone()) {
        placement_kill(task);
        return;
    }
    work->ang.x = 0;
    work->ang.z = 0;
    work->mode = CAR_PARKED;
    motion->speed.x = 0.0F;
    motion->speed.y = 0.0F;
    motion->speed.z = 0.0F;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    if (!placement_editing(task)) {
        VARS(work)->solid = solid = world_object_alloc();
        solid->evalflags = NJD_EVAL_BREAK;
        solid->scl[0] = 1.0F;
        solid->scl[1] = 1.0F;
        solid->scl[2] = 1.0F;
        solid->ang[0] = 0;
        solid->ang[1] = work->ang.y;
        solid->ang[2] = 0;
        solid->pos[0] = work->pos.x + SOLID_SHIFT;
        solid->pos[1] = work->pos.y;
        solid->pos[2] = work->pos.z;
        solid->model = parked_car_solid_object.model;
        solid->child = NULL;
        solid->sibling = NULL;
        world_add(WORLD_SMALL | WORLD_SOLID, task, solid);
        collision_init(task, &parked_car_shape, COLLISION_OBJECT, 1, 1);
    }
    carry_set_kind(work, motion, CARRY_KIND_LARGE);
    task->step = parked_car_step;
    task->draw = parked_car_draw;
    task->destructor = parked_car_destroy;
}

/* once Big holds the car, its solid copy leaves the world. he runs to
 * CAR_AWAY_X and CAR_AWAY_Z while the pads are disabled. once the car is down
 * to CAR_DOWN, the pads are enabled again and he lets go of it */
static void parked_car_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case CAR_PARKED:
            if ((work->flags & CARRY_HELD) &&
                player_get_character(0) == CHARACTER_BIG) {
                work->mode = CAR_LIFTED;
                player_event_run(0, CAR_AWAY_X, CAR_AWAY_Z);
                if (pad_is_enabled()) {
                    pad_disable();
                }
                if (VARS(work)->solid) {
                    world_remove(task, VARS(work)->solid);
                    world_object_free(VARS(work)->solid);
                    VARS(work)->solid = NULL;
                }
            }
            collision_register(work);
            break;
        case CAR_LIFTED:
            if (work->pos.y <= CAR_DOWN) {
                work->pos.y = CAR_DOWN;
                if (!pad_is_enabled()) {
                    pad_enable();
                }
                player_drop_held(0);
                work->mode = CAR_SET_DOWN;
            }
            break;
        case CAR_SET_DOWN:
            break;
        }
    }
    parked_car_draw(task);
}

static void parked_car_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&sscar_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    parked_car_object.model->mats[CAR_LIGHTS].attr_texId = CAR_LIGHTS_TEXTURE;
    draw_tree(&parked_car_object);
    njPopMatrix(1);
}

static void parked_car_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->solid) {
        world_remove(task, VARS(work)->solid);
        world_object_free(VARS(work)->solid);
    }
}
