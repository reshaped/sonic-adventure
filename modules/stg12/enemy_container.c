/* O ENEMYCONTAINER: a container whose door opens as a player comes near */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg12/container_door.h"
#include "stg12/enemy_container.h"
#include "stg12/texlists.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the container, drawn by its action, whose motion opens the door */
/* @hack type: declared an array, not the one NJS_ACTION it is, so SHC
 * pools its address and reads motion through it, MOV.L @(4,R3),R2 at
 * 0x0c913536; declared as one, 3 of enemy_container_run's 114 words and a
 * pool word differ */
extern NJS_ACTION enemy_container_action[];
/* the body's model, for the collision world */
extern NJS_OBJECT enemy_container_body_object;

#define CONTAINER_FLAGS (WORLD_LARGE | WORLD_MOVES)

/* a player within OPEN_RANGE of the container's position, moved OPEN_X along
 * x and -OPEN_Z along z, opens it. the offset ignores the container's turn */
#define OPEN_X 110.0F
#define OPEN_Z 30.0F
#define OPEN_RANGE 55.0F

#define SE_OPEN 327

enum enemy_container_mode { CONTAINER_SHUT, CONTAINER_OPENING, CONTAINER_OPEN };

struct enemy_container_vars {
    NJS_OBJECT *object; /* in the collision world */
    Float frame;
};

#define VARS(work) ((struct enemy_container_vars *)&(work)->vars)

static void enemy_container_run(struct task *task);
static void enemy_container_draw(struct task *task);
static void enemy_container_destroy(struct task *task);
static void enemy_container_door_start(struct task *task);
static void enemy_container_door_step(struct task *task);
static void enemy_container_door_destroy(struct task *task);

void enemy_container_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    work->ang.x = 0;
    work->ang.z = 0;
    work->mode = CONTAINER_SHUT;
    work->timer = 300;
    if (!placement_editing(task)) {
        VARS(work)->object = object = world_object_alloc();
        object->evalflags = NJD_EVAL_BREAK;
        object->scl[0] = 1.0F;
        object->scl[1] = 1.0F;
        object->scl[2] = 1.0F;
        object->ang[0] = 0;
        object->ang[1] = work->ang.y;
        object->ang[2] = 0;
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y;
        object->pos[2] = work->pos.z;
        object->model = enemy_container_body_object.model;
        object->child = NULL;
        object->sibling = NULL;
        world_add(CONTAINER_FLAGS, task, object);
        work->flags |= WORLD_TESTED;
        task_create_child(TASK_WORK | TASK_FORCE, enemy_container_door_start,
                          task);
    }
    task->step = enemy_container_run;
    task->draw = enemy_container_draw;
    task->destructor = enemy_container_destroy;
}

static void enemy_container_run(struct task *task)
{
    struct task_work *work = task->work;
    Float x;
    Float z;
    Float last;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case CONTAINER_SHUT:
        x = work->pos.x;
        z = work->pos.z;
        work->pos.x += OPEN_X;
        work->pos.z -= OPEN_Z;
        if (player_in_range(&work->pos, OPEN_RANGE)) {
            work->mode = CONTAINER_OPENING;
            VARS(work)->frame = 0.0F;
            sound_play(SE_OPEN, 0, 0, 0);
        }
        work->pos.x = x;
        work->pos.z = z;
        task_step_children(task);
        break;
    case CONTAINER_OPENING:
        VARS(work)->frame += 1.0F;
        last = (Float)(enemy_container_action->motion->nbFrame - 1);
        if (VARS(work)->frame >= last) {
            VARS(work)->frame = last;
            work->mode = CONTAINER_OPEN;
            task_destroy_children(task);
        }
        break;
    case CONTAINER_OPEN:
        break;
    }
    enemy_container_draw(task);
}

static void enemy_container_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&hotshelter4_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(enemy_container_action, VARS(work)->frame);
        njPopMatrix(1);
    }
}

static void enemy_container_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->object) {
        world_remove(task, VARS(work)->object);
        world_object_free(VARS(work)->object);
    }
    task_destroy_children(task);
}

/* a child that holds the shut door in the collision world. the container
 * destroys it once open */
static void enemy_container_door_start(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    work->ang.x = 0;
    work->ang.z = 0;
    VARS(work)->object = object = world_object_alloc();
    object->evalflags = NJD_EVAL_BREAK;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->model = enemy_container_door_object.model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(CONTAINER_FLAGS, task, object);
    work->flags |= WORLD_TESTED;
    task->step = enemy_container_door_step;
    task->destructor = enemy_container_door_destroy;
}

static void enemy_container_door_step(struct task *task)
{
}

static void enemy_container_door_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->object) {
        world_remove(task, VARS(work)->object);
        world_object_free(VARS(work)->object);
    }
}
