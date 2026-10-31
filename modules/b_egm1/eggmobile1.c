/* B_EGM1's stage task, which starts the fight with the Egg Hornet, and its
 * sky's task */
#include <ninja.h>

#include "b_egm1/barrier.h"
#include "b_egm1/effect_textures.h"
#include "b_egm1/egg_hornet.h"
#include "b_egm1/eggmobile1.h"
#include "b_egm1/jets.h"
#include "b_egm1/music.h"
#include "camera/camera.h"
#include "draw/tree.h"
#include "hud/boss_name.h"
#include "stage/stage.h"
#include "system/task.h"

#define TEXT_1_LENGTH 10
#include "b_egm1/eggmobile1/text.inc"

enum eggmobile1_mode { EGGMOBILE1_START, EGGMOBILE1_FIGHT, EGGMOBILE1_END };

/* the sky task's modes, in its work's mode. nothing in B_EGM1 sets
 * SKY_RESET */
enum sky_mode { SKY_START, SKY_DRAW, SKY_RESET };

static void eggmobile1_sky_draw(struct task *task);
static void eggmobile1_sky_destroy(struct task *task);

static void eggmobile1_start(struct task *task)
{
    struct task_work *work = task->work;

    /* @hack redundant: a call of stage_get_stage_act whose result nothing
     * reads, the JSR at 0x0c904ee8. the call only reads the stage and act, so
     * it changes nothing. without it, the object's P ends 0x20 bytes short of
     * its row, and the link fails; with the row's end moved to match, 25 of
     * 32 words differ */
    stage_get_stage_act();
    egg_hornet_create();
    boss_name_create(TEXT_1, -1, 240, 120);
    fun_0c900bc0();
    fun_0c904188();
    eggmobile1_music_start();
    work->mode = EGGMOBILE1_FIGHT;
}

void eggmobile1_sky_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SKY_START:
        task->destructor = eggmobile1_sky_destroy;
        task->draw = eggmobile1_sky_draw;
        work->mode = SKY_DRAW;
        /* fall through: the sky draws from this step */
    case SKY_DRAW:
        eggmobile1_sky_draw(task);
        break;
    case SKY_RESET:
        work->mode = SKY_START;
        break;
    default:
        /* @bug memory: a mode past SKY_RESET would pass heap_free the task,
         * which heap_alloc didn't give (JMP at 0x0c904f4c). nothing in B_EGM1
         * sets such a mode */
        heap_free(task);
        break;
    }
}

static void eggmobile1_sky_draw(struct task *task)
{
    njClipZ(-3.0F, -10000.0F);
    njFogDisable();
    njSetTexture(&eggmobile1_sky_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, camera_work->pos.x, 0.0F, camera_work->pos.z);
    draw_tree(&eggmobile1_sky_object);
    njPopMatrix(1);
    njFogEnable();
}

static void eggmobile1_sky_destroy(struct task *task)
{
}

/* @hack spelling: the music's stop in a function of its own, which SHC
 * inlines into eggmobile1_step as a JSR (0x0c90001e); called there directly,
 * it becomes a tail jump, and 20 of 20 words differ */
static void eggmobile1_end(void)
{
    eggmobile1_music_stop();
}

/* #pragma section HEAD puts the step in PHEAD, which B_EGM1's link places
 * first, at 0x0c900000. 1ST_READ.BIN's stage_steps holds that address */
#pragma section HEAD
void eggmobile1_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case EGGMOBILE1_START:
        eggmobile1_start(task);
        break;
    case EGGMOBILE1_FIGHT:
        break;
    case EGGMOBILE1_END:
        eggmobile1_end();
        break;
    }
}
#pragma section
