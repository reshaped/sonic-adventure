/* O Ukishima floating island that sways along its z and rises and falls,
 * carrying what stands on it */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg10/floating_island.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT floating_island_object;
/* the model the collision world tests for the island. its pos is the
 * model's offset from the island */
extern NJS_OBJECT floating_island_world_object;

/* the collision world tests the island while a player is this near */
#define NEAR 48.0F

struct island_vars {
    NJS_POINT3 home; /* the set entry's position: the middle of the ride */
};

#define VARS(work) ((struct island_vars *)&(work)->vars)
#define FORCE(task) ((struct task_force *)(task)->force)

/* the ride, from the set entry. the island sways SWAY(work) either way along
 * its own z, and rises and falls RISE(work). both go as the sine of
 * PHASE(work), which gains PHASE_STEP(work) each step */
#define PHASE(work) ((work)->ang.x)
#define PHASE_STEP(work) ((work)->ang.z)
#define RISE(work) ((work)->scl.y)
#define SWAY(work) ((work)->scl.z)

/* the island's object in the collision world, kept in work->collision.
 * task_free_work frees a non-null collision record there, so the destructor
 * clears it first */
#define OBJECT(work) (*(NJS_OBJECT **)&(work)->collision)

enum island_mode { ISLAND_INIT, ISLAND_RUN };

static void floating_island_init(struct task *task);
static void floating_island_ride(struct task *task);
static void floating_island_draw(struct task *task);
static void floating_island_add(struct task *task);
static void floating_island_destroy(struct task *task);

void floating_island_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_far_from_camera(task)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case ISLAND_INIT:
            floating_island_init(task);
            break;
        case ISLAND_RUN:
            floating_island_draw(task);
            PHASE(work) += PHASE_STEP(work);
            work->pos.y =
                VARS(work)->home.y + (njSin(PHASE(work)) * RISE(work));
            floating_island_ride(task);
            break;
        }
    } else {
        switch (work->mode) {
        case ISLAND_INIT:
            floating_island_init(task);
            break;
        case ISLAND_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "OFFSET");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "MOVE SPEED");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "NO USE");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "MOVE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "MOVE Z");
            floating_island_draw(task);
            floating_island_ride(task);
            break;
        }
    }
}

static void floating_island_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = floating_island_draw;
    task->destructor = floating_island_destroy;
    floating_island_add(task);
    work->mode = ISLAND_RUN;
}

/* sway the island along its own z by its phase. both of the task's struct
 * task_force records take the island's move this step as their speed. that
 * speed carries the players standing on the island. the island's object in
 * the collision world follows the island */
static void floating_island_ride(struct task *task)
{
    struct task_work *work;
    NJS_VECTOR offset = {0.0F, 0.0F, 0.0F};
    Float old_x;
    Float old_y;
    Float old_z;
    NJS_OBJECT *from;
    NJS_OBJECT *object;
    Angle ang;

    work = task->work;
    old_x = work->pos.x;
    /* @bug logic: the island carries its riders along x and z only, not as
     * it rises and falls. floating_island_step has already moved pos.y when
     * the ride reads it as the old height (0x0c910fec), so both records' y
     * speed is pos.y less itself (FSUB at 0x0c91106c), always 0 */
    old_y = work->pos.y;
    old_z = work->pos.z;
    njPushMatrix(&_unit_matrix_);
    offset.z = njSin(PHASE(work)) * SWAY(work);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcVector(NULL, &offset, &offset);
    work->pos.x = VARS(work)->home.x + offset.x;
    work->pos.z = VARS(work)->home.z + offset.z;
    njPopMatrix(1);
    FORCE(task)[1].speed.x = FORCE(task)[0].speed.x = work->pos.x - old_x;
    FORCE(task)[1].speed.y = FORCE(task)[0].speed.y = work->pos.y - old_y;
    FORCE(task)[1].speed.z = FORCE(task)[0].speed.z = work->pos.z - old_z;
    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    from = &floating_island_world_object;
    object = OBJECT(work);
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = work->pos.x + from->pos[0];
    object->pos[1] = work->pos.y + from->pos[1];
    object->pos[2] = work->pos.z + from->pos[2];
    /* the timer counts 0 to 20, round and round, and nothing else reads it */
    work->timer++;
    if (TASK_TIMER(work) >= 21) {
        work->timer = 0;
    }
}

static void floating_island_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&floating_island_object);
        njPopMatrix(1);
    }
}

static void floating_island_add(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *from;
    NJS_OBJECT *object;

    VARS(work)->home.x = work->pos.x;
    VARS(work)->home.y = work->pos.y;
    VARS(work)->home.z = work->pos.z;
    from = &floating_island_world_object;
    OBJECT(work) = object = world_object_alloc();
    object->evalflags = from->evalflags;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = work->pos.x + from->pos[0];
    object->pos[1] = work->pos.y + from->pos[1];
    object->pos[2] = work->pos.z + from->pos[2];
    object->model = from->model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(WORLD_MOVES, task, object);
    work->timer = 0;
}

static void floating_island_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, OBJECT(work));
    world_object_free(OBJECT(work));
    OBJECT(work) = NULL;
}
