/* O Post1 and O Post4: wall posters that are only drawn */
#include <ninja.h>

#include "draw/tree.h"
#include "object/placement.h"
#include "stg04/wall_poster.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT wall_poster_1_object;
extern NJS_OBJECT wall_poster_4_object;

/* a poster's kind, which chooses its model. the kinds number the posters
 * from 0, and the object list holds only the first and the fourth */
enum wall_poster_kind { WALL_POSTER_1 = 0, WALL_POSTER_4 = 3 };

struct wall_poster_vars {
    enum wall_poster_kind kind;
};

#define VARS(work) ((struct wall_poster_vars *)&(work)->vars)

static void wall_poster_destroy(struct task *task);
static void wall_poster_run(struct task *task);

static void wall_poster_destroy(struct task *task)
{
    struct task_work *work = task->work;

    /* @hack redundant: a switch on the kind whose cases do nothing, which keeps
     * its compares (0x0c9090c4, 0x0c9090c8); without it, the function is an
     * RTS and its NOP, and 7 of 7 words differ */
    switch (VARS(work)->kind) {
    case WALL_POSTER_1:
    case WALL_POSTER_4:
        break;
    }
}

/* the poster's step, and its draw. out of range, it draws nothing, and the
 * task is destroyed on its next step */
static void wall_poster_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    switch (VARS(work)->kind) {
    case WALL_POSTER_1:
        texture_set_stage();
        draw_tree(&wall_poster_1_object);
        break;
    case WALL_POSTER_4:
        texture_set_stage();
        draw_tree(&wall_poster_4_object);
        break;
    }
    njPopMatrix(1);
}

void wall_poster_1_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->kind = WALL_POSTER_1;
    task->destructor = wall_poster_destroy;
    task->step = wall_poster_run;
    task->draw = wall_poster_run;
}

void wall_poster_4_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->kind = WALL_POSTER_4;
    task->destructor = wall_poster_destroy;
    task->step = wall_poster_run;
    task->draw = wall_poster_run;
}
