/* the Chao's sparkles. a glint is one sparkle. spirals, trails and bursts
 * leave glints behind them, and flecks of glitter drift down */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/models.h"
#include "al_main/sparkle.h"
#include "draw/draw.h"
#include "effect/sparkle.h" /* NOLINT(misc-include-cleaner) */
#include "object/placement.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

struct sparkle_vars {
    Float frame; /* the sprite frame a glint or a fleck shows */
    Uint8 unk_04[4];
    Float rate; /* the sprite frames a fleck plays each step */
};

#define VARS(work) ((struct sparkle_vars *)&(work)->vars)

/* what a sparkle keeps in its work's scl. a glint keeps its scale in scl.x
 * and its speed up y in scl.y. a trail or a fleck keeps its speed in scl, as
 * a vector */
#define SCALE(work) ((work)->scl.x)
#define RISE(work) ((work)->scl.y)
#define SPEED(work) (&(work)->scl)

/* the sprite frames a glint and a fleck play */
#define GLINT_FRAMES 4.0F
#define FLECK_FRAMES 8.0F

/* the steps a trail lives as it gathers, and as it bursts */
#define GATHER_STEPS 30
#define BURST_STEPS 15

/* the spiral turns a sixteenth of a turn each step, and ends at three turns */
#define SPIRAL_TURN 0x1000
#define SPIRAL_END 0x30000

/* a random Float above -0.5, and up to 0.5 */
#define SPREAD() (-(rand() * RAND_SCALE) + 0.5F)

static NJS_TEXANIM glint_texanims[4] = {
#include "al_main/sparkle/glint_texanims.inc"
};
static NJS_SPRITE glint_sprite = {
#include "al_main/sparkle/glint_sprite.inc"
};
static NJS_TEXANIM fleck_texanims[8] = {
#include "al_main/sparkle/fleck_texanims.inc"
};
static NJS_SPRITE fleck_sprite = {
#include "al_main/sparkle/fleck_sprite.inc"
};

/* a glint's step, which runs only on a frame's first step. each run plays a
 * third of a sprite frame, and the glint sinks faster. it draws added to what
 * is behind it. placement_try_leave ends it early once it is out of
 * range */
static void alife_sparkle_step(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step != 0) {
        return;
    }
    /* @bug memory: the step that kills the glint, once its frame
     * reaches GLINT_FRAMES (0x0c906a08), goes on to draw it at frame 4
     * (0x0c906a86, 0x0c906a8c). glint_texanims ends at frame 3, so the
     * texanim of frame 4 is glint_sprite's first 20 bytes */
    if ((VARS(work)->frame += 1.0F / 3.0F) >= GLINT_FRAMES) {
        task_kill(task);
    }
    work->pos.y += RISE(work);
    RISE(work) -= 0.01F;
    draw_set_material(0.3F, 1.0F, 1.0F, 1.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    texture_set_regular();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njScale(NULL, SCALE(work), SCALE(work), SCALE(work));
    njDrawSprite3D(&glint_sprite, (Uint32)VARS(work)->frame,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    placement_try_leave(task);
}

void alife_sparkle_create(NJS_POINT3 *pos, Float scale)
{
    struct task *task;
    struct task_work *work;

    task = task_create(TASK_WORK, 6, alife_sparkle_step);
    if (task != NULL) {
        work = task->work;
        work->pos = *pos;
        SCALE(work) = scale;
        RISE(work) = 0.0F;
    }
}

/* each step turns the spiral SPIRAL_TURN further round its pos, and leaves
 * a glint there, 3 out. the glints rise 1 a turn, each within 0.5 of that at
 * random */
static void alife_sparkle_spiral_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;

    work->ang.x += SPIRAL_TURN;
    pos = work->pos;
    pos.x += 3.0F * njCos(work->ang.x);
    pos.y += -(rand() * RAND_SCALE) + (work->ang.x * (1.0F / 65536.0F)) + 0.5F;
    pos.z += 3.0F * njSin(work->ang.x);
    alife_sparkle_create(&pos, 1.0F);
    if (work->ang.x >= SPIRAL_END) {
        task_kill(task);
    }
}

void alife_sparkle_spiral_create(NJS_POINT3 *pos)
{
    struct task *task;
    struct task_work *work;

    if ((task = task_create(TASK_WORK, 6, alife_sparkle_spiral_step)) != NULL) {
        work = task->work;
        work->pos = *pos;
    }
}

static void alife_sparkle_gather_step(struct task *task)
{
    struct task_work *work = task->work;

    if (++TASK_TIMER(work) >= GATHER_STEPS) {
        task_kill(task);
        return;
    }
    if (!(++work->tick & 15)) {
        alife_sparkle_create(&work->pos, 1.0F);
    }
    njAddVector(&work->pos, SPEED(work));
}

/* each trail starts 5 from pos, a random way. each step takes it
 * 0.8 / GATHER_STEPS of the way in */
void alife_sparkle_gather_create(NJS_POINT3 *pos)
{
    struct task *task;
    struct task_work *work;
    NJS_VECTOR out;
    Angle ang;
    int i;

    for (i = 0; i < 10; i++) {
        if ((task = task_create(TASK_WORK, 6, alife_sparkle_gather_step)) !=
            NULL) {
            work = task->work;
            ang = RANDOM_ANGLE();
            out.x = njCos(ang);
            out.z = njSin(ang);
            ang = RANDOM_ANGLE();
            out.y = njSin(ang);
            out.y *= 5.0F;
            out.x *= 5.0F * njCos(ang);
            out.z *= 5.0F * njCos(ang);
            work->pos = *pos;
            njAddVector(&work->pos, &out);
            out.x *= -0.8F / GATHER_STEPS;
            out.y *= -0.8F / GATHER_STEPS;
            out.z *= -0.8F / GATHER_STEPS;
            *SPEED(work) = out;
            work->tick = rand() * (255.0F * RAND_SCALE);
        }
    }
}

static void alife_sparkle_burst_step(struct task *task)
{
    struct task_work *work = task->work;

    if (--TASK_TIMER(work) <= 0) {
        task_kill(task);
        return;
    }
    if (!(++work->tick & 7)) {
        alife_sparkle_create(&work->pos, 1.0F);
    }
    if ((SPEED(work)->y += -0.1F) < -1.0F) {
        SPEED(work)->y = -1.0F;
    }
    njAddVector(&work->pos, SPEED(work));
}

void alife_sparkle_burst_create(NJS_POINT3 *pos)
{
    struct task *task;
    struct task_work *work;
    int i;

    for (i = 0; i < 10; i++) {
        if ((task = task_create(TASK_WORK, 6, alife_sparkle_burst_step)) !=
            NULL) {
            work = task->work;
            work->pos = *pos;
            SPEED(work)->x = SPREAD();
            SPEED(work)->y = rand() * RAND_SCALE;
            SPEED(work)->z = SPREAD();
            work->tick = rand() * (255.0F * RAND_SCALE);
            TASK_TIMER(work) = BURST_STEPS;
        }
    }
}

static void fleck_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step != 0) {
        return;
    }
    draw_set_material(0.3F, 1.0F, 1.0F, 1.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetTexture(&alife_objects_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njDrawSprite3D(&fleck_sprite, (Uint32)VARS(work)->frame,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
}

static void fleck_step(struct task *task)
{
    struct task_work *work = task->work;

    /* @bug memory: the step that kills the fleck, once its frame
     * reaches FLECK_FRAMES (0x0c906fee), goes on to fleck_draw, which on a
     * frame's first step draws it at frame 8 (0x0c906f9e, 0x0c906fa4).
     * fleck_texanims ends at frame 7, so the texanim of frame 8 is
     * fleck_sprite's first 20 bytes */
    if ((VARS(work)->frame += VARS(work)->rate) >= FLECK_FRAMES) {
        task_kill(task);
    }
    SPEED(work)->x *= 0.99F;
    SPEED(work)->y += -0.001F;
    SPEED(work)->z *= 0.99F;
    njAddVector(&work->pos, SPEED(work));
    fleck_draw(task);
}

void alife_glitter_create_slow(NJS_POINT3 *pos)
{
    struct task *task;
    struct task_work *work;
    NJS_VECTOR off;

    if ((task = task_create(TASK_WORK, 6, fleck_step)) != NULL) {
        work = task->work;
        off.x = SPREAD();
        off.y = SPREAD();
        off.z = SPREAD();
        work->pos = *pos;
        njAddVector(&work->pos, &off);
        off.x *= 0.1F;
        off.y *= 0.1F;
        off.z *= 0.1F;
        *SPEED(work) = off;
        VARS(work)->rate = FLECK_FRAMES / 90.0F;
    }
}

void alife_glitter_create(NJS_POINT3 *pos)
{
    struct task *task;
    struct task_work *work;
    NJS_VECTOR off;

    if ((task = task_create(TASK_WORK, 6, fleck_step)) != NULL) {
        work = task->work;
        off.x = SPREAD();
        off.y = SPREAD();
        off.z = SPREAD();
        work->pos = *pos;
        njAddVector(&work->pos, &off);
        *SPEED(work) = off;
        VARS(work)->rate = FLECK_FRAMES / 30.0F;
    }
}
