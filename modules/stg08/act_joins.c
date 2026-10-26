/* ACT23JOIN joins act 1 to act 2 with solid ground, and ACT12JOIN act 0 to
 * act 1. their names count acts from 1 */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg08/act_joins.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_OBJECT icecap_act23_join_object;
extern NJS_OBJECT icecap_act12_join_object;

/* the collision world tests a join only while a player is near it. near
 * means within the model's radius plus JOIN_NEAR of the join's center. the
 * join's center is the model's center offset by the join's position */
#define JOIN_NEAR 16.0F

/* which join the task is, in work->mode */
enum act_join_kind { JOIN_ACT12, JOIN_ACT23 };

struct act_join_vars {
    NJS_OBJECT *object; /* its copy of source in the world, or null */
    NJS_OBJECT *source; /* the join's model, which it draws */
};

#define VARS(work) ((struct act_join_vars *)&(work)->vars)

static void act_join_run(struct task *task);
static void act_join_draw(struct task *task);
static void act_join_destroy(struct task *task);

/* a join stands where its model's position puts it, not where its task was
 * created. a join the set file placed holds its placement, so its task lives
 * on as the act changes. a join the set editor holds has no copy in the
 * world */
void icecap_act23_join_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (task->placement != NULL) {
        placement_hold(task);
    }
    work->mode = JOIN_ACT23;
    VARS(work)->source = &icecap_act23_join_object;
    if (!placement_editing(task)) {
        VARS(work)->object = object = world_object_alloc();
        object->evalflags = VARS(work)->source->evalflags &
                            ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
        object->scl[0] = 1.0F;
        object->scl[1] = 1.0F;
        object->scl[2] = 1.0F;
        object->ang[0] = 0;
        object->ang[1] = 0;
        object->ang[2] = 0;
        object->pos[0] = work->pos.x = VARS(work)->source->pos[0];
        object->pos[1] = work->pos.y = VARS(work)->source->pos[1];
        object->pos[2] = work->pos.z = VARS(work)->source->pos[2];
        object->model = VARS(work)->source->model;
        object->child = VARS(work)->source->child;
        object->sibling = VARS(work)->source->sibling;
        world_add(WORLD_SOLID, task, object);
    }
    task->destructor = act_join_destroy;
    task->step = act_join_run;
    task->draw = act_join_draw;
}

/* kill the join in any act but its two. in them, the step does nothing
 * while the camera is beyond the far clip from the model's center */
static void act_join_run(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 act = stage_get_stage_act();
    NJS_OBJECT *source;
    NJS_OBJECT *object;
    Float distance;

    if (work->mode == JOIN_ACT12) {
        if (act != STAGE_ACT(STAGE_ICECAP, 0) &&
            act != STAGE_ACT(STAGE_ICECAP, 1)) {
            task_kill(task);
            return;
        }
    } else if (act != STAGE_ACT(STAGE_ICECAP, 1) &&
               act != STAGE_ACT(STAGE_ICECAP, 2)) {
        task_kill(task);
        return;
    }
    source = VARS(work)->source;
    /* @hack spelling: the distance through a local, which copies it to FR4
     * for the compare (0x0c90e3cc); compared in place, 56 of 112 words
     * differ */
    /* @bug logic: the distance is to the model's center, near the
     * origin, not to the join's center (0x0c90e3c4, 0x0c90e3c6). with the
     * camera within the far clip of the join but beyond it from the origin,
     * the join neither draws nor updates WORLD_TESTED */
    distance = njDistanceP2P(&camera_work->pos, &source->model->center);
    if (distance >= _nj_far_clip_) {
        return;
    }
    object = VARS(work)->object;
    if (object != NULL) {
        NJS_POINT3 center;

        center = object->model->center;
        njAddVector(&center, &work->pos);
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y;
        object->pos[2] = work->pos.z;
        object->ang[0] = work->ang.x;
        object->ang[1] = work->ang.y;
        object->ang[2] = work->ang.z;
        if (player_in_range(&center, object->model->r + JOIN_NEAR)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
    act_join_draw(task);
}

/* draw source by its own position and angles. the task's don't move it */
static void act_join_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    draw_tree(VARS(work)->source);
}

static void act_join_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->object != NULL) {
        world_remove(task, VARS(work)->object);
        world_object_free(VARS(work)->object);
    }
}

void icecap_act12_join_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (task->placement != NULL) {
        placement_hold(task);
    }
    work->mode = JOIN_ACT12;
    VARS(work)->source = &icecap_act12_join_object;
    if (!placement_editing(task)) {
        VARS(work)->object = object = world_object_alloc();
        object->evalflags = VARS(work)->source->evalflags &
                            ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_SCL);
        object->scl[0] = 1.0F;
        object->scl[1] = 1.0F;
        object->scl[2] = 1.0F;
        object->ang[0] = 0;
        object->ang[1] = 0;
        object->ang[2] = 0;
        object->pos[0] = work->pos.x = VARS(work)->source->pos[0];
        object->pos[1] = work->pos.y = VARS(work)->source->pos[1];
        object->pos[2] = work->pos.z = VARS(work)->source->pos[2];
        object->model = VARS(work)->source->model;
        object->child = VARS(work)->source->child;
        object->sibling = VARS(work)->source->sibling;
        world_add(WORLD_SOLID, task, object);
    }
    task->destructor = act_join_destroy;
    task->step = act_join_run;
    task->draw = act_join_draw;
}
