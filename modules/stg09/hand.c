/* O HAND claw, which grabs player 0 and shakes its rings out into the gold
 * heap */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "hud/hud.h"
#include "object/placement.h"
#include "object/savepoint.h"
#include "object/switch.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/coin_spray.h"
#include "stg09/counter.h"
#include "stg09/gold.h"
#include "stg09/hand.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

extern NJS_OBJECT hand_object;
extern NJS_MOTION hand_motion;
extern NJS_ACTION hand_action;

/* destroyed beyond about 250 units of the camera and player 0 */
#define RANGE ((250.0F * 250.0F) + 10.0F)

#define SE_CLAW 271 /* refreshed each step of a grab */
#define SE_JAWS 272 /* as the jaws take player 0, and as they let it go */

/* frames of the action. while the jaws hold nothing, the checks test the
 * grip's shape only from GRIP_FIRST to GRIP_LAST. while they hold player 0,
 * its rings fall from SHAKE_FIRST to SHAKE_LAST */
#define GRIP_FIRST 30.0F
#define GRIP_LAST 43.0F
#define SHAKE_FIRST 57.0F
#define SHAKE_LAST 102.0F

/* the steps the hand waits after it starts and after each grab */
#define GRAB_WAIT 100
/* the hand grabs only while switch GRAB_SWITCH is on and the gold heap holds
 * HEAP_MOST rings or fewer */
#define GRAB_SWITCH 1
#define HEAP_MOST 9999
/* the launch_steps a grab adds to player 0's */
#define GRAB_LOCK 10
/* the rings bounce at a random height up to this far above player 0's floor */
#define BOUNCE_HEIGHT 20.0F

/* the action's nodes that hold the jaws */
#define FIRST_JAW 10
#define SECOND_JAW 11

struct hand_vars {
    Float rise; /* how far the grip's shape rose in the last step */
    Uint8 unk_04[4];
    Float gap; /* between the jaws' tips, at the last step that measured one */
};

#define VARS(work) ((struct hand_vars *)&(work)->vars)
#define HOLDING(work) ((work)->tick) /* TRUE while the jaws hold player 0 */
/* a count of the shake's steps that drop rings, from 0 to FALL_STEPS_LAST
 * and round to 0 again. SE_RINGS_LOST plays on each such step that finds the
 * count at 0 */
#define FALL_STEPS(work) ((work)->submode)
#define FALL_STEPS_LAST 30
/* where player 0 was as the jaws took it */
#define GRIPPED_AT(work) ((work)->scl)

enum hand_mode {
    HAND_WAITING,
    HAND_GRABBING,
    HAND_LETTING_GO, /* the action plays on to its end after the jaws let go */
    /* the action plays back from GRIP_LAST to 0 after jaws that took nothing */
    HAND_RETURNING
};

static Float hand_frame_rate = 0.3F; /* the action's frames a step */
static Float dat_0cb14bdc = 80.0F;
static Float dat_0cb14be0 = 90.0F;
/* closing jaws take player 0 once their gap is under this, if the hand's
 * record touches another */
static Float hand_grip_gap = 13.8F;
/* holding jaws let player 0 go once their gap is over this */
static Float hand_open_gap = 20.0F;
/* a jaw's tip lies this far down its node's y axis */
static Float hand_jaw_length = 10.0F;
static Float dat_0cb14bf0 = 5.0F;
/* the shake bursts rings out each time the grip reverses, where its rise
 * changes sign. a burst's rings rise at hand_burst_rise plus half the grip's
 * rise the step before */
static Float hand_burst_rise = 2.0F;
/* the rings' spread and scale, for coin_spray_add */
static Float hand_ring_spread = 3.0F;
static Float hand_ring_scale = 1.0F;
static int hand_burst_rings = 9; /* the rings in a burst */
/* the grip, which follows the midpoint of the jaws' tips, and the hand */
static struct collision_shape hand_shapes[2] = {
    {
        SHAPE_SPHERE | SHAPE_NO_PUSH,
        0,
        PUSH_FIRM(PUSH_ALL),
        {0.0F, 0.0F, 0.0F},
        5.0F,
        0.0F,
        0.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_SPHERE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 0.0F, 0.0F},
        45.0F,
        0.0F,
        0.0F,
        {0},
        0,
        NULL,
    },
};
#define GRIP_SHAPE 0 /* the grip's index in hand_shapes */
/* the record's radius reaches this far past the grip's center */
#define GRIP_REACH 10.0F

static void hand_init(struct task *task);
static void hand_destroy(struct task *task);
static void hand_run(struct task *task);
static void hand_draw(struct task *task);

/* pose the action at frame, held to its frames. set first and second to the
 * jaws' origins, and first_axis and second_axis to their y axes. all four are
 * in the hand's model space. returns TRUE, as pose_action always does */
static Bool hand_pose(Float frame, NJS_POINT3 *first, NJS_VECTOR *first_axis,
                      NJS_POINT3 *second, NJS_VECTOR *second_axis)
{
    /* Ninja needs a matrix on an 8-byte boundary, and the stack is aligned
     * only to 4. matrix points at the boundary at or below shifted.matrix's
     * start. storage keeps that boundary, and a whole matrix after it, inside
     * the struct */
    struct {
        NJS_MATRIX *matrix;
        union {
            NJS_MATRIX matrix;
            struct {
                Float alignment;
                NJS_MATRIX matrix;
            } shifted;
        } storage;
    } aligned;
    union point_floats origin;
    union point_floats up;
    Float *at;

    aligned.matrix =
        (NJS_MATRIX *)((Uint32)&aligned.storage.shifted.matrix & ~7);
    njPushMatrix(&_unit_matrix_);
    if (frame > hand_action.motion->nbFrame - 1.0F) {
        frame = hand_action.motion->nbFrame - 1.0F;
    }
    if (frame < 0.0F) {
        frame = 0.0F;
    }
    if (pose_action(NULL, &hand_action, frame, FIRST_JAW, aligned.matrix)) {
        /* @hack spelling: each array walked as (&v)->elements, whose address
         * SHC forms from the stack pointer at each use. as v.elements, the
         * addresses take saved registers, and 111 of hand_pose's 127 words
         * differ */
        at = (&origin)->elements;
        *at++ = 0.0F;
        *at++ = 0.0F;
        *at = 0.0F;
        at = (&up)->elements;
        *at++ = 0.0F;
        *at++ = 1.0F;
        *at = 0.0F;
        njCalcPoint(aligned.matrix, &origin.point, first);
        njCalcVector(aligned.matrix, &up.vector, first_axis);
        njUnitMatrix(NULL);
        if (pose_action(NULL, &hand_action, frame, SECOND_JAW,
                        aligned.matrix)) {
            njCalcPoint(aligned.matrix, &origin.point, second);
            njCalcVector(aligned.matrix, &up.vector, second_axis);
        }
        njPopMatrix(1);
        return TRUE;
    }
    njPopMatrix(1);
    return FALSE;
}

void hand_step(struct task *task)
{
    /* @hack spelling: a copy of task, which keeps task in R14 across the
     * range check (MOV R4,R14 at 0x0c912e66). without it, SHC keeps task on the
     * stack, the step runs 2 bytes longer, and 12 of its 16 words differ */
    struct task *current = task;

    if (!placement_try_leave_by(current, RANGE)) {
        hand_init(current);
    }
}

static void hand_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        collision_init(task, hand_shapes, COLLISION_OBJECT, 1, 2);
    }
    task->draw = hand_draw;
    task->step = hand_run;
    task->destructor = hand_destroy;
    work->timer = 0;
    HOLDING(work) = FALSE;
    work->mode = HAND_WAITING;
    VARS(work)->gap = 0.0F;
    FALL_STEPS(work) = 0;
}

static void hand_destroy(struct task *task)
{
}

/* @hack spelling: the pragma, which has SHC expand hand_sum at each call;
 * without it, SHC writes hand_sum out of line and calls it with BSR, the
 * object's code runs 128 bytes short, and 972 of hand_run's 974 words
 * differ */
#pragma inline(hand_sum)
/* @hack spelling: right before left in the parameters, which has SHC form one
 * array's address last (ADD R15,R6 at 0x0c9131b4) and load through the other
 * first. left first, 37 of hand_run's 974 words differ */
static void hand_sum(const Float *right, const Float *left, Float *out)
{
    *out++ = *left++ + *right++;
    *out++ = *left++ + *right++;
    *out = *left + *right;
}

static void hand_run(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision;
    struct task_work *player;
    Float frame;
    Float gap;
    Float rise;
    Float across;
    Float ground;
    /* @hack spelling: the sums into the tips, the grip and the center, and the
     * speed's walk, take their arrays as (&v)->elements, whose address SHC
     * forms from the stack pointer at each use. as v.elements, the object
     * builds 64 bytes shorter, and 864 of hand_run's 974 words differ */
    union point_floats center;
    union point_floats first;
    union point_floats second;
    union point_floats first_axis;
    union point_floats second_axis;
    union point_floats normal;
    union point_floats first_offset;
    union point_floats second_offset;
    union point_floats height;
    union point_floats midpoint;
    union point_floats first_tip;
    union point_floats second_tip;
    union point_floats grip;
    NJS_POINT3 grip_pos;
    NJS_VECTOR grip_normal;
    union point_floats speed;
    NJS_VECTOR burst;
    int count;
    int rings;
    int i;
    int pitch;
    int yaw;
    int roll;
    Sint8 nearest;
    Angle ang;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    hand_draw(task);
    if (placement_editing(task)) {
        return;
    }
    /* @hack spelling: the test for the wrap to 0, written <= 0 for the image's
     * CMP/PL (0x0c912f8a). the decrement undoes the wrap, which holds the timer
     * at 65535. as == 0, the test is TST, and 2 of hand_run's 974 words
     * differ */
    if (++TASK_TIMER(work) <= 0) {
        --TASK_TIMER(work);
    }
    collision = work->collision;
    collision->shapes[GRIP_SHAPE].flags |= SHAPE_OFF;
    switch (work->mode) {
    case HAND_WAITING:
        player = player_work[nearest = player_nearest(&work->pos)];
        /* @hack redundant: three tests of player that never run. they steer
         * SHC's register sort, which puts pitch in the stack frame's word at
         * 4 and yaw at 0, as the image stores them at 0x0c91341e and
         * 0x0c913424; without them, the two swap and 4 of 974 words differ */
        if (0) {
            if (player) {
            }
            if (player) {
            }
            if (player) {
            }
        }
        if (TASK_TIMER(work) <= GRAB_WAIT || !switch_is_on(GRAB_SWITCH) ||
            gold_heap_get_rings() > HEAP_MOST) {
            break;
        }
        if ((Sint16)ring_count() > 0) {
            player_get(0)->launch_steps += GRAB_LOCK;
            player_set_speed(0, 0.0F, 0.0F, 0.0F);
            player_event_stop(nearest);
            work->mode = HAND_GRABBING;
            work->timer = 0;
            HOLDING(work) = FALSE;
            FALL_STEPS(work) = 0;
            restart_save(&player->pos, &player->ang);
        } else {
            switch_set(GRAB_SWITCH, FALSE);
        }
        break;
    case HAND_RETURNING:
        frame = GRIP_LAST - (TASK_TIMER(work) * hand_frame_rate);
        if (frame < 0.0F) {
            work->mode = HAND_WAITING;
            work->timer = 0;
        }
        break;
    case HAND_GRABBING:
        frame = TASK_TIMER(work) * hand_frame_rate;
        player = player_work[0];
        gap = -1.0F;
        rise = VARS(work)->rise;
        if (hand_pose(frame, &first.point, &first_axis.vector, &second.point,
                      &second_axis.vector)) {
            /* each tip lies hand_jaw_length down its jaw's y axis. normal,
             * the unit sum of the two offsets, is the way the jaws point.
             * grip lies player 0's height past the tips' midpoint along
             * normal. the grip's shape goes to center, 1 unit past the midpoint
             * along normal */
            first_offset.vector = first_axis.vector;
            second_offset.vector = second_axis.vector;
            {
                Float *at = first_offset.elements;

                *at++ *= -hand_jaw_length;
                *at++ *= -hand_jaw_length;
                *at *= -hand_jaw_length;
            }
            {
                Float *at = second_offset.elements;

                *at++ *= -hand_jaw_length;
                *at++ *= -hand_jaw_length;
                *at *= -hand_jaw_length;
            }
            hand_sum((&first)->elements, (&first_offset)->elements,
                     (&first_tip)->elements);
            hand_sum((&second)->elements, (&second_offset)->elements,
                     (&second_tip)->elements);
            hand_sum(second_tip.elements, first_tip.elements,
                     midpoint.elements);
            {
                Float *at = midpoint.elements;

                *at++ *= 0.5F;
                *at++ *= 0.5F;
                *at *= 0.5F;
            }
            hand_sum(second_offset.elements, first_offset.elements,
                     normal.elements);
            njUnitVector(&normal.vector);
            height.vector = normal.vector;
            {
                Float *at = height.elements;

                *at++ *= player_get(0)->consts.height;
                *at++ *= player_get(0)->consts.height;
                *at *= player_get(0)->consts.height;
            }
            hand_sum((&midpoint)->elements, (&height)->elements,
                     (&grip)->elements);
            hand_sum((&midpoint)->elements, (&normal)->elements,
                     (&center)->elements);
            rise = center.point.y - collision->shapes[GRIP_SHAPE].center.y;
            collision->shapes[GRIP_SHAPE].center = center.point;
            collision->radius =
                njScalor(&collision->shapes[GRIP_SHAPE].center) + GRIP_REACH;
            gap = njDistanceP2P(&first_tip.point, &second_tip.point);
        }
        if (HOLDING(work) && gap > 0.0F) {
            /* hold player 0 at the grip, placed in the world. its y angle is
             * the hand's. its x and z angles are both normal's angle from
             * straight up */
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);
            TURN_VIA(njRotateY, work->ang.y, ang);
            njCalcPoint(NULL, &grip.point, &grip_pos);
            njCalcVector(NULL, &normal.vector, &grip_normal);
            njPopMatrix(1);
            across = njSqrt((grip_normal.x * grip_normal.x) +
                            (grip_normal.z * grip_normal.z));
            pitch = (int)(atan2f(across, grip_normal.y) * ANGLE_PER_RADIAN);
            yaw = work->ang.y;
            roll = (int)(atan2f(across, grip_normal.y) * ANGLE_PER_RADIAN);
            player_set_angles(0, pitch, yaw, roll);
            player_set_position(0, grip_pos.x, grip_pos.y, grip_pos.z);
            if ((Sint16)ring_count() > 0 && !(frame < SHAKE_FIRST) &&
                !(frame > SHAKE_LAST)) {
                count = 1;
                ground = player_get(0)->shadow.below +
                         (rand() * (BOUNCE_HEIGHT * RAND_SCALE));
                {
                    Float *at = (&speed)->elements;

                    *at++ = 0.0F;
                    *at++ = 1.0F;
                    *at = 0.0F;
                }
                if (!FALL_STEPS(work)++) {
                    sound_play(SE_RINGS_LOST, 0, 0, 0);
                }
                if (FALL_STEPS(work) > FALL_STEPS_LAST) {
                    FALL_STEPS(work) = 0;
                }
                coin_spray_add(&grip_pos, &speed.vector, hand_ring_spread,
                               ground, hand_ring_scale);
                if (VARS(work)->rise > 0.0F && rise < 0.0F) {
                    burst.x = 0.0F;
                    burst.y = hand_burst_rise + (0.5F * VARS(work)->rise);
                    burst.z = 0.0F;
                    for (i = 0; i < hand_burst_rings; ++i) {
                        coin_spray_add(&grip_pos, &burst, hand_ring_spread,
                                       ground, hand_ring_scale);
                    }
                    count += hand_burst_rings;
                }
                if (VARS(work)->rise < 0.0F && rise > 0.0F) {
                    burst.x = 0.0F;
                    burst.y = hand_burst_rise + (0.5F * VARS(work)->rise);
                    burst.z = 0.0F;
                    for (i = 0; i < hand_burst_rings; ++i) {
                        coin_spray_add(&grip_pos, &burst, hand_ring_spread,
                                       ground, hand_ring_scale);
                    }
                    count += hand_burst_rings;
                }
                rings = (Sint16)ring_count();
                if (rings < count) {
                    count = rings;
                }
                ring_add(-count);
                gold_heap_set_rings(gold_heap_get_rings() + count);
            }
            if (!(frame < SHAKE_FIRST) && !(frame > SHAKE_LAST)) {
                if (VARS(work)->rise > 0.0F && rise < 0.0F) {
                    vibration_start(0, 3, 20, 1);
                }
                if (VARS(work)->rise < 0.0F && rise > 0.0F) {
                    vibration_start(0, 4, 15, 1);
                }
            }
            if (gap > 0.0F && gap > hand_open_gap) {
                count = (Sint16)ring_count();
                ring_add(-count);
                gold_heap_set_rings(gold_heap_get_rings() + count);
                player_set_mode(0, PLAYER_MODE_RESET);
                HOLDING(work) = FALSE;
                counter_set(1, gold_heap_get_rings());
                counter_set(2, gold_heap_get_rings());
                sound_play(SE_JAWS, 0, 0, 0);
                work->mode = HAND_LETTING_GO;
            }
        } else {
            HOLDING(work) = FALSE;
            if (VARS(work)->gap > gap && hand_grip_gap > gap &&
                (work->collision->flags & COLLISION_HIT)) {
                GRIPPED_AT(work) = player->pos;
                sound_play(SE_JAWS, 0, 0, 0);
                HOLDING(work) = TRUE;
            }
            if (gap > 0.0F && !(frame > GRIP_LAST) && !(frame < GRIP_FIRST)) {
                collision->shapes[GRIP_SHAPE].flags &= ~SHAPE_OFF;
            }
            if (frame > GRIP_LAST && !HOLDING(work)) {
                work->mode = HAND_RETURNING;
                work->timer = 0;
                switch_set(GRAB_SWITCH, FALSE);
            }
        }
        if (gap > 0.0F) {
            VARS(work)->gap = gap;
        }
        VARS(work)->rise = rise;
        if (!(frame < hand_motion.nbFrame - 1.0F)) {
            if (HOLDING(work)) {
                player_set_mode(0, PLAYER_MODE_RESET);
            }
            HOLDING(work) = FALSE;
            work->mode = HAND_WAITING;
            work->timer = 0;
        }
        sound_play_for(SE_CLAW, (int)work, 1, 0, 5);
        break;
    case HAND_LETTING_GO:
        frame = TASK_TIMER(work) * hand_frame_rate;
        if (!(frame < hand_motion.nbFrame - 1.0F)) {
            work->mode = HAND_WAITING;
            work->timer = 0;
        }
        /* no break: a break moves collision_register's call from the image's
         * R3 to R2 (0x0c9138d2). with one, 2 of hand_run's 974 words differ */
    }
    collision_register(work);
}

static void hand_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &hand_object;
    NJS_MOTION *motion = &hand_motion;
    Float frame;
    Angle ang;

    if (!placement_try_leave_by(task, RANGE) && !frame_step) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        switch (work->mode) {
        case HAND_WAITING:
            draw_tree(object);
            break;
        case HAND_RETURNING:
            frame = GRIP_LAST - (TASK_TIMER(work) * hand_frame_rate);
            if (frame < 0.0F) {
                frame = 0.0F;
            }
            draw_motion(object, motion, frame);
            break;
        case HAND_GRABBING:
        case HAND_LETTING_GO:
            frame = TASK_TIMER(work) * hand_frame_rate;
            if (!(frame < motion->nbFrame - 1.0F)) {
                frame = motion->nbFrame - 1.0F;
            }
            draw_motion(object, motion, frame);
            break;
        }
        njPopMatrix(1);
    }
}
