/* O KSONICZO: the Sonic statue, which bursts into pieces when an attack
 * hurts it. O HAHEN1 to O HAHEN6: six shards, drawn as a burst ends.
 * O SONZCOL: a collision sphere, registered as a burst ends */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/burst_statue.h"
#include "stg09/casino.h"
#include "stg09/helpers.h"
#include "stg09/sonic_statue.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* the statue as it stands, and its glow */
extern NJS_OBJECT burst_statue_object;
extern NJS_OBJECT burst_statue_glow_object;
/* the object the statue lends a copy of to the collision world */
extern NJS_OBJECT burst_statue_world_object;
/* the shards' trees */
extern NJS_OBJECT statue_shard_object_1;
extern NJS_OBJECT statue_shard_object_2;
extern NJS_OBJECT statue_shard_object_3;
extern NJS_OBJECT statue_shard_object_4;
extern NJS_OBJECT statue_shard_object_5;
extern NJS_OBJECT statue_shard_object_6;
/* the objects the statue bursts into. each gives its piece a start and a
 * model */
extern NJS_OBJECT burst_statue_piece_0;
extern NJS_OBJECT burst_statue_piece_1;
extern NJS_OBJECT burst_statue_piece_2;
extern NJS_OBJECT burst_statue_piece_3;
extern NJS_OBJECT burst_statue_piece_4;
extern NJS_OBJECT burst_statue_piece_5;
extern NJS_OBJECT burst_statue_piece_6;
extern NJS_OBJECT burst_statue_piece_7;
extern NJS_OBJECT burst_statue_piece_8;
extern NJS_OBJECT burst_statue_piece_9;
extern NJS_OBJECT burst_statue_piece_10;
extern NJS_OBJECT burst_statue_piece_11;
extern NJS_OBJECT burst_statue_piece_12;
extern NJS_OBJECT burst_statue_piece_13;
extern NJS_OBJECT burst_statue_piece_14;
extern NJS_OBJECT burst_statue_piece_15;
extern NJS_OBJECT burst_statue_piece_16;
extern NJS_OBJECT burst_statue_piece_17;
extern NJS_OBJECT burst_statue_piece_18;
extern NJS_OBJECT burst_statue_piece_19;
extern NJS_OBJECT burst_statue_piece_20;
extern NJS_OBJECT burst_statue_piece_21;
extern NJS_OBJECT burst_statue_piece_22;
extern NJS_OBJECT burst_statue_piece_23;
extern NJS_OBJECT burst_statue_piece_24;
extern NJS_OBJECT burst_statue_piece_25;
extern NJS_OBJECT burst_statue_piece_26;
extern NJS_OBJECT burst_statue_piece_27;
extern NJS_OBJECT burst_statue_piece_28;
extern NJS_OBJECT burst_statue_piece_29;
extern NJS_OBJECT burst_statue_piece_30;
extern NJS_OBJECT burst_statue_piece_31;
extern NJS_OBJECT burst_statue_piece_32;
extern NJS_OBJECT burst_statue_piece_33;
extern NJS_OBJECT burst_statue_piece_34;
extern NJS_OBJECT burst_statue_piece_35;
extern NJS_OBJECT burst_statue_piece_36;
extern NJS_OBJECT burst_statue_piece_37;
extern NJS_OBJECT burst_statue_piece_38;
extern NJS_OBJECT burst_statue_piece_39;
extern NJS_OBJECT burst_statue_piece_40;
extern NJS_OBJECT burst_statue_piece_41;
extern NJS_OBJECT burst_statue_piece_42;
extern NJS_OBJECT burst_statue_piece_43;
extern NJS_OBJECT burst_statue_piece_44;
extern NJS_OBJECT burst_statue_piece_45;
extern NJS_OBJECT burst_statue_piece_46;
extern NJS_OBJECT burst_statue_piece_47;
extern NJS_OBJECT burst_statue_piece_48;
extern NJS_OBJECT burst_statue_piece_49;
extern NJS_OBJECT burst_statue_piece_50;
extern NJS_OBJECT burst_statue_piece_51;
extern NJS_OBJECT burst_statue_piece_52;
extern NJS_OBJECT burst_statue_piece_53;
extern NJS_OBJECT burst_statue_piece_54;
extern NJS_OBJECT burst_statue_piece_55;
extern NJS_OBJECT burst_statue_piece_56;
extern NJS_OBJECT burst_statue_piece_57;
extern NJS_OBJECT burst_statue_piece_58;
extern NJS_OBJECT burst_statue_piece_59;
extern NJS_OBJECT burst_statue_piece_60;
extern NJS_OBJECT burst_statue_piece_61;
extern NJS_OBJECT burst_statue_piece_62;
extern NJS_OBJECT burst_statue_piece_63;
extern NJS_OBJECT burst_statue_piece_64;
extern NJS_OBJECT burst_statue_piece_65;
extern NJS_OBJECT burst_statue_piece_66;
extern NJS_OBJECT burst_statue_piece_67;
extern NJS_OBJECT burst_statue_piece_68;
extern NJS_OBJECT burst_statue_piece_69;
extern NJS_OBJECT burst_statue_piece_70;
extern NJS_OBJECT burst_statue_piece_71;
extern NJS_OBJECT burst_statue_piece_72;
extern NJS_OBJECT burst_statue_piece_73;
extern NJS_OBJECT burst_statue_piece_74;
extern NJS_OBJECT burst_statue_piece_75;
extern NJS_OBJECT burst_statue_piece_76;
extern NJS_OBJECT burst_statue_piece_77;
extern NJS_OBJECT burst_statue_piece_78;
extern NJS_OBJECT burst_statue_piece_79;
extern NJS_OBJECT burst_statue_piece_80;
extern NJS_OBJECT burst_statue_piece_81;
extern NJS_OBJECT burst_statue_piece_82;
extern NJS_OBJECT burst_statue_piece_83;
extern NJS_OBJECT burst_statue_piece_84;
extern NJS_OBJECT burst_statue_piece_85;
extern NJS_OBJECT burst_statue_piece_86;
extern NJS_OBJECT burst_statue_piece_87;
extern NJS_OBJECT burst_statue_piece_88;
extern NJS_OBJECT burst_statue_piece_89;
extern NJS_OBJECT burst_statue_piece_90;
extern NJS_OBJECT burst_statue_piece_91;
extern NJS_OBJECT burst_statue_piece_92;
extern NJS_OBJECT burst_statue_piece_93;
extern NJS_OBJECT burst_statue_piece_94;
extern NJS_OBJECT burst_statue_piece_95;
extern NJS_OBJECT burst_statue_piece_96;
extern NJS_OBJECT burst_statue_piece_97;
extern NJS_OBJECT burst_statue_piece_98;
extern NJS_OBJECT burst_statue_piece_99;
extern NJS_OBJECT burst_statue_piece_100;
extern NJS_OBJECT burst_statue_piece_101;
extern NJS_OBJECT burst_statue_piece_102;
extern NJS_OBJECT burst_statue_piece_103;
extern NJS_OBJECT burst_statue_piece_104;
extern NJS_OBJECT burst_statue_piece_105;
extern NJS_OBJECT burst_statue_piece_106;
extern NJS_OBJECT burst_statue_piece_107;
extern NJS_OBJECT burst_statue_piece_108;
extern NJS_OBJECT burst_statue_piece_109;
extern NJS_OBJECT burst_statue_piece_110;
extern NJS_OBJECT burst_statue_piece_111;
extern NJS_OBJECT burst_statue_piece_112;
extern NJS_OBJECT burst_statue_piece_113;
extern NJS_OBJECT burst_statue_piece_114;
extern NJS_OBJECT burst_statue_piece_115;
extern NJS_OBJECT burst_statue_piece_116;
extern NJS_OBJECT burst_statue_piece_117;
extern NJS_OBJECT burst_statue_piece_118;
extern NJS_OBJECT burst_statue_piece_119;
extern NJS_OBJECT burst_statue_piece_120;
extern NJS_OBJECT burst_statue_piece_121;
extern NJS_OBJECT burst_statue_piece_122;
extern NJS_OBJECT burst_statue_piece_123;
extern NJS_OBJECT burst_statue_piece_124;
extern NJS_OBJECT burst_statue_piece_125;
extern NJS_OBJECT burst_statue_piece_126;
extern NJS_OBJECT burst_statue_piece_127;
extern NJS_OBJECT burst_statue_piece_128;
extern NJS_OBJECT burst_statue_piece_129;
extern NJS_OBJECT burst_statue_piece_130;
extern NJS_OBJECT burst_statue_piece_131;
extern NJS_OBJECT burst_statue_piece_132;
extern NJS_OBJECT burst_statue_piece_133;
extern NJS_OBJECT burst_statue_piece_134;
extern NJS_OBJECT burst_statue_piece_135;
extern NJS_OBJECT burst_statue_piece_136;
extern NJS_OBJECT burst_statue_piece_137;
extern NJS_OBJECT burst_statue_piece_138;
extern NJS_OBJECT burst_statue_piece_139;
extern NJS_OBJECT burst_statue_piece_140;
extern NJS_OBJECT burst_statue_piece_141;
extern NJS_OBJECT burst_statue_piece_142;
extern NJS_OBJECT burst_statue_piece_143;
extern NJS_OBJECT burst_statue_piece_144;
extern NJS_OBJECT burst_statue_piece_145;
extern NJS_OBJECT burst_statue_piece_146;
extern NJS_OBJECT burst_statue_piece_147;
extern NJS_OBJECT burst_statue_piece_148;
extern NJS_OBJECT burst_statue_piece_149;
extern NJS_OBJECT burst_statue_piece_150;
extern NJS_OBJECT burst_statue_piece_151;
extern NJS_OBJECT burst_statue_piece_152;
extern NJS_OBJECT burst_statue_piece_153;
extern NJS_OBJECT burst_statue_piece_154;
extern NJS_OBJECT burst_statue_piece_155;
extern NJS_OBJECT burst_statue_piece_156;
extern NJS_OBJECT burst_statue_piece_157;
extern NJS_OBJECT burst_statue_piece_158;
extern NJS_OBJECT burst_statue_piece_159;
extern NJS_OBJECT burst_statue_piece_160;
extern NJS_OBJECT burst_statue_piece_161;
extern NJS_OBJECT burst_statue_piece_162;
extern NJS_OBJECT burst_statue_piece_163;
extern NJS_OBJECT burst_statue_piece_164;
extern NJS_OBJECT burst_statue_piece_165;
extern NJS_OBJECT burst_statue_piece_166;
extern NJS_OBJECT burst_statue_piece_167;

/* the statue starts only within about 600 of the camera or player 0, and
 * destroys itself otherwise. a shard destroys itself beyond about 600, and
 * the sphere beyond about 250. placement_try_leave_by takes each range
 * squared */
#define RANGE ((600.0F * 600.0F) + 10.0F)
#define COLLISION_RANGE ((250.0F * 250.0F) + 10.0F)
/* the statue runs and draws only while player 0 or 1 is within this of it */
/* @bug logic: NEAR is 600 squared, but player_in_range takes a
 * distance (0x0c915f70, 0x0c91685e). the statue then runs and draws while a
 * player is within 360000 of it */
#define NEAR (600.0F * 600.0F)

#define PIECES 168

#define SE_BURST 240

/* the statue's modes. STATUE_BURST lasts from the step the burst ends to the
 * next, which moves the mode on to 3 */
#define STATUE_STANDING 0
#define STATUE_BURSTING 1
#define STATUE_BURST 2

/* the statue's shapes: the one an attack can hurt, and the one that is on
 * while it bursts */
#define STATUE_SHAPES 7
#define HURT_SHAPE 0
#define BURST_SHAPE 1

/* the copy's radius, as casino_world_add and casino_world_set_model take it */
#define COPY_RADIUS 200.0F
/* the collision world tests the copy only while player 0 or 1 is within
 * TEST_RADIUS of a point. the point is TEST_DEPTH below the statue */
#define TEST_DEPTH 119.0F
#define TEST_RADIUS 110.0F

#define DUST_RISE 0.2F /* the speed the dust rises at */
/* how far toward the camera the dust moves, where the camera is farther */
#define DUST_INSET 20.0F

/* the attrflags the glow's first material draws with, in place of its own */
#define GLOW_FLAGS                                                             \
    (NJD_SA_SRC | NJD_DA_INV_SRC | NJD_FLAG_DOUBLE_SIDE |                      \
     NJD_FLAG_USE_TEXTURE | NJD_FLAG_USE_ALPHA | NJD_FLAG_IGNORE_SPECULAR |    \
     NJD_FLAG_CLAMP_U | NJD_FLAG_CLAMP_V | NJD_FILTER_BILINEAR | NJD_D_100)

/* angle units per degree, written 182.04444: 65536 / 360 cut to five
 * decimal places. as a Float, it is one step below ANGLE_PER_DEGREE */
#define ANGLE_PER_DEGREE_CUT 182.04444F

/* a random Float from -range up to range, never range itself */
#define SCATTER(range) ((((rand() * RAND_SCALE) - 0.5F) * (range)) * 2.0F)
/* a random turn of up to degrees either way, in angle units */
#define RANDOM_TURN(degrees)                                                   \
    ((((rand() * RAND_SCALE) - 0.5F) * (degrees)) *                            \
     (ANGLE_PER_DEGREE_CUT * 2.0F))

/* scale spin's low half, signed, by factor, and keep its high half */
/* @hack spelling: the scale through a Float local, which SHC works in FR4,
 * with the half's address in R4 (0x0c9161f0 to 0x0c91621a). scaled in place,
 * the half takes FR2 and R2, pos and speed trade R4 and R5 in the walk at
 * 0x0c916120, and 77 of 905 words differ */
#define SPIN_SCALE(spin, factor)                                               \
    do {                                                                       \
        Float scaled = (spin).half[0];                                         \
                                                                               \
        scaled *= (factor);                                                    \
        (spin).half[0] = scaled;                                               \
    } while (0)

/* a piece the statue bursts into, in the statue's coordinates. spin_y and
 * spin_z turn ang_y and ang_z each step */
struct statue_piece {
    union point_floats pos;
    union point_floats speed;
    Angle ang_z;
    Angle ang_y;
    union angle_halves spin_y;
    union angle_halves spin_z;
};

/* the pieces, which burst_statue_init allocates and the destructor frees */
struct statue_burst {
    Sint32 count;
    struct statue_piece *pieces;
    /* the height the pieces land at, from the statue's: the floor below
     * player 0 as the statue bursts */
    Float floor;
};

struct burst_statue_vars {
    NJS_OBJECT *world; /* the copy it lends the collision world */
    Uint8 unk_04[4];
    struct statue_burst *burst;
};

#define VARS(work) ((struct burst_statue_vars *)&(work)->vars)
/* the pieces' hard bounces, counted round 0 to BOUNCES_MAX */
#define BOUNCES(work) ((work)->tick)
#define BOUNCES_MAX 20

struct statue_shard_vars {
    Uint8 unk_00[4];
    NJS_OBJECT *object;
    /* the radius draw_in_view tests: the model's radius plus its center's
     * distance from its origin, both scaled, plus 1 */
    Float range;
};

#define SHARD(work) ((struct statue_shard_vars *)&(work)->vars)

static NJS_OBJECT *burst_statue_pieces[PIECES] = {
    &burst_statue_piece_0,   &burst_statue_piece_1,   &burst_statue_piece_2,
    &burst_statue_piece_3,   &burst_statue_piece_4,   &burst_statue_piece_5,
    &burst_statue_piece_6,   &burst_statue_piece_7,   &burst_statue_piece_8,
    &burst_statue_piece_9,   &burst_statue_piece_10,  &burst_statue_piece_11,
    &burst_statue_piece_12,  &burst_statue_piece_13,  &burst_statue_piece_14,
    &burst_statue_piece_15,  &burst_statue_piece_16,  &burst_statue_piece_17,
    &burst_statue_piece_18,  &burst_statue_piece_19,  &burst_statue_piece_20,
    &burst_statue_piece_21,  &burst_statue_piece_22,  &burst_statue_piece_23,
    &burst_statue_piece_24,  &burst_statue_piece_25,  &burst_statue_piece_26,
    &burst_statue_piece_27,  &burst_statue_piece_28,  &burst_statue_piece_29,
    &burst_statue_piece_30,  &burst_statue_piece_31,  &burst_statue_piece_32,
    &burst_statue_piece_33,  &burst_statue_piece_34,  &burst_statue_piece_35,
    &burst_statue_piece_36,  &burst_statue_piece_37,  &burst_statue_piece_38,
    &burst_statue_piece_39,  &burst_statue_piece_40,  &burst_statue_piece_41,
    &burst_statue_piece_42,  &burst_statue_piece_43,  &burst_statue_piece_44,
    &burst_statue_piece_45,  &burst_statue_piece_46,  &burst_statue_piece_47,
    &burst_statue_piece_48,  &burst_statue_piece_49,  &burst_statue_piece_50,
    &burst_statue_piece_51,  &burst_statue_piece_52,  &burst_statue_piece_53,
    &burst_statue_piece_54,  &burst_statue_piece_55,  &burst_statue_piece_56,
    &burst_statue_piece_57,  &burst_statue_piece_58,  &burst_statue_piece_59,
    &burst_statue_piece_60,  &burst_statue_piece_61,  &burst_statue_piece_62,
    &burst_statue_piece_63,  &burst_statue_piece_64,  &burst_statue_piece_65,
    &burst_statue_piece_66,  &burst_statue_piece_67,  &burst_statue_piece_68,
    &burst_statue_piece_69,  &burst_statue_piece_70,  &burst_statue_piece_71,
    &burst_statue_piece_72,  &burst_statue_piece_73,  &burst_statue_piece_74,
    &burst_statue_piece_75,  &burst_statue_piece_76,  &burst_statue_piece_77,
    &burst_statue_piece_78,  &burst_statue_piece_79,  &burst_statue_piece_80,
    &burst_statue_piece_81,  &burst_statue_piece_82,  &burst_statue_piece_83,
    &burst_statue_piece_84,  &burst_statue_piece_85,  &burst_statue_piece_86,
    &burst_statue_piece_87,  &burst_statue_piece_88,  &burst_statue_piece_89,
    &burst_statue_piece_90,  &burst_statue_piece_91,  &burst_statue_piece_92,
    &burst_statue_piece_93,  &burst_statue_piece_94,  &burst_statue_piece_95,
    &burst_statue_piece_96,  &burst_statue_piece_97,  &burst_statue_piece_98,
    &burst_statue_piece_99,  &burst_statue_piece_100, &burst_statue_piece_101,
    &burst_statue_piece_102, &burst_statue_piece_103, &burst_statue_piece_104,
    &burst_statue_piece_105, &burst_statue_piece_106, &burst_statue_piece_107,
    &burst_statue_piece_108, &burst_statue_piece_109, &burst_statue_piece_110,
    &burst_statue_piece_111, &burst_statue_piece_112, &burst_statue_piece_113,
    &burst_statue_piece_114, &burst_statue_piece_115, &burst_statue_piece_116,
    &burst_statue_piece_117, &burst_statue_piece_118, &burst_statue_piece_119,
    &burst_statue_piece_120, &burst_statue_piece_121, &burst_statue_piece_122,
    &burst_statue_piece_123, &burst_statue_piece_124, &burst_statue_piece_125,
    &burst_statue_piece_126, &burst_statue_piece_127, &burst_statue_piece_128,
    &burst_statue_piece_129, &burst_statue_piece_130, &burst_statue_piece_131,
    &burst_statue_piece_132, &burst_statue_piece_133, &burst_statue_piece_134,
    &burst_statue_piece_135, &burst_statue_piece_136, &burst_statue_piece_137,
    &burst_statue_piece_138, &burst_statue_piece_139, &burst_statue_piece_140,
    &burst_statue_piece_141, &burst_statue_piece_142, &burst_statue_piece_143,
    &burst_statue_piece_144, &burst_statue_piece_145, &burst_statue_piece_146,
    &burst_statue_piece_147, &burst_statue_piece_148, &burst_statue_piece_149,
    &burst_statue_piece_150, &burst_statue_piece_151, &burst_statue_piece_152,
    &burst_statue_piece_153, &burst_statue_piece_154, &burst_statue_piece_155,
    &burst_statue_piece_156, &burst_statue_piece_157, &burst_statue_piece_158,
    &burst_statue_piece_159, &burst_statue_piece_160, &burst_statue_piece_161,
    &burst_statue_piece_162, &burst_statue_piece_163, &burst_statue_piece_164,
    &burst_statue_piece_165, &burst_statue_piece_166, &burst_statue_piece_167,
};
/* added to a piece's y speed each step */
static Float burst_statue_gravity = -0.13F;
/* the share of its speed and spin a piece keeps each step */
static Float burst_statue_drag = 0.98F;
/* the most a piece's start speed is off at random, either way, on each axis */
static Float burst_statue_spread = 0.6F;
/* a piece starts away from the statue's origin at start_speed for each
 * start_radius of its distance from it. the spread comes on top */
static Float burst_statue_start_radius = 155.0F;
static Float burst_statue_start_speed = 1.3F;
/* the burst ends on the step that finds the timer past this */
static int burst_statue_steps = 150;
/* the steps over which the pieces start moving, in order */
static int burst_statue_release_steps = 30;
/* the most degrees a piece starts turning by each step, either way */
static Float burst_statue_spin = 11.0F;
/* the most a bounce changes a piece's x and z speed at random, either way */
static Float burst_statue_bounce_spread = 1.3F;
/* the share of its spin a landing piece keeps, and of its speed if it
 * bounces */
static Float burst_statue_bounce_keep = 0.15F;
/* a piece landing faster bounces, and one no faster stops */
static Float burst_statue_bounce_speed = 2.0F;
/* the dust's scale as a piece bounces, for each unit of its speed */
static Float burst_statue_bounce_dust = 2.0F;
/* the dust's scale each step at the piece next to start */
static Float burst_statue_release_dust = 40.0F;
/* the degrees the glow's pulse turns each step */
static int burst_statue_glow_speed = 5;
/* while the statue stands, every shape but BURST_SHAPE is on, and an attack
 * can hurt HURT_SHAPE alone. while it bursts, BURST_SHAPE alone is on, and it
 * grows as the pieces start */
/* clang-format off */
static struct collision_shape burst_statue_shapes[STATUE_SHAPES] = {
    {SHAPE_CAPSULE | SHAPE_HURTABLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {23.983458F, -144.27411F, -17.581982F}, 0.1F, 10.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -154.0F, 0.0F}, 61.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-9.2F, -100.0F, 7.13F}, 14.0F, 56.0F, 14.0F, {0}, 0, NULL},
    {SHAPE_CAPSULE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {51.71F, -11.0F, -4.64F}, 24.0F, 50.0F, 0.0F, {0x4ef0, 0xf4c0, 0xcf00},
     0, NULL},
    {SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {2.87F, 42.0F, 14.62F}, 60.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-47.88F, 37.0F, 18.74F}, 70.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-14.0F, -30.0F, 13.22F}, 46.0F, 0.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */
/* the work of the first statue started outside the set editor while none
 * leads. the shards and O SONZCOL follow its mode */
static struct task_work *burst_statue_leader = NULL;
/* 52 bytes that no code reads; their type is unknown */
static Uint32 dat_0cb152e0[13] = {
    0, 0x0000ff00, 0, 0x40900000, 0, 0x40900000, 0, 0, 0, 0, 0, 0, 0,
};
static NJS_OBJECT *statue_shard_objects[6] = {
    &statue_shard_object_1, &statue_shard_object_2, &statue_shard_object_3,
    &statue_shard_object_4, &statue_shard_object_5, &statue_shard_object_6,
};
static struct collision_shape statue_collision_shape = {
    SHAPE_SPHERE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    1.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static void burst_statue_init(struct task *task);
static void burst_statue_destroy(struct task *task);
static void burst_statue_run(struct task *task);
static void burst_statue_draw(struct task *task);

/* the leader's mode, or 0 with no leader */
static int burst_statue_mode(void)
{
    if (burst_statue_leader) {
        /* @hack spelling: the leader through a local, which takes R1 here
         * (0x0c915ce2) and R13 where statue_collision_run expands this
         * (0x0c916c1c). as burst_statue_leader->mode, it takes R4 and R5, and
         * 3 of 9 words differ, with 3 of 50 in statue_collision_run */
        struct task_work *work = burst_statue_leader;

        return work->mode;
    }
    return 0;
}

/* raise dust of scale at point, a point in the statue's coordinates */
static void burst_statue_dust(NJS_POINT3 *point, Float scale,
                              struct task_work *work)
{
    NJS_POINT3 pos;
    union point_floats speed;
    /* @hack spelling: speed stored through a cursor, which ADD #4 steps
     * (0x0c915d0a, 0x0c915d10). member by member, SHC stores each float from
     * R15, and 59 of 64 words differ */
    Float *at = speed.elements;

    *at++ = 0.0F;
    *at++ = DUST_RISE;
    *at = 0.0F;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njCalcPoint(NULL, point, &pos);
    njCalcVector(NULL, &speed.vector, &speed.vector);
    njPopMatrix(1);
    /* @hack type: the union's own address at this use, from which SHC works
     * speed's address out of R15 at each use, as at 0x0c915d42 and
     * 0x0c915d5e. as &speed.vector here too, SHC keeps it in R14, and 54 of
     * 64 words differ */
    casino_dust_toward_camera(&pos, (NJS_VECTOR *)&speed, scale, DUST_INSET);
}

void sonic_statue_knuckles_step(struct task *task)
{
    if (!placement_try_leave_by(task, RANGE)) {
        if (player_get_character(0) == CHARACTER_SONIC) {
            sonic_statue_step(task);
        } else {
            burst_statue_init(task);
        }
    }
}

static void burst_statue_init(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: the count through a local, which SHC keeps in the frame
     * across both calls (0x0c915eb2, 0x0c915ec8). as PIECES at each use, SHC
     * loads 168 again at the store, and 106 of 109 words differ */
    Sint32 count;

    if (!placement_editing(task)) {
        if (!burst_statue_leader) {
            /* @hack spelling: task->work read again, MOV.L @(32,R13),R3 at
             * 0x0c915e20. as work, SHC stores R14, and 94 of 109 words
             * differ */
            burst_statue_leader = task->work;
        }
        collision_init(task, burst_statue_shapes, COLLISION_OBJECT, 1,
                       STATUE_SHAPES);
        casino_world_add(task, &burst_statue_world_object, COPY_RADIUS,
                         WORLD_MOVES);
        work->collision->shapes[HURT_SHAPE].flags &= ~SHAPE_OFF;
        work->collision->shapes[BURST_SHAPE].flags |= SHAPE_OFF;
        work->collision->shapes[2].flags &= ~SHAPE_OFF;
        work->collision->shapes[3].flags &= ~SHAPE_OFF;
        work->collision->shapes[4].flags &= ~SHAPE_OFF;
        work->collision->shapes[5].flags &= ~SHAPE_OFF;
        work->collision->shapes[6].flags &= ~SHAPE_OFF;
    }
    task->draw = burst_statue_draw;
    task->step = burst_statue_run;
    task->destructor = burst_statue_destroy;
    work->timer = 0;
    work->mode = STATUE_STANDING;
    count = PIECES;
    VARS(work)->burst = heap_alloc(sizeof(struct statue_burst));
    VARS(work)->burst->pieces = heap_alloc(count * sizeof(struct statue_piece));
    VARS(work)->burst->count = count;
}

static void burst_statue_destroy(struct task *task)
{
    struct task_work *work = task->work;

    casino_world_remove(task);
    heap_free(VARS(work)->burst->pieces);
    heap_free(VARS(work)->burst);
    if (burst_statue_leader == work) {
        burst_statue_leader = NULL;
    }
}

static void burst_statue_run(struct task *task)
{
    struct task_work *work = task->work;
    struct statue_piece *piece;
    NJS_OBJECT **object;
    struct collision_work *collision;
    NJS_MODEL *model;
    Sint32 count;
    Float fraction;
    Float floor;
    Float speed;
    Float dx;
    Float dz;
    Angle spin;
    /* @hack spelling: cursors that ADD #4 steps through a vector's floats, in
     * each walk below. the walk of pos loads speed by post-increment
     * (0x0c916120 to 0x0c916142). member by member, SHC addresses each float
     * by displacement, and 494 of 905 words differ */
    Float *at;
    Float *by;

    if (!player_in_range(&work->pos, NEAR)) {
        return;
    }
    burst_statue_draw(task);
    if (placement_editing(task)) {
        return;
    }
    switch (work->mode) {
    case STATUE_BURSTING:
        /* @bug memory: the work keeps the copy's pointer after the
         * burst's first step gives the copy back to the pool. each later step
         * of the burst gives it back again (0x0c916002), as do the step after
         * and the destructor. once the pool lends that object to another
         * task, each such call takes it from under that task */
        casino_world_remove(task);
        work->collision->shapes[HURT_SHAPE].flags |= SHAPE_OFF;
        work->collision->shapes[BURST_SHAPE].flags &= ~SHAPE_OFF;
        work->collision->shapes[2].flags |= SHAPE_OFF;
        work->collision->shapes[3].flags |= SHAPE_OFF;
        work->collision->shapes[4].flags |= SHAPE_OFF;
        work->collision->shapes[5].flags |= SHAPE_OFF;
        work->collision->shapes[6].flags |= SHAPE_OFF;
        if (TASK_TIMER(work) >= burst_statue_release_steps) {
            count = VARS(work)->burst->count;
            work->collision->shapes[BURST_SHAPE].radius =
                burst_statue_shapes[BURST_SHAPE].radius;
        } else {
            fraction = (Float)TASK_TIMER(work) / burst_statue_release_steps;
            count = VARS(work)->burst->count * fraction;
            burst_statue_dust(&VARS(work)->burst->pieces[count].pos.point,
                              burst_statue_release_dust, work);
            work->collision->shapes[BURST_SHAPE].radius =
                burst_statue_shapes[BURST_SHAPE].radius * fraction;
        }
        floor = VARS(work)->burst->floor;
        piece = VARS(work)->burst->pieces;
        while (count--) {
            /* @hack redundant: stores and a call that never run, whose
             * references SHC still counts. with them, it keeps njScalor's
             * address in R8, -0.5 in FR14, 1/32768 in FR15 and 0 in R11
             * (0x0c915f90 to 0x0c915fa6). the call also has the walk below
             * step pos in R4 and speed in R5 (0x0c916120). without them, 497
             * of 905 words differ */
            if (0) {
                BOUNCES(work) = 0;
                burst_statue_gravity = RAND_SCALE;
                njScalor(&piece->pos.vector);
            }
            at = piece->pos.elements;
            by = piece->speed.elements;
            *at++ += *by++;
            *at++ += *by++;
            *at += *by;
            if (piece->pos.point.y < floor) {
                if ((speed = njScalor(&piece->speed.vector)) >
                    burst_statue_bounce_speed) {
                    /* SE_SLAM sounds at the first hard bounce, and at every
                     * 21st after it */
                    if (!BOUNCES(work)++) {
                        sound_play(SE_SLAM, 0, 0, 0);
                    }
                    if (BOUNCES(work) > BOUNCES_MAX) {
                        BOUNCES(work) = 0;
                    }
                    piece->pos.point.y = floor;
                    at = piece->speed.elements;
                    *at++ *= burst_statue_bounce_keep;
                    *at++ *= burst_statue_bounce_keep;
                    *at *= burst_statue_bounce_keep;
                    piece->speed.vector.x +=
                        SCATTER(burst_statue_bounce_spread);
                    piece->speed.vector.y = -piece->speed.vector.y;
                    piece->speed.vector.z +=
                        SCATTER(burst_statue_bounce_spread);
                    SPIN_SCALE(piece->spin_y, burst_statue_bounce_keep);
                    SPIN_SCALE(piece->spin_z, burst_statue_bounce_keep);
                    burst_statue_dust(&piece->pos.point,
                                      burst_statue_bounce_dust * speed, work);
                    vibration_start(0, 3, 20, 0);
                } else {
                    piece->pos.point.y = floor;
                    at = piece->speed.elements;
                    *at++ = 0.0F;
                    *at++ = 0.0F;
                    *at = 0.0F;
                    SPIN_SCALE(piece->spin_y, burst_statue_bounce_keep);
                    SPIN_SCALE(piece->spin_z, burst_statue_bounce_keep);
                }
            }
            piece->speed.vector.y += burst_statue_gravity;
            at = piece->speed.elements;
            *at++ *= burst_statue_drag;
            *at++ *= burst_statue_drag;
            *at *= burst_statue_drag;
            piece->ang_y += piece->spin_y.whole;
            piece->ang_z += piece->spin_z.whole;
            SPIN_SCALE(piece->spin_y, burst_statue_drag);
            SPIN_SCALE(piece->spin_z, burst_statue_drag);
            piece++;
        }
        if (TASK_TIMER(work) > burst_statue_steps) {
            work->collision->shapes[BURST_SHAPE].flags |= SHAPE_OFF;
            camera_restore();
            camera_pose_taken = FALSE;
            work->mode = STATUE_BURST;
            work->timer = 0;
            if (work->scl.x != 0.0F || work->scl.y != 0.0F ||
                work->scl.z != 0.0F) {
                player_set_position(0, work->scl.x, work->scl.y, work->scl.z);
            }
        }
        break;
    case STATUE_STANDING:
        /* @hack spelling: the model through a local. passed straight from the
         * object, the 0 the function keeps in a register takes R10, where the
         * image has it in R11 (MOV #0,R11 at 0x0c915fa6), and 39 of 905 words
         * differ */
        model = burst_statue_world_object.model;
        casino_world_set_model(task, model, COPY_RADIUS);
        /* @hack spelling: collision set inside the test, whose statement holds
         * both calls, which keeps it in the frame (0x0c9163d0, 0x0c9163f6).
         * set before the test, it takes R13, and 68 of 905 words differ */
        if (((collision = work->collision)->flags & COLLISION_HIT) &&
            (work->flags & COLLISION_DAMAGED) &&
            (player_work[0]->flags & PLAYER_ON_LAND) &&
            (collision->hit->owner->work->flags & PLAYER_ON_LAND) &&
            !camera_at_event_level() && !camera_at_comp_level()) {
            VARS(work)->burst->floor =
                player_get(0)->shadow.below - work->pos.y;
            count = VARS(work)->burst->count;
            piece = VARS(work)->burst->pieces;
            object = burst_statue_pieces;
            while (count--) {
                union point_floats start;

                /* @hack type: the object's Float pos[3] read whole as an
                 * NJS_POINT3, which SHC copies as three longs (MOV.L at
                 * 0x0c9164a4 to 0x0c9164ae). member by member, it copies them
                 * through FR3, and 263 of 905 words differ */
                piece->pos.point = *(NJS_POINT3 *)(*object)->pos;
                /* the image calls njScalor here and drops the length
                 * (0x0c9164b0) */
                njScalor(&piece->pos.vector);
                speed = burst_statue_start_speed / burst_statue_start_radius;
                start = piece->pos;
                at = start.elements;
                *at++ *= speed;
                *at++ *= speed;
                *at *= speed;
                start.vector.x += SCATTER(burst_statue_spread);
                start.vector.y += SCATTER(burst_statue_spread);
                start.vector.z += SCATTER(burst_statue_spread);
                /* @hack type: the whole union's address, from which SHC works
                 * start's address out of R15 at each use, as at 0x0c9164b6
                 * and 0x0c9164be. as start.vector, SHC keeps it in R10, where
                 * the image keeps burst_statue_spread's (0x0c916438), and 245
                 * of 905 words differ */
                piece->speed.vector = *(NJS_VECTOR *)&start;
                /* @hack spelling: each spin through a local. assigned
                 * straight, the 0 the function keeps in a register takes R10,
                 * and burst_statue_bounce_keep's address R11. the image has
                 * them in R11 and R10 (0x0c915fa6, 0x0c9160e8), and 42 of 905
                 * words differ */
                spin = RANDOM_TURN(burst_statue_spin);
                piece->spin_y.whole = spin;
                spin = RANDOM_TURN(burst_statue_spin);
                piece->spin_z.whole = spin;
                piece->ang_z = piece->ang_y = 0;
                object++;
                piece++;
            }
            {
                struct angles ang;
                union point_floats eye;
                union point_floats target;
                union point_floats push;

                work->mode = STATUE_BURSTING;
                work->timer = 0;
                BOUNCES(work) = 0;
                /* the push player 0 takes, and the camera's eye and target,
                 * in the statue's coordinates */
                at = push.elements;
                *at++ = 0.0F;
                *at++ = 2.0F;
                *at = 1.0F;
                at = eye.elements;
                *at++ = 221.0F;
                *at++ = -173.0F;
                *at = 97.0F;
                at = target.elements;
                *at++ = -25.0F;
                *at++ = -60.0F;
                *at = -14.5F;
                njPushMatrix(&_unit_matrix_);
                njTranslateV(NULL, &work->pos);
                TURN(njRotateY, work->ang.y);
                /* @hack type: each union's own address once, from which SHC
                 * works each address out of R15 at each use, as at
                 * 0x0c91660c. with the members' alone, it keeps eye's in R13,
                 * target's in R12 and push's in R10, and 259 of 905 words
                 * differ */
                njCalcPoint(NULL, (NJS_POINT3 *)&eye, &eye.point);
                njCalcPoint(NULL, (NJS_POINT3 *)&target, &target.point);
                njCalcVector(NULL, (NJS_VECTOR *)&push, &push.vector);
                njPopMatrix(1);
                dx = eye.vector.x - target.vector.x;
                dz = eye.vector.z - target.vector.z;
                /* @hack redundant: stores to scl.x that never run, which SHC
                 * drops but still counts. with them, dx takes FR15 and dz FR14
                 * (0x0c916644, 0x0c916652), and pos and speed take R4 and R5
                 * in the walk at 0x0c916120. with dx's stores alone,
                 * njScalor's address loads after rand's (0x0c915f90). without
                 * them, 24 of 905 words differ */
                if (0) {
                    work->scl.x = 2.0F;
                    work->scl.x = dx;
                    work->scl.x = dx;
                }
                ang.y = (Angle)(atan2f(dx, dz) * ANGLE_PER_RADIAN);
                ang.x = (Angle)(atan2f(target.vector.y - eye.vector.y,
                                       njSqrt((dx * dx) + (dz * dz))) *
                                ANGLE_PER_RADIAN);
                ang.z = 0;
                vibration_start(0, 5, 55, 2);
                sound_play(SE_BURST, 0, 0, 0);
                camera_override(camera_step_pose, 0, 0);
                camera_pose_taken = FALSE;
                camera_pose_pos = eye.point;
                camera_pose_ang = ang;
                player_set_speed(0, push.vector.x, push.vector.y,
                                 push.vector.z);
            }
        }
        break;
    case STATUE_BURST:
        casino_world_remove(task);
        /* @hack spelling: task->work read again, MOV.L @(32,R13),R2 at
         * 0x0c916772. through work, SHC increments the mode through R14, an
         * instruction shorter, and 74 of 905 words differ */
        task->work->mode++;
        break;
    }
    collision_register(work);
    if (work->flags & WORLD_TESTED) {
        NJS_POINT3 below;

        below = work->pos;
        /* @hack spelling: -TEST_DEPTH added, FADD at 0x0c9167a6. as
         * TEST_DEPTH taken away, it is FSUB, and 1 of 905 words differ */
        below.y += -TEST_DEPTH;
        if (!player_in_range(&below, TEST_RADIUS)) {
            work->flags &= ~WORLD_TESTED;
        }
    }
    /* @hack spelling: the test for the wrap to 0, written <= 0 for the
     * image's CMP/PL (0x0c9167c6). the timer is unsigned, so the test holds
     * at 0 alone, and the decrement holds the timer at 65535. as == 0, the
     * test is TST, and 2 of 905 words differ */
    if (++TASK_TIMER(work) <= 0) {
        --TASK_TIMER(work);
    }
}

static void burst_statue_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct statue_piece *piece;
    NJS_OBJECT **object;
    int count;
    Uint32 attrflags;

    if (player_in_range(&work->pos, NEAR)) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        switch (work->mode) {
        case STATUE_BURSTING:
            piece = VARS(work)->burst->pieces;
            object = burst_statue_pieces;
            count = VARS(work)->burst->count;
            while (count--) {
                njPushMatrix(NULL);
                njTranslateV(NULL, &piece->pos.vector);
                TURN(njRotateY, piece->ang_y);
                TURN(njRotateZ, piece->ang_z);
                draw_model((*object)->model);
                njPopMatrix(1);
                object++;
                piece++;
            }
            break;
        case STATUE_STANDING:
            draw_tree(&burst_statue_object);
            draw_set_material(
                njSin(burst_statue_glow_speed * (Float)TASK_TIMER(work) *
                      ANGLE_PER_DEGREE_CUT),
                0.0F, 0.0F, 0.0F);
            attrflags = burst_statue_glow_object.model->mats[0].attrflags;
            burst_statue_glow_object.model->mats[0].attrflags = GLOW_FLAGS;
            draw_tree(&burst_statue_glow_object);
            burst_statue_glow_object.model->mats[0].attrflags = attrflags;
            draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
            break;
        case STATUE_BURST:
            break;
        }
        njPopMatrix(1);
    }
}

static void statue_shard_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float scale;

    if (!placement_try_leave_by(task, RANGE)) {
        if (placement_editing(task) || burst_statue_mode() == STATUE_BURST) {
            if (draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                             SHARD(work)->range)) {
                scale = work->scl.x + 1.0F;
                casino_set_texture(CASINO_TEXTURES_ACTS_12);
                njPushMatrix(NULL);
                njTranslateV(NULL, &work->pos);
                TURN(njRotateZ, work->ang.z);
                TURN(njRotateX, work->ang.x);
                TURN(njRotateY, work->ang.y);
                njScale(NULL, scale, scale, scale);
                draw_tree(SHARD(work)->object);
                njPopMatrix(1);
            }
        }
    }
}

static void statue_shard_run(struct task *task)
{
    if (!placement_try_leave_by(task, RANGE)) {
        statue_shard_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> Scale");
        }
    }
}

static void statue_shard_start(struct task *task, NJS_OBJECT *object)
{
    struct task_work *work = task->work;
    NJS_MODEL *model;
    Float scale;
    Float radius;

    if (!placement_try_leave_by(task, RANGE)) {
        scale = work->scl.x + 1.0F;
        /* @hack spelling: model set inside the product, which keeps it in the
         * frame (0x0c916b00). set on its own, it takes R12, and 32 of 56
         * words differ */
        radius = (model = object->model)->r * scale;
        SHARD(work)->range = radius + (njScalor(&model->center) * scale) + 1.0F;
        SHARD(work)->object = object;
        task->draw = statue_shard_draw;
        task->step = statue_shard_run;
        /* @hack redundant: an empty test of placement_editing's result, TST at
         * 0x0c916b34, before the return. as a bare call, the call becomes a
         * jump, and 19 of 56 words differ */
        if (placement_editing(task)) {
        }
    }
}

void statue_shard_1_step(struct task *task)
{
    statue_shard_start(task, statue_shard_objects[0]);
}

void statue_shard_2_step(struct task *task)
{
    statue_shard_start(task, statue_shard_objects[1]);
}

void statue_shard_3_step(struct task *task)
{
    statue_shard_start(task, statue_shard_objects[2]);
}

void statue_shard_4_step(struct task *task)
{
    statue_shard_start(task, statue_shard_objects[3]);
}

void statue_shard_5_step(struct task *task)
{
    statue_shard_start(task, statue_shard_objects[4]);
}

void statue_shard_6_step(struct task *task)
{
    statue_shard_start(task, statue_shard_objects[5]);
}

static void statue_collision_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, COLLISION_RANGE)) {
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> Scale");
            casino_draw_sphere(&work->pos, work->scl.x);
            return;
        }
        if (burst_statue_mode() == STATUE_BURST) {
            collision_register(work);
        }
    }
}

void statue_collision_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, COLLISION_RANGE)) {
        return;
    }
    task->step = statue_collision_run;
    if (placement_editing(task)) {
        return;
    }
    collision_init(task, &statue_collision_shape, COLLISION_OBJECT, 1, 1);
    work->collision->shapes->radius = work->scl.x;
    work->collision->shapes->size_y = work->scl.y;
    work->collision->shapes->size_z = work->scl.z;
    collision_set_radius(work->collision);
}
