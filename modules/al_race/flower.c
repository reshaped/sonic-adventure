/* WHITE FLOWER and PURPLE FLOWER, two flowers whose tasks only draw them */
#include <ninja.h>

#include "al_race/flower.h"
#include "draw/model.h"
#include "object/placement.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

/* the white flower and the purple flower */
extern NJS_OBJECT dat_0cbc1730;
extern NJS_OBJECT dat_0cbc0fd0;

/* a flower's state, in its task's motion: its model */
#define FLOWER(task) ((NJS_MODEL **)(task)->motion)

static void flower_start(struct task *task, void *arg);
static void flower_draw(struct task *task);

static const struct task_desc flower_desc = {sizeof(NJS_MODEL *), TASK_WORK,
                                             flower_start,        flower_draw,
                                             flower_draw,         NULL};
static NJS_OBJECT *const white_flower_object = &dat_0cbc1730;
static NJS_OBJECT *const purple_flower_object = &dat_0cbc0fd0;

static void flower_turn_x(struct angles *ang)
{
    Angle turn;

    TURN_VIA(njRotateX, ang->x, turn);
    /* @hack redundant: turn cleared after the rotation, which keeps
     * flower_turn_x a function of its own, called by BSR at 0x0cb97358;
     * without it, SHC expands it in flower_draw, and 52 of flower_draw's 52
     * words differ */
    turn = 0; /* NOLINT(clang-analyzer-deadcode.DeadStores) */
}

static void flower_turn_y(struct angles *ang)
{
    Angle turn;

    TURN_VIA(njRotateY, ang->y, turn);
    /* @hack redundant: turn cleared after the rotation, which keeps
     * flower_turn_y a function of its own, called by BSR at 0x0cb9735c;
     * without it, SHC expands it in flower_draw, and 52 of flower_draw's 52
     * words differ */
    turn = 0; /* NOLINT(clang-analyzer-deadcode.DeadStores) */
}

static void flower_turn_z(struct angles *ang)
{
    Angle turn;

    TURN_VIA(njRotateZ, ang->z, turn);
    /* @hack redundant: turn cleared after the rotation, which keeps
     * flower_turn_z a function of its own, called by BSR at 0x0cb97354;
     * without it, SHC expands it in flower_draw, and 52 of flower_draw's 52
     * words differ */
    turn = 0; /* NOLINT(clang-analyzer-deadcode.DeadStores) */
}

void white_flower_init(struct task *task)
{
    task_apply_desc(task, &flower_desc, (void *)&white_flower_object);
}

void purple_flower_init(struct task *task)
{
    task_apply_desc(task, &flower_desc, (void *)&purple_flower_object);
}

static void flower_start(struct task *task, void *arg)
{
    NJS_MODEL **model = FLOWER(task);

    *model = (*(NJS_OBJECT **)arg)->model;
}

static void flower_draw(struct task *task)
{
    if (frame_step == 0 && !placement_far_from_camera(task)) {
        struct task_work *work = task->work;
        NJS_MODEL **model = FLOWER(task);
        struct angles *ang = &work->ang;

        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        flower_turn_z(ang);
        flower_turn_x(ang);
        flower_turn_y(ang);
        texture_set_stage();
        draw_model(*model);
        njPopMatrix(1);
    }
}
