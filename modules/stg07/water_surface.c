/* the water's surface: O Suimen, a pool with three floor switches. it rises
 * or sinks to LEVEL_BELOW under the switch player 0 last pressed */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg07/ruin01_textures.h"
#include "stg07/water_surface.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_OBJECT water_surface_object;
extern NJS_OBJECT water_plane_object;
extern NJS_OBJECT water_switch_object;
extern NJS_OBJECT water_switch_button; /* a child of water_switch_object */

struct water_vars {
    NJS_OBJECT *plane; /* its copy of water_plane_object in the world */
};

#define VARS(work) ((struct water_vars *)&(work)->vars)

/* in the scale of the water's work. BUTTON_HEIGHT is the height of the
 * switches' button. WATER_TARGET is the height the water moves toward, and
 * WATER_ASKED the height a switch asks for */
#define BUTTON_HEIGHT(work) ((work)->scl.x)
#define WATER_TARGET(work) ((work)->scl.y)
#define WATER_ASKED(work) ((work)->scl.z)

/* in the water's work flags: a switch was pressed this step */
#define WATER_PRESSED 0x200

struct water_switch_extra {
    /* 1.0F while player 0 presses the switch, and 0.0F otherwise. a press
     * plays its sound only on its first step */
    Float pressed;
};

/* the world tests the plane only while player 0 or 1 is within WATER_REACH of
 * the pool's middle */
#define WATER_REACH 500.0F
/* player 0 presses a switch within SWITCH_REACH of the switch's middle,
 * measured flat */
#define SWITCH_REACH 11.0F

/* a switch asks for the water LEVEL_BELOW under it */
#define LEVEL_BELOW 5.0F

/* the button is at BUTTON_DOWN while a switch is pressed. off it, the button
 * rises BUTTON_RISE each step. past BUTTON_UP, it rests at BUTTON_REST */
#define BUTTON_UP 2.8F
#define BUTTON_DOWN (-2.0F)
#define BUTTON_RISE 0.1F
#define BUTTON_REST 2.9F

/* the water moves WATER_SPEED each step. it starts at WATER_START, at x
 * WATER_X and z WATER_Z, whatever its set entry says */
#define WATER_SPEED 1.0F
#define WATER_START (-2679.0F)
#define WATER_X 5599.5F
#define WATER_Z 1355.0F

/* the plane is PLANE_SCALE times its model's size across */
#define PLANE_SCALE 100.0F

#define SE_PRESS 200

Float ruin_water_level = -2796.0F;

/* @hack type: the shape as an array of one, passed as
 * &water_switch_shapes[0], whose address SHC keeps in R11 for the three
 * calls (0x0c90a102). as one shape passed by its address, 131 of
 * water_surface_step's 136 words differ */
/* clang-format off */
static struct collision_shape water_switch_shapes[] = {
    {SHAPE_CYLINDER, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 4.0F, 0.0F},
     15.0F, 3.5F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static void water_surface_destroy(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *plane = VARS(work)->plane;

    world_object_free(plane);
    world_remove(task, plane);
    /* @hack redundant: a return where the function returns anyway, which keeps
     * task at @R15 by ADD #-4,R15 and MOV.L R4,@R15 (0x0c909d64,
     * 0x0c909d66). without it, SHC pushes task with MOV.L R4,@-R15, and 13
     * of water_surface_destroy's 16 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

/* a switch's step, which draws it too. the switch is pressed while
 * something touches it and player 0 is within SWITCH_REACH */
static void water_switch_step(struct task *task)
{
    struct task_work *work;
    struct task_work *water;
    struct water_switch_extra *extra;
    NJS_POINT3 middle;
    NJS_POINT3 player;
    NJS_POINT3 *pos;

    work = task->work;
    water = task->parent->work;
    extra = task->extra;
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    draw_tree(&water_switch_object);
    njPopMatrix(1);
    pos = &player_work[0]->pos;
    middle.x = work->pos.x;
    middle.z = work->pos.z;
    player.x = pos->x;
    player.z = pos->z;
    middle.y = player.y = 0.0F;
    if ((work->collision->flags & COLLISION_HIT) &&
        njDistanceP2P(&middle, &player) < SWITCH_REACH) {
        if (extra->pressed == 0.0F) {
            sound_play(SE_PRESS, 0, 0, 0);
            extra->pressed = 1.0F;
        }
        water->flags |= WATER_PRESSED;
        WATER_ASKED(water) = work->pos.y - LEVEL_BELOW;
    } else {
        extra->pressed = 0.0F;
    }
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
}

/* the water's step and draw. the three switches share one button model, so
 * a press pushes every button down. the water moves toward the height asked
 * for. its plane in the world follows the camera across */
static void water_surface_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *plane = VARS(work)->plane;
    NJS_POINT3 middle = {5597.0F, -2639.0F, 1348.0F};
    NJS_OBJECT *object = &water_surface_object;

    if (placement_try_leave(task)) {
        return;
    }
    if (player_in_range(&middle, WATER_REACH)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    njSetTexture(&ruin01_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    draw_model(object->model);
    njPopMatrix(1);
    work->flags &= ~WATER_PRESSED;
    task_step_children(task);
    /* @bug logic: the else's test of BUTTON_UP reads as meant to pass over
     * a button at rest. but the button rests at BUTTON_REST, so the test
     * always holds (FCMP/EQ FR4,FR3 at 0x0c909fa0, BUTTON_REST stored at
     * 0x0c909fba). a resting button rises each step and goes back to
     * BUTTON_REST */
    if (work->flags & WATER_PRESSED) {
        BUTTON_HEIGHT(work) = BUTTON_DOWN;
        WATER_TARGET(work) = ruin_water_level = WATER_ASKED(work);
    } else if (BUTTON_HEIGHT(work) != BUTTON_UP) {
        BUTTON_HEIGHT(work) += BUTTON_RISE;
        if (BUTTON_HEIGHT(work) > BUTTON_UP) {
            BUTTON_HEIGHT(work) = BUTTON_REST;
        }
    }
    water_switch_button.pos[1] = BUTTON_HEIGHT(work);
    if (WATER_TARGET(work) != work->pos.y) {
        if (WATER_TARGET(work) > work->pos.y) {
            work->pos.y += WATER_SPEED;
            if (WATER_TARGET(work) < work->pos.y) {
                work->pos.y = WATER_TARGET(work);
            }
        } else {
            work->pos.y -= WATER_SPEED;
            if (WATER_TARGET(work) > work->pos.y) {
                work->pos.y = WATER_TARGET(work);
            }
        }
    }
    plane->pos[1] = work->pos.y;
    plane->pos[0] = camera_work->pos.x;
    plane->pos[2] = camera_work->pos.z;
}

/* put the water and its plane in place, and create the three switches */
void water_surface_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *plane;
    struct task *child;
    struct task_work *child_work;
    struct water_switch_extra *extra;

    work->pos.x = WATER_X;
    WATER_TARGET(work) = ruin_water_level = WATER_START;
    work->pos.y = WATER_TARGET(work);
    work->pos.z = WATER_Z;
    plane = VARS(work)->plane =
        object_add_world(&water_plane_object, task, WORLD_MOVES | WORLD_WATER);
    plane->scl[0] = PLANE_SCALE;
    plane->scl[1] = 1.0F;
    plane->scl[2] = PLANE_SCALE;

    child = task_create_child(TASK_MOTION | TASK_WORK | TASK_FORCE | TASK_EXTRA,
                              water_switch_step, task);
    child_work = child->work;
    extra = child->extra;
    child_work->pos.x = 5343.0F;
    child_work->pos.y = -2610.0F;
    child_work->pos.z = 1052.0F;
    extra->pressed = 0.0F;
    collision_init(child, &water_switch_shapes[0], COLLISION_OBJECT, 1, 1);

    child = task_create_child(TASK_MOTION | TASK_WORK | TASK_FORCE | TASK_EXTRA,
                              water_switch_step, task);
    child_work = child->work;
    extra = child->extra;
    child_work->pos.x = 5220.0F;
    child_work->pos.y = -2545.0F;
    child_work->pos.z = 1477.0F;
    extra->pressed = 0.0F;
    collision_init(child, &water_switch_shapes[0], COLLISION_OBJECT, 1, 1);

    child = task_create_child(TASK_MOTION | TASK_WORK | TASK_FORCE | TASK_EXTRA,
                              water_switch_step, task);
    child_work = child->work;
    extra = child->extra;
    child_work->pos.x = 5308.0F;
    child_work->pos.y = -2479.0F;
    child_work->pos.z = 1084.0F;
    extra->pressed = 0.0F;
    collision_init(child, &water_switch_shapes[0], COLLISION_OBJECT, 1, 1);

    task->step = water_surface_run;
    task->draw = water_surface_run;
    task->destructor = water_surface_destroy;
}
