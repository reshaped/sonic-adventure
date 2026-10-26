/* B_E101_R's stage task and sky task. the stage's starts the fight with
 * E-101 mkII, and the sky's draws the sky and the sea */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_e101_r/bg.h"
#include "b_e101_r/e101_mk2.h"
#include "b_e101_r/e101_r.h"
#include "b_e101_r/files.h"
#include "camera/camera.h"
#include "draw/draw.h"
#include "event/play.h"
#include "sound/stream.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* e101_r_step counts its steps in work->mode, and on the E101_R_WAIT-th
 * hands the task to e101_r_run */
#define E101_R_WAIT 2

/* e101_r_run's modes, in work->mode */
enum e101_r_mode { E101_R_INIT, E101_R_WAIT_EVENT, E101_R_START, E101_R_FIGHT };

/* e101_r_sky_step's modes. no code sets SKY_STOP */
enum sky_mode { SKY_START, SKY_RUN, SKY_STOP };

/* the steps of the sea's cycle: each of its ten textures shows for two */
#define SEA_STEPS 20

static void e101_r_run(struct task *task);
static void e101_r_destroy(struct task *task);
static void e101_r_sky_destroy(struct task *task);
static void e101_r_sky_draw(struct task *task);

/* #pragma section HEAD puts the step in PHEAD, after the camera's step, at
 * 0x0c900060. 1ST_READ.BIN's stage_steps reaches it by that address */
#pragma section HEAD
void e101_r_step(struct task *task)
{
    struct task_work *work = task->work;

    work->mode++;
    if (work->mode >= E101_R_WAIT) {
        work->mode = E101_R_INIT;
        task->step = e101_r_run;
    }
}
#pragma section

static void e101_r_run(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case E101_R_INIT:
        task->destructor = e101_r_destroy;
        work->mode++;
        /* fall through: the wait for the event's end starts this step */
    case E101_R_WAIT_EVENT:
        /* @hack spelling: event_in_control's result compared with TRUE, which
         * SHC tests with CMP/EQ #1 at 0x0c9074e8. event_in_control returns only
         * TRUE or FALSE, so !event_in_control() is the same test; as that, 2 of
         * 44 words differ */
        if (event_in_control() != TRUE) {
            work->mode++;
        }
        break;
    case E101_R_START:
        e101_r_load();
        e101_mk2_create();
        stream_reset();
        stream_request_music(MUSIC_BOSSE101);
        work->mode++;
        break;
    }
}

static void e101_r_destroy(struct task *task)
{
    e101_r_release();
}

/* #pragma section HEAD puts the sky's step after the stage's, at
 * 0x0c9000a0. 1ST_READ.BIN's sky_steps reaches it by that address. it draws
 * from its first step */
#pragma section HEAD
void e101_r_sky_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SKY_START:
        texture_load_pvm("E101R_BG", &e101_r_bg_texlist);
        task->destructor = e101_r_sky_destroy;
        task->draw = e101_r_sky_draw;
        work->mode = SKY_RUN;
        /* fall through: the sea turns from this step */
    case SKY_RUN:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) >= SEA_STEPS) {
            TASK_TIMER(work) -= SEA_STEPS;
        }
        break;
    case SKY_STOP:
        break;
    }
    e101_r_sky_draw(task);
}
#pragma section

static void e101_r_sky_destroy(struct task *task)
{
    njReleaseTexture(&e101_r_bg_texlist);
}

static void e101_r_draw_sky(struct task_work *work);
static void e101_r_draw_sea(struct task_work *work);

static void e101_r_sky_draw(struct task *task)
{
    struct task_work *work = task->work;

    e101_r_draw_sky(work);
    e101_r_draw_sea(work);
}

static void e101_r_draw_sky(struct task_work *work)
{
    if (frame_step == 0) {
        njClipZ(-3.0F, -10000.0F);
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
        njClipZ(-3.0F, -3000.0F);
    }
}

static void e101_r_draw_sea(struct task_work *work)
{
    NJS_OBJECT *sea = &e101_r_sea_object;

    sea->model->mats[0].attr_texId = TASK_TIMER(work) >> 1;
    if (frame_step == 0) {
        njClipZ(-3.0F, -10000.0F);
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
        njClipZ(-3.0F, -3000.0F);
    }
}
