/* the tap: O TAP, a spout whose water flows as its texture slides */
#include <ninja.h>

#include "collision/collision.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg07/tap.h"
#include "stg07/tap_water.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL tap_model;
/* tap_water_object's model */
extern NJS_MODEL tap_water_model;

enum tap_mode { TAP_START, TAP_RUN, TAP_STILL };

/* how far the water's texture slides along u each step, in the units of
 * its uvs' u */
#define FLOW_SPEED 45

/* clang-format off */
static struct collision_shape tap_shape = {
    SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, -15.0F},
    16.0F, 0.0F, 0.0F, {0}, 0, NULL,
};
/* clang-format on */

static void tap_flow(struct task *task);
static void tap_draw(struct task *task);
static void tap_destroy(struct task *task);

void tap_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case TAP_START:
        task->destructor = tap_destroy;
        task->draw = tap_draw;
        collision_init(task, &tap_shape, COLLISION_OBJECT, 1, 1);
        work->mode = TAP_RUN;
        break;
    case TAP_RUN:
        tap_flow(task);
        tap_draw(task);
        collision_register(work);
        break;
    case TAP_STILL:
        break;
    }
}

/* slide the water's uvs along u. the water model's one meshset holds quads,
 * four uvs to each */
static void tap_flow(struct task *task)
{
    int i;

    /* @hack spelling: the model through its address, which SHC loads again from
     * the pool each pass (MOV.L at 0x0c909c50). through tap_water_model's
     * members, 22 of tap_flow's 27 words differ */
    /* @hack spelling: the count on the left of the test, which SHC compares by
     * CMP/GT R4,R0 (0x0c909c6e). as i < count, 2 of tap_flow's 27 words
     * differ */
    for (i = 0; (&tap_water_model)->meshsets->nbMesh * 4 > i; i++) {
        (&tap_water_model)->meshsets->vertuv[i].tex.u += FLOW_SPEED;
    }
}

static void tap_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&tap_model);
    draw_tree(&tap_water_object);
    njPopMatrix(1);
}

static void tap_destroy(struct task *task)
{
}
