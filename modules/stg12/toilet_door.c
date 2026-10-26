/* O TOILE_DOOR, a door that an attack swings a quarter turn open */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg12/toilet_door.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT toilet_door_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

/* the door turns SWING_SPEED each step for SWING_STEPS steps, a quarter
 * turn in all */
#define SWING_SPEED 0x400
#define SWING_STEPS 16

#define SE_TOILET_DOOR 27

enum toilet_door_mode { DOOR_INIT, DOOR_SHUT, DOOR_SWINGING, DOOR_OPEN };

static void toilet_door_init(struct task *task);
static void toilet_door_wait_hit(struct task *task);
static void toilet_door_draw(struct task *task);
static void toilet_door_destroy(struct task *task);

static struct collision_shape toilet_door_shape = {
    SHAPE_RECTANGLE | SHAPE_HURTABLE,
    SHAPE_HURTS_PLAYERS,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 11.0F, 12.0F},
    0.0F,
    14.0F,
    12.0F,
    {0},
    0,
    NULL,
};

void toilet_door_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case DOOR_INIT:
            toilet_door_init(task);
            break;
        case DOOR_SHUT:
            toilet_door_draw(task);
            toilet_door_wait_hit(task);
            break;
        case DOOR_SWINGING:
            toilet_door_draw(task);
            work->ang.y -= SWING_SPEED;
            if (--TASK_TIMER(work) <= 0) {
                work->mode = DOOR_OPEN;
            }
            collision_register(task->work);
            break;
        case DOOR_OPEN:
            toilet_door_draw(task);
            collision_register(task->work);
            break;
        }
    } else {
        switch (work->mode) {
        case DOOR_INIT:
            toilet_door_init(task);
            break;
        case DOOR_SHUT:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            toilet_door_draw(task);
            toilet_door_wait_hit(task);
            break;
        }
    }
}

static void toilet_door_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = toilet_door_draw;
    task->destructor = toilet_door_destroy;
    collision_init(task, &toilet_door_shape, COLLISION_OBJECT, 1, 1);
    work->mode = DOOR_SHUT;
}

static void toilet_door_wait_hit(struct task *task)
{
    struct task_work *work = task->work;

    if (work->flags & COLLISION_DAMAGED) {
        TASK_TIMER(work) = SWING_STEPS;
        work->mode = DOOR_SWINGING;
        sound_play(SE_TOILET_DOOR, 0, 0, 0);
    }
    collision_register(task->work);
}

static void toilet_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&toilet_door_object);
        njPopMatrix(1);
    }
}

static void toilet_door_destroy(struct task *task)
{
}
