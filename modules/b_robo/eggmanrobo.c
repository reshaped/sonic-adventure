/* Amy's fight with Zero, on the Egg Carrier's deck. the stage's task starts
 * the fight, and the sky's draws the sky and the sea */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_e101_r/bg.h"
#include "b_robo/banks.h"
#include "b_robo/eggmanrobo.h"
#include "b_robo/erobo.h"
#include "camera/camera.h"
#include "draw/draw.h"
#include "event/play.h"
#include "sound/stream.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* work->mode under eggmanrobo_run. EGGMANROBO_FIGHT runs once, and the
 * mode after it does nothing */
enum eggmanrobo_mode { EGGMANROBO_START, EGGMANROBO_WAIT, EGGMANROBO_FIGHT };

/* the sky task's work->mode. eggmanrobo_sky_step never sets SKY_IDLE,
 * which would hold the sea's texture */
enum sky_mode { SKY_START, SKY_RUN, SKY_IDLE };

static void eggmanrobo_destroy(struct task *task);
static void eggmanrobo_sky_destroy(struct task *task);
static void eggmanrobo_sky_draw(struct task *task);
static void eggmanrobo_draw_sky(struct task_work *work);
static void eggmanrobo_draw_sea(struct task_work *work);

/* the steps eggmanrobo_step counts in work->mode before eggmanrobo_run
 * takes the task over */
#define DELAY_STEPS 2

/* the sea's textures, each shown for two steps of the sky's timer */
#define SEA_TEXTURES 10

/* njClipZ's near plane, and its far plane while the sky and the sea draw.
 * STAGE_FAR is its far plane for the rest of the stage */
#define CLIP_NEAR (-3.0F)
#define SKY_FAR (-10000.0F)
#define STAGE_FAR (-3000.0F)

/* once no event has control, load Zero's banks, make Zero and start its
 * music. the destructor set here releases the banks */
static void eggmanrobo_run(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case EGGMANROBO_START:
        task->destructor = eggmanrobo_destroy;
        work->mode++;
        /* falls through: the wait starts on this step */
    case EGGMANROBO_WAIT:
        if (event_in_control() != TRUE) {
            work->mode++;
        }
        break;
    case EGGMANROBO_FIGHT:
        erobo_load();
        erobo_create();
        stream_reset();
        stream_request_music(MUSIC_EGGROBO);
        work->mode++;
        break;
    }
}

static void eggmanrobo_destroy(struct task *task)
{
    erobo_release();
}

/* section HEAD puts the stage's step at the module's start, and the sky's
 * after it. 1ST_READ.BIN's stage_steps and sky_steps reach them there by
 * address */
#pragma section HEAD
/* count DELAY_STEPS in work->mode, then hand the task to eggmanrobo_run */
void eggmanrobo_step(struct task *task)
{
    struct task_work *work = task->work;

    work->mode++;
    if (work->mode >= DELAY_STEPS) {
        work->mode = EGGMANROBO_START;
        task->step = eggmanrobo_run;
    }
}
#pragma section

static void eggmanrobo_sky_destroy(struct task *task)
{
    njReleaseTexture(&e101_r_bg_texlist);
}

static void eggmanrobo_sky_draw(struct task *task)
{
    struct task_work *work = task->work;

    eggmanrobo_draw_sky(work);
    eggmanrobo_draw_sea(work);
}

/* on a frame's first step, draw the sky centered below the camera. it
 * draws unlit, without fog and out to SKY_FAR */
static void eggmanrobo_draw_sky(struct task_work *work)
{
    if (frame_step == 0) {
        njClipZ(CLIP_NEAR, SKY_FAR);
        draw_control_save();
        draw_attr_save();
        draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
        draw_control_set(NJD_CONTROL_3D_CONSTANT_ATTR |
                         NJD_CONTROL_3D_CONSTANT_MATERIAL);
        draw_attr_set(0, NJD_FLAG_IGNORE_LIGHT);
        njFogDisable();
        njSetTexture(&e101_r_bg_texlist);
        draw_set_material(0.0F, 0.8F, 0.8F, 0.8F);
        njPushMatrix(NULL);
        njTranslate(NULL, camera_work->pos.x, 0.0F, camera_work->pos.z);
        njSimpleDrawObject(&e101_r_sky_object);
        njPopMatrix(1);
        njFogEnable();
        draw_attr_restore();
        draw_control_restore();
        njClipZ(CLIP_NEAR, STAGE_FAR);
    }
}

/* set the sea's texture by the sky's timer. on a frame's first step, draw
 * the sea below the camera, at double scale in x and z. it draws without
 * fog and out to SKY_FAR */
static void eggmanrobo_draw_sea(struct task_work *work)
{
    NJS_OBJECT *sea = &e101_r_sea_object;

    sea->model->mats->attr_texId = TASK_TIMER(work) >> 1;
    if (frame_step == 0) {
        njClipZ(CLIP_NEAR, SKY_FAR);
        njFogDisable();
        njSetTexture(&e101_r_bg_texlist);
        njPushMatrix(NULL);
        draw_control_save();
        draw_attr_save();
        njTranslate(NULL, camera_work->pos.x, 0.0F, camera_work->pos.z);
        njScale(NULL, 2.0F, 1.0F, 2.0F);
        njSimpleDrawObject(sea);
        draw_attr_restore();
        draw_control_restore();
        njPopMatrix(1);
        njFogEnable();
        njClipZ(CLIP_NEAR, STAGE_FAR);
    }
}

#pragma section HEAD
/* the first step loads the sky's textures and sets the task's draw and
 * destructor. each later one moves the sky's timer on, and back to 0 at
 * SEA_TEXTURES * 2. every step draws the sky and the sea */
void eggmanrobo_sky_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SKY_START:
        texture_load_pvm("E101R_BG", &e101_r_bg_texlist);
        task->destructor = eggmanrobo_sky_destroy;
        task->draw = eggmanrobo_sky_draw;
        work->mode = SKY_RUN;
        break;
    case SKY_RUN:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) >= SEA_TEXTURES * 2) {
            TASK_TIMER(work) -= SEA_TEXTURES * 2;
        }
        break;
    case SKY_IDLE:
        break;
    }
    eggmanrobo_sky_draw(task);
}
#pragma section
