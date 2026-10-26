/* the waterfall that act 0's upper pool pours down into the act's water.
 * two ripples stretch down from its top, and a splash rides the water at its
 * foot */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "draw/draw.h"
#include "draw/tree.h"
#include "sound/sound.h"
#include "stg12/texlists.h"
#include "stg12/upper_pool.h"
#include "stg12/water.h"
#include "stg12/waterfall.h"
#include "stg12/waterfall_object.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the waterfall once it reaches full length */
extern NJS_OBJECT shelter_waterfall_long_object;
extern NJS_OBJECT shelter_waterfall_ripple_object;
extern NJS_OBJECT shelter_waterfall_splash_object;

/* the waterfall's top: the plug's x and z, at the height of the upper
 * pool's floor */
#define TOP_X 750.0F
#define TOP_Y 82.5F
#define TOP_Z (-650.0F)

/* the growing waterfall's y scale rises GROW_SPEED each step. at
 * GROW_LENGTH it reaches full length, and the long waterfall takes its
 * place at scale 1 */
#define GROW_SPEED 0.2F
#define GROW_LENGTH 8.0F

/* each mesh's scroll falls RUN_SPEED a step, and wraps at RUN_LENGTH */
#define RUN_SPEED 30
#define RUN_LENGTH 512

/* once the water is FALL_LEVEL high, the long waterfall shortens as the
 * water rises. its y scale would reach 0 with the water FALL_DEPTH high */
#define FALL_LEVEL 2.0F
#define FALL_DEPTH 87.0F

/* the UVs of the growing waterfall's two meshes, and of the long
 * waterfall's one */
#define GROW_UVS_0 68
#define GROW_UVS_1 28
#define LONG_UVS 34

/* the ripples hang RIPPLE_DROP below the waterfall's top. the splash
 * starts SPLASH_DROP below it, and rides the water once that is FALL_LEVEL
 * high */
#define RIPPLE_DROP 2.0F
#define SPLASH_DROP 80.5F

/* the ripples and the splash show TEXTURES textures in turn. work->tick
 * counts a texture's steps up to RIPPLE_TEXTURE_LAST_TICK for a ripple, and
 * up to SPLASH_TEXTURE_LAST_TICK for the splash. so a ripple shows each for
 * nine steps, not counting its waits, and the splash each for three. only
 * the first texture's first showing is one step shorter */
#define TEXTURES 5
#define RIPPLE_TEXTURE_LAST_TICK 8
#define SPLASH_TEXTURE_LAST_TICK 2

#define SE_WATERFALL 315

enum shelter_waterfall_mode {
    WATERFALL_WAIT = 0, /* until BIG_PULLED is set */
    WATERFALL_GROW = 1,
    WATERFALL_FALL = 2,
    WATERFALL_UNK_3 = 3, /* no code sets it */
    WATERFALL_GONE = 6
};

/* a ripple waits work->submode steps, then spreads and fades. it then
 * starts over with no wait */
enum shelter_waterfall_ripple_mode {
    RIPPLE_WAIT = 0,
    RIPPLE_SPREAD = 4,
    RIPPLE_FADE = 5
};

/* work->scl.y holds the waterfall's length as a y scale */
struct shelter_waterfall_vars {
    Float level; /* shelter_water_level as the last step saw it */
    Sint16 run[2]; /* each mesh's scroll. the long waterfall shares run[0] */
};

/* work->scl.y stretches the ripple down from its top */
struct shelter_waterfall_ripple_vars {
    Uint8 unk_00[4];
    Float alpha;
};

/* the splash's spin, which nothing reads */
struct shelter_waterfall_splash_vars {
    Float spin;
};

#define VARS(work) ((struct shelter_waterfall_vars *)&(work)->vars)
#define RIPPLE(work) ((struct shelter_waterfall_ripple_vars *)&(work)->vars)
#define SPLASH(work) ((struct shelter_waterfall_splash_vars *)&(work)->vars)

/* the v of each UV of the waterfalls' meshes, as the start reads them from
 * the models. each step writes them back plus the scroll */
static Sint16 shelter_waterfall_v0[GROW_UVS_0];
static Sint16 shelter_waterfall_v1[GROW_UVS_1];
static Sint16 shelter_waterfall_long_v[LONG_UVS];

static void shelter_waterfall_step(struct task *task);
static void shelter_waterfall_draw(struct task *task);
static void shelter_waterfall_destroy(struct task *task);
static void shelter_waterfall_ripple_start(struct task *task);
static void shelter_waterfall_ripple_step(struct task *task);
static void shelter_waterfall_ripple_draw(struct task *task);
static void shelter_waterfall_splash_start(struct task *task);
static void shelter_waterfall_splash_step(struct task *task);
static void shelter_waterfall_splash_draw(struct task *task);

void shelter_waterfall_start(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    work->tick = 0;
    TASK_TIMER(work) = 0;
    VARS(work)->level = 0.0F;
    work->pos.x = TOP_X;
    work->pos.y = TOP_Y;
    work->pos.z = TOP_Z;
    VARS(work)->run[0] = 0;
    VARS(work)->run[1] = 0;
    for (i = 0; i < GROW_UVS_0; i++) {
        shelter_waterfall_v0[i] =
            shelter_waterfall_object.model->meshsets[0].vertuv[i].tex.v;
    }
    for (i = 0; i < GROW_UVS_1; i++) {
        shelter_waterfall_v1[i] =
            shelter_waterfall_object.model->meshsets[1].vertuv[i].tex.v;
    }
    for (i = 0; i < LONG_UVS; i++) {
        shelter_waterfall_long_v[i] =
            shelter_waterfall_long_object.model->meshsets[0].vertuv[i].tex.v;
    }
    task->step = shelter_waterfall_step;
    task->draw = shelter_waterfall_draw;
    task->destructor = shelter_waterfall_destroy;
}

static void shelter_waterfall_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    NJS_COLOR *uv;
    Sint16 *from;
    Sint16 run;
    int i;

    switch (work->mode) {
    case WATERFALL_WAIT:
        if (shelter_big_flags & BIG_PULLED) {
            work->scl.y = 0.0F;
            work->mode = WATERFALL_GROW;
            child = task_create_child(TASK_WORK, shelter_waterfall_ripple_start,
                                      task);
            child->work->submode = 0;
            child = task_create_child(TASK_WORK, shelter_waterfall_ripple_start,
                                      task);
            child->work->submode = 40;
        }
        break;
    case WATERFALL_GROW:
        work->scl.y += GROW_SPEED;
        if (work->scl.y >= GROW_LENGTH) {
            work->scl.y = 1.0F;
            work->mode = WATERFALL_FALL;
            task_create_child(TASK_WORK, shelter_waterfall_splash_start, task);
            VARS(work)->level = shelter_water_level;
        }
        run = VARS(work)->run[0];
        run -= RUN_SPEED;
        run %= RUN_LENGTH;
        VARS(work)->run[0] = run;
        uv = shelter_waterfall_object.model->meshsets[0].vertuv;
        from = shelter_waterfall_v0;
        for (i = GROW_UVS_0; i > 0; --i) {
            uv->tex.v = *from + run;
            uv += 1;
            from += 1;
        }
        run = VARS(work)->run[1];
        run -= RUN_SPEED;
        run %= RUN_LENGTH;
        VARS(work)->run[1] = run;
        uv = shelter_waterfall_object.model->meshsets[1].vertuv;
        from = shelter_waterfall_v1;
        for (i = GROW_UVS_1; i > 0; --i) {
            uv->tex.v = *from + run;
            uv += 1;
            from += 1;
        }
        sound_play_for(SE_WATERFALL, (int)task, 1, 0, 2);
        break;
    case WATERFALL_FALL:
        if (shelter_water_level >= FALL_LEVEL) {
            work->scl.y = 1.0F - (shelter_water_level * (1.0F / FALL_DEPTH));
        }
        if (VARS(work)->level == shelter_water_level) {
            work->mode = WATERFALL_GONE;
        } else {
            VARS(work)->level = shelter_water_level;
        }
        run = VARS(work)->run[0];
        run -= RUN_SPEED;
        run %= RUN_LENGTH;
        VARS(work)->run[0] = run;
        uv = shelter_waterfall_long_object.model->meshsets[0].vertuv;
        from = shelter_waterfall_long_v;
        for (i = LONG_UVS; i > 0; --i) {
            uv->tex.v = *from + run;
            uv += 1;
            from += 1;
        }
        sound_play_for(SE_WATERFALL, (int)task, 1, 0, 2);
        break;
    case WATERFALL_UNK_3:
        break;
    case WATERFALL_GONE:
        task_kill(task);
        break;
    }
    shelter_waterfall_draw(task);
    task_step_children(task);
}

static void shelter_waterfall_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        njSetTexture(&hotshelter1_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njScale(NULL, 1.0F, work->scl.y, 1.0F);
        switch (work->mode) {
        case WATERFALL_FALL:
            draw_tree(&shelter_waterfall_long_object);
            break;
        default:
            draw_tree(&shelter_waterfall_object);
            break;
        }
        njPopMatrix(1);
    }
}

static void shelter_waterfall_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void shelter_waterfall_ripple_start(struct task *task)
{
    struct task_work *work = task->work;

    work->tick = 0;
    TASK_TIMER(work) = 0;
    work->pos.y -= RIPPLE_DROP;
    task->step = shelter_waterfall_ripple_step;
    task->draw = shelter_waterfall_ripple_draw;
}

static void shelter_waterfall_ripple_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case RIPPLE_WAIT:
        if (work->submode <= 0) {
            work->submode = 0;
            work->mode = RIPPLE_SPREAD;
            RIPPLE(work)->alpha = -0.2F;
            work->scl.y = 0.0F;
            break;
        }
        work->submode--;
        return;
    case RIPPLE_SPREAD:
        work->scl.y += 1.0F;
        if (work->scl.y >= 7.0F) {
            work->mode = RIPPLE_FADE;
        }
        break;
    case RIPPLE_FADE:
        RIPPLE(work)->alpha -= 0.01F;
        work->scl.y += 0.1F;
        if (RIPPLE(work)->alpha <= -1.0F) {
            RIPPLE(work)->alpha = -1.0F;
            work->mode = RIPPLE_WAIT;
        }
        break;
    }
    work->tick++;
    if (work->tick > RIPPLE_TEXTURE_LAST_TICK) {
        work->tick = 0;
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) >= TEXTURES) {
            TASK_TIMER(work) = 0;
        }
    }
    shelter_waterfall_ripple_object.model->mats->attr_texId = TASK_TIMER(work);
    shelter_waterfall_ripple_draw(task);
}

static void shelter_waterfall_ripple_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        njSetTexture(&shelter_column_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njScale(NULL, 1.0F, work->scl.y, 1.0F);
        draw_set_material(RIPPLE(work)->alpha, 0.0F, 0.0F, 0.0F);
        draw_tree(&shelter_waterfall_ripple_object);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPopMatrix(1);
    }
}

static void shelter_waterfall_splash_start(struct task *task)
{
    struct task_work *work = task->work;

    work->tick = 0;
    TASK_TIMER(work) = 0;
    work->pos.y -= SPLASH_DROP;
    SPLASH(work)->spin = 0.0F;
    task->step = shelter_waterfall_splash_step;
    task->draw = shelter_waterfall_splash_draw;
}

static void shelter_waterfall_splash_step(struct task *task)
{
    struct task_work *work = task->work;

    SPLASH(work)->spin += 2000.0F;
    work->scl.x = ((((Float)rand() * RAND_SCALE) - 0.5F) * 0.3F) + 1.2F;
    work->ang.y = RANDOM_ANGLE();
    if (shelter_water_level >= FALL_LEVEL) {
        work->pos.y = shelter_water_level;
    }
    work->tick++;
    if (work->tick > SPLASH_TEXTURE_LAST_TICK) {
        work->tick = 0;
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) >= TEXTURES) {
            TASK_TIMER(work) = 0;
        }
    }
    shelter_waterfall_splash_object.model->mats->attr_texId = TASK_TIMER(work);
    shelter_waterfall_splash_draw(task);
}

static void shelter_waterfall_splash_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        njSetTexture(&shelter_column_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        njScale(NULL, work->scl.x, 1.2F, work->scl.x);
        draw_tree(&shelter_waterfall_splash_object);
        njPopMatrix(1);
    }
}
