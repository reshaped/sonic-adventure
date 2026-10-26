/* PALM, a palm tree that is only drawn */
#include <ninja.h>

#include "al_race/palm.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

/* the palm */
extern NJS_OBJECT dat_0cbd0f40;

/* the palm's state, in its task's motion: its object */
#define PALM(task) ((NJS_OBJECT **)(task)->motion)

static void race_palm_start(struct task *task, void *arg);
static void race_palm_draw(struct task *task);

static const struct task_desc race_palm_desc = {
    sizeof(NJS_OBJECT *), TASK_WORK,      race_palm_start,
    race_palm_draw,       race_palm_draw, NULL};
static NJS_OBJECT *const race_palm_object = &dat_0cbd0f40;

void race_palm_init(struct task *task)
{
    task_apply_desc(task, &race_palm_desc, (void *)&race_palm_object);
}

static void race_palm_start(struct task *task, void *arg)
{
    NJS_OBJECT **object = PALM(task);

    *object = *(NJS_OBJECT **)arg;
}

static void race_palm_draw(struct task *task)
{
    struct task_work *work;
    NJS_OBJECT **object;
    struct angles *ang;
    Angle turn;

    if (frame_step == 0) {
        if (!placement_far_from_camera(task)) {
            work = task->work;
            object = PALM(task);
            njPushMatrix(NULL);
            njTranslateV(NULL, &work->pos);
            ang = &work->ang;
            TURN_VIA(njRotateY, ang->y, turn);
            texture_set_stage();
            draw_tree(*object);
            njPopMatrix(1);
        }
    }
}
