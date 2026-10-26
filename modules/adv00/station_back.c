/* O STATION BACK barrier across the station's back, gone while the story of
 * player 0's character has it open */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/station_back.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT station_back_object;

static void station_back_step(struct task *task);
static void station_back_draw(struct task *task);

/* a wall along z, and a post at each end */
/* clang-format off */
static struct collision_shape station_back_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 20.0F, 0.0F},
     1.0F, 20.0F, 25.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 20.0F, 25.0F},
     5.0F, 20.0F, 25.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 20.0F, -25.0F},
     5.9F, 20.0F, 25.0F, {0}, 0, NULL},
};
/* clang-format on */

/* TRUE while the station's back is open to player 0's character: Sonic,
 * Tails, Knuckles or Amy */
static Bool station_back_open(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_SS_STATION_BACK];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_SS_STATION_BACK];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_SS_STATION_BACK];
    case CHARACTER_AMY:
        return event_flags[FLAG_AMY_SS_STATION_BACK];
    case CHARACTER_E102:
    case CHARACTER_BIG:
        break;
    }
    return FALSE;
}

void station_back_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        if (station_back_open()) {
            placement_kill(task);
            return;
        }
        collision_init(task, station_back_shapes, COLLISION_OBJECT, 1, 3);
    }
    work->ang.x = 0;
    work->ang.z = 0;
    task->step = station_back_step;
    task->draw = station_back_draw;
}

static void station_back_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        if (station_back_open()) {
            placement_kill(task);
            return;
        }
        collision_register(work);
    }
    station_back_draw(task);
}

static void station_back_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    /* the model lies along x: a quarter turn lays it along the wall */
    TURN_VIA(njRotateY, work->ang.y + 0x4000, ang);
    draw_tree(&station_back_object);
    njPopMatrix(1);
}
