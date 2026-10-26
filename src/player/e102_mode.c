/* E-102's modes, set from outside his step: a leap, a fall, a skate */
#include "player/e102_mode.h"
#include "player/e102.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"

void e102_leap(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;
    struct e102 *e102 = record->own.e102.record[0];

    record->speed.y = record->consts.jump_speed * 2.0F;
    record->speed.x -= 1.5625F;
    e102->steps = 0;
    work->flags &= ~PLAYER_ON_TASK;
    record->jump_steps = 0;
    if (e102_in_play) {
        sound_play_at(SE_JUMP, 0, 0, 0, work->pos.x, work->pos.y, work->pos.z);
    }
    work->mode = E102_LEAP_START;
}

void e102_fall(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: record, set from motion and never read, which keeps
     * motion's load (MOV.L @(36,R4),R4 at 0x8c0db66c). without the two, the
     * function runs 2 bytes short and 4 of 5 words differ */
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;

    work->mode = E102_FALL;
}

void e102_skate_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;

    record->speed.x = 0.1F;
    record->speed.y = record->speed.z = 0.0F;
    work->mode = E102_SKATE_START;
}

void e102_skate_brake(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;

    record->accel.x = -2.0F;
    record->accel.y = record->accel.z = 0.0F;
    work->mode = E102_SKATE_BRAKE;
}
