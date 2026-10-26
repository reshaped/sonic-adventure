/* Perfect Chaos's ray along node 15, with its ripples, sparks and flash */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos7/boss.h"
#include "b_chaos7/fight.h"
#include "b_chaos7/ray.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 14
#define TEXT_7_LENGTH 14
#define TEXT_8_LENGTH 14
#define TEXT_9_LENGTH 14
#define TEXT_10_LENGTH 12
#define TEXT_11_LENGTH 12
#define TEXT_12_LENGTH 12
#define TEXT_13_LENGTH 12
#define TEXT_14_LENGTH 12
#define TEXT_15_LENGTH 12
#define TEXT_16_LENGTH 12
#define TEXT_17_LENGTH 12
#define TEXT_18_LENGTH 12
#define TEXT_19_LENGTH 12
#define TEXT_20_LENGTH 12
#define TEXT_21_LENGTH 12
#define TEXT_22_LENGTH 12
#define TEXT_23_LENGTH 12
#define TEXT_24_LENGTH 12
#define TEXT_25_LENGTH 12
#define TEXT_26_LENGTH 12
#define TEXT_27_LENGTH 12
#define TEXT_28_LENGTH 12
#define TEXT_29_LENGTH 12
#define TEXT_30_LENGTH 12
#define TEXT_31_LENGTH 12
#define TEXT_32_LENGTH 12
#define TEXT_33_LENGTH 12
#define TEXT_34_LENGTH 12
#define TEXT_35_LENGTH 12
#define TEXT_36_LENGTH 16
#define TEXT_37_LENGTH 16
#define TEXT_38_LENGTH 16
#define TEXT_39_LENGTH 16
#define TEXT_40_LENGTH 16
#define TEXT_41_LENGTH 16
#define TEXT_42_LENGTH 16
#define TEXT_43_LENGTH 16
#define TEXT_44_LENGTH 16
#define TEXT_45_LENGTH 16
#define TEXT_46_LENGTH 16
#define TEXT_47_LENGTH 16
#define TEXT_48_LENGTH 16
#define TEXT_49_LENGTH 16
#define TEXT_50_LENGTH 16
#define TEXT_51_LENGTH 16
#define TEXT_52_LENGTH 7
#define TEXT_53_LENGTH 7
#define TEXT_54_LENGTH 7
#define TEXT_55_LENGTH 7
#define TEXT_56_LENGTH 14
#include "b_chaos7/ray/text.inc"

extern NJS_OBJECT ray_segment_object;
extern NJS_OBJECT ray_body_object;
extern NJS_OBJECT ray_tip_object;
extern NJS_OBJECT ray_ripple_object;
extern NJS_OBJECT ray_flash_object;
extern NJS_COLOR ray_segment_uvs[];

/* the ray's width, work->scl.x, grows by RAY_GROWTH a step until it reaches
 * RAY_WIDTH. it falls by RAY_THIN a step as the ray thins away */
#define RAY_WIDTH 4.0F
#define RAY_GROWTH (RAY_WIDTH / 30.0F)
#define RAY_THIN 0.4F
/* while the ray holds, its cylinder's middle lies RAY_SHAPE_MIDDLE of the
 * ray's length out along it. the cylinder reaches RAY_SHAPE_HALF of the
 * length either way of its middle */
#define RAY_SHAPE_MIDDLE 0.9F
#define RAY_SHAPE_HALF 0.45F
/* ray_draw scales the ray's frame by RAY_STRETCH along the ray. a move of n in
 * that frame covers n * RAY_STRETCH of the ray's length. each segment, drawn
 * with the body over it, covers RAY_SEGMENT of the length. the tip draws
 * RAY_TIP_BACK, in the scaled frame, short of the ray's end */
#define RAY_STRETCH 4.0F
#define RAY_SEGMENT 120.0F
#define RAY_TIP_BACK 45.0F
/* the segment's first RAY_UVS UVs scroll in pairs. the first of a pair takes
 * the scroll as its v, and the second the scroll plus RAY_UV_SPAN. the scroll
 * falls by RAY_SCROLL a step */
#define RAY_UVS 22
#define RAY_UV_SPAN 255
#define RAY_SCROLL 70
/* the body plays c7_bray01 to c7_bray04, the RAY_FRAMES textures from
 * RAY_BODY_FIRST. the tip plays c7_breathtop01 to c7_breathtop04, from
 * RAY_TIP_FIRST. each texture shows for RAY_TICKS steps */
#define RAY_TICKS 2
#define RAY_FRAMES 4
#define RAY_BODY_FIRST 1
#define RAY_TIP_FIRST 5
/* RAY_REACH is the radius of the ray's collision record, within which the
 * checks test it against another record. the cylinder's radius is RAY_RADIUS
 * times ray_width_scale */
#define RAY_REACH 1000000.0F
#define RAY_RADIUS 40.0F

/* the elements of RAY_VARS's anim */
#define BODY_TICK 0
#define BODY_FRAME 1
#define TIP_TICK 2
#define TIP_FRAME 3

/* the sparks lie SPARK_SPACING apart along their axis. each lies SPARK_NEAR to
 * SPARK_FAR off it, or twice as far in the fight's second part. a spark plays
 * c_attack_b01 to c_attack_b10, the SPARK_FRAMES textures from SPARK_FIRST */
#define SPARK_SPACING 10.0F
#define SPARK_NEAR 30.0F
#define SPARK_FAR 50.0F
#define SPARK_FIRST 9
#define SPARK_FRAMES 10.0F

/* a ripple plays c7_bripple01 to c7_bripple16, the RIPPLE_FRAMES textures from
 * RIPPLE_FIRST, advancing RIPPLE_PACE of a texture a step. it draws at
 * RIPPLE_SCALE times its size. it starts the next ripple once it has waited
 * more than RIPPLE_WAIT steps */
#define RIPPLE_FIRST 19
#define RIPPLE_FRAMES 16.0F
#define RIPPLE_PACE (1.0F / 3.0F)
#define RIPPLE_SCALE 2.0F
#define RIPPLE_WAIT 20
/* the flash plays c7_breathflash01 to c7_breathflash16, the FLASH_FRAMES
 * textures from FLASH_FIRST, each for FLASH_TICKS steps. it draws at
 * FLASH_SCALE times its size */
#define FLASH_FIRST 35
#define FLASH_FRAMES 16
#define FLASH_TICKS 2
#define FLASH_SCALE 5.0F

struct sparks_vars {
    Float spacing;
    Sint32 count;
    Float length; /* the ray's length, which the sparks span */
};

/* a spark, in an array at the sparks task's extra */
struct spark {
    NJS_POINT3 pos;
    Uint8 unk_0c[12];
    Float scale;
    Float frame;
    Float pace;
};

struct ripple_vars {
    Float frame; /* the texture playing, counted from RIPPLE_FIRST */
    Sint8 count; /* the ripples that are to follow this one */
    Sint8 wait; /* the steps it has waited to start the next */
};

struct flash_vars {
    Uint8 unk_00[4];
    /* @hack type: the tick and the frame as an array. ray_flash_step reaches
     * the frame by adding 2 to the array's address (ADD R2,R3 at 0x0c919d1a).
     * as two Sint16 fields, 17 of 97 words differ */
    Sint16 anim[2];
};

/* the elements of FLASH's anim */
#define FLASH_TICK 0
#define FLASH_FRAME 1

#define SPARKS(work) ((struct sparks_vars *)&(work)->vars)
#define RIPPLE(work) ((struct ripple_vars *)&(work)->vars)
#define FLASH(work) ((struct flash_vars *)&(work)->vars)

#pragma section HEAD
/* the ray, its ripples and its sparks tilt by ray_tilt about z */
static Angle ray_tilt = 0x8e00;
/* the ray starts ray_gap out along its axis, in the frame RAY_STRETCH scales */
static Float ray_gap = 50.0F;
/* a ripple draws ripple_reach out along the ray's axis */
static Float ripple_reach = 280.0F;
/* nothing reads dat_0c901b3c */
static Float dat_0c901b3c = 300.0F;
/* the flash draws at flash_x, flash_y and flash_z in node flash_node's frame.
 * it tilts by flash_tilt about z */
static Angle flash_tilt = 0;
static int flash_node = 13;
static Float flash_x = -100.0F;
static Float flash_y = -150.0F;
static Float flash_z = 0.0F;
#pragma section

static const Uint8 ray_shape_count = 1;
static const NJS_POINT3 ray_origin = {0.0F, 0.0F, 0.0F};
static const NJS_VECTOR ray_dir = {1.0F, 0.0F, 0.0F};
static const NJS_POINT3 spark_origin = {0.0F, 0.0F, 0.0F};
static const NJS_VECTOR spark_dir = {0.0F, 0.0F, 0.0F};
static const NJS_VECTOR flash_dir = {1.0F, 0.0F, 0.0F};

static NJS_TEXNAME chaos7_breath_textures[56] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0},
    {TEXT_37, 0, 0}, {TEXT_38, 0, 0}, {TEXT_39, 0, 0}, {TEXT_40, 0, 0},
    {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_43, 0, 0}, {TEXT_44, 0, 0},
    {TEXT_45, 0, 0}, {TEXT_46, 0, 0}, {TEXT_47, 0, 0}, {TEXT_48, 0, 0},
    {TEXT_49, 0, 0}, {TEXT_50, 0, 0}, {TEXT_51, 0, 0}, {TEXT_52, 0, 0},
    {TEXT_53, 0, 0}, {TEXT_54, 0, 0}, {TEXT_55, 0, 0}, {TEXT_56, 0, 0},
};
NJS_TEXLIST chaos7_breath_texlist = {
#include "b_chaos7/ray/chaos7_breath_texlist.inc"
};

/* the ray's cylinder, which collision_init copies into the ray's record.
 * chaos7_ray_start sets the copy's radius. ray_step sets its middle and its
 * length while the ray holds */
static struct collision_shape ray_shapes[1] = {
    {SHAPE_CYLINDER2 | SHAPE_TURN_YZX | SHAPE_HURTABLE,
     ATTACK(2) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_OBJECTS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     RAY_RADIUS,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* a sprite of c7_breath, chaos7_breath_texlist's first texture, and white.
 * nothing reads them */
static NJS_TEXANIM ray_spare_frame = {
#include "b_chaos7/ray/ray_spare_frame.inc"
};
static NJS_SPRITE ray_spare_sprite = {
#include "b_chaos7/ray/ray_spare_sprite.inc"
};
static NJS_ARGB ray_spare_color = {1.0F, 1.0F, 1.0F, 1.0F};

/* in node 15's frame */
static NJS_POINT3 ripple_origin = {0.0F, 0.0F, 0.0F};
static NJS_VECTOR ripple_dir = {1.0F, 0.0F, 0.0F};

static NJS_TEXANIM spark_frame = {
#include "b_chaos7/ray/spark_frame.inc"
};
static NJS_SPRITE spark_sprite = {
#include "b_chaos7/ray/spark_sprite.inc"
};
static NJS_ARGB spark_color = {1.0F, 1.0F, 1.0F, 1.0F};

/* the factor on a ray's drawn width and on its cylinder's radius. it is 2 for
 * a ray that starts in the fight's second part, else 1. every ray draws with
 * the factor of the last ray to start */
static Float ray_width_scale;

static void ray_sparks_start(struct task *task);

/* on a frame's first step, draw the ray without fog. its segments, with the
 * body over each, run out to its length. the tip follows, turned half a turn
 * about y. the ray's alpha is its width over RAY_WIDTH, up to 1 */
static void ray_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float alpha;
    Float drawn;
    Float overrun;
    Float stride;
    NJS_OBJECT *body;
    NJS_OBJECT *segment;
    Angle ang;

    if (frame_step != 0) {
        return;
    }
    draw_control_save();
    draw_control_clear(NJD_CONTROL_3D_CONSTANT_MATERIAL);
    draw_control_set(NJD_CONTROL_3D_OFFSET_MATERIAL);
    alpha = work->scl.x * (1.0F / RAY_WIDTH);
    alpha = alpha > 1.0F ? 1.0F : alpha;
    draw_set_material(alpha, 1.0F, 1.0F, 1.0F);
    njFogDisable();
    njSetTexture(&chaos7_breath_texlist);
    body = &ray_body_object;
    body->model->mats->attr_texId =
        RAY_VARS(work)->anim[BODY_FRAME] + RAY_BODY_FIRST;
    /* @hack spelling: the tip's model read at 4 past the object's address
     * (MOV.L
     * @(4,R3),R2 at 0x0c919252). as ray_tip_object.model, SHC pools the model
     * member's own address, and 14 of 192 words and 1 pool word differ */
    (&ray_tip_object)->model->mats->attr_texId =
        RAY_VARS(work)->anim[TIP_FRAME] + RAY_TIP_FIRST;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, work->ang.z - RAY_SHAPE_TURN, ang);
    njScale(NULL, RAY_STRETCH, work->scl.x * ray_width_scale,
            work->scl.x * ray_width_scale);
    njTranslate(NULL, ray_gap, 0.0F, 0.0F);
    segment = &ray_segment_object;
    stride = RAY_SEGMENT / RAY_STRETCH;
    drawn = 0.0F;
    while (drawn < RAY_VARS(work)->length) {
        draw_tree(segment);
        draw_tree(body);
        njTranslate(NULL, stride, 0.0F, 0.0F);
        drawn += RAY_SEGMENT;
    }
    /* step back over the last segment's overrun to the ray's end, then
     * RAY_TIP_BACK further, where the tip draws */
    overrun = (drawn - RAY_VARS(work)->length) * (1.0F / RAY_STRETCH);
    njTranslate(NULL, -RAY_TIP_BACK - overrun, 0.0F, 0.0F);
    njRotateY(NULL, NJM_DEG_ANG(180));
    draw_tree(&ray_tip_object);
    njPopMatrix(1);
    njFogEnable();
    draw_control_restore();
}

/* grow the ray to RAY_WIDTH and hold it there. once the boss sets
 * RAY_BURSTING, scatter sparks along it and thin it away. until it is gone,
 * it lies along node 15's axis, tilted by ray_tilt. each step until then
 * scrolls its UVs and plays its body and tip */
static void ray_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct task *sparks;
    NJS_POINT3 origin;
    NJS_VECTOR dir;
    NJS_COLOR *uv;
    int i;

    switch (work->mode) {
    case RAY_GROWING:
        work->scl.x += RAY_GROWTH;
        if (work->scl.x >= RAY_WIDTH) {
            work->mode = RAY_HOLDING;
        }
        break;
    case RAY_HOLDING:
        work->collision->shapes->center.y =
            -(RAY_VARS(work)->length * RAY_SHAPE_MIDDLE);
        work->collision->shapes->size_y =
            RAY_VARS(work)->length * RAY_SHAPE_HALF;
        break;
    case RAY_BURSTING:
        sparks = task_create(TASK_WORK, 3, ray_sparks_start);
        if (sparks != NULL) {
            SPARKS(sparks->work)->length = RAY_VARS(work)->length;
            SPARKS(sparks->work)->spacing = SPARK_SPACING;
        }
        sound_stop(SE_RAY);
        work->mode = RAY_THINNING;
        break;
    case RAY_THINNING:
        work->scl.x -= RAY_THIN;
        if (work->scl.x <= 0.0F) {
            work->mode = RAY_GONE;
        }
        break;
    case RAY_GONE:
        return;
    }
    for (i = 0, uv = ray_segment_uvs; i < RAY_UVS; i += 2) {
        uv[0].tex.v = RAY_VARS(work)->scroll;
        uv[1].tex.v = RAY_VARS(work)->scroll + RAY_UV_SPAN;
        uv += 2;
    }
    RAY_VARS(work)->scroll -= RAY_SCROLL;
    if (++RAY_VARS(work)->anim[BODY_TICK] >= RAY_TICKS) {
        RAY_VARS(work)->anim[BODY_TICK] = 0;
        if (++RAY_VARS(work)->anim[BODY_FRAME] >= RAY_FRAMES) {
            RAY_VARS(work)->anim[BODY_FRAME] = 0;
        }
    }
    if (++RAY_VARS(work)->anim[TIP_TICK] >= RAY_TICKS) {
        RAY_VARS(work)->anim[TIP_TICK] = 0;
        if (++RAY_VARS(work)->anim[TIP_FRAME] >= RAY_FRAMES) {
            RAY_VARS(work)->anim[TIP_FRAME] = 0;
        }
    }
    njPushMatrix(&chaos7_beam_matrices[CHAOS7_RAY_BEAM]);
    origin = ray_origin;
    dir = ray_dir;
    njCalcPoint(NULL, &origin, &work->pos);
    njCalcVector(NULL, &dir, &motion->speed);
    njPopMatrix(1);
    work->ang.y =
        -(Angle)(atan2f(motion->speed.z, motion->speed.x) * ANGLE_PER_RADIAN) +
        NJM_DEG_ANG(180);
    work->ang.z = ray_tilt -
                  (Angle)(atan2f(motion->speed.y,
                                 njSqrt(ACROSS_SQUARED(&motion->speed))) *
                          ANGLE_PER_RADIAN) +
                  RAY_SHAPE_TURN;
    collision_register(work);
    ray_draw(task);
}

void chaos7_ray_start(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, ray_shapes, COLLISION_OBJECT, 1, ray_shape_count);
    task->work->collision->radius = RAY_REACH;
    work->scl.x = 0.0F;
    RAY_VARS(work)->length = 0.0F;
    task->step = ray_step;
    task->draw = ray_draw;
    sound_play_held(SE_RAY, (int)work, 0, chaos7_volume);
    ray_width_scale = chaos7_part_2 ? 2.0F : 1.0F;
    work->collision->shapes->radius = ray_width_scale * RAY_RADIUS;
}

/* on a frame's first step, turn the ripple by its speed and ray_tilt, as
 * ray_draw turns the ray. draw it without fog, ripple_reach out along its x
 * and turned half a turn about z */
static void ray_ripple_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    Angle ang;

    if (frame_step == 0) {
        njFogDisable();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        work->ang.y = -(Angle)(atan2f(motion->speed.z, motion->speed.x) *
                               ANGLE_PER_RADIAN) +
                      NJM_DEG_ANG(180);
        work->ang.z =
            ray_tilt - (Angle)(atan2f(motion->speed.y,
                                      njSqrt(ACROSS_SQUARED(&motion->speed))) *
                               ANGLE_PER_RADIAN);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        njTranslate(NULL, ripple_reach, 0.0F, 0.0F);
        njRotateZ(NULL, NJM_DEG_ANG(180));
        njScale(NULL, RIPPLE_SCALE, RIPPLE_SCALE, RIPPLE_SCALE);
        njSetTexture(&chaos7_breath_texlist);
        /* @hack spelling: the ripple's model read at 4 past the object's
         * address (MOV.L @(4,R3),R2 at 0x0c91985a), which draw_tree takes from
         * R3 too. as ray_ripple_object.model, SHC pools the model member's own
         * address, and 13 of 134 words differ. the pool it shares with the
         * next two functions grows a word, and 13 of their 88 words differ */
        (&ray_ripple_object)->model->mats->attr_texId =
            (int)RIPPLE(work)->frame + RIPPLE_FIRST;
        draw_tree(&ray_ripple_object);
        njPopMatrix(1);
        njFogEnable();
    }
}

static void ray_ripple_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct task *next;

    if (RIPPLE(work)->count > 0) {
        if (++RIPPLE(work)->wait > RIPPLE_WAIT) {
            next = task_create(TASK_MOTION | TASK_WORK, 2,
                               chaos7_ray_ripple_start);
            if (next != NULL) {
                RIPPLE(next->work)->count = RIPPLE(work)->count - 1;
            }
            RIPPLE(work)->count = 0;
        }
    }
    RIPPLE(work)->frame += RIPPLE_PACE;
    if (RIPPLE(work)->frame >= RIPPLE_FRAMES) {
        task_destroy(task);
        return;
    }
    njPushMatrix(&chaos7_beam_matrices[CHAOS7_RAY_BEAM]);
    njCalcPoint(NULL, &ripple_origin, &work->pos);
    njCalcVector(NULL, &ripple_dir, &motion->speed);
    njPopMatrix(1);
    ray_ripple_draw(task);
}

void chaos7_ray_ripple_start(struct task *task)
{
    struct task_work *work = task->work;

    RIPPLE(work)->frame = 0.0F;
    RIPPLE(work)->wait = 0;
    task->step = ray_ripple_step;
    task->draw = ray_ripple_draw;
}

/* on a frame's first step, draw each spark still playing, additively */
static void ray_sparks_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct spark *spark = task->extra;
    int count = SPARKS(work)->count;
    NJS_TEXANIM *frame;

    if (frame_step == 0) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njSetConstantMaterial(&spark_color);
        frame = &spark_frame;
        while (--count >= 0) {
            if (spark->frame < SPARK_FRAMES) {
                njPushMatrix(NULL);
                njTranslateV(NULL, &spark->pos);
                frame->texid = (int)spark->frame + SPARK_FIRST;
                njDrawSprite3D(&spark_sprite, 0,
                               NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
                njPopMatrix(1);
            }
            spark++;
        }
    }
}

/* advance each spark still playing, and draw them. the task ends on a step
 * that finds none playing */
static void ray_sparks_step(struct task *task)
{
    struct task_work *work = task->work;
    struct spark *spark = task->extra;
    int count = SPARKS(work)->count;
    int playing = 0;

    while (--count >= 0) {
        if (spark->frame < SPARK_FRAMES) {
            spark->frame += spark->pace;
            playing++;
        }
        spark++;
    }
    if (!playing) {
        task_destroy(task);
    } else {
        ray_sparks_draw(task);
    }
}

/* lay the sparks every spacing out to the ray's length. their axis is node
 * 15's x axis, turned by ray_tilt about z. each lies at a random angle around
 * it, and advances 0.2 to 0.3 of a texture a step. the task ends if the heap
 * can't hold the sparks */
static void ray_sparks_start(struct task *task)
{
    struct task_work *work = task->work;
    struct spark *spark;
    NJS_POINT3 pos;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    NJS_VECTOR unused;
    int count;
    Angle ang;
    Float radius;

    task->step = ray_sparks_step;
    task->draw = ray_sparks_draw;
    SPARKS(work)->count = (int)(SPARKS(work)->length / SPARKS(work)->spacing);
    task->extra = heap_calloc(SPARKS(work)->count, sizeof(struct spark));
    if (task->extra == NULL) {
        task_destroy(task);
        return;
    }
    njPushMatrix(&chaos7_beam_matrices[CHAOS7_RAY_BEAM]);
    pos = spark_origin;
    /* @hack redundant: spark_dir copied into a local nothing reads (its three
     * stores at 0x0c919b18 to 0x0c919b20). with no copy, 129 of 166 words
     * differ */
    unused = spark_dir;
    count = SPARKS(work)->count;
    spark = task->extra;
    TURN(njRotateZ, ray_tilt);
    while (--count >= 0) {
        spark->scale = 1.0F;
        spark->frame = 0.0F;
        spark->pace = RANDOM(2.0F, 3.0F) * 0.1F;
        ang = (Angle)RANDOM_ANGLE();
        radius = RANDOM(SPARK_NEAR, SPARK_FAR);
        if (chaos7_part_2) {
            radius *= 2.0F;
        }
        pos.x -= SPARKS(work)->spacing;
        pos.y = radius * njCos(ang);
        pos.z = radius * njSin(ang);
        njCalcPoint(NULL, &pos, &spark->pos);
        /* @hack redundant: a push and a pop with nothing between them, which
         * leave the matrix stack as it was but keep the image's two calls
         * (JSR @R10 at 0x0c919c3c, JSR @R9 at 0x0c919c40); without them, 113
         * of 166 words differ */
        njPushMatrix(NULL);
        njPopMatrix(1);
        spark++;
    }
    njPopMatrix(1);
}

/* on a frame's first step, draw the flash without fog. it turns by its ang.y
 * about y, and by flash_tilt about z */
static void ray_flash_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njFogDisable();
        njSetTexture(&chaos7_breath_texlist);
        /* @hack spelling: the flash's model read at 4 past the object's address
         * (MOV.L @(4,R3),R2 at 0x0c919c90). as ray_flash_object.model, SHC
         * pools the model member's own address, and 14 of 72 words differ */
        (&ray_flash_object)->model->mats->attr_texId =
            FLASH(work)->anim[FLASH_FRAME] + FLASH_FIRST;
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN(njRotateZ, flash_tilt);
        njScale(NULL, FLASH_SCALE, FLASH_SCALE, FLASH_SCALE);
        draw_tree(&ray_flash_object);
        njPopMatrix(1);
        njFogEnable();
    }
}

/* advance the flash, and end it after its last texture. it sits at its place
 * on node flash_node. its ang.y is the heading of the node's x axis, plus half
 * a turn */
static void ray_flash_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    NJS_VECTOR dir;
    NJS_VECTOR facing;

    if (++FLASH(work)->anim[FLASH_TICK] >= FLASH_TICKS) {
        FLASH(work)->anim[FLASH_TICK] = 0;
        if (++FLASH(work)->anim[FLASH_FRAME] >= FLASH_FRAMES) {
            task_destroy(task);
            return;
        }
    }
    njPushMatrix(&chaos7_node_matrices[flash_node]);
    dir = flash_dir;
    pos.x = flash_x;
    pos.y = flash_y;
    pos.z = flash_z;
    njCalcPoint(NULL, &pos, &work->pos);
    njCalcVector(NULL, &dir, &facing);
    work->ang.y = -(Angle)(atan2f(facing.z, facing.x) * ANGLE_PER_RADIAN) +
                  NJM_DEG_ANG(180);
    njPopMatrix(1);
    ray_flash_draw(task);
}

void chaos7_ray_flash_start(struct task *task)
{
    task->step = ray_flash_step;
    task->draw = ray_flash_draw;
}
