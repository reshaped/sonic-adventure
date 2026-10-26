/* O KNUTREE, a solid tree with a near model and a far one */
#include <ninja.h>

#include "adv03/knuckles_tree.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT knuckles_tree_object;
extern NJS_OBJECT knuckles_tree_far_object;

/* within NEAR of the camera the tree draws its near model, and beyond it the
 * far one */
#define NEAR 1000.0F

enum knuckles_tree_mode { TREE_INIT, TREE_STANDING };

/* a cylinder about its trunk, which pushes every group back and which none
 * can push */
static struct collision_shape knuckles_tree_shape[1] = {
    {SHAPE_CYLINDER | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 25.0F, 0.0F},
     10.0F,
     25.0F,
     0.0F,
     {0},
     0,
     NULL},
};

static void knuckles_tree_destroy(struct task *task);
static void knuckles_tree_draw(struct task *task);

void knuckles_tree_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_editing(task)) {
        knuckles_tree_draw(task);
        return;
    }
    switch (work->mode) {
    case TREE_INIT:
        collision_init(task, knuckles_tree_shape, COLLISION_OBJECT, 1, 1);
        /* @bug logic: stores the draw in task->destructor, where task->draw
         * is meant, and the next line stores the destructor over it. the
         * image keeps only the load of knuckles_tree_draw's address (MOV.L
         * at 0x0c901afe). task->draw stays NULL, so the tree doesn't show under
         * the pause menu */
        task->destructor = knuckles_tree_draw;
        task->destructor = knuckles_tree_destroy;
        work->mode = TREE_STANDING;
        break;
    case TREE_STANDING:
        break;
    }
    knuckles_tree_draw(task);
    collision_register(work);
}

static void knuckles_tree_destroy(struct task *task)
{
}

static void knuckles_tree_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    if (njDistanceP2P(&work->pos, &camera_work->pos) < NEAR) {
        draw_tree(&knuckles_tree_object);
    } else {
        draw_tree(&knuckles_tree_far_object);
    }
    njPopMatrix(1);
}
