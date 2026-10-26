/* the Mystic Ruins garden: the stage's task */
#include <ninja.h>

#include "al_garden02/garden.h"
#include "al_garden02/sky_night_object_3.h"
#include "al_main/alife.h"
#include "al_main/camera.h"
#include "al_main/chao.h"
#include "camera/camera.h"
#include "chao/chao.h"
#include "draw/light.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the uvs of the waterfall's and the stream's models, and how far the
 * step scrolls each uv's v */
#define WATERFALL_UVS 48
#define STREAM_UVS 112
#define WATERFALL_SCROLL 6
#define STREAM_SCROLL 1

extern NJS_COLOR waterfall_uvs[WATERFALL_UVS];
extern NJS_COLOR stream_uvs[STREAM_UVS];
extern NJS_TEXLIST sky_day_texlist;
extern NJS_OBJECT sky_day_object;
extern NJS_OBJECT sky_day_object_2;
extern NJS_TEXLIST sky_evening_texlist;
extern NJS_OBJECT sky_evening_object;
extern NJS_OBJECT sky_evening_object_2;
extern NJS_OBJECT sky_evening_object_3;
extern NJS_TEXLIST sky_night_texlist;
extern NJS_OBJECT sky_night_object;
extern NJS_OBJECT sky_night_object_2;

/* the field of view the start gives camera_set_perspective, which
 * garden_camera_step soon replaces with the zoom's: 30, 49 or 68 degrees */
#define GARDEN_PERSPECTIVE NJM_DEG_ANG(80)
#define PERSPECTIVE NJM_DEG_ANG(70) /* the one the destructor puts back */

#define CEILING 80.0F /* the step holds player 0's y at or below this */

enum garden_mode { GARDEN_INIT, GARDEN_WAIT, GARDEN_RUN };

static void mystic_ruins_garden_start(struct task *task);
static void mystic_ruins_garden_destroy(struct task *task);
static void mystic_ruins_garden_sky_load(struct task *task);
static void mystic_ruins_garden_sky_draw(struct task *task);
static void mystic_ruins_garden_light_load(struct task *task);

void mystic_ruins_garden_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_COLOR *uv;

    switch (work->mode) {
    case GARDEN_INIT:
        alife_create();
        mystic_ruins_garden_start(task);
        task->destructor = mystic_ruins_garden_destroy;
        mystic_ruins_garden_sky_load(task);
        task->draw = mystic_ruins_garden_sky_draw;
        mystic_ruins_garden_light_load(task);
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
        mystic_ruins_garden_sky_draw(task);
        for (uv = waterfall_uvs; uv < &waterfall_uvs[WATERFALL_UVS]; uv++) {
            uv->tex.v -= WATERFALL_SCROLL;
        }
        for (uv = stream_uvs; uv < &stream_uvs[STREAM_UVS]; uv++) {
            uv->tex.v -= STREAM_SCROLL;
        }
        if (player_work[0]->pos.y > CEILING) {
            player_work[0]->pos.y = CEILING;
        }
        break;
    }
}

static void mystic_ruins_garden_start(struct task *task)
{
    chao_gardens_init();
    alife_chao_create_garden(GARDEN_MYSTIC_RUINS);
    camera_set_perspective(GARDEN_PERSPECTIVE);
    stream_reset();
    stream_request_music(MUSIC_CHAO);
}

static void mystic_ruins_garden_destroy(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = story_get_time();
    switch (work->submode) {
    case TIME_DAY:
        njReleaseTexture(&sky_day_texlist);
        break;
    case TIME_EVENING:
        njReleaseTexture(&sky_evening_texlist);
        break;
    case TIME_NIGHT:
        njReleaseTexture(&sky_night_texlist);
    }
    camera_set_perspective(PERSPECTIVE);
    camera_restore();
    njFogDisable();
    stream_destroy();
}

static void mystic_ruins_garden_sky_load(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = story_get_time();
    switch (work->submode) {
    case TIME_DAY:
        texture_load_pvm("garden_mr_sky_hiru", &sky_day_texlist);
        break;
    case TIME_EVENING:
        texture_load_pvm("garden_mr_sky_yuu", &sky_evening_texlist);
        break;
    case TIME_NIGHT:
        texture_load_pvm("garden_mr_sky_yoru", &sky_night_texlist);
        break;
    }
}

/* set the work's submode to the time of day, and on a frame's first step draw
 * that time's sky round the camera, without fog */
static void mystic_ruins_garden_sky_draw(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = story_get_time();
    if (frame_step != 0) {
        return;
    }
    switch (work->submode) {
    case TIME_DAY:
        njSetTexture(&sky_day_texlist);
        draw_control_save();
        draw_control_set(NJD_CONTROL_3D_NO_CLIP_CHECK);
        draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
        njFogDisable();
        njPushMatrix(NULL);
        njTranslate(NULL, camera_work->pos.x, 0.0F, camera_work->pos.z);
        njScale(NULL, 0.3F, 0.3F, 0.3F);
        njSimpleDrawObject(&sky_day_object);
        njSimpleDrawObject(&sky_day_object_2);
        njSimpleDrawObject(&sky_day_object_2);
        njFogEnable();
        njPopMatrix(1);
        draw_control_restore();
        break;
    case TIME_EVENING:
        njSetTexture(&sky_evening_texlist);
        draw_control_save();
        draw_control_set(NJD_CONTROL_3D_NO_CLIP_CHECK);
        draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
        njFogDisable();
        njPushMatrix(NULL);
        njTranslate(NULL, camera_work->pos.x, 0.0F, camera_work->pos.z);
        njScale(NULL, 0.3F, 0.3F, 0.3F);
        njSimpleDrawObject(&sky_evening_object);
        njSimpleDrawObject(&sky_evening_object_2);
        njSimpleDrawObject(&sky_evening_object_3);
        njFogEnable();
        njPopMatrix(1);
        draw_control_restore();
        break;
    case TIME_NIGHT:
        njSetTexture(&sky_night_texlist);
        draw_control_save();
        draw_control_set(NJD_CONTROL_3D_NO_CLIP_CHECK);
        draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
        njFogDisable();
        njPushMatrix(NULL);
        njTranslate(NULL, camera_work->pos.x, 0.0F, camera_work->pos.z);
        njScale(NULL, 0.3F, 0.3F, 0.3F);
        njSimpleDrawObject(&sky_night_object);
        njSimpleDrawObject(&sky_night_object_2);
        njSimpleDrawObject(&sky_night_object_3);
        njFogEnable();
        njPopMatrix(1);
        draw_control_restore();
        break;
    }
}

/* set the work's submode to the time of day, and read the light sources of
 * the act whose number is that time's. a time other than TIME_DAY,
 * TIME_EVENING and TIME_NIGHT sets a red, green and blue background
 * instead */
static void mystic_ruins_garden_light_load(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = story_get_time();
    switch (work->submode) {
    case TIME_DAY:
        light_read_disc_sources(STAGE_ALIFE_MR, 0);
        break;
    case TIME_EVENING:
        light_read_disc_sources(STAGE_ALIFE_MR, 1);
        break;
    case TIME_NIGHT:
        light_read_disc_sources(STAGE_ALIFE_MR, 2);
        break;
    default:
        njSetBackColor(0xff0000, 0xff00, 0xff);
        break;
    }
}
