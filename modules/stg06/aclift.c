/* E ACLIFT, a lift that raises an aircraft through doors in the deck as
 * player 0 comes near. the aircraft takes off, then climbs away or turns to
 * fire its two missiles at player 0. each missile flies at its spot, which
 * follows player 0 along the floor. the spot is a child task that runs
 * missile_spot_step. a missile bursts on what it meets, or at its spot */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "animal/kinds.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/explosion.h"
#include "effect/panlight.h"
#include "enemy/enemy.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "player/player.h"
#include "stg06/aclift.h"
#include "stg06/missile_spot.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 10
#define TEXT_9_LENGTH 10
#define TEXT_10_LENGTH 8
#define TEXT_11_LENGTH 10
#define TEXT_12_LENGTH 6
#define TEXT_13_LENGTH 5
#define TEXT_14_LENGTH 5
#define TEXT_15_LENGTH 6
#define TEXT_16_LENGTH 10
#define TEXT_17_LENGTH 8
#define TEXT_18_LENGTH 7
#define TEXT_19_LENGTH 12
#define TEXT_20_LENGTH 7
#define TEXT_21_LENGTH 3
#define TEXT_22_LENGTH 5
#define TEXT_23_LENGTH 5
#define TEXT_24_LENGTH 4
#define TEXT_25_LENGTH 4
#define TEXT_26_LENGTH 8
#define TEXT_27_LENGTH 8
#define TEXT_28_LENGTH 7
#define TEXT_29_LENGTH 4
#define TEXT_30_LENGTH 3
#define TEXT_31_LENGTH 9
#define TEXT_32_LENGTH 9
#define TEXT_33_LENGTH 6
#define TEXT_34_LENGTH 4
#define TEXT_35_LENGTH 5
#define TEXT_36_LENGTH 7
#define TEXT_37_LENGTH 5
#define TEXT_38_LENGTH 5
#define TEXT_39_LENGTH 8
#define TEXT_40_LENGTH 7
#define TEXT_41_LENGTH 6
#define TEXT_42_LENGTH 5
#define TEXT_43_LENGTH 10
#define TEXT_44_LENGTH 10
#define TEXT_45_LENGTH 10
#include "stg06/aclift/text.inc"
/* the range placement_try_leave_by takes, squared. the camera or player 0
 * within 2000 keeps a missile, the aircraft or a lift with no parent. so
 * does player 1 within 40, if it is Sonic */
#define LEAVE_RANGE (2000.0F * 2000.0F)

/* the aircraft's and the missiles' models face half a turn from their
 * headings */
#define MODEL_TURN 0x8000

/* a missile's work->mode */
enum missile_mode {
    MISSILE_WAIT, /* under its wing, until the aircraft aims */
    MISSILE_LAUNCH, /* under its wing, until the aircraft fires */
    MISSILE_FLY, /* at its spot, until it bursts */
    MISSILE_BLAST,
    MISSILE_GONE
};

/* the wing a missile hangs under, in its work->submode until it flies.
 * missile_collide then toggles submode */
enum missile_wing { MISSILE_RIGHT, MISSILE_LEFT };

/* a bit missile_start sets in work->flags, which nothing reads */
#define MISSILE_UNK_100 0x100

/* a flying missile pushes MISSILE_THRUST a step along its angles. it loses
 * MISSILE_DRAG of its speed a step, and spins MISSILE_SPIN about its
 * length. it turns MISSILE_TURN_SPEED a step toward its spot, and pitches
 * MISSILE_PITCH_SPEED */
#define MISSILE_THRUST 0.3F
#define MISSILE_DRAG 0.03F
#define MISSILE_SPIN 0x200
#define MISSILE_TURN_SPEED 0x400
#define MISSILE_PITCH_SPEED 0x200

/* a missile within MISSILE_NEAR of its spot bursts */
#define MISSILE_NEAR 5.0F
/* a missile within PLAYER_NEAR of player 0 clears the 1 in its tick */
#define PLAYER_NEAR 100.0F

/* a blast's shape grows BLAST_GROWTH a step, from a radius of BLAST_START to
 * BLAST_RADIUS. scl.x keeps the same radius. scl.y then falls BLAST_FADE a
 * step from 0, and below -1 the missile is gone */
#define BLAST_START 5.0F
#define BLAST_GROWTH 1.0F
#define BLAST_RADIUS 50.0F
#define BLAST_FADE 0.05F

/* a missile's place under each wing, from the aircraft's middle in its
 * model's frame. the right wing's is at -MOUNT_Z */
#define MOUNT_X 4.3956776F
#define MOUNT_Y (-14.181034F)
#define MOUNT_Z 25.47671F

/* an aircraft's work->mode. no step sets AIRCRAFT_SET_OFF or
 * AIRCRAFT_LANDING */
enum aircraft_mode {
    AIRCRAFT_WAIT = 0, /* on the lift, until the lift lets it go */
    AIRCRAFT_IGNITE = 1, /* its jets build, still on the lift */
    AIRCRAFT_TAKE_OFF = 2, /* it levels out and gathers speed */
    AIRCRAFT_SET_OFF = 3,
    AIRCRAFT_TURN_LEFT = 4,
    AIRCRAFT_TURN_RIGHT = 5,
    AIRCRAFT_RISE = 6, /* it climbs straight ahead until out of range */
    AIRCRAFT_AIM = 7, /* at player 0, until it fires or gives up */
    AIRCRAFT_LEAVE = 8, /* toward a yaw of 0, rolling, until out of range */
    AIRCRAFT_HIT = 11, /* it breaks on its next step */
    /* the lift carries the aircraft up in any other mode, and down only in
     * this one */
    AIRCRAFT_LANDING = 12
};

/* the way an aircraft leaves, in its work->submode */
enum aircraft_way { AIRCRAFT_LEFT, AIRCRAFT_RIGHT, AIRCRAFT_AHEAD };

/* a bit of an aircraft's work flags. the modes that check for hits register
 * the aircraft's shapes while it is set. a damaging hit by a player's shape
 * that doesn't throw enemies clears it */
#define AIRCRAFT_SOLID 0x100
#define AIRCRAFT_GO 0x200 /* the lift lets it go */
#define AIRCRAFT_FIRE 0x400 /* it fires: its missiles leave their wings */

/* a damaging hit by a player's shape that throws enemies sets the aircraft's
 * speed.y to HIT_THROW. one by any other of a player's shapes sends player 0
 * up at HIT_BOUNCE */
#define HIT_THROW 0.7F
#define HIT_BOUNCE 2.0F
/* the 0.35 enemy_take_hit gives a damaged enemy's scale_speed. a damaging
 * hit stores it in the aircraft's scl.z, which nothing reads */
#define HIT_SPRING 0.35F

/* a player's touch that does no damage pushes player 0 away. player 0 leaves
 * PUSH_SPEED away from the aircraft and PUSH_RISE up, its stick ignored for
 * PUSH_STEPS */
#define PUSH_SPEED 3.5F
#define PUSH_RISE 1.8F
#define PUSH_STEPS 60

/* an aircraft's work->vars. ENEMY_MARK_BEATEN writes bytes 1 and 2 */
struct aircraft_vars {
    Uint8 unk_00[3];
    Uint8 pulse; /* counts steps, for the wing lamps' pulse */
};

#define VARS(work) ((struct aircraft_vars *)&(work)->vars)

/* a missile's work->vars. nothing reads aircraft: a missile reaches its
 * aircraft as task->parent */
struct missile_vars {
    Uint8 unk_00[8];
    struct task_work *aircraft;
};

#define MISSILE(work) ((struct missile_vars *)&(work)->vars)

/* the aircraft's names for spare fields of its record */
#define FLAME(record) ((record)->motion.slip) /* its jets' alpha */
#define BOB(record) ((record)->motion.gravity) /* its draw's rise over pos */
#define BOB_SIZE(record) ((record)->unk_7c) /* BOB's reach as it takes off */
#define SPEED(record) ((record)->motion.accel.x) /* along its pitch and yaw */
#define BOB_PHASE(record) ((record)->angles[0])
#define PITCH(record) ((record)->angles[1]) /* above 0, its nose is up */
#define ROLL(record) ((record)->angles[3])
#define ROLL_SPEED(record) ((record)->angles[5])

/* as the aircraft ignites, its nose dips DIP_TILT a step to DIP_PITCH. FLAME
 * grows FLAME_GROWTH a step, to about 0.6. the aircraft takes off after
 * IGNITE_STEPS, with FLAME at FLAME_FULL */
#define DIP_PITCH 0xfa00
#define DIP_TILT 36
#define FLAME_GROWTH 0.02F
#define FLAME_FULL 2.0F
#define IGNITE_STEPS 30

/* the aircraft bobs BOB_SHARE of FLAME as it ignites. as it takes off, it
 * bobs BOB_SIZE, which starts at BOB_START and falls BOB_DECAY a step. the
 * bob's phase turns BOB_SPEED a step */
#define BOB_SHARE 0.2F
#define BOB_START 0.3F
#define BOB_DECAY 0.003F
#define BOB_SPEED 0x6500

/* SPEED grows AIRCRAFT_THRUST a step while it is below a cap. the cap is
 * TURN_SPEED_CAP as the aircraft turns or aims, and else SPEED_CAP.
 * aircraft_move then keeps SPEED_KEPT of SPEED. SPEED settles toward 4.5,
 * where (SPEED + AIRCRAFT_THRUST) * SPEED_KEPT equals SPEED. so neither cap
 * is reached */
#define AIRCRAFT_THRUST 0.5F
#define SPEED_CAP 8.0F
#define TURN_SPEED_CAP 6.0F
#define SPEED_KEPT 0.9F

/* the aircraft turns after TAKE_OFF_STEPS of take-off, or rises after
 * AHEAD_STEPS */
#define TAKE_OFF_STEPS 30
#define AHEAD_STEPS 10

/* turning, the aircraft climbs to CLIMB_PITCH. after TURN_CLIMB_STEPS it
 * banks to BANK, or -BANK to the left, and turns TURN_SLOW a step. after
 * TURN_LEVEL_STEPS its pitch levels and it turns TURN_FAST a step. it aims
 * after TURN_STEPS */
#define CLIMB_PITCH 0x800
#define BANK 0x2800
#define TURN_SLOW 0x88
#define TURN_FAST 0xe3
#define TURN_CLIMB_STEPS 50
#define TURN_LEVEL_STEPS 150
#define TURN_STEPS 250

/* after take-off, the aircraft's pitch and roll move TILT_SLOW or TILT_FAST a
 * step toward their goals */
#define TILT_SLOW 16
#define TILT_FAST 32

/* aiming, the aircraft pitches to AIM_PITCH. it fires once its heading is
 * within FIRE_ANGLE of the way to player 0. it gives up below AIM_FLOOR over
 * its home */
#define AIM_PITCH 0xfe00
#define FIRE_ANGLE NJM_DEG_ANG(30)
#define AIM_FLOOR 150.0F

/* leaving, the aircraft turns LEAVE_TURN a step to a yaw of 0. its roll then
 * speeds up ROLL_GROWTH a step, to ROLL_SPEED_CAP. it climbs while below
 * LEAVE_HEIGHT */
#define LEAVE_TURN 96
#define ROLL_GROWTH 16
#define ROLL_SPEED_CAP 0x180
#define LEAVE_HEIGHT 235.0F

/* the wing lamps, from the aircraft's middle in its model's frame. the left
 * wing's is at LAMP_Z. their pulse turns LAMP_PULSE a step, so it repeats
 * every 128 steps */
#define LAMP_X 21.051941F
#define LAMP_Y (-23.085513F)
#define LAMP_Z 34.323315F
#define LAMP_PULSE 0x200

/* the jets, from the aircraft's middle in its own frame, which MODEL_TURN
 * doesn't turn. the right jet's is at JET_Z */
#define JET_X (-10.5F)
#define JET_Y (-4.0F)
#define JET_Z 14.0F

/* as it takes off, the aircraft blows three puffs of dust a step. they
 * leave from DUST_DEPTH below it, THIRD_TURN apart */
#define DUST_DEPTH 20.0F
#define THIRD_TURN 0x5555

/* the lift's work->mode */
enum aclift_mode {
    ACLIFT_WAIT, /* closed and down, until it makes an aircraft */
    ACLIFT_OPEN,
    ACLIFT_RAISE,
    ACLIFT_LAUNCH, /* it shakes, and lets the aircraft go */
    ACLIFT_LOWER,
    ACLIFT_CLOSE
};

/* in the lift's work flags: an aircraft is aboard, from its making until
 * the lift lets it go */
#define ACLIFT_LOADED 0x100

/* player 0 within CALL_RANGE calls up an aircraft */
#define CALL_RANGE 1000.0F

/* the lift's scl.z picks the aircraft's way: AIRCRAFT_RIGHT above 0,
 * AIRCRAFT_LEFT above LEFT_FLOOR, and AIRCRAFT_AHEAD otherwise. a turning
 * aircraft heads START_TURN less than the lift's ang.y, and one that rises
 * heads HALF_TURN from that */
#define LEFT_FLOOR (-10.0F)
#define START_TURN 0x4000
#define HALF_TURN 0x8000

/* scl.x is how far each door has slid open, along z. scl.y is the
 * platform's height from the top. the doors slide DOOR_SPEED a step, from 0
 * to DOOR_OPEN. the platform moves PLATFORM_SPEED a step, between
 * PLATFORM_DOWN and 0 */
#define DOOR_OPEN 45.0F
#define DOOR_SPEED 0.45F
#define PLATFORM_DOWN (-40.0F)
#define PLATFORM_SPEED 0.4F
/* the platform's shape sits PLATFORM_SHAPE_DROP below scl.y. each door's
 * sits DOOR_SHAPE_OUT farther out along z than the door has slid */
#define PLATFORM_SHAPE_DROP 5.0F
#define DOOR_SHAPE_OUT 16.25F

/* the aircraft rides SEAT above the platform. it starts START_DEPTH below
 * the lift's pos, on the platform down */
#define SEAT 19.0F
#define START_DEPTH 21.0F

/* the launch shakes the platform until its step SHAKE_END. the lift lets
 * the aircraft go after RELEASE_STEPS, and starts down after LAUNCH_STEPS.
 * the shake reaches SHAKE_SIZE, and its phase turns SHAKE_SPEED a step */
#define SHAKE_END 16
#define RELEASE_STEPS 5
#define LAUNCH_STEPS 60
#define SHAKE_SIZE 0.4F
#define SHAKE_SPEED 0x7500

/* the lift lamp's material LIFT_LAMP_MATERIAL takes texture
 * LIFT_LAMP_LOADED of aclift_lamp_texlist while an aircraft is aboard, else
 * LIFT_LAMP_EMPTY. the lamp sits LIFT_LAMP_X along the platform's x, turned
 * LIFT_LAMP_TURN */
#define LIFT_LAMP_MATERIAL 3
#define LIFT_LAMP_LOADED 3
#define LIFT_LAMP_EMPTY 8
#define LIFT_LAMP_X (-26.0F)
#define LIFT_LAMP_TURN 0x4000

/* the lift's work->vars */
struct aclift_vars {
    Uint8 unk_00[4];
    Float shake; /* the platform's offset up, as it shakes */
    Angle phase; /* the shake's phase */
};

#define LIFT(work) ((struct aclift_vars *)&(work)->vars)

/* a missile's exhaust: the point it leaves from and its speed, in the frame
 * missile_draw sets up */
static const NJS_POINT3 missile_exhaust_at = {-8.0F, 0.0F, 0.0F};
static const NJS_VECTOR missile_exhaust_dir = {-9.0F, 0.0F, 0.0F};
static const NJS_POINT3 missile_origin = {0.0F, 0.0F, 0.0F};
/* each puff of an aircraft's dust starts at aircraft_dust_at and moves at
 * aircraft_dust_dir, turned to the puff's angle */
static const NJS_POINT3 aircraft_dust_at = {5.0F, 1.0F, 0.0F};
static const NJS_VECTOR aircraft_dust_dir = {2.0F, 0.1F, 0.0F};

static NJS_MATERIAL missile_node2_materials[1] = {
#include "stg06/aclift/missile_node2_materials.inc"
};

static Sint16 missile_node2_meshes0[16] = {
#include "stg06/aclift/missile_node2_meshes0.inc"
};

static NJS_COLOR missile_node2_uvs0[12] = {
#include "stg06/aclift/missile_node2_uvs0.inc"
};

static NJS_MESHSET missile_node2_meshsets[1] = {
#include "stg06/aclift/missile_node2_meshsets.inc"
};

static NJS_POINT3 missile_node2_points[12] = {
#include "stg06/aclift/missile_node2_points.inc"
};

static NJS_VECTOR missile_node2_normals[12] = {
#include "stg06/aclift/missile_node2_normals.inc"
};

static NJS_MODEL missile_node2_model = {
#include "stg06/aclift/missile_node2_model.inc"
};

static NJS_OBJECT missile_node2_object = {
#include "stg06/aclift/missile_node2_object.inc"
};

static NJS_MATERIAL missile_node1_materials[1] = {
#include "stg06/aclift/missile_node1_materials.inc"
};

static Sint16 missile_node1_meshes0[16] = {
#include "stg06/aclift/missile_node1_meshes0.inc"
};

static NJS_COLOR missile_node1_uvs0[12] = {
#include "stg06/aclift/missile_node1_uvs0.inc"
};

static NJS_MESHSET missile_node1_meshsets[1] = {
#include "stg06/aclift/missile_node1_meshsets.inc"
};

static NJS_POINT3 missile_node1_points[12] = {
#include "stg06/aclift/missile_node1_points.inc"
};

static NJS_VECTOR missile_node1_normals[12] = {
#include "stg06/aclift/missile_node1_normals.inc"
};

static NJS_MODEL missile_node1_model = {
#include "stg06/aclift/missile_node1_model.inc"
};

static NJS_OBJECT missile_node1_object = {
#include "stg06/aclift/missile_node1_object.inc"
};

static NJS_MATERIAL missile_materials[5] = {
#include "stg06/aclift/missile_materials.inc"
};

static Sint16 missile_meshes0[10] = {
#include "stg06/aclift/missile_meshes0.inc"
};

static Sint16 missile_meshes1[30] = {
#include "stg06/aclift/missile_meshes1.inc"
};

static Sint16 missile_meshes2[30] = {
#include "stg06/aclift/missile_meshes2.inc"
};

static Sint16 missile_meshes3[5] = {
#include "stg06/aclift/missile_meshes3.inc"
};

static Sint16 missile_meshes4[12] = {
#include "stg06/aclift/missile_meshes4.inc"
};

static NJS_COLOR missile_uvs0[8] = {
#include "stg06/aclift/missile_uvs0.inc"
};

static NJS_COLOR missile_uvs1[24] = {
#include "stg06/aclift/missile_uvs1.inc"
};

static NJS_COLOR missile_uvs2[24] = {
#include "stg06/aclift/missile_uvs2.inc"
};

static NJS_COLOR missile_uvs3[4] = {
#include "stg06/aclift/missile_uvs3.inc"
};

static NJS_MESHSET missile_meshsets[5] = {
#include "stg06/aclift/missile_meshsets.inc"
};

static NJS_POINT3 missile_points[20] = {
#include "stg06/aclift/missile_points.inc"
};

static NJS_VECTOR missile_normals[20] = {
#include "stg06/aclift/missile_normals.inc"
};

static NJS_MODEL missile_model = {
#include "stg06/aclift/missile_model.inc"
};

static NJS_OBJECT missile_object = {
#include "stg06/aclift/missile_object.inc"
};

static NJS_MATERIAL aircraft_parts_node13_materials[3] = {
#include "stg06/aclift/aircraft_parts_node13_materials.inc"
};

static Sint16 aircraft_parts_node13_meshes0[17] = {
#include "stg06/aclift/aircraft_parts_node13_meshes0.inc"
};

static Sint16 aircraft_parts_node13_meshes1[17] = {
#include "stg06/aclift/aircraft_parts_node13_meshes1.inc"
};

static Sint16 aircraft_parts_node13_meshes2[53] = {
#include "stg06/aclift/aircraft_parts_node13_meshes2.inc"
};

static NJS_COLOR aircraft_parts_node13_uvs0[14] = {
#include "stg06/aclift/aircraft_parts_node13_uvs0.inc"
};

static NJS_COLOR aircraft_parts_node13_uvs1[14] = {
#include "stg06/aclift/aircraft_parts_node13_uvs1.inc"
};

static NJS_COLOR aircraft_parts_node13_uvs2[48] = {
#include "stg06/aclift/aircraft_parts_node13_uvs2.inc"
};

static NJS_MESHSET aircraft_parts_node13_meshsets[3] = {
#include "stg06/aclift/aircraft_parts_node13_meshsets.inc"
};

static NJS_POINT3 aircraft_parts_node13_points[40] = {
#include "stg06/aclift/aircraft_parts_node13_points.inc"
};

static NJS_VECTOR aircraft_parts_node13_normals[40] = {
#include "stg06/aclift/aircraft_parts_node13_normals.inc"
};

static NJS_MODEL aircraft_parts_node13_model = {
#include "stg06/aclift/aircraft_parts_node13_model.inc"
};

static NJS_OBJECT aircraft_parts_node13_object = {
#include "stg06/aclift/aircraft_parts_node13_object.inc"
};

static NJS_MATERIAL aircraft_parts_node12_materials[2] = {
#include "stg06/aclift/aircraft_parts_node12_materials.inc"
};

static Sint16 aircraft_parts_node12_meshes0[40] = {
#include "stg06/aclift/aircraft_parts_node12_meshes0.inc"
};

static Sint16 aircraft_parts_node12_meshes1[32] = {
#include "stg06/aclift/aircraft_parts_node12_meshes1.inc"
};

static NJS_COLOR aircraft_parts_node12_uvs0[30] = {
#include "stg06/aclift/aircraft_parts_node12_uvs0.inc"
};

static NJS_COLOR aircraft_parts_node12_uvs1[26] = {
#include "stg06/aclift/aircraft_parts_node12_uvs1.inc"
};

static NJS_MESHSET aircraft_parts_node12_meshsets[2] = {
#include "stg06/aclift/aircraft_parts_node12_meshsets.inc"
};

static NJS_POINT3 aircraft_parts_node12_points[16] = {
#include "stg06/aclift/aircraft_parts_node12_points.inc"
};

static NJS_VECTOR aircraft_parts_node12_normals[16] = {
#include "stg06/aclift/aircraft_parts_node12_normals.inc"
};

static NJS_MODEL aircraft_parts_node12_model = {
#include "stg06/aclift/aircraft_parts_node12_model.inc"
};

static NJS_OBJECT aircraft_parts_node12_object = {
#include "stg06/aclift/aircraft_parts_node12_object.inc"
};

static NJS_MATERIAL aircraft_parts_node11_materials[4] = {
#include "stg06/aclift/aircraft_parts_node11_materials.inc"
};

static Sint16 aircraft_parts_node11_meshes0[57] = {
#include "stg06/aclift/aircraft_parts_node11_meshes0.inc"
};

static Sint16 aircraft_parts_node11_meshes1[13] = {
#include "stg06/aclift/aircraft_parts_node11_meshes1.inc"
};

static Sint16 aircraft_parts_node11_meshes2[18] = {
#include "stg06/aclift/aircraft_parts_node11_meshes2.inc"
};

static Sint16 aircraft_parts_node11_meshes3[18] = {
#include "stg06/aclift/aircraft_parts_node11_meshes3.inc"
};

static NJS_COLOR aircraft_parts_node11_uvs0[50] = {
#include "stg06/aclift/aircraft_parts_node11_uvs0.inc"
};

static NJS_COLOR aircraft_parts_node11_uvs1[10] = {
#include "stg06/aclift/aircraft_parts_node11_uvs1.inc"
};

static NJS_COLOR aircraft_parts_node11_uvs2[14] = {
#include "stg06/aclift/aircraft_parts_node11_uvs2.inc"
};

static NJS_COLOR aircraft_parts_node11_uvs3[14] = {
#include "stg06/aclift/aircraft_parts_node11_uvs3.inc"
};

static NJS_MESHSET aircraft_parts_node11_meshsets[4] = {
#include "stg06/aclift/aircraft_parts_node11_meshsets.inc"
};

static NJS_POINT3 aircraft_parts_node11_points[28] = {
#include "stg06/aclift/aircraft_parts_node11_points.inc"
};

static NJS_VECTOR aircraft_parts_node11_normals[28] = {
#include "stg06/aclift/aircraft_parts_node11_normals.inc"
};

static NJS_MODEL aircraft_parts_node11_model = {
#include "stg06/aclift/aircraft_parts_node11_model.inc"
};

static NJS_OBJECT aircraft_parts_node11_object = {
#include "stg06/aclift/aircraft_parts_node11_object.inc"
};

static NJS_OBJECT aircraft_parts_node10_object = {
#include "stg06/aclift/aircraft_parts_node10_object.inc"
};

static NJS_OBJECT aircraft_parts_node9_object = {
#include "stg06/aclift/aircraft_parts_node9_object.inc"
};

static NJS_OBJECT aircraft_parts_node8_object = {
#include "stg06/aclift/aircraft_parts_node8_object.inc"
};

static NJS_MATERIAL aircraft_parts_node7_materials[2] = {
#include "stg06/aclift/aircraft_parts_node7_materials.inc"
};

static Sint16 aircraft_parts_node7_meshes0[5] = {
#include "stg06/aclift/aircraft_parts_node7_meshes0.inc"
};

static Sint16 aircraft_parts_node7_meshes1[10] = {
#include "stg06/aclift/aircraft_parts_node7_meshes1.inc"
};

static NJS_COLOR aircraft_parts_node7_uvs0[4] = {
#include "stg06/aclift/aircraft_parts_node7_uvs0.inc"
};

static NJS_COLOR aircraft_parts_node7_uvs1[8] = {
#include "stg06/aclift/aircraft_parts_node7_uvs1.inc"
};

static NJS_MESHSET aircraft_parts_node7_meshsets[2] = {
#include "stg06/aclift/aircraft_parts_node7_meshsets.inc"
};

static NJS_POINT3 aircraft_parts_node7_points[6] = {
#include "stg06/aclift/aircraft_parts_node7_points.inc"
};

static NJS_VECTOR aircraft_parts_node7_normals[6] = {
#include "stg06/aclift/aircraft_parts_node7_normals.inc"
};

static NJS_MODEL aircraft_parts_node7_model = {
#include "stg06/aclift/aircraft_parts_node7_model.inc"
};

static NJS_OBJECT aircraft_parts_node7_object = {
#include "stg06/aclift/aircraft_parts_node7_object.inc"
};

static NJS_MATERIAL aircraft_parts_node6_materials[2] = {
#include "stg06/aclift/aircraft_parts_node6_materials.inc"
};

static Sint16 aircraft_parts_node6_meshes0[5] = {
#include "stg06/aclift/aircraft_parts_node6_meshes0.inc"
};

static Sint16 aircraft_parts_node6_meshes1[10] = {
#include "stg06/aclift/aircraft_parts_node6_meshes1.inc"
};

static NJS_COLOR aircraft_parts_node6_uvs0[4] = {
#include "stg06/aclift/aircraft_parts_node6_uvs0.inc"
};

static NJS_COLOR aircraft_parts_node6_uvs1[8] = {
#include "stg06/aclift/aircraft_parts_node6_uvs1.inc"
};

static NJS_MESHSET aircraft_parts_node6_meshsets[2] = {
#include "stg06/aclift/aircraft_parts_node6_meshsets.inc"
};

static NJS_POINT3 aircraft_parts_node6_points[6] = {
#include "stg06/aclift/aircraft_parts_node6_points.inc"
};

static NJS_VECTOR aircraft_parts_node6_normals[6] = {
#include "stg06/aclift/aircraft_parts_node6_normals.inc"
};

static NJS_MODEL aircraft_parts_node6_model = {
#include "stg06/aclift/aircraft_parts_node6_model.inc"
};

static NJS_OBJECT aircraft_parts_node6_object = {
#include "stg06/aclift/aircraft_parts_node6_object.inc"
};

static NJS_MATERIAL aircraft_parts_node5_materials[10] = {
#include "stg06/aclift/aircraft_parts_node5_materials.inc"
};

static Sint16 aircraft_parts_node5_meshes0[20] = {
#include "stg06/aclift/aircraft_parts_node5_meshes0.inc"
};

static Sint16 aircraft_parts_node5_meshes1[10] = {
#include "stg06/aclift/aircraft_parts_node5_meshes1.inc"
};

static Sint16 aircraft_parts_node5_meshes2[14] = {
#include "stg06/aclift/aircraft_parts_node5_meshes2.inc"
};

static Sint16 aircraft_parts_node5_meshes3[4] = {
#include "stg06/aclift/aircraft_parts_node5_meshes3.inc"
};

static Sint16 aircraft_parts_node5_meshes4[9] = {
#include "stg06/aclift/aircraft_parts_node5_meshes4.inc"
};

static Sint16 aircraft_parts_node5_meshes5[42] = {
#include "stg06/aclift/aircraft_parts_node5_meshes5.inc"
};

static Sint16 aircraft_parts_node5_meshes6[24] = {
#include "stg06/aclift/aircraft_parts_node5_meshes6.inc"
};

static Sint16 aircraft_parts_node5_meshes7[12] = {
#include "stg06/aclift/aircraft_parts_node5_meshes7.inc"
};

static Sint16 aircraft_parts_node5_meshes8[24] = {
#include "stg06/aclift/aircraft_parts_node5_meshes8.inc"
};

static Sint16 aircraft_parts_node5_meshes9[7] = {
#include "stg06/aclift/aircraft_parts_node5_meshes9.inc"
};

static NJS_COLOR aircraft_parts_node5_uvs0[16] = {
#include "stg06/aclift/aircraft_parts_node5_uvs0.inc"
};

static NJS_COLOR aircraft_parts_node5_uvs1[8] = {
#include "stg06/aclift/aircraft_parts_node5_uvs1.inc"
};

static NJS_COLOR aircraft_parts_node5_uvs2[12] = {
#include "stg06/aclift/aircraft_parts_node5_uvs2.inc"
};

static NJS_COLOR aircraft_parts_node5_uvs3[3] = {
#include "stg06/aclift/aircraft_parts_node5_uvs3.inc"
};

static NJS_COLOR aircraft_parts_node5_uvs4[7] = {
#include "stg06/aclift/aircraft_parts_node5_uvs4.inc"
};

static NJS_COLOR aircraft_parts_node5_uvs5[38] = {
#include "stg06/aclift/aircraft_parts_node5_uvs5.inc"
};

static NJS_COLOR aircraft_parts_node5_uvs6[20] = {
#include "stg06/aclift/aircraft_parts_node5_uvs6.inc"
};

static NJS_COLOR aircraft_parts_node5_uvs8[18] = {
#include "stg06/aclift/aircraft_parts_node5_uvs8.inc"
};

static NJS_COLOR aircraft_parts_node5_uvs9[6] = {
#include "stg06/aclift/aircraft_parts_node5_uvs9.inc"
};

static NJS_MESHSET aircraft_parts_node5_meshsets[10] = {
#include "stg06/aclift/aircraft_parts_node5_meshsets.inc"
};

static NJS_POINT3 aircraft_parts_node5_points[54] = {
#include "stg06/aclift/aircraft_parts_node5_points.inc"
};

static NJS_VECTOR aircraft_parts_node5_normals[54] = {
#include "stg06/aclift/aircraft_parts_node5_normals.inc"
};

static NJS_MODEL aircraft_parts_node5_model = {
#include "stg06/aclift/aircraft_parts_node5_model.inc"
};

static NJS_OBJECT aircraft_parts_node5_object = {
#include "stg06/aclift/aircraft_parts_node5_object.inc"
};

static NJS_MATERIAL aircraft_parts_node4_materials[1] = {
#include "stg06/aclift/aircraft_parts_node4_materials.inc"
};

static Sint16 aircraft_parts_node4_meshes0[14] = {
#include "stg06/aclift/aircraft_parts_node4_meshes0.inc"
};

static NJS_COLOR aircraft_parts_node4_uvs0[12] = {
#include "stg06/aclift/aircraft_parts_node4_uvs0.inc"
};

static NJS_MESHSET aircraft_parts_node4_meshsets[1] = {
#include "stg06/aclift/aircraft_parts_node4_meshsets.inc"
};

static NJS_POINT3 aircraft_parts_node4_points[10] = {
#include "stg06/aclift/aircraft_parts_node4_points.inc"
};

static NJS_VECTOR aircraft_parts_node4_normals[10] = {
#include "stg06/aclift/aircraft_parts_node4_normals.inc"
};

static NJS_MODEL aircraft_parts_node4_model = {
#include "stg06/aclift/aircraft_parts_node4_model.inc"
};

static NJS_OBJECT aircraft_parts_node4_object = {
#include "stg06/aclift/aircraft_parts_node4_object.inc"
};

static NJS_MATERIAL aircraft_parts_node3_materials[1] = {
#include "stg06/aclift/aircraft_parts_node3_materials.inc"
};

static Sint16 aircraft_parts_node3_meshes0[18] = {
#include "stg06/aclift/aircraft_parts_node3_meshes0.inc"
};

static NJS_COLOR aircraft_parts_node3_uvs0[16] = {
#include "stg06/aclift/aircraft_parts_node3_uvs0.inc"
};

static NJS_MESHSET aircraft_parts_node3_meshsets[1] = {
#include "stg06/aclift/aircraft_parts_node3_meshsets.inc"
};

static NJS_POINT3 aircraft_parts_node3_points[16] = {
#include "stg06/aclift/aircraft_parts_node3_points.inc"
};

static NJS_VECTOR aircraft_parts_node3_normals[16] = {
#include "stg06/aclift/aircraft_parts_node3_normals.inc"
};

static NJS_MODEL aircraft_parts_node3_model = {
#include "stg06/aclift/aircraft_parts_node3_model.inc"
};

static NJS_OBJECT aircraft_parts_node3_object = {
#include "stg06/aclift/aircraft_parts_node3_object.inc"
};

static NJS_MATERIAL aircraft_parts_node2_materials[10] = {
#include "stg06/aclift/aircraft_parts_node2_materials.inc"
};

static Sint16 aircraft_parts_node2_meshes0[20] = {
#include "stg06/aclift/aircraft_parts_node2_meshes0.inc"
};

static Sint16 aircraft_parts_node2_meshes1[10] = {
#include "stg06/aclift/aircraft_parts_node2_meshes1.inc"
};

static Sint16 aircraft_parts_node2_meshes2[14] = {
#include "stg06/aclift/aircraft_parts_node2_meshes2.inc"
};

static Sint16 aircraft_parts_node2_meshes3[4] = {
#include "stg06/aclift/aircraft_parts_node2_meshes3.inc"
};

static Sint16 aircraft_parts_node2_meshes4[9] = {
#include "stg06/aclift/aircraft_parts_node2_meshes4.inc"
};

static Sint16 aircraft_parts_node2_meshes5[42] = {
#include "stg06/aclift/aircraft_parts_node2_meshes5.inc"
};

static Sint16 aircraft_parts_node2_meshes6[24] = {
#include "stg06/aclift/aircraft_parts_node2_meshes6.inc"
};

static Sint16 aircraft_parts_node2_meshes7[12] = {
#include "stg06/aclift/aircraft_parts_node2_meshes7.inc"
};

static Sint16 aircraft_parts_node2_meshes8[24] = {
#include "stg06/aclift/aircraft_parts_node2_meshes8.inc"
};

static Sint16 aircraft_parts_node2_meshes9[7] = {
#include "stg06/aclift/aircraft_parts_node2_meshes9.inc"
};

static NJS_COLOR aircraft_parts_node2_uvs0[16] = {
#include "stg06/aclift/aircraft_parts_node2_uvs0.inc"
};

static NJS_COLOR aircraft_parts_node2_uvs1[8] = {
#include "stg06/aclift/aircraft_parts_node2_uvs1.inc"
};

static NJS_COLOR aircraft_parts_node2_uvs2[12] = {
#include "stg06/aclift/aircraft_parts_node2_uvs2.inc"
};

static NJS_COLOR aircraft_parts_node2_uvs3[3] = {
#include "stg06/aclift/aircraft_parts_node2_uvs3.inc"
};

static NJS_COLOR aircraft_parts_node2_uvs4[7] = {
#include "stg06/aclift/aircraft_parts_node2_uvs4.inc"
};

static NJS_COLOR aircraft_parts_node2_uvs5[38] = {
#include "stg06/aclift/aircraft_parts_node2_uvs5.inc"
};

static NJS_COLOR aircraft_parts_node2_uvs6[20] = {
#include "stg06/aclift/aircraft_parts_node2_uvs6.inc"
};

static NJS_COLOR aircraft_parts_node2_uvs8[18] = {
#include "stg06/aclift/aircraft_parts_node2_uvs8.inc"
};

static NJS_COLOR aircraft_parts_node2_uvs9[6] = {
#include "stg06/aclift/aircraft_parts_node2_uvs9.inc"
};

static NJS_MESHSET aircraft_parts_node2_meshsets[10] = {
#include "stg06/aclift/aircraft_parts_node2_meshsets.inc"
};

static NJS_POINT3 aircraft_parts_node2_points[54] = {
#include "stg06/aclift/aircraft_parts_node2_points.inc"
};

static NJS_VECTOR aircraft_parts_node2_normals[54] = {
#include "stg06/aclift/aircraft_parts_node2_normals.inc"
};

static NJS_MODEL aircraft_parts_node2_model = {
#include "stg06/aclift/aircraft_parts_node2_model.inc"
};

static NJS_OBJECT aircraft_parts_node2_object = {
#include "stg06/aclift/aircraft_parts_node2_object.inc"
};

static NJS_MATERIAL aircraft_parts_node1_materials[10] = {
#include "stg06/aclift/aircraft_parts_node1_materials.inc"
};

static Sint16 aircraft_parts_node1_meshes0[58] = {
#include "stg06/aclift/aircraft_parts_node1_meshes0.inc"
};

static Sint16 aircraft_parts_node1_meshes1[20] = {
#include "stg06/aclift/aircraft_parts_node1_meshes1.inc"
};

static Sint16 aircraft_parts_node1_meshes2[8] = {
#include "stg06/aclift/aircraft_parts_node1_meshes2.inc"
};

static Sint16 aircraft_parts_node1_meshes3[8] = {
#include "stg06/aclift/aircraft_parts_node1_meshes3.inc"
};

static Sint16 aircraft_parts_node1_meshes4[12] = {
#include "stg06/aclift/aircraft_parts_node1_meshes4.inc"
};

static Sint16 aircraft_parts_node1_meshes5[15] = {
#include "stg06/aclift/aircraft_parts_node1_meshes5.inc"
};

static Sint16 aircraft_parts_node1_meshes6[25] = {
#include "stg06/aclift/aircraft_parts_node1_meshes6.inc"
};

static Sint16 aircraft_parts_node1_meshes7[12] = {
#include "stg06/aclift/aircraft_parts_node1_meshes7.inc"
};

static Sint16 aircraft_parts_node1_meshes8[5] = {
#include "stg06/aclift/aircraft_parts_node1_meshes8.inc"
};

static Sint16 aircraft_parts_node1_meshes9[10] = {
#include "stg06/aclift/aircraft_parts_node1_meshes9.inc"
};

static NJS_COLOR aircraft_parts_node1_uvs0[50] = {
#include "stg06/aclift/aircraft_parts_node1_uvs0.inc"
};

static NJS_COLOR aircraft_parts_node1_uvs1[15] = {
#include "stg06/aclift/aircraft_parts_node1_uvs1.inc"
};

static NJS_COLOR aircraft_parts_node1_uvs2[6] = {
#include "stg06/aclift/aircraft_parts_node1_uvs2.inc"
};

static NJS_COLOR aircraft_parts_node1_uvs3[6] = {
#include "stg06/aclift/aircraft_parts_node1_uvs3.inc"
};

static NJS_COLOR aircraft_parts_node1_uvs4[10] = {
#include "stg06/aclift/aircraft_parts_node1_uvs4.inc"
};

static NJS_COLOR aircraft_parts_node1_uvs5[12] = {
#include "stg06/aclift/aircraft_parts_node1_uvs5.inc"
};

static NJS_COLOR aircraft_parts_node1_uvs8[4] = {
#include "stg06/aclift/aircraft_parts_node1_uvs8.inc"
};

static NJS_COLOR aircraft_parts_node1_uvs9[8] = {
#include "stg06/aclift/aircraft_parts_node1_uvs9.inc"
};

static NJS_MESHSET aircraft_parts_node1_meshsets[10] = {
#include "stg06/aclift/aircraft_parts_node1_meshsets.inc"
};

static NJS_POINT3 aircraft_parts_node1_points[48] = {
#include "stg06/aclift/aircraft_parts_node1_points.inc"
};

static NJS_VECTOR aircraft_parts_node1_normals[48] = {
#include "stg06/aclift/aircraft_parts_node1_normals.inc"
};

static NJS_MODEL aircraft_parts_node1_model = {
#include "stg06/aclift/aircraft_parts_node1_model.inc"
};

static NJS_OBJECT aircraft_parts_node1_object = {
#include "stg06/aclift/aircraft_parts_node1_object.inc"
};

/* the aircraft, built of the pieces it breaks into, as the set editor
 * draws it */
static NJS_OBJECT aircraft_parts_object = {
#include "stg06/aclift/aircraft_parts_object.inc"
};

static NJS_MATERIAL aircraft_materials[20] = {
#include "stg06/aclift/aircraft_materials.inc"
};

static Sint16 aircraft_meshes0[114] = {
#include "stg06/aclift/aircraft_meshes0.inc"
};

static Sint16 aircraft_meshes1[48] = {
#include "stg06/aclift/aircraft_meshes1.inc"
};

static Sint16 aircraft_meshes2[29] = {
#include "stg06/aclift/aircraft_meshes2.inc"
};

static Sint16 aircraft_meshes3[71] = {
#include "stg06/aclift/aircraft_meshes3.inc"
};

static Sint16 aircraft_meshes4[76] = {
#include "stg06/aclift/aircraft_meshes4.inc"
};

static Sint16 aircraft_meshes5[32] = {
#include "stg06/aclift/aircraft_meshes5.inc"
};

static Sint16 aircraft_meshes6[134] = {
#include "stg06/aclift/aircraft_meshes6.inc"
};

static Sint16 aircraft_meshes7[8] = {
#include "stg06/aclift/aircraft_meshes7.inc"
};

static Sint16 aircraft_meshes8[112] = {
#include "stg06/aclift/aircraft_meshes8.inc"
};

static Sint16 aircraft_meshes9[26] = {
#include "stg06/aclift/aircraft_meshes9.inc"
};

static Sint16 aircraft_meshes10[10] = {
#include "stg06/aclift/aircraft_meshes10.inc"
};

static Sint16 aircraft_meshes11[20] = {
#include "stg06/aclift/aircraft_meshes11.inc"
};

static Sint16 aircraft_meshes12[33] = {
#include "stg06/aclift/aircraft_meshes12.inc"
};

static Sint16 aircraft_meshes13[20] = {
#include "stg06/aclift/aircraft_meshes13.inc"
};

static Sint16 aircraft_meshes14[60] = {
#include "stg06/aclift/aircraft_meshes14.inc"
};

static Sint16 aircraft_meshes15[18] = {
#include "stg06/aclift/aircraft_meshes15.inc"
};

static Sint16 aircraft_meshes16[60] = {
#include "stg06/aclift/aircraft_meshes16.inc"
};

static Sint16 aircraft_meshes17[37] = {
#include "stg06/aclift/aircraft_meshes17.inc"
};

static Sint16 aircraft_meshes18[4] = {
#include "stg06/aclift/aircraft_meshes18.inc"
};

static NJS_COLOR aircraft_uvs0[100] = {
#include "stg06/aclift/aircraft_uvs0.inc"
};

static NJS_COLOR aircraft_uvs1[36] = {
#include "stg06/aclift/aircraft_uvs1.inc"
};

static NJS_COLOR aircraft_uvs2[25] = {
#include "stg06/aclift/aircraft_uvs2.inc"
};

static NJS_COLOR aircraft_uvs3[57] = {
#include "stg06/aclift/aircraft_uvs3.inc"
};

static NJS_COLOR aircraft_uvs4[60] = {
#include "stg06/aclift/aircraft_uvs4.inc"
};

static NJS_COLOR aircraft_uvs5[26] = {
#include "stg06/aclift/aircraft_uvs5.inc"
};

static NJS_COLOR aircraft_uvs6[104] = {
#include "stg06/aclift/aircraft_uvs6.inc"
};

static NJS_COLOR aircraft_uvs7[6] = {
#include "stg06/aclift/aircraft_uvs7.inc"
};

static NJS_COLOR aircraft_uvs8[87] = {
#include "stg06/aclift/aircraft_uvs8.inc"
};

static NJS_COLOR aircraft_uvs9[20] = {
#include "stg06/aclift/aircraft_uvs9.inc"
};

static NJS_COLOR aircraft_uvs10[8] = {
#include "stg06/aclift/aircraft_uvs10.inc"
};

static NJS_COLOR aircraft_uvs11[15] = {
#include "stg06/aclift/aircraft_uvs11.inc"
};

static NJS_COLOR aircraft_uvs12[26] = {
#include "stg06/aclift/aircraft_uvs12.inc"
};

static NJS_COLOR aircraft_uvs13[16] = {
#include "stg06/aclift/aircraft_uvs13.inc"
};

static NJS_COLOR aircraft_uvs14[48] = {
#include "stg06/aclift/aircraft_uvs14.inc"
};

static NJS_COLOR aircraft_uvs15[14] = {
#include "stg06/aclift/aircraft_uvs15.inc"
};

static NJS_COLOR aircraft_uvs16[46] = {
#include "stg06/aclift/aircraft_uvs16.inc"
};

static NJS_COLOR aircraft_uvs17[30] = {
#include "stg06/aclift/aircraft_uvs17.inc"
};

static NJS_COLOR aircraft_uvs18[3] = {
#include "stg06/aclift/aircraft_uvs18.inc"
};

static NJS_MESHSET aircraft_meshsets[19] = {
#include "stg06/aclift/aircraft_meshsets.inc"
};

static NJS_POINT3 aircraft_points[268] = {
#include "stg06/aclift/aircraft_points.inc"
};

static NJS_VECTOR aircraft_normals[268] = {
#include "stg06/aclift/aircraft_normals.inc"
};

static NJS_MODEL aircraft_model = {
#include "stg06/aclift/aircraft_model.inc"
};

static NJS_OBJECT aircraft_object = {
#include "stg06/aclift/aircraft_object.inc"
};

static NJS_MATERIAL aclift_door_materials[3] = {
#include "stg06/aclift/aclift_door_materials.inc"
};

static Sint16 aclift_door_meshes0[3] = {
#include "stg06/aclift/aclift_door_meshes0.inc"
};

static Sint16 aclift_door_meshes1[4] = {
#include "stg06/aclift/aclift_door_meshes1.inc"
};

static Sint16 aclift_door_meshes2[12] = {
#include "stg06/aclift/aclift_door_meshes2.inc"
};

static Sint16 aclift_door_meshes3[12] = {
#include "stg06/aclift/aclift_door_meshes3.inc"
};

static NJS_COLOR aclift_door_uvs0[3] = {
#include "stg06/aclift/aclift_door_uvs0.inc"
};

static NJS_COLOR aclift_door_uvs1[4] = {
#include "stg06/aclift/aclift_door_uvs1.inc"
};

static NJS_COLOR aclift_door_uvs2[12] = {
#include "stg06/aclift/aclift_door_uvs2.inc"
};

static NJS_COLOR aclift_door_uvs3[12] = {
#include "stg06/aclift/aclift_door_uvs3.inc"
};

static NJS_MESHSET aclift_door_meshsets[4] = {
#include "stg06/aclift/aclift_door_meshsets.inc"
};

static NJS_POINT3 aclift_door_points[14] = {
#include "stg06/aclift/aclift_door_points.inc"
};

static NJS_VECTOR aclift_door_normals[14] = {
#include "stg06/aclift/aclift_door_normals.inc"
};

static NJS_MODEL aclift_door_model = {
#include "stg06/aclift/aclift_door_model.inc"
};

static NJS_OBJECT aclift_door_object = {
#include "stg06/aclift/aclift_door_object.inc"
};

static NJS_MATERIAL aclift_platform_materials[2] = {
#include "stg06/aclift/aclift_platform_materials.inc"
};

static Sint16 aclift_platform_meshes0[24] = {
#include "stg06/aclift/aclift_platform_meshes0.inc"
};

static Sint16 aclift_platform_meshes1[32] = {
#include "stg06/aclift/aclift_platform_meshes1.inc"
};

static Sint16 aclift_platform_meshes2[8] = {
#include "stg06/aclift/aclift_platform_meshes2.inc"
};

static NJS_COLOR aclift_platform_uvs0[24] = {
#include "stg06/aclift/aclift_platform_uvs0.inc"
};

static NJS_COLOR aclift_platform_uvs1[32] = {
#include "stg06/aclift/aclift_platform_uvs1.inc"
};

static NJS_COLOR aclift_platform_uvs2[8] = {
#include "stg06/aclift/aclift_platform_uvs2.inc"
};

static NJS_MESHSET aclift_platform_meshsets[3] = {
#include "stg06/aclift/aclift_platform_meshsets.inc"
};

static NJS_POINT3 aclift_platform_points[30] = {
#include "stg06/aclift/aclift_platform_points.inc"
};

static NJS_VECTOR aclift_platform_normals[30] = {
#include "stg06/aclift/aclift_platform_normals.inc"
};

static NJS_MODEL aclift_platform_model = {
#include "stg06/aclift/aclift_platform_model.inc"
};

static NJS_OBJECT aclift_platform_object = {
#include "stg06/aclift/aclift_platform_object.inc"
};

static NJS_TEXNAME dat_0cae4bc0[8] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};

static NJS_TEXLIST dat_0cae4c20 = {
#include "stg06/aclift/dat_0cae4c20.inc"
};

static NJS_MATERIAL aclift_lamp_materials[9] = {
#include "stg06/aclift/aclift_lamp_materials.inc"
};

static Sint16 aclift_lamp_meshes0[30] = {
#include "stg06/aclift/aclift_lamp_meshes0.inc"
};

static Sint16 aclift_lamp_meshes1[20] = {
#include "stg06/aclift/aclift_lamp_meshes1.inc"
};

static Sint16 aclift_lamp_meshes2[5] = {
#include "stg06/aclift/aclift_lamp_meshes2.inc"
};

static Sint16 aclift_lamp_meshes3[5] = {
#include "stg06/aclift/aclift_lamp_meshes3.inc"
};

static Sint16 aclift_lamp_meshes4[5] = {
#include "stg06/aclift/aclift_lamp_meshes4.inc"
};

static Sint16 aclift_lamp_meshes5[10] = {
#include "stg06/aclift/aclift_lamp_meshes5.inc"
};

static Sint16 aclift_lamp_meshes6[10] = {
#include "stg06/aclift/aclift_lamp_meshes6.inc"
};

static Sint16 aclift_lamp_meshes7[10] = {
#include "stg06/aclift/aclift_lamp_meshes7.inc"
};

static Sint16 aclift_lamp_meshes8[75] = {
#include "stg06/aclift/aclift_lamp_meshes8.inc"
};

static NJS_COLOR aclift_lamp_uvs0[24] = {
#include "stg06/aclift/aclift_lamp_uvs0.inc"
};

static NJS_COLOR aclift_lamp_uvs1[16] = {
#include "stg06/aclift/aclift_lamp_uvs1.inc"
};

static NJS_COLOR aclift_lamp_uvs2[4] = {
#include "stg06/aclift/aclift_lamp_uvs2.inc"
};

static NJS_COLOR aclift_lamp_uvs3[4] = {
#include "stg06/aclift/aclift_lamp_uvs3.inc"
};

static NJS_COLOR aclift_lamp_uvs4[4] = {
#include "stg06/aclift/aclift_lamp_uvs4.inc"
};

static NJS_COLOR aclift_lamp_uvs5[8] = {
#include "stg06/aclift/aclift_lamp_uvs5.inc"
};

static NJS_COLOR aclift_lamp_uvs6[8] = {
#include "stg06/aclift/aclift_lamp_uvs6.inc"
};

static NJS_COLOR aclift_lamp_uvs7[8] = {
#include "stg06/aclift/aclift_lamp_uvs7.inc"
};

static NJS_COLOR aclift_lamp_uvs8[60] = {
#include "stg06/aclift/aclift_lamp_uvs8.inc"
};

static NJS_MESHSET aclift_lamp_meshsets[9] = {
#include "stg06/aclift/aclift_lamp_meshsets.inc"
};

static NJS_POINT3 aclift_lamp_points[64] = {
#include "stg06/aclift/aclift_lamp_points.inc"
};

static NJS_VECTOR aclift_lamp_normals[64] = {
#include "stg06/aclift/aclift_lamp_normals.inc"
};

static NJS_MODEL aclift_lamp_model = {
#include "stg06/aclift/aclift_lamp_model.inc"
};

static NJS_OBJECT aclift_lamp_object = {
#include "stg06/aclift/aclift_lamp_object.inc"
};

static NJS_TEXNAME aclift_textures[34] = {
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0},
    {TEXT_37, 0, 0}, {TEXT_38, 0, 0}, {TEXT_39, 0, 0}, {TEXT_40, 0, 0},
    {TEXT_41, 0, 0}, {TEXT_42, 0, 0},
};

NJS_TEXLIST aclift_texlist = {
#include "stg06/aclift/aclift_texlist.inc"
};

static struct collision_shape aircraft_shapes[4] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1) | GUARD(2) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_PLAYERS),
     {12.0F, 1.0F, 0.0F},
     5.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CAPSULE | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_PLAYERS),
     {-1.0F, 1.0F, 1.0F},
     9.0F,
     3.5F,
     0.0F,
     {0, 0, 0x4000},
     0,
     NULL},
    {SHAPE_CAPSULE | SHAPE_HURTABLE,
     GUARD(2) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_PLAYERS),
     {-2.0F, -4.0F, 14.0F},
     5.0F,
     8.0F,
     0.0F,
     {0, 0, 0x4000},
     0,
     NULL},
    {SHAPE_CAPSULE | SHAPE_HURTABLE,
     GUARD(2) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_PLAYERS),
     {-2.0F, -4.0F, -14.0F},
     5.0F,
     8.0F,
     0.0F,
     {0, 0, 0x4000},
     0,
     NULL},
};

/* the platform, then the doors */
static struct collision_shape aclift_shapes[3] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_PLAYERS | PUSH_FIRM(PUSH_ALL),
     {0.0F, -6.0F, 0.0F},
     45.0F,
     5.0F,
     32.5F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_PLAYERS | PUSH_FIRM(PUSH_ALL),
     {0.0F, -1.0F, -22.75F},
     45.0F,
     1.0F,
     24.25F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_PLAYERS | PUSH_FIRM(PUSH_ALL),
     {0.0F, -1.0F, 22.75F},
     45.0F,
     1.0F,
     24.25F,
     {0, 0, 0},
     0,
     NULL},
};

/* the missile's body, then its blast, which hurts players. the blast is off
 * until the missile bursts */
static struct collision_shape missile_shapes[2] = {
    {SHAPE_SPHERE,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     10.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_OFF,
     ATTACK(2) | SHAPE_HURTS_PLAYERS,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     BLAST_START,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

static NJS_TEXLIST aircraft_lamp_texlist = {
#include "stg06/aclift/aircraft_lamp_texlist.inc"
};

static NJS_TEXANIM aircraft_lamp_frame = {
#include "stg06/aclift/aircraft_lamp_frame.inc"
};

static NJS_SPRITE aircraft_lamp_sprite = {
#include "stg06/aclift/aircraft_lamp_sprite.inc"
};

static NJS_MODEL *aircraft_pieces[7] = {
    &aircraft_parts_node7_model,
    &aircraft_parts_node6_model,
    &aircraft_parts_node4_model,
    &aircraft_parts_node3_model,
    &aircraft_parts_node13_model,
    &aircraft_parts_node12_model,
    NULL,
};

static NJS_TEXNAME aclift_lamp_textures[9] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_43, 0, 0}, {TEXT_5, 0, 0}, {TEXT_6, 0, 0},
    {TEXT_7, 0, 0},  {TEXT_8, 0, 0}, {TEXT_4, 0, 0},
};

NJS_TEXLIST aclift_lamp_texlist = {
#include "stg06/aclift/aclift_lamp_texlist.inc"
};

static NJS_TEXNAME dat_0cae5bc4[2] = {
    {TEXT_44, 0, 0},
    {TEXT_45, 0, 0},
};

static NJS_TEXLIST dat_0cae5bdc = {
#include "stg06/aclift/dat_0cae5bdc.inc"
};

static NJS_TEXANIM dat_0cae5be4[2] = {
#include "stg06/aclift/dat_0cae5be4.inc"
};

static NJS_SPRITE dat_0cae5c0c = {
#include "stg06/aclift/dat_0cae5c0c.inc"
};

static struct dust_params missile_flame = {
    1.0F,
    0.07F,
    1.8F,
    0.0F,
    0.0F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.5F, 1.0F, 0.7F, 0.3F},
};

static struct dust_params aircraft_flame = {
    1.25F,
    0.5F,
    0.9F,
    0.05F,
    0.0F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 0.3F, 0.3F, 1.0F},
};

static struct dust_params missile_smoke = {
    0.7F,
    0.07F,
    0.1F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.32F, 0.8F, 0.8F, 1.0F},
};

static struct dust_params aircraft_dust = {
    0.55F,
    0.08F,
    0.32F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.12F, 0.8F, 0.8F, 1.0F},
};

static void missile_destroy(struct task *task);
static void missile_step(struct task *task);
static void aircraft_destroy(struct task *task);
static void aircraft_step(struct task *task);
static void aclift_destroy(struct task *task);
static void aclift_run(struct task *task);

static void missile_start(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;

    collision_init(task, missile_shapes, COLLISION_ENEMY, 1, 2);
    record = enemy_alloc_record(task, work);
    record->flags |= ENEMY_SHADOW;
    work->tick |= 1;
    work->timer = 0;
    record->motion.slip = 0.0F;
    record->turn_speed = 0xe0;
    record->aim_y = work->ang.y;
    record->frame = 0.0F;
    record->probe_offset.x = 0.0F;
    record->probe_offset.y = 0.0F;
    record->probe_offset.z = 0.0F;
    record->top = 5.0F;
    record->radius = 5.0F;
    record->bottom = -5.0F;
    record->step_down = 50.0F;
    record->wall_bounce = 0.0F;
    record->floor_bounce = 0.0F;
    record->unk_c4 = 0.0F;
    record->shadow_size = 1.0F;
    record->shadow_ratio = 1.0F;
    work->ang.x = 0;
    work->ang.z = 0;
    task->destructor = missile_destroy;
    task->step = missile_step;
    work->flags |= MISSILE_UNK_100;
}

/* 1 once the missile touches a player, the ground, a wall or a ceiling, else 0.
 * 1 too once it comes within MISSILE_NEAR of its spot. it runs enemy_collide
 * only every other step. work->submode, which held the missile's wing until it
 * flew, toggles to pick those steps */
static int missile_collide(struct task_work *work, struct lockon_target *record)
{
    struct collision_work *collision;

    if (work->submode) {
        enemy_collide(work, record);
        work->submode = 0;
    } else {
        work->submode = 1;
    }
    /* @hack spelling: collision assigned inside the test, which stores it on
     * the stack and loads it back for the hit's kind (0x0c906ca6,
     * 0x0c906cb0). assigned ahead of the test, 48 of 61 words differ */
    if ((((collision = work->collision)->flags & COLLISION_HIT) &&
         collision->hit->kind == COLLISION_PLAYER) ||
        (record->flags & ENEMY_UNDER_CEILING) ||
        (record->flags & ENEMY_ON_WALL) || (record->flags & ENEMY_ON_GROUND) ||
        njDistanceP2P(&work->pos, &record->target) < MISSILE_NEAR) {
        return 1;
    }
    return 0;
}

static void missile_move(struct task_work *work, struct lockon_target *record)
{
    record->motion.accel.x += -record->motion.speed.x * MISSILE_DRAG;
    record->motion.accel.y += -record->motion.speed.y * MISSILE_DRAG;
    record->motion.accel.z += -record->motion.speed.z * MISSILE_DRAG;
    record->motion.speed.x += record->motion.accel.x;
    record->motion.speed.y += record->motion.accel.y;
    record->motion.speed.z += record->motion.accel.z;
    work->pos.x += record->motion.speed.x;
    work->pos.y += record->motion.speed.y;
    work->pos.z += record->motion.speed.z;
    record->motion.accel.x = 0.0F;
    record->motion.accel.y = 0.0F;
    record->motion.accel.z = 0.0F;
}

/* draw the missile while it is in view. from its second step in flight,
 * add its exhaust's smoke and flame */
static void missile_draw(struct task_work *work, struct lockon_target *record)
{
    NJS_POINT3 at;
    NJS_POINT3 pos;
    NJS_VECTOR dir;
    NJS_VECTOR speed;
    Angle ang;

    at = missile_exhaust_at;
    dir = missile_exhaust_dir;
    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 15.0F)) {
        njSetTexture(&aclift_texlist);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateZ, -work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        njRotateY(NULL, MODEL_TURN);
        draw_tree(&missile_object);
        njPopMatrix(1);
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateZ, -work->ang.z, ang);
        njTranslate(NULL, -4.0F, 0.0F, 0.0F);
        njCalcPoint(NULL, &at, &pos);
        njCalcVector(NULL, &dir, &speed);
        njPopMatrix(1);
        if (TASK_TIMER(work) > 1) {
            njAddVector(&pos, &work->pos);
            njAddVector(&speed, &record->motion.speed);
            missile_smoke.speed = speed;
            missile_smoke.pos = pos;
            dust_create_glow(&missile_smoke);
            speed.x *= 0.2F;
            speed.y *= 0.2F;
            speed.z *= 0.2F;
            missile_flame.speed = speed;
            missile_flame.pos = pos;
            panlight_create(&missile_flame);
        }
    }
}

/* grow the blast's shape, then hold it until the missile is gone, 20 steps
 * after it stops growing. nothing draws the blast. the material set here,
 * flashing every other step, is set back to 0 before anything draws */
static void missile_blast(struct task_work *work, struct lockon_target *record)
{
    struct collision_shape *blast = &work->collision->shapes[1];

    if (TASK_TIMER(work) % 2) {
        draw_set_material(work->scl.y, 1.0F, 0.0F, 0.0F);
    } else {
        draw_set_material(work->scl.y, 0.5F, 0.5F, 0.0F);
    }
    if (work->scl.x < BLAST_RADIUS) {
        work->scl.x += BLAST_GROWTH;
        blast->radius += BLAST_GROWTH;
    } else {
        work->scl.y -= BLAST_FADE;
        if (work->scl.y < -1.0F) {
            ENEMY_SET_MODE(work, record, MISSILE_GONE);
        }
    }
    work->ang.y += 0x100;
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

static void missile_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    enemy_end(task, work, record);
}

static void missile_wait(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    struct task_work *aircraft = task->parent->work;

    if (aircraft->mode == AIRCRAFT_AIM) {
        ENEMY_SET_MODE(work, record, MISSILE_LAUNCH);
    }
}

/* once the aircraft fires, the missile leaves its mount at the aircraft's
 * angles and speed. its ang.z takes the aircraft's PITCH. a missile's ang.z
 * above 0 points its nose down, so it leaves pitched the other way. it makes
 * its spot at player 0's pos, on the floor below where there is one. the
 * spot then follows player 0 along the floor. when task_create_child returns
 * null, task->child stays null. missile_fly then reads the spot's work
 * through it unchecked */
static void missile_launch(struct task *task, struct lockon_target *record)
{
    struct task_work *aircraft = task->parent->work;
    struct task_work *work = task->work;
    struct lockon_target *aircraft_record = LOCKON_TARGET(task->parent);
    struct player *player;
    Angle ang;

    if (aircraft->flags & AIRCRAFT_FIRE) {
        NJS_POINT3 origin;

        origin = missile_origin;
        work->ang.z = PITCH(aircraft_record);
        work->ang.y = aircraft->ang.y;
        work->ang.x = ROLL(aircraft_record);
        record->motion.speed = aircraft_record->motion.speed;
        njPushMatrix(&_unit_matrix_);
        njTranslate(NULL, aircraft->pos.x, aircraft->pos.y, aircraft->pos.z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        njRotateY(NULL, MODEL_TURN);
        njSetTexture(&aclift_texlist);
        if (work->submode == MISSILE_RIGHT) {
            njTranslate(NULL, MOUNT_X, MOUNT_Y, -MOUNT_Z);
        } else {
            njTranslate(NULL, MOUNT_X, MOUNT_Y, MOUNT_Z);
        }
        njCalcPoint(NULL, &origin, &work->pos);
        njPopMatrix(1);
        ENEMY_SET_MODE(work, record, MISSILE_FLY);
        player = player_get(0);
        record->target = player_work[0]->pos;
        if (player->shadow.below != -GROUND_FAR) {
            record->target.y = player->shadow.below;
        }
        if (task_create_child(TASK_WORK, missile_spot_step, task)) {
            task->child->work->pos = record->target;
            task->child->work->tick |= SPOT_FOLLOW | SPOT_TURN;
        }
    }
}

/* turn the missile's ang.z MISSILE_PITCH_SPEED toward the pitch that points
 * it at its target. an ang.z above 0 points its nose down. returns the pitch
 * it turns toward */
static Angle missile_pitch_to_target(struct task_work *work,
                                     struct lockon_target *record)
{
    Angle pitch;

    pitch = (Angle)(atan2f(njSqrt(SQUARE(record->target.x - work->pos.x) +
                                  SQUARE(record->target.z - work->pos.z)),
                           record->target.y - work->pos.y) *
                    ANGLE_PER_RADIAN) -
            0x4000;
    work->ang.z = angle_approach(work->ang.z, pitch, MISSILE_PITCH_SPEED);
    return pitch;
}

/* push the missile along its angles and spin it, and turn it toward its
 * spot. once it hits or nears the spot, it explodes. its body's shape then
 * turns off, and its blast's turns on */
static void missile_fly(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);
    Float across;
    Float up;

    across = njCos(work->ang.z) * MISSILE_THRUST;
    up = njSin(-work->ang.z) * MISSILE_THRUST;
    record->motion.accel.x = across * njCos(work->ang.y);
    record->motion.accel.y = up;
    record->motion.accel.z = across * njSin(-work->ang.y);
    work->ang.x += MISSILE_SPIN;
    record->turn_speed = MISSILE_TURN_SPEED;
    /* nothing reads the 1 in the missile's tick. the spot follows player 0
     * while its own tick holds 1, missile_spot.h's SPOT_FOLLOW, and nothing
     * clears that */
    if (njDistanceP2P(&work->pos, &player_work[0]->pos) < PLAYER_NEAR) {
        work->tick &= ~1;
    }
    record->target = task->child->work->pos;
    enemy_turn_to_target(work, record);
    missile_pitch_to_target(work, record);
    missile_move(work, record);
    if (missile_collide(work, record)) {
        ENEMY_SET_MODE(work, record, MISSILE_BLAST);
        work->scl.x = BLAST_START;
        work->scl.y = 0.0F;
        explosion_create_at(work->pos.x, work->pos.y + 3.0F, work->pos.z, 4.0F);
        work->collision->shapes[1].flags &= ~SHAPE_OFF;
        work->collision->shapes[0].flags |= SHAPE_OFF;
    }
    missile_draw(work, record);
    collision_register(work);
    task_step_children(task);
}

static void missile_step(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    if (edit_mode) {
        return;
    }
    if (placement_try_leave_by(task, LEAVE_RANGE)) {
        return;
    }
    switch (work->mode) {
    case MISSILE_WAIT:
        missile_wait(task, record);
        break;
    case MISSILE_LAUNCH:
        missile_launch(task, record);
        break;
    case MISSILE_FLY:
        missile_fly(task);
        break;
    case MISSILE_BLAST:
        missile_blast(work, record);
        collision_register(work);
        break;
    /* @hack redundant: an empty default ahead of MISSILE_GONE's case, which
     * puts task_kill's address in R3, not R2 (0x0c90754a); without it, 2 of 64
     * words differ */
    default:
        break;
    case MISSILE_GONE:
        task_kill(task);
        break;
    }
    work->timer++;
}

/* set up the aircraft, and make its two missiles, one under each wing */
static void aircraft_start(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;
    struct task *child;

    collision_init(task, aircraft_shapes, COLLISION_ENEMY, 1, 4);
    record = enemy_alloc_record(task, work);
    record->flags |= ENEMY_KEEP_ON_LEDGE | ENEMY_SHADOW;
    work->tick = 0;
    work->timer = 0;
    FLAME(record) = 0.0F;
    record->turn_speed = 0xe0;
    record->aim_y = work->ang.y;
    record->frame = 0.0F;
    record->shadow_size = 1.6F;
    record->shadow_ratio = 1.0F;
    FLAME(record) = 0.0F;
    BOB(record) = 0.0F;
    PITCH(record) = 0;
    ROLL_SPEED(record) = 0;
    work->ang.x = 0;
    work->ang.z = 0;
    enemy_set_home(work, record);
    task->destructor = aircraft_destroy;
    task->step = aircraft_step;
    work->flags |= AIRCRAFT_SOLID;
    work->flags &= ~(AIRCRAFT_GO | AIRCRAFT_FIRE);
    child = task_create_child(TASK_WORK, missile_start, task);
    if (child) {
        child->work->submode = MISSILE_RIGHT;
        MISSILE(child->work)->aircraft = work;
    }
    child = task_create_child(TASK_WORK, missile_start, task);
    if (child) {
        child->work->submode = MISSILE_LEFT;
        MISSILE(child->work)->aircraft = work;
    }
}

/* send player 0 flying away from the aircraft */
static void aircraft_push_away(struct task_work *work,
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
    speed.x = PUSH_SPEED;
    speed.y = PUSH_RISE;
    speed.z = 0.0F;
    player_send_flying(0, &speed, ang, PUSH_STEPS);
    player->pos.y += 1.0F;
}

/* a damaging hit sets AIRCRAFT_HIT, and the aircraft breaks on its next
 * step. where a player's shape did the damage, one that throws enemies sets
 * the aircraft's speed.y. any other sends player 0 up, whichever player hit,
 * and clears AIRCRAFT_SOLID. a player's touch that does no damage pushes
 * player 0 away */
static void aircraft_take_hit(struct task_work *work,
                              struct lockon_target *record)
{
    Sint16 shape;
    struct collision_work *collision;
    struct collision_work *hit;

    enemy_ignore((void *)player_work);
    shape = work->collision->hit_shape;
    collision = work->collision;
    hit = collision->hit;
    if (collision->flags & COLLISION_HIT) {
        if (work->flags & COLLISION_DAMAGED) {
            work->mode = AIRCRAFT_HIT;
            ENEMY_MARK_BEATEN(work);
            work->scl.z = HIT_SPRING;
            if (hit->kind == COLLISION_PLAYER) {
                if (hit->shapes[shape].flags & SHAPE_THROWS_ENEMIES) {
                    record->motion.speed.y = HIT_THROW;
                } else {
                    player_set_speed(0, 0.0F, HIT_BOUNCE, 0.0F);
                    work->flags &= ~AIRCRAFT_SOLID;
                }
            }
            work->flags &= ~COLLISION_DAMAGED;
            return;
        } else {
            /* @hack redundant: this else, after an arm that returns, loads the
             * hit's kind into R1, not R2 (0x0c9077e4); without it, 2 of 70
             * words differ */
            if (hit->kind == COLLISION_PLAYER) {
                aircraft_push_away(work, record);
            }
        }
    }
}

/* move the aircraft SPEED along its pitch and yaw, then keep SPEED_KEPT of
 * SPEED */
static void aircraft_move(struct task_work *work, struct lockon_target *record)
{
    Float speed = SPEED(record);

    record->motion.speed.y = speed * njSin(PITCH(record));
    speed *= njCos(PITCH(record));
    record->motion.speed.x = speed * njCos(work->ang.y);
    record->motion.speed.z = -(speed * njSin(work->ang.y));
    work->pos.x += record->motion.speed.x;
    work->pos.y += record->motion.speed.y;
    work->pos.z += record->motion.speed.z;
    SPEED(record) *= SPEED_KEPT;
}

/* draw the wing lamps in the model's frame aircraft_draw leaves current.
 * the left lamp is red and the right green, and they pulse together */
static void aircraft_draw_lamps(struct task_work *work)
{
    Float alpha = (njSin(VARS(work)->pulse * LAMP_PULSE) * 0.5F) + 0.5F;

    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_INVSRCCOLOR);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    draw_set_material(alpha, 1.0F, 0.3F, 0.3F);
    njPushMatrix(NULL);
    njTranslate(NULL, LAMP_X, LAMP_Y, LAMP_Z);
    njFogDisable();
    njDrawSprite3D(&aircraft_lamp_sprite, 0,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
    njFogEnable();
    njPopMatrix(1);
    draw_set_material(alpha, 0.3F, 1.0F, 0.3F);
    njPushMatrix(NULL);
    njTranslate(NULL, LAMP_X, LAMP_Y, -LAMP_Z);
    njFogDisable();
    njDrawSprite3D(&aircraft_lamp_sprite, 0,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
    njFogEnable();
    njPopMatrix(1);
}

/* draw the aircraft at pos raised by BOB, with its missiles until it fires. it
 * takes the scale it had before enemy_spring_scale springs it. while FLAME
 * is above 0, add its jets' flames, which don't bob */
static void aircraft_draw(struct task_work *work, struct lockon_target *record)
{
    Float scale_xz = work->scl.x;
    Float scale_y = work->scl.y;
    Angle ang;
    NJS_POINT3 right;
    NJS_POINT3 left;
    NJS_POINT3 origin;
    NJS_VECTOR speed;
    NJS_VECTOR dir;

    enemy_spring_scale(work, record);
    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 30.0F)) {
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njSetTexture(&aclift_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y + BOB(record), work->pos.z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateZ, PITCH(record), ang);
        TURN_VIA(njRotateX, ROLL(record), ang);
        njScale(NULL, scale_xz, scale_y, scale_xz);
        ang = MODEL_TURN;
        if (!(work->flags & AIRCRAFT_FIRE)) {
            njPushMatrix(NULL);
            njRotateY(NULL, ang);
            njTranslate(NULL, MOUNT_X, MOUNT_Y, -MOUNT_Z);
            /* @hack redundant: three calls of draw_tree that never run.
             * they keep njPushMatrix's address in R11 (0x0c907a3a). they keep
             * njPopMatrix's, 0x8000 and draw_tree's in R12, R8 and R9
             * (0x0c907a9c to 0x0c907aa2); without it, 24 of 344 words differ */
            if (0) {
                draw_tree(&missile_object);
                draw_tree(&missile_object);
                draw_tree(&missile_object);
            }
            draw_tree(&missile_object);
            njPopMatrix(1);
            njPushMatrix(NULL);
            njRotateY(NULL, ang);
            njTranslate(NULL, MOUNT_X, MOUNT_Y, MOUNT_Z);
            draw_tree(&missile_object);
            njPopMatrix(1);
        }
        njRotateY(NULL, ang);
        draw_tree(&aircraft_object);
        aircraft_draw_lamps(work);
        njPopMatrix(1);
        if (FLAME(record) > 0.0F) {
            dir.x = -1.0F;
            dir.y = 0.0F;
            dir.z = 0.0F;
            njPushMatrix(&_unit_matrix_);
            TURN_VIA(njRotateY, work->ang.y, ang);
            TURN_VIA(njRotateZ, PITCH(record), ang);
            TURN_VIA(njRotateX, ROLL(record), ang);
            njCalcVector(NULL, &dir, &speed);
            njPopMatrix(1);
            origin.z = 0.0F;
            origin.y = 0.0F;
            origin.x = 0.0F;
            njPushMatrix(&_unit_matrix_);
            njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
            TURN_VIA(njRotateY, work->ang.y, ang);
            TURN_VIA(njRotateZ, PITCH(record), ang);
            TURN_VIA(njRotateX, ROLL(record), ang);
            njScale(NULL, scale_xz, scale_y, scale_xz);
            njPushMatrix(NULL);
            njTranslate(NULL, JET_X, JET_Y, JET_Z);
            njCalcPoint(NULL, &origin, &right);
            njPopMatrix(1);
            njPushMatrix(NULL);
            njTranslate(NULL, JET_X, JET_Y, -JET_Z);
            njCalcPoint(NULL, &origin, &left);
            njPopMatrix(1);
            njPopMatrix(1);
            aircraft_flame.speed.x = speed.x;
            aircraft_flame.speed.y = speed.y;
            aircraft_flame.speed.z = speed.z;
            aircraft_flame.color.a = FLAME(record);
            aircraft_flame.pos = right;
            panlight_create(&aircraft_flame);
            aircraft_flame.pos = left;
            panlight_create(&aircraft_flame);
        }
    }
}

/* draw the aircraft as aircraft_parts_object, for the set editor */
static void aircraft_draw_parts(struct task_work *work)
{
    Angle ang;

    njSetTexture(&aclift_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njRotateY(NULL, MODEL_TURN);
    draw_tree(&aircraft_parts_object);
    njPopMatrix(1);
}

static void aircraft_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    enemy_end(task, work, record);
}

/* break the aircraft into aircraft_pieces with an explosion and debris,
 * and free an animal */
static void aircraft_break(struct task_work *work, struct lockon_target *record)
{
    explosion_create_at(work->pos.x, work->pos.y + 5.0F, work->pos.z, 1.4F);
    enemy_break(work, aircraft_pieces, &aclift_texlist);
    enemy_scatter_debris(work, DEBRIS_COUNT);
    animal_create_random(3, work->pos.x, work->pos.y + 15.0F, work->pos.z);
}

static void aircraft_wait(struct task_work *work, struct lockon_target *record)
{
    aircraft_take_hit(work, record);
    aircraft_draw(work, record);
    if (work->flags & AIRCRAFT_GO) {
        ENEMY_SET_MODE(work, record, AIRCRAFT_IGNITE);
        work->flags &= ~AIRCRAFT_GO;
    }
    if (work->flags & AIRCRAFT_SOLID) {
        collision_register(work);
    }
}

/* the jets build and the nose dips, and the aircraft bobs on the lift */
static void aircraft_ignite(struct task_work *work,
                            struct lockon_target *record)
{
    PITCH(record) = angle_approach(PITCH(record), DIP_PITCH, DIP_TILT);
    BOB(record) = FLAME(record) * njSin(BOB_PHASE(record)) * BOB_SHARE;
    BOB_PHASE(record) += BOB_SPEED;
    if (FLAME(record) < FLAME_FULL) {
        FLAME(record) += FLAME_GROWTH;
    }
    aircraft_move(work, record);
    aircraft_take_hit(work, record);
    aircraft_draw(work, record);
    if (TASK_TIMER(work) > IGNITE_STEPS) {
        FLAME(record) = FLAME_FULL;
        BOB_SIZE(record) = BOB_START;
        ENEMY_SET_MODE(work, record, AIRCRAFT_TAKE_OFF);
    }
    if (work->flags & AIRCRAFT_SOLID) {
        collision_register(work);
    }
}

/* blow three puffs of dust out from under the aircraft. the first leaves
 * at a random angle */
static void aircraft_blow_dust(struct task_work *work,
                               struct lockon_target *record)
{
    NJS_POINT3 at;
    NJS_POINT3 pos;
    NJS_VECTOR dir;
    NJS_VECTOR speed;
    Angle ang;
    int i;

    at = aircraft_dust_at;
    dir = aircraft_dust_dir;
    /* any angle short of a whole turn: rand() is at most RAND_MAX, 32767 */
    ang = (Angle)(rand() * 1.9999999F);
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y - DUST_DEPTH, work->pos.z);
    for (i = 0; i < 3; i++) {
        njPushMatrix(NULL);
        TURN(njRotateY, ang + (i * THIRD_TURN));
        njCalcVector(NULL, &dir, &speed);
        njCalcPoint(NULL, &at, &pos);
        aircraft_dust.speed = speed;
        aircraft_dust.pos = pos;
        dust_create_glow(&aircraft_dust);
        njPopMatrix(1);
    }
    njPopMatrix(1);
}

/* the aircraft levels out and gathers speed as its bob dies away, and it
 * blows dust. it then goes the way its submode says */
static void aircraft_take_off(struct task_work *work,
                              struct lockon_target *record)
{
    PITCH(record) = angle_approach(PITCH(record), 0, TILT_SLOW);
    switch (work->submode) {
    case AIRCRAFT_LEFT:
        if (TASK_TIMER(work) > TAKE_OFF_STEPS) {
            ENEMY_SET_MODE(work, record, AIRCRAFT_TURN_LEFT);
        }
        break;
    case AIRCRAFT_RIGHT:
        if (TASK_TIMER(work) > TAKE_OFF_STEPS) {
            ENEMY_SET_MODE(work, record, AIRCRAFT_TURN_RIGHT);
        }
        break;
    case AIRCRAFT_AHEAD:
        if (TASK_TIMER(work) > AHEAD_STEPS) {
            ENEMY_SET_MODE(work, record, AIRCRAFT_RISE);
        }
        break;
    }
    if (SPEED(record) < SPEED_CAP) {
        SPEED(record) += AIRCRAFT_THRUST;
    }
    if (BOB_SIZE(record) > 0.0F) {
        BOB(record) = BOB_SIZE(record) * njSin(BOB_PHASE(record));
        BOB_PHASE(record) += BOB_SPEED;
        BOB_SIZE(record) -= BOB_DECAY;
    } else {
        BOB(record) = 0.0F;
    }
    aircraft_move(work, record);
    aircraft_take_hit(work, record);
    aircraft_draw(work, record);
    aircraft_blow_dust(work, record);
    if (work->flags & AIRCRAFT_SOLID) {
        collision_register(work);
    }
}

/* AIRCRAFT_SET_OFF's step, which never runs. the aircraft pitches up, then
 * goes the way its submode says */
static void aircraft_set_off(struct task_work *work,
                             struct lockon_target *record)
{
    PITCH(record) = angle_approach(PITCH(record), 0x1800, 0xb0);
    /* aircraft_move reads accel.x as SPEED, and never reads accel.z */
    record->motion.accel.x = njCos(work->ang.y) * 0.1F;
    record->motion.accel.z = njSin(-work->ang.y) * 0.1F;
    aircraft_move(work, record);
    aircraft_take_hit(work, record);
    aircraft_draw(work, record);
    switch (work->submode) {
    case AIRCRAFT_LEFT:
        ENEMY_SET_MODE(work, record, AIRCRAFT_TURN_LEFT);
        break;
    case AIRCRAFT_RIGHT:
        ENEMY_SET_MODE(work, record, AIRCRAFT_TURN_RIGHT);
        break;
    case AIRCRAFT_AHEAD:
        ENEMY_SET_MODE(work, record, AIRCRAFT_RISE);
        break;
    }
    if (work->flags & AIRCRAFT_SOLID) {
        collision_register(work);
    }
}

/* the aircraft climbs, then banks left and turns. later its pitch levels
 * and it turns faster, then aims. it registers no shapes while it turns */
static void aircraft_turn_left(struct task_work *work,
                               struct lockon_target *record)
{
    if (TASK_TIMER(work) > TURN_LEVEL_STEPS) {
        ROLL(record) = angle_approach(ROLL(record), -BANK, TILT_FAST);
        PITCH(record) = angle_approach(PITCH(record), 0, TILT_FAST);
        work->ang.y += TURN_FAST;
    } else if (TASK_TIMER(work) > TURN_CLIMB_STEPS) {
        ROLL(record) = angle_approach(ROLL(record), -BANK, TILT_FAST);
        PITCH(record) = angle_approach(PITCH(record), CLIMB_PITCH, TILT_SLOW);
        work->ang.y += TURN_SLOW;
    } else {
        PITCH(record) = angle_approach(PITCH(record), CLIMB_PITCH, TILT_FAST);
    }
    if (SPEED(record) < TURN_SPEED_CAP) {
        SPEED(record) += AIRCRAFT_THRUST;
    }
    aircraft_move(work, record);
    aircraft_draw(work, record);
    if (TASK_TIMER(work) > TURN_STEPS) {
        ENEMY_SET_MODE(work, record, AIRCRAFT_AIM);
    }
}

/* aircraft_turn_left's turn, to the right. it banks more slowly before
 * TURN_LEVEL_STEPS, and its pitch levels more slowly after */
static void aircraft_turn_right(struct task_work *work,
                                struct lockon_target *record)
{
    if (TASK_TIMER(work) > TURN_LEVEL_STEPS) {
        ROLL(record) = angle_approach(ROLL(record), BANK, TILT_FAST);
        PITCH(record) = angle_approach(PITCH(record), 0, TILT_SLOW);
        work->ang.y -= TURN_FAST;
    } else if (TASK_TIMER(work) > TURN_CLIMB_STEPS) {
        ROLL(record) = angle_approach(ROLL(record), BANK, TILT_SLOW);
        PITCH(record) = angle_approach(PITCH(record), CLIMB_PITCH, TILT_SLOW);
        work->ang.y -= TURN_SLOW;
    } else {
        PITCH(record) = angle_approach(PITCH(record), CLIMB_PITCH, TILT_FAST);
    }
    if (SPEED(record) < TURN_SPEED_CAP) {
        SPEED(record) += AIRCRAFT_THRUST;
    }
    aircraft_move(work, record);
    aircraft_draw(work, record);
    if (TASK_TIMER(work) > TURN_STEPS) {
        ENEMY_SET_MODE(work, record, AIRCRAFT_AIM);
    }
}

static void aircraft_rise(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    PITCH(record) = angle_approach(PITCH(record), CLIMB_PITCH, TILT_FAST);
    if (SPEED(record) < SPEED_CAP) {
        SPEED(record) += AIRCRAFT_THRUST;
    }
    aircraft_move(work, record);
    aircraft_draw(work, record);
}

/* the aircraft turns to player 0 and dips. it fires once it faces near
 * enough, then leaves. low over its home, it leaves without firing */
static void aircraft_aim(struct task_work *work, struct lockon_target *record)
{
    int off;

    record->target = player_work[0]->pos;
    off = enemy_turn_to_target(work, record);
    PITCH(record) = angle_approach(PITCH(record), AIM_PITCH, TILT_SLOW);
    if (SPEED(record) < TURN_SPEED_CAP) {
        SPEED(record) += AIRCRAFT_THRUST;
    }
    if (off < FIRE_ANGLE) {
        ENEMY_SET_MODE(work, record, AIRCRAFT_LEAVE);
        work->flags |= AIRCRAFT_FIRE;
    } else if (work->pos.y < record->home.y + AIM_FLOOR) {
        ENEMY_SET_MODE(work, record, AIRCRAFT_LEAVE);
    }
    aircraft_move(work, record);
    aircraft_take_hit(work, record);
    aircraft_draw(work, record);
    if (work->flags & AIRCRAFT_SOLID) {
        collision_register(work);
    }
}

/* the aircraft turns to a yaw of 0, then rolls ever faster. it flies level
 * once above LEAVE_HEIGHT */
static void aircraft_leave(struct task_work *work, struct lockon_target *record)
{
    if (angle_distance(work->ang.y, 0) == 0) {
        ROLL_SPEED(record) += ROLL_GROWTH;
        if (ROLL_SPEED(record) > ROLL_SPEED_CAP) {
            ROLL_SPEED(record) = ROLL_SPEED_CAP;
        }
        ROLL(record) += ROLL_SPEED(record);
    } else {
        work->ang.y = angle_approach(work->ang.y, 0, LEAVE_TURN);
    }
    if (work->pos.y < LEAVE_HEIGHT) {
        PITCH(record) = angle_approach(PITCH(record), CLIMB_PITCH, TILT_FAST);
    } else {
        PITCH(record) = angle_approach(PITCH(record), 0, TILT_FAST);
    }
    if (SPEED(record) < SPEED_CAP) {
        SPEED(record) += AIRCRAFT_THRUST;
    }
    aircraft_move(work, record);
    aircraft_draw(work, record);
}

static void aircraft_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct lockon_target *record = LOCKON_TARGET(task);

    if (edit_mode) {
        /* @hack redundant: a test of player that never runs. it keeps the
         * image's load of player 0's work, which nothing uses (0x0c908910,
         * 0x0c908918); without it, 18 of 117 words differ */
        if (0) {
            if (player) {
            }
        }
        aircraft_draw_parts(work);
        return;
    }
    if (placement_try_leave_by(task, LEAVE_RANGE)) {
        return;
    }
    VARS(work)->pulse++;
    switch (work->mode) {
    case AIRCRAFT_WAIT:
        aircraft_wait(work, record);
        break;
    case AIRCRAFT_IGNITE:
        aircraft_ignite(work, record);
        break;
    case AIRCRAFT_TAKE_OFF:
        aircraft_take_off(work, record);
        break;
    case AIRCRAFT_SET_OFF:
        aircraft_set_off(work, record);
        break;
    case AIRCRAFT_TURN_LEFT:
        aircraft_turn_left(work, record);
        break;
    case AIRCRAFT_TURN_RIGHT:
        aircraft_turn_right(work, record);
        break;
    case AIRCRAFT_RISE:
        aircraft_rise(task);
        break;
    case AIRCRAFT_AIM:
        aircraft_aim(work, record);
        break;
    case AIRCRAFT_LEAVE:
        aircraft_leave(work, record);
        break;
    case AIRCRAFT_HIT:
        aircraft_break(work, record);
        /* @hack spelling: the two calls as one expression, which keeps the call
         * to task_kill a JSR (0x0c908a16). as two statements, SHC makes it a
         * tail jump, and 29 of 117 words differ */
        enemy_end(task, work, record), task_kill(task);
        return;
    /* @hack redundant: an empty default, which puts enemy_save_pos's address in
     * R3, not R2 (0x0c908a1e); without it, 2 of 117 words differ */
    default:
        break;
    }
    enemy_save_pos(work, record);
    work->timer++;
    task_step_children(task);
}

void aclift_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, aclift_shapes, COLLISION_OBJECT, 1, 3);
    work->tick = 0;
    work->timer = 0;
    work->scl.x = 0.0F;
    work->scl.y = PLATFORM_DOWN;
    work->ang.x = 0;
    work->ang.z = 0;
    task->destructor = aclift_destroy;
    task->step = aclift_run;
    work->flags &= ~ACLIFT_LOADED;
}

/* draw the doors scl.x open, and the platform scl.y from the top, shaken.
 * each draw sets the lift lamp's texture first. so the reset to
 * LIFT_LAMP_LOADED at the end comes to nothing */
static void aclift_draw(struct task_work *work)
{
    Angle ang;
    NJS_OBJECT *lamp = &aclift_lamp_object;
    /* @hack redundant: lit, a select of lamp whose condition is constant. it
     * copies the lamp's address to R4 in the slot of the BT/S that tests
     * ACLIFT_LOADED, and the texture's store goes through R4 (BT/S and MOV
     * R11,R4 at 0x0c908ad4 and 0x0c908ad6). as lit = lamp,
     * njPushMatrix's and njTranslate's addresses trade R12 and R13, and 21 of
     * 135 words differ */
    /* NOLINTNEXTLINE(misc-redundant-expression) */
    NJS_OBJECT *lit = 1.0F ? lamp : lamp;

    if (work->flags & ACLIFT_LOADED) {
        lit->model->mats[LIFT_LAMP_MATERIAL].attr_texId = LIFT_LAMP_LOADED;
    } else {
        lit->model->mats[LIFT_LAMP_MATERIAL].attr_texId = LIFT_LAMP_EMPTY;
    }
    njSetTexture(&aclift_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    /* @hack redundant: tests of njTranslate's and njPushMatrix's addresses
     * that never run. they keep the lamp's address in R11 (0x0c908ad0), and
     * njPushMatrix's and njTranslate's in R12 and R13 (0x0c908b32,
     * 0x0c908b3a); without it, 18 of 135 words differ */
    if (0) {
        /* NOLINTBEGIN(clang-diagnostic-pointer-bool-conversion) */
        if (njTranslate) {
        }
        if (njPushMatrix) {
        }
        if (njPushMatrix) {
        }
        /* NOLINTEND(clang-diagnostic-pointer-bool-conversion) */
    }
    /* @hack redundant: a goto to the next statement, which SHC drops. it puts
     * the turn's ang.y in R3, not R2 (0x0c908b4a); without it, 3 of 135 words
     * differ */
    goto turn;
turn:
    TURN_VIA(njRotateY, work->ang.y, ang);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, -work->scl.x);
    draw_tree(&aclift_door_object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, work->scl.x);
    njRotateY(NULL, HALF_TURN);
    draw_tree(&aclift_door_object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, work->scl.y + LIFT(work)->shake, 0.0F);
    draw_tree(&aclift_platform_object);
    njTranslate(NULL, LIFT_LAMP_X, 0.0F, 0.0F);
    njRotateY(NULL, LIFT_LAMP_TURN);
    njSetTexture(&aclift_lamp_texlist);
    draw_tree(lamp);
    njPopMatrix(1);
    njPopMatrix(1);
    lamp->model->mats[LIFT_LAMP_MATERIAL].attr_texId = LIFT_LAMP_LOADED;
}

static void aclift_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* with no aircraft aboard or out, make one once player 0 is within
 * CALL_RANGE, and open. the aircraft starts on the platform down */
static void aclift_wait(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *aircraft;

    if (!(work->flags & ACLIFT_LOADED)) {
        if (!task->child) {
            if (njDistanceP2P(&work->pos, &player_work[0]->pos) < CALL_RANGE) {
                if (task_create_child(TASK_WORK, aircraft_start, task)) {
                    aircraft = task->child->work;
                    /* @hack redundant: an empty if that never runs, with an
                     * empty else. it sends the copy of the lift's pos through
                     * R2, from 0x0c908ca8; without it, 8 of 85 words differ */
                    if (0) {
                    } else {
                    }
                    aircraft->pos = work->pos;
                    aircraft->pos.y -= START_DEPTH;
                    aircraft->ang.y = work->ang.y - START_TURN;
                    if (work->scl.z > 0.0F) {
                        aircraft->submode = AIRCRAFT_RIGHT;
                    } else if (work->scl.z > LEFT_FLOOR) {
                        aircraft->submode = AIRCRAFT_LEFT;
                    } else {
                        aircraft->submode = AIRCRAFT_AHEAD;
                        aircraft->ang.y += HALF_TURN;
                    }
                    work->flags |= ACLIFT_LOADED;
                    /* @hack redundant: a store of ACLIFT_OPEN that never
                     * runs. it keeps 0 in R6 and 1 in R5, not the other way
                     * round (0x0c908ca2, 0x0c908cce); without it, 6 of 85
                     * words differ */
                    if (0) {
                        work->mode = ACLIFT_OPEN;
                    }
                    work->mode = ACLIFT_OPEN;
                    work->timer = 0;
                }
            }
        }
    }
    aclift_draw(work);
}

static void aclift_open(struct task_work *work)
{
    work->scl.x += DOOR_SPEED;
    if (work->scl.x > DOOR_OPEN) {
        work->scl.x = DOOR_OPEN;
        work->mode = ACLIFT_RAISE;
        work->timer = 0;
    }
    aclift_draw(work);
}

static void aclift_raise(struct task *task)
{
    struct task_work *work = task->work;

    work->scl.y += PLATFORM_SPEED;
    if (work->flags & ACLIFT_LOADED) {
        if (task->child) {
            if (task->child->work->mode != AIRCRAFT_LANDING) {
                task->child->work->pos.y += PLATFORM_SPEED;
            }
        }
    }
    if (work->scl.y > 0.0F) {
        work->scl.y = 0.0F;
        work->mode = ACLIFT_LAUNCH;
        work->timer = 0;
    }
    aclift_draw(work);
}

/* shake the platform, and let the aircraft go. the lift holds the aircraft
 * at SEAT over the platform throughout, let go or not */
static void aclift_launch(struct task *task)
{
    struct task_work *work = task->work;

    if (TASK_TIMER(work) > LAUNCH_STEPS) {
        work->mode = ACLIFT_LOWER;
        work->timer = 0;
    }
    if (TASK_TIMER(work) < SHAKE_END) {
        LIFT(work)->shake = njSin(LIFT(work)->phase) * SHAKE_SIZE;
        LIFT(work)->phase += SHAKE_SPEED;
    } else {
        LIFT(work)->shake = 0.0F;
    }
    if (task->child) {
        task->child->work->pos.y = work->pos.y + LIFT(work)->shake + SEAT;
    }
    if (TASK_TIMER(work) > RELEASE_STEPS && (work->flags & ACLIFT_LOADED)) {
        if (task->child) {
            task->child->work->flags |= AIRCRAFT_GO;
        }
        work->flags &= ~ACLIFT_LOADED;
    }
    aclift_draw(work);
}

static void aclift_lower(struct task *task)
{
    struct task_work *work = task->work;

    work->scl.y -= PLATFORM_SPEED;
    if (work->flags & ACLIFT_LOADED) {
        if (task->child) {
            if (task->child->work->mode == AIRCRAFT_LANDING) {
                task->child->work->pos.y -= PLATFORM_SPEED;
            }
        }
    }
    if (work->scl.y < PLATFORM_DOWN) {
        work->scl.y = PLATFORM_DOWN;
        work->mode = ACLIFT_CLOSE;
        work->timer = 0;
    }
    aclift_draw(work);
}

static void aclift_close(struct task_work *work)
{
    work->scl.x -= DOOR_SPEED;
    if (work->scl.x < 0.0F) {
        work->scl.x = 0.0F;
        work->mode = ACLIFT_WAIT;
        work->timer = 0;
    }
    aclift_draw(work);
}

/* run the lift, then its aircraft. the lift's shapes follow the platform
 * and the doors */
static void aclift_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct collision_work *collision = work->collision;
    struct collision_shape *shape;

    if (edit_mode) {
        /* @hack redundant: a test of player that never runs. it keeps the
         * image's load of player 0's work, which nothing uses (0x0c908f22,
         * 0x0c908f26); without it, 12 of 109 words differ */
        if (0) {
            if (player) {
            }
        }
        aclift_draw(work);
    /* @hack redundant: a label nothing jumps to, which keeps the call of
     * aclift_draw a BSR (0x0c908f30). without it, SHC makes it a tail jump,
     * and 4 of 109 words differ */
    drawn: /* NOLINT(clang-diagnostic-unused-label) */
        return;
    }
    if (!task->parent && placement_try_leave_by(task, LEAVE_RANGE)) {
        return;
    }
    switch (work->mode) {
    case ACLIFT_WAIT:
        aclift_wait(task);
        break;
    case ACLIFT_OPEN:
        aclift_open(work);
        break;
    case ACLIFT_RAISE:
        aclift_raise(task);
        break;
    case ACLIFT_LAUNCH:
        aclift_launch(task);
        break;
    case ACLIFT_LOWER:
        aclift_lower(task);
        break;
    case ACLIFT_CLOSE:
        aclift_close(work);
        break;
    }
    work->timer++;
    shape = collision->shapes;
    shape->center.y = work->scl.y - PLATFORM_SHAPE_DROP;
    shape++;
    shape->center.z = -work->scl.x - DOOR_SHAPE_OUT;
    shape++;
    shape->center.z = work->scl.x + DOOR_SHAPE_OUT;
    collision_register(work);
    task_step_children(task);
}
