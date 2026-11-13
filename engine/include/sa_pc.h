/* the PC backend's options, which sa_engine_desc.platform points at, and the
 * calls only a PC answers. ENGINE.md#pc-rendering gives its rules. C17 and
 * C++17 compile it alike, as sa_engine.h. a program links the runtime's
 * core and this backend, and makes a windowed engine so:
 *
 *   sa_pc_options options = {SA_PC_WINDOW, 0, 0, NULL, "Sonic Adventure"};
 *   sa_engine_desc desc = {SA_ENGINE_ABI_VERSION, sizeof desc, SA_GAME_SA1, 0,
 *                          SA_PC_ARENA, 2, SA_PC_RESOURCES, SA_PC_EVENTS,
 *                          &options};
 *
 * options is read during sa_engine_create alone. null gives a headless
 * engine on the first device */
#ifndef SA_PC_H
#define SA_PC_H

#include "sa_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/* this backend reads the bytes a picture's draws borrow until the picture
 * retires. it draws only resources and payloads. under sa_pc_options.rate,
 * the render-owned scene keeps the draws across pictures. it keeps the parts
 * sa_scene_part gives and the scene's lights too. no picture is lit by those
 * lights */
#define SA_BORROW_PICTURE
/* the game's adapter gives this out of line. at the call it copies what the
 * game's drawer would read into resources and payloads */
sa_result sa_present_live(sa_engine *engine, const sa_draw_live *draw);

/* the bounds engine/sa1/play.c gives sa_engine_desc. SA_PC_ARENA holds a
 * picture's palettes, lights and payloads. SA_PC_EVENTS holds an event for
 * each resource's release, which the resource's making promises. it holds
 * 64 more, for the pictures shown, the storage requests ended and a quit
 * that wait for sa_event_poll */
#define SA_PC_ARENA (16U << 20)
#define SA_PC_RESOURCES 16384U
#define SA_PC_EVENTS (SA_PC_RESOURCES + 64U)

enum {
    /* an SDL3 window and a swapchain beside the picture; else headless */
    SA_PC_WINDOW = 0x1,
    /* show at once, not at the display's vertical blank */
    SA_PC_NO_VSYNC = 0x2,
    /* keep each picture for sa_pc_read */
    SA_PC_READBACK = 0x4,
    /* sound on the window's audio device, rendered on its thread as it
     * asks. else sa_pc_audio_render gives it, a stretch at a time */
    SA_PC_SOUND = 0x8,
    /* the window starts fullscreen. F11, or alt and return, takes it in and
     * out of fullscreen whatever this says */
    SA_PC_FULLSCREEN = 0x10,
    /* fullscreen sets the display mode nearest the desktop's, where SDL
     * lists one. else it is a borderless window across the desktop */
    SA_PC_EXCLUSIVE = 0x20
};
/* how the blend list composes: per pixel, as the PowerVR sorts it, which is
 * the contract. the others are a host's orders, to measure against it: the
 * draws' order, each draw's depth, each triangle's */
enum {
    SA_PC_BLEND_PIXEL = 0,
    SA_PC_BLEND_ORDER = 1,
    SA_PC_BLEND_DRAW = 2,
    SA_PC_BLEND_TRIANGLE = 3
};
typedef struct sa_pc_options {
    uint32_t flags; /* SA_PC_* */
    uint32_t blend; /* SA_PC_BLEND_* */
    /* a window's first size, in SA_PC_WIDTH by SA_PC_HEIGHT, 0 for 2 */
    uint32_t scale;
    /* the first device listed whose name holds this. null, or a name none
     * holds, gives the first listed */
    const char *device;
    const char *title; /* a window's */
    /* the display's pictures a second, in thousandths. the backend draws
     * pictures between the game's from the render-owned scene to fill them,
     * and 0 draws none. a rate within 0.2% of a multiple of the game's 59.94
     * steps a second counts as that multiple. it needs
     * sa_engine_desc.pictures of 2 or more */
    uint32_t rate;
    /* the picture drawn at SA_PC_WIDTH by SA_PC_HEIGHT times this. 0 sizes a
     * window's picture to the 4:3 or 16:9 region it shows in, never under 1.
     * 0 draws a headless engine's at 1. a picture larger than where it shows
     * is area-filtered down to it, which supersamples it */
    uint32_t resolution;
    /* a window's pad bindings, dead zone and rumble in engine/pc/input.h's
     * lines of text, or null for the defaults */
    const char *settings;
    /* a window's icon, a PVR's bytes as SA_FORMAT_PVR takes them, or null */
    const void *icon;
    uint32_t icon_bytes;
    /* where the engine can't open its window or device, sa_engine_create
     * writes why here, to SA_PC_WHY bytes. null prints it on stderr */
    char *why;
} sa_pc_options;
#define SA_PC_WHY 256U

/* show a player words in a box on the desktop, and return as it closes. a
 * game opened from a menu has no terminal for them. without a display or
 * SDL3 it shows nothing */
void sa_pc_alert(const char *title, const char *words);

/* the picture's size at resolution 1, the 2D layer's, and what a read gives.
 * at 16:9 it holds a view a third wider, as the Dreamcast's does */
#define SA_PC_WIDTH 640U
#define SA_PC_HEIGHT 480U

/* under a rate, a store of the render-owned scene's that a picture fills:
 * its draws, sprites and triangles; its meshes' materials' textures; its
 * triangles' points; the tasks that end while it is open; and the
 * triangles of a picture between two, as many as a picture's own */
enum {
    SA_PC_FULL_DRAWS = 1,
    SA_PC_FULL_TEXTURES = 2,
    SA_PC_FULL_POINTS = 3,
    SA_PC_FULL_ENDED = 4,
    SA_PC_FULL_BETWEEN = 5
};

/* the most blend fragments a pixel composes: its nearest, and at one depth
 * its latest drawn. a pixel that holds more shows without the rest */
#define SA_PC_PIXEL_FRAGMENTS 64U

/* what the picture last ended took, once it has retired */
typedef struct sa_pc_stats {
    uint32_t draws; /* draws, sprites and triangles records taken */
    uint32_t triangles;
    uint32_t batches;
    /* blend fragments the picture shows without, here and in
     * fragments_cut below. each is one pixel's, and passed the depth test.
     * lost: the picture's lists had no room, so the fragment joined no
     * pixel's. cut: its pixel held more than SA_PC_PIXEL_FRAGMENTS, and
     * composed without it. with both 0, every blend fragment that passed
     * the depth test is in the picture */
    uint32_t fragments_lost;
    uint32_t arena; /* bytes of its arena taken, of sa_engine_desc.arena */
    uint32_t shades; /* of the 4,096 a picture holds */
    uint64_t cpu_ns; /* from sa_present_begin to sa_present_end's return */
    uint64_t gpu_ns; /* the device's own time, 0 where it can't say */
    /* under a rate: the pictures the display showed for it, those between
     * and then itself. the display shows 32 at most for a picture. what the
     * rate owes past them is never shown */
    uint32_t shown;
    /* the two pictures' draws, by how the last picture between took them */
    uint32_t moved; /* drawn at a transform between the two pictures */
    uint32_t snapped; /* drawn at the nearer one's, paired with no draw */
    uint32_t born; /* left out to the half, as the newer drew it first */
    uint32_t stayed; /* drawn to the half at the older's transform alone */
    uint32_t ended; /* left out, as its task ended in the older */
    /* under a rate: the render-owned scene's store that had no room,
     * SA_PC_FULL_*, or 0. the scene keeps none of a picture that fills a
     * store. each picture shown for it is then the picture itself. the
     * picture after it stands alone, as after a cut. the scene's instances
     * stay as the picture before left them. SA_PC_FULL_BETWEEN says the
     * scene kept the picture, but a picture between had no room for a draw.
     * it and each shown after it are the picture itself, so none steps
     * back */
    uint32_t full;
    /* the pictures shown before the picture itself that are it again */
    uint32_t repeated;
    /* under a rate, of the instances the render-owned scene keeps */
    uint32_t resting; /* the picture took no draw of them */
    uint32_t outside; /* those whose last draw was out of the game's view */
    /* the most events the ring has held and promised at once, of
     * sa_engine_desc.events, over the engine's life */
    uint32_t events;
    /* under a rate: the act's scene's parts the render-owned scene keeps.
     * resting counts those the picture took no draw of too */
    uint32_t parts;
    /* under a rate: the scene's one light and each source sa_scene_lighting
     * gave, or 0 where it gave none */
    uint32_t lights;
    uint32_t fragments_cut; /* as fragments_lost's note says */
    uint64_t between_ns; /* the host's time to draw the pictures between */
} sa_pc_stats;

/* under SA_PC_READBACK, wait for the picture last ended and copy it to rgba.
 * the copy is SA_PC_WIDTH by SA_PC_HEIGHT RGBA, top row first. a picture
 * drawn larger comes through the area filter */
sa_result sa_pc_read(sa_engine *engine, void *rgba, uint32_t bytes);
/* copy the index'th picture the display showed for the picture last ended,
 * as sa_pc_read does. index 0 is the first, and the last is the picture
 * itself. the last four pictures between are kept. an older one, or an
 * index of sa_pc_stats.shown or more, gives SA_ERR_INVALID */
sa_result sa_pc_read_shown(sa_engine *engine, uint32_t index, void *rgba,
                           uint32_t bytes);
/* under a rate and SA_PC_READBACK, between pictures: draw the render-owned
 * scene's meshes as last drawn, through picture's view. it draws those out
 * of the game's view too, with no fog. copy the result as sa_pc_read does,
 * and write the count of meshes drawn to drawn. it reads the scene alone */
sa_result sa_pc_read_overview(sa_engine *engine, const sa_picture *picture,
                              void *rgba, uint32_t bytes, uint32_t *drawn);
/* wait for the picture last ended, and give its figures */
sa_result sa_pc_stats_read(sa_engine *engine, sa_pc_stats *stats);
/* between pictures: wait for every picture begun. it lets go of the
 * render-owned scene's pictures too, so nothing reads a released resource
 * again. every release's SA_EVENT_RETIRED is queued as the call returns.
 * the next picture stands alone, as after a cut. a client that ends while
 * its engine stays calls this before it frees what it lent.
 * SA_ERR_INVALID refuses it with a picture open. SA_ERR_UNSUPPORTED says the
 * device refused a picture. the scene's pictures then stay, as does each
 * release that waits on one */
sa_result sa_pc_retire(sa_engine *engine);
/* the device's name, 0-terminated, cut to fit bytes */
sa_result sa_pc_device(sa_engine *engine, char *name, uint32_t bytes);
/* why the device draws no more, 0-terminated, to bytes of why. SA_OK says
 * the device is lost, or may hold part of a picture or texture load. from
 * then it refuses each picture and texture, and sa_pc_read and
 * sa_pc_stats_read give SA_ERR_UNSUPPORTED. the program then ends its
 * engine. SA_EMPTY says the device draws. a picture it refuses then stays
 * open and whole, for sa_present_end to be called again */
sa_result sa_pc_broken(sa_engine *engine, char *why, uint32_t bytes);

/* sound's rate, in frames a second, each frame two int16_t, left first */
#define SA_PC_SOUND_RATE 44100U
/* where no audio device plays the sound, render frames of it to out on the
 * caller's thread. it first carries out the commands queued so far */
sa_result sa_pc_audio_render(sa_engine *engine, int16_t *out, uint32_t frames);
/* what sound took: commands the queue refused, and the most it held. a
 * refused sa_audio_submit counts each of its commands. a call the queue
 * refuses changes refused, and nothing else of the engine */
typedef struct sa_pc_sound_stats {
    uint32_t refused;
    uint32_t most_queued; /* of SA_PC_SOUND_QUEUE */
    uint32_t most_voices; /* of 64 */
    uint32_t unplayed; /* notes with no program, and sequences, unheard */
} sa_pc_sound_stats;
#define SA_PC_SOUND_QUEUE 4096U
sa_result sa_pc_sound_stats_read(sa_engine *engine, sa_pc_sound_stats *stats);

#ifdef __cplusplus
}
#endif

#endif
