/* the Egg Carrier's sky, and the puffs of cloud on its deck. the sky is a
 * dome over a sheet of cloud in flight, and a dome over the sea once the Egg
 * Carrier has sunk */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv0100/egg_carrier.h"
#include "adv0100/sky.h"
#include "adv0100/texlists.h"
#include "camera/camera.h"
#include "draw/draw.h"
#include "draw/sprite.h"
#include "draw/tree.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* the steps into the sea's ripple, which wrap to 0 at SEA_RIPPLES times
 * SEA_RIPPLE_STEPS. only this file reads it. it is the bss word at
 * 0x0cb2de4c, 4 bytes below near_clouds, the first of this file's own bss */
extern Sint32 sea_frame;
#ifdef GCC
/* ADV0100's tables place the word in bss no source holds, so the port
 * defines it here */
Sint32 sea_frame;
#endif

/* the flying_ objects show in flight, and the sea_ ones once the Egg Carrier
 * has sunk */
extern NJS_OBJECT flying_sky_object;
extern NJS_OBJECT flying_clouds_object;
extern NJS_OBJECT sea_sky_object;
extern NJS_OBJECT sea_object;

/* the sea's ripple shows SEA_RIPPLES textures of ec_sea_texlist in turn,
 * from SEA_RIPPLE_FIRST, each for SEA_RIPPLE_STEPS steps */
#define SEA_RIPPLES 10
#define SEA_RIPPLE_STEPS 4
#define SEA_RIPPLE_FIRST 4

/* the near and far clips each of the sky's draws leaves set */
#define CLIP_NEAR (-3.0F)
#define CLIP_FAR (-3000.0F)
/* the far clip the domes, the cloud sheet and the sea draw with */
#define SKY_CLIP_FAR (-10000.0F)

/* a texture's height in uv units. the cloud sheet's texture repeats
 * SHEET_REPEATS times along its v */
#define UV_HEIGHT 255
#define SHEET_REPEATS 8

struct sky_vars {
    Uint8 unk_00[8];
    /* the cloud sheet's scroll along v, from 0 to UV_HEIGHT - 1. it falls 2 a
     * step */
    Sint16 scroll;
};

#define VARS(work) ((struct sky_vars *)&(work)->vars)

enum sky_mode { SKY_INIT, SKY_RUN };

/* a puff of cloud, which cloud_draw draws as a sprite */
struct cloud {
    NJS_POINT3 pos; /* its place in its volume, from the volume's center */
    NJS_POINT3 at; /* its place in the world, which cloud_place sets */
    Float scale;
    Sint32 frame; /* its sprite's frame, from 0 to 2 */
    Angle ang; /* its sprite's turn */
    Float alpha;
    Angle blink; /* the angle whose sine scales its alpha */
};

/* puffs of cloud that fill a box about the camera. as the volume's origin
 * moves, its puffs move with it and wrap round the box */
struct cloud_volume {
    NJS_POINT3 origin;
    NJS_VECTOR offset; /* the origin less the camera's pos, wrapped into size */
    NJS_VECTOR size;
    struct cloud *clouds;
    Sint32 count;
    Float alpha; /* scales each puff's alpha */
};

/* the puffs in a volume */
#define CLOUDS 100
/* a volume's size along x and z, and along y */
#define VOLUME_WIDTH 800.0F
#define VOLUME_HEIGHT 200.0F

/* each step the volumes drift DRIFT along z, wrapping at VOLUME_WIDTH. they
 * sway SWAY either way along x and bob BOB up and down, by angles that turn
 * SWAY_TURN and BOB_TURN a step */
#define DRIFT 5.0F
#define SWAY 100.0F
#define SWAY_TURN 256
#define BOB 20.0F
#define BOB_TURN 512
/* each puff's blink turns BLINK_TURN a step */
#define BLINK_TURN 1024
/* cloud_draw skips a volume whose alpha is below FAINTEST, and a puff whose
 * alpha, times its volume's, is below it */
#define FAINTEST 0.01F
/* a puff's alpha goes by its distance from the camera. it is 0 within
 * CLOUD_NEAREST and rises to 1 at CLOUD_NEAR. it holds 1 out to CLOUD_FAR
 * and falls to 0 at CLOUD_FARTHEST, past which it stays 0 */
#define CLOUD_NEAREST 40.0F
#define CLOUD_NEAR 80.0F
#define CLOUD_FAR 200.0F
#define CLOUD_FARTHEST 400.0F

enum clouds_mode { CLOUDS_INIT, CLOUDS_RUN };

static void sky_destroy(struct task *task);
static void sky_draw(struct task *task);
static void sky_draw_flying(struct task_work *work, Float height);
static void sky_draw_at_sea(struct task_work *work, Float height);
static void sky_draw_cloud_sheet(struct task_work *work, Float height);
static void sky_draw_sea(struct task_work *work, Float height);
static void clouds_destroy(struct task *task);
static void clouds_draw(struct task *task);
static void cloud_scatter(struct cloud_volume *volume);
static void cloud_follow(struct cloud_volume *volume);
static void cloud_place(struct cloud_volume *volume);
static void cloud_draw(struct cloud_volume *volume);

static struct cloud near_clouds[CLOUDS];
static struct cloud far_clouds[CLOUDS];

static struct cloud_volume near_volume = {
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {VOLUME_WIDTH, VOLUME_HEIGHT, VOLUME_WIDTH},
    near_clouds,
    CLOUDS,
    1.0F};
static struct cloud_volume far_volume = {
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {VOLUME_WIDTH, VOLUME_HEIGHT, VOLUME_WIDTH},
    far_clouds,
    CLOUDS,
    1.0F};
static NJS_TEXANIM cloud_frames[3] = {
#include "adv0100/sky/cloud_frames.inc"
};
static NJS_SPRITE cloud_sprite = {
#include "adv0100/sky/cloud_sprite.inc"
};

/* the sky's step opens the module, at 0x0c920000, the address 1ST_READ.BIN's
 * sky_steps holds. #pragma section HEAD puts it in PHEAD, the module's first
 * section. each step advances the sea's ripple and the sheet's scroll, and
 * draws the sky */
#pragma section HEAD
void egg_carrier_sky_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage_act = stage_get_stage_act();

    switch (work->mode) {
    case SKY_INIT:
        sea_frame = 0;
        work->timer = 200;
        task->destructor = sky_destroy;
        task->draw = sky_draw;
        work->mode = SKY_RUN;
        break;
    case SKY_RUN:
        /* @hack redundant: a switch of six empty cases, which keeps the image's
         * CMP/EQ #0 to #5 at 0x0c920038 to 0x0c92004c; without it, 56 of 63
         * words differ */
        switch (stage_act & 0xff) {
        case 0:
            break;
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            break;
        case 5:
            break;
        }
        break;
    }
    sea_frame++;
    if (sea_frame >= SEA_RIPPLES * SEA_RIPPLE_STEPS) {
        sea_frame = 0;
    }
    VARS(work)->scroll -= 2;
    if (VARS(work)->scroll < 0) {
        VARS(work)->scroll += UV_HEIGHT;
    }
    sky_draw(task);
}
#pragma section

static void sky_destroy(struct task *task)
{
}

/* draw the sky by act. on the deck, acts 0 to 2, it is the dome over the
 * cloud sheet in flight. once the Egg Carrier has sunk, it is the dome over
 * the sea. the captain's room, act 3, has the dome alone. acts 4 and 5 have
 * none, and act 6 has the flying dome and sheet 3000 lower than the others */
static void sky_draw(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage_act = stage_get_stage_act();

    switch (stage_act & 0xff) {
    case 0:
        if (!egg_carrier_sunk) {
            sky_draw_flying(work, 0.0F);
            sky_draw_cloud_sheet(work, 0.0F);
        } else {
            sky_draw_at_sea(work, 0.0F);
            sky_draw_sea(work, 0.0F);
        }
        break;
    case 1:
        if (!egg_carrier_sunk) {
            sky_draw_flying(work, 0.0F);
            sky_draw_cloud_sheet(work, 0.0F);
        } else {
            sky_draw_at_sea(work, 0.0F);
            sky_draw_sea(work, 0.0F);
        }
        break;
    case 2:
        if (!egg_carrier_sunk) {
            sky_draw_flying(work, 0.0F);
            sky_draw_cloud_sheet(work, 0.0F);
        } else {
            sky_draw_at_sea(work, 0.0F);
            sky_draw_sea(work, 0.0F);
        }
        break;
    case 3:
        if (!egg_carrier_sunk) {
            sky_draw_flying(work, 0.0F);
        } else {
            sky_draw_at_sea(work, 0.0F);
        }
        break;
    case 4:
    case 5:
        break;
    case 6:
        sky_draw_flying(work, -3000.0F);
        sky_draw_cloud_sheet(work, -3000.0F);
        break;
    }
}

/* draw the flying sky's dome at height, centered on the camera's x and z,
 * with fog and depth cueing off */
static void sky_draw_flying(struct task_work *work, Float height)
{
    njClipZ(CLIP_NEAR, SKY_CLIP_FAR);
    draw_control_save();
    draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
    njFogDisable();
    njSetTexture(&ec_sky_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, camera_work->pos.x, height, camera_work->pos.z);
    draw_tree(&flying_sky_object);
    njPopMatrix(1);
    njFogEnable();
    draw_control_restore();
    njClipZ(CLIP_NEAR, CLIP_FAR);
}

/* draw the sky's dome at sea as sky_draw_flying draws the flying one */
static void sky_draw_at_sea(struct task_work *work, Float height)
{
    njClipZ(CLIP_NEAR, SKY_CLIP_FAR);
    draw_control_save();
    draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
    njFogDisable();
    njSetTexture(&ec_sea_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, camera_work->pos.x, height, camera_work->pos.z);
    draw_tree(&sea_sky_object);
    njPopMatrix(1);
    njFogEnable();
    draw_control_restore();
    njClipZ(CLIP_NEAR, CLIP_FAR);
}

/* draw the cloud sheet at height, centered on the camera's x and z. its uvs
 * scroll, and it is added onto what lies behind it */
static void sky_draw_cloud_sheet(struct task_work *work, Float height)
{
    NJS_OBJECT *object;
    NJS_COLOR *uv;
    int scroll;

    njClipZ(CLIP_NEAR, SKY_CLIP_FAR);
    object = &flying_clouds_object;
    uv = object->model->meshsets->vertuv;
    scroll = VARS(work)->scroll;
    uv[0].tex.v = scroll + (SHEET_REPEATS * UV_HEIGHT);
    uv[2].tex.v = scroll + (SHEET_REPEATS * UV_HEIGHT);
    uv[1].tex.v = scroll;
    uv[3].tex.v = scroll;
    njFogDisable();
    njSetTexture(&ec_sky_texlist);
    njPushMatrix(NULL);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njTranslate(NULL, camera_work->pos.x, height, camera_work->pos.z);
    njScale(NULL, 3.0F, 1.0F, 3.0F);
    draw_tree(object);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njPopMatrix(1);
    njFogEnable();
    njClipZ(CLIP_NEAR, CLIP_FAR);
}

/* draw the sea at height, centered on the camera's x and z, in the ripple's
 * texture */
static void sky_draw_sea(struct task_work *work, Float height)
{
    NJS_OBJECT *object;

    njClipZ(CLIP_NEAR, SKY_CLIP_FAR);
    object = &sea_object;
    object->model->mats->attr_texId =
        (sea_frame / SEA_RIPPLE_STEPS) + SEA_RIPPLE_FIRST;
    njFogDisable();
    njSetTexture(&ec_sea_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, camera_work->pos.x, height, camera_work->pos.z);
    njScale(NULL, 1.1F, 1.0F, 1.1F);
    draw_tree(object);
    njPopMatrix(1);
    njFogEnable();
    njClipZ(CLIP_NEAR, CLIP_FAR);
}

/* move both volumes and place and draw their puffs. the far volume bobs a
 * quarter turn ahead of the near one */
void egg_carrier_clouds_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage_act = stage_get_stage_act();
    Float z;
    Float x;

    switch (work->mode) {
    case CLOUDS_INIT:
        work->tick = stage_act;
        task->destructor = clouds_destroy;
        task->draw = clouds_draw;
        cloud_scatter(&near_volume);
        cloud_scatter(&far_volume);
        work->mode = CLOUDS_RUN;
        break;
    case CLOUDS_RUN:
        cloud_follow(&near_volume);
        cloud_place(&near_volume);
        cloud_draw(&near_volume);
        cloud_follow(&far_volume);
        cloud_place(&far_volume);
        cloud_draw(&far_volume);
        z = work->pos.z;
        z += DRIFT;
        if (z > VOLUME_WIDTH) {
            z -= VOLUME_WIDTH;
        }
        near_volume.origin.z = z;
        far_volume.origin.z = z;
        work->pos.z = z;
        x = njCos(work->ang.y) * SWAY;
        near_volume.origin.x = x;
        far_volume.origin.x = x;
        near_volume.alpha = 1.0F;
        far_volume.alpha = 1.0F;
        work->ang.y += SWAY_TURN;
        near_volume.origin.y = njSin(work->ang.x) * BOB;
        far_volume.origin.y = njCos(work->ang.x) * BOB;
        work->ang.x += BOB_TURN;
        break;
    }
}

static void clouds_destroy(struct task *task)
{
}

static void clouds_draw(struct task *task)
{
    cloud_draw(&near_volume);
    cloud_draw(&far_volume);
}

/* give each puff a random place in its volume, a scale from 1 up to 1.5, and
 * a random frame and turn. its blink starts at its turn */
static void cloud_scatter(struct cloud_volume *volume)
{
    int i;
    struct cloud *cloud = volume->clouds;

    for (i = 0; i < CLOUDS; i++, cloud++) {
        cloud->pos.x = ((rand() * RAND_SCALE) - 0.5F) * VOLUME_WIDTH;
        cloud->pos.y = ((rand() * RAND_SCALE) - 0.5F) * VOLUME_HEIGHT;
        cloud->pos.z = ((rand() * RAND_SCALE) - 0.5F) * VOLUME_WIDTH;
        cloud->scale = (rand() * (0.5F * RAND_SCALE)) + 1.0F;
        cloud->frame = (int)(rand() * (3.0F * RAND_SCALE));
        cloud->ang = (int)RANDOM_ANGLE();
        cloud->blink = cloud->ang;
    }
}

/* set the volume's offset: its origin less the camera's pos, wrapped into 0
 * up to VOLUME_WIDTH along x and z and VOLUME_HEIGHT along y */
static void cloud_follow(struct cloud_volume *volume)
{
    /* @hack spelling: origin read a Float at a time through a byte cursor, as
     * the image's post-increments read it; through the members, 62 of 62
     * words differ */
    char *bytes = (char *)&volume->origin;
    Float x;
    Float y;
    Float z;
    Float cx;
    Float cy;
    Float cz;
    Float *walk;

    x = *(Float *)bytes;
    bytes += sizeof(Float);
    y = *(Float *)bytes;
    bytes += sizeof(Float);
    z = *(Float *)bytes;
    /* @hack spelling: the camera's pos walked as Floats through VECTOR_FLOATS,
     * which gives cx and cy the image's registers; through the members, 55
     * of 62 words differ */
    walk = VECTOR_FLOATS(&camera_work->pos);
    cx = *walk++;
    cy = *walk++;
    cz = *walk;
    x -= cx;
    y -= cy;
    z -= cz;
    /* @hack spelling: each wrap negated and added, an FNEG and an FADD as at
     * 0x0c92db0a and 0x0c92db0c; with -=, 31 of 62 words differ */
    x += -floorf(x * (1.0F / VOLUME_WIDTH)) * VOLUME_WIDTH;
    y += -floorf(y * (1.0F / VOLUME_HEIGHT)) * VOLUME_HEIGHT;
    z += -floorf(z * (1.0F / VOLUME_WIDTH)) * VOLUME_WIDTH;
    /* @hack spelling: the offset stored a Float at a time through the byte
     * cursor, stepped down from its end, as the image's pre-decrements
     * store it; through the members, 38 of 62 words differ */
    bytes = (char *)(&volume->offset + 1);
    bytes -= sizeof(Float);
    *(Float *)bytes = z;
    bytes -= sizeof(Float);
    *(Float *)bytes = y;
    bytes -= sizeof(Float);
    *(Float *)bytes = x;
}

/* place each puff about the camera: its place in the volume, moved by the
 * offset and wrapped in the volume's size. its alpha goes by its distance
 * from the camera, then is scaled by |sin| of the puff's blink, which turns
 * BLINK_TURN a step */
static void cloud_place(struct cloud_volume *volume)
{
    int i;
    Float x;
    Float y;
    Float z;
    Float ox;
    Float oy;
    Float oz;
    Float sx;
    Float sy;
    Float sz;
    Float cx;
    Float cy;
    Float cz;
    Float alpha;
    Float *walk;
    struct cloud *cloud = volume->clouds;

    for (i = 0; i < volume->count; i++) {
        /* @hack spelling: each vector walked as Floats through VECTOR_FLOATS,
         * as cloud_follow walks the camera's, for the image's post-increment
         * loads and registers; through the members, the cloud's pos makes 37
         * of 154 words differ */
        walk = VECTOR_FLOATS(&cloud->pos);
        /* @hack spelling: the read of x and the cursor's step as two
         * statements, which keeps the image's registers for the offset's
         * loads from 0x0c92dbe6; folded into *walk++, 5 of 154 words
         * differ */
        x = *walk;
        walk++;
        y = *walk++;
        z = *walk;
        /* @hack spelling: the offset the same way; through the members, 36 of
         * 154 words differ */
        walk = VECTOR_FLOATS(&volume->offset);
        ox = *walk++;
        oy = *walk++;
        oz = *walk;
        x += ox;
        y += oy;
        z += oz;
        /* @hack spelling: the size the same way; through the members, 36 of 154
         * words differ */
        walk = VECTOR_FLOATS(&volume->size);
        sx = *walk++;
        sy = *walk++;
        sz = *walk;
        if (x > sx) {
            x -= sx;
        } else if (x < 0.0F) {
            x += sx;
        }
        if (y > sy) {
            y -= sy;
        } else if (y < 0.0F) {
            y += sy;
        }
        if (z > sz) {
            z -= sz;
        } else if (z < 0.0F) {
            z += sz;
        }
        x -= VOLUME_WIDTH / 2;
        y -= VOLUME_HEIGHT / 2;
        z -= VOLUME_WIDTH / 2;
        /* @hack spelling: the camera's pos the same way; through the members,
         * the function runs 6 bytes long and 108 of 154 words differ */
        walk = VECTOR_FLOATS(&camera_work->pos);
        cx = *walk++;
        cy = *walk++;
        cz = *walk;
        cx += x;
        cy += y;
        cz += z;
        /* @hack spelling: at stored through VECTOR_FLOATS, stepped down from
         * its end, as the image's pre-decrements store it; through the members,
         * the function runs 2 bytes long and 89 of 154 words differ */
        walk = VECTOR_FLOATS(&cloud->at) + 3;
        *--walk = cz;
        *--walk = cy;
        *--walk = cx;
        x *= x;
        y *= y;
        z *= z;
        x += y;
        x += z;
        x = njSqrt(x);
        if (x < CLOUD_NEAR) {
            if (x < CLOUD_NEAREST) {
                alpha = 0.0F;
            } else {
                alpha =
                    (x - CLOUD_NEAREST) * (1.0F / (CLOUD_NEAR - CLOUD_NEAREST));
            }
        } else if (x > CLOUD_FAR) {
            if (x > CLOUD_FARTHEST) {
                alpha = 0.0F;
            } else {
                alpha = (CLOUD_FARTHEST - x) *
                        (1.0F / (CLOUD_FARTHEST - CLOUD_FAR));
            }
        } else {
            alpha = 1.0F;
        }
        cloud->alpha = alpha * njAbs(njSin(cloud->blink));
        cloud->blink += BLINK_TURN;
        cloud++;
    }
}

/* draw the volume's puffs, each added onto what lies behind it. a puff's
 * sprite takes its scale, a fifth wider and a fifth shorter */
static void cloud_draw(struct cloud_volume *volume)
{
    NJS_ARGB material = {1.0F, 1.0F, 1.0F, 1.0F};
    int i;
    struct cloud *cloud;
    Float alpha;

    if (volume->alpha < FAINTEST) {
        return;
    }
    njFogDisable();
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    for (i = 0, cloud = volume->clouds; i < volume->count; i++, cloud++) {
        alpha = cloud->alpha * volume->alpha;
        if (alpha < FAINTEST) {
            continue;
        }
        material.a = alpha;
        njSetConstantMaterial(&material);
        cloud_sprite.p = cloud->at;
        cloud_sprite.sx = cloud->scale * 1.2F;
        cloud_sprite.sy = cloud->scale * 0.8F;
        cloud_sprite.ang = cloud->ang;
        sprite_draw_3d(&cloud_sprite, cloud->frame,
                       NJD_SPRITE_ANGLE | NJD_SPRITE_COLOR | NJD_SPRITE_SCALE |
                           NJD_SPRITE_ALPHA);
    }
    njFogEnable();
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}
