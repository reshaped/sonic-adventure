/* O MOVA_THORN spiked roller that turns as it rises and falls */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg10/thorn_roller.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the frame the roller hangs from, the arm that lifts it, and the roller */
extern NJS_MODEL thorn_roller_base_model;
extern NJS_MODEL thorn_roller_lift_model;
extern NJS_MODEL thorn_roller_roller_model;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)
/* how far the collision record reaches from the roller's position */
#define REACH 200.0F

/* the roller's turn about x a step. work->timer adds it up, and the draw
 * reads the sum unsigned, through TASK_TIMER */
#define ROLL_STEP 0x400
#define LIFT_STEP 0x200 /* the lift's phase step, a turn each 128 steps */

/* the lift rises from 0 to LIFT_HEIGHT and back */
#define LIFT_HEIGHT 30.0F

#define ROLLER_DROP 147.0F /* how far below the lift the roller's axis lies */
#define ENDS_DROP 128.5F /* how far below the lift the arm's ends lie */

struct roller_vars {
    Uint16 phase; /* the lift's phase */
    Uint8 unk_02[2];
    Float lift; /* how far the lift has risen */
};

#define VARS(work) ((struct roller_vars *)&(work)->vars)

enum roller_mode { ROLLER_INIT, ROLLER_RUN };

static void thorn_roller_init(struct task *task);
static void thorn_roller_reset(struct task *task);
static void thorn_roller_run(struct task *task);
static void thorn_roller_draw(struct task *task);
static void thorn_roller_destroy(struct task *task);

/* the shapes, each centered below the roller's position. thorn_roller_run
 * raises those of the roller and the arm's ends with the lift */
static struct collision_shape thorn_roller_shapes[4] = {
    /* the frame */
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -35.0F, 0.0F},
     15.2F,
     38.0F,
     0.0F,
     {0},
     0,
     NULL},
    /* the roller, turned to lie along x. it hurts players, and no attack
     * exceeds its guard of 3 */
    {SHAPE_CYLINDER2 | SHAPE_HURTABLE,
     ATTACK(2) | GUARD(3) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -ROLLER_DROP, 0.0F},
     12.3F,
     18.0F,
     0.0F,
     {0, 0, 0x4000},
     0,
     NULL},
    /* the arm's two ends */
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {23.0F, -ENDS_DROP, 0.0F},
     8.0F,
     22.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-23.0F, -ENDS_DROP, 0.0F},
     8.0F,
     22.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* the set editor labels all three angles, though only angle y turns the
 * roller */
void thorn_roller_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case ROLLER_INIT:
            thorn_roller_init(task);
            break;
        case ROLLER_RUN:
            thorn_roller_draw(task);
            thorn_roller_run(task);
            break;
        }
    } else {
        switch (work->mode) {
        case ROLLER_INIT:
            thorn_roller_init(task);
            break;
        case ROLLER_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            thorn_roller_draw(task);
            thorn_roller_run(task);
            break;
        }
    }
}

static void thorn_roller_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = thorn_roller_draw;
    task->destructor = thorn_roller_destroy;
    thorn_roller_reset(task);
    work->mode = ROLLER_RUN;
}

static void thorn_roller_reset(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, thorn_roller_shapes, COLLISION_OBJECT, 1, 4);
    work->collision->radius = REACH;
    work->timer = 0;
    VARS(work)->phase = 0;
    VARS(work)->lift = 0.0F;
}

static void thorn_roller_run(struct task *task)
{
    struct task_work *work = task->work;

    work->timer += ROLL_STEP;
    VARS(work)->phase += LIFT_STEP;
    VARS(work)->lift = (njSin(VARS(work)->phase) + 1.0F) * (LIFT_HEIGHT / 2.0F);
    work->collision->shapes[1].center.y = VARS(work)->lift - ROLLER_DROP;
    work->collision->shapes[2].center.y = VARS(work)->lift - ENDS_DROP;
    work->collision->shapes[3].center.y = VARS(work)->lift - ENDS_DROP;
    collision_register(work);
}

static void thorn_roller_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(&thorn_roller_base_model);
        njTranslate(NULL, 0.0F, VARS(work)->lift, 0.0F);
        draw_model(&thorn_roller_lift_model);
        njTranslate(NULL, 0.0F, -ROLLER_DROP, 0.0F);
        TURN_VIA(njRotateX, TASK_TIMER(work), ang);
        draw_model(&thorn_roller_roller_model);
        njPopMatrix(1);
    }
}

static void thorn_roller_destroy(struct task *task)
{
}
