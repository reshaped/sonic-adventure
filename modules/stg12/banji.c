/* O BANJI, a frame about 800 units tall, with a platform near its middle */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg12/banji.h"
#include "stg12/texlists.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT banji_object;
extern NJS_OBJECT banji_platform_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)
/* the collision world tests the platform only while a player is within 148
 * units of the task */
#define NEAR 148.0F

enum banji_mode { BANJI_INIT, BANJI_RUN };

struct banji_vars {
    Uint8 unk_00[8];
    NJS_OBJECT *platform; /* its object in the collision world */
};

#define VARS(work) ((struct banji_vars *)&(work)->vars)

static void banji_init(struct task *task);
static void banji_update(struct task *task);
static void banji_draw(struct task *task);
static void banji_destroy(struct task *task);

/* no code reads this shape, but the image holds it */
static struct collision_shape banji_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    17.0F,
    1.5F,
    0.0F,
    {0},
    0,
    NULL,
};

void banji_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case BANJI_INIT:
            banji_init(task);
            break;
        case BANJI_RUN:
            banji_draw(task);
            banji_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case BANJI_INIT:
            banji_init(task);
            break;
        case BANJI_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            banji_draw(task);
            banji_update(task);
            break;
        }
    }
}

static void banji_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *platform;

    task->draw = banji_draw;
    task->destructor = banji_destroy;
    VARS(work)->platform = platform = world_object_alloc();
    platform->evalflags = banji_platform_object.evalflags;
    platform->scl[0] = 1.0F;
    platform->scl[1] = 1.0F;
    platform->scl[2] = 1.0F;
    platform->ang[0] = 0;
    platform->ang[1] = work->ang.y;
    platform->ang[2] = 0;
    platform->pos[0] = work->pos.x + banji_platform_object.pos[0];
    platform->pos[1] = work->pos.y + banji_platform_object.pos[1];
    platform->pos[2] = work->pos.z + banji_platform_object.pos[2];
    platform->model = banji_platform_object.model;
    platform->child = NULL;
    platform->sibling = NULL;
    world_add(0, task, platform);
    work->mode = BANJI_RUN;
}

/* the platform's object in the collision world follows the task. it turns
 * about z by the task's z angle. init turned it about y by the y angle, as
 * the draw turns the platform */
static void banji_update(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *from;
    NJS_OBJECT *platform;

    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    from = &banji_platform_object;
    platform = VARS(work)->platform;
    platform->ang[0] = 0;
    platform->ang[1] = 0;
    platform->ang[2] = work->ang.z;
    platform->pos[0] = work->pos.x + from->pos[0];
    platform->pos[1] = work->pos.y + from->pos[1];
    platform->pos[2] = work->pos.z + from->pos[2];
}

static void banji_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&hotshelter2_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&banji_object);
        draw_tree(&banji_platform_object);
        njPopMatrix(1);
    }
}

static void banji_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->platform);
    world_object_free(VARS(work)->platform);
}
