/* E-101 mkII's effects. lightning, puffs, beams of light and blasts are
 * shared effects, made at the boss's points. its jets' flames and its sparks
 * draw in its own textures, from e101_r_texlist */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_e101_r/effects.h"
#include "b_e101_r/files.h"
#include "effect/beam.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "effect/hit.h"
#include "effect/sprite.h"
#include "effect/sprite_pool.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the step a lightning task ends on */
#define LIGHTNING_STEPS 60

/* the most sparks each pool holds */
#define BEAM_SPARKS 100
#define SWAT_SPARKS 20
/* the sparks a swat bursts into, and the most steps into its frames each
 * starts */
#define SWAT_BURST 15
#define SPARK_HEAD_START 15.0F

/* the flames' textures, which they turn through, one a step */
#define JET_TEXTURES 4

/* a texture as a frame of the flames: its number in e101_r_texlist, and its
 * global index. e101_mk2_jets_create fills the index in */
struct texture_frame {
    Uint32 texture;
    Uint32 index;
};

/* the jets' record, their task's extra */
struct jets {
    NJS_POINT3 *pos[2]; /* where each flame starts */
    NJS_VECTOR *dirs[2]; /* each flame's way, and its length at a scale of 1 */
    /* a request, JETS_FLARE or JETS_SETTLE, until the next step takes it up.
     * then JETS_DYING or JETS_FLARING */
    int state;
    struct sprite glow; /* drawn at each flame's start */
    Float glow_scale;
    int frame; /* the index of the flames' texture in jet_frames */
    /* the flames' length in dirs, and 4 times their width. under 0.2 they
     * aren't drawn */
    Float scale;
};

/* the states a step leaves the flames in: dying down, or flaring up. dying
 * shrinks them by a fifth each step, and flaring grows them by half. the task
 * starts dying, with no flames */
#define JETS_DYING 0
#define JETS_FLARING 3

/* the point a lightning task flashes about, in its work's vars. its work's
 * timer counts the steps left */
#define LIGHTNING_TARGET(work) ((NJS_POINT3 *)(work)->vars.p[2])

static struct task *jets_task = NULL;
/* @hack type: volatile only so that, for each member
 * e101_mk2_thrust_puff stores to, SHC loads the member's address from a pool
 * word of its own (0x0c904c6e to 0x0c904cb0); without it, 44 of 130 words
 * differ */
static volatile struct dust_params thrust_dust = {
    0.7F,
    0.03F,
    0.19F,
    0.05F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 0.56F, 0.56F, 0.7F},
};
/* a light burst's beam: 20 steps before it fades, white at its foot and
 * clear at its tip. e101_mk2_light_burst sets its length and swing */
static struct beam_params burst_beam = {
    20, {0.3F, 2.0F}, {0xffffffff, 0x00ffffff}, 0x5b, 0.0F, 1.3F,
};
/* the beam's sparks: white, turning red, over 20 steps */
static struct sprite_frame beam_spark_frames[5] = {
#include "b_e101_r/effects/beam_spark_frames.inc"
};
/* the glow at the jets' starts, pulsing in size over and over */
static struct sprite_frame jet_glow_frames[9] = {
#include "b_e101_r/effects/jet_glow_frames.inc"
};
static struct texture_frame jet_frames[JET_TEXTURES] = {
#include "b_e101_r/effects/jet_frames.inc"
};
static struct beam_texture jet_texture = {
#include "b_e101_r/effects/jet_texture.inc"
};
/* the swat's sparks: cyan, turning violet, over 40 steps */
static struct sprite_frame swat_spark_frames[5] = {
#include "b_e101_r/effects/swat_spark_frames.inc"
};
/* the swat's flash, through four textures in 22 steps */
static struct sprite_frame swat_flash_frames[10] = {
#include "b_e101_r/effects/swat_flash_frames.inc"
};

/* the pools' tasks, and the point the beam's sparks close on */
static struct task *beam_spark_pool;
static NJS_POINT3 beam_spark_target;
static struct task *swat_spark_pool;

static void lightning_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 at;

    /* @hack spelling: <= 0, which for the unsigned timer is == 0, keeps
     * CMP/PL R0 at 0x0c904a34; as == 0, 2 of 80 words differ */
    if (--TASK_TIMER(work) <= 0) {
        task_kill(task);
        return;
    }
    if (frame_step == 0) {
        VECTOR_COPY(&at, LIGHTNING_TARGET(work));
        at.x += RANDOM(-9.0F, 9.0F);
        at.y += RANDOM(-9.0F, 9.0F);
        at.z += RANDOM(-9.0F, 9.0F);
        lightning_create(&at, 1.0F);
    }
}

void e101_mk2_lightning_create(NJS_POINT3 *pos)
{
    struct task *task;
    struct task_work *work;

    if ((task = task_create(TASK_WORK, 4, lightning_step)) != NULL) {
        work = task->work;
        work->timer = LIGHTNING_STEPS;
        work->vars.p[2] = pos;
    }
}

void e101_mk2_lightning_near(NJS_POINT3 *pos, Float range)
{
    NJS_POINT3 at;

    VECTOR_COPY(&at, pos);
    at.x += RANDOM(-range, range);
    at.y += RANDOM(-range, range);
    at.z += RANDOM(-range, range);
    lightning_create(&at, 1.0F);
}

void e101_mk2_thrust_puff(NJS_POINT3 *pos, NJS_VECTOR *dir)
{
    NJS_POINT3 at;
    NJS_VECTOR speed;

    VECTOR_COPY(&at, pos);
    at.x += RANDOM(-1.0F, 1.0F);
    at.z += RANDOM(-1.0F, 1.0F);
    VECTOR_COPY(&speed, dir);
    speed.x += RANDOM(-1.0F, 1.0F);
    speed.y += RANDOM(-1.0F, 1.0F);
    speed.z += RANDOM(-1.0F, 1.0F);
    /* @hack spelling: at and speed read through their addresses, which SHC
     * forms from R15 for each read, as MOV R15,R3 and ADD #12,R3 do at
     * 0x0c904c7e; as plain member stores, 46 of 130 words differ */
    (&thrust_dust.pos)->x = (&at)->x;
    (&thrust_dust.pos)->y = (&at)->y;
    (&thrust_dust.pos)->z = (&at)->z;
    (&thrust_dust.speed)->x = (&speed)->x;
    (&thrust_dust.speed)->y = (&speed)->y;
    (&thrust_dust.speed)->z = (&speed)->z;
    dust_create_glow((struct dust_params *)&thrust_dust);
}

void e101_mk2_light_burst(NJS_POINT3 *pos, Float length)
{
    burst_beam.length = length;
    burst_beam.max_speed = 54;
    beam_create(pos, &burst_beam);
}

void e101_mk2_bomb_near(NJS_POINT3 *pos, Float range)
{
    NJS_POINT3 at;

    VECTOR_COPY(&at, pos);
    /* @hack spelling: each draw multiplied by range first, then by 2 *
     * RAND_SCALE, which SHC keeps in FR14 (FMUL FR15,FR3 and FMUL FR14,FR3 at
     * 0x0c904d02 and 0x0c904d04). 2 * RAND_SCALE is a power of 2, and a product
     * by one is exact, so this rounds as RANDOM(-range, range) does; as that,
     * 65 of 68 words differ */
    at.x += (rand() * range * (2.0F * RAND_SCALE)) - range;
    at.y += (rand() * range * (2.0F * RAND_SCALE)) - range;
    at.z += (rand() * range * (2.0F * RAND_SCALE)) - range;
    /* a scale from 0.3 up to 0.7. RANDOM(0.3F, 0.7F) would fold 0.7F - 0.3F
     * to the float an ulp below 0.4F */
    bomb_create(&at, (rand() * (0.4F * RAND_SCALE)) + 0.3F);
}

/* a beam spark's step, which the pool runs in place of its own. the spark
 * closes 3% of its distance to beam_spark_target, and its speed and the
 * pool's motion go unread */
static Bool beam_spark_step(struct sprite_slot *slot,
                            struct sprite_motion *motion,
                            const struct sprite_frame *frames)
{
    NJS_VECTOR step;

    VECTOR_DIFF(&step, &beam_spark_target, &slot->body.pos);
    VECTOR_SCALE(&step, &step, 0.03F);
    VECTOR_SUM(&slot->body.pos, &slot->body.pos, &step);
    if (sprite_play_step(&slot->play, frames) == SPRITE_END) {
        return FALSE;
    }
    return TRUE;
}

void e101_mk2_beam_sparks_init(void)
{
    struct sprite_motion motion;

    sprite_bind(e101_r_texlist, beam_spark_frames, 0);
    motion.drag = 0.03F;
    motion.gravity = 0.0F;
    beam_spark_pool = sprite_pool_create(BEAM_SPARKS, &motion,
                                         beam_spark_frames, beam_spark_step);
}

void e101_mk2_beam_sparks_clear(void)
{
    if (beam_spark_pool != NULL) {
        task_kill(beam_spark_pool);
    }
}

void e101_mk2_beam_sparks(NJS_POINT3 *pos)
{
    NJS_VECTOR speed;
    NJS_POINT3 at;
    int i;

    VECTOR_COPY(&beam_spark_target, pos);
    for (i = 0; i < 3; i++) {
        /* @hack redundant: a do-while of one pass, which keeps the loop round
         * it whole, as the image's BF at 0x0c904fd0 closes it; without it,
         * SHC unrolls the loop's three passes, and 141 of 141 words differ */
        do {
            VECTOR_COPY(&at, pos);
            speed.x = RANDOM(-20.0F, 20.0F);
            speed.y = RANDOM(-20.0F, 20.0F);
            speed.z = RANDOM(-20.0F, 20.0F);
            VECTOR_DIFF(&at, &at, &speed);
            VECTOR_SCALE(&speed, &speed, 0.01F);
            sprite_pool_add(beam_spark_pool, &at, &speed, 0.0F, 0.0F);
        } while (0);
    }
}

/* set up the glow's sprite, which jets_draw draws only at the jets' starts */
static void jet_glow_set(struct sprite *sprite)
{
    struct sprite_desc desc;
    NJS_POINT3 pos;

    desc.step = NULL;
    desc.speed.x = desc.speed.y = desc.speed.z = 0.0F;
    desc.scale = 1.0F;
    desc.depth = -3.0F;
    desc.drag = 0.0F;
    desc.gravity = 0.0F;
    desc.frames = jet_glow_frames;
    desc.rate = 1.0F;
    /* @bug uninitialized: no code writes pos, and sprite_set copies it as
     * the sprite's start, from the frame address MOV R15,R5 passes at
     * 0x0c905028. the start goes unused: jets_draw places the sprite itself */
    /* NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage) */
    sprite_set(sprite, &pos, &desc, SPRITE_MOVES);
}

static void jets_draw(struct task *task)
{
    struct jets *jets = task->extra;
    Float width[2];
    NJS_POINT3 end;
    int i;

    jets->glow.body.scale = jets->glow_scale;
    sprite_draw_at(&jets->glow, jets->pos[0]);
    sprite_draw_at(&jets->glow, jets->pos[1]);
    if (jets->scale >= 0.2F) {
        jet_texture.texture = jet_frames[jets->frame].index;
        width[0] = jets->scale * 0.25F;
        width[1] = jets->scale * 0.25F;
        for (i = 0; i < 2; i++) {
            VECTOR_SCALE(&end, jets->dirs[i], jets->scale);
            /* @hack redundant: a do-while of one pass, which keeps the loop
             * round it whole, as the image's BF/S at 0x0c9051aa closes it;
             * without it, SHC unrolls the loop's two passes, and 118 of 145
             * words differ */
            do {
                VECTOR_SUM(&end, &end, jets->pos[i]);
            } while (0);
            njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
            njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
            beam_draw_line_textured(jets->pos[i], &end, width, &jet_texture);
        }
    }
}

static void jets_step(struct task *task)
{
    struct jets *jets = task->extra;

    jets_draw(task);
    sprite_step(&jets->glow);
    jets->frame = (jets->frame + 1) % JET_TEXTURES;
    switch (jets->state) {
    case JETS_DYING:
        jets->glow_scale = 1.0F;
        jets->scale *= 0.8F;
        break;
    case JETS_FLARE:
        jets->glow_scale = 5.0F;
        jets->scale = 1.0F;
        jets->state = JETS_FLARING;
        break;
    case JETS_SETTLE:
        jets->state = JETS_DYING;
        break;
    case JETS_FLARING:
        jets->glow_scale *= 0.8F;
        if (jets->glow_scale <= 1.0F) {
            jets->glow_scale = 1.0F;
        }
        jets->scale *= 1.5F;
        if (jets->scale >= 30.0F) {
            jets->scale = 30.0F;
        }
        break;
    }
}

void e101_mk2_jets_create(NJS_POINT3 *pos, NJS_VECTOR *dirs)
{
    struct task *task;
    struct jets *jets;
    int i;

    if (jets_task == NULL) {
        sprite_bind(e101_r_texlist, jet_glow_frames, 0);
        for (i = 0; i < JET_TEXTURES; i++) {
            jet_frames[i].index = ((NJS_TEXMEMLIST *)e101_r_texlist
                                       ->textures[jet_frames[i].texture]
                                       .texaddr)
                                      ->globalIndex;
        }
        if ((task = task_create(TASK_WORK, 4, jets_step)) != NULL) {
            jets = heap_alloc(sizeof(struct jets));
            if (jets != NULL) {
                task->extra = jets;
                jets_task = task;
                jets->state = JETS_DYING;
                jets->pos[0] = pos;
                jets->pos[1] = pos + 1;
                jet_glow_set(&jets->glow);
                jets->glow_scale = 1.0F;
                jets->dirs[0] = dirs;
                jets->dirs[1] = dirs + 1;
                jets->frame = 0;
                jets->scale = 0.0F;
                task->draw = jets_draw;
                return;
            } else {
                task_destroy(task);
            }
        }
        jets_task = NULL;
    }
}

void e101_mk2_jets_destroy(void)
{
    if (jets_task != NULL) {
        task_kill(jets_task);
        jets_task = NULL;
    }
}

void e101_mk2_jets_set_state(int state)
{
    struct jets *jets;

    if (jets_task != NULL) {
        jets = jets_task->extra;
        jets->state = state;
    }
}

static void swat_sparks_create(void)
{
    struct sprite_motion motion;

    sprite_bind(e101_r_texlist, swat_spark_frames, 0);
    motion.drag = 0.03F;
    motion.gravity = 0.0F;
    swat_spark_pool =
        sprite_pool_create(SWAT_SPARKS, &motion, swat_spark_frames, NULL);
}

/* global, though only this file calls it. SHC inlines its one call, in
 * e101_mk2_swat_sparks_clear, and writes its body at 0x0c9053ca only for a
 * global */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
void e101_mk2_swat_sparks_kill(void)
{
    if (swat_spark_pool != NULL) {
        task_kill(swat_spark_pool);
    }
}

/* SWAT_BURST sparks from pos, each starting up to SPARK_HEAD_START steps into
 * its frames */
static void swat_sparks_burst(NJS_POINT3 *pos)
{
    NJS_VECTOR speed;
    int i;

    for (i = 0; i < SWAT_BURST; i++) {
        speed.x = RANDOM(-2.0F, 2.0F);
        speed.y = RANDOM(-2.0F, 2.0F);
        speed.z = RANDOM(-2.0F, 2.0F);
        sprite_pool_add(swat_spark_pool, pos, &speed,
                        rand() * RAND_SCALE * SPARK_HEAD_START, -15.0F);
    }
}

void e101_mk2_swat_sparks_init(void)
{
    sprite_bind(e101_r_texlist, swat_flash_frames, 0);
    swat_sparks_create();
}

void e101_mk2_swat_sparks_clear(void)
{
    e101_mk2_swat_sparks_kill();
}

void e101_mk2_swat_sparks(NJS_POINT3 *pos)
{
    struct sprite_desc desc;

    desc.step = NULL;
    desc.speed.x = desc.speed.y = desc.speed.z = 0.0F;
    desc.scale = 1.0F;
    desc.depth = -15.0F;
    desc.drag = 0.0F;
    desc.gravity = 0.0F;
    desc.frames = swat_flash_frames;
    desc.rate = 1.0F;
    sprite_create(pos, &desc, SPRITE_MOVES);
    swat_sparks_burst(pos);
}
