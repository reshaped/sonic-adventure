/* an event's waves: rings on the water, one at a time or from emitters */
#ifndef EVENT_WAVE_H
#define EVENT_WAVE_H
#include <ninja.h>
#include <sg_xpt.h>

/* the ring's texture, which the scripts load from WAVE7_WA */
extern NJS_TEXLIST event_wave_texlist;

/* the emitters' slots. a slot must lie from 0 to EVENT_WAVE_EMITTERS - 1,
 * unchecked */
#define EVENT_WAVE_EMITTERS 10

/* a wave at x, y, z, scaled by scale in x and z. it plays the ring's
 * motion at speed frames a step, then ends. when no task can be made, it
 * reads through null */
void event_wave_create(Float x, Float y, Float z, Float speed, Float scale);
void event_wave_load_textures(void);
void event_wave_release_textures(void);
/* an emitter in slot at x, y, z. it runs for steps + 1 steps. on every
 * interval-th of them, it starts a wave of speed and scale. interval must
 * not be 0. an emitter already in slot runs on, out of reach. when no task
 * can be made, it reads through null */
void event_wave_emitter_create(Float x, Float y, Float z, Float speed,
                               Float scale, int interval, int steps, int slot);
/* kill the emitter in slot, if the slot holds one. an emitter that has
 * ended still holds its slot, so call this only while it runs */
void event_wave_emitter_kill(int slot);

#endif
