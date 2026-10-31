/* Perfect Chaos's damage effects: bursts, drops, mist and ripples */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos7/damage.h"
#include "b_chaos7/ray.h"
#include "draw/motion.h"
#include "draw/tree.h"
#include "player/player.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 11
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 11
#define TEXT_9_LENGTH 11
#define TEXT_10_LENGTH 11
#define TEXT_11_LENGTH 11
#define TEXT_12_LENGTH 11
#define TEXT_13_LENGTH 11
#define TEXT_14_LENGTH 11
#define TEXT_15_LENGTH 11
#define TEXT_16_LENGTH 11
#define TEXT_17_LENGTH 12
#define TEXT_18_LENGTH 12
#define TEXT_19_LENGTH 12
#define TEXT_20_LENGTH 12
#define TEXT_21_LENGTH 12
#define TEXT_22_LENGTH 12
#define TEXT_23_LENGTH 12
#define TEXT_24_LENGTH 12
#define TEXT_25_LENGTH 9
#define TEXT_26_LENGTH 9
#define TEXT_27_LENGTH 9
#include "b_chaos7/damage/text.inc"

extern NJS_OBJECT ray_ripple_object;

/* a burst plays chaos7_damage_texlist's first BURST_FRAMES textures,
 * c7_damage01 to c7_damage16. a splash plays the SPLASH_FRAMES textures from
 * SPLASH_FIRST, c7_sdamage01 to c7_sdamage08 */
#define BURST_FRAMES 16.0F
#define SPLASH_FIRST 16
#define SPLASH_FRAMES 8
/* a mist puff shows one of c7_mist01 to c7_mist03, textures MIST_FIRST to
 * MIST_END - 1 */
#define MIST_FIRST 24.0F
#define MIST_END 27.0F
/* a ripple plays c7_bripple01 to c7_bripple16, the RIPPLE_FRAMES textures of
 * chaos7_breath_texlist from RIPPLE_FIRST, RIPPLE_TIMES times. it draws at
 * RIPPLE_SCALE times its size, RIPPLE_AHEAD from its task along Chaos's
 * facing */
#define RIPPLE_FIRST 19
#define RIPPLE_FRAMES 16.0F
#define RIPPLE_TIMES 3
#define RIPPLE_SCALE 2.0F
#define RIPPLE_AHEAD 150.0F
/* a burst and a ripple advance a texture every three steps */
#define BURST_PACE (1.0F / 3.0F)

/* a splash shows each texture for SPLASH_TICKS steps. a drop leaves a splash
 * every SPLASH_TICKS steps. a drop's life gains DROP_PACE a step, and the drop
 * ends once its life reaches 1. it falls, gaining DROP_GRAVITY downward a
 * step */
#define SPLASH_TICKS 4
#define DROP_PACE (1.0F / 180.0F)
#define DROP_GRAVITY 0.027222222F

/* the steps with no burst between two */
#define BURSTS_WAIT 8

/* the mist's strength changes by MIST_FADE a step */
#define MIST_FADE (1.0F / 30.0F)

/* shared by bursts and ripples */
struct burst_vars {
    Float frame; /* the texture playing, which BURST_PACE advances a step */
    Sint8 times; /* a ripple's plays left, counting this one */
};

/* shared by drops and splashes */
struct drop_vars {
    Float life; /* how far through its life a drop is, from 0 to 1 */
    /* @hack type: both counts as one long, which each start clears with one
     * store (MOV.L R3,@(12,R14) at 0x0c91a0a8, MOV.L R3,@(12,R5) at
     * 0x0c91a0fc); as two stores, 23 of chaos7_damage_drop_start's 26 words
     * and 8 of damage_splash_start's 8 differ */
    union {
        Sint32 both;
        struct {
            /* steps since the drop's last splash, or since the splash's
             * texture last changed */
            Sint16 ticks;
            /* a splash's texture, 0 to 7, counted from SPLASH_FIRST */
            Sint16 frame;
        } each;
    } counts;
};

struct bursts_vars {
    Uint8 unk_00[4];
    Sint8 count; /* bursts left */
    Sint8 wait; /* steps left with no burst */
    Uint8 unk_06[2];
    Float spread; /* the farthest a burst lands from the task */
};

struct mist_vars {
    Uint8 unk_00[4];
    /* @hack type: the puff's texture as a long, which chaos7_mist_start
     * stores whole (MOV.L R3,@(12,R14) at 0x0c91a578), and its low half,
     * which damage_mist_draw reads; as one Sint16, 2 of 308 words differ */
    union {
        Sint32 whole;
        Sint16 low;
    } texture;
};

#define BURST(work) ((struct burst_vars *)&(work)->vars)
#define DROP(work) ((struct drop_vars *)&(work)->vars)
#define BURSTS(work) ((struct bursts_vars *)&(work)->vars)
#define MIST(work) ((struct mist_vars *)&(work)->vars)

/* a mist puff's offset from Chaos and its drift. chaos7_mist_start turns them
 * so that -x points from Chaos to player 0. it sets the offset's x and z and
 * the drift's z at random */
static const NJS_VECTOR mist_offset = {-300.0F, 0.0F, 0.0F};
static const NJS_VECTOR mist_drift = {0.0F, 0.0F, 1.0F};

/* a mist puff's offset, size, brightness and motion. chaos7_mist_start scales
 * mist_size, mist_height, mist_speed and mist_aside by a random 1 to 1.5. it
 * scales mist_pace by 1 to 2 */
#pragma section HEAD
static Float mist_ahead = -500.0F; /* the offset's x */
static Float mist_ahead_more = -150.0F; /* the most added to mist_ahead */
static Float mist_size = 1.5F; /* the sprite's scale */
static Float mist_height = 150.0F; /* the puff's y */
static Float mist_brightness = 0.3F; /* the puff's color at its brightest */
static Float mist_speed = 1.0F; /* the puff's drift a step */
static Sint32 mist_pace = 32; /* what the puff's ang.x gains a step */
static Float mist_aside = 400.0F; /* the offset's z, to a random side */
#pragma section

static NJS_TEXNAME chaos7_damage_textures[27] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0},
};
NJS_TEXLIST chaos7_damage_texlist = {
#include "b_chaos7/damage/chaos7_damage_texlist.inc"
};

static NJS_TEXANIM burst_frame = {
#include "b_chaos7/damage/burst_frame.inc"
};
static NJS_SPRITE burst_sprite = {
#include "b_chaos7/damage/burst_sprite.inc"
};
static NJS_TEXANIM drop_frame = {
#include "b_chaos7/damage/drop_frame.inc"
};
static NJS_SPRITE drop_sprite = {
#include "b_chaos7/damage/drop_sprite.inc"
};

Sint8 chaos7_mist_on = 1;

static NJS_ARGB burst_color = {1.0F, 1.0F, 1.0F, 1.0F};
static NJS_ARGB drop_color = {1.0F, 1.0F, 1.0F, 1.0F};

/* a burst's offset from the bursts' task, before its random turn */
static NJS_VECTOR burst_unturned = {0.0F, 0.0F, 0.0F};

static NJS_TEXANIM mist_frame = {
#include "b_chaos7/damage/mist_frame.inc"
};
static NJS_SPRITE mist_sprite = {
#include "b_chaos7/damage/mist_sprite.inc"
};
/* how strongly the mist shows, from 0 to 1. chaos7_mist_fade moves it toward
 * chaos7_mist_on */
static Float mist_strength = 1.0F;
/* the puff's color, which damage_mist_draw passes to njSetConstantMaterial as
 * one NJS_ARGB from mist_alpha on */
/* @hack type: four Floats, not an NJS_ARGB, so that SHC pools each channel's
 * own address for its store (FMOV.S FR0 at 0x0c91a26e, 0x0c91a270 and
 * 0x0c91a276); as one NJS_ARGB, 74 of damage_mist_draw's 78 words differ */
#ifdef GCC
/* keeps damage_mist_draw's read past mist_alpha: GCC's four are the members
 * of one NJS_ARGB */
static NJS_ARGB mist_color = {0.0F, 0.0F, 0.0F, 0.0F};
#define mist_alpha mist_color.a
#define mist_red mist_color.r
#define mist_green mist_color.g
#define mist_blue mist_color.b
#else
static Float mist_alpha = 0.0F;
static Float mist_red = 0.0F;
static Float mist_green = 0.0F;
static Float mist_blue = 0.0F;
#endif
static NJS_ARGB mist_color_after = {0.0F, 0.0F, 0.0F, 0.0F};

/* a sprite of c7_breath, chaos7_breath_texlist's first texture, and white.
 * nothing reads them */
static NJS_TEXANIM damage_spare_frame = {
#include "b_chaos7/damage/damage_spare_frame.inc"
};
static NJS_SPRITE damage_spare_sprite = {
#include "b_chaos7/damage/damage_spare_sprite.inc"
};
static NJS_ARGB damage_spare_color = {1.0F, 1.0F, 1.0F, 1.0F};

/* a burst's offset after its random turn */
static NJS_VECTOR burst_turned;

static void damage_splash_start(struct task *task);

static void damage_burst_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        njFogDisable();
        njSetConstantMaterial(&burst_color);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        /* @hack spelling: burst_frame's texid stored at 16 past its address
         * (MOV.W R0,@(16,R3) at 0x0c919ecc); without it, SHC pools texid's
         * own address, and 9 of 51 words differ */
        (&burst_frame)->texid = BURST(work)->frame;
        njDrawSprite3D(&burst_sprite, 0, NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
        njPopMatrix(1);
        njFogEnable();
    }
}

static void damage_burst_step(struct task *task)
{
    struct task_work *work = task->work;

    BURST(work)->frame += BURST_PACE;
    if (BURST(work)->frame >= BURST_FRAMES) {
        task_kill(task);
    } else {
        damage_burst_draw(task);
    }
}

static void damage_burst_start(struct task *task)
{
    struct task_work *work = task->work;

    task->step = damage_burst_step;
    task->draw = damage_burst_draw;
    BURST(work)->frame = 0.0F;
}

/* on a frame's first step, draw a drop or splash without fog, added to the
 * color behind it. its size is work->scl.x */
static void damage_drop_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_SPRITE *sprite;

    if (frame_step == 0) {
        njFogDisable();
        njSetConstantMaterial(&drop_color);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        /* @hack spelling: drop_frame's texid stored at 16 past its address
         * (MOV.W R0,@(16,R3) at 0x0c919f64); without it, SHC pools texid's
         * own address, and 1 of 55 words and 1 pool word differ */
        (&drop_frame)->texid = DROP(work)->counts.each.frame + SPLASH_FIRST;
        sprite = &drop_sprite;
        sprite->sx = sprite->sy = work->scl.x;
        njDrawSprite3D(sprite, 0, NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
        njPopMatrix(1);
        njFogEnable();
    }
}

static void damage_drop_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct task *splash;

    if (++DROP(work)->counts.each.ticks >= SPLASH_TICKS) {
        DROP(work)->counts.each.ticks = 0;
        splash = task_create(TASK_WORK, 3, damage_splash_start);
        if (splash != NULL) {
            splash->work->pos = work->pos;
            splash->work->scl.x = work->scl.x;
        }
    }
    DROP(work)->life += DROP_PACE;
    if (DROP(work)->life >= 1.0F) {
        task_kill(task);
        return;
    }
    motion->speed.y -= DROP_GRAVITY;
    njAddVector(&work->pos, &motion->speed);
    damage_drop_draw(task);
}

void chaos7_damage_drop_start(struct task *task)
{
    struct task_work *work = task->work;

    task->step = damage_drop_step;
    task->draw = damage_drop_draw;
    DROP(work)->life = 0.0F;
    DROP(work)->counts.both = 0;
    work->scl.x = RANDOM(0.5F, 1.5F);
}

static void damage_splash_step(struct task *task)
{
    struct task_work *work = task->work;

    if (++DROP(work)->counts.each.ticks >= SPLASH_TICKS) {
        DROP(work)->counts.each.ticks = 0;
        if (++DROP(work)->counts.each.frame >= SPLASH_FRAMES) {
            task_kill(task);
            return;
        }
    }
    damage_drop_draw(task);
}

static void damage_splash_start(struct task *task)
{
    struct task_work *work = task->work;

    task->step = damage_splash_step;
    task->draw = damage_drop_draw;
    DROP(work)->counts.both = 0;
}

/* each burst lands from half the task's spread out to all of it. its offset
 * turns at random about z and then about y. each TURN draws two random
 * numbers: the first decides whether it turns, and the second gives the
 * turn */
void chaos7_damage_bursts_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *burst;
    struct task_work *burst_work;

    if (!BURSTS(work)->count) {
        task_kill(task);
        return;
    }
    if (BURSTS(work)->wait == 0) {
        BURSTS(work)->wait = BURSTS_WAIT;
        BURSTS(work)->count--;
        burst = task_create(TASK_WORK, 2, damage_burst_start);
        if (burst != NULL) {
            burst_work = burst->work;
            burst_work->pos = work->pos;
            njPushMatrix(&_unit_matrix_);
            TURN(njRotateZ, RANDOM_ANGLE());
            TURN(njRotateY, RANDOM_ANGLE());
            burst_unturned.x = BURSTS(work)->spread * RANDOM(0.5F, 1.0F);
            njCalcVector(NULL, &burst_unturned, &burst_turned);
            njAddVector(&burst_work->pos, &burst_turned);
            njPopMatrix(1);
            /* the burst's draw never reads this scale */
            burst_work->scl.x = work->scl.x * 10.0F;
        }
    } else {
        --BURSTS(work)->wait;
    }
}

/* on a frame's first step, draw a puff without fog, added to the color behind
 * it. it draws nothing while mist_strength is 0. the puff brightens as it comes
 * and dims as it goes */
static void damage_mist_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_SPRITE *sprite;

    if (frame_step != 0) {
        return;
    }
    if (mist_strength == 0.0F) {
        return;
    }
    njFogDisable();
    mist_red = mist_green = mist_blue =
        _builtin_fabsf(njSin(work->ang.x)) * mist_brightness * mist_strength;
    njSetConstantMaterial((NJS_ARGB *)&mist_alpha);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    sprite = &mist_sprite;
    sprite->sx = sprite->sy = work->scl.x;
    /* @hack spelling: mist_frame's texid stored at 16 past its address
     * (MOV.W R0,@(16,R3) at 0x0c91a2ae); without it, SHC pools texid's own
     * address, and 1 of 78 words and 1 pool word differ */
    (&mist_frame)->texid = MIST(work)->texture.low;
    njDrawSprite3D(sprite, 0,
                   NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    njPopMatrix(1);
    njFogEnable();
    njSetConstantMaterial(&mist_color_after);
}

/* drift a puff, and turn its work->ang.x by its ang.y. once ang.x reaches half
 * a turn, the puff has come and gone, and starts again elsewhere */
static void damage_mist_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    work->ang.x += work->ang.y;
    if (work->ang.x >= NJM_DEG_ANG(180)) {
        task->step = chaos7_mist_start;
    } else {
        njAddVector(&work->pos, &motion->speed);
        damage_mist_draw(task);
    }
}

void chaos7_mist_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    NJS_VECTOR delta;
    NJS_VECTOR offset;
    NJS_VECTOR drift;
    Angle facing;
    Float side;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Float unused;

    task->step = damage_mist_step;
    task->draw = damage_mist_draw;
    delta = player_work[0]->pos;
    njSubVector(&delta, &boss_work->pos);
    facing = -(Angle)(atan2f(delta.z, delta.x) * ANGLE_PER_RADIAN) +
             NJM_DEG_ANG(180);
    njPushMatrix(&_unit_matrix_);
    offset = mist_offset;
    drift = mist_drift;
    side = rand() * RAND_SCALE >= 0.5F ? 1.0F : -1.0F;
    if (facing) {
        njRotateY(NULL, facing);
    }
    offset.x = mist_ahead + (mist_ahead_more * rand() * RAND_SCALE);
    offset.z = -(mist_aside * RANDOM(1.0F, 1.5F) * side);
    njCalcVector(NULL, &offset, &delta);
    work->pos = boss_work->pos;
    work->pos.x += delta.x;
    work->pos.z += delta.z;
    work->pos.y = mist_height * RANDOM(1.0F, 1.5F);
    drift.z = mist_speed * RANDOM(1.0F, 1.5F) * side;
    njCalcVector(NULL, &drift, &motion->speed);
    njPopMatrix(1);
    work->scl.x = mist_size * RANDOM(1.0F, 1.5F);
    work->ang.x = 0;
    work->ang.y = (Angle)(RANDOM(1.0F, 2.0F) * mist_pace);
    MIST(work)->texture.whole = (int)RANDOM(MIST_FIRST, MIST_END);
    /* @hack redundant: a read of work->pos.x into a local nothing reads, which
     * SHC drops. the read still keeps RAND_SCALE in FR12 and side in FR13
     * (0x0c91a452, 0x0c91a468); without it, the two trade, and 8 of 230 words
     * differ */
    unused = work->pos.x;
}

void chaos7_mist_fade(struct task *task)
{
    if (mist_strength < chaos7_mist_on) {
        mist_strength =
            mist_strength + MIST_FADE < 1.0F ? mist_strength + MIST_FADE : 1.0F;
    } else if (mist_strength > chaos7_mist_on) {
        mist_strength =
            mist_strength - MIST_FADE > 0.0F ? mist_strength - MIST_FADE : 0.0F;
    }
}

/* on a frame's first step, draw the ripple without fog, turned with Chaos's
 * facing */
static void damage_ripple_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njFogDisable();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, -boss_work->ang.y + NJM_DEG_ANG(180), ang);
        njTranslate(NULL, -RIPPLE_AHEAD, 0.0F, 0.0F);
        njRotateZ(NULL, NJM_DEG_ANG(180));
        njScale(NULL, RIPPLE_SCALE, RIPPLE_SCALE, RIPPLE_SCALE);
        njSetTexture(&chaos7_breath_texlist);
        /* @hack spelling: the ripple's model read at 4 past the object's
         * address (MOV.L @(4,R3),R2 at 0x0c91a6dc), which draw_tree takes from
         * R3 too; as ray_ripple_object.model, 13 of 82 words differ */
        (&ray_ripple_object)->model->mats->attr_texId =
            (int)BURST(work)->frame + RIPPLE_FIRST;
        draw_tree(&ray_ripple_object);
        njPopMatrix(1);
        njFogEnable();
    }
}

/* advance the ripple, which plays RIPPLE_TIMES times, and draw it, even on the
 * step it ends */
static void damage_ripple_step(struct task *task)
{
    struct task_work *work = task->work;

    BURST(work)->frame += BURST_PACE;
    if (BURST(work)->frame >= RIPPLE_FRAMES) {
        if (--BURST(work)->times > 0) {
            BURST(work)->frame = 0.0F;
        } else {
            task_kill(task);
        }
    }
    damage_ripple_draw(task);
}

void chaos7_damage_ripple_start(struct task *task)
{
    struct task_work *work = task->work;

    BURST(work)->frame = 0.0F;
    BURST(work)->times = RIPPLE_TIMES;
    task->step = damage_ripple_step;
    task->draw = damage_ripple_draw;
}
