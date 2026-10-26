/* EV00F2: Eggman curses Sonic from the Egg Mobile, then sees Chaos's puddle */
#include <ninja.h>
#include <sg_xpt.h>

#include "chaos/models.h"
#include "chaos/puddle.h"
#include "ev00f2/script.h"
#include "ev00f2/water_textures.h"
#include "event/play.h"
#include "shared/eggmobile.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

#define TEXT_1_LENGTH 25
#define TEXT_2_LENGTH 51
#define TEXT_3_LENGTH 35
#define TEXT_4_LENGTH 9
#include "ev00f2/script/text.inc"

/* the puddle's modes, in work->mode */
enum puddle_mode {
    PUDDLE_WOBBLE,
    /* the puddle moves by record->speed for record->steps + 1 steps, then
     * wobbles again. its y scale swings between 1 and 2 as it moves */
    PUDDLE_MOVE,
    /* the puddle shrinks and fades for record->steps + 1 steps */
    PUDDLE_VANISH,
    /* after a vanish the step changes nothing, so the puddle keeps its last
     * scale and alpha */
    PUDDLE_GONE
};

/* the puddle's heap record, which ev00f2_puddle_destroy frees */
struct puddle_record {
    struct task *drawn; /* the task puddle_create returned, which draws it */
    NJS_VECTOR speed; /* distance a move covers each step */
    Sint32 steps; /* steps a move or a vanish divides by */
    Float scale; /* scale factor during a vanish, from 1 down */
    Float scale_step; /* scale lost each step of a vanish */
    Uint8 alpha; /* the alpha the draw gives chaos_puddle's material */
    Uint8 alpha_step; /* alpha lost each step of a vanish */
};

/* the puddle's view of work->vars */
struct puddle_vars {
    Float phase; /* wobble phase, in degrees */
    Sint32 timer; /* steps the current move or vanish has run */
    struct puddle_record *record;
};

#define VARS(work) ((struct puddle_vars *)&(work)->vars)
#define PHASE_ANG(work) ((Angle)(VARS(work)->phase * ANGLE_PER_DEGREE))

extern NJS_TEXLIST event_eggmobile_texlist;

static struct task *ev00f2_puddle = NULL;
static struct task *ev00f2_fade = NULL;

struct texture_pvm ev00f2_textures[] = {
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"EV_CHAOS0_MANJU", &event_chaos0_water_texlist},
    {"CHAOS_SURFACE", &chaos_surface_texlist},
    {NULL, NULL},
};

static void ev00f2_puddle_init(struct task *task);

/* make the wobbling puddle at x, y, z facing ang_y, and hold its task in
 * ev00f2_puddle. its record keeps the task puddle_create makes to draw it.
 * neither allocation is checked */
static void ev00f2_puddle_create(Float x, Float y, Float z, Angle ang_y)
{
    struct puddle_record *record;
    struct task_work *work;

    ev00f2_puddle = task_create(TASK_WORK, 5, ev00f2_puddle_init);
    record = heap_alloc(sizeof(struct puddle_record));
    work = ev00f2_puddle->work;
    VARS(work)->record = record;
    work->mode = PUDDLE_WOBBLE;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    work->ang.y = ang_y;
    work->scl.x = 0.8F;
    work->scl.y = 0.8F;
    work->scl.z = 0.8F;
    VARS(work)->phase = 0.0F;
    record->drawn = puddle_create(work->ang.y);
    record->alpha = 0x99;
}

/* destroy the task that draws the puddle at once, and kill the puddle,
 * which destroys itself on its next step. with no puddle, do nothing */
static void ev00f2_puddle_kill(void)
{
    struct task_work *work;
    struct puddle_record *record;

    if (ev00f2_puddle != NULL) {
        work = ev00f2_puddle->work;
        record = VARS(work)->record;
        if (record->drawn != NULL) {
            task_destroy(record->drawn);
            record->drawn = NULL;
        }
        /* the image reloads the slot after task_destroy and tests it again
         * (0x0cb800aa, 0x0cb800ac) */
        if (ev00f2_puddle != NULL) {
            task_kill(ev00f2_puddle);
            ev00f2_puddle = NULL;
        }
    }
}

/* not static: the image keeps it at 0x0cb800c2, though nothing calls it. SHC
 * drops a static function nothing in its file calls */
void ev00f2_puddle_move(Float x, Float y, Float z, int steps)
{
    struct task_work *work = ev00f2_puddle->work;
    struct puddle_record *record = VARS(work)->record;

    record->speed.x = (x - work->pos.x) / (Float)steps;
    record->speed.y = (y - work->pos.y) / (Float)steps;
    record->speed.z = (z - work->pos.z) / (Float)steps;
    record->steps = steps;
    VARS(work)->timer = 0;
    work->scl.x = work->scl.y = work->scl.z = 1.0F;
    work->mode = PUDDLE_MOVE;
}

/* not static: the image keeps it at 0x0cb8010e, though nothing calls it. SHC
 * drops a static function nothing in its file calls */
void ev00f2_puddle_vanish(int steps)
{
    struct task_work *work = ev00f2_puddle->work;
    struct puddle_record *record = VARS(work)->record;

    record->scale = 1.0F;
    record->scale_step = 1.0F / (Float)steps;
    record->steps = steps;
    record->alpha_step = record->alpha / steps;
    VARS(work)->timer = 0;
    work->mode = PUDDLE_VANISH;
}

static void ev00f2_puddle_step(struct task *task);
static void ev00f2_puddle_draw(struct task *task);
static void ev00f2_puddle_destroy(struct task *task);

static void ev00f2_puddle_init(struct task *task)
{
    task->step = ev00f2_puddle_step;
    task->draw = ev00f2_puddle_draw;
    task->destructor = ev00f2_puddle_destroy;
}

static void ev00f2_puddle_step(struct task *task)
{
    struct task_work *work = task->work;
    struct puddle_record *record = VARS(work)->record;

    switch (work->mode) {
    case PUDDLE_WOBBLE:
        VARS(work)->phase += 6.0F;
        work->scl.x = 1.0F + (0.125F * njCos(PHASE_ANG(work)));
        work->scl.y = 1.0F + (0.125F * njCos(PHASE_ANG(work)));
        work->scl.z = 1.0F + (0.125F * njSin(PHASE_ANG(work)));
        break;
    case PUDDLE_MOVE:
        work->pos.x += record->speed.x;
        work->pos.y += record->speed.y;
        work->pos.z += record->speed.z;
        if (++VARS(work)->timer > record->steps) {
            work->mode = PUDDLE_WOBBLE;
        }
        VARS(work)->phase += 6.0F;
        work->scl.y = 1.5F + (0.5F * njCos(PHASE_ANG(work)));
        break;
    case PUDDLE_VANISH:
        VARS(work)->phase += 18.0F;
        record->scale -= record->scale_step;
        if (++VARS(work)->timer > record->steps) {
            work->mode = PUDDLE_GONE;
        }
        work->scl.x = 1.0F + (0.125F * njCos(PHASE_ANG(work)));
        work->scl.y = 1.0F + (0.125F * njCos(PHASE_ANG(work)));
        work->scl.z = 1.0F + (0.125F * njSin(PHASE_ANG(work)));
        work->scl.x *= record->scale;
        work->scl.y *= record->scale;
        work->scl.z *= record->scale;
        record->alpha -= record->alpha_step;
        break;
    case PUDDLE_GONE:
        return;
    }
    ev00f2_puddle_draw(task);
}

/* give chaos_puddle's material the puddle's alpha, and pass the puddle's
 * position and scale to the task puddle_create made. it reads the puddle
 * through ev00f2_puddle and ignores task */
static void ev00f2_puddle_draw(struct task *task)
{
    struct task_work *work = ev00f2_puddle->work;
    struct puddle_record *record = VARS(work)->record;

    /* @hack spelling: the model as (&chaos_puddle)->model. SHC then pools
     * chaos_puddle's address (0x0cb803ec) and reads the model 4 bytes past
     * it (MOV.L @(4,R2),R1 at 0x0cb80354). as chaos_puddle.model, the pool
     * word holds the address of chaos_puddle's model field, and 9 of 21
     * words differ */
    (&chaos_puddle)->model->mats->diffuse.argb.a = record->alpha;
    puddle_move(&work->pos, &work->scl);
}

static void ev00f2_puddle_destroy(struct task *task)
{
    if (VARS(task->work)->record != NULL) {
        heap_free(VARS(task->work)->record);
    }
}

void ev00f2_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        sound_load_file("e_00F3.mlt");
        event_sound_init(3);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_eggmobile_create(-923.41F, 20.22F, -157.9F, 0xfbcf, 0xe89e,
                               0xfcbb);
        event_wait(2);
        event_eggmobile_set_jets(EGGMOBILE_JETS_ALL);
        ev00f2_puddle_create(-1130.89F, 34.36F, -250.85F, 0);
        event_wait(2);
        stream_request_music(MUSIC_EGGMAN);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -970.2F, 50.55F, -179.28F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe600, 0xb000, 0);
        event_wait(2);
        event_sound_start(0, 1226, 1800);
        event_eggmobile_slide_to(-1025.64F, 18.34F, -223.17F, 210);
        event_wait(3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, -1061.43F, 48.33F,
                             -238.97F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0xe500, 0xb000, 0);
        event_wait(100);
        event_voice_play(1888);
        event_message_show(TEXT_1);
        event_wait(40);
        event_message_clear();
        event_wait(52);
        event_eggmobile_slide_to(-923.41F, 20.22F, -157.9F, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -927.35F, 37.377F,
                             -160.703F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe500, 0xb000, 0);
        event_eggmobile_slide_to(-1025.64F, 20.34F, -223.17F, 300);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 300, -1031.082F, 37.99F,
                             -227.324F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 300, 0xf100, 0xad00, 0);
        event_wait(40);
        event_voice_play(1889);
        event_message_show(TEXT_2);
        event_wait(80);
        event_message_clear();
        event_wait(20);
        event_voice_play(1890);
        event_message_show(TEXT_3);
        event_voice_wait();
        event_message_clear();
        event_wait(4);
        event_sound_start(0, 1225, 1800);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -1032.18F, 38.3F, -215.87F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe500, 0x3200, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, -1102.2F, 47.47F,
                             -240.93F);
        event_wait(195);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_sound_stop(0);
        event_sound_stop(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1037.21F, 38.02F, -228.94F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xee00, 0xaf00, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 30, 0x11c7);
        event_voice_play(1891);
        event_message_show(TEXT_4);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        ev00f2_fade = event_fade_create(1.0F / 60.0F, 0.0F, 0.0F, 0.0F, 0.0F);
        event_wait(90);
        event_sound_play(1224, 0, 0, 0);
        if (ev00f2_fade != NULL) {
            event_fade_set_speed(ev00f2_fade, -1.0F);
        }
        event_wait(1);
        if (ev00f2_fade != NULL) {
            event_fade_set_speed(ev00f2_fade, 1.0F);
        }
        event_wait(60);
        event_sound_play(1223, 0, 0, 0);
        event_wait(10);
        EVENT_KILL(ev00f2_fade);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_eggmobile_set_jets(EGGMOBILE_JETS_OFF);
        event_reset_player(0);
        EVENT_KILL(ev00f2_fade);
        event_eggmobile_kill();
        ev00f2_puddle_kill();
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
