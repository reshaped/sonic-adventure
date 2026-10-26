/* O RING2 ring that spins about x, where a placed ring spins about y */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "hud/hud.h"
#include "object/models.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg04/ring_2.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* destroyed beyond 500 units of the camera and player 0 */
#define RING_2_RANGE (500.0F * 500.0F)

enum ring_2_mode { RING_2_INIT, RING_2_WAIT, RING_2_TAKEN };

static void ring_2_draw(struct task *task);

static struct collision_shape ring_2_shape = {
    SHAPE_SPHERE,
    0,
    PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OTHERS),
    {0.0F, 0.0F, 0.0F},
    5.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};

/* a hit collects the ring, and its placement is killed on the next step */
void ring_2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RING_2_RANGE)) {
        return;
    }
    switch (work->mode) {
    case RING_2_INIT:
        collision_init(task, &ring_2_shape, COLLISION_RING, 1, 1);
        work->mode = RING_2_WAIT;
        work->id = RING_ID;
        task->draw = ring_2_draw;
        break;
    case RING_2_WAIT:
        if (edit_mode == 0) {
            if (work->collision->flags & COLLISION_HIT) {
                work->mode = RING_2_TAKEN;
                ring_add(1);
                sound_play(SE_RING, 0, 0, 0);
            }
        }
        if (edit_mode == 0) {
            collision_register(work);
            work->ang.x += NJM_DEG_ANG(2);
        }
        ring_2_draw(task);
        break;
    case RING_2_TAKEN:
        placement_kill(task);
        break;
    }
}

static void ring_2_draw(struct task *task)
{
    struct task_work *work = task->work;

    draw_attr_save();
    draw_attr_clear(0, NJD_FLAG_IGNORE_SPECULAR);
    texture_set_regular();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateX, work->ang.x);
    draw_model(&ring_model);
    njPopMatrix(1);
    draw_attr_restore();
}
