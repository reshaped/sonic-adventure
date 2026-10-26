/* O ROCKET, a ride whose tiers turn under eight rocking arms */
#include <ninja.h>

#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stage/stage.h"
#include "stg03/rocket.h"
#include "stg03/shareobj_textures.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_MODEL rocket_base_model;
extern NJS_OBJECT rocket_tier_0_object;
extern NJS_OBJECT rocket_tier_1_object;
extern NJS_OBJECT rocket_tier_2_object;
extern NJS_OBJECT rocket_tier_3_object;
extern NJS_OBJECT rocket_tier_4_object;
extern NJS_OBJECT rocket_tier_5_object;
extern NJS_OBJECT rocket_tier_6_object;
extern NJS_OBJECT rocket_arm_object;

/* the ride's range for placement_try_leave_by, squared */
#define ROCKET_RANGE (1010.0F * 1010.0F)

/* each step takes ROCKET_TURN off the ride's turn */
#define ROCKET_TURN 0x200
/* the arms rock ROCKET_ROCK_SPEED a step, and reverse once their rock passes
 * ROCKET_ROCK either way */
#define ROCKET_ROCK 0x800
#define ROCKET_ROCK_SPEED 32
/* the arms rise or sink ROCKET_LIFT_SPEED a step, and reverse once their
 * height passes ROCKET_LIFT either way */
#define ROCKET_LIFT 20.0F
#define ROCKET_LIFT_SPEED 0.2F

/* the ride's angles, in task->extra, whose TASK_EXTRA_SIZE bytes hold four.
 * rocket_ang names the three the ride uses */
struct rocket_extra {
    /* @hack type: angs an array, which twinkle_rocket_step reaches through
     * an address register (ADD #8,R4 at 0x0c91487e). as three fields, the
     * step loads them at their offsets in extra and the registers trade.
     * twinkle_rocket_draw builds the same either way, and over the two
     * functions 54 of 443 words differ */
    Angle angs[4];
};

/* the indexes into angs */
enum rocket_ang {
    ROCKET_TURNED, /* the ride's turn */
    ROCKET_ROCKED, /* the arms' rock about z */
    ROCKET_ROCK_RATE /* what each step adds to the arms' rock */
};

/* scl.x holds the arms' height. scl.y holds what each step adds to it */
#define ROCKET_HEIGHT(work) ((work)->scl.x)
#define ROCKET_RISE(work) ((work)->scl.y)

enum rocket_mode { ROCKET_INIT, ROCKET_RUN };

static void twinkle_rocket_draw(struct task *task);
static void twinkle_rocket_destroy(struct task *task);

/* each step turns the ride, rocks its arms, raises or lowers them, and
 * draws the ride */
void twinkle_rocket_step(struct task *task)
{
    struct task_work *work = task->work;
    struct rocket_extra *extra = task->extra;

    /* the step ignores the result, so the ride still runs on the step it
     * leaves range */
    placement_try_leave_by(task, ROCKET_RANGE);
    if (stage_get_stage_act() == STAGE_ACT(STAGE_TWINKLE, 0) ||
        IN_STAGE(STAGE_CART)) {
        switch (work->mode) {
        case ROCKET_INIT:
            work->mode = ROCKET_RUN;
            task->draw = twinkle_rocket_draw;
            task->destructor = twinkle_rocket_destroy;
            extra->angs[ROCKET_ROCK_RATE] = ROCKET_ROCK_SPEED;
            ROCKET_RISE(work) = ROCKET_LIFT_SPEED;
            /* falls through: the ride moves on its first step too */
        case ROCKET_RUN:
            extra->angs[ROCKET_TURNED] -= ROCKET_TURN;
            extra->angs[ROCKET_ROCKED] += extra->angs[ROCKET_ROCK_RATE];
            if (extra->angs[ROCKET_ROCKED] > ROCKET_ROCK ||
                extra->angs[ROCKET_ROCKED] < -ROCKET_ROCK) {
                extra->angs[ROCKET_ROCK_RATE] = -extra->angs[ROCKET_ROCK_RATE];
            }
            ROCKET_HEIGHT(work) += ROCKET_RISE(work);
            if (ROCKET_HEIGHT(work) > ROCKET_LIFT ||
                ROCKET_HEIGHT(work) < -ROCKET_LIFT) {
                ROCKET_RISE(work) = -ROCKET_RISE(work);
            }
            twinkle_rocket_draw(task);
            break;
        }
    }
}

/* draw one tier at its object's offset, turned about y by ang */
#define TIER(object, ang)                                                      \
    (njPushMatrix(NULL), njTranslateV(NULL, (NJS_VECTOR *)(object)->pos),      \
     TURN(njRotateY, ang), draw_model((object)->model), njPopMatrix(1))

/* draw arm at a turn of ang around the ride, rocked about z by rocked.
 * TURN_VIA reads rocked through the caller's local rock */
#define ARM(arm, ang, rocked, rock)                                            \
    (njPushMatrix(NULL), njRotateY(NULL, ang),                                 \
     TURN_VIA(njRotateZ, rocked, rock), draw_tree(arm), njPopMatrix(1))

/* on a frame's first step, draw the base, then the tiers and the arms on
 * it. each tier turns about y by a share of the ride's turn:
 *
 *   tier 0        all of it
 *   tiers 1 and 2 half, the other way
 *   tiers 4 and 5 half
 *   tiers 3 and 6 none
 *
 * the eight arms turn with the ride, at their height. the four at quarter
 * turns rock one way, and the four between them the other */
static void twinkle_rocket_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct rocket_extra *extra = task->extra;
    Angle turn = extra->angs[ROCKET_TURNED];
    Angle half = turn >> 1;
    Angle ang;
    Angle rock;
    NJS_OBJECT *arm;

    if (frame_step != 0) {
        return;
    }
    njSetTexture(&obj_shareobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&rocket_base_model);
    /* @hack redundant: two never-run calls of draw_model, which keep its
     * address in R11 (0x0c91497e); without them, it takes R8, what R8 to R10
     * held moves up a register and 31 of 345 words differ */
    if (0) {
        draw_model(NULL);
        draw_model(NULL);
    }
    /* @hack spelling: tiers 0 and 1 reached through their addresses, as TIER
     * reaches the rest. SHC loads each object's address and adds pos's offset
     * (ADD #8,R5 at 0x0c914986); by name, the pool holds pos's address, and
     * 147 of 345 words and 20 pool words differ */
    njTranslateV(NULL, (NJS_VECTOR *)(&rocket_tier_0_object)->pos);
    njPushMatrix(NULL);
    TURN(njRotateY, turn);
    draw_model((&rocket_tier_0_object)->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)(&rocket_tier_1_object)->pos);
    TURN_VIA(njRotateY, -half, ang);
    draw_model((&rocket_tier_1_object)->model);
    njPopMatrix(1);
    /* @hack spelling: tier 2 turned by ang, which TURN_VIA set to -half. the
     * read keeps njTranslateV's address in R10 and draw_model's in R11
     * (0x0c914944, 0x0c91497e); turned by -half, the two trade and 18 of 345
     * words differ */
    TIER(&rocket_tier_2_object, ang);
    TIER(&rocket_tier_3_object, 0);
    TIER(&rocket_tier_4_object, half);
    TIER(&rocket_tier_5_object, half);
    TIER(&rocket_tier_6_object, 0);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, ROCKET_HEIGHT(work), 0.0F);
    TURN(njRotateY, turn);
    njPushMatrix(NULL);
    TURN_VIA(njRotateZ, extra->angs[ROCKET_ROCKED], rock);
    /* @hack spelling: arm set inside the first arm's draw, which loads
     * &rocket_arm_object into R11 ahead of draw_tree's address (0x0c914a98);
     * set in a statement of its own, the two loads and their pool words trade
     * places, and 2 of 345 words and 4 pool words differ */
    draw_tree(arm = &rocket_arm_object);
    njPopMatrix(1);
    /* @hack spelling: the second arm written out, which loads njRotateY's
     * address into R9 after the arm's njPushMatrix (0x0c914aa8); as ARM, the
     * load moves ahead of that call and 3 of 345 words differ */
    njPushMatrix(NULL);
    njRotateY(NULL, NJM_DEG_ANG(90));
    TURN_VIA(njRotateZ, extra->angs[ROCKET_ROCKED], rock);
    draw_tree(arm);
    njPopMatrix(1);
    ARM(arm, NJM_DEG_ANG(180), extra->angs[ROCKET_ROCKED], rock);
    ARM(arm, NJM_DEG_ANG(270), extra->angs[ROCKET_ROCKED], rock);
    ARM(arm, NJM_DEG_ANG(45), -extra->angs[ROCKET_ROCKED], rock);
    ARM(arm, NJM_DEG_ANG(135), -extra->angs[ROCKET_ROCKED], rock);
    ARM(arm, NJM_DEG_ANG(225), -extra->angs[ROCKET_ROCKED], rock);
    ARM(arm, NJM_DEG_ANG(315), -extra->angs[ROCKET_ROCKED], rock);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void twinkle_rocket_destroy(struct task *task)
{
    task_kill(task);
}
