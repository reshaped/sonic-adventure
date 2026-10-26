/* the island act's ground, whose task puts it in the collision world and
 * draws it */
#include <ninja.h>

#include "adv02/island_land.h"
#include "adv02/texlists.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "player/player.h"
#include "system/task.h"

extern NJS_OBJECT island_land_object;
extern NJS_OBJECT island_land_world_1_object;
extern NJS_OBJECT island_land_world_2_object;

/* the ground's fixed position */
#define LAND_X (-1507.74F)
#define LAND_Y (-411.085F)
#define LAND_Z 2120.696F

/* the collision world tests the ground only while a player is this near */
#define NEAR 1500.0F

#define WORLD_1_FLAGS WORLD_LARGE
#define WORLD_2_FLAGS (WORLD_LARGE | WORLD_UNCLIMBABLE)

enum island_land_mode { ISLAND_LAND_INIT, ISLAND_LAND_RUN };

struct island_land_vars {
    NJS_OBJECT *world_1;
    NJS_OBJECT *world_2;
};

#define VARS(work) ((struct island_land_vars *)&(work)->vars)

static void island_land_init(struct task *task);
static void island_land_add_to_world(struct task *task);
static void island_land_update(struct task *task);
static void island_land_draw(struct task *task);
static void island_land_destroy(struct task *task);

/* while the set editor holds the ground, each step rebuilds its world objects,
 * so they follow the edit */
void island_land_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        switch (work->mode) {
        case ISLAND_LAND_INIT:
            island_land_init(task);
            break;
        case ISLAND_LAND_RUN:
            island_land_draw(task);
            island_land_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case ISLAND_LAND_INIT:
            island_land_init(task);
            break;
        case ISLAND_LAND_RUN:
            island_land_destroy(task);
            island_land_add_to_world(task);
            island_land_draw(task);
            island_land_update(task);
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            break;
        }
    }
}

static void island_land_init(struct task *task)
{
    struct task_work *work = task->work;

    work->pos.x = LAND_X;
    work->pos.y = LAND_Y;
    work->pos.z = LAND_Z;
    task->draw = island_land_draw;
    task->destructor = island_land_destroy;
    island_land_add_to_world(task);
    work->mode = ISLAND_LAND_RUN;
}

static void island_land_add_to_world(struct task *task)
{
    struct task_force *force = task->force;
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    NJS_OBJECT *from;

    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    from = &island_land_world_1_object;
    VARS(work)->world_1 = object = world_object_alloc();
    object->evalflags = from->evalflags;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = from->pos[0] + work->pos.x;
    object->pos[1] = from->pos[1] + work->pos.y;
    object->pos[2] = from->pos[2] + work->pos.z;
    object->model = from->model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(WORLD_1_FLAGS, task, object);
    from = &island_land_world_2_object;
    VARS(work)->world_2 = object = world_object_alloc();
    object->evalflags = from->evalflags;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = from->pos[0] + work->pos.x;
    object->pos[1] = from->pos[1] + work->pos.y;
    object->pos[2] = from->pos[2] + work->pos.z;
    object->model = from->model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(WORLD_2_FLAGS, task, object);
}

static void island_land_update(struct task *task)
{
    struct task_work *work = task->work;

    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

static void island_land_draw(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(&mrobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    draw_tree(&island_land_object);
    njPopMatrix(1);
}

static void island_land_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->world_1);
    world_object_free(VARS(work)->world_1);
    world_remove(task, VARS(work)->world_2);
    world_object_free(VARS(work)->world_2);
}
