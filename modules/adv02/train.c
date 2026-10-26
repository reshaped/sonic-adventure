/* O TRAIN three-car train at the station, which takes player 0 to Station
 * Square */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/station_square.h"
#include "adv02/texlists.h"
#include "adv02/train.h"
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

/* a car, its doors shut at the action's frame 0 and open at its last */
/* @hack type: the action declared as an array, and read through ->. SHC then
 * loads its address and reads its motion 4 bytes past it (MOV.L @(4,R3),R2 at
 * 0x0c90e076, and MOV.L @(4,R2),R3 at 0x0c90e76c). declared as one, 17 of
 * train_car_collide's 206 words and 5 of train_start's 135 differ */
extern NJS_ACTION mystic_ruins_train_action[];
/* a car as the collision world holds it */
extern NJS_OBJECT train_car_object;

/* sound of the doors shutting */
#define SE_DOORS 586

/* the distance between cars, and a car's radius for draw_in_view */
#define CAR_LENGTH 135.0F
#define CAR_RADIUS 65.0F

/* a car's flags in the collision world, which tests it only while a player is
 * within NEAR_MARGIN past its model's radius */
#define CAR_WORLD_FLAGS (WORLD_MEDIUM | WORLD_SOLID)
#define NEAR_MARGIN 20.0F

/* player 0 is in a car while in a box about the car's position. the box
 * reaches INSIDE_LENGTH either way along the car's x and INSIDE_WIDTH either
 * way along its z. it runs up from the position to INSIDE_HEIGHT */
#define INSIDE_LENGTH 48.0F
#define INSIDE_WIDTH 10.0F
#define INSIDE_HEIGHT 20.0F

/* a shut inner door's center lies DOOR_INNER_X from the car's position along
 * the car's x, either way, and a shut outer door's DOOR_OUTER_X. each doorway
 * has one of each */
#define DOOR_INNER_X 35.75F
#define DOOR_OUTER_X 44.25F
/* a door slides DOOR_SLIDE * DOOR_SLIDE_SCALE, 12.6 units, from shut to open.
 * the step multiplies the doors' share of the way open by each in turn */
#define DOOR_SLIDE 7.0F
#define DOOR_SLIDE_SCALE 1.8F

/* the doors start to shut SHUT_DELAY steps after player 0 steps into a car */
#define SHUT_DELAY 15
/* the doors shut by SHUT_SPEED action frames a step down to frame
 * SHUT_SLOW_FRAME, and by SHUT_SLOW_SPEED a step from there. they open by
 * OPEN_SPEED a step */
#define SHUT_SPEED 0.2F
#define SHUT_SLOW_FRAME 2.0F
#define SHUT_SLOW_SPEED 0.1F
#define OPEN_SPEED 0.5F

/* the trip's steps count up in work->timer from 0, to no more than
 * CRUISE_STEP. the train pulls out at step PULL_STEP. from then on its speed
 * grows each step by PULL_ACCEL, in car lengths a step, until CRUISE_STEP */
#define PULL_STEP 32
#define CRUISE_STEP 288
#define PULL_ACCEL (0.01F / 256.0F)
/* FADE_STEP steps into the trip, the game fades to entrance TO_ENTRANCE of
 * Station Square's casino act, where Station Square's own train stands */
#define FADE_STEP 130
#define TO_ENTRANCE 3
/* once AWAY_LENGTHS car lengths out, the train comes back over ARRIVE_STEPS
 * steps. while at least STAND_STEPS are left, its offset is -ARRIVE_EASE * n *
 * (n - 1) car lengths, n being the steps left past STAND_STEPS. it stands at
 * its place for the last STAND_STEPS */
#define AWAY_LENGTHS 15.0F
#define ARRIVE_STEPS 80
#define STAND_STEPS 16
#define ARRIVE_EASE (0.05F / 256.0F)

/* the train's modes, which start from 1. train_start sets the one the train
 * starts in */
enum train_mode {
    TRAIN_OPEN = 1, /* doors open: player 0 can step in */
    TRAIN_SHUT, /* doors shut while train_open is FALSE */
    TRAIN_CLOSING, /* player 0 is in a car, and the doors shut */
    TRAIN_REOPENING, /* the doors open again after player 0 steps out */
    TRAIN_LEAVING,
    TRAIN_ARRIVING,
    TRAIN_OPENING,
    /* doors open until a step on which no car sets TRAIN_PLAYER_IN or
     * TRAIN_TOUCHED */
    TRAIN_EMPTYING
};

/* the bits the cars set in the train's work->submode as they step. train_step
 * reads them on its next step, and clears them before the cars step again */
#define TRAIN_PLAYER_IN 0x1 /* player 0 stands in a car */
#define TRAIN_TOUCHED 0x2 /* a player touches a car */

/* the vars of the train and of its cars: object is a car's, and offset and
 * frame are the train's */
struct train_vars {
    NJS_OBJECT *object; /* the car's object in the collision world */
    /* how far the cars stand from the train's position along its x, in car
     * lengths. it is below 0 while the train is away */
    Float offset;
    Float frame; /* the doors' frame of mystic_ruins_train_action */
};

#define VARS(work) ((struct train_vars *)&(work)->vars)

/* a car's place in the train: -1, 0 or 1 car lengths from the train's offset,
 * along its x. work->tick holds it as a Uint8, so a read takes it back as a
 * Sint8 */
#define CAR_PLACE(work) ((work)->tick)

/* the doors' frame while they are open */
#define LAST_FRAME() ((Float)(mystic_ruins_train_action->motion->nbFrame - 1))

static Bool train_open(void);
static Bool train_car_touched(struct task_work *work);
static Bool train_car_holds(struct task_work *work);
static void train_car_draw(struct task *task);
static void train_car_collide(struct task *task);
static void train_car_place(struct task *task);
static void train_car_step(struct task *task);
static void train_car_destroy(struct task *task);
static void train_car_start(struct task *task);
static void train_destroy(struct task *task);
static void train_step(struct task *task);

/* a car's shapes, by index: 0-3 the doors, two to each doorway, 4-5 a block
 * sealing each doorway, and 6-9 the posts beside them */
static struct collision_shape train_car_shapes[10] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {DOOR_INNER_X, 20.5F, 14.01F},
     4.25F,
     9.0F,
     1.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {DOOR_OUTER_X, 20.5F, 14.01F},
     4.25F,
     9.0F,
     1.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-DOOR_INNER_X, 20.5F, 14.01F},
     4.25F,
     9.0F,
     1.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-DOOR_OUTER_X, 20.5F, 14.01F},
     4.25F,
     9.0F,
     1.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {40.0F, 20.5F, 14.01F},
     9.0F,
     9.0F,
     1.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-40.0F, 20.5F, 14.01F},
     9.0F,
     9.0F,
     1.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {51.0F, 20.5F, 14.01F},
     3.0F,
     9.0F,
     1.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {28.0F, 20.5F, 14.01F},
     3.0F,
     9.0F,
     1.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-51.0F, 20.5F, 14.01F},
     3.0F,
     9.0F,
     1.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-28.0F, 20.5F, 14.01F},
     3.0F,
     9.0F,
     1.0F,
     {0},
     0,
     NULL},
};

/* TRUE once player 0's character has its train flag set. Eggman and Tikal have
 * none, and get FALSE */
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

static Bool train_car_touched(struct task_work *work)
{
    return (work->flags & COLLISION_TOUCHING) &&
           work->collision->hit->kind == COLLISION_PLAYER;
}

/* TRUE while player 0 is in the car */
static Bool train_car_holds(struct task_work *work)
{
    Float y = player_work[0]->pos.y - work->pos.y;
    Float x = player_work[0]->pos.x - work->pos.x;
    Float z = player_work[0]->pos.z - work->pos.z;
    Float cos = njCos(-work->ang.y);
    Float sin = njSin(-work->ang.y);
    Float along = (x * cos) + (z * sin);
    Float across = (z * cos) - (x * sin);

    if (along > -INSIDE_LENGTH && along < INSIDE_LENGTH && y > 0.0F &&
        y < INSIDE_HEIGHT && across < INSIDE_WIDTH && across > -INSIDE_WIDTH) {
        return TRUE;
    }
    return FALSE;
}

static void train_car_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *train = task->parent->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, CAR_RADIUS)) {
        njSetTexture(&mr_train_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(mystic_ruins_train_action, VARS(train)->frame);
        njPopMatrix(1);
    }
}

/* move the car's object in the collision world with the car. while the train
 * is shut, the blocks and posts seal the doorways, and the doors are off.
 * otherwise only the doors are on, and they slide apart as the frame opens
 * them */
static void train_car_collide(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *train = task->parent->work;
    NJS_OBJECT *object;
    struct collision_shape *shapes;
    Float open;

    if (player_in_range(&work->pos,
                        VARS(work)->object->model->r + NEAR_MARGIN)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    object = VARS(work)->object;
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->ang[0] = work->ang.x;
    object->ang[1] = work->ang.y;
    object->ang[2] = work->ang.z;
    open = VARS(train)->frame * DOOR_SLIDE /
           (mystic_ruins_train_action->motion->nbFrame - 1);
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
        shapes[0].center.x = DOOR_INNER_X - open;
        shapes[1].center.x = DOOR_OUTER_X + open;
        shapes[2].center.x = -DOOR_INNER_X + open;
        shapes[3].center.x = -DOOR_OUTER_X - open;
    }
    collision_register(work);
}

/* put the car at its place in the train. the move goes into its task's force,
 * so that what stands on the car moves with it */
static void train_car_place(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *train = task->parent->work;
    struct task_force *force;
    Angle ang;
    Float along = (Sint8)CAR_PLACE(work) + VARS(train)->offset;
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
    if (train_car_holds(work)) {
        /* @hack spelling: the train's work read again through task->parent
         * (0x0c90e298, 0x0c90e29a); through train, 33 of 36 words differ */
        struct task_work *parent = task->parent->work;

        parent->submode |= TRAIN_PLAYER_IN;
    }
    if (train_car_touched(work)) {
        train->submode |= TRAIN_TOUCHED;
    }
    train_car_collide(task);
}

static void train_car_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->object);
    world_object_free(VARS(work)->object);
}

static void train_car_start(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    VARS(work)->object = object = world_object_alloc();
    object->evalflags = NJD_EVAL_BREAK;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = work->ang.x;
    object->ang[1] = work->ang.y;
    object->ang[2] = work->ang.z;
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->model = train_car_object.model;
    object->child = train_car_object.child;
    object->sibling = train_car_object.sibling;
    world_add(CAR_WORLD_FLAGS, task, object);
    task->destructor = train_car_destroy;
    collision_init(task, train_car_shapes, COLLISION_OBJECT, 1, 10);
    task->step = train_car_step;
    task->draw = train_car_draw;
}

static void train_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* the doors shut on player 0 in a car, unless player 0 steps out first.
 * player 0 is then suspended, and the train pulls out and gains speed.
 * FADE_STEP steps into the trip, the game fades to Station Square, and the
 * fade ends the act. the train would come back in once AWAY_LENGTHS out,
 * resume player 0 and open its doors. it gets that far only some 1,530 steps
 * after FADE_STEP, so the act has ended first. the doors also open or shut as
 * train_open changes */
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
        } else if (work->submode == TRAIN_PLAYER_IN) {
            TASK_TIMER(work) = SHUT_DELAY;
            work->mode = TRAIN_CLOSING;
            sound_play(SE_DOORS, 0, 0, 0);
        }
        break;
    case TRAIN_CLOSING:
        /* @hack spelling: the last test steps the frame itself, which stores it
         * in the slot of its BF/S (0x0c90e4f0); stepped ahead of it, 42 of 277
         * words differ */
        if (!(work->submode & TRAIN_PLAYER_IN)) {
            work->mode = TRAIN_REOPENING;
        } else if (TASK_TIMER(work) > 0) {
            TASK_TIMER(work)--;
        } else if (VARS(work)->frame > SHUT_SLOW_FRAME) {
            VARS(work)->frame -= SHUT_SPEED;
        } else if ((VARS(work)->frame -= SHUT_SLOW_SPEED) <= 0.0F) {
            player_suspend(0);
            VARS(work)->frame = 0.0F;
            TASK_TIMER(work) = 0;
            work->mode = TRAIN_LEAVING;
        }
        break;
    case TRAIN_REOPENING: {
        Float last = LAST_FRAME();

        /* @hack spelling: the frame stepped in the test, which stores it in the
         * slot of its BF/S (0x0c90e52c); stepped ahead of it, 8 of 277 words
         * differ */
        if ((VARS(work)->frame += OPEN_SPEED) >= last) {
            VARS(work)->frame = last;
            work->mode = TRAIN_OPEN;
        }
        break;
    }
    case TRAIN_LEAVING:
        if (TASK_TIMER(work) < CRUISE_STEP) {
            TASK_TIMER(work)++;
        }
        if (TASK_TIMER(work) == FADE_STEP) {
            stage_set_entrance(TO_ENTRANCE);
            stage_fade_to(STAGE_STATION_SQUARE, STATION_SQUARE_CASINO);
        }
        if (TASK_TIMER(work) >= PULL_STEP) {
            VARS(work)->offset -= (TASK_TIMER(work) - PULL_STEP) * PULL_ACCEL;
            if (VARS(work)->offset < -AWAY_LENGTHS) {
                TASK_TIMER(work) = ARRIVE_STEPS;
                work->mode = TRAIN_ARRIVING;
            }
        }
        break;
    case TRAIN_ARRIVING:
        if (--TASK_TIMER(work) == 0) {
            player_resume(0);
            work->mode = TRAIN_OPENING;
        }
        if (TASK_TIMER(work) >= STAND_STEPS) {
            int left = TASK_TIMER(work) - STAND_STEPS;

            VARS(work)->offset = -(left * (left - 1) * ARRIVE_EASE);
        }
        break;
    case TRAIN_OPENING: {
        Float last = LAST_FRAME();

        /* @hack spelling: the frame stepped in the test, which stores it in the
         * slot of its BT/S (0x0c90e662); stepped ahead of it, 53 of 277 words
         * differ */
        if ((VARS(work)->frame += OPEN_SPEED) >= last) {
            VARS(work)->frame = last;
            work->mode = TRAIN_EMPTYING;
        }
        break;
    }
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
    }
    work->submode = 0;
    task_step_children(task);
}

void train_start(struct task *task)
{
    struct task_work *work = task->work;

    if (stage_get_stage_act() & 1) {
        return;
    }
    if (placement_editing(task)) {
        Angle ang;

        njSetTexture(&mr_train_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(mystic_ruins_train_action, 0.0F);
        njPopMatrix(1);
    } else {
        int place;

        for (place = -1; place < 2; place++) {
            struct task *car = task_create_child(TASK_WORK | TASK_FORCE,
                                                 train_car_start, task);
            struct task_work *car_work = car->work;

            CAR_PLACE(car_work) = place;
            train_car_place(car);
        }
        VARS(work)->offset = 0.0F;
        if (train_open()) {
            work->mode = TRAIN_OPEN;
            VARS(work)->frame = LAST_FRAME();
        } else {
            work->mode = TRAIN_SHUT;
            VARS(work)->frame = 0.0F;
        }
        task->step = train_step;
        task->destructor = train_destroy;
    }
}
