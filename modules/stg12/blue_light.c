/* O BLUELIGHT, a light that loops its action and turns about y to face the
 * camera */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "stg12/blue_light.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: the action declared as an array, so SHC pools only its address,
 * at 0x0c913aec, and reads its motion through it at 0x0c913a32. declared as
 * one, SHC pools the motion field's address too. then 6 of the 103 words of
 * shelter_blue_light_run and shelter_blue_light_draw differ, as do 7 of the 32
 * words of their pool and pad */
extern NJS_ACTION shelter_blue_light_action[];

struct shelter_blue_light_vars {
    Float frame;
};

#define VARS(work) ((struct shelter_blue_light_vars *)&(work)->vars)

static void shelter_blue_light_run(struct task *task);
static void shelter_blue_light_draw(struct task *task);

void shelter_blue_light_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->frame = 0.0F;
    task->step = shelter_blue_light_run;
    task->draw = shelter_blue_light_draw;
}

static void shelter_blue_light_run(struct task *task)
{
    struct task_work *work = task->work;
    Float last;
    Float x;
    Float z;

    if (!placement_try_leave(task)) {
        VARS(work)->frame += 1.0F;
        last = (Float)(shelter_blue_light_action->motion->nbFrame - 1);
        if (VARS(work)->frame >= last) {
            VARS(work)->frame -= last;
        }
        x = camera_work->pos.x - work->pos.x;
        z = camera_work->pos.z - work->pos.z;
        work->ang.y = (Angle)(atan2f(x, z) * ANGLE_PER_RADIAN);
        shelter_blue_light_draw(task);
    }
}

static void shelter_blue_light_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(shelter_blue_light_action, VARS(work)->frame);
        njPopMatrix(1);
    }
}
