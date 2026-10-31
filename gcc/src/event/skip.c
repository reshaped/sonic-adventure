/* the port's skip. Start, while an event holds control and no demo plays,
 * lowers a curtain over the scene, which plays on. once the curtain hides
 * it, the rest of the script runs with each wait one step long: a runner
 * stands at its target, and a mover takes the steps the wait stood for.
 * nothing renders or sounds, and no frame waits for a vsync. the event ends
 * as SEGA's code ends it, and the curtain lifts as control comes back. make
 * gcc links this into 1ST_READ.BIN after the image's sources */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "collision/collision.h"
#include "event/event.h"
#include "event/play.h"
#include "player/player.h"
#include "progress/demo.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/timer.h"
#include "system/util.h"

/* how near its target a run ends, across x and z, as player.c ends it */
#define RUN_REACH 1.0F

Bool event_skipping;
int skip_owed;
/* TRUE from the press until control comes back */
static Bool skip_pressed;
/* Start's bit on the first pad at the last frame's end */
static Uint32 start_held;

/* control has come back: the curtain lifts. after a collapse each slot
 * takes back its volume, and play takes back its pacing where the collapse
 * began after event_leave set it */
static void skip_end(void)
{
    Bool skipped = event_skipping;

    /* sound.c sends each slot's volume as the least while event_skipping is
     * TRUE, so it clears before the slots take theirs back */
    event_skipping = skip_pressed = FALSE;
    if (skipped) {
        if (frame_interval == 0) {
            frame_pacing_init(2, 2);
        }
        sound_unmute();
    }
    port_curtain(FALSE);
}

void skip_frame(void)
{
    /* per_info's press lasts one read of the pad, which can come and go
     * before frame_end looks. Start held now and not at the last frame's end
     * is a press too */
    Uint32 start = per_info[0]->on & PDD_DGT_ST;
    Uint32 pressed = (per_info[0]->press | (start & ~start_held)) & PDD_DGT_ST;

    start_held = start;
    skip_owed = 0;
    if (!event_in_control()) {
        if (skip_pressed) {
            skip_end();
        }
    } else if (event_skipping) {
        /* event_start_step clears draw_off at its 16th step */
        draw_off = TRUE;
    } else if (skip_pressed && port_curtain_shown()) {
        event_skipping = TRUE;
        draw_off = TRUE;
        frame_set_interval(0);
        stream_stop_music();
        stream_stop_voice();
        /* resends each slot's volume, which sound.c sends as the least while
         * event_skipping is TRUE */
        sound_unmute();
    } else if (skip_pressed || (pressed && !demo_playing)) {
        /* the curtain falls only on drawn frames. under draw_off, at the
         * press or from PLAY's end, it closes at once, and this frame
         * renders it whole */
        skip_pressed = TRUE;
        port_curtain(TRUE);
        draw_off = FALSE;
    }
}

int skip_wait(int left)
{
    struct task *task;
    struct task_work *work;
    NJS_POINT3 *to;
    Float x;
    Float z;
    int i;

    for (i = 0; i < PLAYER_COUNT; i++) {
        if ((task = player_get_task(i)) == NULL) {
            continue;
        }
        work = task->work;
        if (work->event->action != EVENT_ACTION_RUN) {
            continue;
        }
        to = &work->event->move.pos;
        x = to->x - work->pos.x;
        z = to->z - work->pos.z;
        if (x * x + z * z <= RUN_REACH * RUN_REACH) {
            continue;
        }
        /* a player faces the way it ran, and aims that way, as player.c
         * aims its run. an aim left from before would turn it off its run's
         * heading on its next step */
        if (work->collision != NULL &&
            work->collision->kind == COLLISION_PLAYER) {
            work->ang.y = (Angle)(atan2f(z, x) * ANGLE_PER_RADIAN) & 0xffff;
            task->motion->aim.y = work->ang.y;
        }
        event_set_pos(task, to->x, to->y, to->z);
    }
    skip_owed = left - 1;
    return 1;
}
