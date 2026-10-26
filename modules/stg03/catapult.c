/* the catapult, which the set editor names O CATAPULT. a band between two
 * sliding posts catches player 0, and A or B launches it */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "debug/editor.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg03/catapult.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* the catapult's action. its root's child and that child's sibling are the
 * two posts, and each post's child is its arm */
extern NJS_ACTION catapult_action;

/* the catapult destroys itself once neither the camera nor player 0 is
 * within 610 units */
#define CATAPULT_RANGE (610.0F * 610.0F)

#define SE_CATAPULT_FIRE 68

/* the posts close in on player 0 once it comes within CATAPULT_NEAR units.
 * each post slides along the catapult's x axis, within POST_REACH of the
 * center. the posts keep POST_GAP apart, and close in to POST_HOLD either
 * side of player 0 */
#define CATAPULT_NEAR 100.0F
#define POST_REACH 54.27F
#define POST_GAP 18.0F
#define POST_HOLD 9.0F

/* the catching box, by its half sizes along the catapult's x, z and y axes.
 * a caught player is pulled in along x to within HOLD_X of the center */
#define CATCH_X POST_REACH
#define CATCH_Z 21.08F
#define CATCH_Y 50.0F
#define HOLD_X 41.5F

/* while the catapult holds player 0, the arms swing ARM_SWING either way,
 * ARM_SWING_SPEED a step. otherwise they turn back to 0, ARM_RETURN_SPEED a
 * step */
#define ARM_SWING NJM_DEG_ANG(45)
#define ARM_SWING_SPEED NJM_DEG_ANG(1)
#define ARM_RETURN_SPEED NJM_DEG_ANG(2)

/* a catapult launches player 0 at its set entry's scale x plus BASE_SPEED.
 * player 0 is then beyond its control for LAUNCH_STEPS steps */
#define BASE_SPEED 6.0F
#define LAUNCH_STEPS 30

struct catapult_vars {
    Uint8 unk_00[4];
    NJS_ACTION *action;
    Uint8 unk_08[4];
};

#define VARS(work) ((struct catapult_vars *)&(work)->vars)
#define FIRST_POST(work) (VARS(work)->action->object->child)
/* bits of work->tick, one for each post. a set bit slides the post toward
 * -x, and a clear one toward +x */
#define POST_WAYS(work) ((work)->tick)
#define FIRST_POST_BACK 0x2
#define SECOND_POST_BACK 0x1
/* the catapult's work->scl. x is the launch speed. y is the frame of the
 * action the catapult draws, and z the action's frame count */
#define CATAPULT_SPEED(work) ((work)->scl.x)
#define CATAPULT_FRAME(work) ((work)->scl.y)
#define CATAPULT_FRAMES(work) ((work)->scl.z)

enum catapult_mode {
    CATAPULT_INIT,
    CATAPULT_SLIDE, /* the posts slide to and fro */
    CATAPULT_NEAR_PLAYER, /* the posts close in on player 0 */
    CATAPULT_CATCH, /* the catapult pulls player 0 in */
    CATAPULT_HOLD, /* the catapult holds player 0 until A or B is pressed */
    /* the catapult launches player 0 each step it is in the catching box */
    CATAPULT_FIRE,
    CATAPULT_IDLE /* no code sets it */
};
/* which way the arms swing, in work->submode. no code sets ARM_STILL */
enum arm_way { ARM_OUT, ARM_BACK, ARM_STILL };

/* reverse both posts once they come within POST_GAP of each other */
static void catapult_posts_meet(struct task_work *work)
{
    NJS_OBJECT *first = FIRST_POST(work);
    NJS_OBJECT *second = FIRST_POST(work)->sibling;

    if (_builtin_fabsf(second->pos[0] - first->pos[0]) < POST_GAP) {
        if (POST_WAYS(work) & FIRST_POST_BACK) {
            POST_WAYS(work) &= ~FIRST_POST_BACK;
        } else {
            POST_WAYS(work) |= FIRST_POST_BACK;
        }
        if (POST_WAYS(work) & SECOND_POST_BACK) {
            POST_WAYS(work) &= ~SECOND_POST_BACK;
        } else {
            POST_WAYS(work) |= SECOND_POST_BACK;
        }
    }
}

static void catapult_turn_in(struct task_work *work, NJS_VECTOR *in,
                             NJS_VECTOR *out)
{
    Angle ang;

    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, 0x10000 - work->ang.y, ang);
    njCalcVector(NULL, in, out);
    njPopMatrix(1);
}

/* TRUE when player 0 is inside the catching box, else FALSE */
static Bool catapult_has_player(struct task_work *work)
{
    NJS_VECTOR from;
    NJS_VECTOR to;
    Bool inside;

    from.x = player_work[0]->pos.x - work->pos.x;
    from.y = player_work[0]->pos.y - work->pos.y;
    from.z = player_work[0]->pos.z - work->pos.z;
    catapult_turn_in(work, &from, &to);
    if (_builtin_fabsf(to.x) < CATCH_X && _builtin_fabsf(to.z) < CATCH_Z &&
        _builtin_fabsf(to.y) < CATCH_Y) {
        inside = TRUE;
    } else {
        inside = FALSE;
    }
    return inside;
}

/* close the posts in on player 0. each moves a quarter of the way to its
 * place, POST_HOLD from player 0 on its side. within 0.5 of its place, it
 * moves there. it then stops at POST_GAP from the other post, and at
 * POST_REACH from the center */
static void catapult_posts_close(struct task_work *work)
{
    NJS_OBJECT *first = FIRST_POST(work);
    NJS_OBJECT *second = FIRST_POST(work)->sibling;
    NJS_VECTOR from;
    NJS_VECTOR to;
    Float gap;

    from.x = player_work[0]->pos.x - work->pos.x;
    from.y = 0.0F;
    from.z = player_work[0]->pos.z - work->pos.z;
    catapult_turn_in(work, &from, &to);
    gap = to.x - second->pos[0] - POST_HOLD;
    if (_builtin_fabsf(gap) < 0.5F) {
        second->pos[0] = to.x - POST_HOLD;
    } else {
        second->pos[0] += 0.25F * gap;
    }
    if (second->pos[0] + POST_GAP > first->pos[0]) {
        second->pos[0] = first->pos[0] - POST_GAP;
    }
    if (second->pos[0] < -POST_REACH) {
        second->pos[0] = -POST_REACH;
    }
    gap = to.x - first->pos[0] + POST_HOLD;
    if (_builtin_fabsf(gap) < 0.5F) {
        first->pos[0] = to.x + POST_HOLD;
    } else {
        first->pos[0] += 0.25F * gap;
    }
    if (first->pos[0] - POST_GAP < second->pos[0]) {
        first->pos[0] = second->pos[0] + POST_GAP;
    }
    if (first->pos[0] > POST_REACH) {
        first->pos[0] = POST_REACH;
    }
}

/* swing the first post's arm, and turn the second's to match */
static void catapult_swing(struct task_work *work)
{
    NJS_OBJECT *post = FIRST_POST(work);
    NJS_OBJECT *arm = post->child;
    NJS_OBJECT *other_arm = post->sibling->child;

    if (work->mode == CATAPULT_HOLD) {
        switch (work->submode) {
        case ARM_OUT:
            arm->ang[1] =
                angle_approach(arm->ang[1], ARM_SWING, ARM_SWING_SPEED);
            if ((Uint16)arm->ang[1] == ARM_SWING) {
                work->submode = ARM_BACK;
            }
            break;
        case ARM_BACK:
            arm->ang[1] = angle_approach(arm->ang[1], 0x10000 - ARM_SWING,
                                         ARM_SWING_SPEED);
            if ((Uint16)arm->ang[1] == 0x10000 - ARM_SWING) {
                work->submode = ARM_OUT;
            }
            break;
        case ARM_STILL:
            break;
        }
    } else if (arm->ang[1] != 0) {
        arm->ang[1] = angle_approach(arm->ang[1], 0, ARM_RETURN_SPEED);
    } else {
        /* @hack redundant: an empty else, which loads the arm's angle into R3
         * and the other arm into R2 at 0x0c90321c; without it, the two trade
         * and 3 of 83 words differ */
    }
    other_arm->ang[1] = arm->ang[1];
}

/* pull player 0 in along the catapult's axes. x moves 0.5 a step to within
 * HOLD_X of the center, and y and z move toward 0. once z is within 0.5, it
 * is 0 and the catapult holds player 0 */
static void catapult_pull_in(struct task_work *work)
{
    NJS_VECTOR from;
    NJS_VECTOR to;
    Angle ang;

    from.x = player_work[0]->pos.x - work->pos.x;
    from.y = player_work[0]->pos.y - work->pos.y;
    from.z = player_work[0]->pos.z - work->pos.z;
    catapult_turn_in(work, &from, &to);
    if (to.x > HOLD_X) {
        to.x -= 0.5F;
    } else if (to.x < -HOLD_X) {
        to.x += 0.5F;
    }
    if (to.y > 0.0F) {
        to.y *= 0.9F;
    } else {
        to.y = 0.0F;
    }
    if (_builtin_fabsf(to.z) < 0.5F) {
        to.z = 0.0F;
        work->mode = CATAPULT_HOLD;
    } else {
        to.z *= 0.75F;
    }
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcVector(NULL, &to, &from);
    njPopMatrix(1);
    player_work[0]->pos.x = from.x + work->pos.x;
    player_work[0]->pos.y = from.y + work->pos.y;
    player_work[0]->pos.z = from.z + work->pos.z;
    sound_play_at_for(SE_MACHINE_RUN, (int)work, 1, 0, work->pos.x, work->pos.y,
                      work->pos.z, 2);
    vibration_start(0, 4, 15, 0);
}

static void catapult_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(VARS(work)->action, CATAPULT_FRAME(work));
        njPopMatrix(1);
    }
}

static void catapult_destroy(struct task *task)
{
}

void catapult_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *first;
    NJS_OBJECT *second;
    NJS_OBJECT *arm;
    NJS_VECTOR speed;
    Angle ang[3];
    Angle heading;
    NJS_ACTION *action;

    if (placement_try_leave_by(task, CATAPULT_RANGE)) {
        return;
    }
    action = &catapult_action;
    if (edit_mode) {
        if (VARS(work)->action == NULL) {
            VARS(work)->action = action;
        }
        catapult_draw(task);
    } else {
        switch (work->mode) {
        case CATAPULT_INIT:
            work->mode = CATAPULT_SLIDE;
            POST_WAYS(work) |= SECOND_POST_BACK;
            POST_WAYS(work) &= ~FIRST_POST_BACK;
            CATAPULT_SPEED(work) += BASE_SPEED;
            CATAPULT_FRAME(work) = 0.0F;
            VARS(work)->action = action;
            CATAPULT_FRAMES(work) = VARS(work)->action->motion->nbFrame;
            task->destructor = catapult_destroy;
            task->draw = catapult_draw;
            break;
        case CATAPULT_SLIDE:
            first = FIRST_POST(work);
            second = FIRST_POST(work)->sibling;
            if (njDistanceP2P(&work->pos, &player_work[0]->pos) <
                CATAPULT_NEAR) {
                work->mode = CATAPULT_NEAR_PLAYER;
            }
            catapult_posts_meet(work);
            if (!(POST_WAYS(work) & FIRST_POST_BACK)) {
                if (first->pos[0] < POST_REACH) {
                    first->pos[0] += 0.5F;
                } else {
                    POST_WAYS(work) |= FIRST_POST_BACK;
                }
            } else {
                if (first->pos[0] > -POST_REACH) {
                    first->pos[0] -= 0.5F;
                } else {
                    POST_WAYS(work) &= ~FIRST_POST_BACK;
                }
            }
            if (!(POST_WAYS(work) & SECOND_POST_BACK)) {
                if (second->pos[0] < POST_REACH) {
                    second->pos[0] += 0.5F;
                } else {
                    POST_WAYS(work) |= SECOND_POST_BACK;
                }
            } else {
                if (second->pos[0] > -POST_REACH) {
                    second->pos[0] -= 0.5F;
                } else {
                    POST_WAYS(work) &= ~SECOND_POST_BACK;
                }
            }
            catapult_swing(work);
            /* @hack spelling: the goto into the next case's draw, which keeps
             * player_set_mode in R10 and 0x8000 in R11 from 0x0c9034ae; as a
             * call of catapult_draw and a break, the two trade and 16 of 375
             * words differ */
            goto draw;
        case CATAPULT_NEAR_PLAYER:
            if (njDistanceP2P(&work->pos, &player_work[0]->pos) >
                CATAPULT_NEAR) {
                work->mode = CATAPULT_SLIDE;
            } else if (catapult_has_player(work)) {
                arm = FIRST_POST(work)->child;
                heading = work->ang.y + arm->ang[1] + 0x8000;
                work->mode = CATAPULT_CATCH;
                player_set_mode(0, PLAYER_MODE_BALL);
                player_set_angles(0, 0, heading, 0);
            }
            catapult_swing(work);
            catapult_posts_close(work);
        draw:
            catapult_draw(task);
            break;
        case CATAPULT_CATCH:
            catapult_pull_in(work);
            if (!catapult_has_player(work)) {
                work->mode = CATAPULT_NEAR_PLAYER;
                player_set_mode(0, PLAYER_MODE_RESET);
            }
            catapult_swing(work);
            catapult_posts_close(work);
            if (!((CATAPULT_FRAME(work) += 1.0F) < CATAPULT_FRAMES(work))) {
                CATAPULT_FRAME(work) = 0.0F;
            }
            catapult_draw(task);
            break;
        case CATAPULT_HOLD:
            catapult_pull_in(work);
            if (!catapult_has_player(work)) {
                work->mode = CATAPULT_NEAR_PLAYER;
                player_set_mode(0, PLAYER_MODE_RESET);
            } else if (per_info[0]->press & (PDD_DGT_TA | PDD_DGT_TB)) {
                work->mode = CATAPULT_FIRE;
            }
            catapult_swing(work);
            catapult_posts_close(work);
            player_set_angles(
                0, 0, work->ang.y + FIRST_POST(work)->child->ang[1] + 0x8000,
                0);
            if (!((CATAPULT_FRAME(work) += 1.0F) < CATAPULT_FRAMES(work))) {
                CATAPULT_FRAME(work) = 0.0F;
            }
            catapult_draw(task);
            break;
        case CATAPULT_FIRE:
            if (catapult_has_player(work)) {
                arm = FIRST_POST(work)->child;
                ang[0] = ang[2] = 0;
                ang[1] = work->ang.y + arm->ang[1] + 0x8000;
                speed.x = CATAPULT_SPEED(work);
                speed.y = speed.z = 0.0F;
                player_launch_speed(0, &speed, ang, LAUNCH_STEPS);
                sound_play(SE_CATAPULT_FIRE, 0, 0, 0);
            } else {
                work->mode = CATAPULT_NEAR_PLAYER;
            }
            catapult_swing(work);
            catapult_posts_close(work);
            if (!((CATAPULT_FRAME(work) += 1.0F) < CATAPULT_FRAMES(work))) {
                CATAPULT_FRAME(work) = 0.0F;
            }
            catapult_draw(task);
            break;
        case CATAPULT_IDLE:
            /* @hack redundant: never-run stores to speed, which give 1 R9 and
             * player_set_mode's address R10 (0x0c9034aa, 0x0c9034b2), which
             * otherwise trade; without it, 14 of 375 words differ */
            if (0) {
                speed.y = 1.0F;
                speed.z = 1.0F;
            }
            break;
        }
    }
}
