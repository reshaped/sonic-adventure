/* the Chao race's sprays of animated drops */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_race/spray.h"
#include "effect/splash.h"
#include "system/frame.h"
#include "system/util.h"

/* the splash's texture frames, its drops' scale, and their gravity and drag */
#define SPLASH_FRAMES 8
#define SPLASH_SCALE 0.2F
#define SPLASH_GRAVITY (-0.1F)
#define SPLASH_DRAG 0.0005F

/* the steps a waiting drop counts past before it flies */
#define SPLASH_WAIT 8
/* the most a new drop's heading turns from the spray's velocity, about each
 * of two axes */
#define SPLASH_TURN NJM_DEG_ANG(10)

static void splash_init(struct spray_drop *drop);
static void splash_move(struct spray_drop *drop, NJS_POINT3 *pos,
                        NJS_VECTOR *speed);

static const NJS_TEXANIM splash_texanims[SPLASH_FRAMES] = {
#include "al_race/spray/splash_texanims.inc"
};

void spray_init(struct spray *spray, struct spray_kind *kind,
                void (*init)(struct spray_drop *drop), spray_move move)
{
    int i;

    spray->texlist = kind->texlist;
    spray->texanim = kind->texanim;
    spray->scale = kind->scale;
    spray->frames = kind->frames;
    spray->count = kind->count < SPRAY_DROPS ? kind->count : SPRAY_DROPS;
    spray->move = move;
    if (init != NULL) {
        for (i = 0; i < spray->count; i++) {
            init(&spray->drops[i]);
        }
    }
}

void spray_step(struct spray *spray, NJS_POINT3 *pos, NJS_VECTOR *speed)
{
    int i;
    struct spray_drop *drop;

    for (i = 0; i < spray->count; i++) {
        drop = &spray->drops[i];
        drop->flags &= ~DROP_LOOPED;
        if (drop->mode == DROP_FLYING || drop->mode == DROP_FLYING_HIDDEN) {
            drop->frame_timer++;
            if (drop->frame_timer >= spray->frame_steps) {
                drop->frame_timer = 0;
                if (++drop->frame >= spray->frames) {
                    drop->frame = 0;
                    drop->flags |= DROP_LOOPED;
                }
            }
        }
        if (spray->move != NULL) {
            spray->move(drop, pos, speed);
        }
        drop->timer++;
        drop->flags &= DROP_KEPT;
    }
}

void spray_draw(struct spray *spray)
{
    int i;
    NJS_SPRITE sprite;
    struct spray_drop *drop;

    if (frame_step != 0) {
        return;
    }
    sprite.sx = spray->scale;
    sprite.sy = spray->scale;
    sprite.ang = 0;
    sprite.tlist = spray->texlist;
    sprite.tanim = spray->texanim;
    njSetTexture(spray->texlist);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    for (i = 0; i < spray->count; i++) {
        drop = &spray->drops[i];
        if (drop->mode == DROP_SHOWN || drop->mode == DROP_FLYING) {
            sprite.p.x = drop->pos.x;
            sprite.p.y = drop->pos.y;
            sprite.p.z = drop->pos.z;
            njDrawSprite3D(&sprite, drop->frame,
                           NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
        }
    }
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
}

void spray_set_move(struct spray *spray, spray_move move)
{
    spray->move = move;
}

void spray_set_flags(struct spray *spray, int flags)
{
    int i;
    struct spray_drop *drop;

    for (i = 0; i < spray->count; i++) {
        drop = &spray->drops[i];
        drop->flags |= flags;
    }
}

void spray_fall(struct spray_drop *drop, Float gravity, Float drag)
{
    NJS_VECTOR slow;

    drop->speed.y += gravity;
    /* @hack spelling: slow through (&slow)->, which forms its address from R15
     * at each use (MOV R15,R3 at 0x0cb8b274 to 0x0cb8b2b4); as slow.x and
     * the rest, the function runs 32 bytes short and 53 of 65 words differ */
    (&slow)->x = drop->speed.x * drag;
    (&slow)->y = drop->speed.y * drag;
    (&slow)->z = drop->speed.z * drag;
    drop->speed.x -= (&slow)->x;
    drop->speed.y -= (&slow)->y;
    drop->speed.z -= (&slow)->z;
    drop->pos.x += drop->speed.x;
    drop->pos.y += drop->speed.y;
    drop->pos.z += drop->speed.z;
}

void spray_init_splash(struct spray *spray)
{
    struct spray_kind kind;

    kind.texlist = &splash_texlist;
    kind.texanim = (NJS_TEXANIM *)splash_texanims;
    kind.scale = SPLASH_SCALE;
    kind.frame_steps = 8;
    kind.frames = SPLASH_FRAMES;
    kind.count = 3;
    spray_init(spray, &kind, splash_init, splash_move);
}

static void splash_init(struct spray_drop *drop)
{
    drop->mode = DROP_WAIT;
    drop->frame = 0;
    drop->frame_timer = 0;
    drop->timer = RANDOM(0.0F, 15.0F);
}

static void splash_move(struct spray_drop *drop, NJS_POINT3 *pos,
                        NJS_VECTOR *speed)
{
    Angle x;
    Angle y;

    switch (drop->mode) {
    case DROP_WAIT:
        if (!(drop->flags & DROP_SPRAYING)) {
            drop->timer = RANDOM(0.0F, 8.0F);
            break;
        }
        if (drop->timer > SPLASH_WAIT) {
            drop->mode = DROP_FLYING;
            drop->timer = 0;
        } else {
            break;
        }
        /* fallthrough */
    case DROP_FLYING:
        if (drop->timer == 0 && pos != NULL && speed != NULL) {
            drop->pos.x = pos->x;
            drop->pos.y = pos->y;
            drop->pos.z = pos->z;
            y = RANDOM(-SPLASH_TURN, SPLASH_TURN);
            x = RANDOM(-SPLASH_TURN, SPLASH_TURN);
            njPushMatrix(NULL);
            njUnitMatrix(NULL);
            njRotateXYZ(NULL, x, y, 0);
            njCalcVector(NULL, speed, &drop->speed);
            njPopMatrix(1);
        }
        if (drop->flags & DROP_LOOPED) {
            drop->timer = RANDOM(0.0F, 8.0F);
            drop->mode = DROP_WAIT;
        }
        spray_fall(drop, SPLASH_GRAVITY, SPLASH_DRAG);
        break;
    }
}
