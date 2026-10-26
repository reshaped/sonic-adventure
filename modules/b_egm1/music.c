/* the fight's music, asked for a few steps into the stage */
#include <sg_xpt.h>

#include "b_egm1/music.h"
#include "sound/stream.h"
#include "system/task.h"

/* the wait asks for the music on its third step. that is the first with
 * more than MUSIC_DELAY steps before it */
#define MUSIC_DELAY 1

/* task->extra points to a Sint32, the steps the wait has run before this
 * one */
static void eggmobile1_music_wait_step(struct task *task)
{
    Sint32 *waited = task->extra;

    if ((*waited)++ > MUSIC_DELAY) {
        stream_request_music(MUSIC_BOSSALL);
        task_kill(task);
    }
}

void eggmobile1_music_start(void)
{
    struct task *task;
    Sint32 *waited;

    stream_reset();
    if ((task = task_create(0, 4, eggmobile1_music_wait_step)) != NULL) {
        waited = heap_calloc(1, sizeof(Sint32));
        task->extra = waited;
        *waited = 0;
    }
}

void eggmobile1_music_stop(void)
{
}
