/* Ideacap, which shows the second table's next idea piece */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "stg09/casino.h"
#include "stg09/idea.h"
#include "stg09/pinball.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT ideacap_object_0;
extern NJS_OBJECT ideacap_object_1;
extern NJS_OBJECT ideacap_object_2;
extern NJS_OBJECT ideacap_object_3;
extern NJS_MOTION ideacap_motion;

/* ideacap_motion's last frame, where it starts over, and the frames it moves
 * on each step */
#define LAST_FRAME 15.0F
#define FRAME_RATE 0.1F

/* the next piece's model, by how many pieces are in place, from 1 */
static NJS_OBJECT *ideacap_objects[PINBALL_IDEAS - 1] = {
    &ideacap_object_0,
    &ideacap_object_1,
    &ideacap_object_2,
    &ideacap_object_3,
};

struct ideacap_vars {
    /* the object kind's param, the task's first var */
    void *param;
    Float frame; /* ideacap_motion's */
};

#define VARS(work) ((struct ideacap_vars *)&(work)->vars)

static void ideacap_destroy(struct task *task)
{
}

static void ideacap_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    /* @bug memory: with every piece in place, this reads the word past
     * ideacap_objects, at 0x0c907508. in the image, that is idea_objects[0],
     * so it draws Idea's first piece */
    draw_motion(ideacap_objects[pinball_ideas - 1], &ideacap_motion,
                VARS(work)->frame);
    njPopMatrix(1);
}

static void ideacap_run(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->frame += FRAME_RATE;
    if (VARS(work)->frame >= LAST_FRAME) {
        VARS(work)->frame = 0.0F;
    }
    ideacap_draw(task);
}

void ideacap_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->frame = 0.0F;
    task->step = ideacap_run;
    task->draw = ideacap_draw;
    task->destructor = ideacap_destroy;
}
