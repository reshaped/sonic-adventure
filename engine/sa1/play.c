/* the game in a window, or headless, in one process. a picture's records go
 * to the PC backend through sa_pc_client_record as the picture ends. a vsync
 * waits out its time on the game's own clock. each of the game's pad reads
 * takes one sample of the pad. the game keeps its FPU mode. each call out
 * runs the backend under the host's default, then puts the game's back.
 *
 * the play program's options, which the runner hands here:
 *
 *   --window ROOT | --headless ROOT   the build's images, and where pictures
 *                    go. with neither, a window on the build's own images
 *   --pace 0         no wait for a vsync's time: as fast as the host runs
 *   --pictures DIR   each picture as DIR/pic-NNNN-tTICK.png, and under a
 *                    rate each between as -bN after it
 *   --video FILE     the picture each vsync shows, as 640 by 480 RGBA rows,
 *                    under a rate each in its turn
 *   --audio FILE     headless, the sound each vsync plays, as a WAV of
 *                    SA_PC_SOUND_RATE frames a second
 *   --times FILE     a line of each picture's figures, as take_stats writes
 *                    them
 *
 * its settings are lines NAME VALUE of the settings file. an option
 * --NAME VALUE overrides the line:
 *
 *   device NAME      the first Vulkan device whose name holds NAME
 *   vsync 0          a window shows each picture at once; 1, at the blank
 *   fullscreen N     a window starts fullscreen: 1 borderless across the
 *                    desktop, 2 in the display mode nearest the desktop's.
 *                    F11, or alt and return, toggles it
 *   size N           a window opens at 640N by 480N, 2 by default
 *   resolution N     each picture drawn at 640N by 480N. 0, the default,
 *                    draws a window's at the size it shows and a headless
 *                    run's at 1. pictures and video take 640 by 480, and
 *                    the area filter reduces a larger one
 *   rate HZ          the display's pictures a second, which the backend
 *                    fills with pictures between the game's, from the scene
 *                    it keeps across pictures. they show as their time
 *                    comes, before the game's own
 *   sound 0          a window plays no sound; 1, on the default device
 *   aspect 16:9      16:9 on from the start, as a save that kept it turns
 *                    it on; 4:3, the default, leaves it to the chord and
 *                    the saves
 *   fps 60           60 fps on from the start, the same way. a frame the
 *                    game splits then shows step 0's picture and step 1's;
 *                    30, the default, leaves it to the chord and the saves
 *
 * the file's other lines bind a window's pads, with their dead zone and
 * rumble, as engine/pc/input.h names them.
 *
 * a run ends with its figures, as native_play_close reports them */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "native.h"
#include "sa_capture.h"
#include "sa_client.h"
#include "sa_engine.h"
#include "sa_pc.h"

/* a backend under SA_BORROW_PICTURE reads a draw's borrowed bytes until the
 * picture retires. records.c gives it each model as a copy of its bytes at the
 * call, named by their hash. so a model the game changes, or one with uvs on a
 * stack, makes a mesh per version */
#ifndef SA_BORROW_PICTURE
#error SA1's adapter on a host serves a backend whose header defines SA_BORROW_PICTURE
#endif

/* png.cpp's, in C */
int pc_png_write(const char *path, uint32_t width, uint32_t height,
                 const uint8_t *rgba);

/* the Dreamcast's vsync, at 59.94 Hz */
#define VSYNC_NS 16683350ULL
/* a run this far behind its clock starts it over rather than race to catch
 * up. so does a run this far ahead */
#define BEHIND_NS 100000000ULL
#define PICTURE_BYTES (SA_PC_WIDTH * SA_PC_HEIGHT * 4U)
/* the frames of sound one render takes, two samples each */
#define RENDER_FRAMES 1024U
/* the pictures a run's figures keep, an hour's at 30 a second */
#define MOST_TIMED 108000U

/* the head of Shinobi's PDS_PERIPHERAL, the game's pad record */
struct peripheral {
    uint32_t id, support, on, off, press, release;
    uint16_t r, l;
    int16_t x1, y1, x2, y2;
};

/* one picture's figures */
struct timed {
    uint64_t
        give_ns; /* its records, client and backend, on the game's thread */
    uint64_t end_ns; /* sa_present_end alone: submission, and any wait */
    uint64_t cpu_ns; /* the backend's, from sa_present_begin to its end */
    uint64_t gpu_ns; /* the device's */
    uint32_t meshes, mesh_bytes, textures; /* first named in this picture */
};

static sa_engine *engine;
static sa_pc_client *client;
static sa_pc_options options = {.flags = SA_PC_WINDOW | SA_PC_SOUND,
                                .blend = SA_PC_BLEND_PIXEL,
                                .title = "Sonic Adventure"};
static const char *build_root = NATIVE_BUILD_IMAGES;
static int paced = 1;
static int wide, fast;
static const char *pictures_dir;
static FILE *video, *times, *audio;
/* the frames of sound the audio file holds, and the time their render took */
static uint64_t audio_frames, audio_ns;
static uint8_t *shown; /* the picture a vsync shows, for --video */
/* under a rate, the pictures the display shows for the game's latest: those
 * between, then that one. each vsync takes the next, and the last holds */
#define MOST_SHOWN 4U
static uint8_t *showing[MOST_SHOWN];
static uint32_t showing_count, showing_next;
static uint64_t next_vsync;
static int recorded_pads;
static struct timed *timed;
static uint32_t timed_count;
static uint64_t vsyncs, late_vsyncs, lost_vsyncs;
/* the most a picture took of the backend's bounds */
static uint32_t most_arena, most_shades, most_triangles, most_lost, most_cut,
    most_events;
/* under a rate, the pictures shown that repeat the game's own for want of room
 * in the scene. last_full is the SA_PC_FULL_* store that last had no room */
static uint64_t repeated;
static uint32_t last_full;
static int pending_stats;

static uint64_t now_ns(void)
{
    struct timespec time;

    clock_gettime(CLOCK_MONOTONIC, &time);
    return (uint64_t)time.tv_sec * 1000000000ULL + (uint64_t)time.tv_nsec;
}

/* a whole number from 0 to most */
static int whole(const char *value, uint32_t most, uint32_t *number)
{
    char *end;
    unsigned long n = strtoul(value, &end, 10);

    *number = (uint32_t)n;
    return value[0] >= '0' && value[0] <= '9' && !*end && n <= most;
}

/* take one of the settings. it returns 1 for its own name, -1 for a value it
 * refuses, and 0 for another's name */
static int setting(const char *name, const char *value)
{
    uint32_t n;

    if (!strcmp(name, "device")) {
        options.device = strdup(value);
        return value[0] ? 1 : -1;
    }
    if (!strcmp(name, "rate")) {
        char *end;
        double hz = strtod(value, &end);

        options.rate = (uint32_t)(hz * 1000.0 + 0.5);
        return value[0] && !*end && hz >= 0 && hz <= 1000 ? 1 : -1;
    }
    if (!strcmp(name, "size")) {
        return whole(value, 8, &options.scale) ? 1 : -1;
    }
    if (!strcmp(name, "resolution")) {
        return whole(value, 16, &options.resolution) ? 1 : -1;
    }
    if (!strcmp(name, "fullscreen")) {
        if (!whole(value, 2, &n)) {
            return -1;
        }
        options.flags &= ~(uint32_t)(SA_PC_FULLSCREEN | SA_PC_EXCLUSIVE);
        options.flags |= n == 0   ? 0
                         : n == 1 ? SA_PC_FULLSCREEN
                                  : SA_PC_FULLSCREEN | SA_PC_EXCLUSIVE;
        return 1;
    }
    if (!strcmp(name, "vsync")) {
        if (!whole(value, 1, &n)) {
            return -1;
        }
        options.flags = n ? options.flags & ~(uint32_t)SA_PC_NO_VSYNC
                          : options.flags | SA_PC_NO_VSYNC;
        return 1;
    }
    if (!strcmp(name, "sound")) {
        if (!whole(value, 1, &n)) {
            return -1;
        }
        options.flags = n ? options.flags | SA_PC_SOUND
                          : options.flags & ~(uint32_t)SA_PC_SOUND;
        return 1;
    }
    if (!strcmp(name, "aspect")) {
        wide = !strcmp(value, "16:9");
        return wide || !strcmp(value, "4:3") ? 1 : -1;
    }
    if (!strcmp(name, "fps")) {
        fast = !strcmp(value, "60");
        return fast || !strcmp(value, "30") ? 1 : -1;
    }
    return 0;
}

void native_play_settings(char *text, const char *path)
{
    native_settings_take(text, path, setting);
    options.settings = text;
}

void native_play_alert(const char *words)
{
    if (options.flags & SA_PC_WINDOW) {
        sa_pc_alert(options.title, words);
    }
}

int native_play_option(const char *name, const char *value)
{
    int took;

    if (!strcmp(name, "--window") || !strcmp(name, "--headless")) {
        build_root = value;
        options.flags = name[2] == 'w' ? options.flags | SA_PC_WINDOW
                                       : options.flags & ~SA_PC_WINDOW;
    } else if (!strcmp(name, "--pace")) {
        paced = atoi(value) != 0;
    } else if (!strcmp(name, "--pictures")) {
        pictures_dir = value;
        options.flags |= SA_PC_READBACK;
    } else if (!strcmp(name, "--video")) {
        if (!(video = fopen(value, "wb"))) {
            fprintf(stderr, "sa1: can't write %s\n", value);
            exit(2);
        }
        options.flags |= SA_PC_READBACK;
    } else if (!strcmp(name, "--audio")) {
        static const uint8_t wav[44] = {0};

        if (!(audio = fopen(value, "wb"))) {
            fprintf(stderr, "sa1: can't write %s\n", value);
            exit(2);
        }
        fwrite(wav, 1, sizeof wav, audio);
    } else if (!strcmp(name, "--times")) {
        if (!(times = fopen(value, "w"))) {
            fprintf(stderr, "sa1: can't write %s\n", value);
            exit(2);
        }
    } else if (!strncmp(name, "--", 2) && (took = setting(name + 2, value))) {
        if (took < 0) {
            fprintf(stderr, "sa1: %s won't take \"%s\"\n", name, value);
            exit(2);
        }
    } else {
        return 0;
    }
    return 1;
}

static int by_value(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;

    return x < y ? -1 : x > y;
}

/* the middle, the 99th and the worst of one figure over the run, in ms */
static void report(const char *what, size_t field)
{
    uint64_t *ns = malloc(sizeof *ns * (timed_count ? timed_count : 1));

    for (uint32_t i = 0; i < timed_count; i++) {
        memcpy(&ns[i], (const char *)&timed[i] + field, sizeof ns[i]);
    }
    qsort(ns, timed_count, sizeof *ns, by_value);
    if (timed_count) {
        fprintf(stderr,
                "sa1: %s a picture: median %.3f ms, 99th %.3f, most %.3f\n",
                what, (double)ns[timed_count / 2] / 1e6,
                (double)ns[(uint64_t)timed_count * 99 / 100] / 1e6,
                (double)ns[timed_count - 1] / 1e6);
    }
    free(ns);
}

static void put32(uint8_t *at, uint32_t value)
{
    for (int i = 0; i < 4; i++) {
        at[i] = (uint8_t)(value >> (8 * i));
    }
}

/* the WAV's header, once its frames are known */
static void close_audio(void)
{
    uint8_t head[44] = "RIFF....WAVEfmt ";
    uint32_t data = (uint32_t)(audio_frames * 4);

    put32(head + 4, 36 + data);
    put32(head + 16, 16);
    put32(head + 20, 1 | 2 << 16);
    put32(head + 24, SA_PC_SOUND_RATE);
    put32(head + 28, SA_PC_SOUND_RATE * 4);
    put32(head + 32, 4 | 16 << 16);
    memcpy(head + 36, "data", 4);
    put32(head + 40, data);
    fseek(audio, 0, SEEK_SET);
    fwrite(head, 1, sizeof head, audio);
    fclose(audio);
}

static void print_sound(void)
{
    sa_pc_sound_stats s;

    if (sa_pc_sound_stats_read(engine, &s) == SA_OK) {
        fprintf(stderr,
                "sa1: sound refused %u commands, held at most %u of %u, "
                "%u voices of 64, %u notes and sequences unheard\n",
                s.refused, s.most_queued, SA_PC_SOUND_QUEUE, s.most_voices,
                s.unplayed);
    }
    if (audio_frames) {
        fprintf(stderr, "sa1: %.1f s of sound rendered in %.1f ms\n",
                (double)audio_frames / SA_PC_SOUND_RATE,
                (double)audio_ns / 1e6);
    }
}

/* the sound a vsync plays, which the frames due by its end give */
static void render_audio(void)
{
    int16_t frames[2 * RENDER_FRAMES];
    uint64_t due = vsyncs * SA_PC_SOUND_RATE * VSYNC_NS / 1000000000ULL;
    uint32_t game = native_fpu_host();

    while (audio_frames < due) {
        uint32_t count = due - audio_frames < RENDER_FRAMES
                             ? (uint32_t)(due - audio_frames)
                             : RENDER_FRAMES;

        uint64_t began = now_ns();

        if (sa_pc_audio_render(engine, frames, count) != SA_OK) {
            memset(frames, 0, sizeof frames);
        }
        audio_ns += now_ns() - began;
        fwrite(frames, 4, count, audio);
        audio_frames += count;
    }
    native_fpu_game(game);
}

void native_play_close(void)
{
    char device[256] = "";

    /* a run given a disc and none of play.c's options opens no engine */
    if (!engine) {
        return;
    }
    sa_pc_device(engine, device, sizeof device);
    fprintf(
        stderr,
        "sa1: %u pictures on %s, %llu vsyncs, %llu reached late, %llu lost\n",
        timed_count, device, (unsigned long long)vsyncs,
        (unsigned long long)late_vsyncs, (unsigned long long)lost_vsyncs);
    report("the game's thread gave", offsetof(struct timed, give_ns));
    report("sa_present_end took", offsetof(struct timed, end_ns));
    report("the backend's CPU took", offsetof(struct timed, cpu_ns));
    report("the device took", offsetof(struct timed, gpu_ns));
    fprintf(stderr,
            "sa1: a picture took at most %u bytes of its arena's %u, %u "
            "shades, %u triangles, and left out at most %u blend fragments "
            "its lists had no room for and %u its pixels held past %u\n",
            most_arena, SA_PC_ARENA, most_shades, most_triangles, most_lost,
            most_cut, SA_PC_PIXEL_FRAGMENTS);
    if (options.rate != 0) {
        fprintf(stderr,
                "sa1: %llu pictures shown were the game's own again for want "
                "of room in the scene, last in store %u\n",
                (unsigned long long)repeated, last_full);
    }
    fprintf(stderr, "sa1: the event ring held at most %u of %u\n", most_events,
            SA_PC_EVENTS);
    if (times) {
        fclose(times);
    }
    if (video) {
        fclose(video);
    }
    if (audio) {
        close_audio();
    }
    if (native_sound_engine) {
        print_sound();
    }
    sa_pc_client_report(client);
    sa1_names_report();
    fflush(stdout);
}

/* the last picture's figures. sa_pc_stats_read waits for the picture, so they
 * are read as the next is given. a device that drew it in that frame's time
 * makes no wait */
static void take_stats(void)
{
    sa_pc_stats stats;

    if (!pending_stats || sa_pc_stats_read(engine, &stats) != SA_OK) {
        return;
    }
    pending_stats = 0;
    most_arena = stats.arena > most_arena ? stats.arena : most_arena;
    most_shades = stats.shades > most_shades ? stats.shades : most_shades;
    most_triangles =
        stats.triangles > most_triangles ? stats.triangles : most_triangles;
    most_lost =
        stats.fragments_lost > most_lost ? stats.fragments_lost : most_lost;
    most_cut = stats.fragments_cut > most_cut ? stats.fragments_cut : most_cut;
    most_events = stats.events;
    repeated += stats.repeated;
    last_full = stats.full != 0 ? stats.full : last_full;
    timed[timed_count - 1].cpu_ns = stats.cpu_ns;
    timed[timed_count - 1].gpu_ns = stats.gpu_ns;
    sa1_figures.device_ns = stats.gpu_ns;
    if (times) {
        const struct timed *t = &timed[timed_count - 1];

        fprintf(times,
                "%u\t%llu\t%llu\t%llu\t%llu\t%llu\t%u\t%u\t%u\t%u\t%u\t"
                "%llu\t%llu\n",
                timed_count - 1, (unsigned long long)native_vsyncs,
                (unsigned long long)t->give_ns, (unsigned long long)t->end_ns,
                (unsigned long long)t->cpu_ns, (unsigned long long)t->gpu_ns,
                stats.draws, stats.triangles, t->meshes, t->mesh_bytes,
                t->textures, (unsigned long long)late_vsyncs,
                (unsigned long long)lost_vsyncs);
    }
}

/* keep the picture last ended for --video, and as --pictures' PNGs. under a
 * rate, its pictures between come first, one a vsync */
static void keep_picture(uint32_t picture, uint32_t tick)
{
    sa_pc_stats stats = {0};
    uint32_t count = 1;

    if (options.rate != 0 && sa_pc_stats_read(engine, &stats) == SA_OK) {
        count = stats.shown < 1 ? 1 : stats.shown;
    }
    showing_count = 0;
    showing_next = 0;
    for (uint32_t k = count > MOST_SHOWN ? count - MOST_SHOWN : 0; k < count;
         k++) {
        uint8_t *rgba = showing[showing_count];

        if ((options.rate == 0
                 ? sa_pc_read(engine, rgba, PICTURE_BYTES)
                 : sa_pc_read_shown(engine, k, rgba, PICTURE_BYTES)) != SA_OK) {
            continue;
        }
        showing_count++;
        if (pictures_dir) {
            char path[4096];

            if (k + 1 < count) {
                snprintf(path, sizeof path, "%s/pic-%04u-t%u-b%u.png",
                         pictures_dir, picture, tick, k);
            } else {
                snprintf(path, sizeof path, "%s/pic-%04u-t%u.png", pictures_dir,
                         picture, tick);
            }
            pc_png_write(path, SA_PC_WIDTH, SA_PC_HEIGHT, rgba);
        }
    }
}

/* end the run where the device draws no more. a player reads why in a window's
 * alert */
static void close_broken(void)
{
    char why[SA_PC_WHY];
    char words[SA_PC_WHY + 32];

    if (sa_pc_broken(engine, why, sizeof why) != SA_OK) {
        return;
    }
    snprintf(words, sizeof words, "can't draw the game: %s", why);
    fprintf(stderr, "sa1: %s\n", words);
    native_play_alert(words);
    native_play_close();
    exit(2);
}

static void give_picture(const uint8_t *records, size_t bytes)
{
    uint32_t game = native_fpu_host();
    uint64_t began = now_ns(), end_ns = 0;
    uint32_t picture = 0, tick = 0;
    uint32_t meshes = 0, mesh_bytes = 0, textures = 0;
    int ended = 0;

    take_stats();
    for (size_t at = 0; at + sizeof(sa_record_head) <= bytes;) {
        sa_record_head head;
        sa_result result;
        uint64_t before = now_ns();

        memcpy(&head, records + at, sizeof head);
        if (head.kind == SA_CAP_PICTURE &&
            head.size >= sizeof(sa_cap_picture)) {
            sa_cap_picture begun;

            memcpy(&begun, records + at, sizeof begun);
            picture = begun.picture;
            tick = begun.tick;
        }
        meshes += head.kind == SA_CAP_MESH;
        mesh_bytes += head.kind == SA_CAP_MESH ? head.size : 0;
        textures += head.kind == SA_CAP_TEXTURE;
        result = sa_pc_client_record(client, records + at);
        if (head.kind == SA_CAP_END) {
            end_ns += now_ns() - before;
        }
        if (result == SA_PC_QUIT) {
            native_play_close();
            exit(0);
        }
        /* a picture the engine refused may be one the device can't draw */
        if (result < 0) {
            close_broken();
        }
        ended |= result == SA_PC_ENDED;
        at += head.size;
    }
    if (ended && timed_count < MOST_TIMED) {
        timed[timed_count].give_ns = now_ns() - began;
        timed[timed_count].end_ns = end_ns;
        timed[timed_count].meshes = meshes;
        timed[timed_count].mesh_bytes = mesh_bytes;
        timed[timed_count].textures = textures;
        timed_count++;
        pending_stats = 1;
    }
    if (ended && shown) {
        keep_picture(picture, tick);
    }
    native_fpu_game(game);
}

/* a vsync: the game's clock, and a video's frame */
static void wait_vsync(void)
{
    uint64_t now = now_ns();

    vsyncs++;
    if (audio) {
        render_audio();
    }
    if (showing_count != 0) {
        shown = showing[showing_next];
        showing_next += showing_next + 1 < showing_count;
    }
    if (video) {
        fwrite(shown, 1, PICTURE_BYTES, video);
    }
    if (!paced) {
        return;
    }
    next_vsync += VSYNC_NS;
    if (now > next_vsync) {
        late_vsyncs++;
        lost_vsyncs += (now - next_vsync) / VSYNC_NS;
        sa1_figures.late = late_vsyncs;
        sa1_figures.lost = lost_vsyncs;
    }
    if (now + BEHIND_NS < next_vsync || now > next_vsync + BEHIND_NS) {
        next_vsync = now;
        return;
    }
    now = now_ns();
    if (options.rate != 0) {
        /* the backend shows each picture between as its time comes, while the
         * game waits */
        uint32_t game = native_fpu_host();

        sa_clock_wait(engine, next_vsync);
        native_fpu_game(game);
    } else if (now < next_vsync) {
        struct timespec wait = {0, (long)(next_vsync - now)};

        nanosleep(&wait, NULL);
    }
    sa1_figures.waited_ns += now_ns() - now;
}

/* one sample of the pad, as the game's own read begins */
static void read_pad(void *record)
{
    struct peripheral *pad = record;
    uint32_t game = native_fpu_host();
    sa_pad sample;

    if (sa_input_sample(engine, 0, &sample) == SA_OK) {
        pad->press = sample.on & ~pad->on;
        pad->release = pad->on & ~sample.on;
        pad->on = sample.on;
        pad->off = ~sample.on;
        pad->x1 = sample.x1;
        pad->y1 = sample.y1;
        pad->x2 = sample.x2;
        pad->y2 = sample.y2;
        pad->r = sample.r;
        pad->l = sample.l;
    }
    native_fpu_game(game);
}

/* the live pad's shaking, as the slice's vibration calls give it */
static int rumble(uint32_t strength, uint32_t ms, uint32_t fade_ms)
{
    uint32_t game = native_fpu_host();
    sa_result result;

    result = sa_input_rumble(engine, 0, (uint16_t)strength, (uint16_t)strength,
                             ms, fade_ms);
    native_fpu_game(game);
    return result == SA_OK;
}

void native_play_open(const char *disc, int recorded)
{
    /* two pictures in flight, which a rate needs */
    sa_engine_desc desc = {.abi = SA_ENGINE_ABI_VERSION,
                           .size = sizeof desc,
                           .game = SA_GAME_SA1,
                           .arena = SA_PC_ARENA,
                           .pictures = 2,
                           .resources = SA_PC_RESOURCES,
                           .events = SA_PC_EVENTS,
                           .platform = &options};
    const char *roots[3] = {build_root, disc, NULL};
    uint32_t bytes = 0;
    void *memory = NULL;
    sa_result result = sa_engine_memory(&desc, &bytes);
    static char why[SA_PC_WHY];
    char words[512];
    FILE *label =
        options.flags & SA_PC_WINDOW ? native_disc_file("0GDTEX.PVR") : NULL;
    static uint8_t icon[1 << 20];

    /* the window's icon, the disc's label as the console's menu shows it */
    if (label) {
        options.icon = icon;
        options.icon_bytes = (uint32_t)fread(icon, 1, sizeof icon, label);
        fclose(label);
    }
    options.why = why;
    if (result == SA_OK) {
#ifdef _WIN32
        memory = _aligned_malloc(bytes, 32);
#else
        if (posix_memalign(&memory, 32, bytes) != 0) {
            memory = NULL;
        }
#endif
    }
    if (result != SA_OK || !memory ||
        (result = sa_engine_create(&desc, memory, bytes, &engine)) != SA_OK) {
        if (!why[0] && result == SA_OK) {
            snprintf(why, sizeof why, "no memory for the engine's %u bytes",
                     bytes);
        } else if (!why[0]) {
            snprintf(why, sizeof why, "the engine's error %d", result);
        }
        snprintf(words, sizeof words, "can't draw the game: %s", why);
        fprintf(stderr, "sa1: %s\n", words);
        native_play_alert(words);
        exit(2);
    }
    client = sa_pc_client_create(engine, roots);
    timed = calloc(MOST_TIMED, sizeof *timed);
    if (options.flags & SA_PC_READBACK) {
        for (uint32_t k = 0; k < MOST_SHOWN; k++) {
            showing[k] = calloc(1, PICTURE_BYTES);
        }
        shown = showing[0];
    }
    native_engine = engine;
    native_card_engine = engine;
    /* sound where something hears it: a window's device, or a WAV */
    if (audio ||
        ((options.flags & SA_PC_WINDOW) && (options.flags & SA_PC_SOUND))) {
        native_sound_engine = engine;
    }
    recorded_pads = recorded;
    if (wide) {
        sa1_wide_on();
    }
    if (fast) {
        sa1_fast_on();
    }
    next_vsync = now_ns();
    native_picture_sink = give_picture;
    native_pace = wait_vsync;
    /* a window reads the live pad and shakes its pack. a headless run, or one
     * whose pad keys or a hook give, has neither */
    if (!recorded_pads && (options.flags & SA_PC_WINDOW)) {
        native_pad_read = read_pad;
        native_rumble = rumble;
    }
}
