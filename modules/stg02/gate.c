/* O GATE: a solid gate whose sides spin about y and whose top spins about z */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg02/gate.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT windy_gate_object;
extern NJS_OBJECT windy_gate_side_a_object;
extern NJS_OBJECT windy_gate_side_b_object;
extern NJS_OBJECT windy_gate_top_object;

/* work's vars: the sides' angle about y and the top's about z, in degrees */
struct gate_vars {
    Float side_angle;
    Float top_angle;
};

#define VARS(work) ((struct gate_vars *)&(work)->vars)

/* the gate's modes. in GATE_STOP and GATE_END its step does nothing, and no
 * step here sets either */
enum gate_mode { GATE_START, GATE_RUN, GATE_STOP, GATE_END };

static struct collision_shape windy_gate_shapes[7] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 25.0F},
     2.0F,
     32.0F,
     2.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 32.0F, 0.0F},
     5.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 32.0F, 10.0F},
     5.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 32.0F, -10.0F},
     5.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 32.0F, 20.0F},
     5.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 32.0F, -20.0F},
     5.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, -25.0F},
     2.0F,
     32.0F,
     2.0F,
     {0, 0, 0},
     0,
     NULL},
};

static void windy_gate_draw(struct task *task);

void windy_gate_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case GATE_START:
        work->mode = GATE_RUN;
        task->draw = windy_gate_draw;
        collision_init(task, windy_gate_shapes, COLLISION_OBJECT, 1, 7);
        break;
    case GATE_RUN:
        VARS(work)->side_angle += work->scl.x;
        VARS(work)->top_angle += work->scl.y;
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- ROTATEY_SPEED");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- ROTATEZ_SPEED");
        }
        windy_gate_draw(task);
        collision_register(work);
        break;
    case GATE_STOP:
        break;
    case GATE_END:
        break;
    }
}

static void windy_gate_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *frame = &windy_gate_object;
    NJS_OBJECT *side_a = &windy_gate_side_a_object;
    NJS_OBJECT *side_b = &windy_gate_side_b_object;
    NJS_OBJECT *top = &windy_gate_top_object;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_model(frame->model);
    njPushMatrix(NULL);
    njTranslate(NULL, side_a->pos[0], side_a->pos[1], side_a->pos[2]);
    TURN(njRotateY, (Angle)(VARS(work)->side_angle * ANGLE_PER_DEGREE));
    draw_model(side_a->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, side_b->pos[0], side_b->pos[1], side_b->pos[2]);
    TURN(njRotateY, (Angle)(VARS(work)->side_angle * ANGLE_PER_DEGREE));
    draw_model(side_b->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, top->pos[0], top->pos[1], top->pos[2]);
    TURN(njRotateZ, (Angle)(VARS(work)->top_angle * ANGLE_PER_DEGREE));
    draw_model(top->model);
    njPopMatrix(1);
    njPopMatrix(1);
}
