/* OGL, a humming yellow engine whose action turns its tanks and pumps its
 * pistons */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg05/machine.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_ACTION mountain_machine_action;

struct machine_vars {
    Float frame;
};

#define VARS(work) ((struct machine_vars *)&(work)->vars)

/* no step sets MACHINE_GONE, in which the step kills its placement */
enum machine_mode { MACHINE_INIT, MACHINE_RUN, MACHINE_GONE };

/* the action starts over at 0 once it reaches frame MACHINE_FRAMES. each draw
 * moves it on MACHINE_SPEED frames */
#define MACHINE_FRAMES 59.0F
#define MACHINE_SPEED 0.5F

/* the machine's hum, which each step refreshes */
#define SE_MACHINE 133

/* around the machine's body, tanks and other parts */
static struct collision_shape mountain_machine_shapes[5] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 9.0F, 0.0F},
     12.0F,
     13.0F,
     12.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 47.0F, -10.0F},
     5.0F,
     5.0F,
     18.0F,
     {0},
     0,
     NULL},
    {SHAPE_CAPSULE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {15.0F, 37.0F, 0.0F},
     8.0F,
     5.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-11.0F, 23.0F, 0.0F},
     1.3F,
     4.0F,
     7.0F,
     {0},
     0,
     NULL},
    {SHAPE_CAPSULE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-2.5F, 5.0F, -16.0F},
     13.4F,
     3.0F,
     0.0F,
     {0},
     0,
     NULL},
};

static void mountain_machine_draw(struct task *task);
static void mountain_machine_destroy(struct task *task);

void mountain_machine_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case MACHINE_INIT:
        if (!placement_editing(task)) {
            collision_init(task, mountain_machine_shapes, COLLISION_OBJECT, 1,
                           5);
        }
        task->destructor = mountain_machine_destroy;
        task->draw = mountain_machine_draw;
        VARS(work)->frame = 0.0F;
        work->mode = MACHINE_RUN;
        break;
    case MACHINE_RUN:
        if (!placement_editing(task)) {
            collision_register(work);
            object_clear_touch(work, NULL);
        }
        mountain_machine_draw(task);
        sound_play_at_for(SE_MACHINE, (int)work, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 10);
        break;
    case MACHINE_GONE:
        placement_kill(task);
        break;
    }
}

/* draw the machine while it is in view, and only then advance its action */
static void mountain_machine_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 200.0F)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(&mountain_machine_action, VARS(work)->frame);
        njPopMatrix(1);
        /* @hack spelling: the frame tested in the assignment, which compares
         * the sum in FR1 (0x0c906b1c, 0x0c906b1e); as two statements, the
         * frame is loaded again, the function is 2 bytes longer and 11 of 78
         * words differ */
        if ((VARS(work)->frame += MACHINE_SPEED) >= MACHINE_FRAMES) {
            VARS(work)->frame = 0.0F;
        }
    }
}

static void mountain_machine_destroy(struct task *task)
{
}
