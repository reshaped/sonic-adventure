/* the cart course's sky task. it draws a backdrop around the camera, and
 * sets the stage's light */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "draw/light.h"
#include "draw/tree.h"
#include "minicart/sky.h"
#include "minicart/sky_model.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

/* the sky's record, in task->motion. it holds the stage and act the sky
 * runs in, and the texlist and object it draws */
struct cart_sky_record {
    Uint8 stage;
    Uint8 act;
    Sint8 mode;
    NJS_TEXLIST *texlist;
    NJS_OBJECT *object;
};

enum sky_mode { SKY_LOAD, SKY_RUN };

static void cart_sky_init(struct task *task, void *arg);
static void cart_sky_run(struct task *task);
static void cart_sky_draw(struct task *task);
static void cart_sky_destroy(struct task *task);
static void cart_sky_light(void);

static const struct task_desc cart_sky_desc = {sizeof(struct cart_sky_record),
                                               TASK_WORK,
                                               cart_sky_init,
                                               cart_sky_run,
                                               cart_sky_draw,
                                               cart_sky_destroy};

/* in HEAD, at 0x0c900000, the module's first byte. 1ST_READ.BIN's
 * sky_steps holds that address for the stage */
#pragma section HEAD
void cart_sky_step(struct task *task)
{
    task_apply_desc(task, &cart_sky_desc, NULL);
}
#pragma section

static void cart_sky_init(struct task *task, void *arg)
{
    struct cart_sky_record *record = (struct cart_sky_record *)task->motion;
    Uint32 stage_act = stage_get_stage_act();

    record->stage = STAGE_OF(stage_act);
    record->act = stage_act;
    record->mode = SKY_LOAD;
    record->texlist = NULL;
    record->object = NULL;
    cart_sky_run(task);
}

/* the textures load once, and the light is set again whenever the act
 * changes. the task destroys itself once the stage changes */
static void cart_sky_run(struct task *task)
{
    struct cart_sky_record *record = (struct cart_sky_record *)task->motion;
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
        if (record->texlist == NULL) {
            NJS_TEXLIST *texlist = &cart_sky_texlist;

            record->texlist = texlist;
            texture_load_pvm("BG_SHAREOBJ", texlist);
        }
        record->object = &cart_sky_object;
        cart_sky_light();
        record->mode = SKY_RUN;
        /* falls through: the sky draws on the step it loads */
    case SKY_RUN:
        if (act == record->act) {
            cart_sky_draw(task);
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

/* turn fog off. on a frame's first step, draw the sky around the camera
 * with depth cueing off */
static void cart_sky_draw(struct task *task)
{
    struct cart_sky_record *record = (struct cart_sky_record *)task->motion;
    NJS_TEXLIST *texlist = record->texlist;

    njFogDisable();
    if (frame_step == 0 && texlist != NULL) {
        draw_control_save();
        draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
        njPushMatrix(NULL);
        njTranslateV(NULL, &camera_work->pos);
        njSetTexture(texlist);
        draw_tree(record->object);
        njPopMatrix(1);
        draw_control_restore();
    }
}

static void cart_sky_destroy(struct task *task)
{
    struct cart_sky_record *record = (struct cart_sky_record *)task->motion;
    NJS_TEXLIST *texlist = record->texlist;

    if (texlist != NULL) {
        njReleaseTexture(texlist);
    }
}

static void cart_sky_light(void)
{
    NJS_LIGHT *light = &dir_light;

    njSetLightIntensity(light, 0.8F, 0.9F, 0.2F);
    njSetLightColor(light, 1.0F, 1.0F, 0.95F);
    njSetLightDirection(light, 0.5F, -0.866025F, 0.0F);
}
