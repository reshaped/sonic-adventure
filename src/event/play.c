/* playing events: an event's module, the thread that plays it, and the
 * calls its script makes */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <sofdec.h>
#include <stdarg.h>
#include <string.h>

#include "adv02/mystic_ruins.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "effect/particle.h"
#include "event/event.h"
#include "event/play.h"
#include "event/script.h"
#include "hud/hud.h"
#include "object/common.h"
#include "player/player.h"
#include "progress/progress.h"
#include "progress/recorder.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/bitmap.h"
#include "system/frame.h"
#include "system/loader.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/thread.h"
#include "system/timer.h"
#include "system/util.h"

/* where an event's module loads, and the bytes event_load zeroes there
 * before a read */
#define EVENT_MODULE ((struct event_module *)0x0cb80000)
#define EVENT_MODULE_SIZE 0x48000

/* the event camera's first field of view, and the game's after an event */
#define FOV NJM_DEG_ANG(70)

/* the work->mode of the tasks that count steps around an event */
enum count_mode {
    COUNT_START, /* the first step, which zeroes work->timer */
    COUNT_STEPS /* each later step, which adds one to it */
};

/* the steps the tasks count after their first */
#define END_DRAW_STEPS 7
#define ENDING_STEPS 8
#define MUSIC_STEPS 8
#define START_STEPS 16

/* a loaded event: its entry, the thread that plays it or null, and its
 * number */
struct event {
    void (*entry)(int phase);
    struct thread *thread;
    int number;
};

/* the data of an event's thread: its event, and the window its messages
 * show in */
struct event_thread {
    struct event *event;
    struct bitmap window;
};

/* the data of a pad's thread: the port it plays recording into */
struct pad_thread {
    Uint8 port;
    const struct event_pad_recording *recording;
};

static void event_thread_destroy(void);
static void event_end_step(struct task *task);
static void event_music_step(struct task *task);
static void event_start_step(struct task *task);
static void event_play(void (*entry)(int phase));
static void event_skip_to_end(void (*entry)(int phase));
static void event_camera_step_pos(void);
static void event_camera_step_ang(void);
static void event_camera_step_fov(void);

struct event_camera event_camera;

/* the module that's loaded, or null */
static struct event_module *event_module;
/* 1 while a module is loaded. nothing reads it */
static Sint8 module_loaded;
/* event_camera_step holds the camera still while it is set, which it
 * never is: only event_camera_take writes it, with 0 */
static Sint8 event_camera_hold;
/* event_init zeroes it, and nothing else writes it. only event_wait's
 * empty test reads it */
static Sint8 dat_8c751dfe;
static struct event loaded_event;
/* the event that has taken control, or null */
static struct event *event_controlling;

/* the number of the event that plays, or -1 */
static int event_current = -1;

/* the events under which a field doesn't start its music. -1 ends the
 * list, though event_check_music reads past it */
static int music_events[] = {
    0x0001, 0x000b, 0x0011, 0x0014, 0x0015, 0x0023, 0x002a, 0x0030, 0x0039,
    0x0050, 0x0051, 0x0056, 0x0040, 0x0080, 0x0086, 0x008b, 0x008e, 0x0098,
    0x009f, 0x0060, 0x006c, 0x00b0, 0x00b9, 0x00d0, 0x00db, 0x00dd, 0x0176,
    0x0177, 0x0178, 0x0179, 0x017a, 0x017b, 0x017c, 0x0008, 0x0012, 0x0034,
    0x0067, 0x0026, 0x003e, 0x0052, 0x00e2, 0x00f2, 0x00f3, -1,
};

/* after one of these, the field's music doesn't start again */
static int music_kept_events[] = {
    0x001a, 0x004b, 0x00c1, 0x0028, 0x0038, 0x008a, 0x0166, -1,
};

void event_init(void)
{
    memset(events_played, 0, sizeof(events_played));
    event_module = NULL;
    event_controlling = NULL;
    module_loaded = 0;
    messages_shown = 1;
    dat_8c751dfe = 0;
    event_current = -1;
}

void event_toggle_messages(void)
{
    if (event_controlling != NULL && (per_info[0]->press & PDD_DGT_TY)) {
        messages_shown = !messages_shown;
    }
}

void event_pads_on(void)
{
    pad_enable();
}

void event_pads_off(void)
{
    pad_disable();
}

void event_take_control(void)
{
    struct event_thread *thread = thread_get_data(NULL);

    if (event_controlling == NULL) {
        event_controlling = thread->event;
    }
}

void event_release_control(void)
{
    struct event_thread *thread = thread_get_data(NULL);

    if (event_controlling == thread->event) {
        event_controlling = NULL;
    }
}

/* an event thread's destructor: release its window, and take control and
 * the camera back from its event if it has them */
static void event_thread_destroy(void)
{
    struct event_thread *thread = thread_get_data(NULL);
    struct event *event = thread->event;

    bitmap_release(&thread->window);
    if (event_controlling == event) {
        event_controlling = NULL;
    }
    if (event_camera.owner == event) {
        event_camera_release();
    }
    event->thread = NULL;
}

/* as an event starts, stop the streams and restart their task. each frame
 * of two vsyncs runs one step, not two, and the field's people leave */
static void event_enter(void)
{
    stream_destroy();
    stream_reset();
    frame_pacing_init(2, 1);
    field_people_remove();
}

/* turn the draw back on END_DRAW_STEPS steps after an event's end, and
 * clear event_ending after ENDING_STEPS */
static void event_end_step(struct task *task)
{
    struct task_work *work = task->work;

    if (work->mode == COUNT_START) {
        work->timer = 0;
        work->mode = COUNT_STEPS;
    } else {
        work->timer++;
        if (TASK_TIMER(work) == END_DRAW_STEPS) {
            draw_off = FALSE;
        } else if (TASK_TIMER(work) == ENDING_STEPS) {
            event_ending = 0;
            task_destroy(task);
        }
    }
}

/* MUSIC_STEPS steps after an event's end, start the field's music again,
 * if no event plays and the act number is as it was */
static void event_music_step(struct task *task)
{
    struct task_work *work = task->work;
    int stage_act;
    int stage;
    int act;

    if (work->mode == COUNT_START) {
        work->timer = 0;
        work->mode = COUNT_STEPS;
        /* submode keeps the low byte, the act number */
        work->submode = stage_get_stage_act();
    } else {
        work->timer++;
        if (TASK_TIMER(work) == MUSIC_STEPS) {
            if (event_current < 0 &&
                /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
                work->submode == (Uint8)stage_get_stage_act()) {
                stage_act = stage_get_stage_act();
                act = (Uint8)stage_act;
                stage = stage_act >> 8;
                switch (stage) {
                case STAGE_STATION_SQUARE:
                    stream_reset();
                    stream_request_music(MUSIC_S_SQUARE);
                    break;
                case STAGE_EGG_CARRIER:
                case STAGE_EGG_CARRIER_2:
                case STAGE_EGG_CARRIER_3:
                case STAGE_EGG_CARRIER_INSIDE:
                    stream_reset();
                    stream_request_music(MUSIC_EGCARER1);
                    break;
                case STAGE_MYSTIC_RUINS:
                    switch (act) {
                    case MYSTIC_RUINS_STATION:
                    case MYSTIC_RUINS_ISLAND:
                    case MYSTIC_RUINS_JUNGLE:
                        stream_reset();
                        stream_request_music(MUSIC_MSTCLN);
                        break;
                    case MYSTIC_RUINS_BASE:
                        stream_reset();
                        stream_request_music(MUSIC_FINALEG1);
                        break;
                    }
                    break;
                case STAGE_PAST:
                    break;
                }
            }
            task_destroy(task);
        }
    }
}

/* keep the draw off for an event's first START_STEPS steps */
static void event_start_step(struct task *task)
{
    struct task_work *work = task->work;

    if (work->mode == COUNT_START) {
        work->timer = 0;
        work->mode = COUNT_STEPS;
    } else {
        work->timer++;
        if (TASK_TIMER(work) == START_STEPS) {
            draw_off = FALSE;
            task_destroy(task);
            return;
        }
        draw_off = TRUE;
    }
}

Bool event_check_music(void)
{
    Bool play = TRUE;
    int *event;

    if (event_current < 0) {
        return TRUE;
    }
    /* @bug memory: play stays TRUE until a match, so for an event the
     * list doesn't hold, the walk passes the -1 that ends it (TST R5,R5 and
     * BF at 0x8c04c3ac) and reads on past the list (0x8c04c39a) until a word
     * equals event_current */
#ifdef GCC
    /* repairs the walk the tag names: it stops at the -1 that ends the list,
     * so under an event the list lacks, the field's music starts */
    for (event = music_events; *event >= 0; event++) {
        if (event_current == *event) {
            play = FALSE;
            break;
        }
    }
#else
    for (event = music_events; *event >= 0 || play; event++) {
        if (event_current == *event) {
            play = FALSE;
            break;
        }
    }
#endif
    return play;
}

/* as an event ends, stop the streams, load the character's voices and
 * bring the field's people back. each frame runs two steps again.
 * event_end_step brings the draw back, and event_music_step the field's
 * music, unless the event is one of music_kept_events */
static void event_leave(void)
{
    int kept;
    int *event;

    stream_destroy();
    frame_pacing_init(2, 2);
    sound_load_voice();
    field_people_place();
    task_create(TASK_WORK, 0, event_end_step);
    event_ending = -1;
    kept = 0;
    for (event = music_kept_events; *event >= 0 || kept; event++) {
        if (event_current == *event) {
            kept = -1;
            break;
        }
    }
    if (!kept) {
        task_create(TASK_WORK, 0, event_music_step);
    }
    event_current = -1;
}

static void event_load_textures(struct texture_pvm *pvm)
{
    for (; pvm->texlist != NULL; pvm++) {
        if (pvm->name != NULL) {
            texture_load_pvm(pvm->name, pvm->texlist);
        } else {
            njLoadTexture(pvm->texlist);
        }
    }
}

/* the phase event's thread has reached, or EVENT_NONE while it has none.
 * a new thread reads EVENT_LOAD until it first runs */
static enum event_phase event_status(struct event *event)
{
    if (event->thread != NULL) {
        return thread_get_status(event->thread);
    }
    return EVENT_NONE;
}

/* an event's thread: play the event and end it, then release every
 * particle, the window and the module's textures, and stop the event's
 * sounds. event_leave then puts the game back. Tails leaves for the event
 * and returns after */
static void event_play(void (*entry)(int phase))
{
    struct event_thread *thread = thread_get_data(NULL);
    struct texture_pvm *pvm;
    char *id;
    int i;

    npc_leave();
    thread_set_status(NULL, EVENT_PLAY);
    entry(EVENT_PLAY);
    draw_off = TRUE;
    thread_set_status(NULL, EVENT_END);
    entry(EVENT_END);
    particle_release_all();
    event_message_close();
    for (pvm = event_module->textures; pvm->texlist != NULL; pvm++) {
        njReleaseTexture(pvm->texlist);
    }
    id = thread_get_data(NULL);
    for (i = EVENT_SOUND_PLAY_IDS; i != 0; i--) {
        sound_stop_id((int)id++);
    }
    thread_yield();
    event_leave();
    thread_yield();
    njSetPerspective(FOV);
    npc_return();
    if (event_controlling == thread->event) {
        event_controlling = NULL;
    }
}

/* event_play from EVENT_END on, where event_skip restarts the thread of a
 * playing event */
static void event_skip_to_end(void (*entry)(int phase))
{
    struct texture_pvm *pvm;
    char *id;
    int i;

    draw_off = TRUE;
    thread_set_status(NULL, EVENT_END);
    entry(EVENT_END);
    particle_release_all();
    event_message_close();
    for (pvm = event_module->textures; pvm->texlist != NULL; pvm++) {
        njReleaseTexture(pvm->texlist);
    }
    id = thread_get_data(NULL);
    for (i = EVENT_SOUND_PLAY_IDS; i != 0; i--) {
        sound_stop_id((int)id++);
    }
    thread_yield();
    event_leave();
    thread_yield();
    njSetPerspective(FOV);
    npc_return();
}

/* start the event's thread over at run(entry), or create one there, whose
 * data is a zeroed struct event_thread. a failed creation leaves the event
 * without a thread */
static void event_start_thread(struct event *event,
                               void (*run)(void (*entry)(int phase)))
{
    struct event_thread *thread;

    if (event->thread != NULL) {
        thread_start(event->thread, (void *)run, (void *)event->entry);
        return;
    }
    if ((event->thread = thread_create((void *)run, (void *)event->entry, 0)) !=
        NULL) {
        thread = thread_get_data(event->thread);
        memset(thread, 0, sizeof(*thread));
        thread->event = event;
        thread_set_destructor(event->thread, event_thread_destroy);
    }
}

void event_load(int number)
{
    char name[32];
    int *word;
    int i;

    /* @hack spelling: a goto into the if, which for a number below 0 takes the
     * module already in memory, as the image's branch into the if's body
     * does (BF/S at 0x8c04c750); as an if around the read, with a return
     * where the read fails, 41 of 65 words differ */
    if (number < 0) {
        goto loaded;
    }
    word = (int *)EVENT_MODULE;
    for (i = EVENT_MODULE_SIZE / 4; i != 0; i--) {
        *word++ = 0;
    }
    string_format(name, "ev%04X.prs", number);
    stream_destroy();
    njWaitVSync();
    if (loader_read_prs(name, EVENT_MODULE) != -1) {
    loaded:
        event_module = EVENT_MODULE;
        module_loaded = 1;
        loaded_event.thread = NULL;
        loaded_event.entry = event_module->entry;
        loaded_event.number = number;
        syCachePurgeAll();
        event_load_textures(event_module->textures);
        loaded_event.entry(EVENT_LOAD);
    }
    stream_reset();
}

void event_unload(void)
{
    module_loaded = 0;
    if (event_module != NULL) {
        if (loaded_event.thread != NULL) {
            thread_destroy(loaded_event.thread);
            loaded_event.thread = NULL;
        }
        if (loaded_event.entry != NULL) {
            loaded_event.entry(EVENT_UNLOAD);
            loaded_event.entry = NULL;
        }
        event_module = NULL;
    }
}

void event_start(void)
{
    if (event_status(&loaded_event) == EVENT_NONE) {
        draw_off = TRUE;
        task_create(TASK_WORK, 0, event_start_step);
        event_start_thread(&loaded_event, event_play);
        events_played[loaded_event.number] = 1;
        event_enter();
        event_current = loaded_event.number;
        message_cut();
    } else {
        event_leave();
    }
    recording_on = FALSE;
}

void event_skip(void)
{
    if (event_controlling == NULL) {
        return;
    }
    if (event_status(event_controlling) == EVENT_PLAY) {
        event_start_thread(event_controlling, event_skip_to_end);
    }
}

Bool event_is_playing(int unused)
{
    if (event_status(&loaded_event) != EVENT_NONE) {
        return TRUE;
    }
    return FALSE;
}

Bool event_in_control(void)
{
    return event_controlling != NULL;
}

Bool event_was_played(int number)
{
    return events_played[number];
}

void event_wait(int steps)
{
    /* @hack type: the thread's bytes through a void *, which keeps them in
     * R13 (MOV R0,R13 at 0x8c04c946); typed, they take R12 and 4 of 35 words
     * differ */
    void *data = thread_get_data(NULL);
    struct event_thread *thread = data;
    int i;

    for (i = steps; i != 0; i--) {
#ifdef GCC
        /* a skip's wait lasts this step: skip_wait stands each runner at
         * its target, leaves each mover owing the rest, and returns 1 */
        if (event_skipping) {
            i = skip_wait(i);
        }
#endif
        bitmap_draw(&thread->window);
        thread_yield();
        /* @hack redundant: an empty test of dat_8c751dfe, which nothing comes
         * of and for which the loop saves R10 (MOV.L R10,@-R15 at 0x8c04c936);
         * without it, the function runs 10 bytes short and 31 of 35 words
         * differ */
        if (dat_8c751dfe) {
        }
    }
}

void event_message_show(const char *text)
{
    struct event_thread *thread = thread_get_data(NULL);
    struct bitmap *window = &thread->window;

#ifdef GCC
    /* a skip shows no message, and clears the window as a fade out would */
    if (event_skipping) {
        window->text_color.argb.a = window->back_color.argb.a = 0;
        return;
    }
#endif
    if (window->pixels == NULL) {
        bitmap_init_window(window);
        window->mode = BITMAP_EVENT;
        window->text_color.argb.a = 0;
        window->back_color.argb.a = 0;
    } else if (window->text_color.argb.a) {
        bitmap_fade_out_text(window);
    }
    bitmap_clear(window);
    bitmap_print(window, text);
    bitmap_queue(window);
    if (window->back_color.argb.a == 0) {
        bitmap_fade_in(window);
    } else {
        bitmap_fade_in_text(window);
    }
}

void event_message_wait(int steps, const char *text)
{
    steps += play_time_get();
    event_message_show(text);
    steps -= play_time_get();
    if (steps > 0) {
        event_wait(steps);
    }
}

void event_message_printf(const char *format, ...)
{
    char text[128];
    va_list args;

    va_start(args, format);
    string_vformat(text, format, args);
    va_end(args);
    event_message_show(text);
}

void event_message_wait_printf(int steps, const char *format, ...)
{
    char text[128];
    va_list args;

    va_start(args, format);
    string_vformat(text, format, args);
    va_end(args);
    steps += play_time_get();
    event_message_show(text);
    steps -= play_time_get();
    if (steps > 0) {
        event_wait(steps);
    }
}

void event_message_close(void)
{
    struct event_thread *thread = thread_get_data(NULL);
    struct bitmap *window = &thread->window;

#ifdef GCC
    /* a skip clears the window as a fade out would, at once, and releases
     * it */
    if (event_skipping) {
        window->text_color.argb.a = window->back_color.argb.a = 0;
        bitmap_release(window);
        return;
    }
#endif
    if (window->pixels != NULL) {
        bitmap_fade_out(window);
        bitmap_release(window);
    }
}

void event_message_clear(void)
{
#ifdef GCC
    /* a skip clears the text as a fade out would, at once */
    if (event_skipping) {
        ((struct event_thread *)thread_get_data(NULL))
            ->window.text_color.argb.a = 0;
        return;
    }
#endif
    bitmap_fade_out_text(
        &((struct event_thread *)thread_get_data(NULL))->window);
}

int event_message_choose(const char *text)
{
    struct event_thread *thread = thread_get_data(NULL);
    struct bitmap *window = &thread->window;
    struct thread *choice = bitmap_choose(text);
    int chosen;

    while ((chosen = thread_get_status(choice)) == 0) {
        bitmap_draw(window);
        thread_yield();
    }
    thread_destroy(choice);
    return chosen;
}

Sint32 event_load_texlist(NJS_TEXLIST *texlist)
{
    return njLoadTexture(texlist);
}

Sint32 event_release_texlist(NJS_TEXLIST *texlist)
{
    return njReleaseTexture(texlist);
}

struct task *event_get_player(int player)
{
    return player_get_task(player);
}

void event_place_actor(struct task **slot, Float x, Float y, Float z,
                       Angle ang_x, Angle ang_y, Angle ang_z)
{
    struct task_work *work;

    event_kill_task(slot);
    *slot = event_actor_create();
    work = (*slot)->work;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    work->ang.x = ang_x;
    work->ang.y = ang_y;
    work->ang.z = ang_z;
}

void event_place_task(struct task **slot, struct task *(*create)(void), Float x,
                      Float y, Float z, Angle ang_x, Angle ang_y, Angle ang_z)
{
    struct task_work *work;

    event_kill_task(slot);
    *slot = create();
    work = (*slot)->work;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    work->ang.x = ang_x;
    work->ang.y = ang_y;
    work->ang.z = ang_z;
}

void event_kill_task(struct task **slot)
{
    if (*slot) {
        task_kill(*slot);
    }
    *slot = NULL;
}

void event_clear_task(struct task *task)
{
    if (task == NULL) {
        return;
    }
    event_clear_motions(task);
    event_clear_paths(task);
    event_look_clear(task);
    event_run_clear(task);
    event_talk_stop(task);
}

void event_set_pos(struct task *task, Float x, Float y, Float z)
{
    struct task_work *work;

    if (task == NULL) {
        return;
    }
    work = task->work;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    if (work->collision != NULL && work->collision->kind == COLLISION_PLAYER) {
        player_set_history(PLAYER_VARS(work)->player, 0, &work->pos, NULL);
    }
}

void event_set_ang(struct task *task, Angle ang_x, Angle ang_y, Angle ang_z)
{
    struct task_work *work;

    if (task == NULL) {
        return;
    }
    work = task->work;
    work->ang.x = ang_x;
    work->ang.y = NJM_DEG_ANG(90) - ang_y;
    work->ang.z = ang_z;
}

void event_set_flags(struct task *task, int flags)
{
    if (task == NULL) {
        return;
    }
    task->work->event->flags = flags;
}

void event_set_offset(struct task *task, Float x, Float y, Float z)
{
    struct task_work *work;
    struct event_work *event;
    struct collision_shape *shape;

    if (task == NULL) {
        return;
    }
    work = task->work;
    event = work->event;
    shape = work->collision->shapes;
    event->offset.x = x;
    shape->center.x = x;
    event->offset.y = y;
    shape->center.y = y;
    event->offset.z = z;
    shape->center.z = z;
}

void event_set_shadow(struct task *task, Float size)
{
    if (task == NULL) {
        return;
    }
    task->work->event->shadow = size;
}

void event_set_radius(struct task *task, Float radius)
{
    if (task == NULL) {
        return;
    }
    task->work->collision->shapes->radius = radius;
}

void event_create_player(int player, void (*step)(struct task *task), Float x,
                         Float y, Float z, Angle ang_x, Angle ang_y,
                         Angle ang_z)
{
    struct task *task;
    struct task_work *work;

    if (event_get_player(player) == NULL) {
        task = task_create(TASK_MOTION | TASK_WORK | TASK_FORCE, 1, step);
        PLAYER_VARS(task->work)->player = player;
        work = task->work;
        work->pos.x = x;
        work->pos.y = y;
        work->pos.z = z;
        work->ang.x = ang_x;
        work->ang.y = NJM_DEG_ANG(90) - ang_y;
        work->ang.z = ang_z;
    }
}

void event_kill_player(int player)
{
    struct task *task = event_get_player(player);

    if (task != NULL) {
        task_kill(task);
    }
}

void event_reset_player(int player)
{
    struct task *task = event_get_player(player);
    struct task_work *work;
    struct player *record;

    if (task != NULL) {
        event_clear_task(task);
        player_set_mode(player, PLAYER_MODE_RESET);
        player_set_speed(player, 0.0F, 0.0F, 0.0F);
        work = player_work[player];
        if (work->flags & PLAYER_HOLDING) {
            record = player_get(player);
            work->flags &= ~PLAYER_HOLDING;
            record->held->work->flags &= ~CARRY_HELD;
            record->held = NULL;
        }
        switch (PLAYER_VARS(task->work)->player) {
        case 0:
            player_work[player]->flags &= ~SONIC_CHARGED;
            break;
        }
    }
}

void event_play_action(struct task *task, NJS_ACTION *action,
                       NJS_TEXLIST *texlist, Float speed, int flags, int blend)
{
    if (task != NULL) {
        event_motions_add_action(task->work, action, texlist, speed, flags,
                                 blend);
    }
}

void event_play_motion(struct task *task, NJS_OBJECT *object,
                       NJS_MOTION *motion, NJS_TEXLIST *texlist, Float speed,
                       int flags, int blend)
{
    if (task != NULL) {
        event_motions_add(task->work, object, motion, texlist, speed, flags,
                          blend);
    }
}

void event_clear_motions(struct task *task)
{
    if (task != NULL) {
        event_motions_clear(task->work);
    }
}

void event_wait_motions(struct task *task)
{
    if (task != NULL) {
        while (task->work->event->motions != NULL) {
            event_wait(1);
#ifdef GCC
            /* a skip's wait lasts one step here too, with the motions
             * still queued */
            if (event_skipping) {
                break;
            }
#endif
        }
    }
}

void event_play_path(struct task *task, struct event_path *path, Float speed,
                     int flags)
{
    if (task != NULL) {
        event_paths_add(task->work, path, speed, flags);
    }
}

void event_clear_paths(struct task *task)
{
    if (task != NULL) {
        event_paths_clear(task->work);
    }
}

void event_wait_paths(struct task *task)
{
    if (task != NULL) {
        while (task->work->event->paths != NULL) {
            event_wait(1);
#ifdef GCC
            /* a skip's wait lasts one step here too, with the paths
             * still queued */
            if (event_skipping) {
                break;
            }
#endif
        }
    }
}

void fun_8c04d0c6(struct task *task, int value)
{
    if (task != NULL) {
        task->work->event->unk_01 = value;
    }
}

/* the port reads key as if from a pad */
static void event_pad_set(int port, const struct event_pad_key *key)
{
    Uint32 on = key->buttons | PDD_DGT_ST;

    pad_records[port].id = PDD_DEV_CONTROLLER;
    pad_records[port].press = on & ~pad_records[0].old;
    pad_records[port].release = pad_records[0].old & ~on;
    pad_records[port].on = on;
    pad_records[port].off = ~on;
    pad_sticks[port].ang = key->ang << 8;
    pad_sticks[port].stroke = key->stroke * (1.0F / 255.0F);
}

/* a pad's thread: a key of the recording each step */
static void event_pad_thread(void)
{
    struct pad_thread *thread = thread_get_data(NULL);
    const struct event_pad_key *key = thread->recording->keys;
    int i;

    for (i = thread->recording->count; i != 0; i--) {
#ifdef GCC
        /* a skip drops the recording, which would drive the player on into
         * EVENT_END. the next pad_scan replaces the last key it set */
        if (event_skipping) {
            break;
        }
#endif
        event_pad_set(thread->port, key++);
        thread_yield();
    }
}

static const struct event_pad_key event_pad_none = {0, 0, 0};

void event_play_pad(int port, const struct event_pad_recording *recording)
{
    struct thread *child;
    struct pad_thread *thread;

    event_wait(1);
    if ((child = thread_create_child(NULL, (void *)event_pad_thread, NULL,
                                     0)) != NULL) {
        thread = thread_get_data(child);
        thread->port = port;
        thread->recording = recording;
    }
    event_pad_set(port, &event_pad_none);
    event_wait(1);
}

void event_wait_start_released(int port)
{
    PDS_PERIPHERAL *pad = &pad_records[port];
    /* @hack spelling: the button through a local, which SHC keeps in R13
     * (MOV.L R13,@-R15 at 0x8c04d1da); tested as a constant, the function
     * runs 4 bytes short and 17 of 22 words differ */
    Uint32 button = PDD_DGT_ST;

    /* @hack spelling: a jump to the loop's test, which enters it there; as a
     * while loop, the function runs 8 bytes long and 19 of 22 words differ */
    goto test;
    do {
        event_wait(1);
    test:;
    } while ((pad->on | pad->old) & button);
}

void event_look_clear(struct task *task)
{
    if (task) {
        task->work->event->look = EVENT_LOOK_NONE;
    }
}

void event_look_at_task(struct task *task, struct task *target, Float x,
                        Float y, Float z)
{
    struct event_work *event;

    if (task != NULL) {
        event = task->work->event;
        event->look = EVENT_LOOK_TASK;
        event->look_task = target;
        event->look_pos.x = x;
        event->look_pos.y = y;
        event->look_pos.z = z;
    }
}

void event_look_at(struct task *task, Float x, Float y, Float z)
{
    struct event_work *event;

    if (task != NULL) {
        event = task->work->event;
        event->look = EVENT_LOOK_POINT;
        event->look_pos.x = x;
        event->look_pos.y = y;
        event->look_pos.z = z;
    }
}

void event_look_angles(struct task *task, Angle ang_x, Angle ang_y, Angle ang_z)
{
    struct event_work *event;

    if (task != NULL) {
        event = task->work->event;
        event->look = EVENT_LOOK_ANGLES;
        event->look_ang.x = ang_x;
        event->look_ang.y = ang_y;
        event->look_ang.z = ang_z;
    }
}

void event_run_clear(struct task *task)
{
    struct event_work *event;

    if (task != NULL) {
        event = task->work->event;
        event->action = EVENT_ACTION_NONE;
        event->speed = 0.0F;
        event->accel = 0.0F;
    }
}

void event_run_to(struct task *task, Float x, Float y, Float z)
{
    struct event_work *event;

    if (task != NULL) {
        event = task->work->event;
        event->action = EVENT_ACTION_RUN;
        event->move.pos.x = x;
        event->move.pos.y = y;
        event->move.pos.z = z;
        event->speed = 0.0F;
        event->accel = 0.0F;
    }
}

void event_run_to_speed(struct task *task, Float x, Float y, Float z,
                        Float speed, Float accel)
{
    struct event_work *event;

    if (task != NULL) {
        event = task->work->event;
        event->action = EVENT_ACTION_RUN;
        event->move.pos.x = x;
        event->move.pos.y = y;
        event->move.pos.z = z;
        event->speed = speed;
        event->accel = accel;
    }
}

void event_turn_by(struct task *task, Angle ang_x, Angle ang_y, Angle ang_z)
{
    struct event_work *event;

    if (task != NULL) {
        event = task->work->event;
        event->action = EVENT_ACTION_TURN_BY;
        event->move.ang.x = ang_x;
        event->move.ang.y = ang_y;
        event->move.ang.z = ang_z;
    }
}

void event_turn_to(struct task *task, Angle ang_y)
{
    struct event_work *event;

    if (task != NULL) {
        event = task->work->event;
        event->action = EVENT_ACTION_TURN_TO;
        event->move.ang.y = ang_y;
    }
}

void event_run_wait(struct task *task)
{
    struct event_work *event;

    if (task != NULL) {
        event = task->work->event;
        while (event->action != EVENT_ACTION_NONE) {
            event_wait(1);
        }
    }
}

/* the value an eased move between from and to reaches after step of its
 * steps. any other ease returns ease itself */
static int event_ease_int(enum event_ease ease, int steps, int step, int from,
                          int to)
{
    Float share;

    switch (ease) {
    case EVENT_EASE_LINEAR:
        return ((to - from) * step / steps) + from;
    case EVENT_EASE_SMOOTH:
        share = (1.0F - njCos((step << 15) / steps)) * 0.5F;
        return (int)((share * (to - from)) + 0.5F) + from;
    }
    /* @bug uninitialized: returns no value for any other ease. the caller
     * takes R0, which still holds ease, copied there for the switch
     * (MOV R4,R0 at 0x8c04d37a), as the function branches out (0x8c04d38e) */
#ifdef GCC
    /* repairs the missing return the tag names: any other ease moves
     * linearly under GCC */
    return ((to - from) * step / steps) + from;
#endif
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}

/* event_ease_int for a float. any other ease returns
 * from * (to - from) + from */
static Float event_ease_float(enum event_ease ease, int steps, int step,
                              Float from, Float to)
{
    Float share;

    switch (ease) {
    case EVENT_EASE_LINEAR:
        share = (Float)step / (Float)steps;
        break;
    case EVENT_EASE_SMOOTH:
        share = (1.0F - njCos((step << 15) / steps)) * 0.5F;
        break;
#ifdef GCC
    /* repairs the unset share the tag below names: any other ease moves
     * linearly under GCC */
    default:
        share = (Float)step / (Float)steps;
        break;
#endif
    }
    /* @bug uninitialized: for any other ease, no assignment reaches share.
     * SHC keeps share in FR4, which still holds from as the function was
     * entered (0x8c04d418 to 0x8c04d432), so the result is
     * from * (to - from) + from */
    return (share * (to - from)) + from;
}

/* event_ease_int, the short way round */
static int event_ease_ang(enum event_ease ease, int steps, int step, int from,
                          int to)
{
    Float share;

    switch (ease) {
    case EVENT_EASE_LINEAR:
        return ((Sint16)(to - from) * step / steps) + from;
    case EVENT_EASE_SMOOTH:
        share = (1.0F - njCos((step << 15) / steps)) * 0.5F;
        return (int)((share * (Sint16)(to - from)) + 0.5F) + from;
    }
    /* @bug uninitialized: returns no value for any other ease. the caller
     * takes R0, which still holds ease, as in event_ease_int (0x8c04d48a,
     * 0x8c04d49e) */
#ifdef GCC
    /* repairs the missing return the tag names: any other ease moves
     * linearly under GCC */
    return ((Sint16)(to - from) * step / steps) + from;
#endif
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}

/* set pos_to and ang_to part of the way from one key of the path to the
 * next, and advance path_frame by the speed. return TRUE once path_frame
 * reaches the last key, and otherwise FALSE */
static Bool event_camera_step_path(void)
{
    struct event_camera_path *path = event_camera.path;
    Float count = path->count;
    Float frame = event_camera.path_frame;
    int index = (int)floorf(frame);
    Float part = njFraction(frame);
    struct event_camera_key *next = path->keys;
    struct event_camera_key *key = &next[index];

    if (path->count > (Float)(index + 1)) {
        next = key + 1;
    }
    event_camera.pos_to.x = key->pos.x + (part * (next->pos.x - key->pos.x));
    event_camera.pos_to.y = key->pos.y + (part * (next->pos.y - key->pos.y));
    event_camera.pos_to.z = key->pos.z + (part * (next->pos.z - key->pos.z));
    /* NOLINTBEGIN(bugprone-incorrect-roundings) */
    event_camera.ang_to.x =
        (int)(key->ang_x + (part * (Sint16)(next->ang_x - key->ang_x)) + 0.5F);
    event_camera.ang_to.y =
        (int)(key->ang_y + (part * (Sint16)(next->ang_y - key->ang_y)) + 0.5F);
    event_camera.ang_to.z =
        (int)(key->ang_z + (part * (Sint16)(next->ang_z - key->ang_z)) + 0.5F);
    /* NOLINTEND(bugprone-incorrect-roundings) */
    frame += event_camera.path_speed;
    event_camera.path_frame = frame;
    return frame >= count - 1.0F;
}

void event_camera_take(void)
{
    struct event_camera *camera;
    struct event_thread *thread = thread_get_data(NULL);
    int *word;
    int i;

    /* @hack spelling: event_camera through a local, which SHC keeps in R14
     * (MOV.L R14,@-R15 at 0x8c04d660); by name, the function runs 2 bytes
     * short and 76 of 78 words differ */
    camera = &event_camera;
    camera_override(camera_step_event, 0, 0);
    word = (int *)camera;
    for (i = sizeof(*camera) / 4; i != 0; i--) {
        *word++ = 0;
    }
    camera->owner = thread->event;
    camera->pos_to = camera->pos_from = camera->pos = camera_work->pos;
    camera->ang_to = camera->ang_from = camera->ang = camera_work->ang;
    camera->look = EVENT_CAMERA_LOOK_NONE;
    camera->fov = FOV;
    event_camera_hold = 0;
}

void event_camera_release(void)
{
    struct event_camera *camera;
    struct event_thread *thread = thread_get_data(NULL);

    /* @hack spelling: event_camera through a local, which loads its address
     * into R4 and keeps the thread's bytes in R5 (0x8c04d704, 0x8c04d708); by
     * name, the two swap places and 5 of 19 words differ */
    camera = &event_camera;
    if (camera->owner == thread->event) {
        camera->owner = NULL;
        camera_restore();
    }
}

/* move pos_to as the follow mode says, then move pos a step of its ease
 * toward pos_to */
static void event_camera_step_pos(void)
{
    NJS_POINT3 offset;
    NJS_POINT3 point;
    /* @hack spelling: event_camera through a local, which SHC keeps in R14 and
     * reads through @(R0,R14) (0x8c04d772 to 0x8c04d778); by name, the
     * function runs 22 bytes short and 492 of 525 words differ */
    struct event_camera *camera = &event_camera;
    struct task_work *work;
    enum event_ease ease;
    int step;
    int steps;

    switch (camera->follow) {
    case EVENT_CAMERA_FOLLOW:
        work = camera->follow_task->work;
        camera->pos_to.x += work->pos.x - camera->follow_pos.x;
        camera->pos_to.y += work->pos.y - camera->follow_pos.y;
        camera->pos_to.z += work->pos.z - camera->follow_pos.z;
        camera->follow_pos = work->pos;
        camera->follow_ang = work->ang;
        break;
    case EVENT_CAMERA_FOLLOW_HEADING: {
        NJS_MATRIX matrix;

        work = camera->follow_task->work;
        offset.x = camera->pos_to.x - camera->follow_pos.x;
        offset.y = camera->pos_to.y - camera->follow_pos.y;
        offset.z = camera->pos_to.z - camera->follow_pos.z;
        njUnitMatrix(&matrix);
        TURN_MATRIX(njRotateY, &matrix, camera->follow_ang.y - work->ang.y);
        njCalcPoint(&matrix, &offset, &point);
        camera->pos_to.x = work->pos.x + point.x;
        camera->pos_to.y = work->pos.y + point.y;
        camera->pos_to.z = work->pos.z + point.z;
        camera->follow_pos = work->pos;
        camera->follow_ang = work->ang;
        break;
    }
    case EVENT_CAMERA_FOLLOW_LOCAL: {
        NJS_MATRIX matrix;

        work = camera->follow_task->work;
        offset.x = camera->pos_to.x - camera->follow_pos.x;
        offset.y = camera->pos_to.y - camera->follow_pos.y;
        offset.z = camera->pos_to.z - camera->follow_pos.z;
        njUnitMatrix(&matrix);
        TURN_MATRIX(njRotateY, &matrix,
                    camera->follow_ang.y - NJM_DEG_ANG(180));
        TURN_MATRIX(njRotateX, &matrix, -camera->follow_ang.x);
        TURN_MATRIX(njRotateZ, &matrix, -camera->follow_ang.z);
        njCalcPoint(&matrix, &offset, &point);
        njUnitMatrix(&matrix);
        TURN_MATRIX(njRotateZ, &matrix, work->ang.z);
        TURN_MATRIX(njRotateX, &matrix, work->ang.x);
        TURN_MATRIX(njRotateY, &matrix, NJM_DEG_ANG(180) - work->ang.y);
        njCalcPoint(&matrix, &point, &offset);
        camera->pos_to.x = work->pos.x + offset.x;
        camera->pos_to.y = work->pos.y + offset.y;
        camera->pos_to.z = work->pos.z + offset.z;
        camera->follow_pos = work->pos;
        camera->follow_ang = work->ang;
        break;
    }
    case EVENT_CAMERA_FOLLOW_ORBIT: {
        struct angles orbit;
        NJS_MATRIX matrix;

        work = camera->follow_task->work;
        steps = camera->orbit_steps;
        step = camera->orbit_step;
        if (step < steps) {
            ease = camera->pos_ease;
            step++;
            camera->orbit_step = step;
            offset.z = event_ease_float(ease, steps, step,
                                        camera->orbit_start_distance,
                                        camera->orbit_end_distance);
            orbit.x = event_ease_int(ease, steps, step, camera->follow_ang.x,
                                     camera->orbit_end_ang.x);
            orbit.y = event_ease_int(ease, steps, step, camera->follow_ang.y,
                                     camera->orbit_end_ang.y);
            orbit.z = event_ease_int(ease, steps, step, camera->follow_ang.z,
                                     camera->orbit_end_ang.z);
        } else {
            offset.z = camera->orbit_end_distance;
            orbit = camera->orbit_end_ang;
        }
        offset.x = 0.0F;
        offset.y = 0.0F;
        njUnitMatrix(&matrix);
        TURN_MATRIX(njRotateZ, &matrix, orbit.z);
        TURN_MATRIX(njRotateX, &matrix, orbit.x);
        TURN_MATRIX(njRotateY, &matrix, orbit.y);
        njCalcPoint(&matrix, &offset, &point);
        camera->pos_to.x = work->pos.x + point.x;
        camera->pos_to.y = work->pos.y + camera->follow_pos.y + point.y;
        camera->pos_to.z = work->pos.z + point.z;
        break;
    }
    case EVENT_CAMERA_FOLLOW_PATH:
        if (event_camera_step_path()) {
            camera->follow = EVENT_CAMERA_FOLLOW_NONE;
        }
        break;
    }
    step = camera->pos_step;
    steps = camera->pos_steps;
    if (step < steps) {
        ease = camera->pos_ease;
        step++;
        camera->pos_step = step;
        camera->pos.x = event_ease_float(ease, steps, step, camera->pos_from.x,
                                         camera->pos_to.x);
        camera->pos.y = event_ease_float(ease, steps, step, camera->pos_from.y,
                                         camera->pos_to.y);
        camera->pos.z = event_ease_float(ease, steps, step, camera->pos_from.z,
                                         camera->pos_to.z);
    } else {
        camera->pos.x = camera->pos_to.x;
        camera->pos.y = camera->pos_to.y;
        camera->pos.z = camera->pos_to.z;
    }
}

/* the y and x angles that look from the first point at the second */
static void event_camera_aim(NJS_POINT3 *from, NJS_POINT3 *to,
                             struct angles *ang)
{
    Float dx = from->x - to->x;
    Float dz = from->z - to->z;
    Float distance = njSqrt((dx * dx) + (dz * dz));

    ang->y = (Angle)(atan2f(dx, dz) * ANGLE_PER_RADIAN);
    ang->x = (Angle)(atan2f(to->y - from->y, distance) * ANGLE_PER_RADIAN);
}

/* aim ang_to as the look says, then move ang a step of its ease toward
 * ang_to */
static void event_camera_step_ang(void)
{
    NJS_POINT3 offset;
    NJS_POINT3 point;
    NJS_MATRIX matrix;
    struct task_work *work;
    int step;
    int steps;
    enum event_ease ease;

    switch (event_camera.look) {
    case EVENT_CAMERA_LOOK_POINT:
        event_camera_aim(&event_camera.pos, &event_camera.look_pos,
                         &event_camera.ang_to);
        break;
    case EVENT_CAMERA_LOOK_TASK:
        work = event_camera.look_task->work;
        offset = event_camera.look_pos;
        njUnitMatrix(&matrix);
        TURN_MATRIX(njRotateZ, &matrix, work->ang.z);
        TURN_MATRIX(njRotateX, &matrix, work->ang.x);
        TURN_MATRIX(njRotateY, &matrix, work->ang.y + NJM_DEG_ANG(90));
        njCalcPoint(&matrix, &offset, &point);
        point.x += work->pos.x;
        point.y += work->pos.y;
        point.z += work->pos.z;
        event_camera_aim(&event_camera.pos, &point, &event_camera.ang_to);
        break;
    }
    step = event_camera.ang_step;
    steps = event_camera.ang_steps;
    if (step < steps) {
        ease = event_camera.ang_ease;
        step++;
        event_camera.ang_step = step;
        event_camera.ang.x = event_ease_ang(
            ease, steps, step, event_camera.ang_from.x, event_camera.ang_to.x);
        event_camera.ang.y = event_ease_ang(
            ease, steps, step, event_camera.ang_from.y, event_camera.ang_to.y);
        event_camera.ang.z = event_ease_ang(
            ease, steps, step, event_camera.ang_from.z, event_camera.ang_to.z);
    } else {
        event_camera.ang.x = event_camera.ang_to.x;
        event_camera.ang.y = event_camera.ang_to.y;
        event_camera.ang.z = event_camera.ang_to.z;
    }
}

/* move fov a step of its ease, and set the perspective to it. once the
 * ease has run, nothing changes */
static void event_camera_step_fov(void)
{
    int step = event_camera.fov_step;
    int steps = event_camera.fov_steps;

    if (step < steps) {
        event_camera.fov_step = ++step;
        event_camera.fov =
            event_ease_int(event_camera.fov_ease, steps, step,
                           event_camera.fov_from, event_camera.fov_to);
        njSetPerspective(event_camera.fov);
    }
}

void event_camera_step(void)
{
    if (event_camera_hold == 0) {
        event_camera_step_pos();
        event_camera_step_ang();
        event_camera_step_fov();
    }
    camera_work->pos = event_camera.pos;
    camera_work->ang = event_camera.ang;
}

/* begin a move of pos from where it is to pos_to over steps steps, as
 * event_camera_ang_begin does for ang. 0 steps arrives at once */
static void event_camera_pos_begin(int steps)
{
    event_camera.pos_from = event_camera.pos;
    event_camera.pos_steps = steps;
    event_camera.pos_step = 0;
    if (steps == 0) {
        event_camera_step_pos();
    }
}

static void event_camera_ang_begin(int steps)
{
    event_camera.ang_from = event_camera.ang;
    event_camera.ang_steps = steps;
    event_camera.ang_step = 0;
    if (steps == 0) {
        event_camera_step_ang();
    }
}

void event_camera_set_ang(enum event_ease ease, int steps, Angle ang_x,
                          Angle ang_y, Angle ang_z)
{
    event_camera.ang_ease = ease;
    event_camera.ang_to.x = ang_x;
    event_camera.ang_to.y = ang_y;
    event_camera.ang_to.z = ang_z;
    event_camera_ang_begin(steps);
}

void event_camera_set_pos(enum event_ease ease, int steps, Float x, Float y,
                          Float z)
{
    event_camera.pos_ease = ease;
    event_camera.pos_to.x = x;
    event_camera.pos_to.y = y;
    event_camera.pos_to.z = z;
    event_camera_pos_begin(steps);
}

void event_camera_set_pos_offset(enum event_ease ease, int steps,
                                 struct task *task, Float x, Float y, Float z)
{
    /* @hack spelling: event_camera through a local, which loads its address
     * into R5 after the work's into R6 (0x8c04dfb6, 0x8c04dfb8); by name, the
     * two swap places and 11 of 22 words differ */
    struct event_camera *camera = &event_camera;
    struct task_work *work = task->work;

    camera->pos_ease = ease;
    camera->pos_to.x = work->pos.x + x;
    camera->pos_to.y = work->pos.y + y;
    camera->pos_to.z = work->pos.z + z;
    event_camera_pos_begin(steps);
}

void event_camera_set_pos_local(enum event_ease ease, int steps,
                                struct task *task, Float x, Float y, Float z)
{
    NJS_MATRIX matrix;
    NJS_POINT3 offset;
    NJS_POINT3 point;
    struct task_work *work = task->work;

    event_camera.pos_ease = ease;
    offset.x = x;
    offset.y = y;
    offset.z = z;
    njUnitMatrix(&matrix);
    TURN_MATRIX(njRotateZ, &matrix, work->ang.z);
    TURN_MATRIX(njRotateX, &matrix, work->ang.x);
    TURN_MATRIX(njRotateY, &matrix, NJM_DEG_ANG(90) - work->ang.y);
    njCalcPoint(&matrix, &offset, &point);
    event_camera.pos_to.x = work->pos.x + point.x;
    event_camera.pos_to.y = work->pos.y + point.y;
    event_camera.pos_to.z = work->pos.z + point.z;
    event_camera_pos_begin(steps);
}

void event_camera_set_pos_heading(enum event_ease ease, int steps,
                                  struct task *task, Float x, Float y, Float z)
{
    NJS_MATRIX matrix;
    NJS_POINT3 offset;
    NJS_POINT3 point;
    struct task_work *work = task->work;

    event_camera.pos_ease = ease;
    offset.x = x;
    offset.y = y;
    offset.z = z;
    njUnitMatrix(&matrix);
    TURN_MATRIX(njRotateY, &matrix, NJM_DEG_ANG(90) - work->ang.y);
    njCalcPoint(&matrix, &offset, &point);
    event_camera.pos_to.x = work->pos.x + point.x;
    event_camera.pos_to.y = work->pos.y + point.y;
    event_camera.pos_to.z = work->pos.z + point.z;
    event_camera_pos_begin(steps);
}

void event_camera_look_clear(void)
{
    /* @hack spelling: event_camera through a local, which stores through
     * @(R0,R4) with the member's offset in R0 (0x8c04e164, 0x8c04e166); by
     * name, SHC loads the member's own address, the function runs 2 bytes
     * short and 5 of 5 words differ */
    struct event_camera *camera = &event_camera;

    camera->look = EVENT_CAMERA_LOOK_NONE;
}

void event_camera_look_at(enum event_ease ease, int steps, Float x, Float y,
                          Float z, Angle roll)
{
    event_camera.ang_ease = ease;
    event_camera.look = EVENT_CAMERA_LOOK_POINT;
    event_camera.look_pos.x = x;
    event_camera.look_pos.y = y;
    event_camera.look_pos.z = z;
    event_camera.ang_to.z = roll;
    event_camera_ang_begin(steps);
}

void event_camera_look_at_task(enum event_ease ease, int steps,
                               struct task *task, Float x, Float y, Float z,
                               Angle roll)
{
    event_camera.ang_ease = ease;
    event_camera.look = EVENT_CAMERA_LOOK_TASK;
    event_camera.look_task = task;
    event_camera.look_pos.x = x;
    event_camera.look_pos.y = y;
    event_camera.look_pos.z = z;
    event_camera.ang_to.z = roll;
    event_camera_ang_begin(steps);
}

void event_camera_follow_clear(void)
{
    /* @hack spelling: event_camera through a local, as in
     * event_camera_look_clear (0x8c04e1e8, 0x8c04e1ea); by name, the
     * function runs 2 bytes short and 5 of 5 words differ */
    struct event_camera *camera = &event_camera;

    camera->follow = EVENT_CAMERA_FOLLOW_NONE;
}

void event_camera_follow(struct task *task)
{
    struct task_work *work = task->work;

    event_camera.follow = EVENT_CAMERA_FOLLOW;
    event_camera.follow_task = task;
    event_camera.follow_pos = work->pos;
    event_camera.follow_ang = work->ang;
}

void event_camera_follow_local(struct task *task)
{
    struct task_work *work = task->work;

    event_camera.follow = EVENT_CAMERA_FOLLOW_LOCAL;
    event_camera.follow_task = task;
    event_camera.follow_pos = work->pos;
    event_camera.follow_ang = work->ang;
}

void event_camera_follow_heading(struct task *task)
{
    struct task_work *work = task->work;

    event_camera.follow = EVENT_CAMERA_FOLLOW_HEADING;
    event_camera.follow_task = task;
    event_camera.follow_pos = work->pos;
    event_camera.follow_ang = work->ang;
}

void event_camera_orbit(enum event_ease ease, int steps, struct task *task,
                        Float height, Angle ang_x, Angle ang_y, Angle ang_z,
                        Angle to_x, Angle to_y, Angle to_z, Float distance,
                        Float to_distance)
{
    event_camera.pos_ease = ease;
    event_camera.follow = EVENT_CAMERA_FOLLOW_ORBIT;
    event_camera.follow_task = task;
    event_camera.orbit_steps = steps;
    event_camera.orbit_step = 0;
    event_camera.follow_ang.x = ang_x;
    event_camera.follow_ang.y = ang_y;
    event_camera.follow_ang.z = ang_z;
    event_camera.orbit_start_distance = distance;
    event_camera.orbit_end_ang.x = to_x;
    event_camera.orbit_end_ang.y = to_y;
    event_camera.orbit_end_ang.z = to_z;
    event_camera.orbit_end_distance = to_distance;
    event_camera.follow_pos.y = height;
    event_camera.pos_step = 0;
    event_camera.pos_steps = 0;
}

void event_camera_set_fov(Sint8 ease, int steps, Angle fov)
{
    event_camera.fov_ease = ease;
    event_camera.fov_steps = steps;
    event_camera.fov_step = 0;
    event_camera.fov_from = event_camera.fov;
    event_camera.fov_to = fov;
    if (steps == 0) {
        event_camera_step_fov();
    }
}

void event_camera_set_path(struct event_camera_path *path, Float speed)
{
    event_camera.follow = EVENT_CAMERA_FOLLOW_PATH;
    event_camera.path = path;
    event_camera.path_frame = 0.0F;
    event_camera.path_speed = speed;
}

void event_camera_wait_path(void)
{
    while (event_camera.follow == EVENT_CAMERA_FOLLOW_PATH) {
        event_wait(1);
    }
}

void event_talk_start(struct task *task, const char *text)
{
    struct task_work *work;

    if (task != NULL) {
        /* @hack spelling: the work through a local, which tests task in R4
         * and loads the work straight from it (0x8c04e3a8 to 0x8c04e3ac);
         * read in the call, the function runs 2 bytes long and 10 of 11 words
         * differ */
        work = task->work;
        event_mouth_start(work, text);
    }
}

void event_talk_stop(struct task *task)
{
    struct task_work *work;

    if (task) {
        /* @hack spelling: the work through a local, which tests task in R4
         * and ends in a tail jump (0x8c04e3bc to 0x8c04e3c2); read in the
         * call, SHC makes a frame, the function runs 10 bytes long and 7 of
         * 7 words differ */
        work = task->work;
        event_mouth_stop(work);
    }
}

void event_voice_wait(void)
{
    Sint32 state = stream_voice_state();

    while (state != ADXT_STAT_STOP && state != ADXT_STAT_PLAYEND) {
        event_wait(1);
        state = stream_voice_state();
    }
}

void event_voice_play(int voice)
{
    event_voice_wait();
    stream_request_voice(voice);
}

void event_voice_stop(void)
{
    stream_stop_voice();
}
