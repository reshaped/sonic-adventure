/* the water splashes that O ICEBAR1 and O ICEBAR2 raise as they rise */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "stg08/splash.h"
#include "system/task.h"
#include "system/util.h"

/* alpha_water01 to alpha_water08, the splash's eight images */
extern NJS_TEXLIST alpha_water_texlist;

/* a splash plays its images at SPLASH_RATE a step. the play ends as it
 * reaches the last, so the rate past it, SPLASH_LAST_RATE, never applies */
#define SPLASH_IMAGES 8
#define SPLASH_LAST (SPLASH_IMAGES - 1.0F)
#define SPLASH_RATE 0.5F
#define SPLASH_LAST_RATE 1.0F
/* how far the splash sinks for each image played: not at all */
#define SPLASH_SINK 0.0F
/* the fade lasts SPLASH_FADE_STEPS steps. its alpha falls from 1 by
 * SPLASH_FADE a step, so it reaches 0 on the fade's third step. it stays
 * below 0 for the fade's other 17 */
#define SPLASH_FADE 0.5F
#define SPLASH_FADE_STEPS 20
/* a flickering splash's alpha rises by up to SPLASH_FLICKER a step. it stays
 * within SPLASH_FLICKER_MIN and SPLASH_FLICKER_MAX */
#define SPLASH_FLICKER 0.01F
#define SPLASH_FLICKER_MAX 0.6F
#define SPLASH_FLICKER_MIN 0.1F

struct splash_vars {
    Float alpha;
    Uint8 unk_04[4];
    Float y; /* pos.y, kept while the draw lowers pos by the drop */
};

#define VARS(work) ((struct splash_vars *)&(work)->vars)
#define SPLASH_SIZE(work) ((work)->scl.x)
/* how far the splash has played, in images */
#define SPLASH_IMAGE(work) ((work)->scl.y)
/* how far the draw lowers it: SPLASH_SINK for each image played */
#define SPLASH_DROP(work) ((work)->scl.z)

/* nothing sets SPLASH_FLICKERING or SPLASH_HOLD */
enum splash_mode {
    SPLASH_INIT,
    SPLASH_PLAY,
    SPLASH_FLICKERING,
    SPLASH_HOLD,
    SPLASH_FADING,
    SPLASH_GONE
};

static void icecap_splash_init(struct task *task);
static void icecap_splash_play(struct task *task);
static void icecap_splash_flicker(struct task *task);
static void icecap_splash_draw(struct task *task);
static void icecap_splash_fade(struct task *task);
static void icecap_splash_destroy(struct task *task);

static NJS_TEXANIM icecap_splash_texanims[SPLASH_IMAGES] = {
#include "stg08/splash/icecap_splash_texanims.inc"
};
static NJS_SPRITE icecap_splash_sprite = {
#include "stg08/splash/icecap_splash_sprite.inc"
};

static void icecap_splash_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SPLASH_INIT:
        icecap_splash_init(task);
        break;
    case SPLASH_PLAY:
        icecap_splash_play(task);
        icecap_splash_draw(task);
        break;
    case SPLASH_FADING:
        icecap_splash_fade(task);
        icecap_splash_draw(task);
        break;
    case SPLASH_FLICKERING:
        icecap_splash_flicker(task);
        /* SPLASH_FLICKERING falls through to SPLASH_HOLD's draw */
    case SPLASH_HOLD:
        icecap_splash_draw(task);
        break;
    case SPLASH_GONE:
        /* @hack redundant: task_kill in a loop of one pass, which keeps the
         * call a JSR followed by a return (0x0c900052, 0x0c900058); as a
         * plain call, SHC jumps to it, the function runs 4 bytes long and 7
         * of 46 words differ */
        do {
            task_kill(task);
        } while (0);
        break;
    /* @hack redundant: the default's return, which returns from the switch's
     * test itself (RTS at 0x0c900024); without it, SHC branches to the
     * end, the function runs 2 bytes short and 35 of 46 words differ */
    default:
        return;
    }
}

static void icecap_splash_init(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = SPLASH_PLAY;
    work->timer = 0;
    VARS(work)->alpha = 1.0F;
    task->draw = icecap_splash_draw;
    task->destructor = icecap_splash_destroy;
}

static void icecap_splash_play(struct task *task)
{
    struct task_work *work = task->work;

    if (SPLASH_IMAGE(work) < SPLASH_LAST) {
        SPLASH_IMAGE(work) += SPLASH_RATE;
    } else {
        SPLASH_IMAGE(work) += SPLASH_LAST_RATE;
    }
    SPLASH_DROP(work) = SPLASH_IMAGE(work) * SPLASH_SINK;
    if (SPLASH_IMAGE(work) >= SPLASH_LAST) {
        work->mode = SPLASH_FADING;
        SPLASH_IMAGE(work) = SPLASH_LAST;
    }
    work->timer++;
}

static void icecap_splash_flicker(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->alpha += rand() * (SPLASH_FLICKER * RAND_SCALE);
    if (VARS(work)->alpha > SPLASH_FLICKER_MAX) {
        VARS(work)->alpha = SPLASH_FLICKER_MAX;
    } else if (VARS(work)->alpha < SPLASH_FLICKER_MIN) {
        VARS(work)->alpha = SPLASH_FLICKER_MIN;
    }
}

static void icecap_splash_draw(struct task *task)
{
    struct task_work *work = task->work;
    Uint32 image = (Uint32)SPLASH_IMAGE(work);
    NJS_ARGB color;

    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    VARS(work)->y = work->pos.y;
    /* @hack spelling: the subtraction written out, which reads the drop at
     * @(R0,R14) (0x0c90016e); with -=, SHC reaches it through its address in
     * R1, the function runs 2 bytes long and 97 of 130 words differ */
    work->pos.y = work->pos.y - SPLASH_DROP(work);
    njSetTexture(&alpha_water_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, camera_work->ang.y);
    TURN(njRotateX, camera_work->ang.x);
    TURN(njRotateZ, work->ang.z);
    njScale(NULL, SPLASH_SIZE(work), SPLASH_SIZE(work), SPLASH_SIZE(work));
    color.a = VARS(work)->alpha;
    color.r = 1.0F;
    color.g = 1.0F;
    color.b = 1.0F;
    njSetConstantMaterial(&color);
    njDrawSprite3D(&icecap_splash_sprite, image,
                   NJD_SPRITE_ANGLE | NJD_SPRITE_COLOR | NJD_SPRITE_ALPHA);
    njPopMatrix(1);
    color.a = 0.0F;
    color.r = 0.0F;
    color.g = 0.0F;
    color.b = 0.0F;
    njSetConstantMaterial(&color);
    work->pos.y = VARS(work)->y;
}

static void icecap_splash_fade(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->alpha = 1.0F - (work->tick * SPLASH_FADE);
    work->tick++;
    if (work->tick == SPLASH_FADE_STEPS) {
        work->mode = SPLASH_GONE;
    }
}

/* the destructor. its task_kill changes nothing: task_destroy clears the
 * step as the destructor returns */
static void icecap_splash_destroy(struct task *task)
{
    task_kill(task);
}

void icecap_splash_create(NJS_POINT3 *pos, Angle ang, Float size)
{
    struct task *task = task_create(TASK_WORK, 6, icecap_splash_step);
    struct task_work *work;

    if (task != NULL) {
        work = task->work;
        work->pos = *pos;
        SPLASH_SIZE(work) = size;
        SPLASH_IMAGE(work) = 0.0F;
        work->ang.x = 0;
        work->ang.y = 0;
        work->ang.z = ang;
    }
}
