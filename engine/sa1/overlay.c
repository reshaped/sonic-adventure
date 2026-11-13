/* the PC's overlay, in engine/dc/overlay.c's place. it draws the curtain and
 * the note that names a toggle as the chord flips it. it draws the readout
 * too, the port's on-screen timing display, as one page of the host's figures.
 * the toggles, the chord and the curtain's count are src/system/port.c's,
 * which every machine shares. so a skip collapses an event at the step the
 * Dreamcast's does. the bands draw by Ninja's 2D polygon, and the text in
 * Ninja's built-in font. a split frame shows two pictures for a frame of play.
 * the overlay goes over the first, where present.c draws it. it goes over each
 * frame's last picture as frame_wait begins. that one shows the overlay as the
 * last frame's end left it */
#include <ninja.h>
#include <string.h>
#include <time.h>

#include "native.h"
#include "sa1.h"
#include "stage/stage.h"
#include "system/chord.h"
#include "system/frame.h"
#include "system/port.h"
#include "system/timer.h"
#include "system/util.h"

/* the overlay prints in cells of 16 pixels, 40 across and 30 down, as the
 * Dreamcast's does. the title-safe area starts a tenth in from each edge. a
 * line ends at RIGHT_END, a cell inside its right edge, so a line of
 * LINE_CHARS characters fits. the readout starts at TOP_ROW, a cell inside its
 * top. the note shows two rows under the readout's last, whether the readout
 * shows or not */
#define CELL 16
#define COLUMNS 40
#define ROWS 30
#define RIGHT_END 35
#define LINE_CHARS (RIGHT_END - COLUMNS / 10)
#define TOP_ROW 4
#define READOUT_LINES 3
#define NOTE_ROW (TOP_ROW + READOUT_LINES + 1)
/* the vsyncs the note fades in and out over, and shows for in all */
#define NOTE_FADE 15
#define NOTE_VSYNCS 120
/* the alpha of the dark band under the overlay's text, out of 255 */
#define BAND_ALPHA 0xa0
/* a vsync, at 59.94 Hz, and a half second, in ns */
#define VSYNC_NS 16683350ULL
#define HALF_SECOND_NS 500000000ULL

/* Ninja's built-in font, which the slice links from the disc's data */
extern Uint8 _nj_debug_char_[];

struct sa1_figures sa1_figures;

/* the note's text, its vsyncs left, and the vsyncs it has faded in */
static struct {
    char text[16];
    int left;
    int level;
} note;
/* the readout's half second. start is in ns on the host's clock, and flips
 * is _nj_vsync_count_ then. busy and device sum the game thread's and the
 * device's times. split counts the frames that showed both pictures. reason
 * is the letter of the last held frame that has one */
static struct {
    uint64_t start;
    Uint32 flips;
    int frames;
    uint64_t busy;
    uint64_t device;
    uint64_t peak;
    int split;
    int reason;
} half;
static char lines[READOUT_LINES][LINE_CHARS + 1];
static Uint32 counter_color = 0x60e080;
/* where the last frame ended on the host's clock, and the time play had
 * waited by then */
static uint64_t frame_end_ns;
static uint64_t frame_waited_ns;
/* the stage and act the late and lost vsyncs count from, and play.c's
 * counts as that act began */
static Sint16 act_stage = -1;
static Sint16 act_act = -1;
static uint64_t act_late;
static uint64_t act_lost;

static uint64_t now_ns(void)
{
    struct timespec time;

    clock_gettime(CLOCK_MONOTONIC, &time);
    return (uint64_t)time.tv_sec * 1000000000ULL + (uint64_t)time.tv_nsec;
}

static uint64_t at_most(uint64_t value, uint64_t most)
{
    return value < most ? value : most;
}

/* draw a see-through band from (x0, y0) to (x1, y1) in color, as the
 * Dreamcast's overlay does. 16:9 doesn't squeeze it */
static void overlay_band(Float x0, Float y0, Float x1, Float y1, Uint32 color)
{
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    port_wide_cover = TRUE;
    port_band(x0, y0, x1, y1, color, -1.0F, NJD_FILL | NJD_TRANSPARENT);
    port_wide_cover = FALSE;
}

/* text at col and row in the ARGB color, as njPrintC draws it */
static void overlay_print(int col, int row, const char *text, Uint32 color)
{
    sa1_record_text(text, (float)(col * CELL), (float)(row * CELL), (float)CELL,
                    color, _nj_debug_char_);
}

/* text at col and row in the rgb color, on a dark band. alpha, out of 255,
 * fades the text and its band alike */
static void overlay_text(int col, int row, const char *text, Uint32 color,
                         int alpha)
{
    overlay_band(col * CELL - 8, row * CELL - 4,
                 (col + (int)strlen(text)) * CELL + 8, (row + 1) * CELL + 4,
                 (Uint32)(alpha * BAND_ALPHA / 255) << 24);
    overlay_print(col, row, text, (Uint32)alpha << 24 | color);
}

static const char *readout_mode(int asked, int applied)
{
    return applied ? "ON" : asked ? "HELD" : "OFF";
}

/* fold the frame into the half second. busy is its busy time in ns, and shown
 * is present_shown's answer. once the half second has run, write the lines and
 * start the next. the first gives pictures a second, then the game thread's
 * and the device's mean times in ms. it turns amber where the busy peak passes
 * 90 percent of the frame's budget, and red past it. the second gives the
 * modes. 60 fps is ON only where every frame showed both pictures. where it is
 * HELD, the last held frame's letter follows. the third gives the vsyncs
 * reached late and lost since the stage or act changed */
static void readout_measure(uint64_t now, uint64_t busy,
                            enum present_shown shown)
{
    static const Uint32 colors[3] = {0x60e080, 0xffc040, 0xff5050};
    static const char reasons[SHOWNS] = {
        [HELD_LATE] = 'T',   [HELD_STOPPED] = 'T', [HELD_FULL] = 'C',
        [HELD_FIRST] = 'T',  [HELD_PVR] = 'R',     [HELD_SECOND] = 'T',
        [HELD_BITMAP] = 'B', [HELD_SPARE] = 'E'};
    uint64_t span = now - half.start;
    uint64_t budget = frame_interval * VSYNC_NS;
    char reason[3] = {0};
    Uint32 flips;
    int split;
    int fps;
    int mean;
    int device;

    half.frames++;
    if (shown == SHOWN_BOTH) {
        half.split++;
    } else if (reasons[shown] != 0) {
        half.reason = reasons[shown];
    }
    half.busy += busy;
    half.device += sa1_figures.device_ns;
    if (busy > half.peak) {
        half.peak = busy;
    }
    if (span < HALF_SECOND_NS) {
        return;
    }
    flips = _nj_vsync_count_ - half.flips;
    fps = (int)at_most((flips * 1000000000ULL + span / 2) / span, 999);
    mean = (int)at_most(half.busy / half.frames / 100000, 999);
    device = (int)at_most(half.device / half.frames / 100000, 999);
    string_format(lines[0], "%2d FPS %2d.%d GPU %2d.%d", fps, mean / 10,
                  mean % 10, device / 10, device % 10);
    counter_color =
        colors[(half.peak * 10 > budget * 9) + (half.peak > budget)];
    split = half.split == half.frames;
    if (port_toggles[TOGGLE_FAST] && !split && half.reason != 0) {
        reason[0] = ' ';
        reason[1] = half.reason;
    }
    string_format(
        lines[1], "16:9 %s  60 FPS %s%s",
        readout_mode(port_toggles[TOGGLE_WIDE], wide_applied == WIDE_ON),
        readout_mode(port_toggles[TOGGLE_FAST], split), reason);
    string_format(lines[2], "LATE %u LOST %u",
                  (Uint32)at_most(sa1_figures.late - act_late, 99999),
                  (Uint32)at_most(sa1_figures.lost - act_lost, 99999));
    memset(&half, 0, sizeof(half));
    half.start = now;
    half.flips = _nj_vsync_count_;
}

/* draw the last half second's lines at the top right, on one dark band */
static void readout_draw(void)
{
    int width = 0;
    int length;
    int i;

    for (i = 0; i < READOUT_LINES; i++) {
        length = strlen(lines[i]);
        width = length > width ? length : width;
    }
    overlay_band((RIGHT_END - width) * CELL - 8, TOP_ROW * CELL - 4,
                 RIGHT_END * CELL + 8, (TOP_ROW + READOUT_LINES) * CELL + 4,
                 (Uint32)BAND_ALPHA << 24);
    for (i = 0; i < READOUT_LINES; i++) {
        overlay_print(RIGHT_END - (int)strlen(lines[i]), TOP_ROW + i, lines[i],
                      0xff000000 | (i == 0 ? counter_color : 0xffffff));
    }
}

/* the host's clock times the frame, so wait and busy go unread */
void overlay_end(Uint32 wait, Uint32 busy, int late)
{
    int vsyncs = frame_interval + late;
    uint64_t now = now_ns();
    uint64_t waited = sa1_figures.waited_ns - frame_waited_ns;
    uint64_t spent = now - frame_end_ns;
    int most;

    (void)wait;
    (void)busy;
    if (stage_current != act_stage || act_current != act_act) {
        act_late = sa1_figures.late;
        act_lost = sa1_figures.lost;
        act_stage = stage_current;
        act_act = act_current;
    }
    /* the first frame starts the half second. a skip's frames, at
     * frame_interval 0, wait for no vsync, so the readout leaves them out */
    if (frame_end_ns == 0) {
        half.start = now;
        half.flips = _nj_vsync_count_;
    } else if (frame_interval != 0) {
        readout_measure(now, spent > waited ? spent - waited : 0,
                        present_shown(late));
    }
    frame_end_ns = now;
    frame_waited_ns = sa1_figures.waited_ns;
    note.left = note.left > vsyncs ? note.left - vsyncs : 0;
    most = note.left > NOTE_FADE ? NOTE_FADE : note.left;
    note.level += vsyncs;
    if (note.level > most) {
        note.level = most;
    }
}

void overlay_draw(void)
{
    int curtain;

    if (draw_off == TRUE || sa1_frame_shown) {
        return;
    }
    curtain = port_curtain_level();
    if (curtain > 0) {
        overlay_band(0.0F, 0.0F, COLUMNS * CELL, ROWS * CELL,
                     (Uint32)(curtain * 255 / CURTAIN_FADE) << 24);
    }
    if (port_toggles[TOGGLE_READOUT]) {
        readout_draw();
    }
    if (note.level > 0) {
        overlay_text((COLUMNS - (int)strlen(note.text)) / 2, NOTE_ROW,
                     note.text, 0xffffff, note.level * 255 / NOTE_FADE);
    }
}

/* the readout is one page, which the chord's up shows or hides */
int overlay_page(void)
{
    return !port_toggles[TOGGLE_READOUT];
}

void overlay_note(int toggle)
{
    static const char *const names[TOGGLES] = {"READOUT", "16:9", "60 FPS"};

    string_format(note.text, "%s %s", names[toggle],
                  port_toggles[toggle] ? "ON" : "OFF");
    note.left = NOTE_VSYNCS;
}

/* the Dreamcast gives port_chord the first pad after each serve of the
 * pads. the PC gives it at the game's read, through sa1_chord */
void overlay_pad(void)
{
}

void overlay_mark(enum overlay_mark mark)
{
    (void)mark;
}

void sa1_chord(void *record)
{
    port_chord(record);
}

void sa1_wide_on(void)
{
    port_toggles[TOGGLE_WIDE] = TRUE;
}

void sa1_fast_on(void)
{
    port_toggles[TOGGLE_FAST] = TRUE;
}
