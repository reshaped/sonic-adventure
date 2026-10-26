/* the dash panel, O AXPNL or O DUSH PANEL in the object lists. a player
 * who touches it on the ground is sent off along its angles */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/dash_panel.h"
#include "object/models.h"
#include "object/placement.h"
#include "player/e102_effects.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* work->scl.x is the speed the panel sends a player off at, and scl.y the
 * steps that player's stick is then ignored for. for as many steps, the
 * panel won't send the same player again. at 0 or below, each takes its
 * default */
#define DASH_SPEED 14.0F
#define DASH_STEPS 60.0F

/* work->submode is the player the panel last sent off, or NO_PLAYER, which
 * is no player's number, before the first */
#define NO_PLAYER 13

struct dash_panel_vars {
    NJS_ACTION *action;
    Float frame;
    Sint32 steps; /* the steps left before it may send that player again */
};

#define VARS(work) ((struct dash_panel_vars *)&(work)->vars)

enum dash_panel_mode { DASH_PANEL_INIT, DASH_PANEL_RUN, DASH_PANEL_IDLE };

/* clang-format off */
static struct collision_shape dash_panel_shape[] = {
    {SHAPE_CAPSULE, 0, PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 10.0F, 4.44F,
     0.0F, {NJM_DEG_ANG(90), 0, 0}, 0, NULL},
};
/* clang-format on */

static void dash_panel_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 30.0F)) {
        return;
    }
    texture_set_regular();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y + NJM_DEG_ANG(180));
    draw_action(VARS(work)->action, VARS(work)->frame);
    njPopMatrix(1);
    if (!placement_editing(task)) {
        /* @hack spelling: the frame advanced inside the test, which compares
         * the sum in FR2 as it stores it (FCMP/GT FR2,FR1 at 0x8c05cc06);
         * advanced before it, SHC reloads the frame, the function runs 2
         * bytes long and 14 of 92 words differ */
        if ((VARS(work)->frame += 1.0F) >=
            VARS(work)->action->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
        }
    }
}

static void dash_panel_destroy(struct task *task)
{
    task_kill(task);
}

void dash_panel_step(struct task *task)
{
    struct task_work *work = task->work;
    Sint8 player;
    Angle ang[3];

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        njPrintC(NJM_LOCATION(20, 5), "SCL_X IS FOR SPEED.");
        njPrintC(NJM_LOCATION(20, 6), "IF 0.0F OR LOWER,");
        njPrintC(NJM_LOCATION(20, 7), "IT'S ");
        njPrintF(NJM_LOCATION(25, 7), DASH_SPEED, 2);
        njPrintC(NJM_LOCATION(33, 7), ".");
        njPrintC(NJM_LOCATION(20, 8), "SCL_Y IS FOR DISABLE TIME.");
        njPrintC(NJM_LOCATION(20, 9), "IF 0.0F OR LOWER,");
        njPrintC(NJM_LOCATION(20, 10), "IT'S ");
        njPrintF(NJM_LOCATION(25, 10), DASH_STEPS, 2);
        njPrintC(NJM_LOCATION(33, 10), ".");
        VARS(work)->action = &dash_panel_action;
        dash_panel_draw(task);
    } else {
        switch (work->mode) {
        case DASH_PANEL_INIT:
            work->mode = DASH_PANEL_RUN;
            work->submode = NO_PLAYER;
            VARS(work)->action = &dash_panel_action;
            task->destructor = dash_panel_destroy;
            task->draw = dash_panel_draw;
            collision_init(task, dash_panel_shape, COLLISION_OBJECT, 1, 1);
            if (work->scl.x <= 0.0F) {
                work->scl.x = DASH_SPEED;
            }
            if (work->scl.y <= 0.0F) {
                work->scl.y = DASH_STEPS;
            }
            break;
        case DASH_PANEL_RUN:
            dash_panel_draw(task);
            if (VARS(work)->steps > 0) {
                VARS(work)->steps--;
            }
            if (placement_editing(task)) {
                break;
            }
            if ((work->collision->flags & COLLISION_HIT) &&
                work->collision->hit->kind == COLLISION_PLAYER) {
                player = PLAYER_VARS(work->collision->hit->owner->work)->player;
                if ((player_work[player]->flags & PLAYER_ON_LAND) &&
                    (VARS(work)->steps == 0 || work->submode != player)) {
                    NJS_VECTOR speed = {0.0F, 0.0F, 0.0F};

                    speed.x = work->scl.x;
                    ang[0] = work->ang.x;
                    ang[1] = work->ang.y;
                    ang[2] = work->ang.z;
                    VARS(work)->steps = ceilf(work->scl.y);
                    player_send_flying(player, &speed, ang, VARS(work)->steps);
                    sound_play(SE_DASH, 0, 0, 0);
                    vibration_start(player, 4, 15, 0);
                    work->submode = player;
                    if (PLAYER_VARS(player_work[player])->character ==
                        CHARACTER_E102) {
                        e102_set_skating(player);
                    }
                }
            }
            collision_register(work);
            break;
        case DASH_PANEL_IDLE:
            break;
        }
    }
}
