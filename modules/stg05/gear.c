/* O GEAR, a great turning wheel that puffs dust along its top */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "stg05/gear.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_ACTION gear_action;

struct gear_vars {
    Float frame;
};

#define VARS(work) ((struct gear_vars *)&(work)->vars)

/* the step does nothing in GEAR_IDLE, which nothing in this file sets */
enum gear_mode { GEAR_INIT, GEAR_RUN, GEAR_IDLE };

/* the gear destroys itself once neither the camera nor player 0 is within
 * 1000 of it. player 1 as Sonic within 40 also keeps it. the range is
 * squared, as placement_try_leave_by takes it */
#define GEAR_LEAVE_RANGE (1000.0F * 1000.0F)

/* the action loops over GEAR_FRAMES frames. it advances GEAR_SPEED of a
 * frame each time the gear draws in view */
#define GEAR_FRAMES 49.0F
#define GEAR_SPEED 0.5F

/* the puffs' starting points, in the gear's own space */
static NJS_POINT3 gear_puff_points[8] = {
    {17.0F, 254.0F, -66.0F},  {17.0F, 254.0F, -81.0F},
    {17.0F, 254.0F, -102.0F}, {17.0F, 254.0F, -120.0F},
    {-27.0F, 254.0F, 97.0F},  {-27.0F, 254.0F, 114.0F},
    {-27.0F, 254.0F, 142.0F}, {-27.0F, 254.0F, 161.0F},
};

/* a box around the gear */
static struct collision_shape gear_shape = {SHAPE_RECTANGLE,
                                            0,
                                            PUSH_ALL | PUSH_FIRM(PUSH_ALL),
                                            {-50.0F, 85.0F, 0.0F},
                                            190.0F,
                                            155.0F,
                                            50.0F,
                                            {0, 0x4000, 0},
                                            0,
                                            NULL};

static void gear_puff(struct task *task);
static void gear_draw(struct task *task);
static void gear_destroy(struct task *task);

void gear_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, GEAR_LEAVE_RANGE)) {
        switch (work->mode) {
        case GEAR_INIT:
            VARS(work)->frame = 0.0F;
            task->destructor = gear_destroy;
            task->draw = gear_draw;
            work->mode = GEAR_RUN;
            break;
        case GEAR_RUN:
            gear_puff(task);
            gear_draw(task);
            break;
        case GEAR_IDLE:
            break;
        }
    }
}

/* a puff of dust from each of gear_puff_points, blowing along -x and up in
 * the gear's own space */
static void gear_puff(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 point;
    NJS_POINT3 pos;
    NJS_VECTOR blow = {-1.0F, 0.5F, 0.0F};
    NJS_VECTOR speed;
    Angle ang;
    int i;

    njPushMatrix(&_unit_matrix_);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    for (i = 0; i < 8; i++) {
        point = gear_puff_points[i];
        njCalcPoint(NULL, &point, &pos);
        njCalcVector(NULL, &blow, &speed);
        dust_create(&pos, &speed, 1.2F);
    }
    njPopMatrix(1);
}

/* draw the gear while it is in view, and only then advance its action */
static void gear_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 1500.0F)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(&gear_action, VARS(work)->frame);
        njPopMatrix(1);
        VARS(work)->frame += GEAR_SPEED;
        if (VARS(work)->frame >= GEAR_FRAMES) {
            VARS(work)->frame -= GEAR_FRAMES;
        }
    }
}

static void gear_destroy(struct task *task)
{
}
