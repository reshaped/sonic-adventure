/* O Erupt, a mound of rocks, and the burning rocks it throws. the mound
 * bursts as player 0 comes near, and lava then rises from it */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "effect/flame.h"
#include "effect/particle.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg05/erupt.h"
#include "stg05/rubble.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT erupt_blob_object;

/* the attack level of the lava's column and of the blast */
#define ERUPT_ATTACK 2

/* where a puff starts, 10 out from the mound's center, before its vent
 * turns it */
static const NJS_POINT3 erupt_puff_origin = {10.0F, 0.0F, 0.0F};

/* the lava's column, which hurts players, and the mound, which pushes what
 * touches it. the column is off until the lava starts to rise, and then
 * rises with it */
static struct collision_shape erupt_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_OFF | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | ERUPT_ATTACK,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 40.0F, 0.0F},
     15.0F,
     40.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 5.0F, 0.0F},
     30.0F,
     5.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* the blast's shape, which hurts players as the mound bursts.
 * erupt_blast_step grows its radius */
static struct collision_shape erupt_blast_shape[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | ERUPT_ATTACK,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F},
     15.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* a vent in the mound's side, from which puffs leave along x. yaw turns the
 * vent about y from the mound's own turn, and tilt then raises it about z */
struct erupt_vent {
    Angle yaw;
    Angle tilt;
    Uint8 unk_08[12];
};

static struct erupt_vent erupt_vents[3] = {
    {0x5555, 0x1000, {0}},
    {0xaaaa, 0x3000, {0}},
    {0, 0x2000, {0}},
};

static struct dust_params erupt_puff_params = {
    1.0F,
    0.1F,
    0.5F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 0.8F, 0.8F, 1.0F},
};

/* a rock's place, angles and scale within the mound */
struct erupt_pose {
    NJS_POINT3 pos;
    struct angles ang;
    NJS_VECTOR scl;
};

/* the mound's rocks. erupt_draw_mound draws the first ten, and no code reads
 * the eleventh */
static struct erupt_pose erupt_poses[11] = {
#include "stg05/erupt/erupt_poses.inc"
};

/* the rocks' models. erupt_draw_mound draws poses 0 to 4 with the first,
 * and poses 5 to 9 with the second. no code reads the rest */
static NJS_OBJECT *erupt_rocks[6] = {
    &rubble_0_object, &rubble_1_object, &rubble_2_object,
    &rubble_3_object, &rubble_4_object, &rubble_5_object,
};

/* the flame a thrown rock trails. erupt_rock_step sets its pos and speed
 * before each create */
static struct dust_params erupt_rock_flame = {
    0.5F,
    0.038F,
    0.5F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

/* a thrown rock's task->motion */
struct erupt_rock_motion {
    void *unk_00;
    NJS_VECTOR speed;
    Uint8 unk_10[24];
    struct angles spin; /* added to its angles each step, about x and z */
    Float unk_34;
    /* the height it next lands at: its start height, then the ground below
     * it */
    Float ground;
};

/* the mound's or a blob's task->motion */
struct erupt_motion {
    void *unk_00;
    /* the lava's rising speed, in y alone, or a blob's speed */
    NJS_VECTOR speed;
    /* the red, green and blue of the constant material it draws with */
    NJS_VECTOR color;
    Uint8 unk_1c[12];
    struct angles spin; /* the turn a blob adds to its angles each step */
    Float scale; /* the scale a blob draws at */
    /* the lava's height above the mound, or how far a blob has risen */
    Float height;
};

#define MOTION(task) ((struct erupt_motion *)(task)->motion)

struct erupt_vars {
    Uint8 unk_00[8];
    /* the turn of the next smoke from the camera's side: 0, 45 or -45
     * degrees, in turn */
    Angle smoke_turn;
};

#define VARS(work) ((struct erupt_vars *)&(work)->vars)

struct erupt_blast_vars {
    Uint8 unk_00[8];
    Float growth; /* what the radius gains each step, a tenth less each time */
};

#define BLAST_VARS(work) ((struct erupt_blast_vars *)&(work)->vars)

enum erupt_mode { ERUPT_INIT, ERUPT_WAIT, ERUPT_GLOW, ERUPT_BURST, ERUPT_FLOW };
/* a mode nothing in this file sets */
#define ERUPT_DEAD (-1)

/* set the mode to next and the submode to 0, unless the mode is ERUPT_DEAD */
#define ERUPT_SET_MODE(work, next)                                             \
    do {                                                                       \
        if ((work)->mode != ERUPT_DEAD) {                                      \
            (work)->mode = (next);                                             \
            (work)->submode = 0;                                               \
        }                                                                      \
    } while (0)

/* player 0 nearer than this starts the glow */
#define ERUPT_NEAR 100.0F
/* the glow brightens over this many steps, then the mound bursts */
#define GLOW_STEPS 15
/* the rocks the burst throws */
#define BURST_ROCKS 8
/* the lava's height once risen, above the mound's position */
#define ERUPT_HEIGHT 100.0F

/* the sound of the flowing lava, the sound as player 0 comes near, and the
 * blast's */
#define SE_ERUPT_LAVA 126
#define SE_ERUPT_RUMBLE 129

static void erupt_init(struct task *task, struct task_work *work,
                       struct erupt_motion *motion);
static void erupt_wait(struct task *task, struct task_work *work,
                       struct erupt_motion *motion);
static void erupt_glow(struct task *task, struct task_work *work,
                       struct erupt_motion *motion);
static void erupt_burst(struct task *task, struct task_work *work,
                        struct erupt_motion *motion);
static void erupt_flow(struct task *task, struct task_work *work,
                       struct erupt_motion *motion);
static void erupt_destroy(struct task *task);
static void erupt_draw(struct task *task);
static void erupt_smoke_step(struct particle *particle,
                             struct particle_pool *pool);
static void erupt_smoke(struct task_work *work);
static void erupt_blob_draw(struct task *task);
static void erupt_blob_step(struct task *task);
static void erupt_blob_create(struct task *task, struct task_work *work);
static void erupt_puff(struct task_work *work, struct erupt_vent *vent);
static int erupt_random(int lo, int hi);
static void erupt_draw_mound(struct task_work *work,
                             struct erupt_motion *motion);
static void erupt_blast_step(struct task *task);
static void erupt_rock_draw(struct task *task);
static void erupt_rock_step(struct task *task);
static void erupt_rock_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale);

/* O Erupt's step. it runs the mode, steps the blobs and draws the mound. in
 * the set editor it only draws the mound */
void erupt_step(struct task *task)
{
    struct task_work *work = task->work;
    struct erupt_motion *motion = MOTION(task);

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        erupt_draw_mound(work, motion);
        return;
    } else {
        switch (work->mode) {
        case ERUPT_INIT:
            if (!placement_editing(task)) {
                collision_init(task, erupt_shapes, COLLISION_OBJECT, 1, 2);
            }
            erupt_init(task, work, motion);
            task->destructor = erupt_destroy;
            task->draw = erupt_draw;
            ERUPT_SET_MODE(work, ERUPT_WAIT);
            break;
        case ERUPT_WAIT:
            erupt_wait(task, work, motion);
            break;
        case ERUPT_GLOW:
            erupt_glow(task, work, motion);
            break;
        case ERUPT_BURST:
            ERUPT_SET_MODE(work, ERUPT_FLOW);
            erupt_burst(task, work, motion);
            break;
        case ERUPT_FLOW:
            sound_play_at_for(SE_ERUPT_LAVA, (int)work, 1, 64, work->pos.x,
                              work->pos.y, work->pos.z, 10);
            erupt_flow(task, work, motion);
            break;
        }
        erupt_draw_mound(work, motion);
        task_step_children(task);
        collision_register(work);
    }
}

static void erupt_init(struct task *task, struct task_work *work,
                       struct erupt_motion *motion)
{
    motion->color.x = 0.0F;
    motion->color.y = 0.0F;
    motion->color.z = 0.0F;
}

/* wait for player 0 to come nearer than ERUPT_NEAR, then start the glow.
 * until then, the vents puff on every second step of the last 32 in each
 * 128 */
static void erupt_wait(struct task *task, struct task_work *work,
                       struct erupt_motion *motion)
{
    NJS_POINT3 player;
    Float distance;

    player_get_history(0, 0, &player, NULL);
    distance = njDistanceP2P(&player, &work->pos);
    if (distance < ERUPT_NEAR) {
        ERUPT_SET_MODE(work, ERUPT_GLOW);
        sound_play(SE_ERUPT_RUMBLE, 0, 0, 0);
    } else {
        if ((TASK_TIMER(work) & 64) && (TASK_TIMER(work) & 32)) {
            if (!(TASK_TIMER(work) & 1)) {
                erupt_puff(work, &erupt_vents[0]);
                erupt_puff(work, &erupt_vents[1]);
                erupt_puff(work, &erupt_vents[2]);
            }
        }
        ++work->timer;
    }
}

/* the mound glows redder over GLOW_STEPS steps, and then bursts. its vents
 * puff on every second step */
static void erupt_glow(struct task *task, struct task_work *work,
                       struct erupt_motion *motion)
{
    switch (work->submode) {
    case 0:
        work->submode = 1;
        work->timer = 0;
        /* fall through */
    case 1:
        if (TASK_TIMER(work) == GLOW_STEPS) {
            ERUPT_SET_MODE(work, ERUPT_BURST);
            sound_play_at(SE_ROCK_BLAST, 0, 0, 96, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        /* @hack redundant: each product added to a zero, FLDI0 at 0x0c909574;
         * without them, the function is 6 bytes shorter and 45 of 87 words
         * differ */
        motion->color.x = 0.0F + (TASK_TIMER(work) * (2.0F / GLOW_STEPS));
        motion->color.y =
            0.0F + (TASK_TIMER(work) * (1.0F / GLOW_STEPS) * 0.4F);
        motion->color.z = motion->color.y;
        break;
    }
    if (!(TASK_TIMER(work) & 1)) {
        erupt_puff(work, &erupt_vents[0]);
        erupt_puff(work, &erupt_vents[1]);
        erupt_puff(work, &erupt_vents[2]);
    }
    ++work->timer;
}

/* throw BURST_ROCKS rocks toward player 0, spread across a quarter turn,
 * and start the blast. player 0 turns to look at the mound for 240 steps */
static void erupt_burst(struct task *task, struct task_work *work,
                        struct erupt_motion *motion)
{
    NJS_VECTOR speed;
    NJS_POINT3 player;
    NJS_POINT3 pos;
    struct task *blast;
    int heading;
    int yaw;
    int pitch;
    int i;

    player = player_work[0]->pos;
    heading = (int)(atan2f(player.x - work->pos.x, player.z - work->pos.z) *
                    ANGLE_PER_RADIAN) -
              0x4000;
    i = 0;
    heading -= 0x2000;
    for (; i < BURST_ROCKS; ++i, heading += 0x800) {
        Float fly;
        Float scale;

        fly = (rand() * (4.0F * RAND_SCALE)) + 2.0F;
        yaw = (int)(rand() * (2048.0F * RAND_SCALE)) - heading;
        pitch = 0x1000 + (int)(rand() * (4096.0F * RAND_SCALE));
        pos.x = work->pos.x + (5.0F * njCos(yaw));
        pos.y = work->pos.y + 10.0F;
        pos.z = work->pos.z + (5.0F * njSin(yaw));
        speed.x = njCos(pitch) * njCos(yaw) * fly;
        speed.y = njSin(pitch) * fly;
        speed.z = njCos(pitch) * njSin(yaw) * fly;
        scale = (rand() * (0.5F * RAND_SCALE)) + 0.2F;
        erupt_rock_create(&pos, &speed, scale);
    }
    player_look_at_point(0, &work->pos, 240);
    blast = task_create(TASK_WORK, 3, erupt_blast_step);
    if (blast != NULL) {
        blast->work->pos = work->pos;
    }
}

/* the lava rises to ERUPT_HEIGHT, slowing as it goes, and its column's shape
 * rises with it. every 8 steps it throws a blob, and every 16 a rock toward
 * player 0. every 3 steps a puff of smoke rises from it */
static void erupt_flow(struct task *task, struct task_work *work,
                       struct erupt_motion *motion)
{
    switch (work->submode) {
    case 0:
        motion->speed.y = 3.0F;
        work->submode = 1;
        work->collision->shapes->flags &= ~SHAPE_OFF;
        work->collision->shapes->size_y = 0.0F;
        work->tick = 0;
        break;
    case 1:
        motion->speed.y += -motion->speed.y * 0.02F;
        motion->height += motion->speed.y;
        if (motion->height > ERUPT_HEIGHT) {
            motion->height = ERUPT_HEIGHT;
            work->submode = 2;
        }
        if (!(TASK_TIMER(work) & 7)) {
            /* @hack redundant: never-run reads of task->work, by which SHC
             * weighs it so that cos, rand and pitch take R12, R11 and R10;
             * without it, 15 of 238 words differ */
            if (0) {
                (void)task->work->pos.x;
                (void)task->work->pos.y;
                (void)task->work->pos.z;
            }
            erupt_blob_create(task, work);
        }
        work->collision->shapes->size_y = motion->height;
        break;
    case 2:
        if (!(TASK_TIMER(work) & 7)) {
            erupt_blob_create(task, work);
        }
        break;
    }
    if (!(TASK_TIMER(work) & 15)) {
        NJS_POINT3 pos;
        NJS_VECTOR speed;
        struct task_work *player;
        int heading;
        int yaw;
        int pitch;
        Float fly;
        Float dz;

        /* @hack spelling: player set within the subtraction, which holds it in
         * R2 and the frame (0x0c909bb4) rather than in R13; set first, R13
         * takes it, the function is 2 bytes shorter and 141 of 238 words
         * differ */
        dz = (player = player_work[0])->pos.z - work->pos.z;
        heading =
            (int)(atan2f(player->pos.x - work->pos.x, dz) * ANGLE_PER_RADIAN) -
            0x4000;
        fly = 3.0F + (rand() * (3.0F * RAND_SCALE));
        yaw = (int)(rand() * 0.5F) - heading - 0x2000;
        pitch = 0x2800 + (int)(rand() * (4096.0F * RAND_SCALE));
        pos.x = work->pos.x + (5.0F * njCos(yaw));
        pos.y = work->pos.y + 10.0F;
        pos.z = work->pos.z + (5.0F * njSin(yaw));
        speed.x = njCos(pitch) * njCos(yaw) * fly;
        speed.y = njSin(pitch) * fly;
        speed.z = njCos(pitch) * njSin(yaw) * fly;
        erupt_rock_create(&pos, &speed, (rand() * (0.5F * RAND_SCALE)) + 0.2F);
    }
    ++work->tick;
    if (work->tick == 3) {
        work->tick = 0;
        erupt_smoke(work);
    }
    ++work->timer;
}

static void erupt_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void erupt_draw(struct task *task)
{
    erupt_draw_mound(task->work, MOTION(task));
}

/* the step of a puff of smoke. it plays the bomb pool's frames at 0.4 a
 * step, and ends once past the last. its speed loses 5% a step, its upward
 * speed then gains 0.02, and its scale grows by 0.005 */
static void erupt_smoke_step(struct particle *particle,
                             struct particle_pool *pool)
{
    Float drag = 0.95F;

    particle->frame = (int)floorf(particle->time);
    if (particle->frame >= (Sint16)pool->anim->frames) {
        particle->frame = (Sint16)pool->anim->frames - 1;
        particle_kill(particle);
        return;
    }
    particle->time += 0.4F;
    particle->speed.x *= drag;
    particle->speed.y *= drag;
    particle->speed.z *= drag;
    particle->speed.y += 0.02F;
    particle->pos.x += particle->speed.x;
    particle->pos.y += particle->speed.y;
    particle->pos.z += particle->speed.z;
    particle->scale += 0.005F;
}

/* a puff of smoke from the bomb pool. it rises from 10 out on the camera's
 * side of the lava. successive puffs turn 0, 45 and -45 degrees from the
 * camera's side, in turn */
static void erupt_smoke(struct task_work *work)
{
    struct particle *particle =
        particle_alloc(bomb_pool.pool, erupt_smoke_step);
    Angle ang;
    Float drift;

    if (particle != NULL) {
        ang = (Angle)(atan2f(camera_work->pos.x - work->pos.x,
                             camera_work->pos.z - work->pos.z) *
                      ANGLE_PER_RADIAN) -
              0x4000;
        ang += (Angle)(rand() * (2048.0F * RAND_SCALE)) +
               VARS(work)->smoke_turn - 0x400;
        drift = 0.8F;
        particle->speed.x = njCos(ang) * drift;
        particle->speed.z = -(njSin(ang) * drift);
        particle->pos.x = work->pos.x + (njCos(ang) * 10.0F);
        particle->pos.z = work->pos.z - (njSin(ang) * 10.0F);
        particle->speed.y = 5.5F + (rand() * (2.0F * RAND_SCALE));
        particle->pos.y = work->pos.y;
        particle->scale = 0.4F;
        particle->time = rand() * (2.0F * RAND_SCALE);
        particle->ang = (Angle)(rand() * (8192.0F * RAND_SCALE)) - 0x1000;
        particle->color.a = 1.0F;
        particle->color.r = 1.0F;
        particle->color.g = drift;
        particle->color.b = drift;
    }
    VARS(work)->smoke_turn += 0x2000;
    if (VARS(work)->smoke_turn > 0x2000) {
        VARS(work)->smoke_turn = -0x2000;
    }
}

static void erupt_blob_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct erupt_motion *motion = MOTION(task);
    Angle ang;
    Float scale;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 8.0F)) {
        draw_set_material(1.0F, motion->color.x, motion->color.y,
                          motion->color.z);
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        scale = motion->scale;
        njScale(NULL, scale, scale, scale);
        njSimpleDrawObject(&erupt_blob_object);
        njPopMatrix(1);
        draw_set_material(1.0F, 0.0F, 0.0F, 0.0F);
    }
}

/* a blob of lava flies, slowing, until it rises higher than the lava or 101
 * steps pass */
static void erupt_blob_step(struct task *task)
{
    struct task_work *work = task->work;
    struct erupt_motion *motion = MOTION(task);

    if (TASK_TIMER(work)++ > 100) {
        task_kill(task);
        return;
    }
    motion->speed.x += -(motion->speed.x * 0.02F);
    motion->speed.y += -(motion->speed.y * 0.02F);
    motion->speed.z += -(motion->speed.z * 0.02F);
    work->pos.x += motion->speed.x;
    work->pos.y += motion->speed.y;
    work->pos.z += motion->speed.z;
    work->ang.x += motion->spin.x;
    work->ang.y += motion->spin.y;
    work->ang.z += motion->spin.z;
    motion->height += motion->speed.y;
    if (motion->height > MOTION(task->parent)->height) {
        task_kill(task);
        return;
    }
    erupt_blob_draw(task);
}

/* create a blob of lava within 4 of the mound's center, flying up and a
 * little outward */
static void erupt_blob_create(struct task *task, struct task_work *work)
{
    struct task *child =
        task_create_child(TASK_MOTION | TASK_WORK, erupt_blob_step, task);
    struct task_work *blob;
    struct erupt_motion *motion;
    Angle ang;
    Float reach;

    if (child != NULL) {
        child->draw = erupt_blob_draw;
        blob = child->work;
        motion = MOTION(child);
        blob->timer = 0;
        motion->spin.x = erupt_random(0, 512) - 1024;
        motion->spin.y = erupt_random(0, 512) - 1024;
        ang = erupt_random(0, 65536);
        reach = rand() * RAND_SCALE;
        motion->speed.x = reach * njCos(ang) * 0.1F;
        motion->speed.z = reach * njSin(ang) * 0.1F;
        blob->pos.x = (reach * njCos(ang) * 4.0F) + work->pos.x;
        blob->pos.z = (reach * njSin(ang) * 4.0F) + work->pos.z;
        motion->speed.y = (1.0F + (rand() * (0.2F * RAND_SCALE))) * 3.0F;
        blob->pos.y = work->pos.y;
        motion->scale = (0.8F + (rand() * (0.2F * RAND_SCALE))) * 3.0F;
        motion->color.x = 1.0F;
        motion->color.y = -(rand() * (0.5F * RAND_SCALE));
        motion->color.z = motion->color.y;
    }
}

/* a glowing puff out of vent, 10 out from the mound's center */
static void erupt_puff(struct task_work *work, struct erupt_vent *vent)
{
    NJS_POINT3 origin;
    NJS_VECTOR speed;
    Angle ang;
    Float scale;

    origin = erupt_puff_origin;
    speed.x = 3.0F + (rand() * (1.5F * RAND_SCALE));
    scale = 1.0F;
    speed.y = (-0.5F + (rand() * RAND_SCALE)) * scale;
    speed.z = (-0.5F + (rand() * RAND_SCALE)) * scale;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y + vent->yaw, ang);
    TURN_VIA(njRotateZ, vent->tilt, ang);
    njCalcPoint(NULL, &origin, &erupt_puff_params.pos);
    njCalcVector(NULL, &speed, &erupt_puff_params.speed);
    njPopMatrix(1);
    erupt_puff_params.scale = scale;
    dust_create_glow(&erupt_puff_params);
}

/* a random int from lo up to hi, never hi itself */
static int erupt_random(int lo, int hi)
{
    return lo + (int)((Float)(hi - lo) * rand() * RAND_SCALE);
}

/* draw the mound's ten rocks with motion->color as the constant material,
 * then set the material to black */
static void erupt_draw_mound(struct task_work *work,
                             struct erupt_motion *motion)
{
    int i;
    Angle ang;
    NJS_OBJECT **rock;
    struct erupt_pose *pose;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 50.0F)) {
        draw_set_material(1.0F, motion->color.x, motion->color.y,
                          motion->color.z);
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y + 6.0F, work->pos.z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScale(NULL, 0.8F, 0.8F, 0.8F);
        for (i = 0, rock = &erupt_rocks[0]; i < 5; ++i) {
            pose = &erupt_poses[i];
            njPushMatrix(NULL);
            njTranslateV(NULL, &pose->pos);
            TURN_VIA(njRotateZ, pose->ang.z, ang);
            TURN_VIA(njRotateX, pose->ang.x, ang);
            TURN_VIA(njRotateY, pose->ang.y, ang);
            njScale(NULL, pose->scl.x, pose->scl.y, pose->scl.z);
            draw_model((*rock)->model);
            njPopMatrix(1);
        }
        for (i = 5, rock = &erupt_rocks[1]; i < 10; ++i) {
            pose = &erupt_poses[i];
            njPushMatrix(NULL);
            njTranslateV(NULL, &pose->pos);
            TURN_VIA(njRotateZ, pose->ang.z, ang);
            TURN_VIA(njRotateX, pose->ang.x, ang);
            TURN_VIA(njRotateY, pose->ang.y, ang);
            njScale(NULL, pose->scl.x, pose->scl.y, pose->scl.z);
            draw_model((*rock)->model);
            njPopMatrix(1);
        }
        njPopMatrix(1);
        draw_set_material(1.0F, 0.0F, 0.0F, 0.0F);
    }
}

/* the blast, which hurts players for 31 steps. on its first step, its
 * radius is the shape's 15. from the second, the radius is work->scl.x,
 * which starts at 10. scl.x grows by 2.88, then by a tenth less each step */
static void erupt_blast_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case 0:
        collision_init(task, erupt_blast_shape, COLLISION_OBJECT, 1, 1);
        work->mode = 1;
        work->timer = 0;
        work->scl.x = 10.0F;
        BLAST_VARS(work)->growth = 3.2F;
        break;
    case 1:
        BLAST_VARS(work)->growth *= 0.9F;
        work->scl.x += BLAST_VARS(work)->growth;
        work->collision->shapes->radius = work->scl.x;
        if (TASK_TIMER(work)++ == 30) {
            task_kill(task);
            return;
        }
        break;
    }
    collision_register(work);
}

static void erupt_rock_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float scale = work->scl.x;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 40.0F) &&
        texture_set_stage()) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        njScale(NULL, scale, scale, scale);
        njSimpleDrawModel(MODEL(work)->object->model);
        njPopMatrix(1);
    }
}

/* a thrown rock falls back to its start height, then to the ground below.
 * it bounces twice there and sinks at its third landing, until 20 below
 * the ground. it ends there or on its 241st step, and trails a flame on 3
 * steps of every 4 */
static void erupt_rock_step(struct task *task)
{
    struct task_work *work = task->work;
    struct erupt_rock_motion *motion = (struct erupt_rock_motion *)task->motion;
    struct angles slope;

    switch (work->mode) {
    case 0:
        motion->ground = work->pos.y;
        work->mode = 1;
        task->draw = erupt_rock_draw;
        break;
    case 1:
        if (motion->speed.y > 0.0F) {
            break;
        }
        if (work->pos.y > motion->ground) {
            break;
        }
        motion->ground =
            ground_below(work->pos.x, work->pos.y + 10.0F, work->pos.z, &slope);
        work->mode = 2;
        break;
    case 2:
        if (work->pos.y > motion->ground) {
            break;
        }
        ++work->tick;
        if (work->tick == 3) {
            work->mode = 3;
        } else {
            work->pos.y = motion->ground + 0.1F;
            motion->speed.x += motion->speed.y * njCos(work->ang.y) * 0.4F;
            motion->speed.y *= -0.6F;
            motion->speed.z += motion->speed.y * njSin(work->ang.y) * 0.4F;
            motion->spin.x *= 0.6F;
            motion->spin.z = motion->spin.z * 0.6F;
            work->mode = 1;
        }
        break;
    case 3:
        if (work->pos.y > motion->ground - 20.0F) {
            break;
        }
        task_kill(task);
        return;
    }
    if (TASK_TIMER(work)++ == 240) {
        task_kill(task);
        return;
    }
    if (TASK_TIMER(work) & 3) {
        erupt_rock_flame.pos = work->pos;
        erupt_rock_flame.speed.x = motion->speed.x * 0.7F;
        erupt_rock_flame.speed.y = motion->speed.y * 0.7F;
        erupt_rock_flame.speed.z = motion->speed.z * 0.7F;
        flame_create_from(&erupt_rock_flame);
    }
    motion->speed.x *= 0.97F;
    motion->speed.y *= 0.97F;
    motion->speed.z *= 0.97F;
    motion->speed.y += -0.1F;
    work->pos.x += motion->speed.x;
    work->pos.y += motion->speed.y;
    work->pos.z += motion->speed.z;
    work->ang.x += motion->spin.x;
    work->ang.z += motion->spin.z;
    erupt_rock_draw(task);
}

/* create a burning rock, rubble_4_object at scale, thrown from pos at speed
 * and spinning at random */
static void erupt_rock_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale)
{
    struct task *task;
    struct task_work *work;
    struct erupt_rock_motion *motion;

    task = task_create(TASK_MOTION | TASK_WORK, 3, erupt_rock_step);
    if (task != NULL) {
        work = task->work;
        motion = (struct erupt_rock_motion *)task->motion;
        work->pos = *pos;
        motion->speed = *speed;
        work->scl.x = scale;
        work->ang.y =
            (Angle)(atan2f(speed->x, speed->z) * ANGLE_PER_RADIAN) + 0x4000;
        MODEL(work)->object = &rubble_4_object;
        motion->spin.x = (Angle)(rand() * 0.125F) - 0x800;
        motion->spin.z = (Angle)(rand() * 0.125F) - 0x800;
    }
}
