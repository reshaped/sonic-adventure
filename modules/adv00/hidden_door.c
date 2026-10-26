/* O HIDDENDOOR door whose two leaves slide apart when its switch is on, and
 * close again after a set time */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/hidden_door.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "object/switch.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the door, whose child and that child's sibling are the leaves at -x and +x */
extern NJS_OBJECT hidden_door_object;

/* each leaf's width. shut, the leaves meet in the middle; open, each has
 * slid out its width */
#define LEAF_WIDTH 15.0F
/* shut, each leaf's center lies this far out from the middle */
#define LEAF_HALF (LEAF_WIDTH / 2.0F)
/* how far the leaves slide a step, opening and closing */
#define OPEN_SPEED 1.0F
#define CLOSE_SPEED 0.1F

enum hidden_door_mode {
    DOOR_SHUT = 0,
    DOOR_OPENING = 1,
    DOOR_OPEN = 2,
    DOOR_CLOSING = 3
};

struct hidden_door_vars {
    Float slide; /* how far each leaf has slid out, from 0 to LEAF_WIDTH */
    int waited; /* steps it has waited open */
    int open_steps; /* steps it waits open */
};

#define VARS(work) ((struct hidden_door_vars *)&(work)->vars)
#define SWITCH_ID(work) ((work)->timer)

/* the leaves at -x and +x, a slab under the doorway, and a wall across
 * both leaves' travel */
/* clang-format off */
static struct collision_shape hidden_door_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-7.5F, 13.5F, 0.0F},
     7.5F, 13.5F, 0.5F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {7.5F, 13.5F, 0.0F},
     7.5F, 13.5F, 0.5F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, -1.0F, 0.0F},
     15.0F, 1.0F, 1.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 13.5F, 0.0F},
     30.0F, 13.5F, 1.0F, {0}, 0, NULL},
};
/* clang-format on */

static void hidden_door_draw(struct task *task)
{
    /* @hack spelling: object declared ahead of work, which with the discarded
     * read of slide below gives njPushMatrix's address R12 and njPopMatrix's
     * R11 (0x0c90fa56, 0x0c90faaa). declared after work, the two trade and 8 of
     * 89 words differ */
    NJS_OBJECT *object = &hidden_door_object;
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_model(object->model);
    object = object->child;
    njPushMatrix(NULL);
    njTranslate(NULL, -VARS(work)->slide - LEAF_WIDTH, object->pos[1],
                object->pos[2]);
    draw_model(object->model);
    njPopMatrix(1);
    object = object->sibling;
    njPushMatrix(NULL);
    /* @hack redundant: a read of slide that a constant ?: discards, which loads
     * slide (0x0c90fac8) ahead of LEAF_WIDTH (0x0c90faca), and with object
     * declared ahead of work gives njPushMatrix's address R12. as LEAF_WIDTH +
     * slide, 12 of 89 words differ */
    njTranslate(NULL,
                ((void)(0 ? VARS(work)->slide : 0.0F), LEAF_WIDTH) +
                    VARS(work)->slide,
                object->pos[1], object->pos[2]);
    draw_model(object->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

/* turn the slab's and the leaves' shapes on, each leaf's where slide puts it,
 * and the wall's off */
static void hidden_door_place_leaves(struct task_work *work)
{
    struct collision_shape *shapes = work->collision->shapes;
    Float slide = VARS(work)->slide;

    shapes[0].flags &= ~SHAPE_OFF;
    shapes[1].flags &= ~SHAPE_OFF;
    shapes[2].flags &= ~SHAPE_OFF;
    shapes[3].flags |= SHAPE_OFF;
    /* @hack spelling: the store through work->collision, which reloads the
     * shapes' address (0x0c90fb20). through shapes, 10 of 36 words differ */
    work->collision->shapes[0].center.x = -LEAF_HALF - slide;
    /* @hack spelling: the store through work->collision, as above (0x0c90fb2a).
     * through shapes, 29 of 36 words differ */
    /* @hack redundant: LEAF_HALF chosen by a constant ?:, which sums into FR3
     * for the store at 0x0c90fb38. as LEAF_HALF + slide, the sum takes FR4 and
     * 2 of 36 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    work->collision->shapes[1].center.x =
        (1.0F ? LEAF_HALF : LEAF_HALF) + slide;
    /* NOLINTEND(misc-redundant-expression) */
}

/* turn the slab's and the wall's shapes on, and the leaves' off */
static void hidden_door_place_wall(struct task_work *work)
{
    struct collision_shape *shapes = work->collision->shapes;

    shapes[0].flags |= SHAPE_OFF;
    shapes[1].flags |= SHAPE_OFF;
    shapes[2].flags &= ~SHAPE_OFF;
    shapes[3].flags &= ~SHAPE_OFF;
}

/* open the door once its switch is on, and shut it again once it has waited
 * open. in the set editor, print the switch's id beside scl.x's row, and a
 * label beside scl.y's */
static void hidden_door_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        SWITCH_ID(work) = (int)njAbs(work->scl.x);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> Switch ID:");
        njPrintD(NJM_LOCATION(30, EDITOR_ROW_SCL_X), (Uint16)SWITCH_ID(work),
                 4);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> OPEN TIME");
        hidden_door_draw(task);
        return;
    }
    hidden_door_draw(task);
    switch (work->mode) {
    case DOOR_SHUT:
        if (switch_is_on((Uint16)SWITCH_ID(work))) {
            work->mode = DOOR_OPENING;
        }
        hidden_door_place_wall(work);
        break;
    case DOOR_OPENING:
        VARS(work)->slide += OPEN_SPEED;
        if (VARS(work)->slide >= LEAF_WIDTH) {
            VARS(work)->slide = LEAF_WIDTH;
            VARS(work)->waited = 0;
            work->mode = DOOR_OPEN;
            hidden_door_place_leaves(work);
        }
        break;
    case DOOR_OPEN:
        VARS(work)->waited++;
        if (VARS(work)->waited >= VARS(work)->open_steps) {
            VARS(work)->waited = 0;
            work->mode = DOOR_CLOSING;
        }
        break;
    case DOOR_CLOSING:
        VARS(work)->slide -= CLOSE_SPEED;
        if (VARS(work)->slide <= 0.0F) {
            VARS(work)->slide = 0.0F;
            work->mode = DOOR_SHUT;
        }
        hidden_door_place_leaves(work);
        break;
    }
    collision_register(work);
}

void hidden_door_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    VARS(work)->slide = 0.0F;
    SWITCH_ID(work) = (int)njAbs(work->scl.x);
    VARS(work)->open_steps = (int)njAbs(work->scl.y);
    switch_set((Uint16)SWITCH_ID(work), FALSE);
    if (!placement_editing(task)) {
        collision_init(task, hidden_door_shapes, COLLISION_OBJECT, 1, 4);
    }
    task->step = hidden_door_step;
    task->draw = hidden_door_draw;
}
