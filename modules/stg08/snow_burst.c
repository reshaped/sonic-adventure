/* snow bursts that the avalanche's rocks throw up where they land */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "draw/draw.h"
#include "stg08/snow_burst.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST yukibaku_texlist;

/* a burst plays its ten images at a quarter of an image a step. it ends
 * after it shows the last */
#define BURST_IMAGES 10
#define BURST_LAST (BURST_IMAGES - 1.0F)
#define BURST_RATE 0.25F
/* while the sprite shows images 1 to BURST_STRETCH_LAST, the burst is drawn
 * taller. its height scales by BURST_STRETCH, less BURST_SQUASH for each
 * image played */
#define BURST_STRETCH_LAST 7
#define BURST_STRETCH 2.6F
#define BURST_SQUASH 0.2F

#define BURST_SIZE 5.0F

struct snow_burst_vars {
    Uint8 unk_00[4];
    Float image; /* how far the burst has played, in images */
};

#define VARS(work) ((struct snow_burst_vars *)&(work)->vars)

static NJS_TEXANIM snow_burst_texanims[BURST_IMAGES] = {
#include "stg08/snow_burst/snow_burst_texanims.inc"
};

/* the sprite all bursts draw, set up again as each burst is created */
static NJS_SPRITE snow_burst_sprite;

/* the burst's step, which also draws it: once a frame, on its first step */
static void snow_burst_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint32 image = (Uint32)VARS(work)->image;

    if (VARS(work)->image < BURST_LAST) {
        VARS(work)->image += BURST_RATE;
    } else {
        VARS(work)->image = BURST_LAST;
        task_kill(task);
    }
    if (frame_step == 0) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njSetTexture(&yukibaku_texlist);
        njFogDisable();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, camera_work->ang.y);
        TURN(njRotateX, camera_work->ang.x);
        TURN(njRotateZ, work->ang.z);
        if (image == 0 || image > BURST_STRETCH_LAST) {
            njScale(NULL, 1.0F, 1.0F, 1.0F);
        } else {
            njScale(NULL, 1.0F,
                    (-VARS(work)->image * BURST_SQUASH) + BURST_STRETCH, 1.0F);
        }
        njDrawSprite3D(&snow_burst_sprite, image,
                       NJD_SPRITE_VFLIP | NJD_SPRITE_ALPHA);
        njPopMatrix(1);
        njFogEnable();
    }
}

void snow_burst_create(NJS_POINT3 *pos)
{
    struct task *task = task_create(TASK_WORK, 6, snow_burst_step);

    task->work->pos = *pos;
    snow_burst_sprite.tlist = &yukibaku_texlist;
    snow_burst_sprite.tanim = snow_burst_texanims;
    snow_burst_sprite.sx = BURST_SIZE;
    snow_burst_sprite.sy = BURST_SIZE;
    snow_burst_sprite.p.x = 0.0F;
    snow_burst_sprite.p.y = 0.0F;
    snow_burst_sprite.p.z = 0.0F;
}
