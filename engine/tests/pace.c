/* the game's frame end, src/system/pace.c, on a host with an engine of the
 * test's own. frame_end asks the engine a mode once a frame. it sets
 * port_split_wanted TRUE only where the engine takes the mode and applies 60.
 * the frames to come may then split, each into two pictures. engine-test runs
 * this. it returns 0, or prints the line of the first check that fails and
 * returns 1 */
#include <stdio.h>

#include <ninja.h>
#include <sg_xpt.h>

#include "progress/demo.h"
#include "progress/recorder.h"
#include "sa_dc.h"
#include "stage/stage.h"
#include "system/frame.h"

/* what frame_end reads of the game: a stage in play at two steps and two
 * vsyncs a frame. the 60 fps toggle is on, so each frame asks 60 */
Uint32 _nj_wait_vsync_out_;
Bool demo_playing;
Bool recording_on;
int frame_interval = 2;
int frame_step_count = 2;
int port_toggles[TOGGLES] = {[TOGGLE_FAST] = TRUE};
Sint16 stage_current = STAGE_BEACH;
Bool stage_in_play(void)
{
    return TRUE;
}

/* what frame_end calls of the port. only present_end does anything here: it
 * keeps the vsyncs late it is given */
static int ended_late;
void overlay_end(Uint32 wait, Uint32 busy, int late)
{
    (void)wait, (void)busy, (void)late;
}
void skip_frame(void)
{
}
void port_curtain_advance(void)
{
}
void wide_end(void)
{
}
void wide_apply(void)
{
}
Uint32 present_idle(void)
{
    return 0;
}
void present_end(int late)
{
    ended_late = late;
}

/* the engine applies the mode asked, holds its rate at 30, or refuses it */
static enum { APPLIES, HOLDS, REFUSES } engine_does;

sa_result sa_present_window(sa_engine *engine, uint32_t aspect)
{
    (void)engine, (void)aspect;
    return SA_OK;
}

/* the backends write applied only where they return SA_OK. this one writes
 * the mode asked even where it refuses. so a frame_end that read applied past
 * a refusal would find 60 and fail */
sa_result sa_present_mode(sa_engine *engine, const sa_mode *asked,
                          sa_mode *applied)
{
    (void)engine;
    *applied = *asked;
    if (engine_does == REFUSES) {
        return SA_ERR_UNSUPPORTED;
    }
    if (engine_does == HOLDS) {
        applied->rate = 30;
    }
    return SA_OK;
}

#define CHECK(test)                                                            \
    do {                                                                       \
        if (!(test)) {                                                         \
            return __LINE__;                                                   \
        }                                                                      \
    } while (0)

/* each frame_end leaves port_split_wanted changed from the one before. each
 * passes present_end its vsyncs late, whatever the engine did */
static int check(void)
{
    engine_does = APPLIES;
    frame_end(0, 1);
    CHECK(port_split_wanted == TRUE && ended_late == 1);
    engine_does = HOLDS;
    frame_end(0, 2);
    CHECK(port_split_wanted == FALSE && ended_late == 2);
    engine_does = APPLIES;
    frame_end(0, 3);
    CHECK(port_split_wanted == TRUE && ended_late == 3);
    engine_does = REFUSES;
    frame_end(0, 4);
    CHECK(port_split_wanted == FALSE && ended_late == 4);
    return 0;
}

int main(void)
{
    int line = check();

    if (line != 0) {
        fprintf(stderr, "pace: line %d\n", line);
    }
    return line != 0;
}
