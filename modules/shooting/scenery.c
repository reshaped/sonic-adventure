/* the sky chase's scenery: the land far under player 0, and drifting cloud
 * over it. act 1 also draws the sun's flare. the land and the cloud follow
 * player 0. while an event or the score plays, they sit under the origin
 * instead */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/lens.h"
#include "player/player.h"
#include "shooting/act1_textures.h"
#include "shooting/act2_textures.h"
#include "shooting/scenery.h"
#include "shooting/shooting.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 12
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 12
#define TEXT_4_LENGTH 15
#include "shooting/scenery/text.inc"

/* how far a cloud's textures have drifted */
struct cloud_vars {
    Uint8 u;
    Uint8 v;
    Uint8 tiles; /* set to 4, never read */
};

#define VARS(work) ((struct cloud_vars *)&(work)->vars)

enum scenery_mode { SCENERY_INIT, SCENERY_RUN, SCENERY_STILL };

/* the land's depth under player 0, and the cloud's height over the land */
#define LAND_DEPTH 4000.0F
#define CLOUD_HEIGHT 2000.0F

extern NJS_OBJECT act1_land_object;
extern NJS_OBJECT act2_land_object;
extern NJS_OBJECT act1_cloud_object;
extern NJS_OBJECT act2_cloud_object;

static void scenery_follow(struct task *task);
static void scenery_hold(struct task *task);
static void land_draw(struct task *task);
static void land_destroy(struct task *task);
static void cloud_step(struct task *task);
static void cloud_drift(struct task_work *work);
static void cloud_draw(struct task *task);
static void cloud_destroy(struct task *task);
static void low_cloud_draw(struct task *task);

/* the sky's texture names, texanim and texture list, which no code reads */
static NJS_TEXNAME sky_textures[4] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
    {TEXT_4, 0, 0},
};
static NJS_TEXANIM sky_frame[1] = {
#include "shooting/scenery/sky_frame.inc"
};
static NJS_TEXLIST sky_texlist = {
#include "shooting/scenery/sky_texlist.inc"
};

void shooting_scenery_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SCENERY_INIT:
        task->destructor = land_destroy;
        task->draw = land_draw;
        work->pos.x = 0.0F;
        work->pos.y = 0.0F;
        work->pos.z = 0.0F;
        task_create_child(TASK_WORK, cloud_step, task);
        work->mode = SCENERY_RUN;
        break;
    case SCENERY_RUN:
        if (shooting_event_playing()) {
            scenery_hold(task);
        } else {
            scenery_follow(task);
        }
        land_draw(task);
    }
    task_step_children(task);
}

static void scenery_follow(struct task *task)
{
    struct task_work *player = player_get_task(0)->work;
    struct task_work *work = task->work;

    work->pos.x = player->pos.x;
    work->pos.y = player->pos.y - LAND_DEPTH;
    work->pos.z = player->pos.z;
}

static void scenery_hold(struct task *task)
{
    struct task_work *work = task->work;

    work->pos.x = 0.0F;
    work->pos.y = -LAND_DEPTH;
    work->pos.z = 0.0F;
}

/* draw the land at four times its size, without fog. act 1 also draws the
 * sun's flare, unless an event or the score plays. it draws at a frame's
 * first step only */
static void land_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 sun;

    if (frame_step != 0) {
        return;
    }
    njFogDisable();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njScale(NULL, 4.0F, 4.0F, 4.0F);
    switch (shooting_act) {
    case 0:
        njSetTexture(&shooting1_texlist);
        draw_tree(&act1_land_object);
        break;
    case 1:
        njSetTexture(&shooting2_texlist);
        draw_tree(&act2_land_object);
        break;
    }
    njPopMatrix(1);
    if (!shooting_event_playing()) {
        switch (shooting_act) {
        case 0:
            sun.x = work->pos.x - 4049.08F;
            sun.y = work->pos.y + 36818.02F;
            sun.z = work->pos.z - 4425.56F;
            lens_draw(&sun);
            break;
        /* @hack redundant: a case 1 that only breaks, which keeps the image's
         * CMP/EQ #1 and its BT to the end at 0x0c904048; without it, 42 of
         * 104 words differ */
        case 1:
            break;
        }
    }
}

static void land_destroy(struct task *task)
{
}

static void cloud_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SCENERY_INIT:
        VARS(work)->u = 0;
        VARS(work)->v = 0;
        VARS(work)->tiles = 4;
        task->destructor = cloud_destroy;
        task->draw = cloud_draw;
        work->mode = SCENERY_RUN;
        break;
    case SCENERY_RUN:
        if (shooting_event_playing()) {
            scenery_hold(task);
        } else {
            scenery_follow(task);
        }
        cloud_drift(work);
        cloud_draw(task);
        break;
    /* @hack redundant: case SCENERY_STILL, which only breaks, keeps the image's
     * CMP/EQ #2 and its BT to the end at 0x0c9040a0; without it, 41 of 49
     * words differ */
    case SCENERY_STILL:
        break;
    }
}

/* drift the act's cloud texture. v falls by 2 each step, and the cloud
 * model's first four uvs become corners plus the drift */
static void cloud_drift(struct task_work *work)
{
    NJS_TEX corners[4] = {
#include "shooting/scenery/corners.inc"
    };
    NJS_COLOR *uv;
    int i;

    switch (shooting_act) {
    case 0:
        uv = act1_cloud_object.model->meshsets->vertuv;
        break;
    case 1:
        uv = act2_cloud_object.model->meshsets->vertuv;
        break;
    }
    VARS(work)->v -= 2;
    for (i = 0; i < 4; i++) {
        uv[i].tex.u = corners[i].u + VARS(work)->u;
        uv[i].tex.v = corners[i].v + VARS(work)->v;
    }
}

/* draw the act's cloud twice, 24 times its size across and turned about y
 * as player 0 is. the first copy sits CLOUD_HEIGHT over the land. it draws
 * with NJD_FLAG_USE_ALPHA, and -0.7 as the constant material's alpha. the
 * second sits at the land's height and draws without them. under
 * NJD_CONTROL_3D_OFFSET_MATERIAL, which main.c sets, that -0.7 adds to the
 * cloud material's alpha of 1.0, so the first copy draws at alpha 0.3 */
static void cloud_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njFogDisable();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + CLOUD_HEIGHT, work->pos.z);
    njScale(NULL, 24.0F, 1.0F, 24.0F);
    TURN_VIA(njRotateY, player_work[0]->ang.y, ang);
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    draw_set_material(-0.7F, 0.0F, 0.0F, 0.0F);
    switch (shooting_act) {
    case 0:
        njSetTexture(&shooting1_texlist);
        draw_tree(&act1_cloud_object);
        break;
    case 1:
        njSetTexture(&shooting2_texlist);
        draw_tree(&act2_cloud_object);
        break;
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    draw_attr_restore();
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njScale(NULL, 24.0F, 1.0F, 24.0F);
    TURN_VIA(njRotateY, player_work[0]->ang.y, ang);
    switch (shooting_act) {
    case 0:
        njSetTexture(&shooting1_texlist);
        draw_tree(&act1_cloud_object);
        break;
    case 1:
        njSetTexture(&shooting2_texlist);
        draw_tree(&act2_cloud_object);
        break;
    }
    njPopMatrix(1);
    njFogDisable();
}

static void cloud_destroy(struct task *task)
{
}

void shooting_low_cloud_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SCENERY_INIT:
        VARS(work)->u = 0;
        VARS(work)->v = 0;
        VARS(work)->tiles = 4;
        task->destructor = cloud_destroy;
        task->draw = cloud_draw;
        work->mode = SCENERY_RUN;
        break;
    case SCENERY_RUN:
        scenery_follow(task);
        low_cloud_draw(task);
        break;
    /* @hack redundant: case SCENERY_STILL, which only breaks, keeps the image's
     * CMP/EQ #2 and its BT to the end at 0x0c904392; without it, 27 of 35
     * words differ */
    case SCENERY_STILL:
        break;
    }
}

static void low_cloud_draw(struct task *task)
{
    struct task_work *work = task->work;

    njFogDisable();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y - 8000.0F, work->pos.z);
    njScale(NULL, 8.0F, 1.0F, 8.0F);
    njSetTexture(&shooting2_texlist);
    draw_tree(&act2_cloud_object);
    njPopMatrix(1);
}
