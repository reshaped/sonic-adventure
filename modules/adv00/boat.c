/* O BOAT boat that takes player 0 to the Egg Carrier once the story opens the
 * way */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/boat.h"
#include "adv00/station_square.h"
#include "adv00/texlists.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT boat_object;

/* while something stands on the boat, it sinks BOAT_SPEED a step until its
 * height is BOAT_SETTLE from rest. otherwise it rises BOAT_SPEED a step back
 * to rest */
#define BOAT_SETTLE (-1.0F)
#define BOAT_SPEED 0.2F

/* player 0 is aboard while something stands on the boat, and player 0 is
 * within BOARD_RADIUS of the boat across and BOARD_HEIGHT up or down */
#define BOARD_RADIUS 5.0F
#define BOARD_HEIGHT 20.0F

/* player 0 boards once it has been aboard more than BOAT_BOARD steps in a
 * row. the boat then leaves for the Egg Carrier more than BOAT_LEAVE steps
 * later */
#define BOAT_BOARD 15
#define BOAT_LEAVE 40

/* Egg Carrier entrance for player 0 arriving by boat */
#define BOAT_ENTRANCE 6

/* the collision record's radius. it reaches past the shapes, whose farthest
 * point lies about 64 from the boat's position */
#define RECORD_RADIUS 100.0F

enum boat_mode {
    BOAT_MOORED = 0,
    BOAT_BOARDED = 1,
    BOAT_SHUT = 4, /* hidden, with only the wall solid, until the way opens */
    BOAT_GONE = 5
};

struct boat_vars {
    Float rest; /* the boat's height at rest */
};

#define VARS(work) ((struct boat_vars *)&(work)->vars)

/* the wall goes around the other three, solid only while the way is shut */
enum boat_shape { BOAT_DECK, BOAT_STERN, BOAT_BOW, BOAT_WALL };

static void boat_step(struct task *boat);
static void boat_draw(struct task *task);

/* clang-format off */
static struct collision_shape boat_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-1.0F, 1.5F, 0.0F},
     16.5F, 0.5F, 9.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-12.0F, 5.0F, 0.0F},
     5.5F, 3.0F, 9.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {11.0F, 4.5F, 0.0F},
     2.5F, 3.5F, 9.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {-1.0F, 30.0F, 0.0F}, 16.5F, 30.0F, 12.0F, {0}, 0, NULL},
};
/* clang-format on */

/* TRUE once player 0's story opens the way to the Egg Carrier */
static Bool boat_open(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_BOAT];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_BOAT];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_BOAT];
    case CHARACTER_AMY:
        return event_flags[FLAG_AMY_BOAT];
    case CHARACTER_E102:
        return event_flags[FLAG_E102_BOAT];
    case CHARACTER_BIG:
        return event_flags[FLAG_BIG_BOAT];
    }
    return FALSE;
}

void station_square_boat_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct collision_shape *shapes;
    Angle ang;

    if ((Uint8)stage_get_stage_act() != STATION_SQUARE_STATION) {
        njPrintC(NJM_LOCATION(10, 8), "ACT STATION ONLY !");
        return;
    }
    task->step = boat_step;
    task->draw = boat_draw;
    work->ang.x = 0;
    work->ang.z = 0;
    work->tick = 0;
    VARS(work)->rest = work->pos.y;
    work->mode = BOAT_MOORED;
    motion->aim.z = 0;
    motion->speed.x = 0.0F;
    motion->speed.y = 0.0F;
    motion->speed.z = 0.0F;
    {
        NJS_VECTOR drift = {-0.1F, 0.0F, 0.0F};

        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njCalcVector(NULL, &drift, &motion->accel);
        njPopMatrix(1);
    }
    if (placement_editing(task)) {
        return;
    }
    collision_init(task, boat_shapes, COLLISION_OBJECT, 1, 4);
    work->collision->radius = RECORD_RADIUS;
    shapes = work->collision->shapes;
    /* for E-102 and Big only the deck is solid */
    switch (player_get_character(0)) {
    /* @hack spelling: a goto from each case to one label, so that the switch
     * falls from its last test into the shared arm and branches to the
     * default (BF at 0x0c90641c). as plain cases, a BRA to the default
     * follows the tests, and 79 of 180 words differ. with a goto from
     * E-102's case alone, the deck's flags pass through R3 in place of R2
     * (0x0c90641e), and 3 of 180 differ */
    case CHARACTER_E102:
        goto deck_only;
    case CHARACTER_BIG:
        goto deck_only;
    deck_only:
        shapes[BOAT_DECK].flags &= ~SHAPE_OFF;
        shapes[BOAT_STERN].flags |= SHAPE_OFF;
        shapes[BOAT_BOW].flags |= SHAPE_OFF;
        shapes[BOAT_WALL].flags |= SHAPE_OFF;
        break;
    default:
        shapes[BOAT_DECK].flags &= ~SHAPE_OFF;
        shapes[BOAT_STERN].flags &= ~SHAPE_OFF;
        shapes[BOAT_BOW].flags &= ~SHAPE_OFF;
        shapes[BOAT_WALL].flags |= SHAPE_OFF;
    }
    if (!boat_open()) {
        work->mode = BOAT_SHUT;
        shapes[BOAT_DECK].flags |= SHAPE_OFF;
        shapes[BOAT_STERN].flags |= SHAPE_OFF;
        shapes[BOAT_BOW].flags |= SHAPE_OFF;
        shapes[BOAT_WALL].flags &= ~SHAPE_OFF;
        task->draw = NULL;
    }
}

static void boat_step(struct task *boat)
{
    /* @hack spelling: a copy of boat, which keeps the task in R13 (MOV
     * R4,R13 at 0x0c9064ee); without it, the task takes R12 and 63 of 237
     * words differ */
    struct task *task = boat;
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (work->mode != BOAT_SHUT) {
        boat_draw(task);
    }
    if (placement_editing(task)) {
        return;
    }
    switch (work->mode) {
    case BOAT_SHUT:
        if (boat_open()) {
            struct collision_shape *shapes = work->collision->shapes;

            work->mode = BOAT_MOORED;
            switch (player_get_character(0)) {
            /* @hack spelling: a goto from each case to one label, as in
             * station_square_boat_init (BF at 0x0c906588). as plain cases,
             * 179 of 237 words differ. with a goto from E-102's case alone,
             * R2 and R3 swap in the three flag updates (0x0c90658a to
             * 0x0c90659a), and 9 of 237 differ */
            case CHARACTER_E102:
                goto deck_only;
            case CHARACTER_BIG:
                goto deck_only;
            deck_only:
                shapes[BOAT_DECK].flags &= ~SHAPE_OFF;
                shapes[BOAT_STERN].flags |= SHAPE_OFF;
                shapes[BOAT_BOW].flags |= SHAPE_OFF;
                shapes[BOAT_WALL].flags |= SHAPE_OFF;
                break;
            default:
                shapes[BOAT_DECK].flags &= ~SHAPE_OFF;
                shapes[BOAT_STERN].flags &= ~SHAPE_OFF;
                shapes[BOAT_BOW].flags &= ~SHAPE_OFF;
                shapes[BOAT_WALL].flags |= SHAPE_OFF;
            }
            task->draw = boat_draw;
        }
        collision_register(work);
        return;
    case BOAT_MOORED:
        if (work->flags & COLLISION_UNDER) {
            work->flags &= ~COLLISION_UNDER;
            if (work->pos.y - VARS(work)->rest > BOAT_SETTLE) {
                work->pos.y -= BOAT_SPEED;
            }
            if (player_in_cylinder(&work->pos, BOARD_RADIUS, BOARD_HEIGHT)) {
                work->tick++;
                if (work->tick > BOAT_BOARD) {
                    player_set_mode(0, PLAYER_MODE_STILL);
                    player_set_heading(0, work->ang.y - 0x4000);
                    /* @hack redundant: player 0's work read and dropped (JSR
                     * at 0x0c906664, MOV.L @(R0,R2),R2 at 0x0c90666a);
                     * without it, 84 of 237 words differ */
                    (void)player_get_task(0)->work;
                    work->tick = 0;
                    work->mode = BOAT_BOARDED;
                }
            } else {
                work->tick = 0;
            }
        } else {
            work->tick = 0;
            if (work->pos.y - VARS(work)->rest < 0.0F) {
                work->pos.y += BOAT_SPEED;
                if (work->pos.y - VARS(work)->rest > 0.0F) {
                    work->pos.y = VARS(work)->rest;
                }
            }
        }
        collision_register(work);
        break;
    case BOAT_BOARDED:
        work->tick++;
        if (work->tick > BOAT_LEAVE) {
            stage_set_entrance(BOAT_ENTRANCE);
            story_go_to(STAGE_EGG_CARRIER, 0);
            work->mode = BOAT_GONE;
        }
        break;
    case BOAT_GONE:
        break;
    }
}

/* draw the boat rolled by motion->aim.z, which init sets to 0 and boat_step
 * leaves as it is */
static void boat_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    Angle ang;

    njSetTexture(&ss_boat_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, motion->aim.z, ang);
    njTranslate(NULL, 1.5F, 0.0F, 0.0F);
    draw_tree(&boat_object);
    njPopMatrix(1);
}
