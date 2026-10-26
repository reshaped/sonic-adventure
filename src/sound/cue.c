/* sound cues: music now or after a wait, and effects that fade with distance */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "sound/cue.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/task_desc.h"

struct music_later {
    int music;
    int wait; /* the steps and draws to count before the music starts */
};

/* the updates sound_play_for and sound_play_at_for give a kept effect */
#define KEEP_UPDATES 2

#define LATER(task) ((struct music_later *)(task)->motion)

static void music_later_init(struct task *task, void *arg);
static void music_later_step(struct task *task);
static void music_later_destroy(struct task *task);

/* the task counts down as it steps, and again as it draws */
static const struct task_desc music_later_desc = {
    sizeof(struct music_later), 0,
    music_later_init,           music_later_step,
    music_later_step,           music_later_destroy};

static const struct sound_falloff sound_falloff_default = {0, -127, 20.0F,
                                                           600.0F};

/* the music_start_later task that waits, or NULL */
static struct task *music_later_task = NULL;

struct task *music_start_later(int music, int wait)
{
    struct task *task;
    struct music_later later;

    if (music_later_task == NULL) {
        later.music = music;
        later.wait = wait;
        task = task_create_desc(0, &music_later_desc, &later);
        music_later_task = task;
    } else {
        task = NULL;
    }
    return task;
}

void music_start(int music)
{
    stream_reset();
    stream_request_and_save_music(music);
    if (music_later_task != NULL) {
        task_destroy(music_later_task);
    }
}

void music_end(void)
{
    stream_destroy();
    if (music_later_task != NULL) {
        task_destroy(music_later_task);
    }
}

void music_play(int music)
{
    stream_request_and_save_music(music);
}

void music_stop(void)
{
    stream_stop_music();
}

void sound_play_near(int se, NJS_POINT3 *pos,
                     const struct sound_falloff *falloff)
{
    struct task_work *camera;
    NJS_POINT3 *at;
    int volume;

    camera = camera_work;
    if (camera == NULL) {
        sound_play(se, 0, 0, 0);
    } else {
        at = &camera->pos;
        volume = sound_falloff_volume(pos, falloff);
        sound_play_at(se, 0, 0, volume, at->x, at->y, at->z);
    }
}

void sound_keep_near(int se, void *owner, NJS_POINT3 *pos,
                     const struct sound_falloff *falloff)
{
    struct task_work *camera;
    NJS_POINT3 *at;
    int volume;

    if (owner == NULL) {
        return;
    }
    volume = sound_falloff_volume(pos, falloff);
    camera = camera_work;
    if (camera == NULL) {
        sound_play_for(se, (int)owner, 1, 0, KEEP_UPDATES);
        if (volume != 0) {
            sound_set_volume((int)owner, volume);
        }
    } else {
        at = &camera->pos;
        sound_play_at_for(se, (int)owner, 1, volume, at->x, at->y, at->z,
                          KEEP_UPDATES);
    }
}

void sound_stop_near(void *owner)
{
    if (owner != NULL) {
        sound_stop_id((int)owner);
    }
}

int sound_falloff_volume(NJS_POINT3 *pos, const struct sound_falloff *falloff)
{
    int near_volume;
    int far_volume;
    int near;
    int far;
    int volume;
    Float distance;

    if (falloff == NULL) {
        falloff = &sound_falloff_default;
    }
    near_volume = falloff->near_volume;
    if (near_volume < -128) {
        near_volume = -128;
    } else if (near_volume > 127) {
        near_volume = 127;
    }
    far_volume = falloff->far_volume;
    if (far_volume < -128) {
        far_volume = -128;
    } else if (far_volume > 127) {
        far_volume = 127;
    }
    near = (int)falloff->near;
    if ((Float)near < 0.0F) {
        near = 0;
    }
    far = (int)falloff->far;
    if ((Float)far < 0.0F) {
        far = 0;
    }
    if (near > far) {
        /* @hack type: swap as a Float, which converts near to a float and back
         * (FLOAT FPUL,FR3 at 0x8c0a6e5c, FTRC FR4,FPUL at 0x8c0a6e60). near
         * holds 0 or a Float truncated to an int, which a Float holds exactly,
         * so the trip changes nothing. as an int, 25 of 115 words differ */
        Float swap = (Float)near;

        near = far;
        far = (int)swap;
    }
    if (pos == NULL || camera_work == NULL) {
        volume = near_volume;
    } else {
        distance = njDistanceP2P(pos, &camera_work->pos);
        if (distance <= (Float)near) {
            volume = near_volume;
        } else if (distance >= (Float)far) {
            volume = far_volume;
        } else {
            volume = (int)(near_volume +
                           ((far_volume - near_volume) *
                            (distance - (Float)near) / (Float)(far - near)));
        }
    }
    if (volume < -128) {
        volume = -128;
    } else if (volume > 127) {
        volume = 127;
    }
    return volume;
}

static void music_later_init(struct task *task, void *arg)
{
    struct music_later *later = LATER(task);
    struct music_later *from = arg;

    later->music = from->music;
    later->wait = from->wait;
}

static void music_later_step(struct task *task)
{
    struct music_later *later = LATER(task);

    if (later->wait > 0) {
        later->wait--;
    } else {
        music_later_task = NULL;
        music_start(later->music);
        task_destroy(task);
    }
}

static void music_later_destroy(struct task *task)
{
    music_later_task = NULL;
}
