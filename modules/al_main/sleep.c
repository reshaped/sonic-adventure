/* a Chao's sleep */
#include <ninja.h>
#include <stdlib.h>

#include "al_main/behavior.h"
#include "al_main/chao.h"
#include "al_main/face.h"
#include "al_main/sleep.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

/* the motions. a Chao whose curio and agres are both 0 or below lies down
 * shy */
#define MOTION_LIE_DOWN 78
#define MOTION_LIE_DOWN_SHY 7
#define MOTION_GET_UP 81

#define SE_SNORE 0x359
#define SE_GET_UP 0x36c

/* the Chao snores as it lies down. each later snore comes once the work's
 * timer passes the Chao's limit. it restarts the timer, and sets the limit
 * to SNORE_WAIT steps and a random count below SNORE_SPREAD more. the first
 * of them waits for the limit the last behavior left */
#define SNORE_WAIT 100
#define SNORE_SPREAD 40.0F

/* a point of sleep runs out each time the work's tick passes SLEEP_TICKS,
 * which restarts it from 0. alife_sleep and the Chao's task step each count
 * the tick up, so each point after the first lasts 51 steps */
#define SLEEP_TICKS 100

/* the work's submode */
enum sleep_mode { SLEEP_LIE_DOWN, SLEEP_SLEEP, SLEEP_GET_UP };

void alife_sleep(struct task_work *work, struct alife_chao *chao)
{
    struct chao_motion *motion = &chao->motion;

    switch (work->submode) {
    case SLEEP_LIE_DOWN:
        work->submode++;
        if (chao->record->curio > 0 || chao->record->agres > 0) {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = MOTION_LIE_DOWN;
        } else {
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = MOTION_LIE_DOWN_SHY;
        }
        sound_play_at(SE_SNORE, 0, 0, -20, work->pos.x, work->pos.y,
                      work->pos.z);
        /* fall through */
    case SLEEP_SLEEP:
        if (chao->record->sleep == 0) {
            work->submode++;
            chao->motion.flags |= MOTION_NEW;
            chao->motion.id = MOTION_GET_UP;
            sound_play_at(SE_GET_UP, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
        } else if (work->tick++ > SLEEP_TICKS) {
            work->tick = 0;
            chao->record->sleep--;
        }
        if (TASK_TIMER(work) > chao->limit) {
            sound_play_at(SE_SNORE, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
            work->timer = 0;
            chao->limit =
                (int)(rand() * (SNORE_SPREAD * RAND_SCALE)) + SNORE_WAIT;
        }
        break;
    case SLEEP_GET_UP:
        if (motion->flags & MOTION_DONE) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            chao->last_mode = work->mode;
            work->mode = MODE_IDLE;
            work->submode = 0;
            work->timer = 0;
            chao->mode_timer = 0;
        }
        break;
    }
    alife_face_set_eyes(chao, 4, 2);
    chao->accel.x = -chao->speed.x * 0.2F;
    chao->accel.z = -chao->speed.z * 0.2F;
}
