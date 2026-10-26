/* an event's sounds. a held sound is a task in a slot. its volume, pan and
 * pitch move to the values sent to it. a played sound plays once, under an
 * id from the calling thread's data */
#include <sg_xpt.h>

#include "event/play.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/thread.h"

/* one of a held sound's settings */
struct sound_level {
    int value;
    int target; /* the value it moves to */
    int steps; /* left to reach target */
};

struct held_sound {
    int id; /* the id it plays under: its task's address */
    struct sound_level volume;
    struct sound_level pan;
    struct sound_level pitch;
};

/* a held sound's work. the sound leaves its first 16 bytes unused */
struct held_sound_work {
    Uint8 unk_00[16];
    struct held_sound sound;
};

#define HELD(task) (&((struct held_sound_work *)(task)->work)->sound)

/* the count of slots event_sound_init made, and each slot's held sound or
 * null */
static int event_sound_count = 0;
static struct task **event_sounds = NULL;
/* the next played sound's id, as an offset into the thread's data */
static int event_sound_next = 0;

static void event_sound_destroy(struct task *task)
{
    struct held_sound *sound = HELD(task);

    sound_stop_id(sound->id);
}

/* move each setting with steps left 1/steps of its gap to its target. it
 * reaches the target on its last step */
static void event_sound_step(struct task *task)
{
    struct held_sound *sound = HELD(task);
    int id = sound->id;

    if (sound->volume.steps > 0) {
        int steps = sound->volume.steps;
        int value = sound->volume.value;
        int target = sound->volume.target;

        value = (((steps - 1) * value) + target) / steps;
        sound_set_volume(id, value);
        sound->volume.value = value;
        sound->volume.steps--;
    }
    if (sound->pan.steps > 0) {
        int steps = sound->pan.steps;
        int value = sound->pan.value;
        int target = sound->pan.target;

        value = (((steps - 1) * value) + target) / steps;
        sound_set_pan(id, value);
        sound->pan.value = value;
        sound->pan.steps--;
    }
    if (sound->pitch.steps > 0) {
        int steps = sound->pitch.steps;
        int value = sound->pitch.value;
        int target = sound->pitch.target;

        value = (((steps - 1) * value) + target) / steps;
        sound_set_pitch(id, value);
        sound->pitch.value = value;
        sound->pitch.steps--;
    }
}

static struct task *event_sound_hold(int se, int steps)
{
    struct task *task = task_create(TASK_WORK, 5, NULL);

    if (task != NULL) {
        int id = (int)task;
        struct held_sound *sound;

        sound_play_for(se, id, 1, 0, steps);
        task->step = event_sound_step;
        task->destructor = event_sound_destroy;
        sound = HELD(task);
        sound->id = id;
        sound->volume.value = 0;
        sound->volume.target = 0;
        sound->volume.steps = 0;
        sound->pan.value = 0;
        sound->pan.target = 0;
        sound->pan.steps = 0;
        sound->pitch.value = 0;
        sound->pitch.target = 0;
        sound->pitch.steps = 0;
    }
    return task;
}

static void event_sound_move_volume(struct task *task, int volume, int steps)
{
    struct held_sound *sound = HELD(task);

    sound->volume.target = volume;
    sound->volume.steps = steps;
    if (steps <= 0) {
        sound_set_volume(sound->id, volume);
        sound->volume.value = volume;
    }
}

static void event_sound_move_pan(struct task *task, int pan, int steps)
{
    struct held_sound *sound = HELD(task);

    sound->pan.target = pan;
    sound->pan.steps = steps;
    if (steps <= 0) {
        sound_set_pan(sound->id, pan);
        sound->pan.value = pan;
    }
}

static void event_sound_move_pitch(struct task *task, int pitch, int steps)
{
    struct held_sound *sound = HELD(task);

    sound->pitch.target = pitch;
    sound->pitch.steps = steps;
    if (steps <= 0) {
        sound_set_pitch(sound->id, pitch);
        sound->pitch.value = pitch;
    }
}

void event_sound_init(int count)
{
    /* NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion) */
    if (event_sounds == NULL) {
        event_sounds =
            (struct task **)heap_calloc(count, sizeof(struct task *));
        event_sound_count = count;
    }
}

void event_sound_stop_all(void)
{
    int i;

    /* NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion) */
    if (event_sounds != NULL) {
        for (i = 0; i < event_sound_count; i++) {
            if (event_sounds[i] != NULL) {
                task_kill(event_sounds[i]);
            }
        }
        heap_free((void *)event_sounds);
        event_sounds = NULL;
    }
}

void event_sound_start(int slot, int se, int steps)
{
    if (slot < event_sound_count && event_sounds[slot] == NULL) {
        event_sounds[slot] = event_sound_hold(se, steps);
    }
}

void event_sound_stop(int slot)
{
    if (slot < event_sound_count && event_sounds[slot] != NULL) {
        task_kill(event_sounds[slot]);
        event_sounds[slot] = NULL;
    }
}

void event_sound_fade_volume(int slot, int volume, int steps)
{
    if (slot < event_sound_count && event_sounds[slot] != NULL) {
        event_sound_move_volume(event_sounds[slot], volume, steps);
    }
}

void event_sound_fade_pan(int slot, int pan, int steps)
{
    if (slot < event_sound_count && event_sounds[slot] != NULL) {
        event_sound_move_pan(event_sounds[slot], pan, steps);
    }
}

void event_sound_fade_pitch(int slot, int pitch, int steps)
{
    if (slot < event_sound_count && event_sounds[slot] != NULL) {
        event_sound_move_pitch(event_sounds[slot], pitch, steps);
    }
}

void event_sound_play(int se, int volume, int pan, int pitch)
{
    int id = (int)thread_get_data(NULL) + event_sound_next;

    sound_play_for(se, id, 1, 0, EVENT_SOUND_PLAY_UPDATES);
    sound_set_volume(id, volume);
    sound_set_pan(id, pan);
    sound_set_pitch(id, pitch);
    event_sound_next++;
    event_sound_next &= EVENT_SOUND_PLAY_IDS - 1;
}
