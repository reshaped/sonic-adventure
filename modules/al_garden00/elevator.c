/* O AR_ELE, the elevator from the garden back to Station Square */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_garden00/door.h"
#include "al_garden00/elevator.h"
#include "al_garden00/textures.h"
#include "al_main/card.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "save/card.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* the frame. its child and that child's sibling are the two doors, and each
 * door's child is its outer leaf */
extern NJS_OBJECT elevator_object;

/* each door's slide from shut, from 0 to DOOR_OPEN, kept in the work's scl.
 * its outer leaf slides twice as far */
#define DOOR(work) ((work)->scl.x)
#define DOOR_OPEN 5.0F
#define DOOR_SPEED 0.1F /* the doors' slide a step */
/* half a door's width. shut, each door's shape lies this far out from the
 * middle */
#define DOOR_HALF 5.0F

#define DOOR_WAIT 60.0F /* steps the doors stay open, as work->timer counts */

/* the doors open once player 0 is within DOOR_RADIUS across and DOOR_HEIGHT
 * above or below the elevator's origin. once they shut, player 0 leaves if
 * it is within the same bounds of a point DOOR_RADIUS into the elevator
 * from its origin */
#define DOOR_RADIUS 10.0F
#define DOOR_HEIGHT 20.0F

/* where the elevator leads in Station Square: the hotel's act, at this
 * entrance */
#define STATION_ACT 4
#define STATION_ENTRANCE 4

#define FLOOR_FLAGS (WORLD_SMALL | WORLD_SOLID)

/* the elevator's work->mode */
enum elevator_mode {
    ELEVATOR_SHUT,
    ELEVATOR_OPENING,
    ELEVATOR_OPEN,
    ELEVATOR_CLOSING,
    ELEVATOR_SHUT_TEST, /* just shut: test whether player 0 is inside */
    ELEVATOR_LEAVE, /* shut on player 0: save the garden */
    ELEVATOR_GONE /* fade to Station Square once the save ends */
};

struct floor_vars {
    /* the floor's object in the collision world, which floor_destroy frees */
    NJS_OBJECT *object;
};

#define FLOOR(work) ((struct floor_vars *)&(work)->vars)

static void elevator_step(struct task *task);
static void elevator_draw(struct task *task);
static void elevator_destroy(struct task *task);
static void doors_move(struct task_work *work);
static void floor_init(struct task *task);
static void floor_step(struct task *task);
static void floor_destroy(struct task *task);

/* the doors, shut: a box for each, which doors_move slides with its outer
 * leaf */
static struct collision_shape elevator_shapes[2] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {DOOR_HALF, 12.5F, 0.0F},
     DOOR_HALF,
     12.5F,
     0.5F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-DOOR_HALF, 12.5F, 0.0F},
     DOOR_HALF,
     12.5F,
     0.5F,
     {0},
     0,
     NULL},
};

/* in the set editor, the elevator gets no door shapes and no floor */
void elevator_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    DOOR(work) = 0.0F;
    MODEL(work)->object = &elevator_object;
    if (!placement_editing(task)) {
        collision_init(task, elevator_shapes, COLLISION_OBJECT, 1, 2);
        task_create_child(TASK_WORK | TASK_FORCE, floor_init, task);
    }
    task->step = elevator_step;
    task->draw = elevator_draw;
    task->destructor = elevator_destroy;
}

static void elevator_step(struct task *task)
{
    struct task_work *work = task->work;
    Float door;
    NJS_POINT3 inside;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case ELEVATOR_SHUT:
        if (player_in_cylinder(&work->pos, DOOR_RADIUS, DOOR_HEIGHT) == 1) {
            work->mode = ELEVATOR_OPENING;
        }
        break;
    case ELEVATOR_OPENING:
        door = DOOR(work);
        door += DOOR_SPEED;
        if (door > DOOR_OPEN) {
            door = DOOR_OPEN;
            work->timer = 0;
            work->mode = ELEVATOR_OPEN;
        }
        DOOR(work) = door;
        if (!placement_editing(task)) {
            doors_move(work);
        }
        break;
    case ELEVATOR_OPEN:
        work->timer++;
        if (TASK_TIMER(work) >= DOOR_WAIT) {
            work->timer = 0;
            work->mode = ELEVATOR_CLOSING;
        }
        break;
    case ELEVATOR_CLOSING:
        if ((work->flags & COLLISION_TOUCHING) &&
            work->collision->hit->kind == COLLISION_PLAYER) {
            work->mode = ELEVATOR_OPENING;
        } else {
            door = DOOR(work);
            door -= DOOR_SPEED;
            if (door < 0.0F) {
                door = 0.0F;
                work->timer = 0;
                work->mode = ELEVATOR_SHUT_TEST;
            }
            DOOR(work) = door;
        }
        if (!placement_editing(task)) {
            doors_move(work);
        }
        break;
    case ELEVATOR_SHUT_TEST:
        ang = work->ang.y;
        inside.x = -(njSin(ang) * DOOR_RADIUS);
        inside.y = 0.0F;
        inside.z = -(njCos(ang) * DOOR_RADIUS);
        inside.x += work->pos.x;
        inside.y += work->pos.y;
        inside.z += work->pos.z;
        if (player_in_cylinder(&inside, DOOR_RADIUS, DOOR_HEIGHT) == 1) {
            work->mode = ELEVATOR_LEAVE;
        } else {
            work->mode = ELEVATOR_SHUT;
        }
        break;
    case ELEVATOR_LEAVE:
        garden_flags_save();
        alife_save();
        work->mode = ELEVATOR_GONE;
        break;
    case ELEVATOR_GONE:
        if (!card_saving) {
            stage_set_entrance(STATION_ENTRANCE);
            stage_fade_to(STAGE_STATION_SQUARE, STATION_ACT);
        }
        break;
    }
    if (!placement_editing(task)) {
        collision_register(work);
        task_step_children(task);
    }
    elevator_draw(task);
}

/* draw the frame, then each door and its outer leaf, the leaf slid twice as
 * far. the second door is turned half a turn, so it slides the other way */
static void elevator_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    NJS_OBJECT *door;
    NJS_OBJECT *leaf;
    Float slide;
    Angle ang;

    njSetTexture(&garden_objects_texlist);
    slide = -DOOR(work);
    object = MODEL(work)->object;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(object->model);
    door = object->child;
    njPushMatrix(NULL);
    njTranslate(NULL, slide, 0.0F, 0.0F);
    draw_model(door->model);
    leaf = door->child;
    njTranslate(NULL, slide, 0.0F, 0.0F);
    draw_model(leaf->model);
    njPopMatrix(1);
    door = door->sibling;
    njPushMatrix(NULL);
    njRotateY(NULL, 0x8000);
    njTranslate(NULL, slide, 0.0F, 0.0F);
    draw_model(door->model);
    leaf = door->child;
    njTranslate(NULL, slide, 0.0F, 0.0F);
    draw_model(leaf->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void elevator_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void doors_move(struct task_work *work)
{
    Float slide = DOOR(work);
    struct collision_shape *shapes = work->collision->shapes;

    slide *= 2.0F;
    shapes[0].center.x = DOOR_HALF + slide;
    shapes[1].center.x = -DOOR_HALF - slide;
}

/* the floor task: the frame's model as a solid world object where the
 * elevator stands, so that a player can stand in the elevator */
static void floor_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *object;

    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    object = world_object_alloc();
    FLOOR(work)->object = object;
    object->evalflags = NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK | NJD_EVAL_ZXY_ANG;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->model = MODEL(task->parent->work)->object->model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(FLOOR_FLAGS, task, object);
    work->flags |= WORLD_TESTED;
    task->step = floor_step;
    task->destructor = floor_destroy;
}

static void floor_step(struct task *task)
{
}

static void floor_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (FLOOR(work)->object != NULL) {
        world_remove(task, FLOOR(work)->object);
        world_object_free(FLOOR(work)->object);
    }
}
