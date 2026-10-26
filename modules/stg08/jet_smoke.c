/* jet smoke clouds that the avalanche's first wave raises */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "stg08/jet_smoke.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST jet_smk_texlist;

/* the animation's frames. below SMOKE_SLOW, a step adds SMOKE_RATE to a
 * cloud's frame. from there up to SMOKE_LAST, it adds SMOKE_SLOW_RATE. the
 * cloud ends at SMOKE_LAST */
#define SMOKE_FRAMES 8
#define SMOKE_LAST (SMOKE_FRAMES - 1.0F)
#define SMOKE_SLOW 4.0F
#define SMOKE_RATE 0.2F
#define SMOKE_SLOW_RATE 0.1F

#define SMOKE_SIZE 0.3F

/* in work->scl: the cloud's height, its animation frame, and its spread, how
 * much wider than high it draws. height and spread scale its sprite */
#define SMOKE_HEIGHT(work) ((work)->scl.x)
#define SMOKE_FRAME(work) ((work)->scl.y)
#define SMOKE_SPREAD(work) ((work)->scl.z)

enum jet_smoke_mode { SMOKE_INIT = 0, SMOKE_PLAY = 1, SMOKE_GONE = 3 };

static void jet_smoke_init(struct task *task, NJS_SPRITE *sprite);
static void jet_smoke_play(struct task *task);
static void jet_smoke_draw(struct task *task);

static NJS_TEXANIM jet_smoke_texanims[SMOKE_FRAMES] = {
#include "stg08/jet_smoke/jet_smoke_texanims.inc"
};

/* the sprite all clouds draw, set up again as each cloud starts */
static NJS_SPRITE jet_smoke_sprite;

static void jet_smoke_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SMOKE_INIT:
        jet_smoke_init(task, &jet_smoke_sprite);
        break;
    case SMOKE_PLAY:
        jet_smoke_play(task);
        jet_smoke_draw(task);
        break;
    case SMOKE_GONE:
        /* @hack redundant: task_kill's call in a loop of one pass, which
         * keeps it a JSR that returns here (JSR @R2 at 0x0c901094); alone,
         * it becomes a tail jump, the function runs 2 bytes short and 8 of 31
         * words differ */
        do {
            task_kill(task);
        } while (0);
        break;
    /* @hack redundant: the default's return, which SHC places right after the
     * compares (RTS at 0x0c901078); without it, the default branches to the
     * shared return, the function runs 2 bytes short and 23 of 31 words
     * differ */
    default:
        return;
    }
}

static void jet_smoke_init(struct task *task, NJS_SPRITE *sprite)
{
    struct task_work *work = task->work;

    work->mode = SMOKE_PLAY;
    work->timer = 0;
    sprite->tlist = &jet_smk_texlist;
    sprite->tanim = jet_smoke_texanims;
    sprite->sx = SMOKE_SIZE;
    sprite->sy = SMOKE_SIZE;
    sprite->p.x = 0.0F;
    sprite->p.y = 0.0F;
    sprite->p.z = 0.0F;
}

static void jet_smoke_play(struct task *task)
{
    struct task_work *work = task->work;

    if (SMOKE_FRAME(work) < SMOKE_SLOW) {
        SMOKE_FRAME(work) += SMOKE_RATE;
    }
    if (SMOKE_FRAME(work) >= SMOKE_SLOW && SMOKE_FRAME(work) < SMOKE_LAST) {
        SMOKE_FRAME(work) += SMOKE_SLOW_RATE;
    }
    if (SMOKE_FRAME(work) >= SMOKE_LAST) {
        work->mode = SMOKE_GONE;
        SMOKE_FRAME(work) = SMOKE_LAST;
    }
}

/* a cloud draws only in a frame's first step, and adds its color to the
 * color behind it */
static void jet_smoke_draw(struct task *task)
{
    struct task_work *work = task->work;
    Uint32 frame = (Uint32)SMOKE_FRAME(work);
    Float width;
    Float height;
    Float depth;

    if (frame_step == 0) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njSetTexture(&jet_smk_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, camera_work->ang.y);
        TURN(njRotateX, camera_work->ang.x);
        TURN(njRotateZ, work->ang.z);
        /* @hack spelling: the scales in two chained assignments, which store
         * height and width on the stack (0x0c9011f8, 0x0c901202); set apart,
         * width takes FR15 and 51 of 108 words differ */
        depth = height = SMOKE_HEIGHT(work);
        depth = width = depth + SMOKE_SPREAD(work);
        njScale(NULL, width, height, depth);
        njFogDisable();
        njDrawSprite3D(&jet_smoke_sprite, frame,
                       NJD_SPRITE_VFLIP | NJD_SPRITE_ALPHA);
        njFogEnable();
        njPopMatrix(1);
    }
}

void jet_smoke_create(NJS_POINT3 *pos, Float height, Float frame, Float spread)
{
    struct task *task = task_create(TASK_WORK, 6, jet_smoke_step);
    struct task_work *work;

    if (task != NULL) {
        work = task->work;
        work->pos = *pos;
        SMOKE_HEIGHT(work) = height;
        SMOKE_FRAME(work) = frame;
        SMOKE_SPREAD(work) = spread;
    }
}
