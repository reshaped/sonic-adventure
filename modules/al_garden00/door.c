/* O AR_DOOR, the garden's folding door, and the copy of the garden's flags */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "al_garden00/door.h"
#include "al_garden00/textures.h"
#include "chao/chao.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "progress/progress.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_ACTION garden_door_action[]; /* the panels folding open */

#define DOOR_SPEED 0.02F /* the action frames the door plays each step */
#define DOOR_WAIT 90 /* steps it stays open, which work->tick counts down */
#define DOOR_TURN ((Float)NJM_DEG_ANG(70)) /* each panel's turn once open */
#define PANEL_HALF 6.25F /* half a panel's width */
/* half the doorway's width. each pair of panels hinges on the inner face of
 * a post this far from the middle */
#define DOORWAY_HALF 25.0F

#define POSTS_RADIUS 35.0F

/* how many of event_flags, from the first, are the garden's, and the
 * offset of their copy in chao_get_file_data's bytes */
#define GARDEN_FLAGS 10
#define GARDEN_FLAGS_AT 0x100

struct garden_door_vars {
    Float frame; /* garden_door_action's frame: 0 shut, its last open */
};

#define VARS(work) ((struct garden_door_vars *)&(work)->vars)

static void garden_door_close(struct task *task);
static void garden_door_wait(struct task *task);
static void garden_door_open(struct task *task);
static void garden_door_idle(struct task *task);
static void garden_door_destroy(struct task *task);
static void garden_door_posts_init(struct task *task);
static void garden_door_posts_step(struct task *task);

/* the four panels, shut, from the left. garden_door_turn moves them */
/* clang-format off */
static struct collision_shape garden_door_shapes[4] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-18.75F, 22.5F, 0.0F},
     PANEL_HALF, 22.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-6.25F, 22.5F, 0.0F},
     PANEL_HALF, 22.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {6.25F, 22.5F, 0.0F},
     PANEL_HALF, 22.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {18.75F, 22.5F, 0.0F},
     PANEL_HALF, 22.5F, 1.0F, {0}, 0, NULL},
};
/* the posts either side of the doorway, which the panels hinge on */
static struct collision_shape garden_door_posts[2] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-27.5F, 12.5F, 0.0F},
     2.5F, 12.5F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {27.5F, 12.5F, 0.0F},
     2.5F, 12.5F, 1.0F, {0}, 0, NULL},
};
/* clang-format on */

/* TRUE while a player touches the door */
static Bool garden_door_touched(struct task_work *work)
{
    return (work->flags & COLLISION_TOUCHING) &&
           work->collision->hit->kind == COLLISION_PLAYER;
}

static void garden_door_draw(struct task *task)
{
    struct task_work *work = task->work;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njSetTexture(&garden_objects_texlist);
    draw_action(garden_door_action, VARS(work)->frame);
    njPopMatrix(1);
}

/* set the panels' shapes from the action's frame, and register them. each
 * panel turns by the frame's share of DOOR_TURN. the two panels of a pair
 * turn opposite ways, so the pair folds toward its post */
static void garden_door_turn(struct task_work *work)
{
    struct collision_shape *shapes = work->collision->shapes;
    Angle ang = floorf(VARS(work)->frame * DOOR_TURN /
                       (garden_door_action->motion->nbFrame - 1));
    Float x = njCos(ang) * PANEL_HALF;
    Float z = njSin(ang) * PANEL_HALF;

    shapes[3].center.x = -(shapes[0].center.x = x - DOORWAY_HALF);
    shapes[2].center.x = -(shapes[1].center.x = (x * 3.0F) - DOORWAY_HALF);
    shapes[0].center.z = shapes[1].center.z = shapes[2].center.z =
        shapes[3].center.z = z;
    shapes[2].ang.y = shapes[0].ang.y = -ang;
    shapes[3].ang.y = shapes[1].ang.y = ang;
    collision_register(work);
}

static void garden_door_close(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: last, the action's last frame, which the image
     * computes at the start (0x0cb80aea to 0x0cb80afe) and never reads;
     * without it, 51 of 52 words differ */
    Float last = garden_door_action->motion->nbFrame - 1;

    if (placement_try_leave(task)) {
        return;
    }
    garden_door_draw(task);
    if (!garden_door_touched(work)) {
        if ((VARS(work)->frame -= DOOR_SPEED) <= 0.0F) {
            VARS(work)->frame = 0.0F;
            task->step = garden_door_idle;
        }
    } else {
        task->step = garden_door_open;
    }
    garden_door_turn(work);
    task_step_children(task);
}

static void garden_door_wait(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    garden_door_draw(task);
    if (!--work->tick) {
        task->step = garden_door_close;
    }
    garden_door_turn(work);
    task_step_children(task);
}

static void garden_door_open(struct task *task)
{
    struct task_work *work = task->work;
    Float last = garden_door_action->motion->nbFrame - 1;

    if (placement_try_leave(task)) {
        return;
    }
    garden_door_draw(task);
    if ((VARS(work)->frame += DOOR_SPEED) >= last) {
        VARS(work)->frame = last;
        work->tick = DOOR_WAIT;
        task->step = garden_door_wait;
    }
    garden_door_turn(work);
    task_step_children(task);
}

static void garden_door_idle(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_editing(task)) {
        garden_door_draw(task);
    } else {
        if (placement_try_leave(task)) {
            return;
        }
        garden_door_draw(task);
        if (garden_door_touched(work)) {
            task->step = garden_door_open;
        }
        garden_door_turn(work);
        task_step_children(task);
    }
}

static void garden_door_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* in the set editor, the door takes no shapes and makes no posts */
void garden_door_init(struct task *task)
{
    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_init(task, garden_door_shapes, COLLISION_OBJECT, 1, 4);
        task_create_child(TASK_WORK, garden_door_posts_init, task);
    }
    task->step = garden_door_idle;
    task->draw = garden_door_draw;
    task->destructor = garden_door_destroy;
}

static void garden_door_posts_init(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, garden_door_posts, COLLISION_OBJECT, 1, 2);
    work->collision->radius = POSTS_RADIUS;
    task->step = garden_door_posts_step;
}

static void garden_door_posts_step(struct task *task)
{
    collision_register(task->work);
}

void garden_flags_clear(void)
{
    Uint8 *data = chao_get_file_data();
    Uint8 *start = &data[GARDEN_FLAGS_AT];
    Uint8 *saved;
    int i;

    /* @hack redundant: a never-run test of data, which keeps data in R4 and
     * builds start in R0 (0x0cb80d6a, 0x0cb80d70), as the image does; without
     * it, start is built in R4 and 5 of 25 words differ */
    if (0) {
        if (data) {
        }
    }
    saved = start;

    for (i = 0; i < GARDEN_FLAGS; i++, saved++) {
        event_flags[i] = 0;
        *saved = 0;
    }
}

void garden_flags_load(void)
{
    Uint8 *data = chao_get_file_data();
    Uint8 *saved = &data[GARDEN_FLAGS_AT];
    int i;

    for (i = 0; i < GARDEN_FLAGS; i++, saved++) {
        event_flags[i] = *saved;
    }
}

void garden_flags_save(void)
{
    Uint8 *data = chao_get_file_data();
    Uint8 *saved = &data[GARDEN_FLAGS_AT];
    int i;

    for (i = 0; i < GARDEN_FLAGS; i++, saved++) {
        *saved = event_flags[i];
    }
}
