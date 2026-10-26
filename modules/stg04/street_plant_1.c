/* O StPlant01 street tree with a solid trunk */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg04/street_plant_1.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT street_plant_1_object;

/* the radius of the tree's collision record. its capsule reaches 17 from
 * its position */
#define TREE_REACH 32.0F

static struct collision_shape street_plant_1_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 8.0F, 0.0F},
    1.0F,
    8.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static void street_plant_1_destroy(struct task *task)
{
}

static void street_plant_1_draw(struct task *task)
{
    struct task_work *work = task->work;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    texture_set_stage();
    draw_tree(&street_plant_1_object);
    njPopMatrix(1);
}

static void street_plant_1_stand(struct task *task)
{
    struct task_work *work;

    if (!placement_try_leave(task)) {
        work = task->work;
        street_plant_1_draw(task);
        if (!placement_editing(task)) {
            collision_register(work);
        }
    }
}

void street_plant_1_step(struct task *task)
{
    task->destructor = street_plant_1_destroy;
    task->step = street_plant_1_stand;
    task->draw = street_plant_1_draw;
    if (!placement_editing(task)) {
        collision_init(task, &street_plant_1_shape, COLLISION_OBJECT, 1, 1);
        task->work->collision->radius = TREE_REACH;
    }
}
