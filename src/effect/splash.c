/* splashes: drops that fly up out of the water, and the ripple that spreads
 * where something meets it */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "draw/draw.h"
#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "effect/splash.h"
#include "object/models.h"
#ifdef GCC
#include "system/frame.h"
#endif
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/texture.h"
#include "system/util.h"

/* a drop's frames, and its size at a scale of 1 */
#define SPLASH_FRAMES 8
#define SPLASH_SIZE 0.17F

/* a ripple's scl.x is its size */
struct ripple_vars {
    /* the r, g and b of its constant material, from -0.5 down past -1 */
    Float fade;
    Float fade_size; /* it starts to fade past this */
    Float growth; /* added to its size each step */
};

#define RIPPLE(work) ((struct ripple_vars *)&(work)->vars)

enum ripple_mode { RIPPLE_GROW, RIPPLE_FADE, RIPPLE_END };

static void ripple_init(struct task *task);
static void ripple_step(struct task *task);
static void ripple_draw(struct task *task);
static void ripple_destroy(struct task *task);

/* EFF_REGULAR's splash_a01 to 08 */
NJS_TEXLIST splash_texlist = {
#include "effect/splash/splash_texlist.inc"
};
static NJS_TEXANIM splash_frames[SPLASH_FRAMES] = {
#include "effect/splash/splash_frames.inc"
};
static struct particle_anim splash_anim = {
    &splash_texlist, splash_frames, SPLASH_FRAMES, NJD_COLOR_BLENDING_ONE,
    NJD_COLOR_BLENDING_ONE};

struct particle_pool_spec splash_pool = {&splash_anim, 0, NULL, NULL};

/* splash_create's params */
static struct dust_params splash_defaults = {
    1.0F,
    0.038F,
    0.19F,
    0.115F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

static void splash_step(struct particle *particle, struct particle_pool *pool)
{
    struct dust_params *params = DUST(particle)->params;
    Float keep;

    particle->frame = particle->time;
    if (particle->frame > SPLASH_FRAMES - 1) {
        particle->frame = SPLASH_FRAMES - 1;
        particle_kill(particle);
        return;
    }
    if (particle->time < 6.0F) {
        particle->time += params->rate * 1.4F;
    } else {
        particle->time += params->rate * 2.8F;
    }
    keep = 1.0F - params->drag;
    /* @hack spelling: the products and sums spelled out, which SHC works in
     * registers (FMAC at 0x8c049720); with *= and +=, it forms each speed's
     * address in R1 to read it for the sums, the function runs 6 bytes long
     * and 53 of 84 words differ */
    particle->speed.x = particle->speed.x * keep;
    particle->speed.y = (particle->speed.y * keep) + params->lift;
    particle->speed.z = particle->speed.z * keep;
    particle->pos.x = particle->pos.x + particle->speed.x;
    particle->pos.y = particle->pos.y + particle->speed.y;
    particle->pos.z = particle->pos.z + particle->speed.z;
    particle->scale += params->growth * (1.5F * SPLASH_SIZE);
}

void splash_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale)
{
    struct particle_pool *pool = splash_pool.pool;
    struct particle *particle = particle_alloc(pool, splash_step);

    if (particle != NULL) {
        particle->pos = *pos;
        particle->speed = *speed;
        particle->scale = scale * SPLASH_SIZE;
        particle->time = rand() * RAND_SCALE * 5.0F;
        particle->ang = RANDOM(100.0F, 260.0F) * ANGLE_PER_DEGREE;
        particle->color = splash_defaults.color;
        DUST(particle)->params = &splash_defaults;
    }
}

void splash_create_from(struct dust_params *params)
{
    struct particle_pool *pool = splash_pool.pool;
    struct particle *particle = particle_alloc(pool, splash_step);

    if (particle != NULL) {
        particle->pos = params->pos;
        particle->speed = params->speed;
        particle->scale = params->scale;
        particle->time = rand() * RAND_SCALE * 5.0F;
        particle->ang = RANDOM(100.0F, 260.0F) * ANGLE_PER_DEGREE;
        particle->color = params->color;
        DUST(particle)->params = params;
    }
}

void ripple_create(NJS_POINT3 *pos, NJS_VECTOR *growth, Float size)
{
    struct task *task = task_create(TASK_WORK, 6, ripple_init);
    struct task_work *work = task->work;

    work->pos = *pos;
    RIPPLE(work)->growth = growth->x;
    RIPPLE(work)->fade_size = growth->z;
    work->scl.x = size;
    work->scl.y = 1.0F;
    RIPPLE(work)->fade = -0.5F;
}

static void ripple_init(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = RIPPLE_GROW;
    task->step = ripple_step;
    task->draw = ripple_draw;
    task->destructor = ripple_destroy;
}

static void ripple_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case RIPPLE_GROW: {
        Float size = work->scl.x + RIPPLE(work)->growth;

        if (size > RIPPLE(work)->fade_size) {
            work->mode = RIPPLE_FADE;
            RIPPLE(work)->fade = -0.5F;
        }
        work->scl.x = size;
        break;
    }
    case RIPPLE_FADE: {
        Float fade = RIPPLE(work)->fade;

        work->scl.x += RIPPLE(work)->growth;
        fade -= 0.02F;
        if (fade < -1.0F) {
            work->mode = RIPPLE_END;
        }
        RIPPLE(work)->fade = fade;
        break;
    }
    case RIPPLE_END:
        task_kill(task);
        break;
    }
#ifdef GCC
    /* the second picture's pass draws the ripple by ripple_draw. step 1 of
     * a split frame draws none, so that the picture adds the ripple once */
    if (frame_split && frame_step == 1) {
        return;
    }
#endif
    ripple_draw(task);
}

static void ripple_draw(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack type: the fade in a color on the stack, its alpha unset, which
     * gives the function a 20-byte frame (ADD #-20,R15 at 0x8c0499c6); as a
     * Float, it takes FR14, the frame is 4 bytes and 81 of 85 words differ */
    NJS_ARGB color;

    texture_set_regular();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    color.r = color.g = color.b = RIPPLE(work)->fade;
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    draw_set_material(0.0F, color.r, color.g, color.b);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njScale(NULL, work->scl.x, 1.0F, work->scl.x);
    njDrawObject(&ripple_object);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    draw_attr_restore();
    njPopMatrix(1);
}

static void ripple_destroy(struct task *task)
{
}
