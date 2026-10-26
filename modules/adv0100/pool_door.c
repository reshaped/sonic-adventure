/* O POOL DOOR, a door by the pool that stands open for Knuckles once his
 * story sets FLAG_KNUCKLES_EC_POOL, which drains the pool */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/pool_door.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL pool_door_model;

/* the door's range for placement_try_leave_by: 1000, squared */
#define POOL_DOOR_RANGE (1000.0F * 1000.0F)
/* how far along z the door slides open */
#define POOL_DOOR_OPEN 20.0F

/* the flag Knuckles' story sets as it plays event 289 in act 2. it opens
 * the door for him, and drains the pool */
#define FLAG_KNUCKLES_EC_POOL 269

enum pool_door_mode { DOOR_INIT, DOOR_RUN };

static void pool_door_init(struct task *task);
static void pool_door_run(struct task *task);
static void pool_door_draw(struct task *task);
static void pool_door_destroy(struct task *task);

static struct collision_shape pool_door_shapes[] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-3.25F, 15.0F, 0.0F},
     3.25F,
     15.0F,
     10.0F,
     {0},
     0,
     NULL},
};

/* TRUE while the door stands open: for Knuckles, once FLAG_KNUCKLES_EC_POOL
 * is set */
static Bool pool_door_open(void)
{
    if (get_character() == CHARACTER_KNUCKLES) {
        return event_flags[FLAG_KNUCKLES_EC_POOL];
    }
    return FALSE;
}

/* draw the door and move it open or shut. while the set editor holds the
 * task, label the editor's angle rows too */
void egg_carrier_pool_door_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, POOL_DOOR_RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case DOOR_INIT:
            pool_door_init(task);
            break;
        case DOOR_RUN:
            pool_door_draw(task);
            pool_door_run(task);
            break;
        }
    } else {
        switch (work->mode) {
        case DOOR_INIT:
            pool_door_init(task);
            break;
        case DOOR_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            pool_door_draw(task);
            pool_door_run(task);
            break;
        }
    }
}

static void pool_door_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = pool_door_draw;
    task->destructor = pool_door_destroy;
    collision_init(task, pool_door_shapes, COLLISION_OBJECT, 1, 1);
    work->mode = DOOR_RUN;
}

/* move the door's box open or shut along z. pool_door_draw draws the door
 * where the box is */
static void pool_door_run(struct task *task)
{
    struct task_work *work = task->work;

    collision_register(work);
    if (pool_door_open()) {
        work->collision->shapes->center.z = POOL_DOOR_OPEN;
    } else {
        work->collision->shapes->center.z = 0.0F;
    }
}

static void pool_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslate(NULL, 0.0F, 0.0F, work->collision->shapes->center.z);
    draw_model(&pool_door_model);
    njPopMatrix(1);
}

static void pool_door_destroy(struct task *task)
{
}
