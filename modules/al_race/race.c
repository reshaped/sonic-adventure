/* the Chao race's stage task, which creates the game, steps it in the hall
 * and on the course, and changes act when the game asks */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_race/game.h"
#include "al_race/race.h"
#include "chao/chao.h"
#include "player/player.h"
#include "sound/cue.h"
#include "sound/stream.h"
#include "stage/act.h"
#include "stage/stage.h"
#include "system/task.h"

/* the stage task's vars: the game alife_race_start creates */
struct race_vars {
    Uint8 unk_00[8];
    struct race_game *game;
};

#define VARS(work) ((struct race_vars *)&(work)->vars)

/* the stage task's modes. RACE_INIT creates the game, and RACE_RUN steps
 * it. RACE_END changes act, and RACE_WAIT then waits a step before RACE_RUN */
enum race_mode { RACE_INIT, RACE_WAIT, RACE_RUN, RACE_END };

/* in the hall, player 0 falls out of the stage below this height */
#define HALL_FLOOR (-600.0F)

/* music_start_later's wait before the hall's music starts, counted down by
 * each step and each draw */
#define MUSIC_WAIT 10

static void alife_race_start(struct task *task);
static void alife_race_run(struct task *task);
static void alife_race_end(struct task *task);
static void alife_race_destroy(struct task *task);

/* in the HEAD section, at the address 1ST_READ.BIN's stage_steps names for
 * it, 0x0cb80040 */
#pragma section HEAD
void alife_race_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case RACE_INIT:
        alife_race_start(task);
        break;
    case RACE_WAIT:
        /* @hack spelling: task->work read again, which SHC loads anew (MOV.L
         * @(32,R4),R4 at 0x0cb8005e); with the work read once, 15 of 27 words
         * differ */
        work = task->work;
        work->mode = RACE_RUN;
        break;
    case RACE_RUN:
        alife_race_run(task);
        break;
    case RACE_END:
        alife_race_end(task);
        break;
    }
}
#pragma section

static void alife_race_start(struct task *task)
{
    struct task_work *work = task->work;

    chao_gardens_init();
    VARS(work)->game = race_game_create();
    /* @hack redundant: an empty test of the game just created, which SHC
     * writes as MOV R0,R3 and TST R3,R3 (0x0cb998b4, 0x0cb998b6); without it,
     * 15 of 25 words differ */
    if (VARS(work)->game == NULL) {
    }
    work->mode = RACE_RUN;
    task->destructor = alife_race_destroy;
    music_start_later(MUSIC_CHAOHALL, MUSIC_WAIT);
}

static void alife_race_run(struct task *task)
{
    struct task_work *work = task->work;
    Uint32 stage_act;
    struct task_work *player;

    if (race_game_step(VARS(work)->game)) {
        work->mode = RACE_END;
    }
    stage_act = stage_get_stage_act();
    if ((Uint8)stage_act == ACT_HALL) {
        player = player_work[0];
        if (player != NULL && player->pos.y < HALL_FLOOR) {
            music_end();
            player_fall(0);
        }
    }
}

static void alife_race_end(struct task *task)
{
    struct task_work *work = task->work;
    Uint32 stage_act;

    work->mode = RACE_WAIT;
    stage_act = stage_get_stage_act();
    switch ((Uint8)stage_act) {
    case ACT_HALL:
        act_change(1, MUSIC_CHAORACE);
        break;
    case ACT_COURSE:
        act_change(-1, -1);
        break;
    }
}

static void alife_race_destroy(struct task *task)
{
    struct task_work *work = task->work;

    race_game_free(VARS(work)->game);
}
