/* O CONNECTION, a coupling between two cargo cars */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "stg12/cargo_train.h"
#include "stg12/connection.h"
#include "stg12/texlists.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT connection_object;

/* how far work->collision reaches from the task. init sets it in place of
 * the reach collision_init measures from the shape */
#define CONNECTION_REACH 60.0F

/* in work->submode: a coupling an act's object list placed, or one
 * cargo_train_start made */
enum connection_kind { CONNECTION_PLACED, CONNECTION_ON_TRAIN };

static void connection_init(struct task *task);
static void connection_placed(struct task *task);
static void connection_on_train(struct task *task);
static void connection_draw(struct task *task);

/* a capsule along the coupling, turned about x to lie along z */
static struct collision_shape connection_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 11.5F, 0.0F},
    5.0F,
    80.0F,
    0.0F,
    {NJM_DEG_ANG(90), 0, 0},
    0,
    NULL,
};

void connection_step(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = CONNECTION_PLACED;
    connection_init(task);
}

void connection_train_step(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = CONNECTION_ON_TRAIN;
    connection_init(task);
}

static void connection_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    if (!placement_editing(task)) {
        collision_init(task, &connection_shape, COLLISION_OBJECT, 1, 1);
        work->collision->radius = CONNECTION_REACH;
    }
    if (work->submode == CONNECTION_PLACED) {
        task->step = connection_placed;
    } else {
        task->step = connection_on_train;
    }
    task->draw = connection_draw;
}

static void connection_placed(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        collision_register(work);
    }
    connection_draw(task);
}

static void connection_on_train(struct task *task)
{
    struct task_work *work = task->work;
    Float speed;

    connection_draw(task);
    speed = cargo_train_speed;
    if (speed != 0.0F) {
        /* @hack spelling: the sum held in speed, then stored, which sums into
         * speed's FR4 after a copy of speed into FR2 at 0x0c9137fc. as
         * work->pos.z += speed, SHC sums into FR2, the function runs 2 bytes
         * short, and 10 of 19 words differ */
        speed = work->pos.z + speed;
        work->pos.z = speed;
    }
    collision_register(work);
}

static void connection_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&hotshelter4_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&connection_object);
        njPopMatrix(1);
    }
}
