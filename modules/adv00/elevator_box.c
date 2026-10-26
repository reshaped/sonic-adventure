/* O ELEBOX IN and O ELEBOX OUT elevator doorways. the way in opens for player 0
 * and leads to the station act, and the way out stays shut */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/elevator_box.h"
#include "adv00/station_square.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: the doors' action declared as an array. SHC then pools its
 * address and reads its motion through it, MOV.L @(4,R3),R2 at 0x0c901f8a.
 * declared as one NJS_ACTION, SHC pools the motion field's address, and 13 of
 * elevator_box_collide's 76 words, 5 of elevator_box_close's 82 and 3 of
 * elevator_box_open's 52 differ */
extern NJS_ACTION elevator_box_action[];
/* the highway elevator's model, whose root the box adds to the world as its
 * floor */
extern NJS_OBJECT highway_elevator_object;

/* player 0 opens the way in, and holds it open, from within DOOR_REACH across
 * and DOOR_HEIGHT above or below it */
#define DOOR_REACH 30.0F
#define DOOR_HEIGHT 22.0F
#define DOOR_SPEED 0.2F /* motion frames a step */
#define LEAVE_WAIT 4 /* steps from the doors' shutting to the fade */

#define DOOR_HALF 5.0F /* half a door's width */
#define SE_DOORS 531 /* the doors starting to open or shut */
#define SE_BIG_SHUT_IN 1202 /* the doors shutting on Big */
/* SE_BIG_SHUT_IN plays for Big only at headings within these bounds */
#define BIG_FACING_LOW 0x3600
#define BIG_FACING_HIGH 0x4a00

/* the world tests the floor only while a player is within its model's radius
 * and this much more */
#define FLOOR_MARGIN 20.0F

/* the world flags of the floor, which players stand on */
#define FLOOR_WORLD_FLAGS (WORLD_MEDIUM | WORLD_SOLID)

/* the station act's entrance at which player 0 arrives */
#define STATION_ENTRANCE 2

/* work->mode: whether the task is the way in or the way out */
enum elevator_box_way { BOX_IN, BOX_OUT };

struct elevator_box_vars {
    /* the floor's object in the world, or in the set editor the model's own */
    NJS_OBJECT *floor;
    NJS_OBJECT *object; /* the model the floor shows */
    Float frame; /* the doors' frame in their motion, 0 shut */
};

#define VARS(work) ((struct elevator_box_vars *)&(work)->vars)

static void elevator_box_close(struct task *task);
static void elevator_box_wait(struct task *task);
static void elevator_box_open(struct task *task);
static void elevator_box_idle(struct task *task);
static void elevator_box_destroy(struct task *task);

/* the shapes of the way in's two doors, and of the way out's wall between its
 * two jambs */
/* clang-format off */
static struct collision_shape elevator_box_in_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-5.0F, 11.0F, 0.0F},
     DOOR_HALF, 11.0F, 0.5F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {5.0F, 11.0F, 0.0F},
     DOOR_HALF, 11.0F, 0.5F, {0}, 0, NULL},
};

static struct collision_shape elevator_box_out_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 16.0F, 0.0F},
     10.0F, 16.0F, 0.5F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {10.0F, 16.0F, 0.0F},
     4.0F, 16.0F, 0.5F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-10.0F, 16.0F, 0.0F},
     4.0F, 16.0F, 0.5F, {0}, 0, NULL},
};
/* clang-format on */

static Bool elevator_box_touched(struct task_work *work)
{
    return (work->flags & COLLISION_TOUCHING) &&
           work->collision->hit->kind == COLLISION_PLAYER;
}

static Bool elevator_box_entered(struct task *task)
{
    return player_stands_on(0, task);
}

static void elevator_box_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(elevator_box_action, VARS(work)->frame);
    njPopMatrix(1);
}

static int elevator_box_near(struct task_work *work)
{
    return player_in_cylinder(&work->pos, DOOR_REACH, DOOR_HEIGHT);
}

/* narrow each door's shape to the part the motion leaves in the doorway, and
 * register the record for this step's checks. the world tests the floor only
 * while a player is near it, as FLOOR_MARGIN says */
static void elevator_box_collide(struct task_work *work)
{
    Float half = DOOR_HALF - (VARS(work)->frame * DOOR_HALF /
                              (elevator_box_action->motion->nbFrame - 1));
    Float x = (2.0F * DOOR_HALF) - half;

    if (half > 0.0F) {
        work->collision->shapes[0].center.x = -x;
        work->collision->shapes[1].center.x = x;
        work->collision->shapes[0].radius = half;
        work->collision->shapes[1].radius = half;
    }
    collision_register(work);
    if (player_in_range(&work->pos,
                        VARS(work)->floor->model->r + FLOOR_MARGIN)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

/* LEAVE_WAIT steps after the doors shut on player 0, fade to the station act */
static void elevator_box_leave(struct task *task)
{
    struct task_work *work = task->work;

    elevator_box_draw(task);
    if (--work->tick == 0) {
        stage_set_entrance(STATION_ENTRANCE);
        stage_fade_to(STAGE_STATION_SQUARE, STATION_SQUARE_STATION);
    }
    elevator_box_collide(work);
}

/* shut the doors, and open them again at a player's touch. once the way in
 * shuts on player 0 standing on its floor, leave for the station act. if
 * player 0 is Big, facing from BIG_FACING_LOW to BIG_FACING_HIGH, play
 * SE_BIG_SHUT_IN too */
static void elevator_box_close(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: last set and never read, which loads the motion's frame
     * count and converts it ahead of the range check (FLOAT at 0x0c902082);
     * without it, 81 of 82 words differ */
    Float last = elevator_box_action->motion->nbFrame - 1;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    elevator_box_draw(task);
    if (!elevator_box_touched(work)) {
        /* @hack spelling: the frame decremented in the test, which SHC tests in
         * FR2 and stores in the slot of the BT/S at 0x0c9020aa. as a statement
         * of its own, SHC reads the frame back to test it, and 82 of 82 words
         * differ */
        if ((VARS(work)->frame -= DOOR_SPEED) <= 0.0F) {
            VARS(work)->frame = 0.0F;
            if (work->mode == BOX_IN && elevator_box_entered(task)) {
                work->tick = LEAVE_WAIT;
                task->step = elevator_box_leave;
                if (player_get_character(0) == CHARACTER_BIG) {
                    ang = player_work[0]->ang.y;
                    if (ang >= BIG_FACING_LOW && ang <= BIG_FACING_HIGH) {
                        sound_play(SE_BIG_SHUT_IN, 0, 0, 0);
                    }
                }
            } else {
                task->step = elevator_box_idle;
            }
        }
    } else {
        task->step = elevator_box_open;
    }
    elevator_box_collide(work);
}

/* hold the way in open until player 0 moves out of reach or stands on its
 * floor. the way out never opens, so never waits here */
static void elevator_box_wait(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    elevator_box_draw(task);
    switch (work->mode) {
    case BOX_IN:
        if (!elevator_box_near(work) || elevator_box_entered(task)) {
            task->step = elevator_box_close;
            sound_play(SE_DOORS, 0, 0, 0);
        }
        break;
    case BOX_OUT:
        if (!elevator_box_entered(task)) {
            task->step = elevator_box_close;
        }
        break;
    }
}

static void elevator_box_open(struct task *task)
{
    struct task_work *work = task->work;
    Float last = elevator_box_action->motion->nbFrame - 1;

    if (placement_try_leave(task)) {
        return;
    }
    elevator_box_draw(task);
    /* @hack spelling: the frame advanced in the test, which keeps the task in
     * R14 and the work in R13 (0x0c902218, 0x0c902224). as a statement of its
     * own, the two swap and 11 of 52 words differ */
    if ((VARS(work)->frame += DOOR_SPEED) >= last) {
        VARS(work)->frame = last;
        task->step = elevator_box_wait;
    }
    elevator_box_collide(work);
}

/* keep the doors shut until player 0 comes within reach of the way in. the way
 * out only registers its shapes, and never opens */
static void elevator_box_idle(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_editing(task)) {
        elevator_box_draw(task);
    } else {
        if (placement_try_leave(task)) {
            return;
        }
        elevator_box_draw(task);
        switch (work->mode) {
        case BOX_IN:
            if (elevator_box_near(work)) {
                task->step = elevator_box_open;
                sound_play(SE_DOORS, 0, 0, 0);
            }
            elevator_box_collide(work);
            break;
        case BOX_OUT:
            collision_register(work);
            return;
        }
    }
}

static void elevator_box_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->floor);
    world_object_free(VARS(work)->floor);
}

/* add the floor to the world at the box's position and angles, and give the
 * doorway its shapes. in the set editor, the floor is the model's own object,
 * and nothing joins the world */
static void elevator_box_start(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *floor;

    if (placement_try_leave(task)) {
        return;
    }
    VARS(work)->object = &highway_elevator_object;
    if (!placement_editing(task)) {
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
        floor->model = VARS(work)->object->model;
        floor->child = NULL;
        floor->sibling = NULL;
        world_add(FLOOR_WORLD_FLAGS, task, floor);
        task->destructor = elevator_box_destroy;
        if (work->mode == BOX_IN) {
            collision_init(task, elevator_box_in_shapes, COLLISION_OBJECT, 1,
                           2);
        } else {
            collision_init(task, elevator_box_out_shapes, COLLISION_OBJECT, 1,
                           3);
        }
    } else {
        VARS(work)->floor = VARS(work)->object;
    }
    task->step = elevator_box_idle;
    task->draw = elevator_box_draw;
}

void elevator_box_in_init(struct task *task)
{
    task->work->mode = BOX_IN;
    elevator_box_start(task);
}

void elevator_box_out_init(struct task *task)
{
    task->work->mode = BOX_OUT;
    elevator_box_start(task);
}
