/* the Egg Walker's smoke sound, Eggman's voice lines and Tikal's hints */
#include <sg_xpt.h>

#include "b_egm2/hints.h"
#include "event/script.h"
#include "hud/hud.h"
#include "hud/life_gauge.h"
#include "object/tikal.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"

#define TEXT_1_LENGTH 34
#define TEXT_2_LENGTH 46
#include "b_egm2/hints/text.inc"

#define EGGMAN_LINES 5
#define SE_SMOKE 0x1de
#define SMOKE_HOLD 60 /* calls of egg_walker_smoke_step */

enum { HINT_LEGS, HINT_COCKPIT, HINTS };

/* the legs' hint shows once the time passes HINT_TIME, 1:30:00. the
 * cockpit's shows once falls passes HINT_FALLS. the cockpit's hint is
 * needless once the boss's health falls below HINT_HEALTH, its health
 * before any hit. after a hint shows, the next waits HINT_WAIT calls of
 * egg_walker_hint_legs */
#define HINT_TIME 9000
#define HINT_HEALTH 5.0F
#define HINT_FALLS 2
#define HINT_WAIT 300

/* the calls of egg_walker_smoke_step the smoke's sound has left */
static Sint8 smoke_time;
static Bool smoke_playing;

static const Sint32 eggman_voices[EGGMAN_LINES] = {
    0xb0, 0xb1, 0xae, 0xaf, 0xb2,
};

static const struct script_page hint_pages[HINTS][2] = {
    {{TEXT_1, 120}, {NULL, 0}},
    {{TEXT_2, 120}, {NULL, 0}},
};
static const Sint32 hint_voices[HINTS] = {0x1b3, 0x337};

static Uint8 falls = 0;
/* the calls of egg_walker_hint_legs before another hint can show */
static Sint32 hint_delay = 0;
/* each set once its hint has shown, or once a hit has made it needless */
static Bool hint_legs_shown = FALSE;
static Bool hint_cockpit_shown = FALSE;

void egg_walker_smoke_reset(void)
{
    smoke_playing = FALSE;
    smoke_time = 0;
}

void egg_walker_smoke_hold(void)
{
    smoke_time = SMOKE_HOLD;
}

void egg_walker_smoke_step(void)
{
    if (smoke_playing) {
        if (--smoke_time <= 0) {
            sound_stop(SE_SMOKE);
            smoke_playing = FALSE;
        }
    } else if (smoke_time > 0) {
        sound_play_held(SE_SMOKE, 0, 1, 0);
        smoke_playing = TRUE;
    }
}

void egg_walker_voice(Uint8 line)
{
    if (line < EGGMAN_LINES) {
        stream_request_voice(eggman_voices[line]);
    }
}

static void hint_show(Uint8 hint)
{
    if (hint < HINTS) {
        tikal_hint_create((struct script_page *)hint_pages[hint],
                          hint_voices[hint]);
    }
}

void egg_walker_hint_legs(void)
{
    Bool show = FALSE;
    Uint8 minutes;
    Uint8 seconds;
    Uint8 frames;
    int time;

    if (hint_delay > 0) {
        hint_delay--;
        return;
    }
    if (hint_legs_shown) {
        return;
    }
    /* a leg hit makes the hint needless, so no later call shows it. this
     * call still can */
    if (falls > 0) {
        hint_legs_shown = TRUE;
    }
    if (stage_get_retries() > 0) {
        show = TRUE;
    } else {
        time_get(&minutes, &seconds, &frames);
        /* the time as one number: its whole seconds times 100, plus its
         * frames, 0 to 59 */
        time = (minutes * 6000) + (seconds * 100) + frames;
        if ((time < 0 ? -time : time) > HINT_TIME) {
            show = TRUE;
        }
    }
    if (show) {
        hint_show(HINT_LEGS);
        hint_legs_shown = TRUE;
        hint_delay = HINT_WAIT;
    }
}

void egg_walker_hint_cockpit(void)
{
    if (hint_delay > 0) {
        return;
    }
    if (hint_cockpit_shown) {
        return;
    }
    /* a hit on the boss makes the hint needless, so no later call shows it.
     * this call still can */
    if (life_gauge_health < HINT_HEALTH) {
        hint_cockpit_shown = TRUE;
    }
    if (falls > HINT_FALLS) {
        hint_show(HINT_COCKPIT);
        hint_cockpit_shown = TRUE;
        hint_delay = HINT_WAIT;
    }
}

void egg_walker_hint_end(void)
{
    tikal_hint_end();
}

void egg_walker_count_fall(void)
{
    falls++;
}
