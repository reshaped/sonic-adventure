/* the O PO SUMMON trigger, which sends out the path orca once player 0 or 1
 * comes near */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg01/orca_control.h"
#include "stg01/path_orca_trigger.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT orca_object;

/* how near player 0 or 1 comes to set off the trigger */
#define RADIUS 45.0F
/* the set editor draws the orca's model at ORCA_SCALE */
#define ORCA_SCALE 0.4F

enum path_orca_trigger_mode { TRIGGER_WAIT, TRIGGER_SUMMON, TRIGGER_DONE };

static void path_orca_trigger_step(struct task *task);
static void path_orca_trigger_draw(struct task *task);

void path_orca_trigger_init(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = TRIGGER_WAIT;
    task->step = path_orca_trigger_step;
}

/* once player 0 or 1 comes within RADIUS, summon the path orca as soon as
 * orca_summon_held allows. the trigger summons only once, so no path orca
 * comes if path_orca_summon finds an orca out. while the set editor is open,
 * it draws the trigger instead */
static void path_orca_trigger_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (edit_mode) {
        path_orca_trigger_draw(task);
        return;
    }
    switch (work->mode) {
    case TRIGGER_WAIT:
        if (player_in_range(&work->pos, RADIUS)) {
            work->mode = TRIGGER_SUMMON;
        }
        break;
    case TRIGGER_SUMMON:
        if (!orca_summon_held()) {
            path_orca_summon();
            work->mode = TRIGGER_DONE;
        }
        break;
    case TRIGGER_DONE:
        break;
    }
}

/* draw the trigger's radius as a marker, and the orca where scl places it
 * from the trigger. ang.z counts up each draw, and nothing reads it */
static void path_orca_trigger_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float scale = RADIUS * MARKER_FIT;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njPushMatrix(NULL);
    njScale(NULL, scale, scale, scale);
    draw_tree(&marker_object);
    njPopMatrix(1);
    njTranslateV(NULL, &work->scl);
    njRotateX(NULL, NJM_DEG_ANG(45));
    TURN_VIA(njRotateY, work->ang.y, ang);
    scale = ORCA_SCALE;
    njScale(NULL, scale, scale, scale);
    draw_tree(&orca_object);
    njPopMatrix(1);
    work->ang.z += 32;
}
