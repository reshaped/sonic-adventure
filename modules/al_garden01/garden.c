/* the Egg Carrier's garden: the stage's task */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_garden01/garden.h"
#include "al_garden01/sky_textures.h"
#include "al_main/alife.h"
#include "al_main/camera.h"
#include "al_main/chao.h"
#include "camera/camera.h"
#include "chao/chao.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT egg_carrier_garden_sky_object;
extern NJS_MATERIAL sea_materials[];
extern NJS_MATERIAL sea_materials_2[];
extern NJS_MATERIAL sea_materials_3[];
extern NJS_MATERIAL wave_materials[];

/* the textures sea_materials and wave_materials cycle through.
 * sea_materials_2 and sea_materials_3 don't cycle: each takes SEA_FIRST as
 * sea_materials wraps */
#define SEA_FIRST 54
#define SEA_LAST 63
#define WAVE_FIRST 64
#define WAVE_LAST 78
/* how far the sea and the waves move toward their next texture a step */
#define SEA_SPEED 0.3F
#define WAVE_SPEED 0.2F

/* the field of view the start gives camera_set_perspective, which
 * garden_camera_step soon replaces with the zoom's: 30, 49 or 68 degrees */
#define GARDEN_PERSPECTIVE NJM_DEG_ANG(80)
#define PERSPECTIVE NJM_DEG_ANG(70) /* the one GARDEN_END would put back */

/* no code sets GARDEN_END, so egg_carrier_garden_end never runs */
enum garden_mode { GARDEN_INIT, GARDEN_WAIT, GARDEN_RUN, GARDEN_END };

/* how far the sea and the waves are toward their next texture. each takes
 * its next texture once it passes 1 */
struct garden_vars {
    Float sea;
    Float wave;
};

#define VARS(work) ((struct garden_vars *)&(work)->vars)

static void egg_carrier_garden_start(struct task *task);
static void egg_carrier_garden_end(struct task *task);
static void egg_carrier_garden_sky_draw(struct task *task);

static void egg_carrier_garden_sky_load(struct task *task)
{
    /* @hack redundant: work, which nothing reads. inlined into
     * egg_carrier_garden_step, it keeps the image's MOV R13,R4 at 0x0cb8002c,
     * a copy of task that the call's own arguments replace; without it, 100
     * of 118 words differ */
    struct task_work *work = task->work;

    texture_load_pvm("garden01_sky", &egg_carrier_garden_sky_texlist);
}

void egg_carrier_garden_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case GARDEN_INIT:
        alife_create();
        egg_carrier_garden_start(task);
        egg_carrier_garden_sky_load(task);
        task->draw = egg_carrier_garden_sky_draw;
        work->mode++;
        work->timer = 0;
        break;
    case GARDEN_WAIT:
        if (TASK_TIMER(work)++ > 3) {
            camera_override_high(garden_camera_step, 1, CAMERA_LOOK_AT);
            work->mode++;
        }
        break;
    case GARDEN_RUN:
        egg_carrier_garden_sky_draw(task);
        if (VARS(work)->sea > 1.0F) {
            sea_materials->attr_texId++;
            if (sea_materials->attr_texId > SEA_LAST) {
                sea_materials->attr_texId = SEA_FIRST;
                sea_materials_2->attr_texId = sea_materials->attr_texId;
                sea_materials_3->attr_texId = sea_materials->attr_texId;
            }
            VARS(work)->sea = 0.0F;
        }
        if (VARS(work)->wave > 1.0F) {
            wave_materials->attr_texId++;
            if (wave_materials->attr_texId > WAVE_LAST) {
                wave_materials->attr_texId = WAVE_FIRST;
            }
            VARS(work)->wave = 0.0F;
        }
        VARS(work)->sea += SEA_SPEED;
        VARS(work)->wave += WAVE_SPEED;
        break;
    case GARDEN_END:
        egg_carrier_garden_end(task);
        break;
    }
}

static void egg_carrier_garden_start(struct task *task)
{
    chao_gardens_init();
    njSetBackColor(0x5555ff, 0x5555ff, 0xffffff);
    alife_chao_create_garden(GARDEN_EGG_CARRIER);
    camera_set_perspective(GARDEN_PERSPECTIVE);
    stream_reset();
    stream_request_music(MUSIC_CHAO);
}

static void egg_carrier_garden_end(struct task *task)
{
    sound_stop_id((int)task);
    camera_set_perspective(PERSPECTIVE);
    njReleaseTexture(&egg_carrier_garden_sky_texlist);
}

/* set the work's submode to the time of day, and on a frame's first step draw
 * the sky round the camera, without fog */
static void egg_carrier_garden_sky_draw(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = story_get_time();
    if (frame_step == 0) {
        njSetTexture(&egg_carrier_garden_sky_texlist);
        draw_control_save();
        draw_control_set(NJD_CONTROL_3D_NO_CLIP_CHECK);
        draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
        njFogDisable();
        njPushMatrix(NULL);
        njTranslate(NULL, camera_work->pos.x, -62.5F, camera_work->pos.z);
        njScale(NULL, 0.15F, 0.15F, 0.15F);
        njSimpleDrawObject(&egg_carrier_garden_sky_object);
        njFogEnable();
        njPopMatrix(1);
        draw_control_restore();
    }
}
