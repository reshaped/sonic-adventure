/* the port's end of each frame. it defines the window, port_view_aspect,
 * and port_split_wanted, which frame_end sets from the mode it asks. make
 * dc links this into 1ST_READ.BIN after the image's sources */
#ifdef NONMATCHING
#include <ninja.h>
#include <sg_xpt.h>

#include "progress/demo.h"
#include "progress/recorder.h"
#include "sa_dc.h"
#include "stage/stage.h"
#include "system/frame.h"

Float port_view_aspect = 1.0F;
Int port_split_wanted;

/* while an attract demo or a recording runs, the window stays 4:3, which
 * pillarboxes 16:9. a replay stays in step only with its recording's calls
 * to rand(). some steps call it only for what is in view, which 16:9's
 * window widens. 60 fps splits a frame into two pictures and adds no step.
 * so it splits a replay's frames as it does play's. the toggle asks 60 only
 * where the game's frames may split. the engine may hold 60 at 30, as where
 * 16:9 changes. port_split_wanted is TRUE only where the engine applies 60.
 * an engine that refuses the mode applies none. present_end draws the
 * overlay between wide_end and wide_apply. so the port's timing readout
 * names the 16:9 the frame drew with */
void frame_end(Uint32 wait, int late)
{
    sa_mode asked = {SA_ASPECT_4_3, 30, 0, 0};
    sa_mode applied;
    Uint32 window;

    if (port_toggles[TOGGLE_WIDE]) {
        asked.aspect = SA_ASPECT_16_9;
    }
    if (port_toggles[TOGGLE_FAST] && stage_in_play() &&
        (stage_current < STAGE_CART || stage_current == STAGE_SUNABOARD) &&
        frame_interval == 2 && frame_step_count == 2) {
        asked.rate = 60;
    }
    window = demo_playing || recording_on ? SA_ASPECT_4_3 : asked.aspect;
    sa_present_window(SA_DC_ENGINE, window);
    port_split_wanted =
        sa_present_mode(SA_DC_ENGINE, &asked, &applied) == SA_OK &&
        applied.rate == 60;
    /* measure the frame before skip_frame can change frame_interval, which
     * overlay_end reads. a skip's interval is 0, and its frames take only
     * the vsyncs they run late. a split frame's busy time leaves out its
     * wait for the middle vsync */
    overlay_end(wait, wait - _nj_wait_vsync_out_ - present_idle(), late);
    skip_frame();
    port_curtain_advance();
    wide_end();
    present_end(late);
    port_view_aspect = window == SA_ASPECT_16_9 ? WIDE_ASPECT : 1.0F;
    wide_apply();
}
#endif
