/* O KINKA solid gold coin that spins on its edge */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg09/casino.h"
#include "stg09/gold_coin.h"
#include "system/task.h"
#include "system/util.h"

/* destroyed beyond about 600 units of the camera and player 0 */
#define RANGE ((600.0F * 600.0F) + 10.0F)

/* a quarter turn, which stands the coin on its edge */
#define EDGE_ON 0x4000

struct gold_coin_vars {
    Angle spin; /* about y, added to work->ang.y */
    Uint8 unk_04[4];
    /* the radius of the sphere about the coin that draw_in_view tests before
     * the coin draws */
    Float view_radius;
};

#define VARS(work) ((struct gold_coin_vars *)&(work)->vars)

/* a coin's view_radius is scl.x + 1 times VIEW_RADIUS, its view_radius at
 * scale 0. while the set editor is open, it is EDIT_VIEW_RADIUS */
#define VIEW_RADIUS 8.0F
#define EDIT_VIEW_RADIUS 400.0F

static Angle gold_coin_turn = 336; /* added to the spin each step */
/* the coin's shape at scale 0. each step turns the collision record's shape
 * by the spin. it scales the shape's radius by scl.x + 1, and its size_y by
 * scl.y + 1 */
/* @hack type: an array of one, not the single shape it is. SHC loads its
 * address at 0x0c91489a, after the spin's load; as a single shape, it loads
 * the address two words sooner, and 3 of gold_coin_run's 65 words differ */
static struct collision_shape gold_coin_shape[1] = {
    {
        SHAPE_CYLINDER2,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 0.0F, 0.0F},
        5.0F,
        0.7F,
        0.0F,
        {0, 0, EDGE_ON},
        0,
        NULL,
    },
};

static void gold_coin_init(struct task *task);
static void gold_coin_destroy(struct task *task);
static void gold_coin_run(struct task *task);
static void gold_coin_draw(struct task *task);

void gold_coin_step(struct task *task)
{
    /* @hack redundant: a read of task->work that nothing uses, which keeps task
     * in R14 across the range check (MOV R4,R14 at 0x0c914788). without it,
     * SHC keeps task on the stack, the step runs 2 bytes longer, and 12 of
     * its 16 words differ */
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        gold_coin_init(task);
    }
}

static void gold_coin_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        collision_init(task, gold_coin_shape, COLLISION_OBJECT, 1, 1);
    }
    task->draw = gold_coin_draw;
    task->step = gold_coin_run;
    task->destructor = gold_coin_destroy;
    work->timer = 0;
    work->mode = 0;
    VARS(work)->view_radius = EDIT_VIEW_RADIUS;
    if (!placement_editing(task)) {
        VARS(work)->view_radius = (work->scl.x + 1.0F) * VIEW_RADIUS;
    }
}

static void gold_coin_destroy(struct task *task)
{
}

static void gold_coin_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        gold_coin_draw(task);
        VARS(work)->spin += gold_coin_turn;
        if (!placement_editing(task)) {
            work->collision->shapes->ang.x = VARS(work)->spin + EDGE_ON;
            work->collision->shapes->radius =
                gold_coin_shape[0].radius * (work->scl.x + 1.0F);
            work->collision->shapes->size_y =
                gold_coin_shape[0].size_y * (work->scl.y + 1.0F);
            collision_register(work);
            /* @hack spelling: the test for the wrap to 0, written <= 0 for the
             * image's CMP/PL (0x0c9148d0). the decrement undoes the wrap, which
             * holds the timer at 65535. as == 0, the test is TST, and 2 of
             * gold_coin_run's 65 words differ */
            if (++TASK_TIMER(work) <= 0) {
                --TASK_TIMER(work);
            }
        }
    }
}

static void gold_coin_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &gold_coin_object;
    Angle ang;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                      VARS(work)->view_radius)) {
        return;
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    draw_attr_save();
    draw_attr_clear(0, NJD_FLAG_IGNORE_SPECULAR);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y + VARS(work)->spin, ang);
    TURN_VIA(njRotateX, work->ang.x + EDGE_ON, ang);
    njScale(NULL, work->scl.x + 1.0F, work->scl.y + 1.0F, work->scl.x + 1.0F);
    draw_model(object->model);
    njPopMatrix(1);
    draw_attr_restore();
}
