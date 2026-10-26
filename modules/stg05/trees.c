/* O TREE1, a standing tree, and O TREE2, a fallen one. each is solid by a
 * capsule. the set entry scales both the tree and its capsule */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg05/trees.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_OBJECT mountain_tree_1_object;
extern NJS_OBJECT mountain_tree_2_object;

/* a tree draws at the set entry's scales plus TREE_BASE_SCALE. unless the
 * set editor holds the tree, its first step adds it to work->scl. while the
 * set editor holds the tree, each draw adds it instead */
#define TREE_BASE_SCALE 1.0F

#define TREE_1_DRAW_RADIUS 100.0F
#define TREE_2_DRAW_RADIUS 200.0F

static void mountain_tree_1_run(struct task *task);
static void mountain_tree_1_draw(struct task *task);
static void mountain_tree_1_destroy(struct task *task);
static void mountain_tree_2_run(struct task *task);
static void mountain_tree_2_draw(struct task *task);
static void mountain_tree_2_destroy(struct task *task);

/* @hack type: an array of one, whose address mountain_tree_1_step loads once
 * for both its calls (0x0c911d60); as a plain struct, 97 of 119 words
 * differ */
static struct collision_shape mountain_tree_1_shape[1] = {{
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {-0.18F, 4.375F, -0.03F},
    1.4F,
    17.5F,
    5.0F,
    {0, 0, 0},
    0,
    NULL,
}};
/* the fallen trunk's capsule. its angle of 100 degrees about z lays it near
 * level */
/* @hack type: an array of one, whose address mountain_tree_2_step loads once
 * for both its calls (0x0c911f6e); as a plain struct, 97 of 115 words
 * differ */
static struct collision_shape mountain_tree_2_shape[1] = {{
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {11.0F, 2.0F, 0.0F},
    1.0F,
    10.0F,
    0.0F,
    {0, 0, 0x471c},
    0,
    NULL,
}};

/* the trunk stands along y. with each of its set entry's scales 0, the
 * capsule stays as it is. otherwise the scales, each plus TREE_BASE_SCALE,
 * scale it. y scales its length, and half of x times z its radius */
void mountain_tree_1_step(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    if (!placement_editing(task)) {
        work->scl.x += TREE_BASE_SCALE;
        work->scl.y += TREE_BASE_SCALE;
        work->scl.z += TREE_BASE_SCALE;
        if (work->scl.x == TREE_BASE_SCALE && work->scl.y == TREE_BASE_SCALE &&
            work->scl.z == TREE_BASE_SCALE) {
            collision_init(task, mountain_tree_1_shape, COLLISION_OBJECT, 1, 1);
        } else {
            collision_init(task, mountain_tree_1_shape, COLLISION_OBJECT, 1, 1);
            work->collision->shapes->center.x *= work->scl.x;
            work->collision->shapes->center.y *= work->scl.y;
            work->collision->shapes->center.z *= work->scl.z;
            work->collision->shapes->size_y *= work->scl.y;
            work->collision->shapes->radius *= work->scl.x * work->scl.z * 0.5F;
        }
    }
    task->step = mountain_tree_1_run;
    task->draw = mountain_tree_1_draw;
    task->destructor = mountain_tree_1_destroy;
}

static void mountain_tree_1_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        if (!placement_editing(task)) {
            collision_register(work);
        }
        mountain_tree_1_draw(task);
    }
}

static void mountain_tree_1_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                     TREE_1_DRAW_RADIUS)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        if (placement_editing(task)) {
            njScale(NULL, work->scl.x + TREE_BASE_SCALE,
                    work->scl.y + TREE_BASE_SCALE,
                    work->scl.z + TREE_BASE_SCALE);
        } else {
            njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
        }
        draw_model(mountain_tree_1_object.model);
        njPopMatrix(1);
    }
}

static void mountain_tree_1_destroy(struct task *task)
{
}

/* the trunk lies along x. with each of its set entry's scales 0, the
 * capsule stays as it is. otherwise the scales, each plus TREE_BASE_SCALE,
 * scale it. x scales its length, and half of y times z its radius */
void mountain_tree_2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        work->scl.x += TREE_BASE_SCALE;
        work->scl.y += TREE_BASE_SCALE;
        work->scl.z += TREE_BASE_SCALE;
        if (work->scl.x == TREE_BASE_SCALE && work->scl.y == TREE_BASE_SCALE &&
            work->scl.z == TREE_BASE_SCALE) {
            collision_init(task, mountain_tree_2_shape, COLLISION_OBJECT, 1, 1);
        } else {
            collision_init(task, mountain_tree_2_shape, COLLISION_OBJECT, 1, 1);
            work->collision->shapes->center.x *= work->scl.x;
            work->collision->shapes->center.y *= work->scl.y;
            work->collision->shapes->center.z *= work->scl.z;
            work->collision->shapes->size_y *= work->scl.x;
            work->collision->shapes->radius *= work->scl.y * work->scl.z * 0.5F;
        }
    }
    task->step = mountain_tree_2_run;
    task->draw = mountain_tree_2_draw;
    task->destructor = mountain_tree_2_destroy;
}

static void mountain_tree_2_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        if (!placement_editing(task)) {
            collision_register(work);
        }
        mountain_tree_2_draw(task);
    }
}

static void mountain_tree_2_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                     TREE_2_DRAW_RADIUS)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        if (placement_editing(task)) {
            njScale(NULL, work->scl.x + TREE_BASE_SCALE,
                    work->scl.y + TREE_BASE_SCALE,
                    work->scl.z + TREE_BASE_SCALE);
        } else {
            njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
        }
        draw_model(mountain_tree_2_object.model);
        njPopMatrix(1);
    }
}

static void mountain_tree_2_destroy(struct task *task)
{
}
