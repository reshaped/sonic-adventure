/* sound cues: music now or after a wait, and effects that fade with distance */
#ifndef SOUND_CUE_H
#define SOUND_CUE_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* an effect's volume by its distance from the camera: near_volume up to
 * near, far_volume from far on, and linear between. sound_falloff_volume
 * counts the distances in whole units */
struct sound_falloff {
    Sint16 near_volume;
    Sint16 far_volume;
    Float near;
    Float far;
};

/* create a task whose steps and draws each count wait down by one. the
 * first to find wait at 0 calls music_start(music) and ends the task. it
 * returns the task, or NULL if one waits already or none can be made */
struct task *music_start_later(int music, int wait);
/* forget every stream request, as stream_reset does, and ask for music,
 * kept for stream_restore_music. it ends a music_start_later wait */
void music_start(int music);
/* end the streams' task, as stream_destroy does, and a music_start_later
 * wait */
void music_end(void);
/* ask for music, kept for stream_restore_music */
void music_play(int music);
/* stop the music playing, as stream_stop_music does */
void music_stop(void);
/* play se at the camera, at the volume falloff gives pos. without a camera
 * it plays at volume 0, whatever the falloff */
void sound_play_near(int se, NJS_POINT3 *pos,
                     const struct sound_falloff *falloff);
/* keep se playing for owner for at least two more updates of
 * sound_send_queue, at the camera, at the volume falloff gives pos. owner
 * is the id of sound.h's requests, and a NULL owner plays nothing */
void sound_keep_near(int se, void *owner, NJS_POINT3 *pos,
                     const struct sound_falloff *falloff);
/* stop everything playing for owner. a NULL owner stops nothing */
void sound_stop_near(void *owner);
/* the volume falloff gives pos, from -128 to 127. without a camera or a
 * pos, it is the near volume. a NULL falloff fades from 0 within 20 to
 * -127 from 600 on */
int sound_falloff_volume(NJS_POINT3 *pos, const struct sound_falloff *falloff);

#endif
