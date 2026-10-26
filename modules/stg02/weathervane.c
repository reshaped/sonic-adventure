/* O KAZAMI: a solid weathervane that turns to random headings as its propeller
 * spins */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg02/weathervane.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT windy_weathervane_object;
extern NJS_OBJECT windy_weathervane_head_object;
extern NJS_OBJECT windy_weathervane_propeller_object;

/* the weathervane's work->vars, in degrees. TASK_TIMER(work) counts the steps
 * the head holds its heading */
struct weathervane_vars {
    Float spin; /* the propeller's angle */
    Float heading; /* the head's angle about y */
    Float target; /* the heading the head turns to */
};

#define VARS(work) ((struct weathervane_vars *)&(work)->vars)

/* in VANE_END each step does nothing, and no step here sets it */
enum weathervane_mode { VANE_START, VANE_HOLD, VANE_TURN, VANE_END };

/* the head holds each heading until the timer passes HOLD_STEPS */
#define HOLD_STEPS 100

/* nothing reads it. it is the image's data word ahead of
 * windy_weathervane_shape */
static Float dat_0ca13f30 = 100.0F;

static struct collision_shape windy_weathervane_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    10.0F,
    50.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};

static void windy_weathervane_draw(struct task *task);

void windy_weathervane_step(struct task *task)
{
    struct task_work *work = task->work;
    Float speed;
    Float heading;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case VANE_START:
        work->mode = VANE_HOLD;
        task->draw = windy_weathervane_draw;
        VARS(work)->heading = 0.0F;
        collision_init(task, &windy_weathervane_shape, COLLISION_OBJECT, 1, 1);
        break;
    case VANE_HOLD:
        VARS(work)->spin += work->scl.x;
        if (TASK_TIMER(work)++ > HOLD_STEPS) {
            work->timer = 0;
            VARS(work)->target = rand() * (360.0F * RAND_SCALE);
            work->mode = VANE_TURN;
        }
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- ROTATE_SPEED");
        }
        windy_weathervane_draw(task);
        collision_register(work);
        break;
    case VANE_TURN:
        VARS(work)->spin += work->scl.x;
        if (VARS(work)->target > VARS(work)->heading) {
            speed = VARS(work)->target / (VARS(work)->heading + 0.1F) *
                    work->scl.x * 0.1F;
        } else {
            speed = VARS(work)->heading / (VARS(work)->target + 0.1F) *
                    work->scl.x * 0.1F;
        }
        ang = angle_approach(VARS(work)->heading * ANGLE_PER_DEGREE,
                             VARS(work)->target * ANGLE_PER_DEGREE,
                             speed * ANGLE_PER_DEGREE);
        heading = ang * (360.0F / 65536.0F);
        /* @hack spelling: a select in place of an if, for the hold once the
         * head stops turning. it loads the mode's 1 at its store (MOV #1,R2 at
         * 0x0c90be5e). as an if, SHC keeps the 1 in R12, a saved register, and
         * 149 of 162 words differ */
        VARS(work)->heading == heading ? work->mode = VANE_HOLD : 0;
        VARS(work)->heading = heading;
        windy_weathervane_draw(task);
        collision_register(work);
        break;
    case VANE_END:
        break;
    }
}

static void windy_weathervane_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *post = &windy_weathervane_object;
    NJS_OBJECT *head = &windy_weathervane_head_object;
    NJS_OBJECT *propeller = &windy_weathervane_propeller_object;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_model(post->model);
    njPushMatrix(NULL);
    njTranslate(NULL, head->pos[0], head->pos[1], head->pos[2]);
    TURN(njRotateY, (Angle)(VARS(work)->heading * ANGLE_PER_DEGREE));
    draw_model(head->model);
    njPushMatrix(NULL);
    njTranslate(NULL, propeller->pos[0], propeller->pos[1], propeller->pos[2]);
    TURN(njRotateX, (Angle)(VARS(work)->spin * ANGLE_PER_DEGREE));
    draw_model(propeller->model);
    njPopMatrix(1);
    njPopMatrix(1);
    njPopMatrix(1);
}
