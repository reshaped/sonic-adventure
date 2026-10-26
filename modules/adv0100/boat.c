/* O BOAT boat that takes player 0 from the sunken Egg Carrier to Station
 * Square */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/station_square.h"
#include "adv0100/boats.h"
#include "adv0100/egg_carrier.h"
#include "adv0100/texlists.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT boat_object;

/* the boat leaves once player 0 has stayed in its cylinder for more steps
 * than this */
#define BOAT_WAIT 15
/* the entrance of STATION_SQUARE_STATION that the boat takes player 0 to */
#define BOAT_ENTRANCE 6

enum boat_shape { BOAT_DECK, BOAT_STERN, BOAT_BOW };

static struct collision_shape boat_shapes[3] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-1.0F, 1.5F, 0.0F},
     16.5F,
     0.5F,
     9.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-12.0F, 5.0F, 0.0F},
     5.5F,
     3.0F,
     9.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {11.0F, 4.5F, 0.0F},
     2.5F,
     3.5F,
     9.0F,
     {0},
     0,
     NULL},
};

enum boat_mode { BOAT_READY, BOAT_LOCKED, BOAT_LEAVING };

static void boat_run(struct task *task);
static void boat_draw(struct task *task);

/* TRUE once player 0's story opens the boat */
static Bool boat_available(void)
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

void egg_carrier_boat_step(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_shape *shapes;

    work->mode = BOAT_READY;
    if (!placement_editing(task)) {
        collision_init(task, boat_shapes, COLLISION_OBJECT, 1, 3);
        work->collision->radius = 30.0F;
        shapes = work->collision->shapes;
        if (!boat_available()) {
            work->mode = BOAT_LOCKED;
            shapes[BOAT_DECK].flags |= SHAPE_OFF;
            shapes[BOAT_STERN].flags |= SHAPE_OFF;
            shapes[BOAT_BOW].flags |= SHAPE_OFF;
            task->draw = NULL;
        } else {
            work->mode = BOAT_READY;
            switch (player_get_character(0)) {
            /* @hack spelling: Big's case ahead of E-102's, which makes SHC test
             * 6 before 7 (CMP/EQ at 0x0c92f356 and 0x0c92f35a); in enum
             * order, 36 of 95 words differ */
            case CHARACTER_BIG:
            case CHARACTER_E102:
                shapes[BOAT_DECK].flags &= ~SHAPE_OFF;
                shapes[BOAT_STERN].flags |= SHAPE_OFF;
                shapes[BOAT_BOW].flags |= SHAPE_OFF;
                break;
            default:
                shapes[BOAT_DECK].flags &= ~SHAPE_OFF;
                shapes[BOAT_STERN].flags &= ~SHAPE_OFF;
                shapes[BOAT_BOW].flags &= ~SHAPE_OFF;
                break;
            }
            task->draw = boat_draw;
        }
    }
    task->step = boat_run;
}

static void boat_run(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_shape *shapes;

    if (!egg_carrier_sunk) {
        return;
    }
    if (placement_editing(task)) {
        boat_draw(task);
        return;
    }
    switch (work->mode) {
    case BOAT_READY:
        if (player_in_cylinder(&work->pos, 10.0F, 20.0F)) {
            ++work->tick;
            if (work->tick > BOAT_WAIT) {
                stage_set_entrance(BOAT_ENTRANCE);
                story_go_to(STAGE_STATION_SQUARE, STATION_SQUARE_STATION);
                work->mode = BOAT_LEAVING;
                work->tick = 0;
            }
        } else {
            work->tick = 0;
        }
        collision_register(work);
        boat_draw(task);
        break;
    case BOAT_LOCKED:
        if (boat_available()) {
            shapes = work->collision->shapes;
            work->mode = BOAT_READY;
            switch (player_get_character(0)) {
            /* @hack spelling: Big's case ahead of E-102's, which makes SHC test
             * 6 before 7 (CMP/EQ at 0x0c92f4b8 and 0x0c92f4bc); in enum
             * order, 48 of 141 words differ */
            case CHARACTER_BIG:
            case CHARACTER_E102:
                shapes[BOAT_DECK].flags &= ~SHAPE_OFF;
                shapes[BOAT_STERN].flags |= SHAPE_OFF;
                shapes[BOAT_BOW].flags |= SHAPE_OFF;
                break;
            default:
                shapes[BOAT_DECK].flags &= ~SHAPE_OFF;
                shapes[BOAT_STERN].flags &= ~SHAPE_OFF;
                shapes[BOAT_BOW].flags &= ~SHAPE_OFF;
                break;
            }
            task->draw = boat_draw;
        }
        break;
    case BOAT_LEAVING:
        boat_draw(task);
        collision_register(work);
        break;
    }
}

static void boat_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle yaw;

    njSetTexture(&boat_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, yaw);
    draw_tree(&boat_object);
    njPopMatrix(1);
}
