/* Zero, E-100 alpha, the robot that chases Amy, who is player 0. it walks
 * after her, and now and then stomps out a shock wave near her. once it
 * sees her, it sweeps its eye's beam toward her. once the beam finds her,
 * it throws its hand at her on a chain. an event can place Zero too, and
 * ask it to throw its hand at a point */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/snow.h"
#include "enemy/zero.h"
#include "enemy/zero_handle.h"
#include "enemy/zero_stand.h"
#include "event/event.h"
#include "event/play.h"
#include "object/placement.h"
#include "player/amy.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg03/mirror.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

enum zero_mode {
    ZERO_APPEAR = 1, /* a ZERO_STAND start: it falls from its first step */
    ZERO_STILL = 2, /* an event's Zero, at rest on the ground */
    ZERO_FALL = 5, /* it falls until it rests on the ground */
    ZERO_WONDER = 6, /* under a question mark, it picks where to look */
    ZERO_TURN_HEAD = 8,
    ZERO_TURN_BODY = 9, /* the body follows the head round */
    ZERO_WALK = 10,
    ZERO_DASH_IN = 11,
    ZERO_HIT_FRONT = 12, /* hit with Amy ahead: it slides back, facing her */
    ZERO_HIT_BEHIND = 13, /* hit with Amy behind: it slides ahead */
    ZERO_STOMP = 14,
    ZERO_LEAVE = 15,
    ZERO_AIM = 16,
    ZERO_BEAM = 17,
    ZERO_WIND_UP = 18,
    ZERO_THROW = 19,
    ZERO_REEL = 20, /* it reels the hand back in */
    ZERO_EVENT = 21 /* an event's path, look or move drives it */
};

/* in Zero's work->flags, which zero_collide sets */
#define ZERO_ON_GROUND 0x1
#define ZERO_AT_WALL 0x100 /* a wall pushed it out */

/* how far zero_collide keeps Zero's position from the world on each side */
#define ZERO_RADIUS 12.0F

/* Amy's middle, which Zero aims at, lies AMY_MIDDLE above her position */
#define AMY_MIDDLE 5.0F

/* Zero ends once Amy's middle is farther than CHASE_RANGE from it */
#define CHASE_RANGE 400.0F

/* Zero spots Amy only within SIGHT_RANGE, and a sight flies no farther */
#define SIGHT_RANGE 150.0F

/* Amy more than REACH_HEIGHT above Zero is out of its reach */
#define REACH_HEIGHT 20.0F

/* a ZERO_DASH start's speed a step */
#define DASH_SPEED 3.0F

/* a leap's speed a step: LEAP_AHEAD along Zero's heading, and LEAP_UP
 * up */
#define LEAP_AHEAD 1.0F
#define LEAP_UP 4.0F

/* how far a mark shows above Zero */
#define MARK_HEIGHT 15.0F

/* the nodes of Zero's model at its eye, where the beam starts, and at its
 * arm's end */
#define EYE_NODE 10
#define ARM_END_NODE 30

/* the beam's full length, at which its far end fades to nothing */
#define BEAM_LENGTH 150.0F

/* the lock lets go once Amy's middle is farther than LOCK_RANGE from Zero */
#define LOCK_RANGE 200.0F

/* the frames on which the stomp's foot lands and the hand leaves */
#define STOMP_FRAME 8.0F
#define THROW_FRAME 16.0F

#define CHAIN_POINTS 16

/* the length of the chain link's model */
#define LINK_LENGTH 10.0F

/* the hand flies HAND_SPEED a step, and HAND_HOMING_SPEED once it is back
 * within HAND_HOMING_RANGE of the arm's end */
#define HAND_SPEED 4.0F
#define HAND_HOMING_SPEED 1.0F
#define HAND_HOMING_RANGE 30.0F

/* the hand turns back once HAND_REACH from the arm's end */
#define HAND_REACH 200.0F

/* how far zero_keep_clear keeps the hand from the world */
#define HAND_RADIUS 8.0F

/* a floor less than HAND_LIFT under the hand lifts it to HAND_LIFT above */
#define HAND_LIFT 5.0F

#define BOB_HEIGHT 1.0F

/* indexes into zero_actions, as zero_anim.action holds them */
enum zero_action {
    ZERO_ACT_HIT_BEHIND = 0,
    ZERO_ACT_HIT_BEHIND_LAND = 1,
    ZERO_ACT_HIT_FRONT = 2,
    ZERO_ACT_HIT_FRONT_LAND = 3,
    ZERO_ACT_AIM = 4,
    ZERO_ACT_WALK = 5,
    ZERO_ACT_WIND_UP = 9,
    ZERO_ACT_THROW = 11,
    ZERO_ACT_BEAM = 12,
    ZERO_ACT_BEAM_END = 13,
    ZERO_ACT_WONDER_START = 14,
    ZERO_ACT_WONDER = 15,
    ZERO_ACT_WONDER_END = 16,
    ZERO_ACT_STOMP_LIFT = 17,
    ZERO_ACT_STOMP = 18,
    ZERO_ACT_LEAVE = 19,
    ZERO_ACT_STAND = 20
};

/* where the hand is, in zero_hand_state. zero_hand_launch, zero_hand_fly
 * and zero_hand_return return it as they set it */
enum zero_hand {
    ZERO_HAND_OUT, /* thrown on this step */
    ZERO_HAND_UNTHROWN, /* kept, since the lock lies at the arm's end */
    ZERO_HAND_READY, /* on the arm, which winds up to throw it */
    ZERO_HAND_FLYING,
    ZERO_HAND_CAUGHT, /* within 5 of Amy's middle */
    ZERO_HAND_MISSED, /* past 20 from the arm's end, and past the lock */
    ZERO_HAND_BACK, /* HAND_REACH out, or on its way back */
    ZERO_HAND_HOME /* back on the arm */
};

/* how far an event's request for a throw has got, in
 * zero_event_grab_state */
enum zero_grab { ZERO_GRAB_NONE, ZERO_GRAB_ASKED, ZERO_GRAB_STARTED };

/* the phases of an event's EVENT_ACTION_TURN_BY or EVENT_ACTION_TURN_TO, in
 * TURN_PHASE: aim the head, turn it, then turn the body after it */
enum zero_turn_phase { PHASE_AIM, PHASE_HEAD, PHASE_BODY, PHASE_DONE };

enum zero_sight_mode { SIGHT_START, SIGHT_FLY };
enum zero_shock_mode { SHOCK_START, SHOCK_SPREAD };

enum zero_marker_mode {
    MARKER_START,
    MARKER_LOCK,
    MARKER_WIND_UP,
    MARKER_LOST,
    MARKER_SWELL
};

#define SE_HIT 767
#define SE_THUD 762
#define SE_MARK 760
#define SE_TURN 772
#define SE_LEAP 771
#define SE_DASH_END 1184
#define SE_AIM 759
#define SE_BEAM 773
#define SE_LOCK 774
#define SE_LATCH 763
#define SE_REEL 761
#define SE_SLIDE 764
#define SE_KNOCK_LAND 775
#define SE_STOMP 710

/* Zero's bytes in work->vars, which VARS casts in place */
struct zero_vars {
    Uint8 player; /* the player slot event_create_player gives it */
    Uint8 unk_01[3];
    Sint32 timer; /* steps left in its mode */
    /* @bug logic: an event's turn keeps its phase in the alpha's second
     * byte (MOV.B R3,@(R0,R14) with R0 17 at 0x8c1162c8). each step of an
     * event's Zero ends by storing 1.0F to the alpha (FMOV.S at 0x8c11635e).
     * 1.0F is 0x3f800000, whose second byte in memory is 0, PHASE_AIM, so
     * the turn never gets past PHASE_AIM, and Zero turns neither its head
     * nor its body */
    union {
        Float alpha;
        Sint8 turn[4];
    } shade;
};

#define VARS(work) ((struct zero_vars *)&(work)->vars)
/* how opaque Zero draws, from 0 to 1 */
#define ALPHA(work) (VARS(work)->shade.alpha)
#define TURN_PHASE(work) (VARS(work)->shade.turn[1])
/* Zero's speed: x along its heading, ang.y, and y up */
#define SPEED(work) ((work)->scl)
/* the turn the head is to reach, and the turn it has made, from the body's
 * heading */
#define HEAD_AIM(work) ((work)->ang.x)
#define HEAD_TURN(work) ((work)->ang.z)

/* a sight flies from Zero toward Amy's middle, scl a step. it sets
 * zero_sight_saw TRUE once it touches a player. it sets it FALSE once it
 * meets an object or the world first, or has flown SIGHT_RANGE. travel is
 * how far it has flown */
struct zero_sight_vars {
    Float travel;
};

#define SIGHT_VARS(work) ((struct zero_sight_vars *)&(work)->vars)

/* a mark's bytes in work->vars: a question mark's or a flash of an idea's */
struct zero_mark_vars {
    Uint16 frame; /* its sprite's frame, from 0 to 2 */
    Uint8 unk_02[6];
    struct task_work *follow; /* the work it keeps MARK_HEIGHT above, or null */
};

#define MARK_VARS(work) ((struct zero_mark_vars *)&(work)->vars)

/* the lock's marker: scl.x is its size, and ang.x and ang.y its spin */
struct zero_marker_vars {
    Uint8 unk_00[8];
    /* what MARKER_SWELL adds to its size each step. it starts at 1 and
     * falls by 0.1 a step, so the marker swells and then shrinks away */
    Float grow;
};

#define MARKER_VARS(work) ((struct zero_marker_vars *)&(work)->vars)

/* a shock wave's scl.x and scl.z are its radius. work->tick counts the
 * wave's draws, and the tick's low bit picks one of two textures */
struct zero_shock_vars {
    Uint32 steps; /* steps since it began, to 35 */
};

#define SHOCK_VARS(work) ((struct zero_shock_vars *)&(work)->vars)

/* a sweep is a quad between the beam and the beam a step before, which
 * fades over 8 steps. pos and scl are the beam's ends, and last_end its end
 * a step before. ang holds the beam's start a step before, as an offset
 * from pos, times 65536 */
struct zero_sweep_vars {
    NJS_POINT3 last_end;
};

#define SWEEP_VARS(work) ((struct zero_sweep_vars *)&(work)->vars)

/* what zero_blind and zero_push ask of Zero, until zero_clear_request */
struct zero_request {
    Bool blind;
    NJS_VECTOR push;
};

/* @hack type: the neck and the head as arrays of one, whose angles SHC
 * stores at an offset from the object's address in the pool (MOV.L
 * R0,@(24,R3) at 0x8c1119f6); as plain objects, the pool holds each
 * angle's own address, and summed over the six functions that set them, 12
 * of 2594 words differ */
extern NJS_OBJECT zero_neck_object[1];
extern NJS_OBJECT zero_head_object[1];
extern NJS_OBJECT zero_bob_object;
/* @hack type: the hand and its claws as arrays of one, which has SHC clear
 * their NJD_EVAL_HIDE bits through R3, R1 and R2 in turn (0x8c111aaa to
 * 0x8c111ac8); as plain objects, each clear goes through R0, and summed
 * over the four functions that use them, 37 of 653 words differ */
extern NJS_OBJECT zero_hand_object[1];
extern NJS_OBJECT zero_claw0_object[1];
extern NJS_OBJECT zero_claw1_object[1];
extern NJS_OBJECT zero_claw1_tip_object[1];
extern NJS_OBJECT zero_link_object;
extern NJS_OBJECT zero_shock_object;
extern NJS_ACTION zero_hit_behind_action;
extern NJS_ACTION zero_hit_behind_land_action;
extern NJS_ACTION zero_hit_front_action;
extern NJS_ACTION zero_hit_front_land_action;
extern NJS_ACTION zero_aim_action;
extern NJS_ACTION zero_walk_action;
extern NJS_ACTION dat_8c5fc4e0;
extern NJS_ACTION dat_8c6001d4;
extern NJS_ACTION dat_8c600f88;
extern NJS_ACTION zero_wind_up_action;
extern NJS_ACTION zero_wind_up_end_action;
extern NJS_ACTION zero_throw_action;
extern NJS_ACTION zero_beam_action;
extern NJS_ACTION zero_beam_end_action;
extern NJS_ACTION zero_wonder_action;
extern NJS_ACTION zero_wonder_loop_action;
extern NJS_ACTION zero_stomp_lift_action;
extern NJS_ACTION zero_stomp_action;
extern NJS_ACTION zero_leave_action;

static void zero_setup(struct task *task);
static void zero_reset(struct task *task);
static void zero_destroy(struct task *task);
static void zero_step(struct task *task);
static void zero_collide(struct task *task);
static Bool zero_look_for_amy(struct task *task);
static Float zero_amy_distance2(NJS_POINT3 *pos);
static void zero_draw(struct task *task);
static void zero_beam_reset(void);
static void zero_beam(struct task *task);
static void zero_sweep_step(struct task *task);
static void zero_scorch_step(struct task *task);
static void zero_marker_step(struct task *task);
static int zero_hand_launch(struct task *task);
static int zero_hand_fly(struct task *task);
static int zero_hand_return(struct task *task);
static void zero_hand_shadow_step(struct task *task);
static void zero_hatena_step(struct task *task);
static void zero_hirame_step(struct task *task);
static void zero_shock_step(struct task *task);
static void zero_event_grab_step(struct task *task);
static void zero_event_grab_start(struct task *task);
static void zero_event_destroy(struct task *task);
static void zero_event_step(struct task *task);
static void zero_fall(struct task *task);
static void zero_move(struct task *task);

static struct player_anim zero_anim;
static Float zero_hit_pace; /* the pace a hit's actions play at */
static Float zero_frame; /* the frame zero_play_frames moves */
static int zero_beam_turn_timer; /* steps until zero_beam_drift turns */
static Bool zero_beam_hit; /* the beam met the world on its last step */
static Float zero_beam_length;
static NJS_POINT3 zero_beam_aim;
static NJS_VECTOR zero_beam_dir;
static NJS_POINT3 zero_beam_last_start;
static NJS_POINT3 zero_beam_end;
static NJS_POINT3 zero_beam_last_end;
/* how far zero_beam_aim moves each step */
static NJS_VECTOR zero_beam_drift;
/* the hand has a lock to go for: the beam found Amy, or an event asked for
 * a throw */
static Bool zero_locked;
/* the lock, the point the hand goes for. while the lock's marker is in
 * MARKER_LOCK and no event is in control, the marker keeps it on Amy's
 * middle */
static NJS_POINT3 zero_lock_pos;
static int zero_hand_state;
static Angle zero_hand_ang_y;
static Angle zero_hand_pitch;
static NJS_POINT3 zero_arm_end;
static NJS_POINT3 zero_hand_pos;
static NJS_VECTOR zero_hand_speed;
/* the chain's points, from the hand back toward the arm's end */
static NJS_POINT3 zero_chain[CHAIN_POINTS];
static Bool zero_sight_out; /* a sight is flying */
static Bool zero_sight_saw; /* the last sight touched a player */
/* how hurt Zero is, from 0 to 1. each hit that knocks it back adds 0.2, and
 * at 1 no hit knocks it back */
static Float zero_damage;
/* how restless Zero's search has made it, from 0 to 1. each walk's end
 * adds 0.125 to zero_restless, a stomp halves it, and spotting Amy clears
 * it. a walk that ends within 150 of Amy's middle stomps with zero_restless
 * as its chance. past 0.9, ZERO_WONDER aims the head at random half the
 * time */
static Float zero_restless;
static struct zero_request zero_request;
/* the beam passes through the world: TRUE in Twinkle Park's acts 1 and 2 */
static Bool zero_beam_unblocked;
static NJS_POINT3 zero_beam_points[2]; /* the ends zero_beam_line draws */
/* 4 bytes that no code names, of unknown type. they keep zero_node_matrix
 * and each static after it at its address. pool words hold those addresses,
 * as 0x8c113d80 and 0x8c114808 hold zero_node_matrix's. without the 4 bytes,
 * each of those statics starts 4 bytes lower. in the eight functions that
 * name one, 0 of 1597 words and 9 pool words differ */
static Uint32 dat_8c7ae0d4;
static NJS_MATRIX zero_node_matrix;
static NJS_POINT3 zero_sweep_points[4];
static NJS_POINT3 zero_scorch_points[2];
static int zero_event_grab_state;
static NJS_POINT3 zero_event_grab_pos;

static struct player_action zero_actions[21] = {
    {&zero_hit_behind_action, 48, PLAYER_ANIM_ONCE_PACE, 1, 1.0F, 1.0F},
    {&zero_hit_behind_land_action, 48, PLAYER_ANIM_ONCE_PACE, 20, 1.0F, 1.0F},
    {&zero_hit_front_action, 48, PLAYER_ANIM_ONCE_PACE, 3, 1.0F, 1.0F},
    {&zero_hit_front_land_action, 48, PLAYER_ANIM_ONCE_PACE, 20, 1.0F, 1.0F},
    {&zero_aim_action, 48, PLAYER_ANIM_ONCE, 20, 1.0F, 0.8F},
    {&zero_walk_action, 48, PLAYER_ANIM_LOOP, 5, 1.0F, 1.0F},
    {&dat_8c5fc4e0, 48, PLAYER_ANIM_LOOP, 6, 1.0F, 1.0F},
    {&dat_8c6001d4, 48, PLAYER_ANIM_LOOP, 7, 1.0F, 1.0F},
    {&dat_8c600f88, 48, PLAYER_ANIM_LOOP, 8, 1.0F, 1.0F},
    {&zero_wind_up_action, 48, PLAYER_ANIM_ONCE, 10, 1.0F, 1.5F},
    {&zero_wind_up_end_action, 48, PLAYER_ANIM_ONCE, 11, 1.0F, 1.0F},
    {&zero_throw_action, 48, PLAYER_ANIM_HOLD, 20, 1.0F, 1.0F},
    {&zero_beam_action, 48, PLAYER_ANIM_HOLD, 13, 1.0F, 1.0F},
    {&zero_beam_end_action, 48, PLAYER_ANIM_DRIVEN, 20, 1.0F, 1.0F},
    {&zero_wonder_action, 48, PLAYER_ANIM_ONCE, 15, 1.0F, 1.0F},
    {&zero_wonder_loop_action, 48, PLAYER_ANIM_LOOP, 15, 1.0F, 0.6F},
    {&zero_wonder_action, 48, PLAYER_ANIM_DRIVEN, 20, 1.0F, 1.0F},
    {&zero_stomp_lift_action, 48, PLAYER_ANIM_ONCE, 18, 1.0F, 0.7F},
    {&zero_stomp_action, 48, PLAYER_ANIM_ONCE, 20, 1.0F, 1.0F},
    {&zero_leave_action, 48, PLAYER_ANIM_LOOP, 19, 1.0F, 1.0F},
    {&zero_stand_action, 48, PLAYER_ANIM_LOOP, 20, 1.0F, 1.0F},
};

/* the body's capsule, and the hand's sphere, which is off until the hand
 * flies */
/* clang-format off */
static struct collision_shape zero_shapes[2] = {
    {SHAPE_CAPSULE | SHAPE_HURTABLE | SHAPE_UNTURNED, SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES),
     {0.0F, 0.0F, 0.0F}, 8.0F, 10.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_OFF | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | GUARD(3) | ATTACK(1),
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F}, 4.0F, 0.0F, 0.0F, {0}, 0, NULL},
};

/* @hack type: a sight's shape as an array of one, which has zero_sight_step
 * push collision_init's last argument from R3 (MOV.L R3,@-R15 at
 * 0x8c1134b6); as a plain shape, it pushes it from R11, and 13 of 177 words
 * differ */
static struct collision_shape zero_sight_shape[1] = {
    {SHAPE_SPHERE, GUARD(3), PUSH_FIRM(PUSH_PLAYERS),
     {0.0F, 0.0F, 0.0F}, 5.0F, 0.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static NJS_COLOR zero_beam_colors[2] = {
#include "enemy/zero/zero_beam_colors.inc"
};
static NJS_POINT3COL zero_beam_line = {
#include "enemy/zero/zero_beam_line.inc"
};
static NJS_COLOR zero_sweep_colors[4] = {
#include "enemy/zero/zero_sweep_colors.inc"
};
static NJS_POINT3COL zero_sweep_quad = {
#include "enemy/zero/zero_sweep_quad.inc"
};
static NJS_COLOR zero_scorch_colors[2] = {
#include "enemy/zero/zero_scorch_colors.inc"
};
static NJS_POINT3COL zero_scorch_line = {
#include "enemy/zero/zero_scorch_line.inc"
};
static NJS_POINT3 zero_marker_points[5] = {
#include "enemy/zero/zero_marker_points.inc"
};
static NJS_COLOR zero_marker_colors[5] = {
#include "enemy/zero/zero_marker_colors.inc"
};
static NJS_POINT3COL zero_marker_faces = {
#include "enemy/zero/zero_marker_faces.inc"
};

/* hirame, a flash of an idea, and hatena, a question mark */
static NJS_TEXANIM zero_hirame_texanims[3] = {
#include "enemy/zero/zero_hirame_texanims.inc"
};
static NJS_SPRITE zero_hirame_sprite = {
#include "enemy/zero/zero_hirame_sprite.inc"
};
static NJS_TEXANIM zero_hatena_texanims[3] = {
#include "enemy/zero/zero_hatena_texanims.inc"
};
static NJS_SPRITE zero_hatena_sprite = {
#include "enemy/zero/zero_hatena_sprite.inc"
};

/* the shock wave's sphere, which zero_shock_step keeps on the wave's edge
 * toward Amy. its radius is 1.5 times what the wave spreads a step */
/* @hack type: the shape as an array of one, which has zero_shock_step pass
 * collision_init's last argument from R3 (MOV #1,R3 at 0x8c11583e); as a
 * plain shape, SHC keeps the 1 in R12, saved at entry, the function runs 2
 * bytes longer and 40 of its 238 words differ */
/* clang-format off */
static struct collision_shape zero_shock_shape[1] = {
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | GUARD(3) | ATTACK(3), PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F}, 4.242857F, 0.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static void zero_clear_request(void)
{
    zero_request.blind = FALSE;
    VECTOR_CLEAR(&zero_request.push);
}

void zero_blind(void)
{
    zero_request.blind = TRUE;
}

void zero_push(NJS_VECTOR *push)
{
    zero_request.push = *push;
}

struct task *zero_create(struct zero_start *start)
{
    struct task *task;
    Float dx;
    Float dz;

    if (get_character() != CHARACTER_AMY || player_work[0] == NULL) {
        return NULL;
    }
    /* @hack spelling: the task stored as it's tested, which tests the result
     * in R0 and copies it to R14 in the branch's slot (TST R0,R0 at
     * 0x8c11171e, MOV R0,R14 at 0x8c111722); as two statements, SHC tests
     * R14 and 3 of 243 words differ */
    if ((task = task_create(TASK_WORK, 3, zero_setup)) == NULL) {
        return NULL;
    }
    zero_beam_unblocked = FALSE;
    switch (stage_get_stage_act()) {
    case STAGE_ACT(STAGE_TWINKLE, 1):
    case STAGE_ACT(STAGE_TWINKLE, 2):
        zero_beam_unblocked = TRUE;
        break;
    }
    switch (start->how) {
    case ZERO_DASH:
        task->work->pos = start->pos;
        task->work->pos.y += 5.0F;
        dx = start->target.x - start->pos.x;
        dz = start->target.z - start->pos.z;
        task->work->ang.y = (Angle)(atan2f(dz, -dx) * ANGLE_PER_RADIAN);
        VARS(task->work)->timer =
            (int)(njSqrt((dx * dx) + (dz * dz)) * (1.0F / DASH_SPEED));
        SPEED(task->work).y = 0.0F;
        SPEED(task->work).z = 0.0F;
        task->work->mode = ZERO_DASH_IN;
        break;
    case ZERO_STAND:
        task->work->pos = start->pos;
        if (player_work[0] == NULL) {
            task->work->ang.y = 0;
        } else {
            dx = player_work[0]->pos.x - start->pos.x;
            dz = player_work[0]->pos.z - start->pos.z;
            task->work->ang.y = (Angle)(atan2f(dz, -dx) * ANGLE_PER_RADIAN);
        }
        task->work->mode = ZERO_APPEAR;
        break;
    case ZERO_LEAP:
        task->work->pos = start->pos;
        task->work->pos.y += 5.0F;
        dx = start->target.x - start->pos.x;
        dz = start->target.z - start->pos.z;
        task->work->ang.y = (Angle)(atan2f(dz, -dx) * ANGLE_PER_RADIAN);
        /* a dash's timer, which the leap's landing replaces before any
         * read */
        VARS(task->work)->timer =
            (int)(njSqrt((dx * dx) + (dz * dz)) * (1.0F / DASH_SPEED));
        SPEED(task->work).x = LEAP_AHEAD;
        SPEED(task->work).y = LEAP_UP;
        SPEED(task->work).z = 0.0F;
        task->work->mode = ZERO_FALL;
        break;
    }
    vibration_start(0, 6, 15, 0);
    return task;
}

static void zero_setup(struct task *task)
{
    collision_init(task, zero_shapes, COLLISION_ENEMY, 1, 2);
    task->destructor = zero_destroy;
    task->draw = zero_draw;
    if (stage_get_stage_act() == STAGE_TWINKLE_MIRRORS) {
        twinkle_reflect(task);
    }
    task->step = zero_step;
    zero_damage = 0.0F;
    zero_reset(task);
    zero_beam_reset();
}

static void zero_reset(struct task *task)
{
    struct task_work *work = task->work;

    if (player_work[0] == NULL) {
        return;
    }
    zero_anim.state = PLAYER_ANIM_NEW;
    zero_anim.actions = zero_actions;
    zero_anim.action = zero_anim.current = ZERO_ACT_STAND;
    zero_anim.pace = &zero_hit_pace;
    work->tick = 0;
    HEAD_AIM(work) = 0;
    HEAD_TURN(work) = 0;
    zero_neck_object[0].ang[1] = 0;
    zero_head_object[0].ang[2] = NJM_DEG_ANG(180);
    zero_sight_out = FALSE;
    zero_sight_saw = FALSE;
    work->collision->shapes[1].flags |= SHAPE_OFF;
    zero_restless = 0.0F;
    zero_clear_request();
}

/* undo what a lock, a throw and a turn of the head leave on Zero and on the
 * model every Zero shares. it runs when a hit knocks Zero back, and as Zero
 * ends */
static void zero_restore(struct task_work *work)
{
    work->tick = 0;
    ALPHA(work) = 1.0F;
    work->collision->shapes[1].flags |= SHAPE_OFF;
    zero_locked = FALSE;
    zero_neck_object[0].ang[1] = 0;
    zero_head_object[0].ang[2] = NJM_DEG_ANG(180);
    zero_hand_object[0].evalflags &= ~NJD_EVAL_HIDE;
    zero_claw1_object[0].evalflags &= ~NJD_EVAL_HIDE;
    zero_claw1_tip_object[0].evalflags &= ~NJD_EVAL_HIDE;
    zero_claw0_object[0].evalflags &= ~NJD_EVAL_HIDE;
}

static void zero_destroy(struct task *task)
{
    zero_restore(task->work);
}

/* move zero_frame by frames, which may be negative, and set zero_anim's
 * frame to it. returns TRUE once one more such move would carry it past
 * the end of the action it moves toward, else FALSE */
static Bool zero_play_frames(Float frames)
{
    zero_frame += frames;
    zero_anim.frame = zero_frame;
    if (frames > 0.0F &&
        zero_anim.frame >
            (Float)zero_actions[zero_anim.action].action->motion->nbFrame -
                frames) {
        return TRUE;
    }
    if (frames < 0.0F && -frames > zero_anim.frame) {
        return TRUE;
    }
    return FALSE;
}

/* start action at frame, for zero_play_frames to move */
/* @hack spelling: zero_anim's action and frame set through an inline
 * function, which SHC expands with action in R4 and frame in FR4 as for a
 * call (MOV #16,R4 at 0x8c1120f4, FADD FR2,FR4 at 0x8c1120fa); written out
 * as stores, the arm runs 4 bytes short, frame slots move, and 68 of
 * zero_step's 1912 words differ */
static void zero_set_action(Uint16 action, Float frame)
{
    zero_anim.action = action;
    zero_frame = frame;
    zero_anim.start_frame = frame;
}

static void zero_step(struct task *task)
{
    struct task_work *work;
    struct task_work *amy;
    struct task *child;
    NJS_POINT3 point;
    NJS_POINT3 vector;
    NJS_POINT2 amy_screen;
    NJS_POINT2 zero_screen;
    Float distance;
    Float dx;
    Float dz;
    Float lock_x;
    Float lock_z;
    Float speed;
    Float amy_depth;
    Float zero_depth;
    Float amy_radius;
    Float zero_radius;
    Float sx;
    Float sy;
    Float frame;
    Float gap;
    Float target_alpha;
    Float alpha;
    Angle angle;
    int action;
    Bool covered;
    int i;

    work = task->work;
    amy = player_work[0];
    if (edit_mode == 1) {
        ALPHA(work) = 0.5F;
        zero_draw(task);
        shadow_draw(work, 1.8F);
        zero_locked = FALSE;
        return;
    }
    if (amy == NULL) {
        sound_stop_id((int)work);
        zero_kill();
        return;
    }
    /* @hack spelling: Zero's position read through task->work again, not work
     * (MOV.L @(32,R13),R4 at 0x8c111c2c); through work, SHC passes R14 and 3
     * of 1912 words differ */
    /* @hack spelling: distance stored as it's tested, which gives it the frame
     * slot at sp+20 (MOV #20,R0 at 0x8c111c32); set apart, it takes sp+4, dx
     * and dz trade sp+76 and sp+80, and 26 of 1912 words differ */
    if ((distance = zero_amy_distance2(&task->work->pos)) >
        CHASE_RANGE * CHASE_RANGE) {
        sound_stop_id((int)work);
        zero_kill();
        return;
    }
    if (work->mode != ZERO_LEAVE && (work->tick & ZERO_LEAVING)) {
        VARS(work)->timer = 60;
        SPEED(work).x = 0.0F;
        SPEED(work).y = 0.0F;
        SPEED(work).z = 0.0F;
        zero_anim.action = ZERO_ACT_LEAVE;
        work->mode = ZERO_LEAVE;
    }
    if (event_in_control() == TRUE) {
        return;
    }
    if (work->mode != ZERO_WIND_UP && work->mode != ZERO_THROW &&
        work->mode != ZERO_HIT_BEHIND && work->mode != ZERO_HIT_FRONT &&
        work->mode != ZERO_LEAVE && (work->flags & COLLISION_DAMAGED)) {
        sound_stop_id((int)work);
        if (zero_damage < 1.0F) {
            zero_restore(work);
            dx = amy->pos.x - work->pos.x;
            dz = amy->pos.z - work->pos.z;
            zero_hit_pace =
                ((1.0F - zero_damage) * 0.5F) + (zero_damage * 2.0F);
            /* @hack spelling: 1.0F held in speed and read back, which loads
             * it into FR4 (FMOV.S FR14,FR4 at 0x8c111cd6); as 1.0F in the
             * expression, 46 of 1912 words differ */
            speed = 1.0F;
            /* @hack redundant: the speed multiplied by 1.0F (FMUL FR14,FR13
             * at 0x8c111d0e); without it, 49 of 1912 words differ */
            SPEED(work).y = speed = (speed - (zero_damage * 0.5F)) * 1.0F;
            /* @hack spelling: angle stored inside the call's argument, which
             * makes the frame 132 bytes, with dz at sp+76 (ADD R0,R15 at
             * 0x8c111b9a); set apart, the frame takes 128, zero_step runs 12
             * bytes short and 1872 of its 1912 words differ */
            if (angle_distance(work->ang.y, angle = (Angle)(atan2f(dz, -dx) *
                                                            ANGLE_PER_RADIAN)) <
                NJM_DEG_ANG(90)) {
                work->ang.y = angle;
                SPEED(work).x = -speed;
                zero_anim.action = ZERO_ACT_HIT_FRONT;
                work->mode = ZERO_HIT_FRONT;
            } else {
                work->ang.y = angle + NJM_DEG_ANG(180);
                SPEED(work).x = speed;
                zero_anim.action = ZERO_ACT_HIT_BEHIND;
                work->mode = ZERO_HIT_BEHIND;
            }
            if ((zero_damage += 0.2F) > 1.0F) {
                zero_damage = 1.0F;
            }
            sound_play_at(SE_HIT, 0, 0, 50, work->pos.x, work->pos.y,
                          work->pos.z);
        }
    }
    /* @hack spelling: action and frame read in the switch's comma expression,
     * after the constants SHC hoists out of the cases (MOV.W @(4,R12),R0 at
     * 0x8c111de2); as statements ahead of the switch, they come first and 8
     * of 1912 words differ */
    switch (action = zero_anim.action, frame = zero_anim.frame, work->mode) {
    case ZERO_APPEAR:
        if (--VARS(work)->timer <= 0) {
            SPEED(work).x = SPEED(work).z = 0.0F;
            SPEED(work).y = -1.0F;
            work->mode = ZERO_FALL;
            /* @hack redundant: a break ahead of the case's, which branches to
             * the ZERO_FALL store ZERO_STOMP's case shares (BRA at 0x8c111e66);
             * without it, 67 of 1912 words differ */
            break;
        }
        break;
    case ZERO_FALL:
        zero_fall(task);
        zero_move(task);
        zero_collide(task);
        if (!(work->flags & ZERO_ON_GROUND)) {
            break;
        }
        if (njAbs(SPEED(work).y) < 0.1F) {
            dx = amy->pos.x - work->pos.x;
            dz = amy->pos.z - work->pos.z;
            if (angle_distance(work->ang.y,
                               (Angle)(atan2f(dz, -dx) * ANGLE_PER_RADIAN)) <
                NJM_DEG_ANG(90)) {
                VARS(work)->timer = 120;
                zero_anim.action = ZERO_ACT_WALK;
                work->mode = ZERO_WALK;
                /* @hack redundant: a break ahead of the case's, which branches
                 * into ZERO_TURN_BODY's stores of ZERO_ACT_WALK and ZERO_WALK
                 * (BRA at 0x8c111ed2); without it, 15 of 1912 words differ */
                break;
            } else {
                VARS(work)->timer = 30;
                zero_anim.action = ZERO_ACT_WONDER_START;
                work->mode = ZERO_WONDER;
                /* @hack redundant: a break ahead of the case's, which branches
                 * to the work->mode store ZERO_WIND_UP's throw ends with (BRA
                 * at 0x8c111ede); without it, 6 of 1912 words differ */
                break;
            }
        } else {
            sound_play_at(SE_THUD, 0, 0, 50, work->pos.x, work->pos.y,
                          work->pos.z);
            vibration_start(0, 4, 15, 0);
            /* @hack redundant: a break ahead of the case's, which has
             * ZERO_WONDER load task_create's address into R3 and
             * zero_sight_saw's into R2 (0x8c111fa6, 0x8c111faa); without it,
             * the two trade and 11 of 1912 words differ */
            break;
        }
        break;
    case ZERO_WONDER:
        switch (action) {
        case ZERO_ACT_WONDER:
            if (VARS(work)->timer == 30) {
                zero_sight_saw = FALSE;
                if ((child = task_create(TASK_WORK, 6, zero_hatena_step)) !=
                    NULL) {
                    child->work->pos = work->pos;
                    child->work->pos.y += MARK_HEIGHT;
                }
                sound_play(SE_MARK, 0, 0, 0);
            }
            if (--VARS(work)->timer > 0) {
                break;
            }
            if (zero_restless > 0.9F && rand() * RAND_SCALE < 0.5F) {
                HEAD_AIM(work) = (Angle)RANDOM_ANGLE();
            } else {
                dx = amy->pos.x - work->pos.x;
                dz = amy->pos.z - work->pos.z;
                HEAD_AIM(work) = angle_difference(
                    work->ang.y, (Angle)(atan2f(dz, -dx) * ANGLE_PER_RADIAN));
                /* the head's aim strays up to an eighth of a turn either
                 * side of Amy */
                HEAD_AIM(work) += (Angle)(RANDOM(-0.5F, 0.5F) * 16384.0F);
            }
            HEAD_TURN(work) = 0;
            zero_set_action(ZERO_ACT_WONDER_END,
                            (Float)zero_actions[ZERO_ACT_WONDER_END]
                                    .action->motion->nbFrame -
                                1.0F);
            break;
        case ZERO_ACT_WONDER_END:
            if (zero_play_frames(-1.0F) == TRUE) {
                work->mode = ZERO_TURN_HEAD;
                sound_play_at(SE_TURN, 0, 0, 50, work->pos.x, work->pos.y,
                              work->pos.z);
            }
        }
        break;
    case ZERO_TURN_HEAD:
        HEAD_TURN(work) =
            angle_approach(HEAD_TURN(work), HEAD_AIM(work), 0x800);
        zero_neck_object[0].ang[1] = HEAD_TURN(work);
        if (angle_distance(HEAD_AIM(work), HEAD_TURN(work)) < 0x800) {
            work->mode = ZERO_TURN_BODY;
            sound_play_at(SE_TURN, 0, 0, 50, work->pos.x, work->pos.y,
                          work->pos.z);
            /* @hack redundant: a break ahead of the case's, which has
             * ZERO_TURN_BODY load angle_approach's address into R3
             * (0x8c1121c4); without it, R2 takes it and 2 of 1912 words
             * differ */
            break;
        }
        break;
    case ZERO_TURN_BODY:
        /* the body turns as far as the head turns back, so the head keeps
         * its heading */
        angle = angle_approach(HEAD_TURN(work), 0, 0x800);
        work->ang.y += angle_difference(angle, HEAD_TURN(work));
        HEAD_TURN(work) = angle;
        zero_neck_object[0].ang[1] = HEAD_TURN(work);
        if (angle_distance(HEAD_TURN(work), 0) < 0x800) {
            VARS(work)->timer = (int)RANDOM(120.0F, 180.0F);

            zero_anim.action = ZERO_ACT_WALK;
            work->mode = ZERO_WALK;
            /* @hack redundant: a break ahead of the case's, which ends the arm
             * with the stores of ZERO_ACT_WALK and ZERO_WALK that ZERO_FALL
             * branches into (0x8c112210); without it, 21 of 1912 words
             * differ */
            break;
        }
        break;
    case ZERO_WALK:
        VARS(work)->timer--;
        if (amy->pos.y - work->pos.y > REACH_HEIGHT) {
            VARS(work)->timer = 0;
        }
        if ((work->flags & ZERO_AT_WALL) && distance < 100.0F * 100.0F) {
            VARS(work)->timer = 0;
        }
        if (amy_carries_drum(0) && distance < 20.0F * 20.0F) {
            dx = amy->pos.x - work->pos.x;
            dz = amy->pos.z - work->pos.z;
            if (angle_distance(work->ang.y,
                               (Angle)(atan2f(dz, -dx) * ANGLE_PER_RADIAN)) <
                NJM_DEG_ANG(90)) {
                VARS(work)->timer = 0;
            }
        }
        if (VARS(work)->timer > 0) {
            if ((SPEED(work).x += 0.04F) > 1.25F) {
                SPEED(work).x = 1.25F;
            }
            if (distance > 100.0F * 100.0F) {
                dx = amy->pos.x - work->pos.x;
                dz = amy->pos.z - work->pos.z;
                work->ang.y = angle_approach(
                    work->ang.y, (Angle)(atan2f(dz, -dx) * ANGLE_PER_RADIAN),
                    0x400);
            }
        } else if ((SPEED(work).x += -0.1F) < 0.0F) {
            SPEED(work).x = 0.0F;
            if ((zero_restless += 0.125F) > 1.0F) {
                zero_restless = 1.0F;
            }
            if (distance < 150.0F * 150.0F &&
                rand() * RAND_SCALE < zero_restless) {
                zero_restless *= 0.5F;
                zero_anim.action = ZERO_ACT_STOMP_LIFT;
                work->mode = ZERO_STOMP;
                break;
            }
            if (amy->pos.y - work->pos.y > REACH_HEIGHT) {
                SPEED(work).x = LEAP_AHEAD;
                SPEED(work).y = LEAP_UP;
                work->mode = ZERO_FALL;
                sound_play_at(SE_LEAP, 0, 0, 50, work->pos.x, work->pos.y,
                              work->pos.z);
                for (i = 0; i < 5; i++) {
                    point.x = 0.5F - (rand() * RAND_SCALE);
                    point.y = -(rand() * (0.5F * RAND_SCALE));
                    point.z = 0.5F - (rand() * RAND_SCALE);
                    dust_create(&work->pos, &point, 1.0F);
                }
            } else {
                work->mode = ZERO_FALL;
                break;
            }
        }
        zero_fall(task);
        zero_move(task);
        zero_collide(task);
        if (zero_look_for_amy(task) == TRUE) {
            if ((child = task_create(TASK_WORK, 6, zero_hirame_step)) != NULL) {
                child->work->pos = work->pos;
                MARK_VARS(child->work)->follow = work;
            }
            sound_play(SE_MARK, 0, 0, 0);
            zero_restless = 0.0F;
            zero_anim.action = ZERO_ACT_AIM;
            work->mode = ZERO_AIM;
            /* @hack redundant: a break ahead of the case's, which branches to
             * the work->mode store ZERO_WIND_UP's throw ends with, ZERO_AIM in
             * its slot (BRA at 0x8c1124fe); without it, 4 of 1912 words
             * differ */
            break;
        }
        break;
    case ZERO_DASH_IN:
        VARS(work)->timer--;
        if (VARS(work)->timer > 0) {
            SPEED(work).x = DASH_SPEED;
        } else if ((SPEED(work).x += -0.5F) < 0.0F) {
            SPEED(work).x = 0.0F;
            work->mode = ZERO_FALL;
            sound_play(SE_DASH_END, 0, 0, 0);
            break;
        }
        zero_move(task);
        break;
    /* @hack redundant: a case for ZERO_STILL, a mode only an event's Zero
     * takes, and zero_event_step, not zero_step, runs an event's Zero. its arm
     * is the BRA at 0x8c1125a8, with a pool after it. without it, the pool
     * moves, zero_step runs 32 bytes longer and 1029 of its 1912 words
     * differ */
    case ZERO_STILL:
        break;
    case ZERO_AIM:
        if (action == ZERO_ACT_STAND) {
            SPEED(work).x = 0.0F;
            VARS(work)->timer = 120;
            zero_anim.action = ZERO_ACT_BEAM;
            work->mode = ZERO_BEAM;
            break;
        }
        if (njDistanceP2P(&work->pos, &amy->pos) > 40.0F) {
            if ((SPEED(work).x += 0.04F) > 0.625F) {
                SPEED(work).x = 1.25F;
            }
        } else if ((SPEED(work).x += -0.1F) < 0.0F) {
            SPEED(work).x = 0.0F;
        }
        if (zero_anim.frame < 30.0F) {
            if (!((int)zero_anim.frame & 7)) {
                sound_play_at(SE_AIM, 0, 0, 50, work->pos.x, work->pos.y,
                              work->pos.z);
            }
        }
        zero_fall(task);
        zero_move(task);
        zero_collide(task);
        break;
    case ZERO_BEAM:
        switch (action) {
        case ZERO_ACT_BEAM:
            if (frame <= 1.0F) {
                zero_beam_reset();
            } else if (frame >= 19.0F) {
                if (--VARS(work)->timer > 0) {
                    zero_beam(task);
                    if (zero_locked == TRUE && VARS(work)->timer > 60) {
                        VARS(work)->timer = 60;
                    }
                } else {
                    VARS(work)->timer = 14;
                    zero_anim.frame = VARS(work)->timer;
                    zero_anim.action = zero_anim.current = zero_anim.previous =
                        zero_anim.next = ZERO_ACT_BEAM_END;
                }
                sound_play_for(SE_BEAM, (int)work, 1, 0, 4);
            }
            break;
        case ZERO_ACT_BEAM_END:
            if (--VARS(work)->timer <= 1) {
                zero_anim.action = ZERO_ACT_STAND;
                break;
            }
            zero_anim.frame = VARS(work)->timer;
            break;
        default:
            if (zero_locked == TRUE) {
                VARS(work)->timer = 180;
                zero_anim.action = ZERO_ACT_WIND_UP;
                work->mode = ZERO_WIND_UP;
                zero_hand_state = ZERO_HAND_READY;
            } else {
                work->mode = ZERO_FALL;
            }
            break;
        }
        if (njDistanceP2P(&work->pos, &amy->pos) > 40.0F) {
            if ((SPEED(work).x += 0.04F) > 0.625F) {
                SPEED(work).x = 1.25F;
            }
        } else if ((SPEED(work).x += -0.1F) < 0.0F) {
            SPEED(work).x = 0.0F;
        }
        zero_fall(task);
        zero_move(task);
        zero_collide(task);
        if (zero_amy_distance2(&work->pos) > SIGHT_RANGE * SIGHT_RANGE ||
            amy->pos.y - work->pos.y > REACH_HEIGHT ||
            amy_carries_drum(0) == TRUE) {
            zero_locked = FALSE;
            work->mode = ZERO_FALL;
            /* @hack redundant: a break ahead of the case's, which branches to
             * the ZERO_FALL store ZERO_STOMP's case shares (BRA at 0x8c112876);
             * without it, 44 of 1912 words differ */
            break;
        }
        break;
    case ZERO_WIND_UP:
        if (action == ZERO_ACT_THROW && frame >= THROW_FRAME) {
            if (zero_locked == TRUE && !zero_hand_launch(task)) {
                sound_play_at(SE_LATCH, 0, 0, 50, work->pos.x, work->pos.y,
                              work->pos.z);
                VARS(work)->timer = 60;
                work->mode = ZERO_THROW;
                break;
            }
            work->mode = ZERO_FALL;
            break;
        }
        if (zero_locked == TRUE &&
            zero_amy_distance2(&work->pos) > LOCK_RANGE * LOCK_RANGE) {
            zero_locked = FALSE;
        }
        break;
    case ZERO_THROW:
        if (--VARS(work)->timer <= 0) {
            zero_locked = FALSE;
            VARS(work)->timer = 180;
            work->mode = ZERO_REEL;
            break;
        }
        if (zero_locked == TRUE &&
            zero_amy_distance2(&work->pos) > LOCK_RANGE * LOCK_RANGE) {
            zero_locked = FALSE;
            break;
        }
        switch (zero_hand_fly(task)) {
        /* @hack redundant: a case for ZERO_HAND_FLYING that does nothing, which
         * puts a test of its own in the compare chain (CMP/EQ #3,R0 at
         * 0x8c1129aa); without it, 41 of 1912 words differ */
        case ZERO_HAND_FLYING:
            break;
        case ZERO_HAND_UNTHROWN:
        case ZERO_HAND_CAUGHT:
        case ZERO_HAND_MISSED:
        case ZERO_HAND_BACK:
            zero_locked = FALSE;
            sound_stop_id((int)work);
            sound_play_held(SE_REEL, (int)work, 1, 0);
            VARS(work)->timer = 180;
            work->mode = ZERO_REEL;
            break;
        }
        break;
    case ZERO_REEL:
        /* @hack spelling: a switch of one case, which loads sound_stop_id's
         * address into R2 (0x8c1129f0); as an if, R3 takes it and 2 of 1912
         * words differ */
        switch (zero_hand_return(task)) {
        case ZERO_HAND_HOME:
            sound_stop_id((int)work);
            work->mode = ZERO_FALL;
            break;
        }
        break;
    case ZERO_HIT_FRONT:
    case ZERO_HIT_BEHIND:
        if (action == ZERO_ACT_STAND) {
            work->mode = ZERO_FALL;
            break;
        }
        zero_fall(task);
        zero_move(task);
        zero_collide(task);
        if (!(work->flags & ZERO_ON_GROUND)) {
            break;
        }
        SPEED(work).x *= 0.9F;
        SPEED(work).z *= 0.9F;
        if (zero_anim.action == ZERO_ACT_HIT_BEHIND ||
            zero_anim.action == ZERO_ACT_HIT_FRONT) {
            point = work->pos;
            point.x += (0.5F - (rand() * RAND_SCALE)) * 16.0F;
            point.y -= 8.0F;
            point.z += (0.5F - (rand() * RAND_SCALE)) * 16.0F;
            vector.x = vector.y = vector.z = 0.0F;
            dust_create(&point, &vector, 1.0F);
        }
        if (zero_anim.action == ZERO_ACT_HIT_BEHIND_LAND ||
            zero_anim.action == ZERO_ACT_HIT_FRONT_LAND) {
            if (zero_anim.frame < 1.0F) {
                sound_play_at(SE_KNOCK_LAND, 0, 0, 50, work->pos.x, work->pos.y,
                              work->pos.z);
            }
        }
        if (njScalor(&SPEED(work)) > 0.5F) {
            sound_play_at(SE_SLIDE, 0, 0, 50, work->pos.x, work->pos.y,
                          work->pos.z);
            vibration_start(0, 4, 15, 0);
        }
        break;
    case ZERO_STOMP:
        if (action == ZERO_ACT_STAND) {
            work->mode = ZERO_FALL;
            break;
        }
        if (action == ZERO_ACT_STOMP && frame >= STOMP_FRAME &&
            frame < STOMP_FRAME + 1.0F) {
            if ((child = task_create(TASK_WORK, 6, zero_shock_step)) != NULL) {
                child->work->pos = work->pos;
                child->work->pos.x += -(njCos(work->ang.y) * 15.0F);
                child->work->pos.z += njSin(work->ang.y) * 15.0F;
                child->work->pos.y += -12.0F;
            }
            sound_play_at(SE_THUD, 0, 0, 50, work->pos.x, work->pos.y,
                          work->pos.z);
            sound_play_at(SE_STOMP, 0, 0, 50, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        break;
    case ZERO_LEAVE:
        zero_locked = FALSE;
        if ((SPEED(work).y += 0.2F) > 4.0F) {
            SPEED(work).y = 4.0F;
        }
        zero_move(task);
        point.x = 0.5F - (rand() * RAND_SCALE);
        point.y = -(rand() * (0.5F * RAND_SCALE));
        point.z = 0.5F - (rand() * RAND_SCALE);
        dust_create(&work->pos, &point, 1.0F);
        if (--VARS(work)->timer <= 0) {
            sound_stop_id((int)work);
            zero_kill();
            return;
        }
        break;
    }
    switch (work->mode) {
    case ZERO_AIM:
    case ZERO_BEAM:
        dx = amy->pos.x - work->pos.x;
        dz = amy->pos.z - work->pos.z;
        work->ang.y = angle_approach(
            work->ang.y, (Angle)(atan2f(dz, -dx) * ANGLE_PER_RADIAN), 0x400);
        break;
    case ZERO_WIND_UP:
    case ZERO_THROW:
        /* @hack redundant: the lock's y read and dropped, which SHC weighs into
         * the frame: the turn above gives dx and dz sp+44 and sp+40 (MOV
         * #44,R0 at 0x8c112d86), and lock_z takes sp+36; without it, 4 of
         * 1912 words differ */
        (void)zero_lock_pos.y;
        /* @hack spelling: lock_x and lock_z, locals of their own, not dx and
         * dz, which keep camera_work's and njProjectScreen's addresses in R8
         * and R9 below (0x8c112df4, 0x8c112e02); with dx and dz, the two
         * trade and 7 of 1912 words differ */
        lock_x = zero_lock_pos.x - work->pos.x;
        lock_z = zero_lock_pos.z - work->pos.z;
        work->ang.y = angle_approach(
            work->ang.y, (Angle)(atan2f(lock_z, -lock_x) * ANGLE_PER_RADIAN),
            0x400);
        break;
    }
    collision_register(work);
    player_anim_step(&zero_anim);
    /* Zero covers Amy when Zero lies within 10 of the camera. it also covers
     * her when it lies nearer the camera than she does and overlaps her on
     * the screen. a Zero that covers her fades toward an alpha of 0.15, and
     * any other toward 1 */
    covered = FALSE;
    /* @hack spelling: a loop of one pass, which SHC weighs into its choice
     * of registers and frame slots: player 0's work takes R11 (0x8c111baa),
     * and distance sp+20 (0x8c111c32). without it, player 0's work takes
     * R10 and distance sp+4, and 185 of 1912 words differ */
    do {
        point = amy->collision->shapes->center;
        vector = work->pos;
        vector.y += 2.0F;
        amy_depth = njDistanceP2P(&camera_work->pos, &point);
        if ((zero_depth = njDistanceP2P(&camera_work->pos, &vector)) < 10.0F) {
            covered = TRUE;
            break;
        }
        if (amy_depth > zero_depth) {
            njProjectScreen(NULL, &point, &amy_screen);
            njProjectScreen(NULL, &vector, &zero_screen);
            amy_radius = _nj_screen_.dist * 5.0F / amy_depth;
            zero_radius = _nj_screen_.dist * 10.0F / zero_depth;
            sx = amy_screen.x - zero_screen.x;
            sy = amy_screen.y - zero_screen.y;
            gap = njSqrt((sx * sx) + (sy * sy));
            if (gap < amy_radius + zero_radius) {
                covered = TRUE;
            }
        }
    } while (0);
    target_alpha = 1.0F;
    if (covered == TRUE) {
        target_alpha = 0.15F;
    }
    if (ALPHA(work) < target_alpha) {
        if ((ALPHA(work) += 0.05F) > 1.0F) {
            ALPHA(work) = 1.0F;
        }
    } else if (ALPHA(work) > target_alpha) {
        if ((ALPHA(work) -= 0.05F) < 0.0F) {
            ALPHA(work) = 0.0F;
        }
    }
    zero_draw(task);
    shadow_draw(work, 1.8F);
    if (work->mode != ZERO_HIT_BEHIND && work->mode != ZERO_HIT_FRONT &&
        zero_damage == 1.0F && rand() * RAND_SCALE < 0.05F) {
        point = work->pos;
        point.x += (0.5F - (rand() * RAND_SCALE)) * 10.0F;
        point.y += 15.0F;
        point.z += (0.5F - (rand() * RAND_SCALE)) * 10.0F;
        vector.x = 0.5F - (rand() * RAND_SCALE);
        vector.y = 0.5F;
        vector.z = 0.5F - (rand() * RAND_SCALE);
        snow_create(&point, &vector, 0.005F);
    }
    /* Zero's children, its reflections in Twinkle Park's mirrors, draw it
     * opaque */
    alpha = ALPHA(work);
    ALPHA(work) = 1.0F;
    task_step_children(task);
    ALPHA(work) = alpha;
    zero_clear_request();
}

/* find, in result, the nearest surface on each side of pos within radius,
 * among the world's objects but liquids. enum side gives two sides to each
 * axis, so side i lies along pos's axis i >> 1. it returns TRUE when it
 * finds any surface, and FALSE when it finds none */
static Bool zero_find_sides(struct sides_query *result, NJS_POINT3 *pos,
                            Float radius)
{
    struct sides_query query;
    struct world_object *object;
    Bool found;
    int count;
    int i;
    Bool better;

    found = FALSE;
    world_list_near(pos->x, pos->y, pos->z, radius);
    for (i = 0; i < SIDES; i++) {
        result->sides[i].found = FALSE;
    }
    query.pos = *pos;
    for (count = world_count, object = world_objects; count > 0;
         count--, object++) {
        if (object->flags & WORLD_LIQUID) {
            continue;
        }
        if (!polygon_find_sides(&query, object->object, object->flags)) {
            continue;
        }
        for (i = 0; i < SIDES; i++) {
            if (!query.sides[i].found) {
                continue;
            }
            /* @hack type: pos's axis read through a Float pointer, which
             * scales i >> 1 in R1 and the side's offset in R3 (0x8c1131c0 to
             * 0x8c1131cc); as (&pos->x)[i >> 1], they move to R3 and R2 and
             * 12 of 209 words differ */
            if (njAbs(((Float *)pos)[i >> 1] - query.sides[i].level) > radius) {
                continue;
            }
            if (!result->sides[i].found) {
                result->sides[i] = query.sides[i];
                found = TRUE;
                continue;
            }
            better = FALSE;
            if (i & 1) {
                if (query.sides[i].level < result->sides[i].level) {
                    better = TRUE;
                }
            } else if (query.sides[i].level > result->sides[i].level) {
                better = TRUE;
            }
            if (better == TRUE) {
                result->sides[i] = query.sides[i];
                found = TRUE;
            }
        }
    }
    return found;
}

/* push Zero out to ZERO_RADIUS from the surfaces around it. a floor
 * bounces it up at half its falling speed, or at 1 when it lifts Zero
 * more than 3 */
static void zero_collide(struct task *task)
{
    struct task_work *work = task->work;
    struct sides_query hits;
    Float y;

    zero_find_sides(&hits, &work->pos, ZERO_RADIUS);
    work->flags &= ~ZERO_AT_WALL;
    if (hits.sides[SIDE_MINUS_X].found == TRUE &&
        work->pos.x - hits.sides[SIDE_MINUS_X].level < ZERO_RADIUS) {
        work->pos.x = hits.sides[SIDE_MINUS_X].level + ZERO_RADIUS;
        work->flags |= ZERO_AT_WALL;
    }
    if (hits.sides[SIDE_PLUS_X].found == TRUE &&
        hits.sides[SIDE_PLUS_X].level - work->pos.x < ZERO_RADIUS) {
        work->pos.x = hits.sides[SIDE_PLUS_X].level - ZERO_RADIUS;
        work->flags |= ZERO_AT_WALL;
    }
    if (hits.sides[SIDE_MINUS_Z].found == TRUE &&
        work->pos.z - hits.sides[SIDE_MINUS_Z].level < ZERO_RADIUS) {
        work->pos.z = hits.sides[SIDE_MINUS_Z].level + ZERO_RADIUS;
        work->flags |= ZERO_AT_WALL;
    }
    if (hits.sides[SIDE_PLUS_Z].found == TRUE &&
        hits.sides[SIDE_PLUS_Z].level - work->pos.z < ZERO_RADIUS) {
        work->pos.z = hits.sides[SIDE_PLUS_Z].level - ZERO_RADIUS;
        work->flags |= ZERO_AT_WALL;
    }
    work->flags &= ~ZERO_ON_GROUND;
    if (hits.sides[SIDE_BELOW].found == TRUE &&
        work->pos.y - hits.sides[SIDE_BELOW].level < ZERO_RADIUS) {
        y = work->pos.y;
        work->pos.y = hits.sides[SIDE_BELOW].level + ZERO_RADIUS;
        work->flags |= ZERO_ON_GROUND;
        if (work->pos.y - y > 3.0F) {
            SPEED(work).y = 1.0F;
        } else {
            SPEED(work).y *= -0.5F;
        }
    }
    if (hits.sides[SIDE_ABOVE].found == TRUE &&
        hits.sides[SIDE_ABOVE].level - work->pos.y < ZERO_RADIUS) {
        work->pos.y = hits.sides[SIDE_ABOVE].level - ZERO_RADIUS;
        SPEED(work).y = 0.0F;
    }
}

static void zero_sight_step(struct task *task)
{
    struct task_work *work = task->work;
    struct sides_query hits;
    struct surface *side;
    struct world_object *object;
    struct collision_work *hit;
    Uint16 kind;
    const char *axis;
    int count;
    int i;

    switch (work->mode) {
    case SIGHT_START:
        collision_init(task, zero_sight_shape, COLLISION_OBJECT, 1, 1);
        SIGHT_VARS(work)->travel = 0.0F;
        work->mode = SIGHT_FLY;
        break;
    case SIGHT_FLY:
        if ((SIGHT_VARS(work)->travel += 5.0F) >= SIGHT_RANGE) {
            zero_sight_saw = FALSE;
            zero_sight_out = FALSE;
            task_kill(task);
            return;
        }
        /* @hack spelling: the hit stored as it's tested, which puts it in R5
         * (MOV.L @(24,R5),R5 at 0x8c113520); set apart, it takes R4, its
         * kind R5, and 8 of 177 words differ */
        if ((hit = work->collision->hit) != NULL) {
            /* @hack spelling: the kind stored as it's tested, which puts it in
             * R4 (MOV.W @R5,R4 at 0x8c113526); set apart, it shares R4 with
             * the hit and 4 of 177 words differ */
            if (!(kind = hit->kind)) {
                zero_sight_saw = TRUE;
                zero_sight_out = FALSE;
                task_kill(task);
                return;
            }
            if (kind >= COLLISION_OBJECT && kind <= COLLISION_RING) {
                zero_sight_saw = FALSE;
                zero_sight_out = FALSE;
                task_kill(task);
                return;
            }
        }
        world_list_near(work->pos.x, work->pos.y, work->pos.z, 5.0F);
        hits.pos = work->pos;
        for (count = world_count, object = world_objects; count > 0;
             count--, object++) {
            if (polygon_find_sides(&hits, object->object, object->flags) ==
                TRUE) {
                side = hits.sides;
                /* @hack spelling: pos's axes through a byte cursor stepped on
                 * each odd side; indexed by the side, the function runs 10
                 * bytes short and 141 of 177 words differ */
                axis = (const char *)&work->pos;
                for (i = SIDES; i > 0; i--) {
                    if (side->found == TRUE &&
                        njAbs(side->level - *(const Float *)axis) < 5.0F) {
                        zero_sight_saw = FALSE;
                        zero_sight_out = FALSE;
                        task_kill(task);
                        return;
                    }
                    side++;
                    if (i & 1) {
                        axis += sizeof(Float);
                    }
                }
            }
        }
        VECTOR_ADD(&work->pos, &work->scl);
        collision_register(work);
        break;
    }
}

/* whether Zero spots Amy, TRUE or FALSE. while she carries a drum or Zero
 * is blind, it never does, and sends out no sight. otherwise it sends out
 * a sight unless one is flying. it spots her only within SIGHT_RANGE and
 * while she isn't balancing. given both, it spots her if she recoils. it
 * also spots her if she stands within 100 degrees of its heading and the
 * last sight touched a player */
static Bool zero_look_for_amy(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR dir;
    NJS_POINT3 from;
    NJS_POINT3 target;
    NJS_VECTOR speed;
    Float distance;
    struct task *sight;

    if (amy_carries_drum(0) == TRUE) {
        return FALSE;
    }
    if (zero_request.blind == TRUE) {
        return FALSE;
    }
    from = work->pos;
    target = player_work[0]->pos;
    target.y += AMY_MIDDLE;
    dir.x = target.x - from.x;
    dir.y = target.y - from.y;
    dir.z = target.z - from.z;
    if ((distance = njScalor(&dir)) == 0.0F) {
        dir.x = 1.0F;
        dir.y = dir.z = 0.0F;
    } else {
        njUnitVector(&dir);
    }
    if (zero_sight_out == FALSE &&
        (sight = task_create(TASK_WORK, 6, zero_sight_step)) != NULL) {
        zero_sight_out = TRUE;
        speed = dir;
        speed.x *= 5.0F;
        speed.y *= 5.0F;
        speed.z *= 5.0F;
        sight->work->pos = from;
        sight->work->scl = speed;
    }
    if (distance > SIGHT_RANGE) {
        return FALSE;
    }
    if (amy_get_action(0) == AMY_BALANCING) {
        return FALSE;
    }
    if (amy_get_action(0) == AMY_RECOILING) {
        return TRUE;
    }
    if (angle_distance(work->ang.y,
                       (Angle)(atan2f(dir.z, -dir.x) * ANGLE_PER_RADIAN)) >
        NJM_DEG_ANG(100)) {
        return FALSE;
    }
    if (zero_sight_saw == FALSE) {
        return FALSE;
    }
    return TRUE;
}

static Float zero_distance2(NJS_POINT3 *from, NJS_POINT3 *to)
{
    NJS_VECTOR between;

    between = *to;
    njSubVector(&between, from);
    return njScalor2(&between);
}

/* the squared distance from pos to Amy's middle */
static Float zero_amy_distance2(NJS_POINT3 *pos)
{
    NJS_POINT3 target;
    NJS_VECTOR away;

    target = player_work[0]->pos;
    target.y += AMY_MIDDLE;
    away.x = target.x - pos->x;
    away.y = target.y - pos->y;
    away.z = target.z - pos->z;
    return (away.x * away.x) + (away.y * away.y) + (away.z * away.z);
}

/* draw Zero on a frame's first step: by an event's motions while it has
 * them, or else by its action. only the action's draw takes its opacity
 * from ALPHA, and draws Zero redder once fully hurt */
static void zero_draw(struct task *task)
{
    struct task_work *work;
    struct player_action *entry;
    Float red;
    Float green;
    Float blue;
#ifdef GCC
    /* the split frame's second picture puts the bob back once Zero is
     * drawn, so that the bob, which zero_beam's and zero_find_arm_end's
     * poses translate by, stays as the steps left it */
    Float bob = zero_bob_object.pos[1];
#endif

    if (frame_step != 0) {
        return;
    }
    /* @hack spelling: two statements as one comma expression, which puts 1.0F
     * in FR15 (FLDI1 FR15 at 0x8c1138fa); as two, FR14 takes it and 14 of
     * 195 words differ */
    work = task->work, zero_bob_object.evalflags |= NJD_EVAL_UNIT_POS;
    switch (zero_anim.action) {
    case ZERO_ACT_WALK:
    case ZERO_ACT_STAND:
        if (work->mode != ZERO_FALL) {
            zero_bob_object.evalflags &= ~NJD_EVAL_UNIT_POS;
            zero_bob_object.pos[1] =
                njCos((Angle)((zero_anim.frame - 5.0F) * 4096.0F)) * BOB_HEIGHT;
        }
        break;
    }
    njSetTexture(&zero_texlist);
    if (work->event != NULL && work->event->motions != NULL) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        event_motions_draw(work);
        njPopMatrix(1);
    } else {
        draw_attr_save();
        if (ALPHA(work) < 1.0F) {
            draw_attr_set(0, NJD_FLAG_USE_ALPHA);
        }
        red = green = blue = 0.0F;
        if (zero_damage == 1.0F) {
            green = blue = -0.4F;
        }
        draw_set_material(ALPHA(work) - 1.0F, red, green, blue);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        if (zero_anim.state == PLAYER_ANIM_BLENDING) {
            if (zero_damage == 1.0F) {
                njSimpleAction(zero_anim.blend, zero_anim.frame);
            } else {
                draw_action(zero_anim.blend, zero_anim.frame);
            }
        } else {
            entry = &zero_anim.actions[zero_anim.action];
            if (zero_damage == 1.0F) {
                njSimpleAction(entry->action, zero_anim.frame);
            } else {
                draw_action(entry->action, zero_anim.frame);
            }
        }
        njPopMatrix(1);
        draw_attr_restore();
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
#ifdef GCC
    if (frame_redrawing) {
        zero_bob_object.pos[1] = bob;
    }
#endif
}

/* start the beam over: no length, no lock, and its aim within 5 of Amy's
 * position on x and z */
static void zero_beam_reset(void)
{
    zero_beam_hit = FALSE;
    zero_beam_turn_timer = 17;
    zero_beam_length = 0.0F;
    zero_beam_aim = player_work[0]->pos;
    zero_beam_aim.x += RANDOM(-0.5F, 0.5F) * 10.0F;
    zero_beam_aim.z += RANDOM(-0.5F, 0.5F) * 10.0F;
    zero_beam_drift.x = zero_beam_drift.y = zero_beam_drift.z = 0.0F;
    zero_locked = FALSE;
}

/* a step of the beam from Zero's eye. it grows by 1 a step to BEAM_LENGTH,
 * and stops at the world unless zero_beam_unblocked. it leaves a sweep
 * each step. when its end meets the world on two steps running, at points
 * less than 5 apart, it leaves a scorch between them. it locks on once it
 * passes within 4 of Amy's middle */
static void zero_beam(struct task *task)
{
    struct task_work *work = task->work;
    struct player_anim *anim;
    NJS_ACTION *action;
    NJS_POINT3 *stride;
    Float falloff;
    Float length;

    /* @hack spelling: a bare scope, whose pos and point share their frame
     * slots with the walk's below (ADD #-80,R15 at 0x8c113bd6); declared
     * with the function's own locals, they take slots of their own, the
     * frame grows to 104 bytes and 14 of 639 words differ */
    {
        NJS_POINT3 pos;
        NJS_POINT3 point;

        njPushMatrix(&_unit_matrix_);
        pos.x = 4.5F;
        pos.y = -5.0F;
        pos.z = 0.0F;
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        /* @hack spelling: zero_anim through a pointer, which leaves R8 to hold
         * TRUE (MOV #1,R8 at 0x8c113eb2); by name, R8 holds task_create's
         * address instead and 206 of 639 words differ */
        anim = &zero_anim;
        if (anim->state == PLAYER_ANIM_BLENDING) {
            action = anim->blend;
        } else {
            action = zero_actions[anim->action].action;
        }
        pose_action(NULL, action, anim->frame, EYE_NODE, &zero_node_matrix);
        njCalcPoint(&zero_node_matrix, &pos, &point);
        zero_beam_points[0] = point;
        njPopMatrix(1);

        if ((zero_beam_length += 1.0F) > BEAM_LENGTH) {
            zero_beam_length = BEAM_LENGTH;
        }
        /* every 17 to 33 steps, zero_beam_drift turns to a unit vector
         * toward Amy's middle. in between, each of its axes leans 0.01 a
         * step toward her position, held to -1 to 1 */
        if (--zero_beam_turn_timer <= 0) {
            zero_beam_turn_timer = (int)(rand() * (17.0F * RAND_SCALE)) + 17;
            pos.x = player_work[0]->pos.x - zero_beam_aim.x;
            pos.y = player_work[0]->pos.y - zero_beam_aim.y + AMY_MIDDLE;
            pos.z = player_work[0]->pos.z - zero_beam_aim.z;
            if (njScalor(&pos) == 0.0F) {
                zero_beam_drift.x = zero_beam_drift.y = zero_beam_drift.z =
                    0.0F;
            } else {
                njUnitVector(&pos);
                zero_beam_drift = pos;
            }
        } else {
            if (zero_beam_aim.x < player_work[0]->pos.x) {
                if ((zero_beam_drift.x += 0.01F) > 1.0F) {
                    zero_beam_drift.x = 1.0F;
                }
            } else {
                if ((zero_beam_drift.x -= 0.01F) < -1.0F) {
                    zero_beam_drift.x = -1.0F;
                }
            }
            if (zero_beam_aim.y < player_work[0]->pos.y) {
                if ((zero_beam_drift.y += 0.01F) > 1.0F) {
                    zero_beam_drift.y = 1.0F;
                }
            } else {
                if ((zero_beam_drift.y -= 0.01F) < -1.0F) {
                    zero_beam_drift.y = -1.0F;
                }
            }
            if (zero_beam_aim.z < player_work[0]->pos.z) {
                if ((zero_beam_drift.z += 0.01F) > 1.0F) {
                    zero_beam_drift.z = 1.0F;
                }
            } else {
                if ((zero_beam_drift.z -= 0.01F) < -1.0F) {
                    zero_beam_drift.z = -1.0F;
                }
            }
        }
        zero_beam_aim.x += zero_beam_drift.x;
        zero_beam_aim.y += zero_beam_drift.y;
        zero_beam_aim.z += zero_beam_drift.z;
    }

    zero_beam_dir.x = zero_beam_aim.x - zero_beam_points[0].x;
    zero_beam_dir.y = zero_beam_aim.y - zero_beam_points[0].y;
    zero_beam_dir.z = zero_beam_aim.z - zero_beam_points[0].z;
    if (njScalor(&zero_beam_dir) == 0.0F) {
        /* @hack spelling: zero_beam_dir through its address, which puts
         * zero_beam_turn_timer's address in R11 and zero_beam_aim's in R12
         * (0x8c113c72, 0x8c113c74); by name, the two trade and 224 of 639
         * words differ */
        (&zero_beam_dir)->x = 1.0F;
        (&zero_beam_dir)->y = (&zero_beam_dir)->z = 0.0F;
    } else {
        njUnitVector(&zero_beam_dir);
    }
    zero_beam_end.x =
        (zero_beam_dir.x * zero_beam_length) + zero_beam_points[0].x;
    zero_beam_end.y =
        (zero_beam_dir.y * zero_beam_length) + zero_beam_points[0].y;
    zero_beam_end.z =
        (zero_beam_dir.z * zero_beam_length) + zero_beam_points[0].z;

    if (zero_beam_unblocked == FALSE) {
        struct ray_query query;

        query.length = zero_beam_length;
        query.origin = zero_beam_points[0];
        query.dir = zero_beam_dir;
        if (polygon_cast_ray(&query) == TRUE) {
            zero_beam_end = query.hit;
            if (zero_beam_hit == TRUE &&
                zero_distance2(&zero_beam_end, &zero_beam_last_end) < 25.0F) {
                struct task *child;

                if ((child = task_create(TASK_WORK, 6, zero_scorch_step)) !=
                    NULL) {
                    child->work->pos = zero_beam_end;
                    child->work->scl = zero_beam_last_end;
                }
            }
            zero_beam_hit = TRUE;
        } else {
            zero_beam_hit = FALSE;
        }
        zero_beam_points[1] = zero_beam_end;
    } else {
        zero_beam_hit = FALSE;
        zero_beam_points[1] = zero_beam_end;
    }

    falloff =
        1.0F - (njDistanceP2P(&zero_beam_points[0], &zero_beam_points[1]) *
                (1.0F / BEAM_LENGTH));
    zero_beam_colors[1].argb.a = falloff * 255.0F;
    if (frame_step == 0) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njDrawLine3D(&zero_beam_line, 1, NJD_TRANSPARENT);
    }
    if (zero_beam_length > 1.0F) {
        struct task *child;

        if ((child = task_create(TASK_WORK, 6, zero_sweep_step)) != NULL) {
            child->work->pos = zero_beam_points[0];
            child->work->scl = zero_beam_points[1];
            SWEEP_VARS(child->work)->last_end.x = zero_beam_last_end.x;
            SWEEP_VARS(child->work)->last_end.y = zero_beam_last_end.y;
            SWEEP_VARS(child->work)->last_end.z = zero_beam_last_end.z;
            child->work->ang.x =
                (zero_beam_last_start.x - zero_beam_points[0].x) * 65536.0F;
            child->work->ang.y =
                (zero_beam_last_start.y - zero_beam_points[0].y) * 65536.0F;
            child->work->ang.z =
                (zero_beam_last_start.z - zero_beam_points[0].z) * 65536.0F;
        }
    }
    if (zero_locked == FALSE) {
        NJS_POINT3 pos;
        NJS_POINT3 point;
        NJS_POINT3 target;

        length = njDistanceP2P(&zero_beam_points[0], &zero_beam_points[1]);
        pos = zero_beam_points[0];
        /* @hack spelling: the stride through a pointer, which keeps the drift's
         * step, 0.01F, in FR5 (FADD FR5,FR3 at 0x8c113d1c); through point by
         * name, FR4 takes it and 218 of 639 words differ */
        stride = &point;
        *stride = zero_beam_dir;
        /* @hack spelling: the products spelled out, which leave R8 to hold
         * TRUE (MOV #1,R8 at 0x8c113eb2) and load player_work's address
         * afresh at each use (0x8c113c90, 0x8c113d06); with *=,
         * player_work's address takes R8 and 305 of 639 words differ */
        stride->x = stride->x * 4.0F;
        stride->y = stride->y * 4.0F;
        stride->z = stride->z * 4.0F;
        /* @hack redundant: Amy's middle worked out into target, which
         * nothing reads (0x8c1140e0 to 0x8c1140fe); without it, zero_beam
         * runs 42 bytes short and 415 of 639 words differ */
        target = player_work[0]->pos;
        target.y += AMY_MIDDLE;
        if (length > 0.0F) {
            do {
                if (zero_amy_distance2(&pos) < 4.0F * 4.0F) {
                    zero_locked = TRUE;
                    task_create(TASK_WORK, 6, zero_marker_step);
                    sound_play(SE_LOCK, 0, 0, 0);
                    break;
                }
                length -= 4.0F;
                pos.x += stride->x;
                pos.y += stride->y;
                pos.z += stride->z;
            } while (length > 0.0F);
        }
    }
    zero_beam_last_start = zero_beam_points[0];
    /* @hack redundant: zero_beam_last_end read and dropped, which puts its
     * address in R9 and TRUE in R8 (MOV #1,R8 at 0x8c113eb2); without it,
     * the two trade and 13 of 639 words differ */
    (void)zero_beam_last_end.x;
    zero_beam_last_end = zero_beam_end;
}

static void zero_sweep_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 last_end;
    Float fade;
    Float near_alpha;
    Float far_alpha;
    Float falloff;

    if (++TASK_TIMER(work) >= 8) {
        task_kill(task);
        return;
    }
    if (frame_step != 0) {
        return;
    }
    last_end.x = SWEEP_VARS(work)->last_end.x;
    last_end.y = SWEEP_VARS(work)->last_end.y;
    last_end.z = SWEEP_VARS(work)->last_end.z;
    fade = (8 - TASK_TIMER(work)) * 0.125F;
    zero_sweep_colors[0].argb.a = near_alpha = fade * 192.0F;
    falloff =
        1.0F - (njDistanceP2P(&work->pos, &work->scl) * (1.0F / BEAM_LENGTH));
    zero_sweep_colors[1].argb.a = near_alpha * falloff;
    fade -= 0.125F;
    zero_sweep_colors[3].argb.a = far_alpha = fade * 192.0F;
    falloff =
        1.0F - (njDistanceP2P(&work->pos, &last_end) * (1.0F / BEAM_LENGTH));
    zero_sweep_colors[2].argb.a = far_alpha * falloff;
    zero_sweep_points[0] = work->pos;
    zero_sweep_points[1] = work->scl;
    zero_sweep_points[2] = last_end;
    zero_sweep_points[3].x = work->pos.x + (work->ang.x * (1.0F / 65536.0F));
    zero_sweep_points[3].y = work->pos.y + (work->ang.y * (1.0F / 65536.0F));
    zero_sweep_points[3].z = work->pos.z + (work->ang.z * (1.0F / 65536.0F));
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njDrawPolygon3D(&zero_sweep_quad, 4, NJD_FILL | NJD_TRANSPARENT);
    placement_try_leave(task);
}

/* a scorch: a line between where the beam's end met the world on two
 * steps running, which pos and scl hold. it fades over 45 steps */
static void zero_scorch_step(struct task *task)
{
    struct task_work *work = task->work;
    Float fade;

    if (++TASK_TIMER(work) >= 45) {
        task_kill(task);
        return;
    }
    if (frame_step != 0) {
        return;
    }
    fade = (45 - TASK_TIMER(work)) * (1.0F / 45.0F);
    zero_scorch_colors[0].argb.a = fade * 255.0F;
    fade -= 1.0F / 45.0F;
    zero_scorch_colors[1].argb.a = fade * 255.0F;
    zero_scorch_points[0] = work->pos;
    zero_scorch_points[1] = work->scl;
    njDrawLine3D(&zero_scorch_line, 1, NJD_TRANSPARENT);
    placement_try_leave(task);
}

/* the lock's marker. it shrinks onto the lock, and keeps zero_lock_pos on
 * Amy's middle while it is in MARKER_LOCK and no event is in control. it
 * turns red and spins faster as Zero winds up. once the lock lets go, it
 * turns blue, and 30 steps later swells away */
static void zero_marker_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_COLOR *color;
    int alpha;
    int red;
    int green;
    int blue;

#ifdef GCC
    /* GCC can't see that work->mode is always one of the cases: task_create
     * zeroes it to MARKER_START, and only this function sets it */
    alpha = red = green = blue = 0;
#endif
    switch (work->mode) {
    case MARKER_START:
        work->scl.x = 100.0F;
        work->ang.x = 0;
        work->ang.y = NJM_DEG_ANG(120);
        alpha = 255;
        red = 255;
        green = 255;
        blue = 0;
        work->mode = MARKER_LOCK;
        break;
    case MARKER_LOCK:
        if (!event_in_control()) {
            zero_lock_pos = player_work[0]->pos;
            zero_lock_pos.y += AMY_MIDDLE;
        }
        if ((work->scl.x *= 0.8F) < 8.0F) {
            work->scl.x = 8.0F;
        }
        work->ang.x += 0x200;
        work->ang.y += 0x200;
        green = work->ang.x >> 6;
        alpha = 160;
        red = 255;
        blue = 0;
        if (zero_hand_state == ZERO_HAND_READY) {
            work->scl.x = 10.0F;
            work->mode = MARKER_WIND_UP;
        }
        if (zero_locked == FALSE) {
            work->scl.x = 8.0F;
            TASK_TIMER(work) = 30;
            work->mode = MARKER_LOST;
        }
        break;
    case MARKER_WIND_UP:
        work->ang.x += 0x600;
        work->ang.y += 0x600;
        alpha = 160;
        red = 255;
        green = 0;
        blue = 0;
        if (zero_locked == FALSE) {
            TASK_TIMER(work) = 30;
            work->mode = MARKER_LOST;
        }
        break;
    case MARKER_LOST:
        if (--TASK_TIMER(work) <= 0) {
            MARKER_VARS(work)->grow = 1.0F;
            work->mode = MARKER_SWELL;
        }
        alpha = 160;
        green = red = 64;
        blue = 255;
        break;
    case MARKER_SWELL:
        MARKER_VARS(work)->grow += -0.1F;
        alpha = 160;
        green = red = 64;
        blue = 255;
        if ((work->scl.x += MARKER_VARS(work)->grow) < 0.01F) {
            task_kill(task);
            return;
        }
        break;
    }
    if (frame_step != 0) {
        return;
    }
    for (color = zero_marker_colors; color < &zero_marker_colors[5]; color++) {
        color->argb.a = alpha;
        color->argb.r = red;
        color->argb.g = green;
        color->argb.b = blue;
    }
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njPushMatrix(NULL);
    njTranslateV(NULL, &zero_lock_pos);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njScale(NULL, work->scl.x, work->scl.x, work->scl.x);
    njDrawTriangle3D(&zero_marker_faces, 3,
                     NJD_TRANSPARENT | NJD_DRAW_CONNECTED);
    njPopMatrix(1);
}

/* set zero_arm_end to the end of Zero's arm, as its action poses it */
static void zero_find_arm_end(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    NJS_POINT3 point;
    NJS_ACTION *action;

    njPushMatrix(&_unit_matrix_);
    pos.x = 0.0F;
    pos.y = 0.0F;
    pos.z = 0.0F;
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    if (zero_anim.state == PLAYER_ANIM_BLENDING) {
        action = zero_anim.blend;
    } else {
        action = zero_actions[zero_anim.action].action;
    }
    pose_action(NULL, action, zero_anim.frame, ARM_END_NODE, &zero_node_matrix);
    njCalcPoint(&zero_node_matrix, &pos, &point);
    zero_arm_end = point;
    njPopMatrix(1);
}

/* throw the hand from the arm's end at the lock, with its shape on and a
 * shadow task to follow it. it returns ZERO_HAND_OUT, which is 0. when the
 * lock lies at the arm's end, Zero keeps the hand and lets the lock go, and
 * zero_hand_launch returns ZERO_HAND_UNTHROWN */
static int zero_hand_launch(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    zero_find_arm_end(task);
    zero_hand_pos = zero_arm_end;
    zero_hand_speed.x = zero_lock_pos.x - zero_arm_end.x;
    zero_hand_speed.y = zero_lock_pos.y - zero_arm_end.y;
    zero_hand_speed.z = zero_lock_pos.z - zero_arm_end.z;
    if (njScalor(&zero_hand_speed) == 0.0F) {
        zero_locked = FALSE;
        return zero_hand_state = ZERO_HAND_UNTHROWN;
    } else {
        zero_hand_speed.x *= HAND_SPEED;
        zero_hand_speed.y *= HAND_SPEED;
        zero_hand_speed.z *= HAND_SPEED;
        zero_hand_ang_y =
            -(Angle)(atan2f(zero_hand_speed.z, zero_hand_speed.x) *
                     ANGLE_PER_RADIAN);
        zero_hand_pitch = 0;
        for (i = 0; i < CHAIN_POINTS; i++) {
            zero_chain[i] = zero_arm_end;
        }
        task_create(TASK_WORK, 6, zero_hand_shadow_step);
        work->collision->shapes[1].flags &= ~SHAPE_OFF;
        work->collision->shapes[1].center = zero_hand_pos;
        return zero_hand_state = ZERO_HAND_OUT;
    }
}

/* draw a link of the chain, stretched from from to to */
static void zero_draw_link(NJS_POINT3 *from, NJS_POINT3 *to)
{
    NJS_VECTOR span;
    Float length;
    Float across;
    Angle ang_y;
    Angle ang_z;

    span.x = to->x - from->x;
    span.y = to->y - from->y;
    span.z = to->z - from->z;
    length = njScalor(&span);
    across = njSqrt((span.x * span.x) + (span.z * span.z));
    ang_y = (Angle)(atan2f(span.z, -span.x) * ANGLE_PER_RADIAN);
    if (span.y < 0.0F) {
        if (across < 0.01F) {
            ang_z = NJM_DEG_ANG(90);
        } else {
            ang_z = (Angle)(acosf(across / length) * ANGLE_PER_RADIAN);
        }
    } else {
        if (across < 0.01F) {
            ang_z = NJM_DEG_ANG(270);
        } else {
            ang_z = -(Angle)(acosf(across / length) * ANGLE_PER_RADIAN);
        }
    }
    length *= 1.0F / LINK_LENGTH;
    njPushMatrix(NULL);
    njTranslateV(NULL, from);
    TURN(njRotateY, ang_y);
    TURN(njRotateZ, ang_z);
    njScale(NULL, length, 1.0F, 1.0F);
    njSetTexture(&zero_texlist);
    draw_tree(&zero_link_object);
    njPopMatrix(1);
}

/* draw the chain's links, from the hand back to the arm's end */
static void zero_draw_chain(void)
{
    NJS_POINT3 *point;
    NJS_POINT3 *next;
    int i;

    point = zero_chain;
    next = point + 1;
    for (i = CHAIN_POINTS; i > 1; i--, point++, next++) {
        zero_draw_link(point, next);
    }
    zero_draw_link(point, &zero_arm_end);
}

/* draw the flying hand, and the dust it trails. the hand and claws show
 * only for this draw, so Zero's own draw leaves them off its arm */
static void zero_draw_hand(void)
{
    NJS_OBJECT *objects[4];
    NJS_VECTOR dir;
    NJS_VECTOR speed;
    NJS_POINT3 pos;
    Float length;

    objects[0] = zero_hand_object;
    objects[1] = zero_claw1_object;
    objects[2] = zero_claw1_tip_object;
    objects[3] = zero_claw0_object;
    objects[0]->evalflags &= ~NJD_EVAL_HIDE;
    objects[1]->evalflags &= ~NJD_EVAL_HIDE;
    objects[2]->evalflags &= ~NJD_EVAL_HIDE;
    objects[3]->evalflags &= ~NJD_EVAL_HIDE;
    njPushMatrix(NULL);
    njTranslateV(NULL, &zero_hand_pos);
    TURN(njRotateY, zero_hand_ang_y);
    njRotateX(NULL, NJM_DEG_ANG(180));
    TURN(njRotateZ, zero_hand_pitch);
    njSetTexture(&zero_texlist);
    draw_tree(objects[3]);
    njPopMatrix(1);
    objects[0]->evalflags |= NJD_EVAL_HIDE;
    objects[1]->evalflags |= NJD_EVAL_HIDE;
    objects[2]->evalflags |= NJD_EVAL_HIDE;
    objects[3]->evalflags |= NJD_EVAL_HIDE;
    /* @hack redundant: speed zeroed and never read, which stores its zeros
     * at sp+36 to sp+44 (0x8c114be6 to 0x8c114bec); without it, the frame
     * shrinks from 52 bytes to 40 and 61 of 188 words differ */
    speed.x = speed.y = speed.z = 0.0F;
    dir = zero_hand_speed;
    if ((length = njScalor(&dir)) == 0.0F) {
        dir.x = dir.y = dir.z = 0.0F;
    } else {
        njUnitVector(&dir);
    }
    dir.x *= -3.0F;
    dir.y *= -3.0F;
    dir.z *= -3.0F;
    pos = zero_hand_pos;
    while (length > 0.0F) {
        dust_create(&pos, &dir, 1.0F);
        length -= 3.0F;
        njAddVector(&pos, &dir);
    }
}

/* push pos out to radius from the surfaces around it */
static void zero_keep_clear(NJS_POINT3 *pos, Float radius)
{
    struct sides_query hits;

    if (!zero_find_sides(&hits, pos, radius)) {
        return;
    }
    if (hits.sides[SIDE_MINUS_X].found == TRUE &&
        pos->x - hits.sides[SIDE_MINUS_X].level < radius) {
        pos->x = hits.sides[SIDE_MINUS_X].level + radius;
    }
    if (hits.sides[SIDE_PLUS_X].found == TRUE &&
        hits.sides[SIDE_PLUS_X].level - pos->x < radius) {
        pos->x = hits.sides[SIDE_PLUS_X].level - radius;
    }
    if (hits.sides[SIDE_MINUS_Z].found == TRUE &&
        pos->z - hits.sides[SIDE_MINUS_Z].level < radius) {
        pos->z = hits.sides[SIDE_MINUS_Z].level + radius;
    }
    if (hits.sides[SIDE_PLUS_Z].found == TRUE &&
        hits.sides[SIDE_PLUS_Z].level - pos->z < radius) {
        pos->z = hits.sides[SIDE_PLUS_Z].level - radius;
    }
    if (hits.sides[SIDE_BELOW].found == TRUE &&
        pos->y - hits.sides[SIDE_BELOW].level < radius) {
        pos->y = hits.sides[SIDE_BELOW].level + radius;
    }
    if (hits.sides[SIDE_ABOVE].found == TRUE &&
        hits.sides[SIDE_ABOVE].level - pos->y < radius) {
        pos->y = hits.sides[SIDE_ABOVE].level - radius;
    }
}

/* a step of the hand's flight: it turns toward the lock, keeps HAND_RADIUS
 * from the world, and drags the chain behind. it returns ZERO_HAND_BACK,
 * ZERO_HAND_MISSED or ZERO_HAND_CAUGHT as the flight ends, before the chain
 * and the hand are drawn. otherwise it draws them and returns
 * ZERO_HAND_FLYING */
static int zero_hand_fly(struct task *task)
{
    struct task_work *work = task->work;
    Float distance;
    NJS_VECTOR dir;
    NJS_VECTOR target;
    int i;

    zero_find_arm_end(task);
    dir = zero_hand_speed;
    if (njScalor(&dir) == 0.0F) {
        /* @hack spelling: dir through its address, formed again for each store
         * (MOV R15,R2 and ADD #28,R2 at 0x8c114e96); by name, 162 of 371
         * words differ */
        (&dir)->x = 1.0F;
        (&dir)->y = (&dir)->z = 0.0F;
    } else {
        njUnitVector(&dir);
    }
    target.x = zero_lock_pos.x - zero_hand_pos.x;
    target.y = zero_lock_pos.y - zero_hand_pos.y;
    target.z = zero_lock_pos.z - zero_hand_pos.z;
    if (njScalor(&target) == 0.0F) {
        target = dir;
    } else {
        njUnitVector(&target);
    }
    zero_hand_speed.x = (dir.x * 0.825F) + (target.x * 0.175F);
    zero_hand_speed.y = (dir.y * 0.825F) + (target.y * 0.175F);
    zero_hand_speed.z = (dir.z * 0.825F) + (target.z * 0.175F);
    if (njScalor(&zero_hand_speed) == 0.0F) {
        zero_hand_speed = target;
    } else {
        njUnitVector(&zero_hand_speed);
    }
    zero_hand_speed.x *= HAND_SPEED;
    zero_hand_speed.y *= HAND_SPEED;
    zero_hand_speed.z *= HAND_SPEED;
    njAddVector(&zero_hand_pos, &zero_hand_speed);
    zero_keep_clear(&zero_hand_pos, HAND_RADIUS);
    work->collision->shapes[1].center = zero_hand_pos;
    if (zero_distance2(&zero_arm_end, &zero_hand_pos) >
        HAND_REACH * HAND_REACH) {
        return zero_hand_state = ZERO_HAND_BACK;
    }
    if ((distance = njDistanceP2P(&zero_arm_end, &zero_hand_pos)) > 20.0F &&
        distance > njDistanceP2P(&zero_arm_end, &zero_lock_pos)) {
        return zero_hand_state = ZERO_HAND_MISSED;
    }
    if (zero_amy_distance2(&zero_hand_pos) < 5.0F * 5.0F) {
        return zero_hand_state = ZERO_HAND_CAUGHT;
    }
    /* @hack redundant: zero_chain read and dropped, which loads its address
     * ahead of the 0xFF that i - 1 adds (MOV.L at 0x8c115062, MOV.W at
     * 0x8c11506c); without it, the two loads trade places and 2 of 371 words
     * differ */
    (void)zero_chain[0].x;
    for (i = CHAIN_POINTS - 1; i > 0; i--) {
        zero_chain[i].x =
            (zero_chain[i - 1].x * 0.85F) + (zero_arm_end.x * 0.15F);
        zero_chain[i].y =
            (zero_chain[i - 1].y * 0.85F) + (zero_arm_end.y * 0.15F);
        zero_chain[i].z =
            (zero_chain[i - 1].z * 0.85F) + (zero_arm_end.z * 0.15F);
    }
    zero_chain[0] = zero_hand_pos;
    zero_draw_chain();
    zero_hand_ang_y = -(Angle)(atan2f(zero_hand_speed.z, zero_hand_speed.x) *
                               ANGLE_PER_RADIAN);
    zero_hand_pitch =
        (Angle)(atan2f(-zero_hand_speed.y,
                       njSqrt((zero_hand_speed.x * zero_hand_speed.x) +
                              (zero_hand_speed.z * zero_hand_speed.z))) *
                ANGLE_PER_RADIAN);
    zero_draw_hand();
    return zero_hand_state = ZERO_HAND_FLYING;
}

/* a step of the hand's way back to the arm's end: at HAND_SPEED, then at
 * HAND_HOMING_SPEED within HAND_HOMING_RANGE. returns ZERO_HAND_BACK, or
 * ZERO_HAND_HOME once within 1.5 of the arm's end or out of time. a hand
 * that is home shows on the arm again, with its shape off */
static int zero_hand_return(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR dir;
    NJS_VECTOR target;
    NJS_OBJECT *objects[4];
    Float distance;
    int i;

    objects[0] = zero_hand_object;
    objects[1] = zero_claw1_object;
    objects[2] = zero_claw1_tip_object;
    objects[3] = zero_claw0_object;
    zero_find_arm_end(task);
    distance = zero_distance2(&zero_arm_end, &zero_hand_pos);
    dir = zero_hand_speed;
    if (njScalor(&dir) == 0.0F) {
        /* @hack spelling: dir through its address, formed again for each store
         * (MOV R15,R2 and ADD #44,R2 at 0x8c11526e); by name, 111 of 397
         * words differ */
        (&dir)->x = 1.0F;
        (&dir)->y = (&dir)->z = 0.0F;
    } else {
        njUnitVector(&dir);
    }
    target.x = zero_arm_end.x - zero_hand_pos.x;
    target.y = zero_arm_end.y - zero_hand_pos.y;
    target.z = zero_arm_end.z - zero_hand_pos.z;
    if (njScalor(&target) == 0.0F) {
        target = dir;
    } else {
        njUnitVector(&target);
    }
    zero_hand_speed.x = (dir.x * 0.825F) + (target.x * 0.175F);
    zero_hand_speed.y = (dir.y * 0.825F) + (target.y * 0.175F);
    zero_hand_speed.z = (dir.z * 0.825F) + (target.z * 0.175F);
    if (njScalor(&zero_hand_speed) == 0.0F) {
        zero_hand_speed = target;
    } else {
        njUnitVector(&zero_hand_speed);
    }
    if (distance > HAND_HOMING_RANGE * HAND_HOMING_RANGE) {
        zero_hand_speed.x *= HAND_SPEED;
        zero_hand_speed.y *= HAND_SPEED;
        zero_hand_speed.z *= HAND_SPEED;
    } else {
        zero_hand_speed.x *= HAND_HOMING_SPEED;
        zero_hand_speed.y *= HAND_HOMING_SPEED;
        zero_hand_speed.z *= HAND_HOMING_SPEED;
    }
    njAddVector(&zero_hand_pos, &zero_hand_speed);
    zero_keep_clear(&zero_hand_pos, HAND_RADIUS);
    work->collision->shapes[1].center = zero_hand_pos;
    if (--VARS(work)->timer <= 0 || distance < 1.5F * 1.5F) {
        objects[0]->evalflags &= ~NJD_EVAL_HIDE;
        objects[1]->evalflags &= ~NJD_EVAL_HIDE;
        objects[2]->evalflags &= ~NJD_EVAL_HIDE;
        objects[3]->evalflags &= ~NJD_EVAL_HIDE;
        work->collision->shapes[1].flags |= SHAPE_OFF;
        sound_stop_id((int)work);
        return zero_hand_state = ZERO_HAND_HOME;
    }
    for (i = CHAIN_POINTS - 1; i > 0; i--) {
        zero_chain[i].x =
            (zero_chain[i - 1].x * 0.85F) + (zero_arm_end.x * 0.15F);
        zero_chain[i].y =
            (zero_chain[i - 1].y * 0.85F) + (zero_arm_end.y * 0.15F);
        zero_chain[i].z =
            (zero_chain[i - 1].z * 0.85F) + (zero_arm_end.z * 0.15F);
    }
    zero_chain[0] = zero_hand_pos;
    zero_draw_chain();
    if (distance < HAND_HOMING_RANGE * HAND_HOMING_RANGE) {
        zero_hand_ang_y = angle_approach(zero_hand_ang_y,
                                         work->ang.y + NJM_DEG_ANG(180), 0x400);
        zero_hand_pitch = angle_approach(zero_hand_pitch, 0, 0x400);
    }
    zero_draw_hand();
    return zero_hand_state = ZERO_HAND_BACK;
}

/* draw the hand's shadow 10 ahead of it, until the hand is home. a floor
 * there less than HAND_LIFT under the hand lifts it to HAND_LIFT above,
 * and stops its fall */
static void zero_hand_shadow_step(struct task *task)
{
    NJS_POINT3 pos;
    Float ground;
    Float bottom;

    if (zero_hand_state == ZERO_HAND_HOME) {
        task_kill(task);
        return;
    }
    pos = zero_hand_pos;
    pos.x += njCos(zero_hand_ang_y) * 10.0F;
    pos.z -= njSin(zero_hand_ang_y) * 10.0F;
    task->work->pos = pos;
    ground = shadow_draw(task->work, 1.0F);
    bottom = zero_hand_pos.y + -HAND_LIFT;
    if (ground > bottom && ground - bottom < HAND_LIFT) {
        zero_hand_pos.y = ground + HAND_LIFT;
        zero_hand_speed.y = 0.0F;
    }
}

static void zero_hirame_step(struct task *task)
{
    struct task_work *work = task->work;

    if (++TASK_TIMER(work) >= 60) {
        task_kill(task);
        return;
    }
    if (MARK_VARS(work)->follow != NULL) {
        work->pos = MARK_VARS(work)->follow->pos;
        work->pos.y += MARK_HEIGHT;
    }
    if (frame_step != 0) {
        return;
    }
    if (++MARK_VARS(work)->frame >= 3) {
        MARK_VARS(work)->frame = 0;
    }
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njSetTexture(&zero_texlist);
    njDrawSprite3D(&zero_hirame_sprite, MARK_VARS(work)->frame,
                   NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
    njPopMatrix(1);
}

static void zero_hatena_step(struct task *task)
{
    struct task_work *work = task->work;

    if (++TASK_TIMER(work) >= 60) {
        task_kill(task);
        return;
    }
    if (frame_step != 0) {
        return;
    }
    if (++MARK_VARS(work)->frame >= 3) {
        MARK_VARS(work)->frame = 0;
    }
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njSetTexture(&zero_texlist);
    njDrawSprite3D(&zero_hatena_sprite, MARK_VARS(work)->frame,
                   NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
    njPopMatrix(1);
}

/* a shock wave. its radius starts at 1 and grows by 99/35 a step. the wave
 * ends on its 35th step, before it grows, at a radius of about 97. its
 * sphere rides the edge nearest Amy */
static void zero_shock_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    NJS_VECTOR edge;

    if (work->mode == SHOCK_START) {
        work->scl.x = 1.0F;
        work->scl.y = 198.0F / 35.0F;
        collision_init(task, zero_shock_shape, COLLISION_OBJECT, 1, 1);
        work->collision->radius = 105.0F;
        work->mode = SHOCK_SPREAD;
    }
    if (++SHOCK_VARS(work)->steps >= 35) {
        task_kill(task);
        return;
    }
    work->scl.x += 99.0F / 35.0F;
    work->scl.z = work->scl.x;
    if (frame_step == 0) {
        work->ang.y += NJM_DEG_ANG(22.5);
        /* @hack spelling: the model through a local pointer, which keeps its
         * address in R13 (MOV.L at 0x8c1158de); by name, R4 takes it, the
         * function runs 2 bytes longer and 118 of its 238 words differ */
        object = &zero_shock_object;
        if (++work->tick & 1) {
            object->model->mats->attr_texId = 21;
        } else {
            object->model->mats->attr_texId = 22;
        }
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        edge.x = work->scl.x * 0.2F;
        edge.y = 1.0F;
        edge.z = edge.x;
        njScaleV(NULL, &edge);
        njSetTexture(&zero_texlist);
        draw_tree(object);
        njPopMatrix(1);
    }
    edge.x = player_work[0]->pos.x - work->pos.x;
    edge.y = 0.0F;
    edge.z = player_work[0]->pos.z - work->pos.z;
    if (njScalor(&edge) == 0.0F) {
        /* @hack spelling: edge through its address, formed again for each store
         * (MOV R15,R2 and ADD #4,R2 at 0x8c1159d2); by name, the function
         * runs 12 bytes short and 76 of its 238 words differ */
        (&edge)->x = 1.0F;
        (&edge)->y = (&edge)->z = 0.0F;
    } else {
        njUnitVector(&edge);
    }
    edge.x *= work->scl.x;
    edge.z *= work->scl.x;
    edge.x += work->pos.x;
    edge.y = work->pos.y;
    edge.z += work->pos.z;
    work->collision->shapes[0].center = edge;
    collision_register(work);
    vibration_start(0, 1, 15, 0);
}

/* an event's throw, run as zero_step runs a throw but without its sounds
 * or LOCK_RANGE. once the hand is home, Zero goes back to
 * zero_event_step */
static void zero_event_grab_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case ZERO_WIND_UP:
        if (zero_anim.action == ZERO_ACT_THROW &&
            zero_anim.frame >= THROW_FRAME) {
            if (zero_locked == TRUE && !zero_hand_launch(task)) {
                VARS(work)->timer = 60;
                work->mode = ZERO_THROW;
                /* @hack redundant: a break ahead of the case's, which has
                 * ZERO_THROW count its timer down in R3 (0x8c115b40); without
                 * it, R1 takes it and 15 of 115 words differ */
                break;
            }
        }
        break;
    case ZERO_THROW:
        if (--VARS(work)->timer <= 0) {
            zero_locked = FALSE;
            VARS(work)->timer = 180;
            work->mode = ZERO_REEL;
            break;
        }
        switch (zero_hand_fly(task)) {
        /* @hack redundant: a case for ZERO_HAND_FLYING that does nothing, which
         * puts a test of its own in the compare chain (CMP/EQ #3,R0 at
         * 0x8c115b5e); without it, 20 of 115 words differ */
        case ZERO_HAND_FLYING:
            break;
        case ZERO_HAND_UNTHROWN:
        case ZERO_HAND_CAUGHT:
        case ZERO_HAND_MISSED:
        case ZERO_HAND_BACK:
            zero_locked = FALSE;
            VARS(work)->timer = 180;
            work->mode = ZERO_REEL;
        }
        break;
    case ZERO_REEL:
        if (zero_hand_return(task) == ZERO_HAND_HOME) {
            work->mode = ZERO_FALL;
            zero_anim.action = ZERO_ACT_STAND;
            task->step = zero_event_step;
        }
        break;
    }
    player_anim_step(&zero_anim);
    ALPHA(work) = 1.0F;
    zero_draw(task);
    shadow_draw(work, 1.8F);
}

/* start the throw zero_event_grab asked for: lock on to its point, show
 * the marker, and wind up */
static void zero_event_grab_start(struct task *task)
{
    struct task_work *work = task->work;

    zero_locked = TRUE;
    zero_lock_pos = zero_event_grab_pos;
    task_create(TASK_WORK, 6, zero_marker_step);
    VARS(work)->timer = 180;
    zero_anim.action = ZERO_ACT_WIND_UP;
    work->mode = ZERO_WIND_UP;
    player_anim_step(&zero_anim);
    ALPHA(work) = 1.0F;
    zero_draw(task);
    shadow_draw(work, 1.8F);
    task->step = zero_event_grab_step;
}

Bool zero_event_grab(NJS_POINT3 *pos)
{
    /* @hack spelling: a switch of one case, which tests the state with CMP/EQ
     * #0,R0 (0x8c115c84); as an if, SHC tests it with TST and 2 of 18 words
     * differ */
    switch (zero_event_grab_state) {
    case ZERO_GRAB_NONE:
        zero_event_grab_state = ZERO_GRAB_ASKED;
        zero_event_grab_pos = *pos;
        return TRUE;
    }
    return FALSE;
}

void zero_show_hatena(NJS_POINT3 *pos)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 6, zero_hatena_step)) != NULL) {
        task->work->pos = *pos;
    }
}

void zero_show_hirame(NJS_POINT3 *pos)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 6, zero_hirame_step)) != NULL) {
        task->work->pos = *pos;
        MARK_VARS(task->work)->follow = NULL;
    }
}

void zero_event_init(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, zero_shapes, COLLISION_ITEM, 1, 2);
    event_alloc(work);
    player_init(VARS(work)->player, task);
    texture_load_pvm("EGGROB", &zero_texlist);
    SPEED(work).z = 0.0F;
    SPEED(work).y = 0.0F;
    SPEED(work).x = 0.0F;
    zero_damage = 0.0F;
    zero_anim.state = PLAYER_ANIM_NEW;
    zero_anim.actions = zero_actions;
    zero_anim.action = zero_anim.current = ZERO_ACT_STAND;
    zero_anim.pace = &zero_hit_pace;
    zero_neck_object[0].ang[1] = 0;
    zero_head_object[0].ang[2] = NJM_DEG_ANG(180);
    zero_sight_out = FALSE;
    zero_sight_saw = FALSE;
    work->collision->shapes[1].flags |= SHAPE_OFF;
    zero_restless = 0.0F;
    VARS(work)->timer = 180;
    work->mode = ZERO_FALL;
    zero_beam_reset();
    zero_event_grab_state = ZERO_GRAB_NONE;
    task->destructor = zero_event_destroy;
    task->step = zero_event_step;
}

static void zero_event_destroy(struct task *task)
{
    zero_neck_object[0].ang[1] = 0;
    zero_head_object[0].ang[2] = NJM_DEG_ANG(180);
    zero_hand_object[0].evalflags &= ~NJD_EVAL_HIDE;
    zero_claw1_object[0].evalflags &= ~NJD_EVAL_HIDE;
    zero_claw1_tip_object[0].evalflags &= ~NJD_EVAL_HIDE;
    zero_claw0_object[0].evalflags &= ~NJD_EVAL_HIDE;
    player_free(VARS(task->work)->player);
}

/* a step of an event's Zero: it falls, rests, or follows the event's path,
 * look or move. a throw zero_event_grab asked for starts on the next
 * step */
static void zero_event_step(struct task *task)
{
    struct task_work *work = task->work;
    struct event_work *event = work->event;
    NJS_POINT3 delta;
    Angle heading;
    Float distance;
    Float angle_scale;
    Float speed;
    Float accel;
    struct task *target;

    if (event == NULL) {
        task_kill(task);
        return;
    }
    if (zero_event_grab_state == ZERO_GRAB_ASKED) {
        task->step = zero_event_grab_start;
        zero_event_grab_state = ZERO_GRAB_STARTED;
        return;
    }
    /* @hack spelling: the event read through work->event again, not event
     * (MOV.L @(60,R14),R3 at 0x8c115ede); through event, SHC reads R13 and
     * 70 of 493 words differ */
    if (work->event->paths != NULL) {
        work->mode = ZERO_EVENT;
    } else if (event->look != EVENT_LOOK_NONE) {
        switch (event->look) {
        case EVENT_LOOK_ANGLES:
        case EVENT_LOOK_POINT:
        case EVENT_LOOK_TASK:
            work->mode = ZERO_EVENT;
            break;
        /* @hack redundant: a case for EVENT_LOOK_NONE, which the else if's test
         * rules out, and which puts a test of its own in the compare chain
         * (CMP/EQ #0,R0 at 0x8c115ef6); without it, 60 of 493 words differ */
        case EVENT_LOOK_NONE:
        default:
            work->mode = ZERO_FALL;
            break;
        }
    } else {
        switch (event->action) {
        case EVENT_ACTION_RUN:
        case EVENT_ACTION_MOVE:
        case EVENT_ACTION_PUSH:
        case EVENT_ACTION_TURN_BY:
        case EVENT_ACTION_TURN_TO:
            work->mode = ZERO_EVENT;
            break;
        /* @hack redundant: cases for EVENT_ACTION_NONE and EVENT_ACTION_DONE
         * beside the default, which put tests of their own in the compare
         * chain (CMP/EQ #0,R0 at 0x8c115f18, CMP/EQ #1,R0 at 0x8c115f1c);
         * without them, 43 of 493 words differ */
        case EVENT_ACTION_NONE:
        case EVENT_ACTION_DONE:
        default:
            work->mode = ZERO_FALL;
            break;
        }
        if (event->action != EVENT_ACTION_TURN_BY &&
            event->action != EVENT_ACTION_TURN_TO) {
            TURN_PHASE(work) = PHASE_AIM;
        }
    }

    switch (work->mode) {
    case ZERO_FALL:
        zero_fall(task);
        zero_move(task);
        zero_collide(task);
        if ((work->flags & ZERO_ON_GROUND) && njAbs(SPEED(work).y) < 0.1F) {
            work->mode = ZERO_STILL;
        }
        break;
    case ZERO_STILL:
        SPEED(work).z = 0.0F;
        SPEED(work).y = 0.0F;
        SPEED(work).x = 0.0F;
        break;
    case ZERO_EVENT:
        /* @hack spelling: the event read through work->event again, not event
         * (MOV.L @(60,R14),R2 at 0x8c115fdc); through event, SHC reads R13
         * and 10 of 493 words differ */
        if (work->event->paths != NULL) {
            event_paths_step(work);
            break;
        }
        /* @hack spelling: ANGLE_PER_RADIAN held in a local, whose pool word
         * then sits before atan2f's (MOVA at 0x8c116020); used directly, the
         * two trade places and 2 of 493 words differ */
        angle_scale = ANGLE_PER_RADIAN;
        if (event->look != EVENT_LOOK_NONE) {
            switch (event->look) {
            case EVENT_LOOK_ANGLES:
                work->ang.y =
                    angle_approach(work->ang.y, event->look_ang.y, 0x400);
                break;
            case EVENT_LOOK_POINT:
                delta = event->look_pos;
                delta.y = 0.0F;
                njSubVector(&delta, &work->pos);
                work->ang.y = angle_approach(
                    work->ang.y,
                    (Angle)(atan2f(delta.z, -delta.x) * angle_scale), 0x400);
                break;
            case EVENT_LOOK_TASK:
                target = event->look_task;
                if (target == NULL || target->work == NULL) {
                    break;
                }
                delta = target->work->pos;
                delta.y = 0.0F;
                njSubVector(&delta, &work->pos);
                work->ang.y = angle_approach(
                    work->ang.y,
                    (Angle)(atan2f(delta.z, -delta.x) * angle_scale), 0x400);
                break;
            }
            break;
        }
        switch (event->action) {
        case EVENT_ACTION_RUN:
            delta = event->move.pos;
            njSubVector(&delta, &work->pos);
            delta.y = 0.0F;
            if (njScalor(&delta) < 1.0F + (njAbs(SPEED(work).x) * 0.5F)) {
                break;
            }
            work->ang.y = angle_approach(
                work->ang.y, (Angle)(atan2f(delta.z, -delta.x) * angle_scale),
                0x400);
            speed = event->speed;
            accel = event->accel;
            if (speed == 0.0F && accel == 0.0F) {
                if ((SPEED(work).x += 0.04F) > 1.25F) {
                    SPEED(work).x = 1.25F;
                }
            } else {
                if (speed == 0.0F) {
                    speed = SPEED(work).x;
                }
                SPEED(work).x = speed + accel;
            }
            zero_fall(task);
            zero_move(task);
            zero_collide(task);
            break;
        case EVENT_ACTION_MOVE:
            delta = event->move.pos;
            delta.y = 0.0F;
            if ((distance = njScalor(&delta)) < 0.01F) {
                break;
            }
            work->ang.y = angle_approach(
                work->ang.y, (Angle)(atan2f(delta.z, -delta.x) * angle_scale),
                0x400);
            if (SPEED(work).x + 0.04F < distance) {
                SPEED(work).x += 0.04F;
            } else if (SPEED(work).x + -0.1F > distance) {
                SPEED(work).x += -0.1F;
            } else {
                SPEED(work).x = distance;
            }
            zero_fall(task);
            zero_move(task);
            zero_collide(task);
            break;
        /* @hack redundant: a case for EVENT_ACTION_PUSH that does nothing,
         * whose test and BRA run from 0x8c116126 to 0x8c11612a; without it,
         * zero_event_step runs 32 bytes short and 259 of its 493 words
         * differ */
        case EVENT_ACTION_PUSH:
            break;
        case EVENT_ACTION_TURN_BY:
        case EVENT_ACTION_TURN_TO:
            switch (TURN_PHASE(work)) {
            case PHASE_AIM:
                if (event->action == EVENT_ACTION_TURN_BY) {
                    HEAD_AIM(work) = event->move.ang.y;
                } else {
                    HEAD_AIM(work) =
                        angle_difference(work->ang.y, event->move.ang.y);
                }
                TURN_PHASE(work) = PHASE_HEAD;
                break;
            case PHASE_HEAD:
                HEAD_TURN(work) =
                    angle_approach(HEAD_TURN(work), HEAD_AIM(work), 0x800);
                zero_neck_object[0].ang[1] = HEAD_TURN(work);
                if (HEAD_TURN(work) == HEAD_AIM(work)) {
                    TURN_PHASE(work) = PHASE_BODY;
                }
                break;
            case PHASE_BODY:
                heading = angle_approach(HEAD_TURN(work), 0, 0x800);
                work->ang.y += angle_difference(heading, HEAD_TURN(work));
                HEAD_TURN(work) = heading;
                zero_neck_object[0].ang[1] = HEAD_TURN(work);
                if (HEAD_TURN(work) == 0) {
                    TURN_PHASE(work) = PHASE_DONE;
                    /* @hack redundant: a break ahead of the case's, which loads
                     * collision_register's address into R3 below
                     * (0x8c11634a); without it, R2 takes it and 4 of 493
                     * words differ */
                    break;
                }
                break;
            }
            break;
        }
        break;
    }
    collision_register(work);
    player_anim_step(&zero_anim);
    ALPHA(work) = 1.0F;
    zero_draw(task);
    shadow_draw(work, 1.8F);
}

/* Zero's fall: its upward speed drops by 0.1 a step, down to -3 */
static void zero_fall(struct task *task)
{
    if ((SPEED(task->work).y += -0.1F) < -3.0F) {
        SPEED(task->work).y = -3.0F;
    }
}

/* move Zero by its speed, along its heading and up, and by any push
 * zero_push asked for. first, each part of its speed under 0.01 either way
 * drops to 0 */
static void zero_move(struct task *task)
{
    struct task_work *work = task->work;

    if (njAbs(SPEED(work).x) < 0.01F) {
        SPEED(work).x = 0.0F;
    }
    if (njAbs(SPEED(work).y) < 0.01F) {
        SPEED(work).y = 0.0F;
    }
    if (njAbs(SPEED(work).z) < 0.01F) {
        SPEED(work).z = 0.0F;
    }
    work->pos.x -= SPEED(work).x * njCos(work->ang.y);
    work->pos.z += SPEED(work).x * njSin(work->ang.y);
    work->pos.y += SPEED(work).y;
    njAddVector(&work->pos, &zero_request.push);
}
