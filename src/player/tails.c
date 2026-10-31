/* Tails as a player: his modes, flight, tail attack and board */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "event/event.h"
#include "hud/hud.h"
#include "main.h"
#include "minicart/riders.h"
#include "object/capsule.h"
#include "object/common.h"
#include "object/helpers.h"
#include "object/panel.h"
#include "object/ring.h"
#include "path/path.h"
#include "player/m_sbmot.h"
#include "player/tails.h"
#include "player/tails_trail.h"
#include "player/player.h"
#include "player/race.h"
#include "player/target.h"
#include "player/textures.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* work->mode. tails_mode_names, which nothing reads, names most of these
 * modes. it skips some, the first TAILS_ROLL, so past TAILS_PATH a mode's
 * value doesn't index its name. an MD_ name in a mode's comment is its
 * name there */
enum tails_mode {
    TAILS_INIT,
    TAILS_STAND,
    TAILS_WALK,
    TAILS_PATH, /* MD_1DWK: running along a path */
    TAILS_ROLL, /* curled up, rolling */
    TAILS_PATH_SPIN, /* rolling along a path, curled up */
    TAILS_JUMP,
    TAILS_SPRING,
    TAILS_LAUNCH,
    TAILS_FLUNG, /* from PLAYER_MODE_FLING, with only gravity on him */
    TAILS_FALL,
    TAILS_STOP, /* MD_STOP: skidding */
    TAILS_PUSH,
    TAILS_HURT, /* MD_DAMG: knocked back and up */
    TAILS_BACK,
    TAILS_FLY,
    TAILS_CRASH, /* knocked back by a wall, or by PLAYER_MODE_KNOCK_BACK */
    TAILS_SLIDE, /* sliding down a steep slope */
    TAILS_POSE, /* MD_PLON: held in the action a request gave him */
    TAILS_TIRED, /* MD_TIRE: falling once his flight is spent */
    TAILS_TAIL_ATTACK, /* MD_TATK */
    TAILS_LIFT, /* MD_PICK: lifting what he faces */
    TAILS_PUT_DOWN,
    TAILS_HOLD_STAND, /* MD_HOLD: standing, holding something */
    TAILS_GRIP, /* MD_TOCH: gripping what he'll push, pull or shake */
    TAILS_HOLD_WALK,
    TAILS_THROW,
    TAILS_HOLD_STOP,
    TAILS_HOLD_FALL,
    TAILS_HOLD_JUMP,
    TAILS_HOLD_SPRING,
    TAILS_HOLD_LAUNCH,
    TAILS_PULL,
    TAILS_AIR_THROW, /* MD_THRU */
    TAILS_SHAKE, /* shaking what he grips */
    /* shaking the small thing he holds. holding the grab button with the
     * stick pushed starts it, and releasing the stick ends it */
    TAILS_HOLD_SHAKE,
    TAILS_HANG, /* MD_HGWT: hanging still from a path */
    TAILS_HANG_GO, /* MD_HGGO: moving along that path */
    TAILS_LEDGE, /* MD_LDWT: hanging still from a ledge */
    TAILS_LEDGE_GO, /* MD_LDGO: moving along that ledge */
    TAILS_PANEL_JUMP, /* MD_PNJP: jumping from a panel to the next */
    TAILS_PANEL_WAIT, /* MD_PNWT: standing on a panel */
    /* held, struggling. each step he presses PAD_MASH_BUTTONS takes 20 off
     * struggle, and he's free once it's 0. nothing in this file enters the
     * mode */
    TAILS_CAUGHT,
    /* in a module's ride action, from PLAYER_MODE_RIDE till
     * PLAYER_MODE_RIDE_OFF */
    TAILS_MODULE_POSE,
    /* from PLAYER_MODE_CIRCLE, he holds on beside the task in record->held.
     * in TAILS_CIRCLE he rests */
    TAILS_CIRCLE,
    TAILS_CIRCLE_DOWN, /* coming down its side to the bottom of its shape */
    TAILS_CIRCLE_LEFT, /* going round it, the stick pushed left */
    TAILS_CIRCLE_RIGHT, /* going round it, the stick pushed right */
    TAILS_BOARD, /* MD_SLID: riding his board */
    TAILS_UNK_49, /* MD_FSLD. nothing in this file enters it */
    TAILS_BOARD_BRAKE, /* MD_BRAK */
    TAILS_BOARD_FALL, /* MD_SFAL */
    TAILS_BOARD_JUMP, /* MD_SJMP */
    TAILS_BOARD_TRICK, /* MD_SPEC: off a ramp */
    TAILS_BOARD_CRASH, /* MD_CRSH */
    TAILS_SLOPE_ROLL, /* curled up, as gravity alone rolls him */
    TAILS_SLOPE_ROLL_FALL, /* curled up in the air, after a slope roll */
    TAILS_PLAY, /* playing the action a request gave him, to its end */
    TAILS_EVENT, /* an event runs him along its paths */
    TAILS_RISE, /* out of air: ACTION_DROWN, then up to the water's top */
    /* from PLAYER_MODE_DOWN, past which tails_step chooses no mode */
    TAILS_DOWN,
    TAILS_HOVER, /* from PLAYER_MODE_HOVER: still in the air */
    TAILS_CLEAR, /* the clear's pose */
    TAILS_EDIT /* the editor's. nothing in this file enters it */
};

/* the rows of tails_actions his code names */
enum tails_action_index {
    ACTION_STAND = 0,
    ACTION_LAND = 1, /* landing still, then ACTION_LANDED */
    ACTION_LANDED = 2,
    ACTION_STAND_ONCE = 3,
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
    ACTION_JUMP = 14, /* curled up, spinning at action_speed */
    ACTION_ROLL = 15, /* curled up, spinning by his speed */
    ACTION_BALL = 16, /* drawn on BALL_TICKS' steps while he's curled up */
    ACTION_SPRING = 17,
    ACTION_FALL_START = 18,
    ACTION_FALL = 19,
    ACTION_HARD_LANDING = 20,
    ACTION_SKID = 21,
    ACTION_TURN = 22,
    ACTION_SLIDE = 23,
    ACTION_PUSH_START = 24,
    ACTION_PUSH = 25,
    ACTION_HURT = 26,
    ACTION_HURT_LAND = 27,
    ACTION_DOWN = 28,
    ACTION_BREATHE = 30,
    ACTION_DROWN = 31,
    ACTION_UPDRAFT = 33,
    ACTION_CRASH = 34,
    ACTION_CRASH_LAND = 35,
    ACTION_UNCURL = 36,
    /* his flight's rows. once FLIGHT_TIRING of his flight is spent,
     * ACTION_FLY_TIRING leads from ACTION_FLY into ACTION_FLY_TIRED.
     * ACTION_TIRED plays ACTION_FLY_TIRED's action as he falls in
     * TAILS_TIRED, his flight spent */
    ACTION_FLY = 37,
    ACTION_FLY_TIRING = 38,
    ACTION_FLY_TIRED = 39,
    ACTION_FLY_FAST = 40,
    ACTION_HOVER = 41,
    ACTION_TIRED = 42,
    /* his tail attack's blows: the first, then two still ones, then from
     * ACTION_ATTACK_RIGHT eight the stick picks, an eighth of a turn apart.
     * they go right, down right, down, down left, left, up left, up and up
     * right */
    ACTION_ATTACK = 43,
    ACTION_ATTACK_STILL_A = 44,
    ACTION_ATTACK_STILL_B = 45,
    ACTION_ATTACK_RIGHT = 46,
    ACTION_ATTACK_UP_RIGHT = 53,
    ACTION_CLEAR = 54,
    ACTION_LOST = 56,
    ACTION_LIFT = 58,
    ACTION_HOLD_RUN = 59,
    ACTION_HOLD_SKID = 60,
    ACTION_HOLD_TURN = 61,
    ACTION_HOLD_STAND = 62,
    ACTION_HOLD_WALK = 63,
    ACTION_BOUNCE_1 = 64,
    ACTION_BOUNCE_1_FALL = 65,
    ACTION_BOUNCE_2 = 66,
    ACTION_BOUNCE_2_FALL = 67,
    ACTION_SQUASH_2 = 69,
    ACTION_SQUASH_3 = 70,
    ACTION_HANG_ON = 72,
    ACTION_PULL = 73,
    ACTION_SHAKE = 74,
    ACTION_HOLD_SHAKE_START = 75,
    ACTION_HOLD_SHAKE_END = 77,
    ACTION_GRIP_START = 78,
    ACTION_GRIP = 79,
    ACTION_HOLD_SPRING = 80,
    ACTION_HOLD_FALL_START = 81,
    ACTION_HOLD_FALL = 82,
    ACTION_HOLD_HARD_LANDING = 83,
    ACTION_THROW_AT = 84, /* at the nearest task */
    ACTION_THROW = 85, /* ahead */
    ACTION_AIR_THROW_AT = 86,
    ACTION_AIR_THROW = 87,
    ACTION_PUT_DOWN = 88,
    ACTION_RIDE = 89,
    ACTION_HANG_GO = 90,
    ACTION_HANG = 91,
    ACTION_HANG_SWAPPED = 92,
    ACTION_LEDGE = 93,
    ACTION_FLUNG = 94,
    ACTION_PANEL_JUMP = 95,
    ACTION_PANEL_WAIT = 96,
    ACTION_ROCKET = 97,
    ACTION_ROCKET_ALONG = 99,
    ACTION_ROCKET_UP = 100,
    ACTION_CIRCLE = 102,
    ACTION_CIRCLE_LEFT = 103,
    ACTION_CIRCLE_RIGHT = 104,
    ACTION_CIRCLE_DOWN = 105,
    ACTION_CIRCLE_LAND = 106,
    /* on his board, from ACTION_BOARD to ACTION_BOARD_CROUCH, the board
     * follows him and plays tails_board_actions' action */
    ACTION_BOARD = 107,
    /* two rows alike: braking, as the stick turns one way or the other */
    ACTION_BOARD_BRAKE_A = 108,
    ACTION_BOARD_BRAKE_B = 109,
    ACTION_BOARD_FALL_START = 110,
    ACTION_BOARD_JUMP = 111,
    ACTION_BOARD_LEAN_RIGHT = 112, /* drive above 0: the stick pushed right */
    ACTION_BOARD_LEAN_LEFT = 113,
    /* a crash into a wall, by the side it meets him on. hurt, he takes
     * ACTION_BOARD_CRASH_B */
    ACTION_BOARD_CRASH_A = 114,
    ACTION_BOARD_CRASH_B = 115,
    ACTION_BOARD_FALL = 116,
    ACTION_BOARD_TRICK_FALL = 117, /* after a trick, or as one hits a wall */
    ACTION_BOARD_HARD_LANDING = 118,
    ACTION_BOARD_SLOW = 119,
    ACTION_BOARD_SLOW_ONCE = 120,
    ACTION_BOARD_TRICK_1 = 121,
    ACTION_BOARD_TRICK_2 = 122,
    ACTION_BOARD_TRICK_3 = 123,
    ACTION_BOARD_TRICK_4 = 126,
    ACTION_BOARD_CROUCH = 127,
    ACTION_SLOPE_ROLL = 128,
    ACTION_SHOWER = 129,
    ACTION_SHOWER_END = 131,
    ACTION_MUD_OUT = 132 /* past tails_actions' last row */
};

#define SE_TAIL_ATTACK 734
#define SE_CRASH 1128
#define SE_LIFT 1130
#define SE_THROW 1131
#define SE_PUSH_START 1132
#define SE_FLY 1133
#define SE_HURT 1139
#define SE_TIRED 1384
/* his board's tricks */
#define SE_TRICK_1 1359
#define SE_TRICK_2 1360
#define SE_TRICK_3 1378
#define SE_TRICK_4 1379

/* his clear's sound, and his voice when he loses a race: VOICE_HIGHWAY_LOST
 * on Speed Highway, and VOICE_LOST in his other races */
#define SE_CLEAR 1362
#define VOICE_HIGHWAY_LOST 229
#define VOICE_LOST 218

/* the steps he can climb, the jump button held, before his flight is
 * spent */
#define FLIGHT_STEPS 120
/* once this much of his flight is spent, his tails spin slower, or not at
 * all while frame_step_count is 1, and his flight's phase steps half as
 * fast */
#define FLIGHT_TIRING 0.4F

/* standing still this many steps, he fidgets */
#define FIDGET_STEPS 1800

/* in his attack's pose, the nodes of his two tails' ends */
#define TAIL_END_A 66
#define TAIL_END_B 62

/* how far the phase that bends tail 0 runs ahead of tail 1's */
#define TAIL_LEAD 0x5000

/* how far he holds on from what he goes round, when it has no motion to
 * give a reach */
#define CIRCLE_REACH 30.0F

/* the steps a later voice waits */
#define VOICE_STEPS 6

/* the bit of his hurt timer that hides him. as the timer counts down from
 * PLAYER_HURT_STEPS, it hides him two steps in every four, so he blinks.
 * the image extends the timer unsigned wherever it extends it (EXTU.W at
 * 0x8c0bf6f4, 0x8c0bf768, 0x8c0bf7c8 and 0x8c0c1fb0), so his code reads it
 * through TASK_TIMER */
#define BLINK_HIDDEN 0x2

/* the bits of work->tick that draw him as ACTION_BALL while he's curled
 * up. he's drawn so while either is set, on 24 steps in every 32 */
#define BALL_TICKS 0x11

/* how far each afterimage of his tails turns back from the one before, in
 * flight until FLIGHT_TIRING of it is spent. later in flight, or with his
 * spin under 1, they turn less */
#define AFTERIMAGE_TURN 0x2000

/* how far his flying tails spin a step, twice that while frame_step_count
 * is 2 */
#define TAIL_SPIN_STEP 0x800

/* with no spin left, how much farther his waving tails turn a step as they
 * settle, beyond the 0x333 they turn anyway */
#define TAIL_SETTLE_STEP 0x333
/* how far past rest tail 0 comes as they settle, before both stop at 0x333,
 * or at rest while the stage is paused */
#define TAIL_SETTLE_ZONE 0x1000

/* the record of the board's rider */
/* @hack type: player_get called through a pointer to a function of a
 * Sint8, so the number MOV.B sign-extends goes to it as it is (MOV R0,R4
 * at 0x8c0c7750); through player.h's Uint8 prototype, SHC adds an EXTU.B,
 * the function runs 2 bytes long and 81 of 91 words differ */
#ifdef GCC
/* keeps the shipped play: GCC calls player_get through player.h's
 * prototype, and the board's player number is always 0, which the two
 * types pass alike. heap_calloc zeroes the work, and nothing sets the
 * number */
#define BOARD_PLAYER(work) (player_get(BOARD_VARS(work)->player))
#else
#define BOARD_PLAYER(work)                                                     \
    (((struct player * (*)(Sint8)) player_get)(BOARD_VARS(work)->player))
#endif

/* how far his board sits below his middle */
#define BOARD_DEPTH (-5.190199F)

/* the nodes a look task turns: his head and his two pupils */
struct tails_look_nodes {
    NJS_OBJECT *head;
    NJS_OBJECT *eye0;
    NJS_OBJECT *eye1;
};

struct board_vars {
    Sint8 player; /* whose board it is */
    Uint8 unk_01[3];
    NJS_OBJECT *object;
    NJS_TEXLIST *texlist;
};

#define BOARD_VARS(work) ((struct board_vars *)&(work)->vars)

/* a voice that waits some steps before it asks to be played */
struct voice_later {
    Sint32 steps;
    Sint32 voice;
};

#define VOICE_LATER(task) ((struct voice_later *)(task)->extra)

/* the camera's path round Sonic as he clears, which tails_clear starts for
 * Tails */
extern struct path sonic_clear_path;
/* his model, and the one he flies and attacks with */
/* @hack type: the pieces declared as arrays, whose address SHC loads to
 * set or read a member at a displacement (MOV.L R2,@(48,R1) at
 * 0x8c0be2a4); declared as objects, it folds the member into the pool
 * word, and summed over the nine functions that use them, 826 of 2836
 * words differ */
extern NJS_OBJECT tails_object;
extern NJS_OBJECT tails_upper_arm_minus_z;
extern NJS_OBJECT tails_forearm_minus_z;
extern NJS_OBJECT tails_cuff_minus_z;
extern NJS_OBJECT tails_hand_minus_z;
extern NJS_OBJECT tails_fingers_minus_z;
extern NJS_OBJECT tails_upper_arm_plus_z;
extern NJS_OBJECT tails_forearm_plus_z;
extern NJS_OBJECT tails_cuff_plus_z;
extern NJS_OBJECT tails_hand_plus_z;
extern NJS_OBJECT tails_fingers_plus_z;
extern NJS_OBJECT tails_thigh_minus_z;
extern NJS_OBJECT tails_shin_minus_z;
extern NJS_OBJECT tails_foot_minus_z[];
extern NJS_OBJECT tails_shoe_minus_z[];
extern NJS_OBJECT tails_toes_minus_z;
extern NJS_OBJECT tails_thigh_plus_z;
extern NJS_OBJECT tails_shin_plus_z;
extern NJS_OBJECT tails_foot_plus_z[];
extern NJS_OBJECT tails_shoe_plus_z[];
extern NJS_OBJECT tails_toes_plus_z;
extern NJS_OBJECT tails_head_joint;
extern NJS_OBJECT tails_pupil_plus_z;
extern NJS_OBJECT tails_pupil_minus_z;
extern NJS_OBJECT tails_body[];
extern NJS_MODEL tails_body_model;
extern NJS_OBJECT tails_fly_object;
extern NJS_OBJECT tails_fly_upper_arm_minus_z;
extern NJS_OBJECT tails_fly_forearm_minus_z;
extern NJS_OBJECT tails_fly_cuff_minus_z;
extern NJS_OBJECT tails_fly_hand_minus_z;
extern NJS_OBJECT tails_fly_fingers_minus_z;
extern NJS_OBJECT tails_fly_upper_arm_plus_z;
extern NJS_OBJECT tails_fly_forearm_plus_z;
extern NJS_OBJECT tails_fly_cuff_plus_z;
extern NJS_OBJECT tails_fly_hand_plus_z;
extern NJS_OBJECT tails_fly_fingers_plus_z;
extern NJS_OBJECT tails_fly_thigh_minus_z;
extern NJS_OBJECT tails_fly_shin_minus_z;
extern NJS_OBJECT tails_fly_foot_minus_z[];
extern NJS_OBJECT tails_fly_shoe_minus_z[];
extern NJS_OBJECT tails_fly_toes_minus_z;
extern NJS_OBJECT tails_fly_thigh_plus_z;
extern NJS_OBJECT tails_fly_shin_plus_z;
extern NJS_OBJECT tails_fly_foot_plus_z[];
extern NJS_OBJECT tails_fly_shoe_plus_z[];
extern NJS_OBJECT tails_fly_toes_plus_z;
extern NJS_OBJECT tails_fly_head_joint;
extern NJS_OBJECT tails_fly_pupil_minus_z;
extern NJS_OBJECT tails_fly_pupil_plus_z;
extern NJS_OBJECT tails_fly_body[];
extern NJS_MODEL tails_fly_body_model;
extern NJS_OBJECT tails_jet_anklet_minus_z[];
extern NJS_OBJECT tails_jet_anklet_plus_z[];
extern NJS_MODEL tails_brooch_body_model;
/* his two tails, each spun about x, and each tail's end piece. an end
 * piece takes its own model, the flying tail's, a waving copy, or a blend
 * of the waving copy and the flying tail's.
 * tails_wave_tails bends both waving copies from one shape */
extern NJS_OBJECT tails_tail0[];
extern NJS_OBJECT tails_tail0_end[];
extern NJS_MODEL tails_tail0_model;
extern NJS_OBJECT tails_tail1[];
extern NJS_OBJECT tails_tail1_end[];
extern NJS_MODEL tails_tail1_model;
extern NJS_OBJECT tails_fly_tail0[];
extern NJS_MODEL tails_fly_tail0_model[];
extern NJS_OBJECT tails_fly_tail1[];
extern NJS_MODEL tails_fly_tail1_model[];
extern NJS_MODEL tails_tail_wave_shape[];
extern NJS_MODEL tails_tail0_wave_model[];
extern NJS_MODEL tails_tail1_wave_model[];
extern NJS_MODEL tails_tail0_blend_model[];
extern NJS_MODEL tails_tail1_blend_model[];
#ifdef GCC
/* the face normals model_calc_face_normals writes for the two waving copies
 * and the two blends: tails_tail1_wave_model's, tails_tail0_wave_model's,
 * tails_tail1_blend_model's, then tails_tail0_blend_model's. each model's
 * three meshsets point their normals into 14 of them, one for each strip.
 * 1ST_READ.BIN's tables place the four models' 56 in bss, where no source
 * holds them. the port defines them here, and repairs a write past them
 * with 7 more. tails_tail0_blend_model's third meshset points its normals
 * at the 54th. blending it (JSR to model_blend at 0x8c0c270a) writes each
 * strip's normals from there, as util.c's bug has it, and the longest
 * writes 10. in the image the 7 land on knuckles.c's bss from 0x8c7ab3a0,
 * and the 60th on knuckles_dig_last_pos. knuckles_set_dig_chances then
 * takes that normal for where Knuckles last dug out */
NJS_VECTOR tails_tail_face_normals[56 + 7];
#endif
/* for each of his two models, the node a mouth task takes as his face,
 * whose sibling is his head. then his head, and its model. after them come
 * the head the mouth task blends into, the shapes his mouth moves through,
 * and the head model his sway task writes */
extern NJS_OBJECT tails_mouth;
extern NJS_OBJECT tails_head[];
extern NJS_MODEL tails_head_model;
extern NJS_OBJECT tails_fly_mouth;
extern NJS_OBJECT tails_fly_head[];
extern NJS_MODEL tails_fly_head_model;
extern NJS_OBJECT tails_head_blend;
extern NJS_MOTION tails_mouth_shapes;
extern NJS_MODEL tails_head_sway_model;

extern NJS_ACTION tails_stand_action;
extern NJS_ACTION tails_fidget_start_action;
extern NJS_ACTION tails_fidget_action;
extern NJS_ACTION tails_stand_uphill_action;
extern NJS_ACTION tails_stand_downhill_action;
extern NJS_ACTION tails_walk_action;
extern NJS_ACTION tails_run_action;
extern NJS_ACTION tails_sprint_action;
extern NJS_ACTION tails_spin_action;
extern NJS_ACTION tails_ball_action;
extern NJS_ACTION tails_spring_action;
extern NJS_ACTION tails_fall_start_action;
extern NJS_ACTION tails_fall_action;
extern NJS_ACTION tails_hard_landing_action;
extern NJS_ACTION tails_skid_action;
extern NJS_ACTION tails_turn_action;
extern NJS_ACTION tails_push_action;
extern NJS_ACTION tails_hurt_action;
extern NJS_ACTION tails_hurt_land_action;
extern NJS_ACTION tails_down_action;
/* a loop from where his down ends */
extern NJS_ACTION tails_down_land_action;
extern NJS_ACTION tails_breathe_action;
extern NJS_ACTION tails_updraft_action;
extern NJS_ACTION tails_crash_action;
extern NJS_ACTION tails_crash_land_action;
extern NJS_ACTION tails_uncurl_action;
extern NJS_ACTION tails_fly_action;
extern NJS_ACTION tails_fly_tire_action;
extern NJS_ACTION tails_fly_tired_action;
extern NJS_ACTION tails_fly_fast_action;
extern NJS_ACTION tails_attack_action;
extern NJS_ACTION tails_attack_still_a_action;
extern NJS_ACTION tails_attack_still_b_action;
extern NJS_ACTION tails_attack_right_action;
extern NJS_ACTION tails_attack_down_right_action;
extern NJS_ACTION tails_attack_down_action;
extern NJS_ACTION tails_attack_down_left_action;
extern NJS_ACTION tails_attack_left_action;
extern NJS_ACTION tails_attack_up_left_action;
extern NJS_ACTION tails_attack_up_action;
extern NJS_ACTION tails_attack_up_right_action;
extern NJS_ACTION tails_clear_action;
/* from where his clear ends back to where it starts */
extern NJS_ACTION tails_clear_end_action;
extern NJS_ACTION tails_lost_action;
extern NJS_ACTION tails_lost_loop_action;
extern NJS_ACTION tails_lift_action;
extern NJS_ACTION tails_hold_run_action;
extern NJS_ACTION tails_hold_skid_action;
extern NJS_ACTION tails_hold_turn_action;
extern NJS_ACTION tails_hold_stand_action;
extern NJS_ACTION tails_hold_walk_action;
extern NJS_ACTION tails_pull_action;
extern NJS_ACTION tails_shake_action;
extern NJS_ACTION tails_hold_shake_start_action;
extern NJS_ACTION tails_hold_shake_action;
extern NJS_ACTION tails_hold_shake_end_action;
extern NJS_ACTION tails_grip_action;
extern NJS_ACTION tails_grip_hold_action;
extern NJS_ACTION tails_throw_at_target_action;
extern NJS_ACTION tails_throw_action;
extern NJS_ACTION tails_hang_go_action;
extern NJS_ACTION tails_hang_action;
extern NJS_ACTION tails_hang_swapped_action;
extern NJS_ACTION tails_ledge_action;
extern NJS_ACTION tails_flung_action;
extern NJS_ACTION tails_rocket_along_action;
extern NJS_ACTION tails_rocket_up_action;
extern NJS_ACTION tails_circle_action;
extern NJS_ACTION tails_circle_left_action;
extern NJS_ACTION tails_circle_right_action;
extern NJS_ACTION tails_circle_down_action;
extern NJS_ACTION tails_circle_land_action;

static void tails_board_score(int points);
static void tails_stand(struct task_work *work, struct player *record);
static void tails_choose_mode(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static void tails_destroy(struct task *task);
static void tails_move_shape(struct task_work *work, struct player *record);
static void tails_draw(struct task *task);
static void tails_set_stand_action(struct task_work *work,
                                   struct player *record);
static void tails_set_walk_action(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record);
static void tails_hang_move(struct task_work *work, struct task_motion *motion,
                            struct player *record);
static void tails_ledge_move(struct task_work *work, struct task_motion *motion,
                             struct player *record);
static void tails_set_board_action(struct task_work *work,
                                   struct task_motion *motion,
                                   struct player *record);
static void tails_board_sound(struct task_work *work, struct player *record);
static Bool tails_try_hurt(struct task_work *work, struct player *record);
static Bool tails_try_jump(struct task_work *work, struct player *record);
static Bool tails_try_uncurl(struct task_work *work, struct player *record);
static Bool tails_follow_event(struct task_work *work);
static Bool tails_try_request(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static Bool tails_try_request_holding(struct task_work *work,
                                      struct task_motion *motion,
                                      struct player *record);
static Bool tails_try_request_board(struct task_work *work,
                                    struct task_motion *motion,
                                    struct player *record);
static Bool tails_try_fall(struct task_work *work, struct player *record);
static Bool tails_try_stop(struct task_work *work, struct player *record);
static Bool tails_try_unroll(struct task_work *work, struct player *record);
static Bool tails_try_grab(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static Bool tails_try_take_lifted(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record);
static Bool tails_try_throw(struct task_work *work, struct task_motion *motion,
                            struct player *record);
static Bool tails_try_release(struct task_work *work, struct player *record);
static Bool tails_try_grip(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static void tails_let_go(struct task_work *work, struct player *record);
static void tails_carry(struct task_work *work, struct player *record);
static void tails_circle_place(struct task_work *work, struct player *record);
static int tails_circle_step(struct player *record);
static Bool tails_try_roll(struct task_work *work, struct player *record);
static Bool tails_try_tail_attack(struct task_work *work,
                                  struct player *record);
static void tails_fly(struct task_work *work, struct task_motion *motion,
                      struct player *record);
static void tails_spin_tails(struct task_work *work, struct player *record);
static void tails_wave_tails(void);
static void tails_reset_tails(void);
static void tails_bend_points(Float rate, NJS_POINT3 *source,
                              NJS_POINT3 *destination, int phase, int count);
static Bool tails_look_create(Sint8 player, struct tails_look_nodes *nodes);
static Bool tails_fly_look_create(Sint8 player, struct tails_look_nodes *nodes);
static void tails_look_destroy(struct task *task);
static void tails_look_step(struct task *task);
static void tails_fly_look_step(struct task *task);
static void tails_look_move_eyes(struct player_look_motion *look);
static void tails_attack_next(struct task_work *work, struct player *record);
static void tails_attack_trails(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record);
static void tails_set_shapes(struct task_work *work, struct player *record);
static void tails_sway_destroy(struct task *task);
static void tails_sway_step(struct task *task);
static void tails_mouth_start(struct task *task);
static void tails_mouth_step(struct task *task);
static void tails_board_step(struct task *task);
static void tails_board_draw(struct task *task);
static void tails_board_destroy(struct task *task);
static void tails_sound(struct task_work *work, int se);
static void tails_push_sound(struct task_work *work);
static void tails_push_sound_stop(struct task_work *work);
static void tails_voice_later(int voice);
static void tails_voice_step(struct task *task);

/* the names of his modes, which nothing reads. they open this object's
 * data */
static char tails_mode_names[][8] = {
    "MD_INIT", "MD_STND", "MD_WALK", "MD_1DWK", "MD_JUMP", "MD_SPRG", "MD_LUNC",
    "MD_FALL", "MD_STOP", "MD_PUSH", "MD_DAMG", "MD_BACK", "MD_FLY_", "MD_CRSH",
    "MD_PLON", "MD_TIRE", "MD_TATK", "MD_PICK", "MD_HOLD", "MD_TOCH", "MD_HWLK",
    "MD_THRW", "MD_HSTP", "MD_HFLL", "MD_HJMP", "MD_HSPR", "MD_HLNC", "MD_PULL",
    "MD_THRU", "MD_SHAK", "MD_HGWT", "MD_HGGO", "MD_LDWT", "MD_LDGO", "MD_PNJP",
    "MD_PNWT", "EVENT",   "MD_KILL", "MD_SLID", "MD_FSLD", "MD_BRAK", "MD_SFAL",
    "MD_SJMP", "MD_SPEC", "MD_CRSH", "MD_EDIT",
};

/* his actions, indexed by enum tails_action_index. as he attacks,
 * tails_attack_next rewrites the next field of each tail attack blow's
 * row */
static struct player_action tails_actions[] = {
    {&tails_stand_action, 68, 3, 0, 0.0625F, 0.5F},
    {&tails_stand_action, 68, 4, 2, 0.25F, 2.0F},
    {&tails_stand_action, 68, 3, 2, 0.25F, 1.0F},
    {&tails_stand_action, 68, 4, 0, 0.25F, 1.0F},
    {&tails_fidget_start_action, 68, 4, 5, 0.25F, 0.25F},
    {&tails_fidget_action, 68, 4, 6, 0.25F, 0.25F},
    {&tails_fidget_start_action, 68, 6, 0, 0.25F, 0.25F},
    {&tails_stand_uphill_action, 68, 3, 7, 0.25F, 1.0F},
    {&tails_stand_downhill_action, 68, 3, 8, 0.25F, 1.0F},
    {&tails_walk_action, 68, 9, 9, 0.25F, 3.4F},
    {&tails_run_action, 68, 9, 10, 0.25F, 1.4F},
    {&tails_run_action, 68, 9, 11, 0.25F, 1.0F},
    {&tails_sprint_action, 68, 9, 12, 0.25F, 1.0F},
    {&tails_sprint_action, 68, 9, 13, 0.25F, 1.0F},
    {&tails_spin_action, 68, 10, 14, 0.25F, 0.4F},
    {&tails_spin_action, 68, 9, 15, 0.25F, 0.7F},
    {&tails_ball_action, 1, 3, 16, 0.5F, 1.0F},
    {&tails_spring_action, 68, 3, 17, 0.25F, 1.0F},
    {&tails_fall_start_action, 68, 4, 19, 0.125F, 1.0F},
    {&tails_fall_action, 68, 3, 19, 0.25F, 1.0F},
    {&tails_hard_landing_action, 68, 4, 0, 1.0F, 1.0F},
    {&tails_skid_action, 68, 4, 0, 0.25F, 1.0F},
    {&tails_turn_action, 68, 4, 0, 1.0F, 1.0F},
    {&tails_walk_action, 68, 3, 23, 0.0625F, 1.0F},
    {&tails_push_action, 68, 4, 25, 0.125F, 3.0F},
    {&tails_stand_action, 68, 3, 25, 0.25F, 1.0F},
    {&tails_hurt_action, 68, 5, 26, 1.0F, 1.0F},
    {&tails_hurt_land_action, 68, 4, 1, 0.125F, 1.0F},
    {&tails_down_action, 68, 5, 28, 0.125F, 1.0F},
    {&tails_down_land_action, 68, 3, 29, 0.125F, 1.0F},
    {&tails_breathe_action, 68, 4, 19, 1.0F, 0.25F},
    {&tails_breathe_action, 68, 5, 31, 1.0F, 0.5F},
    {&tails_skid_action, 68, 3, 32, 0.25F, 1.0F},
    {&tails_updraft_action, 68, 3, 33, 0.0625F, 1.0F},
    {&tails_crash_action, 68, 4, 35, 0.25F, 1.0F},
    {&tails_crash_land_action, 68, 4, 0, 0.0625F, 1.0F},
    {&tails_uncurl_action, 68, 4, 0, 0.0625F, 1.0F},
    {&tails_fly_action, 68, 3, 37, 0.0625F, 0.3F},
    {&tails_fly_tire_action, 68, 4, 39, 1.0F, 0.4F},
    {&tails_fly_tired_action, 68, 3, 39, 0.0625F, 0.5F},
    {&tails_fly_fast_action, 68, 3, 40, 0.0625F, 0.3F},
    {&tails_fly_fast_action, 68, 3, 41, 0.0625F, 0.3F},
    {&tails_fly_tired_action, 68, 3, 42, 0.0625F, 0.5F},
    {&tails_attack_action, 68, 4, 0, 0.125F, 1.0F},
    {&tails_attack_still_a_action, 68, 4, 0, 0.125F, 1.0F},
    {&tails_attack_still_b_action, 68, 4, 0, 0.125F, 1.0F},
    {&tails_attack_right_action, 68, 4, 0, 0.125F, 1.0F},
    {&tails_attack_down_right_action, 68, 4, 0, 0.125F, 1.0F},
    {&tails_attack_down_action, 68, 4, 0, 0.125F, 1.0F},
    {&tails_attack_down_left_action, 68, 4, 0, 0.125F, 1.0F},
    {&tails_attack_left_action, 68, 4, 0, 0.125F, 1.0F},
    {&tails_attack_up_left_action, 68, 4, 0, 0.125F, 1.0F},
    {&tails_attack_up_action, 68, 4, 0, 0.125F, 1.0F},
    {&tails_attack_up_right_action, 68, 4, 0, 0.125F, 1.0F},
    {&tails_clear_action, 68, 5, 54, 0.5F, 1.0F},
    {&tails_clear_end_action, 68, 4, 0, 0.5F, 1.0F},
    {&tails_lost_action, 68, 4, 57, 0.5F, 1.0F},
    {&tails_lost_loop_action, 68, 3, 57, 0.5F, 1.0F},
    {&tails_lift_action, 68, 4, 62, 0.5F, 1.0F},
    {&tails_hold_run_action, 68, 3, 59, 0.5F, 1.0F},
    {&tails_hold_skid_action, 68, 4, 62, 0.5F, 1.0F},
    {&tails_hold_turn_action, 68, 4, 62, 0.5F, 1.0F},
    {&tails_hold_stand_action, 68, 3, 62, 0.5F, 0.5F},
    {&tails_hold_walk_action, 68, 3, 63, 0.5F, 1.0F},
    {&tails_spring_action, 68, 3, 17, 0.25F, 1.0F},
    {&tails_fall_start_action, 68, 4, 19, 0.5F, 1.0F},
    {&tails_spring_action, 68, 3, 17, 0.25F, 1.0F},
    {&tails_fall_start_action, 68, 4, 19, 0.5F, 1.0F},
    {&tails_fall_action, 68, 3, 17, 0.25F, 1.0F},
    {&tails_spring_action, 68, 4, 17, 0.5F, 1.0F},
    {&tails_hard_landing_action, 68, 4, 17, 0.5F, 1.0F},
    {&tails_stand_action, 68, 4, 9, 0.5F, 0.5F},
    {&tails_stand_action, 68, 3, 72, 0.5F, 0.5F},
    {&tails_pull_action, 68, 3, 73, 0.5F, 1.0F},
    {&tails_shake_action, 68, 3, 74, 0.5F, 0.5F},
    {&tails_hold_shake_start_action, 68, 4, 76, 0.5F, 0.5F},
    {&tails_hold_shake_action, 68, 3, 76, 0.5F, 0.5F},
    {&tails_hold_shake_end_action, 68, 4, 62, 0.5F, 0.5F},
    {&tails_grip_action, 68, 4, 79, 0.5F, 1.0F},
    {&tails_grip_hold_action, 68, 3, 79, 0.5F, 1.0F},
    {&tails_lift_action, 68, 3, 80, 0.5F, 1.0F},
    {&tails_lift_action, 68, 4, 82, 0.5F, 1.0F},
    {&tails_lift_action, 68, 3, 82, 0.5F, 1.0F},
    {&tails_lift_action, 68, 4, 62, 0.5F, 1.0F},
    {&tails_throw_at_target_action, 68, 4, 0, 0.5F, 1.0F},
    {&tails_throw_action, 68, 4, 0, 0.5F, 1.0F},
    {&tails_throw_at_target_action, 68, 4, 19, 0.5F, 1.0F},
    {&tails_throw_action, 68, 4, 19, 0.5F, 1.0F},
    {&tails_lift_action, 68, 6, 0, 1.0F, 1.0F},
    {&tails_cart_action, 68, 5, 89, 1.0F, 0.3F},
    {&tails_hang_go_action, 68, 12, 90, 1.0F, 0.3F},
    {&tails_hang_action, 68, 3, 91, 0.25F, 0.25F},
    {&tails_hang_swapped_action, 68, 3, 92, 0.25F, 0.25F},
    {&tails_ledge_action, 68, 12, 93, 1.0F, 0.5F},
    {&tails_flung_action, 68, 3, 94, 1.0F, 0.5F},
    {&tails_spring_action, 68, 3, 95, 0.25F, 1.0F},
    {&tails_stand_action, 68, 3, 96, 0.25F, 1.0F},
    {&tails_grip_action, 68, 4, 98, 0.125F, 1.0F},
    {&tails_grip_hold_action, 68, 3, 98, 0.125F, 1.0F},
    {&tails_rocket_along_action, 68, 3, 99, 0.125F, 1.0F},
    {&tails_rocket_up_action, 68, 3, 100, 0.125F, 1.0F},
    {&tails_hurt_action, 68, 5, 101, 1.0F, 15.0F},
    {&tails_circle_action, 68, 3, 102, 0.25F, 0.5F},
    {&tails_circle_left_action, 68, 3, 103, 0.25F, 2.0F},
    {&tails_circle_right_action, 68, 3, 104, 0.25F, 2.0F},
    {&tails_circle_down_action, 68, 5, 105, 0.25F, 0.5F},
    {&tails_circle_land_action, 68, 4, 102, 0.25F, 0.5F},
    {&dat_0cae1c44, 68, 3, 107, 0.0625F, 1.0F},
    {&dat_0cae1c44, 68, 5, 108, 0.125F, 1.0F},
    {&dat_0cae1c44, 68, 5, 109, 0.125F, 1.0F},
    {&dat_0cae63e0, 68, 4, 116, 0.0625F, 1.0F},
    {&dat_0cae4a2c, 68, 4, 110, 0.25F, 1.0F},
    {&dat_0caec15c, 68, 12, 112, 1.0F, 1.0F},
    {&dat_0caea6b8, 68, 12, 113, 1.0F, 1.0F},
    {&dat_0caeec50, 68, 4, 119, 1.0F, 1.0F},
    {&dat_0cae8c14, 68, 4, 119, 1.0F, 1.0F},
    {&dat_0cadd73c, 68, 3, 117, 0.5F, 0.3F},
    {&dat_0cadd73c, 68, 3, 117, 0.5F, 0.3F},
    {&dat_0cae0210, 68, 4, 107, 0.5F, 1.0F},
    {&dat_0cae1c44, 68, 3, 119, 0.0625F, 1.0F},
    {&dat_0cae1c44, 68, 4, 107, 0.0625F, 1.0F},
    {&dat_0caf883c, 68, 4, 117, 0.0625F, 0.8F},
    {&dat_0cb02590, 68, 4, 117, 0.0625F, 1.1F},
    {&dat_0caf25a4, 68, 4, 124, 1.0F, 1.5F},
    {&dat_0caf25a4, 68, 4, 125, 1.0F, 1.5F},
    {&dat_0caf25a4, 68, 4, 117, 0.25F, 1.5F},
    {&dat_0caf5dc8, 68, 4, 117, 0.25F, 1.5F},
    {&dat_0cae3658, 68, 3, 127, 0.25F, 1.5F},
    {&tails_spin_action, 68, 10, 128, 0.25F, 0.2F},
    {&tails_stand_action, 68, 4, 130, 0.25F, 0.25F},
    {&tails_stand_action, 68, 3, 130, 0.25F, 0.25F},
    {&tails_stand_action, 68, 4, 0, 0.25F, 0.25F},
};

/* his shapes: his body, then the end of each tail, then the disc his tails
 * spin in above him as he flies. the tails' ends strike in his tail
 * attack, and the disc as he flies. tails_set_shapes turns those three on
 * and off */
/* clang-format off */
static struct collision_shape tails_shapes[4] = {
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS,
     PUSH_ALL | PUSH_FIRM(PUSH_OTHERS), {0.0F, 1.0F, 0.0F}, 4.0F, 4.0F, 0.65F,
     {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_OFF | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(1) | GUARD(2) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES |
     SHAPE_HURTS_OBJECTS,
     PUSH_ENEMIES | PUSH_FIRM(PUSH_OTHERS), {0.0F, 0.0F, 0.0F}, 6.0F, 0.0F,
     0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_OFF | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(1) | GUARD(2) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES |
     SHAPE_HURTS_OBJECTS,
     PUSH_ENEMIES | PUSH_FIRM(PUSH_OTHERS), {0.0F, 0.0F, 0.0F}, 6.0F, 0.0F,
     0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_OFF | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(1) | GUARD(2) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES |
     SHAPE_HURTS_OBJECTS,
     PUSH_ENEMIES | PUSH_FIRM(PUSH_OTHERS), {0.0F, 0.0F, 0.0F}, 5.5F, 1.75F,
     0.0F, {0}, 0, NULL},
};
/* clang-format on */

static struct tails_look_nodes tails_look_nodes = {
    &tails_head_joint, &tails_pupil_plus_z, &tails_pupil_minus_z};
static struct tails_look_nodes tails_fly_look_nodes = {
    &tails_fly_head_joint, &tails_fly_pupil_minus_z, &tails_fly_pupil_plus_z};

/* the indices of each kind of seam's welds: for each vertex a weld
 * shares, its index in the struct weld's object_a, then in its object_b */
static Uint16 tails_elbow_vertices[8] = {
#include "player/tails/tails_elbow_vertices.inc"
};
static Uint16 tails_wrist_vertices[8] = {
#include "player/tails/tails_wrist_vertices.inc"
};
static Uint16 tails_leg_vertices[8] = {
#include "player/tails/tails_leg_vertices.inc"
};
static Uint16 tails_toe_vertices[12] = {
#include "player/tails/tails_toe_vertices.inc"
};
static Uint16 tails_finger_vertices[8] = {
#include "player/tails/tails_finger_vertices.inc"
};

/* the welds of both his models' seams, then the points his first model
 * marks: his fingers, his toes and his head */
static struct weld tails_welds[] = {
    {&tails_object, &tails_upper_arm_minus_z, &tails_forearm_minus_z, 4,
     WELD_TO_A, 0, 0, NULL, tails_elbow_vertices},
    {&tails_object, &tails_forearm_minus_z, &tails_cuff_minus_z, 4, WELD_TO_A,
     0, 0, NULL, tails_wrist_vertices},
    {&tails_object, &tails_upper_arm_plus_z, &tails_forearm_plus_z, 4,
     WELD_TO_A, 0, 0, NULL, tails_elbow_vertices},
    {&tails_object, &tails_forearm_plus_z, &tails_cuff_plus_z, 4, WELD_TO_A, 0,
     0, NULL, tails_wrist_vertices},
    {&tails_object, &tails_thigh_minus_z, &tails_shin_minus_z, 4, WELD_TO_A, 0,
     0, NULL, tails_leg_vertices},
    {&tails_object, &tails_shin_minus_z, tails_foot_minus_z, 4, WELD_TO_A, 0, 0,
     NULL, tails_leg_vertices},
    {&tails_object, &tails_thigh_plus_z, &tails_shin_plus_z, 4, WELD_TO_A, 0, 0,
     NULL, tails_leg_vertices},
    {&tails_object, &tails_shin_plus_z, tails_foot_plus_z, 4, WELD_TO_A, 0, 0,
     NULL, tails_leg_vertices},
    {&tails_object, tails_shoe_minus_z, &tails_toes_minus_z, 6, WELD_TO_A, 0, 0,
     NULL, tails_toe_vertices},
    {&tails_object, tails_shoe_plus_z, &tails_toes_plus_z, 6, WELD_TO_A, 0, 0,
     NULL, tails_toe_vertices},
    {&tails_object, &tails_hand_minus_z, &tails_fingers_minus_z, 4, WELD_TO_A,
     0, 0, NULL, tails_finger_vertices},
    {&tails_object, &tails_hand_plus_z, &tails_fingers_plus_z, 4, WELD_TO_A, 0,
     0, NULL, tails_finger_vertices},
    {&tails_fly_object, &tails_fly_upper_arm_minus_z,
     &tails_fly_forearm_minus_z, 4, WELD_TO_A, 0, 0, NULL,
     tails_elbow_vertices},
    {&tails_fly_object, &tails_fly_forearm_minus_z, &tails_fly_cuff_minus_z, 4,
     WELD_TO_A, 0, 0, NULL, tails_wrist_vertices},
    {&tails_fly_object, &tails_fly_upper_arm_plus_z, &tails_fly_forearm_plus_z,
     4, WELD_TO_A, 0, 0, NULL, tails_elbow_vertices},
    {&tails_fly_object, &tails_fly_forearm_plus_z, &tails_fly_cuff_plus_z, 4,
     WELD_TO_A, 0, 0, NULL, tails_wrist_vertices},
    {&tails_fly_object, &tails_fly_thigh_minus_z, &tails_fly_shin_minus_z, 4,
     WELD_TO_A, 0, 0, NULL, tails_leg_vertices},
    {&tails_fly_object, &tails_fly_shin_minus_z, tails_fly_foot_minus_z, 4,
     WELD_TO_A, 0, 0, NULL, tails_leg_vertices},
    {&tails_fly_object, &tails_fly_thigh_plus_z, &tails_fly_shin_plus_z, 4,
     WELD_TO_A, 0, 0, NULL, tails_leg_vertices},
    {&tails_fly_object, &tails_fly_shin_plus_z, tails_fly_foot_plus_z, 4,
     WELD_TO_A, 0, 0, NULL, tails_leg_vertices},
    {&tails_fly_object, tails_fly_shoe_minus_z, &tails_fly_toes_minus_z, 6,
     WELD_TO_A, 0, 0, NULL, tails_toe_vertices},
    {&tails_fly_object, tails_fly_shoe_plus_z, &tails_fly_toes_plus_z, 6,
     WELD_TO_A, 0, 0, NULL, tails_toe_vertices},
    {&tails_fly_object, &tails_fly_hand_minus_z, &tails_fly_fingers_minus_z, 4,
     WELD_TO_A, 0, 0, NULL, tails_finger_vertices},
    {&tails_fly_object, &tails_fly_hand_plus_z, &tails_fly_fingers_plus_z, 4,
     WELD_TO_A, 0, 0, NULL, tails_finger_vertices},
    {&tails_object, &tails_fingers_minus_z, NULL, 2, WELD_MARK, 0, 0, NULL,
     NULL},
    {&tails_object, &tails_fingers_plus_z, NULL, 2, WELD_MARK + 1, 0, 0, NULL,
     NULL},
    {&tails_object, &tails_toes_minus_z, NULL, 0, WELD_MARK + 2, 0, 0, NULL,
     NULL},
    {&tails_object, &tails_toes_plus_z, NULL, 0, WELD_MARK + 3, 0, 0, NULL,
     NULL},
    {&tails_object, tails_head, NULL, 0, WELD_MARK + 4, 0, 0, NULL, NULL},
    {NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL},
};

/* the board's own action for each of his board actions, from ACTION_BOARD
 * on. null draws the board as its model has it */
static NJS_ACTION *tails_board_actions[] = {
    NULL,          &dat_0cb03844, &dat_0cb03844, &dat_0cb02cd0, &dat_0cb02b7c,
    &dat_0cb035a0, &dat_0cb02f14, &dat_0cb0335c, &dat_0cb03138, NULL,
    &dat_0cb02a38, NULL,          NULL,          NULL,          &dat_0cb039c8,
    &dat_0cb0456c, &dat_0cb03844, &dat_0cb03844, &dat_0cb03844, &dat_0cb03844,
    NULL,
};

static Sint32 tails_wave_phase; /* the phase his tails wave on */
static Float tails_wave_blend; /* how far the wave bends them */
/* scratch matrices for his attack's trails and his sway */
static NJS_MATRIX tails_matrix0;
static NJS_MATRIX tails_matrix1;
static NJS_MATRIX tails_matrix2;
static NJS_MATRIX tails_matrix3;

/* add a board trick's points to the score, but only when the trial menu
 * picked a mini game */
static void tails_board_score(int points)
{
    if (game_mode == GAME_MODE_TRIAL &&
        menu_state.trial[0] == MENU_TRIAL_MINI_GAME) {
        score_add(points);
    }
}

void tails_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;

    if ((work->flags & COLLISION_ON_TOP) && motion->speed.y <= 0.0F) {
        work->flags |= PLAYER_ON_TASK;
        work->ang.x = work->collision->owner->work->ang.x;
        work->ang.z = work->collision->owner->work->ang.z;
    } else {
        work->flags &= ~PLAYER_ON_TASK;
    }
    if (work->mode != TAILS_DOWN) {
        tails_choose_mode(work, motion, record);
    }
    if (work->collision != NULL) {
        /* his body's attack and guard levels, by what he's doing */
        /* @hack type: the levels as Uint32s, which SHC zero-extends
         * before it masks them (EXTU.B R0,R0 at 0x8c0bd3b0); as ints, it
         * masks with AND #252 and 151 of 1476 words differ */
        Uint32 attack;
        Uint32 guard;
        struct collision_shape *shape = work->collision->shapes;

        if (record->effects & PLAYER_INVINCIBLE) {
            attack = 3;
            guard = 3;
        } else if (work->flags & PLAYER_BALL) {
            attack = 1;
            guard = 1;
        } else if (work->mode == TAILS_MODULE_POSE) {
            attack = 0;
            guard = 1;
        } else {
            attack = 0;
            guard = 0;
        }
        shape->levels = (shape->levels & ~SHAPE_ATTACK) | ATTACK(attack);
        shape->levels = (shape->levels & ~SHAPE_GUARD) | GUARD(guard);
    }
    switch (work->mode) {
    case TAILS_INIT: {
        struct task *child;

        PLAYER_VARS(work)->character = CHARACTER_TAILS;
        player_init(PLAYER_VARS(work)->player, task);
        record = motion->record;
        record->anim.actions = tails_actions;
        record->anim.action = ACTION_STAND;
        collision_init(task, tails_shapes, COLLISION_PLAYER, 1, 4);
        work->collision->shapes->radius = record->consts.radius;
        work->mode = TAILS_STAND;
        tails_look_create(PLAYER_VARS(work)->player, &tails_look_nodes);
        tails_fly_look_create(PLAYER_VARS(work)->player, &tails_fly_look_nodes);
        texture_load_character(CHARACTER_TAILS);
        record->idle_steps = 0;
        record->anim.welds = tails_welds;
        child = task_create(TASK_WORK, 3, tails_mouth_start);
        if (child != NULL) {
            PLAYER_VARS(child->work)->player = PLAYER_VARS(work)->player;
        }
        child = task_create(TASK_MOTION | TASK_WORK, 3, tails_sway_step);
        if (child != NULL) {
            PLAYER_VARS(child->work)->player = PLAYER_VARS(work)->player;
        }
        if (PLAYER_VARS(work)->player == 0) {
            camera_set_restore_mode(4, 20);
        }
        task->destructor = tails_destroy;
        task->draw = tails_draw;
        break;
    }
    case TAILS_STAND:
        player_turn_to_stick(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        tails_set_stand_action(work, record);
        break;
    case TAILS_WALK:
        if (record->anim.previous == ACTION_TURN &&
            record->anim.current != ACTION_TURN) {
            record->speed.x += record->consts.trot_speed;
        }
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        if (player_collide(work, motion, record) == PLAYER_CRASHED) {
            tails_sound(work, SE_CRASH);
            work->mode = TAILS_CRASH;
            record->anim.action = ACTION_CRASH;
            player_crash_burst(work);
            record->speed.x = -(record->speed.x * 0.3F);
            record->speed.z = -(record->speed.z * 0.3F);
        } else {
            player_take_speed(work, motion, record);
            tails_set_walk_action(work, motion, record);
        }
        break;
    case TAILS_PATH:
        player_accel_path(work, motion, record);
        player_apply_path_accel(work, motion, record);
        player_move_on_path(work, motion, record);
        tails_set_walk_action(work, motion, record);
        break;
    case TAILS_ROLL:
        player_turn_to_stick(work, motion, record);
        player_accel_roll(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_PATH_SPIN:
        player_accel_roll(work, motion, record);
        player_apply_path_accel(work, motion, record);
        player_move_on_path(work, motion, record);
        break;
    case TAILS_JUMP:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_SPRING:
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_LAUNCH:
        if (record->speed.y >= record->consts.launch_speed) {
            player_accel_launched(work, motion, record);
        } else {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_FLUNG:
        player_accel_gravity(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_FALL:
    case TAILS_TIRED:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_STOP:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_PUSH:
        if (record->anim.action == ACTION_PUSH_START) {
            player_accel_brake(work, motion, record);
        } else {
            player_accel_push(work, motion, record);
            tails_push_sound(work);
            if (record->anim.previous == ACTION_PUSH_START) {
                tails_sound(work, SE_PUSH_START);
            }
        }
        player_apply_push(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_HURT:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_BACK:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_CRASH:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_SLIDE:
        player_turn_to_stick(work, motion, record);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_HANG:
    case TAILS_HANG_GO:
        player_slide_on_path(work, motion, record);
        tails_hang_move(work, motion, record);
        break;
    case TAILS_LEDGE:
    case TAILS_LEDGE_GO:
        player_shift_on_path(work, motion, record);
        tails_ledge_move(work, motion, record);
        break;
    case TAILS_PANEL_JUMP:
        if (panel_fly(work, motion, record) <= 31) {
            record->anim.action = ACTION_PANEL_JUMP;
        }
        break;
    case TAILS_PANEL_WAIT:
        break;
    case TAILS_CAUGHT:
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_TAIL_ATTACK:
        player_turn_to_stick(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_FLY:
        player_turn_upright(work, motion, record);
        tails_fly(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_POSE:
    case TAILS_HOVER:
        break;
    case TAILS_LIFT:
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_PUT_DOWN:
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_HOLD_STAND:
        player_turn_to_stick(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_GRIP:
        player_accel_push(work, motion, record);
        player_apply_push(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_THROW:
        player_turn_to_target(work, motion, record);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_HOLD_WALK:
        if (record->anim.previous == ACTION_HOLD_TURN &&
            record->anim.current != ACTION_HOLD_TURN) {
            record->speed.x += record->consts.trot_speed;
        }
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        tails_set_walk_action(work, motion, record);
        break;
    case TAILS_HOLD_STOP:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_HOLD_FALL:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_HOLD_JUMP:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_HOLD_SPRING:
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_HOLD_LAUNCH:
        if (record->speed.y >= record->consts.launch_speed) {
            player_accel_launched(work, motion, record);
        } else {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_PULL:
        player_accel_push(work, motion, record);
        player_apply_push(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        tails_push_sound(work);
        break;
    case TAILS_AIR_THROW:
        player_turn_upright(work, motion, record);
        player_turn_to_target(work, motion, record);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_SHAKE:
        player_shake_touched(work, motion, record);
        break;
    case TAILS_HOLD_SHAKE:
        record->held->motion->accel.x = record->held->motion->accel.y =
            record->held->motion->accel.z = record->anim.frame * 0.01F;
        break;
    case TAILS_MODULE_POSE:
        break;
    case TAILS_CIRCLE:
        tails_circle_place(work, record);
        break;
    case TAILS_CIRCLE_DOWN: {
        struct task_work *held;
        Float top;

        held = record->held->work;
        work->pos.y -= 0.5F;
        top = held->pos.y + held->collision->shapes->center.y -
              held->collision->shapes->size_y;
        if (top >= work->pos.y) {
            work->pos.y = top;
        }
        tails_circle_place(work, record);
        break;
    }
    case TAILS_CIRCLE_LEFT:
        work->ang.y += tails_circle_step(record);
        tails_circle_place(work, record);
        break;
    case TAILS_CIRCLE_RIGHT:
        work->ang.y -= tails_circle_step(record);
        tails_circle_place(work, record);
        break;
    case TAILS_BOARD:
        player_accel_board(work, motion, record, PLAYER_BOARD_TOP_SPEED);
        player_apply_accel(work, motion, record);
        if (player_collide(work, motion, record) == PLAYER_CRASHED) {
            tails_sound(work, SE_CRASH);
            work->mode = TAILS_BOARD_CRASH;
            if (vector_angle(&motion->speed, &record->wall_normal,
                             &record->contact->normal) > 0x8000) {
                record->anim.action = ACTION_BOARD_CRASH_B;
            } else {
                record->anim.action = ACTION_BOARD_CRASH_A;
            }
            player_crash_burst(work);
            if (stage_get_stage_act() == STAGE_ACT(STAGE_ICECAP, 2)) {
                tails_sound(work, SE_BOARD_CRASH);
            }
        } else {
            player_take_speed(work, motion, record);
            tails_set_board_action(work, motion, record);
        }
        break;
    case TAILS_BOARD_BRAKE:
        player_accel_board_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        player_board_spray(work, motion, record);
        break;
    case TAILS_BOARD_FALL:
        player_turn_upright(work, motion, record);
        player_accel_board_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_BOARD_JUMP:
        player_turn_upright(work, motion, record);
        player_accel_board_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_BOARD_TRICK:
        player_turn_upright(work, motion, record);
        player_accel_board_trick(work, motion, record);
        player_apply_accel(work, motion, record);
        if (player_collide(work, motion, record)) {
            record->anim.action = ACTION_BOARD_TRICK_FALL;
        }
        player_take_speed(work, motion, record);
        break;
    case TAILS_BOARD_CRASH:
        player_accel_board_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        player_board_spray(work, motion, record);
        break;
    case TAILS_SLOPE_ROLL:
        player_accel_slope(work, motion, record);
        player_collide(work, motion, record);
        if (work->flags & PLAYER_ON_LAND) {
            motion->speed.y = 0.0F;
        }
        player_take_speed(work, motion, record);
        break;
    case TAILS_SLOPE_ROLL_FALL:
        player_turn_upright(work, motion, record);
        player_accel_roll(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_PLAY:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case TAILS_EVENT:
        event_paths_step(work);
        break;
    case TAILS_RISE:
        /* once his action is done and water lies above his middle, he rises
         * toward its top, at most 0.4 a step */
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
    case TAILS_DOWN:
        if (work->flags & PLAYER_ON_GROUND) {
            record->anim.action = ACTION_DOWN;
        }
        player_turn_upright(work, motion, record);
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    /* @hack redundant: a case for a mode nothing enters, which stretches the
     * jump table to 64 entries (MOV #64,R1 at 0x8c0bd3d0); without it, 103
     * of 1476 words differ */
    case TAILS_EDIT:
        break;
    }
    player_anim_step(&record->anim);
    if (!(work->flags & PLAYER_BALL)) {
        if (record->upgrades & PLAYER_TAILS_JET_ANKLET) {
            tails_foot_plus_z->sibling = tails_jet_anklet_plus_z;
            tails_foot_minus_z->sibling = tails_jet_anklet_minus_z;
            tails_fly_foot_plus_z->sibling = tails_jet_anklet_plus_z;
            tails_fly_foot_minus_z->sibling = tails_jet_anklet_minus_z;
        } else {
            tails_foot_plus_z->sibling = tails_shoe_plus_z;
            tails_foot_minus_z->sibling = tails_shoe_minus_z;
            tails_fly_foot_plus_z->sibling = tails_fly_shoe_plus_z;
            tails_fly_foot_minus_z->sibling = tails_fly_shoe_minus_z;
        }
        if (record->upgrades & PLAYER_TAILS_RHYTHM_BROOCH) {
            tails_body->model = &tails_brooch_body_model;
            tails_fly_body->model = &tails_brooch_body_model;
        } else {
            tails_body->model = &tails_body_model;
            tails_fly_body->model = &tails_fly_body_model;
        }
        player_update_welds(work, motion, record);
    }
    tails_move_shape(work, record);
    tails_draw(task);
    if (record->held != NULL) {
        tails_carry(work, record);
    }
    shadow_update(work, &record->shadow);
    if (!(TAILS_FLAGS(work) & TAILS_UNTOUCHABLE)) {
        tails_set_shapes(work, record);
        collision_register(work);
        work->collision->radius =
            record->consts.height + njScalor(&record->speed);
    }
    player_finish_step(work, motion, record);
}

/* back to standing, in the action the ground gives him */
static void tails_stand(struct task_work *work, struct player *record)
{
    work->mode = TAILS_STAND;
    record->idle_steps = 0;
    tails_set_stand_action(work, record);
}

/* he starts to fly. as player 0, where stage_before_bosses holds, he hands the
 * camera to camera_step_follow and sets TAILS_FLIGHT_CAMERA */
#define START_FLIGHT(work)                                                     \
    do {                                                                       \
        TAILS_STATE(work) |= TAILS_FLYING;                                     \
        TAILS_FLAGS(work) &= ~TAILS_STOP_FLYING;                               \
        if (PLAYER_VARS(work)->player == 0 && stage_before_bosses()) {         \
            TAILS_FLAGS(work) |= TAILS_FLIGHT_CAMERA;                          \
            camera_override(camera_step_follow, 20, CAMERA_LOOK_AT);           \
        }                                                                      \
    } while (0)

/* what Tails does next, from the pads, what he touches and what's asked
 * of him */
/* NOLINTNEXTLINE(readability-function-size) */
static void tails_choose_mode(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    if (record != NULL && (record->effects & PLAYER_INVINCIBLE)) {
        work->flags |= PLAYER_RECOILING;
        work->timer = 0;
    } else if (work->timer == 0) {
        if (tails_try_hurt(work, record)) {
            work->flags |= PLAYER_RECOILING;
            if (work->flags & PLAYER_HOLDING) {
                tails_let_go(work, record);
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
    } else {
        work->timer--;
        work->flags &= ~PLAYER_HURT;
    }

    switch (work->mode) {
    case TAILS_STAND:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (tails_try_fall(work, record)) {
            break;
        }
        if (tails_try_jump(work, record)) {
            break;
        }
        if (tails_try_grab(work, motion, record)) {
            break;
        }
        if (tails_try_tail_attack(work, record)) {
            break;
        }
        /* @hack redundant: an else after a break, which loads the record into
         * R3 and its contact into R2 for the slope's test (0x8c0be6a6,
         * 0x8c0be6aa); as a plain if, they take R1 and R3 and 3 of 5553
         * words differ */
        if (player_read_stick(work, NULL, NULL)) {
            work->mode = TAILS_WALK;
            record->anim.action = ACTION_WALK;
            break;
        } else if (record->contact->normal.y < 0.77F &&
                   njScalor(&record->speed) > record->consts.slide_speed) {
            work->mode = TAILS_SLIDE;
            record->anim.action = ACTION_SLIDE;
        }
        break;
    case TAILS_WALK:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (tails_try_fall(work, record) || tails_try_jump(work, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (record->speed.x > 2.0F) {
            if (tails_try_roll(work, record)) {
                break;
            }
        } else if (tails_try_tail_attack(work, record)) {
            break;
        }
        if (tails_try_grab(work, motion, record)) {
            break;
        }
        if (tails_try_tail_attack(work, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (tails_try_stop(work, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if ((work->collision->flags & COLLISION_PUSHED) &&
            !(work->flags & PLAYER_HURT) &&
            !(work->collision->hit->shapes[work->collision->hit_shape].flags &
              SHAPE_NO_PUSH) &&
            (work->collision->flags & COLLISION_HIT_SIDE)) {
            /* @hack spelling: a switch of one case, whose arm loads work into
             * R2 and the mode into R3 for its stores (0x8c0be8d2, 0x8c0be8d4);
             * as an if, the two trade and 6 of 5553 words differ */
            switch (player_stick_into_wall(work, motion)) {
            case PLAYER_WALL_HEAD_ON:
                work->mode = TAILS_PUSH;
                record->anim.action = ACTION_PUSH_START;
                work->flags &= ~PLAYER_ON_PATH;
                break;
            }
        }
        if (player_stick_back_or_end_event(work, motion) &&
            record->speed.x >= record->consts.trot_speed) {
            work->mode = TAILS_STOP;
            record->anim.action = ACTION_SKID;
            tails_sound(work, SE_SKID);
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case TAILS_PATH:
        work->flags &= ~PLAYER_ATTACKING;
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (tails_try_fall(work, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (tails_try_jump(work, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (record->speed.x < record->consts.trot_speed) {
            work->mode = TAILS_WALK;
            record->anim.action = ACTION_WALK;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (tails_try_stop(work, record)) {
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case TAILS_ROLL:
        if (tails_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (tails_try_fall(work, record)) {
            record->anim.action = ACTION_JUMP;
            record->action_speed = 2.0F;
            work->ang.x = work->ang.z = 0;
            work->flags |= PLAYER_ATTACKING;
            break;
        }
        if (tails_try_jump(work, record)) {
            record->anim.action = ACTION_JUMP;
            record->action_speed = 2.0F;
            work->ang.x = work->ang.z = 0;
            work->flags |= PLAYER_ATTACKING;
            break;
        }
        if (tails_try_unroll(work, record)) {
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                        pad_action_buttons)) {
            work->mode = TAILS_WALK;
            record->anim.action = ACTION_UNCURL;
            work->flags &= ~PLAYER_ATTACKING;
            /* @hack redundant: a break where the case breaks anyway, which
             * leaves the JMP's delay slot a NOP (0x8c0beb9e); without it, the
             * flags' store fills the slot and 16 of 5553 words differ */
            break;
        }
        break;
    case TAILS_PATH_SPIN:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (tails_try_fall(work, record)) {
            record->anim.action = ACTION_JUMP;
            record->action_speed = 5.0F;
            work->ang.x = work->ang.z = 0;
            work->flags |= PLAYER_ATTACKING;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (record->speed.x < 1.85F) {
            record->anim.action = ACTION_JUMP;
            record->action_speed = 5.0F;
            work->ang.x = work->ang.z = 0;
            work->flags |= PLAYER_ATTACKING;
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case TAILS_JUMP:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            tails_sound(work, SE_LAND);
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = TAILS_STOP;
                record->anim.action = ACTION_SKID;
            } else if (tails_try_stop(work, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (motion->speed.y > -5.0F) {
                    record->anim.action = ACTION_LANDED;
                } else {
                    record->anim.action = ACTION_HARD_LANDING;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = TAILS_WALK;
                tails_set_walk_action(work, motion, record);
            }
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (player_jump_pressed(work, record)) {
            work->flags &= ~PLAYER_ATTACKING;
            work->mode = TAILS_FLY;
            record->anim.action = ACTION_FLY;
            record->own.tails.flight = 0.0F;
            work->flags &= ~PLAYER_ATTACKING;
            work->flags |= PLAYER_GUARD;
            START_FLIGHT(work);
            tails_sound(work, SE_FLY);
            break;
        }
        if (tails_try_uncurl(work, record)) {
            break;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.8F;
        }
        break;
    case TAILS_SPRING:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (motion->speed.y <= 0.0F) {
            work->mode = TAILS_FALL;
            if (record->anim.action == ACTION_BOUNCE_1) {
                record->anim.action = ACTION_BOUNCE_1_FALL;
            } else if (record->anim.action == ACTION_BOUNCE_2) {
                record->anim.action = ACTION_BOUNCE_2_FALL;
            } else {
                record->anim.action = ACTION_FALL_START;
            }
        }
        break;
    case TAILS_LAUNCH:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (motion->speed.y <= 0.0F) {
            work->mode = TAILS_FALL;
            record->anim.action = ACTION_FALL_START;
        }
        break;
    case TAILS_FLUNG:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            tails_sound(work, SE_LAND);
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = TAILS_STOP;
                record->anim.action = ACTION_SKID;
            } else if (tails_try_stop(work, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (motion->speed.y > -5.0F) {
                    record->anim.action = ACTION_LANDED;
                } else {
                    record->anim.action = ACTION_HARD_LANDING;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = TAILS_WALK;
                tails_set_walk_action(work, motion, record);
            }
        }
        break;
    case TAILS_FALL:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (TAILS_FLAGS(work) & TAILS_FLIGHT_CAMERA) {
                camera_restore();
            }
            TAILS_STATE(work) &= ~TAILS_FLYING;
            tails_sound(work, SE_LAND);
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = TAILS_STOP;
                record->anim.action = ACTION_SKID;
            } else if (tails_try_stop(work, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (motion->speed.y > -5.0F) {
                    record->anim.action = ACTION_LAND;
                } else {
                    record->anim.action = ACTION_HARD_LANDING;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = TAILS_WALK;
                tails_set_walk_action(work, motion, record);
            }
            break;
        }
        if (!player_jump_pressed(work, record)) {
            break;
        }
        work->flags &= ~PLAYER_ATTACKING;
        work->mode = TAILS_FLY;
        record->anim.action = ACTION_FLY;
        record->own.tails.flight = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_GUARD;
        START_FLIGHT(work);
        tails_sound(work, SE_FLY);
        break;
    case TAILS_TIRED:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (TAILS_FLAGS(work) & TAILS_FLIGHT_CAMERA) {
                camera_restore();
            }
            TAILS_STATE(work) &= ~TAILS_FLYING;
            tails_sound(work, SE_LAND);
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = TAILS_STOP;
                record->anim.action = ACTION_SKID;
            } else if (tails_try_stop(work, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (motion->speed.y > -5.0F) {
                    record->anim.action = ACTION_LAND;
                } else {
                    record->anim.action = ACTION_HARD_LANDING;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = TAILS_WALK;
                tails_set_walk_action(work, motion, record);
            }
        }
        break;
    case TAILS_STOP:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (tails_try_fall(work, record)) {
            break;
        }
        if (tails_try_jump(work, record)) {
            break;
        }
        if (record->speed.x <= 0.0F &&
            player_stick_back_or_end_event(work, motion)) {
            work->mode = TAILS_WALK;
            record->anim.action = ACTION_TURN;
            player_turn_to_stick(work, motion, record);
            work->ang.y = motion->aim.y;
            break;
        }
        if (record->anim.action != ACTION_SKID) {
            tails_stand(work, record);
        }
        break;
    case TAILS_PUSH:
        if (tails_try_request(work, motion, record)) {
            tails_push_sound_stop(work);
            break;
        }
        if (tails_try_jump(work, record)) {
            tails_push_sound_stop(work);
            break;
        }
        if (tails_try_fall(work, record)) {
            tails_push_sound_stop(work);
            break;
        }
        if (tails_try_stop(work, record)) {
            tails_push_sound_stop(work);
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
                    work->mode = TAILS_WALK;
                    record->anim.action = ACTION_WALK;
                    tails_push_sound_stop(work);
                }
                break;
            default:
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = TAILS_PULL;
                    record->anim.action = ACTION_PULL;
                } else {
                    work->mode = TAILS_WALK;
                    record->anim.action = ACTION_WALK;
                }
                tails_push_sound_stop(work);
                break;
            }
            break;
        }
        tails_let_go(work, record);
        work->mode = TAILS_WALK;
        record->anim.action = ACTION_WALK;
        tails_push_sound_stop(work);
        break;
    case TAILS_HURT:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_HURT &&
            record->anim.action != ACTION_HURT_LAND) {
            if (tails_try_stop(work, record)) {
                tails_stand(work, record);
                break;
            } else {
                /* @hack redundant: an else after a break, which loads
                 * work into R2 and TAILS_WALK into R3 for the mode's store
                 * (0x8c0bf6e4, 0x8c0bf6e6); without it, the two trade and 3
                 * of 5553 words differ */
                work->mode = TAILS_WALK;
                tails_set_walk_action(work, motion, record);
                break;
            }
        } else if (TASK_TIMER(work) <= 0) { /* TASK_TIMER is unsigned */
            if (work->flags & PLAYER_ON_GROUND) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = TAILS_WALK;
                tails_set_walk_action(work, motion, record);
                break;
            } else {
                /* @hack redundant: an else after a break, which loads
                 * work into R2 and TAILS_FALL into R3 for the mode's store
                 * (0x8c0bf726, 0x8c0bf728); without it, the two trade and 8
                 * of 5553 words differ */
                work->mode = TAILS_FALL;
                record->anim.action = ACTION_FALL;
                break;
            }
        }
        if (TASK_TIMER(work) <= 60 && player_read_stick(work, NULL, NULL)) {
            if (tails_try_jump(work, record)) {
                break;
            }
            if (!(work->flags & PLAYER_ON_GROUND)) {
                break;
            } else {
                /* @hack redundant: an else after a break, which loads
                 * motion into R3 and work into R2 for the angles' stores
                 * (0x8c0bf7a2, 0x8c0bf7a6); without it, the two trade and 4
                 * of 5553 words differ */
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = TAILS_WALK;
                tails_set_walk_action(work, motion, record);
                break;
            }
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
    case TAILS_BACK:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (tails_try_fall(work, record)) {
            break;
        }
        if (tails_try_jump(work, record)) {
            break;
        }
        if (tails_try_stop(work, record)) {
            break;
        }
        if (record->speed.x < 0.0F) {
            break;
        } else {
            /* @hack redundant: an else after a break, which loads work into R2
             * and TAILS_WALK into R3 for the mode's store (0x8c0bf8a6,
             * 0x8c0bf8a8); without it, the two trade and 3 of 5553 words
             * differ */
            work->mode = TAILS_WALK;
            tails_set_walk_action(work, motion, record);
            /* @hack redundant: a break where the case breaks anyway.
             * TAILS_HURT's three walking arms branch to this arm's call of
             * tails_set_walk_action (BRA 0x8c0bf8ac at 0x8c0bf6ea). without
             * the break, the stick's arm makes the call they share, this arm
             * makes its own, and 46 of 5553 words differ */
            break;
        }
        /* @hack redundant: a break after an if whose arms both break, which
         * loads work into R2 for TAILS_POSE's test (MOV.L @(R0,R15),R2 at
         * 0x8c0bf8c2); without it, R3, and 3 of 5553 words differ */
        break;
    case TAILS_POSE:
        if (work->flags & PLAYER_HOLDING) {
            if (tails_try_request_holding(work, motion, record)) {
                break;
            }
        } else if (tails_try_request(work, motion, record)) {
            break;
        }
        break;
    case TAILS_CRASH:
        if (record->anim.action == ACTION_CRASH) {
            break;
        }
        if (tails_try_fall(work, record)) {
            break;
        }
        if (record->anim.action != ACTION_CRASH_LAND) {
            tails_stand(work, record);
        }
        break;
    case TAILS_SLIDE:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (tails_try_fall(work, record)) {
            break;
        }
        if (tails_try_jump(work, record)) {
            break;
        }
        if (tails_try_tail_attack(work, record)) {
            break;
        }
        if (player_read_stick(work, NULL, NULL)) {
            work->mode = TAILS_WALK;
            record->anim.action = ACTION_WALK;
            break;
        }
        if (njScalor(&motion->speed) < record->consts.slide_speed ||
            record->contact->normal.y > 0.98F) {
            work->mode = TAILS_STAND;
            record->idle_steps = 0;
            record->anim.action = ACTION_LANDED;
        }
        break;
    case TAILS_HANG: {
        Angle angle;

        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (tails_try_jump(work, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (work->flags & PLAYER_ON_PATH) {
            if (player_read_stick(work, &angle, NULL)) {
                if (angle_distance(work->ang.y, angle) < 0x4000) {
                    record->speed.x = 0.375F;
                    if (record->anim.action == ACTION_HANG_SWAPPED) {
                        record->anim.start_frame = 0.0F;
                    } else {
                        record->anim.start_frame = 32.0F;
                    }
                } else {
                    record->speed.x = -0.375F;
                    if (record->anim.action == ACTION_HANG_SWAPPED) {
                        record->anim.start_frame = 0.0F;
                    } else {
                        record->anim.start_frame = 32.0F;
                    }
                }
                work->mode = TAILS_HANG_GO;
                record->anim.action = ACTION_HANG_GO;
                break;
            }
        } else {
            work->mode = TAILS_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                        pad_action_buttons)) {
            work->mode = TAILS_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a break where the case breaks anyway, which
             * leaves the JMP's delay slot a NOP (0x8c0bfc04); without it, the
             * flags' store fills the slot and 16 of 5553 words differ */
            break;
        }
        break;
    }
    case TAILS_HANG_GO:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (tails_try_jump(work, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (work->flags & PLAYER_ON_PATH) {
            /* @hack spelling: an empty arm, which loads work into R2 and the
             * else's TAILS_FALL into R1 (0x8c0bfc84, 0x8c0bfc86); with the
             * test inverted, the two trade and 6 of 5553 words differ */
        } else {
            work->mode = TAILS_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                        pad_action_buttons)) {
            work->mode = TAILS_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a break where the case breaks anyway, which
             * leaves the JMP's delay slot a NOP (0x8c0bfce6); without it, the
             * flags' store fills the slot and 16 of 5553 words differ */
            break;
        }
        break;
    case TAILS_LEDGE: {
        Angle angle;

        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (tails_try_jump(work, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (work->flags & PLAYER_ON_PATH) {
            if (!player_read_stick(work, &angle, NULL)) {
                break;
            }
            if (angle_distance(0xc000 - camera_work->ang.y, angle) < 0x4000) {
                record->speed.x = 0.375F;
                /* @hack redundant: a break where the case breaks anyway, which
                 * gives this arm a store and jump of its own (0x8c0bfd94 to
                 * 0x8c0bfd9e); without it, the else's arm branches into
                 * them and 11 of 5553 words differ */
                break;
            } else {
                /* @hack redundant: an else after a break, which loads the
                 * record and then the jump's address into R2 (0x8c0bfda2,
                 * 0x8c0bfdaa); without it, into R3, and 10 of 5553 words
                 * differ */
                record->speed.x = -0.375F;
                /* @hack redundant: a break where the case breaks anyway, which
                 * leaves the JMP's delay slot a NOP (0x8c0bfdae); without
                 * it, the store fills the slot and 5 of 5553 words differ */
                break;
            }
            /* @hack redundant: a break after an if whose arms both break,
             * which loads work into R1 and TAILS_FALL into R2 for the
             * fall's stores (0x8c0bfe02, 0x8c0bfe04); without it, the two
             * trade and 6 of 5553 words differ */
            break;
        } else {
            /* @hack redundant: an else after a break, which loads work into R1
             * and TAILS_FALL into R2 (0x8c0bfe02, 0x8c0bfe04); without it,
             * into R2 and R3, and 8 of 5553 words differ */
            work->mode = TAILS_FALL;
            record->anim.action = ACTION_FALL;
            player_clear_speed(motion, record);
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
    }
    case TAILS_LEDGE_GO:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (tails_try_jump(work, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (work->flags & PLAYER_ON_PATH) {
            /* @hack spelling: an empty arm, which loads work into R2 and the
             * else's TAILS_FALL into R1 (0x8c0bfe74, 0x8c0bfe76); with the
             * test inverted, the two trade and 6 of 5553 words differ */
        } else {
            work->mode = TAILS_FALL;
            record->anim.action = ACTION_FALL;
            player_clear_speed(motion, record);
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        break;
    case TAILS_PANEL_JUMP:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (panel_touched(work)) {
            tails_sound(work, SE_LAND);
            player_clear_speed(motion, record);
            work->mode = TAILS_PANEL_WAIT;
            record->anim.action = ACTION_PANEL_WAIT;
            break;
        }
        if (!(work->flags & PLAYER_ON_GROUND)) {
            break;
        }
        tails_sound(work, SE_LAND);
        player_clear_speed(motion, record);
        work->mode = TAILS_STAND;
        record->idle_steps = 0;
        record->anim.action = ACTION_LANDED;
        break;
    case TAILS_PANEL_WAIT:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (panel_jump_ready_or_idle(work) <= 0) {
            work->mode = TAILS_STAND;
            record->idle_steps = 0;
            record->anim.action = ACTION_LANDED;
            break;
        }
        if (player_jump_pressed(work, record) != PLAYER_JUMP_PANEL) {
            break;
        }
        panel_jump(work);
        work->mode = TAILS_PANEL_JUMP;
        record->anim.action = ACTION_PANEL_JUMP;
        break;
    case TAILS_CAUGHT:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (pad_press_live[PLAYER_VARS(work)->player] & PAD_MASH_BUTTONS) {
            TAILS_VARS(work)->struggle -= 20;
        }
        if (TAILS_VARS(work)->struggle > 0) {
            break;
        }
        tails_stand(work, record);
        break;
    case TAILS_PLAY:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (!(record->anim.flags & PLAYER_ACTION_DONE)) {
            break;
        }
        tails_stand(work, record);
        break;
    case TAILS_FLY:
        if (tails_try_request(work, motion, record)) {
            if (TAILS_FLAGS(work) & TAILS_FLIGHT_CAMERA) {
                camera_restore();
            }
            TAILS_STATE(work) &= ~TAILS_FLYING;
            break;
        }
        if (TAILS_FLAGS(work) & TAILS_STOP_FLYING) {
            work->mode = TAILS_FALL;
            player_clear_speed(NULL, record);
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_GUARD;
            TAILS_STATE(work) &= ~TAILS_FLYING;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (TAILS_FLAGS(work) & TAILS_FLIGHT_CAMERA) {
                camera_restore();
            }
            TAILS_STATE(work) &= ~TAILS_FLYING;
            tails_sound(work, SE_LAND);
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = TAILS_STOP;
                record->anim.action = ACTION_SKID;
                record->own.tails.spin = 0.8F;
                work->flags &= ~PLAYER_GUARD;
            } else if (tails_try_stop(work, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                record->anim.action = ACTION_LAND;
                record->own.tails.spin = 0.8F;
                work->flags &= ~PLAYER_GUARD;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = TAILS_WALK;
                tails_set_walk_action(work, motion, record);
                record->own.tails.spin = 0.8F;
                work->flags &= ~PLAYER_GUARD;
            }
        }
        switch (record->anim.action) {
        case ACTION_FLY:
            if (_builtin_fabsf(record->speed.y) > 2.0F) {
                record->anim.action = ACTION_FLY_FAST;
            }
            break;
        case ACTION_FLY_FAST:
            if (_builtin_fabsf(record->speed.y) < 1.5F) {
                record->anim.action = ACTION_FLY;
            }
            break;
        }
        break;
    case TAILS_TAIL_ATTACK:
        if (tails_try_request(work, motion, record) ||
            record->anim.action == ACTION_STAND) {
            work->mode = TAILS_STAND;
            record->anim.action = ACTION_STAND;
            TAILS_STATE(work) &= ~TAILS_ATTACKING;
            break;
        }
        tails_attack_next(work, record);
        tails_attack_trails(work, motion, record);
        break;
    case TAILS_LIFT:
        if (work->flags & PLAYER_HOLDING) {
            if (tails_try_request_holding(work, motion, record)) {
                break;
            }
        } else if (tails_try_request(work, motion, record)) {
            break;
        }
        if (tails_try_fall(work, record)) {
            break;
        }
        if (tails_try_take_lifted(work, motion, record)) {
            break;
        }
        break;
    case TAILS_GRIP:
        if (tails_try_grip(work, motion, record)) {
            break;
        }
        if (tails_try_jump(work, record)) {
            break;
        }
        break;
    case TAILS_HOLD_STAND:
        if (tails_try_request_holding(work, motion, record)) {
            break;
        }
        if (tails_try_fall(work, record)) {
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                        pad_grab_buttons) &&
            player_read_stick(work, NULL, NULL) &&
            record->held->work->id == CARRY_KIND_SMALL) {
            work->mode = TAILS_HOLD_SHAKE;
            record->anim.action = ACTION_HOLD_SHAKE_START;
            break;
        }
        if (tails_try_throw(work, motion, record)) {
            break;
        }
        if (tails_try_jump(work, record)) {
            break;
        }
        if (player_read_stick(work, NULL, NULL)) {
            work->mode = TAILS_HOLD_WALK;
            record->anim.action = ACTION_HOLD_WALK;
        }
        break;
    case TAILS_HOLD_WALK:
        if (tails_try_request_holding(work, motion, record)) {
            break;
        }
        if (tails_try_fall(work, record) || tails_try_jump(work, record) ||
            tails_try_throw(work, motion, record) ||
            tails_try_stop(work, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (tails_try_throw(work, motion, record)) {
            break;
        }
        if (player_stick_back_or_end_event(work, motion) &&
            record->speed.x >= record->consts.trot_speed) {
            work->mode = TAILS_HOLD_STOP;
            record->anim.action = ACTION_HOLD_SKID;
            tails_sound(work, SE_SKID);
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a break where the case breaks anyway, which
             * loads work into R3 for TAILS_PUT_DOWN's test (MOV.L @(R0,R15),R3
             * at 0x8c0c0642); without it, R1, and 3 of 5553 words differ */
            break;
        }
        break;
    case TAILS_PUT_DOWN:
    case TAILS_THROW:
        if (work->flags & PLAYER_HOLDING) {
            if (tails_try_request_holding(work, motion, record)) {
                break;
            }
        } else if (tails_try_request(work, motion, record)) {
            break;
        }
        if (tails_try_release(work, record)) {
            break;
        }
        if (record->anim.action != ACTION_THROW_AT &&
            record->anim.action != ACTION_THROW &&
            record->anim.action != ACTION_PUT_DOWN) {
            tails_stand(work, record);
        }
        break;
    case TAILS_HOLD_STOP:
        if (tails_try_request_holding(work, motion, record)) {
            break;
        }
        if (tails_try_fall(work, record)) {
            break;
        }
        if (tails_try_jump(work, record)) {
            break;
        }
        if (record->speed.x <= 0.0F &&
            player_stick_back_or_end_event(work, motion)) {
            work->mode = TAILS_HOLD_WALK;
            record->anim.action = ACTION_HOLD_TURN;
            player_turn_to_stick(work, motion, record);
            work->ang.y = motion->aim.y;
            break;
        }
        if (record->anim.action != ACTION_HOLD_SKID) {
            work->mode = TAILS_HOLD_STAND;
            record->anim.action = ACTION_HOLD_STAND;
        }
        break;
    case TAILS_HOLD_FALL:
        if (tails_try_request_holding(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            tails_sound(work, SE_LAND);
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = TAILS_HOLD_STOP;
                record->anim.action = ACTION_HOLD_SKID;
            } else if (tails_try_stop(work, record)) {
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
                work->mode = TAILS_HOLD_WALK;
                tails_set_walk_action(work, motion, record);
            }
            break;
        }
        if (tails_try_throw(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case TAILS_HOLD_JUMP:
        if (tails_try_request_holding(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            tails_sound(work, SE_LAND);
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = TAILS_HOLD_STOP;
                record->anim.action = ACTION_HOLD_SKID;
            } else if (tails_try_stop(work, record)) {
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
                work->mode = TAILS_HOLD_WALK;
                tails_set_walk_action(work, motion, record);
            }
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.4F;
        }
        if (tails_try_throw(work, motion, record)) {
            break;
        }
        if (motion->speed.y <= 0.0F) {
            work->mode = TAILS_HOLD_FALL;
            record->anim.action = ACTION_HOLD_FALL_START;
        }
        break;
    case TAILS_HOLD_SPRING:
        if (motion->speed.y <= 0.0F) {
            work->mode = TAILS_HOLD_FALL;
            record->anim.action = ACTION_HOLD_FALL_START;
        }
        break;
    case TAILS_HOLD_LAUNCH:
        if (motion->speed.y <= 0.0F) {
            work->mode = TAILS_HOLD_FALL;
            record->anim.action = ACTION_HOLD_FALL_START;
        }
        break;
    case TAILS_PULL:
        if (tails_try_request(work, motion, record)) {
            tails_push_sound_stop(work);
            break;
        }
        if (tails_try_jump(work, record)) {
            tails_push_sound_stop(work);
            break;
        }
        if (tails_try_fall(work, record)) {
            tails_push_sound_stop(work);
            break;
        }
        if (!PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                         pad_grab_buttons)) {
            tails_stand(work, record);
            tails_let_go(work, record);
            tails_push_sound_stop(work);
        }
        if ((work->collision->flags & COLLISION_PUSHED) &&
            !(work->flags & PLAYER_HURT) &&
            !(work->collision->hit->shapes[work->collision->hit_shape].flags &
              SHAPE_NO_PUSH) &&
            (work->collision->flags & COLLISION_HIT_SIDE)) {
            switch (player_stick_into_wall(work, motion)) {
            case PLAYER_WALL_HEAD_ON:
                work->mode = TAILS_PUSH;
                record->anim.action = ACTION_PUSH_START;
                tails_push_sound_stop(work);
                break;
            case PLAYER_WALL_NO_STICK:
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = TAILS_GRIP;
                    record->anim.action = ACTION_GRIP;
                    tails_push_sound_stop(work);
                }
                break;
            default:
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = TAILS_PULL;
                    record->anim.action = ACTION_PULL;
                } else {
                    work->mode = TAILS_WALK;
                    record->anim.action = ACTION_WALK;
                }
                tails_push_sound_stop(work);
                break;
            }
            break;
        } else {
            tails_let_go(work, record);
            work->mode = TAILS_WALK;
            record->anim.action = ACTION_WALK;
            tails_push_sound_stop(work);
            /* @hack redundant: a break where the case breaks anyway.
             * TAILS_PUSH's let-go and the first tests of TAILS_PUSH and
             * TAILS_PULL branch to this arm's call of tails_push_sound_stop.
             * without the break, TAILS_PULL's first test makes the call they
             * share, and 1807 of 5553 words differ */
            break;
        }
        /* @hack redundant: a break after an if whose arms both break, which
         * loads work into R2 for TAILS_AIR_THROW's test (MOV.L @(R0,R15),R2
         * at 0x8c0c0d24); without it, R3, and 3 of 5553 words differ */
        break;
    case TAILS_AIR_THROW:
        if (work->flags & PLAYER_HOLDING) {
            if (tails_try_request_holding(work, motion, record)) {
                break;
            }
        } else if (tails_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            tails_sound(work, SE_LAND);
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = TAILS_HOLD_STOP;
                record->anim.action = ACTION_HOLD_SKID;
            } else if (tails_try_stop(work, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (work->flags & PLAYER_HOLDING) {
                    if (motion->speed.y > -5.0F) {
                        record->anim.action = ACTION_HOLD_STAND;
                    } else {
                        record->anim.action = ACTION_HOLD_HARD_LANDING;
                    }
                } else if (motion->speed.y > -5.0F) {
                    tails_set_stand_action(work, record);
                } else {
                    record->anim.action = ACTION_HARD_LANDING;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = TAILS_HOLD_WALK;
                } else {
                    work->mode = TAILS_WALK;
                }
                tails_set_walk_action(work, motion, record);
            }
            break;
        }
        if (tails_try_release(work, record)) {
            break;
        }
        if (record->anim.action != ACTION_AIR_THROW_AT &&
            record->anim.action != ACTION_AIR_THROW) {
            work->mode = TAILS_FALL;
            record->anim.action = ACTION_FALL;
        }
        break;
    case TAILS_SHAKE:
        if (tails_try_request_holding(work, motion, record)) {
            break;
        }
        if (tails_try_fall(work, record)) {
            break;
        }
        if (tails_try_jump(work, record)) {
            break;
        }
        if (player_touches_shakable(work, motion, record)) {
            if (!player_read_stick(work, NULL, NULL)) {
                work->mode = TAILS_GRIP;
                record->anim.action = ACTION_GRIP;
            }
            break;
        }
        tails_stand(work, record);
        tails_let_go(work, record);
        break;
    case TAILS_HOLD_SHAKE:
        if (tails_try_request_holding(work, motion, record)) {
            break;
        }
        if (tails_try_fall(work, record)) {
            break;
        }
        if (tails_try_jump(work, record)) {
            break;
        }
        if (record->held == NULL ||
            record->held->work->id != CARRY_KIND_SMALL) {
            tails_stand(work, record);
            tails_let_go(work, record);
            break;
        }
        if (!player_read_stick(work, NULL, NULL)) {
            work->mode = TAILS_HOLD_STAND;
            record->anim.action = ACTION_HOLD_SHAKE_END;
            /* @hack redundant: a break where the case breaks anyway, which
             * loads work into R3 for TAILS_MODULE_POSE's test (MOV.L
             * @(R0,R15),R3 at 0x8c0c10ae); without it, R1, and 3 of 5553 words
             * differ */
            break;
        }
        break;
    case TAILS_MODULE_POSE:
        if (!(work->flags & PLAYER_REQUEST)) {
            break;
        }
        if (work->submode != PLAYER_MODE_RIDE_OFF) {
            break;
        }
        work->mode = TAILS_SPRING;
        record->anim.action = ACTION_SPRING;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case TAILS_CIRCLE:
        if (tails_try_request(work, motion, record) ||
            tails_try_jump(work, record)) {
            record->held = NULL;
            break;
        }
        if (pad_records[PLAYER_VARS(work)->player].x1 << 8 < -PAD_DEAD_ZONE) {
            work->mode = TAILS_CIRCLE_LEFT;
            record->anim.action = ACTION_CIRCLE_LEFT;
            break;
        }
        if (pad_records[PLAYER_VARS(work)->player].x1 << 8 > PAD_DEAD_ZONE) {
            work->mode = TAILS_CIRCLE_RIGHT;
            record->anim.action = ACTION_CIRCLE_RIGHT;
            break;
        }
        if (work->mode < TAILS_CIRCLE || work->mode > TAILS_CIRCLE_RIGHT) {
            record->held = NULL;
        }
        break;
    case TAILS_CIRCLE_DOWN:
        if (tails_try_request(work, motion, record) ||
            tails_try_jump(work, record)) {
            record->held = NULL;
            break;
        }
        if (record->held != NULL) {
            struct task_work *held = record->held->work;

            if (held->pos.y + held->collision->shapes->center.y -
                    held->collision->shapes->size_y >=
                work->pos.y) {
                work->mode = TAILS_CIRCLE;
                record->anim.action = ACTION_CIRCLE_LAND;
                break;
            }
        } else {
            tails_stand(work, record);
            break;
        }
        if (work->mode < TAILS_CIRCLE || work->mode > TAILS_CIRCLE_RIGHT) {
            record->held = NULL;
        }
        break;
    case TAILS_CIRCLE_LEFT:
        if (tails_try_request(work, motion, record) ||
            tails_try_jump(work, record)) {
            record->held = NULL;
            break;
        }
        if (pad_records[PLAYER_VARS(work)->player].x1 << 8 > -PAD_DEAD_ZONE) {
            work->mode = TAILS_CIRCLE;
            record->anim.action = ACTION_CIRCLE;
            break;
        }
        if (work->mode < TAILS_CIRCLE || work->mode > TAILS_CIRCLE_RIGHT) {
            record->held = NULL;
        }
        break;
    case TAILS_CIRCLE_RIGHT:
        if (tails_try_request(work, motion, record) ||
            tails_try_jump(work, record)) {
            record->held = NULL;
            break;
        }
        if (pad_records[PLAYER_VARS(work)->player].x1 << 8 < PAD_DEAD_ZONE) {
            work->mode = TAILS_CIRCLE;
            record->anim.action = ACTION_CIRCLE;
            break;
        }
        if (work->mode < TAILS_CIRCLE || work->mode > TAILS_CIRCLE_RIGHT) {
            record->held = NULL;
        }
        break;
    case TAILS_BOARD:
        if (tails_try_request_board(work, motion, record)) {
            break;
        }
        if (tails_try_fall(work, record)) {
            work->mode = TAILS_BOARD_FALL;
            record->anim.action = ACTION_BOARD_FALL;
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                        pad_jump_buttons)) {
            struct task_force *ground;
            NJS_VECTOR vector;
            NJS_VECTOR front;
            struct task_work *ride;
            Angle ang_x;
            Angle ang_y;
            Angle ang_z;
            NJS_VECTOR up;

            work->mode = TAILS_BOARD_JUMP;
            record->anim.action = ACTION_BOARD_JUMP;
            record->speed.y = record->consts.jump_speed;
            if (record->speed.x < record->consts.sprint_speed) {
                record->speed.x += record->consts.trot_speed;
            }
            if (record->ground_task != NULL) {
                ground = record->ground_task->force;
                if (ground != NULL) {
                    player_set_accel(PLAYER_VARS(work)->player, ground->speed.x,
                                     ground->speed.y, ground->speed.z);
                }
                if (record->ground_task->work->id == RAMP_ID) {
                    work->mode = TAILS_BOARD_TRICK;
                    tails_sound(work, SE_BOARD_RAMP);
                    ride = record->ground_task->work;
                    vector.x = 0.0F;
                    vector.y = 1.4F;
                    vector.z = 3.0F;
                    njPushMatrix(&_unit_matrix_);
                    TURN(njRotateZ, ride->ang.z);
                    TURN(njRotateY, ride->ang.y);
                    TURN(njRotateX, ride->ang.x);
                    njRotateY(NULL, 0x8000);
                    njCalcVector(NULL, &vector, &front);
                    vector.x = vector.z = 0.0F;
                    vector.y = 1.0F;
                    njCalcVector(NULL, &vector, &up);
                    ang_y = RADIANS_TO_ANGLE(atan2f(front.x, front.z));
                    ang_x = RADIANS_TO_ANGLE(asinf(up.z));
                    ang_z = -RADIANS_TO_ANGLE(asinf(up.x));
                    player_set_angles(PLAYER_VARS(work)->player, ang_x, ang_y,
                                      ang_z);
                    njPopMatrix(1);
                    player_set_accel(PLAYER_VARS(work)->player, front.x,
                                     front.y, front.z);
                    /* the faster he leaves the ramp, the better the trick
                     * and the more it scores */
                    if (record->speed.x > PLAYER_BOARD_SPEED * 0.9F) {
                        if (rand() * RAND_SCALE > 0.66F) {
                            record->anim.action = ACTION_BOARD_TRICK_1;
                            tails_sound(work, SE_TRICK_1);
                        } else {
                            record->anim.action = ACTION_BOARD_TRICK_2;
                            tails_sound(work, SE_TRICK_2);
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
                        tails_board_score(100);
                    } else if (record->speed.x > PLAYER_BOARD_SPEED * 0.6F) {
                        record->anim.action = ACTION_BOARD_TRICK_3;
                        tails_sound(work, SE_TRICK_3);
                        tails_board_score(50);
                    } else if (record->speed.x > PLAYER_BOARD_SPEED * 0.4F) {
                        record->anim.action = ACTION_BOARD_TRICK_4;
                        tails_sound(work, SE_TRICK_4);
                        tails_board_score(30);
                    } else {
                        work->mode = TAILS_BOARD_JUMP;
                        record->anim.action = ACTION_BOARD_JUMP;
                        tails_board_score(10);
                    }
                }
            }
            work->flags &= ~PLAYER_ON_TASK;
            record->jump_steps = 0;
            player_sound_in_mode(SE_BOARD_JUMP, PLAYER_VARS(work)->player,
                                 work->mode);
            break;
        }
        /* @hack redundant: his speed tested against trot_speed twice, as the
         * FCMP/GT at 0x8c0c18fe and 0x8c0c1912 test it; tested once, the
         * function runs 32 bytes short and 611 of 5553 words differ */
        if (player_stick_back_or_end_event(work, motion) &&
            /* NOLINTNEXTLINE(misc-redundant-expression) */
            record->speed.x >= record->consts.trot_speed &&
            record->speed.x >= record->consts.trot_speed) {
            Angle angle;

            work->mode = TAILS_BOARD_BRAKE;
            player_read_stick(work, &angle, NULL);
            angle = angle_difference(work->ang.y, angle);
            if (angle > 0) {
                record->anim.action = ACTION_BOARD_BRAKE_A;
            } else {
                record->anim.action = ACTION_BOARD_BRAKE_B;
            }
        }
        break;
    case TAILS_BOARD_BRAKE:
        if (tails_try_request_board(work, motion, record)) {
            break;
        }
        if (tails_try_fall(work, record)) {
            work->mode = TAILS_BOARD_FALL;
            record->anim.action = ACTION_BOARD_FALL;
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].release,
                        pad_jump_buttons)) {
            struct task_force *ground;

            work->mode = TAILS_BOARD_JUMP;
            record->speed.y = record->consts.jump_speed;
            if (record->speed.x < record->consts.sprint_speed) {
                record->speed.x += record->consts.trot_speed;
            }
            if (record->ground_task != NULL) {
                ground = record->ground_task->force;
                if (ground != NULL) {
                    player_set_accel(PLAYER_VARS(work)->player, ground->speed.x,
                                     ground->speed.y, ground->speed.z);
                }
            }
            record->anim.action = ACTION_BOARD_JUMP;
            work->flags &= ~PLAYER_ON_TASK;
            record->jump_steps = 0;
            tails_sound(work, SE_JUMP);
            break;
        }
        {
            Angle angle;

            if (!player_stick_back_or_end_event(work, motion) ||
                record->speed.x < record->consts.trot_speed) {
                work->mode = TAILS_BOARD;
                record->anim.action = ACTION_BOARD;
                tails_board_sound(work, record);
                break;
            }
            player_read_stick(work, &angle, NULL);
            angle = angle_difference(work->ang.y, angle);
            if (angle > 0) {
                record->anim.action = ACTION_BOARD_BRAKE_A;
            } else {
                record->anim.action = ACTION_BOARD_BRAKE_B;
            }
        }
        break;
    case TAILS_BOARD_FALL:
        if (tails_try_request_board(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            tails_sound(work, SE_LAND);
            work->mode = TAILS_BOARD;
            if (record->speed.y <= -3.0F) {
                record->anim.action = ACTION_BOARD_HARD_LANDING;
            } else {
                record->anim.action = ACTION_BOARD;
                if (record->speed.x <= 0.3F) {
                    record->speed.x = 1.0F;
                }
            }
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            tails_board_sound(work, record);
        }
        break;
    case TAILS_BOARD_JUMP:
        if (tails_try_request_board(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            work->mode = TAILS_BOARD;
            if (record->speed.y <= -3.0F) {
                record->anim.action = ACTION_BOARD_HARD_LANDING;
            } else {
                record->anim.action = ACTION_BOARD;
                if (record->speed.x <= 0.3F) {
                    record->speed.x = 1.0F;
                }
            }
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            tails_board_sound(work, record);
            /* @hack redundant: a break where the case breaks anyway, whose jump
             * takes this landing to TAILS_BOARD_TRICK's call of
             * tails_board_sound; without it, this arm makes its own call and
             * 315 of 5553 words differ */
            break;
        }
        break;
    case TAILS_BOARD_TRICK:
        if (tails_try_request_board(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            work->mode = TAILS_BOARD;
            if (record->speed.y <= -3.0F) {
                record->anim.action = ACTION_BOARD_HARD_LANDING;
            } else {
                record->anim.action = ACTION_BOARD;
            }
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            tails_board_sound(work, record);
            /* @hack redundant: a break where the case breaks anyway, which
             * keeps this arm's call of tails_board_sound, the one
             * TAILS_BOARD_JUMP's landing shares. without it, each makes its own
             * call and 314 of 5553 words differ */
            break;
        }
        break;
    case TAILS_BOARD_CRASH:
        if (tails_try_request_board(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_BOARD_CRASH_A &&
            record->anim.action != ACTION_BOARD_CRASH_B) {
            work->mode = TAILS_BOARD;
            record->anim.action = ACTION_BOARD;
            tails_board_sound(work, record);
        }
        break;
    case TAILS_SLOPE_ROLL:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (tails_try_fall(work, record)) {
            work->mode = TAILS_SLOPE_ROLL_FALL;
        }
        break;
    case TAILS_SLOPE_ROLL_FALL:
        if (tails_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            tails_sound(work, SE_LAND);
            work->mode = TAILS_SLOPE_ROLL;
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            /* @hack redundant: a break where the case breaks anyway, which
             * loads work into R3 for TAILS_HOVER's test (MOV.L @(R0,R15),R3 at
             * 0x8c0c1db8); without it, R2, and 3 of 5553 words differ */
            break;
        }
        break;
    case TAILS_HOVER:
        if (work->flags & PLAYER_HOLDING) {
            if (tails_try_request_holding(work, motion, record)) {
                break;
            }
        } else if (tails_try_request(work, motion, record)) {
            break;
        }
        break;
    case TAILS_EVENT:
        if (work->event->paths == NULL) {
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            tails_stand(work, record);
        }
        break;
    case TAILS_RISE:
    case TAILS_DOWN:
    case TAILS_CLEAR:
        break;
    }
}

static void tails_destroy(struct task *task)
{
    struct task_work *work = task->work;

    tails_reset_tails();
    player_free(PLAYER_VARS(work)->player);
    sound_stop_id((int)work);
    texture_release_character(CHARACTER_TAILS);
}

static void tails_move_shape(struct task_work *work, struct player *record)
{
    struct collision_shape *shape = work->collision->shapes;
    NJS_VECTOR middle;

    middle.x = middle.z = 0.0F;
    middle.y = record->consts.center_y;
    player_turn_to_world(work, &middle);
    shape->center = middle;
    shape->center.x += work->pos.x;
    shape->center.y += work->pos.y;
    shape->center.z += work->pos.z;
}

/* draw him about his middle, which his body's shape takes as its center.
 * hurt, he blinks. spinning on the ground, he leaves two afterimages. as
 * player 0, or with TAILS_TAIL_SPIN set, his tails leave two afterimages
 * of their own. his tails spin while he flies, stay still in his tail
 * attack, and otherwise wave, spinning up and spreading as he runs */
static void tails_draw(struct task *task)
{
    struct task_work *work;
    struct task_motion *motion;
    struct player *record;
    int action;
    NJS_OBJECT *tail_a;
    NJS_OBJECT *tail_b;
    int step;
    Angle ang_a;
    Angle ang_b;
    Angle saved_a;
    Angle saved_b;
    Float frames;
    Float frame;
    int i;

    work = task->work;
    motion = task->motion;
    record = motion->record;
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        return;
    }
    draw_set_palette(DRAW_PLAYER_PALETTE);
    if (TAILS_FLAGS(work) & TAILS_TAIL_SPIN) {
        record->own.tails.flight = 0.0F;
        record->own.tails.spin = 1.0F;
    }
    action = record->anim.action;
    if (record->anim.state == PLAYER_ANIM_BLENDING) {
        action = record->anim.current;
    }
    if (!(TASK_TIMER(work) & BLINK_HIDDEN)) {
        njSetTexture(&tails_texlist);
        njPushMatrix(NULL);
        /* @hack spelling: his middle's locals in a scope of their own, whose
         * stack space the blocks after it take once it ends: the frame is 60
         * bytes (ADD #-60,R15 at 0x8c0c1f58); declared with the function's
         * locals, it is 76 and 41 of 759 words differ */
        {
            NJS_VECTOR middle;
            NJS_POINT3 position;
            struct collision_shape *shape;

            middle.x = middle.z = 0.0F;
            middle.y = record->consts.height * 0.5F;
            njPushMatrix(&_unit_matrix_);
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
            TURN(njRotateY, -work->ang.y);
            njCalcVector(NULL, &middle, &position);
            njPopMatrix(1);
            position.x += work->pos.x;
            position.y += work->pos.y;
            position.z += work->pos.z;
            njTranslateV(NULL, &position);
            shape = work->collision->shapes;
            shape->center.x = position.x;
            shape->center.y = position.y;
            shape->center.z = position.z;
        }
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, 0x8000 - work->ang.y);
        if (action == ACTION_JUMP && (work->flags & PLAYER_ON_GROUND)) {
            NJS_VECTOR shift;

            shift.x = 0.0F;
            shift.y = -1.0F;
            shift.z = 0.0F;
            njTranslateV(NULL, &shift);
            njRotateZ(NULL, 0x2000);
            shift.x = 0.7F;
            shift.y = 1.1F;
            shift.z = 0.8F;
            njScaleV(NULL, &shift);
        }
        if (work->event->motions != NULL) {
            event_motions_draw(work);
        } else {
            if (record->anim.state != PLAYER_ANIM_BLENDING) {
                if (work->flags & PLAYER_BALL) {
                    if (++work->tick & BALL_TICKS) {
                        action = ACTION_BALL;
                    }
                }
                draw_action(record->anim.actions[action].action,
                            record->anim.frame);
                if (action == ACTION_JUMP && (work->flags & PLAYER_ON_GROUND)) {
                    NJS_ARGB color;

                    njSetConstantAttr(_nj_constant_attr_and_,
                                      _nj_constant_attr_or_ |
                                          NJD_FLAG_USE_ALPHA);
                    frame = record->anim.frame;
                    frames = (Float)record->anim.actions[record->anim.action]
                                 .action->motion->nbFrame;
                    color.r = 0.0F;
                    color.g = 0.0F;
                    color.b = 0.0F;
                    /* two afterimages, each 2 frames behind the last. the
                     * constant material takes 1/3 off the first's alpha and
                     * 2/3 off the second's */
                    for (i = 0; i < 2; i++) {
                        frame -= 2.0F;
                        if (frame < 0.0F) {
                            frame += frames;
                        }
                        color.a = -((Float)(i + 1) * (1.0F / 3.0F));
                        njSetConstantMaterial(&color);
                        draw_action(
                            record->anim.actions[record->anim.action].action,
                            frame);
                    }
                    color.a = 0.0F;
                    njSetConstantAttr(_nj_constant_attr_and_,
                                      _nj_constant_attr_or_ &
                                          ~NJD_FLAG_USE_ALPHA);
                    njSetConstantMaterial(&color);
                }
            } else {
                draw_action(record->anim.blend, record->anim.frame);
            }
            if (PLAYER_VARS(work)->player == 0 ||
                (TAILS_FLAGS(work) & TAILS_TAIL_SPIN)) {
                NJS_ARGB color;

                /* his flying tails, in his flight's actions or while
                 * TAILS_TAIL_SPIN is set */
                /* @hack spelling: ?: in place of ||, which loads
                 * tails_fly_tail1's address into R2 and tails_fly_tail0's
                 * into R3 (0x8c0c23c6, 0x8c0c23c8); with ||, the two trade
                 * and 4 of 759 words differ */
                if ((record->anim.action == ACTION_FLY ||
                     record->anim.action == ACTION_FLY_TIRING ||
                     record->anim.action == ACTION_FLY_TIRED ||
                     record->anim.action == ACTION_FLY_FAST ||
                     record->anim.action == ACTION_HOVER)
                        ? 1
                        : (TAILS_FLAGS(work) & TAILS_TAIL_SPIN)) {
                    tail_a = tails_fly_tail0;
                    tail_b = tails_fly_tail1;
                    if (record->own.tails.flight < FLIGHT_TIRING) {
                        step = AFTERIMAGE_TURN;
                    } else {
                        if (record->anim.action == ACTION_FLY) {
                            record->anim.action = ACTION_FLY_TIRING;
                            record->anim.state = PLAYER_ANIM_BLENDING;
                        }
                        /* from AFTERIMAGE_TURN down to none, as the rest of
                         * his flight runs out */
                        step =
                            (int)((1.0F - record->own.tails.flight) *
                                  (AFTERIMAGE_TURN / (1.0F - FLIGHT_TIRING)));
                    }
                } else {
                    /* @hack spelling: the tails stored in both arms, which
                     * keeps tails_tail0's address in R9 (MOV.L
                     * @(0x8c0c21d0),R9 at 0x8c0c2186); stored once, ahead of
                     * the test, 714 of 759 words differ */
                    if (record->own.tails.spin != 0.0F) {
                        tail_a = tails_tail0;
                        tail_b = tails_tail1;
                        step = (int)(record->own.tails.spin * AFTERIMAGE_TURN);
                    } else {
                        tail_a = tails_tail0;
                        tail_b = tails_tail1;
                        step = 640;
                    }
                }
                ang_a = saved_a = tail_a->ang[0];
                ang_b = saved_b = tail_b->ang[0];
                njSetConstantAttr(_nj_constant_attr_and_,
                                  _nj_constant_attr_or_ | NJD_FLAG_USE_ALPHA);
                color.r = 0.0F;
                color.g = 0.0F;
                color.b = 0.0F;
                /* two afterimages, each with the tails turned back by step
                 * from the last. the constant material takes 1/3 off the
                 * first's alpha and 2/3 off the second's */
                for (i = 0, color.a = -(1.0F / 3.0F); i < 2;
                     i++, color.a += -(1.0F / 3.0F)) {
                    if (!stage_paused()) {
                        ang_a -= step;
                        ang_b -= step;
                    }
                    tail_a->ang[0] = ang_a;
                    tail_b->ang[0] = ang_b;
                    njSetConstantMaterial(&color);
                    if (record->anim.state != PLAYER_ANIM_BLENDING) {
                        draw_action(
                            record->anim.actions[record->anim.action].action,
                            record->anim.frame);
                    } else {
                        draw_action(record->anim.blend, record->anim.frame);
                    }
                }
                color.a = 0.0F;
                njSetConstantAttr(_nj_constant_attr_and_,
                                  _nj_constant_attr_or_ & ~NJD_FLAG_USE_ALPHA);
                tail_a->ang[0] = saved_a;
                tail_b->ang[0] = saved_b;
                tail_a->ang[2] = 0xf800;
                tail_b->ang[2] = 0xf800;
                njSetConstantMaterial(&color);
            }
        }
#ifdef GCC
        /* the split frame's second picture draws the tails as step 0 left
         * them. it moves them no further, so that they turn and wave once a
         * frame, as at 30 fps */
        if (frame_redrawing) {
            njPopMatrix(1);
            draw_set_palette(0);
            return;
        }
#endif
        /* in flight, or with TAILS_TAIL_SPIN set, his flying tails spin.
         * in his tail attack, or with TAILS_TAIL_STILL set, they stay
         * still. otherwise his tails wave, and spin up as he runs */
        /* @hack spelling: ?: in place of || again, which loads
         * stage_paused's address into R3 (0x8c0c25ba); with ||, R2, and 4
         * of 759 words differ */
        if (((record->anim.action == ACTION_FLY ||
              record->anim.action == ACTION_FLY_TIRING ||
              record->anim.action == ACTION_FLY_TIRED ||
              record->anim.action == ACTION_FLY_FAST ||
              record->anim.action == ACTION_HOVER)
                 ? 1
                 : (TAILS_FLAGS(work) & TAILS_TAIL_SPIN)) &&
            !stage_paused()) {
            tails_spin_tails(work, record);
        } else if ((record->anim.action >= ACTION_ATTACK &&
                    record->anim.action <= ACTION_ATTACK_UP_RIGHT) ||
                   (TAILS_FLAGS(work) & TAILS_TAIL_STILL)) {
            tails_fly_tail0->ang[0] = 0;
            tails_fly_tail1->ang[0] = 0;
        } else {
            if (!stage_paused()) {
                tails_wave_tails();
            }
            if (record->anim.action == ACTION_FULL_SPEED ||
                record->anim.action == ACTION_SPRINT ||
                record->anim.action == ACTION_RUN) {
                record->own.tails.spin += 0.01F;
            } else {
                record->own.tails.spin -= 0.01F;
            }
            /* with no spin left, they keep turning, TAIL_SETTLE_STEP a step
             * beyond the turn below, until tail 0 comes round to within
             * TAIL_SETTLE_ZONE past rest. then both rest, and the turn below
             * takes them on to 0x333 */
            /* @hack spelling: the zero on the left of the test, which SHC
             * loads into FR3 (FLDI0 FR3 at 0x8c0c267c); on the right, FR2,
             * and 4 of 759 words differ */
            if (0.0F >= record->own.tails.spin) {
                record->own.tails.spin = 0.0F;
                if ((Uint16)tails_tail0->ang[0] < TAIL_SETTLE_ZONE) {
                    tails_tail0->ang[0] = 0;
                    tails_tail1->ang[0] = 0;
                } else if (!stage_paused()) {
                    /* @hack spelling: TAIL_SETTLE_STEP through step, set
                     * inside the first add, which loads it into R4 (MOV.W
                     * @(0x8c0c26c6),R4 at 0x8c0c26a2) and adds the
                     * assignment's copy in R3; as the constant in both adds,
                     * 19 of 759 words differ */
                    tails_tail0->ang[0] += (step = TAIL_SETTLE_STEP);
                    tails_tail1->ang[0] += step;
                }
            } else if (record->own.tails.spin >= 1.0F) {
                record->own.tails.spin = 1.0F;
                tails_tail1_end->model = tails_fly_tail1_model;
                tails_tail0_end->model = tails_fly_tail0_model;
            } else {
                model_blend(tails_tail1_wave_model, tails_fly_tail1_model,
                            tails_tail1_blend_model, record->own.tails.spin);
                model_blend(tails_tail0_wave_model, tails_fly_tail0_model,
                            tails_tail0_blend_model, record->own.tails.spin);
                tails_tail1_end->model = tails_tail1_blend_model;
                tails_tail0_end->model = tails_tail0_blend_model;
            }
            if (!stage_paused()) {
                tails_tail0->ang[0] +=
                    (int)((0.2F + ((1.0F - 0.2F) * record->own.tails.spin)) *
                          4096.0F);
                tails_tail1->ang[0] = tails_tail0->ang[0] +
                                      (int)(record->own.tails.spin * 32768.0F);
            }
        }
        njPopMatrix(1);
    }
    draw_set_palette(0);
}

/* his action as he stands. on ground within 25 degrees of level, he idles,
 * and fidgets once he has stood FIDGET_STEPS. on ground up to 45 degrees
 * steep, his action shows whether he faces up or down it */
static void tails_set_stand_action(struct task_work *work,
                                   struct player *record)
{
    Float slope;
    NJS_PLANE plane;
    NJS_VECTOR front;
    NJS_VECTOR up;
    NJS_VECTOR ahead;
    int ang;
    int action;

    if (record->anim.action == ACTION_UNCURL) {
        return;
    }
    slope = -njInnerProduct(&world_gravity, &record->ground_normal);
    if (slope > njCos(NJM_DEG_ANG(25))) {
        action = record->anim.action;
        if (action != ACTION_STAND && action != ACTION_LAND &&
            action != ACTION_LANDED && action != ACTION_STAND_ONCE &&
            action != ACTION_FIDGET_START && action != ACTION_FIDGET &&
            action != ACTION_FIDGET_END) {
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
        vector_normalize(&ahead);
        njDistanceP2PL(&world_gravity, &plane, &up);
        vector_normalize(&up);
        ang = vector_angle(&ahead, &up, &record->ground_normal);
        if (ang > 0xc000 || ang < 0x4000) {
            record->anim.action = ACTION_STAND_DOWNHILL;
        } else {
            record->anim.action = ACTION_STAND_UPHILL;
        }
    }
    record->idle_steps += 1;
}

/* his action as he walks, by his speed and whether he holds something,
 * unless he's uncurling or turning. its pace goes by his speed */
static void tails_set_walk_action(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record)
{
    Float speed = record->speed.x + (motion->slip * 5.0F);
    int action = record->anim.action;

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
    } else {
        if (speed > record->consts.full_speed) {
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
    }
    record->action_speed = record->speed.x + (motion->slip * 20.0F);
}

/* hanging from a path, he hangs his height below it. as he moves, his
 * action's frame moves 2 a step, forward or back with his speed. once the
 * stick is released, he stops where the frame reaches 0 or 32, the two
 * still hangs */
static void tails_hang_move(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    NJS_VECTOR up;

    up.x = 0.0F;
    up.y = record->consts.height;
    up.z = 0.0F;
    player_turn_to_world(work, &up);
    njSubVector(&work->pos, &up);

    if (record->speed.x != 0.0F && motion->drive == 0.0F) {
        if (record->anim.frame == 0.0F || record->anim.frame >= 32.0F) {
            work->mode = TAILS_HANG;
            if (record->anim.frame == 0.0F) {
                record->anim.action = ACTION_HANG_SWAPPED;
            } else {
                record->anim.action = ACTION_HANG;
            }
            record->speed.x = 0.0F;
        } else {
            work->mode = TAILS_HANG;
            record->speed.x = 0.0F;
            if (record->anim.frame >= 16.0F && record->anim.frame <= 48.0F) {
                record->anim.action = ACTION_HANG;
            } else {
                record->anim.action = ACTION_HANG_SWAPPED;
            }
        }
    } else if (record->speed.x > 0.0F) {
        if (record->anim.frame >= 0.0F && record->anim.frame < 32.0F) {
            record->anim.frame += 2.0F;
            if (record->anim.frame == 32.0F &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = TAILS_HANG;
                record->anim.action = ACTION_HANG;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame += 2.0F;
            if (record->anim.frame >= 63.0F) {
                record->anim.frame = 0.0F;
            }
            if (record->anim.frame == 0.0F &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = TAILS_HANG;
                record->anim.action = ACTION_HANG_SWAPPED;
                record->speed.x = 0.0F;
            }
        }
    } else if (record->speed.x < 0.0F) {
        if (record->anim.frame > 0.0F && record->anim.frame <= 32.0F) {
            record->anim.frame -= 2.0F;
            if (record->anim.frame == 0.0F &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = TAILS_HANG;
                record->anim.action = ACTION_HANG_SWAPPED;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame -= 2.0F;
            if (record->anim.frame < 0.0F) {
                record->anim.frame = 62.0F;
            }
            if (record->anim.frame == 32.0F &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = TAILS_HANG;
                record->anim.action = ACTION_HANG;
                record->speed.x = 0.0F;
            }
        }
    } else {
        record->speed.x = 0.0F;
    }
}

/* the same on a ledge, stopping at frame 16 or 30 */
static void tails_ledge_move(struct task_work *work, struct task_motion *motion,
                             struct player *record)
{
    NJS_VECTOR up;

    up.x = 0.0F;
    up.y = record->consts.height;
    up.z = 0.0F;
    player_turn_to_world(work, &up);
    njSubVector(&work->pos, &up);

    if (record->speed.x != 0.0F && motion->drive == 0.0F) {
        if (record->anim.frame == 16.0F || record->anim.frame == 30.0F) {
            work->mode = TAILS_LEDGE;
            record->speed.x = 0.0F;
        }
    } else if (record->speed.x > 0.0F) {
        if (record->anim.frame >= 0.0F && record->anim.frame < 16.0F) {
            record->anim.frame += 2.0F;
            if (record->anim.frame == 16.0F &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = TAILS_LEDGE;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame += 2.0F;
            if (record->anim.frame >= 32.0F) {
                record->anim.frame = 0.0F;
            }
            if (record->anim.frame == 30.0F &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = TAILS_LEDGE;
                record->speed.x = 0.0F;
            }
        }
    } else if (record->speed.x < 0.0F) {
        if (record->anim.frame > 16.0F && record->anim.frame <= 32.0F) {
            record->anim.frame -= 2.0F;
            if (record->anim.frame == 16.0F &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = TAILS_LEDGE;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame -= 2.0F;
            if (record->anim.frame < 0.0F) {
                record->anim.frame = 30.0F;
            }
            if (record->anim.frame == 30.0F &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = TAILS_LEDGE;
                record->speed.x = 0.0F;
            }
        }
    } else {
        record->speed.x = 0.0F;
    }
}

/* on his board he leans the way the stick turns him, his action's frame
 * easing toward the lean. while the jump or action button is held he
 * crouches, and once he's slow he stands */
static void tails_set_board_action(struct task_work *work,
                                   struct task_motion *motion,
                                   struct player *record)
{
    Float lean;

    if (record->anim.action != ACTION_BOARD &&
        record->anim.action != ACTION_BOARD_CROUCH &&
        record->anim.action != ACTION_BOARD_LEAN_LEFT &&
        record->anim.action != ACTION_BOARD_LEAN_RIGHT &&
        record->anim.action != ACTION_BOARD_SLOW &&
        record->anim.action != ACTION_BOARD_SLOW_ONCE) {
        return;
    }

    if (player_read_stick(work, NULL, NULL)) {
        if (motion->drive < 0.0F) {
            if (record->anim.action == ACTION_BOARD ||
                record->anim.action == ACTION_BOARD_CROUCH ||
                record->anim.action == ACTION_BOARD_SLOW ||
                record->anim.action == ACTION_BOARD_SLOW_ONCE) {
                record->anim.action = ACTION_BOARD_LEAN_LEFT;
                record->anim.frame = 0.0F;
                lean = -(motion->drive * 12.0F) + 4.0F;
            } else if (record->anim.action == ACTION_BOARD_LEAN_RIGHT) {
                if (record->anim.frame == 0.0F) {
                    record->anim.action = ACTION_BOARD_LEAN_LEFT;
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
                record->anim.action == ACTION_BOARD_SLOW_ONCE) {
                record->anim.action = ACTION_BOARD_LEAN_RIGHT;
                record->anim.frame = 0.0F;
                lean = (motion->drive * 12.0F) + 4.0F;
            } else if (record->anim.action == ACTION_BOARD_LEAN_LEFT) {
                if (record->anim.frame == 0.0F) {
                    record->anim.action = ACTION_BOARD_LEAN_RIGHT;
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
                record->anim.action == ACTION_BOARD_SLOW_ONCE) {
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
    } else if (record->anim.action == ACTION_BOARD_LEAN_LEFT ||
               record->anim.action == ACTION_BOARD_LEAN_RIGHT) {
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
                return;
            }
            if (record->anim.action == ACTION_BOARD_SLOW &&
                record->speed.x < PLAYER_BOARD_SPEED * 0.3F) {
                record->anim.action = ACTION_BOARD_SLOW_ONCE;
            }
        }
    }
}

/* his board's sound along the ground at a trot or faster, for as long as
 * his mode lasts. in Ice Cap it goes by the ground's grip, and elsewhere it
 * is sand's */
static void tails_board_sound(struct task_work *work, struct player *record)
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

/* a hit, or ground that hurts, starts his hurt timer at PLAYER_HURT_STEPS
 * and shakes the pad. on his board he then crashes, and in any other mode
 * but TAILS_MODULE_POSE he's knocked back and up. in TAILS_MODULE_POSE,
 * PLAYER_HURT stays set. a work with RACER_ID, which race.c gives only
 * Sonic as his rival, is never hurt. it returns TRUE when he's knocked
 * back, and FALSE otherwise, a crash included */
static Bool tails_try_hurt(struct task_work *work, struct player *record)
{
    if (work->id != RACER_ID &&
        ((work->flags & PLAYER_HURT) ||
         (record != NULL && (record->ground_flags & WORLD_HURTS)))) {
        work->timer = PLAYER_HURT_STEPS;
        vibration_start_fading(PLAYER_VARS(work)->player, 7, 59, 6);
        if (work->mode >= TAILS_BOARD && work->mode <= TAILS_BOARD_CRASH) {
            tails_sound(work, SE_HURT);
            work->mode = TAILS_BOARD_CRASH;
            record->anim.action = ACTION_BOARD_CRASH_B;
            work->flags &= ~PLAYER_HURT;
        } else if (work->mode != TAILS_MODULE_POSE) {
            tails_sound(work, SE_HURT);
            work->mode = TAILS_HURT;
            record->anim.action = ACTION_HURT;
            work->flags &= ~PLAYER_HURT;
            player_mouth_set_script(PLAYER_VARS(work)->player, 8);
            record->speed.y = record->consts.jump_speed;
            record->speed.x = -record->consts.running_speed;
            work->flags &= ~PLAYER_ATTACKING;
            return TRUE;
        }
    }
    return FALSE;
}

/* jump as player_jump_pressed allows. from a panel, he jumps to the next.
 * holding something he can carry, he jumps with it. otherwise he lets go
 * of what he holds and jumps curled up, with the speed of the task he
 * stands on. it returns TRUE when he jumps, and FALSE otherwise */
static Bool tails_try_jump(struct task_work *work, struct player *record)
{
    struct task_force *ground;
    int flags;

    if (player_jump_pressed(work, record) == PLAYER_JUMP_PANEL) {
        if (work->flags & PLAYER_HOLDING) {
            tails_let_go(work, record);
        }
        panel_jump(work);
        work->mode = TAILS_PANEL_JUMP;
        record->anim.action = ACTION_PANEL_JUMP;
        work->flags &= ~PLAYER_ON_TASK;
        record->jump_steps = 0;
        tails_sound(work, SE_JUMP);
        return TRUE;
    } else if (player_jump_pressed(work, record)) {
        if (work->flags & PLAYER_HOLDING) {
            flags = player_carry_flags(work);
            if (flags & PLAYER_CAN_CARRY) {
                work->mode = TAILS_HOLD_JUMP;
                record->speed.y = record->consts.jump_speed;
                record->anim.action = ACTION_HOLD_STAND;
                work->flags &= ~PLAYER_ON_TASK;
                record->jump_steps = 0;
                tails_sound(work, SE_JUMP);
                return TRUE;
            } else {
                tails_let_go(work, record);
            }
        }
        work->mode = TAILS_JUMP;
        record->speed.y = record->consts.jump_speed;
        if (record->ground_task != NULL) {
            ground = record->ground_task->force;
            if (ground != NULL) {
                player_set_accel(PLAYER_VARS(work)->player, ground->speed.x,
                                 ground->speed.y, ground->speed.z);
            }
        }
        record->anim.action = ACTION_JUMP;
        record->action_speed = 2.0F;
        work->flags &= ~PLAYER_ON_TASK;
        work->flags |= PLAYER_ATTACKING;
        record->jump_steps = 0;
        tails_sound(work, SE_JUMP);
        return TRUE;
    }
    return FALSE;
}

/* curled up in the air, pressing the action button uncurls him, and he
 * falls. it returns TRUE when he uncurls, and FALSE otherwise */
static Bool tails_try_uncurl(struct task_work *work, struct player *record)
{
    if (pad_records[PLAYER_VARS(work)->player].press & pad_action_buttons) {
        work->mode = TAILS_FALL;
        player_clear_speed(NULL, record);
        record->anim.action = ACTION_FALL;
        record->action_speed = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        return TRUE;
    }
    return FALSE;
}

/* take the action an event gives him: driven by the event to move, or
 * turned. it returns TRUE for those. for any other action it clears
 * PLAYER_EVENT_DRIVEN and returns FALSE */
static Bool tails_follow_event(struct task_work *work)
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

/* take what's asked of him: an event's action, then its paths, then the
 * submode PLAYER_REQUEST asks for. it returns TRUE once the paths or the
 * submode have decided his mode, and the caller then checks nothing more.
 * it returns FALSE when nothing is asked, or what's asked sets no mode.
 * three of those still act: tails_follow_event takes an event's action,
 * PLAYER_MODE_JOIN_PATH can set PLAYER_ON_PATH, and PLAYER_MODE_CIRCLE
 * while he circles clears the submode */
static Bool tails_try_request(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    struct collision_shape *shape;
    struct task_work *target;

    if (work->event->action) {
        tails_follow_event(work);
        return FALSE;
    }
    if (work->event->paths != NULL) {
        work->mode = TAILS_EVENT;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        return TRUE;
    }
    if (!(work->flags & PLAYER_REQUEST)) {
        return FALSE;
    }
    switch (work->submode) {
    case PLAYER_MODE_SPRING:
    case PLAYER_MODE_BOUNCE_3:
        work->mode = TAILS_SPRING;
        record->anim.action = ACTION_SPRING;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_BOUNCE_1:
        work->mode = TAILS_SPRING;
        record->anim.action = ACTION_BOUNCE_1;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_BOUNCE_2:
        work->mode = TAILS_SPRING;
        record->anim.action = ACTION_BOUNCE_2;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_LAUNCH:
        work->mode = TAILS_LAUNCH;
        record->anim.action = ACTION_SPRING;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_FLING:
        work->mode = TAILS_FLUNG;
        record->anim.action = ACTION_FLUNG;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_UPDRAFT:
        work->mode = TAILS_FALL;
        record->anim.action = ACTION_UPDRAFT;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_PATH:
        work->mode = TAILS_PATH;
        record->anim.action = ACTION_RUN;
        record->path_distance = 0.0F;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_PATH_SPIN:
        work->mode = TAILS_PATH_SPIN;
        record->anim.action = ACTION_ROLL;
        record->path_distance = 0.0F;
        work->flags |= PLAYER_ATTACKING;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL:
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_STAND;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL_RUN:
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_RUN;
        player_clear_speed(motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_BALL:
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_JUMP;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags |= PLAYER_ATTACKING;
        record->action_speed = 2.0F;
        tails_sound(work, SE_SPIN);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL_PUSH:
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_PUSH_START;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_HANG_ON:
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_HANG_ON;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_GET_ON:
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_FALL;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_RIDE:
        work->mode = TAILS_MODULE_POSE;
        record->anim.action = ACTION_RIDE;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_CLEAR:
        work->mode = TAILS_CLEAR;
        if (race_outcome() == RACE_WON) {
            record->anim.action = ACTION_CLEAR;
        } else {
            record->anim.action = ACTION_LOST;
        }
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        TAILS_FLAGS(work) |= TAILS_UNTOUCHABLE;
        break;
    case PLAYER_MODE_ROCKET:
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_ROCKET;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_ROCKET_ALONG:
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_ROCKET_ALONG;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_ROCKET_UP:
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_ROCKET_UP;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_SQUASH_2:
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_SQUASH_2;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_SQUASH_3:
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_SQUASH_3;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RESET:
        tails_stand(work, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_JOIN_PATH:
        if (work->mode == TAILS_WALK ||
            record->speed.x > record->consts.running_speed) {
            work->flags |= PLAYER_ON_PATH;
        }
        return FALSE;
    case PLAYER_MODE_BOARD:
        work->mode = TAILS_BOARD_FALL;
        record->anim.action = ACTION_BOARD_FALL_START;
        record->consts.height += 2.0F;
        record->consts.center_y = record->consts.height * 0.5F;
        record->consts.jump_speed *= 1.2F;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_HANG:
        tails_sound(work, SE_GRAB);
        work->mode = TAILS_HANG;
        record->anim.action = ACTION_HANG;
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
        tails_sound(work, SE_GRAB);
        work->mode = TAILS_LEDGE;
        record->anim.action = ACTION_LEDGE;
        record->anim.frame = 0.0F;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_CIRCLE:
        if (work->mode >= TAILS_CIRCLE && work->mode <= TAILS_CIRCLE_RIGHT) {
            work->submode = 0;
            return FALSE;
        }
        target = record->held->work;
        shape = target->collision->shapes;
        if (work->pos.y > target->pos.y + shape->center.y - shape->size_y) {
            work->mode = TAILS_CIRCLE_DOWN;
            record->anim.action = ACTION_CIRCLE_DOWN;
        } else {
            work->mode = TAILS_CIRCLE;
            record->anim.action = ACTION_CIRCLE;
        }
        tails_sound(work, SE_GRAB);
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        TAILS_VARS(work)->struggle = 0;
        break;
    case PLAYER_MODE_SLOPE_ROLL:
        work->mode = TAILS_SLOPE_ROLL;
        record->anim.action = ACTION_SLOPE_ROLL;
        work->flags |= PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_FREEZE:
        if (work->mode != TAILS_HURT) {
            work->flags &= ~PLAYER_HURT;
            tails_sound(work, SE_HURT);
            work->mode = TAILS_HURT;
            record->anim.action = ACTION_HURT;
            player_mouth_set_script(0, 8);
            record->speed.y = record->consts.jump_speed;
            record->speed.x = -record->consts.running_speed;
            work->flags &= ~PLAYER_ATTACKING;
        }
        break;
    case PLAYER_MODE_KNOCK_BACK:
        work->mode = TAILS_CRASH;
        record->anim.action = ACTION_CRASH;
        tails_sound(work, SE_CRASH);
        player_crash_burst(work);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_READY:
        work->mode = TAILS_PLAY;
        record->anim.action = ACTION_STAND_ONCE;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_SHOWER:
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_SHOWER;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_SHOWER_END:
        work->mode = TAILS_PLAY;
        record->anim.action = ACTION_SHOWER_END;
        break;
    case PLAYER_MODE_MUD_OUT:
        work->mode = TAILS_PLAY;
        /* @bug memory: ACTION_MUD_OUT, the row after his shower's
         * end, is one past tails_actions' last row (MOV.W @(0x8c0c3f08),R2
         * at 0x8c0c3ece), so player_anim_step reads his action's row from
         * tails_shapes' first bytes */
#ifdef GCC
        /* repairs the read the tag names: GCC takes the last row, his
         * shower's end, which plays his stand once and goes to ACTION_STAND */
        record->anim.action = ACTION_SHOWER_END;
#else
        record->anim.action = ACTION_MUD_OUT;
#endif
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_BREATHE:
        work->mode = TAILS_PLAY;
        record->anim.action = ACTION_BREATHE;
        record->underwater_steps = 0;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_HOLD:
        work->mode = TAILS_HOLD_STAND;
        record->anim.action = ACTION_HOLD_STAND;
        record->held = PLAYER_VARS(work)->arg.carry;
        record->held->work->flags |= CARRY_HELD;
        work->flags |= PLAYER_HOLDING;
        work->flags &= ~COLLISION_TOUCHING;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_FLY:
        work->mode = TAILS_FLY;
        record->anim.action = ACTION_FLY;
        record->own.tails.flight = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_GUARD;
        TAILS_STATE(work) |= TAILS_FLYING;
        TAILS_FLAGS(work) &= ~TAILS_STOP_FLYING;
        tails_sound(work, SE_FLY);
        break;
    case PLAYER_MODE_HOVER:
        work->mode = TAILS_HOVER;
        record->anim.action = ACTION_HOVER;
        record->own.tails.flight = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_GUARD;
        TAILS_STATE(work) |= TAILS_FLYING;
        TAILS_FLAGS(work) &= ~TAILS_STOP_FLYING;
        break;
    case PLAYER_MODE_DROWN:
        work->mode = TAILS_RISE;
        record->anim.action = ACTION_DROWN;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_DOWN:
        work->mode = TAILS_DOWN;
        record->anim.action = ACTION_DOWN;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/* as tails_try_request, while he holds something, but without an
 * event's paths. he lets go of what he holds where the new mode needs his
 * hands */
static Bool tails_try_request_holding(struct task_work *work,
                                      struct task_motion *motion,
                                      struct player *record)
{
    if (work->event->action) {
        tails_follow_event(work);
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
        work->mode = TAILS_HOLD_SPRING;
        record->anim.action = ACTION_HOLD_SPRING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_LAUNCH:
        work->mode = TAILS_HOLD_LAUNCH;
        record->anim.action = ACTION_HOLD_SPRING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_UPDRAFT:
        work->mode = TAILS_HOLD_FALL;
        record->anim.action = ACTION_HOLD_FALL;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_PATH:
        tails_let_go(work, record);
        work->mode = TAILS_PATH;
        record->anim.action = ACTION_RUN;
        record->path_distance = 0.0F;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_PATH_SPIN:
        tails_let_go(work, record);
        work->mode = TAILS_PATH_SPIN;
        record->anim.action = ACTION_ROLL;
        record->path_distance = 0.0F;
        work->flags |= PLAYER_ATTACKING;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL:
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_HOLD_STAND;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_BALL:
        tails_let_go(work, record);
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_JUMP;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags |= PLAYER_ATTACKING;
        record->action_speed = 2.0F;
        tails_sound(work, SE_SPIN);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL_PUSH:
        tails_let_go(work, record);
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_PUSH_START;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL_RUN:
        tails_let_go(work, record);
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_RUN;
        player_clear_speed(motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_HANG_ON:
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_HANG_ON;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_GET_ON:
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_FALL;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RIDE:
        work->mode = TAILS_MODULE_POSE;
        record->anim.action = ACTION_RIDE;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_CLEAR:
        tails_let_go(work, record);
        work->mode = TAILS_CLEAR;
        if (race_outcome() == RACE_WON) {
            record->anim.action = ACTION_CLEAR;
        } else {
            record->anim.action = ACTION_LOST;
        }
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        TAILS_FLAGS(work) |= TAILS_UNTOUCHABLE;
        break;
    case PLAYER_MODE_ROCKET:
        tails_let_go(work, record);
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_ROCKET;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_SQUASH_2:
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_HOLD_STAND;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_SQUASH_3:
        work->mode = TAILS_POSE;
        record->anim.action = ACTION_HOLD_STAND;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RESET:
        work->mode = TAILS_HOLD_STAND;
        record->anim.action = ACTION_HOLD_STAND;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_JOIN_PATH:
        if (work->mode == TAILS_HOLD_WALK ||
            record->speed.x > record->consts.running_speed) {
            work->flags |= PLAYER_ON_PATH;
        }
        return FALSE;
    case PLAYER_MODE_HANG:
        tails_sound(work, SE_GRAB);
        tails_let_go(work, record);
        work->mode = TAILS_HANG;
        record->anim.action = ACTION_HANG;
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
        tails_let_go(work, record);
        tails_sound(work, SE_GRAB);
        work->mode = TAILS_LEDGE;
        record->anim.action = ACTION_LEDGE;
        record->anim.frame = 0.0F;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_FREEZE:
        if (work->mode != TAILS_HURT) {
            tails_let_go(work, record);
            work->flags &= ~PLAYER_HURT;
            tails_sound(work, SE_HURT);
            work->mode = TAILS_HURT;
            record->anim.action = ACTION_HURT;
            player_mouth_set_script(0, 8);
            record->speed.y = record->consts.jump_speed;
            record->speed.x = -record->consts.running_speed;
            work->flags &= ~PLAYER_ATTACKING;
        }
        break;
    case PLAYER_MODE_KNOCK_BACK:
        tails_let_go(work, record);
        work->mode = TAILS_CRASH;
        record->anim.action = ACTION_CRASH;
        tails_sound(work, SE_CRASH);
        player_crash_burst(work);
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_READY:
        tails_let_go(work, record);
        work->mode = TAILS_PLAY;
        record->anim.action = ACTION_STAND_ONCE;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_FLY:
        work->mode = TAILS_FLY;
        record->anim.action = ACTION_FLY;
        record->own.tails.flight = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_GUARD;
        TAILS_STATE(work) |= TAILS_FLYING;
        TAILS_FLAGS(work) &= ~TAILS_STOP_FLYING;
        tails_sound(work, SE_FLY);
        break;
    case PLAYER_MODE_HOVER:
        work->mode = TAILS_HOVER;
        record->anim.action = ACTION_HOVER;
        record->own.tails.flight = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_GUARD;
        TAILS_STATE(work) |= TAILS_FLYING;
        TAILS_FLAGS(work) &= ~TAILS_STOP_FLYING;
        break;
    case PLAYER_MODE_DROWN:
        tails_let_go(work, record);
        work->mode = TAILS_RISE;
        record->anim.action = ACTION_DROWN;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    case PLAYER_MODE_DOWN:
        tails_let_go(work, record);
        work->mode = TAILS_DOWN;
        record->anim.action = ACTION_DOWN;
        work->flags &= ~(PLAYER_ON_PATH | PLAYER_ATTACKING);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/* take the requests his board takes. PLAYER_MODE_RESET drops him off it,
 * falling, with his height set to 9. PLAYER_MODE_CLEAR stills him in the
 * clear's pose. it returns TRUE for any request, taken or not, and FALSE
 * with none */
static Bool tails_try_request_board(struct task_work *work,
                                    struct task_motion *motion,
                                    struct player *record)
{
    if (!(work->flags & PLAYER_REQUEST)) {
        return FALSE;
    }
    switch (work->submode) {
    case PLAYER_MODE_RESET:
        work->mode = TAILS_FALL;
        record->anim.action = ACTION_FALL;
        record->consts.height = 9.0F;
        record->consts.center_y = record->consts.height * 0.5F;
        break;
    case PLAYER_MODE_CLEAR:
        work->mode = TAILS_CLEAR;
        if (race_outcome() == RACE_WON) {
            record->anim.action = ACTION_CLEAR;
        } else {
            record->anim.action = ACTION_LOST;
        }
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        TAILS_FLAGS(work) |= TAILS_UNTOUCHABLE;
        break;
    }
    return TRUE;
}

/* off the ground, he falls, still curled up if he was, and lets go of
 * what he holds unless he can fall with it. it returns TRUE when he
 * falls, and FALSE on the ground */
static Bool tails_try_fall(struct task_work *work, struct player *record)
{
    if (!(work->flags & PLAYER_ON_GROUND)) {
        if (work->flags & PLAYER_BALL) {
            work->mode = TAILS_JUMP;
            return TRUE;
        } else if (work->flags & PLAYER_HOLDING) {
            if (player_carry_flags(work) & PLAYER_CAN_CARRY) {
                work->mode = TAILS_HOLD_FALL;
                record->anim.action = ACTION_HOLD_FALL;
                return TRUE;
            }
            tails_let_go(work, record);
        }
        work->mode = TAILS_FALL;
        record->anim.action = ACTION_FALL;
        return TRUE;
    }
    return FALSE;
}

/* with the stick released and no speed, he stops: holding something, he
 * grips it or stands holding it. it returns TRUE when he stops, and FALSE
 * otherwise */
static Bool tails_try_stop(struct task_work *work, struct player *record)
{
    if (!player_read_stick(work, NULL, NULL) && record->speed.x == 0.0F) {
        if (work->flags & PLAYER_HOLDING) {
            if (player_carry_flags(work) &
                (PLAYER_CAN_PUSH | PLAYER_CAN_SHAKE)) {
                work->mode = TAILS_GRIP;
                record->anim.action = ACTION_GRIP;
            } else {
                work->mode = TAILS_HOLD_STAND;
                record->anim.action = ACTION_HOLD_STAND;
            }
        } else {
            tails_stand(work, record);
        }
        return TRUE;
    }
    return FALSE;
}

/* rolling, slower than he runs, he gets up: standing, or walking on. it
 * returns TRUE when he gets up, and FALSE otherwise */
static Bool tails_try_unroll(struct task_work *work, struct player *record)
{
    Uint16 action;

    if (record->speed.x < record->consts.running_speed) {
        /* @hack spelling: the action set first, in a local, whose MOV #36
         * fills the BT/S's delay slot (0x8c0c4854); stored straight after
         * the mode, 7 of 22 words differ */
        action = ACTION_UNCURL;
        work->mode = (record->speed.x <= 0.0F) ? TAILS_STAND : TAILS_WALK;
        record->anim.action = action;
        return TRUE;
    }
    return FALSE;
}

/* pressing the grab button at what he touches, whichever way he faces, he
 * grips it or starts to lift it, as player_carry_flags allows. it returns
 * TRUE when he does, and FALSE otherwise */
static Bool tails_try_grab(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    int flags;

    if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                    pad_grab_buttons)) {
        if ((flags = player_carry_flags(work)) != 0) {
            /* @hack spelling: the four cases, which hold every value
             * player_face_wall gives, end in an empty statement, and the
             * tests follow the switch. the last compare is then a BF to the
             * default (BF at 0x8c0c48ea); with the tests inside the last
             * case, it becomes BT over a BRA and 61 of 89 words differ */
            switch (player_face_wall(work, motion)) {
            default:
                return FALSE;
            case PLAYER_WALL_AWAY:
            case PLAYER_WALL_ACROSS:
            case PLAYER_WALL_ASKEW:
            case PLAYER_WALL_HEAD_ON:;
            }
            if (flags & PLAYER_CAN_PUSH) {
                work->mode = TAILS_GRIP;
                record->anim.action = ACTION_GRIP_START;
                record->held = work->collision->hit->owner;
                work->flags |= PLAYER_HOLDING;
                return TRUE;
            } else if (flags & PLAYER_CAN_CARRY) {
                work->mode = TAILS_LIFT;
                record->anim.action = ACTION_LIFT;
                if (record->speed.x >= record->consts.trot_speed) {
                    record->speed.x = record->consts.trot_speed;
                }
                return TRUE;
            } else if (flags & PLAYER_CAN_SHAKE) {
                work->mode = TAILS_GRIP;
                record->anim.action = ACTION_GRIP_START;
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* from frame 12 of the lift, he takes what he faces head on if he can
 * carry it, and it returns TRUE. it returns FALSE before frame 12, and
 * from frame 12 when he can't take it, as he gives up and stands */
static Bool tails_try_take_lifted(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record)
{
    int flags;

    player_approach_touched(work, motion);
    if (record->anim.frame >= 12.0F) {
        if ((flags = player_carry_flags(work)) != 0) {
            /* @hack spelling: a switch of one case, which loads tails_stand's
             * address into R2 for the call after it (0x8c0c4a20); as an if,
             * R3, and 2 of 69 words differ */
            switch (player_face_wall(work, motion)) {
            case PLAYER_WALL_HEAD_ON:
                if (flags & PLAYER_CAN_CARRY) {
                    work->mode = TAILS_HOLD_STAND;
                    tails_sound(work, SE_LIFT);
                    record->held = work->collision->hit->owner;
                    record->held->work->flags |= CARRY_HELD;
                    work->flags |= PLAYER_HOLDING;
                    work->flags &= ~COLLISION_TOUCHING;
                    return TRUE;
                }
                break;
            }
        }
        tails_stand(work, record);
    }
    return FALSE;
}

/* pressing the grab button as he holds something, he throws it, in the
 * air or on the run, or sets it down. once it's out of his hands, he
 * stands or falls. it returns TRUE when he throws it or sets it down, and
 * when it's out of his hands, and FALSE otherwise */
static Bool tails_try_throw(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    if (pad_records[PLAYER_VARS(work)->player].press & pad_grab_buttons) {
        if (!(work->flags & PLAYER_ON_GROUND)) {
            work->mode = TAILS_AIR_THROW;
            if (player_has_target(PLAYER_VARS(work)->player)) {
                record->anim.action = ACTION_AIR_THROW_AT;
            } else {
                record->anim.action = ACTION_AIR_THROW;
            }
        } else if (record->speed.x < record->consts.trot_speed) {
            work->mode = TAILS_PUT_DOWN;
            record->anim.action = ACTION_PUT_DOWN;
            player_clear_speed(motion, record);
        } else {
            work->mode = TAILS_THROW;
            if (player_has_target(PLAYER_VARS(work)->player)) {
                record->anim.action = ACTION_THROW_AT;
            } else {
                record->anim.action = ACTION_THROW;
            }
        }
        return TRUE;
    } else if (!(work->flags & PLAYER_HOLDING)) {
        if (work->flags & PLAYER_ON_GROUND) {
            tails_stand(work, record);
        } else {
            work->mode = TAILS_FALL;
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

/* at the throw's frame he lets go: at the nearest task, ahead, or set
 * down. it returns TRUE when he lets go. with nothing in his hands, it
 * clears PLAYER_HOLDING and returns TRUE if that was set, and FALSE if
 * not. it returns FALSE otherwise */
static Bool tails_try_release(struct task_work *work, struct player *record)
{
    struct near_task *near;
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
    /* @hack spelling: action assigned inside the test, which SHC extends in R0
     * (EXTU.W R6,R0 at 0x8c0c4be8); assigned in a statement before it, in
     * R6, the copies for njSqrt trade frame slots and 65 of 277 words
     * differ */
    if ((((action = record->anim.action) == ACTION_THROW_AT &&
          record->anim.frame == 20.0F) ||
         (action == ACTION_AIR_THROW_AT && record->anim.frame == 7.0F)) &&
        near->work != NULL) {
        held = record->held->work;
        if (held != NULL) {
            held->flags &= ~CARRY_HELD;
            target = near->work;
            distance = near->distance;
            for (; near->work != NULL; near++) {
                if (near->distance < distance) {
                    distance = near->distance;
                    target = near->work;
                }
            }
            shape = target->collision->shapes;
            toss.x = target->pos.x + shape->center.x - work->pos.x;
            toss.y = target->pos.y + shape->center.y - work->pos.y;
            toss.z = target->pos.z + shape->center.z - work->pos.z;
            toss.x = njSqrt((toss.x * toss.x) + (toss.z * toss.z));
            toss.z = 0.0F;
            vector_normalize(&toss);
            toss.x *= 3.0F;
            toss.y *= 3.0F;
            player_turn_to_world(work, &toss);
            record->held->motion->speed = toss;
        }
        work->flags &= ~PLAYER_HOLDING;
        record->held = NULL;
        tails_sound(work, SE_THROW);
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
        tails_sound(work, SE_THROW);
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

/* with the grab button held, he pushes or pulls what he grips, or shakes
 * it. with the button released, he lifts it or lets go. it returns FALSE
 * only while the button is held and player_stick_into_wall gives
 * PLAYER_WALL_NO_STICK, and TRUE otherwise */
static Bool tails_try_grip(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    int wall;

    if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                    pad_grab_buttons)) {
        wall = player_stick_into_wall(work, motion);
        if (wall == PLAYER_WALL_NO_STICK) {
            return FALSE;
        } else if (player_carry_flags(work) & PLAYER_CAN_SHAKE) {
            work->mode = TAILS_SHAKE;
            record->anim.action = ACTION_SHAKE;
            return TRUE;
        } else if (wall == PLAYER_WALL_HEAD_ON) {
            work->mode = TAILS_PUSH;
            record->anim.action = ACTION_PUSH;
            tails_sound(work, SE_PUSH_START);
            return TRUE;
        } else {
            work->mode = TAILS_PULL;
            record->anim.action = ACTION_PULL;
            return TRUE;
        }
    } else {
        if (player_carry_flags(work) & PLAYER_CAN_CARRY) {
            work->mode = TAILS_LIFT;
            record->anim.action = ACTION_LIFT;
            if (record->speed.x >= record->consts.trot_speed) {
                record->speed.x = record->consts.trot_speed;
            }
        } else {
            tails_let_go(work, record);
            tails_stand(work, record);
        }
        return TRUE;
    }
    return FALSE;
}

/* he lets go of what he holds, and tosses it ahead if it's a thing to
 * carry */
static void tails_let_go(struct task_work *work, struct player *record)
{
    NJS_VECTOR toss;
    struct task_work *held;

    if (work->flags & PLAYER_HOLDING) {
        held = record->held->work;
        if (held != NULL) {
            held->flags &= ~CARRY_HELD;
        }
        record->held->work->flags &= ~CARRY_HELD;
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

/* what he carries stays between his hands, or out past one as he throws.
 * what he grips he lets go of once he touches something else */
static void tails_carry(struct task_work *work, struct player *record)
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
            tails_let_go(work, record);
            if (record->held == NULL) {
                if (work->flags & PLAYER_ON_GROUND) {
                    tails_stand(work, record);
                } else {
                    work->mode = TAILS_FALL;
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

/* he stays facing what he holds, its reach away, or CIRCLE_REACH if it has
 * no motion */
static void tails_circle_place(struct task_work *work, struct player *record)
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

/* how far one step of his push turns him round what he holds: less the
 * further it reaches, and only between frames 4 and 28 of his action,
 * easing in and out */
static int tails_circle_step(struct player *record)
{
    struct task_motion *held = record->held->motion;
    Float frame = record->anim.frame;
    int step;

    if (held == NULL || CARRY_REACH(held) == 0.0F) {
        step = 0x100;
    } else {
        step = (int)(65536.0F / (CARRY_REACH(held) * 10.47197F));
    }
    if (frame >= 28.0F || frame <= 4.0F) {
        step = 0;
    } else if (frame >= 18.0F) {
        step = (int)((28.0F - frame) * step * 0.1F);
    } else if (frame <= 14.0F) {
        step = (int)((frame - 4.0F) * step * 0.1F);
    }
    return step;
}

/* pressing the action button curls him up to roll. it returns TRUE when
 * he rolls, and FALSE otherwise */
static Bool tails_try_roll(struct task_work *work, struct player *record)
{
    if (pad_records[PLAYER_VARS(work)->player].press & pad_action_buttons) {
        work->mode = TAILS_ROLL;
        record->anim.action = ACTION_ROLL;
        work->flags |= PLAYER_ATTACKING;
        record->action_speed = record->speed.x;
        return TRUE;
    }
    return FALSE;
}

/* pressing the action button starts his tail attack, and his trails start
 * afresh. it returns TRUE when he attacks, and FALSE otherwise */
static Bool tails_try_tail_attack(struct task_work *work, struct player *record)
{
    NJS_OBJECT *object;

    if (pad_records[PLAYER_VARS(work)->player].press & pad_action_buttons) {
        work->mode = TAILS_TAIL_ATTACK;
        record->anim.action = ACTION_ATTACK;
        TAILS_STATE(work) |= TAILS_ATTACKING;
        record->own.tails.tips.coord[0] = 0.0F;
        record->own.tails.tips.coord[1] = 0.0F;
        record->own.tails.tips.coord[2] = 0.0F;
        record->own.tails.tips.coord[3] = 0.0F;
        record->own.tails.tips.coord[4] = 0.0F;
        record->own.tails.tips.coord[5] = 0.0F;
        /* @hack spelling: both tails through one pointer, which R6 holds for
         * each in turn (MOV.L @(0x8c0c5450),R6 at 0x8c0c53ec); written to
         * directly, 19 of 51 words differ */
        object = tails_fly_tail0;
        object->ang[0] = 0;
        object = tails_fly_tail1;
        object->ang[0] = 0;
        tails_sound(work, SE_TAIL_ATTACK);
        return TRUE;
    }
    return FALSE;
}

/* speed him up or down as he flies. near the ground, he slows his fall.
 * holding the jump button, he climbs until his flight is spent, then tires
 * if he falls fast. holding the action button, he dives as fast as he can.
 * with neither, he drifts down. his flight's phase steps half as fast once
 * FLIGHT_TIRING is spent, faster as he climbs and slower as he dives */
static void tails_fly(struct task_work *work, struct task_motion *motion,
                      struct player *record)
{
    struct height_query probe;
    Float height;
    Float spent;
    Sint8 step;

    probe.pos = work->pos;
    polygon_find_near_heights(&probe, 20.0F);
    if (probe.below.found == TRUE) {
        height = work->pos.y - probe.below.level;
        if (record->speed.y < -3.0F && height >= 25.0F && height < 50.0F) {
            motion->accel.y += 0.25F * (-3.0F - record->speed.y);
        } else if (record->speed.y < -0.3F && height > 0.0F && height < 25.0F) {
            motion->accel.y += 0.25F * (-0.3F - record->speed.y);
        }
    }

    TAILS_CLIMB(work) = 0;
    if (pad_records[PLAYER_VARS(work)->player].on & pad_jump_buttons) {
        spent = record->own.tails.flight + (1.0F / FLIGHT_STEPS);
        if (spent > 1.0F) {
            spent = 1.0F;
        }
        record->own.tails.flight = spent;
        if (spent < 1.0F) {
            TAILS_CLIMB(work) = 1;
            if (record->speed.y < 0.9F) {
                motion->accel.y += 0.1F;
            } else {
                motion->accel.y += 0.99999999F - record->speed.y;
            }
        } else if (record->speed.y < 0.0F) {
            if (record->speed.y > -2.65F) {
                motion->accel.y -= record->speed.y * 0.02F;
            } else {
                work->mode = TAILS_TIRED;
                record->anim.action = ACTION_TIRED;
                work->flags &= ~PLAYER_GUARD;
                TAILS_STATE(work) &= ~TAILS_FLYING;
                tails_sound(work, SE_TIRED);
            }
        }
    } else if (pad_records[PLAYER_VARS(work)->player].on & pad_action_buttons) {
        if (record->speed.y > -record->consts.fall_limit) {
            motion->accel.y += -0.1F;
        } else {
            motion->accel.y +=
                -record->consts.fall_limit - record->speed.y - 0.1F;
        }
        TAILS_CLIMB(work) = -1;
    } else if (record->speed.y < -2.0F) {
        motion->accel.y += 0.1F * (-2.0F - record->speed.y);
    }

    player_accel_fly(work, motion, record);
    if (record->own.tails.flight < FLIGHT_TIRING) {
        step = 8;
    } else {
        step = 4;
    }
    step += step * TAILS_CLIMB(work) / 2;
    if ((TAILS_PHASE(work) += step) >= 64) {
        TAILS_PHASE(work) = 0;
    }
}

/* his flying tails spin TAIL_SPIN_STEP a step, twice that while
 * frame_step_count is 2, and half as far once FLIGHT_TIRING is spent,
 * though with frame_step_count 1 that half step is under the 0x800 grain
 * their angle keeps, and they stop. they stretch as he dives, and squash
 * back otherwise */
static void tails_spin_tails(struct task_work *work, struct player *record)
{
    NJS_OBJECT *object;
    int step;
    int angle;
    Float x;
    Float y;

    /* @hack spelling: both tails through one pointer, which R14 holds for each
     * in turn (MOV.L @(0x8c0c57c8),R14 at 0x8c0c5766); written to directly,
     * 80 of 85 words differ */
    object = tails_fly_tail0;
    angle = object->ang[0];
    if (frame_step_count == 2) {
        step = TAIL_SPIN_STEP * 2;
    } else {
        step = TAIL_SPIN_STEP;
    }
    if (record->own.tails.flight < FLIGHT_TIRING) {
        angle += step;
    } else {
        /* @hack type: the halved step narrowed to a Uint16, which SHC
         * extends with EXTU.W R7,R5 (0x8c0c5788); without it, 73 of 85
         * words differ */
        step = (Uint16)(step >> 1);
        angle += step;
    }
    angle &= 0xf800;
    object->ang[0] = angle;
    object = tails_fly_tail1;
    object->ang[0] = angle & 0xf800;
    object = tails_fly_tail0;
    if (TAILS_CLIMB(work) == -1) {
        object->scl[0] += 0.02F;
        object->scl[1] -= 0.02F;
    } else {
        object->scl[0] -= 0.02F;
        object->scl[1] += 0.02F;
    }
    if (object->scl[0] < 1.0F) {
        object->scl[0] = 1.0F;
        object->scl[1] = 1.0F;
    }
    if (object->scl[0] > 1.3F) {
        object->scl[0] = 1.3F;
        object->scl[1] = 0.7F;
    }
    x = object->scl[0];
    y = object->scl[1];
    object->scl[2] = x;
    object = tails_fly_tail1;
    object->scl[0] = x;
    object->scl[1] = y;
    object->scl[2] = x;
}

/* his tails wave as he stands or runs. both waving copies bend from one
 * shape by the phase, tail 0's ahead of tail 1's by TAIL_LEAD, and their
 * normals follow. each call moves the phase on by one of four paces, from
 * 0x500 to 0x800, which the phase's bits 16 and 17 pick */
static void tails_wave_tails(void)
{
    NJS_MODEL *model;
    Float blend;
    NJS_POINT3 *source_points;
    int point_count;
    NJS_POINT3 *points;

    /* @hack spelling: the copy's model read between the shape's count and its
     * points, which loads it into R12 ahead of them (MOV.L @(0x8c0c5894),R12
     * at 0x8c0c584e); read after them, 2 of 83 words differ */
    point_count = tails_tail_wave_shape[0].nbPoint;
    model = tails_tail1_wave_model;
    source_points = tails_tail_wave_shape[0].points;
    blend = 0.4F;
    points = model->points;
    switch (tails_wave_phase & 0x30000) {
    case 0:
        tails_wave_phase += 0x700;
        tails_wave_blend = blend;
        break;
    case 0x10000:
        tails_wave_phase += 0x500;
        tails_wave_blend = blend;
        break;
    case 0x20000:
        tails_wave_phase += 0x800;
        tails_wave_blend = blend;
        break;
    case 0x30000:
        tails_wave_phase += 0x600;
        tails_wave_blend = blend;
        break;
    }
    tails_bend_points(tails_wave_blend, source_points, points,
                      tails_wave_phase & 0xff00, point_count);
    model_calc_face_normals(model);
    model_calc_point_normals(model);
    point_count = tails_tail_wave_shape[0].nbPoint;
    source_points = tails_tail_wave_shape[0].points;
    model = tails_tail0_wave_model;
    points = model->points;
    tails_bend_points(tails_wave_blend, source_points, points,
                      (tails_wave_phase + TAIL_LEAD) & 0xff00, point_count);
    model_calc_face_normals(model);
    model_calc_point_normals(model);
    tails_tail1_end[0].model = tails_tail1_wave_model;
    tails_tail0_end[0].model = tails_tail0_wave_model;
}

/* put both his tails at rest, their ends in their own models */
static void tails_reset_tails(void)
{
    tails_tail0[0].ang[0] = 0;
    tails_tail1[0].ang[0] = 0;
    tails_tail1_end[0].model = &tails_tail1_model;
    tails_tail0_end[0].model = &tails_tail0_model;
}

/* bend a tail's points from source into destination as it waves. the
 * phase puts a turning point within the tail's first 8 units along x. the
 * points before it curl one way, and the points past it the other, as far
 * as rate says. in the second half of the phase, the two curls trade
 * sides. every point then stretches along x by 1.15 */
static void tails_bend_points(Float rate, NJS_POINT3 *source,
                              NJS_POINT3 *destination, int phase, int count)
{
    /* @hack spelling: the bend's start in origin, a local 0.0F, which SHC
     * keeps at @R15 (FMOV.S FR3,@R15 at 0x8c0c59a6); as the constant, the
     * frame is 8 bytes smaller (ADD #-56,R15 for ADD #-64,R15 at
     * 0x8c0c5998) and 311 of 329 words differ */
    Float origin = 0.0F;
    /* @hack type: i a Uint32, whose loop SHC tests with CMP/HS (0x8c0c5a2a);
     * as an int, 43 of 329 words differ */
    Uint32 i;
    int angle;
    Float position;
    Float first_range;
    Float second_range;
    Float radius;
    /* @hack spelling: blend, a copy of rate declared after position and
     * radius, as the source of their products (FMUL FR4,FR2 at 0x8c0c59da);
     * through rate, FMUL FR7,FR2, and 85 of 329 words differ */
    Float blend;
    Float base_x;
    Float y_offset;
    Float cosine;
    Float half_sine;
    Float second_half_sine;
    Float sine;

    blend = rate;
    if (phase < 32768) {
        position = (Float)phase * (8.0F / 32768.0F);
    } else {
        position = (Float)(phase - 32768) * (8.0F / 32768.0F);
    }
    radius = 8.0F - position;
    first_range = (position * blend * 1.570796F) + ((1.0F - blend) * position);
    second_range = (radius * blend * 1.570796F) + ((1.0F - blend) * radius);
    for (i = 0; i < (Uint32)count; i++) {
        if (source[i].x < 0.0F) {
            base_x = source[i].x;
            y_offset = 0.0F;
        } else if (source[i].x < origin + first_range) {
            if (first_range != 0.0F) {
                angle =
                    (int)(((source[i].x - origin) / first_range) * 32768.0F);
                sine = njSin(angle);
                cosine = njCos(angle);
                y_offset = position * blend * sine * 0.5F;
                base_x = origin + (0.5F * ((1.0F - cosine) * position));
                angle = (angle + 0x4000) >> 1;
                half_sine = njSin(angle);
                cosine = njCos(angle);
                if (phase < 32768) {
                    base_x -= source[i].y * cosine * half_sine * 2.0F;
                } else {
                    base_x += source[i].y * cosine * half_sine * 2.0F;
                    y_offset = -y_offset;
                }
                y_offset -= source[i].y * cosine * cosine * 2.0F;
            } else {
                base_x = source[i].x;
                y_offset = 0.0F;
            }
        } else {
            if (second_range != 0.0F) {
                angle = (int)(((source[i].x - first_range - origin) /
                               second_range) *
                              32768.0F);
                angle = 65536 - angle;
                sine = njSin(angle);
                cosine = njCos(angle);
                y_offset = radius * blend * sine * 0.5F;
                base_x =
                    origin + (0.5F * ((1.0F - cosine) * radius)) + position;
                angle = (0x4000 - angle) >> 1;
                second_half_sine = njSin(angle);
                cosine = njCos(angle);
                if (phase < 32768) {
                    base_x += source[i].y * cosine * second_half_sine * 2.0F;
                } else {
                    base_x -= source[i].y * cosine * second_half_sine * 2.0F;
                    y_offset = -y_offset;
                }
                y_offset -= source[i].y * cosine * cosine * 2.0F;
            } else {
                base_x = source[i].x - first_range + position;
                y_offset = 0.0F;
            }
        }
        destination[i].x = base_x * 1.15F;
        destination[i].y = source[i].y + y_offset;
        destination[i].z = source[i].z;
    }
}

/* make a task that turns the nodes of one of his models toward what he
 * looks at, with a player_look_motion as its motion. it returns FALSE when
 * the task or its motion can't be allocated, and otherwise 96, which a
 * test takes as TRUE */
static Bool tails_look_create(Sint8 player, struct tails_look_nodes *nodes)
{
    struct task *task;
    struct player_look_motion *look;

    /* @hack spelling: the failure's return in an else, which puts it after the
     * stores, where the made task's BRA branches over it, the last store in
     * its delay slot (0x8c0c5d1a); as an early return, 16 of 46 words
     * differ */
    if ((task = task_create(TASK_WORK, 2, tails_look_step)) != NULL) {
        if ((look = heap_alloc(sizeof(*look))) == NULL) {
            task_kill(task);
            return FALSE;
        }
        task->motion = (struct task_motion *)look;
        PLAYER_VARS(task->work)->player = player;
        look->look.head = nodes->head;
        look->look.eye0 = nodes->eye0;
        look->look.eye1 = nodes->eye1;
    } else {
        return FALSE;
    }
    /* @hack spelling: once the task is made, R0 keeps 96 from MOV #96,R0 at
     * 0x8c0c5d16. its one caller drops the result, so with return TRUE, 7 of 46
     * words differ */
#ifdef GCC
    /* repairs the return the tag names: GCC's is TRUE, for the task made */
    return TRUE;
#endif
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}

static Bool tails_fly_look_create(Sint8 player, struct tails_look_nodes *nodes)
{
    struct task *task;
    struct player_look_motion *look;

    /* @hack spelling: the failure's return in an else, as in tails_look_create
     * (0x8c0c5d98); as an early return, 11 of 46 words differ */
    if ((task = task_create(TASK_WORK, 2, tails_fly_look_step)) != NULL) {
        if ((look = heap_alloc(sizeof(*look))) == NULL) {
            task_kill(task);
            return FALSE;
        }
        task->motion = (struct task_motion *)look;
        PLAYER_VARS(task->work)->player = player;
        look->look.head = nodes->head;
        look->look.eye0 = nodes->eye0;
        look->look.eye1 = nodes->eye1;
    } else {
        return FALSE;
    }
    /* @hack spelling: once the task is made, R0 keeps 96 from MOV #96,R0 at
     * 0x8c0c5d94. its one caller drops the result, so with return TRUE, 7 of 46
     * words differ */
#ifdef GCC
    /* repairs the return the tag names: GCC's is TRUE, for the task made */
    return TRUE;
#endif
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}

/* his head and eyes back as the model had them */
static void tails_look_destroy(struct task *task)
{
    struct player_look_motion *look = PLAYER_LOOK(task);

    look->look.head->ang[0] = look->head_ang[0];
    look->look.head->ang[1] = look->head_ang[1];
    look->look.head->ang[2] = look->head_ang[2];
    look->look.eye1->pos[0] = look->eye1.pos[0];
    look->look.eye1->pos[1] = look->eye1.pos[1];
    look->look.eye1->pos[2] = look->eye1.pos[2];
    look->look.eye0->pos[0] = look->eye0.pos[0];
    look->look.eye0->pos[1] = look->eye0.pos[1];
    look->look.eye0->pos[2] = look->eye0.pos[2];
}

/* turn his head and eyes toward the heading he turns to, at the camera
 * while PDD_DGT_TZ is held, or where an event has him look. the task ends
 * once his player is gone */
static void tails_look_step(struct task *task)
{
    struct task_work *work = task->work;
    struct player_look_motion *look = PLAYER_LOOK(task);
    struct task_work *owner;
    struct task_motion *owner_motion;
    struct event_work *event;
    struct collision_shape *shape;
    struct task_work *camera;
    struct task_work *target;
    Angle angles[3];
    NJS_VECTOR direction;
    NJS_VECTOR facing;

    owner = player_work[PLAYER_VARS(work)->player];
    owner_motion = player_motion[PLAYER_VARS(work)->player];
    if (owner == NULL) {
        work->mode = LOOK_END;
    } else if (work->mode == LOOK_INIT) {
        /* @hack spelling: an empty arm for LOOK_INIT, which gives LOOK_END's
         * store an arm of its own (MOV.B R3,@R5 at 0x8c0c5e7e); with the
         * tests below nested under mode != LOOK_INIT, 14 of 396 words
         * differ */
    } else if (owner->event->look) {
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
        look->eye0.pos[0] = look->look.eye0->pos[0];
        look->eye0.pos[1] = look->look.eye0->pos[1];
        look->eye0.pos[2] = look->look.eye0->pos[2];
        look->eye1.pos[0] = look->look.eye1->pos[0];
        look->eye1.pos[1] = look->look.eye1->pos[1];
        look->eye1.pos[2] = look->look.eye1->pos[2];
        look->look.head_max_y = 0x1800;
        look->look.head_min_y = -0x1800;
        look->look.head_max_z = 0x400;
        look->look.head_min_z = -0x1800;
        look->look.eye0_max_y = 0x200;
        look->look.eye0_min_y = -0x1000;
        look->look.eye1_max_y = 0x1000;
        look->look.eye1_min_y = -0x200;
        look->look.eye0_max_z = 0x400;
        look->look.eye0_min_z = -0x800;
        look->look.eyes_max_z = 0x400;
        look->look.eyes_min_z = -0x800;
        task->destructor = tails_look_destroy;
        break;
    case LOOK_AHEAD:
        look->look.head_aim_y =
            -angle_difference(owner->ang.y, owner_motion->aim.y);
        look->look.head_aim_z = 0;
        player_look_turn_head(&look->look);
        look->look.eyes_aim_y = 0;
        look->look.eyes_aim_z = 0;
        player_look_turn_eyes(&look->look);
        tails_look_move_eyes(look);
        break;
    case LOOK_CAMERA:
        camera = camera_work;
        shape = owner->collision->shapes;
        /* the camera's angles from his middle, against the way he faces */
        facing.y = facing.z = 0.0F;
        facing.x = 1.0F;
        player_turn_to_world(owner, &facing);
        direction.x = camera->pos.x - shape->center.x;
        direction.y = camera->pos.y - shape->center.y;
        direction.z = camera->pos.z - shape->center.z;
        vector_get_angles(&direction, &facing, angles);
        look->look.head_aim_y = -angles[1];
        look->look.head_aim_z = angles[2];
        player_look_turn_head(&look->look);
        look->look.eyes_aim_y = angles[1] + look->look.head_y;
        look->look.eyes_aim_z = angles[2] - look->look.head_z;
        player_look_turn_eyes(&look->look);
        tails_look_move_eyes(look);
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
            direction.x = event->look_pos.x - shape->center.x;
            direction.y = event->look_pos.y - shape->center.y;
            direction.z = event->look_pos.z - shape->center.z;
            vector_get_angles(&direction, &facing, angles);
            break;
        case EVENT_LOOK_TASK:
            target = event->look_task->work;
#ifdef GCC
            /* repairs the unset angles the tag below names: 0, as the
             * default case sets them, looks ahead */
            angles[1] = 0;
            angles[2] = 0;
#endif
            /* @bug uninitialized: when the event's task has no work,
             * angles[1] and [2] stay unset. the BT at 0x8c0c6146 skips to
             * the aims, which read them at 0x8c0c61b8 and 0x8c0c61c2 */
            if (target != NULL) {
                facing.y = facing.z = 0.0F;
                facing.x = 1.0F;
                player_turn_to_world(owner, &facing);
                direction.x =
                    target->pos.x + event->look_pos.x - shape->center.x;
                direction.y =
                    target->pos.y + event->look_pos.y - shape->center.y;
                direction.z =
                    target->pos.z + event->look_pos.z - shape->center.z;
                vector_get_angles(&direction, &facing, angles);
            }
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
        tails_look_move_eyes(look);
        break;
    default:
        task_kill(task);
        break;
    }
}

/* on his flying model, his head tips against his climb or fall. his eyes
 * look ahead, or at the camera while PDD_DGT_TZ is held. each eye keeps
 * within the limits tails_look_step gives his other model's eyes */
static void tails_fly_look_step(struct task *task)
{
    struct task_work *work = task->work;
    struct player_look_motion *look = PLAYER_LOOK(task);
    struct task_work *owner;
    struct task_motion *owner_motion;
    struct player *record;
    Angle ang;
    Float across;
    NJS_VECTOR to;
    NJS_VECTOR front;

    if (player_work[PLAYER_VARS(work)->player] == NULL) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case LOOK_INIT:
        look->head_ang[0] = look->look.head->ang[0];
        look->head_ang[1] = look->look.head->ang[1];
        look->head_ang[2] = look->look.head->ang[2];
        look->eye0.pos[0] = look->look.eye0->pos[0];
        look->eye0.pos[1] = look->look.eye0->pos[1];
        look->eye0.pos[2] = look->look.eye0->pos[2];
        look->eye1.pos[0] = look->look.eye1->pos[0];
        look->eye1.pos[1] = look->look.eye1->pos[1];
        look->eye1.pos[2] = look->look.eye1->pos[2];
        task->destructor = tails_look_destroy;
        work->mode = LOOK_AHEAD;
        break;
    case LOOK_AHEAD:
        owner = player_work[PLAYER_VARS(work)->player];
        owner_motion = player_motion[PLAYER_VARS(work)->player];
        /* @hack spelling: a return where a break would end the function too,
         * here and in LOOK_CAMERA, which gives the store of LOOK_CAMERA a
         * tail of its own (0x8c0c627c); with break, it takes LOOK_INIT's
         * tail and fills a branch's delay slot, and 33 of 528 words
         * differ */
        if (owner == NULL) {
            work->mode = LOOK_CAMERA;
            return;
        }
        if (owner_motion->speed.y > 0.0F) {
            ang = -0x1000;
        } else if (owner_motion->speed.y < 0.0F) {
            ang = 0x400;
        } else {
            ang = 0;
        }
        /* @hack type: ang narrowed to a Uint16 in a statement of its own,
         * which keeps it in R4 (0x8c0c6250); angle_approach narrows it
         * anyway, and without it, 145 of 528 words differ */
        ang = (Uint16)ang;
        work->ang.z = angle_approach(work->ang.z, ang, 0x400);
        look->look.head->ang[2] = work->ang.z;
        look->look.eyes_aim_y = 0;
        look->look.eyes_aim_z = 0;
        ang = angle_approach(look->look.eye0_y, look->look.eyes_aim_y, 0x800);
        if (ang & 0x8000) {
            ang |= 0xffff0000;
        }
        if (ang > 0x200) {
            ang = 0x200;
        } else if (ang < -0x1000) {
            ang = -0x1000;
        }
        look->look.eye0_y = ang;
        ang = angle_approach(look->look.eye1_y, look->look.eyes_aim_y, 0x800);
        if (ang & 0x8000) {
            ang |= 0xffff0000;
        }
        if (ang < -0x200) {
            ang = -0x200;
        } else if (ang > 0x1000) {
            ang = 0x1000;
        }
        look->look.eye1_y = ang;
        ang = angle_approach(look->look.eye0_z, look->look.eyes_aim_z, 0x800);
        if (ang & 0x8000) {
            ang |= 0xffff0000;
        }
        ang >>= 1;
        if (ang < -0x800) {
            ang = -0x800;
        } else if (ang > 0x400) {
            ang = 0x400;
        }
        look->look.eye0_z = ang;
        look->look.eye1_z = ang;
        tails_look_move_eyes(look);
        if (pad_records[PLAYER_VARS(work)->player].on & PDD_DGT_TZ) {
            work->mode = LOOK_CAMERA;
        }
        break;
    case LOOK_CAMERA:
        owner = player_work[PLAYER_VARS(work)->player];
        owner_motion = player_motion[PLAYER_VARS(work)->player];
        record = owner_motion->record;
        if (owner == NULL) {
            work->mode = LOOK_CAMERA;
            return;
        }
        if (owner_motion->speed.y > 1.0F) {
            ang = -0x1000;
        } else if (owner_motion->speed.y < -1.0F) {
            ang = 0x1000;
        } else {
            ang = 0;
        }
        /* @hack type: ang narrowed again, as in LOOK_AHEAD; without it,
         * 213 of 528 words differ */
        ang = (Uint16)ang;
        work->ang.z = angle_approach(work->ang.z, ang, 0x400);
        look->look.head->ang[2] = work->ang.z;
        /* the camera, from his eyes, against the way he faces */
        to.x = to.z = 0.0F;
        to.y = record->consts.eye_height;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, owner->ang.z);
        TURN(njRotateX, owner->ang.x);
        TURN(njRotateY, -owner->ang.y);
        njCalcVector(NULL, &to, &front);
        to.x = camera_work->pos.x - owner->pos.x - front.x;
        to.y = camera_work->pos.y - owner->pos.y - front.y;
        to.z = camera_work->pos.z - owner->pos.z - front.z;
        across = njSqrt((to.x * to.x) + (to.z * to.z));
        look->look.eyes_aim_y = (Angle)(atan2f(to.x, -to.z) * ANGLE_PER_RADIAN);
        look->look.eyes_aim_z =
            (Angle)(atan2f(-to.y, across) * ANGLE_PER_RADIAN);
        front.y = front.z = 0.0F;
        front.x = 1.0F;
        njCalcVector(NULL, &front, &to);
        njPopMatrix(1);
        across = njSqrt((to.x * to.x) + (to.z * to.z));
        /* @hack spelling: the eyes' aims updated written out, here and in
         * the two stores below, which keeps -0x1000 in R8 and 0x400 in R10
         * (MOV R8,R4 at 0x8c0c6358); as -= and +=, 21 of 528 words
         * differ */
        look->look.eyes_aim_y = look->look.eyes_aim_y -
                                (Angle)(atan2f(to.x, -to.z) * ANGLE_PER_RADIAN);
        look->look.eyes_aim_z =
            look->look.eyes_aim_z -
            (Angle)(atan2f(-to.y, across) * ANGLE_PER_RADIAN);
        look->look.eyes_aim_z = look->look.eyes_aim_z + work->ang.z;
        ang = angle_approach(look->look.eye0_y, look->look.eyes_aim_y, 0x800);
        if (ang & 0x8000) {
            ang |= 0xffff0000;
        }
        if (ang > 0x200) {
            ang = 0x200;
        } else if (ang < -0x1000) {
            ang = -0x1000;
        }
        look->look.eye0_y = ang;
        ang = angle_approach(look->look.eye1_y, look->look.eyes_aim_y, 0x800);
        if (ang & 0x8000) {
            ang |= 0xffff0000;
        }
        if (ang < -0x200) {
            ang = -0x200;
        } else if (ang > 0x1000) {
            ang = 0x1000;
        }
        look->look.eye1_y = ang;
        ang = angle_approach(look->look.eye0_z, look->look.eyes_aim_z, 0x800);
        if (ang & 0x8000) {
            ang |= 0xffff0000;
        }
        /* @hack spelling: the shift inside the test, which keeps njSqrt's
         * copies at the image's frame slots (MOV #24,R0 at 0x8c0c65ba);
         * shifted in a statement before it, they move and 6 of 528 words
         * differ */
        if ((ang >>= 1) < -0x800) {
            ang = -0x800;
        } else if (ang > 0x400) {
            ang = 0x400;
        }
        look->look.eye1_z = look->look.eye0_z = ang;
        tails_look_move_eyes(look);
        if (!(pad_records[PLAYER_VARS(work)->player].on & PDD_DGT_TZ)) {
            work->mode = LOOK_AHEAD;
        }
        break;
    default:
        task_kill(task);
        break;
    }
}

/* each eye slides over his face, to where the line its angles aim along
 * meets the plane it rests on */
static void tails_look_move_eyes(struct player_look_motion *look)
{
    /* @hack spelling: the locals declared line, plane, point, which puts point
     * at R15+4, plane at R15+16 and line at R15+40 (MOV #16,R0 at
     * 0x8c0c6838); declared point, plane, line, 36 of 191 words differ */
    union line_view line;
    union plane_view plane;
    NJS_POINT3 point;

    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, look->look.eye0_z);
    TURN(njRotateY, look->look.eye0_y);
    plane.plane.px = look->eye0.pos[0];
    plane.plane.py = look->eye0.pos[1];
    plane.plane.pz = look->eye0.pos[2];
    plane.plane.vx = 0.932285F;
    plane.plane.vy = 0.061793F;
    plane.plane.vz = 0.356407F;
    line.line.px = 0.0F;
    line.line.py = 0.0F;
    line.line.pz = 0.0F;
    njCalcVector(NULL, &plane.split.point, &line.split.direction);
    vector_normalize(&line.split.direction);
    /* @hack type: the unions' own addresses as the NJS_LINE and NJS_PLANE,
     * which SHC forms from R15 at each use. as &line.line and &plane.plane
     * here too, it keeps them in saved registers, the function runs 44 bytes
     * short and 188 of 191 words differ */
    njDistanceL2PL((NJS_LINE *)&line, (NJS_PLANE *)&plane, &point);
    look->look.eye0->pos[0] = point.x;
    look->look.eye0->pos[1] = point.y;
    look->look.eye0->pos[2] = point.z;

    njUnitMatrix(NULL);
    TURN(njRotateZ, look->look.eye1_z);
    TURN(njRotateY, look->look.eye1_y);
    plane.plane.px = look->eye1.pos[0];
    plane.plane.py = look->eye1.pos[1];
    plane.plane.pz = look->eye1.pos[2];
    plane.plane.vx = 0.932285F;
    plane.plane.vy = 0.061793F;
    plane.plane.vz = -0.356407F;
    line.line.px = 0.0F;
    line.line.py = 0.0F;
    line.line.pz = 0.0F;
    njCalcVector(NULL, &plane.split.point, &line.split.direction);
    vector_normalize(&line.split.direction);
    njDistanceL2PL(&line.line, &plane.plane, &point);
    look->look.eye1->pos[0] = point.x;
    look->look.eye1->pos[1] = point.y;
    look->look.eye1->pos[2] = point.z;
    njPopMatrix(1);
}

/* chain his tail attack's blows. before a blow's last 10 frames, it sets
 * the next field of the blow's tails_actions row to ACTION_STAND. in the
 * last 10, with the rhythm brooch and the action button held, it sets
 * another blow there. with the stick pushed, that's the blow the stick
 * points along, or a still blow when he's in that one. with the stick
 * released, it's a still blow: the other still blow when he's in one */
static void tails_attack_next(struct task_work *work, struct player *record)
{
    struct player_action *entry;
    Float end;
    int next;

    if (record->anim.action < ACTION_ATTACK ||
        record->anim.action > ACTION_ATTACK_UP_RIGHT) {
        return;
    }
    entry = &tails_actions[record->anim.action];
    end = (Float)entry->action->motion->nbFrame;
    end += -10.0F;
    if (record->anim.frame > end) {
        if ((pad_records[PLAYER_VARS(work)->player].on & pad_action_buttons) ==
                0 ||
            (record->upgrades & PLAYER_TAILS_RHYTHM_BROOCH) == 0) {
            return;
        }
        if (player_read_stick(work, NULL, NULL)) {
            PDS_PERIPHERAL *pad;

#ifdef GCC
            /* sets pad first, as SHC evaluates the shipped arguments right to
             * left */
            pad = &pad_records[PLAYER_VARS(work)->player];
            next =
                ((((int)(atan2f((Float)(pad->y1 << 8), (Float)(pad->x1 << 8)) *
                         ANGLE_PER_RADIAN) +
                   4096) >>
                  13) &
                 7) +
                ACTION_ATTACK_RIGHT;
#else
            /* @hack implicit: the second argument sets pad and the first reads
             * it, unsequenced, which SHC evaluates right to left, keeping pad
             * in a stack slot between the reads (MOV.L R0,@R15 at 0x8c0c6aa2,
             * MOV.L @R15,R3 at 0x8c0c6aa6). set in a statement of its own, pad
             * takes R11 and 69 of 172 words differ. set in the statement that
             * takes x1 into a Float ahead of the call, 5 of 172 words differ */
            /* NOLINTBEGIN(clang-diagnostic-uninitialized) */
            /* NOLINTBEGIN(clang-diagnostic-unsequenced) */
            next = ((((int)(atan2f((Float)(pad->y1 << 8),
                                   (Float)((pad = &pad_records[PLAYER_VARS(work)
                                                                   ->player])
                                               ->x1
                                           << 8)) *
                            ANGLE_PER_RADIAN) +
                      4096) >>
                     13) &
                    7) +
                   ACTION_ATTACK_RIGHT;
            /* NOLINTEND(clang-diagnostic-unsequenced) */
            /* NOLINTEND(clang-diagnostic-uninitialized) */
#endif
            if ((Uint8)next == record->anim.action) {
                entry->next = ((Uint8)(int)record->anim.frame & 1) +
                              ACTION_ATTACK_STILL_A;
            } else {
                entry->next = (Uint8)next;
                tails_sound(work, SE_TAIL_ATTACK);
            }
        } else if (record->anim.action == ACTION_ATTACK_STILL_A ||
                   record->anim.action == ACTION_ATTACK_STILL_B) {
            entry->next = record->anim.action ^ 1;
            tails_sound(work, SE_TAIL_ATTACK);
        } else {
            entry->next =
                ((Uint8)(int)record->anim.frame & 1) + ACTION_ATTACK_STILL_A;
        }
    } else {
        entry->next = ACTION_STAND;
    }
}

/* as he attacks, each tail's end leaves two trails, from where it was to
 * where it is, the inner one half a unit higher. both ends are then kept
 * in his record's tips for the next step. no trail starts while any kept
 * coordinate is 0, as all six are once tails_try_tail_attack starts the
 * attack */
static void tails_attack_trails(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record)
{
    NJS_POINT3 end_a;
    NJS_POINT3 end_b;
    NJS_POINT3 offset;
    NJS_POINT3 previous;
    Float lift;
    struct task *task;

    njUnitMatrix(&tails_matrix0);
    njTranslateV(&tails_matrix0, &work->collision->shapes->center);
    TURN_MATRIX(njRotateZ, &tails_matrix0, work->ang.z);
    TURN_MATRIX(njRotateX, &tails_matrix0, work->ang.x);
    TURN_MATRIX(njRotateY, &tails_matrix0, 0x8000 - work->ang.y);
    if (!pose_action(&tails_matrix0, tails_actions[record->anim.action].action,
                     record->anim.frame, TAIL_END_A, &tails_matrix1)) {
        return;
    }
    njSetMatrix(&tails_matrix2, &tails_matrix1);
    if (!pose_get_matrix(TAIL_END_B, &tails_matrix1)) {
        return;
    }
    njSetMatrix(&tails_matrix3, &tails_matrix1);
    offset.x = 7.0F;
    offset.y = 0.0F;
    offset.z = 1.0F;
    njCalcPoint(&tails_matrix2, &offset, &end_a);
    njCalcPoint(&tails_matrix3, &offset, &end_b);

    if (record->own.tails.tips.coord[0] == 0.0F ||
        record->own.tails.tips.coord[1] == 0.0F ||
        record->own.tails.tips.coord[2] == 0.0F ||
        record->own.tails.tips.coord[3] == 0.0F ||
        record->own.tails.tips.coord[4] == 0.0F ||
        record->own.tails.tips.coord[5] == 0.0F) {
        /* @hack spelling: a goto past the trails, after which SHC forms the
         * kept ends' address in R3 (MOV R12,R3 at 0x8c0c6d0c); as an if
         * around the trails, in R2, and 3 of 549 words differ */
        goto keep;
    }
    previous.x = record->own.tails.tips.coord[0];
    previous.y = record->own.tails.tips.coord[1];
    previous.z = record->own.tails.tips.coord[2];
    if ((task = task_create(TASK_WORK | TASK_EXTRA, 6, tails_trail_step)) !=
        NULL) {
        TAILS_TRAIL_WORK(task)->from.x = previous.x;
        TAILS_TRAIL_WORK(task)->from.y = previous.y;
        TAILS_TRAIL_WORK(task)->from.z = previous.z;
        TAILS_TRAIL_WORK(task)->to.x = end_a.x;
        TAILS_TRAIL_WORK(task)->to.y = end_a.y;
        TAILS_TRAIL_WORK(task)->to.z = end_a.z;
        TAILS_TRAIL_WORK(task)->center.x =
            work->collision->shapes->center.x - motion->speed.x;
        TAILS_TRAIL_WORK(task)->center.y =
            work->collision->shapes->center.y - motion->speed.y;
        TAILS_TRAIL_WORK(task)->center.z =
            work->collision->shapes->center.z - motion->speed.z;
        TAILS_TRAIL_CENTER(task)->x = work->collision->shapes->center.x;
        TAILS_TRAIL_CENTER(task)->y = work->collision->shapes->center.y;
        TAILS_TRAIL_CENTER(task)->z = work->collision->shapes->center.z;
    }

    lift = 0.5F;
    if ((task = task_create(TASK_WORK | TASK_EXTRA, 6,
                            tails_trail_inner_step)) != NULL) {
        TAILS_TRAIL_WORK(task)->from.x = previous.x;
        TAILS_TRAIL_WORK(task)->from.y = previous.y + lift;
        TAILS_TRAIL_WORK(task)->from.z = previous.z;
        TAILS_TRAIL_WORK(task)->to.x = end_a.x;
        TAILS_TRAIL_WORK(task)->to.y = end_a.y + lift;
        TAILS_TRAIL_WORK(task)->to.z = end_a.z;
        TAILS_TRAIL_WORK(task)->center.x =
            work->collision->shapes->center.x - motion->speed.x;
        TAILS_TRAIL_WORK(task)->center.y =
            work->collision->shapes->center.y - motion->speed.y + lift;
        TAILS_TRAIL_WORK(task)->center.z =
            work->collision->shapes->center.z - motion->speed.z;
        TAILS_TRAIL_CENTER(task)->x = work->collision->shapes->center.x;
        TAILS_TRAIL_CENTER(task)->y = work->collision->shapes->center.y + lift;
        TAILS_TRAIL_CENTER(task)->z = work->collision->shapes->center.z;
    }

    previous.x = record->own.tails.tips.coord[3];
    previous.y = record->own.tails.tips.coord[4];
    previous.z = record->own.tails.tips.coord[5];
    if ((task = task_create(TASK_WORK | TASK_EXTRA, 6, tails_trail_step)) !=
        NULL) {
        TAILS_TRAIL_WORK(task)->from.x = previous.x;
        TAILS_TRAIL_WORK(task)->from.y = previous.y;
        TAILS_TRAIL_WORK(task)->from.z = previous.z;
        TAILS_TRAIL_WORK(task)->to.x = end_b.x;
        TAILS_TRAIL_WORK(task)->to.y = end_b.y;
        TAILS_TRAIL_WORK(task)->to.z = end_b.z;
        TAILS_TRAIL_WORK(task)->center.x =
            work->collision->shapes->center.x - motion->speed.x;
        TAILS_TRAIL_WORK(task)->center.y =
            work->collision->shapes->center.y - motion->speed.y;
        TAILS_TRAIL_WORK(task)->center.z =
            work->collision->shapes->center.z - motion->speed.z;
        TAILS_TRAIL_CENTER(task)->x = work->collision->shapes->center.x;
        TAILS_TRAIL_CENTER(task)->y = work->collision->shapes->center.y;
        TAILS_TRAIL_CENTER(task)->z = work->collision->shapes->center.z;
    }

    lift = 0.5F;
    if ((task = task_create(TASK_WORK | TASK_EXTRA, 6,
                            tails_trail_inner_step)) != NULL) {
        TAILS_TRAIL_WORK(task)->from.x = previous.x;
        TAILS_TRAIL_WORK(task)->from.y = previous.y + lift;
        TAILS_TRAIL_WORK(task)->from.z = previous.z;
        TAILS_TRAIL_WORK(task)->to.x = end_b.x;
        TAILS_TRAIL_WORK(task)->to.y = end_b.y + lift;
        TAILS_TRAIL_WORK(task)->to.z = end_b.z;
        TAILS_TRAIL_WORK(task)->center.x =
            work->collision->shapes->center.x - motion->speed.x;
        TAILS_TRAIL_WORK(task)->center.y =
            work->collision->shapes->center.y - motion->speed.y + lift;
        TAILS_TRAIL_WORK(task)->center.z =
            work->collision->shapes->center.z - motion->speed.z;
        TAILS_TRAIL_CENTER(task)->x = work->collision->shapes->center.x;
        TAILS_TRAIL_CENTER(task)->y = work->collision->shapes->center.y + lift;
        TAILS_TRAIL_CENTER(task)->z = work->collision->shapes->center.z;
    }

keep:
    record->own.tails.tips.coord[0] = end_a.x;
    record->own.tails.tips.coord[1] = end_a.y;
    record->own.tails.tips.coord[2] = end_a.z;
    record->own.tails.tips.coord[3] = end_b.x;
    record->own.tails.tips.coord[4] = end_b.y;
    record->own.tails.tips.coord[5] = end_b.z;
}

/* set his shapes for what he does. his body's attack and guard levels are
 * 1 while he's curled up, and 0 otherwise. his tails' ends are on only in
 * his tail attack, at the points tails_attack_trails last kept. the disc
 * above him is on only as he flies */
static void tails_set_shapes(struct task_work *work, struct player *record)
{
    int i;

    work->collision->shapes->radius = 4.0F;
    work->collision->shapes->levels =
        work->collision->shapes->levels & ~SHAPE_ATTACK;
    work->collision->shapes->levels =
        work->collision->shapes->levels & ~SHAPE_GUARD;
    for (i = 1; i < 4; i++) {
        work->collision->shapes[i].flags |= SHAPE_OFF;
    }
    if (work->flags & PLAYER_BALL) {
        work->collision->shapes->levels =
            (work->collision->shapes->levels & ~SHAPE_ATTACK) | ATTACK(1);
        work->collision->shapes->levels =
            (work->collision->shapes->levels & ~SHAPE_GUARD) | GUARD(1);
    }
    switch (work->mode) {
    case TAILS_TAIL_ATTACK:
        work->collision->shapes[1].flags &= ~SHAPE_OFF;
        work->collision->shapes[2].flags &= ~SHAPE_OFF;
        work->collision->shapes[1].center = record->own.tails.tips.point[0];
        work->collision->shapes[2].center = record->own.tails.tips.point[1];
        break;
    case TAILS_FLY:
        work->collision->shapes->radius = 2.25F;
        work->collision->shapes[3].flags &= ~SHAPE_OFF;
        work->collision->shapes[3].center.x =
            work->pos.x - (njCos(work->ang.y) * 2.0F);
        work->collision->shapes[3].center.y = work->pos.y + 8.25F;
        work->collision->shapes[3].center.z =
            work->pos.z - (njSin(work->ang.y) * 2.0F);
        break;
    }
}

/* both heads' own models back in their places */
static void tails_sway_destroy(struct task *task)
{
    tails_head[0].model = &tails_head_model;
    tails_fly_head[0].model = &tails_fly_head_model;
}

/* the ends of his ears sway as he moves, in a copy of whichever head his
 * action draws, which takes both heads' places */
static void tails_sway_step(struct task *task)
{
    struct task_work *work = task->work;
    struct player_sway *sway = PLAYER_SWAY(task);
    struct task_motion *owner_motion;
    struct player *record;
    NJS_MODEL *deformed;
    NJS_POINT3 *to;
    NJS_POINT3 *from;
    Uint32 count;
    Uint32 i;
    NJS_VECTOR point;
    Float speed;
    Float tilt;
    Angle step;

    if (player_work[PLAYER_VARS(work)->player] == NULL) {
        task_kill(task);
        return;
    }
    owner_motion = player_motion[PLAYER_VARS(work)->player];
    record = player_get(PLAYER_VARS(work)->player);
    /* @hack spelling: deformed set inside the store, which keeps the task in
     * R9 (MOV R4,R9 at 0x8c0c729a); set in a statement before it, deformed
     * and the task trade R8 and R9 and 38 of 335 words differ */
    to = (deformed = &tails_head_sway_model)->points;
    /* @hack spelling: the first test adds the table to the index where the
     * second indexes it, which loads the record into R4 and the table into
     * R5 (0x8c0c7312, 0x8c0c7316); indexed alike, the two trade and 7 of
     * 335 words differ */
    if ((record->anim.action + tails_actions)->action->object ==
        &tails_object) {
        from = tails_head_model.points;
        count = tails_head_model.nbPoint;
    } else if (tails_actions[record->anim.action].action->object ==
               &tails_fly_object) {
        from = tails_fly_head_model.points;
        count = tails_fly_head_model.nbPoint;
    } else {
        return;
    }
    switch (work->mode) {
    case SWAY_INIT:
        work->mode = SWAY_SWAY;
        task->destructor = tails_sway_destroy;
        break;
    case SWAY_SWAY:
        speed = njSqrt(VECTOR_SQUARED(&owner_motion->speed)) * 2.0F;
        if (speed > _builtin_fabsf(sway->bob)) {
            speed = speed < 0.0F ? -speed : speed;
            if (speed > 4.0F) {
                speed = 4.0F;
            }
            sway->bob = speed;
        }
        /* @hack spelling: the bob's decay written out, which keeps the
         * owner's motion in a frame slot (MOV.L R2,@R15 at 0x8c0c7310); as
         * *=, it takes R8 and 156 of 335 words differ */
        if (sway->bob >= 0.2F || sway->bob <= -0.2F) {
            sway->bob = sway->bob * 0.99F;
        } else {
            sway->bob = 0.0F;
        }
        step = (Angle)_builtin_fabsf(sway->bob * 512.0F);
        /* @hack spelling: step raised inside its test, which puts 0xc00 ahead
         * of 0x800 in the pool (0x8c0c7560); raised before it, the two swap
         * and 2 of 335 words differ */
        if ((step = step + 0x800) > 0xc00) {
            step = 0xc00;
        }
        sway->bob_phase += step;
        /* @hack spelling: the tilt's product in three statements, which keep
         * njSqrt's copies at frame slots 8 and 12 (MOV #8,R0 at
         * 0x8c0c73c8); as one, the product takes FR0, the copies trade and
         * 7 of 335 words differ */
        tilt = njCos(sway->bob_phase);
        tilt = tilt * sway->bob;
        tilt = tilt * 819.2F;
        work->ang.z = (Angle)tilt;
        njUnitMatrix(&tails_matrix0);
        TURN_MATRIX(njRotateZ, &tails_matrix0, work->ang.z);
        njUnitMatrix(&tails_matrix1);
        TURN_MATRIX(njRotateZ, &tails_matrix1, work->ang.z >> 1);
        for (i = 0; i < count; i++) {
            if (_builtin_fabsf(from->z) >= 1.7F) {
                if (from->y < -3.0F) {
                    point.x = from->x + 0.3F;
                    point.y = from->y + 1.2F;
                    point.z = from->z;
                    njCalcVector(&tails_matrix0, &point, to);
                    to->x -= 0.3F;
                    to->y -= 1.2F;
                } else {
                    point.x = from->x + 2.5F;
                    point.y = from->y;
                    point.z = from->z;
                    njCalcVector(&tails_matrix0, &point, to);
                    to->x -= 2.5F;
                }
            } else if (from->x > 2.2F) {
                point.x = from->x + 0.25F;
                point.y = from->y;
                point.z = from->z;
                njCalcVector(&tails_matrix1, &point, to);
                to->x -= 0.25F;
            }
            from++;
            to++;
        }
        break;
    default:
        task_destroy(task);
        break;
    }
    tails_head[0].model = deformed;
    tails_fly_head[0].model = deformed;
}

/* the task that keeps his mouth: its record, then the mouth's own step */
static void tails_mouth_start(struct task *task)
{
    struct player_mouth *mouth;

    if ((mouth = heap_calloc(1, sizeof(*mouth))) != NULL) {
        task->motion = (struct task_motion *)mouth;
        mouth->blend = &tails_head_blend;
        mouth->shapes = &tails_mouth_shapes;
        task->destructor = player_mouth_destroy;
        task->step = tails_mouth_step;
    }
}

/* his mouth moves on his flying model's head while TAILS_FLYING_HEAD is
 * set, and otherwise on his first model's */
static void tails_mouth_step(struct task *task)
{
    struct player_mouth *mouth = (struct player_mouth *)task->motion;
    struct task_work *owner = player_work[PLAYER_VARS(task->work)->player];

    if (mouth == NULL || owner == NULL) {
        task_kill(task);
        return;
    }
    if (!(TAILS_FLAGS(owner) & TAILS_FLYING_HEAD)) {
        mouth->face = &tails_mouth;
        mouth->mouth = tails_head;
    } else {
        mouth->face = &tails_fly_mouth;
        mouth->mouth = tails_fly_head;
    }
    player_mouth_step(task);
}

void tails_board_start(struct task *task)
{
    struct task_work *work = task->work;

    BOARD_VARS(work)->object = &tails_board_object;
    BOARD_VARS(work)->texlist = &tails_board_texlist;
    task->motion->gravity = 0.02F;
    njLoadTexture(BOARD_VARS(work)->texlist);
    task->destructor = tails_board_destroy;
    task->step = tails_board_step;
    task->draw = tails_board_draw;
}

/* in his board actions, from ACTION_BOARD to ACTION_BOARD_CROUCH, the
 * board goes where he does. otherwise it acts as a small thing set down,
 * its middle at its own height */
static void tails_board_step(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack type: the rider's number read through (Uint8) here and below,
     * which SHC extends with EXTU.B R4,R5 (0x8c0c7754); without the casts,
     * EXTS.B, and 1 of 91 words differ */
    struct task_work *rider = player_work[(Uint8)BOARD_VARS(work)->player];
    struct task_motion *rider_motion =
        player_motion[(Uint8)BOARD_VARS(work)->player];
    struct task_motion *motion = task->motion;
    struct player_anim *anim = &BOARD_PLAYER(work)->anim;
    Sint16 action = anim->current;

    if (rider == NULL) {
        task_kill(task);
        return;
    }
    if (action < ACTION_BOARD || action > ACTION_BOARD_CROUCH) {
        work->pos.y += BOARD_DEPTH;
        carry_step_small(task);
        work->pos.y -= BOARD_DEPTH;
    } else {
        work->pos = rider->collision->shapes->center;
        work->ang.x = rider->ang.x;
        work->ang.y = 0x8000 - rider->ang.y;
        work->ang.z = rider->ang.z;
        motion->speed = rider_motion->speed;
    }
    tails_board_draw(task);
}

/* in his board actions the board plays its own action at his frame, or is
 * drawn as its model has it where it has none. otherwise it's drawn
 * BOARD_DEPTH below its middle */
static void tails_board_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct player_anim *anim = &player_get(BOARD_VARS(work)->player)->anim;
    Sint16 action = anim->current;
    NJS_ACTION *board_action;

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
        /* @hack spelling: action lowered in a statement of its own, which
         * keeps it in a register (0x8c0c783a); lowered in the index, as
         * tails_board_actions[action - ACTION_BOARD], it goes to the frame
         * and 26 of 174 words differ */
        action -= ACTION_BOARD;
        board_action = tails_board_actions[action];
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        if (board_action != NULL) {
            NJS_MOTION *motion = board_action->motion;

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

static void tails_board_destroy(struct task *task)
{
    struct task_work *work = task->work;

    njReleaseTexture(BOARD_VARS(work)->texlist);
}

/* in his races, the camera's path round him starts if he won, and his
 * voice plays later if he lost. a task then shows the result. in each race
 * but Speed Highway's, the stage's capsule, if it has one, decides the race
 * first. he wins unless player 1 stands on its button. anywhere else he
 * wins, and in Sand Hill player 0's height is set to 9 */
void tails_clear(void)
{
    int stage = (Uint8)(stage_get_stage_act() >> 8);

    switch (stage) {
    case STAGE_HIGHWAY:
        if (race_outcome() == RACE_WON) {
            camera_start_path(&sonic_clear_path, CAMERA_PATH_HOLD,
                              CLEAR_PATH_STEPS);
            sound_play_later(SE_CLEAR);
            music_play_later(MUSIC_RNDCLEAR);
        } else {
            tails_voice_later(VOICE_HIGHWAY_LOST);
        }
        task_create(TASK_WORK, 3, race_result_step);
        break;
    case STAGE_WINDY:
    case STAGE_SKY_DECK:
    case STAGE_ICECAP:
    case STAGE_CASINO:
        if (capsule_get() != NULL) {
            if (capsule_get_player() != 1) {
                race_set_won();
            } else {
                race_set_lost();
            }
        }
        if (race_outcome() == RACE_WON) {
            camera_start_path(&sonic_clear_path, CAMERA_PATH_HOLD,
                              CLEAR_PATH_STEPS);
            sound_play_later(SE_CLEAR);
            music_play_later(MUSIC_RNDCLEAR);
        } else {
            tails_voice_later(VOICE_LOST);
        }
        task_create(TASK_WORK, 3, race_result_step);
        break;
    default:
        race_set_won();
        if (stage == STAGE_SUNABOARD) {
            struct player *record = player_get(0);

            record->consts.height = 9.0F;
            record->consts.center_y = record->consts.height * 0.5F;
        }
        camera_start_path(&sonic_clear_path, CAMERA_PATH_HOLD,
                          CLEAR_PATH_STEPS);
        sound_play_later(SE_CLEAR);
        music_play_later(MUSIC_RNDCLEAR);
        break;
    }
}

/* play se, unless he is player 1 */
static void tails_sound(struct task_work *work, int se)
{
    if (PLAYER_VARS(work)->player != 1) {
        sound_play(se, 0, 0, 0);
    }
}

static void tails_push_sound(struct task_work *work)
{
    if (PLAYER_VARS(work)->player != 1 &&
        (player_carry_flags(work) & PLAYER_CAN_PUSH)) {
        sound_play_for(SE_PUSH, (int)work, 1, 0, 2);
    }
}

static void tails_push_sound_stop(struct task_work *work)
{
    if (PLAYER_VARS(work)->player != 1) {
        sound_stop(SE_PUSH);
    }
}

Sint8 tails_set_tail_spin(int player)
{
    struct task_work *work = player_work[player];

    if (work == NULL) {
        return 0;
    }
    return TAILS_FLAGS(work) |= TAILS_TAIL_SPIN;
}

Sint8 tails_set_tail_spin_copy(int player)
{
    struct task_work *work = player_work[player];

    if (work == NULL) {
        return 0;
    }
    return TAILS_FLAGS(work) |= TAILS_TAIL_SPIN;
}

/* ask for voice to be played VOICE_STEPS steps from now */
static void tails_voice_later(int voice)
{
    struct task *task;

    if ((task = task_create(TASK_EXTRA, 1, tails_voice_step)) == NULL) {
        return;
    }
    VOICE_LATER(task)->steps = VOICE_STEPS;
    VOICE_LATER(task)->voice = voice;
}

static void tails_voice_step(struct task *task)
{
    struct voice_later *later = VOICE_LATER(task);

    later->steps--;
    if (later->steps <= 0) {
        stream_request_voice(later->voice);
        task_destroy(task);
    }
}
