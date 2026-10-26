/* O AXEL PANEL, the Perfect Chaos fight's own dash panel. unlike
 * 1ST_READ.BIN's O DUSH PANEL, it adds its speed to a player's own */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos7/axel.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/models.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* a panel leaves once neither the camera nor player 0 is within 4010 of
 * it. placement_try_leave_by takes the range squared. a panel that is a
 * child task never leaves */
#define AXEL_RANGE (4010.0F * 4010.0F)

/* the panel adds AXEL_SPEED to its scl.x and AXEL_STEPS to its scl.y as it
 * starts, and keeps each at 0 or above. scl.x is then the speed it adds to
 * a player's speed ahead. scl.y is the steps the player's stick is then
 * ignored for, and the panel's wait before it sends a player again */
#define AXEL_SPEED 4.0F
#define AXEL_STEPS 60.0F

struct axel_vars {
    NJS_ACTION *action;
    Float frame;
    /* the steps before the panel may send a player again. the step's test
     * reads the count as a Float, through as_float */
    union {
        Sint32 steps;
        Float as_float;
    } wait;
};

#define VARS(work) ((struct axel_vars *)&(work)->vars)

enum axel_mode { AXEL_INIT, AXEL_RUN, AXEL_IDLE };

/* the capsule of 1ST_READ.BIN's dash_panel_shape, with SHAPE_NO_PUSH */
static struct collision_shape axel_shape[1] = {
    {SHAPE_CAPSULE | SHAPE_NO_PUSH,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     10.0F,
     4.44F,
     0.0F,
     {NJM_DEG_ANG(90), 0, 0},
     0,
     NULL},
};

static void axel_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_regular();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y + NJM_DEG_ANG(180), ang);
    draw_action(VARS(work)->action, VARS(work)->frame);
    njPopMatrix(1);
}

static void axel_destroy(struct task *task)
{
    task_kill(task);
}

/* while the set editor is open, draw the panel, and label its set fields
 * while the editor holds it. otherwise send off a player on the ground who
 * touches it, along the panel's angles */
void chaos7_axel_panel_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ACTION *action;
    Sint8 player;
    NJS_VECTOR speed;
    Angle ang[3];

    if (task->parent == NULL && placement_try_leave_by(task, AXEL_RANGE)) {
        return;
    }
    action = &dash_panel_action;
    if (edit_mode) {
        VARS(work)->action = action;
        axel_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "> AXELERATION +");
            njPrintF(NJM_LOCATION(33, EDITOR_ROW_SCL_X), AXEL_SPEED, 2);
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "> DISABLE TIME +");
            njPrintF(NJM_LOCATION(34, EDITOR_ROW_SCL_Y), AXEL_STEPS, 2);
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), ">");
        }
    } else {
        switch (work->mode) {
        case AXEL_INIT:
            work->mode = AXEL_RUN;
            work->scl.x += AXEL_SPEED;
            if (work->scl.x <= 0.0F) {
                work->scl.x = 0.0F;
            }
            work->scl.y += AXEL_STEPS;
            if (work->scl.y <= 0.0F) {
                work->scl.y = 0.0F;
            }
            VARS(work)->action = action;
            task->destructor = axel_destroy;
            task->draw = axel_draw;
            collision_init(task, axel_shape, COLLISION_OBJECT, 1, 1);
            break;
        case AXEL_RUN:
            axel_draw(task);
            /* @hack spelling: the frame advanced inside the test, which
             * compares the sum in FR2 as it stores it (FCMP/GT FR2,FR1 at
             * 0x0c91c8a2); advanced before it, SHC reloads the frame, the
             * function runs 2 bytes long and 102 of 237 words differ */
            if ((VARS(work)->frame += 1.0F) >=
                VARS(work)->action->motion->nbFrame) {
                VARS(work)->frame = 0.0F;
            }
            if (VARS(work)->wait.steps > 0) {
                VARS(work)->wait.steps--;
            }
            if ((work->collision->flags & COLLISION_HIT) &&
                work->collision->hit->kind == COLLISION_PLAYER) {
                player = PLAYER_VARS(work->collision->hit->owner->work)->player;
                /* @bug arithmetic: the test finds every wait from 1 to 0x7fffff
                 * equal to 0, so the wait holds no player back. the wait is an
                 * Sint32, counted down at 0x0c91c8b2, but the test reads it as
                 * a Float (FCMP/EQ at 0x0c91c8e8), and such a count reads as a
                 * denormal. the game runs with FPSCR at 0x40001, whose DN bit
                 * makes FCMP take a denormal as 0 */
                if ((player_work[player]->flags & PLAYER_ON_LAND) &&
                    VARS(work)->wait.as_float == 0.0F) {
                    speed = player_get(player)->speed;
                    speed.x += work->scl.x;
                    ang[0] = work->ang.x;
                    ang[1] = work->ang.y;
                    ang[2] = work->ang.z;
                    VARS(work)->wait.steps = ceilf(work->scl.y);
                    player_send_flying(player, &speed, ang,
                                       VARS(work)->wait.steps);
                    sound_play(SE_DASH, 0, 0, 0);
                }
            }
            collision_register(work);
            break;
        case AXEL_IDLE:
            break;
        }
    }
}
