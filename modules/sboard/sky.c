/* Sand Hill's sky task. it draws a backdrop from BG_SANDBOARD around the
 * camera, and sets the stage's fog and light */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "draw/light.h"
#include "draw/tree.h"
#include "sboard/sky.h"
#include "sboard/sky_model.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

/* the sky's record, in task->motion. it holds the stage and act the sky
 * runs in, and the texlist and object it draws */
struct sky_record {
    Uint8 stage;
    Uint8 act;
    Sint8 mode;
    NJS_TEXLIST *texlist;
    NJS_OBJECT *object;
};

enum sky_mode { SKY_LOAD, SKY_RUN };

#define SKY_BACK_COLOR 0x00a88848
#define SKY_FOG_COLOR 0xffe0e0b0

static void sunaboard_sky_init(struct task *task, void *arg);
static void sunaboard_sky_run(struct task *task);
static void sunaboard_sky_draw(struct task *task);
static void sunaboard_sky_destroy(struct task *task);

/* the sky task's kind, in HEAD at 0x0c900178 */
#pragma section HEAD
static const struct task_desc sunaboard_sky_desc = {
    sizeof(struct sky_record), TASK_WORK,          sunaboard_sky_init,
    sunaboard_sky_run,         sunaboard_sky_draw, sunaboard_sky_destroy};
#pragma section

static NJS_FOG_TABLE sunaboard_fog;

/* in HEAD, at 0x0c900000, the module's first byte. 1ST_READ.BIN's
 * sky_steps holds that address for the stage */
#pragma section HEAD
void sunaboard_sky_step(struct task *task)
{
    task_apply_desc(task, &sunaboard_sky_desc, NULL);
}
#pragma section

static void sunaboard_sky_init(struct task *task, void *arg)
{
    struct sky_record *record = (struct sky_record *)task->motion;
    Uint32 stage_act = stage_get_stage_act();

    record->stage = STAGE_OF(stage_act);
    record->act = stage_act;
    record->mode = SKY_LOAD;
    record->texlist = NULL;
    sunaboard_sky_run(task);
}

/* the textures load once, and the rest of SKY_LOAD runs again whenever the
 * act changes. the task destroys itself once the stage changes */
static void sunaboard_sky_run(struct task *task)
{
    struct sky_record *record = (struct sky_record *)task->motion;
    Uint32 stage_act = stage_get_stage_act();
    Uint8 act;

    if (STAGE_OF(stage_act) != record->stage) {
        task_kill(task);
        return;
    }
    act = stage_act;
    switch (record->mode) {
    case SKY_LOAD:
        if (act != record->act) {
            record->act = act;
        }
        njSetBackColor(SKY_BACK_COLOR, SKY_BACK_COLOR, SKY_BACK_COLOR);
        if (record->texlist == NULL) {
            NJS_TEXLIST *texlist = &sunaboard_sky_texlist;

            record->texlist = texlist;
            texture_load_pvm("BG_SANDBOARD", texlist);
        }
        record->object = &sunaboard_sky_object;
        njClipZ(-2.9999998F, -3000.0F);
        njSetDepthQueue(-1700.0F, -1900.0F);
        njSetFogColor(SKY_FOG_COLOR);
        njGenerateFogTable3(sunaboard_fog, -800.0F, -3000.0F);
        njSetFogTable(sunaboard_fog);
        njFogEnable();
        njSetLightIntensity(&dir_light, 1.0F, 1.0F, 0.36F);
        njSetLightColor(&dir_light, 1.0F, 1.0F, 1.0F);
        njSetLightDirection(&dir_light, 0.0F, -1.0F, 0.0F);
        record->mode = SKY_RUN;
        /* falls through: the sky draws on the step it loads */
    case SKY_RUN:
        if (act == record->act) {
            sunaboard_sky_draw(task);
        } else {
            record->act = act;
            record->mode = SKY_LOAD;
        }
        break;
    default:
        task_kill(task);
        break;
    }
}

/* on a frame's first step, draw the sky around the camera without fog,
 * depth cueing or clip checks. the stage's fog then goes back on */
static void sunaboard_sky_draw(struct task *task)
{
    struct sky_record *record = (struct sky_record *)task->motion;
    NJS_TEXLIST *texlist = record->texlist;

    if (frame_step == 0 && texlist != NULL) {
        draw_control_save();
        draw_control_set(NJD_CONTROL_3D_NO_CLIP_CHECK);
        draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
        njFogDisable();
        njPushMatrix(NULL);
        njTranslateV(NULL, &camera_work->pos);
        njSetTexture(texlist);
        draw_tree(record->object);
        njPopMatrix(1);
        njSetFogTable(sunaboard_fog);
        njFogEnable();
        draw_control_restore();
    }
}

static void sunaboard_sky_destroy(struct task *task)
{
    struct sky_record *record = (struct sky_record *)task->motion;
    NJS_TEXLIST *texlist = record->texlist;

    if (texlist != NULL) {
        njReleaseTexture(texlist);
        record->texlist = NULL;
    }
}
