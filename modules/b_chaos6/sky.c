/* Chaos 6's sky: a dome over a sheet of cloud whose texture scrolls, both
 * centered on the camera's x and z. puffs of cloud stream past the deck
 * from two volumes, each kept in a box about the camera */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos6/sky.h"
#include "camera/camera.h"
#include "draw/draw.h"
#include "stage/stage.h"
#ifdef GCC
#include "system/frame.h"
#endif
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 13
#define TEXT_6_LENGTH 13
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 10
#define TEXT_9_LENGTH 10
#include "b_chaos6/sky/text.inc"

/* the near and far clips each of the sky's draws leaves set */
#define CLIP_NEAR (-3.0F)
#define CLIP_FAR (-3000.0F)
/* the far clip the dome and the sheet draw with */
#define SKY_CLIP_FAR (-10000.0F)

/* a texture's height in uv units. the sheet's texture repeats
 * SHEET_REPEATS times along its v, and the sheet lies SHEET_HEIGHT up */
#define UV_HEIGHT 255
#define SHEET_REPEATS 8
#define SHEET_HEIGHT 2300.0F

struct sky_vars {
    Uint8 unk_00[8];
    /* the sheet's scroll along v, from 0 to UV_HEIGHT - 1. it falls 2 a
     * step */
    Sint16 scroll;
};

#define VARS(work) ((struct sky_vars *)&(work)->vars)

enum sky_mode { SKY_INIT, SKY_RUN };

/* a puff of cloud, which chaos6_cloud_draw draws as a sprite */
struct cloud {
    NJS_POINT3 pos; /* its place in its volume, from the volume's center */
    NJS_POINT3 at; /* its place in the world, which chaos6_cloud_place sets */
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
#define CLOUDS 50
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
/* chaos6_cloud_draw skips a volume whose alpha is below FAINTEST, and a puff
 * whose alpha, times its volume's, is below it */
#define FAINTEST 0.01F
/* a puff's alpha goes by its distance from the camera. it is 0 within
 * CLOUD_NEAREST and rises to 1 at CLOUD_NEAR. it holds 1 out to CLOUD_FAR
 * and falls to 0 at CLOUD_FARTHEST, past which it stays 0 */
#define CLOUD_NEAREST 40.0F
#define CLOUD_NEAR 80.0F
#define CLOUD_FAR 200.0F
#define CLOUD_FARTHEST 400.0F

enum clouds_mode { CLOUDS_INIT, CLOUDS_RUN };

static void chaos6_sky_destroy(struct task *task);
static void chaos6_sky_draw(struct task *task);
static void chaos6_sky_draw_dome(struct task_work *work);
static void chaos6_sky_draw_cloud_sheet(struct task_work *work);
static void chaos6_clouds_destroy(struct task *task);
static void chaos6_clouds_draw(struct task *task);
static void chaos6_cloud_scatter(struct cloud_volume *volume);
static void chaos6_cloud_follow(struct cloud_volume *volume);
static void chaos6_cloud_place(struct cloud_volume *volume);
static void chaos6_cloud_draw(struct cloud_volume *volume);

static NJS_MATERIAL cloud_sheet_materials[1] = {
#include "b_chaos6/sky/cloud_sheet_materials.inc"
};

static Sint16 cloud_sheet_meshes[6] = {
#include "b_chaos6/sky/cloud_sheet_meshes.inc"
};

static NJS_COLOR cloud_sheet_uvs[4] = {
#include "b_chaos6/sky/cloud_sheet_uvs.inc"
};

static NJS_MESHSET cloud_sheet_meshsets[1] = {
#include "b_chaos6/sky/cloud_sheet_meshsets.inc"
};

static NJS_POINT3 cloud_sheet_points[4] = {
#include "b_chaos6/sky/cloud_sheet_points.inc"
};

static NJS_VECTOR cloud_sheet_normals[4] = {
#include "b_chaos6/sky/cloud_sheet_normals.inc"
};

static NJS_MODEL cloud_sheet_model = {
#include "b_chaos6/sky/cloud_sheet_model.inc"
};

static NJS_OBJECT cloud_sheet_object = {
#include "b_chaos6/sky/cloud_sheet_object.inc"
};

static NJS_MATERIAL sky_dome_materials[3] = {
#include "b_chaos6/sky/sky_dome_materials.inc"
};

static Sint16 sky_dome_meshes0[80] = {
#include "b_chaos6/sky/sky_dome_meshes0.inc"
};

static Sint16 sky_dome_meshes1[34] = {
#include "b_chaos6/sky/sky_dome_meshes1.inc"
};

static Sint16 sky_dome_meshes2[140] = {
#include "b_chaos6/sky/sky_dome_meshes2.inc"
};

static NJS_COLOR sky_dome_uvs0[64] = {
#include "b_chaos6/sky/sky_dome_uvs0.inc"
};

static NJS_COLOR sky_dome_uvs1[28] = {
#include "b_chaos6/sky/sky_dome_uvs1.inc"
};

static NJS_COLOR sky_dome_uvs2[136] = {
#include "b_chaos6/sky/sky_dome_uvs2.inc"
};

static NJS_MESHSET sky_dome_meshsets[3] = {
#include "b_chaos6/sky/sky_dome_meshsets.inc"
};

static NJS_POINT3 sky_dome_points[97] = {
#include "b_chaos6/sky/sky_dome_points.inc"
};

static NJS_VECTOR sky_dome_normals[97] = {
#include "b_chaos6/sky/sky_dome_normals.inc"
};

static NJS_MODEL sky_dome_model = {
#include "b_chaos6/sky/sky_dome_model.inc"
};

static NJS_OBJECT sky_dome_object = {
#include "b_chaos6/sky/sky_dome_object.inc"
};

/* the textures chaos6_pvms loads from CHAOS6_BG, the same nine as the Egg
 * Carrier's ec_sky_texlist. the sheet takes texture 0, the dome 1 to 3 and
 * the puffs 6 to 8. no draw in this file takes 4 and 5, stg_s_bkfirec and
 * stg_s_bkfired */
static NJS_TEXNAME chaos6_bg_textures[9] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0}, {TEXT_6, 0, 0},
    {TEXT_7, 0, 0}, {TEXT_8, 0, 0}, {TEXT_9, 0, 0},
};
NJS_TEXLIST chaos6_bg_texlist = {
#include "b_chaos6/sky/chaos6_bg_texlist.inc"
};

static struct cloud chaos6_near_clouds[CLOUDS];
static struct cloud chaos6_far_clouds[CLOUDS];

static struct cloud_volume chaos6_near_volume = {
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {VOLUME_WIDTH, VOLUME_HEIGHT, VOLUME_WIDTH},
    chaos6_near_clouds,
    CLOUDS,
    1.0F};
static struct cloud_volume chaos6_far_volume = {
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {VOLUME_WIDTH, VOLUME_HEIGHT, VOLUME_WIDTH},
    chaos6_far_clouds,
    CLOUDS,
    1.0F};
static NJS_TEXANIM chaos6_cloud_frames[3] = {
#include "b_chaos6/sky/chaos6_cloud_frames.inc"
};
static NJS_SPRITE chaos6_cloud_sprite = {
#include "b_chaos6/sky/chaos6_cloud_sprite.inc"
};

/* under section HEAD, SHC writes the step to section PHEAD, which leads the
 * module. so the step lies at 0x0c9000a0, where 1ST_READ.BIN's sky_steps
 * reaches it. every step scrolls the sheet, and each after the first draws
 * the sky as chaos6_sky_draw does */
#pragma section HEAD
void chaos6_sky_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage_act = stage_get_stage_act();

    switch (work->mode) {
    case SKY_INIT:
        task->destructor = chaos6_sky_destroy;
        task->draw = chaos6_sky_draw;
        work->mode = SKY_RUN;
        break;
    case SKY_RUN:
#ifdef GCC
        /* the second picture's pass draws the sky by chaos6_sky_draw. step
         * 1 of a split frame draws none, so that the picture adds the sheet
         * once */
        if (frame_split && frame_step == 1) {
            break;
        }
#endif
        switch (stage_act & 0xff) {
        case 0:
            chaos6_sky_draw_dome(work);
            chaos6_sky_draw_cloud_sheet(work);
            break;
        case 1:
            chaos6_sky_draw_dome(work);
            chaos6_sky_draw_cloud_sheet(work);
            break;
        case 2:
            chaos6_sky_draw_dome(work);
            chaos6_sky_draw_cloud_sheet(work);
            break;
        case 3:
            chaos6_sky_draw_dome(work);
            break;
        case 4:
        case 5:
            break;
        }
        break;
    }
    VARS(work)->scroll -= 2;
    if (VARS(work)->scroll < 0) {
        VARS(work)->scroll += UV_HEIGHT;
    }
}
#pragma section

static void chaos6_sky_destroy(struct task *task)
{
}

/* the task's draw, which task_draw_all runs while the game is paused. it
 * draws the dome and then the sheet in acts 0 to 2, and the dome alone in
 * act 3. acts 4 and 5 have no sky */
static void chaos6_sky_draw(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage_act = stage_get_stage_act();

    switch (stage_act & 0xff) {
    case 0:
        chaos6_sky_draw_dome(work);
        chaos6_sky_draw_cloud_sheet(work);
        break;
    case 1:
        chaos6_sky_draw_dome(work);
        chaos6_sky_draw_cloud_sheet(work);
        break;
    case 2:
        chaos6_sky_draw_dome(work);
        chaos6_sky_draw_cloud_sheet(work);
        break;
    case 3:
        chaos6_sky_draw_dome(work);
        break;
    case 4:
    case 5:
        break;
    }
}

/* draw the dome centered on the camera's x and z, with fog and depth cueing
 * off */
static void chaos6_sky_draw_dome(struct task_work *work)
{
    njClipZ(CLIP_NEAR, SKY_CLIP_FAR);
    draw_control_save();
    draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
    njFogDisable();
    njSetTexture(&chaos6_bg_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, camera_work->pos.x, 0.0F, camera_work->pos.z);
    njRotateY(NULL, -0x2000);
    njSimpleDrawObject(&sky_dome_object);
    njPopMatrix(1);
    njFogEnable();
    draw_control_restore();
    njClipZ(CLIP_NEAR, CLIP_FAR);
}

/* draw the sheet SHEET_HEIGHT up, centered on the camera's x and z, its v
 * scrolled by scroll. it is added onto what lies behind it */
static void chaos6_sky_draw_cloud_sheet(struct task_work *work)
{
    NJS_OBJECT *object;
    NJS_COLOR *uv;
    int scroll;

    njClipZ(CLIP_NEAR, SKY_CLIP_FAR);
    object = &cloud_sheet_object;
    uv = object->model->meshsets->vertuv;
    scroll = VARS(work)->scroll;
    uv[0].tex.v = scroll + (SHEET_REPEATS * UV_HEIGHT);
    uv[2].tex.v = scroll + (SHEET_REPEATS * UV_HEIGHT);
    uv[1].tex.v = scroll;
    uv[3].tex.v = scroll;
    njFogDisable();
    njSetTexture(&chaos6_bg_texlist);
    njPushMatrix(NULL);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    draw_set_material(0.5F, 0.0F, 0.0F, 0.0F);
    njTranslate(NULL, camera_work->pos.x, SHEET_HEIGHT, camera_work->pos.z);
    njScale(NULL, 3.0F, 1.0F, 3.0F);
    njSimpleDrawObject(&cloud_sheet_object);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njPopMatrix(1);
    njFogEnable();
    njClipZ(CLIP_NEAR, CLIP_FAR);
}

/* move both volumes, and place and draw their puffs. the far volume bobs a
 * quarter turn ahead of the near one */
void chaos6_clouds_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage_act = stage_get_stage_act();
    Float z;
    Float x;

    switch (work->mode) {
    case CLOUDS_INIT:
        work->tick = stage_act;
        task->destructor = chaos6_clouds_destroy;
        task->draw = chaos6_clouds_draw;
        chaos6_cloud_scatter(&chaos6_near_volume);
        chaos6_cloud_scatter(&chaos6_far_volume);
        work->mode = CLOUDS_RUN;
        break;
    case CLOUDS_RUN:
        if (work->tick != (stage_act & 0xff)) {
            task_kill(task);
            return;
        }
        chaos6_cloud_follow(&chaos6_near_volume);
        chaos6_cloud_place(&chaos6_near_volume);
        chaos6_cloud_draw(&chaos6_near_volume);
        chaos6_cloud_follow(&chaos6_far_volume);
        chaos6_cloud_place(&chaos6_far_volume);
        chaos6_cloud_draw(&chaos6_far_volume);
        z = work->pos.z;
        z += DRIFT;
        if (z > VOLUME_WIDTH) {
            z -= VOLUME_WIDTH;
        }
        chaos6_near_volume.origin.z = z;
        chaos6_far_volume.origin.z = z;
        work->pos.z = z;
        x = njCos(work->ang.y) * SWAY;
        chaos6_near_volume.origin.x = x;
        chaos6_far_volume.origin.x = x;
        chaos6_near_volume.alpha = 1.0F;
        chaos6_far_volume.alpha = 1.0F;
        work->ang.y += SWAY_TURN;
        chaos6_near_volume.origin.y = njSin(work->ang.x) * BOB;
        chaos6_far_volume.origin.y = njCos(work->ang.x) * BOB;
        work->ang.x += BOB_TURN;
        break;
    }
}

static void chaos6_clouds_destroy(struct task *task)
{
}

/* the task's draw, which task_draw_all runs while the game is paused. it
 * draws the near volume alone, where each step draws both */
static void chaos6_clouds_draw(struct task *task)
{
    chaos6_cloud_draw(&chaos6_near_volume);
}

/* give each puff a random place in its volume, a scale from 1 up to 1.2, and
 * a random frame and turn. its blink starts at its turn */
static void chaos6_cloud_scatter(struct cloud_volume *volume)
{
    int i;
    struct cloud *cloud = volume->clouds;

    for (i = 0; i < CLOUDS; i++, cloud++) {
        cloud->pos.x = ((rand() * RAND_SCALE) - 0.5F) * VOLUME_WIDTH;
        cloud->pos.y = ((rand() * RAND_SCALE) - 0.5F) * VOLUME_HEIGHT;
        cloud->pos.z = ((rand() * RAND_SCALE) - 0.5F) * VOLUME_WIDTH;
        cloud->scale = (rand() * (0.2F * RAND_SCALE)) + 1.0F;
        cloud->frame = (int)(rand() * (3.0F * RAND_SCALE));
        cloud->ang = (int)RANDOM_ANGLE();
        cloud->blink = cloud->ang;
    }
}

/* set the volume's offset: its origin less the camera's pos, wrapped into 0
 * up to VOLUME_WIDTH along x and z and VOLUME_HEIGHT along y */
static void chaos6_cloud_follow(struct cloud_volume *volume)
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
     * 0x0c9075ca and 0x0c9075cc; with -=, 31 of 62 words differ */
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
static void chaos6_cloud_place(struct cloud_volume *volume)
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
         * as chaos6_cloud_follow walks the camera's, for the image's
         * post-increment loads and registers; through the members, the
         * cloud's pos makes 37 of 154 words differ */
        walk = VECTOR_FLOATS(&cloud->pos);
        /* @hack spelling: the read of x and the cursor's step as two
         * statements, which keeps the image's registers for the offset's
         * loads from 0x0c9076a6; folded into *walk++, 5 of 154 words
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
static void chaos6_cloud_draw(struct cloud_volume *volume)
{
    NJS_ARGB material = {1.0F, 1.0F, 1.0F, 1.0F};
    int i;
    struct cloud *cloud;
    Float alpha;

    if (volume->alpha < FAINTEST) {
        return;
    }
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
        chaos6_cloud_sprite.p = cloud->at;
        chaos6_cloud_sprite.sx = cloud->scale * 1.2F;
        chaos6_cloud_sprite.sy = cloud->scale * 0.8F;
        chaos6_cloud_sprite.ang = cloud->ang;
        njDrawSprite3D(&chaos6_cloud_sprite, cloud->frame,
                       NJD_SPRITE_ANGLE | NJD_SPRITE_COLOR | NJD_SPRITE_SCALE |
                           NJD_SPRITE_ALPHA);
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}
