/* the dig place the object list names O DIG PLACE2: a mound Knuckles digs a
 * kill switch out of */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/dig_place_2.h"
#include "adv02/kill_switch.h"
#include "adv02/texlists.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "player/knuckles.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT dig_place_2_ground_object;
extern NJS_OBJECT dig_place_2_spot_object;
/* the action the place's draw loops. its object's child is
 * dig_place_2_spot_object */
/* @hack type: declared an array, not the one NJS_ACTION it is, so SHC pools
 * its address at 0x0c905294 and reads motion through it, MOV.L @(4,R4),R3 at
 * 0x0c9051e8; declared as one, the code runs 32 bytes longer, and 21 of
 * dig_place_2_update's 124 words differ */
extern NJS_ACTION dig_place_2_action[];

/* placement_try_leave_by ends the task once the camera and player 0 are
 * both beyond 500 */
#define RANGE (500.0F * 500.0F)
/* the collision world tests the place only while a player is this near */
#define NEAR 500.0F
#define DRAW_RANGE 500.0F /* drawn only while a player is this near */
#define RADIUS 20.0F /* the collision record's radius */

#define GROUND_FLAGS WORLD_MEDIUM
#define SPOT_FLAGS (WORLD_MEDIUM | WORLD_DIGGABLE)

#define FRAME_SPEED 0.3F /* the motion frames the place advances each step */

enum dig_place_mode { DIG_PLACE_INIT, DIG_PLACE_RUN };

struct dig_place_2_vars {
    /* the ground's and the spot's copies in the collision world */
    NJS_OBJECT *ground;
    NJS_OBJECT *spot;
    Float frame;
};

#define VARS(work) ((struct dig_place_2_vars *)&(work)->vars)

static void dig_place_2_init(struct task *task);
static void dig_place_2_add_to_world(struct task *task);
static void dig_place_2_update(struct task *task);
static void dig_place_2_draw(struct task *task);
static void dig_place_2_destroy(struct task *task);

static struct collision_shape dig_place_2_shape[1] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 8.0F, 0.0F},
     3.0F,
     10.0F,
     0.0F,
     {0},
     0,
     NULL},
};

static void dig_place_2_reset(struct task *task)
{
    struct task_work *work = task->work;

    dig_place_2_add_to_world(task);
    work->submode = 0;
}

/* while the set editor holds the place, each step rebuilds its world objects
 * at its position and angle, so they follow the edit */
void dig_place_2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case DIG_PLACE_INIT:
            dig_place_2_init(task);
            break;
        case DIG_PLACE_RUN:
            dig_place_2_draw(task);
            dig_place_2_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case DIG_PLACE_INIT:
            dig_place_2_init(task);
            break;
        case DIG_PLACE_RUN:
            dig_place_2_destroy(task);
            dig_place_2_reset(task);
            dig_place_2_draw(task);
            dig_place_2_update(task);
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            break;
        }
    }
}

static void dig_place_2_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = dig_place_2_draw;
    task->destructor = dig_place_2_destroy;
    VARS(work)->frame = 0.0F;
    collision_init(task, dig_place_2_shape, COLLISION_OBJECT, 1, 1);
    work->collision->radius = RADIUS;
    dig_place_2_reset(task);
    work->mode = DIG_PLACE_RUN;
}

static void dig_place_2_add_to_world(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *object;
    NJS_OBJECT *from;

    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    from = &dig_place_2_ground_object;
    VARS(work)->ground = object = world_object_alloc();
    object->evalflags = from->evalflags;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = from->pos[0] + work->pos.x;
    object->pos[1] = from->pos[1] + work->pos.y;
    object->pos[2] = from->pos[2] + work->pos.z;
    object->model = from->model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(GROUND_FLAGS, task, object);
    from = &dig_place_2_spot_object;
    VARS(work)->spot = object = world_object_alloc();
    object->evalflags = from->evalflags;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = from->pos[0] + work->pos.x;
    object->pos[1] = from->pos[1] + work->pos.y;
    object->pos[2] = from->pos[2] + work->pos.z;
    object->model = from->model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(SPOT_FLAGS, task, object);
}

/* while no kill switch lives, raise one when Knuckles touches the place and
 * player 0's action is KNUCKLES_DIGGING_OUT. the toucher and player 0 can be
 * different players */
static void dig_place_2_update(struct task *task)
{
    struct task_work *work = task->work;
    struct task *button;
    struct task_work *button_work;
    struct task_motion *motion;

    if (kill_switch_work == NULL && (work->collision->flags & COLLISION_HIT) &&
        player_task_character(work->collision->hit->owner) ==
            CHARACTER_KNUCKLES &&
        knuckles_get_activity(0) == KNUCKLES_DIGGING_OUT) {
        button = task_create(TASK_MOTION | TASK_WORK, 3, kill_switch_step);
        button_work = button->work;
        motion = button->motion;
        button_work->pos.x = work->pos.x;
        button_work->pos.y = work->pos.y + 11.0F;
        button_work->pos.z = work->pos.z;
        motion->speed.x = 0.0F;
        motion->speed.y = 0.5F;
        motion->speed.z = 0.0F;
        sound_play(SE_NOTICE, 0, 0, 0);
    }
    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    if (kill_switch_work == NULL) {
        collision_register(work);
    }
    VARS(work)->frame += FRAME_SPEED;
    if (VARS(work)->frame >= dig_place_2_action->motion->nbFrame - 1.0F) {
        VARS(work)->frame -= dig_place_2_action->motion->nbFrame - 1.0F;
    }
}

static void dig_place_2_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (player_in_range(&work->pos, DRAW_RANGE) == 0) {
        return;
    }
    njSetTexture(&mrobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(dig_place_2_action, VARS(work)->frame);
    njPopMatrix(1);
}

static void dig_place_2_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->ground);
    world_object_free(VARS(work)->ground);
    world_remove(task, VARS(work)->spot);
    world_object_free(VARS(work)->spot);
}
