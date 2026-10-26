/* O AO KILL: a trigger that sends the chasing orca away while player 0 or
 * 1 is near */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg01/orca_away_trigger.h"
#include "stg01/orca_control.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_OBJECT orca_object;

/* how near player 0 or 1 comes to set off the trigger */
#define RADIUS 45.0F

/* TRIGGER_DONE keeps orca_trigger.c's number. no code sets it, so the
 * trigger stays in TRIGGER_WAIT */
enum orca_away_trigger_mode { TRIGGER_WAIT = 0, TRIGGER_DONE = 2 };

static void orca_away_trigger_step(struct task *task);
static void orca_away_trigger_draw(struct task *task);

void orca_away_trigger_init(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = TRIGGER_WAIT;
    task->step = orca_away_trigger_step;
}

/* send the chasing orca away at each step that player 0 or 1 is within
 * RADIUS. while the set editor is open, draw the trigger instead */
static void orca_away_trigger_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (edit_mode) {
        orca_away_trigger_draw(task);
        return;
    }
    switch (work->mode) {
    case TRIGGER_WAIT:
        if (player_in_range(&work->pos, RADIUS)) {
            orca_send_away();
        }
        break;
    case TRIGGER_DONE:
        break;
    }
}

/* draw the trigger's radius as a marker, and the orca upside down at 0.4
 * scale */
static void orca_away_trigger_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float scale = RADIUS * MARKER_FIT;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njPushMatrix(NULL);
    njScale(NULL, scale, scale, scale);
    draw_tree(&marker_object);
    njPopMatrix(1);
    njRotateZ(NULL, NJM_DEG_ANG(180));
    scale = 0.4F;
    njScale(NULL, scale, scale, scale);
    draw_tree(&orca_object);
    njPopMatrix(1);
}
