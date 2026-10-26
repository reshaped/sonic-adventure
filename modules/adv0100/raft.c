/* O IKADA, the raft that takes player 0 from the sunken Egg Carrier to the
 * Mystic Ruins */
#include <ninja.h>

#include "adv0100/boats.h"
#include "adv0100/egg_carrier.h"
#include "adv0100/texlists.h"
#include "adv02/mystic_ruins.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT raft_object;

/* player 0 is on the raft within RIDE_RADIUS across and RIDE_HEIGHT above
 * or below its position. more than RIDE_AFTER steps in a row there send
 * player 0 to MYSTIC_RUINS_STATION, at RIDE_ENTRANCE */
#define RIDE_RADIUS 15.0F
#define RIDE_HEIGHT 5.0F
#define RIDE_AFTER 15
#define RIDE_ENTRANCE 7

/* the record's reach: the distance across x and z to the box's corners,
 * sqrt(24 * 24 + 18 * 18), from raft_shape's radius and size_z */
#define RAFT_REACH 30.0F

static struct collision_shape raft_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    24.0F,
    1.0F,
    18.0F,
    {0},
    0,
    NULL,
};

enum raft_mode { RAFT_READY, RAFT_LOCKED, RAFT_LEAVING };

static void raft_run(struct task *task);
static void raft_draw(struct task *task);

/* player 0's character's raft flag, or 0 for Eggman and Tikal */
static int raft_available(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_RAFT];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_RAFT];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_RAFT];
    case CHARACTER_AMY:
        return event_flags[FLAG_AMY_RAFT];
    case CHARACTER_E102:
        return event_flags[FLAG_E102_RAFT];
    case CHARACTER_BIG:
        return event_flags[FLAG_BIG_RAFT];
    }
    return 0;
}

/* outside the set editor, a raft whose flag is clear starts locked: its
 * shape off and nothing drawn. raft_run unlocks it once the flag is set */
void egg_carrier_raft_step(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_shape *shape;

    work->mode = RAFT_READY;
    if (!placement_editing(task)) {
        collision_init(task, &raft_shape, COLLISION_OBJECT, 1, 1);
        work->collision->radius = RAFT_REACH;
        shape = work->collision->shapes;
        if (!raft_available()) {
            work->mode = RAFT_LOCKED;
            shape->flags |= SHAPE_OFF;
            task->draw = NULL;
        } else {
            work->mode = RAFT_READY;
            shape->flags &= ~SHAPE_OFF;
            task->draw = raft_draw;
        }
    }
    task->step = raft_run;
}

/* do nothing until the Egg Carrier has sunk. while the set editor holds the
 * task, only draw the raft */
static void raft_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!egg_carrier_sunk) {
        return;
    }
    if (placement_editing(task)) {
        raft_draw(task);
        return;
    }
    switch (work->mode) {
    case RAFT_READY:
        if (player_in_cylinder(&work->pos, RIDE_RADIUS, RIDE_HEIGHT)) {
            ++work->tick;
            if (work->tick > RIDE_AFTER) {
                stage_set_entrance(RIDE_ENTRANCE);
                story_go_to(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_STATION);
                work->mode = RAFT_LEAVING;
                work->tick = 0;
            }
        } else {
            work->tick = 0;
        }
        collision_register(work);
        raft_draw(task);
        break;
    case RAFT_LOCKED:
        if (raft_available()) {
            work->mode = RAFT_READY;
            work->collision->shapes->flags &= ~SHAPE_OFF;
            task->draw = raft_draw;
        }
        break;
    case RAFT_LEAVING:
        raft_draw(task);
        collision_register(work);
        break;
    }
}

static void raft_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle yaw;

    njSetTexture(&raft_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, yaw);
    draw_tree(&raft_object);
    njPopMatrix(1);
}
