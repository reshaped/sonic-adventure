/* the light and smoke of Zero's fight. the glow, the zaps, the sparkles,
 * the chains and the sparks draw with erobo_textures. the glow's object
 * lies in the model bank */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_egm2/blast.h"
#include "b_robo/banks.h"
#include "b_robo/effects.h"
#include "b_robo/explosion.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/beam.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "effect/hit.h"
#include "effect/sprite.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the glow's object, as an offset into the model bank. its GLOW_TEXTURES
 * textures in erobo_textures start at GLOW_TEXTURE, and the glow shows each
 * for a step */
#define GLOW_OBJECT 0xf50
#define GLOW_TEXTURE 17
#define GLOW_TEXTURES 3

/* the alpha EROBO_GLOW_IN raises the glow to, and the steps it takes from 0 */
#define GLOW_IN_ALPHA 0.6F
#define GLOW_IN_STEPS 30.0F
/* how far EROBO_GLOW_BRIGHT raises the glow's alpha a step, and how far
 * EROBO_GLOW_OUT lowers it */
#define GLOW_BRIGHTEN 0.1F
#define GLOW_FADE 0.04F

/* how far EROBO_SPARKLES_IN raises the sparkles' scale a step, and how far
 * EROBO_SPARKLES_OUT lowers it */
#define SPARKLES_GROW 0.04F
#define SPARKLES_SHRINK 0.02F

/* the frames a zap lasts, each with a texture of its own */
#define ZAP_FRAMES 8
/* the chain's textures, which hold two looks each, one in each half */
#define CHAIN_TEXTURES 6

/* the most a zap's second end turns round from its first, in degrees.
 * ZAP_RISE is the most the second lies above or below the first */
#define ZAP_TURN 30.0F
#define ZAP_RISE 7.0F

/* the least and the most height effect_height adds to a point */
#define HEIGHT_LOW (-8.0F)
#define HEIGHT_HIGH 10.0F

/* the steps Zero's last explosion lasts */
#define EXPLOSION_STEPS 330

/* the glow's record, at its task's extra */
struct glow {
    NJS_OBJECT *object;
    NJS_POINT3 *pos;
    int texture; /* which of its GLOW_TEXTURES textures it shows, from 0 */
    enum erobo_glow_mode mode;
    Float alpha;
};

/* a zap's record, at its task's extra */
struct zap {
    NJS_POINT3 ends[2];
    Uint32 frame; /* the frames it has lasted, and its zap_frames index */
};

/* the sparkles' record, at their task's extra */
struct sparkles {
    enum erobo_sparkles_mode mode;
    struct sprite sprite;
    NJS_POINT3 *points; /* the two points they draw at, kept, not copied */
    Float scale;
};

/* a texture by its number in erobo_textures. index is Ninja's global
 * texture index for it, once zap_bind or chain_bind sets it */
struct texture_frame {
    Uint32 texture;
    Uint32 index;
};

/* a zap's width at its start and its end, as beam_draw_line_textured takes
 * them */
static const Float zap_width[2] = {3.0F, 3.0F};

/* the glow's task and the sparkles', or NULL while there is none */
static struct task *glow_task = NULL;
static struct task *sparkles_task = NULL;
/* each beam lasts 20 steps before it fades. it runs from opaque white at
 * its foot to clear at its tip */
static struct beam_params beams = {
    20, {0.3F, 1.0F}, {0xffffffff, 0x00ffffff}, 0x5b, 0.0F, 1.3F,
};
/* the params every puff of erobo_dust_create keeps. each call changes only
 * pos, speed and scale, which a puff copies as it starts */
static struct dust_params dust = {
    0.7F,
    0.03F,
    0.19F,
    0.05F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 0.56F, 0.56F, 0.7F},
};
/* a zap's textures, one for each frame it lasts */
static struct texture_frame zap_frames[ZAP_FRAMES] = {
#include "b_robo/effects/zap_frames.inc"
};
/* a zap's quad. zap_draw sets its texture for the zap's frame */
static struct beam_texture zap_texture = {
#include "b_robo/effects/zap_texture.inc"
};
/* the sparkles' frames: eight textures of erobo_textures, two steps apart,
 * and a SPRITE_LOOP entry to play them again */
static struct sprite_frame sparkles_frames[9] = {
#include "b_robo/effects/sparkles_frames.inc"
};
/* a chain link's width at its start and its end, at a scale of 1 */
static Float chain_width_from = 3.0F;
static Float chain_width_to = 3.0F;
/* the chain's textures in erobo_textures */
static struct texture_frame chain_frames[CHAIN_TEXTURES] = {
#include "b_robo/effects/chain_frames.inc"
};
/* a link's quad over v from 0 to 0.5 of its texture, and over 0.5 to 1.
 * erobo_chain_draw sets the texture for the look */
static struct beam_texture chain_halves[2] = {
#include "b_robo/effects/chain_halves.inc"
};
/* the sparks' frames: six textures of erobo_textures, two steps apart, and
 * a SPRITE_LOOP entry to play them again */
static struct sprite_frame spark_frames[7] = {
#include "b_robo/effects/spark_frames.inc"
};
static struct sprite_desc spark = {
    NULL, {0.0F, 0.0F, 0.0F}, 0.0F, -3.0F, 0.0F, 0.0F, spark_frames, 1.0F,
};

static void glow_draw(struct task *task);
static void zap_draw(struct task *task);
static void sparkles_draw(struct task *task);

/* set out to pos raised by a random height from HEIGHT_LOW to HEIGHT_HIGH.
 * returns that height */
static Float effect_height(NJS_POINT3 *pos, NJS_POINT3 *out)
{
    Float height = RANDOM(HEIGHT_LOW, HEIGHT_HIGH);

    out->x = pos->x;
    out->y = pos->y + height;
    out->z = pos->z;
    return height;
}

int erobo_explosion_start(NJS_POINT3 *pos)
{
    struct blast_params params;

    params.pos = pos;
    params.flags = 0;
    params.beam_start = 0;
    params.beam_end = 300;
    params.beam_length = 130.0F;
    params.beam_speed = 0x5b;
    params.grow_start = 200;
    params.grow_end = 250;
    params.fade_start = 300;
    params.end = EXPLOSION_STEPS;
    params.size = 30.0F;
    erobo_explosion_create(&params);
    return EXPLOSION_STEPS;
}

void erobo_beams_create(NJS_POINT3 *pos, Float length, Angle speed)
{
    if (length > 0.0F) {
        beams.length = length;
        beams.max_speed = speed;
        beam_create(pos, &beams);
    }
}

void erobo_dust_create(NJS_POINT3 *pos, Float scale)
{
    NJS_POINT3 offset;

    offset.x = RANDOM(-1.0F, 1.0F);
    offset.z = RANDOM(-1.0F, 1.0F);
    offset.y = 0.0F;
    VECTOR_SUM(&dust.pos, pos, &offset);
    dust.speed.x = RANDOM(-1.0F, 1.0F);
    dust.speed.y = RANDOM(-1.0F, 1.0F);
    dust.speed.z = RANDOM(-1.0F, 1.0F);
    dust.scale = scale;
    dust_create_from(&dust);
}

void erobo_lightning_create(NJS_POINT3 *pos, Float range)
{
    NJS_POINT3 at;

    effect_height(pos, &at);
    /* @hack spelling: the offsets' span as range + range, which SHC adds
     * once and keeps on the stack for z (FADD at 0x0c9057d4, FMOV.S at
     * 0x0c9057da); as 2.0F * range, it folds the 2 into RAND_SCALE, and 46
     * of 49 words differ */
    at.x += (rand() * (range + range) * RAND_SCALE) - range;
    at.z += (rand() * (range + range) * RAND_SCALE) - range;
    lightning_create(&at, 1.0F);
}

void erobo_bomb_create(NJS_POINT3 *pos, Float range, Float scale)
{
    NJS_POINT3 at;

    effect_height(pos, &at);
    /* @hack spelling: the offsets' span as range + range, which SHC adds
     * once and keeps on the stack for z (FADD at 0x0c905838, FMOV.S at
     * 0x0c90583e); as 2.0F * range, it folds the 2 into RAND_SCALE, and 42
     * of 52 words differ */
    at.x += (rand() * (range + range) * RAND_SCALE) - range;
    at.z += (rand() * (range + range) * RAND_SCALE) - range;
    bomb_create(&at, scale);
}

void erobo_glow_set_mode(enum erobo_glow_mode mode)
{
    struct glow *glow;

    if (glow_task != NULL) {
        glow = glow_task->extra;
        glow->mode = mode;
    }
}

/* on a frame's first step, draw the glow in white at its alpha, unless the
 * alpha is 0 */
static void glow_draw(struct task *task)
{
    struct glow *glow = task->extra;

    if (frame_step == 0 && glow->alpha != 0.0F) {
        draw_control_save();
        njControl3D(NJD_CONTROL_3D_CONSTANT_MATERIAL);
        draw_set_material(glow->alpha, 1.0F, 1.0F, 1.0F);
        njPushMatrix(NULL);
        njTranslateV(NULL, glow->pos);
        njSetTexture(erobo_textures);
        draw_tree(glow->object);
        njPopMatrix(1);
        draw_control_restore();
    }
}

/* turn the glow to its next texture, move its alpha as its mode says, and
 * draw it. the texture goes in the glow's material in the model bank */
static void glow_step(struct task *task)
{
    struct glow *glow = task->extra;

    glow->texture = (glow->texture + 1) % GLOW_TEXTURES;
    glow->object->model->mats->attr_texId = glow->texture + GLOW_TEXTURE;
    switch (glow->mode) {
    case EROBO_GLOW_STILL:
        break;
    case EROBO_GLOW_IN:
        glow->alpha += GLOW_IN_ALPHA / GLOW_IN_STEPS;
        if (glow->alpha >= GLOW_IN_ALPHA) {
            glow->alpha = GLOW_IN_ALPHA;
        }
        break;
    case EROBO_GLOW_BRIGHT:
        glow->alpha += GLOW_BRIGHTEN;
        if (glow->alpha >= 1.0F) {
            glow->alpha = 1.0F;
        }
        break;
    case EROBO_GLOW_OUT:
        glow->alpha -= GLOW_FADE;
        if (glow->alpha <= 0.0F) {
            glow->alpha = 0.0F;
            glow->mode = EROBO_GLOW_STILL;
        }
        break;
    }
    glow_draw(task);
}

/* make the glow's task, which follows pos, unless the glow is there
 * already */
static void glow_create(NJS_POINT3 *pos)
{
    struct task *task;
    struct glow *glow;

    if (glow_task == NULL) {
        if ((task = task_create(TASK_WORK, 4, glow_step)) != NULL) {
            glow_task = task;
            glow = heap_alloc(sizeof(struct glow));
            task->extra = glow;
            glow->object = (NJS_OBJECT *)(erobo_models + GLOW_OBJECT);
            glow->pos = pos;
            glow->texture = 0;
            glow->mode = EROBO_GLOW_STILL;
            glow->alpha = 0.0F;
            task->draw = glow_draw;
        }
    }
}

/* on a frame's first step, draw the zap with its frame's texture. it adds
 * its light to what lies behind */
static void zap_draw(struct task *task)
{
    struct zap *zap = task->extra;

    if (frame_step != 0) {
        return;
    }
    zap_texture.texture = zap_frames[zap->frame].index;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    beam_draw_line_textured(&zap->ends[0], &zap->ends[1], (Float *)zap_width,
                            &zap_texture);
}

/* on a frame's first step, move the zap on a frame and draw it. once it
 * has lasted ZAP_FRAMES, kill it instead */
static void zap_step(struct task *task)
{
    struct zap *zap = task->extra;

    if (frame_step != 0) {
        return;
    }
    if (++zap->frame >= ZAP_FRAMES) {
        task_kill(task);
    } else {
        zap_draw(task);
    }
}

void erobo_zap_create(NJS_POINT3 *pos, Float radius)
{
    struct task *task;
    struct zap *zap;
    Float sine;
    Float cosine;
    NJS_POINT3 offset;
    NJS_POINT3 center;
    Angle ang;

    if ((task = task_create(TASK_WORK, 4, zap_step)) == NULL) {
        return;
    }
    zap = heap_alloc(sizeof(struct zap));
    task->extra = zap;
    zap->frame = 0;
    effect_height(pos, &center);
    ang = (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
    _builtin_fsca(ang, &sine, &cosine);
    offset.x = radius * cosine;
    offset.z = radius * sine;
    offset.y = 0.0F;
    VECTOR_SUM(&zap->ends[0], &center, &offset);
    ang += (Angle)(RANDOM(-ZAP_TURN, ZAP_TURN) * ANGLE_PER_DEGREE);
    _builtin_fsca(ang, &sine, &cosine);
    offset.x = radius * cosine;
    offset.z = radius * sine;
    offset.y = RANDOM(-ZAP_RISE, ZAP_RISE);
    VECTOR_SUM(&zap->ends[1], &center, &offset);
    task->draw = zap_draw;
}

/* set each zap texture's index, Ninja's global texture index */
static void zap_bind(void)
{
    /* @hack type: i unsigned, which SHC tests with CMP/HS (0x0c905ca4); as
     * an int, it tests with CMP/GE and 1 of 33 words differ */
    unsigned int i;

    for (i = 0; i < ZAP_FRAMES; i++) {
        zap_frames[i].index =
            ((NJS_TEXMEMLIST *)erobo_textures->textures[zap_frames[i].texture]
                 .texaddr)
                ->globalIndex;
    }
}

void erobo_sparkles_set_mode(enum erobo_sparkles_mode mode)
{
    struct sparkles *sparkles;

    if (sparkles_task != NULL) {
        sparkles = sparkles_task->extra;
        sparkles->mode = mode;
    }
}

void erobo_sparkles_set_points(NJS_POINT3 *points)
{
    struct sparkles *sparkles;

    if (sparkles_task != NULL) {
        sparkles = sparkles_task->extra;
        sparkles->points = points;
    }
}

/* on a frame's first step, draw a sparkle at each of the two points.
 * hidden sparkles draw nothing */
static void sparkles_draw(struct task *task)
{
    struct sparkles *sparkles = task->extra;

    if (frame_step != 0) {
        return;
    }
    if (sparkles->mode >= EROBO_SPARKLES_IN) {
        sparkles->sprite.body.scale = sparkles->scale;
        sprite_draw_at(&sparkles->sprite, sparkles->points);
        sprite_draw_at(&sparkles->sprite, sparkles->points + 1);
    }
}

/* play the sparkles on a step, move their scale as their mode says, and
 * draw them */
static void sparkles_step(struct task *task)
{
    struct sparkles *sparkles = task->extra;

    sprite_step(&sparkles->sprite);
    switch (sparkles->mode) {
    case EROBO_SPARKLES_HIDDEN:
        break;
    case EROBO_SPARKLES_IN:
        sparkles->scale += SPARKLES_GROW;
        if (sparkles->scale >= 1.0F) {
            sparkles->scale = 1.0F;
        }
        break;
    case EROBO_SPARKLES_OUT:
        sparkles->scale -= SPARKLES_SHRINK;
        if (sparkles->scale <= 0.0F) {
            sparkles->scale = 0.0F;
            sparkles->mode = EROBO_SPARKLES_HIDDEN;
        }
        break;
    }
    sparkles_draw(task);
}

/* set sprite up to play the sparkles' frames, standing still */
static void sparkles_set(struct sprite *sprite)
{
    struct sprite_desc desc;
    NJS_POINT3 pos;

    desc.step = NULL;
    desc.speed.x = desc.speed.y = desc.speed.z = 0.0F;
    desc.scale = 1.0F;
    desc.depth = -6.0F;
    desc.drag = 0.0F;
    desc.gravity = 0.0F;
    desc.frames = sparkles_frames;
    desc.rate = 1.0F;
    /* @bug uninitialized: no path writes pos, which sprite_set copies into
     * the sprite's body. no draw reads the copy, since the sparkles draw only
     * at their points. the call is the JSR at 0x0c905e00, with pos at R15 */
    /* NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage) */
    sprite_set(sprite, &pos, &desc, SPRITE_MOVES);
}

/* unless the sparkles' task is there already, bind their textures, then
 * make the task. the sparkles start hidden, at a scale of 0, with no
 * points */
static void sparkles_create(void)
{
    struct task *task;
    struct sparkles *sparkles;

    if (sparkles_task != NULL) {
        return;
    }
    sprite_bind(erobo_textures, sparkles_frames, 0);
    if ((task = task_create(TASK_WORK, 4, sparkles_step)) != NULL) {
        sparkles_task = task;
        sparkles = heap_alloc(sizeof(struct sparkles));
        task->extra = sparkles;
        sparkles_set(&sparkles->sprite);
        sparkles->mode = EROBO_SPARKLES_HIDDEN;
        sparkles->points = NULL;
        sparkles->scale = 0.0F;
        task->draw = sparkles_draw;
    }
}

void erobo_chain_draw(NJS_POINT3 *from, NJS_POINT3 *to, int frame, int count,
                      Float scale)
{
    Float width[2];
    NJS_POINT3 at;
    NJS_POINT3 next;
    NJS_VECTOR step;
    struct beam_texture *texture;
    Float share;
    int i;

    width[0] = chain_width_from * scale;
    width[1] = chain_width_to * scale;
    frame %= CHAIN_TEXTURES * 2;
    texture = &chain_halves[frame % 2];
    texture->texture = chain_frames[frame >> 1].index;
    VECTOR_COPY(&at, from);
    VECTOR_DIFF(&step, to, from);
    share = 1.0F / (Float)count;
    VECTOR_SCALE(&step, &step, share);
    /* @hack spelling: a do-while inside an if in place of a for, which loads
     * beam_draw_line_textured's address into R11 (0x0c905f7a) after the
     * test at 0x0c905f78; as a for loop counting i down, SHC hoists the load
     * above the test and 4 of 160 words differ */
    i = count;
    if (i > 0) {
        do {
            VECTOR_SUM(&next, &at, &step);
            beam_draw_line_textured(&at, &next, width, texture);
            VECTOR_COPY(&at, &next);
        } while (--i > 0);
    }
}

/* set each chain texture's index, Ninja's global texture index */
static void chain_bind(void)
{
    int i;

    for (i = 0; i < CHAIN_TEXTURES; i++) {
        chain_frames[i].index =
            ((NJS_TEXMEMLIST *)erobo_textures->textures[chain_frames[i].texture]
                 .texaddr)
                ->globalIndex;
    }
}

void erobo_spark_set(struct sprite *sprite, NJS_POINT3 *pos, Float scale,
                     int flags)
{
    spark.scale = scale;
    sprite_set(sprite, pos, &spark, flags);
}

void erobo_effects_start(struct task *task)
{
    struct task_work *work = task->work;

    glow_create(&work->pos);
    zap_bind();
    sparkles_create();
    /* @hack spelling: chain_bind and sprite_bind as one statement, which calls
     * sprite_bind by JSR and returns after it (0x0c90608c); as two
     * statements, SHC jumps to sprite_bind and 7 of 23 words differ */
    chain_bind(), sprite_bind(erobo_textures, spark_frames, 0);
}

void erobo_effects_end(void)
{
    /* @hack spelling: an empty then-arm, which loads task_kill's address into
     * R3 (0x0c9060a2) and the NULL it stores into R2 (0x0c9060aa); as a plain
     * if, R1 and R3 take them and 5 of 24 words differ */
    if (glow_task == NULL) {
    } else {
        task_kill(glow_task);
        glow_task = NULL;
    }
    /* @hack spelling: an empty then-arm, which loads task_kill's address into
     * R3 (0x0c9060b6) and the NULL it stores into R2 (0x0c9060be); as a plain
     * if, R2 and R3 take them and 5 of 24 words differ */
    if (sparkles_task == NULL) {
    } else {
        task_kill(sparkles_task);
        sparkles_task = NULL;
    }
}
