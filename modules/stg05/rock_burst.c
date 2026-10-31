/* the two rocks that burst under player 0 in Red Mountain act 0, and Ex
 * Rock, which kills itself on its first step */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg05/rock_burst.h"
#include "stg05/rubble.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

#define TEXT_1_LENGTH 5
#define TEXT_2_LENGTH 5
#define TEXT_3_LENGTH 5
#define TEXT_4_LENGTH 5
#define TEXT_5_LENGTH 5
#define TEXT_6_LENGTH 5
#define TEXT_7_LENGTH 5
#define TEXT_8_LENGTH 5
#include "stg05/rock_burst/text.inc"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_OBJECT rock_burst_0_piece_0_object;
extern NJS_OBJECT rock_burst_0_piece_1_object;
extern NJS_OBJECT rock_burst_0_piece_2_object;
extern NJS_OBJECT rock_burst_0_piece_3_object;
extern NJS_OBJECT rock_burst_0_piece_4_object;
extern NJS_OBJECT rock_burst_0_piece_5_object;
extern NJS_OBJECT rock_burst_0_piece_6_object;
extern NJS_OBJECT rock_burst_0_piece_7_object;
/* rock 1, whose children are its pieces */
extern NJS_OBJECT rock_burst_1_object;

/* a spot on a rock, which puffs dust until the rock bursts, and throws
 * fragments as it does */
struct rock_burst_spot {
    NJS_POINT3 pos;
    Uint16 start; /* the timer's first count, which staggers the spots */
    /* rock_burst_puff_spots counts it from 0 to 101, then from 0 again */
    Uint16 timer;
};

enum rock_burst_mode { ROCK_INIT, ROCK_HOLD, ROCK_FLING, ROCK_DONE };

/* in a rock's work->flags. its pieces set ROCK_STOOD_ON, and the rock sets
 * ROCK_BURST */
#define ROCK_STOOD_ON 0x100 /* player 0 stood on a piece last step */
#define ROCK_BURST 0x200 /* the rock has burst, and its pieces fly */

/* where both rocks' tasks stand: among rock 1's spots, and 290 to 390 from
 * each of rock 0's. so rock 0's task stands at rock 1's place, not its own.
 * before a rock bursts, it and its pieces step only while player 0 is within
 * ROCKS_RANGE of it. each rock's burst sounds at this position */
#define ROCKS_X (-573.0F)
#define ROCKS_Y 1134.0F
#define ROCKS_Z 0.0F
#define ROCKS_RANGE 1000.0F
/* the steps running that player 0 must stand on a rock's pieces. the rock
 * bursts on the next step, if player 0 still stands on them */
#define ROCK_HOLD_STEPS 20

/* a piece's layout of its task->motion */
struct rock_burst_piece_motion {
    void *unk_00;
    NJS_VECTOR speed;
    NJS_POINT3 center; /* its model's center, where the piece stands */
    Uint8 unk_1c[24];
    Float radius; /* its model's radius */
};

#define PIECE_MOTION(task) ((struct rock_burst_piece_motion *)(task)->motion)

struct rock_burst_piece_vars {
    /* its copy of from's model in the world until it flies, then NULL */
    NJS_OBJECT *world;
    /* the object whose place it takes and whose model it draws */
    NJS_OBJECT *from;
};

#define PIECE(work) ((struct rock_burst_piece_vars *)&(work)->vars)

enum rock_burst_piece_mode { PIECE_INIT, PIECE_STAND, PIECE_FLY };

/* the world tests a piece's object only while player 0 is within this of
 * its model's sphere */
#define PIECE_MARGIN 10.0F
#define PIECE_LIFE 200 /* the steps a piece flies */

/* a fragment's layout of its task->motion */
struct rock_burst_fragment_motion {
    void *unk_00;
    NJS_VECTOR speed;
    Uint8 unk_10[24];
    struct angles spin; /* its turn about x and z each step */
};

#define FRAGMENT_MOTION(task)                                                  \
    ((struct rock_burst_fragment_motion *)(task)->motion)

struct rock_burst_fragment_vars {
    NJS_OBJECT *object; /* the object whose model it draws */
};

#define FRAGMENT(work) ((struct rock_burst_fragment_vars *)&(work)->vars)

enum rock_burst_fragment_mode { FRAGMENT_INIT, FRAGMENT_FALL };

#define FRAGMENT_LIFE 60 /* the steps a fragment falls after its first */

#define DUST_FRAMES 8

/* the dust a falling fragment trails */
static struct dust_params rock_burst_trail = {
    0.5F,
    0.038F,
    0.3F,
    0.06F,
    -0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

static NJS_OBJECT *rock_burst_0_pieces[9] = {
    &rock_burst_0_piece_0_object,
    &rock_burst_0_piece_1_object,
    &rock_burst_0_piece_2_object,
    &rock_burst_0_piece_3_object,
    &rock_burst_0_piece_4_object,
    &rock_burst_0_piece_5_object,
    &rock_burst_0_piece_6_object,
    &rock_burst_0_piece_7_object,
    NULL,
};

static struct rock_burst_spot rock_burst_0_spots[4] = {
    {{-802.0F, 1088.0F, -203.0F}, 0, 0},
    {{-783.0F, 1085.0F, -195.0F}, 15, 0},
    {{-820.0F, 1106.0F, -160.0F}, 40, 0},
    {{-864.0F, 1096.0F, -257.0F}, 70, 0},
};

static struct rock_burst_spot rock_burst_1_spots[5] = {
    {{-569.0F, 1123.0F, -23.0F}, 0, 0}, {{-580.0F, 1129.0F, -29.0F}, 22, 0},
    {{-630.0F, 1125.0F, -4.0F}, 50, 0}, {{-552.0F, 1129.0F, -58.0F}, 60, 0},
    {{-561.0F, 1118.0F, 33.0F}, 80, 0},
};

static NJS_TEXNAME rock_burst_dust_textures[DUST_FRAMES] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};
NJS_TEXLIST rock_burst_dust_texlist = {
#include "stg05/rock_burst/rock_burst_dust_texlist.inc"
};
static NJS_TEXANIM rock_burst_dust_frames[DUST_FRAMES] = {
#include "stg05/rock_burst/rock_burst_dust_frames.inc"
};
static struct particle_anim rock_burst_dust_anim = {
    &rock_burst_dust_texlist, rock_burst_dust_frames, DUST_FRAMES,
    NJD_COLOR_BLENDING_SRCALPHA, NJD_COLOR_BLENDING_ONE};

struct particle_pool_spec rock_burst_dust_pool = {&rock_burst_dust_anim, 0,
                                                  NULL, NULL};

/* the dust a spot puffs. it starts red, and rock_burst_dust_step whitens
 * it */
static struct dust_params rock_burst_dust_params = {
    1.0F,
    0.08F,
    0.5F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.8F, 1.0F, 0.3F, 0.3F},
};

/* the speed across x and z, per step, at which each rock flings player 0 */
static Float rock_burst_0_throw_speed = 5.0F;
static Float rock_burst_1_throw_speed = 5.0F;

static void rock_burst_fragment_draw(struct task *task);
static void rock_burst_fragment_step(struct task *task);
static void rock_burst_fragment_create(NJS_POINT3 *pos, NJS_VECTOR *speed,
                                       Float scale);
static void rock_burst_0_step(struct task *task);
static void rock_burst_destroy(struct task *task);
static void rock_burst_1_step(struct task *task);
static void rock_burst_piece_step(struct task *task);
static void rock_burst_piece_destroy(struct task *task);
static void rock_burst_piece_draw(struct task *task);
static void rock_burst_dust_step(struct particle *particle,
                                 struct particle_pool *pool);
static void rock_burst_dust_create(NJS_POINT3 *pos, NJS_VECTOR *speed,
                                   Float scale);
static void rock_burst_reset_spots(struct rock_burst_spot *spots, int count);
static void rock_burst_puff_spots(struct rock_burst_spot *spots, int count);
static void rock_burst_blast_spots(struct rock_burst_spot *spots, int count);
static void rock_burst_0_throw(Float x, Float y, Float z);
static void rock_burst_1_throw(Float x, Float y, Float z);

void ex_rock_step(struct task *task)
{
    /* @hack redundant: a point whose address the step takes and never uses,
     * which reserves the 12-byte frame at 0x0c90af62 and makes placement_kill
     * a call that returns here, not a tail jump; without it, the function is
     * 12 bytes shorter and 9 of 9 words differ */
    NJS_POINT3 unused;

    (void)&unused;
    placement_kill(task);
}

static void rock_burst_fragment_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float scale = work->scl.x;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 40.0F) &&
        texture_set_stage()) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        njScale(NULL, scale, scale, scale);
        draw_model(FRAGMENT(work)->object->model);
        njPopMatrix(1);
    }
}

/* a fragment falls and slows for 61 steps. while it drops, it trails dust
 * on 15 steps in 16 */
static void rock_burst_fragment_step(struct task *task)
{
    struct task_work *work = task->work;
    struct rock_burst_fragment_motion *motion = FRAGMENT_MOTION(task);

    switch (work->mode) {
    case FRAGMENT_INIT:
        work->mode = FRAGMENT_FALL;
        task->draw = rock_burst_fragment_draw;
        break;
    case FRAGMENT_FALL:
        if (TASK_TIMER(work)++ < FRAGMENT_LIFE) {
            break;
        }
        task_kill(task);
        return;
    }
    if ((TASK_TIMER(work) & 15) && motion->speed.y < 0.0F) {
        rock_burst_trail.pos = work->pos;
        rock_burst_trail.speed.x = motion->speed.x * 0.5F;
        rock_burst_trail.speed.y = motion->speed.y * 0.5F;
        rock_burst_trail.speed.z = motion->speed.z * 0.5F;
        dust_create_from(&rock_burst_trail);
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
    rock_burst_fragment_draw(task);
}

static void rock_burst_fragment_create(NJS_POINT3 *pos, NJS_VECTOR *speed,
                                       Float scale)
{
    struct task *task =
        task_create(TASK_MOTION | TASK_WORK, 3, rock_burst_fragment_step);
    struct task_work *work;
    struct rock_burst_fragment_motion *motion;

    if (task) {
        work = task->work;
        motion = FRAGMENT_MOTION(task);
        work->pos = *pos;
        motion->speed = *speed;
        work->scl.x = scale;
        FRAGMENT(work)->object = &rubble_4_object;
        motion->spin.x = (Angle)(rand() * 0.125F) - 0x800;
        motion->spin.z = (Angle)(rand() * 0.125F) - 0x800;
    }
}

void rock_burst_create(void)
{
    task_create(TASK_WORK, 3, rock_burst_0_step);
    task_create(TASK_WORK, 3, rock_burst_1_step);
}

/* rock 0. until it bursts, it puffs dust from its spots. it counts the steps
 * player 0 stands on its pieces. it flings player 0 the step after it
 * bursts. it then steps its pieces until they are gone, and destroys
 * itself */
static void rock_burst_0_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT **from;
    struct task *child;

    switch (work->mode) {
    case ROCK_INIT:
        work->pos.x = ROCKS_X;
        work->pos.y = ROCKS_Y;
        work->pos.z = ROCKS_Z;
        /* nothing reads the rock's scale. each piece starts with the rock's
         * angles. the rock then sets the piece's y angle, and the piece's init
         * sets its x and z. so no piece keeps the rock's angles */
        work->scl.x = 34.0F;
        work->ang.y = 0x45cb;
        work->ang.z = 0x2e1c;
        task->destructor = rock_burst_destroy;
        for (from = rock_burst_0_pieces; *from; ++from) {
            child = task_create_child(TASK_MOTION | TASK_WORK | TASK_FORCE,
                                      rock_burst_piece_step, task);
            if (child) {
                PIECE(child->work)->from = *from;
                child->work->ang.y = 0;
            }
        }
        rock_burst_reset_spots(rock_burst_0_spots, 4);
        work->mode = ROCK_HOLD;
        break;
    case ROCK_HOLD:
        if (njDistanceP2P(&player_work[0]->pos, &work->pos) > ROCKS_RANGE) {
            return;
        }
        rock_burst_puff_spots(rock_burst_0_spots, 4);
        if (!(work->flags & ROCK_STOOD_ON)) {
            work->timer = 0;
            break;
        }
        work->flags &= ~ROCK_STOOD_ON;
        if (TASK_TIMER(work)++ < ROCK_HOLD_STEPS) {
            break;
        }
        sound_play_at(SE_ROCK_BLAST, 0, 0, 0, work->pos.x, work->pos.y,
                      work->pos.z);
        rock_burst_blast_spots(rock_burst_0_spots, 4);
        work->flags |= ROCK_BURST;
        work->mode = ROCK_FLING;
        break;
    case ROCK_FLING:
        rock_burst_0_throw(-931.0F, 856.0F, -933.0F);
        work->mode = ROCK_DONE;
        break;
    case ROCK_DONE:
        task->step = task_destroy_children;
        return;
    }
    task_step_children(task);
}

static void rock_burst_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* rock 1, which steps as rock 0 does with its own pieces, spots and fling.
 * its pieces fly along -x, and rock 0's along x */
static void rock_burst_1_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *rock;
    NJS_OBJECT *from;
    struct task *child;

    switch (work->mode) {
    case ROCK_INIT:
        work->pos.x = ROCKS_X;
        work->pos.y = ROCKS_Y;
        work->pos.z = ROCKS_Z;
        /* as with rock 0, nothing reads the rock's scale, and no piece keeps
         * its angles */
        work->scl.x = 5.0F;
        work->ang.y = 0;
        work->ang.z = 0x2000;
        task->destructor = rock_burst_destroy;
        /* @hack spelling: the object through rock, which R4 takes at 0x0c90b4a2
         * and the child loads from at +44; by name, the pool holds
         * &rock_burst_1_object.child, and 38 of 159 words differ */
        rock = &rock_burst_1_object;
        for (from = rock->child; from; from = from->sibling) {
            child = task_create_child(TASK_MOTION | TASK_WORK | TASK_FORCE,
                                      rock_burst_piece_step, task);
            if (child) {
                PIECE(child->work)->from = from;
                child->work->ang.y = 0x8000;
            }
        }
        rock_burst_reset_spots(rock_burst_1_spots, 5);
        work->mode = ROCK_HOLD;
        break;
    case ROCK_HOLD:
        if (njDistanceP2P(&player_work[0]->pos, &work->pos) > ROCKS_RANGE) {
            return;
        }
        rock_burst_puff_spots(rock_burst_1_spots, 5);
        if (!(work->flags & ROCK_STOOD_ON)) {
            work->timer = 0;
            break;
        }
        work->flags &= ~ROCK_STOOD_ON;
        if (TASK_TIMER(work)++ < ROCK_HOLD_STEPS) {
            break;
        }
        sound_play_at(SE_ROCK_BLAST, 0, 0, 0, work->pos.x, work->pos.y,
                      work->pos.z);
        rock_burst_blast_spots(rock_burst_1_spots, 5);
        work->flags |= ROCK_BURST;
        work->mode = ROCK_FLING;
        break;
    case ROCK_FLING:
        rock_burst_1_throw(-800.0F, 1120.0F, -125.0F);
        work->mode = ROCK_DONE;
        break;
    case ROCK_DONE:
        task->step = task_destroy_children;
        return;
    }
    task_step_children(task);
}

/* a piece is solid where its model stands until its rock bursts. it then
 * flies off along its y angle for PIECE_LIFE steps */
static void rock_burst_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct rock_burst_piece_motion *motion = PIECE_MOTION(task);
    NJS_OBJECT *world;
    NJS_MODEL *model;
    Float speed;

    switch (work->mode) {
    case PIECE_INIT:
        model = PIECE(work)->from->model;
        world = world_object_alloc();
        PIECE(work)->world = world;
        world->evalflags = NJD_EVAL_BREAK;
        world->scl[0] = 1.0F;
        world->scl[1] = 1.0F;
        world->scl[2] = 1.0F;
        world->ang[0] = 0;
        world->ang[1] = 0;
        world->ang[2] = 0;
        world->pos[0] = PIECE(work)->from->pos[0];
        world->pos[1] = PIECE(work)->from->pos[1];
        world->pos[2] = PIECE(work)->from->pos[2];
        world->model = model;
        world->child = NULL;
        world->sibling = NULL;
        world_add(WORLD_MEDIUM | WORLD_SOLID, task, world);
        work->pos.x = PIECE(work)->from->pos[0];
        work->pos.y = PIECE(work)->from->pos[1];
        work->pos.z = PIECE(work)->from->pos[2];
        work->ang.x = 0;
        work->ang.z = 0;
        motion->center.x = work->pos.x + model->center.x;
        motion->center.y = work->pos.y + model->center.y;
        motion->center.z = work->pos.z + model->center.z;
        motion->radius = model->r;
        task->draw = rock_burst_piece_draw;
        task->destructor = rock_burst_piece_destroy;
        work->mode = PIECE_STAND;
        break;
    case PIECE_STAND:
        work->flags &= ~WORLD_TESTED;
        if (njDistanceP2P(&player_work[0]->pos, &motion->center) <
            motion->radius + PIECE_MARGIN) {
            work->flags |= WORLD_TESTED;
        }
        if (player_stands_on(0, task) == TRUE) {
            task->parent->work->flags |= ROCK_STOOD_ON;
        }
        if (!(task->parent->work->flags & ROCK_BURST)) {
            break;
        }
        speed = 4.0F + (rand() * (0.5F * RAND_SCALE));
        motion->speed.x = njCos(work->ang.y) * speed;
        motion->speed.y = rand() * (4.0F * RAND_SCALE);
        motion->speed.z = njSin(work->ang.y) * speed;
        if (PIECE(work)->world) {
            world_remove(task, PIECE(work)->world);
            world_object_free(PIECE(work)->world);
            PIECE(work)->world = NULL;
        }
        work->mode = PIECE_FLY;
        break;
    case PIECE_FLY:
        if (TASK_TIMER(work)++ == PIECE_LIFE) {
            task_kill(task);
            return;
        }
        motion->speed.x *= 0.94F;
        motion->speed.y *= 0.94F;
        motion->speed.z *= 0.94F;
        motion->speed.y += -0.1F;
        work->pos.x += motion->speed.x;
        work->pos.y += motion->speed.y;
        work->pos.z += motion->speed.z;
        break;
    }
    rock_burst_piece_draw(task);
}

static void rock_burst_piece_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (PIECE(work)->world) {
        world_remove(task, PIECE(work)->world);
        world_object_free(PIECE(work)->world);
    }
}

static void rock_burst_piece_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 40.0F)) {
        draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        draw_model(PIECE(work)->from->model);
        njPopMatrix(1);
        draw_set_material(1.0F, 0.0F, 0.0F, 0.0F);
    }
}

static void rock_burst_dust_step(struct particle *particle,
                                 struct particle_pool *pool)
{
    struct dust_params *params = DUST(particle)->params;
    Float drag;

    particle->frame = particle->time;
    if (particle->frame >= DUST_FRAMES) {
        particle->frame = DUST_FRAMES - 1;
        particle_kill(particle);
        return;
    }
    particle->time += params->rate;
    particle->scale += params->growth;
    if (particle->color.g < 1.0F) {
        particle->color.g += 0.07F;
        particle->color.b += 0.07F;
    }
    drag = 1.0F - params->drag;
    particle->speed.x *= drag;
    particle->speed.y *= drag;
    particle->speed.z *= drag;
    particle->speed.y += params->lift;
    particle->pos.x += particle->speed.x;
    particle->pos.y += particle->speed.y;
    particle->pos.z += particle->speed.z;
}

static void rock_burst_dust_create(NJS_POINT3 *pos, NJS_VECTOR *speed,
                                   Float scale)
{
    struct dust_params *params = &rock_burst_dust_params;
    struct particle_pool *pool = rock_burst_dust_pool.pool;
    struct particle *particle = particle_alloc(pool, rock_burst_dust_step);

    if (particle) {
        particle->pos = *pos;
        particle->speed = *speed;
        particle->scale = scale * DUST_SIZE;
        particle->time = 0.0F;
        particle->ang =
            (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
        particle->color = params->color;
        DUST(particle)->params = params;
    }
}

static void rock_burst_reset_spots(struct rock_burst_spot *spots, int count)
{
    int i;

    for (i = 0; i < count; ++i) {
        spots[i].timer = spots[i].start;
    }
}

/* puff dust from each spot whose timer reads an odd count below 20, and move
 * every timer on. a spot puffs 10 times in every 102 calls */
static void rock_burst_puff_spots(struct rock_burst_spot *spots, int count)
{
    int i;
    Uint16 timer;
    NJS_VECTOR speed;

    for (i = 0; i < count; ++i) {
        if ((timer = spots[i].timer++) > 100) {
            spots[i].timer = 0;
        } else if (timer < 20 && (timer & 1)) {
            speed.x = 0.0F;
            speed.y = 2.5F + (rand() * (0.1F * RAND_SCALE));
            speed.z = 0.0F;
            rock_burst_dust_create(&spots[i].pos, &speed,
                                   2.0F + (rand() * (0.2F * RAND_SCALE)));
        }
    }
}

/* raise a bomb puff at each spot. throw sixteen fragments from it, across a
 * quarter turn about the way to the camera */
static void rock_burst_blast_spots(struct rock_burst_spot *spots, int count)
{
    int i;
    Angle ang;
    Angle yaw;
    Angle pitch;
    Angle heading;
    Float fly;
    NJS_POINT3 pos;
    NJS_VECTOR speed;

    for (i = 0; i < count; ++i) {
        pos = spots[i].pos;
        bomb_create(&pos, 2.0F);
        heading = (Angle)(atan2f(camera_work->pos.x - pos.x,
                                 camera_work->pos.z - pos.z) *
                          ANGLE_PER_RADIAN) -
                  0x4000;
        for (ang = heading - 0x2000; ang < heading + 0x2000; ang += 0x400) {
            fly = 2.0F + (rand() * (4.0F * RAND_SCALE));
            yaw = (Angle)(rand() * 0.25F) - ang - 0x1000;
            pitch = (Angle)(rand() * 0.25F) + 0x1000;
            speed.x = njCos(pitch) * njCos(yaw) * fly;
            speed.y = njSin(pitch) * fly;
            speed.z = njCos(pitch) * njSin(yaw) * fly;
            rock_burst_fragment_create(&pos, &speed,
                                       0.1F + (rand() * (0.2F * RAND_SCALE)));
        }
    }
}

/* fling player 0 toward x, y, z, at rock_burst_0_throw_speed a step across
 * x and z. from y or below, player 0 reaches y as it reaches x and z, under
 * its gravity. from above y, y plays no part. the upward speed then depends
 * on player 0's gravity and fall limit. it also depends on the steps the
 * fling takes across x and z */
static void rock_burst_0_throw(Float x, Float y, Float z)
{
    Float px = player_work[0]->pos.x;
    Float py = player_work[0]->pos.y;
    Float pz = player_work[0]->pos.z;
    Float time;
    Float fall_time;
    Float gravity = player_get(0)->consts.gravity;
    Float limit = player_get(0)->consts.fall_limit;
    NJS_VECTOR speed;
    NJS_VECTOR line;

    time = njSqrt(((px - x) * (px - x)) + ((pz - z) * (pz - z)));
    time /= rock_burst_0_throw_speed;
    speed.x = (x - px) / time;
    speed.z = (z - pz) / time;
    if (py > y) {
        fall_time = limit / gravity;
        /* from here, time holds a height, not a count of steps */
        time = py + (limit * (time - fall_time));
        time += fall_time * fall_time * gravity * 0.5F;
        speed.y = (time - py) / fall_time;
        /* @hack redundant: never-run tests, whose reads SHC weighs: they put x,
         * y and z at sp+0, sp+4 and sp+8 and compute speed.y in FR0, leaving
         * time in FR4; without it, 67 of 135 words differ */
        if (0) {
            if (time + time) {
            }
            if (py + px) {
            }
            if (py + py) {
            }
            if (py) {
            }
            if (fall_time + limit) {
            }
        }
    } else {
        speed.y = (y + (time * time * gravity * 0.5F) - py) / time;
    }
    line = speed;
    njUnitVector(&line);
    player_fling(0, njScalor(&speed), &line);
}

/* fling player 0 toward x, y, z, at rock_burst_1_throw_speed a step across
 * x and z. from any height, it reaches y as it reaches x and z, under its
 * gravity */
static void rock_burst_1_throw(Float x, Float y, Float z)
{
    Float px = player_work[0]->pos.x;
    Float py = player_work[0]->pos.y;
    Float pz = player_work[0]->pos.z;
    Float time;
    Float gravity = player_get(0)->consts.gravity;
    /* @hack redundant: the fall limit read at 0x0c90bf80 and never used;
     * without it, the function is 10 bytes shorter and 90 of 115 words
     * differ */
    Float limit = player_get(0)->consts.fall_limit;
    NJS_VECTOR speed;
    NJS_VECTOR line;

    time = njSqrt(((px - x) * (px - x)) + ((pz - z) * (pz - z)));
    time /= rock_burst_1_throw_speed;
    speed.x = (x - px) / time;
    speed.z = (z - pz) / time;
    speed.y = (y + (time * time * gravity * 0.5F) - py) / time;
    line = speed;
    njUnitVector(&line);
    player_fling(0, njScalor(&speed), &line);
}
