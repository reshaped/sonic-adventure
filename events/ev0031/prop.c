/* EV0031.PRS's prop: a model of the event's own, standing still */
#include <ninja.h>

#include "draw/tree.h"
#include "ev0031/prop.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT ev0031_prop_object;

static void ev0031_prop_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_regular();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&ev0031_prop_object);
    njPopMatrix(1);
}

static void ev0031_prop_step(struct task *task)
{
    ev0031_prop_draw(task);
}

struct task *ev0031_prop_create(void)
{
    /* @hack spelling: task_create's result through a local, so SHC calls it
     * with JSR @R3 and then returns (0x0cb8060c). returned directly, SHC
     * jumps to it with JMP @R3, the object runs 8 bytes short, and 9 of
     * ev0031_prop_create's 9 words differ */
    struct task *task = task_create(TASK_WORK, 3, ev0031_prop_step);

    return task;
}
