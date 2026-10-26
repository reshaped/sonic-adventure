/* Syuujin b, Syuujin r and Syuujin g: prisoners who stir where they sit.
 * Syuujin b also swells along x, and Syuujin r along y */
#include <ninja.h>

#include "debug/editor.h"
#include "draw/draw.h"
#include "object/common.h"
#include "object/placement.h"
#include "stg05/prisoner.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 11
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 12
#define TEXT_9_LENGTH 12
#define TEXT_10_LENGTH 11
#define TEXT_11_LENGTH 11
#include "stg05/prisoner/text.inc"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_ACTION prisoner_action;

#define PRISONER_LAST_FRAME 29.0F
/* the action's frames each step, for each unit of the set entry's z angle */
#define PRISONER_PACE 0.1F

/* the swell's phase, an angle that moves on by SWELL_STEP each step. the
 * swell sets one scale to 1 plus SWELL_SIZE times its sine, in place of the
 * set entry's */
#define SWELL_PHASE(work) TASK_TIMER(work)
#define SWELL_STEP 1392
#define SWELL_SIZE 0.1F

/* the prisoners' texture names, which differ only in the clothes:
 * ai16_fuku1a, ai16_fuku1b and ai16_fuku1c. no code reads them. the
 * prisoners draw from obj_mountain_texlist */
static NJS_TEXNAME prisoner_b_textures[9] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0}, {TEXT_6, 0, 0},
    {TEXT_7, 0, 0}, {TEXT_8, 0, 0}, {TEXT_9, 0, 0},
};
static NJS_TEXNAME prisoner_r_textures[9] = {
    {TEXT_1, 0, 0}, {TEXT_10, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0},  {TEXT_6, 0, 0},
    {TEXT_7, 0, 0}, {TEXT_8, 0, 0},  {TEXT_9, 0, 0},
};
static NJS_TEXNAME prisoner_g_textures[9] = {
    {TEXT_1, 0, 0}, {TEXT_11, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0},  {TEXT_6, 0, 0},
    {TEXT_7, 0, 0}, {TEXT_8, 0, 0},  {TEXT_9, 0, 0},
};

static void prisoner_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&obj_mountain_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScaleV(NULL, &work->scl);
    draw_action(ACTION(work)->action, ACTION(work)->frame);
    njPopMatrix(1);
}

static void prisoner_swell_x(struct task *task)
{
    struct task_work *work;

    if (!placement_try_leave(task)) {
        work = task->work;
        /* @hack spelling: the frame tested in the addition, which compares
         * the sum in FR2 (FCMP/GT at 0x0c90f9ca); as two statements, the test
         * reads the frame back, the function is 2 bytes longer and 25 of 50
         * words differ */
        if ((ACTION(work)->frame += work->ang.z * PRISONER_PACE) >
            PRISONER_LAST_FRAME) {
            ACTION(work)->frame = 0.0F;
        }
        SWELL_PHASE(work) += SWELL_STEP;
        work->scl.x = 1.0F + (njSin(SWELL_PHASE(work)) * SWELL_SIZE);
        prisoner_draw(task);
    }
}

static void prisoner_swell_y(struct task *task)
{
    struct task_work *work;

    if (!placement_try_leave(task)) {
        work = task->work;
        /* @hack spelling: the frame tested in the addition, which compares
         * the sum in FR2 (FCMP/GT at 0x0c90fa72); as two statements, the test
         * reads the frame back, the function is 2 bytes longer and 30 of 50
         * words differ */
        if ((ACTION(work)->frame += work->ang.z * PRISONER_PACE) >
            PRISONER_LAST_FRAME) {
            ACTION(work)->frame = 0.0F;
        }
        SWELL_PHASE(work) += SWELL_STEP;
        work->scl.y = 1.0F + (njSin(SWELL_PHASE(work)) * SWELL_SIZE);
        prisoner_draw(task);
    }
}

static void prisoner_play(struct task *task)
{
    struct task_work *work;

    if (!placement_try_leave(task)) {
        work = task->work;
        /* @hack spelling: the frame tested in the addition, which compares
         * the sum in FR2 (FCMP/GT at 0x0c90fb08); as two statements, the test
         * reads the frame back, the function is 2 bytes longer and 13 of 32
         * words differ */
        if ((ACTION(work)->frame += work->ang.z * PRISONER_PACE) >
            PRISONER_LAST_FRAME) {
            ACTION(work)->frame = 0.0F;
        }
        prisoner_draw(task);
    }
}

void prisoner_b_step(struct task *task)
{
    struct task_work *work = task->work;

    if (work->scl.x == 0.0F) {
        work->scl.x = 1.0F;
    }
    if (work->scl.y == 0.0F) {
        work->scl.y = 1.0F;
    }
    if (work->scl.z == 0.0F) {
        work->scl.z = 1.0F;
    }
    ACTION(work)->texlist = &obj_mountain_texlist;
    ACTION(work)->action = &prisoner_action;
    if (placement_editing(task)) {
        task->step = prisoner_draw;
        return;
    }
    task->draw = prisoner_draw;
    task->step = prisoner_swell_x;
}

void prisoner_r_step(struct task *task)
{
    struct task_work *work = task->work;

    if (work->scl.x == 0.0F) {
        work->scl.x = 1.0F;
    }
    if (work->scl.y == 0.0F) {
        work->scl.y = 1.0F;
    }
    if (work->scl.z == 0.0F) {
        work->scl.z = 1.0F;
    }
    ACTION(work)->texlist = &obj_mountain_texlist;
    ACTION(work)->action = &prisoner_action;
    if (placement_editing(task)) {
        task->step = prisoner_draw;
        return;
    }
    task->draw = prisoner_draw;
    task->step = prisoner_swell_y;
}

void prisoner_g_step(struct task *task)
{
    struct task_work *work = task->work;

    if (work->scl.x == 0.0F) {
        work->scl.x = 1.0F;
    }
    if (work->scl.y == 0.0F) {
        work->scl.y = 1.0F;
    }
    if (work->scl.z == 0.0F) {
        work->scl.z = 1.0F;
    }
    ACTION(work)->texlist = &obj_mountain_texlist;
    ACTION(work)->action = &prisoner_action;
    if (placement_editing(task)) {
        task->step = prisoner_draw;
        return;
    }
    task->draw = prisoner_draw;
    task->step = prisoner_play;
}
