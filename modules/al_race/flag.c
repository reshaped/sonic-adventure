/* FLAG, a flag that waves, from a random frame of its wave */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_race/flag.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

/* the flag's waving action */
extern NJS_ACTION dat_0cbd2278;

/* the flag's record, at its task's motion */
struct flag {
    Float frame; /* the action's frame it draws next, below last */
    Float last; /* the action's last frame */
    NJS_ACTION *action;
};

#define FLAG(task) ((struct flag *)(task)->motion)

/* the y turn of every flag, whatever its set entry's angle */
#define FLAG_TURN 0x7000

static void flag_start(struct task *task, void *arg);
static void flag_step(struct task *task);
static void flag_draw(struct task *task);

static const struct task_desc flag_desc = {
    sizeof(struct flag), TASK_WORK, flag_start, flag_step, flag_draw, NULL};
static NJS_ACTION *const flag_action = &dat_0cbd2278;

void flag_init(struct task *task)
{
    task_apply_desc(task, &flag_desc, (void *)&flag_action);
}

static void flag_start(struct task *task, void *arg)
{
    NJS_ACTION *action = *(NJS_ACTION **)arg;
    Float last = (Float)action->motion->nbFrame - 1.0F;
    struct flag *flag = FLAG(task);

    flag->frame = rand() * last * RAND_SCALE;
    flag->last = last;
    flag->action = action;
}

static void flag_step(struct task *task)
{
    struct flag *flag = FLAG(task);

    flag->frame += 1.0F;
    if (flag->frame >= flag->last) {
        flag->frame -= flag->last;
    }
    flag_draw(task);
}

static void flag_draw(struct task *task)
{
    struct task_work *work;
    struct flag *flag;

    if (frame_step == 0 && !placement_far_from_camera(task)) {
        work = task->work;
        flag = FLAG(task);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njRotateY(NULL, FLAG_TURN);
        texture_set_stage();
        draw_action(flag->action, flag->frame);
        njPopMatrix(1);
    }
}
