/* the hooks' bodies. a hook is a call made at a fixed place in a run. native.h
 * declares each, and prelude.h declares those the game's C calls. the bodies
 * down to native_hook_voice_request do nothing. one that answers gives 0, but
 * the opening's gives 1 */
#include <stdint.h>

#include "native.h"

const char native_hook_usage[] = "";

int native_hook_option(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    return 0;
}

int native_hook_run_start(void)
{
    return 0;
}

void native_hook_run_end(void)
{
}

int native_hook_pads(void)
{
    return 0;
}

void native_hook_pad_scan(void *pad)
{
    (void)pad;
}

void native_hook_pad_scanned(void)
{
}

/* the opening ends into the target's act */
int native_hook_opening(void)
{
    return 1;
}

int native_hook_clock(uint32_t *count)
{
    (void)count;
    return 0;
}

void native_hook_date(uint32_t count)
{
    (void)count;
}

void native_hook_textures_released(void *texlist)
{
    (void)texlist;
}

void native_hook_stage_start(void)
{
}

void native_hook_stage_step(void)
{
}

void native_hook_frame_end(void)
{
}

void native_hook_frame_waited(void)
{
}

void native_hook_task_flags(unsigned int flags)
{
    (void)flags;
}

void native_hook_task_parent(void *parent)
{
    (void)parent;
}

void native_hook_task_alloc(void *task, int level, void *step, void *root_step)
{
    (void)task;
    (void)level;
    (void)step;
    (void)root_step;
}

void native_hook_task_destroy(void *task)
{
    (void)task;
}

void native_hook_heap_free(void *memory)
{
    (void)memory;
}

void native_hook_texture_load(const char *name)
{
    (void)name;
}

void native_hook_music_request(int music)
{
    (void)music;
}

void native_hook_voice_request(int voice)
{
    (void)voice;
}

/* the hooks round the loads and the sound calls. marks.py renames the game's
 * own definition of each with the prefix native_real_ */
int native_real_loader_read_prs(char *name, void *dest);
int native_real_loader_read_file(char *name, void *dest);
int native_real_sound_play(int, int, int, int);
int native_real_sound_play_at(int, int, int, int, float, float, float);
int native_real_sound_play_held(int, int, int, int);
int native_real_sound_play_for(int, int, int, int, int);
int native_real_sound_play_at_for(int, int, int, int, float, float, float, int);
int native_real_sound_play_around(int, int, int, int, float, float, float,
                                  float, int);
void native_real_sound_stop(int);
void native_real_sound_stop_id(int);
void native_real_sound_set_volume(int, int);
void native_real_sound_set_pan(int, int);
void native_real_sound_set_pitch(int, int);
void native_real_sound_mute(void);
void native_real_sound_unmute(void);

int loader_read_prs(char *name, void *dest)
{
    return native_real_loader_read_prs(name, dest);
}

int loader_read_file(char *name, void *dest)
{
    return native_real_loader_read_file(name, dest);
}

int sound_play(int se, int id, int priority, int volume)
{
    return native_real_sound_play(se, id, priority, volume);
}

int sound_play_at(int se, int id, int priority, int volume, float x, float y,
                  float z)
{
    return native_real_sound_play_at(se, id, priority, volume, x, y, z);
}

int sound_play_held(int se, int id, int priority, int volume)
{
    return native_real_sound_play_held(se, id, priority, volume);
}

int sound_play_for(int se, int id, int priority, int volume, int updates)
{
    return native_real_sound_play_for(se, id, priority, volume, updates);
}

int sound_play_at_for(int se, int id, int priority, int volume, float x,
                      float y, float z, int updates)
{
    return native_real_sound_play_at_for(se, id, priority, volume, x, y, z,
                                         updates);
}

int sound_play_around(int se, int id, int priority, int volume, float x,
                      float y, float z, float radius, int updates)
{
    return native_real_sound_play_around(se, id, priority, volume, x, y, z,
                                         radius, updates);
}

void sound_stop(int se)
{
    native_real_sound_stop(se);
}

void sound_stop_id(int id)
{
    native_real_sound_stop_id(id);
}

void sound_set_volume(int id, int volume)
{
    native_real_sound_set_volume(id, volume);
}

void sound_set_pan(int id, int pan)
{
    native_real_sound_set_pan(id, pan);
}

void sound_set_pitch(int id, int pitch)
{
    native_real_sound_set_pitch(id, pitch);
}

void sound_mute(void)
{
    native_real_sound_mute();
}

void sound_unmute(void)
{
    native_real_sound_unmute();
}
