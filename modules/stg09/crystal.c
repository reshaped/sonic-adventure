/* O CRYSTAL: a crystal that turns about y while in view, with a solid
 * sphere */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg09/casino.h"
#include "stg09/crystal.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT casino_crystal_object;

/* the task destroys itself beyond about 600 of the camera and player 0.
 * placement_try_leave_by takes the range squared */
#define RANGE ((600.0F * 600.0F) + 10.0F)
/* draw_in_view tests a sphere of this radius about the crystal. the crystal
 * turns and registers its sphere only while that sphere is in view */
#define VIEW_RADIUS 20.0F

struct casino_crystal_vars {
    Angle turned; /* its turn so far about y, on top of its y angle */
};

#define VARS(work) ((struct casino_crystal_vars *)&(work)->vars)

static Angle casino_crystal_turn = 336; /* the crystal's turn each step */
static struct collision_shape casino_crystal_shape = {
    SHAPE_SPHERE | SHAPE_UNTURNED,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 10.0F, 0.0F},
    10.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static void casino_crystal_init(struct task *task);
static void casino_crystal_destroy(struct task *task);
static void casino_crystal_run(struct task *task);
static void casino_crystal_draw(struct task *task);

void casino_crystal_step(struct task *task)
{
    /* @hack redundant: work, read and never used, which keeps task in R14
     * across the range check (MOV R4,R14 at 0x0c90b9c8). without it, 12 of
     * 16 words differ */
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        casino_crystal_init(task);
    }
}

static void casino_crystal_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        work->ang.x = 0;
        work->ang.z = 0;
        collision_init(task, &casino_crystal_shape, COLLISION_OBJECT, 1, 1);
    }
    task->draw = casino_crystal_draw;
    task->step = casino_crystal_run;
    task->destructor = casino_crystal_destroy;
    work->timer = 0;
    work->mode = 0;
}

static void casino_crystal_destroy(struct task *task)
{
}

static void casino_crystal_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE) &&
        draw_in_view(work->pos.x, work->pos.y, work->pos.z, VIEW_RADIUS)) {
        casino_crystal_draw(task);
        VARS(work)->turned += casino_crystal_turn;
        if (!placement_editing(task)) {
            collision_register(work);
            /* @hack spelling: the test for the wrap to 0, written <= 0 for
             * the image's CMP/PL (0x0c90ba80). the timer is unsigned, so the
             * test holds at 0 alone, and the decrement holds the timer at
             * 65535. as == 0, the test is TST, and 2 of 51 words differ */
            if (++TASK_TIMER(work) <= 0) {
                --TASK_TIMER(work);
            }
        }
    }
}

static void casino_crystal_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &casino_crystal_object;
    Angle ang;

    if (!placement_try_leave_by(task, RANGE)) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y + VARS(work)->turned, ang);
        draw_model(object->model);
        njPopMatrix(1);
    }
}
