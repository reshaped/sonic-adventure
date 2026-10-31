/* the Chaos fights' sparks, flashes, trails, drops, spots and pulses, his
 * cyan light, and alpha fades */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "chaos/effect.h"
#include "chaos/models.h"
#include "draw/motion.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 14
#define TEXT_2_LENGTH 14
#define TEXT_3_LENGTH 14
#define TEXT_4_LENGTH 14
#define TEXT_5_LENGTH 14
#define TEXT_6_LENGTH 14
#define TEXT_7_LENGTH 14
#define TEXT_8_LENGTH 14
#define TEXT_9_LENGTH 14
#define TEXT_10_LENGTH 14
#define TEXT_11_LENGTH 14
#define TEXT_12_LENGTH 14
#include "chaos/effect/text.inc"

/* the most sparks and trail pieces one step lists to draw */
#define SPARK_MAX 256
#define TRAIL_MAX 128

/* the animation frames of a flash and of a trail piece */
#define FLASH_FRAMES 15.0F
#define TRAIL_FRAMES 10.0F

/* the bytes a spark allocates for its speed, of which it uses 12 */
#define SPEED_BYTES 24

/* how far a spark's fade moves each step: down from 1, or up from 0 while
 * it gathers */
#define SPARK_FADE 0.05F
/* the width of the cube, centered where a spark is made, that it starts
 * in */
#define SPARK_SPREAD 7.0F

/* a random angle, short of a turn */
#define ANY_ANGLE() ((Angle)(rand() * 1.9999999F))

/* a drop falls this much faster each step */
#define DROP_GRAVITY 0.027222222F

/* the work mode of a spark, a drop and a spot: made, then on its way */
enum effect_mode { EFFECT_START, EFFECT_RUN };

/* a spark's vars, which a trail piece shares. the work's scl.x is its
 * size */
struct spark_vars {
    Float frame; /* the sprite's animation frame */
    Float fade; /* how bright it is, 0 to 1 */
};

struct flash_vars {
    Float frame; /* the sprite's animation frame */
    NJS_POINT3 *target; /* the point it follows */
};

/* a drop's vars. the work's ang.x wobbles the drop, and scl holds its
 * width, height and size */
struct drop_vars {
    Uint16 steps; /* the steps it has flown */
    Uint16 limit; /* the steps it flies in all */
};

/* a spot's alpha, which also sets its size and lifts it */
struct spot_vars {
    Float alpha;
};

/* the object a pulse scales. the work's ang.x is the pulse's phase */
struct pulse_vars {
    Uint8 unk_00[4];
    NJS_OBJECT *object;
};

#define SPARK(work) ((struct spark_vars *)&(work)->vars)
#define FLASH(work) ((struct flash_vars *)&(work)->vars)
#define DROP(work) ((struct drop_vars *)&(work)->vars)
#define SPOT(work) ((struct spot_vars *)&(work)->vars)
#define PULSE(work) ((struct pulse_vars *)&(work)->vars)

/* a fade task's extra */
struct fade {
    NJS_OBJECT **objects;
    Float alpha;
    Float end;
    Float change; /* added to alpha each step */
    int steps; /* steps to go */
};

static NJS_TEXANIM chaos_flash_texanims[15] = {
#include "chaos/effect/chaos_flash_texanims.inc"
};
static NJS_TEXANIM chaos_trail_texanims[10] = {
#include "chaos/effect/chaos_trail_texanims.inc"
};
static NJS_TEXANIM chaos_spark_texanim[1] = {
#include "chaos/effect/chaos_spark_texanim.inc"
};
static NJS_TEXANIM chaos_drop_texanim[1] = {
#include "chaos/effect/chaos_drop_texanim.inc"
};
static NJS_TEXANIM chaos_spot_texanim[1] = {
#include "chaos/effect/chaos_spot_texanim.inc"
};

/* C_DEFENS's textures, as one list and a list each. no code reads them */
static NJS_TEXNAME chaos_defens_textures[12] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
};
static NJS_TEXLIST chaos_defens_texlist = {
#include "chaos/effect/chaos_defens_texlist.inc"
};
static NJS_TEXLIST chaos_defens_texlists[12] = {
#include "chaos/effect/chaos_defens_texlists.inc"
};

/* how far out a gathering spark starts, before it's turned */
static NJS_VECTOR chaos_gather_reach = {0.0F, 0.0F, 8.0F};

NJS_SPRITE chaos_spot_sprite;
static NJS_SPRITE chaos_flash_sprite;
static NJS_SPRITE chaos_trail_sprite;
static NJS_SPRITE chaos_spark_sprite;
static NJS_SPRITE chaos_drop_sprite;
/* the sparks and trail pieces to draw this step */
static struct task_work *chaos_spark_list[SPARK_MAX];
static int chaos_spark_count;
static struct task_work *chaos_trail_list[TRAIL_MAX];
static int chaos_trail_count;
/* 8 bytes this file never reads; their type is unknown */
static Sint32 dat_8c7adbf0[2];
static NJS_LIGHT chaos_light;
static Float chaos_flash_scale;
static Float chaos_trail_scale;
static Float chaos_spark_scale;
static Float chaos_spark_speed;

static void chaos_sparks_add(struct task_work *work);

/* @hack spelling: njSubVector through an inline function, which copies by
 * to a frame slot of its own in chaos_gather_step (MOV.L R3,@R15 at
 * 0x8c0fad56); called directly, summed over chaos_gather_step and
 * chaos_effect_trail, effect.c's code runs 32 bytes short and 155 of 157
 * words differ */
#pragma inline(sub_vector)
static void sub_vector(NJS_VECTOR *vector, NJS_VECTOR *by)
{
    njSubVector(vector, by);
}

void chaos_effect_init(void)
{
    sprite_init(&chaos_flash_sprite, &chaos_effect_texlist,
                chaos_flash_texanims);
    sprite_init(&chaos_trail_sprite, &chaos_effect_texlist,
                chaos_trail_texanims);
    sprite_init(&chaos_spark_sprite, &chaos_effect_texlist,
                chaos_spark_texanim);
    sprite_init(&chaos_drop_sprite, &chaos_effect_texlist, chaos_drop_texanim);
    sprite_init(&chaos_spot_sprite, &chaos_effect_texlist, chaos_spot_texanim);
    chaos_spark_count = 0;
    chaos_trail_count = 0;
    chaos_flash_scale = 1.3F;
    chaos_trail_scale = 1.3F;
    chaos_spark_scale = 1.0F;
    chaos_spark_speed = 0.2F;
}

/* a flash plays its frames at the point it follows, then ends */
static void chaos_flash_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;
    Angle ang;
    Float scale;

    /* @hack spelling: the frame advanced inside the test, which compares the
     * sum in FR1 as it stores it (FCMP/GT FR2,FR1 at 0x8c0faa80); advanced
     * before it, SHC reloads the frame, the function runs 4 bytes long and
     * 119 of 136 words differ */
    if ((FLASH(work)->frame += 0.75F) >= FLASH_FRAMES) {
        task_destroy(task);
        return;
    }
    work->pos = *FLASH(work)->target;
    if (!frame_step) {
        color.a = 0.5F;
        color.r = -0.5F;
        color.g = -0.5F;
        color.b = -0.5F;
        njSetConstantMaterial(&color);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njSetTexture(&chaos_effect_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, camera_work->ang.y, ang);
        TURN_VIA(njRotateX, camera_work->ang.x, ang);
        scale = work->scl.x + 0.5F;
        njScale(NULL, scale, scale, scale);
        njDrawSprite3D(&chaos_flash_sprite, (Uint32)FLASH(work)->frame,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        njPopMatrix(1);
        color.a = 1.0F;
        njSetConstantMaterial(&color);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_BOTHSRCALPHA);
    }
}

/* a spark starts at a random offset of up to SPARK_SPREAD / 2 on each axis
 * from where it's made, and flies on along it. it shrinks and fades out
 * over 20 steps. its speed is its task's extra */
static void chaos_spark_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR *speed;
    NJS_VECTOR offset;
    Float scale;
    NJS_VECTOR *step;

    switch (work->mode) {
    case EFFECT_START:
        if ((task->extra = heap_alloc(SPEED_BYTES)) == NULL) {
            task_destroy(task);
            break;
        }
        speed = task->extra;
        SPARK(work)->fade = 1.0F;
        offset.x = (0.5F - (rand() * RAND_SCALE)) * SPARK_SPREAD;
        offset.y = (0.5F - (rand() * RAND_SCALE)) * SPARK_SPREAD;
        offset.z = (0.5F - (rand() * RAND_SCALE)) * SPARK_SPREAD;
        njAddVector(&work->pos, &offset);
        work->scl.x = chaos_spark_scale;
        scale = ((rand() * RAND_SCALE) + 2.0F) * chaos_spark_speed *
                ((1.0F / 7.0F) * (1.0F / 3.0F));
        speed->x = offset.x * scale;
        speed->y = offset.y * scale;
        speed->z = offset.z * scale;
        work->mode = EFFECT_RUN;
        break;
    case EFFECT_RUN:
        speed = task->extra;
        /* @hack spelling: the speed through a second pointer, step, which
         * gives the function a 16-byte frame (ADD #-16,R15 at 0x8c0fab80);
         * passed as speed, the frame is 12 bytes, the function runs 10 bytes
         * short and 117 of 150 words differ */
        step = speed;
        njAddVector(&work->pos, step);
        work->scl.x *= 0.9F;
        SPARK(work)->fade -= SPARK_FADE;
        if (SPARK(work)->fade <= 0.0F) {
            task_destroy(task);
        } else {
            chaos_sparks_add(work);
        }
        break;
    }
}

/* a gathering spark closes in on its point, growing as it brightens */
static void chaos_gather_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR *speed = task->extra;

    sub_vector(&work->pos, speed);
    work->scl.x += 0.1F / 40.0F;
    SPARK(work)->fade += SPARK_FADE;
    if (SPARK(work)->fade >= 1.0F) {
        task_destroy(task);
    } else {
        chaos_sparks_add(work);
    }
}

/* start a gathering spark 8 out from its point, a random way, to come back
 * in over 20 steps */
static void chaos_gather_start(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR *speed;

    if ((task->extra = heap_alloc(SPEED_BYTES)) == NULL) {
        task_destroy(task);
        return;
    }
    SPARK(work)->fade = 0.0F;
    speed = task->extra;
    njPushMatrix(&_unit_matrix_);
    /* TURN reads its angle twice. each of these draws a random angle for
     * the test, and unless it is 0, draws a second to turn by */
    TURN(njRotateX, ANY_ANGLE());
    TURN(njRotateY, ANY_ANGLE());
    njCalcVector(NULL, &chaos_gather_reach, speed);
    njPopMatrix(1);
    njAddVector(&work->pos, speed);
    speed->x *= 0.05F;
    speed->y *= 0.05F;
    speed->z *= 0.05F;
    work->scl.x = 0.1F;
    task->step = chaos_gather_step;
}

/* on a frame's first step, draw the step's sparks, from blue to orange as
 * they brighten. then empty the list, and end */
static void chaos_sparks_draw(struct task *task)
{
    NJS_ARGB color;
    /* @hack redundant: register on entry, which keeps camera_work's address
     * in R10 (0x8c0faf06); without it, R11, and 27 of 166 words differ */
    register struct task_work **entry;
    /* @hack redundant: register on work, which leaves i R13 (0x8c0faf00,
     * 0x8c0faf08); without it, i takes R14 and 18 of 166 words differ */
    register struct task_work *work;
    Float level;
    /* @hack redundant: register on bright, which leaves a 20-byte frame
     * (ADD #-20,R15 at 0x8c0faed2); without it, bright takes a slot, the
     * frame is 28 bytes, the function runs 6 bytes long and 119 of 166 words
     * differ */
    register Float bright;
    Float scale;
    Angle ang;
    /* @hack redundant: register on i, which keeps it in R13 (0x8c0faf00,
     * 0x8c0faf08); without it, R11, and 26 of 166 words differ */
    register int i;

    if (!frame_step) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njSetTexture(&chaos_effect_texlist);
        entry = chaos_spark_list;
        for (i = chaos_spark_count; i > 0; i--) {
            work = *entry++;
            njPushMatrix(NULL);
            level = (SPARK(work)->fade - 0.25F) * 2.0F;
            /* @hack spelling: the level clamped into a second variable,
             * bright; clamped in place and copied, the function runs 4 bytes
             * long, its pool moves (0x8c0faed4) and 121 of 166 words differ */
            bright = CLAMP(level, 0.0F, 1.0F);
            color.a = 0.5F;
            color.r = (bright * 0.5F) + 0.25F;
            color.g = (bright * (25.0F / 255.0F)) + 0.25F;
            color.b = 0.75F - (bright * 0.5F);
            njSetConstantMaterial(&color);
            njTranslateV(NULL, &work->pos);
            TURN_VIA(njRotateY, camera_work->ang.y, ang);
            TURN_VIA(njRotateX, camera_work->ang.x, ang);
            scale = work->scl.x;
            njScale(NULL, scale, scale, scale);
            njDrawSprite3D(&chaos_spark_sprite, (int)SPARK(work)->frame,
                           NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
            njPopMatrix(1);
        }
        color.a = 1.0F;
        njSetConstantMaterial(&color);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_BOTHSRCALPHA);
    }
    chaos_spark_count = 0;
    task_destroy(task);
}

/* on a frame's first step, draw the step's trail pieces. then empty the
 * list, and end */
static void chaos_trail_draw(struct task *task)
{
    NJS_ARGB color;
    /* @hack redundant: register on entry, which keeps njColorBlendingMode's
     * address in R10 (0x8c0fb0c6); without it, R11, and 22 of 125 words
     * differ */
    register struct task_work **entry;
    /* @hack redundant: register on work, which leaves i R13 (0x8c0fb0e0);
     * without it, i takes R14 and 19 of 125 words differ */
    register struct task_work *work;
    Float scale;
    Angle ang;
    /* @hack redundant: register on i, which keeps it in R13 (0x8c0fb0e0);
     * without it, R12, and 18 of 125 words differ */
    register int i;

    if (!frame_step) {
        color.a = 0.5F;
        color.r = 0.5F;
        color.g = 0.5F;
        color.b = 0.5F;
        njSetConstantMaterial(&color);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njSetTexture(&chaos_effect_texlist);
        entry = chaos_trail_list;
        for (i = chaos_trail_count; i > 0; i--) {
            work = *entry++;
            njPushMatrix(NULL);
            njTranslateV(NULL, &work->pos);
            TURN_VIA(njRotateY, camera_work->ang.y, ang);
            TURN_VIA(njRotateX, camera_work->ang.x, ang);
            scale = work->scl.x;
            njScale(NULL, scale, scale, scale);
            njDrawSprite3D(&chaos_trail_sprite, (int)SPARK(work)->frame,
                           NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
            njPopMatrix(1);
        }
        color.a = 1.0F;
        njSetConstantMaterial(&color);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_BOTHSRCALPHA);
    }
    chaos_trail_count = 0;
    task_destroy(task);
}

/* a drop starts 4 out across from where it's made, a random way, and is
 * thrown up and outward. it wobbles as it flies, and ends once it has
 * fallen back to the height it started at */
static void chaos_drop_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    NJS_ARGB color;
    Angle ang;
    Float dx;
    Float dz;
    Float speed;
    Float squash;

    switch (work->mode) {
    case EFFECT_START:
        ang = RANDOM_ANGLE();
        dx = njCos(ang);
        dz = njSin(ang);
        work->pos.x += dx * 4.0F;
        work->pos.z += dz * 4.0F;
        speed = (rand() * RAND_SCALE) + 0.3F;
        motion->speed.x = dx * speed * 0.2F;
        motion->speed.z = dz * speed * 0.2F;
        motion->accel.y = speed * 0.6F; /* the speed it's thrown up at */
        work->scl.z *= ((rand() * RAND_SCALE) + 1.0F) * 0.08F;
        work->ang.x = RANDOM_ANGLE();
        DROP(work)->steps = 0;
        DROP(work)->limit = motion->accel.y * (2.0F / DROP_GRAVITY);
        work->mode = EFFECT_RUN;
        break;
    case EFFECT_RUN:
        motion->speed.y = motion->accel.y - (DROP(work)->steps * DROP_GRAVITY);
        work->ang.x += NJM_DEG_ANG(22.5);
        squash = (njAbs(njCos(work->ang.x)) + 1.0F) * 0.5F;
        work->scl.x = work->scl.z * squash;
        work->scl.y = work->scl.z * (1.5F - squash);
        njAddVector(&work->pos, &motion->speed);
        if (DROP(work)->steps++ >= DROP(work)->limit) {
            task_destroy(task);
            return;
        }
        break;
    }
    if (!frame_step) {
        color.a = 0.75F;
        color.r = 0.5F;
        color.g = 0.5F;
        color.b = 0.5F;
        njSetConstantMaterial(&color);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njSetTexture(&chaos_effect_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, camera_work->ang.y, ang);
        TURN_VIA(njRotateX, camera_work->ang.x, ang);
        njScale(NULL, work->scl.x, work->scl.y, work->scl.x);
        njDrawSprite3D(&chaos_drop_sprite, 0,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        njPopMatrix(1);
        color.a = 1.0F;
        njSetConstantMaterial(&color);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_BOTHSRCALPHA);
    }
}

void chaos_spot_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;
    Float size;

    switch (work->mode) {
    case EFFECT_START:
        work->pos.x = boss_work->pos.x;
        work->pos.y = boss_work->pos.y + 0.1F;
        work->pos.z = boss_work->pos.z;
        work->mode = EFFECT_RUN;
        break;
    case EFFECT_RUN:
        /* @hack spelling: the alpha lowered inside the test, which loads the 0
         * it is compared with by FLDI0 FR1 (0x8c0fb4ce); lowered before it, the
         * function runs 2 bytes long and 61 of 133 words differ */
        if ((SPOT(work)->alpha -= 0.1F / 15.0F) <= 0.0F) {
            task_destroy(task);
            return;
        }
        break;
    }
    if (frame_step) {
        return;
    }
    color.a = SPOT(work)->alpha;
    color.r = color.g = color.b = 0.0F;
    njSetConstantMaterial(&color);
    njSetTexture(&chaos_effect_texlist);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njPushMatrix(NULL);
    size = (SPOT(work)->alpha < 0.5F ? SPOT(work)->alpha : 0.5F) * 1.25F;
    njTranslate(NULL, work->pos.x, work->pos.y + SPOT(work)->alpha,
                work->pos.z);
    njRotateX(NULL, NJM_DEG_ANG(90));
    njScale(NULL, size, size, size);
    njDrawSprite3D(&chaos_spot_sprite, 0, NJD_SPRITE_ALPHA);
    njPopMatrix(1);
    color.a = 1.0F;
    njSetConstantMaterial(&color);
}

void chaos_effect_splash(struct task_work *work, int count, Float size,
                         Float height)
{
    struct task *task;
    int i;

    for (i = 0; i < count; i++) {
        task = task_create(TASK_MOTION | TASK_WORK, 5, chaos_drop_step);
        if (task != NULL) {
            task->work->pos.x = work->pos.x;
            task->work->pos.y = work->pos.y + height;
            task->work->pos.z = work->pos.z;
            task->work->scl.z = size;
        }
    }
}

void chaos_effect_gather(NJS_POINT3 *pos, int count)
{
    struct task *task;
    int i;

    if (count == 0) {
        count = 2;
    }
    for (i = 0; i < count; i++) {
        task = task_create(TASK_WORK, 5, chaos_gather_start);
        if (task != NULL) {
            task->work->pos = *pos;
        }
    }
}

void chaos_effect_flash(NJS_POINT3 *target)
{
    struct task *task;
    struct task_work *work;

    task = task_create(TASK_WORK, 5, chaos_flash_step);
    if (task != NULL) {
        work = task->work;
        FLASH(work)->frame = 0.0F;
        work->scl.x = chaos_flash_scale;
        FLASH(work)->target = target;
    }
}

void chaos_light_create(Float range)
{
    njCreateLight(&chaos_light, NJD_POINT_LIGHT);
    njSetLightColor(&chaos_light, 0.0F, 1.0F, 1.0F);
    njSetLightIntensity(&chaos_light, 0.5F, 1.0F, 1.0F);
    njSetLightRange(&chaos_light, range, range);
    njLightOn(&chaos_light);
}

void chaos_light_follow(struct task_work *work)
{
    njUnitLightMatrix(&chaos_light);
    njTranslateLightV(&chaos_light, &work->pos);
}

void chaos_light_place(NJS_POINT3 *pos, Float height)
{
    njUnitLightMatrix(&chaos_light);
    njTranslateLight(&chaos_light, pos->x, pos->y + height, pos->z);
}

void chaos_light_delete(void)
{
    njDeleteLight(&chaos_light);
}

/* swing the object's scale from 0.85 up to 1.15 and back */
static void chaos_pulse_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = PULSE(work)->object;
    Float scale;

    work->ang.x += NJM_DEG_ANG(5.625);
    scale = (njAbs(njSin(work->ang.x)) * 0.3F) + 0.85F;
    object->scl[0] = object->scl[1] = object->scl[2] = scale;
}

void chaos_effect_pulse(NJS_OBJECT *object, struct task *owner)
{
    struct task *task;

    task = task_create(TASK_WORK, 2, chaos_pulse_step);
    if (task != NULL) {
        PULSE(task->work)->object = object;
        object->evalflags &= ~NJD_EVAL_UNIT_SCL;
    }
}

/* list a spark to draw this step, unless the list holds SPARK_MAX. the
 * step's first spark starts the task that draws them */
static void chaos_sparks_add(struct task_work *work)
{
    if (chaos_spark_count < SPARK_MAX) {
        if (chaos_spark_count == 0) {
            task_create(0, 6, chaos_sparks_draw);
        }
        chaos_spark_list[chaos_spark_count++] = work;
    }
}

/* a trail piece plays its frames, then ends. each step lists it to draw,
 * unless the list holds TRAIL_MAX. the step's first piece starts the task
 * that draws them */
static void chaos_trail_step(struct task *task)
{
    struct task_work *work = task->work;

    /* @hack spelling: the frame advanced inside the test, which compares the
     * sum in FR1 as it stores it (FCMP/GT FR2,FR1 at 0x8c0fb91a); advanced
     * before it, the function runs 4 bytes long and 31 of 43 words differ */
    if ((SPARK(work)->frame += 1.25F) >= TRAIL_FRAMES) {
        task_destroy(task);
        return;
    }
    if (chaos_trail_count < TRAIL_MAX) {
        if (chaos_trail_count == 0) {
            task_create(0, 6, chaos_trail_draw);
        }
        chaos_trail_list[chaos_trail_count++] = work;
    }
}

static void chaos_trail_start(struct task *task)
{
    struct task_work *work = task->work;

    SPARK(work)->frame = 0.0F;
    work->scl.x = chaos_trail_scale;
    task->step = chaos_trail_step;
}

void chaos_effect_trail(NJS_POINT3 *from, NJS_POINT3 *to, int count)
{
    int sparks;
    NJS_VECTOR step;
    NJS_POINT3 pos;
    int pieces;
    int i;
    struct task *task;

    /* @hack spelling: count copied to sparks, which keeps from in R9
     * (MOV R4,R9 at 0x8c0fb9d6); counted in count itself, R10, and 18 of 113
     * words differ */
    sparks = count;
    if (sparks == 0) {
        sparks = 2;
    }
    step = pos = *from;
    sub_vector(&step, to);
    pieces = (int)(njScalor(&step) * (2.0F / 7.0F));
    step.x /= (Float)pieces;
    step.y /= (Float)pieces;
    step.z /= (Float)pieces;
    i = 0;
    do {
        task = task_create(TASK_WORK, 5, chaos_trail_start);
        if (task != NULL) {
            task->work->pos = pos;
            njSubVector(&pos, &step);
        }
    } while (++i < pieces);
    for (i = 0; i < sparks; i++) {
        task = task_create(TASK_WORK, 5, chaos_spark_step);
        if (task != NULL) {
            task->work->pos = *from;
        }
    }
}

void chaos_set_alpha(NJS_OBJECT **objects, Float alpha)
{
    int byte = (int)(alpha * 255.0F);
    NJS_OBJECT *object;

    while ((object = *objects++) != NULL) {
        NJS_MODEL *model = object->model;
        NJS_MATERIAL *mat = model->mats;
        int count = model->nbMat;
        int i;

        for (i = 0; i < count; i++, mat++) {
            mat->attrflags |= NJD_FLAG_USE_ALPHA;
            mat->diffuse.argb.a = byte;
        }
    }
}

static void chaos_fade_step(struct task *task)
{
    struct fade *fade = task->extra;

    fade->alpha += fade->change;
    fade->steps--;
    if (fade->steps == 0) {
        fade->alpha = fade->end;
        task->step = task_destroy;
    }
    chaos_set_alpha(fade->objects, fade->alpha);
}

struct task *chaos_fade_alpha(struct task *parent, NJS_OBJECT **objects,
                              Float from, Float to, int steps)
{
    struct task *task;

    task = task_create_child(0, chaos_fade_step, parent);
    if (task != NULL) {
        struct fade *fade;

        fade = task->extra = heap_calloc(1, sizeof(*fade));
        fade->objects = objects;
        fade->alpha = from;
        fade->end = to;
        fade->change = (to - from) / (Float)steps;
        fade->steps = steps;
        chaos_set_alpha(objects, from);
    }
    return task;
}

void chaos_effect_set_sizes(Float flash, Float trail, Float spark, Float speed)
{
    if (flash > 0.0F) {
        chaos_flash_scale = flash;
    }
    if (trail > 0.0F) {
        chaos_trail_scale = trail;
    }
    if (spark > 0.0F) {
        chaos_spark_scale = spark;
    }
    if (speed > 0.0F) {
        chaos_spark_speed = speed;
    }
}
