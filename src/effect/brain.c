/* Chaos's brain: CHAOS_BRAINFRAME's sixteen frames, played once at a
 * task's place, or looped at a point the brain follows */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "draw/draw.h"
#include "effect/brain.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 10
#define TEXT_9_LENGTH 10
#define TEXT_10_LENGTH 10
#define TEXT_11_LENGTH 10
#define TEXT_12_LENGTH 10
#define TEXT_13_LENGTH 10
#define TEXT_14_LENGTH 10
#define TEXT_15_LENGTH 10
#define TEXT_16_LENGTH 10
#include "effect/brain/text.inc"

/* CHAOS_BRAINFRAME's frames, and the steps each one shows */
#define BRAIN_FRAMES 16
#define BRAIN_WAIT 2

enum brain_mode { BRAIN_INIT, BRAIN_PLAY };

struct brain_vars {
    NJS_POINT3 *follow;
    /* @hack type: the two cleared together through a Sint32, MOV #0 and one
     * MOV.L store (0x8c0fdc82, 0x8c0fdc84 in brain_once_step); cleared one by
     * one, 79 of the two steps' 95 words differ */
    /* @hack type: the two as an array, which SHC reaches through an address
     * it forms (MOV R5,R2 and ADD #12,R2 at 0x8c0fdca8, 0x8c0fdcaa); as
     * fields, they take displacement loads, and 64 of the 201 words of the
     * four functions that use them differ */
    union {
        Sint32 both;
        /* the steps the current frame has shown, then that frame, which
         * WAIT and FRAME read */
        Sint16 count[2];
    } time;
};

#define VARS(work) ((struct brain_vars *)&(work)->vars)
#define WAIT(work) (VARS(work)->time.count[0])
#define FRAME(work) (VARS(work)->time.count[1])

/* as many as the texanims in brain_frames */
/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const Sint32 brain_texanim_count = 2;

static NJS_TEXNAME chaos_brainframe_textures[BRAIN_FRAMES] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
};
NJS_TEXLIST chaos_brainframe_texlist = {
#include "effect/brain/chaos_brainframe_texlist.inc"
};

/* the texanim of the brain that plays once, then the looping brain's.
 * each draw points its texanim at the texture it shows */
static NJS_TEXANIM brain_frames[2] = {
#include "effect/brain/brain_frames.inc"
};
static NJS_SPRITE brain_once_sprite = {
#include "effect/brain/brain_once_sprite.inc"
};
static NJS_SPRITE brain_sprite = {
#include "effect/brain/brain_sprite.inc"
};

Sint8 brain_shown;
/* the looping brains' offset from the point each follows, in the camera's
 * frame */
static const NJS_POINT3 *brain_offset;

static void brain_once_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        brain_frames[0].texid = FRAME(work);
        njDrawSprite3D(&brain_once_sprite, 0,
                       NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
        njPopMatrix(1);
    }
}

void brain_once_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case BRAIN_INIT:
        VARS(work)->time.both = 0;
        work->mode = BRAIN_PLAY;
        brain_once_sprite.sx = brain_once_sprite.sy = 1.0F;
        task->draw = brain_once_draw;
        break;
    case BRAIN_PLAY:
        WAIT(work)++;
        if (WAIT(work) >= BRAIN_WAIT) {
            WAIT(work) -= BRAIN_WAIT;
            FRAME(work)++;
            if (FRAME(work) >= BRAIN_FRAMES) {
                task_destroy(task);
                return;
            }
        }
        break;
    }
    brain_once_draw(task);
}

static void brain_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (brain_shown) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, camera_work->ang.y);
        TURN(njRotateX, camera_work->ang.x);
        /* njTranslateV takes a mutable vector but only reads it */
        njTranslateV(NULL, (NJS_VECTOR *)brain_offset);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        brain_frames[1].texid = FRAME(work);
        njDrawSprite3D(&brain_sprite, 1, NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
        njPopMatrix(1);
    }
}

static void brain_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case BRAIN_INIT:
        VARS(work)->time.both = 0;
        work->mode = BRAIN_PLAY;
        task->draw = brain_draw;
        break;
    case BRAIN_PLAY:
        WAIT(work)++;
        if (WAIT(work) >= BRAIN_WAIT) {
            WAIT(work) -= BRAIN_WAIT;
            FRAME(work)++;
            if (FRAME(work) >= BRAIN_FRAMES) {
                FRAME(work) = 0;
            }
        }
        if (VARS(work)->follow != NULL) {
            work->pos = *VARS(work)->follow;
        }
        break;
    }
    brain_draw(task);
}

struct task *brain_create(struct task *boss, Float scale, NJS_POINT3 *follow,
                          const NJS_POINT3 *offset)
{
    struct task *task;
    struct task_work *work;

    task = task_create(TASK_WORK, 2, brain_step);
    if (task != NULL) {
        work = task->work;
        brain_sprite.sx = brain_sprite.sy = scale;
        VARS(work)->follow = follow;
        brain_offset = offset;
    }
    brain_shown = 1;
    return task;
}
