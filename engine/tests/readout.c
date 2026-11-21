/* run the PC readout against a controlled clock and its drawing recorders */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <ninja.h>
#include "native.h"
#include "sa1.h"
#include "stage/stage.h"
#include "system/chord.h"
#include "system/frame.h"
#include "system/port.h"
#include "system/timer.h"

Uint8 _nj_debug_char_[1];
Uint32 _nj_vsync_count_;
Sint16 stage_current, act_current;
int frame_interval = 2, sa1_frame_shown;
Bool draw_off;
enum wide_mode wide_applied;
Int port_wide_cover;
Float port_view_aspect = 1;

static uint64_t now;
static int formats, bands, texts, detailed;
static char first_line[64];
static Uint32 first_color;

int readout_clock(clockid_t clock, struct timespec *time)
{
    assert(clock == CLOCK_MONOTONIC);
    time->tv_sec = now / 1000000000;
    time->tv_nsec = now % 1000000000;
    return 0;
}

enum present_shown present_shown(int late)
{
    detailed++;
    return late ? HELD_SECOND : SHOWN_BOTH;
}

int string_format(char *out, const char *format, ...)
{
    va_list args;
    int count;
    formats++;
    va_start(args, format);
    count = vsprintf(out, format, args);
    va_end(args);
    return count;
}

void sa1_record_band(float left, float top, float right, float bottom,
                     uint32_t color)
{
    assert(right > left && bottom > top && (color >> 24));
    bands++;
}

void sa1_record_text(const char *text, float x, float y, float side,
                     uint32_t color, const void *font)
{
    assert(x >= 0 && y >= 0 && side > 0 && font == _nj_debug_char_);
    if (!texts++) {
        snprintf(first_line, sizeof(first_line), "%s", text);
        first_color = color;
    }
}

static void draw(int count, const char *prefix)
{
    bands = texts = 0;
    overlay_draw();
    assert(bands == !!count && texts == count);
    if (count) {
        assert(!strncmp(first_line, prefix, strlen(prefix)) &&
               (first_color & 0xffffff));
    }
    assert(port_wide_cover == TRUE);
}

static void advance(unsigned ns, int flips, int late)
{
    now += ns;
    _nj_vsync_count_ += flips;
    overlay_end(0, 0, late);
}

static void cycle(void)
{
    port_toggles[TOGGLE_READOUT] = overlay_page();
}

int main(void)
{
    port_wide_cover = TRUE;
    _nj_vsync_count_ = UINT32_MAX - 7;
    advance(500000000, 0, 0);
    draw(0, "");
    assert(formats == 0 && detailed == 0);
    cycle();
    draw(1, "-- FPS");
    advance(499999999, 14, 0);
    draw(1, "-- FPS");
    advance(1, 1, 1);
    draw(1, "30 FPS");
    advance(500000000, 30, 0);
    draw(1, "60 FPS");
    advance(500000000, 22, 1);
    draw(1, "44 FPS");
    advance(500000000, 0, 0);
    draw(1, " 0 FPS");
    assert(formats == 4 && detailed == 0);
    frame_interval = 0;
    advance(500000000, 0, 0);
    assert(formats == 4);
    frame_interval = 2;
    draw_off = TRUE;
    draw(0, "");
    draw_off = FALSE;
    sa1_frame_shown = 1;
    draw(0, "");
    sa1_frame_shown = 0;
    cycle();
    draw(3, " 0 FPS");
    advance(500000000, 15, 1);
    draw(3, "30 FPS");
    assert(detailed == 1 && strstr(first_line, "GPU"));
    cycle();
    draw(0, "");
    cycle();
    draw(1, "-- FPS");
    return 0;
}
