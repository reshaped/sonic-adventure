/* O AO SUMMON: a trigger that sends out the chasing orca once player 0
 * comes near */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg01/orca_control.h"
#include "stg01/orca_trigger.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT orca_object;

/* how near player 0 comes to set off the trigger */
#define RADIUS 45.0F

enum orca_trigger_mode { TRIGGER_WAIT, TRIGGER_SUMMON, TRIGGER_DONE };

static void orca_trigger_step(struct task *task);
static void orca_trigger_draw(struct task *task);

void orca_trigger_init(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = TRIGGER_WAIT;
    task->step = orca_trigger_step;
}

/* once player 0 comes within RADIUS, summon the chasing orca as soon as
 * orca_summon_held allows. the summon gives the orca the position plus scl,
 * heading ang.y. the trigger then does nothing more. while the set editor
 * is open, it draws itself instead */
static void orca_trigger_step(struct task *task)
{
    struct task_work *work = task->work;
    Float *pos;
    Float *offset;
    Float x;
    Float y;
    Float z;

    if (placement_try_leave(task)) {
        return;
    }
    if (edit_mode) {
        orca_trigger_draw(task);
        return;
    }
    switch (work->mode) {
    case TRIGGER_WAIT:
        if (player_in_range(&work->pos, RADIUS) == 1) {
            work->mode = TRIGGER_SUMMON;
        }
        break;
    case TRIGGER_SUMMON:
        if (!orca_summon_held()) {
            /* @hack spelling: the cursors pos and offset, which read the
             * position and scl by FMOV.S @R5+ and @R4+, from 0x0c90ae22 to
             * 0x0c90ae3c. as work->pos.x + work->scl.x and the like, the
             * code grows 4 bytes, and 29 of 65 words differ */
            pos = VECTOR_FLOATS(&work->pos);
            offset = VECTOR_FLOATS(&work->scl);
            x = *pos++ + *offset++;
            y = *pos++ + *offset++;
            z = *pos + *offset;
            orca_summon(x, y, z, work->ang.y);
            work->mode = TRIGGER_DONE;
        }
        break;
    case TRIGGER_DONE:
        break;
    }
}

/* draw the trigger's radius as a marker, and the orca at 0.4 scale where
 * it will appear, turned by ang.y */
static void orca_trigger_draw(struct task *task)
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
    TURN_VIA(njRotateY, work->ang.y, ang);
    scale = 0.4F;
    njScale(NULL, scale, scale, scale);
    draw_tree(&orca_object);
    njPopMatrix(1);
}
