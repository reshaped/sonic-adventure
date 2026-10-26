/* O EGGMANBED Eggman's bed, which stands on end until switch 1 turns on, then
 * swings down flat */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/eggman_bed.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "object/switch.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: the bed's object as an array, which has SHC pool its address
 * and read its model 4 bytes past it (MOV.L @(4,R4),R4 at 0x0c9244d2); as a
 * plain object, SHC pools the model's address, and 6 of 31 words differ */
extern NJS_OBJECT eggman_bed_object[];

#define BED_SWITCH 1

#define BED_ON_END 0x4000 /* the bed's angle about z standing on end */
#define SWING_STEP 16 /* the speed the swing gains each step */
#define SWING_MOST 0x200 /* the swing's top speed, in angle each step */

struct eggman_bed_vars {
    Uint8 unk_00[8];
    Angle speed; /* the swing's speed */
};

#define VARS(work) ((struct eggman_bed_vars *)&(work)->vars)

enum eggman_bed_mode { BED_INIT, BED_WAIT, BED_DOWN, BED_SWING };

static void eggman_bed_draw(struct task *task);
static void eggman_bed_destroy(struct task *task);
static void eggman_bed_draw_work(struct task_work *work);

static struct collision_shape eggman_bed_shapes[1] = {
    {SHAPE_RECTANGLE2,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {15.0F, 0.0F, 0.0F},
     15.0F,
     10.0F,
     12.0F,
     {0, 0, 0},
     0,
     NULL},
};

void egg_carrier_eggman_bed_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case BED_INIT:
        VARS(work)->speed = 0;
        work->ang.x = 0;
        work->ang.z = BED_ON_END;
        if (!placement_editing(task)) {
            collision_init(task, eggman_bed_shapes, COLLISION_OBJECT, 1, 1);
        }
        task->destructor = eggman_bed_destroy;
        task->draw = eggman_bed_draw;
        work->mode = BED_WAIT;
        break;
    case BED_WAIT:
        /* @hack spelling: an early break, not an if around the store, which
         * lets this case's store of its mode share the swing's (BRA at
         * 0x0c9243ea to 0x0c924442); as an if, 2 of 90 words differ */
        if (!switch_is_on(BED_SWITCH)) {
            break;
        }
        work->mode = BED_SWING;
        break;
    case BED_SWING:
        VARS(work)->speed += SWING_STEP;
        if (VARS(work)->speed > SWING_MOST) {
            VARS(work)->speed = SWING_MOST;
        }
        work->ang.z -= VARS(work)->speed;
        /* @hack spelling: an early break, not an if around the stores, whose
         * store of the mode BED_WAIT's shares (0x0c924442); as an if, 4 of 90
         * words differ */
        if (work->ang.z > 0) {
            break;
        }
        work->ang.z = 0;
        work->mode = BED_DOWN;
        break;
    case BED_DOWN:
        break;
    }
    eggman_bed_draw_work(work);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void eggman_bed_draw(struct task *task)
{
    eggman_bed_draw_work(task->work);
}

static void eggman_bed_destroy(struct task *task)
{
}

static void eggman_bed_draw_work(struct task_work *work)
{
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    draw_model(eggman_bed_object->model);
    njPopMatrix(1);
}
