/* ZONE, a sphere a racing Chao can touch, drawn only in edit mode. the
 * collision checks ignore its shape for players and attacks, objects, pickups
 * and rings */
#include <ninja.h>

#include "al_race/zone.h"
#include "collision/collision.h"
#include "collision/draw.h"
#include "debug/editor.h"
#include "object/simple.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"

/* a zone's radius is its scale's x. zone_get_size tells a small zone from a
 * large one by its scale's z */
#define ZONE_RADIUS(work) ((work)->scl.x)

static void zone_start(struct task *task, void *arg);
static void zone_step(struct task *task);
static void zone_draw(struct task *task);

static struct collision_shape zone_shapes[1];

static const struct task_desc zone_desc = {0,         TASK_WORK, zone_start,
                                           zone_step, zone_draw, zone_destroy};
static const struct collision_list zone_list = {zone_shapes, 1, 40.0F};
static const struct collision_kind zone_collision = {COLLISION_OBJECT, 1};

static struct collision_shape zone_shapes[1] = {
    {SHAPE_SPHERE | SHAPE_IGNORE_PLAYERS | SHAPE_IGNORE_OBJECTS |
         SHAPE_IGNORE_OTHERS,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     0.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

void zone_init(struct task *task)
{
    task_apply_desc(task, &zone_desc, NULL);
}

void zone_destroy(struct task *task)
{
}

static void zone_start(struct task *task, void *arg)
{
    struct task_work *work = task->work;

    zone_shapes[0].radius = ZONE_RADIUS(work);
    object_collision_init(task, (struct collision_list *)&zone_list,
                          (struct collision_kind *)&zone_collision);
}

static void zone_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_editing(task)) {
        work->collision->shapes[0].radius = ZONE_RADIUS(work);
    } else {
        collision_register(work);
    }
    zone_draw(task);
}

static void zone_draw(struct task *task)
{
    if (edit_mode && frame_step == 0) {
        collision_draw(task->work->collision);
    }
}
