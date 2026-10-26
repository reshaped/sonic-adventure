/* the ladder the object list names O LONG LADDER: three ladders tall, and
 * players climb it along a ledge path */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/ladder.h"
#include "adv02/long_ladder.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "path/path.h"
#include "path/steps.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

#define RUNGS 79 /* the path's points */
#define RUNG_SPACING 3.0F /* the distance between points, up the ladder's y */
/* the first point's height above the ladder's position */
#define FIRST_RUNG 6.0F
/* the path's offset along the ladder's z, in front of the ladder */
#define PATH_Z (-2.55F)

/* the ladder draws as LADDERS ladder models, each LADDER_HEIGHT above the last,
 * from LADDER_FOOT below its position */
#define LADDERS 3
#define LADDER_HEIGHT 99.0F
#define LADDER_FOOT 60.0F

enum long_ladder_mode { LONG_LADDER_INIT, LONG_LADDER_RUN };

/* the path the ladder's path_ledge_step child follows, and the path's points.
 * the ladder's first step allocates both from the heap, and
 * long_ladder_destroy frees them */
struct long_ladder_vars {
    Uint8 unk_00[4];
    struct path_point *points;
    struct path *path;
};

#define VARS(work) ((struct long_ladder_vars *)&(work)->vars)

static void long_ladder_destroy(struct task *task);
static void long_ladder_draw(struct task *task);

/* the ladder's first step builds a ledge path of RUNGS points up it, for a
 * child task that runs path_ledge_step. while the set editor holds the
 * ladder, each step only draws it */
void long_ladder_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    struct task_work *child_work;
    struct path_point *points;
    struct path_point *point;
    struct path *path;
    NJS_POINT3 rung;
    NJS_POINT3 offset;
    Angle ang;
    int i;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        long_ladder_draw(task);
        return;
    }
    switch (work->mode) {
    case LONG_LADDER_INIT:
        task->destructor = long_ladder_destroy;
        task->draw = long_ladder_draw;
        child = task_create_child(TASK_WORK, path_ledge_step, task);
        if (child != NULL) {
            child_work = child->work;
            PATH_VARS(child_work)->stage_act = stage_get_stage_act();
            child_work->ang.y = work->ang.y;
        }
        offset.x = 0.0F;
        offset.y = FIRST_RUNG;
        offset.z = PATH_Z;
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateY, work->ang.y, ang);
        points = heap_alloc(sizeof(struct path_point) * RUNGS);
        VARS(work)->points = points;
        point = points;
        for (i = 0; i < RUNGS; i++, point++) {
            point->ang_x = 0;
            point->ang_z = 0;
            point->length = RUNG_SPACING;
            njCalcPoint(NULL, &offset, &rung);
            point->pos.x = work->pos.x + rung.x;
            point->pos.y = work->pos.y + rung.y;
            point->pos.z = work->pos.z + rung.z;
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
        /* @bug memory: the path's store reads child->work, MOV.L @(32,R8),R3 at
         * 0x0c912186, without the test of child that guards the stores above
         * (TST R8,R8 at 0x0c9120b4). when task_create_child returns null, it
         * reads address 32 */
        PATH_VARS(child->work)->path = path;
        work->mode = LONG_LADDER_RUN;
        break;
    case LONG_LADDER_RUN:
        break;
    }
    task_step_children(task);
    long_ladder_draw(task);
}

static void long_ladder_destroy(struct task *task)
{
    struct task_work *work = task->work;

    heap_free(VARS(work)->path);
    heap_free(VARS(work)->points);
    task_destroy_children(task);
}

static void long_ladder_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    int i;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y - LADDER_FOOT, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    for (i = 0; i < LADDERS; i++) {
        draw_model(ladder_object->model);
        njTranslate(NULL, 0.0F, LADDER_HEIGHT, 0.0F);
    }
    njPopMatrix(1);
}
