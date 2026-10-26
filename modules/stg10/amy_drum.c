/* O AMY DRUM drum with O DRUM's model, which never bursts */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg10/amy_drum.h"
#include "stg10/drum.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"

/* no code sets DRUM_UNUSED */
enum amy_drum_mode { DRUM_INIT, DRUM_RUN, DRUM_UNUSED };

static void final_amy_drum_destroy(struct task *task);
static void final_amy_drum_draw(struct task *task);
static void final_amy_drum_draw_at(struct task_work *work);

static struct collision_shape final_amy_drum_shape = {
    SHAPE_CYLINDER | SHAPE_HURTABLE,
    SHAPE_HURTS_PLAYERS,
    PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
    {0.0F, 10.0F, 0.0F},
    5.0F,
    10.0F,
    0.0F,
    {0},
    0,
    NULL,
};

void final_amy_drum_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case DRUM_INIT:
        task->destructor = final_amy_drum_destroy;
        task->draw = final_amy_drum_draw;
        if (!placement_editing(task)) {
            collision_init(task, &final_amy_drum_shape, COLLISION_OBJECT, 1, 1);
        }
        work->mode = DRUM_RUN;
        break;
    case DRUM_RUN:
    /* @hack redundant: a case for DRUM_UNUSED, which only breaks, keeps the
     * image's CMP/EQ #2 (0x0c90effc); without it, 43 of 55 words differ */
    case DRUM_UNUSED:
        break;
    }
    final_amy_drum_draw_at(work);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void final_amy_drum_destroy(struct task *task)
{
}

static void final_amy_drum_draw(struct task *task)
{
    final_amy_drum_draw_at(task->work);
}

static void final_amy_drum_draw_at(struct task_work *work)
{
    if (frame_step == 0) {
        texture_set_stage();
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        draw_tree(&final_drum_object);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
}
