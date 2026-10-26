/* O WSR water spray, a puff of water smoke every few frames */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "debug/editor.h"
#include "draw/draw.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "stg09/water_smoke.h"
#include "stg09/water_spray.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* O WSR is destroyed beyond about 250 units of the camera and player 0.
 * RANGE is that distance squared */
#define RANGE 62510.0F

#define INTERVAL(work) ((Uint8)(work)->ang.x)
#define SPEED(work) ((work)->scl.x)
#define SPREAD(work) ((work)->scl.y)
#define SIZE(work) ((work)->scl.z)

/* every puff a spray makes keeps a pointer to this and moves by its rates.
 * a new puff first overwrites the scale, pos and speed it starts with */
static struct dust_params water_spray_puff = {
    0.3F,
    0.001F,
    0.07F,
    0.06F,
    0.002F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.2F, 1.0F, 1.0F, 1.0F},
};

static void water_spray_run(struct task *task)
{
    struct task_work *work = task->work;
    union point_floats speed;
    Angle ang;
    Float spread;
    Float *at;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (frame_step) {
        return;
    }
    if (TASK_TIMER(work)++ > INTERVAL(work)) {
        /* @hack spelling: the cursor taken as (&speed)->elements, whose address
         * SHC forms from R15 in R4 (0x0c91e326 to 0x0c91e32a); as
         * speed.elements, 200 of 220 words differ, and as member stores,
         * 107 */
        at = (&speed)->elements;
        *at++ = 0.0F;
        *at++ = SPEED(work);
        *at = 0.0F;
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        njCalcVector(NULL, &speed.vector, &speed.vector);
        njPopMatrix(1);
        spread = SPREAD(work) * 2.0F;
        speed.vector.x += spread * ((rand() * RAND_SCALE) - 0.5F);
        speed.vector.y += spread * ((rand() * RAND_SCALE) - 0.5F);
        speed.vector.z += spread * ((rand() * RAND_SCALE) - 0.5F);
        water_spray_puff.scale = SIZE(work) + 0.1F;
        water_spray_puff.pos = work->pos;
        water_spray_puff.speed = speed.vector;
        water_smoke_create_from(&water_spray_puff);
        work->timer = 0;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_X), "-> create pitch");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_Y), "-> angy");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_ANG_Z), "-> angz");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> Spd");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "-> Spread Spd");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "-> scale");
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        draw_line(0.0F, 0.0F, 0.0F, 0.0F, 20.0F, 0.0F);
        njPopMatrix(1);
    }
}

void water_spray_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        work->timer = 0;
        task->step = water_spray_run;
    }
}
