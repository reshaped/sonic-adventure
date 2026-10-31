/* E POLICE, a patrol car that hovers over the road. it chases player 0
 * near its home with its siren lit, then drifts back */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "animal/kinds.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/polygon.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/explosion.h"
#include "effect/panlight.h"
#include "enemy/enemy.h"
#include "object/placement.h"
#include "path/path.h"
#include "player/knuckles.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg04/paths.h"
#include "stg04/police.h"
#include "stg04/traffic.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 4
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 5
#define TEXT_9_LENGTH 8
#define TEXT_10_LENGTH 8
#define TEXT_11_LENGTH 7
#define TEXT_12_LENGTH 8
#define TEXT_13_LENGTH 5
#define TEXT_14_LENGTH 10
#define TEXT_15_LENGTH 12
#define TEXT_16_LENGTH 7
#define TEXT_17_LENGTH 11
#define TEXT_18_LENGTH 7
#define TEXT_19_LENGTH 14
#define TEXT_20_LENGTH 4
#define TEXT_21_LENGTH 10
#include "stg04/police/text.inc"

/* the models of the car, its driver and the siren, and their texlist. this
 * file's code reads them and the car's one action. 1ST_READ.BIN's
 * highway_pvms names the texlist, for NISEPAT's textures */

/* police_texlist's names. the last, the headlight's spot, is
 * police_spot_texlist's only one */
static NJS_TEXNAME police_texnames[21] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0},
};
NJS_TEXLIST police_texlist = {
#include "stg04/police/police_texlist.inc"
};
static NJS_MATERIAL dat_0ca791bc[5] = {
#include "stg04/police/dat_0ca791bc.inc"
};
static Sint16 dat_0ca79220[24] = {
#include "stg04/police/dat_0ca79220.inc"
};
static Sint16 dat_0ca79250[15] = {
#include "stg04/police/dat_0ca79250.inc"
};
static Sint16 dat_0ca7926e[26] = {
#include "stg04/police/dat_0ca7926e.inc"
};
static Sint16 dat_0ca792a2[33] = {
#include "stg04/police/dat_0ca792a2.inc"
};
static Sint16 dat_0ca792e4[22] = {
#include "stg04/police/dat_0ca792e4.inc"
};
static NJS_COLOR dat_0ca79310[20] = {
#include "stg04/police/dat_0ca79310.inc"
};
static NJS_COLOR dat_0ca79360[12] = {
#include "stg04/police/dat_0ca79360.inc"
};
static NJS_COLOR dat_0ca79390[22] = {
#include "stg04/police/dat_0ca79390.inc"
};
static NJS_COLOR dat_0ca793e8[30] = {
#include "stg04/police/dat_0ca793e8.inc"
};
static NJS_COLOR dat_0ca79460[18] = {
#include "stg04/police/dat_0ca79460.inc"
};
static NJS_MESHSET dat_0ca794a8[5] = {
#include "stg04/police/dat_0ca794a8.inc"
};
static NJS_POINT3 dat_0ca79520[44] = {
#include "stg04/police/dat_0ca79520.inc"
};
static NJS_VECTOR dat_0ca79730[44] = {
#include "stg04/police/dat_0ca79730.inc"
};
static NJS_MODEL dat_0ca79940 = {
#include "stg04/police/dat_0ca79940.inc"
};
static NJS_OBJECT police_driver_object = {
#include "stg04/police/police_driver_object.inc"
};
/* the materials of the siren's two models, one to a model */
static NJS_MATERIAL police_siren_1_materials[1] = {
#include "stg04/police/police_siren_1_materials.inc"
};
static Sint16 dat_0ca799b0[6] = {
#include "stg04/police/dat_0ca799b0.inc"
};
static NJS_COLOR dat_0ca799bc[4] = {
#include "stg04/police/dat_0ca799bc.inc"
};
static NJS_MESHSET dat_0ca799cc[1] = {
#include "stg04/police/dat_0ca799cc.inc"
};
static NJS_POINT3 dat_0ca799e4[4] = {
#include "stg04/police/dat_0ca799e4.inc"
};
static NJS_VECTOR dat_0ca79a14[4] = {
#include "stg04/police/dat_0ca79a14.inc"
};
static NJS_MODEL dat_0ca79a44 = {
#include "stg04/police/dat_0ca79a44.inc"
};
static NJS_OBJECT dat_0ca79a6c = {
#include "stg04/police/dat_0ca79a6c.inc"
};
static NJS_MATERIAL police_siren_2_materials[1] = {
#include "stg04/police/police_siren_2_materials.inc"
};
static Sint16 dat_0ca79ab4[6] = {
#include "stg04/police/dat_0ca79ab4.inc"
};
static NJS_COLOR dat_0ca79ac0[4] = {
#include "stg04/police/dat_0ca79ac0.inc"
};
static NJS_MESHSET dat_0ca79ad0[1] = {
#include "stg04/police/dat_0ca79ad0.inc"
};
static NJS_POINT3 dat_0ca79ae8[4] = {
#include "stg04/police/dat_0ca79ae8.inc"
};
static NJS_VECTOR dat_0ca79b18[4] = {
#include "stg04/police/dat_0ca79b18.inc"
};
static NJS_MODEL dat_0ca79b48 = {
#include "stg04/police/dat_0ca79b48.inc"
};
static NJS_OBJECT dat_0ca79b70 = {
#include "stg04/police/dat_0ca79b70.inc"
};
static NJS_MATERIAL dat_0ca79ba4[17] = {
#include "stg04/police/dat_0ca79ba4.inc"
};
static Sint16 dat_0ca79cf8[5] = {
#include "stg04/police/dat_0ca79cf8.inc"
};
static Sint16 dat_0ca79d02[39] = {
#include "stg04/police/dat_0ca79d02.inc"
};
static Sint16 dat_0ca79d50[38] = {
#include "stg04/police/dat_0ca79d50.inc"
};
static Sint16 dat_0ca79d9c[12] = {
#include "stg04/police/dat_0ca79d9c.inc"
};
static Sint16 dat_0ca79db4[7] = {
#include "stg04/police/dat_0ca79db4.inc"
};
static Sint16 dat_0ca79dc2[28] = {
#include "stg04/police/dat_0ca79dc2.inc"
};
static Sint16 dat_0ca79dfa[81] = {
#include "stg04/police/dat_0ca79dfa.inc"
};
static Sint16 dat_0ca79e9c[38] = {
#include "stg04/police/dat_0ca79e9c.inc"
};
static Sint16 dat_0ca79ee8[15] = {
#include "stg04/police/dat_0ca79ee8.inc"
};
static Sint16 dat_0ca79f06[18] = {
#include "stg04/police/dat_0ca79f06.inc"
};
static Sint16 dat_0ca79f2a[42] = {
#include "stg04/police/dat_0ca79f2a.inc"
};
static Sint16 dat_0ca79f7e[32] = {
#include "stg04/police/dat_0ca79f7e.inc"
};
static Sint16 dat_0ca79fbe[37] = {
#include "stg04/police/dat_0ca79fbe.inc"
};
static Sint16 dat_0ca7a008[19] = {
#include "stg04/police/dat_0ca7a008.inc"
};
static Sint16 dat_0ca7a02e[26] = {
#include "stg04/police/dat_0ca7a02e.inc"
};
static Sint16 dat_0ca7a062[12] = {
#include "stg04/police/dat_0ca7a062.inc"
};
static Sint16 dat_0ca7a07a[7] = {
#include "stg04/police/dat_0ca7a07a.inc"
};
static NJS_COLOR dat_0ca7a088[4] = {
#include "stg04/police/dat_0ca7a088.inc"
};
static NJS_COLOR dat_0ca7a098[34] = {
#include "stg04/police/dat_0ca7a098.inc"
};
static NJS_COLOR dat_0ca7a120[32] = {
#include "stg04/police/dat_0ca7a120.inc"
};
static NJS_COLOR dat_0ca7a1a0[10] = {
#include "stg04/police/dat_0ca7a1a0.inc"
};
static NJS_COLOR dat_0ca7a1c8[6] = {
#include "stg04/police/dat_0ca7a1c8.inc"
};
static NJS_COLOR dat_0ca7a1e0[24] = {
#include "stg04/police/dat_0ca7a1e0.inc"
};
static NJS_COLOR dat_0ca7a240[66] = {
#include "stg04/police/dat_0ca7a240.inc"
};
static NJS_COLOR dat_0ca7a348[30] = {
#include "stg04/police/dat_0ca7a348.inc"
};
static NJS_COLOR dat_0ca7a3c0[12] = {
#include "stg04/police/dat_0ca7a3c0.inc"
};
static NJS_COLOR dat_0ca7a3f0[14] = {
#include "stg04/police/dat_0ca7a3f0.inc"
};
static NJS_COLOR dat_0ca7a428[28] = {
#include "stg04/police/dat_0ca7a428.inc"
};
static NJS_COLOR dat_0ca7a498[34] = {
#include "stg04/police/dat_0ca7a498.inc"
};
static NJS_COLOR dat_0ca7a520[18] = {
#include "stg04/police/dat_0ca7a520.inc"
};
static NJS_COLOR dat_0ca7a568[20] = {
#include "stg04/police/dat_0ca7a568.inc"
};
static NJS_COLOR dat_0ca7a5b8[10] = {
#include "stg04/police/dat_0ca7a5b8.inc"
};
static NJS_COLOR dat_0ca7a5e0[6] = {
#include "stg04/police/dat_0ca7a5e0.inc"
};
static NJS_MESHSET dat_0ca7a5f8[17] = {
#include "stg04/police/dat_0ca7a5f8.inc"
};
static NJS_POINT3 dat_0ca7a790[196] = {
#include "stg04/police/dat_0ca7a790.inc"
};
static NJS_VECTOR dat_0ca7b0c0[196] = {
#include "stg04/police/dat_0ca7b0c0.inc"
};
static NJS_MODEL dat_0ca7b9f0 = {
#include "stg04/police/dat_0ca7b9f0.inc"
};
static NJS_OBJECT police_object = {
#include "stg04/police/police_object.inc"
};
static NJS_MKEY_A dat_0ca7ba4c[9] = {
#include "stg04/police/dat_0ca7ba4c.inc"
};
static NJS_MKEY_A dat_0ca7badc[9] = {
#include "stg04/police/dat_0ca7badc.inc"
};
static NJS_MKEY_A dat_0ca7bb6c[9] = {
#include "stg04/police/dat_0ca7bb6c.inc"
};
static NJS_MKEY_A dat_0ca7bbfc[9] = {
#include "stg04/police/dat_0ca7bbfc.inc"
};
/* the motion's data, entries for 30 nodes. police_object's three nodes
 * read the first three, which hold no keys. nothing reads the 27 after
 * them. four of those point at the key arrays dat_0ca7ba4c to
 * dat_0ca7bbfc */
static NJS_MDATA2 dat_0ca7bc8c[30] = {
#include "stg04/police/dat_0ca7bc8c.inc"
};
static NJS_MOTION dat_0ca7be6c = {
#include "stg04/police/dat_0ca7be6c.inc"
};
static NJS_ACTION police_action = {
#include "stg04/police/police_action.inc"
};

/* the car's modes, in its work->mode, which skip 3. police_step runs each
 * by the function of its name, and POLICE_GONE by blowing the car up */
enum police_mode {
    POLICE_WAIT = 0,
    POLICE_START = 1,
    POLICE_CHASE = 2,
    POLICE_RETURN = 4,
    POLICE_HIT = 5,
    POLICE_GONE = 6
};

/* the car's bits in work->flags. police_init sets POLICE_CHASING, and so
 * does player 0 coming within PLAYER_RANGE of the car. police_choose_target
 * clears it while the car is out of its chase range of home */
#define POLICE_CHASING 0x400
#define POLICE_SOLID 0x800 /* police_step registers the collision record */
/* police_init sets it if scl.z, the set editor's CHASE RANGE, is 0, and
 * clears it otherwise. no code here reads it */
#define POLICE_UNK_1000 0x1000

/* the lights' strength, from 0 to 1 */
#define POLICE_GLOW(record) ((record)->motion.drive)
/* the height of the floor the car last hovered over */
#define POLICE_FLOOR(record) ((record)->motion.slip)
/* the hover's thrust, from THRUST_IDLE to THRUST_FULL */
#define POLICE_THRUST(record) ((record)->motion.gravity)
/* one of enemy_target_near_player's four spots near player 0, which
 * police_start picks. police_chase sets the spot as the target while the
 * car is off screen. police_choose_target then replaces that target, so the
 * spot never steers the car */
#define POLICE_SPOT(record) ((record)->unk_4c)
/* the car's pitch and bank, turns about its z and x after its heading */
#define POLICE_PITCH(record) ((record)->phase)
#define POLICE_BANK(record) ((record)->sight_angle)
/* the car's squared speed across, as the step before left it */
#define POLICE_SPEED2(record) ((record)->sight_distance)
/* the exhaust's throttle. police_accelerate moves it toward a value that
 * rises the straighter the car heads for its aim. the exhaust's panlights
 * brighten and shrink more slowly as it rises */
#define POLICE_THROTTLE(record) ((record)->near_distance)
/* the siren's turn, which police_step advances by SIREN_SPEED a step */
#define POLICE_SIREN(record) ((record)->angles[0])
/* the driver's look, a turn from the car's heading */
#define POLICE_LOOK(record) ((record)->angles[1])
/* the place of the Master Emerald piece the car carries in Knuckles' hunt.
 * police_init takes it from ang.z, the set editor's EMERALD */
#define POLICE_PIECE(record) ((record)->angles[3])

/* the hover's thrust idles at THRUST_IDLE, and police_start raises it to
 * THRUST_FULL */
#define THRUST_IDLE 2.0F
#define THRUST_FULL 3.3F

/* destroyed beyond 700 units of the camera and player 0 */
#define POLICE_RANGE (700.0F * 700.0F)

/* the car chases player 0 within PLAYER_RANGE of it, squared here. it
 * chases only within its chase range of home, CHASE_RANGE plus scl.z, the
 * set editor's CHASE RANGE */
#define CHASE_RANGE 300.0F
#define PLAYER_RANGE (200.0F * 200.0F)
/* with player 0 out of reach, the car returns within RETURN_NEAR of home,
 * and waits within HOME_NEAR */
#define RETURN_NEAR 30.0F
#define HOME_NEAR 10.0F
/* a car that isn't waiting blows dust off the floor while its clearance is
 * under DOWNWASH_REACH. police_hover's clearance is the car's height over
 * the floor, plus 3.5 */
#define DOWNWASH_REACH 12.8F

/* police_start chases once the timer passes START_STEPS. police_return
 * waits once it passes RETURN_STEPS */
#define START_STEPS 20
#define RETURN_STEPS 150

/* the banks police_turn_hard and police_turn lean the car toward, and how
 * fast each banks and levels, a step. AIM_NEAR is how near the aim the
 * heading must come for them to level the car */
#define BANK_HARD 0x1a00
#define BANK_HARD_SPEED 96
#define LEVEL_HARD_SPEED 192
#define BANK_SOFT 0x800
#define BANK_SOFT_SPEED 48
#define LEVEL_SOFT_SPEED 128
#define AIM_NEAR 0x80

/* the bank pushes the car sideways by BANK_PUSH for each degree of bank, up
 * to BANK_PUSH_DEGREES each way */
#define BANK_PUSH 0.0028F
#define BANK_PUSH_DEGREES 10.0F

/* a touch of an object adds TOUCH_LIFT to the car's upward acceleration.
 * with player 0 within TOUCH_NEAR of the object, it adds TOUCH_LIFT_NEAR
 * instead */
#define TOUCH_LIFT 0.2F
#define TOUCH_LIFT_NEAR 0.02F
#define TOUCH_NEAR 50.0F

/* the sound police_step plays while the car starts, chases and returns,
 * within SIREN_RADIUS of the camera. its name is inferred from those
 * modes */
#define SE_SIREN 107
#define SIREN_RADIUS 200.0F
/* the siren's turn a step */
#define SIREN_SPEED 0x700

/* how far the driver's look turns a step */
#define LOOK_SPEED 0x360

/* the land tilts the headlight's spot by at most 20 degrees each way. each
 * slope, a quarter turn on, is held from SPOT_TILT_LOW to SPOT_TILT_HIGH */
#define SPOT_TILT_LOW NJM_DEG_ANG(70)
#define SPOT_TILT_HIGH NJM_DEG_ANG(110)

/* in the siren materials' attrflags, the bit that turns NJD_DA_ONE into
 * NJD_DA_INV_SRC. set, as on the disc, it blends the siren's models by
 * inverse source alpha. clear, they add their light over what's behind
 * them */
#define SIREN_DARK (NJD_DA_INV_SRC & ~NJD_DA_ONE)

struct police_vars {
    Uint8 unk_00;
    /* the steps since the hit that beat the car, which ENEMY_MARK_BEATEN
     * zeroes */
    Uint8 since_hit;
};

#define VARS(work) ((struct police_vars *)&(work)->vars)

static void police_step(struct task *task);
static void police_draw(struct task *task);
static void police_destroy(struct task *task);

/* the car's two sides and its driver, which hurt players. a wider ball
 * pushes items and enemies away, and no attack passes its guard */
/* clang-format off */
static struct collision_shape police_shapes[4] = {
    {SHAPE_CAPSULE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_PLAYERS),
     {0.5F, 2.2F, -4.0F}, 4.5F, 6.5F, 0.0F, {0, 0, 0x3c00}, 0, NULL},
    {SHAPE_CAPSULE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_PLAYERS),
     {0.5F, 2.2F, 4.0F}, 4.5F, 6.5F, 0.0F, {0, 0, 0x3c00}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_PLAYERS),
     {0.0F, 8.0F, 0.0F}, 3.0F, 0.0F, 0.0F, {0, 0, 0x4000}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, GUARD(3) | SHAPE_HURTS_PLAYERS,
     PUSH_ENEMIES | PUSH_FIRM(PUSH_OBJECTS | PUSH_OTHERS),
     {0.0F, 5.0F, 0.0F}, 12.0F, 0.0F, 0.0F, {0, 0, 0x4000}, 0, NULL},
};
/* clang-format on */

Float police_stretches[POLICE_STRETCHES][2] = {
    {4150.0F, 2640.0F}, {2640.0F, 2500.0F}, {2500.0F, 2020.0F},
    {2020.0F, 1800.0F}, {1800.0F, 1010.0F}, {1010.0F, 850.0F},
    {850.0F, 0.0F},
};

/* highway_act2_route in nine stretches: police_stretches with its first
 * stretch cut at 3320 and 3280. nothing reads it */
static Float police_nine_stretches[9][2] = {
    {4150.0F, 3320.0F}, {3320.0F, 3280.0F}, {3280.0F, 2640.0F},
    {2640.0F, 2500.0F}, {2500.0F, 2020.0F}, {2020.0F, 1800.0F},
    {1800.0F, 1010.0F}, {1010.0F, 850.0F},  {850.0F, 0.0F},
};

/* the panlight texture alone, and a glow of it, for the headlight and the
 * siren's lamps */
static NJS_TEXLIST police_glow_texlist = {
#include "stg04/police/police_glow_texlist.inc"
};
static NJS_TEXANIM police_glow_texanim[1] = {
#include "stg04/police/police_glow_texanim.inc"
};
static NJS_SPRITE police_glow = {
#include "stg04/police/police_glow.inc"
};

/* the headlight's spot on the land */
static NJS_TEXLIST police_spot_texlist = {
#include "stg04/police/police_spot_texlist.inc"
};
static NJS_TEXANIM police_spot_texanim[1] = {
#include "stg04/police/police_spot_texanim.inc"
};
static NJS_SPRITE police_spot = {
#include "stg04/police/police_spot.inc"
};

/* the dust it blows off the floor, and its exhaust's two panlights */
static struct dust_params police_downwash = {
    1.0F,
    0.03F,
    0.19F,
    0.05F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.15F, 0.8F, 0.8F, 1.0F},
};
static struct dust_params police_exhaust = {
    0.75F,
    0.5F,
    3.1F,
    0.05F,
    0.0F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.5F, 0.0F},
    {1.0F, 0.3F, 0.3F, 1.0F},
};
static struct dust_params police_exhaust_low = {
    0.4F,
    0.4F,
    3.0F,
    0.04F,
    0.0F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.5F, 0.0F},
    {1.0F, 0.3F, 0.3F, 1.0F},
};

/* the siren's four lamps on the car, and how far each is turned from the
 * siren's turn */
static NJS_POINT3 police_lamps[4] = {
    {-4.7F, 9.5F, 5.5F},
    {-4.7F, 9.0F, 3.5F},
    {-4.7F, 9.0F, -3.5F},
    {-4.7F, 9.5F, -5.5F},
};
static Angle police_lamp_phases[4] = {0, 0x4000, 0x2000, 0x6000};

/* the car's matrix, and its rotation alone, as its lights last drew it */
static NJS_MATRIX police_matrix;
static NJS_MATRIX police_rotation;

void police_init(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;
    struct angles slope;
    Float along;
    int i;
    int stretch;

    collision_init(task, police_shapes, COLLISION_ENEMY, 1, 4);
    record = enemy_alloc_record(task, work);
    work->mode = POLICE_WAIT;
    record->flags |= ENEMY_SHADOW;
    work->tick = 0;
    work->timer = 0;
    POLICE_FLOOR(record) = 0.0F;
    POLICE_THRUST(record) = THRUST_IDLE;
    POLICE_BANK(record) = 0;
    POLICE_PITCH(record) = 0;
    POLICE_SPEED2(record) = 0.0F;
    POLICE_GLOW(record) = 0.0F;
    POLICE_THROTTLE(record) = 0.0F;
    POLICE_LOOK(record) = work->ang.y;
    record->aim_y = work->ang.y;
    record->frame = 0.0F;
    record->heading = work->ang.y;
    record->probe_offset.x = 0.0F;
    record->probe_offset.y = 10.0F;
    record->probe_offset.z = 0.0F;
    record->top = 5.0F;
    record->radius = 7.0F;
    record->bottom = -5.0F;
    record->wall_bounce = 1.0F;
    record->floor_bounce = 1.0F;
    record->friction = 1.0F;
    record->unk_c4 = 1.0F;
    record->shadow_size = 1.6F;
    record->shadow_ratio = 1.0F;
    POLICE_PIECE(record) = work->ang.z;
    work->pos.y = shadow_draw_slope(work, 1.0F, &slope) + 0.5F;
    work->ang.x = slope.x;
    work->ang.z = slope.z;
    record->unk_1e4 = work->pos.y;
    POLICE_FLOOR(record) = work->pos.y;
    if (work->scl.z == 0.0F) {
        work->flags |= POLICE_UNK_1000;
    } else {
        work->flags &= ~POLICE_UNK_1000;
    }
    work->scl.x = 1.0F;
    work->scl.y = 1.0F;
    record->scale_speed = 0.0F;
    path_find_nearest(&highway_act2_route, &work->pos, NULL, &along);
    stretch = -1;
    for (i = 0; i < POLICE_STRETCHES; ++i) {
        if (police_stretches[i][1] <= along &&
            police_stretches[i][0] >= along) {
            stretch = i;
            break;
        }
    }
    work->submode = stretch;
    enemy_set_home(work, record);
    task->destructor = police_destroy;
    task->step = police_step;
    task->draw = police_draw;
    work->flags |= POLICE_SOLID;
    work->flags |= POLICE_CHASING;
}

/* turn toward the target, and bank toward BANK_HARD on the aim's side by
 * BANK_HARD_SPEED a step. within AIM_NEAR of the aim, level out by
 * LEVEL_HARD_SPEED a step */
static void police_turn_hard(struct task_work *work,
                             struct lockon_target *record)
{
    Angle heading;
    Angle aim;
    Angle away;

    enemy_turn_to_target(work, record);
    /* @hack spelling: the heading read before the aim, which holds it in a
     * 4-byte frame (ADD #-4,R15 at 0x0c90ca66), read at 0x0c90ca72; without it,
     * 64 of 66 words differ */
    heading = work->ang.y;
    aim = (Angle)(atan2f(record->target.x - work->pos.x,
                         record->target.z - work->pos.z) *
                  ANGLE_PER_RADIAN) -
          NJM_DEG_ANG(90);
    away = angle_difference(aim, heading);
    if (away < -AIM_NEAR) {
        POLICE_BANK(record) =
            angle_approach(POLICE_BANK(record), -BANK_HARD, BANK_HARD_SPEED);
        /* @hack spelling: a return where an else would do, which leaves BRA's
         * delay slot at 0x0c90cabc empty; as else if (away > AIM_NEAR), 23 of
         * 66 words differ */
        return;
    }
    if (away > AIM_NEAR) {
        POLICE_BANK(record) =
            angle_approach(POLICE_BANK(record), BANK_HARD, BANK_HARD_SPEED);
    } else {
        POLICE_BANK(record) =
            angle_approach(POLICE_BANK(record), 0, LEVEL_HARD_SPEED);
    }
}

/* as police_turn_hard, toward BANK_SOFT by BANK_SOFT_SPEED, and level by
 * LEVEL_SOFT_SPEED */
static void police_turn(struct task_work *work, struct lockon_target *record)
{
    Angle heading;
    Angle aim;
    Angle away;

    enemy_turn_to_target(work, record);
    /* @hack spelling: the heading read before the aim, which holds it in a
     * 4-byte frame (ADD #-4,R15 at 0x0c90cb26), read at 0x0c90cb32; without it,
     * 64 of 66 words differ */
    heading = work->ang.y;
    aim = (Angle)(atan2f(record->target.x - work->pos.x,
                         record->target.z - work->pos.z) *
                  ANGLE_PER_RADIAN) -
          NJM_DEG_ANG(90);
    away = angle_difference(aim, heading);
    if (away < -AIM_NEAR) {
        POLICE_BANK(record) =
            angle_approach(POLICE_BANK(record), -BANK_SOFT, BANK_SOFT_SPEED);
        /* @hack spelling: a return where an else would do, which leaves BRA's
         * delay slot at 0x0c90cb7c empty; as else if (away > AIM_NEAR), 23 of
         * 66 words differ */
        return;
    }
    if (away > AIM_NEAR) {
        POLICE_BANK(record) =
            angle_approach(POLICE_BANK(record), BANK_SOFT, BANK_SOFT_SPEED);
    } else {
        POLICE_BANK(record) =
            angle_approach(POLICE_BANK(record), 0, LEVEL_SOFT_SPEED);
    }
}

/* thrust toward the target, less the farther the heading is off the aim.
 * set turn_speed and POLICE_THROTTLE too. each thrust is a speed times
 * 0.51, written as that product. SHC folds it to the image's constant,
 * which a literal of the product misses in its last bits */
static void police_accelerate(struct task_work *work,
                              struct lockon_target *record)
{
    Float distance;
    Float thrust;
    Float push;
    Float throttle;
    int away;

    distance = njSqrt(enemy_target_distance2(work, record));
    away = enemy_aim_at_target(work, record);
    if (distance > 140.0F) {
        thrust = 0.15F * 0.51F;
        record->turn_speed = 0x140;
    } else if (distance > 70.0F) {
        thrust = 0.14F * 0.51F;
        record->turn_speed = 0x160;
    } else if (distance > 28.0F) {
        if (record->target.y - traffic_act2_nearest.pos.y < 10.0F) {
            thrust = 0.065F * 0.51F;
            record->turn_speed = 0x1b0;
        } else {
            thrust = 0.15F * 0.51F;
            record->turn_speed = 0x1b0;
        }
    } else {
        if (record->target.y - traffic_act2_nearest.pos.y < 10.0F) {
            thrust = 0.05F * 0.51F;
            record->turn_speed = 0x210;
        } else {
            thrust = 0.15F * 0.51F;
            record->motion.accel.y -= 0.08F;
            record->turn_speed = 0x210;
        }
    }
    /* the bank in degrees, from -180 to 180 */
    push = (Uint16)POLICE_BANK(record) * (360.0F / 65536.0F);
    if (push > 180.0F) {
        push -= 360.0F;
    }
    if (push > BANK_PUSH_DEGREES) {
        push = BANK_PUSH_DEGREES;
    } else if (push < -BANK_PUSH_DEGREES) {
        push = -BANK_PUSH_DEGREES;
    }
    push *= BANK_PUSH;
    if (away > 0x6000) {
        thrust *= 0.2F;
    } else if (away > 0x4000) {
        thrust *= 0.4F;
    } else if (away > 0x2000) {
        thrust *= 0.7F;
    } else {
        /* @hack redundant: thrust times one, which the image multiplies too,
         * FLDI1 and FMUL at 0x0c90cd80 and 0x0c90cd82; without it, 91 of 209
         * words differ */
        thrust *= 1.0F;
    }
    /* the throttle to move toward, highest when the car heads straight at
     * its aim */
    throttle = (3.15F - (away * (2.0F * NJD_PI / 65536.0F))) * 0.3F;
    if (POLICE_THROTTLE(record) < throttle) {
        POLICE_THROTTLE(record) += 0.1F;
    } else {
        POLICE_THROTTLE(record) -= 0.1F;
    }
    record->motion.accel.x +=
        (thrust * njCos(work->ang.y)) + (push * njSin(work->ang.y));
    record->motion.accel.z +=
        (thrust * njSin(-work->ang.y)) + (push * njCos(work->ang.y));
    if (!(work->flags & POLICE_CHASING)) {
        record->motion.accel.x *= 0.35F;
        record->motion.accel.z *= 0.35F;
    }
}

/* in its chase range of home, the car aims at player 0 within PLAYER_RANGE
 * of it. it then sets POLICE_CHASING. otherwise it aims at home, and a car
 * that isn't waiting goes to POLICE_RETURN within RETURN_NEAR of home. out
 * of the chase range, the car aims at home and clears POLICE_CHASING */
static void police_choose_target(struct task_work *work,
                                 struct lockon_target *record)
{
    struct task_work *player = player_work[0];

    if (enemy_home_distance2(work, record) <
        (work->scl.z + CHASE_RANGE) * (work->scl.z + CHASE_RANGE)) {
        if (enemy_player_distance2(work, 0) < PLAYER_RANGE) {
            work->flags |= POLICE_CHASING;
            record->target = player->pos;
            /* @hack spelling: a return where an else would do, which leaves
             * BRA's delay slot at 0x0c90ce98 empty; as an else around the rest
             * of the block, 53 of 93 words differ */
            return;
        }
        record->target = record->home;
        if (work->mode != POLICE_WAIT &&
            enemy_home_distance2(work, record) < RETURN_NEAR * RETURN_NEAR) {
            ENEMY_SET_MODE(work, record, POLICE_RETURN);
        }
    } else {
        record->target = record->home;
        work->flags &= ~POLICE_CHASING;
    }
}

/* push the car up when it touches an object, by TOUCH_LIFT or
 * TOUCH_LIFT_NEAR. a touch that enemy_take_hit counts as a hit beats the
 * car, and sets POLICE_HIT */
static void police_touch(struct task_work *work, struct lockon_target *record)
{
    struct task_work *player = player_work[0];
    struct collision_work *collision;
    struct collision_work *hit;
    struct task_work *other;
    /* @hack spelling: x's difference through a local, which keeps player 0's
     * work in R4 and the other's in R6, loaded at 0x0c90cf2c and 0x0c90cf46;
     * without it, the two trade, and 10 of 72 words differ */
    Float dx;

    /* @hack redundant: a read of hit_shape, which the image loads at 0x0c90cf32
     * and never uses; without it, 70 of 72 words differ */
    (void)work->collision->hit_shape;
    collision = work->collision;
    hit = collision->hit;
    if (collision->flags & COLLISION_HIT) {
        other = hit->owner->work;
        if (hit->kind == COLLISION_OBJECT) {
            dx = player->pos.x - other->pos.x;
            if ((dx * dx) +
                    ((player->pos.y - other->pos.y) *
                     (player->pos.y - other->pos.y)) +
                    ((player->pos.z - other->pos.z) *
                     (player->pos.z - other->pos.z)) <
                TOUCH_NEAR * TOUCH_NEAR) {
                record->motion.accel.y += TOUCH_LIFT_NEAR;
            } else {
                record->motion.accel.y += TOUCH_LIFT;
            }
        }
        if (enemy_take_hit(work, record)) {
            work->mode = POLICE_HIT;
            ENEMY_MARK_BEATEN(work);
            work->flags &= ~COLLISION_DAMAGED;
        }
    }
}

/* add drag and gravity to the speed, and the speed to pos. while the squared
 * speed across grows by more than 0.01 a step, the car pitches one way.
 * while it falls by 0.03 or more, the car pitches the other way. between the
 * two, it levels */
static void police_fly(struct task_work *work, struct lockon_target *record)
{
    Float speed2;
    Float change;

    record->motion.accel.x += -record->motion.speed.x * 0.016F;
    record->motion.accel.y += -record->motion.speed.y * 0.05F;
    record->motion.accel.z += -record->motion.speed.z * 0.016F;
    record->motion.speed.x += record->motion.accel.x;
    record->motion.speed.y += record->motion.accel.y - 0.22F;
    record->motion.speed.z += record->motion.accel.z;
    work->pos.x += record->motion.speed.x;
    work->pos.y += record->motion.speed.y;
    work->pos.z += record->motion.speed.z;
    VECTOR_CLEAR(&record->motion.accel);
    speed2 = ACROSS_SQUARED(&record->motion.speed);
    change = speed2 - POLICE_SPEED2(record);
    if (change > 0.01F) {
        POLICE_PITCH(record) =
            angle_approach(POLICE_PITCH(record), -0x900, 112);
    } else if (change < 0.01F && change > -0.03F) {
        POLICE_PITCH(record) = angle_approach(POLICE_PITCH(record), 0, 160);
    } else {
        POLICE_PITCH(record) = angle_approach(POLICE_PITCH(record), 0xf00, 112);
    }
    POLICE_SPEED2(record) = speed2;
}

/* as police_fly, with a pull toward home across, more drag across, and
 * each slow speed zeroed. the car pitches the other way once its squared
 * speed across falls by 0.01, not 0.03. within HOME_NEAR of home, the car
 * waits */
static void police_fly_home(struct task_work *work,
                            struct lockon_target *record)
{
    Float speed2;
    Float change;

    if (work->pos.x < record->home.x) {
        record->motion.accel.x += 0.01F;
    } else {
        record->motion.accel.x -= 0.01F;
    }
    if (work->pos.z < record->home.z) {
        record->motion.accel.z += 0.01F;
    } else {
        record->motion.accel.z -= 0.01F;
    }
    record->motion.accel.x += -record->motion.speed.x * 0.048F;
    if (njAbs(record->motion.speed.x) < 0.001F) {
        record->motion.speed.x = 0.0F;
    }
    record->motion.accel.y += -record->motion.speed.y * 0.05F;
    if (njAbs(record->motion.speed.y) < 0.01F) {
        record->motion.speed.y = 0.0F;
    }
    record->motion.accel.z += -record->motion.speed.z * 0.048F;
    if (njAbs(record->motion.speed.z) < 0.001F) {
        record->motion.speed.z = 0.0F;
    }
    record->motion.speed.x += record->motion.accel.x;
    record->motion.speed.y += record->motion.accel.y - 0.22F;
    record->motion.speed.z += record->motion.accel.z;
    work->pos.x += record->motion.speed.x;
    work->pos.y += record->motion.speed.y;
    work->pos.z += record->motion.speed.z;
    VECTOR_CLEAR(&record->motion.accel);
    speed2 = ACROSS_SQUARED(&record->motion.speed);
    change = speed2 - POLICE_SPEED2(record);
    if (change > 0.01F) {
        POLICE_PITCH(record) =
            angle_approach(POLICE_PITCH(record), -0x900, 112);
    } else if (change < 0.01F && change > -0.01F) {
        POLICE_PITCH(record) = angle_approach(POLICE_PITCH(record), 0, 160);
    } else {
        POLICE_PITCH(record) = angle_approach(POLICE_PITCH(record), 0xf00, 112);
    }
    POLICE_SPEED2(record) = speed2;
    if (enemy_home_distance2(work, record) < HOME_NEAR * HOME_NEAR) {
        /* @hack spelling: the mode set by hand, which keeps FR6 and FR7 as at
         * 0x0c90d18e and 0x0c90d1a0; without it, as ENEMY_SET_MODE's comma
         * expression, the two trade, and 6 of 207 words differ */
        record->last_mode = work->mode;
        work->mode = POLICE_WAIT;
        work->timer = 0;
    }
}

/* draw the car's glows in a frame's first step, added over what's behind
 * them. they are the headlight, its spot on the land, the siren's four
 * lamps and the exhaust's two panlights. POLICE_GLOW scales them all. the
 * headlight and the lamps brighten as they face the camera. where the land
 * at the spot is above the headlight, the function draws the headlight
 * alone. it then returns with the blending and the constant material still
 * as it set them */
static void police_draw_lights(struct task_work *work,
                               struct lockon_target *record)
{
    NJS_POINT3 origin;
    NJS_POINT3 glow;
    NJS_POINT3 point;
    NJS_VECTOR direction;
    NJS_VECTOR facing;
    NJS_VECTOR view;
    Float brightness;
    Float scale;
    Float dot;
    Float height;
    Float distance;
    int i;

    if (frame_step) {
        return;
    }
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_INVSRCCOLOR);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetTexture(&police_glow_texlist);
    view.x = work->pos.x - camera_work->pos.x;
    view.y = work->pos.y - camera_work->pos.y;
    view.z = work->pos.z - camera_work->pos.z;
    njUnitVector(&view);
    direction.x = 1.0F;
    direction.y = -0.1F;
    direction.z = 0.0F;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, POLICE_PITCH(record));
    TURN(njRotateX, POLICE_BANK(record));
    njCalcVector(NULL, &direction, &facing);
    njGetMatrix(&police_rotation);
    njPopMatrix(1);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    njMultiMatrix(NULL, &police_rotation);
    njGetMatrix(&police_matrix);
    direction.x = njSin(camera_work->ang.y);
    direction.y = 0.0F;
    direction.z = njCos(camera_work->ang.y);
    if (njInnerProduct(&direction, &view) < 0.0F) {
        dot = -njInnerProduct(&facing, &view);
        if (dot < 0.0F) {
            brightness = 0.0F;
        } else {
            dot = dot * dot;
            brightness = (dot / (1.4F - dot)) - 0.2F;
        }
        brightness *= POLICE_GLOW(record);
    } else {
        brightness = 0.0F;
    }
    scale = brightness * 2.5F;
    origin.x = origin.y = origin.z = 0.0F;
    njTranslate(NULL, 11.4F, 2.4F, 0.0F);
    njCalcPoint(NULL, &origin, &point);
    njPopMatrix(1);
    direction.x = camera_work->pos.x - point.x;
    direction.y = camera_work->pos.y - point.y;
    direction.z = camera_work->pos.z - point.z;
    direction.x *= 0.5F;
    direction.y *= 0.5F;
    direction.z *= 0.5F;
    glow.x = point.x + direction.x;
    glow.y = point.y + direction.y;
    glow.z = point.z + direction.z;
    draw_set_material(brightness + 0.1F, 1.0F, 1.0F, 0.95F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &glow);
    TURN(njRotateY, camera_work->ang.y);
    TURN(njRotateX, camera_work->ang.x);
    njScale(NULL, scale, scale, scale);
    njFogDisable();
    njDrawSprite3D(&police_glow, 0, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    njFogEnable();
    njPopMatrix(1);
    njPushMatrix(&police_matrix);
    njTranslate(NULL,
                (njAbs(work->pos.y - POLICE_FLOOR(record)) + 55.0F) * 0.7F,
                2.4F, 0.0F);
    njCalcPoint(NULL, &origin, &point);
    njPopMatrix(1);
    {
        struct angles slope;

        height = ground_nearest_land(point.x, point.y, point.z, &slope);
        slope.x = (Uint16)((Sint16)slope.x + 0x4000);
        if (slope.x < SPOT_TILT_LOW) {
            slope.x = SPOT_TILT_LOW;
        } else if (slope.x > SPOT_TILT_HIGH) {
            slope.x = SPOT_TILT_HIGH;
        }
        slope.x -= 0x4000;
        slope.z = (Uint16)((Sint16)slope.z + 0x4000);
        if (slope.z < SPOT_TILT_LOW) {
            slope.z = SPOT_TILT_LOW;
        } else if (slope.z > SPOT_TILT_HIGH) {
            slope.z = SPOT_TILT_HIGH;
        }
        slope.z -= 0x4000;
        distance = point.y - height;
        if (distance < 0.0F) {
            return;
        }
        brightness = (distance + 30.0F) * 0.03F;
        point.y = height + (rand() * (0.2F * RAND_SCALE)) + 0.1F;
        draw_set_material(POLICE_GLOW(record) * (2.5F - brightness) * 0.3F,
                          1.0F, 1.0F, 0.95F);
        njPushMatrix(NULL);
        njTranslateV(NULL, &point);
        TURN(njRotateZ, slope.z);
        TURN(njRotateX, slope.x);
        TURN(njRotateY, work->ang.y + 0xc000);
        njRotateX(NULL, 0x4000);
        njScale(NULL, brightness * 2.2F, brightness * 3.0F, brightness * 2.2F);
        njDrawSprite3D(&police_spot, 0, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        njPopMatrix(1);
    }
    /* @hack spelling: i = i + 1, which keeps two frame slots, R15+4 and
     * R15+8, as stored at 0x0c90d8f0 and 0x0c90d8f2; without it, the two
     * trade, and 10 of 1806 words differ */
    for (i = 0; i < 4; i = i + 1) {
        direction.x = 1.0F;
        direction.y = 0.0F;
        direction.z = 0.0F;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, police_lamp_phases[i] + POLICE_SIREN(record));
        njCalcVector(NULL, &direction, &facing);
        njPopMatrix(1);
        dot = -njInnerProduct(&facing, &view);
        if (dot < 0.0F) {
            dot *= -1.0F;
        }
        /* @hack spelling: one statement, which keeps two frame slots, R15+32
         * and R15+24, as at 0x0c90dc48 and 0x0c90dc5e; without it, as two, the
         * two trade, and 5 of 1806 words differ */
        brightness = (dot * dot * dot) + 0.1F,
        brightness *= POLICE_GLOW(record);
        scale = brightness * 1.8F;
        origin.x = origin.y = origin.z = 0.0F;
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        TURN(njRotateZ, POLICE_PITCH(record));
        TURN(njRotateX, POLICE_BANK(record));
        njTranslateV(NULL, &police_lamps[i]);
        njCalcPoint(NULL, &origin, &point);
        njPopMatrix(1);
        direction.x = camera_work->pos.x - point.x;
        direction.y = camera_work->pos.y - point.y;
        direction.z = camera_work->pos.z - point.z;
        njUnitVector(&direction);
        glow.x = point.x + (direction.x * 2.0F);
        glow.y = point.y + (direction.y * 2.0F);
        glow.z = point.z + (direction.z * 2.0F);
        draw_set_material(brightness, 1.0F, 0.4F, 0.3F);
        njPushMatrix(NULL);
        njTranslateV(NULL, &glow);
        TURN(njRotateY, camera_work->ang.y);
        TURN(njRotateX, camera_work->ang.x);
        njScale(NULL, scale, scale, scale);
        njFogDisable();
        njDrawSprite3D(&police_glow, 0, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        njFogEnable();
        njPopMatrix(1);
    }
    {
        NJS_POINT3 emission;
        NJS_VECTOR velocity;
        /* @hack redundant: 64 bytes of frame that no code reads, of a type
         * unknown, declared as a Uint8[64]. they make the frame the image's
         * 216 bytes (ADD R0,R15 at 0x0c90d35a, R0 -216); without them, the
         * object's code runs 256 bytes short, and 1692 of the 1806 words of
         * police_draw_lights differ */
        Uint8 unused[64];

        direction.x = -1.0F;
        direction.y = 0.0F;
        direction.z = 0.0F;
        njCalcVector(&police_rotation, &direction, &velocity);
        velocity.x *= 1.4F;
        velocity.y *= 1.4F;
        velocity.z *= 1.4F;
        origin.x = origin.y = origin.z = 0.0F;
        njPushMatrix(&police_matrix);
        TURN(njRotateY, record->turn_speed);
        njTranslate(NULL, -10.5F, 5.6F, 0.0F);
        njCalcPoint(NULL, &origin, &emission);
        njPopMatrix(1);
        police_exhaust.pos = emission;
        police_exhaust.speed.x =
            velocity.x + record->motion.speed.x +
            (njSin(work->ang.y) * record->turn_speed * (360.0F / 65536.0F));
        police_exhaust.speed.y = velocity.y + record->motion.speed.y;
        police_exhaust.speed.z =
            velocity.z + record->motion.speed.z -
            (njCos(work->ang.y) * record->turn_speed * (360.0F / 65536.0F));
        police_exhaust.color.a =
            (1.0F + (POLICE_THROTTLE(record) * 1.5F)) * POLICE_GLOW(record);
        police_exhaust.growth = (-POLICE_THROTTLE(record) + 2.0F) * 0.5F;
        panlight_create(&police_exhaust);
        direction.x = -1.6F;
        direction.y = -0.6F;
        direction.z = 0.0F;
        njCalcVector(&police_rotation, &direction, &velocity);
        velocity.x *= 0.6F;
        velocity.y *= 0.6F;
        velocity.z *= 0.6F;
        origin.x = origin.y = origin.z = 0.0F;
        njPushMatrix(&police_matrix);
        njTranslate(NULL, -9.7F, 2.3F, 0.0F);
        njCalcPoint(NULL, &origin, &emission);
        njPopMatrix(1);
        police_exhaust_low.pos = emission;
        police_exhaust_low.speed.x = velocity.x + record->motion.speed.x;
        police_exhaust_low.speed.y = velocity.y + record->motion.speed.y;
        police_exhaust_low.speed.z = velocity.z + record->motion.speed.z;
        police_exhaust_low.color.a =
            (0.8F + (POLICE_THROTTLE(record) * 1.5F)) * POLICE_GLOW(record);
        panlight_create(&police_exhaust_low);
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
}

/* draw the car at its action's frame, the driver on it, and then the
 * lights. the driver looks POLICE_LOOK from the car's heading. a car out of
 * view draws nothing */
static void police_draw_car(struct task_work *work,
                            struct lockon_target *record)
{
    NJS_ACTION *action;
    /* the driver leans against half the car's bank and pitch. each lean is
     * a whole turn less than that, so it is never 0. so TURN, which skips a
     * turn of 0, always rotates by it */
    Angle lean_x =
        -((Uint16)((Sint16)POLICE_BANK(record) + 0x8000) >> 1) - 0xc000;
    Angle lean_z =
        -((Uint16)((Sint16)POLICE_PITCH(record) + 0x8000) >> 1) - 0xc000;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 50.0F)) {
        return;
    }
    enemy_spring_scale(work, record);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njSetTexture(&police_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, POLICE_PITCH(record));
    TURN(njRotateX, POLICE_BANK(record));
    njPushMatrix(NULL);
    njRotateY(NULL, 0x8000);
    njScale(NULL, work->scl.x, work->scl.y, work->scl.x);
    action = &police_action;
    if (record->frame >= action->motion->nbFrame) {
        record->frame -= action->motion->nbFrame;
    }
    draw_action(&police_action, record->frame);
    njPopMatrix(1);
    njTranslate(NULL, 1.0F, 5.0F, 0.0F);
    TURN(njRotateZ, lean_z);
    TURN(njRotateX, lean_x);
    TURN(njRotateY, POLICE_LOOK(record));
    njTranslate(NULL, 0.0F, 1.0F, 0.0F);
    njRotateY(NULL, 0x8000);
    draw_tree(&police_driver_object);
    njPopMatrix(1);
    police_draw_lights(work, record);
}

static void police_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);
#ifdef GCC
    /* the split frame's second picture draws from a copy of the record, so
     * that the action's frame, which police_draw_car wraps, stays as the
     * steps left it */
    struct lockon_target copy;

    if (frame_redrawing) {
        copy = *record;
        record = &copy;
    }
#endif

    police_draw_car(work, record);
}

/* the driver looks toward aim_y by LOOK_SPEED a step. the look stays within
 * about a quarter turn of the heading either way */
static void police_look(struct task_work *work, struct lockon_target *record)
{
    int angle = angle_difference(record->aim_y, work->ang.y);

    angle = (Uint16)(angle + 0x8000);
    if (angle < 0x400f) {
        angle = 0x400f;
    } else if (angle > 0xbff0) {
        angle = 0xbff0;
    }
    angle = (Uint16)(angle + 0x8000);
    POLICE_LOOK(record) =
        angle_approach(POLICE_LOOK(record), -angle, LOOK_SPEED);
}

/* hold the car up off the floor below, tilting it to the floor's slope.
 * with no floor below, or one more than 20 below the last, hold to the last
 * floor's height. the car also levels out then. touching a wall sets its
 * scale springing. near the floor, a car that isn't waiting blows dust off
 * it */
static void police_hover(struct task_work *work, struct lockon_target *record)
{
    struct surface *floor;
    struct angles slope;
    Float height;
    Float clearance;
    Float lift;

    enemy_collide(work, record);
    floor = &record->ground.sides[SIDE_BELOW];
    /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
    if (floor->found) {
        height = floor->level;
        slope.x = floor->ang_x;
        slope.z = floor->ang_z;
        work->ang.x = angle_approach(work->ang.x, slope.x, 0xe0);
        work->ang.z = angle_approach(work->ang.z, slope.z, 0xe0);
        clearance = work->pos.y - height + 3.5F;
        lift = POLICE_SPEED2(record) * 0.26F;
        if (lift > 0.8F) {
            lift = 0.8F;
        }
        if (clearance > -20.0F && clearance < 50.0F) {
            record->motion.accel.y += POLICE_THRUST(record) * (lift + 1.1F) /
                                      (3.0F + (clearance * 1.4F));
            if (record->motion.accel.y > 0.7F) {
                record->motion.accel.y = 0.7F;
            }
            POLICE_FLOOR(record) = height;
        } else if (POLICE_FLOOR(record) - height > 20.0F) {
            record->motion.accel.y +=
                POLICE_THRUST(record) *
                (0.85F + (POLICE_SPEED2(record) * 0.045F)) /
                (work->pos.y - POLICE_FLOOR(record) + 6.8F);
            if (record->motion.accel.y > 0.8F) {
                record->motion.accel.y = 0.8F;
            }
            work->ang.x = angle_approach(work->ang.x, 0, 0xf0);
            work->ang.z = angle_approach(work->ang.z, 0, 0xf0);
        }
    } else {
#ifdef GCC
        /* repairs the unset clearance the tag below names: with no floor,
         * clearance is DOWNWASH_REACH, so the car raises no dust, as in the
         * image. GCC can't see that height then goes unread, so height
         * takes the last floor's */
        clearance = DOWNWASH_REACH;
        height = POLICE_FLOOR(record);
#endif
        record->motion.accel.y += POLICE_THRUST(record) *
                                  (0.85F + (POLICE_SPEED2(record) * 0.045F)) /
                                  (work->pos.y - POLICE_FLOOR(record) + 6.8F);
        if (record->motion.accel.y > 0.8F) {
            record->motion.accel.y = 0.8F;
        }
        work->ang.x = angle_approach(work->ang.x, 0, 0xf0);
        work->ang.z = angle_approach(work->ang.z, 0, 0xf0);
    }
    if (record->flags & ENEMY_ON_WALL) {
        record->scale_speed = 0.3F;
    }
    /* @bug uninitialized: the downwash test reads clearance where no floor
     * below has set it. with no floor, the branch at 0x0c90ea10 skips the
     * stores to height, in FR13, and clearance, in FR15. police_step loads
     * SIREN_RADIUS into FR15 at 0x0c90f666, before it calls the five modes'
     * functions. so with no floor, clearance holds 200, and the test at
     * 0x0c90ebf2 fails. height then goes unread */
    if (clearance > -3.0F && clearance < DOWNWASH_REACH && work->mode) {
        Float strength = (21.0F - clearance) * 0.25F;

        police_downwash.scale = 0.5F + (strength * 0.1F);
        police_downwash.speed.x = strength * ((rand() * RAND_SCALE) - 0.5F);
        police_downwash.speed.y = 0.0F;
        police_downwash.speed.z = strength * ((rand() * RAND_SCALE) - 0.5F);
        police_downwash.pos.x = work->pos.x + (police_downwash.speed.x * 4.0F);
        police_downwash.pos.y = height + 1.5F;
        police_downwash.pos.z = work->pos.z + (police_downwash.speed.z * 4.0F);
        dust_create_glow(&police_downwash);
    }
}

/* the set editor's view: the chase range's circle and the car. while the
 * editor holds the car, a blinking label and the prompts too */
static void police_edit_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 edge;
    NJS_POINT3 from;
    NJS_POINT3 to;
    int i;

    if (placement_editing(task)) {
        if (work->tick > 25) {
            njPrintC(NJM_LOCATION(8, 8), "< POLICE >");
        } else {
            njPrintC(NJM_LOCATION(8, 8), "<        >");
        }
        if (work->tick++ > 50) {
            work->tick = 0;
        }
        njPrintC(NJM_LOCATION(17, 16), "->             ");
        njPrintC(NJM_LOCATION(17, 17), "-> ROT Y       ");
        njPrintC(NJM_LOCATION(17, 18), "-> EMERALD     ");
        njPrintC(NJM_LOCATION(17, 19), "->             ");
        njPrintC(NJM_LOCATION(17, 20), "->             ");
        njPrintC(NJM_LOCATION(17, 21), "-> CHASE RANGE ");
    }
    edge.x = work->scl.z + CHASE_RANGE;
    edge.y = 0.0F;
    edge.z = 0.0F;
    for (i = 0; i < ENEMY_RANGE_SEGMENTS; i++) {
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, i * ENEMY_RANGE_TURN);
        njCalcPoint(NULL, &edge, &from);
        njPopMatrix(1);
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, (i + 1) * ENEMY_RANGE_TURN);
        njCalcPoint(NULL, &edge, &to);
        njPopMatrix(1);
        draw_line_v(&from, &to);
    }
    njSetTexture(&police_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y + 0x8000);
    draw_tree(&police_object);
    njPopMatrix(1);
}

static void police_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    enemy_end(task, work, record);
}

/* the blast, the debris and an animal set free */
static void police_explode(struct task_work *work, struct lockon_target *record)
{
    explosion_create_at(work->pos.x, work->pos.y + 5.0F, work->pos.z, 1.4F);
    enemy_scatter_debris(work, DEBRIS_COUNT);
    animal_create_random(15, work->pos.x, work->pos.y + 15.0F, work->pos.z);
}

/* hover at home as the thrust eases to THRUST_IDLE, the lights fade and
 * the car levels. once player 0 comes within PLAYER_RANGE, start */
static void police_wait(struct task_work *work, struct lockon_target *record)
{
    police_choose_target(work, record);
    police_hover(work, record);
    police_fly(work, record);
    police_draw_car(work, record);
    POLICE_THRUST(record) += -0.004F;
    if (POLICE_THRUST(record) < THRUST_IDLE) {
        POLICE_THRUST(record) = THRUST_IDLE;
    }
    if (enemy_player_distance2(work, 0) < PLAYER_RANGE) {
        ENEMY_SET_MODE(work, record, POLICE_START);
        work->flags |= POLICE_CHASING;
        record->target = player_work[0]->pos;
    }
    POLICE_GLOW(record) -= 0.02F;
    if (POLICE_GLOW(record) < 0.0F) {
        POLICE_GLOW(record) = 0.0F;
    }
    POLICE_BANK(record) = angle_approach(POLICE_BANK(record), 0, 0x60);
    POLICE_PITCH(record) = angle_approach(POLICE_PITCH(record), 0, 0x60);
    POLICE_LOOK(record) = angle_approach(POLICE_LOOK(record), 0, 0x100);
    work->ang.x = angle_approach(work->ang.x, 0, 0xb0);
    work->ang.z = angle_approach(work->ang.z, 0, 0xb0);
}

/* raise the thrust to THRUST_FULL and the lights, light the siren, and
 * turn toward the target. once the timer passes START_STEPS, pick
 * POLICE_SPOT and chase */
static void police_start(struct task_work *work, struct lockon_target *record)
{
    record->turn_speed = 80;
    police_touch(work, record);
    police_hover(work, record);
    police_turn(work, record);
    police_fly(work, record);
    police_look(work, record);
    police_draw_car(work, record);
    POLICE_THRUST(record) += 0.05F;
    if (POLICE_THRUST(record) > THRUST_FULL) {
        POLICE_THRUST(record) = THRUST_FULL;
    }
    if (TASK_TIMER(work) > START_STEPS) {
        POLICE_SPOT(record) = (Uint8)(rand() * RAND_SCALE * 3.999F);
        ENEMY_SET_MODE(work, record, POLICE_CHASE);
    }
    POLICE_GLOW(record) += 0.02F;
    if (POLICE_GLOW(record) > 1.0F) {
        POLICE_GLOW(record) = 1.0F;
    }
    record->frame += 0.4F;
    police_siren_2_materials[0].attrflags &= ~SIREN_DARK;
    police_siren_1_materials[0].attrflags &= ~SIREN_DARK;
}

/* aim at player 0 while the car is on screen, else at POLICE_SPOT's spot
 * near player 0. police_choose_target then replaces either aim. once
 * POLICE_CHASING clears, the car returns within HOME_NEAR of home */
static void police_chase(struct task_work *work, struct lockon_target *record)
{
    if (enemy_on_screen(&work->pos)) {
        record->target = player_work[0]->pos;
    } else {
        enemy_target_near_player(work, record, 0, POLICE_SPOT(record));
    }
    police_choose_target(work, record);
    police_touch(work, record);
    police_hover(work, record);
    police_turn_hard(work, record);
    police_accelerate(work, record);
    police_fly(work, record);
    police_look(work, record);
    police_draw_car(work, record);
    if (!(work->flags & POLICE_CHASING)) {
        if (enemy_home_distance2(work, record) < HOME_NEAR * HOME_NEAR) {
            ENEMY_SET_MODE(work, record, POLICE_RETURN);
        }
    }
    record->frame += 0.8F;
}

/* drift home, turning back to the heading the car was placed at. the
 * thrust and the lights fall, and the siren darkens. police_fly_home makes
 * the car wait within HOME_NEAR of home, and so does the timer passing
 * RETURN_STEPS. police_choose_target restarts the timer within RETURN_NEAR
 * of home. as player 0 comes within PLAYER_RANGE, the car starts again */
static void police_return(struct task_work *work, struct lockon_target *record)
{
    record->turn_speed = 80;
    police_choose_target(work, record);
    police_touch(work, record);
    police_hover(work, record);
    work->ang.y = angle_approach(work->ang.y, record->heading, 0x200);
    POLICE_BANK(record) = angle_approach(POLICE_BANK(record), 0, 0x60);
    police_fly_home(work, record);
    police_look(work, record);
    police_draw_car(work, record);
    POLICE_THRUST(record) -= 0.013F;
    if (POLICE_THRUST(record) < THRUST_IDLE) {
        POLICE_THRUST(record) = THRUST_IDLE;
    }
    if (TASK_TIMER(work) > RETURN_STEPS) {
        ENEMY_SET_MODE(work, record, POLICE_WAIT);
    }
    if (enemy_player_distance2(work, 0) < PLAYER_RANGE) {
        ENEMY_SET_MODE(work, record, POLICE_START);
        work->flags |= POLICE_CHASING;
        record->target = player_work[0]->pos;
    }
    POLICE_GLOW(record) -= 0.005F;
    if (POLICE_GLOW(record) < 0.0F) {
        POLICE_GLOW(record) = 0.0F;
    }
    record->frame += 0.2F;
    police_siren_2_materials[0].attrflags |= SIREN_DARK;
    police_siren_1_materials[0].attrflags |= SIREN_DARK;
}

/* the beaten car flies on for 12 steps, then blows up */
static void police_hit(struct task_work *work, struct lockon_target *record)
{
    police_touch(work, record);
    police_fly(work, record);
    police_hover(work, record);
    police_draw_car(work, record);
    if (VARS(work)->since_hit++ > 10) {
        work->mode = POLICE_GONE;
    }
}

static void police_step(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;

    /* @hack redundant: player 0's work handed to enemy_ignore, which keeps the
     * image's load of it into R4 (MOV.L @R4,R4 at 0x0c90f636); without it,
     * 85 of 155 words differ */
    enemy_ignore(player_work[0]);
    record = LOCKON_TARGET(task);
    if (edit_mode) {
        police_edit_draw(task);
        return;
    }
    if (placement_try_leave_by(task, POLICE_RANGE)) {
        return;
    }
    switch (work->mode) {
    case POLICE_WAIT:
        police_wait(work, record);
        break;
    case POLICE_START:
        police_start(work, record);
        sound_play_around(SE_SIREN, (int)work, 1, 48, work->pos.x, work->pos.y,
                          work->pos.z, SIREN_RADIUS, 2);
        break;
    case POLICE_CHASE:
        police_chase(work, record);
        sound_play_around(SE_SIREN, (int)work, 1, 48, work->pos.x, work->pos.y,
                          work->pos.z, SIREN_RADIUS, 2);
        break;
    case POLICE_RETURN:
        police_return(work, record);
        sound_play_around(SE_SIREN, (int)work, 1, 48, work->pos.x, work->pos.y,
                          work->pos.z, SIREN_RADIUS, 2);
        break;
    case POLICE_HIT:
        police_hit(work, record);
        break;
    case POLICE_GONE:
        police_explode(work, record);
        enemy_end(task, work, record);
        placement_kill(task);
        knuckles_take_piece(POLICE_PIECE(record), &work->pos);
        return;
    }
    /* @hack redundant: a store of 2 to the timer that never runs, by which
     * SHC weighs the sounds' 2 above sound_play_around's address. the address
     * takes R10 and the 2 R11, at 0x0c90f664 and 0x0c90f66e; without it and
     * its else, the two trade and 4 of 155 words differ */
    /* @hack redundant: the empty else, which loads enemy_save_pos's address
     * into R2 for its call (0x0c90f730, 0x0c90f734); without it, R3 takes it
     * and 2 of 155 words differ */
    if (0) {
        work->timer = 2;
    } else {
    }
    enemy_save_pos(work, record);
    knuckles_find_piece(POLICE_PIECE(record), &work->pos);
    POLICE_SIREN(record) += SIREN_SPEED;
    work->timer++;
    if (work->flags & POLICE_SOLID) {
        collision_register(work);
    }
}
