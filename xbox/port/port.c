/* the port layer's core: its log, and the report of a call the port layer
 * doesn't serve yet. tools/pclink.py's stubs call port_unimplemented for
 * each name nothing defines, once each */
#include <sg_xpt.h>

#include "xbox/port/host.h"
#include "xbox/port/port.h"

void port_log(const char *text)
{
    HOST->log(text);
}

struct port_counts port_counts;
unsigned port_ninja_vertices; /* njModel.c's packets: the renderer moves them to port_counts */
volatile int port_profile;
volatile int port_overlay;
struct port_shown port_shown;
/* bit 4: the kept models the host lights (xbox/engine/sa1/model.c's
 * keep_lit); bit 5: the meshsets the walk projects and clips itself, and
 * bit 6 SA1's screen polygons, 2D polygons and lines, as runs of points
 * already on the screen rather than the lists' vertices
 * (xbox/engine/sa1/emit.h) */
volatile int port_gpu_models = 127;
/* src/collision/polygon.c's cap on a far sweep's steps: 0, none, as the
 * console has none (the sweep is logged); 1 widens them */
volatile int port_sweep_cap = 0;

/* a model src/draw's drawer draws: whether its points are as they were when
 * it was last drawn, by their words' sum, for the models the game changes in
 * place, which a drawer that kept their vertices would have to send again */
#define MODELS 4096
static struct {
    const void *model;
    unsigned sum;
} seen_models[MODELS];

void port_count_model(const void *model, const void *points, int count)
{
    const unsigned *words = points;
    unsigned sum = 0x9e3779b1u;
    unsigned slot = ((unsigned)model >> 4) % MODELS;
    int k;

    port_counts.drawer_models++;
    port_counts.drawer_points += (unsigned)count;
    port_watch_model(model);
    if (!(port_profile & 2)) {
        return;
    }
    for (k = 0; k < count * 3; k++) {
        sum = (sum ^ words[k]) * 0x01000193u;
    }
    for (k = 0; k < 8; k++, slot = (slot + 1) % MODELS) {
        if (seen_models[slot].model == model) {
            if (seen_models[slot].sum != sum) {
                port_counts.changed_models++;
                port_counts.changed_points += (unsigned)count;
                seen_models[slot].sum = sum;
            }
            return;
        }
        if (seen_models[slot].model == NULL) {
            break;
        }
    }
    if (k == 8) {
        slot = ((unsigned)model >> 4) % MODELS;
    }
    seen_models[slot].model = model;
    seen_models[slot].sum = sum;
}

/* the port's figures on the screen, in Ninja's debug text, which goes to
 * the translucent list as it's printed: so before the lists are rendered.
 * a figure in tenths shows as its whole part, a point and the tenth */
void njPrintC(int loc, const char *s);
void njPrintD(int loc, int val, int digit);
void njPrintColor(unsigned color);
#define LOCATION(x, y) ((x) << 16 | (y))

static void overlay_tenths(int row, const char *label, unsigned tenths)
{
    njPrintC(LOCATION(27, row), label);
    njPrintD(LOCATION(32, row), (int)(tenths / 10 > 999 ? 999 : tenths / 10), 3);
    njPrintC(LOCATION(35, row), ".");
    njPrintD(LOCATION(36, row), (int)(tenths % 10), 1);
}

static void overlay_count(int row, const char *label, unsigned count)
{
    njPrintC(LOCATION(27, row), label);
    njPrintD(LOCATION(32, row), (int)(count > 99999 ? 99999 : count), 5);
}

/* Ninja's count of rendered frames: HOST_CLOCK=frames' and the shots' */
extern Uint32 _nj_vsync_count_;

void port_overlay_draw(void)
{
    /* the counts it shows are the profile's */
    if (!port_profile) {
        port_profile = 1;
    }
    /* in yellow, then Ninja's own grey again, njInitPrint's */
    njPrintColor(0xffffff00u);
    overlay_tenths(3, "FPS", port_shown.fps10);
    overlay_tenths(4, "MS", port_shown.frame10);
    overlay_tenths(13, "WRST", port_shown.worst10);
    overlay_tenths(5, "DRAW", port_shown.drawer10);
    overlay_tenths(6, "REND", port_shown.render10);
    overlay_tenths(7, "GPUW", port_shown.gpu_wait10);
    overlay_tenths(8, "VBLW", port_shown.blank_wait10);
    overlay_tenths(9, "SND", port_shown.sound10);
    overlay_count(10, "VTX", port_shown.vertices);
    overlay_count(11, "GPUV", port_shown.gpu_vertices);
    overlay_count(12, "MODL", port_shown.models);
    /* the frame being made, to name a moment in a capture of the screen */
    njPrintC(LOCATION(27, 14), "FRM");
    njPrintD(LOCATION(31, 14), (int)(_nj_vsync_count_ % 1000000u), 6);
    njPrintColor(0xffbfbfbfu);
}

/* the names reported so far, by address: each stub's name is a string of
 * its own, so its address names it */
static const char *reported[512];
static int reported_count;

void port_unimplemented(const char *name)
{
    int k;
    char line[96];
    const char *from = name;
    char *to = line;
    const char *lead = "unimplemented: ";

    for (k = 0; k < reported_count; k++) {
        if (reported[k] == name) {
            return;
        }
    }
    if (reported_count < 512) {
        reported[reported_count++] = name;
    }
    while (*lead != '\0') {
        *to++ = *lead++;
    }
    while (*from != '\0' && to < line + sizeof line - 1) {
        *to++ = *from++;
    }
    *to = '\0';
    port_log(line);
}

/* engine/dc/dreamcast.h's stop at a failure the game can't go on from, which
 * make dc's crash report (engine/dc/crash.c, which the port leaves out) shows
 * on the screen: the port logs what, name and number, and ends the run */
static char *put_text(char *to, char *end, const char *text)
{
    while (text != NULL && *text != '\0' && to < end) {
        *to++ = *text++;
    }
    return to;
}

void port_stop(const char *what, const char *name, Uint32 number)
{
    static const char hex[] = "0123456789abcdef";
    char line[160];
    char *to = line;
    char *end = line + sizeof line - 12;
    int k;

    to = put_text(to, end, "stop: ");
    to = put_text(to, end, what);
    if (name != NULL) {
        to = put_text(to, end, " ");
        to = put_text(to, end, name);
    }
    *to++ = ' ';
    *to++ = '0';
    *to++ = 'x';
    for (k = 28; k >= 0; k -= 4) {
        *to++ = hex[(number >> k) & 0xf];
    }
    *to = '\0';
    port_log(line);
    HOST->exit(1);
    for (;;) {
    }
}

/* the crash report's frame mark, which the readout makes each frame
 * (engine/dc/overlay.c). the port's hang is the host's to see */
void crash_frame(void)
{
}
