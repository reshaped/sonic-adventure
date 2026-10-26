/* Knuckles' effects: his punches, digging and radar, and his maximum heat,
 * the attack his fighting gloves charge */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "object/ring.h"
#include "player/knuckles_effects.h"
#include "player/player.h"
#include "player/textures.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the maximum heat's trail keeps up to TRAIL_RINGS rings of TRAIL_POINTS
 * points round him. the strip between two rings takes their points in pairs,
 * then the first pair again to close the loop */
#define TRAIL_RINGS 16
#define TRAIL_POINTS 8
#define STRIP_POINTS ((TRAIL_POINTS * 2) + 2)
#define STRIP_TRIANGLES (STRIP_POINTS - 2)

/* the flash's sprite frames. it steps through them half a frame a step,
 * then ends */
#define FLASH_FRAMES 6
/* the wave's radius at its start, before it shrinks */
#define WAVE_RADIUS 8.0F
/* the steps a glow of the burst lasts */
#define BURST_STEPS 30
/* the rings a fountain scatters, a few at a time, FOUNTAIN_DELAY steps
 * apart */
#define FOUNTAIN_RINGS 32
#define FOUNTAIN_DELAY 17

/* an effect's work mode. EFFECT_START sets it up on its first step, and
 * dirt sinks once it falls below its floor */
enum effect_mode { EFFECT_START, EFFECT_PLAY, EFFECT_SINK };

/* the work of each effect but the fountain, the motes and the pulses, in
 * place of task_work. pos and speed are task_work's pos and scl */
struct effect_work {
    Sint8 mode;
    Uint8 unk_01[5];
    Uint16 timer; /* steps since it was made */
    /* the flash's frame and the dirt's size. for the wave and a puff, the
     * part of it left, from 1 down to 0 */
    Float scale;
    Float flip; /* 1, or -1 to mirror the dirt */
    Float floor; /* the height below which the dirt sinks */
    Angle ang_x;
    Angle spin; /* added to a rock's ang_x and ang_z each step */
    Angle ang_z;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    struct collision_work *collision;
};

/* the fountain's work, in place of task_work */
struct fountain_work {
    Sint8 mode;
    Uint8 unk_01[19];
    Sint32 rings; /* the rings it has yet to scatter */
    Sint32 delay; /* steps until it scatters more */
    Sint32 steps; /* steps it has run */
    NJS_POINT3 pos;
};

/* the work of a mote and of the copy it leaves each step, in place of
 * task_work. a mote circles player 0 */
struct mote_work {
    Sint8 mode;
    Uint8 unk_01[11];
    Float age; /* the part of its life gone, from 0 to 1 */
    Float alpha; /* its alpha before it fades */
    Angle ang; /* its angle round player 0 */
    Angle turn; /* added to ang each step */
    Uint8 unk_1c[4];
    NJS_POINT3 pos;
    Float height; /* its height above player 0 */
    Float radius; /* the radius of its circle round player 0 */
};

/* a pulse's work, in place of task_work. as age goes from 0 to 1, its scale
 * goes from scale_from to scale_to and its alpha from alpha_from to
 * alpha_to */
struct pulse_work {
    Uint8 unk_00[8];
    Float alpha_to;
    Float age;
    Float alpha_from;
    Uint8 unk_14[24];
    Float speed; /* added to age each step */
    Float scale_from;
    Float scale_to;
};

#define EFFECT(task) ((struct effect_work *)(task)->work)
#define FOUNTAIN(task) ((struct fountain_work *)(task)->work)
#define MOTE(task) ((struct mote_work *)(task)->work)
#define PULSE(task) ((struct pulse_work *)(task)->work)

/* the points round him at one step of the trail */
struct trail_ring {
    NJS_POINT3 points[TRAIL_POINTS];
};

/* the trail's rings, newest first, of which the first count hold points */
struct trail {
    Sint32 count;
    struct trail_ring rings[TRAIL_RINGS];
};

extern NJS_OBJECT knuckles_swing_object;
/* @hack type: the aura as an array, whose address SHC loads before it reads
 * model at @(4,R3) (0x8c0d40fc). as an object, SHC folds model's offset into
 * the pool word, and 20 of the 185 words of knuckles_draw_aura and
 * pulse_step differ */
extern NJS_OBJECT knuckles_aura_object[];

static void burst_step(struct task *task);
static void rock_step(struct task *task);
static void fountain_step(struct task *task);
static void mote_trail_step(struct task *task);
static void mote_step(struct task *task);
static void pulse_step(struct task *task);

static NJS_TEXANIM flash_texanims[FLASH_FRAMES] = {
#include "player/knuckles_effects/flash_texanims.inc"
};
static NJS_SPRITE flash_sprite = {
#include "player/knuckles_effects/flash_sprite.inc"
};
/* a soft glow, which the puffs, the burst, the wave and the motes draw */
static NJS_TEXANIM glow_texanim = {
#include "player/knuckles_effects/glow_texanim.inc"
};
static NJS_SPRITE puff_sprite = {
#include "player/knuckles_effects/puff_sprite.inc"
};
static NJS_SPRITE wave_sprite = {
#include "player/knuckles_effects/wave_sprite.inc"
};
static NJS_SPRITE mote_sprite = {
#include "player/knuckles_effects/mote_sprite.inc"
};
/* a fleck of earth, as dirt and as rock */
static NJS_TEXANIM fleck_texanim = {
#include "player/knuckles_effects/fleck_texanim.inc"
};
static NJS_SPRITE dirt_sprite = {
#include "player/knuckles_effects/dirt_sprite.inc"
};
static NJS_SPRITE rock_sprite = {
#include "player/knuckles_effects/rock_sprite.inc"
};
/* the radar's lights, and the pieces it has found */
static NJS_TEXANIM radar_texanims[10] = {
#include "player/knuckles_effects/radar_texanims.inc"
};
static NJS_SPRITE radar_sprite = {
#include "player/knuckles_effects/radar_sprite.inc"
};

/* the wave's shape, whose radius shrinks with the wave */
/* clang-format off */
static struct collision_shape wave_shapes[] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_ENEMIES,
     PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, WAVE_RADIUS, 0.0F, 0.0F, {0}, 0,
     NULL},
};
/* clang-format on */

static struct trail trail;
static NJS_POINT3 trail_points[STRIP_POINTS];
static NJS_COLOR trail_colors[STRIP_POINTS];
static NJS_POINT3COL trail_strip = {
#include "player/knuckles_effects/trail_strip.inc"
};

void knuckles_fist_flash_step(struct task *task)
{
    struct effect_work *work = EFFECT(task);

    switch (work->mode) {
    case EFFECT_START:
        work->scale = 0.0F;
        work->mode = EFFECT_PLAY;
        break;
    case EFFECT_PLAY:
        /* @hack spelling: scale added to inside the test, which compares the
         * sum in FR2 as it stores it (FCMP/GT FR2,FR1 at 0x8c0d2b3a). added
         * apart, SHC loads scale back, and 77 of 103 words differ */
        if ((work->scale += 0.5F) >= FLASH_FRAMES) {
            task_kill(task);
            return;
        }
        break;
    }
    if (frame_step == 0) {
        draw_set_material(0.5F, 1.0F, 1.0F, 1.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njSetTexture(&knu_eff_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njDrawSprite3D(&flash_sprite, (Uint32)work->scale,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
    placement_try_leave(task);
}

void knuckles_punch_wave_step(struct task *task)
{
    struct effect_work *work = EFFECT(task);
    Float alpha;

    switch (work->mode) {
    case EFFECT_START:
        work->scale = 1.0F;
        collision_init(task, wave_shapes, COLLISION_OBJECT, 1, 1);
        work->mode = EFFECT_PLAY;
        break;
    case EFFECT_PLAY:
        /* @hack spelling: scale added to inside the test, which compares the
         * sum in FR2 and stores it in the BT/S's slot (0x8c0d2c14,
         * 0x8c0d2c18). added apart, SHC loads scale back, and 13 of 134
         * words differ */
        if ((work->scale += -1.0F / (0.2F * 60.0F)) <= 0.0F) {
            task_kill(task);
            return;
        }
        work->pos.x += work->speed.x;
        work->pos.y += work->speed.y;
        work->pos.z += work->speed.z;
        work->collision->shapes->radius = work->scale * WAVE_RADIUS;
        collision_register((struct task_work *)work);
        break;
    }
    if (frame_step == 0) {
        alpha = work->scale * 0.8F;
        draw_set_material(alpha, 1.0F, 1.0F, 1.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njSetTexture(&knu_eff_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njDrawSprite3D(&wave_sprite, 0,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
    placement_try_leave(task);
}

void knuckles_puff_step(struct task *task)
{
    struct effect_work *work = EFFECT(task);
    Float alpha;
    Float size;

    switch (work->mode) {
    case EFFECT_START:
        work->scale = 1.0F;
        work->speed.x = (rand() * RAND_SCALE) - 0.5F;
        work->speed.y = (rand() * RAND_SCALE) - 0.5F;
        work->speed.z = (rand() * RAND_SCALE) - 0.5F;
        if (njScalor(&work->speed) == 0.0F) {
            work->speed.x = 1.0F;
            work->speed.y = work->speed.z = 0.0F;
        } else {
            njUnitVector(&work->speed);
        }
        work->speed.x *= 0.5F;
        work->speed.y *= 0.5F;
        work->speed.z *= 0.5F;
        work->mode = EFFECT_PLAY;
        break;
    case EFFECT_PLAY:
        /* @hack spelling: scale added to inside the test, which compares the
         * sum in FR2 and stores it in the BT/S's slot (0x8c0d2e0a,
         * 0x8c0d2e0e). added apart, SHC loads scale back, and 20 of 234
         * words differ */
        if ((work->scale += -1.0F / (2.5F * 60.0F)) <= 0.0F) {
            task_kill(task);
            return;
        }
        work->pos.x += work->speed.x;
        work->pos.y += work->speed.y;
        work->pos.z += work->speed.z;
        work->speed.x *= 0.975F;
        work->speed.y *= 0.975F;
        work->speed.z *= 0.975F;
        break;
    }
    if (frame_step == 0) {
        alpha = 0.005F + (work->scale * 0.015F);
        /* @hack redundant: the scale times 1.0F, which SHC multiplies by the
         * 1.0F it keeps in FR15 (FMUL FR15,FR5 at 0x8c0d2f04). without it,
         * 87 of 234 words differ */
        size = 4.0F - (work->scale * 1.0F);
        draw_set_material(alpha, 1.0F, 1.0F, 1.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njSetTexture(&knu_eff_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, camera_work->ang.y);
        TURN(njRotateX, camera_work->ang.x);
        njScale(NULL, size, size, size);
        njDrawSprite3D(&puff_sprite, 0,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_VFLIP | NJD_SPRITE_COLOR);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
    placement_try_leave(task);
}

/* a glow of the burst, which slows and fades out over BURST_STEPS */
static void burst_step(struct task *task)
{
    struct effect_work *work = EFFECT(task);
    NJS_POINT3 pos; /* NOLINT(clang-diagnostic-unused-but-set-variable) */
    NJS_VECTOR drag;
    Float alpha;

    if (work->mode == EFFECT_START) {
        work->mode = EFFECT_PLAY;
    }
    /* @hack spelling: the timer stepped inside the test, which compares the
     * new count in R0 as it stores it (CMP/GT R3,R0 at 0x8c0d2fc2).
     * stepped apart, 157 of 177 words differ */
    if (++work->timer > BURST_STEPS) {
        task_kill(task);
        return;
    }
    /* @hack redundant: a copy of the work's pos that nothing reads, which
     * SHC stores at @(12,R15) in a 24-byte frame (0x8c0d2fd8, ADD #-24,R15
     * at 0x8c0d2fa8). without it, the frame is 12 bytes, and 41 of 177 words
     * differ */
    pos = work->pos;
    drag.x = work->speed.x;
    drag.y = work->speed.y + 0.1F;
    drag.z = work->speed.z;
    /* @hack spelling: the stores through &drag, whose address SHC copies from
     * R15 for each (MOV R15,R2 at 0x8c0d300c and 0x8c0d3012, MOV R15,R3 at
     * 0x8c0d3018). as drag.x, drag.y and drag.z, SHC stores off R15, and 10
     * of 177 words differ */
    if (njScalor(&drag) == 0.0F) {
        (&drag)->x = 1.0F;
        (&drag)->y = (&drag)->z = 0.0F;
    } else {
        njUnitVector(&drag);
    }
    /* @hack spelling: each part scaled inside its test, which scales, stores
     * and tests it in FR2 (0x8c0d3090 to 0x8c0d3096). scaled in a statement
     * before each test, 104 of 177 words differ */
    if (njAbs(drag.x *= -0.05F) < 0.001F) {
        drag.x = 0.0F;
    }
    if (njAbs(drag.y *= -0.05F) < 0.001F) {
        drag.y = 0.0F;
    }
    if (njAbs(drag.z *= -0.05F) < 0.001F) {
        drag.z = 0.0F;
    }
    njAddVector(&work->speed, &drag);
    njAddVector(&work->pos, &work->speed);
    if (frame_step == 0) {
        alpha = (BURST_STEPS - work->timer) * (1.0F / BURST_STEPS);
        draw_set_material(alpha, 1.0F, 1.0F, 1.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njSetTexture(&knu_eff_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njDrawSprite3D(&puff_sprite, 0,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
    placement_try_leave(task);
}

void knuckles_make_burst(NJS_POINT3 *pos, int unused)
{
    /* @hack type: i as a byte, which SHC counts with ADD #1 and EXTU.B
     * (0x8c0d3330, 0x8c0d3332) over four copies of the body. as an int, SHC
     * counts down with DT over one copy, and 269 of 270 words differ */
    Uint8 i;
    struct task *task;

    for (i = 0; i < 10; i++) {
        /* @hack spelling: task assigned in the test, which tests task_create's
         * R0 and copies it in the BT/S's slot (0x8c0d31d2, 0x8c0d31d6).
         * assigned apart, 24 of 270 words differ */
        if ((task = task_create(TASK_WORK, 6, burst_step)) != NULL) {
            EFFECT(task)->pos = *pos;
            EFFECT(task)->speed.x =
                (1.0F - (rand() * (1.0F / 16384.0F))) * 2.0F;
            EFFECT(task)->speed.y =
                (1.0F - (rand() * (1.0F / 16384.0F))) * 2.0F;
            EFFECT(task)->speed.z =
                (1.0F - (rand() * (1.0F / 16384.0F))) * 2.0F;
        }
    }
}

void knuckles_dirt_step(struct task *task)
{
    struct effect_work *work = EFFECT(task);
    Float stretch;
    Float width;
    Float height;

    /* @hack spelling: the timer stepped inside the test, which converts the
     * new count in R0 as it stores it (EXTU.W R0,R0 at 0x8c0d33d2). stepped
     * apart, SHC loads it back, and 28 of 219 words differ */
    if (++work->timer > 0.8F * 60.0F) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case EFFECT_START:
        work->floor = work->pos.y - (rand() * RAND_SCALE);
        work->scale = (rand() * RAND_SCALE) + 1.0F;
        if (rand() * RAND_SCALE < 0.5F) {
            work->flip = 1.0F;
        } else {
            work->flip = -1.0F;
        }
        work->mode = EFFECT_PLAY;
        return;
    case EFFECT_PLAY:
        work->speed.y += -0.2F;
        work->pos.x += work->speed.x;
        work->pos.y += work->speed.y;
        work->pos.z += work->speed.z;
        if (work->pos.y < work->floor) {
            work->mode = EFFECT_SINK;
        }
        break;
    case EFFECT_SINK:
        work->pos.y += -0.2F;
        break;
    }
    if (frame_step == 0) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njSetTexture(&knu_eff_texlist);
        njPushMatrix(NULL);
        /* stretched as it rises, squashed as it falls */
        stretch = work->speed.y * 0.25F;
        if (stretch < -0.25F) {
            stretch = -0.25F;
        } else if (stretch > 0.25F) {
            stretch = 0.25F;
        }
        width = (1.0F - stretch) * work->scale * work->flip;
        height = (stretch + 1.0F) * work->scale;
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, camera_work->ang.y);
        TURN(njRotateX, camera_work->ang.x + NJM_DEG_ANG(180));
        njScale(NULL, width, height, width);
        njDrawSprite3D(&dirt_sprite, 0, NJD_SPRITE_ALPHA);
        njPopMatrix(1);
    }
    placement_try_leave(task);
}

void knuckles_draw_radar_sprite(NJS_POINT3 *pos, int sprite, Float scale,
                                Float alpha)
{
    if (frame_step == 0) {
        draw_set_material(alpha, 1.0F, 1.0F, 1.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
        radar_sprite.p.x = pos->x;
        radar_sprite.p.y = pos->y;
        radar_sprite.sx = scale;
        radar_sprite.sy = scale;
        njSetTexture(&knu_eff_texlist);
        njDrawSprite2D(&radar_sprite, sprite, (scale * 0.01F) - 2.0F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
}

/* a chip of rock, which tumbles as it falls, for 60 steps */
static void rock_step(struct task *task)
{
    struct effect_work *work = EFFECT(task);

    /* @hack spelling: the timer stepped inside the test, which compares the
     * new count in R0 as it stores it (EXTU.W R0,R0 at 0x8c0d36b6). stepped
     * apart, SHC loads it back, and 102 of 110 words differ */
    if (++work->timer >= 60) {
        task_kill(task);
    }
    /* @hack spelling: the speed added to inside the test, which compares the
     * sum in FR2 and stores it in the BF/S's slot (0x8c0d36d2, 0x8c0d36d6).
     * added apart, SHC loads it back, and 95 of 110 words differ */
    if ((work->speed.y += -0.1F) < -2.0F) {
        work->speed.y = -2.0F;
    }
    work->pos.x += work->speed.x;
    work->pos.y += work->speed.y;
    work->pos.z += work->speed.z;
    work->ang_x += work->spin;
    work->ang_z += work->spin;
    if (frame_step == 0) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njSetTexture(&knu_eff_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateX, work->ang_x);
        TURN(njRotateZ, work->ang_z);
        njDrawSprite3D(&rock_sprite, 0, NJD_SPRITE_ALPHA);
        njPopMatrix(1);
    }
}

void knuckles_scatter_rocks(NJS_POINT3 *pos, NJS_VECTOR *speed)
{
    int i;
    struct task *task;

    for (i = 0; i < 4; i++) {
        /* @hack spelling: task assigned in the test, which tests task_create's
         * R0 and copies it in the BT/S's slot (0x8c0d3838, 0x8c0d383c).
         * assigned apart, 12 of 380 words differ */
        if ((task = task_create(TASK_WORK, 6, rock_step)) != NULL) {
            EFFECT(task)->pos = *pos;
            /* @hack redundant: each spread times 1.0F, which SHC adds with
             * FMAC by the 1.0F it keeps in FR13 (FMAC FR0,FR3,FR2 at
             * 0x8c0d3860). without them, 352 of 380 words differ */
            EFFECT(task)->speed.x =
                speed->x + ((0.5F - (rand() * RAND_SCALE)) * 1.0F);
            EFFECT(task)->speed.y =
                speed->y + ((0.5F - (rand() * RAND_SCALE)) * 1.0F);
            EFFECT(task)->speed.z =
                speed->z + ((0.5F - (rand() * RAND_SCALE)) * 1.0F);
            EFFECT(task)->ang_x = (Angle)(rand() * RAND_SCALE * 65536.0F);
            EFFECT(task)->spin =
                (Angle)((0.5F - (rand() * RAND_SCALE)) * 4096.0F);
            EFFECT(task)->ang_z = (Angle)(rand() * RAND_SCALE * 65536.0F);
        }
    }
}

/* a fountain of what his digging turns up: FOUNTAIN_RINGS rings, up to 5
 * at a time, and dirt and rock every eighth step. it ends once it has
 * scattered the rings */
static void fountain_step(struct task *task)
{
    struct fountain_work *work = FOUNTAIN(task);
    NJS_VECTOR up;
    struct task *child;
    int count;
    Angle ang;

    switch (work->mode) {
    case EFFECT_START:
        work->rings = FOUNTAIN_RINGS;
        work->delay = FOUNTAIN_DELAY;
        work->mode = EFFECT_PLAY;
        break;
    case EFFECT_PLAY:
        /* @hack spelling: delay counted down inside the test, which tests R1
         * as it stores it (CMP/PL R1 at 0x8c0d3b62). counted down apart, SHC
         * tests a copy, and 38 of 182 words differ */
        if (--work->delay <= 0) {
            work->delay = FOUNTAIN_DELAY;
            count = (int)(rand() * RAND_SCALE * 5.1F);
            if (work->rings < count) {
                count = work->rings;
            }
            ring_scatter(work->pos.x, work->pos.y - 8.0F, work->pos.z, count);
            /* @hack spelling: rings counted down inside the test, which tests
             * R2 as it stores it (CMP/PL R2 at 0x8c0d3b9e). counted down
             * apart, SHC tests a copy, and 7 of 182 words differ */
            if ((work->rings -= count) <= 0) {
                task_kill(task);
                return;
            }
        }
        break;
    }
    /* @hack spelling: steps counted inside the test, which tests R0 and stores
     * it in the BF/S's slot (TST #7,R0 at 0x8c0d3be4). counted apart, SHC
     * counts in R3 and copies it to R0, and 112 of 182 words differ */
    if (!(++work->steps & 7)) {
        for (ang = (Angle)(rand() * RAND_SCALE * 2048.0F); ang < 0x10000;
             ang += 0x800) {
            /* @hack spelling: child assigned in the test, which tests
             * task_create's R0 (TST R0,R0 at 0x8c0d3c1c). assigned apart,
             * rand's address takes R12, not R13, and 18 of 182 words
             * differ */
            if ((child = task_create(TASK_WORK, 6, knuckles_dirt_step)) !=
                NULL) {
                EFFECT(child)->pos = work->pos;
                EFFECT(child)->speed.x = rand() * njSin(ang) * RAND_SCALE;
                EFFECT(child)->speed.y = (rand() * RAND_SCALE * 1.75F) + 0.25F;
                EFFECT(child)->speed.z = rand() * njCos(ang) * RAND_SCALE;
            }
        }
        up.x = up.z = 0.0F;
        up.y = 1.0F;
        knuckles_scatter_rocks(&work->pos, &up);
    }
}

void knuckles_make_fountain(NJS_POINT3 *pos)
{
    struct task *task;

    /* @hack spelling: task assigned in the test, which tests task_create's R0
     * and copies it in the BT/S's slot (0x8c0d3d10, 0x8c0d3d14). assigned
     * apart, 3 of 23 words differ */
    if ((task = task_create(TASK_WORK, 6, fountain_step)) != NULL) {
        FOUNTAIN(task)->pos = *pos;
    }
}

static void mote_draw(NJS_POINT3 *pos, Float alpha)
{
    if (frame_step == 0) {
        njSetTexture(&knu_eff_texlist);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        draw_set_material(alpha, 1.0F, 1.0F, 1.0F);
        njPushMatrix(NULL);
        njTranslateV(NULL, pos);
        njDrawSprite3D(&mote_sprite, 0,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
}

/* the copy a mote leaves, which fades out over about 6 steps */
static void mote_trail_step(struct task *task)
{
    struct mote_work *work = MOTE(task);

    /* @hack spelling: age added to inside the test, which compares the sum in
     * FR2 and stores it in the BT/S's slot (0x8c0d3db6, 0x8c0d3dba). added
     * apart, SHC loads age back, and 22 of 27 words differ */
    if ((work->age += 1.0F / (0.1F * 60.0F)) >= 1.0F) {
        task_kill(task);
        return;
    }
    mote_draw(&work->pos, (1.0F - work->age) * work->alpha);
}

/* a mote, which circles player 0 for about 18 steps as it fades, and
 * leaves a copy each step. it ends once player 0 is gone */
static void mote_step(struct task *task)
{
    struct mote_work *work = MOTE(task);
    struct task_work *player;
    struct task *child;
    Float alpha;

    /* @hack spelling: age added to inside the test, which compares the sum in
     * FR2 and stores it in the BF/S's slot (0x8c0d3df2, 0x8c0d3df6). added
     * apart, SHC loads age back, and 19 of 91 words differ */
    if ((work->age += 1.0F / (0.3F * 60.0F)) >= 1.0F) {
        task_kill(task);
        return;
    }
    player = player_work[0];
    if (player == NULL) {
        task_kill(task);
        return;
    }
    work->ang += work->turn;
    work->pos.x = (work->radius * njCos(work->ang)) + player->pos.x;
    work->pos.y = work->height + player->pos.y;
    work->pos.z = (work->radius * njSin(work->ang)) + player->pos.z;
    alpha = work->alpha * (1.0F - work->age);
    mote_draw(&work->pos, alpha);
    /* @hack spelling: child assigned in the test, which tests task_create's R0
     * and copies it in the BT/S's slot (0x8c0d3ec0, 0x8c0d3ec4). assigned
     * apart, 3 of 91 words differ */
    if ((child = task_create(TASK_WORK, 6, mote_trail_step)) != NULL) {
        MOTE(child)->pos = work->pos;
        MOTE(child)->alpha = alpha;
    }
}

void knuckles_make_mote(Float charge)
{
    struct task *task;
    struct task_work *player;
    Angle turn;

    player = player_work[0];
    if (player == NULL) {
        return;
    }
    /* @hack spelling: task assigned in the test, which tests task_create's R0
     * and copies it in the BT/S's slot (0x8c0d3f04, 0x8c0d3f08). assigned
     * apart, 12 of 83 words differ */
    if ((task = task_create(TASK_WORK, 6, mote_step)) == NULL) {
        return;
    }
    turn = (Angle)(charge * 4096.0F) + 16;
    if (rand() * RAND_SCALE < 0.5F) {
        turn = -turn;
    }
    MOTE(task)->height = (rand() * RAND_SCALE * 8.0F) + 1.0F;
    MOTE(task)->radius = (rand() * RAND_SCALE * 2.5F) + 2.5F;
    MOTE(task)->ang = (Angle)(rand() * RAND_SCALE * 65536.0F);
    MOTE(task)->turn = turn;
    MOTE(task)->alpha = charge;
}

void knuckles_draw_swing(NJS_POINT3 *pos, struct angles *ang, Float frame)
{
    Float alpha;

    if (frame < 25.0F) {
        alpha = (frame - 20.0F) * 0.15F;
    } else {
        alpha = (30.0F - frame) * 0.15F;
    }
    if (frame_step == 0) {
        draw_set_material(alpha, 1.0F, 1.0F, 1.0F);
        njPushMatrix(NULL);
        njTranslateV(NULL, pos);
        TURN(njRotateZ, ang->z);
        TURN(njRotateX, ang->x);
        TURN(njRotateY, NJM_DEG_ANG(180) - ang->y);
        TURN(njRotateX, (Angle)(frame * 6553.6F));
        njScale(NULL, 1.0F, 2.0F, 2.0F);
        njSetTexture(&knu_eff_texlist);
        draw_tree(&knuckles_swing_object);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
}

void knuckles_draw_aura(NJS_POINT3 *pos, Float pulse)
{
    Float size;
    Float alpha;

    if (frame_step != 0) {
        return;
    }
    knuckles_aura_object->model->mats->attr_texId = 1;
    alpha = -(pulse * 0.6F);
    draw_set_material(alpha, 1.0F, 1.0F, 1.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, pos);
    /* @hack redundant: the square times 1.0F, which SHC multiplies by the
     * 1.0F in FR14 (FMUL FR14,FR3 at 0x8c0d412e). without it, 48 of 85 words
     * differ */
    size = 2.0F - (pulse * pulse * 1.0F);
    njScale(NULL, size, size, size);
    TURN(njRotateY, (Angle)(pulse * 262144.0F));
    njSetTexture(&knu_eff_texlist);
    draw_tree(knuckles_aura_object);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* a pulse of the aura round player 0. it ends once age reaches 1, or once
 * player 0 is gone */
static void pulse_step(struct task *task)
{
    struct pulse_work *work = PULSE(task);
    struct task_work *player;
    Float scale;
    Float alpha;

    /* @hack spelling: player and age set inside the test, which keeps player at
     * @R15 by a store in the BT/S's slot (TST R2,R2 at 0x8c0d419a,
     * 0x8c0d419e). set apart, SHC keeps player in R14, and 91 of 100 words
     * differ */
    if ((player = player_work[0]) == NULL ||
        (work->age += work->speed) >= 1.0F) {
        task_kill(task);
        return;
    }
    if (frame_step == 0) {
        knuckles_aura_object->model->mats->attr_texId = 2;
        scale = work->scale_from +
                ((work->scale_to - work->scale_from) * work->age);
        alpha = work->alpha_from +
                ((work->alpha_to - work->alpha_from) * work->age);
        draw_set_material(alpha, 1.0F, 1.0F, 1.0F);
        njPushMatrix(NULL);
        njTranslateV(NULL, &player->collision->shapes->center);
        njScale(NULL, scale, scale, scale);
        njSetTexture(&knu_eff_texlist);
        draw_tree(knuckles_aura_object);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
}

void knuckles_pulse_in(void)
{
    struct task *task;
    struct pulse_work *work;

    /* @hack spelling: task assigned in the test, which tests task_create's R0
     * and copies it in the BT/S's slot (0x8c0d42c0, 0x8c0d42c4). assigned
     * apart, 3 of 32 words differ */
    if ((task = task_create(TASK_WORK, 6, pulse_step)) != NULL) {
        work = PULSE(task);
        work->speed = 1.0F / (0.3F * 60.0F);
        work->scale_from = 6.0F;
        work->scale_to = 1.2F;
        work->alpha_from = 0.0F;
        work->alpha_to = -1.0F;
    }
}

void knuckles_pulse_out(void)
{
    struct task *task;
    struct pulse_work *work;

    /* @hack spelling: task assigned in the test, which tests task_create's R0
     * and copies it in the BT/S's slot (0x8c0d4300, 0x8c0d4304). assigned
     * apart, 3 of 32 words differ */
    if ((task = task_create(TASK_WORK, 6, pulse_step)) != NULL) {
        work = PULSE(task);
        work->speed = 1.0F / (0.4F * 60.0F);
        work->scale_from = 0.6F;
        work->scale_to = 4.5F;
        work->alpha_from = 0.0F;
        work->alpha_to = -1.0F;
    }
}

void knuckles_trail_clear(void)
{
    trail.count = 0;
}

/* add a ring of radius 5 round a point 7.5 behind work and 7.5 above it,
 * turned by work's angles. then draw a red strip from each ring to the next.
 * the trail is faint at both ends and brightest a fifth of the way back */
void knuckles_trail_draw(struct task_work *work)
{
    int i;
    Angle turn;
    NJS_POINT3 *point;
    NJS_COLOR *color;
    Float fade;

    i = trail.count;
    if (i > TRAIL_RINGS - 1) {
        i = TRAIL_RINGS - 1;
    }
    for (; i > 0; i--) {
        trail.rings[i] = trail.rings[i - 1];
    }
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, NJM_DEG_ANG(180) - work->ang.y);
    /* @hack spelling: a bare scope for spoke and out, whose frame bytes
     * col's block below reuses (ADD #-28,R15 at 0x8c0d43b8). declared at the
     * top, they keep their own bytes, the frame takes 32, and 21 of 418
     * words differ */
    {
        NJS_VECTOR spoke;
        NJS_VECTOR out;

        for (i = 0, turn = 0; i < TRAIL_POINTS; i++, turn += NJM_DEG_ANG(45)) {
            spoke.x = -7.5F;
            spoke.y = njSin(turn) * 5.0F;
            spoke.z = njCos(turn) * 5.0F;
            njCalcVector(NULL, &spoke, &out);
            njAddVector(&out, &work->pos);
            out.y += 7.5F;
            trail.rings[0].points[i] = out;
        }
    }
    njPopMatrix(1);
    /* @hack spelling: count stepped inside the test, which tests R3 and stores
     * it in the BF/S's slot (CMP/GT R4,R3 at 0x8c0d458a). stepped apart, SHC
     * tests a copy, and 86 of 418 words differ */
    if (++trail.count > TRAIL_RINGS) {
        trail.count = TRAIL_RINGS;
    }
    if (frame_step == 0) {
        NJS_COLOR col;
        int j;

        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        for (i = 0; i < trail.count - 1; i++) {
            fade = (Float)i / (Float)trail.count;
            if (fade < 0.2F) {
                fade *= 0.5F / 0.2F;
            } else {
                fade = (1.0F - fade) * (0.5F / (1.0F - 0.2F));
            }
            col.argb.a = (Uint8)(fade * 256.0F);
            col.argb.r = 0xff;
            col.argb.b = col.argb.g = 0;
            point = trail_points;
            color = trail_colors;
            /* @hack type: the next ring's index cast to a short, which SHC
             * works out from i + 1 in a register of its own (ADD #1,R9 at
             * 0x8c0d4634, EXTS.W R9,R9 at 0x8c0d465c). uncast, 217 of 418
             * words differ */
            for (j = 0; j < TRAIL_POINTS; j++) {
                *point++ = trail.rings[i].points[j];
                *point++ = trail.rings[(Sint16)(i + 1)].points[j];
                *color++ = col;
                *color++ = col;
            }
            *point++ = trail.rings[i].points[0];
            *point = trail.rings[(Sint16)(i + 1)].points[0];
            *color++ = col;
            *color = col;
            njDrawTriangle3D(&trail_strip, STRIP_TRIANGLES,
                             NJD_TRANSPARENT | NJD_FILL | NJD_DRAW_CONNECTED);
        }
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_BOTHSRCALPHA);
    }
}
