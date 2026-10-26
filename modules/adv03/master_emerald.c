/* O M EMERALD, the Master Emerald, which turns and bobs */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv03/master_emerald.h"
#include "collision/collision.h"
#include "draw/tree.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT master_emerald_object;

/* each step the emerald turns TURN_SPEED about y, and its bob's phase
 * advances BOB_SPEED. both are angles */
#define TURN_SPEED 0x80
#define BOB_SPEED 0x100

enum master_emerald_mode { EMERALD_INIT, EMERALD_TURNING };

struct emerald_vars {
    Uint8 unk_00[4];
    Sint16 phase; /* the bob's phase */
    Uint8 unk_06[2];
    Float bob; /* the sine of phase, which the draw adds to pos.y */
};

#define VARS(work) ((struct emerald_vars *)&(work)->vars)

/* a sphere about it, which pushes every group back and which none can
 * push */
static struct collision_shape master_emerald_shape[1] = {
    {SHAPE_SPHERE | SHAPE_UNTURNED,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 10.0F, 0.0F},
     20.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

static void master_emerald_draw(struct task *task);

void master_emerald_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case EMERALD_INIT:
        collision_init(task, master_emerald_shape, COLLISION_OBJECT, 1, 1);
        task->draw = master_emerald_draw;
        work->mode = EMERALD_TURNING;
        break;
    case EMERALD_TURNING:
        work->ang.y += TURN_SPEED;
        VARS(work)->phase += BOB_SPEED;
        VARS(work)->bob = njSin(VARS(work)->phase);
        break;
    }
    collision_register(work);
    master_emerald_draw(task);
}

static void master_emerald_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + VARS(work)->bob, work->pos.z);
    TURN(njRotateY, work->ang.y);
    draw_tree(&master_emerald_object);
    njPopMatrix(1);
}
