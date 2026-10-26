/* O HASIGO: a solid ladder, with a ledge path up its rungs that players
 * climb */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "path/path.h"
#include "path/steps.h"
#include "stage/stage.h"
#include "stg12/ladder.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: declared an array, not the one NJS_OBJECT it is, so SHC
 * pools its address and reads model through it, MOV.L @(4,R4),R4 at
 * 0x0c908466; declared as one, 2 of shelter_ladder_draw's 51 words and a
 * pool word differ */
extern NJS_OBJECT shelter_ladder_object[];

#define RUNGS 33 /* the most points the path has, one at each rung */
/* the distance between rungs, along the ladder's y */
#define RUNG_SPACING 3.0F
/* the path starts this many rungs up the ladder. the rungs scale y leaves out
 * raise it further */
#define FOOT_RUNGS 3

#define REACH 180.0F /* the collision record's radius */

/* the path the ladder's path_ledge_step child holds, and the path's points.
 * the ladder allocates both from the heap and frees them as it's destroyed */
struct shelter_ladder_vars {
    Uint8 unk_00[4];
    struct path_point *points;
    struct path *path;
};

#define VARS(work) ((struct shelter_ladder_vars *)&(work)->vars)

/* the path's z in the ladder's axes, in front of its rungs */
static Float shelter_ladder_path_z = -3.2F;

static struct collision_shape shelter_ladder_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 50.0F, 0.0F},
    8.0F,
    50.0F,
    1.0F,
    {0},
    0,
    NULL,
};

static void shelter_ladder_destroy(struct task *task)
{
    struct task_work *work = task->work;

    heap_free(VARS(work)->path);
    heap_free(VARS(work)->points);
}

/* draw the ladder turned a half turn from its path */
static void shelter_ladder_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y + 0x8000, ang);
        draw_model(shelter_ladder_object->model);
        njPopMatrix(1);
    }
}

static void shelter_ladder_step(struct task *task)
{
    if (!placement_try_leave(task)) {
        shelter_ladder_draw(task);
        collision_register(task->work);
        task_step_children(task);
    }
}

void shelter_ladder_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    struct path_point *points;
    struct path *path;
    NJS_POINT3 rung;
    NJS_POINT3 offset;
    Angle ang;
    int rungs;
    int i;

    collision_init(task, &shelter_ladder_shape, COLLISION_OBJECT, 1, 1);
    work->collision->radius = REACH;
    if (placement_editing(task)) {
        shelter_ladder_draw(task);
        task->step = shelter_ladder_step;
        return;
    }
    child = task_create_child(TASK_WORK, path_ledge_step, task);
    PATH_VARS(child->work)->stage_act = stage_get_stage_act();
    child->work->ang.x = work->ang.x;
    child->work->ang.y = work->ang.y;
    child->work->ang.z = work->ang.z;
    /* scale y counts the rungs the path leaves out at the foot, or none when
     * it isn't positive. the path holds the rest, so a set entry must give
     * fewer than RUNGS, which nothing here checks */
    if (work->scl.y <= 0.0F) {
        rungs = 0;
    } else {
        rungs = (int)work->scl.y;
    }
    offset.x = 0.0F;
    offset.y = (rungs + FOOT_RUNGS) * RUNG_SPACING;
    offset.z = shelter_ladder_path_z;
    rungs = RUNGS - rungs;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    points = heap_alloc(sizeof(struct path_point) * rungs);
    VARS(work)->points = points;
    for (i = 0; i < rungs; i++) {
        points[i].ang_x = work->ang.x;
        points[i].ang_z = work->ang.z;
        points[i].length = RUNG_SPACING;
        njCalcPoint(NULL, &offset, &rung);
        points[i].pos.x = work->pos.x + rung.x;
        points[i].pos.y = work->pos.y + rung.y;
        points[i].pos.z = work->pos.z + rung.z;
        offset.y += RUNG_SPACING;
    }
    njPopMatrix(1);
    path = heap_alloc(sizeof(struct path));
    VARS(work)->path = path;
    path->ends = PATH_STOP;
    path->count = rungs;
    path->length = (rungs - 1) * RUNG_SPACING;
    path->points = points;
    path->step = path_ledge_step;
    PATH_VARS(child->work)->path = path;
    task->step = shelter_ladder_step;
    task->draw = shelter_ladder_draw;
    task->destructor = shelter_ladder_destroy;
}
