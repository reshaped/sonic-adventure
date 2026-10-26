/* O WSR2 water spray, which puffs water smoke as O WSR does, in spells
 * between rests */
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

/* O WSR2 is destroyed beyond about 250 units of the camera and player 0.
 * RANGE is that distance squared */
#define RANGE 62510.0F

struct water_spray2_vars {
    Uint16 countdown; /* frames left in the spell or rest, after this one */
};

#define VARS(work) ((struct water_spray2_vars *)&(work)->vars)
#define SPRAYING(work) ((work)->submode)
#define INTERVAL(work) ((Uint8)(work)->ang.x)
#define SPEED(work) ((work)->scl.x)
#define SPREAD(work) ((work)->scl.y)
#define SIZE(work) ((work)->scl.z)

/* each spell and each rest lasts from 1 frame to this many, at random */
static Sint32 water_spray2_spell_frames = 120;
static Sint32 water_spray2_rest_frames = 120;
/* every puff a spray makes keeps a pointer to this and moves by its rates.
 * a new puff first overwrites the scale, pos and speed it starts with */
static struct dust_params water_spray2_puff = {
    3.0F,
    0.003F,
    2.0F,
    0.0F,
    0.0F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 0.3F, 0.2F, 0.1F},
};

static void water_spray2_run(struct task *task)
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
    /* @hack spelling: the test for 0, written <= 0 for the image's CMP/PL
     * (0x0c91e5c2). countdown is a Uint16, so <= 0 holds only at 0. as == 0,
     * the test is TST, and 2 of 265 words differ */
    if (VARS(work)->countdown-- <= 0) {
        if (SPRAYING(work)) {
            VARS(work)->countdown =
                rand() * (Float)water_spray2_rest_frames * RAND_SCALE;
            SPRAYING(work) = FALSE;
        } else {
            VARS(work)->countdown =
                rand() * (Float)water_spray2_spell_frames * RAND_SCALE;
            SPRAYING(work) = TRUE;
        }
    }
    if (SPRAYING(work)) {
        if (TASK_TIMER(work)++ > INTERVAL(work)) {
            /* @hack spelling: the cursor taken as (&speed)->elements, whose
             * address SHC forms from R15 in R4 (0x0c91e626 to 0x0c91e62a); as
             * speed.elements, 247 of 265 words differ, and as member stores,
             * 152 */
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
            water_spray2_puff.scale = SIZE(work) + 0.1F;
            water_spray2_puff.pos = work->pos;
            water_spray2_puff.speed = speed.vector;
            water_smoke_create_from(&water_spray2_puff);
            work->timer = 0;
        }
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

void water_spray2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        work->timer = 0;
        VARS(work)->countdown = 0;
        SPRAYING(work) = TRUE;
        task->step = water_spray2_run;
    }
}
