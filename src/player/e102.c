/* E-102's player task: his modes, his moves and his draw */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "event/event.h"
#include "event/play.h"
#include "hud/hud.h"
#include "minicart/riders.h"
#include "object/common.h"
#include "object/ring.h"
#include "player/e102.h"
#include "player/e102_aim.h"
#include "player/e102_bonus.h"
#include "player/e102_countdown.h"
#include "player/e102_effects.h"
#include "player/e102_hit.h"
#include "player/e102_shot.h"
#include "player/e102_smoke.h"
#include "player/e102_stand_downhill.h"
#include "player/e_mot.h"
#include "player/player.h"
#include "player/target.h"
#include "player/textures.h"
#include "sound/sound.h"
#include "stage/stage.h"
#ifdef GCC
#include "system/frame.h"
#endif
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* SE_SKATE and SE_HOVER play until stopped */
#define SE_STEP 782 /* his footsteps, and a light landing */
#define SE_E102_JUMP 783 /* his own jump. sound.h's SE_JUMP is everyone's */
#define SE_BRAKE 784
#define SE_THUD 785 /* a hard landing */
#define SE_SKATE 786
#define SE_HOVER 789
#define SE_LIFT 1189
#define SE_PUT_DOWN 1190
#define SE_THROW 1191
#define SE_PUSH_START 1192
#define SE_PULL 1193
#define SE_SKATE_START 1194
#define SE_HURT 1330 /* his voice, as he's hurt */
#define SE_TIME_UP 1340 /* his voice, as his time runs out */

/* how long he runs at full speed before he skates: 2.2 seconds of steps. SHC
 * rounds the product to 0x4303ffff, just under 132, so his 132nd step at full
 * speed starts the skate */
#define STEPS_TO_SKATE (2.2F * 60.0F)
/* the steps off the water before a skate on it turns to one on the ground */
#define STEPS_OFF_WATER 15
/* the idle steps before he fidgets: 30 seconds */
#define FIDGET_STEPS (30 * 60)
/* the steps after his init or his last leap before his time running out can
 * take him down */
#define TIME_UP_WAIT 60
/* how far he walks between his steps' sounds */
#define STEP_LENGTH 50.0F

/* his leap's fall stops in E102_LEAP_HOVER once he's less than this far
 * above the ground */
#define HOVER_HEIGHT 40.0F
/* the hover his booster gives adds 0.05 to his speed.y each step, but never
 * leaves it below HOVER_SPEED */
#define HOVER_SPEED (-0.4F)

/* the frame of ACTION_LIFT from which he has what he lifts, if he faces it
 * squarely */
#define LIFT_FRAME 12.0F

/* ACTION_LEDGE's frames, which he steps round as he moves along a ledge, and
 * the two he stops at: the last of each half */
#define LEDGE_FRAMES 32.0F
#define LEDGE_HALF 15.0F
#define LEDGE_LAST 31.0F

/* the rows of e102_actions that his code sets or tests, or that one of
 * those names as its next */
enum e102_action {
    ACTION_STAND = 0,
    ACTION_WALK = 1,
    ACTION_RUN = 2,
    ACTION_SKATE_START = 3,
    ACTION_SKATE = 5,
    ACTION_JUMP = 6,
    ACTION_FALL_START = 7, /* as a spring's or a launch's rise ends */
    ACTION_FALL = 8,
    ACTION_HARD_LANDING = 9,
    /* ACTION_STAND's motion, played once at 8 frames a step: a stop, or a light
     * landing */
    ACTION_STOP = 10,
    ACTION_PUSH_START = 11,
    ACTION_PUSH = 12,
    ACTION_HURT = 13,
    ACTION_HURT_LAND = 14,
    ACTION_UPDRAFT = 16,
    ACTION_CRASH = 17,
    ACTION_CRASH_END = 18,
    ACTION_TURN = 19, /* from a stop, back the way the stick points */
    ACTION_SLIDE = 20,
    ACTION_LIFT = 21,
    ACTION_HOLD_RUN = 22,
    ACTION_HOLD_STOP = 23,
    ACTION_HOLD_SKATE_TURN = 24, /* ACTION_HOLD_TURN's motion, into a skate */
    ACTION_HOLD_STAND = 25,
    ACTION_HOLD_WALK = 26,
    ACTION_PULL = 27,
    ACTION_SHAKE = 28,
    ACTION_GRIP_START = 29,
    ACTION_GRIP = 30, /* ACTION_SHAKE's motion, held still */
    ACTION_HOLD_JUMP = 31,
    ACTION_HOLD_FALL_START = 32,
    ACTION_HOLD_FALL = 33,
    ACTION_HOLD_HARD_LANDING = 34,
    ACTION_THROW_AT = 35, /* at the nearest task near him */
    ACTION_THROW = 36, /* ahead */
    ACTION_AIR_THROW_AT = 37,
    ACTION_AIR_THROW = 38,
    ACTION_PUT_DOWN = 39, /* ACTION_LIFT's motion, backward */
    ACTION_HOLD_JUMP_START = 41,
    ACTION_SLIDE_STOP = 42,
    ACTION_SLIDE_STAND = 43,
    ACTION_WATER_START = 44,
    ACTION_SKATE_TO_WATER = 45,
    ACTION_WATER_SKATE = 46,
    ACTION_SKATE_STOP = 47,
    ACTION_HOVER_START = 48,
    ACTION_HOVER = 49,
    ACTION_HOLD_WATER_START = 50,
    ACTION_HOLD_SKATE_TO_WATER = 51,
    ACTION_HOLD_WATER_SKATE = 52,
    /* ACTION_HOLD_RUN's motion, paced by his speed. e102_step tests for it, but
     * no code sets it */
    ACTION_HOLD_RUN_PACED = 53,
    ACTION_HOLD_SKATE_START = 54,
    ACTION_HOLD_SKATE = 55,
    ACTION_SKATE_STOP_END = 56, /* ACTION_STAND's motion, backward */
    /* ACTION_HOLD_SKATE_START's motion, backward */
    ACTION_HOLD_SKATE_END = 57,
    ACTION_WATER_TO_SKATE = 58, /* ACTION_SKATE_TO_WATER's motion, backward */
    /* ACTION_HOLD_WATER_START's motion, backward */
    ACTION_HOLD_WATER_TO_SKATE = 59,
    ACTION_SKATE_TURN = 60,
    ACTION_HOLD_TURN = 61,
    ACTION_LEDGE = 66, /* at the frame e102_ledge_move sets */
    ACTION_CLEAR = 67,
    ACTION_CLEAR_END = 68,
    ACTION_DOWN = 69,
    /* ACTION_STAND's motion, once, then ACTION_DOWN: going down from a skate
     * on the ground or on water */
    ACTION_SKATE_DOWN = 70,
    ACTION_WATER_DOWN = 71,
    ACTION_RIDE = 72,
    ACTION_FIDGET_START = 73,
    ACTION_FIDGET = 74,
    ACTION_FIDGET_END = 75, /* ACTION_FIDGET_START's motion, backward */
    ACTION_HOLD_SHAKE_START = 82,
    ACTION_HOLD_SHAKE = 83,
    ACTION_HOLD_SHAKE_END = 84,
    ACTION_STAND_UPHILL = 85,
    ACTION_STAND_DOWNHILL = 86
};

/* his model's nodes. his gun, plain or laser, ends his -z arm, and his hand
 * is on his +z side. e102_draw hangs the gun he has as the sibling of
 * e102_gun_mount in his model, and of e102_shot_gun_mount in his shot's arm */
extern NJS_OBJECT e102_object;
extern NJS_OBJECT e102_head;
extern NJS_OBJECT e102_hand;
extern NJS_OBJECT e102_upper_arm_minus_z;
extern NJS_OBJECT e102_forearm_minus_z;
extern NJS_OBJECT e102_gun_tip;
extern NJS_OBJECT e102_gun;
/* @hack type: e102_gun_mount as an array, which puts its address in the pool
 * word before e102_shot_gun_mount's (0x8c0d8824) and copies it to R4 in a
 * BT/S's slot (0x8c0d87ea). as one object, the two pool words trade places, the
 * loads reorder, and 15 of e102_draw's 862 words differ */
extern NJS_OBJECT e102_gun_mount[];
extern NJS_OBJECT e102_body;
extern NJS_OBJECT e102_foot_plus_z;
extern NJS_OBJECT e102_foot_minus_z;
extern NJS_OBJECT e102_shot_gun_mount;
extern NJS_OBJECT e102_plain_gun_tip;
extern NJS_OBJECT e102_plain_gun;
extern NJS_OBJECT e102_laser_gun_tip;
extern NJS_OBJECT e102_laser_gun;
extern NJS_OBJECT e102_shot_plain_gun;
extern NJS_OBJECT e102_shot_laser_gun;
/* the materials that fade as his time runs out: each eye's and each toe's
 * FADE_MATERIAL, and e102_head_materials[GLOW_MATERIAL], which also glows */
#define FADE_MATERIAL 2
#define GLOW_MATERIAL 6
extern NJS_MATERIAL e102_eye_minus_z_materials[];
extern NJS_MATERIAL e102_eye_plus_z_materials[];
extern NJS_MATERIAL e102_head_materials[];
extern NJS_MATERIAL e102_toe_plus_z_materials[];
extern NJS_MATERIAL e102_toe_minus_z_materials[];
extern NJS_OBJECT e102_booster;
extern NJS_ACTION e102_cannon_action;
extern NJS_ACTION e102_stand_action;
extern NJS_ACTION e102_slide_stop_action;
extern NJS_ACTION e102_slide_stand_action;
extern NJS_ACTION e102_walk_action;
extern NJS_ACTION e102_run_action;
extern NJS_ACTION e102_skate_start_action;
extern NJS_ACTION e102_skate_action;
extern NJS_ACTION e102_jump_start_action;
extern NJS_ACTION e102_jump_action;
extern NJS_ACTION e102_fall_start_action;
extern NJS_ACTION e102_fall_action;
extern NJS_ACTION e102_hard_landing_action;
extern NJS_ACTION e102_skate_stop_action;
extern NJS_ACTION e102_skate_turn_action;
extern NJS_ACTION e102_turn_action;
extern NJS_ACTION e102_water_start_action;
extern NJS_ACTION e102_skate_to_water_action;
extern NJS_ACTION e102_crash_action;
extern NJS_ACTION e102_water_skate_action;
extern NJS_ACTION e102_grip_action;
extern NJS_ACTION e102_shake_action;
extern NJS_ACTION e102_push_start_action;
extern NJS_ACTION e102_push_action;
extern NJS_ACTION e102_pull_action;
extern NJS_ACTION e102_hold_stand_action;
extern NJS_ACTION dat_8c489a20;
extern NJS_ACTION dat_8c48cbd4;
extern NJS_ACTION e102_throw_at_target_action;
extern NJS_ACTION e102_throw_action;
extern NJS_ACTION e102_hover_start_action;
extern NJS_ACTION e102_hover_action;
extern NJS_ACTION e102_clear_action;
/* the motion from his clear's last pose back to its first, which
 * ACTION_CLEAR_END plays */
extern NJS_ACTION e102_clear_end_action;
extern NJS_ACTION e102_down_action;
extern NJS_ACTION e102_hurt_action;
extern NJS_ACTION e102_hurt_land_action;
extern NJS_ACTION e102_fidget_start_action;
extern NJS_ACTION e102_fidget_action;
extern NJS_ACTION e102_stand_uphill_action;

static void e102_stand(struct task_work *work, struct task_motion *motion,
                       struct player *record);
static void e102_choose_mode(struct task *task, struct task_motion *motion,
                             struct player *record);
static void e102_destroy(struct task *task);
static void e102_draw(struct task *task);
static Bool e102_try_hurt(struct task_work *work, struct task_motion *motion,
                          struct player *record);
static Bool e102_try_jump(struct task_work *work, struct task_motion *motion,
                          struct player *record);
static Bool e102_try_halt(struct task_work *work, struct task_motion *motion,
                          struct player *record);
static Bool e102_follow_event(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static Bool e102_try_request(struct task_work *work, struct task_motion *motion,
                             struct player *record);
static Bool e102_try_request_holding(struct task_work *work,
                                     struct task_motion *motion,
                                     struct player *record);
static Bool e102_try_fall(struct task_work *work, struct task_motion *motion,
                          struct player *record);
static Bool e102_try_stop(struct task_work *work, struct task_motion *motion,
                          struct player *record);
static Bool e102_try_grab(struct task_work *work, struct task_motion *motion,
                          struct player *record);
static Bool e102_try_take_lifted(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record);
static Bool e102_try_throw(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static Bool e102_try_release(struct task_work *work, struct task_motion *motion,
                             struct player *record);
static Bool e102_try_grip(struct task_work *work, struct task_motion *motion,
                          struct player *record);
static void e102_let_go(struct task_work *work, struct task_motion *motion,
                        struct player *record);
static void e102_carry(struct task_work *work, struct task_motion *motion,
                       struct player *record);
static void e102_look_destroy(struct task *task);
static Bool e102_hover(struct task_work *work, struct task_motion *motion,
                       struct player *record);
static Bool e102_hover_water(struct task_work *work, struct task_motion *motion,
                             struct player *record);
static void e102_set_stand_action(struct task_work *work,
                                  struct task_motion *motion,
                                  struct player *record);
static void e102_set_walk_action(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record);
static void e102_move_ground(struct task_work *work, struct task_motion *motion,
                             struct player *record);
static void e102_move_air(struct task_work *work, struct task_motion *motion,
                          struct player *record);
static void e102_skate(struct task_work *work, struct task_motion *motion,
                       struct player *record);
static void e102_move_stop(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static void e102_turn(struct task_work *work, struct task_motion *motion,
                      struct player *record);
static void e102_pose_node(NJS_OBJECT *object);
static void e102_ledge_move(struct task_work *work, struct task_motion *motion,
                            struct player *record);
static Bool e102_try_time_up(struct task_work *work, struct player *record);
static void e102_step_sound(struct task_work *work, struct player *record,
                            struct e102 *e102);
static void e102_skate_sound_on(struct task_work *work, struct e102 *e102);
static void e102_skate_sound_off(struct task_work *work, struct e102 *e102);

static const NJS_POINT3 e102_zero = {0.0F, 0.0F, 0.0F};

/* his modes' names as the image spells them. nothing reads them, but they open
 * this object's data. the first 47 name the modes at their indexes. the last
 * three, EVENT, MD_KILL and MD_EDIT, sit at the indexes of E102_LEDGE to
 * E102_TIME_UP */
static char e102_mode_names[][8] = {
    "MD_INIT", "MD_STND", "MD_WALK", "MD_1DWK", "MD_JUMP", "MD_SPRG", "MD_LUNC",
    "MD_FALL", "MD_STOP", "MD_PUSH", "MD_DAMG", "MD_BACK", "MD_CRSH", "MD_SLID",
    "MD_PLON", "MD_PICK", "MD_PUT_", "MD_HOLD", "MD_TOCH", "MD_HWLK", "MD_THRW",
    "MD_HSTP", "MD_HFLL", "MD_HJMP", "MD_HSPR", "MD_HLNC", "MD_PULL", "MD_THRU",
    "MD_SHAK", "MD_PJMP", "MD_PHJP", "MD_GRUN", "MD_GRNH", "MD_WRUN", "MD_WRNH",
    "MD_WJMP", "MD_WJPH", "MD_GJMP", "MD_GJPH", "MD_GSTP", "MD_GHST", "MD_AJPI",
    "MD_AJMP", "MD_AFAL", "MD_AHVR", "MD_ACHR", "MD_ACHW", "EVENT",   "MD_KILL",
    "MD_EDIT",
};

/* his actions, which record->anim.action indexes by enum e102_action. each
 * row's next is a name, or a number where his code names no row */
static struct player_action e102_actions[] = {
    {&e102_stand_action, 77, PLAYER_ANIM_LOOP, ACTION_STAND, 0.0625F, 0.3F},
    {&e102_walk_action, 77, PLAYER_ANIM_LOOP_SPEED, ACTION_WALK, 0.25F, 0.3F},
    {&e102_run_action, 77, PLAYER_ANIM_LOOP_SPEED, ACTION_RUN, 0.25F, 0.3F},
    {&e102_skate_start_action, 77, PLAYER_ANIM_ONCE, ACTION_SKATE, 0.25F, 0.3F},
    {&e102_skate_action, 77, PLAYER_ANIM_LOOP_SPEED, 4, 0.25F, 0.3F},
    {&e102_skate_action, 77, PLAYER_ANIM_LOOP_SPEED, ACTION_SKATE, 0.125F,
     0.3F},
    {&e102_jump_action, 77, PLAYER_ANIM_LOOP, ACTION_JUMP, 0.25F, 0.3F},
    {&e102_fall_start_action, 77, PLAYER_ANIM_ONCE, ACTION_FALL, 0.125F, 0.3F},
    {&e102_fall_action, 77, PLAYER_ANIM_LOOP, ACTION_FALL, 0.25F, 0.3F},
    {&e102_hard_landing_action, 77, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F,
     0.3F},
    {&e102_stand_action, 77, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F, 8.0F},
    {&e102_push_start_action, 77, PLAYER_ANIM_ONCE, ACTION_PUSH, 0.125F, 3.0F},
    {&e102_push_action, 77, PLAYER_ANIM_LOOP, ACTION_PUSH, 0.25F, 1.0F},
    {&e102_hurt_action, 77, PLAYER_ANIM_HOLD, ACTION_HURT_LAND, 0.25F, 1.0F},
    {&e102_hurt_land_action, 77, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F, 1.0F},
    {&e102_stand_action, 77, PLAYER_ANIM_LOOP, 15, 0.25F, 1.0F},
    {&e102_stand_action, 77, PLAYER_ANIM_LOOP, ACTION_UPDRAFT, 0.0625F, 1.0F},
    {&e102_crash_action, 77, PLAYER_ANIM_ONCE, ACTION_CRASH_END, 0.5F, 1.0F},
    {&e102_stand_action, 77, PLAYER_ANIM_ONCE, ACTION_STAND, 0.0625F, 1.0F},
    {&e102_turn_action, 77, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F, 0.9F},
    {&e102_stand_action, 77, PLAYER_ANIM_LOOP, ACTION_SLIDE, 0.0625F, 1.0F},
    {&e102_lift_action, 77, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND, 0.5F, 1.0F},
    {&e102_hold_run_action, 77, PLAYER_ANIM_LOOP, ACTION_HOLD_RUN, 0.5F, 1.0F},
    {&e102_hold_stand_action, 77, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND, 1.0F,
     1.0F},
    {&e102_hold_skate_turn_action, 77, PLAYER_ANIM_ONCE, ACTION_HOLD_SKATE,
     0.5F, 1.0F},
    {&e102_hold_stand_action, 77, PLAYER_ANIM_LOOP, ACTION_HOLD_STAND, 0.0625F,
     0.5F},
    {&e102_hold_walk_action, 77, PLAYER_ANIM_LOOP, ACTION_HOLD_WALK, 0.5F,
     1.0F},
    {&e102_pull_action, 77, PLAYER_ANIM_LOOP, ACTION_PULL, 0.5F, 1.0F},
    {&e102_shake_action, 77, PLAYER_ANIM_LOOP, ACTION_SHAKE, 0.5F, 0.5F},
    {&e102_grip_action, 77, PLAYER_ANIM_ONCE, ACTION_GRIP, 0.5F, 1.0F},
    {&e102_shake_action, 77, PLAYER_ANIM_LOOP, ACTION_GRIP, 0.5F, 0.0F},
    {&e102_hold_jump_action, 77, PLAYER_ANIM_LOOP, ACTION_HOLD_JUMP, 0.5F,
     1.0F},
    {&e102_hold_fall_start_action, 77, PLAYER_ANIM_ONCE, ACTION_HOLD_FALL, 0.5F,
     1.0F},
    {&e102_hold_fall_action, 77, PLAYER_ANIM_LOOP, ACTION_HOLD_FALL, 0.5F,
     1.0F},
    {&e102_hold_hard_landing_action, 77, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND,
     0.5F, 1.0F},
    {&e102_throw_at_target_action, 77, PLAYER_ANIM_ONCE, ACTION_STAND, 0.5F,
     1.0F},
    {&e102_throw_action, 77, PLAYER_ANIM_ONCE, ACTION_STAND, 0.5F, 1.0F},
    {&e102_throw_at_target_action, 77, PLAYER_ANIM_ONCE, ACTION_FALL, 0.5F,
     1.0F},
    {&e102_throw_action, 77, PLAYER_ANIM_ONCE, ACTION_FALL, 0.5F, 1.0F},
    {&e102_lift_action, 77, PLAYER_ANIM_BACK, ACTION_STAND, 0.5F, 1.0F},
    {&e102_jump_start_action, 77, PLAYER_ANIM_ONCE, ACTION_JUMP, 0.25F, 1.0F},
    {&e102_hold_jump_start_action, 77, PLAYER_ANIM_ONCE, ACTION_HOLD_JUMP,
     0.25F, 1.0F},
    {&e102_slide_stop_action, 77, PLAYER_ANIM_ONCE, ACTION_SLIDE_STAND,
     1.0F / 6.0F, 0.5F},
    {&e102_slide_stand_action, 77, PLAYER_ANIM_ONCE, ACTION_SLIDE_STAND,
     1.0F / 6.0F, 0.5F},
    {&e102_water_start_action, 77, PLAYER_ANIM_ONCE, ACTION_WATER_SKATE, 0.25F,
     1.5F},
    {&e102_skate_to_water_action, 77, PLAYER_ANIM_ONCE, ACTION_WATER_SKATE,
     0.25F, 1.5F},
    {&e102_water_skate_action, 77, PLAYER_ANIM_LOOP, ACTION_WATER_SKATE, 1.0F,
     1.0F},
    {&e102_skate_stop_action, 77, PLAYER_ANIM_ONCE, ACTION_SKATE, 0.25F, 1.0F},
    {&e102_hover_start_action, 77, PLAYER_ANIM_ONCE, ACTION_HOVER, 0.25F, 1.0F},
    {&e102_hover_action, 77, PLAYER_ANIM_LOOP, ACTION_HOVER, 0.25F, 1.0F},
    {&e102_hold_water_start_action, 77, PLAYER_ANIM_ONCE,
     ACTION_HOLD_WATER_SKATE, 0.25F, 1.5F},
    {&e102_hold_skate_to_water_action, 77, PLAYER_ANIM_ONCE,
     ACTION_HOLD_WATER_SKATE, 0.25F, 1.0F},
    {&e102_hold_water_skate_action, 77, PLAYER_ANIM_LOOP,
     ACTION_HOLD_WATER_SKATE, 0.25F, 1.0F},
    {&e102_hold_run_action, 77, PLAYER_ANIM_LOOP_SPEED, ACTION_HOLD_RUN_PACED,
     0.25F, 0.5F},
    {&e102_hold_skate_start_action, 77, PLAYER_ANIM_ONCE, ACTION_HOLD_SKATE,
     0.25F, 0.8F},
    {&e102_hold_skate_action, 77, PLAYER_ANIM_LOOP_SPEED, ACTION_HOLD_SKATE,
     0.25F, 0.5F},
    {&e102_stand_action, 77, PLAYER_ANIM_BACK, ACTION_STOP, 0.0625F, 1.0F},
    {&e102_hold_skate_start_action, 77, PLAYER_ANIM_BACK, ACTION_HOLD_STAND,
     1.0F, 1.0F},
    {&e102_skate_to_water_action, 77, PLAYER_ANIM_BACK, ACTION_SKATE, 0.25F,
     1.5F},
    {&e102_hold_water_start_action, 77, PLAYER_ANIM_BACK, ACTION_HOLD_SKATE,
     0.25F, 1.5F},
    {&e102_skate_turn_action, 77, PLAYER_ANIM_ONCE, ACTION_SKATE, 1.0F, 0.9F},
    {&e102_hold_skate_turn_action, 77, PLAYER_ANIM_ONCE, ACTION_HOLD_SKATE_END,
     1.0F, 1.0F},
    {&dat_8c489a20, 77, PLAYER_ANIM_BACK, 63, 0.25F, 1.0F},
    {&dat_8c489a20, 77, PLAYER_ANIM_ONCE, 62, 0.25F, 1.0F},
    {&dat_8c48cbd4, 77, PLAYER_ANIM_BACK, 65, 1.0F / 6.0F, 1.0F},
    {&dat_8c48cbd4, 77, PLAYER_ANIM_ONCE, 64, 0.25F, 1.0F},
    {&e102_ledge_action, 77, PLAYER_ANIM_DRIVEN, ACTION_LEDGE, 0.125F, 1.0F},
    {&e102_clear_action, 77, PLAYER_ANIM_HOLD, ACTION_CLEAR, 1.0F, 0.35F},
    {&e102_clear_end_action, 77, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F, 0.35F},
    {&e102_down_action, 77, PLAYER_ANIM_HOLD, ACTION_DOWN, 0.25F, 1.0F},
    {&e102_stand_action, 77, PLAYER_ANIM_ONCE, ACTION_DOWN, 0.25F, 1.0F},
    {&e102_stand_action, 77, PLAYER_ANIM_ONCE, ACTION_DOWN, 0.25F, 1.0F},
    {&e102_cart_action, 77, PLAYER_ANIM_HOLD, ACTION_RIDE, 0.25F, 1.0F},
    {&e102_fidget_start_action, 77, PLAYER_ANIM_ONCE, ACTION_FIDGET,
     1.0F / 6.0F, 0.25F},
    {&e102_fidget_action, 77, PLAYER_ANIM_ONCE, ACTION_FIDGET_END, 1.0F / 6.0F,
     0.25F},
    {&e102_fidget_start_action, 77, PLAYER_ANIM_BACK, ACTION_STAND, 1.0F / 6.0F,
     0.25F},
    {&dat_0cc29324, 77, PLAYER_ANIM_ONCE, 77, 1.0F / 6.0F, 0.5F},
    {&dat_0cc2b7c8, 77, PLAYER_ANIM_ONCE, 77, 1.0F / 6.0F, 0.5F},
    {&dat_0cc2e3ec, 77, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F / 6.0F, 0.5F},
    {&dat_0cc30570, 77, PLAYER_ANIM_ONCE, 80, 1.0F / 6.0F, 0.5F},
    {&dat_0cc32df4, 77, PLAYER_ANIM_ONCE, 80, 1.0F / 6.0F, 0.5F},
    {&dat_0cc35bb8, 77, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F / 6.0F, 0.5F},
    {&e102_hold_shake_start_action, 77, PLAYER_ANIM_ONCE, ACTION_HOLD_SHAKE,
     1.0F / 6.0F, 0.5F},
    {&e102_hold_shake_action, 77, PLAYER_ANIM_LOOP, ACTION_HOLD_SHAKE,
     1.0F / 6.0F, 0.5F},
    {&e102_hold_shake_end_action, 77, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND,
     1.0F / 6.0F, 0.5F},
    {&e102_stand_uphill_action, 77, PLAYER_ANIM_LOOP, ACTION_STAND_UPHILL,
     1.0F / 6.0F, 1.0F},
    {&e102_stand_downhill_action, 77, PLAYER_ANIM_LOOP, ACTION_STAND_DOWNHILL,
     1.0F / 6.0F, 1.0F},
};

/* by enum e102_mode, TRUE where he can aim. its last three entries, past
 * E102_EDIT, belong to no mode */
static Sint8 e102_aim_modes[] = {
    TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,
    FALSE, TRUE,  TRUE,  TRUE,  TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE,
    FALSE, TRUE,  FALSE, TRUE,  FALSE, TRUE,  FALSE, TRUE,  FALSE, TRUE,
    TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE};

/* his collision shapes: his body's, and a smaller one that e102_draw puts
 * 5 above his feet */
/* clang-format off */
static struct collision_shape e102_shapes[] = {
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS,
     PUSH_ALL | PUSH_FIRM(PUSH_OTHERS), {0.0F, 1.0F, 0.0F}, 6.0F, 4.0F, 0.65F,
     {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS,
     PUSH_ALL | PUSH_FIRM(PUSH_OTHERS), {0.0F, 1.0F, 0.0F}, 4.0F, 4.0F, 0.65F,
     {0}, 0, NULL},
};
/* clang-format on */

/* the points his welds mark: his gun's tip, his hand, then his feet.
 * e102_carry holds what he carries between the first two, and e102_draw
 * points the first at the tip of the gun he has */
static struct weld e102_welds[] = {
    {&e102_object, &e102_gun_tip, NULL, 2, WELD_MARK, 0, 0, NULL, NULL},
    {&e102_object, &e102_hand, NULL, 2, WELD_MARK + 1, 0, 0, NULL, NULL},
    {&e102_object, &e102_foot_plus_z, NULL, 0, WELD_MARK + 2, 0, 0, NULL, NULL},
    {&e102_object, &e102_foot_minus_z, NULL, 0, WELD_MARK + 3, 0, 0, NULL,
     NULL},
    {NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL},
};

NJS_POINT3 e102_beam_points[2] = {{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F}};
NJS_COLOR e102_beam_colors[4] = {
#include "player/e102/e102_beam_colors.inc"
};

/* his arm's parts, hidden while his shot's action takes their place */
static NJS_OBJECT *e102_arm_objects[] = {
    &e102_upper_arm_minus_z,
    &e102_forearm_minus_z,
    &e102_gun,
    &e102_gun_tip,
    e102_gun_mount,
    &e102_plain_gun,
    &e102_plain_gun_tip,
    &e102_laser_gun,
    &e102_laser_gun_tip,
    NULL,
};

/* how far above his cannon e102_draw puts record->consts.height */
static Float e102_height_above_cannon = 15.0F;
/* a point in a node's space. e102_draw passes it, with e102_node_point_z after
 * it, as the NJS_POINT3 it puts through the node's matrix */
#ifdef GCC
/* one NJS_POINT3 under GCC, so that z is the point's own. GCC inlines
 * njCalcPoint, whose read of z lies past an NJS_POINT2. GCC may assume that
 * read touches no other object, and so move a store to e102_node_point_z
 * past it, or drop the store */
static NJS_POINT3 e102_node_point = {0.0F, 0.0F, 0.0F};
#define e102_node_point_z e102_node_point.z
#else
/* @hack type: the point as an NJS_POINT2 and a Float after it, so that SHC
 * loads z's own address to reach it (MOV.L @(0x8c0d8f0c),R3 at 0x8c0d8e62). as
 * one NJS_POINT3, SHC reaches z from the point's address, e102_draw's frame
 * grows by 4, and 495 of its 862 words differ */
static NJS_POINT2 e102_node_point = {0.0F, 0.0F};
static Float e102_node_point_z = 0.0F;
#endif

Sint8 e102_in_play;
Sint8 e102_hovering;
Sint8 e102_time_runs;
NJS_POINT3COL e102_beam;
struct e102 *e102_current;
static Sint8 e102_hovered; /* TRUE if he hovered on his last step */
static Sint8 e102_time_up; /* TRUE once his time has run out */
static Sint32 dat_8c7abc7c;
static NJS_MATRIX cannon_matrix;
static NJS_MATRIX booster_matrix;
static NJS_MATRIX cannon_end_matrix;
static NJS_MATRIX arm_matrix;
static NJS_MATRIX shot_matrix;
static NJS_MATRIX node_2_matrix;
/* each material his init readies, in turn */
static NJS_MATERIAL *e102_material;

void e102_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;
    struct e102 *e102 = NULL;

    e102_hovered = e102_hovering;
    e102_hovering = FALSE;
    e102_in_play = !event_in_control();
    e102_time_up = time_is_zero();
    if (motion != NULL) {
        /* on top of a task and not rising, he stands on it. his collision's
         * owner is his own task, so copying its x and z angles leaves his as
         * they were */
        if ((work->flags & COLLISION_ON_TOP) && motion->speed.y <= 0.0F) {
            work->flags |= PLAYER_ON_TASK;
            work->ang.x = work->collision->owner->work->ang.x;
            work->ang.z = work->collision->owner->work->ang.z;
        } else {
            work->flags &= ~PLAYER_ON_TASK;
        }
        if (record != NULL) {
            Bool lag;

            e102 = record->own.e102.record[0];
            lag = e102->aim_mode == E102_AIM_LOCKING;
            e102_record_stick(e102, PLAYER_VARS(work)->player, lag);
            if (edit_mode == 1) {
                work->mode = E102_EDIT;
            } else if (work->mode != E102_DOWN) {
                e102_choose_mode(task, motion, record);
                e102_try_time_up(work, record);
            }
        }
    }
    if (work->collision != NULL) {
        Uint32 attack;
        Uint32 guard;
        struct collision_shape *shape = work->collision->shapes;

        if (record->effects & PLAYER_INVINCIBLE) {
            attack = 3;
            guard = 3;
        } else if (work->mode == E102_MODULE_POSE) {
            attack = 0;
            guard = 1;
        } else {
            attack = 0;
            guard = 0;
        }
        shape->levels = (shape->levels & ~SHAPE_ATTACK) | ATTACK(attack);
        shape->levels = (shape->levels & ~SHAPE_GUARD) | GUARD(guard);
        shape[1].levels = (shape[1].levels & ~SHAPE_ATTACK) | ATTACK(attack);
        shape[1].levels = (shape[1].levels & ~SHAPE_GUARD) | GUARD(guard);
    }
    /* @hack spelling: modes that move him alike keep cases of their own,
     * since the image keeps a copy of the calls for each. with each such
     * case's label moved to the first case that makes the same calls,
     * e102.c's code runs 480 bytes short. in e102_step, 1144 of 1846 words
     * differ */
    switch (work->mode) {
    case E102_INIT:
        PLAYER_VARS(work)->character = CHARACTER_E102;
        player_init(PLAYER_VARS(work)->player, task);
        record = motion->record;
        record->anim.actions = e102_actions;
        record->anim.action = ACTION_STAND;
        collision_init(task, e102_shapes, COLLISION_PLAYER, 1, 2);
        work->collision->shapes->radius = record->consts.radius;
        texture_load_character(CHARACTER_E102);
        record->anim.welds = e102_welds;
        task->destructor = e102_destroy;
        task->draw = e102_draw;
        e102 = heap_calloc(1, sizeof(struct e102));
        record->own.e102.record[0] = e102;
        record->own.e102.skate_steps = 0;
        e102_make_strips(&e102->inner, &e102->outer);
        e102->walked = 0.0F;
        e102_beam.p = e102_beam_points;
        e102_beam.tex = NULL;
        e102_beam.num = 2;
        e102_beam.col = e102_beam_colors;
        work->mode = E102_STAND;
        e102_smoke_init();
        /* the materials that fade blend by their alpha */
        e102_material = &e102_head_materials[GLOW_MATERIAL];
        e102_material->attrflags &= ~(NJD_SA_MASK | NJD_DA_MASK);
        e102_material->attrflags |= NJD_SA_SRC | NJD_FLAG_USE_ALPHA;
        e102_material = &e102_eye_plus_z_materials[FADE_MATERIAL];
        e102_material->attrflags &= ~(NJD_SA_MASK | NJD_DA_MASK);
        e102_material->attrflags |= NJD_SA_SRC | NJD_FLAG_USE_ALPHA;
        e102_material->diffuse.argb.a = 255;
        e102_material = &e102_eye_minus_z_materials[FADE_MATERIAL];
        e102_material->attrflags &= ~(NJD_SA_MASK | NJD_DA_MASK);
        e102_material->attrflags |= NJD_SA_SRC | NJD_FLAG_USE_ALPHA;
        e102_material->diffuse.argb.a = 255;
        e102_material = &e102_toe_plus_z_materials[FADE_MATERIAL];
        e102_material->attrflags &= ~(NJD_SA_MASK | NJD_DA_MASK);
        e102_material->attrflags |= NJD_SA_SRC | NJD_FLAG_USE_ALPHA;
        e102_material->diffuse.argb.a = 255;
        e102_material = &e102_toe_minus_z_materials[FADE_MATERIAL];
        e102_material->attrflags &= ~(NJD_SA_MASK | NJD_DA_MASK);
        e102_material->attrflags |= NJD_SA_SRC | NJD_FLAG_USE_ALPHA;
        e102_material->diffuse.argb.a = 255;
        /* as a player task, his time counts down in the action stages and
         * the bosses, unless an event holds the game */
        e102_time_runs = FALSE;
        if (work->id == PLAYER_ID && event_in_control() != TRUE &&
            STAGE_OF(stage_get_stage_act()) <= STAGE_BOSS_LAST) {
            e102_time_runs = TRUE;
            task_create_child(TASK_WORK, e102_countdown_step, task);
        }
        e102_hit_init();
        work->flags |= PLAYER_ON_LAND;
        break;
    case E102_STAND:
    case E102_PRE_JUMP:
        e102_turn(work, motion, record);
        e102_move_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        e102_set_stand_action(work, motion, record);
        break;
    case E102_WALK:
        if (record->anim.previous == ACTION_TURN &&
            record->anim.current != ACTION_TURN) {
            record->speed.x += record->consts.trot_speed;
        }
        e102_move_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        if (player_collide(work, motion, record) == PLAYER_CRASHED) {
            record->speed.x = -record->speed.x;
            record->speed.z = -record->speed.z;
        } else {
            player_take_speed(work, motion, record);
            e102_set_walk_action(work, motion, record);
        }
        e102_step_sound(work, record, e102);
        break;
    case E102_SKATE:
        if (record->anim.previous == ACTION_SKATE_TURN &&
            record->anim.current != ACTION_SKATE_TURN) {
            record->speed.x += record->consts.trot_speed;
        }
        e102_skate(work, motion, record);
        player_apply_accel(work, motion, record);
        if (player_collide(work, motion, record) == PLAYER_CRASHED) {
            record->speed.x = -record->speed.x;
            record->speed.z = -record->speed.z;
        } else {
            player_take_speed(work, motion, record);
        }
        break;
    case E102_WATER_SKATE:
        if (record->anim.previous == ACTION_TURN &&
            record->anim.current != ACTION_TURN) {
            record->speed.x += record->consts.trot_speed;
        }
        e102_skate(work, motion, record);
        player_apply_accel(work, motion, record);
        if (player_collide(work, motion, record) == PLAYER_CRASHED) {
            player_crash_burst(work);
            record->speed.x = -record->speed.x;
            record->speed.z = -record->speed.z;
        } else {
            player_take_speed(work, motion, record);
        }
        record->speed.x = CLAMP(record->speed.x, -record->consts.running_speed,
                                record->consts.running_speed);
        break;
    case E102_PATH:
        player_accel_path(work, motion, record);
        player_apply_path_accel(work, motion, record);
        player_move_on_path(work, motion, record);
        e102_set_walk_action(work, motion, record);
        break;
    case E102_JUMP:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            e102_move_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        if (motion->speed.y <= 0.0F) {
            work->mode = E102_FALL;
        }
        break;
    case E102_WATER_JUMP:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            e102_move_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        e102_hover_water(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_HOLD_WATER_JUMP:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            e102_move_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        e102_hover_water(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_SKATE_JUMP:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            e102_move_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        if (e102_try_halt(work, motion, record)) {
            work->mode = E102_FALL;
            record->anim.action = ACTION_FALL;
        }
        break;
    case E102_SPRING:
        e102_move_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_LAUNCH:
        if (record->speed.y >= record->consts.launch_speed) {
            player_accel_launched(work, motion, record);
        } else {
            e102_move_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_FALL:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            e102_move_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        if (e102_hover(work, motion, record) == TRUE) {
            if (record->anim.action != ACTION_HOVER_START &&
                record->anim.action != ACTION_HOVER) {
                record->anim.action = ACTION_HOVER_START;
            }
        } else {
            record->anim.action = ACTION_FALL;
        }
        record->own.e102.record[0]->fall_speed = record->speed.y;
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_STOP:
    case E102_SKATE_STOP:
        e102_move_stop(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_PUSH:
        if (record->anim.action == ACTION_PUSH_START) {
            e102_move_stop(work, motion, record);
        } else {
            player_accel_push(work, motion, record);
            sound_play_for(SE_PUSH, (int)work, 1, 0, 2);
            if (record->anim.previous == ACTION_PUSH_START) {
                if (e102_in_play) {
                    sound_play_at(SE_PUSH_START, 0, 0, 0, work->pos.x,
                                  work->pos.y, work->pos.z);
                }
            }
        }
        player_apply_push(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_HURT:
        player_turn_upright(work, motion, record);
        e102_move_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_BACK:
        e102_move_stop(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_CRASH:
        e102_move_stop(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_SLIDE:
        e102_turn(work, motion, record);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_POSE:
    case E102_CLEAR:
        break;
    case E102_LIFT:
        e102_turn(work, motion, record);
        e102_move_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_PUT_DOWN:
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_HOLD_STAND:
        e102_turn(work, motion, record);
        e102_move_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_GRIP:
        player_accel_push(work, motion, record);
        player_apply_push(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_THROW:
        player_turn_to_target(work, motion, record);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_HOLD_WALK:
        if (record->anim.previous == ACTION_HOLD_SKATE_TURN &&
            record->anim.current != ACTION_HOLD_SKATE_TURN) {
            record->speed.x += record->consts.trot_speed;
        }
        e102_move_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        e102_set_walk_action(work, motion, record);
        e102_step_sound(work, record, e102);
        break;
    case E102_HOLD_SKATE:
        if (record->anim.previous == ACTION_HOLD_SKATE_TURN &&
            record->anim.current != ACTION_HOLD_SKATE_TURN) {
            record->speed.x += record->consts.trot_speed;
        }
        e102_move_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_HOLD_WATER_SKATE:
        if (record->anim.previous == ACTION_HOLD_SKATE_TURN &&
            record->anim.current != ACTION_HOLD_SKATE_TURN) {
            record->speed.x += record->consts.trot_speed;
        }
        e102_move_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_HOLD_STOP:
    case E102_HOLD_SKATE_STOP:
        e102_move_stop(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_HOLD_FALL:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            e102_move_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        e102_hover(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_HOLD_JUMP:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            e102_move_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_HOLD_SKATE_JUMP:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            e102_move_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_HOLD_SPRING:
        e102_move_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_HOLD_LAUNCH:
        if (record->speed.y >= record->consts.launch_speed) {
            player_accel_launched(work, motion, record);
        } else {
            e102_move_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_PULL:
        player_accel_push(work, motion, record);
        player_apply_push(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        sound_play_for(SE_PUSH, (int)work, 1, 0, 2);
        break;
    case E102_AIR_THROW:
        player_turn_upright(work, motion, record);
        player_turn_to_target(work, motion, record);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case E102_SHAKE:
        player_shake_touched(work, motion, record);
        break;
    case E102_HOLD_SHAKE:
        record->held->motion->accel.x = record->held->motion->accel.y =
            record->held->motion->accel.z = record->anim.frame * 0.01F;
        break;
    case E102_LEAP_START:
        record->anim.action = ACTION_JUMP;
        work->mode = E102_LEAP;
        break;
    case E102_LEAP:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            e102_move_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        if (motion->speed.y <= 0.0F) {
            work->mode = E102_LEAP_FALL;
            record->anim.action = ACTION_FALL;
        }
        break;
    case E102_LEAP_FALL:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            e102_move_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        if (record->speed.y < 0.0F && e102->above < HOVER_HEIGHT) {
            record->speed.x = record->speed.y = record->speed.z = 0.0F;
            record->anim.action = ACTION_HOVER_START;
            work->mode = E102_LEAP_HOVER;
        }
        break;
    case E102_LEAP_HOVER:
        player_turn_to_stick(work, motion, record);
        e102_hovering = TRUE;
        break;
    case E102_SKATE_START:
        record->anim.action = ACTION_SKATE_START;
        work->mode = E102_SKATE;
        e102_skate_sound_on(work, e102);
        break;
    case E102_SKATE_BRAKE:
        work->mode = E102_SKATE_STOP;
        record->anim.action = ACTION_SKATE_STOP;
        if (e102_in_play) {
            sound_play_at(SE_BRAKE, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        break;
    case E102_LEDGE:
    case E102_LEDGE_GO:
        player_shift_on_path(work, motion, record);
        e102_ledge_move(work, motion, record);
        break;
    case E102_MODULE_POSE:
        break;
    case E102_EVENT:
        event_paths_step(work);
        break;
    case E102_TIME_UP:
    case E102_DOWN:
        player_turn_upright(work, motion, record);
        player_accel_roll(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    /* @hack redundant: two cases nothing needs, which keep e102_current's
     * address in R3 and e102 in R2 for the store after the switch (0x8c0d59d2,
     * 0x8c0d59d4). without them, the two trade and 5 of 1846 words differ */
    case E102_HOLD_PRE_JUMP:
    case E102_UNK_50:
        break;
    }
    e102_current = e102;
    player_anim_step(&record->anim);
    player_update_welds(work, motion, record);
    /* @hack spelling: a bare scope for own, which shares a frame slot with
     * the blocks above (ADD #-32,R15 at 0x8c0d47a4). declared with e102, own
     * takes a slot of its own, the frame grows by 4, and 589 of 1846 words
     * differ */
    {
        struct e102 *own;

        own = record->own.e102.record[0];
        e102_restore_stick(own, PLAYER_VARS(work)->player);
        pose_get_matrix(54, &cannon_matrix);
        pose_get_matrix(30, &booster_matrix);
        pose_get_matrix(39, &arm_matrix);
        njRotateZ(&arm_matrix, 0x4000);
        pose_get_matrix(2, &node_2_matrix);
        pose_get_matrix(37, &shot_matrix);
        e102_bob_booster(work, record, e102);
        e102_draw(task);
        e102_aim(work, record, record->own.e102.record[0]);
        e102_shoot(task, record, record->own.e102.record[0]);
        if (record->held != NULL) {
            e102_carry(work, motion, record);
        }
        own = record->own.e102.record[0];
        own->ground = shadow_draw(work, 1.5F);
        own->above = work->pos.y - own->ground;
        own->ground_flags = e102_floor_flags(work);
        own->steps++;
        if ((record->anim.action == ACTION_SKATE ||
             record->anim.action == ACTION_HOLD_RUN_PACED) &&
            record->speed.x > 3.5F) {
            e102_skate_sparks(work, motion, record);
        }
        /* in his clear's actions, nothing collides with him */
        if (record->anim.current != ACTION_CLEAR &&
            record->anim.current != ACTION_CLEAR_END) {
            collision_register(work);
        }
        player_finish_step(work, motion, record);
    }
    task_step_children(task);
}

static void e102_stand(struct task_work *work, struct task_motion *motion,
                       struct player *record)
{
    work->mode = E102_STAND;
    record->idle_steps = 0;
    e102_set_stand_action(work, motion, record);
}

/* his mode for this step. e102_try_hurt comes before his mode's own checks.
 * it doesn't run while he's invincible, in E102_EDIT, or while work->timer
 * counts down from his last hurt */
/* NOLINTNEXTLINE(readability-function-size) */
static void e102_choose_mode(struct task *task, struct task_motion *motion,
                             struct player *record)
{
    struct task_work *work = task->work;
    struct e102 *e102 = record->own.e102.record[0];

    if (record != NULL && (record->effects & PLAYER_INVINCIBLE)) {
        work->flags |= PLAYER_RECOILING;
        work->timer = 0;
    } else if (work->timer == 0) {
        if (work->mode != E102_EDIT) {
            if (e102_try_hurt(work, motion, record)) {
                work->flags |= PLAYER_RECOILING;
                if (work->flags & PLAYER_HOLDING) {
                    e102_let_go(work, motion, record);
                }
                work->flags &= ~(PLAYER_ON_PATH | PLAYER_REQUEST |
                                 PLAYER_HOLDING | PLAYER_BALL);
                /* a barrier takes the hit, or his rings do */
                if (record->effects &
                    (PLAYER_BARRIER | PLAYER_MAGNETIC_BARRIER)) {
                    record->effects &=
                        ~(PLAYER_BARRIER | PLAYER_MAGNETIC_BARRIER);
                } else {
                    ring_lose(PLAYER_VARS(work)->player);
                }
                e102->shot_phase = E102_SHOT_START;
            } else {
                work->flags &= ~PLAYER_RECOILING;
            }
        }
    } else {
        work->timer--;
        work->flags &= ~PLAYER_HURT;
    }
    switch (work->mode) {
    case E102_STAND:
        if (e102_try_request(work, motion, record)) {
            break;
        }
        if (e102_try_fall(work, motion, record)) {
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            break;
        }
        if (e102_try_grab(work, motion, record)) {
            break;
        }
        /* @hack redundant: an else after an arm that breaks, which puts the
         * record in R3 and its contact in R2 for the slope's test (0x8c0d5dd4,
         * 0x8c0d5dd8). as an if, R1 and R3 take them, and 3 of 4148 words
         * differ */
        if (player_read_stick(work, NULL, NULL)) {
            work->mode = E102_WALK;
            record->anim.action = ACTION_WALK;
            break;
        } else if (record->contact->normal.y < 0.77F &&
                   njScalor(&record->speed) > record->consts.slide_speed) {
            work->mode = E102_SLIDE;
            record->anim.action = ACTION_SLIDE;
            break;
        }
        if ((record->ground_flags & WORLD_WATER) &&
            record->anim.action != ACTION_WATER_START) {
            work->mode = E102_WATER_SKATE;
            record->anim.action = ACTION_WATER_START;
            e102_skate_sound_off(work, e102);
            /* @hack redundant: a redundant break, which branches to
             * E102_SKATE's water arm (BRA 0x8c0d6258 at 0x8c0d5e90). there the
             * call to e102_skate_sound_off ends the function as a JMP. without
             * the break, this arm calls it with a JSR of its own, the code
             * after it moves, and 42 of 4148 words differ */
            break;
        }
        break;
    case E102_WALK:
        if (e102_try_request(work, motion, record)) {
            break;
        }
        if (e102_try_fall(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (e102_try_stop(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if ((work->collision->flags & COLLISION_PUSHED) &&
            !(work->flags & PLAYER_HURT) &&
            !(work->collision->hit->shapes[work->collision->hit_shape].flags &
              SHAPE_NO_PUSH) &&
            (work->collision->flags & COLLISION_HIT_SIDE)) {
            /* @hack spelling: a switch of one case, which keeps work in R2 and
             * the values its arm stores in R3 (0x8c0d5fa6 to 0x8c0d5fb2). as an
             * if, the two trade and 6 of 4148 words differ */
            switch (player_stick_into_wall(work, motion)) {
            case PLAYER_WALL_HEAD_ON:
                work->mode = E102_PUSH;
                record->anim.action = ACTION_PUSH_START;
                work->flags &= ~PLAYER_ON_PATH;
                break;
            }
        }
        if (player_stick_back_or_end_event(work, motion) &&
            record->speed.x >= record->consts.trot_speed) {
            work->mode = E102_STOP;
            record->anim.action = ACTION_STOP;
            if (e102_in_play) {
                sound_play_at(SE_BRAKE, 0, 0, 0, work->pos.x, work->pos.y,
                              work->pos.z);
            }
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (record->anim.action == ACTION_TURN) {
            break;
        }
        if (record->speed.x >= record->consts.running_speed) {
            record->speed.x = record->consts.running_speed;
            record->own.e102.skate_steps++;
            if (record->own.e102.skate_steps > STEPS_TO_SKATE) {
                record->own.e102.skate_steps = 0;
                work->mode = E102_SKATE;
                record->anim.action = ACTION_SKATE_START;
                if (e102_in_play) {
                    sound_play_at(SE_SKATE_START, 0, 0, 0, work->pos.x,
                                  work->pos.y, work->pos.z);
                }
                break;
            }
        } else {
            record->own.e102.skate_steps = 0;
        }
        if ((record->ground_flags & WORLD_WATER) &&
            record->anim.action != ACTION_WATER_START) {
            work->mode = E102_WATER_SKATE;
            record->anim.action = ACTION_WATER_START;
            e102_skate_sound_off(work, e102);
            if (e102_in_play) {
                sound_play_at(SE_SKATE_START, 0, 0, 0, work->pos.x, work->pos.y,
                              work->pos.z);
            }
        }
        break;
    case E102_SKATE:
        if (e102_try_request(work, motion, record)) {
            break;
        }
        if (e102_try_fall(work, motion, record)) {
            e102_skate_sound_off(work, e102);
            work->flags &= ~PLAYER_ON_PATH;
            work->mode = E102_SKATE_JUMP;
            record->anim.action = ACTION_SKATE;
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (_builtin_fabsf(record->speed.x) > 0.5F) {
            e102_skate_sound_on(work, e102);
        } else {
            e102_skate_sound_off(work, e102);
        }
        if ((record->ground_flags & WORLD_WATER) &&
            record->anim.action == ACTION_SKATE) {
            work->mode = E102_WATER_SKATE;
            record->anim.action = ACTION_SKATE_TO_WATER;
            e102_skate_sound_off(work, e102);
            /* @hack redundant: a redundant break, which makes this arm's call
             * to e102_skate_sound_off the JMP that ends the function
             * (0x8c0d6258), where E102_STAND's water arm branches too. without
             * it, the arm calls it with a JSR and jumps to the end, the code
             * after it moves, and 113 of 4148 words differ */
            break;
        }
        break;
    case E102_WATER_SKATE:
        if (e102_try_request(work, motion, record)) {
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (!(record->ground_flags & WORLD_WATER) &&
            record->anim.action == ACTION_WATER_SKATE) {
            record->own.e102.skate_steps++;
            if (record->own.e102.skate_steps > STEPS_OFF_WATER) {
                record->own.e102.skate_steps = 0;
                work->mode = E102_SKATE;
                record->anim.action = ACTION_WATER_TO_SKATE;
                e102_skate_sound_on(work, e102);
                break;
            }
        } else {
            record->own.e102.skate_steps = 0;
        }
        break;
    case E102_PATH:
        if (e102_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (e102_try_fall(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (record->speed.x < record->consts.trot_speed) {
            work->mode = E102_WALK;
            record->anim.action = ACTION_WALK;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (e102_try_stop(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        break;
    case E102_PRE_JUMP:
        if (e102_try_request(work, motion, record)) {
            break;
        }
        if (record->anim.current == ACTION_JUMP ||
            record->anim.current == ACTION_HOLD_JUMP) {
            /* @hack redundant: mode held in R14, the one register the
             * function saves (MOV.L R14,@-R15 at 0x8c0d5bae). without
             * register, mode takes a frame slot, the code after it moves,
             * and 3482 of 4148 words differ */
            register int mode;

            record->speed.y = record->consts.jump_speed;
            /* he leaves with the speed of what he stood on */
            if (record->ground_task != NULL) {
                struct task_force *force = record->ground_task->force;

                if (force != NULL) {
                    player_set_accel(PLAYER_VARS(work)->player, force->speed.x,
                                     force->speed.y, force->speed.z);
                }
            }
            work->flags &= ~PLAYER_ON_TASK;
            record->jump_steps = 0;
            if (e102_in_play) {
                sound_play_at(SE_E102_JUMP, 0, 0, 0, work->pos.x, work->pos.y,
                              work->pos.z);
            }
            if (record->anim.current == ACTION_HOLD_JUMP) {
                mode = E102_HOLD_JUMP;
            } else {
                mode = E102_JUMP;
            }
            work->mode = mode;
            break;
        }
        break;
    case E102_JUMP:
        if (e102_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = E102_STOP;
                record->anim.action = ACTION_STOP;
            } else if (e102_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (motion->speed.y > -5.0F) {
                    record->anim.action = ACTION_STAND;
                } else {
                    record->anim.action = ACTION_HARD_LANDING;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = E102_WALK;
                e102_set_walk_action(work, motion, record);
            }
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (e102_try_halt(work, motion, record)) {
            work->mode = E102_FALL;
            record->anim.action = ACTION_FALL;
            break;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.8F;
        }
        break;
    case E102_WATER_JUMP:
        if (e102_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            struct e102 *own = record->own.e102.record[0];

            if (own->ground_flags & WORLD_WATER) {
                work->mode = E102_WATER_SKATE;
                e102_skate_sound_off(work, own);
            } else {
                work->mode = E102_SKATE;
                record->anim.action = ACTION_WATER_TO_SKATE;
                e102_skate_sound_on(work, own);
            }
            break;
        }
        if (e102_try_halt(work, motion, record)) {
            break;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.8F;
        }
        break;
    case E102_SKATE_JUMP:
        if (e102_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            struct e102 *own = record->own.e102.record[0];

            if (own->ground_flags & WORLD_WATER) {
                work->mode = E102_WATER_SKATE;
                record->anim.action = ACTION_SKATE_TO_WATER;
                e102_skate_sound_off(work, own);
            } else {
                work->mode = E102_SKATE;
                e102_skate_sound_on(work, own);
            }
            break;
        }
        if (e102_try_halt(work, motion, record)) {
            break;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.8F;
        }
        break;
    case E102_SPRING:
        if (motion->speed.y <= 0.0F) {
            work->mode = E102_FALL;
            record->anim.action = ACTION_FALL_START;
        }
        break;
    case E102_LAUNCH:
        if (motion->speed.y <= 0.0F) {
            work->mode = E102_FALL;
            record->anim.action = ACTION_FALL_START;
        }
        break;
    case E102_FALL:
        if (e102_try_request(work, motion, record)) {
            break;
        }
        {
            struct e102 *own = record->own.e102.record[0];

            if (own->above < 100.0F && (own->ground_flags & WORLD_WATER)) {
                work->mode = E102_WATER_JUMP;
                record->anim.action = ACTION_WATER_START;
                break;
            }
        }
        if (work->flags & PLAYER_ON_GROUND) {
            sound_stop(SE_HOVER);
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = E102_STOP;
                record->anim.action = ACTION_STOP;
            } else {
                work->mode = E102_WALK;
                if (record->own.e102.record[0]->fall_speed < -2.0F) {
                    record->anim.action = ACTION_HARD_LANDING;
                    if (e102_in_play) {
                        sound_play_at(SE_THUD, 0, 0, 0, work->pos.x,
                                      work->pos.y, work->pos.z);
                    }
                    e102_landing_dust(work, motion);
                } else {
                    record->anim.action = ACTION_STOP;
                    if (e102_in_play) {
                        sound_play_at(SE_STEP, 0, 0, 0, work->pos.x,
                                      work->pos.y, work->pos.z);
                    }
                }
            }
        }
        break;
    case E102_STOP:
        if (e102_try_request(work, motion, record)) {
            break;
        }
        if (e102_try_fall(work, motion, record)) {
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            break;
        }
        if (record->speed.x <= 0.0F &&
            player_stick_back_or_end_event(work, motion)) {
            work->mode = E102_WALK;
            record->anim.action = ACTION_TURN;
            e102_turn(work, motion, record);
            work->ang.y = motion->aim.y;
            break;
        }
        if (record->anim.action != ACTION_STOP) {
            e102_stand(work, motion, record);
        }
        break;
    case E102_SKATE_STOP:
        if (e102_try_request(work, motion, record)) {
            break;
        }
        if (e102_try_fall(work, motion, record)) {
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            break;
        }
        if (record->speed.x <= 0.0F &&
            player_stick_back_or_end_event(work, motion)) {
            work->mode = E102_WALK;
            record->anim.action = ACTION_TURN;
            e102_turn(work, motion, record);
            work->ang.y = motion->aim.y;
            break;
        }
        if (record->anim.action != ACTION_SKATE_STOP) {
            work->mode = E102_STAND;
            record->anim.action = ACTION_SKATE_STOP_END;
        }
        break;
    case E102_PUSH:
        if (e102_try_request(work, motion, record) ||
            e102_try_jump(work, motion, record) ||
            e102_try_fall(work, motion, record) ||
            e102_try_stop(work, motion, record)) {
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
                    work->mode = E102_WALK;
                    record->anim.action = ACTION_WALK;
                    sound_stop(SE_PUSH);
                }
                break;
            default:
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = E102_PULL;
                    record->anim.action = ACTION_PULL;
                    if (e102_in_play) {
                        sound_play_at(SE_PULL, 0, 0, 0, work->pos.x,
                                      work->pos.y, work->pos.z);
                    }
                } else {
                    work->mode = E102_WALK;
                    record->anim.action = ACTION_WALK;
                }
                sound_stop(SE_PUSH);
                break;
            }
            break;
        }
        e102_let_go(work, motion, record);
        work->mode = E102_WALK;
        record->anim.action = ACTION_WALK;
        sound_stop(SE_PUSH);
        break;
    case E102_HURT:
        if (e102_try_request(work, motion, record)) {
            break;
        }
        /* not on the hit's own step, while work->timer is still
         * PLAYER_HURT_STEPS */
        if (TASK_TIMER(work) == PLAYER_HURT_STEPS) {
            break;
        }
        if (!(work->flags & PLAYER_ON_GROUND)) {
            break;
        }
        if (e102_try_stop(work, motion, record)) {
            e102_stand(work, motion, record);
            /* @hack redundant: a redundant break, which puts motion in R3 and
             * work in R2 for the else's stores (0x8c0d6dd0, 0x8c0d6dd2).
             * without it, the two trade and 4 of 4148 words differ */
            break;
        } else {
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            work->mode = E102_WALK;
            record->anim.action = ACTION_HURT_LAND;
        }
        break;
    case E102_BACK:
        if (e102_try_request(work, motion, record)) {
            break;
        }
        if (e102_try_fall(work, motion, record)) {
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            break;
        }
        if (e102_try_stop(work, motion, record)) {
            break;
        }
        if (record->speed.x >= 0.0F) {
            work->mode = E102_WALK;
            e102_set_walk_action(work, motion, record);
            /* @hack redundant: a redundant break, which branches to
             * E102_HOLD_FALL's call to e102_set_walk_action (BRA 0x8c0d7b10 at
             * 0x8c0d6e88). without it, this arm makes its own call, the code
             * after it moves, and 45 of 4148 words differ */
            break;
        }
        break;
    case E102_CLEAR:
        if (record->anim.action == ACTION_CLEAR &&
            record->anim.frame >= 53.0F) {
            struct e102 *own = record->own.e102.record[0];

            e102_fire_effects(task, record, own);
            work->mode = E102_POSE;
        }
        /* fall through */
    case E102_POSE:
        if (work->flags & PLAYER_HOLDING) {
            if (e102_try_request_holding(work, motion, record)) {
                break;
            }
        } else {
            if (e102_try_request(work, motion, record)) {
                break;
            }
        }
        break;
    case E102_CRASH:
        if (e102_try_fall(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_CRASH &&
            record->anim.action != ACTION_CRASH_END) {
            e102_stand(work, motion, record);
        }
        break;
    case E102_SLIDE:
        if (e102_try_request(work, motion, record)) {
            break;
        }
        if (e102_try_fall(work, motion, record)) {
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            break;
        }
        if (player_read_stick(work, NULL, NULL)) {
            work->mode = E102_WALK;
            record->anim.action = ACTION_WALK;
            break;
        }
        if (njScalor(&motion->speed) < record->consts.slide_speed ||
            record->contact->normal.y > 0.98F) {
            work->mode = E102_STAND;
            record->anim.action = ACTION_SLIDE_STOP;
        }
        break;
    case E102_LEDGE:
        if (e102_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (work->flags & PLAYER_ON_PATH) {
            Angle angle;

            if (!player_read_stick(work, &angle, NULL)) {
                break;
            }
            if (angle_distance(0xc000 - camera_work->ang.y, angle) < 0x4000) {
                record->speed.x = 0.1875F;
                /* @hack redundant: a redundant break, which gives this arm a
                 * store and jump of its own (0x8c0d70f8 to 0x8c0d7102). without
                 * it, the else's arm branches to them, and 31 of 4148 words
                 * differ */
                break;
            } else {
                /* @hack redundant: an else after an arm that breaks, which puts
                 * the record in R2, not R3, for this store and jump
                 * (0x8c0d7106 to 0x8c0d7110); without it, 10 of 4148 words
                 * differ */
                record->speed.x = -0.1875F;
                /* @hack redundant: a redundant break, which leaves the JMP's
                 * delay slot a NOP (0x8c0d7112). without it, the store fills
                 * the slot and 25 of 4148 words differ */
                break;
            }
            /* @hack redundant: a break after an if whose arms both break,
             * which puts work in R1 and E102_FALL in R2 for the fall's stores
             * (0x8c0d7114, 0x8c0d7116). without it, the two trade and 6 of
             * 4148 words differ */
            break;
        } else {
            /* @hack redundant: an else after an arm that breaks, which keeps
             * work in R1 and E102_FALL in R2 (0x8c0d7114, 0x8c0d7116). without
             * it, R2 and R3 take them, and 8 of 4148 words differ */
            work->mode = E102_FALL;
            record->anim.action = ACTION_FALL;
            player_clear_speed(motion, record);
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case E102_LEDGE_GO:
        if (e102_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (work->flags & PLAYER_ON_PATH) {
            /* @hack spelling: an empty arm, which puts work in R2 and E102_FALL
             * in R1 for the else's stores (0x8c0d71cc, 0x8c0d71ce). tested as
             * if (!...), the two trade and 6 of 4148 words differ */
        } else {
            work->mode = E102_FALL;
            record->anim.action = ACTION_FALL;
            player_clear_speed(motion, record);
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case E102_TIME_UP:
        if (record->anim.action == ACTION_DOWN && record->anim.frame >= 78.0F) {
            /* @hack type: the player cast to Sint8, as MOV.B loads it, which
             * passes it on as loaded (MOV R0,R4 at 0x8c0d724e). as the Uint8
             * it is, it takes an EXTU.B there, and 1 of 4148 words differs */
            player_down((Sint8)PLAYER_VARS(work)->player);
            work->mode = E102_DOWN;
        }
        break;
    case E102_LIFT:
        if (work->flags & PLAYER_HOLDING) {
            if (e102_try_request_holding(work, motion, record)) {
                break;
            }
        } else {
            if (e102_try_request(work, motion, record)) {
                break;
            }
        }
        if (e102_try_fall(work, motion, record)) {
            break;
        }
        if (e102_try_take_lifted(work, motion, record)) {
            break;
        }
        break;
    case E102_GRIP:
        if (e102_try_grip(work, motion, record)) {
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            break;
        }
        break;
    case E102_HOLD_STAND:
        if (e102_try_request_holding(work, motion, record)) {
            break;
        }
        if (e102_try_fall(work, motion, record)) {
            break;
        }
        if (e102_try_throw(work, motion, record)) {
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                        pad_grab_buttons) &&
            player_read_stick(work, NULL, NULL) &&
            record->held->work->id == CARRY_KIND_SMALL) {
            work->mode = E102_HOLD_SHAKE;
            record->anim.action = ACTION_HOLD_SHAKE_START;
            break;
        }
        if (e102_try_throw(work, motion, record)) {
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            break;
        }
        if (player_read_stick(work, NULL, NULL)) {
            work->mode = E102_HOLD_WALK;
            record->anim.action = ACTION_HOLD_WALK;
        }
        break;
    case E102_HOLD_WALK:
        if (e102_try_request_holding(work, motion, record)) {
            break;
        }
        if (e102_try_fall(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (e102_try_throw(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (e102_try_stop(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (player_stick_back_or_end_event(work, motion) &&
            record->speed.x >= record->consts.trot_speed) {
            work->mode = E102_HOLD_STOP;
            record->anim.action = ACTION_HOLD_STOP;
            if (e102_in_play) {
                sound_play_at(SE_BRAKE, 0, 0, 0, work->pos.x, work->pos.y,
                              work->pos.z);
            }
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (record->speed.x >= record->consts.sprint_speed) {
            record->speed.x = record->consts.sprint_speed;
            record->own.e102.skate_steps++;
            if (record->own.e102.skate_steps > STEPS_TO_SKATE) {
                record->own.e102.skate_steps = 0;
                work->mode = E102_HOLD_SKATE;
                record->anim.action = ACTION_HOLD_SKATE_START;
                break;
            }
        } else {
            record->own.e102.skate_steps = 0;
        }
        if ((record->ground_flags & WORLD_WATER) &&
            record->anim.action != ACTION_WATER_START) {
            work->mode = E102_HOLD_WATER_SKATE;
            record->anim.action = ACTION_HOLD_WATER_START;
        }
        break;
    case E102_HOLD_SKATE:
        if (e102_try_request_holding(work, motion, record)) {
            break;
        }
        if (e102_try_fall(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (e102_try_throw(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (e102_try_stop(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (player_stick_back_or_end_event(work, motion) &&
            record->speed.x >= record->consts.trot_speed) {
            work->mode = E102_HOLD_SKATE_STOP;
            record->anim.action = ACTION_HOLD_STOP;
            if (e102_in_play) {
                sound_play_at(SE_BRAKE, 0, 0, 0, work->pos.x, work->pos.y,
                              work->pos.z);
            }
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if ((record->ground_flags & WORLD_WATER) &&
            record->anim.action == ACTION_HOLD_SKATE) {
            work->mode = E102_HOLD_WATER_SKATE;
            record->anim.action = ACTION_HOLD_SKATE_TO_WATER;
        }
        break;
    case E102_HOLD_WATER_SKATE:
        if (e102_try_request_holding(work, motion, record)) {
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (!(record->ground_flags & WORLD_WATER) &&
            record->anim.action == ACTION_HOLD_WATER_SKATE) {
            record->own.e102.skate_steps++;
            if (record->own.e102.skate_steps > STEPS_OFF_WATER) {
                record->own.e102.skate_steps = 0;
                work->mode = E102_HOLD_SKATE;
                record->anim.action = ACTION_HOLD_WATER_TO_SKATE;
                break;
            }
        } else {
            record->own.e102.skate_steps = 0;
        }
        break;
    case E102_PUT_DOWN:
    case E102_THROW:
        if (work->flags & PLAYER_HOLDING) {
            if (e102_try_request_holding(work, motion, record)) {
                break;
            }
        } else {
            if (e102_try_request(work, motion, record)) {
                break;
            }
        }
        if (e102_try_release(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_THROW_AT &&
            record->anim.action != ACTION_THROW &&
            record->anim.action != ACTION_PUT_DOWN) {
            e102_stand(work, motion, record);
        }
        break;
    case E102_HOLD_STOP:
        if (e102_try_request_holding(work, motion, record)) {
            break;
        }
        if (e102_try_fall(work, motion, record)) {
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            break;
        }
        if (record->speed.x <= 0.0F &&
            player_stick_back_or_end_event(work, motion)) {
            work->mode = E102_HOLD_WALK;
            record->anim.action = ACTION_HOLD_TURN;
            e102_turn(work, motion, record);
            work->ang.y = motion->aim.y;
            break;
        }
        if (record->anim.action != ACTION_HOLD_STOP) {
            work->mode = E102_HOLD_STAND;
            record->anim.action = ACTION_HOLD_SKATE_END;
        }
        break;
    case E102_HOLD_SKATE_STOP:
        if (e102_try_request_holding(work, motion, record)) {
            break;
        }
        if (e102_try_fall(work, motion, record)) {
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            break;
        }
        if (record->speed.x <= 0.0F &&
            player_stick_back_or_end_event(work, motion)) {
            work->mode = E102_HOLD_SKATE;
            record->anim.action = ACTION_HOLD_SKATE_TURN;
            e102_turn(work, motion, record);
            work->ang.y = motion->aim.y;
            break;
        }
        if (record->anim.action != ACTION_HOLD_STOP) {
            work->mode = E102_HOLD_STAND;
            record->anim.action = ACTION_HOLD_SKATE_END;
        }
        break;
    case E102_HOLD_FALL:
        if (e102_try_request_holding(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            sound_stop(SE_HOVER);
            if (record->ground_flags & WORLD_WATER) {
                e102_set_walk_action(work, motion, record);
            } else if (player_stick_back_or_end_event(work, motion) &&
                       record->speed.x > 0.0F) {
                work->mode = E102_HOLD_STOP;
                record->anim.action = ACTION_HOLD_STOP;
                if (e102_in_play) {
                    sound_play_at(SE_STEP, 0, 0, 0, work->pos.x, work->pos.y,
                                  work->pos.z);
                }
            } else if (e102_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (motion->speed.y > -5.0F) {
                    record->anim.action = ACTION_HOLD_STAND;
                    if (e102_in_play) {
                        sound_play_at(SE_STEP, 0, 0, 0, work->pos.x,
                                      work->pos.y, work->pos.z);
                    }
                } else {
                    record->anim.action = ACTION_HOLD_HARD_LANDING;
                    if (e102_in_play) {
                        sound_play_at(SE_THUD, 0, 0, 0, work->pos.x,
                                      work->pos.y, work->pos.z);
                    }
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = E102_HOLD_WALK;
                record->anim.action = ACTION_HOLD_WALK;
                if (e102_in_play) {
                    sound_play_at(SE_STEP, 0, 0, 0, work->pos.x, work->pos.y,
                                  work->pos.z);
                }
            }
            break;
        }
        if (e102_try_throw(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        break;
    case E102_HOLD_JUMP:
        if (e102_try_request_holding(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = E102_HOLD_STOP;
                record->anim.action = ACTION_HOLD_STOP;
            } else if (e102_try_stop(work, motion, record)) {
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
                work->mode = E102_HOLD_WALK;
                e102_set_walk_action(work, motion, record);
            }
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.4F;
        }
        if (e102_try_throw(work, motion, record)) {
            break;
        }
        if (motion->speed.y <= 0.0F) {
            work->mode = E102_HOLD_FALL;
            record->anim.action = ACTION_HOLD_FALL_START;
        }
        break;
    case E102_HOLD_SKATE_JUMP:
        if (e102_try_request_holding(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            work->mode = E102_HOLD_SKATE;
            break;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.4F;
        }
        if (e102_try_throw(work, motion, record)) {
            break;
        }
        break;
    case E102_HOLD_WATER_JUMP:
        if (e102_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            struct e102 *own = record->own.e102.record[0];

            if (own->ground_flags & WORLD_WATER) {
                work->mode = E102_HOLD_WATER_SKATE;
                e102_skate_sound_off(work, own);
            } else {
                work->mode = E102_HOLD_SKATE;
                record->anim.action = ACTION_WATER_TO_SKATE;
                e102_skate_sound_on(work, own);
            }
            break;
        }
        if (e102_try_halt(work, motion, record)) {
            break;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.8F;
        }
        break;
    case E102_HOLD_SPRING:
        if (motion->speed.y <= 0.0F) {
            work->mode = E102_HOLD_FALL;
            record->anim.action = ACTION_HOLD_FALL_START;
        }
        break;
    case E102_HOLD_LAUNCH:
        if (motion->speed.y <= 0.0F) {
            work->mode = E102_HOLD_FALL;
            record->anim.action = ACTION_HOLD_FALL_START;
        }
        break;
    case E102_PULL:
        if (e102_try_request(work, motion, record) ||
            e102_try_jump(work, motion, record) ||
            e102_try_fall(work, motion, record)) {
            sound_stop(SE_PUSH);
            break;
        }
        if (!PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                         pad_grab_buttons)) {
            e102_stand(work, motion, record);
            e102_let_go(work, motion, record);
            sound_stop(SE_PUSH);
        }
        if ((work->collision->flags & COLLISION_PUSHED) &&
            !(work->flags & PLAYER_HURT) &&
            !(work->collision->hit->shapes[work->collision->hit_shape].flags &
              SHAPE_NO_PUSH) &&
            (work->collision->flags & COLLISION_HIT_SIDE)) {
            switch (player_stick_into_wall(work, motion)) {
            case PLAYER_WALL_HEAD_ON:
                work->mode = E102_PUSH;
                record->anim.action = ACTION_PUSH_START;
                break;
            case PLAYER_WALL_NO_STICK:
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = E102_GRIP;
                    record->anim.action = ACTION_GRIP;
                    sound_stop(SE_PUSH);
                }
                break;
            default:
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = E102_PULL;
                    record->anim.action = ACTION_PULL;
                    if (e102_in_play) {
                        sound_play_at(SE_PULL, 0, 0, 0, work->pos.x,
                                      work->pos.y, work->pos.z);
                    }
                } else {
                    work->mode = E102_WALK;
                    record->anim.action = ACTION_WALK;
                }
                sound_stop(SE_PUSH);
                break;
            }
        } else {
            e102_let_go(work, motion, record);
            work->mode = E102_WALK;
            record->anim.action = ACTION_WALK;
            sound_stop(SE_PUSH);
            /* @hack redundant: a redundant break, which makes this arm's
             * sound_stop the one that E102_PUSH's let-go and the first tests
             * of E102_PUSH and E102_PULL jump to (0x8c0d81b8). without it,
             * E102_PULL's first test holds that call, the code after it
             * moves, and 797 of 4148 words differ */
            break;
        }
        break;
    case E102_AIR_THROW:
        if (work->flags & PLAYER_HOLDING) {
            if (e102_try_request_holding(work, motion, record)) {
                break;
            }
        } else {
            if (e102_try_request(work, motion, record)) {
                break;
            }
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = E102_HOLD_STOP;
                record->anim.action = ACTION_HOLD_STOP;
            } else if (e102_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (work->flags & PLAYER_HOLDING) {
                    if (motion->speed.y > -5.0F) {
                        record->anim.action = ACTION_HOLD_STAND;
                        if (e102_in_play) {
                            sound_play_at(SE_STEP, 0, 0, 0, work->pos.x,
                                          work->pos.y, work->pos.z);
                        }
                    } else {
                        record->anim.action = ACTION_HOLD_HARD_LANDING;
                        if (e102_in_play) {
                            sound_play_at(SE_THUD, 0, 0, 0, work->pos.x,
                                          work->pos.y, work->pos.z);
                        }
                    }
                } else {
                    if (motion->speed.y > -5.0F) {
                        e102_set_stand_action(work, motion, record);
                        if (e102_in_play) {
                            sound_play_at(SE_STEP, 0, 0, 0, work->pos.x,
                                          work->pos.y, work->pos.z);
                        }
                    } else {
                        record->anim.action = ACTION_HARD_LANDING;
                        if (e102_in_play) {
                            sound_play_at(SE_THUD, 0, 0, 0, work->pos.x,
                                          work->pos.y, work->pos.z);
                        }
                    }
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = E102_HOLD_WALK;
                } else {
                    work->mode = E102_WALK;
                }
                e102_set_walk_action(work, motion, record);
            }
            break;
        }
        if (e102_try_release(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_AIR_THROW_AT &&
            record->anim.action != ACTION_AIR_THROW) {
            work->mode = E102_FALL;
            record->anim.action = ACTION_FALL;
        }
        break;
    case E102_SHAKE:
        if (e102_try_request_holding(work, motion, record)) {
            break;
        }
        if (e102_try_fall(work, motion, record)) {
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            break;
        }
        if (player_touches_shakable(work, motion, record)) {
            if (!player_read_stick(work, NULL, NULL)) {
                work->mode = E102_GRIP;
                record->anim.action = ACTION_GRIP;
            }
            break;
        }
        e102_stand(work, motion, record);
        e102_let_go(work, motion, record);
        break;
    case E102_HOLD_SHAKE:
        if (e102_try_request_holding(work, motion, record)) {
            break;
        }
        if (e102_try_fall(work, motion, record)) {
            break;
        }
        if (e102_try_jump(work, motion, record)) {
            break;
        }
        if (record->held == NULL ||
            record->held->work->id != CARRY_KIND_SMALL) {
            e102_stand(work, motion, record);
            e102_let_go(work, motion, record);
            break;
        }
        if (!player_read_stick(work, NULL, NULL)) {
            work->mode = E102_HOLD_STAND;
            record->anim.action = ACTION_HOLD_SHAKE_END;
        }
        break;
    case E102_LEAP:
        break;
    case E102_LEAP_FALL:
        if (e102_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            sound_stop(SE_HOVER);
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = E102_STOP;
                record->anim.action = ACTION_STOP;
            } else if (e102_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                record->anim.action = ACTION_STOP;
                if (e102_in_play) {
                    sound_play_at(SE_STEP, 0, 0, 0, work->pos.x, work->pos.y,
                                  work->pos.z);
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = E102_WALK;
                record->anim.action = ACTION_HARD_LANDING;
                if (e102_in_play) {
                    sound_play_at(SE_THUD, 0, 0, 0, work->pos.x, work->pos.y,
                                  work->pos.z);
                }
                e102_landing_dust(work, motion);
            }
        }
        break;
    case E102_MODULE_POSE:
        if ((work->flags & PLAYER_REQUEST) &&
            work->submode == PLAYER_MODE_RIDE_OFF) {
            work->mode = E102_SPRING;
            record->anim.action = ACTION_JUMP;
            work->flags &= ~PLAYER_ATTACKING;
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a redundant break, which puts work in R3 and its
             * event in R2 for E102_EVENT's test (0x8c0d8706, 0x8c0d870a).
             * without it, the two trade and 3 of 4148 words differ */
            break;
        }
        break;
    case E102_EVENT:
        if (work->event->paths == NULL) {
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            e102_stand(work, motion, record);
        }
    }
}

static void e102_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;
    struct e102 *e102 = record->own.e102.record[0];

    e102_free_strips(&e102->inner, &e102->outer);
    if (e102->search != NULL) {
        task_destroy(e102->search);
    }
    heap_free(e102);
    record->anim.action = ACTION_STAND;
    player_free(PLAYER_VARS(work)->player);
    texture_release_character(CHARACTER_E102);
    sound_stop_id((int)work);
    e102_hit_free();
}

/* draw his model, his cannon, his booster and his shot's arm. the draw also
 * places his shapes and turns his cannon. it then finds his muzzle and his aim,
 * where a shot leaves and the way it goes, and his height. a step he blinks out
 * on does none of this */
static void e102_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;
    struct e102 *e102 = record->own.e102.record[0];
    NJS_VECTOR base;
    NJS_VECTOR offset;
    NJS_VECTOR center;
    int action;
    /* @hack spelling: njPushMatrix and njCalcVector called through locals,
     * which SHC keeps in R11 and R8 for the whole draw, loaded where the locals
     * are set (0x8c0d888c, 0x8c0d88de). called by name, njPushMatrix takes R10,
     * njCalcVector loads for each call, and 365 of 862 words differ */
    Bool (*push)(NJS_MATRIX *matrix);
    void (*calc_vector)(NJS_MATRIX *matrix, NJS_VECTOR *from, NJS_VECTOR *to);
#ifdef GCC
    /* the split frame's second picture draws from a copy of his own record,
     * so that the cannon's turn, the draws left to aim and the phases of his
     * glow and bob stay as his steps left them */
    struct e102 copy;

    if (frame_redrawing) {
        copy = *e102;
        e102 = &copy;
    }
#endif

    if (record->upgrades & PLAYER_E102_LASER) {
        e102_gun_mount->sibling = &e102_laser_gun;
        e102_shot_gun_mount.sibling = &e102_shot_laser_gun;
    } else {
        e102_gun_mount->sibling = &e102_plain_gun;
        e102_shot_gun_mount.sibling = &e102_shot_plain_gun;
    }
    e102_welds[0].object_a = e102_gun_mount->sibling->child;
    draw_set_palette(DRAW_PLAYER_PALETTE);
    action = record->anim.action;
    if (record->anim.state == PLAYER_ANIM_BLENDING) {
        action = record->anim.current;
    }
    /* he blinks as work->timer counts down from a hit: he's drawn two steps
     * in four */
    if (!(TASK_TIMER(work) & 2)) {
        struct collision_shape *shape;
        Float scale;
        /* @hack spelling: the lift through a local, which leaves njTranslate's
         * NULL to the JSR's delay slot (MOV #0,R4 at 0x8c0d8adc). passed
         * straight, SHC sets it before the call and 5 of 862 words differ */
        Float lift;
        Angle target;
        int turn;
        int alpha;

        njSetTexture(&e102_texlist);
        draw_attr_save();
        draw_attr_clear(0, NJD_FLAG_IGNORE_SPECULAR);
        push = njPushMatrix;
        push(NULL);
        offset.z = 0.0F;
        offset.x = 0.0F;
        offset.y = record->consts.center_y;
        push(&_unit_matrix_);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, -work->ang.y);
        calc_vector = njCalcVector;
        calc_vector(NULL, &offset, &center);
        njPopMatrix(1);
        base = center;
        center.x += work->pos.x;
        center.y += work->pos.y;
        center.z += work->pos.z;
        njTranslateV(NULL, &center);
        shape = work->collision->shapes;
        shape->center = center;
        shape++;
        shape->center.x = center.x;
        shape->center.y = work->pos.y + 5.0F;
        shape->center.z = center.z;
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, 0x8000 - work->ang.y);
        /* skating on water, he bobs, and leans with his speed */
        if (record->anim.action == ACTION_WATER_SKATE ||
            record->anim.action == ACTION_HOLD_WATER_SKATE) {
            scale =
                CLAMP(_builtin_fabsf(record->speed.x + record->speed.z) - 0.8F,
                      0.0F, 2.0F);
            e102->bob += (Angle)(scale * 1536.0F) + 0x600;
            lift = njSin(e102->bob) * 1.8F;
            njTranslate(NULL, 0.0F, lift, 0.0F);
            TURN(njRotateZ, (Angle)(scale * 2048.0F));
        } else {
            e102->bob = 0;
        }
        if (work->event->motions != NULL) {
            event_motions_draw(work);
            pose_get_matrix(54, &cannon_matrix);
            pose_get_matrix(30, &booster_matrix);
        } else {
            njSetMotionCallback(e102_pose_node);
            if (record->anim.state != PLAYER_ANIM_BLENDING) {
                draw_action(record->anim.actions[action].action,
                            record->anim.frame);
            } else {
                draw_action(record->anim.blend, record->anim.frame);
            }
            njSetMotionCallback(NULL);
        }
        push(NULL);
        njTranslate(NULL, 0.0F, -(e102_current->kick * 2.0F), 0.0F);
        njMultiMatrix(NULL, &cannon_matrix);
        TURN(njRotateY, e102->cannon_ang);
        draw_action(&e102_cannon_action, e102->cannon_frame);
        pose_action(&_unit_matrix_, &e102_cannon_action, e102->cannon_frame, 2,
                    &cannon_end_matrix);
        njPopMatrix(1);
        if (record->upgrades & PLAYER_E102_BOOSTER) {
            push(NULL);
            njMultiMatrix(NULL, &booster_matrix);
            njRotateZ(NULL, 0x8000);
            njTranslate(NULL, 5.0F, e102->booster_lift + 5.0F, 0.0F);
            TURN(njRotateZ, e102->booster_ang_z);
            TURN(njRotateX, e102->booster_ang_x);
            draw_tree(&e102_booster);
            if (e102_hovering) {
                njFogDisable();
                njSetTexture(&e102effect_texlist);
                e102_flame->model->mats->attr_texId =
                    ((e102->steps >> 1) & 3) + 8;
                draw_tree(e102_flame);
                njSetTexture(&e102_texlist);
                njFogEnable();
                njColorBlendingMode(NJD_SOURCE_COLOR,
                                    NJD_COLOR_BLENDING_SRCALPHA);
                njColorBlendingMode(NJD_DESTINATION_COLOR,
                                    NJD_COLOR_BLENDING_INVSRCALPHA);
            }
            njPopMatrix(1);
        }
        if ((e102->shot_phase >= E102_SHOT_MARK &&
             e102->shot_phase <= E102_SHOT_KICK) ||
            (e102->aim_mode >= E102_AIM_START &&
             e102->aim_mode <= E102_AIM_LOCKING)) {
            evalflags_set_all(e102_arm_objects, NJD_EVAL_HIDE);
            push(NULL);
            njTranslate(NULL, 0.0F, -(e102_current->kick * 2.0F), 0.0F);
            njMultiMatrix(NULL, &arm_matrix);
            draw_action(e102->shot_action, e102->shot_frame);
            njPopMatrix(1);
            pose_action(&_unit_matrix_, e102->shot_action, e102->shot_frame, 6,
                        &shot_matrix);
        } else {
            evalflags_clear_all(e102_arm_objects, NJD_EVAL_HIDE);
        }
        push(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        njTranslateV(NULL, &base);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, 0x8000 - work->ang.y);
        push(NULL);
        njMultiMatrix(NULL, &node_2_matrix);
        e102_node_point.x = 0.0F;
        e102_node_point_z = 6.0F;
        njCalcPoint(NULL, (NJS_POINT3 *)&e102_node_point, &e102->points[0]);
        e102_node_point_z = -e102_node_point_z;
        njCalcPoint(NULL, (NJS_POINT3 *)&e102_node_point, &e102->points[1]);
        e102_node_point_z = 0.0F;
        njPopMatrix(1);
        push(NULL);
        /* locking on, the cannon snaps to the target, or turns to the
         * stick, else back to center, at most 8 degrees a draw */
        if (e102->aim_mode == E102_AIM_LOCKING) {
            if (e102->aim_draws > 0) {
                struct task_work *goal = e102->target->work;
                /* @hack spelling: the differences through dx and dz, which keep
                 * the target's work in R4 while atan2f's arguments form (MOV.L
                 * @(32,R4),R4 at 0x8c0d8eb0). passed straight, SHC keeps it in
                 * the frame, and 23 of 862 words differ */
                Float dx = goal->pos.x - e102->muzzle.x;
                Float dz = goal->pos.z - e102->muzzle.z;

                target =
                    work->ang.y - (Angle)(atan2f(dz, dx) * ANGLE_PER_RADIAN);
                e102->cannon_ang = target;
            } else if (pad_sticks[PLAYER_VARS(work)->player].stroke != 0.0F) {
                target =
                    work->ang.y - pad_sticks[PLAYER_VARS(work)->player].ang;
            } else {
                target = 0;
            }
        } else {
            target = 0;
        }
        turn = angle_difference(e102->cannon_ang, target);
        e102->cannon_ang += CLAMP(turn, -NJM_DEG_ANG(8), NJM_DEG_ANG(8));
        njMultiMatrix(NULL, &cannon_matrix);
        TURN(njRotateY, e102->cannon_ang);
        njMultiMatrix(NULL, &cannon_end_matrix);
        njCalcPoint(NULL, (NJS_POINT3 *)&e102_node_point, &e102->muzzle);
        /* @hack spelling: the matrix calls in both arms, which SHC joins.
         * written once after the if, they leave SHC loading glow again in
         * njSin's delay slot, where the image copies the sum first (MOV R2,R4
         * at 0x8c0d90d4). the function then runs 2 bytes short and 67 of 862
         * words differ */
        if (e102->aim_draws > 0) {
            e102->aim = e102->target->work->pos;
            njSubVector(&e102->aim, &e102->muzzle);
            njUnitVector(&e102->aim);
            e102->aim_draws--;
            njPopMatrix(1);
            push(NULL);
        } else {
            e102_node_point.x = -1.0F;
            calc_vector(NULL, (NJS_POINT3 *)&e102_node_point, &e102->aim);
            njPopMatrix(1);
            push(NULL);
        }
        njMultiMatrix(NULL, &shot_matrix);
        e102_node_point.x = 0.0F;
        njCalcPoint(NULL, (NJS_POINT3 *)&e102_node_point, &e102->shot_pos);
        e102_node_point.x = 1.0F;
        calc_vector(NULL, (NJS_POINT3 *)&e102_node_point, &e102->shot_dir);
        njUnitVector(&e102->shot_dir);
        njPopMatrix(1);
        njPopMatrix(1);
        push(&cannon_matrix);
        offset = e102_zero;
        njCalcPoint(NULL, &offset, &center);
        record->consts.height = e102_height_above_cannon + center.y;
        njPopMatrix(1);
        njPopMatrix(1);
        e102->glow += 0x400;
        alpha = _builtin_fabsf(njSin(e102->glow) * 255.0F);
        e102_head_materials[GLOW_MATERIAL].diffuse.argb.a = alpha;
        /* the materials that fade go from opaque to clear over frames 40 to
         * 80 of ACTION_DOWN */
        /* @hack spelling: the two tests nested, which form the difference in
         * FR1, with 80.0F in FR0 and 6.375F in FR3 (0x8c0d9106 to
         * 0x8c0d9112). joined with &&, the three take FR3, FR2 and FR1, and 7
         * of 862 words differ */
        if (record->anim.action == ACTION_DOWN) {
            if (record->anim.frame >= 40.0F) {
                /* @hack spelling: the product through scale, which SHC forms in
                 * a copy in FR4 (FMOV FR1,FR4 at 0x8c0d911c). stored straight
                 * to alpha, it forms in FR1, the function runs 2 bytes short,
                 * and 33 of 862 words differ */
                scale = (80.0F - record->anim.frame) * 6.375F;
                alpha = scale;
                e102_head_materials[GLOW_MATERIAL].diffuse.argb.a = alpha;
                e102_eye_plus_z_materials[FADE_MATERIAL].diffuse.argb.a = alpha;
                e102_eye_minus_z_materials[FADE_MATERIAL].diffuse.argb.a =
                    alpha;
                e102_toe_plus_z_materials[FADE_MATERIAL].diffuse.argb.a = alpha;
                e102_toe_minus_z_materials[FADE_MATERIAL].diffuse.argb.a =
                    alpha;
            }
        }
        draw_attr_restore();
    }
    draw_set_palette(0);
}

/* a hit, or ground that hurts, hurts him unless he's in E102_MODULE_POSE. he's
 * knocked back, lockon_number goes back to 0, and work->timer starts at
 * PLAYER_HURT_STEPS. TRUE if he's hurt */
static Bool e102_try_hurt(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    if (record != NULL &&
        ((work->flags & PLAYER_HURT) || (record->ground_flags & WORLD_HURTS)) &&
        work->mode != E102_MODULE_POSE) {
        struct e102 *e102 = record->own.e102.record[0];

        work->mode = E102_HURT;
        record->anim.action = ACTION_HURT;
        work->flags &= ~PLAYER_HURT;
        record->speed.y = record->consts.jump_speed;
        record->speed.x = -record->consts.running_speed;
        work->timer = PLAYER_HURT_STEPS;
        sound_play(SE_HURT, 0, 0, 0);
        vibration_start_fading(PLAYER_VARS(work)->player, 7, 59, 6);
        e102->lockon_number = 0;
        e102->unk_0c = 0;
        return TRUE;
    }
    return FALSE;
}

/* he jumps as the button's pressed. what he carries goes with him if it
 * can, or he lets it go. TRUE if he jumps */
static Bool e102_try_jump(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    struct e102 *e102 = record->own.e102.record[0];

    if (player_jump_pressed(work, record)) {
        if (work->flags & PLAYER_HOLDING) {
            int flags = player_carry_flags(work);

            if (flags & PLAYER_CAN_CARRY) {
                if (work->mode == E102_HOLD_WATER_SKATE) {
                    work->mode = E102_HOLD_WATER_JUMP;
                } else {
                    work->mode = E102_HOLD_JUMP;
                    record->anim.action = ACTION_HOLD_JUMP_START;
                    e102_skate_sound_off(work, e102);
                }
                record->speed.y = record->consts.jump_speed;
                work->flags &= ~PLAYER_ON_TASK;
                record->jump_steps = 0;
                if (e102_in_play) {
                    sound_play_at(SE_E102_JUMP, 0, 0, 0, work->pos.x,
                                  work->pos.y, work->pos.z);
                }
                return TRUE;
            } else {
                e102_let_go(work, motion, record);
            }
        }

        if (work->mode == E102_WATER_SKATE) {
            work->mode = E102_WATER_JUMP;
        } else {
            work->mode = E102_JUMP;
            record->anim.action = ACTION_JUMP;
        }
        record->speed.y = record->consts.jump_speed;
        if (record->ground_task != NULL) {
            struct task_force *force = record->ground_task->force;

            if (force != NULL) {
                player_set_accel(PLAYER_VARS(work)->player, force->speed.x,
                                 force->speed.y, force->speed.z);
            }
        }
        work->flags &= ~PLAYER_ON_TASK;
        record->jump_steps = 0;
        e102_skate_sound_off(work, e102);
        if (e102_in_play) {
            sound_play_at(SE_E102_JUMP, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        return TRUE;
    }
    return FALSE;
}

/* jump pressed in the air stops him where he is, to fall or hover. TRUE if
 * it's pressed */
static Bool e102_try_halt(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    if (pad_records[PLAYER_VARS(work)->player].press & pad_jump_buttons) {
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        return TRUE;
    }
    return FALSE;
}

/* what his event's action asks: to drive him, or to turn him. FALSE when
 * it asks for nothing he knows */
static Bool e102_follow_event(struct task_work *work,
                              struct task_motion *motion, struct player *record)
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

/* his event's action, then its paths, then a PLAYER_REQUEST submode. TRUE
 * ends the caller's checks. FALSE can still have changed him */
static Bool e102_try_request(struct task_work *work, struct task_motion *motion,
                             struct player *record)
{
    if (work->event->action) {
        e102_follow_event(work, motion, record);
        return FALSE;
    }
    if (work->event->paths != NULL) {
        work->mode = E102_EVENT;
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        return TRUE;
    }
    if (!(work->flags & PLAYER_REQUEST)) {
        return FALSE;
    }
    switch (work->submode) {
    case PLAYER_MODE_SPRING:
    case PLAYER_MODE_BOUNCE_1:
    case PLAYER_MODE_BOUNCE_2:
    case PLAYER_MODE_BOUNCE_3:
        work->mode = E102_SPRING;
        record->anim.action = ACTION_JUMP;
        work->ang.x = work->ang.z = 0;
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_LAUNCH:
        work->mode = E102_LAUNCH;
        record->anim.action = ACTION_JUMP;
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_UPDRAFT:
        work->mode = E102_FALL;
        record->anim.action = ACTION_UPDRAFT;
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_PATH:
    case PLAYER_MODE_PATH_SPIN:
        work->mode = E102_PATH;
        record->anim.action = ACTION_SKATE_START;
        record->path_distance = 0.0F;
        work->flags &= ~PLAYER_GUARD;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_SQUASH_2:
    case PLAYER_MODE_SQUASH_3:
    case PLAYER_MODE_STILL:
    case PLAYER_MODE_BALL:
        work->mode = E102_POSE;
        record->anim.action = ACTION_STAND;
        work->ang.x = motion->aim.x;
        work->ang.z = motion->aim.z;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL_PUSH:
        work->mode = E102_POSE;
        record->anim.action = ACTION_PUSH_START;
        work->ang.x = motion->aim.x;
        work->ang.z = motion->aim.z;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RESET:
    case PLAYER_MODE_LAUNCH_SPEED:
        e102_stand(work, motion, record);
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_JOIN_PATH:
        if (work->mode == E102_WALK ||
            record->speed.x > record->consts.running_speed) {
            record->path_distance = 0.0F;
            work->flags &= ~PLAYER_GUARD;
            work->flags |= PLAYER_ON_PATH;
        }
        return FALSE;
    case PLAYER_MODE_LEDGE:
        if (work->flags & PLAYER_ON_GROUND) {
            return FALSE;
        }
        if (e102_in_play) {
            sound_play_at(SE_GRAB, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        work->mode = E102_LEDGE;
        record->anim.action = ACTION_LEDGE;
        record->anim.frame = 0.0F;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RIDE:
        work->mode = E102_MODULE_POSE;
        record->anim.action = ACTION_RIDE;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_CLEAR:
        work->mode = E102_CLEAR;
        record->anim.action = ACTION_CLEAR;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_DOWN:
        if (work->mode == E102_DOWN) {
            return TRUE;
        }
        switch (work->mode) {
        case E102_SKATE:
            record->anim.action = ACTION_SKATE_DOWN;
            break;
        case E102_WATER_SKATE:
            record->anim.action = ACTION_WATER_DOWN;
            break;
        default:
            record->anim.action = ACTION_DOWN;
            break;
        }
        work->mode = E102_DOWN;
        break;
    case PLAYER_MODE_HOLD:
        work->mode = E102_HOLD_STAND;
        record->anim.action = ACTION_HOLD_STAND;
        record->held = PLAYER_VARS(work)->arg.carry;
        record->held->work->flags |= CARRY_HELD;
        work->flags |= PLAYER_HOLDING;
        work->flags &= ~COLLISION_TOUCHING;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/* e102_try_request while he carries something, without the event's
 * paths. some requests make him let go of it */
static Bool e102_try_request_holding(struct task_work *work,
                                     struct task_motion *motion,
                                     struct player *record)
{
    if (work->event->action) {
        e102_follow_event(work, motion, record);
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
        work->mode = E102_HOLD_SPRING;
        record->anim.action = ACTION_HOLD_JUMP;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_LAUNCH:
        work->mode = E102_HOLD_LAUNCH;
        record->anim.action = ACTION_HOLD_JUMP;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_UPDRAFT:
        work->mode = E102_HOLD_FALL;
        record->anim.action = ACTION_HOLD_FALL;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_PATH:
        e102_let_go(work, motion, record);
        work->mode = E102_PATH;
        record->anim.action = ACTION_SKATE_START;
        record->path_distance = 0.0F;
        work->flags &= ~PLAYER_GUARD;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_SQUASH_2:
    case PLAYER_MODE_SQUASH_3:
    case PLAYER_MODE_STILL:
        work->mode = E102_POSE;
        record->anim.action = ACTION_HOLD_STAND;
        work->ang.x = motion->aim.x;
        work->ang.z = motion->aim.z;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_BALL:
    case PLAYER_MODE_STILL_PUSH:
        e102_let_go(work, motion, record);
        work->mode = E102_POSE;
        record->anim.action = ACTION_PUSH_START;
        work->ang.x = motion->aim.x;
        work->ang.z = motion->aim.z;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RESET:
        work->mode = E102_HOLD_STAND;
        record->anim.action = ACTION_HOLD_STAND;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_JOIN_PATH:
        if (work->mode == E102_HOLD_WALK ||
            record->speed.x > record->consts.running_speed) {
            record->path_distance = 0.0F;
            work->flags |= PLAYER_ON_PATH;
        }
        return FALSE;
    case PLAYER_MODE_LEDGE:
        if (work->flags & PLAYER_ON_GROUND) {
            return FALSE;
        }
        e102_let_go(work, motion, record);
        if (e102_in_play) {
            sound_play_at(SE_GRAB, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        work->mode = E102_LEDGE;
        record->anim.action = ACTION_LEDGE;
        record->anim.frame = 0.0F;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RIDE:
        work->mode = E102_MODULE_POSE;
        record->anim.action = ACTION_RIDE;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_CLEAR:
        e102_let_go(work, motion, record);
        work->mode = E102_CLEAR;
        record->anim.action = ACTION_CLEAR;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_DOWN:
        work->mode = E102_DOWN;
        record->anim.action = ACTION_DOWN;
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/* off the ground, he falls, and what he carries goes with him if it can.
 * TRUE if he falls */
static Bool e102_try_fall(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    if (!(work->flags & PLAYER_ON_GROUND)) {
        if (work->flags & PLAYER_HOLDING) {
            if (player_carry_flags(work) & PLAYER_CAN_CARRY) {
                work->mode = E102_HOLD_FALL;
                record->anim.action = ACTION_HOLD_FALL;
                return TRUE;
            }
            e102_let_go(work, motion, record);
        }
        work->mode = E102_FALL;
        record->anim.action = ACTION_FALL;
        return TRUE;
    }
    return FALSE;
}

/* with no stick and no speed, he stops: he stands, or grips or holds what
 * he carries. ACTION_HARD_LANDING plays out first. TRUE if he stops */
static Bool e102_try_stop(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    if (record->anim.action != ACTION_HARD_LANDING &&
        !player_read_stick(work, NULL, NULL) && record->speed.x == 0.0F) {
        if (work->flags & PLAYER_HOLDING) {
            if (player_carry_flags(work) &
                (PLAYER_CAN_PUSH | PLAYER_CAN_SHAKE)) {
                work->mode = E102_GRIP;
                record->anim.action = ACTION_GRIP;
            } else {
                work->mode = E102_HOLD_STAND;
                record->anim.action = ACTION_HOLD_STAND;
            }
        } else {
            e102_stand(work, motion, record);
        }
        return TRUE;
    }
    return FALSE;
}

/* the grab button pressed while he touches something he can take: he
 * starts to lift it, or grips it to push, pull or shake. TRUE if he
 * does */
static Bool e102_try_grab(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    int flags;

    if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                    pad_grab_buttons)) {
        /* @hack spelling: flags assigned in the test, which SHC tests in R0 and
         * copies to R12 in the BT/S's slot (0x8c0d9d48 to 0x8c0d9d4c).
         * assigned in a statement before it, SHC copies first and tests R12,
         * and 3 of 89 words differ */
        if ((flags = player_carry_flags(work)) != 0) {
            /* @hack spelling: the default first, so that the last case ends the
             * switch with no break, which leaves PLAYER_CAN_PUSH in R2
             * (0x8c0d9d66). with the default last, after a break, R3 takes it
             * and 2 of 89 words differ */
            switch (player_face_wall(work, motion)) {
            default:
                return FALSE;
            case PLAYER_WALL_AWAY:
            case PLAYER_WALL_ACROSS:
            case PLAYER_WALL_ASKEW:
            case PLAYER_WALL_HEAD_ON:;
            }
            if (flags & PLAYER_CAN_PUSH) {
                work->mode = E102_GRIP;
                record->anim.action = ACTION_GRIP_START;
                record->held = work->collision->hit->owner;
                work->flags |= PLAYER_HOLDING;
                return TRUE;
            } else if (flags & PLAYER_CAN_CARRY) {
                work->mode = E102_LIFT;
                record->anim.action = ACTION_LIFT;
                if (record->speed.x >= record->consts.trot_speed) {
                    record->speed.x = record->consts.trot_speed;
                }
                return TRUE;
            } else if (flags & PLAYER_CAN_SHAKE) {
                work->mode = E102_GRIP;
                record->anim.action = ACTION_GRIP_START;
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* at ACTION_LIFT's LIFT_FRAME, he has what he lifts if he faces it squarely,
 * and stands if he doesn't. TRUE once he has it */
static Bool e102_try_take_lifted(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record)
{
    int flags;

    player_approach_touched(work, motion);
    if (record->anim.frame >= LIFT_FRAME) {
        /* @hack spelling: flags assigned in the test, which SHC tests in R0 and
         * stores to the frame in the BT/S's slot (MOV.L R0,@R15 at
         * 0x8c0d9e6a). assigned in a statement before it, flags takes R11,
         * which the function then saves, and 67 of 83 words differ */
        if ((flags = player_carry_flags(work)) != 0) {
            /* @hack spelling: a switch of one case, which puts e102_stand's
             * address in R2 (0x8c0d9ece). as an if, R3 takes it and 2 of 83
             * words differ */
            switch (player_face_wall(work, motion)) {
            case PLAYER_WALL_HEAD_ON:
                if (flags & PLAYER_CAN_CARRY) {
                    work->mode = E102_HOLD_STAND;
                    if (e102_in_play) {
                        sound_play_at(SE_LIFT, 0, 0, 0, work->pos.x,
                                      work->pos.y, work->pos.z);
                    }
                    record->held = work->collision->hit->owner;
                    record->held->work->flags |= CARRY_HELD;
                    work->flags |= PLAYER_HOLDING;
                    work->flags &= ~COLLISION_TOUCHING;
                    return TRUE;
                }
                break;
            }
            e102_stand(work, motion, record);
        }
    }
    return FALSE;
}

/* the grab button pressed as he carries something. in the air, or at trot
 * speed or faster, he throws it, at a task near him if there is one.
 * otherwise he puts it down. TRUE if it's pressed */
static Bool e102_try_throw(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    if (pad_records[PLAYER_VARS(work)->player].press & pad_grab_buttons) {
        if (!(work->flags & PLAYER_ON_GROUND)) {
            work->mode = E102_AIR_THROW;
            if (player_has_target(PLAYER_VARS(work)->player)) {
                record->anim.action = ACTION_AIR_THROW_AT;
            } else {
                record->anim.action = ACTION_AIR_THROW;
            }
        } else if (record->speed.x < record->consts.trot_speed) {
            if (e102_in_play) {
                sound_play_at(SE_PUT_DOWN, 0, 0, 0, work->pos.x, work->pos.y,
                              work->pos.z);
            }
            work->mode = E102_PUT_DOWN;
            record->anim.action = ACTION_PUT_DOWN;
            player_clear_speed(motion, record);
        } else {
            work->mode = E102_THROW;
            if (player_has_target(PLAYER_VARS(work)->player)) {
                record->anim.action = ACTION_THROW_AT;
            } else {
                record->anim.action = ACTION_THROW;
            }
        }
        return TRUE;
    }
    return FALSE;
}

/* what he carries leaves his hands at a set frame of his action. he throws
 * it at the nearest task near him or ahead, or puts it down. TRUE as he
 * lets it go, or once it's gone */
static Bool e102_try_release(struct task_work *work, struct task_motion *motion,
                             struct player *record)
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
    /* @hack spelling: action assigned in the test, which extends it into R0
     * (EXTU.W R6,R0 at 0x8c0da0c8). assigned in a statement before it, SHC
     * extends it in R6, the code after it moves, and 200 of 303 words
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
            njUnitVector(&toss);
            toss.x *= 3.0F;
            toss.y *= 3.0F;
            player_turn_to_world(work, &toss);
            record->held->motion->speed = toss;
        }
        work->flags &= ~PLAYER_HOLDING;
        record->held = NULL;
        if (e102_in_play) {
            sound_play_at(SE_THROW, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
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
        if (e102_in_play) {
            sound_play_at(SE_THROW, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
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

/* with the grab button held, the stick pushes what he grips, pulls it or
 * shakes it. with the button let go, he lifts it if he can, or stands.
 * FALSE only with the button held and no stick */
static Bool e102_try_grip(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    int wall;

    if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                    pad_grab_buttons)) {
        wall = player_stick_into_wall(work, motion);
        /* @hack redundant: an else after the return, which puts
         * player_carry_flags's address in R3, not R2 (0x8c0da398); without it,
         * 44 of 82 words differ */
        if (wall == PLAYER_WALL_NO_STICK) {
            return FALSE;
        } else if (player_carry_flags(work) & PLAYER_CAN_SHAKE) {
            work->mode = E102_SHAKE;
            record->anim.action = ACTION_SHAKE;
        } else if (wall == PLAYER_WALL_HEAD_ON) {
            work->mode = E102_PUSH;
            record->anim.action = ACTION_PUSH;
            /* @hack redundant: a return in the push arm, which gives it its
             * stores and a BRA of its own (0x8c0da3e6 to 0x8c0da3f0). without
             * it, the arm branches to the pull arm's store, the code after it
             * moves, and 45 of 82 words differ */
            return TRUE;
        } else {
            work->mode = E102_PULL;
            record->anim.action = ACTION_PULL;
        }
        return TRUE;
    } else {
        if (player_carry_flags(work) & PLAYER_CAN_CARRY) {
            work->mode = E102_LIFT;
            record->anim.action = ACTION_LIFT;
        } else {
            e102_let_go(work, motion, record);
            e102_stand(work, motion, record);
        }
        return TRUE;
    }
    /* @hack redundant: a return after an if whose arms both return. the
     * NO_STICK test's return branches to its MOV #0,R0 (BT 0x8c0da42e at
     * 0x8c0da396). without it, that test returns in place, and 61 of 82 words
     * differ */
    return FALSE;
}

/* he lets go of what he holds, which moves off ahead of him if he was
 * carrying it */
static void e102_let_go(struct task_work *work, struct task_motion *motion,
                        struct player *record)
{
    NJS_VECTOR toss;

    if (work->flags & PLAYER_HOLDING) {
        record->held->work->flags &= ~CARRY_HELD;
        work->flags &= ~PLAYER_HOLDING;
        if (player_carry_flags(work) & PLAYER_CAN_CARRY) {
            toss.x = 1.0F;
            toss.y = 1.0F;
            toss.z = 0.0F;
            njPushMatrix(&_unit_matrix_);
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
            TURN(njRotateY, -work->ang.y);
            njCalcVector(NULL, &toss, &record->held->motion->speed);
            njPopMatrix(1);
        }
        record->held = NULL;
    }
}

/* what he carries sits midway between his gun's tip and his hand, or 2.5 out
 * along his gun tip's axis as he throws. he lets go of what he holds but can't
 * carry once he touches another task */
static void e102_carry(struct task_work *work, struct task_motion *motion,
                       struct player *record)
{
    struct collision_shape *shape;
    struct task_work *held;
    NJS_POINT3 hands;
    NJS_VECTOR offset;
    int action;

    held = record->held->work;
    shape = work->collision->shapes;
    if (held == NULL) {
        return;
    }
    if (!(player_carry_flags(work) & PLAYER_CAN_CARRY)) {
        if (work->collision->hit != NULL) {
            if (held != work->collision->hit->owner->work) {
                e102_let_go(work, motion, record);
                if (record->held == NULL) {
                    if (work->flags & PLAYER_ON_GROUND) {
                        e102_stand(work, motion, record);
                        if (e102_in_play) {
                            sound_play_at(SE_STEP, 0, 0, 0, work->pos.x,
                                          work->pos.y, work->pos.z);
                        }
                    } else {
                        work->mode = E102_FALL;
                        record->anim.action = ACTION_FALL;
                    }
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

/* the head and eyes take back the angles they had before the look */
static void e102_look_destroy(struct task *task)
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

/* the head turns the way the player turns, at the camera while Z is held,
 * or at what its event says. the head takes what its limits allow, and the
 * eyes the rest. no task runs this step. it isn't static, since SHC would
 * leave out a static nothing refers to */
void e102_look_step(struct task *task)
{
    struct task_work *work = task->work;
    struct player_look_motion *look = PLAYER_LOOK(task);
    struct task_work *owner = player_work[PLAYER_VARS(work)->player];
    struct task_motion *owner_motion = player_motion[PLAYER_VARS(work)->player];
    struct event_work *event;
    struct collision_shape *shape;
    struct task_work *camera;
    struct task_work *target;
    /* @hack spelling: angles declared before the vectors, which puts facing at
     * 8, direction at 20 and angles at 32 in the frame (ADD #32,R12 at
     * 0x8c0da8ee). declared facing first, angles take 8 and facing 32, and
     * 15 of 373 words differ */
    Angle angles[3];
    NJS_VECTOR direction;
    NJS_VECTOR facing;

    if (owner == NULL) {
        work->mode = LOOK_END;
    } else if (work->mode == LOOK_INIT) {
        /* @hack spelling: an empty arm, which leaves a starting look in
         * LOOK_INIT (BT 0x8c0da8de at 0x8c0da874). with the tests below nested
         * under a test of LOOK_INIT, the owner's event and LOOK_EVENT trade R2
         * and R3 (0x8c0da876, 0x8c0da87e), and 14 of 373 words differ */
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
        task->destructor = e102_look_destroy;
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
        /* @hack spelling: the camera's task taken first, which puts
         * camera_work's address in R2 and player_turn_to_world's in R3
         * (0x8c0daa22, 0x8c0daa28). the other way round, the two trade and 6
         * of 373 words differ */
        camera = camera_work;
        shape = owner->collision->shapes;
        /* @hack spelling: y and z set in one chain, which keeps owner in R13
         * for the whole step (0x8c0da85c). set apart, owner takes R11, and 191
         * of 373 words differ */
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
            /* @hack spelling: y and z set in one chain, which keeps owner in
             * R13 for the whole step (0x8c0da85c), as LOOK_CAMERA's chain does.
             * set apart, owner takes R11, and 189 of 373 words differ */
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
            /* @hack spelling: y and z set in one chain, which keeps owner in
             * R13 for the whole step (0x8c0da85c), as LOOK_CAMERA's chain does.
             * set apart, owner takes R11, and 189 of 373 words differ */
            facing.y = facing.z = 0.0F;
            facing.x = 1.0F;
            player_turn_to_world(owner, &facing);
            direction.x = target->pos.x + event->look_pos.x - shape->center.x;
            direction.y = target->pos.y + event->look_pos.y - shape->center.y;
            direction.z = target->pos.z + event->look_pos.z - shape->center.z;
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

/* with his booster, holding jump slows his fall to a hover. TRUE while he
 * hovers */
static Bool e102_hover(struct task_work *work, struct task_motion *motion,
                       struct player *record)
{
    struct e102 *e102;

    if (!(record->upgrades & PLAYER_E102_BOOSTER)) {
        return FALSE;
    }
    if (pad_records[PLAYER_VARS(work)->player].on & pad_jump_buttons) {
        e102 = record->own.e102.record[0];
        record->accel.y = 0.0F;
        motion->speed.y = motion->speed.y + 0.05F > HOVER_SPEED
                              ? motion->speed.y + 0.05F
                              : HOVER_SPEED;
        e102_hover_dust(work, e102->ground);
        e102_hovering = TRUE;
        if (e102_hovered != TRUE && e102_in_play) {
            sound_play_at(SE_HOVER, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        return TRUE;
    } else {
        sound_stop(SE_HOVER);
    }
    return FALSE;
}

/* e102_hover for a jump off the water, which hovers only once he falls at
 * HOVER_SPEED or faster. TRUE while he hovers */
static Bool e102_hover_water(struct task_work *work, struct task_motion *motion,
                             struct player *record)
{
    struct e102 *e102;

    if (!(record->upgrades & PLAYER_E102_BOOSTER)) {
        return FALSE;
    }
    if ((pad_records[PLAYER_VARS(work)->player].on & pad_jump_buttons) &&
        motion->speed.y <= HOVER_SPEED) {
        e102 = record->own.e102.record[0];
        record->accel.y = motion->accel.y = 0.0F;
        motion->speed.y = motion->speed.y + 0.05F > HOVER_SPEED
                              ? motion->speed.y + 0.05F
                              : HOVER_SPEED;
        e102_hover_dust(work, e102->ground);
        e102_hovering = TRUE;
        if (e102_hovered != TRUE && e102_in_play) {
            sound_play_at(SE_HOVER, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        return TRUE;
    } else {
        sound_stop(SE_HOVER);
    }
    return FALSE;
}

/* under 25 degrees of slope he stands, and fidgets after FIDGET_STEPS. up
 * to 45, unless the ground is WORLD_NO_SLOPE, he leans uphill or
 * downhill */
static void e102_set_stand_action(struct task_work *work,
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

    cosine = -njInnerProduct(&world_gravity, &record->ground_normal);
    if (cosine > njCos(NJM_DEG_ANG(25))) {
        action = record->anim.action;
        if (action != ACTION_STAND && action != ACTION_SLIDE_STOP &&
            action != ACTION_SLIDE_STAND && action != ACTION_FIDGET_START &&
            action != ACTION_FIDGET && action != ACTION_FIDGET_END) {
            record->anim.action = ACTION_STAND;
        } else if (record->idle_steps > FIDGET_STEPS) {
            record->idle_steps = 0;
            record->anim.action = ACTION_FIDGET_START;
        }
    } else if (!(record->ground_flags & WORLD_NO_SLOPE) &&
               cosine > njCos(NJM_DEG_ANG(45))) {
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
    record->idle_steps++;
}

/* how he walks, or walks carrying, by the speed he's about to have. not
 * while he turns back or lands hard */
static void e102_set_walk_action(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record)
{
    Float speed = record->speed.x + (motion->slip * 5.0F);
    int action = record->anim.action;

    if (action == ACTION_TURN || action == ACTION_HOLD_SKATE_TURN ||
        action == ACTION_HARD_LANDING) {
        return;
    }
    if (work->flags & PLAYER_HOLDING) {
        if (speed > record->consts.trot_speed) {
            record->anim.action = ACTION_HOLD_RUN;
        } else {
            record->anim.action = ACTION_HOLD_WALK;
        }
    } else {
        if (speed > record->consts.trot_speed) {
            record->anim.action = ACTION_RUN;
        } else {
            record->anim.action = ACTION_WALK;
        }
    }
}

Bool e102_can_aim(struct task_work *work)
{
    return e102_aim_modes[work->mode];
}

static void e102_move_ground(struct task_work *work, struct task_motion *motion,
                             struct player *record)
{
    player_accel_ground(work, motion, record);
}

static void e102_move_air(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    player_accel_air(work, motion, record);
}

/* the stick speeds his skate up, and with it let go he slows. his speed
 * stays within its limits, and he keeps to the ground */
static void e102_skate(struct task_work *work, struct task_motion *motion,
                       struct player *record)
{
    Float stroke;
    Float drag;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Float unused;
    Bool pushed;

    e102_turn(work, motion, record);
#ifdef GCC
    /* repairs the unset stroke the tag below names: GCC's stroke starts at
     * 0, a let-go stick's, so a call that leaves it as it was leaves 0 */
    stroke = 0.0F;
#endif
    pushed = player_read_stick(work, NULL, &stroke);
    /* @bug uninitialized: player_read_stick sets stroke only from a stick
     * that's pushed. otherwise FMOV.S @R15,FR1 at 0x8c0db058 reads what
     * stroke's frame slot held, and his speed changes by that, scaled, with
     * the drag */
    stroke *= record->consts.ground_accel * 0.5F;
    /* @hack spelling: a loop of one pass, which stores stroke before a plain
     * BF (FMOV.S FR1,@R15 at 0x8c0db060, BF at 0x8c0db062). as an if, SHC
     * puts the store in a BF/S's slot and 2 of 96 words differ */
    while (pushed) {
        goto move;
    }
    /* @hack redundant: a read of motion's speed into a local nothing reads.
     * SHC drops the read but keeps motion in the frame (MOV.L R5,@(8,R15) at
     * 0x8c0db03e). without it, the frame is 4 bytes smaller and 94 of 96
     * words differ */
    unused = motion->speed.x;
    drag = record->consts.ground_drag;
    if (record->speed.x < 0.0F) {
        drag = -(drag * 1.5F);
    }
    stroke += drag;
move:
    record->speed.x += stroke;
    if (record->launch_steps) {
        record->speed.x -= record->consts.ground_drag;
    } else {
        record->speed.x = record->speed.x < record->consts.sprint_speed
                              ? record->speed.x
                              : record->consts.sprint_speed;
    }
    record->speed.x = record->speed.x > -record->consts.sprint_speed
                          ? record->speed.x
                          : -record->consts.sprint_speed;
    record->speed.y = -3.0F;
    record->speed.z = 0.0F;
    record->accel.x = 0.0F;
    record->accel.y = 0.0F;
    record->accel.z = 0.0F;
}

static void e102_move_stop(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    player_accel_brake(work, motion, record);
}

static void e102_turn(struct task_work *work, struct task_motion *motion,
                      struct player *record)
{
    player_turn_to_stick(work, motion, record);
}

/* as his model is posed, his head turns with his cannon, and a shot's kick
 * moves his body */
static void e102_pose_node(NJS_OBJECT *object)
{
    if (object == &e102_head) {
        TURN(njRotateY, e102_current->cannon_ang);
        /* @hack spelling: a return in the head's arm, which ends it with an
         * epilogue and RTS of its own (0x8c0db180 to 0x8c0db186). as an
         * else, the arm branches to the shared one and 25 of 41 words
         * differ */
        return;
    }
    if (object == &e102_body) {
        njTranslate(NULL, 0.0F, e102_current->kick * 2.0F, 0.0F);
    }
}

/* on a ledge he hangs his height below the point its path gives. as he moves
 * along it, ACTION_LEDGE steps a frame a step, forward or back, round its
 * LEDGE_FRAMES. once the stick is let go, he stops at LEDGE_HALF or
 * LEDGE_LAST */
static void e102_ledge_move(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    NJS_VECTOR up;

    up.x = 0.0F;
    up.y = record->consts.height;
    up.z = 0.0F;
    player_turn_to_world(work, &up);
    njSubVector(&work->pos, &up);

    if (record->speed.x != 0.0F && motion->drive == 0.0F) {
        if (record->anim.frame == LEDGE_HALF ||
            record->anim.frame >= LEDGE_LAST) {
            work->mode = E102_LEDGE;
            record->speed.x = 0.0F;
        }
    } else if (record->speed.x > 0.0F) {
        if (record->anim.frame >= 0.0F && record->anim.frame < LEDGE_HALF) {
            record->anim.frame += 1.0F;
            if (record->anim.frame == LEDGE_HALF &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = E102_LEDGE;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame += 1.0F;
            if (record->anim.frame >= LEDGE_FRAMES) {
                record->anim.frame = 0.0F;
            }
            if (record->anim.frame == LEDGE_LAST &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = E102_LEDGE;
                record->speed.x = 0.0F;
            }
        }
    } else if (record->speed.x < 0.0F) {
        if (record->anim.frame > LEDGE_HALF &&
            record->anim.frame <= LEDGE_LAST) {
            record->anim.frame -= 1.0F;
            if (record->anim.frame == LEDGE_HALF &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = E102_LEDGE;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame -= 1.0F;
            if (record->anim.frame < 0.0F) {
                record->anim.frame = LEDGE_LAST;
            }
            if (record->anim.frame == LEDGE_LAST &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = E102_LEDGE;
                record->speed.x = 0.0F;
            }
        }
    } else {
        record->speed.x = 0.0F;
    }
}

/* once his time runs out, he goes down, his sounds stop, and an aim that's
 * locking on is cancelled. TRUE when he goes down. he doesn't go down while an
 * event holds the game or a shot homes in. he doesn't in his first TIME_UP_WAIT
 * steps after his init or his last leap either. the rival in a battle never
 * goes down */
static Bool e102_try_time_up(struct task_work *work, struct player *record)
{
    struct e102 *e102 = record->own.e102.record[0];

    if (!e102_time_runs) {
        return FALSE;
    }
    if (work->mode == E102_TIME_UP || work->mode == E102_DOWN) {
        return FALSE;
    }
    if (!e102_time_up || e102->shot_count > 0 || e102->steps < TIME_UP_WAIT) {
        return FALSE;
    }
    if (event_in_control() == TRUE || work->id == RIVAL_ID) {
        return FALSE;
    }

    switch (work->mode) {
    case E102_SKATE:
        record->anim.action = ACTION_SKATE_DOWN;
        break;
    case E102_WATER_SKATE:
        record->anim.action = ACTION_WATER_DOWN;
        break;
    default:
        record->anim.action = ACTION_DOWN;
        break;
    }

    work->mode = E102_TIME_UP;
    /* @hack type: the player cast to Sint8, as MOV.B loads it, which passes
     * it on as loaded (MOV R0,R4 at 0x8c0db44a). as the Uint8 it is, it
     * takes an EXTU.B there, and 1 of 94 words differs */
    player_down((Sint8)PLAYER_VARS(work)->player);
    sound_stop_id((int)work);
    if (e102_in_play) {
        sound_play_at(SE_TIME_UP, 0, 0, 0, work->pos.x, work->pos.y,
                      work->pos.z);
    }
    if (e102->aim_mode == E102_AIM_LOCKING) {
        e102->aim_mode = E102_AIM_CANCEL;
    }
    return TRUE;
}

/* a step's sound each time he has walked another STEP_LENGTH */
static void e102_step_sound(struct task_work *work, struct player *record,
                            struct e102 *e102)
{
    e102->walked += _builtin_fabsf(record->speed.x);
    if (e102->walked > STEP_LENGTH) {
        e102->walked -= STEP_LENGTH;
        if (e102_in_play) {
            sound_play_at(SE_STEP, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
    }
}

/* the skate's sound starts once, and stops with a brake's sound */
static void e102_skate_sound_on(struct task_work *work, struct e102 *e102)
{
    if (!(e102->flags & E102_SKATE_SOUND)) {
        if (e102_in_play) {
            sound_play_at(SE_SKATE, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        e102->flags |= E102_SKATE_SOUND;
    }
}

static void e102_skate_sound_off(struct task_work *work, struct e102 *e102)
{
    if (e102->flags & E102_SKATE_SOUND) {
        sound_stop(SE_SKATE);
        if (e102_in_play) {
            sound_play_at(SE_BRAKE, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        e102->flags &= ~E102_SKATE_SOUND;
    }
}
