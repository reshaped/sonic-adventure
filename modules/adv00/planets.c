/* O WAKUSEI planet model on a stand, whose four parts turn */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/planets.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the stand, whose children are the four turning parts */
extern NJS_OBJECT planets_object;

/* the share of a whole turn the parts turn each step */
#define PLANETS_SPEED 0.005F

/* a whole turn, in Angle's units */
#define WHOLE_TURN 65536.0F

struct planets_vars {
    Float turn; /* the parts' angle, as a share of a whole turn */
};

#define VARS(work) ((struct planets_vars *)&(work)->vars)

static void planets_step(struct task *task);
static void planets_draw(struct task *task);

/* clang-format off */
static struct collision_shape planets_shape = {
    SHAPE_SPHERE | SHAPE_UNTURNED, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
    {0.0F, 30.0F, 0.0F}, 30.0F, 0.0F, 0.0F, {0}, 0, NULL,
};
/* clang-format on */

void planets_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    work->ang.x = 0;
    work->ang.y = 0;
    work->ang.z = 0;
    work->mode = 0;
    VARS(work)->turn = 0.0F;
    motion->aim.x = 0;
    motion->aim.y = 0;
    motion->aim.z = 0;
    if (!placement_editing(task)) {
        collision_init(task, &planets_shape, COLLISION_OBJECT, 1, 1);
    }
    task->step = planets_step;
    task->draw = planets_draw;
}

static void planets_step(struct task *task)
{
    struct task_work *work = task->work;
    Float turn;

    if (!placement_try_leave(task)) {
        if (!placement_editing(task)) {
            collision_register(work);
        }
        turn = VARS(work)->turn + PLANETS_SPEED;
        if (turn >= 1.0F) {
            turn -= 1.0F;
        }
        VARS(work)->turn = turn;
        planets_draw(task);
    }
}

/* draw the stand, then its four parts, each turned by spin. spin replaces the
 * second part's node's y angle and the last two parts' z angles. the first
 * part turns by spin about y alone, without its node's angles */
static void planets_draw(struct task *task)
{
    NJS_OBJECT *object;
    NJS_OBJECT *root;
    Angle spin;
    Angle ang;
    struct task_work *work = task->work;

    spin = (Angle)(VARS(work)->turn * WHOLE_TURN);
    root = &planets_object;
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(root->model);
    object = root->child;
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    TURN(njRotateY, spin);
    draw_model(object->model);
    njPopMatrix(1);
    object = object->sibling;
    /* @hack redundant: a never-run test of object, which keeps object in R13
     * and draw_model's address in R10 (0x0c90a98e, 0x0c90a988), as the image
     * has them; without it, the registers rotate and 36 of 189 words differ */
    if (0) {
        if (object) {
        }
    }
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    TURN_VIA(njRotateZ, object->ang[2], ang);
    TURN(njRotateY, spin);
    TURN_VIA(njRotateX, object->ang[0], ang);
    draw_model(object->model);
    njPopMatrix(1);
    object = object->sibling;
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    TURN_VIA(njRotateX, object->ang[0], ang);
    TURN_VIA(njRotateY, object->ang[1], ang);
    TURN(njRotateZ, spin);
    draw_model(object->model);
    njPopMatrix(1);
    object = object->sibling;
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    TURN_VIA(njRotateX, object->ang[0], ang);
    TURN_VIA(njRotateY, object->ang[1], ang);
    TURN(njRotateZ, spin);
    draw_model(object->model);
    njPopMatrix(1);
    njPopMatrix(1);
}
