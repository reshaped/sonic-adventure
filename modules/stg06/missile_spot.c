/* the spot an aircraft's missile flies at */
#include <ninja.h>

#include "collision/ground.h"
#include "draw/tree.h"
#include "player/player.h"
#include "stg06/missile_spot.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT spot_object;

/* the spot is drawn this far above its pos */
#define SPOT_LIFT 0.5F
/* the share of the way to player 0 the spot moves each step, along x and z */
#define SPOT_EASE 0.25F

/* the spot's work->mode. it neither moves nor draws in SPOT_IDLE */
enum missile_spot_mode { SPOT_INIT, SPOT_RUN, SPOT_IDLE };

struct missile_spot_vars {
    NJS_OBJECT *object;
};

#define VARS(work) ((struct missile_spot_vars *)&(work)->vars)

static void missile_spot_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + SPOT_LIFT, work->pos.z);
    if (work->tick & SPOT_TURN) {
        TURN_VIA(njRotateY, work->ang.y, ang);
    }
    if (work->tick & SPOT_SCALE) {
        njScaleV(NULL, &work->scl);
    }
    texture_set_stage();
    draw_tree(VARS(work)->object);
    njPopMatrix(1);
}

/* task_destroy runs the destructor, then clears task->step. so the step
 * task_kill sets here never runs */
static void missile_spot_destroy(struct task *task)
{
    task_kill(task);
}

void missile_spot_step(struct task *task)
{
    struct task_work *work = task->work;
    struct player *player;

    switch (work->mode) {
    case SPOT_INIT:
        work->mode = SPOT_RUN;
        task->destructor = missile_spot_destroy;
        task->draw = missile_spot_draw;
        VARS(work)->object = &spot_object;
        break;
    case SPOT_RUN:
        if (work->tick & SPOT_FOLLOW) {
            player = player_get(0);
            work->pos.x += (player_work[0]->pos.x - work->pos.x) * SPOT_EASE;
            work->pos.z += (player_work[0]->pos.z - work->pos.z) * SPOT_EASE;
            if (player->shadow.below != -GROUND_FAR) {
                work->pos.y = player->shadow.below;
                if (work->tick & SPOT_TURN) {
                    work->ang.x = player->shadow.ang_x;
                    work->ang.z = player->shadow.ang_z;
                }
            }
        }
        missile_spot_draw(task);
        break;
    case SPOT_IDLE:
        break;
    }
}
