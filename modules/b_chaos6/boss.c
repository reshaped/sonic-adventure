/* Chaos 6, the boss: his modes, his attacks and how he draws */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos6/bomb.h"
#include "b_chaos6/boss.h"
#include "b_chaos6/change_shape.h"
#include "b_chaos6/chaos6.h"
#include "b_chaos6/debris.h"
#include "b_chaos6/defense.h"
#include "b_chaos6/eggman.h"
#include "b_chaos6/frog.h"
#include "b_chaos6/frozen_model.h"
#include "b_chaos6/hint.h"
#include "b_chaos6/inhale.h"
#include "b_chaos6/jets.h"
#include "b_chaos6/shockwave.h"
#include "b_chaos6/textures.h"
#include "camera/camera.h"
#include "chaos/chaos.h"
#include "chaos/deform.h"
#include "chaos/effect.h"
#include "chaos/puddle.h"
#include "chaos/steer.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/motion.h"
#include "draw/pose.h"
#include "draw/tree.h"
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
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 7
#include "b_chaos6/boss/text.inc"

/* in this file, a distance across is measured in x and z alone */
#pragma section HEAD
/* how far across chaos6_inhale_pull draws its work toward his mouth a step */
static Float chaos6_inhale_speed = {
#include "b_chaos6/boss/chaos6_inhale_speed.inc"
};
static Float dat_0c90024c = {
#include "b_chaos6/boss/dat_0c90024c.inc"
};
static Float dat_0c900250 = {
#include "b_chaos6/boss/dat_0c900250.inc"
};
/* how near his mouth, across, chaos6_inhale_pull catches its work */
static Float chaos6_inhale_reach = {
#include "b_chaos6/boss/chaos6_inhale_reach.inc"
};
static Float dat_0c900258 = {
#include "b_chaos6/boss/dat_0c900258.inc"
};
static Float dat_0c90025c = {
#include "b_chaos6/boss/dat_0c90025c.inc"
};
static Float dat_0c900260 = {
#include "b_chaos6/boss/dat_0c900260.inc"
};
static Float dat_0c900264 = {
#include "b_chaos6/boss/dat_0c900264.inc"
};
static Float dat_0c900268 = {
#include "b_chaos6/boss/dat_0c900268.inc"
};
static Float dat_0c90026c = {
#include "b_chaos6/boss/dat_0c90026c.inc"
};
static Float dat_0c900270 = {
#include "b_chaos6/boss/dat_0c900270.inc"
};
static Float dat_0c900274 = {
#include "b_chaos6/boss/dat_0c900274.inc"
};
static Float dat_0c900278 = {
#include "b_chaos6/boss/dat_0c900278.inc"
};
static Float dat_0c90027c = {
#include "b_chaos6/boss/dat_0c90027c.inc"
};
static Float dat_0c900280 = {
#include "b_chaos6/boss/dat_0c900280.inc"
};
/* the aiming node's x scale for each unit of player 0's distance across */
static Float chaos6_jets_aim_scale = {
#include "b_chaos6/boss/chaos6_jets_aim_scale.inc"
};
#pragma section
/* the node of his jets form that chaos6_fire_jets turns and stretches
 * toward player 0 */
extern NJS_OBJECT chaos6_jets_aim_object;
/* the roots of his three figures' trees: his body, his jets form, and the
 * change figure, which he passes through between them */
extern NJS_OBJECT chaos6_body_object;
extern NJS_OBJECT chaos6_jets_object;
extern NJS_OBJECT chaos6_change_object;
/* lists of nodes, each ended by a null. dat_0c93c220 and dat_0c93c284 hold
 * only their null. chaos6_body_fade_nodes and chaos6_jets_fade_nodes hold
 * the nodes of his body and of his jets form that fade as he changes form */
extern NJS_OBJECT *dat_0c93c220[];
extern NJS_OBJECT *dat_0c93c284[];
extern NJS_OBJECT *chaos6_body_fade_nodes[];
extern NJS_OBJECT *chaos6_jets_fade_nodes[];
/* the nodes the welds join. the number in each name is the node's index in
 * its form's tree */
extern NJS_OBJECT chaos6_body_node3;
extern NJS_OBJECT chaos6_body_node24;
extern NJS_OBJECT chaos6_body_node30;
extern NJS_OBJECT chaos6_body_node36;
extern NJS_OBJECT chaos6_body_node38;
extern NJS_OBJECT chaos6_body_node44;
extern NJS_OBJECT chaos6_body_node45;
extern NJS_OBJECT chaos6_body_node50;
extern NJS_OBJECT chaos6_body_node51;
extern NJS_OBJECT chaos6_body_node56;
extern NJS_OBJECT chaos6_body_node57;
extern NJS_OBJECT chaos6_body_node62;
extern NJS_OBJECT chaos6_body_node63;
extern NJS_OBJECT chaos6_body_node68;
extern NJS_OBJECT chaos6_body_node69;
extern NJS_OBJECT chaos6_body_node74;
extern NJS_OBJECT chaos6_body_node75;
extern NJS_OBJECT chaos6_body_node79;
extern NJS_OBJECT chaos6_body_node86;
extern NJS_OBJECT chaos6_body_node87;
extern NJS_OBJECT chaos6_body_node91;
extern NJS_OBJECT chaos6_body_node98;
extern NJS_OBJECT chaos6_body_node99;
extern NJS_OBJECT chaos6_body_node103;
extern NJS_OBJECT chaos6_body_node113;
extern NJS_OBJECT chaos6_body_node114;
extern NJS_OBJECT chaos6_body_node115;
extern NJS_OBJECT chaos6_body_node116;
extern NJS_OBJECT chaos6_jets_node1;
extern NJS_OBJECT chaos6_jets_node2;
extern NJS_OBJECT chaos6_jets_node6;
extern NJS_OBJECT chaos6_jets_node10;
extern NJS_OBJECT chaos6_jets_node14;
extern NJS_OBJECT chaos6_jets_node18;
extern NJS_OBJECT chaos6_jets_node22;
extern NJS_OBJECT chaos6_jets_node26;
extern NJS_OBJECT chaos6_jets_node35;
extern NJS_OBJECT chaos6_jets_node36;
extern NJS_OBJECT chaos6_jets_node37;
extern NJS_OBJECT chaos6_jets_node38;
extern NJS_OBJECT chaos6_jets_node39;
extern NJS_OBJECT chaos6_jets_node40;
extern NJS_OBJECT chaos6_jets_node44;
extern NJS_OBJECT chaos6_jets_node48;
extern NJS_OBJECT chaos6_jets_node52;
extern NJS_OBJECT chaos6_jets_node56;
extern NJS_OBJECT chaos6_jets_node60;
extern NJS_OBJECT chaos6_jets_node64;
extern NJS_OBJECT chaos6_jets_node65;
extern NJS_OBJECT chaos6_change_node1;
extern NJS_OBJECT chaos6_change_node2;
extern NJS_OBJECT chaos6_change_node3;
extern NJS_OBJECT chaos6_change_node4;
extern NJS_OBJECT chaos6_change_node5;
extern NJS_OBJECT chaos6_change_node6;
extern NJS_OBJECT chaos6_change_node7;
extern NJS_OBJECT chaos6_change_node8;
extern NJS_OBJECT chaos6_change_node23;
extern NJS_OBJECT chaos6_change_node24;
extern NJS_OBJECT chaos6_change_node25;
extern NJS_OBJECT chaos6_change_node26;
/* the actions body_actions plays on his body's tree */
extern NJS_ACTION dat_0c964a84;
extern NJS_ACTION dat_0c96e540;
extern NJS_ACTION dat_0c97148c;
extern NJS_ACTION dat_0c977d08;
extern NJS_ACTION dat_0c97e324;
extern NJS_ACTION dat_0c983c70;
extern NJS_ACTION dat_0c98cb4c;
extern NJS_ACTION dat_0c991098;
extern NJS_ACTION dat_0c9945a4;
extern NJS_ACTION dat_0c997b80;
extern NJS_ACTION dat_0c99b10c;
extern NJS_ACTION dat_0c99ef28;
extern NJS_ACTION dat_0c9a36a4;
extern NJS_ACTION dat_0c9a80a0;
extern NJS_ACTION dat_0c9b18cc;
extern NJS_ACTION dat_0c9b308c;
extern NJS_ACTION dat_0c9b87d8;
extern NJS_ACTION dat_0c9bcb14;
extern NJS_ACTION dat_0c9c4250;
extern NJS_ACTION dat_0c9c5a0c;
extern NJS_ACTION dat_0c9cb348;
extern NJS_ACTION dat_0c9cf6a4;
extern NJS_ACTION dat_0c9d6df0;
extern NJS_ACTION dat_0c9d857c;
extern NJS_ACTION dat_0c9dde68;
extern NJS_ACTION dat_0c9e20d4;
extern NJS_ACTION dat_0c9e9930;
extern NJS_ACTION dat_0c9eb0dc;
extern NJS_ACTION dat_0c9f0888;
extern NJS_ACTION dat_0c9f4b44;
extern NJS_ACTION dat_0c9fc100;
extern NJS_ACTION dat_0ca035dc;
extern NJS_ACTION dat_0ca0aa98;
extern NJS_ACTION dat_0ca0e7d4;
extern NJS_ACTION dat_0ca18a30;
extern NJS_ACTION dat_0ca1d41c;
/* the actions jets_actions plays: five on his jets form's tree, and
 * dat_0ca3a840 on the change figure's */
extern NJS_ACTION dat_0ca20604;
extern NJS_ACTION dat_0ca272b0;
extern NJS_ACTION dat_0ca2c16c;
extern NJS_ACTION dat_0ca30c68;
extern NJS_ACTION dat_0ca35b24;
extern NJS_ACTION dat_0ca3a840;

/* the submode a mode ends in. a mode that cleans up does so as it runs in
 * MODE_DONE. as he leaves a mode that hasn't reached it, his step runs the
 * mode once more in MODE_DONE, within the same step. a mode that run asks
 * for through boss_next_mode is the one he takes */
#define MODE_DONE (-1)

/* bits of his record's figures. each but FIGURE_JETS says what chaos6_draw
 * draws */
#define FIGURE_BODY 0x1 /* his form's tree, at the action playing */
#define FIGURE_PUDDLE 0x2 /* his puddle, or with no health left his blobs */
/* his form is his jets form: his step tracks jets_marks, and
 * chaos6_place_tip_shape follows boss_points[0] */
#define FIGURE_JETS 0x8
#define FIGURE_CHANGE 0x10 /* the change figure, by its shape motion */
#define FIGURE_FROZEN 0x20 /* the frozen overlay */
#define FIGURE_MELTING 0x8000 /* his tree melts as his deformer says */

/* the welds and actions chaos6_change_form can give his motion: his
 * body's, his jets form's and the change figure's */
enum chaos6_form { FORM_BODY, FORM_JETS, FORM_CHANGE };

/* the entries of jets_actions that his modes ask for */
enum jets_entry {
    JETS_FORM_FIRE = 1,
    JETS_FORM_STOP = 2, /* it holds its last frame */
    CHANGE_ENTRY = 5 /* the change figure's first entry */
};

/* the change figure's last frame, which a change plays it to or from */
#define CHANGE_END 179.0F

/* how many collision shapes he has */
#define SHAPES 4

/* rows of shape_rows, named for the modes that turn them on */
enum shape_row {
    SHAPES_NONE = 0,
    SHAPES_CHASE = 1,
    SHAPES_MELTED = 4,
    SHAPES_FROZEN = 5,
    SHAPES_INHALE = 6
};

/* x without its sign. it evaluates x twice, so chaos6_inhale_pull calls
 * angle_difference twice, as the image does */
#define ABS(x) ((x) < 0 ? -(x) : (x))

/* where chaos6_inhale_pull pulls its work: nearer than INHALE_HEIGHT and
 * INHALE_RANGE to his mouth, and within INHALE_CONE of the way he faces */
#define INHALE_HEIGHT 45.0F /* above or below */
#define INHALE_RANGE 250.0F /* across */
#define INHALE_CONE 0x2aaa /* 60 degrees either side of the way he faces */

/* how much a leap's upward speed drops each step */
#define GRAVITY 0.027222222F

/* the steps a melt or a rise takes */
#define MELT_STEPS 60.0F

/* the farthest across the aiming node stretches to player 0 */
#define AIM_REACH 220.0F

/* the steps BOSS_TRANSFORM's and BOSS_UNTRANSFORM's fades take, and
 * BOSS_UNTRANSFORM's wait from its fade's start to the change figure */
#define FADE_STEPS 30
#define UNTRANSFORM_WAIT 60

/* BOSS_JETS's counts of steps: from the glow's start to the jets', while
 * the jets fire, and from their drawing back in to his untransform */
#define JETS_LEAD 120
#define JETS_STEPS 240
#define JETS_TRAIL 240

/* the steps he holds frozen before he thaws, and from his break to the
 * stage's clear, less one */
#define FROZEN_HOLD 300
#define BREAK_WAIT 180

/* the steps his inhale breathes in once BODY_INHALING plays, and the frame
 * of BODY_THROW at which it throws player 0 out */
#define INHALE_STEPS 240
#define THROW_FRAME 20.0F

/* how much of the way to player 0 a BOSS_LEAP leap covers */
#define LEAP_SHARE 0.75F

/* a weave walks WEAVE_LEGS legs of WEAVE_LEG each. it ends early at a leg
 * that would end more than WEAVE_BOUND across from the fight's center */
#define WEAVE_LEGS 4
#define WEAVE_LEG 150.0F
#define WEAVE_BOUND 180.0F
/* a retreat ends once he's more than this across from the fight's center */
#define RETREAT_BOUND 150.0F

/* chaos6_choose_attack sorts player 0's distance into bands: nearer than
 * BAND_NEAR, nearer than BAND_MIDDLE, and farther. Big's fight sorts by
 * BAND_NEAR alone, across */
#define BAND_NEAR 100.0F
#define BAND_MIDDLE 150.0F

/* the steps after his guard flash shows during which a hit shows no
 * other */
#define FLASH_WAIT 60

/* the voice Sonic's fight plays once he's frozen, unless a hint has just
 * shown */
#define VOICE_FROZEN 179

#define SE_MELT 341 /* each step of a melt or a rise */
#define SE_STEP 342
#define SE_LEAP 384
#define SE_HIT 386
#define SE_RISE 388
#define SE_LEAP_LAND 389
/* as the jets stop, after JETS_STEPS steps or a hit */
#define SE_JETS_END 392
#define SE_INHALE 393
#define SE_STRIKE 394
#define SE_SINK 396
#define SE_FROST 398
#define SE_THAW 401
/* each step player 0 is nearer than NEAR_SOUND_RANGE to him */
#define SE_NEAR 1000
#define NEAR_SOUND_RANGE 60.0F

/* the counts a mode keeps in its work's vars */
struct boss_vars {
    Uint8 unk_00[4];
    union {
        /* the steps left in a wait, or those since the jets drew back */
        Sint32 steps;
        /* [0] counts down his inhale's steps. [1] holds a leap's wait */
        Sint16 short_steps[2];
        Sint8 count; /* the leaps or weave legs left */
    } mode;
    Uint8 unk_08[4];
};

#define VARS(work) ((struct boss_vars *)&(work)->vars)

static void chaos6_change_form(struct task_work *work, struct motion *motion,
                               int form);
static Bool chaos6_event_in_control(void);
static void chaos6_knuckles_lift(struct task *task);
static void chaos6_boss_step(struct task *task);
static void chaos6_draw(struct task *task);

/* the tether keeps player 0 within chaos6_tether_radius across of
 * chaos6_tether_center */
static const Float chaos6_tether_radius = 220.0F;
static const Sint8 chaos6_shape_count = SHAPES;

static NJS_POINT3 chaos6_tether_center = {0.0F, 744.0F, -385.0F};
static struct task *chaos6_tether = NULL;

/* his collision shapes, all spheres. 0 is his body, and 1 the tip
 * chaos6_place_tip_shape moves. no row of shape_rows turns on 2. 3 is the
 * widest, and only SHAPES_FROZEN turns it on */
/* clang-format off */
static struct collision_shape chaos6_shapes[] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE, SHAPE_HURTS_PLAYERS | ATTACK(1),
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 30.0F, 0.0F}, 35.0F, 0.0F, 0.0F,
     {0, 0, 0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_IN_WORLD,
     SHAPE_HURTS_PLAYERS | ATTACK(1), PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F}, 18.0F, 0.0F, 0.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, SHAPE_HURTS_PLAYERS | ATTACK(1),
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 25.0F, 0.0F}, 25.0F, 0.0F, 0.0F,
     {0, 0, 0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 30.0F, 0.0F}, 45.0F, 0.0F, 0.0F,
     {0, 0, 0}, 0, NULL},
};
/* clang-format on */
/* the shapes each row of chaos6_set_shapes turns on */
static Sint8 shape_rows[][SHAPES] = {{0, 0, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0},
                                     {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 1},
                                     {0, 0, 0, 0}};
/* the marks chaos_track_points moves boss_points onto. in his body,
 * boss_points[0] is his mouth, on node 38, and boss_points[1] the point on
 * node 112 his strike trails from. in his jets form, boss_points[0] lies 20
 * along the x axis of node 35, his tail */
static struct boss_mark body_marks[] = {{38, 0, {0.0F, -15.0F, 7.0F}},
                                        {112, 0, {0.0F, 0.0F, 0.0F}},
                                        {-1, 0, {0.0F, 0.0F, 0.0F}}};
static struct boss_mark jets_marks[] = {{35, 0, {20.0F, 0.0F, 0.0F}},
                                        {-1, 0, {0.0F, 0.0F, 0.0F}}};
/* the boss camera's offsets, for camera_set_boss_offsets. each pair is the
 * target's offset from what the camera follows, then the camera's offset
 * from player 0. the first pair holds through the fight. the others hold
 * through his strike, his jets and his leap */
static NJS_POINT3 camera_target_offset = {0.0F, 0.0F, 0.0F};
static NJS_POINT3 camera_place_offset = {0.0F, 20.0F, 50.0F};
static NJS_POINT3 strike_target_offset = {0.0F, 50.0F, 0.0F};
static NJS_POINT3 strike_place_offset = {0.0F, 20.0F, 100.0F};
static NJS_POINT3 jets_target_offset = {0.0F, 60.0F, 0.0F};
static NJS_POINT3 jets_place_offset = {0.0F, 20.0F, 100.0F};
static NJS_POINT3 leap_target_offset = {0.0F, -15.0F, 0.0F};
static NJS_POINT3 leap_place_offset = {0.0F, 20.0F, 80.0F};
/* the boss camera's bounds on its spins and on its move a step, for
 * camera_set_boss_limits */
static struct angles camera_spin_limits = {0x5f7, 0x5f7, 0x5f7};
static NJS_VECTOR camera_move_limits = {10.0F, 6.0F, 0.0F};

/* set once BOSS_START has started the fight's camera, life gauge and
 * hints, which start only while it's clear */
static Sint8 chaos6_started = 0;
/* the speeds a throw gives player 0: hit_recoil after a hit on him while he's
 * frozen, and inhale_recoil after his inhale. each is written in the frame
 * player 0 faces, and holds that speed until the first throw turns it into the
 * world in place */
static NJS_VECTOR hit_recoil = {-2.0F, 2.0F, 0.0F};
static NJS_VECTOR inhale_recoil = {-2.0F, 2.0F, 0.0F};
static NJS_OBJECT *puddle_objects[] = {&chaos_puddle, NULL};

/* each weld's indices: for each vertex its two nodes share, its index in
 * object_a, then in object_b */
static Uint16 body_indices_0[14] = {
#include "b_chaos6/boss/body_indices_0.inc"
};
static Uint16 body_indices_1[10] = {
#include "b_chaos6/boss/body_indices_1.inc"
};
static Uint16 body_indices_2[10] = {
#include "b_chaos6/boss/body_indices_2.inc"
};
static Uint16 body_indices_3[14] = {
#include "b_chaos6/boss/body_indices_3.inc"
};
static Uint16 body_indices_4[20] = {
#include "b_chaos6/boss/body_indices_4.inc"
};
static Uint16 body_indices_5[10] = {
#include "b_chaos6/boss/body_indices_5.inc"
};
static Uint16 body_indices_6[10] = {
#include "b_chaos6/boss/body_indices_6.inc"
};
/* @bug memory: the last pair names vertex 109 of chaos6_body_node3,
 * whose model, at 0x0c91f080, has 109 points. motion_weld reads the model's
 * first normal, at 0x0c91eb64, as that vertex's point, and the model's own
 * first 12 bytes as its normal. each step it welds, it writes both back from
 * its copies, and sets vertex 11 of chaos6_body_node45 from them */
static Uint16 body_indices_7[10] = {
#include "b_chaos6/boss/body_indices_7.inc"
};
static Uint16 body_indices_8[10] = {
#include "b_chaos6/boss/body_indices_8.inc"
};
static Uint16 body_indices_9[12] = {
#include "b_chaos6/boss/body_indices_9.inc"
};
static Uint16 body_indices_10[12] = {
#include "b_chaos6/boss/body_indices_10.inc"
};
static Uint16 body_indices_11[10] = {
#include "b_chaos6/boss/body_indices_11.inc"
};
static Uint16 body_indices_12[10] = {
#include "b_chaos6/boss/body_indices_12.inc"
};
static Uint16 body_indices_13[10] = {
#include "b_chaos6/boss/body_indices_13.inc"
};
static Uint16 body_indices_14[10] = {
#include "b_chaos6/boss/body_indices_14.inc"
};
static Uint16 body_indices_15[12] = {
#include "b_chaos6/boss/body_indices_15.inc"
};
static Uint16 body_indices_16[12] = {
#include "b_chaos6/boss/body_indices_16.inc"
};
static Uint16 body_indices_17[12] = {
#include "b_chaos6/boss/body_indices_17.inc"
};
static Uint16 body_indices_18[10] = {
#include "b_chaos6/boss/body_indices_18.inc"
};
static Uint16 body_indices_19[10] = {
#include "b_chaos6/boss/body_indices_19.inc"
};
static Uint16 body_indices_20[10] = {
#include "b_chaos6/boss/body_indices_20.inc"
};
static Uint16 body_indices_21[10] = {
#include "b_chaos6/boss/body_indices_21.inc"
};
static Uint16 body_indices_22[10] = {
#include "b_chaos6/boss/body_indices_22.inc"
};
static Uint16 body_indices_23[10] = {
#include "b_chaos6/boss/body_indices_23.inc"
};
static Uint16 body_indices_24[10] = {
#include "b_chaos6/boss/body_indices_24.inc"
};
static Uint16 body_indices_25[10] = {
#include "b_chaos6/boss/body_indices_25.inc"
};
static Uint16 body_indices_26[10] = {
#include "b_chaos6/boss/body_indices_26.inc"
};

/* clang-format off */
static struct weld body_welds[] = {
    {&chaos6_body_object, &chaos6_body_node38, &chaos6_body_node3, 7,
     WELD_TO_B, 0, 0, NULL, body_indices_0},
    {&chaos6_body_object, &chaos6_body_node24, &chaos6_body_node3, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_1},
    {&chaos6_body_object, &chaos6_body_node30, &chaos6_body_node3, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_2},
    {&chaos6_body_object, &chaos6_body_node36, &chaos6_body_node3, 7,
     WELD_TO_B, 0, 0, NULL, body_indices_3},
    {&chaos6_body_object, &chaos6_body_node79, &chaos6_body_node3, 10,
     WELD_TO_B, 0, 0, NULL, body_indices_4},
    {&chaos6_body_object, &chaos6_body_node57, &chaos6_body_node3, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_5},
    {&chaos6_body_object, &chaos6_body_node63, &chaos6_body_node3, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_6},
#ifdef GCC
    /* repairs body_indices_7's last pair, which its tag names: GCC welds
     * only the 4 pairs before it */
    {&chaos6_body_object, &chaos6_body_node45, &chaos6_body_node3, 4,
     WELD_TO_B, 0, 0, NULL, body_indices_7},
#else
    {&chaos6_body_object, &chaos6_body_node45, &chaos6_body_node3, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_7},
#endif
    {&chaos6_body_object, &chaos6_body_node51, &chaos6_body_node3, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_8},
    {&chaos6_body_object, &chaos6_body_node69, &chaos6_body_node3, 6,
     WELD_TO_B, 0, 0, NULL, body_indices_9},
    {&chaos6_body_object, &chaos6_body_node75, &chaos6_body_node3, 6,
     WELD_TO_B, 0, 0, NULL, body_indices_10},
    {&chaos6_body_object, &chaos6_body_node56, &chaos6_body_node57, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_11},
    {&chaos6_body_object, &chaos6_body_node62, &chaos6_body_node63, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_12},
    {&chaos6_body_object, &chaos6_body_node44, &chaos6_body_node45, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_13},
    {&chaos6_body_object, &chaos6_body_node50, &chaos6_body_node51, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_14},
    {&chaos6_body_object, &chaos6_body_node74, &chaos6_body_node75, 6,
     WELD_TO_B, 0, 0, NULL, body_indices_15},
    {&chaos6_body_object, &chaos6_body_node68, &chaos6_body_node69, 6,
     WELD_TO_B, 0, 0, NULL, body_indices_16},
    {&chaos6_body_object, &chaos6_body_node116, &chaos6_body_node79, 6,
     WELD_TO_B, 0, 0, NULL, body_indices_17},
    {&chaos6_body_object, &chaos6_body_node115, &chaos6_body_node116, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_18},
    {&chaos6_body_object, &chaos6_body_node114, &chaos6_body_node115, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_19},
    {&chaos6_body_object, &chaos6_body_node113, &chaos6_body_node114, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_20},
    {&chaos6_body_object, &chaos6_body_node103, &chaos6_body_node79, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_21},
    {&chaos6_body_object, &chaos6_body_node99, &chaos6_body_node103, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_22},
    {&chaos6_body_object, &chaos6_body_node98, &chaos6_body_node99, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_23},
    {&chaos6_body_object, &chaos6_body_node91, &chaos6_body_node79, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_24},
    {&chaos6_body_object, &chaos6_body_node87, &chaos6_body_node91, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_25},
    {&chaos6_body_object, &chaos6_body_node86, &chaos6_body_node87, 5,
     WELD_TO_B, 0, 0, NULL, body_indices_26},
    {NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL},
};
/* clang-format on */

static Uint16 jets_indices_0[12] = {
#include "b_chaos6/boss/jets_indices_0.inc"
};
static Uint16 jets_indices_1[12] = {
#include "b_chaos6/boss/jets_indices_1.inc"
};
static Uint16 jets_indices_2[12] = {
#include "b_chaos6/boss/jets_indices_2.inc"
};
static Uint16 jets_indices_3[12] = {
#include "b_chaos6/boss/jets_indices_3.inc"
};
static Uint16 jets_indices_4[12] = {
#include "b_chaos6/boss/jets_indices_4.inc"
};
static Uint16 jets_indices_5[12] = {
#include "b_chaos6/boss/jets_indices_5.inc"
};
static Uint16 jets_indices_6[12] = {
#include "b_chaos6/boss/jets_indices_6.inc"
};
static Uint16 jets_indices_7[12] = {
#include "b_chaos6/boss/jets_indices_7.inc"
};
static Uint16 jets_indices_8[8] = {
#include "b_chaos6/boss/jets_indices_8.inc"
};
static Uint16 jets_indices_9[8] = {
#include "b_chaos6/boss/jets_indices_9.inc"
};
static Uint16 jets_indices_10[8] = {
#include "b_chaos6/boss/jets_indices_10.inc"
};
static Uint16 jets_indices_11[8] = {
#include "b_chaos6/boss/jets_indices_11.inc"
};
static Uint16 jets_indices_12[8] = {
#include "b_chaos6/boss/jets_indices_12.inc"
};
static Uint16 jets_indices_13[8] = {
#include "b_chaos6/boss/jets_indices_13.inc"
};
static Uint16 jets_indices_14[10] = {
#include "b_chaos6/boss/jets_indices_14.inc"
};
static Uint16 jets_indices_15[10] = {
#include "b_chaos6/boss/jets_indices_15.inc"
};
static Uint16 jets_indices_16[10] = {
#include "b_chaos6/boss/jets_indices_16.inc"
};
static Uint16 jets_indices_17[12] = {
#include "b_chaos6/boss/jets_indices_17.inc"
};
static Uint16 jets_indices_18[12] = {
#include "b_chaos6/boss/jets_indices_18.inc"
};
static Uint16 jets_indices_19[12] = {
#include "b_chaos6/boss/jets_indices_19.inc"
};

/* clang-format off */
static struct weld jets_welds[] = {
    {&chaos6_jets_object, &chaos6_jets_node44, &chaos6_jets_object, 6,
     WELD_TO_A, 0, 0, NULL, jets_indices_0},
    {&chaos6_jets_object, &chaos6_jets_node48, &chaos6_jets_object, 6,
     WELD_TO_A, 0, 0, NULL, jets_indices_1},
    {&chaos6_jets_object, &chaos6_jets_node52, &chaos6_jets_object, 6,
     WELD_TO_A, 0, 0, NULL, jets_indices_2},
    {&chaos6_jets_object, &chaos6_jets_node56, &chaos6_jets_object, 6,
     WELD_TO_A, 0, 0, NULL, jets_indices_3},
    {&chaos6_jets_object, &chaos6_jets_node60, &chaos6_jets_object, 6,
     WELD_TO_A, 0, 0, NULL, jets_indices_4},
    {&chaos6_jets_object, &chaos6_jets_node64, &chaos6_jets_object, 6,
     WELD_TO_A, 0, 0, NULL, jets_indices_5},
    {&chaos6_jets_object, &chaos6_jets_node1, &chaos6_jets_object, 6,
     WELD_TO_A, 0, 0, NULL, jets_indices_6},
    {&chaos6_jets_object, &chaos6_jets_node2, &chaos6_jets_node1, 6,
     WELD_TO_A, 0, 0, NULL, jets_indices_7},
    {&chaos6_jets_object, &chaos6_jets_node6, &chaos6_jets_node2, 4,
     WELD_TO_A, 0, 0, NULL, jets_indices_8},
    {&chaos6_jets_object, &chaos6_jets_node10, &chaos6_jets_node2, 4,
     WELD_TO_A, 0, 0, NULL, jets_indices_9},
    {&chaos6_jets_object, &chaos6_jets_node14, &chaos6_jets_node2, 4,
     WELD_TO_A, 0, 0, NULL, jets_indices_10},
    {&chaos6_jets_object, &chaos6_jets_node18, &chaos6_jets_node2, 4,
     WELD_TO_A, 0, 0, NULL, jets_indices_11},
    {&chaos6_jets_object, &chaos6_jets_node22, &chaos6_jets_node2, 4,
     WELD_TO_A, 0, 0, NULL, jets_indices_12},
    {&chaos6_jets_object, &chaos6_jets_node26, &chaos6_jets_node2, 4,
     WELD_TO_A, 0, 0, NULL, jets_indices_13},
    {&chaos6_jets_object, &chaos6_jets_node35, &chaos6_jets_node36, 5,
     WELD_TO_A, 0, 0, NULL, jets_indices_14},
    {&chaos6_jets_object, &chaos6_jets_node36, &chaos6_jets_node37, 5,
     WELD_TO_A, 0, 0, NULL, jets_indices_15},
    {&chaos6_jets_object, &chaos6_jets_node37, &chaos6_jets_node38, 5,
     WELD_TO_A, 0, 0, NULL, jets_indices_16},
    {&chaos6_jets_object, &chaos6_jets_node38, &chaos6_jets_node2, 6,
     WELD_TO_A, 0, 0, NULL, jets_indices_17},
    {&chaos6_jets_object, &chaos6_jets_node39, &chaos6_jets_node40, 6,
     WELD_TO_A, 0, 0, NULL, jets_indices_18},
    {&chaos6_jets_object, &chaos6_jets_node40, &chaos6_jets_node65, 6,
     WELD_TO_A, 0, 0, NULL, jets_indices_19},
    {NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL},
};
/* clang-format on */

static Uint16 change_indices_0[10] = {
#include "b_chaos6/boss/change_indices_0.inc"
};
static Uint16 change_indices_1[10] = {
#include "b_chaos6/boss/change_indices_1.inc"
};
static Uint16 change_indices_2[10] = {
#include "b_chaos6/boss/change_indices_2.inc"
};
static Uint16 change_indices_3[12] = {
#include "b_chaos6/boss/change_indices_3.inc"
};
static Uint16 change_indices_4[10] = {
#include "b_chaos6/boss/change_indices_4.inc"
};
static Uint16 change_indices_5[10] = {
#include "b_chaos6/boss/change_indices_5.inc"
};
static Uint16 change_indices_6[10] = {
#include "b_chaos6/boss/change_indices_6.inc"
};
static Uint16 change_indices_7[12] = {
#include "b_chaos6/boss/change_indices_7.inc"
};
static Uint16 change_indices_8[10] = {
#include "b_chaos6/boss/change_indices_8.inc"
};
static Uint16 change_indices_9[10] = {
#include "b_chaos6/boss/change_indices_9.inc"
};
static Uint16 change_indices_10[10] = {
#include "b_chaos6/boss/change_indices_10.inc"
};

/* clang-format off */
static struct weld change_welds[] = {
    {&chaos6_change_object, &chaos6_change_node1, &chaos6_change_object, 5,
     WELD_TO_A, 0, 0, NULL, change_indices_0},
    {&chaos6_change_object, &chaos6_change_node2, &chaos6_change_object, 5,
     WELD_TO_A, 0, 0, NULL, change_indices_1},
    {&chaos6_change_object, &chaos6_change_node5, &chaos6_change_object, 5,
     WELD_TO_A, 0, 0, NULL, change_indices_2},
    {&chaos6_change_object, &chaos6_change_node7, &chaos6_change_object, 6,
     WELD_TO_A, 0, 0, NULL, change_indices_3},
    {&chaos6_change_object, &chaos6_change_node3, &chaos6_change_object, 5,
     WELD_TO_A, 0, 0, NULL, change_indices_4},
    {&chaos6_change_object, &chaos6_change_node4, &chaos6_change_object, 5,
     WELD_TO_A, 0, 0, NULL, change_indices_5},
    {&chaos6_change_object, &chaos6_change_node6, &chaos6_change_object, 5,
     WELD_TO_A, 0, 0, NULL, change_indices_6},
    {&chaos6_change_object, &chaos6_change_node8, &chaos6_change_object, 6,
     WELD_TO_A, 0, 0, NULL, change_indices_7},
    {&chaos6_change_object, &chaos6_change_node23, &chaos6_change_node24, 5,
     WELD_TO_A, 0, 0, NULL, change_indices_8},
    {&chaos6_change_object, &chaos6_change_node24, &chaos6_change_node25, 5,
     WELD_TO_A, 0, 0, NULL, change_indices_9},
    {&chaos6_change_object, &chaos6_change_node25, &chaos6_change_node26, 5,
     WELD_TO_A, 0, 0, NULL, change_indices_10},
    {NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL},
};
/* clang-format on */

/* his jets form's actions, then at 5 and 6 the change figure's. each of
 * the change figure's two entries holds 79 in nodes, the jets form's
 * count, though the change figure's tree has 27 */
/* clang-format off */
static struct motion_entry jets_actions[] = {
    {&dat_0ca35b24, 79, MOTION_LOOP, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0ca30c68, 79, MOTION_LOOP, 1, 1.0F / 6.0F, 0.5F},
    {&dat_0ca2c16c, 79, MOTION_HOLD, 1, 1.0F, 0.5F},
    {&dat_0ca272b0, 79, MOTION_LOOP, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0ca20604, 79, MOTION_LOOP, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0ca3a840, 79, MOTION_LOOP, 5, 1.0F / 6.0F, 1.0F},
    {&dat_0ca3a840, 79, MOTION_LOOP, 6, 1.0F / 6.0F, 1.0F},
};
/* his body's actions */
static struct motion_entry body_actions[] = {
    {&dat_0c9b18cc, 117, MOTION_LOOP, 0, 1.0F, 0.5F},
    {&dat_0ca0e7d4, 117, MOTION_ONCE, 2, 1.0F, 0.5F},
    {&dat_0ca18a30, 117, MOTION_LOOP, 2, 1.0F, 0.5F},
    {&dat_0ca1d41c, 117, MOTION_ONCE, 0, 1.0F, 0.5F},
    {&dat_0ca0aa98, 117, MOTION_LOOP, 4, 1.0F, 0.5F},
    {&dat_0ca035dc, 117, MOTION_LOOP, 5, 1.0F, 0.5F},
    {&dat_0c991098, 117, MOTION_ONCE, 7, 0.0625F, 0.5F},
    {&dat_0c9945a4, 117, MOTION_ONCE, 8, 0.0625F, 0.5F},
    {&dat_0c997b80, 117, MOTION_LOOP, 8, 0.0625F, 0.5F},
    {&dat_0c99b10c, 117, MOTION_ONCE, 10, 0.0625F, 0.5F},
    {&dat_0c99ef28, 117, MOTION_ONCE, 0, 0.0625F, 0.5F},
    {&dat_0c98cb4c, 117, MOTION_ONCE, 12, 1.0F / 6.0F, 0.5F},
    {&dat_0c9a80a0, 117, MOTION_LOOP, 12, 1.0F / 6.0F, 0.5F},
    {&dat_0c9a36a4, 117, MOTION_ONCE, 14, 1.0F / 6.0F, 0.5F},
    {&dat_0c983c70, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c964a84, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.4F},
    {&dat_0c97e324, 117, MOTION_HOLD, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c96e540, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c97148c, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c9b308c, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c9b87d8, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c9bcb14, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c9c4250, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c9c5a0c, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c9cb348, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c9cf6a4, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c9d6df0, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c9eb0dc, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c9f0888, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c9f4b44, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c9fc100, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c9d857c, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c9dde68, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c9e20d4, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c9e9930, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c977d08, 117, MOTION_HOLD, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0c9a36a4, 117, MOTION_ONCE, 0, 1.0F / 6.0F, 0.5F},
    {&dat_0ca0e7d4, 117, MOTION_ONCE, 38, 1.0F, 1.0F},
    {&dat_0ca18a30, 117, MOTION_LOOP, 38, 1.0F, 1.0F},
    {&dat_0ca1d41c, 117, MOTION_ONCE, 0, 1.0F, 1.0F},
};
/* clang-format on */

Sint8 chaos6_frog_caught;
struct deform *chaos6_deform;
Sint8 chaos6_shape_row;
Sint8 chaos6_inhale_due;
Sint16 chaos6_defense_wait;
Sint8 chaos6_landed;
NJS_MATRIX chaos6_tail_matrix;
/* the steps since his chase's last footstep sound */
static int chaos6_chase_sound_steps;
/* how far a melt and a rise have run, from 0 to MELT_END */
static Float chaos6_melt_time;
static Float chaos6_form_time;
/* the jets' task, which his jets mode starts and ends */
static struct task *chaos6_jets_task;
/* his frozen overlay's alpha, from 0 to 1 */
static Float chaos6_frozen_alpha;
/* set when his inhale catches player 0 holding something */
static char chaos6_inhaled_holding;
/* the way his weave's leg heads */
static Angle chaos6_weave_angle;

/* turn on the shapes that row of shape_rows marks, and turn off the rest */
static void chaos6_set_shapes(struct task_work *work, int row)
{
    int i;

    chaos6_shape_row = row;
    for (i = 0; i < SHAPES; ++i) {
        if (shape_rows[row][i]) {
            work->collision->shapes[i].flags &= ~SHAPE_OFF;
        } else {
            work->collision->shapes[i].flags |= SHAPE_OFF;
        }
    }
}

/* move shape 1 to boss_points[1], or in his jets form to boss_points[0] */
/* @hack type: the record taken as a void pointer, which SHC copies to R6
 * for its read (0x0c900504). typed, SHC reads it through R5, the function
 * runs 2 bytes short, and 23 of 25 words differ */
static void chaos6_place_tip_shape(struct task_work *work, void *arg)
{
    struct collision_shape *shapes = work->collision->shapes;
    struct chaos *chaos = arg;

    if (chaos->figures & FIGURE_JETS) {
        shapes[1].center = boss_points[0].pos;
    } else {
        shapes[1].center = boss_points[1].pos;
    }
}

/* BOSS_CHASE: walk after player 0, once BODY_WALK plays. a splash rises
 * each time his record's steps reach a multiple of 256, and a footstep
 * sounds every 31 steps. his shapes are on after his record's first 60 steps */
static void chaos6_chase(struct task_work *work, struct task_motion *unused,
                         struct motion *motion)
{
    struct chaos *chaos = (struct chaos *)motion;

    if (chaos->steps > 60) {
        chaos6_set_shapes(work, SHAPES_CHASE);
    }
    switch (work->submode) {
    case 0:
        chaos->figures = FIGURE_BODY;
        motion->wanted = BODY_WALK;
        work->submode = 1;
        chaos_params->speed = 0.3F;
        chaos_travel = 0.0F;
        /* fall through */
    case 1:
        motion_flags &= ~MOTION_HOLDING;
        if (motion->current == BODY_WALK) {
            chaos6_chase_sound_steps = 0;
            work->submode = 2;
        }
        break;
    case 2:
        chaos->target = player_work[0]->pos;
        chaos_steer(work, unused, motion);
        if (!(Uint8)chaos->steps) {
            chaos_effect_splash(work, 8, 0.5F, 1.0F);
        }
        ++chaos6_chase_sound_steps;
        if (chaos6_chase_sound_steps > 30) {
            chaos6_chase_sound_steps = 0;
            sound_play_at(SE_STEP, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        break;
    }
}

/* BOSS_MELT: face player 0, then melt into his puddle over MELT_STEPS and
 * take boss_return_mode */
static void chaos6_melt(struct task_work *work, struct motion *motion)
{
    struct task_work *player = player_work[0];
    struct chaos *chaos = (struct chaos *)motion;

    switch (work->submode) {
    case 0:
        work->ang.y = (Angle)(atan2f(player->pos.z - work->pos.z,
                                     player->pos.x - work->pos.x) *
                              ANGLE_PER_RADIAN);
        motion->wanted = BODY_STAND;
        chaos6_melt_time = 0.0F;
        chaos6_deform->mode = DEFORM_MELT;
        work->submode = 1;
        break;
    case 1:
        if (motion->current == BODY_STAND) {
            chaos->flags |= CHAOS_NO_BEADS;
            chaos->figures =
                (Sint16)(FIGURE_MELTING | FIGURE_PUDDLE | FIGURE_BODY);
            chaos6_deform->scale = 1.0F;
            chaos->scale.x = chaos->scale.y = chaos->scale.z = 0.0F;
            chaos6_set_shapes(work, SHAPES_MELTED);
            work->submode = 2;
        }
        break;
    case 2:
        sound_play_for(SE_SINK, (int)work, 1, 0, 6);
        chaos6_melt_time += MELT_END / MELT_STEPS;
        motion->pace = njSqrt(chaos6_melt_time) * MELT_PACE;
        if (chaos6_melt_time >= MELT_END) {
            chaos->figures = FIGURE_PUDDLE;
            if (boss_return_mode != work->mode) {
                boss_next_mode = boss_return_mode;
                boss_last_mode = work->mode;
            }
            work->submode = MODE_DONE;
        }
        chaos6_deform->scale = motion->pace;
        chaos->scale.x = chaos->scale.z = chaos_params->full_scale;
        chaos->scale.y = motion->pace * chaos_params->full_scale;
        break;
    }
    if (work->submode == MODE_DONE) {
        chaos6_deform->mode = DEFORM_RESTORE;
        deform_step(chaos6_deform);
    }
    sound_play_for(SE_MELT, (int)work, 1, 0, 6);
}

/* BOSS_FORM: face player 0, then once he stands rise out of his puddle
 * over MELT_STEPS and take boss_return_mode */
static void chaos6_form(struct task_work *work, struct motion *motion)
{
    struct chaos *chaos = (struct chaos *)motion;
    struct task_work *player = player_work[0];

    switch (work->submode) {
    case 0:
        work->ang.y = (Angle)(atan2f(player->pos.z - work->pos.z,
                                     player->pos.x - work->pos.x) *
                              ANGLE_PER_RADIAN);
        chaos6_form_time = 0.0F;
        chaos6_deform->mode = DEFORM_MELT;
        work->submode = 1;
        break;
    case 1:
        if (motion->current == BODY_STAND) {
            chaos->figures =
                (Sint16)(FIGURE_MELTING | FIGURE_PUDDLE | FIGURE_BODY);
            chaos6_deform->scale = 1.0F;
            chaos->scale.x = chaos->scale.y = chaos->scale.z = 0.0F;
            chaos6_set_shapes(work, SHAPES_MELTED);
            work->submode = 2;
        }
        break;
    case 2:
        sound_play_for(SE_RISE, (int)work, 1, 0, 6);
        chaos6_form_time += MELT_END / MELT_STEPS;
        motion->pace = 1.0F - (njSqrt(chaos6_form_time) * MELT_PACE);
        if (chaos6_form_time >= MELT_END) {
            if (boss_return_mode != work->mode) {
                boss_next_mode = boss_return_mode;
                boss_last_mode = work->mode;
            }
            work->submode = MODE_DONE;
        }
        chaos6_deform->scale = motion->pace;
        chaos->scale.x = chaos->scale.z = chaos_params->full_scale;
        chaos->scale.y = motion->pace * chaos_params->full_scale;
        break;
    }
    if (work->submode == MODE_DONE) {
        chaos6_deform->scale = 0.0F;
        chaos->figures = FIGURE_BODY;
        chaos->flags &= ~CHAOS_NO_BEADS;
        chaos6_deform->mode = DEFORM_RESTORE;
        deform_step(chaos6_deform);
    }
    sound_play_for(SE_MELT, (int)work, 1, 0, 6);
}

/* BOSS_TRANSFORM: fade his body out over FADE_STEPS steps. then play the
 * change figure forward to CHANGE_END, a frame a step, fade his jets form
 * in and take BOSS_JETS */
static void chaos6_transform(struct task *task, struct motion *motion)
{
    struct task_work *work = task->work;
    struct chaos *chaos = (struct chaos *)motion;

    switch (work->submode) {
    case 0:
        motion->wanted = BODY_STAND;
        VARS(work)->mode.steps = FADE_STEPS;
        chaos_fade_alpha(task, chaos6_body_fade_nodes, 1.0F, 0.0F, FADE_STEPS);
        work->submode = 1;
        break;
    case 1:
        --VARS(work)->mode.steps;
        if (VARS(work)->mode.steps == 0) {
            motion_flags |= MOTION_HOLDING;
            motion->wanted = BODY_STAND;
            motion->frame = 0.0F;
            chaos->flags |= CHAOS_NO_BEADS;
            chaos->figures = FIGURE_CHANGE;
            chaos6_change_form(work, motion, FORM_CHANGE);
            work->submode = 3;
        }
        break;
    case 3:
        motion->frame += 1.0F;
        if (motion->frame >= CHANGE_END) {
            if (work->mode != BOSS_JETS) {
                boss_next_mode = BOSS_JETS;
                boss_last_mode = work->mode;
            }
            chaos->figures = FIGURE_BODY;
            chaos6_change_form(work, motion, FORM_JETS);
            chaos_fade_alpha(task, chaos6_jets_fade_nodes, 0.0F, 1.0F,
                             FADE_STEPS);
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        motion_flags &= ~MOTION_HOLDING;
    }
}

/* BOSS_UNTRANSFORM: fade his jets form out, and UNTRANSFORM_WAIT steps on
 * play the change figure back from CHANGE_END, a frame a step. then fade
 * his body in and chase */
static void chaos6_untransform(struct task *task, struct motion *motion)
{
    struct task_work *work = task->work;
    struct chaos *chaos = (struct chaos *)motion;

    switch (work->submode) {
    case 0:
        VARS(work)->mode.steps = UNTRANSFORM_WAIT;
        chaos_fade_alpha(task, chaos6_jets_fade_nodes, 1.0F, 0.0F, FADE_STEPS);
        work->submode = 1;
        break;
    case 1:
        --VARS(work)->mode.steps;
        if (VARS(work)->mode.steps == 0) {
            motion->frame = CHANGE_END;
            chaos->flags |= CHAOS_NO_BEADS;
            chaos->figures = FIGURE_CHANGE;
            chaos6_change_form(work, motion, FORM_CHANGE);
            work->submode = 2;
        }
        break;
    case 2:
        if ((motion->frame -= 1.0F) <= 0.0F) {
            if (work->mode != BOSS_CHASE) {
                boss_next_mode = BOSS_CHASE;
                boss_last_mode = work->mode;
            }
            chaos->figures = FIGURE_BODY;
            chaos6_change_form(work, motion, FORM_BODY);
            chaos_fade_alpha(task, chaos6_body_fade_nodes, 0.0F, 1.0F,
                             FADE_STEPS);
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        motion_flags &= ~MOTION_HOLDING;
    }
}

/* BOSS_JETS: his tail glows, and JETS_LEAD - 1 steps later the jets
 * start. they fire for JETS_STEPS steps, or until one knocks player 0 back.
 * then JETS_FORM_STOP plays to its last frame, the jets draw back in, and
 * JETS_TRAIL + 1 steps later he untransforms. until the jets stop, the
 * aiming node turns to player 0 and stretches with player 0's distance
 * across, up to AIM_REACH */
static void chaos6_fire_jets(struct task_work *work, struct task_motion *unused,
                             struct motion *motion)
{
    struct task_work *player = player_work[0];
    Float dx;
    Float dz;
    Float distance;
    Angle heading;

    switch (work->submode) {
    case 0:
        camera_set_boss_offsets(&jets_target_offset, &jets_place_offset);
        camera_set_boss_limits(NULL, &camera_spin_limits, &camera_move_limits);
        motion->wanted = JETS_FORM_FIRE;
        VARS(work)->mode.steps = JETS_LEAD;
        task_create(TASK_WORK, 2, chaos6_tail_glow_start);
        work->submode = 1;
        /* fall through */
    case 1:
        if (--VARS(work)->mode.steps == 0) {
            VARS(work)->mode.steps = JETS_STEPS;
            chaos6_jets_task = task_create(TASK_WORK, 2, chaos6_jets_step);
            work->submode = 2;
        }
        break;
    case 2:
        if (--VARS(work)->mode.steps == 0 ||
            chaos6_jets_task->work->flags & JETS_HIT) {
            work->submode = 3;
        }
        break;
    case 3:
        motion->wanted = JETS_FORM_STOP;
        sound_play(SE_JETS_END, 0, 0, 127);
        vibration_start(0, 4, 45, 0);
        work->submode = 4;
        break;
    case 4:
        if (motion_flags & MOTION_HOLDING) {
            motion_flags &= ~MOTION_HOLDING;
            chaos6_jets_task->work->flags |= JETS_END;
            VARS(work)->mode.steps = 0;
            work->submode = 5;
        }
        break;
    case 5:
        if (++VARS(work)->mode.steps > JETS_TRAIL) {
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode <= 2) {
        dx = player->pos.x - work->pos.x;
        dz = player->pos.z - work->pos.z;
        heading = (Angle)(atan2f(dz, dx) * ANGLE_PER_RADIAN);
        chaos6_jets_aim_object.ang[1] = work->ang.y - heading + 0x4000;
        distance = njSqrt((dx * dx) + (dz * dz));
        if (distance >= AIM_REACH) {
            distance = AIM_REACH;
        }
        chaos6_jets_aim_object.evalflags &= ~NJD_EVAL_UNIT_SCL;
        chaos6_jets_aim_object.scl[0] = chaos6_jets_aim_scale * distance;
    }
    if (work->submode == MODE_DONE) {
        motion_flags &= ~MOTION_HOLDING;
        camera_set_boss_offsets(&camera_target_offset, &camera_place_offset);
        camera_set_boss_limits(NULL, &camera_spin_limits, &camera_move_limits);
        chaos6_tail_glow_end();
        if (work->mode != BOSS_UNTRANSFORM) {
            boss_next_mode = BOSS_UNTRANSFORM;
            boss_last_mode = work->mode;
        }
    }
}

/* BOSS_FROZEN: freeze as the frozen overlay fades in, by 1/30 a step, then
 * hold for FROZEN_HOLD steps. a hit throws player 0 off. it takes 2 health
 * for an attack of level 2 or more, and 1 for any other. a hit that leaves
 * no health breaks him, and the stage clears BREAK_WAIT + 1 steps after it.
 * after any other hit he chases again. unhit, he thaws as the overlay fades
 * out at the same rate, and chases */
static void chaos6_frozen(struct task *task, struct motion *motion)
{
    struct task_work *work = task->work;
    /* @hack spelling: 1/30 in a local, which SHC loads once, from the pool's
     * first word, by the MOVA at 0x0c900ed4. as two literals, SHC loads it
     * from the second word, the record and boss_next_mode's address trade R9
     * and R8, and 12 of 373 words differ */
    Float fade_step = 1.0F / 30.0F;
    struct chaos *chaos = (struct chaos *)motion;
    /* @hack spelling: boss_next_mode written through a pointer, which keeps its
     * address in R8, loaded at 0x0c900ede. written by name, R8 holds -1,
     * and 121 of 373 words differ */
    Sint8 *next_mode = &boss_next_mode;
    struct task_work *player;
    struct angles slope;
    Float ground;

    switch (work->submode) {
    case 0:
        chaos6_set_shapes(work, SHAPES_FROZEN);
        motion->wanted = BODY_FREEZE;
        work->submode = 1;
        chaos6_frozen_alpha = 0.0F;
        player_set_mode(0, PLAYER_MODE_RESET);
        break;
    case 1:
        if (motion_flags & MOTION_HOLDING) {
            chaos->figures |= FIGURE_FROZEN;
            chaos->unk_b8 = -1.0F;
            work->submode = 2;
            if (get_character() == CHARACTER_SONIC && !chaos6_hint_waiting) {
                stream_request_voice(VOICE_FROZEN);
            }
        }
        break;
    case 2:
        chaos6_frozen_alpha += fade_step;
        sound_play_for(SE_FROST, (int)work, 1, 0, 10);
        if (chaos6_frozen_alpha >= 1.0F && (motion_flags & MOTION_HOLDING)) {
            VARS(work)->mode.steps = FROZEN_HOLD;
            work->submode = 3;
        }
        break;
    case 3:
        if (work->flags & COLLISION_DAMAGED) {
            player = player_work[0];
            /* @bug logic: turns hit_recoil into the world in place, and nothing
             * sets it back, so each hit after the first turns the world speed
             * the hit before it left (JSR at 0x0c90102e) */
            player_turn_to_world(player, &hit_recoil);
            /* @hack spelling: x read through hit_recoil's address, which gives
             * each speed's address a pool word of its own, from 0x0c901032.
             * as hit_recoil.x, SHC loads the address once, into R11, player
             * takes R9, and 13 of 373 words differ */
            player_set_speed(0, (&hit_recoil)->x, hit_recoil.y, hit_recoil.z);
            chaos->figures &= ~FIGURE_FROZEN;
            if ((player->collision->shapes[0].levels & SHAPE_ATTACK) >= 2) {
                chaos->health -= 2.0F;
            } else {
                chaos->health -= 1.0F;
            }
            if (chaos->health <= 0.0F) {
                work->submode = 5;
            } else {
                work->submode = 7;
            }
            vibration_start_fading(0, 7, 59, 6);
        } else {
            --VARS(work)->mode.steps;
            if (VARS(work)->mode.steps == 0) {
                motion_flags &= ~MOTION_HOLDING;
                sound_play(SE_THAW, 0, 0, 0);
                work->submode = 4;
            }
        }
        break;
    case 4:
        if ((chaos6_frozen_alpha -= fade_step) <= 0.0F) {
            if (work->mode != BOSS_CHASE) {
                *next_mode = BOSS_CHASE;
                boss_last_mode = work->mode;
            }
            chaos->figures &= ~FIGURE_FROZEN;
            work->submode = MODE_DONE;
        }
        break;
    case 5:
        sound_play(SE_HIT, 0, 0, 0);
        motion_flags &= ~MOTION_HOLDING;
        chaos->figures &= ~FIGURE_BODY;
        VARS(work)->mode.steps = BREAK_WAIT;
        chaos6_debris_scatter(1, 50);
        work->submode = 6;
        /* fall through */
    case 6:
        if (VARS(work)->mode.steps-- == 0) {
            if (work->mode != BOSS_DEFEATED) {
                *next_mode = BOSS_DEFEATED;
                boss_last_mode = work->mode;
            }
            stage_clear();
            if (get_character() == CHARACTER_KNUCKLES) {
                player = player_work[0];
                ground =
                    ground_below(player->pos.x, 754.0F, player->pos.z, &slope);
                if (ground <= 744.0F) {
                    ground = 744.0F;
                }
                player->pos.y = ground > player->pos.y ? ground : player->pos.y;
            }
            task_destroy_children(task);
            work->submode = MODE_DONE;
        }
        break;
    case 7:
        sound_play(SE_HIT, 0, 0, 0);
        motion_flags &= ~MOTION_HOLDING;
        motion->wanted = BODY_FROZEN_HIT;
        chaos6_debris_scatter(0, 40);
        work->submode = 8;
        /* fall through */
    case 8:
        if (motion->current == BODY_STAND) {
            if (work->mode != BOSS_CHASE) {
                *next_mode = BOSS_CHASE;
                boss_last_mode = work->mode;
            }
            work->submode = MODE_DONE;
        }
        break;
    }
    chaos6_frozen_alpha = CLAMP(chaos6_frozen_alpha, 0.0F, 1.0F);
    /* @hack spelling: the model read through the object's address, which the
     * pool holds, not that of its model pointer: the load at 0x0c90127c.
     * read as chaos6_frozen_object.model, 3 of 373 words differ */
    (&chaos6_frozen_object)->model->mats[0].diffuse.argb.a =
        (Uint8)(chaos6_frozen_alpha * 255.0F);
    /* @hack redundant: a return that ends nothing, for the image's MOV.B and
     * CMP/EQ at 0x0c90128a. without it, the functions after move, and 18 of
     * 373 words differ */
    if (work->submode == MODE_DONE) {
        return;
    }
}

/* BOSS_INHALE: breathe in for INHALE_STEPS steps once BODY_INHALING plays.
 * once chaos6_inhale_pull catches player 0, player 0 spins at his mouth
 * until THROW_FRAME of BODY_THROW. then player 0 is thrown out, in
 * PLAYER_MODE_FREEZE if it held something when caught, or else hurt. he
 * chases again once he stands */
static void chaos6_inhale(struct task *task, struct motion *motion)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    NJS_VECTOR offset;
    NJS_POINT3 *mouth;

    switch (work->submode) {
    case 0:
        chaos6_set_shapes(work, SHAPES_INHALE);
        motion->wanted = BODY_INHALE_START;
        chaos6_inhaled_holding = 0;
        work->submode = 1;
        break;
    case 1:
        if (motion->current == BODY_INHALING) {
            task_create_child(TASK_MOTION | TASK_WORK, chaos6_inhale_step,
                              task);
            VARS(work)->mode.short_steps[0] = INHALE_STEPS;
            work->submode = 2;
        }
        break;
    case 2:
        sound_play_for(SE_INHALE, (int)work, 1, 0, 10);
        vibration_start(0, 1, 15, 0);
        chaos_face_player(work, motion);
        --VARS(work)->mode.short_steps[0];
        if (VARS(work)->mode.short_steps[0] == 0) {
            work->submode = 4;
        }
        if (chaos6_inhale_pull(player)) {
            motion->wanted = BODY_CATCH;
            player_set_mode(0, PLAYER_MODE_STILL);
            work->submode = 3;
            vibration_start(0, 4, 30, 0);
            if (player->flags & PLAYER_HOLDING) {
                chaos6_inhaled_holding = 1;
            }
        }
        break;
    case 3:
        mouth = &boss_points[0].pos;
        player->ang.x += 0x800;
        player->ang.y += 0x1000;
        player->ang.z += 0x1800;
        offset = player->pos;
        njSubVector(&offset, mouth);
        /* the offset's length, which the image computes and never reads */
        njScalor(&offset);
        player->pos = *mouth;
        if (motion->current == BODY_THROW && motion->frame >= THROW_FRAME) {
            player->ang.x = player->ang.z = 0;
            player->ang.y = -work->ang.y + 0x4000;
            player_set_mode(0, PLAYER_MODE_RESET);
            if (chaos6_inhaled_holding) {
                player_set_mode(0, PLAYER_MODE_FREEZE);
            } else {
                player->flags |= PLAYER_HURT;
            }
            /* @bug logic: turns inhale_recoil into the world in place, and
             * nothing sets it back, so each inhale's throw after the first
             * turns the world speed the one before it left (JSR at
             * 0x0c9014ac) */
            player_turn_to_world(player, &inhale_recoil);
            /* @hack spelling: x read through inhale_recoil's address, which
             * gives each speed's address a pool word of its own, from
             * 0x0c9014b0. as inhale_recoil.x, SHC loads the address once,
             * into R13, and 7 of 233 words differ */
            player_set_speed(0, (&inhale_recoil)->x, inhale_recoil.y,
                             inhale_recoil.z);
            work->submode = 5;
        }
        break;
    case 4:
        motion->wanted = BODY_INHALE_END;
        work->submode = 5;
        break;
    case 5:
        if (motion->current == BODY_STAND) {
            if (work->mode != BOSS_CHASE) {
                boss_next_mode = BOSS_CHASE;
                boss_last_mode = work->mode;
            }
            work->submode = MODE_DONE;
        }
        break;
    }
    /* @hack redundant: a return that ends nothing, for the image's MOV.B and
     * CMP/EQ at 0x0c90153c. without it, the functions after move, and 11 of
     * 233 words differ */
    if (work->submode == MODE_DONE) {
        return;
    }
}

/* BOSS_STRIKE: strike once with a trail from boss_points[1], then call
 * chaos6_eggman_drop_bomb and chase again */
static void chaos6_strike(struct task *task, struct motion *motion)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    Float dx;
    Float dz;

    switch (work->submode) {
    case 0:
        camera_set_boss_offsets(&strike_target_offset, &strike_place_offset);
        camera_set_boss_limits(NULL, &camera_spin_limits, &camera_move_limits);
        motion->wanted = BODY_STRIKE;
        sound_play(SE_STRIKE, 0, 0, 0);
        work->submode = 1;
        break;
    case 1:
        chaos_effect_trail(&boss_points[1].pos, &boss_points[1].last, 1);
        if (motion->current == BODY_STAND) {
            work->submode = MODE_DONE;
            chaos6_eggman_drop_bomb();
        }
        break;
    }
    /* the distances across to player 0 and to boss_points[1], which the
     * image computes and never reads */
    dx = work->pos.x - player->pos.x;
    dz = work->pos.z - player->pos.z;
    njSqrt((dx * dx) + (dz * dz));
    dx = work->pos.x - boss_points[1].pos.x;
    dz = work->pos.z - boss_points[1].pos.z;
    njSqrt((dx * dx) + (dz * dz));
    if (work->submode == MODE_DONE) {
        camera_set_boss_offsets(&camera_target_offset, &camera_place_offset);
        camera_set_boss_limits(NULL, &camera_spin_limits, &camera_move_limits);
        if (work->mode != BOSS_CHASE) {
            boss_next_mode = BOSS_CHASE;
            boss_last_mode = work->mode;
        }
    }
}

/* BOSS_LEAP: leap LEAP_SHARE of the way to player 0, once or twice,
 * with a shockwave at each landing. then call chaos6_eggman_drop_bomb and
 * chase again */
static void chaos6_leap(struct task *task, struct task_motion *movement,
                        struct motion *motion)
{
    struct task_work *work = task->work;
    struct chaos *chaos = (struct chaos *)motion;
    struct task_work *player;
    int steps;

    switch (work->submode) {
    case 0:
        camera_set_boss_offsets(&leap_target_offset, &leap_place_offset);
        camera_set_boss_limits(NULL, &camera_spin_limits, &camera_move_limits);
        VARS(work)->mode.count = rand() * RAND_SCALE > 0.5F ? 1 : 2;
        /* fall through */
    case 1:
        motion->wanted = BODY_LEAP_START;
        work->submode = 2;
        /* fall through */
    case 2:
        if (motion->current == BODY_LEAP) {
            player = player_work[0];
            movement->speed.y = 2.0F;
            steps = (int)(movement->speed.y * (2.0F / GRAVITY));
            movement->speed.x =
                (player->pos.x - work->pos.x) * LEAP_SHARE / steps;
            movement->speed.z =
                (player->pos.z - work->pos.z) * LEAP_SHARE / steps;
            work->submode = 3;
            sound_play(SE_LEAP, 0, 0, 0);
        }
        break;
    case 3:
        chaos_face_player(work, motion);
        movement->speed.y -= GRAVITY;
        njAddVector(&work->pos, &movement->speed);
        if (movement->speed.y < 0.0F) {
            work->submode = 4;
        }
        break;
    case 4:
        chaos_face_player(work, motion);
        movement->speed.y -= GRAVITY;
        njAddVector(&work->pos, &movement->speed);
        if (work->pos.y <= chaos->ground) {
            motion->wanted = BODY_LAND;
            work->pos.y = chaos->ground;
            chaos6_shockwave_create(work);
            chaos6_landed = 1;
            work->submode = 5;
            sound_play(SE_LEAP_LAND, 0, 0, 0);
            vibration_start(0, 5, 15, 0);
        }
        break;
    case 5:
        if (motion->current == BODY_STAND) {
            --VARS(work)->mode.count;
            if (VARS(work)->mode.count == 0) {
                work->submode = MODE_DONE;
                chaos6_eggman_drop_bomb();
            } else {
                /* a wait of 30 steps, for case 6 to count down. no submode
                 * leads to case 6, so the wait never runs, and his next leap
                 * starts at once */
                VARS(work)->mode.short_steps[1] = 30;
                work->submode = 1;
            }
        }
        break;
    case 6:
        --VARS(work)->mode.short_steps[1];
        if (VARS(work)->mode.short_steps[1] == 0) {
            work->submode = 1;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        camera_set_boss_offsets(&camera_target_offset, &camera_place_offset);
        camera_set_boss_limits(NULL, &camera_spin_limits, &camera_move_limits);
        if (work->mode != BOSS_CHASE) {
            boss_next_mode = BOSS_CHASE;
            boss_last_mode = work->mode;
        }
    }
}

/* BOSS_LEAP_HOME: leap to the fight's center, then take boss_return_mode */
static void chaos6_leap_home(struct task *task, struct task_motion *movement,
                             struct motion *motion)
{
    struct task_work *work = task->work;
    struct chaos *chaos = (struct chaos *)motion;
    int steps;

    switch (work->submode) {
    case 0:
        /* a count drawn as BOSS_LEAP draws it, though he leaps home once */
        VARS(work)->mode.count = rand() * RAND_SCALE > 0.5F ? 1 : 2;
        motion->wanted = BODY_LEAP_START;
        work->submode = 1;
        /* fall through */
    case 1:
        if (motion->current == BODY_LEAP) {
            movement->speed.y = 2.0F;
            steps = (int)(movement->speed.y * (2.0F / GRAVITY));
            movement->speed.x = (chaos_params->center.x - work->pos.x) / steps;
            movement->speed.z = (chaos_params->center.z - work->pos.z) / steps;
            work->submode = 2;
            sound_play(SE_LEAP, 0, 0, 0);
        }
        break;
    case 2:
        chaos_face_player(work, motion);
        movement->speed.y -= GRAVITY;
        njAddVector(&work->pos, &movement->speed);
        if (movement->speed.y < 0.0F) {
            work->submode = 3;
        }
        break;
    case 3:
        movement->speed.y -= GRAVITY;
        njAddVector(&work->pos, &movement->speed);
        if (work->pos.y <= chaos->ground) {
            motion->wanted = BODY_LAND;
            work->pos.y = chaos->ground;
            work->submode = 4;
            sound_play(SE_LEAP_LAND, 0, 0, 0);
            vibration_start(0, 5, 15, 0);
        }
        break;
    case 4:
        if (motion->current == BODY_STAND) {
            work->submode = MODE_DONE;
            if (boss_return_mode != work->mode) {
                boss_next_mode = boss_return_mode;
                boss_last_mode = work->mode;
            }
        }
        break;
    }
    /* @hack redundant: a return that ends nothing, for the image's MOV.B at
     * 0x0c901b52 and its CMP/EQ at 0x0c901b56. without it, the functions
     * after move, and 104 of 186 words differ */
    if (work->submode == MODE_DONE) {
        return;
    }
}

/* BOSS_WEAVE: walk WEAVE_LEGS legs toward player 0. each leg heads an
 * eighth of a turn off the way to him, to one side and then the other. he
 * chases once the last leg ends, or at the start of a leg that would end
 * past WEAVE_BOUND */
/* @hack type: the record taken as a void pointer and read into two locals,
 * as a motion and as a record, which keeps the record in R12 and
 * chaos6_weave_angle's address in R13, from 0x0c901b78. taken as a struct
 * motion pointer, the two trade, and 14 of 175 words differ */
static void chaos6_weave(struct task *task, struct task_motion *movement,
                         void *arg)
{
    struct motion *motion = arg;
    struct task_work *work = task->work;
    struct chaos *chaos = arg;
    struct task_work *player;
    Float distance;
    Float dx;
    Float dz;

    switch (work->submode) {
    case 0:
        VARS(work)->mode.count = WEAVE_LEGS;
        motion->wanted = BODY_FAST_WALK;
        /* fall through */
    case 1:
        player = player_work[0];
        dx = player->pos.x - work->pos.x;
        dz = player->pos.z - work->pos.z;
        chaos6_weave_angle = (Angle)(atan2f(dz, dx) * ANGLE_PER_RADIAN);
        chaos6_weave_angle += (VARS(work)->mode.count & 1) ? 0x2000 : -0x2000;
        chaos->target = work->pos;
        chaos->target.x += njCos(chaos6_weave_angle) * WEAVE_LEG;
        chaos->target.z += njSin(chaos6_weave_angle) * WEAVE_LEG;
        movement->speed.x = njCos(chaos6_weave_angle) * 0.8F;
        movement->speed.y = 0.0F;
        movement->speed.z = njSin(chaos6_weave_angle) * 0.8F;
        work->submode = 2;
        /* fall through */
    case 2:
        chaos_face_player(work, motion);
        njAddVector(&work->pos, &movement->speed);
        distance = chaos_distance_across(&chaos->target, &chaos_params->center);
        if (distance > WEAVE_BOUND) {
            work->submode = MODE_DONE;
        } else if (chaos_distance_across(&work->pos, &chaos->target) < 10.0F) {
            --VARS(work)->mode.count;
            work->submode = VARS(work)->mode.count > 0 ? 1 : MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        if (work->mode != BOSS_CHASE) {
            boss_next_mode = BOSS_CHASE;
            boss_last_mode = work->mode;
        }
    }
}

/* BOSS_FROG_HOOKED: stand. whatever mode he's asked for next, he leaves
 * for BOSS_CHASE */
static void chaos6_idle(struct task_work *work, struct motion *motion)
{
    switch (work->submode) {
    case 0:
        motion->wanted = BODY_STAND;
        work->submode = 1;
        break;
    case 1:
        break;
    }
    if (work->submode == MODE_DONE) {
        if (work->mode != BOSS_CHASE) {
            boss_next_mode = BOSS_CHASE;
            boss_last_mode = work->mode;
        }
    }
}

/* BOSS_RETREAT: back away from player 0 until more than RETREAT_BOUND
 * across from the fight's center, then chase again */
static void chaos6_retreat(struct task *task, struct task_motion *movement,
                           struct motion *motion)
{
    struct task_work *work = task->work;
    Float distance;

    switch (work->submode) {
    case 0:
        motion->wanted = BODY_FAST_WALK;
        work->submode = 1;
        /* fall through */
    case 1:
        chaos_face_player(work, motion);
        movement->speed.x = -njCos(work->ang.y) * 0.8F;
        movement->speed.y = 0.0F;
        movement->speed.z = -njSin(work->ang.y) * 0.8F;
        njAddVector(&work->pos, &movement->speed);
        distance = chaos_distance_across(&work->pos, &chaos_params->center);
        if (distance > RETREAT_BOUND) {
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        if (work->mode != BOSS_CHASE) {
            boss_next_mode = BOSS_CHASE;
            boss_last_mode = work->mode;
        }
    }
}

/* make the first material of each of objects, a list a null ends,
 * translucent at opacity percent. an object without a model is skipped */
static void chaos6_set_opacity(NJS_OBJECT **objects, int opacity)
{
    NJS_MATERIAL *material;
    NJS_OBJECT *object;
    /* @hack redundant: register on alpha, which puts it in R6 and each
     * object and material in R1, from 0x0c901e9a. as a plain int, alpha
     * takes R7 and they R5, and 18 of 35 words differ */
    register int alpha = opacity * 255 / 100;

    while ((object = *objects) != NULL) {
        if (object->model) {
            material = object->model->mats;
            material->attrflags &= ~NJD_FLAG_USE_ENV;
            material->attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
            material->attrflags |= NJD_FLAG_USE_ALPHA;
            material->diffuse.argb.a = alpha;
        }
        ++objects;
    }
}

/* ready his models: the puddle translucent at chaos_params' alpha, every
 * node of the change figure shown, and his body's fading nodes opaque */
static void chaos6_reset_models(void)
{
    NJS_OBJECT *objects[100];
    NJS_OBJECT *object;
    int i;

    chaos6_set_opacity(dat_0c93c220, chaos_params->alpha);
    chaos6_set_opacity(dat_0c93c284, chaos_params->alpha);
    chaos6_set_opacity(puddle_objects, chaos_params->alpha);
    chaos_list_nodes(&chaos6_change_object, objects);
    i = 0;
    while ((object = objects[i++]) != NULL) {
        object->evalflags &= ~NJD_EVAL_HIDE;
    }
    chaos_set_alpha(chaos6_body_fade_nodes, 1.0F);
}

/* run one step of his mode */
static void chaos6_step_mode(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *movement = task->motion;
    struct chaos *chaos = task->extra;

    switch (work->mode) {
    case BOSS_START:
        chaos->figures = FIGURE_BODY;
        chaos6_set_shapes(work, SHAPES_NONE);
        if (camera_swing_done) {
            if (work->mode != BOSS_CHASE) {
                boss_next_mode = BOSS_CHASE;
                boss_last_mode = work->mode;
            }
            time_start();
            pad_enable_player(0);
            if (!chaos6_started) {
                chaos6_started = 1;
                switch (get_character()) {
                case CHARACTER_SONIC:
                case CHARACTER_KNUCKLES:
                    camera_override_mode(CAMERA_MODE_CHAOS_STD,
                                         CAMERA_ADJUST_NONE);
                    life_gauge_create(0, 0, chaos_params->hits);
                    break;
                case CHARACTER_BIG:
                    camera_restore();
                    camera_set_mode(CAMERA_MODE_CHAOS_STD, CAMERA_ADJUST_NONE);
                    break;
                }
                task_create_child(TASK_MOTION | TASK_WORK, chaos6_hint_step,
                                  task);
            }
            chaos->steps = 0;
        }
        break;
    case BOSS_CHASE:
        chaos6_chase(work, movement, &chaos->motion);
        break;
    case BOSS_MELT:
        chaos6_melt(work, &chaos->motion);
        break;
    case BOSS_FORM:
        chaos6_form(work, &chaos->motion);
        break;
    case BOSS_TRANSFORM:
        chaos6_transform(task, &chaos->motion);
        break;
    case BOSS_UNTRANSFORM:
        chaos6_untransform(task, &chaos->motion);
        break;
    case BOSS_JETS:
        chaos6_fire_jets(work, movement, &chaos->motion);
        break;
    case BOSS_FROZEN:
        chaos6_frozen(task, &chaos->motion);
        break;
    case BOSS_INHALE:
        chaos6_inhale(task, &chaos->motion);
        break;
    case BOSS_STRIKE:
        chaos6_strike(task, &chaos->motion);
        break;
    case BOSS_LEAP:
        chaos6_leap(task, movement, &chaos->motion);
        break;
    case BOSS_LEAP_HOME:
        chaos6_leap_home(task, movement, &chaos->motion);
        break;
    case BOSS_DEFEATED:
        break;
    case BOSS_WEAVE:
        chaos6_weave(task, movement, &chaos->motion);
        break;
    case BOSS_MODE_20:
        break;
    case BOSS_MODE_21:
        break;
    case BOSS_FROG_HOOKED:
        chaos6_idle(work, &chaos->motion);
        break;
    case BOSS_RETREAT:
        chaos6_retreat(task, movement, &chaos->motion);
        break;
    case BOSS_WAIT:
    case BOSS_MELTED:
        chaos6_set_shapes(work, SHAPES_NONE);
        break;
    }
}

/* count his record's attack_timer down while his mode and boss_next_mode
 * are both BOSS_CHASE. returns 1 at the step it runs out, and starts it
 * again at 480 to 959. returns 0 at any other step */
/* @hack type: the record taken as a void pointer, which keeps it in R14
 * from 0x0c902234. typed, SHC keeps it at R15+0, the functions after move,
 * and 38 of 39 words differ */
static int chaos6_step_attack_timer(struct task_work *work, void *arg)
{
    struct chaos *chaos = arg;

    if (work->mode != BOSS_CHASE) {
        return 0;
    }
    if (boss_next_mode != BOSS_CHASE) {
        return 0;
    }
    if (--chaos->attack_timer > 0) {
        return 0;
    }
    chaos->attack_timer = (int)(rand() * (480.0F * RAND_SCALE)) + 480;
    return 1;
}

/* @hack spelling: BOSS_INHALE through an inline function, which keeps
 * chaos6_choose_attack's record at R15+0 and player at R15+4. as
 * BOSS_INHALE in place, the record takes R11 and player R15+0, the
 * functions after move, and 114 of 183 words differ */
#pragma inline(inhale_mode)
static int inhale_mode(void)
{
    return BOSS_INHALE;
}

/* choose his next mode in Sonic's and Knuckles' fights, each time his
 * attack timer runs out. the choice turns on his health, player 0's
 * distance and chaos6_inhale_due. to transform, he leaps home first, with
 * BOSS_TRANSFORM as boss_return_mode */
static void chaos6_choose_attack(struct task_work *work, struct chaos *record)
{
    /* @hack spelling: the record copied to a local, which keeps it at R15+0
     * and player at R15+4, stored at 0x0c9022b8 and 0x0c9022be. through the
     * parameter alone, the record takes R11 and player R15+0, the functions
     * after move, and 180 of 183 words differ */
    struct chaos *chaos = record;
    int band;
    struct task_work *player = player_work[0];
    int return_mode = BOSS_CHASE;
    int next;
    Float random;
    Float distance;
    Float far_chance;
    Float near_chance;
    Float nearest_chance;

    if (chaos6_step_attack_timer(work, record)) {
        random = rand() * RAND_SCALE;
        distance = njDistanceP2P(&player->pos, &work->pos);
        /* @hack redundant: a never-run block of tests of random, which SHC
         * counts: it keeps the record at R15+0, player at R15+4 and work in R12
         * (0x0c9022b8, 0x0c9022be, 0x0c9022bc); without it, the record takes
         * R11, player R15+0 and work R13, the function runs 2 bytes long, the
         * functions after it move, and 123 of 183 words differ */
        if (0) {
            if (random) {
            }
            if (random) {
            }
            if (random) {
            }
            if (random) {
            }
            if (random) {
            }
            if (random) {
            }
            if (distance) {
            }
        }
        if (distance < BAND_NEAR) {
            band = 0;
        } else if (distance < BAND_MIDDLE) {
            band = 1;
        } else {
            band = 2;
        }
        next = BOSS_LEAP_HOME;
        /* @hack spelling: the thresholds in locals. as literals, 0.3F leaves
         * the record in R9 and player at R15+0, 0.8F follows 0.7F in the
         * pool at 0x0c9023a0, and 0.7F takes FR5 and 0.8F FR4. the
         * functions after move, and 175 of 183 words differ */
        far_chance = 0.3F;
        nearest_chance = 0.8F;
        near_chance = 0.7F;
        /* @hack redundant: a never-run test of chaos, which keeps 0, 1 and 2 in
         * R8, R9 and R10 (0x0c9022f0, 0x0c9022f6, 0x0c9022b4); without it, they
         * take R9, R10 and R8, the record and player trade slots, and 23 of 183
         * words differ */
        if (0) {
            if (chaos) {
            }
        }
        switch ((int)chaos->health) {
        case 5:
        case 4:
        case 3:
            switch (chaos6_inhale_due) {
            case 0:
                switch (band) {
                case 0:
                    if (random < near_chance) {
                        next = BOSS_STRIKE;
                    } else if (random < nearest_chance) {
                        return_mode = BOSS_TRANSFORM;
                    } else {
                        next = BOSS_LEAP;
                    }
                    break;
                case 1:
                    if (random < near_chance) {
                        return_mode = BOSS_TRANSFORM;
                    } else {
                        next = BOSS_LEAP;
                    }
                    break;
                case 2:
                    if (random < far_chance) {
                        return_mode = BOSS_TRANSFORM;
                    } else {
                        next = BOSS_LEAP;
                    }
                    break;
                }
                chaos6_inhale_due = 1;
                break;
            case 1:
                next = inhale_mode();
                chaos6_inhale_due = 0;
                break;
            }
            break;
        case 1:
        case 2:
            switch (chaos6_inhale_due) {
            case 0:
                switch (band) {
                case 0:
                    if (random < near_chance) {
                        next = BOSS_STRIKE;
                    } else if (random < nearest_chance) {
                        return_mode = BOSS_TRANSFORM;
                    } else {
                        next = BOSS_LEAP;
                    }
                    break;
                case 1:
                    if (random < near_chance) {
                        return_mode = BOSS_TRANSFORM;
                    } else {
                        next = BOSS_LEAP;
                    }
                    break;
                case 2:
                    if (random < far_chance) {
                        return_mode = BOSS_TRANSFORM;
                    } else {
                        next = BOSS_LEAP;
                    }
                    break;
                }
                chaos6_inhale_due = 1;
                break;
            case 1:
                if (random < 0.65F) {
                    next = BOSS_INHALE;
                } else {
                    next = BOSS_CHASE;
                }
                chaos6_inhale_due = 0;
                break;
            }
            break;
        }
        /* @hack redundant: a never-run block that sets next to BOSS_LEAP and
         * tests random against nearest_chance, which keeps the record at R15+0
         * and player at R15+4 (0x0c9022b8, 0x0c9022be); without it, the record
         * takes R11, player R15+0, 0 R13 and 1 R8, the function runs 4 bytes
         * long, the functions after it move, and 143 of 183 words differ */
        if (0) {
            next = BOSS_LEAP;
            if (random < nearest_chance) {
            }
            if (random < nearest_chance) {
            }
        }
        if (next != work->mode) {
            boss_next_mode = next;
            boss_last_mode = work->mode;
        }
        boss_return_mode = return_mode;
        /* NOLINTBEGIN(clang-diagnostic-literal-conversion) */
        /* @hack redundant: a never-run block of tests of constants, which keeps
         * 0 in R8 and 1 in R9, the band in R7 and chaos6_inhale_due's address
         * in R13, and 0.7 in FR5 and 0.8 in FR4 (0x0c9022f0 to 0x0c90236e);
         * without it, 0 takes R10 and 2 R8, the band and chaos6_inhale_due's
         * address trade R7 and R13, 0.7 and 0.8 trade FR4 and FR5, and 23 of
         * 183 words differ */
        if (0) {
            if (0.1F ? 1.0F : 0.1F) {
            }
            if (0.2F ? 1.0F : 0.2F) {
            }
            if (0.4F ? 1.0F : 0.4F) {
            }
            if (0.6F ? 1.0F : 0.6F) {
            }
            if (0.9F ? 1.0F : 0.9F) {
            }
            if (1.1F ? 1.0F : 1.1F) {
            }
            if (1.2F ? 1.0F : 1.2F) {
            }
        }
        /* NOLINTEND(clang-diagnostic-literal-conversion) */
    }
}

/* choose his next mode in Big's fight. once Big has caught the frog, he
 * stands from BOSS_CHASE or BOSS_WEAVE. until then, he chooses each time
 * his attack timer runs out. within BAND_NEAR across of player 0 he
 * strikes, retreats or weaves, and farther off he leaps or weaves */
static void chaos6_choose_big_attack(struct task_work *work,
                                     struct chaos *chaos)
{
    struct task_work *player = player_work[0];
    /* @hack spelling: boss_next_mode written through a pointer, which keeps its
     * address in R13 from 0x0c902498. written by name, SHC saves no R13,
     * chaos6_create after it moves, and 92 of 93 words differ */
    Sint8 *next_mode = &boss_next_mode;
    Float distance;
    Float random;
    int next;

    if (chaos6_frog_caught) {
        /* @hack redundant: a never-run test of chaos, which keeps player at
         * R15+0 and distance at R15+4, stored at 0x0c90249a and 0x0c9024ec;
         * without it, the two trade, the function runs 4 bytes short,
         * chaos6_create after it moves, and 52 of 93 words differ */
        if (0) {
            if (chaos) {
            }
        }
        if (work->mode == BOSS_CHASE || work->mode == BOSS_WEAVE) {
            if (work->mode != BOSS_FROG_HOOKED) {
                *next_mode = BOSS_FROG_HOOKED;
                boss_last_mode = work->mode;
                /* @hack redundant: a return, which keeps the BRA and its NOP at
                 * 0x0c9024c2. without it, the store before takes the delay
                 * slot, chaos6_create after it moves, and 69 of 93 words
                 * differ */
                return;
            }
        }
    } else if (work->mode == BOSS_CHASE && *next_mode == BOSS_CHASE &&
               chaos6_step_attack_timer(work, chaos)) {
        distance = chaos_distance_across(&player->pos, &work->pos);
        random = rand() * RAND_SCALE;
        if (distance < BAND_NEAR) {
            if (random > 0.8F) {
                next = BOSS_STRIKE;
            } else if (random > 0.3F) {
                next = BOSS_RETREAT;
            } else {
                next = BOSS_WEAVE;
            }
        } else {
            next = random > 0.8F ? BOSS_LEAP : BOSS_WEAVE;
        }
        if (next != work->mode) {
            *next_mode = next;
            boss_last_mode = work->mode;
        }
    }
}

void chaos6_create(void)
{
    task_create(TASK_MOTION | TASK_WORK, 2, chaos6_boss_step);
}

/* his task's destructor. it ends his mode and kills the tether. it frees
 * his light, his motion's blend, his welds' buffers and his deformer */
static void chaos6_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos *chaos = task->extra;

    if (work->submode != MODE_DONE) {
        work->submode = MODE_DONE;
        chaos6_step_mode(task);
    }
    chaos_light_delete();
    njSetMotionCallback(NULL);
    chaos->motion.state = MOTION_FREE;
    motion_step(work, &chaos->motion);
    chaos->motion.weld_mode = PLAYER_WELD_FREE;
    motion_weld(work, &chaos->motion);
    deform_free(chaos6_deform);
    boss_waiting = 0;
    if (chaos6_tether) {
        task_kill(chaos6_tether);
    }
}

/* his draw's motion callback, which leaves each object as it is */
static void ignore_object(NJS_OBJECT *object)
{
}

/* begin the fight. his name shows, and the camera swings round him while
 * the clock stops and player 0's pad is off. each character's tasks, the
 * music's wait and the tether start, and he takes BOSS_START */
static void chaos6_begin(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos *chaos = task->extra;
    struct task *music_wait;

    chaos6_set_shapes(work, SHAPES_NONE);
    chaos_face_off(work);
    boss_name_create(TEXT_1, -1, 240, 120);
    camera_set_boss_offsets(&camera_target_offset, &camera_place_offset);
    camera_set_boss_limits(NULL, &camera_spin_limits, &camera_move_limits);
    camera_start_swing(work, 30.0F, 90.0F, 240);
    camera_override_mode(CAMERA_MODE_CHAOS_STINIT, CAMERA_ADJUST_NONE);
    if (work->mode != BOSS_START) {
        boss_next_mode = BOSS_START;
        boss_last_mode = work->mode;
    }
    time_stop();
    pad_disable_player(0);
    chaos_face_off(work);
    switch (get_character()) {
    case CHARACTER_SONIC:
        task_create_child(TASK_MOTION | TASK_WORK, chaos6_eggman_step, task);
        break;
    case CHARACTER_KNUCKLES:
        task_create_child(TASK_MOTION | TASK_WORK, chaos6_bomb_ring_step, task);
        task_create_child(TASK_WORK, chaos6_knuckles_lift, task);
        break;
    case CHARACTER_BIG:
        task_create(TASK_MOTION | TASK_WORK, 2, chaos6_frog_step);
        chaos6_frog_caught = 0;
        break;
    }
    stream_destroy();
    music_wait = task_create(TASK_WORK, 0, chaos6_music_wait_step);
    music_wait->work->mode = MUSIC_CHAOS_6;
    music_wait->work->timer = 60;
    camera_set_perspective(NJM_DEG_ANG(70));
    work->ang.x = work->ang.z = 0;
    boss_waiting = 0;
    chaos6_bomb_count = 0;
    chaos6_tether = chaos_tether_create(
        &player_work[0]->pos, &chaos6_tether_center, chaos6_tether_radius);
    camera_set_perspective(NJM_DEG_ANG(80));
    chaos->figures = FIGURE_BODY;
}

/* his task's step. the setup step runs no mode. while boss_waiting is set
 * he chooses no attack, and the fight begins once his record's steps pass 1
 * and no event is in control */
static void chaos6_boss_step(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: the record read into two locals, as a motion and as a
     * record, which puts motion in R13 and the record in R11, from
     * 0x0c902862. with motion as &chaos->motion, SHC loads the record
     * straight into R11, and 130 of 490 words differ */
    struct motion *motion = task->extra;
    struct chaos *chaos = task->extra;
    struct task *defense;
    struct task_work *player;
    struct angles slope;

    /* @hack redundant: a store under 0.0F, which never runs. without it,
     * chaos6_landed's address and its 0 trade R2 and R3 at 0x0c902858,
     * boss_next_mode's and chaos_params' addresses trade R8 and R9 at
     * 0x0c90285e, and 30 of 490 words differ */
    if (0.0F) {
        task->extra = motion;
    }
    /* @hack redundant: a jump to the next line, which keeps chaos6_landed's
     * address in R2 and its 0 in R3 at 0x0c902858. without it, they trade
     * there and through the setup, and 22 of 490 words differ */
    goto step;
step:
    chaos6_landed = 0;
    switch (work->mode) {
    case BOSS_SETUP:
        chaos6_started = 0;
        motion = motion_create(work, sizeof(struct chaos));
        if (!motion) {
            return;
        }
        collision_init(task, chaos6_shapes, COLLISION_ENEMY, 1,
                       chaos6_shape_count);
        /* @hack spelling: the collision reached through task, which loads
         * task->work again, at 0x0c90289e. through work, SHC reads the
         * collision through R14, after the MOVA there, and 89 of 490 words
         * differ */
        task->work->collision->radius = 400.0F;
        motion->wanted = BODY_STAND;
        motion->current = BODY_STAND;
        motion->actions = body_actions;
        motion->welds = body_welds;
        work->mode = BOSS_START;
        task->extra = motion;
        chaos = (struct chaos *)motion;
        boss_work = work;
        chaos_record = chaos;
        task->destructor = chaos6_destroy;
        chaos_setup(work, chaos, CHAOS_6);
        chaos_effect_set_sizes(2.5F, 2.5F, 2.5F, 0.7F);
        chaos_light_create(60.0F);
        chaos6_reset_models();
        motion_step(work, motion);
        chaos->attack_timer = chaos_params->attack_wait;
        chaos->texlist = &chaos6_texlist;
        chaos_params->model_y = 10.0F;
        motion_weld(work, motion);
        chaos6_deform = deform_create(&chaos6_body_object, work);
        chaos6_inhale_reset();
        task->draw = chaos6_draw;
        if (work->mode != BOSS_WAIT) {
            boss_next_mode = BOSS_WAIT;
            boss_last_mode = work->mode;
        }
        boss_waiting = 1;
        chaos_face_off(work);
        chaos6_inhale_due = 1;
        break;
    default:
        chaos6_step_mode(task);
        break;
    }
    if (chaos6_defense_wait > 0) {
        --chaos6_defense_wait;
    } else if (work->mode == BOSS_FROZEN) {
        /* @hack spelling: an empty arm for BOSS_FROZEN, which keeps
         * task_create's address in R1, at 0x0c902a22. tested with the flag
         * in one condition, the address takes R2, and 2 of 490 words
         * differ */
    } else if (work->flags & COLLISION_DAMAGED) {
        defense = task_create(TASK_WORK, 2, chaos6_defense_step);
        if (defense) {
            defense->work->pos = player_work[0]->pos;
            chaos6_defense_wait = FLASH_WAIT;
        }
    }
    if (!boss_waiting) {
        switch (get_character()) {
        case CHARACTER_SONIC:
        case CHARACTER_KNUCKLES:
            /* @hack spelling: the record passed through motion, here and to
             * chaos_animate_surface and chaos6_place_tip_shape. each call
             * passes R13, at 0x0c902a66, 0x0c902a70, 0x0c902c78 and
             * 0x0c902c80. through chaos, each passes R11, and 104 of 490
             * words differ */
            chaos6_choose_attack(work, (struct chaos *)motion);
            break;
        case CHARACTER_BIG:
            chaos6_choose_big_attack(work, (struct chaos *)motion);
            break;
        }
        if (player_work[0]->pos.y <= 600.0F) {
            player_down(0);
        }
    } else if (chaos->steps > 1 && !chaos6_event_in_control()) {
        chaos6_begin(task);
    }
    if (task->step == task_destroy) {
        return;
    }
    if ((player_work[0]->flags & PLAYER_HOLDING) || work->mode == BOSS_FROZEN) {
        work->collision->flags |= COLLISION_TARGET;
    } else {
        work->collision->flags &= ~COLLISION_TARGET;
    }
    if (work->mode != boss_next_mode) {
        if (work->submode != MODE_DONE) {
            work->submode = MODE_DONE;
            chaos6_step_mode(task);
        }
        boss_last_mode = work->mode;
        work->mode = boss_next_mode;
        work->submode = 0;
    }
    if (!(motion_flags & MOTION_HOLDING)) {
        motion_step(work, motion);
    }
    motion_weld(work, motion);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, -work->ang.y + 0x4000);
    njTranslate(NULL, 0.0F, chaos_params->model_y, 0.0F);
    if (chaos->figures & FIGURE_JETS) {
        chaos_track_points(work, jets_marks, chaos_params->model_y);
        njPushMatrix(NULL);
        pose_get_matrix(35, &chaos6_tail_matrix);
        njMultiMatrix(NULL, &chaos6_tail_matrix);
        njGetMatrix(&chaos6_tail_matrix);
        njPopMatrix(1);
    } else {
        chaos_track_points(work, body_marks, chaos_params->model_y);
    }
    njPopMatrix(1);
    chaos_animate_surface((struct chaos *)motion);
    chaos6_draw(task);
    chaos6_place_tip_shape(work, motion);
    chaos->ground =
        ground_below(work->pos.x, work->pos.y + 4.0F, work->pos.z, &slope);
    chaos->ground = chaos->ground > 744.0F ? chaos->ground : 744.0F;
    chaos->steps++;
    life_gauge_health = chaos->health;
    if (njDistanceP2P(&work->pos, &player_work[0]->pos) < NEAR_SOUND_RANGE) {
        sound_play_for(SE_NEAR, (int)work, 1, 0, 10);
    }
    /* in Big's fight his body's shape stays off. player 0 within 40 of him is
     * hurt instead, and he retreats unless chaos6_bombs_paused returns TRUE */
    if (get_character() == CHARACTER_BIG) {
        player = player_work[0];
        work->collision->shapes->flags |= SHAPE_OFF;
        if (njDistanceP2P(&work->pos, &player->pos) <= 40.0F) {
            player->flags |= PLAYER_HURT;
            if (!chaos6_bombs_paused()) {
                if (work->mode != BOSS_RETREAT) {
                    boss_next_mode = BOSS_RETREAT;
                    boss_last_mode = work->mode;
                }
            }
        }
    }
    collision_register(work);
    task_step_children(task);
    if (!event_in_control()) {
        player = player_work[0];
        if (player->pos.y < 730.0F) {
            player->pos.y = 740.0F;
        }
    }
}

/* draw him as his figures say, at the first step of each frame. his puddle
 * and his tree draw in palette 4, and the frozen overlay in palette 0 */
static void chaos6_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos *chaos = task->extra;
    int current;
    NJS_ACTION *action;

    if (frame_step) {
        return;
    }
    draw_set_palette(4);
    njSetMotionCallback(ignore_object);
    chaos_light_place(&work->pos, -10.0F);
    draw_control_save();
    draw_control_set(NJD_CONTROL_3D_OFFSET_MATERIAL);
    njSetTexture(chaos->texlist);
    current = chaos->motion.current;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    if (chaos->figures & FIGURE_PUDDLE) {
        if (chaos->scale.y < 0.2F) {
            chaos->colors[0].a = (chaos->scale.y - 0.2F) * 5.0F;
        } else {
            chaos->colors[0].a = 0.0F;
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
    if (chaos->figures & (FIGURE_CHANGE | FIGURE_BODY)) {
        njPushMatrix(NULL);
        njSetConstantMaterial(&chaos->colors[0]);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, -work->ang.y + 0x4000);
        njTranslate(NULL, 0.0F, chaos_params->model_y, 0.0F);
        if (chaos->figures & FIGURE_MELTING) {
            deform_step(chaos6_deform);
            if (chaos->motion.pace > 0.7F) {
                chaos->colors[0].a = (0.7F - chaos->motion.pace) *
                                     chaos_params->unk_20 *
                                     (1.0F / (1.0F - 0.7F));
            }
        }
        if (chaos->motion.state != MOTION_BLENDING) {
            action = chaos->motion.actions[current].action;
        } else {
            action = chaos->motion.blend;
        }
        njSetConstantMaterial(&chaos->colors[0]);
        if (chaos->figures & FIGURE_BODY) {
            draw_action(action, chaos->motion.frame);
        }
        if (chaos->figures & FIGURE_CHANGE) {
            /* @hack spelling: the action's object and motion in locals, which
             * keep draw_set_palette's and njPushMatrix's addresses in R10
             * and R12, loaded at 0x0c902e22 and 0x0c902e52. passed
             * straight, they take R9 and R11, njSetConstantMaterial's and
             * njPopMatrix's take R10 and R12, and 24 of 259 words differ */
            NJS_OBJECT *object = action->object;
            NJS_MOTION *motion = action->motion;

            draw_shape_motion(object, motion, &chaos6_change_shape,
                              chaos->motion.frame);
        }
        if (chaos->figures & FIGURE_FROZEN) {
            njPushMatrix(NULL);
            draw_set_palette(0);
            njScale(NULL, 1.01F, 1.01F, 1.01F);
            draw_tree(&chaos6_frozen_object);
            njPopMatrix(1);
        }
        njPopMatrix(1);
    }
    njPopMatrix(1);
    draw_control_restore();
    njSetMotionCallback(NULL);
    draw_set_palette(0);
}

/* give his motion form's welds and actions, from their first entry or, for
 * FORM_CHANGE, CHANGE_ENTRY. the old welds' buffers are freed, and the new
 * ones made. but for FORM_CHANGE, his motion's blend is freed and made
 * again too. FORM_CHANGE sets MOTION_HOLDING, so his step leaves the
 * frame to his mode */
static void chaos6_change_form(struct task_work *work, struct motion *motion,
                               int form)
{
    struct chaos *chaos = (struct chaos *)motion;

    motion->weld_mode = PLAYER_WELD_FREE;
    motion_weld(work, motion);
    if (form != FORM_CHANGE) {
        motion->state = MOTION_FREE;
        motion_step(work, motion);
    }
    switch (form) {
    case FORM_BODY:
        motion->welds = body_welds;
        motion->current = motion->wanted = BODY_STAND;
        motion->actions = body_actions;
        chaos_params->model_y = 10.0F;
        break;
    case FORM_JETS:
        motion->welds = jets_welds;
        motion->current = motion->wanted = 0;
        motion->actions = jets_actions;
        chaos->figures |= FIGURE_JETS;
        chaos_params->model_y = 2.0F;
        break;
    case FORM_CHANGE:
        motion->welds = change_welds;
        motion->current = motion->wanted = CHANGE_ENTRY;
        motion->actions = jets_actions;
        chaos_params->model_y = 2.0F;
        motion_flags |= MOTION_HOLDING;
        motion->state = MOTION_LOOP;
        break;
    }
    motion->weld_mode = PLAYER_WELD_MAKE;
    motion_weld(work, motion);
    if (form != FORM_CHANGE) {
        motion->state = MOTION_START;
        motion_step(work, motion);
    }
}

Bool chaos6_bombs_paused(void)
{
    if (boss_work->mode == BOSS_START || boss_work->mode == BOSS_CHASE ||
        boss_work->mode == BOSS_WEAVE || boss_work->mode == BOSS_STRIKE) {
        return FALSE;
    }
    return TRUE;
}

Bool chaos6_inhale_pull(struct task_work *work)
{
    struct task_work *boss = boss_work;
    NJS_VECTOR offset;
    Float distance;
    Float scale;
    Angle bearing;
    Angle heading;

    if (boss->mode != BOSS_INHALE) {
        return FALSE;
    }
    if (chaos_record->motion.current != BODY_INHALING) {
        return FALSE;
    }
    offset = work->pos;
    njSubVector(&offset, &boss_points[0].pos);
    distance = njSqrt((offset.x * offset.x) + (offset.z * offset.z));
    bearing = (Angle)(atan2f(offset.z, offset.x) * ANGLE_PER_RADIAN);
    heading = boss->ang.y;
    if (njAbs(offset.y) < INHALE_HEIGHT && distance < INHALE_RANGE) {
        if (ABS(angle_difference(bearing, heading)) < INHALE_CONE) {
            scale = -chaos6_inhale_speed / distance;
            offset.x *= scale;
            offset.y *= scale;
            offset.z *= scale;
            njAddVector(&work->pos, &offset);
        }
    }
    if (distance <= chaos6_inhale_reach) {
        return TRUE;
    }
    return FALSE;
}

void chaos6_set_mode(int mode)
{
    struct task_work *work = boss_work;

    if (mode != work->mode) {
        boss_next_mode = mode;
        boss_last_mode = work->mode;
    }
}

static Bool chaos6_event_in_control(void)
{
    return event_in_control();
}

/* a step for Knuckles' fight. while player 0 is on ground that hurts, it
 * sets player 0's speed to 3 straight up. it does nothing while player 0
 * falls out of the stage */
static void chaos6_knuckles_lift(struct task *task)
{
    struct task_motion *motion = player_motion[0];
    struct player *record = motion->record;

    /* @bug memory: the record's effects are read before the record is tested
     * for null, so a null record is read through. player 0's motion holds a
     * null record until player_init sets one, which is null too where its
     * heap_calloc fails. the test guards nothing: the image reads them in
     * this order (MOV.W @(6,R4),R0 at 0x0c903428, TST R4,R4 at 0x0c90342e) */
    if (record->effects & PLAYER_FALLING) {
        return;
    }
    if (!record) {
        return;
    }
    if (record->ground_flags & WORLD_HURTS) {
        player_set_speed(0, 0.0F, 3.0F, 0.0F);
    }
}
