/* the engine sound: SND_ENG, an engine's hum, kept playing where the set
 * file puts it */
#include "stg06/engine_sound.h"
#include "sound/sound.h"
#include "system/task.h"

/* the hum and its volume. each step leaves the hum ENGINE_UPDATES + 1 updates
 * to play. the hum stops soon after the last step */
#define SE_ENGINE 964
#define ENGINE_VOLUME 48
#define ENGINE_UPDATES 2

void engine_sound_step(struct task *task)
{
    struct task_work *work = task->work;

    sound_play_at_for(SE_ENGINE, (int)task, 1, ENGINE_VOLUME, work->pos.x,
                      work->pos.y, work->pos.z, ENGINE_UPDATES);
}
