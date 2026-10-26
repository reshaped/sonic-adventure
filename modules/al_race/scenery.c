/* the Chao race's scenery task, whose children are the act's fixtures */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_race/ball.h"
#include "al_race/coconut_palm.h"
#include "al_race/course_model.h"
#include "al_race/cycle.h"
#include "al_race/deal.h"
#include "al_race/door.h"
#include "al_race/game.h"
#include "al_race/gate.h"
#include "al_race/lamps.h"
#include "al_race/scenery.h"
#include "al_race/sign.h"
#include "al_race/stream.h"
#include "effect/flare.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/task_desc.h"

static void scenery_init(struct task *task, void *arg);
static void scenery_step(struct task *task);

static const struct task_desc scenery_desc = {
    0, 0, scenery_init, scenery_step, NULL, NULL};
static const struct angles scenery_no_turn = {0, 0, 0};
static const NJS_POINT3 scenery_unit = {1.0F, 1.0F, 1.0F};
/* the position of the light behind the course's lens flare */
static const NJS_POINT3 scenery_flare = {8.0F, 633.0F, 638.0F};

static struct task *scenery_task = NULL;

void scenery_create(void)
{
    /* @hack redundant: task, set and never read, which keeps the call a JSR
     * and an RTS (0x0cb8bee8, 0x0cb8beee); as a bare call, SHC jumps to
     * task_create_desc, and 9 of 9 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    struct task *task;

    task = task_create_desc(3, &scenery_desc, NULL);
}

void scenery_destroy(void)
{
    if (scenery_task != NULL) {
        task_kill(scenery_task);
        scenery_task = NULL;
    }
}

struct task *scenery_add(const struct task_desc *desc, void *arg)
{
    struct task *parent = scenery_task;
    struct task *task;

    task = parent == NULL ? NULL : task_create_desc_child(parent, desc, arg);
    return task;
}

void scenery_place(struct task_work *work, NJS_OBJECT *object)
{
    NJS_POINT3 *pos = &work->pos;
    Float *from = object->pos;

    pos->x = from[0];
    pos->y = from[1];
    pos->z = from[2];
    work->ang = scenery_no_turn;
    work->scl = scenery_unit;
}

static void scenery_init(struct task *task, void *arg)
{
    Uint32 stage_act;
    int kind;
    int open;

    scenery_task = task;
    stage_act = stage_get_stage_act();
    switch ((Uint8)stage_act) {
    case ACT_HALL:
        door_create();
        gates_create();
        sign_create();
        hall_lamps_create();
        break;
    case ACT_COURSE:
        streams_create();
        course_lamps_create();
        cycles_create();
        course_model_create();
        scenery_add(&flare_desc, (void *)&scenery_flare);
        /* @bug uninitialized: race_get_kind's result goes untested, so without
         * a race the switch reads kind unset, from sp+0 at 0x0cb8bfce */
        race_get_kind(&kind);
        open = 0;
        switch (kind) {
        case 0:
            ball_create();
            break;
        case 1:
            open = 1;
            break;
        case 2:
        case 3:
            break;
        case 4:
            open = 1;
            coconut_palm_create();
            break;
        }
        deal_create(open);
        break;
    }
}

static void scenery_step(struct task *task)
{
    task_step_children(task);
}
