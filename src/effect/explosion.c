/* explosions: a blast's sprite, and the sparks that fly from it */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "effect/explosion.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "effect/spark.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

/* the blast's sound */
#define SE_EXPLOSION 25

struct explosion_vars {
    Uint8 unk_00[8];
    /* the sparks' clock, up by 0.4 a step. the step that takes it past 7
     * throws none, and ends the explosion */
    Float time;
};

#define VARS(work) ((struct explosion_vars *)&(work)->vars)

enum explosion_mode { EXPLOSION_BLAST, EXPLOSION_SPARK, EXPLOSION_END };

static void blast_step(struct particle *particle, struct particle_pool *pool);

/* EFF_REGULAR's exp01 to 08 */
NJS_TEXLIST explosion_texlist = {
#include "effect/explosion/explosion_texlist.inc"
};
static NJS_TEXANIM explosion_frames[8] = {
#include "effect/explosion/explosion_frames.inc"
};
static struct particle_anim explosion_anim = {
    &explosion_texlist, explosion_frames, 8, NJD_COLOR_BLENDING_SRCALPHA,
    NJD_COLOR_BLENDING_ONE};

struct particle_pool_spec explosion_pool = {&explosion_anim, 0, NULL, NULL};

static void explosion_blast(struct task_work *work)
{
    struct particle_pool *pool = explosion_pool.pool;
    struct particle *particle = particle_alloc(pool, blast_step);

    if (particle != NULL) {
        particle->pos = work->pos;
        particle->scale = work->scl.x;
        particle->time = 0.0F;
        particle->ang = rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE;
        particle->color.a = 1.0F;
        particle->color.r = 1.0F;
        particle->color.g = 1.0F;
        particle->color.b = 1.0F;
    }
    work->mode = EXPLOSION_SPARK;
}

static void explosion_spark(struct task_work *work)
{
    NJS_VECTOR speed;
    int i;

    VARS(work)->time += 0.4F;
    if (VARS(work)->time > 7.0F) {
        work->mode = EXPLOSION_END;
        return;
    }
    for (i = 0; i < 3; i++) {
        speed.x = -2.75F + (rand() * RAND_SCALE * 5.5F);
        speed.y = -2.0F + (rand() * RAND_SCALE * 5.5F);
        speed.z = -2.75F + (rand() * RAND_SCALE * 5.5F);
        spark_create(&work->pos, &speed, 1.0F);
    }
}

static void explosion_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case EXPLOSION_BLAST:
        explosion_blast(work);
        break;
    case EXPLOSION_SPARK:
        explosion_spark(work);
        break;
    case EXPLOSION_END:
        task_kill(task);
        break;
    }
}

static void blast_step(struct particle *particle, struct particle_pool *pool)
{
    particle->frame = particle->time;
    if (particle->frame > 7) {
        particle->frame = 7;
        particle_kill(particle);
    } else {
        particle->time += 0.4F;
    }
}

void explosion_create(struct task_work *work, Float scale)
{
    struct task *task;
    struct task_work *explosion;

    sound_play(SE_EXPLOSION, 0, 0, 0);
    task = task_create(TASK_WORK, 6, explosion_step);
    if (task != NULL) {
        explosion = task->work;
        explosion->pos = work->pos;
        explosion->scl.x = scale;
    }
}

void explosion_create_at(Float x, Float y, Float z, Float scale)
{
    struct task *task;
    struct task_work *explosion;

    sound_play(SE_EXPLOSION, 0, 0, 0);
    task = task_create(TASK_WORK, 6, explosion_step);
    if (task != NULL) {
        explosion = task->work;
        explosion->pos.x = x;
        explosion->pos.y = y;
        explosion->pos.z = z;
        explosion->scl.x = scale;
    }
}
