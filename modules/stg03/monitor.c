/* O MONITOR screen with a scrolling picture, under a rocking speaker */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg03/monitor.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the monitor, whose child is the speaker */
extern NJS_OBJECT monitor_object;

/* the monitor's range for placement_try_leave_by, squared */
#define MONITOR_RANGE (1010.0F * 1010.0F)
/* how far the monitor's collision record reaches from its position */
#define MONITOR_REACH 20.0F

/* the speaker reverses once its turn passes SPEAKER_SWING either way */
#define SPEAKER_SWING NJM_DEG_ANG(10)
/* the picture's scroll returns to 0 once it passes SCROLL_END */
#define SCROLL_END 512
/* the screen's meshset in the monitor's model, and its count of uvs */
#define SCREEN_MESHSET 5
#define SCREEN_CORNERS 4

struct monitor_vars {
    NJS_OBJECT *object;
    Angle rate; /* the speaker's turn a step */
    Angle turn; /* the speaker's turn about x */
};

#define VARS(work) ((struct monitor_vars *)&(work)->vars)
/* the picture's scroll a step, and its scroll so far */
#define SCROLL_SPEED(work) ((work)->tick)
#define SCROLL(work) TASK_TIMER(work)

enum monitor_mode { MONITOR_INIT, MONITOR_RUN };
/* the speaker's rocking direction, in work->submode */
enum speaker_way { SPEAKER_UP, SPEAKER_DOWN };

static struct collision_shape monitor_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
    {0.0F, -18.0F, 2.0F},
    15.0F,
    18.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static void monitor_destroy(struct task *task)
{
    task_kill(task);
}

/* on a frame's first step, draw the monitor, then the speaker turned about x */
static void monitor_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = VARS(work)->object;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njTranslateV(NULL, (NJS_VECTOR *)object->pos);
        draw_model(object->model);
        njPushMatrix(NULL);
        object = object->child;
        njTranslateV(NULL, (NJS_VECTOR *)object->pos);
        TURN_VIA(njRotateX, VARS(work)->turn, ang);
        draw_model(object->model);
        njPopMatrix(1);
        njPopMatrix(1);
    }
}

/* rock the speaker, scroll the picture, and draw the monitor. every monitor
 * draws monitor_object, so the screen's uvs take the scroll only for the
 * draw */
static void monitor_animate(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack type: i a Uint16, which zero-extends it for each loop's test
     * (EXTU.W at 0x0c905db6 and 0x0c905df6); as an int, the tests move into
     * the bodies, the function runs 4 bytes shorter and 44 of 113 words
     * differ */
    Uint16 i;

    switch (work->submode) {
    case SPEAKER_UP:
        VARS(work)->turn += VARS(work)->rate;
        if (VARS(work)->turn > SPEAKER_SWING) {
            work->submode = SPEAKER_DOWN;
        }
        break;
    case SPEAKER_DOWN:
        VARS(work)->turn -= VARS(work)->rate;
        if (VARS(work)->turn < -SPEAKER_SWING) {
            work->submode = SPEAKER_UP;
        }
        break;
    }
    /* @hack spelling: the add inside the test, which tests the sum still in R0
     * (EXTU.W R0,R0 at 0x0c905d76); as a statement of its own, the test reads
     * the scroll again, the function runs 2 bytes longer and 73 of 113 words
     * differ */
    if ((SCROLL(work) += SCROLL_SPEED(work)) > SCROLL_END) {
        SCROLL(work) = 0;
    }
    for (i = 0; i < SCREEN_CORNERS; i++) {
        VARS(work)->object->model->meshsets[SCREEN_MESHSET].vertuv[i].tex.v -=
            SCROLL(work);
    }
    monitor_draw(task);
    for (i = 0; i < SCREEN_CORNERS; i++) {
        VARS(work)->object->model->meshsets[SCREEN_MESHSET].vertuv[i].tex.v +=
            SCROLL(work);
    }
}

/* in the set editor, the monitor animates but takes no collision record.
 * while the editor holds it, the step labels the set entry's fields */
void monitor_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, MONITOR_RANGE)) {
        return;
    }
    if (edit_mode) {
        VARS(work)->object = &monitor_object;
        SCROLL_SPEED(work) = (Uint8)floorf(work->scl.x);
        VARS(work)->rate = (Angle)(work->scl.y * ANGLE_PER_DEGREE);
        monitor_animate(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "> TEX SCROLL SPEED");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "> SPEAKER ROT RATE");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "> INIT SPEAKER ROT");
        }
    } else {
        switch (work->mode) {
        case MONITOR_INIT:
            work->mode = MONITOR_RUN;
            SCROLL_SPEED(work) = (Uint8)floorf(work->scl.x);
            VARS(work)->rate = (Angle)(work->scl.y * ANGLE_PER_DEGREE);
            VARS(work)->turn = (Angle)(work->scl.z * ANGLE_PER_DEGREE);
            VARS(work)->object = &monitor_object;
            task->destructor = monitor_destroy;
            task->draw = monitor_draw;
            collision_init(task, &monitor_shape, COLLISION_OBJECT, 1, 1);
            work->collision->radius = MONITOR_REACH;
            break;
        case MONITOR_RUN:
            monitor_animate(task);
            collision_register(work);
            object_clear_touch(work, NULL);
            break;
        }
    }
}
