/* O TRAIN train to the Mystic Ruins, which leaves once player 0 boards */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/station_square.h"
#include "adv00/texlists.h"
#include "adv00/train.h"
#include "adv02/mystic_ruins.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* @hack type: the doors' action declared as an array, so SHC pools its
 * address and reads motion through it (MOV.L @(4,R3),R2 at 0x0c9049f6);
 * declared as one, 17 of train_car_collide's 206 words and 5 of
 * station_square_train_init's 147 differ */
extern NJS_ACTION train_action[1];
extern NJS_OBJECT train_floor_object;

/* the cars, numbered from CAR_FIRST. train_car_place puts each on the
 * train's x axis, and the train leaves toward -x, led by car CAR_FIRST */
#define CARS 3
#define CAR_FIRST (-1)
#define CAR_LENGTH 135.0F

#define FLOOR_WORLD_FLAGS (WORLD_MEDIUM | WORLD_SOLID)
/* a player within a car's floor's model's radius plus this has the world
 * test the floor */
#define WORLD_REACH 20.0F

/* a door panel slides out by the doors' frame over their last frame, times
 * DOOR_SLIDE and then DOOR_SLIDE_SCALE: 12.6 at the last frame */
#define DOOR_SLIDE 7.0F
#define DOOR_SLIDE_SCALE 1.8F

#define SE_DOORS 523 /* the doors shutting */

enum train_mode {
    TRAIN_OPEN = 0, /* waiting with its doors open */
    TRAIN_CLOSING = 1, /* shutting its doors on player 0 aboard */
    TRAIN_REOPENING = 2, /* after a touch, or once player 0 leaves */
    TRAIN_LEAVING = 3, /* pulling out, and fading to the Mystic Ruins */
    TRAIN_ARRIVING = 4,
    TRAIN_OPENING = 5,
    TRAIN_EMPTYING = 6, /* open until no car sets a flag in a step */
    TRAIN_SHUT = 8 /* until the story opens the line */
};

/* flags the cars set in the train's work->submode, which the train clears
 * each step */
#define TRAIN_ABOARD 0x1 /* player 0 is in a car */
#define TRAIN_DOOR_TOUCHED 0x2 /* a player touched a car */

/* the train's vars and a car's: a car keeps its floor, and the train its
 * place and frame */
struct train_vars {
    NJS_OBJECT *floor; /* the car's floor in the collision world */
    Float place; /* the train's move along its x axis, in car lengths */
    Float frame; /* the doors' frame of train_action, 0 shut */
};

#define VARS(work) ((struct train_vars *)&(work)->vars)
#define CAR(work) ((Sint8)(work)->tick) /* a car's number, from CAR_FIRST */

static void train_car_step(struct task *task);
static void train_car_destroy(struct task *task);

/* a car's door shapes: the pair sliding apart in each of its two doorways,
 * each shut doorway, and the jambs */
/* clang-format off */
static struct collision_shape train_car_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {35.75F, 20.5F, 14.01F}, 4.25F, 9.0F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {44.25F, 20.5F, 14.01F}, 4.25F, 9.0F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {-35.75F, 20.5F, 14.01F}, 4.25F, 9.0F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {-44.25F, 20.5F, 14.01F}, 4.25F, 9.0F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {40.0F, 20.5F, 14.01F}, 9.0F, 9.0F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {-40.0F, 20.5F, 14.01F}, 9.0F, 9.0F, 1.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {51.0F, 20.5F, 14.01F}, 3.0F, 9.0F, 1.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {28.0F, 20.5F, 14.01F}, 3.0F, 9.0F, 1.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {-51.0F, 20.5F, 14.01F}, 3.0F, 9.0F, 1.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {-28.0F, 20.5F, 14.01F}, 3.0F, 9.0F, 1.0F, {0}, 0, NULL},
};
/* clang-format on */

/* TRUE once the story of player 0's character has opened the line */
static Bool train_open(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_TRAIN];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_TRAIN];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_TRAIN];
    case CHARACTER_AMY:
        return event_flags[FLAG_AMY_TRAIN];
    case CHARACTER_E102:
        return event_flags[FLAG_E102_TRAIN];
    case CHARACTER_BIG:
        return event_flags[FLAG_BIG_TRAIN];
    }
    return FALSE;
}

/* TRUE when a player touches the car */
static Bool train_door_touched(struct task_work *work)
{
    return (work->flags & COLLISION_TOUCHING) &&
           work->collision->hit->kind == COLLISION_PLAYER;
}

/* TRUE while player 0 is inside the car */
static Bool train_aboard(struct task_work *work)
{
    Float y = player_work[0]->pos.y - work->pos.y;
    Float x = player_work[0]->pos.x - work->pos.x;
    Float z = player_work[0]->pos.z - work->pos.z;
    Float cos = njCos(-work->ang.y);
    Float sin = njSin(-work->ang.y);
    Float along = (x * cos) + (z * sin);
    Float across = (z * cos) - (x * sin);

    if (along > -48.0F && along < 48.0F && y > 0.0F && y < 20.0F &&
        across < 10.0F && across > -10.0F) {
        return TRUE;
    }
    return FALSE;
}

static void train_car_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *train = task->parent->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 65.0F)) {
        njSetTexture(&ss_train_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(train_action, VARS(train)->frame);
        njPopMatrix(1);
    }
}

/* put the floor where the car stands, and let the world test it while a
 * player is near. a shut train blocks each doorway whole. otherwise each
 * panel slides out with the doors' frame, as DOOR_SLIDE says */
static void train_car_collide(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *train = task->parent->work;
    NJS_OBJECT *floor;
    Float open;
    struct collision_shape *shapes;

    if (player_in_range(&work->pos,
                        VARS(work)->floor->model->r + WORLD_REACH)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    floor = VARS(work)->floor;
    floor->pos[0] = work->pos.x;
    floor->pos[1] = work->pos.y;
    floor->pos[2] = work->pos.z;
    floor->ang[0] = work->ang.x;
    floor->ang[1] = work->ang.y;
    floor->ang[2] = work->ang.z;
    open =
        VARS(train)->frame * DOOR_SLIDE / (train_action->motion->nbFrame - 1);
    shapes = work->collision->shapes;
    if (train->mode == TRAIN_SHUT) {
        shapes[0].flags |= SHAPE_OFF;
        shapes[1].flags |= SHAPE_OFF;
        shapes[2].flags |= SHAPE_OFF;
        shapes[3].flags |= SHAPE_OFF;
        shapes[4].flags &= ~SHAPE_OFF;
        shapes[5].flags &= ~SHAPE_OFF;
        shapes[6].flags &= ~SHAPE_OFF;
        shapes[7].flags &= ~SHAPE_OFF;
        shapes[8].flags &= ~SHAPE_OFF;
        shapes[9].flags &= ~SHAPE_OFF;
    } else {
        shapes[0].flags &= ~SHAPE_OFF;
        shapes[1].flags &= ~SHAPE_OFF;
        shapes[2].flags &= ~SHAPE_OFF;
        shapes[3].flags &= ~SHAPE_OFF;
        shapes[4].flags |= SHAPE_OFF;
        shapes[5].flags |= SHAPE_OFF;
        shapes[6].flags |= SHAPE_OFF;
        shapes[7].flags |= SHAPE_OFF;
        shapes[8].flags |= SHAPE_OFF;
        shapes[9].flags |= SHAPE_OFF;
        open *= DOOR_SLIDE_SCALE;
        shapes[0].center.x = 35.75F - open;
        shapes[1].center.x = 44.25F + open;
        shapes[2].center.x = -35.75F + open;
        shapes[3].center.x = -44.25F - open;
    }
    collision_register(work);
}

/* put the car CAR(work) + place car lengths from the train's position,
 * along the train's x axis, turned as the train is. the force's speed takes
 * the move, which carries what stands on the car */
static void train_car_place(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *train = task->parent->work;
    struct task_force *force;
    Angle ang;
    Float along = CAR(work) + VARS(train)->place;
    Float x;
    Float y;
    Float z;

    force = task->force;
    ang = train->ang.y;
    along *= CAR_LENGTH;
    x = train->pos.x;
    y = train->pos.y;
    z = train->pos.z;
    x += njCos(ang) * along;
    z -= njSin(ang) * along;
    force->speed.x = x - work->pos.x;
    force->speed.y = y - work->pos.y;
    force->speed.z = z - work->pos.z;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    work->ang = train->ang;
}

static void train_car_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *train = task->parent->work;

    train_car_draw(task);
    train_car_place(task);
    if (train_aboard(work)) {
        /* @hack spelling: the train's work read again (MOV.L @(8,R14),R4 at
         * 0x0c904c18), while the first read waits at sp+0 for the touch's
         * flag; read through train instead, train takes R12 and 33 of 36
         * words differ */
        struct task_work *again = task->parent->work;

        again->submode |= TRAIN_ABOARD;
    }
    if (train_door_touched(work)) {
        train->submode |= TRAIN_DOOR_TOUCHED;
    }
    train_car_collide(task);
}

static void train_car_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->floor);
    world_object_free(VARS(work)->floor);
}

static void train_car_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *floor;

    VARS(work)->floor = floor = world_object_alloc();
    floor->evalflags = NJD_EVAL_BREAK;
    floor->scl[0] = 1.0F;
    floor->scl[1] = 1.0F;
    floor->scl[2] = 1.0F;
    floor->ang[0] = work->ang.x;
    floor->ang[1] = work->ang.y;
    floor->ang[2] = work->ang.z;
    floor->pos[0] = work->pos.x;
    floor->pos[1] = work->pos.y;
    floor->pos[2] = work->pos.z;
    floor->model = train_floor_object.model;
    floor->child = train_floor_object.child;
    floor->sibling = train_floor_object.sibling;
    world_add(FLOOR_WORLD_FLAGS, task, floor);
    task->destructor = train_car_destroy;
    collision_init(task, train_car_shapes, COLLISION_OBJECT, 1, 10);
    task->step = train_car_step;
    task->draw = train_car_draw;
}

static void train_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void train_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case TRAIN_OPEN:
        if (!train_open()) {
            work->mode = TRAIN_SHUT;
            VARS(work)->frame = 0.0F;
        } else if (work->submode == TRAIN_ABOARD) {
            work->timer = 15;
            work->mode = TRAIN_CLOSING;
            sound_play(SE_DOORS, 0, 0, 0);
        }
        break;
    case TRAIN_CLOSING:
        if ((work->submode & TRAIN_DOOR_TOUCHED) ||
            !(work->submode & TRAIN_ABOARD)) {
            work->mode = TRAIN_REOPENING;
        } else if (TASK_TIMER(work) > 0) {
            work->timer--;
        } else if (VARS(work)->frame > 2.0F) {
            VARS(work)->frame -= 0.2F;
        } else if ((VARS(work)->frame -= 0.1F) <= 0.0F) {
            player_suspend(0);
            work->scl.x = 0.0F;
            work->scl.z = 0.0F;
            VARS(work)->frame = 0.0F;
            work->timer = 0;
            work->mode = TRAIN_LEAVING;
        }
        break;
    case TRAIN_REOPENING: {
        Float last = train_action->motion->nbFrame - 1;

        if ((VARS(work)->frame += 0.5F) >= last) {
            VARS(work)->frame = last;
            work->mode = TRAIN_OPEN;
        }
    } break;
    case TRAIN_LEAVING:
        if (TASK_TIMER(work) < 288) {
            work->timer++;
        }
        if (TASK_TIMER(work) == 130) {
            stage_set_entrance(0);
            stage_fade_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION);
        }
        if (TASK_TIMER(work) >= 32) {
            VARS(work)->place -= (TASK_TIMER(work) - 32) * (0.01F / 256.0F);
            if (VARS(work)->place < -15.0F) {
                work->timer = 80;
                work->mode = TRAIN_ARRIVING;
            }
        }
        break;
    case TRAIN_ARRIVING:
        if (--TASK_TIMER(work) == 0) {
            player_resume(0);
            work->mode = TRAIN_OPENING;
        }
        if (TASK_TIMER(work) >= 16) {
            int left = TASK_TIMER(work) - 16;

            VARS(work)->place = -(left * (left - 1) * (0.05F / 256.0F));
        }
        break;
    case TRAIN_OPENING: {
        Float last = train_action->motion->nbFrame - 1;

        if ((VARS(work)->frame += 0.5F) >= last) {
            VARS(work)->frame = last;
            work->mode = TRAIN_EMPTYING;
        }
    } break;
    case TRAIN_EMPTYING:
        if (work->submode == 0) {
            work->mode = TRAIN_OPEN;
        }
        break;
    case TRAIN_SHUT:
        if (train_open()) {
            work->mode = TRAIN_OPENING;
            VARS(work)->frame = 0.0F;
        }
        break;
    }
    work->submode = 0;
    task_step_children(task);
}

void station_square_train_init(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage_act = stage_get_stage_act();

    if ((Uint8)stage_act != STATION_SQUARE_CASINO) {
        njPrintC(NJM_LOCATION(10, 8), "ACT CASINO ONLY !");
        return;
    }
    if (placement_editing(task)) {
        Angle ang;

        njSetTexture(&ss_train_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(train_action, 0.0F);
        njPopMatrix(1);
    } else {
        int car;

        for (car = CAR_FIRST; car < CAR_FIRST + CARS; car++) {
            struct task *child =
                task_create_child(TASK_WORK | TASK_FORCE, train_car_init, task);
            struct task_work *child_work = child->work;

            child_work->tick = car;
            train_car_place(child);
        }
        VARS(work)->place = 0.0F;
        if (train_open()) {
            work->mode = TRAIN_OPEN;
            VARS(work)->frame = train_action->motion->nbFrame - 1;
        } else {
            work->mode = TRAIN_SHUT;
            VARS(work)->frame = 0.0F;
        }
        task->step = train_step;
        task->destructor = train_destroy;
    }
}
