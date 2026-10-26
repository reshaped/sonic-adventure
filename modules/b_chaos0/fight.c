/* Chaos 0's fight: when he attacks and what he does next, his stretched
 * arm, his hurts, and which of his shapes each mode turns on */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos0/boss.h"
#include "chaos/chaos.h"
#include "collision/collision.h"
#include "draw/motion.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"

/* the times he has finished BOSS_SPIN at 1 health */
extern Sint8 chaos0_spins;
/* each pose node's matrix in turn, and the stretched hand's matrix as
 * tree_calc_matrices gives it */
extern NJS_MATRIX chaos0_node_matrix;
extern NJS_MATRIX *chaos0_stretch_hand_matrix;
/* the row chaos0_set_shapes last took */
extern Sint8 chaos0_shape_row;

#define SHAPE_ROWS 7

/* nearer than ATTACK_NEAR to player 0, he attacks at once. nearer than
 * JUMP_NEAR as a spin ends, he jumps across rather than walk */
#define ATTACK_NEAR 30.0F
#define JUMP_NEAR 50.0F
/* the calls of chaos0_attack_due after which it attacks within its range */
#define ATTACK_WAIT 180

/* nodes of his pose: the shoulder the stretched arm hangs from, and his
 * hands. STRETCH_HAND is the hand's node in chaos0_stretch_object */
#define NODE_SHOULDER 26
#define NODE_HAND_0 34
#define NODE_HAND_1 23
#define STRETCH_HAND 7

static NJS_POINT3 node_origin = {0.0F, 0.0F, 0.0F};
/* a byte for each of his shapes in each chaos0_row: 1 turns the shape on,
 * and any other value off */
/* clang-format off */
static Sint8 chaos0_shape_rows[SHAPE_ROWS * BOSS_SHAPES] = {
    1, 1, 0, 0, 0,
    1, 1, 0, 1, 0,
    1, 1, 0, 0, 1,
    0, 1, 1, 0, 0,
    0, 0, 0, 0, 1,
    0, 1, 0, 0, 0,
    0, 0, 0, 0, 0,
};
/* clang-format on */

Bool chaos0_attack_due(struct task_work *work, struct chaos *chaos,
                       struct task_work *player, Float range)
{
    Bool due = FALSE;
    Float distance;

    ++boss_attack_checks;
    if (chaos->flags & CHAOS0_CAN_ATTACK) {
        distance = njDistanceP2P(&work->pos, &player_work[0]->pos);
        if (distance < ATTACK_NEAR) {
            due = TRUE;
        }
        if (distance < range && boss_attack_checks > ATTACK_WAIT) {
            due = TRUE;
            boss_attack_checks = 0;
        }
        if (due && work->mode != BOSS_PUNCH) {
            boss_next_mode = BOSS_PUNCH;
            boss_last_mode = boss_return_mode = work->mode;
        }
    }
    return due;
}

void chaos0_choose_attack(struct task_work *work, struct task_motion *motion,
                          struct chaos *record)
{
    /* @hack spelling: a copy of record, which SHC reads the health through in
     * R5 (MOV R6,R5 at 0x0c902ef0); with record alone, the code is 32 bytes
     * longer and 154 of 159 words differ */
    struct chaos *chaos = record;
    struct task_work *player = player_work[0];
    int mode;

    switch ((int)chaos->health) {
    case 3:
        if (work->mode == BOSS_WALK &&
            chaos0_attack_due(work, chaos, player, 80.0F)) {
            boss_repeats = 2;
        }
        break;
    case 2:
        switch (work->mode) {
        case BOSS_WALK:
            if (chaos0_attack_due(work, chaos, player, 120.0F)) {
                boss_repeats = 3;
                boss_return_mode = BOSS_SPIN;
            }
            break;
        case BOSS_SPIN:
            if (work->submode == MODE_DONE) {
                if (njDistanceP2P(&work->pos, &player_work[0]->pos) >
                    JUMP_NEAR) {
                    mode = BOSS_WALK;
                } else {
                    mode = BOSS_JUMP;
                    CHAOS0_VARS(work)->jump = JUMP_ACROSS;
                }
                if (mode != work->mode) {
                    boss_next_mode = mode;
                    boss_last_mode = boss_return_mode = work->mode;
                }
            }
            break;
        }
        break;
    case 1:
        switch (work->mode) {
        case BOSS_WALK:
            if (chaos0_attack_due(work, chaos, player, 120.0F)) {
                boss_repeats = 3;
                boss_return_mode = BOSS_SPIN;
            }
            break;
        case BOSS_SPIN:
            if (work->submode == MODE_DONE) {
                ++chaos0_spins;
                if (chaos0_spins & 1) {
                    if (njDistanceP2P(&work->pos, &player_work[0]->pos) >
                        JUMP_NEAR) {
                        mode = BOSS_WALK;
                    } else {
                        mode = BOSS_JUMP;
                        CHAOS0_VARS(work)->jump = JUMP_ACROSS;
                    }
                } else {
                    mode = BOSS_JUMP;
                    CHAOS0_VARS(work)->jump = JUMP_POLE;
                }
                if (mode != work->mode) {
                    boss_next_mode = mode;
                    boss_last_mode = boss_return_mode = work->mode;
                }
            }
            break;
        }
        break;
    }
}

void chaos0_arm_pose(struct task_work *work, struct chaos *record)
{
    /* @hack spelling: a copy of record, for which SHC keeps record in its
     * frame (MOV.L R5,@R15 at 0x0c903094) to read the pace in the stretch;
     * with record alone, 241 of 256 words differ */
    struct chaos *chaos = record;
    NJS_POINT3 shoulder;
    Angle ang;
    Float scale;

    pose_get_matrix(NODE_SHOULDER, &chaos0_node_matrix);
    njCalcPoint(&chaos0_node_matrix, &node_origin, &shoulder);
    chaos0_hands_last[0] = chaos0_hands[0];
    chaos0_hands_last[1] = chaos0_hands[1];
    if ((chaos->figures & (FIGURE_BODY | FIGURE_STRETCH)) !=
        (FIGURE_BODY | FIGURE_STRETCH)) {
        work->collision->shapes[SHAPE_HAND].flags |= SHAPE_OFF;
        njPushMatrix(&_unit_matrix_);
        njTranslate(NULL, work->pos.x, work->pos.y + ORIGIN_HEIGHT,
                    work->pos.z);
        TURN_VIA(njRotateY, 0x8000 - work->ang.y, ang);
        njPushMatrix(NULL);
        pose_get_matrix(NODE_HAND_0, &chaos0_node_matrix);
        njMultiMatrix(NULL, &chaos0_node_matrix);
        njCalcPoint(NULL, &node_origin, &chaos0_hands[0]);
        njPopMatrix(1);
        njPushMatrix(NULL);
        pose_get_matrix(NODE_HAND_1, &chaos0_node_matrix);
        njMultiMatrix(NULL, &chaos0_node_matrix);
        njCalcPoint(NULL, &node_origin, &chaos0_hands[1]);
        njPopMatrix(1);
        njPopMatrix(1);
    } else {
        work->collision->shapes[SHAPE_HAND].flags &= ~SHAPE_OFF;
        /* his motion's pace is how far the punch has stretched, from 0 to
         * 1 */
        scale = 1.0F + (record->motion.pace * chaos->reach * 0.125F);
        /* @hack spelling: each node through its address, which SHC pools and
         * stores scl through at offset 32 in R0 (MOV #32,R0 at 0x0c9031c6);
         * without it, 127 of 256 words differ */
        (&chaos0_stretch_arm)->scl[0] = scale;
        (&dat_0c9262bc)->scl[0] = (&dat_0c926514)->scl[0] = 1.0F / scale;
        chaos->facing =
            -(Angle)(atan2f(shoulder.x, chaos->reach) * ANGLE_PER_RADIAN);
        njPushMatrix(NULL);
        njTranslateV(NULL, &shoulder);
        TURN_VIA(njRotateZ, chaos->pitch, ang);
        draw_tree(&chaos0_stretch_object);
        njPopMatrix(1);
        njPushMatrix(&_unit_matrix_);
        njTranslate(NULL, work->pos.x, work->pos.y + ORIGIN_HEIGHT,
                    work->pos.z);
        TURN_VIA(njRotateY, 0x8000 - work->ang.y, ang);
        njTranslateV(NULL, &shoulder);
        TURN_VIA(njRotateZ, chaos->pitch, ang);
        chaos0_stretch_hand_matrix =
            tree_calc_matrices(NULL, &chaos0_stretch_object, STRETCH_HAND);
        njCalcPoint(chaos0_stretch_hand_matrix, &node_origin, &chaos0_hands[1]);
        work->collision->shapes[SHAPE_HAND].center.x = chaos0_hands[1].x;
        work->collision->shapes[SHAPE_HAND].center.y = chaos0_hands[1].y;
        work->collision->shapes[SHAPE_HAND].center.z = chaos0_hands[1].z;
        njPopMatrix(1);
    }
}

Bool chaos0_take_hit(struct task_work *work, struct chaos *chaos)
{
    struct task_work *player = player_work[0];

    if (work->flags & COLLISION_DAMAGED) {
        /* in the frame player 0 faces */
        NJS_VECTOR push = {-2.0F, 2.0F, 0.0F};

        motion_flags &= ~(MOTION_ENDED | MOTION_HOLDING);
        if (work->submode == 4 || work->pos.y >= ON_POLE_HEIGHT) {
            if (work->mode != BOSS_FALL) {
                boss_next_mode = BOSS_FALL;
                boss_last_mode = boss_return_mode = work->mode;
            }
        } else {
            if (work->mode != BOSS_HIT) {
                boss_next_mode = BOSS_HIT;
                boss_last_mode = boss_return_mode = work->mode;
            }
            boss_return_mode = BOSS_FLOW_ACROSS;
        }
        player_turn_to_world(player, &push);
        player_set_speed(0, push.x, push.y, push.z);
        return TRUE;
    }
    return FALSE;
}

void chaos0_set_shapes(struct task_work *work, int row)
{
    int i;

    if (row < 0) {
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        row = chaos0_shape_row;
    } else {
        chaos0_shape_row = row;
    }
    for (i = 0; i < BOSS_SHAPES; ++i) {
        /* @hack type: the row's offset narrowed to a Sint8 (EXTS.B at
         * 0x0c903414). rows run to SHAPES_NONE, 6, so the offset is at most 30
         * and the Sint8 holds it; without the cast, the code is 32 bytes
         * shorter and 126 of 150 words differ */
        if (chaos0_shape_rows[(Sint8)(row * BOSS_SHAPES) + i] == 1) {
            work->collision->shapes[i].flags &= ~SHAPE_OFF;
        } else {
            work->collision->shapes[i].flags |= SHAPE_OFF;
        }
    }
    switch (row) {
    case SHAPES_SPIN:
        work->collision->shapes[SHAPE_AROUND].center.y = 5.0F;
        work->collision->shapes[SHAPE_AROUND].size_y = 5.0F;
        break;
    case SHAPES_FLAT:
        work->collision->shapes[SHAPE_AROUND].center.y = 2.0F;
        work->collision->shapes[SHAPE_AROUND].radius = 5.0F;
        work->collision->shapes[SHAPE_AROUND].size_y = 2.0F;
        break;
    }
}
