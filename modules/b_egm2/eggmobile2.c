/* the Egg Walker's stage: the steps of its stage task and its sky task */
#include <ninja.h>

#include "adv00/station_square.h"
#include "b_egm2/egg_walker.h"
#include "b_egm2/eggmobile2.h"
#include "draw/light.h"
#include "hud/boss_name.h"
#include "hud/hud.h"
#include "player/player.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/task.h"

enum eggmobile2_mode {
    EGGMOBILE2_START,
    EGGMOBILE2_LOAD,
    EGGMOBILE2_FIGHT,
    EGGMOBILE2_IDLE
};

/* the sky task's work->mode. SKY_RESET starts the sky over. a mode past it
 * passes the task to heap_free */
enum sky_mode { SKY_START, SKY_RUN, SKY_RESET };

/* eggmobile2_keep_player_in holds player 0 within these bounds, the
 * stretch of street the fight takes up */
#define BOUND_X_MIN (-515.0F)
#define BOUND_X_MAX (-375.0F)
#define BOUND_Y_MIN (-3.0F)
#define BOUND_Y_MAX 45.0F
#define BOUND_Z_MIN 930.0F
#define BOUND_Z_MAX 1480.0F

/* the color behind the land, an ARGB of blue at 30 of 255: near black */
#define BACK_COLOR 0x1e

static const char eggmobile2_boss_name[11] = {
#include "b_egm2/eggmobile2/eggmobile2_boss_name.inc"
};

static void eggmobile2_fight_start(struct task *task);
static void eggmobile2_keep_player_in(void);

/* #pragma section HEAD puts the two steps in PHEAD, which leads the module.
 * there they sit at the addresses 1ST_READ.BIN's stage_steps and sky_steps
 * name */
#pragma section HEAD
void eggmobile2_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case EGGMOBILE2_START:
        work->mode = EGGMOBILE2_LOAD;
        time_stop();
        break;
    case EGGMOBILE2_LOAD:
        stream_reset();
        stream_request_music(MUSIC_EGGMBL23);
        light_read_disc_sources(STAGE_STATION_SQUARE, STATION_SQUARE_STATION);
        light_read_disc_palettes(STAGE_STATION_SQUARE, STATION_SQUARE_STATION);
        time_stop();
        eggmobile2_fight_start(task);
        break;
    case EGGMOBILE2_FIGHT:
        eggmobile2_keep_player_in();
        break;
    case EGGMOBILE2_IDLE:
        break;
    }
}
/* switching back to the default section, P, ends eggmobile2_step on a
 * literal pool of its own. in one run of PHEAD, the two steps would share
 * one */
#pragma section

static void eggmobile2_fight_start(struct task *task)
{
    struct task_work *work = task->work;

    stage_get_stage_act();
    egg_walker_create();
    boss_name_create(eggmobile2_boss_name, -1, 240, 120);
    work->mode = EGGMOBILE2_FIGHT;
}

static void eggmobile2_keep_player_in(void)
{
    if (player_work[0]->pos.z < BOUND_Z_MIN) {
        player_work[0]->pos.z = BOUND_Z_MIN;
    } else if (player_work[0]->pos.z > BOUND_Z_MAX) {
        player_work[0]->pos.z = BOUND_Z_MAX;
    }
    if (player_work[0]->pos.x < BOUND_X_MIN) {
        player_work[0]->pos.x = BOUND_X_MIN;
    } else if (player_work[0]->pos.x > BOUND_X_MAX) {
        player_work[0]->pos.x = BOUND_X_MAX;
    }
    if (player_work[0]->pos.y < BOUND_Y_MIN) {
        player_work[0]->pos.y = BOUND_Y_MIN;
    } else if (player_work[0]->pos.y > BOUND_Y_MAX) {
        player_work[0]->pos.y = BOUND_Y_MAX;
    }
}

#pragma section HEAD
void eggmobile2_sky_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SKY_START:
        njSetBackColor(BACK_COLOR, BACK_COLOR, BACK_COLOR);
        work->mode = SKY_RUN;
    case SKY_RUN:
        njFogDisable();
        njPushMatrix(NULL);
        njPopMatrix(1);
        njFogEnable();
        break;
    case SKY_RESET:
        work->mode = SKY_START;
        break;
    default:
        /* @bug memory: a mode past SKY_RESET hands the task itself to
         * heap_free (JMP at 0x0c9000ea). a task is a slot of the pool
         * task_init takes from the heap, not a block heap_alloc gave */
        heap_free(task);
        break;
    }
}
#pragma section
