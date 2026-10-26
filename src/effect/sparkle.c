/* sparkles: glints that drift and spin, and a glint that comes and goes */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "effect/particle.h"
#include "effect/pools.h"
#include "effect/sparkle.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

/* a glint's frames, and its scale at a size of 1 */
#define GLINT_FRAMES 7
#define GLINT_SIZE 0.1F

/* a sparkle's frames, and its size at a scale of 1 */
#define SPARKLE_FRAMES 4
#define SPARKLE_SIZE 1.0F

struct sparkle_vars {
    Angle spin; /* the turn each step */
};

/* a glint's scl.x is its scale, and scl.z the frame it's at */
struct glint_vars {
    Uint8 unk_00[8];
    Float rate; /* frames a step */
};

#define SPARKLE(particle) ((struct sparkle_vars *)&(particle)->vars)
#define GLINT(work) ((struct glint_vars *)&(work)->vars)

static void glint_step(struct task *task);
static void glint_draw(struct task *task);
static void glint_destroy(struct task *task);

/* the glint's textures, by frame: kiran01 up to 04 and back */
static const Sint32 glint_textures[GLINT_FRAMES] = {
#include "effect/sparkle/glint_textures.inc"
};

/* EFF_REGULAR's kiran01 to 04 */
NJS_TEXLIST sparkle_texlist = {
#include "effect/sparkle/sparkle_texlist.inc"
};
static NJS_TEXANIM sparkle_frames[SPARKLE_FRAMES] = {
#include "effect/sparkle/sparkle_frames.inc"
};
static struct particle_anim sparkle_anim = {
    &sparkle_texlist, sparkle_frames, SPARKLE_FRAMES,
    NJD_COLOR_BLENDING_SRCALPHA, NJD_COLOR_BLENDING_ONE};

struct particle_pool_spec sparkle_pool = {&sparkle_anim, 0, NULL, NULL};

static void sparkle_step(struct particle *particle, struct particle_pool *pool)
{
    particle->frame = particle->time;
    if (particle->frame >= (Sint16)pool->anim->frames) {
        particle->frame = pool->anim->frames - 1;
        particle_kill(particle);
        return;
    }
    particle->time += 0.01F;
    particle->speed.x *= 1.02F;
    particle->speed.y *= 0.98F;
    particle->speed.z *= 1.02F;
    particle->pos.x += particle->speed.x;
    particle->pos.y += particle->speed.y;
    particle->pos.z += particle->speed.z;
    particle->ang += SPARKLE(particle)->spin;
    particle->scale *= 59.0F / 60.0F;
}

void sparkle_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale, Angle spin)
{
    struct particle_pool *pool = sparkle_pool.pool;
    struct particle *particle = particle_alloc(pool, sparkle_step);

    if (particle != NULL) {
        particle->pos = *pos;
        particle->speed = *speed;
        particle->scale = scale * SPARKLE_SIZE;
        particle->time = 0.0F;
        particle->ang = rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE;
        particle->color.a = 1.0F;
        particle->color.r = 1.0F;
        particle->color.g = 1.0F;
        particle->color.b = 1.0F;
        SPARKLE(particle)->spin =
            (spin * (Float)rand() * RAND_SCALE) - (Angle)(spin / 2);
    }
}

void sparkle_create_glint(NJS_POINT3 *pos, Float size, Float rate)
{
    struct task *task;
    struct task_work *work;

    task = task_create(TASK_WORK, 3, glint_step);
    work = task->work;
    task->destructor = glint_destroy;
    task->draw = glint_draw;
    work->scl.x = size * GLINT_SIZE;
    work->scl.z = 0.0F;
    GLINT(work)->rate = rate;
    work->pos = *pos;
}

static void glint_step(struct task *task)
{
    struct task_work *work = task->work;

    work->scl.z += GLINT(work)->rate;
    if ((Uint32)work->scl.z >= GLINT_FRAMES) {
        task_kill(task);
        return;
    }
    glint_draw(task);
}

static void glint_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_SPRITE sprite;

    sprite.p = work->pos;
    sprite.sx = work->scl.x;
    sprite.sy = work->scl.x;
    sprite.ang = 0;
    sprite.tlist = &sparkle_texlist;
    sprite.tanim = sparkle_frames;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njDrawSprite3D(&sprite, glint_textures[(Uint32)work->scl.z],
                   NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
}

static void glint_destroy(struct task *task)
{
}
