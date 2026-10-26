/* O StPlant02 street plant that is only drawn */
#include <ninja.h>

#include "draw/tree.h"
#include "object/placement.h"
#include "stg04/street_plant_2.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT street_plant_2_object;

static void street_plant_2_destroy(struct task *task)
{
}

static void street_plant_2_draw(struct task *task)
{
    struct task_work *work = task->work;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    texture_set_stage();
    draw_tree(&street_plant_2_object);
    njPopMatrix(1);
}

static void street_plant_2_stand(struct task *task)
{
    if (!placement_try_leave(task)) {
        street_plant_2_draw(task);
    }
}

void street_plant_2_step(struct task *task)
{
    task->destructor = street_plant_2_destroy;
    task->step = street_plant_2_stand;
    task->draw = street_plant_2_draw;
}
