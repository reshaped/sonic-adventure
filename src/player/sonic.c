/* Sonic as a player: his modes, his dashes and attacks, his board and his
 * super form */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "b_chaos7/super.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "effect/wind.h"
#include "event/event.h"
#include "hud/hud.h"
#include "main.h"
#include "object/common.h"
#include "object/panel.h"
#include "object/ring.h"
#include "player/effects.h"
#include "player/player.h"
#include "player/s_mot.h"
#include "player/s_sbmot.h"
#include "player/sonic.h"
#include "player/sonic_effect_models.h"
#include "player/target.h"
#include "player/textures.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg02/sonic_actions.h"
#include "stg04/dat_0ca72368.h"
#include "stg06/dat_0cadb8c8.h"
#include "stg06/sonic_actions.h"
#include "stg07/dat_0ca57908.h"
#include "stg08/sonic_actions.h"
#include "stg09/sonic_actions.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 8
#include "player/sonic/text.inc"

/* his modes, the values of his work->mode. the board modes are for Ice Cap
 * and Sand Hill, and the super modes for the last boss */
enum sonic_mode {
    SONIC_INIT,
    SONIC_STAND,
    SONIC_WALK,
    SONIC_PATH, /* running along a path */
    SONIC_SPIN_CHARGE, /* curled up in place, charging a spin dash */
    SONIC_SPIN_DASH, /* rolling, once the action button is let go */
    SONIC_LIGHT_DASH, /* dashing from ring to ring */
    SONIC_PATH_SPIN, /* rolling along a path, curled up */
    SONIC_JUMP,
    SONIC_SPRING,
    SONIC_LAUNCH,
    SONIC_FLUNG, /* flung along a line, with only gravity on him */
    SONIC_FALL,
    SONIC_STOP, /* braking from a run */
    SONIC_HOMING, /* in a homing attack, from a jump */
    SONIC_PUSH,
    SONIC_HURT,
    SONIC_BACK,
    SONIC_CRASH, /* thrown back from a wall he ran into */
    SONIC_SLIDE,
    SONIC_POSE, /* still, in the action a request gave him */
    SONIC_LIFT,
    SONIC_PUT_DOWN,
    SONIC_HOLD_STAND,
    SONIC_GRIP, /* holding on to what he'll push, pull or shake */
    SONIC_HOLD_WALK,
    SONIC_THROW,
    SONIC_HOLD_STOP,
    SONIC_HOLD_FALL,
    SONIC_HOLD_JUMP,
    SONIC_HOLD_SPRING,
    SONIC_HOLD_LAUNCH,
    SONIC_PULL,
    SONIC_AIR_THROW,
    SONIC_SHAKE,
    /* shaking the small thing he carries, while grab is held and the stick
     * is pushed. his action's frame sets the thing's accel */
    SONIC_HOLD_SHAKE,
    SONIC_HANG, /* hanging from a path */
    SONIC_HANG_GO, /* moving along the path he hangs from */
    SONIC_LEDGE, /* hanging from a ledge */
    SONIC_LEDGE_GO, /* moving along the ledge */
    SONIC_PANEL_JUMP, /* leaping from one panel to the next */
    SONIC_PANEL_WAIT, /* standing on a panel */
    SONIC_CAUGHT, /* in a grabber, until he breaks free */
    SONIC_ICED, /* in a block of ice, until he breaks free */
    SONIC_LIGHT_ATTACK, /* in a light dash's attack on a target */
    /* riding, from PLAYER_MODE_RIDE until PLAYER_MODE_RIDE_OFF or
     * PLAYER_MODE_DOWN */
    SONIC_RIDE,
    /* on a wall: running and rolling, falling and jumping, and crashing */
    SONIC_WALL_RUN,
    SONIC_WALL_SPIN,
    SONIC_WALL_FALL,
    SONIC_WALL_JUMP,
    SONIC_WALL_CRASH,
    /* holding on beside the task player_circle gives him: at rest, coming
     * down to the foot of its shape, and going round it either way */
    SONIC_CIRCLE,
    SONIC_CIRCLE_DOWN,
    SONIC_CIRCLE_LEFT,
    SONIC_CIRCLE_RIGHT,
    /* hanging below the task player_dangle gives him: at rest, slipping as
     * his grip tires, and turning either way. then letting go, climbing up
     * to it, and leaping off */
    SONIC_DANGLE,
    SONIC_DANGLE_SLIP,
    SONIC_DANGLE_LEFT,
    SONIC_DANGLE_RIGHT,
    SONIC_DANGLE_LET_GO,
    SONIC_DANGLE_CLIMB,
    SONIC_DANGLE_LEAP,
    SONIC_BOARD,
    SONIC_UNK_63, /* a mode sonic.c neither sets nor tests */
    SONIC_BOARD_BRAKE,
    SONIC_BOARD_FALL,
    SONIC_BOARD_JUMP,
    SONIC_BOARD_TRICK, /* leaping off a ramp, with a trick */
    SONIC_BOARD_CRASH,
    SONIC_SLOPE_ROLL, /* curled up, as gravity alone rolls him */
    SONIC_SLOPE_ROLL_FALL, /* curled up in the air, after a slope roll */
    SONIC_SUPER_STAND,
    SONIC_SUPER_WALK,
    SONIC_SUPER_PATH,
    SONIC_SUPER_FALL,
    SONIC_SUPER_LAUNCH,
    SONIC_SUPER_SPRING,
    SONIC_SUPER_HOMING,
    SONIC_SUPER_JUMP,
    SONIC_PLAY, /* in the action a request gave him, until it ends */
    SONIC_EVENT, /* following an event's paths */
    SONIC_DROWN, /* out of air: ACTION_DROWN, then up to the water's surface */
    SONIC_DOWN, /* from PLAYER_MODE_DOWN: sonic_step chooses no mode after it */
    SONIC_EDIT /* in the editor, where the stick moves him freely */
};

/* the rows of sonic_actions his code sets or tests, by index. his board
 * actions run from ACTION_BOARD to ACTION_BOARD_CROUCH, and super Sonic's
 * from ACTION_SUPER_STAND to ACTION_SUPER_BALL */
enum sonic_action {
    ACTION_STAND = 0,
    ACTION_LAND = 1,
    ACTION_LANDED = 2,
    ACTION_READY = 3,
    ACTION_FIDGET_START = 4,
    ACTION_FIDGET = 5,
    ACTION_FIDGET_END = 6,
    ACTION_STAND_UPHILL = 7,
    ACTION_STAND_DOWNHILL = 8,
    ACTION_WALK = 9,
    ACTION_TROT = 10,
    ACTION_RUN = 11,
    ACTION_SPRINT = 12,
    ACTION_FULL_SPEED = 13,
    /* his spin, paced by the record's action_speed: his jump, and a spin
     * dash as it charges */
    ACTION_SPIN = 14,
    /* his spin, paced by his speed ahead, as in a spin dash once let go */
    ACTION_ROLL = 15,
    ACTION_SPRING = 16,
    ACTION_FALL_START = 17,
    ACTION_FALL = 18,
    ACTION_HARD_LANDING = 19,
    ACTION_SKID = 20,
    ACTION_PUSH_START = 21,
    ACTION_PUSH = 22,
    ACTION_HURT = 23,
    ACTION_HURT_LAND = 24,
    ACTION_UPDRAFT = 26,
    ACTION_CRASH = 27,
    ACTION_CRASH_LAND = 28,
    ACTION_UNCURL = 29,
    ACTION_TURN = 30,
    ACTION_SLIDE = 31,
    ACTION_BALL = 32, /* the ball sonic_draw draws now and then for a spin */
    ACTION_LIFT = 33,
    ACTION_HOLD_RUN = 34,
    ACTION_HOLD_STOP = 35,
    ACTION_HOLD_TURN = 36,
    ACTION_HOLD_STAND = 37,
    ACTION_HOLD_WALK = 38,
    ACTION_BOUNCE_1 = 39,
    ACTION_BOUNCE_1_FALL = 40,
    ACTION_BOUNCE_2 = 41,
    ACTION_BOUNCE_2_FALL = 42,
    ACTION_SQUASH_2 = 44,
    ACTION_SQUASH_3 = 45,
    ACTION_WALL_CRASH = 46,
    ACTION_HANG_ON = 47,
    ACTION_PULL = 48,
    ACTION_SHAKE = 49,
    ACTION_HOLD_SHAKE_START = 50,
    ACTION_HOLD_SHAKE = 51,
    ACTION_HOLD_SHAKE_END = 52,
    ACTION_GRIP_START = 53,
    ACTION_GRIP = 54,
    ACTION_HOLD_JUMP = 55,
    ACTION_HOLD_FALL_START = 56,
    ACTION_HOLD_FALL = 57,
    ACTION_HOLD_HARD_LANDING = 58,
    ACTION_THROW_AT = 59, /* a throw at the nearest task near him */
    ACTION_THROW = 60, /* a throw straight ahead */
    ACTION_AIR_THROW_AT = 61,
    ACTION_AIR_THROW = 62,
    ACTION_PUT_DOWN = 63,
    ACTION_LIGHT_DASH = 64,
    ACTION_RIDE = 65,
    ACTION_HANG_GO = 66,
    /* still on a path, where ACTION_HANG_GO stops at HANG_HALFWAY */
    ACTION_HANG_HALFWAY = 67,
    /* still on a path, where ACTION_HANG_GO stops at frame 0 */
    ACTION_HANG = 68,
    ACTION_LEDGE = 69,
    ACTION_FLUNG = 70,
    /* faster than a trot on ground of the least grip, with the stick let
     * go */
    ACTION_GLIDE = 71,
    ACTION_PANEL_LAND = 72,
    ACTION_PANEL_JUMP = 73,
    ACTION_PANEL_WAIT = 74,
    ACTION_CLEAR = 75,
    ACTION_ROCKET = 77,
    ACTION_ROCKET_ALONG = 79,
    ACTION_ROCKET_UP = 80,
    ACTION_CAUGHT = 81,
    ACTION_ICED = 82,
    ACTION_BREATHE = 84,
    ACTION_DROWN = 85,
    ACTION_DOWN = 86,
    ACTION_DOWN_LAND = 87,
    ACTION_PATH_SLIDE = 88,
    ACTION_CIRCLE = 89,
    ACTION_CIRCLE_LEFT = 90,
    ACTION_CIRCLE_RIGHT = 91,
    ACTION_CIRCLE_DOWN = 92,
    ACTION_CIRCLE_LAND = 93,
    ACTION_DANGLE = 94,
    ACTION_DANGLE_SLIP = 95,
    ACTION_DANGLE_LET_GO = 96,
    ACTION_DANGLE_LEFT = 99,
    ACTION_DANGLE_RIGHT = 100,
    ACTION_DANGLE_CLIMB = 101,
    ACTION_BOARD = 102,
    /* braking, with the stick a positive turn from his heading, or not */
    ACTION_BOARD_BRAKE_PLUS = 103,
    ACTION_BOARD_BRAKE_MINUS = 104,
    ACTION_BOARD_AIR = 105,
    ACTION_BOARD_JUMP = 106,
    /* leaning as motion->drive is above 0, or below it */
    ACTION_BOARD_LEAN_PLUS = 107,
    ACTION_BOARD_LEAN_MINUS = 108,
    /* crashing into a wall. the second plays when the turn from his speed
     * to the wall's normal is negative about the ground's normal. it plays
     * for a hit on the board too. the first plays for any other crash */
    ACTION_BOARD_CRASH_PLUS = 109,
    ACTION_BOARD_CRASH_MINUS = 110,
    ACTION_BOARD_FALL_START = 111,
    ACTION_BOARD_FALL = 112,
    ACTION_BOARD_HARD_LANDING = 113,
    ACTION_BOARD_SLOW = 114,
    ACTION_BOARD_STRAIGHTEN = 115,
    /* the tricks off a ramp. sonic_choose_mode picks one of the first three
     * at the greatest speeds, then ACTION_TRICK_MEDIUM, then
     * ACTION_TRICK_SMALL */
    ACTION_TRICK_BIG = 117,
    ACTION_TRICK = 118,
    ACTION_TRICK_BACK = 119,
    ACTION_TRICK_MEDIUM = 120,
    ACTION_TRICK_SMALL = 123,
    ACTION_BOARD_CROUCH = 124,
    ACTION_SLOPE_ROLL = 125,
    ACTION_SHOWER = 126,
    ACTION_SHOWER_END = 128,
    ACTION_MUD_OUT = 129,
    ACTION_SUPER_STAND = 130,
    ACTION_SUPER_WALK = 131,
    ACTION_SUPER_TROT = 132,
    ACTION_SUPER_RUN = 133,
    ACTION_SUPER_SPRINT = 134,
    ACTION_SUPER_SPRING = 135,
    ACTION_SUPER_FALL_START = 136,
    ACTION_SUPER_FALL = 137,
    ACTION_SUPER_CLEAR = 139,
    ACTION_SUPER_BALL = 141
};

#define SE_LEAP_OFF 219 /* as he leaps off what he dangles from */
#define SE_HOMING 723
#define SE_LIGHT_CHARGE 724
#define SE_LIGHT_DASH 725 /* it plays until sound_stop stops it */
#define SE_SPIN_DASH 729
#define SE_CRASH 1112
#define SE_LIFT 1114
#define SE_THROW 1115
#define SE_PUSH_START 1116
#define SE_CHARGED 1117
#define SE_LIGHT_DASH_START 1118
#define SE_HURT 1123
#define SE_CAUGHT 1409
#define SE_SONIC_DOWN 1411 /* his own, in place of SE_DOWN */
#define SE_TRICK_BIG 1419
#define SE_TRICK 1420

/* the bit of his timer that hides him. while the timer counts down after a
 * hit, it hides him two steps in every four, so he blinks */
#define BLINK_HIDDEN 0x2

/* the bits of curled_steps that show the ball. while he's curled up,
 * sonic_draw draws ACTION_BALL in place of his spin on each step whose
 * curled_steps has one of them */
#define BALL_STEP_MASK 0x11

/* at full speed and in a spin on the ground, TRAIL_COPIES copies of his
 * action trail him, each TRAIL_GAP frames behind the one before */
#define TRAIL_COPIES 2
#define TRAIL_GAP 2.0F

/* a landing at this speed down or faster is a hard one */
#define HARD_LANDING_SPEED (-5.0F)
#define BOARD_HARD_LANDING_SPEED (-3.0F)

/* the steps he stands still before he fidgets */
#define FIDGET_STEPS (30 * 60)

/* he stands upright on ground up to FLAT_SLOPE steep, and leans uphill or
 * downhill on ground up to LEAN_SLOPE steep */
#define FLAT_SLOPE NJM_DEG_ANG(25)
#define LEAN_SLOPE NJM_DEG_ANG(45)

/* a crash into a wall sends him back at this part of his speed */
#define CRASH_REBOUND 0.3F

/* a crash on a wall pushes him this far off it, and leaves him this part
 * of his speed */
#define WALL_CRASH_PUSH 5.0F
#define WALL_CRASH_SLOW 0.2F

/* the pace of ACTION_SPIN, in the record's action_speed. sonic_try_spin
 * starts it at his speed, at least SPIN_PACE_LEAST. sonic_spin_charge then
 * adds SPIN_PACE_GAIN a step while it's under SPIN_PACE_MOST, and the spin
 * dash he lets go takes the pace for its speed. every other start of
 * ACTION_SPIN sets JUMP_SPIN_PACE */
#define JUMP_SPIN_PACE 5.0F
#define SPIN_PACE_LEAST 2.0F
#define SPIN_PACE_MOST 10.0F
#define SPIN_PACE_GAIN 0.4F

/* a spin dash held CHARGE_STEPS curled up with the light speed shoes, or
 * RING_CHARGE_STEPS with the crystal ring too, charges a light dash. he
 * uncurls after CURL_STEPS */
#define CHARGE_STEPS 210
#define RING_CHARGE_STEPS (2 * 60)
#define CURL_STEPS (5 * 60)

/* his top speed while the light dash is charged */
#define CHARGED_TOP_SPEED 0.5F

/* a light dash starts at LIGHT_DASH_SPEED and lasts LIGHT_DASH_STEPS. each
 * step that finds a ring within RING_CONE of the way he faces leaves it at
 * least RING_STEPS more. he turns toward the ring by up to LIGHT_DASH_TURN,
 * and moves its distance, from DASH_STEP_LEAST to DASH_STEP_MOST */
#define LIGHT_DASH_SPEED 16.0F
#define LIGHT_DASH_STEPS 10
#define RING_CONE 0x3000
#define RING_STEPS 5
#define LIGHT_DASH_TURN 0x1800
#define DASH_STEP_LEAST 2.0F
#define DASH_STEP_MOST 16.0F
/* a light dash stops once he moves LIGHT_DASH_STOP or less in a step, and
 * leaves him no more speed than LIGHT_DASH_END_SPEED */
#define LIGHT_DASH_STOP 1.5F
#define LIGHT_DASH_END_SPEED 2.0F

/* how near a target must be for a homing attack, and for a light dash's
 * attack when there's no rival */
#define TARGET_RANGE 400.0F

/* a homing attack flies at HOMING_SPEED, or SUPER_HOMING_SPEED when he's
 * super, and turns by up to HOMING_TURN a step, or SUPER_HOMING_TURN. a
 * target counts only within HOMING_CONE of the way he faces */
#define HOMING_SPEED 5.0F
#define SUPER_HOMING_SPEED 10.0F
#define HOMING_TURN 0x800
#define SUPER_HOMING_TURN 0x1000
#define HOMING_CONE 0x5000
/* a homing attack ends after HOMING_STEPS. it flies slower, and unevenly,
 * after HOMING_TIRED_STEPS, and ends after HOMING_UNAIMED_STEPS with no
 * target while he has no rival */
#define HOMING_STEPS (6 * 60)
#define HOMING_TIRED_STEPS (3 * 60)
#define HOMING_UNAIMED_STEPS (0.1F * 60.0F)

/* a light attack starts up at ATTACK_RISE. it flies on straight until
 * curled_steps reaches ATTACK_STRAIGHT_STEPS, then at its target at
 * ATTACK_SPEED. the attack would end after LIGHT_ATTACK_STEPS */
#define ATTACK_RISE 16.0F
#define ATTACK_STRAIGHT_STEPS 2
#define ATTACK_SPEED 10.0F
#define LIGHT_ATTACK_STEPS (6 * 60)
/* at its target, the attack flies along a blend of the way he moves and
 * the way to the target. the first weighs ATTACK_INERTIA, and the second
 * the pull. the pull is ATTACK_PULL_LEAST plus ATTACK_PULL_GAIN for each
 * step of curled_steps, which it counts round a cycle of ATTACK_PULL_STEPS */
#define ATTACK_INERTIA 8.0F
#define ATTACK_PULL_LEAST 2.0F
#define ATTACK_PULL_GAIN 0.0625F
#define ATTACK_PULL_STEPS 32

/* his struggle as a grabber catches him, and his ice as he freezes. each
 * press of PAD_MASH_BUTTONS takes STRUGGLE_PRESS off either */
#define STRUGGLE_START (3 * 60)
#define ICE_START (5 * 60)
#define STRUGGLE_PRESS 20

/* his speed as he moves along the path or ledge he hangs from */
#define HANG_SPEED 0.375F
/* sonic_hang_move steps ACTION_HANG_GO by hand, HANG_FRAME_STEP frames a
 * step. the action has HANG_FRAMES frames, and he stops at frame 0 and at
 * HANG_HALFWAY */
#define HANG_FRAME_STEP 2.0F
#define HANG_FRAMES 63.0F
#define HANG_HALFWAY 32.0F
/* sonic_ledge_move steps ACTION_LEDGE by hand too, HANG_FRAME_STEP frames
 * a step. it has LEDGE_FRAMES frames, and he stops at LEDGE_HALFWAY and at
 * LEDGE_LAST, the last frame a step reaches */
#define LEDGE_FRAMES 32.0F
#define LEDGE_HALFWAY 16.0F
#define LEDGE_LAST 30.0F

/* he holds on CIRCLE_REACH from what he goes round when it has no motion
 * to give its reach. SONIC_CIRCLE_DOWN takes him down CIRCLE_DROP a step */
#define CIRCLE_REACH 30.0F
#define CIRCLE_DROP 0.5F
/* a step turns him round what he goes round by 0x10000 / (reach *
 * CIRCLE_ARC_SCALE). reach is how far from it its motion holds him.
 * CIRCLE_ARC_SCALE is about 2 pi / 0.6, so the turn is an arc about 0.6
 * long. with no motion, or a reach of 0, he turns CIRCLE_TURN */
#define CIRCLE_ARC_SCALE 10.47197F
#define CIRCLE_TURN 0x100
/* the turn starts at frame CIRCLE_START of his action, eases in until
 * CIRCLE_FULL, eases out from CIRCLE_SLOW, and stops at CIRCLE_END */
#define CIRCLE_START 4.0F
#define CIRCLE_FULL 14.0F
#define CIRCLE_SLOW 18.0F
#define CIRCLE_END 28.0F

/* below a task, he hangs DANGLE_BEHIND back from it along the way he
 * faces. he climbs DANGLE_CLIMB_SPEED a step while it's more than
 * DANGLE_REACH above him. the stick turns him DANGLE_TURN a step */
#define DANGLE_BEHIND 4.0F
#define DANGLE_REACH 16.0F
#define DANGLE_CLIMB_SPEED 0.3F
#define DANGLE_TURN 0x300
/* his grip slips at DANGLE_SLIP_1, DANGLE_SLIP_2 and DANGLE_SLIP_3 steps,
 * and after the third slip he falls */
#define DANGLE_SLIP_1 (5 * 60)
#define DANGLE_SLIP_2 (7 * 60)
#define DANGLE_SLIP_3 (8 * 60)
/* he lets go LET_GO_FRAMES before his action ends. he falls once he has
 * leapt off for more than LEAP_STEPS */
#define LET_GO_FRAMES 2.0F
#define LEAP_STEPS 20

/* the frame of ACTION_LIFT from which he has what he lifts */
#define LIFT_FRAME 12.0F

/* the frame of each throw's action at which he lets go */
#define THROW_AT_FRAME 20.0F
#define AIR_THROW_AT_FRAME 7.0F
#define THROW_FRAME 24.0F
#define AIR_THROW_FRAME 6.0F
#define PUT_DOWN_FRAME 12.0F

/* his speed past which his spines sway with it */
#define SWAY_SPEED 0.2F

/* the most he rises a step as he drowns */
#define DROWN_RISE 0.4F

/* his shape's center_y above his feet, and super Sonic's */
#define SONIC_CENTER_Y 5.4F
#define SUPER_CENTER_Y 6.4F

/* each sway pass turns the points of his head behind its depth along x,
 * about a pivot from which the model's origin lies SWAY_PIVOT along y */
#define SWAY_PIVOT 2.0F
#define SWAY_DEPTH_1 (-2.0F)
#define SWAY_DEPTH_2 (-1.2F)
#define SWAY_DEPTH_3 (-0.6F)

/* how far below its work->pos the board is drawn: always while it isn't
 * under him, and under him with BOARD_LOWER set */
#define BOARD_DEPTH (-5.1901989F)

/* what the task of his charged light dash keeps in task->extra. it holds
 * his top speed down until it ends */
struct sonic_charged {
    struct task_work *work;
    struct player *record;
    Float top_speed; /* his own, which the task puts back as it ends */
};

#define CHARGED(task) ((struct sonic_charged *)(task)->extra)

/* what the task of his super form keeps in task->extra: his player
 * number, and the consts sonic_super_start replaces, as they were */
struct sonic_super {
    Sint8 player;
    Uint8 unk_01[3];
    Float resist_x;
    Float air_resist;
    Float air_accel;
};

#define SUPER(task) ((struct sonic_super *)(task)->extra)

/* a board's vars: its rider's player number, and its model and texlist */
struct board_vars {
    Uint8 player;
    Uint8 unk_01[3];
    NJS_OBJECT *object;
    NJS_TEXLIST *texlist;
};

#define BOARD_VARS(work) ((struct board_vars *)&(work)->vars)

/* his model's root, and the nodes sonic_welds joins or marks, each on his
 * -z or +z side */
extern NJS_OBJECT sonic_object;
extern NJS_OBJECT sonic_upper_arm_minus_z;
extern NJS_OBJECT sonic_forearm_minus_z;
/* @hack type: an array, not the single NJS_OBJECT it is, so SHC loads its
 * address and stores its sibling at a displacement (MOV.L R3,@(48,R2) at
 * 0x8c0af860); as one object, SHC folds &sonic_ring_mount.sibling into a
 * pool word and 5 of sonic_step's 2810 words differ */
extern NJS_OBJECT sonic_ring_mount[];
/* the wrist sonic_step hangs from sonic_ring_mount: plain, or with the
 * crystal ring */
extern NJS_OBJECT sonic_wrist_minus_z;
extern NJS_OBJECT sonic_ring_wrist;
extern NJS_OBJECT sonic_hand_minus_z;
extern NJS_OBJECT sonic_fingers_minus_z;
extern NJS_OBJECT sonic_upper_arm_plus_z;
extern NJS_OBJECT sonic_forearm_plus_z;
extern NJS_OBJECT sonic_wrist_plus_z;
extern NJS_OBJECT sonic_hand_plus_z;
extern NJS_OBJECT sonic_fingers_plus_z;
extern NJS_OBJECT sonic_thigh_minus_z;
extern NJS_OBJECT sonic_shin_minus_z;
/* @hack type: an array, not the single NJS_OBJECT it is, so SHC loads its
 * address and stores its sibling at a displacement (MOV.L R2,@(48,R3) at
 * 0x8c0af84a); as one object, SHC folds &sonic_foot_minus_z.sibling into a
 * pool word and 7 of sonic_step's 2810 words differ */
extern NJS_OBJECT sonic_foot_minus_z[];
/* the shoes sonic_step hangs from each foot: plain, or the light speed
 * shoes */
extern NJS_OBJECT sonic_shoe_minus_z;
extern NJS_OBJECT sonic_toe_minus_z;
extern NJS_OBJECT sonic_light_shoe_minus_z;
extern NJS_OBJECT sonic_light_toe_minus_z;
extern NJS_OBJECT sonic_thigh_plus_z;
extern NJS_OBJECT sonic_shin_plus_z;
/* @hack type: an array, not the single NJS_OBJECT it is, so SHC loads its
 * address and stores its sibling at a displacement (MOV.L R2,@(48,R1) at
 * 0x8c0af7f2, MOV.L R2,@(48,R0) at 0x8c0af846); as one object, SHC folds
 * &sonic_foot_plus_z.sibling into a pool word and 5 of sonic_step's 2810
 * words differ */
extern NJS_OBJECT sonic_foot_plus_z[];
extern NJS_OBJECT sonic_shoe_plus_z;
extern NJS_OBJECT sonic_toe_plus_z;
extern NJS_OBJECT sonic_light_shoe_plus_z;
extern NJS_OBJECT sonic_light_toe_plus_z;
extern NJS_MODEL sonic_toe_plus_z_model;
extern NJS_MODEL sonic_toe_plus_z_shape_a;
extern NJS_MODEL sonic_toe_plus_z_shape_b;
extern NJS_MODEL sonic_toe_plus_z_blend;
extern NJS_MODEL sonic_toe_minus_z_model;
extern NJS_MODEL sonic_toe_minus_z_shape_a;
extern NJS_MODEL sonic_toe_minus_z_shape_b;
extern NJS_MODEL sonic_toe_minus_z_blend;
#ifdef GCC
/* the face normals model_blend writes for sonic_toe_plus_z_blend and
 * sonic_toe_minus_z_blend, an array each. in each blend, the first meshset's
 * normals point at the array's first, with room for the meshset's 26
 * triangles. the second meshset's point at the 27th, with room for its 6.
 * model_calc_face_normals writes each strip's normals from its meshset's
 * first, so the first meshset writes at most 10 and the second 4, inside
 * the array's 32. 1ST_READ.BIN's tables place the arrays in bss, where no
 * source holds them. the port defines them here */
NJS_VECTOR sonic_toe_plus_z_face_normals[32];
NJS_VECTOR sonic_toe_minus_z_face_normals[32];
#endif
extern NJS_OBJECT sonic_head_joint;
extern NJS_OBJECT sonic_eye0;
extern NJS_OBJECT sonic_eye1;
/* the face of his mouth's task. sonic_head hangs from it as its sibling */
extern NJS_OBJECT sonic_mouth;
/* the head his mouth's task blends to sonic_head_blend as he talks, and
 * whose model sonic_sway_step replaces with its swayed copy */
/* @hack type: an array, not the single NJS_OBJECT it is, so SHC loads its
 * address and stores its model at a displacement (MOV.L R1,@(4,R3) at
 * 0x8c0b91a2, MOV.L R3,@(4,R2) at 0x8c0b9670); as one object, SHC folds
 * &sonic_head.model into a pool word, and 7 of the 490 words of the three
 * functions that name it differ */
extern NJS_OBJECT sonic_head[];
extern NJS_OBJECT sonic_head_blend;
extern NJS_MOTION sonic_mouth_shapes;
extern NJS_MODEL sonic_head_model;
extern NJS_MODEL sonic_head_sway_model;

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_land_action;
extern NJS_ACTION sonic_jump_land_action;
extern NJS_ACTION sonic_ready_action;
extern NJS_ACTION sonic_fidget_start_action;
extern NJS_ACTION sonic_stand_uphill_action;
extern NJS_ACTION sonic_stand_downhill_action;
extern NJS_ACTION sonic_walk_action;
extern NJS_ACTION sonic_trot_action;
extern NJS_ACTION sonic_run_action;
extern NJS_ACTION sonic_sprint_action;
extern NJS_ACTION sonic_full_speed_action;
extern NJS_ACTION sonic_spin_action;
extern NJS_ACTION sonic_spring_action;
extern NJS_ACTION sonic_fall_start_action;
extern NJS_ACTION sonic_fall_action;
extern NJS_ACTION sonic_hard_landing_action;
extern NJS_ACTION sonic_skid_action;
extern NJS_ACTION sonic_push_start_action;
extern NJS_ACTION sonic_push_action;
extern NJS_ACTION sonic_hurt_action;
extern NJS_ACTION sonic_hurt_land_action;
extern NJS_ACTION sonic_crash_action;
extern NJS_ACTION sonic_crash_land_action;
extern NJS_ACTION sonic_uncurl_action;
extern NJS_ACTION sonic_turn_action;
extern NJS_ACTION sonic_slide_action;
extern NJS_ACTION sonic_ball_action;
extern NJS_ACTION sonic_down_action;

static void sonic_board_score(int points);
static void sonic_stand(struct task_work *work, struct task_motion *motion,
                        struct player *record);
static void sonic_choose_mode(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static void sonic_destroy(struct task *task);
static void sonic_move_shape(struct task_work *work, struct task_motion *motion,
                             struct player *record);
static void sonic_draw(struct task *task);
static void sonic_set_stand_action(struct task_work *work,
                                   struct task_motion *motion,
                                   struct player *record);
static void sonic_set_walk_action(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record);
static void sonic_super_set_walk_action(struct task_work *work,
                                        struct task_motion *motion,
                                        struct player *record);
static void sonic_hang_move(struct task_work *work, struct task_motion *motion,
                            struct player *record);
static void sonic_ledge_move(struct task_work *work, struct task_motion *motion,
                             struct player *record);
static void sonic_set_board_action(struct task_work *work,
                                   struct task_motion *motion,
                                   struct player *record);
static void sonic_board_sound(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static void sonic_edit(struct task_work *work, struct task_motion *motion,
                       struct player *record);
static void sonic_homing_start(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static void sonic_homing(struct task_work *work, struct task_motion *motion,
                         struct player *record);
static struct task_work *sonic_restart_attack(struct task_work *work,
                                              struct player *record);
static Bool sonic_light_attack(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static void sonic_light_dash(struct task_work *work, struct task_motion *motion,
                             struct player *record);
static Bool sonic_try_hurt(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static Bool sonic_try_jump(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static Bool sonic_try_uncurl(struct task_work *work, struct task_motion *motion,
                             struct player *record);
static Bool sonic_follow_event(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static Bool sonic_try_request(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static Bool sonic_try_request_holding(struct task_work *work,
                                      struct task_motion *motion,
                                      struct player *record);
static Bool sonic_try_request_board(struct task_work *work,
                                    struct task_motion *motion,
                                    struct player *record);
static Bool sonic_try_request_super(struct task_work *work,
                                    struct task_motion *motion,
                                    struct player *record);
static Bool sonic_try_fall(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static Bool sonic_try_stop(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static Bool sonic_try_spin(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static Bool sonic_try_light_dash(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record);
static Bool sonic_spin_charge(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static Bool sonic_try_unroll(struct task_work *work, struct task_motion *motion,
                             struct player *record);
static Bool sonic_try_grab(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static Bool sonic_try_take_lifted(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record);
static Bool sonic_try_throw(struct task_work *work, struct task_motion *motion,
                            struct player *record);
static Bool sonic_try_release(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static Bool sonic_try_grip(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static void sonic_let_go(struct task_work *work, struct task_motion *motion,
                         struct player *record);
static void sonic_carry(struct task_work *work, struct task_motion *motion,
                        struct player *record);
static void sonic_circle_place(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static int sonic_circle_step(struct task_work *work, struct task_motion *motion,
                             struct player *record);
static void sonic_dangle_below(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static void sonic_blend_toes(struct task_work *work, struct task_motion *motion,
                             struct player *record);
static void sonic_mouth_start(struct task *task);
static void sonic_sway_destroy(struct task *task);
static void sonic_sway_step(struct task *task);
static Bool sonic_look_create(Sint8 player, NJS_OBJECT **nodes);
static void sonic_look_destroy(struct task *task);
static void sonic_look_step(struct task *task);
static void sonic_dash_create(const struct task_work *work);
static void sonic_super_dash_create(const struct task_work *work);
static void sonic_ball_jitter_create(const struct task_work *work);
static void sonic_ball_follow_create(const struct task_work *work);
static void sonic_super_ball_follow_create(const struct task_work *work);
static void sonic_ball_shrink_create(const struct task_work *work);
static void sonic_super_ball_shrink_create(const struct task_work *work);
static void sonic_charged_step(struct task *task);
static void sonic_charged_destroy(struct task *task);
static void sonic_charged_create(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record);
static void sonic_board_step(struct task *task);
static void sonic_board_draw(struct task *task);
static void sonic_board_destroy(struct task *task);
static void sonic_super_destroy(struct task *task);
static void sonic_super_step(struct task *task);
static void sonic_super_start(struct task *task);

/* the nodes in the model an action moves: his own, his spin's, or a
 * ball's */
#define SONIC_NODES 73
#define SPIN_NODES 74
#define BALL_NODES 1

/* his actions, indexed by enum sonic_action. a row's next is a name from
 * that enum, or a bare index where his code names no row */
static struct player_action sonic_actions[] = {
    {&sonic_stand_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_STAND, 0.0625F,
     0.5F},
    {&sonic_land_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_LANDED, 0.25F,
     0.25F},
    {&sonic_jump_land_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_LANDED,
     0.25F, 0.25F},
    {&sonic_ready_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F,
     1.0F},
    {&sonic_fidget_start_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_FIDGET,
     0.25F, 0.25F},
    {&sonic_fidget_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_FIDGET_END,
     0.25F, 0.25F},
    {&sonic_fidget_start_action, SONIC_NODES, PLAYER_ANIM_BACK, ACTION_STAND,
     0.25F, 0.25F},
    {&sonic_stand_uphill_action, SONIC_NODES, PLAYER_ANIM_LOOP,
     ACTION_STAND_UPHILL, 0.25F, 1.0F},
    {&sonic_stand_downhill_action, SONIC_NODES, PLAYER_ANIM_LOOP,
     ACTION_STAND_DOWNHILL, 0.25F, 1.0F},
    {&sonic_walk_action, SONIC_NODES, PLAYER_ANIM_LOOP_PACE, ACTION_WALK, 0.25F,
     3.4F},
    {&sonic_trot_action, SONIC_NODES, PLAYER_ANIM_LOOP_PACE, ACTION_TROT, 0.25F,
     1.4F},
    {&sonic_run_action, SONIC_NODES, PLAYER_ANIM_LOOP_PACE, ACTION_RUN, 0.25F,
     0.8F},
    {&sonic_sprint_action, SONIC_NODES, PLAYER_ANIM_LOOP_PACE, ACTION_SPRINT,
     0.25F, 0.5F},
    {&sonic_full_speed_action, SONIC_NODES, PLAYER_ANIM_LOOP_SPEED,
     ACTION_FULL_SPEED, 0.5F, 0.5F},
    {&sonic_spin_action, SPIN_NODES, PLAYER_ANIM_LOOP_PACE, ACTION_SPIN, 0.25F,
     0.2F},
    {&sonic_spin_action, SPIN_NODES, PLAYER_ANIM_LOOP_SPEED, ACTION_ROLL, 0.25F,
     0.7F},
    {&sonic_spring_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_SPRING, 0.25F,
     1.0F},
    {&sonic_fall_start_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_FALL,
     0.125F, 1.0F},
    {&sonic_fall_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_FALL, 0.25F,
     1.0F},
    {&sonic_hard_landing_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_STAND,
     1.0F, 1.0F},
    {&sonic_skid_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F,
     1.0F},
    {&sonic_push_start_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_PUSH,
     0.125F, 3.0F},
    {&sonic_push_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_PUSH, 0.25F,
     1.0F},
    {&sonic_hurt_action, SONIC_NODES, PLAYER_ANIM_HOLD, ACTION_HURT, 1.0F,
     1.0F},
    {&sonic_hurt_land_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F,
     1.0F},
    {&sonic_skid_action, SONIC_NODES, PLAYER_ANIM_LOOP, 25, 0.25F, 1.0F},
    {&sonic_updraft_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_UPDRAFT,
     0.0625F, 1.0F},
    {&sonic_crash_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_CRASH_LAND,
     0.5F, 1.0F},
    {&sonic_crash_land_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_STAND,
     0.0625F, 1.0F},
    {&sonic_uncurl_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_LAND, 0.0625F,
     1.0F},
    {&sonic_turn_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F,
     0.9F},
    {&sonic_slide_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_SLIDE, 0.0625F,
     1.0F},
    {&sonic_ball_action, BALL_NODES, PLAYER_ANIM_LOOP, ACTION_BALL, 0.5F, 1.0F},
    {&sonic_lift_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND, 0.5F,
     1.0F},
    {&sonic_hold_run_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_HOLD_RUN,
     0.5F, 1.0F},
    {&sonic_hold_stop_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND,
     0.5F, 1.0F},
    {&sonic_hold_turn_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND,
     0.5F, 1.0F},
    {&sonic_hold_stand_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_HOLD_STAND,
     0.5F, 0.5F},
    {&sonic_hold_walk_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_HOLD_WALK,
     0.5F, 1.0F},
    {&sonic_spring_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_SPRING, 0.25F,
     1.0F},
    {&dat_0c9e28a0, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_FALL, 0.5F, 1.0F},
    {&sonic_spring_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_SPRING, 0.25F,
     1.0F},
    {&dat_0c9e5224, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_FALL, 0.5F, 1.0F},
    {&sonic_spring_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_SPRING, 0.25F,
     1.0F},
    {&dat_0c9dff1c, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_SPRING, 0.5F, 1.0F},
    {&dat_0c9e6e18, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_SPRING, 0.5F, 1.0F},
    {&dat_0ca72374, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_WALK, 0.5F, 0.5F},
    {&sonic_hang_on_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_HANG_ON, 0.5F,
     0.5F},
    {&sonic_pull_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_PULL, 0.5F,
     1.0F},
    {&sonic_shake_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_SHAKE, 0.5F,
     0.5F},
    {&sonic_hold_shake_start_action, SONIC_NODES, PLAYER_ANIM_ONCE,
     ACTION_HOLD_SHAKE, 0.5F, 0.5F},
    {&sonic_hold_shake_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_HOLD_SHAKE,
     0.5F, 0.5F},
    {&sonic_hold_shake_end_action, SONIC_NODES, PLAYER_ANIM_ONCE,
     ACTION_HOLD_STAND, 0.5F, 0.5F},
    {&sonic_grip_start_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_GRIP, 0.5F,
     0.5F},
    {&sonic_grip_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_GRIP, 0.5F,
     0.5F},
    {&sonic_hold_jump_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_HOLD_JUMP,
     0.5F, 1.0F},
    {&sonic_hold_fall_start_action, SONIC_NODES, PLAYER_ANIM_ONCE,
     ACTION_HOLD_FALL, 0.5F, 1.0F},
    {&sonic_hold_fall_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_HOLD_FALL,
     0.5F, 1.0F},
    {&sonic_hold_hard_landing_action, SONIC_NODES, PLAYER_ANIM_ONCE,
     ACTION_HOLD_STAND, 0.5F, 1.0F},
    {&sonic_throw_at_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 0.5F,
     1.0F},
    {&sonic_throw_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 0.5F,
     1.0F},
    {&sonic_air_throw_at_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_FALL,
     0.5F, 1.0F},
    {&sonic_air_throw_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_FALL, 0.5F,
     1.0F},
    {&sonic_lift_action, SONIC_NODES, PLAYER_ANIM_BACK, ACTION_STAND, 1.0F,
     1.0F},
    {&sonic_light_dash_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_SLIDE,
     1.0F, 1.0F},
    {&sonic_ride_action, SONIC_NODES, PLAYER_ANIM_HOLD, ACTION_RIDE, 1.0F,
     0.3F},
    {&sonic_hang_go_action, SONIC_NODES, PLAYER_ANIM_DRIVEN, ACTION_HANG_GO,
     1.0F, 1.0F},
    {&sonic_hang_halfway_action, SONIC_NODES, PLAYER_ANIM_LOOP,
     ACTION_HANG_HALFWAY, 0.25F, 0.25F},
    {&sonic_hang_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_HANG, 0.25F,
     0.25F},
    {&sonic_ledge_action, SONIC_NODES, PLAYER_ANIM_DRIVEN, ACTION_LEDGE, 1.0F,
     1.0F},
    {&sonic_flung_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_FLUNG, 1.0F,
     0.5F},
    {&sonic_glide_action, SONIC_NODES, PLAYER_ANIM_LOOP_SPEED, ACTION_GLIDE,
     0.5F, 2.0F},
    {&sonic_panel_land_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_PANEL_WAIT,
     1.0F, 1.0F},
    {&sonic_panel_jump_action, SONIC_NODES, PLAYER_ANIM_HOLD, ACTION_PANEL_JUMP,
     1.0F, 2.0F},
    {&sonic_jump_land_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_PANEL_WAIT,
     0.25F, 1.0F},
    {&sonic_clear_action, SONIC_NODES, PLAYER_ANIM_HOLD, ACTION_CLEAR, 0.5F,
     1.0F},
    {&dat_0cc45d6c, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 0.5F, 1.0F},
    {&sonic_grip_start_action, SONIC_NODES, PLAYER_ANIM_ONCE, 78, 0.125F, 1.0F},
    {&sonic_grip_action, SONIC_NODES, PLAYER_ANIM_LOOP, 78, 0.125F, 1.0F},
    {&sonic_rocket_along_action, SONIC_NODES, PLAYER_ANIM_LOOP,
     ACTION_ROCKET_ALONG, 0.125F, 1.0F},
    {&sonic_rocket_up_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_ROCKET_UP,
     0.125F, 1.0F},
    {&sonic_caught_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_CAUGHT, 0.125F,
     1.0F},
    {&sonic_hurt_action, SONIC_NODES, PLAYER_ANIM_HOLD, ACTION_ICED, 1.0F,
     15.0F},
    {&sonic_spin_action, SPIN_NODES, PLAYER_ANIM_LOOP_PACE, 83, 1.0F, 0.2F},
    {&sonic_breathe_action, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_FALL, 1.0F,
     0.25F},
    {&sonic_drown_action, SONIC_NODES, PLAYER_ANIM_HOLD, ACTION_DROWN, 1.0F,
     0.5F},
    {&sonic_hurt_action, SONIC_NODES, PLAYER_ANIM_HOLD, ACTION_DOWN, 1.0F,
     1.0F},
    {&sonic_down_action, SONIC_NODES, PLAYER_ANIM_HOLD, ACTION_DOWN_LAND, 0.25F,
     1.0F},
    {&dat_0ca57914, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_PATH_SLIDE, 0.125F,
     1.0F},
    {&dat_0cad6dac, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_CIRCLE, 0.25F, 0.5F},
    {&dat_0cad5248, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_CIRCLE_LEFT, 0.25F,
     2.0F},
    {&dat_0cad1584, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_CIRCLE_RIGHT, 0.25F,
     2.0F},
    {&dat_0cad8f60, SONIC_NODES, PLAYER_ANIM_HOLD, ACTION_CIRCLE_DOWN, 0.25F,
     0.5F},
    {&dat_0cadb8d4, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_CIRCLE, 0.25F, 0.5F},
    {&dat_0caaee64, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_DANGLE, 0.25F, 0.5F},
    {&dat_0cab0ae8, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_DANGLE, 0.25F, 1.0F},
    {&dat_0caa6454, SONIC_NODES, PLAYER_ANIM_ONCE, 97, 1.0F, 1.0F},
    {&dat_0caa7d08, SONIC_NODES, PLAYER_ANIM_DRIVEN, 97, 1.0F, 1.0F},
    {&dat_0caa7d08, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_DANGLE, 1.0F, 1.0F},
    {&dat_0caaaaac, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_DANGLE_LEFT, 0.25F,
     1.0F},
    {&dat_0caad820, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_DANGLE_RIGHT, 0.25F,
     1.0F},
    {&dat_0caad820, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_DANGLE_CLIMB, 0.25F,
     1.0F},
    {&dat_0cb24580, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_BOARD, 0.0625F, 1.0F},
    {&dat_0cb3328c, SONIC_NODES, PLAYER_ANIM_HOLD, ACTION_BOARD_BRAKE_PLUS,
     0.125F, 1.0F},
    {&dat_0cb31228, SONIC_NODES, PLAYER_ANIM_HOLD, ACTION_BOARD_BRAKE_MINUS,
     0.125F, 1.0F},
    {&dat_0cb2980c, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_BOARD_FALL_START,
     0.0625F, 1.0F},
    {&dat_0cb27d88, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_BOARD_AIR, 0.25F,
     1.0F},
    {&dat_0cb35110, SONIC_NODES, PLAYER_ANIM_DRIVEN, ACTION_BOARD_LEAN_PLUS,
     1.0F, 1.0F},
    {&dat_0cb36f84, SONIC_NODES, PLAYER_ANIM_DRIVEN, ACTION_BOARD_LEAN_MINUS,
     1.0F, 1.0F},
    {&dat_0cb2f4b4, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_BOARD_SLOW, 1.0F,
     1.0F},
    {&dat_0cb2c750, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_BOARD_SLOW, 1.0F,
     1.0F},
    {&dat_0cb1e5b4, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_BOARD_FALL, 0.5F,
     1.0F},
    {&dat_0cb20168, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_BOARD_FALL, 0.5F,
     0.3F},
    {&dat_0cb2316c, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_BOARD, 0.5F, 1.0F},
    {&dat_0cb1ad0c, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_BOARD_SLOW, 0.0625F,
     1.0F},
    {&dat_0cb1c930, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_BOARD, 0.0625F, 1.0F},
    {&dat_0cb12974, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_BOARD_FALL, 0.0625F,
     0.85F},
    {&dat_0cb19108, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_BOARD_FALL, 0.0625F,
     0.85F},
    {&dat_0cb46fd8, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_BOARD_FALL, 0.0625F,
     0.85F},
    {&dat_0cb46fd8, SONIC_NODES, PLAYER_ANIM_BACK, ACTION_BOARD_FALL, 0.0625F,
     0.85F},
    {&dat_0cb099dc, SONIC_NODES, PLAYER_ANIM_ONCE, 121, 1.0F, 1.0F},
    {&dat_0cb39f30, SONIC_NODES, PLAYER_ANIM_ONCE, 122, 1.0F, 1.0F},
    {&dat_0cb3b694, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_BOARD_FALL, 0.25F,
     1.0F},
    {&dat_0cb0d880, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_BOARD_FALL, 0.25F,
     1.5F},
    {&dat_0cb26314, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_BOARD_CROUCH, 0.25F,
     1.5F},
    {&sonic_spin_action, SPIN_NODES, PLAYER_ANIM_LOOP_PACE, ACTION_SLOPE_ROLL,
     0.25F, 0.2F},
    {&dat_0cb062e0, SONIC_NODES, PLAYER_ANIM_ONCE, 127, 0.25F, 0.25F},
    {&dat_0cb099f4, SONIC_NODES, PLAYER_ANIM_LOOP, 127, 0.25F, 0.25F},
    {&dat_0cb12948, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F, 0.25F},
    {&dat_0cafe4dc, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F, 0.5F},
    {&super_sonic_stand_action, SONIC_NODES, PLAYER_ANIM_LOOP,
     ACTION_SUPER_STAND, 0.0625F, 0.25F},
    {&super_sonic_walk_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_SUPER_WALK,
     0.25F, 0.5F},
    {&super_sonic_trot_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_SUPER_TROT,
     0.25F, 0.5F},
    {&super_sonic_run_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_SUPER_RUN,
     0.25F, 0.5F},
    {&super_sonic_run_action, SONIC_NODES, PLAYER_ANIM_LOOP,
     ACTION_SUPER_SPRINT, 0.25F, 0.5F},
    {&super_sonic_spring_action, SONIC_NODES, PLAYER_ANIM_LOOP,
     ACTION_SUPER_SPRING, 0.25F, 1.0F},
    {&super_sonic_fall_start_action, SONIC_NODES, PLAYER_ANIM_ONCE,
     ACTION_SUPER_FALL, 0.125F, 1.0F},
    {&super_sonic_fall_action, SONIC_NODES, PLAYER_ANIM_LOOP, ACTION_SUPER_FALL,
     0.25F, 1.0F},
    {&super_sonic_hard_landing_action, SONIC_NODES, PLAYER_ANIM_ONCE,
     ACTION_SUPER_STAND, 1.0F, 1.0F},
    {&super_sonic_clear_action, SONIC_NODES, PLAYER_ANIM_HOLD,
     ACTION_SUPER_CLEAR, 0.5F, 1.0F},
    {&dat_0c914dc0, SONIC_NODES, PLAYER_ANIM_ONCE, ACTION_SUPER_STAND, 0.5F,
     1.0F},
    {&super_sonic_ball_action, BALL_NODES, PLAYER_ANIM_LOOP_PACE,
     ACTION_SUPER_BALL, 0.25F, 0.2F},
};

/* his body's shape. sonic_step gives collision_init a count of 1, so the
 * second shape, which differs only in its attack and guard, goes unused */
/* clang-format off */
static struct collision_shape sonic_shapes[] = {
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS,
     PUSH_ALL | PUSH_FIRM(PUSH_OTHERS), {0.0F, 1.0F, 0.0F}, 4.0F, 4.0F, 0.65F,
     {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(2) | GUARD(3) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES |
         SHAPE_HURTS_OBJECTS,
     PUSH_ALL | PUSH_FIRM(PUSH_OTHERS), {0.0F, 1.0F, 0.0F}, 4.0F, 4.0F, 0.65F,
     {0}, 0, NULL},
};
/* clang-format on */

/* each toe's models, by index. model_blend writes TOE_BLEND, a blend of
 * TOE_SHAPE_A and TOE_SHAPE_B. TOE_MODEL is the toe's own, which his other
 * actions show */
enum toe_model { TOE_BLEND, TOE_SHAPE_A, TOE_SHAPE_B, TOE_MODEL, TOE_MODELS };

static NJS_MODEL *sonic_toe_plus_z_models[TOE_MODELS] = {
    &sonic_toe_plus_z_blend, &sonic_toe_plus_z_shape_a,
    &sonic_toe_plus_z_shape_b, &sonic_toe_plus_z_model};
static NJS_MODEL *sonic_toe_minus_z_models[TOE_MODELS] = {
    &sonic_toe_minus_z_blend, &sonic_toe_minus_z_shape_a,
    &sonic_toe_minus_z_shape_b, &sonic_toe_minus_z_model};

static NJS_OBJECT *sonic_look_nodes[3] = {&sonic_head_joint, &sonic_eye0,
                                          &sonic_eye1};

/* the head's models, by index: the copy sonic_sway_step writes, and the
 * head's own model, which it reads */
enum head_model { HEAD_SWAY, HEAD_OWN, HEAD_MODELS };

static NJS_MODEL *sonic_head_models[HEAD_MODELS] = {&sonic_head_sway_model,
                                                    &sonic_head_model};
/* a copy of sonic_head_models that nothing reads. it holds its place in
 * this object's data, at 0x8c1dc204 */
static NJS_MODEL *sonic_head_models_copy[HEAD_MODELS] = {&sonic_head_sway_model,
                                                         &sonic_head_model};

/* the vertex pairs each weld joins: its vertex in object_a, then in
 * object_b */
static Uint16 sonic_arm_indices[8] = {
#include "player/sonic/sonic_arm_indices.inc"
};
static Uint16 sonic_wrist_indices[8] = {
#include "player/sonic/sonic_wrist_indices.inc"
};
static Uint16 sonic_thigh_indices[8] = {
#include "player/sonic/sonic_thigh_indices.inc"
};
static Uint16 sonic_shin_indices[8] = {
#include "player/sonic/sonic_shin_indices.inc"
};
static Uint16 sonic_shoe_indices[12] = {
#include "player/sonic/sonic_shoe_indices.inc"
};
static Uint16 sonic_hand_indices[8] = {
#include "player/sonic/sonic_hand_indices.inc"
};
/* a copy of sonic_shoe_indices that nothing reads. it holds its place in
 * this object's data, at 0x8c1dc274 */
static Uint16 sonic_shoe_indices_copy[12] = {
#include "player/sonic/sonic_shoe_indices_copy.inc"
};

/* the seams of his model, then those of the light speed shoes, the
 * crystal ring and super Sonic. SONIC_INIT turns super Sonic's off outside
 * the last boss's stage. the welds after them mark points of his fingers,
 * toes and mouth */
static struct weld sonic_welds[] = {
    {&sonic_object, &sonic_upper_arm_minus_z, &sonic_forearm_minus_z, 4,
     WELD_TO_A, 0, 0, NULL, sonic_arm_indices},
    {&sonic_object, &sonic_forearm_minus_z, &sonic_wrist_minus_z, 4, WELD_TO_A,
     0, 0, NULL, sonic_wrist_indices},
    {&sonic_object, &sonic_upper_arm_plus_z, &sonic_forearm_plus_z, 4,
     WELD_TO_A, 0, 0, NULL, sonic_arm_indices},
    {&sonic_object, &sonic_forearm_plus_z, &sonic_wrist_plus_z, 4, WELD_TO_A, 0,
     0, NULL, sonic_wrist_indices},
    {&sonic_object, &sonic_thigh_minus_z, &sonic_shin_minus_z, 4, WELD_TO_A, 0,
     0, NULL, sonic_thigh_indices},
    {&sonic_object, &sonic_shin_minus_z, sonic_foot_minus_z, 4, WELD_TO_A, 0, 0,
     NULL, sonic_shin_indices},
    {&sonic_object, &sonic_thigh_plus_z, &sonic_shin_plus_z, 4, WELD_TO_A, 0, 0,
     NULL, sonic_thigh_indices},
    {&sonic_object, &sonic_shin_plus_z, sonic_foot_plus_z, 4, WELD_TO_A, 0, 0,
     NULL, sonic_shin_indices},
    {&sonic_object, &sonic_shoe_minus_z, &sonic_toe_minus_z, 6, WELD_TO_A, 0, 0,
     NULL, sonic_shoe_indices},
    {&sonic_object, &sonic_shoe_plus_z, &sonic_toe_plus_z, 6, WELD_TO_A, 0, 0,
     NULL, sonic_shoe_indices},
    {&sonic_object, &sonic_hand_plus_z, &sonic_fingers_plus_z, 4, WELD_TO_A, 0,
     0, NULL, sonic_hand_indices},
    {&sonic_object, &sonic_hand_minus_z, &sonic_fingers_minus_z, 4, WELD_TO_A,
     0, 0, NULL, sonic_hand_indices},
    {&sonic_object, &sonic_light_shoe_plus_z, &sonic_light_toe_plus_z, 6,
     WELD_TO_A, 0, 0, NULL, sonic_shoe_indices},
    {&sonic_object, &sonic_light_shoe_minus_z, &sonic_light_toe_minus_z, 6,
     WELD_TO_A, 0, 0, NULL, sonic_shoe_indices},
    {&sonic_object, &sonic_forearm_minus_z, &sonic_ring_wrist, 4, WELD_TO_A, 0,
     0, NULL, sonic_wrist_indices},
    {&super_sonic_object, &dat_0c905708, &dat_0c905890, 4, WELD_TO_A, 0, 0,
     NULL, sonic_arm_indices},
    {&super_sonic_object, &dat_0c905890, &dat_0c90647c, 4, WELD_TO_A, 0, 0,
     NULL, sonic_wrist_indices},
    {&super_sonic_object, &dat_0c9046e4, &dat_0c90486c, 4, WELD_TO_A, 0, 0,
     NULL, sonic_arm_indices},
    {&super_sonic_object, &dat_0c90486c, &dat_0c905464, 4, WELD_TO_A, 0, 0,
     NULL, sonic_wrist_indices},
    {&super_sonic_object, &dat_0c903504, &dat_0c90368c, 4, WELD_TO_A, 0, 0,
     NULL, sonic_thigh_indices},
    {&super_sonic_object, &dat_0c90368c, &dat_0c904430, 4, WELD_TO_A, 0, 0,
     NULL, sonic_shin_indices},
    {&super_sonic_object, &dat_0c902334, &dat_0c9024bc, 4, WELD_TO_A, 0, 0,
     NULL, sonic_thigh_indices},
    {&super_sonic_object, &dat_0c9024bc, &dat_0c903278, 4, WELD_TO_A, 0, 0,
     NULL, sonic_shin_indices},
    {&super_sonic_object, &dat_0c903fec, &dat_0c9039fc, 6, WELD_TO_A, 0, 0,
     NULL, sonic_shoe_indices},
    {&super_sonic_object, &dat_0c902e34, &dat_0c902838, 6, WELD_TO_A, 0, 0,
     NULL, sonic_shoe_indices},
    {&super_sonic_object, &dat_0c905020, &dat_0c904bac, 4, WELD_TO_A, 0, 0,
     NULL, sonic_hand_indices},
    {&super_sonic_object, &dat_0c906044, &dat_0c905bd0, 4, WELD_TO_A, 0, 0,
     NULL, sonic_hand_indices},
    {&sonic_object, &sonic_fingers_minus_z, NULL, 2, WELD_MARK, 0, 0, NULL,
     NULL},
    {&sonic_object, &sonic_fingers_plus_z, NULL, 2, WELD_MARK + 1, 0, 0, NULL,
     NULL},
    {&sonic_object, &sonic_toe_minus_z, NULL, 0, WELD_MARK + 2, 0, 0, NULL,
     NULL},
    {&sonic_object, &sonic_toe_plus_z, NULL, 0, WELD_MARK + 3, 0, 0, NULL,
     NULL},
    {&sonic_object, &sonic_mouth, NULL, 0, WELD_MARK + 4, 0, 0, NULL, NULL},
    {NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL},
};

/* a table that nothing reads. it holds its place in this object's data,
 * between sonic_welds and sonic_board_actions */
static Uint16 dat_8c1dc5a4[22] = {0x0020, 0x0120, 0x0220, 0x0320, 0x0420,
                                  0x0520, 0x0620, 0x0720, 0x0820, 0x0920,
                                  0x0a20, 0x0b20, 0x0c20, 0x0d20, 0x0e20,
                                  0x0f20, 0x1020, 0x1120, 0x1220, 0x1320};

/* the board's own action for each of his board actions, by index from
 * ACTION_BOARD. null draws the board as its model has it */
#define BOARD_ACTIONS (ACTION_BOARD_CROUCH - ACTION_BOARD + 1)
static NJS_ACTION *sonic_board_actions[BOARD_ACTIONS] = {
    NULL,          &dat_0cb38c24, &dat_0cb38af0, &dat_0cb38574, &dat_0cb38440,
    &dat_0cb38e68, &dat_0cb390ac, &dat_0cb389bc, &dat_0cb38798, &dat_0cb381c8,
    &dat_0cb382fc, NULL,          NULL,          NULL,          &dat_0cb37c70,
    &dat_0cb38094, &dat_0cb47e34, &dat_0cb47e34, &dat_0cb375b8, &dat_0cb470fc,
    &dat_0cb47230, &dat_0cb3784c, NULL,
};

/* the board's own textures. sonic_board_start picks the first of
 * sonic_board_texlists or s_sbmot.PRS's sb_board1_texlist by the play
 * time. sonic_board_texlist_count is 1, so the other three lists go unused */
static NJS_TEXNAME sonic_board_textures[] = {{TEXT_1, 0, 0}};
static NJS_TEXLIST sonic_board_texlists[4] = {
#include "player/sonic/sonic_board_texlists.inc"
};
static int sonic_board_texlist_count = 1;

/* add a trick's points to the score, in the trial menu's mini game
 * alone */
static void sonic_board_score(int points)
{
    if (game_mode == GAME_MODE_TRIAL &&
        menu_state.trial[0] == MENU_TRIAL_MINI_GAME) {
        score_add(points);
    }
}

/* choose his mode for this step, then move, pose and draw him by it */
void sonic_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;

    /* coming down onto a task, he stands on it */
    if ((work->flags & COLLISION_ON_TOP) && motion->speed.y <= 0.0F) {
        work->flags |= PLAYER_ON_TASK;
        /* @bug logic: he copies his own tilt, not the task's. his collision
         * record's owner is his own task, which collision_init stores
         * (MOV.L R3,@(16,R14) at 0x8c030154). these stores, through it, copy
         * his angles back unchanged (MOV.L @(16,R1),R0 at 0x8c0ad93a and
         * 0x8c0ad948) */
        work->ang.x = work->collision->owner->work->ang.x;
        work->ang.z = work->collision->owner->work->ang.z;
    } else {
        work->flags &= ~PLAYER_ON_TASK;
    }
    if (edit_mode == 1) {
        work->mode = SONIC_EDIT;
    } else if (work->mode != SONIC_DOWN) {
        sonic_choose_mode(work, motion, record);
    }
    /* his shape's attack and guard levels: the highest while he's
     * invincible or super, raised while he attacks, and 0 at rest */
    if (work->collision != NULL) {
        Uint32 attack;
        Uint32 guard;
        struct collision_shape *shape = work->collision->shapes;

        if (record->effects & PLAYER_INVINCIBLE) {
            attack = 3;
            guard = 3;
        } else if (record->upgrades & PLAYER_SONIC_SUPER) {
            attack = 3;
            guard = 3;
        } else if (work->mode == SONIC_LIGHT_ATTACK) {
            attack = 2;
            guard = 3;
        } else if (work->mode == SONIC_LIGHT_DASH) {
            attack = 1;
            guard = 3;
        } else if (work->flags & PLAYER_BALL) {
            attack = 1;
            guard = 1;
        } else if (work->mode == SONIC_RIDE) {
            attack = 0;
            guard = 1;
        } else {
            attack = 0;
            guard = 0;
        }
        shape->levels = (shape->levels & ~SHAPE_ATTACK) | ATTACK(attack);
        shape->levels = (shape->levels & ~SHAPE_GUARD) | GUARD(guard);
    }
    if (work->flags & PLAYER_BALL) {
        record->own.sonic.curled_steps += 1;
    }
    /* @hack spelling: modes that move him alike keep cases of their own,
     * since the image keeps a copy of the calls for each. with each such
     * case's label moved to the first case that makes the same calls,
     * sonic.c's code runs 480 bytes short and 2031 of sonic_step's 2810
     * words differ */
    switch (work->mode) {
    case SONIC_INIT:
        PLAYER_VARS(work)->character = CHARACTER_SONIC;
        player_init(PLAYER_VARS(work)->player, task);
        record = motion->record;
        record->anim.actions = sonic_actions;
        record->anim.action = ACTION_STAND;
        collision_init(task, sonic_shapes, COLLISION_PLAYER, 1, 1);
        work->collision->shapes->radius = record->consts.radius;
        work->mode = SONIC_STAND;
        record->idle_steps = 0;
        texture_load_character(CHARACTER_SONIC);
        record->own.sonic.toe_blend_plus_z = 0.0F;
        record->own.sonic.toe_blend_minus_z = 0.0F;
        sonic_look_create(PLAYER_VARS(work)->player, sonic_look_nodes);
        player_anim_step(&record->anim);
        /* super Sonic's seams join only in the last boss's stage */
        /* @hack spelling: two bare scopes, whose locals share a frame slot:
         * weld, then child, at @(8,R15) (MOV.L R0,@(8,R15) at 0x8c0adcc2); in
         * one block, child takes @(4,R15) and 2 of 2810 words differ */
        {
            struct weld *weld = sonic_welds;

            if (IN_STAGE(STAGE_CHAOS7)) {
                for (; weld->base != NULL; weld++) {
                    if (weld->base == &super_sonic_object) {
                        weld->kind = WELD_TO_A;
                    }
                }
            } else {
                for (; weld->base != NULL; weld++) {
                    if (weld->base == &super_sonic_object) {
                        weld->kind = WELD_OFF;
                    }
                }
            }
        }
        record->anim.welds = sonic_welds;
        player_update_welds(work, motion, record);
        {
            struct task *child;

            child = task_create(TASK_WORK, 3, sonic_mouth_start);
            if (child != NULL) {
                PLAYER_VARS(child->work)->player = PLAYER_VARS(work)->player;
            }
            child = task_create(TASK_MOTION | TASK_WORK, 3, sonic_sway_step);
            if (child != NULL) {
                PLAYER_VARS(child->work)->player = PLAYER_VARS(work)->player;
            }
        }
        task->destructor = sonic_destroy;
        task->draw = sonic_draw;
        break;
    case SONIC_STAND:
        player_turn_to_stick(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        sonic_set_stand_action(work, motion, record);
        break;
    case SONIC_WALK:
        if (record->anim.previous == ACTION_TURN &&
            record->anim.current != ACTION_TURN) {
            record->speed.x += record->consts.trot_speed;
        }
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        if (player_collide(work, motion, record) == PLAYER_CRASHED) {
            sound_play(SE_CRASH, 0, 0, 0);
            work->mode = SONIC_CRASH;
            record->anim.action = ACTION_CRASH;
            player_crash_burst(work);
            record->speed.x = -(record->speed.x * CRASH_REBOUND);
            record->speed.z = -(record->speed.z * CRASH_REBOUND);
        } else {
            player_take_speed(work, motion, record);
            sonic_set_walk_action(work, motion, record);
        }
        break;
    case SONIC_PATH:
        player_accel_path(work, motion, record);
        player_apply_path_accel(work, motion, record);
        player_move_on_path(work, motion, record);
        sonic_set_walk_action(work, motion, record);
        break;
    case SONIC_SPIN_CHARGE: {
        int i;

        player_turn_to_stick(work, motion, record);
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        sonic_ball_jitter_create(work);
        if ((record->upgrades & PLAYER_SONIC_LIGHT_SHOES) &&
            rand() * RAND_SCALE < 0.1F) {
            for (i = 0; i < 3; i++) {
                charge_create(work, 31);
            }
        }
        break;
    }
    case SONIC_SPIN_DASH:
        player_turn_to_stick(work, motion, record);
        player_accel_roll(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        if (record->speed.x >= record->consts.running_speed) {
            sonic_dash_create(work);
            record->own.sonic.dashes[0] |= 1;
            sonic_ball_follow_create(work);
        }
        break;
    case SONIC_LIGHT_DASH:
        sonic_light_dash(work, motion, record);
        if (player_collide(work, motion, record) == PLAYER_CRASHED) {
            sound_play(SE_CRASH, 0, 0, 0);
            work->mode = SONIC_CRASH;
            record->anim.action = ACTION_CRASH;
            work->flags &= ~PLAYER_GUARD;
            record->own.sonic.dash_steps = 0;
            sound_stop(SE_LIGHT_DASH);
            player_crash_burst(work);
        } else {
            player_take_speed(work, motion, record);
            sonic_dash_create(work);
            record->own.sonic.dashes[0] |= 1;
        }
        break;
    case SONIC_PATH_SPIN:
        player_accel_roll(work, motion, record);
        player_apply_path_accel(work, motion, record);
        player_move_on_path(work, motion, record);
        sonic_ball_follow_create(work);
        break;
    case SONIC_JUMP:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_SPRING:
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_LAUNCH:
        if (record->speed.y >= record->consts.launch_speed) {
            player_accel_launched(work, motion, record);
        } else {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_FLUNG:
        player_accel_gravity(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_FALL:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_STOP:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_HOMING:
        player_turn_upright(work, motion, record);
        sonic_homing(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        sonic_dash_create(work);
        record->own.sonic.dashes[0] |= 1;
        sonic_ball_follow_create(work);
        break;
    case SONIC_PUSH:
        if (record->anim.action == ACTION_PUSH_START) {
            player_accel_brake(work, motion, record);
        } else {
            player_accel_push(work, motion, record);
            if (player_carry_flags(work) & PLAYER_CAN_PUSH) {
                sound_play_for(SE_PUSH, (int)work, 1, 0, 2);
            }
            if (record->anim.previous == ACTION_PUSH_START) {
                sound_play(SE_PUSH_START, 0, 0, 0);
            }
        }
        player_apply_push(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_HURT:
        if (work->flags & PLAYER_ON_GROUND) {
            player_turn_upright(work, motion, record);
        }
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_BACK:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_CRASH:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_SLIDE:
        player_turn_to_stick(work, motion, record);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_HANG:
    case SONIC_HANG_GO:
        player_slide_on_path(work, motion, record);
        sonic_hang_move(work, motion, record);
        break;
    case SONIC_LEDGE:
    case SONIC_LEDGE_GO:
        player_shift_on_path(work, motion, record);
        sonic_ledge_move(work, motion, record);
        break;
    case SONIC_PANEL_JUMP:
        if (panel_fly(work, motion, record) <= 31) {
            record->anim.action = ACTION_PANEL_LAND;
        }
        break;
    case SONIC_PANEL_WAIT:
        break;
    case SONIC_CAUGHT:
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_ICED: {
        Float stroke;
        Angle ang;

        player_mouth_set_script(0, 30);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        /* the stick shakes him in the ice, and on the ground he hops now
         * and then */
        if (player_read_stick(work, &ang, &stroke)) {
            motion->speed.x = stroke * (0.5F - (rand() * RAND_SCALE));
            if ((work->flags & PLAYER_ON_GROUND) &&
                rand() * RAND_SCALE > 0.8F) {
                motion->speed.y = (rand() * (0.2F * RAND_SCALE)) + 0.3F;
            }
            motion->speed.z = stroke * (0.5F - (rand() * RAND_SCALE));
        }
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    }
    case SONIC_LIGHT_ATTACK:
        player_turn_upright(work, motion, record);
        if (!sonic_light_attack(work, motion, record)) {
            work->mode = SONIC_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ATTACKING;
        }
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        sonic_dash_create(work);
        record->own.sonic.dashes[0] |= 1;
        break;
    case SONIC_POSE:
        break;
    case SONIC_LIFT:
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_PUT_DOWN:
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_HOLD_STAND:
        player_turn_to_stick(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_GRIP:
        player_accel_push(work, motion, record);
        player_apply_push(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_THROW:
        player_turn_to_target(work, motion, record);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_HOLD_WALK:
        if (record->anim.previous == ACTION_HOLD_TURN &&
            record->anim.current != ACTION_HOLD_TURN) {
            record->speed.x += record->consts.trot_speed;
        }
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        sonic_set_walk_action(work, motion, record);
        break;
    case SONIC_HOLD_STOP:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_HOLD_FALL:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_HOLD_JUMP:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_HOLD_SPRING:
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_HOLD_LAUNCH:
        if (record->speed.y >= record->consts.launch_speed) {
            player_accel_launched(work, motion, record);
        } else {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_PULL:
        player_accel_push(work, motion, record);
        player_apply_push(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        if (player_carry_flags(work) & PLAYER_CAN_PUSH) {
            sound_play_for(SE_PUSH, (int)work, 1, 0, 2);
        }
        break;
    case SONIC_AIR_THROW:
        player_turn_upright(work, motion, record);
        player_turn_to_target(work, motion, record);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_SHAKE:
        player_shake_touched(work, motion, record);
        break;
    case SONIC_HOLD_SHAKE:
        record->held->motion->accel.x = record->held->motion->accel.y =
            record->held->motion->accel.z = record->anim.frame * 0.01F;
        break;
    case SONIC_RIDE:
        break;
    case SONIC_WALL_RUN:
        player_accel_wall(work, motion, record);
        player_apply_accel(work, motion, record);
        if (player_collide_wall(work, motion, record) == PLAYER_CRASHED) {
            sound_play(SE_CRASH, 0, 0, 0);
            work->mode = SONIC_WALL_CRASH;
            record->anim.action = ACTION_WALL_CRASH;
            player_crash_burst(work);
            work->pos.x += record->contact->normal.x * WALL_CRASH_PUSH;
            work->pos.y += record->contact->normal.y * WALL_CRASH_PUSH;
            work->pos.z += record->contact->normal.z * WALL_CRASH_PUSH;
            record->speed.x *= WALL_CRASH_SLOW;
            record->speed.y = record->consts.jump_speed;
            work->ang.x = 0;
            work->ang.z = 0xc000;
        } else {
            player_take_wall_speed(work, motion, record);
            sonic_set_walk_action(work, motion, record);
        }
        break;
    case SONIC_WALL_SPIN:
        player_accel_wall(work, motion, record);
        player_apply_accel(work, motion, record);
        if (player_collide_wall(work, motion, record) == PLAYER_CRASHED) {
            sound_play(SE_CRASH, 0, 0, 0);
            work->flags &= ~PLAYER_ATTACKING;
            work->mode = SONIC_WALL_CRASH;
            record->anim.action = ACTION_WALL_CRASH;
            player_crash_burst(work);
            work->pos.x += record->contact->normal.x * WALL_CRASH_PUSH;
            work->pos.y += record->contact->normal.y * WALL_CRASH_PUSH;
            work->pos.z += record->contact->normal.z * WALL_CRASH_PUSH;
            record->speed.x *= WALL_CRASH_SLOW;
            record->speed.y = record->consts.jump_speed;
            work->ang.x = 0;
            work->ang.z = 0xc000;
        } else {
            player_take_wall_speed(work, motion, record);
        }
        if (record->speed.x >= record->consts.running_speed) {
            sonic_dash_create(work);
            record->own.sonic.dashes[0] |= 1;
            sonic_ball_follow_create(work);
        }
        break;
    case SONIC_WALL_JUMP:
        player_turn_to_wall(work, motion, record);
        player_accel_wall(work, motion, record);
        player_apply_accel(work, motion, record);
        if (player_collide_wall(work, motion, record) == PLAYER_CRASHED) {
            sound_play(SE_CRASH, 0, 0, 0);
            work->mode = SONIC_WALL_CRASH;
            record->anim.action = ACTION_WALL_CRASH;
            player_crash_burst(work);
            work->pos.x += record->contact->normal.x * WALL_CRASH_PUSH;
            work->pos.y += record->contact->normal.y * WALL_CRASH_PUSH;
            work->pos.z += record->contact->normal.z * WALL_CRASH_PUSH;
            record->speed.x *= WALL_CRASH_SLOW;
            record->speed.y = record->consts.jump_speed;
            work->ang.x = 0;
            work->ang.z = 0xc000;
            work->flags &= ~PLAYER_ATTACKING;
        } else {
            player_take_wall_speed(work, motion, record);
        }
        player_take_wall_speed(work, motion, record);
        break;
    case SONIC_WALL_FALL:
        player_turn_to_wall(work, motion, record);
        player_accel_wall(work, motion, record);
        player_apply_accel(work, motion, record);
        if (player_collide_wall(work, motion, record) == PLAYER_CRASHED) {
            sound_play(SE_CRASH, 0, 0, 0);
            work->mode = SONIC_WALL_CRASH;
            record->anim.action = ACTION_WALL_CRASH;
            player_crash_burst(work);
            work->pos.x += record->contact->normal.x * WALL_CRASH_PUSH;
            work->pos.y += record->contact->normal.y * WALL_CRASH_PUSH;
            work->pos.z += record->contact->normal.z * WALL_CRASH_PUSH;
            record->speed.x *= WALL_CRASH_SLOW;
            record->speed.y = record->consts.jump_speed;
            work->ang.x = 0;
            work->ang.z = 0xc000;
        } else {
            player_take_wall_speed(work, motion, record);
        }
        player_take_wall_speed(work, motion, record);
        break;
    case SONIC_WALL_CRASH:
        player_accel_wall(work, motion, record);
        player_apply_accel(work, motion, record);
        work->pos.y += motion->speed.y;
        player_take_wall_speed(work, motion, record);
        break;
    case SONIC_CIRCLE:
        sonic_circle_place(work, motion, record);
        break;
    case SONIC_CIRCLE_DOWN: {
        struct task_work *held;
        Float top;

        held = record->held->work;
        work->pos.y -= CIRCLE_DROP;
        top = held->pos.y + held->collision->shapes->center.y -
              held->collision->shapes->size_y;
        if (top >= work->pos.y) {
            work->pos.y = top;
        }
        sonic_circle_place(work, motion, record);
        break;
    }
    case SONIC_CIRCLE_LEFT:
        work->ang.y += sonic_circle_step(work, motion, record);
        sonic_circle_place(work, motion, record);
        break;
    case SONIC_CIRCLE_RIGHT:
        work->ang.y -= sonic_circle_step(work, motion, record);
        sonic_circle_place(work, motion, record);
        break;
    case SONIC_DANGLE:
        sonic_dangle_below(work, motion, record);
        break;
    case SONIC_DANGLE_SLIP:
        sonic_dangle_below(work, motion, record);
        break;
    case SONIC_DANGLE_LEFT:
        work->ang.y += DANGLE_TURN;
        work->ang.y &= 0xfe00;
        sonic_dangle_below(work, motion, record);
        break;
    case SONIC_DANGLE_RIGHT:
        work->ang.y -= DANGLE_TURN;
        work->ang.y &= 0xfe00;
        sonic_dangle_below(work, motion, record);
        break;
    case SONIC_DANGLE_LET_GO:
        sonic_dangle_below(work, motion, record);
        break;
    case SONIC_DANGLE_CLIMB:
        work->pos.y += DANGLE_CLIMB_SPEED;
        sonic_dangle_below(work, motion, record);
        break;
    case SONIC_DANGLE_LEAP:
        player_turn_upright(work, motion, record);
        player_accel_roll(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_BOARD:
        player_accel_board(work, motion, record, PLAYER_BOARD_TOP_SPEED);
        player_apply_accel(work, motion, record);
        if (player_collide(work, motion, record) == PLAYER_CRASHED) {
            if (PLAYER_VARS(work)->player == 0) {
                sound_play(SE_CRASH, 0, 0, 0);
            }
            work->mode = SONIC_BOARD_CRASH;
            if (vector_angle(&motion->speed, &record->wall_normal,
                             &record->contact->normal) > 0x8000) {
                record->anim.action = ACTION_BOARD_CRASH_MINUS;
            } else {
                record->anim.action = ACTION_BOARD_CRASH_PLUS;
            }
            player_crash_burst(work);
            if (stage_get_stage_act() == STAGE_ACT(STAGE_ICECAP, 2)) {
                sound_play(SE_BOARD_CRASH, 0, 0, 0);
            }
        } else {
            player_take_speed(work, motion, record);
            sonic_set_board_action(work, motion, record);
        }
        player_board_spray(work, motion, record);
        break;
    case SONIC_BOARD_BRAKE:
        player_accel_board_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        player_board_spray(work, motion, record);
        break;
    case SONIC_BOARD_FALL:
        player_turn_upright(work, motion, record);
        player_accel_board_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_BOARD_JUMP:
        player_turn_upright(work, motion, record);
        player_accel_board_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_BOARD_TRICK:
        player_turn_upright(work, motion, record);
        player_accel_board_trick(work, motion, record);
        player_apply_accel(work, motion, record);
        if (player_collide(work, motion, record)) {
            record->anim.action = ACTION_BOARD_FALL;
        }
        player_take_speed(work, motion, record);
        break;
    case SONIC_BOARD_CRASH:
        player_accel_board_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        player_board_spray(work, motion, record);
        break;
    case SONIC_SLOPE_ROLL:
        player_accel_slope(work, motion, record);
        player_collide(work, motion, record);
        if (work->flags & PLAYER_ON_LAND) {
            motion->speed.y = 0.0F;
        }
        player_take_speed(work, motion, record);
        if (record->speed.x >= record->consts.running_speed) {
            sonic_dash_create(work);
            record->own.sonic.dashes[0] |= 1;
            sonic_ball_follow_create(work);
        }
        break;
    case SONIC_SLOPE_ROLL_FALL:
        player_turn_upright(work, motion, record);
        player_accel_roll(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        if (record->speed.x >= record->consts.running_speed) {
            sonic_dash_create(work);
            record->own.sonic.dashes[0] |= 1;
            sonic_ball_follow_create(work);
        }
        break;
    case SONIC_SUPER_STAND:
        player_turn_to_stick(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_SUPER_WALK:
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        if (player_collide(work, motion, record) == PLAYER_CRASHED) {
            work->mode = SONIC_SUPER_STAND;
            record->anim.action = ACTION_SUPER_STAND;
            record->speed.x = -(record->speed.x * CRASH_REBOUND);
            record->speed.z = -(record->speed.z * CRASH_REBOUND);
        } else {
            player_take_speed(work, motion, record);
            sonic_super_set_walk_action(work, motion, record);
        }
        break;
    case SONIC_SUPER_PATH:
        player_accel_path(work, motion, record);
        player_apply_path_accel(work, motion, record);
        player_move_on_path(work, motion, record);
        sonic_super_set_walk_action(work, motion, record);
        sonic_super_dash_create(work);
        record->own.sonic.dashes[0] |= 1;
        break;
    case SONIC_SUPER_JUMP:
        player_turn_upright(work, motion, record);
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_SUPER_SPRING:
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_SUPER_LAUNCH:
        if (record->speed.y >= record->consts.launch_speed) {
            player_accel_launched(work, motion, record);
        } else {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_SUPER_FALL:
        player_turn_upright(work, motion, record);
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_SUPER_HOMING:
        player_turn_upright(work, motion, record);
        sonic_homing(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        sonic_super_dash_create(work);
        record->own.sonic.dashes[0] |= 1;
        sonic_super_ball_follow_create(work);
        break;
    case SONIC_PLAY:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_EVENT:
        event_paths_step(work);
        break;
    case SONIC_DROWN:
        /* once ACTION_DROWN has played, he rises while the water's surface
         * is above his shape's center */
        if ((record->anim.flags & PLAYER_ACTION_DONE) &&
            record->shadow.above > work->collision->shapes->center.y &&
            (record->shadow.above_flags & WORLD_WATER)) {
            record->speed.y =
                record->shadow.above - work->collision->shapes->center.y;
            if (record->speed.y > DROWN_RISE) {
                record->speed.y = DROWN_RISE;
            }
        } else {
            record->speed.y = 0.0F;
        }
        player_turn_upright(work, motion, record);
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_DOWN:
        if (work->flags & PLAYER_ON_GROUND) {
            record->anim.action = ACTION_DOWN_LAND;
        }
        player_turn_upright(work, motion, record);
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case SONIC_EDIT:
        sonic_edit(work, motion, record);
        break;
    }
    player_anim_step(&record->anim);
    /* uncurled, he wears the light speed shoes and the crystal ring if he
     * has them */
    if (!(work->flags & PLAYER_BALL)) {
        if (record->upgrades & PLAYER_SONIC_LIGHT_SHOES) {
            sonic_foot_plus_z->sibling = &sonic_light_shoe_plus_z;
            sonic_foot_minus_z->sibling = &sonic_light_shoe_minus_z;
        } else {
            sonic_foot_plus_z->sibling = &sonic_shoe_plus_z;
            sonic_foot_minus_z->sibling = &sonic_shoe_minus_z;
        }
        if (record->upgrades & PLAYER_SONIC_CRYSTAL_RING) {
            sonic_ring_mount->sibling = &sonic_ring_wrist;
        } else {
            sonic_ring_mount->sibling = &sonic_wrist_minus_z;
        }
        player_update_welds(work, motion, record);
    }
    sonic_move_shape(work, motion, record);
    sonic_draw(task);
    if (record->held != NULL) {
        sonic_carry(work, motion, record);
    }
    /* @hack spelling: dashes[0] = dashes[0] << 1, which forms the byte's
     * address twice, for the load and the store (0x8c0af890 to 0x8c0af89e);
     * as <<=, SHC forms it once and 53 of 2810 words differ */
    record->own.sonic.dashes[0] = record->own.sonic.dashes[0] << 1;
    shadow_update(work, &record->shadow);
    /* in the clear's poses and while he hangs below a task, he stays out
     * of collision, and his record forgets its hits */
    if (record->anim.current != ACTION_CLEAR &&
        record->anim.current != ACTION_SUPER_CLEAR &&
        (work->mode < SONIC_DANGLE || work->mode > SONIC_DANGLE_CLIMB)) {
        collision_register(work);
        work->collision->radius =
            record->consts.height + njScalor(&record->speed);
    } else {
        work->collision->hit_shape = -1;
        work->collision->hit = NULL;
        work->collision->flags &=
            ~(COLLISION_HIT | COLLISION_HIT_TOP | COLLISION_HIT_SIDE |
              COLLISION_HIT_BOTTOM | COLLISION_PUSHED);
    }
    player_finish_step(work, motion, record);
}

static void sonic_stand(struct task_work *work, struct task_motion *motion,
                        struct player *record)
{
    work->mode = SONIC_STAND;
    record->idle_steps = 0;
    sonic_set_stand_action(work, motion, record);
}

/* choose his mode for this step. a hit comes first, though none lands
 * while he's invincible, or while work->timer counts down from the last */
/* NOLINTNEXTLINE(readability-function-size) */
static void sonic_choose_mode(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    if (record != NULL && (record->effects & PLAYER_INVINCIBLE)) {
        work->flags |= PLAYER_RECOILING;
        work->timer = 0;
    } else if (!work->timer) {
        if (sonic_try_hurt(work, motion, record)) {
            work->flags |= PLAYER_RECOILING;
            if (work->flags & PLAYER_HOLDING) {
                sonic_let_go(work, motion, record);
            }
            work->flags &= ~(PLAYER_ON_PATH | PLAYER_REQUEST | PLAYER_HOLDING |
                             SONIC_CHARGED | PLAYER_BALL);
            if (record->effects & (PLAYER_BARRIER | PLAYER_MAGNETIC_BARRIER)) {
                record->effects &= ~(PLAYER_BARRIER | PLAYER_MAGNETIC_BARRIER);
            } else {
                ring_lose(PLAYER_VARS(work)->player);
            }
        } else {
            work->flags &= ~PLAYER_RECOILING;
        }
    } else {
        work->timer--;
        work->flags &= ~PLAYER_HURT;
    }

    /* SHC picks a scratch register for each jump in this switch, even one
     * it deletes. the hacks tagged below, but for the test whose arms do
     * nothing and the repeated test against trot_speed, steer SHC by that */
    switch (work->mode) {
    case SONIC_STAND:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_light_dash(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            break;
        }
        if (sonic_try_jump(work, motion, record)) {
            break;
        }
        if (sonic_try_grab(work, motion, record)) {
            break;
        }
        if (sonic_try_spin(work, motion, record)) {
            break;
        }
        /* @hack redundant: an else after an arm that breaks, which puts the
         * record in R3 and its contact in R2 for the slope's test (0x8c0afc0c,
         * 0x8c0afc10); without it, R1 and R3, and 3 of 7698 words differ */
        if (player_read_stick(work, NULL, NULL)) {
            work->mode = SONIC_WALK;
            record->anim.action = ACTION_WALK;
            break;
        } else if (record->contact->normal.y < 0.77F &&
                   njScalor(&record->speed) > record->consts.slide_speed) {
            work->mode = SONIC_SLIDE;
            record->anim.action = ACTION_SLIDE;
        }
        break;
    case SONIC_WALK:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_light_dash(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record) ||
            sonic_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (sonic_try_grab(work, motion, record)) {
            break;
        }
        if (sonic_try_spin(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (sonic_try_stop(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        /* against a wall that he can push and that he faces head on, he
         * pushes */
        if ((work->collision->flags & COLLISION_PUSHED) &&
            !(work->flags & PLAYER_HURT) &&
            !(work->collision->hit->shapes[work->collision->hit_shape].flags &
              SHAPE_NO_PUSH) &&
            (work->collision->flags & COLLISION_HIT_SIDE)) {
            /* @hack spelling: a switch of one case, which puts work in R2 and
             * SONIC_PUSH in R3 for the arm's stores (0x8c0afdd4, 0x8c0afdd6);
             * as an if, the two trade and 6 of 7698 words differ */
            switch (player_stick_into_wall(work, motion)) {
            case PLAYER_WALL_HEAD_ON:
                work->mode = SONIC_PUSH;
                record->anim.action = ACTION_PUSH_START;
                work->flags &= ~PLAYER_ON_PATH;
                break;
            }
        }
        if (player_stick_back_or_end_event(work, motion) &&
            record->speed.x >= record->consts.trot_speed) {
            work->mode = SONIC_STOP;
            record->anim.action = ACTION_SKID;
            sound_play(SE_SKID, 0, 0, 0);
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case SONIC_PATH:
        work->flags &= ~PLAYER_ATTACKING;
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_light_dash(work, motion, record) ||
            sonic_try_fall(work, motion, record) ||
            sonic_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (record->speed.x < record->consts.trot_speed) {
            work->mode = SONIC_WALK;
            record->anim.action = ACTION_WALK;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (sonic_try_stop(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case SONIC_SPIN_CHARGE:
        if (sonic_try_request(work, motion, record)) {
            sound_stop(SE_SPIN);
            sound_stop(SE_LIGHT_CHARGE);
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            record->anim.action = ACTION_SPIN;
            record->action_speed = JUMP_SPIN_PACE;
            work->flags |= PLAYER_ATTACKING;
            sound_stop(SE_SPIN);
            sound_stop(SE_LIGHT_CHARGE);
            break;
        }
        if (sonic_spin_charge(work, motion, record)) {
            break;
        }
        break;
    case SONIC_SPIN_DASH:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            record->anim.action = ACTION_SPIN;
            record->action_speed = JUMP_SPIN_PACE;
            work->flags |= PLAYER_ATTACKING;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (sonic_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (sonic_try_unroll(work, motion, record)) {
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                        pad_action_buttons)) {
            work->mode = SONIC_WALK;
            record->anim.action = ACTION_UNCURL;
            work->flags &= ~PLAYER_ATTACKING;
            /* @hack redundant: a break where another follows, which leaves a
             * NOP in the JMP's delay slot (0x8c0b0102); without it, the flags'
             * store fills the slot and 51 of 7698 words differ */
            break;
        }
        break;
    case SONIC_LIGHT_DASH: {
        NJS_POINT3 pos;

        if (sonic_try_request(work, motion, record)) {
            record->own.sonic.dash_steps = 0;
            sound_stop(SE_LIGHT_DASH);
            if (record->speed.x > LIGHT_DASH_END_SPEED) {
                record->speed.x = LIGHT_DASH_END_SPEED;
            }
            work->flags &= ~PLAYER_GUARD;
            break;
        }
        /* @hack spelling: an empty arm for a dash with steps left, with the
         * stop in the else, which puts work in R2 and SONIC_STOP in R3 for its
         * stores (0x8c0b01b4, 0x8c0b01b6); as one test of the steps run out,
         * the two trade and 8 of 7698 words differ */
        if (record->own.sonic.dash_steps-- > 0) {
        } else {
            work->mode = SONIC_STOP;
            record->anim.action = ACTION_SKID;
            if (record->speed.x > LIGHT_DASH_END_SPEED) {
                record->speed.x = LIGHT_DASH_END_SPEED;
            }
            work->flags &= ~PLAYER_GUARD;
            record->own.sonic.dash_steps = 0;
            sound_stop(SE_LIGHT_DASH);
            break;
        }
        /* pos is where his history has him a step back */
        player_get_history(PLAYER_VARS(work)->player, 1, &pos, NULL);
        if (!(njDistanceP2P(&pos, &work->pos) > LIGHT_DASH_STOP)) {
            work->mode = SONIC_STOP;
            record->anim.action = ACTION_SKID;
            if (record->speed.x > LIGHT_DASH_END_SPEED) {
                record->speed.x = LIGHT_DASH_END_SPEED;
            }
            work->flags &= ~PLAYER_GUARD;
            record->own.sonic.dash_steps = 0;
            sound_stop(SE_LIGHT_DASH);
            /* @hack redundant: a break where another follows, which keeps this
             * arm's tail for the stop in the else above to join (BRA at
             * 0x8c0b01e0); without it, that stop keeps a tail of its own, the
             * code runs longer and 6830 of 7698 words differ */
            break;
        }
        break;
    }
    case SONIC_PATH_SPIN:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            record->anim.action = ACTION_SPIN;
            record->action_speed = JUMP_SPIN_PACE;
            work->ang.x = work->ang.z = 0;
            work->flags |= PLAYER_ATTACKING;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (record->speed.x < PLAYER_SPIN_SPEED) {
            record->anim.action = ACTION_SPIN;
            record->action_speed = JUMP_SPIN_PACE;
            work->ang.x = work->ang.z = 0;
            work->flags |= PLAYER_ATTACKING;
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case SONIC_JUMP:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_light_dash(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (PLAYER_VARS(work)->player == 0) {
                sound_play(SE_LAND, 0, 0, 0);
            }
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = SONIC_STOP;
                record->anim.action = ACTION_SKID;
            } else if (sonic_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (motion->speed.y > HARD_LANDING_SPEED) {
                    record->anim.action = ACTION_LANDED;
                } else {
                    record->anim.action = ACTION_HARD_LANDING;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = SONIC_WALK;
                sonic_set_walk_action(work, motion, record);
            }
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (sonic_try_uncurl(work, motion, record)) {
            break;
        }
        if (player_jump_pressed(work, record)) {
            work->mode = SONIC_HOMING;
            sonic_homing_start(work, motion, record);
            break;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.8F;
        }
        break;
    case SONIC_SPRING:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_light_dash(work, motion, record)) {
            break;
        }
        if (motion->speed.y <= 0.0F) {
            work->mode = SONIC_FALL;
            if (record->anim.action == ACTION_BOUNCE_1) {
                record->anim.action = ACTION_BOUNCE_1_FALL;
            } else if (record->anim.action == ACTION_BOUNCE_2) {
                record->anim.action = ACTION_BOUNCE_2_FALL;
            } else {
                record->anim.action = ACTION_FALL_START;
            }
        }
        break;
    case SONIC_LAUNCH:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_light_dash(work, motion, record)) {
            break;
        }
        if (motion->speed.y <= 0.0F) {
            work->mode = SONIC_FALL;
            record->anim.action = ACTION_FALL_START;
        }
        break;
    case SONIC_FLUNG:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_light_dash(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (PLAYER_VARS(work)->player == 0) {
                sound_play(SE_LAND, 0, 0, 0);
            }
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = SONIC_STOP;
                record->anim.action = ACTION_SKID;
            } else if (sonic_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (motion->speed.y > HARD_LANDING_SPEED) {
                    record->anim.action = ACTION_LANDED;
                } else {
                    record->anim.action = ACTION_HARD_LANDING;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = SONIC_WALK;
                sonic_set_walk_action(work, motion, record);
            }
        }
        break;
    case SONIC_FALL:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_light_dash(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (PLAYER_VARS(work)->player == 0) {
                sound_play(SE_LAND, 0, 0, 0);
            }
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = SONIC_STOP;
                record->anim.action = ACTION_SKID;
            } else if (sonic_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (motion->speed.y > HARD_LANDING_SPEED) {
                    record->anim.action = ACTION_LAND;
                } else {
                    record->anim.action = ACTION_HARD_LANDING;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = SONIC_WALK;
                sonic_set_walk_action(work, motion, record);
            }
        }
        break;
    case SONIC_STOP:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_light_dash(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            break;
        }
        if (sonic_try_jump(work, motion, record)) {
            break;
        }
        if (record->speed.x <= 0.0F &&
            player_stick_back_or_end_event(work, motion)) {
            work->mode = SONIC_WALK;
            record->anim.action = ACTION_TURN;
            player_turn_to_stick(work, motion, record);
            work->ang.y = motion->aim.y;
            break;
        }
        if (record->anim.action != ACTION_SKID) {
            sonic_stand(work, motion, record);
        }
        break;
    case SONIC_HOMING: {
        struct collision_work *hit;

        if (sonic_try_request(work, motion, record)) {
            break;
        }
        /* the attack ends after HOMING_STEPS, or as he hits a player, an
         * item or an enemy */
        hit = work->collision->hit;
        if (record->own.sonic.steps++ >= HOMING_STEPS ||
            (hit != NULL &&
             (hit->kind == COLLISION_PLAYER || hit->kind == COLLISION_ITEM ||
              hit->kind == COLLISION_ENEMY))) {
            record->own.sonic.dash_steps = -1;
            work->mode = SONIC_JUMP;
            break;
        }
        if (record->own.sonic.dash_steps >= HOMING_UNAIMED_STEPS ||
            (pad_records[PLAYER_VARS(work)->player].press &
             pad_action_buttons)) {
            record->own.sonic.dash_steps = 0;
            work->mode = SONIC_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (PLAYER_VARS(work)->player == 0) {
                sound_play(SE_LAND, 0, 0, 0);
            }
            work->flags &= ~PLAYER_ATTACKING;
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = SONIC_STOP;
                record->anim.action = ACTION_SKID;
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
            } else if (sonic_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (motion->speed.y > HARD_LANDING_SPEED) {
                    record->anim.action = ACTION_LANDED;
                } else {
                    record->anim.action = ACTION_HARD_LANDING;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = SONIC_WALK;
                sonic_set_walk_action(work, motion, record);
            }
        }
        break;
    }
    case SONIC_PUSH:
        if (sonic_try_request(work, motion, record) ||
            sonic_try_light_dash(work, motion, record) ||
            sonic_try_jump(work, motion, record) ||
            sonic_try_fall(work, motion, record) ||
            sonic_try_stop(work, motion, record)) {
            sound_stop(SE_PUSH);
            break;
        }
        if ((work->collision->flags & COLLISION_PUSHED) &&
            !(work->flags & PLAYER_HURT) &&
            !(work->collision->hit->shapes[work->collision->hit_shape].flags &
              SHAPE_NO_PUSH) &&
            (work->collision->flags & COLLISION_HIT_SIDE)) {
            switch (player_stick_into_wall(work, motion)) {
            case PLAYER_WALL_HEAD_ON:
                break;
            case PLAYER_WALL_ASKEW:
                if (!(work->flags & PLAYER_HOLDING)) {
                    work->mode = SONIC_WALK;
                    record->anim.action = ACTION_WALK;
                    sound_stop(SE_PUSH);
                }
                break;
            default:
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = SONIC_PULL;
                    record->anim.action = ACTION_PULL;
                } else {
                    work->mode = SONIC_WALK;
                    record->anim.action = ACTION_WALK;
                }
                sound_stop(SE_PUSH);
                break;
            }
            break;
        }
        sonic_let_go(work, motion, record);
        work->mode = SONIC_WALK;
        record->anim.action = ACTION_WALK;
        sound_stop(SE_PUSH);
        break;
    case SONIC_HURT:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_HURT &&
            record->anim.action != ACTION_HURT_LAND) {
            /* @hack redundant: an else after an arm that breaks, which puts
             * work in R2 and SONIC_WALK in R3 for its stores (0x8c0b0e04,
             * 0x8c0b0e06); without it, the two trade and 3 of 7698 words
             * differ */
            if (sonic_try_stop(work, motion, record)) {
                sonic_stand(work, motion, record);
                break;
            } else {
                work->mode = SONIC_WALK;
                sonic_set_walk_action(work, motion, record);
                break;
            }
        } else if (TASK_TIMER(work) <= 0) {
            /* @hack redundant: an else after an arm that breaks, which puts
             * work in R2 and SONIC_FALL in R3 for its stores (0x8c0b0e78,
             * 0x8c0b0e7a); without it, the two trade and 8 of 7698 words
             * differ */
            if (work->flags & PLAYER_ON_GROUND) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = SONIC_WALK;
                sonic_set_walk_action(work, motion, record);
                break;
            } else {
                work->mode = SONIC_FALL;
                record->anim.action = ACTION_FALL;
                break;
            }
        }
        if (TASK_TIMER(work) <= 60 && player_read_stick(work, NULL, NULL)) {
            if (sonic_try_jump(work, motion, record)) {
                break;
            }
            if (work->flags & PLAYER_ON_GROUND) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = SONIC_WALK;
                sonic_set_walk_action(work, motion, record);
                /* @hack redundant: a break where another follows, which joins
                 * this arm to SONIC_BACK's sonic_set_walk_action, as this
                 * case's other walk arms join it (BRA at 0x8c0b0eec); without
                 * it, this arm makes its own call and 3 of 7698 words differ */
                break;
            }
            break;
        }
        if (TASK_TIMER(work) == PLAYER_HURT_STEPS) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            record->anim.action = ACTION_HURT_LAND;
        }
        break;
    case SONIC_BACK:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_light_dash(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            break;
        }
        if (sonic_try_jump(work, motion, record)) {
            break;
        }
        if (sonic_try_stop(work, motion, record)) {
            break;
        }
        if (!(record->speed.x < 0.0F)) {
            work->mode = SONIC_WALK;
            sonic_set_walk_action(work, motion, record);
            /* @hack redundant: a break where another follows, which keeps here
             * the sonic_set_walk_action that SONIC_HURT's walk arms join
             * (0x8c0b0fec); without it, SONIC_HURT's stick arm keeps the call
             * its other walk arms join, this arm makes its own, and 10 of 7698
             * words differ */
            break;
        }
        break;
    case SONIC_POSE:
        if (record->upgrades & PLAYER_SONIC_SUPER) {
            if (sonic_try_request_super(work, motion, record)) {
                break;
            }
        } else if (work->flags & PLAYER_HOLDING) {
            if (sonic_try_request_holding(work, motion, record)) {
                break;
            }
        } else if (sonic_try_request(work, motion, record)) {
            break;
        }
        break;
    case SONIC_CRASH:
        if (record->anim.action == ACTION_CRASH) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_CRASH_LAND) {
            sonic_stand(work, motion, record);
        }
        break;
    case SONIC_SLIDE:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_light_dash(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            break;
        }
        if (sonic_try_jump(work, motion, record)) {
            break;
        }
        if (sonic_try_spin(work, motion, record)) {
            break;
        }
        if (player_read_stick(work, NULL, NULL)) {
            work->mode = SONIC_WALK;
            record->anim.action = ACTION_WALK;
            break;
        }
        if (njScalor(&motion->speed) < record->consts.slide_speed ||
            record->contact->normal.y > 0.98F) {
            work->mode = SONIC_STAND;
            record->idle_steps = 0;
            record->anim.action = ACTION_LANDED;
        }
        break;
    case SONIC_HANG: {
        Angle ang;

        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_light_dash(work, motion, record) ||
            sonic_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (work->flags & PLAYER_ON_PATH) {
            if (player_read_stick(work, &ang, NULL)) {
                /* ACTION_HANG_GO starts at the frame of the still action
                 * he leaves: 0 from ACTION_HANG, and HANG_HALFWAY from
                 * ACTION_HANG_HALFWAY */
                if (angle_distance(work->ang.y, ang) < 0x4000) {
                    record->speed.x = HANG_SPEED;
                    if (record->anim.action == ACTION_HANG) {
                        record->anim.start_frame = 0.0F;
                    } else {
                        record->anim.start_frame = HANG_HALFWAY;
                    }
                } else {
                    record->speed.x = -HANG_SPEED;
                    if (record->anim.action == ACTION_HANG) {
                        record->anim.start_frame = 0.0F;
                    } else {
                        record->anim.start_frame = HANG_HALFWAY;
                    }
                }
                work->mode = SONIC_HANG_GO;
                record->anim.action = ACTION_HANG_GO;
                break;
            }
        } else {
            work->mode = SONIC_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                        pad_action_buttons)) {
            work->mode = SONIC_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a break where another follows, which leaves a
             * NOP in the JMP's delay slot (0x8c0b13e4); without it, the flags'
             * store fills the slot and 16 of 7698 words differ */
            break;
        }
        break;
    }
    case SONIC_HANG_GO:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_light_dash(work, motion, record) ||
            sonic_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        /* @hack spelling: an empty arm for the path, with the fall in the else,
         * which puts work in R2 and SONIC_FALL in R1 for its stores
         * (0x8c0b1476, 0x8c0b1478); as one test of !PLAYER_ON_PATH, the two
         * trade and 6 of 7698 words differ */
        if (work->flags & PLAYER_ON_PATH) {
        } else {
            work->mode = SONIC_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                        pad_action_buttons)) {
            work->mode = SONIC_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a break where another follows, which leaves a
             * NOP in the JMP's delay slot (0x8c0b14d8); without it, the flags'
             * store fills the slot and 16 of 7698 words differ */
            break;
        }
        break;
    case SONIC_LEDGE: {
        Angle ang;

        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_light_dash(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (sonic_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        /* @hack redundant: an else after an arm that always breaks, which puts
         * work in R1 and SONIC_FALL in R2 for its stores (0x8c0b1612,
         * 0x8c0b1614); without it, R2 and R3, and 8 of 7698 words differ */
        if (work->flags & PLAYER_ON_PATH) {
            if (!player_read_stick(work, &ang, NULL)) {
                break;
            }
            /* he moves along the ledge one way or the other, as the stick
             * points, seen from the camera */
            /* @hack redundant: an else after an arm that breaks, which puts the
             * record in R2 for its store and JMP (0x8c0b1602, 0x8c0b160c);
             * without it, R3, and 10 of 7698 words differ */
            if (angle_distance(0xc000 - camera_work->ang.y, ang) < 0x4000) {
                record->speed.x = HANG_SPEED;
                /* @hack redundant: a break that keeps the two arms apart, each
                 * with its own store and JMP (0x8c0b15a0, 0x8c0b160c);
                 * without it, SHC joins the else's store and jump to this
                 * arm's and 78 of 7698 words differ */
                break;
            } else {
                record->speed.x = -HANG_SPEED;
                /* @hack redundant: a break where another follows, which leaves
                 * a NOP in the JMP's delay slot (0x8c0b160e); without it, the
                 * store fills the slot and 72 of 7698 words differ */
                break;
            }
            /* @hack redundant: a break that no path reaches, which puts work
             * in R1 and SONIC_FALL in R2 for the fall's stores (0x8c0b1612,
             * 0x8c0b1614); without it, the two trade and 6 of 7698 words
             * differ */
            break;
        } else {
            work->mode = SONIC_FALL;
            record->anim.action = ACTION_FALL;
            player_clear_speed(motion, record);
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
    }
    case SONIC_LEDGE_GO:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_light_dash(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (sonic_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        /* @hack spelling: an empty arm for the path, with the fall in the else,
         * which puts work in R2 and SONIC_FALL in R1 for its stores
         * (0x8c0b16d4, 0x8c0b16d6); as one test of !PLAYER_ON_PATH, the two
         * trade and 6 of 7698 words differ */
        if (work->flags & PLAYER_ON_PATH) {
        } else {
            work->mode = SONIC_FALL;
            record->anim.action = ACTION_FALL;
            player_clear_speed(motion, record);
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        break;
    case SONIC_PANEL_JUMP:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (panel_touched(work)) {
            if (PLAYER_VARS(work)->player == 0) {
                sound_play(SE_LAND, 0, 0, 0);
            }
            player_clear_speed(motion, record);
            work->mode = SONIC_PANEL_WAIT;
            record->anim.action = ACTION_PANEL_WAIT;
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (PLAYER_VARS(work)->player == 0) {
                sound_play(SE_LAND, 0, 0, 0);
            }
            player_clear_speed(motion, record);
            work->mode = SONIC_STAND;
            record->idle_steps = 0;
            record->anim.action = ACTION_LANDED;
        }
        break;
    case SONIC_PANEL_WAIT:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (panel_jump_ready_or_idle(work) <= 0) {
            work->mode = SONIC_STAND;
            record->idle_steps = 0;
            record->anim.action = ACTION_LANDED;
            break;
        }
        if (player_jump_pressed(work, record) == PLAYER_JUMP_PANEL) {
            panel_jump(work);
            work->mode = SONIC_PANEL_JUMP;
            record->anim.action = ACTION_PANEL_JUMP;
            sound_play(SE_JUMP, 0, 0, 0);
        }
        break;
    case SONIC_CAUGHT:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_light_dash(work, motion, record)) {
            break;
        }
        if (pad_press_live[PLAYER_VARS(work)->player] & PAD_MASH_BUTTONS) {
            record->own.sonic.steps -= STRUGGLE_PRESS;
        }
        if (record->own.sonic.steps > 0) {
            break;
        }
        sonic_stand(work, motion, record);
        break;
    case SONIC_ICED:
        if (sonic_try_request(work, motion, record) ||
            sonic_try_light_dash(work, motion, record)) {
            player_mouth_stop(PLAYER_VARS(work)->player);
            sound_play(SE_ICE_BREAK, 0, 0, 0);
            break;
        }
        /* a press of PAD_MASH_BUTTONS chips STRUGGLE_PRESS steps off the ice,
         * and a step with no press chips one */
        if (pad_press_live[PLAYER_VARS(work)->player] & PAD_MASH_BUTTONS) {
            record->own.sonic.steps -= STRUGGLE_PRESS;
            /* @hack redundant: a goto to the next statement, which loads 0x82,
             * the offset of steps, into R0 and the record into R2 for the
             * else's subtraction (0x8c0b1982, 0x8c0b1984); without it, the two
             * trade and 2 of 7698 words differ */
            goto thawed;
        } else {
            record->own.sonic.steps -= 1;
        }
    thawed:
        if (record->own.sonic.steps <= 0) {
            work->mode = SONIC_JUMP;
            record->speed.y = record->consts.jump_speed;
            record->anim.action = ACTION_SPIN;
            record->action_speed = JUMP_SPIN_PACE;
            work->flags &= ~PLAYER_ON_TASK;
            work->flags |= PLAYER_ATTACKING;
            record->jump_steps = 0;
            sound_play(SE_JUMP, 0, 0, 0);
            sound_play(SE_ICE_BREAK, 0, 0, 0);
            player_mouth_stop(PLAYER_VARS(work)->player);
        }
        break;
    case SONIC_PLAY:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (!(record->anim.flags & PLAYER_ACTION_DONE)) {
            break;
        }
        sonic_stand(work, motion, record);
        break;
    case SONIC_LIGHT_ATTACK:
        /* he attacks while there's a target, until the action button is
         * pressed */
        /* @bug logic: steps++ > LIGHT_ATTACK_STEPS never holds.
         * sonic_restart_attack, called first in the same test, sets steps
         * to 0 (MOV.W R3,@(R0,R5) at 0x8c0b6146), so the test reads 0
         * (0x8c0b1a9a) and the attack never runs out of steps */
        /* @hack spelling: an empty arm for the attack going on, with its end in
         * the else, which puts work in R1 and SONIC_FALL in R3 for its stores
         * (0x8c0b1aac, 0x8c0b1aae); as one test of !(...), R3 and R2, and 8
         * of 7698 words differ */
        if (sonic_restart_attack(work, record) &&
            !PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                         pad_action_buttons) &&
            !(record->own.sonic.steps++ > LIGHT_ATTACK_STEPS)) {
        } else {
            Float jump;

            work->mode = SONIC_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ATTACKING;
            jump = record->consts.jump_speed;
            motion->speed.x = world_gravity.x * jump;
            motion->speed.y = world_gravity.y * jump;
            motion->speed.z = world_gravity.z * jump;
            record->speed.x = 0.0F;
            record->speed.y = jump;
            record->speed.z = 0.0F;
            camera_restore();
            break;
        }
        /* @hack redundant: a test of the kind of what he hits, whose arms do
         * nothing (0x8c0b1b60 to 0x8c0b1b9a, the last CMP/EQ in the JMP's
         * delay slot); without it, the code runs shorter and 4456 of 7698
         * words differ */
        if (work->collision->flags & COLLISION_HIT) {
            if (work->collision->hit->kind == COLLISION_ITEM ||
                work->collision->hit->kind == COLLISION_ENEMY) {
            }
        }
        break;
    case SONIC_LIFT:
        if (work->flags & PLAYER_HOLDING) {
            if (sonic_try_request_holding(work, motion, record)) {
                break;
            }
        } else if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            break;
        }
        if (sonic_try_take_lifted(work, motion, record)) {
            break;
        }
        break;
    case SONIC_GRIP:
        if (sonic_try_grip(work, motion, record)) {
            break;
        }
        if (sonic_try_jump(work, motion, record)) {
            break;
        }
        break;
    case SONIC_HOLD_STAND:
        if (sonic_try_request_holding(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                        pad_grab_buttons) &&
            player_read_stick(work, NULL, NULL) &&
            record->held->work->id == CARRY_KIND_SMALL) {
            work->mode = SONIC_HOLD_SHAKE;
            record->anim.action = ACTION_HOLD_SHAKE_START;
            break;
        }
        if (sonic_try_throw(work, motion, record)) {
            break;
        }
        if (sonic_try_jump(work, motion, record)) {
            break;
        }
        if (player_read_stick(work, NULL, NULL)) {
            work->mode = SONIC_HOLD_WALK;
            record->anim.action = ACTION_HOLD_WALK;
        }
        break;
    case SONIC_HOLD_WALK:
        if (sonic_try_request_holding(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record) ||
            sonic_try_jump(work, motion, record) ||
            sonic_try_throw(work, motion, record) ||
            sonic_try_stop(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (sonic_try_throw(work, motion, record)) {
            break;
        }
        if (player_stick_back_or_end_event(work, motion) &&
            record->speed.x >= record->consts.trot_speed) {
            work->mode = SONIC_HOLD_STOP;
            record->anim.action = ACTION_HOLD_STOP;
            sound_play(SE_SKID, 0, 0, 0);
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a break where another follows, which puts work
             * in R3 for SONIC_PUT_DOWN's test (MOV.L @(R0,R15),R3 at
             * 0x8c0b1e90); without it, R1, and 3 of 7698 words differ */
            break;
        }
        break;
    case SONIC_PUT_DOWN:
    case SONIC_THROW:
        if (work->flags & PLAYER_HOLDING) {
            if (sonic_try_request_holding(work, motion, record)) {
                break;
            }
        } else if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_release(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_THROW_AT &&
            record->anim.action != ACTION_THROW &&
            record->anim.action != ACTION_PUT_DOWN) {
            sonic_stand(work, motion, record);
        }
        break;
    case SONIC_HOLD_STOP:
        if (sonic_try_request_holding(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            break;
        }
        if (sonic_try_jump(work, motion, record)) {
            break;
        }
        if (record->speed.x <= 0.0F &&
            player_stick_back_or_end_event(work, motion)) {
            work->mode = SONIC_HOLD_WALK;
            record->anim.action = ACTION_HOLD_TURN;
            player_turn_to_stick(work, motion, record);
            work->ang.y = motion->aim.y;
            break;
        }
        if (record->anim.action != ACTION_HOLD_STOP) {
            work->mode = SONIC_HOLD_STAND;
            record->anim.action = ACTION_HOLD_STAND;
        }
        break;
    case SONIC_HOLD_FALL:
        if (sonic_try_request_holding(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (PLAYER_VARS(work)->player == 0) {
                sound_play(SE_LAND, 0, 0, 0);
            }
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = SONIC_HOLD_STOP;
                record->anim.action = ACTION_HOLD_STOP;
            } else if (sonic_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (motion->speed.y > HARD_LANDING_SPEED) {
                    record->anim.action = ACTION_HOLD_STAND;
                } else {
                    record->anim.action = ACTION_HOLD_HARD_LANDING;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = SONIC_HOLD_WALK;
                sonic_set_walk_action(work, motion, record);
            }
            break;
        }
        if (sonic_try_throw(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case SONIC_HOLD_JUMP:
        if (sonic_try_request_holding(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (PLAYER_VARS(work)->player == 0) {
                sound_play(SE_LAND, 0, 0, 0);
            }
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = SONIC_HOLD_STOP;
                record->anim.action = ACTION_HOLD_STOP;
            } else if (sonic_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (motion->speed.y > HARD_LANDING_SPEED) {
                    record->anim.action = ACTION_HOLD_STAND;
                } else {
                    record->anim.action = ACTION_HOLD_HARD_LANDING;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = SONIC_HOLD_WALK;
                sonic_set_walk_action(work, motion, record);
            }
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.4F;
        }
        if (sonic_try_throw(work, motion, record)) {
            break;
        }
        if (motion->speed.y <= 0.0F) {
            work->mode = SONIC_HOLD_FALL;
            record->anim.action = ACTION_HOLD_FALL_START;
        }
        break;
    case SONIC_HOLD_SPRING:
        if (motion->speed.y <= 0.0F) {
            work->mode = SONIC_HOLD_FALL;
            record->anim.action = ACTION_HOLD_FALL_START;
        }
        break;
    case SONIC_HOLD_LAUNCH:
        if (motion->speed.y <= 0.0F) {
            work->mode = SONIC_HOLD_FALL;
            record->anim.action = ACTION_HOLD_FALL_START;
        }
        break;
    case SONIC_PULL:
        if (sonic_try_request(work, motion, record) ||
            sonic_try_jump(work, motion, record) ||
            sonic_try_fall(work, motion, record)) {
            sound_stop(SE_PUSH);
            break;
        }
        if (!PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                         pad_grab_buttons)) {
            sonic_stand(work, motion, record);
            sonic_let_go(work, motion, record);
            sound_stop(SE_PUSH);
        }
        if ((work->collision->flags & COLLISION_PUSHED) &&
            !(work->flags & PLAYER_HURT) &&
            !(work->collision->hit->shapes[work->collision->hit_shape].flags &
              SHAPE_NO_PUSH) &&
            (work->collision->flags & COLLISION_HIT_SIDE)) {
            switch (player_stick_into_wall(work, motion)) {
            case PLAYER_WALL_HEAD_ON:
                work->mode = SONIC_PUSH;
                record->anim.action = ACTION_PUSH_START;
                sound_stop(SE_PUSH);
                break;
            case PLAYER_WALL_NO_STICK:
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = SONIC_GRIP;
                    record->anim.action = ACTION_GRIP;
                    sound_stop(SE_PUSH);
                }
                break;
            default:
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = SONIC_PULL;
                    record->anim.action = ACTION_PULL;
                } else {
                    work->mode = SONIC_WALK;
                    record->anim.action = ACTION_WALK;
                }
                sound_stop(SE_PUSH);
                break;
            }
            break;
        } else {
            sonic_let_go(work, motion, record);
            work->mode = SONIC_WALK;
            record->anim.action = ACTION_WALK;
            sound_stop(SE_PUSH);
            /* @hack redundant: a break where another follows, which keeps here
             * the sound_stop that the light dash's stops, SONIC_PUSH's stops
             * and SONIC_PULL's first test jump to, each with its sound in R4
             * (0x8c0b259a); without it, SONIC_PULL's first test keeps the call
             * and 32 of 7698 words differ */
            break;
        }
        /* @hack redundant: a break that no path reaches, which puts work in
         * R2 for SONIC_AIR_THROW's test (0x8c0b25aa); without it, R3, and 3
         * of 7698 words differ */
        break;
    case SONIC_AIR_THROW:
        if (work->flags & PLAYER_HOLDING) {
            if (sonic_try_request_holding(work, motion, record)) {
                break;
            }
        } else if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (PLAYER_VARS(work)->player == 0) {
                sound_play(SE_LAND, 0, 0, 0);
            }
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = SONIC_HOLD_STOP;
                record->anim.action = ACTION_HOLD_STOP;
            } else if (sonic_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (work->flags & PLAYER_HOLDING) {
                    if (motion->speed.y > HARD_LANDING_SPEED) {
                        record->anim.action = ACTION_HOLD_STAND;
                    } else {
                        record->anim.action = ACTION_HOLD_HARD_LANDING;
                    }
                } else if (motion->speed.y > HARD_LANDING_SPEED) {
                    sonic_set_stand_action(work, motion, record);
                } else {
                    record->anim.action = ACTION_HARD_LANDING;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = SONIC_HOLD_WALK;
                } else {
                    work->mode = SONIC_WALK;
                }
                sonic_set_walk_action(work, motion, record);
            }
            break;
        }
        if (sonic_try_release(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_AIR_THROW_AT &&
            record->anim.action != ACTION_AIR_THROW) {
            work->mode = SONIC_FALL;
            record->anim.action = ACTION_FALL;
        }
        break;
    case SONIC_SHAKE:
        if (sonic_try_request_holding(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            break;
        }
        if (sonic_try_jump(work, motion, record)) {
            break;
        }
        if (player_touches_shakable(work, motion, record)) {
            if (!player_read_stick(work, NULL, NULL)) {
                work->mode = SONIC_GRIP;
                record->anim.action = ACTION_GRIP;
            }
            break;
        }
        sonic_stand(work, motion, record);
        sonic_let_go(work, motion, record);
        break;
    case SONIC_HOLD_SHAKE:
        if (sonic_try_request_holding(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            break;
        }
        if (sonic_try_jump(work, motion, record)) {
            break;
        }
        if (record->held == NULL ||
            record->held->work->id != CARRY_KIND_SMALL) {
            sonic_stand(work, motion, record);
            sonic_let_go(work, motion, record);
            break;
        }
        if (!player_read_stick(work, NULL, NULL)) {
            work->mode = SONIC_HOLD_STAND;
            record->anim.action = ACTION_HOLD_SHAKE_END;
            /* @hack redundant: a break where another follows, which puts work
             * in R3 for SONIC_RIDE's test (MOV.L @(R0,R15),R3 at 0x8c0b29a2);
             * without it, R1, and 3 of 7698 words differ */
            break;
        }
        break;
    case SONIC_RIDE:
        /* @hack redundant: the else before the ride-off test, after an arm that
         * breaks. SONIC_ICED's first arm jumps to the down arm's sound_play
         * with SE_ICE_BREAK in R4 (JMP at 0x8c0b1914 to 0x8c0b2a12). this
         * else, the else before the down test and the down arm's second
         * break each keep that jump; without this one, SONIC_ICED makes its
         * own call and 106 of 7698 words differ */
        /* @hack redundant: the else before the down test, after an arm that
         * breaks, which keeps SONIC_ICED's jump to the down arm's
         * sound_play; without it, 93 of 7698 words differ */
        if (!(work->flags & PLAYER_REQUEST)) {
            break;
        } else if (work->submode == PLAYER_MODE_RIDE_OFF) {
            work->mode = SONIC_SPRING;
            record->anim.action = ACTION_SPRING;
            work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
            break;
        } else if (work->submode == PLAYER_MODE_DOWN) {
            work->mode = SONIC_DOWN;
            record->anim.action = ACTION_DOWN;
            work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
            sound_play(SE_SONIC_DOWN, 0, 0, 0);
            /* @hack redundant: a break where another follows, which keeps
             * SONIC_ICED's jump to this arm's sound_play; without it, 43 of
             * 7698 words differ */
            break;
        }
        break;
    case SONIC_WALL_RUN:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            work->mode = SONIC_WALL_FALL;
            break;
        }
        if (sonic_try_jump(work, motion, record)) {
            record->speed.y = record->consts.jump_speed * 1.05F;
            work->mode = SONIC_WALL_JUMP;
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                        pad_action_buttons)) {
            work->mode = SONIC_WALL_SPIN;
            record->anim.action = ACTION_ROLL;
            sound_play(SE_SPIN_DASH, 0, 0, 0);
            work->flags |= PLAYER_ATTACKING;
        }
        break;
    case SONIC_WALL_SPIN:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            work->mode = SONIC_WALL_FALL;
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (sonic_try_jump(work, motion, record)) {
            work->mode = SONIC_WALL_JUMP;
            record->speed.y = record->consts.jump_speed * 1.05F;
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                        pad_action_buttons)) {
            work->mode = SONIC_WALL_RUN;
            sonic_set_walk_action(work, motion, record);
            work->flags &= ~PLAYER_ATTACKING;
        }
        break;
    case SONIC_WALL_JUMP:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (PLAYER_VARS(work)->player == 0) {
                sound_play(SE_LAND, 0, 0, 0);
            }
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            work->mode = SONIC_WALL_RUN;
            sonic_set_walk_action(work, motion, record);
            work->flags &= ~PLAYER_ATTACKING;
        }
        break;
    case SONIC_WALL_FALL:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (PLAYER_VARS(work)->player == 0) {
                sound_play(SE_LAND, 0, 0, 0);
            }
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            work->mode = SONIC_WALL_RUN;
            sonic_set_walk_action(work, motion, record);
        }
        break;
    case SONIC_WALL_CRASH:
        if (record->anim.action != ACTION_WALL_CRASH) {
            work->mode = SONIC_WALL_RUN;
            record->anim.action = ACTION_WALK;
        }
        break;
    case SONIC_CIRCLE:
        if (sonic_try_request(work, motion, record) ||
            sonic_try_jump(work, motion, record)) {
            record->held = NULL;
            break;
        }
        if (pad_records[PLAYER_VARS(work)->player].x1 << 8 < -PAD_DEAD_ZONE) {
            work->mode = SONIC_CIRCLE_LEFT;
            record->anim.action = ACTION_CIRCLE_LEFT;
            break;
        }
        if (pad_records[PLAYER_VARS(work)->player].x1 << 8 > PAD_DEAD_ZONE) {
            work->mode = SONIC_CIRCLE_RIGHT;
            record->anim.action = ACTION_CIRCLE_RIGHT;
            break;
        }
        if (work->mode < SONIC_CIRCLE || work->mode > SONIC_CIRCLE_RIGHT) {
            record->held = NULL;
        }
        break;
    case SONIC_CIRCLE_DOWN:
        if (sonic_try_request(work, motion, record) ||
            sonic_try_jump(work, motion, record)) {
            record->held = NULL;
            break;
        }
        if (record->held != NULL) {
            struct task_work *held = record->held->work;

            if (held->pos.y + held->collision->shapes->center.y -
                    held->collision->shapes->size_y >=
                work->pos.y) {
                work->mode = SONIC_CIRCLE;
                record->anim.action = ACTION_CIRCLE_LAND;
                break;
            }
        } else {
            sonic_stand(work, motion, record);
            break;
        }
        if (work->mode < SONIC_CIRCLE || work->mode > SONIC_CIRCLE_RIGHT) {
            record->held = NULL;
        }
        break;
    case SONIC_CIRCLE_LEFT:
        if (sonic_try_request(work, motion, record) ||
            sonic_try_jump(work, motion, record)) {
            record->held = NULL;
            break;
        }
        if (pad_records[PLAYER_VARS(work)->player].x1 << 8 > -PAD_DEAD_ZONE) {
            work->mode = SONIC_CIRCLE;
            record->anim.action = ACTION_CIRCLE;
            break;
        }
        if (work->mode < SONIC_CIRCLE || work->mode > SONIC_CIRCLE_RIGHT) {
            record->held = NULL;
        }
        break;
    case SONIC_CIRCLE_RIGHT:
        if (sonic_try_request(work, motion, record) ||
            sonic_try_jump(work, motion, record)) {
            record->held = NULL;
            break;
        }
        if (pad_records[PLAYER_VARS(work)->player].x1 << 8 < PAD_DEAD_ZONE) {
            work->mode = SONIC_CIRCLE;
            record->anim.action = ACTION_CIRCLE;
            break;
        }
        if (work->mode < SONIC_CIRCLE || work->mode > SONIC_CIRCLE_RIGHT) {
            record->held = NULL;
        }
        break;
    case SONIC_DANGLE:
        if (sonic_try_request(work, motion, record)) {
            record->held = NULL;
            break;
        }
        if (player_jump_pressed(work, record)) {
            work->mode = SONIC_DANGLE_LET_GO;
            record->anim.action = ACTION_DANGLE_LET_GO;
            work->flags &= ~PLAYER_ON_TASK;
            break;
        }
        if (pad_records[PLAYER_VARS(work)->player].x1 << 8 < -PAD_DEAD_ZONE) {
            work->mode = SONIC_DANGLE_LEFT;
            record->anim.action = ACTION_DANGLE_LEFT;
            break;
        }
        if (pad_records[PLAYER_VARS(work)->player].x1 << 8 > PAD_DEAD_ZONE) {
            work->mode = SONIC_DANGLE_RIGHT;
            record->anim.action = ACTION_DANGLE_RIGHT;
            break;
        }
        if (record->own.sonic.steps == DANGLE_SLIP_1 ||
            record->own.sonic.steps == DANGLE_SLIP_2 ||
            record->own.sonic.steps == DANGLE_SLIP_3) {
            work->mode = SONIC_DANGLE_SLIP;
            record->anim.action = ACTION_DANGLE_SLIP;
        }
        break;
    case SONIC_DANGLE_SLIP:
        if (sonic_try_request(work, motion, record)) {
            record->held = NULL;
            break;
        }
        if (player_jump_pressed(work, record)) {
            work->mode = SONIC_DANGLE_LET_GO;
            record->anim.action = ACTION_DANGLE_LET_GO;
            work->flags &= ~PLAYER_ON_TASK;
            break;
        }
        /* @hack redundant: an else after an arm that breaks, which puts the
         * record in R2 and steps in R3 for their test (0x8c0b3284,
         * 0x8c0b3286); without it, the two trade and 20 of 7698 words
         * differ */
        /* @hack redundant: the last else, after an arm that breaks, which puts
         * work in R3 and SONIC_DANGLE in R1 for its stores (0x8c0b32b8,
         * 0x8c0b32ba); without it, R2 and R3, and 8 of 7698 words differ */
        if (record->anim.action == ACTION_DANGLE_SLIP) {
            break;
        } else if (record->own.sonic.steps >= DANGLE_SLIP_3) {
            work->mode = SONIC_FALL;
            record->anim.action = ACTION_FALL;
            record->own.sonic.steps = 0;
            record->held = NULL;
            record->jump_steps = 0;
            break;
        } else {
            work->mode = SONIC_DANGLE;
            record->anim.action = ACTION_DANGLE;
            break;
        }
    case SONIC_DANGLE_LEFT:
        if (sonic_try_request(work, motion, record)) {
            record->held = NULL;
            break;
        }
        if (player_jump_pressed(work, record)) {
            work->mode = SONIC_DANGLE_LET_GO;
            record->anim.action = ACTION_DANGLE_LET_GO;
            work->flags &= ~PLAYER_ON_TASK;
            break;
        }
        if (record->own.sonic.steps == DANGLE_SLIP_1 ||
            record->own.sonic.steps == DANGLE_SLIP_2 ||
            record->own.sonic.steps == DANGLE_SLIP_3) {
            work->mode = SONIC_DANGLE_SLIP;
            record->anim.action = ACTION_DANGLE_SLIP;
            break;
        }
        if (njAbs(pad_records[PLAYER_VARS(work)->player].x1 << 8) <
            PAD_DEAD_ZONE) {
            work->mode = SONIC_DANGLE;
            record->anim.action = ACTION_DANGLE;
            /* @hack redundant: a break where another follows, which joins this
             * arm to SONIC_DANGLE_RIGHT's last at its JMP, whose delay slot
             * stores the action (BRA at 0x8c0b33f8 to 0x8c0b34f4); without
             * it, this arm keeps its own and 44 of 7698 words differ */
            break;
        }
        break;
    case SONIC_DANGLE_RIGHT:
        if (sonic_try_request(work, motion, record)) {
            record->held = NULL;
            break;
        }
        if (player_jump_pressed(work, record)) {
            work->mode = SONIC_DANGLE_LET_GO;
            record->anim.action = ACTION_DANGLE_LET_GO;
            work->flags &= ~PLAYER_ON_TASK;
            break;
        }
        if (record->own.sonic.steps == DANGLE_SLIP_1 ||
            record->own.sonic.steps == DANGLE_SLIP_2 ||
            record->own.sonic.steps == DANGLE_SLIP_3) {
            work->mode = SONIC_DANGLE_SLIP;
            record->anim.action = ACTION_DANGLE_SLIP;
            break;
        }
        if (njAbs(pad_records[PLAYER_VARS(work)->player].x1 << 8) <
            PAD_DEAD_ZONE) {
            work->mode = SONIC_DANGLE;
            record->anim.action = ACTION_DANGLE;
            /* @hack redundant: a break where another follows, which keeps the
             * JMP that SONIC_DANGLE_LEFT's last arm joins, with the action's
             * store in its delay slot (0x8c0b34f6); without it, each arm
             * keeps its own and 49 of 7698 words differ */
            break;
        }
        break;
    case SONIC_DANGLE_LET_GO:
        if (sonic_try_request(work, motion, record)) {
            record->held = NULL;
            break;
        }
        /* LET_GO_FRAMES before his action ends, he lets go and turns
         * back */
        if (record->anim.frame >=
            (Float)record->anim.actions[record->anim.current]
                    .action->motion->nbFrame -
                LET_GO_FRAMES) {
            work->mode = SONIC_DANGLE_LEAP;
            work->ang.y += 0x8000;
            motion->aim.y = work->ang.y;
            record->speed.x = 2.0F;
            record->speed.y = 0.5F;
            record->speed.z = 0.0F;
            record->jump_steps = 0;
            sound_play(SE_LEAP_OFF, 0, 0, 0);
            record->held = NULL;
            work->submode = 0;
        }
        break;
    case SONIC_DANGLE_CLIMB:
        if (sonic_try_request(work, motion, record)) {
            record->held = NULL;
            break;
        }
        if (player_jump_pressed(work, record)) {
            work->mode = SONIC_DANGLE_LET_GO;
            record->anim.action = ACTION_DANGLE_LET_GO;
            work->flags &= ~PLAYER_ON_TASK;
            break;
        }
        if (record->held->work->pos.y <= work->pos.y + DANGLE_REACH) {
            work->mode = SONIC_DANGLE;
            record->anim.action = ACTION_DANGLE;
            break;
        }
        if (record->own.sonic.steps == DANGLE_SLIP_1 ||
            record->own.sonic.steps == DANGLE_SLIP_2 ||
            record->own.sonic.steps == DANGLE_SLIP_3) {
            work->mode = SONIC_DANGLE_SLIP;
            record->anim.action = ACTION_DANGLE_SLIP;
        }
        break;
    case SONIC_DANGLE_LEAP:
        if (sonic_try_request(work, motion, record)) {
            record->jump_steps = 0;
            break;
        }
        if (record->jump_steps++ > LEAP_STEPS) {
            record->jump_steps = 0;
            work->mode = SONIC_FALL;
            record->anim.action = ACTION_FALL;
            record->held = NULL;
            /* @hack redundant: a break where another follows, which leaves a
             * NOP in the JMP's delay slot (0x8c0b3776); without it, the store
             * to held fills the slot and 46 of 7698 words differ */
            break;
        }
        break;
    case SONIC_BOARD:
        if (sonic_try_request_board(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            work->mode = SONIC_BOARD_FALL;
            record->anim.action = ACTION_BOARD_FALL_START;
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                        pad_jump_buttons)) {
            struct task_force *force;
            NJS_VECTOR vector;
            NJS_VECTOR ahead;
            struct task_work *ramp;
            Angle ang_x;
            Angle ang_y;
            Angle ang_z;
            NJS_VECTOR up;

            work->mode = SONIC_BOARD_JUMP;
            record->anim.action = ACTION_BOARD_JUMP;
            record->speed.y = record->consts.jump_speed;
            if (record->speed.x < record->consts.sprint_speed) {
                record->speed.x += record->consts.trot_speed;
            }
            if (record->ground_task != NULL) {
                force = record->ground_task->force;
                if (force != NULL) {
                    player_set_accel(PLAYER_VARS(work)->player, force->speed.x,
                                     force->speed.y, force->speed.z);
                }
                /* off a ramp he leaps the way it faces, with a trick that
                 * scores by how fast he takes it */
                if (record->ground_task->work->id == RAMP_ID) {
                    work->mode = SONIC_BOARD_TRICK;
                    sound_play(SE_BOARD_RAMP, 0, 0, 0);
                    ramp = record->ground_task->work;
                    vector.x = 0.0F;
                    vector.y = 1.7F;
                    vector.z = 3.0F;
                    njPushMatrix(&_unit_matrix_);
                    TURN(njRotateZ, ramp->ang.z);
                    TURN(njRotateY, ramp->ang.y);
                    TURN(njRotateX, ramp->ang.x);
                    njRotateY(NULL, 0x8000);
                    njCalcVector(NULL, &vector, &ahead);
                    vector.x = vector.z = 0.0F;
                    vector.y = 1.0F;
                    njCalcVector(NULL, &vector, &up);
                    ang_y = RADIANS_TO_ANGLE(atan2f(ahead.x, ahead.z));
                    ang_x = RADIANS_TO_ANGLE(asinf(up.z));
                    ang_z = -RADIANS_TO_ANGLE(asinf(up.x));
                    player_set_angles(PLAYER_VARS(work)->player, ang_x, ang_y,
                                      ang_z);
                    njPopMatrix(1);
                    player_set_accel(PLAYER_VARS(work)->player, ahead.x,
                                     ahead.y, ahead.z);
                    if (record->speed.x > PLAYER_BOARD_SPEED * 0.9F) {
                        if (rand() * RAND_SCALE > 0.66F) {
                            record->anim.action = ACTION_TRICK_BIG;
                            sound_play(SE_TRICK_BIG, 0, 0, 0);
                        } else if (rand() * RAND_SCALE > 0.66F) {
                            record->anim.action = ACTION_TRICK;
                            sound_play(SE_TRICK, 0, 0, 0);
                        } else {
                            record->anim.action = ACTION_TRICK_BACK;
                            sound_play(SE_TRICK, 0, 0, 0);
                        }
                        if (rand() * RAND_SCALE >= 0.5F) {
                            camera_override(camera_step_trick_a, 0,
                                            CAMERA_LOOK_AT);
                            camera_state->move.x = 0.0F;
                        } else {
                            camera_override(camera_step_trick_b, 0,
                                            CAMERA_LOOK_AT);
                            camera_state->move.x = 0.0F;
                        }
                        sonic_board_score(100);
                    } else if (record->speed.x > PLAYER_BOARD_SPEED * 0.6F) {
                        record->anim.action = ACTION_TRICK_MEDIUM;
                        sonic_board_score(50);
                    } else if (record->speed.x > PLAYER_BOARD_SPEED * 0.4F) {
                        record->anim.action = ACTION_TRICK_SMALL;
                        sonic_board_score(30);
                    } else {
                        work->mode = SONIC_BOARD_JUMP;
                        record->anim.action = ACTION_BOARD_JUMP;
                        sonic_board_score(10);
                    }
                }
            }
            work->flags &= ~PLAYER_ON_TASK;
            record->jump_steps = 0;
            player_sound_in_mode(SE_BOARD_JUMP, PLAYER_VARS(work)->player,
                                 work->mode);
            break;
        }
        {
            Angle ang;

            /* @hack redundant: the second test of the speed against
             * trot_speed repeats the first and changes nothing. the image
             * makes both (FCMP/GT at 0x8c0b3c1e and 0x8c0b3c32); tested once,
             * 1281 of 7698 words differ */
            /* @hack redundant: an else after an arm that breaks, which puts
             * work in R2 and SONIC_BOARD_BRAKE in R3 for its stores
             * (0x8c0b3c3e, 0x8c0b3c40); without it, the two trade and 3 of 7698
             * words differ */
            /* NOLINTBEGIN(misc-redundant-expression) */
            if (!player_stick_back_or_end_event(work, motion) ||
                record->speed.x < record->consts.trot_speed ||
                record->speed.x < record->consts.trot_speed) {
                /* NOLINTEND(misc-redundant-expression) */
                break;
            } else {
                work->mode = SONIC_BOARD_BRAKE;
                player_read_stick(work, &ang, NULL);
                ang = angle_difference(work->ang.y, ang);
                if (ang > 0) {
                    record->anim.action = ACTION_BOARD_BRAKE_PLUS;
                } else {
                    record->anim.action = ACTION_BOARD_BRAKE_MINUS;
                }
            }
        }
        break;
    case SONIC_BOARD_BRAKE:
        if (sonic_try_request_board(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            work->mode = SONIC_BOARD_FALL;
            record->anim.action = ACTION_BOARD_FALL_START;
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].release,
                        pad_jump_buttons)) {
            struct task_force *force;

            work->mode = SONIC_BOARD_JUMP;
            record->speed.y = record->consts.jump_speed;
            if (record->speed.x < record->consts.sprint_speed) {
                record->speed.x += record->consts.trot_speed;
            }
            if (record->ground_task != NULL) {
                force = record->ground_task->force;
                if (force != NULL) {
                    player_set_accel(PLAYER_VARS(work)->player, force->speed.x,
                                     force->speed.y, force->speed.z);
                }
            }
            record->anim.action = ACTION_BOARD_JUMP;
            work->flags &= ~PLAYER_ON_TASK;
            record->jump_steps = 0;
            sound_play(SE_JUMP, 0, 0, 0);
            break;
        }
        {
            Angle ang;

            if (!player_stick_back_or_end_event(work, motion) ||
                record->speed.x < record->consts.trot_speed) {
                work->mode = SONIC_BOARD;
                record->anim.action = ACTION_BOARD;
                sonic_board_sound(work, motion, record);
                break;
            }
            player_read_stick(work, &ang, NULL);
            ang = angle_difference(work->ang.y, ang);
            if (ang > 0) {
                record->anim.action = ACTION_BOARD_BRAKE_PLUS;
            } else {
                record->anim.action = ACTION_BOARD_BRAKE_MINUS;
            }
        }
        break;
    case SONIC_BOARD_FALL:
        if (sonic_try_request_board(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (PLAYER_VARS(work)->player == 0) {
                sound_play(SE_LAND, 0, 0, 0);
            }
            work->mode = SONIC_BOARD;
            if (record->speed.y <= BOARD_HARD_LANDING_SPEED) {
                record->anim.action = ACTION_BOARD_HARD_LANDING;
            } else {
                record->anim.action = ACTION_BOARD;
                if (record->speed.x <= 0.3F) {
                    record->speed.x = 1.0F;
                }
            }
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            sonic_board_sound(work, motion, record);
        }
        break;
    case SONIC_BOARD_JUMP:
        if (sonic_try_request_board(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            work->mode = SONIC_BOARD;
            if (record->speed.y <= BOARD_HARD_LANDING_SPEED) {
                record->anim.action = ACTION_BOARD_HARD_LANDING;
            } else {
                record->anim.action = ACTION_BOARD;
                if (record->speed.x <= 0.3F) {
                    record->speed.x = 1.0F;
                }
            }
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            sonic_board_sound(work, motion, record);
            /* @hack redundant: a break where another follows, which joins this
             * landing to SONIC_BOARD_TRICK's at its sonic_board_sound (BRA at
             * 0x8c0b3f76 to 0x8c0b4000); without it, this landing makes its
             * own call and 1059 of 7698 words differ */
            break;
        }
        break;
    case SONIC_BOARD_TRICK:
        if (sonic_try_request_board(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            work->mode = SONIC_BOARD;
            if (record->speed.y <= BOARD_HARD_LANDING_SPEED) {
                record->anim.action = ACTION_BOARD_HARD_LANDING;
            } else {
                record->anim.action = ACTION_BOARD;
            }
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            sonic_board_sound(work, motion, record);
            /* @hack redundant: a break where another follows, which keeps here
             * the sonic_board_sound that SONIC_BOARD_JUMP's landing joins
             * (0x8c0b4000); without it, each landing makes its own call and
             * 1058 of 7698 words differ */
            break;
        }
        break;
    case SONIC_BOARD_CRASH:
        if (sonic_try_request_board(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_BOARD_CRASH_PLUS &&
            record->anim.action != ACTION_BOARD_CRASH_MINUS) {
            work->mode = SONIC_BOARD;
            record->anim.action = ACTION_BOARD;
            sonic_board_sound(work, motion, record);
        }
        break;
    case SONIC_SLOPE_ROLL:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            work->mode = SONIC_SLOPE_ROLL_FALL;
        }
        break;
    case SONIC_SLOPE_ROLL_FALL:
        if (sonic_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (PLAYER_VARS(work)->player == 0) {
                sound_play(SE_LAND, 0, 0, 0);
            }
            work->mode = SONIC_SLOPE_ROLL;
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
        }
        break;
    case SONIC_SUPER_STAND:
        if (sonic_try_request_super(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            work->mode = SONIC_SUPER_FALL;
            record->anim.action = ACTION_SUPER_FALL;
            break;
        }
        if (sonic_try_jump(work, motion, record)) {
            work->mode = SONIC_SUPER_JUMP;
            record->anim.action = ACTION_SUPER_BALL;
            break;
        }
        if (player_read_stick(work, NULL, NULL)) {
            work->mode = SONIC_SUPER_WALK;
            sonic_super_set_walk_action(work, motion, record);
        }
        break;
    case SONIC_SUPER_WALK:
        if (sonic_try_request_super(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            work->mode = SONIC_SUPER_FALL;
            record->anim.action = ACTION_SUPER_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (sonic_try_jump(work, motion, record)) {
            work->mode = SONIC_SUPER_JUMP;
            record->anim.action = ACTION_SUPER_BALL;
            break;
        }
        if (sonic_try_stop(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            work->mode = SONIC_SUPER_STAND;
            record->anim.action = ACTION_SUPER_STAND;
        }
        break;
    case SONIC_SUPER_PATH:
        work->flags &= ~PLAYER_ATTACKING;
        if (sonic_try_request_super(work, motion, record)) {
            break;
        }
        if (sonic_try_fall(work, motion, record)) {
            work->mode = SONIC_SUPER_FALL;
            record->anim.action = ACTION_SUPER_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (record->speed.x < record->consts.trot_speed) {
            work->mode = SONIC_SUPER_WALK;
            record->anim.action = ACTION_SUPER_WALK;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (sonic_try_stop(work, motion, record)) {
            work->mode = SONIC_SUPER_STAND;
            record->anim.action = ACTION_SUPER_STAND;
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case SONIC_SUPER_JUMP:
        if (sonic_try_request_super(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (sonic_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = SONIC_SUPER_STAND;
                record->anim.action = ACTION_SUPER_STAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = SONIC_SUPER_WALK;
                sonic_super_set_walk_action(work, motion, record);
            }
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (sonic_try_uncurl(work, motion, record)) {
            work->mode = SONIC_SUPER_FALL;
            record->anim.action = ACTION_SUPER_FALL;
            break;
        }
        if (player_jump_pressed(work, record)) {
            work->mode = SONIC_SUPER_HOMING;
            sonic_homing_start(work, motion, record);
            break;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.8F;
        }
        break;
    case SONIC_SUPER_SPRING:
        if (sonic_try_request_super(work, motion, record)) {
            break;
        }
        if (motion->speed.y <= 0.0F) {
            work->mode = SONIC_SUPER_FALL;
            record->anim.action = ACTION_SUPER_FALL_START;
        }
        break;
    case SONIC_SUPER_LAUNCH:
        if (sonic_try_request_super(work, motion, record)) {
            break;
        }
        if (motion->speed.y <= 0.0F) {
            work->mode = SONIC_SUPER_FALL;
            record->anim.action = ACTION_SUPER_FALL_START;
        }
        break;
    case SONIC_SUPER_FALL:
        if (sonic_try_request_super(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (sonic_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = SONIC_SUPER_STAND;
                record->anim.action = ACTION_SUPER_STAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = SONIC_SUPER_WALK;
                sonic_super_set_walk_action(work, motion, record);
            }
        }
        break;
    case SONIC_SUPER_HOMING: {
        struct collision_work *hit;

        if (sonic_try_request_super(work, motion, record)) {
            break;
        }
        hit = work->collision->hit;
        if (record->own.sonic.steps++ >= HOMING_STEPS ||
            (hit != NULL &&
             (hit->kind == COLLISION_PLAYER || hit->kind == COLLISION_ITEM ||
              hit->kind == COLLISION_ENEMY))) {
            record->own.sonic.dash_steps = -1;
            work->mode = SONIC_SUPER_JUMP;
            break;
        }
        if (record->own.sonic.dash_steps >= HOMING_UNAIMED_STEPS) {
            record->own.sonic.dash_steps = 0;
            work->mode = SONIC_SUPER_FALL;
            record->anim.action = ACTION_SUPER_FALL;
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            work->flags &= ~PLAYER_ATTACKING;
            if (sonic_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = SONIC_SUPER_STAND;
                record->anim.action = ACTION_SUPER_STAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = SONIC_SUPER_WALK;
                sonic_super_set_walk_action(work, motion, record);
            }
        }
        break;
    }
    case SONIC_EVENT:
        if (work->event->paths == NULL) {
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            sonic_stand(work, motion, record);
        }
        break;
    case SONIC_EDIT:
        /* a press of the pad's A puts him in a mode for the stage and act
         * he's in. B held picks the wall run in Speed Highway's act 1 */
        if (pad_records[PLAYER_VARS(work)->player].press & PDD_DGT_TA) {
            if ((pad_records[PLAYER_VARS(work)->player].on & PDD_DGT_TB) &&
                stage_get_stage_act() == STAGE_ACT(STAGE_HIGHWAY, 1)) {
                work->mode = SONIC_WALL_RUN;
                record->anim.action = ACTION_WALK;
            } else if (!(pad_records[PLAYER_VARS(work)->player].on &
                         PDD_DGT_TB) &&
                       stage_get_stage_act() == STAGE_ACT(STAGE_ICECAP, 2)) {
                work->mode = SONIC_BOARD_FALL;
                record->anim.action = ACTION_BOARD_AIR;
            } else if (!(pad_records[PLAYER_VARS(work)->player].on &
                         PDD_DGT_TB) &&
                       stage_get_stage_act() == STAGE_ACT(STAGE_SUNABOARD, 0)) {
                work->mode = SONIC_BOARD_FALL;
                record->anim.action = ACTION_BOARD_AIR;
            } else if ((record->upgrades & PLAYER_SONIC_SUPER) &&
                       IN_STAGE(STAGE_CHAOS7)) {
                work->mode = SONIC_SUPER_STAND;
                record->anim.action = ACTION_SUPER_STAND;
            } else {
                work->mode = SONIC_WALK;
                record->anim.action = ACTION_WALK;
            }
        }
        break;
    }
}

/* his record and his textures go with his task. his toes take their own
 * models back */
static void sonic_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;

    record->anim.action = ACTION_STAND;
    sonic_blend_toes(work, motion, record);
    player_free(PLAYER_VARS(work)->player);
    sound_stop_id((int)work);
    texture_release_character(CHARACTER_SONIC);
}

/* his shape's center: center_y above his feet, turned with him */
static void sonic_move_shape(struct task_work *work, struct task_motion *motion,
                             struct player *record)
{
    struct collision_shape *shape = work->collision->shapes;
    NJS_VECTOR center;

    center.x = center.z = 0.0F;
    center.y = record->consts.center_y;
    player_turn_to_world(work, &center);
    shape->center = center;
    shape->center.x += work->pos.x;
    shape->center.y += work->pos.y;
    shape->center.z += work->pos.z;
}

/* draw him while he's in view, but not while his timer has BLINK_HIDDEN
 * set. his super actions take super Sonic's textures. a spin on the ground
 * squashes him. curled up, he shows ACTION_BALL in place of his spin on the
 * steps BALL_STEP_MASK picks. at full speed and in a spin on the ground,
 * copies trail him, each with a third less alpha than the one before */
static void sonic_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;
    Uint32 action;
    Float frame;
    Float frames;
    int i;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 15.0F)) {
        return;
    }
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_IGNORE_SPECULAR);
    draw_set_palette(DRAW_PLAYER_PALETTE);
    action = record->anim.action;
    if (record->anim.state == PLAYER_ANIM_BLENDING) {
        action = record->anim.current;
    }
    if (!(TASK_TIMER(work) & BLINK_HIDDEN)) {
        if (action >= ACTION_SUPER_STAND && action <= ACTION_SUPER_BALL) {
            njSetTexture(&super_sonic_texlist);
        } else {
            njSetTexture(&sonic_texlist);
        }
        sonic_blend_toes(work, motion, record);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->collision->shapes->center);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, 0x8000 - work->ang.y);
        if (action == ACTION_SPIN && (work->flags & PLAYER_ON_GROUND)) {
            NJS_VECTOR squash;

            squash.x = 0.0F;
            squash.y = -1.0F;
            squash.z = 0.0F;
            njTranslateV(NULL, &squash);
            njRotateZ(NULL, 0x2000);
            squash.x = 0.7F;
            squash.y = 1.1F;
            squash.z = 0.8F;
            njScaleV(NULL, &squash);
        }
        if (work->event->motions != NULL) {
            event_motions_draw(work);
        } else {
            if (record->anim.state != PLAYER_ANIM_BLENDING) {
                if ((work->flags & PLAYER_BALL) &&
                    action != ACTION_SUPER_BALL &&
                    (record->own.sonic.curled_steps & BALL_STEP_MASK)) {
                    action = ACTION_BALL;
                }
                draw_action(record->anim.actions[action].action,
                            record->anim.frame);
                if (action == ACTION_FULL_SPEED ||
                    (action == ACTION_SPIN &&
                     (work->flags & PLAYER_ON_GROUND))) {
                    NJS_ARGB color;

                    frame = record->anim.frame;
                    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
                    frames = (Float)record->anim.actions[action]
                                 .action->motion->nbFrame;
                    color.r = color.g = color.b = color.a = 0.0F;
                    i = 0;
                    while (i < TRAIL_COPIES) {
                        frame -= TRAIL_GAP;
                        if (frame < 0.0F) {
                            frame += frames;
                        }
                        color.a = -((Float)(i + 1) * (1.0F / 3.0F));
                        njSetConstantMaterial(&color);
                        draw_action(record->anim.actions[action].action, frame);
                        i++;
                    }
                    color.a = 0.0F;
                    njSetConstantMaterial(&color);
                }
            } else {
                draw_action(record->anim.blend, record->anim.frame);
            }
            if (work->flags & SONIC_CHARGED) {
                aura_draw(work, motion, record);
            }
        }
        njPopMatrix(1);
    }
    draw_set_palette(0);
    draw_attr_restore();
}

/* choose his standing action. he fidgets after FIDGET_STEPS idle. on a
 * slope from FLAT_SLOPE to LEAN_SLOPE, but for ground with WORLD_NO_SLOPE,
 * he leans uphill or downhill. ACTION_UNCURL plays on */
static void sonic_set_stand_action(struct task_work *work,
                                   struct task_motion *motion,
                                   struct player *record)
{
    Float cosine;
    NJS_PLANE plane;
    NJS_VECTOR basis;
    NJS_VECTOR second;
    NJS_VECTOR first;
    int angle;
    int action;

    if (record->anim.action == ACTION_UNCURL) {
        return;
    }
    cosine = -njInnerProduct(&world_gravity, &record->ground_normal);
    if (cosine > njCos(FLAT_SLOPE)) {
        action = record->anim.action;
        if (action != ACTION_STAND && action != ACTION_LAND &&
            action != ACTION_LANDED && action != ACTION_READY &&
            action != ACTION_FIDGET_START && action != ACTION_FIDGET &&
            action != ACTION_FIDGET_END) {
            record->anim.action = ACTION_STAND;
        } else if (record->idle_steps > FIDGET_STEPS) {
            record->idle_steps = 0;
            record->anim.action = ACTION_FIDGET_START;
        }
    } else if (!(record->ground_flags & WORLD_NO_SLOPE) &&
               cosine > njCos(LEAN_SLOPE)) {
        plane.vx = record->ground_normal.x;
        plane.vy = record->ground_normal.y;
        plane.vz = record->ground_normal.z;
        plane.px = plane.py = plane.pz = 0.0F;
        basis.x = 1.0F;
        basis.y = basis.z = 0.0F;
        player_turn_to_world(work, &basis);
        njDistanceP2PL(&basis, &plane, &first);
        njUnitVector(&first);
        njDistanceP2PL(&world_gravity, &plane, &second);
        njUnitVector(&second);
        angle = vector_angle(&first, &second, &record->ground_normal);
        if (angle > 0xc000 || angle < 0x4000) {
            record->anim.action = ACTION_STAND_DOWNHILL;
        } else {
            record->anim.action = ACTION_STAND_UPHILL;
        }
    }
    record->idle_steps += 1;
}

/* choose his walking action, carrying or not, by the speed he's about to
 * have, and set how fast it plays. ACTION_UNCURL, ACTION_TURN and
 * ACTION_HOLD_TURN play on */
static void sonic_set_walk_action(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record)
{
    Float speed;
    int action;

    speed = record->speed.x + (motion->slip * 5.0F);
    action = record->anim.action;
    if (action == ACTION_UNCURL || action == ACTION_TURN ||
        action == ACTION_HOLD_TURN) {
        return;
    }
    if (work->flags & PLAYER_HOLDING) {
        if (speed > record->consts.running_speed) {
            record->anim.action = ACTION_HOLD_RUN;
        } else {
            record->anim.action = ACTION_HOLD_WALK;
        }
    } else if (record->consts.trot_speed < record->speed.x &&
               !player_read_stick(work, NULL, NULL) &&
               (record->ground_flags & WORLD_GRIP_LEAST)) {
        record->anim.action = ACTION_GLIDE;
    } else if (speed > record->consts.full_speed) {
        record->anim.action = ACTION_FULL_SPEED;
    } else if (speed > record->consts.sprint_speed) {
        record->anim.action = ACTION_SPRINT;
    } else if (speed > record->consts.running_speed) {
        record->anim.action = ACTION_RUN;
    } else if (speed > record->consts.trot_speed) {
        record->anim.action = ACTION_TROT;
    } else {
        record->anim.action = ACTION_WALK;
    }
    record->action_speed = record->speed.x + (motion->slip * 20.0F);
}

/* choose super Sonic's running action by the speed he's about to have,
 * and set how fast it plays */
static void sonic_super_set_walk_action(struct task_work *work,
                                        struct task_motion *motion,
                                        struct player *record)
{
    Float speed;

    speed = record->speed.x + (motion->slip * 5.0F);
    if (speed > record->consts.sprint_speed) {
        record->anim.action = ACTION_SUPER_SPRINT;
    } else if (speed > record->consts.running_speed) {
        record->anim.action = ACTION_SUPER_RUN;
    } else if (speed > record->consts.trot_speed) {
        record->anim.action = ACTION_SUPER_TROT;
    } else {
        record->anim.action = ACTION_SUPER_WALK;
    }
    record->action_speed = record->speed.x + (motion->slip * 20.0F);
}

/* hang him his height below the path. as he moves, ACTION_HANG_GO steps
 * HANG_FRAME_STEP frames the way he moves. he stops at frame 0 or at
 * HANG_HALFWAY when player_read_stick finds the stick let go there. with
 * motion->drive 0, he stops at once: in ACTION_HANG below frame
 * HANG_HALFWAY / 2, and in ACTION_HANG_HALFWAY from there on */
static void sonic_hang_move(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    NJS_VECTOR up;

    up.x = 0.0F;
    up.y = record->consts.height;
    up.z = 0.0F;
    player_turn_to_world(work, &up);
    njSubVector(&work->pos, &up);
    if (record->speed.x != 0.0F && motion->drive == 0.0F) {
        if (record->anim.frame == 0.0F || record->anim.frame >= HANG_HALFWAY) {
            work->mode = SONIC_HANG;
            if (record->anim.frame == 0.0F) {
                record->anim.action = ACTION_HANG;
            } else {
                record->anim.action = ACTION_HANG_HALFWAY;
            }
            record->speed.x = 0.0F;
        } else {
            work->mode = SONIC_HANG;
            record->speed.x = 0.0F;
            if (record->anim.frame >= HANG_HALFWAY / 2.0F &&
                record->anim.frame <= HANG_HALFWAY * 1.5F) {
                record->anim.action = ACTION_HANG_HALFWAY;
            } else {
                record->anim.action = ACTION_HANG;
            }
        }
    } else if (record->speed.x > 0.0F) {
        if (record->anim.frame >= 0.0F && record->anim.frame < HANG_HALFWAY) {
            record->anim.frame += HANG_FRAME_STEP;
            if (record->anim.frame == HANG_HALFWAY &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = SONIC_HANG;
                record->anim.action = ACTION_HANG_HALFWAY;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame += HANG_FRAME_STEP;
            if (record->anim.frame >= HANG_FRAMES) {
                record->anim.frame = 0.0F;
            }
            if (record->anim.frame == 0.0F &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = SONIC_HANG;
                record->anim.action = ACTION_HANG;
                record->speed.x = 0.0F;
            }
        }
    } else if (record->speed.x < 0.0F) {
        if (record->anim.frame > 0.0F && record->anim.frame <= HANG_HALFWAY) {
            record->anim.frame -= HANG_FRAME_STEP;
            if (record->anim.frame == 0.0F &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = SONIC_HANG;
                record->anim.action = ACTION_HANG;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame -= HANG_FRAME_STEP;
            if (record->anim.frame < 0.0F) {
                record->anim.frame = HANG_FRAMES - 1.0F;
            }
            if (record->anim.frame == HANG_HALFWAY &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = SONIC_HANG;
                record->anim.action = ACTION_HANG_HALFWAY;
                record->speed.x = 0.0F;
            }
        }
    } else {
        record->speed.x = 0.0F;
    }
}

/* hang him his height below the ledge. as he moves, ACTION_LEDGE steps
 * HANG_FRAME_STEP frames the way he moves. he stops at LEDGE_HALFWAY or at
 * LEDGE_LAST when player_read_stick finds the stick let go there. with
 * motion->drive 0, he stops only at those frames, and elsewhere this leaves
 * his frame and mode as they are. his action stays ACTION_LEDGE */
static void sonic_ledge_move(struct task_work *work, struct task_motion *motion,
                             struct player *record)
{
    NJS_VECTOR up;

    up.x = 0.0F;
    up.y = record->consts.height;
    up.z = 0.0F;
    player_turn_to_world(work, &up);
    njSubVector(&work->pos, &up);
    if (record->speed.x != 0.0F && motion->drive == 0.0F) {
        if (record->anim.frame == LEDGE_HALFWAY ||
            record->anim.frame == LEDGE_LAST) {
            work->mode = SONIC_LEDGE;
            record->speed.x = 0.0F;
        }
    } else if (record->speed.x > 0.0F) {
        if (record->anim.frame >= 0.0F && record->anim.frame < LEDGE_HALFWAY) {
            record->anim.frame += HANG_FRAME_STEP;
            if (record->anim.frame == LEDGE_HALFWAY &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = SONIC_LEDGE;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame += HANG_FRAME_STEP;
            if (record->anim.frame >= LEDGE_FRAMES) {
                record->anim.frame = 0.0F;
            }
            if (record->anim.frame == LEDGE_LAST &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = SONIC_LEDGE;
                record->speed.x = 0.0F;
            }
        }
    } else if (record->speed.x < 0.0F) {
        if (record->anim.frame > LEDGE_HALFWAY &&
            record->anim.frame <= LEDGE_FRAMES) {
            record->anim.frame -= HANG_FRAME_STEP;
            if (record->anim.frame == LEDGE_HALFWAY &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = SONIC_LEDGE;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame -= HANG_FRAME_STEP;
            if (record->anim.frame < 0.0F) {
                record->anim.frame = LEDGE_LAST;
            }
            if (record->anim.frame == LEDGE_LAST &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = SONIC_LEDGE;
                record->speed.x = 0.0F;
            }
        }
    } else {
        record->speed.x = 0.0F;
    }
}

/* choose his action on the board, and its frame. he leans the way the
 * stick turns him, and his action's frame moves toward the lean the turn
 * asks for. with the stick let go he stands up again, crouched while the
 * jump or action button is held. slowing down, he straightens */
static void sonic_set_board_action(struct task_work *work,
                                   struct task_motion *motion,
                                   struct player *record)
{
    Float lean;

    if (record->anim.action != ACTION_BOARD &&
        record->anim.action != ACTION_BOARD_CROUCH &&
        record->anim.action != ACTION_BOARD_LEAN_MINUS &&
        record->anim.action != ACTION_BOARD_LEAN_PLUS &&
        record->anim.action != ACTION_BOARD_SLOW &&
        record->anim.action != ACTION_BOARD_STRAIGHTEN) {
        return;
    }

    if (player_read_stick(work, NULL, NULL)) {
        if (motion->drive < 0.0F) {
            if (record->anim.action == ACTION_BOARD ||
                record->anim.action == ACTION_BOARD_CROUCH ||
                record->anim.action == ACTION_BOARD_SLOW ||
                record->anim.action == ACTION_BOARD_STRAIGHTEN) {
                record->anim.action = ACTION_BOARD_LEAN_MINUS;
                record->anim.frame = 0.0F;
                lean = -(motion->drive * 12.0F) + 4.0F;
            } else if (record->anim.action == ACTION_BOARD_LEAN_PLUS) {
                if (record->anim.frame == 0.0F) {
                    record->anim.action = ACTION_BOARD_LEAN_MINUS;
                    lean = -(motion->drive * 12.0F) + 4.0F;
                } else {
                    lean = 0.0F;
                }
            } else {
                lean = -(motion->drive * 12.0F) + 4.0F;
            }
        } else if (motion->drive > 0.0F) {
            if (record->anim.action == ACTION_BOARD ||
                record->anim.action == ACTION_BOARD_CROUCH ||
                record->anim.action == ACTION_BOARD_SLOW ||
                record->anim.action == ACTION_BOARD_STRAIGHTEN) {
                record->anim.action = ACTION_BOARD_LEAN_PLUS;
                record->anim.frame = 0.0F;
                lean = (motion->drive * 12.0F) + 4.0F;
            } else if (record->anim.action == ACTION_BOARD_LEAN_MINUS) {
                if (record->anim.frame == 0.0F) {
                    record->anim.action = ACTION_BOARD_LEAN_PLUS;
                    lean = (motion->drive * 12.0F) + 4.0F;
                } else {
                    lean = 0.0F;
                }
            } else {
                lean = (motion->drive * 12.0F) + 4.0F;
            }
        } else {
            if (record->anim.action == ACTION_BOARD ||
                record->anim.action == ACTION_BOARD_CROUCH ||
                record->anim.action == ACTION_BOARD_SLOW ||
                record->anim.action == ACTION_BOARD_STRAIGHTEN) {
                if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                                pad_jump_buttons | pad_action_buttons)) {
                    record->anim.action = ACTION_BOARD_CROUCH;
                }
            } else {
                record->anim.frame -= 1.0F;
                if (record->anim.frame <= 0.0F) {
                    record->anim.frame = 0.0F;
                    if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                                    pad_jump_buttons | pad_action_buttons)) {
                        record->anim.action = ACTION_BOARD_CROUCH;
                    } else {
                        record->anim.action = ACTION_BOARD;
                    }
                }
            }
            lean = record->anim.frame;
        }

        if (record->anim.frame > lean) {
            if (lean == 0.0F) {
                record->anim.frame -= 2.0F;
                if (record->anim.frame < 0.0F) {
                    record->anim.frame = 0.0F;
                }
            } else if (record->anim.frame - lean > 1.0F) {
                if (_builtin_fabsf(motion->drive) > 0.5F) {
                    record->anim.frame -= _builtin_fabsf(motion->drive);
                } else {
                    record->anim.frame -= 0.5F;
                }
            } else {
                record->anim.frame = lean;
            }
        } else if (lean > record->anim.frame) {
            if (lean - record->anim.frame > 1.0F) {
                if (_builtin_fabsf(motion->drive) > 0.5F) {
                    record->anim.frame += _builtin_fabsf(motion->drive);
                } else {
                    record->anim.frame += 0.5F;
                }
            } else {
                record->anim.frame = lean;
            }
        }
    } else if (record->anim.action == ACTION_BOARD_LEAN_MINUS ||
               record->anim.action == ACTION_BOARD_LEAN_PLUS) {
        record->anim.frame -= 1.0F;
        if (record->anim.frame <= 0.0F) {
            if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                            pad_jump_buttons | pad_action_buttons)) {
                record->anim.action = ACTION_BOARD_CROUCH;
            } else {
                record->anim.action = ACTION_BOARD;
            }
            record->anim.frame = 0.0F;
        }
    } else {
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                        pad_jump_buttons | pad_action_buttons)) {
            record->anim.action = ACTION_BOARD_CROUCH;
        } else {
            if (record->anim.action == ACTION_BOARD &&
                record->speed.x < PLAYER_BOARD_SPEED * 0.3F) {
                record->anim.action = ACTION_BOARD_SLOW;
            } else if (record->anim.action == ACTION_BOARD_SLOW &&
                       record->speed.x < PLAYER_BOARD_SPEED * 0.3F) {
                record->anim.action = ACTION_BOARD_STRAIGHTEN;
            }
        }
    }
}

/* play the board's sound while he stays in his mode, if he's as fast as a
 * trot. in Ice Cap it's the sound of the ground's grip, and elsewhere of
 * sand */
static void sonic_board_sound(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    int se;

    if (record->speed.x < record->consts.trot_speed) {
        return;
    }

    if (IN_STAGE(STAGE_ICECAP)) {
        if (record->ground_flags == WORLD_GRIP_LEAST) {
            se = SE_BOARD_ICECAP_GRIP_LEAST;
        } else if (record->ground_flags == WORLD_GRIP_LOW) {
            se = SE_BOARD_ICECAP_GRIP_LOW;
        } else {
            se = SE_BOARD_ICECAP;
        }
    } else {
        se = SE_BOARD_SAND;
    }
    player_sound_in_mode(se, PLAYER_VARS(work)->player, work->mode);
}

/* fly him about in the editor. the stick moves him up and down while X is
 * held, and across, as the camera faces, while it isn't. B puts him back at
 * his start */
static void sonic_edit(struct task_work *work, struct task_motion *motion,
                       struct player *record)
{
    Float x;
    Float y;
    Float sign;
    Angle ang;

    if (pad_records[PLAYER_VARS(work)->player].press & PDD_DGT_TB) {
        stage_place_player(work);
    }
    /* @hack spelling: the stick read in each arm of the X test, which loads
     * the dead-zone constants ahead of the test (MOVA at 0x8c0b5b06); read
     * once above it, they load after the test, the code runs shorter and 132
     * of 151 words differ */
    if (pad_records[PLAYER_VARS(work)->player].on & PDD_DGT_TX) {
        x = (Float)(pad_records[PLAYER_VARS(work)->player].x1 << 8);
        y = (Float)(pad_records[PLAYER_VARS(work)->player].y1 << 8);
        if (x > PAD_DEAD_ZONE || x < -PAD_DEAD_ZONE || y > PAD_DEAD_ZONE ||
            y < -PAD_DEAD_ZONE) {
            sign = y != 0.0F ? y / njSqrt(y * y) : 0.0F;
            work->pos.y -= sign * PLAYER_EDIT_SPEED;
        }
    } else {
        x = (Float)(pad_records[PLAYER_VARS(work)->player].x1 << 8);
        y = (Float)(pad_records[PLAYER_VARS(work)->player].y1 << 8);
        /* @hack spelling: an empty arm for a stick in the dead zone, with the
         * move in the else, whose jump puts atan2f's address in R2
         * (0x8c0b5bd4); as one test of a stick out of it, R3, and 2 of 151
         * words differ */
        if (x <= PAD_DEAD_ZONE && x >= -PAD_DEAD_ZONE && y <= PAD_DEAD_ZONE &&
            y >= -PAD_DEAD_ZONE) {
        } else {
            ang = (Angle)(atan2f(y, x) * ANGLE_PER_RADIAN) - camera_work->ang.y;
            work->pos.x += njCos(ang) * PLAYER_EDIT_SPEED;
            work->pos.z += njSin(ang) * PLAYER_EDIT_SPEED;
        }
    }
}

/* start a homing attack: its ball, its sound, and a turn to its target.
 * with no task near him and no rival, he flies on ahead at HOMING_SPEED, or
 * SUPER_HOMING_SPEED when he's super. when sonic_homing finds no target, or
 * one above him, he flies on at HOMING_SPEED in either form */
static void sonic_homing_start(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    struct near_task *near;

    if (record->upgrades & PLAYER_SONIC_SUPER) {
        sonic_super_ball_shrink_create(work);
    } else {
        sonic_ball_shrink_create(work);
    }
    record->own.sonic.dash_steps = -1;
    record->own.sonic.steps = 0;
    sound_play(SE_HOMING, 0, 0, 0);
    if (PLAYER_VARS(work)->player == 0) {
        near = near_tasks;
    } else {
        near = near_tasks_player1;
    }
    if (near->work == NULL &&
        player_find_rival(PLAYER_VARS(work)->player) == -1) {
        /* @hack spelling: the speed stored in each arm of an if, which lets SHC
         * form the offset of dash_steps by adding 127 to the 1 it stores
         * (ADD #127,R0 at 0x8c0b5cd8); as one store of a ?:, SHC forms it
         * from the 56 in R0 and 6 of 82 words differ */
        if (record->upgrades & PLAYER_SONIC_SUPER) {
            record->speed.x = SUPER_HOMING_SPEED;
        } else {
            record->speed.x = HOMING_SPEED;
        }
        record->own.sonic.dash_steps = 1;
    } else {
        sonic_homing(work, motion, record);
        if (record->own.sonic.dash_steps) {
            record->speed.x = HOMING_SPEED;
        }
    }
}

/* fly him on a homing attack at the nearest target ahead of him, or at the
 * rival when it's nearer. he turns toward it and flies at it. when he's
 * super, a target counts in any direction. with no target and no rival,
 * he slows */
static void sonic_homing(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    struct near_task *near;
    struct task_work *best;
    struct collision_shape *shape;
    NJS_VECTOR to;
    Float best_distance;
    Float speed;
    Angle heading;
    Angle step;
    int rival;

    if (PLAYER_VARS(work)->player == 0) {
        near = near_tasks;
    } else {
        near = near_tasks_player1;
    }
    if ((record->own.sonic.dash_steps > 0 || near->work == NULL) &&
        player_find_rival(PLAYER_VARS(work)->player) == -1) {
        record->own.sonic.dash_steps++;
        record->speed.x *= 0.9F;
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
    } else {
        if ((rival = player_find_rival(PLAYER_VARS(work)->player)) != -1) {
            best = player_work[rival];
            to.x = work->pos.x - best->pos.x;
            to.y = work->pos.y - best->pos.y;
            to.z = work->pos.z - best->pos.z;
            best_distance = njScalor2(&to);
            if (angle_distance(work->ang.y, (Angle)(atan2f(-to.z, -to.x) *
                                                    ANGLE_PER_RADIAN)) >
                    HOMING_CONE ||
                !(best_distance < TARGET_RANGE * TARGET_RANGE)) {
                best = NULL;
                best_distance = TARGET_RANGE * TARGET_RANGE;
            }
        } else {
            best_distance = TARGET_RANGE * TARGET_RANGE;
            best = NULL;
        }
        for (; near->work != NULL; near++) {
            /* @hack spelling: shape set inside the copy of its center, which
             * keeps 32 in R8 (MOV #32,R8 at 0x8c0b5e8e); set in a statement
             * of its own, atan2f's address takes R8 and 147 of 373 words
             * differ */
            to = (shape = near->work->collision->shapes)->center;
            if (!(shape->flags & SHAPE_IN_WORLD)) {
                njAddVector(&to, &near->work->pos);
            }
            njSubVector(&to, &work->pos);
            heading = (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN);
            if ((record->upgrades & PLAYER_SONIC_SUPER) ||
                angle_distance(work->ang.y, heading) <= HOMING_CONE) {
                if (near->distance < best_distance) {
                    best_distance = near->distance;
                    best = near->work;
                }
            }
        }
        if (best == NULL) {
            record->own.sonic.dash_steps = 1;
            player_accel_ground(work, motion, record);
            player_apply_accel(work, motion, record);
            return;
        }
        to = (shape = best->collision->shapes)->center;
        if (!(shape->flags & SHAPE_IN_WORLD)) {
            njAddVector(&to, &best->pos);
        }
        njSubVector(&to, &work->pos);
        heading = (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN) & 0xffff;
        if (record->upgrades & PLAYER_SONIC_SUPER) {
            step = SUPER_HOMING_TURN;
            /* @hack redundant: a goto to the next statement, which makes no
             * code but puts angle_approach's address in R2 (0x8c0b5f96);
             * without it, R3, and 2 of 373 words differ */
            goto turn;
        } else {
            step = HOMING_TURN;
        }
    turn:
        work->ang.y = angle_approach(work->ang.y, heading, step);
        /* a target above him on the attack's first step still turns him,
         * but he flies on level. dash_steps 1 then slows him from the next
         * step, as with no target, while he has no rival */
        if (to.y > 0.0F) {
            if (record->own.sonic.dash_steps < 0) {
                to.y = 0.0F;
                record->own.sonic.dash_steps = 1;
            }
        }
        njUnitVector(&to);
        to.x = njCos(work->ang.y) * njSqrt(1.0F - (to.y * to.y));
        to.z = njSin(work->ang.y) * njSqrt(1.0F - (to.y * to.y));
        if (record->upgrades & PLAYER_SONIC_SUPER) {
            speed = SUPER_HOMING_SPEED;
        } else {
            speed = HOMING_SPEED;
        }
        if (record->own.sonic.steps > HOMING_TIRED_STEPS) {
            speed *= (rand() * (0.1F * RAND_SCALE)) + 0.7F;
        }
        /* @hack redundant: an if (0) that adds to speed, which SHC deletes.
         * it keeps to.y's copy for njSqrt at @R15 (0x8c0b5fd4); without it,
         * the copy goes to @(4,R15) and 20 of 373 words differ */
        if (0) {
            speed += speed * speed;
        }
        to.x *= speed;
        to.y *= speed;
        to.z *= speed;
        motion->speed = to;
        player_turn_back(work, &to);
        record->speed = to;
        if (record->own.sonic.dash_steps < 0) {
            record->own.sonic.dash_steps = 0;
        }
    }
}

/* restart his light attack's count of steps, record->own.sonic.steps, and
 * return its target, or null for none. the target is the nearest item or
 * enemy near him, but the rival where the rival is nearer. with no task
 * near him, there's none, rival or not */
static struct task_work *sonic_restart_attack(struct task_work *work,
                                              struct player *record)
{
    struct near_task *near;
    struct task_work *best;
    NJS_VECTOR to;
    Float best_distance;
    int rival;
    struct collision_work *collision;

    if (PLAYER_VARS(work)->player == 0) {
        near = near_tasks;
    } else {
        near = near_tasks_player1;
    }
    record->own.sonic.steps = 0;
    if (near->work == NULL) {
        return NULL;
    }
    rival = player_find_rival(PLAYER_VARS(work)->player);
    if (rival != -1) {
        best = player_work[rival];
        to.x = work->pos.x - best->pos.x;
        to.y = work->pos.y - best->pos.y;
        to.z = work->pos.z - best->pos.z;
        best_distance = njScalor2(&to);
    } else {
        best_distance = TARGET_RANGE * TARGET_RANGE;
        best = NULL;
    }
    for (; near->work != NULL; near++) {
        /* @hack spelling: the record through this local, which loads its
         * kind into R5 and extends it into R0 for the first compare
         * (0x8c0b61a4, 0x8c0b61a6); read twice through near, the kind takes
         * R4 and 6 of 87 words differ */
        collision = near->work->collision;
        if (collision->kind == COLLISION_ITEM ||
            collision->kind == COLLISION_ENEMY) {
            if (near->distance < best_distance) {
                best_distance = near->distance;
                best = near->work;
            }
        }
    }
    return best;
}

/* move him on a light attack at what sonic_restart_attack finds. he flies
 * on straight until curled_steps reaches ATTACK_STRAIGHT_STEPS. then he
 * flies at ATTACK_SPEED, turned from the way he moves toward the target by
 * the weights ATTACK_INERTIA's comment gives. FALSE with nothing to attack */
static Bool sonic_light_attack(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    struct task_work *best;
    struct collision_shape *shape;
    NJS_VECTOR to;
    NJS_VECTOR way;
    Float pull;
    Float inertia;

    if (!(best = sonic_restart_attack(work, record))) {
        record->speed.x *= 0.9F;
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        return FALSE;
    } else if (record->own.sonic.curled_steps < ATTACK_STRAIGHT_STEPS) {
        to = record->speed;
        player_turn_to_world(work, &to);
        motion->speed = to;
    } else {
        shape = best->collision->shapes;
        to = shape->center;
        if (!(shape->flags & SHAPE_IN_WORLD)) {
            njAddVector(&to, &best->pos);
        }
        njSubVector(&to, &work->pos);
        if (njScalor(&to) == 0.0F) {
            to.x = -world_gravity.x;
            to.y = -world_gravity.y;
            to.z = -world_gravity.z;
        } else {
            njUnitVector(&to);
        }
        way = motion->speed;
        if (njScalor(&way) == 0.0F) {
            way = to;
        } else {
            njUnitVector(&way);
        }
        /* @hack spelling: ATTACK_INERTIA through this local, which puts it
         * in FR4 and ATTACK_PULL_LEAST in FR5 (FMOV.S @R0,FR4 at
         * 0x8c0b631a); written in its place, the two trade and 15 of 243
         * words differ */
        inertia = ATTACK_INERTIA;
        pull = ((record->own.sonic.curled_steps & (ATTACK_PULL_STEPS - 1)) *
                ATTACK_PULL_GAIN) +
               ATTACK_PULL_LEAST;
        to.x = (way.x * inertia) + (to.x * pull);
        to.y = (way.y * inertia) + (to.y * pull);
        to.z = (way.z * inertia) + (to.z * pull);
        if (njScalor(&to) == 0.0F) {
            to = way;
        } else {
            njUnitVector(&to);
        }
        to.x *= ATTACK_SPEED;
        to.y *= ATTACK_SPEED;
        to.z *= ATTACK_SPEED;
        motion->speed = to;
        player_turn_back(work, &to);
        record->speed = to;
    }
    return TRUE;
}

/* move him on a light dash at the nearest ring within RING_CONE of the way
 * he faces, by its distance, from DASH_STEP_LEAST to DASH_STEP_MOST. with no
 * ring near him he rolls on, and with none ahead his speed stays */
static void sonic_light_dash(struct task_work *work, struct task_motion *motion,
                             struct player *record)
{
    struct near_task *near;
    struct task_work *ring;
    struct task_work *best;
    struct collision_shape *shape;
    /* @hack spelling: dir declared before to, which puts dir at @(16,R15) and
     * to at @(4,R15) (MOV #16,R0 at 0x8c0b64a0); the other way round, the
     * two trade and 24 of 234 words differ */
    NJS_VECTOR dir;
    NJS_VECTOR to;
    Float best_distance;
    Angle angle;

    if (PLAYER_VARS(work)->player == 0) {
        near = near_rings;
    } else {
        near = near_rings_player1;
    }
    if (near->work == NULL) {
        player_accel_roll(work, motion, record);
        player_apply_accel(work, motion, record);
        return;
    }
    dir.x = 1.0F;
    dir.y = 0.0F;
    dir.z = 0.0F;
    player_turn_to_world(work, &dir);
    njUnitVector(&dir);
    best = NULL;
    for (; near->work != NULL; near++) {
        ring = near->work;
        shape = ring->collision->shapes;
        to = ring->pos;
        njAddVector(&to, &shape->center);
        njSubVector(&to, &work->pos);
        njUnitVector(&to);
        angle = vector_angle(&dir, &to, NULL);
        if (angle <= RING_CONE) {
            if (best == NULL || near->distance < best_distance) {
                best = ring;
                best_distance = near->distance;
            }
        }
    }
    if (best == NULL) {
        return;
    }
    if (record->own.sonic.dash_steps < RING_STEPS) {
        record->own.sonic.dash_steps = RING_STEPS;
    }
    shape = best->collision->shapes;
    to = best->pos;
    njAddVector(&to, &shape->center);
    njSubVector(&to, &work->pos);
    work->ang.y = angle_approach(work->ang.y,
                                 (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN),
                                 LIGHT_DASH_TURN);
    motion->aim.y = work->ang.y;
    best_distance = njSqrt(best_distance);
    if (best_distance > DASH_STEP_MOST) {
        best_distance = DASH_STEP_MOST;
    } else if (best_distance < DASH_STEP_LEAST) {
        best_distance = DASH_STEP_LEAST;
    }
    if (!(njScalor(&to) > 1.0F)) {
        to = motion->speed;
    }
    njUnitVector(&to);
    motion->speed.x = to.x * best_distance;
    motion->speed.y = to.y * best_distance;
    motion->speed.z = to.z * best_distance;
    player_turn_back(work, &to);
    record->speed = to;
}

/* a hit, or ground that hurts, knocks him up and back. on a wall or the
 * board it crashes him, and when he's super it only slows him. as RACER_ID,
 * the racer Tails races, he isn't hurt. TRUE when he's knocked back or
 * crashes on a wall, for which sonic_choose_mode takes his barrier or his
 * rings. a crash on the board changes his mode but returns FALSE */
static Bool sonic_try_hurt(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    if (work->id != RACER_ID &&
        ((work->flags & PLAYER_HURT) ||
         (record != NULL && (record->ground_flags & WORLD_HURTS)))) {
        work->timer = PLAYER_HURT_STEPS;
        vibration_start_fading(PLAYER_VARS(work)->player, 7, 59, 6);
        /* @hack redundant: an else after an arm that returns, which keeps 62 in
         * R3 for the board's compare (MOV #62,R3 at 0x8c0b6716); without it,
         * R2, and 7 of 140 words differ */
        if (work->mode >= SONIC_WALL_RUN && work->mode <= SONIC_WALL_CRASH) {
            sound_play(SE_CRASH, 0, 0, 0);
            work->mode = SONIC_WALL_CRASH;
            record->anim.action = ACTION_WALL_CRASH;
            player_crash_burst(work);
            return TRUE;
        } else if (work->mode >= SONIC_BOARD &&
                   work->mode <= SONIC_BOARD_CRASH) {
            sound_play(SE_HURT, 0, 0, 0);
            work->mode = SONIC_BOARD_CRASH;
            record->anim.action = ACTION_BOARD_CRASH_MINUS;
            work->flags &= ~PLAYER_HURT;
        } else if (record->upgrades & PLAYER_SONIC_SUPER) {
            record->speed.x *= 0.5F;
        } else if (work->mode != SONIC_RIDE) {
            if (work->mode == SONIC_LIGHT_ATTACK) {
                camera_restore();
            }
            sound_play(SE_HURT, 0, 0, 0);
            work->mode = SONIC_HURT;
            record->anim.action = ACTION_HURT;
            work->flags &= ~PLAYER_HURT;
            player_mouth_set_script(0, 8);
            record->speed.y = record->consts.jump_speed;
            record->speed.x = -record->consts.running_speed;
            work->flags &= ~PLAYER_ATTACKING;
            return TRUE;
        }
    }
    return FALSE;
}

/* the jump button pressed, he jumps, from a panel to the next panel if
 * he stands on one. what he carries goes with him if it can, or he lets go
 * of it. TRUE when he jumps */
static Bool sonic_try_jump(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    int flags;

    if (player_jump_pressed(work, record) == PLAYER_JUMP_PANEL) {
        if (work->flags & PLAYER_HOLDING) {
            sonic_let_go(work, motion, record);
        }
        panel_jump(work);
        work->mode = SONIC_PANEL_JUMP;
        record->anim.action = ACTION_PANEL_JUMP;
        work->flags &= ~PLAYER_ON_TASK;
        record->jump_steps = 0;
        sound_play(SE_JUMP, 0, 0, 0);
        return TRUE;
    } else if (player_jump_pressed(work, record)) {
        if (work->flags & PLAYER_HOLDING) {
            flags = player_carry_flags(work);
            if (flags & PLAYER_CAN_CARRY) {
                work->mode = SONIC_HOLD_JUMP;
                record->speed.y = record->consts.jump_speed;
                record->anim.action = ACTION_HOLD_JUMP;
                work->flags &= ~PLAYER_ON_TASK;
                record->jump_steps = 0;
                sound_play(SE_JUMP, 0, 0, 0);
                return TRUE;
            } else {
                sonic_let_go(work, motion, record);
            }
        }
        work->mode = SONIC_JUMP;
        record->speed.y = record->consts.jump_speed;
        if (record->ground_task != NULL) {
            struct task_force *force = record->ground_task->force;

            if (force != NULL) {
                player_set_accel(PLAYER_VARS(work)->player, force->speed.x,
                                 force->speed.y, force->speed.z);
            }
        }
        record->anim.action = ACTION_SPIN;
        record->action_speed = JUMP_SPIN_PACE;
        work->flags &= ~PLAYER_ON_TASK;
        work->flags |= PLAYER_ATTACKING;
        record->jump_steps = 0;
        sound_play(SE_JUMP, 0, 0, 0);
        return TRUE;
    }
    return FALSE;
}

/* the action button pressed, he uncurls and falls. TRUE when he does */
static Bool sonic_try_uncurl(struct task_work *work, struct task_motion *motion,
                             struct player *record)
{
    if (pad_records[PLAYER_VARS(work)->player].press & pad_action_buttons) {
        work->mode = SONIC_FALL;
        player_clear_speed(NULL, record);
        record->anim.action = ACTION_FALL;
        record->action_speed = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        return TRUE;
    }
    return FALSE;
}

/* do what his event's action asks: drive him, or turn him. FALSE when it
 * asks for anything else, which clears PLAYER_EVENT_DRIVEN */
static Bool sonic_follow_event(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    struct event_work *event = work->event;

    switch (event->action) {
    case EVENT_ACTION_DONE:
    case EVENT_ACTION_RUN:
    case EVENT_ACTION_MOVE:
    case EVENT_ACTION_PUSH:
        work->flags |= PLAYER_EVENT_DRIVEN;
        break;
    case EVENT_ACTION_TURN_TO:
        player_set_heading(PLAYER_VARS(work)->player, event->move.ang.y);
        break;
    case EVENT_ACTION_TURN_BY:
        player_set_heading(PLAYER_VARS(work)->player,
                           event->move.ang.y - work->ang.y + 0x4000);
        break;
    default:
        work->flags &= ~PLAYER_EVENT_DRIVEN;
        return FALSE;
    }
    return TRUE;
}

/* do what his event's action, its paths or a request asks of him. TRUE
 * when that sets his mode. FALSE may still have changed his flags, heading
 * or submode */
static Bool sonic_try_request(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    struct task *task;

    if (work->event->action) {
        sonic_follow_event(work, motion, record);
        return FALSE;
    }
    if (work->event->paths != NULL) {
        work->mode = SONIC_EVENT;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        return TRUE;
    }
    if (!(work->flags & PLAYER_REQUEST)) {
        return FALSE;
    }
    switch (work->submode) {
    case PLAYER_MODE_SPRING:
    case PLAYER_MODE_BOUNCE_3:
        work->mode = SONIC_SPRING;
        record->anim.action = ACTION_SPRING;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_BOUNCE_1:
        work->mode = SONIC_SPRING;
        record->anim.action = ACTION_BOUNCE_1;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_BOUNCE_2:
        work->mode = SONIC_SPRING;
        record->anim.action = ACTION_BOUNCE_2;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_LAUNCH:
        work->mode = SONIC_LAUNCH;
        record->anim.action = ACTION_SPRING;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_FLING:
        work->mode = SONIC_FLUNG;
        record->anim.action = ACTION_FLUNG;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_UPDRAFT:
        work->mode = SONIC_FALL;
        record->anim.action = ACTION_UPDRAFT;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_PATH:
        work->mode = SONIC_PATH;
        record->anim.action = ACTION_RUN;
        record->path_distance = 0.0F;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_PATH_SPIN:
        work->mode = SONIC_PATH_SPIN;
        record->anim.action = ACTION_ROLL;
        record->path_distance = 0.0F;
        work->flags |= PLAYER_ATTACKING;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_STAND;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL_RUN:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_SPRINT;
        player_clear_speed(motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_BALL:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_SPIN;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags |= PLAYER_ATTACKING;
        record->action_speed = JUMP_SPIN_PACE;
        sound_play(SE_SPIN, 0, 0, 0);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL_PUSH:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_PUSH_START;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_HANG_ON:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_HANG_ON;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_GET_ON:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_FALL;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_RIDE:
        work->mode = SONIC_RIDE;
        record->anim.action = ACTION_RIDE;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_CLEAR:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_CLEAR;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_ROCKET:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_ROCKET;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_ROCKET_ALONG:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_ROCKET_ALONG;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_ROCKET_UP:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_ROCKET_UP;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_PATH_SLIDE:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_PATH_SLIDE;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_SQUASH_2:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_SQUASH_2;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_SQUASH_3:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_SQUASH_3;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RESET:
        sonic_stand(work, motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_LAUNCH_SPEED:
        work->mode = SONIC_SPIN_DASH;
        record->anim.action = ACTION_ROLL;
        work->flags |= PLAYER_ATTACKING;
        sound_play(SE_SPIN_DASH, 0, 0, 0);
        break;
    case PLAYER_MODE_JOIN_PATH:
        if (work->mode == SONIC_WALK || work->mode == SONIC_SPIN_DASH ||
            record->speed.x > record->consts.running_speed) {
            work->flags |= PLAYER_ON_PATH;
        }
        return FALSE;
    case PLAYER_MODE_WALL_FALL:
        work->mode = SONIC_WALL_FALL;
        record->anim.action = ACTION_FALL;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_BOARD:
        work->mode = SONIC_BOARD_FALL;
        record->anim.action = ACTION_BOARD_AIR;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_HANG:
        sound_play(SE_GRAB, 0, 0, 0);
        work->mode = SONIC_HANG;
        record->anim.action = ACTION_HANG_HALFWAY;
        record->anim.frame = 0.0F;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_LEDGE:
        if (work->flags & PLAYER_ON_GROUND) {
            return FALSE;
        }
        sound_play(SE_GRAB, 0, 0, 0);
        work->mode = SONIC_LEDGE;
        record->anim.action = ACTION_LEDGE;
        record->anim.frame = 0.0F;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_CIRCLE:
        if (work->mode >= SONIC_CIRCLE && work->mode <= SONIC_CIRCLE_RIGHT) {
            work->submode = 0;
            return FALSE;
        }
        {
            struct task_work *held = record->held->work;
            struct collision_shape *shape = held->collision->shapes;

            if (work->pos.y > held->pos.y + shape->center.y - shape->size_y) {
                work->mode = SONIC_CIRCLE_DOWN;
                record->anim.action = ACTION_CIRCLE_DOWN;
            } else {
                work->mode = SONIC_CIRCLE;
                record->anim.action = ACTION_CIRCLE;
            }
        }
        sound_play(SE_GRAB, 0, 0, 0);
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        record->own.sonic.steps = 0;
        break;
    case PLAYER_MODE_DANGLE:
        if (work->mode >= SONIC_DANGLE && work->mode <= SONIC_DANGLE_CLIMB) {
            work->submode = 0;
            return FALSE;
        }
        if (record->held->work->pos.y > work->pos.y + DANGLE_REACH) {
            work->mode = SONIC_DANGLE_CLIMB;
            record->anim.action = ACTION_DANGLE_CLIMB;
        } else {
            work->mode = SONIC_DANGLE;
            record->anim.action = ACTION_DANGLE;
        }
        sound_play(SE_GRAB, 0, 0, 0);
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        record->own.sonic.steps = 0;
        break;
    case PLAYER_MODE_SLOPE_ROLL:
        work->mode = SONIC_SLOPE_ROLL;
        record->anim.action = ACTION_SLOPE_ROLL;
        work->flags |= PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_CAUGHT:
        work->mode = SONIC_CAUGHT;
        record->anim.action = ACTION_CAUGHT;
        record->own.sonic.steps = STRUGGLE_START;
        player_clear_speed(motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        sound_play(SE_CAUGHT, 0, 0, 0);
        break;
    case PLAYER_MODE_FREEZE:
        if (work->mode != SONIC_ICED) {
            work->mode = SONIC_ICED;
            record->anim.action = ACTION_ICED;
            record->own.sonic.steps = ICE_START;
            ice_create(PLAYER_VARS(work)->player);
            player_clear_speed(motion, record);
            if (work->flags & PLAYER_BALL) {
                work->flags &= ~PLAYER_ATTACKING;
            }
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case PLAYER_MODE_KNOCK_BACK:
        work->mode = SONIC_CRASH;
        record->anim.action = ACTION_CRASH;
        sound_play(SE_CRASH, 0, 0, 0);
        record->own.sonic.dash_steps = 0;
        player_crash_burst(work);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_READY:
        work->mode = SONIC_PLAY;
        record->anim.action = ACTION_READY;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_SHOWER:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_SHOWER;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_SHOWER_END:
        work->mode = SONIC_PLAY;
        record->anim.action = ACTION_SHOWER_END;
        break;
    case PLAYER_MODE_MUD_OUT:
        work->mode = SONIC_PLAY;
        record->anim.action = ACTION_MUD_OUT;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_BREATHE:
        work->mode = SONIC_PLAY;
        record->anim.action = ACTION_BREATHE;
        record->underwater_steps = 0;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_HOLD:
        work->mode = SONIC_HOLD_STAND;
        record->anim.action = ACTION_HOLD_STAND;
        record->held = PLAYER_VARS(work)->arg.carry;
        record->held->work->flags |= CARRY_HELD;
        work->flags |= PLAYER_HOLDING;
        work->flags &= ~COLLISION_TOUCHING;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_SUPER:
        record->upgrades |= PLAYER_SONIC_SUPER;
        task = task_create(TASK_WORK, 2, super_aura_start);
        if (task != NULL) {
            PLAYER_VARS(task->work)->player = PLAYER_VARS(work)->player;
        }
        /* @hack spelling: the task tested as it's assigned, which tests R0
         * with the copy to R4 in the BT/S slot (0x8c0b737a to 0x8c0b737e);
         * assigned first, SHC tests R4 and 3 of 833 words differ */
        if ((task = task_create(TASK_EXTRA, 2, sonic_super_start)) != NULL) {
            SUPER(task)->player = PLAYER_VARS(work)->player;
        }
        work->mode = SONIC_SUPER_STAND;
        record->anim.action = ACTION_SUPER_STAND;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_DROWN:
        work->mode = SONIC_DROWN;
        record->anim.action = ACTION_DROWN;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_DOWN:
        work->mode = SONIC_DOWN;
        record->anim.action = ACTION_DOWN;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        sound_play(SE_SONIC_DOWN, 0, 0, 0);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/* sonic_try_request while he holds something. most requests make him let
 * go of it */
static Bool sonic_try_request_holding(struct task_work *work,
                                      struct task_motion *motion,
                                      struct player *record)
{
    struct task *task;

    if (work->event->action) {
        sonic_follow_event(work, motion, record);
        return FALSE;
    }
    if (!(work->flags & PLAYER_REQUEST)) {
        return FALSE;
    }
    switch (work->submode) {
    case PLAYER_MODE_SPRING:
    case PLAYER_MODE_BOUNCE_1:
    case PLAYER_MODE_BOUNCE_2:
    case PLAYER_MODE_BOUNCE_3:
        work->mode = SONIC_HOLD_SPRING;
        record->anim.action = ACTION_HOLD_JUMP;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_LAUNCH:
        work->mode = SONIC_HOLD_LAUNCH;
        record->anim.action = ACTION_HOLD_JUMP;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_UPDRAFT:
        work->mode = SONIC_HOLD_FALL;
        record->anim.action = ACTION_HOLD_FALL;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_PATH:
        sonic_let_go(work, motion, record);
        work->mode = SONIC_PATH;
        record->anim.action = ACTION_RUN;
        record->path_distance = 0.0F;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_PATH_SPIN:
        sonic_let_go(work, motion, record);
        work->mode = SONIC_PATH_SPIN;
        record->anim.action = ACTION_ROLL;
        record->path_distance = 0.0F;
        work->flags |= PLAYER_ATTACKING;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_HOLD_STAND;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_BALL:
        sonic_let_go(work, motion, record);
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_SPIN;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags |= PLAYER_ATTACKING;
        record->action_speed = JUMP_SPIN_PACE;
        sound_play(SE_SPIN, 0, 0, 0);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL_PUSH:
        sonic_let_go(work, motion, record);
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_PUSH_START;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL_RUN:
        sonic_let_go(work, motion, record);
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_SPRINT;
        player_clear_speed(motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_HANG_ON:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_HANG_ON;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_GET_ON:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_FALL;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RIDE:
        work->mode = SONIC_RIDE;
        record->anim.action = ACTION_RIDE;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_CLEAR:
        sonic_let_go(work, motion, record);
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_CLEAR;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_ROCKET:
        sonic_let_go(work, motion, record);
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_ROCKET;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_PATH_SLIDE:
        sonic_let_go(work, motion, record);
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_PATH_SLIDE;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_SQUASH_2:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_HOLD_STAND;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_SQUASH_3:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_HOLD_STAND;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RESET:
        work->mode = SONIC_HOLD_STAND;
        record->anim.action = ACTION_HOLD_STAND;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_LAUNCH_SPEED:
        sonic_let_go(work, motion, record);
        work->mode = SONIC_SPIN_DASH;
        record->anim.action = ACTION_ROLL;
        work->flags |= PLAYER_ATTACKING;
        sound_play(SE_SPIN_DASH, 0, 0, 0);
        break;
    case PLAYER_MODE_WALL_FALL:
        work->mode = SONIC_WALL_FALL;
        record->anim.action = ACTION_FALL;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_JOIN_PATH:
        if (work->mode == SONIC_HOLD_WALK ||
            record->speed.x > record->consts.running_speed) {
            work->flags |= PLAYER_ON_PATH;
        }
        return FALSE;
    case PLAYER_MODE_HANG:
        sound_play(SE_GRAB, 0, 0, 0);
        sonic_let_go(work, motion, record);
        work->mode = SONIC_HANG;
        record->anim.action = ACTION_HANG_HALFWAY;
        record->anim.frame = 0.0F;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_LEDGE:
        if (work->flags & PLAYER_ON_GROUND) {
            return FALSE;
        }
        sonic_let_go(work, motion, record);
        sound_play(SE_GRAB, 0, 0, 0);
        work->mode = SONIC_LEDGE;
        record->anim.action = ACTION_LEDGE;
        record->anim.frame = 0.0F;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_CAUGHT:
        sonic_let_go(work, motion, record);
        work->mode = SONIC_CAUGHT;
        record->anim.action = ACTION_CAUGHT;
        record->own.sonic.steps = STRUGGLE_START;
        player_clear_speed(motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_FREEZE:
        if (work->mode != SONIC_ICED) {
            sonic_let_go(work, motion, record);
            work->mode = SONIC_ICED;
            record->anim.action = ACTION_ICED;
            record->own.sonic.steps = ICE_START;
            player_clear_speed(motion, record);
            ice_create(PLAYER_VARS(work)->player);
            if (work->flags & PLAYER_BALL) {
                work->flags &= ~PLAYER_ATTACKING;
            }
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case PLAYER_MODE_KNOCK_BACK:
        sonic_let_go(work, motion, record);
        work->mode = SONIC_CRASH;
        record->anim.action = ACTION_CRASH;
        sound_play(SE_CRASH, 0, 0, 0);
        record->own.sonic.dash_steps = 0;
        player_crash_burst(work);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_READY:
        sonic_let_go(work, motion, record);
        work->mode = SONIC_PLAY;
        record->anim.action = ACTION_READY;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_SUPER:
        sonic_let_go(work, motion, record);
        record->upgrades |= PLAYER_SONIC_SUPER;
        task = task_create(TASK_WORK, 2, super_aura_start);
        if (task != NULL) {
            PLAYER_VARS(task->work)->player = PLAYER_VARS(work)->player;
        }
        /* @hack spelling: the task tested as it's assigned, which tests R0
         * with the copy to R4 in the BT/S slot (0x8c0b7a6e to 0x8c0b7a72);
         * assigned first, SHC tests R4 and 3 of 616 words differ */
        if ((task = task_create(TASK_EXTRA, 2, sonic_super_start)) != NULL) {
            SUPER(task)->player = PLAYER_VARS(work)->player;
        }
        work->mode = SONIC_SUPER_STAND;
        record->anim.action = ACTION_SUPER_STAND;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_DROWN:
        sonic_let_go(work, motion, record);
        work->mode = SONIC_DROWN;
        record->anim.action = ACTION_DROWN;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_DOWN:
        sonic_let_go(work, motion, record);
        work->mode = SONIC_DOWN;
        record->anim.action = ACTION_DOWN;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        sound_play(SE_SONIC_DOWN, 0, 0, 0);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/* sonic_try_request on the board. it takes PLAYER_MODE_RESET, which drops
 * him into SONIC_FALL, and PLAYER_MODE_CLEAR, and no other request */
static Bool sonic_try_request_board(struct task_work *work,
                                    struct task_motion *motion,
                                    struct player *record)
{
    if (!(work->flags & PLAYER_REQUEST)) {
        return FALSE;
    }
    if (work->submode == PLAYER_MODE_RESET) {
        work->mode = SONIC_FALL;
        record->anim.action = ACTION_FALL;
        return TRUE;
    } else if (work->submode == PLAYER_MODE_CLEAR) {
        sonic_let_go(work, motion, record);
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_CLEAR;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        return TRUE;
    }
    return FALSE;
}

/* sonic_try_request while he's super, with PLAYER_MODE_SUPER_END to end
 * it */
static Bool sonic_try_request_super(struct task_work *work,
                                    struct task_motion *motion,
                                    struct player *record)
{
    if (!(work->flags & PLAYER_REQUEST)) {
        return FALSE;
    }
    switch (work->submode) {
    case PLAYER_MODE_SPRING:
        work->mode = SONIC_SUPER_SPRING;
        record->anim.action = ACTION_SUPER_SPRING;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_LAUNCH:
        work->mode = SONIC_SUPER_LAUNCH;
        record->anim.action = ACTION_SUPER_SPRING;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_PATH:
        work->mode = SONIC_SUPER_PATH;
        record->anim.action = ACTION_SUPER_RUN;
        record->path_distance = 0.0F;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL_RUN:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_SUPER_SPRINT;
        player_clear_speed(motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_SUPER_STAND;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_BALL:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_SUPER_BALL;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags |= PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_CLEAR:
        work->mode = SONIC_POSE;
        record->anim.action = ACTION_SUPER_CLEAR;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_RESET:
        work->mode = SONIC_SUPER_STAND;
        record->anim.action = ACTION_SUPER_STAND;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_SUPER_END:
        record->upgrades &= ~PLAYER_SONIC_SUPER;
        sonic_stand(work, motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_JOIN_PATH:
        if (work->mode == SONIC_SUPER_WALK ||
            record->speed.x > record->consts.running_speed) {
            work->flags |= PLAYER_ON_PATH;
        }
        return FALSE;
    case PLAYER_MODE_DOWN:
        record->upgrades &= ~PLAYER_SONIC_SUPER;
        work->mode = SONIC_DOWN;
        record->anim.action = ACTION_DOWN;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        sound_play(SE_SONIC_DOWN, 0, 0, 0);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/* off the ground, he falls, still curled up if he was. what he carries
 * goes with him if it can, or he lets go of it. TRUE when he falls */
static Bool sonic_try_fall(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    if (!(work->flags & PLAYER_ON_GROUND)) {
        /* @hack redundant: an else after an arm that returns, which loads
         * PLAYER_HOLDING's mask into R2 (0x8c0b7e84); without it, R3, and 2
         * of 50 words differ */
        if (work->flags & PLAYER_BALL) {
            work->mode = SONIC_JUMP;
            return TRUE;
        } else {
            if (work->flags & PLAYER_HOLDING) {
                if (player_carry_flags(work) & PLAYER_CAN_CARRY) {
                    work->mode = SONIC_HOLD_FALL;
                    record->anim.action = ACTION_HOLD_FALL;
                    return TRUE;
                }
                sonic_let_go(work, motion, record);
            }
            work->mode = SONIC_FALL;
            record->anim.action = ACTION_FALL;
        }
        return TRUE;
    }
    return FALSE;
}

/* stopped with the stick let go, he stands, or stands holding what he
 * carries, or holds on to what he can push or shake. TRUE when he does */
static Bool sonic_try_stop(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    if (!player_read_stick(work, NULL, NULL) && record->speed.x == 0.0F) {
        if (work->flags & PLAYER_HOLDING) {
            if (player_carry_flags(work) &
                (PLAYER_CAN_PUSH | PLAYER_CAN_SHAKE)) {
                work->mode = SONIC_GRIP;
                record->anim.action = ACTION_GRIP;
            } else {
                work->mode = SONIC_HOLD_STAND;
                record->anim.action = ACTION_HOLD_STAND;
            }
        } else {
            sonic_stand(work, motion, record);
        }
        return TRUE;
    }
    return FALSE;
}

/* the action button pressed, he curls up in place and starts to charge a
 * spin dash. TRUE when he does */
static Bool sonic_try_spin(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    if (pad_records[PLAYER_VARS(work)->player].press & pad_action_buttons) {
        work->mode = SONIC_SPIN_CHARGE;
        record->anim.action = ACTION_SPIN;
        work->flags |= PLAYER_ATTACKING;
        record->own.sonic.curled_steps = 0;
        if (record->speed.x > SPIN_PACE_LEAST) {
            record->action_speed = record->speed.x;
        } else {
            record->action_speed = SPIN_PACE_LEAST;
        }
        sound_play_for(SE_SPIN, (int)work, 1, 0, 2);
        return TRUE;
    }
    return FALSE;
}

/* the action button let go with his light dash charged, he dashes. with
 * the ancient light he attacks what he can, and else he dashes along the
 * rings. TRUE when he does */
static Bool sonic_try_light_dash(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record)
{
    /* @hack redundant: target, stored and never read, which takes a frame
     * slot (ADD #-4,R15 at 0x8c0b8048); without the store, the frame is 16
     * bytes, not 20, and 88 of 132 words differ */
    struct task_work *target;

    if (!(pad_records[PLAYER_VARS(work)->player].on & pad_action_buttons) &&
        (work->flags & SONIC_CHARGED)) {
        if ((record->upgrades & PLAYER_SONIC_ANCIENT_LIGHT) &&
            (target = sonic_restart_attack(work, record)) != NULL) {
            work->flags &= ~SONIC_CHARGED;
            work->mode = SONIC_LIGHT_ATTACK;
            record->anim.action = ACTION_ROLL;
            record->speed.y = ATTACK_RISE;
            work->flags |= PLAYER_ATTACKING;
            sound_play(SE_LIGHT_DASH, 0, 0, 0);
            record->own.sonic.curled_steps = 0;
            camera_override(camera_step_eyes, 0, 0);
            work->collision->shapes->levels =
                (work->collision->shapes->levels & ~SHAPE_ATTACK) | ATTACK(3);
            work->collision->shapes->levels =
                (work->collision->shapes->levels & ~SHAPE_GUARD) | GUARD(3);
            return TRUE;
        }
        work->flags &= ~SONIC_CHARGED;
        work->mode = SONIC_LIGHT_DASH;
        record->anim.action = ACTION_LIGHT_DASH;
        record->own.sonic.dash_steps = LIGHT_DASH_STEPS;
        record->speed.x = LIGHT_DASH_SPEED;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_GUARD;
        sound_play(SE_LIGHT_DASH, 0, 0, 0);
        sound_stop(SE_CHARGED);
        sound_play(SE_LIGHT_DASH_START, 0, 0, 0);
        return TRUE;
    }
    return FALSE;
}

/* charge the spin dash while the action button is held, and send him off
 * once it's let go. held CHARGE_STEPS with the light speed shoes, or
 * RING_CHARGE_STEPS with the crystal ring too, it charges a light dash
 * instead. CURL_STEPS curled up end it. TRUE when his mode changes */
static Bool sonic_spin_charge(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    if ((record->own.sonic.curled_steps >= RING_CHARGE_STEPS &&
         (record->upgrades &
          (PLAYER_SONIC_CRYSTAL_RING | PLAYER_SONIC_LIGHT_SHOES)) ==
             (PLAYER_SONIC_CRYSTAL_RING | PLAYER_SONIC_LIGHT_SHOES)) ||
        (record->own.sonic.curled_steps >= CHARGE_STEPS &&
         (record->upgrades & PLAYER_SONIC_LIGHT_SHOES))) {
        work->flags |= SONIC_CHARGED;
        sonic_charged_create(work, motion, record);
        record->own.sonic.curled_steps = 0;
        if (record->speed.x <= 0.0F) {
            work->mode = SONIC_STAND;
            record->idle_steps = 0;
        } else {
            work->mode = SONIC_WALK;
        }
        record->anim.action = ACTION_UNCURL;
        work->flags &= ~PLAYER_ATTACKING;
        sound_stop(SE_SPIN);
        sound_stop(SE_LIGHT_CHARGE);
        sound_play(SE_CHARGED, 0, 0, 0);
        return TRUE;
    } else if (record->own.sonic.curled_steps >= CURL_STEPS) {
        record->own.sonic.curled_steps = 0;
        if (record->speed.x <= 0.0F) {
            work->mode = SONIC_STAND;
            record->idle_steps = 0;
        } else {
            work->mode = SONIC_WALK;
        }
        record->anim.action = ACTION_UNCURL;
        work->flags &= ~PLAYER_ATTACKING;
        sound_stop(SE_SPIN);
        sound_stop(SE_LIGHT_CHARGE);
        return TRUE;
    } else if (!PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                            pad_action_buttons)) {
        work->mode = SONIC_SPIN_DASH;
        record->anim.action = ACTION_ROLL;
        record->speed.x = record->action_speed;
        sound_play(SE_SPIN_DASH, 0, 0, 0);
        sound_stop(SE_SPIN);
        sound_stop(SE_LIGHT_CHARGE);
        record->own.sonic.curled_steps = 0;
        record->action_speed = 0.0F;
        return TRUE;
    }
    if (SPIN_PACE_MOST > record->action_speed) {
        record->action_speed += SPIN_PACE_GAIN;
    }
    if (record->upgrades & PLAYER_SONIC_LIGHT_SHOES) {
        sound_play_for(SE_LIGHT_CHARGE, (int)work, 1, 0, 2);
    } else {
        sound_play_for(SE_SPIN, (int)work, 1, 0, 2);
    }
    return FALSE;
}

/* slower than a run, he uncurls from his spin dash. TRUE when he does */
static Bool sonic_try_unroll(struct task_work *work, struct task_motion *motion,
                             struct player *record)
{
    /* @hack spelling: ACTION_UNCURL through this local, which SHC loads
     * into R5 in the BT/S slot of the speed's test (MOV #29,R5 at
     * 0x8c0b83a0); stored in its place, 11 of 25 words differ */
    Uint16 action;

    if (record->speed.x < record->consts.running_speed) {
        action = ACTION_UNCURL;
        if (record->speed.x <= 0.0F) {
            work->mode = SONIC_STAND;
            record->idle_steps = 0;
        } else {
            work->mode = SONIC_WALK;
        }
        record->anim.action = action;
        return TRUE;
    }
    return FALSE;
}

/* the grab button pressed as he touches something, whichever way he faces
 * it, he starts to lift what he can carry, or takes hold of what he can
 * push or shake. TRUE when he does */
static Bool sonic_try_grab(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    int flags;

    if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                    pad_grab_buttons)) {
        if ((flags = player_carry_flags(work)) != 0) {
            switch (player_face_wall(work, motion)) {
            /* @hack spelling: PLAYER_WALL_HEAD_ON's label first, which ends the
             * compares with a BF past the arm, so the arm follows them (BF
             * at 0x8c0b8430); in enum order, a BT to the arm and a BRA past
             * it end them, and 22 of 89 words differ */
            case PLAYER_WALL_HEAD_ON:
            case PLAYER_WALL_AWAY:
            case PLAYER_WALL_ACROSS:
            case PLAYER_WALL_ASKEW:
                if (flags & PLAYER_CAN_PUSH) {
                    work->mode = SONIC_GRIP;
                    record->anim.action = ACTION_GRIP_START;
                    record->held = work->collision->hit->owner;
                    work->flags |= PLAYER_HOLDING;
                    return TRUE;
                } else if (flags & PLAYER_CAN_CARRY) {
                    work->mode = SONIC_LIFT;
                    record->anim.action = ACTION_LIFT;
                    if (record->speed.x >= record->consts.trot_speed) {
                        record->speed.x = record->consts.trot_speed;
                    }
                    return TRUE;
                } else if (flags & PLAYER_CAN_SHAKE) {
                    work->mode = SONIC_GRIP;
                    record->anim.action = ACTION_GRIP_START;
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

/* from LIFT_FRAME of ACTION_LIFT, he has what he touches if he faces it
 * head on and can carry it, or else he stands. TRUE when he has it */
static Bool sonic_try_take_lifted(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record)
{
    int flags;

    player_approach_touched(work, motion);
    if (record->anim.frame >= LIFT_FRAME) {
        if ((flags = player_carry_flags(work)) != 0) {
            /* @hack spelling: a switch of one case, which puts sound_play's
             * address in R1 and SONIC_HOLD_STAND in R2 (0x8c0b8544,
             * 0x8c0b8546); as an if, R2 and R3, and 6 of 73 words differ */
            switch (player_face_wall(work, motion)) {
            case PLAYER_WALL_HEAD_ON:
                if (flags & PLAYER_CAN_CARRY) {
                    work->mode = SONIC_HOLD_STAND;
                    sound_play(SE_LIFT, 0, 0, 0);
                    record->held = work->collision->hit->owner;
                    record->held->work->flags |= CARRY_HELD;
                    work->flags |= PLAYER_HOLDING;
                    work->flags &= ~COLLISION_TOUCHING;
                    return TRUE;
                }
                break;
            }
        }
        sonic_stand(work, motion, record);
    }
    return FALSE;
}

/* the grab button pressed with something in hand, he throws it, on the
 * ground or in the air, at a target if there's one. on the ground slower
 * than a trot, he puts it down instead. no longer holding it, he stands or
 * falls, and it's let go. TRUE when either happens */
static Bool sonic_try_throw(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    if (pad_records[PLAYER_VARS(work)->player].press & pad_grab_buttons) {
        if (!(work->flags & PLAYER_ON_GROUND)) {
            work->mode = SONIC_AIR_THROW;
            if (player_has_target(PLAYER_VARS(work)->player)) {
                record->anim.action = ACTION_AIR_THROW_AT;
            } else {
                record->anim.action = ACTION_AIR_THROW;
            }
        } else if (record->speed.x < record->consts.trot_speed) {
            work->mode = SONIC_PUT_DOWN;
            record->anim.action = ACTION_PUT_DOWN;
            player_clear_speed(motion, record);
        } else {
            work->mode = SONIC_THROW;
            if (player_has_target(PLAYER_VARS(work)->player)) {
                record->anim.action = ACTION_THROW_AT;
            } else {
                record->anim.action = ACTION_THROW;
            }
        }
        return TRUE;
    } else if (!(work->flags & PLAYER_HOLDING)) {
        if (work->flags & PLAYER_ON_GROUND) {
            sonic_stand(work, motion, record);
        } else {
            work->mode = SONIC_FALL;
            record->anim.action = ACTION_FALL;
        }
        if (record->held != NULL) {
            record->held->work->flags &= ~CARRY_HELD;
            record->held = NULL;
        }
        return TRUE;
    }
    return FALSE;
}

/* let go of what he throws at its action's frame: at the nearest target,
 * straight ahead, or where he puts it down. TRUE once he has let go */
static Bool sonic_try_release(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    struct near_task *near;
    struct near_task *each;
    struct task_work *held;
    struct task_work *target;
    struct collision_shape *shape;
    Float distance;
    NJS_VECTOR toss;
    int action;

    if (PLAYER_VARS(work)->player == 0) {
        near = near_tasks;
    } else {
        near = near_tasks_player1;
    }
    if (record->held == NULL) {
        if (!(work->flags & PLAYER_HOLDING)) {
            return FALSE;
        }
        work->flags &= ~PLAYER_HOLDING;
        return TRUE;
    }
    if ((((action = record->anim.action) == ACTION_THROW_AT &&
          record->anim.frame == THROW_AT_FRAME) ||
         (action == ACTION_AIR_THROW_AT &&
          record->anim.frame == AIR_THROW_AT_FRAME)) &&
        near->work != NULL) {
        held = record->held->work;
        if (held != NULL) {
            held->flags &= ~CARRY_HELD;
            each = near;
            target = each->work;
            distance = each->distance;
            for (; each->work != NULL; each++) {
                if (each->distance < distance) {
                    distance = each->distance;
                    target = each->work;
                }
            }
            shape = target->collision->shapes;
            toss.x = target->pos.x + shape->center.x - work->pos.x;
            toss.y = target->pos.y + shape->center.y - work->pos.y;
            toss.z = target->pos.z + shape->center.z - work->pos.z;
            toss.x = njSqrt((toss.x * toss.x) + (toss.z * toss.z));
            toss.z = 0.0F;
            njUnitVector(&toss);
            toss.x *= 3.0F;
            toss.y *= 3.0F;
            player_turn_to_world(work, &toss);
            record->held->motion->speed = toss;
        }
        work->flags &= ~PLAYER_HOLDING;
        record->held = NULL;
        sound_play(SE_THROW, 0, 0, 0);
        return TRUE;
    }
    action = record->anim.action;
    if ((action == ACTION_THROW && record->anim.frame == THROW_FRAME) ||
        (action == ACTION_AIR_THROW && record->anim.frame == AIR_THROW_FRAME)) {
        held = record->held->work;
        if (held != NULL) {
            held->flags &= ~CARRY_HELD;
            toss.x = 2.0F;
            toss.y = 0.8F;
            toss.z = 0.0F;
            player_turn_to_world(work, &toss);
            record->held->motion->speed = toss;
        }
        work->flags &= ~PLAYER_HOLDING;
        record->held = NULL;
        sound_play(SE_THROW, 0, 0, 0);
        return TRUE;
    }
    if (record->anim.action == ACTION_PUT_DOWN &&
        record->anim.frame == PUT_DOWN_FRAME) {
        held = record->held->work;
        if (held != NULL) {
            held->flags &= ~CARRY_HELD;
        }
        work->flags &= ~PLAYER_HOLDING;
        record->held = NULL;
        return TRUE;
    }
    return FALSE;
}

/* holding on to what he touches with grab held, he shakes what he can
 * shake, pushes what's head on, and pulls the rest. with grab let go, he
 * lifts what he can carry and lets go of the rest. TRUE when his mode
 * changes */
static Bool sonic_try_grip(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    int wall;

    if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                    pad_grab_buttons)) {
        wall = player_stick_into_wall(work, motion);
        /* @hack spelling: a return in each arm, which puts SONIC_PUSH in R3
         * before the action's offset (MOV #15,R3 at 0x8c0b8a82); with one
         * return after the arms, 8 of 100 words differ */
        if (wall == PLAYER_WALL_NO_STICK) {
            return FALSE;
        } else if (player_carry_flags(work) & PLAYER_CAN_SHAKE) {
            work->mode = SONIC_SHAKE;
            record->anim.action = ACTION_SHAKE;
            return TRUE;
        } else if (wall == PLAYER_WALL_HEAD_ON) {
            work->mode = SONIC_PUSH;
            record->anim.action = ACTION_PUSH;
            sound_play(SE_PUSH_START, 0, 0, 0);
            return TRUE;
        } else {
            work->mode = SONIC_PULL;
            record->anim.action = ACTION_PULL;
            return TRUE;
        }
    } else {
        if (player_carry_flags(work) & PLAYER_CAN_CARRY) {
            work->mode = SONIC_LIFT;
            record->anim.action = ACTION_LIFT;
            if (record->speed.x >= record->consts.trot_speed) {
                record->speed.x = record->consts.trot_speed;
            }
        } else {
            sonic_let_go(work, motion, record);
            sonic_stand(work, motion, record);
        }
        return TRUE;
    }
    /* @hack redundant: a return that no path reaches, which the stick's test
     * branches to (BT at 0x8c0b8a62); without it, that test returns in place
     * and 37 of 100 words differ */
    return FALSE;
}

/* let go of what he holds. what he can carry, he tosses up and ahead */
static void sonic_let_go(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    struct task_work *held;
    NJS_VECTOR toss;

    if (work->flags & PLAYER_HOLDING) {
        held = record->held->work;
        if (held != NULL) {
            held->flags &= ~CARRY_HELD;
        }
        work->flags &= ~PLAYER_HOLDING;
        if (player_carry_flags(work) & PLAYER_CAN_CARRY) {
            toss.x = 1.0F;
            toss.y = 1.0F;
            toss.z = 0.0F;
            player_turn_to_world(work, &toss);
            record->held->motion->speed = toss;
        }
        record->held = NULL;
    }
}

/* put what he carries between his hands, or past his fingers in a throw.
 * what he holds and can't carry, he lets go of on a step he touches
 * something else, and he then stands or falls */
static void sonic_carry(struct task_work *work, struct task_motion *motion,
                        struct player *record)
{
    struct collision_shape *shape;
    struct task_work *held;
    NJS_POINT3 hands;
    NJS_VECTOR offset;
    int action;

    held = record->held->work;
    shape = work->collision->shapes;
    if (!(player_carry_flags(work) & PLAYER_CAN_CARRY)) {
        if (work->collision->hit == NULL) {
            return;
        } else if (held != work->collision->hit->owner->work) {
            sonic_let_go(work, motion, record);
            if (record->held == NULL) {
                if (work->flags & PLAYER_ON_GROUND) {
                    sonic_stand(work, motion, record);
                } else {
                    work->mode = SONIC_FALL;
                    record->anim.action = ACTION_FALL;
                }
            }
        }
        return;
    }
    held->pos = shape->center;
    action = record->anim.action;
    if (action == ACTION_THROW_AT || action == ACTION_THROW ||
        action == ACTION_AIR_THROW_AT || action == ACTION_AIR_THROW) {
        hands.x = record->weld_point0.x + (record->weld_axis0.x * 2.5F);
        hands.y = record->weld_point0.y + (record->weld_axis0.y * 2.5F);
        hands.z = record->weld_point0.z + (record->weld_axis0.z * 2.5F);
    } else {
        hands.x = (record->weld_point0.x + record->weld_point1.x) * 0.5F;
        hands.y = (record->weld_point0.y + record->weld_point1.y) * 0.5F;
        hands.z = (record->weld_point0.z + record->weld_point1.z) * 0.5F;
    }
    hands.x -= held->collision->shapes->center.x;
    hands.y -= held->collision->shapes->center.y;
    hands.z -= held->collision->shapes->center.z;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, 0x8000 - work->ang.y);
    njCalcVector(NULL, &hands, &offset);
    njPopMatrix(1);
    njAddVector(&held->pos, &offset);
}

/* put him beside what he goes round, its reach back from it along the way
 * he faces, or CIRCLE_REACH when it has no motion */
static void sonic_circle_place(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    struct task_work *target;
    struct task_motion *held;
    Float distance;

    target = record->held->work;
    held = record->held->motion;
    if (held == NULL) {
        distance = CIRCLE_REACH;
    } else {
        distance = CARRY_REACH(held);
    }
    work->pos.x = target->pos.x - (njCos(work->ang.y) * distance);
    work->pos.z = target->pos.z - (njSin(work->ang.y) * distance);
}

/* the angle he turns this step round what he goes round, as
 * CIRCLE_ARC_SCALE and his action's frames from CIRCLE_START to CIRCLE_END
 * set it */
static int sonic_circle_step(struct task_work *work, struct task_motion *motion,
                             struct player *record)
{
    struct task_motion *held = record->held->motion;
    Float frame = record->anim.frame;
    int step;

    if (held == NULL || CARRY_REACH(held) == 0.0F) {
        step = CIRCLE_TURN;
    } else {
        step = (int)(65536.0F / (CARRY_REACH(held) * CIRCLE_ARC_SCALE));
    }
    if (frame >= CIRCLE_END || frame <= CIRCLE_START) {
        step = 0;
    } else if (frame >= CIRCLE_SLOW) {
        step = (int)((CIRCLE_END - frame) * step * 0.1F);
    } else if (frame <= CIRCLE_FULL) {
        step = (int)((frame - CIRCLE_START) * step * 0.1F);
    }
    return step;
}

/* put him DANGLE_BEHIND back from what he hangs below, and count the
 * step */
static void sonic_dangle_below(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    struct task_work *target;

    target = record->held->work;
    record->own.sonic.steps++;
    motion->aim.y = 0x8000 - work->ang.y;
    work->pos.x = target->pos.x - (njCos(work->ang.y) * DANGLE_BEHIND);
    work->pos.z = target->pos.z - (njSin(work->ang.y) * DANGLE_BEHIND);
}

/* in ACTION_FULL_SPEED, each toe blends between its two shapes as his
 * stride goes, by his action's frame. in any other action, each toe has its
 * own model */
static void sonic_blend_toes(struct task_work *work, struct task_motion *motion,
                             struct player *record)
{
    if (record->anim.action == ACTION_FULL_SPEED) {
        if (record->anim.frame < 16.0F) {
            record->own.sonic.toe_blend_plus_z =
                (16.0F - record->anim.frame) * 0.0625F;
            model_blend(sonic_toe_plus_z_models[TOE_SHAPE_A],
                        sonic_toe_plus_z_models[TOE_SHAPE_B],
                        sonic_toe_plus_z_models[TOE_BLEND],
                        record->own.sonic.toe_blend_plus_z);
            sonic_toe_plus_z.model = sonic_toe_plus_z_models[TOE_BLEND];
        } else if (record->anim.frame > 24.0F) {
            record->own.sonic.toe_blend_plus_z =
                (record->anim.frame - 24.0F) * 0.125F;
            model_blend(sonic_toe_plus_z_models[TOE_SHAPE_A],
                        sonic_toe_plus_z_models[TOE_SHAPE_B],
                        sonic_toe_plus_z_models[TOE_BLEND],
                        record->own.sonic.toe_blend_plus_z);
            sonic_toe_plus_z.model = sonic_toe_plus_z_models[TOE_BLEND];
        } else {
            record->own.sonic.toe_blend_plus_z = 0.0F;
            sonic_toe_plus_z.model = sonic_toe_plus_z_models[TOE_SHAPE_A];
        }
        if (record->anim.frame >= 8.0F && record->anim.frame <= 16.0F) {
            record->own.sonic.toe_blend_minus_z =
                (record->anim.frame - 8.0F) * 0.125F;
            model_blend(sonic_toe_minus_z_models[TOE_SHAPE_A],
                        sonic_toe_minus_z_models[TOE_SHAPE_B],
                        sonic_toe_minus_z_models[TOE_BLEND],
                        record->own.sonic.toe_blend_minus_z);
            sonic_toe_minus_z.model = sonic_toe_minus_z_models[TOE_BLEND];
        } else if (record->anim.frame > 16.0F && record->anim.frame <= 32.0F) {
            record->own.sonic.toe_blend_minus_z =
                (32.0F - record->anim.frame) * 0.0625F;
            model_blend(sonic_toe_minus_z_models[TOE_SHAPE_A],
                        sonic_toe_minus_z_models[TOE_SHAPE_B],
                        sonic_toe_minus_z_models[TOE_BLEND],
                        record->own.sonic.toe_blend_minus_z);
            sonic_toe_minus_z.model = sonic_toe_minus_z_models[TOE_BLEND];
        } else {
            record->own.sonic.toe_blend_minus_z = 0.0F;
            sonic_toe_minus_z.model = sonic_toe_minus_z_models[TOE_SHAPE_A];
        }
    } else {
        record->own.sonic.toe_blend_plus_z = 0.0F;
        record->own.sonic.toe_blend_minus_z = 0.0F;
        sonic_toe_plus_z.model = sonic_toe_plus_z_models[TOE_MODEL];
        sonic_toe_minus_z.model = sonic_toe_minus_z_models[TOE_MODEL];
    }
}

/* the first step of his mouth's task, which keeps a player_mouth in its
 * motion. without memory for one, it tries again the next step */
static void sonic_mouth_start(struct task *task)
{
    struct player_mouth *mouth;

    if ((mouth = heap_calloc(1, sizeof(*mouth))) != NULL) {
        task->motion = (struct task_motion *)mouth;
        mouth->face = &sonic_mouth;
        mouth->mouth = sonic_head;
        mouth->blend = &sonic_head_blend;
        mouth->shapes = &sonic_mouth_shapes;
        task->destructor = player_mouth_destroy;
        task->step = player_mouth_step;
    }
}

/* put the head's own model back in its place */
static void sonic_sway_destroy(struct task *task)
{
    sonic_head->model = sonic_head_models[HEAD_OWN];
}

/* bob the spines at the back of his head as he rises and falls. sway them
 * in the wind, or with his speed past SWAY_SPEED. the swayed copy of his
 * head's model takes the model's place. each pass turns the points behind
 * its depth along x, by half the turn of the pass before */
static void sonic_sway_step(struct task *task)
{
    struct task_work *work = task->work;
    struct player_sway *sway = PLAYER_SWAY(task);
    struct task_work *owner = player_work[PLAYER_VARS(work)->player];
    struct task_motion *owner_motion = player_motion[PLAYER_VARS(work)->player];
    NJS_POINT3 *to = sonic_head_models[HEAD_SWAY]->points;
    /* @hack spelling: from, a Float cursor through the points, which reads each
     * float with FMOV.S @R14+ (0x8c0b9628); as an NJS_POINT3 pointer, 184 of
     * 459 words differ */
    Float *from = (Float *)sonic_head_models[HEAD_OWN]->points;
    /* @hack type: count, and i below, unsigned, which SHC tests with CMP/HI
     * (0x8c0b961a); as the Sint32 that nbPoint is, the code runs shorter and
     * 181 of 459 words differ */
    Uint32 count = sonic_head_models[HEAD_OWN]->nbPoint;
    /* @hack spelling: the second pass reads the head's own model through
     * own_entry, its entry's address in sonic_head_models, which takes a
     * frame slot (ADD #-52,R15 at 0x8c0b9214); read from sonic_head_models
     * there, the frame is 88 bytes, not 92, and 439 of 459 words differ */
    NJS_MODEL **own_entry = &sonic_head_models[HEAD_OWN];
    struct player *record;
    NJS_VECTOR point;
    NJS_VECTOR wind;
    NJS_VECTOR ahead;
    Float speed;
    /* @hack spelling: the cosines and the sine through wave, which keeps
     * the address of sonic_head_models in R9 (0x8c0b9218); written in their
     * places, it takes R8 and 114 of 459 words differ */
    Float wave;
    Angle tilt;
    Uint32 i;

    switch (work->mode) {
    case SWAY_INIT:
        work->mode = SWAY_SWAY;
        task->destructor = sonic_sway_destroy;
        break;
    case SWAY_SWAY:
        if (owner == NULL) {
            work->mode = SWAY_END;
            return;
        }
        if (owner_motion->speed.y > _builtin_fabsf(sway->bob)) {
            sway->bob = owner_motion->speed.y;
            sway->bob_phase = 0x8000;
        } else if (owner_motion->speed.y < -_builtin_fabsf(sway->bob)) {
            sway->bob = owner_motion->speed.y;
            sway->bob_phase = 0;
        }
        wind.x = wind.y = wind.z = 0.0F;
        wind_get_force(&owner->pos, &wind);
        ahead.x = 1.0F;
        ahead.y = 0.0F;
        ahead.z = 0.0F;
        player_turn_to_world(owner, &ahead);
        ahead.y = 0.0F;
        if (njUnitVector(&ahead) != 0.0F) {
            record = player_get(0);
            sway->sway = njScalor(&wind);
            /* @hack spelling: speed set in its test, which keeps it in the
             * frame; set before the test, it takes FR13, which the entry
             * then saves (FMOV.S FR13,@-R15), and 444 of 459 words differ */
            if ((speed = njScalor(&owner_motion->speed)) > SWAY_SPEED) {
                sway->sway = speed * ((rand() * RAND_SCALE) + 1.0F) * 0.25F;
            }
            njUnitVector(&wind);
            sway->sway_phase =
                vector_angle(&ahead, &wind, &record->contact->normal);
            sway->lean = (Angle)(sway->sway * 6553.6F);
        }
        if (sway->bob >= 0.2F || sway->bob <= -0.2F) {
            sway->bob *= 0.98F;
        } else {
            sway->bob = 0.0F;
        }
        sway->bob_phase = (sway->bob_phase + 0x1000) & 0xfe00;
        wave = njCos(sway->bob_phase);
        work->ang.z = (Angle)(wave * sway->bob * 819.2F);
        if (sway->sway >= 0.2F || sway->sway <= -0.2F) {
            sway->sway *= 0.98F;
        } else {
            sway->sway = 0.0F;
        }
        sway->sway_phase = (sway->sway_phase + 0x1000) & 0xfe00;
        wave = njCos(sway->sway_phase);
        work->ang.y = (Angle)(wave * sway->sway * 1638.4F);
        wave = njSin(sway->lean);
        work->ang.x = (Angle)(wave * sway->sway * 409.6F);
        if (frame_step == 0) {
            break;
        }
        tilt = work->ang.x + work->ang.z;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, work->ang.y);
        TURN(njRotateZ, tilt);
        for (i = count; i > 0; i--) {
            if (*from < SWAY_DEPTH_1) {
                point.x = *from++;
                point.y = *from++ + SWAY_PIVOT;
                point.z = *from++;
                njCalcVector(NULL, &point, to);
                to->y -= SWAY_PIVOT;
            } else {
                from += 3;
            }
            to++;
        }
        to = sonic_head_models[HEAD_SWAY]->points;
        from = (Float *)(*own_entry)->points;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, work->ang.y >> 1);
        TURN(njRotateZ, tilt >> 1);
        for (i = count; i > 0; i--) {
            if (*from < SWAY_DEPTH_2) {
                point.x = *from++;
                point.y = *from++ + SWAY_PIVOT;
                point.z = *from++;
                njCalcVector(NULL, &point, to);
                to->y -= SWAY_PIVOT;
            } else {
                from += 3;
            }
            to++;
        }
        /* @bug memory: the third pass sets neither to nor from, so it
         * starts where the second stopped, past the points. it reads count
         * points past the end of the head's own points, and writes those it
         * turns past the end of the sway model's (0x8c0b9616 on, from in R14
         * and to in R12) */
#ifdef GCC
        /* repairs the read past the head's points and the write past the sway
         * model's that the tag names, by dropping the third pass. in the image
         * each model's count normals follow its points, sonic_head_model's at
         * 0x8c1e3124 and sonic_head_sway_model's at 0x8c1ef480. the third pass
         * turns each of the head's normals whose x is below SWAY_DEPTH_3 into
         * the sway model's at the same index. here the sway model's points
         * take the first two passes alone, as in the image, and its normals
         * stay its own */
        njPopMatrix(2);
#else
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, work->ang.y >> 2);
        TURN(njRotateZ, tilt >> 2);
        for (i = count; i > 0; i--) {
            if (*from < SWAY_DEPTH_3) {
                point.x = *from++;
                point.y = *from++ + SWAY_PIVOT;
                point.z = *from++;
                njCalcVector(NULL, &point, to);
                to->y -= SWAY_PIVOT;
            } else {
                from += 3;
            }
            to++;
        }
        njPopMatrix(3);
#endif
        break;
    default:
        task_destroy(task);
        break;
    }
    sonic_head->model = sonic_head_models[HEAD_SWAY];
}

/* make the look task for the player, which turns nodes: his head and his
 * two eyes. it keeps a player_look_motion in its motion. FALSE when the task
 * can't be made. FALSE too when its motion can't be allocated, and then the
 * task is killed */
static Bool sonic_look_create(Sint8 player, NJS_OBJECT **nodes)
{
    struct task *task;
    struct player_look_motion *look;

    /* @hack spelling: the task tested as it's assigned, with its copy to R14
     * in the BT/S slot (0x8c0b96f6 to 0x8c0b96fa); assigned first, SHC tests
     * the copy in R14 and 3 of 48 words differ */
    /* @hack spelling: the return of FALSE in an else, where an early return
     * would do, which puts it after the success path; as an early return,
     * the code runs shorter and 17 of 48 words differ */
    if ((task = task_create(TASK_WORK, 2, sonic_look_step)) != NULL) {
        if ((look = heap_alloc(sizeof(*look))) == NULL) {
            task_kill(task);
            return FALSE;
        }
        task->motion = (struct task_motion *)look;
        PLAYER_VARS(task->work)->player = player;
        look->look.head = nodes[0];
        look->look.eye0 = nodes[1];
        look->look.eye1 = nodes[2];
    } else {
        return FALSE;
    }
    return TRUE;
}

/* give the look's nodes their own angles back */
static void sonic_look_destroy(struct task *task)
{
    struct player_look_motion *look = PLAYER_LOOK(task);

    look->look.head->ang[0] = look->head_ang[0];
    look->look.head->ang[1] = look->head_ang[1];
    look->look.head->ang[2] = look->head_ang[2];
    look->look.eye1->ang[0] = look->eye1.ang[0];
    look->look.eye1->ang[1] = look->eye1.ang[1];
    look->look.eye1->ang[2] = look->eye1.ang[2];
    look->look.eye0->ang[0] = look->eye0.ang[0];
    look->look.eye0->ang[1] = look->eye0.ang[1];
    look->look.eye0->ang[2] = look->eye0.ang[2];
}

/* turn his head the way he turns, at the camera while Z is held, or at
 * what his event says. the head takes what its limits allow, and the eyes
 * the rest. for player 0, the aura's head turns with his */
static void sonic_look_step(struct task *task)
{
    struct task_work *work = task->work;
    struct player_look_motion *look = PLAYER_LOOK(task);
    struct task_work *owner;
    struct task_motion *owner_motion;
    struct collision_shape *shape;
    struct event_work *event;
    struct task_work *target;
    Angle angles[3];
    NJS_POINT3 direction;
    NJS_VECTOR facing;

    owner = player_work[PLAYER_VARS(work)->player];
    owner_motion = player_motion[PLAYER_VARS(work)->player];
    /* @hack spelling: an empty arm for LOOK_INIT, which leaves a starting look
     * as it is, and whose jump keeps R3 for the event record (0x8c0b97f6);
     * with the tests below nested under a test of LOOK_INIT, R2, and 2 of
     * 406 words differ */
    if (owner == NULL) {
        work->mode = LOOK_END;
    } else if (work->mode == LOOK_INIT) {
    } else if (owner->event->look != 0) {
        work->mode = LOOK_EVENT;
    } else if (pad_records[PLAYER_VARS(work)->player].on & PDD_DGT_TZ) {
        work->mode = LOOK_CAMERA;
    } else {
        work->mode = LOOK_AHEAD;
    }
    switch (work->mode) {
    case LOOK_INIT:
        work->mode = LOOK_AHEAD;
        look->head_ang[0] = look->look.head->ang[0];
        look->head_ang[1] = look->look.head->ang[1];
        look->head_ang[2] = look->look.head->ang[2];
        look->eye0.ang[0] = look->look.eye0->ang[0];
        look->eye0.ang[1] = look->look.eye0->ang[1];
        look->eye0.ang[2] = look->look.eye0->ang[2];
        look->eye1.ang[0] = look->look.eye1->ang[0];
        look->eye1.ang[1] = look->look.eye1->ang[1];
        look->eye1.ang[2] = look->look.eye1->ang[2];
        look->look.head_max_y = 0x2000;
        look->look.head_min_y = -0x2000;
        look->look.head_max_z = 0x400;
        look->look.head_min_z = -0x1800;
        look->look.eye0_max_y = 0x200;
        look->look.eye0_min_y = -0x1b00;
        look->look.eye1_max_y = 0x1b00;
        look->look.eye1_min_y = -0x200;
        look->look.eye0_max_z = 0x800;
        look->look.eye0_min_z = -0xd00;
        look->look.eyes_max_z = 0x800;
        look->look.eyes_min_z = -0xd00;
        /* @hack redundant: an if (0.0F) block, whose store never runs and
         * makes no code, which keeps a frame word at @R15 (MOV.L R3,@R15 at
         * 0x8c0b9b6a); without it, the frame words at @R15 and @(4,R15)
         * trade and 4 of 406 words differ */
        if (0.0F) {
            look->look.eye0_max_z =
                (((look->look.eye0_max_z + 0x800) ^ 0x800) + 0x800) ^ 0x800;
        }
        task->destructor = sonic_look_destroy;
        break;
    case LOOK_AHEAD:
        look->look.head_aim_y =
            -angle_difference(owner->ang.y, owner_motion->aim.y);
        look->look.head_aim_z = 0;
        player_look_turn_head(&look->look);
        look->look.eyes_aim_y = 0;
        look->look.eyes_aim_z = 0;
        player_look_turn_eyes(&look->look);
        break;
    case LOOK_CAMERA:
        target = camera_work;
        shape = owner->collision->shapes;
        /* @hack spelling: the chained stores of zero, which give target and
         * shape their frame slots (MOV #16,R0 at 0x8c0b99a2); stored one by
         * one, 6 of 406 words differ */
        facing.y = facing.z = 0.0F;
        facing.x = 1.0F;
        player_turn_to_world(owner, &facing);
        direction = target->pos;
        njSubVector(&direction, &shape->center);
        vector_get_angles(&direction, &facing, angles);
        look->look.head_aim_y = -angles[1];
        look->look.head_aim_z = angles[2];
        player_look_turn_head(&look->look);
        look->look.eyes_aim_y = angles[1] + look->look.head_y;
        look->look.eyes_aim_z = angles[2] - look->look.head_z;
        player_look_turn_eyes(&look->look);
        break;
    case LOOK_EVENT:
        event = owner->event;
        shape = owner->collision->shapes;
        switch (event->look) {
        case EVENT_LOOK_ANGLES:
            angles[1] = event->look_ang.y;
            angles[2] = event->look_ang.z;
            break;
        case EVENT_LOOK_POINT:
            facing.y = facing.z = 0.0F;
            facing.x = 1.0F;
            player_turn_to_world(owner, &facing);
            direction = event->look_pos;
            njSubVector(&direction, &shape->center);
            vector_get_angles(&direction, &facing, angles);
            break;
        case EVENT_LOOK_TASK:
#ifdef GCC
            /* repairs angles[1] and [2], which the shipped code leaves
             * unset with no task, or a task with no work. the BRA at
             * 0x8c0b9a8e skips to the aims, which read them at 0x8c0b9b6c
             * and 0x8c0b9b76. 0, as the default case sets them, looks
             * ahead */
            angles[1] = 0;
            angles[2] = 0;
#endif
            /* @hack spelling: two tests, each with its own store and break;
             * joined, the frame grows from 76 bytes to 80 (ADD #-44,R15 at
             * 0x8c0b97ce) and 196 of 406 words differ */
            if (event->look_task == NULL) {
                event->look = 0;
                break;
            }
            target = event->look_task->work;
            if (target == NULL) {
                event->look = 0;
                break;
            }
            facing.y = facing.z = 0.0F;
            facing.x = 1.0F;
            player_turn_to_world(owner, &facing);
            njPushMatrix(&_unit_matrix_);
            TURN(njRotateZ, target->ang.z);
            TURN(njRotateX, target->ang.x);
            TURN(njRotateY, target->ang.y);
            njCalcVector(NULL, &event->look_pos, &direction);
            njPopMatrix(1);
            njAddVector(&direction, &target->pos);
            njSubVector(&direction, &shape->center);
            vector_get_angles(&direction, &facing, angles);
            break;
        default:
            angles[1] = 0;
            angles[2] = 0;
            break;
        }
        look->look.head_aim_y = -angles[1];
        look->look.head_aim_z = angles[2];
        player_look_turn_head(&look->look);
        look->look.eyes_aim_y = angles[1] + look->look.head_y;
        look->look.eyes_aim_z = angles[2] - look->look.head_z;
        player_look_turn_eyes(&look->look);
        break;
    default:
        task_kill(task);
        break;
    }
    if (PLAYER_VARS(work)->player == 0) {
        sonic_aura_head.ang[1] = look->look.head->ang[1];
        sonic_aura_head.ang[2] = look->look.head->ang[2];
    }
}

/* make the tasks of his effects, each with his player number in its vars:
 * the dash, the spin balls, and super Sonic's of each */
static void sonic_dash_create(const struct task_work *work)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 6, dash_start)) != NULL) {
        PLAYER_VARS(task->work)->player = PLAYER_VARS(work)->player;
    }
}

static void sonic_super_dash_create(const struct task_work *work)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 6, super_dash_start)) != NULL) {
        PLAYER_VARS(task->work)->player = PLAYER_VARS(work)->player;
    }
}

static void sonic_ball_jitter_create(const struct task_work *work)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 5, ball_jitter_start)) != NULL) {
        PLAYER_VARS(task->work)->player = PLAYER_VARS(work)->player;
    }
}

static void sonic_ball_follow_create(const struct task_work *work)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 5, ball_follow_start)) != NULL) {
        PLAYER_VARS(task->work)->player = PLAYER_VARS(work)->player;
    }
}

static void sonic_super_ball_follow_create(const struct task_work *work)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 5, super_ball_follow_start)) != NULL) {
        PLAYER_VARS(task->work)->player = PLAYER_VARS(work)->player;
    }
}

static void sonic_ball_shrink_create(const struct task_work *work)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 5, ball_shrink_start)) != NULL) {
        PLAYER_VARS(task->work)->player = PLAYER_VARS(work)->player;
    }
}

static void sonic_super_ball_shrink_create(const struct task_work *work)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 5, super_ball_shrink_start)) != NULL) {
        PLAYER_VARS(task->work)->player = PLAYER_VARS(work)->player;
    }
}

/* the charge lasts until player 0 is gone, his light dash is no longer
 * charged, or the action button is let go */
static void sonic_charged_step(struct task *task)
{
    struct task_work *work = CHARGED(task)->work;

    /* @hack spelling: the release's test nested in an if, with the kill for the
     * rest in its else, which puts the test first (0x8c0b9d26); as an early
     * return, the kill comes first and 14 of 24 words differ */
    if (player_work[0] != NULL && (work->flags & SONIC_CHARGED)) {
        if (pad_records[0].release & pad_action_buttons) {
            task_kill(task);
        }
    } else {
        task_kill(task);
    }
}

/* give him his top speed back as the charge ends, unless player 0 is
 * gone */
static void sonic_charged_destroy(struct task *task)
{
    struct player *record = CHARGED(task)->record;

    if (player_work[0] != NULL) {
        record->consts.top_speed = CHARGED(task)->top_speed;
    }
}

/* make the task that holds his top speed to CHARGED_TOP_SPEED while his
 * light dash is charged */
static void sonic_charged_create(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record)
{
    struct task *task;

    task = task_create(TASK_EXTRA, 0, sonic_charged_step);
    if (task != NULL) {
        CHARGED(task)->work = work;
        CHARGED(task)->record = record;
        CHARGED(task)->top_speed = record->consts.top_speed;
        record->consts.top_speed = CHARGED_TOP_SPEED;
        task->destructor = sonic_charged_destroy;
    }
}

/* the board takes its model, and the texlist the play time picks. an even
 * play time picks the first of sonic_board_texlists, and an odd one
 * s_sbmot.PRS's sb_board1_texlist */
void sonic_board_start(struct task *task)
{
    struct task_work *work = task->work;
    int index;

    BOARD_VARS(work)->object = &sonic_board_object;
    index = (Uint32)play_time_get() % (sonic_board_texlist_count + 1);
    if (index >= sonic_board_texlist_count) {
        BOARD_VARS(work)->texlist = &sb_board1_texlist;
    } else {
        BOARD_VARS(work)->texlist = &sonic_board_texlists[index];
    }
    task->motion->gravity = 0.02F;
    njLoadTexture(BOARD_VARS(work)->texlist);
    task->destructor = sonic_board_destroy;
    task->step = sonic_board_step;
    task->draw = sonic_board_draw;
}

/* in his board actions, the board stays under him, turned as he is. in his
 * other actions it falls and lands as a small carried thing does, by
 * carry_step_small, unless its BOARD_STILL flag keeps it where it is */
static void sonic_board_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *rider = player_work[BOARD_VARS(work)->player];
    struct task_motion *rider_motion = player_motion[BOARD_VARS(work)->player];
    struct task_motion *motion = task->motion;
    struct player_anim *anim = &(player_get(BOARD_VARS(work)->player))->anim;
    Sint16 action = anim->current;

    if (action < ACTION_BOARD || action > ACTION_BOARD_CROUCH) {
        if (!(work->flags & BOARD_STILL)) {
            work->pos.y += BOARD_DEPTH;
            carry_step_small(task);
            work->pos.y -= BOARD_DEPTH;
        }
    } else {
        work->pos = rider->collision->shapes->center;
        work->ang.x = rider->ang.x;
        work->ang.y = 0x8000 - rider->ang.y;
        work->ang.z = rider->ang.z;
        motion->speed = rider_motion->speed;
        work->flags &= ~BOARD_STILL;
    }
    sonic_board_draw(task);
}

/* draw the board where it lies, or under him, posed at his action's frame
 * by sonic_board_actions' action for his */
static void sonic_board_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct player_anim *anim = &(player_get(BOARD_VARS(work)->player))->anim;
    Sint16 action = anim->current;
    NJS_ACTION *board_action;
    NJS_MOTION *motion;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 12.0F)) {
        return;
    }
    njSetTexture(BOARD_VARS(work)->texlist);
    njPushMatrix(NULL);
    if (action < ACTION_BOARD || action > ACTION_BOARD_CROUCH) {
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        njTranslate(NULL, 0.0F, BOARD_DEPTH, 0.0F);
        njEasyDrawModel(BOARD_VARS(work)->object->model);
    } else {
        /* @hack spelling: ACTION_BOARD taken off action in a statement of its
         * own, which keeps action in R12 (MOV R0,R12 at 0x8c0b9fa8); taken
         * off in the index, action goes to the frame and 26 of 174 words
         * differ */
        action -= ACTION_BOARD;
        board_action = sonic_board_actions[action];
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        if (board_action != NULL) {
            motion = board_action->motion;
            if (work->flags & BOARD_LOWER) {
                njTranslate(NULL, 0.0F, BOARD_DEPTH, 0.0F);
            }
            njEasyDrawMotion(BOARD_VARS(work)->object, motion, anim->frame);
        } else {
            if (work->flags & BOARD_LOWER) {
                njTranslate(NULL, 0.0F, BOARD_DEPTH, 0.0F);
            }
            njEasyDrawObject(BOARD_VARS(work)->object);
        }
    }
    njPopMatrix(1);
}

static void sonic_board_destroy(struct task *task)
{
    struct task_work *work = task->work;

    njReleaseTexture(BOARD_VARS(work)->texlist);
}

/* end the super form: his air_accel goes back as sonic_super_start saved
 * it, and his shape's center_y to SONIC_CENTER_Y. the saved resist.x and
 * air_resist land in top_speed and ground_accel instead */
static void sonic_super_destroy(struct task *task)
{
    struct sonic_super *super = SUPER(task);
    struct player *record = player_get(super->player);

    if (record == NULL) {
        return;
    }
    /* @bug logic: the saved resist.x and air_resist go back into
     * top_speed and ground_accel (FMOV.S at 0x8c0ba198, 0x8c0ba1a0), not
     * where sonic_super_start read them (0x8c0ba246, 0x8c0ba250). his
     * resist.x and air_resist stay as super Sonic's */
    record->consts.top_speed = super->resist_x;
    record->consts.ground_accel = super->air_resist;
    record->consts.air_accel = super->air_accel;
    record->consts.center_y = SONIC_CENTER_Y;
}

/* the super form lasts until his record is gone or he's no longer
 * super */
static void sonic_super_step(struct task *task)
{
    struct player *record = player_get(SUPER(task)->player);

    /* @hack spelling: two tests, each with its own tail call to task_destroy;
     * joined, they share one, the code runs shorter and 23 of 27 words
     * differ */
    if (record == NULL) {
        task_destroy(task);
    } else if (!(record->upgrades & PLAYER_SONIC_SUPER)) {
        task_destroy(task);
    }
}

/* the first step of the super form: he hardly slows, steers more in the
 * air, and his shape's center rises to SUPER_CENTER_Y. with no record, the
 * task ends at once */
static void sonic_super_start(struct task *task)
{
    struct sonic_super *super = SUPER(task);
    struct player *record = player_get(super->player);

    if (record == NULL) {
        task_destroy(task);
        return;
    }
    super->resist_x = record->consts.resist.x;
    super->air_resist = record->consts.air_resist;
    super->air_accel = record->consts.air_accel;
    record->consts.resist.x = -0.001F;
    record->consts.air_resist = -0.002F;
    record->consts.air_accel = 0.05F;
    record->consts.center_y = SUPER_CENTER_Y;
    task->destructor = sonic_super_destroy;
    task->step = sonic_super_step;
}
