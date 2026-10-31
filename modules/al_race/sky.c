/* the Chao race's sky, a dome around the camera, drawn only on the course */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_race/game.h"
#include "al_race/sky.h"
#include "camera/camera.h"
#include "draw/tree.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

#define TEXT_1_LENGTH 7
#include "al_race/sky/text.inc"

/* the sky's state, in its task's motion. texlist and object stay null until
 * SKY_LOAD loads the textures */
struct sky {
    Uint8 stage; /* the stage the sky was created in */
    Uint8 act; /* the act sky_step last saw */
    Sint8 mode; /* an enum sky_mode */
    Uint8 unk_03; /* sky.c never reads or writes it */
    NJS_TEXLIST *texlist;
    NJS_OBJECT *object;
};

#define SKY(task) ((struct sky *)(task)->motion)

enum sky_mode { SKY_LOAD, SKY_SHOW };

/* the depth of the dome's center below the camera, and its scale */
#define SKY_DROP 140.0F
#define SKY_SCALE 2.0F

static void sky_init(struct task *task, void *arg);
static void sky_step(struct task *task);
static void sky_draw(struct task *task);
static void sky_destroy(struct task *task);

static const struct task_desc sky_desc = {
    sizeof(struct sky), TASK_WORK, sky_init, sky_step, sky_draw, sky_destroy};

static NJS_MATERIAL sky_materials[1] = {
#include "al_race/sky/sky_materials.inc"
};

static Sint16 sky_meshes0[96] = {
#include "al_race/sky/sky_meshes0.inc"
};

static NJS_COLOR sky_uvs0[88] = {
#include "al_race/sky/sky_uvs0.inc"
};

static NJS_MESHSET sky_meshsets[1] = {
#include "al_race/sky/sky_meshsets.inc"
};

static NJS_POINT3 sky_points[41] = {
#include "al_race/sky/sky_points.inc"
};

static NJS_VECTOR sky_normals[41] = {
#include "al_race/sky/sky_normals.inc"
};

static NJS_MODEL sky_model = {
#include "al_race/sky/sky_model.inc"
};

static NJS_OBJECT sky_object = {
#include "al_race/sky/sky_object.inc"
};

static NJS_TEXNAME sky_textures[1] = {{TEXT_1, 0, 0}};
static NJS_TEXLIST sky_texlist = {
#include "al_race/sky/sky_texlist.inc"
};

/* in the HEAD section, which opens AL_RACE: 1ST_READ.BIN's sky_steps holds
 * race_sky_step's address there, 0x0cb80000 */
#pragma section HEAD
void race_sky_step(struct task *task)
{
    task_apply_desc(task, &sky_desc, NULL);
}
#pragma section

static void sky_init(struct task *task, void *arg)
{
    struct sky *sky = SKY(task);
    Uint32 stage_act = stage_get_stage_act();

    sky->stage = STAGE_OF(stage_act);
    sky->act = stage_act;
    sky->mode = SKY_LOAD;
    sky->texlist = NULL;
    sky->object = NULL;
    sky_step(task);
}

/* end the task once the stage changes. the textures load at the first step,
 * and a change of act sends the sky back to SKY_LOAD, which leaves them
 * loaded */
static void sky_step(struct task *task)
{
    struct sky *sky = SKY(task);
    Uint32 stage_act = stage_get_stage_act();
    Uint8 act;

    if (STAGE_OF(stage_act) != sky->stage) {
        task_kill(task);
        return;
    }
    act = stage_act;
    switch (sky->mode) {
    case SKY_LOAD:
        if (act != sky->act) {
            sky->act = act;
        }
        if (sky->texlist == NULL) {
            NJS_TEXLIST *texlist = &sky_texlist;

            sky->texlist = texlist;
            texture_load_pvm("BG_AL_RACE02", texlist);
        }
        sky->object = &sky_object;
        sky->mode = SKY_SHOW;
        /* fallthrough */
    case SKY_SHOW:
        if (act != sky->act) {
            sky->act = act;
            sky->mode = SKY_LOAD;
        }
        break;
    default:
        task_kill(task);
        break;
    }
    sky_draw(task);
}

/* draw the dome, on the course once it is loaded. every call turns fog off,
 * whether it draws or not */
static void sky_draw(struct task *task)
{
    struct sky *sky = SKY(task);
    NJS_TEXLIST *texlist = sky->texlist;
    NJS_OBJECT *object = sky->object;
    NJS_POINT3 pos;
    NJS_POINT3 *camera = &camera_work->pos;

    pos.x = camera->x;
    pos.y = camera->y - SKY_DROP;
    pos.z = camera->z;
    njFogDisable();
    if (frame_step == 0 && texlist != NULL && object != NULL &&
        sky->act == ACT_COURSE) {
        draw_control_save();
        draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
        njPushMatrix(NULL);
        njTranslateV(NULL, &pos);
        njScale(NULL, SKY_SCALE, SKY_SCALE, SKY_SCALE);
        njSetTexture(texlist);
        draw_tree(object);
        njPopMatrix(1);
        draw_control_restore();
    }
}

static void sky_destroy(struct task *task)
{
    struct sky *sky = SKY(task);
    NJS_TEXLIST *texlist = sky->texlist;

    if (texlist != NULL) {
        njReleaseTexture(texlist);
        sky->texlist = NULL;
    }
}
