/* the Egg Walker's cannon attack: three missiles lobbed at player 0. they
 * trail smoke and blow up where they land */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_egm2/blast.h"
#include "b_egm2/cannons.h"
#include "b_egm2/egg_walker.h"
#include "b_egm2/parts.h"
#include "b_egm2/texlists.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "draw/motion.h"
#include "draw/state.h"
#include "draw/tree.h"
#include "enemy/enemy.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* the boss's submodes in EGG_WALKER_CANNONS */
enum cannons_mode {
    CANNONS_START,
    CANNONS_AIM,
    CANNONS_TURN, /* for CANNONS_TURN_TIME steps */
    CANNONS_FIRE
};

/* work->vars of the boss in EGG_WALKER_CANNONS */
struct cannons_vars {
    /* @hack type: shots unsigned, which converts it to a Float as unsigned:
     * FLOAT, then 2^32 added to a negative (CMP/PZ at 0x0c90671e, FADD at
     * 0x0c90672c); as a Sint32, 67 of 162 words differ */
    Uint32 shots; /* the missiles left to fire */
    Sint32 timer; /* the steps left to turn */
};

#define VARS(work) ((struct cannons_vars *)&(work)->vars)

enum missile_mode { MISSILE_START, MISSILE_FLY };
enum trail_mode { TRAIL_START, TRAIL_PLAY, TRAIL_END };

/* work->vars of a trail puff: its sprite frame */
struct trail_vars {
    Float frame;
};

#define TRAIL_VARS(work) ((struct trail_vars *)&(work)->vars)

/* the boss fires CANNONS_SHOTS missiles. before each, it turns for
 * CANNONS_TURN_TIME steps. each step, the turret and the cannon cover
 * CANNONS_PACE of their remaining turn. a missile flies at the speed
 * cannons_aim picks, less CANNONS_SHORT of it for each missile still to
 * come */
#define CANNONS_SHOTS 3
#define CANNONS_TURN_TIME 30
#define CANNONS_PACE 0.06F
#define CANNONS_SHORT 0.13F

/* cannons_aim aims a missile to reach player 0 in about FLIGHT_TIME steps.
 * each step takes MISSILE_FALL off the missile's upward speed, and rolls it
 * MISSILE_SPIN about its line of flight. a step leaves a puff of smoke with
 * a chance of TRAIL_CHANCE */
#define FLIGHT_TIME 120.0F
#define MISSILE_FALL (9.8F / 360.0F)
#define MISSILE_SPIN (65536.0F / 30.0F)
#define TRAIL_CHANCE 0.2F

#define SE_CANNON 0x1d9
#define SE_MISSILE_BLAST 0x1db

/* a missile's blast: its grow_end, fade_start, end and size. pad 0 shakes
 * at BLAST_POWER and BLAST_FREQ as it blows up */
#define BLAST_GROWN 20
#define BLAST_FADE 40
#define BLAST_END 60
#define BLAST_SIZE 25.0F
#define BLAST_POWER 3
#define BLAST_FREQ 30

/* a puff's sprite frame starts at 1. each step adds a random share of a
 * frame, from 0 up to 1, until it reaches TRAIL_LAST. the sprite has
 * TRAIL_FRAMES frames and draws at TRAIL_SCALE times its size */
#define TRAIL_FRAMES 8
#define TRAIL_LAST 7.0F
#define TRAIL_SCALE 5.0F

#define QUARTER_TURN 0x4000
#define HALF_TURN 0x8000

/* the turns about y, and then x, that carry the missile model's +z along
 * its velocity v */
#define MISSILE_YAW(v)                                                         \
    (QUARTER_TURN - (Angle)(atan2f((v)->z, (v)->x) * ANGLE_PER_RADIAN))
#define MISSILE_PITCH(v)                                                       \
    (-(Angle)(atan2f((v)->y, njSqrt(((v)->x * (v)->x) + ((v)->z * (v)->z))) *  \
              ANGLE_PER_RADIAN))

static void cannons_aim(struct task *task);
static void egm2_missile_shoot(NJS_POINT3 *pos, Float yaw, Float pitch,
                               Float speed);
static void egm2_missile_step(struct task *task);
static void egm2_missile_draw(struct task *task);
static void egm2_missile_destroy(struct task *task);
static void egm2_trail_create(NJS_POINT3 *pos);
static void egm2_trail_step(struct task *task);
static void egm2_trail_draw(struct task *task);
static void muzzle_pos(NJS_POINT3 *pos, Angle body, Angle turret);

/* the muzzle's offset from part 0, before the body's and the turret's turns */
static const NJS_POINT3 muzzle_offset = {-21.9F, 24.67F, 29.4F};

static NJS_MATERIAL egm2_missile_materials[5] = {
#include "b_egm2/cannons/egm2_missile_materials.inc"
};

static Sint16 egm2_missile_meshes0[12] = {
#include "b_egm2/cannons/egm2_missile_meshes0.inc"
};

static Sint16 egm2_missile_meshes1[19] = {
#include "b_egm2/cannons/egm2_missile_meshes1.inc"
};

static Sint16 egm2_missile_meshes2[15] = {
#include "b_egm2/cannons/egm2_missile_meshes2.inc"
};

static Sint16 egm2_missile_meshes3[15] = {
#include "b_egm2/cannons/egm2_missile_meshes3.inc"
};

static Sint16 egm2_missile_meshes4[15] = {
#include "b_egm2/cannons/egm2_missile_meshes4.inc"
};

static NJS_COLOR egm2_missile_uvs0[10] = {
#include "b_egm2/cannons/egm2_missile_uvs0.inc"
};

static NJS_COLOR egm2_missile_uvs1[16] = {
#include "b_egm2/cannons/egm2_missile_uvs1.inc"
};

static NJS_COLOR egm2_missile_uvs2[14] = {
#include "b_egm2/cannons/egm2_missile_uvs2.inc"
};

static NJS_COLOR egm2_missile_uvs3[14] = {
#include "b_egm2/cannons/egm2_missile_uvs3.inc"
};

static NJS_COLOR egm2_missile_uvs4[14] = {
#include "b_egm2/cannons/egm2_missile_uvs4.inc"
};

static NJS_MESHSET egm2_missile_meshsets[5] = {
#include "b_egm2/cannons/egm2_missile_meshsets.inc"
};

static NJS_POINT3 egm2_missile_points[35] = {
#include "b_egm2/cannons/egm2_missile_points.inc"
};

static NJS_VECTOR egm2_missile_normals[35] = {
#include "b_egm2/cannons/egm2_missile_normals.inc"
};

static NJS_MODEL egm2_missile_model = {
#include "b_egm2/cannons/egm2_missile_model.inc"
};

static NJS_OBJECT egm2_missile_object = {
#include "b_egm2/cannons/egm2_missile_object.inc"
};

static struct collision_shape egm2_missile_shape[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURTS_OBJECTS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_PLAYERS |
         SHAPE_GUARD | SHAPE_ATTACK,
     PUSH_PLAYERS,
     {0.0F, 0.0F, 0.0F},
     3.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* the trail sprite's frames: frame n is the whole of egm2_missile_texlist's
 * texture n */
static NJS_TEXANIM egm2_trail_texanims[TRAIL_FRAMES] = {
#include "b_egm2/cannons/egm2_trail_texanims.inc"
};

static NJS_SPRITE egm2_trail_sprite;

void egg_walker_cannons(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_walker *boss = task->extra;
    NJS_POINT3 pos;
    int turn;

    switch (work->submode) {
    case CANNONS_START:
        VARS(work)->shots = CANNONS_SHOTS;
        work->submode = CANNONS_AIM;
    case CANNONS_AIM:
        cannons_aim(task);
        VARS(work)->timer = CANNONS_TURN_TIME;
        work->submode = CANNONS_TURN;
        break;
    case CANNONS_TURN:
        turn = (Uint16)(boss->turret_aim - boss->turret_yaw);
        boss->turret_yaw += SHORT_TURN(turn) * CANNONS_PACE;
        /* @hack spelling: turn assigned inside the sum, under which SHC takes
         * cannon_aim's address before the turret's store and loads both
         * angles through registers (MOV #68,R4 at 0x0c90667a, MOV.W @R4,R4
         * at 0x0c906692); as a statement of its own, 40 of 162 words
         * differ */
        boss->cannon_pitch +=
            (turn = (Uint16)(boss->cannon_aim - boss->cannon_pitch),
             SHORT_TURN(turn)) *
            CANNONS_PACE;
        if (--VARS(work)->timer <= 0) {
            work->submode = CANNONS_FIRE;
        }
        break;
    case CANNONS_FIRE:
        muzzle_pos(&pos, work->ang.y, boss->turret_yaw);
        sound_play(SE_CANNON, 0, 0, 0);
        VARS(work)->shots--;
        boss->cannon_speed *= 1.0F - (VARS(work)->shots * CANNONS_SHORT);
        egm2_missile_shoot(&pos, -boss->turret_yaw - work->ang.y + HALF_TURN,
                           -boss->cannon_pitch, boss->cannon_speed);
        if (VARS(work)->shots == 0) {
            work->submode = MODE_DONE;
        } else {
            work->submode = CANNONS_AIM;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        if (work->mode != EGG_WALKER_WALK) {
            boss_next_mode = EGG_WALKER_WALK;
            boss_last_mode = work->mode;
        }
        work->submode = CANNONS_START;
    }
}

/* aim the turret at player 0 from part 0. then aim it again from where the
 * muzzle would be at that aim. set the cannon's aim and speed for a missile to
 * reach player 0 in about FLIGHT_TIME steps */
static void cannons_aim(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_walker *boss = task->extra;
    NJS_POINT3 to;
    Float yaw;
    Float across;
    Float up;
    Float dist;

    VECTOR_DIFF(&to, &player_work[0]->pos, egg_walker_part_pos(0));
    yaw = QUARTER_TURN - (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN);
    yaw = -yaw - work->ang.y + HALF_TURN;
    muzzle_pos(&to, work->ang.y, (Angle)yaw);
    VECTOR_DIFF(&to, &player_work[0]->pos, &to);
    yaw = QUARTER_TURN - (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN);
    /* @hack spelling: the half turn added in a statement of its own, which
     * keeps across in FR13 and up in FR15 (0x0c906900, 0x0c906906); as one
     * statement, the two trade and 10 of 199 words differ */
    yaw = -yaw - work->ang.y;
    yaw += HALF_TURN;
    boss->turret_aim = yaw;
    dist = njSqrt((to.x * to.x) + (to.z * to.z));
    across = dist * (1.0F / FLIGHT_TIME);
    up = (to.y * (1.0F / FLIGHT_TIME)) + (MISSILE_FALL * FLIGHT_TIME * 0.5F);
    boss->cannon_aim = -(Angle)(atan2f(up, across) * ANGLE_PER_RADIAN);
    boss->cannon_speed = njSqrt((up * up) + (across * across));
}

/* make a missile at *pos, flying at speed. yaw and pitch, in angle units,
 * give its heading and pitch. it makes none when no task or memory is
 * free */
static void egm2_missile_shoot(NJS_POINT3 *pos, Float yaw, Float pitch,
                               Float speed)
{
    struct task *task;
    struct task_work *work;
    NJS_VECTOR *velocity;

    if ((task = task_create(TASK_WORK, 3, egm2_missile_step)) == NULL) {
        return;
    }
    work = task->work;
    velocity = heap_calloc(1, sizeof(NJS_VECTOR));
    if (velocity == NULL) {
        task_kill(task);
        return;
    }
    task->extra = velocity;
    enemy_alloc_record(task, work);
    work->pos.x = pos->x;
    work->pos.y = pos->y;
    work->pos.z = pos->z;
    velocity->y = speed * njSin((Angle)pitch);
    velocity->x = speed * njCos((Angle)pitch) * njSin((Angle)yaw);
    velocity->z = speed * njCos((Angle)pitch) * njCos((Angle)yaw);
    task->step = egm2_missile_step;
    task->draw = egm2_missile_draw;
    task->destructor = egm2_missile_destroy;
    collision_init(task, egm2_missile_shape, COLLISION_OBJECT, 1, 1);
}

/* the missile flies until it falls below the street's height of 0, and
 * blows up there */
static void egm2_missile_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR *velocity = task->extra;
    struct blast_params params;

    switch (work->mode) {
    case MISSILE_START:
        work->mode = MISSILE_FLY;
    case MISSILE_FLY:
        VECTOR_SUM(&work->pos, &work->pos, velocity);
        /* @hack spelling: the fall added as its negative, an FADD at
         * 0x0c906b12 of the pool's -9.8 / 360; subtracted, SHC writes an
         * FSUB and 1 of 120 words differ */
        velocity->y += -MISSILE_FALL;
        if (rand() * RAND_SCALE <= TRAIL_CHANCE) {
            egm2_trail_create(&work->pos);
        }
        if (work->pos.y < 0.0F) {
            params.pos = &work->pos;
            params.flags = BLAST_COPY_POS | BLAST_HURTS;
            params.beam_start = 0;
            params.beam_end = 0;
            params.beam_length = 0.0F;
            params.beam_speed = 0;
            params.grow_start = 0;
            params.grow_end = BLAST_GROWN;
            params.fade_start = BLAST_FADE;
            params.end = BLAST_END;
            params.size = BLAST_SIZE;
            egg_walker_blast_create(&params);
            sound_play(SE_MISSILE_BLAST, 0, 0, 0);
            vibration_start(0, BLAST_POWER, BLAST_FREQ, 1);
            task_kill(task);
        }
        break;
    }
    work->ang.z += MISSILE_SPIN;
    collision_register(work);
    egm2_missile_draw(task);
}

static void egm2_missile_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR *velocity = task->extra;
    Angle ang;

    if (frame_step != 0) {
        return;
    }
    njSetTexture(&egm2_common_texlist);
    draw_state_save();
    draw_attr_set(0, NJD_FLAG_IGNORE_LIGHT);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, MISSILE_YAW(velocity));
    /* @hack redundant: an outer test of the pitch around TURN's own, so SHC
     * works it out three times, tests it twice and passes it once (TST R3,R3
     * at 0x0c906ca6 and 0x0c906ccc); as TURN alone, the object runs 64
     * bytes short and 142 of 156 words differ */
    MISSILE_PITCH(velocity) ? TURN(njRotateX, MISSILE_PITCH(velocity))
                            : (void)0;
    TURN_VIA(njRotateZ, work->ang.z, ang);
    draw_tree(&egm2_missile_object);
    njPopMatrix(1);
    draw_state_restore();
}

static void egm2_missile_destroy(struct task *task)
{
}

void egg_walker_cannons_init(void)
{
    sprite_init(&egm2_trail_sprite, &egm2_missile_texlist, egm2_trail_texanims);
}

static void egm2_trail_create(NJS_POINT3 *pos)
{
    struct task *task;
    struct task_work *work;

    if ((task = task_create(TASK_WORK, 3, egm2_trail_step)) != NULL) {
        work = task->work;
        enemy_alloc_record(task, work);
        work->pos.x = pos->x;
        work->pos.y = pos->y;
        work->pos.z = pos->z;
        task->step = egm2_trail_step;
        task->draw = egm2_trail_draw;
    }
}

static void egm2_trail_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case TRAIL_START:
        TRAIL_VARS(work)->frame = 1.0F;
        work->mode = TRAIL_PLAY;
        break;
    case TRAIL_PLAY:
        TRAIL_VARS(work)->frame += rand() * RAND_SCALE;
        if (TRAIL_VARS(work)->frame >= TRAIL_LAST) {
            TRAIL_VARS(work)->frame = TRAIL_LAST;
            work->mode = TRAIL_END;
        }
        break;
    case TRAIL_END:
        task_kill(task);
        break;
    }
    egm2_trail_draw(task);
}

static void egm2_trail_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;
    Angle ang;

    if (frame_step != 0) {
        return;
    }
    njPushMatrix(NULL);
    color.a = 0.75F;
    color.r = color.g = color.b = 1.0F;
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    njScale(NULL, TRAIL_SCALE, TRAIL_SCALE, TRAIL_SCALE);
    njDrawSprite3D(&egm2_trail_sprite, (int)TRAIL_VARS(work)->frame,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    color.a = 0.0F;
    color.r = 0.0F;
    color.g = 0.0F;
    color.b = 0.0F;
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njPopMatrix(1);
}

/* set *pos to the muzzle's place in the world. body is the boss's heading,
 * and turret the turret's turn. it allocates room for a matrix, and
 * frees it unused */
static void muzzle_pos(NJS_POINT3 *pos, Angle body, Angle turret)
{
    Angle ang;
    void *matrix = heap_alloc(sizeof(NJS_MATRIX));

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, egg_walker_part_pos(0));
    TURN_VIA(njRotateY, HALF_TURN - body, ang);
    TURN_VIA(njRotateY, -turret, ang);
    njCalcPoint(NULL, (NJS_POINT3 *)&muzzle_offset, pos);
    njPopMatrix(1);
    heap_free(matrix);
}
