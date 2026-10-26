/* Chaos 0, the boss: his task, his modes and his draw */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos0/boss.h"
#include "b_chaos0/chaos0.h"
#include "b_chaos0/hint.h"
#include "b_chaos0/land.h"
#include "b_chaos0/textures.h"
#include "b_chaos0/wobble.h"
#include "camera/camera.h"
#include "chaos/chaos.h"
#include "chaos/deform.h"
#include "chaos/effect.h"
#include "chaos/puddle.h"
#include "chaos/ribbon.h"
#include "chaos/steer.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "draw/motion.h"
#include "draw/tree.h"
#include "effect/brain.h"
#include "event/play.h"
#include "hud/boss_name.h"
#include "hud/hud.h"
#include "hud/life_gauge.h"
#include "player/player.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 7
#include "b_chaos0/boss/text.inc"

extern struct weld chaos0_welds[];
extern struct ribbon chaos0_ribbon;
/* his body's tree, and the node in it that chaos_effect_pulse pulses */
extern NJS_OBJECT chaos0_body_object;
extern NJS_OBJECT chaos0_pulse_node;
/* the punching arm's node, its hand and the hand's sibling, then the other
 * arm's three. each of the other arm's nodes mirrors the punching arm's in
 * the same place */
extern NJS_OBJECT chaos0_punch_arm;
extern NJS_OBJECT chaos0_punch_hand;
extern NJS_OBJECT dat_0c92139c;
extern NJS_OBJECT chaos0_other_arm;
extern NJS_OBJECT chaos0_other_hand;
extern NJS_OBJECT dat_0c92046c;

/* the rest of the punching arm's nodes that the stretched arm hides */
extern NJS_OBJECT dat_0c920a40;
extern NJS_OBJECT dat_0c920c30;
extern NJS_OBJECT dat_0c92158c;
extern NJS_OBJECT dat_0c920c64;

/* chaos0_setup_nodes fades the nodes of chaos0_alpha_nodes, and hides
 * those of dat_0c95e728, which holds none. each list ends at a null */
extern NJS_OBJECT *chaos0_alpha_nodes[];
extern NJS_OBJECT *dat_0c95e728[];
/* the poles he jumps between */
extern NJS_POINT3 chaos0_poles[];

#define POLES 8

/* a bit of a land entry's flags, beside world.h's. land_show_low hides the
 * entries that have it while his camera is CAMERA_MODE_CHAOS_P, and draws
 * them otherwise */
#define LAND_LOW 0x2000

/* the field of view chaos0_begin sets for the fight */
#define BOSS_PERSPECTIVE NJM_DEG_ANG(70)

#define SE_WIND_UP 0x14d
#define SE_STRIKE 0x14e
#define SE_LEAP 0x151
#define SE_HIT 0x153
#define SE_SPLASH 0x154
#define SE_MELT 0x155
#define SE_WHIRL 0x2bc /* it plays through his spin */
#define SE_SLOSH 0x2cd
/* his step plays SE_NEAR while player 0 is nearer than NEAR_RANGE to him.
 * it plays SE_HIGH and SE_LOW at two fixed places, one 167 up and one on
 * the ground */
#define SE_NEAR 1000
#define SE_HIGH 1002
#define SE_LOW 1004
#define NEAR_RANGE 60.0F

/* the speed his jump and his throw gain downward each step */
#define GRAVITY 0.027222222F

/* ground_below looks for the ground from GROUND_PROBE above him */
#define GROUND_PROBE 4.0F

/* how far his walk and his spin move him a step */
#define WALK_SPEED 0.6F
/* his walk falls off where the ground drops more than STEP_HEIGHT below
 * him. it climbs where the ground LOOK_AHEAD ahead rises more than
 * STEP_HEIGHT above him, in at most CLIMB_STEPS steps */
#define STEP_HEIGHT 1.0F
#define LOOK_AHEAD 5.0F
#define CLIMB_STEPS 6
/* the speed his walk's fall gains downward each step */
#define WALK_FALL 0.1F
/* his walk splashes when the low byte of his record's steps is 0, once
 * every 256 steps */
#define SPLASH_MASK 0xff
/* his walk sloshes as walk_sound_wait passes SLOSH_WAIT, every 31 steps */
#define SLOSH_WAIT 30

/* the steps between the punch's wind-up and its strike, on the ground and
 * up on a pole. he aims while more than AIM_UNTIL of them are left. SHC
 * folds AIM_UNTIL to 0x40bfffff, a bit under 6.0F */
#define STRIKE_WAIT 30
#define POLE_STRIKE_WAIT 15
#define AIM_UNTIL (30.0F * 0.2F)
/* how far his hand moves a step as his arm stretches out or back. up on a
 * pole, it moves POLE_STRETCH times as far */
#define STRETCH_SPEED (35.0F / 12.0F)
#define POLE_STRETCH 1.5F

/* his figure turns SPIN_START a step as the spin starts, and SPIN_TOP at
 * its fastest. the turn grows by SPIN_GAIN a step to the top, and shrinks
 * by it as the spin slows. at its fastest, he chases player 0 for
 * SPIN_STEPS steps */
#define SPIN_START 1820
#define SPIN_TOP 2730
#define SPIN_GAIN 18
#define SPIN_STEPS 120
/* the motion's pace a step adds or takes as his arms spread or close */
#define SPREAD_STEP 0.04F
/* at full spread, his arms stretch to 1 + ARM_SPREAD times their length.
 * SHAPE_AROUND's radius is SPIN_RADIUS times their stretch */
#define ARM_SPREAD 3.5F
#define SPIN_RADIUS 10.0F
/* the most he rises or sinks a step toward the ground as he spins */
#define SPIN_CLIMB 0.15F

/* aim_across aims this far from the fight's center */
#define ACROSS_RADIUS 60.0F
/* a pole jump lands POLE_TOP above the pole's point in chaos0_poles. its
 * motion turns to ACTION_LAND_POLE POLE_LAND_LEAD steps before it lands */
#define POLE_TOP 65.0F
#define POLE_LAND_LEAD 20
/* a jump to the ground turns to ACTION_LAND within LAND_LEAD above it */
#define LAND_LEAD 10.0F

/* the punches a pole gives him, and how near across player 0 must come to
 * draw one */
#define POLE_PUNCHES 5
#define POLE_REACH 80.0F

/* a hit takes HIT_STEPS steps, and a melt or a rise MELT_STEPS */
#define HIT_STEPS 90.0F
#define MELT_STEPS 180.0F
/* the puddle's size in each axis as a hit or a melt ends */
#define PUDDLE_SIZE 1.4F
/* the height of his body's lower sphere's center, as chaos0_shapes gives
 * it. the sphere sinks as he melts and rises as he forms */
#define LOWER_HEIGHT 7.5F

/* BOSS_FLOW_ACROSS turns to BOSS_FORM within ACROSS_NEAR of its target.
 * BOSS_FLOW_AFTER turns to it within AFTER_NEAR of its own, or once it has
 * run more than AFTER_STEPS steps */
#define ACROSS_NEAR 10.6F
#define AFTER_NEAR 2.4F
#define AFTER_STEPS 240

/* the barrier stays up for BARRIER_HOLD steps from its last throw, the
 * throw's own step counted. it stays up past them while player 0 is curled
 * up within BARRIER_NEAR. its level rises by LEVEL_STEP a step to
 * BARRIER_LEVEL, and falls back by LEVEL_STEP as it goes down */
#define BARRIER_HOLD 11
#define BARRIER_NEAR 50.0F
#define BARRIER_LEVEL 0.5F
#define LEVEL_STEP 0.125F

/* his fall's throw moves him THROW_SPEED a step across, for
 * THROW_STEPS - 1 steps */
#define THROW_SPEED 2.0F
#define THROW_STEPS 10

/* room for his body's 72 nodes and the null, and for the stretched arm's
 * 10 and the null */
#define BODY_LIST 73
#define STRETCH_LIST 20

/* COLLISION_REACH is how far his shapes reach, as his collision record
 * gives it. LIGHT_RANGE is his cyan light's range, and SHADOW_SIZE his
 * shadow's size */
#define COLLISION_REACH 300.0F
#define LIGHT_RANGE 25.0F
#define SHADOW_SIZE 0.8F
/* while CHAOS0_CAN_ATTACK is set, a spot marks his way each time
 * chaos_travel passes SPOT_TRAVEL. each spot's alpha starts at SPOT_ALPHA */
#define SPOT_TRAVEL 5.0F
#define SPOT_ALPHA 0.8F
/* more than HIGH_ABOVE above player 0, he takes CAMERA_MODE_CHAOS_P */
#define HIGH_ABOVE 60.0F

/* the boss music's wait counts down from MUSIC_DELAY */
#define MUSIC_DELAY 60

/* the puddle fades in as its height grows to PUDDLE_FADE. he fades out as a
 * melt's pace passes MELT_FADE */
#define PUDDLE_FADE 0.2F
#define MELT_FADE 0.7F

static void chaos0_boss_step(struct task *task);
static void chaos0_draw(struct task *task);
static void land_show_low(int shown);

/* where the brain sits from the point it follows, in the camera's frame */
#pragma section HEAD
static NJS_POINT3 brain_offset = {0.0F, 1.0F, 5.0F};
#pragma section

static const Uint8 shape_count = BOSS_SHAPES;
/* how far across player 0 can go from the tether's center */
static const Float tether_radius = 90.0F;
/* the push that throws player 0 off the barrier, back and up in the frame
 * player 0 faces */
static const NJS_VECTOR barrier_push = {-1.2F, 1.2F, 0.0F};

/* the nodes that hide while the stretched arm shows */
static NJS_OBJECT *stretch_hides[] = {
    &dat_0c920a40, &dat_0c920c30, &dat_0c92158c, &chaos0_punch_hand,
    &dat_0c92139c, &dat_0c920c64, NULL,
};
/* two spheres of his body, a sphere round him that only guards, the
 * stretched hand, and a cylinder round him that his modes resize. all but
 * the guard hurt the players */
/* clang-format off */
static struct collision_shape chaos0_shapes[BOSS_SHAPES] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_HURTS_PLAYERS, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 15.0F, 0.0F}, 4.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1) | GUARD(2) | SHAPE_HURTS_PLAYERS, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 7.5F, 0.0F}, 5.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     GUARD(1), PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 12.5F, 0.0F}, 13.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(2) | GUARD(1) | SHAPE_HURTS_PLAYERS, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F}, 5.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE,
     ATTACK(2) | GUARD(1) | SHAPE_HURTS_PLAYERS, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F}, 0.0F, 0.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */
/* six words nothing reads. the last two hold 15.0F and 30.0F as floats */
static Uint32 dat_0c95f508[6] = {0, 0, 0, 0, 0x41700000, 0x41f00000};
/* the camera's offset from what it follows in CAMERA_MODE_CHAOS_P */
static NJS_POINT3 high_camera_offset = {0.0F, 15.0F, 20.0F};
/* the one point chaos_track_points finds for boss_points, on node 13 of
 * his pose, under the node that pulses. the brain follows it */
static struct boss_mark chaos0_marks[] = {
    {13, 0, {0.0F, 0.0F, 0.0F}},
    {-1, 0, {0.0F, 0.0F, 0.0F}},
};
/* the tether's center, and the tether, which his destructor kills */
static NJS_POINT3 tether_center = {270.0F, 13.0F, 315.0F};
static struct task *tether = NULL;
/* the arms' six nodes and the stretched arm's three, whose scale counts once
 * he sets up */
static NJS_OBJECT *scaled_nodes[] = {
    &chaos0_punch_arm,   &chaos0_punch_hand,
    &dat_0c92139c,       &chaos0_other_arm,
    &chaos0_other_hand,  &dat_0c92046c,
    &chaos0_stretch_arm, &dat_0c9262bc,
    &dat_0c926514,       NULL,
};
/* how high over its start each jump peaks, by his vars' jump */
static Float jump_heights[5] = {30.0F, 10.0F, 140.0F, 20.0F, 30.0F};

/* his nodes, listed as he sets up, and freed by his destructor */
static NJS_OBJECT **body_nodes;
/* bytes nothing reads or writes */
static Uint8 dat_0c95fdec[24];
/* the wobble's task, which his destructor destroys */
static struct task *wobble_task;
static struct deform *body_deform;
/* the steps since the barrier last threw player 0 off, the throw's own
 * step counted, capped at BARRIER_HOLD */
static Sint16 barrier_steps;
/* the barrier's level, which rises as it goes up and falls as it goes
 * down. nothing but the barrier's timing reads it */
static Float barrier_level;
/* a byte nothing reads or writes */
static Uint8 dat_0c95fe14;
/* set while his step picks the camera's mode by his height over player 0 */
static Sint8 fight_camera;
/* the height of the ground his walk climbs onto */
static Float ground_ahead;
/* the steps since his walk last sloshed */
static Sint32 walk_sound_wait;
/* chaos0_punch zeroes it as a punch begins. nothing reads it */
static struct angles dat_0c95fe20;
/* the share of the punch's stretch a step makes */
static Float stretch_step;
/* bytes nothing reads or writes */
static Uint8 dat_0c95fe30[4];
/* how far his figure turns a step as he spins */
static Angle spin_speed;
/* his jump's upward speed as it starts */
static Float jump_speed;
/* the punches he has left on a pole */
static Sint8 pole_punches;
/* how far a hit has run, from 0 to MELT_END, and whether its splash is
 * still to sound */
static Float hit_time;
static Sint8 splash_due;
/* how far a melt and a rise have run, from 0 to MELT_END */
static Float melt_time;
static Float form_time;

/* hide the beads on his arms' ribbon nodes, shoulder to hand, or show
 * them */
static void arm_beads_hide(Bool hidden)
{
    ribbon_hidden[3] = hidden;
    ribbon_hidden[4] = hidden;
    ribbon_hidden[5] = hidden;
    ribbon_hidden[15] = hidden;
    ribbon_hidden[6] = hidden;
    ribbon_hidden[7] = hidden;
    ribbon_hidden[8] = hidden;
    ribbon_hidden[18] = hidden;
}

/* stretch chaos0_punch_arm along x by scale, and shrink its hand and the
 * hand's sibling by as much, so they keep their size */
static void punch_arm_scale(Float scale)
{
    /* @hack spelling: each node through its address, which SHC pools and stores
     * scl through at offset 32 in R0 (MOV #32,R0 at 0x0c90035e); without it,
     * the code is 32 bytes shorter and 8 of 10 words differ */
    (&chaos0_punch_arm)->scl[0] = scale;
    (&chaos0_punch_hand)->scl[0] = (&dat_0c92139c)->scl[0] = 1.0F / scale;
}

/* stretch chaos0_other_arm along x by scale, and shrink its hand by as
 * much */
static void other_arm_scale(Float scale)
{
    /* @hack spelling: each node through its address, which SHC pools and stores
     * scl through at offset 32 in R0 (MOV #32,R0 at 0x0c900372); without it,
     * the code is 32 bytes shorter and 8 of 10 words differ */
    (&chaos0_other_arm)->scl[0] = scale;
    /* @bug logic: the second 1.0F / scale goes to the punching arm's
     * dat_0c92139c (MOV.L at 0x0c900370), not to dat_0c92046c, its mirror
     * under this arm. scaled_nodes lists dat_0c92046c, but nothing sets its
     * scale, so as the spin stretches this arm, dat_0c92046c stretches with
     * it */
    (&chaos0_other_hand)->scl[0] = (&dat_0c92139c)->scl[0] = 1.0F / scale;
}

/* the pole nearest pos, in chaos0_poles */
static NJS_POINT3 *pole_nearest(NJS_POINT3 *pos)
{
    int i;
    int nearest;
    Float best = 999999.0F;
    Float distance;

    for (i = 0; i < POLES; i++) {
        distance = njDistanceP2P(pos, &chaos0_poles[i]);
        if (best > distance) {
            best = distance;
            nearest = i;
        }
    }
    /* @bug memory: with no distance below 999999, as for a NaN pos,
     * no pass sets nearest, and the return indexes chaos0_poles by R11
     * (MOV R11,R0 at 0x0c900414) as chaos0_jump, its one caller, left it:
     * the address of player 0's work (MOV.L @R11,R11 at 0x0c900f28).
     * chaos0_jump then reads x outside chaos0_poles (FMOV.S @R4,FR3 at
     * 0x0c900fec) */
    return &chaos0_poles[nearest];
}

/* aim the record's target ACROSS_RADIUS from the fight's center, on the far
 * side from player 0. returns the target's heading from the center */
static Angle aim_across(struct task_work *work, void *extra)
{
    struct task_work *player = player_work[0];
    /* @hack spelling: a copy of extra, which SHC keeps in R12 across the calls
     * (MOV R5,R12 at 0x0c90043c); with extra cast to a struct chaos pointer at
     * each use, 57 of 58 words differ */
    struct chaos *chaos = extra;
    Angle ang = (Angle)(atan2f(chaos_params->center.z - player->pos.z,
                               chaos_params->center.x - player->pos.x) *
                        ANGLE_PER_RADIAN);

    chaos->target.x = chaos_params->center.x + (njCos(ang) * ACROSS_RADIUS);
    chaos->target.z = chaos_params->center.z + (njSin(ang) * ACROSS_RADIUS);
    return ang;
}

/* he walks after player 0, up onto what's ahead or down off it */
static void chaos0_walk(struct task_work *work, struct task_motion *motion,
                        struct chaos *extra)
{
    /* @hack spelling: a second pointer to the record, for which SHC keeps the
     * record in R9 and in R11 (MOV R6,R9 and MOV R6,R11 at 0x0c900522 and
     * 0x0c900524); with extra alone, 171 of 313 words differ */
    struct chaos *chaos = extra;
    Float x;
    Float z;
    struct angles slope;
    Float ground;

    switch (work->submode) {
    case 0:
        chaos0_set_shapes(work, SHAPES_FORMED);
        boss_attack_checks = 0;
        extra->motion.wanted = ACTION_WALK;
        motion_flags = 0;
        walk_sound_wait = 0;
        work->submode = 1;
    case 1:
        motion_flags = 0;
        if (extra->motion.current == ACTION_WALK) {
            work->submode = 2;
        }
        break;
    case 2:
    case 3:
    case 4:
        work->pos.x += njCos(work->ang.y) * WALK_SPEED;
        work->pos.z += njSin(work->ang.y) * WALK_SPEED;
        chaos_travel += WALK_SPEED;
        /* @hack redundant: a never-run step of chaos_travel by 0.6, which, with
         * its use of 0.6, has R8 hold 2, the walk's action and next submode,
         * where it would hold 0 (0x0c90051c); without it, the function runs 2
         * bytes short, chaos0_punch after it moves, and 155 of 313 words
         * differ */
        if (0) {
            chaos_travel += 0.6F;
        }
        if (!(chaos->steps & SPLASH_MASK)) {
            chaos_effect_splash(work, 8, 0.5F, 1.0F);
            chaos0_wobble_push(0.7F);
        }
        break;
    }
    ground = ground_below(work->pos.x, work->pos.y + GROUND_PROBE, work->pos.z,
                          &slope);
    ground = ground > 0.0F ? ground : 0.0F;
    /* @hack redundant: a never-run block of steps of ground and a read of
     * motion's speed, whose uses put motion in R12 and walk_sound_wait's
     * address in R10 (0x0c900520, 0x0c90051a), and 1.0's FLDI1 in the first
     * branch's delay slot (0x0c900528); without it, the two registers swap
     * and 14 of 313 words differ */
    if (0) {
        ground += 1.0F;
        ground += 1.0F;
        jump_speed = motion->speed.y;
    }
    switch (work->submode) {
    case 2:
        chaos->flags |= CHAOS0_CAN_ATTACK;
        chaos_face_player(work, extra);
        if (work->pos.y - STEP_HEIGHT > ground) {
            work->submode = 3;
            motion->speed.y = 0.0F;
            chaos->flags &= ~CHAOS0_CAN_ATTACK;
        } else {
            x = work->pos.x + (njCos(work->ang.y) * LOOK_AHEAD);
            z = work->pos.z + (njSin(work->ang.y) * LOOK_AHEAD);
            ground_ahead =
                ground_below(x, work->pos.y + GROUND_PROBE, z, &slope);
            ground_ahead = ground_ahead > 0.0F ? ground_ahead : 0.0F;
            if (ground_ahead > work->pos.y + STEP_HEIGHT) {
                work->timer = CLIMB_STEPS;
                motion->speed.y =
                    (ground_ahead - work->pos.y) / TASK_TIMER(work);
                work->submode = 4;
                chaos->flags &= ~CHAOS0_CAN_ATTACK;
            } else {
                work->pos.y = ground;
            }
        }
        break;
    case 3:
        /* @hack spelling: -WALK_FALL added, an FADD at 0x0c900734; with -=,
         * SHC subtracts, and 1 of 313 words and a pool word differ */
        motion->speed.y += -WALK_FALL;
        work->pos.y += motion->speed.y;
        if (work->pos.y <= ground) {
            work->pos.y = ground;
            work->submode = 2;
            chaos_effect_splash(work, 8, 1.0F, 0.0F);
            chaos0_wobble_push(1.5F);
        }
        break;
    case 4:
        work->pos.y += motion->speed.y;
        work->timer--;
        if (work->pos.y >= ground_ahead) {
            work->pos.y = ground_ahead;
            work->submode = 2;
        }
        break;
    }
    if (work->submode != MODE_DONE) {
        walk_sound_wait++;
        if (walk_sound_wait > SLOSH_WAIT) {
            walk_sound_wait = 0;
            sound_play_at(SE_SLOSH, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
    }
}

/* he winds up and stretches his arm out at player 0, as many times as
 * boss_repeats says. the motion's pace is how far the arm has gone */
static void chaos0_punch(struct task_work *work, struct chaos *chaos,
                         struct motion *motion)
{
    struct task_work *player = player_work[0];
    Float dx;
    Float dy;
    Float dz;

    switch (work->submode) {
    case 0:
        chaos0_set_shapes(work, SHAPES_PUNCH);
        dat_0c95fe20.x = dat_0c95fe20.y = dat_0c95fe20.z = 0;
        chaos->flags &= ~CHAOS0_STRIKING;
    case 1:
        motion->wanted = ACTION_WIND_UP;
        motion_flags &= ~(MOTION_ENDED | MOTION_HOLDING);
        work->submode = 2;
    case 2:
        /* @hack spelling: the record by its motion, which starts it, here and
         * in submode 3. SHC keeps chaos in R11, motion in R12 and motion_flags'
         * address in R13 (0x0c90084c to 0x0c900856); with chaos at both
         * calls, 44 of 349 words differ */
        chaos_face_player(work, motion);
        if ((motion_flags & MOTION_ENDED) &&
            motion->current == ACTION_STRETCH) {
            motion_flags |= MOTION_HOLDING;
            chaos->flags &= ~CHAOS0_STRIKING;
            work->timer = STRIKE_WAIT;
            if (work->pos.y > ON_POLE_HEIGHT) {
                work->timer = POLE_STRIKE_WAIT;
            }
            work->submode = 3;
            chaos_effect_flash(&chaos0_hands[1]);
            sound_play_at(SE_WIND_UP, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        break;
    case 3:
        if (TASK_TIMER(work) > AIM_UNTIL) {
            chaos_face_player(work, motion);
            dx = player->pos.x - work->pos.x;
            dy = player->pos.y - work->pos.y;
            dz = player->pos.z - work->pos.z;
            chaos->pitch = -(Angle)(atan2f(dy, njSqrt((dx * dx) + (dz * dz))) *
                                    ANGLE_PER_RADIAN);
            chaos->reach = njDistanceP2P(&work->pos, &player_work[0]->pos);
            chaos_effect_gather(&chaos0_hands[1], 0);
        }
        work->timer--;
        if (!work->timer) {
            motion_flags &= ~(MOTION_ENDED | MOTION_HOLDING);
            motion->pace = 0.0F;
            work->submode = 4;
            chaos->flags |= CHAOS0_STRIKING;
            chaos_effect_splash(work, 10, 0.5F, 2.0F);
            chaos0_wobble_push(1.5F);
            sound_play(SE_STRIKE, 0, 0, 127);
            stretch_step = STRETCH_SPEED / chaos->reach;
            if (work->pos.y > ON_POLE_HEIGHT) {
                stretch_step *= POLE_STRETCH;
            }
        }
        break;
    case 4:
        if ((motion_flags & MOTION_ENDED) &&
            motion->current == ACTION_STRETCHED) {
            motion_flags |= MOTION_HOLDING;
            evalflags_set_all(stretch_hides, NJD_EVAL_HIDE);
            chaos->figures |= FIGURE_STRETCH;
        }
        chaos_effect_trail(&chaos0_hands[1], &chaos0_hands_last[1], 0);
        motion->pace += stretch_step;
        if (motion->pace >= 1.0F) {
            motion->pace = 1.0F;
            work->submode = 5;
        }
        break;
    case 5:
        motion->pace -= stretch_step;
        if (motion->pace <= 0.0F) {
            motion->pace = 0.0F;
            motion_flags &= ~(MOTION_ENDED | MOTION_HOLDING);
            work->submode = 6;
            evalflags_clear_all(stretch_hides, NJD_EVAL_HIDE);
            chaos->figures &= ~FIGURE_STRETCH;
        }
        break;
    case 6:
        boss_repeats--;
        if (boss_repeats > 0) {
            work->submode = 1;
        } else {
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        chaos->flags &= ~CHAOS0_STRIKING;
        evalflags_clear_all(stretch_hides, NJD_EVAL_HIDE);
        chaos->figures &= ~FIGURE_STRETCH;
        punch_arm_scale(1.0F);
        if (boss_return_mode != work->mode) {
            boss_next_mode = boss_return_mode;
            boss_last_mode = boss_return_mode = work->mode;
        }
    }
}

/* his arms stretched out, he spins up, goes after player 0 for SPIN_STEPS
 * steps, and slows again */
static void chaos0_spin(struct task_work *work, struct task_motion *motion,
                        struct chaos *extra)
{
    struct angles slope;
    Float ground;
    Float rise;
    Float scale;
    Float height;
    Float least_rise;
    int i;

    switch (work->submode) {
    case 0:
        chaos0_set_shapes(work, SHAPES_SPIN);
        extra->motion.wanted = ACTION_SPIN;
        motion_flags &= ~MOTION_ENDED;
        work->submode = 1;
    case 1:
        if (motion_flags & MOTION_HOLDING) {
            spin_speed = SPIN_START;
            work->timer = SPIN_STEPS;
            extra->motion.pace = 0.0F;
            work->submode = 2;
            chaos0_wobble_push(1.5F);
        }
        break;
    case 2:
        extra->motion.pace = extra->motion.pace + SPREAD_STEP < 1.0F
                                 ? extra->motion.pace + SPREAD_STEP
                                 : 1.0F;
        spin_speed += SPIN_GAIN;
        chaos0_body_object.ang[1] += spin_speed;
        if (spin_speed >= SPIN_TOP) {
            work->submode = 3;
        }
        break;
    case 3:
        chaos_face_player(work, extra);
        work->pos.x += njCos(work->ang.y) * WALK_SPEED;
        work->pos.z += njSin(work->ang.y) * WALK_SPEED;
        ground = ground_below(work->pos.x, work->pos.y + GROUND_PROBE,
                              work->pos.z, &slope);
        height = ground > 0.0F ? ground : 0.0F;
        /* @hack redundant: a never-run step of height, which puts motion_flags'
         * address in R10 and chaos0_body_object's in R9 (0x0c900c6c,
         * 0x0c900cb8), and height in FR5 and least_rise in FR6 (0x0c900d62,
         * 0x0c900d6a); without it, each pair of registers swaps and 18 of 289
         * words differ */
        if (0) {
            height += 1.0F;
        }
        /* @hack spelling: the lower bound in a local, which keeps motion_flags'
         * address in R10 and chaos0_body_object's in R9 (0x0c900c6c,
         * 0x0c900cb8); as -SPIN_CLIMB at both uses, 12 of 289 words differ */
        least_rise = -SPIN_CLIMB;
        if ((rise = height - work->pos.y) < least_rise) {
            rise = least_rise;
        } else if (rise > SPIN_CLIMB) {
            /* @hack redundant: a constant select, which keeps SPIN_CLIMB in
             * FR3, where the test loaded it (0x0c900d7c, 0x0c900d82); without
             * it, 15 of 289 words differ */
            rise = 2.0F > 0.0F ? SPIN_CLIMB : 0.0F;
        }
        work->pos.y += rise;
        chaos0_wobble_push(1.5F);
        chaos0_body_object.ang[1] += spin_speed;
        /* @hack redundant: a never-run turn of chaos0_body_object by 18, which
         * keeps its address in R9 and the loop's index in R8 (0x0c900be0,
         * 0x0c900e50); without it, the two registers swap and 11 of 289 words
         * differ */
        if (0) {
            chaos0_body_object.ang[1] += 18;
        }
        work->timer--;
        if (!work->timer) {
            work->submode = 4;
        }
        break;
    case 4:
        extra->motion.pace = extra->motion.pace - SPREAD_STEP > 0.0F
                                 ? extra->motion.pace - SPREAD_STEP
                                 : 0.0F;
        spin_speed -= SPIN_GAIN;
        chaos0_body_object.ang[1] += spin_speed;
        if (spin_speed <= SPIN_START) {
            work->submode = MODE_DONE;
        }
        break;
    }
    scale = 1.0F + (extra->motion.pace * ARM_SPREAD);
    other_arm_scale(scale);
    punch_arm_scale(scale);
    work->collision->shapes[SHAPE_AROUND].radius = scale * SPIN_RADIUS;
    if (work->submode >= 2 && work->submode <= 4) {
        for (i = 0; i < 2; i++) {
            chaos_effect_trail(&chaos0_hands[i], &chaos0_hands_last[i], 1);
        }
        sound_play_at_for(SE_WHIRL, (int)work, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 30);
    }
    if (work->submode == MODE_DONE) {
        other_arm_scale(1.0F);
        punch_arm_scale(1.0F);
        chaos0_body_object.ang[1] = 0;
        motion_flags &= ~MOTION_HOLDING;
        extra->motion.state = MOTION_CHANGE;
        extra->motion.wanted = ACTION_STAND;
    }
}

/* he leaps as his vars' jump says, then takes to the pole he lands on, or
 * walks once he's down */
static void chaos0_jump(struct task_work *work, struct task_motion *motion,
                        struct chaos *extra)
{
    struct task_work *player = player_work[0];
    /* @hack spelling: a second pointer to the record, for which SHC keeps the
     * record in R12 and in R13 (MOV R6,R12 and MOV R6,R13 at 0x0c900f26 and
     * 0x0c900f2c); with extra alone, 155 of 445 words differ */
    struct chaos *chaos = extra;
    struct angles slope;
    NJS_POINT3 *point;
    Float old_y;
    Float time;
    Float ground;

    switch (work->submode) {
    case 0:
        chaos0_set_shapes(work, SHAPES_FORMED);
        chaos->jump_from.x = work->pos.x;
        chaos->jump_from.z = work->pos.z;
        chaos->jump_from.y = work->pos.y;
        chaos->flags &= ~CHAOS0_CAN_ATTACK;
        switch (CHAOS0_VARS(work)->jump) {
        case 0:
        case JUMP_AT_PLAYER:
            chaos->target.x =
                work->pos.x + ((player->pos.x - work->pos.x) * 0.5F);
            chaos->target.z =
                work->pos.z + ((player->pos.z - work->pos.z) * 0.5F);
            chaos->target.y =
                ground_below(chaos->target.x, work->pos.y + GROUND_PROBE,
                             chaos->target.z, &slope);
            chaos->target.y = chaos->target.y > 0.0F ? chaos->target.y : 0.0F;
            break;
        case JUMP_POLE:
        case JUMP_POLE_AGAIN:
            point = pole_nearest(&player->pos);
            /* @hack redundant: a never-run block of stores to point and reads
             * of player, whose uses put motion in R10, jump_speed's address in
             * R8 and 2 in R9 (0x0c900f22 to 0x0c900f30), and the steps flown in
             * FR4 and old_y in FR5 (0x0c90116e); without it, those registers
             * change hands and 22 of 445 words differ */
            if (0) {
                point->x = point->y;
                point->y = point->z;
                (void)player->ang.x;
                (void)player->ang.z;
                (void)motion->aim.x;
            }
            /* @bug logic: the test can't tell the pole he jumps from from
             * one that shares only his x or only his z, since either FCMP/EQ
             * alone sends him at player 0 (0x0c900fee, 0x0c900ffa).
             * chaos0_poles' points share an x or a z in pairs, so from a
             * pole, a jump to such a pole goes halfway to player 0 instead */
            if (chaos->jump_from.x != point->x &&
                chaos->jump_from.z != point->z) {
                chaos->target.x = point->x;
                chaos->target.z = point->z;
                chaos->target.y = point->y + POLE_TOP;
            } else {
                CHAOS0_VARS(work)->jump = JUMP_AT_PLAYER;
                chaos->target.x =
                    work->pos.x + ((player->pos.x - work->pos.x) * 0.5F);
                chaos->target.z =
                    work->pos.z + ((player->pos.z - work->pos.z) * 0.5F);
                chaos->target.y = player->pos.y;
            }
            break;
        case JUMP_ACROSS:
            aim_across(work, extra);
            chaos->target.y =
                ground_below(chaos->target.x, work->pos.y + GROUND_PROBE,
                             chaos->target.z, &slope);
            chaos->target.y = chaos->target.y > 0.0F ? chaos->target.y : 0.0F;
            break;
        }
        jump_speed =
            njSqrt(jump_heights[CHAOS0_VARS(work)->jump] * (2.0F * GRAVITY));
        time = (jump_speed + njSqrt((jump_speed * jump_speed) -
                                    ((chaos->target.y - chaos->jump_from.y) *
                                     (2.0F * GRAVITY)))) *
               (1.0F / GRAVITY);
        CHAOS0_VARS(work)->count.flight[0] = 0;
        CHAOS0_VARS(work)->count.flight[1] = (Sint16)time;
        motion->speed.x = (chaos->target.x - chaos->jump_from.x) /
                          CHAOS0_VARS(work)->count.flight[1];
        motion->speed.z = (chaos->target.z - chaos->jump_from.z) /
                          CHAOS0_VARS(work)->count.flight[1];
        sound_play_at(SE_LEAP, 0, 0, 0, work->pos.x, work->pos.y, work->pos.z);
        extra->motion.wanted = ACTION_LEAP;
        work->submode = 1;
    case 1:
    case 2:
        chaos_face_player(work, extra);
        work->pos.x += motion->speed.x;
        work->pos.z += motion->speed.z;
        old_y = work->pos.y;
        motion->speed.y =
            (work->pos.y =
                 chaos->jump_from.y +
                 (jump_speed * (Float)CHAOS0_VARS(work)->count.flight[0]) -
                 ((Float)CHAOS0_VARS(work)->count.flight[0] *
                  (Float)CHAOS0_VARS(work)->count.flight[0] *
                  (GRAVITY / 2.0F))) -
            old_y;
        CHAOS0_VARS(work)->count.flight[0]++;
        if (work->submode == 1) {
            if (work->pos.y < old_y) {
                extra->motion.wanted = ACTION_FALL;
                work->submode = 2;
            }
        } else if (CHAOS0_VARS(work)->jump & JUMP_POLE) {
            if (CHAOS0_VARS(work)->count.flight[0] >=
                CHAOS0_VARS(work)->count.flight[1]) {
                work->pos = chaos->target;
                work->submode = 3;
            } else if (CHAOS0_VARS(work)->count.flight[0] + POLE_LAND_LEAD >=
                       CHAOS0_VARS(work)->count.flight[1]) {
                extra->motion.wanted = ACTION_LAND_POLE;
            }
        } else {
            ground = ground_below(work->pos.x, work->pos.y + GROUND_PROBE,
                                  work->pos.z, &slope);
            ground = ground > 0.0F ? ground : 0.0F;
            if (work->pos.y <= ground) {
                work->pos.y = ground;
                work->submode = 3;
                chaos_effect_splash(work, 48, 1.0F, 0.0F);
                chaos0_wobble_push(1.5F);
            } else if (work->pos.y <= ground + LAND_LEAD) {
                extra->motion.wanted = ACTION_LAND;
                motion_flags &= ~MOTION_ENDED;
            }
        }
        break;
    case 3:
        if (CHAOS0_VARS(work)->jump & JUMP_POLE) {
            if (work->mode != BOSS_ON_POLE) {
                /* @hack spelling: the comma, which puts boss_next_mode's
                 * address in R6 and boss_last_mode's in R5 (0x0c9012ae,
                 * 0x0c9012b6), here and in the mode change below; as two
                 * statements, 4 of 445 words differ */
                boss_last_mode = (boss_next_mode = BOSS_ON_POLE,
                                  boss_return_mode = work->mode);
            }
        } else if (motion_flags & MOTION_ENDED) {
            chaos->flags |= CHAOS0_CAN_ATTACK;
            if (work->mode != BOSS_WALK) {
                boss_next_mode = BOSS_WALK;
                boss_last_mode = boss_return_mode = work->mode;
            }
        }
        break;
    }
}

/* on a pole, he punches each time player 0 comes within POLE_REACH across,
 * while he has punches left. while player 0 stays farther, he makes a
 * JUMP_POLE_AGAIN jump. out of punches, he jumps down at player 0 */
static void chaos0_on_pole(struct task_work *work, struct task_motion *motion,
                           struct chaos *extra)
{
    struct task_work *player = player_work[0];
    Float dx;
    Float dz;
    Float distance;

    switch (work->submode) {
    case 0:
        extra->motion.wanted = ACTION_ON_POLE;
        motion_flags &= ~MOTION_HOLDING;
        if (CHAOS0_VARS(work)->jump == JUMP_POLE) {
            pole_punches = POLE_PUNCHES;
            CHAOS0_VARS(work)->jump = JUMP_POLE_AGAIN;
        }
        work->submode = 1;
    case 1:
        chaos_face_player(work, extra);
        if (pole_punches <= 0) {
            if (work->mode != BOSS_JUMP) {
                boss_next_mode = BOSS_JUMP;
                boss_last_mode = boss_return_mode = work->mode;
            }
            CHAOS0_VARS(work)->jump = JUMP_AT_PLAYER;
        } else {
            dx = player->pos.x - work->pos.x;
            dz = player->pos.z - work->pos.z;
            distance = (dx * dx) + (dz * dz);
            if (distance < POLE_REACH * POLE_REACH) {
                boss_repeats = 1;
                pole_punches--;
                if (work->mode != BOSS_PUNCH) {
                    boss_next_mode = BOSS_PUNCH;
                    boss_last_mode = boss_return_mode = work->mode;
                }
            } else {
                if (work->mode != BOSS_JUMP) {
                    boss_next_mode = BOSS_JUMP;
                    boss_last_mode = boss_return_mode = work->mode;
                }
                CHAOS0_VARS(work)->jump = JUMP_POLE_AGAIN;
            }
        }
        break;
    }
}

/* put both arms back to their length, and show the nodes the stretched arm
 * hides */
static void arms_restore(void)
{
    other_arm_scale(1.0F);
    punch_arm_scale(1.0F);
    evalflags_clear_all(stretch_hides, NJD_EVAL_HIDE);
    /* @hack redundant: a return where the function ends, which keeps SHC from
     * expanding it at its two calls (0x0c90171a, 0x0c902642); without it,
     * the code is 160 bytes longer and 12 of 12 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

/* struck, he loses a point of health and melts into the puddle, to flow
 * off in it. his last hit leaves blobs and clears the stage */
static void chaos0_hit(struct task *task, struct task_motion *motion,
                       struct chaos *extra)
{
    struct task_work *work = task->work;
    /* @hack spelling: a second pointer to the record, for which SHC keeps the
     * record in R10 and in R11 (MOV R6,R10 and MOV R6,R11 at 0x0c90143e and
     * 0x0c901446); with extra alone, 128 of 291 words differ */
    struct chaos *chaos = extra;
    /* @hack spelling: 0 and 1 in locals, for the stores below that take them,
     * which SHC keeps in R13 and R14 (0x0c901442, 0x0c901452); as
     * constants, with the test below gone, 85 of 291 words differ */
    /* @hack redundant: register on both, which keeps work in R12 and 0.5 in
     * FR14 beside them (0x0c901448, 0x0c90144a); without it, 86 of 291 words
     * differ */
    register int inactive = 0;
    register Sint8 active = 1;

    /* @hack redundant: a test of 0.0F, which SHC drops only after it has
     * given out registers: inactive keeps R13, and hit_time's and
     * body_deform's addresses R8 and R9 (0x0c90143c, 0x0c901440); without
     * it, 85 of 291 words differ */
    if (0.0F) {
        inactive = -1;
    }
    switch (work->submode) {
    case 0:
        chaos0_set_shapes(work, SHAPES_NONE);
        brain_shown = inactive;
        extra->motion.wanted = ACTION_HIT;
        chaos->health -= 1.0F;
        sound_play_at(SE_HIT, 0, 0, 0, work->pos.x, work->pos.y, work->pos.z);
        splash_due = active;
        body_deform->mode = active; /* active holds 1, DEFORM_MELT */
        work->submode = active;
        break;
    case 1:
        if (extra->motion.current != ACTION_HIT) {
            break;
        }
        work->submode = 2;
        chaos->figures |= (Sint16)(FIGURE_MELTING | FIGURE_PUDDLE);
        chaos->scale.x = chaos->scale.y = chaos->scale.z = 0.0F;
        chaos_effect_splash(work, 64, 1.5F, 0.0F);
        if (chaos->health <= 0.0F) {
            chaos_effect_splash(work, 15, 3.5F, 1.0F);
            chaos_effect_splash(work, 15, 2.5F, 2.0F);
            chaos_effect_splash(work, 15, 0.5F, 2.5F);
            /* blob_place doesn't read its second argument. the image passes
             * the record there (0x0c901568) */
            blob_place(work, (int)chaos, 8);
            chaos->puddle_mode = active; /* active holds 1, PUDDLE_RESET */
            chaos_puddle_deform(work, chaos, &chaos_puddle);
        }
        extra->motion.pace = hit_time = 0.0F;
        break;
    case 2:
        hit_time += MELT_END / HIT_STEPS;
        extra->motion.pace = njSqrt(hit_time) * MELT_PACE;
        if (extra->motion.pace > 0.5F && splash_due) {
            sound_play_at(SE_SPLASH, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
            splash_due = inactive;
        }
        if (extra->motion.pace > 0.5F) {
            /* @hack redundant: a jump to the next line, which loads
             * chaos0_blink's address into R3 for this call (0x0c90162e);
             * without it, 2 of 291 words differ */
            goto unblink;
        unblink:
            chaos0_blink(inactive);
        } else {
            chaos0_blink(chaos->steps);
        }
        /* @hack redundant: a never-run block of stores of active, which keeps 1
         * in R14 and 0 in R13 (0x0c901452, 0x0c901442); without it, the two
         * registers swap and 21 of 291 words differ */
        if (0) {
            splash_due = active;
            work->submode = active;
        }
        if (hit_time >= MELT_END) {
            work->submode = MODE_DONE;
            switch ((int)chaos->health) {
            case 0:
                fight_camera = inactive;
                work->submode = 3;
                stage_clear();
                task_kill(task);
                return;
            case 1:
                if (work->mode != BOSS_FLOW_AFTER) {
                    boss_next_mode = BOSS_FLOW_AFTER;
                    boss_last_mode = boss_return_mode = work->mode;
                }
                break;
            case 2:
                if (work->mode != BOSS_FLOW_ACROSS) {
                    boss_next_mode = BOSS_FLOW_ACROSS;
                    boss_last_mode = boss_return_mode = work->mode;
                }
                break;
            }
        }
        body_deform->scale = extra->motion.pace;
        chaos->scale.x = chaos->scale.y = chaos->scale.z =
            (0.5F + (extra->motion.pace * 0.5F)) * PUDDLE_SIZE;
        break;
    case 3:
        break;
    }
    if (work->submode == MODE_DONE) {
        chaos->figures &= ~(FIGURE_MELTING | FIGURE_BODY);
        arms_restore();
        body_deform->mode = DEFORM_RESTORE;
        deform_step(body_deform);
    }
}

/* he melts down into the puddle over MELT_STEPS steps */
static void chaos0_melt(struct task_work *work, struct task_motion *motion,
                        struct chaos *extra)
{
    /* @hack spelling: a second pointer to the record, for which SHC keeps the
     * record in R13 and in R12 (MOV R6,R13 and MOV R6,R12 at 0x0c90177e and
     * 0x0c901792); with extra alone, 89 of 162 words differ */
    struct chaos *chaos = extra;

    switch (work->submode) {
    case 0:
        chaos0_set_shapes(work, SHAPES_MELTING);
        brain_shown = 0;
        extra->motion.wanted = ACTION_STAND;
        work->submode = 1;
        break;
    case 1:
        chaos_face_player(work, extra);
        if (extra->motion.current == ACTION_STAND) {
            chaos->figures =
                (Sint16)(FIGURE_MELTING | FIGURE_PUDDLE | FIGURE_BODY);
            chaos->scale.x = chaos->scale.y = chaos->scale.z = 0.0F;
            melt_time = 0.0F;
            extra->motion.pace = 1.0F;
            body_deform->mode = DEFORM_MELT;
            work->submode = 2;
        }
        break;
    case 2:
        chaos_face_player(work, extra);
        melt_time += MELT_END / MELT_STEPS;
        extra->motion.pace = njSqrt(melt_time) * MELT_PACE;
        work->collision->shapes[SHAPE_LOWER].center.y =
            (1.0F - extra->motion.pace) * LOWER_HEIGHT;
        if (melt_time >= MELT_END) {
            work->submode = MODE_DONE;
        }
        body_deform->scale = extra->motion.pace;
        chaos->scale.x = chaos->scale.z =
            (extra->motion.pace + 2.0F) * (PUDDLE_SIZE * (1.0F / 3.0F));
        chaos->scale.y = extra->motion.pace * PUDDLE_SIZE;
        break;
    }
    if (work->submode == MODE_DONE) {
        body_deform->mode = DEFORM_RESTORE;
        deform_step(body_deform);
        if (boss_return_mode != work->mode) {
            boss_next_mode = boss_return_mode;
            boss_last_mode = boss_return_mode = work->mode;
        }
        chaos->figures &= ~(FIGURE_MELTING | FIGURE_BODY);
        return;
    }
    sound_play_at_for(SE_MELT, (int)work, 1, 0, work->pos.x, work->pos.y,
                      work->pos.z, 6);
}

/* he rises again out of the puddle over MELT_STEPS steps */
static void chaos0_form(struct task_work *work, struct task_motion *motion,
                        struct chaos *extra)
{
    /* @hack spelling: a second pointer to the record, for which SHC keeps the
     * record in R13 and in R12 (MOV R6,R13 and MOV R6,R12 at 0x0c901946 and
     * 0x0c90194a); with extra alone, 87 of 167 words differ */
    struct chaos *chaos = extra;

    switch (work->submode) {
    case 0:
        chaos0_set_shapes(work, SHAPES_MELTING);
        extra->motion.wanted = ACTION_STAND;
        work->submode = 1;
        break;
    case 1:
        chaos_face_player(work, extra);
        if (extra->motion.current == ACTION_STAND) {
            chaos->figures =
                (Sint16)(FIGURE_MELTING | FIGURE_PUDDLE | FIGURE_BODY);
            chaos->scale.x = chaos->scale.y = chaos->scale.z = 0.0F;
            form_time = 0.0F;
            extra->motion.pace = 1.0F;
            body_deform->mode = DEFORM_MELT;
            work->submode = 2;
        }
        break;
    case 2:
        chaos_face_player(work, extra);
        form_time += MELT_END / MELT_STEPS;
        extra->motion.pace = 1.0F - (njSqrt(form_time) * MELT_PACE);
        work->collision->shapes[SHAPE_LOWER].center.y =
            (1.0F - extra->motion.pace) * LOWER_HEIGHT;
        if (form_time >= MELT_END) {
            work->submode = MODE_DONE;
        }
        body_deform->scale = extra->motion.pace;
        chaos->scale.x = chaos->scale.z =
            (extra->motion.pace + 2.0F) * (PUDDLE_SIZE * (1.0F / 3.0F));
        chaos->scale.y = extra->motion.pace * PUDDLE_SIZE;
        break;
    }
    if (work->submode == MODE_DONE) {
        body_deform->mode = DEFORM_RESTORE;
        deform_step(body_deform);
        if (boss_return_mode != work->mode) {
            boss_next_mode = boss_return_mode;
            boss_last_mode = boss_return_mode = work->mode;
        }
        chaos->figures &= ~(FIGURE_MELTING | FIGURE_PUDDLE);
        brain_shown = 1;
        return;
    }
    sound_play_at_for(SE_MELT, (int)work, 1, 0, work->pos.x, work->pos.y,
                      work->pos.z, 6);
}

/* as the puddle, he flows to the far side of the fight's center from
 * player 0, then forms and spins */
static void chaos0_flow_across(struct task_work *work,
                               struct task_motion *motion, struct chaos *extra)
{
    /* @hack spelling: a second pointer to the record, for which SHC keeps the
     * record in R13 and in R12 (MOV R6,R13 and MOV R6,R12 at 0x0c901b10 and
     * 0x0c901b16); with extra alone, 67 of 69 words differ */
    struct chaos *chaos = extra;
    Float dx;
    Float dz;

    switch (work->submode) {
    case 0:
        chaos0_set_shapes(work, SHAPES_FLAT);
        work->ang.y = aim_across(work, extra);
        work->submode = 1;
    case 1:
        chaos_steer(work, motion, extra);
        dx = work->pos.x - chaos->target.x;
        dz = work->pos.z - chaos->target.z;
        if (ACROSS_NEAR * ACROSS_NEAR > (dx * dx) + (dz * dz)) {
            if (work->mode != BOSS_FORM) {
                boss_next_mode = BOSS_FORM;
                boss_last_mode = boss_return_mode = work->mode;
            }
            boss_return_mode = BOSS_SPIN;
        }
        break;
    }
}

/* as the puddle, he flows to where player 0 was, for up to AFTER_STEPS + 1
 * steps, then forms and jumps to a pole */
static void chaos0_flow_after(struct task_work *work,
                              struct task_motion *motion, struct chaos *extra)
{
    /* @hack spelling: a second pointer to the record, for which SHC keeps the
     * record in a word of its frame and in R13 (MOV.L R6,@R15 and MOV
     * R6,R13 at 0x0c901b98 and 0x0c901ba4); with extra alone, 17 of 77
     * words differ */
    struct chaos *chaos = extra;
    struct task_work *player = player_work[0];

    switch (work->submode) {
    case 0:
        chaos0_set_shapes(work, SHAPES_FLAT);
        chaos->target = player->pos;
        CHAOS0_VARS(work)->count.steps = 0;
        work->submode = 1;
    case 1:
        chaos_steer(work, motion, extra);
        CHAOS0_VARS(work)->count.steps++;
        if (njDistanceP2P(&work->pos, &chaos->target) < AFTER_NEAR ||
            CHAOS0_VARS(work)->count.steps > AFTER_STEPS) {
            if (work->mode != BOSS_FORM) {
                boss_next_mode = BOSS_FORM;
                boss_last_mode = boss_return_mode = work->mode;
            }
            boss_return_mode = BOSS_JUMP;
            CHAOS0_VARS(work)->jump = JUMP_POLE;
        }
        break;
    }
}

/* a barrier round him throws player 0 off each time player 0 touches it.
 * it falls once 10 steps have passed since the last throw, unless player 0
 * is curled up within BARRIER_NEAR */
static void chaos0_barrier(struct task_work *work, struct chaos *extra)
{
    struct task_work *player = player_work[0];
    /* @hack spelling: a second pointer to the record, for which SHC keeps the
     * record in R12 and in R9 (MOV R5,R12 and MOV R5,R9 at 0x0c901c6a and
     * 0x0c901c74); with extra alone, 52 of 184 words differ */
    struct chaos *chaos = extra;
    NJS_VECTOR push;
    int steps;

    chaos_face_player(work, extra);
    switch (work->submode) {
    case 0:
        chaos0_set_shapes(work, SHAPES_BARRIER);
        extra->motion.wanted = ACTION_BARRIER_UP;
        motion_flags &= ~(MOTION_ENDED | MOTION_HOLDING);
        work->submode = 1;
    case 1:
        if (extra->motion.current == ACTION_BARRIER_UP) {
            chaos->flags |= CHAOS0_BARRIER;
            barrier_level = 0.0F;
            barrier_steps = BARRIER_HOLD + 1;
            work->submode = 2;
        }
        break;
    case 2:
        if (work->flags & COLLISION_TOUCHING) {
            push = barrier_push;
            player_turn_to_world(player, &push);
            player_set_speed(0, push.x, push.y, push.z);
            barrier_steps = 0;
        }
        barrier_level = barrier_level + LEVEL_STEP < BARRIER_LEVEL
                            ? barrier_level + LEVEL_STEP
                            : BARRIER_LEVEL;
        steps = barrier_steps + 1;
        barrier_steps = steps < BARRIER_HOLD ? steps : BARRIER_HOLD;
        if ((!(player->flags & PLAYER_BALL) ||
             njDistanceP2P(&work->pos, &player_work[0]->pos) > BARRIER_NEAR) &&
            barrier_steps >= BARRIER_HOLD) {
            work->submode = 3;
        }
        break;
    case 3:
        barrier_level = barrier_level - LEVEL_STEP > 0.0F
                            ? barrier_level - LEVEL_STEP
                            : 0.0F;
        if (barrier_level <= 0.0F) {
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        chaos0_set_shapes(work, SHAPES_FORMED);
        motion_flags &= ~MOTION_HOLDING;
        extra->motion.wanted = ACTION_BARRIER_DOWN;
        chaos->flags &= ~CHAOS0_BARRIER;
        if (work->mode != BOSS_WALK) {
            boss_next_mode = BOSS_WALK;
            boss_last_mode = boss_return_mode = work->mode;
        }
    }
}

/* thrown back from player 0 for 9 steps, he falls to the ground, blinking,
 * then takes his hit */
static void chaos0_fall(struct task_work *work, struct task_motion *motion,
                        struct chaos *extra)
{
    struct task_work *player = player_work[0];
    /* @hack spelling: a second pointer to the record, for which SHC keeps the
     * record in two words of its frame (MOV.L R6,@R15 and MOV.L R3,@(4,R15)
     * at 0x0c901e90 and 0x0c901e94); with extra alone, 139 of 145 words
     * differ */
    struct chaos *chaos = extra;
    struct angles slope;
    Angle ang;
    Float ground;

    switch (work->submode) {
    case 0:
        ang = (Angle)(atan2f(work->pos.z - player->pos.z,
                             work->pos.x - player->pos.x) *
                      ANGLE_PER_RADIAN);
        motion->speed.x = njCos(ang) * THROW_SPEED;
        motion->speed.z = njSin(ang) * THROW_SPEED;
        motion->speed.y = 0.0F;
        extra->motion.wanted = ACTION_THROWN;
        work->submode = 1;
        CHAOS0_VARS(work)->count.steps = THROW_STEPS;
    case 1:
        CHAOS0_VARS(work)->count.steps--;
        if (CHAOS0_VARS(work)->count.steps == 0) {
            motion->speed.x = motion->speed.z = 0.0F;
            work->submode = 2;
        }
    case 2:
        motion->speed.y -= GRAVITY;
        njAddVector(&work->pos, &motion->speed);
        ground = ground_below(work->pos.x, work->pos.y + GROUND_PROBE,
                              work->pos.z, &slope);
        ground = ground > 0.0F ? ground : 0.0F;
        /* the fall ends only at or below ON_POLE_HEIGHT, so never on a
         * pole's top */
        if (work->pos.y <= ground && work->pos.y <= ON_POLE_HEIGHT) {
            work->pos.y = ground;
            work->submode = MODE_DONE;
        }
        break;
    }
    chaos0_blink(chaos->steps);
    if (work->submode == MODE_DONE) {
        chaos0_blink(0);
        if (work->mode != BOSS_HIT) {
            boss_next_mode = BOSS_HIT;
            boss_last_mode = boss_return_mode = work->mode;
        }
    }
}

/* give object's first material an alpha of percent, out of 100, with
 * NJD_FLAG_USE_ALPHA set and NJD_FLAG_USE_ENV cleared. an object without a
 * model is left alone */
static void object_set_alpha(NJS_OBJECT *object, int percent)
{
    if (object->model) {
        object->model->mats->attrflags |= NJD_FLAG_USE_ALPHA;
        object->model->mats->attrflags &= ~NJD_FLAG_USE_ENV;
        /* @hack redundant: a second store of NJD_FLAG_USE_ALPHA (0x0c901fc2 to
         * 0x0c901fca); without it, the code is 32 bytes shorter and 28 of 35
         * words differ */
        object->model->mats->attrflags |= NJD_FLAG_USE_ALPHA;
        object->model->mats->diffuse.argb.a = percent * 255 / 100;
    }
}

/* give his form's alpha to chaos0_alpha_nodes, the puddle, the blob, the
 * sphere and the stretched arm's nodes. list his body's nodes in body_nodes,
 * and set NJD_EVAL_UNIT_POS in each. the wobbling node takes his record's
 * second color */
static void chaos0_setup_nodes(void)
{
    NJS_OBJECT *object;
    NJS_OBJECT **list;
    /* @hack redundant: register on the cursor, which keeps it in R14
     * through both lists (0x0c90206e); without it, 15 of 83 words differ */
    register NJS_OBJECT **objects;
    NJS_MATERIAL *material;

    body_nodes = (NJS_OBJECT **)heap_alloc(BODY_LIST * sizeof(NJS_OBJECT *));
    chaos_list_nodes(&chaos0_body_object, body_nodes);
    objects = chaos0_alpha_nodes;
    while ((object = *objects++) != 0) {
        object_set_alpha(object, chaos_params->alpha);
    }
    object_set_alpha(&chaos_puddle, chaos_params->alpha);
    object_set_alpha(&chaos_blob, chaos_params->alpha);
    object_set_alpha(&chaos_sphere, chaos_params->alpha);
    list = (NJS_OBJECT **)heap_alloc(STRETCH_LIST * sizeof(NJS_OBJECT *));
    chaos_list_nodes(&chaos0_stretch_object, list);
    objects = list;
    while ((object = *objects++) != 0) {
        object_set_alpha(object, chaos_params->alpha);
    }
    heap_free((void *)list);
    evalflags_set_all(body_nodes, NJD_EVAL_UNIT_POS);
    /* @hack spelling: the wobbling node through its address, which SHC pools to
     * read the model at offset 4 (0x0c90213c); without it, 12 of 83 words
     * differ */
    material = (&chaos0_wobble_node)->model->mats;
    /* the material stops blinking and draws in the second color. the mask
     * clears 0x4 too, a bit nothing in the module reads */
    material->attrflags &= ~(MATERIAL_BLINKS | MATERIAL_SECOND_COLOR | 0x4);
    material->attrflags |= MATERIAL_SECOND_COLOR;
    evalflags_set_all(dat_0c95e728, NJD_EVAL_HIDE);
}

/* run his mode for a step. BOSS_START starts the fight once the camera's
 * swing ends: his life gauge, his camera, the clock, player 0's pad and the
 * hints */
static void chaos0_run_mode(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct chaos *chaos = task->extra;

    switch (work->mode) {
    case BOSS_START:
        if (camera_swing_done) {
            life_gauge_create(0, 0, chaos_params->hits);
            fight_camera = 1;
            if (work->mode != BOSS_WALK) {
                boss_next_mode = BOSS_WALK;
                boss_last_mode = work->mode;
            }
            camera_override_mode(CAMERA_MODE_CHAOS, CAMERA_ADJUST_NONE);
            chaos->camera_mode = CAMERA_MODE_CHAOS;
            time_start();
            pad_enable_player(0);
            task_create_child(TASK_MOTION | TASK_WORK, chaos0_hint_step, task);
        }
        break;
    case BOSS_WALK:
        chaos0_walk(work, motion, chaos);
        chaos0_take_hit(work, chaos);
        break;
    case BOSS_PUNCH:
        chaos0_punch(work, chaos, &chaos->motion);
        chaos0_take_hit(work, chaos);
        break;
    case BOSS_SPIN:
        chaos0_spin(work, motion, chaos);
        chaos0_take_hit(work, chaos);
        break;
    case BOSS_JUMP:
        chaos0_jump(work, motion, chaos);
        break;
    case BOSS_ON_POLE:
        chaos0_on_pole(work, motion, chaos);
        break;
    case BOSS_HIT:
        chaos0_hit(task, motion, chaos);
        break;
    case BOSS_FLOW_AFTER:
        chaos0_flow_after(work, motion, chaos);
        break;
    case BOSS_MELT:
        chaos0_melt(work, motion, chaos);
        break;
    case BOSS_FORM:
        chaos0_form(work, motion, chaos);
        break;
    case BOSS_FLOW_ACROSS:
        chaos0_flow_across(work, motion, chaos);
        break;
    case BOSS_BARRIER:
        chaos0_barrier(work, chaos);
        break;
    case BOSS_FALL:
        chaos0_fall(work, motion, chaos);
        break;
    case BOSS_WAIT:
    case BOSS_MODE_21:
        chaos0_set_shapes(work, SHAPES_NONE);
        break;
    }
}

void chaos0_create(void)
{
    task_create(TASK_MOTION | TASK_WORK, 2, chaos0_boss_step);
}

/* his task's destructor: end his mode, then free and end what his setup
 * made, the wobble's task and the tether among them */
static void chaos0_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos *chaos = task->extra;
    /* @hack spelling: the record again, as its motion, which keeps work in R14
     * and the record in R13 (0x0c90237e, 0x0c902386); with &chaos->motion,
     * 13 of 68 words differ */
    struct motion *record = task->extra;

    if (work->submode != MODE_DONE) {
        work->submode = MODE_DONE;
        chaos0_run_mode(task);
    }
    heap_free((void *)body_nodes);
    ribbon_free();
    chaos_light_delete();
    njSetMotionCallback(NULL);
    chaos->puddle_mode = PUDDLE_FREE;
    chaos_puddle_deform(work, chaos, &chaos_puddle);
    record->state = MOTION_FREE;
    motion_step(work, record);
    record->weld_mode = PLAYER_WELD_FREE;
    motion_weld(work, record);
    deform_free(body_deform);
    task_destroy(wobble_task);
    boss_waiting = 0;
    if (tether) {
        task_kill(tether);
    }
}

/* begin the fight: show his name, swing the camera round him with player 0
 * held still, and start the tether and the boss music's wait */
static void chaos0_begin(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos *chaos = task->extra;
    struct task *wait;

    chaos_face_off(work);
    boss_name_create(TEXT_1, -1, 240, 60);
    camera_set_boss_offsets(NULL, NULL);
    camera_start_swing(work, 10.0F, 20.0F, 240);
    camera_override_mode(CAMERA_MODE_CHAOS_STINIT, CAMERA_ADJUST_NONE);
    fight_camera = 0;
    time_stop();
    pad_disable_player(0);
    tether = chaos_tether_create(&player_work[0]->pos, &tether_center,
                                 tether_radius);
    camera_set_perspective(BOSS_PERSPECTIVE);
    stream_destroy();
    /* the music's wait asks for MUSIC_CHAOS on its 61st step */
    wait = task_create(TASK_WORK, 0, chaos0_music_wait_step);
    wait->work->mode = MUSIC_CHAOS;
    wait->work->timer = MUSIC_DELAY;
    boss_waiting = 0;
    brain_shown = 1;
    chaos->figures = FIGURE_BODY;
    chaos->flags &= ~CHAOS_NO_BEADS;
}

/* the boss's step. the first makes his record and sets him up. each one
 * after runs his mode and his choice of attack, then his camera, his motion
 * and his draw */
static void chaos0_boss_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct chaos *chaos = task->extra;
    /* @hack spelling: a second pointer to the record, read from task->extra
     * again once the setup has set it, for which SHC keeps the record in R11
     * and in R13 (MOV R4,R11 and MOV R4,R13 at 0x0c9025a6 and 0x0c9025a8);
     * with chaos alone, 279 of 467 words differ */
    struct chaos *record = task->extra;
    short camera;
    struct task *spot;

    switch (work->mode) {
    case BOSS_SETUP:
        camera_state->phase = 0;
        chaos = (struct chaos *)motion_create(work, sizeof(struct chaos));
        if (!chaos) {
            return;
        }
        collision_init(task, chaos0_shapes, COLLISION_ITEM, 1, shape_count);
        /* @hack spelling: work through the task, which loads task->work again
         * (0x0c9025e2); through work, 95 of 467 words differ */
        task->work->collision->radius = COLLISION_REACH;
        chaos->motion.current = chaos->motion.wanted = ACTION_STAND;
        chaos->motion.actions = chaos0_actions;
        chaos->motion.welds = chaos0_welds;
        work->mode = BOSS_START;
        task->extra = chaos;
        record = task->extra;
        boss_work = work;
        chaos_record = record;
        evalflags_clear_all(scaled_nodes, NJD_EVAL_UNIT_SCL);
        /* @hack spelling: the figure through its address, which loads
         * chaos0_wobble_step's address ahead of the color stores
         * (0x0c902616); without it, 18 of 467 words differ */
        (&chaos0_body_object)->evalflags &= ~NJD_EVAL_UNIT_ANG;
        boss_next_mode = work->mode;
        record->colors[0].a = 0.0F;
        record->colors[0].r = 0.0F;
        record->colors[0].g = 0.0F;
        record->colors[0].b = 0.0F;
        wobble_task =
            task_create(TASK_MOTION | TASK_WORK, 2, chaos0_wobble_step);
        task->destructor = chaos0_destroy;
        arms_restore();
        ribbon_init(work, record, &chaos0_ribbon);
        record->puddle_mode = PUDDLE_MAKE;
        chaos_puddle_deform(work, record, &chaos_puddle);
        chaos_setup(work, record, CHAOS_0);
        chaos_light_create(LIGHT_RANGE);
        chaos0_setup_nodes();
        motion_step(work, &record->motion);
        motion_weld(work, &record->motion);
        body_deform = deform_create(&chaos0_body_object, work);
        chaos_effect_pulse(&chaos0_pulse_node, task);
        land_show_low(1);
        task->draw = chaos0_draw;
        /* the textures the module head's pvm list loads from CHAOS0 */
        record->texlist = &chaos0_texlist;
        brain_shown = 0;
        if (work->mode != BOSS_WAIT) {
            boss_next_mode = BOSS_WAIT;
            boss_last_mode = work->mode;
        }
        boss_waiting = 1;
        brain_create(task, 1.0F, &boss_points[0].pos, &brain_offset);
        brain_shown = 1;
        break;
    default:
        chaos0_run_mode(task);
        break;
    }
    if (!boss_waiting) {
        chaos0_choose_attack(work, motion, chaos);
    } else {
        brain_shown = 0;
        if (record->steps > 1 && !event_in_control()) {
            chaos0_begin(task);
            chaos->motion.current = chaos->motion.wanted = ACTION_STAND;
            if (work->submode != MODE_DONE) {
                work->submode = MODE_DONE;
                chaos0_run_mode(task);
            }
            if (work->mode != BOSS_START) {
                boss_next_mode = BOSS_START;
                boss_last_mode = work->mode;
            }
        }
    }
    if (task->step == task_destroy) {
        return;
    }
    arm_beads_hide(work->mode == BOSS_PUNCH || work->mode == BOSS_SPIN);
    if (work->mode != boss_next_mode) {
        if (work->submode != MODE_DONE) {
            work->submode = MODE_DONE;
            chaos0_run_mode(task);
        }
        boss_last_mode = work->mode;
        work->mode = boss_next_mode;
        work->submode = 0;
    }
    if (fight_camera && !boss_waiting) {
        camera = CAMERA_MODE_CHAOS;
        if (work->pos.y - player_work[0]->pos.y > HIGH_ABOVE) {
            camera = CAMERA_MODE_CHAOS_P;
        }
        if (work->mode == BOSS_MODE_24) {
            camera = CAMERA_MODE_FOLLOW;
        }
        if (record->camera_mode != camera) {
            record->camera_mode = camera;
            camera_override_mode(camera, CAMERA_ADJUST_NONE);
            land_show_low(camera != CAMERA_MODE_CHAOS_P);
            if (camera == CAMERA_MODE_CHAOS_P) {
                camera_set_boss_offsets(NULL, &high_camera_offset);
            } else {
                camera_set_boss_offsets(NULL, NULL);
            }
        }
    }
    if (!(motion_flags & MOTION_HOLDING)) {
        motion_step(work, &chaos->motion);
    }
    motion_weld(work, &chaos->motion);
    if (record->figures & FIGURE_BODY) {
        ribbon_spawn();
    }
    chaos_track_points(work, chaos0_marks, chaos_params->model_y);
    chaos_animate_surface(chaos);
    chaos0_draw(task);
    if (record->flags & CHAOS0_CAN_ATTACK) {
        if (chaos_travel > SPOT_TRAVEL) {
            spot = task_create(TASK_WORK, 5, chaos_spot_step);
            if (spot) {
                spot->work->vars.f[0] = SPOT_ALPHA;
                chaos_travel = 0.0F;
            }
        }
    }
    if (record->figures & FIGURE_BODY) {
        record->ground = shadow_draw(work, SHADOW_SIZE);
    }
    work->pos.y = work->pos.y < 0.0F ? 0.0F : work->pos.y;
    ++record->steps;
    life_gauge_health = record->health;
    if (njDistanceP2P(&work->pos, &player_work[0]->pos) < NEAR_RANGE) {
        sound_play_at_for(SE_NEAR, (int)work, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 10);
    }
    sound_play_at_for(SE_HIGH, (int)work, 1, 0, 171.39F, 167.0F, 442.65F, 10);
    sound_play_at_for(SE_LOW, (int)work, 1, 0, 151.56F, 1.0F, 393.29F, 10);
    collision_register(work);
    task_step_children(task);
    work->pos.y = work->pos.y > 0.0F ? work->pos.y : 0.0F;
    if (!event_in_control()) {
        player_work[0]->pos.y =
            player_work[0]->pos.y > 0.0F ? player_work[0]->pos.y : 0.0F;
    }
}

/* draw his puddle, or his blobs once he is beaten, and his body with the
 * stretched arm, in his record's first color */
static void chaos0_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos *chaos = task->extra;
    Uint32 control;
    int current;
    Angle ang;
    NJS_ACTION *action;

    if (frame_step) {
        return;
    }
    chaos_light_follow(work);
    control = _nj_control_3d_flag_;
    njControl3D(_nj_control_3d_flag_ | NJD_CONTROL_3D_OFFSET_MATERIAL);
    draw_attr_save();
    draw_attr_clear(0, NJD_FLAG_IGNORE_SPECULAR);
    njSetTexture(chaos->texlist);
    current = chaos->motion.current;
    if (chaos->figures & (FIGURE_PUDDLE | FIGURE_4)) {
        chaos->flags |= CHAOS_NO_BEADS;
    } else {
        chaos->flags &= ~CHAOS_NO_BEADS;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    if (chaos->figures & FIGURE_PUDDLE) {
        if (chaos->scale.y < PUDDLE_FADE) {
            chaos->colors[0].a = (chaos->scale.y - PUDDLE_FADE) * 5.0F;
            njSetConstantMaterial(&chaos->colors[0]);
        } else {
            chaos->colors[0].a = 0.0F;
        }
        /* event_is_playing doesn't read its argument. the image passes 2
         * (0x0c902bc4) */
        if (chaos->health > 0.0F || event_is_playing(2)) {
            chaos_puddle_deform(work, chaos, &chaos_puddle);
            njPushMatrix(NULL);
            njScaleV(NULL, &chaos->scale);
            draw_tree(&chaos_puddle);
            njPopMatrix(1);
        } else {
            blob_draw(work, chaos);
        }
        chaos->colors[0].a = 0.0F;
    } else {
        chaos->puddle_mode = PUDDLE_RESET;
    }
    if (chaos->figures & FIGURE_BODY) {
        njPushMatrix(NULL);
        njSetConstantMaterial(&chaos->colors[0]);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, -work->ang.y + 0x8000, ang);
        njTranslate(NULL, 0.0F, ORIGIN_HEIGHT, 0.0F);
        njScaleV(NULL, &work->scl);
        chaos0_arm_pose(work, chaos);
        if (chaos->figures & FIGURE_MELTING) {
            deform_step(body_deform);
            if (chaos->motion.pace > MELT_FADE) {
                chaos->colors[0].a = (MELT_FADE - chaos->motion.pace) *
                                     chaos_params->alpha *
                                     (1.0F / (1.0F - MELT_FADE) / 100.0F);
            }
        }
        if (chaos->motion.state != MOTION_BLENDING) {
            action = chaos->motion.actions[current].action;
        } else {
            action = chaos->motion.blend;
        }
        njSetConstantMaterial(&chaos->colors[0]);
        draw_action(action, chaos->motion.frame);
        ribbon_place_nodes();
        njPopMatrix(1);
    }
    njPopMatrix(1);
    njControl3D(control);
    draw_attr_restore();
    njSetMotionCallback(NULL);
}

/* draw the land's LAND_LOW entries, or hide them */
static void land_show_low(int shown)
{
    struct land_entry *entry = chaos0_land.entries;
    int count = chaos0_land.count;
    int i;

    if (shown) {
        for (i = count - 1; i >= 0; entry++, i--) {
            if (entry->flags & LAND_LOW) {
                entry->flags |= LAND_DRAWN;
            }
        }
    } else {
        for (i = count - 1; i >= 0; entry++, i--) {
            if (entry->flags & LAND_LOW) {
                entry->flags &= ~LAND_DRAWN;
            }
        }
    }
}
