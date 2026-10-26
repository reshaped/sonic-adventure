/* the Egg Walker beaten. small blasts break out about it at random. once
 * player 0 is down on the street, the stage is clear */
#include <ninja.h>
#include <stdlib.h>

#include "b_egm2/defeat.h"
#include "b_egm2/egg_walker.h"
#include "effect/bomb.h"
#include "hud/hud.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* the boss's submodes in EGG_WALKER_DEFEAT */
enum defeat_mode {
    DEFEAT_START,
    DEFEAT_WAIT, /* for player 0 to come down to the street, at y 0 or below */
    DEFEAT_PAUSE, /* one step */
    DEFEAT_CLEAR,
    DEFEAT_DONE
};

/* each step, a blast breaks out with a chance of BLAST_CHANCE. it lies in
 * a cube BLAST_REACH on a side about the boss, at bomb_create's scale
 * BLAST_SCALE */
#define BLAST_CHANCE 0.1F
#define BLAST_REACH 50.0F
#define BLAST_SCALE 0.5F

static void defeat_blast(struct task_work *work);

void egg_walker_defeat(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->submode) {
    case DEFEAT_START:
        work->submode = DEFEAT_WAIT;
    case DEFEAT_WAIT:
        if (player_work[0]->pos.y <= 0.0F) {
            pad_disable();
            time_stop();
            work->submode = DEFEAT_PAUSE;
        }
        break;
    case DEFEAT_PAUSE:
        work->submode = DEFEAT_CLEAR;
        break;
    case DEFEAT_CLEAR:
        stage_clear();
        work->submode = DEFEAT_DONE;
        break;
    case DEFEAT_DONE:
        break;
    }
    defeat_blast(work);
}

static void defeat_blast(struct task_work *work)
{
    NJS_POINT3 pos;

    if (rand() * RAND_SCALE <= BLAST_CHANCE) {
        pos = work->pos;
        pos.x += RANDOM(-BLAST_REACH / 2, BLAST_REACH / 2);
        pos.y += RANDOM(-BLAST_REACH / 2, BLAST_REACH / 2);
        pos.z += RANDOM(-BLAST_REACH / 2, BLAST_REACH / 2);
        sound_play(SE_WALKER_BLAST, 0, 0, 0);
        bomb_create(&pos, BLAST_SCALE);
    }
}
