/* E-102's last seconds: his time in a big digit, pulsing */
#include <ninja.h>
#include <sg_xpt.h>

#include "hud/hud.h"
#include "main.h"
#include "player/e102_bonus.h"
#include "player/e102_countdown.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"

#define SE_LAST_SECONDS 1195

/* the time, in seconds, at which the countdown opens, and the scale of its
 * digit and its sign */
#define LAST_SECONDS 5
#define DIGIT_SCALE 2.2F

enum countdown_mode {
    COUNTDOWN_INIT,
    COUNTDOWN_WAIT,
    COUNTDOWN_OPEN,
    COUNTDOWN_SHOW,
    COUNTDOWN_CLOSE
};

struct countdown_vars {
    Float open; /* how wide, from 0 to 1 */
};

#define COUNTDOWN(work) ((struct countdown_vars *)&(work)->vars)

/* the time left, as one digit under the sign, pulsing twice a second. the time
 * in seconds is the digit's texanim index in e102time_sprite. so as the
 * countdown closes, a time above 9 draws a texanim past the digits */
static void countdown_draw(struct task *task)
{
    struct task_work *work = task->work;
    Uint8 minutes;
    Uint8 seconds;
    Uint8 frames;
    int time;
    Float size;
    Angle ang;

    if (frame_step != 0) {
        return;
    }
    time_get(&minutes, &seconds, &frames);
    time = (minutes * 60) + seconds;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    e102time_sprite.p.x = SCREEN_MIDDLE_X;
    e102time_sprite.p.y = 415.2F;
    ang = frames * 0x8000 / 30;
    size = (0.9F + (0.4F * _builtin_fabsf(njSin(ang)))) * DIGIT_SCALE;
    e102time_sprite.sx = size * COUNTDOWN(work)->open;
    e102time_sprite.sy = size;
    njDrawSprite2D(&e102time_sprite, time, -2.0F, NJD_SPRITE_ALPHA);
    e102time_sprite.p.x = SCREEN_MIDDLE_X;
    e102time_sprite.p.y = 380.0F;
    e102time_sprite.sx = COUNTDOWN(work)->open * DIGIT_SCALE;
    e102time_sprite.sy = DIGIT_SCALE;
    njDrawSprite2D(&e102time_sprite, E102TIME_LASTSEC, -3.0F, NJD_SPRITE_ALPHA);
}

/* at LAST_SECONDS or less, the countdown sounds a warning and opens over 8
 * steps. once the time is above LAST_SECONDS again, it closes */
void e102_countdown_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint8 minutes;
    Uint8 seconds;
    Uint8 frames;
    int time;

    time_get(&minutes, &seconds, &frames);
    time = (minutes * 60) + seconds;
    switch (work->mode) {
    case COUNTDOWN_INIT:
        COUNTDOWN(work)->open = 0.0F;
        work->mode = COUNTDOWN_WAIT;
        /* fall through */
    case COUNTDOWN_WAIT:
        if (time <= LAST_SECONDS) {
            work->mode = COUNTDOWN_OPEN;
            sound_play(SE_LAST_SECONDS, 0, 0, 0);
        }
        return;
        /* @hack redundant: a break after the return, whose deleted jump has
         * the next two cases store their modes from R2 and R1 (0x8c0df632,
         * 0x8c0df63e). without it, R3 and R2 take them and 4 of 92 words
         * differ */
        break;
    case COUNTDOWN_OPEN:
        COUNTDOWN(work)->open += 0.125F;
        if (COUNTDOWN(work)->open >= 1.0F) {
            COUNTDOWN(work)->open = 1.0F;
            work->mode = COUNTDOWN_SHOW;
        }
        break;
    case COUNTDOWN_SHOW:
        if (time > LAST_SECONDS) {
            work->mode = COUNTDOWN_CLOSE;
        }
        break;
    case COUNTDOWN_CLOSE:
        COUNTDOWN(work)->open -= 0.125F;
        if (COUNTDOWN(work)->open <= 0.0F) {
            COUNTDOWN(work)->open = 0.0F;
            work->mode = COUNTDOWN_WAIT;
            return;
        }
        break;
    }
    countdown_draw(task);
}
