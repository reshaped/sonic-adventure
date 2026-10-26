/* O HBOX crate that an attack breaks into five boards */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv0100/crate.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* its first board is its child, and the other boards that child's siblings */
extern NJS_OBJECT crate_object;

#define CRATE_BOARDS 5
#define BOARD_STEPS 120 /* steps a board lasts */
/* steps after which a board skips its draw on odd steps, and so flickers */
#define BOARD_FLASH 60

#define BOARD_FALL 0.05F /* speed gravity adds to a board's fall each step */
#define BOARD_KEEP 0.92F /* fraction of its speed a board keeps each step */

/* a board's vars: the model it draws. TASK_TIMER(work) counts its steps */
struct board_vars {
    NJS_MODEL *model;
};

#define BOARD(work) ((struct board_vars *)&(work)->vars)

/* a board's own layout of its task_motion */
struct board_motion {
    void *unk_00;
    NJS_VECTOR speed;
    Uint8 unk_10[24];
    Angle spin_x;
    Angle spin_y;
    Angle spin_z;
};

enum crate_mode { CRATE_INIT, CRATE_RUN };

static void crate_draw(struct task *task);
static void crate_destroy(struct task *task);
static void crate_draw_work(struct task_work *work);
static void board_step(struct task *task);
static void crate_board_draw(struct task *task);
static void crate_break(struct task_work *work);

static struct collision_shape crate_shapes[] = {
    {SHAPE_RECTANGLE | SHAPE_HURTABLE,
     GUARD(1),
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 10.0F, 0.0F},
     10.0F,
     10.0F,
     10.0F,
     {0},
     0,
     NULL},
};

static struct dust_params crate_dust = {
    1.0F,
    0.1F,
    0.2F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 0.9F, 0.9F, 1.0F},
};

void egg_carrier_crate_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case CRATE_INIT:
        work->ang.x = 0;
        work->ang.z = 0;
        if (!placement_editing(task)) {
            collision_init(task, crate_shapes, COLLISION_OBJECT_MUTUAL, 1, 1);
            work->collision->radius = 40.0F;
        }
        task->destructor = crate_destroy;
        task->draw = crate_draw;
        work->mode = CRATE_RUN;
        break;
    case CRATE_RUN:
        if (work->flags & COLLISION_DAMAGED) {
            crate_break(work);
            placement_kill(task);
            return;
        }
        break;
    }
    crate_draw_work(work);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void crate_draw(struct task *task)
{
    struct task_work *work = task->work;

    crate_draw_work(work);
}

static void crate_destroy(struct task *task)
{
}

static void crate_draw_work(struct task_work *work)
{
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&crate_object);
    njPopMatrix(1);
}

static void board_step(struct task *task)
{
    struct board_motion *motion = (struct board_motion *)task->motion;
    struct task_work *work = task->work;
    Float x;
    Float y;
    Float z;
    Float px;
    Float py;
    Float pz;
    /* @hack spelling: each vector through a pointer, read forward and written
     * back backward, which keeps the walk as @R5+ loads and @-R5 stores
     * (0x0c9260ac to 0x0c9260d8); through the members, SHC indexes them
     * through R0, board_step runs 14 bytes long and 59 of 59 words differ */
    Float *walk = &motion->speed.x;

    x = *walk;
    walk++;
    y = *walk;
    walk++;
    z = *walk;
    walk++;
    /* @hack spelling: -BOARD_FALL added, an FADD of -0.05 from the pool
     * (0x0c9260b6); with -=, an FSUB of 0.05, and 1 of 59 words differ */
    y += -BOARD_FALL;
    x *= BOARD_KEEP;
    y *= BOARD_KEEP;
    z *= BOARD_KEEP;
    walk--;
    *walk = z;
    walk--;
    *walk = y;
    walk--;
    *walk = x;
    walk = &work->pos.x;
    px = *walk;
    walk++;
    py = *walk;
    walk++;
    pz = *walk;
    walk++;
    px += x;
    /* @hack redundant: a never-run negation of px, which keeps the image's
     * registers for px and the walk's loads (0x0c9260ac to 0x0c9260d2); without
     * it, FR5, FR6 and FR7 rotate and 12 of 59 words differ */
    if (0) {
        px = -px;
    }
    py += y;
    pz += z;
    walk--;
    *walk = pz;
    walk--;
    *walk = py;
    walk--;
    *walk = px;
    work->ang.x += motion->spin_x;
    work->ang.z += motion->spin_z;
    TASK_TIMER(work)++;
    if (TASK_TIMER(work) == BOARD_STEPS) {
        task_kill(task);
        return;
    }
    if (TASK_TIMER(work) > BOARD_FLASH && (TASK_TIMER(work) & 1)) {
        return;
    }
    crate_board_draw(task);
}

static void crate_board_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    draw_model(BOARD(work)->model);
    njPopMatrix(1);
}

/* break the crate into a task for each of its boards. each board's
 * direction is its position less the crate's, taken before njCalcPoint sets
 * that position. it is still 0 then, so all five fly the same way */
static void crate_break(struct task_work *work)
{
    NJS_OBJECT *object = &crate_object;
    struct task *task;
    struct task_work *board;
    struct board_motion *motion;
    NJS_VECTOR out;
    Angle ang;
    int i;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    object = object->child;
    for (i = 0; i < CRATE_BOARDS; i++, object = object->sibling) {
        task = task_create(TASK_WORK | TASK_MOTION, 6, board_step);
        if (task != NULL) {
            board = task->work;
            motion = (struct board_motion *)task->motion;
            out.x = board->pos.x - work->pos.x;
            out.y = board->pos.y - work->pos.y;
            out.z = board->pos.z - work->pos.z;
            njUnitVector(&out);
            out.x *= 3.0F;
            out.y *= 5.0F;
            out.z *= 3.0F;
            njCalcPoint(NULL, (NJS_POINT3 *)object->pos, &board->pos);
            njCalcVector(NULL, &out, &motion->speed);
            board->ang.y = work->ang.y;
            motion->spin_x = (int)(rand() * (2048.0F * RAND_SCALE)) - 1024;
            motion->spin_z = rand() * RAND_SCALE > 0.5F ? 1024 : -1024;
            crate_dust.scale = (rand() * RAND_SCALE) + 2.0F;
            crate_dust.pos = board->pos;
            crate_dust.speed.x = out.x * 0.1F;
            crate_dust.speed.y = out.y * 0.1F;
            crate_dust.speed.z = out.z * 0.1F;
            dust_create_from(&crate_dust);
            BOARD(board)->model = object->model;
        }
    }
    njPopMatrix(1);
}
