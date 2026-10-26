/* O ASIBA1, a solid scaffold footing that its set entry scales */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg05/scaffold.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_OBJECT scaffold_object;

struct scaffold_vars {
    NJS_OBJECT *world;
    Uint8 unk_04[4];
    /* world's model: a heap copy of scaffold_object's, its radius scaled */
    NJS_MODEL *model;
};

#define VARS(work) ((struct scaffold_vars *)&(work)->vars)

/* the scaffold draws at the set entry's scales plus 1, and its size is the
 * largest of those. its reach and draw radius are at a size of 1. its
 * model's radius is less, about 26 */
#define SCAFFOLD_REACH 36.0F
#define DRAW_RADIUS 46.0F
/* the world tests the copy only while a player is within its reach plus
 * this margin */
#define NEAR_MARGIN 16.0F

static void scaffold_destroy(struct task *task);
static void scaffold_run(struct task *task);
static void scaffold_draw(struct task *task);

/* unless the set editor holds the scaffold, put a scaled copy of the model in
 * the world. the copy's radius scales by the size, so it bounds the scaled
 * model. the size times SCAFFOLD_REACH picks its world size */
void scaffold_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_MODEL *model;
    NJS_OBJECT *world;
    Float size;
    Uint32 flags;

    if (placement_try_leave(task)) {
        return;
    }
    task->destructor = scaffold_destroy;
    task->step = scaffold_run;
    task->draw = scaffold_draw;
    work->ang.x = 0;
    work->ang.z = 0;
    if (placement_editing(task)) {
        return;
    }
    size = work->scl.x;
    if (work->scl.y > size) {
        size = work->scl.y;
    }
    if (work->scl.z > size) {
        size = work->scl.z;
    }
    size += 1.0F;
    model = heap_alloc(sizeof(NJS_MODEL));
    *model = *scaffold_object.model;
    VARS(work)->model = model;
    model->r *= size;
    VARS(work)->world = world = world_object_alloc();
    world->scl[0] = work->scl.x + 1.0F;
    world->scl[1] = work->scl.y + 1.0F;
    world->scl[2] = work->scl.z + 1.0F;
    world->ang[0] = 0;
    world->ang[1] = work->ang.y;
    world->ang[2] = 0;
    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    world->model = model;
    work->flags &= ~WORLD_TESTED;
    size *= SCAFFOLD_REACH;
    flags = WORLD_SOLID | WORLD_HUGE;
    if (size <= WORLD_SMALL_REACH) {
        flags = WORLD_SOLID | WORLD_SMALL;
    } else if (size <= WORLD_MEDIUM_REACH) {
        flags = WORLD_SOLID | WORLD_MEDIUM;
    } else if (size <= WORLD_LARGE_REACH) {
        flags = WORLD_SOLID | WORLD_LARGE;
    }
    world_add(flags, task, world);
}

static void scaffold_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->world) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
    heap_free(VARS(work)->model);
}

static void scaffold_run(struct task *task)
{
    struct task_work *work = task->work;
    Float size;
    NJS_OBJECT *world;

    if (placement_try_leave(task)) {
        return;
    }
    world = VARS(work)->world;
    if (world) {
        size = work->scl.x;
        if (size < work->scl.y) {
            size = work->scl.y;
        }
        if (size < work->scl.z) {
            size = work->scl.z;
        }
        if (player_in_range(&work->pos,
                            ((size + 1.0F) * SCAFFOLD_REACH) + NEAR_MARGIN)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
    scaffold_draw(task);
}

static void scaffold_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float size;
    Float reach;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    size = work->scl.x;
    if (size < work->scl.y) {
        size = work->scl.y;
    }
    if (size < work->scl.z) {
        size = work->scl.z;
    }
    size += 1.0F;
    reach = size * DRAW_RADIUS;
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, reach)) {
        return;
    }
    njSetTexture(&obj_mountain_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, work->scl.x + 1.0F, work->scl.y + 1.0F, work->scl.z + 1.0F);
    draw_model(scaffold_object.model);
    njPopMatrix(1);
}
