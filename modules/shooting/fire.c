/* the sky chase's fire. FIRE_L is one flame, which shows a frame a step
 * for FIRE_FRAMES steps. pom is six such flames at points on the tornado,
 * which starts them on request. pom takes its name from its textures, pom01
 * to pom08. pom_burn's sprite takes pom_texlist, though it sets
 * fire_l_texlist as the current texlist */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "shooting/fire.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 8
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 5
#define TEXT_10_LENGTH 5
#define TEXT_11_LENGTH 5
#define TEXT_12_LENGTH 5
#define TEXT_13_LENGTH 5
#define TEXT_14_LENGTH 5
#define TEXT_15_LENGTH 5
#define TEXT_16_LENGTH 5
#include "shooting/fire/text.inc"

enum fire_mode { FIRE_INIT, FIRE_BURN, FIRE_GONE };

#define FIRE_FRAMES 8
#define FIRE_SIZE 0.4F

#define POM_POINTS 6

struct fire_vars {
    Uint8 unk_00[8];
    Sint32 frame; /* the frame fire_l_loop draws next */
};

#define FIRE_FRAME(work) (((struct fire_vars *)&(work)->vars)->frame)

static void fire_l_init(struct task *task);
static void fire_l_burn(struct task *task);
static void pom_init(struct task *task);
static void pom_burn(struct task *task);

static NJS_TEXNAME fire_l_textures[FIRE_FRAMES] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};
static NJS_TEXNAME pom_textures[FIRE_FRAMES] = {
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
};
static NJS_TEXLIST fire_l_texlist = {
#include "shooting/fire/fire_l_texlist.inc"
};
static NJS_TEXLIST pom_texlist = {
#include "shooting/fire/pom_texlist.inc"
};
static NJS_TEXANIM fire_l_frames[FIRE_FRAMES] = {
#include "shooting/fire/fire_l_frames.inc"
};
/* where the six burn, in the tornado's model space */
static Float pom_points[POM_POINTS][3] = {
#include "shooting/fire/pom_points.inc"
};

void fire_l_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case FIRE_INIT:
        fire_l_init(task);
        break;
    case FIRE_BURN:
        fire_l_burn(task);
        break;
    case FIRE_GONE:
        /* @hack redundant: the call in a loop of one pass, which keeps it a JSR
         * (0x0c903980) and fire_l_step's push of PR; alone, it becomes a
         * tail jump, fire_l_step runs 4 bytes short and 21 of 21 words
         * differ */
        do {
            task_destroy(task);
        } while (0);
        break;
    }
}

/* start the flame at its parent's pos. it also turns a point 8 above the
 * parent by the parent's angles, and never reads it */
static void fire_l_init(struct task *task)
{
    struct task_work *parent = task->parent->work;
    struct task_work *work = task->work;
    NJS_MATRIX matrix;
    NJS_VECTOR over;
    NJS_VECTOR turned;

    work->mode = FIRE_BURN;
    if (parent != NULL) {
        over.x = over.z = 0.0F;
        over.y = 8.0F;
        njUnitMatrix(&matrix);
        TURN_MATRIX(njRotateZ, &matrix, parent->ang.z);
        TURN_MATRIX(njRotateX, &matrix, parent->ang.x);
        TURN_MATRIX(njRotateY, &matrix, -parent->ang.y);
        njCalcVector(&matrix, &over, &turned);
        work->pos.x = parent->pos.x;
        work->pos.y = parent->pos.y;
        work->pos.z = parent->pos.z;
    }
}

/* draw the next frame each step, added to what's behind it. after
 * FIRE_FRAMES steps the flame goes */
static void fire_l_burn(struct task *task)
{
    struct task_work *work = task->work;
    /* ang stays unset: no draw passes NJD_SPRITE_ANGLE */
    NJS_SPRITE sprite;

    sprite.tlist = &fire_l_texlist;
    sprite.tanim = fire_l_frames;
    sprite.sx = FIRE_SIZE;
    sprite.sy = FIRE_SIZE;
    sprite.p.x = 0.0F;
    sprite.p.y = 0.0F;
    sprite.p.z = 0.0F;
    if (TASK_TIMER(work) < FIRE_FRAMES) {
        TASK_TIMER(work)++;
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njSetTexture(&fire_l_texlist);
        njFogDisable();
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        njDrawSprite3D(&sprite, TASK_TIMER(work) & (FIRE_FRAMES - 1),
                       NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
        njPopMatrix(1);
    } else {
        work->timer = 15;
        work->mode = FIRE_GONE;
    }
}

void fire_l_loop(struct task_work *work)
{
    /* ang stays unset: no draw passes NJD_SPRITE_ANGLE */
    NJS_SPRITE sprite;

    sprite.tlist = &fire_l_texlist;
    sprite.tanim = fire_l_frames;
    sprite.sx = FIRE_SIZE;
    sprite.sy = FIRE_SIZE;
    sprite.p.x = 0.0F;
    sprite.p.y = 0.0F;
    sprite.p.z = 0.0F;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetTexture(&fire_l_texlist);
    njFogDisable();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njDrawSprite3D(&sprite, FIRE_FRAME(work),
                   NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
    njPopMatrix(1);
    FIRE_FRAME(work)++;
    if (FIRE_FRAME(work) >= FIRE_FRAMES) {
        FIRE_FRAME(work) = 0;
    }
}

void pom_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case FIRE_INIT:
        pom_init(task);
        break;
    case FIRE_BURN:
        pom_burn(task);
        break;
    case FIRE_GONE:
        /* @hack redundant: the call in a loop of one pass, as in fire_l_step,
         * which keeps it a JSR (0x0c903bc8); alone, it becomes a tail jump
         * and 21 of 21 words differ */
        do {
            task_destroy(task);
        } while (0);
        break;
    }
}

/* as fire_l_init does, pom_init turns a point over the parent and never
 * reads it. nothing reads the pos it sets either: pom_burn draws at the
 * parent's */
static void pom_init(struct task *task)
{
    struct task_work *parent = task->parent->work;
    struct task_work *work = task->work;
    NJS_MATRIX matrix;
    NJS_VECTOR over;
    NJS_VECTOR turned;

    work->mode = FIRE_BURN;
    if (parent != NULL) {
        over.x = over.z = 0.0F;
        over.y = 8.0F;
        njUnitMatrix(&matrix);
        TURN_MATRIX(njRotateZ, &matrix, parent->ang.z);
        TURN_MATRIX(njRotateX, &matrix, parent->ang.x);
        TURN_MATRIX(njRotateY, &matrix, -parent->ang.y);
        njCalcVector(&matrix, &over, &turned);
        work->pos.x = 10.0F;
        work->pos.y = 0.0F;
        work->pos.z = 0.0F;
        work->timer = 0;
    }
}

/* draw the next frame each step at each of the POM_POINTS, which turn as
 * the parent turns. after FIRE_FRAMES steps the flames go */
static void pom_burn(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    /* ang stays unset: no draw passes NJD_SPRITE_ANGLE */
    NJS_SPRITE sprite;
    int i;

    sprite.tlist = &pom_texlist;
    sprite.tanim = fire_l_frames;
    sprite.sx = FIRE_SIZE;
    sprite.sy = FIRE_SIZE;
    sprite.p.x = 0.0F;
    sprite.p.y = 0.0F;
    sprite.p.z = 0.0F;
    if (TASK_TIMER(work) < FIRE_FRAMES) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njSetTexture(&fire_l_texlist);
        draw_set_material(-0.5F, 0.0F, 0.0F, 0.0F);
        njFogDisable();
        njPushMatrix(NULL);
        njTranslate(NULL, parent->pos.x, parent->pos.y, parent->pos.z);
        njRotateXYZ(NULL, parent->ang.x, parent->ang.y, parent->ang.z);
        for (i = 0; i < POM_POINTS; i++) {
            njPushMatrix(NULL);
            njTranslate(NULL, pom_points[i][0], pom_points[i][1],
                        pom_points[i][2]);
            njDrawSprite3D(&sprite, TASK_TIMER(work),
                           NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
            njPopMatrix(1);
        }
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        work->timer++;
    } else {
        work->mode = FIRE_GONE;
    }
}
