/* Big as a player: his modes, his fishing and his carrying */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/invisible.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "event/event.h"
#include "minicart/riders.h"
#include "object/common.h"
#include "object/ring.h"
#include "object/savepoint.h"
#include "player/b_mot.h"
#include "player/big.h"
#include "player/big_models.h"
#include "player/big_stand_downhill.h"
#include "player/fish.h"
#include "player/fishing.h"
#include "player/player.h"
#include "player/target.h"
#include "player/textures.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

#define SE_CRASH 1202
#define SE_JUMP_VOICE 1200
#define SE_LIFT 1204
#define SE_THROW 1206
#define SE_PUSH_START 1207
#define SE_HURT 1212
#define SE_SNAP 803
#define SE_CATCH 1311
#define SE_CATCH_GOAL 1297
#define SE_CATCH_CHAOS 1300

/* how many steps the catch shows, unless a press of the action button ends
 * it */
#define CATCH_STEPS 120
/* how far his rod's shape sits back from the rod's origin, weld_point4,
 * against its x axis, weld_axis4 */
#define ROD_SHAPE_BACK 10.0F
/* the attack level of his rod's shape, in the SHAPE_ATTACK bits of its
 * levels */
#define ROD_ATTACK 2

/* his modes, in work->mode */
enum big_mode {
    BIG_INIT,
    BIG_STAND,
    BIG_WALK,
    BIG_PATH,
    BIG_JUMP,
    BIG_SPRING,
    BIG_LAUNCH,
    BIG_FLUNG, /* flung by PLAYER_MODE_FLING: only gravity accelerates him */
    BIG_FALL,
    BIG_STOP, /* braking, the stick turned back against his run */
    BIG_PUSH,
    BIG_HURT,
    BIG_CRASH, /* knocked back from a wall he ran into */
    BIG_SLIDE,
    BIG_POSE, /* still, in the action a PLAYER_REQUEST submode set */
    BIG_LIFT,
    BIG_PUT_DOWN,
    BIG_HOLD_STAND,
    BIG_GRIP, /* gripping a task, to push, pull or shake it */
    BIG_HOLD_WALK,
    BIG_THROW,
    BIG_HOLD_STOP,
    BIG_HOLD_FALL,
    BIG_HOLD_JUMP,
    BIG_HOLD_SPRING,
    BIG_HOLD_LAUNCH,
    BIG_PULL,
    BIG_AIR_THROW,
    BIG_SHAKE,
    BIG_LEDGE, /* hanging from a ledge's path */
    BIG_LEDGE_GO,
    BIG_SWING, /* swinging his rod as an attack */
    BIG_CAST, /* raising the rod to cast */
    BIG_AIM, /* aiming the target, until the action button's release casts */
    /* swinging the rod, the action button let go before the cast */
    BIG_CAST_SWING,
    BIG_REEL,
    BIG_REEL_TURN, /* turning the reel, the stick held right, down or left */
    BIG_CATCH,
    BIG_BREATHE, /* breathing at a bubble of air */
    /* the line has snapped. player_down takes him down once ACTION_SNAP
     * ends */
    BIG_SNAPPED,
    /* the six modes from BIG_CAST to BIG_CATCH, again, as he floats in his
     * life belt */
    BIG_FLOAT_CAST,
    BIG_FLOAT_AIM,
    BIG_FLOAT_CAST_SWING,
    BIG_FLOAT_REEL,
    BIG_FLOAT_REEL_TURN,
    BIG_FLOAT_CATCH,
    BIG_UNK_46,
    BIG_FLOAT,
    BIG_SWIM,
    BIG_DIVE,
    /* shaking a CARRY_KIND_SMALL task he holds. from BIG_HOLD_STAND, the
     * grab button held while the stick is pushed starts it, and letting the
     * stick go ends it */
    BIG_HOLD_SHAKE,
    /* riding, still in a stage module's action, until PLAYER_MODE_RIDE_OFF */
    BIG_MODULE_POSE,
    BIG_EVENT, /* moved along an event's paths, until it has none */
    BIG_RISE, /* drowning: ACTION_DROWN, then up to the water's surface */
    BIG_DOWN,
    /* named for AMY_EDIT and E102_EDIT, which the steps of Amy and E-102
     * enter while the set editor is open. nothing in this file sets it, but
     * big_draw draws his model unposed in it while X is held */
    BIG_EDIT
};

extern NJS_ACTION big_fidget_start_action;
extern NJS_ACTION big_fidget_action;
extern NJS_ACTION big_stand_uphill_action;
extern NJS_ACTION big_push_action;
extern NJS_ACTION big_hurt_action;
extern NJS_ACTION big_hurt_land_action;

/* each helper that takes his record takes his work and motion with it, as
 * player.h's steps do, whether it uses them or not */
static void big_choose_mode(struct task_work *work, struct task_motion *motion,
                            struct player *record);
static void big_destroy(struct task *task);
static void big_move_shapes(struct task_work *work, struct task_motion *motion,
                            struct player *record);
static void big_draw(struct task *task);
static void big_set_stand_action(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record);
static void big_set_walk_action(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record);
static void big_ledge_move(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static Bool big_try_hurt(struct task_work *work, struct task_motion *motion,
                         struct player *record);
static Bool big_try_jump(struct task_work *work, struct task_motion *motion,
                         struct player *record);
static Bool big_eyes_above_water(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record);
static Bool big_try_cast(struct task_work *work, struct task_motion *motion,
                         struct player *record);
static Bool big_try_float_cast(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static Bool big_follow_event(struct task_work *work, struct task_motion *motion,
                             struct player *record);
static Bool big_try_request(struct task_work *work, struct task_motion *motion,
                            struct player *record);
static Bool big_try_request_holding(struct task_work *work,
                                    struct task_motion *motion,
                                    struct player *record);
static Bool big_can_float(struct task_work *work, struct task_motion *motion,
                          struct player *record);
static Bool big_belt_in_water(struct task_work *work,
                              struct task_motion *motion,
                              struct player *record);
static Bool big_try_float(struct task_work *work, struct task_motion *motion,
                          struct player *record);
static Bool big_try_fall(struct task_work *work, struct task_motion *motion,
                         struct player *record);
static Bool big_try_stop(struct task_work *work, struct task_motion *motion,
                         struct player *record);
static Bool big_try_grab(struct task_work *work, struct task_motion *motion,
                         struct player *record);
static Bool big_try_take_lifted(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record);
static Bool big_try_throw(struct task_work *work, struct task_motion *motion,
                          struct player *record);
static Bool big_try_release(struct task_work *work, struct task_motion *motion,
                            struct player *record);
static Bool big_try_grip(struct task_work *work, struct task_motion *motion,
                         struct player *record);
static void big_let_go(struct task_work *work, struct task_motion *motion,
                       struct player *record);
static void big_carry(struct task_work *work, struct task_motion *motion,
                      struct player *record);
static void big_float_up(struct task_work *work, struct task_motion *motion,
                         struct player *record);
static void big_mouth_start(struct task *task);
static void big_sway_destroy(struct task *task);
static void big_sway_step(struct task *task);
static Bool big_look_create(Sint8 player, NJS_OBJECT **nodes);
static void big_look_destroy(struct task *task);
static void big_look_step(struct task *task);
static void big_look_move_eyes(struct player_look_motion *look);
static Bool big_reeling_or_catching(struct task_work *work);
static void big_play_catch_sound(void);
static void big_land_catch(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static void big_snap_line(struct task_work *work, struct task_motion *motion,
                          struct player *record);
static void big_take_camera(void);
static void big_give_camera(void);

/* his actions, which record->anim.action indexes by enum big_action */
static struct player_action big_actions[] = {
    {&big_stand_action, 73, PLAYER_ANIM_LOOP, ACTION_STAND, 0.0625F, 0.2F},
    {&big_fidget_start_action, 73, PLAYER_ANIM_ONCE, ACTION_FIDGET, 0.0625F,
     0.2F},
    {&big_fidget_action, 73, PLAYER_ANIM_ONCE, ACTION_FIDGET_END, 0.125F, 0.2F},
    {&big_fidget_start_action, 73, PLAYER_ANIM_BACK, ACTION_STAND, 0.125F,
     0.2F},
    {&big_stand_uphill_action, 73, PLAYER_ANIM_LOOP, ACTION_STAND_UPHILL,
     0.0625F, 0.2F},
    {&big_stand_downhill_action, 73, PLAYER_ANIM_LOOP, ACTION_STAND_DOWNHILL,
     0.0625F, 0.2F},
    {&big_land_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F, 1.0F},
    {&big_walk_action, 73, PLAYER_ANIM_LOOP_SPEED, ACTION_WALK, 0.25F, 2.0F},
    {&big_run_action, 73, PLAYER_ANIM_LOOP_SPEED, ACTION_TROT, 0.25F, 1.4F},
    {&big_run_action, 73, PLAYER_ANIM_LOOP_SPEED, ACTION_RUN, 0.25F, 0.8F},
    {&big_run_action, 73, PLAYER_ANIM_LOOP_SPEED, ACTION_SPRINT, 0.25F, 0.5F},
    {&big_run_action, 73, PLAYER_ANIM_LOOP_SPEED, ACTION_FULL_SPEED, 0.5F,
     0.5F},
    {&big_jump_action, 73, PLAYER_ANIM_LOOP, ACTION_SPRING, 0.25F, 1.0F},
    {&big_fall_start_action, 73, PLAYER_ANIM_ONCE, ACTION_FALL, 0.125F, 1.0F},
    {&big_fall_action, 73, PLAYER_ANIM_LOOP, ACTION_FALL, 0.25F, 1.0F},
    {&big_jump_action, 73, PLAYER_ANIM_LOOP, ACTION_JUMP, 0.25F, 1.0F},
    {&big_walk_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F, 1.0F},
    {&big_push_start_action, 73, PLAYER_ANIM_ONCE, ACTION_PUSH, 0.125F, 1.0F},
    {&big_push_action, 73, PLAYER_ANIM_LOOP, ACTION_PUSH, 0.25F, 0.3F},
    {&big_hurt_action, 73, PLAYER_ANIM_HOLD, ACTION_HURT, 1.0F, 1.0F},
    {&big_hurt_land_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F, 1.0F},
    {&big_walk_action, 73, PLAYER_ANIM_LOOP, ACTION_UPDRAFT, 0.0625F, 1.0F},
    {&big_walk_action, 73, PLAYER_ANIM_ONCE, ACTION_CRASH_END, 0.5F, 1.0F},
    {&big_walk_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.0625F, 1.0F},
    {&big_walk_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F, 0.9F},
    {&big_walk_action, 73, PLAYER_ANIM_LOOP, ACTION_SLIDE, 0.0625F, 1.0F},
    {&big_lift_action, 73, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND, 0.125F, 0.25F},
    {&big_hold_run_action, 73, PLAYER_ANIM_LOOP_SPEED, ACTION_HOLD_RUN, 0.125F,
     0.6F},
    {&big_hold_stand_action, 73, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND, 0.5F,
     1.0F},
    {&big_hold_stand_action, 73, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND, 0.5F,
     1.0F},
    {&big_hold_stand_action, 73, PLAYER_ANIM_LOOP, ACTION_HOLD_STAND, 0.0625F,
     0.25F},
    {&big_hold_walk_action, 73, PLAYER_ANIM_LOOP_SPEED, ACTION_HOLD_WALK,
     0.125F, 1.0F},
    {&big_hold_stand_action, 73, PLAYER_ANIM_LOOP, ACTION_HANG_ON, 0.5F, 1.0F},
    {&big_pull_action, 73, PLAYER_ANIM_LOOP, ACTION_PULL, 0.125F, 0.3F},
    {&big_shake_action, 73, PLAYER_ANIM_LOOP, ACTION_SHAKE, 0.125F, 0.5F},
    {&big_push_start_action, 73, PLAYER_ANIM_ONCE, ACTION_GRIP, 0.25F, 1.0F},
    {&big_grip_action, 73, PLAYER_ANIM_LOOP, ACTION_GRIP, 0.5F, 0.25F},
    {&big_hold_jump_action, 73, PLAYER_ANIM_LOOP, ACTION_HOLD_JUMP, 0.25F,
     1.0F},
    {&big_hold_fall_start_action, 73, PLAYER_ANIM_ONCE, ACTION_HOLD_FALL, 0.25F,
     1.0F},
    {&big_hold_fall_action, 73, PLAYER_ANIM_LOOP, ACTION_HOLD_FALL, 0.25F,
     1.0F},
    {&big_hold_land_action, 73, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND, 0.25F,
     1.0F},
    {&big_throw_at_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.5F, 1.0F},
    {&big_throw_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 0.5F, 1.0F},
    {&big_throw_at_action, 73, PLAYER_ANIM_ONCE, ACTION_FALL, 0.5F, 1.0F},
    {&big_throw_action, 73, PLAYER_ANIM_ONCE, ACTION_FALL, 0.5F, 1.0F},
    {&big_lift_action, 73, PLAYER_ANIM_BACK, ACTION_STAND, 1.0F, 0.25F},
    {&big_ledge_action, 73, PLAYER_ANIM_DRIVEN, ACTION_LEDGE, 1.0F, 0.5F},
    {&big_flung_action, 73, PLAYER_ANIM_LOOP, ACTION_FLUNG, 1.0F, 0.3F},
    {&big_clear_action, 73, PLAYER_ANIM_LOOP, ACTION_CLEAR, 0.5F, 0.5F},
    {&big_swing_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F, 1.0F},
    {&big_reel_action, 73, PLAYER_ANIM_HOLD, ACTION_STAND, 1.0F, 1.0F},
    {&big_reel_action, 73, PLAYER_ANIM_DRIVEN, ACTION_STAND, 1.0F, 1.0F},
    {&big_cast_start_action, 73, PLAYER_ANIM_HOLD, ACTION_CAST, 1.0F, 1.0F},
    {&big_cast_action, 73, PLAYER_ANIM_ONCE, ACTION_REEL, 1.0F, 1.0F},
    {&big_cast_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F, 1.0F},
    {&big_reel_down_action, 73, PLAYER_ANIM_ONCE, ACTION_REEL_HELD, 1.0F, 0.5F},
    {&big_reel_down_right_action, 73, PLAYER_ANIM_ONCE, ACTION_REEL_HELD, 1.0F,
     0.5F},
    {&big_reel_down_left_action, 73, PLAYER_ANIM_ONCE, ACTION_REEL_HELD, 1.0F,
     0.5F},
    {&big_reel_right_action, 73, PLAYER_ANIM_ONCE, ACTION_REEL_HELD, 1.0F,
     0.5F},
    {&big_reel_left_action, 73, PLAYER_ANIM_ONCE, ACTION_REEL_HELD, 1.0F, 0.5F},
    {&big_catch_small_action, 73, PLAYER_ANIM_LOOP, ACTION_CATCH_SMALL, 1.0F,
     0.5F},
    {&big_catch_medium_action, 73, PLAYER_ANIM_LOOP, ACTION_CATCH_MEDIUM, 1.0F,
     0.5F},
    {&big_catch_large_action, 73, PLAYER_ANIM_LOOP, ACTION_CATCH_LARGE, 1.0F,
     0.5F},
    {&big_snap_action, 73, PLAYER_ANIM_ONCE, ACTION_SNAPPED, 1.0F, 0.5F},
    {&big_snapped_action, 73, PLAYER_ANIM_LOOP, ACTION_STAND, 1.0F, 0.5F},
    {&dat_0cc61540, 73, PLAYER_ANIM_LOOP, ACTION_STAND, 1.0F, 1.0F},
    {&big_float_cast_start_action, 73, PLAYER_ANIM_HOLD, ACTION_FLOAT_CAST,
     1.0F, 0.5F},
    {&big_float_cast_action, 73, PLAYER_ANIM_HOLD, ACTION_FLOAT, 1.0F, 0.5F},
    {&big_float_cast_action, 73, PLAYER_ANIM_DRIVEN, ACTION_FLOAT, 1.0F, 0.5F},
    {&big_float_reel_down_action, 73, PLAYER_ANIM_ONCE, ACTION_FLOAT_REEL_HELD,
     0.125F, 0.5F},
    {&big_float_reel_down_right_action, 73, PLAYER_ANIM_ONCE,
     ACTION_FLOAT_REEL_HELD, 0.125F, 0.5F},
    {&big_float_reel_down_left_action, 73, PLAYER_ANIM_ONCE,
     ACTION_FLOAT_REEL_HELD, 0.125F, 0.5F},
    {&big_float_reel_right_action, 73, PLAYER_ANIM_ONCE, ACTION_FLOAT_REEL_HELD,
     0.125F, 0.5F},
    {&big_float_reel_left_action, 73, PLAYER_ANIM_ONCE, ACTION_FLOAT_REEL_HELD,
     0.125F, 0.5F},
    {&big_hold_small_action, 73, PLAYER_ANIM_LOOP, ACTION_HOLD_SMALL, 0.0625F,
     0.3F},
    {&big_hold_small_walk_action, 73, PLAYER_ANIM_LOOP_SPEED,
     ACTION_HOLD_SMALL_WALK, 0.125F, 1.0F},
    {&big_hold_small_run_action, 73, PLAYER_ANIM_LOOP_SPEED,
     ACTION_HOLD_SMALL_RUN, 0.125F, 0.6F},
    {NULL, 73, PLAYER_ANIM_LOOP, ACTION_STAND, 1.0F, 1.0F},
    {&big_lift_small_action, 73, PLAYER_ANIM_ONCE, ACTION_HOLD_SMALL, 0.125F,
     0.25F},
    {NULL, 73, PLAYER_ANIM_LOOP, ACTION_STAND, 1.0F, 1.0F},
    {&big_lift_small_action, 73, PLAYER_ANIM_BACK, ACTION_STAND, 1.0F, 0.25F},
    {NULL, 73, PLAYER_ANIM_LOOP, ACTION_STAND, 1.0F, 1.0F},
    {NULL, 73, PLAYER_ANIM_LOOP, ACTION_STAND, 1.0F, 1.0F},
    {NULL, 73, PLAYER_ANIM_LOOP, ACTION_STAND, 1.0F, 1.0F},
    {NULL, 73, PLAYER_ANIM_LOOP, ACTION_STAND, 1.0F, 1.0F},
    {NULL, 73, PLAYER_ANIM_LOOP, ACTION_STAND, 1.0F, 1.0F},
    {&big_swim_start_action, 73, PLAYER_ANIM_ONCE, ACTION_SWIM, 0.125F, 0.5F},
    {&big_swim_action, 73, PLAYER_ANIM_LOOP, ACTION_SWIM, 1.0F, 0.5F},
    {&big_dive_start_action, 73, PLAYER_ANIM_ONCE, ACTION_DIVE, 0.125F, 1.0F},
    {&big_dive_action, 73, PLAYER_ANIM_LOOP, ACTION_DIVE, 1.0F, 0.3F},
    {&big_float_under_start_action, 73, PLAYER_ANIM_ONCE, ACTION_FLOAT_UNDER,
     0.125F, 1.0F},
    {&big_float_under_action, 73, PLAYER_ANIM_LOOP, ACTION_FLOAT_UNDER, 1.0F,
     0.3F},
    {&big_float_action, 73, PLAYER_ANIM_LOOP, ACTION_FLOAT, 0.0625F, 0.3F},
    {&big_hold_shake_start_action, 73, PLAYER_ANIM_ONCE, ACTION_HOLD_SHAKE,
     0.5F, 0.5F},
    {&big_hold_shake_action, 73, PLAYER_ANIM_LOOP, ACTION_HOLD_SHAKE, 0.5F,
     0.5F},
    {&big_hold_shake_end_action, 73, PLAYER_ANIM_ONCE, ACTION_HOLD_SMALL, 0.5F,
     0.5F},
    {&big_cart_action, 73, PLAYER_ANIM_HOLD, ACTION_RIDE, 0.25F, 0.3F},
    {&big_breathe_action, 73, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F, 1.0F},
    {&big_drown_action, 73, PLAYER_ANIM_HOLD, ACTION_DROWN, 1.0F, 0.5F},
    {&big_hurt_action, 73, PLAYER_ANIM_HOLD, ACTION_DOWN, 1.0F, 1.0F},
    {&big_down_land_action, 73, PLAYER_ANIM_HOLD, ACTION_DOWN_LAND, 1.0F, 1.0F},
};

/* his body's shape, then his rod's */
/* clang-format off */
static struct collision_shape big_shapes[2] = {
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS,
     PUSH_ALL | PUSH_FIRM(PUSH_OTHERS), {0.0F, 1.0F, 0.0F}, 4.0F, 4.0F, 0.65F,
     {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_OFF | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS |
         GUARD(3) | ROD_ATTACK,
     PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 8.0F, 0.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static NJS_OBJECT *big_look_nodes[3] = {&big_head_end, &big_pupil_plus_z,
                                        &big_pupil_minus_z};

/* the head's model: [0] is the copy big_sway_step sways, and [1] the
 * original it sways from */
static NJS_MODEL *big_head_models[2] = {&big_head_sway_model, &big_head_model};

/* the vertex pairs of each seam his welds close. a row each serves his
 * elbows, wrists, knees and toes, on his -z and +z sides alike */
static Uint16 big_weld_indices[4][12] = {
#include "player/big/big_weld_indices.inc"
};

/* his welds. the seams at his elbows, wrists, knees and toes come first.
 * the marks after them keep his fingers, toes, rod and mouth in the
 * record's weld_point0 to weld_point5 */
static struct weld big_welds[] = {
    {&big_object, &big_upper_arm_minus_z, &big_forearm_minus_z, 6, WELD_BOTH, 0,
     0, NULL, big_weld_indices[0]},
    {&big_object, &big_forearm_minus_z, &big_hand_minus_z, 6, WELD_BOTH, 0, 0,
     NULL, big_weld_indices[1]},
    {&big_object, &big_upper_arm_plus_z, &big_forearm_plus_z, 6, WELD_BOTH, 0,
     0, NULL, big_weld_indices[0]},
    {&big_object, &big_forearm_plus_z, &big_hand_plus_z, 6, WELD_BOTH, 0, 0,
     NULL, big_weld_indices[1]},
    {&big_object, &big_thigh_minus_z, &big_shin_minus_z, 6, WELD_TO_A, 0, 0,
     NULL, big_weld_indices[2]},
    {&big_object, &big_thigh_plus_z, &big_shin_plus_z, 6, WELD_TO_A, 0, 0, NULL,
     big_weld_indices[2]},
    {&big_object, &big_foot_minus_z, &big_toes_minus_z, 6, WELD_TO_A, 0, 0,
     NULL, big_weld_indices[3]},
    {&big_object, &big_foot_plus_z, &big_toes_plus_z, 6, WELD_TO_A, 0, 0, NULL,
     big_weld_indices[3]},
    {&big_object, &big_fingers_minus_z, NULL, 2, WELD_MARK, 0, 0, NULL, NULL},
    {&big_object, &big_fingers_plus_z, NULL, 2, WELD_MARK + 1, 0, 0, NULL,
     NULL},
    {&big_object, &big_toes_minus_z, NULL, 0, WELD_MARK + 2, 0, 0, NULL, NULL},
    {&big_object, &big_toes_plus_z, NULL, 0, WELD_MARK + 3, 0, 0, NULL, NULL},
    {&big_object, big_rod, NULL, 0, WELD_MARK + 4, 0, 0, NULL, NULL},
    {&big_object, &big_mouth, NULL, 0, WELD_MARK + 5, 0, 0, NULL, NULL},
    {NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL},
};

/* his upward acceleration as he floats, for each unit of his depth below
 * the surface above him, and the most it can be */
static Float big_float_rate = 0.01F;
static Float big_float_max = 0.089F;

struct task *big_lure;
struct task *big_catch;
int fishing_fish_count;
Bool fishing_shelter_ready;

void big_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;

    if ((work->flags & COLLISION_ON_TOP) && motion->speed.y <= 0.0F) {
        work->flags |= PLAYER_ON_TASK;
        /* his collision's owner is his own task, so these stores copy his
         * own angles back unchanged */
        work->ang.x = work->collision->owner->work->ang.x;
        work->ang.z = work->collision->owner->work->ang.z;
    } else {
        work->flags &= ~PLAYER_ON_TASK;
    }
    if (work->mode != BIG_DOWN) {
        big_choose_mode(work, motion, record);
    }
    if (record != NULL && (BIG_FLAGS(record) & BIG_START_FISHING)) {
        /* @hack spelling: the new target through a local, which SHC spills to
         * the frame across the call and reloads for the store (MOV.L R0,@R15
         * at 0x8c0e9ec8). stored straight, SHC forms the store's address
         * before the call, and 81 of 1388 words differ */
        struct task *target = fishing_create_target(task);

        BIG_TARGET(record) = target;
        BIG_FLAGS(record) &= ~BIG_START_FISHING;
    }
    if (record != NULL && (BIG_FLAGS(record) & BIG_START_CAST)) {
        fishing_flags |= FISHING_CAST;
        BIG_FLAGS(record) &= ~BIG_START_CAST;
    }
    /* PLAYER_GUARD marks a swing of his rod, whose shape is on only then */
    if (work->collision != NULL) {
        struct collision_shape *shapes = work->collision->shapes;

        if (record->effects & PLAYER_INVINCIBLE) {
            shapes->levels = (shapes->levels & ~SHAPE_ATTACK) | ATTACK(3);
            shapes->levels = (shapes->levels & ~SHAPE_GUARD) | GUARD(3);
        } else if (work->mode == BIG_MODULE_POSE) {
            shapes->levels = (shapes->levels & ~SHAPE_ATTACK) | ATTACK(0);
            shapes->levels = (shapes->levels & ~SHAPE_GUARD) | GUARD(1);
        } else if (work->flags & PLAYER_GUARD) {
            shapes[1].flags &= ~SHAPE_OFF;
            shapes->levels = (shapes->levels & ~SHAPE_ATTACK) | ATTACK(0);
            shapes->levels = (shapes->levels & ~SHAPE_GUARD) | GUARD(0);
        } else {
            shapes[1].flags |= SHAPE_OFF;
            shapes->levels = (shapes->levels & ~SHAPE_ATTACK) | ATTACK(0);
            shapes->levels = (shapes->levels & ~SHAPE_GUARD) | GUARD(0);
        }
    }
    switch (work->mode) {
    case BIG_INIT: {
        struct task *child;

        PLAYER_VARS(work)->character = CHARACTER_BIG;
        player_init(PLAYER_VARS(work)->player, task);
        record = motion->record;
        record->anim.actions = big_actions;
        record->anim.action = ACTION_STAND;
        BIG_FLAGS(record) = 0;
        collision_init(task, big_shapes, COLLISION_PLAYER, 1, 2);
        work->collision->shapes->radius = record->consts.radius;
        work->mode = BIG_STAND;
        texture_load_character(CHARACTER_BIG);
        big_look_create(PLAYER_VARS(work)->player, big_look_nodes);
        record->anim.welds = big_welds;
        child = task_create(TASK_WORK, 3, big_mouth_start);
        if (child != NULL) {
            PLAYER_VARS(child->work)->player = PLAYER_VARS(work)->player;
        }
        child = task_create(TASK_MOTION | TASK_WORK, 3, big_sway_step);
        if (child != NULL) {
            PLAYER_VARS(child->work)->player = PLAYER_VARS(work)->player;
        }
        if (PLAYER_VARS(work)->player == 0) {
            child = fishing_create_lure(task);
            BIG_LURE(record) = big_lure = child;
            /* @hack redundant: a second test of player 0, inside the first,
             * which the image makes again (MOV.B @(8,R3),R0 at 0x8c0ea146).
             * without it, 33 of 1388 words differ */
            if (PLAYER_VARS(work)->player == 0) {
                child = task_create(TASK_WORK, 3, fishing_hud_step);
            }
        }
        fishing_fish_count = 0;
        fishing_shelter_ready = FALSE;
        fishing_flags = 0;
        task->destructor = big_destroy;
        task->draw = big_draw;
        break;
    }
    case BIG_STAND:
        player_turn_to_stick(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        big_set_stand_action(work, motion, record);
        break;
    case BIG_WALK:
        if (record->anim.previous == ACTION_TURN &&
            record->anim.current != ACTION_TURN) {
            record->speed.x += record->consts.trot_speed;
        }
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        if (player_collide(work, motion, record) == PLAYER_CRASHED) {
            sound_play(SE_CRASH, 0, 0, 0);
            work->mode = BIG_CRASH;
            record->anim.action = ACTION_CRASH;
            player_crash_burst(work);
            record->speed.x = -record->speed.x;
            record->speed.z = -record->speed.z;
        } else {
            player_take_speed(work, motion, record);
            big_set_walk_action(work, motion, record);
        }
        break;
    case BIG_PATH:
        player_accel_path(work, motion, record);
        player_apply_path_accel(work, motion, record);
        player_move_on_path(work, motion, record);
        big_set_walk_action(work, motion, record);
        break;
    case BIG_JUMP:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_SPRING:
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_LAUNCH:
        if (record->speed.y >= record->consts.launch_speed) {
            player_accel_launched(work, motion, record);
        } else {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_FLUNG:
        player_accel_gravity(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_FALL:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_STOP:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_PUSH:
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
    case BIG_HURT:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_CRASH:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_SLIDE:
        player_turn_to_stick(work, motion, record);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_LEDGE:
    case BIG_LEDGE_GO:
        player_shift_on_path(work, motion, record);
        big_ledge_move(work, motion, record);
        break;
    case BIG_SWING:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_POSE:
        break;
    case BIG_CAST:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_AIM:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_REEL:
    case BIG_REEL_TURN:
    case BIG_CATCH:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_FLOAT_CAST:
        big_float_up(work, motion, record);
        player_turn_to_stick(work, motion, record);
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_FLOAT_AIM:
    case BIG_FLOAT_CAST_SWING:
    case BIG_FLOAT_REEL:
    case BIG_FLOAT_REEL_TURN:
    case BIG_FLOAT_CATCH:
        big_float_up(work, motion, record);
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_FLOAT:
        big_float_up(work, motion, record);
        player_turn_to_stick(work, motion, record);
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        if (record->shadow.above < work->pos.y + record->consts.eye_height) {
            record->anim.action = ACTION_FLOAT;
        }
        break;
    case BIG_SWIM:
        big_float_up(work, motion, record);
        player_turn_upright(work, motion, record);
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_DIVE:
        player_turn_upright(work, motion, record);
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_LIFT:
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_PUT_DOWN:
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_HOLD_STAND:
        player_turn_to_stick(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_GRIP:
        player_accel_push(work, motion, record);
        player_apply_push(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_THROW:
        player_turn_to_target(work, motion, record);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_HOLD_WALK:
        if (record->anim.previous == ACTION_HOLD_TURN &&
            record->anim.current != ACTION_HOLD_TURN) {
            record->speed.x += record->consts.trot_speed;
        }
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        big_set_walk_action(work, motion, record);
        break;
    case BIG_HOLD_STOP:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_HOLD_FALL:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_HOLD_JUMP:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_HOLD_SPRING:
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_HOLD_LAUNCH:
        if (record->speed.y >= record->consts.launch_speed) {
            player_accel_launched(work, motion, record);
        } else {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_PULL:
        player_accel_push(work, motion, record);
        player_apply_push(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        if (player_carry_flags(work) & PLAYER_CAN_PUSH) {
            sound_play_for(SE_PUSH, (int)work, 1, 0, 2);
        }
        break;
    case BIG_AIR_THROW:
        player_turn_upright(work, motion, record);
        player_turn_to_target(work, motion, record);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_SHAKE:
        player_shake_touched(work, motion, record);
        break;
    case BIG_HOLD_SHAKE:
        record->held->motion->accel.x = record->held->motion->accel.y =
            record->held->motion->accel.z = record->anim.frame * 0.01F;
        break;
    case BIG_MODULE_POSE:
        break;
    case BIG_BREATHE:
        player_turn_to_stick(work, motion, record);
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case BIG_EVENT:
        event_paths_step(work);
        break;
    case BIG_RISE:
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
    case BIG_DOWN:
        if (work->flags & PLAYER_ON_GROUND) {
            record->anim.action = ACTION_DOWN_LAND;
        }
        player_turn_upright(work, motion, record);
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    }
    player_anim_step(&record->anim);
    if (record->upgrades & PLAYER_BIG_LIFE_BELT) {
        big_neck->sibling = big_belt_torso;
        if (big_can_float(work, motion, record)) {
            if (BIG_BELT(record) < 1.0F) {
                BIG_BELT(record) += 0.05F;
            } else {
                BIG_BELT(record) = 1.0F;
            }
        } else {
            if (BIG_BELT(record) > 0.0F) {
                BIG_BELT(record) -= 0.05F;
            } else {
                BIG_BELT(record) = 0.0F;
            }
        }
        points_blend(big_belt_torso_flat, big_belt_torso_full,
                     big_belt_torso_points, BIG_BELT(record),
                     big_belt_torso->model->nbPoint);
        points_blend(big_belt_piece_flat, big_belt_piece_full,
                     big_belt_piece_points, BIG_BELT(record),
                     big_belt_torso->child->model->nbPoint);
    } else {
        big_neck->sibling = &big_torso;
    }
    if (record->upgrades & PLAYER_BIG_POWER_ROD) {
        big_rod->model = &big_power_rod_model;
        big_rod->child = &big_power_rod_reel;
    } else {
        big_rod->model = &big_rod_model;
        big_rod->child = &big_rod_reel;
    }
    player_update_welds(work, motion, record);
    big_move_shapes(work, motion, record);
    /* his step draws him, and so does task->draw when task_draw_all runs,
     * as in the pause menu */
    big_draw(task);
    if (record->held != NULL) {
        big_carry(work, motion, record);
    }
    shadow_update(work, &record->shadow);
    /* nothing touches him in ACTION_CLEAR, the stage's clear pose */
    if (record->anim.current != ACTION_CLEAR) {
        collision_register(work);
    }
    player_finish_step(work, motion, record);
}

/* NOLINTNEXTLINE(readability-function-size) */
static void big_choose_mode(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    if (record != NULL) {
        if (record->effects & PLAYER_INVINCIBLE) {
            work->flags |= PLAYER_RECOILING;
            work->timer = 0;
        } else if (work->timer == 0) {
            if (big_try_hurt(work, motion, record)) {
                work->flags |= PLAYER_RECOILING;
                if (work->flags & PLAYER_HOLDING) {
                    big_let_go(work, motion, record);
                }
                work->flags &= ~(PLAYER_ON_PATH | PLAYER_REQUEST | PLAYER_BALL);
                if (fishing_flags & FISHING_HOOKED) {
                    fishing_flags |= FISHING_LOST;
                }
                if (record->effects &
                    (PLAYER_BARRIER | PLAYER_MAGNETIC_BARRIER)) {
                    record->effects &=
                        ~(PLAYER_BARRIER | PLAYER_MAGNETIC_BARRIER);
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
    }
    switch (work->mode) {
    case BIG_STAND:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (big_try_float(work, motion, record)) {
            break;
        }
        if (big_try_fall(work, motion, record)) {
            break;
        }
        if (big_try_jump(work, motion, record)) {
            break;
        }
        if (big_try_grab(work, motion, record)) {
            break;
        }
        if (big_try_cast(work, motion, record)) {
            break;
        }
        if (player_read_stick(work, NULL, NULL)) {
            work->mode = BIG_WALK;
            record->anim.action = ACTION_WALK;
            break;
        } else if (record->contact->normal.y < 0.77F &&
                   njScalor(&record->speed) > record->consts.slide_speed) {
            work->mode = BIG_SLIDE;
            record->anim.action = ACTION_SLIDE;
        }
        break;
    case BIG_WALK:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (big_try_float(work, motion, record) ||
            big_try_fall(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (big_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (big_try_cast(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (big_try_stop(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if ((work->collision->flags & COLLISION_PUSHED) &&
            !(work->flags & PLAYER_HURT) &&
            !(work->collision->hit->shapes[work->collision->hit_shape].flags &
              SHAPE_NO_PUSH) &&
            (work->collision->flags & COLLISION_HIT_SIDE)) {
            /* @hack spelling: a switch of one case, which keeps work in R2 and
             * the values its arm stores in R3 (0x8c0eb184 to 0x8c0eb190). as
             * an if, the two trade and 6 of 4304 words differ */
            switch (player_stick_into_wall(work, motion)) {
            case PLAYER_WALL_HEAD_ON:
                work->mode = BIG_PUSH;
                record->anim.action = ACTION_PUSH_START;
                work->flags &= ~PLAYER_ON_PATH;
                break;
            }
        }
        if (player_stick_back_or_end_event(work, motion) &&
            record->speed.x >= record->consts.trot_speed) {
            work->mode = BIG_STOP;
            record->anim.action = ACTION_STOP;
            sound_play(SE_SKID, 0, 0, 0);
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        break;
    case BIG_PATH:
        if (big_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (big_try_fall(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (big_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (record->speed.x < record->consts.trot_speed) {
            work->mode = BIG_WALK;
            record->anim.action = ACTION_WALK;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (big_try_stop(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        break;
    case BIG_JUMP:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (big_try_float(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            sound_play(SE_LAND, 0, 0, 0);
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = BIG_STOP;
                record->anim.action = ACTION_STOP;
            } else if (big_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                record->anim.action = ACTION_LAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = BIG_WALK;
                big_set_walk_action(work, motion, record);
            }
            break;
        }
        if (motion->speed.y <= 0.0F) {
            record->anim.action = ACTION_FALL;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.8F;
        }
        break;
    case BIG_SPRING:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (big_try_float(work, motion, record)) {
            break;
        }
        if (motion->speed.y <= 0.0F) {
            work->mode = BIG_FALL;
            record->anim.action = ACTION_FALL_START;
        }
        break;
    case BIG_LAUNCH:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (big_try_float(work, motion, record)) {
            break;
        }
        if (motion->speed.y <= 0.0F) {
            work->mode = BIG_FALL;
            record->anim.action = ACTION_FALL_START;
        }
        break;
    case BIG_FLUNG:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (big_try_float(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            sound_play(SE_LAND, 0, 0, 0);
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = BIG_STOP;
                record->anim.action = ACTION_STOP;
            } else if (big_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                record->anim.action = ACTION_STAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = BIG_WALK;
                big_set_walk_action(work, motion, record);
            }
        }
        break;
    case BIG_FALL:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (big_try_float(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            sound_play(SE_LAND, 0, 0, 0);
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = BIG_STOP;
                record->anim.action = ACTION_STOP;
            } else if (big_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                record->anim.action = ACTION_LAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = BIG_WALK;
                big_set_walk_action(work, motion, record);
            }
        }
        break;
    case BIG_STOP:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (big_try_float(work, motion, record)) {
            break;
        }
        if (big_try_fall(work, motion, record)) {
            break;
        }
        if (big_try_jump(work, motion, record)) {
            break;
        }
        if (big_try_cast(work, motion, record)) {
            break;
        }
        if (record->speed.x <= 0.0F &&
            player_stick_back_or_end_event(work, motion)) {
            work->mode = BIG_WALK;
            record->anim.action = ACTION_TURN;
            player_turn_to_stick(work, motion, record);
            work->ang.y = motion->aim.y;
            break;
        }
        if (record->anim.action != ACTION_STOP) {
            work->mode = BIG_STAND;
            record->anim.action = ACTION_STAND;
        }
        break;
    case BIG_PUSH:
        if (big_try_request(work, motion, record) ||
            big_try_float(work, motion, record) ||
            big_try_jump(work, motion, record) ||
            big_try_fall(work, motion, record) ||
            big_try_stop(work, motion, record)) {
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
                    work->mode = BIG_WALK;
                    record->anim.action = ACTION_WALK;
                    sound_stop(SE_PUSH);
                }
                break;
            default:
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = BIG_PULL;
                    record->anim.action = ACTION_PULL;
                } else {
                    work->mode = BIG_WALK;
                    record->anim.action = ACTION_WALK;
                }
                sound_stop(SE_PUSH);
                break;
            }
            break;
        }
        big_let_go(work, motion, record);
        work->mode = BIG_WALK;
        record->anim.action = ACTION_WALK;
        sound_stop(SE_PUSH);
        break;
    case BIG_HURT:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_HURT &&
            record->anim.action != ACTION_HURT_LAND) {
            if (big_try_stop(work, motion, record)) {
                work->mode = BIG_STAND;
                record->anim.action = ACTION_STAND;
                break;
            } else {
                work->mode = BIG_WALK;
                big_set_walk_action(work, motion, record);
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
    case BIG_CRASH:
        if (big_try_float(work, motion, record)) {
            break;
        }
        if (big_try_fall(work, motion, record)) {
            break;
        }
        if (record->anim.action == ACTION_CRASH) {
            /* @hack spelling: an empty arm, which loads work and the record
             * into R0 for the else's stores (0x8c0ebaac, 0x8c0ebab6). tested as
             * one with &&, they load into R2 and 7 of 4304 words differ */
        } else if (record->anim.action != ACTION_CRASH_END) {
            work->mode = BIG_STAND;
            record->anim.action = ACTION_STAND;
        }
        break;
    case BIG_SLIDE:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (big_try_float(work, motion, record)) {
            break;
        }
        if (big_try_fall(work, motion, record)) {
            break;
        }
        if (big_try_jump(work, motion, record)) {
            break;
        }
        if (player_read_stick(work, NULL, NULL)) {
            work->mode = BIG_WALK;
            record->anim.action = ACTION_WALK;
            break;
        }
        if (njScalor(&motion->speed) < record->consts.slide_speed ||
            record->contact->normal.y > 0.98F) {
            work->mode = BIG_STAND;
            record->anim.action = ACTION_STAND;
        }
        break;
    case BIG_SWING:
        if (big_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_GUARD;
            break;
        }
        if (record->anim.action != ACTION_SWING) {
            work->flags &= ~PLAYER_GUARD;
            work->mode = BIG_STAND;
            record->anim.action = ACTION_STAND;
            /* @hack redundant: a redundant break, which puts work in R3, not
             * R2, for BIG_POSE's test (0x8c0ebc20); without it, 3 of 4304 words
             * differ */
            break;
        }
        break;
    case BIG_POSE:
        if (work->flags & PLAYER_HOLDING) {
            if (big_try_request_holding(work, motion, record)) {
                break;
            }
        } else {
            if (big_try_request(work, motion, record)) {
                break;
            }
        }
        break;
    case BIG_LEDGE: {
        Angle ang;

        if (big_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (big_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (work->flags & PLAYER_ON_PATH) {
            if (!player_read_stick(work, &ang, NULL)) {
                break;
            }
            /* the stick, as the camera sees it, moves him one way or the
             * other along the ledge's path */
            if (angle_distance(0xc000 - camera_work->ang.y, ang) < 0x4000) {
                record->speed.x = 0.75F;
                /* @hack redundant: a redundant break, which gives this arm a
                 * store and jump of its own (JMP @R2 at 0x8c0ebd02, the store
                 * in its slot). without it, the else's arm loads its speed and
                 * branches to that store and jump, and 27 of 4304 words
                 * differ */
                break;
            } else {
                /* @hack redundant: an else after an arm that breaks, which puts
                 * the record in R2, not R3, for this store and jump
                 * (0x8c0ebd08); without it, 10 of 4304 words differ */
                record->speed.x = -0.75F;
                /* @hack redundant: a redundant break, which leaves the JMP's
                 * delay slot a NOP (0x8c0ebd14). without it, the store fills
                 * the slot and 21 of 4304 words differ */
                break;
            }
            /* @hack redundant: a break after an if whose arms both break,
             * which puts work in R1 and BIG_FALL in R2 for the fall's stores
             * (0x8c0ebd16, 0x8c0ebd18). without it, the two trade and 6 of
             * 4304 words differ */
            break;
        } else {
            /* @hack redundant: an else after an arm that breaks, which keeps
             * work in R1 and BIG_FALL in R2 (0x8c0ebd16, 0x8c0ebd18). without
             * it, R2 and R3 take them, and 13 of 4304 words differ */
            work->mode = BIG_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
    }
    case BIG_LEDGE_GO:
        if (big_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (big_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (work->flags & PLAYER_ON_PATH) {
            /* @hack spelling: an empty arm, which puts work in R2 and BIG_FALL
             * in R1 for the else's stores (0x8c0ebdcc, 0x8c0ebdce). tested as
             * if (!...), the two trade and 6 of 4304 words differ */
        } else {
            work->mode = BIG_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a redundant break, which leaves the JMP's delay
             * slot a NOP (0x8c0ebde8). without it, the flags' store fills the
             * slot and 9 of 4304 words differ */
            break;
        }
        break;
    case BIG_CAST:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].release,
                        pad_action_buttons)) {
            BIG_FLAGS(record) |= BIG_CAST_RELEASED;
        }
        if (record->anim.flags & PLAYER_ACTION_DONE) {
            if (BIG_FLAGS(record) & BIG_CAST_RELEASED) {
                record->anim.action = ACTION_CAST_SWING;
                work->mode = BIG_CAST_SWING;
                work->flags |= PLAYER_GUARD;
            } else {
                BIG_FLAGS(record) |= BIG_START_FISHING;
                work->mode = BIG_AIM;
                big_take_camera();
            }
            BIG_FLAGS(record) &= ~BIG_CAST_RELEASED;
        }
        break;
    case BIG_AIM:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (!big_eyes_above_water(work, motion, record)) {
            big_give_camera();
            work->mode = BIG_STAND;
            record->anim.action = ACTION_STAND;
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].release,
                        pad_action_buttons)) {
            record->anim.action = ACTION_CAST;
            work->mode = BIG_REEL;
            BIG_FLAGS(record) |= BIG_START_CAST;
            BIG_FLAGS(record) &= ~BIG_FISHING_OVER;
            /* @hack redundant: a redundant break, which leaves the JMP's delay
             * slot a NOP (0x8c0ebf64). without it, the store to BIG_FLAGS
             * fills the slot, the code after it moves, and 2597 of 4304 words
             * differ */
            break;
        }
        break;
    case BIG_CAST_SWING:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_CAST_SWING) {
            work->mode = BIG_STAND;
            work->flags &= ~PLAYER_GUARD;
        }
        break;
    case BIG_REEL:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (!big_eyes_above_water(work, motion, record)) {
            BIG_FLAGS(record) |= BIG_FISHING_OVER;
            if (big_catch != NULL && (fishing_flags & FISHING_HOOKED)) {
                fishing_flags |= FISHING_LOST;
            }
        }
        if (BIG_FLAGS(record) & BIG_FISHING_OVER) {
            if (fishing_flags & FISHING_LANDED) {
                if (fishing_flags & FISHING_HOOKED) {
                    struct angles ang = {0, 0, 0};

                    restart_save(&work->pos, &ang);
                    work->mode = BIG_CATCH;
                    work->tick = CATCH_STEPS;
                    big_land_catch(work, motion, record);
                    break;
                }
            }
            big_give_camera();
            work->mode = BIG_STAND;
            record->anim.action = ACTION_STAND;
            BIG_FLAGS(record) &= ~BIG_FISHING_OVER;
            break;
        }
        if (fishing_flags & FISHING_SNAPPED) {
            big_snap_line(work, motion, record);
            break;
        }
        /* the stand's motion, not the reel's, sets when he may turn the reel.
         * its 32 frames less 3 make 29, the last of ACTION_REEL's 30 */
        if (record->anim.frame >=
            record->anim.actions[ACTION_STAND].action->motion->nbFrame - 3.0F) {
            Sint16 action;
            Angle ang;

            if (player_read_stick(work, &ang, NULL)) {
                if (camera_work != NULL) {
                    ang = (Uint16)(ang + camera_work->ang.y + 0x4000);
                } else {
                    ang = 0;
                }
                if (ang > 0x4000) {
                    if (ang <= 0x5800) {
                        action = ACTION_REEL_RIGHT;
                    } else if (ang <= 0x7000) {
                        action = ACTION_REEL_DOWN_RIGHT;
                    } else if (ang <= 0x9000) {
                        action = ACTION_REEL_DOWN;
                    } else if (ang <= 0xa800) {
                        action = ACTION_REEL_DOWN_LEFT;
                    } else if (ang <= 0xc000) {
                        action = ACTION_REEL_LEFT;
                    } else {
                        break;
                    }
                    work->mode = BIG_REEL_TURN;
                    record->anim.action = action;
                }
            }
        }
        break;
    case BIG_REEL_TURN:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (BIG_FLAGS(record) & BIG_FISHING_OVER) {
            if (fishing_flags & FISHING_LANDED) {
                if (fishing_flags & FISHING_HOOKED) {
                    struct angles ang = {0, 0, 0};

                    restart_save(&work->pos, &ang);
                    work->mode = BIG_CATCH;
                    work->tick = CATCH_STEPS;
                    big_land_catch(work, motion, record);
                    break;
                }
            }
            big_give_camera();
            work->mode = BIG_STAND;
            record->anim.action = ACTION_STAND;
            BIG_FLAGS(record) &= ~BIG_FISHING_OVER;
            break;
        }
        if (fishing_flags & FISHING_SNAPPED) {
            big_snap_line(work, motion, record);
            break;
        }
        record->anim.start_frame =
            record->anim.actions[ACTION_REEL_HELD].action->motion->nbFrame -
            1.0F;
        if (record->anim.frame >= 10.0F && record->anim.frame <= 30.0F) {
            fishing_flags |= FISHING_PULL;
        } else {
            fishing_flags &= ~FISHING_PULL;
        }
        if (record->anim.action < ACTION_REEL_DOWN ||
            record->anim.action > ACTION_REEL_LEFT) {
            work->mode = BIG_REEL;
            fishing_flags &= ~FISHING_PULL;
        }
        break;
    case BIG_CATCH:
        if (big_lure != NULL) {
            fishing_look_at_big(big_lure);
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                        pad_action_buttons) ||
            --work->tick <= 0) {
            work->mode = BIG_STAND;
            big_catch = NULL;
            camera_restore();
            if (fishing_flags & FISHING_GOAL) {
                stage_clear();
            } else {
                record->anim.action = ACTION_STAND;
                BIG_FLAGS(record) &= ~BIG_FISHING_OVER;
            }
            fishing_flags = 0;
        }
        if (big_try_request(work, motion, record)) {
            break;
        }
        break;
    case BIG_BREATHE:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (record->anim.flags & PLAYER_ACTION_DONE) {
            work->mode = BIG_STAND;
            record->anim.action = ACTION_STAND;
        }
        break;
    case BIG_SNAPPED:
        if (record->anim.action != ACTION_SNAP) {
            /* @hack type: the player cast to Sint8, as MOV.B loads it, which
             * passes it on as loaded (MOV R0,R4 at 0x8c0ec512). as the Uint8
             * it is, it takes an EXTU.B there, and 1 of 4304 words differs */
            player_down((Sint8)PLAYER_VARS(work)->player);
        }
        break;
    case BIG_FLOAT_CAST:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].release,
                        pad_action_buttons)) {
            BIG_FLAGS(record) |= BIG_CAST_RELEASED;
        }
        if (record->anim.flags & PLAYER_ACTION_DONE) {
            if (BIG_FLAGS(record) & BIG_CAST_RELEASED) {
                record->anim.action = ACTION_FLOAT_REEL_HELD;
                work->mode = BIG_FLOAT_CAST_SWING;
                work->flags |= PLAYER_GUARD;
            } else {
                BIG_FLAGS(record) |= BIG_START_FISHING;
                work->mode = BIG_FLOAT_AIM;
                big_take_camera();
            }
            BIG_FLAGS(record) &= ~BIG_CAST_RELEASED;
        }
        break;
    case BIG_FLOAT_AIM:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].release,
                        pad_action_buttons)) {
            record->anim.action = ACTION_FLOAT_CAST;
            work->mode = BIG_FLOAT_REEL;
            BIG_FLAGS(record) |= BIG_START_CAST;
            BIG_FLAGS(record) &= ~BIG_FISHING_OVER;
        }
        break;
    case BIG_FLOAT_CAST_SWING:
        if (big_try_request(work, motion, record)) {
            break;
        }
        /* @bug logic: BIG_FLOAT_CAST starts the swing in
         * ACTION_FLOAT_REEL_HELD (MOV #68 at 0x8c0ec594), but this waits for
         * the action to leave ACTION_FLOAT_CAST (CMP/EQ #67 at 0x8c0ec678).
         * the test holds at once, so the swing ends on its next step */
        if (record->anim.action != ACTION_FLOAT_CAST) {
            work->mode = BIG_STAND;
            work->flags &= ~PLAYER_GUARD;
        }
        break;
    case BIG_FLOAT_REEL:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (BIG_FLAGS(record) & BIG_FISHING_OVER) {
            if (fishing_flags & FISHING_LANDED) {
                if (fishing_flags & FISHING_HOOKED) {
                    struct angles ang = {0, 0, 0};

                    restart_save(&work->pos, &ang);
                    work->mode = BIG_FLOAT_CATCH;
                    work->tick = CATCH_STEPS;
                    big_land_catch(work, motion, record);
                    break;
                }
            }
            big_give_camera();
            work->mode = BIG_FLOAT;
            record->anim.action = ACTION_FLOAT;
            BIG_FLAGS(record) &= ~BIG_FISHING_OVER;
            break;
        }
        if (fishing_flags & FISHING_SNAPPED) {
            big_snap_line(work, motion, record);
            break;
        }
        /* the stand's motion, not the cast's, sets when he may turn the reel,
         * as on land */
        if (record->anim.frame >=
            record->anim.actions[ACTION_STAND].action->motion->nbFrame - 3.0F) {
            Sint16 action;
            Angle ang;

            if (player_read_stick(work, &ang, NULL)) {
                if (camera_work != NULL) {
                    ang = (Uint16)(ang + camera_work->ang.y + 0x4000);
                } else {
                    ang = 0;
                }
                if (ang > 0x4000) {
                    if (ang <= 0x5800) {
                        action = ACTION_FLOAT_REEL_RIGHT;
                    } else if (ang <= 0x7000) {
                        action = ACTION_FLOAT_REEL_DOWN_RIGHT;
                    } else if (ang <= 0x9000) {
                        action = ACTION_FLOAT_REEL_DOWN;
                    } else if (ang <= 0xa800) {
                        action = ACTION_FLOAT_REEL_DOWN_LEFT;
                    } else if (ang <= 0xc000) {
                        action = ACTION_FLOAT_REEL_LEFT;
                    } else {
                        break;
                    }
                    work->mode = BIG_FLOAT_REEL_TURN;
                    record->anim.action = action;
                }
            }
        }
        break;
    case BIG_FLOAT_REEL_TURN:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (BIG_FLAGS(record) & BIG_FISHING_OVER) {
            if (fishing_flags & FISHING_LANDED) {
                if (fishing_flags & FISHING_HOOKED) {
                    struct angles ang = {0, 0, 0};

                    restart_save(&work->pos, &ang);
                    work->mode = BIG_FLOAT_CATCH;
                    work->tick = CATCH_STEPS;
                    big_land_catch(work, motion, record);
                    break;
                }
            }
            big_give_camera();
            work->mode = BIG_FLOAT;
            record->anim.action = ACTION_FLOAT;
            BIG_FLAGS(record) &= ~BIG_FISHING_OVER;
            break;
        }
        if (fishing_flags & FISHING_SNAPPED) {
            big_snap_line(work, motion, record);
            break;
        }
        record->anim.start_frame = record->anim.actions[ACTION_FLOAT_REEL_HELD]
                                       .action->motion->nbFrame -
                                   1.0F;
        if (record->anim.frame >= 10.0F && record->anim.frame <= 20.0F) {
            fishing_flags |= FISHING_PULL;
        } else {
            fishing_flags &= ~FISHING_PULL;
        }
        if (record->anim.action < ACTION_FLOAT_REEL_DOWN ||
            record->anim.action > ACTION_FLOAT_REEL_LEFT) {
            work->mode = BIG_FLOAT_REEL;
            fishing_flags &= ~FISHING_PULL;
        }
        break;
    case BIG_FLOAT_CATCH:
        if (big_lure != NULL) {
            fishing_look_at_big(big_lure);
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                        pad_action_buttons) ||
            --work->tick <= 0) {
            work->mode = BIG_FLOAT;
            big_catch = NULL;
            camera_restore();
            if (fishing_flags & FISHING_GOAL) {
                stage_clear();
            } else {
                record->anim.action = ACTION_FLOAT;
                BIG_FLAGS(record) &= ~BIG_FISHING_OVER;
            }
            fishing_flags = 0;
        }
        if (big_try_request(work, motion, record)) {
            break;
        }
        break;
    case BIG_FLOAT:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (!big_belt_in_water(work, motion, record)) {
            work->mode = BIG_STAND;
            record->anim.action = ACTION_STAND;
            break;
        }
        if (big_try_float_cast(work, motion, record)) {
            break;
        }
        if (work->collision->hit != NULL &&
            work->collision->hit->owner->work->id == JUMP_BOX_ID &&
            big_try_jump(work, motion, record)) {
            break;
        }
        if (player_read_stick(work, NULL, NULL)) {
            work->mode = BIG_SWIM;
            record->anim.action = ACTION_SWIM_START;
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                        pad_jump_buttons)) {
            work->mode = BIG_DIVE;
            record->anim.action = ACTION_DIVE_START;
            /* @hack redundant: a redundant break, which leaves the JMP's delay
             * slot a NOP (0x8c0ecc72). without it, the action's store fills
             * the slot and 32 of 4304 words differ */
            break;
        }
        break;
    case BIG_SWIM:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (!big_belt_in_water(work, motion, record)) {
            work->mode = BIG_WALK;
            record->anim.action = ACTION_WALK;
            break;
        }
        if (big_try_float_cast(work, motion, record)) {
            break;
        }
        if (work->collision->hit != NULL &&
            work->collision->hit->owner->work->id == JUMP_BOX_ID &&
            big_try_jump(work, motion, record)) {
            break;
        }
        if (!player_read_stick(work, NULL, NULL)) {
            work->mode = BIG_FLOAT;
            record->anim.action = ACTION_FLOAT_UNDER_START;
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                        pad_jump_buttons)) {
            work->mode = BIG_DIVE;
            record->anim.action = ACTION_DIVE_START;
            /* @hack redundant: a redundant break, which leaves the JMP's delay
             * slot a NOP (0x8c0ecd78). without it, the action's store fills
             * the slot and 5 of 4304 words differ */
            break;
        }
        break;
    case BIG_DIVE:
        if (big_try_request(work, motion, record)) {
            break;
        }
        if (big_can_float(work, motion, record)) {
            work->mode = BIG_FLOAT;
            record->anim.action = ACTION_FLOAT_UNDER_START;
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = BIG_STOP;
                record->anim.action = ACTION_STOP;
            } else if (big_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                record->anim.action = ACTION_LAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = BIG_WALK;
                big_set_walk_action(work, motion, record);
            }
        }
        if (!PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                         pad_jump_buttons)) {
            work->mode = BIG_FLOAT;
            record->anim.action = ACTION_FLOAT_UNDER_START;
            break;
        }
        if (player_read_stick(work, NULL, NULL)) {
            work->mode = BIG_SWIM;
            record->anim.action = ACTION_SWIM_START;
            /* @hack redundant: a redundant break, which puts work in R3, not
             * R1, for BIG_LIFT's test (0x8c0eced6); without it, 3 of 4304 words
             * differ */
            break;
        }
        break;
    case BIG_LIFT:
        if (work->flags & PLAYER_HOLDING) {
            if (big_try_request_holding(work, motion, record)) {
                break;
            }
        } else {
            if (big_try_request(work, motion, record)) {
                break;
            }
        }
        if (big_try_fall(work, motion, record)) {
            break;
        }
        if (big_try_take_lifted(work, motion, record)) {
            break;
        }
        break;
    case BIG_GRIP:
        if (big_try_grip(work, motion, record)) {
            break;
        }
        if (big_try_jump(work, motion, record)) {
            break;
        }
        break;
    case BIG_HOLD_STAND:
        if (big_try_request_holding(work, motion, record)) {
            break;
        }
        if (big_try_fall(work, motion, record)) {
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                        pad_grab_buttons) &&
            player_read_stick(work, NULL, NULL) &&
            record->held->work->id == CARRY_KIND_SMALL) {
            work->mode = BIG_HOLD_SHAKE;
            record->anim.action = ACTION_HOLD_SHAKE_START;
            break;
        }
        if (big_try_throw(work, motion, record)) {
            break;
        }
        if (big_try_jump(work, motion, record)) {
            break;
        }
        if (player_read_stick(work, NULL, NULL)) {
            work->mode = BIG_HOLD_WALK;
            record->anim.action = ACTION_HOLD_WALK;
        }
        break;
    case BIG_HOLD_WALK:
        if (big_try_request_holding(work, motion, record)) {
            break;
        }
        if (big_try_fall(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (big_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (big_try_throw(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (big_try_stop(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (big_try_throw(work, motion, record)) {
            break;
        }
        if (player_stick_back_or_end_event(work, motion) &&
            record->speed.x >= record->consts.trot_speed) {
            work->mode = BIG_HOLD_STOP;
            record->anim.action = ACTION_HOLD_STOP;
            sound_play(SE_SKID, 0, 0, 0);
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a redundant break, which puts work in R3, not
             * R1, for BIG_PUT_DOWN's test (0x8c0ed1ba); without it, 3 of 4304
             * words differ */
            break;
        }
        break;
    case BIG_PUT_DOWN:
    case BIG_THROW:
        if (work->flags & PLAYER_HOLDING) {
            if (big_try_request_holding(work, motion, record)) {
                break;
            }
        } else {
            if (big_try_request(work, motion, record)) {
                break;
            }
        }
        if (big_try_release(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_THROW_AT &&
            record->anim.action != ACTION_THROW &&
            record->anim.action != ACTION_PUT_DOWN &&
            record->anim.action != ACTION_PUT_DOWN_SMALL) {
            work->mode = BIG_STAND;
            record->anim.action = ACTION_STAND;
        }
        break;
    case BIG_HOLD_STOP:
        if (big_try_request_holding(work, motion, record)) {
            break;
        }
        if (big_try_fall(work, motion, record)) {
            break;
        }
        if (big_try_jump(work, motion, record)) {
            break;
        }
        if (record->speed.x <= 0.0F &&
            player_stick_back_or_end_event(work, motion)) {
            work->mode = BIG_HOLD_WALK;
            record->anim.action = ACTION_HOLD_TURN;
            player_turn_to_stick(work, motion, record);
            work->ang.y = motion->aim.y;
            break;
        }
        if (record->anim.action != ACTION_HOLD_STOP) {
            work->mode = BIG_HOLD_STAND;
            big_set_stand_action(work, motion, record);
        }
        break;
    case BIG_HOLD_FALL:
        if (big_try_request_holding(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            sound_play(SE_LAND, 0, 0, 0);
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = BIG_HOLD_STOP;
                record->anim.action = ACTION_HOLD_STOP;
            } else if (big_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (motion->speed.y > -5.0F) {
                    big_set_stand_action(work, motion, record);
                } else {
                    record->anim.action = ACTION_HOLD_LAND;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = BIG_HOLD_WALK;
                big_set_walk_action(work, motion, record);
            }
            break;
        }
        if (big_try_throw(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        break;
    case BIG_HOLD_JUMP:
        if (big_try_request_holding(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            sound_play(SE_LAND, 0, 0, 0);
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = BIG_HOLD_STOP;
                record->anim.action = ACTION_HOLD_STOP;
            } else if (big_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (motion->speed.y > -5.0F) {
                    big_set_stand_action(work, motion, record);
                } else {
                    record->anim.action = ACTION_HOLD_LAND;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = BIG_HOLD_WALK;
                big_set_walk_action(work, motion, record);
            }
            break;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.4F;
        }
        if (big_try_throw(work, motion, record)) {
            break;
        }
        if (motion->speed.y <= 0.0F) {
            work->mode = BIG_HOLD_FALL;
            record->anim.action = ACTION_HOLD_FALL_START;
        }
        break;
    case BIG_HOLD_SPRING:
        if (motion->speed.y <= 0.0F) {
            work->mode = BIG_HOLD_FALL;
            record->anim.action = ACTION_HOLD_FALL_START;
        }
        break;
    case BIG_HOLD_LAUNCH:
        if (motion->speed.y <= 0.0F) {
            work->mode = BIG_HOLD_FALL;
            record->anim.action = ACTION_HOLD_FALL_START;
        }
        break;
    case BIG_PULL:
        if (big_try_request(work, motion, record) ||
            big_try_jump(work, motion, record) ||
            big_try_fall(work, motion, record)) {
            sound_stop(SE_PUSH);
            break;
        }
        if (!PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                         pad_grab_buttons)) {
            work->mode = BIG_STAND;
            record->anim.action = ACTION_STAND;
            big_let_go(work, motion, record);
            sound_stop(SE_PUSH);
        }
        if ((work->collision->flags & COLLISION_PUSHED) &&
            !(work->flags & PLAYER_HURT) &&
            !(work->collision->hit->shapes[work->collision->hit_shape].flags &
              SHAPE_NO_PUSH) &&
            (work->collision->flags & COLLISION_HIT_SIDE)) {
            switch (player_stick_into_wall(work, motion)) {
            case PLAYER_WALL_HEAD_ON:
                work->mode = BIG_PUSH;
                record->anim.action = ACTION_PUSH_START;
                sound_stop(SE_PUSH);
                break;
            case PLAYER_WALL_NO_STICK:
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = BIG_GRIP;
                    record->anim.action = ACTION_GRIP;
                    sound_stop(SE_PUSH);
                }
                break;
            default:
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = BIG_PULL;
                    record->anim.action = ACTION_PULL;
                } else {
                    work->mode = BIG_WALK;
                    record->anim.action = ACTION_WALK;
                }
                sound_stop(SE_PUSH);
                break;
            }
            break;
        } else {
            big_let_go(work, motion, record);
            work->mode = BIG_WALK;
            record->anim.action = ACTION_WALK;
            sound_stop(SE_PUSH);
            /* @hack redundant: a redundant break, which makes this arm's
             * sound_stop the one that BIG_PUSH's let-go and the first tests
             * of BIG_PUSH and BIG_PULL jump to (0x8c0ed7d2). without it,
             * BIG_PULL's first test holds that call, the code after it moves,
             * and 610 of 4304 words differ */
            break;
        }
        /* @hack redundant: a break after an if whose arms both break, which
         * puts work in R2, not R3, for BIG_AIR_THROW's first test
         * (0x8c0ed800); without it, 3 of 4304 words differ */
        break;
    case BIG_AIR_THROW:
        if (work->flags & PLAYER_HOLDING) {
            if (big_try_request_holding(work, motion, record)) {
                break;
            }
        } else {
            if (big_try_request(work, motion, record)) {
                break;
            }
        }
        if (work->flags & PLAYER_ON_GROUND) {
            sound_play(SE_LAND, 0, 0, 0);
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = BIG_HOLD_STOP;
                record->anim.action = ACTION_HOLD_STOP;
            } else if (big_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (work->flags & PLAYER_HOLDING) {
                    if (motion->speed.y > -5.0F) {
                        big_set_stand_action(work, motion, record);
                    } else {
                        record->anim.action = ACTION_HOLD_LAND;
                    }
                } else {
                    if (motion->speed.y > -5.0F) {
                        record->anim.action = ACTION_STAND;
                    } else {
                        record->anim.action = ACTION_LAND;
                    }
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = BIG_HOLD_WALK;
                } else {
                    work->mode = BIG_WALK;
                }
                big_set_walk_action(work, motion, record);
            }
            break;
        }
        if (big_try_release(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_AIR_THROW_AT &&
            record->anim.action != ACTION_AIR_THROW) {
            work->mode = BIG_FALL;
            record->anim.action = ACTION_FALL;
        }
        break;
    case BIG_SHAKE:
        if (big_try_request_holding(work, motion, record)) {
            break;
        }
        if (big_try_fall(work, motion, record)) {
            break;
        }
        if (big_try_jump(work, motion, record)) {
            break;
        }
        if (!player_touches_shakable(work, motion, record)) {
            work->mode = BIG_STAND;
            record->anim.action = ACTION_STAND;
            big_let_go(work, motion, record);
            break;
        }
        if (!player_read_stick(work, NULL, NULL)) {
            work->mode = BIG_GRIP;
            record->anim.action = ACTION_GRIP;
        }
        break;
    case BIG_HOLD_SHAKE:
        if (big_try_request_holding(work, motion, record)) {
            break;
        }
        if (big_try_fall(work, motion, record)) {
            break;
        }
        if (big_try_jump(work, motion, record)) {
            break;
        }
        if (record->held == NULL ||
            record->held->work->id != CARRY_KIND_SMALL) {
            work->mode = BIG_STAND;
            record->anim.action = ACTION_STAND;
            big_let_go(work, motion, record);
            break;
        }
        if (!player_read_stick(work, NULL, NULL)) {
            work->mode = BIG_HOLD_STAND;
            record->anim.action = ACTION_HOLD_SHAKE_END;
            /* @hack redundant: a redundant break, which puts work in R3, not
             * R1, for BIG_MODULE_POSE's test (0x8c0edb40); without it, 3 of
             * 4304 words differ */
            break;
        }
        break;
    case BIG_MODULE_POSE:
        if ((work->flags & PLAYER_REQUEST) &&
            work->submode == PLAYER_MODE_RIDE_OFF) {
            work->mode = BIG_SPRING;
            record->anim.action = ACTION_SPRING;
            work->flags &= ~PLAYER_GUARD;
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a redundant break, which puts work in R3 and its
             * event in R2 for BIG_EVENT's test (0x8c0edb78, 0x8c0edb7c).
             * without it, the two trade and 3 of 4304 words differ */
            break;
        }
        break;
    case BIG_EVENT:
        if (work->event->paths == NULL) {
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            work->mode = BIG_STAND;
            record->anim.action = ACTION_STAND;
        }
        break;
    }
}

static void big_destroy(struct task *task)
{
    struct task_work *work = task->work;

    player_free(PLAYER_VARS(work)->player);
    sound_stop_id((int)work);
    texture_release_character(CHARACTER_BIG);
}

/* place his shapes, turned with him: his body's center_y up from his
 * position, and his rod's ROD_SHAPE_BACK back along the rod from
 * weld_point4 */
static void big_move_shapes(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    struct collision_shape *shapes = work->collision->shapes;
    NJS_VECTOR offset;

    njPushMatrix(&_unit_matrix_);
    offset.x = offset.z = 0.0F;
    offset.y = record->consts.center_y;
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, 0x8000 - work->ang.y);
    njCalcVector(NULL, &offset, &shapes[0].center);
    VECTOR_ADD(&shapes[0].center, &work->pos);
    offset.x = record->weld_point4.x - (record->weld_axis4.x * ROD_SHAPE_BACK);
    offset.y = record->weld_point4.y - (record->weld_axis4.y * ROD_SHAPE_BACK);
    offset.z = record->weld_point4.z - (record->weld_axis4.z * ROD_SHAPE_BACK);
    njCalcVector(NULL, &offset, &shapes[1].center);
    VECTOR_ADD(&shapes[1].center, &shapes[0].center);
    njPopMatrix(1);
}

static void big_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;
    int action;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 25.0F)) {
        return;
    }
    draw_set_palette(DRAW_PLAYER_PALETTE);
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_IGNORE_SPECULAR);
    action = record->anim.action;
    if (record->anim.state == PLAYER_ANIM_BLENDING) {
        action = record->anim.current;
    }
    /* while the hurt timer runs, he's drawn on two steps in four, so he
     * blinks */
    if (!(TASK_TIMER(work) & 2)) {
        njSetTexture(&big_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->collision->shapes->center);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, 0x8000 - work->ang.y);
        njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
        if (work->event->motions != NULL) {
            event_motions_draw(work);
        } else if ((pad_records[0].on & PDD_DGT_TX) && work->mode == BIG_EDIT) {
            draw_tree(record->anim.actions[action].action->object);
        } else if (record->anim.state != PLAYER_ANIM_BLENDING) {
            draw_action(record->anim.actions[action].action,
                        record->anim.frame);
        } else {
            draw_action(record->anim.blend, record->anim.frame);
        }
        njPopMatrix(1);
    }
    draw_attr_restore();
    draw_set_palette(0);
}

static void big_set_stand_action(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record)
{
    NJS_PLANE plane;
    NJS_VECTOR front;
    NJS_VECTOR downhill;
    NJS_VECTOR ahead;
    Float slope;
    int ang;

    slope = -njInnerProduct(&world_gravity, &record->ground_normal);
    if (work->flags & PLAYER_HOLDING) {
        if (record->held != NULL &&
            record->held->work->id == CARRY_KIND_SMALL) {
            record->anim.action = ACTION_HOLD_SMALL;
        } else {
            record->anim.action = ACTION_HOLD_STAND;
        }
    } else if (slope > njCos(NJM_DEG_ANG(25))) {
        if (record->anim.action != ACTION_STAND &&
            record->anim.action != ACTION_FIDGET_START &&
            record->anim.action != ACTION_FIDGET &&
            record->anim.action != ACTION_FIDGET_END &&
            record->anim.action != ACTION_STAND_UPHILL &&
            record->anim.action != ACTION_STAND_DOWNHILL) {
            record->anim.action = ACTION_STAND;
        } else if (record->idle_steps > 1800) {
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
        njUnitVector(&ahead);
        njDistanceP2PL(&world_gravity, &plane, &downhill);
        njUnitVector(&downhill);
        ang = vector_angle(&ahead, &downhill, &record->ground_normal);
        if (ang > 0xc000 || ang < 0x4000) {
            record->anim.action = ACTION_STAND_DOWNHILL;
        } else {
            record->anim.action = ACTION_STAND_UPHILL;
        }
    }
    record->idle_steps++;
}

static void big_set_walk_action(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record)
{
    Float speed = record->speed.x + (motion->slip * 5.0F);

    if (record->anim.action == ACTION_TURN ||
        record->anim.action == ACTION_HOLD_TURN) {
        return;
    }
    if (work->flags & PLAYER_HOLDING) {
        if (record->held->work->id != CARRY_KIND_SMALL) {
            if (speed > record->consts.running_speed) {
                record->anim.action = ACTION_HOLD_RUN;
            } else {
                record->anim.action = ACTION_HOLD_WALK;
            }
        } else if (speed > record->consts.running_speed) {
            record->anim.action = ACTION_HOLD_SMALL_RUN;
        } else {
            record->anim.action = ACTION_HOLD_SMALL_WALK;
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
}

/* he hangs record->consts.height below the ledge's path. while he moves,
 * his action runs two frames a step, forward or back with him, round its
 * 32 frames. once the stick is let go, he stops at frame 16 or 30 */
static void big_ledge_move(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    NJS_VECTOR up;

    up.x = 0.0F;
    up.y = record->consts.height;
    up.z = 0.0F;
    player_turn_to_world(work, &up);
    njSubVector(&work->pos, &up);

    if (record->speed.x != 0.0F && motion->drive == 0.0F) {
        if (record->anim.frame == 16.0F || record->anim.frame >= 30.0F) {
            work->mode = BIG_LEDGE;
            record->speed.x = 0.0F;
        }
    } else if (record->speed.x > 0.0F) {
        if (record->anim.frame >= 0.0F && record->anim.frame < 16.0F) {
            record->anim.frame += 2.0F;
            if (record->anim.frame == 16.0F &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = BIG_LEDGE;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame += 2.0F;
            if (record->anim.frame >= 32.0F) {
                record->anim.frame = 0.0F;
            }
            if (record->anim.frame == 30.0F &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = BIG_LEDGE;
                record->speed.x = 0.0F;
            }
        }
    } else if (record->speed.x < 0.0F) {
        if (record->anim.frame > 16.0F && record->anim.frame <= 32.0F) {
            record->anim.frame -= 2.0F;
            if (record->anim.frame == 16.0F &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = BIG_LEDGE;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame -= 2.0F;
            if (record->anim.frame < 0.0F) {
                record->anim.frame = 30.0F;
            }
            if (record->anim.frame == 30.0F &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = BIG_LEDGE;
                record->speed.x = 0.0F;
            }
        }
    } else {
        record->speed.x = 0.0F;
    }
}

static Bool big_try_hurt(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    /* @hack spelling: the two mode tests apart, which load the mode again
     * for the second (MOV.B @R14,R0 at 0x8c0ee410). joined with ||, SHC
     * keeps the first load in R4, and 69 of 72 words differ */
    if (work->mode == BIG_MODULE_POSE) {
        return FALSE;
    }
    if (work->mode == BIG_SNAPPED) {
        return FALSE;
    }
    if ((work->flags & PLAYER_REQUEST) && work->submode == PLAYER_MODE_CLEAR) {
        return FALSE;
    }
    if (big_reeling_or_catching(work)) {
        return FALSE;
    }
    if ((work->flags & PLAYER_HURT) ||
        (record != NULL && (record->ground_flags & WORLD_HURTS))) {
        work->flags &= ~PLAYER_GUARD;
        sound_play(SE_HURT, 0, 0, 0);
        work->mode = BIG_HURT;
        record->anim.action = ACTION_HURT;
        work->flags &= ~PLAYER_HURT;
        record->speed.y = record->consts.jump_speed;
        record->speed.x = -record->consts.running_speed;
        work->timer = PLAYER_HURT_STEPS;
        camera_restore();
        return TRUE;
    }
    return FALSE;
}

static Bool big_try_jump(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    struct task_force *ground;

    if (player_jump_pressed(work, record)) {
        sound_play(SE_JUMP_VOICE, 0, 0, 0);
        if (work->flags & PLAYER_HOLDING) {
            if (record->held != NULL &&
                (record->held->work->flags & CARRY_HELD)) {
                work->mode = BIG_HOLD_JUMP;
                record->speed.y = record->consts.jump_speed;
                record->anim.action = ACTION_HOLD_JUMP;
                work->flags &= ~PLAYER_ON_TASK;
                record->jump_steps = 0;
                sound_play(SE_JUMP, 0, 0, 0);
                return TRUE;
            }
            big_let_go(work, motion, record);
        }
        work->mode = BIG_JUMP;
        record->speed.y = record->consts.jump_speed;
        if (record->ground_task != NULL) {
            ground = record->ground_task->force;
            if (ground != NULL) {
                player_set_accel(PLAYER_VARS(work)->player, ground->speed.x,
                                 ground->speed.y, ground->speed.z);
            }
        }
        record->anim.action = ACTION_JUMP;
        work->flags &= ~PLAYER_ON_TASK;
        record->jump_steps = 0;
        sound_play(SE_JUMP, 0, 0, 0);
        return TRUE;
    }
    return FALSE;
}

/* FALSE while liquid above him reaches his eyes, and TRUE otherwise */
static Bool big_eyes_above_water(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record)
{
    NJS_VECTOR eyes;

    if (record->shadow.above_flags & WORLD_LIQUID) {
        eyes.x = eyes.z = 0.0F;
        eyes.y = record->consts.eye_height;
        player_turn_to_world(work, &eyes);
        njAddVector(&eyes, &work->pos);
        if (record->shadow.above >= eyes.y) {
            return FALSE;
        }
    }
    return TRUE;
}

static Bool big_try_cast(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                    pad_action_buttons)) {
        if (fishing_task_in_reach(work) ||
            !big_eyes_above_water(work, motion, record)) {
            record->anim.action = ACTION_SWING;
            work->mode = BIG_SWING;
            work->flags |= PLAYER_GUARD;
            return TRUE;
        } else {
            record->anim.action = ACTION_CAST_START;
            work->mode = BIG_CAST;
            return TRUE;
        }
    }
    return FALSE;
}

static Bool big_try_float_cast(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    /* @hack spelling: the two tests nested, which store
     * ACTION_FLOAT_CAST_START from R1 and BIG_FLOAT_CAST from R2
     * (0x8c0ee72c, 0x8c0ee732). joined with &&, the stores take R2 and R3,
     * and 5 of 37 words differ */
    if (big_eyes_above_water(work, motion, record)) {
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                        pad_action_buttons)) {
            record->anim.action = ACTION_FLOAT_CAST_START;
            work->mode = BIG_FLOAT_CAST;
            return TRUE;
        }
    }
    return FALSE;
}

static Bool big_follow_event(struct task_work *work, struct task_motion *motion,
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

/* answer an event's action, then its paths, then a PLAYER_REQUEST
 * submode. it returns TRUE once it sets a mode, and the caller then checks
 * nothing more. it returns FALSE otherwise, even after it follows an
 * event's action or puts him on a path for PLAYER_MODE_JOIN_PATH */
static Bool big_try_request(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    if (work->event->action) {
        big_follow_event(work, motion, record);
        return FALSE;
    }
    if (work->event->paths != NULL) {
        work->mode = BIG_EVENT;
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
        work->mode = BIG_SPRING;
        record->anim.action = ACTION_SPRING;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_GUARD;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_LAUNCH:
        work->mode = BIG_LAUNCH;
        record->anim.action = ACTION_SPRING;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_GUARD;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_FLING:
        work->mode = BIG_FLUNG;
        record->anim.action = ACTION_FLUNG;
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_UPDRAFT:
        work->mode = BIG_FALL;
        record->anim.action = ACTION_UPDRAFT;
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_PATH:
        work->mode = BIG_PATH;
        record->anim.action = ACTION_RUN;
        record->path_distance = 0.0F;
        work->flags &= ~PLAYER_GUARD;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_SQUASH_2:
    case PLAYER_MODE_SQUASH_3:
    case PLAYER_MODE_STILL:
        work->mode = BIG_POSE;
        record->anim.action = ACTION_STAND;
        work->ang.x = motion->aim.x;
        work->ang.z = motion->aim.z;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL_PUSH:
        work->mode = BIG_POSE;
        record->anim.action = ACTION_PUSH_START;
        work->ang.x = motion->aim.x;
        work->ang.z = motion->aim.z;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_HANG_ON:
        work->mode = BIG_POSE;
        record->anim.action = ACTION_HANG_ON;
        work->ang.x = motion->aim.x;
        work->ang.z = motion->aim.z;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_GET_ON:
        work->mode = BIG_POSE;
        record->anim.action = ACTION_FALL;
        work->ang.x = motion->aim.x;
        work->ang.z = motion->aim.z;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_CLEAR:
        work->mode = BIG_POSE;
        record->anim.action = ACTION_CLEAR;
        work->ang.x = motion->aim.x;
        work->ang.z = motion->aim.z;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RESET:
        work->mode = BIG_STAND;
        record->anim.action = ACTION_STAND;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_GUARD;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_JOIN_PATH:
        if (work->mode == BIG_WALK ||
            record->speed.x > record->consts.running_speed) {
            record->path_distance = 0.0F;
            work->flags |= PLAYER_ON_PATH;
        }
        return FALSE;
    case PLAYER_MODE_LEDGE:
        if (work->flags & PLAYER_ON_GROUND) {
            return FALSE;
        }
        sound_play(SE_GRAB, 0, 0, 0);
        work->mode = BIG_LEDGE;
        record->anim.action = ACTION_LEDGE;
        record->anim.frame = 0.0F;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_GUARD;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_BREATHE:
        work->mode = BIG_BREATHE;
        record->anim.action = ACTION_BREATHE;
        record->underwater_steps = 0;
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_HOLD:
        record->held = PLAYER_VARS(work)->arg.carry;
        record->held->work->flags |= CARRY_HELD;
        work->mode = BIG_HOLD_STAND;
        work->flags |= PLAYER_HOLDING;
        work->flags &= ~COLLISION_TOUCHING;
        big_set_stand_action(work, motion, record);
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RIDE:
        work->mode = BIG_MODULE_POSE;
        record->anim.action = ACTION_RIDE;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_DROWN:
        work->mode = BIG_RISE;
        record->anim.action = ACTION_DROWN;
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_DOWN:
        work->mode = BIG_DOWN;
        record->anim.action = ACTION_DOWN;
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/* big_try_request while he holds something, without the test of an
 * event's paths, and with the same result, except that it answers neither
 * PLAYER_MODE_FLING, PLAYER_MODE_BREATHE nor PLAYER_MODE_HOLD, and returns
 * FALSE for them. he lets go first for
 * PLAYER_MODE_PATH, PLAYER_MODE_STILL_PUSH, PLAYER_MODE_CLEAR,
 * PLAYER_MODE_LEDGE, PLAYER_MODE_DROWN and PLAYER_MODE_DOWN, and keeps hold
 * for the rest */
static Bool big_try_request_holding(struct task_work *work,
                                    struct task_motion *motion,
                                    struct player *record)
{
    if (work->event->action) {
        big_follow_event(work, motion, record);
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
        work->mode = BIG_HOLD_SPRING;
        record->anim.action = ACTION_HOLD_JUMP;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_LAUNCH:
        work->mode = BIG_HOLD_LAUNCH;
        record->anim.action = ACTION_HOLD_JUMP;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_UPDRAFT:
        work->mode = BIG_HOLD_FALL;
        record->anim.action = ACTION_HOLD_FALL;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_PATH:
        big_let_go(work, motion, record);
        work->mode = BIG_PATH;
        record->anim.action = ACTION_RUN;
        record->path_distance = 0.0F;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
            work->flags &= ~PLAYER_GUARD;
        }
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_SQUASH_2:
    case PLAYER_MODE_SQUASH_3:
    case PLAYER_MODE_STILL:
        work->mode = BIG_POSE;
        big_set_stand_action(work, motion, record);
        work->ang.x = motion->aim.x;
        work->ang.z = motion->aim.z;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL_PUSH:
        big_let_go(work, motion, record);
        work->mode = BIG_POSE;
        record->anim.action = ACTION_PUSH_START;
        work->ang.x = motion->aim.x;
        work->ang.z = motion->aim.z;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_HANG_ON:
        work->mode = BIG_POSE;
        record->anim.action = ACTION_HANG_ON;
        work->ang.x = motion->aim.x;
        work->ang.z = motion->aim.z;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_GET_ON:
        work->mode = BIG_POSE;
        record->anim.action = ACTION_FALL;
        work->ang.x = motion->aim.x;
        work->ang.z = motion->aim.z;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_CLEAR:
        big_let_go(work, motion, record);
        work->mode = BIG_POSE;
        record->anim.action = ACTION_CLEAR;
        work->ang.x = motion->aim.x;
        work->ang.z = motion->aim.z;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RESET:
        work->mode = BIG_HOLD_STAND;
        big_set_stand_action(work, motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_JOIN_PATH:
        if (work->mode == BIG_HOLD_WALK ||
            record->speed.x > record->consts.running_speed) {
            record->path_distance = 0.0F;
            work->flags |= PLAYER_ON_PATH;
        }
        return FALSE;
    case PLAYER_MODE_LEDGE:
        if (work->flags & PLAYER_ON_GROUND) {
            return FALSE;
        }
        big_let_go(work, motion, record);
        sound_play(SE_GRAB, 0, 0, 0);
        work->mode = BIG_LEDGE;
        record->anim.action = ACTION_LEDGE;
        record->anim.frame = 0.0F;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_GUARD;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RIDE:
        work->mode = BIG_MODULE_POSE;
        record->anim.action = ACTION_RIDE;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_DROWN:
        big_let_go(work, motion, record);
        work->mode = BIG_RISE;
        record->anim.action = ACTION_DROWN;
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_DOWN:
        sound_play(SE_DOWN, 0, 0, 0);
        big_let_go(work, motion, record);
        work->mode = BIG_DOWN;
        record->anim.action = ACTION_DOWN;
        work->flags &= ~PLAYER_GUARD;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

static Bool big_can_float(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    if ((record->upgrades & PLAYER_BIG_LIFE_BELT) &&
        !PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                     pad_jump_buttons) &&
        (record->shadow.above_flags & WORLD_WATER) &&
        (record->shadow.above > work->collision->shapes->center.y ||
         !(work->flags & PLAYER_ON_GROUND))) {
        return TRUE;
    }
    return FALSE;
}

static Bool big_belt_in_water(struct task_work *work,
                              struct task_motion *motion, struct player *record)
{
    if ((record->upgrades & PLAYER_BIG_LIFE_BELT) &&
        (record->shadow.above_flags & WORLD_WATER)) {
        return TRUE;
    }
    return FALSE;
}

static Bool big_try_float(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    if (big_can_float(work, motion, record)) {
        work->mode = BIG_FLOAT;
        record->anim.action = ACTION_FLOAT;
        return TRUE;
    }
    return FALSE;
}

static Bool big_try_fall(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    /* @hack spelling: the fall nested in the test of PLAYER_ON_GROUND, which
     * leaves return FALSE at the function's end (MOV #0,R0 at 0x8c0ef20a,
     * where BF/S at 0x8c0ef1d0 branches). as an early return on the ground,
     * return FALSE follows the test, and 33 of 43 words differ */
    if (!(work->flags & PLAYER_ON_GROUND)) {
        if (work->flags & PLAYER_HOLDING) {
            if (player_carry_flags(work) & PLAYER_CAN_CARRY) {
                work->mode = BIG_HOLD_FALL;
                record->anim.action = ACTION_HOLD_FALL;
                return TRUE;
            }
            big_let_go(work, motion, record);
        }
        work->mode = BIG_FALL;
        record->anim.action = ACTION_FALL;
        return TRUE;
    }
    return FALSE;
}

static Bool big_try_stop(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    if (!player_read_stick(work, NULL, NULL) && record->speed.x == 0.0F) {
        if (work->flags & PLAYER_HOLDING) {
            if (work->mode == BIG_PUSH || work->mode == BIG_SHAKE ||
                work->mode == BIG_PULL) {
                work->mode = BIG_GRIP;
                record->anim.action = ACTION_GRIP;
            } else {
                work->mode = BIG_HOLD_STAND;
                big_set_stand_action(work, motion, record);
            }
        } else {
            work->mode = BIG_STAND;
            record->anim.action = ACTION_STAND;
        }
        return TRUE;
    }
    return FALSE;
}

/* at a press of the grab button, he grips what he touches to push, pull or
 * shake it. he starts to lift instead a task he can carry but not push. it
 * returns TRUE once he grips or lifts, and FALSE otherwise */
static Bool big_try_grab(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    int flags;

    if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                    pad_grab_buttons)) {
        if ((flags = player_carry_flags(work)) != 0) {
            switch (player_face_wall(work, motion)) {
            default:
                return FALSE;
            case PLAYER_WALL_AWAY:
            case PLAYER_WALL_ACROSS:
            case PLAYER_WALL_ASKEW:
            /* no break ends PLAYER_WALL_HEAD_ON's case, the last. with one,
             * PLAYER_CAN_PUSH takes R3, not R2 (0x8c0ef32c), and 2 of 92
             * words differ */
            case PLAYER_WALL_HEAD_ON:;
            }
            if (flags & PLAYER_CAN_PUSH) {
                work->mode = BIG_GRIP;
                record->anim.action = ACTION_GRIP_START;
                record->held = work->collision->hit->owner;
                work->flags |= PLAYER_HOLDING;
                return TRUE;
            } else if (flags & PLAYER_CAN_CARRY) {
                work->mode = BIG_LIFT;
                if (work->collision->hit->owner->work->id == CARRY_KIND_SMALL) {
                    record->anim.action = ACTION_LIFT_SMALL;
                } else {
                    record->anim.action = ACTION_LIFT;
                }
                return TRUE;
            } else if (flags & PLAYER_CAN_SHAKE) {
                work->mode = BIG_GRIP;
                record->anim.action = ACTION_GRIP_START;
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* he steps toward what he touches. from frame 8 of the lift, he takes it
 * up if he faces it head on and can carry it, or else stands. TRUE once he
 * holds it */
static Bool big_try_take_lifted(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record)
{
    int flags;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a vector. the image's ADD #-20,R15 reserves them
     * (0x8c0ef406); without them, it reserves 8 and 2 of 72 words differ */
    NJS_VECTOR unused;

    player_approach_touched(work, motion);
    if (record->anim.frame < 8.0F) {
        return FALSE;
    }
    if ((flags = player_carry_flags(work)) != 0 &&
        player_face_wall(work, motion) == PLAYER_WALL_HEAD_ON) {
        if (flags & PLAYER_CAN_CARRY) {
            work->mode = BIG_HOLD_STAND;
            sound_play(SE_LIFT, 0, 0, 0);
            record->held = work->collision->hit->owner;
            record->held->work->flags |= CARRY_HELD;
            work->flags |= PLAYER_HOLDING;
            work->flags &= ~COLLISION_TOUCHING;
            return TRUE;
        }
    } else {
        /* @hack redundant: an empty else, which puts ACTION_STAND's 0 in R3 and
         * BIG_STAND in R2 for the stores below (0x8c0ef47c, 0x8c0ef47e).
         * without it, the two trade and 4 of 72 words differ */
    }
    work->mode = BIG_STAND;
    record->anim.action = ACTION_STAND;
    return FALSE;
}

static Bool big_try_throw(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    if (pad_records[PLAYER_VARS(work)->player].press & pad_grab_buttons) {
        if (!(work->flags & PLAYER_ON_GROUND)) {
            work->mode = BIG_AIR_THROW;
            if (player_has_target(PLAYER_VARS(work)->player)) {
                record->anim.action = ACTION_AIR_THROW_AT;
            } else {
                record->anim.action = ACTION_AIR_THROW;
            }
        } else if (record->speed.x < record->consts.trot_speed) {
            work->mode = BIG_PUT_DOWN;
            if (record->held->work->id == CARRY_KIND_SMALL) {
                record->anim.action = ACTION_PUT_DOWN_SMALL;
            } else {
                record->anim.action = ACTION_PUT_DOWN;
            }
            player_clear_speed(motion, record);
        } else {
            work->mode = BIG_THROW;
            if (player_has_target(PLAYER_VARS(work)->player)) {
                record->anim.action = ACTION_THROW_AT;
            } else {
                record->anim.action = ACTION_THROW;
            }
        }
        return TRUE;
    } else if (!(work->flags & PLAYER_HOLDING)) {
        if (work->flags & PLAYER_ON_GROUND) {
            work->mode = BIG_STAND;
            record->anim.action = ACTION_STAND;
        } else {
            work->mode = BIG_FALL;
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

/* he lets go of what he holds at his action's release frame. he throws it
 * at the nearest task he can aim at, throws it ahead, or puts it down.
 * TRUE once he lets go, or once he clears PLAYER_HOLDING with nothing held */
static Bool big_try_release(struct task_work *work, struct task_motion *motion,
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
    /* @hack spelling: action assigned inside the test, which SHC zero-extends
     * straight into R0 for the compare (EXTU.W R6,R0 at 0x8c0ef688).
     * assigned in a statement before the test, the action is extended in R6,
     * and 9 of 360 words differ */
    if ((((action = record->anim.action) == ACTION_THROW_AT &&
          record->anim.frame == 12.0F) ||
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
            toss.x *= 4.0F;
            toss.y *= 4.0F;
            njPushMatrix(&_unit_matrix_);
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
            TURN(njRotateY, -work->ang.y);
            njCalcVector(NULL, &toss, &record->held->motion->speed);
            njPopMatrix(1);
        }
        work->flags &= ~PLAYER_HOLDING;
        record->held = NULL;
        sound_play(SE_THROW, 0, 0, 0);
        return TRUE;
    }
    action = record->anim.action;
    if ((action == ACTION_THROW && record->anim.frame == 16.0F) ||
        (action == ACTION_AIR_THROW && record->anim.frame == 6.0F)) {
        held = record->held->work;
        if (held != NULL) {
            held->flags &= ~CARRY_HELD;
            toss.x = 3.0F;
            toss.y = 3.0F;
            toss.z = 0.0F;
            njPushMatrix(&_unit_matrix_);
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
            TURN(njRotateY, -work->ang.y);
            njCalcVector(NULL, &toss, &record->held->motion->speed);
            njPopMatrix(1);
        }
        work->flags &= ~PLAYER_HOLDING;
        record->held = NULL;
        sound_play(SE_THROW, 0, 0, 0);
        return TRUE;
    }
    if (record->anim.action == ACTION_PUT_DOWN_SMALL &&
        record->anim.frame == 8.0F) {
        held = record->held->work;
        if (held != NULL) {
            held->flags &= ~CARRY_HELD;
        }
        work->flags &= ~PLAYER_HOLDING;
        record->held = NULL;
        return TRUE;
    }
    if (record->anim.action == ACTION_PUT_DOWN && record->anim.frame == 8.0F) {
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
 * pushes or pulls it as the stick says. with the button let go, he lifts it
 * if he can carry it, or else lets go and stands. FALSE while he holds the
 * button with the stick let go */
static Bool big_try_grip(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    int wall;

    /* @hack spelling: a return in every arm, which loads BIG_PUSH into R3 and
     * ACTION_PUSH into R1 for the push arm's stores (0x8c0efa54,
     * 0x8c0efa5e). with one return after the chain, those values and
     * sound_play's arguments take other registers. 65 of 92 words then
     * differ */
    if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                    pad_grab_buttons)) {
        wall = player_stick_into_wall(work, motion);
        if (wall == PLAYER_WALL_NO_STICK) {
            return FALSE;
        } else if (player_carry_flags(work) & PLAYER_CAN_SHAKE) {
            work->mode = BIG_SHAKE;
            record->anim.action = ACTION_SHAKE;
            return TRUE;
        } else if (wall == PLAYER_WALL_HEAD_ON) {
            work->mode = BIG_PUSH;
            record->anim.action = ACTION_PUSH;
            sound_play(SE_PUSH_START, 0, 0, 0);
            return TRUE;
        } else {
            work->mode = BIG_PULL;
            record->anim.action = ACTION_PULL;
            return TRUE;
        }
    } else if (player_carry_flags(work) & PLAYER_CAN_CARRY) {
        work->mode = BIG_LIFT;
        record->anim.action = ACTION_LIFT;
        return TRUE;
    } else {
        big_let_go(work, motion, record);
        work->mode = BIG_STAND;
        record->anim.action = ACTION_STAND;
        return TRUE;
    }
    /* @hack redundant: a return no path reaches, which the NO_STICK test's
     * return FALSE branches to (BT at 0x8c0efa34). without it, that test
     * returns in place, the arms reorder, and 67 of 92 words differ */
    return FALSE;
}

/* he lets go of what he holds, if anything, tossing a task he can carry
 * up and ahead. record->held must be set while PLAYER_HOLDING is */
static void big_let_go(struct task_work *work, struct task_motion *motion,
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

/* keep what he holds with him. a task he carries sits between his hands,
 * pushed out of his body's shape. a task he grips he lets go of once he
 * touches another */
static void big_carry(struct task_work *work, struct task_motion *motion,
                      struct player *record)
{
    struct collision_shape *shape;
    struct task_work *held;
    Float reach;
    Float distance;
    Float scale;
    NJS_POINT3 hands;
    NJS_VECTOR offset;

    held = record->held->work;
    shape = work->collision->shapes;
    if (!(player_carry_flags(work) & PLAYER_CAN_CARRY)) {
        if (work->collision->hit == NULL) {
            return;
        } else if (held != work->collision->hit->owner->work) {
            big_let_go(work, motion, record);
            if (record->held == NULL) {
                if (work->flags & PLAYER_ON_GROUND) {
                    work->mode = BIG_STAND;
                    record->anim.action = ACTION_STAND;
                } else {
                    work->mode = BIG_FALL;
                    record->anim.action = ACTION_FALL;
                }
            }
        }
        return;
    }
    if (!(held->flags & CARRY_HELD)) {
        return;
    }
    held->pos = shape->center;
    hands.x = (record->weld_point0.x + record->weld_point1.x) * 0.5F;
    hands.y = (record->weld_point0.y + record->weld_point1.y) * 0.5F;
    hands.z = (record->weld_point0.z + record->weld_point1.z) * 0.5F;
    hands.x -= held->collision->shapes->center.x;
    if (held->id != CARRY_KIND_SMALL) {
        /* @hack spelling: an empty arm, whose jump loads the held task's
         * collision into R3 and its shapes into R2 (0x8c0efd06,
         * 0x8c0efd0a). tested the other way, the two swap and 3 of 260
         * words differ */
    } else {
        hands.y -= held->collision->shapes->center.y;
    }
    hands.z -= held->collision->shapes->center.z;
    if (work->flags & PLAYER_HOLDING) {
        reach = shape->radius + held->collision->shapes[1].radius;
        distance = njSqrt((hands.x * hands.x) + (hands.y * hands.y));
        if (distance < reach) {
            if (record->weld_point0.y > 9.0F) {
                scale = _builtin_fabsf(hands.x) / distance;
                hands.x = -(reach * scale);
                scale = _builtin_fabsf(hands.y) / distance;
                hands.y = reach * scale;
                if (hands.y > 13.0F) {
                    hands.y = 13.0F;
                }
            } else {
                hands.x = -njSqrt((reach * reach) - (hands.y * hands.y));
            }
        }
    }
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, 0x8000 - work->ang.y);
    njCalcVector(NULL, &hands, &offset);
    njPopMatrix(1);
    njAddVector(&held->pos, &offset);
}

static void big_float_up(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    Float speed = (record->shadow.above - work->pos.y) * big_float_rate;

    if (record->shadow.above > work->collision->shapes->center.y) {
        speed *= 0.9F;
    }
    speed += rand() * (0.0001F * RAND_SCALE);
    if (speed >= big_float_max) {
        speed = big_float_max;
    }
    player_set_accel(PLAYER_VARS(work)->player, 0.0F, speed, 0.0F);
}

/* the first step of his mouth task. it gives the task a player_mouth and
 * hands it on to player_mouth_step. without memory, it tries again the
 * next step */
static void big_mouth_start(struct task *task)
{
    struct player_mouth *mouth;

    if ((mouth = heap_calloc(1, sizeof(*mouth))) != NULL) {
        task->motion = (struct task_motion *)mouth;
        mouth->face = &big_mouth;
        mouth->mouth = big_head;
        mouth->blend = &big_head_blend;
        mouth->shapes = &big_mouth_shapes;
        task->destructor = player_mouth_destroy;
        task->step = player_mouth_step;
    }
}

static void big_sway_destroy(struct task *task)
{
    big_head->model = big_head_models[1];
}

/* sway the ends of his mouth as he rises and falls. the swayed points go
 * into big_head_models[0], the copy his head draws in place of its model */
static void big_sway_step(struct task *task)
{
    struct task_work *work = task->work;
    struct player_sway *sway = PLAYER_SWAY(task);
    struct task_work *owner = player_work[PLAYER_VARS(work)->player];
    struct task_motion *owner_motion = player_motion[PLAYER_VARS(work)->player];
    /* @hack spelling: to declared before from, which makes the frame 328
     * bytes, 280 of them below the saved registers (ADD R0,R15 at
     * 0x8c0effba). declared the other way round, the frame is 324 bytes and
     * 130 of 305 words differ */
    NJS_POINT3 *to = big_head_models[0]->points;
    NJS_POINT3 *from = big_head_models[1]->points;
    Uint32 count = big_head_models[1]->nbPoint;
    Uint32 i;
    Float wave;
    NJS_MATRIX tilt;
    NJS_MATRIX half_tilt;
    NJS_MATRIX back_tilt;
    NJS_MATRIX half_back_tilt;
    NJS_VECTOR point;

    switch (work->mode) {
    case SWAY_INIT:
        work->mode = SWAY_SWAY;
        task->destructor = big_sway_destroy;
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
        if (!(sway->bob < 0.2F && sway->bob > -0.2F)) {
            /* @hack spelling: the decay written out, which loads 0.98F with
             * MOVA (0x8c0f0062). with *=, SHC loads it through R1 and FPUL,
             * and 7 of 305 words differ */
            sway->bob = sway->bob * 0.98F;
        } else {
            sway->bob = 0.0F;
        }
        sway->bob_phase += 0x1000;
        sway->bob_phase &= 0xfe00;
        wave = njCos(sway->bob_phase);
        work->ang.z = wave * sway->bob * 819.2F;
        njUnitMatrix(&tilt);
        TURN_MATRIX(njRotateX, &tilt, work->ang.z);
        njUnitMatrix(&half_tilt);
        TURN_MATRIX(njRotateX, &half_tilt, work->ang.z >> 1);
        njUnitMatrix(&back_tilt);
        TURN_MATRIX(njRotateX, &back_tilt, -work->ang.z);
        njUnitMatrix(&half_back_tilt);
        TURN_MATRIX(njRotateX, &half_back_tilt, -work->ang.z >> 1);
        for (i = 0; i < count; i++, from++, to++) {
            if (from->z < -8.0F) {
                point.x = from->x;
                point.y = from->y + 4.0F;
                point.z = from->z;
                njCalcVector(&tilt, &point, to);
                to->y -= 4.0F;
            } else if (from->z > 8.0F) {
                point.x = from->x;
                point.y = from->y + 4.0F;
                point.z = from->z;
                njCalcVector(&back_tilt, &point, to);
                to->y -= 4.0F;
            } else if (from->z < -4.0F) {
                point.x = from->x;
                point.y = from->y + 4.0F;
                point.z = from->z;
                njCalcVector(&half_tilt, &point, to);
                to->y -= 4.0F;
            } else if (from->z > 4.0F) {
                point.x = from->x;
                point.y = from->y + 4.0F;
                point.z = from->z;
                njCalcVector(&half_back_tilt, &point, to);
                to->y -= 4.0F;
            }
        }
        break;
    default:
        task_destroy(task);
        break;
    }
    /* his head draws the swayed copy. after task_destroy, this undoes what
     * big_sway_destroy put back */
    big_head->model = big_head_models[0];
}

/* create player's look task. it turns nodes[0], his head, and moves
 * nodes[1] and nodes[2], his eyes. FALSE without memory for the task or its
 * player_look_motion */
static Bool big_look_create(Sint8 player, NJS_OBJECT **nodes)
{
    struct task *task;
    struct player_look_motion *look;

    /* @hack spelling: each call assigned in its test, which tests R0 with the
     * copy in the branch's delay slot (BT/S at 0x8c0f02be, BF/S at
     * 0x8c0f02cc). assigned apart, SHC copies before each test, and 6 of 48
     * words differ */
    /* @hack spelling: the failed create's return FALSE in an else, which puts
     * it after the success path (0x8c0f02f6). as an early return, SHC puts
     * it ahead of that path, and 12 of 48 words differ */
    if ((task = task_create(TASK_WORK, 2, big_look_step)) != NULL) {
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

static void big_look_destroy(struct task *task)
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

/* turn his head the way he turns, at the camera while Z is held, or at
 * what his event says. the head turns as far as its limits allow, and the
 * eyes turn the rest of the way */
static void big_look_step(struct task *task)
{
    struct task_work *work = task->work;
    struct player_look_motion *look = PLAYER_LOOK(task);
    struct task_work *owner = player_work[PLAYER_VARS(work)->player];
    struct task_motion *owner_motion = player_motion[PLAYER_VARS(work)->player];
    struct event_work *event;
    struct collision_shape *shape;
    struct task_work *target;
    Angle angles[3];
    NJS_VECTOR direction;
    NJS_VECTOR facing;

    if (owner == NULL) {
        work->mode = LOOK_END;
    } else if (work->mode == LOOK_INIT) {
        /* @hack spelling: an empty arm, which leaves a starting look in
         * LOOK_INIT (0x8c0f03c2). with the tests below nested under a test
         * of LOOK_INIT, the event's record takes R2, not R3, the function
         * runs 2 bytes short, and 14 of 407 words differ */
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
        look->look.eye0_max_y = 0x400;
        look->look.eye0_min_y = -0x800;
        look->look.eye1_max_y = 0x800;
        look->look.eye1_min_y = -0x400;
        look->look.eye0_max_z = 0x800;
        look->look.eye0_min_z = -0x800;
        look->look.eyes_max_z = 0x800;
        look->look.eyes_min_z = -0x800;
        task->destructor = big_look_destroy;
        break;
    case LOOK_AHEAD:
        look->look.head_aim_y =
            -angle_difference(owner->ang.y, owner_motion->aim.y);
        look->look.head_aim_z = 0;
        player_look_turn_head(&look->look);
        look->look.eyes_aim_y = 0;
        look->look.eyes_aim_z = 0;
        player_look_turn_eyes(&look->look);
        big_look_move_eyes(look);
        break;
    case LOOK_CAMERA:
        target = camera_work;
        shape = owner->collision->shapes;
        /* @hack spelling: y and z set first, in one chain, which keeps 1.0F in
         * FR5 and 0.0F in FR4 (0x8c0f0428). with x first, the two trade and
         * 12 of 407 words differ. set apart, the limits' loads reorder and
         * 24 of 407 words differ */
        facing.y = facing.z = 0.0F;
        facing.x = 1.0F;
        player_turn_to_world(owner, &facing);
        direction.x = target->pos.x - shape->center.x;
        direction.y = target->pos.y - shape->center.y;
        direction.z = target->pos.z - shape->center.z;
        vector_get_angles(&direction, &facing, angles);
        look->look.head_aim_y = -angles[1];
        look->look.head_aim_z = angles[2];
        player_look_turn_head(&look->look);
        look->look.eyes_aim_y = angles[1] + look->look.head_y;
        look->look.eyes_aim_z = angles[2] - look->look.head_z;
        player_look_turn_eyes(&look->look);
        big_look_move_eyes(look);
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
            if (event->look_task->work != NULL) {
                target = event->look_task->work;
                facing.y = facing.z = 0.0F;
                facing.x = 1.0F;
                player_turn_to_world(owner, &facing);
                direction.x =
                    target->pos.x + event->look_pos.x - shape->center.x;
                direction.y =
                    target->pos.y + event->look_pos.y - shape->center.y;
                direction.z =
                    target->pos.z + event->look_pos.z - shape->center.z;
            } else {
                /* @bug uninitialized: when the look task's work is null,
                 * direction copies facing, which no path here sets. the copy
                 * reads the frame at R15 + 8 (0x8c0f06d6 to 0x8c0f06e8).
                 * vector_get_angles then measures facing against itself, so
                 * both angles come out 0 and the look aims where he faces */
                /* NOLINTNEXTLINE(clang-diagnostic-uninitialized) */
                direction = facing;
            }
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
        big_look_move_eyes(look);
        break;
    default:
        task_kill(task);
        break;
    }
}

/* slide each eye over his face. a line runs from the head's origin through
 * the eye's resting place, turned by the eye's angles. the eye moves to
 * where that line meets the plane its resting place lies on */
static void big_look_move_eyes(struct player_look_motion *look)
{
    NJS_MATRIX turn;
    union line_view line;
    union plane_view plane;
    NJS_POINT3 point;

    njUnitMatrix(&turn);
    TURN_MATRIX(njRotateZ, &turn, look->look.eye0_z);
    TURN_MATRIX(njRotateY, &turn, look->look.eye0_y);
    plane.plane.px = look->eye0.pos[0];
    plane.plane.py = look->eye0.pos[1];
    plane.plane.pz = look->eye0.pos[2];
    plane.plane.vx = 0.893132F;
    plane.plane.vy = -0.339211F;
    plane.plane.vz = 0.295384F;
    line.line.px = 0.0F;
    line.line.py = 0.0F;
    line.line.pz = 0.0F;
    njCalcVector(&turn, &plane.split.point, &line.split.direction);
    njUnitVector(&line.split.direction);
    /* @hack type: the unions' own addresses as the NJS_LINE and NJS_PLANE,
     * which SHC forms from R15 at each use. as &line.line and &plane.plane
     * here too, SHC keeps them in saved registers, the function runs 30
     * bytes short, and 193 of 196 words differ */
    njDistanceL2PL((NJS_LINE *)&line, (NJS_PLANE *)&plane, &point);
    look->look.eye0->pos[0] = point.x;
    look->look.eye0->pos[1] = point.y;
    look->look.eye0->pos[2] = point.z;

    njUnitMatrix(&turn);
    TURN_MATRIX(njRotateZ, &turn, look->look.eye1_z);
    TURN_MATRIX(njRotateY, &turn, look->look.eye1_y);
    plane.plane.px = look->eye1.pos[0];
    plane.plane.py = look->eye1.pos[1];
    plane.plane.pz = look->eye1.pos[2];
    plane.plane.vx = 0.893042F;
    plane.plane.vy = -0.339211F;
    plane.plane.vz = -0.295656F;
    line.line.px = 0.0F;
    line.line.py = 0.0F;
    line.line.pz = 0.0F;
    njCalcVector(&turn, &plane.split.point, &line.split.direction);
    njUnitVector(&line.split.direction);
    njDistanceL2PL(&line.line, &plane.plane, &point);
    look->look.eye1->pos[0] = point.x;
    look->look.eye1->pos[1] = point.y;
    look->look.eye1->pos[2] = point.z;
}

Bool big_not_fishing(struct task *task)
{
    struct task_work *work = player_work[0];

    if ((work->mode >= BIG_CAST && work->mode <= BIG_CATCH) ||
        (work->mode >= BIG_FLOAT_CAST && work->mode <= BIG_FLOAT_CATCH)) {
        return FALSE;
    }
    return TRUE;
}

/* TRUE while he reels in or shows the catch, afloat or not, and FALSE
 * otherwise */
static Bool big_reeling_or_catching(struct task_work *work)
{
    if ((work->mode >= BIG_REEL && work->mode <= BIG_CATCH) ||
        (work->mode >= BIG_FLOAT_REEL && work->mode <= BIG_FLOAT_CATCH)) {
        return TRUE;
    }
    return FALSE;
}

/* play the catch's sound, which for any catch but the frog is SE_CATCH at
 * once. FISHING_GOAL marks a catch of the frog, whose sound plays five
 * steps later. it is SE_CATCH_CHAOS in Chaos 6's stage and SE_CATCH_GOAL
 * elsewhere, but none plays at Twinkle Park or Ice Cap */
static void big_play_catch_sound(void)
{
    if (fishing_flags & FISHING_GOAL) {
        if (!IN_STAGE(STAGE_TWINKLE) && !IN_STAGE(STAGE_ICECAP)) {
            if (IN_STAGE(STAGE_CHAOS6)) {
                sound_play_later(SE_CATCH_CHAOS);
            } else {
                sound_play_later(SE_CATCH_GOAL);
            }
        }
    } else {
        sound_play(SE_CATCH, 0, 0, 0);
    }
}

/* land the catch: its sound, the fanfare unless it's the frog, its weight
 * shown and scored, and his action by its size. with no catch, he takes
 * ACTION_CATCH_LARGE */
static void big_land_catch(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    if (big_catch != NULL) {
        big_play_catch_sound();
        /* @hack spelling: the kind tested bare, for any catch but
         * FISH_FROG, which is 0. the call below then takes
         * stream_request_interlude's address in R3 (MOV.L and JSR @R3 at
         * 0x8c0f0ac6). compared with FISH_FROG, R2 takes it and 2 of 63
         * words differ */
        if (FISH_VARS(big_catch->work)->kind) {
            stream_request_interlude(MUSIC_FISHGET);
        }
        fishing_show_catch(FISH_MOTION(big_catch)->weight,
                           FISH_VARS(big_catch->work)->kind);
        if (big_catch->work->scl.y <= 0.0F) {
            record->anim.action = ACTION_CATCH_SMALL;
        } else if (big_catch->work->scl.y <= 1.0F) {
            record->anim.action = ACTION_CATCH_MEDIUM;
        } else {
            record->anim.action = ACTION_CATCH_LARGE;
        }
    } else {
        record->anim.action = ACTION_CATCH_LARGE;
    }
    fishing_flags &= FISHING_HOOKED | FISHING_GOAL;
}

static void big_snap_line(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    camera_restore_high();
    record->anim.action = ACTION_SNAP;
    work->mode = BIG_SNAPPED;
    sound_play(SE_SNAP, 0, 0, 0);
    sound_play(SE_HURT, 0, 0, 0);
}

/* take the camera as he starts to aim. outside the adventure fields, it
 * looks at camera_place's target, which fishing.c sets */
static void big_take_camera(void)
{
    if (stage_group() == STAGE_GROUP_FIELD) {
        camera_override_mode(CAMERA_MODE_KNUCKLES2, CAMERA_ADJUST_TIME);
    } else {
        camera_override(camera_step_fishing_point, CAMERA_ADJUST_SLOW,
                        CAMERA_LOOK_AT);
    }
}

static void big_give_camera(void)
{
    if (fishing_flags & FISHING_CAMERA) {
        camera_restore_high();
    } else {
        camera_restore();
    }
}
