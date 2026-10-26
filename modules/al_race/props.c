/* START MARK, SMALL SCREEN and ISLET, props that are only drawn */
#include <ninja.h>

#include "al_race/props.h"
#include "draw/model.h"
#include "object/placement.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

/* the start mark, small screen and islet */
extern NJS_OBJECT dat_0cbc517c;
extern NJS_OBJECT dat_0cbc7cdc;
extern NJS_OBJECT dat_0cbc7904;

/* a prop's state, in its task's motion: its model */
#define PROP(task) ((NJS_MODEL **)(task)->motion)

static void prop_start(struct task *task, void *arg);
static void prop_draw(struct task *task);

static const struct task_desc prop_desc = {
    sizeof(NJS_MODEL *), TASK_WORK, prop_start, prop_draw, prop_draw, NULL};
static NJS_OBJECT *const start_mark_object = &dat_0cbc517c;
static NJS_OBJECT *const small_screen_object = &dat_0cbc7cdc;
static NJS_OBJECT *const islet_object = &dat_0cbc7904;

void start_mark_init(struct task *task)
{
    task_apply_desc(task, &prop_desc, (void *)&start_mark_object);
}

void small_screen_init(struct task *task)
{
    task_apply_desc(task, &prop_desc, (void *)&small_screen_object);
}

void islet_init(struct task *task)
{
    task_apply_desc(task, &prop_desc, (void *)&islet_object);
}

static void prop_start(struct task *task, void *arg)
{
    NJS_MODEL **model = PROP(task);

    *model = (*(NJS_OBJECT **)arg)->model;
}

static void prop_draw(struct task *task)
{
    struct task_work *work;
    NJS_MODEL **model;
    struct angles *ang;
    Angle turn;

    if (frame_step == 0) {
        if (!placement_far_from_camera(task)) {
            work = task->work;
            model = PROP(task);
            /* @hack redundant: turn takes ang.x and then ang.z, unread,
             * which leaves the model's stack slot at @(4,R15) and ang's at
             * @R15 (0x0cb97ea8, 0x0cb97eb0); without them, the slots trade
             * places, and 6 of 56 words differ */
            turn = work->ang.x; /* NOLINT(clang-analyzer-deadcode.DeadStores) */
            turn = work->ang.z; /* NOLINT(clang-analyzer-deadcode.DeadStores) */
            ang = &work->ang;
            njPushMatrix(NULL);
            njTranslateV(NULL, &work->pos);
            TURN_VIA(njRotateY, ang->y, turn);
            texture_set_stage();
            draw_model(*model);
            njPopMatrix(1);
        }
    }
}
