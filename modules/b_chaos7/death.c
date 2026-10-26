/* the three effects that play as Perfect Chaos dies */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos7/death.h"
#include "draw/draw.h"
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
#define TEXT_17_LENGTH 10
#define TEXT_18_LENGTH 10
#define TEXT_19_LENGTH 10
#define TEXT_20_LENGTH 10
#define TEXT_21_LENGTH 10
#define TEXT_22_LENGTH 10
#define TEXT_23_LENGTH 10
#define TEXT_24_LENGTH 10
#include "b_chaos7/death/text.inc"

extern NJS_ACTION death_1_action;
extern NJS_ACTION death_2_action;
extern NJS_ACTION death_3_action;
extern NJS_OBJECT death_1_object;
extern NJS_OBJECT death_2_object;
extern NJS_OBJECT death_3_object;

/* the last of each action's 32 frames. the first two effects' frames reach it
 * on their 61st step and hold it there. the third's reaches it on its 121st */
#define DEATH_LAST_FRAME 31.0F
#define DEATH_PACE (DEATH_LAST_FRAME / 60.0F)
#define DEATH_3_PACE (DEATH_LAST_FRAME / 120.0F)

/* each effect cycles through its eight textures, a texture every DEATH_TICKS
 * steps. the effects' first textures are 0, DEATH_2_FIRST and DEATH_3_FIRST
 * in chaos7_death_texlist */
#define DEATH_TEXTURES 8
#define DEATH_TICKS 2
#define DEATH_2_FIRST 8
#define DEATH_3_FIRST 16

/* the first two effects fade in by DEATH_1_FADE and DEATH_2_FADE a step. the
 * third fades in up to its action's frame DEATH_3_IN, and out from DEATH_3_OUT
 * to the last. it plays its action DEATH_3_TIMES times, turning DEATH_3_TURN a
 * step about y */
#define DEATH_1_FADE 0.125F
#define DEATH_2_FADE 0.25F
#define DEATH_3_IN 8.0F
#define DEATH_3_OUT 20.0F
#define DEATH_3_TIMES 2
#define DEATH_3_TURN 0x100

struct death_vars {
    Float frame; /* the action's frame */
    Sint8 ticks; /* steps since the texture last changed */
    Sint8 texture; /* 0 to 7, counted from the effect's first */
    Sint8 times; /* the third effect's plays left, counting this one */
    Uint8 unk_07;
    Float alpha;
};

#define VARS(work) ((struct death_vars *)&(work)->vars)

/* the effects' first textures, in a texlist of their own that nothing
 * reads */
static NJS_TEXNAME death_first_textures[3] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
};
static NJS_TEXLIST death_first_texlist = {
#include "b_chaos7/death/death_first_texlist.inc"
};
static NJS_TEXNAME chaos7_death_textures[24] = {
    {TEXT_1, 0, 0},  {TEXT_4, 0, 0},  {TEXT_5, 0, 0},  {TEXT_6, 0, 0},
    {TEXT_7, 0, 0},  {TEXT_8, 0, 0},  {TEXT_9, 0, 0},  {TEXT_10, 0, 0},
    {TEXT_2, 0, 0},  {TEXT_11, 0, 0}, {TEXT_12, 0, 0}, {TEXT_13, 0, 0},
    {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0}, {TEXT_17, 0, 0},
    {TEXT_3, 0, 0},  {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
};
NJS_TEXLIST chaos7_death_texlist = {
#include "b_chaos7/death/chaos7_death_texlist.inc"
};

static void death_1_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;
    NJS_MATERIAL *material;

    if (frame_step == 0) {
        /* @hack spelling: the model read at 4 past the object's address (MOV.L
         * @(4,R4),R4 at 0x0c91a816); without it, SHC pools the model
         * member's own address, and 18 of 72 words and 1 pool word differ */
        material = (&death_1_object)->model->mats;
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        material->attr_texId = VARS(work)->texture;
        color.a = VARS(work)->alpha;
        color.r = color.g = color.b = 1.0F;
        njFogDisable();
        draw_control_save();
        draw_control_set(NJD_CONTROL_3D_CONSTANT_MATERIAL);
        njSetConstantMaterial(&color);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njScale(NULL, 1.0F, 1.0F, 1.0F);
        njSetTexture(&chaos7_death_texlist);
        draw_action(&death_1_action, VARS(work)->frame);
        njPopMatrix(1);
        draw_control_restore();
        njFogEnable();
    }
}

static void death_1_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->alpha += DEATH_1_FADE;
    VARS(work)->alpha = VARS(work)->alpha < 1.0F ? VARS(work)->alpha : 1.0F;
    if (++VARS(work)->ticks >= DEATH_TICKS) {
        VARS(work)->ticks = 0;
        if (++VARS(work)->texture >= DEATH_TEXTURES) {
            VARS(work)->texture = 0;
        }
    }
    VARS(work)->frame = VARS(work)->frame + DEATH_PACE < DEATH_LAST_FRAME
                            ? VARS(work)->frame + DEATH_PACE
                            : DEATH_LAST_FRAME;
    death_1_draw(task);
}

void chaos7_death_1_start(struct task *task)
{
    struct task_work *work = task->work;

    task->step = death_1_step;
    task->draw = death_1_draw;
    VARS(work)->alpha = 0.0F;
    VARS(work)->frame = 0.0F;
}

static void death_2_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;
    NJS_MATERIAL *material;

    if (frame_step == 0) {
        /* @hack spelling: the model read at 4 past the object's address (MOV.L
         * @(4,R4),R4 at 0x0c91a996); without it, SHC pools the model
         * member's own address, and 18 of 73 words and 1 pool word differ */
        material = (&death_2_object)->model->mats;
        material->attr_texId = VARS(work)->texture + DEATH_2_FIRST;
        color.a = VARS(work)->alpha;
        color.r = color.g = color.b = 1.0F;
        njFogDisable();
        draw_control_save();
        draw_control_set(NJD_CONTROL_3D_CONSTANT_MATERIAL);
        njSetConstantMaterial(&color);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njScale(NULL, 1.0F, 1.0F, 1.0F);
        njSetTexture(&chaos7_death_texlist);
        draw_action(&death_2_action, VARS(work)->frame);
        njPopMatrix(1);
        draw_control_restore();
        njFogEnable();
    }
}

static void death_2_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->alpha += DEATH_2_FADE;
    VARS(work)->alpha = VARS(work)->alpha < 1.0F ? VARS(work)->alpha : 1.0F;
    if (++VARS(work)->ticks >= DEATH_TICKS) {
        VARS(work)->ticks = 0;
        if (++VARS(work)->texture >= DEATH_TEXTURES) {
            VARS(work)->texture = 0;
        }
    }
    VARS(work)->frame = VARS(work)->frame + DEATH_PACE < DEATH_LAST_FRAME
                            ? VARS(work)->frame + DEATH_PACE
                            : DEATH_LAST_FRAME;
    death_2_draw(task);
}

void chaos7_death_2_start(struct task *task)
{
    struct task_work *work = task->work;

    task->step = death_2_step;
    task->draw = death_2_draw;
    VARS(work)->alpha = 0.0F;
    VARS(work)->frame = 0.0F;
}

static void death_3_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;
    Angle ang;
    NJS_MATERIAL *material;

    if (frame_step == 0) {
        /* @hack spelling: the model read at 4 past the object's address (MOV.L
         * @(4,R4),R4 at 0x0c91ab16); without it, SHC pools the model
         * member's own address, and 17 of 83 words and 1 pool word differ */
        material = (&death_3_object)->model->mats;
        material->attr_texId = VARS(work)->texture + DEATH_3_FIRST;
        color.a = VARS(work)->alpha;
        color.r = color.g = color.b = 1.0F;
        njFogDisable();
        draw_control_save();
        draw_control_set(NJD_CONTROL_3D_CONSTANT_MATERIAL);
        njSetConstantMaterial(&color);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScale(NULL, 1.0F, 1.0F, 1.0F);
        njSetTexture(&chaos7_death_texlist);
        draw_action(&death_3_action, VARS(work)->frame);
        njPopMatrix(1);
        draw_control_restore();
        njFogEnable();
    }
}

static void death_3_step(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->frame < DEATH_3_IN) {
        VARS(work)->alpha = VARS(work)->frame * (1.0F / DEATH_3_IN);
    } else if (VARS(work)->frame > DEATH_3_OUT) {
        VARS(work)->alpha = (DEATH_LAST_FRAME - VARS(work)->frame) *
                            (1.0F / (DEATH_LAST_FRAME - DEATH_3_OUT));
    } else {
        VARS(work)->alpha = 1.0F;
    }
    if (++VARS(work)->ticks >= DEATH_TICKS) {
        VARS(work)->ticks = 0;
        if (++VARS(work)->texture >= DEATH_TEXTURES) {
            VARS(work)->texture = 0;
        }
    }
    VARS(work)->frame += DEATH_3_PACE;
    if (VARS(work)->frame >= DEATH_LAST_FRAME) {
        VARS(work)->times--;
        if (VARS(work)->times > 0) {
            VARS(work)->frame = 0.0F;
        } else {
            task_kill(task);
            return;
        }
    }
    work->ang.y += DEATH_3_TURN;
    death_3_draw(task);
}

void chaos7_death_3_start(struct task *task)
{
    struct task_work *work = task->work;

    task->step = death_3_step;
    task->draw = death_3_draw;
    VARS(work)->alpha = 0.0F;
    VARS(work)->frame = 0.0F;
    VARS(work)->times = DEATH_3_TIMES;
}
