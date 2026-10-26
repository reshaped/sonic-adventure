/* O SLIGHT solid searchlight whose beam sweeps the sky */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg04/searchlight.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the head is the lamp, and its child the beam */
extern NJS_MODEL searchlight_base_model;
extern NJS_OBJECT searchlight_head_object;

/* destroyed beyond 5000 units of the camera and player 0. SEARCHLIGHT_REACH
 * is the radius of its collision record, whose capsule reaches 29 from its
 * position */
#define SEARCHLIGHT_RANGE (5000.0F * 5000.0F)
#define SEARCHLIGHT_REACH 25.0F

/* the lamp and its beam sweep SWEEP each way about z, there and back every
 * 360 steps */
#define SWEEP (23.0F * ANGLE_PER_DEGREE)

static void searchlight_stand(struct task *task);
static void searchlight_draw(struct task *task);

static struct collision_shape searchlight_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 15.0F, 0.0F},
    7.0F,
    7.0F,
    0.0F,
    {0},
    0,
    NULL,
};

void searchlight_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, SEARCHLIGHT_RANGE)) {
        if (!placement_editing(task)) {
            collision_init(task, &searchlight_shape, COLLISION_OBJECT, 1, 1);
            work->collision->radius = SEARCHLIGHT_REACH;
        }
        task->draw = searchlight_draw;
        task->step = searchlight_stand;
    }
}

static void searchlight_stand(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, SEARCHLIGHT_RANGE)) {
        if (!placement_editing(task)) {
            collision_register(work);
            object_clear_touch(work, NULL);
        }
        searchlight_draw(task);
        work->timer++;
    }
}

/* draw the stand, then the lamp and its beam without fog, swept about z.
 * after the sweep, the lamp turns about y by the set entry's angle a second
 * time */
static void searchlight_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &searchlight_head_object;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_model(&searchlight_base_model);
    njFogDisable();
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    TURN(njRotateZ,
         (Angle)(njSin((Angle)(TASK_TIMER(work) * ANGLE_PER_DEGREE)) * SWEEP));
    TURN(njRotateY, work->ang.y);
    draw_model(object->model);
    object = object->child;
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    draw_model(object->model);
    njFogEnable();
    njPopMatrix(1);
}
