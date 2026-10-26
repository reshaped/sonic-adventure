/* Chaos 2, the boss: his task, his modes, his attacks and his draw */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos2/ball.h"
#include "b_chaos2/blight.h"
#include "b_chaos2/boss.h"
#include "b_chaos2/camera.h"
#include "b_chaos2/chaos2.h"
#include "b_chaos2/effects.h"
#include "b_chaos2/hint.h"
#include "b_chaos2/textures.h"
#include "camera/camera.h"
#include "chaos/chaos.h"
#include "chaos/deform.h"
#include "chaos/effect.h"
#include "chaos/emeralds.h"
#include "chaos/models.h"
#include "chaos/puddle.h"
#include "chaos/ribbon.h"
#include "chaos/steer.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/motion.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "effect/brain.h"
#include "event/play.h"
#include "hud/boss_name.h"
#include "hud/hud.h"
#include "hud/life_gauge.h"
#include "player/knuckles.h"
#include "player/player.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 7
#include "b_chaos2/boss/text.inc"

extern struct ribbon chaos2_ribbon;
extern struct weld chaos2_welds[];
/* nodes of his figure. his setup gives the first materials of dat_0c908664
 * and dat_0c90c388 MATERIAL_BLINKS, so they blink as he is hit. it hands
 * chaos2_pulse_node to chaos_effect_pulse */
extern NJS_OBJECT dat_0c908664;
extern NJS_OBJECT chaos2_pulse_node;
extern NJS_OBJECT dat_0c90c388;
/* the arm node of his figure whose x scale the stretch grows */
extern NJS_OBJECT chaos2_arm_node;
extern NJS_OBJECT chaos2_body_object;
extern NJS_OBJECT chaos2_ball_object;
/* the stretched arm's figure, and the arm and hand nodes in it that the
 * stretch scales */
extern NJS_OBJECT chaos2_stretch_hand;
extern NJS_OBJECT chaos2_stretch_arm;
extern NJS_OBJECT chaos2_stretch_object;
/* the stretch shows chaos2_arm_nodes again as it ends, and hides
 * chaos2_stretch_hides while the stretched arm shows. chaos2_setup_nodes
 * gives chaos2_alpha_nodes his form's alpha, and hides chaos2_hidden_nodes.
 * each list ends at a null */
extern NJS_OBJECT *chaos2_arm_nodes[];
extern NJS_OBJECT *chaos2_stretch_hides[];
extern NJS_OBJECT *chaos2_alpha_nodes[];
extern NJS_OBJECT *chaos2_hidden_nodes[];

/* a fist's SHAPE_ATTACK level, striking and at rest */
#define FIST_STRIKE 3
#define FIST_REST 1

/* the field of view chaos2_begin sets for the fight */
#define BOSS_PERSPECTIVE NJM_DEG_ANG(70)

#define SE_WIND_UP 0x14d
#define SE_STRIKE 0x14e
#define SE_LEAP 0x151
#define SE_THROWN 0x152
#define SE_HIT 0x153
#define SE_SPLASH 0x154
#define SE_MELT 0x155
#define SE_SPREAD 0x15d
#define SE_RISE 0x15e
#define SE_REACH 0x15f
#define SE_SLOSH 0x2cd
/* his step plays SE_NEAR while player 0 is nearer than NEAR_RANGE to him */
#define SE_NEAR 1000
#define NEAR_RANGE 60.0F

/* his chase splashes when the low byte of his record's steps is 0, once
 * every 256 steps. it sloshes as chase_sound_wait passes SLOSH_WAIT, every
 * 31 steps */
#define SPLASH_MASK 0xff
#define SLOSH_WAIT 30
/* a spot marks his chase's way each time chaos_travel passes SPOT_TRAVEL.
 * each spot's alpha starts at SPOT_ALPHA */
#define SPOT_TRAVEL 5
#define SPOT_ALPHA 1.6F

/* within PUNCH_REACH of player 0 he punches, and beyond it he stretches his
 * arm out. his leap starts only within it */
#define PUNCH_REACH 50.0F
/* his barrier rises at player 0 curled up, or punching as Knuckles, within
 * BARRIER_REACH and within BARRIER_FRONT of his facing either way. it falls
 * once player 0 is no longer curled up within BARRIER_REACH, and his
 * record's barrier_steps has reached BARRIER_WAIT */
#define BARRIER_REACH 70.0F
#define BARRIER_FRONT NJM_DEG_ANG(45)
#define BARRIER_WAIT 11
/* the barrier's glow rises by GLOW_STEP a step to GLOW_MOST, and falls back
 * by GLOW_STEP as the barrier goes down */
#define GLOW_STEP 0.0625F
#define GLOW_MOST 0.5F

/* as the stretch begins, chaos2_arm_node's x scale grows to 1 + ARM_GROWTH
 * at full pace. the stretched arm's x scale then grows to 1 + REACH_SHARE
 * times his reach at full pace. its hand's shrinks by as much, but to no
 * less than HAND_LEAST */
#define ARM_GROWTH (10.0F / 3.0F)
#define REACH_SHARE 0.125F
#define HAND_LEAST 0.2F
/* the stretched arm's aim turns at most AIM_TURN a step, toward AIM_ABOVE
 * above aim_target. while the motion's pace is below AIM_UNTIL, aim_target
 * and his reach follow player 0 */
#define AIM_TURN NJM_DEG_ANG(2)
#define AIM_ABOVE 5.0F
#define AIM_UNTIL 0.6F
/* a stretch or a punch ends in a rest of REST_TIME + 1 steps */
#define REST_TIME 180
/* a punch plays SE_STRIKE once its action passes frame STRIKE_FRAME */
#define STRIKE_FRAME 20.0F

/* a hit, a melt and a rise each add MELT_STEP a step to their time, toward
 * MELT_END */
#define MELT_STEP (MELT_END / 60.0F)
/* SHAPE_CORE's radius as he melts, and the height of its center as a melt
 * starts */
#define CORE_SIZE 10.0F
/* the puddle's radius is PUDDLE_RADIUS times its scale. the melt, the rise
 * and the gather set SHAPE_PUDDLE's so, and the spread sets the puddle
 * model's */
#define PUDDLE_RADIUS 5.0F
/* the puddle gathers into the ball over GATHER_STEPS steps */
#define GATHER_STEPS 60.0F
/* his spread takes the puddle out over SPREAD_STEPS steps, holds it
 * SPREAD_HOLD steps, and draws it back over SPREAD_STEPS. spread out, the
 * puddle's x and z scale is SPREAD_SCALE, and SHAPE_PUDDLE's radius
 * SPREAD_RADIUS. a blight starts as it spreads, and every BLIGHT_WAIT + 1
 * steps after, until it draws back */
#define SPREAD_STEPS 90.0F
#define SPREAD_HOLD 60
#define SPREAD_SCALE 20.0F
#define SPREAD_RADIUS 100.0F
#define BLIGHT_WAIT 30

/* he leaves the ground as ACTION_LEAP passes frame LEAP_FRAME. he leaps
 * LEAP_HEIGHT high, to LEAP_REACH from the hall's middle on the far side
 * from player 0, give or take LEAP_SPREAD degrees */
#define LEAP_FRAME 40.0F
#define LEAP_HEIGHT 30.0F
#define LEAP_REACH 70.0F
#define LEAP_SPREAD 30.0F

/* COLLISION_REACH is how far his shapes reach, as his collision record
 * gives it. LIGHT_RANGE is his cyan light's range, and NODE_SHOULDER the
 * node of his pose the stretched arm starts at. SHADOW_SIZE is his shadow's
 * size while his body or his round figure shows */
#define COLLISION_REACH 300.0F
#define LIGHT_RANGE 40.0F
#define NODE_SHOULDER 69
#define SHADOW_SIZE 7.0F
/* the boss music's wait counts down from MUSIC_DELAY */
#define MUSIC_DELAY 60
/* the puddle fades in as its height grows to PUDDLE_FADE. he fades out as a
 * melt's pace passes MELT_FADE */
#define PUDDLE_FADE 0.2F
#define MELT_FADE 0.7F

/* the dark spot a spread leaves fades in over SPREAD_SPOT_FADE steps, then
 * shrinks away over SPREAD_SPOT_SHRINK. its sprite's scale starts at
 * SPREAD_SPOT_SIZE */
#define SPREAD_SPOT_FADE 30
#define SPREAD_SPOT_SHRINK 90.0F
#define SPREAD_SPOT_SIZE (1.7F * 10.0F)

/* the dark spot's vars */
struct spot_vars {
    Float size; /* its share of SPREAD_SPOT_SIZE */
    Sint32 steps; /* the steps it has left to fade in */
    Float alpha;
};

#define SPOT(work) ((struct spot_vars *)&(work)->vars)

/* the dark spot's work->mode */
enum spot_mode { SPOT_START, SPOT_FADE_IN, SPOT_SHRINK };

static void chaos2_boss_step(struct task *task);
static void chaos2_draw(struct task *task);
static void chaos2_set_material(NJS_OBJECT *object);
static void spread_spot_step(struct task *task);

/* how far out along the stretched hand's node boss_points[10] lies, and
 * where the brain sits from the point it follows, in the camera's frame */
#pragma section HEAD
static Float hand_reach = 10.0F;
static NJS_POINT3 chaos2_brain_offset = {0.0F, 1.0F, 1.0F};
#pragma section

static const Uint8 chaos2_shape_count = BOSS_SHAPES;
/* how far across player 0 can go from the tether's center */
static const Float tether_radius = 86.0F;

/* his shapes. shapes 0 to 10 are balls on his body, which
 * chaos2_place_shapes moves to his figure's points. each of them hurts the
 * players at attack 1. 0 to 2 have no guard, and 3 to 10 the highest guard,
 * which no attack passes. shape 11, the barrier, only guards. SHAPE_PUDDLE
 * hurts the players at attack 3, and SHAPE_CORE at attack 1 */
/* clang-format off */
static struct collision_shape chaos2_shapes[BOSS_SHAPES] = {
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_HURTS_PLAYERS, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F}, 5.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_HURTS_PLAYERS, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F}, 5.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_HURTS_PLAYERS, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F}, 5.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 3.0F, 0.0F, 0.0F, {0},
     0, NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 3.0F, 0.0F, 0.0F, {0},
     0, NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 3.0F, 0.0F, 0.0F, {0},
     0, NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 7.0F, 0.0F, 0.0F, {0},
     0, NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 3.0F, 0.0F, 0.0F, {0},
     0, NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 3.0F, 0.0F, 0.0F, {0},
     0, NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 3.0F, 0.0F, 0.0F, {0},
     0, NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 7.0F, 0.0F, 0.0F, {0},
     0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_GUARD, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 12.5F, 0.0F}, 18.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE,
     ATTACK(3) | SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 0.0F, 3.0F, 0.0F, {0},
     0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_GUARD | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 0.0F, 0.0F, 0.0F, {0},
     0, NULL},
};
/* chaos2_set_shapes' rows: a 1 for each shape a row turns on */
static Sint8 shape_rows[][BOSS_SHAPES] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0},
};
/* the marks chaos_track_points moves boss_points onto each step, a point
 * to a mark. each mark is a point in the frame of a node of his pose */
static struct boss_mark chaos2_marks[] = {
    {76, 0, {-2.0F, -2.0F, 0.0F}},
    {29, 0, {0.0F, 0.0F, 0.0F}},
    {71, 0, {0.0F, 0.0F, 0.0F}},
    {50, 0, {0.0F, 0.0F, 0.0F}},
    {69, 0, {0.0F, 0.0F, 0.0F}},
    {48, 0, {0.0F, 0.0F, 0.0F}},
    {56, 0, {0.0F, 0.0F, 0.0F}},
    {35, 0, {0.0F, 0.0F, 0.0F}},
    {60, 0, {0.0F, 0.0F, 0.0F}},
    {39, 0, {0.0F, 0.0F, 0.0F}},
    {66, 0, {3.0F, 0.0F, 0.0F}},
    {45, 0, {3.0F, 0.0F, 0.0F}},
    {78, 0, {3.0F, 0.0F, 0.0F}},
    {-1, 0, {0.0F, 0.0F, 0.0F}},
};
/* clang-format on */
/* the offsets chaos2_begin gives camera_set_boss_offsets: none from what
 * the camera follows to its target, and camera_place_offset from player 0
 * to its place */
static NJS_POINT3 camera_target_offset = {0.0F, 0.0F, 0.0F};
static NJS_POINT3 camera_place_offset = {0.0F, 15.0F, 30.0F};
/* the tether's center, and the tether, which his destructor kills */
static NJS_POINT3 tether_center = {0.0F, 10.0F, 0.0F};
static struct task *tether = NULL;
/* each fist's punch, by fist */
static Sint8 punch_actions[2] = {ACTION_PUNCH_6, ACTION_PUNCH_10};
/* a constant material of 0, which chaos2_gather sets on each side of its
 * draw of the ball */
static NJS_ARGB ball_color = {0.0F, 0.0F, 0.0F, 0.0F};
/* the figures beside his own nodes that chaos2_setup_nodes gives his form's
 * alpha. the list ends at a null */
static NJS_OBJECT *alpha_objects[] = {&chaos_puddle,       &chaos_blob,
                                      &chaos2_drop_object, &chaos2_ball_object,
                                      chaos2_round_object, NULL};
/* the push that throws player 0 off his body, back and up in the frame
 * player 0 faces */
static NJS_VECTOR body_push = {-1.2F, 1.2F, 0.0F};

/* the deformer that melts his body, which his destructor frees */
static struct deform *body_deform;
/* the stretch's scales along x: chaos2_arm_node's, chaos2_stretch_arm's and
 * chaos2_stretch_hand's */
static Float arm_scale;
static Float reach_scale;
static Float hand_scale;
/* NODE_SHOULDER's matrix in the last pose, where the stretched arm starts */
static NJS_MATRIX shoulder_matrix;
/* the hints' task, which his destructor destroys */
static struct task *hint_task;
/* the steps since his chase last sloshed */
static Sint32 chase_sound_wait;
/* the stretched arm's turn toward aim_target, the point it reaches for */
static Angle aim_yaw;
static Angle aim_pitch;
static NJS_VECTOR aim_target;
/* player 0's place, which each step of the wind-up keeps. nothing reads
 * it */
static NJS_POINT3 player_seen;
/* the fist he punches with: 0 for SHAPE_FIST_6, 1 for SHAPE_FIST_10 */
static Sint8 fist;
/* how far a hit has run, from 0 to MELT_END, and whether its splash is
 * still to sound */
static Float hit_time;
static Sint8 splash_due;
/* how far a melt, a rise and a spread have run, from 0 to MELT_END */
static Float melt_time;
static Float form_time;
static Float spread_time;
/* the steps to the spread's next blight. the next starts once the count
 * falls below 0 */
static Sint16 spread_blight_wait;

void chaos2_set_shapes(struct task_work *work, int row)
{
    int i;

    for (i = 0; i < BOSS_SHAPES; i++) {
        if (shape_rows[row][i]) {
            work->collision->shapes[i].flags &= ~SHAPE_OFF;
        } else {
            work->collision->shapes[i].flags |= SHAPE_OFF;
        }
    }
    if (row == SHAPES_MELTED) {
        work->collision->shapes->levels =
            (work->collision->shapes->levels & ~SHAPE_GUARD) | SHAPE_GUARD;
    } else {
        work->collision->shapes->levels =
            work->collision->shapes->levels & ~SHAPE_GUARD;
    }
}

/* hide the beads on ribbon nodes 2 to 5, or show them. his step hides them
 * while he stretches his arm */
static void arm_beads_hide(Bool hidden)
{
    ribbon_hidden[2] = hidden;
    ribbon_hidden[3] = hidden;
    ribbon_hidden[4] = hidden;
    ribbon_hidden[5] = hidden;
}

/* his barrier, in front of him. it throws player 0 off each time player 0
 * touches him. it falls once player 0 is no longer curled up within
 * BARRIER_REACH, and BARRIER_WAIT steps have run since its last throw, the
 * throw's own counted. he then asks for BOSS_CHASE */
static void chaos2_barrier(struct task_work *work, struct chaos *extra)
{
    struct task_work *player = player_work[0];
    /* @hack spelling: a second pointer to the record, for which SHC keeps the
     * record in R11 and in R13 (MOV R5,R11 and MOV R5,R13 at 0x0c900532 and
     * 0x0c90053c); with extra alone, 88 of 207 words differ */
    struct chaos *chaos = extra;

    chaos_face_player(work, extra);
    switch (work->submode) {
    case 0:
        chaos2_set_shapes(work, SHAPES_BARRIER);
        chaos->flags |= CHAOS2_BARRIER;
        chaos->barrier_steps = BARRIER_WAIT;
        extra->motion.wanted = ACTION_BARRIER_UP;
        motion_flags &= ~(MOTION_ENDED | MOTION_HOLDING);
        work->submode = 1;
    case 1:
        if (extra->motion.current == ACTION_BARRIER_UP) {
            chaos->barrier_glow = 0.0F;
            work->submode = 2;
        }
        break;
    case 2:
        if (work->flags & COLLISION_TOUCHING) {
            NJS_VECTOR push = {-1.2F, 1.2F, 0.0F};

            player_turn_to_world(player, &push);
            player_set_speed(0, push.x, push.y, push.z);
            chaos->barrier_steps = 0;
            sound_play_at(SE_THROWN, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
            vibration_start(0, 7, 30, 2);
        }
        chaos->barrier_glow = chaos->barrier_glow + GLOW_STEP < GLOW_MOST
                                  ? chaos->barrier_glow + GLOW_STEP
                                  : GLOW_MOST;
        chaos->barrier_steps = chaos->barrier_steps + 1 < BARRIER_WAIT
                                   ? chaos->barrier_steps + 1
                                   : BARRIER_WAIT;
        if (!(player->flags & PLAYER_BALL) ||
            njDistanceP2P(&work->pos, &player_work[0]->pos) > BARRIER_REACH) {
            if (chaos->barrier_steps >= BARRIER_WAIT) {
                work->submode = 3;
                motion_flags &= ~MOTION_HOLDING;
                extra->motion.wanted = ACTION_BARRIER_DOWN;
            }
        }
        break;
    case 3:
        chaos->barrier_glow = chaos->barrier_glow - GLOW_STEP > 0.0F
                                  ? chaos->barrier_glow - GLOW_STEP
                                  : 0.0F;
        if (chaos->barrier_glow <= 0.0F && (motion_flags & MOTION_HOLDING)) {
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        motion_flags &= ~MOTION_HOLDING;
        chaos2_set_shapes(work, SHAPES_BODY);
        chaos->flags &= ~CHAOS2_BARRIER;
        if (work->mode != BOSS_CHASE) {
            boss_next_mode = BOSS_CHASE;
            boss_last_mode = work->mode;
        }
    }
}

/* move his body's balls, shapes 0 to 10, to points of boss_points. shapes
 * 1 and 2 share point 1, and sit 5 above it and 5 below */
static void chaos2_place_shapes(struct task_work *work)
{
    struct collision_shape *shapes = work->collision->shapes;
    struct boss_point *points = boss_points;

    shapes[0].center = points[0].pos;
    shapes[1].center = points[1].pos;
    shapes[2].center = points[1].pos;
    shapes[1].center.y += 5.0F;
    shapes[2].center.y -= 5.0F;
    /* @hack redundant: shape 0 placed twice more, each a copy of its center's
     * 12 bytes (0x0c9007da to 0x0c9007f4); without the two, the code is 32
     * bytes shorter and 114 of 137 words differ */
    shapes[0].center = points[0].pos;
    shapes[0].center = points[0].pos;
    shapes[3].center = points[3].pos;
    shapes[7].center = points[2].pos;
    shapes[4].center = boss_points[5].pos;
    shapes[8].center = boss_points[4].pos;
    shapes[5].center = boss_points[7].pos;
    shapes[9].center = boss_points[6].pos;
    shapes[6].center = boss_points[11].pos;
    shapes[10].center = boss_points[10].pos;
}

/* he walks after player 0. as he goes he splashes, sloshes and leaves
 * spots */
static void chaos2_chase(struct task_work *work, struct task_motion *motion,
                         struct chaos *extra)
{
    /* @hack spelling: a second pointer to the record, for which SHC keeps the
     * record in R10 and in R11 (MOV R6,R10 and MOV R6,R11 at 0x0c9008ec and
     * 0x0c9008fc); with extra alone, 76 of 147 words differ */
    struct chaos *chaos = extra;
    struct task *spot;

    switch (work->submode) {
    case 0:
        chaos->figures = FIGURE_BODY;
        brain_shown = 1;
        chaos->flags &= ~CHAOS2_BUSY;
        chaos2_set_shapes(work, SHAPES_BODY);
        extra->motion.wanted = ACTION_WALK;
        work->submode = 1;
        chaos_travel = 0.0F;
    case 1:
        motion_flags &= ~MOTION_HOLDING;
        if (extra->motion.current == ACTION_WALK) {
            chase_sound_wait = 0;
            work->submode = 2;
        }
        break;
    case 2:
        chaos->target = player_work[0]->pos;
        chaos_steer(work, motion, extra);
        if (!(chaos->steps & SPLASH_MASK)) {
            chaos_effect_splash(work, 8, 0.5F, 1.0F);
        }
        if (chaos_travel > SPOT_TRAVEL) {
            spot = task_create(TASK_WORK, 2, chaos_spot_step);
            if (spot) {
                spot->work->vars.f[0] = SPOT_ALPHA;
                /* keep what's past the last whole SPOT_TRAVEL */
                /* NOLINTNEXTLINE(bugprone-integer-division) */
                chaos_travel -= (int)chaos_travel / SPOT_TRAVEL * SPOT_TRAVEL;
            }
        }
        ++chase_sound_wait;
        if (chase_sound_wait > SLOSH_WAIT) {
            chase_sound_wait = 0;
            sound_play_at(SE_SLOSH, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        break;
    }
}

/* turn the stretched arm's aim toward AIM_ABOVE above aim_target, as seen
 * from boss_points[4], by at most AIM_TURN a step. record and motion are
 * unread */
static void stretch_aim(struct task_work *work, void *record,
                        struct motion *motion)
{
    NJS_VECTOR direction;
    Angle ang;

    direction = aim_target;
    direction.y += AIM_ABOVE;
    njSubVector(&direction, &boss_points[4].pos);
    ang = -(Angle)(atan2f(direction.z, direction.x) * ANGLE_PER_RADIAN) +
          work->ang.y;
    aim_yaw = angle_approach(aim_yaw, ang, AIM_TURN);
    ang = -(Angle)(atan2f(direction.y, njSqrt((direction.x * direction.x) +
                                              (direction.z * direction.z))) *
                   ANGLE_PER_RADIAN);
    aim_pitch = angle_approach(aim_pitch, ang, AIM_TURN);
}

/* set arm_scale to 1 + ARM_GROWTH times *amount */
/* @hack type: amount by its address, the address of the motion's pace. SHC
 * expands each call, copies the address into R4, and reads the pace back
 * through it after its store (MOV R13,R4 and FMOV.S @R4,FR2 at 0x0c900eb2
 * and 0x0c900ec0); by value, 106 of chaos2_stretch's 582 words differ */
static void arm_grow(const Float *amount)
{
    arm_scale = 1.0F + (*amount * ARM_GROWTH);
}

/* set reach_scale to 1 + REACH_SHARE times *amount times chaos's reach, and
 * hand_scale to its inverse, but to no less than HAND_LEAST */
/* @hack type: amount by its address, the address of the motion's pace,
 * which the calls pass in R5 (MOV R13,R5 at 0x0c900fc0); by value, 18 of
 * its 24 words and 3 of chaos2_stretch's 582 differ */
static void stretch_scale(struct chaos *chaos, const Float *amount)
{
    reach_scale = 1.0F + (chaos->reach * *amount * REACH_SHARE);
    hand_scale =
        1.0F / reach_scale < HAND_LEAST ? HAND_LEAST : 1.0F / reach_scale;
}

/* he winds up and stretches his arm out at player 0, as many times as
 * boss_repeats says. with player 0 within PUNCH_REACH, he asks for
 * BOSS_PUNCH instead. the motion's pace is how far the arm has gone */
static void chaos2_stretch(struct task_work *work, struct chaos *chaos,
                           struct motion *motion)
{
    struct task_work *player = player_work[0];
    NJS_VECTOR direction;
    Angle ang;

    switch (work->submode) {
    case 0:
        chaos->figures = FIGURE_BODY;
        /* @hack spelling: the three nodes through their addresses, here, in the
         * scale stores below and in MODE_DONE's block. SHC pools the
         * addresses, and stores scl through offset 32 in R0 (MOV #32,R0 at
         * 0x0c901156); through the nodes, the function is 4 bytes shorter
         * and 70 of 582 words differ */
        (&chaos2_arm_node)->evalflags &= ~NJD_EVAL_UNIT_SCL;
        (&chaos2_stretch_arm)->evalflags &= ~NJD_EVAL_UNIT_SCL;
        (&chaos2_stretch_hand)->evalflags &= ~NJD_EVAL_UNIT_SCL;
        chaos2_set_shapes(work, SHAPES_BODY);
    case 1:
        if (njDistanceP2P(&work->pos, &player->pos) < PUNCH_REACH) {
            if (work->mode != BOSS_PUNCH) {
                boss_next_mode = BOSS_PUNCH;
                boss_last_mode = work->mode;
            }
        } else {
            --boss_repeats;
            arm_scale = reach_scale = hand_scale = 1.0F;
            motion->wanted = ACTION_WIND_UP;
            motion_flags &= ~(MOTION_ENDED | MOTION_HOLDING);
            work->collision->shapes[SHAPE_FIST_10].levels =
                (work->collision->shapes[SHAPE_FIST_10].levels &
                 ~SHAPE_ATTACK) |
                FIST_STRIKE;
            sound_play_at(SE_WIND_UP, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
            work->submode = 2;
        }
    case 2:
        chaos_effect_flash(&boss_points[8].pos);
        chaos_effect_gather(&boss_points[8].pos, 3);
        player_seen = player->pos;
        ang = (Angle)(atan2f(player->pos.z - boss_points[4].pos.z,
                             player->pos.x - boss_points[4].pos.x) *
                      ANGLE_PER_RADIAN);
        work->ang.y = angle_approach(work->ang.y, ang, chaos_params->turn);
        if (motion->current == ACTION_WIND_UP) {
            direction = player->pos;
            njSubVector(&direction, &work->pos);
            chaos->pitch =
                -(Angle)(atan2f(direction.y,
                                njSqrt((direction.x * direction.x) +
                                       (direction.z * direction.z))) *
                         ANGLE_PER_RADIAN);
        }
        if ((motion_flags & MOTION_ENDED) &&
            motion->current == ACTION_STRETCHED) {
            chaos->flags |= CHAOS2_BUSY;
            motion_flags |= MOTION_HOLDING;
            motion->pace = 0.0F;
            evalflags_set_all(chaos2_stretch_hides, NJD_EVAL_HIDE);
            chaos->figures |= FIGURE_STRETCH;
            work->submode = 4;
            chaos_effect_splash(work, 10, 0.5F, 2.0F);
            arm_scale = reach_scale = hand_scale = 1.0F;
            aim_yaw = aim_pitch = 0;
            sound_play_at(SE_STRIKE, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        break;
    case 4:
        /* @hack spelling: the record by its motion, which starts it, so the
         * call passes motion's R13, not chaos's R12 (MOV R13,R5 at 0x0c900ea2);
         * with chaos, 4 of 582 words differ */
        chaos_face_player(work, motion);
        motion->pace += chaos_params->stretch_speed * 0.125F;
        arm_grow(&motion->pace);
        if (motion->pace >= 1.0F) {
            if (njDistanceP2P(&work->pos, &player_work[0]->pos) > PUNCH_REACH) {
                motion->pace = 0.0F;
                work->submode = 5;
                sound_play_at(SE_REACH, 0, 0, 0, work->pos.x, work->pos.y,
                              work->pos.z);
            } else {
                motion->pace = 1.0F;
                work->submode = 7;
                work->collision->shapes[SHAPE_FIST_10].levels =
                    (work->collision->shapes[SHAPE_FIST_10].levels &
                     ~SHAPE_ATTACK) |
                    FIST_REST;
            }
        }
        break;
    case 5:
        if (motion->pace < AIM_UNTIL) {
            aim_target = player->pos;
            chaos->reach = njDistanceP2P(&boss_points[4].pos, &aim_target);
        }
        motion->pace += chaos_params->stretch_speed / chaos->reach;
        stretch_aim(work, chaos, motion);
        stretch_scale(chaos, &motion->pace);
        if (motion->pace >= 1.0F) {
            motion->pace = 1.0F;
            work->collision->shapes[SHAPE_FIST_10].levels =
                (work->collision->shapes[SHAPE_FIST_10].levels &
                 ~SHAPE_ATTACK) |
                FIST_REST;
            work->submode = 6;
        }
        break;
    case 6:
        motion->pace -= chaos_params->stretch_speed * (1.0F / 15.0F);
        aim_yaw = angle_approach(aim_yaw, 0, AIM_TURN);
        aim_pitch = angle_approach(aim_pitch, 0, AIM_TURN);
        stretch_scale(chaos, &motion->pace);
        if (motion->pace <= 0.0F) {
            aim_yaw = 0;
            motion->pace = 1.0F;
            work->submode = 7;
        }
        break;
    case 7:
        motion->pace -= chaos_params->stretch_speed * (1.0F / 15.0F);
        arm_grow(&motion->pace);
        if (motion->pace <= 0.0F) {
            motion->wanted = ACTION_STRETCHED;
            evalflags_clear_all(chaos2_stretch_hides, NJD_EVAL_HIDE);
            chaos->figures &= ~FIGURE_STRETCH;
            CHAOS2_VARS(work)->timer = REST_TIME;
            work->submode = 8;
        }
        break;
    case 8:
        if (CHAOS2_VARS(work)->timer-- == 0) {
            motion_flags &= ~(MOTION_ENDED | MOTION_HOLDING);
            work->submode = 9;
        }
        break;
    case 9:
        if (motion_flags & MOTION_ENDED) {
            if (boss_repeats > 0) {
                work->submode = 1;
            } else {
                if (boss_return_mode != work->mode) {
                    boss_next_mode = boss_return_mode;
                    boss_last_mode = work->mode;
                }
                work->submode = MODE_DONE;
            }
        }
        break;
    }
    if (work->submode >= 4 && work->submode <= 5) {
        chaos_effect_trail(&boss_points[8].pos, &boss_points[8].last, 2);
    }
    (&chaos2_arm_node)->scl[0] = arm_scale;
    (&chaos2_stretch_arm)->scl[0] = reach_scale;
    (&chaos2_stretch_hand)->scl[0] = hand_scale;
    if (work->submode == MODE_DONE) {
        motion_flags &= ~MOTION_HOLDING;
        evalflags_clear_all(chaos2_arm_nodes, NJD_EVAL_HIDE);
        chaos->figures &= ~FIGURE_STRETCH;
        (&chaos2_arm_node)->evalflags |= NJD_EVAL_UNIT_SCL;
        (&chaos2_stretch_arm)->evalflags |= NJD_EVAL_UNIT_SCL;
        (&chaos2_stretch_hand)->evalflags |= NJD_EVAL_UNIT_SCL;
        (&chaos2_arm_node)->scl[0] = 1.0F;
        (&chaos2_stretch_arm)->scl[0] = 1.0F;
        (&chaos2_stretch_hand)->scl[0] = 1.0F;
    }
}

/* he punches with the fist on player 0's side, as many times as
 * boss_repeats says, half the time after a feint. with player 0 beyond
 * PUNCH_REACH, he asks for BOSS_STRETCH instead */
/* @hack spelling: extra, the record again, which chaos2_run_mode passes as
 * chaos too. SHC keeps chaos in R12 and extra in R11 (MOV R5,R12 and MOV
 * R6,R11 at 0x0c9011de and 0x0c9011e2); with chaos alone, the code is 32
 * bytes shorter and 313 of 320 words differ */
static void chaos2_punch(struct task_work *work, struct chaos *chaos,
                         struct chaos *extra)
{
    struct task_work *player = player_work[0];
    NJS_VECTOR direction;
    Sint8 *which;
    int point;
    Angle ang;
    struct collision_shape *hit;

    /* @hack redundant: a never-run store of 0 to boss_repeats, which sends
     * fist's address to R10 ahead of boss_repeats' to R4 (0x0c9011e0,
     * 0x0c9011e8), and puts boss_repeats' pool word first; without it, the two
     * loads swap and 2 of 320 words differ */
    if (0) {
        boss_repeats = 0;
    }
    /* @hack spelling: fist through a pointer, which keeps fist's address in R10
     * and 1 in R9 (0x0c9011e0, 0x0c9011ee); with fist itself, 9 of 320 words
     * differ */
    which = &fist;
    switch (work->submode) {
    case 0:
        chaos->flags |= CHAOS2_BUSY;
        chaos2_set_shapes(work, SHAPES_BODY);
    case 1:
        if (njDistanceP2P(&work->pos, &player->pos) > PUNCH_REACH) {
            if (work->mode != BOSS_STRETCH) {
                boss_next_mode = BOSS_STRETCH;
                boss_last_mode = work->mode;
            }
            break;
        }
        if (rand() * RAND_SCALE <= 0.5F) {
            extra->motion.wanted = ACTION_FEINT;
            work->submode = 5;
        } else {
            work->submode = 2;
        }
        break;
    case 2:
        --boss_repeats;
        direction = player->pos;
        njSubVector(&direction, &work->pos);
        ang = (Angle)(atan2f(direction.z, direction.x) * ANGLE_PER_RADIAN);
        ang = angle_difference(ang, work->ang.y);
        if (ang > 0) {
            *which = 0;
            hit = &work->collision->shapes[SHAPE_FIST_6];
        } else {
            *which = 1;
            hit = &work->collision->shapes[SHAPE_FIST_10];
        }
        hit->levels = (hit->levels & ~SHAPE_ATTACK) | FIST_STRIKE;
        extra->motion.wanted =
            punch_actions[*which]; /* NOLINT(bugprone-signed-char-misuse) */
        work->submode = 3;
        sound_play_at(SE_WIND_UP, 0, 0, 0, work->pos.x, work->pos.y,
                      work->pos.z);
        /* @hack spelling: the clear written out as a store, for which SHC
         * keeps chaos in R12 and extra in R11, and fist's address in R10 and
         * 1 in R9 (0x0c9011de to 0x0c9011ee); with &=, both pairs swap and 26
         * of 320 words differ */
        chaos->flags = chaos->flags & ~CHAOS2_PUNCH_HEARD;
        break;
    case 3:
        chaos_face_player(work, extra);
        point = *which == 0 ? 9 : 8;
        chaos_effect_trail(&boss_points[point].pos, &boss_points[point].last,
                           2);
        if (extra->motion.current == ACTION_PUNCH_6 ||
            extra->motion.current == ACTION_PUNCH_10) {
            if (motion_flags & MOTION_ENDED) {
                motion_flags |= MOTION_HOLDING;
                CHAOS2_VARS(work)->timer = REST_TIME;
                work->submode = 6;
            }
        }
        if (!(chaos->flags & CHAOS2_PUNCH_HEARD)) {
            if (extra->motion.frame > STRIKE_FRAME) {
                chaos->flags |= CHAOS2_PUNCH_HEARD;
                sound_play_at(SE_STRIKE, 0, 0, 0, work->pos.x, work->pos.y,
                              work->pos.z);
            }
        }
        break;
    case 6:
        if (CHAOS2_VARS(work)->timer-- == 0) {
            motion_flags &= ~MOTION_HOLDING;
            work->submode = 4;
        }
        break;
    case 4:
        if (boss_repeats > 0) {
            work->submode = 1;
        } else {
            if (boss_return_mode != work->mode) {
                boss_next_mode = boss_return_mode;
                boss_last_mode = work->mode;
            }
            work->submode = MODE_DONE;
        }
        break;
    case 5:
        if (extra->motion.current == ACTION_FEINT) {
            if (motion_flags & MOTION_ENDED) {
                work->submode = 2;
            }
        }
        break;
    }
    motion_flags &= ~MOTION_ENDED;
    if (work->submode == MODE_DONE) {
        motion_flags &= ~MOTION_HOLDING;
        work->collision->shapes[SHAPE_FIST_6].levels =
            (work->collision->shapes[SHAPE_FIST_6].levels & ~SHAPE_ATTACK) |
            FIST_REST;
        work->collision->shapes[SHAPE_FIST_10].levels =
            (work->collision->shapes[SHAPE_FIST_10].levels & ~SHAPE_ATTACK) |
            FIST_REST;
        chaos->flags &= ~CHAOS2_BUSY;
    }
}

/* struck, he loses a point of health, throws out two emeralds and melts
 * into the puddle. he then gathers into the ball, and his last hit instead
 * leaves blobs and clears the stage */
static void chaos2_hit(struct task *task, struct task_motion *motion,
                       struct chaos *extra)
{
    struct task_work *work = task->work;
    /* @hack spelling: a second pointer to the record, for which SHC keeps the
     * record in R12 and in R13 (MOV R6,R12 and MOV R6,R13 at 0x0c901580 and
     * 0x0c901584); with extra alone, 184 of 325 words differ */
    struct chaos *chaos = extra;

    switch (work->submode) {
    case 0:
        chaos->flags |= CHAOS_NO_BEADS;
        chaos->flags |= CHAOS2_BUSY;
        brain_shown = 0;
        extra->motion.wanted = ACTION_HIT;
        hit_time = 0.0F;
        chaos->health -= 1.0F;
        chaos2_set_shapes(work, SHAPES_NONE);
        work->collision->shapes[SHAPE_CORE].radius = CORE_SIZE;
        work->collision->shapes[SHAPE_CORE].center.y = CORE_SIZE;
        chaos->blink = 1;
        sound_play_at(SE_HIT, 0, 0, 0, work->pos.x, work->pos.y, work->pos.z);
        splash_due = 1;
        work->submode = 1;
        break;
    case 1:
        if (extra->motion.current != ACTION_HIT) {
            break;
        }
        chaos_emerald_create(&boss_points[10].pos, 0);
        chaos_emerald_create(&boss_points[11].pos, 1);
        work->submode = 2;
        chaos->figures |= FIGURE_MELTING | FIGURE_PUDDLE;
        chaos->scale.x = chaos->scale.y = chaos->scale.z = 0.0F;
        chaos_effect_splash(work, 64, 1.5F, 0.0F);
        if (chaos->health <= 0.0F) {
            chaos_effect_splash(work, 15, 3.5F, 1.0F);
            chaos_effect_splash(work, 15, 2.5F, 2.0F);
            chaos_effect_splash(work, 15, 0.5F, 2.5F);
            /* blob_place doesn't read its second argument. the image passes
             * the record there (MOV R13,R5 at 0x0c9016d6) */
            blob_place(work, (int)chaos, 12);
        }
        body_deform->mode = DEFORM_MELT;
        break;
    case 2:
        hit_time += MELT_STEP;
        extra->motion.pace = njSqrt(hit_time) * MELT_PACE;
        work->collision->shapes[SHAPE_CORE].center.y =
            (1.0F - (extra->motion.pace * 2.0F)) * CORE_SIZE;
        if (hit_time >= MELT_END) {
            work->submode = MODE_DONE;
        }
        body_deform->scale = extra->motion.pace;
        chaos->scale.x = chaos->scale.y = chaos->scale.z =
            ((extra->motion.pace * 0.5F) + 0.5F) * chaos_params->full_scale;
        if (extra->motion.pace > 0.5F && splash_due) {
            sound_play_at(SE_SPLASH, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
            splash_due = 0;
        }
        break;
    /* @hack redundant: an empty case 3, which keeps the switch's CMP/EQ #3 at
     * 0x0c9015aa; without it, 59 of 325 words differ */
    case 3:
        break;
    }
    if (work->submode == MODE_DONE) {
        chaos->figures &= ~(FIGURE_MELTING | FIGURE_BODY);
        chaos2_set_shapes(work, SHAPES_PUDDLE);
        work->collision->shapes[SHAPE_PUDDLE].radius = chaos_params->full_scale;
        chaos->flags &= ~CHAOS2_BUSY;
        chaos->flags |= CHAOS2_STRUCK;
        chaos->blink = 0;
        body_deform->mode = DEFORM_RESTORE;
        deform_step(body_deform);
        if (chaos->health > 0.0F) {
            if (work->mode != BOSS_GATHER) {
                boss_next_mode = BOSS_GATHER;
                boss_last_mode = work->mode;
            }
            boss_return_mode = BOSS_CHASE;
        } else {
            stage_clear();
            task_kill(task);
        }
    }
}

/* he melts down into the puddle, which grows as he goes */
static void chaos2_melt(struct task_work *work, struct task_motion *motion,
                        struct chaos *extra)
{
    struct task_work *player = player_work[0];
    /* @hack spelling: a second pointer to the record, for which SHC keeps the
     * record in R12 and in R13 (MOV R6,R12 and MOV R6,R13 at 0x0c9018ec and
     * 0x0c90190c); with extra alone, 48 of 218 words differ */
    struct chaos *chaos = extra;

    switch (work->submode) {
    case 0:
        chaos->flags |= CHAOS2_BUSY | CHAOS_NO_BEADS;
        work->ang.y = (Angle)(atan2f(player->pos.z - work->pos.z,
                                     player->pos.x - work->pos.x) *
                              ANGLE_PER_RADIAN);
        extra->motion.wanted = ACTION_STAND;
        melt_time = 0.0F;
        work->submode = 1;
        break;
    case 1:
        if (extra->motion.current == ACTION_STAND) {
            chaos->figures =
                (Sint16)(FIGURE_MELTING | FIGURE_PUDDLE | FIGURE_BODY);
            body_deform->scale = 1.0F;
            chaos->scale.x = chaos->scale.y = chaos->scale.z = 0.0F;
            chaos2_set_shapes(work, SHAPES_MELTED);
            work->collision->shapes[SHAPE_CORE].radius = CORE_SIZE;
            work->collision->shapes[SHAPE_CORE].center.y = CORE_SIZE;
            body_deform->mode = DEFORM_MELT;
            work->submode = 2;
        }
        break;
    case 2:
        melt_time += MELT_STEP;
        extra->motion.pace = njSqrt(melt_time) * MELT_PACE;
        if (melt_time >= MELT_END) {
            if (boss_return_mode != work->mode) {
                boss_next_mode = boss_return_mode;
                boss_last_mode = work->mode;
            }
            work->submode = MODE_DONE;
        }
        body_deform->scale = extra->motion.pace;
        chaos->scale.x = chaos->scale.z = (extra->motion.pace + 2.0F) *
                                          chaos_params->full_scale *
                                          (1.0F / 3.0F);
        chaos->scale.y = extra->motion.pace * chaos_params->full_scale;
        work->collision->shapes[SHAPE_CORE].center.y =
            (1.0F - (extra->motion.pace * 2.0F)) * CORE_SIZE;
        work->collision->shapes[SHAPE_PUDDLE].radius =
            chaos->scale.x * PUDDLE_RADIUS;
        break;
    }
    if (work->submode == MODE_DONE) {
        chaos->flags &= ~CHAOS2_BUSY;
        chaos->figures &= ~(FIGURE_MELTING | FIGURE_BODY);
        body_deform->mode = DEFORM_RESTORE;
        deform_step(body_deform);
    }
    sound_play_at_for(SE_MELT, (int)work, 1, 0, work->pos.x, work->pos.y,
                      work->pos.z, 6);
}

/* he rises out of the puddle, which shrinks away. as the rise ends, he calls
 * his emeralds back, unless boss_waiting is set */
static void chaos2_form(struct task_work *work, struct task_motion *motion,
                        struct chaos *extra)
{
    struct task_work *player = player_work[0];
    /* @hack spelling: a second pointer to the record, for which SHC keeps the
     * record in R12 and in R13 (MOV R6,R12 and MOV R12,R13 at 0x0c901b60 and
     * 0x0c901b64); with extra alone, the code is 96 bytes shorter and 241 of
     * 242 words differ */
    struct chaos *chaos = extra;

    switch (work->submode) {
    case 0:
        chaos->flags |= CHAOS2_BUSY;
        work->ang.y = (Angle)(atan2f(player->pos.z - work->pos.z,
                                     player->pos.x - work->pos.x) *
                              ANGLE_PER_RADIAN);
        extra->motion.wanted = ACTION_STAND;
        form_time = 0.0F;
        work->submode = 1;
        break;
    case 1:
        if (extra->motion.current == ACTION_STAND) {
            chaos->figures =
                (Sint16)(FIGURE_MELTING | FIGURE_PUDDLE | FIGURE_BODY);
            body_deform->scale = 1.0F;
            chaos->scale.x = chaos->scale.y = chaos->scale.z = 0.0F;
            chaos2_set_shapes(work, SHAPES_MELTED);
            work->collision->shapes[SHAPE_CORE].radius = CORE_SIZE;
            work->collision->shapes[SHAPE_CORE].center.y = CORE_SIZE;
            body_deform->mode = DEFORM_MELT;
            work->submode = 2;
        }
        break;
    case 2:
        form_time += MELT_STEP;
        extra->motion.pace = 1.0F - (njSqrt(form_time) * MELT_PACE);
        if (form_time >= MELT_END) {
            if (boss_return_mode != work->mode) {
                boss_next_mode = boss_return_mode;
                boss_last_mode = work->mode;
            }
            work->submode = MODE_DONE;
        }
        body_deform->scale = extra->motion.pace;
        chaos->scale.x = chaos->scale.z = (extra->motion.pace + 2.0F) *
                                          chaos_params->full_scale *
                                          (1.0F / 3.0F);
        chaos->scale.y = extra->motion.pace * chaos_params->full_scale;
        work->collision->shapes[SHAPE_CORE].center.y =
            (1.0F - (extra->motion.pace * 2.0F)) * CORE_SIZE;
        work->collision->shapes[SHAPE_PUDDLE].radius =
            chaos->scale.x * PUDDLE_RADIUS;
        break;
    }
    if (work->submode == MODE_DONE) {
        chaos->flags &= ~CHAOS_NO_BEADS;
        chaos->flags &= ~CHAOS2_BUSY;
        chaos->figures &= ~(FIGURE_MELTING | FIGURE_PUDDLE);
        brain_shown = 1;
        if (!boss_waiting) {
            chaos_emerald_return_all();
        }
        body_deform->mode = DEFORM_RESTORE;
        deform_step(body_deform);
    }
    sound_play_at_for(SE_MELT, (int)work, 1, 0, work->pos.x, work->pos.y,
                      work->pos.z, 6);
}

/* the puddle gathers into a ball of water over GATHER_STEPS steps, then
 * he asks for BOSS_BOUNCE. until then, the gather draws the ball itself */
static void chaos2_gather(struct task_work *work, struct task_motion *motion,
                          struct chaos *extra)
{
    struct task_work *player = player_work[0];
    /* @hack spelling: a second pointer to the record, for which SHC keeps the
     * record in R12 and in R13 (MOV R6,R12 and MOV R6,R13 at 0x0c901e1c and
     * 0x0c901e20); with extra alone, 54 of 255 words differ */
    struct chaos *chaos = extra;
    Uint32 control;
    Angle ang;

    switch (work->submode) {
    case 0:
        chaos->flags |= CHAOS2_BUSY;
        work->ang.y = (Angle)(atan2f(player->pos.z - work->pos.z,
                                     player->pos.x - work->pos.x) *
                              ANGLE_PER_RADIAN);
        extra->motion.pace = 0.0F;
        chaos->figures |= FIGURE_PUDDLE;
        chaos2_set_shapes(work, SHAPES_MELTED);
        work->collision->shapes[SHAPE_CORE].radius = CORE_SIZE;
        work->collision->shapes[SHAPE_CORE].center.y = CORE_SIZE;
        sound_play_at(SE_RISE, 0, 0, 0, work->pos.x, work->pos.y, work->pos.z);
        work->submode = 2;
    case 2:
        if ((extra->motion.pace += 1.0F / GATHER_STEPS) >= 1.0F) {
            if (work->mode != BOSS_BOUNCE) {
                boss_next_mode = BOSS_BOUNCE;
                boss_last_mode = work->mode;
            }
            work->submode = MODE_DONE;
        }
        chaos->scale.x = chaos->scale.y = chaos->scale.z =
            (1.0F - extra->motion.pace) * chaos_params->full_scale;
        work->collision->shapes[SHAPE_CORE].center.y =
            (1.0F - (extra->motion.pace * 2.0F)) * CORE_SIZE;
        work->collision->shapes[SHAPE_PUDDLE].radius =
            chaos->scale.x * PUDDLE_RADIUS;
        break;
    }
    if (work->submode == MODE_DONE) {
        chaos->figures &= ~FIGURE_PUDDLE;
        chaos->figures |= FIGURE_BALL;
        return;
    }
    /* the ball grows up out of the ground, raised by its radius as it grows */
    njPushMatrix(NULL);
    control = _nj_control_3d_flag_;
    njControl3D(_nj_control_3d_flag_ | NJD_CONTROL_3D_OFFSET_MATERIAL);
    njSetTexture(chaos->texlist);
    njSetConstantMaterial(&ball_color);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, -work->ang.y + 0x8000, ang);
    njSetConstantMaterial(&chaos->colors[0]);
    njTranslate(NULL, 0.0F, extra->motion.pace * BALL_RADIUS, 0.0F);
    njScale(NULL, extra->motion.pace, extra->motion.pace, extra->motion.pace);
    draw_tree(&chaos2_ball_object);
    ball_color.a = 0.0F;
    njSetConstantMaterial(&ball_color);
    njControl3D(control);
    njPopMatrix(1);
}

/* he sinks into the puddle and spreads it over the floor. three times in
 * ten he spreads it twice, and otherwise once. blights rise from it as it
 * spreads, and each spread leaves a dark spot as it draws back. he then rises
 * again, and asks for boss_return_mode */
static void chaos2_spread(struct task *task, struct task_motion *motion,
                          struct chaos *extra)
{
    struct task_work *work = task->work;
    /* @hack spelling: a second pointer to the record, for which SHC keeps the
     * record in R13 and in R12 (MOV R6,R13 and MOV R6,R12 at 0x0c902100 and
     * 0x0c902104); with extra alone, 255 of 481 words differ */
    struct chaos *chaos = extra;
    struct task *child;

    switch (work->submode) {
    case 0:
        chaos->flags |= CHAOS2_BUSY;
        chaos->flags |= CHAOS_NO_BEADS;
        extra->motion.wanted = ACTION_STAND;
        spread_time = 0.0F;
        chaos->figures |= FIGURE_MELTING | FIGURE_PUDDLE;
        boss_repeats = rand() * RAND_SCALE > 0.3F ? 1 : 2;
        chaos2_set_shapes(work, SHAPES_MELTED);
        work->collision->shapes[SHAPE_CORE].radius = CORE_SIZE;
        work->collision->shapes[SHAPE_CORE].center.y = CORE_SIZE;
        chaos_emerald_create(&boss_points[10].pos, 0);
        chaos_emerald_create(&boss_points[11].pos, 1);
        sound_play_at(SE_SPLASH, 0, 0, 0, work->pos.x, work->pos.y,
                      work->pos.z);
        body_deform->mode = DEFORM_MELT;
        brain_shown = 0;
        work->submode = 1;
    case 1:
        /* 10000 / 60 divides as integers, to 166 a step. chaos2_hit,
         * chaos2_melt and chaos2_form add MELT_STEP, 166 and two thirds */
        spread_time += 10000 / 60; /* NOLINT(bugprone-integer-division) */
        if (spread_time >= MELT_END) {
            chaos->figures &= ~FIGURE_BODY;
            work->submode = 2;
        }
        break;
    case 2:
        extra->motion.pace = 0.0F;
        spread_blight_wait = 0;
        sound_play_at(SE_SPREAD, 0, 0, 0, work->pos.x, work->pos.y,
                      work->pos.z);
        work->submode = 3;
    case 3:
        if ((extra->motion.pace += 1.0F / SPREAD_STEPS) >= 1.0F) {
            extra->motion.pace = 1.0F;
            CHAOS2_VARS(work)->timer = SPREAD_HOLD;
            work->submode = 4;
        }
        break;
    case 4:
        --CHAOS2_VARS(work)->timer;
        if (CHAOS2_VARS(work)->timer == 0) {
            work->submode = 5;
            task_create(TASK_WORK, 2, spread_spot_step);
            sound_play_at(SE_SPREAD, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        break;
    case 5:
        extra->motion.pace -= 1.0F / SPREAD_STEPS;
        if (extra->motion.pace <= 0.0F) {
            extra->motion.pace = 0.0F;
            work->submode = 6;
        }
        break;
    case 6:
        if (--boss_repeats > 0) {
            work->submode = 2;
        } else {
            spread_time = MELT_END;
            chaos->figures |= FIGURE_BODY;
            work->submode = 7;
            sound_play_at(SE_RISE, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        break;
    case 7:
        chaos_face_player(work, extra);
        spread_time -= 10000 / 60; /* NOLINT(bugprone-integer-division) */
        if (spread_time < 0.0F) {
            if (boss_return_mode != work->mode) {
                boss_next_mode = boss_return_mode;
                boss_last_mode = work->mode;
            }
            work->submode = MODE_DONE;
        }
        break;
    }
    switch (work->submode) {
    case 3:
    case 5:
        chaos->scale.x = chaos->scale.z =
            (chaos_params->full_scale * (1.0F - extra->motion.pace)) +
            (extra->motion.pace * SPREAD_SCALE);
        chaos->scale.y =
            (1.0F - (extra->motion.pace * 0.5F)) * chaos_params->full_scale;
        work->collision->shapes[SHAPE_PUDDLE].radius =
            extra->motion.pace * SPREAD_RADIUS;
        break;
    case 1:
    case 7:
        extra->motion.pace = njSqrt(spread_time) * MELT_PACE;
        body_deform->scale = extra->motion.pace;
        chaos->scale.x = chaos->scale.y = chaos->scale.z =
            ((extra->motion.pace * 0.5F) + 0.5F) * chaos_params->full_scale;
        work->collision->shapes[SHAPE_CORE].center.y =
            (1.0F - (extra->motion.pace * 2.0F)) * CORE_SIZE;
        work->collision->shapes[SHAPE_PUDDLE].radius =
            extra->motion.pace * chaos_params->full_scale * PUDDLE_RADIUS;
        break;
    }
    /* @hack spelling: the puddle through its address, which SHC pools to read
     * the model at offset 4 (MOV.L @(4,R1),R3 at 0x0c9024d8); through
     * chaos_puddle.model, 3 of 481 words and a pool word differ */
    (&chaos_puddle)->model->r = chaos->scale.x * PUDDLE_RADIUS;
    if ((work->submode == 3 || work->submode == 4) &&
        --spread_blight_wait < 0) {
        spread_blight_wait = BLIGHT_WAIT;
        child = chaos_create_child(TASK_MOTION | TASK_WORK, chaos2_blight_step,
                                   task, NULL);
        if (child) {
            child->work->scl.x = child->work->scl.z = 1.5F;
            child->work->scl.y = 1.0F;
        }
    }
    if (work->submode == MODE_DONE) {
        brain_shown = 1;
        chaos_emerald_return_all();
        chaos->figures &= ~(FIGURE_MELTING | FIGURE_PUDDLE);
        chaos->colors[0].a = 0.0F;
        body_deform->mode = DEFORM_RESTORE;
        deform_step(body_deform);
    }
}

/* with player 0 within PUNCH_REACH, he leaps away to the far side of the
 * hall. he then asks for BOSS_CHASE */
static void chaos2_leap(struct task_work *work, struct task_motion *motion,
                        struct chaos *extra)
{
    /* @hack spelling: a second pointer to the record, for which SHC keeps the
     * record in R12 and in a word of its frame (MOV R6,R12 and MOV.L
     * R12,@(4,R15) at 0x0c9025f2 and 0x0c9025f6); with extra alone, 151 of
     * 232 words differ */
    struct chaos *chaos = extra;
    struct task_work *player = player_work[0];
    Angle ang;
    Float scale;

    switch (work->submode) {
    case 0:
        if (njDistanceP2P(&work->pos, &player_work[0]->pos) > PUNCH_REACH) {
            work->submode = MODE_DONE;
            break;
        }
        chaos->flags |= CHAOS2_BUSY;
        extra->motion.wanted = ACTION_LEAP;
        work->submode = 1;
    case 1:
        if (extra->motion.current != ACTION_LEAP) {
            break;
        }
        if (extra->motion.frame <= LEAP_FRAME) {
            break;
        }
        ang = (Angle)(atan2f(player->pos.z, player->pos.x) * ANGLE_PER_RADIAN) +
              0x8000;
        ang +=
            (Angle)((LEAP_SPREAD - (rand() * (2 * LEAP_SPREAD * RAND_SCALE))) *
                    ANGLE_PER_DEGREE);
        motion->speed.x = njCos(ang) * LEAP_REACH;
        motion->speed.z = njSin(ang) * LEAP_REACH;
        njSubVector(&motion->speed, &work->pos);
        /* the jump peaks LEAP_HEIGHT up. its speed across covers the way in
         * 2 * speed.y / GRAVITY steps, the jump's time in the air */
        motion->speed.y = njSqrt(2.0F * LEAP_HEIGHT * GRAVITY);
        scale = GRAVITY / (motion->speed.y * 2.0F);
        motion->speed.x *= scale;
        motion->speed.z *= scale;
        work->submode = 2;
        sound_play_at(SE_LEAP, 0, 0, 0, work->pos.x, work->pos.y, work->pos.z);
        break;
    case 2:
        chaos_face_player(work, extra);
        motion->speed.y -= GRAVITY;
        njAddVector(&work->pos, &motion->speed);
        if (work->pos.y <= 0.0F) {
            work->pos.y = 0.0F;
            chaos_effect_splash(work, 15, 1.5F, 0.0F);
            extra->motion.wanted = ACTION_LAND;
            motion_flags &= ~MOTION_HOLDING;
            work->submode = 3;
        }
        break;
    case 3:
        if (motion_flags & MOTION_HOLDING) {
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        /* @hack spelling: a third pointer to the record, through which SHC
         * clears CHAOS2_BUSY in R4 (MOV R12,R4 at 0x0c90281e); with chaos,
         * 100 of 232 words differ */
        struct chaos *record = extra;

        record->flags &= ~CHAOS2_BUSY;
        motion_flags &= ~MOTION_HOLDING;
        if (work->mode != BOSS_CHASE) {
            boss_next_mode = BOSS_CHASE;
            boss_last_mode = work->mode;
        }
    }
}

/* set the alpha of the first material of each object in objects, a list a
 * null ends, to percent of opaque. the material takes NJD_FLAG_USE_ALPHA,
 * and loses NJD_FLAG_USE_ENV and NJD_FLAG_IGNORE_SPECULAR. an object without
 * a model is passed over */
static void objects_set_alpha(NJS_OBJECT **objects, int percent)
{
    NJS_MATERIAL *material;
    NJS_OBJECT *object;
    /* @hack redundant: register on alpha, which keeps it in R6 and the masks
     * in R5, R7 and R14 (0x0c902862 to 0x0c90286a); without it, 19 of 37
     * words differ */
    register int alpha = percent * 255 / 100;

    for (; (object = *objects) != NULL; objects++) {
        if (object->model) {
            material = object->model->mats;
            material->attrflags |= NJD_FLAG_USE_ALPHA;
            material->attrflags &= ~NJD_FLAG_USE_ENV;
            material->attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
            /* @hack redundant: a second store of NJD_FLAG_USE_ALPHA (OR R5,R3
             * at 0x0c902888); without it, 14 of 37 words differ */
            material->attrflags |= NJD_FLAG_USE_ALPHA;
            material->diffuse.argb.a = alpha;
        }
    }
}

/* give chaos2_alpha_nodes, alpha_objects and the stretched arm's nodes his
 * form's alpha, and hide chaos2_hidden_nodes */
static void chaos2_setup_nodes(void)
{
    NJS_OBJECT *objects[100];

    objects_set_alpha(chaos2_alpha_nodes, chaos_params->alpha);
    objects_set_alpha(alpha_objects, chaos_params->alpha);
    chaos_list_nodes(&chaos2_stretch_object, objects);
    objects_set_alpha(objects, chaos_params->alpha);
    evalflags_set_all(chaos2_hidden_nodes, NJD_EVAL_HIDE);
}

/* when a shape of his is hurt, ask for BOSS_HIT, set boss_return_mode to
 * BOSS_FORM and turn his shapes to SHAPES_MELTED. otherwise a touch throws
 * player 0 off his body */
static void chaos2_take_hits(struct task_work *work)
{
    if (work->flags & COLLISION_DAMAGED) {
        work->flags &= ~COLLISION_DAMAGED;
        if (work->mode != BOSS_HIT) {
            boss_next_mode = BOSS_HIT;
            boss_last_mode = work->mode;
        }
        chaos2_set_shapes(work, SHAPES_MELTED);
        boss_return_mode = BOSS_FORM;
        sound_play_at(SE_HIT, 0, 0, 0, work->pos.x, work->pos.y, work->pos.z);
    } else if (work->flags & COLLISION_TOUCHING) {
        /* @bug logic: player_turn_to_world turns body_push itself
         * (MOV.L at 0x0c9029c8 passes its address in R5), so each throw after
         * the first turns the last throw's push again. the barrier turns a
         * fresh copy of the same push */
        player_turn_to_world(player_work[0], &body_push);
        /* @hack type: y and z read through byte offsets into body_push, so
         * each address loads from its own pool word (0x0c9029ce, 0x0c9029d2);
         * through the members, the function runs 8 bytes short and 55 of 79
         * words differ */
        player_set_speed(0, body_push.x,
                         *(Float *)((char *)&body_push + sizeof(Float)),
                         *(Float *)((char *)&body_push + (2 * sizeof(Float))));
        sound_play_at(SE_THROWN, 0, 0, 0, work->pos.x, work->pos.y,
                      work->pos.z);
        vibration_start_fading(0, 7, 59, 6);
    }
}

/* run his work's mode for a step. the chase clears CHAOS2_BUSY, and most
 * other modes set it */
static void chaos2_run_mode(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct chaos *chaos = task->extra;

    switch (work->mode) {
    case BOSS_START:
        chaos->flags |= CHAOS2_BUSY;
        chaos2_set_shapes(work, SHAPES_NONE);
        if (camera_swing_done) {
            life_gauge_create(0, 0, chaos_params->hits);
            camera_override(chaos2_camera_step, CAMERA_ADJUST_NONE,
                            CAMERA_AT_ANGLES);
            if (work->mode != BOSS_TO_BALL) {
                boss_next_mode = BOSS_TO_BALL;
                boss_last_mode = work->mode;
            }
            boss_return_mode = BOSS_BOUNCE;
            time_start();
            pad_enable_player(0);
            hint_task = task_create(TASK_WORK, 2, chaos2_hint_step);
        }
        break;
    case BOSS_CHASE:
        chaos->flags &= ~CHAOS2_BUSY;
        chaos2_chase(work, motion, chaos);
        chaos2_take_hits(work);
        break;
    case BOSS_HIT:
        chaos->flags |= CHAOS2_BUSY;
        chaos2_hit(task, motion, chaos);
        break;
    case BOSS_GATHER:
        chaos->flags |= CHAOS2_BUSY;
        chaos2_gather(work, motion, chaos);
        break;
    case BOSS_MODE_5:
        break;
    case BOSS_MODE_6:
        chaos->flags |= CHAOS2_BUSY;
        break;
    case BOSS_LEAP:
        chaos->flags |= CHAOS2_BUSY;
        chaos2_leap(work, motion, chaos);
        chaos2_take_hits(work);
        break;
    case BOSS_MELT:
        chaos2_melt(work, motion, chaos);
        break;
    case BOSS_FORM:
        chaos2_form(work, motion, chaos);
        break;
    case BOSS_TO_BALL:
    case BOSS_FROM_BALL:
        chaos->flags |= CHAOS2_BUSY;
        chaos2_round(work, chaos, chaos);
        break;
    case BOSS_STRETCH:
        chaos->flags |= CHAOS2_BUSY;
        chaos2_stretch(work, chaos, &chaos->motion);
        chaos2_take_hits(work);
        break;
    case BOSS_PUNCH:
        chaos->flags |= CHAOS2_BUSY;
        chaos2_punch(work, chaos, chaos);
        chaos2_take_hits(work);
        break;
    case BOSS_BOUNCE:
        chaos->flags |= CHAOS2_BUSY;
        chaos2_bounce(task, motion, chaos);
        break;
    case BOSS_SPREAD:
        chaos->flags |= CHAOS2_BUSY;
        chaos2_spread(task, motion, chaos);
        break;
    case BOSS_BURST:
        chaos->flags |= CHAOS2_BUSY;
        chaos2_burst(task, work, chaos, chaos);
        break;
    case BOSS_BARRIER:
        chaos2_barrier(work, chaos);
        break;
    case BOSS_WAIT:
    case BOSS_MODE_19:
        chaos2_set_shapes(work, SHAPES_NONE);
        break;
    }
}

/* ask for BOSS_BARRIER against player 0 curled up or punching as Knuckles,
 * within BARRIER_REACH and within BARRIER_FRONT of his facing. CHAOS2_BUSY
 * or CHAOS2_BARRIER in his flags stops it. returns TRUE where it finds such
 * a player, and otherwise FALSE */
static Bool chaos2_ask_barrier(struct task_work *work, struct chaos *extra)
{
    struct task_work *player = player_work[0];
    /* @hack spelling: a copy of extra, for which SHC moves work into R14 and
     * the record into R4 ahead of the first test (MOV R4,R14 and MOV R5,R4 at
     * 0x0c902d22 and 0x0c902d24); with extra alone, 73 of 74 words differ */
    struct chaos *chaos = extra;
    Angle ang;

    if (chaos->flags & (CHAOS2_BUSY | CHAOS2_BARRIER)) {
        return FALSE;
    }
    if (!(player->flags & PLAYER_BALL) &&
        knuckles_get_activity(0) != KNUCKLES_PUNCHING) {
        return FALSE;
    }
    if (njDistanceP2P(&work->pos, &player_work[0]->pos) > BARRIER_REACH) {
        return FALSE;
    }
    ang = angle_difference((Angle)(atan2f(player->pos.z - work->pos.z,
                                          player->pos.x - work->pos.x) *
                                   ANGLE_PER_RADIAN),
                           work->ang.y);
    if ((ang < 0 ? -ang : ang) <= BARRIER_FRONT) {
        /* @hack redundant: a jump to the next line, which loads
         * boss_last_mode's address into R0 first, BOSS_BARRIER into R2 and
         * boss_next_mode's address into R3 (0x0c902d9a to 0x0c902d9e); without
         * it, 6 of 74 words differ */
        goto barrier;
    barrier:
        if (work->mode != BOSS_BARRIER) {
            boss_next_mode = BOSS_BARRIER;
            boss_last_mode = work->mode;
        }
        return TRUE;
    }
    return FALSE;
}

/* TRUE when an attack is due, and otherwise FALSE. only a call in
 * BOSS_CHASE, with no other mode asked for, counts his record's
 * attack_timer down by 1. a call that brings it to 0 or below finds an
 * attack due. it sets the timer to attack_wait plus a random count below
 * attack_wait_random */
static Bool chaos2_attack_due(struct task_work *work, struct chaos *extra)
{
    /* @hack spelling: a copy of extra, which SHC keeps in R14 (MOV R5,R14 at
     * 0x0c902e0c); with extra alone, the record goes to the frame and 46 of
     * 49 words differ */
    struct chaos *chaos = extra;

    if (work->mode != BOSS_CHASE || boss_next_mode != BOSS_CHASE) {
        return FALSE;
    }
    if (--chaos->attack_timer > 0) {
        return FALSE;
    }
    chaos->attack_timer =
        chaos_params->attack_wait +
        (int)((Float)rand() * chaos_params->attack_wait_random * RAND_SCALE);
    return TRUE;
}

/* run after each step's mode while the fight is on. unless
 * chaos2_ask_barrier asks for his barrier, he picks an attack at random
 * once chaos2_attack_due finds one due. his health sets the choices. motion
 * is unread */
static void chaos2_choose_attack(struct task_work *work,
                                 struct task_motion *motion,
                                 struct chaos *extra)
{
    /* @hack spelling: a second pointer to the record, for which SHC keeps the
     * record in R13 and in a word of its frame (MOV R6,R13 and MOV.L R13,@R15
     * at 0x0c902eb2 and 0x0c902eb6); with extra alone, 145 of 175 words
     * differ */
    struct chaos *chaos = extra;
    Float choice;

    if (chaos2_ask_barrier(work, extra)) {
        return;
    }
    if (!chaos2_attack_due(work, extra)) {
        return;
    }
    choice = rand() * RAND_SCALE;
    switch ((int)chaos->health) {
    case 4:
        if (choice > 0.75F) {
            if (work->mode != BOSS_TO_BALL) {
                boss_next_mode = BOSS_TO_BALL;
                boss_last_mode = work->mode;
            }
            boss_return_mode = BOSS_BOUNCE;
        } else {
            if (work->mode != BOSS_STRETCH) {
                boss_next_mode = BOSS_STRETCH;
                boss_last_mode = work->mode;
            }
            boss_repeats = rand() * RAND_SCALE < 0.5F ? 2 : 3;
            boss_return_mode = BOSS_CHASE;
        }
        break;
    case 3:
        if (choice > 0.6F) {
            if (work->mode != BOSS_SPREAD) {
                boss_next_mode = BOSS_SPREAD;
                boss_last_mode = work->mode;
            }
            boss_return_mode = BOSS_LEAP;
        } else {
            if (work->mode != BOSS_STRETCH) {
                boss_next_mode = BOSS_STRETCH;
                boss_last_mode = work->mode;
            }
            boss_repeats = rand() * RAND_SCALE < 0.5F ? 2 : 3;
            boss_return_mode = BOSS_LEAP;
        }
        break;
    case 1:
    case 2:
        if (choice > 0.8F) {
            if (work->mode != BOSS_SPREAD) {
                boss_next_mode = BOSS_SPREAD;
                boss_last_mode = work->mode;
            }
            boss_return_mode = BOSS_CHASE;
        } else if (choice > 0.6F) {
            if (work->mode != BOSS_TO_BALL) {
                boss_next_mode = BOSS_TO_BALL;
                boss_last_mode = work->mode;
            }
            boss_return_mode = BOSS_BURST;
        } else {
            if (work->mode != BOSS_STRETCH) {
                boss_next_mode = BOSS_STRETCH;
                boss_last_mode = work->mode;
            }
            boss_repeats = rand() * RAND_SCALE < 0.5F ? 2 : 3;
            boss_return_mode = BOSS_CHASE;
        }
        break;
    }
}

void chaos2_create(void)
{
    task_create(TASK_MOTION | TASK_WORK, 2, chaos2_boss_step);
}

/* begin the fight. he shows his name and the camera swings round him,
 * while player 0's pad and the clock stop until BOSS_START sees the swing
 * done. he also tethers player 0, and starts the boss music's wait */
static void chaos2_begin(struct task *task)
{
    struct chaos *chaos = task->extra;
    struct task_work *work = task->work;
    struct task *wait;

    chaos_face_off(work);
    boss_name_create(TEXT_1, -1, 240, 60);
    camera_set_boss_offsets(&camera_target_offset, &camera_place_offset);
    camera_start_swing(work, 10.0F, 30.0F, 240);
    camera_override_mode(CAMERA_MODE_CHAOS_STINIT, CAMERA_ADJUST_NONE);
    chaos2_set_shapes(work, SHAPES_NONE);
    time_stop();
    pad_disable_player(0);
    tether = chaos_tether_create(&player_work[0]->pos, &tether_center,
                                 tether_radius);
    stream_destroy();
    /* the music's wait asks for MUSIC_CHAOS on its 61st step */
    wait = task_create(TASK_WORK, 1, chaos2_music_wait_step);
    wait->work->mode = MUSIC_CHAOS;
    wait->work->timer = MUSIC_DELAY;
    camera_set_perspective(BOSS_PERSPECTIVE);
    boss_waiting = 0;
    chaos2_land_fade_on = 1;
    chaos->figures = FIGURE_BODY;
    brain_shown = 1;
    chaos->colors[0].a = 0.0F;
    chaos_draw_begin();
    motion_flags = 0;
    chaos->motion.wanted = ACTION_STAND;
    chaos->figures = FIGURE_BODY;
    chaos->flags &= ~CHAOS_NO_BEADS;
}

/* his destructor. it runs his mode once more in MODE_DONE, unless the mode
 * has set it. it then frees his ribbon, his light, his motion's blend and
 * welds and his deformer, and ends the hints' task and the tether */
static void chaos2_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos *chaos = task->extra;

    if (work->submode != MODE_DONE) {
        work->submode = MODE_DONE;
        chaos2_run_mode(task);
    }
    ribbon_free();
    chaos_light_delete();
    njSetMotionCallback(NULL);
    chaos->motion.state = MOTION_FREE;
    motion_step(work, &chaos->motion);
    chaos->motion.weld_mode = PLAYER_WELD_FREE;
    motion_weld(work, &chaos->motion);
    deform_free(body_deform);
    boss_waiting = 0;
    if (hint_task) {
        task_destroy(hint_task);
    }
    if (tether) {
        task_kill(tether);
    }
}

/* his task's step. the first sets up his record and his figure. while
 * boss_waiting is set, a step that finds no event in control begins the
 * fight. each step runs his mode, and his choice of attack once the fight
 * is on. it then runs his motion, points, draw, shapes and shadow. while no
 * event has control, it keeps player 0 from going below y 0 */
static void chaos2_boss_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct chaos *chaos = task->extra;
    /* @hack spelling: a second pointer to the record, read from task->extra
     * again once the setup has set it, for which SHC keeps the record in R11
     * and in R12 (MOV R4,R11 and MOV R4,R12 at 0x0c9032c0 and 0x0c9032c2);
     * with chaos alone, 168 of 336 words differ */
    struct chaos *record = task->extra;

    switch (work->mode) {
    case BOSS_SETUP:
        hint_task = NULL;
        chaos = (struct chaos *)motion_create(work, sizeof(struct chaos));
        if (!chaos) {
            return;
        }
        collision_init(task, chaos2_shapes, COLLISION_ENEMY, 1,
                       chaos2_shape_count);
        /* @hack spelling: work through the task, which loads task->work again
         * (MOV.L @(32,R13),R3 at 0x0c9032f8); through work, 93 of 336 words
         * differ */
        task->work->collision->radius = COLLISION_REACH;
        chaos2_set_shapes(work, SHAPES_NONE);
        chaos->motion.wanted = ACTION_STAND;
        chaos->motion.current = ACTION_STAND;
        chaos->motion.actions = chaos2_actions;
        chaos->motion.welds = chaos2_welds;
        work->mode = BOSS_START;
        task->extra = chaos;
        record = task->extra;
        boss_work = work;
        chaos_record = record;
        task->draw = chaos2_draw;
        task->destructor = chaos2_destroy;
        ribbon_init(work, record, &chaos2_ribbon);
        chaos_setup(work, record, CHAOS_2);
        chaos_light_create(LIGHT_RANGE);
        chaos2_setup_nodes();
        work->mode = BOSS_WAIT;
        boss_waiting = 1;
        motion_step(work, &record->motion);
        record->attack_timer = chaos_params->attack_wait;
        record->texlist = &chaos2_texlist;
        /* @hack spelling: each node through its address, which SHC pools to
         * read the model at offset 4 (MOV.L @(4,R3),R2 at 0x0c903376); through
         * the nodes, 23 of 336 words differ */
        (&dat_0c908664)->model->mats->attrflags |= MATERIAL_BLINKS;
        (&dat_0c90c388)->model->mats->attrflags |= MATERIAL_BLINKS;
        motion_weld(work, &record->motion);
        body_deform = deform_create(&chaos2_body_object, work);
        chaos_effect_pulse(&chaos2_pulse_node, task);
        brain_create(task, 1.5F, &boss_points[12].pos, &chaos2_brain_offset);
        break;
    default:
        chaos2_run_mode(task);
        break;
    }
    if (!boss_waiting) {
        chaos2_choose_attack(work, motion, chaos);
    } else {
        brain_shown = 0;
        if (record->steps > 1 && !event_in_control()) {
            if (work->submode != MODE_DONE) {
                work->submode = MODE_DONE;
                chaos2_run_mode(task);
            }
            chaos2_begin(task);
            if (work->mode != BOSS_START) {
                boss_next_mode = BOSS_START;
                boss_last_mode = work->mode;
            }
        }
    }
    if (task->step == task_destroy) {
        return;
    }
    arm_beads_hide(work->mode == BOSS_STRETCH);
    if (work->mode != boss_next_mode) {
        if (work->submode != MODE_DONE) {
            work->submode = MODE_DONE;
            chaos2_run_mode(task);
        }
        boss_last_mode = work->mode;
        work->mode = boss_next_mode;
        work->submode = 0;
    }
    if (!(motion_flags & MOTION_HOLDING)) {
        motion_step(work, &chaos->motion);
    }
    motion_weld(work, &chaos->motion);
    chaos_track_points(work, chaos2_marks, chaos_params->model_y);
    if (record->figures & FIGURE_BODY) {
        ribbon_spawn();
    }
    pose_get_matrix(NODE_SHOULDER, &shoulder_matrix);
    chaos_animate_surface(chaos);
    chaos2_draw(task);
    chaos2_place_shapes(work);
    if (record->figures & (FIGURE_ROUND | FIGURE_BODY)) {
        chaos2_shadow_draw(&work->pos, SHADOW_SIZE);
        record->ground = 2.0F;
    } else if (record->figures & FIGURE_BALL) {
        chaos2_shadow_draw(&work->pos, record->scale.x * BALL_RADIUS);
    }
    record->steps++;
    life_gauge_health = record->health;
    collision_register(work);
    if (njDistanceP2P(&work->pos, &player_work[0]->pos) < NEAR_RANGE) {
        sound_play_at_for(SE_NEAR, (int)work, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 10);
    }
    task_step_children(task);
    if (!event_in_control()) {
        player_work[0]->pos.y =
            player_work[0]->pos.y > 0.0F ? player_work[0]->pos.y : 0.0F;
    }
}

/* draw the stretched arm from shoulder_matrix's origin, turned by aim_yaw
 * and aim_pitch. boss_points[4] moves onto the arm's node 16, [6] onto node
 * 4, [8] onto node 8, and [10] hand_reach out along node 14. world and
 * direction are found and never read, and record is unread */
static void stretch_draw(struct task_work *work, void *record)
{
    NJS_POINT3 origin = {0.0F, 0.0F, 0.0F};
    NJS_VECTOR axis = {1.0F, 0.0F, 0.0F};
    NJS_POINT3 shoulder;
    NJS_POINT3 world;
    NJS_VECTOR direction;
    Angle ang;

    njPushMatrix(&shoulder_matrix);
    njCalcPoint(NULL, &origin, &shoulder);
    njCalcVector(NULL, &axis, &direction);
    njPopMatrix(1);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, -work->ang.y + 0x8000, ang);
    njTranslate(NULL, 0.0F, chaos_params->model_y, 0.0F);
    njTranslateV(NULL, &shoulder);
    njCalcPoint(NULL, &origin, &world);
    TURN(njRotateY, aim_yaw);
    TURN(njRotateZ, aim_pitch);
    tree_calc_matrices(NULL, &chaos2_stretch_object, 0);
    tree_get_matrix(NULL, 16);
    njCalcPoint(NULL, &origin, &boss_points[4].pos);
    tree_get_matrix(NULL, 4);
    njCalcPoint(NULL, &origin, &boss_points[6].pos);
    tree_get_matrix(NULL, 8);
    njCalcPoint(NULL, &origin, &boss_points[8].pos);
    tree_get_matrix(NULL, 14);
    origin.x = hand_reach;
    njCalcPoint(NULL, &origin, &boss_points[10].pos);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslateV(NULL, &shoulder);
    TURN(njRotateY, aim_yaw);
    TURN(njRotateZ, aim_pitch);
    draw_tree(&chaos2_stretch_object);
    njPopMatrix(1);
}

/* his draw, with his record's first color as an offset material. it draws
 * the puddle, or his blobs once he is beaten, and then the ball. it then
 * draws his body or his round figure, with the stretched arm and the
 * barrier */
static void chaos2_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos *chaos = task->extra;
    NJS_ARGB glow;
    int current;
    Angle ang;
    NJS_ACTION *action;

    draw_set_palette(4);
    njSetMotionCallback(chaos2_set_material);
    chaos_light_place(&work->pos, -10.0F);
    draw_control_save();
    draw_control_clear(NJD_CONTROL_3D_CONSTANT_MATERIAL);
    draw_control_set(NJD_CONTROL_3D_OFFSET_MATERIAL);
    njSetTexture(chaos->texlist);
    current = chaos->motion.current;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    if (chaos->figures & FIGURE_PUDDLE) {
        if (work->mode != BOSS_BURST) {
            if (chaos->scale.y < PUDDLE_FADE) {
                chaos->colors[0].a =
                    (chaos->scale.y - PUDDLE_FADE) * (1.0F / PUDDLE_FADE);
            } else {
                chaos->colors[0].a = 0.0F;
            }
        }
        njSetConstantMaterial(&chaos->colors[0]);
        if (chaos->health > 0.0F) {
            njPushMatrix(NULL);
            njScaleV(NULL, &chaos->scale);
            draw_tree(&chaos_puddle);
            njPopMatrix(1);
        } else {
            blob_draw_spread(work, chaos, chaos->motion.pace);
        }
        chaos->colors[0].a = 0.0F;
    }
    if (chaos->figures & FIGURE_BALL) {
        njPushMatrix(NULL);
        njSetConstantMaterial(&chaos->colors[0]);
        njTranslate(NULL, 0.0F, BALL_RADIUS, 0.0F);
        njScaleV(NULL, &chaos->scale);
        draw_tree(&chaos2_ball_object);
        njPopMatrix(1);
    }
    if (chaos->figures & (FIGURE_ROUND | FIGURE_BODY)) {
        njPushMatrix(NULL);
        njSetConstantMaterial(&chaos->colors[0]);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, -work->ang.y + 0x8000, ang);
        njTranslate(NULL, 0.0F, chaos_params->model_y, 0.0F);
        if (chaos->figures & FIGURE_STRETCH) {
            stretch_draw(work, chaos);
        }
        if (chaos->figures & FIGURE_MELTING) {
            deform_step(body_deform);
            if (chaos->motion.pace > MELT_FADE) {
                chaos->colors[0].a = (MELT_FADE - chaos->motion.pace) *
                                     (1.0F / (1.0F - MELT_FADE));
            }
        }
        if (chaos->motion.state != MOTION_BLENDING) {
            action = chaos->motion.actions[current].action;
        } else {
            action = chaos->motion.blend;
        }
        njSetConstantMaterial(&chaos->colors[0]);
        if (chaos->figures & FIGURE_BODY) {
            ribbon_place_nodes();
            draw_action(action, chaos->motion.frame);
        }
        if (chaos->figures & FIGURE_ROUND) {
            draw_tree(chaos2_round_object);
        }
        if (chaos->flags & CHAOS2_BARRIER) {
            njPushMatrix(NULL);
            njTranslate(NULL, -8.0F, 5.0F, 0.0F);
            /* @bug uninitialized: no path writes glow.a. the stores at
             * 0x0c903bb6 to 0x0c903bbe fill R15+12 to R15+20, its red, green
             * and blue, and njSetConstantMaterial (JSR at 0x0c903bc0) reads
             * its alpha from R15+8, whatever the stack held there */
            glow.r = glow.g = glow.b = chaos->barrier_glow - 1.0F;
            njSetConstantMaterial(&glow);
            njSetTexture(&chaos2_barrier_texlist);
            njScale(NULL, 1.5F, 1.5F, 1.5F);
            njSimpleDrawObject(&chaos_barrier_object);
            njPopMatrix(1);
        }
        njPopMatrix(1);
    }
    njPopMatrix(1);
    draw_control_restore();
    njSetMotionCallback(NULL);
    draw_set_palette(0);
}

/* the motion callback chaos2_draw sets: chaos_set_material */
static void chaos2_set_material(NJS_OBJECT *object)
{
    chaos_set_material(object);
}

/* the dark spot a spread leaves on the ground under him, which fades in,
 * shrinks away and ends */
static void spread_spot_step(struct task *task)
{
    struct task_work *work = task->work;
    Float alpha;
    Float scale;

    switch (work->mode) {
    case SPOT_START:
        work->pos.x = boss_work->pos.x;
        work->pos.y = 2.0F;
        work->pos.z = boss_work->pos.z;
        SPOT(work)->steps = SPREAD_SPOT_FADE;
        SPOT(work)->alpha = 0.0F;
        SPOT(work)->size = 1.0F;
        work->mode = SPOT_FADE_IN;
    case SPOT_FADE_IN:
        --SPOT(work)->steps;
        SPOT(work)->alpha += 1.0F / SPREAD_SPOT_FADE;
        if (SPOT(work)->steps == 0) {
            work->mode = SPOT_SHRINK;
        }
        break;
    case SPOT_SHRINK:
        if ((SPOT(work)->size -= 1.0F / SPREAD_SPOT_SHRINK) <= 0.0F) {
            task_destroy(task);
            return;
        }
        break;
    }
    alpha = SPOT(work)->alpha;
    draw_set_material(alpha, alpha, alpha, alpha);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njPushMatrix(NULL);
    scale = SPOT(work)->size * SPREAD_SPOT_SIZE;
    njTranslateV(NULL, &work->pos);
    njRotateX(NULL, NJM_DEG_ANG(90));
    njScale(NULL, scale, scale, scale);
    njDrawSprite3D(&chaos_spot_sprite, 0, NJD_SPRITE_ALPHA);
    njPopMatrix(1);
    draw_set_material(1.0F, 0.0F, 0.0F, 0.0F);
}
