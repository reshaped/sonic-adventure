/* O SUB TV, a solid television whose two screens show static */
#include <ninja.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg12/sub_tv.h"
#include "stg12/texlists.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT sub_tv_object;
extern NJS_OBJECT sub_tv_screen_1_object;
extern NJS_OBJECT sub_tv_screen_2_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

/* each screen strip's UVs, one at each corner */
#define SCREEN_CORNERS 4

enum sub_tv_mode { TV_INIT, TV_RUN };

static void sub_tv_init(struct task *task);
static void sub_tv_draw(struct task *task);
static void sub_tv_destroy(struct task *task);
static void sub_tv_static(struct task_work *work);

static struct collision_shape sub_tv_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    12.0F,
    8.0F,
    8.0F,
    {0},
    0,
    NULL,
};

/* @hack spelling: collision_register through an inline function, which reads
 * task->work again at each call (MOV.L @(32,R4),R4 at 0x0c905746 and
 * 0x0c90578a). called with sub_tv_step's own work, the step saves R12 and
 * jumps to collision_register. it then runs 16 bytes longer, and 69 of its
 * 74 words differ */
static void sub_tv_register(struct task *task)
{
    struct task_work *work = task->work;

    collision_register(work);
}

void sub_tv_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case TV_INIT:
            sub_tv_init(task);
            break;
        case TV_RUN:
            sub_tv_static(work);
            sub_tv_draw(task);
            sub_tv_register(task);
            break;
        }
    } else {
        switch (work->mode) {
        case TV_INIT:
            sub_tv_init(task);
            break;
        case TV_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            sub_tv_draw(task);
            sub_tv_register(task);
            break;
        }
    }
}

static void sub_tv_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = sub_tv_draw;
    task->destructor = sub_tv_destroy;
    collision_init(task, &sub_tv_shape, COLLISION_OBJECT, 1, 1);
    work->mode = TV_RUN;
}

static void sub_tv_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&hotshelter2_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&sub_tv_object);
        njPopMatrix(1);
    }
}

static void sub_tv_destroy(struct task *task)
{
}

static void sub_tv_static(struct task_work *work)
{
    NJS_TEX corners[SCREEN_CORNERS] = {
#include "stg12/sub_tv/corners.inc"
    };
    NJS_COLOR *uv;
    int i;
    int drop;

    uv = sub_tv_screen_2_object.model->meshsets[0].vertuv;
    rand();
    drop = (int)(rand() * (256.0F * RAND_SCALE));
    for (i = 0; i < SCREEN_CORNERS; i++) {
        uv[i].tex.u = corners[i].u;
        uv[i].tex.v = corners[i].v + drop;
    }
    uv = sub_tv_screen_1_object.model->meshsets[0].vertuv;
    rand();
    drop = (int)(rand() * (256.0F * RAND_SCALE));
    for (i = 0; i < SCREEN_CORNERS; i++) {
        uv[i].tex.u = corners[i].u;
        uv[i].tex.v = corners[i].v + drop;
    }
}
