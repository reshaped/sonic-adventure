/* the port's overlay over each frame: its switches and the chord that flips
 * them, the frame counter, the note that names a switch as it flips, and the
 * curtain. make gcc links this into 1ST_READ.BIN after the image's sources */
#include <kamui.h>
#include <kmistat.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

#include "main.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/timer.h"
#include "system/util.h"

/* the overlay prints in cells of the 16 pixels njPrintSize keeps, 40 across.
 * a line ends at RIGHT_END, a cell inside the title-safe middle 80 percent,
 * and TOP_ROW lies a cell inside it too. the readout shows on the top row and
 * the one under it, and the note two rows under them */
#define CELL 16
#define COLUMNS 40
#define RIGHT_END 35
#define TOP_ROW 4
#define NOTE_ROW (TOP_ROW + 3)
/* the vsyncs the note fades in and out over, and shows for in all */
#define NOTE_FADE 15
#define NOTE_VSYNCS 120
/* the drawn frames the curtain fades over */
#define CURTAIN_FADE 8
#define CHORD (PDD_DGT_TL | PDD_DGT_TR)
/* njInitPrint's color, which the game's prints take */
#define PRINT_COLOR 0xffbfbfbf

/* the switches as the game boots */
int port_toggles[TOGGLES] = {FALSE, FALSE, FALSE};

/* the note's text, its vsyncs left, and the vsyncs it has faded in */
static struct {
    char text[16];
    int left;
    int level;
} note;
/* whether the curtain falls, and the drawn frames it has faded in */
static int curtain_on;
static int curtain_level;
/* whether a frame has rendered the curtain faded all the way in */
static int curtain_shown;
/* the chord's latch, and the pad's buttons at the last serve */
static int chord_latched;
static Uint32 chord_old;
/* the readout up steps through: off, the counter, then the counter over
 * each of SEGA's debug pages, debug_page 1 to 3 */
#define READOUTS 5
static int readout;
/* the frame measure, in syTmrGetCount's counts: busy, as port.h's overlay_end
 * says, which holds the frame's steps, its draws and all it runs outside
 * frame_wait; and gpu, the time of the last render Kamui finished, as its
 * interrupt records it. the counter sums each over its half second */
static struct {
    Uint32 busy;
    Uint32 gpu;
} measure, counter_sum;
/* the counter's half second: its start, the frames frame_wait ended in it,
 * which the means take, and their busy peak. then _nj_vsync_count_ at its
 * start: frame_wait, njWaitVSync and the split frame's first picture add 1
 * there for each picture they present, as they flip to it. then how many
 * of its frames split, and the reason its last held frame gave. then the
 * line it shows, and its color, and the modes' line under it */
static Uint32 counter_start;
static int counter_frames;
static Uint32 counter_peak;
static Uint32 counter_flips;
static int counter_split;
static int counter_reason;
static char counter_line[32];
static Uint32 counter_color;
static char modes_line[32];

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

/* a see-through band from x0, y0 to x1, y1 in color, as fade_draw_cover
 * draws its quad. at -1 it takes depth 1, as the fades do, and drawn after
 * them it lies over them. it takes no 16:9 squeeze, as njPrintC's glyphs
 * take none */
static void overlay_band(Float x0, Float y0, Float x1, Float y1, Uint32 color)
{
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];
    NJS_POINT2COL band;
    int i;

    points[0].x = points[3].x = x0;
    points[1].x = points[2].x = x1;
    points[0].y = points[1].y = y0;
    points[2].y = points[3].y = y1;
    for (i = 0; i < 4; i++) {
        colors[i].color = color;
    }
    band.p = points;
    band.col = colors;
    band.tex = NULL;
    band.num = 4;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    port_wide_cover = TRUE;
    njDrawPolygon2D(&band, 4, -1.0F, NJD_FILL | NJD_TRANSPARENT);
    port_wide_cover = FALSE;
}

/* text at col and row in the rgb color, on a dark band, both at alpha out
 * of 255 */
static void overlay_text(int col, int row, const char *text, Uint32 color,
                         int alpha)
{
    overlay_band(col * CELL - 8, row * CELL - 4,
                 (col + (int)strlen(text)) * CELL + 8, (row + 1) * CELL + 4,
                 (Uint32)(alpha * 0xa0 / 255) << 24);
    njPrintColor((Uint32)alpha << 24 | color);
    njPrintC(NJM_LOCATION(col, row), text);
}

/* a sum's mean over the counter's frames in tenths of a millisecond, up to
 * 99.9 */
static int counter_mean(Uint32 sum)
{
    int tenths = syTmrCountToMicro(sum / counter_frames) / 100;

    return tenths < 999 ? tenths : 999;
}

/* a mode on the readout: ON where play applies it, HELD where its switch
 * asks it and play holds it back, and else OFF */
static const char *readout_mode(int asked, int applied)
{
    return applied ? "ON" : asked ? "HELD" : "OFF";
}

/* write the modes' line for the half second ending: what each mode's switch
 * asks and play applies, 16:9 as it stands and 60 fps over the half second,
 * ON where every frame split. a held 60 fps gives after it the reason its
 * last held frame gave, R or T, as port.h's present_reason says. the line
 * holds over the half second, as the counter's does, since a split frame's
 * first picture draws before its frame splits and would show HELD */
static void modes_write(void)
{
    char reason[3] = {0};
    int split = counter_split == counter_frames;

    if (port_toggles[TOGGLE_FAST] && !split && counter_reason != 0) {
        reason[0] = ' ';
        reason[1] = counter_reason;
    }
    string_format(
        modes_line, "16:9 %s  60 FPS %s%s",
        readout_mode(port_toggles[TOGGLE_WIDE], wide_applied == WIDE_ON),
        readout_mode(port_toggles[TOGGLE_FAST], split), reason);
}

/* fold the frame, and reason, present_reason's for it, into the counter's
 * half second. as each ends at now, write the line: the pictures presented
 * a second, from the mean time between their flips, which no half second
 * overflows however long a movie draws it out, then in milliseconds the mean
 * busy time and the render's time. 20 characters keep it clear of the hud's
 * rings. it shows in green, in amber once the busy peak passes 90 percent of
 * the frame's budget, and in red past it */
static void counter_measure(Uint32 now, int reason)
{
    static const Uint32 colors[3] = {0x60e080, 0xffc040, 0xff5050};
    Uint32 span = now - counter_start;
    Uint32 budget = frame_interval * VSYNC_COUNTS;
    Uint32 flips;
    Uint32 between;
    int fps;
    int busy;
    int gpu;

    counter_frames++;
    if (frame_split) {
        counter_split++;
    } else if (reason != 0) {
        counter_reason = reason;
    }
    counter_sum.busy += measure.busy;
    counter_sum.gpu += measure.gpu;
    if (measure.busy > counter_peak) {
        counter_peak = measure.busy;
    }
    if (span < COUNTS_PER_SECOND / 2) {
        return;
    }
    flips = _nj_vsync_count_ - counter_flips;
    between = flips != 0 ? span / flips : 0;
    fps = between != 0 ? (COUNTS_PER_SECOND + between / 2) / between : 0;
    busy = counter_mean(counter_sum.busy);
    gpu = counter_mean(counter_sum.gpu);
    string_format(counter_line, "%2d FPS %2d.%d GPU %2d.%d", fps, busy / 10,
                  busy % 10, gpu / 10, gpu % 10);
    counter_color =
        colors[(counter_peak * 10 > budget * 9) + (counter_peak > budget)];
    modes_write();
    counter_start = now;
    counter_flips = _nj_vsync_count_;
    counter_frames = 0;
    counter_split = counter_reason = 0;
    counter_sum.busy = counter_sum.gpu = 0;
    counter_peak = 0;
}

/* the readout at the top right, on one dark band: the counter's line, and
 * under it in white the modes' line, both from the last half second */
static void readout_draw(void)
{
    int counter = strlen(counter_line);
    int modes = strlen(modes_line);
    int width = counter > modes ? counter : modes;

    overlay_band((RIGHT_END - width) * CELL - 8, TOP_ROW * CELL - 4,
                 RIGHT_END * CELL + 8, (TOP_ROW + 2) * CELL + 4, 0xa0U << 24);
    njPrintColor(0xff000000 | counter_color);
    njPrintC(NJM_LOCATION(RIGHT_END - counter, TOP_ROW), counter_line);
    njPrintColor(0xffffffff);
    njPrintC(NJM_LOCATION(RIGHT_END - modes, TOP_ROW + 1), modes_line);
}

void overlay_end(Uint32 wait, Uint32 busy, int vsyncs, int reason)
{
    int most;

    measure.busy = busy;
    measure.gpu = kmiIntStatisticData.render_time;
    counter_measure(wait, reason);
    note.left = note.left > vsyncs ? note.left - vsyncs : 0;
    most = note.left > NOTE_FADE ? NOTE_FADE : note.left;
    note.level += vsyncs;
    if (note.level > most) {
        note.level = most;
    }
}

void overlay_advance(void)
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

void overlay_draw(void)
{
    if (draw_off == TRUE) {
        return;
    }
    if (curtain_level > 0) {
        overlay_band(0.0F, 0.0F, SCREEN_WIDTH, SCREEN_HEIGHT,
                     (Uint32)(curtain_level * 255 / CURTAIN_FADE) << 24);
    }
    if (port_toggles[TOGGLE_COUNTER]) {
        readout_draw();
    }
    if (note.level > 0) {
        overlay_text((COLUMNS - (int)strlen(note.text)) / 2, NOTE_ROW,
                     note.text, 0xffffff, note.level * 255 / NOTE_FADE);
    }
    njPrintColor(PRINT_COLOR);
}

void overlay_pad(void)
{
    static const Uint32 buttons[TOGGLES] = {PDD_DGT_KU, PDD_DGT_KR, PDD_DGT_KD};
    static const char *const names[TOGGLES] = {"COUNTER", "16:9", "60 FPS"};
    PDS_PERIPHERAL *pad = per_info[0];
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
            if (i == TOGGLE_COUNTER) {
                readout = (readout + 1) % READOUTS;
                port_toggles[i] = readout != 0;
                debug_page = readout > 1 ? readout - 1 : 0;
            } else {
                port_toggles[i] = !port_toggles[i];
            }
            if (debug_page && i == TOGGLE_COUNTER) {
                string_format(note.text, "DEBUG PAGE %d", debug_page);
            } else {
                string_format(note.text, "%s %s", names[i],
                              port_toggles[i] ? "ON" : "OFF");
            }
            note.left = NOTE_VSYNCS;
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
