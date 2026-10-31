/* the port's end of each frame: its measure, and the modes play lets the
 * switches apply. make gcc links this into 1ST_READ.BIN after the image's
 * sources */
#include <ninja.h>
#include <sg_xpt.h>

#include "progress/demo.h"
#include "progress/recorder.h"
#include "stage/stage.h"
#include "system/frame.h"

/* while an attract demo or a recording runs, 16:9 pillarboxes, which holds
 * the window at 4:3: a replay stays in step only with its recording's calls
 * to rand(), and some steps call it only for what is in view, which the
 * window widens in play. 60 fps is the split frame, which present.c plans,
 * and changes no step, so it splits a replay's frames as it does play's. it
 * takes a frame only where the stage is in play, or in the fade from black
 * that opens play, and is Sand Hill or one before the mini games. a frame
 * after which 16:9 changes doesn't split the next, whose first picture would
 * replay this frame's 2D, pillarbox's bands among it, under the other mode.
 * the overlay draws after pillarbox's bands, outside pillarbox, and before
 * the next frame's 16:9 applies, so its readout names the 16:9 the frame was
 * drawn with */
void frame_end(Uint32 wait, int late)
{
    int demo = demo_playing || recording_on;
    enum wide_mode wide = !port_toggles[TOGGLE_WIDE] ? WIDE_OFF
                          : demo                     ? WIDE_PILLARBOX
                                                     : WIDE_ON;
    int fast =
        port_toggles[TOGGLE_FAST] && stage_in_play() &&
        (stage_current < STAGE_CART || stage_current == STAGE_SUNABOARD) &&
        wide == wide_applied;

    /* measure the frame before skip_frame can change frame_interval. a
     * skip's interval is 0, and its frames take the vsyncs they run late. a
     * split frame's busy time leaves out its wait for the middle vsync */
    overlay_end(wait, wait - _nj_wait_vsync_out_ - present_idle(),
                frame_interval + late, present_reason(fast, late));
    skip_frame();
    overlay_advance();
    wide_end();
    present_end(fast, late);
    wide_apply(wide);
}
