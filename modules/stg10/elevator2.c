/* O Elevator2 floor that carries a player down with the pads off */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg10/elevator2.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT elevator2_object;
extern NJS_OBJECT elevator2_world_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

#define DOWN_SPEED 1.0F /* units a step */
#define BOTTOM (-804.1F)

/* the collision world tests the floor while a player is this near */
#define NEAR 40.0F

/* the floor's y angle outside the set editor, whatever the set entry gives */
#define FACING (-35899)

#define SE_ELEVATOR2_RIDE 294

enum elevator2_mode { LIFT_INIT, LIFT_STILL, LIFT_WAIT, LIFT_DOWN, LIFT_DONE };

struct elevator2_vars {
    Float unk_00; /* init clears it, and the floor never reads it */
    Uint8 unk_04[4];
    NJS_OBJECT *floor; /* the floor's object in the collision world */
};

#define VARS(work) ((struct elevator2_vars *)&(work)->vars)
/* its speed carries what stands on the floor */
#define FORCE(task) ((struct task_force *)(task)->force)

static void elevator2_init(struct task *task);
static void elevator2_update(struct task *task);
static void elevator2_draw(struct task *task);
static void elevator2_down(struct task *task);
static void elevator2_add(struct task *task);
static void elevator2_destroy(struct task *task);
static void elevator2_wait(struct task *task);

void elevator2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case LIFT_INIT:
            elevator2_init(task);
            work->ang.y = FACING;
            work->mode = LIFT_WAIT;
            break;
        case LIFT_STILL:
            elevator2_draw(task);
            elevator2_update(task);
            break;
        case LIFT_WAIT:
            elevator2_draw(task);
            elevator2_wait(task);
            elevator2_update(task);
            break;
        case LIFT_DOWN:
            elevator2_draw(task);
            elevator2_down(task);
            elevator2_update(task);
            break;
        case LIFT_DONE:
            FORCE(task)->speed.y = 0.0F;
            elevator2_draw(task);
            break;
        }
    } else {
        switch (work->mode) {
        case LIFT_INIT:
            elevator2_init(task);
            work->mode = LIFT_STILL;
            break;
        case LIFT_STILL:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            elevator2_draw(task);
            elevator2_update(task);
            break;
        }
    }
}

static void elevator2_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = elevator2_draw;
    task->destructor = elevator2_destroy;
    elevator2_add(task);
    VARS(work)->unk_00 = 0.0F;
}

static void elevator2_update(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *floor;
    NJS_OBJECT *from;

    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    from = &elevator2_world_object;
    floor = VARS(work)->floor;
    floor->ang[0] = 0;
    floor->ang[1] = work->ang.y;
    floor->ang[2] = 0;
    floor->pos[0] = work->pos.x + from->pos[0];
    floor->pos[1] = work->pos.y + from->pos[1];
    floor->pos[2] = work->pos.z + from->pos[2];
}

static void elevator2_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&elevator2_object);
        njPopMatrix(1);
    }
}

static void elevator2_down(struct task *task)
{
    struct task_work *work = task->work;

    if (work->pos.y <= BOTTOM) {
        FORCE(task)->speed.y = 0.0F;
        work->mode = LIFT_DONE;
        pad_enable();
    } else {
        work->pos.y -= DOWN_SPEED;
        FORCE(task)->speed.y = -DOWN_SPEED;
        sound_play_at_for(SE_ELEVATOR2_RIDE, (int)task, 1, 0, work->pos.x,
                          work->pos.y, work->pos.z, 2);
    }
}

static void elevator2_add(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *from = &elevator2_world_object;
    NJS_OBJECT *floor;

    work->scl.x = 1.0F; /* the floor never reads scl.x */
    VARS(work)->floor = floor = world_object_alloc();
    floor->evalflags = from->evalflags;
    floor->scl[0] = 1.0F;
    floor->scl[1] = 1.0F;
    floor->scl[2] = 1.0F;
    floor->ang[0] = 0;
    floor->ang[1] = work->ang.y;
    floor->ang[2] = 0;
    floor->pos[0] = work->pos.x + from->pos[0];
    floor->pos[1] = work->pos.y + from->pos[1];
    floor->pos[2] = work->pos.z + from->pos[2];
    floor->model = from->model;
    floor->child = NULL;
    floor->sibling = NULL;
    world_add(WORLD_MOVES, task, floor);
}

static void elevator2_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->floor);
    world_object_free(VARS(work)->floor);
}

static void elevator2_wait(struct task *task)
{
    struct task_work *work = task->work;

    if (player_stands_on(0, task) || player_stands_on(1, task)) {
        work->mode = LIFT_DOWN;
        work->timer = 500; /* the floor never reads its timer */
        pad_disable();
    }
}
