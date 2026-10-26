/* o_untei1, o_untei2, o_untei3 and o_untei6: monkey bars, each with a path
 * that players hang from */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "path/path.h"
#include "path/steps.h"
#include "stage/stage.h"
#include "stg05/monkey_bars.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_OBJECT monkey_bars_1_object;
extern NJS_OBJECT monkey_bars_2_object;
extern NJS_OBJECT monkey_bars_3_object;
extern NJS_OBJECT monkey_bars_6_object;

/* points and path are the bars' path. init allocates both from the heap,
 * and the destructor frees them. work->scl is the path's first point
 * relative to the bars, before their angles turn it */
struct monkey_bars_vars {
    Uint8 unk_00[4];
    struct path_point *points;
    struct path *path;
};

#define VARS(work) ((struct monkey_bars_vars *)&(work)->vars)
#define KIND(work) ((work)->submode)
#define POINT_COUNT(work) TASK_TIMER(work)

/* the bars pass from BARS_WAIT to BARS_TOUCHED once work->flags is nonzero.
 * the two modes do the same */
enum monkey_bars_mode { BARS_INIT, BARS_WAIT, BARS_TOUCHED };

enum monkey_bars_kind { BARS_1, BARS_2, BARS_3, BARS_6 };
#define BARS_SPACING 6.0F /* between the path's points */

/* indexed by monkey_bars_kind, as monkey_bars_objects is */
static struct collision_shape monkey_bars_shapes[4] = {
    {SHAPE_CAPSULE,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, -51.0F},
     5.0F,
     51.0F,
     0.0F,
     {0x4000, 0, 0},
     0,
     NULL},
    {SHAPE_CAPSULE,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, -51.0F},
     5.0F,
     51.0F,
     0.0F,
     {0x4000, 0, 0},
     0,
     NULL},
    {SHAPE_CAPSULE,
     0,
     PUSH_FIRM(PUSH_ALL),
     {-10.0F, 0.0F, 0.0F},
     5.0F,
     6.0F,
     0.0F,
     {0x4000, 0, 0},
     0,
     NULL},
    {SHAPE_CAPSULE,
     0,
     PUSH_FIRM(PUSH_ALL),
     {-10.0F, 0.0F, 0.0F},
     5.0F,
     15.0F,
     0.0F,
     {0x4000, 0, 0},
     0,
     NULL},
};
static NJS_OBJECT *monkey_bars_objects[4] = {
    &monkey_bars_1_object,
    &monkey_bars_2_object,
    &monkey_bars_3_object,
    &monkey_bars_6_object,
};

static void monkey_bars_step(struct task *task);
static void monkey_bars_draw(struct task *task);
static void monkey_bars_destroy(struct task *task);

/* @hack spelling: each kind's step reads task->work again for every store;
 * through a local work, each step is 10 bytes shorter and 96 of the four
 * steps' 100 words differ */
void monkey_bars_1_step(struct task *task)
{
    /* @hack spelling: a switch of one case, which tests the mode with CMP/EQ
     * #0,R0 (0x0c904da4); as an if, 5 of 24 words differ */
    switch (task->work->mode) {
    case BARS_INIT:
        KIND(task->work) = BARS_1;
        POINT_COUNT(task->work) = 17;
        task->work->scl.x = 0.0F;
        task->work->scl.y = 0.0F;
        task->work->scl.z = -3.0F;
        break;
    }
    monkey_bars_step(task);
}

void monkey_bars_2_step(struct task *task)
{
    /* @hack spelling: the same switch of one case, whose CMP/EQ #0,R0 is at
     * 0x0c904dd4; as an if, 5 of 24 words differ */
    switch (task->work->mode) {
    case BARS_INIT:
        KIND(task->work) = BARS_2;
        POINT_COUNT(task->work) = 17;
        task->work->scl.x = 0.0F;
        task->work->scl.y = 0.0F;
        task->work->scl.z = -3.0F;
        break;
    }
    monkey_bars_step(task);
}

void monkey_bars_3_step(struct task *task)
{
    /* @hack spelling: the same switch of one case, whose CMP/EQ #0,R0 is at
     * 0x0c904e04; as an if, 5 of 26 words differ */
    switch (task->work->mode) {
    case BARS_INIT:
        KIND(task->work) = BARS_3;
        POINT_COUNT(task->work) = 2;
        task->work->scl.x = -10.0F;
        task->work->scl.y = 0.0F;
        task->work->scl.z = 3.0F;
        break;
    }
    monkey_bars_step(task);
}

void monkey_bars_6_step(struct task *task)
{
    /* @hack spelling: the same switch of one case, whose CMP/EQ #0,R0 is at
     * 0x0c904e38; as an if, 5 of 26 words differ */
    switch (task->work->mode) {
    case BARS_INIT:
        KIND(task->work) = BARS_6;
        POINT_COUNT(task->work) = 5;
        task->work->scl.x = -10.0F;
        task->work->scl.y = 0.0F;
        task->work->scl.z = 12.0F;
        break;
    }
    monkey_bars_step(task);
    /* @hack redundant: a return where control returns anyway, which keeps the
     * BRA to monkey_bars_step at 0x0c904e64; without it, the code falls
     * through into monkey_bars_step, which follows, and 2 of 26 words
     * differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

/* unless the set editor holds the bars, init lays the path's points
 * BARS_SPACING apart along the bars' -z. a child task running path_hang_step
 * hangs players from the path */
static void monkey_bars_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    NJS_POINT3 out;
    NJS_POINT3 offset;
    Angle ang;
    struct path_point *points;
    struct path *path;
    int i;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case BARS_INIT:
            work->mode = BARS_WAIT;
            collision_init(task, &monkey_bars_shapes[KIND(work)],
                           COLLISION_OBJECT, 1, 1);
            child = task_create_child(TASK_WORK, path_hang_step, task);
            PATH_VARS(child->work)->stage_act = stage_get_stage_act();
            offset.x = work->scl.x;
            offset.y = work->scl.y;
            offset.z = work->scl.z;
            njPushMatrix(&_unit_matrix_);
            TURN_VIA(njRotateZ, work->ang.z, ang);
            TURN_VIA(njRotateX, work->ang.x, ang);
            TURN_VIA(njRotateY, work->ang.y, ang);
            points = heap_alloc(POINT_COUNT(work) * sizeof(struct path_point));
            VARS(work)->points = points;
            for (i = 0; i < POINT_COUNT(work); i++) {
                points[i].ang_x = work->ang.x;
                points[i].ang_z = work->ang.z;
                points[i].length = BARS_SPACING;
                njCalcPoint(NULL, &offset, &out);
                points[i].pos.x = work->pos.x + out.x;
                points[i].pos.y = work->pos.y + out.y;
                points[i].pos.z = work->pos.z + out.z;
                offset.z -= BARS_SPACING;
            }
            njPopMatrix(1);
            path = heap_alloc(sizeof(struct path));
            VARS(work)->path = path;
            path->ends = PATH_STOP;
            path->count = POINT_COUNT(work);
            path->length = (POINT_COUNT(work) - 1) * BARS_SPACING;
            path->points = points;
            path->step = path_hang_step;
            PATH_VARS(child->work)->path = path;
            task->draw = monkey_bars_draw;
            task->destructor = monkey_bars_destroy;
            break;
        case BARS_WAIT:
            if (work->flags == 0) {
                break;
            }
            work->mode = BARS_TOUCHED;
            break;
        case BARS_TOUCHED:
            break;
        }
        task_step_children(task);
        collision_register(work);
    }
    monkey_bars_draw(task);
}

static void monkey_bars_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = monkey_bars_objects[KIND(work)];
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 200.0F)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(object);
        njPopMatrix(1);
    }
}

static void monkey_bars_destroy(struct task *task)
{
    struct task_work *work = task->work;

    heap_free(VARS(work)->path);
    heap_free(VARS(work)->points);
}
