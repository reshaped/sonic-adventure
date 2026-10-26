/* Knuckles as a player: his modes, and the hunt for the pieces of the master
 * emerald */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "animal/kinds.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/helpers.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "event/event.h"
#include "event/play.h"
#include "hud/hud.h"
#include "main.h"
#include "object/common.h"
#include "object/item_box.h"
#include "object/item_box_air.h"
#include "object/panel.h"
#include "object/piece.h"
#include "object/ring.h"
#include "object/savepoint.h"
#include "player/effects.h"
#include "player/knuckles.h"
#include "player/knuckles_effects.h"
#include "player/player.h"
#include "player/target.h"
#include "player/textures.h"
#include "progress/demo.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

#define SE_GLIDE_START 748
#define SE_GLIDE 737
#define SE_PUNCH 738
#define SE_PUNCH_LAST 740
#define SE_PUNCH_LAST_VOICE 1155
#define SE_CLIMB 746
#define SE_DIG_START 1150
#define SE_DIG 742
#define SE_DUG_NOTHING 1152
#define SE_DUG_RINGS 1153
/* a dig that finds a fountain of rings, an item box or an animal */
#define SE_DUG_ITEM 1154
/* his maximum heat, the attack his fighting gloves charge: as it charges,
 * once it's full, and as the charge ends */
#define SE_CHARGE 749
#define SE_CHARGED 21
#define SE_HEAT 750
#define SE_HOMING 741
#define SE_CRASH 1145
#define SE_THROW 1148
#define SE_PUSH_START 1149
#define SE_RADAR 745
#define SE_PIECE 744
/* his voice as he takes his first piece. the next two ids are his voice for
 * the second and the third */
#define VOICE_PIECE 1158

/* what a dig turns up, by its index in each table of chances */
enum dig_find {
    DIG_NOTHING,
    DIG_RING,
    DIG_FIVE_RINGS,
    DIG_TEN_RINGS,
    DIG_FOUNTAIN,
    DIG_ITEM_BOX,
    DIG_ANIMAL,
    DIG_FINDS
};

/* ends the list of stages where a dig can turn up an animal */
#define DIG_STAGES_END 127

/* how near player 0 is to a piece, which picks its sprite on the radar */
enum radar_near {
    RADAR_FAR, /* farther than 400, or its place not found yet */
    RADAR_WITHIN_400,
    RADAR_WITHIN_300,
    RADAR_WITHIN_100,
    RADAR_WITHIN_50,
    RADAR_WITHIN_30,
    RADAR_NONE /* no sprite at all */
};

/* an action in k_mot.PRS, his motion file, by its address in MOTION_AREA,
 * where the file loads */
#define K_MOT(address) ((NJS_ACTION *)(address))

/* the rows of knuckles_actions that his code sets or tests, or that a row
 * names as its next */
enum {
    ACTION_STAND = 0,
    ACTION_LAND = 1, /* landing at a stop */
    ACTION_STAND_LANDED = 2, /* standing, after ACTION_LAND or a panel */
    ACTION_STAND_ONCE = 3, /* the stand's motion once, then ACTION_STAND */
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
    ACTION_SPIN = 14, /* curled up, paced by record->action_speed */
    ACTION_ROLL = 15, /* curled up on the ground, paced by his speed */
    ACTION_BALL = 16, /* the ball he flickers to as he's drawn curled up */
    ACTION_SPRING = 17,
    ACTION_FALL_START = 18,
    ACTION_FALL = 19,
    ACTION_HARD_LANDING = 20,
    ACTION_SKID = 21,
    ACTION_TURN = 22, /* from a skid, back the way the stick points */
    ACTION_PUSH_START = 23,
    ACTION_PUSH = 24,
    ACTION_HURT = 25,
    ACTION_HURT_LAND = 26,
    ACTION_DOWN = 27,
    ACTION_DOWN_LAND = 28,
    ACTION_SKID_LOOP = 29, /* the skid's motion, round and round */
    ACTION_UPDRAFT = 30,
    ACTION_CRASH = 31,
    ACTION_CRASH_LAND = 32,
    ACTION_UNCURL = 33,
    /* curling up in the air: out of a fall or a knock back, or as his glide
     * ends */
    ACTION_CURL = 34,
    ACTION_HOMING = 35,
    ACTION_RIDE = 36,
    ACTION_ICED = 37,
    ACTION_DROWN = 38,
    ACTION_CLEAR = 39, /* the stage's clear pose */
    /* a pose played once, which his step keeps out of collisions as it does
     * ACTION_CLEAR. its motion follows ACTION_CLEAR's in k_mot.PRS: by both
     * signs, a second clear pose */
    ACTION_CLEAR_ONCE = 40,
    ACTION_DIG_START = 41,
    ACTION_DIG = 42,
    ACTION_DIG_UP = 43,
    ACTION_DIG_OUT = 44,
    ACTION_DIG_FAIL = 45,
    ACTION_CLIMB = 46, /* up or down the wall */
    ACTION_CLIMB_MINUS_Z = 47, /* across the wall, toward his -z or his +z */
    ACTION_CLIMB_PLUS_Z = 48,
    ACTION_CLING = 49, /* still on the wall */
    ACTION_CLIMB_OUT = 50,
    ACTION_GLIDE = 51,
    ACTION_GRAB_WALL = 52,
    ACTION_GLIDE_LAND = 53,
    /* his punches, in knuckles_punches' order: two runs of five, from
     * ACTION_PUNCH_1 and from ACTION_PUNCH_B1. each row names the row after
     * it as its next, but the last of each run names ACTION_STAND. he throws
     * only ACTION_PUNCH_1 to ACTION_PUNCH_3, each after the one before.
     * nothing starts the second run, B, whose rows pose
     * knuckles_stand_action, as ACTION_PUNCH_4's and ACTION_PUNCH_5's do */
    ACTION_PUNCH_1 = 54,
    ACTION_PUNCH_2 = 55,
    ACTION_PUNCH_3 = 56,
    ACTION_PUNCH_4 = 57,
    ACTION_PUNCH_5 = 58,
    ACTION_PUNCH_B1 = 59,
    ACTION_PUNCH_B2 = 60,
    ACTION_PUNCH_B3 = 61,
    ACTION_PUNCH_B4 = 62,
    ACTION_PUNCH_B5 = 63,
    /* the last of the rows from ACTION_PUNCH_1 whose shapes
     * knuckles_set_shapes leaves to knuckles_punch */
    ACTION_PUNCH_ROWS_LAST = 69,
    ACTION_LIFT = 70,
    ACTION_HOLD_RUN = 71,
    ACTION_HOLD_STOP = 72,
    ACTION_HOLD_TURN = 73,
    ACTION_HOLD_STAND = 74,
    ACTION_HOLD_WALK = 75,
    ACTION_BOUNCE_1 = 76,
    ACTION_BOUNCE_2 = 78,
    ACTION_SQUASH_2 = 81,
    ACTION_SQUASH_3 = 82,
    ACTION_HANG_ON = 84,
    ACTION_PULL = 85,
    ACTION_SHAKE = 86,
    ACTION_HOLD_SHAKE_START = 87,
    ACTION_HOLD_SHAKE = 88,
    ACTION_HOLD_SHAKE_END = 89,
    ACTION_GRIP_START = 90,
    ACTION_GRIP = 91,
    ACTION_HOLD_JUMP = 92,
    ACTION_HOLD_FALL_START = 93,
    ACTION_HOLD_FALL = 94,
    ACTION_HOLD_HARD_LANDING = 95,
    ACTION_THROW_AT = 96, /* at the nearest target */
    ACTION_THROW = 97, /* ahead */
    ACTION_AIR_THROW_AT = 98,
    ACTION_AIR_THROW = 99,
    ACTION_PUT_DOWN = 100,
    /* along the path he hangs from, at the frame knuckles_hang_move sets in
     * a cycle of HANG_FRAMES. stopped, he holds the pose of its frame 32 or
     * of its frame 0 */
    ACTION_HANG_GO = 101,
    ACTION_HANG_AT_32 = 102,
    ACTION_HANG_AT_0 = 103,
    ACTION_LEDGE = 104, /* at the frame knuckles_ledge_move sets */
    ACTION_PANEL_JUMP = 105,
    ACTION_PANEL_WAIT = 106,
    /* gripping a rocket as it takes off, then flying with it, along or up */
    ACTION_ROCKET_START = 107,
    ACTION_ROCKET = 108,
    ACTION_ROCKET_ALONG = 109,
    ACTION_ROCKET_UP = 110
};

/* the words of record->own.knuckles that his code names. one word holds
 * his lean as he glides, an Angle, and as he climbs, the steps his grip
 * lasts with no wall near */
#define LEAN(record) ((record)->own.knuckles.w[3])
#define GRIP_STEPS(record) ((record)->own.knuckles.w[3])
/* counts his draws while he's curled up, which flicker him to ACTION_BALL */
#define FLICKER(record) ((record)->own.knuckles.w[4])
/* his maximum heat's charge: the steps he has held the action buttons, up
 * to KNUCKLES_CHARGED. a punch sets it to 0, which lets it charge, and it
 * is -1 while it can't */
#define CHARGE(record) ((record)->own.knuckles.w[5])
/* the struggle left before he breaks out of the ice, from ICE_STRUGGLE. a
 * step takes ICE_PRESS from it with a press of PAD_MASH_BUTTONS, and 1
 * without */
#define STRUGGLE(record) ((record)->own.knuckles.w[6])

/* the charge that fills his maximum heat */
#define KNUCKLES_CHARGED 120
/* the steps he stands still before he fidgets */
#define FIDGET_STEPS 1800
/* the struggle he starts with in the ice, and what a press takes from it */
#define ICE_STRUGGLE 300
#define ICE_PRESS 20
/* the steps of his homing, and the steps it runs before it can end because
 * he has stopped */
#define HOMING_STEPS 180
#define HOMING_MIN_STEPS 30
/* how near a target his homing takes, and how near it sets off a burst.
 * near_tasks lists no target past 100, so HOMING_REACH rules none out */
#define HOMING_REACH 400.0F
#define HOMING_BURST_REACH 15.0F
/* the steps he glides on, straight, once his turn back has brought him back
 * over the act's death zones */
#define GLIDE_TURN_STEPS 30
/* the bumps each wall his glide runs into adds. in the adventure fields,
 * the bumps lose 1 a step, and GLIDE_BUMPS_MAX of them end his glide */
#define GLIDE_BUMP 20
#define GLIDE_BUMPS_MAX 60
/* the steps his grip on a wall lasts with no wall near */
#define GRIP_HOLD_STEPS 6
/* how deep his dig goes, along the ground's normal */
#define DIG_DEPTH 20.0F
/* a dig's chances rise near the points of a grid DIG_GRID apart. near is
 * within DIG_NEAR of one, and on is within DIG_ON */
#define DIG_GRID 100.0F
#define DIG_NEAR 20.0F
#define DIG_ON 4.0F
/* within DIG_AGAIN of where he last dug out, a dig's chances scale down, to
 * none within DIG_SPENT */
#define DIG_AGAIN 15.0F
#define DIG_SPENT 8.0F
/* the frames of ACTION_HANG_GO's cycle and of ACTION_LEDGE's. hanging from a
 * path or a ledge, he moves round the cycle HANG_PACE frames a step */
#define HANG_FRAMES 64.0F
#define LEDGE_FRAMES 32.0F
#define HANG_PACE 2.0F

/* his task's work->vars. player_vars' player and character come first, and
 * depth shares the bytes of player_vars' arg */
struct knuckles_vars {
    Uint8 unk_00[4];
    /* the steps his glide or his dig has run. in his turn back, it holds
     * GLIDE_TURN_STEPS while he turns, and counts down from there once he
     * is back over the act's death zones */
    Sint16 steps;
    /* the steps his homing has left, from HOMING_STEPS */
    Sint16 homing_steps;
    /* how far down his dig has gone along the ground's normal: from 0 past
     * -DIG_DEPTH, and back */
    Float depth;
};

#define KNUCKLES_VARS(work) ((struct knuckles_vars *)&(work)->vars)

/* work->vars of the task that shows his maximum heat as it charges */
struct heat_vars {
    Uint8 player;
    Uint8 unk_01[7];
    Sint16 *charge; /* CHARGE in its player's record */
};

#define HEAT_VARS(work) ((struct heat_vars *)&(work)->vars)

/* work->vars of the radar. pulse[i] swells piece i's sprite: its scale is
 * 0.5 plus 0.005 times pulse[i] */
struct radar_vars {
    Uint8 unk_00[8];
    Sint8 pulse[3];
};

#define RADAR_VARS(work) ((struct radar_vars *)&(work)->vars)

/* the head models his sway reads and writes */
struct knuckles_sway_models {
    NJS_MODEL *swayed; /* the copy it writes, which both heads draw */
    /* the head's model, read while his action poses knuckles_object */
    NJS_MODEL *own;
    /* the b model's head's model, read while his action poses
     * knuckles_b_object */
    NJS_MODEL *b_own;
};

/* a punch: where it strikes from, and the frames of its action on which
 * its parts happen */
struct punch {
    /* the weld point it strikes from, counted from weld_point0 */
    Uint16 point;
    Sint16 wave; /* the way its wave sets out, an enum wave */
    /* the frames his fist flashes on, or none for a start below 0 */
    Float flash_start;
    Float flash_end;
    /* the frames its shape strikes on. it sounds on the first */
    Float hit_start;
    Float hit_end;
    Float wave_frame; /* the frame its wave sets out on */
};

/* a piece of the master emerald */
struct piece {
    Sint32 id; /* the id of its place, as knuckles.h gives it */
    Bool taken;
    Bool found; /* knuckles_find_piece has found it, at pos */
    Bool marked; /* knuckles_mark_piece has set its mark */
    NJS_POINT3 pos;
    NJS_POINT3 mark; /* where he restarts once he takes it */
};

/* an act with pieces. a table of them ends at PIECE_ACTS_END */
struct piece_act {
    /* how many places each group has, in the order of knuckles.h's
     * PIECE_FIRST, PIECE_BURIED, PIECE_SECOND and PIECE_THIRD */
    Sint8 places[4];
    Uint16 stage_act;
};

#define PIECE_ACTS_END 0xffff

/* player 0's work->timer once he takes the third piece. the timer holds
 * there, so no hit hurts him while he finishes the hunt */
#define KNUCKLES_TIMER_HELD 0x7ff0

/* a taken piece's icon: waiting, flying to the top of the screen, and
 * blinking there */
enum icon_mode { ICON_WAIT, ICON_FLY, ICON_BLINK };

/* the radar's sprites along the bottom of the screen: piece 0's at RADAR_X
 * and RADAR_Y, and each next piece's RADAR_GAP to the right */
#define RADAR_X 285.0F
#define RADAR_Y 418.0F
#define RADAR_GAP 35.0F
/* a taken piece's sprite flies from the radar toward ICON_ENTRY_X and
 * ICON_Y, off the right of the screen, for ICON_FLIGHT_STEPS. its icon
 * waits as long, then flies in to its place along the top. the first
 * piece's place is at ICON_FIRST_X, and each next one's ICON_GAP to the
 * left */
#define ICON_ENTRY_X 688.0F
#define ICON_Y 70.0F
#define ICON_FLIGHT_STEPS 17
#define ICON_FIRST_X 588.0F
#define ICON_GAP 70.0F
/* the progress a taken piece's sprite makes a step, one ulp above
 * 1.0F / (ICON_FLIGHT_STEPS + 1). at progress p, the sprite has covered p
 * of its way in x and p * p in y */
#define ICON_FLIGHT_SHARE 0.05555556F
/* the ang.x at which an icon stops blinking: three turns of the cosine its
 * alpha follows */
#define ICON_BLINK_END 0x30000
/* the steps from the third piece's taking to the act's clear */
#define FINISH_STEPS 30
/* the steps the time shows, blinking, as he takes a piece */
#define TIME_STEPS 180

/* the way a punch's wave sets out from his fist, as he faces */
enum wave { WAVE_NONE, WAVE_AHEAD, WAVE_BACK, WAVE_UP, WAVE_DOWN };

/* what his glide runs into, as knuckles_glide_collide reports it */
enum glide_hit {
    GLIDE_CLEAR,
    GLIDE_BOUNCE, /* a surface he turns or is pushed off */
    GLIDE_CLIMB, /* a wall he can climb */
    GLIDE_WATER
};

/* his two models. his sprint, spring, glide and climb actions, the start of
 * his fall and his hard landing pose knuckles_b_object */
extern NJS_OBJECT knuckles_object;
extern NJS_OBJECT knuckles_b_object;
/* the head and eyes his look task turns */
extern NJS_OBJECT knuckles_head_joint;
extern NJS_OBJECT knuckles_eye0;
extern NJS_OBJECT knuckles_eye1;
/* the objects knuckles_mouth_start hands his mouth task. knuckles_head and
 * knuckles_b_head are his heads on his two models, whose models his sway
 * swaps */
extern NJS_OBJECT knuckles_mouth;
/* @hack type: the heads as arrays, here and knuckles_b_head below, whose
 * model SHC stores through the head's address at a displacement (MOV.L
 * R2,@(4,R3) at 0x8c0cfb60). as plain objects, the model's address is a
 * pool word of its own, and summed over knuckles_sway_destroy,
 * knuckles_sway_step and knuckles_mouth_start, 13 of 373 words differ */
extern NJS_OBJECT knuckles_head[];
extern NJS_MODEL knuckles_head_model;
extern NJS_OBJECT knuckles_b_head[];
extern NJS_MODEL knuckles_b_head_model;
extern NJS_OBJECT knuckles_head_blend; /* the head the mouth blends to */
extern NJS_MOTION knuckles_mouth_shapes; /* the shapes it moves through */
extern NJS_MODEL knuckles_head_sway_model; /* the copy his sway writes */
/* the parts his welds join and mark */
extern NJS_OBJECT knuckles_upper_arm_minus_z;
extern NJS_OBJECT knuckles_forearm_minus_z;
/* @hack type: the four parts of each hand his upgrades change, on both
 * models, as arrays, here and below, whose models SHC stores as it does the
 * heads'. as plain objects, knuckles_set_hands saves other registers (MOV.L
 * R12,@-R15 at 0x8c0cd234), and 172 of its 227 words differ */
extern NJS_OBJECT knuckles_cuff_minus_z[];
extern NJS_OBJECT knuckles_hand_minus_z[];
extern NJS_OBJECT knuckles_fingers_minus_z[];
extern NJS_OBJECT knuckles_thumb_minus_z[];
extern NJS_OBJECT knuckles_upper_arm_plus_z;
extern NJS_OBJECT knuckles_forearm_plus_z;
extern NJS_OBJECT knuckles_cuff_plus_z[];
extern NJS_OBJECT knuckles_hand_plus_z[];
extern NJS_OBJECT knuckles_fingers_plus_z[];
extern NJS_OBJECT knuckles_thumb_plus_z[];
extern NJS_OBJECT knuckles_thigh_minus_z;
extern NJS_OBJECT knuckles_shin_minus_z;
extern NJS_OBJECT knuckles_foot_minus_z;
extern NJS_OBJECT knuckles_shoe_minus_z;
extern NJS_OBJECT knuckles_toes_minus_z;
extern NJS_OBJECT knuckles_thigh_plus_z;
extern NJS_OBJECT knuckles_shin_plus_z;
extern NJS_OBJECT knuckles_foot_plus_z;
extern NJS_OBJECT knuckles_shoe_plus_z;
extern NJS_OBJECT knuckles_toes_plus_z;
extern NJS_OBJECT knuckles_b_upper_arm_minus_z;
extern NJS_OBJECT knuckles_b_forearm_minus_z;
extern NJS_OBJECT knuckles_b_cuff_minus_z[];
extern NJS_OBJECT knuckles_b_hand_minus_z[];
extern NJS_OBJECT knuckles_b_fingers_minus_z[];
extern NJS_OBJECT knuckles_b_thumb_minus_z[];
extern NJS_OBJECT knuckles_b_upper_arm_plus_z;
extern NJS_OBJECT knuckles_b_forearm_plus_z;
extern NJS_OBJECT knuckles_b_cuff_plus_z[];
extern NJS_OBJECT knuckles_b_hand_plus_z[];
extern NJS_OBJECT knuckles_b_fingers_plus_z[];
extern NJS_OBJECT knuckles_b_thumb_plus_z[];
extern NJS_OBJECT knuckles_b_thigh_minus_z;
extern NJS_OBJECT knuckles_b_shin_minus_z;
extern NJS_OBJECT knuckles_b_foot_minus_z;
extern NJS_OBJECT knuckles_b_shoe_minus_z;
extern NJS_OBJECT knuckles_b_toes_minus_z;
extern NJS_OBJECT knuckles_b_thigh_plus_z;
extern NJS_OBJECT knuckles_b_shin_plus_z;
extern NJS_OBJECT knuckles_b_foot_plus_z;
extern NJS_OBJECT knuckles_b_shoe_plus_z;
extern NJS_OBJECT knuckles_b_toes_plus_z;
/* the hand parts' models, by upgrade, which both his models share */
/* @hack type: the models as arrays, whose addresses knuckles_set_hands
 * spills two of to its frame (MOV.L R0,@R15 at 0x8c0cd244). as plain
 * objects, it keeps no frame, and 224 of its 227 words differ */
extern NJS_MODEL knuckles_cuff_minus_z_model[];
extern NJS_MODEL knuckles_hand_minus_z_model[];
extern NJS_MODEL knuckles_fingers_minus_z_model[];
extern NJS_MODEL knuckles_thumb_minus_z_model[];
extern NJS_MODEL knuckles_cuff_plus_z_model[];
extern NJS_MODEL knuckles_hand_plus_z_model[];
extern NJS_MODEL knuckles_fingers_plus_z_model[];
extern NJS_MODEL knuckles_thumb_plus_z_model[];
extern NJS_MODEL knuckles_claw_fingers_minus_z_model[];
extern NJS_MODEL knuckles_claw_fingers_plus_z_model[];
extern NJS_MODEL knuckles_glove_cuff_minus_z_model[];
extern NJS_MODEL knuckles_glove_hand_minus_z_model[];
extern NJS_MODEL knuckles_glove_fingers_minus_z_model[];
extern NJS_MODEL knuckles_glove_thumb_minus_z_model[];
extern NJS_MODEL knuckles_glove_cuff_plus_z_model[];
extern NJS_MODEL knuckles_glove_hand_plus_z_model[];
extern NJS_MODEL knuckles_glove_fingers_plus_z_model[];
extern NJS_MODEL knuckles_glove_thumb_plus_z_model[];
extern NJS_MODEL knuckles_glove_claw_fingers_minus_z_model[];
extern NJS_MODEL knuckles_glove_claw_fingers_plus_z_model[];

extern NJS_ACTION knuckles_stand_action;
extern NJS_ACTION knuckles_fidget_start_action;
extern NJS_ACTION knuckles_fidget_action;
extern NJS_ACTION knuckles_stand_uphill_action;
extern NJS_ACTION knuckles_stand_downhill_action;
extern NJS_ACTION knuckles_walk_action;
extern NJS_ACTION knuckles_run_action;
extern NJS_ACTION knuckles_sprint_action;
extern NJS_ACTION knuckles_spin_action;
extern NJS_ACTION knuckles_ball_action;
extern NJS_ACTION knuckles_spring_action;
extern NJS_ACTION knuckles_fall_start_action;
extern NJS_ACTION knuckles_fall_action;
extern NJS_ACTION knuckles_hard_landing_action;
extern NJS_ACTION knuckles_skid_action;
extern NJS_ACTION knuckles_turn_action;
extern NJS_ACTION knuckles_push_action;
extern NJS_ACTION knuckles_hurt_action;
extern NJS_ACTION knuckles_hurt_land_action;
extern NJS_ACTION knuckles_down_action;
extern NJS_ACTION knuckles_down_land_action;
extern NJS_ACTION knuckles_updraft_action;
extern NJS_ACTION knuckles_crash_action;
extern NJS_ACTION knuckles_crash_land_action;
extern NJS_ACTION knuckles_uncurl_action;
extern NJS_ACTION knuckles_glide_action;
extern NJS_ACTION knuckles_climb_action;
extern NJS_ACTION knuckles_climb_minus_z_action;
extern NJS_ACTION knuckles_climb_plus_z_action;
extern NJS_ACTION knuckles_cling_action;
extern NJS_ACTION knuckles_climb_out_action;
extern NJS_ACTION knuckles_grab_wall_action;
extern NJS_ACTION knuckles_glide_land_action;
extern NJS_ACTION knuckles_punch1_action;
extern NJS_ACTION knuckles_punch2_action;
extern NJS_ACTION knuckles_punch3_action;
/* the end of his third punch, as an action of its own */
extern NJS_ACTION knuckles_punch3_end_action;
extern NJS_ACTION knuckles_hang_on_action;
extern NJS_ACTION knuckles_shake_action;
extern NJS_ACTION knuckles_grip_action;
extern NJS_ACTION knuckles_grip_hold_action;
extern NJS_ACTION knuckles_throw_at_target_action;
extern NJS_ACTION knuckles_throw_action;

/* while knuckles_dig_forced is TRUE, a dig finds nothing and plays
 * knuckles_dig_forced_sound, a KNUCKLES_DIG_SOUND_ value.
 * knuckles_force_dig sets both, and his step clears the flag as it ends */
static Bool knuckles_dig_forced;
static int knuckles_dig_forced_sound;
/* the chance of each find for a dig where he is, as his step sets them */
static Float knuckles_dig_chances[DIG_FINDS];
static NJS_POINT3 knuckles_dig_last_pos; /* where he last dug out */
/* the matrices his sway turns his head's points by: its whole tilt, half
 * of it, and a quarter */
static NJS_MATRIX knuckles_sway_tilt;
static NJS_MATRIX knuckles_sway_half;
static NJS_MATRIX knuckles_sway_quarter;
/* the hunt: whether it's on, how many pieces he has taken, and the three */
static Bool knuckles_pieces_picked;
static int knuckles_pieces_taken;
static struct piece knuckles_pieces[3];
/* where player 0 is put as the hunt ends: the last piece's restart point,
 * on the ground below it */
static NJS_POINT3 knuckles_finish_pos;
/* the line the debug text draws from a piece to player 0 */
static NJS_POINT3 knuckles_piece_line[2];

/* each mode's name in the image, in enum knuckles_mode's order. no code
 * reads them, but they open the object's data */
static char knuckles_mode_names[][8] = {
    "MD_INIT", "MD_STND", "MD_WALK", "MD_1DWK", "MD_ROLL", "MD_1DSP", "MD_JUMP",
    "MD_SPRG", "MD_LUNC", "MD_FALL", "MD_STOP", "MD_GLID", "MD_GTRN", "MD_PUSH",
    "MD_DAMG", "MD_BACK", "MD_PLON", "MD_CRSH", "MD_HMG0", "MD_DIGD", "MD_DIGU",
    "MD_DIGT", "MD_DGX0", "MD_DGX1", "MD_CLIM", "MD_CLIU", "MD_PUNC", "MD_PICK",
    "MD_PUT_", "MD_HOLD", "MD_TOCH", "MD_HWLK", "MD_THRW", "MD_HSTP", "MD_HFLL",
    "MD_HJMP", "MD_HSPR", "MD_HLNC", "MD_PULL", "MD_THRU", "MD_SHAK", "MD_ESHK",
    "MD_HGWT", "MD_HGGO", "MD_LDWT", "MD_LDGO", "MD_PNJP", "MD_PNWT", "MD_ICED",
    "EVENT",   "MD_DOZA", "MD_KILL", "MD_S3A2", "MD_EDIT",
};

/* his actions, which record->anim.action indexes. knuckles_try_punch and
 * knuckles_punch_next rewrite the first three punches' nexts */
static struct player_action knuckles_actions[] = {
    {&knuckles_stand_action, 73, PLAYER_ANIM_LOOP, ACTION_STAND, 0.0625F, 1.0F},
    {&knuckles_stand_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND_LANDED, 0.25F,
     2.0F},
    {&knuckles_stand_action, 73, PLAYER_ANIM_LOOP, ACTION_STAND_LANDED, 0.25F,
     1.0F},
    {&knuckles_stand_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F, 1.0F},
    {&knuckles_fidget_start_action, 73, PLAYER_ANIM_ONCE, ACTION_FIDGET, 0.25F,
     0.25F},
    {&knuckles_fidget_action, 73, PLAYER_ANIM_ONCE, ACTION_FIDGET_END, 0.25F,
     0.25F},
    {&knuckles_fidget_start_action, 73, PLAYER_ANIM_BACK, ACTION_STAND, 0.25F,
     0.25F},
    {&knuckles_stand_uphill_action, 73, PLAYER_ANIM_LOOP, ACTION_STAND_UPHILL,
     0.25F, 1.0F},
    {&knuckles_stand_downhill_action, 73, PLAYER_ANIM_LOOP,
     ACTION_STAND_DOWNHILL, 0.25F, 1.0F},
    {&knuckles_walk_action, 73, PLAYER_ANIM_LOOP_SPEED, ACTION_WALK, 0.25F,
     3.4F},
    {&knuckles_run_action, 73, PLAYER_ANIM_LOOP_SPEED, ACTION_TROT, 0.25F,
     1.4F},
    {&knuckles_run_action, 73, PLAYER_ANIM_LOOP_SPEED, ACTION_RUN, 0.25F, 1.0F},
    {&knuckles_sprint_action, 73, PLAYER_ANIM_LOOP_SPEED, ACTION_SPRINT, 0.25F,
     1.0F},
    {&knuckles_sprint_action, 73, PLAYER_ANIM_LOOP_SPEED, ACTION_FULL_SPEED,
     0.25F, 1.0F},
    {&knuckles_spin_action, 74, PLAYER_ANIM_LOOP_PACE, ACTION_SPIN, 0.25F,
     0.2F},
    {&knuckles_spin_action, 74, PLAYER_ANIM_LOOP_SPEED, ACTION_ROLL, 0.25F,
     0.7F},
    {&knuckles_ball_action, 1, PLAYER_ANIM_LOOP, ACTION_BALL, 0.5F, 1.0F},
    {&knuckles_spring_action, 73, PLAYER_ANIM_LOOP, ACTION_SPRING, 0.25F, 1.0F},
    {&knuckles_fall_start_action, 73, PLAYER_ANIM_ONCE, ACTION_FALL, 0.125F,
     1.0F},
    {&knuckles_fall_action, 73, PLAYER_ANIM_LOOP, ACTION_FALL, 0.25F, 1.0F},
    {&knuckles_hard_landing_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F,
     1.0F},
    {&knuckles_skid_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F, 1.0F},
    {&knuckles_turn_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F, 1.0F},
    {&knuckles_push_action, 73, PLAYER_ANIM_ONCE, ACTION_PUSH, 0.125F, 3.0F},
    {&knuckles_stand_action, 73, PLAYER_ANIM_LOOP, ACTION_PUSH, 0.25F, 3.0F},
    {&knuckles_hurt_action, 73, PLAYER_ANIM_LOOP, ACTION_HURT, 0.25F, 1.0F},
    {&knuckles_hurt_land_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F,
     1.0F},
    {&knuckles_down_action, 73, PLAYER_ANIM_ONCE, ACTION_DOWN_LAND, 0.25F,
     1.0F},
    {&knuckles_down_land_action, 73, PLAYER_ANIM_LOOP, ACTION_DOWN_LAND, 0.25F,
     1.0F},
    {&knuckles_skid_action, 73, PLAYER_ANIM_LOOP, ACTION_SKID_LOOP, 0.25F,
     1.0F},
    {&knuckles_updraft_action, 73, PLAYER_ANIM_LOOP, ACTION_UPDRAFT, 0.0625F,
     1.0F},
    {&knuckles_crash_action, 73, PLAYER_ANIM_ONCE, ACTION_CRASH_LAND, 0.25F,
     1.0F},
    {&knuckles_crash_land_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.0625F,
     1.0F},
    {&knuckles_uncurl_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.0625F,
     1.0F},
    {&knuckles_spin_action, 74, PLAYER_ANIM_ONCE, ACTION_SPIN, 0.25F, 0.15F},
    {&knuckles_glide_action, 73, PLAYER_ANIM_LOOP, ACTION_HOMING, 0.5F, 1.5F},
    {MODULE_ACTION(0x0c909958), 73, PLAYER_ANIM_LOOP, ACTION_RIDE, 0.25F,
     0.15F},
    {&knuckles_hurt_action, 73, PLAYER_ANIM_HOLD, ACTION_ICED, 1.0F, 15.0F},
    {K_MOT(0x0cc0ca5c), 73, PLAYER_ANIM_HOLD, ACTION_DROWN, 1.0F, 0.5F},
    {K_MOT(0x0cc436d8), 73, PLAYER_ANIM_HOLD, ACTION_CLEAR, 0.5F, 1.0F},
    {K_MOT(0x0cc4523c), 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.5F, 1.0F},
    {K_MOT(0x0cc023dc), 73, PLAYER_ANIM_ONCE, ACTION_DIG, 0.25F, 0.7F},
    {K_MOT(0x0cc04570), 73, PLAYER_ANIM_LOOP, ACTION_DIG, 0.25F, 2.0F},
    {K_MOT(0x0cc053d4), 73, PLAYER_ANIM_LOOP, ACTION_DIG_UP, 0.25F, 1.0F},
    {K_MOT(0x0cc06df8), 73, PLAYER_ANIM_ONCE, ACTION_FALL, 0.125F, 1.0F},
    {K_MOT(0x0cc023dc), 73, PLAYER_ANIM_DRIVEN, ACTION_STAND, 0.25F, 0.7F},
    {&knuckles_climb_action, 73, PLAYER_ANIM_LOOP_SPEED, ACTION_CLIMB, 0.25F,
     1.6F},
    {&knuckles_climb_minus_z_action, 73, PLAYER_ANIM_LOOP_PACE,
     ACTION_CLIMB_MINUS_Z, 0.25F, 1.6F},
    {&knuckles_climb_plus_z_action, 73, PLAYER_ANIM_LOOP_PACE,
     ACTION_CLIMB_PLUS_Z, 0.25F, 1.6F},
    {&knuckles_cling_action, 73, PLAYER_ANIM_LOOP, ACTION_CLING, 0.25F, 1.0F},
    {&knuckles_climb_out_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.125F,
     1.0F},
    {&knuckles_glide_action, 73, PLAYER_ANIM_LOOP, ACTION_GLIDE, 0.25F, 1.0F},
    {&knuckles_grab_wall_action, 73, PLAYER_ANIM_ONCE, ACTION_CLIMB, 0.25F,
     1.0F},
    {&knuckles_glide_land_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F,
     1.0F},
    {&knuckles_punch1_action, 73, PLAYER_ANIM_ONCE, ACTION_PUNCH_2, 1.0F, 2.0F},
    {&knuckles_punch2_action, 73, PLAYER_ANIM_ONCE, ACTION_PUNCH_3, 1.0F, 2.0F},
    {&knuckles_punch3_action, 73, PLAYER_ANIM_ONCE, ACTION_PUNCH_4, 0.25F,
     1.0F},
    {&knuckles_stand_action, 73, PLAYER_ANIM_ONCE, ACTION_PUNCH_5, 0.25F, 1.0F},
    {&knuckles_stand_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F, 1.0F},
    {&knuckles_stand_action, 73, PLAYER_ANIM_ONCE, ACTION_PUNCH_B2, 1.0F, 1.0F},
    {&knuckles_stand_action, 73, PLAYER_ANIM_ONCE, ACTION_PUNCH_B3, 1.0F, 1.0F},
    {&knuckles_stand_action, 73, PLAYER_ANIM_ONCE, ACTION_PUNCH_B4, 0.25F,
     1.0F},
    {&knuckles_stand_action, 73, PLAYER_ANIM_ONCE, ACTION_PUNCH_B5, 0.25F,
     1.0F},
    {&knuckles_stand_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F, 1.5F},
    {&knuckles_punch3_end_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.0625F,
     1.0F},
    {&knuckles_stand_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.0625F, 1.0F},
    {&knuckles_stand_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.0625F, 1.0F},
    {&knuckles_stand_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.0625F, 1.0F},
    {&knuckles_stand_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.0625F, 1.0F},
    {&knuckles_stand_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.0625F, 1.0F},
    {K_MOT(0x0cc2ae4c), 73, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND, 0.5F, 1.0F},
    {K_MOT(0x0cc1b194), 73, PLAYER_ANIM_LOOP, ACTION_HOLD_RUN, 0.5F, 1.0F},
    {K_MOT(0x0cc1e908), 73, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND, 0.5F, 1.0F},
    {K_MOT(0x0cc2008c), 73, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND, 0.5F, 1.0F},
    {K_MOT(0x0cc21ef0), 73, PLAYER_ANIM_LOOP, ACTION_HOLD_STAND, 0.5F, 0.5F},
    {K_MOT(0x0cc24244), 73, PLAYER_ANIM_LOOP, ACTION_HOLD_WALK, 0.5F, 1.0F},
    {&knuckles_spring_action, 73, PLAYER_ANIM_LOOP, ACTION_SPRING, 0.25F, 1.0F},
    {&knuckles_fall_start_action, 73, PLAYER_ANIM_ONCE, ACTION_FALL, 0.5F,
     1.0F},
    {&knuckles_spring_action, 73, PLAYER_ANIM_LOOP, ACTION_SPRING, 0.25F, 1.0F},
    {&knuckles_fall_start_action, 73, PLAYER_ANIM_ONCE, ACTION_FALL, 0.5F,
     1.0F},
    {&knuckles_fall_action, 73, PLAYER_ANIM_LOOP, ACTION_SPRING, 0.25F, 1.0F},
    {&knuckles_spring_action, 73, PLAYER_ANIM_ONCE, ACTION_SPRING, 0.5F, 1.0F},
    {&knuckles_hard_landing_action, 73, PLAYER_ANIM_ONCE, ACTION_SPRING, 0.5F,
     1.0F},
    {&knuckles_stand_action, 73, PLAYER_ANIM_ONCE, ACTION_WALK, 0.5F, 0.5F},
    {&knuckles_hang_on_action, 73, PLAYER_ANIM_LOOP, ACTION_HANG_ON, 0.5F,
     0.5F},
    {K_MOT(0x0cc4bca8), 73, PLAYER_ANIM_LOOP, ACTION_PULL, 0.5F, 1.0F},
    {&knuckles_shake_action, 73, PLAYER_ANIM_LOOP, ACTION_SHAKE, 0.5F, 0.5F},
    {K_MOT(0x0cc0e560), 73, PLAYER_ANIM_ONCE, ACTION_HOLD_SHAKE, 0.5F, 0.5F},
    {K_MOT(0x0cc10284), 73, PLAYER_ANIM_LOOP, ACTION_HOLD_SHAKE, 0.5F, 0.5F},
    {K_MOT(0x0cc11f18), 73, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND, 0.5F, 0.5F},
    {&knuckles_grip_action, 73, PLAYER_ANIM_ONCE, ACTION_GRIP, 0.5F, 1.0F},
    {&knuckles_grip_hold_action, 73, PLAYER_ANIM_LOOP, ACTION_GRIP, 0.5F, 1.0F},
    {K_MOT(0x0cc12d5c), 73, PLAYER_ANIM_LOOP, ACTION_HOLD_JUMP, 0.5F, 1.0F},
    {K_MOT(0x0cc13e00), 73, PLAYER_ANIM_ONCE, ACTION_HOLD_FALL, 0.5F, 1.0F},
    {K_MOT(0x0cc15124), 73, PLAYER_ANIM_LOOP, ACTION_HOLD_FALL, 0.5F, 1.0F},
    {K_MOT(0x0cc18ae0), 73, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND, 0.5F, 1.0F},
    {&knuckles_throw_at_target_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.5F,
     1.0F},
    {&knuckles_throw_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.5F, 1.0F},
    {&knuckles_throw_at_target_action, 73, PLAYER_ANIM_ONCE, ACTION_FALL, 0.5F,
     1.0F},
    {&knuckles_throw_action, 73, PLAYER_ANIM_ONCE, ACTION_FALL, 0.5F, 1.0F},
    {K_MOT(0x0cc2ae4c), 73, PLAYER_ANIM_BACK, ACTION_STAND, 1.0F, 1.0F},
    {K_MOT(0x0cc5861c), 73, PLAYER_ANIM_DRIVEN, ACTION_HANG_GO, 1.0F, 0.3F},
    {K_MOT(0x0cc4f380), 73, PLAYER_ANIM_LOOP, ACTION_HANG_AT_32, 0.25F, 0.25F},
    {K_MOT(0x0cc4d874), 73, PLAYER_ANIM_LOOP, ACTION_HANG_AT_0, 0.25F, 0.25F},
    {K_MOT(0x0cc27b28), 73, PLAYER_ANIM_DRIVEN, ACTION_LEDGE, 1.0F, 0.5F},
    {&knuckles_spring_action, 73, PLAYER_ANIM_LOOP, ACTION_PANEL_JUMP, 0.25F,
     1.0F},
    {&knuckles_stand_action, 73, PLAYER_ANIM_LOOP, ACTION_PANEL_WAIT, 0.25F,
     1.0F},
    {&knuckles_grip_action, 73, PLAYER_ANIM_ONCE, ACTION_ROCKET, 0.125F, 1.0F},
    {&knuckles_grip_hold_action, 73, PLAYER_ANIM_LOOP, ACTION_ROCKET, 0.125F,
     1.0F},
    {K_MOT(0x0cc489a4), 73, PLAYER_ANIM_LOOP, ACTION_ROCKET_ALONG, 0.125F,
     1.0F},
    {K_MOT(0x0cc46d80), 73, PLAYER_ANIM_LOOP, ACTION_ROCKET_UP, 0.125F, 1.0F},
};

/* his body's shape, his punches' fist, and two shapes knuckles_set_shapes
 * sets as he glides, ahead of him and behind. only the one ahead strikes:
 * the one behind, at attack 0, pushes enemies and hurts nothing */
/* clang-format off */
static struct collision_shape knuckles_shapes[4] = {
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS,
     PUSH_ALL | PUSH_FIRM(PUSH_OTHERS), {0.0F, 1.0F, 0.0F}, 4.0F, 4.0F, 0.65F,
     {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_OFF | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(3) | GUARD(2) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES |
     SHAPE_HURTS_OBJECTS,
     PUSH_ENEMIES | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 8.0F, 0.0F, 0.0F,
     {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_OFF | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(1) | GUARD(2) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES |
     SHAPE_HURTS_OBJECTS,
     PUSH_ENEMIES | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 4.5F, 0.0F, 0.0F,
     {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_OFF | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS,
     PUSH_ENEMIES | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 2.75F, 0.0F, 0.0F,
     {0}, 0, NULL},
};
/* clang-format on */

static NJS_OBJECT *knuckles_look_nodes[3] = {&knuckles_head_joint,
                                             &knuckles_eye0, &knuckles_eye1};

static struct knuckles_sway_models knuckles_sway_models = {
    &knuckles_head_sway_model, &knuckles_head_model, &knuckles_b_head_model};

/* the vertices each seam's welds join, in pairs: a vertex of object_a,
 * then its partner in object_b. the last two tables are his fingers', as
 * they are and with his shovel claws on */
static Uint16 knuckles_elbow_vertices[8] = {
#include "player/knuckles/knuckles_elbow_vertices.inc"
};
static Uint16 knuckles_wrist_vertices[8] = {
#include "player/knuckles/knuckles_wrist_vertices.inc"
};
static Uint16 knuckles_leg_vertices[8] = {
#include "player/knuckles/knuckles_leg_vertices.inc"
};
static Uint16 knuckles_toe_vertices[12] = {
#include "player/knuckles/knuckles_toe_vertices.inc"
};
static Uint16 knuckles_finger_vertices[10] = {
#include "player/knuckles/knuckles_finger_vertices.inc"
};
static Uint16 knuckles_claw_vertices[10] = {
#include "player/knuckles/knuckles_claw_vertices.inc"
};

/* his welds. the seams of both his models come first. the marks after them
 * keep his hands, feet and mouth in the record's weld_point0 to
 * weld_point4 */
static struct weld knuckles_welds[] = {
    {&knuckles_object, &knuckles_upper_arm_minus_z, &knuckles_forearm_minus_z,
     4, WELD_TO_A, 0, 0, NULL, knuckles_elbow_vertices},
    {&knuckles_object, &knuckles_forearm_minus_z, knuckles_cuff_minus_z, 4,
     WELD_TO_A, 0, 0, NULL, knuckles_wrist_vertices},
    {&knuckles_object, &knuckles_upper_arm_plus_z, &knuckles_forearm_plus_z, 4,
     WELD_TO_A, 0, 0, NULL, knuckles_elbow_vertices},
    {&knuckles_object, &knuckles_forearm_plus_z, knuckles_cuff_plus_z, 4,
     WELD_TO_A, 0, 0, NULL, knuckles_wrist_vertices},
    {&knuckles_object, &knuckles_thigh_minus_z, &knuckles_shin_minus_z, 4,
     WELD_TO_A, 0, 0, NULL, knuckles_leg_vertices},
    {&knuckles_object, &knuckles_shin_minus_z, &knuckles_foot_minus_z, 4,
     WELD_TO_A, 0, 0, NULL, knuckles_leg_vertices},
    {&knuckles_object, &knuckles_thigh_plus_z, &knuckles_shin_plus_z, 4,
     WELD_TO_A, 0, 0, NULL, knuckles_leg_vertices},
    {&knuckles_object, &knuckles_shin_plus_z, &knuckles_foot_plus_z, 4,
     WELD_TO_A, 0, 0, NULL, knuckles_leg_vertices},
    {&knuckles_object, &knuckles_shoe_minus_z, &knuckles_toes_minus_z, 6,
     WELD_TO_A, 0, 0, NULL, knuckles_toe_vertices},
    {&knuckles_object, &knuckles_shoe_plus_z, &knuckles_toes_plus_z, 6,
     WELD_TO_A, 0, 0, NULL, knuckles_toe_vertices},
    {&knuckles_object, knuckles_hand_minus_z, knuckles_fingers_minus_z, 5,
     WELD_TO_A, 0, 0, NULL, knuckles_finger_vertices},
    {&knuckles_object, knuckles_hand_plus_z, knuckles_fingers_plus_z, 5,
     WELD_TO_A, 0, 0, NULL, knuckles_finger_vertices},
    {&knuckles_b_object, &knuckles_b_upper_arm_minus_z,
     &knuckles_b_forearm_minus_z, 4, WELD_TO_A, 0, 0, NULL,
     knuckles_elbow_vertices},
    {&knuckles_b_object, &knuckles_b_forearm_minus_z, knuckles_b_cuff_minus_z,
     4, WELD_TO_A, 0, 0, NULL, knuckles_wrist_vertices},
    {&knuckles_b_object, &knuckles_b_upper_arm_plus_z,
     &knuckles_b_forearm_plus_z, 4, WELD_TO_A, 0, 0, NULL,
     knuckles_elbow_vertices},
    {&knuckles_b_object, &knuckles_b_forearm_plus_z, knuckles_b_cuff_plus_z, 4,
     WELD_TO_A, 0, 0, NULL, knuckles_wrist_vertices},
    {&knuckles_b_object, &knuckles_b_thigh_minus_z, &knuckles_b_shin_minus_z, 4,
     WELD_TO_A, 0, 0, NULL, knuckles_leg_vertices},
    {&knuckles_b_object, &knuckles_b_shin_minus_z, &knuckles_b_foot_minus_z, 4,
     WELD_TO_A, 0, 0, NULL, knuckles_leg_vertices},
    {&knuckles_b_object, &knuckles_b_thigh_plus_z, &knuckles_b_shin_plus_z, 4,
     WELD_TO_A, 0, 0, NULL, knuckles_leg_vertices},
    {&knuckles_b_object, &knuckles_b_shin_plus_z, &knuckles_b_foot_plus_z, 4,
     WELD_TO_A, 0, 0, NULL, knuckles_leg_vertices},
    {&knuckles_b_object, &knuckles_b_shoe_minus_z, &knuckles_b_toes_minus_z, 6,
     WELD_TO_A, 0, 0, NULL, knuckles_toe_vertices},
    {&knuckles_b_object, &knuckles_b_shoe_plus_z, &knuckles_b_toes_plus_z, 6,
     WELD_TO_A, 0, 0, NULL, knuckles_toe_vertices},
    {&knuckles_b_object, knuckles_b_hand_minus_z, knuckles_b_fingers_minus_z, 5,
     WELD_TO_A, 0, 0, NULL, knuckles_finger_vertices},
    {&knuckles_b_object, knuckles_b_hand_plus_z, knuckles_b_fingers_plus_z, 5,
     WELD_TO_A, 0, 0, NULL, knuckles_finger_vertices},
    {&knuckles_object, knuckles_fingers_minus_z, NULL, 2, WELD_MARK, 0, 0, NULL,
     NULL},
    {&knuckles_object, knuckles_fingers_plus_z, NULL, 2, WELD_MARK + 1, 0, 0,
     NULL, NULL},
    {&knuckles_object, &knuckles_toes_minus_z, NULL, 0, WELD_MARK + 2, 0, 0,
     NULL, NULL},
    {&knuckles_object, &knuckles_toes_plus_z, NULL, 0, WELD_MARK + 3, 0, 0,
     NULL, NULL},
    {&knuckles_object, &knuckles_mouth, NULL, 0, WELD_MARK + 4, 0, 0, NULL,
     NULL},
    {NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL},
};

/* the debug text's line from each piece to player 0, in its color */
static NJS_COLOR knuckles_piece_line_colors[6] = {
#include "player/knuckles/knuckles_piece_line_colors.inc"
};
static NJS_POINT3COL knuckles_piece_lines[3] = {
#include "player/knuckles/knuckles_piece_lines.inc"
};

/* where the time shows as he takes a piece */
static struct hud_layout knuckles_time_layout = {
    {450.0F, 400.0F}, {1.0F, 1.0F}, {30.0F, 0.0F}};

/* the chance of each find for a dig anywhere, near a point of the grid,
 * and on one */
static const Float knuckles_dig_chances_anywhere[DIG_FINDS] = {
    0.0F, 0.2F, 0.12F, 0.08F, 0.03F, 0.05F, 0.02F};
static const Float knuckles_dig_chances_near[DIG_FINDS] = {
    0.0F, 0.7F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
static const Float knuckles_dig_chances_on[DIG_FINDS] = {
    0.0F, 0.15F, 0.04F, 0.01F, 0.0F, 0.0F, 0.3F};

/* each punch's frames, from ACTION_PUNCH_1 on */
static const struct punch knuckles_punches[10] = {
    {0, WAVE_AHEAD, -19.0F, 20.0F, 2.0F, 20.0F, 8.0F},
    {1, WAVE_AHEAD, -16.0F, 17.0F, 2.0F, 18.0F, 8.0F},
    {0, WAVE_NONE, 18.0F, 22.0F, 2.0F, 49.0F, 16.0F},
    {1, WAVE_NONE, 20.0F, 24.0F, 16.0F, 28.0F, 20.0F},
    {1, WAVE_NONE, 17.0F, 23.0F, 16.0F, 38.0F, 25.0F},
    {0, WAVE_UP, -4.0F, 8.0F, 2.0F, 16.0F, 8.0F},
    {1, WAVE_UP, -2.0F, 8.0F, 2.0F, 16.0F, 4.0F},
    {0, WAVE_NONE, 10.0F, 18.0F, 2.0F, 24.0F, 16.0F},
    {3, WAVE_NONE, 10.0F, 16.0F, 8.0F, 20.0F, 18.0F},
    {1, WAVE_NONE, 8.0F, 14.0F, 4.0F, 18.0F, 10.0F},
};

static void knuckles_choose_mode(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record);
static void knuckles_charge_heat(struct task_work *work, struct player *record);
static void knuckles_destroy(struct task *task);
static void knuckles_draw(struct task *task);
static void knuckles_set_hands(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static void knuckles_stand(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static void knuckles_set_stand_action(struct task_work *work,
                                      struct task_motion *motion,
                                      struct player *record);
static void knuckles_set_walk_action(struct task_work *work,
                                     struct task_motion *motion,
                                     struct player *record);
static void knuckles_hang_move(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static void knuckles_ledge_move(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record);
static void knuckles_edit(struct task_work *work, struct task_motion *motion,
                          struct player *record);
static void knuckles_glide_start(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record);
static Bool knuckles_try_hurt(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static Bool knuckles_try_jump(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static Bool knuckles_try_uncurl(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record);
static Bool knuckles_try_wall_jump(struct task_work *work,
                                   struct player *record);
static Bool knuckles_follow_event(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record);
static Bool knuckles_try_request(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record);
static Bool knuckles_try_request_holding(struct task_work *work,
                                         struct task_motion *motion,
                                         struct player *record);
static Bool knuckles_try_grab(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static Bool knuckles_try_take_lifted(struct task_work *work,
                                     struct task_motion *motion,
                                     struct player *record);
static Bool knuckles_try_throw(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static Bool knuckles_try_release(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record);
static Bool knuckles_try_grip(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static void knuckles_let_go(struct task_work *work, struct task_motion *motion,
                            struct player *record);
static void knuckles_carry(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static Bool knuckles_try_fall(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static Bool knuckles_try_stop(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static Bool knuckles_try_roll(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static Bool knuckles_try_punch(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static Bool knuckles_try_unroll(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record);
static void knuckles_sway_destroy(struct task *task);
static void knuckles_sway_step(struct task *task);
static void knuckles_heat_destroy(struct task *task);
static void knuckles_heat_step(struct task *task);
static void knuckles_heat_start(struct task *task);
static void knuckles_punch_next(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record);
static void knuckles_punch(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static void knuckles_glide_bounce(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record, NJS_VECTOR *normal);
static enum glide_hit knuckles_glide_collide(struct task_work *work,
                                             struct task_motion *motion,
                                             struct player *record);
static Bool knuckles_wall_near(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static void knuckles_set_climb_action(struct task_work *work,
                                      struct player *record);
static void knuckles_set_shapes(struct task_work *work, struct player *record);
static Bool knuckles_try_dig(struct task_work *work, struct task_motion *motion,
                             struct player *record);
static void knuckles_sound_step(struct task *task);
static void knuckles_dig_find(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static void knuckles_set_dig_chances(struct task_work *work);
static Bool knuckles_outside_zones(struct task_work *work);
static Bool knuckles_look_create(Sint8 player, NJS_OBJECT **nodes);
static void knuckles_look_destroy(struct task *task);
static void knuckles_look_step(struct task *task);
static void knuckles_mouth_start(struct task *task);
static struct task_work *knuckles_home(struct task_work *work);
static void knuckles_start_climb(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record);
static void knuckles_icon_step(struct task *task);
static void knuckles_icons_step(struct task *task);
static void knuckles_radar_draw(struct task_work *work, int piece,
                                enum radar_near near);
static void knuckles_taken_icon_step(struct task *task);
static void knuckles_finish_step(struct task *task);
static void knuckles_radar_step(struct task *task);
static void knuckles_radar_destroy(struct task *task);
static void knuckles_radar_start(struct task *task);
static Bool knuckles_buried_piece_near(NJS_POINT3 *pos);
static void knuckles_time_step(struct task *task);

/* the helpers that take his motion take his work and record with it, as
 * player.h's steps do, whether they use all three or not */

void knuckles_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;

    if ((work->flags & COLLISION_ON_TOP) && motion->speed.y <= 0.0F) {
        work->flags |= PLAYER_ON_TASK;
        /* @bug logic: coming down onto a task, he copies his own tilt, not the
         * task's. his collision's owner is his own task, which collision_init
         * stores (MOV.L R3,@(16,R14) at 0x8c030154). these stores, through it,
         * copy his angles back unchanged (MOV.L @(16,R1),R0 at 0x8c0c85bc and
         * 0x8c0c85ca) */
        work->ang.x = work->collision->owner->work->ang.x;
        work->ang.z = work->collision->owner->work->ang.z;
    } else {
        work->flags &= ~PLAYER_ON_TASK;
    }
    knuckles_charge_heat(work, record);
    if (edit_mode == 1) {
        work->mode = KNUCKLES_EDIT;
    } else if (work->mode != KNUCKLES_DOWN) {
        knuckles_choose_mode(work, motion, record);
    }
    switch (work->mode) {
    case KNUCKLES_INIT: {
        struct task *child;

        PLAYER_VARS(work)->character = CHARACTER_KNUCKLES;
        player_init(PLAYER_VARS(work)->player, task);
        record = motion->record;
        record->anim.action = ACTION_STAND;
        record->anim.actions = knuckles_actions;
        collision_init(task, knuckles_shapes, COLLISION_PLAYER, 1, 4);
        work->collision->shapes->radius = record->consts.radius;
        work->mode = KNUCKLES_STAND;
        texture_load_character(CHARACTER_KNUCKLES);
        record->anim.welds = knuckles_welds;
        CHARGE(record) = -1;
        knuckles_look_create(PLAYER_VARS(work)->player, knuckles_look_nodes);
        child = task_create(TASK_WORK, 3, knuckles_mouth_start);
        if (child != NULL) {
            PLAYER_VARS(child->work)->player = PLAYER_VARS(work)->player;
        }
        child = task_create(TASK_MOTION | TASK_WORK, 3, knuckles_sway_step);
        if (child != NULL) {
            PLAYER_VARS(child->work)->player = PLAYER_VARS(work)->player;
        }
        task->destructor = knuckles_destroy;
        task->draw = knuckles_draw;
        if (event_in_control() != TRUE &&
            (game_mode == GAME_MODE_ACTION || game_mode == GAME_MODE_TRIAL)) {
            task_create(TASK_WORK, 3, knuckles_radar_start);
        }
        child = task_create(TASK_WORK, 3, knuckles_heat_start);
        if (child != NULL) {
            HEAT_VARS(child->work)->player = PLAYER_VARS(work)->player;
            HEAT_VARS(child->work)->charge = &CHARGE(record);
        }
        break;
    }
    case KNUCKLES_STAND:
        player_turn_to_stick(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        knuckles_set_stand_action(work, motion, record);
        break;
    case KNUCKLES_WALK:
        if (record->anim.previous == ACTION_TURN &&
            record->anim.current != ACTION_TURN) {
            record->speed.x += record->consts.trot_speed;
        }
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        if (player_collide(work, motion, record) == PLAYER_CRASHED) {
            work->mode = KNUCKLES_CRASH;
            record->anim.action = ACTION_CRASH;
            player_crash_burst(work);
            record->speed.x = -record->speed.x;
            record->speed.z = -record->speed.z;
        } else {
            player_take_speed(work, motion, record);
            knuckles_set_walk_action(work, motion, record);
        }
        break;
    case KNUCKLES_PATH:
        player_accel_path(work, motion, record);
        player_apply_path_accel(work, motion, record);
        player_move_on_path(work, motion, record);
        knuckles_set_walk_action(work, motion, record);
        break;
    case KNUCKLES_ROLL:
        player_turn_to_stick(work, motion, record);
        player_accel_roll(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_PATH_SPIN:
        player_accel_roll(work, motion, record);
        player_apply_path_accel(work, motion, record);
        player_move_on_path(work, motion, record);
        break;
    case KNUCKLES_JUMP:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_SPRING:
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_LAUNCH:
        if (record->speed.y >= record->consts.launch_speed) {
            player_accel_launched(work, motion, record);
        } else {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_FALL:
    case KNUCKLES_DIG_OUT:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_STOP:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        if (record->speed.x <= 0.0F &&
            player_stick_back_or_end_event(work, motion)) {
            work->mode = KNUCKLES_WALK;
            record->anim.action = ACTION_TURN;
            player_turn_to_stick(work, motion, record);
            work->ang.y = motion->aim.y;
        }
        break;
    case KNUCKLES_GLIDE:
        player_turn_upright(work, motion, record);
        if (motion->speed.y > 0.0F) {
            player_accel_air(work, motion, record);
            player_apply_accel(work, motion, record);
        } else {
            player_accel_glide(work, motion, record);
            player_apply_glide_accel(work, motion, record);
        }
        switch (knuckles_glide_collide(work, motion, record)) {
        case GLIDE_BOUNCE:
            player_take_speed(work, motion, record);
            vibration_start(PLAYER_VARS(work)->player, 3, 15, 0);
            record->own.knuckles.glide.bumps += GLIDE_BUMP;
            break;
        case GLIDE_CLIMB:
            knuckles_start_climb(work, motion, record);
            vibration_start(PLAYER_VARS(work)->player, 3, 15, 0);
            break;
        case GLIDE_WATER:
            work->flags &= ~PLAYER_ATTACKING;
            work->mode = KNUCKLES_STOP;
            record->anim.action = ACTION_GLIDE_LAND;
            break;
        default:
            player_collide(work, motion, record);
            player_take_speed(work, motion, record);
            break;
        }
        break;
    case KNUCKLES_GLIDE_TURN:
        player_turn_upright(work, motion, record);
        if (KNUCKLES_VARS(work)->steps == GLIDE_TURN_STEPS) {
            work->ang.y += 0x400;
        }
        if (motion->speed.y > 0.0F) {
            player_apply_accel(work, motion, record);
        } else {
            player_apply_glide_accel(work, motion, record);
        }
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_PUSH:
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
    case KNUCKLES_HURT:
        player_turn_upright(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_BACK:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_POSE:
        break;
    case KNUCKLES_CRASH:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_HOMING: {
        struct task_work *target;
        NJS_VECTOR speed;

        work->flags |= PLAYER_GUARD;
        target = knuckles_home(work);
        if (target == NULL) {
            speed.x = speed.y = speed.z = 0.0F;
            record->speed = speed;
            player_turn_to_world(work, &speed);
            motion->speed = speed;
            if (stage_get_stage_act() < STAGE_ACT(STAGE_CHAOS0, 0)) {
                camera_restore();
            }
            work->flags &= ~PLAYER_GUARD;
            work->mode = KNUCKLES_FALL;
            record->anim.action = ACTION_FALL_START;
            break;
        }
        speed.x = 5.0F;
        speed.y = ((target->pos.y - work->pos.y) * 0.05F) + record->speed.y;
        if (speed.y < -5.0F) {
            speed.y = -5.0F;
        } else if (speed.y > 5.0F) {
            speed.y = 5.0F;
        }
        speed.z = 0.0F;
        record->speed = speed;
        player_turn_to_world(work, &speed);
        motion->speed = speed;
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        if (!(KNUCKLES_VARS(work)->homing_steps & 8)) {
            sound_play(SE_HOMING, 0, 0, 0);
        }
        vibration_start(PLAYER_VARS(work)->player, 2, 30, 0);
        break;
    }
    case KNUCKLES_DIG_DOWN:
    case KNUCKLES_DIG_UP:
        break;
    case KNUCKLES_DIG_FAIL:
        if ((record->anim.frame += 0.7F) > 10.0F) {
            work->mode = KNUCKLES_DIG_FAIL_BACK;
            /* @hack redundant: a redundant break, which puts 0.5F in R1 and the
             * record in R2 for KNUCKLES_DIG_FAIL_BACK's arm (0x8c0c8fc6,
             * 0x8c0c8fc8). without it, the two trade and 5 of 1873 words
             * differ */
            break;
        }
        break;
    case KNUCKLES_DIG_FAIL_BACK:
        if ((record->anim.frame -= 0.5F) < 0.5F) {
            knuckles_stand(work, motion, record);
        }
        break;
    case KNUCKLES_CLIMB:
        if (record->anim.action != ACTION_GRAB_WALL &&
            record->anim.action != ACTION_CLIMB_OUT) {
            player_accel_climb(work, motion, record);
            player_apply_climb_accel(work, motion, record);
            player_collide_climb(work, motion, record);
        }
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_CLIMB_OUT:
        work->pos.x += motion->speed.x;
        work->pos.y += motion->speed.y;
        work->pos.z += motion->speed.z;
        break;
    case KNUCKLES_PUNCH: {
        NJS_POINT3 pos;

        knuckles_punch(work, motion, record);
        player_turn_to_stick(work, motion, record);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        /* @hack redundant: a second test of ACTION_PUNCH_1, which the image
         * makes again (CMP/EQ #54 at 0x8c0c90d0 and at 0x8c0c90dc). tested
         * once, the code after the test moves up, and 462 of 1873 words
         * differ */
        /* NOLINTNEXTLINE(misc-redundant-expression) */
        if (record->anim.action == ACTION_PUNCH_1 ||
            record->anim.action == ACTION_PUNCH_1) {
            if (record->speed.x > 0.2F) {
                record->speed.x += (0.2F - record->speed.x) * 0.5F;
            }
        }
        if (player_collide(work, motion, record)) {
            player_get_history(PLAYER_VARS(work)->player, 1, &pos, NULL);
            work->pos = pos;
            record->speed.x = record->speed.y = record->speed.z = 0.0F;
            motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        }
        player_take_speed(work, motion, record);
        break;
    }
    case KNUCKLES_HANG:
    case KNUCKLES_HANG_GO:
        player_slide_on_path(work, motion, record);
        knuckles_hang_move(work, motion, record);
        break;
    case KNUCKLES_LEDGE:
    case KNUCKLES_LEDGE_GO:
        player_shift_on_path(work, motion, record);
        knuckles_ledge_move(work, motion, record);
        break;
    case KNUCKLES_PANEL_JUMP:
        work->flags &= ~PLAYER_ON_TASK;
        panel_fly(work, motion, record);
        break;
    case KNUCKLES_PANEL_WAIT:
        break;
    case KNUCKLES_ICED: {
        Float stroke;
        Angle ang;

        player_mouth_set_script(0, 30);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
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
    case KNUCKLES_LIFT:
        player_turn_to_stick(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_PUT_DOWN:
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_HOLD_STAND:
        player_turn_to_stick(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_GRIP:
        player_accel_push(work, motion, record);
        player_apply_push(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_THROW:
        player_turn_to_target(work, motion, record);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_HOLD_WALK:
        if (record->anim.previous == ACTION_HOLD_TURN &&
            record->anim.current != ACTION_HOLD_TURN) {
            record->speed.x += record->consts.trot_speed;
        }
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        knuckles_set_walk_action(work, motion, record);
        break;
    case KNUCKLES_HOLD_STOP:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_HOLD_FALL:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_HOLD_JUMP:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_HOLD_SPRING:
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_HOLD_LAUNCH:
        if (record->speed.y >= record->consts.launch_speed) {
            player_accel_launched(work, motion, record);
        } else {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_PULL:
        player_accel_push(work, motion, record);
        player_apply_push(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_AIR_THROW:
        player_turn_upright(work, motion, record);
        player_turn_to_target(work, motion, record);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_SHAKE:
        player_shake_touched(work, motion, record);
        break;
    case KNUCKLES_HOLD_SHAKE:
        record->held->motion->accel.x = record->held->motion->accel.y =
            record->held->motion->accel.z = record->anim.frame * 0.01F;
        break;
    case KNUCKLES_EVENT:
        event_paths_step(work);
        break;
    case KNUCKLES_RISE:
        /* once his action is done, he rises to the water's surface above
         * him, 0.4 a step at most */
        if ((record->anim.flags & PLAYER_ACTION_DONE) &&
            record->shadow.above > work->collision->shapes->center.y &&
            (record->shadow.above_flags & WORLD_WATER)) {
            record->speed.y =
                record->shadow.above - work->collision->shapes->center.y;
            if (record->speed.y > 0.4F) {
                record->speed.y = 0.4F;
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
    case KNUCKLES_DOWN:
        if (work->flags & PLAYER_ON_GROUND) {
            record->anim.action = ACTION_DOWN_LAND;
        }
        player_turn_upright(work, motion, record);
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case KNUCKLES_MODULE_POSE:
        break;
    case KNUCKLES_EDIT:
        knuckles_edit(work, motion, record);
        break;
    }
    /* he steps back the way he came when he's past the edges of the act's
     * death zones. he does so only in a stage, a trial or a field, and not
     * as he glides or turns back from a glide. the step is the offset
     * across the ground from him to the newest of his last 127 places that
     * lies more than 0.1 away. with none that far, it is the offset to the
     * oldest. he takes it once, then again while he's still past the edges,
     * 129 times at most */
    if (work->mode != KNUCKLES_GLIDE && work->mode != KNUCKLES_GLIDE_TURN &&
        (game_mode == GAME_MODE_ACTION || game_mode == GAME_MODE_FIELD ||
         game_mode == GAME_MODE_TRIAL) &&
        knuckles_outside_zones(work) == TRUE) {
        /* @hack type: back as a byte, which SHC keeps at @(23,R15) and
         * zero-extends for the compare to 128 (EXTU.B at 0x8c0c98d2). as
         * an int, it takes the word at @(20,R15), and 146 of 1873 words
         * differ */
        Uint8 back;
        Float dx;
        Float dz;
        NJS_POINT3 pos;

        for (back = 1; back < 128; back++) {
            /* @hack type: back cast to Sint8, which SHC passes on as MOV.B
             * loads it (MOV.B @(R0,R15),R5 at 0x8c0c9884). uncast, it takes
             * an EXTU.B there, and 61 of 1873 words differ */
            player_get_history(PLAYER_VARS(work)->player, (Sint8)back, &pos,
                               NULL);
            dx = pos.x - work->pos.x;
            dz = pos.z - work->pos.z;
            if ((dx * dx) + (dz * dz) > 0.1F * 0.1F) {
                break;
            }
        }
        back = 128;
        work->pos.x += dx;
        work->pos.z += dz;
        while (back > 0 && knuckles_outside_zones(work) == TRUE) {
            back--;
            work->pos.x += dx;
            work->pos.z += dz;
        }
    }
    player_anim_step(&record->anim);
    knuckles_set_hands(work, motion, record);
    if (!(work->flags & PLAYER_BALL)) {
        player_update_welds(work, motion, record);
    }
    knuckles_draw(task);
    if (record->held != NULL) {
        knuckles_carry(work, motion, record);
    }
    shadow_update(work, &record->shadow);
    knuckles_set_shapes(work, record);
    knuckles_set_dig_chances(work);
    knuckles_dig_forced = FALSE;
    if (record->anim.action != ACTION_CLEAR &&
        record->anim.action != ACTION_CLEAR_ONCE) {
        collision_register(work);
    }
    player_finish_step(work, motion, record);
}

/* NOLINTNEXTLINE(readability-function-size) */
static void knuckles_choose_mode(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record)
{
    if (record != NULL && (record->effects & PLAYER_INVINCIBLE)) {
        work->flags |= PLAYER_RECOILING;
        work->timer = 0;
    } else if (work->timer == 0) {
        if (knuckles_try_hurt(work, motion, record)) {
            work->flags |= PLAYER_RECOILING;
            if (work->flags & PLAYER_HOLDING) {
                knuckles_let_go(work, motion, record);
            }
            work->flags &= ~(PLAYER_ON_PATH | PLAYER_REQUEST | PLAYER_HOLDING |
                             PLAYER_BALL);
            if (record->effects & (PLAYER_BARRIER | PLAYER_MAGNETIC_BARRIER)) {
                record->effects &= ~(PLAYER_BARRIER | PLAYER_MAGNETIC_BARRIER);
            } else {
                ring_lose(PLAYER_VARS(work)->player);
            }
        } else {
            work->flags &= ~PLAYER_RECOILING;
        }
    } else if (TASK_TIMER(work) == KNUCKLES_TIMER_HELD) {
        /* @hack spelling: an empty arm, which leaves the held timer as it is,
         * and whose jump loads work into R2 for timer-- (0x8c0c9b6c).
         * tested with != around the countdown, work takes R1, and 5 of 4453
         * words differ */
    } else {
        work->timer--;
        work->flags &= ~PLAYER_HURT;
    }
    switch (work->mode) {
    case KNUCKLES_STAND:
        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (knuckles_try_fall(work, motion, record)) {
            break;
        }
        if (knuckles_try_dig(work, motion, record)) {
            break;
        }
        if (knuckles_try_jump(work, motion, record)) {
            break;
        }
        if (knuckles_try_grab(work, motion, record)) {
            break;
        }
        if (knuckles_try_punch(work, motion, record)) {
            break;
        }
        if (player_read_stick(work, NULL, NULL) || record->speed.x != 0.0F) {
            work->mode = KNUCKLES_WALK;
            record->anim.action = ACTION_WALK;
            break;
        }
        break;
    case KNUCKLES_WALK:
        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (knuckles_try_fall(work, motion, record)) {
            break;
        }
        if (record->speed.x > 2.0F) {
            if (knuckles_try_jump(work, motion, record)) {
                break;
            }
            if (knuckles_try_roll(work, motion, record)) {
                break;
            }
        } else {
            if (knuckles_try_dig(work, motion, record)) {
                break;
            }
            if (knuckles_try_jump(work, motion, record)) {
                break;
            }
            if (knuckles_try_punch(work, motion, record)) {
                break;
            }
        }
        if (knuckles_try_stop(work, motion, record)) {
            break;
        }
        /* he starts to push a task whose shape pushes him back from the
         * side, if he isn't hurt and faces it head on */
        if ((work->collision->flags & COLLISION_PUSHED) &&
            !(work->flags & PLAYER_HURT) &&
            (work->collision->flags & COLLISION_HIT_SIDE) &&
            (work->collision->hit->shapes[work->collision->hit_shape].push &
             PUSH_ALL)) {
            /* @hack spelling: a switch of one case, which keeps work in R2 and
             * the values its arm stores in R3 (0x8c0c9e3a to 0x8c0c9e46). as
             * an if, the two trade and 6 of 4453 words differ */
            switch (player_face_wall(work, motion)) {
            case PLAYER_WALL_HEAD_ON:
                work->mode = KNUCKLES_PUSH;
                record->anim.action = ACTION_PUSH_START;
                break;
            }
        }
        if (player_stick_back_or_end_event(work, motion) &&
            record->speed.x >= record->consts.trot_speed) {
            work->mode = KNUCKLES_STOP;
            record->anim.action = ACTION_SKID;
            sound_play(SE_SKID, 0, 0, 0);
        }
        break;
    case KNUCKLES_PATH:
        work->flags &= ~PLAYER_ATTACKING;
        if (knuckles_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (knuckles_try_fall(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (knuckles_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (record->speed.x < record->consts.trot_speed) {
            work->mode = KNUCKLES_WALK;
            record->anim.action = ACTION_WALK;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (knuckles_try_stop(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        break;
    case KNUCKLES_ROLL:
        if (knuckles_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (knuckles_try_fall(work, motion, record)) {
            record->anim.action = ACTION_SPIN;
            record->action_speed = 2.0F;
            work->ang.x = work->ang.z = 0;
            work->flags |= PLAYER_ATTACKING;
            break;
        }
        if (knuckles_try_jump(work, motion, record)) {
            record->anim.action = ACTION_SPIN;
            record->action_speed = 2.0F;
            work->ang.x = work->ang.z = 0;
            work->flags |= PLAYER_ATTACKING;
            break;
        }
        if (knuckles_try_unroll(work, motion, record)) {
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                        pad_action_buttons)) {
            work->mode = KNUCKLES_WALK;
            record->anim.action = ACTION_UNCURL;
            work->flags &= ~PLAYER_ATTACKING;
            /* @hack redundant: a redundant break, which leaves the JMP's delay
             * slot a NOP (0x8c0ca11c). without it, the flags' store fills
             * the slot, the code after it moves, and 62 of 4453 words
             * differ */
            break;
        }
        break;
    case KNUCKLES_PATH_SPIN:
        if (knuckles_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_ATTACKING;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (knuckles_try_fall(work, motion, record)) {
            record->anim.action = ACTION_SPIN;
            record->action_speed = 2.0F;
            work->ang.x = work->ang.z = 0;
            work->flags |= PLAYER_ATTACKING;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (record->speed.x < PLAYER_SPIN_SPEED) {
            record->anim.action = ACTION_SPIN;
            record->action_speed = 2.0F;
            work->ang.x = work->ang.z = 0;
            work->flags |= PLAYER_ATTACKING;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        break;
    case KNUCKLES_JUMP:
        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = KNUCKLES_STOP;
                record->anim.action = ACTION_SKID;
            } else if (knuckles_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                record->anim.action = ACTION_LAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = KNUCKLES_WALK;
                knuckles_set_walk_action(work, motion, record);
            }
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        /* in his jump's spin, a press of the jump button starts his glide.
         * in a curl, holding the button to its last two frames does */
        if (record->anim.action == ACTION_SPIN) {
            if (player_jump_pressed(work, record)) {
                work->mode = KNUCKLES_GLIDE;
                record->anim.action = ACTION_GLIDE;
                knuckles_glide_start(work, motion, record);
                work->flags &= ~PLAYER_ATTACKING;
                work->flags |= PLAYER_GUARD;
                record->own.knuckles.glide.bumps = 0;
                break;
            }
        } else if (record->anim.action == ACTION_CURL &&
                   PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                               pad_jump_buttons) &&
                   record->anim.frame >
                       knuckles_actions[ACTION_CURL].action->motion->nbFrame -
                           2) {
            work->mode = KNUCKLES_GLIDE;
            record->anim.action = ACTION_GLIDE;
            knuckles_glide_start(work, motion, record);
            work->flags &= ~PLAYER_ATTACKING;
            work->flags |= PLAYER_GUARD;
            record->own.knuckles.glide.bumps = 0;
            break;
        }
        if (knuckles_try_uncurl(work, motion, record)) {
            break;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.8F;
            break;
        }
        if (motion->speed.y > 0.0F) {
            break;
        }
        if (knuckles_glide_collide(work, motion, record) == GLIDE_CLIMB) {
            work->flags &= ~PLAYER_ATTACKING;
            knuckles_start_climb(work, motion, record);
            /* @hack redundant: a redundant break, which puts motion in R2, not
             * R3, for KNUCKLES_SPRING's test (0x8c0ca4a0); without it, 2 of
             * 4453 words differ */
            break;
        }
        break;
    case KNUCKLES_SPRING:
        if (motion->speed.y <= 0.0F) {
            work->mode = KNUCKLES_FALL;
            record->anim.action = ACTION_FALL_START;
        }
        break;
    case KNUCKLES_LAUNCH:
        if (motion->speed.y <= 0.0F) {
            work->mode = KNUCKLES_FALL;
            record->anim.action = ACTION_FALL_START;
        }
        break;
    case KNUCKLES_FALL:
    case KNUCKLES_DIG_OUT:
        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = KNUCKLES_STOP;
                record->anim.action = ACTION_SKID;
            } else if (knuckles_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                record->anim.action = ACTION_LAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = KNUCKLES_WALK;
                knuckles_set_walk_action(work, motion, record);
            }
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                        pad_jump_buttons)) {
            work->mode = KNUCKLES_JUMP;
            record->anim.action = ACTION_CURL;
            record->action_speed = 2.0F;
            work->flags |= PLAYER_ATTACKING;
            record->jump_steps = 0;
            sound_stop(SE_GLIDE);
            /* @hack redundant: a redundant break, which joins this arm to
             * KNUCKLES_GLIDE's identical one, sound_stop and all. without
             * it, this arm keeps a copy of its own, the code after it moves,
             * and 3411 of 4453 words differ */
            break;
        }
        break;
    case KNUCKLES_STOP:
        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (knuckles_try_fall(work, motion, record)) {
            break;
        }
        if (knuckles_try_jump(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_SKID) {
            knuckles_stand(work, motion, record);
        }
        break;
    case KNUCKLES_GLIDE:
        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (knuckles_outside_zones(work)) {
            if (stage_get_stage_act() < STAGE_ACT(STAGE_CHAOS0, 0)) {
                camera_override_mode(68, 0);
            }
            motion->aim.y = work->ang.y + 0x8000;
            KNUCKLES_VARS(work)->steps = GLIDE_TURN_STEPS;
            work->mode = KNUCKLES_GLIDE_TURN;
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            work->flags &= ~PLAYER_ATTACKING;
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = KNUCKLES_STOP;
                record->anim.action = ACTION_GLIDE_LAND;
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
            } else if (knuckles_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                record->anim.action = ACTION_GLIDE_LAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = KNUCKLES_WALK;
                knuckles_set_walk_action(work, motion, record);
            }
            sound_stop(SE_GLIDE);
            break;
        }
        if (!PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                         pad_jump_buttons)) {
            work->mode = KNUCKLES_JUMP;
            record->anim.action = ACTION_CURL;
            record->action_speed = 2.0F;
            work->flags |= PLAYER_ATTACKING;
            record->jump_steps = 0;
            sound_stop(SE_GLIDE);
            break;
        }
        /* in the adventure fields, his bumps lose 1 a step, so four walls
         * in quick succession end his glide */
        {
            Uint16 stage_act;

            /* @hack spelling: stage_act set inside the test, which loads
             * 0x2300 into R2 for the compare (MOV.W at 0x8c0ca820). set in
             * its declaration, 0x2300 takes R3, the bumps' test takes other
             * registers, and 22 of 4453 words differ */
            if ((stage_act = stage_get_stage_act()) >=
                    STAGE_ACT(STAGE_STATION_SQUARE, 0) &&
                stage_act < STAGE_ACT(STAGE_CART, 0)) {
                /* @hack redundant: the decrement in an else of this test, whose
                 * arm breaks, which puts the record in R2 and the bumps in
                 * R1 for it (0x8c0ca8a2, 0x8c0ca8a4). as an if of its own,
                 * R1 and R3 take them, and 8 of 4453 words differ */
                if (record->own.knuckles.glide.bumps >= GLIDE_BUMPS_MAX) {
                    work->flags &= ~PLAYER_GUARD;
                    work->mode = KNUCKLES_FALL;
                    record->anim.action = ACTION_FALL;
                    break;
                } else if (--record->own.knuckles.glide.bumps < 0) {
                    record->own.knuckles.glide.bumps = 0;
                }
            }
        }
        if (!(++KNUCKLES_VARS(work)->steps & 0x3f)) {
            sound_play(SE_GLIDE, 0, 0, 0);
        }
        break;
    case KNUCKLES_GLIDE_TURN:
        if (!knuckles_outside_zones(work)) {
            if (--KNUCKLES_VARS(work)->steps <= 0) {
                if (stage_get_stage_act() < STAGE_ACT(STAGE_CHAOS0, 0)) {
                    camera_restore();
                }
                work->mode = KNUCKLES_GLIDE;
                record->own.knuckles.glide.bumps = 0;
            }
            break;
        }
        if (KNUCKLES_VARS(work)->steps != GLIDE_TURN_STEPS) {
            KNUCKLES_VARS(work)->steps = GLIDE_TURN_STEPS;
        }
        break;
    case KNUCKLES_PUSH:
        if (knuckles_try_request(work, motion, record) ||
            knuckles_try_jump(work, motion, record) ||
            knuckles_try_fall(work, motion, record) ||
            knuckles_try_stop(work, motion, record)) {
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
                    work->mode = KNUCKLES_WALK;
                    record->anim.action = ACTION_WALK;
                    sound_stop(SE_PUSH);
                }
                break;
            default:
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = KNUCKLES_PULL;
                    record->anim.action = ACTION_PULL;
                } else {
                    work->mode = KNUCKLES_WALK;
                    record->anim.action = ACTION_WALK;
                }
                sound_stop(SE_PUSH);
                break;
            }
            break;
        }
        knuckles_let_go(work, motion, record);
        work->mode = KNUCKLES_WALK;
        record->anim.action = ACTION_WALK;
        sound_stop(SE_PUSH);
        break;
    case KNUCKLES_HURT:
        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (TASK_TIMER(work) == PLAYER_HURT_STEPS) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (knuckles_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                record->anim.action = ACTION_HURT_LAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = KNUCKLES_WALK;
                knuckles_set_walk_action(work, motion, record);
            }
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                        pad_jump_buttons)) {
            work->mode = KNUCKLES_JUMP;
            record->anim.action = ACTION_CURL;
            record->action_speed = 2.0F;
            work->flags |= PLAYER_ATTACKING;
            record->jump_steps = 0;
            /* @hack redundant: a redundant break, which leaves the JMP's delay
             * slot a NOP (0x8c0cabc8). without it, the store to jump_steps
             * fills the slot and 16 of 4453 words differ */
            break;
        }
        break;
    case KNUCKLES_BACK:
        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (knuckles_try_fall(work, motion, record)) {
            break;
        }
        if (knuckles_try_jump(work, motion, record)) {
            break;
        }
        if (knuckles_try_stop(work, motion, record)) {
            break;
        }
        if (record->speed.x >= 0.0F) {
            work->mode = KNUCKLES_WALK;
            knuckles_set_walk_action(work, motion, record);
        }
        break;
        /* @hack redundant: a second break, which no path reaches, and which
         * puts work in R2, not R3, for KNUCKLES_POSE's test (0x8c0cac78);
         * without it, 3 of 4453 words differ */
        /* NOLINTNEXTLINE(clang-diagnostic-unreachable-code-break) */
        break;
    case KNUCKLES_POSE:
        if (work->flags & PLAYER_HOLDING) {
            if (knuckles_try_request_holding(work, motion, record)) {
                break;
            }
        } else if (knuckles_try_request(work, motion, record)) {
            break;
        }
        break;
    case KNUCKLES_CRASH:
        if (record->anim.action == ACTION_CRASH) {
            break;
        }
        if (knuckles_try_fall(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_CRASH_LAND) {
            knuckles_stand(work, motion, record);
        }
        break;
    case KNUCKLES_HOMING: {
        Bool stopped;
        int back;
        NJS_POINT3 pos;

        if (knuckles_try_request(work, motion, record)) {
            if (stage_get_stage_act() < STAGE_ACT(STAGE_CHAOS0, 0)) {
                camera_restore();
            }
            break;
        }
        /* after its first HOMING_MIN_STEPS, a homing ends once he has stayed
         * within 1 of each of his last three places. it ends anyway once its
         * steps run out */
        stopped = FALSE;
        if (KNUCKLES_VARS(work)->homing_steps <
            HOMING_STEPS - HOMING_MIN_STEPS) {
            for (back = 3; back > 0; back--) {
                player_get_history(PLAYER_VARS(work)->player, back, &pos, NULL);
                if (njDistanceP2P(&work->pos, &pos) > 0.99999999F) {
                    break;
                }
            }
            if (!back) {
                stopped = TRUE;
            }
        }
        if (stopped == TRUE || --KNUCKLES_VARS(work)->homing_steps <= 0) {
            if (stage_get_stage_act() < STAGE_ACT(STAGE_CHAOS0, 0)) {
                camera_restore();
            }
            work->flags &= ~PLAYER_GUARD;
            work->mode = KNUCKLES_FALL;
            record->anim.action = ACTION_FALL;
        }
        break;
    }
    case KNUCKLES_DIG_DOWN: {
        Angle ang;
        struct task *dirt;
        NJS_VECTOR up;

        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        KNUCKLES_VARS(work)->steps++;
        if (KNUCKLES_VARS(work)->steps > 25 &&
            !(KNUCKLES_VARS(work)->steps & 7)) {
            for (ang = (Angle)(rand() * 0.0625F); ang < 0x10000; ang += 0x800) {
                dirt = task_create(TASK_WORK, 6, knuckles_dirt_step);
                if (dirt != NULL) {
                    dirt->work->pos = work->pos;
                    dirt->work->scl.x = rand() * njSin(ang) * RAND_SCALE;
                    dirt->work->scl.y = rand() * RAND_SCALE;
                    dirt->work->scl.z = rand() * njCos(ang) * RAND_SCALE;
                }
            }
            up.x = up.z = 0.0F;
            up.y = 1.0F;
            knuckles_scatter_rocks(&work->pos, &up);
        }
        if (KNUCKLES_VARS(work)->steps < 40) {
            break;
        }
        KNUCKLES_VARS(work)->depth -= 0.4F;
        if (KNUCKLES_VARS(work)->depth < -DIG_DEPTH) {
            knuckles_dig_find(work, motion, record);
            work->mode = KNUCKLES_DIG_UP;
            record->anim.action = ACTION_DIG_UP;
            sound_stop(SE_DIG);
        }
        if (!(KNUCKLES_VARS(work)->steps & 7)) {
            sound_stop(SE_DIG);
            sound_play(SE_DIG, 0, 0, 0);
            vibration_start(PLAYER_VARS(work)->player, 2, 15, 0);
        }
        break;
    }
    case KNUCKLES_DIG_UP: {
        Angle ang;
        struct task *dirt;
        NJS_VECTOR up;

        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        KNUCKLES_VARS(work)->depth += 2.0F;
        if (KNUCKLES_VARS(work)->depth >= 0.0F) {
            knuckles_dig_last_pos = work->pos;
            record->speed.y = 2.0F;
            work->mode = KNUCKLES_DIG_OUT;
            record->anim.action = ACTION_DIG_OUT;
            break;
        }
        if (!(++KNUCKLES_VARS(work)->steps & 7)) {
            for (ang = (Angle)(rand() * 0.25F); ang < 0x10000; ang += 0x2000) {
                dirt = task_create(TASK_WORK, 6, knuckles_dirt_step);
                if (dirt != NULL) {
                    dirt->work->pos = work->pos;
                    dirt->work->scl.x = rand() * njSin(ang) * RAND_SCALE;
                    dirt->work->scl.y = rand() * (3.5F * RAND_SCALE);
                    dirt->work->scl.z = rand() * njCos(ang) * RAND_SCALE;
                }
            }
            up.x = up.z = 0.0F;
            up.y = 2.0F;
            knuckles_scatter_rocks(&work->pos, &up);
        }
        break;
    }
    case KNUCKLES_DIG_FAIL:
    case KNUCKLES_DIG_FAIL_BACK:
        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (knuckles_try_fall(work, motion, record)) {
            break;
        }
        if (work->mode != KNUCKLES_DIG_FAIL_BACK) {
            break;
        }
        if (knuckles_try_jump(work, motion, record)) {
            break;
        }
        if (knuckles_try_grab(work, motion, record)) {
            break;
        }
        if (knuckles_try_punch(work, motion, record)) {
            break;
        }
        if (player_read_stick(work, NULL, NULL) || record->speed.x != 0.0F) {
            work->mode = KNUCKLES_WALK;
            record->anim.action = ACTION_WALK;
        }
        break;
    case KNUCKLES_CLIMB:
        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        /* on top of a task, he steps back from the wall and stands */
        if (work->flags & (COLLISION_ON_TOP | PLAYER_ON_TASK)) {
            record->speed.x = record->speed.y = record->speed.z =
                motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
            work->pos.x +=
                record->consts.height * record->contact->hit.normal.x * 0.3F;
            work->pos.z +=
                record->consts.height * record->contact->hit.normal.z * 0.3F;
            knuckles_stand(work, motion, record);
            break;
        }
        if (motion->speed.y >= 0.0F) {
            /* climbing up to the top of the wall, he climbs out over it */
            if (record->contact->hit.normal.y >= 0.86F) {
                NJS_VECTOR down;

                down.x = down.z = 0.0F;
                down.y = -1.0F;
                player_turn_to_world(work, &down);
                motion->speed.x = down.x * record->consts.height * 0.01F;
                motion->speed.z = down.z * record->consts.height * 0.01F;
                motion->speed.y = record->consts.height * 0.028F;
                work->mode = KNUCKLES_CLIMB_OUT;
                record->anim.action = ACTION_CLIMB_OUT;
                break;
            }
        } else if (record->contact->hit.normal.y >= 0.86F) {
            /* climbing down onto the ground below, he stands */
            record->speed.x = record->speed.y = record->speed.z =
                motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
            work->pos.x +=
                record->consts.height * record->contact->hit.normal.x * 0.3F;
            work->pos.z +=
                record->consts.height * record->contact->hit.normal.z * 0.3F;
            knuckles_stand(work, motion, record);
            break;
        }
        if (knuckles_try_wall_jump(work, record)) {
            break;
        }
        if (!knuckles_wall_near(work, motion, record)) {
            if (--GRIP_STEPS(record) <= 0) {
                work->mode = KNUCKLES_FALL;
                record->anim.action = ACTION_FALL;
                break;
            }
        } else {
            GRIP_STEPS(record) = GRIP_HOLD_STEPS;
        }
        switch (record->anim.action) {
        case ACTION_CLIMB:
        case ACTION_CLIMB_MINUS_Z:
        case ACTION_CLIMB_PLUS_Z:
            /* a climbing action plays a step's sound and scatters grit from
             * his hands as it reaches frame 0 and frame 16 */
            if ((record->anim.frame >= 0.0F && record->anim.frame < 1.5F) ||
                (record->anim.frame >= 16.0F && record->anim.frame < 17.5F)) {
                NJS_POINT3 pos;
                NJS_VECTOR speed;

                pos = work->pos;
                pos.y += 4.0F;
                speed.x = speed.y = speed.z = 0.0F;
                knuckles_scatter_rocks(&pos, &speed);
                sound_play(SE_CLIMB, 0, 0, 0);
            }
            break;
        }
        knuckles_set_climb_action(work, record);
        break;
    case KNUCKLES_PUNCH:
        if (knuckles_try_request(work, motion, record) ||
            record->anim.action == ACTION_STAND) {
            work->collision->shapes[1].flags |= SHAPE_OFF;
            knuckles_stand(work, motion, record);
            work->flags &= ~PLAYER_GUARD;
            break;
        }
        if (record->anim.action == ACTION_PUNCH_1 ||
            record->anim.action == ACTION_PUNCH_B1) {
            if (knuckles_try_dig(work, motion, record)) {
                work->flags &= ~PLAYER_GUARD;
                break;
            }
        }
        knuckles_punch_next(work, motion, record);
        if (!PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                         pad_action_buttons)) {
            CHARGE(record) = -1;
        }
        break;
    case KNUCKLES_CLIMB_OUT:
        if (record->anim.action == ACTION_STAND) {
            work->mode = KNUCKLES_STAND;
        }
        break;
    case KNUCKLES_HANG: {
        Angle ang;

        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (knuckles_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (work->flags & PLAYER_ON_PATH) {
            if (player_read_stick(work, &ang, NULL)) {
                if (angle_distance(work->ang.y, ang) < 0x4000) {
                    record->speed.x = 0.375F;
                    if (record->anim.action == ACTION_HANG_AT_0) {
                        record->anim.start_frame = 0.0F;
                    } else {
                        record->anim.start_frame = 32.0F;
                    }
                } else {
                    record->speed.x = -0.375F;
                    if (record->anim.action == ACTION_HANG_AT_0) {
                        record->anim.start_frame = 0.0F;
                    } else {
                        record->anim.start_frame = 32.0F;
                    }
                }
                work->mode = KNUCKLES_HANG_GO;
                record->anim.action = ACTION_HANG_GO;
                break;
            }
        } else {
            work->mode = KNUCKLES_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                        pad_action_buttons)) {
            work->mode = KNUCKLES_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a redundant break, which leaves the JMP's delay
             * slot a NOP (0x8c0cb93c). without it, the flags' store fills
             * the slot, the code after it moves, and 54 of 4453 words
             * differ */
            break;
        }
        break;
    }
    case KNUCKLES_HANG_GO:
        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (knuckles_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (work->flags & PLAYER_ON_PATH) {
            /* @hack spelling: an empty arm, which puts work in R2 and
             * KNUCKLES_FALL in R1 for the else's stores (0x8c0cb97a,
             * 0x8c0cb97c). tested as if (!...), the two trade and 6 of 4453
             * words differ */
        } else {
            work->mode = KNUCKLES_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                        pad_action_buttons)) {
            work->mode = KNUCKLES_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a redundant break, which leaves the JMP's delay
             * slot a NOP (0x8c0cb9fc). without it, the flags' store fills
             * the slot, the code after it moves, and 69 of 4453 words
             * differ */
            break;
        }
        break;
    case KNUCKLES_LEDGE: {
        Angle ang;

        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (knuckles_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        /* @hack redundant: the else after the arm that breaks, which keeps work
         * in R1 and KNUCKLES_FALL in R2 (0x8c0cbad0, 0x8c0cbad2). without it,
         * R2 and R3 take them, and 8 of 4453 words differ */
        if (work->flags & PLAYER_ON_PATH) {
            if (!player_read_stick(work, &ang, NULL)) {
                break;
            }
            /* the stick, as the camera sees it, moves him one way or the
             * other along the ledge's path */
            /* @hack redundant: the else after the arm that breaks, which puts
             * the record in R2, not R3, for its store and jump (0x8c0cbac2);
             * without it, 10 of 4453 words differ */
            if (angle_distance(0xc000 - camera_work->ang.y, ang) < 0x4000) {
                record->speed.x = 0.375F;
                /* @hack redundant: a redundant break, which gives this arm a
                 * store and jump of its own (JMP @R2 at 0x8c0cba72, the
                 * store in its slot). without it, the else's arm branches to
                 * that store and jump, and 83 of 4453 words differ */
                break;
            } else {
                record->speed.x = -0.375F;
                /* @hack redundant: a redundant break, which leaves the JMP's
                 * delay slot a NOP (0x8c0cbace). without it, the store fills
                 * the slot, the code after it moves, and 77 of 4453 words
                 * differ */
                break;
            }
            /* @hack redundant: a break after an if whose arms both break,
             * which puts work in R1 and KNUCKLES_FALL in R2 for the fall's
             * stores (0x8c0cbad0, 0x8c0cbad2). without it, the two trade and
             * 6 of 4453 words differ */
            /* NOLINTNEXTLINE(clang-diagnostic-unreachable-code-break) */
            break;
        } else {
            work->mode = KNUCKLES_FALL;
            record->anim.action = ACTION_FALL;
            player_clear_speed(motion, record);
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
    }
    case KNUCKLES_LEDGE_GO:
        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (knuckles_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (work->flags & PLAYER_ON_PATH) {
            /* @hack spelling: an empty arm, which puts work in R2 and
             * KNUCKLES_FALL in R1 for the else's stores (0x8c0cbb38,
             * 0x8c0cbb3a). tested as if (!...), the two trade and 6 of 4453
             * words differ */
        } else {
            work->mode = KNUCKLES_FALL;
            record->anim.action = ACTION_FALL;
            player_clear_speed(motion, record);
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        break;
    case KNUCKLES_PANEL_JUMP:
        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            record->speed.x = record->speed.y = record->speed.z = 0.0F;
            motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
            if (panel_jump_ready_or_idle(work)) {
                work->mode = KNUCKLES_PANEL_WAIT;
                record->anim.action = ACTION_PANEL_WAIT;
                break;
            }
            work->mode = KNUCKLES_STAND;
            record->anim.action = ACTION_STAND_LANDED;
        }
        break;
    case KNUCKLES_PANEL_WAIT:
        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (!panel_jump_ready_or_idle(work)) {
            work->mode = KNUCKLES_STAND;
            record->anim.action = ACTION_STAND_LANDED;
            break;
        }
        if (player_jump_pressed(work, record) != PLAYER_JUMP_PANEL) {
            break;
        }
        panel_jump(work);
        work->mode = KNUCKLES_PANEL_JUMP;
        record->anim.action = ACTION_PANEL_JUMP;
        break;
    case KNUCKLES_ICED:
        if (knuckles_try_request(work, motion, record)) {
            player_mouth_stop(PLAYER_VARS(work)->player);
            sound_play(SE_ICE_BREAK, 0, 0, 0);
            break;
        }
        if (pad_press_live[PLAYER_VARS(work)->player] & PAD_MASH_BUTTONS) {
            STRUGGLE(record) -= ICE_PRESS;
        } else {
            STRUGGLE(record) -= 1;
        }
        if (STRUGGLE(record) <= 0) {
            work->mode = KNUCKLES_JUMP;
            record->speed.y = record->consts.jump_speed;
            record->anim.action = ACTION_SPIN;
            record->action_speed = 2.0F;
            work->flags &= ~PLAYER_ON_TASK;
            work->flags |= PLAYER_ATTACKING;
            record->jump_steps = 0;
            sound_play(SE_JUMP, 0, 0, 0);
            sound_play(SE_ICE_BREAK, 0, 0, 0);
            player_mouth_stop(PLAYER_VARS(work)->player);
            /* @hack redundant: a redundant break, which puts work in R2, not
             * R3, for KNUCKLES_LIFT's test (0x8c0cbd9c); without it, 3 of 4453
             * words differ */
            break;
        }
        break;
    case KNUCKLES_LIFT:
        if (work->flags & PLAYER_HOLDING) {
            if (knuckles_try_request_holding(work, motion, record)) {
                break;
            }
        } else if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (knuckles_try_fall(work, motion, record)) {
            break;
        }
        if (knuckles_try_take_lifted(work, motion, record)) {
            break;
        }
        break;
    case KNUCKLES_GRIP:
        if (knuckles_try_grip(work, motion, record)) {
            break;
        }
        if (knuckles_try_jump(work, motion, record)) {
            break;
        }
        break;
    case KNUCKLES_HOLD_STAND:
        if (knuckles_try_request_holding(work, motion, record)) {
            break;
        }
        if (knuckles_try_fall(work, motion, record)) {
            break;
        }
        /* the grab button held with the stick pushed shakes the
         * CARRY_KIND_SMALL task he holds */
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                        pad_grab_buttons) &&
            player_read_stick(work, NULL, NULL) &&
            record->held->work->id == CARRY_KIND_SMALL) {
            work->mode = KNUCKLES_HOLD_SHAKE;
            record->anim.action = ACTION_HOLD_SHAKE_START;
            break;
        }
        if (knuckles_try_throw(work, motion, record)) {
            break;
        }
        if (knuckles_try_jump(work, motion, record)) {
            break;
        }
        if (player_read_stick(work, NULL, NULL)) {
            work->mode = KNUCKLES_HOLD_WALK;
            record->anim.action = ACTION_HOLD_WALK;
        }
        break;
    case KNUCKLES_HOLD_WALK:
        if (knuckles_try_request_holding(work, motion, record)) {
            break;
        }
        if (knuckles_try_fall(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (knuckles_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (knuckles_try_throw(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (knuckles_try_stop(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (player_stick_back_or_end_event(work, motion) &&
            record->speed.x >= record->consts.trot_speed) {
            work->mode = KNUCKLES_HOLD_STOP;
            record->anim.action = ACTION_HOLD_STOP;
            sound_play(SE_SKID, 0, 0, 0);
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a redundant break, which puts work in R3, not
             * R1, for KNUCKLES_PUT_DOWN's test (0x8c0cc068); without it, 3 of
             * 4453 words differ */
            break;
        }
        break;
    case KNUCKLES_PUT_DOWN:
    case KNUCKLES_THROW:
        if (work->flags & PLAYER_HOLDING) {
            if (knuckles_try_request_holding(work, motion, record)) {
                break;
            }
        } else if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (knuckles_try_release(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_THROW_AT &&
            record->anim.action != ACTION_THROW &&
            record->anim.action != ACTION_PUT_DOWN) {
            knuckles_stand(work, motion, record);
        }
        break;
    case KNUCKLES_HOLD_STOP:
        if (knuckles_try_request_holding(work, motion, record)) {
            break;
        }
        if (knuckles_try_fall(work, motion, record)) {
            break;
        }
        if (knuckles_try_jump(work, motion, record)) {
            break;
        }
        if (record->speed.x <= 0.0F &&
            player_stick_back_or_end_event(work, motion)) {
            work->mode = KNUCKLES_HOLD_WALK;
            record->anim.action = ACTION_HOLD_TURN;
            player_turn_to_stick(work, motion, record);
            work->ang.y = motion->aim.y;
            break;
        }
        if (record->anim.action != ACTION_HOLD_STOP) {
            work->mode = KNUCKLES_HOLD_STAND;
            record->anim.action = ACTION_HOLD_STAND;
        }
        break;
    case KNUCKLES_HOLD_FALL:
        if (knuckles_try_request_holding(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = KNUCKLES_HOLD_STOP;
                record->anim.action = ACTION_HOLD_STOP;
                break;
            }
            if (knuckles_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (motion->speed.y > -5.0F) {
                    record->anim.action = ACTION_HOLD_STAND;
                } else {
                    record->anim.action = ACTION_HOLD_HARD_LANDING;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = KNUCKLES_HOLD_WALK;
                knuckles_set_walk_action(work, motion, record);
            }
            break;
        }
        if (knuckles_try_throw(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case KNUCKLES_HOLD_JUMP:
        if (knuckles_try_request_holding(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = KNUCKLES_HOLD_STOP;
                record->anim.action = ACTION_HOLD_STOP;
            } else if (knuckles_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (motion->speed.y > -5.0F) {
                    record->anim.action = ACTION_HOLD_STAND;
                } else {
                    record->anim.action = ACTION_HOLD_HARD_LANDING;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = KNUCKLES_HOLD_WALK;
                knuckles_set_walk_action(work, motion, record);
            }
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.4F;
        }
        if (knuckles_try_throw(work, motion, record)) {
            break;
        }
        if (motion->speed.y <= 0.0F) {
            work->mode = KNUCKLES_HOLD_FALL;
            record->anim.action = ACTION_HOLD_FALL_START;
        }
        break;
    case KNUCKLES_HOLD_SPRING:
        if (motion->speed.y <= 0.0F) {
            work->mode = KNUCKLES_HOLD_FALL;
            record->anim.action = ACTION_HOLD_FALL_START;
        }
        break;
    case KNUCKLES_HOLD_LAUNCH:
        if (motion->speed.y <= 0.0F) {
            work->mode = KNUCKLES_HOLD_FALL;
            record->anim.action = ACTION_HOLD_FALL_START;
        }
        break;
    case KNUCKLES_PULL:
        if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (knuckles_try_jump(work, motion, record)) {
            break;
        }
        if (knuckles_try_fall(work, motion, record)) {
            break;
        }
        if (!PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                         pad_grab_buttons)) {
            knuckles_stand(work, motion, record);
            knuckles_let_go(work, motion, record);
        }
        if ((work->collision->flags & COLLISION_PUSHED) &&
            !(work->flags & PLAYER_HURT) &&
            !(work->collision->hit->shapes[work->collision->hit_shape].flags &
              SHAPE_NO_PUSH) &&
            (work->collision->flags & COLLISION_HIT_SIDE)) {
            switch (player_stick_into_wall(work, motion)) {
            case PLAYER_WALL_HEAD_ON:
                work->mode = KNUCKLES_PUSH;
                record->anim.action = ACTION_PUSH_START;
                break;
            case PLAYER_WALL_NO_STICK:
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = KNUCKLES_GRIP;
                    record->anim.action = ACTION_GRIP;
                }
                break;
            default:
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = KNUCKLES_PULL;
                    record->anim.action = ACTION_PULL;
                } else {
                    work->mode = KNUCKLES_WALK;
                    record->anim.action = ACTION_WALK;
                }
                break;
            }
            break;
        } else {
            knuckles_let_go(work, motion, record);
            work->mode = KNUCKLES_WALK;
            record->anim.action = ACTION_WALK;
            /* @hack redundant: a redundant break, which with the break below
             * puts work in R2, not R1, for KNUCKLES_AIR_THROW's test
             * (0x8c0cc6e0); without it, 2 of 4453 words differ */
            break;
        }
        /* @hack redundant: a break after an if whose arms both break, which
         * with the break above puts work in R2, not R1, for
         * KNUCKLES_AIR_THROW's test (0x8c0cc6e0); without it, 2 of 4453
         * words differ */
        /* NOLINTNEXTLINE(clang-diagnostic-unreachable-code-break) */
        break;
    case KNUCKLES_AIR_THROW:
        if (work->flags & PLAYER_HOLDING) {
            if (knuckles_try_request_holding(work, motion, record)) {
                break;
            }
        } else if (knuckles_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = KNUCKLES_HOLD_STOP;
                record->anim.action = ACTION_HOLD_STOP;
                break;
            }
            if (knuckles_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (work->flags & PLAYER_HOLDING) {
                    if (motion->speed.y > -5.0F) {
                        record->anim.action = ACTION_HOLD_STAND;
                    } else {
                        record->anim.action = ACTION_HOLD_HARD_LANDING;
                    }
                } else if (motion->speed.y > -5.0F) {
                    knuckles_set_stand_action(work, motion, record);
                } else {
                    record->anim.action = ACTION_HARD_LANDING;
                }
                break;
            }
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            if (work->flags & PLAYER_HOLDING) {
                work->mode = KNUCKLES_HOLD_WALK;
            } else {
                work->mode = KNUCKLES_WALK;
            }
            knuckles_set_walk_action(work, motion, record);
            break;
        }
        if (knuckles_try_release(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_AIR_THROW_AT &&
            record->anim.action != ACTION_AIR_THROW) {
            work->mode = KNUCKLES_FALL;
            record->anim.action = ACTION_FALL;
        }
        break;
    case KNUCKLES_SHAKE:
        if (knuckles_try_request_holding(work, motion, record)) {
            break;
        }
        if (knuckles_try_fall(work, motion, record)) {
            break;
        }
        if (knuckles_try_jump(work, motion, record)) {
            break;
        }
        if (player_touches_shakable(work, motion, record)) {
            if (!player_read_stick(work, NULL, NULL)) {
                work->mode = KNUCKLES_GRIP;
                record->anim.action = ACTION_GRIP;
            }
            break;
        }
        knuckles_stand(work, motion, record);
        knuckles_let_go(work, motion, record);
        break;
    case KNUCKLES_HOLD_SHAKE:
        if (knuckles_try_request_holding(work, motion, record)) {
            break;
        }
        if (knuckles_try_fall(work, motion, record)) {
            break;
        }
        if (knuckles_try_jump(work, motion, record)) {
            break;
        }
        if (record->held == NULL ||
            record->held->work->id != CARRY_KIND_SMALL) {
            knuckles_stand(work, motion, record);
            knuckles_let_go(work, motion, record);
            break;
        }
        if (!player_read_stick(work, NULL, NULL)) {
            work->mode = KNUCKLES_HOLD_STAND;
            record->anim.action = ACTION_HOLD_SHAKE_END;
            /* @hack redundant: a redundant break, which puts work in R3 and its
             * event in R2, not R1 and R3, for KNUCKLES_EVENT's test
             * (0x8c0cc9e0, 0x8c0cc9e4); without it, 7 of 4453 words
             * differ */
            break;
        }
        break;
    case KNUCKLES_EVENT:
        if (work->event->paths == NULL) {
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            knuckles_stand(work, motion, record);
        }
        break;
    case KNUCKLES_MODULE_POSE:
        if (!(work->flags & PLAYER_REQUEST)) {
            break;
        }
        if (work->submode != PLAYER_MODE_RIDE_OFF) {
            break;
        }
        work->mode = KNUCKLES_SPRING;
        record->anim.action = ACTION_SPRING;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case KNUCKLES_EDIT:
        if (pad_records[0].press & PDD_DGT_TA) {
            work->mode = KNUCKLES_WALK;
            record->anim.action = ACTION_WALK;
        }
        break;
    }
}

/* charge his maximum heat, with the fighting gloves. after a punch, holding
 * the action buttons as he stands, walks or brakes on the ground charges
 * it, and slows him toward 0.5. once it's full and the buttons are let go,
 * or he leaves the ground or those modes, he homes in on the nearest
 * target. letting go or leaving sooner ends the charge, but a punch starts
 * it over */
static void knuckles_charge_heat(struct task_work *work, struct player *record)
{
    Sint16 charge;

    if (record == NULL) {
        return;
    }
    if (!(record->upgrades & PLAYER_KNUCKLES_FIGHTING_GLOVES)) {
        CHARGE(record) = -1;
        return;
    }
    if (work->mode == KNUCKLES_PUNCH || work->mode == KNUCKLES_HOMING) {
        return;
    }
    if (CHARGE(record) < 0) {
        return;
    }
    if ((pad_records[PLAYER_VARS(work)->player].on & pad_action_buttons) &&
        (work->flags & PLAYER_ON_GROUND) &&
        (work->mode == KNUCKLES_STAND || work->mode == KNUCKLES_WALK ||
         work->mode == KNUCKLES_STOP)) {
        charge = CHARGE(record);
        if (++CHARGE(record) >= KNUCKLES_CHARGED) {
            CHARGE(record) = KNUCKLES_CHARGED;
            if (charge != KNUCKLES_CHARGED) {
                knuckles_pulse_in();
                sound_stop_id((int)work);
                sound_play(SE_CHARGED, 0, 0, 0);
            }
        } else {
            sound_play_held(SE_CHARGE, (int)work, 1, 0);
        }
        if (record->speed.x > 0.5F) {
            record->speed.x += (0.5F - record->speed.x) * 0.5F;
        }
        return;
    }
    if (CHARGE(record) == KNUCKLES_CHARGED) {
        if (stage_get_stage_act() < STAGE_ACT(STAGE_CHAOS0, 0)) {
            camera_override_mode(68, 0);
        }
        knuckles_pulse_out();
        knuckles_trail_clear();
        KNUCKLES_VARS(work)->homing_steps = HOMING_STEPS;
        work->mode = KNUCKLES_HOMING;
        record->anim.action = ACTION_HOMING;
    }
    CHARGE(record) = -1;
    sound_stop_id((int)work);
    if (work->mode != KNUCKLES_DIG_DOWN) {
        sound_play(SE_HEAT, 0, 0, 0);
    }
}

static void knuckles_destroy(struct task *task)
{
    struct task_work *work = task->work;

    player_free(PLAYER_VARS(work)->player);
    texture_release_character(CHARACTER_KNUCKLES);
}

/* draw him if he's in view, and put his body's shape at his middle. an
 * event's motions pose him while it has any. he leans as he glides, and
 * tilts to the wall's slope as he grabs it. spinning on the ground, he's
 * squashed. at full speed, or spinning on the ground, two fading copies of
 * him follow, posed two and four frames back. curled up, he flickers to
 * ACTION_BALL. while his hurt timer counts down, he's hidden two steps in
 * four */
static void knuckles_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;
    Angle tilt;
    struct collision_shape *shape;
    Float frame;
    Float frames;
    int action;
    int i;
    int timer;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    action = record->anim.action;
    if (record->anim.state == PLAYER_ANIM_BLENDING) {
        action = record->anim.current;
    }
    if (work->flags & PLAYER_BALL) {
        if (record->anim.action == ACTION_CURL) {
            if (++FLICKER(record) & 1) {
                action = ACTION_BALL;
            }
        } else if (++FLICKER(record) & 0x11) {
            action = ACTION_BALL;
        }
    }
    timer = TASK_TIMER(work);
    /* @hack redundant: the test of timer < 0, which never holds: EXTU.W at
     * 0x8c0ccd36 zero-extends the timer, and no store gives it 0x8000 or
     * more. it keeps CMP/PZ at 0x8c0ccd38; without it, count unmeasured */
    if (timer < 0 || (timer & 2)) {
        return;
    }
    njSetTexture(&knuckles_texlist);
    draw_set_palette(DRAW_PLAYER_PALETTE);
    /* @hack spelling: a bare scope for offset and pos, whose frame bytes
     * color's block below reuses (ADD #-32,R15 at 0x8c0ccc98). declared at
     * the top, they take bytes of their own, the frame grows 16 bytes, and
     * 38 of 550 words differ */
    {
        NJS_VECTOR offset;
        NJS_VECTOR pos;

        njPushMatrix(NULL);
        offset.x = offset.z = 0.0F;
        offset.y = record->consts.height * 0.5F;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, -work->ang.y);
        njCalcVector(NULL, &offset, &pos);
        njPopMatrix(1);
        pos.x += work->pos.x;
        pos.y += work->pos.y;
        pos.z += work->pos.z;
        if (work->mode == KNUCKLES_DIG_DOWN || work->mode == KNUCKLES_DIG_UP) {
            pos.x += KNUCKLES_VARS(work)->depth * record->contact->hit.normal.x;
            pos.y += KNUCKLES_VARS(work)->depth * record->contact->hit.normal.y;
            pos.z += KNUCKLES_VARS(work)->depth * record->contact->hit.normal.z;
        }
        njTranslateV(NULL, &pos);
        shape = work->collision->shapes;
        shape->center.x = pos.x;
        shape->center.y = pos.y;
        shape->center.z = pos.z;
        switch (action) {
        case ACTION_GLIDE:
            LEAN(record) = angle_approach(
                LEAN(record), -angle_difference(work->ang.y, motion->aim.y),
                0x800);
            TURN(njRotateY, 0x8000 - work->ang.y);
            /* @hack type: the lean cast to an Angle, which SHC spills as a
             * long and passes from its register (MOV.L R2,@R15 at
             * 0x8c0ccf00, MOV R2,R5 at 0x8c0ccf08). uncast, it spills a word
             * and reloads it, and 15 of 550 words differ */
            TURN(njRotateX, (Angle)LEAN(record));
            break;
        case ACTION_GRAB_WALL:
            frame =
                1.0F -
                (record->anim.frame /
                 knuckles_actions[record->anim.action].action->motion->nbFrame);
            /* @bug logic: the draw eases out his grip count, not his lean.
             * GRIP_STEPS shares LEAN's word, which his climb sets to
             * GRIP_HOLD_STEPS or counts down (MOV #6,R3 at 0x8c0cb580,
             * MOV.W R2,@(R0,R3) at 0x8c0cb542). once a climb step has run in
             * ACTION_GRAB_WALL, the word read here (MOV.W @(R0,R14),R3 at
             * 0x8c0ccf6c) holds that count */
            tilt = LEAN(record) * frame;
            TURN(njRotateY, 0x8000 - work->ang.y);
            TURN(njRotateX, tilt);
            /* @hack spelling: TURN written out, the wall's slope tested
             * != 0, which puts the second acosf's address in R2 and the
             * collision in R3 (0x8c0ccfbc, 0x8c0ccfbe). through TURN, R3 and
             * R1 take them, and 4 of 550 words differ */
            -(Angle)(acosf(record->contact->hit.normal.y) * ANGLE_PER_RADIAN) !=
                    0
                ? njRotateZ(NULL,
                            -(Angle)(acosf(record->contact->hit.normal.y) *
                                     ANGLE_PER_RADIAN))
                : (void)0;
            break;
        default:
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
            TURN(njRotateY, 0x8000 - work->ang.y);
            break;
        }
        if (action == ACTION_SPIN && (work->flags & PLAYER_ON_GROUND)) {
            offset.x = 0.0F;
            offset.y = -1.0F;
            offset.z = 0.0F;
            njTranslateV(NULL, &offset);
            njRotateZ(NULL, 0x2000);
            offset.x = 0.7F;
            offset.y = 1.1F;
            offset.z = 0.8F;
            njScaleV(NULL, &offset);
        }
    }
    if (work->event->motions != NULL) {
        event_motions_draw(work);
    } else if (record->anim.state != PLAYER_ANIM_BLENDING) {
        draw_action(record->anim.actions[action].action, record->anim.frame);
        if (action == ACTION_FULL_SPEED ||
            (action == ACTION_SPIN && (work->flags & PLAYER_ON_GROUND))) {
            NJS_ARGB color;

            frame = record->anim.frame;
            frames = record->anim.actions[record->anim.action]
                         .action->motion->nbFrame;
            color.r = 0.0F;
            color.g = 0.0F;
            color.b = 0.0F;
            for (i = 0; i < 2; i++) {
                frame -= 2.0F;
                if (frame < 0.0F) {
                    frame += frames;
                }
                color.a = -((i + 1) * 85.333333F);
                njSetConstantMaterial(&color);
                draw_action(record->anim.actions[action].action, frame);
            }
            color.a = 0.0F;
            njSetConstantMaterial(&color);
        }
    } else {
        draw_action(record->anim.blend, record->anim.frame);
    }
    njPopMatrix(1);
    draw_set_palette(0);
}

/* his hands on both models, by his upgrades. the fighting gloves change
 * all four parts of each hand. the shovel claws change his fingers, and the
 * vertices their welds join */
static void knuckles_set_hands(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    /* @hack spelling: each model's hand welds through a pointer to the first,
     * whose address SHC forms once (ADD R4,R5 at 0x8c0cd24e, ADD R3,R4 at
     * 0x8c0cd254). as knuckles_welds[10], [11], [22] and [23], the code
     * grows, and 224 of 227 words differ */
    struct weld *hands = &knuckles_welds[10];
    struct weld *b_hands = &knuckles_welds[22];

    switch (record->upgrades &
            (PLAYER_KNUCKLES_FIGHTING_GLOVES | PLAYER_KNUCKLES_SHOVEL_CLAWS)) {
    case 0:
        knuckles_cuff_plus_z[0].model = knuckles_cuff_plus_z_model;
        knuckles_hand_plus_z[0].model = knuckles_hand_plus_z_model;
        knuckles_fingers_plus_z[0].model = knuckles_fingers_plus_z_model;
        knuckles_thumb_plus_z[0].model = knuckles_thumb_plus_z_model;
        knuckles_cuff_minus_z[0].model = knuckles_cuff_minus_z_model;
        knuckles_hand_minus_z[0].model = knuckles_hand_minus_z_model;
        knuckles_fingers_minus_z[0].model = knuckles_fingers_minus_z_model;
        knuckles_thumb_minus_z[0].model = knuckles_thumb_minus_z_model;
        knuckles_b_cuff_plus_z[0].model = knuckles_cuff_plus_z_model;
        knuckles_b_hand_plus_z[0].model = knuckles_hand_plus_z_model;
        knuckles_b_fingers_plus_z[0].model = knuckles_fingers_plus_z_model;
        knuckles_b_thumb_plus_z[0].model = knuckles_thumb_plus_z_model;
        knuckles_b_cuff_minus_z[0].model = knuckles_cuff_minus_z_model;
        knuckles_b_hand_minus_z[0].model = knuckles_hand_minus_z_model;
        knuckles_b_fingers_minus_z[0].model = knuckles_fingers_minus_z_model;
        knuckles_b_thumb_minus_z[0].model = knuckles_thumb_minus_z_model;
        hands[0].indices = hands[1].indices = b_hands[0].indices =
            b_hands[1].indices = knuckles_finger_vertices;
        break;
    case PLAYER_KNUCKLES_SHOVEL_CLAWS:
        knuckles_cuff_plus_z[0].model = knuckles_cuff_plus_z_model;
        knuckles_hand_plus_z[0].model = knuckles_hand_plus_z_model;
        knuckles_fingers_plus_z[0].model = knuckles_claw_fingers_plus_z_model;
        knuckles_thumb_plus_z[0].model = knuckles_thumb_plus_z_model;
        knuckles_cuff_minus_z[0].model = knuckles_cuff_minus_z_model;
        knuckles_hand_minus_z[0].model = knuckles_hand_minus_z_model;
        knuckles_fingers_minus_z[0].model = knuckles_claw_fingers_minus_z_model;
        knuckles_thumb_minus_z[0].model = knuckles_thumb_minus_z_model;
        knuckles_b_cuff_plus_z[0].model = knuckles_cuff_plus_z_model;
        knuckles_b_hand_plus_z[0].model = knuckles_hand_plus_z_model;
        knuckles_b_fingers_plus_z[0].model = knuckles_claw_fingers_plus_z_model;
        knuckles_b_thumb_plus_z[0].model = knuckles_thumb_plus_z_model;
        knuckles_b_cuff_minus_z[0].model = knuckles_cuff_minus_z_model;
        knuckles_b_hand_minus_z[0].model = knuckles_hand_minus_z_model;
        knuckles_b_fingers_minus_z[0].model =
            knuckles_claw_fingers_minus_z_model;
        knuckles_b_thumb_minus_z[0].model = knuckles_thumb_minus_z_model;
        hands[0].indices = hands[1].indices = b_hands[0].indices =
            b_hands[1].indices = knuckles_claw_vertices;
        break;
    case PLAYER_KNUCKLES_FIGHTING_GLOVES:
        knuckles_cuff_plus_z[0].model = knuckles_glove_cuff_plus_z_model;
        knuckles_hand_plus_z[0].model = knuckles_glove_hand_plus_z_model;
        knuckles_fingers_plus_z[0].model = knuckles_glove_fingers_plus_z_model;
        knuckles_thumb_plus_z[0].model = knuckles_glove_thumb_plus_z_model;
        knuckles_cuff_minus_z[0].model = knuckles_glove_cuff_minus_z_model;
        knuckles_hand_minus_z[0].model = knuckles_glove_hand_minus_z_model;
        knuckles_fingers_minus_z[0].model =
            knuckles_glove_fingers_minus_z_model;
        knuckles_thumb_minus_z[0].model = knuckles_glove_thumb_minus_z_model;
        knuckles_b_cuff_plus_z[0].model = knuckles_glove_cuff_plus_z_model;
        knuckles_b_hand_plus_z[0].model = knuckles_glove_hand_plus_z_model;
        knuckles_b_fingers_plus_z[0].model =
            knuckles_glove_fingers_plus_z_model;
        knuckles_b_thumb_plus_z[0].model = knuckles_glove_thumb_plus_z_model;
        knuckles_b_cuff_minus_z[0].model = knuckles_glove_cuff_minus_z_model;
        knuckles_b_hand_minus_z[0].model = knuckles_glove_hand_minus_z_model;
        knuckles_b_fingers_minus_z[0].model =
            knuckles_glove_fingers_minus_z_model;
        knuckles_b_thumb_minus_z[0].model = knuckles_glove_thumb_minus_z_model;
        hands[0].indices = hands[1].indices = b_hands[0].indices =
            b_hands[1].indices = knuckles_finger_vertices;
        break;
    case PLAYER_KNUCKLES_FIGHTING_GLOVES | PLAYER_KNUCKLES_SHOVEL_CLAWS:
        knuckles_cuff_plus_z[0].model = knuckles_glove_cuff_plus_z_model;
        knuckles_hand_plus_z[0].model = knuckles_glove_hand_plus_z_model;
        knuckles_fingers_plus_z[0].model =
            knuckles_glove_claw_fingers_plus_z_model;
        knuckles_thumb_plus_z[0].model = knuckles_glove_thumb_plus_z_model;
        knuckles_cuff_minus_z[0].model = knuckles_glove_cuff_minus_z_model;
        knuckles_hand_minus_z[0].model = knuckles_glove_hand_minus_z_model;
        knuckles_fingers_minus_z[0].model =
            knuckles_glove_claw_fingers_minus_z_model;
        knuckles_thumb_minus_z[0].model = knuckles_glove_thumb_minus_z_model;
        knuckles_b_cuff_plus_z[0].model = knuckles_glove_cuff_plus_z_model;
        knuckles_b_hand_plus_z[0].model = knuckles_glove_hand_plus_z_model;
        knuckles_b_fingers_plus_z[0].model =
            knuckles_glove_claw_fingers_plus_z_model;
        knuckles_b_thumb_plus_z[0].model = knuckles_glove_thumb_plus_z_model;
        knuckles_b_cuff_minus_z[0].model = knuckles_glove_cuff_minus_z_model;
        knuckles_b_hand_minus_z[0].model = knuckles_glove_hand_minus_z_model;
        knuckles_b_fingers_minus_z[0].model =
            knuckles_glove_claw_fingers_minus_z_model;
        knuckles_b_thumb_minus_z[0].model = knuckles_glove_thumb_minus_z_model;
        hands[0].indices = hands[1].indices = b_hands[0].indices =
            b_hands[1].indices = knuckles_claw_vertices;
        break;
    }
}

static void knuckles_stand(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    work->mode = KNUCKLES_STAND;
    record->idle_steps = 0;
    knuckles_set_stand_action(work, motion, record);
}

/* his action as he stands, by the slope of the ground. within 25 degrees
 * of flat, he stands, and after FIDGET_STEPS he fidgets. on a slope of 25
 * to 45 degrees without WORLD_NO_SLOPE, he takes ACTION_STAND_UPHILL or
 * ACTION_STAND_DOWNHILL by whether he faces up the slope or down it. on
 * steeper ground, or ground with WORLD_NO_SLOPE, his action stays as it
 * is. ACTION_UNCURL plays to its end */
static void knuckles_set_stand_action(struct task_work *work,
                                      struct task_motion *motion,
                                      struct player *record)
{
    NJS_PLANE plane;
    NJS_VECTOR front;
    NJS_VECTOR downhill;
    NJS_VECTOR ahead;
    Float slope;

    if (record->anim.action == ACTION_UNCURL) {
        return;
    }
    slope = -njInnerProduct(&world_gravity, &record->ground_normal);
    if (slope > njCos(NJM_DEG_ANG(25))) {
        if (record->anim.action != ACTION_STAND &&
            record->anim.action != ACTION_LAND &&
            record->anim.action != ACTION_STAND_LANDED &&
            record->anim.action != ACTION_STAND_ONCE &&
            record->anim.action != ACTION_FIDGET_START &&
            record->anim.action != ACTION_FIDGET &&
            record->anim.action != ACTION_FIDGET_END) {
            record->anim.action = ACTION_STAND;
        } else if (record->idle_steps > FIDGET_STEPS) {
            record->idle_steps = 0;
            record->anim.action = ACTION_FIDGET_START;
        }
    } else if (!(record->ground_flags & WORLD_NO_SLOPE) &&
               slope > njCos(NJM_DEG_ANG(45))) {
        plane.vx = record->ground_normal.x;
        plane.vy = record->ground_normal.y;
        plane.vz = record->ground_normal.z;
        plane.px = plane.py = plane.pz = 0.0F;
        front.x = 1.0F;
        front.y = front.z = 0.0F;
        player_turn_to_world(work, &front);
        njDistanceP2PL(&front, &plane, &ahead);
        /* @hack spelling: the stores through &ahead, whose address SHC forms
         * again for each (ADD #4 at 0x8c0cd61c, 0x8c0cd622 and 0x8c0cd62c).
         * as ahead.x, ahead.y and ahead.z, 45 of 205 words differ */
        if (njScalor(&ahead) == 0.0F) {
            (&ahead)->x = 1.0F;
            (&ahead)->y = (&ahead)->z = 0.0F;
        } else {
            njUnitVector(&ahead);
        }
        njDistanceP2PL(&world_gravity, &plane, &downhill);
        /* @hack spelling: the stores through &downhill, whose address SHC forms
         * again for each (ADD #16 at 0x8c0cd65a, 0x8c0cd660 and 0x8c0cd66a).
         * as downhill.x, downhill.y and downhill.z, 13 of 205 words differ */
        if (njScalor(&downhill) == 0.0F) {
            (&downhill)->x = 1.0F;
            (&downhill)->y = (&downhill)->z = 0.0F;
        } else {
            njUnitVector(&downhill);
        }
        {
            int ang = vector_angle(&ahead, &downhill, &record->ground_normal);

            if (ang > 0xc000 || ang < 0x4000) {
                record->anim.action = ACTION_STAND_DOWNHILL;
            } else {
                record->anim.action = ACTION_STAND_UPHILL;
            }
        }
    }
    record->idle_steps++;
}

static void knuckles_set_walk_action(struct task_work *work,
                                     struct task_motion *motion,
                                     struct player *record)
{
    Float speed = record->speed.x + (motion->slip * 5.0F);

    if (record->anim.action == ACTION_UNCURL ||
        record->anim.action == ACTION_TURN ||
        record->anim.action == ACTION_HOLD_TURN) {
        return;
    }
    if (work->flags & PLAYER_HOLDING) {
        if (speed > record->consts.running_speed) {
            record->anim.action = ACTION_HOLD_RUN;
        } else {
            record->anim.action = ACTION_HOLD_WALK;
        }
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

/* he hangs record->consts.height below the path. while he moves along it,
 * ACTION_HANG_GO runs round its cycle, forward or back with him. with the
 * stick let go, he stops once the cycle reaches its frame 0 or half way
 * round, in ACTION_HANG_AT_0 or ACTION_HANG_AT_32 */
static void knuckles_hang_move(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    NJS_VECTOR up;

    up.x = 0.0F;
    up.y = record->consts.height;
    up.z = 0.0F;
    player_turn_to_world(work, &up);
    njSubVector(&work->pos, &up);

    if (record->speed.x != 0.0F && motion->drive == 0.0F) {
        if (record->anim.frame == 0.0F ||
            record->anim.frame >= HANG_FRAMES / 2) {
            work->mode = KNUCKLES_HANG;
            if (record->anim.frame == 0.0F) {
                record->anim.action = ACTION_HANG_AT_0;
            } else {
                record->anim.action = ACTION_HANG_AT_32;
            }
            record->speed.x = 0.0F;
        } else {
            work->mode = KNUCKLES_HANG;
            record->speed.x = 0.0F;
            if (record->anim.frame >= HANG_FRAMES / 4 &&
                record->anim.frame <= HANG_FRAMES * 3 / 4) {
                record->anim.action = ACTION_HANG_AT_32;
            } else {
                record->anim.action = ACTION_HANG_AT_0;
            }
        }
    } else if (record->speed.x > 0.0F) {
        if (record->anim.frame >= 0.0F &&
            record->anim.frame < HANG_FRAMES / 2) {
            record->anim.frame += HANG_PACE;
            if (record->anim.frame == HANG_FRAMES / 2 &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = KNUCKLES_HANG;
                record->anim.action = ACTION_HANG_AT_32;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame += HANG_PACE;
            if (record->anim.frame >= HANG_FRAMES - 1.0F) {
                record->anim.frame = 0.0F;
            }
            if (record->anim.frame == 0.0F &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = KNUCKLES_HANG;
                record->anim.action = ACTION_HANG_AT_0;
                record->speed.x = 0.0F;
            }
        }
    } else if (record->speed.x < 0.0F) {
        if (record->anim.frame > 0.0F &&
            record->anim.frame <= HANG_FRAMES / 2) {
            record->anim.frame -= HANG_PACE;
            if (record->anim.frame == 0.0F &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = KNUCKLES_HANG;
                record->anim.action = ACTION_HANG_AT_0;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame -= HANG_PACE;
            if (record->anim.frame < 0.0F) {
                record->anim.frame = HANG_FRAMES - HANG_PACE;
            }
            if (record->anim.frame == HANG_FRAMES / 2 &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = KNUCKLES_HANG;
                record->anim.action = ACTION_HANG_AT_32;
                record->speed.x = 0.0F;
            }
        }
    } else {
        record->speed.x = 0.0F;
    }
}

/* as knuckles_hang_move, but along a ledge's path, round ACTION_LEDGE's
 * cycle. with the stick let go, he stops once the cycle reaches half way
 * round or a step short of its end, and keeps ACTION_LEDGE */
static void knuckles_ledge_move(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record)
{
    NJS_VECTOR up;

    up.x = 0.0F;
    up.y = record->consts.height;
    up.z = 0.0F;
    player_turn_to_world(work, &up);
    njSubVector(&work->pos, &up);

    if (record->speed.x != 0.0F && motion->drive == 0.0F) {
        if (record->anim.frame == LEDGE_FRAMES / 2 ||
            record->anim.frame == LEDGE_FRAMES - HANG_PACE) {
            work->mode = KNUCKLES_LEDGE;
            record->speed.x = 0.0F;
        }
    } else if (record->speed.x > 0.0F) {
        if (record->anim.frame >= 0.0F &&
            record->anim.frame < LEDGE_FRAMES / 2) {
            record->anim.frame += HANG_PACE;
            if (record->anim.frame == LEDGE_FRAMES / 2 &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = KNUCKLES_LEDGE;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame += HANG_PACE;
            if (record->anim.frame >= LEDGE_FRAMES) {
                record->anim.frame = 0.0F;
            }
            if (record->anim.frame == LEDGE_FRAMES - HANG_PACE &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = KNUCKLES_LEDGE;
                record->speed.x = 0.0F;
            }
        }
    } else if (record->speed.x < 0.0F) {
        if (record->anim.frame > LEDGE_FRAMES / 2 &&
            record->anim.frame <= LEDGE_FRAMES) {
            record->anim.frame -= HANG_PACE;
            if (record->anim.frame == LEDGE_FRAMES / 2 &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = KNUCKLES_LEDGE;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame -= HANG_PACE;
            if (record->anim.frame < 0.0F) {
                record->anim.frame = LEDGE_FRAMES - HANG_PACE;
            }
            if (record->anim.frame == LEDGE_FRAMES - HANG_PACE &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = KNUCKLES_LEDGE;
                record->speed.x = 0.0F;
            }
        }
    } else {
        record->speed.x = 0.0F;
    }
}

/* in the set editor, his pad's stick moves him across the camera's view,
 * or up and down while player 0 holds X. player 0's B puts him at the act's
 * start */
static void knuckles_edit(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    Float x;
    Float y;
    Float sign;
    Angle ang;

    if (pad_records[0].press & PDD_DGT_TB) {
        stage_place_player(work);
    }
    if (pad_records[0].on & PDD_DGT_TX) {
        x = (Float)(pad_records[PLAYER_VARS(work)->player].x1 << 8);
        y = (Float)(pad_records[PLAYER_VARS(work)->player].y1 << 8);
        if (x > PAD_DEAD_ZONE || x < -PAD_DEAD_ZONE || y > PAD_DEAD_ZONE ||
            y < -PAD_DEAD_ZONE) {
            sign = y != 0.0F ? y / njSqrt(y * y) : 0.0F;
            work->pos.y -= sign * PLAYER_EDIT_SPEED;
        }
        return;
    }
    x = (Float)(pad_records[PLAYER_VARS(work)->player].x1 << 8);
    y = (Float)(pad_records[PLAYER_VARS(work)->player].y1 << 8);
    if (x > PAD_DEAD_ZONE || x < -PAD_DEAD_ZONE || y > PAD_DEAD_ZONE ||
        y < -PAD_DEAD_ZONE) {
        ang = (Angle)(atan2f(y, x) * ANGLE_PER_RADIAN) - camera_work->ang.y;
        work->pos.x += njCos(ang) * PLAYER_EDIT_SPEED;
        work->pos.z += njSin(ang) * PLAYER_EDIT_SPEED;
    }
}

static void knuckles_glide_start(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record)
{
    if (record->speed.x <= record->consts.running_speed) {
        record->speed.x = record->consts.running_speed;
    }
    record->speed.y = 0.0F;
    LEAN(record) = 0;
    sound_play(SE_GLIDE_START, 0, 0, 0);
}

/* a hit, or ground that hurts, knocks him back into KNUCKLES_HURT, unless a
 * module poses him. TRUE once it does, and FALSE otherwise */
static Bool knuckles_try_hurt(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    if (record != NULL &&
        ((work->flags & PLAYER_HURT) || (record->ground_flags & WORLD_HURTS)) &&
        work->mode != KNUCKLES_MODULE_POSE) {
        work->mode = KNUCKLES_HURT;
        record->anim.action = ACTION_HURT;
        work->flags &= ~PLAYER_HURT;
        record->speed.y = record->consts.jump_speed;
        record->speed.x = -record->consts.running_speed;
        work->timer = PLAYER_HURT_STEPS;
        vibration_start_fading(PLAYER_VARS(work)->player, 7, 59, 6);
        return TRUE;
    }
    return FALSE;
}

/* at a press of the jump button, he jumps. on a panel, he jumps to the
 * next. holding something he can carry, he jumps with it. otherwise he lets
 * go of anything he holds and jumps curled up, with the speed of the task
 * he stands on. TRUE once he jumps, and FALSE otherwise */
static Bool knuckles_try_jump(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    int flags;

    /* @hack redundant: the else before the second test, after an arm that
     * returns, whose jump puts player_jump_pressed's address in R2, not R3
     * (0x8c0cdf1c); without it, 2 of 144 words differ */
    if (player_jump_pressed(work, record) == PLAYER_JUMP_PANEL) {
        if (work->flags & PLAYER_HOLDING) {
            knuckles_let_go(work, motion, record);
        }
        panel_jump(work);
        work->mode = KNUCKLES_PANEL_JUMP;
        record->anim.action = ACTION_PANEL_JUMP;
        work->flags &= ~PLAYER_ON_TASK;
        record->jump_steps = 0;
        sound_play(SE_JUMP, 0, 0, 0);
        return TRUE;
    } else if (player_jump_pressed(work, record)) {
        if (work->flags & PLAYER_HOLDING) {
            flags = player_carry_flags(work);
            /* @hack redundant: the else after the arm that returns, whose jump
             * puts knuckles_let_go's address in R2, not R3 (0x8c0cdfa0);
             * without it, 6 of 144 words differ */
            if (flags & PLAYER_CAN_CARRY) {
                work->mode = KNUCKLES_HOLD_JUMP;
                record->speed.y = record->consts.jump_speed;
                record->anim.action = ACTION_HOLD_JUMP;
                work->flags &= ~PLAYER_ON_TASK;
                record->jump_steps = 0;
                sound_play(SE_JUMP, 0, 0, 0);
                return TRUE;
            } else {
                knuckles_let_go(work, motion, record);
            }
        }
        work->mode = KNUCKLES_JUMP;
        if (record->ground_task != NULL) {
            struct task_force *ground =
                (struct task_force *)record->ground_task->force;

            if (ground != NULL) {
                player_set_accel(PLAYER_VARS(work)->player, ground->speed.x,
                                 ground->speed.y, ground->speed.z);
            }
        }
        record->speed.y = record->consts.jump_speed;
        record->anim.action = ACTION_SPIN;
        record->action_speed = 2.0F;
        work->flags &= ~PLAYER_ON_TASK;
        work->flags |= PLAYER_ATTACKING;
        record->jump_steps = 0;
        sound_play(SE_JUMP, 0, 0, 0);
        return TRUE;
    }
    return FALSE;
}

/* at a press of the action buttons, he uncurls and falls. TRUE once he
 * does, and FALSE otherwise */
static Bool knuckles_try_uncurl(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record)
{
    if (pad_records[PLAYER_VARS(work)->player].press & pad_action_buttons) {
        work->mode = KNUCKLES_FALL;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        record->anim.action = ACTION_FALL;
        record->action_speed = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        return TRUE;
    }
    return FALSE;
}

/* at a press of the jump button as he climbs, he leaps off the wall curled
 * up, with his back to it. TRUE once he does, and FALSE otherwise */
static Bool knuckles_try_wall_jump(struct task_work *work,
                                   struct player *record)
{
    NJS_VECTOR up;

    if (player_jump_pressed(work, record)) {
        work->mode = KNUCKLES_JUMP;
        record->speed.x = record->consts.jump_speed * 0.5325F;
        record->speed.y = record->consts.jump_speed * 0.5325F;
        record->speed.z = 0.0F;
        up.x = up.z = 0.0F;
        up.y = 1.0F;
        player_turn_to_world(work, &up);
        work->pos.x += up.x * 5.0F;
        work->pos.y += up.y * 5.0F;
        work->pos.z += up.z * 5.0F;
        work->ang.x = 0;
        work->ang.y = (Angle)(atan2f(up.z, up.x) * ANGLE_PER_RADIAN);
        work->ang.z = 0;
        up.x = up.z = 0.0F;
        up.y = 1.0F;
        player_turn_to_world(work, &up);
        work->pos.x -= up.x * 5.0F;
        work->pos.y -= up.y * 5.0F;
        work->pos.z -= up.z * 5.0F;
        record->anim.action = ACTION_SPIN;
        record->action_speed = 2.0F;
        work->flags |= PLAYER_ATTACKING;
        record->jump_steps = 0;
        sound_play(SE_JUMP, 0, 0, 0);
        return TRUE;
    }
    return FALSE;
}

/* follow an event's action. EVENT_ACTION_RUN, EVENT_ACTION_MOVE,
 * EVENT_ACTION_PUSH and EVENT_ACTION_DONE mark him PLAYER_EVENT_DRIVEN, and
 * the two turns set his heading. TRUE for those, and for any other action
 * FALSE, with the mark cleared */
static Bool knuckles_follow_event(struct task_work *work,
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
                           event->move.ang.y + work->ang.y);
        break;
    default:
        work->flags &= ~PLAYER_EVENT_DRIVEN;
        return FALSE;
    }
    return TRUE;
}

/* answer an event's action, then its paths, then a PLAYER_REQUEST submode.
 * TRUE once it answers the paths or the submode, and the caller then checks
 * nothing more. FALSE otherwise, even after it follows an event's action or
 * puts him on a path for PLAYER_MODE_JOIN_PATH */
static Bool knuckles_try_request(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record)
{
    if (work->event->action) {
        knuckles_follow_event(work, motion, record);
        return FALSE;
    }
    if (work->event->paths != NULL) {
        work->mode = KNUCKLES_EVENT;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        return TRUE;
    }
    if (!(work->flags & PLAYER_REQUEST)) {
        return FALSE;
    }
    switch (work->submode) {
    case PLAYER_MODE_SPRING:
    case PLAYER_MODE_BOUNCE_3:
        work->mode = KNUCKLES_SPRING;
        record->anim.action = ACTION_SPRING;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_BOUNCE_1:
        work->mode = KNUCKLES_SPRING;
        record->anim.action = ACTION_BOUNCE_1;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_BOUNCE_2:
        work->mode = KNUCKLES_SPRING;
        record->anim.action = ACTION_BOUNCE_2;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_LAUNCH:
        work->mode = KNUCKLES_LAUNCH;
        record->anim.action = ACTION_SPRING;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_UPDRAFT:
        work->mode = KNUCKLES_FALL;
        record->anim.action = ACTION_UPDRAFT;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_PATH:
        work->mode = KNUCKLES_PATH;
        record->anim.action = ACTION_RUN;
        record->path_distance = 0.0F;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL:
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_STAND;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_BALL:
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_SPIN;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags |= PLAYER_ATTACKING;
        record->action_speed = 2.0F;
        sound_play(SE_SPIN, 0, 0, 0);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL_PUSH:
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_PUSH_START;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_HANG_ON:
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_HANG_ON;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_GET_ON:
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_FALL;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RIDE:
        work->mode = KNUCKLES_MODULE_POSE;
        record->anim.action = ACTION_RIDE;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_CLEAR:
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_CLEAR;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_ROCKET:
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_ROCKET_START;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_ROCKET_ALONG:
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_ROCKET_ALONG;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_ROCKET_UP:
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_ROCKET_UP;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_SQUASH_2:
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_SQUASH_2;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_SQUASH_3:
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_SQUASH_3;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RESET:
        knuckles_stand(work, motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_JOIN_PATH:
        if (work->mode == KNUCKLES_WALK ||
            record->speed.x > record->consts.running_speed) {
            record->path_distance = 0.0F;
            if (work->flags & PLAYER_BALL) {
                work->flags &= ~PLAYER_ATTACKING;
            }
            work->flags |= PLAYER_ON_PATH;
        }
        return FALSE;
    case PLAYER_MODE_HANG:
        work->mode = KNUCKLES_HANG;
        record->anim.action = ACTION_HANG_GO;
        record->anim.frame = 16.0F;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_LEDGE:
        if (work->flags & PLAYER_ON_GROUND) {
            return FALSE;
        }
        work->mode = KNUCKLES_LEDGE;
        record->anim.action = ACTION_LEDGE;
        record->anim.frame = 0.0F;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_FREEZE:
        if (work->mode != KNUCKLES_ICED) {
            work->mode = KNUCKLES_ICED;
            record->anim.action = ACTION_ICED;
            STRUGGLE(record) = ICE_STRUGGLE;
            ice_create(PLAYER_VARS(work)->player);
            player_clear_speed(motion, record);
            if (work->flags & PLAYER_BALL) {
                work->flags &= ~PLAYER_ATTACKING;
            }
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case PLAYER_MODE_KNOCK_BACK:
        work->mode = KNUCKLES_CRASH;
        record->anim.action = ACTION_CRASH;
        sound_play(SE_CRASH, 0, 0, 0);
        player_crash_burst(work);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_HOLD:
        work->mode = KNUCKLES_HOLD_STAND;
        record->anim.action = ACTION_HOLD_STAND;
        record->held = PLAYER_VARS(work)->arg.carry;
        record->held->work->flags |= CARRY_HELD;
        work->flags |= PLAYER_HOLDING;
        work->flags &= ~COLLISION_TOUCHING;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_FLY:
        work->mode = KNUCKLES_GLIDE;
        record->anim.action = ACTION_GLIDE;
        knuckles_glide_start(work, motion, record);
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_GUARD;
        record->own.knuckles.glide.bumps = 0;
        break;
    case PLAYER_MODE_DROWN:
        work->mode = KNUCKLES_RISE;
        record->anim.action = ACTION_DROWN;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_DOWN:
        work->mode = KNUCKLES_DOWN;
        record->anim.action = ACTION_DOWN;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/* knuckles_try_request while he holds something, but with no test of an
 * event's paths. it answers neither PLAYER_MODE_ROCKET_ALONG nor
 * PLAYER_MODE_ROCKET_UP, and returns FALSE for them. a spring, a bounce, a
 * launch, an updraft or PLAYER_MODE_RESET puts him in a HOLD_ mode with
 * it. he lets go of it first for PLAYER_MODE_PATH,
 * PLAYER_MODE_BALL, PLAYER_MODE_STILL_PUSH, PLAYER_MODE_CLEAR,
 * PLAYER_MODE_ROCKET, PLAYER_MODE_HANG, PLAYER_MODE_FREEZE,
 * PLAYER_MODE_KNOCK_BACK, PLAYER_MODE_DROWN and PLAYER_MODE_DOWN. for
 * every other submode, it returns TRUE and FALSE as knuckles_try_request
 * does */
static Bool knuckles_try_request_holding(struct task_work *work,
                                         struct task_motion *motion,
                                         struct player *record)
{
    if (work->event->action) {
        knuckles_follow_event(work, motion, record);
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
        work->mode = KNUCKLES_HOLD_SPRING;
        record->anim.action = ACTION_HOLD_JUMP;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_LAUNCH:
        work->mode = KNUCKLES_HOLD_LAUNCH;
        record->anim.action = ACTION_HOLD_JUMP;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_UPDRAFT:
        work->mode = KNUCKLES_HOLD_FALL;
        record->anim.action = ACTION_HOLD_FALL;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_PATH:
        knuckles_let_go(work, motion, record);
        work->mode = KNUCKLES_PATH;
        record->anim.action = ACTION_RUN;
        record->path_distance = 0.0F;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL:
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_HOLD_STAND;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_BALL:
        knuckles_let_go(work, motion, record);
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_SPIN;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags |= PLAYER_ATTACKING;
        record->action_speed = 2.0F;
        sound_play(SE_SPIN, 0, 0, 0);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL_PUSH:
        knuckles_let_go(work, motion, record);
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_PUSH_START;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_HANG_ON:
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_HANG_ON;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_GET_ON:
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_FALL;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RIDE:
        work->mode = KNUCKLES_MODULE_POSE;
        record->anim.action = ACTION_RIDE;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_CLEAR:
        knuckles_let_go(work, motion, record);
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_CLEAR;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_ROCKET:
        knuckles_let_go(work, motion, record);
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_ROCKET_START;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_SQUASH_2:
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_HOLD_STAND;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_SQUASH_3:
        work->mode = KNUCKLES_POSE;
        record->anim.action = ACTION_HOLD_STAND;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RESET:
        work->mode = KNUCKLES_HOLD_STAND;
        record->anim.action = ACTION_HOLD_STAND;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_JOIN_PATH:
        if (work->mode == KNUCKLES_HOLD_WALK ||
            record->speed.x > record->consts.running_speed) {
            record->path_distance = 0.0F;
            work->flags |= PLAYER_ON_PATH;
        }
        return FALSE;
    case PLAYER_MODE_HANG:
        knuckles_let_go(work, motion, record);
        work->mode = KNUCKLES_HANG;
        record->anim.action = ACTION_HANG_GO;
        record->anim.frame = 16.0F;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_LEDGE:
        if (work->flags & PLAYER_ON_GROUND) {
            return FALSE;
        }
        work->mode = KNUCKLES_LEDGE;
        record->anim.action = ACTION_LEDGE;
        record->anim.frame = 0.0F;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_FREEZE:
        if (work->mode != KNUCKLES_ICED) {
            knuckles_let_go(work, motion, record);
            work->mode = KNUCKLES_ICED;
            record->anim.action = ACTION_ICED;
            STRUGGLE(record) = ICE_STRUGGLE;
            player_clear_speed(motion, record);
            ice_create(PLAYER_VARS(work)->player);
            if (work->flags & PLAYER_BALL) {
                work->flags &= ~PLAYER_ATTACKING;
            }
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case PLAYER_MODE_KNOCK_BACK:
        knuckles_let_go(work, motion, record);
        work->mode = KNUCKLES_CRASH;
        record->anim.action = ACTION_CRASH;
        sound_play(SE_CRASH, 0, 0, 0);
        player_crash_burst(work);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_HOLD:
        work->mode = KNUCKLES_HOLD_STAND;
        record->anim.action = ACTION_HOLD_STAND;
        record->held = PLAYER_VARS(work)->arg.carry;
        record->held->work->flags |= CARRY_HELD;
        work->flags |= PLAYER_HOLDING;
        work->flags &= ~COLLISION_TOUCHING;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_FLY:
        work->mode = KNUCKLES_GLIDE;
        record->anim.action = ACTION_GLIDE;
        knuckles_glide_start(work, motion, record);
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_GUARD;
        record->own.knuckles.glide.bumps = 0;
        break;
    case PLAYER_MODE_DROWN:
        knuckles_let_go(work, motion, record);
        work->mode = KNUCKLES_RISE;
        record->anim.action = ACTION_DROWN;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_DOWN:
        knuckles_let_go(work, motion, record);
        work->mode = KNUCKLES_DOWN;
        record->anim.action = ACTION_DOWN;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/* at a press of the grab button, head on to what he touches, he grips it
 * to push or pull it. a task he can't push he lifts if he can carry it, or
 * else grips to shake it if it shakes. TRUE once he grips or lifts, and
 * FALSE otherwise */
static Bool knuckles_try_grab(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    int flags;

    if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                    pad_grab_buttons)) {
        if ((flags = player_carry_flags(work)) != 0) {
            /* @hack spelling: a switch of one case, which puts
             * PLAYER_CAN_PUSH's 0x400 in R2 (MOV.W at 0x8c0ceff6). as an if, R3
             * takes it and 2 of 73 words differ */
            switch (player_face_wall(work, motion)) {
            case PLAYER_WALL_HEAD_ON:
                if (flags & PLAYER_CAN_PUSH) {
                    work->mode = KNUCKLES_GRIP;
                    record->anim.action = ACTION_GRIP_START;
                    record->held = work->collision->hit->owner;
                    work->flags |= PLAYER_HOLDING;
                    return TRUE;
                } else if (flags & PLAYER_CAN_CARRY) {
                    work->mode = KNUCKLES_LIFT;
                    record->anim.action = ACTION_LIFT;
                    return TRUE;
                } else if (flags & PLAYER_CAN_SHAKE) {
                    work->mode = KNUCKLES_GRIP;
                    record->anim.action = ACTION_GRIP_START;
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

/* from frame 12 of the lift, he holds what he lifts if he faces it head on
 * and can carry it, or else stands. TRUE once he holds it, and FALSE
 * otherwise */
static Bool knuckles_try_take_lifted(struct task_work *work,
                                     struct task_motion *motion,
                                     struct player *record)
{
    int flags;

    if (record->anim.frame >= 12.0F) {
        if ((flags = player_carry_flags(work)) != 0) {
            /* @hack spelling: the two tests nested, which store
             * KNUCKLES_HOLD_STAND from R2 and load record->held again after
             * its store (0x8c0cf0b6, 0x8c0cf0c8). joined with &&, R3 takes
             * KNUCKLES_HOLD_STAND, the reload goes, and 21 of 61 words
             * differ */
            if (player_face_wall(work, motion) == PLAYER_WALL_HEAD_ON) {
                if (flags & PLAYER_CAN_CARRY) {
                    work->mode = KNUCKLES_HOLD_STAND;
                    record->held = work->collision->hit->owner;
                    record->held->work->flags |= CARRY_HELD;
                    work->flags |= PLAYER_HOLDING;
                    return TRUE;
                }
            }
        }
        knuckles_stand(work, motion, record);
    }
    return FALSE;
}

/* at a press of the grab button, he throws what he holds, at a target if
 * he has one. slower than a trot on the ground, he puts it down instead.
 * once PLAYER_HOLDING is gone, he stands or falls, and drops record->held.
 * TRUE for either, and FALSE otherwise */
static Bool knuckles_try_throw(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    if (pad_records[PLAYER_VARS(work)->player].press & pad_grab_buttons) {
        if (!(work->flags & PLAYER_ON_GROUND)) {
            work->mode = KNUCKLES_AIR_THROW;
            if (player_has_target(PLAYER_VARS(work)->player)) {
                record->anim.action = ACTION_AIR_THROW_AT;
            } else {
                record->anim.action = ACTION_AIR_THROW;
            }
        } else if (record->speed.x < record->consts.trot_speed) {
            work->mode = KNUCKLES_PUT_DOWN;
            record->anim.action = ACTION_PUT_DOWN;
            player_clear_speed(motion, record);
        } else {
            work->mode = KNUCKLES_THROW;
            if (player_has_target(PLAYER_VARS(work)->player)) {
                record->anim.action = ACTION_THROW_AT;
            } else {
                record->anim.action = ACTION_THROW;
            }
        }
        return TRUE;
    } else if (!(work->flags & PLAYER_HOLDING)) {
        if (work->flags & PLAYER_ON_GROUND) {
            knuckles_stand(work, motion, record);
        } else {
            work->mode = KNUCKLES_FALL;
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

/* he lets go of what he holds on his action's release frame. on frame 20
 * of ACTION_THROW_AT and frame 7 of ACTION_AIR_THROW_AT, it flies at the
 * nearest target in his near_tasks list. with that list empty, he keeps
 * it. on frame 24 of ACTION_THROW and frame 6 of ACTION_AIR_THROW, it
 * flies ahead and up. on frame 12 of ACTION_PUT_DOWN, it stays where it
 * is. holding nothing, he clears PLAYER_HOLDING. TRUE once he lets go, or
 * once he clears a PLAYER_HOLDING that was set. FALSE otherwise */
static Bool knuckles_try_release(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record)
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
    /* @hack spelling: action assigned inside the test, which SHC zero-extends
     * straight into R0 for the compare (EXTU.W R6,R0 at 0x8c0cf2e8).
     * assigned in a statement before the test, the action is extended in
     * R6, and 68 of 282 words differ */
    if ((((action = record->anim.action) == ACTION_THROW_AT &&
          record->anim.frame == 20.0F) ||
         (action == ACTION_AIR_THROW_AT && record->anim.frame == 7.0F)) &&
        near->work != NULL) {
        held = record->held->work;
        if (held != NULL) {
            held->flags &= ~CARRY_HELD;
            /* @hack spelling: the search through each, a pointer of its own,
             * which keeps the copies for njSqrt in their frame slots (FMOV.S
             * FR0,@(R0,R15) at 0x8c0cf38e, R0 4). walking near itself, the
             * copies trade slots and 62 of 282 words differ */
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
    if ((action == ACTION_THROW && record->anim.frame == 24.0F) ||
        (action == ACTION_AIR_THROW && record->anim.frame == 6.0F)) {
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
    if (record->anim.action == ACTION_PUT_DOWN && record->anim.frame == 12.0F) {
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

/* with the grab button held, he shakes what he grips if it shakes, or else
 * pushes or pulls it as the stick says. with the button let go, he lifts
 * it if he can carry it, or else lets go and stands. TRUE once he shakes,
 * pushes, pulls, lifts or lets go. FALSE while he holds the button with the
 * stick let go */
static Bool knuckles_try_grip(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    int wall;

    if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                    pad_grab_buttons)) {
        wall = player_stick_into_wall(work, motion);
        /* @hack spelling: a return in every arm, which joins the
         * PLAYER_CAN_SHAKE arm to the last arm's action store (BRA at
         * 0x8c0cf5b4). with one return after the chain, SHC joins it to the
         * head-on arm's, and 14 of 81 words differ */
        if (wall == PLAYER_WALL_NO_STICK) {
            return FALSE;
        } else if (player_carry_flags(work) & PLAYER_CAN_SHAKE) {
            work->mode = KNUCKLES_SHAKE;
            record->anim.action = ACTION_SHAKE;
            return TRUE;
        } else if (wall == PLAYER_WALL_HEAD_ON) {
            work->mode = KNUCKLES_PUSH;
            record->anim.action = ACTION_PUSH;
            return TRUE;
        } else {
            work->mode = KNUCKLES_PULL;
            record->anim.action = ACTION_PULL;
            return TRUE;
        }
    } else {
        if (player_carry_flags(work) & PLAYER_CAN_CARRY) {
            work->mode = KNUCKLES_LIFT;
            record->anim.action = ACTION_LIFT;
        } else {
            knuckles_let_go(work, motion, record);
            knuckles_stand(work, motion, record);
        }
        return TRUE;
    }
    /* @hack redundant: a return no path reaches, which the NO_STICK test's
     * return FALSE branches to (BT at 0x8c0cf5a2). without it, that test
     * returns in place, the arms reorder, and 57 of 81 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unreachable-code-return) */
    return FALSE;
}

/* he lets go of what he holds, if anything, tossing a task he can carry up
 * and ahead. record->held must be set while PLAYER_HOLDING is */
static void knuckles_let_go(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    NJS_VECTOR toss;
    struct task_work *held;

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

/* keep what he holds with him. a task he carries sits between his hands,
 * or as he throws, 2.5 out from his first hand along its axis. a task he
 * grips he lets go of once he touches another */
static void knuckles_carry(struct task_work *work, struct task_motion *motion,
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
        /* @hack redundant: the else after the arm that returns, which loads the
         * collision chain into R0, R3, R1 and R0 for the second test
         * (0x8c0cf70a to 0x8c0cf710). without it, R3, R2, R1 and R3 take
         * it, and 5 of 212 words differ */
        if (work->collision->hit == NULL) {
            return;
        } else if (held != work->collision->hit->owner->work) {
            knuckles_let_go(work, motion, record);
            if (record->held == NULL) {
                if (work->flags & PLAYER_ON_GROUND) {
                    knuckles_stand(work, motion, record);
                } else {
                    work->mode = KNUCKLES_FALL;
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

/* off the ground, curled up, he goes on jumping. otherwise he falls, with
 * what he holds if he can carry it, letting go of anything else. TRUE off
 * the ground, and FALSE on it */
static Bool knuckles_try_fall(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    if (!(work->flags & PLAYER_ON_GROUND)) {
        if (work->flags & PLAYER_BALL) {
            work->mode = KNUCKLES_JUMP;
            return TRUE;
        } else if (work->flags & PLAYER_HOLDING) {
            if (player_carry_flags(work) & PLAYER_CAN_CARRY) {
                work->mode = KNUCKLES_HOLD_FALL;
                record->anim.action = ACTION_HOLD_FALL;
                return TRUE;
            }
            knuckles_let_go(work, motion, record);
        }
        work->mode = KNUCKLES_FALL;
        record->anim.action = ACTION_FALL;
        return TRUE;
    }
    return FALSE;
}

/* stopped with the stick let go, he stands. holding something, he grips it
 * if he can push or shake it, or else stands holding it. TRUE once he
 * does, and FALSE otherwise */
static Bool knuckles_try_stop(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    if (!player_read_stick(work, NULL, NULL) && record->speed.x == 0.0F) {
        if (work->flags & PLAYER_HOLDING) {
            if (player_carry_flags(work) &
                (PLAYER_CAN_PUSH | PLAYER_CAN_SHAKE)) {
                work->mode = KNUCKLES_GRIP;
                record->anim.action = ACTION_GRIP;
            } else {
                work->mode = KNUCKLES_HOLD_STAND;
                record->anim.action = ACTION_HOLD_STAND;
            }
        } else {
            knuckles_stand(work, motion, record);
        }
        return TRUE;
    }
    return FALSE;
}

/* at a press of the action buttons, he curls up and rolls. TRUE once he
 * does, and FALSE otherwise */
static Bool knuckles_try_roll(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    if (pad_records[PLAYER_VARS(work)->player].press & pad_action_buttons) {
        work->mode = KNUCKLES_ROLL;
        record->anim.action = ACTION_ROLL;
        work->flags |= PLAYER_ATTACKING;
        record->action_speed = record->speed.x;
        return TRUE;
    }
    return FALSE;
}

/* at a press of the action buttons, he throws his first punch, and each
 * punch after it waits for another press. TRUE once he punches, and FALSE
 * otherwise */
static Bool knuckles_try_punch(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    if (pad_records[PLAYER_VARS(work)->player].press & pad_action_buttons) {
        knuckles_actions[ACTION_PUNCH_1].next = 0;
        knuckles_actions[ACTION_PUNCH_2].next = 0;
        knuckles_actions[ACTION_PUNCH_3].next = 0;
        work->mode = KNUCKLES_PUNCH;
        record->anim.action = ACTION_PUNCH_1;
        CHARGE(record) = 0;
        return TRUE;
    }
    return FALSE;
}

/* rolling slower than running_speed, he gets up: he stands if he has
 * stopped, or else walks on. TRUE once he does, and FALSE otherwise */
static Bool knuckles_try_unroll(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record)
{
    if (record->speed.x < record->consts.running_speed) {
        if (record->speed.x <= 0.0F) {
            knuckles_stand(work, motion, record);
        } else {
            work->mode = KNUCKLES_WALK;
            record->anim.action = ACTION_WALK;
        }
        return TRUE;
    }
    return FALSE;
}

/* put both heads' own models back in their places */
static void knuckles_sway_destroy(struct task *task)
{
    knuckles_head[0].model = knuckles_sway_models.own;
    knuckles_b_head[0].model = knuckles_sway_models.b_own;
}

/* sway the ends of his dreadlocks as he moves. the swayed points go into
 * the head's copy, which both heads then draw in place of their models.
 * the sway reads the head of the model his action poses, and rests while
 * his action poses neither */
static void knuckles_sway_step(struct task *task)
{
    struct task_work *work = task->work;
    struct player_sway *sway = PLAYER_SWAY(task);
    /* @hack redundant: task->extra read into a local nothing uses, which
     * weighs task enough for SHC to keep it in R10, not R9 (MOV R4,R10 at
     * 0x8c0cfbba). without it, 39 of 339 words differ */
    void *unused = task->extra;
    struct task_motion *owner_motion;
    struct player *record;
    NJS_POINT3 *from;
    NJS_POINT3 *to;
    NJS_POINT3 point;
    Float speed;
    Float tilt;
    int step;
    Uint32 i;
    Uint32 count;

    if (player_work[PLAYER_VARS(work)->player] == NULL) {
        task_destroy(task);
        return;
    }
    owner_motion = player_motion[PLAYER_VARS(work)->player];
    record = player_get(PLAYER_VARS(work)->player);
    to = knuckles_sway_models.swayed->points;
    if (knuckles_actions[record->anim.action].action->object ==
        &knuckles_object) {
        from = knuckles_sway_models.own->points;
        count = knuckles_sway_models.own->nbPoint;
    } else if (knuckles_actions[record->anim.action].action->object ==
               &knuckles_b_object) {
        from = knuckles_sway_models.b_own->points;
        count = knuckles_sway_models.b_own->nbPoint;
    } else {
        return;
    }
    switch (work->mode) {
    case SWAY_INIT:
        work->mode = SWAY_SWAY;
        task->destructor = knuckles_sway_destroy;
        break;
    case SWAY_SWAY:
        speed = njSqrt(VECTOR_SQUARED(&owner_motion->speed)) * 2.0F;
        if (speed > _builtin_fabsf(sway->bob)) {
            if (speed < 0.0F) {
                speed = -speed;
            }
            if (speed > 5.0F) {
                speed = 5.0F;
            }
            sway->bob = speed;
        }
        /* @hack spelling: the decay written out, which keeps the record in
         * R5 and the action table in R6 (0x8c0cfbe8, 0x8c0cfbec). with *=,
         * the two trade and 30 of 339 words differ */
        if (sway->bob >= 0.2F || sway->bob <= -0.2F) {
            sway->bob = sway->bob * 0.98F;
        } else {
            sway->bob = 0.0F;
        }
        step = (int)_builtin_fabsf(sway->bob * 512.0F);
        /* @hack spelling: step raised inside its test, which puts 0x1400 ahead
         * of 0x800 in the pool. raised before the test, the two swap, the
         * record and the action table trade R5 and R6 (0x8c0cfbe8), and 9
         * of 339 words differ */
        if ((step += 0x800) > 0x1400) {
            step = 0x1400;
        }
        sway->bob_phase += step;
        /* @hack spelling: the tilt worked out in three statements, which loads
         * knuckles_sway_tilt's address after it copies the cosine to FR4
         * (FMOV FR0,FR4 at 0x8c0cfd70, MOV.L at 0x8c0cfd76). as one
         * statement, SHC loads the address first, and 8 of 339 words
         * differ */
        tilt = njCos(sway->bob_phase);
        tilt = tilt * sway->bob;
        tilt = tilt * 819.2F;
        work->ang.z = (Angle)tilt;
        njUnitMatrix(&knuckles_sway_tilt);
        TURN_MATRIX(njRotateZ, &knuckles_sway_tilt, work->ang.z);
        njUnitMatrix(&knuckles_sway_half);
        TURN_MATRIX(njRotateZ, &knuckles_sway_half, work->ang.z >> 1);
        njUnitMatrix(&knuckles_sway_quarter);
        TURN_MATRIX(njRotateZ, &knuckles_sway_quarter, work->ang.z >> 2);
        for (i = 0; i < count; i++) {
            if (from->y >= 0.2F) {
                point.x = from->x + 1.0F;
                point.y = from->y + 3.0F;
                point.z = from->z;
                njCalcVector(&knuckles_sway_tilt, &point, to);
                to->x -= 1.0F;
                to->y -= 3.0F;
            } else if (from->x < -2.0F) {
                point.x = from->x + 1.0F;
                point.y = from->y + 3.0F;
                point.z = from->z;
                njCalcVector(&knuckles_sway_half, &point, to);
                to->x -= 1.0F;
                to->y -= 3.0F;
            } else if (_builtin_fabsf(from->z) > 2.0F) {
                point.x = from->x + 1.0F;
                point.y = from->y + 3.0F;
                point.z = from->z;
                njCalcVector(&knuckles_sway_quarter, &point, to);
                to->x -= 1.0F;
                to->y -= 3.0F;
            }
            from++;
            to++;
        }
        break;
    default:
        task_destroy(task);
        break;
    }
    {
        struct knuckles_sway_models *models;

        /* @hack spelling: models set inside the first store, which puts its
         * pool word after knuckles_head's (MOV.L at 0x8c0cfef0). set apart,
         * the two swap and 2 of 339 words differ */
        knuckles_head[0].model = (models = &knuckles_sway_models)->swayed;
        knuckles_b_head[0].model = models->swayed;
    }
}

static void knuckles_heat_destroy(struct task *task)
{
}

/* show his maximum heat as it charges: a mote every fourth step, and once
 * it's full, his aura, pulsing */
static void knuckles_heat_step(struct task *task)
{
    int charge;
    struct task_work *work = task->work;
    struct task_work *owner;

    if (!(owner = player_work[HEAT_VARS(work)->player])) {
        task_destroy(task);
        return;
    }
    work->tick++;
    charge = *HEAT_VARS(work)->charge;
    if (charge > 0 && charge < KNUCKLES_CHARGED && !(charge & 3)) {
        knuckles_make_mote((Float)charge * (1.0F / KNUCKLES_CHARGED));
        return;
    }
    if (charge == KNUCKLES_CHARGED) {
        Float pulse = (1.0F + njSin(work->tick * 1024)) * 0.5F;

        knuckles_draw_aura(&owner->collision->shapes->center, pulse);
    }
}

static void knuckles_heat_start(struct task *task)
{
    task->destructor = knuckles_heat_destroy;
    task->step = knuckles_heat_step;
}

/* in his first or second punch, a press of the action buttons before frame
 * 20 marks the next punch to follow it. from frame 20 on, a press, or the
 * mark, starts the next punch at once */
static void knuckles_punch_next(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record)
{
    switch (record->anim.action) {
    case ACTION_PUNCH_1:
    case ACTION_PUNCH_2:
        if (record->anim.frame < 20.0F) {
            if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                            pad_action_buttons)) {
                knuckles_actions[record->anim.action].next =
                    record->anim.action + 1;
                CHARGE(record) = 0;
            }
        } else if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                               pad_action_buttons) ||
                   knuckles_actions[record->anim.action].next) {
            record->anim.action++;
            record->anim.frame = 0.0F;
            CHARGE(record) = 0;
        }
        break;
    }
}

/* play the punch he throws. on the frames knuckles_punches gives it, his
 * fist's shape strikes, its wave sets out and his fist flashes. it sounds
 * as it starts to strike. his third punch drives him on from frame 15,
 * raising dust and a swing from frame 20 to 30 */
static void knuckles_punch(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    NJS_POINT3 pos;
    NJS_VECTOR vec;
    const struct punch *punch;
    struct task *task;

    if (record->anim.action == ACTION_PUNCH_3) {
        if (record->anim.frame >= 15.0F) {
            if (record->anim.frame < 20.0F) {
                record->speed.x = (record->anim.frame - 15.0F) * 0.8F;
            } else if (record->anim.frame < 30.0F) {
                record->speed.x = 4.0F;
                pos = work->pos;
                pos.y += 2.0F;
                vec.x = -(motion->speed.x * 0.2F);
                vec.y = 0.0F;
                vec.z = -(motion->speed.z * 0.2F);
                dust_create(&pos, &vec, 1.0F);
                vec.x = vec.z = 0.0F;
                dust_create(&pos, &vec, 1.0F);
                knuckles_draw_swing(&work->collision->shapes->center,
                                    &work->ang, record->anim.frame);
                vibration_start(PLAYER_VARS(work)->player, 3, 15, 0);
            } else {
                record->speed.x *= 0.25F;
            }
        }
    }
    /* @hack spelling: each clear written out in full, which keeps 1.0F in
     * FR13 (FLDI1 FR13 at 0x8c0d00de). as &=, 1.0F moves to FR14, and 225
     * of 474 words differ */
    work->collision->shapes->levels =
        work->collision->shapes->levels & ~SHAPE_ATTACK;
    work->collision->shapes->levels =
        work->collision->shapes->levels & ~SHAPE_GUARD;
    work->collision->shapes[1].flags |= SHAPE_OFF;
    if (record->anim.action < ACTION_PUNCH_1 ||
        record->anim.action > ACTION_PUNCH_B5) {
        return;
    }
    punch = &knuckles_punches[(Uint8)(record->anim.action - ACTION_PUNCH_1)];
    /* @hack spelling: the punch's weld point indexed from weld_point0's
     * address, across the weld points that follow it in the record. read
     * at a byte offset from the record instead, 176 of 474 words differ */
    vec = (&record->weld_point0)[punch->point];
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, 0x8000 - work->ang.y);
    njCalcVector(NULL, &vec, &pos);
    njPopMatrix(1);
    njAddVector(&pos, &work->collision->shapes->center);
    if (record->anim.action == ACTION_PUNCH_5) {
        pos.y += 2.5F;
    }
    work->flags &= ~PLAYER_GUARD;
    if (punch->hit_start <= record->anim.frame &&
        record->anim.frame <= punch->hit_end) {
        work->collision->shapes->levels =
            work->collision->shapes->levels & ~SHAPE_ATTACK;
        work->collision->shapes->levels =
            (work->collision->shapes->levels & ~SHAPE_GUARD) | GUARD(3);
        work->collision->shapes[1].flags &= ~SHAPE_OFF;
        work->collision->shapes[1].center = pos;
        work->flags |= PLAYER_GUARD;
    }
    if (punch->wave && record->anim.frame >= punch->wave_frame &&
        record->anim.frame < punch->wave_frame + 1.0F) {
        if ((task = task_create(TASK_WORK, 6, knuckles_punch_wave_step)) !=
            NULL) {
            switch (punch->wave) {
            case WAVE_AHEAD:
                vec.x = 2.0F;
                vec.y = vec.z = 0.0F;
                break;
            case WAVE_BACK:
                vec.x = -2.0F;
                vec.y = vec.z = 0.0F;
                break;
            case WAVE_UP:
                vec.y = 2.0F;
                vec.x = vec.z = 0.0F;
                break;
            case WAVE_DOWN:
                vec.y = -2.0F;
                vec.x = vec.z = 0.0F;
                break;
            }
            player_turn_to_world(work, &vec);
            task->work->pos = pos;
            task->work->scl = vec;
        }
    }
    if (record->anim.frame >= punch->hit_start &&
        record->anim.frame < punch->hit_start + 1.0F) {
        switch (record->anim.action) {
        case ACTION_PUNCH_1:
        case ACTION_PUNCH_2:
            sound_play(SE_PUNCH, 0, 0, 0);
            vibration_start(PLAYER_VARS(work)->player, 3, 15, 0);
            break;
        case ACTION_PUNCH_3:
            sound_play(SE_PUNCH_LAST_VOICE, 0, 0, 0);
            sound_play(SE_PUNCH_LAST, 0, 0, 0);
            break;
        }
    }
    if (punch->flash_start >= 0.0F &&
        punch->flash_start <= record->anim.frame &&
        record->anim.frame <= punch->flash_end) {
        if ((task = task_create(TASK_WORK, 6, knuckles_fist_flash_step)) !=
            NULL) {
            task->work->pos = pos;
            task->work->scl.x = 1.0F;
        }
    }
}

/* turn him off a wall his glide runs into. meeting it at a slant, he turns
 * along it. meeting it head on, he turns back off it, mirrored. then he
 * moves on by his last step, and out from the wall by its normal */
static void knuckles_glide_bounce(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record, NJS_VECTOR *normal)
{
    NJS_POINT3 previous;
    Float dx;
    Float dz;
    Angle facing = (Angle)(atan2f(normal->z, normal->x) * ANGLE_PER_RADIAN);

    if (angle_distance(work->ang.y + 0x8000, facing) > 0x2000) {
        if (angle_difference(work->ang.y + 0x8000, facing) > 0) {
            work->ang.y = facing + 0x4000;
        } else {
            work->ang.y = facing - 0x4000;
        }
    } else {
        work->ang.y = angle_difference(work->ang.y + 0x8000, facing) + facing;
    }
    motion->aim.y = work->ang.y;
    LEAN(record) = 0;
    player_get_history(PLAYER_VARS(work)->player, 1, &previous, NULL);
    dx = work->pos.x - previous.x;
    dz = work->pos.z - previous.z;
    work->pos.x += dx + normal->x;
    work->pos.z += dz + normal->z;
}

/* what his glide runs into, in the world's objects ahead of him. a push
 * from a task's shape leaves him gliding, GLIDE_CLEAR. water comes first.
 * he turns off a wall he can't climb or a floor, and is pushed off a
 * ceiling. a wall he can climb gives GLIDE_CLIMB if player_collide reports
 * a hit, and GLIDE_BOUNCE otherwise */
static enum glide_hit knuckles_glide_collide(struct task_work *work,
                                             struct task_motion *motion,
                                             struct player *record)
{
    Bool climbable;
    struct polygon_ball contact;
    NJS_POINT3 offset;
    struct world_object *object;
    int count;
    struct polygon_contact *surface;
    Sint16 surfaces;
    int kind;
    Uint32 flags;

    if (work->collision != NULL &&
        (work->collision->flags & COLLISION_PUSHED)) {
        kind = work->collision->kind;
        /* @bug logic: the kind tested is that of his own collision,
         * which collision_init set to COLLISION_PLAYER, not the pushing
         * task's. so the test at 0x8c0d06d2 always holds, and no push
         * knocks him into the fall below */
        if (kind == COLLISION_PLAYER || kind == COLLISION_ATTACK ||
            kind == COLLISION_ITEM || kind == COLLISION_ENEMY) {
            return GLIDE_CLEAR;
        }
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ATTACKING;
        work->mode = KNUCKLES_FALL;
        record->anim.action = ACTION_FALL;
        return GLIDE_BOUNCE;
    }
    contact.radius = record->consts.height * 0.5F;
    contact.hit.ang_x = work->ang.x;
    contact.hit.ang_z = work->ang.z;
    contact.pos.x = work->collision->shapes->center.x +
                    (contact.radius * njCos(work->ang.y));
    contact.pos.y = work->collision->shapes->center.y;
    contact.pos.z = work->collision->shapes->center.z +
                    (contact.radius * njSin(work->ang.y));
    contact.speed = motion->speed;
    polygon_collect_near(&contact);
    count = world_count;
    object = world_objects;
    while (count > 0) {
        /* @hack spelling: flags set inside the test, which keeps the record in
         * R11 (MOV R6,R11 at 0x8c0d06c2). set apart, the record and the
         * mask 0x400002 trade R10 and R11, and 9 of 259 words differ */
        if (((flags = object->flags) & WORLD_LIQUID) &&
            polygon_sweep(&contact, object->object, flags) == TRUE) {
            return GLIDE_WATER;
        }
        count--;
        object++;
    }
    climbable = FALSE;
    count = world_count;
    object = world_objects;
    for (; count > 0; count--, object++) {
        if (object->flags & WORLD_LIQUID) {
            continue;
        }
        if (polygon_sweep(&contact, object->object, object->flags) == TRUE) {
            climbable = TRUE;
            if (object->flags & WORLD_NO_CLIMB) {
                knuckles_glide_bounce(work, motion, record,
                                      &contact.hit.normal);
                return GLIDE_BOUNCE;
            }
            for (surfaces = contact.count, surface = polygon_contacts;
                 surfaces > 0; surfaces--, surface++) {
                if (!(surface->hit.normal.y > -0.71F &&
                      surface->hit.normal.y < 0.71F)) {
                    if (surface->hit.normal.y >= 0.0F) {
                        knuckles_glide_bounce(work, motion, record,
                                              &surface->hit.normal);
                        return GLIDE_BOUNCE;
                    } else {
                        offset = surface->hit.pos;
                        njSubVector(&offset, &work->collision->shapes->center);
                        njAddVector(&work->pos, &offset);
                        return GLIDE_BOUNCE;
                    }
                }
            }
        }
    }
    if (climbable == TRUE) {
        if (player_collide(work, motion, record)) {
            return GLIDE_CLIMB;
        }
        return GLIDE_BOUNCE;
    }
    return GLIDE_CLEAR;
}

/* TRUE while a surface lies within his height of any of nine points. the
 * points are his middle and those around it 4 away along x, z or both.
 * FALSE otherwise */
static Bool knuckles_wall_near(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    struct polygon_ball contact;
    Float dz;
    Float dx;
    struct world_object *object;
    int count;

    contact.radius = record->consts.height;
    contact.hit.ang_x = work->ang.x;
    contact.hit.ang_z = work->ang.z;
    contact.pos = work->collision->shapes->center;
    contact.speed.x = contact.speed.y = contact.speed.z = 0.0F;
    polygon_collect_near(&contact);
    /* NOLINTBEGIN(bugprone-float-loop-counter) */
    for (dz = -4.0F; dz <= 4.0F; dz += 4.0F) {
        for (dx = -4.0F; dx <= 4.0F; dx += 4.0F) {
            /* NOLINTEND(bugprone-float-loop-counter) */
            contact.pos = work->collision->shapes->center;
            contact.pos.x += dx;
            contact.pos.z += dz;
            count = world_count;
            object = world_objects;
            while (count > 0) {
                if (polygon_sweep(&contact, object->object, object->flags) ==
                    1) {
                    return TRUE;
                }
                count--;
                object++;
            }
        }
    }
    return FALSE;
}

/* his action as he climbs, by the way he moves: up or down, across, or
 * still. ACTION_GRAB_WALL and ACTION_CLIMB_OUT play to their end */
static void knuckles_set_climb_action(struct task_work *work,
                                      struct player *record)
{
    int action = record->anim.action;

    if (action == ACTION_GRAB_WALL || action == ACTION_CLIMB_OUT) {
        return;
    }
    if (!(work->flags & PLAYER_ON_LAND) ||
        (record->speed.x == 0.0F && record->speed.z == 0.0F)) {
        record->anim.action = ACTION_CLING;
        return;
    }
    if (_builtin_fabsf(record->speed.x) > _builtin_fabsf(record->speed.z)) {
        record->anim.action = ACTION_CLIMB;
    } else if (record->speed.z > 0.0F) {
        record->action_speed = record->speed.z;
        record->anim.action = ACTION_CLIMB_PLUS_Z;
    } else {
        record->action_speed = -record->speed.z;
        record->anim.action = ACTION_CLIMB_MINUS_Z;
    }
}

/* set his shapes for what he does. in his punches' rows, knuckles_punch
 * sets them. otherwise only his body's shape is on, with the attack and
 * guard levels of what he does. gliding, his body guards at level 3, and
 * the shapes ahead of him and behind him go on. the one ahead strikes, and
 * the one behind, at attack 0, only pushes */
static void knuckles_set_shapes(struct task_work *work, struct player *record)
{
    int action = record->anim.action;
    int i;
    /* @hack redundant: collision, which nothing reads, and whose address
     * unused takes, so SHC keeps its store in the frame (MOV.L R13,@(4,R15)
     * at 0x8c0d0b16). without the two, the frame shrinks 4 bytes, and 32 of
     * 208 words differ */
    struct collision_work *collision;
    struct collision_work **unused = &collision;
    struct collision_shape *shape;
    int attack;
    int guard;

    /* @hack redundant: an else after an arm that returns, whose jump makes the
     * test below load the collision into R2, not R3 (0x8c0d0b04); without
     * it, 2 of 208 words differ */
    if (action >= ACTION_PUNCH_1 && action <= ACTION_PUNCH_ROWS_LAST) {
        return;
    } else if (work->collision != NULL) {
        shape = (collision = work->collision)->shapes;
        for (i = 1; i < 4; i++) {
            shape[i].flags |= SHAPE_OFF;
        }
        if (record->effects & PLAYER_INVINCIBLE) {
            attack = 3;
            guard = 3;
        } else if (work->flags & PLAYER_BALL) {
            attack = 1;
            guard = 1;
        } else if (work->mode == KNUCKLES_HOMING) {
            attack = 3;
            guard = 3;
        } else if (work->mode == KNUCKLES_MODULE_POSE) {
            attack = 0;
            guard = 1;
        } else {
            guard = attack = 0;
        }
        shape->levels = (shape->levels & ~SHAPE_ATTACK) | ATTACK(attack);
        shape->levels = (shape->levels & ~SHAPE_GUARD) | GUARD(guard);
    }
    if (record->anim.action == ACTION_GLIDE) {
        work->collision->shapes->levels =
            work->collision->shapes->levels & ~SHAPE_ATTACK;
        work->collision->shapes->levels =
            (work->collision->shapes->levels & ~SHAPE_GUARD) | GUARD(3);
        work->collision->shapes[2].flags &= ~SHAPE_OFF;
        work->collision->shapes[2].center.x =
            work->collision->shapes->center.x + (njCos(work->ang.y) * 2.0F);
        work->collision->shapes[2].center.y = work->collision->shapes->center.y;
        work->collision->shapes[2].center.z =
            work->collision->shapes->center.z + (njSin(work->ang.y) * 2.0F);
        work->collision->shapes[3].flags &= ~SHAPE_OFF;
        work->collision->shapes[3].center.x =
            work->collision->shapes->center.x - (njCos(work->ang.y) * 4.75F);
        work->collision->shapes[3].center.y =
            work->collision->shapes->center.y - 0.75F;
        work->collision->shapes[3].center.z =
            work->collision->shapes->center.z - (njSin(work->ang.y) * 4.75F);
    }
}

/* with the shovel claws, he digs as he holds the action buttons and
 * presses the jump button. he must stand on land, or on a task he can dig.
 * the nearest surface he touches must have WORLD_DIGGABLE and not
 * WORLD_MOVES, or he tries and can't. TRUE once he digs or tries, and FALSE
 * otherwise */
static Bool knuckles_try_dig(struct task_work *work, struct task_motion *motion,
                             struct player *record)
{
    struct polygon_ball contact;
    struct world_object *object;
    int count;
    Bool diggable;
    Float nearest;
    Float distance;

    if (record->upgrades & PLAYER_KNUCKLES_SHOVEL_CLAWS) {
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                        pad_action_buttons) &&
            (pad_records[PLAYER_VARS(work)->player].press & pad_jump_buttons)) {
            /* @hack spelling: a loop of one pass, which keeps the record in
             * R11 (MOV R6,R11 at 0x8c0d0cf6). as one if, the record takes
             * R12 and 54 of 171 words differ */
            for (;;) {
                if ((work->flags & PLAYER_ON_TASK) &&
                    (work->collision->hit->shapes->flags & SHAPE_DIGGABLE)) {
                    break;
                }
                if (work->flags & PLAYER_ON_LAND) {
                    break;
                }
                return FALSE;
            }
            contact.radius = record->consts.height * 0.5F;
            contact.hit.ang_x = work->ang.x;
            contact.hit.ang_z = work->ang.z;
            contact.pos = work->pos;
            contact.speed = motion->speed;
            polygon_collect_near(&contact);
            diggable = FALSE;
            nearest = GROUND_FAR;
            count = world_count;
            object = world_objects;
            while (count > 0) {
                if (polygon_sweep(&contact, object->object, object->flags) ==
                    1) {
                    if (nearest > (distance = njDistanceP2P(&contact.hit.point,
                                                            &contact.pos))) {
                        nearest = distance;
                        if ((object->flags & WORLD_DIGGABLE) &&
                            !(object->flags & WORLD_MOVES)) {
                            diggable = TRUE;
                        } else {
                            diggable = FALSE;
                        }
                    }
                }
                count--;
                object++;
            }
            if (!diggable) {
                work->mode = KNUCKLES_DIG_FAIL;
                record->anim.action = ACTION_DIG_FAIL;
            } else {
                record->speed.x = 0.0F;
                record->speed.y = 0.0F;
                record->speed.z = 0.0F;
                KNUCKLES_VARS(work)->depth = 0.0F;
                KNUCKLES_VARS(work)->steps = 0;
                work->mode = KNUCKLES_DIG_DOWN;
                record->anim.action = ACTION_DIG_START;
                sound_play(SE_DIG_START, 0, 0, 0);
            }
            return TRUE;
        }
    }
    return FALSE;
}

/* a task that waits out its timer, then plays the sound in its ang.x */
static void knuckles_sound_step(struct task *task)
{
    struct task_work *work = task->work;

    if (--TASK_TIMER(work) == 0) {
        sound_play(work->ang.x, 0, 0, 0);
        task_kill(task);
    }
}

/* a random fraction from 0 to just under 1 */
/* @hack spelling: the fraction through a function SHC expands, which puts
 * knuckles_dig_find's chance in FR15 (FMOV FR3,FR15 at 0x8c0d0f92).
 * written out there, the chance takes FR5, and 9 of 217 words differ */
static Float random_fraction(void)
{
    return rand() * RAND_SCALE;
}

/* turn up what his dig finds, picked by the chances where he is. it finds
 * nothing, rings, a fountain of rings or an item box, and in some stages a
 * rare animal. its sound plays 60 steps later. near the buried piece he
 * finds nothing, and no sound plays. while knuckles_force_dig forces his
 * dig, he finds nothing, and knuckles_dig_forced_sound plays at once */
static void knuckles_dig_find(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    struct task *task;
    Float chance;
    Float sum;
    Float *weight;
    /* @hack type: find as a Uint32, which SHC compares with 0 unsigned
     * (CMP/HI at 0x8c0d0fa4). as an int, it takes CMP/PL, and 33 of 217
     * words differ */
    Uint32 find;
    int se;

    if (knuckles_buried_piece_near(&work->pos)) {
        return;
    }
    if (knuckles_dig_forced == TRUE) {
        switch (knuckles_dig_forced_sound) {
        case KNUCKLES_DIG_SOUND_NOTHING:
            sound_play(SE_DUG_NOTHING, 0, 0, 0);
            break;
        case KNUCKLES_DIG_SOUND_ITEM:
            sound_play(SE_DUG_ITEM, 0, 0, 0);
            break;
        /* @hack redundant: default: break, whose jump puts rand's address in
         * R3, not R2 (0x8c0d0f80); without it, 2 of 217 words differ */
        default:
            break;
        }
        return;
    }
    chance = random_fraction();
    find = DIG_ANIMAL;
    weight = &knuckles_dig_chances[find];
    sum = *weight;
    while (chance > sum) {
        weight--;
        sum += *weight;
        find--;
        if (find <= 0) {
            break;
        }
    }
    switch (find) {
    case DIG_NOTHING:
        se = SE_DUG_NOTHING;
        break;
    case DIG_RING:
        ring_scatter(work->pos.x, work->pos.y - 8.0F, work->pos.z, 1);
        se = SE_DUG_RINGS;
        break;
    case DIG_FIVE_RINGS:
        ring_scatter(work->pos.x, work->pos.y - 8.0F, work->pos.z, 5);
        se = SE_DUG_RINGS;
        break;
    case DIG_TEN_RINGS:
        ring_scatter(work->pos.x, work->pos.y - 8.0F, work->pos.z, 10);
        se = SE_DUG_RINGS;
        break;
    case DIG_FOUNTAIN:
        knuckles_make_fountain(&work->pos);
        se = SE_DUG_ITEM;
        break;
    case DIG_ITEM_BOX: {
        enum item_kind kinds[6] = {ITEM_SPEED_UP,  ITEM_INVINCIBLE,
                                   ITEM_BARRIER,   ITEM_1UP,
                                   ITEM_EXPLOSION, ITEM_MAGNETIC_BARRIER};

        item_box_air_create(&work->pos,
                            kinds[(int)((Float)rand() * (5.94F * RAND_SCALE))]);
        se = SE_DUG_ITEM;
        break;
    }
    case DIG_ANIMAL: {
        Uint16 stages[6] = {STAGE_WINDY,  STAGE_MOUNTAIN, STAGE_RUIN,
                            STAGE_ICECAP, STAGE_FINAL,    DIG_STAGES_END};
        Uint16 *stage;

        stage = stages;
        while (*stage != DIG_STAGES_END) {
            if (IN_STAGE(*stage)) {
                break;
            }
            stage++;
        }
        if (*stage != DIG_STAGES_END) {
            animal_create_rare(work->pos.x, work->pos.y, work->pos.z);
            se = SE_DUG_ITEM;
        } else {
            se = SE_DUG_NOTHING;
        }
        break;
    }
    }
    if ((task = task_create(TASK_WORK, 6, knuckles_sound_step)) != NULL) {
        TASK_TIMER(task->work) = 60;
        task->work->ang.x = se;
    }
}

/* set the chances of a dig's finds where he is. within DIG_AGAIN of where
 * he last dug out, they're the chances anywhere, scaled down. near a point
 * of the grid, they're knuckles_dig_chances_near's, and on one,
 * knuckles_dig_chances_on's. elsewhere, they're the chances anywhere */
static void knuckles_set_dig_chances(struct task_work *work)
{
    NJS_POINT3 grid;
    Float distance;
    const Float *from;
    Float *to;
    int i;
    /* @hack type: j as a Uint32, which with to stepped apart keeps the
     * chance table in R11 and the zero in R12 (0x8c0d119a, 0x8c0d11a4). as
     * an int, as i is, the two trade and 14 of 131 words differ */
    Uint32 j;

    grid.x = work->pos.x + (DIG_GRID / 2);
    grid.x = grid.x - float_wrap(grid.x, DIG_GRID);
    grid.y = work->pos.y;
    grid.z = work->pos.z + (DIG_GRID / 2);
    grid.z = grid.z - float_wrap(grid.z, DIG_GRID);

    if ((distance = njDistanceP2P(&work->pos, &knuckles_dig_last_pos)) <
        DIG_AGAIN) {
        Float scale;

        i = 0;
        if (distance < DIG_SPENT) {
            scale = 0.0F;
        } else {
            scale = (distance - DIG_SPENT) * 0.125F;
        }
        from = knuckles_dig_chances_anywhere;
        to = knuckles_dig_chances;
        do {
            *to++ = *from * scale;
            from++;
            /* NOLINTNEXTLINE(clang-diagnostic-sign-compare) */
        } while (++i < sizeof(knuckles_dig_chances) /
                           sizeof(knuckles_dig_chances[0]));
    } else if ((distance = njDistanceP2P(&work->pos, &grid)) < DIG_NEAR) {
        if (distance < DIG_ON) {
            from = knuckles_dig_chances_on;
        } else {
            from = knuckles_dig_chances_near;
        }
        i = 0;
        to = knuckles_dig_chances;
        do {
            *to++ = *from;
            from++;
            /* NOLINTNEXTLINE(clang-diagnostic-sign-compare) */
        } while (++i < sizeof(knuckles_dig_chances) /
                           sizeof(knuckles_dig_chances[0]));
    } else {
        j = 0;
        to = knuckles_dig_chances;
        from = knuckles_dig_chances_anywhere;
        do {
            /* @hack spelling: to stepped apart from its store, which with j's
             * type keeps the chance table in R11 and the zero in R12
             * (0x8c0d119a, 0x8c0d11a4). stepped in the store, as in the
             * copies above, the two trade and 18 of 131 words differ */
            *to = *from;
            to++;
            from++;
        } while (++j < sizeof(knuckles_dig_chances) /
                           sizeof(knuckles_dig_chances[0]));
    }
}

/* TRUE when none of the act's death zones that take his character lies
 * above or below him: he is past their edges. FALSE otherwise, and in an
 * act with no death zones */
static Bool knuckles_outside_zones(struct task_work *work)
{
    struct height_query query;
    struct death_zone **acts;
    struct death_zone *zone;
    Uint16 stage_act;
    /* @hack type: stage as a Uint16, which SHC zero-extends before it
     * scales it (EXTU.W at 0x8c0d12be). as an int, 52 of 67 words differ */
    Uint16 stage;
    Uint16 act;
    int mask;

    stage_act = stage_get_stage_act();
    act = stage_act & 0xff;
    stage = STAGE_OF(stage_act);
    acts = death_zones[stage];
    if (!acts) {
        return FALSE;
    }
    zone = acts[act];
    if (zone == NULL || zone->object == NULL) {
        return FALSE;
    }
    mask = 1 << get_character();
    query.pos = work->pos;
    for (; zone->object != NULL; zone++) {
        if ((zone->characters & mask) &&
            polygon_find_model_heights(&query, zone->object)) {
            return FALSE;
        }
    }
    return TRUE;
}

/* create player's look task. it turns nodes[0], his head, and moves
 * nodes[1] and nodes[2], his eyes. FALSE without memory for the task or
 * its player_look_motion. once made, it returns 96, not TRUE, and no
 * caller reads it */
/* NOLINTBEGIN(clang-diagnostic-return-type) */
static Bool knuckles_look_create(Sint8 player, NJS_OBJECT **nodes)
{
    struct task *task;
    struct player_look_motion *look;

    /* @hack spelling: the failed create's return FALSE in an else, which puts
     * it after the success path (MOV #0,R0 at 0x8c0d13b0, which the
     * success's BRA at 0x8c0d13ac skips). as an early return, SHC puts it
     * ahead of that path, and 16 of 46 words differ */
    if ((task = task_create(TASK_WORK, 2, knuckles_look_step)) != NULL) {
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
    /* @hack spelling: no return once made, where R0 keeps the 96 that
     * MOV #96,R0 at 0x8c0d13a8 loads as the last store's offset, so the
     * function returns 96. its one caller drops the result, so with return
     * TRUE, 7 of 46 words differ */
}
/* NOLINTEND(clang-diagnostic-return-type) */

/* put the look's nodes back at the angles they had as it began */
static void knuckles_look_destroy(struct task *task)
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

/* turn his head at what his event says to look at. without one, it turns
 * at the camera while Z is held, and otherwise the way he turns. the head
 * turns as far as its limits allow, and the eyes turn the rest of the
 * way */
static void knuckles_look_step(struct task *task)
{
    struct task_work *work = task->work;
    struct player_look_motion *look = PLAYER_LOOK(task);
    struct task_work *owner;
    struct task_motion *owner_motion;
    struct task_work *camera;
    struct collision_shape *shape;
    struct event_work *event;
    struct task_work *target;
    Angle angles[3];
    NJS_POINT3 direction;
    NJS_VECTOR facing;

    owner = player_work[PLAYER_VARS(work)->player];
    owner_motion = player_motion[PLAYER_VARS(work)->player];
    if (owner == NULL) {
        work->mode = LOOK_END;
    } else if (work->mode == LOOK_INIT) {
        /* @hack spelling: an empty arm, which leaves a starting look in
         * LOOK_INIT, and whose jump keeps the event's record in R3 (MOV.L
         * @(60,R12),R3 at 0x8c0d1476). with the tests below nested under a
         * test of LOOK_INIT, R2 takes it and 2 of 396 words differ */
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
        task->destructor = knuckles_look_destroy;
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
        camera = camera_work;
        shape = owner->collision->shapes;
        /* @hack spelling: y and z set first, in one chain, which keeps 0.0F in
         * FR5 and 1.0F in FR4 (FLDI0 FR5 at 0x8c0d14ea). set apart, with x
         * first, the two trade, vector_get_angles' address takes R8, and 191
         * of 396 words differ */
        facing.y = facing.z = 0.0F;
        facing.x = 1.0F;
        player_turn_to_world(owner, &facing);
        direction = camera->pos;
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
            if (event->look_task == NULL ||
                (target = event->look_task->work) == NULL) {
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
}

int knuckles_get_activity(int player)
{
    struct task_work *work = player_work[player];

    if (work != NULL && PLAYER_VARS(work)->character == CHARACTER_KNUCKLES) {
        /* @hack spelling: the mode kept in the parameter player, which holds
         * it in R4 and the work in R5 (MOV.B @R5,R4 at 0x8c0d1866). in a
         * local of its own, the mode takes R5 and the work R4, and 46 of 46
         * words differ */
        player = work->mode; /* NOLINT(bugprone-signed-char-misuse) */
        if (player >= KNUCKLES_DIG_DOWN && player <= KNUCKLES_DIG_OUT) {
            return work->mode - (KNUCKLES_DIG_DOWN - KNUCKLES_DIGGING_DOWN);
        }
        if (work->mode == KNUCKLES_PUNCH) {
            return work->mode - (KNUCKLES_PUNCH - KNUCKLES_PUNCHING);
        }
        if (work->mode == KNUCKLES_CLIMB) {
            return KNUCKLES_CLIMBING;
        }
        if (work->mode == KNUCKLES_CLIMB_OUT) {
            return KNUCKLES_CLIMBING_OUT;
        }
        if (work->mode == KNUCKLES_GLIDE || work->mode == KNUCKLES_GLIDE_TURN) {
            return KNUCKLES_GLIDING;
        }
    }
    return 0;
}

/* the first step of his mouth task. it gives the task a player_mouth and
 * hands it on to player_mouth_step. without memory, it tries again the
 * next step */
static void knuckles_mouth_start(struct task *task)
{
    struct player_mouth *mouth;

    if ((mouth = heap_calloc(1, sizeof(*mouth))) != NULL) {
        task->motion = (struct task_motion *)mouth;
        mouth->face = &knuckles_mouth;
        mouth->mouth = knuckles_head;
        mouth->blend = &knuckles_head_blend;
        mouth->shapes = &knuckles_mouth_shapes;
        task->destructor = player_mouth_destroy;
        task->step = player_mouth_step;
    }
}

/* the target he homes in on: the nearest in his near_tasks list that
 * isn't straight behind him, or NULL. he turns toward it, 0x1000 a step at
 * most, and draws his trail. each step he's within HOMING_BURST_REACH of
 * it, a burst of glows goes off at him */
static struct task_work *knuckles_home(struct task_work *work)
{
    struct near_task *near;
    struct task_work *best;
    NJS_POINT3 pos;
    Float best_distance;
    Angle best_heading;

    if (PLAYER_VARS(work)->player == 0) {
        near = near_tasks;
    } else {
        near = near_tasks_player1;
    }

    /* near_tasks gives squared distances */
    best_distance = HOMING_REACH * HOMING_REACH;
    best = NULL;

    for (; near->work != NULL; near++) {
        struct collision_shape *shape = near->work->collision->shapes;
        Angle heading;

        pos = shape->center;
        if (!(shape->flags & SHAPE_IN_WORLD)) {
            njAddVector(&pos, &near->work->pos);
        }
        njSubVector(&pos, &work->pos);
        heading = (Angle)(atan2f(pos.z, pos.x) * ANGLE_PER_RADIAN);
        if (angle_distance(work->ang.y, heading) > 0x7fff) {
            continue;
        }
        if (near->distance < best_distance) {
            best_heading = heading;
            best_distance = near->distance;
            best = near->work;
        }
    }

    if (best != NULL) {
        work->ang.y = angle_approach(work->ang.y, best_heading, 0x1000);
        knuckles_trail_draw(work);
    }
    if (best_distance <= HOMING_BURST_REACH * HOMING_BURST_REACH) {
        knuckles_make_burst(&work->pos, 0);
    }
    return best;
}

/* he holds on to a wall he can climb, at the point his contact found on
 * it, tilted to its slope */
static void knuckles_start_climb(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record)
{
    work->flags |= PLAYER_ON_LAND;
    record->speed.z = record->speed.x = 0.0F;
    work->ang.x = record->contact->hit.ang_x;
    work->ang.z = record->contact->hit.ang_z;
    work->pos.x = record->contact->hit.point.x;
    work->pos.y = record->contact->hit.point.y;
    work->pos.z = record->contact->hit.point.z;
    work->flags &= ~PLAYER_GUARD;
    work->mode = KNUCKLES_CLIMB;
    record->anim.action = ACTION_GRAB_WALL;
}

static const struct piece_act knuckles_piece_acts[] = {
    {{6, 6, 6, 3}, STAGE_ACT(STAGE_MOUNTAIN, 2)},
    {{6, 0, 12, 3}, STAGE_ACT(STAGE_HIGHWAY, 2)},
    {{6, 0, 5, 3}, STAGE_ACT(STAGE_CASINO, 0)},
    {{3, 4, 8, 6}, STAGE_ACT(STAGE_SKY_DECK, 2)},
    {{6, 6, 6, 3}, STAGE_ACT(STAGE_RUIN, 1)},
    {{0, 0, 0, 0}, PIECE_ACTS_END},
};

/* rand()'s result times scale: with RAND_SCALE, a fraction from 0 to just
 * under 1 */
/* @hack spelling: the product through a function SHC expands, with which
 * knuckles_pick_pieces saves R9 and FR14 last (FMOV.S FR14,@-R15 at
 * 0x8c0d1b02). written out in its place, SHC saves R8 and FR12 too, and
 * 229 of 254 words differ */
static Float random_scaled(Float scale)
{
    return rand() * scale;
}

Bool knuckles_pick_pieces(void)
{
    const struct piece_act *entry;
    Uint16 stage_act;
    int count;
    int choice;
    int buried;
    int i;
    Float scale;

    knuckles_pieces_picked = FALSE;
    knuckles_pieces_taken = 0;
    if (get_character() != CHARACTER_KNUCKLES) {
        return FALSE;
    }
    stage_act = stage_get_stage_act();
    entry = knuckles_piece_acts;
    while (entry->stage_act != stage_act) {
        if (entry->stage_act == PIECE_ACTS_END) {
            return FALSE;
        }
        entry++;
    }
    if (demo_playing && stage_get_stage_act() == STAGE_ACT(STAGE_MOUNTAIN, 2)) {
        knuckles_pieces[0].id = PIECE_FIRST | 3;
        knuckles_pieces[1].id = PIECE_BURIED | 4;
        knuckles_pieces[2].id = PIECE_THIRD;
    } else {
        for (count = play_time_get() & 15; count > 0; count--) {
            rand();
        }
        if (entry->places[0] < 1) {
            return FALSE;
        }
        /* @hack spelling: RAND_SCALE in a local, which SHC keeps in FR15, with
         * 0.99F in FR14 (FMOV.S @R0,FR15 at 0x8c0d1be8, FMOV.S @R0,FR14 at
         * 0x8c0d1bf2). passed as the constant, it goes to the frame, which
         * grows 16 bytes, and 179 of 254 words differ */
        scale = RAND_SCALE;
        knuckles_pieces[0].id =
            (int)(entry->places[0] * random_scaled(scale) * 0.99F) |
            PIECE_FIRST;
        if (entry->places[1] < 0 || entry->places[2] < 0 ||
            entry->places[1] + entry->places[2] < 1) {
            return FALSE;
        }
        if (event_flag_check(FLAG_KNUCKLES_SHOVEL_CLAWS) && entry->places[1]) {
            buried = entry->places[1]; /* NOLINT(bugprone-signed-char-misuse) */
            choice = buried + entry->places[2];
            choice = (int)(choice * random_scaled(scale) * 0.99F);
            /* @hack spelling: each arm's id set and then ORed, which gives each
             * arm an OR and store of its own (OR R4,R3 at 0x8c0d1c66). as
             * one expression, SHC joins the two arms' OR and store, and 16
             * of 254 words differ */
            if (choice < buried) {
                knuckles_pieces[1].id = PIECE_BURIED;
                knuckles_pieces[1].id |= choice;
            } else {
                knuckles_pieces[1].id = PIECE_SECOND;
                knuckles_pieces[1].id |= choice - buried;
            }
        } else {
            knuckles_pieces[1].id = PIECE_SECOND;
            knuckles_pieces[1].id |=
                (int)(entry->places[2] * random_scaled(scale) * 0.99F);
        }
        if (entry->places[3] < 1) {
            return FALSE;
        }
        knuckles_pieces[2].id =
            (int)(entry->places[3] * random_scaled(scale) * 0.99F) |
            PIECE_THIRD;
    }
    for (i = 0; i < 3; i++) {
        knuckles_pieces[i].taken = FALSE;
        knuckles_pieces[i].found = FALSE;
        knuckles_pieces[i].marked = FALSE;
    }
    knuckles_pieces_picked = TRUE;
    return TRUE;
}

/* a taken piece's icon. after ICON_FLIGHT_STEPS, it flies in from the
 * right of the screen to its place along the top, blinking. it blinks on
 * in its place until ang.x reaches ICON_BLINK_END, then shows steady */
static void knuckles_icon_step(struct task *task)
{
    struct task_work *work;
    Float gap;
    Float step;
    Float alpha;

    work = task->work;
    if (edit_mode != 0) {
        return;
    }
    alpha = 1.0F;
    switch (work->mode) {
    case ICON_WAIT:
        if (++TASK_TIMER(work) >= ICON_FLIGHT_STEPS) {
            work->mode = ICON_FLY;
        }
        break;
    case ICON_FLY:
        gap = work->scl.x - work->pos.x;
        if (gap < -50.0F) {
            step = -50.0F * 0.3F;
        } else {
            step = gap * 0.3F;
        }
        work->pos.x += step;
        work->ang.x += 0x800;
        alpha = _builtin_fabsf(njCos(work->ang.x));
        if (hud_hidden >= 0) {
            knuckles_draw_radar_sprite(&work->pos, 3, 1.0F, alpha);
        }
        if (step >= -0.01F) {
            work->mode = ICON_BLINK;
            /* @hack redundant: a redundant break, whose jump SHC deletes, but
             * which still puts ICON_BLINK's ICON_BLINK_END in R3 (MOV.L at
             * 0x8c0d1e44). without it, R1 takes it and 8 of 110 words
             * differ */
            break;
        }
        break;
    case ICON_BLINK:
        if (work->ang.x >= ICON_BLINK_END) {
            alpha = 1.0F;
        } else {
            work->ang.x += 0x800;
            alpha = _builtin_fabsf(njCos(work->ang.x));
        }
        if (hud_hidden >= 0) {
            knuckles_draw_radar_sprite(&work->pos, 3, 1.0F, alpha);
        }
        break;
    }
}

/* start an icon for each piece he takes, counting them in ang.x, until
 * the third */
static void knuckles_icons_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *icon;

    while (work->ang.x < knuckles_pieces_taken) {
        if ((icon = task_create(TASK_WORK, 3, knuckles_icon_step)) != NULL) {
            icon->work->scl.x = ICON_FIRST_X - (work->ang.x * ICON_GAP);
            icon->work->scl.y = ICON_Y;
            icon->work->pos.x = ICON_ENTRY_X;
            icon->work->pos.y = ICON_Y;
        }
        if (++work->ang.x == 3) {
            task_kill(task);
        }
    }
}

/* draw a piece's sprite on the radar, by how near player 0 is to it. the
 * sprite for RADAR_FAR is dim */
static void knuckles_radar_draw(struct task_work *work, int piece,
                                enum radar_near near)
{
    NJS_POINT3 pos;
    Float scale;
    Float alpha;
    int sprite;

    if (hud_hidden >= 0 && near != RADAR_NONE) {
        pos.x = RADAR_X + (piece * RADAR_GAP);
        pos.y = RADAR_Y;
        scale = 0.5F + (RADAR_VARS(work)->pulse[piece] * 0.005F);
        alpha = 1.0F;
        /* NOLINTNEXTLINE(clang-diagnostic-switch) */
        switch (near) {
        case RADAR_FAR:
            sprite = 2;
            alpha = 0.2F;
            break;
        case RADAR_WITHIN_400:
            sprite = 5;
            break;
        case RADAR_WITHIN_300:
            sprite = 4;
            break;
        case RADAR_WITHIN_100:
            sprite = 1;
            break;
        case RADAR_WITHIN_50:
            sprite = 7;
            break;
        case RADAR_WITHIN_30:
            sprite = 0;
            break;
        }
        knuckles_draw_radar_sprite(&pos, sprite, scale, alpha);
    }
}

/* a taken piece's sprite, which flies from the radar toward ICON_ENTRY_X
 * and ICON_Y for ICON_FLIGHT_STEPS */
static void knuckles_taken_icon_step(struct task *task)
{
    struct task_work *work = task->work;
    Float progress;

    if (++TASK_TIMER(work) >= ICON_FLIGHT_STEPS) {
        task_kill(task);
    }
    progress = TASK_TIMER(work) * ICON_FLIGHT_SHARE;
    work->scl.x = (progress * (ICON_ENTRY_X - work->pos.x)) + work->pos.x;
    work->scl.y = (progress * progress * (ICON_Y - work->pos.y)) + work->pos.y;
    knuckles_draw_radar_sprite(&work->scl, 3, 0.5F, 0.5F);
}

/* keep pauses off. after FINISH_STEPS, put player 0 at
 * knuckles_finish_pos, end the hunt and clear the act */
static void knuckles_finish_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];

    TASK_TIMER(work)++;
    pause_disable();
    if (TASK_TIMER(work) > FINISH_STEPS) {
        player->pos = knuckles_finish_pos;
        knuckles_pieces_picked = FALSE;
        stage_clear();
        task_kill(task);
    }
}

/* the radar. each piece's sprite shows how near player 0 is to it. a beep
 * quickens as he nears the nearest found piece within 400, and that
 * piece's sprite pulses with each beep. once he takes the third piece, the
 * time stops, no hit hurts him, and he finishes the hunt. the debug text
 * shows each piece's state, and a line from it to him */
static void knuckles_radar_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player;
    struct piece *piece;
    /* @hack redundant: nearest, which the loop sets and nothing reads, and
     * which takes 12 bytes of the frame (ADD #-24,R15 at 0x8c0d210c).
     * without it and its copy, the frame shrinks to 12 bytes, and 319 of
     * 347 words differ */
    NJS_POINT3 nearest; /* NOLINT(clang-diagnostic-unused-but-set-variable) */
    Float best;
    Float ratio;
    int best_piece;
    int interval;
    int i;

    if (edit_mode != 0) {
        return;
    }
    player = player_work[0];
    if (player == NULL ||
        PLAYER_VARS(player)->character != CHARACTER_KNUCKLES ||
        !knuckles_pieces_picked) {
        task_kill(task);
        return;
    }
    if (knuckles_pieces_taken == 3) {
        struct collision_shape *shape = player->collision->shapes;

        /* @hack spelling: shape = shape + 1, after which SHC loads 3 into
         * R12 for the pieces' loop (MOV #3,R12 at 0x8c0d219c). with
         * shape++, it copies player_work's address there instead, the code
         * shrinks, and 280 of 347 words differ */
        for (i = 4; i > 0; i--, shape = shape + 1) {
            shape->push = PUSH_FIRM(PUSH_ALL);
        }
        player->timer = KNUCKLES_TIMER_HELD;
        pause_disable();
        time_stop();
        TASK_TIMER(work) = 0;
        task->step = knuckles_finish_step;
        return;
    }
    work->tick++;
    best = 800.0F;
    piece = knuckles_pieces;
    for (i = 0; i < 3; i++, piece++) {
        if (!piece->taken) {
            Float distance = njDistanceP2P(&player_work[0]->pos, &piece->pos);

            if (!piece->found || distance > 400.0F) {
                knuckles_radar_draw(work, i, RADAR_FAR);
            } else {
                if (distance < best) {
                    best = distance;
                    best_piece = i;
                    nearest = piece->pos;
                }
                if (distance < 30.0F) {
                    knuckles_radar_draw(work, i, RADAR_WITHIN_30);
                } else if (distance < 50.0F) {
                    knuckles_radar_draw(work, i, RADAR_WITHIN_50);
                } else if (distance < 100.0F) {
                    knuckles_radar_draw(work, i, RADAR_WITHIN_100);
                } else if (distance < 300.0F) {
                    knuckles_radar_draw(work, i, RADAR_WITHIN_300);
                } else {
                    knuckles_radar_draw(work, i, RADAR_WITHIN_400);
                }
            }
        }
    }
    if (best <= 400.0F) {
        TASK_TIMER(work)++;
        ratio = best * 0.0025F;
        if (ratio > 1.0F) {
            ratio = 1.0F;
        } else if (ratio < 0.0F) {
            ratio = 0.0F;
        }
        if (ratio < 0.5F) {
            interval = ratio * 35.999999F;
            if (interval < 8) {
                interval = 8;
            }
        } else {
            interval = (0.3F + ((ratio - 0.5F) * 1.4F)) * 60.0F;
        }
        if (TASK_TIMER(work) > interval) {
            RADAR_VARS(work)->pulse[best_piece] = 76 - interval;
            TASK_TIMER(work) = 0;
            sound_play(SE_RADAR, 0, 0, 0);
        }
    }
    /* @hack spelling: i = i + 1, which leaves R8 for the debug text's 23
     * (MOV #23,R8 at 0x8c0d23a0). with i++, 23 takes R9 and 15 of 347 words
     * differ */
    for (i = 0; i < 3; i = i + 1) {
        if ((RADAR_VARS(work)->pulse[i] -=
             (RADAR_VARS(work)->pulse[i] >> 4) + 1) < 0) {
            RADAR_VARS(work)->pulse[i] = 0;
        }
    }
    if (debug_hidden >= 0) {
        njPrintC(NJM_LOCATION(2, 18), "id");
        njPrintC(NJM_LOCATION(5, 18), "bo");
        njPrintC(NJM_LOCATION(8, 18), "ps");
        njPrintC(NJM_LOCATION(11, 18), "cp");
        piece = knuckles_pieces;
        for (i = 3; i > 0; i--, piece++) {
            njPrintH(NJM_LOCATION(2, 23 - i), piece->id, 2);
            njPrintH(NJM_LOCATION(6, 23 - i), piece->taken, 1);
            njPrintH(NJM_LOCATION(9, 23 - i), piece->found, 1);
            njPrintH(NJM_LOCATION(12, 23 - i), piece->marked, 1);
            if (!piece->taken && piece->found == TRUE) {
                knuckles_piece_line[0] = piece->pos;
                knuckles_piece_line[1] = player_work[0]->pos;
                njDrawLine3D(&knuckles_piece_lines[3 - i], 1, 0);
            }
        }
    }
}

static void knuckles_radar_destroy(struct task *task)
{
}

/* start the radar in an act with pieces, with the task that adds the taken
 * pieces' sprites. in any other act, the task ends */
static void knuckles_radar_start(struct task *task)
{
    Uint16 stage_act = stage_get_stage_act();
    const struct piece_act *entry = knuckles_piece_acts;
    struct task *icons;

    while (entry->stage_act != stage_act) {
        if (entry->stage_act == PIECE_ACTS_END) {
            task_kill(task);
            return;
        }
        entry++;
    }
    if ((icons = task_create(TASK_WORK, 3, knuckles_icons_step)) != NULL) {
        icons->work->ang.x = 0;
    }
    task->destructor = knuckles_radar_destroy;
    task->step = knuckles_radar_step;
}

Bool knuckles_nearest_piece(NJS_POINT3 *from, NJS_POINT3 *pos, Float *distance)
{
    struct task_work *player = player_work[0];
    NJS_POINT3 nearest;
    Float best;
    struct piece *piece;
    int i;

    if (player == NULL ||
        PLAYER_VARS(player)->character != CHARACTER_KNUCKLES ||
        !knuckles_pieces_picked || knuckles_pieces_taken == 3) {
        return FALSE;
    }
    best = 100000.0F;
    /* @hack spelling: piece set in the loop's first clause, after best, which
     * puts 100000.0F ahead of the table in the pool (MOVA at 0x8c0d257c).
     * set before best, the two swap and 2 of 84 words differ */
    for (i = 0, piece = knuckles_pieces; i < 3; i++, piece++) {
        if (!piece->taken && piece->found == TRUE) {
            Float here = njDistanceP2P(from, &piece->pos);

            if (here < best) {
                best = here;
                nearest = piece->pos;
            }
        }
    }
    if (best <= 99999.0F) {
        *pos = nearest;
        *distance = best;
        return TRUE;
    }
    return FALSE;
}

/* TRUE when the second piece is a buried one, found and not taken, within
 * 30 of pos. FALSE otherwise, and while player 0 has no task or isn't
 * Knuckles, while the hunt is off, and once all three are taken */
static Bool knuckles_buried_piece_near(NJS_POINT3 *pos)
{
    struct task_work *player = player_work[0];
    struct piece *piece;

    if (player == NULL ||
        PLAYER_VARS(player)->character != CHARACTER_KNUCKLES ||
        !knuckles_pieces_picked || knuckles_pieces_taken == 3) {
        return FALSE;
    }
    piece = &knuckles_pieces[1];
    /* @hack spelling: a switch of one case, which loads piece->taken into R3
     * (MOV.L @(4,R14),R3 at 0x8c0d266e). as an if, R2 takes it and 2 of 45
     * words differ */
    switch (piece->id & PIECE_GROUP) {
    case PIECE_BURIED:
        if (!piece->taken && piece->found == TRUE &&
            njDistanceP2P(pos, &piece->pos) < 30.0F) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

Bool knuckles_find_piece(int id, NJS_POINT3 *pos)
{
    struct piece *piece;
    int count;

    if (get_character() != CHARACTER_KNUCKLES || !knuckles_pieces_picked) {
        return FALSE;
    }
    id = (Uint8)id;
    if (id < PIECE_ID_FIRST || id > PIECE_ID_LAST) {
        return FALSE;
    }
    piece = knuckles_pieces;
    for (count = 3; count > 0 && piece->id != id; count--, piece++) {
    }
    if (count == 0 || piece->taken == TRUE) {
        return FALSE;
    }
    piece->found = TRUE;
    piece->pos = *pos;
    return TRUE;
}

/* the taken piece's sprite flies to the top of the screen, and
 * knuckles_finish_pos becomes the ground below the restart point. for the first
 * two pieces, the time shows. his voice follows 60 steps later, or 11 after the
 * third */
Bool knuckles_take_piece(int id, NJS_POINT3 *pos)
{
    struct piece *piece;
    struct task *task;
    NJS_POINT3 restart_pos;
    struct angles restart_ang;
    struct angles slope;
    Uint8 minutes;
    Uint8 seconds;
    int count;
    int voice;
    int delay;
    Float height;

    if (get_character() != CHARACTER_KNUCKLES || !knuckles_pieces_picked) {
        return FALSE;
    }
    id = (Uint8)id;
    if (id < PIECE_ID_FIRST || id > PIECE_ID_LAST) {
        return FALSE;
    }
    piece = knuckles_pieces;
    for (count = 3; count > 0 && piece->id != id; count--, piece++) {
    }
    if (count == 0 || piece->taken == TRUE) {
        return FALSE;
    }

    piece->taken = TRUE;
    knuckles_pieces_taken++;
    sound_play(SE_PIECE, 0, 0, 0);

    if ((task = task_create(TASK_WORK, 6, piece_taken_step)) != NULL) {
        if ((id & PIECE_GROUP) == PIECE_BURIED && player_work[0] != NULL) {
            task->work->pos = player_work[0]->pos;
        } else {
            task->work->pos = *pos;
        }
    }
    if ((task = task_create(TASK_WORK, 6, knuckles_taken_icon_step)) != NULL) {
        task->work->pos.x = RADAR_X + ((3 - count) * RADAR_GAP);
        task->work->pos.y = RADAR_Y;
    }

    if (piece->marked == TRUE) {
        restart_pos = piece->mark;
    } else {
        restart_pos = piece->pos;
    }
    if (player_work[0] == NULL) {
        restart_ang.x = 0;
        restart_ang.y = 0;
        restart_ang.z = 0;
    } else {
        restart_ang = player_work[0]->ang;
    }
    restart_save(&restart_pos, &restart_ang);
    knuckles_finish_pos = restart_pos;
    height = ground_below(knuckles_finish_pos.x, knuckles_finish_pos.y + 2.5F,
                          knuckles_finish_pos.z, &slope);
    if (height > -10000.0F) {
        knuckles_finish_pos.y = height;
    }

    if (knuckles_pieces_taken < 3) {
        time_get_whole(&minutes, &seconds);
        if ((task = task_create(TASK_WORK, 6, knuckles_time_step)) != NULL) {
            task->work->ang.x = minutes;
            task->work->ang.y = seconds;
        }
    }
    count = knuckles_pieces_taken;
    delay = 60;
    voice = count + (VOICE_PIECE - 1);
    if (count == 3) {
        delay = 11;
    }
    if ((task = task_create(TASK_WORK, 3, knuckles_sound_step)) != NULL) {
        TASK_TIMER(task->work) = delay;
        task->work->ang.x = voice;
    }
    return TRUE;
}

Bool knuckles_mark_piece(int id, NJS_POINT3 *pos)
{
    struct piece *piece;
    int count;

    if (get_character() != CHARACTER_KNUCKLES || !knuckles_pieces_picked) {
        return FALSE;
    }
    id = (Uint8)id;
    if (id < PIECE_ID_FIRST || id > PIECE_ID_LAST) {
        return FALSE;
    }
    piece = knuckles_pieces;
    for (count = 3; count > 0 && piece->id != id; count--, piece++) {
    }
    if (count == 0 || piece->marked == TRUE) {
        return FALSE;
    }
    piece->marked = TRUE;
    piece->mark = *pos;
    return TRUE;
}

/* the time in ang.x and ang.y, blinking, for TIME_STEPS */
static void knuckles_time_step(struct task *task)
{
    struct task_work *work = task->work;

    if (hud_hidden < 0) {
        task_kill(task);
        return;
    }
    if (++TASK_TIMER(work) > TIME_STEPS) {
        task_kill(task);
    } else if (TASK_TIMER(work) & 0x98) {
        hud_draw_time(&knuckles_time_layout, work->ang.x, work->ang.y);
    }
}

void knuckles_force_dig(int sound)
{
    knuckles_dig_forced = TRUE;
    knuckles_dig_forced_sound = sound;
}
