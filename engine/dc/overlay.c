/* the port's overlay as the Dreamcast draws it over each picture. the
 * readout is the port's on-screen timing display. the note names a toggle as
 * port_chord sets it. the curtain is the black port_curtain fades in.
 * src/system/port.c keeps the toggles and the curtain's count. make dc links
 * this into 1ST_READ.BIN among kernel.txt's cold code */
#include <kamui.h>
#include <kmistat.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

#include "engine/dc/dreamcast.h"
#include "main.h"
#include "stage/stage.h"
#include "system/chord.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/timer.h"
#include "system/util.h"

/* the overlay prints in cells of the 16 pixels njPrintSize keeps, 40 across.
 * the title-safe area is the middle 80 percent, 4 columns in from each side.
 * a line ends at RIGHT_END, a cell inside it. a line of LINE_CHARS
 * characters starts at its left edge. TOP_ROW lies a cell inside it too */
#define CELL 16
#define COLUMNS 40
#define RIGHT_END 35
#define LINE_CHARS (RIGHT_END - COLUMNS / 10)
#define TOP_ROW 4
#define UNDER_ROW (TOP_ROW + 2)
#define PAGE_TIMES 4
#define PERF_LINES (PAGE_TIMES + 4)
/* the vsyncs the note fades in and out over, and shows for in all */
#define NOTE_FADE 15
#define NOTE_VSYNCS 120
/* njInitPrint's color, which the game's prints take */
#define PRINT_COLOR 0xffbfbfbf
/* the alpha of the dark band under the overlay's text, out of 255 */
#define BAND_ALPHA 0xa0
/* the pixels the band reaches past its text, across and down */
#define BAND_PAD_X 8
#define BAND_PAD_Y 4

/* the note's text, its vsyncs left, and the vsyncs it has faded in */
static struct {
    char text[16];
    int left;
    int level;
} note;
/* the readout's page, which the chord's up steps through. port_chord gives
 * the readout's toggle FALSE at READOUT_OFF, so the readout draws nothing.
 * no page folds a frame then, and the SH-4's counters are left alone.
 * counter_measure and hitch_measure still take each frame */
enum { READOUT_OFF, READOUT_PERF, READOUT_CACHE, READOUTS };
static int readout;
/* the frame's times, in syTmrGetCount's counts. busy is as overlay_end takes
 * it. gpu is the last finished render's, as owner.c's handler records it */
static struct {
    Uint32 busy;
    Uint32 gpu;
} measure, counter_sum;
/* the counter, the readout's top line, sums the frames of a half second.
 * counter_peak is their busy peak. counter_flips is _nj_vsync_count_ at the
 * half second's start. frame_wait, njWaitVSync and present.c's show_first add
 * 1 to it as each flips to a picture. a split frame is a frame of play that
 * shows two pictures. counter_split counts the frames that showed both */
static Uint32 counter_start;
static int counter_frames;
static Uint32 counter_peak;
static Uint32 counter_flips;
static int counter_split;
static int counter_reason;
static char counter_line[LINE_CHARS + 1];
static Uint32 counter_color;
static char modes_line[LINE_CHARS + 1];
/* the pages' half second, which a frame folds into while a page shows. its
 * times are step 0's, step 1's, the second picture's pass's and the TA's last
 * transfer's. it counts the frames that wanted the split, and those that
 * showed both pictures. it counts those held by the HELD_ that says why.
 * wanted stops at 99, which keeps the perf page's lines to LINE_CHARS
 * characters. page_step1 is when the wait before step 1 ended, and
 * page_timed is TRUE where this frame read it */
static struct {
    Uint32 sum[PAGE_TIMES];
    Uint32 peak[PAGE_TIMES];
    int frames[PAGE_TIMES];
    int wanted;
    int both;
    int held[SHOWNS];
} page;
static char perf_lines[PERF_LINES][LINE_CHARS + 1];
static Uint32 page_step1;
static int page_timed;
/* the store that holds the split off at the frame's end, or STORES where
 * none does */
static int page_out = STORES;
/* the SH-4's two performance counters: each one's control word, PMCR, and
 * the low word of its count, PMCTR. a count of CPU cycles wraps in 21 s.
 * PMCR_RUN with an event's number counts that event. the clock bit, 0x100,
 * stays clear, so an event that counts time counts CPU cycles of 5 ns.
 * PMCR_CLEAR stops a counter and clears its count. nothing else in the game
 * writes them */
#define PMCR(n) (*(volatile Uint16 *)(0xff000084 + (n) * 4))
#define PMCTR(n) (*(volatile Uint32 *)(0xff100008 + (n) * 8))
#define PMCR_RUN 0xc000
#define PMCR_CLEAR 0x2000
/* the events the cache page counts. a freeze is the cycles the pipeline
 * stands still for a cache's miss. a fill is the cycles a cache takes
 * filling lines */
#define EVENT_OC_FREEZE 0x25
#define EVENT_IC_FREEZE 0x24
#define EVENT_OC_MISSES 0x0f
#define EVENT_OC_FILLS 0x22
#define EVENT_IC_MISSES 0x08
#define EVENT_IC_FILLS 0x21
#define EVENT_ISSUED 0x13
#define EVENT_PARALLEL 0x14
/* the CPU's cycles in a hundredth of a millisecond, at 200 MHz */
#define CYCLES_HUNDREDTH 2000
/* the frame's zones the counters time, the perf page's first three times.
 * from holds the counts at a zone's start, and count each counter's change
 * over it. open is TRUE from the zone's start to the frame's end. done is
 * TRUE where the zone's end came while it was open. so the cache page folds
 * no zone it saw only part of */
enum { ZONE_STEP0, ZONE_STEP1, ZONE_PASS, ZONES };
static struct {
    Uint32 from[2];
    Uint32 count[2];
    int open;
    int done;
} zones[ZONES];
/* counting is TRUE while the cache page runs the counters. found holds the
 * control words they had before. frame_ended is TRUE from overlay_end
 * to frame_wait's call of overlay_pad, where step 0 starts */
static int counting;
static Uint16 found[2];
static int frame_ended;
/* the cache page's pairs of events, which take turns by half second. the
 * page shows a line a zone for the freezes, then a line for each other pair.
 * each line stays until its pair comes round again */
enum { PAIR_FREEZES, PAIR_OC, PAIR_IC, PAIR_ISSUED, PAIRS };
#define CACHE_LINES (ZONES + PAIRS - 1)
_Static_assert(CACHE_LINES <= PERF_LINES,
               "the cache page has more lines than PERF_LINES");
/* each zone's sums of the counters' changes, over the frames it was done */
static struct {
    Uint32 sum[ZONES][2];
    int frames[ZONES];
} counts;
static int cache_pair;
static char cache_lines[CACHE_LINES][LINE_CHARS + 1];
/* the hitch counts since the stage or act last changed, of the frames that
 * wanted the split. switches counts the changes between both pictures and
 * one. lone counts the frames that showed one between two that showed both.
 * both holds whether each of the last two showed both, newest first.
 * fullest is the store of the split's records that came fullest in any
 * frame. full is the percent of its room it took */
static struct {
    int switches;
    int lone;
    int dropped;
    int stopped;
    int late;
    int both[2];
    int fullest;
    Uint32 full;
} hitch;
static Sint16 hitch_stage = -1;
static Sint16 hitch_act = -1;
/* the counter's render peak, which the perf page's hitch line shows */
static Uint32 counter_gpu_peak;
/* Kamui's count of object list overflows as the stage or act last changed.
 * the TA overflows where its object lists reach their bank's ol_limit. the
 * perf page shows those since, up to 99 */
static Uint32 act_start_overflows;

/* a see-through band from x0, y0 to x1, y1 in color, blended as
 * fade_draw_cover blends its quad. at pri -1 it takes depth 1, the most a 2D
 * draw takes. drawn after the fades, it lies over them. port_wide_cover
 * keeps it from 16:9's squeeze, as njPrintC's glyphs take none */
static void overlay_band(Float x0, Float y0, Float x1, Float y1, Uint32 color)
{
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    port_wide_cover = TRUE;
    port_band(x0, y0, x1, y1, color, -1.0F, NJD_FILL | NJD_TRANSPARENT);
    port_wide_cover = FALSE;
}

/* text at col and row in the rgb color, on a dark band. alpha, out of 255,
 * fades both */
static void overlay_text(int col, int row, const char *text, Uint32 color,
                         int alpha)
{
    overlay_band(col * CELL - BAND_PAD_X, row * CELL - BAND_PAD_Y,
                 (col + (int)strlen(text)) * CELL + BAND_PAD_X,
                 (row + 1) * CELL + BAND_PAD_Y,
                 (Uint32)(alpha * BAND_ALPHA / 255) << 24);
    njPrintColor((Uint32)alpha << 24 | color);
    njPrintC(NJM_LOCATION(col, row), text);
}

/* value, or most where value passes it. string_format checks no length. a
 * figure that could outgrow its place in a line passes through this */
static Uint32 at_most(Uint32 value, Uint32 most)
{
    return value < most ? value : most;
}

/* a sum's mean over the counter's frames in tenths of a millisecond, up to
 * 99.9 */
static int counter_mean(Uint32 sum)
{
    return at_most(syTmrCountToMicro(sum / counter_frames) / 100, 999);
}

/* a time in syTmrGetCount's counts as hundredths of a millisecond, to the
 * nearest, up to 99.99 */
static Uint32 time_hundredths(Uint32 counts)
{
    return at_most((syTmrCountToMicro(counts) + 5) / 10, 9999);
}

/* ON where play applies a mode, HELD where only its toggle asks it, and
 * else OFF */
static const char *readout_mode(int asked, int applied)
{
    return applied ? "ON" : asked ? "HELD" : "OFF";
}

/* write the modes' line, the readout's second, for the half second ending.
 * 16:9 shows as it stands. 60 fps shows ON where each frame showed both
 * pictures. a held 60 fps adds the letter of the last held frame that has
 * one. the line holds for the next half second. a split frame's first
 * picture draws before the split, so a live line would show HELD there */
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

/* fold the frame into the hitch counts. shown is present_shown's, and frame
 * the frame's figures, or NULL where no stage loop ran. a frame that showed
 * both pictures late counts as late too */
static void hitch_measure(enum present_shown shown, int late,
                          const struct present_figures *frame)
{
    int both = shown == SHOWN_BOTH;
    int i;

    if (stage_current != hitch_stage || act_current != hitch_act) {
        memset(&hitch, 0, sizeof(hitch));
        act_start_overflows = kmiIntStatisticData.ol_overflows;
        hitch_stage = stage_current;
        hitch_act = act_current;
    }
    for (i = 0; frame != NULL && i < STORES; i++) {
        if (frame->full[i] > hitch.full) {
            hitch.fullest = i;
            hitch.full = frame->full[i];
        }
    }
    if (shown == SHOWN_UNWANTED) {
        return;
    }
    hitch.switches += both != hitch.both[0];
    hitch.lone += both && !hitch.both[0] && hitch.both[1];
    hitch.dropped += shown == HELD_DROPPED;
    hitch.stopped += shown == HELD_STOPPED;
    hitch.late += late != 0;
    hitch.both[1] = hitch.both[0];
    hitch.both[0] = both;
}

/* write time i's line on the perf page: its mean and peak over the half
 * second in milliseconds */
static void perf_time(char *line, const char *label, int i)
{
    Uint32 mean;
    Uint32 peak;

    if (page.frames[i] == 0) {
        string_format(line, "%-6s %6s %6s", label, "-", "-");
        return;
    }
    mean = time_hundredths(page.sum[i] / page.frames[i]);
    peak = time_hundredths(page.peak[i]);
    string_format(line, "%-6s %3u.%02u %3u.%02u", label, mean / 100, mean % 100,
                  peak / 100, peak % 100);
}

static void counters_start(Uint16 a, Uint16 b)
{
    if (!counting) {
        found[0] = PMCR(0);
        found[1] = PMCR(1);
        counting = TRUE;
    }
    memset(zones, 0, sizeof(zones));
    PMCR(0) = PMCR_CLEAR;
    PMCR(1) = PMCR_CLEAR;
    PMCR(0) = PMCR_RUN | a;
    PMCR(1) = PMCR_RUN | b;
}

static void counters_end(void)
{
    if (counting) {
        PMCR(0) = PMCR_CLEAR;
        PMCR(1) = PMCR_CLEAR;
        PMCR(0) = found[0];
        PMCR(1) = found[1];
        counting = FALSE;
    }
}

static void pair_start(void)
{
    static const Uint16 events[PAIRS][2] = {{EVENT_OC_FREEZE, EVENT_IC_FREEZE},
                                            {EVENT_OC_MISSES, EVENT_OC_FILLS},
                                            {EVENT_IC_MISSES, EVENT_IC_FILLS},
                                            {EVENT_ISSUED, EVENT_PARALLEL}};

    counters_start(events[cache_pair][0], events[cache_pair][1]);
}

static void zone_start(int zone)
{
    zones[zone].from[0] = PMCTR(0);
    zones[zone].from[1] = PMCTR(1);
    zones[zone].open = TRUE;
}

static void zone_end(int zone)
{
    if (zones[zone].open) {
        zones[zone].count[0] = PMCTR(0) - zones[zone].from[0];
        zones[zone].count[1] = PMCTR(1) - zones[zone].from[1];
        zones[zone].done = TRUE;
    }
}

void overlay_mark(enum overlay_mark mark)
{
    if (!counting) {
        return;
    }
    switch (mark) {
    case MARK_STEP0_END:
        zone_end(ZONE_STEP0);
        break;
    case MARK_LATER_END:
        zone_end(ZONE_STEP1);
        break;
    case MARK_PASS_START:
        zone_start(ZONE_PASS);
        break;
    default:
        zone_end(ZONE_PASS);
        break;
    }
}

/* a count's mean in hundredths of a millisecond at 5 ns a count, up to
 * 99.99 */
static Uint32 mean_hundredths(Uint32 sum, int frames)
{
    return at_most((sum / frames + CYCLES_HUNDREDTH / 2) / CYCLES_HUNDREDTH,
                   9999);
}

/* write zone's line on the cache page: the means of its time and its two
 * freezes, in milliseconds */
static void freeze_line(char *line, const char *label, int zone)
{
    Uint32 mean;
    Uint32 oc;
    Uint32 ic;

    if (page.frames[zone] == 0 || counts.frames[zone] == 0) {
        string_format(line, "%-6s %5s", label, "-");
        return;
    }
    mean = time_hundredths(page.sum[zone] / page.frames[zone]);
    oc = mean_hundredths(counts.sum[zone][0], counts.frames[zone]);
    ic = mean_hundredths(counts.sum[zone][1], counts.frames[zone]);
    string_format(line, "%-6s %2u.%02u OC %2u.%02u IC %2u.%02u", label,
                  mean / 100, mean % 100, oc / 100, oc % 100, ic / 100,
                  ic % 100);
}

/* write the cache page's line for a cache's misses over step 0. it shows
 * the mean misses and fill cycles a frame, then a miss's fill cycles to a
 * tenth */
static void miss_line(char *line, const char *label)
{
    int frames = counts.frames[ZONE_STEP0];
    Uint32 misses;
    Uint32 fills;
    Uint32 tenths;

    if (frames == 0) {
        string_format(line, "%s -", label);
        return;
    }
    misses = counts.sum[ZONE_STEP0][0] / frames;
    fills = counts.sum[ZONE_STEP0][1] / frames;
    if (misses == 0) {
        string_format(line, "%s %u %u -", label, misses,
                      at_most(fills, 9999999));
        return;
    }
    tenths = at_most(fills / misses * 10 + fills % misses * 10 / misses, 9999);
    string_format(line, "%s %u %u %u.%u", label, at_most(misses, 999999),
                  at_most(fills, 9999999), tenths / 10, tenths % 10);
}

/* write the cache page's line of step 0's instructions: the mean issued a
 * frame and issued in parallel. step 0's time, less the instructions'
 * issue cycles and the two freezes, leaves what no pair counts. the rest
 * holds freezes on registers, the FPU and branches, and a store queue's
 * waits on the TA */
static void issue_line(char *line)
{
    int frames = counts.frames[ZONE_STEP0];
    Uint32 issued;
    Uint32 parallel;

    if (frames == 0) {
        string_format(line, "IS -");
        return;
    }
    issued = counts.sum[ZONE_STEP0][0] / frames;
    parallel = counts.sum[ZONE_STEP0][1] / frames;
    string_format(line, "IS %u %u", at_most(issued, 9999999),
                  at_most(parallel, 9999999));
}

static void cache_write(int pair)
{
    static const char *const labels[ZONES] = {"STEP 0", "STEP 1", "PASS"};
    int i;

    if (pair == PAIR_FREEZES) {
        for (i = 0; i < ZONES; i++) {
            freeze_line(cache_lines[i], labels[i], i);
        }
    } else if (pair == PAIR_ISSUED) {
        issue_line(cache_lines[CACHE_LINES - 1]);
    } else {
        miss_line(cache_lines[ZONES + pair - 1], pair == PAIR_OC ? "OC" : "IC");
    }
}

/* write the perf page's lines: each time's line, then four more. the first
 * shows how many of the frames that wanted the split showed both pictures.
 * it then counts the HELD_FIRST, HELD_SECOND and HELD_PVR frames. the second
 * counts the HELD_DROPPED, HELD_STOPPED and HELD_LATE frames, then the act's
 * object list overflows. the third shows the hitch counts and the counter's
 * render peak. the last names the store that holds the split off, and the
 * act's fullest */
static void perf_write(void)
{
    static const char *const labels[PAGE_TIMES] = {"STEP 0", "STEP 1", "PASS",
                                                   "TA"};
    static const char *const stores[STORES] = {"DRAWS", "SPRITES", "2D",
                                               "MARKS", "SHAPES",  "SHADOWS"};
    char fullest[16] = "-";
    Uint32 gpu = at_most(syTmrCountToMicro(counter_gpu_peak) / 100, 999);
    Uint32 overflows = kmiIntStatisticData.ol_overflows - act_start_overflows;
    int i;

    for (i = 0; i < PAGE_TIMES; i++) {
        perf_time(perf_lines[i], labels[i], i);
    }
    string_format(perf_lines[i], "BOTH %d OF %d  T0 %d T1 %d R %d", page.both,
                  page.wanted, page.held[HELD_FIRST], page.held[HELD_SECOND],
                  page.held[HELD_PVR]);
    string_format(perf_lines[i + 1], "DROP %d STOP %d LATE %d  OL %u",
                  page.held[HELD_DROPPED], page.held[HELD_STOPPED],
                  page.held[HELD_LATE], at_most(overflows, 99));
    string_format(perf_lines[i + 2], "SW%u LONE%u D%u S%u L%u PK%u.%u",
                  at_most(hitch.switches, 999), at_most(hitch.lone, 99),
                  at_most(hitch.dropped, 99), at_most(hitch.stopped, 99),
                  at_most(hitch.late, 99), gpu / 10, gpu % 10);
    if (hitch.full != 0) {
        string_format(fullest, "%s %u", stores[hitch.fullest], hitch.full);
    }
    string_format(perf_lines[i + 3], "OUT %s  PEAK %s",
                  page_out < STORES ? stores[page_out] : "-", fullest);
}

/* write the page's lines for the half second ending, and start the next */
static void readout_write(void)
{
    if (readout == READOUT_PERF) {
        perf_write();
    } else {
        cache_write(cache_pair);
        cache_pair = (cache_pair + 1) % PAIRS;
        pair_start();
    }
    memset(&page, 0, sizeof(page));
    memset(&counts, 0, sizeof(counts));
}

/* clear the page the readout stepped to, and write its lines from no
 * frames. leaving the cache page stops the counters */
static void readout_start(void)
{
    int i;

    memset(&page, 0, sizeof(page));
    memset(&counts, 0, sizeof(counts));
    page_timed = FALSE;
    if (readout == READOUT_PERF) {
        perf_write();
    }
    if (readout == READOUT_CACHE) {
        for (i = 0; i < PAIRS; i++) {
            cache_write(i);
        }
        cache_pair = 0;
        pair_start();
    } else {
        counters_end();
    }
}

/* fold the frame into the pages' half second. shown is present_shown's.
 * step 1's time runs from the end of the wait before it to the last step's
 * end. so it holds the extra step stage.c's loops add where step 1 ends past
 * their limit */
static void page_measure(enum present_shown shown,
                         const struct present_figures *frame)
{
    Uint32 times[PAGE_TIMES];
    int i;

    times[0] = frame->step0;
    times[1] = page_timed ? frame->later_end - page_step1 : 0;
    times[2] = frame->pass;
    times[3] = frame->ta;
    for (i = 0; i < PAGE_TIMES; i++) {
        if (times[i] == 0) {
            continue;
        }
        page.sum[i] += times[i];
        page.frames[i]++;
        if (times[i] > page.peak[i]) {
            page.peak[i] = times[i];
        }
        if (i < ZONES && zones[i].done) {
            counts.sum[i][0] += zones[i].count[0];
            counts.sum[i][1] += zones[i].count[1];
            counts.frames[i]++;
        }
    }
    if (shown != SHOWN_UNWANTED && page.wanted < 99) {
        page.wanted++;
        if (shown == SHOWN_BOTH) {
            page.both++;
        } else {
            page.held[shown]++;
        }
    }
}

/* fold the frame into the counter's half second. shown is present_shown's.
 * once the half second has passed at now, write the counter's line. it shows
 * the pictures presented a second, then the mean busy and render times in
 * milliseconds. dividing the span by the flips can't overflow, however long
 * a movie draws the half second out. the busy peak sets its color: green,
 * amber past 90 percent of the frame's budget, red past it */
static void counter_measure(Uint32 now, enum present_shown shown)
{
    static const Uint32 colors[3] = {0x60e080, 0xffc040, 0xff5050};
    static const char reasons[SHOWNS] = {
        [HELD_LATE] = 'T',   [HELD_STOPPED] = 'T', [HELD_FULL] = 'C',
        [HELD_FIRST] = 'T',  [HELD_PVR] = 'R',     [HELD_SECOND] = 'T',
        [HELD_BITMAP] = 'B', [HELD_SPARE] = 'E'};
    Uint32 span = now - counter_start;
    Uint32 budget = frame_interval * VSYNC_COUNTS;
    Uint32 flips;
    Uint32 between;
    int fps;
    int busy;
    int gpu;

    counter_frames++;
    if (shown == SHOWN_BOTH) {
        counter_split++;
    } else if (reasons[shown] != 0) {
        counter_reason = reasons[shown];
    }
    counter_sum.busy += measure.busy;
    counter_sum.gpu += measure.gpu;
    if (measure.busy > counter_peak) {
        counter_peak = measure.busy;
    }
    if (measure.gpu > counter_gpu_peak) {
        counter_gpu_peak = measure.gpu;
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
    if (readout != READOUT_OFF) {
        readout_write();
    }
    counter_start = now;
    counter_flips = _nj_vsync_count_;
    counter_frames = 0;
    counter_split = counter_reason = 0;
    counter_sum.busy = counter_sum.gpu = 0;
    counter_peak = counter_gpu_peak = 0;
}

/* how many lines the readout's page shows under the modes' line */
static int readout_lines(void)
{
    return readout == READOUT_PERF    ? PERF_LINES
           : readout == READOUT_CACHE ? CACHE_LINES
                                      : 0;
}

/* fill under with the page's lines. it returns how many there are */
static int readout_under(const char **under)
{
    int lines = readout_lines();
    int i;

    for (i = 0; i < lines; i++) {
        under[i] = readout == READOUT_PERF ? perf_lines[i] : cache_lines[i];
    }
    return lines;
}

/* draw the readout at the top right on one dark band */
static void readout_draw(void)
{
    const char *under[PERF_LINES];
    int counter = strlen(counter_line);
    int modes = strlen(modes_line);
    int width = counter > modes ? counter : modes;
    int lines = readout_under(under);
    int length;
    int i;

    for (i = 0; i < lines; i++) {
        length = strlen(under[i]);
        width = length > width ? length : width;
    }
    overlay_band((RIGHT_END - width) * CELL - BAND_PAD_X,
                 TOP_ROW * CELL - BAND_PAD_Y, RIGHT_END * CELL + BAND_PAD_X,
                 (UNDER_ROW + lines) * CELL + BAND_PAD_Y,
                 (Uint32)BAND_ALPHA << 24);
    njPrintColor(0xff000000 | counter_color);
    njPrintC(NJM_LOCATION(RIGHT_END - counter, TOP_ROW), counter_line);
    njPrintColor(0xffffffff);
    njPrintC(NJM_LOCATION(RIGHT_END - modes, TOP_ROW + 1), modes_line);
    for (i = 0; i < lines; i++) {
        njPrintC(NJM_LOCATION(RIGHT_END - (int)strlen(under[i]), UNDER_ROW + i),
                 under[i]);
    }
}

void overlay_end(Uint32 wait, Uint32 busy, int late)
{
    int vsyncs = frame_interval + late;
    enum present_shown shown;
    struct present_figures frame;
    int figured;
    int most;

    /* a skip's frames wait for no vsync, so they add to no count. a half
     * second can hold hundreds of them, each past a budget of 0 */
    if (frame_interval != 0) {
        shown = present_shown(late);
        figured = present_figures(&frame);
        page_out = figured ? frame.out : STORES;
        measure.busy = busy;
        measure.gpu = kmiIntStatisticData.render_time;
        hitch_measure(shown, late, figured ? &frame : NULL);
        if (readout != READOUT_OFF && figured) {
            page_measure(shown, &frame);
        }
        counter_measure(wait, shown);
    }
    page_timed = FALSE;
    if (counting) {
        memset(zones, 0, sizeof(zones));
    }
    frame_ended = TRUE;
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

    if (draw_off == TRUE) {
        return;
    }
    curtain = port_curtain_level();
    if (curtain > 0) {
        overlay_band(0.0F, 0.0F, SCREEN_WIDTH, SCREEN_HEIGHT,
                     (Uint32)(curtain * 255 / CURTAIN_FADE) << 24);
    }
    if (port_toggles[TOGGLE_READOUT]) {
        readout_draw();
    }
    if (note.level > 0) {
        overlay_text((COLUMNS - (int)strlen(note.text)) / 2,
                     UNDER_ROW + readout_lines() + 1, note.text, 0xffffff,
                     note.level * 255 / NOTE_FADE);
    }
    njPrintColor(PRINT_COLOR);
}

int overlay_page(void)
{
    readout = (readout + 1) % READOUTS;
    readout_start();
    return readout != READOUT_OFF;
}

void overlay_note(int toggle)
{
    static const char *const names[TOGGLES] = {"READOUT", "16:9", "60 FPS"};
    static const char *const readouts[READOUTS] = {"OFF", "PERF", "CACHE"};

    string_format(note.text, "%s %s", names[toggle],
                  toggle == TOGGLE_READOUT ? readouts[readout]
                  : port_toggles[toggle]   ? "ON"
                                           : "OFF");
    note.left = NOTE_VSYNCS;
}

void overlay_pad(void)
{
    port_chord(per_info[0]);
    /* frame_wait's wait before step 0 ends here, and frame_wait_step's
     * before each later step. frame_ended tells the first from the rest,
     * since frame_wait finds frame_step where the last frame left it. that
     * is 1 after an event's frame of one step */
    if (frame_ended) {
        if (counting) {
            zone_start(ZONE_STEP0);
        }
    } else if (frame_step == 1) {
        if (readout != READOUT_OFF) {
            page_step1 = syTmrGetCount();
            page_timed = TRUE;
        }
        if (counting) {
            zone_start(ZONE_STEP1);
        }
    }
    frame_ended = FALSE;
}
