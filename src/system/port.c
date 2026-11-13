/* the game's side of the port's features: its toggles and their chord, the
 * skip's curtain and port_view_x. port_view_x reads port_view_aspect, which
 * pace.c's frame_end sets. a machine draws the features, as
 * engine/dc/overlay.c and wide.c do for the Dreamcast. make dc links this
 * into 1ST_READ.BIN after the image's last code and data.
 * config/1st_read/kernel.txt's first cold row places it there */
#ifdef NONMATCHING
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "system/chord.h"
#include "system/pad.h"
#include "system/timer.h"

#define CHORD (PDD_DGT_TL | PDD_DGT_TR)

int port_toggles[TOGGLES] = {FALSE, FALSE, FALSE};

/* whether the curtain falls, and the drawn frames it has faded in */
static int curtain_on;
static int curtain_level;
/* whether a frame has rendered the curtain faded all the way in */
static int curtain_shown;
/* the chord's latch, and the pad's buttons as port_chord last read them */
static int chord_latched;
static Uint32 chord_old;

Float port_view_x(Float x)
{
    /* cx + (x - cx) can round away from x */
    if (port_view_aspect == 1.0F) {
        return x;
    }
    return _nj_screen_.cx + ((x - _nj_screen_.cx) * port_view_aspect);
}

void port_curtain(int on)
{
    curtain_on = on;
    /* the overlay draws nothing under draw_off, so the curtain can't fall
     * there frame by frame */
    if (on && draw_off == TRUE) {
        curtain_level = CURTAIN_FADE;
    }
}

int port_curtain_shown(void)
{
    return curtain_shown;
}

int port_curtain_level(void)
{
    return curtain_level;
}

void port_curtain_advance(void)
{
    if (draw_off == TRUE) {
        return;
    }
    curtain_shown = curtain_level == CURTAIN_FADE;
    if (curtain_on && curtain_level < CURTAIN_FADE) {
        curtain_level++;
    } else if (!curtain_on && curtain_level > 0) {
        curtain_level--;
    }
}

void port_chord(PDS_PERIPHERAL *pad)
{
    static const Uint32 buttons[TOGGLES] = {PDD_DGT_KU, PDD_DGT_KR, PDD_DGT_KD};
    Uint32 fresh;
    int i;

    fresh = pad->on & ~chord_old;
    chord_old = pad->on;
    if ((pad->on & CHORD) == CHORD && (fresh & PAD_DIRECTIONS)) {
        chord_latched = TRUE;
        for (i = 0; i < TOGGLES; i++) {
            if (!(fresh & buttons[i])) {
                continue;
            }
            /* the port's timing readout steps through its machine's pages
             * rather than flip. its toggle is on while a page shows */
            port_toggles[i] =
                i == TOGGLE_READOUT ? overlay_page() : !port_toggles[i];
            overlay_note(i);
        }
    } else if (!(pad->on & (CHORD | PAD_DIRECTIONS))) {
        chord_latched = FALSE;
    }
    if (chord_latched) {
        pad->on &= ~(CHORD | PAD_DIRECTIONS);
        pad->press &= ~(CHORD | PAD_DIRECTIONS);
        pad->l = pad->r = 0;
    }
}
#endif
