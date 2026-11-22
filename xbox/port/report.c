/* the report: what each frame's parts took, against what they usually take,
 * the frames that ran slow, the tables' and the heaps' use, and where the
 * game is, logged when the host asks (port_report: the pad's Back on the
 * Xbox). each frame is measured at the end of port_render (render.c), where
 * port_profile is on; the frames of the last few seconds stay in
 * port_frames, which the host reads itself after a fault or a stall */
#include <kamui.h>
#include <ninja.h>
#include <shinobi.h>
#include <sg_xpt.h>
#include <stddef.h>
#include <stdio.h>

#include "main.h"
#include "stage/stage.h"
#include "system/task.h"
#include "xbox/port/host.h"
#include "xbox/port/port.h"

/* the game's, declared here rather than by their headers, which bring in
 * much more */
extern Sint16 stage_current, act_current;
extern Uint16 character_current;
extern struct task_work *player_work[];
extern struct task_motion *player_motion[]; /* src/player/player.c's: each player's speed */
extern struct task_work *camera_work;
extern int frame_interval; /* src/system/frame.c's: the blanks a frame is meant to take */
/* src/system/frame.c's: the steps of play a frame takes, and the steps the
 * last frame of play took: a stage takes one more where its first two ran
 * long (stage.c's stage_run_action), which moves everything half as far
 * again in one picture: a jump the frame's time doesn't show */
extern int frame_step, frame_step_count;
#define GAME_HEAP_SIZE 0x90000 /* src/system/task.c's HEAP_SIZE */

struct port_frame port_frames[PORT_FRAMES];
volatile unsigned port_frame_count;
struct port_where port_where;
volatile int port_report;
volatile struct port_place port_place;

static const char *const phase_names[PHASES] = {
    "frame", "busy", "logic", "drawer", "render", "lists", "draws", "sort", "gpu wait", "blank wait", "sound", "idle",
};

/* what each phase usually takes: a mean that forgets over some 256 frames,
 * in microseconds. a frame's value counts at most four times the mean, so
 * that a load's long frame doesn't lift it for long */
static float usual[PHASES];
static int usual_ready;
/* the most each phase has taken since the start, and in which frame */
static unsigned short most[PHASES];
static unsigned most_frame[PHASES];
/* the slow frames since the last report: a frame that missed its blank,
 * longer than frame_interval's blanks and 4 ms. the newest SLOW_KEPT of them, with the usual of each
 * phase as it was then */
#define SLOW_KEPT 16
static struct {
    struct port_frame frame;
    unsigned short usual[PHASES];
} slow[SLOW_KEPT];
static unsigned slow_count;
/* the frames since the last report that took more steps of play than meant */
static struct port_frame extra_frames[SLOW_KEPT];
static unsigned extra_count;

/* the timestamp counter's rate, counts a microsecond, measured over the
 * frames since the last calibration */
static unsigned long long calibrate_tsc;
static unsigned calibrate_us;
static unsigned tsc_per_us;

/* what the frame's busy time was (port_frame_busy), the host's times since
 * the last frame, and the counts at the last frame */
static unsigned busy_us;
static unsigned host_sums[4];
static unsigned long long drawer_last;
static unsigned models_last, gpu_vertices_last;
static unsigned frame_us_last;

void port_frame_busy(unsigned us)
{
    busy_us = us;
}

void port_report_counts_reset(void)
{
    drawer_last = 0;
    models_last = gpu_vertices_last = 0;
}

/* the host's times since the last call, for the overlay (render.c), which
 * once asked the host itself: the report asks it each frame */
void port_report_host_us(unsigned out[4])
{
    int k;

    for (k = 0; k < 4; k++) {
        out[k] = host_sums[k];
        host_sums[k] = 0;
    }
}

static unsigned short clamp16(unsigned long long value)
{
    return (unsigned short)(value > 65535 ? 65535 : value);
}

static unsigned to_us(unsigned long long tsc)
{
    return tsc_per_us != 0 ? (unsigned)(tsc / tsc_per_us) : 0;
}

/* because Kamui's texture memory can run out over a long play, stage after
 * stage, its free memory and largest piece are logged at each change of
 * game_mode and in the report, to see what keeps textures past their stage */
KMSTATUS kmGetFreeTextureMem(PKMUINT32 pSizeOfTexture, PKMUINT32 pMaxBlockSizeOfTexture);

static void log_texture_memory(const char *when)
{
    static char line[160];
    KMUINT32 size = 0, block = 0;

    kmGetFreeTextureMem(&size, &block);
    sprintf(line, "kamui: texture memory free %u KB, the largest piece %u KB, %s", (unsigned)(size >> 10),
            (unsigned)(block >> 10), when);
    port_log(line);
}

static void note_where(void)
{
    struct task_work *player = player_work[0];
    struct task_work *camera = camera_work;

    {
        static int last_mode = -1;
        static char when[64];

        if ((int)game_mode != last_mode) {
            last_mode = (int)game_mode;
            sprintf(when, "at game_mode %d, stage %d act %d", (int)game_mode, stage_current, act_current);
            log_texture_memory(when);
        }
    }
    port_where.game_mode = (int)game_mode;
    port_where.stage = stage_current;
    port_where.act = act_current;
    port_where.character = character_current;
    port_where.player = player != NULL;
    if (player != NULL) {
        port_where.player_pos[0] = player->pos.x;
        port_where.player_pos[1] = player->pos.y;
        port_where.player_pos[2] = player->pos.z;
        port_where.player_ang[0] = player->ang.x;
        port_where.player_ang[1] = player->ang.y;
        port_where.player_ang[2] = player->ang.z;
    }
    {
        /* because a place that isn't a number makes the 3D vanish (models
         * placed by it go nowhere, the HUD stays), the player's or the
         * camera's is logged as it starts and as it ends */
        static int was_bad;
        int bad = 0;
        static char line[200];

        if (player != NULL) {
            bad |= !(player->pos.x == player->pos.x && player->pos.y == player->pos.y &&
                     player->pos.z == player->pos.z);
        }
        if (camera != NULL) {
            bad |= (!(camera->pos.x == camera->pos.x && camera->pos.y == camera->pos.y &&
                      camera->pos.z == camera->pos.z)) << 1;
        }
        if (bad != was_bad) {
            sprintf(line, "report: at frame %u, game_mode %d, stage %d act %d: the player's place %s, the camera's %s",
                    port_frame_count, (int)game_mode, stage_current, act_current,
                    (bad & 1) ? "isn't a number" : "is a number", (bad & 2) ? "isn't a number" : "is a number");
            port_log(line);
            was_bad = bad;
        }
    }
    port_where.camera = camera != NULL;
    if (camera != NULL) {
        port_where.camera_pos[0] = camera->pos.x;
        port_where.camera_pos[1] = camera->pos.y;
        port_where.camera_pos[2] = camera->pos.z;
        port_where.camera_ang[0] = camera->ang.x;
        port_where.camera_ang[1] = camera->ang.y;
        port_where.camera_ang[2] = camera->ang.z;
    }
}

int port_place_enter(void)
{
    if (!port_place.armed || !port_place.enter) {
        return 0;
    }
    port_place.enter = 0;
    set_character_number((Sint16)port_place.character);
    stage_set_stage_act((Uint8)port_place.stage, (Uint8)port_place.act);
    port_log("report: the report's stage entered from the menus");
    return 1;
}

int port_place_take(int player, NJS_POINT3 *pos, struct angles *ang)
{
    if (player != 0 || !port_place.armed || port_place.stage != stage_current || port_place.act != act_current) {
        return 0;
    }
    port_place.armed = 0;
    pos->x = port_place.pos[0];
    pos->y = port_place.pos[1];
    pos->z = port_place.pos[2];
    ang->x = port_place.ang[0];
    ang->y = port_place.ang[1];
    ang->z = port_place.ang[2];
    port_log("report: player 0 placed at the report's place");
    return 1;
}

static void report_log(void);

/* the watch: player 0's last frames, for the moment the game goes out of
 * reason, and frames that drew far less than the frames on either side.
 * because player 0 and the camera can be thrown out of the level (a sweep
 * of more than 2^31 units first), and a frame can flash, both are logged
 * as they happen */
#define WATCH_FRAMES 32
static struct watch {
    unsigned number;
    int mode, submode;
    float pos[3], speed[3];
    unsigned short strips, models;
} watched[WATCH_FRAMES];
static unsigned watch_count;
/* the models each of the last three frames drew (port_count_model), for a
 * frame that drew less: those drawn on either side of it and not in it */
#define WATCH_MODELS 1024
static const void *watch_models[3][WATCH_MODELS];
static unsigned watch_model_count[3];

void port_watch_model(const void *model)
{
    unsigned slot = watch_count % 3;

    if (watch_model_count[slot] < WATCH_MODELS) {
        watch_models[slot][watch_model_count[slot]++] = model;
    }
}

static int watch_drew(unsigned slot, const void *model)
{
    unsigned k;

    for (k = 0; k < watch_model_count[slot]; k++) {
        if (watch_models[slot][k] == model) {
            return 1;
        }
    }
    return 0;
}

/* the models frame slot dip lacks that slots before and after both drew,
 * 16 at most, as addresses to name by the images' maps (symbolize.py) */
static void watch_missing(unsigned before, unsigned dip, unsigned after)
{
    static char line[256];
    unsigned k, shown = 0, missing = 0;
    int used = sprintf(line, "watch:   not drawn there:");

    for (k = 0; k < watch_model_count[before]; k++) {
        const void *model = watch_models[before][k];

        if (!watch_drew(dip, model) && watch_drew(after, model)) {
            missing++;
            if (shown < 16) {
                used += sprintf(line + used, " %08x", (unsigned)model);
                shown++;
            }
        }
    }
    sprintf(line + used, " (%u)", missing);
    port_log(line);
}
static int watch_thrown; /* logged once a run */
static unsigned watch_dips;
#define WATCH_PLACE 1.0e6F /* farther than any stage reaches */
#define WATCH_SPEED 1.0e4F /* a step's speed no play makes */

static int out_of_reason(const float v[3], float bound)
{
    int k;

    for (k = 0; k < 3; k++) {
        if (!(v[k] < bound && v[k] > -bound)) {
            return 1;
        }
    }
    return 0;
}

static void watch_log(const struct watch *w)
{
    static char line[256];

    sprintf(line, "watch:   frame %u: mode %d/%d, at %g %g %g, speed %g %g %g; %u strips, %u models", w->number, w->mode,
            w->submode, w->pos[0], w->pos[1], w->pos[2], w->speed[0], w->speed[1], w->speed[2], w->strips, w->models);
    port_log(line);
}

/* player 0's last frames, oldest first: from a sweep out of reason
 * (src/collision/polygon.c), which may never reach the next frame */
void port_watch_dump(void)
{
    unsigned n = watch_count < WATCH_FRAMES ? watch_count : WATCH_FRAMES;
    unsigned j;

    port_log("watch: player 0's last frames, oldest first:");
    for (j = watch_count - n; j < watch_count; j++) {
        watch_log(&watched[j % WATCH_FRAMES]);
    }
}

static void watch_frame(const struct port_frame *f)
{
    struct task_work *player = player_work[0];
    struct task_motion *motion = player != NULL ? player_motion[0] : NULL;
    struct watch *w = &watched[watch_count % WATCH_FRAMES];
    int k;

    w->number = f->number;
    w->strips = f->strips;
    w->models = f->models;
    w->mode = w->submode = -1;
    for (k = 0; k < 3; k++) {
        w->pos[k] = w->speed[k] = 0.0F;
    }
    if (player != NULL) {
        w->mode = player->mode;
        w->submode = player->submode;
        w->pos[0] = player->pos.x;
        w->pos[1] = player->pos.y;
        w->pos[2] = player->pos.z;
    }
    if (motion != NULL) {
        w->speed[0] = motion->speed.x;
        w->speed[1] = motion->speed.y;
        w->speed[2] = motion->speed.z;
    }
    watch_count++;
    if (player != NULL && !watch_thrown && (out_of_reason(w->pos, WATCH_PLACE) || out_of_reason(w->speed, WATCH_SPEED))) {
        unsigned n = watch_count < WATCH_FRAMES ? watch_count : WATCH_FRAMES;
        unsigned j;

        watch_thrown = 1;
        port_log("watch: player 0's place or speed is out of reason: its last frames, oldest first, and a report");
        for (j = watch_count - n; j < watch_count; j++) {
            watch_log(&watched[j % WATCH_FRAMES]);
        }
        port_report = 1;
    }
    /* the frame before this one, against the frames on either side */
    if (watch_count >= 3 && watch_dips < 32) {
        const struct watch *before = &watched[(watch_count - 3) % WATCH_FRAMES];
        const struct watch *dip = &watched[(watch_count - 2) % WATCH_FRAMES];
        const struct watch *after = &watched[(watch_count - 1) % WATCH_FRAMES];

        /* in play: a load's frames draw next to nothing */
        if (before->strips >= 200 && after->strips >= 200 &&
            ((dip->strips * 5u < before->strips * 4u && dip->strips * 5u < after->strips * 4u) ||
             (dip->models * 5u < before->models * 4u && dip->models * 5u < after->models * 4u))) {
            static char line[160];

            watch_dips++;
            sprintf(line, "watch: frame %u drew far less than either side (%u and %u strips, %u and %u models):",
                    dip->number, before->strips, after->strips, before->models, after->models);
            port_log(line);
            watch_log(dip);
            watch_missing((watch_count - 3) % 3, (watch_count - 2) % 3, (watch_count - 1) % 3);
        }
    }
    /* the next frame's models go in the oldest slot, compared above */
    watch_model_count[watch_count % 3] = 0;
}

void port_report_frame(unsigned long long render_tsc, unsigned long long lists_tsc, unsigned long long draws_tsc,
                       unsigned long long sort_tsc, int strips, int vertices)
{
    static unsigned number;
    struct port_frame *f = &port_frames[port_frame_count % PORT_FRAMES];
    unsigned now_us = HOST->time_us();
    unsigned long long now_tsc = __builtin_ia32_rdtsc();
    unsigned host_us[4] = {0, 0, 0, 0};
    unsigned drawer, frame, used;
    int k;

    /* the counter's rate, again every second or so */
    if (calibrate_us == 0) {
        calibrate_us = now_us;
        calibrate_tsc = now_tsc;
    } else if (now_us - calibrate_us >= 1000000) {
        tsc_per_us = (unsigned)((now_tsc - calibrate_tsc) / (now_us - calibrate_us));
        calibrate_us = now_us;
        calibrate_tsc = now_tsc;
    }
    HOST->stats(host_us);
    for (k = 0; k < 4; k++) {
        host_sums[k] += host_us[k];
    }
    drawer = to_us(port_counts.drawer_tsc - drawer_last);
    drawer_last = port_counts.drawer_tsc;
    frame = frame_us_last != 0 ? now_us - frame_us_last : 0;
    frame_us_last = now_us;

    f->number = ++number;
    f->us[PHASE_FRAME] = clamp16(frame);
    f->us[PHASE_BUSY] = clamp16(busy_us);
    f->us[PHASE_LOGIC] = clamp16(busy_us > drawer ? busy_us - drawer : 0);
    f->us[PHASE_DRAWER] = clamp16(drawer);
    f->us[PHASE_RENDER] = clamp16(to_us(render_tsc));
    f->us[PHASE_LISTS] = clamp16(to_us(lists_tsc));
    f->us[PHASE_DRAWS] = clamp16(to_us(draws_tsc));
    f->us[PHASE_SORT] = clamp16(to_us(sort_tsc));
    f->us[PHASE_GPU_WAIT] = clamp16(host_us[0]);
    f->us[PHASE_BLANK_WAIT] = clamp16(host_us[1]);
    f->us[PHASE_SOUND] = clamp16(host_us[2]);
    used = busy_us + to_us(render_tsc);
    f->us[PHASE_IDLE] = clamp16(frame > used ? frame - used : 0);
    f->steps = (unsigned char)(frame_step > 255 ? 255 : frame_step < 0 ? 0 : frame_step);
    f->steps_meant = (unsigned char)(frame_step_count > 255 ? 255 : frame_step_count < 0 ? 0 : frame_step_count);
    f->strips = (unsigned short)strips;
    f->vertices = (unsigned)vertices;
    f->models = (unsigned short)(port_counts.drawer_models - models_last);
    f->gpu_vertices = port_counts.gpu_vertices - gpu_vertices_last;
    models_last = port_counts.drawer_models;
    gpu_vertices_last = port_counts.gpu_vertices;
    if (f->steps > f->steps_meant && f->steps_meant != 0) {
        int at = (int)(extra_count++ % SLOW_KEPT);

        extra_frames[at] = *f;
    }
    busy_us = 0;
    port_frame_count++;

    /* the first frames, before the counter's rate is known, and the first
     * of all, which has no frame before it, set nothing */
    if (tsc_per_us != 0 && frame != 0) {
        if (!usual_ready) {
            usual_ready = 1;
            for (k = 0; k < PHASES; k++) {
                usual[k] = f->us[k];
            }
        }
        if (f->us[PHASE_FRAME] > (unsigned)(frame_interval > 1 ? frame_interval : 1) * 16683u + 4000u) {
            int at = (int)(slow_count++ % SLOW_KEPT);

            slow[at].frame = *f;
            for (k = 0; k < PHASES; k++) {
                slow[at].usual[k] = clamp16((unsigned long long)usual[k]);
            }
        }
        for (k = 0; k < PHASES; k++) {
            float value = f->us[k];

            if (f->us[k] > most[k]) {
                most[k] = f->us[k];
                most_frame[k] = f->number;
            }
            if (value > usual[k] * 4.0F + 1000.0F) {
                value = usual[k] * 4.0F + 1000.0F;
            }
            usual[k] += (value - usual[k]) * (1.0F / 256.0F);
        }
    }
    note_where();
    watch_frame(f);
    if (port_report) {
        report_log();
        port_report = 0;
    }
}

/* tenths of a millisecond, from microseconds */
#define MS(us) (unsigned)(us) / 1000, (unsigned)(us) / 100 % 10

static void report_log(void)
{
    static char line[448];
    unsigned have = port_frame_count < PORT_FRAMES ? port_frame_count : PORT_FRAMES;
    unsigned window = have < 60 ? have : 60;
    unsigned k, j;

    sprintf(line, "report: frame %u, game_mode %d, stage %d act %d, character %d", port_frame_count,
            port_where.game_mode, port_where.stage, port_where.act, port_where.character);
    port_log(line);
    if (port_where.player) {
        sprintf(line, "report: player 0 at %.2f %.2f %.2f, angles 0x%x 0x%x 0x%x", port_where.player_pos[0],
                port_where.player_pos[1], port_where.player_pos[2], port_where.player_ang[0] & 0xffff,
                port_where.player_ang[1] & 0xffff, port_where.player_ang[2] & 0xffff);
        port_log(line);
    } else {
        port_log("report: no player 0");
    }
    if (port_where.camera) {
        sprintf(line, "report: the camera at %.2f %.2f %.2f, angles 0x%x 0x%x 0x%x", port_where.camera_pos[0],
                port_where.camera_pos[1], port_where.camera_pos[2], port_where.camera_ang[0] & 0xffff,
                port_where.camera_ang[1] & 0xffff, port_where.camera_ang[2] & 0xffff);
        port_log(line);
    }
    if (port_where.player) {
        /* the place as port_place takes it, the floats by their bits, for
         * stage_place_player as the stage and act start: on either host */
        union {
            float f;
            unsigned u;
        } x, y, z;

        x.f = port_where.player_pos[0];
        y.f = port_where.player_pos[1];
        z.f = port_where.player_pos[2];
        sprintf(line,
                "report: to start here, first in HOST_POKE: 1:port_place+%u:%d,1:port_place+%u:%d,"
                "1:port_place+%u:0x%08x,1:port_place+%u:0x%08x,1:port_place+%u:0x%08x,1:port_place+%u:%d,"
                "1:port_place+%u:%d,1:port_place+%u:%d,1:port_place+%u:%d,1:port_place+%u:1,"
                "1:port_place+%u:1",
                (unsigned)offsetof(struct port_place, stage), port_where.stage,
                (unsigned)offsetof(struct port_place, act), port_where.act,
                (unsigned)offsetof(struct port_place, pos[0]), x.u, (unsigned)offsetof(struct port_place, pos[1]), y.u,
                (unsigned)offsetof(struct port_place, pos[2]), z.u, (unsigned)offsetof(struct port_place, ang[0]),
                port_where.player_ang[0], (unsigned)offsetof(struct port_place, ang[1]), port_where.player_ang[1],
                (unsigned)offsetof(struct port_place, ang[2]), port_where.player_ang[2],
                (unsigned)offsetof(struct port_place, character), port_where.character,
                (unsigned)offsetof(struct port_place, enter), (unsigned)offsetof(struct port_place, armed));
        port_log(line);
        sprintf(line, "report: which enters stage %d act %d as character %d from the menus", port_where.stage,
                port_where.act, port_where.character);
        port_log(line);
    }

    /* the phases: the last frame, the last 60 frames' mean and most, the
     * usual, the most since the start */
    sprintf(line, "report: phases in ms over the last %u frames: last, mean, most; usual; most since the start", window);
    port_log(line);
    for (k = 0; k < PHASES && window != 0; k++) {
        unsigned sum = 0, top = 0, last = port_frames[(port_frame_count - 1) % PORT_FRAMES].us[k];

        for (j = 0; j < window; j++) {
            unsigned v = port_frames[(port_frame_count - 1 - j) % PORT_FRAMES].us[k];

            sum += v;
            top = v > top ? v : top;
        }
        sprintf(line, "report:   %-10s %3u.%u %3u.%u %3u.%u; %3u.%u%s; %3u.%u (frame %u)", phase_names[k], MS(last),
                MS(sum / window), MS(top), MS(usual[k]),
                sum / window > usual[k] * 1.25F + 500.0F ? " LONGER THAN USUAL" : "", MS(most[k]), most_frame[k]);
        port_log(line);
    }

    /* the slow frames, newest first, each with the phases that grew most
     * over their usual */
    sprintf(line, "report: %u slow frames since the last report%s", slow_count,
            slow_count > SLOW_KEPT ? ", the newest of them:" : "");
    port_log(line);
    for (k = 0; k < slow_count && k < SLOW_KEPT; k++) {
        const struct port_frame *f = &slow[(slow_count - 1 - k) % SLOW_KEPT].frame;
        const unsigned short *u = slow[(slow_count - 1 - k) % SLOW_KEPT].usual;
        char *at = line;
        int shown = 0;

        at += sprintf(at, "report:   frame %u: %u.%u ms, usually %u.%u; longer:", f->number, MS(f->us[PHASE_FRAME]),
                      MS(u[PHASE_FRAME]));
        /* the three that grew most, but the sums (frame, busy) */
        {
            unsigned taken = 0;

            while (shown < 3) {
                int best = -1;
                int grew = 1000;

                for (j = PHASE_LOGIC; j < PHASES; j++) {
                    int by = (int)f->us[j] - (int)u[j];

                    if (by > grew && !(taken & 1u << j)) {
                        best = (int)j;
                        grew = by;
                    }
                }
                if (best < 0) {
                    break;
                }
                taken |= 1u << best;
                at += sprintf(at, " %s +%u.%u", phase_names[best], MS(grew));
                shown++;
            }
        }
        if (shown == 0) {
            at += sprintf(at, " no one part");
        }
        sprintf(at, "; %u strips, %u vertices, %u models, %u host-transformed", f->strips, f->vertices, f->models,
                f->gpu_vertices);
        port_log(line);
    }
    slow_count = 0;

    sprintf(line, "report: %u frames since the last report took more steps of play than meant (a jump in the "
                  "picture)%s",
            extra_count, extra_count > SLOW_KEPT ? ", the newest of them:" : "");
    port_log(line);
    for (k = 0; k < extra_count && k < SLOW_KEPT; k++) {
        const struct port_frame *f = &extra_frames[(extra_count - 1 - k) % SLOW_KEPT];

        sprintf(line, "report:   frame %u: %u steps of %u meant, %u.%u ms, busy %u.%u, logic %u.%u, render %u.%u",
                f->number, f->steps, f->steps_meant, MS(f->us[PHASE_FRAME]), MS(f->us[PHASE_BUSY]),
                MS(f->us[PHASE_LOGIC]), MS(f->us[PHASE_RENDER]));
        port_log(line);
    }
    extra_count = 0;

    /* the last 30 frames, one a line */
    sprintf(line, "report: the last frames, in ms:");
    port_log(line);
    {
        char *at = line;

        at += sprintf(at, "report:   %7s", "frame");
        for (j = 0; j < PHASES; j++) {
            at += sprintf(at, " %6.6s", phase_names[j]);
        }
        sprintf(at, " %6s %6s %6s %6s %5s", "strips", "verts", "models", "gpuv", "steps");
        port_log(line);
    }
    for (k = have < 30 ? have : 30; k > 0; k--) {
        const struct port_frame *f = &port_frames[(port_frame_count - k) % PORT_FRAMES];
        char *at = line;

        at += sprintf(at, "report:   %7u", f->number);
        for (j = 0; j < PHASES; j++) {
            at += sprintf(at, " %4u.%u", MS(f->us[j]));
        }
        sprintf(at, " %6u %6u %6u %6u %3u/%u", f->strips, f->vertices, f->models, f->gpu_vertices, f->steps,
                f->steps_meant);
        port_log(line);
    }

    port_render_tables();
    {
        int total = 0, largest = 0;
        Uint32 free_bytes = 0, biggest = 0;

        heap_stats(&total, &largest);
        syMallocStat(&free_bytes, &biggest);
        log_texture_memory("at the report");
        sprintf(line, "report: the game's heap: %d KB free of %d, the largest piece %d KB; Shinobi's heap: %u KB free, "
                      "the largest piece %u KB",
                total >> 10, GAME_HEAP_SIZE >> 10, largest >> 10, (unsigned)(free_bytes >> 10), (unsigned)(biggest >> 10));
        port_log(line);
    }
}

