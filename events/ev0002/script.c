/* EV0002, Sonic's story: Chaos's puddle slips away from Sonic. from high above,
 * Eggman says that he has found Chaos at last */
#include <ninja.h>
#include <sg_xpt.h>

#include "chaos/models.h"
#include "chaos/puddle.h"
#include "draw/tree.h"
#include "ev0002/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/eggman.h"
#include "player/textures.h"
#include "shared/chaos0_water_textures.h"
#include "shared/fade.h"
#include "shared/floor.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

#define TEXT_1_LENGTH 20
#define TEXT_2_LENGTH 48
#define TEXT_3_LENGTH 23
#define TEXT_4_LENGTH 2
#include "ev0002/script/text.inc"

/* the puddle task's work->mode: what its step does. a move or a vanish
 * runs until the timer passes record->steps. then a move wobbles, and a
 * vanish stops */
enum puddle_mode {
    PUDDLE_WOBBLE, /* swing the scale between 7/8 and 9/8 every 60 steps */
    PUDDLE_MOVE, /* add record->speed to pos, swinging the y scale 1 to 2 */
    PUDDLE_VANISH, /* wobble every 20 steps, shrinking and fading */
    PUDDLE_GONE /* return at once, without drawing the puddle */
};

/* what the puddle task keeps besides its work. ev0002_puddle_create
 * allocates it, unchecked, and the task's destructor frees it */
struct puddle_record {
    Uint8 unk_00[4];
    NJS_VECTOR speed; /* how far a move goes each step */
    /* the steps a move divides its distance by, or a vanish its scale and
     * alpha */
    Sint32 steps;
    Float scale; /* the vanish's factor on the wobble's scale, from 1 down */
    Float scale_step; /* how much the vanish takes from scale each step */
    Uint8 alpha; /* the alpha the draw gives chaos_puddle's material */
    Uint8 alpha_step; /* how much the vanish takes from alpha each step */
};

/* the puddle task's work->vars */
struct puddle_vars {
    Float phase; /* the phase of the scale's swing, in degrees */
    Sint32 timer; /* the steps PUDDLE_MOVE or PUDDLE_VANISH has run */
    struct puddle_record *record;
};

#define VARS(work) ((struct puddle_vars *)&(work)->vars)
#define PHASE_ANG(work) ((Angle)(VARS(work)->phase * ANGLE_PER_DEGREE))

extern NJS_ACTION sonic_jump_land_action;
extern NJS_ACTION sonic_run_action;
extern NJS_ACTION sonic_walk_action;
extern NJS_ACTION dat_8c59d224; /* an action that moves eggman_object */

extern NJS_ACTION ev0002_sonic_action;

static struct task *ev0002_fade = NULL;
static struct task *ev0002_floor = NULL;

struct texture_pvm ev0002_textures[] = {
    {"EV_CHAOS0_MANJU", &event_chaos0_water_texlist},
    {"CHAOS_SURFACE", &chaos_surface_texlist},
    {NULL, NULL},
};

static struct task *ev0002_puddle = NULL;

static void ev0002_puddle_init(struct task *task);

/* start the puddle task in ev0002_puddle, wobbling at x, y, z and facing
 * ang_y. the task and its record are allocated unchecked */
static void ev0002_puddle_create(Float x, Float y, Float z, Angle ang_y)
{
    struct puddle_record *record;
    struct task_work *work;

    ev0002_puddle = task_create(TASK_WORK, 5, ev0002_puddle_init);
    record = heap_alloc(sizeof(struct puddle_record));
    work = ev0002_puddle->work;
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
    record->alpha = 0x99;
}

/* kill the puddle task, which destroys itself on its next step, and clear
 * its slot. without a puddle task it does nothing */
static void ev0002_puddle_kill(void)
{
    struct task *task = ev0002_puddle;
    void *work;

    if (task != NULL) {
        work = task->work;
        /* @hack redundant: task tested again, though the test above makes this
         * one always pass. SHC then tests R4 a second time (TST R4,R4 at
         * 0x0cb80082). without it, ev0002_puddle_kill runs 6 bytes short, and
         * 13 of its 17 words differ */
        if (task != NULL) {
            /* @bug mismatched-call: task_kill takes only the task, but the call
             * passes the task's work too, in R5, which task_kill never
             * reads (MOV.L @(32,R4),R5 at 0x0cb80086, before the JSR at
             * 0x0cb8008a) */
            /* NOLINTNEXTLINE(clang-diagnostic-cast-function-type-mismatch) */
            ((void (*)(struct task *, void *))task_kill)(task, work);
            ev0002_puddle = NULL;
        }
    }
}

/* move the puddle a 1/steps share of the way to x, y, z each step, then
 * wobble. the move runs steps + 1 steps, so it stops one share past x, y,
 * z. steps must be positive, and the puddle task must exist */
static void ev0002_puddle_move(Float x, Float y, Float z, int steps)
{
    struct task_work *work = ev0002_puddle->work;
    struct puddle_record *record = VARS(work)->record;

    record->speed.x = (x - work->pos.x) / (Float)steps;
    record->speed.y = (y - work->pos.y) / (Float)steps;
    record->speed.z = (z - work->pos.z) / (Float)steps;
    record->steps = steps;
    VARS(work)->timer = 0;
    work->scl.x = work->scl.y = work->scl.z = 1.0F;
    work->mode = PUDDLE_MOVE;
}

/* shrink and fade the puddle, then stop. each step takes a 1/steps share from
 * the scale, and alpha / steps, rounded down, from the alpha. the vanish runs
 * steps + 1 steps, so the scale ends at -1/steps. steps must be positive, and
 * the puddle task must exist */
static void ev0002_puddle_vanish(int steps)
{
    struct task_work *work = ev0002_puddle->work;
    struct puddle_record *record = VARS(work)->record;

    record->scale = 1.0F;
    record->scale_step = 1.0F / (Float)steps;
    record->steps = steps;
    record->alpha_step = record->alpha / steps;
    VARS(work)->timer = 0;
    work->mode = PUDDLE_VANISH;
}

static void ev0002_puddle_step(struct task *task);
static void ev0002_puddle_draw(struct task *task);
static void ev0002_puddle_destroy(struct task *task);

static void ev0002_puddle_init(struct task *task)
{
    task->step = ev0002_puddle_step;
    task->draw = ev0002_puddle_draw;
    task->destructor = ev0002_puddle_destroy;
}

static void ev0002_puddle_step(struct task *task)
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
    ev0002_puddle_draw(task);
}

/* draw chaos_puddle at the puddle task's place and scale, with the
 * record's alpha. the draw reads the puddle task through ev0002_puddle, not
 * through task */
static void ev0002_puddle_draw(struct task *task)
{
    struct task_work *work = ev0002_puddle->work;
    struct puddle_record *record = VARS(work)->record;

    /* @hack spelling: chaos_puddle read through its address. SHC loads that
     * address from the pool word that draw_tree's argument comes from
     * (0x0cb8031e, 0x0cb8035c), and reads model at offset 4 (MOV.L @(4,R2),R1
     * at 0x0cb80326). as chaos_puddle.model, a pool word of its own holds
     * model's address, the code runs 4 bytes long, and 12 of
     * ev0002_puddle_draw's 41 words differ */
    (&chaos_puddle)->model->mats->diffuse.argb.a = record->alpha;
    njSetTexture(&chaos_surface_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njScaleV(NULL, &work->scl);
    draw_tree(&chaos_puddle);
    njPopMatrix(1);
}

static void ev0002_puddle_destroy(struct task *task)
{
    if (VARS(task->work)->record != NULL) {
        heap_free(VARS(task->work)->record);
    }
}

void ev0002_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        sound_load_file("e_0001.mlt");
        event_sound_init(5);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x2e39);
        event_create_player(2, eggman_step, 380.0F, 0.0F, 353.0F, 0, 0x61e0, 0);
        event_set_pos(event_get_player(0), 266.25F, 3.0F, 263.0F);
        event_set_ang(event_get_player(0), 0, 0xf999, 0);
        event_reset_player(0);
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1b00, 0x534d, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 275.8F, 3.43F, 262.6F);
        event_wait(30);
        ev0002_puddle_create(271.09F, 2.2F, 306.28F, 0);
        event_wait(1);
        event_camera_set_fov(EVENT_EASE_LINEAR, 90, 0x31c7);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x0d00, 0x1251, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 276.22F, 3.97F, 281.2F);
        event_wait(93);
        event_camera_set_ang(EVENT_EASE_LINEAR, 116, 0xfc00, 0x1599, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 116, 280.0F, 7.17F, 317.77F);
        event_set_pos(event_get_player(2), 100.66F, 122.8F, 421.42F);
        event_set_ang(event_get_player(2), 0, 0x4d00, 0);
        event_wait(1);
        ev0002_floor = event_floor_create(event_get_player(2), 122.8F);
        event_wait(125);
        event_wait(20);
        /* the move's extra step carries the puddle a hundredth of its
         * 97.6-unit way, under a unit, past 217.2, 0, 387.6. it vanishes
         * there, and only its draw uses that place, so the step is
         * harmless */
        ev0002_puddle_move(217.2F, 0.0F, 387.6F, 100);
        event_sound_start(0, 1235, 1800);
        event_sound_fade_volume(0, -30, 1);
        event_wait(10);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe700, 0x138e, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 279.15F, 34.2F, 337.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xe700, 0x0400, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 225.1F, 28.4F, 417.18F);
        event_play_action(event_get_player(0), &ev0002_sonic_action,
                          &sonic_texlist, 0.9F, 0, 4);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_4);
        event_voice_play(943);
        event_message_show(TEXT_1);
        event_wait_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_run_action,
                          &sonic_texlist, 1.7F, EVENT_REPEAT, 1);
        event_run_to_speed(event_get_player(0), 233.0F, 0.0F, 359.6F, 1.1F,
                           0.0F);
        event_wait(50);
        event_message_close();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0xde00, 0x0b00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 219.22F, 7.2F, 394.8F);
        event_wait(60);
        /* the alpha falls 3 a step from 0x99, and reaches 0 on the vanish's
         * 51st step, its extra one. that step also turns the scale to -1/50,
         * which alpha 0 leaves unseen */
        ev0002_puddle_vanish(50);
        event_sound_fade_volume(0, -100, 60);
        event_wait(54);
        event_wait(11);
        event_clear_motions(event_get_player(0));
        event_run_to_speed(event_get_player(0), 223.24F, 0.0F, 381.45F, 0.9F,
                           0.0F);
        event_play_action(event_get_player(0), &sonic_walk_action,
                          &sonic_texlist, 0.9F, EVENT_REPEAT, 1);
        event_wait(62);
        event_clear_motions(event_get_player(0));
        event_turn_by(event_get_player(0), 0, 0xeb00, 0);
        event_talk_stop(event_get_player(0));
        event_look_at(event_get_player(0), 216.0F, -8.9F, 390.5F);
        event_wait(40);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x293f);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x01eb, 0xf505, 0x0150);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 218.7F, 0.65F, 390.87F);
        event_wait(21);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0x1500, 0xf505, 0x0150);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, 219.77F, 3.6F, 389.59F);
        event_wait(120);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 85, 214.35F, 20.2F, 398.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 85, 0xd704, 0xf705, 0x0650);
        event_wait(81);
        event_camera_set_fov(EVENT_EASE_LINEAR, 140, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, 103.5F, 135.6F, 426.62F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 150, 0xd707, 0xd805, 0x064f);
        event_wait(149);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 99.92F, 139.3F, 427.535F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0xd704, 0xd705, 0x0650);
        event_wait(30);
        event_voice_play(945);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &dat_8c59d224, &eggman_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1407, 0x4705, 0x0500);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 117.0F, 128.11F, 421.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 104.9677F, 135.535F,
                             422.3887F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x1207, 0x3b05, 0xfe00);
        event_voice_play(946);
        event_message_show(TEXT_3);
        event_wait(35);
        ev0002_fade = event_fade_create(1.0F / 60.0F, 0.1F, 0.0F, 0.0F, 0.0F);
        event_wait(40);
        event_message_close();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        EVENT_KILL(ev0002_floor);
        event_reset_player(0);
        event_kill_player(2);
        EVENT_KILL(ev0002_fade);
        event_sound_stop_all();
        ev0002_puddle_kill();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
