/* O KNUDOOR, a door that only draws */
#include <ninja.h>

#include "adv03/knuckles_door.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT knuckles_door_object;

enum knuckles_door_mode { DOOR_INIT, DOOR_SHOWN };

static void knuckles_door_destroy(struct task *task);
static void knuckles_door_draw(struct task *task);

void knuckles_door_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case DOOR_INIT:
        /* @bug logic: stores the draw in task->destructor, where task->draw
         * is meant, and the next line stores the destructor over it. the
         * image keeps only the load of knuckles_door_draw's address (MOV.L
         * at 0x0c901c02). task->draw stays NULL, so the door doesn't show under
         * the pause menu */
        task->destructor = knuckles_door_draw;
        task->destructor = knuckles_door_destroy;
        work->mode = DOOR_SHOWN;
        break;
    case DOOR_SHOWN:
        break;
    }
    knuckles_door_draw(task);
}

static void knuckles_door_destroy(struct task *task)
{
}

static void knuckles_door_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_tree(&knuckles_door_object);
    njPopMatrix(1);
}
