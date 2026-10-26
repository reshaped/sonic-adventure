/* the robo, a flying enemy that sends its grabber after player 0, then
 * fetches it. the grabber catches a player it touches. final's and
 * shelter's object lists name the robo E ROBO */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "animal/kinds.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/polygon.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "effect/explosion.h"
#include "effect/panlight.h"
#include "enemy/enemy.h"
#include "enemy/robo.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 5
#define TEXT_4_LENGTH 6
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 13
#define TEXT_10_LENGTH 9
#define TEXT_11_LENGTH 11
#define TEXT_12_LENGTH 6
#define TEXT_13_LENGTH 6
#define TEXT_14_LENGTH 8
#define TEXT_15_LENGTH 7
#define TEXT_16_LENGTH 6
#define TEXT_17_LENGTH 9
#define TEXT_18_LENGTH 7
#define TEXT_19_LENGTH 7
#define TEXT_20_LENGTH 7
#define TEXT_21_LENGTH 6
#define TEXT_22_LENGTH 6
#include "enemy/robo/text.inc"

/* the robo's work->mode */
enum robo_mode {
    ROBO_HOVER, /* it sways in place until robo_notices finds player 0 */
    ROBO_RISE, /* it levels out and climbs, then cruises */
    /* it flies where robo_search steers it, until robo_search sees
     * player 0. it lets the grabber go as it leaves */
    ROBO_CRUISE,
    /* it hangs facing player 0 while the grabber chases, until the grabber
     * holds */
    ROBO_WATCH,
    ROBO_AIM, /* it turns to the grabber, pitches down and sinks */
    /* it flies at the grabber until it is within SWOOP_RANGE of it in x and
     * z */
    ROBO_SWOOP,
    ROBO_BRAKE, /* it pitches up and slows, then turns about */
    /* it levels out until the grabber is back on it, then hovers or
     * cruises */
    ROBO_CLIMB,
    ROBO_SWING, /* it swings its arms at player 0, who came near */
    ROBO_FALL, /* robo.c never enters it */
    ROBO_BEATEN /* a hit beat it, and robo_step blows it up */
};

/* the stages of ROBO_SWING, in work->submode. robo_animate moves the robo
 * on from SWING_SPIN_UP and SWING_SPIN_DOWN as their blends end */
enum robo_swing_stage {
    SWING_SPIN_UP, /* the robo spins as its arms blend to SWING_START */
    SWING_OUT, /* the arms' shape is on as they swing out to SWING_END */
    SWING_HOLD, /* the arms stay out until the swing is SWING_STEPS old */
    /* the arms' shape is off, and the robo spins back as its arms blend
     * back to SWING_FROM */
    SWING_SPIN_DOWN,
    SWING_DONE /* the robo goes back to its mode before the swing */
};

/* robo_action's frames for the swing: the arms blend to SWING_START, then
 * swing out to SWING_END */
#define SWING_START 23.0F
#define SWING_END 29.0F
/* the arms stay out until the swing's timer passes SWING_STEPS */
#define SWING_STEPS 20

/* the robo's bits in work->flags */
/* the grabber is on the robo, which draws its copy. robo_cruise clears it
 * as it lets the grabber go, and grabber_settle sets it as the grabber is
 * back */
#define ROBO_CARRYING 0x100
/* the grabber holds, waiting for the robo to fetch it. grabber_chase sets
 * it, and robo_climb clears it once the grabber is back */
#define ROBO_FETCH 0x200
/* robo_step registers the robo for collision only while this is set.
 * robo_init sets it, and nothing clears it */
#define ROBO_SOLID 0x400
#define ROBO_SWINGING 0x800 /* the robo is in ROBO_SWING */
/* robo_hover lowers the pitch to HOVER_PITCH while this is set, then
 * clears it. robo_climb sets it as the robo goes back to hovering */
#define ROBO_LEANING 0x1000
/* robo_animate blends the pose from frame toward next_frame, and moves
 * work->submode on as the blend ends */
#define ROBO_BLENDING 0x2000

/* the grabber's work->mode */
enum grabber_mode {
    /* it waits while the robo draws a copy of it, until the robo enters
     * ROBO_WATCH */
    GRABBER_WAIT,
    /* it sprays as it starts to rise, then climbs, dripping, to
     * GRABBER_HEIGHT above the ground */
    GRABBER_RISE,
    /* it chases player 0, its beam on, until it catches a player or the
     * chase times out */
    GRABBER_CHASE,
    /* it spins as it slows to a stop, its beam on, until the robo enters
     * ROBO_CLIMB */
    GRABBER_HOLD,
    GRABBER_RETURN, /* it flies back toward the robo */
    GRABBER_SETTLE, /* it eases onto its spot on the robo */
    GRABBER_IDLE /* robo.c never enters it */
};

/* the grabber's bits in work->flags. grabber_init sets GRABBER_UNK_400,
 * and grabber_chase sets GRABBER_CAUGHT as the grabber catches a player.
 * robo.c reads neither */
#define GRABBER_UNK_400 0x400
#define GRABBER_CAUGHT 0x8000

/* the height above the ground the grabber climbs to and sinks back to */
#define GRABBER_HEIGHT 30.0F
/* a chase that catches no one ends once its timer passes CHASE_STEPS */
#define CHASE_STEPS 300
/* the distance from its spot on the robo within which the grabber settles */
#define SETTLE_RANGE 0.5F
/* grabber_return measures in x and z. within EASE_RANGE of the robo, the
 * grabber starts GRABBER_SETTLE. within SLOW_RANGE, it pushes at a fifth of
 * GRABBER_THRUST, and beyond, at two fifths */
#define EASE_RANGE 10.0F
#define SLOW_RANGE 20.0F

/* a beam piece's work->submode. its first step sets its draw, and moves it
 * on to BEAM_RUN */
enum beam_mode { BEAM_START, BEAM_RUN };
/* a beam piece keeps its size and its alpha in work->scl */
#define BEAM_SIZE(work) ((work)->scl.x)
#define BEAM_ALPHA(work) ((work)->scl.z)

/* the sounds the robo's and the grabber's steps play on each step of the
 * mode of the same name */
#define SE_HOVER 827
#define SE_RISE 828 /* also once as the grabber settles back on */
#define SE_GRABBER_HOLD 829
#define SE_GRABBER_CHASE 830
#define SE_SWOOP 831

/* the robo's angles in record->angles */
enum robo_angle {
    ROBO_BOB, /* the phase of BOB's swing */
    /* the pitch robo_animate draws the robo at. robo_hover, robo_aim,
     * robo_brake and robo_climb clamp it by (Uint16)(pitch + 0x8000), which
     * orders its low 16 bits as a signed angle */
    ROBO_PITCH,
    /* how far robo_turn turns the heading toward ROBO_HEAD in a step */
    ROBO_TURN_SPEED,
    /* the phase of the hover's sway, which rocks the pitch 8 degrees each
     * way */
    ROBO_SWAY,
    ROBO_HEAD, /* the head's heading */
    /* how far robo_turn turns ROBO_HEAD toward aim_y in a step */
    ROBO_HEAD_SPEED,
    /* the phase of the arms' beat, which rocks the action's frame between
     * 7 and 9 */
    ROBO_BEAT
};

/* the hovering robo's pitch, about which its sway rocks it */
#define HOVER_PITCH (-0x1a00)
/* robo_aim pitches the robo down to AIM_PITCH, which is -0x2800 in its low
 * 16 bits, and robo_brake pitches it up to BRAKE_PITCH */
#define AIM_PITCH 0xd800
#define BRAKE_PITCH 0x2000

/* the record's spare fields, under the names the robo and the grabber give
 * them. each task has a record of its own, so BOB and RISEN share a field */
/* the robo's push ahead as it swoops, and a fiftieth of it as it cruises */
#define THRUST(record) ((record)->motion.gravity)
/* the grabber's push ahead as it chases, and a fraction of it as it
 * returns */
#define GRABBER_THRUST(record) ((record)->unk_74)
/* the robo's lift, which weakens with its height above the ground */
#define LIFT(record) ((record)->unk_7c)
/* the robo's height above pos as drawn */
#define BOB(record) ((record)->unk_78)
/* the grabber's height where its rise ended */
#define RISEN(record) ((record)->unk_78)
/* the robo's spin about its up axis, which turns its body and arms but
 * not its head or its copy of the grabber */
#define SPIN(record) ((record)->phase)
/* the frame a swing starts from, which the arms blend back to */
#define SWING_FROM(record) ((record)->unk_1e4)

/* the range placement_try_leave_by takes, squared. the robo stays while
 * the camera or player 0 is within 700 of it, or player 1 is Sonic and
 * within 40 */
#define ROBO_LEAVE_RANGE (700.0F * 700.0F)

/* robo_search sees player 0 within SIGHT_RANGE of the robo, less than
 * SIGHT_HEIGHT above or below it, and less than SIGHT_ANGLE off the head's
 * heading */
#define SIGHT_RANGE 200.0F
#define SIGHT_HEIGHT 40.0F
#define SIGHT_ANGLE NJM_DEG_ANG(15)
/* robo_notices and robo_swing_when_near test player 0's distance against these
 */
#define NOTICE_RANGE 150.0F
#define SWING_RANGE 60.0F
/* robo_swoop brakes once the grabber is within SWOOP_RANGE in x and z */
#define SWOOP_RANGE 18.0F

/* the turn that takes +z to +x. a heading of 0 faces +x, but the models
 * face +z, and atan2f(x, z) gives an angle from +z */
#define Z_TURN NJM_DEG_ANG(90)

/* the robo's bytes in work->vars */
struct robo_vars {
    Uint8 unk_00[4]; /* ENEMY_MARK_BEATEN writes bytes 1 and 2 */
    /* robo_init's heap copy of robo_grabber_model, which the robo draws
     * while ROBO_CARRYING is set */
    NJS_MODEL *grabber;
};

#define VARS(work) ((struct robo_vars *)&(work)->vars)

static void grabber_init(struct task *task);
static void grabber_render(struct task_work *work,
                           struct lockon_target *record);
static void grabber_draw(struct task *task);
static void grabber_destroy(struct task *task);
static void grabber_step(struct task *task);
static void robo_draw(struct task *task);
static void robo_draw_edit(struct task_work *work);
static void robo_destroy(struct task *task);
static void robo_step(struct task *task);

static NJS_TEXNAME e_robo_textures[22] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0},
};
NJS_TEXLIST e_robo_texlist = {
#include "enemy/robo/e_robo_texlist.inc"
};

static NJS_MATERIAL robo_hand_minus_x_materials[2] = {
#include "enemy/robo/robo_hand_minus_x_materials.inc"
};

static Sint16 robo_hand_minus_x_meshes0[11] = {
#include "enemy/robo/robo_hand_minus_x_meshes0.inc"
};

static Sint16 robo_hand_minus_x_meshes1[15] = {
#include "enemy/robo/robo_hand_minus_x_meshes1.inc"
};

static NJS_COLOR robo_hand_minus_x_uvs0[9] = {
#include "enemy/robo/robo_hand_minus_x_uvs0.inc"
};

static NJS_COLOR robo_hand_minus_x_uvs1[13] = {
#include "enemy/robo/robo_hand_minus_x_uvs1.inc"
};

static NJS_MESHSET robo_hand_minus_x_meshsets[2] = {
#include "enemy/robo/robo_hand_minus_x_meshsets.inc"
};

static NJS_POINT3 robo_hand_minus_x_points[9] = {
#include "enemy/robo/robo_hand_minus_x_points.inc"
};

static NJS_VECTOR robo_hand_minus_x_normals[9] = {
#include "enemy/robo/robo_hand_minus_x_normals.inc"
};

static NJS_MODEL robo_hand_minus_x_model = {
#include "enemy/robo/robo_hand_minus_x_model.inc"
};

static NJS_OBJECT robo_hand_minus_x = {
#include "enemy/robo/robo_hand_minus_x.inc"
};

static NJS_MATERIAL robo_forearm_minus_x_materials[4] = {
#include "enemy/robo/robo_forearm_minus_x_materials.inc"
};

static Sint16 robo_forearm_minus_x_meshes0[7] = {
#include "enemy/robo/robo_forearm_minus_x_meshes0.inc"
};

static Sint16 robo_forearm_minus_x_meshes1[7] = {
#include "enemy/robo/robo_forearm_minus_x_meshes1.inc"
};

static Sint16 robo_forearm_minus_x_meshes2[14] = {
#include "enemy/robo/robo_forearm_minus_x_meshes2.inc"
};

static Sint16 robo_forearm_minus_x_meshes3[22] = {
#include "enemy/robo/robo_forearm_minus_x_meshes3.inc"
};

static NJS_COLOR robo_forearm_minus_x_uvs0[6] = {
#include "enemy/robo/robo_forearm_minus_x_uvs0.inc"
};

static NJS_COLOR robo_forearm_minus_x_uvs1[6] = {
#include "enemy/robo/robo_forearm_minus_x_uvs1.inc"
};

static NJS_COLOR robo_forearm_minus_x_uvs2[12] = {
#include "enemy/robo/robo_forearm_minus_x_uvs2.inc"
};

static NJS_COLOR robo_forearm_minus_x_uvs3[20] = {
#include "enemy/robo/robo_forearm_minus_x_uvs3.inc"
};

static NJS_MESHSET robo_forearm_minus_x_meshsets[4] = {
#include "enemy/robo/robo_forearm_minus_x_meshsets.inc"
};

static NJS_POINT3 robo_forearm_minus_x_points[26] = {
#include "enemy/robo/robo_forearm_minus_x_points.inc"
};

static NJS_VECTOR robo_forearm_minus_x_normals[26] = {
#include "enemy/robo/robo_forearm_minus_x_normals.inc"
};

static NJS_MODEL robo_forearm_minus_x_model = {
#include "enemy/robo/robo_forearm_minus_x_model.inc"
};

static NJS_OBJECT robo_forearm_minus_x = {
#include "enemy/robo/robo_forearm_minus_x.inc"
};

static NJS_MATERIAL robo_upper_arm_minus_x_materials[1] = {
#include "enemy/robo/robo_upper_arm_minus_x_materials.inc"
};

static Sint16 robo_upper_arm_minus_x_meshes0[11] = {
#include "enemy/robo/robo_upper_arm_minus_x_meshes0.inc"
};

static NJS_COLOR robo_upper_arm_minus_x_uvs0[10] = {
#include "enemy/robo/robo_upper_arm_minus_x_uvs0.inc"
};

static NJS_MESHSET robo_upper_arm_minus_x_meshsets[1] = {
#include "enemy/robo/robo_upper_arm_minus_x_meshsets.inc"
};

static NJS_POINT3 robo_upper_arm_minus_x_points[8] = {
#include "enemy/robo/robo_upper_arm_minus_x_points.inc"
};

static NJS_VECTOR robo_upper_arm_minus_x_normals[8] = {
#include "enemy/robo/robo_upper_arm_minus_x_normals.inc"
};

static NJS_MODEL robo_upper_arm_minus_x_model = {
#include "enemy/robo/robo_upper_arm_minus_x_model.inc"
};

static NJS_OBJECT robo_upper_arm_minus_x = {
#include "enemy/robo/robo_upper_arm_minus_x.inc"
};

static NJS_MATERIAL robo_shoulder_minus_x_materials[2] = {
#include "enemy/robo/robo_shoulder_minus_x_materials.inc"
};

static Sint16 robo_shoulder_minus_x_meshes0[25] = {
#include "enemy/robo/robo_shoulder_minus_x_meshes0.inc"
};

static Sint16 robo_shoulder_minus_x_meshes1[15] = {
#include "enemy/robo/robo_shoulder_minus_x_meshes1.inc"
};

static NJS_COLOR robo_shoulder_minus_x_uvs0[22] = {
#include "enemy/robo/robo_shoulder_minus_x_uvs0.inc"
};

static NJS_COLOR robo_shoulder_minus_x_uvs1[14] = {
#include "enemy/robo/robo_shoulder_minus_x_uvs1.inc"
};

static NJS_MESHSET robo_shoulder_minus_x_meshsets[2] = {
#include "enemy/robo/robo_shoulder_minus_x_meshsets.inc"
};

static NJS_POINT3 robo_shoulder_minus_x_points[18] = {
#include "enemy/robo/robo_shoulder_minus_x_points.inc"
};

static NJS_VECTOR robo_shoulder_minus_x_normals[18] = {
#include "enemy/robo/robo_shoulder_minus_x_normals.inc"
};

static NJS_MODEL robo_shoulder_minus_x_model = {
#include "enemy/robo/robo_shoulder_minus_x_model.inc"
};

static NJS_OBJECT robo_shoulder_minus_x = {
#include "enemy/robo/robo_shoulder_minus_x.inc"
};

static NJS_MATERIAL robo_light_materials[2] = {
#include "enemy/robo/robo_light_materials.inc"
};

static Sint16 robo_light_meshes0[12] = {
#include "enemy/robo/robo_light_meshes0.inc"
};

static Sint16 robo_light_meshes1[15] = {
#include "enemy/robo/robo_light_meshes1.inc"
};

static NJS_MESHSET robo_light_meshsets[2] = {
#include "enemy/robo/robo_light_meshsets.inc"
};

static NJS_POINT3 robo_light_points[13] = {
#include "enemy/robo/robo_light_points.inc"
};

static NJS_VECTOR robo_light_normals[13] = {
#include "enemy/robo/robo_light_normals.inc"
};

static NJS_MODEL robo_light_model = {
#include "enemy/robo/robo_light_model.inc"
};

static NJS_OBJECT robo_light = {
#include "enemy/robo/robo_light.inc"
};

static NJS_MATERIAL robo_grabber_materials[5] = {
#include "enemy/robo/robo_grabber_materials.inc"
};

static Sint16 robo_grabber_meshes0[95] = {
#include "enemy/robo/robo_grabber_meshes0.inc"
};

static Sint16 robo_grabber_meshes1[37] = {
#include "enemy/robo/robo_grabber_meshes1.inc"
};

static Sint16 robo_grabber_meshes2[5] = {
#include "enemy/robo/robo_grabber_meshes2.inc"
};

static Sint16 robo_grabber_meshes3[7] = {
#include "enemy/robo/robo_grabber_meshes3.inc"
};

static Sint16 robo_grabber_meshes4[11] = {
#include "enemy/robo/robo_grabber_meshes4.inc"
};

static NJS_COLOR robo_grabber_uvs0[84] = {
#include "enemy/robo/robo_grabber_uvs0.inc"
};

static NJS_COLOR robo_grabber_uvs1[32] = {
#include "enemy/robo/robo_grabber_uvs1.inc"
};

static NJS_COLOR robo_grabber_uvs3[6] = {
#include "enemy/robo/robo_grabber_uvs3.inc"
};

static NJS_COLOR robo_grabber_uvs4[10] = {
#include "enemy/robo/robo_grabber_uvs4.inc"
};

static NJS_MESHSET robo_grabber_meshsets[5] = {
#include "enemy/robo/robo_grabber_meshsets.inc"
};

static NJS_POINT3 robo_grabber_points[56] = {
#include "enemy/robo/robo_grabber_points.inc"
};

static NJS_VECTOR robo_grabber_normals[56] = {
#include "enemy/robo/robo_grabber_normals.inc"
};

/* @hack type: an array of one, so that robo_init reads mats 16 bytes past
 * the model's address, from the pool's one entry for it (MOV.L @(16,R5),R5
 * at 0x8c106e3c). as a plain model, SHC pools the address of mats as well.
 * summed over robo_init and grabber_render, which name it, 8 of 340 words
 * then differ */
static NJS_MODEL robo_grabber_model[1] = {
#include "enemy/robo/robo_grabber_model.inc"
};

static NJS_OBJECT robo_grabber = {
#include "enemy/robo/robo_grabber.inc"
};

static NJS_MATERIAL robo_head_materials[4] = {
#include "enemy/robo/robo_head_materials.inc"
};

static Sint16 robo_head_meshes0[25] = {
#include "enemy/robo/robo_head_meshes0.inc"
};

static Sint16 robo_head_meshes1[6] = {
#include "enemy/robo/robo_head_meshes1.inc"
};

static Sint16 robo_head_meshes2[12] = {
#include "enemy/robo/robo_head_meshes2.inc"
};

static Sint16 robo_head_meshes3[6] = {
#include "enemy/robo/robo_head_meshes3.inc"
};

static NJS_COLOR robo_head_uvs0[22] = {
#include "enemy/robo/robo_head_uvs0.inc"
};

static NJS_COLOR robo_head_uvs2[10] = {
#include "enemy/robo/robo_head_uvs2.inc"
};

static NJS_MESHSET robo_head_meshsets[4] = {
#include "enemy/robo/robo_head_meshsets.inc"
};

static NJS_POINT3 robo_head_points[24] = {
#include "enemy/robo/robo_head_points.inc"
};

static NJS_VECTOR robo_head_normals[24] = {
#include "enemy/robo/robo_head_normals.inc"
};

static NJS_MODEL robo_head_model = {
#include "enemy/robo/robo_head_model.inc"
};

static NJS_OBJECT robo_head = {
#include "enemy/robo/robo_head.inc"
};

static NJS_MATERIAL robo_hand_plus_x_materials[2] = {
#include "enemy/robo/robo_hand_plus_x_materials.inc"
};

static Sint16 robo_hand_plus_x_meshes0[11] = {
#include "enemy/robo/robo_hand_plus_x_meshes0.inc"
};

static Sint16 robo_hand_plus_x_meshes1[15] = {
#include "enemy/robo/robo_hand_plus_x_meshes1.inc"
};

static NJS_COLOR robo_hand_plus_x_uvs0[9] = {
#include "enemy/robo/robo_hand_plus_x_uvs0.inc"
};

static NJS_COLOR robo_hand_plus_x_uvs1[13] = {
#include "enemy/robo/robo_hand_plus_x_uvs1.inc"
};

static NJS_MESHSET robo_hand_plus_x_meshsets[2] = {
#include "enemy/robo/robo_hand_plus_x_meshsets.inc"
};

static NJS_POINT3 robo_hand_plus_x_points[9] = {
#include "enemy/robo/robo_hand_plus_x_points.inc"
};

static NJS_VECTOR robo_hand_plus_x_normals[9] = {
#include "enemy/robo/robo_hand_plus_x_normals.inc"
};

static NJS_MODEL robo_hand_plus_x_model = {
#include "enemy/robo/robo_hand_plus_x_model.inc"
};

static NJS_OBJECT robo_hand_plus_x = {
#include "enemy/robo/robo_hand_plus_x.inc"
};

static NJS_MATERIAL robo_forearm_plus_x_materials[4] = {
#include "enemy/robo/robo_forearm_plus_x_materials.inc"
};

static Sint16 robo_forearm_plus_x_meshes0[7] = {
#include "enemy/robo/robo_forearm_plus_x_meshes0.inc"
};

static Sint16 robo_forearm_plus_x_meshes1[7] = {
#include "enemy/robo/robo_forearm_plus_x_meshes1.inc"
};

static Sint16 robo_forearm_plus_x_meshes2[14] = {
#include "enemy/robo/robo_forearm_plus_x_meshes2.inc"
};

static Sint16 robo_forearm_plus_x_meshes3[22] = {
#include "enemy/robo/robo_forearm_plus_x_meshes3.inc"
};

static NJS_COLOR robo_forearm_plus_x_uvs0[6] = {
#include "enemy/robo/robo_forearm_plus_x_uvs0.inc"
};

static NJS_COLOR robo_forearm_plus_x_uvs1[6] = {
#include "enemy/robo/robo_forearm_plus_x_uvs1.inc"
};

static NJS_COLOR robo_forearm_plus_x_uvs2[12] = {
#include "enemy/robo/robo_forearm_plus_x_uvs2.inc"
};

static NJS_COLOR robo_forearm_plus_x_uvs3[20] = {
#include "enemy/robo/robo_forearm_plus_x_uvs3.inc"
};

static NJS_MESHSET robo_forearm_plus_x_meshsets[4] = {
#include "enemy/robo/robo_forearm_plus_x_meshsets.inc"
};

static NJS_POINT3 robo_forearm_plus_x_points[26] = {
#include "enemy/robo/robo_forearm_plus_x_points.inc"
};

static NJS_VECTOR robo_forearm_plus_x_normals[26] = {
#include "enemy/robo/robo_forearm_plus_x_normals.inc"
};

static NJS_MODEL robo_forearm_plus_x_model = {
#include "enemy/robo/robo_forearm_plus_x_model.inc"
};

static NJS_OBJECT robo_forearm_plus_x = {
#include "enemy/robo/robo_forearm_plus_x.inc"
};

static NJS_MATERIAL robo_upper_arm_plus_x_materials[1] = {
#include "enemy/robo/robo_upper_arm_plus_x_materials.inc"
};

static Sint16 robo_upper_arm_plus_x_meshes0[11] = {
#include "enemy/robo/robo_upper_arm_plus_x_meshes0.inc"
};

static NJS_COLOR robo_upper_arm_plus_x_uvs0[10] = {
#include "enemy/robo/robo_upper_arm_plus_x_uvs0.inc"
};

static NJS_MESHSET robo_upper_arm_plus_x_meshsets[1] = {
#include "enemy/robo/robo_upper_arm_plus_x_meshsets.inc"
};

static NJS_POINT3 robo_upper_arm_plus_x_points[8] = {
#include "enemy/robo/robo_upper_arm_plus_x_points.inc"
};

static NJS_VECTOR robo_upper_arm_plus_x_normals[8] = {
#include "enemy/robo/robo_upper_arm_plus_x_normals.inc"
};

static NJS_MODEL robo_upper_arm_plus_x_model = {
#include "enemy/robo/robo_upper_arm_plus_x_model.inc"
};

static NJS_OBJECT robo_upper_arm_plus_x = {
#include "enemy/robo/robo_upper_arm_plus_x.inc"
};

static NJS_MATERIAL robo_shoulder_plus_x_materials[2] = {
#include "enemy/robo/robo_shoulder_plus_x_materials.inc"
};

static Sint16 robo_shoulder_plus_x_meshes0[25] = {
#include "enemy/robo/robo_shoulder_plus_x_meshes0.inc"
};

static Sint16 robo_shoulder_plus_x_meshes1[15] = {
#include "enemy/robo/robo_shoulder_plus_x_meshes1.inc"
};

static NJS_COLOR robo_shoulder_plus_x_uvs0[22] = {
#include "enemy/robo/robo_shoulder_plus_x_uvs0.inc"
};

static NJS_COLOR robo_shoulder_plus_x_uvs1[14] = {
#include "enemy/robo/robo_shoulder_plus_x_uvs1.inc"
};

static NJS_MESHSET robo_shoulder_plus_x_meshsets[2] = {
#include "enemy/robo/robo_shoulder_plus_x_meshsets.inc"
};

static NJS_POINT3 robo_shoulder_plus_x_points[18] = {
#include "enemy/robo/robo_shoulder_plus_x_points.inc"
};

static NJS_VECTOR robo_shoulder_plus_x_normals[18] = {
#include "enemy/robo/robo_shoulder_plus_x_normals.inc"
};

static NJS_MODEL robo_shoulder_plus_x_model = {
#include "enemy/robo/robo_shoulder_plus_x_model.inc"
};

static NJS_OBJECT robo_shoulder_plus_x = {
#include "enemy/robo/robo_shoulder_plus_x.inc"
};

static NJS_MATERIAL robo_body_materials[10] = {
#include "enemy/robo/robo_body_materials.inc"
};

static Sint16 robo_body_meshes0[22] = {
#include "enemy/robo/robo_body_meshes0.inc"
};

static Sint16 robo_body_meshes1[47] = {
#include "enemy/robo/robo_body_meshes1.inc"
};

static Sint16 robo_body_meshes2[82] = {
#include "enemy/robo/robo_body_meshes2.inc"
};

static Sint16 robo_body_meshes3[16] = {
#include "enemy/robo/robo_body_meshes3.inc"
};

static Sint16 robo_body_meshes4[5] = {
#include "enemy/robo/robo_body_meshes4.inc"
};

static Sint16 robo_body_meshes5[28] = {
#include "enemy/robo/robo_body_meshes5.inc"
};

static Sint16 robo_body_meshes6[7] = {
#include "enemy/robo/robo_body_meshes6.inc"
};

static Sint16 robo_body_meshes7[25] = {
#include "enemy/robo/robo_body_meshes7.inc"
};

static Sint16 robo_body_meshes8[10] = {
#include "enemy/robo/robo_body_meshes8.inc"
};

static Sint16 robo_body_meshes9[16] = {
#include "enemy/robo/robo_body_meshes9.inc"
};

static NJS_COLOR robo_body_uvs0[18] = {
#include "enemy/robo/robo_body_uvs0.inc"
};

static NJS_COLOR robo_body_uvs1[38] = {
#include "enemy/robo/robo_body_uvs1.inc"
};

static NJS_COLOR robo_body_uvs2[74] = {
#include "enemy/robo/robo_body_uvs2.inc"
};

static NJS_COLOR robo_body_uvs3[14] = {
#include "enemy/robo/robo_body_uvs3.inc"
};

static NJS_COLOR robo_body_uvs4[4] = {
#include "enemy/robo/robo_body_uvs4.inc"
};

static NJS_COLOR robo_body_uvs5[24] = {
#include "enemy/robo/robo_body_uvs5.inc"
};

static NJS_COLOR robo_body_uvs6[6] = {
#include "enemy/robo/robo_body_uvs6.inc"
};

static NJS_COLOR robo_body_uvs7[20] = {
#include "enemy/robo/robo_body_uvs7.inc"
};

static NJS_COLOR robo_body_uvs8[8] = {
#include "enemy/robo/robo_body_uvs8.inc"
};

static NJS_COLOR robo_body_uvs9[14] = {
#include "enemy/robo/robo_body_uvs9.inc"
};

static NJS_MESHSET robo_body_meshsets[10] = {
#include "enemy/robo/robo_body_meshsets.inc"
};

static NJS_POINT3 robo_body_points[82] = {
#include "enemy/robo/robo_body_points.inc"
};

static NJS_VECTOR robo_body_normals[82] = {
#include "enemy/robo/robo_body_normals.inc"
};

static NJS_MODEL robo_body_model = {
#include "enemy/robo/robo_body_model.inc"
};

/* the tree hides the head and the grabber. robo_animate and robo_draw add
 * the head with its own turn, and the grabber's copy while ROBO_CARRYING is
 * set */
static NJS_OBJECT robo_object = {
#include "enemy/robo/robo_object.inc"
};

static NJS_MKEY_A robo_body_turns[14] = {
#include "enemy/robo/robo_body_turns.inc"
};

static NJS_MKEY_A robo_upper_arm_plus_x_turns[25] = {
#include "enemy/robo/robo_upper_arm_plus_x_turns.inc"
};

static NJS_MKEY_A robo_forearm_plus_x_turns[25] = {
#include "enemy/robo/robo_forearm_plus_x_turns.inc"
};

static NJS_MKEY_A robo_upper_arm_minus_x_turns[31] = {
#include "enemy/robo/robo_upper_arm_minus_x_turns.inc"
};

static NJS_MKEY_A robo_forearm_minus_x_turns[31] = {
#include "enemy/robo/robo_forearm_minus_x_turns.inc"
};

static NJS_MDATA2 robo_motion_data[12] = {
#include "enemy/robo/robo_motion_data.inc"
};

static NJS_MOTION robo_motion = {
#include "enemy/robo/robo_motion.inc"
};

static NJS_ACTION robo_action = {
#include "enemy/robo/robo_action.inc"
};

/* the robo's body, and its arms' reach, which robo_swing turns on while
 * the arms are out */
/* clang-format off */
static struct collision_shape robo_shapes[2] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_ENEMIES | PUSH_FIRM(PUSH_PLAYERS), {0.0F, -3.0F, -0.0F}, 9.0F, 5.5F,
     0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_OFF | SHAPE_HURTABLE,
     ATTACK(1) | GUARD(2) | SHAPE_HURTS_PLAYERS,
     PUSH_ENEMIES | PUSH_FIRM(PUSH_PLAYERS), {0.0F, -4.0F, 0.0F}, 22.0F, 11.0F,
     0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* the grabber's body, and its beam below it */
/* clang-format off */
static struct collision_shape grabber_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE, GUARD(3) | SHAPE_HURTS_PLAYERS,
     PUSH_FIRM(PUSH_ALL), {0.0F, -11.0F, -0.0F}, 7.0F, 10.0F, 0.0F, {0}, 0,
     NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE, GUARD(3) | SHAPE_HURTS_PLAYERS,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, -28.0F, -0.0F}, 9.0F, 5.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* a third shape, which nothing names */
/* clang-format off */
static struct collision_shape dat_8c5ce9f8[1] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE,
     ATTACK(3) | GUARD(1) | SHAPE_HURTS_PLAYERS,
     PUSH_ENEMIES | PUSH_FIRM(PUSH_PLAYERS), {0.0F, 5.0F, -0.0F}, 5.0F, 2.0F,
     0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* the last two of the robo's textures: lens02 for the beam's fast pieces,
 * and lens07 for its slow ones */
static NJS_TEXLIST grabber_beam_texlist = {
#include "enemy/robo/grabber_beam_texlist.inc"
};
static NJS_TEXANIM grabber_beam_texanims[2] = {
#include "enemy/robo/grabber_beam_texanims.inc"
};
static NJS_SPRITE grabber_beam_sprite = {
#include "enemy/robo/grabber_beam_sprite.inc"
};

static struct dust_params robo_trail_puff = {
    1.0F,
    0.4F,
    1.0F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.5F, 0.0F},
    {0.7F, 0.4F, 0.4F, 1.0F},
};
static struct dust_params grabber_spray_puff = {
    0.3F,
    0.08F,
    0.64F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.12F, 0.8F, 0.8F, 1.0F},
};
static struct dust_params grabber_drip_puff = {
    0.95F,
    0.08F,
    0.44F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.12F, 0.8F, 0.8F, 1.0F},
};

/* the grabber's faces in e_robo_texlist: sor_e01 to sor_e04 */
static Uint32 grabber_faces[4] = {14, 17, 18, 19};

static struct dust_params robo_side_puff = {
    0.4F,
    0.07F,
    0.47F,
    0.1F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.18F, 0.8F, 0.8F, 1.0F},
};
static struct dust_params robo_brake_puff = {
    0.8F,
    0.28F,
    0.37F,
    0.1F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.58F, 0.7F, 0.75F, 1.0F},
};
static struct dust_params robo_below_puff = {
    0.9F,
    0.08F,
    0.44F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.12F, 0.8F, 0.8F, 1.0F},
};
static struct dust_params robo_ground_puff = {
    0.45F,
    0.08F,
    0.44F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.12F, 0.8F, 0.8F, 1.0F},
};

/* the trail's speed and start, in record->matrix's frame */
static NJS_VECTOR robo_trail_speed = {0.0F, -2.0F, 0.0F};
static NJS_POINT3 robo_trail_from = {0.0F, -4.3F, 0.0F};

static void grabber_beam_slow_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float size = BEAM_SIZE(work);

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        draw_set_material(BEAM_ALPHA(work), 1.0F, 1.0F, 1.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njRotateX(NULL, NJM_DEG_ANG(270));
        njScale(NULL, size, size, size);
        njDrawSprite3D(&grabber_beam_sprite, 1,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        njPopMatrix(1);
    }
}

static void grabber_beam_slow_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *grabber = task->parent->work;
    Float size = BEAM_SIZE(work);

    switch (work->submode) {
    case BEAM_START:
        task->draw = grabber_beam_slow_draw;
        work->submode++;
        /* fall through */
    case BEAM_RUN:
        break;
    default:
        return;
    }
    work->pos.y -= 0.25F;
    if (TASK_TIMER(work) < 100) {
        BEAM_ALPHA(work) = 1.5F;
    } else {
        BEAM_ALPHA(work) -= 0.1F;
    }
    work->pos.x = grabber->pos.x;
    work->pos.z = grabber->pos.z;
    BEAM_SIZE(work) += 0.007F;
    draw_set_material(BEAM_ALPHA(work), 1.0F, 1.0F, 1.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njRotateX(NULL, NJM_DEG_ANG(270));
    njScale(NULL, size, size, size);
    njDrawSprite3D(&grabber_beam_sprite, 1,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    njPopMatrix(1);
    work->timer++;
    if (BEAM_ALPHA(work) < 0.0F) {
        task_kill(task);
    }
}

static void grabber_beam_fast_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float size = BEAM_SIZE(work);

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        draw_set_material(BEAM_ALPHA(work), 0.4F, 0.4F, 1.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njRotateX(NULL, NJM_DEG_ANG(270));
        njScale(NULL, size, size, size);
        njDrawSprite3D(&grabber_beam_sprite, 0,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        njPopMatrix(1);
    }
}

static void grabber_beam_fast_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *grabber = task->parent->work;
    Float size = BEAM_SIZE(work);

    switch (work->submode) {
    case BEAM_START:
        task->draw = grabber_beam_fast_draw;
        work->submode++;
        /* fall through */
    case BEAM_RUN:
        break;
    default:
        return;
    }
    work->pos.y -= 0.7F;
    if (TASK_TIMER(work) < 24) {
        BEAM_ALPHA(work) = 1.5F;
    } else {
        BEAM_ALPHA(work) -= 0.1F;
    }
    work->pos.x = grabber->pos.x;
    work->pos.z = grabber->pos.z;
    BEAM_SIZE(work) += 0.018F;
    draw_set_material(BEAM_ALPHA(work), 0.4F, 0.4F, 1.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njRotateX(NULL, NJM_DEG_ANG(270));
    njScale(NULL, size, size, size);
    njDrawSprite3D(&grabber_beam_sprite, 0,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    njPopMatrix(1);
    work->timer++;
    if (BEAM_ALPHA(work) < 0.0F) {
        task_kill(task);
    }
}

static void grabber_init(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;
    struct angles slope;

    collision_init(task, grabber_shapes, COLLISION_ENEMY, 1, 2);
    record = enemy_alloc_record(task, work);
    record->flags |= ENEMY_KEEP_ON_LEDGE | ENEMY_SHADOW;
    work->tick = 0;
    work->timer = 0;
    record->motion.slip = 0.0F;
    record->turn_speed = 0xe0;
    LIFT(record) = 5.0F;
    record->aim_y = work->ang.y;
    record->probe_offset.x = 0.0F;
    record->probe_offset.y = 0.0F;
    record->probe_offset.z = 0.0F;
    record->top = 5.0F;
    record->radius = 10.0F;
    record->bottom = 0.0F;
    record->step_down = 50.0F;
    record->wall_bounce = 1.0F;
    record->shadow_size = 1.6F;
    record->shadow_ratio = 1.0F;
    record->angles[ROBO_HEAD] = work->ang.y;
    record->turn_speed = 0x200;
    GRABBER_THRUST(record) = 0.05F;
    work->pos.y = shadow_draw_slope(work, 0.0F, &slope) + 10.0F;
    work->ang.x = slope.x;
    work->ang.z = slope.z;
    enemy_set_home(work, record);
    task->destructor = grabber_destroy;
    task->step = grabber_step;
    task->draw = grabber_draw;
    work->flags |= GRABBER_UNK_400;
}

static void grabber_spray(struct task_work *work, struct lockon_target *record)
{
    NJS_POINT3 from = {5.0F, 1.5F, 0.0F};
    NJS_POINT3 pos;
    NJS_VECTOR way = {0.7F, -0.05F, 0.0F};
    NJS_VECTOR speed;
    int i;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njSetMatrix(NULL, &record->matrix);
    for (i = 0; i < 8; i++) {
        njPushMatrix(NULL);
        TURN(njRotateY, i * NJM_DEG_ANG(45));
        njCalcVector(NULL, &way, &speed);
        njCalcPoint(NULL, &from, &pos);
        /* dust_create_from copies grabber_spray_puff before this pass stores
         * its speed and pos. so puff i takes pass i - 1's, and puff 0 takes
         * those of the previous call's last pass */
        dust_create_from(&grabber_spray_puff);
        grabber_spray_puff.speed = speed;
        grabber_spray_puff.pos = pos;
        njPopMatrix(1);
    }
    njPopMatrix(1);
}

static void grabber_drip(struct task_work *work, struct lockon_target *record)
{
    NJS_POINT3 from = {0.0F, 1.0F, 0.0F};
    NJS_POINT3 pos;
    NJS_VECTOR way = {0.0F, -0.5F, 0.0F};
    NJS_VECTOR speed;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njSetMatrix(NULL, &record->matrix);
    njCalcVector(NULL, &way, &speed);
    njCalcPoint(NULL, &from, &pos);
    njPopMatrix(1);
    grabber_drip_puff.speed = speed;
    grabber_drip_puff.pos = pos;
    dust_create_from(&grabber_drip_puff);
}

/* draw the grabber, and keep its drawn matrix in record->matrix for the
 * puffs. it does both only in view, on a frame's first step */
static void grabber_render(struct task_work *work, struct lockon_target *record)
{
    njSetTexture(&e_robo_texlist);
    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        TURN(njRotateY, work->ang.y);
        njRotateY(NULL, Z_TURN);
        njScale(NULL, 1.5F, 1.5F, 1.5F);
        njGetMatrix(&record->matrix);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njMultiMatrix(NULL, &record->matrix);
        draw_model(robo_grabber_model);
        njPopMatrix(1);
    }
}

static void grabber_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    grabber_render(work, record);
}

/* move the grabber across by its speed, less a drag of 0.04 of it. accel.y
 * takes a drag as well, which goes unused: the grabber's steps set its
 * height themselves */
static void grabber_move(struct task_work *work, struct lockon_target *record)
{
    record->motion.accel.x += -record->motion.speed.x * 0.04F;
    record->motion.accel.y += -record->motion.speed.y * 0.04F;
    record->motion.accel.z += -record->motion.speed.z * 0.04F;
    record->motion.speed.x += record->motion.accel.x;
    record->motion.speed.z += record->motion.accel.z;
    work->pos.x += record->motion.speed.x;
    work->pos.z += record->motion.speed.z;
    record->motion.accel.x = 0.0F;
    record->motion.accel.y = 0.0F;
    record->motion.accel.z = 0.0F;
}

static void grabber_wait(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;

    if (task->parent->work->mode == ROBO_WATCH) {
        record->last_mode = work->mode;
        work->mode = GRABBER_RISE;
        work->timer = 0;
        work->pos = task->parent->work->pos;
        work->ang.y = LOCKON_TARGET(task->parent)->angles[ROBO_HEAD];
    }
}

static void grabber_rise(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    Float ground = record->ground.sides[SIDE_BELOW].level;

    if (TASK_TIMER(work) < 20) {
        grabber_spray(work, record);
        work->pos.y += 0.05F;
    } else if (work->pos.y < ground + GRABBER_HEIGHT) {
        work->pos.y += 0.12F;
        grabber_drip(work, record);
    } else {
        record->last_mode = work->mode;
        work->mode = GRABBER_CHASE;
        work->timer = 0;
        work->collision->flags &= ~COLLISION_HIT;
        RISEN(record) = work->pos.y;
    }
    enemy_collide(work, record);
    grabber_render(work, record);
}

/* add the beam's pieces, a slow one and four fast ones in every 17 calls.
 * TRUE if the grabber's collision record touched a player's this step */
static Bool grabber_shine_beam(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    struct task *child;
    struct task_work *piece;
    struct collision_work *collision;
    int shape;

    if (work->tick++ > 15) {
        child = task_create_child(TASK_WORK, grabber_beam_slow_step, task);
        if (child != NULL) {
            piece = child->work;
            piece->pos = work->pos;
            piece->pos.y += 1.0F;
            BEAM_SIZE(piece) = 0.6F;
            piece->timer = 0;
        }
        work->tick = 0;
    }
    if (work->tick % 5 == 0) {
        child = task_create_child(TASK_WORK, grabber_beam_fast_step, task);
        if (child != NULL) {
            piece = child->work;
            piece->pos = work->pos;
            piece->pos.y += 1.0F;
            BEAM_SIZE(piece) = 0.56F;
            piece->timer = 0;
        }
    }
    if (work->collision->flags & COLLISION_HIT) {
        collision = work->collision;
        shape = collision->own_shape;
        if ((shape == 0 || shape == 1) &&
            collision->hit->kind == COLLISION_PLAYER) {
            return TRUE;
        }
    }
    return FALSE;
}

static void grabber_keep_height(struct task_work *work,
                                struct lockon_target *record)
{
    Float ground = record->ground.sides[SIDE_BELOW].level;

    if (enemy_player_distance2(work, 0) < 30.0F * 30.0F &&
        !(player_work[0]->flags & PLAYER_RECOILING)) {
        work->pos.y += 1.2F;
        if (work->pos.y > RISEN(record) + 20.0F) {
            work->pos.y = RISEN(record) + 20.0F;
        }
    } else {
        work->pos.y -= 0.5F;
        if (work->pos.y < ground + GRABBER_HEIGHT) {
            work->pos.y = ground + GRABBER_HEIGHT;
        }
    }
}

/* chase player 0 until the grabber catches a player it touches, whichever
 * player it is, or until the chase times out. either way the grabber then
 * holds and sets ROBO_FETCH */
static void grabber_chase(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];

    record->target = player->pos;
    enemy_turn_to_target(work, record);
    record->motion.accel.x = GRABBER_THRUST(record) * njCos(work->ang.y);
    record->motion.accel.z = GRABBER_THRUST(record) * njSin(-work->ang.y);
    grabber_keep_height(work, record);
    grabber_move(work, record);
    enemy_collide(work, record);
    grabber_render(work, record);
    if (grabber_shine_beam(task, record)) {
        record->last_mode = work->mode;
        work->mode = GRABBER_HOLD;
        work->timer = 0;
        task->parent->work->flags |= ROBO_FETCH;
        work->flags |= GRABBER_CAUGHT;
        player_set_mode(PLAYER_VARS(work->collision->hit->owner->work)->player,
                        PLAYER_MODE_CAUGHT);
    }
    if (TASK_TIMER(work) > CHASE_STEPS) {
        record->last_mode = work->mode;
        work->mode = GRABBER_HOLD;
        work->timer = 0;
        task->parent->work->flags |= ROBO_FETCH;
    }
    if (TASK_TIMER(work) > 60) {
        collision_register(work);
    } else {
        work->collision->flags &= ~COLLISION_HIT;
    }
}

static void grabber_hold(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;

    work->ang.y -= 0x400;
    record->motion.accel.x -= record->motion.speed.x * 0.05F;
    record->motion.accel.z -= record->motion.speed.z * 0.05F;
    grabber_keep_height(work, record);
    grabber_move(work, record);
    enemy_collide(work, record);
    /* @hack redundant: an empty if on the beam's touch, which keeps the image's
     * TST R0,R0 at 0x8c10696a. as a plain call, the function runs 2 bytes
     * short and 32 of 75 words differ */
    if (grabber_shine_beam(task, record)) {
    }
    grabber_render(work, record);
    if (task->parent->work->mode == ROBO_CLIMB) {
        record->last_mode = work->mode;
        work->mode = GRABBER_RETURN;
        work->timer = 0;
        record->target = task->parent->work->pos;
    }
    collision_register(work);
}

/* fly back toward the robo, and start GRABBER_SETTLE within EASE_RANGE of
 * it. the distance is to where record->target put the robo on the step
 * before */
static void grabber_return(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    Float distance2 = SQUARE(record->target.x - work->pos.x) +
                      SQUARE(record->target.z - work->pos.z);

    record->target = task->parent->work->pos;
    grabber_keep_height(work, record);
    enemy_turn_to_target(work, record);
    grabber_move(work, record);
    enemy_collide(work, record);
    grabber_render(work, record);
    if (distance2 < EASE_RANGE * EASE_RANGE) {
        record->last_mode = work->mode;
        work->mode = GRABBER_SETTLE;
        work->timer = 0;
        record->aim_y = LOCKON_TARGET(task->parent)->angles[ROBO_HEAD];
        /* @hack redundant: a return where the else-if chain ends anyway, which
         * keeps the aim_y store ahead of the BRA, and a NOP in its delay
         * slot (0x8c106a64). without it, the store fills the slot, the
         * function runs 2 bytes short, and 29 of 123 words differ */
        return;
    } else if (distance2 < SLOW_RANGE * SLOW_RANGE) {
        record->motion.accel.x =
            GRABBER_THRUST(record) * njCos(work->ang.y) * 0.2F;
        record->motion.accel.z =
            GRABBER_THRUST(record) * njSin(-work->ang.y) * 0.2F;
    } else {
        record->motion.accel.x =
            GRABBER_THRUST(record) * njCos(work->ang.y) * 0.4F;
        record->motion.accel.z =
            GRABBER_THRUST(record) * njSin(-work->ang.y) * 0.4F;
    }
}

/* move the grabber a fixed step on each axis toward its spot on the robo,
 * at the robo's drawn height, and settle it within SETTLE_RANGE. over the
 * last 1 above the spot, it sinks slower and sprays */
static void grabber_settle(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    struct task_work *robo = task->parent->work;
    Float dx;
    Float dy;
    Float dz;

    work->ang.y = angle_approach(
        work->ang.y, LOCKON_TARGET(task->parent)->angles[ROBO_HEAD], 0x200);
    dx = robo->pos.x - work->pos.x;
    dy = (robo->pos.y + BOB(LOCKON_TARGET(task->parent))) - work->pos.y;
    dz = robo->pos.z - work->pos.z;
    if (dx > 0.0F) {
        work->pos.x += 0.1F;
    } else {
        work->pos.x -= 0.1F;
    }
    if (dy > 0.0F) {
        work->pos.y += 0.07F;
    } else if (dy > -1.0F) {
        work->pos.y -= 0.03F;
        grabber_spray(work, record);
    } else {
        work->pos.y -= 0.07F;
    }
    if (dz > 0.0F) {
        work->pos.z += 0.1F;
    } else {
        work->pos.z -= 0.1F;
    }
    if ((dx * dx) + (dy * dy) + (dz * dz) < SETTLE_RANGE * SETTLE_RANGE) {
        record->last_mode = work->mode;
        work->mode = GRABBER_WAIT;
        work->timer = 0;
        task->parent->work->flags |= ROBO_CARRYING;
        sound_play_at_for(SE_RISE, (int)work, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 1);
    }
    enemy_collide(work, record);
    grabber_render(work, record);
}

static void grabber_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    enemy_end(task, work, record);
}

static void grabber_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct lockon_target *record = LOCKON_TARGET(task);

    if (edit_mode) {
        return;
    }
    /* @hack redundant: a store to player->mode that never runs. it keeps the
     * image's load of player 0's work, which nothing uses (0x8c106c9c,
     * 0x8c106ca6). without it, the load goes, the function runs 4 bytes short
     * and 52 of 107 words differ */
    if (0) {
        player->mode = 0;
    }
    switch (work->mode) {
    case GRABBER_WAIT:
        grabber_wait(task, record);
        break;
    case GRABBER_RISE:
        grabber_rise(task, record);
        break;
    case GRABBER_CHASE:
        grabber_chase(task, record);
        sound_play_at_for(SE_GRABBER_CHASE, (int)work, 1, 0, work->pos.x,
                          work->pos.y, work->pos.z, 1);
        break;
    case GRABBER_HOLD:
        grabber_hold(task, record);
        sound_play_at_for(SE_GRABBER_HOLD, (int)work, 1, 0, work->pos.x,
                          work->pos.y, work->pos.z, 1);
        break;
    case GRABBER_RETURN:
        grabber_return(task, record);
        break;
    case GRABBER_SETTLE:
        grabber_settle(task, record);
        break;
    case GRABBER_IDLE:
        return;
    }
    enemy_save_pos(work, record);
    work->timer++;
    task_step_children(task);
}

/* a placement's scl.x of 0 or more starts the robo hovering, and a
 * negative one cruising. while the set editor holds the task, robo_init
 * sets up only the task's functions and flags */
void robo_init(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;
    NJS_MODEL *model;
    NJS_MATERIAL *materials;
    struct task *child;
    struct task_work *grabber;
    struct angles slope;
    NJS_ACTION *action;

    if (!placement_editing(task)) {
        collision_init(task, robo_shapes, COLLISION_ENEMY, 1, 2);
        work->collision->shapes[1].flags |= SHAPE_OFF;
        record = enemy_alloc_record(task, work);
        record->flags |= ENEMY_KEEP_ON_LEDGE | ENEMY_SHADOW;
        model = heap_alloc(sizeof(NJS_MODEL));
        njMemCopy(model, robo_grabber_model, sizeof(NJS_MODEL));
        /* robo_grabber_model has five materials. the copy gets room for
         * six */
        materials = heap_alloc(6 * sizeof(NJS_MATERIAL));
        njMemCopy(materials, robo_grabber_model->mats,
                  5 * sizeof(NJS_MATERIAL));
        model->mats = materials;
        VARS(work)->grabber = model;
        if (work->scl.x >= 0.0F) {
            work->mode = ROBO_HOVER;
            record->angles[ROBO_PITCH] =
                (Angle)(rand() * RAND_SCALE * 5.0F * ANGLE_PER_DEGREE) + 0x1a00;
            record->angles[ROBO_SWAY] =
                (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
            work->submode = 3;
        } else {
            work->mode = ROBO_CRUISE;
            record->angles[ROBO_PITCH] = 0;
            record->angles[ROBO_SWAY] = 16;
            work->submode = 3;
        }
        /* work->submode picks the copy's face, its material 3. both starts
         * pick sor_e04, which the material has already */
        VARS(work)->grabber->mats[3].attr_texId = grabber_faces[work->submode];
        work->submode = 1;
        work->tick = 0;
        work->timer = 0;
        record->motion.slip = 0.0F;
        record->turn_speed = 0xe0;
        LIFT(record) = 5.0F;
        record->aim_y = work->ang.y;
        record->frame = 0.0F;
        action = &robo_action;
        record->action = action;
        record->next_action = action;
        record->probe_offset.x = 0.0F;
        record->probe_offset.y = -5.0F;
        record->probe_offset.z = 0.0F;
        record->top = 15.0F;
        record->radius = 15.0F;
        record->bottom = -3.0F;
        record->wall_bounce = 1.0F;
        record->shadow_size = 1.6F;
        record->shadow_ratio = 1.0F;
        record->angles[ROBO_HEAD] = work->ang.y;
        SPIN(record) = 0;
        THRUST(record) = 0.1F;
        work->pos.y = shadow_draw_slope(work, 0.0F, &slope) + 30.0F;
        work->ang.x = slope.x;
        work->ang.z = slope.z;
        child = task_create_child(TASK_WORK, grabber_init, task);
        if (child != NULL) {
            grabber = child->work;
            grabber->pos = work->pos;
            grabber->ang = work->ang;
            grabber->scl = work->scl;
            grabber->submode = 0;
            grabber->timer = 0;
        }
        enemy_set_home(work, record);
    }
    task->destructor = robo_destroy;
    task->step = robo_step;
    task->draw = robo_draw;
    work->flags |= ROBO_CARRYING | ROBO_SOLID;
    work->flags &= ~(ROBO_FETCH | ROBO_SWINGING | ROBO_BLENDING);
}

static void robo_knock_away(struct task_work *work,
                            struct lockon_target *record)
{
    struct task_work *player = player_work[0];
    NJS_VECTOR speed;
    NJS_VECTOR away;
    Angle ang[3];

    away.x = player->pos.x - work->pos.x;
    away.y = 0.0F;
    away.z = player->pos.z - work->pos.z;
    ang[0] = 0;
    ang[1] = (Angle)(atan2f(away.x, away.z) * ANGLE_PER_RADIAN);
    ang[2] = 0;
    speed.x = 3.5F;
    speed.y = 1.8F;
    speed.z = 0.0F;
    player_send_flying(0, &speed, ang, 60);
    player->pos.y += 1.0F;
}

/* start ROBO_BEATEN when enemy_take_hit returns TRUE on a step the robo's
 * collision record touched another */
static void robo_take_hit(struct task_work *work, struct lockon_target *record)
{
    struct collision_work *collision;

    enemy_ignore((void *)player_work);
    /* @hack redundant: a discarded read of hit_shape, which the image makes and
     * never uses (MOV.W @R3,R3 at 0x8c107116). without it, the function
     * runs 6 bytes short and 29 of 31 words differ */
    (void)work->collision->hit_shape;
    collision = work->collision;
    enemy_ignore(collision->hit);
    if (!(collision->flags & COLLISION_HIT)) {
        return;
    }
    if (!enemy_take_hit(work, record)) {
        return;
    }
    work->mode = ROBO_BEATEN;
    ENEMY_MARK_BEATEN(work);
    work->scl.z = 0.35F;
}

static void robo_lift(struct task_work *work, struct lockon_target *record)
{
    Float height = work->pos.y - record->ground.sides[SIDE_BELOW].level;

    record->motion.accel.y += LIFT(record) / (3.0F + (1.4F * height));
}

static void robo_move(struct task_work *work, struct lockon_target *record)
{
    record->motion.accel.x += -record->motion.speed.x * 0.04F;
    record->motion.accel.y += -record->motion.speed.y * 0.04F;
    record->motion.accel.z += -record->motion.speed.z * 0.04F;
    record->motion.speed.x += record->motion.accel.x;
    record->motion.speed.y += record->motion.accel.y - 0.06F;
    record->motion.speed.z += record->motion.accel.z;
    work->pos.x += record->motion.speed.x;
    work->pos.y += record->motion.speed.y;
    work->pos.z += record->motion.speed.z;
    record->motion.accel.x = 0.0F;
    record->motion.accel.y = 0.0F;
    record->motion.accel.z = 0.0F;
}

/* advance the robo's squash and its blend, even out of view, then draw it
 * as robo_draw does */
static void robo_animate(struct task_work *work, struct lockon_target *record)
{
    Float across = work->scl.x * 1.5F;
    Float up = work->scl.y * 1.5F;

    enemy_spring_scale(work, record);
    njSetTexture(&e_robo_texlist);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    if (work->flags & ROBO_BLENDING) {
        record->blend += 0.1F;
        if (record->blend > 1.0F) {
            record->blend = 0.0F;
            record->frame = record->next_frame;
            record->action = record->next_action;
            work->submode++;
        }
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        return;
    }
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + BOB(record), work->pos.z);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, record->angles[ROBO_PITCH]);
    TURN(njRotateY, SPIN(record) + Z_TURN);
    njScale(NULL, across, up, across);
    njGetMatrix(&record->matrix);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njMultiMatrix(NULL, &record->matrix);
    if (work->flags & ROBO_BLENDING) {
        NJS_MOTION_LINK link;
        NJS_ACTION_LINK blend;

        blend.motionlink = &link;
        blend.object = record->action->object;
        link.motion[0] = record->action->motion;
        link.motion[1] = record->action->motion;
        link.frame[0] = record->frame;
        link.frame[1] = record->next_frame;
        draw_action_link(&blend, record->blend);
    } else {
        draw_action(record->action, record->frame);
    }
    TURN(njRotateY, record->angles[ROBO_HEAD] - work->ang.y - SPIN(record));
    draw_model(&robo_head_model);
    if (work->flags & ROBO_CARRYING) {
        draw_model(VARS(work)->grabber);
    }
    njPopMatrix(1);
}

/* draw the robo, and keep its drawn matrix in record->matrix for the puffs
 * and the trail. it does both only in view, on a frame's first step */
static void robo_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);
    Float across = work->scl.x * 1.5F;
    Float up = work->scl.y * 1.5F;

    njSetTexture(&e_robo_texlist);
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        return;
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + BOB(record), work->pos.z);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, record->angles[ROBO_PITCH]);
    TURN(njRotateY, SPIN(record) + Z_TURN);
    njScale(NULL, across, up, across);
    njGetMatrix(&record->matrix);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njMultiMatrix(NULL, &record->matrix);
    if (work->flags & ROBO_BLENDING) {
        NJS_MOTION_LINK link;
        NJS_ACTION_LINK blend;

        blend.motionlink = &link;
        blend.object = record->action->object;
        link.motion[0] = record->action->motion;
        link.motion[1] = record->action->motion;
        link.frame[0] = record->frame;
        link.frame[1] = record->next_frame;
        draw_action_link(&blend, record->blend);
    } else {
        draw_action(record->action, record->frame);
    }
    TURN(njRotateY, record->angles[ROBO_HEAD] - work->ang.y - SPIN(record));
    draw_model(&robo_head_model);
    if (work->flags & ROBO_CARRYING) {
        draw_model(VARS(work)->grabber);
    }
    njPopMatrix(1);
}

static void robo_draw_edit(struct task_work *work)
{
    njSetTexture(&e_robo_texlist);
    if (work->scl.x >= 0.0F) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y + Z_TURN);
        njRotateX(NULL, NJM_DEG_ANG(45));
        njScale(NULL, 1.2F, 1.2F, 1.2F);
        draw_action(&robo_action, 1.0F);
        njPopMatrix(1);
    } else {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y + Z_TURN);
        njScale(NULL, 1.2F, 1.2F, 1.2F);
        draw_action(&robo_action, 1.0F);
        njPopMatrix(1);
    }
}

static void robo_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);
    NJS_MODEL *model = VARS(work)->grabber;

    if (model != NULL) {
        NJS_MATERIAL *materials = model->mats;

        if (materials != NULL) {
            heap_free(materials);
        }
        heap_free(model);
        VARS(work)->grabber = NULL;
    }
    enemy_end(task, work, record);
}

/* free the robo's copy of the grabber, and kill the task so that its
 * placement makes no more */
static void robo_finish(struct task *task, struct task_work *work,
                        struct lockon_target *record)
{
    NJS_MODEL *model = VARS(work)->grabber;
    NJS_MATERIAL *materials = model->mats;

    if (materials != NULL) {
        heap_free(materials);
    }
    heap_free(model);
    VARS(work)->grabber = NULL;
    enemy_end(task, work, record);
    placement_kill(task);
}

static void robo_explode(struct task_work *work, struct lockon_target *record)
{
    explosion_create_at(work->pos.x, work->pos.y + 5.0F, work->pos.z, 1.4F);
    enemy_scatter_debris(work, DEBRIS_COUNT);
    animal_create_random(8, work->pos.x, work->pos.y + 15.0F, work->pos.z);
}

/* TRUE while player 0, within NOTICE_RANGE, presses jump or runs at 1.8
 * times its trot speed or more */
static Bool robo_notices(struct task_work *work, struct lockon_target *record)
{
    struct task_work *player = player_work[0];
    struct player *own = player_motion[0]->record;

    if (enemy_player_distance2(work, 0) < NOTICE_RANGE * NOTICE_RANGE) {
        if (player_jump_pressed(player, own)) {
            return TRUE;
        }
        if (own->speed.x >= own->consts.trot_speed * 1.8F) {
            return TRUE;
        }
    }
    return FALSE;
}

/* TRUE while player 0 is in the robo's sight, which SIGHT_RANGE,
 * SIGHT_HEIGHT and SIGHT_ANGLE bound. otherwise it returns FALSE and steers
 * aim_y: toward player 0 while robo_notices finds it, and else on a
 * wander */
static Bool robo_search(struct task_work *work, struct lockon_target *record)
{
    struct task_work *player = player_work[0];
    int difference;

    if (njAbs(player->pos.y - work->pos.y) < SIGHT_HEIGHT &&
        enemy_player_distance2(work, 0) < SIGHT_RANGE * SIGHT_RANGE) {
        difference =
            angle_distance((Angle)(atan2f(player->pos.x - work->pos.x,
                                          player->pos.z - work->pos.z) *
                                   ANGLE_PER_RADIAN) -
                               Z_TURN,
                           record->angles[ROBO_HEAD]);
        if (difference < SIGHT_ANGLE) {
            return TRUE;
        }
    }
    if (robo_notices(work, record)) {
        work->timer = 0;
        work->tick = 90;
        enemy_aim_at_player(work, record, 0);
    } else {
        if (TASK_TIMER(work) > work->tick) {
            work->timer = 0;
            work->tick = (int)(rand() * RAND_SCALE * 70.0F) + 60;
            if (work->tick < 120) {
                record->aim_y =
                    (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
            }
        }
        if (work->tick > 120) {
            record->aim_y += 0x200;
        }
    }
    return FALSE;
}

static void robo_puff_sides(struct task_work *work,
                            struct lockon_target *record)
{
    NJS_POINT3 minus_x = {-4.1F, -3.7F, 2.0F};
    NJS_POINT3 plus_x = {4.1F, -3.7F, 2.0F};
    NJS_POINT3 minus_x_pos;
    NJS_POINT3 plus_x_pos;
    NJS_VECTOR way = {0.0F, -0.8F, 0.6F};
    NJS_VECTOR speed;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njSetMatrix(NULL, &record->matrix);
    njCalcVector(NULL, &way, &speed);
    njCalcPoint(NULL, &minus_x, &minus_x_pos);
    njCalcPoint(NULL, &plus_x, &plus_x_pos);
    njPopMatrix(1);
    robo_side_puff.speed = speed;
    robo_side_puff.pos = minus_x_pos;
    dust_create_from(&robo_side_puff);
    robo_side_puff.pos = plus_x_pos;
    dust_create_from(&robo_side_puff);
}

static void robo_puff_brake(struct task_work *work,
                            struct lockon_target *record)
{
    NJS_POINT3 minus_x = {-4.1F, -3.9F, 1.7F};
    NJS_POINT3 plus_x = {4.1F, -3.9F, 1.7F};
    NJS_POINT3 minus_x_pos;
    NJS_POINT3 plus_x_pos;
    NJS_VECTOR way = {0.0F, -2.4F, 2.4F};
    NJS_VECTOR speed;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njSetMatrix(NULL, &record->matrix);
    njCalcVector(NULL, &way, &speed);
    njCalcPoint(NULL, &minus_x, &minus_x_pos);
    njCalcPoint(NULL, &plus_x, &plus_x_pos);
    njPopMatrix(1);
    robo_brake_puff.speed = speed;
    robo_brake_puff.pos = minus_x_pos;
    dust_create_from(&robo_brake_puff);
    robo_brake_puff.pos = plus_x_pos;
    dust_create_from(&robo_brake_puff);
}

static void robo_puff_below(struct task_work *work,
                            struct lockon_target *record)
{
    NJS_POINT3 from = {0.0F, -4.3F, 0.0F};
    NJS_POINT3 pos;
    NJS_VECTOR way = {0.0F, -0.7F, 0.0F};
    NJS_VECTOR speed;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njSetMatrix(NULL, &record->matrix);
    njCalcVector(NULL, &way, &speed);
    njCalcPoint(NULL, &from, &pos);
    njPopMatrix(1);
    robo_below_puff.speed = speed;
    robo_below_puff.pos = pos;
    dust_create_from(&robo_below_puff);
}

static void robo_puff_ground(struct task_work *work,
                             struct lockon_target *record)
{
    Float ground = record->ground.sides[SIDE_BELOW].level;
    NJS_POINT3 from = {2.0F, 1.0F, 0.0F};
    NJS_POINT3 pos;
    NJS_VECTOR way = {0.7F, 0.1F, 0.0F};
    NJS_VECTOR speed;
    Angle start = (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
    int i;

    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslate(NULL, work->pos.x, ground, work->pos.z);
    for (i = 0; i < 4; i++) {
        njPushMatrix(NULL);
        TURN(njRotateY, start + (i * NJM_DEG_ANG(90)));
        njCalcVector(NULL, &way, &speed);
        njCalcPoint(NULL, &from, &pos);
        robo_ground_puff.speed = speed;
        robo_ground_puff.pos = pos;
        dust_create_from(&robo_ground_puff);
        njPopMatrix(1);
    }
    njPopMatrix(1);
}

static void robo_trail(struct task_work *work, struct lockon_target *record)
{
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njSetMatrix(NULL, &record->matrix);
    njCalcVector(NULL, &robo_trail_speed, &robo_trail_puff.speed);
    njCalcPoint(NULL, &robo_trail_from, &robo_trail_puff.pos);
    njPopMatrix(1);
    njAddVector(&robo_trail_puff.speed, &record->motion.speed);
    panlight_create(&robo_trail_puff);
    /* @hack redundant: a return where the function ends, which makes record's
     * frame slot with ADD #-4,R15 and a store (0x8c108024). without it, SHC
     * pushes record instead, the function runs 2 bytes short, and 42 of 44
     * words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

static void robo_turn(struct task_work *work, struct lockon_target *record)
{
    record->angles[ROBO_HEAD] =
        angle_approach(record->angles[ROBO_HEAD], record->aim_y,
                       record->angles[ROBO_HEAD_SPEED]);
    work->ang.y = angle_approach(work->ang.y, record->angles[ROBO_HEAD],
                                 record->angles[ROBO_TURN_SPEED]);
}

/* start ROBO_SWING while player 0 is within SWING_RANGE */
static void robo_swing_when_near(struct task_work *work,
                                 struct lockon_target *record)
{
    Float distance2 = enemy_player_distance2(work, 0);

    if (distance2 < SWING_RANGE * SWING_RANGE) {
        record->last_mode = work->mode;
        work->mode = ROBO_SWING;
        work->timer = 0;
        work->flags |= ROBO_SWINGING | ROBO_BLENDING;
        work->submode = SWING_SPIN_UP;
        record->blend = 0.0F;
        record->next_frame = SWING_START;
        SWING_FROM(record) = record->frame;
    }
}

/* turn toward player 0 and swing the arms through the stages of enum
 * robo_swing_stage. a player's touch of the arms' shape knocks player 0
 * away, whichever player it was */
static void robo_swing(struct task_work *work, struct lockon_target *record)
{
    struct collision_work *collision;

    enemy_player_distance2(work, 0);
    collision = work->collision;
    record->target = player_work[0]->pos;
    enemy_aim_at_target(work, record);
    work->ang.y = angle_approach(work->ang.y, record->aim_y, 0x900);
    if ((collision->flags & COLLISION_HIT) && collision->own_shape == 1 &&
        collision->hit->kind == COLLISION_PLAYER) {
        robo_knock_away(work, record);
    }
    switch (work->submode) {
    case SWING_SPIN_UP:
        SPIN(record) += 0x800;
        break;
    case SWING_OUT:
        work->collision->shapes[1].flags &= ~SHAPE_OFF;
        record->frame += 0.45F;
        if (record->frame > SWING_END) {
            record->frame = SWING_END;
            work->submode = SWING_HOLD;
            record->next_frame = SWING_FROM(record);
        }
        work->flags &= ~ROBO_BLENDING;
        break;
    case SWING_HOLD:
        work->collision->shapes[1].flags &= ~SHAPE_OFF;
        if (TASK_TIMER(work) > SWING_STEPS) {
            work->submode = SWING_SPIN_DOWN;
        }
        work->flags &= ~ROBO_BLENDING;
        break;
    case SWING_SPIN_DOWN:
        work->collision->shapes[1].flags |= SHAPE_OFF;
        SPIN(record) -= 0x800;
        work->flags |= ROBO_BLENDING;
        break;
    case SWING_DONE:
        work->flags &= ~(ROBO_SWINGING | ROBO_BLENDING);
        work->mode = record->last_mode;
        work->timer = 0;
        break;
    }
    robo_lift(work, record);
    robo_move(work, record);
    enemy_collide(work, record);
    robo_animate(work, record);
}

static void robo_hover(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    Float sway;

    if (robo_notices(work, record)) {
        record->last_mode = work->mode;
        work->mode = ROBO_RISE;
        work->timer = 0;
    }
    robo_lift(work, record);
    robo_move(work, record);
    if (work->flags & ROBO_LEANING) {
        record->angles[ROBO_PITCH] -= 0x44;
        if ((Uint16)(record->angles[ROBO_PITCH] + 0x8000) <
            0x8000 + HOVER_PITCH) {
            work->flags &= ~ROBO_LEANING;
            record->angles[ROBO_PITCH] = HOVER_PITCH;
            record->angles[ROBO_SWAY] = 0x8000;
        }
        record->frame -= 0.12F;
        if (record->frame < 0.5F) {
            record->frame = 0.5F;
        }
    } else {
        sway = njSin(record->angles[ROBO_SWAY]);
        record->angles[ROBO_PITCH] =
            (Angle)(sway * 8.0F * ANGLE_PER_DEGREE) + HOVER_PITCH;
        record->frame = (sway + 1.0F) * 0.5F;
    }
    record->angles[ROBO_SWAY] += 0x180;
    BOB(record) = njSin(record->angles[ROBO_BOB]) * 0.5F;
    LIFT(record) = 1.9F;
    record->angles[ROBO_BOB] += 0x130;
    record->angles[ROBO_HEAD] =
        angle_approach(record->angles[ROBO_HEAD], work->ang.y, 0x80);
    enemy_collide(work, record);
    robo_animate(work, record);
}

static void robo_rise(struct task_work *work, struct lockon_target *record)
{
    Uint16 beat;

    robo_lift(work, record);
    robo_move(work, record);
    record->angles[ROBO_PITCH] =
        angle_approach(record->angles[ROBO_PITCH], 0, 0xb0);
    if (LIFT(record) < 2.1F) {
        LIFT(record) += 0.05F;
    }
    record->angles[ROBO_BOB] += 0x190;
    BOB(record) = njSin(record->angles[ROBO_BOB]) * 0.5F;
    if (record->frame < 9.0F) {
        record->frame += 0.22F;
    }
    enemy_collide(work, record);
    robo_animate(work, record);
    beat = TASK_TIMER(work) % 50;
    if (beat == 0 || beat == 17) {
        work->tick = 0;
    }
    if (work->tick++ < 10) {
        robo_puff_sides(work, record);
    }
    robo_puff_below(work, record);
    if (TASK_TIMER(work) > 40) {
        record->angles[ROBO_BEAT] = 0x4000;
        record->last_mode = work->mode;
        work->mode = ROBO_CRUISE;
        work->timer = 0;
        record->angles[ROBO_HEAD_SPEED] = 0x1a0;
        record->angles[ROBO_TURN_SPEED] = 0x70;
    }
}

static void robo_cruise(struct task_work *work, struct lockon_target *record)
{
    if (robo_search(work, record)) {
        record->last_mode = work->mode;
        work->mode = ROBO_WATCH;
        work->timer = 0;
        work->flags &= ~ROBO_CARRYING;
        record->angles[ROBO_HEAD_SPEED] = 0x1a0;
        record->angles[ROBO_TURN_SPEED] = 0xb0;
    }
    record->motion.accel.x = THRUST(record) * njCos(work->ang.y) * 0.02F;
    record->motion.accel.z = THRUST(record) * njSin(-work->ang.y) * 0.02F;
    robo_turn(work, record);
    robo_lift(work, record);
    robo_move(work, record);
    record->angles[ROBO_BEAT] += 0x300;
    record->angles[ROBO_BOB] += 0x1b0;
    record->frame = njSin(record->angles[ROBO_BEAT]) + 8.0F;
    BOB(record) = njSin(record->angles[ROBO_BOB]) * 0.5F;
    enemy_collide(work, record);
    robo_animate(work, record);
}

static void robo_watch(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;

    if ((work->flags & ROBO_FETCH) && !(work->flags & ROBO_SWINGING)) {
        record->last_mode = work->mode;
        work->mode = ROBO_AIM;
        work->timer = 0;
        if (task->child != NULL) {
            record->target = task->child->work->pos;
        }
        record->angles[ROBO_HEAD_SPEED] = 0x500;
        record->angles[ROBO_TURN_SPEED] = 0x3b0;
        work->collision->shapes[1].flags |= SHAPE_OFF;
    }
    robo_swing_when_near(work, record);
    robo_lift(work, record);
    robo_move(work, record);
    enemy_aim_at_player(work, record, 0);
    robo_turn(work, record);
    record->angles[ROBO_BEAT] += 0x220;
    record->angles[ROBO_BOB] += 0x1b0;
    record->frame = njSin(record->angles[ROBO_BEAT]) + 8.0F;
    BOB(record) = njSin(record->angles[ROBO_BOB]) * 0.5F;
    enemy_collide(work, record);
    robo_animate(work, record);
}

static void robo_aim(struct task_work *work, struct lockon_target *record)
{
    enemy_aim_at_target(work, record);
    record->angles[ROBO_HEAD] =
        angle_approach(record->angles[ROBO_HEAD], record->aim_y, 0x1a0);
    work->ang.y = angle_approach(work->ang.y, record->angles[ROBO_HEAD], 0xb0);
    SPIN(record) = angle_approach(SPIN(record), 0, 0x100);
    record->angles[ROBO_PITCH] -= 0x100;
    if ((Uint16)(record->angles[ROBO_PITCH] + 0x8000) <
        (Uint16)(AIM_PITCH + 0x8000)) {
        record->angles[ROBO_PITCH] = AIM_PITCH;
    }
    record->angles[ROBO_BOB] += 0x3800;
    BOB(record) = njSin(record->angles[ROBO_BOB]) * 0.5F;
    if (record->frame < 15.0F) {
        record->frame += 0.09F;
    }
    if (LIFT(record) > 1.5F) {
        LIFT(record) -= 0.04F;
    }
    robo_lift(work, record);
    robo_move(work, record);
    enemy_collide(work, record);
    robo_animate(work, record);
    if (TASK_TIMER(work) > 60) {
        record->last_mode = work->mode;
        work->mode = ROBO_SWOOP;
        work->timer = 0;
        record->angles[ROBO_HEAD_SPEED] = 0x300;
        record->angles[ROBO_TURN_SPEED] = 0x1d0;
    }
}

static void robo_swoop(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;

    if (task->child != NULL) {
        record->target = task->child->work->pos;
    }
    enemy_aim_at_target(work, record);
    record->angles[ROBO_BOB] += 0x3800;
    BOB(record) = njSin(record->angles[ROBO_BOB]) * 0.5F;
    record->motion.accel.x = THRUST(record) * njCos(work->ang.y);
    record->motion.accel.z = THRUST(record) * njSin(-work->ang.y);
    robo_turn(work, record);
    robo_lift(work, record);
    robo_move(work, record);
    enemy_collide(work, record);
    robo_animate(work, record);
    robo_puff_below(work, record);
    robo_trail(work, record);
    robo_puff_ground(work, record);
    if (SQUARE(record->target.x - work->pos.x) +
            SQUARE(record->target.z - work->pos.z) <
        SWOOP_RANGE * SWOOP_RANGE) {
        record->last_mode = work->mode;
        work->mode = ROBO_BRAKE;
        work->timer = 0;
        record->angles[ROBO_HEAD_SPEED] = 0x1a0;
        record->angles[ROBO_TURN_SPEED] = 0xa0;
    }
}

static void robo_brake(struct task_work *work, struct lockon_target *record)
{
    record->angles[ROBO_PITCH] += 0x400;
    if ((Uint16)(record->angles[ROBO_PITCH] + 0x8000) > BRAKE_PITCH + 0x8000) {
        record->angles[ROBO_PITCH] = BRAKE_PITCH;
    }
    record->angles[ROBO_BOB] += 0x500;
    BOB(record) = njSin(record->angles[ROBO_BOB]) * 0.5F;
    if (record->frame > 8.0F) {
        record->frame -= 0.1F;
    }
    if (LIFT(record) < 2.0F) {
        LIFT(record) += 0.02F;
    }
    robo_turn(work, record);
    robo_lift(work, record);
    robo_move(work, record);
    enemy_collide(work, record);
    robo_animate(work, record);
    if (TASK_TIMER(work) > 10) {
        robo_puff_brake(work, record);
        record->motion.accel.x -= record->motion.speed.x * 0.025F;
        record->motion.accel.z -= record->motion.speed.z * 0.025F;
    }
    if (TASK_TIMER(work) > 40) {
        record->last_mode = work->mode;
        work->mode = ROBO_CLIMB;
        work->timer = 0;
        record->angles[ROBO_BEAT] = 0;
        record->aim_y += 0x8000;
    }
}

static void robo_climb(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;

    record->angles[ROBO_PITCH] -= 0xb0;
    if ((Uint16)(record->angles[ROBO_PITCH] + 0x8000) < 0x8000) {
        record->angles[ROBO_PITCH] = 0;
    }
    record->angles[ROBO_BOB] += 0x400;
    BOB(record) = njSin(record->angles[ROBO_BOB]) * 0.5F;
    record->angles[ROBO_BEAT] += 0x300;
    record->frame = njSin(record->angles[ROBO_BEAT]) + 8.0F;
    robo_turn(work, record);
    robo_lift(work, record);
    robo_move(work, record);
    enemy_collide(work, record);
    robo_animate(work, record);
    if (work->flags & ROBO_CARRYING) {
        /* @hack spelling: robo_search's arm and the last arm both start
         * ROBO_CRUISE, each with stores of its own (0x8c108d18, 0x8c108dae);
         * joined with ||, 125 of 126 words differ */
        if (robo_search(work, record)) {
            record->last_mode = work->mode;
            work->mode = ROBO_CRUISE;
            work->timer = 0;
            work->flags &= ~ROBO_FETCH;
        } else if (rand() * RAND_SCALE > 0.5F) {
            record->last_mode = work->mode;
            work->mode = ROBO_HOVER;
            work->timer = 0;
            work->flags |= ROBO_LEANING;
            work->flags &= ~ROBO_FETCH;
        } else {
            record->last_mode = work->mode;
            work->mode = ROBO_CRUISE;
            work->timer = 0;
            work->flags &= ~ROBO_FETCH;
        }
    }
}

static void robo_fall(struct task_work *work, struct lockon_target *record)
{
    robo_move(work, record);
    enemy_collide(work, record);
    robo_animate(work, record);
    work->mode = ROBO_BEATEN;
}

static void robo_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct lockon_target *record = LOCKON_TARGET(task);

    if (edit_mode) {
        robo_draw_edit(work);
        return;
    }
    /* @hack redundant: a store to player->mode that never runs. it keeps the
     * image's load of player 0's work, which nothing uses (0x8c108e2c,
     * 0x8c108e36). without it, the load goes, sound_play_at_for's address
     * and its argument 1 trade R10 and R11, and 25 of 179 words differ */
    if (0) {
        player->mode = 0;
    }
    if (placement_try_leave_by(task, ROBO_LEAVE_RANGE)) {
        return;
    }
    robo_take_hit(work, record);
    switch (work->mode) {
    case ROBO_HOVER:
        robo_hover(task, record);
        sound_play_at_for(SE_HOVER, (int)work, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 1);
        break;
    case ROBO_RISE:
        robo_rise(work, record);
        sound_play_at_for(SE_RISE, (int)work, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 1);
        break;
    case ROBO_CRUISE:
        robo_cruise(work, record);
        break;
    case ROBO_WATCH:
        robo_watch(task, record);
        break;
    case ROBO_AIM:
        robo_aim(work, record);
        break;
    case ROBO_SWOOP:
        robo_swoop(task, record);
        sound_play_at_for(SE_SWOOP, (int)work, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 1);
        break;
    case ROBO_BRAKE:
        robo_brake(work, record);
        break;
    case ROBO_CLIMB:
        robo_climb(task, record);
        break;
    case ROBO_SWING:
        robo_swing(work, record);
        break;
    case ROBO_FALL:
        robo_fall(work, record);
        break;
    case ROBO_BEATEN:
        robo_explode(work, record);
        robo_finish(task, work, record);
        return;
    }
    enemy_save_pos(work, record);
    work->timer++;
    if (work->flags & ROBO_SOLID) {
        collision_register(work);
    }
    task_step_children(task);
}
