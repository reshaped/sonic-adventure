/* O PYSTAIRS and O BIGSTAIRS, the pyramid's stairs */
#include <ninja.h>

#include "adv03/stairs.h"
#include "camera/camera.h"
#include "draw/tree.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT pyramid_stairs_object;
extern NJS_OBJECT pyramid_stairs_far_object;
extern NJS_OBJECT big_stairs_object;
extern NJS_OBJECT big_stairs_far_object;

/* within NEAR of the camera the stairs draw their near model, and beyond it
 * the far one */
#define NEAR 400.0F

enum stairs_mode { STAIRS_INIT, STAIRS_STANDING };

static void pyramid_stairs_destroy(struct task *task);
static void pyramid_stairs_draw(struct task *task);
static void big_stairs_destroy(struct task *task);
static void big_stairs_draw(struct task *task);

void pyramid_stairs_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case STAIRS_INIT:
        task->draw = pyramid_stairs_draw;
        task->destructor = pyramid_stairs_destroy;
        work->mode = STAIRS_STANDING;
        break;
    case STAIRS_STANDING:
        break;
    }
    pyramid_stairs_draw(task);
}

static void pyramid_stairs_destroy(struct task *task)
{
}

static void pyramid_stairs_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    if (njDistanceP2P(&work->pos, &camera_work->pos) < NEAR) {
        draw_tree(&pyramid_stairs_object);
    } else {
        draw_tree(&pyramid_stairs_far_object);
    }
    njPopMatrix(1);
}

void big_stairs_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case STAIRS_INIT:
        task->draw = big_stairs_draw;
        task->destructor = big_stairs_destroy;
        work->mode = STAIRS_STANDING;
        break;
    case STAIRS_STANDING:
        break;
    }
    big_stairs_draw(task);
}

static void big_stairs_destroy(struct task *task)
{
}

static void big_stairs_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    if (njDistanceP2P(&work->pos, &camera_work->pos) < NEAR) {
        draw_tree(&big_stairs_object);
    } else {
        draw_tree(&big_stairs_far_object);
    }
    njPopMatrix(1);
}
