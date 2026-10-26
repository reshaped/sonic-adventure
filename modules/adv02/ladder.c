/* the ladder the object list names O LADDER, which players climb along a
 * ledge path */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/ladder.h"
#include "adv02/texlists.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "path/path.h"
#include "path/steps.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

#define RUNGS 33 /* the path's points */
#define RUNG_SPACING 3.0F /* the distance between points, up the ladder's y */
/* the first point's height above the ladder's position */
#define FIRST_RUNG 6.0F

/* the path the ladder's path_ledge_step child follows, and the path's points.
 * ladder_start allocates both from the heap, and ladder_destroy frees them */
struct ladder_vars {
    Uint8 unk_00[4];
    struct path_point *points;
    struct path *path;
};

#define VARS(work) ((struct ladder_vars *)&(work)->vars)

static void ladder_step(struct task *task);
static void ladder_draw(struct task *task);
static void ladder_destroy(struct task *task);

/* the path's offset along the ladder's z, in front of the ladder */
static Float ladder_reach = -2.55F;

/* a ladder the set editor holds as it starts never gets its path */
void ladder_start(struct task *task)
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

    if (placement_editing(task)) {
        ladder_draw(task);
        task->step = ladder_step;
        return;
    }
    child = task_create_child(TASK_WORK, path_ledge_step, task);
    PATH_VARS(child->work)->stage_act = stage_get_stage_act();
    child->work->ang.x = 0;
    child->work->ang.y = work->ang.y;
    child->work->ang.z = 0;
    offset.x = 0.0F;
    offset.y = FIRST_RUNG;
    offset.z = ladder_reach;
    /* @hack spelling: the count through a local, which SHC keeps in the frame,
     * MOV.L R2,@R15 at 0x0c90cce6; as RUNGS in the loop's test, the function
     * grows, the functions after it move, and 122 of its 159 words differ */
    rungs = RUNGS;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, work->ang.y, ang);
    points = heap_alloc(sizeof(struct path_point) * RUNGS);
    VARS(work)->points = points;
    for (i = 0; i < rungs; i++) {
        points[i].ang_x = 0;
        points[i].ang_z = 0;
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
    path->count = RUNGS;
    path->length = (RUNGS - 1) * RUNG_SPACING;
    path->points = points;
    path->step = path_ledge_step;
    PATH_VARS(child->work)->path = path;
    task->step = ladder_step;
    task->draw = ladder_draw;
    task->destructor = ladder_destroy;
}

static void ladder_step(struct task *task)
{
    if (!placement_try_leave(task)) {
        ladder_draw(task);
        task_step_children(task);
    }
}

static void ladder_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njSetTexture(&mrobj_texlist);
    draw_model(ladder_object->model);
    njPopMatrix(1);
}

static void ladder_destroy(struct task *task)
{
    struct task_work *work = task->work;

    heap_free(VARS(work)->path);
    heap_free(VARS(work)->points);
}
