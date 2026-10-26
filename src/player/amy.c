/* Amy as a player: her modes, her hammer and what she carries */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "enemy/zero.h"
#include "enemy/zero_handle.h"
#include "event/event.h"
#include "event/play.h"
#include "object/common.h"
#include "object/panel.h"
#include "object/ring.h"
#include "player/amy.h"
#include "player/amy_bird.h"
#include "player/amy_drum.h"
#include "player/amy_effects.h"
#include "player/amy_handle.h"
#include "player/amy_push.h"
#include "player/player.h"
#include "player/target.h"
#include "player/textures.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* her modes, the values of her work->mode. amy_mode_names holds the game's
 * own name for each, in this order */
enum amy_mode {
    AMY_INIT,
    AMY_STAND,
    AMY_WALK,
    AMY_PATH, /* running along a path */
    AMY_PATH_SPIN, /* rolling along a path, curled up */
    AMY_JUMP,
    AMY_SPRING,
    AMY_LAUNCH,
    AMY_FALL,
    AMY_STOP, /* braking */
    AMY_PUSH,
    AMY_HURT,
    AMY_BACK,
    AMY_POSE, /* still, in the action a request gave her */
    AMY_CRASH, /* thrown back from a wall she ran into */
    AMY_HANDLE, /* holding the handle she's called to */
    AMY_BALANCE, /* on a spot she keeps her balance on */
    AMY_TAKE_DRUM,
    AMY_SET_DRUM, /* setting a drum down */
    AMY_HAMMER, /* swinging her hammer */
    AMY_HAMMER_SPIN, /* spinning round with her hammer, then dizzy */
    AMY_HAMMER_JUMP, /* leaping off her hammer from a run */
    AMY_AIR_HAMMER, /* swinging her hammer in the air */
    AMY_LIFT,
    AMY_PUT_DOWN,
    AMY_HOLD_STAND,
    AMY_GRIP, /* holding on to what she'll push, pull or shake */
    AMY_HOLD_WALK,
    AMY_THROW,
    AMY_HOLD_STOP,
    AMY_HOLD_FALL,
    AMY_HOLD_JUMP,
    AMY_HOLD_SPRING,
    AMY_HOLD_LAUNCH,
    AMY_PULL,
    AMY_AIR_THROW,
    AMY_SHAKE,
    AMY_HOLD_SHAKE, /* shaking the small thing she holds */
    AMY_HANG, /* hanging from a path */
    AMY_HANG_GO, /* moving along the path she hangs from */
    AMY_LEDGE, /* hanging from a ledge */
    AMY_LEDGE_GO, /* moving along the ledge */
    AMY_PANEL_JUMP, /* leaping from one panel to the next */
    AMY_PANEL_WAIT, /* standing on a panel */
    AMY_CAUGHT, /* in the robo's grabber, until she breaks free */
    AMY_EVENT, /* following an event's paths */
    AMY_DROWN, /* out of air: ACTION_DROWN, then up to the water's surface */
    AMY_DOWN, /* from PLAYER_MODE_DOWN: amy_step chooses no mode after it */
    AMY_RIDE, /* riding, from PLAYER_MODE_RIDE until PLAYER_MODE_RIDE_OFF */
    AMY_EDIT, /* in the editor, where the stick moves her freely */
    AMY_MODES
};

/* the rows of amy_actions her code names, by index */
enum amy_action_index {
    ACTION_STAND = 0,
    ACTION_LAND = 1, /* a landing at a standstill, once before ACTION_LANDED */
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
    ACTION_JUMP = 14, /* her jump, curled up and spinning */
    ACTION_SPRING = 16,
    ACTION_FALL_START = 17,
    ACTION_FALL = 18,
    ACTION_LAND_FAST = 19, /* ACTION_STAND's motion, blended in at once */
    ACTION_STOP = 20,
    ACTION_TURN = 21,
    ACTION_PUSH_START = 22,
    ACTION_PUSH = 23,
    ACTION_HURT = 24,
    ACTION_HURT_LAND = 25,
    ACTION_DOWN = 26,
    ACTION_DOWN_LAND = 27,
    ACTION_UPDRAFT = 29,
    ACTION_CRASH = 30,
    ACTION_CRASH_END = 31,
    /* ACTION_HANG_ON's motion at 0.2 frames a step, where ACTION_HANG_ON
     * plays at 0.5. it is her pose for a PLAYER_MODE_PATH_SLIDE request,
     * which only amy_try_request_holding takes, and for a clear in the
     * stages before STAGE_CHAOS0 */
    ACTION_HANG_ON_SLOW = 32,
    ACTION_HANDLE = 33,
    ACTION_BALANCE_START = 35,
    ACTION_BALANCE = 36,
    ACTION_DROWN = 37,
    ACTION_TAKE_DRUM = 38,
    ACTION_RAISE_DRUM = 39,
    ACTION_SET_DRUM = 41, /* ACTION_RAISE_DRUM's motion, at DRUM_FRAME */
    ACTION_CLEAR = 42, /* her pose for a clear from STAGE_CHAOS0 on */
    ACTION_LIFT = 43,
    ACTION_HOLD_RUN = 44,
    ACTION_HOLD_STOP = 45,
    ACTION_HOLD_TURN = 46,
    ACTION_HOLD_STAND = 47,
    ACTION_HOLD_WALK = 48,
    ACTION_BOUNCE_1 = 49,
    ACTION_BOUNCE_2 = 51,
    ACTION_SQUASH = 54,
    ACTION_HANG_ON = 55,
    ACTION_PULL = 56,
    ACTION_SHAKE = 57,
    ACTION_HOLD_SHAKE = 58,
    ACTION_HOLD_SHAKE_END = 60,
    ACTION_GRIP_START = 61,
    ACTION_GRIP = 62,
    ACTION_HOLD_JUMP = 63,
    ACTION_HOLD_FALL_START = 64,
    ACTION_HOLD_FALL = 65,
    ACTION_HOLD_LAND_FAST = 66,
    ACTION_THROW_AT = 67, /* a throw toward the nearest task near her */
    ACTION_THROW = 68, /* a throw straight ahead */
    ACTION_AIR_THROW_AT = 69,
    ACTION_AIR_THROW = 70,
    ACTION_PUT_DOWN = 71,
    ACTION_LEDGE = 72,
    ACTION_PANEL_JUMP = 73,
    ACTION_PANEL_WAIT = 74,
    ACTION_CAUGHT = 75,
    ACTION_RIDE = 76,
    ACTION_ROCKET = 77,
    ACTION_ROCKET_ALONG = 79,
    ACTION_ROCKET_UP = 80,
    /* the swing, and the spin it can go on into */
    ACTION_HAMMER_START = 81,
    ACTION_HAMMER = 82,
    ACTION_HAMMER_END = 83,
    ACTION_SPIN_START = 84,
    ACTION_SPIN_WIND_UP = 85, /* until amy_spin_stick gives TRUE */
    ACTION_SPIN = 86,
    ACTION_DIZZY = 87,
    ACTION_DIZZY_WALK = 88,
    /* the leap off her hammer: its swing, then four actions in the air. the
     * last three play in turn, over and over */
    ACTION_HAMMER_JUMP = 89,
    ACTION_HAMMER_LEAP = 90,
    ACTION_HAMMER_LEAP_2 = 91,
    ACTION_HAMMER_LEAP_3 = 92,
    ACTION_HAMMER_LEAP_4 = 93,
    /* ACTION_AIR_HAMMER_LAND's motion. her code never sets this row, and no
     * row's next names it, but amy_choose_mode keeps her in AMY_HAMMER_JUMP
     * while it plays */
    ACTION_HAMMER_JUMP_LAND = 94,
    ACTION_AIR_HAMMER = 95,
    ACTION_AIR_HAMMER_FALL = 96,
    ACTION_AIR_HAMMER_LAND = 97
};

#define SE_HAMMER 752
#define SE_LONG_HAMMER 753
#define SE_HAMMER_SPIN 754
#define SE_DRUM 756 /* a drum taken or set down */

/* her voice */
#define SE_JUMP_VOICE 1169
#define SE_AIR_HAMMER 1170
#define SE_HAMMER_SWING 1178
#define SE_HAMMER_JUMP 1179
#define SE_DIZZY 1180

#define SE_THROW 1175
#define SE_PUSH_START 1176

/* an action in a_mot.PRS, her motion file, by the address it has once the
 * file is loaded at MOTION_AREA */
#define A_MOT(address) ((NJS_ACTION *)(address))

/* her fields, as elements of record->own's arrays */

/* TRUE while STICK_HEADING holds where the stick pointed: from a step that
 * finds it pushed until it's let go or the spin starts over */
#define STICK_HEADING_KNOWN(record) ((record)->own.b[0])
/* how far the stick has spun one way, in 64ths of a turn */
#define STICK_SPIN(record) ((record)->own.b[1])
/* where the stick pointed, and how far it had turned, at the last step that
 * started STICK_SPIN or added to it. both are in 256ths of a turn */
#define STICK_HEADING(record) ((record)->own.b[2])
#define STICK_TURN(record) ((record)->own.b[3])
#define HAS_DRUM(record) ((record)->own.b[4])
/* TRUE from when amy_set_hammer_scale or amy_grow_hammer sets a scale above
 * 0 until a call of either with 0 or less. while it's TRUE, amy_draw leaves
 * HAMMER_AIM as it is, though her hammer's modes still set it */
#define HAMMER_AIM_HELD(record) ((record)->own.b[5])
/* the CALL_ bits set for her this step, which amy_step clears at its end */
#define CALLS(record) ((record)->own.b[6])
/* TRUE once her swing in the air has hit something and sounded */
#define HAMMER_STRUCK(record) ((record)->own.b[7])
/* the steps her spin has run, then those of her dizziness after it */
#define SPIN_STEPS(record) ((record)->own.w[4])
/* what's left of her struggle in the robo's grabber */
#define STRUGGLE(record) ((record)->own.w[5])
/* how far she tips about x and about z each step on the balance spot */
#define TIP_SPEED_X(record) ((record)->own.w[6])
#define TIP_SPEED_Z(record) ((record)->own.w[7])
/* ACTION_SET_DRUM's frame, which counts down a frame a step */
#define DRUM_FRAME(record) ((record)->own.f[5])
/* the scale her hammer moves toward, and the scale it's drawn at. a scale
 * of 0 hides it */
#define HAMMER_AIM(record) ((record)->own.f[6])
#define HAMMER_SCALE(record) ((record)->own.f[7])

/* in CALLS: to the handle, onto the balance spot, and hidden from Zero */
#define CALL_HANDLE 0x1
#define CALL_BALANCE 0x2
#define CALL_HIDDEN 0x4

/* the bit of her timer that hides her. while the timer counts down after a
 * hit, it hides her two steps in every four, so she blinks */
#define BLINK_HIDDEN 0x2

enum skirt_mode { SKIRT_INIT, SKIRT_FLOW };

/* what a shock from her hammer starts each step. its work->tick holds one
 * of these */
enum shock_kind { SHOCK_RING, SHOCK_WAVE, SHOCK_KINDS };

#define SPIN_LENGTH 300.0F /* the steps of a spin, and of the dizziness */
#define WIND_UP_SPIN 28 /* the STICK_SPIN that ends her wind-up */
#define DIZZY_PRESS 18 /* steps a press takes off her dizziness */

/* a turn of the stick in one step this far or further, in 256ths of a turn,
 * starts her spin over */
#define STICK_TURN_LIMIT 40

/* the speed past which she runs in ACTION_FULL_SPEED, with her hammer out.
 * past it, the action button leaps off her hammer instead of swinging it */
#define FULL_SPEED 1.2F

/* the steps she stands still before she fidgets */
#define FIDGET_STEPS 1800

/* the most she rises a step as she drowns */
#define DROWN_RISE 0.4F

/* the frame of ACTION_HAMMER at which her hammer comes down, and how far
 * ahead of her the shock of a swing starts */
#define HAMMER_STRIKE_FRAME 3.0F
#define SHOCK_AHEAD 15.0F

/* how far from a handle she stands to hold it */
#define HANDLE_REACH 6.0F

/* the frame of ACTION_LIFT from which she has what she lifts */
#define LIFT_FRAME 12.0F

/* amy_hang_move steps her action by hand as she hangs from a path. the
 * action has HANG_FRAMES frames and two holds where she rests, A and B, each
 * from its FIRST frame to its LAST */
#define HANG_FRAMES 63.0F
#define HANG_HOLD_A_FIRST 14.0F
#define HANG_HOLD_A_LAST 18.0F
#define HANG_HOLD_B_FIRST 46.0F
#define HANG_HOLD_B_LAST 50.0F

/* amy_ledge_move steps her action by hand as she hangs from a ledge. the
 * action has LEDGE_FRAMES frames, and she rests at LEDGE_HOLD, midway round
 * them, as at the last */
#define LEDGE_FRAMES 32.0F
#define LEDGE_HOLD 15.0F

/* how far ahead of her she sets a drum down, and how high over her feet
 * she holds one */
#define DRUM_AHEAD 7.5F
#define DRUM_HEIGHT 4.5F

/* her struggle as the grabber catches her, and what each step she presses
 * PAD_MASH_BUTTONS takes off it */
#define STRUGGLE_START 180
#define STRUGGLE_PRESS 20

/* the most TIP_SPEED_X and TIP_SPEED_Z can be either way, and how far she
 * can tip about x or z before she falls off the spot */
#define TIP_SPEED_MOST 0x400
#define TIP_LIMIT 0x3800
/* how far she moves the way she leans as she falls off the spot, and how
 * far down */
#define TIP_FALL_OUT 10.0F
#define TIP_FALL_DOWN 2.0F

/* the balance spot calls her onto it while she's within SPOT_REACH of it and
 * SPOT_HEIGHT above or below it. it calls again only SPOT_WAIT steps after
 * she was last on it */
#define SPOT_REACH 6.0F
#define SPOT_HEIGHT 2.0F
#define SPOT_WAIT 30

/* the node of her pose the feather is drawn at, and the one her hammer's
 * shape follows */
#define FEATHER_NODE 22
#define HAMMER_NODE 36

/* the nodes of her pose her skirt is pushed off: her hips and knees, on her
 * -z and +z sides */
#define HIP_MINUS_Z_NODE 51
#define KNEE_MINUS_Z_NODE 52
#define HIP_PLUS_Z_NODE 61
#define KNEE_PLUS_Z_NODE 62

/* her skirt's hem: the points of its model more than HEM_RADIUS out from
 * her middle. each hangs HEM_LENGTH from a pivot of its own, HEM_DROP below
 * the model's origin, at the point's x and z scaled by HEM_PIVOT_SCALE */
#define HEM_RADIUS 1.7F
#define HEM_LENGTH 1.31F
#define HEM_DROP 0.47F
#define HEM_PIVOT_SCALE (0.83F / 1.93F)
/* the most a point of the hem may rise above its pivot, before the point
 * is set HEM_LENGTH from the pivot */
#define HEM_RISE_MOST (HEM_DROP * 0.8F)
/* a hip or knee pushes a point of the hem away while the square of their
 * distance is under this */
#define HEM_NEAR_SQUARED 1.75F

/* her hair sways in two parts, in her head's model. the top part is the
 * points SWAY_TOP_Y or higher, and the low part the points below SWAY_LOW_Y
 * and more than SWAY_LOW_Z to either side. each part swings round its own
 * pivot, from which the model's origin lies the part's ORIGIN_X along x and
 * ORIGIN_Y along y */
#define SWAY_TOP_Y 0.2F
#define SWAY_TOP_ORIGIN_X 1.0F
#define SWAY_TOP_ORIGIN_Y 3.0F
#define SWAY_LOW_Y (-3.2F)
#define SWAY_LOW_Z 1.7F
#define SWAY_LOW_ORIGIN_X 0.3F
#define SWAY_LOW_ORIGIN_Y 1.2F

/* what a trigger in amy_zero_triggers does to Zero */
enum zero_trigger_kind {
    TRIGGER_LEAVE, /* it leaves once player 0 comes within radius of pos */
    TRIGGER_STAND, /* it stands at target from the start */
    /* it comes in from target toward player 0 once she comes within
     * radius of pos */
    TRIGGER_APPROACH
};

struct zero_trigger {
    Uint16 stage_act;
    Sint16 kind;
    Float radius;
    NJS_POINT3 pos;
    NJS_POINT3 target;
};

#define TRIGGERS_END 0x0fff

/* a trigger's task keeps its radius here. an approach's task also keeps
 * its kind in work->timer and Zero's target in work->scl */
struct trigger_vars {
    Uint8 unk_00[8];
    Float radius;
};

#define TRIGGER_VARS(work) ((struct trigger_vars *)&(work)->vars)

/* an afterimage keeps her action's index in work->timer, and counts its
 * steps in work->tick */
struct afterimage_vars {
    Float frame; /* the frame of her action it shows */
    Float hammer_scale;
    Float height; /* hers: the afterimage is drawn half of it above its pos */
};

#define AFTERIMAGE_VARS(work) ((struct afterimage_vars *)&(work)->vars)

/* her model's root, and the nodes amy_welds joins or marks, each on her
 * -z or +z side */
extern NJS_OBJECT amy_object;
extern NJS_OBJECT amy_upper_arm_minus_z;
extern NJS_OBJECT amy_forearm_minus_z;
extern NJS_OBJECT amy_hand_minus_z;
extern NJS_OBJECT amy_upper_arm_plus_z;
extern NJS_OBJECT amy_forearm_plus_z;
extern NJS_OBJECT amy_hand_plus_z;
extern NJS_OBJECT amy_thigh_minus_z;
extern NJS_OBJECT amy_shin_minus_z;
extern NJS_OBJECT amy_boot_minus_z;
extern NJS_OBJECT amy_thigh_plus_z;
extern NJS_OBJECT amy_shin_plus_z;
extern NJS_OBJECT amy_boot_plus_z;
extern NJS_OBJECT amy_foot_minus_z;
extern NJS_OBJECT amy_toes_minus_z;
extern NJS_OBJECT amy_foot_plus_z;
extern NJS_OBJECT amy_toes_plus_z;
extern NJS_OBJECT amy_palm_minus_z;
extern NJS_OBJECT amy_fingers_minus_z;
extern NJS_OBJECT amy_palm_plus_z;
extern NJS_OBJECT amy_fingers_plus_z;
/* the nodes her look turns: her head and eyes */
extern NJS_OBJECT amy_head_joint;
extern NJS_OBJECT amy_eye0;
extern NJS_OBJECT amy_eye1;
/* the face of her mouth's task: the node amy_head is the sibling of */
extern NJS_OBJECT amy_mouth;
/* her head, whose shape her mouth blends and whose model her sway swaps */
/* @hack type: an array, not the single NJS_OBJECT it is, so SHC loads its
 * address and stores its model at a displacement (MOV.L R2,@(4,R3) at
 * 0x8c0e6e1a and 0x8c0e7144); as one object, SHC folds &amy_head.model into
 * a pool word, and 5 of the 330 words of the three functions that name it
 * and amy_skirt_destroy, whose registers change too, differ */
extern NJS_OBJECT amy_head[];
extern NJS_MODEL amy_head_model;
/* @hack type: an array, not the single NJS_MODEL it is, so SHC reads its
 * points in the delay slot of the test of her action's object (MOV.L
 * @R13,R13 at 0x8c0e6eca) and keeps the task in R10; as one model, the task
 * takes R9 and 34 of amy_sway_step's 296 words differ */
extern NJS_MODEL amy_head_sway_model[]; /* the copy her sway writes */
extern NJS_OBJECT amy_head_blend; /* the head the mouth blends to */
extern NJS_MOTION amy_mouth_shapes; /* the shapes the mouth moves through */
/* @hack type: an array, not the single NJS_OBJECT it is, so SHC loads its
 * address and stores its model at a displacement (MOV.L R2,@(4,R3) at
 * 0x8c0e7168, MOV.L R3,@(4,R2) at 0x8c0e75a8); as one object, 3 of the 483
 * words of amy_skirt_destroy and amy_skirt_step differ */
extern NJS_OBJECT amy_skirt[];
extern NJS_MODEL amy_skirt_model;
/* @hack type: an array, not the single NJS_MODEL it is, so SHC reads its
 * points in the delay slot of the test of her action's object (MOV.L
 * @R11,R11 at 0x8c0e71e2); as one model, the locals take 88 bytes, not 92,
 * the function runs 32 bytes shorter and 344 of amy_skirt_step's 479 words
 * differ */
extern NJS_MODEL amy_skirt_moved_model[]; /* the copy her skirt writes */
/* her -z thumb, whose sibling amy_set_hammer makes her own hammer or the
 * long one, and the warrior feather */
/* @hack type: an array, not the single NJS_OBJECT it is, so SHC reads its
 * sibling at a displacement from its address (MOV.L @(48,R12),R12 at
 * 0x8c0e3a1e); as one object, SHC folds &amy_thumb_minus_z.sibling into a
 * pool word, and 112 of the 654 words of amy_draw, amy_set_hammer and
 * amy_afterimage_step differ */
extern NJS_OBJECT amy_thumb_minus_z[];
extern NJS_OBJECT amy_hammer;
extern NJS_OBJECT amy_long_hammer;
extern NJS_OBJECT amy_feather_object;

extern NJS_ACTION amy_stand_action;
extern NJS_ACTION amy_fidget_start_action;
extern NJS_ACTION amy_fidget_action;
extern NJS_ACTION amy_stand_uphill_action;
extern NJS_ACTION amy_stand_downhill_action;
extern NJS_ACTION amy_walk_action;
extern NJS_ACTION amy_trot_action;
extern NJS_ACTION amy_run_action;
extern NJS_ACTION amy_sprint_action;
extern NJS_ACTION amy_jump_action;
extern NJS_ACTION amy_fall_start_action;
extern NJS_ACTION amy_fall_action;
extern NJS_ACTION amy_stop_action;
extern NJS_ACTION amy_turn_action;
extern NJS_ACTION amy_clear_action;

static void amy_choose_mode(struct task_work *work, struct task_motion *motion,
                            struct player *record);
static void amy_destroy(struct task *task);
static void amy_draw(struct task *task);
static void amy_draw_feather(struct task_work *work, struct task_motion *motion,
                             struct player *record);
static void amy_set_hammer(struct player *record);
static void amy_stand(struct task_work *work, struct task_motion *motion,
                      struct player *record);
static void amy_set_stand_action(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record);
static void amy_set_walk_action(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record);
static void amy_hang_move(struct task_work *work, struct task_motion *motion,
                          struct player *record);
static void amy_ledge_move(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static void amy_edit(struct task_work *work, struct task_motion *motion,
                     struct player *record);
static void amy_balance(struct task_work *work, struct task_motion *motion,
                        struct player *record);
static Bool amy_try_hurt(struct task_work *work, struct task_motion *motion,
                         struct player *record);
static Bool amy_try_jump(struct task_work *work, struct task_motion *motion,
                         struct player *record);
static Bool amy_try_air_hammer(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static Bool amy_try_handle(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static Bool amy_try_handle_off(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static Bool amy_try_balance(struct task_work *work, struct task_motion *motion,
                            struct player *record);
static Bool amy_try_balance_fall(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record);
static Bool amy_try_request(struct task_work *work, struct task_motion *motion,
                            struct player *record);
static Bool amy_try_request_holding(struct task_work *work,
                                    struct task_motion *motion,
                                    struct player *record);
static Bool amy_try_grab(struct task_work *work, struct task_motion *motion,
                         struct player *record);
static Bool amy_try_take_lifted(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record);
static Bool amy_try_throw(struct task_work *work, struct task_motion *motion,
                          struct player *record);
static Bool amy_try_release(struct task_work *work, struct task_motion *motion,
                            struct player *record);
static Bool amy_try_grip(struct task_work *work, struct task_motion *motion,
                         struct player *record);
static void amy_let_go(struct task_work *work, struct task_motion *motion,
                       struct player *record);
static void amy_carry(struct task_work *work, struct task_motion *motion,
                      struct player *record);
static Bool amy_try_fall(struct task_work *work, struct task_motion *motion,
                         struct player *record);
static Bool amy_try_stop(struct task_work *work, struct task_motion *motion,
                         struct player *record);
static Bool amy_try_hammer_jump(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record);
static Bool amy_try_hammer(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static Bool amy_try_take_drum(struct task_work *work, struct player *record);
static Bool amy_try_set_drum(struct task_work *work, struct player *record);
static void amy_set_shapes(struct task_work *work, struct player *record);
static Bool amy_spin_stick(struct task_work *work, struct player *record);
static void amy_shock_step(struct task *task);
static void amy_dizzy_turn(struct task_work *work, struct task_motion *motion,
                           struct player *record);
static void amy_sway_step(struct task *task);
static void amy_skirt_step(struct task *task);
static Bool amy_look_create(Sint8 player, NJS_OBJECT **nodes);
static void amy_look_step(struct task *task);
static void amy_mouth_start(struct task *task);
static void amy_create_zero_triggers(void);
static void amy_create_balance_spot(void);
static Bool amy_draw_drum(struct task_work *work, struct task_motion *motion,
                          struct player *record);
static void amy_afterimage_create(struct task_work *work,
                                  struct player *record);

/* the game's own name for each mode. nothing reads the table, but this
 * object's data opens with it */
static char amy_mode_names[AMY_MODES][8] = {
    "MD_INIT", "MD_STND", "MD_WALK", "MD_1DWK", "MD_1DSP", "MD_JUMP", "MD_SPRG",
    "MD_LNCH", "MD_FALL", "MD_STOP", "MD_PUSH", "MD_DAMG", "MD_BACK", "MD_PLON",
    "MD_CRSH", "MD_HNDL", "MD_BLNC", "MD_DRTK", "MD_DROF", "MD_HAT_", "MD_RHM_",
    "MD_HMJ_", "MD_FHM_", "MD_PICK", "MD_PUT_", "MD_HOLD", "MD_TOCH", "MD_HWLK",
    "MD_THRW", "MD_HSTP", "MD_HFLL", "MD_HJMP", "MD_HSPR", "MD_HLNC", "MD_PULL",
    "MD_THRU", "MD_SHAK", "MD_ESHK", "MD_HGWT", "MD_HGGO", "MD_LDWT", "MD_LDGO",
    "MD_PNJP", "MD_PNWT", "MD_CAPT", "EVENT",   "MD_DOZA", "MD_KILL", "MD_S3A2",
    "MD_EDIT",
};

#define AMY_NODES 78 /* in the model each of her actions moves */

/* her actions, indexed by enum amy_action_index. a row's next is a name from
 * that enum, or a bare index where her code names no row. her steps change
 * two rows as she plays: ACTION_HAMMER's next and ACTION_DIZZY's speed */
static struct player_action amy_actions[] = {
    {&amy_stand_action, AMY_NODES, PLAYER_ANIM_LOOP, ACTION_STAND, 0.0625F,
     1.0F},
    {&amy_stand_action, AMY_NODES, PLAYER_ANIM_ONCE, ACTION_LANDED, 0.25F,
     2.0F},
    {&amy_stand_action, AMY_NODES, PLAYER_ANIM_LOOP, ACTION_LANDED, 0.25F,
     1.0F},
    {&amy_stand_action, AMY_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F, 1.0F},
    {&amy_fidget_start_action, AMY_NODES, PLAYER_ANIM_ONCE, ACTION_FIDGET,
     0.25F, 0.25F},
    {&amy_fidget_action, AMY_NODES, PLAYER_ANIM_ONCE, ACTION_FIDGET_END, 0.25F,
     0.25F},
    {&amy_fidget_start_action, AMY_NODES, PLAYER_ANIM_BACK, ACTION_STAND, 0.25F,
     0.25F},
    {&amy_stand_uphill_action, AMY_NODES, PLAYER_ANIM_LOOP, ACTION_STAND_UPHILL,
     0.25F, 1.0F},
    {&amy_stand_downhill_action, AMY_NODES, PLAYER_ANIM_LOOP,
     ACTION_STAND_DOWNHILL, 0.25F, 1.0F},
    {&amy_walk_action, AMY_NODES, PLAYER_ANIM_LOOP_SPEED, ACTION_WALK, 0.25F,
     3.4F},
    {&amy_trot_action, AMY_NODES, PLAYER_ANIM_LOOP_SPEED, ACTION_TROT, 0.25F,
     1.4F},
    {&amy_run_action, AMY_NODES, PLAYER_ANIM_LOOP_SPEED, ACTION_RUN, 0.25F,
     1.0F},
    {&amy_sprint_action, AMY_NODES, PLAYER_ANIM_LOOP_SPEED, ACTION_SPRINT,
     0.25F, 1.0F},
    {&amy_sprint_action, AMY_NODES, PLAYER_ANIM_LOOP_SPEED, ACTION_FULL_SPEED,
     0.25F, 1.0F},
    {&amy_jump_action, AMY_NODES, PLAYER_ANIM_LOOP_PACE, ACTION_JUMP, 0.25F,
     0.2F},
    {&amy_stand_action, AMY_NODES, PLAYER_ANIM_LOOP_SPEED, 15, 0.25F, 0.7F},
    {&amy_jump_action, AMY_NODES, PLAYER_ANIM_LOOP, ACTION_SPRING, 0.25F, 1.0F},
    {&amy_fall_start_action, AMY_NODES, PLAYER_ANIM_ONCE, ACTION_FALL, 0.125F,
     1.0F},
    {&amy_fall_action, AMY_NODES, PLAYER_ANIM_LOOP, ACTION_FALL, 0.25F, 1.0F},
    {&amy_stand_action, AMY_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F, 1.0F},
    {&amy_stop_action, AMY_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F, 1.0F},
    {&amy_turn_action, AMY_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F, 1.0F},
    {&amy_push_action, AMY_NODES, PLAYER_ANIM_ONCE, ACTION_PUSH, 0.125F, 3.0F},
    {&amy_stand_action, AMY_NODES, PLAYER_ANIM_LOOP, ACTION_PUSH, 0.25F, 1.0F},
    {A_MOT(0x0cc4a520), AMY_NODES, PLAYER_ANIM_LOOP, ACTION_HURT, 0.25F, 1.0F},
    {A_MOT(0x0cc4ed44), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F, 1.0F},
    {A_MOT(0x0cc57f48), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_DOWN_LAND, 0.25F,
     1.0F},
    {A_MOT(0x0cc59fdc), AMY_NODES, PLAYER_ANIM_LOOP, ACTION_DOWN_LAND, 0.25F,
     1.0F},
    {&amy_stop_action, AMY_NODES, PLAYER_ANIM_LOOP, 28, 0.25F, 1.0F},
    {&amy_stand_action, AMY_NODES, PLAYER_ANIM_LOOP, ACTION_UPDRAFT, 0.0625F,
     1.0F},
    {&amy_stand_action, AMY_NODES, PLAYER_ANIM_ONCE, ACTION_CRASH_END, 0.25F,
     1.0F},
    {&amy_stand_action, AMY_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 0.0625F,
     1.0F},
    {A_MOT(0x0cc8a0e0), AMY_NODES, PLAYER_ANIM_LOOP, ACTION_HANG_ON_SLOW,
     0.0625F, 0.2F},
    {A_MOT(0x0cc3a1a8), AMY_NODES, PLAYER_ANIM_ONCE, 34, 0.25F, 1.0F},
    {A_MOT(0x0cc3cb8c), AMY_NODES, PLAYER_ANIM_DRIVEN, 34, 0.25F, 1.0F},
    {A_MOT(0x0cc8fca0), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_BALANCE, 0.25F,
     1.0F},
    {A_MOT(0x0cc93434), AMY_NODES, PLAYER_ANIM_LOOP_PACE, ACTION_BALANCE, 0.25F,
     0.8F},
    {A_MOT(0x0cc119ec), AMY_NODES, PLAYER_ANIM_HOLD, ACTION_DROWN, 1.0F, 0.5F},
    {A_MOT(0x0cc12e80), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_RAISE_DRUM, 1.0F,
     1.0F},
    {A_MOT(0x0cc15aa4), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 1.0F, 1.0F},
    {A_MOT(0x0cc18708), AMY_NODES, PLAYER_ANIM_LOOP, 40, 1.0F, 1.0F},
    {A_MOT(0x0cc15aa4), AMY_NODES, PLAYER_ANIM_DRIVEN, ACTION_SET_DRUM, 1.0F,
     1.0F},
    {&amy_clear_action, AMY_NODES, PLAYER_ANIM_HOLD, ACTION_CLEAR, 1.0F, 0.5F},
    {A_MOT(0x0cc31728), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND, 0.5F,
     1.0F},
    {A_MOT(0x0cc26e98), AMY_NODES, PLAYER_ANIM_LOOP, ACTION_HOLD_RUN, 0.5F,
     1.0F},
    {A_MOT(0x0cc33d9c), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND, 0.5F,
     1.0F},
    {A_MOT(0x0cc35490), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND, 0.5F,
     1.0F},
    {A_MOT(0x0cc27bec), AMY_NODES, PLAYER_ANIM_LOOP, ACTION_HOLD_STAND, 0.5F,
     0.5F},
    {A_MOT(0x0cc2a600), AMY_NODES, PLAYER_ANIM_LOOP, ACTION_HOLD_WALK, 0.5F,
     1.0F},
    {&amy_jump_action, AMY_NODES, PLAYER_ANIM_LOOP, ACTION_SPRING, 0.25F, 1.0F},
    {&amy_fall_start_action, AMY_NODES, PLAYER_ANIM_ONCE, ACTION_FALL, 0.5F,
     1.0F},
    {&amy_jump_action, AMY_NODES, PLAYER_ANIM_LOOP, ACTION_SPRING, 0.25F, 1.0F},
    {&amy_fall_start_action, AMY_NODES, PLAYER_ANIM_ONCE, ACTION_FALL, 0.5F,
     1.0F},
    {&amy_fall_action, AMY_NODES, PLAYER_ANIM_LOOP, ACTION_SPRING, 0.25F, 1.0F},
    {&amy_jump_action, AMY_NODES, PLAYER_ANIM_ONCE, ACTION_SPRING, 0.5F, 1.0F},
    {A_MOT(0x0cc8a0e0), AMY_NODES, PLAYER_ANIM_LOOP, ACTION_HANG_ON, 0.5F,
     0.5F},
    {A_MOT(0x0cc7feb4), AMY_NODES, PLAYER_ANIM_LOOP, ACTION_PULL, 0.5F, 1.0F},
    {A_MOT(0x0cc7c070), AMY_NODES, PLAYER_ANIM_LOOP, ACTION_SHAKE, 0.5F, 0.5F},
    {A_MOT(0x0cc1a18c), AMY_NODES, PLAYER_ANIM_ONCE, 59, 0.5F, 0.5F},
    {A_MOT(0x0cc1beb0), AMY_NODES, PLAYER_ANIM_LOOP, 59, 0.5F, 0.5F},
    {A_MOT(0x0cc1dac4), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND, 0.5F,
     0.5F},
    {A_MOT(0x0cc8b234), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_GRIP, 0.5F, 1.0F},
    {A_MOT(0x0cc8caf8), AMY_NODES, PLAYER_ANIM_LOOP, ACTION_GRIP, 0.5F, 1.0F},
    {A_MOT(0x0cc1ed28), AMY_NODES, PLAYER_ANIM_LOOP, ACTION_HOLD_JUMP, 0.5F,
     1.0F},
    {A_MOT(0x0cc1fe7c), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_HOLD_FALL, 0.5F,
     1.0F},
    {A_MOT(0x0cc216f0), AMY_NODES, PLAYER_ANIM_LOOP, ACTION_HOLD_FALL, 0.5F,
     1.0F},
    {A_MOT(0x0cc23f34), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_HOLD_STAND, 0.5F,
     1.0F},
    {A_MOT(0x0cc84758), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 0.5F, 1.0F},
    {A_MOT(0x0cc8864c), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 0.5F, 1.0F},
    {A_MOT(0x0cc84758), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_FALL, 0.5F, 1.0F},
    {A_MOT(0x0cc8864c), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_FALL, 0.5F, 1.0F},
    {A_MOT(0x0cc31728), AMY_NODES, PLAYER_ANIM_BACK, ACTION_STAND, 1.0F, 1.0F},
    {A_MOT(0x0cc2deb4), AMY_NODES, PLAYER_ANIM_DRIVEN, ACTION_LEDGE, 1.0F,
     0.5F},
    {&amy_jump_action, AMY_NODES, PLAYER_ANIM_LOOP, ACTION_PANEL_JUMP, 0.25F,
     1.0F},
    {&amy_stand_action, AMY_NODES, PLAYER_ANIM_LOOP, ACTION_PANEL_WAIT, 0.25F,
     1.0F},
    {A_MOT(0x0cc66ff0), AMY_NODES, PLAYER_ANIM_LOOP, ACTION_CAUGHT, 0.25F,
     1.0F},
    {MODULE_ACTION(0x0c90818c), AMY_NODES, PLAYER_ANIM_LOOP, ACTION_RIDE, 0.25F,
     1.0F},
    {A_MOT(0x0cc8b234), AMY_NODES, PLAYER_ANIM_ONCE, 78, 0.125F, 1.0F},
    {A_MOT(0x0cc8caf8), AMY_NODES, PLAYER_ANIM_LOOP, 78, 0.125F, 1.0F},
    {A_MOT(0x0cc5d7e4), AMY_NODES, PLAYER_ANIM_LOOP, ACTION_ROCKET_ALONG,
     0.125F, 1.0F},
    {A_MOT(0x0cc5bb70), AMY_NODES, PLAYER_ANIM_LOOP, ACTION_ROCKET_UP, 0.125F,
     1.0F},
    {A_MOT(0x0cc025dc), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_HAMMER, 0.25F,
     1.0F},
    {A_MOT(0x0cc052c0), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_SPIN_START, 0.25F,
     1.0F},
    {A_MOT(0x0cc077c4), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F, 1.0F},
    {A_MOT(0x0cc09318), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_SPIN_WIND_UP, 0.25F,
     1.0F},
    {A_MOT(0x0cc605e8), AMY_NODES, PLAYER_ANIM_LOOP_PACE, ACTION_SPIN_WIND_UP,
     0.25F, 1.0F},
    {A_MOT(0x0cc60f8c), AMY_NODES, PLAYER_ANIM_LOOP_PACE, ACTION_SPIN, 0.25F,
     1.0F},
    {A_MOT(0x0cc66ff0), AMY_NODES, PLAYER_ANIM_LOOP, ACTION_DIZZY, 0.25F, 0.5F},
    {A_MOT(0x0cc6ae84), AMY_NODES, PLAYER_ANIM_LOOP_SPEED, ACTION_DIZZY_WALK,
     0.25F, 3.0F},
    {A_MOT(0x0cc6ca18), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_HAMMER_LEAP, 0.25F,
     1.0F},
    {A_MOT(0x0cc70bfc), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_HAMMER_LEAP_2,
     0.25F, 1.0F},
    {A_MOT(0x0cc75a24), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_HAMMER_LEAP_3,
     0.25F, 1.0F},
    {A_MOT(0x0cc77d38), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_HAMMER_LEAP_4,
     0.25F, 1.0F},
    {A_MOT(0x0cc7a5ac), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_HAMMER_LEAP_2,
     0.25F, 1.0F},
    {A_MOT(0x0cc73ac0), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F, 1.0F},
    {A_MOT(0x0cc39094), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_AIR_HAMMER_FALL,
     0.25F, 1.0F},
    {&amy_fall_action, AMY_NODES, PLAYER_ANIM_LOOP, ACTION_AIR_HAMMER_FALL,
     0.25F, 1.0F},
    {A_MOT(0x0cc73ac0), AMY_NODES, PLAYER_ANIM_ONCE, ACTION_STAND, 0.25F, 1.0F},
};

/* her body, and her hammer, which strikes only as she swings it */
/* clang-format off */
static struct collision_shape amy_shapes[2] = {
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS,
     PUSH_ALL | PUSH_FIRM(PUSH_OTHERS), {0.0F, 1.0F, 0.0F}, 4.0F, 4.0F, 0.65F,
     {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_OFF | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(1) | GUARD(2) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES |
     SHAPE_HURTS_OBJECTS,
     PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 7.5F, 0.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static NJS_OBJECT *amy_look_nodes[3] = {&amy_head_joint, &amy_eye0, &amy_eye1};

/* the vertices amy_welds joins, four pairs to a row: a vertex's index in a
 * weld's object_a, then its partner's in object_b. the welds of a joint on
 * her two sides share a row, and so do her knees' and her ankles' */
static Uint16 amy_weld_indices[5][8] = {
#include "player/amy/amy_weld_indices.inc"
};

/* the seams of her model, then the points it marks: her fingers, her toes
 * and her mouth. the marks keep their points in weld_point0 to
 * weld_point4, in that order */
static struct weld amy_welds[] = {
    {&amy_object, &amy_upper_arm_minus_z, &amy_forearm_minus_z, 4, WELD_TO_A, 0,
     0, NULL, amy_weld_indices[0]},
    {&amy_object, &amy_forearm_minus_z, &amy_hand_minus_z, 4, WELD_TO_A, 0, 0,
     NULL, amy_weld_indices[1]},
    {&amy_object, &amy_upper_arm_plus_z, &amy_forearm_plus_z, 4, WELD_TO_A, 0,
     0, NULL, amy_weld_indices[0]},
    {&amy_object, &amy_forearm_plus_z, &amy_hand_plus_z, 4, WELD_TO_A, 0, 0,
     NULL, amy_weld_indices[1]},
    {&amy_object, &amy_thigh_minus_z, &amy_shin_minus_z, 4, WELD_TO_A, 0, 0,
     NULL, amy_weld_indices[2]},
    {&amy_object, &amy_shin_minus_z, &amy_boot_minus_z, 4, WELD_TO_A, 0, 0,
     NULL, amy_weld_indices[2]},
    {&amy_object, &amy_thigh_plus_z, &amy_shin_plus_z, 4, WELD_TO_A, 0, 0, NULL,
     amy_weld_indices[2]},
    {&amy_object, &amy_shin_plus_z, &amy_boot_plus_z, 4, WELD_TO_A, 0, 0, NULL,
     amy_weld_indices[2]},
    {&amy_object, &amy_foot_minus_z, &amy_toes_minus_z, 4, WELD_TO_A, 0, 0,
     NULL, amy_weld_indices[3]},
    {&amy_object, &amy_foot_plus_z, &amy_toes_plus_z, 4, WELD_TO_A, 0, 0, NULL,
     amy_weld_indices[3]},
    {&amy_object, &amy_palm_minus_z, &amy_fingers_minus_z, 4, WELD_TO_A, 0, 0,
     NULL, amy_weld_indices[4]},
    {&amy_object, &amy_palm_plus_z, &amy_fingers_plus_z, 4, WELD_TO_A, 0, 0,
     NULL, amy_weld_indices[4]},
    {&amy_object, &amy_fingers_minus_z, NULL, 2, WELD_MARK, 0, 0, NULL, NULL},
    {&amy_object, &amy_fingers_plus_z, NULL, 2, WELD_MARK + 1, 0, 0, NULL,
     NULL},
    {&amy_object, &amy_toes_minus_z, NULL, 0, WELD_MARK + 2, 0, 0, NULL, NULL},
    {&amy_object, &amy_toes_plus_z, NULL, 0, WELD_MARK + 3, 0, 0, NULL, NULL},
    {&amy_object, &amy_mouth, NULL, 0, WELD_MARK + 4, 0, 0, NULL, NULL},
    {NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL},
};

/* Zero's triggers, each for the stage and act it names. in Final Egg's
 * first act Zero stands at its target from the start and leaves further on.
 * in Twinkle Park's second act it comes in toward her */
static const struct zero_trigger amy_zero_triggers[] = {
    {STAGE_ACT(STAGE_FINAL, 0),
     TRIGGER_STAND,
     0.0F,
     {0.0F, 0.0F, 0.0F},
     {170.0F, -160.0F, 550.0F}},
    {STAGE_ACT(STAGE_FINAL, 0),
     TRIGGER_LEAVE,
     80.0F,
     {412.0F, -232.0F, -1563.0F},
     {0.0F, 0.0F, 0.0F}},
    {STAGE_ACT(STAGE_TWINKLE, 1),
     TRIGGER_APPROACH,
     80.0F,
     {323.0F, 0.0F, -559.0F},
     {207.0F, -14.0F, -657.0F}},
    {TRIGGERS_END, 0, 0.0F, {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F}},
};

/* scratch matrices her steps share: amy_base_matrix to pose or turn in,
 * and amy_node_matrix for a node of the pose */
static NJS_MATRIX amy_base_matrix;
static NJS_MATRIX amy_node_matrix;

void amy_step(struct task *task)
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
    if (edit_mode == 1) {
        work->mode = AMY_EDIT;
    } else if (work->mode != AMY_DOWN) {
        amy_choose_mode(work, motion, record);
    }
    switch (work->mode) {
    case AMY_INIT: {
        struct task *child;

        PLAYER_VARS(work)->character = CHARACTER_AMY;
        player_init(PLAYER_VARS(work)->player, task);
        record = motion->record;
        record->anim.actions = amy_actions;
        record->anim.action = ACTION_STAND;
        collision_init(task, amy_shapes, COLLISION_PLAYER, 1, 2);
        work->collision->shapes->radius = record->consts.radius;
        work->mode = AMY_STAND;
        texture_load_character(CHARACTER_AMY);
        record->anim.welds = amy_welds;
        amy_look_create(PLAYER_VARS(work)->player, amy_look_nodes);
        child = task_create(TASK_WORK, 3, amy_mouth_start);
        if (child != NULL) {
            PLAYER_VARS(child->work)->player = PLAYER_VARS(work)->player;
        }
        child = task_create(TASK_MOTION | TASK_WORK, 3, amy_sway_step);
        if (child != NULL) {
            PLAYER_VARS(child->work)->player = PLAYER_VARS(work)->player;
        }
        child = task_create(TASK_MOTION | TASK_WORK, 3, amy_skirt_step);
        if (child != NULL) {
            PLAYER_VARS(child->work)->player = PLAYER_VARS(work)->player;
        }
        HAMMER_AIM(record) = 0.0F;
        HAMMER_SCALE(record) = 0.0F;
        HAMMER_AIM_HELD(record) = FALSE;
        CALLS(record) = 0;
        task->destructor = amy_destroy;
        task->draw = amy_draw;
        amy_bird_create();
        zero_init();
        amy_create_zero_triggers();
        HAS_DRUM(record) = FALSE;
        amy_drum_create_all();
        amy_create_balance_spot();
        break;
    }
    case AMY_STAND:
        player_turn_to_stick(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        amy_set_stand_action(work, motion, record);
        break;
    case AMY_WALK:
        if (record->anim.previous == ACTION_TURN &&
            record->anim.current != ACTION_TURN) {
            record->speed.x += record->consts.trot_speed;
        }
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        if (player_collide(work, motion, record) == PLAYER_CRASHED) {
            work->mode = AMY_CRASH;
            record->anim.action = ACTION_CRASH;
            player_crash_burst(work);
            record->speed.x = -record->speed.x;
            record->speed.z = -record->speed.z;
        } else {
            player_take_speed(work, motion, record);
            amy_set_walk_action(work, motion, record);
        }
        break;
    case AMY_PATH:
        player_accel_path(work, motion, record);
        player_apply_path_accel(work, motion, record);
        player_move_on_path(work, motion, record);
        amy_set_walk_action(work, motion, record);
        break;
    case AMY_PATH_SPIN:
        player_accel_roll(work, motion, record);
        player_apply_path_accel(work, motion, record);
        player_move_on_path(work, motion, record);
        break;
    case AMY_JUMP:
    case AMY_AIR_HAMMER:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        /* her swing in the air sounds the first time it hits something */
        if (!HAMMER_STRUCK(record) &&
            record->anim.action == ACTION_AIR_HAMMER &&
            work->collision != NULL) {
            /* @hack spelling: the hit tested in an if of its own, which loads
             * it into R1 (MOV.L @(24,R2),R1 at 0x8c0e03ee); joined to the
             * test above, it loads into R0 and 2 of 1312 words differ */
            if (work->collision->hit != NULL) {
                if (record->upgrades & PLAYER_AMY_LONG_HAMMER) {
                    sound_play(SE_LONG_HAMMER, 0, 0, 0);
                } else {
                    sound_play(SE_HAMMER, 0, 0, 0);
                }
                HAMMER_STRUCK(record) = TRUE;
            }
        }
        break;
    case AMY_SPRING:
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_LAUNCH:
        if (record->speed.y >= record->consts.launch_speed) {
            player_accel_launched(work, motion, record);
        } else {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_FALL:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_STOP:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        if (record->speed.x <= 0.0F &&
            player_stick_back_or_end_event(work, motion)) {
            work->mode = AMY_WALK;
            record->anim.action = ACTION_TURN;
            player_turn_to_stick(work, motion, record);
            work->ang.y = motion->aim.y;
        }
        break;
    case AMY_PUSH:
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
    case AMY_HURT:
        player_turn_upright(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_BACK:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_POSE:
        break;
    case AMY_CRASH:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_HANDLE: {
        Angle ang;

        /* her action's frame follows the stick round, 32 frames to a turn */
        /* @hack redundant: the empty else, which puts the frame's store in the
         * break's delay slot (FMOV.S FR3,@(R0,R3) at 0x8c0e06de); without
         * it, a NOP fills the slot and 9 of 1312 words differ */
        if (amy_read_stick(&ang) == TRUE) {
            record->anim.frame = (Uint16)ang * (1.0F / 2048.0F);
        } else {
        }
        break;
    }
    case AMY_BALANCE:
        amy_balance(work, motion, record);
        break;
    case AMY_HAMMER:
        player_turn_to_stick(work, motion, record);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_HAMMER_SPIN:
        if (record->anim.action == ACTION_DIZZY ||
            record->anim.action == ACTION_DIZZY_WALK) {
            amy_dizzy_turn(work, motion, record);
            player_accel_ground(work, motion, record);
            player_apply_accel(work, motion, record);
        } else if (record->anim.action == ACTION_SPIN_START ||
                   record->anim.action == ACTION_SPIN_WIND_UP) {
            player_turn_to_stick(work, motion, record);
        }
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        sound_play_for(SE_HAMMER_SPIN, (int)work, 1, 0, 2);
        break;
    case AMY_HAMMER_JUMP:
        /* @hack spelling: the same three calls at the end of each arm; written
         * once after the if, SHC joins them to the three that end another
         * case, and 337 of 1312 words differ */
        if (record->anim.action >= ACTION_HAMMER_LEAP &&
            record->anim.action <= ACTION_HAMMER_LEAP_4) {
            player_turn_upright(work, motion, record);
            if (!player_steer_to_target(work, motion, record)) {
                player_accel_air(work, motion, record);
            }
            player_apply_accel(work, motion, record);
            player_collide(work, motion, record);
            player_take_speed(work, motion, record);
        } else {
            player_turn_to_stick(work, motion, record);
            player_accel_ground(work, motion, record);
            player_apply_accel(work, motion, record);
            player_collide(work, motion, record);
            player_take_speed(work, motion, record);
        }
        break;
    case AMY_TAKE_DRUM:
    case AMY_SET_DRUM:
        break;
    case AMY_HANG:
    case AMY_HANG_GO:
        player_slide_on_path(work, motion, record);
        amy_hang_move(work, motion, record);
        break;
    case AMY_LEDGE:
    case AMY_LEDGE_GO:
        player_shift_on_path(work, motion, record);
        amy_ledge_move(work, motion, record);
        break;
    case AMY_PANEL_JUMP:
        work->flags &= ~PLAYER_ON_TASK;
        panel_fly(work, motion, record);
        break;
    case AMY_PANEL_WAIT:
        break;
    case AMY_CAUGHT:
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_LIFT:
        player_turn_to_stick(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_PUT_DOWN:
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_HOLD_STAND:
        player_turn_to_stick(work, motion, record);
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_GRIP:
        player_accel_push(work, motion, record);
        player_apply_push(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_THROW:
        player_turn_to_target(work, motion, record);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_HOLD_WALK:
        if (record->anim.previous == ACTION_HOLD_TURN &&
            record->anim.current != ACTION_HOLD_TURN) {
            record->speed.x += record->consts.trot_speed;
        }
        player_accel_ground(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        amy_set_walk_action(work, motion, record);
        break;
    case AMY_HOLD_STOP:
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_HOLD_FALL:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_HOLD_JUMP:
        player_turn_upright(work, motion, record);
        if (!player_steer_to_target(work, motion, record)) {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_HOLD_SPRING:
        player_accel_air(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_HOLD_LAUNCH:
        if (record->speed.y >= record->consts.launch_speed) {
            player_accel_launched(work, motion, record);
        } else {
            player_accel_air(work, motion, record);
        }
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_PULL:
        player_accel_push(work, motion, record);
        player_apply_push(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_AIR_THROW:
        player_turn_upright(work, motion, record);
        player_turn_to_target(work, motion, record);
        player_accel_launched(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_SHAKE:
        player_shake_touched(work, motion, record);
        break;
    case AMY_HOLD_SHAKE:
        /* what she shakes takes her action's frame, in hundredths, as its
         * accel along each axis */
        record->held->motion->accel.x = record->held->motion->accel.y =
            record->held->motion->accel.z = record->anim.frame * 0.01F;
        break;
    case AMY_EVENT:
        event_paths_step(work);
        break;
    case AMY_DROWN:
        /* once ACTION_DROWN is done, she rises toward the surface of the
         * water above her, at most DROWN_RISE a step */
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
    case AMY_DOWN:
        if (work->flags & PLAYER_ON_GROUND) {
            record->anim.action = ACTION_DOWN_LAND;
        }
        player_turn_upright(work, motion, record);
        player_accel_brake(work, motion, record);
        player_apply_accel(work, motion, record);
        player_collide(work, motion, record);
        player_take_speed(work, motion, record);
        break;
    case AMY_RIDE:
        break;
    case AMY_EDIT:
        amy_edit(work, motion, record);
        break;
    }
    player_anim_step(&record->anim);
    amy_set_hammer(record);
    player_update_welds(work, motion, record);
    /* amy_draw runs here as well as in the task's draw. it also sets her
     * hammer's scale, and her body shape's center on a step it draws her */
    amy_draw(task);
    amy_draw_feather(work, motion, record);
    if (record->held != NULL) {
        amy_carry(work, motion, record);
    }
    shadow_update(work, &record->shadow);
    amy_set_shapes(work, record);
    /* in ACTION_CLEAR she touches nothing */
    if (record->anim.action != ACTION_CLEAR) {
        collision_register(work);
    }
    player_finish_step(work, motion, record);
    amy_drum_clear_touched();
    CALLS(record) = 0;
}

/* choose her mode for this step from her pad, what she touches and what's
 * asked of her. AMY_HAMMER, AMY_HAMMER_SPIN and AMY_SET_DRUM each call
 * amy_try_request twice. the second call can take a mode only where the
 * first follows an event's turn, which ends the event's action */
/* NOLINTNEXTLINE(readability-function-size) */
static void amy_choose_mode(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    if (record != NULL && (record->effects & PLAYER_INVINCIBLE)) {
        work->flags |= PLAYER_RECOILING;
        work->timer = 0;
    } else if (work->timer == 0) {
        if (amy_try_hurt(work, motion, record)) {
            work->flags |= PLAYER_RECOILING;
            if (work->flags & PLAYER_HOLDING) {
                amy_let_go(work, motion, record);
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
    case AMY_STAND:
        if (amy_try_request(work, motion, record)) {
            break;
        }
        if (amy_try_fall(work, motion, record)) {
            break;
        }
        if (amy_try_set_drum(work, record)) {
            break;
        }
        if (amy_try_take_drum(work, record)) {
            break;
        }
        if (amy_try_jump(work, motion, record)) {
            break;
        }
        if (amy_try_grab(work, motion, record)) {
            break;
        }
        if (amy_try_handle(work, motion, record)) {
            break;
        }
        if (amy_try_balance(work, motion, record)) {
            break;
        }
        if (amy_try_hammer(work, motion, record)) {
            break;
        }
        if (player_read_stick(work, NULL, NULL) || record->speed.x != 0.0F) {
            work->mode = AMY_WALK;
            record->anim.action = ACTION_WALK;
        }
        break;
    case AMY_WALK:
        if (amy_try_request(work, motion, record)) {
            break;
        }
        if (amy_try_fall(work, motion, record)) {
            break;
        }
        if (amy_try_set_drum(work, record)) {
            break;
        }
        if (amy_try_jump(work, motion, record)) {
            break;
        }
        if (record->speed.x > FULL_SPEED) {
            if (amy_try_hammer_jump(work, motion, record)) {
                break;
            }
        } else {
            if (amy_try_take_drum(work, record)) {
                break;
            }
            if (amy_try_handle(work, motion, record)) {
                break;
            }
            if (amy_try_hammer(work, motion, record)) {
                break;
            }
            if (amy_try_balance(work, motion, record)) {
                break;
            }
        }
        if (amy_try_stop(work, motion, record)) {
            break;
        }
        if ((work->collision->flags & COLLISION_PUSHED) &&
            !(work->flags & PLAYER_HURT) &&
            (work->collision->flags & COLLISION_HIT_SIDE) &&
            (work->collision->hit->shapes[work->collision->hit_shape].push &
             PUSH_ALL)) {
            /* @hack spelling: a switch of one case, which puts work in R2 and
             * AMY_PUSH in R3 for the arm's stores (0x8c0e126c, 0x8c0e126e);
             * as an if, the two trade and 6 of 4039 words differ */
            switch (player_face_wall(work, motion)) {
            case PLAYER_WALL_HEAD_ON:
                work->mode = AMY_PUSH;
                record->anim.action = ACTION_PUSH_START;
                break;
            }
        }
        if (player_stick_back_or_end_event(work, motion) &&
            record->speed.x >= record->consts.trot_speed) {
            work->mode = AMY_STOP;
            record->anim.action = ACTION_STOP;
            sound_play(SE_SKID, 0, 0, 0);
        }
        break;
    case AMY_PATH:
        work->flags &= ~PLAYER_ATTACKING;
        if (amy_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (amy_try_fall(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (amy_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (record->speed.x < record->consts.trot_speed) {
            work->mode = AMY_WALK;
            record->anim.action = ACTION_WALK;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (amy_try_stop(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case AMY_PATH_SPIN:
        if (amy_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_ATTACKING;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (amy_try_fall(work, motion, record)) {
            record->anim.action = ACTION_JUMP;
            record->action_speed = 1.2F;
            work->ang.x = work->ang.z = 0;
            work->flags |= PLAYER_ATTACKING;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (record->speed.x < PLAYER_SPIN_SPEED) {
            record->anim.action = ACTION_JUMP;
            record->action_speed = 1.2F;
            work->ang.x = work->ang.z = 0;
            work->flags |= PLAYER_ATTACKING;
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case AMY_JUMP:
        if (amy_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = AMY_STOP;
                record->anim.action = ACTION_STOP;
            } else if (amy_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                record->anim.action = ACTION_LAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = AMY_WALK;
                amy_set_walk_action(work, motion, record);
            }
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (amy_try_air_hammer(work, motion, record)) {
            break;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.8F;
        }
        if (record->anim.action == ACTION_JUMP && motion->speed.y <= 0.0F) {
            record->anim.action = ACTION_FALL_START;
        }
        break;
    case AMY_SPRING:
        if (motion->speed.y <= 0.0F) {
            work->mode = AMY_FALL;
            record->anim.action = ACTION_FALL_START;
        }
        break;
    case AMY_LAUNCH:
        if (motion->speed.y <= 0.0F) {
            work->mode = AMY_FALL;
            record->anim.action = ACTION_FALL_START;
        }
        break;
    case AMY_FALL:
        if (amy_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = AMY_STOP;
                record->anim.action = ACTION_STOP;
            } else if (amy_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                record->anim.action = ACTION_LAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = AMY_WALK;
                amy_set_walk_action(work, motion, record);
            }
        }
        break;
    case AMY_STOP:
        if (amy_try_request(work, motion, record)) {
            break;
        }
        if (amy_try_fall(work, motion, record)) {
            break;
        }
        if (amy_try_jump(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_STOP) {
            amy_stand(work, motion, record);
        }
        break;
    case AMY_PUSH:
        if (amy_try_request(work, motion, record) ||
            amy_try_jump(work, motion, record) ||
            amy_try_fall(work, motion, record) ||
            amy_try_stop(work, motion, record)) {
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
                    work->mode = AMY_WALK;
                    record->anim.action = ACTION_WALK;
                    sound_stop(SE_PUSH);
                }
                break;
            default:
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = AMY_PULL;
                    record->anim.action = ACTION_PULL;
                } else {
                    work->mode = AMY_WALK;
                    record->anim.action = ACTION_WALK;
                }
                sound_stop(SE_PUSH);
                break;
            }
            break;
        }
        amy_let_go(work, motion, record);
        work->mode = AMY_WALK;
        record->anim.action = ACTION_WALK;
        sound_stop(SE_PUSH);
        break;
    case AMY_HURT:
        if (amy_try_request(work, motion, record)) {
            break;
        }
        if (TASK_TIMER(work) == PLAYER_HURT_STEPS) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (amy_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                record->anim.action = ACTION_HURT_LAND;
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = AMY_WALK;
                amy_set_walk_action(work, motion, record);
            }
        }
        break;
    case AMY_BACK:
        if (amy_try_request(work, motion, record)) {
            break;
        }
        if (amy_try_fall(work, motion, record)) {
            break;
        }
        if (amy_try_jump(work, motion, record)) {
            break;
        }
        if (amy_try_stop(work, motion, record)) {
            break;
        }
        if (record->speed.x >= 0.0F) {
            work->mode = AMY_WALK;
            amy_set_walk_action(work, motion, record);
            /* @hack redundant: a break where another follows, which puts work
             * in R2 for AMY_POSE's test (0x8c0e1ab8); without it, R3, and 3 of
             * 4039 words differ */
            break;
        }
        break;
    case AMY_POSE:
        if (work->flags & PLAYER_HOLDING) {
            if (amy_try_request_holding(work, motion, record)) {
                break;
            }
        } else {
            if (amy_try_request(work, motion, record)) {
                break;
            }
        }
        break;
    case AMY_CRASH:
        if (amy_try_fall(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_CRASH &&
            record->anim.action != ACTION_CRASH_END) {
            amy_stand(work, motion, record);
        }
        break;
    case AMY_HANDLE:
        if (amy_try_request(work, motion, record)) {
            break;
        }
        if (amy_try_handle_off(work, motion, record)) {
            break;
        }
        break;
    case AMY_BALANCE:
        if (amy_try_request(work, motion, record)) {
            break;
        }
        if (amy_try_jump(work, motion, record)) {
            break;
        }
        if (amy_try_balance_fall(work, motion, record)) {
            break;
        }
        break;
    case AMY_HAMMER:
        if (amy_try_request(work, motion, record)) {
            break;
        }
        if (amy_try_fall(work, motion, record)) {
            break;
        }
        if (record->anim.action == ACTION_HAMMER_END) {
            HAMMER_AIM(record) = 0.0F;
        } else {
            HAMMER_AIM(record) = 1.0F;
        }
        /* as her hammer comes down, a shock of waves and a burst of hearts
         * start SHOCK_AHEAD ahead of her, and the pad shakes */
        if (record->anim.action == ACTION_HAMMER &&
            record->anim.frame >= HAMMER_STRIKE_FRAME &&
            record->anim.frame < HAMMER_STRIKE_FRAME + 1.0F) {
            struct task *child;
            NJS_VECTOR ahead;
            NJS_VECTOR offset;

            child = task_create(TASK_WORK, 6, amy_shock_step);
            if (child != NULL) {
                ahead.x = SHOCK_AHEAD;
                ahead.y = ahead.z = 0.0F;
                njPushMatrix(&_unit_matrix_);
                TURN(njRotateZ, work->ang.z);
                TURN(njRotateX, work->ang.x);
                TURN(njRotateY, -work->ang.y);
                njCalcVector(NULL, &ahead, &offset);
                njPopMatrix(1);
                child->work->pos.x = work->pos.x + offset.x;
                child->work->pos.y = work->pos.y + offset.y;
                child->work->pos.z = work->pos.z + offset.z;
                child->work->ang.x = work->ang.x;
                child->work->ang.z = work->ang.z;
                child->work->timer = 10;
                child->work->tick = SHOCK_WAVE;
                amy_heart_burst(&child->work->pos, work->ang.x, work->ang.z);
            }
            if (record->upgrades & PLAYER_AMY_LONG_HAMMER) {
                sound_play(SE_LONG_HAMMER, 0, 0, 0);
            } else {
                sound_play(SE_HAMMER, 0, 0, 0);
            }
            vibration_start(PLAYER_VARS(work)->player, 3, 15, 0);
        }
        if (amy_try_request(work, motion, record) ||
            record->anim.action == ACTION_STAND) {
            amy_stand(work, motion, record);
            break;
        }
        if (record->anim.action == ACTION_SPIN_START) {
            STICK_HEADING_KNOWN(record) = FALSE;
            work->mode = AMY_HAMMER_SPIN;
            break;
        }
        /* without the warrior feather, or with the action button let go,
         * the swing ends instead of going on into a spin */
        if (!(record->upgrades & PLAYER_AMY_WARRIOR_FEATHER) ||
            !(pad_records[PLAYER_VARS(work)->player].on & pad_action_buttons)) {
            amy_actions[ACTION_HAMMER].next = ACTION_HAMMER_END;
        }
        if (record->anim.action >= ACTION_HAMMER_END) {
            if (amy_try_hammer(work, motion, record)) {
                break;
            }
        }
        break;
    case AMY_HAMMER_SPIN:
        if (amy_try_request(work, motion, record)) {
            break;
        }
        if (amy_try_fall(work, motion, record)) {
            break;
        }
        if (amy_try_request(work, motion, record)) {
            break;
        }
        /* @hack spelling: an empty arm for the ground, with the fall in the
         * else, which puts work in R3 and AMY_FALL in R2 (0x8c0e1f24,
         * 0x8c0e1f26); as one test of !PLAYER_ON_GROUND, the two trade and 8 of
         * 4039 words differ */
        if (work->flags & PLAYER_ON_GROUND) {
        } else {
            work->mode = AMY_FALL;
            record->anim.action = ACTION_FALL;
            break;
        }
        if (record->anim.action != ACTION_DIZZY &&
            record->anim.action != ACTION_DIZZY_WALK) {
            if (!PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                             pad_action_buttons)) {
                amy_stand(work, motion, record);
                break;
            }
        }
        switch (record->anim.action) {
        case ACTION_SPIN_START:
            HAMMER_AIM(record) = 1.0F;
            break;
        case ACTION_SPIN_WIND_UP: {
            /* @hack spelling: the scale through this local, which SHC keeps
             * in a frame slot (FMOV.S FR2,@(R0,R15) at 0x8c0e1ff0); stored
             * straight into HAMMER_AIM, 43 of 4039 words differ */
            Float scale;

            /* her hammer grows as the stick spins, to 1.5 at WIND_UP_SPIN.
             * the further the stick turned at its last step, the faster her
             * action plays */
            scale = (STICK_SPIN(record) * (1.0F / 56.0F)) + 1.0F;
            HAMMER_AIM(record) = scale;
            record->action_speed =
                (_builtin_fabsf((Float)STICK_TURN(record)) * 0.06F) + 0.1F;
            if (amy_spin_stick(work, record)) {
                if (STICK_TURN(record) > 0) {
                    record->action_speed = -1.0F;
                } else {
                    record->action_speed = 1.0F;
                }
                SPIN_STEPS(record) = 0;
                record->anim.action = ACTION_SPIN;
                sound_play(SE_HAMMER_SPIN, 0, 0, 0);
            }
            break;
        }
        case ACTION_SPIN:
            HAMMER_AIM(record) = 1.5F;
            /* after SPIN_LENGTH steps the spin ends, and she's dizzy, with
             * chicks round her head */
            if (++SPIN_STEPS(record) > SPIN_LENGTH) {
                record->speed.x = record->speed.y = record->speed.z = 0.0F;
                SPIN_STEPS(record) = 0;
                record->anim.action = ACTION_DIZZY;
                sound_play(SE_DIZZY, 0, 0, 0);
                task_create(TASK_WORK, 6, amy_chicks_step);
            }
            /* every eighth step a shock of rings starts at her middle, and
             * the pad shakes */
            if (!(SPIN_STEPS(record) & 7)) {
                struct task *child;

                if ((child = task_create(TASK_WORK, 6, amy_shock_step)) !=
                    NULL) {
                    child->work->pos = work->pos;
                    child->work->pos.y += record->consts.height * 0.5F;
                    child->work->ang = work->ang;
                    child->work->timer = 4;
                    child->work->tick = SHOCK_RING;
                }
                vibration_start(PLAYER_VARS(work)->player, 1, 15, 0);
            }
            break;
        case ACTION_DIZZY:
        case ACTION_DIZZY_WALK: {
            Float dizziness;
            Angle ang;

            HAMMER_AIM(record) = 0.0F;
            if (SPIN_STEPS(record) < SPIN_LENGTH / 2.0F &&
                (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                             pad_jump_buttons) ||
                 (pad_records[PLAYER_VARS(work)->player].press &
                  pad_action_buttons))) {
                SPIN_STEPS(record) += DIZZY_PRESS;
            }
            if (++SPIN_STEPS(record) > SPIN_LENGTH) {
                amy_stand(work, motion, record);
                break;
            }
            /* she staggers, less and less as her dizziness wears off.
             * dizziness is the part of it left, from 1 down to 0 */
            dizziness = 1.0F - (SPIN_STEPS(record) * (1.0F / SPIN_LENGTH));
            ang = SPIN_STEPS(record) << 9;
            if (record->speed.x > 0.17F) {
                record->speed.x -= ((dizziness * 0.5F) + 0.5F) *
                                   record->speed.x *
                                   _builtin_fabsf(njSin(ang)) * 0.8F;
                record->speed.z =
                    ((dizziness * 0.5F) + 0.5F) * njCos(ang) * 0.3F;
            }
            if (_builtin_fabsf(record->speed.x) > 0.0F) {
                record->anim.action = ACTION_DIZZY_WALK;
                break;
            }
            amy_actions[ACTION_DIZZY].speed = (dizziness * 0.5F) + 0.3F;
            record->anim.action = ACTION_DIZZY;
            break;
        }
        }
        break;
    case AMY_HAMMER_JUMP:
        if (amy_try_request(work, motion, record)) {
            break;
        }
        switch (record->anim.action) {
        case ACTION_HAMMER_JUMP: {
            /* @hack spelling: the frame count through this local, which takes
             * a frame slot (ADD #-44,R15 at 0x8c0e0e54); compared in place,
             * the locals take 40 bytes, not 44, and 1097 of 4039 words
             * differ */
            Float frames;
            struct task *child;
            NJS_VECTOR ahead;
            NJS_VECTOR offset;

            /* two frames before the swing's end, a shock of waves starts
             * SHOCK_AHEAD ahead of her, and she leaps */
            HAMMER_AIM(record) = 1.0F;
            frames =
                (Float)amy_actions[ACTION_HAMMER_JUMP].action->motion->nbFrame;
            if (record->anim.frame >= frames - 2.0F) {
                if ((child = task_create(TASK_WORK, 6, amy_shock_step)) !=
                    NULL) {
                    ahead.x = SHOCK_AHEAD;
                    ahead.y = ahead.z = 0.0F;
                    njPushMatrix(&_unit_matrix_);
                    TURN(njRotateZ, work->ang.z);
                    TURN(njRotateX, work->ang.x);
                    TURN(njRotateY, -work->ang.y);
                    njCalcVector(NULL, &ahead, &offset);
                    njPopMatrix(1);
                    child->work->pos.x = work->pos.x + offset.x;
                    child->work->pos.y = work->pos.y + offset.y;
                    child->work->pos.z = work->pos.z + offset.z;
                    child->work->ang = work->ang;
                    child->work->timer = 10;
                    child->work->tick = SHOCK_WAVE;
                }
                if (record->upgrades & PLAYER_AMY_LONG_HAMMER) {
                    sound_play(SE_LONG_HAMMER, 0, 0, 0);
                } else {
                    sound_play(SE_HAMMER, 0, 0, 0);
                }
                vibration_start(PLAYER_VARS(work)->player, 6, 15, 0);
                record->speed.y = record->consts.jump_speed * 1.9F;
                /* @hack redundant: a break where another follows, which puts
                 * the record in R3 for the leap's first store (0x8c0e2576);
                 * without it, R2, and 2 of 4039 words differ */
                break;
            }
            break;
        }
        case ACTION_HAMMER_LEAP:
        case ACTION_HAMMER_LEAP_2:
        case ACTION_HAMMER_LEAP_3:
        case ACTION_HAMMER_LEAP_4:
            HAMMER_AIM(record) = 0.0F;
            if (work->flags & PLAYER_ON_GROUND) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (amy_try_stop(work, motion, record)) {
                    record->anim.action = ACTION_LAND;
                } else {
                    work->mode = AMY_WALK;
                    amy_set_walk_action(work, motion, record);
                }
                break;
            }
            if (amy_try_air_hammer(work, motion, record)) {
                break;
            }
            break;
        default:
            HAMMER_AIM(record) = 0.0F;
            HAMMER_SCALE(record) = 0.0F;
            if (record->anim.action != ACTION_HAMMER_JUMP_LAND) {
                work->mode = AMY_STAND;
            }
            break;
        }
        break;
    case AMY_AIR_HAMMER:
        if (amy_try_request(work, motion, record)) {
            break;
        }
        if (record->anim.action == ACTION_AIR_HAMMER_FALL) {
            work->mode = AMY_STAND;
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            record->anim.action = ACTION_AIR_HAMMER_LAND;
            work->mode = AMY_STAND;
            break;
        }
        if (record->anim.action == ACTION_AIR_HAMMER &&
            record->anim.frame <
                (Float)amy_actions[ACTION_AIR_HAMMER].action->motion->nbFrame -
                    2.0F) {
            HAMMER_AIM(record) = 1.0F;
            HAMMER_SCALE(record) = 1.0F;
            break;
        }
        HAMMER_AIM(record) = 0.0F;
        HAMMER_SCALE(record) = 0.0F;
        break;
    case AMY_TAKE_DRUM:
        if (amy_try_request(work, motion, record)) {
            break;
        }
        if (record->anim.action == ACTION_STAND) {
            amy_stand(work, motion, record);
            HAS_DRUM(record) = TRUE;
            /* @hack redundant: a break where another follows, which puts the
             * record in R2 for AMY_SET_DRUM's first store (0x8c0e2782);
             * without it, R1, and 3 of 4039 words differ */
            break;
        }
        break;
    case AMY_SET_DRUM:
        DRUM_FRAME(record) -= 1.0F;
        record->anim.frame = DRUM_FRAME(record);
        if (amy_try_request(work, motion, record)) {
            break;
        }
        /* she sets the drum down DRUM_AHEAD ahead of her once DRUM_FRAME is
         * down to 1, or once the second amy_try_request takes a mode */
        if (amy_try_request(work, motion, record) ||
            DRUM_FRAME(record) <= 1.0F) {
            NJS_POINT3 pos;

            pos = work->pos;
            pos.x += njCos(work->ang.y) * DRUM_AHEAD;
            pos.z += njSin(work->ang.y) * DRUM_AHEAD;
            amy_drum_create(&pos);
            amy_stand(work, motion, record);
            /* @hack redundant: a break where another follows, which lets SHC
             * join this call of amy_stand to AMY_CAUGHT's (BRA at 0x8c0e2816
             * to 0x8c0e2ca4); without it, each case keeps its own call, the
             * object's code runs 64 bytes longer and 1579 of 4039 words
             * differ */
            break;
        }
        break;
    case AMY_HANG: {
        Angle ang;

        if (amy_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (amy_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        /* @hack redundant: an else after an arm that breaks, which puts work in
         * R2 and AMY_FALL in R1 for its stores (0x8c0e28ca, 0x8c0e28cc);
         * without it, R3 and R2, and 13 of 4039 words differ */
        if (work->flags & PLAYER_ON_PATH) {
            if (!player_read_stick(work, &ang, NULL)) {
                break;
            }
            if (angle_distance(work->ang.y, ang) < 0x4000) {
                record->speed.x = 0.1875F;
                work->mode = AMY_HANG_GO;
                /* @hack redundant: a break where another follows, which puts
                 * work in R2 and AMY_FALL in R1 for the fall's stores below
                 * (0x8c0e28ca, 0x8c0e28cc); without it, R0 and R2, and 7 of
                 * 4039 words differ */
                break;
            }
            break;
        } else {
            work->mode = AMY_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
    }
    case AMY_HANG_GO:
        if (amy_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (amy_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        /* @hack spelling: an empty arm for the path, with the fall in the else,
         * which puts work in R2 and AMY_FALL in R1 (0x8c0e296c, 0x8c0e296e);
         * as one test of !PLAYER_ON_PATH, the two trade and 6 of 4039 words
         * differ */
        if (work->flags & PLAYER_ON_PATH) {
        } else {
            work->mode = AMY_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a break where another follows, which leaves a
             * NOP in the JMP's delay slot (0x8c0e2988); without it, the flags'
             * store fills the slot and 24 of 4039 words differ */
            break;
        }
        break;
    case AMY_LEDGE: {
        Angle ang;

        if (amy_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (amy_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        /* @hack redundant: an else after an arm that breaks, which puts work in
         * R1 and AMY_FALL in R2 for its stores (0x8c0e2aa0, 0x8c0e2aa2);
         * without it, R2 and R3, and 13 of 4039 words differ */
        if (work->flags & PLAYER_ON_PATH) {
            if (!player_read_stick(work, &ang, NULL)) {
                break;
            }
            /* she moves along the ledge one way or the other, as the stick
             * points, seen from the camera */
            /* @hack redundant: an else after an arm that breaks, which puts the
             * record in R2 for its store and JMP (0x8c0e2a46, 0x8c0e2a50);
             * without it, R3, and 10 of 4039 words differ */
            if (angle_distance(0xc000 - camera_work->ang.y, ang) < 0x4000) {
                record->speed.x = 0.1875F;
                /* @hack redundant: a break that keeps the two arms apart, each
                 * with its own store and JMP (0x8c0e2a38, 0x8c0e2a46); without
                 * it, SHC joins the else's store and jump to this arm's and 11
                 * of 4039 words differ */
                break;
            } else {
                record->speed.x = -0.1875F;
                /* @hack redundant: a break where another follows, which leaves
                 * a NOP in the JMP's delay slot (0x8c0e2a52); without it, the
                 * store fills the slot and 5 of 4039 words differ */
                break;
            }
            /* @hack redundant: a break that no path reaches, which puts work
             * in R1 and AMY_FALL in R2 for the fall's stores (0x8c0e2aa0,
             * 0x8c0e2aa2); without it, the two trade and 6 of 4039 words
             * differ */
            break;
        } else {
            work->mode = AMY_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
    }
    case AMY_LEDGE_GO:
        if (amy_try_request(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (amy_try_jump(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        /* @hack spelling: an empty arm for the path, with the fall in the else,
         * which puts work in R2 and AMY_FALL in R1 (0x8c0e2b0a, 0x8c0e2b0c);
         * as one test of !PLAYER_ON_PATH, the two trade and 6 of 4039 words
         * differ */
        if (work->flags & PLAYER_ON_PATH) {
        } else {
            work->mode = AMY_FALL;
            record->anim.action = ACTION_FALL;
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a break where another follows, which leaves a
             * NOP in the JMP's delay slot (0x8c0e2b26); without it, the flags'
             * store fills the slot and 9 of 4039 words differ */
            break;
        }
        break;
    case AMY_PANEL_JUMP:
        if (amy_try_request(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            record->speed.x = record->speed.y = record->speed.z = 0.0F;
            motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
            if (panel_jump_ready_or_idle(work)) {
                work->mode = AMY_PANEL_WAIT;
                record->anim.action = ACTION_PANEL_WAIT;
                break;
            }
            work->mode = AMY_STAND;
            record->anim.action = ACTION_LANDED;
        }
        break;
    case AMY_PANEL_WAIT:
        if (amy_try_request(work, motion, record)) {
            break;
        }
        if (!panel_jump_ready_or_idle(work)) {
            work->mode = AMY_STAND;
            record->anim.action = ACTION_LANDED;
            break;
        }
        if (player_jump_pressed(work, record) != PLAYER_JUMP_PANEL) {
            break;
        }
        panel_jump(work);
        work->mode = AMY_PANEL_JUMP;
        record->anim.action = ACTION_PANEL_JUMP;
        break;
    case AMY_CAUGHT:
        if (amy_try_request(work, motion, record)) {
            break;
        }
        if (pad_press_live[PLAYER_VARS(work)->player] & PAD_MASH_BUTTONS) {
            STRUGGLE(record) -= STRUGGLE_PRESS;
        }
        if (STRUGGLE(record) <= 0) {
            amy_stand(work, motion, record);
            /* @hack redundant: a break where another follows, which keeps this
             * call of amy_stand (0x8c0e2ca4) for AMY_SET_DRUM's to join (BRA
             * at 0x8c0e2816); without it, each case keeps its own call, the
             * object's code runs 64 bytes longer and 1651 of 4039 words
             * differ */
            break;
        }
        break;
    case AMY_LIFT:
        if (work->flags & PLAYER_HOLDING) {
            if (amy_try_request_holding(work, motion, record)) {
                break;
            }
        } else {
            if (amy_try_request(work, motion, record)) {
                break;
            }
        }
        if (amy_try_fall(work, motion, record)) {
            break;
        }
        if (amy_try_take_lifted(work, motion, record)) {
            break;
        }
        break;
    case AMY_GRIP:
        if (amy_try_grip(work, motion, record)) {
            break;
        }
        if (amy_try_jump(work, motion, record)) {
            break;
        }
        break;
    case AMY_HOLD_STAND:
        if (amy_try_request_holding(work, motion, record)) {
            break;
        }
        if (amy_try_fall(work, motion, record)) {
            break;
        }
        if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                        pad_grab_buttons) &&
            player_read_stick(work, NULL, NULL) &&
            record->held->work->id == CARRY_KIND_SMALL) {
            work->mode = AMY_HOLD_SHAKE;
            record->anim.action = ACTION_HOLD_SHAKE;
            break;
        }
        if (amy_try_throw(work, motion, record)) {
            break;
        }
        if (amy_try_jump(work, motion, record)) {
            break;
        }
        if (player_read_stick(work, NULL, NULL)) {
            work->mode = AMY_HOLD_WALK;
            record->anim.action = ACTION_HOLD_WALK;
        }
        break;
    case AMY_HOLD_WALK:
        if (amy_try_request_holding(work, motion, record)) {
            break;
        }
        if (amy_try_fall(work, motion, record) ||
            amy_try_jump(work, motion, record) ||
            amy_try_throw(work, motion, record) ||
            amy_try_stop(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
            break;
        }
        if (player_stick_back_or_end_event(work, motion) &&
            record->speed.x >= record->consts.trot_speed) {
            work->mode = AMY_HOLD_STOP;
            record->anim.action = ACTION_HOLD_STOP;
            sound_play(SE_SKID, 0, 0, 0);
            work->flags &= ~PLAYER_ON_PATH;
            /* @hack redundant: a break where another follows, which puts work
             * in R3 for AMY_PUT_DOWN's test (0x8c0e2fa0); without it, R1, and 3
             * of 4039 words differ */
            break;
        }
        break;
    case AMY_PUT_DOWN:
    case AMY_THROW:
        if (work->flags & PLAYER_HOLDING) {
            if (amy_try_request_holding(work, motion, record)) {
                break;
            }
        } else {
            if (amy_try_request(work, motion, record)) {
                break;
            }
        }
        if (amy_try_release(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_THROW_AT &&
            record->anim.action != ACTION_THROW &&
            record->anim.action != ACTION_PUT_DOWN) {
            amy_stand(work, motion, record);
        }
        break;
    case AMY_HOLD_STOP:
        if (amy_try_request_holding(work, motion, record)) {
            break;
        }
        if (amy_try_fall(work, motion, record)) {
            break;
        }
        if (amy_try_jump(work, motion, record)) {
            break;
        }
        if (record->speed.x <= 0.0F &&
            player_stick_back_or_end_event(work, motion)) {
            work->mode = AMY_HOLD_WALK;
            record->anim.action = ACTION_HOLD_TURN;
            player_turn_to_stick(work, motion, record);
            work->ang.y = motion->aim.y;
            break;
        }
        if (record->anim.action != ACTION_HOLD_STOP) {
            work->mode = AMY_HOLD_STAND;
            record->anim.action = ACTION_HOLD_STAND;
        }
        break;
    case AMY_HOLD_FALL:
        if (amy_try_request_holding(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = AMY_HOLD_STOP;
                record->anim.action = ACTION_HOLD_STOP;
            } else if (amy_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (motion->speed.y > -5.0F) {
                    record->anim.action = ACTION_HOLD_STAND;
                } else {
                    record->anim.action = ACTION_HOLD_LAND_FAST;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = AMY_HOLD_WALK;
                amy_set_walk_action(work, motion, record);
            }
            break;
        }
        if (amy_try_throw(work, motion, record)) {
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case AMY_HOLD_JUMP:
        if (amy_try_request_holding(work, motion, record)) {
            break;
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = AMY_HOLD_STOP;
                record->anim.action = ACTION_HOLD_STOP;
            } else if (amy_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (motion->speed.y > -5.0F) {
                    record->anim.action = ACTION_HOLD_STAND;
                } else {
                    record->anim.action = ACTION_HOLD_LAND_FAST;
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                work->mode = AMY_HOLD_WALK;
                amy_set_walk_action(work, motion, record);
            }
            work->flags &= ~PLAYER_ATTACKING;
            break;
        }
        if (player_count_jump_hold(work, record)) {
            record->speed.y += record->consts.jump_hold_speed * 0.4F;
        }
        if (amy_try_throw(work, motion, record)) {
            break;
        }
        if (motion->speed.y <= 0.0F) {
            work->mode = AMY_HOLD_FALL;
            record->anim.action = ACTION_HOLD_FALL_START;
        }
        break;
    case AMY_HOLD_SPRING:
        if (motion->speed.y <= 0.0F) {
            work->mode = AMY_HOLD_FALL;
            record->anim.action = ACTION_HOLD_FALL_START;
        }
        break;
    case AMY_HOLD_LAUNCH:
        if (motion->speed.y <= 0.0F) {
            work->mode = AMY_HOLD_FALL;
            record->anim.action = ACTION_HOLD_FALL_START;
        }
        break;
    case AMY_PULL:
        if (amy_try_request(work, motion, record)) {
            break;
        }
        if (amy_try_jump(work, motion, record)) {
            break;
        }
        if (amy_try_fall(work, motion, record)) {
            break;
        }
        if (!(pad_records[PLAYER_VARS(work)->player].on & pad_grab_buttons)) {
            amy_stand(work, motion, record);
            amy_let_go(work, motion, record);
        }
        if ((work->collision->flags & COLLISION_PUSHED) &&
            !(work->flags & PLAYER_HURT) &&
            !(work->collision->hit->shapes[work->collision->hit_shape].flags &
              SHAPE_NO_PUSH) &&
            (work->collision->flags & COLLISION_HIT_SIDE)) {
            switch (player_stick_into_wall(work, motion)) {
            case PLAYER_WALL_HEAD_ON:
                work->mode = AMY_PUSH;
                record->anim.action = ACTION_PUSH_START;
                break;
            case PLAYER_WALL_NO_STICK:
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = AMY_GRIP;
                    record->anim.action = ACTION_GRIP;
                }
                break;
            default:
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = AMY_PULL;
                    record->anim.action = ACTION_PULL;
                } else {
                    work->mode = AMY_WALK;
                    record->anim.action = ACTION_WALK;
                }
                break;
            }
            break;
        } else {
            amy_let_go(work, motion, record);
            work->mode = AMY_WALK;
            record->anim.action = ACTION_WALK;
            /* @hack redundant: a break where another follows, which puts work
             * in R2 for AMY_AIR_THROW's test (0x8c0e35a0); without it, R1, and
             * 2 of 4039 words differ */
            break;
        }
        /* @hack redundant: a break that no path reaches, which puts work in R2
         * for AMY_AIR_THROW's test (0x8c0e35a0); without it, R1, and 2 of
         * 4039 words differ */
        break;
    case AMY_AIR_THROW:
        if (work->flags & PLAYER_HOLDING) {
            if (amy_try_request_holding(work, motion, record)) {
                break;
            }
        } else {
            if (amy_try_request(work, motion, record)) {
                break;
            }
        }
        if (work->flags & PLAYER_ON_GROUND) {
            if (player_stick_back_or_end_event(work, motion) &&
                record->speed.x > 0.0F) {
                work->mode = AMY_HOLD_STOP;
                record->anim.action = ACTION_HOLD_STOP;
            } else if (amy_try_stop(work, motion, record)) {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (work->flags & PLAYER_HOLDING) {
                    if (motion->speed.y > -5.0F) {
                        record->anim.action = ACTION_HOLD_STAND;
                    } else {
                        record->anim.action = ACTION_HOLD_LAND_FAST;
                    }
                } else {
                    if (motion->speed.y > -5.0F) {
                        amy_set_stand_action(work, motion, record);
                    } else {
                        record->anim.action = ACTION_LAND_FAST;
                    }
                }
            } else {
                work->ang.x = motion->aim.x;
                work->ang.z = motion->aim.z;
                if (work->flags & PLAYER_HOLDING) {
                    work->mode = AMY_HOLD_WALK;
                } else {
                    work->mode = AMY_WALK;
                }
                amy_set_walk_action(work, motion, record);
            }
            break;
        }
        if (amy_try_release(work, motion, record)) {
            break;
        }
        if (record->anim.action != ACTION_AIR_THROW_AT &&
            record->anim.action != ACTION_AIR_THROW) {
            work->mode = AMY_FALL;
            record->anim.action = ACTION_FALL;
        }
        break;
    case AMY_SHAKE:
        if (amy_try_request_holding(work, motion, record)) {
            break;
        }
        if (amy_try_fall(work, motion, record)) {
            break;
        }
        if (amy_try_jump(work, motion, record)) {
            break;
        }
        if (!player_touches_shakable(work, motion, record)) {
            amy_stand(work, motion, record);
            amy_let_go(work, motion, record);
            break;
        }
        if (!player_read_stick(work, NULL, NULL)) {
            work->mode = AMY_GRIP;
            record->anim.action = ACTION_GRIP;
        }
        break;
    case AMY_HOLD_SHAKE:
        if (amy_try_request_holding(work, motion, record)) {
            break;
        }
        if (amy_try_fall(work, motion, record)) {
            break;
        }
        if (amy_try_jump(work, motion, record)) {
            break;
        }
        if (record->held == NULL ||
            record->held->work->id != CARRY_KIND_SMALL) {
            amy_stand(work, motion, record);
            amy_let_go(work, motion, record);
            break;
        }
        if (!player_read_stick(work, NULL, NULL)) {
            work->mode = AMY_HOLD_STAND;
            record->anim.action = ACTION_HOLD_SHAKE_END;
            /* @hack redundant: a break where another follows, which puts work
             * in R3 and work->event in R2 for AMY_EVENT's test (0x8c0e38a0,
             * 0x8c0e38a4); without it, R1 and R3, and 7 of 4039 words
             * differ */
            break;
        }
        break;
    case AMY_EVENT:
        if (work->event->paths == NULL) {
            work->ang.x = motion->aim.x;
            work->ang.z = motion->aim.z;
            amy_stand(work, motion, record);
        }
        break;
    case AMY_RIDE:
        if ((work->flags & PLAYER_REQUEST) &&
            work->submode == PLAYER_MODE_RIDE_OFF) {
            work->mode = AMY_SPRING;
            record->anim.action = ACTION_SPRING;
            work->flags &= ~PLAYER_ATTACKING;
            work->flags &= ~PLAYER_ON_PATH;
        }
        break;
    case AMY_EDIT:
        if (pad_records[0].press & PDD_DGT_TA) {
            work->mode = AMY_WALK;
            record->anim.action = ACTION_WALK;
        }
    }
}

static void amy_destroy(struct task *task)
{
    struct task_work *work = task->work;

    player_free(PLAYER_VARS(work)->player);
    texture_release_character(CHARACTER_AMY);
}

/* set her hammer's scale, then draw her unless her timer has BLINK_HIDDEN
 * set. while she carries a drum, amy_draw_drum draws it and her. a step
 * that draws her also moves her body shape's center to her middle */
static void amy_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;
    NJS_OBJECT *hammer;
    NJS_OBJECT *head;
    struct collision_shape *shape;
    int action;
    Bool carried;

    /* unless HAMMER_AIM_HELD, her hammer is away while an event controls the
     * game and in any action before ACTION_HAMMER_START, and out in
     * ACTION_FULL_SPEED */
    /* @hack spelling: an empty arm for HAMMER_AIM_HELD, whose jump puts
     * event_in_control's address in R3 (0x8c0e397a); as one test of
     * !HAMMER_AIM_HELD round the rest, R2, and 2 of 383 words differ */
    if (HAMMER_AIM_HELD(record) == TRUE) {
    } else if (event_in_control() == TRUE) {
        HAMMER_AIM(record) = 0.0F;
    } else if (record->anim.action == ACTION_FULL_SPEED) {
        HAMMER_AIM(record) = 1.0F;
    } else if (record->anim.action < ACTION_HAMMER_START) {
        HAMMER_AIM(record) = 0.0F;
    }
    /* the scale moves toward HAMMER_AIM by 0.1 each draw */
    if (HAMMER_AIM(record) > HAMMER_SCALE(record) + 0.1F) {
        HAMMER_SCALE(record) += 0.1F;
    } else if (HAMMER_AIM(record) < HAMMER_SCALE(record) - 0.1F) {
        HAMMER_SCALE(record) -= 0.1F;
    } else {
        HAMMER_SCALE(record) = HAMMER_AIM(record);
    }
    hammer = amy_thumb_minus_z->sibling;
    /* @hack spelling: head, the hammer's head, read through her thumb again,
     * not as hammer->child, which keeps hammer in R12 (0x8c0e3a18); as
     * hammer->child, hammer takes R4 and 34 of 383 words differ */
    head = amy_thumb_minus_z->sibling->child;
    hammer->scl[0] = HAMMER_SCALE(record);
    hammer->scl[1] = HAMMER_SCALE(record);
    hammer->scl[2] = HAMMER_SCALE(record);
    head->scl[0] = HAMMER_SCALE(record);
    head->scl[1] = HAMMER_SCALE(record);
    head->scl[2] = HAMMER_SCALE(record);
    if (HAMMER_SCALE(record) == 0.0F) {
        hammer->evalflags |= NJD_EVAL_HIDE;
        head->evalflags |= NJD_EVAL_HIDE;
    } else {
        hammer->evalflags &= ~NJD_EVAL_HIDE;
        head->evalflags &= ~NJD_EVAL_HIDE;
        if (HAMMER_SCALE(record) == 1.0F) {
            hammer->evalflags |= NJD_EVAL_UNIT_SCL;
            head->evalflags |= NJD_EVAL_UNIT_SCL;
        } else {
            hammer->evalflags &= ~NJD_EVAL_UNIT_SCL;
            head->evalflags &= ~NJD_EVAL_UNIT_SCL;
        }
    }
    action = record->anim.action;
    if (record->anim.state == PLAYER_ANIM_BLENDING) {
        action = record->anim.current;
    }
    if (TASK_TIMER(work) & BLINK_HIDDEN) {
        return;
    }
    njSetTexture(&amy_texlist);
    carried = amy_draw_drum(work, motion, record);
    draw_set_palette(DRAW_PLAYER_PALETTE);
    {
        NJS_VECTOR offset;
        NJS_VECTOR middle;
        NJS_VECTOR squash;

        njPushMatrix(NULL);
        offset.x = offset.z = 0.0F;
        offset.y = record->consts.height * 0.5F;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, -work->ang.y);
        njCalcVector(NULL, &offset, &middle);
        njPopMatrix(1);
        njAddVector(&middle, &work->pos);
        njTranslateV(NULL, &middle);
        shape = work->collision->shapes;
        shape->center = middle;
        if (!carried) {
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
            TURN(njRotateY, 0x8000 - work->ang.y);
            /* curled up on the ground, she's drawn low, tipped and
             * squashed */
            if (action == ACTION_JUMP && (work->flags & PLAYER_ON_GROUND)) {
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
            } else if ((pad_records[0].on & PDD_DGT_TX) &&
                       work->mode == AMY_EDIT) {
                draw_tree(
                    record->anim.actions[record->anim.action].action->object);
            } else if (record->anim.state != PLAYER_ANIM_BLENDING) {
                draw_action(record->anim.actions[action].action,
                            record->anim.frame);
            } else {
                draw_action(record->anim.blend, record->anim.frame);
            }
        }
    }
    njPopMatrix(1);
    draw_set_palette(0);
}

/* TRUE while an event plays a motion on her, with its action in *action
 * and its frame in *frame. while the motion blends in from the one before,
 * *frame is 0. FALSE when no event motion plays, with neither set */
static Bool amy_get_event_motion(struct task_work *work, NJS_ACTION **action,
                                 Float *frame)
{
    struct event_work *event = work->event;
    struct event_motion *motions;
    struct event_motion *previous;

    if (event != NULL) {
        motions = event->motions;
        previous = event->previous;
        if (motions != NULL) {
            if (previous != NULL && motions->blend) {
                *action = &motions->action;
                *frame = 0.0F;
            } else {
                *action = &motions->action;
                *frame = event->motion_frame;
            }
            return TRUE;
        }
    }
    return FALSE;
}

/* draw the warrior feather while she has it, at FEATHER_NODE of her pose,
 * or of the motion an event plays on her. it draws on a frame's first step
 * only */
static void amy_draw_feather(struct task_work *work, struct task_motion *motion,
                             struct player *record)
{
    NJS_ACTION *action;
    Float frame;

    /* @hack spelling: the two tests nested, which puts njPushMatrix's address
     * in R1 (0x8c0e3e6a); as early returns, R3, and 2 of 125 words differ */
    if (record->upgrades & PLAYER_AMY_WARRIOR_FEATHER) {
        if (frame_step == 0) {
            njPushMatrix(NULL);
            njTranslateV(NULL, &work->collision->shapes->center);
            TURN(njRotateZ, work->ang.z);
            TURN(njRotateX, work->ang.x);
            TURN(njRotateY, 0x8000 - work->ang.y);
            if (work->event->motions == NULL ||
                !amy_get_event_motion(work, &action, &frame)) {
                if (record->anim.state != PLAYER_ANIM_BLENDING) {
                    action = record->anim.actions[record->anim.action].action;
                } else {
                    action = record->anim.blend;
                }
                frame = record->anim.frame;
            }
            if (pose_action(NULL, action, frame, FEATHER_NODE,
                            &amy_node_matrix) == TRUE) {
                njSetMatrix(NULL, &amy_node_matrix);
                njTranslate(NULL, 0.0F, 0.0F, -0.1F);
                njSetTexture(&amy_texlist);
                draw_tree(&amy_feather_object);
            }
            njPopMatrix(1);
        }
    }
}

static void amy_set_hammer(struct player *record)
{
    if (record->upgrades & PLAYER_AMY_LONG_HAMMER) {
        amy_thumb_minus_z->sibling = &amy_long_hammer;
    } else {
        amy_thumb_minus_z->sibling = &amy_hammer;
    }
}

static void amy_stand(struct task_work *work, struct task_motion *motion,
                      struct player *record)
{
    work->mode = AMY_STAND;
    record->idle_steps = 0;
    amy_set_stand_action(work, motion, record);
}

/* choose her action while she stands. where the ground is within 25
 * degrees of flat, she idles, and fidgets after FIDGET_STEPS. on a slope of
 * 25 to 45 degrees she takes the uphill or the downhill stand, as she faces,
 * unless the ground is WORLD_NO_SLOPE. on steeper ground her action stays */
static void amy_set_stand_action(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record)
{
    NJS_PLANE plane;
    NJS_VECTOR front;
    NJS_VECTOR downhill;
    NJS_VECTOR ahead;
    Float slope;

    slope = -njInnerProduct(&world_gravity, &record->ground_normal);
    if (slope > njCos(NJM_DEG_ANG(25))) {
        if (record->anim.action != ACTION_STAND &&
            record->anim.action != ACTION_LAND &&
            record->anim.action != ACTION_LANDED &&
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
        if (njScalor(&ahead) == 0.0F) {
            /* @hack spelling: ahead stored through &ahead, whose address SHC
             * forms again for each store (MOV R15,R2 and ADD #4,R2 at
             * 0x8c0e411a); stored as ahead.x, 45 of 198 words differ */
            (&ahead)->x = 1.0F;
            (&ahead)->y = (&ahead)->z = 0.0F;
        } else {
            njUnitVector(&ahead);
        }
        njDistanceP2PL(&world_gravity, &plane, &downhill);
        if (njScalor(&downhill) == 0.0F) {
            /* @hack spelling: downhill stored through &downhill, whose address
             * SHC forms again for each store (MOV R15,R2 and ADD #16,R2 at
             * 0x8c0e4158); stored as downhill.x, 13 of 198 words differ */
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

/* choose her action while she moves, by whether she holds anything and by
 * her speed and slip, but not while she turns. her speed and slip also set
 * the action's pace */
static void amy_set_walk_action(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record)
{
    Float speed = record->speed.x + (motion->slip * 5.0F);

    if (record->anim.action == ACTION_TURN ||
        record->anim.action == ACTION_HOLD_TURN) {
        return;
    }
    if (work->flags & PLAYER_HOLDING) {
        if (speed > record->consts.running_speed) {
            record->anim.action = ACTION_HOLD_RUN;
        } else {
            record->anim.action = ACTION_HOLD_WALK;
        }
    } else if (speed > FULL_SPEED) {
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

/* lower her by her height, so she hangs below the path, and step her action
 * by hand. while she moves with motion->drive set, she steps a frame her way
 * each step, round HANG_FRAMES. she stops at a hold she reaches with the
 * stick let go. moving with no drive, she stops if she's in a hold, and
 * otherwise keeps her frame. still, she steps to one end of her hold and
 * rocks over its two frames there. she goes to its last two while
 * record->speed.z isn't 0, and to its first two while it is. the -1.0F
 * stored as she passes the last frame isn't 0, so she steps forward again */
static void amy_hang_move(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    NJS_VECTOR up;

    up.x = 0.0F;
    up.y = record->consts.height;
    up.z = 0.0F;
    player_turn_to_world(work, &up);
    njSubVector(&work->pos, &up);

    if (record->speed.x != 0.0F && motion->drive == 0.0F) {
        if ((record->anim.frame >= HANG_HOLD_A_FIRST &&
             record->anim.frame <= HANG_HOLD_A_LAST) ||
            (record->anim.frame >= HANG_HOLD_B_FIRST &&
             record->anim.frame <= HANG_HOLD_B_LAST)) {
            work->mode = AMY_HANG;
            record->speed.x = 0.0F;
        }
    } else if (record->speed.x > 0.0F) {
        if (record->anim.frame >= HANG_HOLD_A_FIRST &&
            record->anim.frame < HANG_HOLD_B_FIRST) {
            record->anim.frame += 1.0F;
            if (record->anim.frame == HANG_HOLD_B_FIRST &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = AMY_HANG;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame += 1.0F;
            if (record->anim.frame >= HANG_FRAMES) {
                record->anim.frame = 0.0F;
            }
            if (record->anim.frame == HANG_HOLD_A_FIRST &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = AMY_HANG;
                record->speed.x = 0.0F;
            }
        }
    } else if (record->speed.x < 0.0F) {
        if (record->anim.frame > HANG_HOLD_A_LAST &&
            record->anim.frame <= HANG_HOLD_B_LAST) {
            record->anim.frame -= 1.0F;
            if (record->anim.frame == HANG_HOLD_A_LAST &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = AMY_HANG;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame -= 1.0F;
            if (record->anim.frame < 0.0F) {
                record->anim.frame = HANG_FRAMES - 1.0F;
            }
            if (record->anim.frame == HANG_HOLD_B_LAST &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = AMY_HANG;
                record->speed.x = 0.0F;
            }
        }
    } else {
        record->speed.x = 0.0F;
        if (record->anim.frame >= HANG_HOLD_A_FIRST &&
            record->anim.frame <= HANG_HOLD_A_LAST) {
            if (record->speed.z != 0.0F) {
                record->anim.frame += 1.0F;
                if (record->anim.frame > HANG_HOLD_A_LAST) {
                    record->anim.frame = HANG_HOLD_A_LAST - 1.0F;
                    record->speed.z = -1.0F;
                }
            } else {
                record->anim.frame -= 1.0F;
                if (record->anim.frame < HANG_HOLD_A_FIRST) {
                    record->anim.frame = HANG_HOLD_A_FIRST + 1.0F;
                    record->speed.z = 0.0F;
                }
            }
        } else {
            if (record->speed.z != 0.0F) {
                record->anim.frame += 1.0F;
                if (record->anim.frame > HANG_HOLD_B_LAST) {
                    record->anim.frame = HANG_HOLD_B_LAST - 1.0F;
                    record->speed.z = -1.0F;
                }
            } else {
                record->anim.frame -= 1.0F;
                if (record->anim.frame < HANG_HOLD_B_FIRST) {
                    record->anim.frame = HANG_HOLD_B_FIRST + 1.0F;
                    record->speed.z = 0.0F;
                }
            }
        }
    }
}

/* lower her by her height, so she hangs below the ledge, and step her
 * action by hand. while she moves with motion->drive set, she steps a frame
 * her way each step, round LEDGE_FRAMES. she stops at LEDGE_HOLD or the last
 * frame if she reaches it with the stick let go. moving with no drive, she
 * stops if she's at LEDGE_HOLD or the last frame, and otherwise keeps her
 * frame */
static void amy_ledge_move(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    NJS_VECTOR up;

    up.x = 0.0F;
    up.y = record->consts.height;
    up.z = 0.0F;
    player_turn_to_world(work, &up);
    njSubVector(&work->pos, &up);

    if (record->speed.x != 0.0F && motion->drive == 0.0F) {
        if (record->anim.frame == LEDGE_HOLD ||
            record->anim.frame >= LEDGE_FRAMES - 1.0F) {
            work->mode = AMY_LEDGE;
            record->speed.x = 0.0F;
        }
    } else if (record->speed.x > 0.0F) {
        if (record->anim.frame >= 0.0F && record->anim.frame < LEDGE_HOLD) {
            record->anim.frame += 1.0F;
            if (record->anim.frame == LEDGE_HOLD &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = AMY_LEDGE;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame += 1.0F;
            if (record->anim.frame >= LEDGE_FRAMES) {
                record->anim.frame = 0.0F;
            }
            if (record->anim.frame == LEDGE_FRAMES - 1.0F &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = AMY_LEDGE;
                record->speed.x = 0.0F;
            }
        }
    } else if (record->speed.x < 0.0F) {
        if (record->anim.frame > LEDGE_HOLD &&
            record->anim.frame <= LEDGE_FRAMES - 1.0F) {
            record->anim.frame -= 1.0F;
            if (record->anim.frame == LEDGE_HOLD &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = AMY_LEDGE;
                record->speed.x = 0.0F;
            }
        } else {
            record->anim.frame -= 1.0F;
            if (record->anim.frame < 0.0F) {
                record->anim.frame = LEDGE_FRAMES - 1.0F;
            }
            if (record->anim.frame == LEDGE_FRAMES - 1.0F &&
                !player_read_stick(work, NULL, NULL)) {
                work->mode = AMY_LEDGE;
                record->speed.x = 0.0F;
            }
        }
    } else {
        record->speed.x = 0.0F;
    }
}

/* the editor's mode. player 0's B button puts her back at her start. her
 * stick moves her across the ground as the camera faces, or up and down
 * while player 0 holds X */
static void amy_edit(struct task_work *work, struct task_motion *motion,
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

/* take a hit, or a step on ground that hurts, unless she's riding. she's
 * knocked back and hurt for PLAYER_HURT_STEPS, and her pad shakes. a drum
 * she's taking or setting down drops where she stands. TRUE when she's
 * hurt */
static Bool amy_try_hurt(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    if (record != NULL &&
        ((work->flags & PLAYER_HURT) || (record->ground_flags & WORLD_HURTS)) &&
        work->mode != AMY_RIDE) {
        if (work->mode == AMY_TAKE_DRUM || work->mode == AMY_SET_DRUM) {
            amy_drum_create(&work->pos);
        }
        work->mode = AMY_HURT;
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

/* jump at the jump button, unless she has a drum. on a panel she leaps for
 * the next one. holding what she can carry, she jumps with it; holding
 * anything else, she lets go first. her own jump curls her up and takes on
 * the speed of the task she stands on. TRUE when she jumps */
static Bool amy_try_jump(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    /* @hack spelling: player_carry_flags's result through this local, which
     * keeps ~PLAYER_ON_TASK in R9 and 0 in R11 (0x8c0e4a84, 0x8c0e4a8a);
     * tested in place, the two take R10 and R9 and 34 of 162 words differ */
    int flags;

    if (HAS_DRUM(record) == TRUE) {
        return FALSE;
    }
    /* @hack redundant: an else after a return, which puts player_jump_pressed's
     * address in R2 for the second test (0x8c0e4ad2); without it, R3, and 2
     * of 162 words differ */
    if (player_jump_pressed(work, record) == PLAYER_JUMP_PANEL) {
        if (work->flags & PLAYER_HOLDING) {
            amy_let_go(work, motion, record);
        }
        panel_jump(work);
        work->mode = AMY_PANEL_JUMP;
        record->anim.action = ACTION_PANEL_JUMP;
        work->flags &= ~PLAYER_ON_TASK;
        record->jump_steps = 0;
        sound_play(SE_JUMP, 0, 0, 0);
        sound_play(SE_JUMP_VOICE, 0, 0, 0);
        return TRUE;
    } else if (player_jump_pressed(work, record)) {
        if (work->flags & PLAYER_HOLDING) {
            flags = player_carry_flags(work);
            /* @hack redundant: an else after a return, which puts amy_let_go's
             * address in R2 (0x8c0e4b40); without it, R3, and 6 of 162 words
             * differ */
            if (flags & PLAYER_CAN_CARRY) {
                work->mode = AMY_HOLD_JUMP;
                record->speed.y = record->consts.jump_speed;
                record->anim.action = ACTION_HOLD_JUMP;
                work->flags &= ~PLAYER_ON_TASK;
                record->jump_steps = 0;
                sound_play(SE_JUMP, 0, 0, 0);
                return TRUE;
            } else {
                amy_let_go(work, motion, record);
            }
        }
        work->mode = AMY_JUMP;
        if (record->ground_task != NULL) {
            struct task_force *ground = record->ground_task->force;

            if (ground != NULL) {
                player_set_accel(PLAYER_VARS(work)->player, ground->speed.x,
                                 ground->speed.y, ground->speed.z);
            }
        }
        record->speed.y = record->consts.jump_speed;
        record->anim.action = ACTION_JUMP;
        record->action_speed = 1.2F;
        work->flags &= ~PLAYER_ON_TASK;
        work->flags |= PLAYER_ATTACKING;
        record->jump_steps = 0;
        sound_play(SE_JUMP, 0, 0, 0);
        sound_play(SE_JUMP_VOICE, 0, 0, 0);
        return TRUE;
    }
    return FALSE;
}

/* swing her hammer in the air at the action button. TRUE when she swings */
static Bool amy_try_air_hammer(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    if (pad_records[PLAYER_VARS(work)->player].press & pad_action_buttons) {
        HAMMER_STRUCK(record) = FALSE;
        work->mode = AMY_AIR_HAMMER;
        record->anim.action = ACTION_AIR_HAMMER;
        sound_play(SE_AIR_HAMMER, 0, 0, 0);
        vibration_start(PLAYER_VARS(work)->player, 3, 15, 0);
        return TRUE;
    }
    return FALSE;
}

/* take the handle at the action button while one calls her, and stand
 * HANDLE_REACH from it, facing it. TRUE when she takes it */
static Bool amy_try_handle(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    struct amy_handle *handle;

    if ((pad_records[PLAYER_VARS(work)->player].press & pad_action_buttons) &&
        (CALLS(record) & CALL_HANDLE)) {
        if ((handle = amy_get_handle()) != NULL) {
            work->ang.y = (Uint16)(0xc000 - handle->ang);
            work->pos.x = (njSin(handle->ang) * HANDLE_REACH) + handle->pos.x;
            work->pos.z = (njCos(handle->ang) * HANDLE_REACH) + handle->pos.z;
        }
        work->mode = AMY_HANDLE;
        record->anim.action = ACTION_HANDLE;
        return TRUE;
    }
    return FALSE;
}

/* let go of the handle and stand, once it stops calling her or at the jump
 * or action button. TRUE when she lets go */
static Bool amy_try_handle_off(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    if (!(CALLS(record) & CALL_HANDLE) ||
        PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                    pad_action_buttons | pad_jump_buttons)) {
        amy_stand(work, motion, record);
        return TRUE;
    }
    return FALSE;
}

/* step onto the balance spot once it calls her. she stands up straight and
 * starts to tip a random way. TRUE when it calls */
static Bool amy_try_balance(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    Angle ang;

    if (CALLS(record) & CALL_BALANCE) {
        work->ang.x = 0;
        work->ang.z = 0;
        ang = (Angle)RANDOM_ANGLE();
        TIP_SPEED_X(record) = (Sint16)(njSin(ang) * 64.0F);
        TIP_SPEED_Z(record) = (Sint16)(njCos(ang) * 64.0F);
        work->mode = AMY_BALANCE;
        record->anim.action = ACTION_BALANCE_START;
        return TRUE;
    }
    return FALSE;
}

/* keep her balance on the spot. each step, her lean adds to TIP_SPEED_X
 * and TIP_SPEED_Z, and so does the stick, as it points from the camera. each
 * stays within TIP_SPEED_MOST either way, and she tips by them. the more she
 * leans, the faster ACTION_BALANCE plays */
static void amy_balance(struct task_work *work, struct task_motion *motion,
                        struct player *record)
{
    Float x;
    Float y;
    Float push;
    Angle ang;

    TIP_SPEED_X(record) += (Sint16)work->ang.x >> 11;
    TIP_SPEED_Z(record) += (Sint16)work->ang.z >> 11;
    x = (pad_records[PLAYER_VARS(work)->player].x1 << 8) * (1.0F / 32768.0F);
    y = (pad_records[PLAYER_VARS(work)->player].y1 << 8) * (1.0F / 32768.0F);
    push = njSqrt((x * x) + (y * y));
    if (push > 0.1F) {
        ang = (Angle)(atan2f(x, y) * ANGLE_PER_RADIAN) + camera_work->ang.y +
              0x4000;
        TIP_SPEED_X(record) += (Sint16)(push * njSin(ang) * 12.0F);
        TIP_SPEED_Z(record) += (Sint16)(push * njCos(ang) * 12.0F);
    }
    if (TIP_SPEED_X(record) < -TIP_SPEED_MOST) {
        TIP_SPEED_X(record) = -TIP_SPEED_MOST;
    } else if (TIP_SPEED_X(record) > TIP_SPEED_MOST) {
        TIP_SPEED_X(record) = TIP_SPEED_MOST;
    }
    if (TIP_SPEED_Z(record) < -TIP_SPEED_MOST) {
        TIP_SPEED_Z(record) = -TIP_SPEED_MOST;
    } else if (TIP_SPEED_Z(record) > TIP_SPEED_MOST) {
        TIP_SPEED_Z(record) = TIP_SPEED_MOST;
    }
    work->ang.x += TIP_SPEED_X(record);
    work->ang.z += TIP_SPEED_Z(record);
    if (record->anim.action == ACTION_BALANCE) {
        x = njSin(work->ang.x);
        y = njSin(work->ang.z);
        record->action_speed = (njSqrt((x * x) + (y * y)) * 1.5F) + 0.5F;
    }
}

/* fall off the spot once she tips past TIP_LIMIT, moving TIP_FALL_OUT the
 * way she leans and TIP_FALL_DOWN down. TRUE when she falls */
static Bool amy_try_balance_fall(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record)
{
    if (work->ang.x < -TIP_LIMIT || work->ang.x > TIP_LIMIT ||
        work->ang.z < -TIP_LIMIT || work->ang.z > TIP_LIMIT) {
        /* @hack spelling: the move along x added negated, which keeps the
         * image's FNEG and FADD (0x8c0e504c, 0x8c0e504e); as -=, SHC writes
         * an FSUB and 36 of 73 words differ */
        work->pos.x +=
            -(njSin(work->ang.z) * njCos(work->ang.x) * TIP_FALL_OUT);
        /* @hack spelling: -TIP_FALL_DOWN added, which keeps the image's FADD
         * and its -2.0F (0x8c0e5056); as -= TIP_FALL_DOWN, SHC writes an FSUB
         * and 1 of 73 words differ */
        work->pos.y += -TIP_FALL_DOWN;
        work->pos.z += njSin(work->ang.x) * TIP_FALL_OUT;
        work->mode = AMY_FALL;
        record->anim.action = ACTION_FALL_START;
        return TRUE;
    }
    return FALSE;
}

/* follow the action her event gives. EVENT_ACTION_DONE and an action that
 * moves her set PLAYER_EVENT_DRIVEN, and a turn sets her heading: TRUE for
 * these. any other action clears PLAYER_EVENT_DRIVEN and gives FALSE */
static Bool amy_follow_event(struct task_work *work, struct task_motion *motion,
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

/* do what her event or a request asks: the event's action first, then its
 * paths, then the submode a PLAYER_REQUEST gives. TRUE when she takes a new
 * mode, which ends the caller's checks. FALSE can still change her: an
 * event's action and PLAYER_MODE_JOIN_PATH act without a new mode */
static Bool amy_try_request(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    if (work->event->action) {
        amy_follow_event(work, motion, record);
        return FALSE;
    }
    if (work->event->paths != NULL) {
        work->mode = AMY_EVENT;
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
        work->mode = AMY_SPRING;
        record->anim.action = ACTION_SPRING;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_BOUNCE_1:
        work->mode = AMY_SPRING;
        record->anim.action = ACTION_BOUNCE_1;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_BOUNCE_2:
        work->mode = AMY_SPRING;
        record->anim.action = ACTION_BOUNCE_2;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_LAUNCH:
        work->mode = AMY_LAUNCH;
        record->anim.action = ACTION_SPRING;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_UPDRAFT:
        work->mode = AMY_FALL;
        record->anim.action = ACTION_UPDRAFT;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_PATH:
        work->mode = AMY_PATH;
        record->anim.action = ACTION_RUN;
        record->path_distance = 0.0F;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL:
        work->mode = AMY_POSE;
        record->anim.action = ACTION_STAND;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL_PUSH:
        work->mode = AMY_POSE;
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
        work->mode = AMY_POSE;
        record->anim.action = ACTION_HANG_ON;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_GET_ON:
        work->mode = AMY_POSE;
        record->anim.action = ACTION_FALL;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RIDE:
        work->mode = AMY_RIDE;
        record->anim.action = ACTION_RIDE;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_CLEAR:
        work->mode = AMY_POSE;
        if (stage_get_stage_act() < STAGE_ACT(STAGE_CHAOS0, 0)) {
            record->anim.action = ACTION_HANG_ON_SLOW;
        } else {
            record->anim.action = ACTION_CLEAR;
        }
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_ROCKET:
        work->mode = AMY_POSE;
        record->anim.action = ACTION_ROCKET;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_ROCKET_ALONG:
        work->mode = AMY_POSE;
        record->anim.action = ACTION_ROCKET_ALONG;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_ROCKET_UP:
        work->mode = AMY_POSE;
        record->anim.action = ACTION_ROCKET_UP;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_SQUASH_2:
        work->mode = AMY_POSE;
        record->anim.action = ACTION_SQUASH;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RESET:
        amy_stand(work, motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_JOIN_PATH:
        if (work->mode == AMY_WALK ||
            record->speed.x > record->consts.running_speed) {
            record->path_distance = 0.0F;
            if (work->flags & PLAYER_BALL) {
                work->flags &= ~PLAYER_ATTACKING;
            }
            work->flags |= PLAYER_ON_PATH;
        }
        return FALSE;
    case PLAYER_MODE_LEDGE:
        if (work->flags & PLAYER_ON_GROUND) {
            return FALSE;
        }
        work->mode = AMY_LEDGE;
        record->anim.action = ACTION_LEDGE;
        record->anim.frame = 0.0F;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_HOLD:
        work->mode = AMY_HOLD_STAND;
        record->anim.action = ACTION_HOLD_STAND;
        record->held = PLAYER_VARS(work)->arg.carry;
        record->held->work->flags |= CARRY_HELD;
        work->flags |= PLAYER_HOLDING;
        work->flags &= ~COLLISION_TOUCHING;
        work->flags &= ~PLAYER_ATTACKING;
        break;
    case PLAYER_MODE_CAUGHT:
        work->mode = AMY_CAUGHT;
        record->anim.action = ACTION_CAUGHT;
        STRUGGLE(record) = STRUGGLE_START;
        player_clear_speed(motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_DROWN:
        work->mode = AMY_DROWN;
        record->anim.action = ACTION_DROWN;
        break;
    case PLAYER_MODE_DOWN:
        work->mode = AMY_DOWN;
        record->anim.action = ACTION_DOWN;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/* amy_try_request while she holds something, without the event's paths.
 * where the new mode needs her hands, she lets go first */
static Bool amy_try_request_holding(struct task_work *work,
                                    struct task_motion *motion,
                                    struct player *record)
{
    if (work->event->action) {
        amy_follow_event(work, motion, record);
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
        work->mode = AMY_HOLD_SPRING;
        record->anim.action = ACTION_HOLD_JUMP;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_LAUNCH:
        work->mode = AMY_HOLD_LAUNCH;
        record->anim.action = ACTION_HOLD_JUMP;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_UPDRAFT:
        work->mode = AMY_HOLD_FALL;
        record->anim.action = ACTION_HOLD_FALL;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_PATH:
    case PLAYER_MODE_PATH_SPIN:
        amy_let_go(work, motion, record);
        work->mode = AMY_PATH;
        record->anim.action = ACTION_RUN;
        record->path_distance = 0.0F;
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_STILL:
        work->mode = AMY_POSE;
        record->anim.action = ACTION_HOLD_STAND;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_BALL:
    case PLAYER_MODE_STILL_PUSH:
        amy_let_go(work, motion, record);
        work->mode = AMY_POSE;
        record->anim.action = ACTION_PUSH_START;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_HANG_ON:
        work->mode = AMY_POSE;
        record->anim.action = ACTION_HANG_ON;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_GET_ON:
        work->mode = AMY_POSE;
        record->anim.action = ACTION_FALL;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RIDE:
        work->mode = AMY_RIDE;
        record->anim.action = ACTION_RIDE;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_CLEAR:
        amy_let_go(work, motion, record);
        work->mode = AMY_POSE;
        if (stage_get_stage_act() < STAGE_ACT(STAGE_CHAOS0, 0)) {
            record->anim.action = ACTION_HANG_ON_SLOW;
        } else {
            record->anim.action = ACTION_CLEAR;
        }
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_ROCKET:
        amy_let_go(work, motion, record);
        work->mode = AMY_POSE;
        record->anim.action = ACTION_ROCKET;
        work->ang.x = work->ang.z = 0;
        player_clear_speed(motion, record);
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_PATH_SLIDE:
        amy_let_go(work, motion, record);
        work->mode = AMY_POSE;
        record->anim.action = ACTION_HANG_ON_SLOW;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_SQUASH_2:
        work->mode = AMY_POSE;
        record->anim.action = ACTION_HOLD_STAND;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_SQUASH_3:
        work->mode = AMY_POSE;
        record->anim.action = ACTION_HOLD_STAND;
        work->ang.x = work->ang.z = 0;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_RESET:
    case PLAYER_MODE_LAUNCH_SPEED:
    case PLAYER_MODE_WALL_FALL:
        work->mode = AMY_HOLD_STAND;
        record->anim.action = ACTION_HOLD_STAND;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_JOIN_PATH:
        if (work->mode == AMY_HOLD_WALK ||
            record->speed.x > record->consts.running_speed) {
            record->path_distance = 0.0F;
            work->flags |= PLAYER_ON_PATH;
        }
        return FALSE;
    case PLAYER_MODE_LEDGE:
        if (work->flags & PLAYER_ON_GROUND) {
            return FALSE;
        }
        work->mode = AMY_LEDGE;
        record->anim.action = ACTION_LEDGE;
        record->anim.frame = 0.0F;
        motion->speed.x = motion->speed.y = motion->speed.z = 0.0F;
        record->speed.x = record->speed.y = record->speed.z = 0.0F;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags |= PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_HOLD:
        work->mode = AMY_HOLD_STAND;
        record->anim.action = ACTION_HOLD_STAND;
        record->held = PLAYER_VARS(work)->arg.carry;
        record->held->work->flags |= CARRY_HELD;
        work->flags |= PLAYER_HOLDING;
        work->flags &= ~COLLISION_TOUCHING;
        work->flags &= ~PLAYER_ATTACKING;
        break;
    case PLAYER_MODE_CAUGHT:
        amy_let_go(work, motion, record);
        work->mode = AMY_CAUGHT;
        record->anim.action = ACTION_CAUGHT;
        STRUGGLE(record) = STRUGGLE_START;
        player_clear_speed(motion, record);
        if (work->flags & PLAYER_BALL) {
            work->flags &= ~PLAYER_ATTACKING;
        }
        work->flags &= ~PLAYER_ON_PATH;
        break;
    case PLAYER_MODE_DROWN:
        amy_let_go(work, motion, record);
        work->mode = AMY_DROWN;
        record->anim.action = ACTION_DROWN;
        break;
    case PLAYER_MODE_DOWN:
        amy_let_go(work, motion, record);
        work->mode = AMY_DOWN;
        record->anim.action = ACTION_DOWN;
        work->flags &= ~PLAYER_ATTACKING;
        work->flags &= ~PLAYER_ON_PATH;
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

/* take what she faces head on at the grab button. she grips it if she can
 * push it. otherwise she lifts it if she can carry it, and grips it if she
 * can only shake it. TRUE when she takes it */
static Bool amy_try_grab(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    int flags;

    if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                    pad_grab_buttons)) {
        if ((flags = player_carry_flags(work)) != 0) {
            /* @hack spelling: a switch of a case and a default, which puts
             * PLAYER_CAN_PUSH in R2 (0x8c0e5c76); as an if, R3, and 2 of 73
             * words differ */
            switch (player_face_wall(work, motion)) {
            default:
                return FALSE;
            case PLAYER_WALL_HEAD_ON:;
            }
            if (flags & PLAYER_CAN_PUSH) {
                work->mode = AMY_GRIP;
                record->anim.action = ACTION_GRIP_START;
                record->held = work->collision->hit->owner;
                work->flags |= PLAYER_HOLDING;
                return TRUE;
            } else if (flags & PLAYER_CAN_CARRY) {
                work->mode = AMY_LIFT;
                record->anim.action = ACTION_LIFT;
                return TRUE;
            } else if (flags & PLAYER_CAN_SHAKE) {
                work->mode = AMY_GRIP;
                record->anim.action = ACTION_GRIP_START;
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* from LIFT_FRAME of ACTION_LIFT on, take what she faces head on if she can
 * carry it, or else stand. TRUE when she takes it */
static Bool amy_try_take_lifted(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record)
{
    int flags;

    if (record->anim.frame >= LIFT_FRAME) {
        if ((flags = player_carry_flags(work)) != 0) {
            /* @hack spelling: the two tests nested, which puts AMY_HOLD_STAND
             * in R2 and loads record->held again after its store (0x8c0e5d36,
             * 0x8c0e5d48); joined with &&, 21 of 61 words differ */
            if (player_face_wall(work, motion) == PLAYER_WALL_HEAD_ON) {
                if (flags & PLAYER_CAN_CARRY) {
                    work->mode = AMY_HOLD_STAND;
                    record->held = work->collision->hit->owner;
                    record->held->work->flags |= CARRY_HELD;
                    work->flags |= PLAYER_HOLDING;
                    return TRUE;
                }
            }
        }
        amy_stand(work, motion, record);
    }
    return FALSE;
}

/* throw what she holds at the grab button, in the air or on the run, or
 * else put it down. without the button, once PLAYER_HOLDING is clear, she
 * stands or falls and drops record->held. TRUE when she does either */
static Bool amy_try_throw(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    /* @hack redundant: an else after a return, which puts PLAYER_HOLDING in R2
     * and amy_stand's address in R1, and stores the fall's action through
     * an ADD (0x8c0e5e62, 0x8c0e5e70, 0x8c0e5e86); without it, 30 of 112
     * words differ */
    if (pad_records[PLAYER_VARS(work)->player].press & pad_grab_buttons) {
        if (!(work->flags & PLAYER_ON_GROUND)) {
            work->mode = AMY_AIR_THROW;
            if (player_has_target(PLAYER_VARS(work)->player)) {
                record->anim.action = ACTION_AIR_THROW_AT;
            } else {
                record->anim.action = ACTION_AIR_THROW;
            }
        } else if (record->speed.x < record->consts.trot_speed) {
            work->mode = AMY_PUT_DOWN;
            record->anim.action = ACTION_PUT_DOWN;
            player_clear_speed(motion, record);
        } else {
            work->mode = AMY_THROW;
            if (player_has_target(PLAYER_VARS(work)->player)) {
                record->anim.action = ACTION_THROW_AT;
            } else {
                record->anim.action = ACTION_THROW;
            }
        }
        return TRUE;
    } else if (!(work->flags & PLAYER_HOLDING)) {
        if (work->flags & PLAYER_ON_GROUND) {
            amy_stand(work, motion, record);
        } else {
            work->mode = AMY_FALL;
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

/* let go of what she holds at its frame of her throw. ACTION_THROW_AT and
 * ACTION_AIR_THROW_AT toss it toward the nearest of her near tasks.
 * ACTION_THROW and ACTION_AIR_THROW toss it ahead, and ACTION_PUT_DOWN puts
 * it down. TRUE when she lets go. with record->held null, she clears
 * PLAYER_HOLDING instead, and the result is TRUE if it was set */
static Bool amy_try_release(struct task_work *work, struct task_motion *motion,
                            struct player *record)
{
    struct near_task *near;
    /* @hack spelling: a second pointer that steps through the near tasks
     * instead of near, which copies toss.x to 4(R15) and toss.z to 0(R15)
     * for njSqrt (0x8c0e600e, 0x8c0e601a); stepping near, the two slots
     * trade and 62 of 282 words differ */
    struct near_task *each;
    struct task_work *held;
    struct task_work *target;
    struct collision_shape *shape;
    Float distance;
    NJS_VECTOR toss;

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
    if (((record->anim.action == ACTION_THROW_AT &&
          record->anim.frame == 20.0F) ||
         (record->anim.action == ACTION_AIR_THROW_AT &&
          record->anim.frame == 7.0F)) &&
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
    if ((record->anim.action == ACTION_THROW && record->anim.frame == 24.0F) ||
        (record->anim.action == ACTION_AIR_THROW &&
         record->anim.frame == 6.0F)) {
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

/* with the grab button held, shake what she grips if she can. otherwise
 * she pushes it while the stick points into it head on, and pulls it while
 * the stick points any other way. with the button let go, she lifts it if
 * she can carry it, or else lets go and stands. FALSE while she holds the
 * button and player_stick_into_wall gives PLAYER_WALL_NO_STICK, and TRUE
 * otherwise */
static Bool amy_try_grip(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    int wall;

    if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].on,
                    pad_grab_buttons)) {
        wall = player_stick_into_wall(work, motion);
        /* @hack spelling: a return in each arm, which lets SHC join the
         * PLAYER_CAN_SHAKE arm's action store to the last arm's (BRA at
         * 0x8c0e6234 to 0x8c0e6252); with one return after the arms, it
         * joins the head-on arm's and 14 of 81 words differ */
        if (wall == PLAYER_WALL_NO_STICK) {
            return FALSE;
        } else if (player_carry_flags(work) & PLAYER_CAN_SHAKE) {
            work->mode = AMY_SHAKE;
            record->anim.action = ACTION_SHAKE;
            return TRUE;
        } else if (wall == PLAYER_WALL_HEAD_ON) {
            work->mode = AMY_PUSH;
            record->anim.action = ACTION_PUSH;
            return TRUE;
        } else {
            work->mode = AMY_PULL;
            record->anim.action = ACTION_PULL;
            return TRUE;
        }
    } else {
        if (player_carry_flags(work) & PLAYER_CAN_CARRY) {
            work->mode = AMY_LIFT;
            record->anim.action = ACTION_LIFT;
        } else {
            amy_let_go(work, motion, record);
            amy_stand(work, motion, record);
        }
        return TRUE;
    }
    /* @hack redundant: a return that no path reaches, to which the
     * PLAYER_WALL_NO_STICK test branches (BT at 0x8c0e6222 to 0x8c0e62ae);
     * without it, that test returns in place, the arms change order, the
     * function runs 32 bytes longer and 57 of 81 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unreachable-code-return) */
    return FALSE;
}

/* let go of what she holds, and toss it up and ahead if it's a thing she
 * can carry */
static void amy_let_go(struct task_work *work, struct task_motion *motion,
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

/* keep what she can carry between her hands, or 2.5 out from her -z
 * fingers as she throws it. she lets go of what she grips once she touches
 * something else */
static void amy_carry(struct task_work *work, struct task_motion *motion,
                      struct player *record)
{
    struct collision_shape *shape;
    struct task_work *held;
    NJS_POINT3 hands;
    NJS_VECTOR offset;

    held = record->held->work;
    shape = work->collision->shapes;
    if (!(player_carry_flags(work) & PLAYER_CAN_CARRY)) {
        /* @hack redundant: an else after a return, which loads the chain to the
         * hit's owner's work through R0, R3, R1 and R0 (0x8c0e638a to
         * 0x8c0e6390); without it, R3, R2, R1 and R3, and 5 of 212 words
         * differ */
        if (work->collision->hit == NULL) {
            return;
        } else if (held != work->collision->hit->owner->work) {
            amy_let_go(work, motion, record);
            if (record->held == NULL) {
                if (work->flags & PLAYER_ON_GROUND) {
                    amy_stand(work, motion, record);
                } else {
                    work->mode = AMY_FALL;
                    record->anim.action = ACTION_FALL;
                }
            }
        }
        return;
    }
    held->pos = shape->center;
    if (record->anim.action == ACTION_THROW_AT ||
        record->anim.action == ACTION_THROW ||
        record->anim.action == ACTION_AIR_THROW_AT ||
        record->anim.action == ACTION_AIR_THROW) {
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

/* fall once she's off the ground. while PLAYER_BALL is set, she falls
 * curled up, in AMY_JUMP. she keeps hold of what she can carry, and lets go
 * of anything else. TRUE off the ground, and FALSE on it */
static Bool amy_try_fall(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    if (!(work->flags & PLAYER_ON_GROUND)) {
        /* @hack redundant: an else after a return, which puts PLAYER_HOLDING in
         * R2 (0x8c0e65e4); without it, R3, and 2 of 50 words differ */
        if (work->flags & PLAYER_BALL) {
            work->mode = AMY_JUMP;
            return TRUE;
        } else if (work->flags & PLAYER_HOLDING) {
            if (player_carry_flags(work) & PLAYER_CAN_CARRY) {
                work->mode = AMY_HOLD_FALL;
                record->anim.action = ACTION_HOLD_FALL;
                return TRUE;
            }
            amy_let_go(work, motion, record);
        }
        work->mode = AMY_FALL;
        record->anim.action = ACTION_FALL;
        return TRUE;
    }
    return FALSE;
}

/* stop once the stick is let go and she has no speed. TRUE when she
 * stops */
static Bool amy_try_stop(struct task_work *work, struct task_motion *motion,
                         struct player *record)
{
    if (!player_read_stick(work, NULL, NULL) && record->speed.x == 0.0F) {
        if (work->flags & PLAYER_HOLDING) {
            if (player_carry_flags(work) &
                (PLAYER_CAN_PUSH | PLAYER_CAN_SHAKE)) {
                work->mode = AMY_GRIP;
                record->anim.action = ACTION_GRIP;
            } else {
                work->mode = AMY_HOLD_STAND;
                record->anim.action = ACTION_HOLD_STAND;
            }
        } else {
            amy_stand(work, motion, record);
        }
        return TRUE;
    }
    return FALSE;
}

/* at the action button on the run, swing her hammer to leap off it. TRUE
 * when she swings */
static Bool amy_try_hammer_jump(struct task_work *work,
                                struct task_motion *motion,
                                struct player *record)
{
    if (pad_records[PLAYER_VARS(work)->player].press & pad_action_buttons) {
        work->mode = AMY_HAMMER_JUMP;
        record->anim.action = ACTION_HAMMER_JUMP;
        sound_play(SE_HAMMER_JUMP, 0, 0, 0);
        return TRUE;
    }
    return FALSE;
}

/* stop and swing her hammer at the action button. ACTION_HAMMER then goes
 * on into ACTION_SPIN_START, unless her step makes its next
 * ACTION_HAMMER_END. TRUE when she swings */
static Bool amy_try_hammer(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    if (PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                    pad_action_buttons)) {
        amy_actions[ACTION_HAMMER].next = ACTION_SPIN_START;
        record->speed.x = 0.0F;
        record->speed.z = 0.0F;
        STICK_SPIN(record) = 0;
        work->mode = AMY_HAMMER;
        record->anim.action = ACTION_HAMMER_START;
        sound_play(SE_HAMMER_SWING, 0, 0, 0);
        return TRUE;
    }
    return FALSE;
}

/* take the drum amy_drum_get_touched gives at the action button, unless
 * she has one. the drum's task ends. TRUE when she takes it */
static Bool amy_try_take_drum(struct task_work *work, struct player *record)
{
    struct task *drum;

    if (HAS_DRUM(record) != TRUE && (drum = amy_drum_get_touched()) != NULL) {
        /* @hack spelling: the button tested in an if of its own, which puts
         * task_kill's address in R2 (0x8c0e6800); joined to the test above,
         * R3, and 9 of 58 words differ */
        if (pad_records[PLAYER_VARS(work)->player].press & pad_action_buttons) {
            record->speed.x = 0.0F;
            record->speed.z = 0.0F;
            task_kill(drum);
            work->mode = AMY_TAKE_DRUM;
            record->anim.action = ACTION_TAKE_DRUM;
            sound_play(SE_DRUM, 0, 0, 0);
            return TRUE;
        }
    }
    return FALSE;
}

/* set her drum down at the jump or action button. her action runs back
 * from ten frames before its end. TRUE when she sets it down */
static Bool amy_try_set_drum(struct task_work *work, struct player *record)
{
    if (HAS_DRUM(record) == TRUE &&
        PAD_BUTTONS(pad_records[PLAYER_VARS(work)->player].press,
                    pad_action_buttons | pad_jump_buttons)) {
        record->speed.x = 0.0F;
        record->speed.z = 0.0F;
        DRUM_FRAME(record) =
            (Float)amy_actions[ACTION_SET_DRUM].action->motion->nbFrame - 10.0F;
        work->mode = AMY_SET_DRUM;
        record->anim.action = ACTION_SET_DRUM;
        sound_play(SE_DRUM, 0, 0, 0);
        HAS_DRUM(record) = FALSE;
        return TRUE;
    }
    return FALSE;
}

/* turn off her hammer's shape, which stays off until she swings it */
/* @hack spelling: the store as a helper SHC expands in amy_set_shapes, which
 * keeps shapes there in R12, saved at its entry (MOV.L R12,@-R15 at
 * 0x8c0e6944); written in place, shapes takes R4 and 62 of amy_set_shapes's
 * 199 words differ */
static void amy_hammer_shape_off(struct collision_shape *shapes)
{
    shapes[1].flags |= SHAPE_OFF;
}

/* set her body's attack and guard levels by what she's doing. while she
 * swings her hammer, its shape strikes and an afterimage trails her. the
 * shape then follows HAMMER_NODE where her pose has it, and a heart drifts
 * from that node on each step but a frame's first */
static void amy_set_shapes(struct task_work *work, struct player *record)
{
    NJS_POINT3 origin;
    NJS_POINT3 pos;
    struct collision_shape *shapes;
    int attack;
    int guard;

    if (work->collision != NULL) {
        shapes = work->collision->shapes;
        amy_hammer_shape_off(shapes);
        work->flags &= ~PLAYER_GUARD;
        if (record->effects & PLAYER_INVINCIBLE) {
            attack = 3;
            guard = 3;
        } else if (work->mode == AMY_RIDE) {
            attack = 0;
            guard = 1;
        } else {
            guard = attack = 0;
        }
        shapes->levels = (shapes->levels & ~SHAPE_ATTACK) | ATTACK(attack);
        shapes->levels = (shapes->levels & ~SHAPE_GUARD) | GUARD(guard);
    }
    /* @hack spelling: a switch, which SHC tests with one load of the action
     * into R0 and four CMP/EQ (0x8c0e69a0 to 0x8c0e69b0); as an if of four
     * tests, SHC copies the action into R0 before each, and 65 of 199 words
     * differ */
    switch (record->anim.action) {
    case ACTION_HAMMER:
    case ACTION_SPIN:
    case ACTION_HAMMER_JUMP:
    case ACTION_AIR_HAMMER:
        /* @hack spelling: levels = levels & ~SHAPE_ATTACK, which reads the
         * levels through MOV #4,R0 (0x8c0e69e2, 0x8c0e69e8); as &=, SHC
         * reaches them by a displacement, the function runs 32 bytes shorter
         * and 138 of 199 words differ */
        work->collision->shapes->levels =
            work->collision->shapes->levels & ~SHAPE_ATTACK;
        work->collision->shapes->levels =
            (work->collision->shapes->levels & ~SHAPE_GUARD) | GUARD(3);
        work->collision->shapes[1].flags &= ~SHAPE_OFF;
        /* @bug logic: the test ORs PLAYER_AMY_LONG_HAMMER with her
         * upgrades where an AND was meant (OR #128,R0 at 0x8c0e6a0a), so it
         * always holds, and either hammer strikes as wide as the long one */
        if (record->upgrades | PLAYER_AMY_LONG_HAMMER) {
            work->collision->shapes[1].radius = 9.0F;
        } else {
            work->collision->shapes[1].radius = 7.5F;
        }
        work->flags |= PLAYER_GUARD;
        njUnitMatrix(&amy_base_matrix);
        njTranslateV(&amy_base_matrix, &work->collision->shapes->center);
        TURN_MATRIX(njRotateZ, &amy_base_matrix, work->ang.z);
        TURN_MATRIX(njRotateX, &amy_base_matrix, work->ang.x);
        TURN_MATRIX(njRotateY, &amy_base_matrix, 0x8000 - work->ang.y);
        if (pose_action(
                &amy_base_matrix, amy_actions[record->anim.action].action,
                record->anim.frame, HAMMER_NODE, &amy_node_matrix) == TRUE) {
            origin.x = 0.0F;
            origin.y = 0.0F;
            origin.z = 0.0F;
            njCalcPoint(&amy_node_matrix, &origin, &pos);
            work->collision->shapes[1].center = pos;
            if (frame_step) {
                amy_heart_create(&pos);
            }
        }
        amy_afterimage_create(work, record);
        break;
    }
}

/* add the stick's turn this step to STICK_SPIN, and give TRUE once it
 * reaches WIND_UP_SPIN. the turn, from STICK_HEADING, adds in 64ths of a
 * turn, rounded down. it adds only while it's under STICK_TURN_LIMIT and,
 * once STICK_SPIN isn't 0, the same way as STICK_TURN. a step with no turn
 * takes 2 off instead. letting the stick go, a turn of STICK_TURN_LIMIT or
 * more and a turn the other way each start the spin over */
static Bool amy_spin_stick(struct task_work *work, struct player *record)
{
    Float x;
    Float y;
    Angle ang;
    Sint8 heading;
    Sint8 turn;
    Sint8 distance;

    x = (Float)(pad_records[PLAYER_VARS(work)->player].x1 << 8);
    y = (Float)(pad_records[PLAYER_VARS(work)->player].y1 << 8);
    if (x <= PAD_DEAD_ZONE && x >= -PAD_DEAD_ZONE && y <= PAD_DEAD_ZONE &&
        y >= -PAD_DEAD_ZONE) {
        STICK_SPIN(record) = 0;
        STICK_TURN(record) = 0;
        STICK_HEADING_KNOWN(record) = FALSE;
    } else {
        ang = (Angle)(atan2f(y, x) * ANGLE_PER_RADIAN);
        heading = ang >> 8;
        if (STICK_HEADING_KNOWN(record)) {
            turn = angle_difference(STICK_HEADING(record) << 8, ang) >> 8;
            /* @hack spelling: distance assigned in the test, which puts turn in
             * R4 and distance in R6 (0x8c0e6c06, 0x8c0e6c0e); assigned before
             * the test, the two trade and 3 of 154 words differ */
            if ((distance = turn) < 0) {
                distance = -turn;
            }
            if (distance < STICK_TURN_LIMIT) {
                if (STICK_SPIN(record)) {
                    /* @bug logic: the same-way test below wants the turn
                     * XOR STICK_TURN above 0 (XOR R6,R1 and CMP/PL R1 at
                     * 0x8c0e6c3c), but a turn equal to STICK_TURN gives 0, so
                     * it starts the spin over as a turn back does */
                    if (turn == 0) {
                        if ((STICK_SPIN(record) -= 2) < 0) {
                            STICK_SPIN(record) = 0;
                        }
                    } else if ((STICK_TURN(record) ^ turn) > 0) {
                        STICK_SPIN(record) += distance >> 2;
                        STICK_HEADING(record) = heading;
                        STICK_TURN(record) = turn;
                        if (STICK_SPIN(record) >= WIND_UP_SPIN) {
                            return TRUE;
                        }
                    } else {
                        STICK_SPIN(record) = 0;
                        STICK_HEADING_KNOWN(record) = FALSE;
                    }
                } else {
                    /* @hack redundant: & 15, which changes nothing, distance
                     * being under STICK_TURN_LIMIT, and keeps the image's
                     * AND R3,R5 (0x8c0e6c78); without it, 42 of 154 words
                     * differ */
                    STICK_SPIN(record) += (distance >> 2) & 15;
                    STICK_HEADING(record) = heading;
                    STICK_TURN(record) = turn;
                }
            } else {
                STICK_SPIN(record) = 0;
                STICK_HEADING_KNOWN(record) = FALSE;
            }
        } else {
            STICK_HEADING_KNOWN(record) = TRUE;
            STICK_SPIN(record) = 0;
            STICK_HEADING(record) = heading;
            STICK_TURN(record) = 0;
        }
    }
    return FALSE;
}

/* a shock from her hammer. each step until its timer runs out, it starts
 * the ring or the wave its tick names */
static void amy_shock_step(struct task *task)
{
    struct task_work *work = task->work;

    if (--TASK_TIMER(work) == 0) {
        task_kill(task);
        return;
    }
    /* @hack spelling: a bare scope, which declares the table after the
     * timer's test, so SHC copies it into the frame only once the test
     * passes (0x8c0e6d02); declared at the function's top, it copies the
     * table at the entry and 30 of 64 words differ */
    {
        void (*steps[SHOCK_KINDS])(struct task *task) = {amy_shock_ring_step,
                                                         amy_shock_wave_step};
        Uint8 kind;
        struct task *child;

        /* @hack spelling: kind assigned in the test, which keeps it in the
         * frame (MOV.B R0,@(4,R15) at 0x8c0e6d14); assigned in a statement of
         * its own, kind takes R13 and 63 of 64 words differ */
        if ((kind = work->tick) < SHOCK_KINDS &&
            (child = task_create(TASK_WORK, 6, steps[kind])) != NULL) {
            child->work->pos = work->pos;
            child->work->ang.x = work->ang.x;
            child->work->ang.z = work->ang.z;
        }
    }
}

/* turn her while she's dizzy, unless an event drives her. she turns toward
 * an aim SPIN_STEPS 64ths of a turn past her heading. while the stick is
 * pushed, the aim is that far past where it points, as seen from the
 * camera */
static void amy_dizzy_turn(struct task_work *work, struct task_motion *motion,
                           struct player *record)
{
    Angle ang = SPIN_STEPS(record) << 10;
    Float x;
    Float y;
    Angle heading;

    if (work->flags & PLAYER_EVENT_DRIVEN) {
        return;
    }
    if (!player_read_stick(work, NULL, NULL)) {
        ang += work->ang.y;
        motion->aim.y = ang;
    } else {
        x = (Float)(pad_records[PLAYER_VARS(work)->player].x1 << 8);
        y = (Float)(pad_records[PLAYER_VARS(work)->player].y1 << 8);
        heading = (Angle)(atan2f(y, x) * ANGLE_PER_RADIAN);
        motion->aim.y = heading + ang - camera_work->ang.y;
    }
    work->ang.y = angle_approach(work->ang.y, motion->aim.y, 16);
}

static void amy_sway_destroy(struct task *task)
{
    amy_head->model = &amy_head_model;
}

/* swing her hair's two parts as she moves, while her action plays on her
 * own model. the swing goes into amy_head_sway_model, a copy of her head's
 * model that takes the model's place. both parts swing by amy_base_matrix's
 * tilt. amy_node_matrix gets half the tilt too, which the step never
 * reads */
static void amy_sway_step(struct task *task)
{
    struct task_work *work = task->work;
    struct player_sway *sway = PLAYER_SWAY(task);
    struct task_motion *owner_motion;
    struct player *owner;
    NJS_POINT3 *from;
    NJS_POINT3 *to;
    Uint32 count;
    Uint32 i;
    Float speed;
    Float tilt;
    Angle step;
    /* @hack spelling: 0x800 through this local, which holds it in R4 beside
     * step in R5 (0x8c0e6fc8, 0x8c0e6fd6) and keeps the task in R10; written
     * in its place, the two trade, the task takes R9 and 26 of 296 words
     * differ */
    Angle limit;
    NJS_VECTOR point;

    if (player_work[PLAYER_VARS(work)->player] == NULL) {
        task_destroy(task);
        return;
    }
    owner_motion = player_motion[PLAYER_VARS(work)->player];
    owner = player_get(PLAYER_VARS(work)->player);
    to = amy_head_sway_model->points;
    /* @hack spelling: the return in an else, whose jump SHC deletes, which puts
     * amy_sway_destroy's address in R3 and 1 in R2 (0x8c0e6ee4, 0x8c0e6ee6);
     * as an early return before the loads, the two trade and 8 of 296 words
     * differ */
    if (amy_actions[owner->anim.action].action->object == &amy_object) {
        from = amy_head_model.points;
        count = amy_head_model.nbPoint;
    } else {
        return;
    }
    switch (work->mode) {
    case SWAY_INIT:
        work->mode = SWAY_SWAY;
        task->destructor = amy_sway_destroy;
        break;
    case SWAY_SWAY:
        speed = njSqrt((owner_motion->speed.x * owner_motion->speed.x) +
                       (owner_motion->speed.y * owner_motion->speed.y) +
                       (owner_motion->speed.z * owner_motion->speed.z)) *
                2.0F;
        if (speed > _builtin_fabsf(sway->bob)) {
            /* @hack spelling: the absolute value of speed, a length and never
             * negative, by ?:, which keeps the task in R10 (MOV R4,R10 at
             * 0x8c0e6e36); as _builtin_fabsf, the task takes R9 and 48 of 296
             * words differ, and without it 48 differ as well */
            speed = speed < 0.0F ? -speed : speed;
            if (speed > 6.0F) {
                speed = 6.0F;
            }
            sway->bob = speed;
        }
        if (sway->bob >= 0.2F || sway->bob <= -0.2F) {
            sway->bob = sway->bob * 0.99F;
        } else {
            sway->bob = 0.0F;
        }
        limit = 0x800;
        step = (Angle)_builtin_fabsf(sway->bob * 512.0F) + limit;
        if (step > limit) {
            step = limit;
        }
        sway->bob_phase += step;
        /* @hack spelling: the tilt in two statements, which keeps the owner's
         * motion in R8 and the sway in R9 (0x8c0e6eae, 0x8c0e6ed4); in one,
         * the two trade and 36 of 296 words differ */
        tilt = njCos(sway->bob_phase);
        tilt = tilt * sway->bob * 819.2F;
        work->ang.z = (Angle)tilt;
        njUnitMatrix(&amy_base_matrix);
        TURN_MATRIX(njRotateZ, &amy_base_matrix, work->ang.z);
        njUnitMatrix(&amy_node_matrix);
        TURN_MATRIX(njRotateZ, &amy_node_matrix, work->ang.z >> 1);
        for (i = 0; i < count; i++) {
            if (from->y >= SWAY_TOP_Y) {
                point.x = from->x + SWAY_TOP_ORIGIN_X;
                point.y = from->y + SWAY_TOP_ORIGIN_Y;
                point.z = from->z;
                njCalcVector(&amy_base_matrix, &point, to);
                to->x -= SWAY_TOP_ORIGIN_X;
                to->y -= SWAY_TOP_ORIGIN_Y;
            } else if (from->y < SWAY_LOW_Y &&
                       _builtin_fabsf(from->z) > SWAY_LOW_Z) {
                point.x = from->x + SWAY_LOW_ORIGIN_X;
                point.y = from->y + SWAY_LOW_ORIGIN_Y;
                point.z = from->z;
                njCalcVector(&amy_base_matrix, &point, to);
                to->x -= SWAY_LOW_ORIGIN_X;
                to->y -= SWAY_LOW_ORIGIN_Y;
            }
            from++;
            to++;
        }
        break;
    default:
        task_destroy(task);
        break;
    }
    amy_head->model = amy_head_sway_model;
}

static void amy_skirt_destroy(struct task *task)
{
    amy_skirt->model = &amy_skirt_model;
}

/* flow her skirt as she moves, while her action plays on her own model.
 * the flow goes into amy_skirt_moved_model, a copy of her skirt's model
 * that takes the model's place. each point of the hem hangs from its pivot,
 * and the copy's point eases toward where it hangs */
static void amy_skirt_step(struct task *task)
{
    struct task_work *work = task->work;
    struct player *owner;
    NJS_POINT3 *from;
    NJS_POINT3 *to;
    Uint32 count;
    Uint32 i;
    int j;
    NJS_POINT3 joints[4];
    Float move;
    Float squared;

    /* @hack spelling: the first test reads the work through task->work, which
     * keeps the task in R14 and the work in R13 (MOV R4,R14 at 0x8c0e7184);
     * through work, the two trade, the locals take 88 bytes, not 92, the
     * function runs 32 bytes shorter and 345 of 479 words differ */
    if (player_work[PLAYER_VARS(task->work)->player] == NULL) {
        task_destroy(task);
        return;
    }
    owner = player_get(PLAYER_VARS(work)->player);
    to = amy_skirt_moved_model->points;
    if (amy_actions[owner->anim.action].action->object != &amy_object) {
        return;
    }
    from = amy_skirt_model.points;
    count = amy_skirt_model.nbPoint;
    switch (work->mode) {
    case SKIRT_INIT:
        work->mode = SKIRT_FLOW;
        task->destructor = amy_skirt_destroy;
        break;
    case SKIRT_FLOW: {
        /* @hack spelling: two bare scopes, whose locals share frame slots:
         * origin and offset at 32, and the two pos at 20 (ADD #20,R6 at
         * 0x8c0e7280, ADD #32,R5 at 0x8c0e7284); in one block, the locals
         * take 116 bytes, not 92, and 18 of 479 words differ */
        {
            NJS_POINT3 origin;
            NJS_POINT3 pos;

            origin.x = 0.0F;
            origin.y = 0.0F;
            origin.z = 0.0F;
            njUnitMatrix(&amy_base_matrix);
            if (pose_action(&amy_base_matrix,
                            amy_actions[owner->anim.action].action,
                            owner->anim.frame, KNEE_PLUS_Z_NODE,
                            &amy_node_matrix) == TRUE) {
                njCalcPoint(&amy_node_matrix, &origin, &pos);
                joints[2] = pos;
            }
            if (pose_get_matrix(KNEE_MINUS_Z_NODE, &amy_node_matrix)) {
                njCalcPoint(&amy_node_matrix, &origin, &pos);
                joints[3] = pos;
            }
            if (pose_get_matrix(HIP_PLUS_Z_NODE, &amy_node_matrix)) {
                njCalcPoint(&amy_node_matrix, &origin, &pos);
                joints[0] = pos;
            }
            if (pose_get_matrix(HIP_MINUS_Z_NODE, &amy_node_matrix)) {
                njCalcPoint(&amy_node_matrix, &origin, &pos);
                joints[1] = pos;
            }
        }
        {
            NJS_POINT3 offset;
            NJS_POINT3 pos;
            NJS_VECTOR delta;

            /* @hack spelling: a test, then a do-while, which tests i >= count
             * (CMP/HS at 0x8c0e7312) before loading the constants SHC hoists
             * out of the loop; as a for loop, it loads them first and 263 of
             * 479 words differ */
            if ((i = 0) >= count) {
                break;
            }
            do {
                /* @hack spelling: from->z through this local, which keeps the
                 * locals at 92 bytes (ADD #-92,R15 at 0x8c0e7182); without it,
                 * 88, the function runs 32 bytes shorter and 337 of 479 words
                 * differ */
                Float from_z = from->z;

                squared = (from->x * from->x) + (from_z * from_z);
                if (squared > HEM_RADIUS * HEM_RADIUS) {
                    /* offset is the point's pivot, and pos the point from
                     * its pivot, moved by her speed */
                    offset.x = from->x * HEM_PIVOT_SCALE;
                    offset.y = -HEM_DROP;
                    offset.z = from->z * HEM_PIVOT_SCALE;
                    pos.x = from->x + owner->speed.x - offset.x;
                    pos.y = from->y - owner->speed.y - offset.y;
                    pos.z = from->z + owner->speed.z - offset.z;
                    /* pos never points in past the pivot: along x or z, a
                     * part of the other sign from the pivot's becomes 0 */
                    if (pos.x * offset.x < 0.0F) {
                        pos.x = 0.0F;
                    }
                    if (pos.z * offset.z < 0.0F) {
                        pos.z = 0.0F;
                    }
                    /* @hack redundant: a call of njScale that never runs, a use
                     * of 1.31F, 0.376F and 0.001F that keeps them in FR14, FR12
                     * and FR13; without it, the locals take 88 bytes, not 92
                     * (ADD #-92,R15 at 0x8c0e7182), the function runs 32 bytes
                     * shorter and 312 of 479 words differ */
                    if (0) {
                        njScale(NULL, 1.31F, 0.47F * 0.8F, 0.001F);
                    }
                    if (pos.y > HEM_RISE_MOST) {
                        pos.y = HEM_RISE_MOST;
                    }
                    /* the first of her posed hips and knees whose squared
                     * distance from the point is under HEM_NEAR_SQUARED
                     * pushes it away. it moves as far again from the joint,
                     * and never down */
                    for (j = 0; j < 4; j++) {
                        delta.x = pos.x + offset.x - joints[j].x;
                        delta.y = pos.y + offset.y - joints[j].y;
                        delta.z = pos.z + offset.z - joints[j].z;
                        /* @hack spelling: delta.y through this local, which
                         * keeps it in FR4 for its square
                         * (FMOV.S @(R0,R13),FR4 at 0x8c0e745c); without it,
                         * FR6, and 6 of 479 words differ */
                        {
                            Float delta_y = delta.y;

                            squared = (delta.x * delta.x) +
                                      (delta_y * delta_y) + (delta.z * delta.z);
                        }
                        if (squared < HEM_NEAR_SQUARED) {
                            if (delta.y < 0.0F) {
                                delta.y *= -1.0F;
                            }
                            pos.x += delta.x;
                            pos.y += delta.y;
                            pos.z += delta.z;
                            break;
                        }
                    }
                    /* @hack spelling: pos stored through &pos, whose address
                     * SHC forms again for each store (MOV R15,R2 and
                     * ADD #20,R2 at 0x8c0e74cc); stored as pos.x, the
                     * function runs 32 bytes shorter and 134 of 479 words
                     * differ */
                    if (njScalor(&pos) == 0.0F) {
                        (&pos)->x = 1.0F;
                        (&pos)->y = (&pos)->z = 0.0F;
                    } else {
                        njUnitVector(&pos);
                    }
                    pos.x *= HEM_LENGTH;
                    pos.y *= HEM_LENGTH;
                    pos.z *= HEM_LENGTH;
                    /* the copy's point eases a fifth of the way there a
                     * step */
                    move = (pos.x + offset.x - to->x) * 0.2F;
                    if (_builtin_fabsf(move) < 0.001F) {
                        move = 0.0F;
                    }
                    to->x += move;
                    move = (pos.y + offset.y - to->y) * 0.2F;
                    if (_builtin_fabsf(move) < 0.001F) {
                        move = 0.0F;
                    }
                    to->y += move;
                    move = (pos.z + offset.z - to->z) * 0.2F;
                    if (_builtin_fabsf(move) < 0.001F) {
                        move = 0.0F;
                    }
                    to->z += move;
                }
                from++;
                to++;
            } while (++i < count);
            /* @hack redundant: a store of 0.376 to pos.y that never runs, a use
             * that ranks 0.376 ahead of 0.2 for FR12; without it, the locals
             * take 88 bytes, not 92 (ADD #-92,R15 at 0x8c0e7182), the function
             * runs 32 bytes shorter and 309 of 479 words differ */
            if (0) {
                pos.y = 0.47F * 0.8F;
            }
        }
        break;
    }
    default:
        task_destroy(task);
        break;
    }
    amy_skirt->model = amy_skirt_moved_model;
}

/* make the task that turns her head and eyes, nodes[0] to nodes[2]. FALSE
 * when there's no task or no heap for it. with the task made, it returns
 * without a value, leaving 96 in R0 */
/* NOLINTBEGIN(clang-diagnostic-return-type) */
static Bool amy_look_create(Sint8 player, NJS_OBJECT **nodes)
{
    struct task *task;
    struct player_look_motion *look;

    /* @hack spelling: the return of FALSE in an else, where an early return
     * would do, which puts it after the stores. the path that made the task
     * branches over it with the last store in the BRA's delay slot
     * (0x8c0e7656); as an early return, 16 of 51 words differ */
    if ((task = task_create(TASK_WORK, 2, amy_look_step)) != NULL) {
        if ((look = heap_alloc(sizeof(*look))) == NULL) {
            task_kill(task);
            return FALSE;
        }
        task->motion = (struct task_motion *)look;
        PLAYER_VARS(task->work)->player = player;
        look->look.head = nodes[0];
        nodes[0]->evalflags &= ~NJD_EVAL_UNIT_ANG;
        look->look.eye0 = nodes[1];
        look->look.eye1 = nodes[2];
    } else {
        return FALSE;
    }
    /* @hack spelling: once the task is made, no return value is set,
     * and R0 keeps 96 from MOV #96,R0 (0x8c0e7652). the one caller reads no
     * result. with return TRUE, SHC loads 1 into R0 and 7 of 51 words
     * differ */
}
/* NOLINTEND(clang-diagnostic-return-type) */

/* give her head and eyes back the angles they had before the look's first
 * step */
static void amy_look_destroy(struct task *task)
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

/* turn her head and eyes toward where her event says, or else toward the
 * camera while Z is held. otherwise her head alone turns the way she's
 * turning. the head turns as far as its limits allow, and the eyes turn the
 * rest of the way. the first step saves the nodes' angles and sets the
 * limits. the task ends once its player has no task */
static void amy_look_step(struct task *task)
{
    struct task_work *work = task->work;
    struct player_look_motion *look = PLAYER_LOOK(task);
    struct task_work *owner = player_work[PLAYER_VARS(work)->player];
    struct task_motion *owner_motion = player_motion[PLAYER_VARS(work)->player];
    struct event_work *event;
    struct task_work *target;
    struct collision_shape *shape;
    struct task_work *camera;
    Angle angles[3];
    NJS_POINT3 direction;
    NJS_VECTOR facing;

    /* @hack spelling: an empty arm for LOOK_INIT, whose jump loads owner->event
     * into R3 (MOV.L @(60,R12),R3 at 0x8c0e7736); with the tests below
     * nested under a test of LOOK_INIT, R2, and 2 of 395 words differ */
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
        look->look.eye0_min_y = -0x800;
        look->look.eye1_max_y = 0x800;
        look->look.eye1_min_y = -0x200;
        look->look.eye0_max_z = 0x800;
        look->look.eye0_min_z = -0xd00;
        look->look.eyes_max_z = 0x800;
        look->look.eyes_min_z = -0xd00;
        task->destructor = amy_look_destroy;
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
        /* @hack spelling: facing.y and facing.z in one chained store, which
         * loads 0.0F into FR5 and 1.0F into FR4 ahead of the switch and keeps
         * player_look_turn_head's address in R8 (0x8c0e77a6 to 0x8c0e77ac);
         * as two stores, the constants trade registers, R8 takes
         * vector_get_angles's address and 193 of 395 words differ */
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

/* the first step of her mouth's task. it gives the task a player_mouth
 * record of her mouth's nodes and shapes, with player_mouth_step as its step
 * and player_mouth_destroy as its destructor. without heap for the record,
 * it runs again next step */
static void amy_mouth_start(struct task *task)
{
    struct player_mouth *mouth;

    if ((mouth = heap_calloc(1, sizeof(*mouth))) != NULL) {
        task->motion = (struct task_motion *)mouth;
        mouth->face = &amy_mouth;
        mouth->mouth = amy_head;
        mouth->blend = &amy_head_blend;
        mouth->shapes = &amy_mouth_shapes;
        task->destructor = player_mouth_destroy;
        task->step = player_mouth_step;
    }
}

Bool amy_carries_drum(int player)
{
    struct task_work *work = player_work[player];
    struct player *record;

    if (work == NULL || PLAYER_VARS(work)->character != CHARACTER_AMY) {
        return FALSE;
    }
    record = player_get(player);
    return HAS_DRUM(record);
}

int amy_get_action(int player)
{
    struct task_work *work;
    struct player *record;

    /* @bug logic: the dizzy test reads player 0's record whichever player is
     * asked (JSR to player_get at 0x8c0e7bfa, MOV #0,R4 in its slot). AMY_DIZZY
     * comes back when player 0 is dizzy, not when the player asked is. every
     * caller asks for player 0, so no call shows it */
    if ((work = player_work[player]) != NULL &&
        PLAYER_VARS(work)->character == CHARACTER_AMY &&
        (record = player_get(0)) != NULL) {
        switch (record->anim.action) {
        case ACTION_DIZZY:
        case ACTION_DIZZY_WALK:
            return AMY_DIZZY;
        }
        switch (work->mode) {
        case AMY_HURT:
            return AMY_RECOILING;
        case AMY_TAKE_DRUM:
            return AMY_TAKING_DRUM;
        case AMY_SET_DRUM:
            return AMY_SETTING_DRUM;
        case AMY_HANDLE:
            return AMY_HOLDING_HANDLE;
        case AMY_HAMMER_SPIN:
            return AMY_SPINNING;
        case AMY_BALANCE:
            return AMY_BALANCING;
        case AMY_HAMMER_JUMP:
            return AMY_HAMMER_JUMPING;
        case AMY_AIR_HAMMER:
            return AMY_AIR_HAMMERING;
        }
    }
    return 0;
}

/* once player 0 comes within the trigger's radius of its work->pos, make
 * Zero leave and end. the trigger also ends once player 0 has no task */
static void amy_zero_leave_step(struct task *task)
{
    struct task_work *player = player_work[0];
    struct task_work *work;

    if (player == NULL) {
        task_kill(task);
        return;
    }
    work = task->work;
    if (njDistanceP2P(&player->pos, &work->pos) < TRIGGER_VARS(work)->radius) {
        zero_leave();
        task_kill(task);
    }
}

/* once player 0 comes within the trigger's radius of its work->pos, bring
 * Zero in toward her from the trigger's target, kept in work->scl, and end.
 * the trigger also ends once player 0 has no task */
static void amy_zero_approach_step(struct task *task)
{
    struct task_work *player = player_work[0];
    struct task_work *work;
    struct zero_start start;

    if (player == NULL) {
        task_kill(task);
        return;
    }
    work = task->work;
    if (!(njDistanceP2P(&player->pos, &work->pos) <
          TRIGGER_VARS(work)->radius)) {
        return;
    }
    /* @hack spelling: a switch of one case, which puts start.pos's address in
     * R2 and start.target's in R3 for the copies (0x8c0e7d88 to 0x8c0e7d92); as
     * an if, the two trade and 12 of 55 words differ */
    switch (TASK_TIMER(work)) {
    case TRIGGER_APPROACH:
        start.how = ZERO_LEAP;
        start.pos = work->scl;
        start.target = player->pos;
        break;
    }
    zero_appear(&start);
    task_kill(task);
}

static void amy_create_zero_triggers(void)
{
    Uint16 stage_act = stage_get_stage_act();
    const struct zero_trigger *trigger;
    struct task *task;
    struct zero_start start;

    for (trigger = amy_zero_triggers; trigger->stage_act != TRIGGERS_END;
         trigger++) {
        if (trigger->stage_act != stage_act) {
            continue;
        }
        switch (trigger->kind) {
        case TRIGGER_LEAVE:
            if ((task = task_create(TASK_WORK, 3, amy_zero_leave_step)) !=
                NULL) {
                task->work->pos = trigger->pos;
                TRIGGER_VARS(task->work)->radius = trigger->radius;
            }
            break;
        case TRIGGER_STAND:
            start.how = ZERO_STAND;
            start.pos = trigger->target;
            zero_appear(&start);
            break;
        case TRIGGER_APPROACH:
            if ((task = task_create(TASK_WORK, 3, amy_zero_approach_step)) !=
                NULL) {
                TASK_TIMER(task->work) = trigger->kind;
                task->work->pos = trigger->pos;
                TRIGGER_VARS(task->work)->radius = trigger->radius;
                task->work->scl = trigger->target;
            }
            break;
        }
    }
}

/* call player 0 onto the spot in Hot Shelter's first act where Amy keeps
 * her balance, and hold her there while she balances. the task ends in any
 * other act, or once player 0 has no task or record */
static void amy_balance_spot_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    NJS_POINT3 spot = {-36.0F, 16.0F, -205.0F};
    struct player *record;

    /* @hack redundant: a first call of player_get, whose result the test
     * below replaces unread (JSR at 0x8c0e7f1c); without it, 30 of 104 words
     * differ */
    record = player_get(0);

    if (player == NULL || (record = player_get(0)) == NULL ||
        stage_get_stage_act() != STAGE_ACT(STAGE_SHELTER, 0)) {
        task_kill(task);
        return;
    }
    if (work->timer == 0) {
        if (_builtin_fabsf(player->pos.y - spot.y) < SPOT_HEIGHT &&
            njDistanceP2P(&player->pos, &spot) < SPOT_REACH) {
            CALLS(record) |= CALL_BALANCE;
        }
    } else {
        work->timer--;
    }
    if (amy_get_action(0) == AMY_BALANCING) {
        work->timer = SPOT_WAIT;
        player->pos = spot;
        record->speed.x = 0.0F;
        record->speed.y = 0.0F;
        record->speed.z = 0.0F;
    }
}

static void amy_create_balance_spot(void)
{
    if (stage_get_stage_act() == STAGE_ACT(STAGE_SHELTER, 0)) {
        task_create(TASK_WORK, 3, amy_balance_spot_step);
    }
}

/* draw the drum DRUM_HEIGHT over her feet, turned as she is. as she takes
 * it or sets it down, the drum plays its own action. as she carries it, the
 * action of her holding it up plays instead of her own, at her frame wrapped
 * to 32. TRUE when it plays instead of hers */
static Bool amy_draw_drum(struct task_work *work, struct task_motion *motion,
                          struct player *record)
{
    NJS_ACTION *action;
    Bool carried = FALSE;
    Float frame;
    NJS_POINT3 pos;
    NJS_VECTOR offset;

    if (work->mode == AMY_TAKE_DRUM) {
        action = A_MOT(0x0cc93b9c);
        if (record->anim.action == ACTION_RAISE_DRUM ||
            record->anim.action == ACTION_SET_DRUM) {
            frame = record->anim.frame;
        } else if (record->anim.action == ACTION_TAKE_DRUM) {
            frame = 0.0F;
        } else {
            frame =
                (Float)amy_actions[ACTION_RAISE_DRUM].action->motion->nbFrame -
                1.0F;
        }
    } else if (work->mode == AMY_SET_DRUM) {
        action = A_MOT(0x0cc93b9c);
        frame = DRUM_FRAME(record);
    } else if (HAS_DRUM(record) == TRUE) {
        frame = record->anim.frame;
        while (frame >= 32.0F) {
            frame -= 32.0F;
        }
        action = A_MOT(0x0cc93698);
        carried = TRUE;
    } else {
        return FALSE;
    }
    njPushMatrix(NULL);
    offset.x = offset.z = 0.0F;
    offset.y = DRUM_HEIGHT;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, -work->ang.y);
    njCalcVector(NULL, &offset, &pos);
    njPopMatrix(1);
    pos.x += work->pos.x;
    pos.y += work->pos.y;
    pos.z += work->pos.z;
    njTranslateV(NULL, &pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, 0x8000 - work->ang.y);
    draw_action(action, frame);
    njPopMatrix(1);
    return carried;
}

Bool amy_set_hammer_scale(Uint8 player, Float scale)
{
    struct player *record;

    if ((record = player_get(player)) == NULL) {
        return FALSE;
    }
    if (scale <= 0.0F) {
        HAMMER_AIM_HELD(record) = FALSE;
    } else {
        HAMMER_AIM_HELD(record) = TRUE;
        HAMMER_AIM(record) = scale;
        HAMMER_SCALE(record) = scale;
    }
    return TRUE;
}

Bool amy_grow_hammer(Uint8 player, Float scale)
{
    struct player *record;

    if ((record = player_get(player)) == NULL) {
        return FALSE;
    }
    if (scale <= 0.0F) {
        HAMMER_AIM_HELD(record) = FALSE;
    } else {
        HAMMER_AIM_HELD(record) = TRUE;
        HAMMER_AIM(record) = scale;
    }
    return TRUE;
}

Bool amy_call_to_handle(void)
{
    struct player *record;

    if ((record = player_get(0)) == NULL) {
        return FALSE;
    }
    CALLS(record) |= CALL_HANDLE;
    return TRUE;
}

Bool amy_call_hidden(void)
{
    struct player *record;

    if ((record = player_get(0)) == NULL) {
        return FALSE;
    }
    CALLS(record) |= CALL_HIDDEN;
    return TRUE;
}

/* draw an afterimage of her swing: the action and frame it copied, with
 * her hammer at the scale it had. it draws on its second and third steps,
 * fainter on the third, and only on a frame's first step. it ends on its
 * fourth */
static void amy_afterimage_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *hammer;
    NJS_OBJECT *head;
    /* @hack spelling: the alpha through this local, which puts its FADD ahead
     * of draw_set_material's JSR (0x8c0e84e0); as the argument, the FADD
     * fills the JSR's delay slot and 7 of 260 words differ */
    Float alpha;
    NJS_VECTOR offset;
    NJS_VECTOR middle;

    if (++work->tick >= 4) {
        task_kill(task);
        return;
    }
    if (work->tick < 2) {
        return;
    }
    if (frame_step) {
        return;
    }
    hammer = amy_thumb_minus_z->sibling;
    /* @hack spelling: head, the hammer's head, read through her thumb again,
     * not as hammer->child, which keeps hammer in R1 (MOV.L @(48,R1),R1 at
     * 0x8c0e8404); as hammer->child, hammer takes R4 and 35 of 260 words
     * differ */
    head = amy_thumb_minus_z->sibling->child;
    hammer->scl[0] = AFTERIMAGE_VARS(work)->hammer_scale;
    hammer->scl[1] = AFTERIMAGE_VARS(work)->hammer_scale;
    hammer->scl[2] = AFTERIMAGE_VARS(work)->hammer_scale;
    head->scl[0] = AFTERIMAGE_VARS(work)->hammer_scale;
    head->scl[1] = AFTERIMAGE_VARS(work)->hammer_scale;
    head->scl[2] = AFTERIMAGE_VARS(work)->hammer_scale;
    if (AFTERIMAGE_VARS(work)->hammer_scale == 0.0F) {
        hammer->evalflags |= NJD_EVAL_HIDE;
        head->evalflags |= NJD_EVAL_HIDE;
    } else {
        hammer->evalflags &= ~NJD_EVAL_HIDE;
        head->evalflags &= ~NJD_EVAL_HIDE;
        if (AFTERIMAGE_VARS(work)->hammer_scale == 1.0F) {
            hammer->evalflags |= NJD_EVAL_UNIT_SCL;
            head->evalflags |= NJD_EVAL_UNIT_SCL;
        } else {
            hammer->evalflags &= ~NJD_EVAL_UNIT_SCL;
            head->evalflags &= ~NJD_EVAL_UNIT_SCL;
        }
    }
    draw_set_palette(DRAW_PLAYER_PALETTE);
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    alpha = -(work->tick * 0.1F) - 0.5F;
    draw_set_material(alpha, 0.0F, 0.0F, 0.0F);
    njSetTexture(&amy_texlist);
    njPushMatrix(NULL);
    offset.z = 0.0F;
    offset.x = 0.0F;
    offset.y = AFTERIMAGE_VARS(work)->height * 0.5F;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, -work->ang.y);
    njCalcVector(NULL, &offset, &middle);
    njPopMatrix(1);
    middle.x += work->pos.x;
    middle.y += work->pos.y;
    middle.z += work->pos.z;
    njTranslateV(NULL, &middle);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, 0x8000 - work->ang.y);
    draw_action(amy_actions[TASK_TIMER(work)].action,
                AFTERIMAGE_VARS(work)->frame);
    njPopMatrix(1);
    draw_attr_restore();
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    draw_set_palette(0);
}

static void amy_afterimage_create(struct task_work *work, struct player *record)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 6, amy_afterimage_step)) != NULL) {
        task->work->pos = work->pos;
        task->work->ang = work->ang;
        TASK_TIMER(task->work) = record->anim.action;
        AFTERIMAGE_VARS(task->work)->frame = record->anim.frame;
        AFTERIMAGE_VARS(task->work)->height = record->consts.height;
        AFTERIMAGE_VARS(task->work)->hammer_scale = HAMMER_SCALE(record);
    }
}
